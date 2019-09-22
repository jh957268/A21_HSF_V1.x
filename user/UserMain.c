
#include <sys/types.h>
#include <network.h>
#include <stdio.h>
#include <netinet/if_ether.h>
#include <sys/mbuf.h>
#include <sys/types.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>
#include <ifaddrs.h>
#include <cyg/io/eth/eth_drv_stats.h>
#include <cyg/io/eth/eth_drv.h>
#include <cyg/io/eth/netdev.h>

#include <config.h>
#include <http_proc.h>
#include <http_conf.h>
#include <cfg_net.h>
#include <sys_status.h>
#include <net/route.h>
#include <net/if.h>
#include <stdlib.h>

#include <cgi_api.h>
#include <m2m_debug.h>
#include <m2m_app.h>
#include <hsf.h>

#include "aks_version.h"
#include "e131.h"
#include "aks_debug_printf.h"

//#define SSID_STR 	"MySpectrumWiFi60-2G"
#define SSID_STR 	"Ratpac AKS"

void Joo_uart_send(char *data);

static void server_thread_main(void* arg);

USER_FUNC static void client_thread_main(void* arg);
void sACN_main(void *arg); 

void check_gaffer_packet(char *data, uint32_t len);
void replace_channel_data(char *data, uint32_t len);
int check_duplicate_ip(char *ip_addr);
int ratpac_get_str(int id, char *val);
int ratpac_set_str(int id, char *val);
int Send_Battery_Command(void);
void age_client_list(int new);
void populate_new_client(char *ip_ad, char *rcv_msg);
void save_valid_client_entry(int i);

void send_artpollreply_msg(void);

static int refresh_clients;
static char gaffer_data[514];

cyg_mutex_t samd_mutex;

extern struct eth_drv_sc devive_wireless_sc0;

int rt28xx_get_wifi_channel(void *dev);

struct eth_drv_sc	*dev;
//RTMP_ADAPTER	*pAd;

#define AP_MODE 		1
#define STA_ETH_MODE	2
#define STA_WIFI_MODE	3

#define AKS_PRIORITIES	8			// since the web server uses 8, the UserMain thread need to change 8 too

#define MAGIC0			0xab
#define MAGIC1			0x12

static int operation_mode;

// #define debug(format, args...) fprintf (stderr, format, args)

#if 0
#define eprintf(fmt, args...) do {
							sprintf (ebuffer, fmt, args);
							Joo_uart_send((char *)ebuffer);
						}

#define E_DEBUG_PRINT	0

#if E_DEBUG_PRINT
char ebuffer[128];
#define eprintf(fmt, ...) do {  \
	                               sprintf(ebuffer, fmt, ## __VA_ARGS__); \
	                               Joo_uart_send((char *)ebuffer); \                               
	                             } while (0)
#else
#define eprintf(fmt, ...)
#endif

#endif
	
char at_rsp[96] = {0};
char samd_ver[4] = {'1', '.', 'x', 0};
char timo_ver[4] = {'1', '.', 'y', 0};
char battery_info[32] = {0x31, 0};
volatile char ipAddress[4]={};
static char at_scan_rsp[1024];

static char ap_wln_channel = 0;

extern cyg_netdevtab_entry_t devive_wireless_netdev0;
cyg_netdevtab_entry_t *pWIFIDev;

//char *rel_date = __DATE__" " __TIME__;
//char *eCos_ver="1.01";

const int hf_gpio_fid_to_pid_map_table[HFM_MAX_FUNC_CODE]=
{
	HFM_NOPIN,		//0HFGPIO_F_JTAG_TCK
	HFM_NOPIN,		//1HFGPIO_F_JTAG_TDO
	HFM_NOPIN,		//2HFGPIO_F_JTAG_TDI
	HFM_NOPIN,		//3HFGPIO_F_JTAG_TMS
	HF_M_PIN(12),	//4HFGPIO_F_USBDP
	HF_M_PIN(13),	//5HFGPIO_F_USBDM
	HF_M_PIN(4),	//6HFGPIO_F_UART0_TX
	HFM_NOPIN,		//7HFGPIO_F_UART0_RTS
	HF_M_PIN(5),	//8HFGPIO_F_UART0_RX
	HFM_NOPIN,		//9HFGPIO_F_UART0_CTS
	
	HFM_NOPIN,		//10HFGPIO_F_SPI_MISO
	HFM_NOPIN,		//11HFGPIO_F_SPI_CLK
	HFM_NOPIN,		//12HFGPIO_F_SPI_CS
	HFM_NOPIN,		//13HFGPIO_F_SPI_MOSI
	
	HF_M_PIN(22),	//14HFGPIO_F_UART1_TX,
	HFM_NOPIN,		//15HFGPIO_F_UART1_RTS,
	HF_M_PIN(21),	//16HFGPIO_F_UART1_RX,
	HFM_NOPIN,		//17HFGPIO_F_UART1_CTS,
	
	HF_M_PIN(14),	//18HFGPIO_F_NLINK
	HF_M_PIN(16),	//19HFGPIO_F_NREADY
	HF_M_PIN(17),	//20HFGPIO_F_NRELOAD
	HFM_NOPIN,		//21HFGPIO_F_SLEEP_RQ
	HFM_NOPIN,		//22HFGPIO_F_SLEEP_ON
		
	HFM_NOPIN,		//23HFGPIO_F_WPS,
	HFM_NOPIN,		//24HFGPIO_F_IR
	HF_M_PIN(6),	//25HFGPIO_F_GPIO_0
	HF_M_PIN(19),	//26HFGPIO_F_I2C_SD
	HF_M_PIN(20),	//27HFGPIO_F_I2C_SCLK
	HF_M_PIN(23),	//28HFGPIO_F_GPIO_1
	
	HFM_NOPIN,		//29HFGPIO_F_USER_DEFINE
};

typedef	struct user_cgi_cmd_t
{
	char * cmd;
	int (*func)(http_req *req);
} user_cgi_cmd;

user_cgi_cmd user_ps_cgi_cmds[]=
{
};
int CGi_do_user_cmd(http_req *req)
{
	char *cmd;
	int i;
	int rc=0;
	
	cmd=WEB_query(req,"CMD");
	
	if (cmd)
	{
		for (i=0; i< (sizeof(user_ps_cgi_cmds)/sizeof(user_cgi_cmd)) ; i++)
		{
			if (!strcmp(user_ps_cgi_cmds[i].cmd, cmd))
			{
				if (user_ps_cgi_cmds[i].func)
				{
					rc=(user_ps_cgi_cmds[i].func)(req);
					return rc;
				}
			}
		}
	}
	
	return 0;
}

/******************hiflying add start ¡ý*******************/

#define WEB_DATA_FLASH_ADDR		(0)
#define WEB_DATA_FLASH_PAGE		(1) //1page = 4096Bytes

typedef struct _web_data
{
	unsigned char initFlag;
	unsigned char name[20];
	unsigned char universe[3];
	unsigned char subnet[3];
	unsigned char timopower[3];
	unsigned char channelWidth[3];
	unsigned char secondChannel[3];
	unsigned char bitSetting[3];
	unsigned char gaffer_enb[2];
	unsigned char gaffer_sub[3];
	unsigned char gaffer_univ[3];
	unsigned char gaffer_lower[4];
	unsigned char gaffer_upper[4];
	unsigned char sacn_enb[2];
	unsigned char sacn_universe[8];
	unsigned char protname[2];
	unsigned char led_color[8];
	unsigned char led_g[8];
	unsigned char led_b[8];	
	unsigned char use_xlr[2];
	unsigned char main_xlr[2];
	unsigned char sort_by[4];
	unsigned char magic[2];
}WEB_DATA_T;

WEB_DATA_T g_web_config = {0};


/*AT CMD start ¡ý*/

void write_data_to_flash(void)
{
	int i;
	unsigned char ck_sum = 0;
	unsigned char *fls_data;
	
	fls_data = (unsigned char *)&g_web_config;
	for (i = 1; i < sizeof(WEB_DATA_T); i++)
	{
		if (0xff ==  fls_data[i])
			continue;
		ck_sum ^= fls_data[i];
	}
	g_web_config.initFlag = ck_sum;
	hfuflash_erase_page(WEB_DATA_FLASH_ADDR,WEB_DATA_FLASH_PAGE);
	hfuflash_write(WEB_DATA_FLASH_ADDR,&g_web_config,sizeof(WEB_DATA_T));
}
static int USER_FUNC cmd_web_para_node_name(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int ret = 0;
	char temp_buf[100] = {0};

	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		ratpac_get_str( CFG_str2id("AKS_NAME"), temp_buf);
		sprintf(rsp, "=%s", temp_buf);
	}
	else
	{
		ratpac_set_str( CFG_str2id("AKS_NAME"), argv[0]);
		write_data_to_flash();
		//CFG_save(0);
	}
	return 0;
}

int artnet_enable = 0;

static int USER_FUNC cmd_web_para_artnet(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int ret = 0;
	char *input;
	
	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		sprintf(rsp, "=%d", artnet_enable);
	}
	else
	{
		input = argv[0];
		artnet_enable = input[0] - '0';
	}
	
	//refresh_clients = 1;

	return 0;
}

static int USER_FUNC cmd_web_para_rd(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int ret = 0;
	char *input;
	unsigned int addr, val, val1;
	int chn = 0;
	
	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		sprintf(rsp, "=%d", artnet_enable);
	}
	else
	{
		input = argv[0];
		if ( 0 == sscanf( argv[ 0 ], "%x", &addr ) )
		{
			eprintf( "No valid addr input\n" );
			return ( -1 );
		}
		eprintf("Enter %08x\n", addr);
		hf_thread_delay(500);
		val = *((unsigned int *)addr);
		val1 = *((unsigned int *)(addr+4));
		
		//chn = rt28xx_get_wifi_channel((void *)&devive_wireless_netdev0);
		chn = rt28xx_get_wifi_channel((void *)addr);
		sprintf(rsp, "val = %08x, val1 = %08x, AP Channel = %d\n", val, val1, chn);	
	}
	
	//refresh_clients = 1;

	return 0;
}

static int USER_FUNC cmd_web_para_universe(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int ret = 0;
	char temp_buf[100] = {0};
	int temp_value = -1;

	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		ratpac_get_str( CFG_str2id("AKS_UNIVERSE"), temp_buf);
		sprintf(rsp, "=%s", temp_buf);
	}
	else
	{
		temp_value = atoi(argv[0]);
		printf("universe_value=%s_%d\n",argv[0],temp_value);
		if((temp_value>=0)&&(temp_value<=9))
		{
			ratpac_set_str( CFG_str2id("AKS_UNIVERSE"), argv[0]);
			write_data_to_flash();
			//CFG_save(0);
		}
		else
		{
			return -1;
		}
	}

	return 0;
}
static int USER_FUNC cmd_web_para_timopower(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int ret = 0;
	char temp_buf[100] = {0};
	int temp_value = -1;

	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		ratpac_get_str( CFG_str2id("TIMO_POWER"), temp_buf);
		sprintf(rsp, "=%s", temp_buf);
	}
	else
	{
		temp_value = atoi(argv[0]);
		printf("TIMO_POWER=%s_%d\n",argv[0],temp_value);
		if((temp_value>=0)&&(temp_value<=5))
		{
			ratpac_set_str( CFG_str2id("TIMO_POWER"), argv[0]);
			write_data_to_flash();
			//CFG_save(0);
		}
		else
		{
			return -1;
		}
	}

	return 0;
}

static int USER_FUNC cmd_web_para_channelwidth(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int ret = 0;
	char temp_buf[100] = {0};
	int temp_value = -1;

	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		ratpac_get_str( CFG_str2id("AKS_CHANNEL_WIDTH"), temp_buf);
		sprintf(rsp, "=%s", temp_buf);
	}
	else
	{
		temp_value = atoi(argv[0]);
		printf("AKS_CHANNEL_WIDTH=%s_%d\n",argv[0],temp_value);
		if((temp_value>=9)&&(temp_value<=18))
		{
			ratpac_set_str( CFG_str2id("AKS_CHANNEL_WIDTH"), argv[0]);
			write_data_to_flash();
			//CFG_save(0);
		}
		else
		{
			return -1;
		}
	}

	return 0;
}


static int USER_FUNC cmd_web_para_secondchannel(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int ret = 0;
	char temp_buf[100] = {0};
	int temp_value = -1;

	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		ratpac_get_str( CFG_str2id("AKS_SECOND_CHANNEL"), temp_buf);
		sprintf(rsp, "=%s", temp_buf);
	}
	else
	{
		temp_value = atoi(argv[0]);
		printf("AKS_SECOND_CHANNEL=%s_%d\n",argv[0],temp_value);
		if((temp_value>=0)&&(temp_value<=11))
		{
			ratpac_set_str( CFG_str2id("AKS_SECOND_CHANNEL"), argv[0]);
			write_data_to_flash();
			//CFG_save(0);
		}
		else
		{
			return -1;
		}
	}

	return 0;
}


static int USER_FUNC cmd_web_para_bitsetting(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int ret = 0;
	char temp_buf[100] = {0};
	int temp_value = -1;

	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		ratpac_get_str( CFG_str2id("AKS_BIT_SETTINGS"), temp_buf);
		sprintf(rsp, "=%s", temp_buf);
	}
	else
	{
		temp_value = atoi(argv[0]);
		printf("AKS_BIT_SETTINGS=%s_%d\n",argv[0],temp_value);
		if((temp_value>=0)&&(temp_value<=1))
		{
			ratpac_set_str( CFG_str2id("AKS_BIT_SETTINGS"), argv[0]);
			write_data_to_flash();
			//CFG_save(0);
		}
		else
		{
			return -1;
		}
	}

	return 0;
}

static int USER_FUNC reload_cmd_web_para_node_name(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		sprintf(rsp, "=%s", g_web_config.name);
	}
	else
	{
		sprintf(g_web_config.name,"%s",argv[0]);
		write_data_to_flash();
	}	
	return 0;
}
static int USER_FUNC reload_cmd_web_para_universe(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int temp_value = -1;
	
	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		sprintf(rsp, "=%s", g_web_config.universe);
	}
	else
	{
		temp_value = atoi(argv[0]);
		if((temp_value>=0)&&(temp_value<=9))
		{
			strcpy(g_web_config.universe,argv[0]);
			write_data_to_flash();
		}
		else
			return -1;
	}
	return 0;
}

static int USER_FUNC reload_cmd_web_para_timopower(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int temp_value = -1;
	
	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		sprintf(rsp, "=%s", g_web_config.timopower);
	}
	else
	{
		temp_value = atoi(argv[0]);
		if((temp_value>=0)&&(temp_value<=5))
		{
			strcpy(g_web_config.timopower,argv[0]);
			write_data_to_flash();
		}
		else
			return -1;
	}
	return 0;
}

static int USER_FUNC reload_cmd_web_para_channelwidth(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int temp_value = -1;
	
	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		sprintf(rsp, "=%s", g_web_config.channelWidth);
	}
	else
	{
		temp_value = atoi(argv[0]);
		if((temp_value>=9)&&(temp_value<=18))
		{
			strcpy(g_web_config.channelWidth,argv[0]);
			write_data_to_flash();
		}
		else
			return -1;
	}
	return 0;
}

static int USER_FUNC reload_cmd_web_para_secondchannel(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int temp_value = -1;
	
	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{

		sprintf(rsp, "=%s", g_web_config.secondChannel);
	}
	else
	{
		temp_value = atoi(argv[0]);
		if((temp_value>=0)&&(temp_value<=11))
		{
			strcpy(g_web_config.secondChannel,argv[0]);
			write_data_to_flash();
		}
		else
			return -1;
	}
	return 0;
}

static int USER_FUNC reload_cmd_web_para_bitsetting(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int temp_value = -1;
	
	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{

		sprintf(rsp, "=%s", g_web_config.bitSetting);
	}
	else
	{
		temp_value = atoi(argv[0]);
		if((temp_value>=0)&&(temp_value<=1))
		{
			strcpy(g_web_config.bitSetting,argv[0]);
			write_data_to_flash();
		}
		else
			return -1;
	}
	return 0;
}



const hfat_cmd_t user_define_at_cmds_table[]=
{
//AT CMD: set /get
	{"NODENAME",cmd_web_para_node_name,"AT+NODENAME: get/set device node name\r\n",NULL},
	{"UNIVERSE",cmd_web_para_universe,"AT+UNIVERSE: get/set UNIVERSE ,value must be 0~15\r\n",NULL},
	{"TIMOPOWER",cmd_web_para_timopower,"AT+TIMOPOWER: get/set TIMOPOWER ,value must be 0~5\r\n",NULL},
	{"CHANNELWIDTH",cmd_web_para_channelwidth,"AT+CHANNELWIDTH: get/set CHANNELWIDTH ,value must be 9~18\r\n",NULL},
	{"SECONDCHANNEL",cmd_web_para_secondchannel,"AT+SECONDCHANNEL: get/set SECONDCHANNEL,value must be 0~11\r\n",NULL},
	{"BITSETTING",cmd_web_para_bitsetting,"AT+BITSETTING: get/set BITSETTING,value must be 0 or 1\r\n",NULL},
	{"ARTNET",cmd_web_para_artnet,"AT+ARTNET: get/set ARTNET,value must be on or off\r\n",NULL},
	{"RD",cmd_web_para_rd,"AT+RD: RD register value\r\n",NULL},
	
//AT CMD: init data after reload
	{"WNODENAME",reload_cmd_web_para_node_name,"AT+WNODENAME: get/set device node init name after reload\r\n",NULL},
	{"WUNIVERSE",reload_cmd_web_para_universe,"AT+WUNIVERSE: get/set init UNIVERSE after reload,value must be 0~15\r\n",NULL},
	{"WTIMOPOWER",reload_cmd_web_para_timopower,"AT+WTIMOPOWER: get/set init TIMOPOWER after reload,value must be 0~5\r\n",NULL},
	{"WCHANNELWIDTH",reload_cmd_web_para_channelwidth,"AT+WCHANNELWIDTH: get/set init CHANNELWIDTH after reload,value must be 9~18\r\n",NULL},
	{"WSECONDCHANNEL",reload_cmd_web_para_secondchannel,"AT+WSECONDCHANNEL: get/set init SECONDCHANNEL after reload,value must be 0~11\r\n",NULL},
	{"WBITSETTING",reload_cmd_web_para_bitsetting,"AT+WBITSETTING: get/set init BITSETTING after reload,value must be 0 or 1\r\n",NULL},

//the last item must be null
	{NULL,NULL,NULL,NULL} 
};
/*AT CMD end¡ü*/


//system event ,use for system relaod here
void init_webdata_when_reload(void)
{
#if 0	
	int temp_value = -1;
	
	memset(&g_web_config,0,sizeof(WEB_DATA_T));
	hfuflash_read(WEB_DATA_FLASH_ADDR,&g_web_config,sizeof(WEB_DATA_T));

	printf("___ g_web_config.name=%s_%d\n",g_web_config.name,strlen(g_web_config.name));
	if(strlen(g_web_config.name)>0)
	{
		ratpac_set_str( CFG_str2id("AKS_NAME"), g_web_config.name);
	}
	else
	{
		ratpac_set_str( CFG_str2id("AKS_NAME"), "Ratpac AKS");
	}
	
	temp_value = atoi(g_web_config.universe);
	printf("___ g_web_config.universe=%s_%d\n",g_web_config.universe,temp_value);
	if((temp_value>0)&&(temp_value<=15))
	{
		ratpac_set_str( CFG_str2id("AKS_UNIVERSE"), g_web_config.universe);
	}
	else
	{
		ratpac_set_str( CFG_str2id("AKS_UNIVERSE"), "0");
	}
	
	temp_value = atoi(g_web_config.timopower);
	printf("___ g_web_config.timopower=%s_%d\n",g_web_config.timopower,temp_value);
	if((temp_value>0)&&(temp_value<=5))
	{
		ratpac_set_str( CFG_str2id("TIMO_POWER"), g_web_config.timopower);
	}
	else
	{
		ratpac_set_str( CFG_str2id("TIMO_POWER"), "3");
	}

	temp_value = atoi(g_web_config.channelWidth);
	printf("___ g_web_config.channelWidth=%s_%d\n",g_web_config.channelWidth,temp_value);
	if((temp_value>9)&&(temp_value<=18))
	{
		ratpac_set_str( CFG_str2id("AKS_CHANNEL_WIDTH"), g_web_config.channelWidth);
	}
	else
	{
		ratpac_set_str( CFG_str2id("AKS_CHANNEL_WIDTH"), "16");
	}

	temp_value = atoi(g_web_config.secondChannel);
	printf("___ g_web_config.secondChannel=%s_%d\n",g_web_config.secondChannel,temp_value);
	if((temp_value>0)&&(temp_value<=11))
	{
		ratpac_set_str( CFG_str2id("AKS_SECOND_CHANNEL"), g_web_config.secondChannel);
	}
	else
	{
		ratpac_set_str( CFG_str2id("AKS_SECOND_CHANNEL"), "0");
	}

	temp_value = atoi(g_web_config.bitSetting);
	printf("___ g_web_config.bitSetting=%s_%d\n",g_web_config.secondChannel,temp_value);
	if((temp_value>0)&&(temp_value<=1))
	{
		ratpac_set_str( CFG_str2id("AKS_BIT_SETTINGS"), g_web_config.bitSetting);
	}
	else
	{
		ratpac_set_str( CFG_str2id("AKS_BIT_SETTINGS"), "0");
	}
#endif	
}

static int hfsys_event_callback( uint32_t event_id,void * param)
{
	switch(event_id)
	{
		case HFE_WIFI_STA_CONNECTED:
			printf("wifi sta connected!!\n");
			break;
		case HFE_WIFI_STA_DISCONNECTED:
			printf("wifi sta disconnected!!\n");
			break;
		case HFE_SMTLK_OK:
			printf("smtlk ok!\n");
			break;
		case HFE_CONFIG_RELOAD:
			printf("___system reload!\n");
			Joo_uart_send("___system reload!\n");
			init_webdata_when_reload();
			break;
		default:
			break;
	}
	return 0;
}
void web_flash_data_init(void)
{
	int i;
	unsigned char ck_sum = 0;
	unsigned char *fls_data;
	
	memset(&g_web_config,0,sizeof(WEB_DATA_T));
	hfuflash_read(WEB_DATA_FLASH_ADDR,&g_web_config,sizeof(WEB_DATA_T));
	printf("\n----------read flash data-----------\n");
	printf("____g_web_config.initFlag=0x%02X\n",g_web_config.initFlag);
	printf("____g_web_config.initFlag=%s\n",g_web_config.name);
	printf("____g_web_config.initFlag=%s\n",g_web_config.universe);
	printf("____g_web_config.initFlag=%s\n",g_web_config.timopower);
	printf("____g_web_config.initFlag=%s\n",g_web_config.channelWidth);
	printf("____g_web_config.initFlag=%s\n",g_web_config.secondChannel);
	printf("____g_web_config.initFlag=%s\n",g_web_config.bitSetting);
	printf("-----------------------------------\n\n");
	
	fls_data = (unsigned char *)&g_web_config;
	for (i = 1; i < sizeof(WEB_DATA_T); i++)
	{
		if (0xff == fls_data[i])
		{
			ck_sum = 0;		// no valid
			break;
		}
		ck_sum ^= fls_data[i];
	}
	if((g_web_config.initFlag != ck_sum) || (g_web_config.magic[0] != MAGIC0) || (g_web_config.magic[1] != MAGIC1))
	{
		memset(&g_web_config,0,sizeof(WEB_DATA_T));

		if (!(strcmp(build_img, "AKS")))
			sprintf(g_web_config.name,"Ratpac AKS");
		else
			sprintf(g_web_config.name,"Satellite");
		sprintf(g_web_config.universe,"0");
		sprintf(g_web_config.subnet,"0");
		sprintf(g_web_config.gaffer_enb,"1");		// disable
		sprintf(g_web_config.protname,"0");			// ArtNet
		sprintf(g_web_config.gaffer_sub,"0");
		sprintf(g_web_config.gaffer_univ,"0");
		sprintf(g_web_config.gaffer_lower,"1");
		sprintf(g_web_config.gaffer_upper,"1");

		sprintf(g_web_config.sacn_enb,"1");		// disable
		sprintf(g_web_config.sacn_universe,"1");
		
		sprintf(g_web_config.timopower,"3");
		sprintf(g_web_config.channelWidth,"16");
		sprintf(g_web_config.secondChannel,"0");
		sprintf(g_web_config.bitSetting,"0");
		sprintf(g_web_config.led_color,"255");
		sprintf(g_web_config.led_g,"112");
		sprintf(g_web_config.led_b,"102");		
		sprintf(g_web_config.use_xlr,"0");
		sprintf(g_web_config.main_xlr,"0");
		sprintf(g_web_config.sort_by,"0");		
		g_web_config.magic[0] = MAGIC0;
		g_web_config.magic[1] = MAGIC1;		
		write_data_to_flash();
		//m2m_do_reload();
	}
}

static char uart_rcv_data[64];
static int uart_rvc_len = 0;
static int uart_rvc_done = 1;

static debug_buff[16];
static int USER_FUNC uart_recv_callback(uint32_t event,char *data,uint32_t len,uint32_t buf_len)
{
	int copy_len;
#if 1	
	static int first_time = 0;
	//static cyg_priority_t pri;
	static cyg_handle_t uartHandle;
	
	if (0 == first_time)
	{
		uartHandle = cyg_thread_self();
		cyg_thread_set_priority(uartHandle, AKS_PRIORITIES );    // was 14 set by default vendor
		//pri = cyg_thread_get_priority(uartHandle);
		//eprintf ("Uart Thread Pri = %d\n", (int)pri);
		//pri = cyg_thread_get_current_priority(uartHandle);
		//eprintf ("Uart Thread Cur Pri = %d\n", (int)pri);
		first_time = 1;
	}
#endif
#if 0
	{
		memmove(debug_buff, data, len);
		debug_buff[len] = 0;
		Joo_uart_send(debug_buff);
		
	}
#endif	
	if (1 == uart_rvc_done /* artnet_enable */)
	{
		uart_rvc_len=0;
		return len;			// was len before, now 0 => don't send to UDP port
	}
#if 0	
	{
		memmove(debug_buff, data, len);
		debug_buff[len] = 0;
		Joo_uart_send(debug_buff);
		
	}
#endif
	
	copy_len = ((len > 60) ? 60 : len);
	strncpy(&uart_rcv_data[uart_rvc_len], data, copy_len);
	uart_rvc_len += copy_len;
	if (uart_rcv_data[uart_rvc_len-1] == '\n' || uart_rcv_data[uart_rvc_len-1] == '\r')
	{
		uart_rcv_data[uart_rvc_len] = 0;
		uart_rvc_done = 1;
#if 0
		if (uart_rcv_data[uart_rvc_len-1] == '\n')
			Joo_uart_send("TWN");
		else if (uart_rcv_data[uart_rvc_len-1] == '\r')
			Joo_uart_send("TWR");
		else		
			Joo_uart_send("TW?");
#endif		
		//uart_rvc_len = 0;
		return (copy_len);
	}
	//Joo_uart_send("PD");
	return (copy_len);
}	

#if 0
hfnet_socketa_client_t artpoll_client;

static int USER_FUNC socketa_recv_callback(uint32_t event,char *data,uint32_t len,uint32_t buf_len)
{
	if(event==HFNET_SOCKETA_CONNECTED)
	{
		//hfgpio_fset_out_low(HFGPIO_F_TCP_LINK);
	}
	else if(event==HFNET_SOCKETA_DISCONNECTED)
	{
		//hfgpio_fset_out_high(HFGPIO_F_TCP_LINK);
	}
	else if(event==HFNET_SOCKETA_DATA_READY)
	{
		char Uni[4]={0};
		char label[] = "Art-Net";
		int uni_num, art_sub;
		
		if(strncmp(data, label, 7)==0)
		{
			check_gaffer_packet(data, len);
			ratpac_get_str(CFG_str2id("AKS_UNIVERSE"),Uni);	
			uni_num = atoi(Uni);;
			ratpac_get_str(CFG_str2id("AKS_SUBNET"),Uni);
			art_sub = atoi(Uni);
			art_sub <<= 4;
			uni_num = ((uni_num | art_sub) & 0xff);
			if (0 == artnet_enable)
			{
				return 0;			// do not send data if artnet_enable is 0
			}
			if(len > 50)
			{
				if(uni_num == (int)data[14])
				{
					replace_channel_data(data, len); 
					hfuart_send(HFUART0, data,len,1000);
				}
			}else
			{
				// check if this is artnet poll request
				//send_artpollreply_msg();
				// hfnet_socketa_get_client(0, &artpoll_client);
				// send_artpollreply_msg(&artpoll_client);
				hfuart_send(HFUART0, data,len,1000);
			}
		}
	}
	return 0;
}

static int USER_FUNC socketb_recv_callback(uint32_t event,char *data,uint32_t len,uint32_t buf_len)
{
	e131_error_t error;
	static uint8_t last_seq = 0x00;
	e131_packet_t *packet;
	
	if(event==HFNET_SOCKETB_CONNECTED)
	{
		//hfgpio_fset_out_low(HFGPIO_F_TCP_LINK);
	}
	else if(event==HFNET_SOCKETB_DISCONNECTED)
	{
		//hfgpio_fset_out_high(HFGPIO_F_TCP_LINK);
	}
	else if(event==HFNET_SOCKETB_DATA_READY)
	{
		packet = (e131_packet_t *)data;
		
		if ((error = e131_pkt_validate(packet)) != E131_ERR_NONE) {
			eprintf("e131_pkt_validate: %s\n", e131_strerror(error));
			return 0;
		}
		if (e131_pkt_discard(packet, last_seq)) {
			eprintf("warning: packet out of order received\n");
			last_seq = packet->frame.seq_number;
			return 0;
		}
		//e131_pkt_dump(stderr, &packet);
		eprintf("receive sACN packet %d\n", last_seq);
		last_seq = packet->frame.seq_number;
	}
	return 0;
}
#endif


static char rgb0, rgb1, rgb2;
static char rgb_led_color[16];
static char use_xlr, main_xlr, ip_code;

void UserMain(void *arg)
{
	
	int ret1;
	
	int level;
	level = DEBUG_LEVEL;
	
	hfdbg_set_level(level);
	level =  hfdbg_get_level();
	M2M_LOG(("**********dbglevel:%d\r\n", level));
	time_t time_now;
	time_now = time(NULL);
	HF_Debug(DEBUG_LEVEL,"sdk version(%s),the app_main start time is %d %s\n",hfsys_get_sdk_version(),time_now,ctime(&time_now));
	pWIFIDev = &devive_wireless_netdev0;
	if(hfgpio_fmap_check(HFM_TYPE_A21)!=0)
	{
		while(1)
		{
			HF_Debug(DEBUG_ERROR,"gpio map file error\n");
			msleep(1000);
		}
		//return 0;
	}
	while(!hfnet_wifi_is_active())
	{
		msleep(50);
	}
	if(hfsys_register_system_event((hfsys_event_callback_t)hfsys_event_callback)!=HF_SUCCESS)
	{
		u_printf("register system event fail\n");
	}
	web_flash_data_init();   // cannot print UART0, since it is not started yet. If so, system will crash, and box is not recovery!!! */
	if(hfnet_start_assis(ASSIS_PORT)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start assis fail\n");
	}

	// Need to start UART, or calling the uart send function will crash and system is not recoverable
	
	if(hfnet_start_uart(AKS_PRIORITIES,(hfnet_callback_t)uart_recv_callback)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start uart fail\n");
	}

#if 0	
	if(hfnet_start_socketa(HFTHREAD_PRIORITIES_LOW,(hfnet_callback_t)socketa_recv_callback)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start socketa fail\n");
	}
	if(hfnet_start_socketb(HFTHREAD_PRIORITIES_LOW,(hfnet_callback_t)NULL)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start socketb fail\n");
	}
#endif

	{
		//static cyg_priority_t pri;
		static cyg_handle_t userHandle;
		
		userHandle = cyg_thread_self();
		cyg_thread_set_priority(userHandle, AKS_PRIORITIES );    // was 6 set by default vendor
		//pri = cyg_thread_get_priority(uartHandle);
		//eprintf ("UserMain Thread Pri = %d\n", (int)pri);
		//pri = cyg_thread_get_current_priority(uartHandle);
		//eprintf ("UserMain Thread Cur Pri = %d\n", (int)pri);
	}

	Joo_uart_send("__system_reboot");
	// just stop here in case of crashing
#if 0	
	while(1)
	{
		if (0 == artnet_enable)
		{
			hf_thread_delay(1000);	
			continue;
		}
		break;
	}
#endif
	hf_thread_delay(100);
	cyg_mutex_init(&samd_mutex);
	
	ret1 = hfthread_create(sACN_main,"udp_sACN_main",1024,(void*)1,AKS_PRIORITIES,NULL,NULL);

	if (HF_SUCCESS != ret1)
	{
		eprintf("Create UDP server fails, %d\n", ret1);
	}	
	
	// Check a dedicated IO Pin if we should bring up the whole system

	ret1 = hfat_send_cmd("AT+FVEW\r\n", strlen("AT+FVEW\r\n"), at_rsp, sizeof(at_rsp));
	if (HF_SUCCESS != ret1)
	{
		at_rsp[0] = 0;
		eprintf("hfat_send_cmd fails\n");
	}
	else
	{
		at_rsp[13] = 0;
		eprintf("hfat_send_cmd: %s\n", at_rsp);
	}

	if (strstr(at_rsp, "+ok=Enable"))
	{	
		// turn off the wifi module, no need, SAMD does it
#if 0		
		ret1 = hfat_send_cmd("AT+MSLP=OFF\r\n", strlen("AT+MSLP=OFF\r\n"), at_rsp, sizeof(at_rsp));
		if (HF_SUCCESS != ret1)
		{	
			at_rsp[0] = 0;
			eprintf("MSLP=OFF fails\n");
		}
#endif	
		operation_mode = STA_ETH_MODE;
		
		//ret1 = hfthread_create(client_thread_main,"udp_client_main",1024,(void*)1,AKS_PRIORITIES,NULL,NULL);

		//if (HF_SUCCESS != ret1)
		//{
		//	eprintf("Create UDP client fails, %d\n", ret1);
		//}		
	}
	else
	{
		ret1 = hfat_send_cmd("AT+WMODE\r\n", strlen("AT+WMODE\r\n"), at_rsp, sizeof(at_rsp));
		if (HF_SUCCESS != ret1)
		{
			at_rsp[0] = 0;
			eprintf("hfat_send_cmd fails\n");
		}
		else
		{
			at_rsp[8] = 0;
			eprintf("hfat_send_cmd: %s\n", at_rsp);
		}
		if (strstr(at_rsp, "+ok=STA"))
		{
			operation_mode = STA_WIFI_MODE;
			//ret1 = hfthread_create(client_thread_main,"udp_client_main",1024,(void*)1,AKS_PRIORITIES,NULL,NULL);

			//if (HF_SUCCESS != ret1)
			//{
			//	eprintf("Create UDP client fails, %d\n", ret1);
			//}
		}
		else
		{
			operation_mode = AP_MODE;
			//ret1 = hfthread_create(server_thread_main,"udp_server_main",1024,(void*)1,AKS_PRIORITIES,NULL,NULL);

			//if (HF_SUCCESS != ret1)
			//{
			//	eprintf("Create UDP server fails, %d\n", ret1);
			//}
		}
	}
	
	ret1 = hfat_send_cmd("AT+UART\r\n", strlen("AT+UART\r\n"), at_rsp, sizeof(at_rsp));
	if (HF_SUCCESS != ret1)
	{
		at_rsp[0] = 0;
		eprintf("hfat_send_cmd uart fails\n");
		while (1)
		{
			hf_thread_delay(5000);
		}
	}
	else
	{
		at_rsp[13] = 0;
		eprintf("hfat_send_cmd: %s\n", at_rsp);
	}

	if (!strstr(at_rsp, "+ok=230400"))
	{
		hfat_send_cmd("AT+UART=230400,8,1,NONE\r\n", strlen("AT+UART=230400,8,1,NONE\r\n"), at_rsp, sizeof(at_rsp));
		while (1)
		{
			hf_thread_delay(5000);    // after changing speed, wait for box reset to take effect
		}
	}

	ret1 = hfthread_create(client_thread_main,"udp_client_main",1536,(void*)1,AKS_PRIORITIES,NULL,NULL);

	if (HF_SUCCESS != ret1)
	{
		eprintf("Create UDP client fails, %d\n", ret1);
		while (1)
		{
			hf_thread_delay(5000);
		}
	}	
	
	ret1 = hfthread_create(server_thread_main,"udp_server_main",1536,(void*)1,AKS_PRIORITIES,NULL,NULL);

	if (HF_SUCCESS != ret1)
	{
		eprintf("Create UDP server fails, %d\n", ret1);
		while (1)
		{
			hf_thread_delay(5000);
		}		
	}	
	// int i=0;
	

	// Joo_uart_send("__system_reboot");

	while(1)
	{
		if (0 == artnet_enable)
		{
			hf_thread_delay(500);
#if 0  // only for testing UART receive			
			if (1 == uart_rvc_done)
			{
				Joo_uart_send(uart_rcv_data);
				uart_rvc_done = 0;
				uart_rvc_len = 0;
			}
#endif			
			continue;
		}
		/*
		char TimoEnable[1] = {};
		ratpac_get_str(CFG_str2id("AKS_SETTINGS"),TimoEnable);
		if(TimoEnable[0] == 48)
		{
			*/			
			char timo_DMX_Window[4] = {0b00000000, 0b00000000, 0b00000010, 0b00000000};
			char timo_DMX_Spec[8] = {0b00000000, 0b00000010, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b01100001, 0b10101000};
			char TimoPower[1] = {};
			char timo_Blocked_Channel[11] = {0,0,0,0,0,0,0,0,0,0,0};

			char channel[4]={};
			
			if (AP_MODE == operation_mode)
			{
				CFG_get_str(CFG_str2id("WLN_Channel"),channel);
				channel[0] = (char)atoi(channel);
			}
			else if (STA_WIFI_MODE == operation_mode)
			{	
				static int get_ap_wln_channel = 0;
				static char *ptr_ssid_str;
				
				if ((uint8_t)ipAddress[0] == 0)
				{
					ap_wln_channel = 0;
					channel[0] = 1;
					get_ap_wln_channel = 0;
				}
				else if (0 == get_ap_wln_channel) 
				{
					ret1 = hfat_send_cmd("AT+WSCAN\r\n", strlen("AT+WSCAN\r\n"), at_scan_rsp, sizeof(at_scan_rsp));
					if (HF_SUCCESS == ret1)
					{
						at_scan_rsp[512] = 0;
						ptr_ssid_str = strstr(at_scan_rsp, SSID_STR);
						if (ptr_ssid_str)
						{
							ptr_ssid_str -= 2;
							ap_wln_channel = *ptr_ssid_str - '0';
							get_ap_wln_channel = 1;
						}	
					}					
				}
				channel[0] = ap_wln_channel;
			}
			else
			{
				channel[0] = 1;
			}

			char width[4]={};
			ratpac_get_str(CFG_str2id("AKS_CHANNEL_WIDTH"),width);
			width[0] = (char)atoi(width);

			char channelTwo[4]={};
			ratpac_get_str(CFG_str2id("AKS_SECOND_CHANNEL"),channelTwo);
			channelTwo[0] = (char)atoi(channelTwo);

			//hfuart_send(HFUART0, channel,sizeof(channel),1000);
			if(channel[0] > 0 && channel[0] < 12)
			{
				char actualChannel[11] = {12,17,22,27,32,37,42,47,52,57,62};
		        char channelTotal = 0;
		        int x, y;
		        for (x=0; x<11; x++)
		        {
		            for (y=0; y<8; y++)
		            {
		                if(!(channelTotal < (actualChannel[channel[0] - 1] - width[0]) || channelTotal > (actualChannel[channel[0] - 1] + width[0])))
		                {
		                	timo_Blocked_Channel[x] = timo_Blocked_Channel[x] | (1 << y);
		                }
		                channelTotal++;
		            }
		        }
		    }
			if(channelTwo[0] > 0 && channelTwo[0] < 12)
			{
				char actualChannel[11] = {12,17,22,27,32,37,42,47,52,57,62};
		        char channelTotal = 0;
		        int x, y;
		        for (x=0; x<11; x++)
		        {
		            for (y=0; y<8; y++)
		            {
		            	if(!(channelTotal < (actualChannel[channelTwo[0] - 1] - width[0]) || channelTotal > (actualChannel[channelTwo[0] - 1] + width[0])))
		            	{
								timo_Blocked_Channel[x] = timo_Blocked_Channel[x] | (1 << y);
		            	}
		                channelTotal++;
		            }
		        }
		    }
			ratpac_get_str(CFG_str2id("TIMO_POWER"),TimoPower);
			TimoPower[0] = TimoPower[0] - 48;

			char Battery[8] = {0b00110100, 0b00011011, 0b01100000, 0b00010001, 0b10110010, 0b10011010, 0b00000011, 0b01001011};
			char Timo[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 0, 0b10000010, 0b00000000, timo_DMX_Window[0], timo_DMX_Window[1], timo_DMX_Window[2], timo_DMX_Window[3], timo_DMX_Spec[0], timo_DMX_Spec[1], timo_DMX_Spec[2], timo_DMX_Spec[3], timo_DMX_Spec[4], timo_DMX_Spec[5], timo_DMX_Spec[6], timo_DMX_Spec[7], 0b00000001, TimoPower[0], timo_Blocked_Channel[0], timo_Blocked_Channel[1], timo_Blocked_Channel[2], timo_Blocked_Channel[3], timo_Blocked_Channel[4], timo_Blocked_Channel[5], timo_Blocked_Channel[6], timo_Blocked_Channel[7], timo_Blocked_Channel[8], timo_Blocked_Channel[9], timo_Blocked_Channel[10], Battery[0], Battery[1], Battery[2], Battery[3], Battery[4], Battery[5], Battery[6], Battery[7]};
			hfuart_send(HFUART0, Timo,sizeof(Timo),100);

			//char ipAddress[4]={};
			int rc;			
			if (AP_MODE == operation_mode)
			{
				rc = hfat_send_cmd("AT+LANN\r\n", sizeof("AT+LANN\r\n"),at_rsp,sizeof(at_rsp));				
				//ipAddress[0] = 0xa;
				//ipAddress[1] = 0xa;
				//ipAddress[2] = 0x64;
				//ipAddress[3] = 0xfe;
			}
			else
			{			
				rc = hfat_send_cmd("AT+WANN\r\n", sizeof("AT+WANN\r\n"),at_rsp,sizeof(at_rsp));
			}
			at_rsp[24] = 0;
			if (HF_SUCCESS != rc)
			{
				eprintf("WAN Command fails.\n");
				ipAddress[0] = 0x0;
				ipAddress[1] = 0x0;
				ipAddress[2] = 0x0;
				ipAddress[3] = 0x0;
			}
			else if (strstr(at_rsp, "+ERR"))
			{
				eprintf("WAN Command Err.\n");
				ipAddress[0] = 0x0;
				ipAddress[1] = 0x0;
				ipAddress[2] = 0x0;
				ipAddress[3] = 0x0;
			}			
			else
			{
				const char s[] = ".,";
				char *token;
				if (isdigit(at_rsp[4]))
				{
					token = strtok((char *)&at_rsp[4], s);
				}
				else
				{
					token = strtok((char *)&at_rsp[9], s);
				}
				ipAddress[0] = (char)atoi(token);
				token = strtok(NULL, s);
				ipAddress[1] = (char)atoi(token);
				token = strtok(NULL, s);
				ipAddress[2] = (char)atoi(token);
				token = strtok(NULL, s);
				ipAddress[3] = (char)atoi(token);					
			}
#if 0				
				else if (strstr(at_rsp, "+ok=DHCP,0.0.0.0"))
				{
					ipAddress[0] = 0x0;
					ipAddress[1] = 0x0;
					ipAddress[2] = 0x0;
					ipAddress[3] = 0x0;
				}
				else
				{
					// fake adddres indicate address is acquired
					char *tmp = (char *)&at_rsp[9];
					char *tmp1;
					tmp1 = strstr(tmp, ",");
					if (tmp1 == 0)
					{
						// cannot happen, just for sanity
						ipAddress[0] = 0xa;
						ipAddress[1] = 0xa;
						ipAddress[2] = 0x64;
						ipAddress[3] = 0xfe;
					}
					else
					{
						struct in_addr ap;
						
						*tmp1 = 0;
						// always assume it is a valid IP address
						inet_aton(tmp, &ap);
						ipAddress[0] = ap.s_addr & 0xff;
						ipAddress[1] = (ap.s_addr >> 8) & 0xff;
						ipAddress[2] = (ap.s_addr >> 16) & 0xff;
						ipAddress[3] = (ap.s_addr >> 24) & 0xff;										
					}
				}
#endif				
			
#if 0			
		    char ipAddressSTR[20]={};
			CFG_get_str(CFG_str2id("LAN_IP"),ipAddressSTR);
			char lenth = strlen(ipAddressSTR);

			char ipAddressSTRTrimmed[lenth];

			strncpy(ipAddressSTRTrimmed, ipAddressSTR, lenth);
			char ipAddress[4]={};
			const char s[] = ".";
			char *token;

			rc = hfat_send_cmd("AT+WANN\r\n", sizeof("AT+WANN\r\n"),at_rsp,sizeof(at_rsp));
			token = strtok(ipAddressSTRTrimmed, s);
			ipAddress[0] = (char)atoi(token);
			token = strtok(NULL, s);
			ipAddress[1] = (char)atoi(token);
			token = strtok(NULL, s);
			ipAddress[2] = (char)atoi(token);
			token = strtok(NULL, s);
			ipAddress[3] = (char)atoi(token);

			char name[18]={' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',0};
			ratpac_get_str(CFG_str2id("AKS_NAME"),name);
			char Settings[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 1,name[0],name[1],name[2],name[3],name[4],name[5],name[6],name[7],name[8],name[9],name[10],name[11],name[12],name[13],name[14],name[15],name[16],name[17],ipAddress[0],ipAddress[1],ipAddress[2],ipAddress[3]};
#endif
			if (0 != ipAddress[0])
			{
				ip_code = 1;
			}
			else
			{
				ip_code = 0;
			}
			char Settings[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 1, ip_code};
			//char Settings[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 1, ipAddress[0], ipAddress[1], ipAddress[2],ipAddress[3] };
			hfuart_send(HFUART0, Settings,sizeof(Settings),100);

			//char Settings2[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 2, 1, 0};
			//char Settings2[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 5};
			//hfuart_send(HFUART0, Settings2,sizeof(Settings2),100);
			
			char gaf_enb, gaf_low_lsb, gaf_low_msb, gaf_high_lsb, gaf_high_msb;
			int gaf_chn;
			
			gaf_enb = atoi(g_web_config.gaffer_enb);
			gaf_chn = atoi(g_web_config.gaffer_lower);
			gaf_low_lsb = (char)(gaf_chn & 0xff);
			gaf_low_msb = (char)((gaf_chn >> 8) & 0xff);
			gaf_chn = atoi(g_web_config.gaffer_upper);
			gaf_high_lsb = (char)(gaf_chn & 0xff);
			gaf_high_msb = (char)((gaf_chn >> 8) & 0xff);
			
			//strcpy(rgb_led_color, g_web_config.led_color);
			//rgb_led_color[3] = 0;
			//rgb_led_color[7] = 0;
			//rgb_led_color[11] = 0;
			rgb0 = (char)atoi(g_web_config.led_color);
			rgb1 = (char)atoi(g_web_config.led_g);
			rgb2 = (char)atoi(g_web_config.led_b);
			use_xlr = g_web_config.use_xlr[0] - '0';
			main_xlr = g_web_config.main_xlr[0] - '0';
			char Settings6[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 6, gaf_enb, gaf_low_lsb, gaf_low_msb, gaf_high_lsb, gaf_high_msb, rgb0, rgb1, rgb2, use_xlr, main_xlr };
			hfuart_send(HFUART0, Settings6,sizeof(Settings6),100);

			hf_thread_delay(2000);
			if (1 == artnet_enable)
			{
				Send_Battery_Command();
			}

		//}
		hf_thread_delay(3000);
	}
	return ;
}

void Joo_uart_send(char *data)
{
	hfuart_send(HFUART0, data, strlen(data),100);

}


#define LOCAL_SERVER_PORT 10000
#define REMOTE_CLIENT_PORT 10001
#define MAX_MSG 100
static char msg[MAX_MSG];
#define MAX_NUM_ENTRY 24

struct client_ent
{
	char node_name[20];
	char ip_addr[20];
	char universe[4];
	char subnet[4];
	char baterry[8];
	char eCos_ver[8];
	char samd_ver[8];
	char timo_ver[8];
	char sacn_univ[8];
	char sort_by[4];
	char crmx_power[4];
	char host_mode[4];
	char protocol[4];
	int	 age_cnt;
};

struct client_ent client_list[MAX_NUM_ENTRY];
struct client_ent client_valid_list[MAX_NUM_ENTRY];
static client_valid_num = 0;
static char tmp_buff[20];

struct client_ent *sort_array[MAX_NUM_ENTRY];

char this_node_ip_address[20];

static void server_thread_main(void* arg) 
{
  int sd, rc, n, cliLen;
  struct sockaddr_in cliAddr, servAddr, remoteServAddr;
  fd_set rset;
  struct timeval timeout;
  int i;
  cyg_tick_count_t cur_tick;

  int broadcast = 1;

  refresh_clients = 0;

  eprintf("server_thread_main started\n");
  // operation_mode = AP_MODE;
  artnet_enable = 1;  
  
  for (i = 0; i < MAX_NUM_ENTRY; i++)
  {
	  client_list[i].age_cnt = 0;
	  client_list[i].ip_addr[19] = 0;
  }
	  
  /* socket creation */
  sd=socket(AF_INET, SOCK_DGRAM, 0);
  if(sd<0) {
    eprintf("cannot open socket \n");
   
  }
  
  if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof broadcast) == -1) {
          eprintf("setsockopt (SO_BROADCAST)");
		  close (sd);
          
  }

  memset((char*)&servAddr,0,sizeof(servAddr));

  /* bind local server port */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(LOCAL_SERVER_PORT);
  rc = bind (sd, (struct sockaddr *) &servAddr,sizeof(servAddr));
  if(rc<0) {
    eprintf("cannot bind port number %d, error=%d \n", 
	   LOCAL_SERVER_PORT, rc);
	close (sd);
  }

  hfnet_set_udp_broadcast_port_valid(LOCAL_SERVER_PORT, REMOTE_CLIENT_PORT);
  
  //eprintf("waiting for data on port UDP %u\n", 
   //	   LOCAL_SERVER_PORT);

  memset((char*)&remoteServAddr,0,sizeof(remoteServAddr));
  remoteServAddr.sin_family = AF_INET;
  remoteServAddr.sin_addr.s_addr = htonl(0x0a0a64ff);
  remoteServAddr.sin_port = htons(REMOTE_CLIENT_PORT);

#if 0
  sprintf(msg, "This is a UDP broadcast test\n");
  Joo_uart_send(msg);

  rc = sendto(sd, msg, strlen(msg), 0, 
		  (struct sockaddr *) &remoteServAddr, 
  			sizeof(remoteServAddr));

  close(sd);

  return 0;
#endif

  sprintf(client_list[0].ip_addr, "10.10.100.254");
  client_valid_num = 0;
   /* server infinite loop */
 
	age_client_list(1);

	cur_tick = cyg_current_time();
	while (1)
	{
		FD_ZERO(&rset);
		FD_SET(sd,&rset);
		timeout.tv_sec= 2;
		timeout.tv_usec= 0;
		rc = select(sd+1, &rset, NULL, NULL, &timeout);
		if ((cyg_current_time() - cur_tick) >= 200)   // every tick is 10ms, age every 2 seconds	
		{
			age_client_list(0);
			cur_tick = cyg_current_time();
		}

		if (rc <= 0)
		{
			eprintf("No data within 3 seconds.\n");
			refresh_clients = 0;
			cur_tick = cyg_current_time();
			continue;
		}
		if (FD_ISSET(sd, &rset))
		{
    		/* receive message */
			memset(msg,0x0,MAX_MSG);
    		cliLen = sizeof(cliAddr);
    		n = recvfrom(sd, msg, MAX_MSG, 0, 
		 		(struct sockaddr *) &cliAddr, &cliLen);

    		if(n<0) {
      			eprintf("cannot receive data \n");
      			continue;
    		}
  
    		/* print received message */
    		eprintf("from %s:UDP%u : len=%d \n", 
	   			inet_ntoa(cliAddr.sin_addr),
	   			ntohs(cliAddr.sin_port), n);
			for (i = 0; i < n; i++)
			{
				eprintf("%02x ", msg[i]);
			}
			eprintf("\r\n");
			msg[19] = 0;
			msg[22] = 0;
			msg[26] = 0;
			msg[36] = 0;
			msg[44] = 0;
			msg[51] = 0;
			msg[59] = 0;
			msg[71] = 0;
				
			char ip_addr[20];
			sprintf(ip_addr, "%s", inet_ntoa(cliAddr.sin_addr));
			populate_new_client(ip_addr, msg);
#if 0			
			if (0 == check_duplicate_ip(ip_addr))
			{	
				sprintf(client_list[client_valid_num].node_name, "%s", &msg[0]);
				sprintf(client_list[client_valid_num].ip_addr, "%s", inet_ntoa(cliAddr.sin_addr));
				sprintf(client_list[client_valid_num].universe, &msg[20]);
				sprintf(client_list[client_valid_num].subnet, &msg[24]);
				client_valid_num++;
			}
#endif			
		}
	}
 

#if 0
 
  while(1) 
  {
	hf_thread_delay(2000);
	ratpac_get_str( CFG_str2id("AKS_NAME"), tmp_buff);
	sprintf(client_list[0].node_name, "%s", tmp_buff);
	ratpac_get_str( CFG_str2id("AKS_UNIVERSE"), tmp_buff);
	sprintf(client_list[0].universe, tmp_buff);	
	ratpac_get_str( CFG_str2id("AKS_SUBNET"), tmp_buff);
	sprintf(client_list[0].subnet, tmp_buff);
	client_list[0].age_cnt = 3;
	
	age_client_list(1);

#if 0	
   	if (0 == refresh_clients)
	{
		hf_thread_delay(1000);
		continue;
	}
#endif
	//hf_thread_delay(2000);
	
	if ((uint8_t)ipAddress[0] == 0)
	{
		continue;
	}
	
	remoteServAddr.sin_addr.s_addr = 0xff000000 | ((uint8_t)ipAddress[2] << 16) | ((uint8_t)ipAddress[1] << 8) | (uint8_t)ipAddress[0];
	
    /* init buffer */
	// refresh_clients = 0;
	// client_valid_num = 1;
	sprintf(msg, "Registering UDP broadcast message.\n");
	eprintf("Registering UDP broadcast message.\n");
	// continue;
	
	//CFG_get_str(CFG_str2id("WLN_Channel"),tmp_buff);
	//ap_wln_channel = (char)atoi(tmp_buff);
	msg[38] = ap_wln_channel; 
	
	rc = sendto(sd, msg, 40, 0, 
		  (struct sockaddr *) &remoteServAddr, 
  			sizeof(remoteServAddr));
	if ( rc < 0)
	{
		eprintf("Server Sendto fails\n");
		continue;
	}

	while (1)
	{
		FD_ZERO(&rset);
		FD_SET(sd,&rset);
		timeout.tv_sec= 3;
		timeout.tv_usec= 0;
		eprintf("Calling select for 3 seconds.\n");
		
		// hf_thread_delay(3000);
		rc = select(sd+1, &rset, NULL, NULL, &timeout);

		if (rc <= 0)
		{
			eprintf("No data within 3 seconds.\n");
			refresh_clients = 0;
			break;
		}
		eprintf("select return for 3 seconds.\n");
		if (FD_ISSET(sd, &rset))
		{
    		/* receive message */
			memset(msg,0x0,MAX_MSG);
    		cliLen = sizeof(cliAddr);
    		n = recvfrom(sd, msg, MAX_MSG, 0, 
		 		(struct sockaddr *) &cliAddr, &cliLen);

    		if(n<0) {
      			eprintf("cannot receive data \n");
      			continue;
    		}
  
    		/* print received message */
    		eprintf("from %s:UDP%u : len=%d \n", 
	   			inet_ntoa(cliAddr.sin_addr),
	   			ntohs(cliAddr.sin_port), n);
			for (i = 0; i < n; i++)
			{
				eprintf("%02x ", msg[i]);
			}
			eprintf("\r\n");
			msg[19] = 0;
			msg[22] = 0;
			msg[26] = 0;
			msg[36] = 0;
				
			char ip_addr[20];
			sprintf(ip_addr, "%s", inet_ntoa(cliAddr.sin_addr));
			populate_new_client(ip_addr, msg);
#if 0			
			if (0 == check_duplicate_ip(ip_addr))
			{	
				sprintf(client_list[client_valid_num].node_name, "%s", &msg[0]);
				sprintf(client_list[client_valid_num].ip_addr, "%s", inet_ntoa(cliAddr.sin_addr));
				sprintf(client_list[client_valid_num].universe, &msg[20]);
				sprintf(client_list[client_valid_num].subnet, &msg[24]);
				client_valid_num++;
			}
#endif			
		}
	}

  }/* end of server infinite loop */
  
#endif 

}

int check_duplicate_ip(char *ip_addr)
{
	int i;
	
	for (i = 0; i < client_valid_num; i++)
	{
		if (!strcmp(client_list[i].ip_addr, ip_addr))
		{
			return 1;
		}
	}
	return 0;
}

static char cli_recv[128]={0};
USER_FUNC static void client_thread_main(void* arg)
{
	int sd,id, rc;
	int tmp=1,recv_num=0;
	//char recv[128]={0};
	//char *p;
	struct sockaddr_in addr;
	struct sockaddr_in serv;
	//fd_set rset;
    //struct timeval timeout;

	id = (int)arg;

	eprintf("client_thread_main started\n");
	
#if 0	
	eprintf("Send AT+WANN\n");

	while (1)
	{
		if (artnet_enable == 0)
		{
			hf_thread_delay(1000);
			continue;
		}
		break;
	}
	artnet_enable = 0;
#endif

	// operation_mode = STA_ETH_MODE;
	hf_thread_delay(100);
	
#if 0	
	while (1)
	{
		// eprintf("Send AT+WANN\n");

 		rc = hfat_send_cmd("AT+WANN\r\n", sizeof("AT+WANN\r\n"),at_rsp,sizeof(at_rsp));
 
 		at_rsp[16] = 0;
		if (HF_SUCCESS != rc)
		{
			at_rsp[0] = 0;
			eprintf("WANN cmd fails\n");
			hf_thread_delay(1000);
			continue;
		}

		eprintf("PWANN resp: %s\n", at_rsp);
		if (strstr(at_rsp, "+ERR"))
		{
			eprintf("ERR resp\n");
			hf_thread_delay(1000);
			continue;
		}

		if (strstr(at_rsp, "+ok=DHCP,0.0.0.0"))
		{
			eprintf("No address\n");
			hf_thread_delay(500);
		   continue;
		}
		break;
	}
#endif	
	
	artnet_enable = 1;
	
	memset((char*)&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(REMOTE_CLIENT_PORT);
	addr.sin_addr.s_addr=htonl(INADDR_ANY);

	memset((char*)&serv,0,sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = htonl(0x0a0a64fe);
	serv.sin_port = htons(LOCAL_SERVER_PORT);
	

	// = socket()
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd<0) {
	  eprintf("client cannot open socket \n");

	}
	
	rc = bind(sd, (struct sockaddr*)&addr, sizeof(addr));

	if(rc<0) {
    	eprintf("cannot bind port number %d, error=%d \n", 
	   		REMOTE_CLIENT_PORT, rc);
		// close (sd);
    }
	tmp=1;
	setsockopt(sd, SOL_SOCKET,SO_BROADCAST,&tmp,sizeof(tmp));

	hfnet_set_udp_broadcast_port_valid(LOCAL_SERVER_PORT, REMOTE_CLIENT_PORT);

	while(1)
	{
#if 0		
		FD_ZERO(&rset);
		FD_SET(sd,&rset);
		timeout.tv_sec= 10;
		timeout.tv_usec= 0;
		rc = select(sd+1, &rset, NULL, NULL, &timeout);

		if (rc <= 0)
		{
			//eprintf("No data within 10 seconds.\n");
			//refresh_clients = 0;
			continue;
		}			
#endif
		hf_thread_delay(2000);
		
		if ((uint8_t)ipAddress[0] == 0)
		{
			continue;
		}
		
		if ( 1 /* FD_ISSET(sd, &rset) */)
		{
    		/* receive message */
#if 0		
			tmp = sizeof(addr);
			recv_num = recvfrom(sd, cli_recv, 96, 0, (struct sockaddr *)&addr, (socklen_t*)&tmp);
		
			if(recv_num <= 0)
			{
				continue;
			}
			
			//ap_wln_channel = cli_recv[38];
			cli_recv[recv_num] = 0;
			eprintf("thread %d, msg=%s, IP=%s\n",id, cli_recv, inet_ntoa(addr.sin_addr));
#endif
			
			//g_web_config.name[19] = 0;
			//g_web_config.universe[2] = 0;
			//eprintf("Node name = %s, Uni = %s\n", g_web_config.name, g_web_config.universe);
			ratpac_get_str( CFG_str2id("AKS_NAME"), &cli_recv[0]);
			ratpac_get_str( CFG_str2id("AKS_UNIVERSE"), &cli_recv[20]);
			ratpac_get_str( CFG_str2id("AKS_SUBNET"), &cli_recv[24]);
			sprintf(&cli_recv[30], "%s", battery_info);
			sprintf(&cli_recv[40], "%s", eCos_ver);
			sprintf(&cli_recv[48], "%s", samd_ver);
			sprintf(&cli_recv[56], "%s", timo_ver);
			ratpac_get_str( CFG_SACN_UNIV, &cli_recv[64]);
			ratpac_get_str( CFG_PROTNAME, &cli_recv[72]);
			ratpac_get_str( CFG_TIMO_POWER, &cli_recv[76]);
			ratpac_get_str( CFG_SORT_BY, &cli_recv[80]);				
			sprintf(&cli_recv[84], "%d", operation_mode);			
			cli_recv[36] = 0;
			//sprintf(&cli_recv[0], "AKS" );
			//sprintf(&cli_recv[20], "10");
#if 0
			for (i = 0; i < 32; i++)
			{
				eprintf("%02x ", recv[i]);
			}
			eprintf("\r\n");
#endif
			// serv.sin_addr.s_addr = addr.sin_addr.s_addr;
			serv.sin_addr.s_addr = 0xff000000 | ((uint8_t)ipAddress[2] << 16) | ((uint8_t)ipAddress[1] << 8) | (uint8_t)ipAddress[0];			
			rc = sendto(sd, cli_recv, 96, 0, 
				(struct sockaddr *) &serv, 
				sizeof(serv));
			if ( rc < 0)
			{
				eprintf("Client Sendto server fails\n");
			}
		}   // if (1)
	}

}
int get_client_entry(int idx, char *node_name, char *ip_addr, char *universe, char *art_sub, char *battery, char *protocol, char *sacn_uni, char *sort_by, char *host_mode, char *timopower)
{
	struct client_ent *client_ptr;
	
	if (idx >= client_valid_num)
	{
		node_name[0] = 0;
		ip_addr[0] = 0;
		universe[0] = 0;
		return -1;
	}
	
	client_ptr = sort_array[idx];

	sprintf(node_name, "%s", client_ptr->node_name);
	if (strlen(node_name) == 0)
	{
		sprintf(node_name,"NoName");
	}
	sprintf(ip_addr, "%s", client_ptr->ip_addr);
	sprintf(universe, "%s", client_ptr->universe);
	sprintf(art_sub, "%s", client_ptr->subnet);

	sprintf(protocol, "%s", client_ptr->protocol);
	sprintf(sacn_uni, "%s", client_ptr->sacn_univ);
	sprintf(sort_by, "%s", client_ptr->sort_by);
	sprintf(host_mode, "%s", client_ptr->host_mode);
	sprintf(timopower, "%s", client_ptr->crmx_power);
	
	//sprintf(battery, "%s", battery_info);
	snprintf(battery, 8, "%s%%", client_ptr->baterry);
#if 0	
	sprintf(node_name, "%s", client_valid_list[idx].node_name);
	if (strlen(node_name) == 0)
	{
		sprintf(node_name,"NoName");
	}
	sprintf(ip_addr, "%s", client_valid_list[idx].ip_addr);
	sprintf(universe, "%s", client_valid_list[idx].universe);
	sprintf(art_sub, "%s", client_valid_list[idx].subnet);

	sprintf(protocol, "%s", client_valid_list[idx].protocol);
	sprintf(sacn_uni, "%s", client_valid_list[idx].sacn_univ);
	sprintf(sort_by, "%s", client_valid_list[idx].sort_by);
	sprintf(host_mode, "%s", client_valid_list[idx].host_mode);
	sprintf(timopower, "%s", client_valid_list[idx].crmx_power);
	
	//sprintf(battery, "%s", battery_info);
	snprintf(battery, 8, "%s%%", client_valid_list[idx].baterry);
#endif	
	return 0;
}

int get_client_entry_adva(int idx, char *node_name, char *ip_addr, char *ecos_ver, char *samd_ver, char *timo_ver)
{
	if (idx >= client_valid_num)
	{
		node_name[0] = 0;
		ip_addr[0] = 0;
		ecos_ver[0] = 0;
		samd_ver[0] = 0;
		timo_ver[0] = 0;
		return -1;
	}
	sprintf(node_name, "%s", client_valid_list[idx].node_name);
	if (strlen(node_name) == 0)
	{
		sprintf(node_name,"NoName");
	}
	sprintf(ip_addr, "%s", client_valid_list[idx].ip_addr);
	sprintf(ecos_ver, "%s", client_valid_list[idx].eCos_ver);
	sprintf(samd_ver, "%s", client_valid_list[idx].samd_ver);	
	sprintf(timo_ver, "%s", client_valid_list[idx].timo_ver);

	return 0;
}

void do_refresh_clients(void)
{
	refresh_clients = 1;
	
	return;
	
	while (1)
	{
		if ( 0 == refresh_clients)
		{
			break;
		}
		hf_thread_delay(1000);
	}
}

int refresh_client_done(void)
{
	if ( 0 == refresh_clients)
	{
		return 1;
	}
	return 0;
}

void check_gaffer_packet(char *data, uint32_t len)
{
	char Uni[4]={0};
	int uni_num, art_sub, lower, upper;
	char *tmp;

	// caution: need to check if the artnet packet has 512 bytes data, or need to cehck out of range when copying
	if (len < 50)
	{
		return;				// 50 magic number for arnet data packet
	}
	ratpac_get_str(CFG_str2id("GAFFER_ENABLE"),Uni);
	if (Uni[0] == '1')      // Gaffer is disabled
	{
		return;
	}
	ratpac_get_str(CFG_str2id("GAFFER_UNIVERSE"),Uni);	
	uni_num = atoi(Uni);;
	ratpac_get_str(CFG_str2id("GAFFER_SUBNET"),Uni);
	art_sub = atoi(Uni);
	art_sub <<= 4;
	uni_num = ((uni_num | art_sub) & 0xff);
	if(uni_num == (int)data[14])
	{
		ratpac_get_str(CFG_str2id("GAFFER_LOWER"),Uni);
		lower = atoi(Uni);
 		ratpac_get_str(CFG_str2id("GAFFER_UPPER"),Uni);
		upper = atoi(Uni);
		tmp = &data[17];											// lower start from 1, upper is 512
		memmove(&gaffer_data[lower], &tmp[lower], upper-lower+1);  // when lower==upper, it is 1 byte
	}
}

void replace_channel_data(char *data, uint32_t len) 
{
	char Uni[4]={0};
	int lower, upper;
	char *tmp;
	
	ratpac_get_str(CFG_str2id("GAFFER_ENABLE"),Uni);
	if (Uni[0] == '1')      // Gaffer is disabled
	{
		return;
	}
	ratpac_get_str(CFG_str2id("GAFFER_LOWER"),Uni);
	lower = atoi(Uni);
 	ratpac_get_str(CFG_str2id("GAFFER_UPPER"),Uni);
	upper = atoi(Uni);
	tmp = &data[17];											// lower start from 1, upper is 512
	memmove(&tmp[lower], &gaffer_data[lower], upper-lower+1);  // when lower==upper, it is 1 byte	
}

int ratpac_get_str(int id, char *val)
{
	*val = 0;
	switch (id)
	{
		case CFG_AKS_NAME:
			sprintf(val, "%s", g_web_config.name);
			break;
		case CFG_AKS_UNIVERSE:
			sprintf(val, "%s", g_web_config.universe);
			break;		
		case CFG_AKS_SUBNET:
			sprintf(val, "%s", g_web_config.subnet);
			break;		
		case CFG_GAFFER_ENABLE:
			snprintf(val, 2, "%s", g_web_config.gaffer_enb);
			break;		
		case CFG_GAFFER_UNIVERSE:
			sprintf(val, "%s", g_web_config.gaffer_univ);
			break;		
		case CFG_GAFFER_SUBNET:
			sprintf(val, "%s", g_web_config.gaffer_sub);
			break;		
		case CFG_GAFFER_LOWER:
			sprintf(val, "%s", g_web_config.gaffer_lower);
			break;		
		case CFG_GAFFER_UPPER:
			sprintf(val, "%s", g_web_config.gaffer_upper);
			break;
		case CFG_TIMO_POWER:
			sprintf(val, "%s", g_web_config.timopower);		
			break;
		case CFG_AKS_CHANNEL_WIDTH:
			sprintf(val, "%s", g_web_config.channelWidth);			
			break;
		case CFG_AKS_SECOND_CHANNEL:
			sprintf(val, "%s", g_web_config.secondChannel);			
			break;	
		case CFG_SACN_OUTPUT:
			sprintf(val,"%s",g_web_config.sacn_enb );
			break;
		case CFG_SACN_UNIV:
			sprintf(val,"%s", g_web_config.sacn_universe);
			break;	
		case CFG_PROTNAME:
			snprintf(val, 2, "%s", g_web_config.protname);
			break;
		case CFG_LED_COLOR:
			snprintf(val, 16, "%s", g_web_config.led_color);
			break;
		case CFG_LED_G:
			snprintf(val, 16, "%s", g_web_config.led_g);
			break;
		case CFG_LED_B:
			snprintf(val, 16, "%s", g_web_config.led_b);
			break;					
		case CFG_AKS_USE_XLR:
			snprintf(val, 2, "%s", g_web_config.use_xlr);			
			break;
		case CFG_AKS_MAIN_XLR:	
			snprintf(val, 2, "%s", g_web_config.main_xlr);			
			break;
		case CFG_SORT_BY:	
			snprintf(val, 2, "%s", g_web_config.sort_by);			
			break;				
		case CFG_AKS_CONS_NUM:
			snprintf(val, 2, "%d", client_valid_num);
			break;
		default:
			return -1;
	}
	if (strlen(val) == 0)
		sprintf(val, "0");
	return 0;
}

int ratpac_set_str(int id, char *val)
{
	switch (id)
	{
		case CFG_AKS_NAME:
			snprintf(g_web_config.name, 16, "%s", val);
			break;
		case CFG_AKS_UNIVERSE:
			sprintf(g_web_config.universe, "%s", val);
			break;		
		case CFG_AKS_SUBNET:
			sprintf(g_web_config.subnet, "%s", val);
			break;		
		case CFG_GAFFER_ENABLE:
			sprintf(g_web_config.gaffer_enb, "%s", val);
			break;		
		case CFG_GAFFER_UNIVERSE:
			sprintf(g_web_config.gaffer_univ, "%s", val);
			break;		
		case CFG_GAFFER_SUBNET:
			sprintf(g_web_config.gaffer_sub, "%s", val);
			break;		
		case CFG_GAFFER_LOWER:
			sprintf(g_web_config.gaffer_lower, "%s", val);
			break;		
		case CFG_GAFFER_UPPER:
			sprintf(g_web_config.gaffer_upper, "%s", val);
			break;
		case CFG_TIMO_POWER:
			sprintf(g_web_config.timopower, "%s", val);		
			break;
		case CFG_AKS_CHANNEL_WIDTH:
			sprintf(g_web_config.channelWidth, "%s", val);		
			break;
		case CFG_AKS_SECOND_CHANNEL:
			sprintf(g_web_config.secondChannel, "%s", val);		
			break;
		case CFG_SACN_OUTPUT:
			sprintf(g_web_config.sacn_enb,"%s", val);		
			break;
		case CFG_SACN_UNIV:
			sprintf(g_web_config.sacn_universe,"%s", val);
			break;
		case CFG_PROTNAME:
			sprintf(g_web_config.protname, "%s", val);
			break;
		case CFG_LED_COLOR:
			sprintf(g_web_config.led_color, "%s", val);
			break;
		case CFG_LED_G:
			sprintf(g_web_config.led_g, "%s", val);
			break;
		case CFG_LED_B:
			sprintf(g_web_config.led_b, "%s", val);
			break;			
		case CFG_AKS_USE_XLR:
			snprintf(g_web_config.use_xlr, 2, "%s",val);			
			break;
		case CFG_AKS_MAIN_XLR:
			snprintf(g_web_config.main_xlr, 2, "%s", val);			
			break;
		case CFG_SORT_BY:
			snprintf(g_web_config.sort_by, 2, "%s", val);
			break;		
		default:
			return -1;
	}
	//write_data_to_flash();
	return 0;
}

void set_artnet_enable(int val)
{
	artnet_enable = val;			// no longer need to control arnet frame going to the SAMD, since all the task priority is same, AKS_PRIORITY
#if 0	
	if (0 == val)
	{
		cyg_mutex_lock(&samd_mutex);
	}
	else
	{
		cyg_mutex_unlock(&samd_mutex);
	}
#endif	
}

void clear_uart_recv(void)
{
	uart_rvc_done = 0;
	uart_rvc_len = 0;
}

int Send_SAMD_CMD(char *cmd, int len, char *expect_resp, cyg_tick_count_t timeout_tick)
{
	int try_snd;
	cyg_tick_count_t cur_tick;
	//static cyg_handle_t webHandle;
	
	expect_resp[0] = 0;		// for the safe side
	
	if ( 0 == len)
	{
		return (-100);
	}
	
	//cyg_mutex_lock(&samd_mutex);			// since userMain and web server can call this function, and wait for global response  uart_rcv data
	//set_artnet_enable(0);
	//webHandle = cyg_thread_self();
	//cyg_thread_set_priority(webHandle, 14 );
	for (try_snd = 0; try_snd < 2; try_snd++)
	{
		clear_uart_recv();
		hfuart_send(HFUART0, cmd, len, 100);
		
		cur_tick = cyg_current_time();
	
		// for (i = 0; i < 1200; i++)
		while (	(cyg_current_time() - cur_tick) < timeout_tick)   // every tick is 10ms
		{
			//hf_thread_delay(2);
			if ( 1 == uart_rvc_done)
				break;
			cyg_thread_yield();
			//msleep(20);
		}
		if (0 == uart_rvc_done)
		{
			//Joo_uart_send("SAMD no response\n");
			continue;
		}
		uart_rvc_done = 1;		// no longer need data for now
		uart_rcv_data[31] = 0; // for the safe side
		memmove(expect_resp, uart_rcv_data,uart_rvc_len );
		expect_resp[uart_rvc_len] = 0;
		//set_artnet_enable(1);
		//cyg_thread_set_priority(webHandle, 8 );
		//cyg_mutex_unlock(&samd_mutex);
		return 0;
	}
	//set_artnet_enable(1);
	//cyg_thread_set_priority(webHandle, 8 );
	//cyg_mutex_unlock(&samd_mutex);
	uart_rvc_done = 1;		// no longer need data for now
	return (-101);
}

#define PKT_SIZE 512
unsigned char samd_pkt_buff[PKT_SIZE+16];
unsigned char align_buff[PKT_SIZE+16];
static unsigned char expect_buff[8] = {'o', 'k', ':', 0, 0};
static char ret_buff[32];
static char firmware_pad[32 * 1024];
static int pad_len;

#define NUM_TRY 2

int SAMD_firmware_download(unsigned char *firmware, int len, int which)
{
	int ret, try;
	//unsigned char cksum, code_cksum;
	uint32_t cksum, code_cksum;
	uint32_t *pSrc, *pDst;
	//int cpy_len;
	unsigned char bcksum, *pCksum;
	int remain_len = len;
	int total_len = 0;
	int i;
	
	
	char Settings[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 7};
	char done_msg[7] = {'d','o','n','e', 0x1 , 0x2, 0x3};
#if 0	
	static int first_time = 0;
	static cyg_priority_t pri;
	static cyg_handle_t uartHandle;
		
	if (0 == first_time)
	{
		uartHandle = cyg_thread_self();
		pri = cyg_thread_get_priority(uartHandle);
		eprintf ("WEB Thread Pri = %d\n", (int)pri);
		pri = cyg_thread_get_current_priority(uartHandle);
		eprintf ("WEB Thread Cur Pri = %d\n", (int)pri);
		first_time = 1;
	}
#endif

	pad_len = ((len + PKT_SIZE - 1) & 0xfe00);
	remain_len = pad_len;
	
	for (i = 0; i < len; i++)
	{
		firmware_pad[i] = firmware[i];
	}
	for (i = len; i < pad_len; i++)
	{
		firmware_pad[i] = 0;
	}
	
	if (0 == which)
	{
		Settings[12] = 7;
	}
	else
	{
		Settings[12] = 8;
	}
	set_artnet_enable(0);
	hf_thread_delay(2000);		// make sure all the command (battery) waiting for response has finished
	//msleep(20);				// make sure at least 20ms gap between two artnet packet
	
	cyg_mutex_lock(&samd_mutex);
	for (i = 0; i < NUM_TRY; i++)  // try two times because the SAMD jumps 0 and re-init uart always send some garbage at the beginning, the first always fails
	{
		ret = Send_SAMD_CMD(Settings, sizeof(Settings), ret_buff, 200);
		if (ret != 0)
		{
			//cyg_mutex_unlock(&samd_mutex);
			//set_artnet_enable(1);
			//return (ret);
			continue;
		}
		if (!strstr(ret_buff,"update"))
		{
			if ((which == 1) || (i == 1) )
			{
				cyg_mutex_unlock(&samd_mutex);
				set_artnet_enable(1);
				return (-102);
			}
			hf_thread_delay(1000);    // garbage data
		}
		else
		{
			break;
		}
	}
	if (ret != 0)
	{
		cyg_mutex_unlock(&samd_mutex);
		set_artnet_enable(1);
		return (ret);
	}	
	ret = Send_SAMD_CMD("yes", strlen("yes"), ret_buff, 100);
	if (ret != 0)
	{
		cyg_mutex_unlock(&samd_mutex);
		set_artnet_enable(1);
		return (ret);
	}
	
	if (!strstr(ret_buff,"ok"))
	{
		cyg_mutex_unlock(&samd_mutex);
		set_artnet_enable(1);
		return (-103);
	}
	
	total_len = 0;
	code_cksum = 0;
	pCksum = (unsigned char *)&cksum;
	
	while (remain_len)
	{
#if 0		
		cpy_len = ((remain_len >= PKT_SIZE) ? PKT_SIZE : remain_len);
		cksum = 0;
		for (i = 0; i < cpy_len; i++)
		{
			cksum ^= firmware[total_len + i];
			pkt_buff[i] = firmware[total_len + i];
		}
		if (cpy_len < PKT_SIZE)
		{
			memset(&pkt_buff[i], 0x0, (PKT_SIZE - cpy_len));
		}
		pkt_buff[PKT_SIZE] = cksum;
		remain_len -= cpy_len;
		total_len += cpy_len;
		code_cksum ^= cksum;
#endif
		//pSrc = (uint32_t *)&firmware[total_len];
		pSrc = (uint32_t *)align_buff;
		pDst = (uint32_t *)samd_pkt_buff;
		cksum = 0x0;

		//memset(align_buff, 0x0, 512);
		for (i = 0; i < 512; i++)
		{
			//cksum ^= pSrc[i];
			//*pDst++ = *pSrc++;
			//pDst[i] = pSrc[i];
			align_buff[i] = firmware_pad[total_len + i];
		}
		for (i = 0; (i < PKT_SIZE >> 2); i++)
		{
			cksum ^= *pSrc;
			*pDst++ = *pSrc++;
			//pDst[i] = pSrc[i];
			//samd_pkt_buff[i] = firmware[total_len + i];
		}		
		bcksum = pCksum[0] ^ pCksum[1] ^ pCksum[2] ^ pCksum[3];
		samd_pkt_buff[PKT_SIZE] = bcksum;
		total_len += PKT_SIZE;
		remain_len -= PKT_SIZE;
		code_cksum ^= cksum;
	
		for (try = 0; try < 2; try++)
		{
			ret = Send_SAMD_CMD(samd_pkt_buff, PKT_SIZE+1, ret_buff, 200);
			if (ret != 0)
			{
				cyg_mutex_unlock(&samd_mutex);
				set_artnet_enable(1);
				return (ret);
			}
			if (!strstr(ret_buff,"ok"))
			{
				if (0 == try)
					continue;
				cyg_mutex_unlock(&samd_mutex);
				set_artnet_enable(1);
				return (-104);				
			}
			break;
		}
	}
	pCksum = (unsigned char *)&code_cksum;
	bcksum = pCksum[0] ^ pCksum[1] ^ pCksum[2] ^ pCksum[3];
	done_msg[4] = len & 0xff;
	done_msg[5] = (len >> 8) & 0xff;
	//done_msg[6] = code_cksum;
	done_msg[6] = bcksum;	
	Send_SAMD_CMD(done_msg, 7, ret_buff, 200);
	cyg_mutex_unlock(&samd_mutex);
	set_artnet_enable(1);
	return 0;
}

void get_eCos_ver(char *buffer)
{
	strcpy(buffer, eCos_ver);
}

void get_SAMD_ver(char *buffer)
{
	strcpy(buffer, samd_ver);
}

void get_TIMO_ver(char *buffer)
{
	strcpy(buffer, timo_ver);
}

void get_SysUpTime(char *buffer)
{
	extern unsigned int dmx_count;
	
	sprintf(buffer,"%llu - %d", cyg_current_time(), dmx_count);
}				

void get_eCos_rel_build(char *buffer)
{
	sprintf(buffer, "Release Date: %s Ver:%s", rel_date, eCos_ver);
}

void get_battery_info(char *buffer)
{
	battery_info[6] = 0;
	//eprintf("!!Battery : %s\n", battery_info);
	//strcpy(buffer, battery_info);
	sprintf(buffer,"%s", battery_info);
	//eprintf("**Battery : %s\n", buffer);
}

//char add_print[32];
void get_module_ipaddress_mode(char *ip, char *mode)
{
	struct in_addr   sin_addr;     // see struct in_addr, below
	char *tmp;
	
	sin_addr.s_addr = ((uint8_t)ipAddress[3] << 24) | ((uint8_t)ipAddress[2] << 16) | ((uint8_t)ipAddress[1] << 8) | (uint8_t)ipAddress[0];
	//sprintf(add_print, "%02x %02x %02x %02x\n", (uint8_t)ipAddress[0], (uint8_t)ipAddress[1], (uint8_t)ipAddress[2], (uint8_t)ipAddress[3]);
	//hfuart_send(HFUART0, add_print,strlen(add_print),100);
	tmp = inet_ntoa(sin_addr);
	snprintf(ip, 20, "%s", tmp);
	
	if (AP_MODE == operation_mode)
	{
		sprintf(mode, "WiFi Server");
	}
	else if (STA_ETH_MODE == operation_mode)
	{
		sprintf(mode, "Eth Client");
	}
	else
	{
		sprintf(mode, "WiFi Client");
	}	
}

int Send_Link_Command(void)
{
	int ret;

	char Settings[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 3, 2};
	
	//set_artnet_enable(0);
	cyg_mutex_lock(&samd_mutex);
	ret = Send_SAMD_CMD(Settings, sizeof(Settings), ret_buff, 200);
	if (ret != 0)
	{
		cyg_mutex_unlock(&samd_mutex);
		//set_artnet_enable(1);
		return (ret);
	}
	if (strstr(ret_buff,"ok"))
	{
		cyg_mutex_unlock(&samd_mutex);
		//set_artnet_enable(1);		
		return (0);
	}
	cyg_mutex_unlock(&samd_mutex);
	//set_artnet_enable(1);
	return (-106);
}

int Send_UnLink_Command(void)
{
	int ret;
	
	char Settings[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 3, 1};

	//set_artnet_enable(0);
	cyg_mutex_lock(&samd_mutex);
	ret = Send_SAMD_CMD(Settings, sizeof(Settings), ret_buff, 200);
	if (ret != 0)
	{
		//set_artnet_enable(1);
		cyg_mutex_unlock(&samd_mutex);
		return (ret);
	}
	if (strstr(ret_buff,"ok"))
	{
		//set_artnet_enable(1);
		cyg_mutex_unlock(&samd_mutex);
		return (0);
	}
	//set_artnet_enable(1);
	cyg_mutex_unlock(&samd_mutex);
	return (-106);	
}

int Send_ID_Command(void)
{
	int ret;

	char Settings[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 4};
	
	//set_artnet_enable(0);
	cyg_mutex_lock(&samd_mutex);
	ret = Send_SAMD_CMD(Settings, sizeof(Settings), ret_buff, 200);
	if (ret != 0)
	{
		//set_artnet_enable(1);
		cyg_mutex_unlock(&samd_mutex);
		return (ret);
	}
	if (strstr(ret_buff,"ok"))
	{
		//set_artnet_enable(1);
		cyg_mutex_unlock(&samd_mutex);	
		return (0);
	}
	//set_artnet_enable(1);
	cyg_mutex_unlock(&samd_mutex);
	return (-106);
}

int Send_Battery_Command(void)
{
	int ret;
	const char s[] = "\n\r";
	char *token;

	char Settings[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 5};
	
	//set_artnet_enable(0);
	cyg_mutex_lock(&samd_mutex);
	ret = Send_SAMD_CMD(Settings, sizeof(Settings), ret_buff, 100);
	if (ret != 0)
	{
		//set_artnet_enable(1);
		cyg_mutex_unlock(&samd_mutex);
		return (ret);
	}
	samd_ver[0] = ret_buff[0];
	samd_ver[2] = ret_buff[2];
	//hfuart_send(HFUART0, ret_buff,6,100);  //for debug
	sprintf(battery_info, "%s", (char *)&ret_buff[3]);
	//hfuart_send(HFUART0, battery_info,6,100);  //for debug
	//sp = strchr(battery_info, ' ');
	//if (sp)
	//{
	//	*sp = 0;
	//}
	token = strtok((char *)battery_info, s);
	battery_info[6] = 0;     // remove LF and CR sent by SAMD, or the web displayMsg cannot display	
	cyg_mutex_unlock(&samd_mutex);
	//set_artnet_enable(1);
	return (0);
}

int Send_Gaffer_Command(void)
{
	int ret;

	char Settings[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 6};
	ret = Send_SAMD_CMD(Settings, sizeof(Settings), ret_buff, 200);
	return (ret);
}

void save_valid_client_entry(int i)
{
	client_valid_list[client_valid_num] = client_list[i];
	client_valid_num++;
}


void age_client_list(int new)
{
	int i;
	struct in_addr   sin_addr;     // see struct in_addr, below
	char *tmp;
	
	ratpac_get_str( CFG_str2id("AKS_NAME"), tmp_buff);
	sprintf(client_list[0].node_name, "%s", tmp_buff);
	ratpac_get_str( CFG_str2id("AKS_UNIVERSE"), tmp_buff);
	sprintf(client_list[0].universe, tmp_buff);	
	ratpac_get_str( CFG_str2id("AKS_SUBNET"), tmp_buff);
	sprintf(client_list[0].subnet, tmp_buff);
	ratpac_get_str( CFG_SACN_UNIV, tmp_buff);
	sprintf(client_list[0].sacn_univ, tmp_buff);
	ratpac_get_str( CFG_SORT_BY, tmp_buff);
	sprintf(client_list[0].sort_by, tmp_buff);
	ratpac_get_str( CFG_TIMO_POWER, tmp_buff);
	sprintf(client_list[0].crmx_power, tmp_buff);
	ratpac_get_str( CFG_PROTNAME, tmp_buff);
	sprintf(client_list[0].protocol, tmp_buff);	
	
	sprintf(client_list[0].eCos_ver, "%s", eCos_ver);
	sprintf(client_list[0].samd_ver, "%s", samd_ver);
	sprintf(client_list[0].timo_ver, "%s", timo_ver);
	sprintf(client_list[0].host_mode, "%d", operation_mode);	
	client_list[0].age_cnt = 3;
	
	client_valid_list[0] = client_list[0];
	snprintf(client_valid_list[0].baterry, 7, "%s", battery_info);
	
	sin_addr.s_addr = ((uint8_t)ipAddress[3] << 24) | ((uint8_t)ipAddress[2] << 16) | ((uint8_t)ipAddress[1] << 8) | (uint8_t)ipAddress[0];
	tmp = inet_ntoa(sin_addr);
	snprintf(client_valid_list[0].ip_addr, 20, "%s", tmp);
	snprintf(this_node_ip_address, 20, "%s", tmp);	

	client_valid_num = 1;
	
	for (i = 1; i < MAX_NUM_ENTRY; i++)
	{	
		if (client_list[i].age_cnt > 0)
		{
			if (new)
			{
				save_valid_client_entry(i);
			}
			else if (--client_list[i].age_cnt > 0)
			{
				save_valid_client_entry(i);
			}
		}
	}
}


void my_ip_address(void)
{
	char *tmp;
	struct in_addr sin_addr;     // see struct in_addr, below
	sin_addr.s_addr = ((uint8_t)ipAddress[3] << 24) | ((uint8_t)ipAddress[2] << 16) | ((uint8_t)ipAddress[1] << 8) | (uint8_t)ipAddress[0];
	tmp = inet_ntoa(sin_addr);
	snprintf(client_valid_list[0].ip_addr, 20, "%s", tmp);	
}

#define AGE_COUNT	3
void populate_new_client(char *ip_ad, char *rcv_msg)
{
	int i;
	
	// set my ipaddress in case at start up, it is zero, since this value is get from the main loop
	my_ip_address();
	if (!strcmp(client_valid_list[0].ip_addr, ip_ad))
	{
		return;
	}
	
	for (i = 1; i < MAX_NUM_ENTRY; i++)
	{
		if ((!strcmp(client_list[i].ip_addr, ip_ad)) && (client_list[i].age_cnt > 0))
		{
			sprintf(client_list[i].node_name, "%s", &rcv_msg[0]);
			//sprintf(client_list[i].ip_addr, "%s", inet_ntoa(cliAddr.sin_addr));
			sprintf(client_list[i].universe, &rcv_msg[20]);
			sprintf(client_list[i].subnet, &rcv_msg[24]);
			sprintf(client_list[i].baterry, &rcv_msg[30]);
			sprintf(client_list[i].eCos_ver, &rcv_msg[40]);
			sprintf(client_list[i].samd_ver, &rcv_msg[48]);
			sprintf(client_list[i].timo_ver, &rcv_msg[56]);
			sprintf(client_list[i].sacn_univ, &rcv_msg[64]);
			sprintf(client_list[i].protocol, &rcv_msg[72]);
			sprintf(client_list[i].crmx_power, &rcv_msg[76]);
			sprintf(client_list[i].sort_by, &rcv_msg[80]);
			sprintf(client_list[i].host_mode, &rcv_msg[84]);			
			client_list[i].age_cnt = AGE_COUNT;	
			age_client_list(1);
			return;
		}
	}
	
	
	for (i = 1; i < MAX_NUM_ENTRY; i++)
	{
		if (client_list[i].age_cnt == 0)
		{
			sprintf(client_list[i].node_name, "%s", &rcv_msg[0]);
			sprintf(client_list[i].ip_addr, "%s", ip_ad);
			sprintf(client_list[i].universe, &rcv_msg[20]);
			sprintf(client_list[i].subnet, &rcv_msg[24]);
			sprintf(client_list[i].baterry, &rcv_msg[30]);
			sprintf(client_list[i].eCos_ver, &rcv_msg[40]);
			sprintf(client_list[i].samd_ver, &rcv_msg[48]);
			sprintf(client_list[i].timo_ver, &rcv_msg[56]);
			sprintf(client_list[i].sacn_univ, &rcv_msg[64]);
			sprintf(client_list[i].protocol, &rcv_msg[72]);
			sprintf(client_list[i].crmx_power, &rcv_msg[76]);
			sprintf(client_list[i].sort_by, &rcv_msg[80]);
			sprintf(client_list[i].host_mode, &rcv_msg[84]);			
			client_list[i].age_cnt = AGE_COUNT;
			age_client_list(1);
			return;
		}
	}	
}

void sort_client_list(void)
{
	int i;
	
	for (i = 0; i < client_valid_num; i++)
	{
		sort_array[i] = &client_valid_list[i];
	}
	// sort the sort_array
}	

