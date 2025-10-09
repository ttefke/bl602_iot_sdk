// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// UART HAL
#include <bl_uart.h>

// DHT22 driver
#include <suas_dht22.h>

#define DATA_PIN 2 // GPIO pin used to connect to DHT22
#define DHT22_STACK_SIZE 256

/* Connection setup
    DHT22   PineCone
    GND     GND
    VCC     3V3
    DATA    IO2
*/

void task_dht22([[gnu::unused]] void *pvParameters) {
    while (1) {
        printf("Current temperature: %.1f\r\n", suas_dht22_get_temperature(DATA_PIN));
        vTaskDelay(pdMS_TO_TICKS(2500));
        printf("Current humidity: %.1f\r\n", suas_dht22_get_humidity(DATA_PIN));
        vTaskDelay(pdMS_TO_TICKS(2500));
    }

    printf("Should never happen - exited DHT22 task\r\n");
    vTaskDelete(NULL);
}

/* Main function */
void bfl_main(void)
{
  /* Task and stack definition */
  static StackType_t dht22_stack[DHT22_STACK_SIZE];
  static StaticTask_t dht22_task;

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

  extern void task_dht22(void *pvParameters);
  xTaskCreateStatic(task_dht22, (char* )"dht22", DHT22_STACK_SIZE, NULL, 15, dht22_stack, &dht22_task);

  vTaskStartScheduler();
}
