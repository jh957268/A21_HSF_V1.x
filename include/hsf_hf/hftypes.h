
#ifndef _HF_TYPES_H_H_
#define _HF_TYPES_H_H_




#if 0


#ifdef __KEIL_MDK__
	#ifdef __BUILD_HSF_SDK__
#include <common/type.h>
	#else
#define false           (0)
#define true            (1)
#define	FALSE			(0)
#define	TRUE			(1)

typedef unsigned char   BOOL;
typedef unsigned char   bool;
typedef	unsigned int	size_t;
	#endif
#include <stdint.h>

#endif



//#define uint32_t unsigned int
//#define uint16_t unsigned short
//#define uint8_t unsigned char
//#define size_t unsigned int
//#define int32_t  int
//#define bool  unsigned char

#endif

#if 0
#ifndef uint32_t
#define uint32_t unsigned int
#endif

#ifndef uint16_t
#define uint16_t unsigned short
#endif

#ifndef uint8_t
#define uint8_t unsigned char
#endif

#ifndef size_t
#define size_t unsigned int
#endif

#endif

typedef  struct in_addr ip_addr_t;


#endif


