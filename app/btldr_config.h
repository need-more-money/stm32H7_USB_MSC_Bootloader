/*************************************************************************************
# Released under MIT License

Copyright (c) 2020 SF Yip (yipxxx@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************/

#ifndef _BTLDR_CONFIG_H_
#define _BTLDR_CONFIG_H_

#define DEV_CODE_ADDR           FLASH_BASE
#define DEV_FLASH_SIZE          FLASH_SIZE
#define DEV_ERASE_PAGE_SIZE     FLASH_PAGE_SIZE

#define APP_ADDR                (DEV_CODE_ADDR + APP_OFFSET)
#define APP_SIZE                (DEV_FLASH_SIZE - APP_OFFSET)

#define CONFIG_READ_FLASH       1U


#define APP_OFFSET							0x5000			//bootloader size (byte)



#endif
