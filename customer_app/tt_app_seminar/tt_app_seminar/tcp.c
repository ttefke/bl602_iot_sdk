#include <FreeRTOS.h>
#include <task.h>
#include <lwip/tcpip.h>
#include <stdio.h>
#include <bl_sys.h>

#include "conf.h"

void task_tcp([[gnu::unused]] void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(NETWORK_CONNECTION_DELAY * 1000));

    tcpip_init(NULL, NULL);
    while (1) {
        /* wait forever */
        vTaskDelay(pdMS_TO_TICKS(60 * 1000));
    }
#ifdef REBOOT_ON_EXCEPTION
    bl_sys_reset_system();
#else
    printf("Should never happen - TCP task exited\r\n");
#endif
    vTaskDelete(NULL);
}