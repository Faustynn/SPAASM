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
#include <ctype.h>

#include "utils.h"
#include "global.h"

int read_properties(const char *filename, ServerConfig *config) {
    // try 0pen file
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening properties file");
        return 0;
    }

    char line[512];
    int line_number = 0;

    // read file line by line until EOF
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        
        // Trim whitespaces
        char *start = line;
        while (*start && isspace((unsigned char)*start)) start++;
        
        // Skip empty lines and COMMENTS
        if (*start == '\0' || *start == '#') continue;
        
        // remove whitespace and newline in line
        char *end = start + strlen(start) - 1;
        while (end > start && isspace((unsigned char)*end)) end--;
        *(end + 1) = '\0';

        char key[128] = {0};
        char value[256] = {0};

        // Parse key and value, handling whitespace around =
        char *equal_sign = strchr(start, '=');
        if (equal_sign == NULL) {
            fprintf(stderr, "Warning: Invalid line format at line %d: %s\n", line_number, line);
            continue;
        }

        // Extract key //left
        size_t key_len = equal_sign - start;
        strncpy(key, start, key_len);
        key[key_len] = '\0';
        while (key_len > 0 && isspace((unsigned char)key[key_len - 1])) {
            key[key_len - 1] = '\0';
            key_len--;
        }

        // Extract value //right
        char *value_start = equal_sign + 1;
        while (*value_start && isspace((unsigned char)*value_start)) value_start++;
        strncpy(value, value_start, sizeof(value) - 1);


        // Process key-value pairs
        if (strcmp(key, "port") == 0) {
            config->port = atoi(value);
        } else if (strcmp(key, "server_address") == 0) {
            strncpy(config->server_address, value, sizeof(config->server_address) - 1);
            config->server_address[sizeof(config->server_address) - 1] = '\0';
        } else if (strcmp(key, "inet_addr_len") == 0) {
            config->inet_addr_len = atoi(value);
        } else if (strcmp(key, "max_clients") == 0) {
            config->max_clients = atoi(value);
        } else if (strcmp(key, "max_socket_listen_conn") == 0) {
            config->max_socket_listen_conn = atoi(value);
        } else if (strcmp(key, "client_timeout") == 0) {
            config->client_timeout = atoi(value);
        } else if (strcmp(key, "max_connection_time") == 0) {
            config->max_connection_time = atoi(value);
        } else if (strcmp(key, "prompt_size") == 0) {
            config->prompt_size = atoi(value);
        } else if (strcmp(key, "buffer_size") == 0) {
            config->buffer_size = atoi(value);
        } else if (strcmp(key, "prompt_prefix") == 0) {
            strncpy(config->prompt_prefix, value, sizeof(config->prompt_prefix) - 1);
            config->prompt_prefix[sizeof(config->prompt_prefix) - 1] = '\0';
        } else if (strcmp(key, "show_system_info") == 0) {
            config->show_system_info = atoi(value);
        } else if (strcmp(key, "log_file") == 0) {
            strncpy(config->log_file, value, sizeof(config->log_file) - 1);
            config->log_file[sizeof(config->log_file) - 1] = '\0';
        } else if (strcmp(key, "max_command_length") == 0) {
            config->max_command_length = atoi(value);
        } else if (strcmp(key, "allow_remote_exec") == 0) {
            config->allow_remote_exec = atoi(value);
        } else if (strcmp(key, "banned_commands") == 0) {
            // process every banned command by ,
            char *token = strtok(value, ",");
            int count = 0;
            while (token != NULL && count < MAX_BANNED_COMMANDS) {
                while (*token == ' ') token++;
                char *end_token = token + strlen(token) - 1;
                while (end_token > token && *end_token == ' ') *end_token-- = '\0';
                
                //  put command to config
                strncpy(config->banned_commands[count], token, MAX_COMMAND_LENGTH - 1);
                config->banned_commands[count][MAX_COMMAND_LENGTH - 1] = '\0';
                count++;

                token = strtok(NULL, ",");
            }
            config->banned_commands_count = count;
        } else if (strcmp(key, "thread_pool_size") == 0) {
            config->thread_pool_size = atoi(value);
        } else if (strcmp(key, "max_concurrent_commands") == 0) {
            config->max_concurrent_commands = atoi(value);
        } else {
            fprintf(stderr, "Warning: Unknown configuration key at line %d: %s\n", line_number, key);
        }
    }

    // Close file
    fclose(file);
    printf("Properties file read successfully.\n");
    return 1;
}

// function for gen. start prompt
void generate_prompt(char *prompt, const ServerConfig *config) {
    // Take info about curent user
    struct passwd *pw;
    char hostname[HOST_NAME_MAX + 1];
    time_t current_time;         
    struct tm *time_info;            
    char time_str[10];              

    // Get username
    pw = getpwuid(getuid());
    // Get hostname
    gethostname(hostname, sizeof(hostname));
    // Get current time
    time(&current_time);
    // Convert current time to local
    time_info = localtime(&current_time);
    // Format time as HH:MM
    strftime(time_str, sizeof(time_str), "%H:%M", time_info);

    // Use prompt prefix from conf. file or fallback to "Default-Shell" if not set
    const char *prefix = config->prompt_prefix[0] ? config->prompt_prefix : "Default-Shell";

    // put system info into prompt if true in config
    if (config->show_system_info) {
        // Format prompt with new user info
        snprintf(prompt, config->prompt_size, "[%s] %s%s@%s# ", 
                 time_str, prefix, pw->pw_name, hostname);
    } else {
        // Format prompt without user info
        snprintf(prompt, config->prompt_size, "%s# ", prefix);
    }
}

// print_help usfull function
char* print_help(void) {
    static char help_message[2048];
    snprintf(help_message, sizeof(help_message),
        "---------------------------------------------------------------------------\n"
        "Usage:\n"
        "  shell [-s] [-c server_address] [-p port] [-u unix_socket_path] [-h]\n\n"
        "---------------------------------------------------------------------------\n"
        "Options:\n"
        "  -s               Run in server mode (default)\n"
        "  -c server_address Run in client mode and connect to the specified address\n"
        "  -p port          Specify port for server or client\n"
        "  -u socket_path   Specify path to Unix socket\n"
        "  -h               Show this help message\n\n"
        "---------------------------------------------------------------------------\n"
        "Internal commands:\n"
        "  /help             Show this help message\n"
        "  /quit             Close the current connection\n"
        "  /halt             Shut down the entire program\n\n"
        "---------------------------------------------------------------------------\n"
        "Special character support:\n"
        "  #                Comment - ignores the rest of the line\n"
        "  /help # /quit    Command do only print help function\n "
        "  ;                Command separator\n"
        "  /help ; /quit    Command help function and then quit\n "
        "  <                Input redirection\n"
        "  >                Output redirection\n"
        "  |                Pipe - connect the output of one command to the input of another\n"
        "---------------------------------------------------------------------------\n"
    );
    return help_message;
}

void cleanup(int max_clients) {
    // Close all client sockets
    for (int i = 0; i < max_clients; i++) {
        if (client_sockets[i] != -1) {
            close(client_sockets[i]);
            client_sockets[i] = -1;
        }
    }
    
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
    
    printf("Cleanup completed.\n");
}