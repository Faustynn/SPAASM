#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>

#include "utils.h"
#include "global.h"

void client_mode(ServerConfig *config, char *prompt, char* command, const bool stderr_mode) {
    // init vars.
    int port = config->port;
    const char *server_address = config->server_address;
    int buffer_size = config->buffer_size;

    // print start prompt
    generate_prompt(prompt, config);

    int sockfd;
    struct sockaddr_in server_addr;
    int running = 1;

    // create socket for new user
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        if(stderr_mode){
            fprintf(stderr, "Failed to create socket\n");
            perror("socket");
        }
        return;
    }

    // set socket options
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_port = htons(port); // port
    if (inet_pton(AF_INET, server_address, &server_addr.sin_addr) <= 0) { // convert address from text to binary
        if(stderr_mode){
            fprintf(stderr, "Invalid address: %s\n", server_address);
            perror("inet_pton");
        }
        close(sockfd);
        return;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { // try to connect with server
        if(stderr_mode){
            fprintf(stderr, "Failed to connect to server %s:%d\n", server_address, port);
            perror("connect");
        }
     //   perror("connect");
        close(sockfd);
        return;
    }

    signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE signal

    // check if command is NULL
    if(command == NULL){
        printf("Connected to server %s:%d\n", server_address, port);
    }else {
        printf("Temp. connected to server %s:%d\n", server_address, port);

        // send one command and wait for response
        if (write(sockfd, command, strlen(command)) < 0) {
            if(stderr_mode){
            perror("write");
            printf("Failed to send command to server. Connection might be broken.\n");
            }
            close(sockfd);
            return;
        }

        // read response from server
        char response[buffer_size];
        memset(response, 0, buffer_size);

        // check response size
        ssize_t bytes_received = read(sockfd, response, buffer_size - 1);
        if (bytes_received < 0) {
            if(stderr_mode){
                perror("read");
                printf("Failed to read response from server.\n");
            }
        } else if (bytes_received == 0) {
            printf("Server closed the connection unexpectedly.\n");
        } else {
            printf("Server response: \n%s\n", response);
        }

        // exit when take response
        close(sockfd);
        printf("Client temp. connection disconnected!\n");
        exit(EXIT_SUCCESS);
    }

    while (running) {
        generate_prompt(prompt, config);
        printf("%s", prompt); // print prompt
        fflush(stdout); // flush stdout

        // alocate buffer for user input
        char *buffer = malloc(buffer_size);
        if (buffer == NULL) {
            if(stderr_mode){
                perror("malloc");
                printf("Failed to allocate memory for input buffer\n");
            }
            break;
        }

        memset(buffer, 0, buffer_size); // clear buffer
        if (fgets(buffer, buffer_size, stdin) == NULL) { // read user input
            if (feof(stdin)) {
                printf("End of input. Exiting...\n");
            } else {
                if(stderr_mode){
                    perror("fgets");
                    printf("Failed to read input from stdin\n");
                }
            }
            free(buffer);
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0'; // remove newline character

        if (strlen(buffer) == 0) {
            free(buffer);
            continue;
        }

        if (strcmp(buffer, "/quit") == 0) { // handle /quit command
            if (write(sockfd, buffer, strlen(buffer)) < 0) {
                if(stderr_mode){
                    perror("write");
                    printf("Failed to send quit command. Exiting anyway...\n");
                }
            }
            free(buffer); // free buffer
            buffer = NULL;
            close(sockfd); // Close socket
            printf("Client disconnected\n");
            exit(EXIT_SUCCESS);
        }
        

        if (write(sockfd, buffer, strlen(buffer)) < 0) { // send command to server if another command
            if(stderr_mode){
                perror("write");
                printf("Failed to send command to server. Connection might be broken.\n");
            }
            free(buffer);
            break;
        }

        // read response from server logic
        size_t current_size = buffer_size;
        size_t total_size = 0;
        char *response = malloc(current_size);
        if (response == NULL) {
            if(stderr_mode){
                perror("malloc");
                printf("Failed to allocate memory for response buffer\n");
            }
            free(buffer);
            break;
        }

        int flags = fcntl(sockfd, F_GETFL, 0); // get current flags
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK); // set non-blocking mode

        ssize_t bytes_received = 0;
        fd_set read_set;
        struct timeval timeout;
        int ready;

        // set timeout for select
        FD_ZERO(&read_set);
        FD_SET(sockfd, &read_set);
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        // wait for response from server
        ready = select(sockfd + 1, &read_set, NULL, NULL, &timeout);

        if (ready > 0) { // check if socket is ready for reading
            while ((bytes_received = read(sockfd, response + total_size, current_size - total_size - 1)) > 0) { // read response and put it to response
                total_size += bytes_received;
                if (total_size + 1 >= current_size) {
                    current_size *= 2;
                    char *new_response = realloc(response, current_size);
                    if (new_response == NULL) {
                        if(stderr_mode){
                            perror("realloc");
                            printf("Failed to reallocate memory for response buffer\n");
                        }
                        free(response);
                        free(buffer);
                        break;
                    }
                    response = new_response;
                }

                // check if there is more data to read (BIG responses)
                FD_ZERO(&read_set);
                FD_SET(sockfd, &read_set);
                timeout.tv_sec = 0;
                timeout.tv_usec = 100000;
                ready = select(sockfd + 1, &read_set, NULL, NULL, &timeout);
                if (ready <= 0) break;
            }
        }

        fcntl(sockfd, F_SETFL, flags); // reset to blocking mode

        // handle read errors
        if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) { 
            if(stderr_mode){
                perror("read");
                printf("Error reading from server. Will try again with next command.\n");
            }
        } else if (total_size == 0 && ready <= 0) {
            printf("No response from server within timeout\n");
        } else if (total_size == 0) {
            printf("Unknown command.\n");
            fflush(stdout);
        } else { // print response
            response[total_size] = '\0';
            printf("Server response: \n%s\n", response);
            fflush(stdout);
            // TODO: Test to print full response if it is too long and have more than 1 part
        }

        // free mem
        free(response);
        free(buffer);
    }

    // cleanup and exit
    close(sockfd);
    printf("Client disconnected\n");
    exit(EXIT_SUCCESS);
}