/*
 * Implementation of CGI handlers and adoption of generated JSON to sensors use case.
 */

#include "lwip/apps/httpd.h"
#include "lwip/opt.h"

#include "lwip/apps/fs.h"
#include "lwip/def.h"
#include "lwip/mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bl_gpio.h>

#include "cJSON.h"

#include "cgi.h"
#include "leds.h"

static const char *
cgi_handler_led(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  printf("iIndex: %d, iNumParams: %d, pcParam: %s, pcValue: %s\r\n", iIndex, iNumParams, pcParam[0], pcValue[0]);
  if (iNumParams == 2)
  {
    enum selected_led led;
    uint8_t state;

    /* check if valid led selected */
    if (!strcmp(pcParam[0], "led"))
    {
      if (!strcmp(pcValue[0], "red"))
      {
        led = LED_RED;
      }
      else if (!strcmp(pcValue[0], "green"))
      {
        led = LED_GREEN;
      }
      else if (!strcmp(pcValue[0], "blue"))
      {
        led = LED_BLUE;
      }
      else
      {
        return ERROR_404_ENDPOINT;
      }

      printf("Selected LED: %d\r\n", led);
    }
    else
    {
      return ERROR_404_ENDPOINT;
    }

    /* get wanted led state */
    if (!strcmp(pcParam[1], "state"))
    {
      state = strtol(pcValue[1], NULL, 10);

      if (state == 0)
      {
        state = LED_OFF;
      } else {
        state = LED_ON;
      }

      switch (led)
      {
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
    }
    else
    {
      return ERROR_404_ENDPOINT;
    }
  }
  else
  {
    return ERROR_404_ENDPOINT;
  }
  return LED_ENDPOINT;
}

static const tCGI cgi_handlers[] = {
    {SET_LED_ENDPOINT,
     cgi_handler_led}};

/* opening (creating) the in real-time created file (page) */
int fs_open_custom(struct fs_file *file, const char *name)
{
  cJSON *json_response = NULL;
  char *response = NULL;
  if (!strcmp(name, LED_ENDPOINT))
  {
    /* Show links to control LEDs */
    response = (char *)calloc(350, sizeof(char *));
    if (led_state.state_led_red == LED_OFF)
    {
      strcat(response, "<a href=\"" SET_LED_ENDPOINT "?led=red&state=1\">Turn on red LED</a></br>");
    }
    else
    {
      strcat(response, "<a href=\"" SET_LED_ENDPOINT "?led=red&state=0\">Turn off red LED</a></br>");
    }

    if (led_state.state_led_green == LED_OFF)
    {
      strcat(response, "<a href=\"" SET_LED_ENDPOINT "?led=green&state=1\">Turn on green LED</a></br>");
    }
    else
    {
      strcat(response, "<a href=\"" SET_LED_ENDPOINT "?led=green&state=0\">Turn off green LED</a></br>");
    }

    if (led_state.state_led_blue == LED_OFF)
    {
      strcat(response, "<a href=\"" SET_LED_ENDPOINT "?led=blue&state=1\">Turn on blue LED</a></br>");
    }
    else
    {
      strcat(response, "<a href=\"" SET_LED_ENDPOINT "?led=blue&state=0\">Turn off blue LED</a></br>");
    }
  }
  else if (!strcmp(name, GET_LED_STATUS_ENDPOINT))
  {
    json_response = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_response, "led_red", led_state.state_led_red);
    cJSON_AddNumberToObject(json_response, "led_green", led_state.state_led_green);
    cJSON_AddNumberToObject(json_response, "led_blue", led_state.state_led_blue);
    response = cJSON_PrintUnformatted(json_response);
  }
  else
  {
    /* send null if unknown URI */
    return 0;
  }

  int response_size = strlen(response);

  /* allocate memory */
  memset(file, 0, sizeof(struct fs_file));
  file->pextension = mem_malloc(response_size);
  if (file->pextension != NULL)
  {
    /* copy json to file handler */
    memcpy(file->pextension, response, response_size);
    file->data = (const char *)file->pextension;
    file->len = response_size;
    file->index = file->len;
    /* allow persisting connections */
    file->flags = FS_FILE_FLAGS_HEADER_PERSISTENT;
  }

  /* free no longer needed memory */
  if (json_response != NULL)
  {
    // delete json structure
    cJSON_Delete(json_response);
  }
  if (response != NULL)
  {
    free(response);
  }

  /* return whether data was sent */
  if (file->pextension != NULL)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

/* closing the custom file (free the memory) */
void fs_close_custom(struct fs_file *file)
{
  if (file && file->pextension)
  {
    mem_free(file->pextension);
    file->pextension = NULL;
  }
}

/* reading the custom file (nothing has to be done here, but function must be defined */
int fs_read_custom(struct fs_file *file, char *buffer, int count)
{
  LWIP_UNUSED_ARG(file);
  LWIP_UNUSED_ARG(buffer);
  LWIP_UNUSED_ARG(count);
  return FS_READ_EOF;
}

/* initialization functions */
void custom_files_init(void)
{
  printf("Initializing module for generating JSON output\r\n");
  /* Nothing to do as of now, should be initialized automatically */
}

void cgi_init(void)
{
  printf("Initializing module for CGI\r\n");
  http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
}
