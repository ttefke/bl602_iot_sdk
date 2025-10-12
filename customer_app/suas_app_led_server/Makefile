#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := suas_app_led_server
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD
#CONFIG_TOOLPREFIX :=

-include ./proj_config.mk

NETWORK_FLAGS := -DLWIP_HTTPD_CUSTOM_FILES=1 \
	-DLWIP_HTTPD_DYNAMIC_HEADERS=1 \
	-DLWIP_HTTPD_CGI=1

CFLAGS += $(NETWORK_FLAGS)
CPPFLAGS += $(NETWORK_FLAGS)
CXXFLAGS += $(NETWORK_FLAGS)

ifeq ($(origin BL60X_SDK_PATH), undefined)
$(error ****** Please set SDK paths ******)
endif

COMPONENTS_NETWORK := sntp dns_server
COMPONENTS_BLSYS   := bltime blfdt blmtd blota bloop loopadc looprt loopset
COMPONENTS_VFS     := romfs
COMPONENTS_UTILS   := cjson etl

INCLUDE_COMPONENTS += freertos bl602 bl602_std bl602_wifi bl602_wifidrv hal_drv lwip lwip_dhcpd vfs yloop utils cli httpc netutils blog blog_testc
INCLUDE_COMPONENTS += easyflash4 mbedtls-bl602
INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += $(COMPONENTS_VFS)
INCLUDE_COMPONENTS += $(COMPONENTS_UTILS)
INCLUDE_COMPONENTS += $(PROJECT_NAME)
include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk
