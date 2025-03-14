.global _main

.include "macra.s"           
.extern _print_error_message

.data
help:
    .ascii "\n_____________________________________________________________________________________________\n"
    .ascii " HELP MENU: \n"
    .ascii " ./main -h -> print help info \n"
    .ascii " ./main file.txt file2.txt file3.txt-> change all lowercase to uppercase letters and show number of changes \n"
    .ascii " ./main -r file.txt file2.txt ... -> write output in reverse order where logic ^ \n"
    .ascii " ./main -u file.txt file2.txt ... -> uppercase to lowercase \n"
    .ascii " ./main -l file.txt file2.txt .. -> lowercase to uppercase \n"
    .ascii "_____________________________________________________________________________________________\n\n"
len_help = . - help


error:
    .ascii "  Invalid input, try ./main -h\n"
len_err = . - error


error_file:
    .ascii "  Error while opening file\n"
len_err_file = . - error_file


changes_msg:
    .ascii "  Changes: "
len_changes_msg = . - changes_msg


newline:
    .ascii "\n"
len_newline = . - newline

file_msg:
    .ascii "\nProcessing file: "
len_file_msg = . - file_msg

buffer:
    .skip 4096            

temp_buffer:
    .skip 4096          

number_buffer:
    .skip 20            


// Main program logic
.text
_main:                           // Start of the main function 
    stp x29, x30, [sp, #-16]!    // Save values for return upon function exit
    mov x29, sp                  // Set the frame pointer (FP) to the current SP value
    
    mov x22, x1                  // Save arguments from register x1 (argv) to register x35
    mov x20, x0                  // Save the number of arguments in x20

    cmp x0, #1                   // Compare the number of arguments with 1
    beq print_error              // If less, call the error function; otherwise, continue
    
    mov x25, #3                  // Set the default flag to lower_to_upper
    ldr x1, [x22, #8]            // Load the user argument from register x19 into x1
    
    ldrb w2, [x1]                // Load the first character from the first register into w2
    cmp w2, #0x2D                // Check if the character is "-" in ASCII
    bne start_processing         // If not, it's a file, and we proceed to the file opening logic
    
    // We encountered a flag
    ldrb w2, [x1, #1]            // Load the second character from the first register into w2
    cmp w2, #0x68                // Check if the character is "h" in ASCII
    beq print_help               // If yes, proceed to the print_help logic
    
    cmp w2, #0x72                // Check if the character is "r" in ASCII
    beq set_reverse_flag         // If yes, proceed to the file handling logic
    
    cmp w2, #0x75                // Check if the character is "u" in ASCII
    beq set_upper_to_lower_flag  // If yes, proceed to the file handling logic

    cmp w2, #0x6C                // Check if the character is "l" in ASCII
    beq set_lower_to_upper_flag  // If yes, proceed to the file handling logic

    b print_error                // If something else, call the error printing logic


set_reverse_flag:
    mov x25, #1                   // Set the reverse flag
    add x22, x22, #8              // Move to the next argument (first file)
    sub x20, x20, #1              // Decrement the number of remaining arguments
    b start_processing

set_upper_to_lower_flag:
    mov x25, #2                   // Set the upper_to_lower flag
    add x22, x22, #8              // Move to the next argument (first file)
    sub x20, x20, #1              // Decrement the number of remaining arguments
    b start_processing

set_lower_to_upper_flag:
    mov x25, #3                   // Set the lower_to_upper flag
    add x22, x22, #8              // Move to the next argument (first file)
    sub x20, x20, #1              // Decrement the number of remaining arguments
    b start_processing

start_processing:
    cmp x20, #1                   // Check if there are any files left to process
    ble exit_prog                 // If not, exit the program
process_files:
    cmp x20, #1                   // Check if there are any files left to process
    ble exit_prog                 // If not, exit the program

    ldr x1, [x22, #8]             // Load the next argument (file name)
    add x22, x22, #8              // Move to the next argument
    sub x20, x20, #1              // Decrement the number of remaining arguments

    mov x0, x1                    // Move the file name to x0
    mov x1, #0                    // Set the flag for read-only
    mov x16, #5                   // System call for opening a file
    svc #0x80

    cmp x0, #0                    // Check if the file was opened successfully
    blt print_err_file            // If there was an error, print the error message
    mov x19, x0                   // Save the file descriptor in x19

    // Save the registers we will use
    stp x20, x21, [sp, #-16]!     // Save x20 and x21 on the stack
    stp x22, x23, [sp, #-16]!     // Save x22 and x23 on the stack
    stp x24, x25, [sp, #-16]!     // Save x24 and x25 on the stack
    
    mov x20, #0                   // Register x20 will store our change counter

read_loop:
    read_part_of_file             // Call the macro to read bytes from the file

    cmp x0, #0                    // Have we reached the end of the file?
    beq print_changes             // If yes (x0=0), print the number of changes
    blt file_read_error           // If x0<0, there was an error reading the file
    
    mov x21, x0                   // Save the number of bytes read in register x21
    
    adrp x1, buffer@PAGE          // Load the address of the main buffer
    add x1, x1, buffer@PAGEOFF
    adrp x2, temp_buffer@PAGE     // Load the address of the temporary buffer where changes will be stored
    add x2, x2, temp_buffer@PAGEOFF
    mov x3, x21                   // Load register x21 (number of bytes read) into register x3 as a counter 
    
    // Save the pointer to the start of the buffer for reverse mode
    mov x23, x1                   // Save the initial buffer address

    cmp x25, #1                   // Check the operation mode
    beq handle_reverse            // If reverse, use separate handling
    b process_string              // Otherwise, process character by character

handle_reverse:
    // Correct position calculation for reverse mode
    mov x1, x23                   // Restore the original buffer address
    add x1, x1, x21               // Add the size of the read data
    sub x1, x1, #1                // Set the pointer to the last byte
    b reverse_file_loop           // Proceed to the reverse processing loop

process_string:
    ldrb w4, [x1], #1             // Load 1 byte from the buffer, incrementing register x1
    
    cmp x25, #2                   // Check for upper_to_lower
    beq do_upper_to_lower         // If matched, call the upper_to_lower logic

    cmp x25, #3                   // Check for lower_to_upper
    beq do_lower_to_upper         // If matched, call the lower_to_upper logic

    b do_lower_to_upper           // Default to lower_to_upper

reverse_file_loop:
    ldrb w4, [x1], #-1            // Load a byte from the end and move backward

    cmp w4, #'a'                  // Compare with 'a'
    blt store_reverse_char        // If less, skip as it's not a letter
    cmp w4, #'z'                  // Compare with 'z'
    bgt store_reverse_char        // If greater, skip as it's not a letter

    sub w4, w4, #32               // Convert the letter to uppercase
    add x20, x20, #1              // Increment the change counter
store_reverse_char:
    strb w4, [x2], #1             // Save the byte in temp_buffer
    subs x3, x3, #1               // Decrement the counter
    bne reverse_file_loop         // If the counter is not 0, continue the loop

    write_message temp_buffer, x21  // Print the buffer contents to the screen
    b reset_buffers                 // Reset buffers before the next read

do_upper_to_lower:
    cmp w4, #'A'                 // Compare with 'A'
    blt store_char               // If less, skip as it's not a letter
    cmp w4, #'Z'                 // Compare with 'Z'
    bgt store_char               // If greater, skip as it's not a letter
    
    add w4, w4, #32              // Convert the letter to lowercase by adding 32 in ASCII
    add x20, x20, #1             // Increment the change counter
    b store_char                 // Proceed to the logic for saving changes to the buffer

do_lower_to_upper:
    cmp w4, #'a'                 // Compare with 'a'
    blt store_char               // If less, skip as it's not a letter
    cmp w4, #'z'                 // Compare with 'z'
    bgt store_char               // If greater, skip as it's not a letter
    
    sub w4, w4, #32              // Convert the letter to uppercase by subtracting 32 in ASCII
    add x20, x20, #1             // Increment the change counter
    b store_char                 // Proceed to the logic for saving changes to the buffer

store_char:
    strb w4, [x2], #1            // Write the character to the temporary buffer and increment x2
    
    subs x3, x3, #1              // Decrement the counter of unprocessed bytes
    bne process_string           // If the counter is not 0, continue processing the buffer
    
    write_message temp_buffer, x21  // Print the buffer contents to the screen

reset_buffers:
    // Clear buffers before the next read
    mov x21, #0                  // Value for clearing
    mov x22, #4096               // Buffer size
    
    adrp x1, buffer@PAGE         // Address of buffer
    add x1, x1, buffer@PAGEOFF
    
clear_loop:
    strb w21, [x1], #1           // Write 0 and increment pointer
    subs x22, x22, #1            // Decrement counter
    bne clear_loop               // Continue until the buffer is cleared
    
    // Do the same for temp_buffer
    mov x22, #8192
    adrp x2, temp_buffer@PAGE
    add x2, x2, temp_buffer@PAGEOFF
    
clear_temp_loop:
    strb w21, [x2], #1          // Write 0 and increment pointer
    subs x22, x22, #1           // Decrement counter
    bne clear_temp_loop         // Continue until the buffer is cleared
    
    b read_loop

print_changes:
    write_message newline, len_newline          // Print a newline for better visuals
    write_message changes_msg, len_changes_msg  // Print the changes message

    sub sp, sp, #64                             // Allocate 64 bytes on the stack
    mov x2, sp                                  // Save the current stack pointer in register x2
    mov x1, x20                                 // Save our counter in register x1
    mov x3, sp                                  // Save another stack pointer in register x3

    cmp x1, #0                                  // Compare the counter value with 0
    beq print_zero                              // If true, proceed to the logic for printing 0

collect_digits:                                 // Convert the number to a string
    mov x4, #10                                 // Divide the number by 10   
    udiv x5, x1, x4                             // x5 = x1 / 10
    msub x6, x5, x4, x1                         // Save the remainder in register x6
    add x6, x6, #'0'                            // Convert the remainder to ASCII format
    strb w6, [x3], #1                           // Save our ASCII number in the buffer and increment the buffer pointer x3 by 1 for the next number 
    mov x1, x5                                  // Load the remainder into register x1
    cbnz x1, collect_digits                     // Continue until the remainder is 0

    mov x4, sp                                  // Load our allocated buffer into register x4
    sub x5, x3, #1                              // Load the pointer to the end of our buffer into register x5
reverse_loop:
    cmp x4, x5                                  // Are we at the end of the buffer?
    bge print_result                            // If yes, proceed to printing
    ldrb w6, [x4], #1                            // Load 1 byte from the START of the buffer
    ldrb w7, [x5]                               // Load 1 byte from the END of the buffer
    strb w7, [x4], #1                           // Swap the start with the end
    strb w6, [x5], #-1                          // Swap the end with the start
    b reverse_loop                              // Restart the loop

print_zero:
    mov w6, #'0'                                // Load '0' in ASCII format into the 32-bit register
    strb w6, [x3], #1                           // Save it
    b print_result                              // Call the printing logic

print_result:
    mov x5, sp                                  // Load the start of the buffer into register x5
    sub x2, x3, x5                              // Calculate the output length: x2 = x3 - x5 = length
    mov x0, #1                                  // Call stdout
    mov x1, sp                                  // Load the stack pointer into register x1
    mov x16, #4                                 // Call the write command
    svc #0x80                                   // Execute the system call 

    add sp, sp, #64                             // Reset the stack pointer by returning the allocated 64 bytes
    write_message newline, len_newline          // Print a newline for better visuals
    write_message newline, len_newline          // Print a newline for better visuals

    b full_clear_buffers                        // Fully clear buffers before exiting

file_read_error:
    b full_clear_buffers                        // Clear buffers before exiting with an error

full_clear_buffers:
    // Fully clear buffers
    mov x21, #0                                // Value to write (0)
    adrp x1, buffer@PAGE                       // Load the high part of the buffer address into x1
    add x1, x1, buffer@PAGEOFF                 // Add the low part of the address
    mov x2, #4096                              // Buffer size (4096 bytes)

clear_buffer_loop:
    strb w21, [x1], #1                         // Write 0 to memory, increment address
    subs x2, x2, #1                            // Decrement counter
    bne clear_buffer_loop                      // Repeat if not all bytes are cleared

    // Clear temp_buffer
    adrp x1, temp_buffer@PAGE                  // Load the high part of the temp_buffer address into x1
    add x1, x1, temp_buffer@PAGEOFF            // Add the low part of the address
    mov x2, #4096                              // Buffer size (4096 bytes)

clear_temp_buffer_loop:
    strb w21, [x1], #1                         // Write 0 to memory, increment address
    subs x2, x2, #1                            // Decrement counter
    bne clear_temp_buffer_loop                 // Repeat if not all bytes are cleared

    close_file                                  // Close the file

    // Restore saved registers
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16  
    ldp x20, x21, [sp], #16
    
    b process_files               // Proceed to the next file



// Error printing logic
print_error:
    adrp x0, error@PAGE          // Load the address of the error message
    add x0, x0, error@PAGEOFF    
    mov x1, len_err              // Load the length of the message
    bl _print_error_message      // Call the external procedure
    b exit_prog                  // Call the program termination logic

print_help:                                 
    write_message help, len_help            // Call the macro to print the help message with its calculated length
    b exit_prog                             // Call the program termination logic

print_err_file:
    adrp x0, error_file@PAGE     // Load the address of the file error message
    add x0, x0, error_file@PAGEOFF
    mov x1, len_err_file         // Load the length of the message
    bl _print_error_message      // Call the external procedure
    
    b process_files              // Continue with other files


// End program logic
exit_prog:                                  
    ldp x29, x30, [sp], #16                 // Restore registers x29 and x30 from the stack
    mov x0, #0                              // Set the program exit code to 0, indicating successful execution
    ret                                     // Return from _main, finally terminating the program