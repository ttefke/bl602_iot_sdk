#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <stdbool.h>

/* BouffaloLabs HALs */
#include <blog.h>
#include <bl_dma.h>
#include <bl_irq.h>
#include <bl_rtc.h>
#include <bl_sec.h>
#include <bl_sys.h>
#include <bl_uart.h>
#include <hal_board.h>
#include <hal_boot2.h>
#ifdef REBOOT_ON_EXCEPTION
#include <bl_sys.h>
#endif

/* Functions required by FreeRTOS */
#define TIME_5MS_IN_32768CYCLE  (164) // (5000/(1000000/32768))

volatile uint32_t uxTopUsedPriority __attribute__((used)) = configMAX_PRIORITIES - 1;
extern bool pds_start;

/* Define heap regions */
extern uint8_t _heap_start;
extern uint8_t _heap_size;
extern uint8_t _heap_wifi_start;
extern uint8_t _heap_wifi_size;

static HeapRegion_t xHeapRegions[] =
{
  { &_heap_start, (unsigned int) &_heap_size},
  { &_heap_wifi_start, (unsigned int) &_heap_wifi_size },
  { NULL, 0},
  { NULL, 0}
};

[[gnu::weak]] void vInitializeBL602(void)
{
  // Early init
  bl_sys_early_init();

  /* Initialize UART
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);
  printf("[BL602] Starting up!\r\n");

  // System init
  bl_sys_init();

  /* Define Heap */
  vPortDefineHeapRegions(xHeapRegions);
  
  /* Initialize system */
  blog_init();
  bl_irq_init();
  bl_sec_init();
  bl_dma_init();
  bl_rtc_init();
  hal_boot2_init();
  hal_board_cfg(0);
}

[[gnu::weak]] void vAssertCalled(void)
{
#ifdef REBOOT_ON_EXCEPTION
  bl_sys_reset_system();
#else
  volatile uint32_t ulSetTo1ToExitFunction = 0;
  
  taskDISABLE_INTERRUPTS();
  
  while(ulSetTo1ToExitFunction != 1) {
    __asm volatile("NOP");
  }
#endif
}

void user_vAssertCalled(void) __attribute__ ((weak, alias ("vAssertCalled")));

[[gnu::weak]] void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
  /* if the buffers to be provided to the idle task are declared inside
   * this function then they must be declared static - otherwise they
   * will be allocated on the stack and will be removed after this function exits */
   static StaticTask_t xIdleTaskTCB;
   static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
   
   /* pass out a pointer to the StaticTask_t structure in which the state of
    * the idle task will be stored */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    
   /* pass out the array that will be used as the stack of the idle task */
   *ppxIdleTaskStackBuffer = uxIdleTaskStack;
   
   /* pass out the size of the array pointed to by *ppxIdleStackBuffer.
    * Note that, as the array is of the type stackType_t, configMINIMAL_STACK_SIZE
    * is specified in words, not bytes */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

[[gnu::weak]] void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
  StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
  /* if the buffers to be provided to the timer task are declared inside of this
   * function, they must be static - otherwise they will be gone after the function ends */
   static StaticTask_t xTimerTaskTCB;
   static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
   
   /* pass out a pointer to the StaticTask_t struct in which the state of
    * the timer will be stored */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    
    /* pass out the array that will be used as the timer's stack */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    
    /* pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * note that configTimer_TASK_STACK_DEPTH is specified in words as
     * the array is of the type StackType_t */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

[[gnu::weak]] void vApplicationIdleHook(void)
{
#if defined(CFG_BLE_PDS)
  if (!pds_start) {
#endif
  (void)uxTopUsedPriority;
#if defined(CFG_BLE_PDS)
  }
#endif
}

[[gnu::weak]] void vApplicationMallocFailedHook(void)
{
  printf("[%s] malloc failed, currently left memory in bytes: %d\r\n",
      __func__, xPortGetFreeHeapSize());
#ifdef REBOOT_ON_EXCEPTION
  bl_sys_reset_system();
#else
  while (1) {}
#endif
}

[[gnu::weak]] void vApplicationStackOverflowHook([[gnu::unused]] TaskHandle_t xTask, char *pcTaskName)
{
  printf("[%s] Stack overflow, task name: %s\r\n", __func__, pcTaskName);
#ifdef REBOOT_ON_EXCEPTION
  bl_sys_reset_system();
#else
  while (1) {}
#endif
}

#if ( configUSE_TICKLESS_IDLE != 0 )
[[gnu::weak]] void vApplicationSleep( [[gnu::unused]] TickType_t xExpectedIdleTime_ms )
{
#if defined(CFG_BLE_PDS)
    int32_t bleSleepDuration_32768cycles = 0;
    int32_t expectedIdleTime_32768cycles = 0;
    eSleepModeStatus eSleepStatus;
    bool freertos_max_idle = false;

    if (pds_start == 0)
        return;

    if(xExpectedIdleTime_ms + xTaskGetTickCount() == portMAX_DELAY){
        freertos_max_idle = true;
    }else{
        xExpectedIdleTime_ms -= 1;
        expectedIdleTime_32768cycles = 32768 * xExpectedIdleTime_ms / 1000;
    }

    if((!freertos_max_idle)&&(expectedIdleTime_32768cycles < TIME_5MS_IN_32768CYCLE)){
        return;
    }

    /*Disable mtimer interrrupt*/
    *(volatile uint8_t*)configCLIC_TIMER_ENABLE_ADDRESS = 0;

    eSleepStatus = eTaskConfirmSleepModeStatus();
    if(eSleepStatus == eAbortSleep || ble_controller_sleep_is_ongoing())
    {
        /*A task has been moved out of the Blocked state since this macro was
        executed, or a context siwth is being held pending.Restart the tick
        and exit the critical section. */
        /*Enable mtimer interrrupt*/
        *(volatile uint8_t*)configCLIC_TIMER_ENABLE_ADDRESS = 1;
        //printf("%s:not do ble sleep\r\n", __func__);
        return;
    }

    bleSleepDuration_32768cycles = ble_controller_sleep();

    if(bleSleepDuration_32768cycles < TIME_5MS_IN_32768CYCLE)
    {
        /*BLE controller does not allow sleep.  Do not enter a sleep state.Restart the tick
        and exit the critical section. */
        /*Enable mtimer interrrupt*/
        //printf("%s:not do pds sleep\r\n", __func__);
        *(volatile uint8_t*)configCLIC_TIMER_ENABLE_ADDRESS = 1;
    }
    else
    {
        printf("%s:bleSleepDuration_32768cycles=%ld\r\n", __func__, bleSleepDuration_32768cycles);
        if(eSleepStatus == eStandardSleep && ((!freertos_max_idle) && (expectedIdleTime_32768cycles < bleSleepDuration_32768cycles)))
        {
           hal_pds_enter_with_time_compensation(1, expectedIdleTime_32768cycles - 40);//40);//20);
        }
        else
        {
           hal_pds_enter_with_time_compensation(1, bleSleepDuration_32768cycles - 40);//40);//20);
        }
    }
#endif
}
#endif
