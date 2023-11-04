#include <stddef.h>
#include <stdint.h>
#include <string.h>

void mbedtls_platform_zeroize(void *buf, size_t len)
{
    uint8_t *tmp = (uint8_t*)buf;
    if ((len > 0) && (buf != NULL)) {
      for (uint32_t i = 0; i < len; i++) {
        *(tmp + i) = 0;
      }
    }
}

uint32_t mbedtls_get_unaligned_uint32(const void *p)
{
    uint32_t r;
    memcpy(&r, p, sizeof(r));
    return r;
}

void mbedtls_put_unaligned_uint32(void *p, uint32_t x)
{
    memcpy(p, &x, sizeof(x));
}

