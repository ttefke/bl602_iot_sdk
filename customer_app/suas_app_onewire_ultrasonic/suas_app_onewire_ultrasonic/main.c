// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// UART HAL
#include <bl_uart.h>

// Long integers
#include <inttypes.h>

// Ultrasonic driver
#include <suas_ultrasonic.h>

#define DATA_PIN 2 // GPIO pin used to connect to Ultrasonic ranger
#define ULTRASONIC_STACK_SIZE 256

/* Connection setup
    Sensor  PineCone
    GND     GND
    VCC     3V3
    DATA    IO2
*/

// Task that regularly queries the sensor
void task_dht_ultrasonic([[gnu::unused]] void *pvParameters) {
    while (1) {
        printf("Distance: %"PRIu32" cm, ", suas_ultrasonic_measure_in_centimeters(DATA_PIN));
        vTaskDelay(pdMS_TO_TICKS(200));
        printf("%"PRIu32" mm\r\n", suas_ultrasonic_measure_in_millimeters(DATA_PIN));
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    printf("Should never happen - exited ultrasonic task\r\n");
    vTaskDelete(NULL);
}

/* Main function */
void bfl_main(void)
{
  /* Task and stack definition */
  static StackType_t ultrasonic_stack[ULTRASONIC_STACK_SIZE];
  static StaticTask_t ultrasonic_task;

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

  extern void task_dht_ultrasonic(void *pvParameters);
  xTaskCreateStatic(task_dht_ultrasonic, (char* )"ultrasonic", ULTRASONIC_STACK_SIZE, NULL, 15, ultrasonic_stack, &ultrasonic_task);

  vTaskStartScheduler();
}
