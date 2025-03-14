#ifndef UTILS_H
#define UTILS_H

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

#define PROPERTIES_FILE "config/config.properties"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

void generate_prompt(char *prompt, int prompt_size);
char* print_help(void);
void cleanup(int max_clients);

void server_mode(int port, int buffer_size, int max_clients, int max_socket_listen_conn, char prompt[], int prompt_size);
void client_mode(const char *server_address, int port, int buffer_size, char prompt[], int prompt_size);

#endif