
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

## Project Structure
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
├──client               # Compiled client script, for running
├──server               # Compiled server script, for running
└── Makefile             # Build instructions for the project
```

## Building the Project
To build the project, navigate to the project directory and run the following command:

```
make
```

This will compile the source files and create the executable file

## Run  Application
After building the project, you can run the application in either server or client mode:

- To run as a server:
    On localhost 8088 port
  ```
  ./server -s
  ```

    On your post and address
    
  ```
  ./server -s <server_address> -p <port>
  ```

- To run as a client:
  ```
  ./client -c <server_address> -p <port>
  ```

Replace `<server_address>` with the address of the server and `<port>` with your port number

## Features
- Internal commands: `/help`, `/quit`, `/halt`
- Handles multiple client connections
- Provides a prompt displaying the current time, user, and hostname

---

## Zadanie3

### Coming Soon...