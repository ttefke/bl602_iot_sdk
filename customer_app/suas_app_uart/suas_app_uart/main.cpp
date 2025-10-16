extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// BouffaloLabs includes (UART)
#include <bl_uart.h>

// Standard input/output
#include <stdio.h>

// Sensor header
#include <suas_mmWave.h>
}

#include <etl/string.h>

/* Connection setup
    mmWave  PineCone
    GND     GND
    VCC     3V3
    TX      IO4
    RX      IO3
*/

void read_sensor([[gnu::unused]] void *pvParameters) {
  /* Initialize sensor */
  suas_mmwave_init();

  /* Read sensor */
  while (1) {
    if (suas_mmwave_is_human_present()) {
      printf("Human presence detected\r\n");
    } else {
      printf("Nobody present\r\n");
    }

    // Wait for a second
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Delete task -- should never happen
  vTaskDelete(nullptr);
}

extern "C" void bfl_main(void) {
  /* Initialize system */
  vInitializeBL602();

  /* Set up sensor reading task */
  constexpr uint16_t SENSOR_STACK_SIZE = 256;
  static StackType_t sensor_stack[SENSOR_STACK_SIZE]{};
  static StaticTask_t sensor_task{};

  /* Create and start task */
  xTaskCreateStatic(
      read_sensor, /* name of the function implementing the task */
      etl::string_view("Sensor reading task").data(), /* Human readable name */
      SENSOR_STACK_SIZE,                              /* Stack size */
      nullptr,      /* Additional parameters for read_sensor (none)*/
      15,           /* Task priority */
      sensor_stack, /* Stack to use*/
      &sensor_task  /* Task handle for potential API calls */
  );

  vTaskStartScheduler();
}
