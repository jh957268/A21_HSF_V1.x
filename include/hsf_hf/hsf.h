
/* hsf.h
 *
 * Copyright (C) 2013 ShangHai High-flying Electronics Technology Co.,Ltd.
 *
 * This file is part of HSF.
 *
 */


#ifndef _HSF_H_H_H_H_H_
#define _HSF_H_H_H_H_H_


#if 0
#include "hfconfig.h"

#ifdef _ATMEL_ASF_
#include <asf.h>
#endif

#if defined   ( __CC_ARM   )
#	define USER_FUNC	//__attribute__ ((section(".appmain")))
#	define HSF_API		//__attribute__ ((section(".appmain")))
#elif defined (  __GNUC__  )
#   define USER_FUNC __attribute__((section(".appmain")))
#	define HSF_API		__attribute__ ((section(".appmain")))
#else
#define USER_FUNC
#endif

#define HSF_IAPI		HSF_API

#define _HSF_INLINE_	inline

#ifndef bzero
#define bzero(_ptr,_size)	memset(_ptr,0,_size)
#endif

#ifdef __BUILD_HSF_SDK__

#ifdef CONFIG_CHIP_HFO18
#include <os/os.h>
#include "driver/gpio.h"
#endif

#ifdef CONFIG_FREERTOS
#include <FreeRTOS.h>

						#ifndef NORMAL_FREERTOS
#define pvPortMalloc(size) OSMalloc(size, MMM_ALLOC_NORMAL)
#define vPortFree(ptr) OSFree(ptr)
						#endif
						
#endif

#endif

#ifndef FD_SET
  #undef  FD_SETSIZE
  /* Make FD_SETSIZE match NUM_SOCKETS in socket.c */
  #define FD_SETSIZE    24
  #define FD_SET(n, p)  ((p)->fd_bits[(n)/8] |=  (1 << ((n) & 7)))
  #define FD_CLR(n, p)  ((p)->fd_bits[(n)/8] &= ~(1 << ((n) & 7)))
  #define FD_ISSET(n,p) ((p)->fd_bits[(n)/8] &   (1 << ((n) & 7)))
  #define FD_ZERO(p)    memset((void*)(p),0,sizeof(*(p)))

  typedef struct fd_set {
          unsigned char fd_bits [(FD_SETSIZE+7)/8];
        } fd_set;

#endif /* FD_SET */

#endif

#define USER_FUNC	//__attribute__ ((section(".appmain")))
#define HSF_API		//__attribute__ ((section(".appmain")))

#define HSF_IAPI		HSF_API

#define _HSF_INLINE_	inline



#include <m2m_head.h>

#include "hftypes.h"
#include "hfsys.h"
#include "hferrno.h"
#include "hf_debug.h"
#include "hfat.h"
#include "hfgpio.h"
#include "hfspi.h"
#include "hfthread.h"
#include "hfnet.h"
#include "hfuart.h"
#include "hfusb.h"
#include "hftimer.h"
#include "hftime.h"
#include "hffile.h"
#include "hfupdate.h"
#include "hfflash.h"
#include <flash_layout.h>
#include <hfir.h>
#include <hfwifi.h>
#include <hfsmtlk.h>


#ifdef __LPB100U__
#include <hfusbhosthcd.h>
#include <hfaudio.h>
#endif

#endif


