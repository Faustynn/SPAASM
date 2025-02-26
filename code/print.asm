.global _print_error_message

//   x0 - pointer to error message
//   x1 - length of error message

.text
_print_error_message:
    stp x29, x30, [sp, #-16]!               // Save registers
    mov x29, sp                             // Set frame pointer to current SP
    
    mov x2, x1          // Mess. length to x2
    mov x1, x0          // Mess. pointer to x1
    mov x0, #1          // File descriptor (stdout = 1)
    mov x16, #4         // System call write
    svc #0x80           // Make sys call
    
    // Restore registers and return
    ldp x29, x30, [sp], #16
    ret