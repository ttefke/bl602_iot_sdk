extern "C" {  // FreeRTOS headers
#include <FreeRTOS.h>
#include <task.h>

// Standard library includes
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Include HTTP client header
#include <http_client.h>

// Wifi and HTTP server headers
#include "httpd.h"
#include "wifi.h"
}

#include <etl/memory.h>
#include <etl/string.h>

/* Data transfer finished handler */
static void cb_httpc_result([[gnu::unused]] void *arg,
                            httpc_result_t httpc_result,
                            [[gnu::unused]] u32_t rx_content_len,
                            [[gnu::unused]] u32_t srv_res,
                            [[gnu::unused]] err_t err) {
  if (httpc_result == HTTPC_RESULT_OK) {
    printf("[HTTPC] Data transfer finished successfully\r\n");
  } else {
    printf("[HTTPC] Data transfer failed.\r\n");
  }
}

/* Custom deleter for the pbuf
This is required because the pbuf we receive in cb_altcp_recv_fn is allocated on
the heap and we must use pbuf_free to delete it; hence it can not be
automatically deleted. */
struct pbufDeleter {
  void operator()(struct pbuf *p) const {
    if (p) {
      pbuf_free(p);
    }
  }
};

/* Received headers handler*/
static err_t cb_httpc_headers_done_fn([[gnu::unused]] httpc_state_t *connection,
                                      [[gnu::unused]] void *arg,
                                      [[gnu::unused]] struct pbuf *hdr,
                                      [[gnu::unused]] u16_t hdr_len,
                                      [[gnu::unused]] u32_t content_len) {
  /* headers are ignored */
  auto p = etl::unique_ptr<pbuf, pbufDeleter>(hdr);
  return ERR_OK;
}

/* Received response handler */
static err_t cb_altcp_recv_fn([[gnu::unused]] void *arg, struct altcp_pcb *conn,
                              struct pbuf *p, [[gnu::unused]] err_t err) {
  /* Receive response
  1 packet only. Must be modified to receive multiple packets.
  See lwip/src/include/lwip/pbuf.h*/

  // Make p a smart pointer with custom deleter to ease memory management
  // We can do this because we know p is allocated on the heap
  auto packet = etl::unique_ptr<pbuf, pbufDeleter>(p);

  // 1. Create buffer for response
  auto response = etl::string_view(
      static_cast<const char *>(packet.get()->payload), packet.get()->tot_len);

  // 2. Print response
  printf("HTTPC] Received response: \r\n");
  printf("%s", response.data());
  printf("\r\n");

  // 3. Confirm packet receipt
  altcp_recved(conn, packet.get()->tot_len);

  // 4. Return result (no errors here)
  return ERR_OK;
}

/* Create an HTTP request */
void send_http_request(void) {
  /* 1. Set up connection data structure */
  constinit static httpc_connection_t connection{};
  connection.use_proxy = 0;
  connection.result_fn = cb_httpc_result;
  connection.headers_done_fn = cb_httpc_headers_done_fn;

  static httpc_state_t *request;

  /* 2. Send request */
  printf("Creating HTTP request\r\n");
  httpc_get_file_dns("192.168.169.1",  /* Hardcoded IP address of lwIP server */
                     80,               /* Port */
                     CUSTOM_ENDPOINT,  /* URI path */
                     &connection,      /* Connection data structure */
                     cb_altcp_recv_fn, /* Response handler */
                     &request,         /* Request data */
                     &request /* Request data */);
}

/* HTTP client task */
void task_http([[gnu::unused]] void *pvParameters) {
  // Send one request every five seconds
  while (1) {
    send_http_request();
    vTaskDelay(pdMS_TO_TICKS(5 * 1000));
  }

  // Stop task if loop exists - should not happen
  printf("[HTTPC] Deleting task - should not happen\r\n");
  vTaskDelete(nullptr);
}