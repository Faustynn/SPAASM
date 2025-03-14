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

void generate_prompt(char *prompt, int prompt_size) {
    struct passwd *pw;
    char hostname[HOST_NAME_MAX + 1];
    time_t current_time;
    struct tm *time_info;
    char time_str[10];

    pw = getpwuid(getuid());
    gethostname(hostname, sizeof(hostname));
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_str, sizeof(time_str), "%H:%M", time_info);

    snprintf(prompt, prompt_size, "%s %s@%s# ", time_str, pw->pw_name, hostname);
}

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