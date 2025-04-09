#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <netinet/in.h>
#include "utils.h"
#include "global.h"

// struct for server datas
typedef struct {
    ServerConfig config;
    char prompt[MAX_PROMPT_SIZE];
    int *client_sockets;
    bool stderr_mode;
} Server;
// initialize server method
void initialize_server(Server *server, const char *properties_file, const char *start_prompt) {
 //  printf("TEST 1...\n");

    // Read read conf. from the prop file
    if (!read_properties(properties_file, &server->config)) {
        fprintf(stderr, "Failed to read properties file.\n");
        exit(EXIT_FAILURE);
    }

  //  printf("TEST 2...\n");


    // init prompt
    int max_prompt_size = (server->config.prompt_size < MAX_PROMPT_SIZE) ? server->config.prompt_size : MAX_PROMPT_SIZE;
  //  printf("%d",server->config.prompt_size);
   // printf("%d",MAX_PROMPT_SIZE);

 //   printf("TEST 4...\n");
    int prompt_len = strlen(start_prompt);
    if (prompt_len >= max_prompt_size) {
        fprintf(stderr, "Prompt too long. Max length: %d, Current length: %d\n",  max_prompt_size - 1, prompt_len);
        exit(EXIT_FAILURE);
    }

    // Copy start prompt to server prompt
    strncpy(server->prompt, start_prompt, max_prompt_size - 1);
    server->prompt[max_prompt_size - 1] = '\0';
  //  printf("TEST 3...\n");
    server->prompt[max_prompt_size - 1] = '\0';

    // generate actual prompt
    generate_prompt(server->prompt, &server->config);

    // Allocate memory for client sockets
    server->client_sockets = malloc(server->config.max_clients * sizeof(int));
    if (server->client_sockets == NULL) {
        perror("Failed to allocate memory for client_sockets");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < server->config.max_clients; i++) {
        server->client_sockets[i] = -1;
    }
}
// cleanup server method
void cleanup_server(Server *server) {
    if (server->client_sockets) {
        free(server->client_sockets);
        server->client_sockets = NULL;
    }
    cleanup(server->config.max_clients);
}

// start server mode
void handle_server_mode(Server *server, const char *server_address, int port) {
    const char *logfile=server->config.log_file;
    const bool stderr_mode = server->stderr_mode;

    printf("Starting server on %s:%d\n", server_address, port);

    // start server mode
    server_mode(&server->config, server->prompt, logfile,stderr_mode);
}

// start client mode
void handle_client_mode(const char *server_address, int port, Server *server, char* command) {
    // Validate address and port
    if (server_address == NULL || strlen(server_address) == 0) {
        fprintf(stderr, "Error: Server address is not specified.\n");
        exit(EXIT_FAILURE);
    }
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Error: Invalid port number %d.\n", port);
        exit(EXIT_FAILURE);
    }

    // check command
    if(command == NULL){
        printf("Connecting to server at %s:%d\n", server_address, port);
    }else{
        printf("Temp. connection to server at %s:%d\n", server_address, port);
    }
    const bool stderr_mode = server->stderr_mode;

    // if all OK, start client mode
    client_mode(&server->config, server->prompt,command,stderr_mode);
}



int main(int argc, char *argv[]) {

    // Check env vars.
    const char *properties_file = getenv("PROPERTIES_FILE");
    if (properties_file == NULL) {
        fprintf(stderr, "Environment variable PROPERTIES_FILE is not set.\n");
        exit(EXIT_FAILURE);
    }

    const char *start_prompt = getenv("PROMPT");
    if (start_prompt == NULL) {
        fprintf(stderr, "Environment variable PROMPT is not set.\n");
        exit(EXIT_FAILURE);
    }

    const char *log_file_path = getenv("LOG_FILE");
    if (log_file_path == NULL) {
        fprintf(stderr, "Environment variable LOG_FILE is not set.\n");
        exit(EXIT_FAILURE);
    }



    Server server;
    initialize_server(&server, properties_file, start_prompt); // init server struct

   // printf("TEST 10...\n");

    
    signal(SIGPIPE, SIG_IGN);

    if (argc == 1) {
        print_help();
    } else if (argc == 2 && strcmp(argv[1], "-s") == 0) { // default server mode
        handle_server_mode(&server, server.config.server_address, server.config.port);
    } else if (argc == 5 && strcmp(argv[1], "-s") == 0 && strcmp(argv[3], "-p") == 0) { // server mode with port
        strncpy(server.config.server_address, argv[2], MAX_SERVER_ADDRESS_LEN - 1);
        server.config.server_address[MAX_SERVER_ADDRESS_LEN - 1] = '\0';

        server.config.port = atoi(argv[4]);
        handle_server_mode(&server, server.config.server_address, server.config.port);
    } else if (argc == 6 && strcmp(argv[1], "-s") == 0 && strcmp(argv[3], "-p") == 0 && strcmp(argv[5], "-l") == 0) { // server mode with port and log file or default log file
        strncpy(server.config.server_address, argv[2], MAX_SERVER_ADDRESS_LEN - 1);
        server.config.server_address[MAX_SERVER_ADDRESS_LEN - 1] = '\0';

        server.config.port = atoi(argv[4]);
        const char *logfile = (argc == 6) ? log_file_path : argv[6];
        strncpy(server.config.log_file, logfile, MAX_LOG_FILE_PATH - 1);
        server.config.log_file[MAX_LOG_FILE_PATH - 1] = '\0';

        handle_server_mode(&server, server.config.server_address, server.config.port);
    } else if (argc == 5 && strcmp(argv[1], "-c") == 0 && strcmp(argv[3], "-p") == 0) { // client mode with server address and port
        strncpy(server.config.server_address, argv[2], MAX_SERVER_ADDRESS_LEN - 1);
        server.config.server_address[MAX_SERVER_ADDRESS_LEN - 1] = '\0';
        server.config.port = atoi(argv[4]);

        handle_client_mode(server.config.server_address, server.config.port, &server,NULL);
    }else if (argc == 2 && (strcmp(argv[1], "-c") == 0)) { // client mode with default mode 
        handle_client_mode(server.config.server_address, server.config.port, &server,NULL);
    } else if (argc == 4 && (strcmp(argv[1], "-c") == 0) && (strcmp(argv[2], "-C") == 0)) { // client mode with config file mode
        char* config_file = argv[3];
        if (!read_properties(config_file, &server.config)) {
            fprintf(stderr, "Failed to read properties file!\n");
            exit(EXIT_FAILURE);
        }else{
            handle_client_mode(server.config.server_address, server.config.port, &server, NULL);
        }
    } else if (argc == 4 && (strcmp(argv[1], "-s") == 0) && (strcmp(argv[2], "-C") == 0)) { // server mode with config file mode
        char* config_file = argv[3];
        if (!read_properties(config_file, &server.config)) {
            fprintf(stderr, "Failed to read properties file!\n");
            exit(EXIT_FAILURE);
        }else{
            handle_server_mode(&server, server.config.server_address, server.config.port);
        }
    }else if (argc == 4 && (strcmp(argv[1], "-c") == 0) && (strcmp(argv[2], "-c") == 0)) { // client mode with one speed command
        char* command = argv[3];
        handle_client_mode(server.config.server_address, server.config.port, &server,command);
    }else if (argc == 3 && (strcmp(argv[1], "-c") == 0) && (strcmp(argv[2], "-v") == 0)) { // client mode with stderr out
        server.stderr_mode = true;
        printf("Client mode with stderr out mode\n");
        handle_client_mode(server.config.server_address, server.config.port, &server,NULL);
    }else if (argc == 3 && (strcmp(argv[1], "-s") == 0) && (strcmp(argv[2], "-v") == 0)) { // server mode with stderr out
        server.stderr_mode = true;
        printf("Server mode with stderr out mode\n");
        handle_server_mode(&server, server.config.server_address, server.config.port);
    }else { // invalid arguments
        print_help();
    }

    cleanup_server(&server);
    return 0;
}