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

volatile sig_atomic_t is_running = 1; // global running flag

// method to stop server 
void stop_server_signal(int signal) {
    (void)signal;
    is_running = 0;
}

// method to delete \n \r and space
void trim_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r' || str[len - 1] == ' ')) {
        str[len - 1] = '\0';
        len--;
    }
}

void execute_single_command(char *cmd, int client_fd, const char* logfile, const bool stderr_mode);
void execute_parsed_command(char *input, int client_fd, const char* logfile, const bool stderr_mode);


// method to execute commands for special chars (; # < > >>)
void execute_parsed_command(char *input, int client_fd, const char* logfile, const bool stderr_mode) {
    char *commands[64]; // Array to hold multi commands
    int cmd_count = 0;
    
    // handle ;
    char *cmd_ptr = strtok(input, ";");
    while (cmd_ptr != NULL && cmd_count < 64) {
        commands[cmd_count++] = cmd_ptr;
        cmd_ptr = strtok(NULL, ";");
    }
    
    // Execute each command
    for (int i = 0; i < cmd_count; i++) {
        char *cmd = commands[i];
        
        // Skip spaces
        while (*cmd == ' ' || *cmd == '\t') cmd++;
        if (*cmd == '\0') continue; // skip empty commands
        
        execute_single_command(cmd, client_fd, logfile, stderr_mode); // process each command
    }
}

// method to execute single command (without special chars)
void execute_single_command(char *cmd, int client_fd, const char* logfile, const bool stderr_mode) {
    char *input_file = NULL;
    char *output_file = NULL;
    int append_output = 0;
    
    // Check for #
    char *comment_pos = strchr(cmd, '#');
    if (comment_pos != NULL) {
        *comment_pos = '\0'; // Terminate string at comment
    }
    
    // Check for <
    char *input_redir = strstr(cmd, "<");
    if (input_redir != NULL) {
        *input_redir = '\0'; // Terminate command string
        input_file = input_redir + 1;
        
        // Skip spaces
        while (*input_file == ' ' || *input_file == '\t') input_file++;
        
        // Find end of filename
        char *end = input_file;
        while (*end != '\0' && *end != ' ' && *end != '\t' && 
               *end != '<' && *end != '>' && *end != '#') end++;
        *end = '\0';
    }
    
    // Check for > or >>
    char *output_redir = strstr(cmd, ">>");
    if (output_redir != NULL) {
        *output_redir = '\0'; // Terminate command string
        output_file = output_redir + 2;
        append_output = 1;
    } else {
        output_redir = strstr(cmd, ">");
        if (output_redir != NULL) {
            *output_redir = '\0'; // Terminate command string
            output_file = output_redir + 1;
        }
    }
    
    // Skip spaces
    if (output_file) {
        while (*output_file == ' ' || *output_file == '\t') output_file++;
        
        // Find end of filename
        char *end = output_file;
        while (*end != '\0' && *end != ' ' && *end != '\t' && 
               *end != '<' && *end != '>' && *end != '#') end++;
        *end = '\0';
    }
    
    // Handle scripts  ./script.sh
    if (strncmp(cmd, "./", 2) == 0) {
        if (access(cmd, F_OK) != 0) {
            const char *error_message = "Script file doesn't exist\n";
            if (stderr_mode) {
                fprintf(stderr, "Script file doesn't exist\n");
            }
            write(client_fd, error_message, strlen(error_message));
            return;
        }
        if (access(cmd, X_OK) != 0) {
            const char *error_message = "Script file not executable\n";
            if (stderr_mode) {
                fprintf(stderr, "Script file not executable\n");
            }
            write(client_fd, error_message, strlen(error_message));
            return;
        }
    }
    
    char *args[64];
    int arg_count = 0;
    
    // Skip spaces
    while (*cmd == ' ' || *cmd == '\t') cmd++;
    
    // Build argument arr.
    char *token = strtok(cmd, " \t");
    while (token != NULL && arg_count < 63) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t");
    }
    args[arg_count] = NULL; // NULL-terminate the array
    
    if (arg_count == 0) return; // is empty command?
    
    // Create pipes for capture output
    int output_pipe[2];
    if (pipe(output_pipe) == -1) {
        const char *error_message = "Failed to create pipe\n";
        if (stderr_mode) {
            fprintf(stderr, "Failed to create pipe\n");
            perror("pipe");
        }
        write(client_fd, error_message, strlen(error_message));
        return;
    }
    
    // Fork and execute
    pid_t pid = fork();
    
    if (pid == -1) {
        const char *error_message = "Failed to fork process\n";
        if (stderr_mode) {
            fprintf(stderr, "Failed to fork process\n");
            perror("fork");
        }
        write(client_fd, error_message, strlen(error_message));
        close(output_pipe[0]);
        close(output_pipe[1]);
        return;
    }
    
    if (pid == 0) {
        // Child process
        
        // Redirect stdin if input file specified
        if (input_file != NULL) {
            int fd = open(input_file, O_RDONLY);
            if (fd == -1) {
                const char *error_message = "Failed to open input file\n";
                if (stderr_mode) {
                    fprintf(stderr, "Failed to open input file: %s\n", input_file);
                    perror("open");
                }
                write(client_fd, error_message, strlen(error_message));
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        
        // Redirect stdout if output file specified
        if (output_file != NULL) {
            int flags = O_WRONLY | O_CREAT;
            if (append_output) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }
            
            int fd = open(output_file, flags, 0644);
            if (fd == -1) {
                const char *error_message = "Failed to open output file\n";
                if (stderr_mode) {
                    fprintf(stderr, "Failed to open output file: %s\n", output_file);
                    perror("open");
                }
                write(client_fd, error_message, strlen(error_message));
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else {
            // If no file redirection, redirect to pipe
            close(output_pipe[0]);
            dup2(output_pipe[1], STDOUT_FILENO);
            dup2(output_pipe[1], STDERR_FILENO);
        }
        
        close(output_pipe[1]);
        
        // Execute command
        execvp(args[0], args);
        
        fprintf(stderr, "Failed to execute command: %s\n", args[0]);
        exit(EXIT_FAILURE);
    } else { // Parent process
        close(output_pipe[1]); // Close write end of pipe
        
        // Read command output
        size_t buffer_size = 1024;
        size_t total_size = 0;
        char *output = malloc(buffer_size);
        
        if (output == NULL) {
            const char *error_message = "Memory allocation failed\n";
            if (stderr_mode) {
                fprintf(stderr, "Memory allocation failed\n");
            }
            write(client_fd, error_message, strlen(error_message));
            close(output_pipe[0]);
            return;
        }
        
        char temp_buffer[1024]; // buffer for reading output
        ssize_t bytes_read;
        
        while ((bytes_read = read(output_pipe[0], temp_buffer, sizeof(temp_buffer) - 1)) > 0) { // start reading
            if (total_size + bytes_read + 1 > buffer_size) { // reallocate buffer if needed
                buffer_size *= 2;
                char *new_output = realloc(output, buffer_size);
                if (new_output == NULL) {
                    const char *error_message = "Memory reallocation failed\n";
                    if (stderr_mode) {
                        fprintf(stderr, "Memory reallocation failed\n");
                    }
                    write(client_fd, error_message, strlen(error_message));
                    free(output);
                    close(output_pipe[0]);
                    return;
                }
                output = new_output;
            }
            
            temp_buffer[bytes_read] = '\0'; // add null-terminate
            memcpy(output + total_size, temp_buffer, bytes_read); // copy to output
            total_size += bytes_read; // update total size
        }
        
        close(output_pipe[0]); // close read end of pipe
        


        // Wait for child to complete
        int status;
        waitpid(pid, &status, 0);
        
        if (total_size == 0) { // no output
            const char *message = "Command executed but returned no output\n";
            write(client_fd, message, strlen(message));
        } else {
            output[total_size] = '\0';
            write(client_fd, output, total_size);
            printf("Command output: %s\n", output); // print output
        }
        
        // Log command execution if logfile specified
        if (logfile != NULL) {
            FILE *log = fopen(logfile, "a");
            if (log == NULL) {
                if (stderr_mode) {
                    fprintf(stderr, "Failed to open log file\n");
                    perror("fopen");
                }
                free(output);
                return;
            }
            
            time_t now = time(NULL);
            struct tm *local_time = localtime(&now);
            char timestamp[20];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time);
            
            // Get process ID
            pid_t process_id = getpid();
            int end_code = WEXITSTATUS(status);
            
            // Write log
            fprintf(log, "[%s] PID:%d CODE:%d %s\n", timestamp, process_id, end_code, output);
            fflush(log);
            fclose(log);
        }
        
        free(output);
    }
}

// main method to process commands
void process_command(char *input, int client_fd, const char* logfile, const bool stderr_mode) {
    printf("Processing command: '%s'\n", input);
    trim_newline(input);

    // Check for /help /halt and /quit commands
    if (strcmp(input, "/help") == 0) {
        printf("Sending help message to client\n");
        const char *help_message = print_help();
        write(client_fd, help_message, strlen(help_message));
        return;
    } else if (strcmp(input, "/halt") == 0) {
        stop_server_signal(0);
        const char *halt_message = "Server is stopping...\n";
        ssize_t bytes_written = write(client_fd, halt_message, strlen(halt_message));
        if (bytes_written < 0) {
            perror("write");
        }
        printf("Server is stopping by client %d\n", client_fd);
        is_running = 0;
        exit(EXIT_SUCCESS);
        return;
    } else if (strcmp(input, "/quit") == 0) {
        const char *quit_message = "Disconnecting from the server...\n";
        write(client_fd, quit_message, strlen(quit_message));
        return;
    } else if (input[0] == '/' && strcmp(input, "./") != 0) { // if not script
        const char *error_message = "Unknown command. Type /help for available commands.\n";
        write(client_fd, error_message, strlen(error_message));
        return;
    }

    // Parse command for special chars
    execute_parsed_command(input, client_fd, logfile, stderr_mode);
    
    // clear
    memset(input, 0, strlen(input));
}

void server_mode(ServerConfig *config, char *prompt, const char *logfile,const bool stderr_mode){
    int port = config->port;
    int max_clients = config->max_clients;
    int max_socket_listen_conn = config->max_socket_listen_conn;
    int buffer_size = config->buffer_size;
   // char *shell_name = config->prompt_prefix;

   // print start prompt 
    generate_prompt(prompt, config);

    fd_set read_fds, master_fds;
    int max_fd;
    struct sockaddr_in server_addr;
    char buffer[buffer_size];
    ssize_t bytes_received = 0;
    int client_count = 0;

    // signal handling
    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);


    // if we have port
    if (port > 0) {
        server_socket = socket(AF_INET, SOCK_STREAM, 0); // create socket for server
        if (server_socket < 0) {
            if(stderr_mode){
                fprintf(stderr, "Failed to create socket\n");
                perror("socket");
            }
            return;
        }

        int reuse = 1; // allow address reuse
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) { // set socket options like reusefull
            if(stderr_mode){
                fprintf(stderr, "Failed to set socket options\n");
                perror("setsockopt");
            }
            close(server_socket);
            return;
        }

        memset(&server_addr, 0, sizeof(server_addr)); // clear address structure
        server_addr.sin_family = AF_INET; // set IPv4
        server_addr.sin_addr.s_addr = INADDR_ANY; // bind to any address
        server_addr.sin_port = htons(port); // set port number

        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { // bind socket to address
            if(stderr_mode){
                fprintf(stderr, "Failed to bind socket\n");
                perror("bind");
            }
            close(server_socket);
            return;
        }

        if (listen(server_socket, max_socket_listen_conn) < 0) { // start listening for connections
            if(stderr_mode){
                fprintf(stderr, "Failed to listen on socket\n");
                perror("listen");
            }
         //   perror("listen");
            close(server_socket);
            return;
        }

        generate_prompt(prompt, config);
        printf("Server listening on port %d\n", port);

        FD_SET(server_socket, &master_fds); // add server socket to master set
        max_fd = server_socket; // set max fd to server socket
    }

    while (is_running) { // main loop
        read_fds = master_fds; // copy master set to read set

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) { // check for activity on sockets
            if (errno == EINTR) {
                continue;
            }
            if(stderr_mode){
                fprintf(stderr, "Failed to select on sockets\n");
                perror("select");
            }
            break;
        }

        for (int i = 0; i <= max_fd; i++) { // check each of sockets
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_socket) { // handle new connection
                    if (client_count >= max_clients) { // handle max clients conn.
                        generate_prompt(prompt, config);
                        printf("Maximum number of clients reached. Rejecting new connection.\n");
                        int new_fd = accept(server_socket, NULL, NULL);
                        if (new_fd >= 0) {
                            close(new_fd);
                        }
                    } else {
                        struct sockaddr_in client_addr; // client address
                        socklen_t client_len = sizeof(client_addr); // client address length
                        int new_fd = accept(server_socket, (struct sockaddr *)&client_addr, &client_len); // set new fd accept from socket
                        if (new_fd < 0) {
                            if(stderr_mode){
                                fprintf(stderr, "Failed to accept new connection\n");
                                perror("accept");
                            }
                        } else {
                            generate_prompt(prompt, config);
                            printf("New connection from %s on socket %d\n", inet_ntoa(client_addr.sin_addr), new_fd);
                            FD_SET(new_fd, &master_fds); // add new fd to master set
                            if (new_fd > max_fd) {
                                max_fd = new_fd;
                            }
                            client_count++; // incrise client count
                        }
                    }
                } else { // handle existing connection
                    bytes_received = read(i, buffer, buffer_size - 1); // read data from socket
                    if (bytes_received <= 0) { // if empty or error close conn.
                        if (bytes_received < 0) {
                            if(stderr_mode){
                                perror("read");
                                printf("Error reading from client on socket %d\n", i);
                            }
                        }
                        generate_prompt(prompt, config);
                        printf("Client on socket %d disconnected\n", i);
                        close(i);
                        FD_CLR(i, &master_fds);
                        client_count--;
                    } else {
                        buffer[bytes_received] = '\0'; // add null-terminate
                        generate_prompt(prompt, config);
                        printf("Received command: %s\n", buffer); // print command
                        process_command(buffer, i,logfile,stderr_mode); // process this command
                    }
                }
            }
        }
    }

    // Cleanup
    for (int i = 0; i <= max_fd; i++) {
        if (FD_ISSET(i, &master_fds)) {
            close(i);
        }
    }

    printf("%s", prompt); // print prompt
}