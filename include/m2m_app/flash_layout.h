
#ifndef _FLASH_LAYOUT_H_H_
#define _FLASH_LAYOUT_H_H_

#define FLASH_CFG_OFFSET			(0x31000)


#define FLASH_FWM_OFFSET			(0x50000)

#if CONFIG_XROUTER_FLASH_SIZE == 1024


#ifdef CONFIG_FIX_PROFILE_IN_FLASH

#define FLASH_FWM_MAX_SIZE		(704*1024)

#define F_SETTING_START			0x20000//0xF0000
#define F_SETTING_LEN				4096

#define CONFIG_HF_SETTING_START	0x20000
#define CONFIG_HF_SETTING_LEN		64*1024

#define CONFIG_SETTING_BAK_START	0x24000 //menghui
#define CONFIG_SETTING_BAK_LEN	16*1024 //menghui

#define SSL_CERT_OFFSET			0x28000
#define SSL_CERT_MAX_SIZE			16*1024
#define SSL_CERT_HDR_SIZE			32

#define PROFILE_OFFSET				0x2C000
#define MAX_PROFILE_SIZE			12*1024
#define PROFILE_HDR_SIZE			32

//#define USER_BIN_FILE_OFFSET					0x2F000
//#define USER_BIN_FILE_SIZE						4*1024
//#define USER_BIN_FILE_BAK_OFFSET				0x30000
//#define MODULE_DIFFERENT_ADDR					0x0

#else
#error must open the macro CONFIG_FIX_PROFILE_IN_FLASH
#endif

#elif CONFIG_XROUTER_FLASH_SIZE == 2048

#ifdef CONFIG_FIX_PROFILE_IN_FLASH

#define FLASH_FWM_MAX_SIZE		(832*1024)

#define F_SETTING_START			0x20000//0xF0000
#define F_SETTING_LEN				4096

#define CONFIG_HF_SETTING_START	0x20000
#define CONFIG_HF_SETTING_LEN		64*1024

#define CONFIG_SETTING_BAK_START	0x24000 //menghui
#define CONFIG_SETTING_BAK_LEN	16*1024 //menghui

#define SSL_CERT_OFFSET			0x28000
#define SSL_CERT_MAX_SIZE			16*1024
#define SSL_CERT_HDR_SIZE			32

#define PROFILE_OFFSET				0x2C000
#define MAX_PROFILE_SIZE			12*1024
#define PROFILE_HDR_SIZE			32

/*user zone start*/
#define USER_BIN_FILE_OFFSET					0x120000
#define USER_BIN_FILE_SIZE						4*1024
#define USER_BIN_FILE_BAK_OFFSET				0x121000
//#define MODULE_DIFFERENT_ADDR					0x152000

#define UFLASH_ADDRESS							0x122000
#define HFUFLASH_SIZE							(56*1024)
//#define HFUFLASH1_SIZE							(100*1024)
//#define UFLASH1_ADDRESS						0x150000

/*user zone end*/

#else
#error must open the macro CONFIG_FIX_PROFILE_IN_FLASH
#endif

#elif CONFIG_XROUTER_FLASH_SIZE == 4096

#ifdef CONFIG_FIX_PROFILE_IN_FLASH

#define FLASH_FWM_MAX_SIZE		(1344*1024)

#define F_SETTING_START			0x20000//0xF0000
#define F_SETTING_LEN				4096

#define CONFIG_HF_SETTING_START	0x20000
#define CONFIG_HF_SETTING_LEN		64*1024

#define CONFIG_SETTING_BAK_START	0x24000 //menghui
#define CONFIG_SETTING_BAK_LEN	16*1024 //menghui

#define SSL_CERT_OFFSET			0x28000
#define SSL_CERT_MAX_SIZE			16*1024
#define SSL_CERT_HDR_SIZE			32

#define PROFILE_OFFSET				0x2C000
#define MAX_PROFILE_SIZE			12*1024
#define PROFILE_HDR_SIZE			32

#define USER_BIN_FILE_OFFSET					0x1A0000
#define USER_BIN_FILE_SIZE						4*1024
#define USER_BIN_FILE_BAK_OFFSET				0x1A1000
//#define MODULE_DIFFERENT_ADDR					0x

#define UFLASH_ADDRESS							0x1A2000
#define HFUFLASH_SIZE							(1080*1024)
//#define HFUFLASH1_SIZE							(100*1024)
//#define UFLASH1_ADDRESS						0x200000


#else
#error must open the macro CONFIG_FIX_PROFILE_IN_FLASH
#endif

#else
#error flash size only support 1024KB, 2048KB,4096KB
#endif

#endif

