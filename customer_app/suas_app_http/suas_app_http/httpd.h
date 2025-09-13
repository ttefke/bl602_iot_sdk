#ifndef __HTTPD_H
#define __HTTPD_H
/* Show errors if required dependencies are not included by according defines */

#if !LWIP_HTTPD_CUSTOM_FILES
#error This app needs LWIP_HTTPD_CUSTOM_FILES
#endif

#if !LWIP_HTTPD_DYNAMIC_HEADERS
#error This app needs LWIP_HTTPD_DYNAMIC_HEADERS
#endif

/* Our endpoint -> file ending is used to determine the MIME type */
#define CUSTOM_ENDPOINT "/endpoint.html"

#endif