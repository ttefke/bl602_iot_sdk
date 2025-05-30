OUTPUT_ARCH( "riscv" )

ENTRY( bl602_start )

__EM_SIZE = DEFINED(ble_controller_init) ? 8K : 0K;

MEMORY
{
  rom       (rxai!w) : ORIGIN = 0x21015000, LENGTH = 44K
  flash     (rxai!w) : ORIGIN = 0x23000000, LENGTH = 4M
  ram_tcm   (wxa)    : ORIGIN = 0x4200C000, LENGTH = (16K + 16K + 48K + 64K + 64K + 8K - __EM_SIZE) /*put itcm with dtam and also OCRAM*/
  ram_wifi  (wxa)    : ORIGIN = 0x42042000 - __EM_SIZE, LENGTH = (8K + 104K - 64K - 8K) /*leave 8K left for BLE*/
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

    /* static fw attribute entry */
    . = ALIGN(4);
    _bl_static_fw_cfg_entry_start = .;
    KEEP(*(.wifi.cfg.entry))
    _bl_static_fw_cfg_entry_end = .;

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

    /*Framework table section, use ALIGN here to avoid fill section*/
    . = ALIGN(4);
    _rom_framework_audio_device_start = .;
    KEEP(*(.framework.audio_device))
    _rom_framework_audio_device_end = .;
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

  .romdata       :
  {
    /*always put freetos under global_pointer with the following order. No change!*/
    PROVIDE( __global_pointer_head$ =  . );
    PROVIDE( __global_pointer$ = . + 0x7F0 );
    . = . + 0x498;
  } > ram_tcm AT > flash

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

  /*CFG FW used in code*/
  PROVIDE( _ld_bl_static_cfg_entry_start = _bl_static_fw_cfg_entry_start );
  PROVIDE( _ld_bl_static_cfg_entry_end   = _bl_static_fw_cfg_entry_end );

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

  PROVIDE(xTaskGetTickCount = 0x0000000021017694);
  PROVIDE(xTaskGetTickCountFromISR = 0x00000000210176aa);
  PROVIDE(pvPortMalloc = 0x0000000021019662);
  PROVIDE(vPortFree = 0x000000002101973a);
  PROVIDE(vTaskNotifyGiveFromISR = 0x00000000210188e8);
  PROVIDE(vTaskSwitchContext = 0x0000000021017a04);
  PROVIDE(ulTaskNotifyTake = 0x0000000021018548);
  PROVIDE(vTaskExitCritical = 0x00000000210183f4);
  PROVIDE(vTaskEnterCritical = 0x00000000210183e4);
  PROVIDE(xTaskGetCurrentTaskHandle = 0x0000000021018152);
  PROVIDE(xQueueSemaphoreTake = 0x0000000021015ce8);
  PROVIDE(xQueueGenericSend = 0x0000000021015834);
  PROVIDE(xQueueGenericSendFromISR = 0x0000000021015a4c);
  PROVIDE(xTaskCreateStatic = 0x00000000210170a2);
  PROVIDE(xTaskCreate = 0x000000002101713a);
  PROVIDE(xQueueCreateMutex = 0x0000000021015a1c);
  PROVIDE(xQueueCreateMutexStatic = 0x0000000021015994);
  PROVIDE(vQueueDelete = 0x00000000210161d8);
  PROVIDE(xQueueGenericCreateStatic = 0x00000000210156c2);
  PROVIDE(xQueueGenericCreate = 0x0000000021015744);
  PROVIDE(xQueueReceive = 0x0000000021015b8a);
  PROVIDE(uxQueueMessagesWaiting = 0x0000000021016168);
  PROVIDE(vTaskDelay = 0x00000000210179c6);
  PROVIDE(vTaskDelayUntil = 0x0000000021017952);
  PROVIDE(xPortGetFreeHeapSize = 0x00000000210197ce);
  PROVIDE(vTaskList = 0x0000000021018408);
  PROVIDE(xTimerGenericCommand = 0x0000000021018bec);
  PROVIDE(xTimerCreateTimerTask = 0x0000000021018a9e);
  PROVIDE(xTimerCreate = 0x0000000021018af6);
  PROVIDE(xTimerCreateStatic = 0x0000000021018b66);
  PROVIDE(xQueueCreateCountingSemaphoreStatic = 0x00000000210157c2);
  PROVIDE(xQueueCreateCountingSemaphore = 0x0000000021015800);
  PROVIDE(pTrapNetCounter = __global_pointer_head$);
  PROVIDE(TrapNetCounter = __global_pointer_head$ + 0x58);
  PROVIDE(vEventGroupDelete = 0x00000000210153be);
  PROVIDE(xEventGroupWaitBits = 0x0000000021015086);
  PROVIDE(xEventGroupCreateStatic = 0x0000000021015000);
  PROVIDE(xEventGroupSetBits = 0x00000000210151e0);
  PROVIDE(xStreamBufferGenericCreateStatic = 0x00000000210165c0);
  PROVIDE(xStreamBufferReceive = 0x00000000210169ae);
  PROVIDE(xStreamBufferSend = 0x00000000210167a8);
  PROVIDE(pvTimerGetTimerID = 0x0000000021018fd4);
  PROVIDE(xTaskGenericNotify = 0x00000000210186be);
  PROVIDE(xTaskGenericNotifyFromISR = 0x00000000210187de);
  PROVIDE(xQueueGiveMutexRecursive = 0x00000000210159c8);
  PROVIDE(xQueueTakeMutexRecursive = 0x0000000021015e70);
  PROVIDE(xTaskGetTickCount2 = 0x000000002101769a);
  PROVIDE(xQueueGiveFromISR = 0x0000000021015b0e);
  PROVIDE(vTaskDelete = 0x00000000210171b6);
  PROVIDE(uxTaskGetStackHighWaterMark = 0x0000000021018110);
  PROVIDE(pcTaskGetName = 0x00000000210176b6);
  PROVIDE(vTaskStartScheduler = 0x0000000021017610);
  PROVIDE(vPortDefineHeapRegions = 0x00000000210197da);
  PROVIDE(__LD_CONFIG_EM_SEL = __EM_SIZE);

  PROVIDE( _ld_symbol_rom_framework_audio_device_start = _rom_framework_audio_device_start);
  PROVIDE( _ld_symbol_rom_framework_audio_device_end = _rom_framework_audio_device_end);

}
