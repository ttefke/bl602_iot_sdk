extern "C" {
#include <FreeRTOS.h>
#include <lwip/apps/httpd.h>
#include <stdio.h>
#include <task.h>

#include "cgi.h"
}

/* hello task */
extern "C" void task_httpd([[gnu::unused]] void *pvParameters) {
  /* wait for half a second */
  printf("Starting up httpd\r\n");

  httpd_init();

  /* initialize CGI */
  custom_files_init();
  cgi_init();
  while (1) {
    printf("httpd keepalive\r\n");
    vTaskDelay(pdMS_TO_TICKS(5 * 60 * 1000));
  }

  /* will never happen */
  vTaskDelete(nullptr);
}
