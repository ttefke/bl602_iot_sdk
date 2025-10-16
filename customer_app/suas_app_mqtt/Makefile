#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := suas_app_mqtt
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD
#CONFIG_TOOLPREFIX :=

-include ./proj_config.mk

PROJECT_FLAGS += -DLWIP_ALTCP_TLS_MBEDTLS
PROJECT_FLAGS += -DWITH_SNIFFER

CFLAGS += $(PROJECT_FLAGS)
CPPFLAGS += $(PROJECT_FLAGS)
CXXFLAGS += $(PROJECT_FLAGS)

ifeq ($(origin BL60X_SDK_PATH), undefined)
$(error ****** Please set SDK paths ******)
endif
COMPONENTS_NETWORK := dns_server lwip_dhcpd lwip mbedtls-bl602 sniffer
COMPONENTS_BLSYS   := bltime blfdt bloop looprt loopset vfs yloop utils cli blog
COMPONENTS_HAL	   := bl602 bl602_std bl602_wifi bl602_wifidrv hal_drv

INCLUDE_COMPONENTS += freertos etl
INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += $(COMPONENTS_HAL)
INCLUDE_COMPONENTS += $(PROJECT_NAME)
include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk
