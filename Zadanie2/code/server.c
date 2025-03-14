#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <limits.h>

#include "utils.h"
#include "global.h"

volatile sig_atomic_t is_running = 1;

void stop_server_signal(int signal) {
    (void)signal;
    is_running = 0;
}

void process_command(char *input, int client_fd) {
    if (strcmp(input, "/help") == 0) {
        const char *help_message = print_help();
        ssize_t bytes_written = write(client_fd, help_message, strlen(help_message));
        if (bytes_written < (ssize_t)strlen(help_message)) {
            perror("write incomplete");
        }
    } else if (strcmp(input, "/halt") == 0) {
        stop_server_signal(0);
        const char *halt_message = "Server is stopping...\n";
        write(client_fd, halt_message, strlen(halt_message));
    } else if (strcmp(input, "/quit") == 0) {
        const char *quit_message = "Disconnecting from the server...\n";
        write(client_fd, quit_message, strlen(quit_message));
    } else {
        const char *unknown_command_message = "Unknown command\n";
        ssize_t bytes_written = write(client_fd, unknown_command_message, strlen(unknown_command_message));
        if (bytes_written < (ssize_t)strlen(unknown_command_message)) {
            perror("write incomplete");
        }
    }
}

void server_mode(int port, int buffer_size, int max_clients, int max_socket_listen_conn, char prompt[], int prompt_size) {
    generate_prompt(prompt, prompt_size);
    fd_set read_fds, master_fds;
    int max_fd;
    struct sockaddr_in server_addr;
    char buffer[buffer_size];
    ssize_t bytes_received;
    int client_count = 0;

    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);

    if (port > 0) {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            perror("socket");
            return;
        }

        int reuse = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            perror("setsockopt");
            close(server_socket);
            return;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("bind");
            close(server_socket);
            return;
        }

        if (listen(server_socket, max_socket_listen_conn) < 0) {
            perror("listen");
            close(server_socket);
            return;
        }

        generate_prompt(prompt, prompt_size);
        printf("Server listening on port %d\n", port);

        FD_SET(server_socket, &master_fds);
        max_fd = server_socket;
    }

    while (is_running) {
        read_fds = master_fds;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_socket) {
                    if (client_count >= max_clients) {
                        generate_prompt(prompt, prompt_size);
                        printf("Maximum number of clients reached. Rejecting new connection.\n");
                        int new_fd = accept(server_socket, NULL, NULL);
                        if (new_fd >= 0) {
                            close(new_fd);
                        }
                    } else {
                        struct sockaddr_in client_addr;
                        socklen_t client_len = sizeof(client_addr);
                        int new_fd = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
                        if (new_fd < 0) {
                            perror("accept");
                        } else {
                            generate_prompt(prompt, prompt_size);
                            printf("New connection from %s on socket %d\n", inet_ntoa(client_addr.sin_addr), new_fd);
                            FD_SET(new_fd, &master_fds);
                            if (new_fd > max_fd) {
                                max_fd = new_fd;
                            }
                            client_count++;
                        }
                    }
                } else {
                    bytes_received = read(i, buffer, buffer_size - 1);
                    if (bytes_received <= 0) {
                        if (bytes_received < 0) {
                            perror("read");
                        }
                        generate_prompt(prompt, prompt_size);
                        printf("Client on socket %d disconnected\n", i);
                        close(i);
                        FD_CLR(i, &master_fds);
                        client_count--;
                    } else {
                        buffer[bytes_received] = '\0';
                        generate_prompt(prompt, prompt_size);
                        printf("Received command: %s\n", buffer);
                        process_command(buffer, i);
                    }
                }
            }
        }
    }

    for (int i = 0; i <= max_fd; i++) {
        if (FD_ISSET(i, &master_fds)) {
            close(i);
        }
    }
    printf("%s", prompt);
}