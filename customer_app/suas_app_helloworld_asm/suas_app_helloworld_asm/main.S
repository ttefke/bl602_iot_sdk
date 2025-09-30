# See suas_app_helloworld for C equivalent

# Export program entry
.global bfl_main

# Data segment
.section .rodata
message: .asciz "Hello world!\r\n"

# Source code
.text

bfl_main:
    # Initialize UART
    # Registers a0-a7 are used for function parameters
    # So the first argument goes into a0, the second into a1 and so on
    # If the numbers of registers should be insufficient (not the case here) you have to use the stack pointer

    # Initialize UART channel
    mv   a0, x0       # 0 is the first UART channel (USB port translation), x0 register is always zero
    li   a1, 16       # 16: Transmission pin
    li   a2, 7        # 7: Receive pin
    li   a3, 255      # Unused -> 255 per convention for the UART HAL
    li   a4, 255      # Same
    li   a5, 2000000  # Set baudrate to two million
    call bl_uart_init # Call UART Initialization function

    # Show hello world message
    la   a0, message  # Load message into first register
    call printf       # Show hello world message

    # Finish
    ret