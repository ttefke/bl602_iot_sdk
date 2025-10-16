extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// UART HAL
#include <bl_uart.h>

// Long integers
#include <inttypes.h>

// Ultrasonic driver
#include <suas_ultrasonic.h>
}

#include <etl/string.h>

/* Connection setup
    Sensor  PineCone
    GND     GND
    VCC     3V3
    DATA    IO2
*/

// Task that regularly queries the sensor
void task_dht_ultrasonic([[gnu::unused]] void *pvParameters) {
  // GPIO pin used to connect to Ultrasonic ranger
  constexpr uint8_t DATA_PIN = 2;

  while (1) {
    printf("Distance: %" PRIu32 " cm, ",
           suas_ultrasonic_measure_in_centimeters(DATA_PIN));
    vTaskDelay(pdMS_TO_TICKS(200));
    printf("%" PRIu32 " mm\r\n",
           suas_ultrasonic_measure_in_millimeters(DATA_PIN));
    vTaskDelay(pdMS_TO_TICKS(200));
  }

  printf("Should never happen - exited ultrasonic task\r\n");
  vTaskDelete(nullptr);
}

/* Main function */
extern "C" void bfl_main(void) {
  /* Task and stack definition */
  constexpr uint16_t ULTRASONIC_STACK_SIZE = 256;
  constinit static StackType_t ultrasonic_stack[ULTRASONIC_STACK_SIZE]{};
  constinit static StaticTask_t ultrasonic_task{};

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
      /* Baud rate */ 2 * 1000 * 1000);

  xTaskCreateStatic(task_dht_ultrasonic, etl::string_view("ultrasonic").data(),
                    ULTRASONIC_STACK_SIZE, nullptr, 15, ultrasonic_stack,
                    &ultrasonic_task);

  vTaskStartScheduler();
}
