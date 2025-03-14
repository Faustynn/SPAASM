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
    char buffer[buffer_size];
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

    printf("Connected to server %s:%d\n", server_address, port);
    printf("Type 'выход' to quit the client\n");

    signal(SIGPIPE, SIG_IGN);

    while (running) {
        generate_prompt(prompt, prompt_size);
        printf("%s", prompt);
        fflush(stdout);
        
        if (fgets(buffer, buffer_size, stdin) == NULL) {
            if (feof(stdin)) {
                printf("End of input. Exiting...\n");
            } else {
                perror("fgets");
                printf("Error reading input. Trying again...\n");
                clearerr(stdin);
                continue;
            }
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "выход") == 0) {
            printf("Exiting client...\n");
            break;
        }

        if (strcmp(buffer, "/quit") == 0) {
            if (write(sockfd, buffer, strlen(buffer)) < 0) {
                perror("write");
                printf("Failed to send quit command. Exiting anyway...\n");
            } else {
                ssize_t bytes_received = read(sockfd, buffer, buffer_size - 1);
                if (bytes_received > 0) {
                    buffer[bytes_received] = '\0';
                    printf("Server response: %s\n", buffer);
                }
            }
            break;
        }

        if (strlen(buffer) == 0) {
            continue;
        }

        if (write(sockfd, buffer, strlen(buffer)) < 0) {
            perror("write");
            printf("Failed to send command to server. Connection might be broken.\n");
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

        ssize_t bytes_received = read(sockfd, buffer, buffer_size - 1);
        if (bytes_received < 0) {
            perror("read");
            printf("Error reading from server. Will try again with next command.\n");
            continue; 
        } else if (bytes_received == 0) {
            printf("Server closed the connection\n");
            printf("Would you like to try to reconnect? (y/n): ");
            fflush(stdout);
            char answer[10];
            if (fgets(answer, sizeof(answer), stdin) != NULL && (answer[0] == 'y' || answer[0] == 'Y')) {
                close(sockfd);

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

        buffer[bytes_received] = '\0';
        printf("Server response: %s\n", buffer);
        
    }

    close(sockfd);
    printf("Client disconnected\n");
}