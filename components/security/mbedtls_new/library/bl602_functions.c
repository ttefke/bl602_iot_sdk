#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void mbedtls_platform_zeroize(void *buf, size_t len)
{
  /* 1. Overwrite key -> this can hardly be optimized away by the compiler
     2. Set corresponding memory to zero */ 
    uint8_t *tmp = (uint8_t*)buf;
    if ((len > 0) && (buf != NULL)) {
      for (uint32_t i = 0; i < len; i++) {
        *(tmp + i) = 0;
      }
    }
    memset(buf, 0, len);
}

void mbedtls_zeroize_and_free(void *buf, size_t len)
{
  if (buf != NULL) {
    mbedtls_platform_zeroize(buf, len);
  }
  free(buf);
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

void mbedtls_xor(unsigned char *r, const unsigned char *a, const unsigned char *b, size_t n)
{
    size_t i = 0;
    for (; (i + 4) <= n; i += 4) {
        uint32_t x = mbedtls_get_unaligned_uint32(a + i) ^ mbedtls_get_unaligned_uint32(b + i);
        mbedtls_put_unaligned_uint32(r + i, x);
    }
    for (; i < n; i++) {
        r[i] = a[i] ^ b[i];
    }
}


