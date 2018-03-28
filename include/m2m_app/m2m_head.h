#ifndef M2M_APP_HEAD_H
#define M2M_APP_HEAD_H

//#include <assis.h>
#include <cfg_def.h>
#include <cyg/kernel/kapi.h>
#include <stdio.h>
#include <version.h>
//#include <cli_cmd.h>
#include <cyg/hal/plf_io.h>
#include <cyg/io/serialio.h>
#include <cyg/io/io.h>
#include <cyg/infra/diag.h>
#include <stdlib.h>
#include <string.h>
#include <flash.h>
#include <cyg/io/flash.h>
#include <network.h>
#include <pkgconf/system.h>
#include <pkgconf/net.h>
#include <cyg/infra/testcase.h>
#include <cfg_net.h>
#include <assis.h>
#include <hsf.h>


#include "m2m_debug.h"

//#define printf( _fmt ) M2M_DEBUG((_fmt))


extern int ap_connected;
extern int apcli_connected2;
extern int apcli_connected;
extern int is_apcli_enable(void);

#define M2M_STATE_STOP 					0
#define M2M_STATE_RUN_CMD 				1
#define M2M_STATE_RUN_THROUGH 			2
#define M2M_STATE_THROUGH_OUT_CHECKING 	3
#define M2M_STATE_RUN_AGREEMENT 			4
#define M2M_STATE_AGREE_OUT_CHECKING 	5
#define M2M_STATE_RUN_GPIO1			 	6
#define M2M_STATE_RUN_GPIO2			 	7
int m2m_app_state;
cyg_mutex_t m2m_app_state_mutex;
int m2m_bsleep;

cyg_mutex_t TempString_mutex;
cyg_mutex_t m2m_uart_write_mutex;

#define M2M_UART_TRANSMODE_FREE	1
#define M2M_UART_TRANSMODE_FRAME	0
//int m2m_uart_autoframe;

#define M2M_MAXNETFRAME 4*1024
//char m2m_net_recv[M2M_MAXNETFRAME];
char *m2m_net_recv;

int m2m_btcpconnector;
int m2m_tcpdisconnect;

int m2m_telnet_open;
int m2m_fudlx;

#define BITS(_b,_n,_v)				(((_v)&((1<<(_n))-1))<<((_b+1)-(_n)))
#define RD_BITS(_b,_n,_v)			(((_v)>>((_b+1)-(_n)))&((1<<(_n))-1))
#define BIT(_v)						((1)<<(_v))

#define M2M_FNRDY_NORMAL	0
#define M2M_FNRDY_BEAT		1
#define M2M_FNRDY_BEATF	2
#define M2M_FNRDY_BEATVF	3
int m2m_fnrdy;

#define DEFAULT_COM_PORT			0		//0: for "/dev/ser0", 1: for "/dev/ser1"
#define UART_TRANSMODE_FREE		1
#define UART_TRANSMODE_FRAME		0
enum ENBAUDRATE
{
	BAUDRATE_50							= 0x00,
	BAUDRATE_75,
	BAUDRATE_110,
	BAUDRATE_134,
	BAUDRATE_150,
	BAUDRATE_200,
	BAUDRATE_300,
	BAUDRATE_600,
	BAUDRATE_1200							= 0x08,
	BAUDRATE_1800,
	BAUDRATE_2400							= 0x0a,
	BAUDRATE_4800,
	BAUDRATE_9600,
	BAUDRATE_19200,
	BAUDRATE_38400,
	BAUDRATE_57600,
	BAUDRATE_115200,
	BAUDRATE_230400,
	BAUDRATE_345600,
	BAUDRATE_460800
};

enum ENCOMBITS
{
	COMBITS_5							= 0x00,
	COMBITS_6,
	COMBITS_7,
	COMBITS_8
};

enum ENCOMDEVICE
{
	COMDEVICE_COM0					= 0x00,
	COMDEVICE_COM1,
};

enum ENCOMPARITY
{
	COMPARITY_NONE					= 0x00,
	COMPARITY_ODD,
	COMPARITY_EVEN,
	COMPARITY_MARK,
	COMPARITY_SPACE
};

enum ENCOMSTOPBITS
{
	COMSTOPBITS_1					= 0x00,
	//COMSTOPBITS_1P5,
	COMSTOPBITS_2
};

struct _COM
{
	char device;		//COM1: 0, COM2: 1, ....
	char baudrate;
	char data_bits;	//5, 6,7, 8
	char parity;		//none: 0, odd: 1, even: 2, mark: 3, space: 4
	char stop_bits;	// 1: 0, 1.5: 1, 2: 2
	char ctsrts; 		// 0: not ctsrts, 1: need ctsrts, V2.30
	int adj_time;
};
struct _COM strdevicecom;
int m2m_comset_autoframe;
int m2m_comset_ftime;			// unit is 100ms
int m2m_comset_flen;

enum ENNETSETAPP
{
	NETSETAPP_SERVER		=0x00,
	NETSETAPP_CLIENT
};

enum ENNETSETPRO
{
	NETSETPRO_TCP		=0x00,
	NETSETPRO_UDP
};

enum ENNETSETADD
{
	NETSETADD_HOST		=0x00,
	NETSETADD_IP
};

struct _NETSET
{
	char app_mode;		//server mode: 0, client mode: 1
	char protocol;			//TCP: 0, UDP: 1
	unsigned short pro_port;			//port number
	unsigned char ipadd[4];		//ip address: ipadd[0]:ipadd[1]:ipadd[2]:ipadd[3]
	char bip;				// 1: is IP, 0: is host name
	char cipadd[20];
	int max_sk;
	int tcpto;
};
struct _NETSET strnetset;
char m2m_nethosename[100];

int m2m_init_uart_port(void);
int m2m_net_read(int fd, char *ptr, int nbytes);
int m2m_net_write(int fd, char *ptr, int nbytes);
int m2m_cfg_get(char *field, char *val);
int m2m_cfg_set(char *field, char *val);
int m2m_net_udp_read(int fd, char *ptr, int nbytes);
int m2m_net_udp_write(int fd, char *ptr, int nbytes);
void m2m_cmd_reply(char *reply);
void m2m_do_reboot(void);

void m2m_at_cmd_write(char *reply,int len,int timeouts,int flags);
int m2m_uart_write(cyg_io_handle_t fd, char *buf, int len);


#endif

