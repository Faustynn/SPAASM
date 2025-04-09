CC = gcc
CFLAGS = -Wall -Wextra -I.

# Dirs.
CONFIG_FILE = config/config.properties
LOGS_FILE = logs/logs.log

# Source files
SRCS = code/client.c code/global.c code/main.c code/server.c code/utils.c

# Header files
HDRS = code/global.h code/utils.h

# Object files
OBJS = $(SRCS:.c=.o)

# Output
TARGETS = client server
all: $(TARGETS)

# Compile
client: $(SRCS) $(HDRS) $(CONFIG_FILE)
	$(CC) $(CFLAGS) $(SRCS) -o client  

server: $(SRCS) $(HDRS) $(CONFIG_FILE)
	$(CC) $(CFLAGS) $(SRCS) -o server 

# Clean
clean:
	rm -f $(OBJS) $(TARGETS) 

# Phony targets
.PHONY: all clean client server