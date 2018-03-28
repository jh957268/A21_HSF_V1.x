
/* hfflash.h
 *
 * Copyright (C) 2013 High-flying Electronics Technology Co.,Ltd.
 *
 * This file is part of HSF.
 *
 */

#ifndef _HF_FLASH_H_H_H_
#define _HF_FLASH_H_H_H_


int HSF_API hfuflash_erase_page(uint32_t addr, int pages);

int HSF_API hfuflash_write(uint32_t addr, char *data, int len);

int HSF_API hfuflash_read(uint32_t addr, char *data, int len);

int HSF_IAPI hfflash_copy(uint32_t dstaddr,uint32_t srcaddr,uint32_t len);

int  flash_write(unsigned int addr, char *data, int len, int no_used);
int  flash_erase_page(unsigned int addr, int pages);
int  flash_read(unsigned int addr, char *data, int len, int no_used);

#define HFFLASH_PAGE_SIZE			(4*1024)



extern int flsh_read(unsigned int addr, unsigned int to, unsigned int len);
extern int flsh_erase(unsigned int faddr, unsigned int len);
extern int flsh_write(unsigned int faddr, unsigned int from, unsigned int len);
extern int flsh_init(void);




#endif

