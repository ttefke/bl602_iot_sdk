/****************************************************************************************
* @file map.txt
*
* @brief This file is the map file (gnuarm or armgcc).
*
* Copyright (C) BouffaloLab 2018
*
****************************************************************************************
*/

/* configure the CPU type */
OUTPUT_ARCH( "riscv" )
/* link with the standard c library */
INPUT(-lc)
/* link with the standard GCC library */
INPUT(-lgcc)
/* configure the entry point */
ENTRY(_enter)

StackSize = 0x0400; /*  1KB */

MEMORY
{
    xip_memory  (rx)  : ORIGIN = 0x23000000, LENGTH = 48K
    itcm_memory (rx)  : ORIGIN = 0x22010000, LENGTH = 16K
    dtcm_memory (rx)  : ORIGIN = 0x42014000, LENGTH = 48K
    ram_memory  (!rx) : ORIGIN = 0x42020000, LENGTH = 96K
}

SECTIONS
{
    PROVIDE(__metal_chicken_bit = 0);

    .text :
    {
        . = ALIGN(4);
        __text_code_start__ = .;

        KEEP (*(.text.metal.init.enter))
        KEEP (*(SORT_NONE(.init)))

        *(.text)
        *(.text.*)

        /*put .rodata**/
        *(EXCLUDE_FILE( *bl602_glb.o \
                        *bl602_pds.o \
                        *bl602_common.o \
                        *bl602_sf_cfg.o \
                        *bl602_sf_ctrl.o \
                        *bl602_sflash.o \
                        *bl602_xip_sflash.o \
                        *bl602_ef_ctrl.o) .rodata*)
        *(.rodata)
        *(.rodata.*)

        *(.srodata)
        *(.srodata.*)

        . = ALIGN(4);
        __text_code_end__ = .;
    } > xip_memory

    . = ALIGN(4);
    __itcm_load_addr = .;

    .itcm_region : AT (__itcm_load_addr)
    {
        . = ALIGN(4);
        __tcm_code_start__ = .;

        *(.tcm_code)
        *(.tcm_const)
        *(.sclock_rlt_code)
        *(.sclock_rlt_const)

        *bl602_romapi.o(.text)
        *bl602_romapi.o(.text.*)
        *bl602_romapi.o(.rodata)
        *bl602_romapi.o(.rodata.*)
        *bl602_romapi.o(.srodata)
        *bl602_romapi.o(.srodata.*)

        *bl602_glb.o*(.rodata*)
        *bl602_pds.o*(.rodata*)
        *bl602_common.o*(.rodata*)
        *bl602_sf_cfg.o*(.rodata*)
        *bl602_sf_ctrl.o*(.rodata*)
        *bl602_sflash.o*(.rodata*)
        *bl602_xip_sflash.o*(.rodata*)
        *bl602_ef_ctrl.o*(.rodata*)

        . = ALIGN(4);
        __tcm_code_end__ = .;
    } > itcm_memory

    __dtcm_load_addr = __itcm_load_addr + SIZEOF(.itcm_region);

    .dtcm_region : AT (__dtcm_load_addr)
    {
        . = ALIGN(4);
        __tcm_data_start__ = .;

        *(.tcm_data)

        . = ALIGN(4);
        __tcm_data_end__ = .;
    } > dtcm_memory

    /*************************************************************************/
    /* .stack_dummy section doesn't contains any symbols. It is only
     * used for linker to calculate size of stack sections, and assign
     * values to stack symbols later */
    .stack_dummy (NOLOAD):
    {
        . = ALIGN(0x4);
        . = . + StackSize;
        . = ALIGN(0x4);
    } > dtcm_memory

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __StackTop = ORIGIN(dtcm_memory) + LENGTH(dtcm_memory);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);

    /* Check if data + heap + stack exceeds RAM limit */
    ASSERT(__StackLimit >= __tcm_data_end__, "region RAM overflowed with stack")
    /*************************************************************************/

    __system_ram_load_addr = __dtcm_load_addr + SIZEOF(.dtcm_region);

    .system_ram_data_region : AT (__system_ram_load_addr)
    {
        . = ALIGN(4);
        __system_ram_data_start__ = .;

        *(.system_ram)

        . = ALIGN(4);
        __system_ram_data_end__ = .;
    } > ram_memory

    __ram_load_addr = __system_ram_load_addr + SIZEOF(.system_ram_data_region);

    /* Data section */
    RAM_DATA : AT (__ram_load_addr)
    {
        . = ALIGN(4);
        __ram_data_start__ = .;

        PROVIDE( __global_pointer$ = . + 0x800 );

        *(.data)
        *(.data.*)
        *(.sdata)
        *(.sdata.*)
        *(.sdata2)
        *(.sdata2.*)

        . = ALIGN(4);
        __ram_data_end__ = .;
    } > ram_memory

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;

        *(.bss*)
        *(.sbss*)
        *(COMMON)

        . = ALIGN(4);
        __bss_end__ = .;
    } > ram_memory

    .noinit_data (NOLOAD) :
    {
        . = ALIGN(4);
        __noinit_data_start__ = .;

        *(.noinit_data*)

        . = ALIGN(4);
        __noinit_data_end__ = .;
    } > ram_memory

    .heap (NOLOAD):
    {
        . = ALIGN(4);
        __HeapBase = .;

        /*__end__ = .;*/
        /*end = __end__;*/
        KEEP(*(.heap*))

        . = ALIGN(4);
        __HeapLimit = .;
    } > ram_memory

}

