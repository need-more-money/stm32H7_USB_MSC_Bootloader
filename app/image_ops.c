/*
 * image_chk.c
 *
 *  Created on: Oct 10, 2020
 *      Author: zjm09
 */
#include <string.h>
#include "main.h"
#include "octospi.h"
#include "gpio.h"
//#include "usb_device.h"
#include "mbedtls/sha256.h"
//#include "mbedtls/pk.h"
//#include "mbedtls/rsa.h"

extern int __m_text_start;
extern int __m_text_size;
extern int __m_data_size;

typedef struct{
	union{
		struct{
			uint32_t magic;
			char image_name[64];
			uint32_t image_size;
			uint32_t exec_addr;
			uint32_t sign_method;
			uint8_t sign[0];
		};
		uint8_t array[1024];
	};
}file_header_t;

//mbedtls_sha256_context sha256;

//
//void flash_erase_all(void)
//{
//	flash?sfud_chip_erase(flash):0;
//}
//
//void flash_erase(uint32_t offset, uint32_t size)
//{
//	flash?sfud_erase(flash, offset, size):0;
//}
//
//void flash_read(uint32_t offset, void *buf, uint32_t size)
//{
//	flash?sfud_read(flash, offset, size, buf):0;
//}
//
//void flash_write(uint32_t offset, void *buf, uint32_t size)
//{
//	flash?sfud_write(flash, offset, size, buf):0;
//}

int self_check(void)
{
	int image_size = (uint32_t)&__m_text_size + (uint32_t)&__m_data_size;
	uint8_t sha256_hash[32];
	void *p = &__m_text_start;
	uint8_t *sha256 = (void *)((uint32_t)p + image_size);
	// check self image
	mbedtls_sha256_ret(p, image_size, sha256_hash, 0);
	if(memcmp(sha256_hash, sha256, sizeof(sha256_hash))){
//		printf("sha256 error...\n");
		return 1;
	}

	return 1;
}

uint8_t QSPI_Wait_Flag(uint32_t flag, uint8_t sta, uint32_t wtime)
{
    uint8_t flagsta = 0;
    while(wtime)
    {
        flagsta = (OCTOSPI1->SR & flag) ? 1 : 0;
        if(flagsta == sta)break;
        wtime--;
    }
    if(wtime)return 0;
    else return 1;
}

static void QSPI_EnableMapped(void)
{
//	OSPI_RegularCmdTypeDef s_command;
//	OSPI_MemoryMappedTypeDef s_mem_mapped_cfg;
//
//	/* Configure the command for the read instruction */
//	s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
//	s_command.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
//	s_command.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
//	s_command.DataMode = HAL_OSPI_DATA_4_LINES;
////	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
//	s_command.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
//	s_command.AlternateBytes = 0xFF;
//	s_command.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
////	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
//	s_command.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;
//	s_command.Instruction = 0xEB;
//	s_command.DummyCycles = 4;
//
//	/* Configure the memory mapped mode */
//	s_mem_mapped_cfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;
//	s_mem_mapped_cfg.TimeOutPeriod = 0;
//
//	HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
//
//	if (HAL_OSPI_MemoryMapped(&hospi1, &s_mem_mapped_cfg) != HAL_OK)
//	{
//
//	}

    uint32_t tempreg;

    tempreg = (3 << 24);
    tempreg |= (2 << 12);
    tempreg |= (1 << 8);
    tempreg |= (1 << 0);
    OCTOSPI1->CCR = tempreg;
    OCTOSPI1->IR = 0x6b;
    OCTOSPI1->TCR = 8;
    tempreg = OCTOSPI1->CR;
    tempreg &= ~(3 << 28);
    tempreg |= (3 << 28);
    OCTOSPI1->CR = tempreg;
}

static void QSPI_DisableMapped(void)
{
    hospi1.Instance->CR |= OCTOSPI_CR_ABORT_Msk;
    while ((hospi1.Instance->SR & OCTOSPI_SR_BUSY_Msk) != 0);
    hospi1.Instance->CR &= ~OCTOSPI_CR_FMODE_Msk;
    HAL_OSPI_DeInit(&hospi1);
    HAL_OSPI_Init(&hospi1);
//    QSPI_ResetMemory(hqspi);
}

void *isImageBootable(void)
{
	uint8_t sha256_hash[32];
	file_header_t *image_header = NULL;

	QSPI_EnableMapped();

	image_header = (void *)0x90000000;

	return (void *)image_header;

	if(image_header->magic == 0x77478507){

		mbedtls_sha256_ret((void *)image_header->exec_addr, image_header->image_size, sha256_hash, 0);

		if(!memcmp(sha256_hash, image_header->sign, sizeof(sha256_hash))){
			if(image_header->exec_addr)
				return (void *)image_header->exec_addr;
			else
				return (void *)(((uint32_t)image_header)+sizeof(file_header_t));
		}

	}

	QSPI_DisableMapped();

	return (void *)-1;
}

void boot(uint32_t imgStart);
void doBoot(void)
{
	if(self_check()){
		if(LL_GPIO_IsInputPinSet(BOOT_KEY_GPIO_Port, BOOT_KEY_Pin)){
			void *start = isImageBootable();
			if(start != (void *)-1){
				boot((uint32_t)start);
			}
		}

		LL_GPIO_SetOutputPin(USBHS_RST_GPIO_Port, USBHS_RST_Pin);
//		LL_GPIO_WritePin(USBHS_RST_GPIO_Port, USBHS_RST_Pin, GPIO_PIN_SET);
		HAL_Delay(100);
		LL_GPIO_ResetOutputPin(USBHS_RST_GPIO_Port, USBHS_RST_Pin);
//		HAL_GPIO_WritePin(USBHS_RST_GPIO_Port, USBHS_RST_Pin, GPIO_PIN_RESET);
		HAL_Delay(100);

//		MX_USB_DEVICE_Init();
	}
}
