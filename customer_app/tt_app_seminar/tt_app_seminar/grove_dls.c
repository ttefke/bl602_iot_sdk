#include <FreeRTOS.h>
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <stdio.h>
#include <looprt.h>
#include <loopset_i2c.h>
#include <hal_i2c.h>
#include <suas_dls.h>
#include <bl_sys.h>

#include "conf.h"

volatile unsigned long lux;
void task_grove_dls([[gnu::unused]] void *pvParameters) {
    // yloop looping
    static StackType_t proc_stack_looprt[512];
    static StaticTask_t proc_task_looprt;
    looprt_start(proc_stack_looprt, 512, &proc_task_looprt);

    printf("Initializing Grove Digital Light Sensor\r\n");

    if (suas_init_grove_dls() != 0) {
#ifdef REBOOT_ON_EXCEPTION
        bl_sys_reset_system();
#else
        // retry once if initialization fails
        suas_init_grove_dls();
#endif
    }
    
    // wait until wifi is available
    vTaskDelay(pdMS_TO_TICKS(NETWORK_CONNECTION_DELAY * 1000));

    // endless loop to get data twice per second
    while (1) {
        lux = suas_read_visible_lux();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

#ifdef REBOOT_ON_EXCEPTION
    bl_sys_reset_system();
#else
    printf("Should never happen - exiting loop\r\n");
#endif
    vTaskDelete(NULL);
}
