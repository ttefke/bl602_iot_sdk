extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>
}

#include <etl/string.h>

/* Connection setup
    DLS     PineCone
    GND     GND
    VCC     3V3
    SCL     IO4
    SDA     IO3
*/

/* Main function, execution starts here */
extern "C" void bfl_main(void) {
  /* Define tasks */
  constexpr uint16_t GROVE_STACK_SIZE = 512;
  constinit static StackType_t grove_stack[GROVE_STACK_SIZE]{};
  constinit static StaticTask_t grove_task_dls{};

  constexpr uint16_t WIFI_STACK_SIZE = 1024;
  constinit static StackType_t wifi_stack[WIFI_STACK_SIZE]{};
  constinit static StaticTask_t wifi_task{};

  constexpr uint16_t TCP_STACK_SIZE = 1024;
  constinit static StackType_t tcp_stack[TCP_STACK_SIZE]{};
  constinit static StaticTask_t tcp_task{};

  constexpr uint16_t HTTP_STACK_SIZE = 768;
  static StackType_t http_stack[HTTP_STACK_SIZE]{};
  static StaticTask_t http_task{};

  /* Initialize system */
  vInitializeBL602();

  /* Start tasks */
  printf("[SYSTEM] Starting grove light sensor task\r\n");
  extern void task_grove_dls(void *pvParameters);
  xTaskCreateStatic(task_grove_dls, etl::string_view("grove dls").data(),
                    GROVE_STACK_SIZE, nullptr, 15, grove_stack,
                    &grove_task_dls);

  printf("[SYSTEM] Starting WiFi task\r\n");
  extern void task_wifi(void *pvParameters);
  xTaskCreateStatic(task_wifi, etl::string_view("wifi").data(), WIFI_STACK_SIZE,
                    nullptr, 17, wifi_stack, &wifi_task);

  printf("[SYSTEM] Starting TCP/IP stack\r\n");
  extern void task_tcp(void *pvParameters);
  xTaskCreateStatic(task_tcp, etl::string_view("tcp").data(), TCP_STACK_SIZE,
                    nullptr, 18, tcp_stack, &tcp_task);

  printf("[SYSTEM] Starting http task\r\n");
  extern void task_http(void *pvParameters);
  xTaskCreateStatic(task_http, etl::string_view("http").data(), HTTP_STACK_SIZE,
                    nullptr, 16, http_stack, &http_task);

  /* Start scheduler */
  printf("[SYSTEM] Starting scheduler\r\n");
  vTaskStartScheduler();
}
