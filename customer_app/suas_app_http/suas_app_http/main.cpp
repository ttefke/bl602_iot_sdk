extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>

// IP stack
#include <lwip/tcpip.h>
}

#include <etl/string.h>

#include "wifi.h"

/* Main function, execution starts here */
extern "C" void bfl_main(void) {
  /* Define information containers for wifi task */
  constexpr uint16_t WIFI_STACK_SIZE = 1024;
  constinit static StackType_t wifi_stack[WIFI_STACK_SIZE]{};
  constinit static StaticTask_t wifi_task{};

  /* Initialize system */
  vInitializeBL602();

  /* Start tasks */
  printf("[SYSTEM] Starting WiFi task\r\n");
  xTaskCreateStatic(task_wifi, etl::string_view("wifi").data(), WIFI_STACK_SIZE,
                    nullptr, 16, wifi_stack, &wifi_task);

  /* Start TCP/IP stack */
  printf("[SYSTEM] Starting TCP/IP stack\r\n");
  tcpip_init(nullptr, nullptr);

  /* Start task scheduler */
  printf("[SYSTEM] Starting scheduler\r\n");
  vTaskStartScheduler();
}
