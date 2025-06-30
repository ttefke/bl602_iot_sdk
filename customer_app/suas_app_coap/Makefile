#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := suas_app_coap
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD
#CONFIG_TOOLPREFIX :=

-include ./proj_config.mk

# CoAP options
CFLAGS += -DWITH_LWIP
CFLAGS += -DWITH_COAP
CFLAGS += -DCOAP_CLIENT_SUPPORT
CFLAGS += -DCOAP_SERVER_SUPPORT

ifeq ($(origin BL60X_SDK_PATH), undefined)
$(error ****** Please set SDK paths ******)
endif

COMPONENTS_NETWORK := dns_server lwip lwip_dhcpd libcoap
COMPONENTS_BLSYS   := bltime blfdt blmtd bloop looprt loopset
COMPONENTS_VFS     := romfs

INCLUDE_COMPONENTS += freertos_riscv_ram bl602 bl602_std bl602_wifi bl602_wifidrv hal_drv vfs yloop utils cli blog blog_testc
INCLUDE_COMPONENTS += easyflash4 mbedtls
INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += $(COMPONENTS_VFS)
INCLUDE_COMPONENTS += $(PROJECT_NAME)
include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk
