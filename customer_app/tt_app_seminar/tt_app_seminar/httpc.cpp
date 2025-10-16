extern "C" {
#include <FreeRTOS.h>
#include <bl_sys.h>
#include <cJSON.h>
#include <http_client.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <wifi_mgmr_ext.h>
}

#include <etl/memory.h>
#include <etl/string.h>

#include "conf.h"

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

// Custom deleter for cJSON data structures
struct cJSONDeleter {
  void operator()(cJSON *ptr) const {
    if (ptr) {
      cJSON_Delete(ptr);
    }
  }
};

/* helper function: callback function for the result of the HTTP request */
static void cb_httpc_result([[gnu::unused]] void *arg,
                            httpc_result_t httpc_result,
                            [[gnu::unused]] u32_t rx_content_len,
                            [[gnu::unused]] u32_t srv_res,
                            [[gnu::unused]] err_t err) {
  /* evaluate data transfer result */
  if (httpc_result != HTTPC_RESULT_OK) {
    printf("[HTTPC] Data transfer failed.\r\n");
  }
}

/* helper function: HTTP headers are received*/
static err_t cb_httpc_headers_done_fn([[gnu::unused]] httpc_state_t *connection,
                                      [[gnu::unused]] void *arg,
                                      struct pbuf *hdr,
                                      [[gnu::unused]] u16_t hdr_len,
                                      [[gnu::unused]] u32_t content_len) {
  /* headers are ignored */
  auto p = etl::unique_ptr<pbuf, pbufDeleter>(hdr);
  return ERR_OK;
}

/* helper function: received the data from the queried URL*/
static err_t cb_altcp_recv_fn([[gnu::unused]] void *arg, struct altcp_pcb *conn,
                              struct pbuf *p, [[gnu::unused]] err_t err) {
  // Make p a smart pointer with custom deleter to ease memory management
  // We can do this because we know p is allocated on the heap
  auto packet = etl::unique_ptr<pbuf, pbufDeleter>(p);

  /* We receive nothing -> just confirm receipt */
  altcp_recved(conn, packet.get()->tot_len);

  return ERR_OK;
}

/* Send HTTP request */
void send_http_request(unsigned char *data) {
  /* create connection data structure*/
  constinit static httpc_connection_t connection{};

  /* configure connection */
  connection.use_proxy = 0;
  connection.req_type = REQ_TYPE_POST;
  connection.data = data;
  connection.content_type = CONTENT_TYPE_JSON;
  connection.result_fn = cb_httpc_result;
  connection.headers_done_fn = cb_httpc_headers_done_fn;

  /* create and send request */
  static httpc_state_t *request;
  httpc_get_file_dns(GATEWAY_IP_ADDRESS, GATEWAY_PORT, GATEWAY_ROUTE_LUX,
                     &connection, cb_altcp_recv_fn, &request, &request);
}

/* http task */
extern "C" void task_http([[gnu::unused]] void *pvParameters) {
  /* variables to identify measurements */
  auto message_id = 1;
  extern volatile unsigned long lux;

  /* delay sending data until WiFi is available */
  vTaskDelay(pdMS_TO_TICKS((NETWORK_CONNECTION_DELAY + 3) * 1000));

  while (1) {
    /* create data structure to store data to be sent */
    auto request_data =
        etl::unique_ptr<cJSON, cJSONDeleter>(cJSON_CreateObject());
    cJSON_AddNumberToObject(request_data.get(),
                            etl::string_view("device_id").data(), DEVICE_ID);
    cJSON_AddNumberToObject(request_data.get(),
                            etl::string_view("message_id").data(), message_id);
    cJSON_AddNumberToObject(request_data.get(), etl::string_view("lux").data(),
                            lux);

    /* send data */
    send_http_request(reinterpret_cast<unsigned char *>(
        cJSON_PrintUnformatted(request_data.get())));

    /* increase message id and wait for one second */
    message_id++;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  /* while loop should never exit */
#ifdef REBOOT_ON_EXCEPTION
  bl_sys_reset_system();
#else
  printf("Deleting task - should not happen\r\n");
#endif
  vTaskDelete(nullptr);
}