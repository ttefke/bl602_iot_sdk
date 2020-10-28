README
=========

Bouffalolab bl_iot_sdk. Support BL602 Wi-Fi/BLE Combo RISC-V based Chip.

Check ``docs/html`` for more detail.

Fire an issue, if you have any issue or need any support.

Quick Start
===========

In order to build one of the sample apps, you need to set a few environment
variables:
```
export BL60X_SDK_PATH=/path/to/this/repo
export CONFIG_CHIP_NAME=bl602
```
Then go to the sample directory of interest and call `make`, for example:
```
cd customer_app/bl602_boot2
make
```
There is a linker script (written in python) at `image_conf/flash_build.py`.
To run this, you need to specify the application and the target, for example:
```
python3 flash_build.py bl602_boot2 bl602
```

Hardware
=========

BL602 is a 32-bit RISC-V based combo chipset supporting Wi-Fi and BLE (Bluetooth Low Energy). The chip is made by `Nanjing-based Bouffalo Lab <https://www.bouffalolab.com/bl602>`_ for ultra-low-power applications.
In terms of price range and feature set, the chip is competing against `Espressif ESP8266 <https://www.espressif.com/en/products/socs/esp8266>`_

Comparison with ESP8266
=======================
+-------------------+-----------------------------+----------------------------------+
|                   | Bouffalo Lab BL602          | Espressif ESP8266                |
+===================+=============================+==================================+
| Architecture      | 32-bit RISC-V               | 32-bit Xtensa                    |
|                   |                             |                                  |
|                   | @192MHz (dynamic @1-192MHz) | @80MHz (and 160MHz)              |
|                   |                             |                                  |
|                   | L1 cache                    |                                  |
|                   |                             |                                  |
|                   | FPU                         |                                  |
+-------------------+-----------------------------+----------------------------------+
| Memory            | 276KB SRAM                  | 32 KiB instruction RAM           |
|                   |                             |                                  |
|                   | 128KB ROM                   | 32 KiB instruction cache RAM     |
|                   |                             |                                  |
|                   | 1 Kb eFuse                  | 80 KiB user-data RAM             |
|                   |                             |                                  |
|                   | optional embdedded flash    | 16 KiB ETS system-data RAM       |
|                   |                             |                                  |
|                   |                             |                                  |
|                   | XIP QSPI flash support      | No programmable ROM              |
|                   |                             |                                  |
|                   |                             | QSPI flash support               |
|                   |                             | (up to 16 MB)                    |
+-------------------+-----------------------------+----------------------------------+
| Wi-Fi             | 802.11 b/g/n @2.4GHz        | 802.11 b/g/n @2.4GHz             |
|                   |                             |                                  |
|                   | WPS/WEP/WPA/WPA2/WPA3       | WEP/WPA/WPA2                     |
+-------------------+-----------------------------+----------------------------------+
| Bluetooth         | LE 5.0                      | NONE                             |
+-------------------+-----------------------------+----------------------------------+
| GPIO              | x16                         | x16                              |
+-------------------+-----------------------------+----------------------------------+
| SDIO              | x1 2.0 slave                | x1 v2.0 slave                    |
+-------------------+-----------------------------+----------------------------------+
| SPI               | x1                          | x2                               |
+-------------------+-----------------------------+----------------------------------+
| UART              | x2                          | x1.5                             |
|                   |                             | (One Tx only)                    |
+-------------------+-----------------------------+----------------------------------+
| I2C               | x1                          | x1 (software implemented)        |
+-------------------+-----------------------------+----------------------------------+
| I2S               | NONE                        | x1 (with DMA)                    |
+-------------------+-----------------------------+----------------------------------+
| PWM channels      | x5                          | x4                               |
+-------------------+-----------------------------+----------------------------------+
| ADC               | 12-bit                      | 10-bit (SAR)                     |
+-------------------+-----------------------------+----------------------------------+
| DAC               | 10-bit                      | NONE                             |
+-------------------+-----------------------------+----------------------------------+
| Analog Comparator | x2                          | NONE                             |
+-------------------+-----------------------------+----------------------------------+
| DMA               | x4                          | with I2S                         |
+-------------------+-----------------------------+----------------------------------+
| Timer             | RTC (up to 1 year)          | x1 hardware                      |
|                   |                             |                                  |
|                   | x2 32-bit general-purpose   | x1 software                      |
|                   |                             |                                  |
|                   |                             | (no interrupt gen. on sw. timer) |
+-------------------+-----------------------------+----------------------------------+
| IR Remote Control | x1                          | x1                               |
+-------------------+-----------------------------+----------------------------------+
| Debug             | JTAG support                | ?                                |
+-------------------+-----------------------------+----------------------------------+
