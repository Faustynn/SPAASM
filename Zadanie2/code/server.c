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

void trim_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r' || str[len - 1] == ' ')) {
        str[len - 1] = '\0';
        len--;
    }
}

void process_command(char *input, int client_fd,char* logfile) {
    printf("Processing command: '%s'\n", input);

    trim_newline(input);

    if (strcmp(input, "/help") == 0) {
        printf("Sending help message to client\n");
        const char *help_message = print_help();
        write(client_fd, help_message, strlen(help_message));
    } else if (strcmp(input, "/halt") == 0) {
        stop_server_signal(0);
        const char *halt_message = "Server is stopping...\n";
        write(client_fd, halt_message, strlen(halt_message));
    } else if (strcmp(input, "/quit") == 0) {
        const char *quit_message = "Disconnecting from the server...\n";
        write(client_fd, quit_message, strlen(quit_message));
    } else if (input[0] == '/') {
        const char *error_message = "Unknown command. Type /help for available commands.\n";
        write(client_fd, error_message, strlen(error_message));
    } else {
        char command[1024];
        snprintf(command, sizeof(command), "sh -c '%s 2>&1'", input);

        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            const char *error_message = "Failed to execute command\n";
            write(client_fd, error_message, strlen(error_message));
        } else {
            size_t buffer_size = 1024;
            size_t total_size = 0;
            char *output = malloc(buffer_size);
            if (output == NULL) {
                const char *error_message = "Memory allocation failed\n";
                write(client_fd, error_message, strlen(error_message));
                pclose(fp);
                return;
            }

            char temp_buffer[1024];
            while (fgets(temp_buffer, sizeof(temp_buffer), fp) != NULL) {
                size_t len = strlen(temp_buffer);
                if (total_size + len + 1 > buffer_size) {
                    buffer_size *= 2;
                    char *new_output = realloc(output, buffer_size);
                    if (new_output == NULL) {
                        const char *error_message = "Memory reallocation failed\n";
                        write(client_fd, error_message, strlen(error_message));
                        free(output);
                        pclose(fp);
                        return;
                    }
                    output = new_output;
                }
                memcpy(output + total_size, temp_buffer, len);
                total_size += len;
            }

            int status = pclose(fp);
            if (status == -1 || total_size == 0) {
                const char *error_message = "Command executed but returned no output\n";
                write(client_fd, error_message, strlen(error_message));
            } else {
                output[total_size] = '\0';
                write(client_fd, output, total_size);
                printf("Command output: %s\n", output);
            }

             if(logfile!=NULL){
                FILE *log = fopen(logfile, "a");
                if (log == NULL) {
                    perror("fopen");
                    free(output);
                }

                time_t now = time(NULL);
                struct tm *local_time = localtime(&now);
                char timestamp[20];
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time);

                // Get proces ID
                pid_t pid = getpid();
                int end_code = WEXITSTATUS(status);

                // Write log
                fprintf(log, "[%s] PID:%d CODE:%d %s\n", timestamp, pid, end_code, output);
                fflush(log);
                fclose(log);
            }
            
            free(output);
        }
    }
    // clear
    memset(input, 0, strlen(input));
}

void server_mode(int port, int buffer_size, int max_clients, int max_socket_listen_conn, char prompt[], int prompt_size, char* logfile) {
    generate_prompt(prompt, prompt_size);
    fd_set read_fds, master_fds;
    int max_fd;
    struct sockaddr_in server_addr;
    char buffer[buffer_size];
    ssize_t bytes_received = 0;
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
                        process_command(buffer, i,logfile);
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