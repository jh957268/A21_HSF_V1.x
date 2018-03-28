

/* hffile.h
 *
 * Copyright (C) 2013-2020 ShangHai High-flying Electronics Technology Co.,Ltd.
 *				  	All rights reserved.
 *
 * This file is part of HSF.
 *
 *
 */

#ifndef _H_H_HFFILE_H_
#define _H_H_HFFILE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void * HFFILE;

//current postion of the file
#define HFFILE_SEEK_CUR		0

//begin of the file
#define HFFILE_SEEK_SET		1

//end of the file
//#define HFFILE_SEEK_END	1

int HSF_API  hffile_userbin_write(uint32_t offset,char *data,int len);
int HSF_API  hffile_userbin_read(uint32_t offset,char *data,int len);
int HSF_API hffile_userbin_size(void);
int HSF_API hffile_userbin_zero(void);
int HSF_IAPI hffile_fs_init(void);
int HSF_API hffile_seek(  HFFILE *file,long offset,int origin);
int HSF_API hffile_zero(HFFILE *file);
int HSF_API hffile_eof(HFFILE *file);
int HSF_API hffile_pos(HFFILE *file);
int HSF_API hffile_size(HFFILE *file);
int HSF_API hffile_write(const void *data,size_t size,size_t count,HFFILE *file);
int  HSF_API hffile_read(  void *buffer,size_t size,size_t count,HFFILE *file);
int HSF_API hffile_close(HFFILE *file);
HFFILE  *HSF_API hffile_open(const char *name,const char *mode);


#ifdef __cplusplus
}
#endif

#endif /* HFSYS_H_ */
