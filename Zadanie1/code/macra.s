.global _main
.extern _print_error_message


// Main macro for printing errors to the console
.macro write_message msg, len
    mov x0, #1                  // Load 1 (stdout) into register x0
    adrp x1, \msg@PAGE          // Load the first (upper) 48 bits of the message into register x1
    add x1, x1, \msg@PAGEOFF    // Load the remaining 12 bits of the message into register x1
    mov x2, \len                // Load the length of the given string into register x2
    mov x16, #4                 // Load 4 (write command) into register x16
    svc #0x80                   // Execute the system call to print text to the console
.endm  // In other words, we execute write(1, msg, len)



// Macro for reading a part of a file
.macro read_part_of_file
    mov x0, x19                  // Load the file descriptor from register x19 into x0
    adrp x1, buffer@PAGE         // Load the first (upper) 48 bits of the buffer into register x1
    add x1, x1, buffer@PAGEOFF   // Load the remaining 12 bits of the buffer into register x1
    mov x2, #4096                // Read 4096 bytes from the file
    mov x16, #3                  // Load 3 (read command) into register x16
    svc #0x80                    // Execute the system call to read 4096 bytes from the file

.endm // In other words, we execute read(file_descriptor, buffer, 4096)


.macro close_file
    mov x0, x19         // Load the file descriptor into register x0
    mov x16, #6         // Load 6 (close command) into register x16
    svc #0x80           // Call the system command with the given parameters
.endm
