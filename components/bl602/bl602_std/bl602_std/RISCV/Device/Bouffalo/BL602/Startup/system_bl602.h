#ifndef __SYSTEM_BL602_H__
#define __SYSTEM_BL602_H__

/**
 *  @brief PLL Clock type definition
 */

extern uint32_t SystemCoreClock;

extern void SystemCoreClockUpdate (void);
extern void SystemInit (void);
extern void Systick_Stop(void);
extern void Systick_Start(void);

#endif
