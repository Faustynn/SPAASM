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

void client_mode(const char *server_address, int port, int buffer_size, char prompt[], int prompt_size) {
    generate_prompt(prompt, prompt_size);
    int sockfd;
    struct sockaddr_in server_addr;
    int running = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_address, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return;
    }

    signal(SIGPIPE, SIG_IGN);


    printf("Connected to server %s:%d\n", server_address, port);

    while (running) {
        generate_prompt(prompt, prompt_size);
        printf("%s", prompt);
        fflush(stdout);
        
        char *buffer = malloc(buffer_size);
        if (buffer == NULL) {
            perror("malloc");
            break;
        }

        memset(buffer, 0, buffer_size);
        if (fgets(buffer, buffer_size, stdin) == NULL) {
            if (feof(stdin)) {
                printf("End of input. Exiting...\n");
            } else {
                perror("fgets");
            }
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';

        if (strlen(buffer) == 0) {
            free(buffer);
            continue;
        }

        if (strcmp(buffer, "/quit") == 0) {
            if (write(sockfd, buffer, strlen(buffer)) < 0) {
                perror("write");
                printf("Failed to send quit command. Exiting anyway...\n");
            } else {
                size_t total_size = 0;
                size_t current_size = buffer_size;
                char *response = malloc(current_size);
                if (response == NULL) {
                    perror("malloc");
                    free(buffer);
                    break;
                }

                // Unset blocking mode for socket
                int flags = fcntl(sockfd, F_GETFL, 0);
                fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

                ssize_t bytes_received=0;
                fd_set read_set;
                struct timeval timeout;
                int ready;

                // whait for data unlock or timeout
                FD_ZERO(&read_set);
                FD_SET(sockfd, &read_set);
                timeout.tv_sec = 2; // time out settings
                timeout.tv_usec = 0;

                ready = select(sockfd + 1, &read_set, NULL, NULL, &timeout);
                
                if (ready > 0) {
                    while ((bytes_received = read(sockfd, response + total_size, current_size - total_size - 1)) > 0) {
                        total_size += bytes_received;
                        if (total_size + 1 >= current_size) {
                            current_size *= 2;
                            char *new_response = realloc(response, current_size);
                            if (new_response == NULL) {
                                perror("realloc");
                                free(response);
                                free(buffer);
                                break;
                            }
                            response = new_response;
                        }
                        
                        // we have datas?
                        FD_ZERO(&read_set);
                        FD_SET(sockfd, &read_set);
                        timeout.tv_sec = 0;
                        timeout.tv_usec = 100000;
                        ready = select(sockfd + 1, &read_set, NULL, NULL, &timeout);
                        if (ready <= 0) break;
                    }
                }

                // return blocking mode
                fcntl(sockfd, F_SETFL, flags);

                if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("read");
                } else {
                    response[total_size] = '\0';
                    printf("Server response: %s", response);
                    fflush(stdout);
                }

                free(response);
            }
            free(buffer);
            break;
        }

        if (write(sockfd, buffer, strlen(buffer)) < 0) {
            perror("write");
            printf("Failed to send command to server. Connection might be broken.\n");
            free(buffer);
            printf("Would you like to try to reconnect? (y/n): ");
            fflush(stdout);
            char answer[10];
            if (fgets(answer, sizeof(answer), stdin) != NULL && (answer[0] == 'y' || answer[0] == 'Y')) {
                close(sockfd);
                // Try to reconnect
                sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0 || 
                    connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                    perror("reconnect");
                    printf("Reconnection failed. Exiting...\n");
                    break;
                }
                printf("Reconnected to server.\n");
                continue;
            } else {
                break;
            }
        }

        size_t current_size = buffer_size;
        size_t total_size = 0;
        char *response = malloc(current_size);
        if (response == NULL) {
            perror("malloc");
            free(buffer);
            break;
        }

        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        ssize_t bytes_received =0;
        fd_set read_set;
        struct timeval timeout;
        int ready;

        FD_ZERO(&read_set);
        FD_SET(sockfd, &read_set);
        timeout.tv_sec = 2;  
        timeout.tv_usec = 0;

        ready = select(sockfd + 1, &read_set, NULL, NULL, &timeout);
        
        if (ready > 0) {
            while ((bytes_received = read(sockfd, response + total_size, current_size - total_size - 1)) > 0) {
                total_size += bytes_received;
                if (total_size + 1 >= current_size) {
                    current_size *= 2;
                    char *new_response = realloc(response, current_size);
                    if (new_response == NULL) {
                        perror("realloc");
                        free(response);
                        free(buffer);
                        break;
                    }
                    response = new_response;
                }
                
                FD_ZERO(&read_set);
                FD_SET(sockfd, &read_set);
                timeout.tv_sec = 0;
                timeout.tv_usec = 100000;
                ready = select(sockfd + 1, &read_set, NULL, NULL, &timeout);
                if (ready <= 0) break;
            }
        }

        fcntl(sockfd, F_SETFL, flags);

        if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("read");
            printf("Error reading from server. Will try again with next command.\n");
        } else if (total_size == 0 && ready <= 0) {
            printf("No response from server within timeout\n");
        } else if (total_size == 0) {
            printf("Unknown command.\n");
            fflush(stdout);
        } else {
            response[total_size] = '\0';
            printf("Server response: \n%s\n", response);
            fflush(stdout);
        }

        free(response);
        free(buffer);
    }

    close(sockfd);
    printf("Client disconnected\n");
}