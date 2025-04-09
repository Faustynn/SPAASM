# README for Zadanie1, Zadanie2, and Zadanie3

## Zadanie1 - Asemblery  

### Compilation:

To compile the project files, follow these steps:

1. Navigate to the directory where your files are saved:
   ```bash
   cd path/to/your/files
   ```

2. Compile `main.asm` and `print.asm`:
   ```bash
   as main.asm -o main.o
   as print.asm -o print.o
   ```

3. Link the object files to create the executable:
   ```bash
   ld main.o print.o -o main
   ```

### Usage:

To use the program, follow the command syntax below:

1. **Help Command**:
   Display the help information for available options:
   ```bash
   ./main -h
   ```

2. **Standard Calling (change lowercase to uppercase)**:
   ```bash
   ./main file.txt
   ```

3. **Lowercase Conversion (change lowercase to uppercase)**:
   ```bash
   ./main -l file.txt
   ```

4. **Uppercase Conversion (change uppercase to lowercase)**:
   ```bash
   ./main -u file.txt
   ```

5. **Reverse File Path Reading** (like standard but reads the file in reverse path):
   ```bash
   ./main -r file.txt
   ```

### Multiple File Support:

You can process multiple files at once:
```bash
./main file.txt big.txt
```

You can also process files with similar names:
```bash
./main file.txt file.txt
```

### Code Efficiency:

The program supports processing large files, including those larger than 64KB.

---

## Zadanie2 - Systémové programovanie

### Project Structure

```
Zadanie2
├── code
│   ├── client.c        # Client functionality implementation
│   ├── server.c        # Server functionality implementation
│   ├── main.c          # Entry point of the application
│   ├── global.c        # Implement global functions
│   ├── global.h        # Header for global functions file 
│   ├── utils.c         # Utility functions for shared functionalities
│   └── utils.h         # Header file for utility functions
├── config              # Dir with example config files
│   ├── example.conf
├── logs                # Dir with example log files
│   ├── example.log
├── scripts             # Dir with example scripts
│   ├── example.sh
├── .env                # file with env settings 
├── client              # Compiled client script, for running
├── server              # Compiled server script, for running
└── Makefile            # Build instructions for the project
```

### Building the Project

To build the project, navigate to the project directory and run the following commands:

```bash
set -a
source .env
set +a
make
```

This will compile the source files and create the executable file and set your environment settings.

### Special Characters

The following special characters are available:

```
# ; < > /help /halt /quit
```

### Command-Line Options

#### Server Options:
- `-l` [filename] - Logs commands to the specified file
- `-C` [filename] - Loads configuration from the specified file (prompt, idle time, log file, etc.)
- `-c` - Executes the provided command and then terminates
- `-v` - Outputs debug information to stderr

#### Client Options:
- `-c` - Runs in client mode
- `-C` [filename] - Loads configuration from the specified file
- `-c` [command] - Executes the provided command
- `-v` - Outputs debug information to stderr
- `-p` [port] - Specifies the port number

### Run Application Examples

#### Client Examples:
```bash
./client -c
./client -c -v
./client -c 127.0.0.1 -p 8088
./client -c -C ../main/Zadanie2/config/test.properties
./client -c -c "command"
```

#### Server Examples:
```bash
./server -s
./server -s -v
./server -s 127.0.0.1 -p 8088
./server -s 127.0.0.1 -p 8088 -l
./server -s 127.0.0.1 -p 8088 -l ../main/Zadanie2/config/config.properties
./server -s -C ../main/Zadanie2/config/test.properties
```

#### Script Execution Examples:
```bash
./scripts/hello
./scripts/big.sh
./scripts/test.sh
```

---

## Zadanie3

### Coming Soon...
