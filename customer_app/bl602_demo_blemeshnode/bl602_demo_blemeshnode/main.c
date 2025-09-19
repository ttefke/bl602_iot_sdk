#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <vfs.h>
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <event_device.h>
#include <cli.h>

#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/tcp.h>
#include <lwip/err.h>
#include <http_client.h>
#include <netutils/netutils.h>

#include <bl602_glb.h>
#include <bl602_hbn.h>
#include "bl602_adc.h"

#include <bl_sys.h>
#include <bl_uart.h>
#include <bl_chip.h>
#include <bl_wifi.h>
#include <hal_wifi.h>
#include <bl_sec.h>
#include <bl_cks.h>
#include <bl_irq.h>
#include <bl_timer.h>
#include <bl_dma.h>
#include <bl_gpio_cli.h>
#include <bl_wdt_cli.h>
#include <hal_uart.h>
#include <hal_sys.h>
#include <hal_gpio.h>
#include <hal_hbn.h>
#include <hal_boot2.h>
#include <hal_board.h>
#include <hal_button.h>
#include <looprt.h>
#include <loopset.h>
#include <sntp.h>
#include <bl_sys_time.h>
#include <bl_sys_ota.h>
#include <bl_romfs.h>
#include <fdt.h>

//#include <easyflash.h>
#include <bl60x_fw_api.h>
#include <wifi_mgmr_ext.h>
#include <utils_log.h>
#include <libfdt.h>
#include <blog.h>
#include "ble_lib_api.h"
#include "hal_pds.h"
#include "bl_rtc.h"
#include "utils_string.h"

#include "mesh_node.h"

#define TIME_5MS_IN_32768CYCLE  (164) // (5000/(1000000/32768))

bool pds_start = false;


extern uint8_t _heap_start;
extern uint8_t _heap_size; // @suppress("Type cannot be resolved")
extern uint8_t _heap_wifi_start;
extern uint8_t _heap_wifi_size; // @suppress("Type cannot be resolved")
static HeapRegion_t xHeapRegions[] =
{
        { &_heap_start,  (unsigned int) &_heap_size}, //set on runtime
        { &_heap_wifi_start, (unsigned int) &_heap_wifi_size },
        { NULL, 0 }, /* Terminates the array. */
        { NULL, 0 } /* Terminates the array. */
};

static void proc_hellow_entry([[gnu::unused]] void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(500));
    blemesh_node();
    while (1) {
        printf("%s: RISC-V rv32imafc\r\n", __func__);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    vTaskDelete(NULL);
}

#if defined(CFG_BLE_PDS)
static void cmd_start_pds(char *buf, int len, int argc, char **argv)
{
    if(argc != 2)
    {
        printf("Invalid params\r\n");
        return;
    }
    get_uint8_from_string(&argv[1], (uint8_t *)&pds_start);
    if (pds_start == 1)
    {
        hal_pds_init();
    }
}
#endif

static int get_dts_addr(const char *name, uint32_t *start, uint32_t *off)
{
    uint32_t addr = hal_board_get_factory_addr();
    const void *fdt = (const void *)addr;
    uint32_t offset;

    if (!name || !start || !off) {
        return -1;
    }

    offset = fdt_subnode_offset(fdt, 0, name);
    if (offset <= 0) {
       log_error("%s NULL.\r\n", name);
       return -1;
    }

    *start = (uint32_t)fdt;
    *off = offset;

    return 0;
}

static void __opt_feature_init(void)
{
#ifdef CONF_USER_ENABLE_VFS_ROMFS
    romfs_register();
#endif
}

static void event_cb_key_event(input_event_t *event, [[gnu::unused]] void *private_data)
{
    switch (event->code) {
        case KEY_1:
        {
            printf("[KEY_1] [EVT] INIT DONE %lld\r\n", aos_now_ms());
            printf("short press \r\n");
            mesh_node_open();
        }
        break;
        case KEY_2:
        {
            printf("[KEY_2] [EVT] INIT DONE %lld\r\n", aos_now_ms());
            printf("long press \r\n");
            mesh_node_reset();
        }
        break;
        case KEY_3:
        {
            printf("[KEY_3] [EVT] INIT DONE %lld\r\n", aos_now_ms());
            printf("longlong press \r\n");
        }
        break;
        default:
        {
            printf("[KEY] [EVT] Unknown code %u, %lld\r\n", event->code, aos_now_ms());
            /*nothing*/
        }
    }
}

#if defined(CONFIG_BT_TL)
extern void uart_init(uint8_t uartid);
#endif

static void aos_loop_proc([[gnu::unused]] void *pvParameters)
{
    int fd_console;
    uint32_t fdt = 0, offset = 0;
    static StackType_t proc_stack_looprt[512];
    static StaticTask_t proc_task_looprt;
    
    /*Init bloop stuff*/
    looprt_start(proc_stack_looprt, 512, &proc_task_looprt);
    loopset_led_hook_on_looprt();

//    easyflash_init();
    vfs_init();
    vfs_device_init();

    /* uart */
#if 1
    if (0 == get_dts_addr("uart", &fdt, &offset)) {
        vfs_uart_init(fdt, offset);
    }
#else
    vfs_uart_init_simple_mode(0, 7, 16, 2 * 1000 * 1000, "/dev/ttyS0");
#endif
    if (0 == get_dts_addr("gpio", &fdt, &offset)) {
        fdt_button_module_init((const void *)fdt, (int)offset);
    }

    __opt_feature_init();
    aos_loop_init();

    fd_console = aos_open("/dev/ttyS0", 0);
    if (fd_console >= 0) {
        printf("Init CLI with event Driven\r\n");
        aos_cli_init(0);
        aos_poll_read_fd(fd_console, aos_cli_event_cb_read_get(), (void*)0x12345678);
       
    }


    aos_register_event_filter(EV_KEY, event_cb_key_event, NULL);

    //tsen_adc_init();

    #if defined(CONFIG_BT_TL)
    //uart's pinmux has been configured in vfs_uart_init(load uart1's pin info from devicetree)
    uart_init(1);
    ble_controller_init(configMAX_PRIORITIES - 1);
    #endif

    aos_loop_run();

    puts("------------------------------------------\r\n");
    puts("+++++++++Critical Exit From Loop++++++++++\r\n");
    puts("******************************************\r\n");
    vTaskDelete(NULL);
}

static void _dump_boot_info(void)
{
    char chip_feature[40];
    const char *banner;

    puts("Booting BL602 Chip...\r\n");

    /*Display Banner*/
    if (0 == bl_chip_banner(&banner)) {
        puts(banner);
    }
    puts("\r\n");
    /*Chip Feature list*/
    puts("\r\n");
    puts("------------------------------------------------------------\r\n");
    puts("RISC-V Core Feature:");
    bl_chip_info(chip_feature);
    puts(chip_feature);
    puts("\r\n");

    puts("Build Version: ");
    puts(BL_SDK_VER); // @suppress("Symbol is not resolved")
    puts("\r\n");

    puts("Build Version: ");
    puts(BL_SDK_VER); // @suppress("Symbol is not resolved")
    puts("\r\n");

    puts("PHY   Version: ");
    puts(BL_SDK_PHY_VER); // @suppress("Symbol is not resolved")
    puts("\r\n");

    puts("RF    Version: ");
    puts(BL_SDK_RF_VER); // @suppress("Symbol is not resolved")
    puts("\r\n");

    puts("Build Date: ");
    puts(__DATE__);
    puts("\r\n");
    puts("Build Time: ");
    puts(__TIME__);
    puts("\r\n");
    puts("------------------------------------------------------------\r\n");

}

static void system_init(void)
{
    blog_init();
    bl_irq_init();
    bl_sec_init();
    bl_sec_test();
    bl_dma_init();
    bl_rtc_init();
    hal_boot2_init();

    /* board config is set after system is init*/
    hal_board_cfg(0);
}

static void system_thread_init()
{
    /*nothing here*/
}

void bfl_main()
{
    static StackType_t aos_loop_proc_stack[1024];
    static StaticTask_t aos_loop_proc_task;
    static StackType_t proc_hellow_stack[512];
    static StaticTask_t proc_hellow_task;

    bl_sys_early_init();

    /*Init UART In the first place*/
    bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);
    puts("Starting bl602 now....\r\n");

    bl_sys_init();

    _dump_boot_info();

    vPortDefineHeapRegions(xHeapRegions);
    printf("Heap %u@%p, %u@%p\r\n",
            (unsigned int)&_heap_size, &_heap_start,
            (unsigned int)&_heap_wifi_size, &_heap_wifi_start
    );

    system_init();
    system_thread_init();

    puts("[OS] Starting proc_hellow_entry task...\r\n");
    xTaskCreateStatic(proc_hellow_entry, (char*)"hellow", 512, NULL, 15, proc_hellow_stack, &proc_hellow_task);
    puts("[OS] Starting aos_loop_proc task...\r\n");
    xTaskCreateStatic(aos_loop_proc, (char*)"event_loop", 1024, NULL, 15, aos_loop_proc_stack, &aos_loop_proc_task);
    puts("[OS] Starting OS Scheduler...\r\n");
    vTaskStartScheduler();
}
