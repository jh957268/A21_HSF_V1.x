
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
}WEB_DATA_T;

WEB_DATA_T g_web_config = {0};


/*AT CMD start ¡ý*/

void write_data_to_flash(void)
{
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
			init_webdata_when_reload();
			break;
		default:
			break;
	}
	return 0;
}
void web_flash_data_init(void)
{
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
	
	if(g_web_config.initFlag!=0xAA)
	{
		memset(&g_web_config,0,sizeof(WEB_DATA_T));
		g_web_config.initFlag = 0xAA;
		write_data_to_flash();
		m2m_do_reload();
	}
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
		char Uni[1]={0};
		char label[] = "Art-Net";
		if(strncmp(data, label, 7)==0)
		{
			CFG_get_str(CFG_str2id("AKS_UNIVERSE"),Uni);

			Uni[0] = (char)Uni[0]-48;
			if(len > 50)
			{
				if(strcmp(&Uni[0], &data[14]) == 0)
				{
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
	if(hfnet_start_uart(HFTHREAD_PRIORITIES_LOW,NULL)!=HF_SUCCESS)
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

	int i=0;
	while(1)
	{
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


