PROJECT_NAME := suas_app_ble
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD

-include ./proj_config.mk

ifeq ($(origin BL60X_SDK_PATH), undefined)
$(error ****** Please SET BL60X_SDK_PATH ******)
endif

COMPONENTS_NETWORK := dns_server lwip lwip_dhcpd mbedtls-bl602
COMPONENTS_BLSYS   := bltime blfdt blmtd blota bloop loopadc looprt loopset
COMPONENTS_BLE     := blecontroller blestack

INCLUDE_COMPONENTS += freertos bl602 bl602_std bl602_wifi bl602_wifidrv hal_drv vfs yloop utils cli blog
INCLUDE_COMPONENTS += easyflash4 etl
INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += $(PROJECT_NAME)

ifeq ($(CONFIG_BT),1)
INCLUDE_COMPONENTS += $(COMPONENTS_BLE)
ifeq ($(CONFIG_BT_MESH),1)
INCLUDE_COMPONENTS += blemesh
endif
endif


include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk
