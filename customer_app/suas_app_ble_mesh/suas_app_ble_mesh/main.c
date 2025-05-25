// Include libraries
// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>

// Hardware abstraction layers
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <event_device.h>
#include <looprt.h>
#include <loopset.h>
#include <vfs.h>

#include <bl_dma.h>
#include <bl_irq.h>
#include <bl_romfs.h>
#include <bl_rtc.h>
#include <bl_sec.h>
#include <bl_sys.h>
#include <bl_timer.h>
#include <bl_uart.h>

#include <hal_boot2.h>
#include <hal_board.h>
#include <hal_button.h>
#include <hal_sys.h>
#include <hal_uart.h>

#include <blog.h>
#include <fdt.h>
#include <libfdt.h>

// Standard library
#include <stdio.h>

// Own headers
#include "include/board.h"
#include "include/node.h"
#include "include/provisioning.h"
#include "include/client.h"

// Head redefinition
extern uint8_t _heap_start;
extern uint8_t _heap_size;
extern uint8_t _heap_wifi_start;
extern uint8_t _heap_wifi_size;

static HeapRegion_t xHeapRegions[] =
{
        { &_heap_start,  (unsigned int) &_heap_size},
        { &_heap_wifi_start, (unsigned int) &_heap_wifi_size },
        { NULL, 0 },
        { NULL, 0 }
};

// Keepalive function
static void keepalive(void *pvParameters)
{
    // Wait half a second for system initialization
    vTaskDelay(500);

    // Initialize mesh
    mesh_init();

    // Keep alive: endless loop
    while (1) {
        vTaskDelay(60 * 1000);
    }

    printf("[MAIN] Keepalive task ended - should never happen\r\n");
    vTaskDelete(NULL);
}

// Read device tree
int get_dts_addr(const char *name, uint32_t *start, uint32_t *off)
{
    uint32_t addr = hal_board_get_factory_addr();
    const void *fdt = (const void *)addr;
    uint32_t offset;

    if (!name || !start || !off) {
        return -1;
    }

    offset = fdt_subnode_offset(fdt, 0, name);
    if (offset <= 0) {
       printf("[MAIN] Can not read device tree\r\n");
       return -1;
    }

    *start = (uint32_t)fdt;
    *off = offset;

    return 0;
}

// Key event callback
static void event_cb_key_event(input_event_t *event, void *private_data)
{
    switch (event->code) {
        // Short key press (read a high value on GPIO pin 2 for 100-3000ms)
        // -> Provision if unprovisioned or send SET message to all nodes
        case KEY_1:
            // Board is provisioned: send SET message to all nodes
            if (bt_mesh_is_provisioned()) {
                mesh_send_set_message(false);
            } else {
                // Board is not provisioned: provision device
                mesh_node_provision();
            }
            break;
        // Long key press (read a high value on GPIO pin 2 for 6-10s)
        // -> send GET message to all nodes
        case KEY_2:
            if (bt_mesh_is_provisioned()) {
                mesh_send_get_message();
            } else {
                printf("[MAIN] Node is unprovisioned!\r\n");
            }
            break;
        // Very long key press (read a high value on GPIO pin 2 for > 15s)
        // -> send acknowledget SET message to all nodes
        case KEY_3:
            if (bt_mesh_is_provisioned()) {
                mesh_send_set_message(true);
            } else {
                printf("[MAIN] Node is unprovisioned!\r\n");
            }
            break;
        default:
            printf("[MAIN] Unsupported key event\r\n");
    }
}

// Event loop
static void aos_loop_proc(void *pvParameters)
{
    uint32_t fdt = 0, offset = 0;
    static StackType_t proc_stack_looprt[512];
    static StaticTask_t proc_task_looprt;
    
    // Init bloop stuff
    looprt_start(proc_stack_looprt, 512, &proc_task_looprt);
    loopset_led_hook_on_looprt();

    // Initialize virtual file system
    vfs_init();
    vfs_device_init();

    // UART
    if (0 == get_dts_addr("uart", &fdt, &offset)) {
        vfs_uart_init(fdt, offset);
    }

    // GPIO button
    if (0 == get_dts_addr("gpio", &fdt, &offset)) {
        fdt_button_module_init((const void *)fdt, (int)offset);
    }

#ifdef CONF_USER_ENABLE_VFS_ROMFS
    romfs_register();
#endif

    // Initialize loop
    aos_loop_init();

    // Register event filter
    aos_register_event_filter(EV_KEY, event_cb_key_event, NULL);

    // Start loop
    aos_loop_run();

    // Loop ended - should never happen
    printf("[MAIN] AOS event loop ended - should never happen\r\n");
    vTaskDelete(NULL);
}

// Main function
void bfl_main()
{
    static StackType_t aos_loop_proc_stack[1024];
    static StaticTask_t aos_loop_proc_task;
    static StackType_t keepalive_stack[512];
    static StaticTask_t keepalive_task;

    bl_sys_early_init();

    // Initialize UART
    bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);

    bl_sys_init();

    // Define heap
    vPortDefineHeapRegions(xHeapRegions);

    blog_init();
    bl_irq_init();
    bl_sec_init(); // we need this to generate random numbers
    bl_sec_test();
    bl_dma_init();
    bl_rtc_init();
    hal_boot2_init();

    // Board config is set after system is init
    hal_board_cfg(0);

    // Initialize GPIO for LED output
    board_init();

    // Create and start tasks
    xTaskCreateStatic(keepalive, (char*)"keepalive", 512, NULL, 15, keepalive_stack, &keepalive_task);
    xTaskCreateStatic(aos_loop_proc, (char*)"event_loop", 1024, NULL, 15, aos_loop_proc_stack, &aos_loop_proc_task);
    vTaskStartScheduler();
}
