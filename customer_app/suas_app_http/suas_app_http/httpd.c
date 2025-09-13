// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// lwIP includes
#include <lwip/apps/httpd.h>
#include <lwip/apps/fs.h>
#include <lwip/mem.h>

// Standard library
#include <stdio.h>
#include <string.h>

// Own header
#include "httpd.h"

/* Opening (creating) the in real-time created file (page) */
int fs_open_custom(struct fs_file *file, const char *name)
{
    /* 1. Check if this is the supported custom endpoint 
        You can use multiple comparisons to set up multiple endpoints
        (see LED server example) */
    if (strcmp(name, CUSTOM_ENDPOINT) != 0)
    {
        return 0;
    }

    /* 2. Create response */
    char response[] = "Hello world!";
    int response_size = strlen(response);

    /* 3. Allocate memory */
    memset(file, 0, sizeof(struct fs_file));
    file->pextension = mem_malloc(response_size);
    if (file->pextension != NULL)
    {
        /* Copy data to output data structure */
        memcpy(file->pextension, response, response_size);
        file->data = (const char *)file->pextension;
        file->len = response_size;
        file->index = file->len;

        /* Allow persisting connections */
        file->flags = FS_FILE_FLAGS_HEADER_PERSISTENT;
    }

    /* 4. Return whether data was sent */
    if (file->pextension != NULL)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/* Close the custom file (free the memory) */
void fs_close_custom(struct fs_file *file)
{
  if (file && file->pextension)
  {
    mem_free(file->pextension);
    file->pextension = NULL;
  }
}

/* Read the custom file (nothing has to be done here, but function must be defined */
int fs_read_custom(struct fs_file *file, char *buffer, int count)
{
  LWIP_UNUSED_ARG(file);
  LWIP_UNUSED_ARG(buffer);
  LWIP_UNUSED_ARG(count);
  return FS_READ_EOF;
}

/* HTTP server task */
void task_httpd([[gnu::unused]] void *pvParameters)
{
  // Start HTTP server
  printf("[HTTPD] Starting server\r\n");
 
  httpd_init();

  // Keep this task (and consequently the server) running
  while(1) {
    vTaskDelay(60*1000);
  }
  
  /* Should never happen */
  printf("[HTTPD] Server stopped\r\n");
  vTaskDelete(NULL);
}