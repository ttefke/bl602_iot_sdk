#include <FreeRTOS.h>
#include <task.h>
#include <lwip/tcpip.h>
#include <stdio.h>

#include "conf.h"

void task_tcp(void *pvParameters) {
    vTaskDelay(NETWORK_CONNECTION_DELAY * 1000 / portTICK_PERIOD_MS);

    tcpip_init(NULL, NULL);
    while (1) {
        /* wait forever */
        vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
    }

    printf("Should never happen - TCP task exited\r\n");
    vTaskDelete(NULL);
}