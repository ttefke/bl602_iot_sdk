#ifndef MBEDTLS_602FUNCTIONS_H
#define MBEDTLS_602FUNCTIONS_H

#include <stddef.h>
#include <stdint.h>

void mbedtls_platform_zeroize(void *buf, size_t len);
void mbedtls_zeroize_and_free(void *buf, size_t len);
uint32_t mbedtls_get_unaligned_uint32(const void *p);
void mbedtls_put_unaligned_uint32(void *p, uint32_t x);
void mbedtls_xor(unsigned char *r, const unsigned char *a, const unsigned char *b, size_t n);

#endif
