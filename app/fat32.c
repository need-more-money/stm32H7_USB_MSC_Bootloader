/*************************************************************************************
# Released under MIT License

Copyright (c) 2020 SF Yip (yipxxx@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "main.h"
#include "btldr_config.h"
#include "fat32.h"
#include "sfud.h"

//-------------------------------------------------------

#ifndef MIN
  #define MIN(a,b) (((a)<(b))?(a):(b))
#endif

//-------------------------------------------------------

#define FAT32_SECTOR_SIZE           512

#define FAT32_ATTR_READ_ONLY        0x01
#define FAT32_ATTR_HIDDEN           0x02
#define FAT32_ATTR_SYSTEM           0x04
#define FAT32_ATTR_VOLUME_ID        0x08
#define FAT32_ATTR_DIRECTORY        0x10
#define FAT32_ATTR_ARCHIVE          0x20
#define FAT32_ATTR_LONG_NAME        (FAT32_ATTR_READ_ONLY | FAT32_ATTR_HIDDEN | FAT32_ATTR_SYSTEM | FAT32_ATTR_VOLUME_ID)

#define FAT32_MAKE_DATE(dd, mm, yyyy)   (uint16_t)( ( (((yyyy)-1980) & 0x7F)  << 9) | (((mm) & 0x0F) << 5) | ((dd) & 0x1F) )
#define FAT32_MAKE_TIME(hh,mm)          (uint16_t)(( ((hh) & 0x1F)<< 11) | (((mm) & 0x3F) << 5))

//-------------------------------------------------------

typedef struct __attribute__((packed))
{
    uint8_t BS_jmpBoot[3];	//
    uint8_t BS_OEMName[8];
    uint16_t BPB_BytsPerSec;	// 0B
    uint8_t BPB_SecPerClus;		// 0D
    uint16_t BPB_RsvdSecCnt;	// 0E
    uint8_t BPB_NumFATs;		// 10
    uint16_t BPB_RootEntCnt;	// 11
    uint16_t BPB_TotSec16;		// 13
    uint8_t BPB_Media;			// 15
    uint16_t BPB_FATSz16;		// 16
    uint16_t BPB_SecPerTrk;		// 18
    uint16_t BPB_NumHeads;		// 1A
    uint32_t BPB_HiddSec;		// 1C
    uint32_t BPB_TotSec32;		// 20
    
    // FAT32 Structure
    uint32_t BPB_FATSz32;		// 24
    uint16_t BPB_ExtFlags;		// 28
    uint16_t BPB_FSVer;			// 2A
    uint32_t BPB_RootClus;		// 2C
    uint16_t BPB_FSInfo;		// 30
    uint16_t BPB_BkBootSec;		// 34
    uint8_t BS_Reserved[12];	// 36
    uint8_t BS_DrvNum;			// 40
    uint8_t BS_Reserved1;		// 41
    uint8_t BS_BootSig;			// 42
    uint32_t BS_VolID;			// 43
    uint8_t BS_VolLab[11];		// 47
    uint8_t BS_FilSysType[8];
}fat32_bpb_t;

typedef struct __attribute__((packed))
{
    uint32_t FSI_LeadSig;
    uint8_t FSI_Reserved1[480];
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count;
    uint32_t FSI_Nxt_Free;
    uint8_t FSI_Reserved2[12];
    uint32_t FSI_TrailSig;
}fat32_fsinfo_t;

typedef struct __attribute__((packed))
{
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
}fat32_dir_entry_t;

//-------------------------------------------------------

enum{
	FW_TYPE_BIN = 1,
	FW_TYPE_DATA = 2,
};

typedef struct
{
    uint32_t begin;
    uint32_t end;
    int32_t image_size;
    uint32_t type;
}fat32_range_t;



//static fat32_range_t fw_addr_range = {0x00400600, (0x00400600 + APP_SIZE)};
static fat32_range_t image_addr_range = {0, 0};
static const sfud_flash *flash = NULL;
//-------------------------------------------------------

#define FAT32_MBR_HARDCODE  0u
#define FAT32_BPB_RsvdSecCnt	(16)	// 保留扇区数
#define FAT32_BPB_FATSz32		(2560)	// FAT表扇区数
#define FAT32_FAT1_ADDRESS	(FAT32_BPB_RsvdSecCnt * FAT32_SECTOR_SIZE)
#define FAT32_FAT2_ADDRESS	(FAT32_FAT1_ADDRESS + (FAT32_BPB_FATSz32 * FAT32_SECTOR_SIZE))

#define FAT32_DIR_ENTRY_ADDR         (FAT32_FAT2_ADDRESS + (FAT32_BPB_FATSz32 * FAT32_SECTOR_SIZE))
#define FILE_README_ADDR	(FAT32_DIR_ENTRY_ADDR + 3*FAT32_SECTOR_SIZE)

#if (FAT32_MBR_HARDCODE > 0u)
static const uint8_t FAT32_MBR_DATA0[] = {
    0xEB, 0xFE, 0x90, 0x4D, 0x53, 0x44, 0x4F, 0x53, 0x35, 0x2E, 0x30, 0x00, 0x02, 0x01, 0x7C, 0x11,
    0x02, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x3F, 0x00, 0xFF, 0x00, 0x3F, 0x00, 0x00, 0x00,
    0xC1, 0xC0, 0x03, 0x00, 0x42, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x29, 0xB0, 0x49, 0x90, 0x02, 0x4E, 0x4F, 0x20, 0x4E, 0x41, 0x4D, 0x45, 0x20, 0x20,
    0x20, 0x20, 0x46, 0x41, 0x54, 0x33, 0x32, 0x20, 0x20, 0x20};
#endif
  
// Addr: 0x0000 and 0x0C00
static void _fat32_read_bpb(uint8_t *b)
{
#if (FAT32_MBR_HARDCODE > 0u)
    memcpy(b, FAT32_MBR_DATA0, sizeof(FAT32_MBR_DATA0));
    memset(b + sizeof(FAT32_MBR_DATA0), 0x00, FAT32_SECTOR_SIZE - sizeof(FAT32_MBR_DATA0) - 2);
    b[510] = 0x55;
    b[511] = 0xAA;
#else
    fat32_bpb_t *bpb = (fat32_bpb_t*)b;
    memset(b, 0, FAT32_SECTOR_SIZE);
    
    bpb->BS_jmpBoot[0] = 0xEB;
    bpb->BS_jmpBoot[1] = 0xFE;
    bpb->BS_jmpBoot[2] = 0x90;
    memcpy(bpb->BS_OEMName, "MSDOS5.0", 8);
    bpb->BPB_BytsPerSec = FAT32_SECTOR_SIZE;  // 00 02
    bpb->BPB_SecPerClus = 1;
    bpb->BPB_RsvdSecCnt = FAT32_BPB_RsvdSecCnt;//0x117C;   //2238KB
    bpb->BPB_NumFATs = 2;
    //bpb->BPB_RootEntCnt = 0x0000;
    //bpb->BPB_TotSec16 = 0x0000;
    bpb->BPB_Media = 0xF8;
    //bpb->BPB_FATSz16 = 0x0000;
    bpb->BPB_SecPerTrk = 0x003F;
    bpb->BPB_NumHeads = 0x00FF;
    bpb->BPB_HiddSec = 0x0000003F;
    bpb->BPB_TotSec32 = FAT32_BPB_FATSz32 + FAT32_BPB_FATSz32 + FAT32_BPB_RsvdSecCnt + 0x81;     //120MB
    if(flash){
    	bpb->BPB_TotSec32 += (flash->chip.capacity/FAT32_SECTOR_SIZE);
    }else{
    	bpb->BPB_TotSec32 += 0x0001000;
    }
    
    // FAT32 Structure
    bpb->BPB_FATSz32 = FAT32_BPB_FATSz32;
    bpb->BPB_ExtFlags = 0x0000;
    bpb->BPB_FSVer = 0x0000;
    bpb->BPB_RootClus = 0x00000002;
    bpb->BPB_FSInfo = 0x0001;
    bpb->BPB_BkBootSec = 0x0006;
    //bpb->BS_Reserved[12];
    bpb->BS_DrvNum = 0x80;
    //bpb->BS_Reserved1;
    bpb->BS_BootSig = 0x29;
    bpb->BS_VolID = 0x94B11E23;
    memcpy(bpb->BS_VolLab, "NO NAME    ", 11);
    memcpy(bpb->BS_FilSysType, "FAT32   ", 8);
    
    b[510] = 0x55;
    b[511] = 0xAA;
#endif
}

// Addr: 0x0000_0200 and 0x0000_0E00
static void _fat32_read_fsinfo(uint8_t *b)
{
    fat32_fsinfo_t *fsinfo = (fat32_fsinfo_t*)b;
    memset(b, 0, FAT32_SECTOR_SIZE);
    fsinfo->FSI_LeadSig = 0x41615252;
    fsinfo->FSI_StrucSig = 0x61417272;
    fsinfo->FSI_Free_Count = 0x000198BE; //0xFFFFFFFF;
    fsinfo->FSI_Nxt_Free = (FILE_README_ADDR/FAT32_SECTOR_SIZE)+2;
    b[510] = 0x55;
    b[511] = 0xAA;
}

// Addr: 0x0000_0400 and 0x0000_1000
static void _fat32_read_fsinfo2(uint8_t *b)
{
    memset(b, 0, FAT32_SECTOR_SIZE);
    b[510] = 0x55;
    b[511] = 0xAA;
}

// Addr: 0x0000_1800
// An operating system is missing.... BS_jmpBoot[1] = 0xFE (jmp $)
// No need to gen bootcode

// FAT32 table
// Addr: 0x0022_F800 - 0x0023_1814
// Addr: 0x0031_7C00 - 0x0031_9C14
static void _fat32_read_fat_table(uint8_t *b, uint32_t addr)
{
    uint32_t s_offset = (addr - FAT32_FAT1_ADDRESS) >> 2;
    uint32_t *b32 = (uint32_t*)b;
    uint32_t i;
    
    if(addr == FAT32_FAT1_ADDRESS)    // FAT table start
    {
        // 1MB
        b32[0] = 0x0FFFFFF8;
        b32[1] = 0xFFFFFFFF;
        b32[2] = 0x0FFFFFFF;
        b32[3] = 0x0FFFFFFF;
        b32[4] = 0x0FFFFFFF;
        
        for(i=5; i<128; i++)
        {
            b32[i] = s_offset + i +1;
        }
    }
//    else if(addr == FAT32_FAT1_ADDRESS+0x2000)    // FAT table end
//    {
//        for(i=0; i<128; i++)
//        {
//            b32[i] = 0x00000000;
//        }
//    }
    else
    {
        for(i=0; i<128; i++)
        {
            b32[i] = 0x00000000;//s_offset + i +1;
        }
    }
}

const char *readme = "STM32H750 USB MSC BOOTLOADER VERSION: 1.0.0";
// Addr: 0x0040_0000
static void _fat32_read_dir_entry(uint8_t *b)
{
    fat32_dir_entry_t *dir = (fat32_dir_entry_t*)b;
    memset(b, 0, FAT32_SECTOR_SIZE);
    
    memcpy(dir->DIR_Name, "BOOTLOADER ", 11);
    dir->DIR_Attr = FAT32_ATTR_VOLUME_ID;
    dir->DIR_NTRes = 0x00;
    dir->DIR_CrtTimeTenth = 0x00;
    dir->DIR_CrtTime = 0x0000;
    dir->DIR_CrtDate = 0x0000;
    dir->DIR_LstAccDate = 0x0000;
    dir->DIR_FstClusHI = 0x0000;
    dir->DIR_WrtTime = FAT32_MAKE_TIME(0,0);
    dir->DIR_WrtDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_FstClusLO = 0x0000;
    dir->DIR_FileSize = 0x00000000;
    
    ++dir;
    
    memcpy(dir->DIR_Name, "VERSION TXT", 11);
    dir->DIR_Attr = FAT32_ATTR_ARCHIVE;
    dir->DIR_NTRes = 0x18;
    dir->DIR_CrtTimeTenth = 0x00;
    dir->DIR_CrtTime = FAT32_MAKE_TIME(0,0);
    dir->DIR_CrtDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_LstAccDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_FstClusHI = 0x0000;
    dir->DIR_WrtTime = FAT32_MAKE_TIME(0,0);
    dir->DIR_WrtDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_FstClusLO = 0x0005;
    dir->DIR_FileSize = strlen(readme);
}

// Addr : 0x0040_0600
static void _fat32_read_readme(uint8_t *b, uint32_t addr)
{
    memcpy(b, readme, FAT32_SECTOR_SIZE);
}

static bool _fat32_write_firmware(const uint8_t *b, uint32_t addr)
{
    bool return_status = true;
  
    HAL_StatusTypeDef status = HAL_OK;
    
    uint32_t offset = addr - image_addr_range.begin;
    uint32_t phy_addr = APP_ADDR + offset;
    uint32_t prog_size = MIN(FAT32_SECTOR_SIZE, image_addr_range.end - image_addr_range.begin);
  
    if(addr == image_addr_range.begin)
    {
        // TODO erase flash
    }
    
    
    if((phy_addr >= APP_ADDR) && (phy_addr < (APP_ADDR + APP_SIZE))){
    	if(flash){
    		LL_GPIO_SetOutputPin(LED2_GPIO_Port, LED2_Pin);
    		if((offset % flash->chip.erase_gran) == 0){
    			sfud_erase(flash, offset, flash->chip.erase_gran);
    		}
    		sfud_write(flash, offset, prog_size, b);
    		LL_GPIO_ResetOutputPin(LED2_GPIO_Port, LED2_Pin);
    		image_addr_range.image_size -= prog_size;
    		if(image_addr_range.image_size <= 0){
    			LL_GPIO_SetOutputPin(LED3_GPIO_Port, LED3_Pin);
    		}
    	}
    }
    
    return return_status;
}

//-------------------------------------------------------

// sector size should be 512 byte

bool fat32_read(uint8_t *b, uint32_t addr)
{
    if(addr & (FAT32_SECTOR_SIZE - 1))      // if not align ?
    {
        return false;
    }
    
    if(addr == 0x0000 || addr == 0x0C00)
    {
        _fat32_read_bpb(b);
    }
    else if(addr == 0x0200 || addr == 0x0E00)
    {
        _fat32_read_fsinfo(b);
    }
    else if(addr == 0x0400 || addr == 0x1000)
    {
        _fat32_read_fsinfo2(b);
    }
    // BPB_RsvdSecCnt * 512 -> 17*512
//    else if((addr >= 0x22F800 && addr < 0x231A00) || (addr >= 0x317C00&& addr < 0x319E00))
    else if((addr >= FAT32_FAT1_ADDRESS && addr < FAT32_FAT1_ADDRESS+0x2800) || (addr >= FAT32_FAT2_ADDRESS && addr < FAT32_FAT2_ADDRESS+0x2800))
    {
        _fat32_read_fat_table(b, addr);
    }
    else if(addr == FAT32_DIR_ENTRY_ADDR)
    {
        _fat32_read_dir_entry(b);
    }
    else if(addr == FILE_README_ADDR)
    {
    	_fat32_read_readme(b, addr);
    }
    else
    {
        memset(b, 0x00, FAT32_SECTOR_SIZE);
    }
    
    return true;
}

bool fat32_write(const uint8_t *b, uint32_t addr)
{
    if(addr & (FAT32_SECTOR_SIZE - 1))      // if not align ?
    {
        return false;
    }
    
    uint32_t align_addr_end = (image_addr_range.end & ~(FAT32_SECTOR_SIZE-1)) + ((image_addr_range.end & (FAT32_SECTOR_SIZE-1)) ? FAT32_SECTOR_SIZE : 0);
    
    if(addr < FAT32_DIR_ENTRY_ADDR)
    {
      // No operation
    }
    else if(addr == FAT32_DIR_ENTRY_ADDR)
    {
        uint32_t i;
        for(i=0; i<FAT32_SECTOR_SIZE; i+= sizeof(fat32_dir_entry_t))
        {
            const uint8_t *b_offset = (const uint8_t *)(b + i);
            fat32_dir_entry_t *entry = (fat32_dir_entry_t*)b_offset;
            uint32_t clus = (((uint32_t)(entry->DIR_FstClusHI)) << 16) | entry->DIR_FstClusLO;
             
            if(!memcmp((void*) &entry->DIR_Name[8], "BIN", 3) || !memcmp((void*) &entry->DIR_Name[0], "FIRMWAREBIN", 11)){

            	image_addr_range.begin = ((clus-2) + (FAT32_BPB_RsvdSecCnt + FAT32_BPB_FATSz32*2 ) ) * FAT32_SECTOR_SIZE;
            	image_addr_range.end = image_addr_range.begin + MIN(entry->DIR_FileSize, APP_SIZE);
            	image_addr_range.image_size = MIN(entry->DIR_FileSize, APP_SIZE);

            	image_addr_range.type = FW_TYPE_BIN;
//            	printf("FIRMWARE [%.11s] -> %08X\n",entry->DIR_Name,image_addr_range.begin);
            }else if(!memcmp((void*) &entry->DIR_Name[8], "DAT", 3)){
            	image_addr_range.begin = ((clus-2) + (FAT32_BPB_RsvdSecCnt + FAT32_BPB_FATSz32*2 ) ) * FAT32_SECTOR_SIZE;
            	image_addr_range.end = image_addr_range.begin + MIN(entry->DIR_FileSize, APP_SIZE);
            	image_addr_range.image_size = MIN(entry->DIR_FileSize, APP_SIZE);

            	image_addr_range.type = FW_TYPE_DATA;
//            	printf("DATA [%.11s] -> %08X\n",entry->DIR_Name,image_addr_range.begin);
            }else if(!memcmp((void*) &entry->DIR_Name[0], "ERASEALL   ", 11)){
//            	image_addr_range.begin = ((clus-2) + (FAT32_BPB_RsvdSecCnt + FAT32_BPB_FATSz32*2 ) ) * FAT32_SECTOR_SIZE;
//            	printf("ERASE ALL DATA -> %08X\n",i);
            	flash?sfud_chip_erase(flash):0;
            }

        }
    }
    else if(addr >= image_addr_range.begin && addr < align_addr_end)
    {
        if(!_fat32_write_firmware(b, addr))
        {
            return false;
        }
    }
    else
    {
        volatile uint8_t halt = 1;

    }
    
    return true;
}

bool fat32_init(void)
{
	uint8_t buf[512];

    image_addr_range.begin = 0x292200;
    image_addr_range.end = 0x292200 + APP_SIZE;

	if(sfud_init() == SFUD_SUCCESS){
		flash = sfud_get_device_table();

		LL_GPIO_SetOutputPin(LED1_GPIO_Port, LED1_Pin);
	}

    return true;
}
