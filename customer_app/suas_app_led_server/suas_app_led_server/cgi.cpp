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

#include <etl/array.h>
#include <etl/memory.h>
#include <etl/string.h>

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

static const char *cgi_handler_led([[gnu::unused]] int iIndex, int iNumParams,
                                   char *pcParam[], char *pcValue[]) {
  if (iNumParams == 2) {
    /* Use ETL data structures */
    auto keys = etl::array{
        etl::string_view(pcParam[0]),
        etl::string_view(pcParam[1]),
    };

    auto values = etl::array{
        etl::string_view(pcValue[0]),
        etl::string_view(pcValue[1]),
    };

    /* Get selected LED */
    enum selected_led led;

    if (etl::string_view("led") == keys[0]) {
      if (etl::string_view("red") == values[0]) {
        led = LED_RED;
      } else if (etl::string_view("green") == values[0]) {
        led = LED_GREEN;
      } else if (etl::string_view("blue") == values[0]) {
        led = LED_BLUE;
      } else {
        return ERROR_404_ENDPOINT;
      }
      printf("Selected LED: %s\r\n", values[0].data());
    } else {
      return ERROR_404_ENDPOINT;
    }

    /* Get desired LED state */
    if (etl::string_view("state") == keys[1]) {
      /* Obtain LED state*/
      uint8_t state;
      sscanf(values[1].data(), "%" PRIu8 "", &state);
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

      /* Apply LED state -> done with GPIO */
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
  //  -> We need a pointer here, this outlives this function
  auto response = etl::unique_ptr<char[]>();

  // Requested route
  auto route = etl::string_view(name);

  // Match response endpoints
  // LED settings endpoint
  if (route == LED_ENDPOINT) {
    /* Create response buffer structure */
    auto responseBuffer = etl::string<350>();

    /* Show links to control LEDs */
    if (led_state.state_led_red == LED_OFF) {
      responseBuffer += "<a href=\"" SET_LED_ENDPOINT
                        "?led=red&state=1\">Turn on red LED</a></br>";
    } else {
      responseBuffer += "<a href=\"" SET_LED_ENDPOINT
                        "?led=red&state=0\">Turn off red LED</a></br>";
    }

    if (led_state.state_led_green == LED_OFF) {
      responseBuffer += "<a href=\"" SET_LED_ENDPOINT
                        "?led=green&state=1\">Turn on green LED</a></br>";
    } else {
      responseBuffer += "<a href=\"" SET_LED_ENDPOINT
                        "?led=green&state=0\">Turn off green LED</a></br>";
    }

    if (led_state.state_led_blue == LED_OFF) {
      responseBuffer += "<a href=\"" SET_LED_ENDPOINT
                        "?led=blue&state=1\">Turn on blue LED</a></br>";
    } else {
      responseBuffer += "<a href=\"" SET_LED_ENDPOINT
                        "?led=blue&state=0\">Turn off blue LED</a></br>";
    }

    // Copy response string from buffer into pointer
    response.reset(new char[350]);
    memset(response.get(), 0, 350);
    memcpy(response.get(), responseBuffer.data(), responseBuffer.length());
  }  // LED status endpoint
  else if (route == GET_LED_STATUS_ENDPOINT) {
    // Initialize cJSON response
    auto json_response =
        etl::unique_ptr<cJSON, cJSONDeleter>(cJSON_CreateObject());
    cJSON_AddNumberToObject(json_response.get(),
                            etl::string_view("led_red").data(),
                            led_state.state_led_red);
    cJSON_AddNumberToObject(json_response.get(),
                            etl::string_view("led_green").data(),
                            led_state.state_led_green);
    cJSON_AddNumberToObject(json_response.get(),
                            etl::string_view("led_blue").data(),
                            led_state.state_led_blue);

    // Copy response from formatted JSON string to pointer
    response.reset(cJSON_PrintUnformatted(json_response.get()));
  } else {
    /* send zero if unknown URI */
    return 0;
  }

  // Get the size of the response
  auto response_size = etl::string_view(response.get()).length();

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
