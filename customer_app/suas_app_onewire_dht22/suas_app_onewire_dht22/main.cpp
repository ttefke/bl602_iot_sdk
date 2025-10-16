extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// UART HAL
#include <bl_uart.h>

// DHT22 driver
#include <suas_dht22.h>
}

#include <etl/string.h>

/* Connection setup
    DHT22   PineCone
    GND     GND
    VCC     3V3
    DATA    IO2
*/

void task_dht22([[gnu::unused]] void *pvParameters) {
  // GPIO pin used to connect to DHT22
  constexpr uint8_t DATA_PIN = 2;

  // Repeatedly get temperature and humidity
  while (1) {
    printf("Current temperature: %.1f\r\n",
           suas_dht22_get_temperature(DATA_PIN));
    vTaskDelay(pdMS_TO_TICKS(2500));
    printf("Current humidity: %.1f\r\n", suas_dht22_get_humidity(DATA_PIN));
    vTaskDelay(pdMS_TO_TICKS(2500));
  }

  printf("Should never happen - exited DHT22 task\r\n");
  vTaskDelete(nullptr);
}

/* Main function */
extern "C" void bfl_main(void) {
  /* Task and stack definition */
  constexpr uint16_t DHT22_STACK_SIZE = 256;
  constinit static StackType_t dht22_stack[DHT22_STACK_SIZE]{};
  constinit static StaticTask_t dht22_task{};

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

  xTaskCreateStatic(task_dht22, etl::string_view("dht22").data(),
                    DHT22_STACK_SIZE, nullptr, 15, dht22_stack, &dht22_task);

  vTaskStartScheduler();
}
