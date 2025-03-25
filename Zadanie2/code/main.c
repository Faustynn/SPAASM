#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <netinet/in.h>
#include "utils.h"
#include "global.h"

void read_properties(const char *filename, char *server_address, int *port, int *prompt_size, int *buffer_size, int *max_clients, int *max_socket_listen_conn) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening properties file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *key = strtok(line, " =\n");
        char *value = strtok(NULL, " =\n");

        if (key && value) {
            if (strcmp(key, "port") == 0) {
                *port = atoi(value);
                printf("Port: %d\n", *port);
            } else if (strcmp(key, "server_address") == 0) {
                strncpy(server_address, value, INET_ADDRSTRLEN - 1);
                server_address[INET_ADDRSTRLEN - 1] = '\0';
            } else if (strcmp(key, "prompt_size") == 0) {
                *prompt_size = atoi(value);
            } else if (strcmp(key, "buffer_size") == 0) {
                *buffer_size = atoi(value);
            } else if (strcmp(key, "max_clients") == 0) {
                *max_clients = atoi(value);
            } else if (strcmp(key, "max_socket_listen_conn") == 0) {
                *max_socket_listen_conn = atoi(value);
            }
        }
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    char server_address[INET_ADDRSTRLEN] = {0};
    int port = 0;
    int prompt_size = 0;
    int buffer_size = 0;
    int max_clients = 0;
    int max_socket_listen_conn = 0;

    const char *properties_file = getenv("PROPERTIES_FILE");
    if (properties_file == NULL) {
        fprintf(stderr, "Environment variable PROPERTIES_FILE is not set.\n");
        exit(EXIT_FAILURE);
    }
    const char *prompt = getenv("PROMPT");
    if (prompt == NULL) {
        fprintf(stderr, "Environment variable PROMPT is not set.\n");
        exit(EXIT_FAILURE);
    }
    const char *log_file_path = getenv("LOG_FILE");
    if (log_file_path == NULL) {
        fprintf(stderr, "Environment variable LOG_FILE is not set.\n");
        exit(EXIT_FAILURE);
    }



    read_properties(properties_file, server_address, &port, &prompt_size, &buffer_size, &max_clients, &max_socket_listen_conn);
    char prompt[prompt_size];
    generate_prompt(prompt, prompt_size);

    client_sockets = malloc(max_clients * sizeof(int));
    if (client_sockets == NULL) {
        perror("Failed to allocate memory for client_sockets");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < max_clients; i++) {
        client_sockets[i] = -1;
    }

    signal(SIGPIPE, SIG_IGN); 
    
    if (argc == 1) {
        print_help();
    } else if (argc == 2 && strcmp(argv[1], "-s") == 0) {
        printf("Starting server on %s:%d\n", server_address, port);
        server_mode(port, buffer_size, max_clients, max_socket_listen_conn, prompt, prompt_size, NULL);
    } else if (argc == 5 && strcmp(argv[1], "-s") == 0 && strcmp(argv[3], "-p") == 0) {
        strcpy(server_address, argv[2]);
        port = atoi(argv[4]);
        printf("Starting server on %s:%d\n", server_address, port);
        server_mode(port, buffer_size, max_clients, max_socket_listen_conn, prompt, prompt_size, NULL);
    } else if(argc == 7 && strcmp(argv[1], "-s") == 0 && strcmp(argv[3], "-p") == 0 && strcmp(argv[5], "-l") == 0) { // loging mode
        strcpy(server_address, argv[2]);
        port = atoi(argv[4]);
        char* logfile;
        if(argv[6] == NULL) {
            logfile = log_file_path; 
        }else{logfile = argv[6];}
        printf("Connecting to server at %s:%d with logs: %s\n", server_address, port, logfile);
        server_mode(port, buffer_size, max_clients, max_socket_listen_conn, prompt, prompt_size, logfile);

    } else if(argc == 6 && strcmp(argv[1], "-s") == 0 && strcmp(argv[3], "-p") == 0 && strcmp(argv[5], "-v") == 0) { // stderr info
        strcpy(server_address, argv[2]); 
        port = atoi(argv[4]);

        server_mode(port, buffer_size, max_clients, max_socket_listen_conn, prompt, prompt_size, NULL);
    } else if(argc == 7 && strcmp(argv[1], "-c") == 0 && strcmp(argv[3], "-p") == 0 && strcmp(argv[5], "-c") == 0) { // make command and end
        strcpy(server_address, argv[2]); 
        port = atoi(argv[4]);
        char* command = argv[6];
        printf("Make command to server at %s:%d\n", server_address, port);
      //  client_mode(server_address, port, buffer_size, prompt, prompt_size);




    } else if(argc == 7 && strcmp(argv[1], "-c") == 0 && strcmp(argv[3], "-p") == 0 && strcmp(argv[5], "-C") == 0) { // load config from file
        
        // TODO



    } else if (argc == 3 && strcmp(argv[1], "-c") == 0) {
        port = atoi(argv[2]);
        printf("Connecting to server at %s:%d\n", server_address, port);
        client_mode(server_address, port, buffer_size, prompt, prompt_size);
        
    }else if (argc == 5 && strcmp(argv[1], "-c") == 0 && strcmp(argv[3], "-p") == 0) {
        strcpy(server_address, argv[2]);
        port = atoi(argv[4]);
        printf("Connecting to server at %s:%d\n", server_address, port);
        client_mode(server_address, port, buffer_size, prompt, prompt_size);

        return 0;
    } else {
        print_help();
    }

    cleanup(max_clients);
    free(client_sockets);
    return 0;
}