/*
 * image_boot.c
 *
 *  Created on: Oct 10, 2020
 *      Author: zjm09
 */
#include "main.h"

void boot(uint32_t imgStart)
{
//	img_header_t *h = (void *)imgStart;
	typedef void (*funcPtr)(void);
	uint32_t appStart = imgStart;// + h->headerSize;
	funcPtr userMain;

	userMain = (funcPtr)(*(volatile uint32_t *)(appStart + 4));

//	SCB_DisableICache();
//	SCB_DisableDCache();

	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL  = 0;

	SCB->VTOR = appStart;

	__set_MSP(*(volatile uint32_t *)appStart);

	userMain();

	while(1);
}

