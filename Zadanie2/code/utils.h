#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <limits.h>
#include <stdbool.h>

#define MAX_SERVER_ADDRESS_LEN 16
#define MAX_LOG_FILE_PATH 256
#define MAX_PROMPT_PREFIX 50
#define MAX_BANNED_COMMANDS 10
#define MAX_COMMAND_LENGTH 1024
#define HOST_NAME_MAX 64
#define MAX_PROMPT_SIZE 128

// struct for sever configuration
typedef struct {
    // Network Configuration
    int port;
    char server_address[MAX_SERVER_ADDRESS_LEN];
    int inet_addr_len;

    // Server Connection Limits
    int max_clients;
    int max_socket_listen_conn;
    int client_timeout;       // Seconds of inactivity before disconnection
    int max_connection_time;  // Maximum time a client can stay connected

    // Prompt and Display Settings
    int prompt_size;
    int buffer_size;
    char prompt_prefix[MAX_PROMPT_PREFIX];
    int show_system_info;

    // Logging Configuration
    char log_file[MAX_LOG_FILE_PATH];

    // Security Settings
    int max_command_length;
    int allow_remote_exec;
    char banned_commands[MAX_BANNED_COMMANDS][MAX_COMMAND_LENGTH];
    int banned_commands_count;

    // Performance Tuning
    int thread_pool_size;
    int max_concurrent_commands;
} ServerConfig;


// Function prototypes
void generate_prompt(char *prompt, const ServerConfig *config);
int read_properties(const char *filename, ServerConfig *config);
char* print_help(void);
void cleanup(int max_clients);

// Logging functions
void log_message(const ServerConfig *config, const char *message);
void rotate_log_file(const ServerConfig *config);

// Server and client mode functions
void server_mode(ServerConfig *config, char *prompt, const char *logfile, const bool stderr_mode);
void client_mode(ServerConfig *config, char *prompt, char *command, const bool stderr_mode);

#endif