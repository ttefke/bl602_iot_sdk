// Standard input/output
#include <stdio.h>

// UART Hardware Abstraction Layer
#include <bl_uart.h>

void bfl_main(void)
{
  /* Initialize UART
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(
    /* UART channel (0 = USB port)*/ 0,
    /* Transmission pin */ 16,
    /* Receive pin */ 7,
    /* Unused */ 255, 
    /* Unused */ 255,
    /* Baud rate */ 2 * 1000 * 1000
  );
  printf("Hello world!\r\n");
}
