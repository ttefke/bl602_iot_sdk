extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>

// TCP/IP stack
#include <lwip/tcpip.h>

// Own header
#include "wifi.h"
}

#include <etl/string.h>

/* main function, execution starts here */
extern "C" void bfl_main(void) {
  /* Define information containers for tasks */
  constexpr uint16_t WIFI_STACK_SIZE = 1024;
  constinit static StackType_t wifi_stack[WIFI_STACK_SIZE]{};
  constinit static StaticTask_t wifi_task{};

  /* Initialize hardware drivers */
  vInitializeBL602();

  /* Start tasks */
  /* WiFi task */
  printf("[SYSTEM] Starting WiFi task\r\n");
  xTaskCreateStatic(task_wifi, etl::string_view("wifi").data(), WIFI_STACK_SIZE,
                    nullptr, 16, wifi_stack, &wifi_task);

  /* Start TCP/IP stack */
  printf("[SYSTEM] Starting TCP/IP stack\r\n");
  tcpip_init(nullptr, nullptr);

  /* Start scheduler */
  printf("[SYSTEM] Starting scheduler\r\n");
  vTaskStartScheduler();
}
