extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// lwIP includes
#include <lwip/apps/fs.h>
#include <lwip/apps/httpd.h>
#include <lwip/mem.h>

// Standard library
#include <stdio.h>
#include <string.h>

// Own header
#include "httpd.h"
}

#include <etl/memory.h>

/* Opening (creating) the in real-time created file (page) */
extern "C" int fs_open_custom(struct fs_file *file, const char *name) {
  /* 1. Check if this is the supported custom endpoint
      You can use multiple comparisons to set up multiple endpoints
      (see LED server example) */
  if (strcmp(name, CUSTOM_ENDPOINT) != 0) {
    return 0;
  }

  /* 2. Create response */
  char response[] = "Hello world!";
  int response_size = strlen(response);

  /* 3. Allocate memory */
  memset(file, 0, sizeof(struct fs_file));

  // Copy response into response buffer
  auto data = etl::unique_ptr<char[]>(new char[response_size]);
  memset(data.get(), 0, response_size);
  memcpy(data.get(), response, response_size);

  // Note: This creates a raw pointer from the smart pointer
  // We must delete that one manually!
  file->pextension = data.release();

  if (file->pextension != nullptr) {
    file->data = (const char *)file->pextension;
    file->len = response_size;
    file->index = file->len;

    /* Allow persisting connections */
    file->flags = FS_FILE_FLAGS_HEADER_PERSISTENT;
  }

  /* 4. Return whether data was sent */
  return file->pextension != nullptr;
}

/* Close the custom file (free the memory) */
extern "C" void fs_close_custom(struct fs_file *file) {
  if (file && file->pextension) {
    delete[] static_cast<char *>(file->pextension);
    file->pextension = nullptr;
  }
}

/* Read the custom file (nothing has to be done here, but function must be
 * defined */
extern "C" int fs_read_custom([[gnu::unused]] struct fs_file *file,
                              [[gnu::unused]] char *buffer,
                              [[gnu::unused]] int count) {
  return FS_READ_EOF;
}

/* HTTP server task */
extern "C" void task_httpd([[gnu::unused]] void *pvParameters) {
  // Start HTTP server
  printf("[HTTPD] Starting server\r\n");

  httpd_init();

  // Keep this task (and consequently the server) running
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(60 * 1000));
  }

  /* Should never happen */
  printf("[HTTPD] Server stopped\r\n");
  vTaskDelete(NULL);
}