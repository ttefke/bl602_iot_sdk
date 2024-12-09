#include <FreeRTOS.h>
#include <task.h>

#include <stdio.h>

#include <bl_dma.h>
#include <bl_gpio.h>
#include <bl_irq.h>
#include <bl_sec.h>
#include <bl_sys_time.h>
#include <bl_uart.h>
#include <hal_boot2.h>
#include <hal_board.h>
#include <hal_hwtimer.h>

#include <blog.h>
#include <lwip/tcpip.h>

#include "leds.h"

/* Define heap regions */
extern uint8_t _heap_start;
extern uint8_t _heap_size;
extern uint8_t _heap_wifi_start;
extern uint8_t _heap_wifi_size;

static HeapRegion_t xHeapRegions[] =
    {
        {&_heap_start, (unsigned int)&_heap_size},
        {&_heap_wifi_start, (unsigned int)&_heap_wifi_size},
        {NULL, 0},
        {NULL, 0}};

/* main function, execution starts here */
void bfl_main(void)
{
  /* Define information containers for tasks */
  static StackType_t wifi_stack[1024];
  static StaticTask_t wifi_task;

  static StackType_t httpd_stack[512];
  static StaticTask_t httpd_task;

  /* Initialize UART
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);
  printf("[SYSTEM] Starting up!\r\n");

  /* (Re)define Heap */
  vPortDefineHeapRegions(xHeapRegions);

  /* Initialize system */
  blog_init();
  bl_irq_init();
  bl_sec_init();
  bl_dma_init();
  hal_boot2_init();
  hal_board_cfg(0);

  /* Configure LED pins as GPIO output */
  bl_gpio_enable_output(LED_R_PIN, 0, 0);
  bl_gpio_enable_output(LED_G_PIN, 0, 0);
  bl_gpio_enable_output(LED_B_PIN, 0, 0);

  /* Set all LEDs to low */
  led_state.state_led_red = LED_OFF;
  led_state.state_led_green = LED_OFF;
  led_state.state_led_blue = LED_OFF;
  apply_led_state();

  /* Start tasks */

  printf("[SYSTEM] Starting httpd task\r\n");
  extern void task_httpd(void *pvParameters);
  xTaskCreateStatic(task_httpd, (char *)"httpd", 512, NULL, 10, httpd_stack, &httpd_task);

  printf("[SYSTEM] Starting WiFi task\r\n");
  extern void task_wifi(void *pvParameters);
  xTaskCreateStatic(task_wifi, (char *)"wifi", 1024, NULL, 16, wifi_stack, &wifi_task);

  /* Start TCP/IP stack */
  printf("[SYSTEM] Starting TCP/IP stack\r\n");
  tcpip_init(NULL, NULL);

  /* Start scheduler */
  printf("[SYSTEM] Starting scheduler\r\n");
  vTaskStartScheduler();
}
