////////////////////////////////////////////////////////////////////////////////
//                   Mountain View Silicon Tech. Inc.
//		Copyright 2011, Mountain View Silicon Tech. Inc., ShangHai, China
//                   All rights reserved.
//
//		Filename	:types.h
//		Description	:re-define language C basic data type for porting conveniently
//						over platforms
//		Changelog	:	
//					2012-02-21	Create basic data type re-definition for uniform 
//						platform by Robert
///////////////////////////////////////////////////////////////////////////////

#ifndef		_TYPES_H_
#define		_TYPES_H_
#if 0
/*
 * 0-bit
 */
#define VOID void

#ifndef	NULL
#define	NULL			((VOID*)0)
#define	null			(NULL)
#endif	//NULL
typedef	VOID(*FPCALLBACK)(VOID);
/*
 * 1-bit(boolean type)
 */
#define false           (0)
#define true            (1)
#define	FALSE			(0)
#define	TRUE			(1)

#ifndef _MSC_VER
typedef unsigned char   BOOL;
typedef unsigned char   bool;
#else
typedef int   BOOL;
#endif
#endif
/*
 * 8-bit
 */
typedef signed char				SBYTE;
typedef unsigned char			BYTE;
/*
 * 16-bit
 */
typedef signed short			SWORD;
typedef unsigned short			WORD;
/*
 * 32-bit
 */
#ifndef _MSC_VER
typedef signed int				SDWORD;
typedef unsigned int			DWORD;
#else
typedef signed long				SDWORD;
typedef unsigned long			DWORD;
#endif
//typedef	float					FLOAT;
/*
 * 64-bit(for exfat only)
 */
//typedef	double float			DFLOAT;
typedef	unsigned long long int	DLONG;
typedef	signed long long int	SDLONG;


//typedef   signed char		int8_t;
//typedef   signed short int	int16_t;
//typedef   signed int		int32_t;
//typedef   signed			int64_t;

//typedef unsigned char		uint8_t;
//typedef unsigned short int	uint16_t;
//typedef unsigned int		uint32_t;
//typedef unsigned			uint64_t;
//typedef	unsigned int		size_t;


#endif	//__TYPE_H__

