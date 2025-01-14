#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <dht22.h>

#define DATA_PIN 14

void task_dht22(void *pvParameters) {
    while (1) {
        printf("Current temperature: %.1f\r\n", getTemperature(DATA_PIN));
        vTaskDelay(2500 / portTICK_PERIOD_MS);
        printf("Current humidity: %.1f\r\n", getHumidity(DATA_PIN));
        vTaskDelay(2500 / portTICK_PERIOD_MS);
    }

    printf("Should never happen - exited DHT22 task\r\n");
    vTaskDelete(NULL);
}