/*
 * image_chk.c
 *
 *  Created on: Oct 10, 2020
 *      Author: zjm09
 */
#include <string.h>
#include "main.h"
#include "quadspi.h"
#include "usb_device.h"
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

mbedtls_sha256_context sha256;

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

	// check self image
	mbedtls_sha256_ret(p, image_size, sha256_hash, 0);


	return 1;
}

static void QSPI_EnableMapped(void)
{
	QSPI_CommandTypeDef s_command;
	QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;

	/* Configure the command for the read instruction */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.Instruction = 0xEB;
	s_command.AddressMode = QSPI_ADDRESS_4_LINES;
	s_command.AddressSize = QSPI_ADDRESS_24_BITS;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
	s_command.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
	s_command.DataMode = QSPI_DATA_4_LINES;
	s_command.DummyCycles = 4;
	s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
	s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure the memory mapped mode */
	s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
	s_mem_mapped_cfg.TimeOutPeriod = 0;

	if (HAL_QSPI_MemoryMapped(&hqspi, &s_command, &s_mem_mapped_cfg) != HAL_OK)
	{

	}
}

static void QSPI_DisableMapped(void)
{
    hqspi.Instance->CR |= QUADSPI_CR_ABORT_Msk;
    while ((hqspi.Instance->SR & QUADSPI_SR_BUSY_Msk) != 0);
    hqspi.Instance->CCR &= ~QUADSPI_CCR_FMODE_Msk;
    HAL_QSPI_DeInit(&hqspi);
    HAL_QSPI_Init(&hqspi);
//    QSPI_ResetMemory(hqspi);
}

void *isImageBootable(void)
{
	uint8_t sha256_hash[32];
	file_header_t *image_header = NULL;

	QSPI_EnableMapped();

	image_header = (void *)0x90000000;
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
		void *start = isImageBootable();
		if(start != (void *)-1){
			boot((uint32_t)start);
		}

		MX_USB_DEVICE_Init();
	}
}
