#include <FreeRTOS.h>
#include <task.h>
#include <lwip/tcpip.h>
#include <stdio.h>
#include <bl_sys.h>

#include "conf.h"

void task_tcp([[gnu::unused]] void *pvParameters) {
    vTaskDelay(NETWORK_CONNECTION_DELAY * 1000 / portTICK_PERIOD_MS);

    tcpip_init(NULL, NULL);
    while (1) {
        /* wait forever */
        vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
    }
#ifdef REBOOT_ON_EXCEPTION
    bl_sys_reset_system();
#else
    printf("Should never happen - TCP task exited\r\n");
#endif
    vTaskDelete(NULL);
}