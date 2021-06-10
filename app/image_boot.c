/*
 * image_boot.c
 *
 *  Created on: Oct 10, 2020
 *      Author: zjm09
 */
#include "main.h"

typedef  void (*pFunction)(void);

__attribute__((noreturn)) void boot(uint32_t addr)
{
    const uint32_t *vtor = (uint32_t *)addr;
    pFunction JumpToApplication;

//    __disable_irq();
    SCB_DisableICache();
//    SCB_DisableDCache();

//    NVIC->ICER[0] = 0xFFFFFFFF;
//    NVIC->ICER[1] = 0xFFFFFFFF;
//    NVIC->ICER[2] = 0xFFFFFFFF;
//    NVIC->ICER[3] = 0xFFFFFFFF;
//    NVIC->ICER[4] = 0xFFFFFFFF;
//    NVIC->ICER[5] = 0xFFFFFFFF;
//    NVIC->ICER[6] = 0xFFFFFFFF;
//    NVIC->ICER[7] = 0xFFFFFFFF;
//
//    NVIC->ICPR[0] = 0xFFFFFFFF;
//    NVIC->ICPR[1] = 0xFFFFFFFF;
//    NVIC->ICPR[2] = 0xFFFFFFFF;
//    NVIC->ICPR[3] = 0xFFFFFFFF;
//    NVIC->ICPR[4] = 0xFFFFFFFF;
//    NVIC->ICPR[5] = 0xFFFFFFFF;
//    NVIC->ICPR[6] = 0xFFFFFFFF;
//    NVIC->ICPR[7] = 0xFFFFFFFF;

    SysTick->CTRL = 0;
//    SysTick->LOAD = 0; // Needed?
//    SysTick->VAL = 0;  // Needed?
//    SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;
//
//    SCB->SHCSR &= ~(SCB_SHCSR_USGFAULTENA_Msk | //
//                    SCB_SHCSR_BUSFAULTENA_Msk | //
//                    SCB_SHCSR_MEMFAULTENA_Msk);
    SCB->VTOR = addr;
    __set_MSP(vtor[0]);
//    __set_PSP(vtor[0]);
//    __set_CONTROL(0);
    JumpToApplication = (pFunction) (*(__IO uint32_t*) (addr + 4));
    JumpToApplication();
    for (;;)
        ;
}

