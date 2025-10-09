// Stdandard input / output
#include <stdio.h>

// BouffaloLabs HALs
#include <bl_uart.h>
#include <hal_hwtimer.h>
#include <bl_timer.h>

// 74HC595 library
#include <suas_74hc595.h>

/* Connection setup
    744hc595  PineCone
    VCC       3V3
    GND       GND
    OE        0
    DS        1
    SHCP      2
    STCP      3
    MR     4
*/

void bfl_main(void)
{
  /* Initialize UART
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);
  printf("Hello world!\r\n");

  hal_hwtimer_init();

  // Configure shift register
  suas_74hc595_conf_t sr_config = {
      .with_output_enable = 1,
      .pin_output_enable = 0,
      .pin_data_signal = 1,
      .pin_shift_clock = 2,
      .pin_store_clock = 3,
      .with_master_reset = 1,
      .pin_master_reset = 4,
      .number_of_registers = 2};

  suas_74hc595_config(&sr_config);

  // Shift out data
  for (uint8_t i = 0; i < 129; i++)
  {
    bl_timer_delay_us(1 * 1000 * 1000);
    suas_74hc595_store(&sr_config, i);
  }
}
