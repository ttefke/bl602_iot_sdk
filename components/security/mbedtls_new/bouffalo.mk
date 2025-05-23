# Component Makefile
#
## These include paths would be exported to project level
COMPONENT_ADD_INCLUDEDIRS +=

## not be exported to project level
COMPONENT_PRIV_INCLUDEDIRS := include/mbedtls include/psa

NAME := mbedtls_new

$(NAME)_MBINS_TYPE := kernel
$(NAME)_VERSION := 3.5.0
$(NAME)_SUMMARY := Mbed Transport Layer Security

## The component's src
COMPONENT_SRCS := library/bl602_functions.c
COMPONENT_SRCS += library/aes.c
COMPONENT_SRCS += library/aesce.c
COMPONENT_SRCS += library/aesni.c
COMPONENT_SRCS += library/aria.c
COMPONENT_SRCS += library/asn1write.c
COMPONENT_SRCS += library/asn1parse.c
COMPONENT_SRCS += library/base64.c
COMPONENT_SRCS += library/bignum.c
COMPONENT_SRCS += library/bignum_core.c
COMPONENT_SRCS += library/bignum_mod.c
COMPONENT_SRCS += library/bignum_mod_raw.c
COMPONENT_SRCS += library/camellia.c
COMPONENT_SRCS += library/ccm.c
COMPONENT_SRCS += library/chacha20.c
COMPONENT_SRCS += library/chachapoly.c
COMPONENT_SRCS += library/cipher.c
COMPONENT_SRCS += library/cipher_wrap.c
COMPONENT_SRCS += library/cmac.c
COMPONENT_SRCS += library/ctr_drbg.c
COMPONENT_SRCS += library/debug.c
COMPONENT_SRCS += library/des.c
COMPONENT_SRCS += library/dhm.c
COMPONENT_SRCS += library/ecdh.c
COMPONENT_SRCS += library/ecdsa.c
COMPONENT_SRCS += library/ecjpake.c
COMPONENT_SRCS += library/ecp.c
COMPONENT_SRCS += library/ecp_curves.c
COMPONENT_SRCS += library/ecp_curves_new.c
COMPONENT_SRCS += library/entropy.c
#COMPONENT_SRCS += library/entropy_poll.c
COMPONENT_SRCS += library/error.c
COMPONENT_SRCS += library/gcm.c
COMPONENT_SRCS += library/hkdf.c
COMPONENT_SRCS += library/hmac_drbg.c
COMPONENT_SRCS += library/lmots.c
COMPONENT_SRCS += library/lms.c
COMPONENT_SRCS += library/md.c
COMPONENT_SRCS += library/md5.c
COMPONENT_SRCS += library/memory_buffer_alloc.c
COMPONENT_SRCS += library/mps_reader.c
COMPONENT_SRCS += library/mps_trace.c
#COMPONENT_SRCS += library/net_sockets.c
COMPONENT_SRCS += library/nist_kw.c
COMPONENT_SRCS += library/oid.c
COMPONENT_SRCS += library/padlock.c
COMPONENT_SRCS += library/pem.c
COMPONENT_SRCS += library/pk.c
COMPONENT_SRCS += library/pkcs5.c
COMPONENT_SRCS += library/pkcs7.c
COMPONENT_SRCS += library/pkcs12.c
COMPONENT_SRCS += library/pkparse.c
COMPONENT_SRCS += library/pk_wrap.c
COMPONENT_SRCS += library/pkwrite.c
COMPONENT_SRCS += library/platform.c
#COMPONENT_SRCS += library/platform_util.c
COMPONENT_SRCS += library/poly1305.c
COMPONENT_SRCS += library/psa_crypto.c
COMPONENT_SRCS += library/psa_crypto_aead.c
COMPONENT_SRCS += library/psa_crypto_cipher.c
COMPONENT_SRCS += library/psa_crypto_client.c
COMPONENT_SRCS += library/psa_crypto_driver_wrappers_no_static.c
COMPONENT_SRCS += library/psa_crypto_ecp.c
COMPONENT_SRCS += library/psa_crypto_ffdh.c
COMPONENT_SRCS += library/psa_crypto_hash.c
COMPONENT_SRCS += library/psa_crypto_mac.c
COMPONENT_SRCS += library/psa_crypto_pake.c
COMPONENT_SRCS += library/psa_crypto_rsa.c
COMPONENT_SRCS += library/psa_crypto_se.c
COMPONENT_SRCS += library/psa_crypto_slot_management.c
COMPONENT_SRCS += library/psa_crypto_storage.c
COMPONENT_SRCS += library/psa_its_file.c
COMPONENT_SRCS += library/psa_util.c
COMPONENT_SRCS += library/ripemd160.c
COMPONENT_SRCS += library/rsa.c
COMPONENT_SRCS += library/rsa_alt_helpers.c
COMPONENT_SRCS += library/sha1.c
COMPONENT_SRCS += library/sha3.c
COMPONENT_SRCS += library/sha256.c
COMPONENT_SRCS += library/sha512.c
COMPONENT_SRCS += library/ssl_cache.c
COMPONENT_SRCS += library/ssl_ciphersuites.c
COMPONENT_SRCS += library/ssl_client.c
COMPONENT_SRCS += library/ssl_cookie.c
COMPONENT_SRCS += library/ssl_debug_helpers_generated.c
COMPONENT_SRCS += library/ssl_msg.c
COMPONENT_SRCS += library/ssl_tls.c
COMPONENT_SRCS += library/ssl_tls12_client.c
COMPONENT_SRCS += library/ssl_tls12_server.c
COMPONENT_SRCS += library/ssl_tls13_client.c
COMPONENT_SRCS += library/ssl_tls13_generic.c
COMPONENT_SRCS += library/ssl_tls13_keys.c
COMPONENT_SRCS += library/ssl_tls13_server.c
COMPONENT_SRCS += library/threading.c
#COMPONENT_SRCS += library/timing.c
COMPONENT_SRCS += library/version.c
COMPONENT_SRCS += library/version_features.c
COMPONENT_SRCS += library/x509.c
COMPONENT_SRCS += library/x509_create.c
COMPONENT_SRCS += library/x509_crl.c
COMPONENT_SRCS += library/x509_crt.c
COMPONENT_SRCS += library/x509_csr.c
COMPONENT_SRCS += library/x509write.c
COMPONENT_SRCS += library/x509write_crt.c
COMPONENT_SRCS += library/x509write_csr.c

COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))

COMPONENT_SRCDIRS := library
