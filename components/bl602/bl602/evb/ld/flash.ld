OUTPUT_ARCH( "riscv" )

ENTRY( bl602_start )

__EM_SIZE = DEFINED(ble_controller_init) ? 8K : 0K;

MEMORY
{
  rom       (rxai!w) : ORIGIN = 0x21015000, LENGTH = 44K
  flash     (rxai!w) : ORIGIN = 0x23000000, LENGTH = 4M
  ram_tcm   (wxa)    : ORIGIN = 0x4200C000, LENGTH = (16K + 16K + 48K + 64K) /*put itcm with dtcm and also OCRAM*/
  ram_wifi  (wxa)    : ORIGIN = 0x42030000, LENGTH = (104K) /*leave 8K left for BLE*/
}

SECTIONS
{
  __stack_size = DEFINED(__stack_size) ? __stack_size : 2K;
  BOOT2_PT_ADDR = 0x42049C00;
  BOOT2_FLASHCFG_ADDR = 0x42049c18;

  .init           :
  {
    KEEP (*(SORT_NONE(.init)))
  } > flash

  .text           :
  {
    *(.text.unlikely .text.unlikely.*)
    *(.text.startup .text.startup.*)
    *(.text .text.*)
    *(.gnu.linkonce.t.*)
  } > flash

  .rodata         :
  {
    *(.rdata)
    *(.rodata .rodata.*)
    *(.sdata2.*)

    /* static cli cmds */
    . = ALIGN(4);
    _bl_static_cli_cmds_start = .;
    KEEP(*(.static_cli_cmds))
    *(.static_cli_cmds)
    _bl_static_cli_cmds_end = .;

    /* static blog code1 */
    . = ALIGN(4);
    _bl_static_blogcomponent_code_start = .;
    KEEP(SORT(*)(.static_blogcomponent_code*))
    *(.static_blogcomponent_code*)
    _bl_static_blogcomponent_code_end = .;

    /* static blog code2 */
    . = ALIGN(4);
    _bl_static_blogfile_code_start = .;
    KEEP(SORT(*)(.static_blogfile_code*))
    *(.static_blogfile_code*)
    _bl_static_blogfile_code_end = .;

    /* static blog code3 */
    . = ALIGN(4);
    _bl_static_blogpri_code_start = .;
    KEEP(SORT(*)(.static_blogpri_code*))
    *(.static_blogpri_code*)
    _bl_static_blogpri_code_end = .;

    *(.gnu.linkonce.r.*)
  } > flash
  
  .preinit_array :
  {
    . = ALIGN(4);
    __preinit_array_start = .;
    KEEP (*(.preinit_array))
    __preinit_array_end = .;
  } > flash

  .init_array :
  {
    . = ALIGN(4);
    __init_array_start = .;
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*)))
    KEEP (*(.init_array))
    __init_array_end = .;
  } > flash

  /*put wifibss in the first place*/
  .wifibss         (NOLOAD) :
  {
    PROVIDE( __wifi_bss_start = ADDR(.wifibss) );
    PROVIDE( __wifi_bss_end = ADDR(.wifibss) + SIZEOF(.wifibss) );
    *ipc_shared.o(COMMON)
    *sdu_shared.o(COMMON)
    *hal_desc.o(COMMON)
    *txl_buffer_shared.o(COMMON)
    *txl_frame_shared.o(COMMON)
    *scan_shared.o(COMMON)
    *scanu_shared.o(COMMON)
    *mfp_bip.o(COMMON)
    *me_mic.o(COMMON)
    *bl_sta_mgmt_others.o(COMMON)
    *bl_pmk_mgmt.o(COMMON)
    *bl_pmk_mgmt_internal.o(COMMON)
    *libwifi_drv.a:bl_utils.o(COMMON)
    *libwifi_drv.a:bl_utils.o(.bss*)
    *(.wifi_ram*)
    . = ALIGN(16);
  } > ram_wifi

  PROVIDE( _heap_wifi_start = . );
  PROVIDE( _heap_wifi_size = ORIGIN(ram_wifi) + LENGTH(ram_wifi) - _heap_wifi_start );

  .data          :
  {
    PROVIDE( _data_load = LOADADDR(.data) );
    PROVIDE( _data_run = ADDR(.data) );
    PROVIDE( _data_run_end = ADDR(.data) + SIZEOF(.data));

    *(.tcm_code)
    *(.tcm_const)
    *(.sclock_rlt_code)
    *(.sclock_rlt_const)
    *(.data .data.*)
    *(.gnu.linkonce.d.*)

    . = ALIGN(8);
    /*always put freetos under global_pointer with the following order. No change!*/
    PROVIDE( __global_pointer$ = . + 0x7F0 );
    KEEP(*libfreertos_riscv.a:*(.sdata.*))
    KEEP(*libfreertos_riscv.a:*(.sbss.*))
    KEEP(*libfreertos_riscv.a:*(.bss.*))
    KEEP(*libfreertos_riscv.a:*(COMMON))
    *(.sdata .sdata.*)
    *(.gnu.linkonce.s.*)

    . = ALIGN(8);
    *(.srodata.cst16)
    *(.srodata.cst8)
    *(.srodata.cst4)
    *(.srodata.cst2)
    *(.srodata .srodata.*)

    . = ALIGN(8);
    *(._k_queue.static.*)
    *(._k_sem.static.*)
    *(._k_mutex.static.*)
    _net_buf_pool_list = .;
    KEEP(*(SORT_BY_NAME("._net_buf_pool.static.*")))
    _bt_gatt_service_static_list_start = .;
    KEEP(*(SORT_BY_NAME("._bt_gatt_service_static.static.*")))
    _bt_gatt_service_static_list_end = .;
    _bt_l2cap_fixed_chan_list_start = .;
    KEEP(*(SORT_BY_NAME("._bt_l2cap_fixed_chan.static.*")))
    _bt_l2cap_fixed_chan_list_end = .;
  } > ram_tcm AT > flash

  .boot2 (NOLOAD) :
  {
      PROVIDE ( __boot2_pt_addr_start = . );
      *(.bss.boot2_partition_table)
      PROVIDE ( __boot2_pt_addr_end   = . );

      PROVIDE ( __boot2_flashCfg_start = . );
      *(.bss.boot2_flashCfg)
      PROVIDE ( __boot2_flashCfg_end = . );

  } > ram_tcm

  .bss (NOLOAD)   :
  {
    PROVIDE( __bss_start = ADDR(.bss) );
    PROVIDE( __bss_end = ADDR(.bss) + SIZEOF(.bss) );

    *(.sbss*)
    *(.gnu.linkonce.sb.*)
    *(.bss .bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
  } > ram_tcm

  PROVIDE( _heap_start = . );
  PROVIDE( _heap_size = ADDR(.stack) - _heap_start );


  .stack ORIGIN(ram_tcm) + LENGTH(ram_tcm) - __stack_size (NOLOAD) :
  {
    . = . + __stack_size;
    PROVIDE( _sp_main = . );
    __freertos_irq_stack_top = .;
  } >ram_tcm


  /*SYMOBOL used in code*/
  PROVIDE( _ld_bl_static_cli_cmds_start = _bl_static_cli_cmds_start );
  PROVIDE( _ld_bl_static_cli_cmds_end   = _bl_static_cli_cmds_end );

  /* blog */
  PROVIDE( _ld_bl_static_blogcomponent_code_start = _bl_static_blogcomponent_code_start );
  PROVIDE( _ld_bl_static_blogcomponent_code_end   = _bl_static_blogcomponent_code_end );
  PROVIDE( _ld_bl_static_blogfile_code_start = _bl_static_blogfile_code_start );
  PROVIDE( _ld_bl_static_blogfile_code_end   = _bl_static_blogfile_code_end );
  PROVIDE( _ld_bl_static_blogpri_code_start = _bl_static_blogpri_code_start );
  PROVIDE( _ld_bl_static_blogpri_code_end   = _bl_static_blogpri_code_end );

  PROVIDE( _ld_ram_size0 = LENGTH(flash) );
  PROVIDE( _ld_ram_addr0 = ORIGIN(flash) );
  PROVIDE( _ld_ram_size1 = LENGTH(ram_tcm) );
  PROVIDE( _ld_ram_addr1 = ORIGIN(ram_tcm) );
  PROVIDE( _ld_ram_size2 = LENGTH(ram_wifi) );
  PROVIDE( _ld_ram_addr2 = ORIGIN(ram_wifi) );


  /*BOOT2 sections*/
  PROVIDE ( __boot2_pt_addr_src = BOOT2_PT_ADDR );
  PROVIDE ( __boot2_flashCfg_src = BOOT2_FLASHCFG_ADDR );

  PROVIDE(__LD_CONFIG_EM_SEL = __EM_SIZE);
}
