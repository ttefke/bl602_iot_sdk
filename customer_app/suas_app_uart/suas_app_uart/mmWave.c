// FreeRtos includes
#include <FreeRTOS.h>
#include <task.h>

// BouffaloLabs includes (UART)
#include <bl_uart.h>

// Input/output
#include <stdio.h>

// Our headers
#include "mmWave.h"

// Sensor: waveshare HMMD mmWave: https://www.waveshare.com/wiki/HMMD_mmWave_Sensor

void read_sensor([[gnu::unused]] void *pvParameters)
{
  printf("Task to read sensor started\r\n");

  /* Read sensor */
  while(1) {
    // Data array
    int8_t data[DATA_ARRAY_LENGTH];

    /* If someone is present we receive the text 'ON' plus distance or 'OFF'
     * To determine human presence, only two bytes must be read, the second byte tells
     * if someone is present */
    for (uint8_t i = 0; i < DATA_ARRAY_LENGTH;) {
      int8_t result = bl_uart_data_recv(1 /* User defined channel */);
      // drop result if invalid
      if (result < 0) {
        continue;
      // only start reading if 'O' is first character (line begin)
      } else if ((i == 0) && (result != 0x4F)) {
        continue;
      // write data to array
      } else {
        data[i] = result;
        i++; /* only increment if we wrote data*/
      }
    }

    // Second character is 'F' -> OFF/no human presence
    if (data[1] == 0x46) {
      printf("Nobody present\r\n");
    // Someone is present (second character is 'N')
    } else if (data[1] == 0x4E) {
      printf("Human presence detected\r\n");
    }

    // Wait for a second
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  // Delete task -- should never happen
  vTaskDelete(NULL);
}
