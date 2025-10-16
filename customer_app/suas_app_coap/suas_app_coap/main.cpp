extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard library
#include <stdio.h>

// lwIP
#include <lwip/tcpip.h>
}
#include <etl/string.h>

// Wifi header
#include "include/wifi.h"

/* Main function, system starts here */
extern "C" void bfl_main(void) {
  /* Define containers for WiFi task */
  constexpr uint16_t WIFI_STACK_SIZE = 512;
  constinit static StackType_t wifi_stack[WIFI_STACK_SIZE]{};
  constinit static StaticTask_t wifi_task{};

  /* Initialize system */
  vInitializeBL602();

  /* Start tasks */
  /* WiFi task */
  printf("[%s] Starting WiFi task\r\n", __func__);
  xTaskCreateStatic(task_wifi, etl::string_view("wifi").data(), WIFI_STACK_SIZE,
                    nullptr, 16, wifi_stack, &wifi_task);

  /* Start TCP/IP stack */
  printf("[%s] Starting TCP/IP stack\r\n", __func__);
  tcpip_init(nullptr, nullptr);

  /* Start scheduler */
  printf("[%s] Starting scheduler\r\n", __func__);
  vTaskStartScheduler();
}
