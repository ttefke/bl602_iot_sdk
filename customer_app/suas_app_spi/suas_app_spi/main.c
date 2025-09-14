// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Input/Output
#include <stdio.h>

// UART library
#include <hal_uart.h>

// Hardware realtime loop
#include <looprt.h>
#include <loopset.h>

// Virtual file system
#include <event_device.h>
#include <libfdt.h>
#include <vfs.h>

// AOS loop
#include <aos/yloop.h>

// SPI implementation
#include <hal_spi.h>
#include "spi_adapter.h"
#include "spi.h"

// BL602
#include <hal_board.h>

// GPIO
#include <bl602_glb.h>
#include <bl_gpio.h>

// Logging
#include <blog.h>

// Helper function to get addresses from device tree
static int get_dts_addr(const char *name, uint32_t *start, uint32_t *off) {
  uint32_t addr = hal_board_get_factory_addr();
  const void *fdt = (const void*)addr;
  uint32_t offset;

  if (!name || !start  || !off) {
    return -1;
  }

  offset = fdt_subnode_offset(fdt, 0, name);
  if (offset <= 0) {
    log_error("%s is null\r\n", name);
    return -1;
  }

  *start = (uint32_t) fdt;
  *off = offset;

  return 0;
}

// Register AOS device tree
static void aos_loop_proc([[gnu::unused]] void *pvParameters) {
  uint32_t fdt = 0, offset = 0;
  static StackType_t proc_stack_looprt[512];
  static StaticTask_t proc_task_looprt;

  /*Init bloop stuff*/
  looprt_start(proc_stack_looprt, 512, &proc_task_looprt);
  loopset_led_hook_on_looprt();

  // VFS init
  vfs_init();
  vfs_device_init();

  // UART
  if (get_dts_addr("uart", &fdt, &offset) == 0) {
    vfs_uart_init(fdt, offset);
  }

  // SPI
  if (get_dts_addr("spi", &fdt, &offset) == 0) {
    vfs_spi_fdt_init(fdt, offset);

    // Reconfigure CS pin
    GLB_GPIO_Type cs_pin[1];
    cs_pin[0] = 2; // from device tree specification
    GLB_GPIO_Func_Init(
      GPIO_FUN_SWGPIO, // Configure as GPIO
      cs_pin, // Pin to be configure
      sizeof(cs_pin) / sizeof(cs_pin[0]) // PIN size
    );

    // Configure real CS pin
    bl_gpio_enable_output(CS_PIN, 0, 0);
    bl_gpio_output_set(CS_PIN, CS_DISABLE);
    printf("Initialized spi\r\n");
  }

  // ROMFS
  #ifdef CONF_USER_ENABLE_VFS_ROMFS
    romfs_register();
  #endif 

  // Start loop
  aos_loop_init();
  aos_loop_run();

  printf("Critical exit from AOS loop procedure\r\n");
  vTaskDelete(NULL);
}

#define PROC_STACK_SIZE 1024
#define SPI_STACK_SIZE 512

void bfl_main(void)
{
  static StackType_t aos_loop_proc_stack[PROC_STACK_SIZE];
  static StaticTask_t aos_loop_proc_task;
  static StackType_t spi_stack[SPI_STACK_SIZE];
  static StaticTask_t spi_task;

  /* Initialize system */
  vInitializeBL602();

  /* Create the tasks */
  xTaskCreateStatic(
    aos_loop_proc,
    (char*)"event loop",
    PROC_STACK_SIZE,
    NULL,
    15,
    aos_loop_proc_stack,
    &aos_loop_proc_task   
  );
  
  xTaskCreateStatic(
    spi_proc,
    (char*)"spi",
    SPI_STACK_SIZE,
    NULL,
    12,
    spi_stack,
    &spi_task
  );

  /* Start tasks */
  vTaskStartScheduler();
}
