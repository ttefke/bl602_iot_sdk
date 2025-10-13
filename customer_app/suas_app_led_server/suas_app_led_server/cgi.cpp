/*
 * Implementation of CGI handlers and adoption of generated JSON to sensors use
 * case.
 */

extern "C" {
#include "cgi.h"

#include <bl_gpio.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "leds.h"
#include "lwip/apps/fs.h"
#include "lwip/apps/httpd.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/opt.h"
}

#include <etl/memory.h>

// Prototype
static const char *cgi_handler_led(int iIndex, int iNumParams, char *pcParam[],
                                   char *pcValue[]);

static const tCGI cgi_handlers[] = {{SET_LED_ENDPOINT, cgi_handler_led}};

// Custom deleter for cJSON data structures
struct cJSONDeleter {
  void operator()(cJSON *ptr) const {
    if (ptr) {
      cJSON_Delete(ptr);
    }
  }
};

static const char *cgi_handler_led(int iIndex, int iNumParams, char *pcParam[],
                                   char *pcValue[]) {
  printf("iIndex: %d, iNumParams: %d, pcParam: %s, pcValue: %s\r\n", iIndex,
         iNumParams, pcParam[0], pcValue[0]);
  if (iNumParams == 2) {
    enum selected_led led;
    uint8_t state;

    /* check if valid led selected */
    if (!strcmp(pcParam[0], "led")) {
      if (!strcmp(pcValue[0], "red")) {
        led = LED_RED;
      } else if (!strcmp(pcValue[0], "green")) {
        led = LED_GREEN;
      } else if (!strcmp(pcValue[0], "blue")) {
        led = LED_BLUE;
      } else {
        return ERROR_404_ENDPOINT;
      }

      printf("Selected LED: %d\r\n", led);
    } else {
      return ERROR_404_ENDPOINT;
    }

    /* get wanted led state */
    if (!strcmp(pcParam[1], "state")) {
      // Convert character to uint
      sscanf(pcValue[1], "%" PRIu8 "", &state);

      if (state == 0) {
        state = LED_OFF;
      } else {
        state = LED_ON;
      }

      // Change led state
      switch (led) {
        case LED_RED:
          led_state.state_led_red = state;
          break;
        case LED_GREEN:
          led_state.state_led_green = state;
          break;
        case LED_BLUE:
          led_state.state_led_blue = state;
          break;
        default:
          printf("Invalid LED selected");
      }

      printf("State: %d\r\n", state);

      // Apply LED state by using GPIO pins
      apply_led_state();
    } else {
      return ERROR_404_ENDPOINT;
    }
  } else {
    return ERROR_404_ENDPOINT;
  }
  return LED_ENDPOINT;
}

/* opening (creating) the in real-time created file (page) */
extern "C" int fs_open_custom(struct fs_file *file, const char *name) {
  // Pointer to response object
  etl::unique_ptr<char[]> response;

  // Match response endpoints
  if (!strcmp(name, LED_ENDPOINT)) {
    // Initialize response: up to 350 chars
    response.reset(new char[350]);
    memset(response.get(), 0, 350);
    /* Show links to control LEDs */
    if (led_state.state_led_red == LED_OFF) {
      strcat(response.get(), "<a href=\"" SET_LED_ENDPOINT
                             "?led=red&state=1\">Turn on red LED</a></br>");
    } else {
      strcat(response.get(), "<a href=\"" SET_LED_ENDPOINT
                             "?led=red&state=0\">Turn off red LED</a></br>");
    }

    if (led_state.state_led_green == LED_OFF) {
      strcat(response.get(), "<a href=\"" SET_LED_ENDPOINT
                             "?led=green&state=1\">Turn on green LED</a></br>");
    } else {
      strcat(response.get(),
             "<a href=\"" SET_LED_ENDPOINT
             "?led=green&state=0\">Turn off green LED</a></br>");
    }

    if (led_state.state_led_blue == LED_OFF) {
      strcat(response.get(), "<a href=\"" SET_LED_ENDPOINT
                             "?led=blue&state=1\">Turn on blue LED</a></br>");
    } else {
      strcat(response.get(), "<a href=\"" SET_LED_ENDPOINT
                             "?led=blue&state=0\">Turn off blue LED</a></br>");
    }
  } else if (!strcmp(name, GET_LED_STATUS_ENDPOINT)) {
    // Initialize cJSON response
    etl::unique_ptr<cJSON, cJSONDeleter> json_response(cJSON_CreateObject());
    cJSON_AddNumberToObject(json_response.get(), "led_red",
                            led_state.state_led_red);
    cJSON_AddNumberToObject(json_response.get(), "led_green",
                            led_state.state_led_green);
    cJSON_AddNumberToObject(json_response.get(), "led_blue",
                            led_state.state_led_blue);

    // Reponse is now the formatted JSON string
    response.reset(cJSON_PrintUnformatted(json_response.get()));
  } else {
    /* send zero if unknown URI */
    return 0;
  }

  // Get the size of the response
  int response_size = strlen(response.get());

  /* Reset the response data structure */
  memset(file, 0, sizeof(struct fs_file));

  /* copy response to file handler */
  /* NOTE: The smart pointer is a raw pointer now!
  We must delete it in the close function */
  file->pextension = response.release();

  /* Construct response */
  if (file->pextension != nullptr) {
    file->data = (const char *)file->pextension;
    file->len = response_size;
    file->index = file->len;
    /* allow persisting connections */
    file->flags = FS_FILE_FLAGS_HEADER_PERSISTENT;
  }

  /* return whether data was sent */
  return file->pextension != nullptr;
}

/* closing the custom file (nothing to do) */
extern "C" void fs_close_custom(struct fs_file *file) {
  if (file && file->pextension) {
    delete[] static_cast<char *>(file->pextension);
    file->pextension = nullptr;
  }
}

/* reading the custom file (nothing has to be done here, but function must be
 * defined */
extern "C" int fs_read_custom([[gnu::unused]] struct fs_file *file,
                              [[gnu::unused]] char *buffer,
                              [[gnu::unused]] int count) {
  return FS_READ_EOF;
}

/* initialization functions */
extern "C" void custom_files_init(void) {
  printf("Initializing module for generating JSON output\r\n");
  /* Nothing to do as of now, should be initialized automatically */
}

extern "C" void cgi_init(void) {
  printf("Initializing module for CGI\r\n");
  http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
}
