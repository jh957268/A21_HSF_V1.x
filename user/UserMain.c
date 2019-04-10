
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


void Joo_uart_send(char *data);

static void server_thread_main(void* arg);

USER_FUNC static void client_thread_main(void* arg);

void check_gaffer_packet(char *data, uint32_t len);
void replace_channel_data(char *data, uint32_t len);

static int refresh_clients;
static char gaffer_data[514];

// #define debug(format, args...) fprintf (stderr, format, args)

#if 0
#define eprintf(fmt, args...) do {
							sprintf (ebuffer, fmt, args);
							Joo_uart_send((char *)ebuffer);
						}

#endif

#define E_DEBUG_PRINT	1

#if E_DEBUG_PRINT
char ebuffer[128];
#define eprintf(fmt, ...) do {  \
	                               sprintf(ebuffer, fmt, ## __VA_ARGS__); \
	                               Joo_uart_send((char *)ebuffer); \                               
	                             } while (0)
#else
#define eprintf(fmt, ...)
#endif
	
char at_rsp[64] = {0};

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
		CFG_get_str( CFG_str2id("AKS_NAME"), temp_buf);
		sprintf(rsp, "=%s", temp_buf);
	}
	else
	{
		CFG_set_str( CFG_str2id("AKS_NAME"), argv[0]);
		CFG_save(0);
	}
	return 0;
}

static int artnet_enable = 0;

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

static int USER_FUNC cmd_web_para_universe(pat_session_t s,int argc,char *argv[],char *rsp,int len)
{
	int ret = 0;
	char temp_buf[100] = {0};
	int temp_value = -1;

	if(argc > 1)
		return -1;
	
	if(0 == argc)
	{
		CFG_get_str( CFG_str2id("AKS_UNIVERSE"), temp_buf);
		sprintf(rsp, "=%s", temp_buf);
	}
	else
	{
		temp_value = atoi(argv[0]);
		printf("universe_value=%s_%d\n",argv[0],temp_value);
		if((temp_value>=0)&&(temp_value<=9))
		{
			CFG_set_str( CFG_str2id("AKS_UNIVERSE"), argv[0]);
			CFG_save(0);
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
		CFG_get_str( CFG_str2id("TIMO_POWER"), temp_buf);
		sprintf(rsp, "=%s", temp_buf);
	}
	else
	{
		temp_value = atoi(argv[0]);
		printf("TIMO_POWER=%s_%d\n",argv[0],temp_value);
		if((temp_value>=0)&&(temp_value<=5))
		{
			CFG_set_str( CFG_str2id("TIMO_POWER"), argv[0]);
			CFG_save(0);
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
		CFG_get_str( CFG_str2id("AKS_CHANNEL_WIDTH"), temp_buf);
		sprintf(rsp, "=%s", temp_buf);
	}
	else
	{
		temp_value = atoi(argv[0]);
		printf("AKS_CHANNEL_WIDTH=%s_%d\n",argv[0],temp_value);
		if((temp_value>=9)&&(temp_value<=18))
		{
			CFG_set_str( CFG_str2id("AKS_CHANNEL_WIDTH"), argv[0]);
			CFG_save(0);
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
		CFG_get_str( CFG_str2id("AKS_SECOND_CHANNEL"), temp_buf);
		sprintf(rsp, "=%s", temp_buf);
	}
	else
	{
		temp_value = atoi(argv[0]);
		printf("AKS_SECOND_CHANNEL=%s_%d\n",argv[0],temp_value);
		if((temp_value>=0)&&(temp_value<=11))
		{
			CFG_set_str( CFG_str2id("AKS_SECOND_CHANNEL"), argv[0]);
			CFG_save(0);
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
		CFG_get_str( CFG_str2id("AKS_BIT_SETTINGS"), temp_buf);
		sprintf(rsp, "=%s", temp_buf);
	}
	else
	{
		temp_value = atoi(argv[0]);
		printf("AKS_BIT_SETTINGS=%s_%d\n",argv[0],temp_value);
		if((temp_value>=0)&&(temp_value<=1))
		{
			CFG_set_str( CFG_str2id("AKS_BIT_SETTINGS"), argv[0]);
			CFG_save(0);
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
	int temp_value = -1;
	
	memset(&g_web_config,0,sizeof(WEB_DATA_T));
	hfuflash_read(WEB_DATA_FLASH_ADDR,&g_web_config,sizeof(WEB_DATA_T));

	printf("___ g_web_config.name=%s_%d\n",g_web_config.name,strlen(g_web_config.name));
	if(strlen(g_web_config.name)>0)
	{
		CFG_set_str( CFG_str2id("AKS_NAME"), g_web_config.name);
	}
	else
	{
		CFG_set_str( CFG_str2id("AKS_NAME"), "Ratpac AKS");
	}
	
	temp_value = atoi(g_web_config.universe);
	printf("___ g_web_config.universe=%s_%d\n",g_web_config.universe,temp_value);
	if((temp_value>0)&&(temp_value<=15))
	{
		CFG_set_str( CFG_str2id("AKS_UNIVERSE"), g_web_config.universe);
	}
	else
	{
		CFG_set_str( CFG_str2id("AKS_UNIVERSE"), "0");
	}
	
	temp_value = atoi(g_web_config.timopower);
	printf("___ g_web_config.timopower=%s_%d\n",g_web_config.timopower,temp_value);
	if((temp_value>0)&&(temp_value<=5))
	{
		CFG_set_str( CFG_str2id("TIMO_POWER"), g_web_config.timopower);
	}
	else
	{
		CFG_set_str( CFG_str2id("TIMO_POWER"), "3");
	}

	temp_value = atoi(g_web_config.channelWidth);
	printf("___ g_web_config.channelWidth=%s_%d\n",g_web_config.channelWidth,temp_value);
	if((temp_value>9)&&(temp_value<=18))
	{
		CFG_set_str( CFG_str2id("AKS_CHANNEL_WIDTH"), g_web_config.channelWidth);
	}
	else
	{
		CFG_set_str( CFG_str2id("AKS_CHANNEL_WIDTH"), "16");
	}

	temp_value = atoi(g_web_config.secondChannel);
	printf("___ g_web_config.secondChannel=%s_%d\n",g_web_config.secondChannel,temp_value);
	if((temp_value>0)&&(temp_value<=11))
	{
		CFG_set_str( CFG_str2id("AKS_SECOND_CHANNEL"), g_web_config.secondChannel);
	}
	else
	{
		CFG_set_str( CFG_str2id("AKS_SECOND_CHANNEL"), "0");
	}

	temp_value = atoi(g_web_config.bitSetting);
	printf("___ g_web_config.bitSetting=%s_%d\n",g_web_config.secondChannel,temp_value);
	if((temp_value>0)&&(temp_value<=1))
	{
		CFG_set_str( CFG_str2id("AKS_BIT_SETTINGS"), g_web_config.bitSetting);
	}
	else
	{
		CFG_set_str( CFG_str2id("AKS_BIT_SETTINGS"), "0");
	}
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
		ck_sum ^= fls_data[i];
	}

	if(g_web_config.initFlag != ck_sum)
	{
		memset(&g_web_config,0,sizeof(WEB_DATA_T));

		sprintf(g_web_config.name,"Ratpac AKS");
		sprintf(g_web_config.universe,"0");
		sprintf(g_web_config.subnet,"0");
		sprintf(g_web_config.gaffer_enb,"1");		// disable
		sprintf(g_web_config.gaffer_sub,"0");
		sprintf(g_web_config.gaffer_univ,"0");
		sprintf(g_web_config.gaffer_lower,"1");
		sprintf(g_web_config.gaffer_upper,"1");

		sprintf(g_web_config.timopower,"3");
		sprintf(g_web_config.channelWidth,"16");
		sprintf(g_web_config.secondChannel,"0");
		sprintf(g_web_config.bitSetting,"0");		
		write_data_to_flash();
		//m2m_do_reload();
	}
}

static char uart_rcv_data[64];
static int uart_rvc_len = 0;
static int uart_rvc_done = 0;

static int USER_FUNC uart_recv_callback(uint32_t event,char *data,uint32_t len,uint32_t buf_len)
{
	int copy_len;
	
	if (1 == artnet_enable)
	{
		uart_rvc_len=0;
		return len;
	}
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
		uart_rvc_len = 0;
		return (copy_len);
	}
//	Joo_uart_send("PD");
	return (copy_len);
}	

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
			CFG_get_str(CFG_str2id("AKS_UNIVERSE"),Uni);	
			uni_num = atoi(Uni);;
			CFG_get_str(CFG_str2id("AKS_SUBNET"),Uni);
			art_sub = atoi(Uni);
			art_sub <<= 4;
			uni_num = ((uni_num | art_sub) & 0xff);
			if(len > 50)
			{
				if(uni_num == (int)data[14])
				{
					replace_channel_data(data, len); 
					hfuart_send(HFUART0, data,len,1000);
				}
			}else
			{
				hfuart_send(HFUART0, data,len,1000);
			}
		}
	}
	return 0;
}

void UserMain(void *arg)
{
	
	int ret1, ret2;
	
	int level;
	level = DEBUG_LEVEL;
	
	hfdbg_set_level(level);
	level =  hfdbg_get_level();
	M2M_LOG(("**********dbglevel:%d\r\n", level));
	time_t time_now;
	time_now = time(NULL);
	HF_Debug(DEBUG_LEVEL,"sdk version(%s),the app_main start time is %d %s\n",hfsys_get_sdk_version(),time_now,ctime(&time_now));

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
	web_flash_data_init();
	if(hfnet_start_assis(ASSIS_PORT)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start assis fail\n");
	}	
	if(hfnet_start_uart(HFTHREAD_PRIORITIES_LOW,(hfnet_callback_t)uart_recv_callback)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start uart fail\n");
	}
	if(hfnet_start_socketa(HFTHREAD_PRIORITIES_LOW,(hfnet_callback_t)socketa_recv_callback)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start socketa fail\n");
	}
	if(hfnet_start_socketb(HFTHREAD_PRIORITIES_LOW,(hfnet_callback_t)NULL)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start socketb fail\n");
	}

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
		ret1 = hfthread_create(client_thread_main,"udp_client_main",512+128,(void*)1,HFTHREAD_PRIORITIES_NORMAL,NULL,NULL);

		if (HF_SUCCESS != ret1)
		{
			eprintf("Create UDP client fails, %d\n", ret1);
		}
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
			ret1 = hfthread_create(client_thread_main,"udp_client_main",512+128,(void*)1,HFTHREAD_PRIORITIES_NORMAL,NULL,NULL);

			if (HF_SUCCESS != ret1)
			{
				eprintf("Create UDP client fails, %d\n", ret1);
			}
		}
		else
		{
			ret1 = hfthread_create(server_thread_main,"udp_server_main",512+128,(void*)1,HFTHREAD_PRIORITIES_NORMAL,NULL,NULL);

			if (HF_SUCCESS != ret1)
			{
				eprintf("Create UDP server fails, %d\n", ret1);
			}
		}
	}
	int i=0;
	

	Joo_uart_send("__system_reboot");
	while(1)
	{
		if (0 == artnet_enable)
		{
			hf_thread_delay(1000);
			if (1 == uart_rvc_done)
			{
				Joo_uart_send(uart_rcv_data);
				uart_rvc_done = 0;
				uart_rvc_len = 0;
			}
			continue;
		}
		/*
		char TimoEnable[1] = {};
		CFG_get_str(CFG_str2id("AKS_SETTINGS"),TimoEnable);
		if(TimoEnable[0] == 48)
		{
			*/
			char timo_DMX_Window[4] = {0b00000000, 0b00000000, 0b00000010, 0b00000000};
			char timo_DMX_Spec[8] = {0b00000000, 0b00000010, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b01100001, 0b10101000};
			char TimoPower[1] = {};
			char timo_Blocked_Channel[11] = {0,0,0,0,0,0,0,0,0,0,0};

			char channel[2]={};
			CFG_get_str(CFG_str2id("WLN_Channel"),channel);
			channel[0] = (char)atoi(channel);

			char width[2]={};
			CFG_get_str(CFG_str2id("AKS_CHANNEL_WIDTH"),width);
			width[0] = (char)atoi(width);

			char channelTwo[2]={};
			CFG_get_str(CFG_str2id("AKS_SECOND_CHANNEL"),channelTwo);
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
			CFG_get_str(CFG_str2id("TIMO_POWER"),TimoPower);
			TimoPower[0] = TimoPower[0] - 48;

			char Battery[8] = {0b00110100, 0b00011011, 0b01100000, 0b00010001, 0b10110010, 0b10011010, 0b00000011, 0b01001011};
			char Timo[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 0, 0b10000010, 0b00000000, timo_DMX_Window[0], timo_DMX_Window[1], timo_DMX_Window[2], timo_DMX_Window[3], timo_DMX_Spec[0], timo_DMX_Spec[1], timo_DMX_Spec[2], timo_DMX_Spec[3], timo_DMX_Spec[4], timo_DMX_Spec[5], timo_DMX_Spec[6], timo_DMX_Spec[7], 0b00000001, TimoPower[0], timo_Blocked_Channel[0], timo_Blocked_Channel[1], timo_Blocked_Channel[2], timo_Blocked_Channel[3], timo_Blocked_Channel[4], timo_Blocked_Channel[5], timo_Blocked_Channel[6], timo_Blocked_Channel[7], timo_Blocked_Channel[8], timo_Blocked_Channel[9], timo_Blocked_Channel[10], Battery[0], Battery[1], Battery[2], Battery[3], Battery[4], Battery[5], Battery[6], Battery[7]};
			hfuart_send(HFUART0, Timo,sizeof(Timo),100);


		    char ipAddressSTR[20]={};
			CFG_get_str(CFG_str2id("LAN_IP"),ipAddressSTR);
			char lenth = strlen(ipAddressSTR);

			char ipAddressSTRTrimmed[lenth];

			strncpy(ipAddressSTRTrimmed, ipAddressSTR, lenth);
			char ipAddress[4]={};
			const char s[] = ".";
			char *token;

			token = strtok(ipAddressSTRTrimmed, s);
			ipAddress[0] = (char)atoi(token);
			token = strtok(NULL, s);
			ipAddress[1] = (char)atoi(token);
			token = strtok(NULL, s);
			ipAddress[2] = (char)atoi(token);
			token = strtok(NULL, s);
			ipAddress[3] = (char)atoi(token);

			char name[18]={' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',0};
			CFG_get_str(CFG_str2id("AKS_NAME"),name);
			char Settings[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 1,name[0],name[1],name[2],name[3],name[4],name[5],name[6],name[7],name[8],name[9],name[10],name[11],name[12],name[13],name[14],name[15],name[16],name[17],ipAddress[0],ipAddress[1],ipAddress[2],ipAddress[3]};
			hfuart_send(HFUART0, Settings,sizeof(Settings),100);



			char Settings2[] = {'A','r','t','-','N','e','t',0,0,50,0,0, 2, 1, 0};
			hfuart_send(HFUART0, Settings2,sizeof(Settings2),100);

		//}
		hf_thread_delay(5000);
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
char msg[MAX_MSG];
#define MAX_NUM_ENTRY 16

struct client_ent
{
	char node_name[20];
	char ip_addr[20];
	char universe[4];
	char subnet[4];
	char baterry[4];
};

struct client_ent client_list[MAX_NUM_ENTRY];
static client_valid_num = 0;
static char tmp_buff[20];

static void server_thread_main(void* arg) 
{
  int sd, rc, n, cliLen;
  struct sockaddr_in cliAddr, servAddr, remoteServAddr;
  fd_set rset;
  struct timeval timeout;
  int i;

  int broadcast = 1;

  refresh_clients = 0;

  eprintf("server_thread_main started\n");
  artnet_enable = 1;  
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
  client_valid_num = 1;
   /* server infinite loop */
  while(1) 
  {

	CFG_get_str( CFG_str2id("AKS_NAME"), tmp_buff);
	sprintf(client_list[0].node_name, "%s", tmp_buff);
	CFG_get_str( CFG_str2id("AKS_UNIVERSE"), tmp_buff);
	sprintf(client_list[0].universe, tmp_buff);	
	CFG_get_str( CFG_str2id("AKS_SUBNET"), tmp_buff);
	sprintf(client_list[0].subnet, tmp_buff);	
	
   	if (0 == refresh_clients)
	{
		hf_thread_delay(1000);
		continue;
	}
    /* init buffer */
	// refresh_clients = 0;
	client_valid_num = 1;
	sprintf(msg, "Registering UDP broadcast message.\n");
	eprintf("Registering UDP broadcast message.\n");
	// continue;
	
	rc = sendto(sd, msg, strlen(msg), 0, 
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
			sprintf(client_list[client_valid_num].node_name, "%s", &msg[0]);
			sprintf(client_list[client_valid_num].ip_addr, "%s", inet_ntoa(cliAddr.sin_addr));
			sprintf(client_list[client_valid_num].universe, &msg[20]);
			sprintf(client_list[client_valid_num].subnet, &msg[24]);
			client_valid_num++;
			
		}
	}

  }/* end of server infinite loop */

}

static char cli_recv[128]={0};
USER_FUNC static void client_thread_main(void* arg)
{
	int sd,id, rc, i;
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

	hf_thread_delay(300);
	while (1)
	{
		// eprintf("Send AT+WANN\n");

 		rc = hfat_send_cmd("AT+WANN\r\n", sizeof("AT+WANN\r\n"),at_rsp,96);
 
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
		if ( 1 /* FD_ISSET(sd, &rset) */)
		{
    		/* receive message */
		
			tmp = sizeof(addr);
			recv_num = recvfrom(sd, cli_recv, 96, 0, (struct sockaddr *)&addr, (socklen_t*)&tmp);
		
			if(recv_num <= 0)
			{
				continue;
			}
			cli_recv[recv_num] = 0;
			eprintf("thread %d, msg=%s, IP=%s\n",id, recv, inet_ntoa(addr.sin_addr));
			//g_web_config.name[19] = 0;
			//g_web_config.universe[2] = 0;
			//eprintf("Node name = %s, Uni = %s\n", g_web_config.name, g_web_config.universe);
			CFG_get_str( CFG_str2id("AKS_NAME"), &cli_recv[0]);
			CFG_get_str( CFG_str2id("AKS_UNIVERSE"), &cli_recv[20]);
			CFG_get_str( CFG_str2id("AKS_SUBNET"), &cli_recv[24]);
			//sprintf(&cli_recv[0], "AKS" );
			//sprintf(&cli_recv[20], "10");
#if 0
			for (i = 0; i < 32; i++)
			{
				eprintf("%02x ", recv[i]);
			}
			eprintf("\r\n");
#endif		
			rc = sendto(sd, cli_recv, 32, 0, 
				(struct sockaddr *) &serv, 
				sizeof(serv));
			if ( rc < 0)
			{
				eprintf("Client Sendto server fails\n");
			}
		}   // if (1)
	}

}

int get_client_entry(int idx, char *node_name, char *ip_addr, char *universe, char *art_sub, char *battery)
{
	if (idx >= client_valid_num)
	{
		node_name[0] = 0;
		ip_addr[0] = 0;
		universe[0] = 0;
		return -1;
	}
	sprintf(node_name, "%s", client_list[idx].node_name);
	if (strlen(node_name) == 0)
	{
		sprintf(node_name,"NoName");
	}
	sprintf(ip_addr, "%s", client_list[idx].ip_addr);
	sprintf(universe, "%s", client_list[idx].universe);
	sprintf(art_sub, "%s", client_list[idx].subnet);	

	sprintf(battery, "88\%");
	return 0;
}

void do_refresh_clients(void)
{
	refresh_clients = 1;
	
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
	CFG_get_str(CFG_str2id("GAFFER_ENABLE"),Uni);
	if (Uni[0] == '1')      // Gaffer is disabled
	{
		return;
	}
	CFG_get_str(CFG_str2id("GAFFER_UNIVERSE"),Uni);	
	uni_num = atoi(Uni);;
	CFG_get_str(CFG_str2id("GAFFER_SUBNET"),Uni);
	art_sub = atoi(Uni);
	art_sub <<= 4;
	uni_num = ((uni_num | art_sub) & 0xff);
	if(uni_num == (int)data[14])
	{
		CFG_get_str(CFG_str2id("GAFFER_LOWER"),Uni);
		lower = atoi(Uni);
 		CFG_get_str(CFG_str2id("GAFFER_UPPER"),Uni);
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
	
	CFG_get_str(CFG_str2id("GAFFER_ENABLE"),Uni);
	if (Uni[0] == '1')      // Gaffer is disabled
	{
		return;
	}
	CFG_get_str(CFG_str2id("GAFFER_LOWER"),Uni);
	lower = atoi(Uni);
 	CFG_get_str(CFG_str2id("GAFFER_UPPER"),Uni);
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
			sprintf(val, "%s", g_web_config.gaffer_enb);
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
			sprintf(g_web_config.name, "%s", val);
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
		
		default:
			return -1;
	}
	write_data_to_flash();
	return 0;
}

void set_artnet_enable(int val)
{
	artnet_enable = val;
}

void clear_uart_recv(void)
{
	uart_rvc_done = 0;
	uart_rvc_len = 0;
}

int Joo_uart_cmd(char *cmd)
{
	int i;
	set_artnet_enable(0);
	clear_uart_recv();
	Joo_uart_send(cmd);
	for (i = 0; i < 1000; i++)
	{
		hf_thread_delay(2);
		if ( 1 == uart_rvc_done)
			break;
	}
	if (0 == uart_rvc_done)
	{
		return (-1);
	}
	uart_rvc_done = 0;
	if (strstr(uart_rcv_data, "+ok"))
	{
		return 0;
	}
	return (-1);
}	




