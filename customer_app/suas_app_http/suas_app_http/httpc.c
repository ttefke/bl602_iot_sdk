// FreeRTOS headers
#include <FreeRTOS.h>
#include <task.h>

// Standard library includes
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Include HTTP client header
#include <http_client.h>

// Wifi and HTTP server headers
#include "httpd.h"
#include "wifi.h"

/* Data transfer finished handler */
static void cb_httpc_result(void *arg, httpc_result_t httpc_result,
    [[gnu::unused]] u32_t rx_content_len, [[gnu::unused]] u32_t srv_res, [[gnu::unused]] err_t err)
{
    httpc_state_t **req = (httpc_state_t **)arg;

    if (httpc_result == HTTPC_RESULT_OK)
    {
        printf("[HTTPC] Data transfer finished successfully\r\n");
    }
    else
    {
        printf("[HTTPC] Data transfer failed.\r\n");
    }

    *req = NULL;
}

/* Received headers handler*/
static err_t cb_httpc_headers_done_fn([[gnu::unused]] httpc_state_t *connection,
        [[gnu::unused]] void *arg, [[gnu::unused]] struct pbuf *hdr,
        [[gnu::unused]] u16_t hdr_len, [[gnu::unused]] u32_t content_len)
{
    /* headers received, could be processed here */
    return ERR_OK;
}

/* Received response handler */
static err_t cb_altcp_recv_fn([[gnu::unused]] void *arg,
    struct altcp_pcb *conn, struct pbuf *p, [[gnu::unused]] err_t err)
{
    /* Receive response
    1 packet only. Must be modified to receive multiple packets. 
    See lwip/src/include/lwip/pbuf.h*/

    // 1. Allocate memory for response
    char *response = (char*) calloc(p->tot_len, sizeof(char));

    // 2. Copy respoinse into own data structure
    strncpy(response, p->payload, p->tot_len);

    // 3. Print response
    printf("HTTPC] Received response: \r\n");
    printf("%s", response);
    printf("\r\n");

    // 4. Cleanup data structures
    altcp_recved(conn, p->tot_len);
    pbuf_free(p);
    free(response);
    response = NULL;

    // 5. Return result (no errors here)
    return ERR_OK;
}

/* Create an HTTP request */
void send_http_request(void)
{
    /* 1. Set up connection data structure */
    static httpc_connection_t connection;
    memset(&connection, 0, sizeof(connection));
    connection.use_proxy = 0;
    connection.result_fn = cb_httpc_result;
    connection.headers_done_fn = cb_httpc_headers_done_fn;
            
    static httpc_state_t *request;
    
    /* 2. Send request */
    printf("Creating HTTPS request\r\n");
    httpc_get_file_dns(
        "192.168.169.1", /* Hardcoded IP address of lwIP server */
        80, /* Port */
        CUSTOM_ENDPOINT, /* URI path */
        &connection, /* Connection data structure */
        cb_altcp_recv_fn, /* Response handler */
        &request, /* Request data */
        &request /* Request data */);
}

/* HTTP client task */
void task_http([[gnu::unused]] void *pvParameters)
{
    // Send one request every five seconds
    while (1)
    {
        send_http_request();
        vTaskDelay(5 * 1000);
    }
    
    // Stop task if loop exists - should not happen
    printf("[HTTPC] Deleting task - should not happen\r\n");
    vTaskDelete(NULL);
}