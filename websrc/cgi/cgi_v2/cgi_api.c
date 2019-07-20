/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    cfg_api.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
// begin
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

#if	MODULE_SYSLOG
#include <eventlog.h>
#endif

#include <cfg_def.h>
#include <cfg_net.h>
#include <time.h>
#include <cgi_api.h>
#include <assis.h>

#include <usercgi.h>
#include <flash.h>
#include <flash_layout.h>
#include <m2m_debug.h>
#include <m2m_wifi.h>

#if defined(RT3052) || defined(RT3352) || defined(RT6352) || defined(MT7620)||defined(CONFIG_MT7628_ASIC)
#define	SYS_TX_ANTENNA	2
#define	SYS_RX_ANTENNA	2
#else
#define	SYS_TX_ANTENNA	1
#define	SYS_RX_ANTENNA	1
#endif

//==============================================================================
//                                    MACROS
//==============================================================================

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               Porting from APSoC
//==============================================================================

static int	security_index = 0;
static int	bssid_index = 0;
static int	ssid_index = 0;
static int	current_ssid_num = 1;
int 		g_wsc_configured = 0;
static multi_ssid_s	mBSSID[4];
static wds_tbl_s	mWDS[4];

static int g_new_version;
int		site_survey_count = 0;
char		if_addr[16] = {0};
char		if_net[16] = {0};
char		if_hw[18] = {0};

time_t wps_status_start_time = 0;
time_t system_start_time = 0;
int wps_status_flag = 0;

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
char *upgrade_file=0;
//char *upgrade_mem=0;
http_upload_content *upgrade_mem=0;
int upgrade_file_len=0;
unsigned int upgrade_time_stamp=0;
unsigned char	wlan_tx_rx_count[100];


char PING_Result[350];
extern char SavePingResult[60];
//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================


//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
#ifdef	CONFIG_ZWEB
extern int zweb_location;
#endif
extern int opmode; /*1:Gateway, 2:Apcli as WAN port, 3:Repeater Mode */


//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------

extern struct	radix_node_head *rt_tables[AF_MAX+1];

typedef	struct cgi_cmd_t
{
	char * cmd;
	int (*func)(http_req *req);
} cgi_cmd;

int CGI_do_wan_connect(http_req *req);
int CGI_do_rt_add(http_req *req);
int system2(char *str);
int CGI_do_wireless_simple_config_device(http_req *req);
int CGI_do_wireless_security(http_req *req);
int CGI_do_sys_config(http_req *req);
int CGI_do_sys_log(http_req *req);
int CGI_do_secure_log(http_req *req);
int CGI_do_sys_chk_pass(http_req *req);
int CGI_do_lan_dhcp_clients(http_req *req);
int CGI_set_ap_only(http_req *req);
int CGI_do_sys_log_mail(http_req *req);
int CGI_do_ntp_config(http_req *req);
int CGI_upgrade(http_req *req);
int CGI_SAMD_upgrade(http_req *req);
int CGI_TIMO_upgrade(http_req *req);
int CGI_upgrade_upload(http_req *req);
int CGI_upgrade_save(http_req *req);
void CGI_upgrade_free(void);
int CGI_do_vpn_connect(http_req *req);
int OidQueryInformation(unsigned short OidQueryCode, char *DeviceName, void *ptr, unsigned long PtrLength);
int  parse_config_file(void);
int save_mbss_value_to_flash(int save_level);
void ConverterStringToDisplay(char *str);
void ConverterStringToDisplay2(char *str);
int CGI_do_uboot_upgrade(http_req *req);
static int CGI_do_load_cert(http_req *req);

void do_refresh_clients(void);
int  refresh_client_done(void);
int ratpac_get_str(int id, char *val);
int ratpac_set_str(int id, char *val);
int SAMD_firmware_download(char *firmware, int len, int which);
void get_eCos_ver(char *buffer);
void get_eCos_rel_build(char *buff);
void get_SAMD_ver(char *buffer);
void get_TIMO_ver(char *buffer);
void get_battery_info(char *buffer);
void get_module_ipaddress_mode(char *ip, char *mode);

int Send_Link_Command(void);
int Send_UnLink_Command(void);
int Send_ID_Command(void);
void write_data_to_flash(void);

cgi_cmd ps_cgi_cmds[]=
{
	{ "SYS_CONF", &CGI_do_sys_config },
#if	MODULE_SYSLOG
	{ "SYS_LOG", &CGI_do_sys_log },
	{ "SECURE_LOG",&CGI_do_secure_log},
#endif
	{ "SYS_PASS", &CGI_do_sys_chk_pass },
	{ "WAN_CON", &CGI_do_wan_connect },
	{ "SYS_UPG", &CGI_upgrade },
	{ "SYS_ULD", &CGI_upgrade_upload },
	{ "SAMD_UPG", &CGI_SAMD_upgrade },
	{ "TIMO_UPG", &CGI_TIMO_upgrade },
#if	MODULE_DHCPS
	{ "LAN_DHC", &CGI_do_lan_dhcp_clients },
#endif
	{ "RT_ADD"  , &CGI_do_rt_add },
	{ "WIRELESS_SECURITY", &CGI_do_wireless_security },
	{ "WIRELESS_SIMPLE_CONFIG_DEVICE", &CGI_do_wireless_simple_config_device },
	{ "ACL", &CGI_do_wireless_security },
	{"UBOOT_UPG",&CGI_do_uboot_upgrade},
	{"LOAD_CERT",&CGI_do_load_cert}
};

cgi_cmd as_cgi_cmds[]=
{
#if		MODULE_SYSLOG
	{ "SYS_LOG_MAIL", &CGI_do_sys_log_mail },
#endif
#ifdef CONFIG_NTP
	{ "NTP", &CGI_do_ntp_config },
#endif
};

int CGI_do_cmd(http_req *req)
{
	int i;
	char cmdi[8];
	char *cmdv, *val;
	int id;
	int rc=0;
	char * cmd;
	int update_cnt =0;
	int forceToReboot = 0;
	//char ebuff[64];

#if	(defined(CONFIG_HTTPD_MAX_USERS)&&(CONFIG_HTTPD_MAX_USERS>1))
	#ifdef	CONFIG_HTTPD_ADMIN_NAME
	// check user privilege 
	if(strcmp(req->auth_username, CONFIG_HTTPD_ADMIN_NAME))
	{
		rc=CGI_RC_PRIVILEGE_ERR;
		goto bad;
	}
#else
	char admin[256];
	CFG_get(CFG_SYS_USERS, admin);
	if (strcmp(req->auth_username,admin))
	{
		CFG_get(CFG_SYS_USERS1, admin);//menghui add
		if (strcmp(req->auth_username,admin))
		{
			M2M_DEBUG(("invalid user name  %s \n",req->auth_username));
			rc=CGI_RC_PRIVILEGE_ERR;
			goto bad;
		}
	}
	#endif
#endif
	//menghui
	if((rc=CGi_do_user_cmd(req))<0)
		goto bad;

	cmd=WEB_query(req,"CMD");
	if (!cmd)
		goto bad;
	if (cmd)
	{
		for (i=0; i< (sizeof(ps_cgi_cmds)/sizeof(cgi_cmd)) ; i++)
		{
			if (!strcmp(ps_cgi_cmds[i].cmd, cmd))
			{
				if (ps_cgi_cmds[i].func)
				{
					rc=(ps_cgi_cmds[i].func)(req);
					if (rc<0)	break;
				}

			}
		}

        	if( !(strcmp(cmd, "WIRELESS_SECURITY") && strcmp(cmd, "WIRELESS_SIMPLE_CONFIG_DEVICE") && strcmp(cmd, "ACL")))
	        	goto ReadyToOut;
	}

	if (rc<0)
		goto bad;
	
	for (i=0;i<50;i++)
	{
		sprintf(cmdi,"SET%d",i);
		cmdv=WEB_query(req,cmdi);
		if (cmdv==0)
			break;

		val=strchr(cmdv,'=');
		if ( (unsigned int)val <= (unsigned int)cmdv)
		{
			rc=CGI_RC_CFG_ERR;
			break;
		}
		*val=0;
		val++;
		id=atoi(cmdv);
		if (id!=0)
		{

			if((CFG_ID_TYPE(id) == CFG_TYPE_STR)  
				&& (id != CFG_POE_USER)
				&& (id != CFG_POE_PASS))
			{
				/* we don't want to see '\n' and '\r'*/
				char *c=0;
				if((c = strpbrk(val, "\n\r")))
					*c = '\0';
			}
			diag_printf("call CFG set %s=[%s]\n", CFG_id2str(id), val);
			//sprintf(ebuff, "call CFG set id = %d %s=[%s]\n", id, CFG_id2str(id), val);
			//Joo_uart_send((char *)ebuff);

			if (!strcmp(CFG_id2str(id), "WLN_BssidNum"))
			{
				forceToReboot = 1;
			}

			rc = ratpac_set_str(id, val);    // return 0 or -1, 0 means done and success
			if (-1 == rc)
			{
				rc=CFG_set_str(id,val);
			}
			
			if(id==CFG_SYS_USERS||id==CFG_SYS_ADMPASS)
			{
				user_config(0,0);
			}
			if (rc>=0) rc=CGI_RC_CFG_OK;
		}
	}

	if (!strcmp(cmd, "WIRELESS_APCLI_SETUP"))
	{
		char var[5];
		char fver[5];
		int rc;

		
		rc = CFG_get_str( CFG_str2id("WLN_ApCliEnable"), var);
		if (strcmp(var, "1")== 0)	
		{
			m2m_cfg_set("M2M_WIFI_MODE","STA");
			f_setting_get("FVERSION", fver);
			m2m_cfg_set("WLN_CountryRegion", "5");
			if (strcmp(fver, "z")== 0)
			{
				m2m_cfg_set("LAN_DHCPD_EN", "0");
				m2m_cfg_set("SYS_OPMODE","3");
				CFG_set_str(CFG_WLN_MACRepeaterEn, "1");
			}	
			else
			{
				m2m_cfg_set("LAN_DHCPD_EN", "1");
				m2m_cfg_set("SYS_OPMODE","2");
				CFG_set_str(CFG_WLN_MACRepeaterEn, "0");
			}
				
		}
		else
		{
			m2m_cfg_set("LAN_DHCPD_EN", "1");
			//m2m_cfg_set("WLN_CountryRegion", "0");
			m2m_cfg_set("WLN_CountryRegion", "5"); //Jim
			m2m_cfg_set("M2M_WIFI_MODE","AP");
			m2m_cfg_set("SYS_OPMODE","1");
			CFG_set_str(CFG_WLN_MACRepeaterEn, "0");
		}
		CFG_save(0);
	}
	if (!strcmp(cmd, "CONSTELLATION_REFRESH"))
	{
		do_refresh_clients();
		return CGI_RC_OK;
	}
	if (strstr(cmd, "SET_UNIVERSE"))
	{
		char art_subnet[4], uniNo[4];
		char *tmp1, *tmp2;

		tmp1 = strchr(cmd, '=');
		if (0 == tmp1)
		{
			return (CGI_RC_OK);
		}
		tmp1++;
		tmp2 = strchr(tmp1, '-');
		if (0 == tmp2)
		{
			return (CGI_RC_OK);
		}
		*tmp2 = 0;
		tmp2++;
		
		sprintf(art_subnet, "%s", tmp1);
		sprintf(uniNo, "%s", tmp2);			
		ratpac_set_str(CFG_AKS_UNIVERSE,uniNo);
		ratpac_set_str(CFG_AKS_SUBNET,art_subnet);
		//CFG_save(0);
		write_data_to_flash();					// write ratpac data to flash
		return CGI_RC_OK;
	}

ReadyToOut:	

	if (rc==CGI_RC_CFG_OK)
	{
		//update_cnt = CFG_commit(CFG_DELAY_EVENT);
		CFG_save(0);
		write_data_to_flash();
	}
	else
	if (rc<0)
		goto bad;

	if (cmd)
	{
		for (i=0; i< (sizeof(as_cgi_cmds)/sizeof(cgi_cmd)) ; i++)
		{
			if (!strcmp(as_cgi_cmds[i].cmd, cmd))
			{
				if (as_cgi_cmds[i].func)
				{
					rc = (as_cgi_cmds[i].func)(req);
					if (rc<0)
						break;
				}

			}
		}
	}
        /* change the settings without rebooting the whole system */
	if (update_cnt && (forceToReboot == 0) && strcmp(cmd, "LAN") && strcmp(cmd, "DHCP") && strcmp(cmd, "QOS") && 
		strcmp(cmd, "WIRELESS_SIMPLE_CONFIG_DEVICE") && strcmp(cmd, "OPMODE"))
	{
		extern int CFG_parse_wlan(struct cfgobj_list_t *cfg_lp);
		extern struct cfgobj_list_t cfg_list;
#ifdef CONFIG_BRIDGE
		extern int BRG_start(int cmd);
		extern char bridge_cfg[128];
#endif /* CONFIG_BRIDGE */
#ifdef	CONFIG_UPNP
extern int ssdp_Delete_MulticastIP(void);
                ssdp_Delete_MulticastIP();
#endif	// CONFIG_UPNP //

#ifdef CONFIG_BRIDGE
		BRG_start(0);
#endif /* CONFIG_BRIDGE */
#ifdef CONFIG_WIRELESS
		WLAN_start(0);
#endif /* CONFIG_WIRELESS */
		LAN_start(0);

		/* reload the cfgobj list and parse WIFI configuration into the array which WIFI driver uses. */
		CFG_parse_wlan(&cfg_list);

#ifdef CONFIG_BRIDGE                
		memset(&bridge_cfg[0], 0 , 128);
#endif /* CONFIG_BRIDGE */		
		interface_config();
		LAN_start(1);
#ifdef CONFIG_WIRELESS
		WLAN_start(1);
#endif /* CONFIG_WIRELESS */
#ifdef CONFIG_BRIDGE
		BRG_start(1);
#endif /* CONFIG_BRIDGE */

#ifdef	CONFIG_UPNP
extern int ssdp_Update_MulticastIP(void);
                ssdp_Update_MulticastIP();
#endif	// CONFIG_UPNP //

#ifdef RALINK_1X_SUPPORT		
		Dot1x_Reboot();
#endif //RALINK_1X_SUPPORT
	}
#if CONFIG_QOS
	else if (update_cnt && ( !strcmp(cmd, "QOS")))
	{	
		tfq_load_config();
	}
#endif
	else if ((update_cnt && ((forceToReboot == 1) || !strcmp(cmd, "LAN") || !strcmp(cmd, "DHCP") || 
		!strcmp(cmd, "OPMODE")))|| !strcmp(cmd, "SYS_UPG"))
	{
		/* need to reboot only while changing the number of BSSID or the LAN configuration. */ 
		mon_snd_cmd(MON_CMD_REBOOT);
		rc = CGI_RC_REBOOT;

	}

	if ((update_cnt == 0) && !strcmp(cmd, "WIRELESS_SIMPLE_CONFIG_RESET"))
	{
		diag_printf("Reset OOB!!!\n");
		
		int ret;
		char	wlanbuf[128];
		
		memset(wlanbuf, 0, 128);
		sprintf(wlanbuf, "WscStop=%d", 1);
		ret = system2(wlanbuf);

		memset(wlanbuf, 0, 128);
		sprintf(wlanbuf, "WscConfStatus=%d", 1);
		ret = system2(wlanbuf);
	}
	
bad:
	if (rc < 0)
	{
		diag_printf("cgi result=%d\n", rc);
	}
	
	return rc;
}

void sc_convert(char *instr)
{
	char *p;	
	
    p = instr;
    while(*p)
    {
    	if((*p == '"') || (*p == '\'') || (*p == '\\'))
    	{
    		memmove(p+1, p, strlen(p)+1);
    		*p = '\\';
    		p++; 
    	}
    	p++;
    }
}

void fill_space(char *datarray, int arrlen)
{
	int i;
	int len = strlen(datarray);

	for (i = len; i < (arrlen - 1); i++)
	{
		datarray[i] = ' ';
	}
	datarray[arrlen-1] = 0;
}

int Joo_uart_cmd(char *cmd, int len, char *expect_resp, cyg_tick_count_t timeout_tick);
int get_client_entry(int idx, char *node_name, char *ip_addr, char *universe, char *art_sub, char *battery);
void CGI_var_map(http_req *req, char *name, int id)
{
	static char val[512+128];    // carefule, the stack size may not be enough for such a big value
	static char href_link[128];
	static char cmd_buff[128];
	int idx, base_idx, i;
	char node_name[24];
	char ip_addr[20];
	char universe[4];
	char art_sub[4];
	char ip_addr_save[20];
	char sel_ele[64];
	char battery[12];
	int node_len, link_len;
	int uni_num, art_sub_num;

	switch (id)
	{
		case CFG_AKS_ADVA1:
		case CFG_AKS_ADVA2:
		case CFG_AKS_ADVA3:
		case CFG_AKS_ADVA4:
		case CFG_AKS_ADVA5:
		case CFG_AKS_ADVA6:
		case CFG_AKS_ADVA7:
		case CFG_AKS_ADVA8:
		case CFG_AKS_ADVA9:
		case CFG_AKS_ADVA10:
		case CFG_AKS_ADVA11:
		case CFG_AKS_ADVA12:
		case CFG_AKS_ADVA13:
		case CFG_AKS_ADVA14:
		case CFG_AKS_ADVA15:
		case CFG_AKS_ADVA16:
			base_idx = ((CFG_AKS_ADVA1 >> 16) & 0xff);
			idx = ((id >> 16) & 0xff) - base_idx;
			if (-1 == get_client_entry(idx, node_name, ip_addr, universe, art_sub, battery))
			{
				return;
			}
			WEB_printf(req, "displayMsg('%s',",name);
			
			sprintf(ip_addr_save, "%s", ip_addr);	
			sprintf(href_link, "<a href=\"http://%s\" target=\"_blank\">%s</a>", ip_addr, node_name);
			
			link_len = strlen(href_link);
			node_len = strlen(node_name);
			fill_space(href_link, link_len + 17 - node_len);
			fill_space(ip_addr, 17);			
			sprintf(val, "%s", href_link);
			strcat(val, ip_addr);	
			sc_convert(val);
			WEB_printf(req, "'%s'+",val);
			
			sprintf(cmd_buff, "<button type=\"button\" onclick=\"popupCenter('http://%s/EN/id.html', 'myPop1',400,300);\">ID</button>", ip_addr_save);
			link_len = strlen(cmd_buff);
			fill_space(cmd_buff, link_len+3);
			sc_convert(cmd_buff);
			WEB_printf(req, "'%s'+",cmd_buff);			
			
			get_eCos_ver(val);
			strcat(val, " ");
			sprintf(cmd_buff, "<button type=\"button\" onclick=\"updateFirmware(document.Upgrade_eCos, 'http://%s/EN/update_firmware.html');\">Update</button>", ip_addr_save);
			link_len = strlen(cmd_buff);
			fill_space(cmd_buff, link_len+3);
			strcat(val, cmd_buff);
			sc_convert(val);
			WEB_printf(req, "'%s'+",val);
	
			get_SAMD_ver(val);
			strcat(val, " ");			
			sprintf(cmd_buff, "<button type=\"button\" onclick=\"updateFirmware(document.Upgrade_SAMD, 'http://%s/EN/update_firmware.html');\">Update</button>", ip_addr_save);
			link_len = strlen(cmd_buff);
			fill_space(cmd_buff, link_len+3);
			strcat(val, cmd_buff);
			sc_convert(val);
			WEB_printf(req, "'%s'+",val);
			
			get_TIMO_ver(val);
			strcat(val, " ");			
			sprintf(cmd_buff, "<button type=\"button\" onclick=\"updateFirmware(document.Upgrade_TIMO, 'http://%s/EN/update_firmware.html');\">Update</button>", ip_addr_save);
			link_len = strlen(cmd_buff);
			fill_space(cmd_buff, link_len+0);
			strcat(val, cmd_buff);
			sc_convert(val);			
			WEB_printf(req, "'%s'",val);			// last one
			WEB_printf(req, ");\n");				// close quote		
			return;
		case CFG_AKS_CONS1:
		case CFG_AKS_CONS2:
		case CFG_AKS_CONS3:
		case CFG_AKS_CONS4:
		case CFG_AKS_CONS5:
		case CFG_AKS_CONS6:
		case CFG_AKS_CONS7:
		case CFG_AKS_CONS8:
		case CFG_AKS_CONS9:
		case CFG_AKS_CONS10:
		case CFG_AKS_CONS11:
		case CFG_AKS_CONS12:
		case CFG_AKS_CONS13:
		case CFG_AKS_CONS14:
		case CFG_AKS_CONS15:
		case CFG_AKS_CONS16:
#if 0		
			if (CFG_AKS_CONS1 == id)
			{
				do_refresh_clients();
				while (1)
				{
					if ( 1 == refresh_client_done())
					{
						break;
					}
					hf_thread_delay(1000);
				}
			}
#endif		
			base_idx = ((CFG_AKS_CONS1 >> 16) & 0xff);	
			idx = ((id >> 16) & 0xff) - base_idx;
			if (-1 == get_client_entry(idx, node_name, ip_addr, universe, art_sub, battery))
			{
				return;
			}
			//sprintf(node_name, "AKS_NODE%d", idx);
			//sprintf(ip_addr,"10.10.100.2%d", idx);
			//sprintf(universe,"%d", idx);
			//sprintf(identify,"ID*");
			//sprintf(link, "Link");
			//sprintf(node_name, "AKS_NODETest");
			//sprintf(ip_addr,"10.10.100.254");
			//sprintf(art_sub,"1");
			//sprintf(baterry,"88");
			//strcat(baterry,"\%");
			sprintf(ip_addr_save, "%s", ip_addr);
			
			sprintf(href_link, "<a href=\"http://%s\" target=\"_blank\">%s</a>", ip_addr, node_name);
			
			link_len = strlen(href_link);
			node_len = strlen(node_name);
			fill_space(href_link, link_len + 17 - node_len);
			fill_space(ip_addr, 18);
			fill_space(battery, 9);
			sprintf(val, "%s", href_link);
			strcat(val, ip_addr);
			strcat(val, battery);
			
			sprintf(cmd_buff, "<button type=\"button\" onclick=\"popupCenter('http://%s/EN/id.html', 'myPop1',400,300);\">ID</button>", ip_addr_save);
			link_len = strlen(cmd_buff);
			fill_space(cmd_buff, link_len+5);
			strcat(val, cmd_buff);
			
			sprintf(cmd_buff, "<button type=\"button\" onclick=\"popupCenter('http://%s/EN/link.html', 'myPop1',400,300);\">Link</button>", ip_addr_save);			
			link_len = strlen(cmd_buff);
			fill_space(cmd_buff, link_len+1);	
			strcat(val, cmd_buff);
			
			sprintf(cmd_buff, "<button type=\"button\" onclick=\"popupCenter('http://%s/EN/unlink.html', 'myPop1',400,300);\">Unlink</button>", ip_addr_save);
			link_len = strlen(cmd_buff);
			fill_space(cmd_buff, link_len+4);
			strcat(val, cmd_buff);
			
			//fill_space(universe, 4);
			uni_num = atoi(universe);
			art_sub_num = atoi(art_sub);

			//strcat(val, universe);
			
			//sprintf(val, "<a href=\"http://%s\" target=\"_blank\">%s</a> &emsp;&emsp;&emsp;&emsp;&emsp; %s", ip_addr, node_name, ip_addr) ;
			sc_convert(val);
			//sc_convert(universe);
			//WEB_printf(req, "displayMsg('%s','%s');\n",name,val);
			WEB_printf(req, "displayMsg('%s',",name);
			//WEB_printf(req, "'%s');\n",val);
			WEB_printf(req, "'%s'+",val);
			//WEB_printf(req, "'%s'",universe);
			
			sprintf(sel_ele,"<select id=\"artsub%d\" size=\"1\">", idx);
			sc_convert(sel_ele);
			WEB_printf(req, "'%s'+",sel_ele);

			sprintf(cmd_buff, "<button type=\"button\" onclick=\"setUniverse('http://%s/EN/', '%d');\">Set</button>", ip_addr_save, idx);
			
			for (i = 0; i < 16; i++)
			{
				if (i == art_sub_num)
					sprintf(sel_ele,"<option value=\"%d\" selected>%d</option>", i, i);
				else
					sprintf(sel_ele,"<option value=\"%d\">%d</option>", i, i);
				sc_convert(sel_ele);
				WEB_printf(req, "'%s'+",sel_ele);
			}
	
			//sprintf(sel_ele,"<option selected>2</option>");
			//sc_convert(sel_ele);
			//WEB_printf(req, "'%s'+",sel_ele);
	
			sprintf(sel_ele,"</select>");
			sc_convert(sel_ele);
			WEB_printf(req, "'%s'+",sel_ele);
			
			sprintf(sel_ele,"<select id=\"univ%d\" size=\"1\">", idx);
			sc_convert(sel_ele);
			WEB_printf(req, "'%s'+",sel_ele);
			
			for (i = 0; i < 16; i++)
			{
				if (i == uni_num)
					sprintf(sel_ele,"<option value=\"%d\" selected>%d</option>", i, i);
				else
					sprintf(sel_ele,"<option value=\"%d\">%d</option>", i, i);
				sc_convert(sel_ele);
				WEB_printf(req, "'%s'+",sel_ele);
			}
			sprintf(sel_ele,"</select>");
			sc_convert(sel_ele);
			WEB_printf(req, "'%s'+",sel_ele);
			
			// print the set button
			sc_convert(cmd_buff);
			WEB_printf(req, "'%s'",cmd_buff);
			
			WEB_printf(req, ");\n");
			return;

		case CFG_SET_ID:
			if (Send_ID_Command() == 0)
			{
				sprintf(val, "Command Status : Success");
			}
			else
			{
				sprintf(val, "Command Status : Fail");				
			}
			WEB_printf(req, "displayMsg('%s','%s');\n",name, val);
			return;
		case CFG_SET_LINK:
			if (Send_Link_Command() == 0)
			{
				sprintf(val, "Command Status : Success");
			}
			else
			{
				sprintf(val, "Command Status : Fail");				
			}
			WEB_printf(req, "displayMsg('%s','%s');\n",name, val);
			return;	
		case CFG_SET_UNLINK:
			if (Send_UnLink_Command() == 0)
			{
				sprintf(val, "Command Status : Success");
			}
			else
			{
				sprintf(val, "Command Status : Fail");				
			}
			WEB_printf(req, "displayMsg('%s','%s');\n",name, val);
			return;
		case CFG_AKS_REL_BUILD:
			get_eCos_rel_build(val);
			get_SAMD_ver(cmd_buff);
			strcat(val, "  SAMD:");
			strcat(val, cmd_buff);
			get_TIMO_ver(cmd_buff);	
			strcat(val, "  TIMO:");
			strcat(val, cmd_buff);			
			WEB_printf(req, "displayMsg('%s',",name);
			WEB_printf(req, "'%s'",val);			// last one
			WEB_printf(req, ");\n");				// close quote	
			return;
			
		case CFG_BATTERY_ID:
			get_battery_info(cmd_buff);
			get_module_ipaddress_mode(ip_addr, ip_addr_save);

			//Joo_uart_send(cmd_buff);
			//sprintf(cmd_buff,"USB 88");
			//sprintf(val, "Battery: %s", cmd_buff);
			//sprintf(val, "<center><font color=\"black\" size=\"2\">Mode:");
			sprintf(val, "<center><font color=\"black\" size=\"2\">Mode: </font><font color=\"orange\" size=\"3\"><b>%s</b> &nbsp;&nbsp;&nbsp;</font><font color=\"black\" size=\"2\">IP: </font><font color=\"blue\" size=\"3\"><b>%s</b> &nbsp;&nbsp;</font><font color=\"black\" size=\"2\">Battery: </font><font color=\"green\" size=\"3\"><b>%s%%</b> &nbsp;&nbsp;</font></center>",ip_addr_save, ip_addr, cmd_buff);

			//Joo_uart_send(val);
			WEB_printf(req, "displayMsg('%s',",name);
			sc_convert(val);
			WEB_printf(req, "'%s'",val);			// last one
			WEB_printf(req, ");\n");				// close quote	
			return;
		
		case CFG_SET_UNIV0:
		case CFG_SET_UNIV1:
		case CFG_SET_UNIV2:
		case CFG_SET_UNIV3:
		case CFG_SET_UNIV4:
		case CFG_SET_UNIV5:
		case CFG_SET_UNIV6:
		case CFG_SET_UNIV7:	
		case CFG_SET_UNIV8:
		case CFG_SET_UNIV9:
		case CFG_SET_UNIV10:
		case CFG_SET_UNIV11:
		case CFG_SET_UNIV12:
		case CFG_SET_UNIV13:
		case CFG_SET_UNIV14:
		case CFG_SET_UNIV15:
			base_idx = ((CFG_SET_UNIV0 >> 16) & 0xff);
			idx = ((id >> 16) & 0xff) - base_idx;
			sprintf(val,"%d", idx);
			CFG_set_str(CFG_AKS_UNIVERSE,val);
			CFG_save(0);
			return;

		default:
			idx = ratpac_get_str(id, val);
			if (-1 == idx)
			{
				CFG_get_str(id, val);
			}
			break;
	}
#if 0	
	if (CFG_AKS_CONS1 == id)
	{
	    //if (0 == jootest)
	    //{
		//	sprintf(joo_test_value,"JooTest-AKS");
		//	jootest++;
		//}
		sprintf(val,"AKS_NODE1         10.10.100.20");
		//jootest++;
		//Joo_uart_send(val);
	}
	else if (CFG_AKS_CONS2 == id )
	{
		sprintf(val,"AKS_NODE2         10.10.100.21");
	}
	else
	{
		CFG_get_str(id, val);
	}	
#endif
	sc_convert(val);
	WEB_printf(req, "addCfg('%s',0x%08x,'%s');\n",name,id,val);
}

void CGI_var_map_array(http_req *req, char *name, int id, int num)
{
	char val[512];
	int i;
	for( i=0; i<num ; i++)
	{
		val[0]='\0';
		CFG_get_str(id+i, val);
		sc_convert(val);
		WEB_printf(req, "addCfg('%s%d',0x%08x,'%s');\n",name,i+1,id+i,val);
	}
}

char * CGI_put_var(http_req *req, char *var)
{
    char *val;
    val=WEB_query(req, var);
    if (val)
        WEB_printf(req, "%s", val );
	return val;
}


// 4
int CGI_do_wan_connect(http_req *req) { 

   char *action ;
   char *argv[1];
	int rc=CGI_RC_CONNECTING;
        
   action = WEB_query(req,"action");
   if (!action) return -1;

   	switch (*action)
	{
#if	(MODULE_WANIF==0)
	case '1': // call release function here
     	DHCPC_release_one(SYS_lan_if);
		rc=8;
     	break ;
	case '2': // call renew function here
    	DHCPC_renew(SYS_lan_if);
     	break ;
#else
#if (MODULE_DHCPC == 1)	
     case '1': // call release function here
     	DHCPC_release_one(SYS_wan_if);
		rc=8;
     	break ;
     case '2': // call renew function here
     	DHCPC_renew(SYS_wan_if);
     	break ;
#endif
#if (MODULE_PPPOE == 1)	
     case '3': // call connection function here
     	argv[0] = "up";
     	pppoe_cmd(1, argv);
    	 break ;
     case '4': // call disconnected function here
     	argv[0] = "down";
     	pppoe_cmd(1, argv);
		rc=8;
    	 break ;
#endif
#if (MODULE_PPTP == 1)	
     case '5': // call pptp connection function here
     	argv[0] = "up";
     	pptp_cmd(1, argv);
        break;
        
     case '6': // call disconnected function here
     	argv[0] = "down";
     	pptp_cmd(1, argv);
		rc=8;
        break;    
#endif
#if (MODULE_L2TP == 1)	 
     case '7': // call pptp connection function here
     	argv[0] = "up";
     	l2tp_cmd(1, argv);
        break;
      
     case '8': // call disconnected function here
     	argv[0] = "down";
     	l2tp_cmd(1, argv);
		rc=8;
        break;   
#endif    	 
#endif
   }                                        
    return rc;
} 

void CGI_reset_default(void)
{
	diag_printf("CGI_reset_default----->\n");
	int pt, i, len;
	char cakey[70], cassid[40], cfver[20], cfwm[10], csssid[40], clogo[5], cbaud[5], cmaxsk[5], clan[20], cgw[20], cephy[5], ceptp[5];
	char cnetpro[5], cnetcs[10], cnetport[10], cnetip[100], cskey[70], cwan[20], cwangw[20], cwmode[5], clang[5];
	char clg[20], cpwd[20], caauth[10], caenc[10], csauth[10], csenc[10], ctmode[10], ctcpto[10], caft[10], cafl[10];	
	char cmid[30];
	char cfvew[10];
extern int f_setting_get(char *feild, char *val);
extern int m2m_cfg_set(char *field, char *val);
	static char cwandns[32];

	bzero(cwandns,sizeof(cwandns));
	f_setting_get("FWDNS",cwandns);
	
	//f_setting_get("M2M_MODULE_ID", cmid);
	f_setting_get("FAKey", cakey);
	f_setting_get("FASSID", cassid);
	f_setting_get("FVERSION", cfver);
	f_setting_get("FBGN", cfwm);
	//fnrdy= nvram_get(RTDEV_NVRAM, "FNRDY");			// for FNRDY
	f_setting_get("FSSSID", csssid);
	//f_setting_get("FLOGO", clogo);
	f_setting_get("FBAUD", cbaud);
	f_setting_get("FMAXSK", cmaxsk);
	f_setting_get("FLAN", clan);
	f_setting_get("FGW", cgw);
	f_setting_get("FEPHY", cephy);
	f_setting_get("FEPTP", ceptp);
	f_setting_get("FNetProtocol", cnetpro);
	f_setting_get("FNetMode", cnetcs);
	f_setting_get("FNetPort", cnetport);
	f_setting_get("FNetIPClient", cnetip);
	f_setting_get("FSKey", cskey);
	f_setting_get("FWAN", cwan);
	f_setting_get("FWANGW", cwangw);
	f_setting_get("FWMODE", cwmode);
	f_setting_get("FLANG", clang);
	f_setting_get("FWEBULG", clg);
	f_setting_get("FWEBUPWD", cpwd);
	f_setting_get("FAAUTH", caauth);
	f_setting_get("FAENC", caenc);
	f_setting_get("FSAUTH", csauth);
	f_setting_get("FSENC", csenc);
	f_setting_get("FTMODE", ctmode);
	f_setting_get("FTCPTO", ctcpto);
	f_setting_get("FAFT", caft);
	f_setting_get("FAFL", cafl);
	f_setting_get("FVEW", cfvew);

	CFG_init_prof();			// reload the default setting
	cyg_thread_delay(50);

	//m2m_cfg_set("M2M_MODULE_ID", cmid);
	//m2m_cfg_set("FLOGO", clogo);
	//m2m_cfg_set("FVERSION", cfver);
	if (strcmp(cfvew, "Enable")== 0)
		m2m_cfg_set("M2M_FVEW", "1");
	else
		m2m_cfg_set("M2M_FVEW", "0");
	if (strcmp(ctcpto, "")!= 0)
		m2m_cfg_set("M2M_NET_TCPTO", ctcpto);
	if ((strcmp(caft, "")!= 0)&&(strcmp(cafl, "")!= 0))
	{
		m2m_cfg_set("M2M_UART_AF", "enable");
		m2m_cfg_set("M2M_UART_AFT", caft);
		m2m_cfg_set("M2M_UART_AFL", cafl);
	}
	if (strcmp(ctmode, "")!= 0)
		m2m_cfg_set("M2M_RUN_MODE", ctmode);
	if (strcmp(clg, "")!= 0)
	{
		m2m_cfg_set("SYS_USERS", clg);
		m2m_cfg_set("SYS_ADMPASS", cpwd);
	}
	if ((strcmp(caauth, "")!= 0)&&(strcmp(cakey, "")!= 0))
	{
		put_field_n(1, "WLN_AuthMode", caauth);
		if (strcmp(caenc, "WEP-H")== 0)
		{
			put_field_n(1, "WLN_EncrypType", "WEP");
			put_field_n(1, "WLN_Key1Type", "0");
			m2m_cfg_set("WLN_Key1Str1", cakey);
		}
		else if (strcmp(caenc, "WEP-A")== 0)
		{
			put_field_n(1, "WLN_EncrypType", "WEP");
			put_field_n(1, "WLN_Key1Type", "1");
			m2m_cfg_set("WLN_Key1Str1", cakey);
		}
		else
		{
			put_field_n(1, "WLN_EncrypType", caenc);
			m2m_cfg_set("WLN_WPAPSK1", cakey);
		}
	}
	if ((strcmp(csauth, "")!= 0)&&(strcmp(cskey, "")!= 0))
	{
		m2m_cfg_set("WLN_ApCliAuthMode", csauth);
		if (strcmp(csenc, "WEP-H")== 0)
		{
			m2m_cfg_set("WLN_ApCliEncrypType", "WEP");
			m2m_cfg_set("WLN_ApCliKey1Type", "0");
			m2m_cfg_set("WLN_ApCliKey1Str", cskey);
		}
		else if (strcmp(csenc, "WEP-A")== 0)
		{
			m2m_cfg_set("WLN_ApCliEncrypType", "WEP");
			m2m_cfg_set("WLN_ApCliKey1Type", "1");
			m2m_cfg_set("WLN_ApCliKey1Str", cskey);
		}
		else
		{
			m2m_cfg_set("WLN_ApCliEncrypType", csenc);
			m2m_cfg_set("WLN_ApCliWPAPSK", cskey);
		}
	}
	if (strcmp(clang, "")!= 0)
		;//m2m_cfg_set();
		//nvram_bufset(RT2860_NVRAM, "Language", clang);
	if (strcmp(cwmode, "AP")==0)
	{
		m2m_cfg_set("SYS_OPMODE","1");
		m2m_cfg_set("WLN_ApCliEnable", "0");
		m2m_cfg_set("LAN_DHCPD_EN", "1");
	}
	else if (strcmp(cwmode, "STA")==0)
	{
		
		if ((strcmp(cfver, "n")==0)||(strcmp(cfver, "m")==0))
		{
			m2m_cfg_set("WLN_ApCliEnable", "1");
			m2m_cfg_set("LAN_DHCPD_EN", "1");
			m2m_cfg_set("WLN_CountryRegion", "5");
			m2m_cfg_set("SYS_OPMODE","2");
		}
		else if (strcmp(cfver, "z")==0)
		{
			m2m_cfg_set("WLN_ApCliEnable", "1");
			m2m_cfg_set("LAN_DHCPD_EN", "0");
			m2m_cfg_set("WLN_CountryRegion", "5");
			m2m_cfg_set("SYS_OPMODE","3");
		}
		else
		{
			m2m_cfg_set("WLN_ApCliEnable", "1");
			m2m_cfg_set("LAN_DHCPD_EN", "1");
			m2m_cfg_set("WLN_CountryRegion", "5");
			m2m_cfg_set("SYS_OPMODE","2");
		}
	}
	if (strcmp(cnetpro, "")!= 0)
		m2m_cfg_set("M2M_NET_PROC", cnetpro);
	if (strcmp(cnetcs, "")!= 0)
		m2m_cfg_set("M2M_NET_MODE", cnetcs);
	if (strcmp(cnetport, "")!= 0)
		m2m_cfg_set("M2M_NET_PORT", cnetport);
	if (strcmp(cnetip, "")!= 0)
		m2m_cfg_set("M2M_NET_SERADD", cnetip);
	if (strcmp(cgw, "")!= 0)
		;//m2m_cfg_set("M2M_NET_SERADD", cnetip);
	if (strcmp(cwan, "")!= 0)
	{
		in_addr_t dns;
		m2m_cfg_set("WAN_MSK", "255.255.255.0");
		m2m_cfg_set("WAN_IP_MODE", "1");			// STATIC
		m2m_cfg_set("WAN_IP", cwan);
		m2m_cfg_set("DNS_FIX", "1");
		dns = 0x04040808;
		CFG_set(CFG_DNS_SVR+1,&dns);
		dns=0x08080808;
		CFG_set(CFG_DNS_SVR+2,&dns);
	}
	if(strlen(cwandns)>0)
	{
		in_addr_t dns = inet_addr(cwandns);
		CFG_set(CFG_DNS_SVR+1,&dns);
		dns=0x08080808;
		CFG_set(CFG_DNS_SVR+2,&dns);
	}
	if (strcmp(cwangw, "")!= 0)
		m2m_cfg_set("WAN_GW", cwangw);
	if (strcmp(clan, "")!= 0)
	{
		m2m_cfg_set("LAN_IP", clan);
		pt= 0;
		len= strlen(clan);
		for (i= 0; i< len; i++)
		{
			if (clan[i]== '.')
			{
				pt++;
				if (pt==3)
					break;
			}
		}
		clan[i+1]='1';
		clan[i+2]= '0';
		clan[i+3]= '0';
		clan[i+4]= '\0';
		m2m_cfg_set("LAN_DHCPD_START", clan);
		clan[i+1]='2';
		m2m_cfg_set("LAN_DHCPD_END", clan);
	}
	if (strcmp(cbaud, "")!=0)
		m2m_cfg_set("M2M_UART_BAUD", cbaud);
	if (strcmp(cmaxsk, "")!=0)
		m2m_cfg_set("M2M_NET_MAXSK", cmaxsk);
	if (strcmp(cassid, "")!=0)
		m2m_cfg_set("WLN_SSID1", cassid);
	if (strcmp(csssid, "")!=0)
		m2m_cfg_set("WLN_ApCliSsid", csssid);
	if (strcmp(cfwm, "")!=0)
	{
		if ((strcmp(cfwm, "11BGN")==0)||(strcmp(cfwm, "11bgn")==0))
		{
			m2m_cfg_set("WLN_WirelessMode", "9");
			m2m_cfg_set("WLN_BasicRate", "15");
			m2m_cfg_set("WLN_FixedTxMode", "HT");
			m2m_cfg_set("WLN_HT_MCS", "0");
		}
		else if ((strcmp(cfwm, "11BG")==0)||(strcmp(cfwm, "11bg")==0))
		{
			m2m_cfg_set("WLN_WirelessMode", "0");
			m2m_cfg_set("WLN_BasicRate", "15");
			m2m_cfg_set("WLN_FixedTxMode", "OFDM");
			m2m_cfg_set("WLN_HT_MCS", "7");
		}
		else if ((strcmp(cfwm, "11B")==0)||(strcmp(cfwm, "11b")==0))
		{
			m2m_cfg_set("WLN_WirelessMode", "1");
			m2m_cfg_set("WLN_BasicRate", "3");
			m2m_cfg_set("WLN_FixedTxMode", "CCK");
			m2m_cfg_set("WLN_HT_MCS", "0");
		}
		else if ((strcmp(cfwm, "11G")==0)||(strcmp(cfwm, "11g")==0))
		{
			m2m_cfg_set("WLN_WirelessMode", "4");
			m2m_cfg_set("WLN_BasicRate", "351");
			m2m_cfg_set("WLN_FixedTxMode", "OFDM");
			m2m_cfg_set("WLN_HT_MCS", "7");
		}
		else if ((strcmp(cfwm, "11N")==0)||(strcmp(cfwm, "11n")==0))
		{
			m2m_cfg_set("WLN_WirelessMode", "6");
			m2m_cfg_set("WLN_BasicRate", "15");
			m2m_cfg_set("WLN_FixedTxMode", "HT");
			m2m_cfg_set("WLN_HT_MCS", "7");
		}
		else
		{
			m2m_cfg_set("WLN_WirelessMode", "9");
			m2m_cfg_set("WLN_BasicRate", "15");
			m2m_cfg_set("WLN_FixedTxMode", "HT");
			m2m_cfg_set("WLN_HT_MCS", "0");
		}
	}
	cyg_thread_delay(20);
	CFG_save(0);
	cyg_thread_delay(300);	
	//sys_reboot();
	mon_snd_cmd(MON_CMD_REBOOT);
}

// 8
int CGI_do_sys_config(http_req *req)
{
   char *CCMD, *file,*ping;
   int len,rc, dst;
   int rtt, i;	
   int loop = 5;
   if ((CCMD = WEB_query(req,"CCMD"))==0)
        return -1;

	switch (CCMD[0])
	{
	case '3':
		file = WEB_query(req, "files");
		if( !file )
			return CFG_ERANGE;
		
		file = WEB_upload_file(req, "files", &len);

		//rc=CFG_put_file(0, file, len) ;
        rc=http_put_file(0, req->content, file, len);
		if (rc)
			return CGI_RC_FILE_INVALID;
		
		mon_snd_cmd(MON_CMD_REBOOT);
		return CGI_RC_FACTORY_DEFAULT;
			
	case '1':
		//diag_printf("doing Restore Factory Default\n");
		//CFG_reset_default();
		CGI_reset_default();
		mon_snd_cmd(MON_CMD_REBOOT);
		return CGI_RC_FACTORY_DEFAULT;
		
	case '0':
		//diag_printf("doing Restart System\n");
		mon_snd_cmd(MON_CMD_REBOOT);
		return CGI_RC_REBOOT;
			
	case '4':
	{
		int  ping_len=0;
		ping = WEB_query(req,"PING");
		diag_printf("ping %s\n",ping);
		memset(PING_Result,0,sizeof(PING_Result));
		if ((ping!=NULL)&&((dst = nstr2ip(ping)) != 0))
		{
			for(i=1; i<=loop; i++)
			{
				if((rtt = net_ping_test(dst, i, 300)) < 0)
					{
					ping_len=strlen(PING_Result);
					sprintf(&PING_Result[ping_len]," %d: timeout<BR>", i);
					}
				else if (strlen(SavePingResult) > 0)
					{
					ping_len=strlen(PING_Result);
					sprintf(&PING_Result[ping_len], "%d: %s<BR>",  i, SavePingResult);
					}
		
				sprintf(SavePingResult,"");
			}

			return CGI_RC_PING_OK;
		} else {
			sprintf(PING_Result,"PING %s Failed<BR>",ping);
			return CGI_RC_PING_TIMEOUT;
		}
	}
		
	case '5':
		netif_clear_cnt(WAN_NAME);
		netif_clear_cnt(LAN_NAME);
		return 0;
		
	default:
		break;
	}
	
	return 0;
}

int CGI_do_sys_chk_pass(http_req *req) {
	char *CPW;
	char cpw[256];
   	if ((CPW = WEB_query(req,"CPW"))==0)
        return 0; // no password update
	// if same , ok
	CFG_get(CFG_SYS_ADMPASS, cpw);
	if (!strcmp(CPW,cpw)) return 0;

    return -1;
}

// 9
#if		MODULE_SYSLOG
int CGI_do_sys_log(http_req *req)
{
	clear_system_log();
	return 0;
}
int CGI_do_secure_log(http_req *req)
{
	clear_seurity_log();
	return 0;
}
int CGI_do_sys_log_mail(http_req *req)
{
	event_send_mail();
	return CGI_RC_SNDMAIL;
}
#endif
// 17
#if		MODULE_DHCPS
int CGI_do_lan_dhcp_clients(http_req *req) 
{ 
	char dhcpc_lname[8], *dhcpc_lvar;
	int i = 1;
	
	do 
	{
		sprintf(dhcpc_lname, "list%d", i);
		dhcpc_lvar = WEB_query(req, dhcpc_lname);
		
		DHCPD_leases_set(dhcpc_lvar);
	}
	while(i++ <256);
	
    return 0;                        
}
#endif

// 27
int CGI_do_rt_add(http_req *req) 
{ 
	int i=0, rc = 0;
	char *cmd,*entstr;
	char rtent[100];
	
	cmd = WEB_getvar(req,"cmd","");    
	entstr= WEB_getvar(req,"str","");    
       
    if (atoi(cmd) == 1) // delete
    {
		while (CFG_get(CFG_RT_ENTRY+i+1, rtent) != -1)
		{
			if(!strcmp(entstr, rtent))
			{
				CFG_del(CFG_RT_ENTRY+i+1);
				rc = RT_del_staticrt(entstr);
			}
			i++;
		}  
	}
	else // add
	{
		while (CFG_get(CFG_RT_ENTRY+i+1, rtent) != -1)
			i++;
		
		rc = RT_add_staticrt(entstr);
		if (rc != -1)
			CFG_set(CFG_RT_ENTRY+i+1, entstr);
	}                                           

	// Force to save configuration   
	if (rc == 0)
		rc = CGI_RC_CFG_OK;
                                                   
    return rc;           
}

int system2(char *str)
{
	struct iwreq iwr;
	struct eth_drv_sc 		*sc;
	cyg_netdevtab_entry_t 	*t;
	int	found = 0;

#ifdef CONFIG_WIRELESS
	memset(&iwr, 0, sizeof(iwr));
	//strncpy(iwr.ifr_name, "ra0", IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) str;

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		
		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
	       }
	}

	if (found == 1)	
	{
		if(rt28xx_ap_ioctl(sc, RTPRIV_IOCTL_SET, (caddr_t)&iwr) < 0)
		{
			diag_printf("RTPRIV_IOCTL_SET has error.\n");
		}
	}	
	else
	{
		diag_printf("Can't find ra0.\n");
		return -1;
	}

	return 0;
#else
	diag_printf("Wireless IF does not init.\n");
        return -1;
#endif
}

int CGI_do_wireless_simple_config_device(http_req *req) 
{
	int i, ret, id, id2, rc = 0;
	int WscPinCode, WscMode, WscConfMode, WscConfStatus;
	char cmdi[8], cmdi2[8];
	char *cmdv, *val, *cmdv2, *val2;
	char	wlanbuf[128];
	char WscPinCodeTemp[255]; 
	char WscModeTemp[10];
	char WscConfModeTemp[10];
	char WscConfStatusTemp[10];

	WscPinCodeTemp[0]='\0';
	WscModeTemp[0]='\0';
	WscConfModeTemp[0] = '\0';
	WscConfStatusTemp[0] = '\0';
	
	CFG_get_str(CFG_WLN_WscPinCode, WscPinCodeTemp);
	CFG_get_str(CFG_WLN_WscModeOption, WscModeTemp);
	CFG_get_str(CFG_WLN_WscConfMode, WscConfModeTemp);
	CFG_get_str(CFG_WLN_WscConfStatus, WscConfStatusTemp);
	
	WscPinCode = atoi(&WscPinCodeTemp[0]);
	WscMode = atoi(&WscModeTemp[0]);
	WscConfMode = atoi(&WscConfModeTemp[0]);
	WscConfStatus = atoi(&WscConfStatusTemp[0]);

	for (i = 0; i < 10; i++)
	{
		sprintf(cmdi2,"SET%d",i);
		cmdv2 = WEB_query(req,cmdi2);
		if (cmdv2 == 0)
			continue;
		
		val2 = strchr(cmdv2,'=');
		
		if ((unsigned int)val2 <= (unsigned int)cmdv2)
		{
			rc = CGI_RC_CFG_ERR;
			continue;
		}
		
		val2++;
		id2 = atoi(cmdv2);

		if (id2 == 0x4720100) //if update WscPinCode
			WscPinCode = atoi(val2);
		else if(id2 == 0x4700100) // if update WscModeOption
			WscMode = atoi(val2);
	}

	CFG_set(CFG_WLN_WscPinCode, &WscPinCode);
	CFG_set(CFG_WLN_WscModeOption, &WscMode);

	diag_printf("WscPinCode=%d\n", WscPinCode);
	diag_printf("WscMode=%d\n", WscMode);
	diag_printf("WscConfMode=%d\n", WscConfMode);
	diag_printf("WscConfStatus=%d\n", WscConfStatus);

	memset(wlanbuf, 0, 128);
	sprintf(wlanbuf, "WscGetConf=0");
	ret = system2(wlanbuf);

	memset(wlanbuf, 0, 128);
	sprintf(wlanbuf, "WscConfMode=%d", WscConfMode);
	ret = system2(wlanbuf);

	/* snowpin recommands that below is unnecessary. */
	memset(wlanbuf, 0, 128);
	sprintf(wlanbuf, "WscConfStatus=%d", WscConfStatus);
	ret = system2(wlanbuf);
	
	memset(wlanbuf, 0, 128);
	sprintf(wlanbuf, "WscMode=%d", WscMode);
	ret = system2(wlanbuf);

	if(WscMode == 1)
	{
		memset(wlanbuf, 0, 128);
		sprintf(wlanbuf, "WscPinCode=%d", WscPinCode);
		ret = system2(wlanbuf);	
	}

	memset(wlanbuf, 0, 128);
	sprintf(wlanbuf, "WscGetConf=1");
	ret = system2(wlanbuf);
	
	rc = CGI_RC_CFG_OK;

	return rc;
}

int CGI_do_wireless_security(http_req *req) 
{
	int i, id, id2, rc = 0;
	int ssid_index_num = 0;
	char cmdi[8], cmdi2[8];
	char SSIDIndexTemp[255]; 
	char *cmdv, *val, *cmdv2, *val2;

	SSIDIndexTemp[0]='\0';
	
	CFG_get_str(CFG_WLN_SSIDIndex, SSIDIndexTemp);
	
	ssid_index_num = atoi(&SSIDIndexTemp[0]);

	for (i = 0; i < 50; i++)
	{
		sprintf(cmdi2,"SET%d",i);

		cmdv2 = WEB_query(req,cmdi2);

		if (cmdv2 == 0)
			continue;
		
		val2 = strchr(cmdv2,'=');
		
		if ((unsigned int)val2 <= (unsigned int)cmdv2)
		{
			rc = CGI_RC_CFG_ERR;
			continue;
		}
		
		val2++;
		id2 = atoi(cmdv2);

		if (id2 == 0x4E90100) // search for SSIDIndex
		{
			ssid_index_num = atoi(val2);
			break;
		}
	}
	
	for (i = 0; i < 50; i++)
	{
		sprintf(cmdi,"SET%d",i);
		
		cmdv=WEB_query(req,cmdi);
		
		if (cmdv == 0)
			break;
		
		val = strchr(cmdv,'=');
		
		if ( (unsigned int)val <= (unsigned int)cmdv)
		{
			rc=CGI_RC_CFG_ERR;
			break;
		}
		
		*val = 0;
		val++;
		id = atoi(cmdv);
		
		if (id != 0)
		{
			if((CFG_ID_TYPE(id) == CFG_TYPE_STR) && (id != CFG_POE_USER) && (id != CFG_POE_PASS))
			{
				/* we don't want to see '\n' and '\r'*/
				char *c = 0;
				if((c = strpbrk(val, "\n\r")))
					*c = '\0';
			}

			char * var = CFG_id2str(id);
   			int len = strlen(var);
			int lastByte;

			memmove(&lastByte, var+len-1, 1);

    			if (len == 0)
        			break;
	
			if (!isdigit(var[len-1]) ) 
			{
				/* Only when SSID index == 0, WscNewKey, WscAuthType,WscEncrypType, and WscConfigured can be modified */
				if(!((ssid_index_num != 0) && (id == 0x4920200 || id == 0x4900100 || id == 0x4910100 || id == 0x4710100)))
					rc = CFG_set_str(id,val);
			}
			else 
			{
				/* AccessControlList0, AccessControlList1, AccessControlList2, AccessControlList3, AccessPolicy0, AccessPolicy1,
				    AccessPolicy2, and AccessPolicy3 are the special cases since the index is counted from 0 not from 1 */			  
				if(id == 0x4A60200 || id == 0x4A70200 || id == 0x4A80200 || id == 0x4A90200 ||
					id == 0x46A0100 || id == 0x46B0100 || id == 0x46C0100 || id == 0x46D0100)
				{
					if(atoi(&lastByte) == ssid_index_num)
						rc = CFG_set_str(id,val);
				}
				else if(atoi(&lastByte) == (ssid_index_num + 1))
					rc = CFG_set_str(id,val);
			}
			
			if (rc >= 0) 
				rc = CGI_RC_CFG_OK;		
		}
	}

	return rc; 
}

void CGI_get_cfg(http_req *req, int tag) 
{ 
	char val[255];  

	val[0]='\0';
	CFG_get_str(tag, val);
	
	if (CFG_ID_TYPE(tag) == CFG_TYPE_STR)
	{	
		char seperators[] = "\"";
		char *start = &val[0];
		char * end = NULL;
		char temp[255];
		char temp2[255];
		int lencnt = 0;

		memset(temp, 0x0, 255);
#if 1
		int i = 0;

		while (val[i] != 0)
		{
			if (val[i] == '\"')
			{
				strcpy(temp2, temp);
				sprintf(temp, "%s%s", temp2, "&quot;");
			}
			else if (val[i] == '\\')
			{
				strcpy(temp2, temp);
				sprintf(temp, "%s%s", temp2, "\\\\");
			}			
			else
			{
				strcpy(temp2, temp);
				sprintf(temp, "%s%c", temp2, val[i]);
			}
			i++;
		}

#else
		while ((end = strpbrk(start , seperators)) != NULL)
		{ 
			if ((end != NULL) && (*end == '\"'))
			{
				diag_printf("%s  %d\n", end, end-start);
				if (end != start)
				{
					strncpy(&temp[lencnt], start, end-start);
					lencnt += (end - start);
				}
				strcat(&temp[lencnt], "&quot;");
				lencnt += strlen("&quot;");
			} 
			start = end + 1;
		}
		
		if ((start - &val[0]) < strlen(val))
			strncpy(&temp[lencnt], start, strlen(start)+1);	
#endif
		
		WEB_printf( req, "%s", temp);
	}
	else	
		WEB_printf( req, "%s", val);
}

#if		MODULE_WANIF
int get_wan_state_value(http_req *req, struct ip_set *ip_point, int term)
{
  struct ip_set *wan_ipset = ip_point;
  int type, dns;
  int time_diff; 
  char strbuf[50];
  char *buffer = 0;

  CFG_get(CFG_WAN_IP_MODE, &type);
  switch(term)
	{	
	case 1:
		switch(wan_ipset->up)
		{
		case DISCONNECTED:
			buffer = "0";
			break;
		case CONNECTED:
			buffer = "1";
			break;
		case CONNECTING:
			buffer = "2";
			break;
		}
		break;
		
	case 2:
		buffer = NSTR(wan_ipset->ip);
		break;
		
	case 3:
		buffer = NSTR(wan_ipset->netmask);
		break
		;
	case 4:
		buffer = NSTR(wan_ipset->gw_ip);
		break;
		
	case 5:
		dns = SYS_dns_1;
		buffer = NSTR(dns);
		break;
		
	case 6:
		dns = SYS_dns_2;
		buffer = NSTR(dns);
		break;
		
	case 7:
		buffer = ESTR(SYS_wan_mac);
		break;
		
	case 8:		                		
		buffer = wan_ipset->domain;
		break;
		
	case 9:
		if (wan_ipset->conn_time)
			time_diff = time(0) - wan_ipset->conn_time;
		else
			time_diff = 0;
			
		sprintf(strbuf, "%d", time_diff);
		buffer=strbuf;
		break;
		
	case 10:
		/* only DHCP mode */
		if (type == 2)
			time_diff = DHCPC_remaining_time(wan_ipset->ifname);
		else
			time_diff = 0;
		
		sprintf(strbuf, "%d", time_diff);
		buffer=strbuf;
		break;
	}  

	WEB_printf( req, "%s", buffer);
  return 0;
}

void CGI_get_wan_state(http_req *req, int term)
{
  int ipmode;
  CFG_get(CFG_WAN_IP_MODE, &ipmode);
  
  if (opmode == 3)
  {
  	if (term == 1)
  		WEB_printf( req, "0");
  	else
  		WEB_printf( req, "");
  	return;
  }

  #ifdef CONFIG_PPTP_PPPOE
  extern struct ip_set ppp_ip_set[CONFIG_PPP_NUM_SESSIONS];
  int pptp_wanif;
  if(ipmode == PPTPMODE){
    CFG_get(CFG_PTP_WANIF, &pptp_wanif);
    if(pptp_wanif == 2)
	  ipmode = 9;//pppoe mode for pptp vpn
  }
  #endif

  #ifdef CONFIG_L2TP_OVER_PPPOE
  extern struct ip_set l2tp_wan_ip_set[CONFIG_PPP_NUM_SESSIONS];
  int l2tp_wanif;
  if(ipmode == L2TPMODE){
    CFG_get(CFG_L2T_WANIF, &l2tp_wanif);
    if(l2tp_wanif == 2)
	  ipmode = 10;//pppoe mode for l2tp
  }
  #endif

  switch(ipmode){
  	#ifdef CONFIG_PPTP_PPPOE
  	case 9:
	  get_wan_state_value(req, &ppp_ip_set[0], term);
	  break;
    #endif

    #ifdef CONFIG_L2TP_OVER_PPPOE
	case 10:
	  get_wan_state_value(req, &l2tp_wan_ip_set[0], term);
	  break;
    #endif

	default:
      get_wan_state_value(req, &primary_wan_ip_set[0], term);
	  break;
  }
}
#endif	//MODULE_WANIF

void CGI_get_time(http_req *req)
{
	WEB_printf( req, "%d", GMTtime(0));
}

void CGI_get_uptime(http_req *req)
{
	WEB_printf( req, "%d", time(0));
}

void CGI_get_ntpinfo(http_req *req)
{
	int te,now;
	struct tm TM;
	
	te = get_ntp_type();
	now = ntp_getlocaltime(0);
	gmtime_r(&now,&TM);
	WEB_printf( req, "\"%d\",\"0\",\"%d/%d/%d\",\"%d:%d:%d\",\"\",\"\",\"\",\"\"",te,
				TM.tm_mon+1,TM.tm_mday,TM.tm_year+1900,TM.tm_hour,TM.tm_min,TM.tm_sec);
}


static void _mask(struct sockaddr *sa, char *buf, int _len)
{
    unsigned char *cp = ((char *)sa) + 4;
    int len = sa->sa_len - 4;
    int tot = 0;

    while (len-- > 0) {
        if (tot) *buf++ = '.';
        buf += diag_sprintf(buf, "%d", *cp++);
        tot++;
    }

    while (tot < 4) {
        if (tot) *buf++ = '.';
        buf += diag_sprintf(buf, "%d", 0);
        tot++;
    }
}

// routing table entry, format: d.d.d.d,n.n.n.n,g.g.g.g,metric,if
// routing table entry number
static int rte_num;
static int CGI_routing_entry(struct radix_node *rn, void *vw)
{
    struct rtentry *rt = (struct rtentry *)rn;
    struct sockaddr *dst, *gate, *netmask, *genmask;
    char addr[32]={0}, addr1[32]={0}, addr2[32]={0};
    http_req *req = (http_req *)vw;

    dst = rt_key(rt);
    gate = rt->rt_gateway;
    netmask = rt_mask(rt);
    genmask = rt->rt_genmask;
    if ((rt->rt_flags & (RTF_UP | RTF_LLINFO)) == RTF_UP) 
	{
        _inet_ntop(dst, addr, sizeof(addr));

        if (netmask != NULL) {
            _mask(netmask, addr2, sizeof(addr2));
        } else {
        	if(rt->rt_flags & RTF_HOST) 
        		sprintf(addr2, "255.255.255.255");
        	else
            	sprintf(addr2, " ");
        }
       
        if (gate != NULL && gate->sa_family == AF_INET) {
            _inet_ntop(gate, addr1, sizeof(addr1));
        } else if(gate != NULL && gate->sa_family == AF_LINK) {
        	_inet_ntop((struct sockaddr *)rt_key(rt), addr1, sizeof(addr1));
        } else {
            sprintf(addr1,"unknown");
        }
		if (rte_num++!=0)
			WEB_printf(req,",");
        WEB_printf(req, "\n'%s,%s,%s,%d,", addr,addr2,addr1,rt->rt_rmx.rmx_hopcount);
        if_indextoname(rt->rt_ifp->if_index, addr);
        WEB_printf(req, "%s'", addr);
    }
    return 0;
}

void CGI_show_routing_table(http_req *req)
{
	struct radix_node_head *rnh;
	int i;

	rte_num=0;
	for (i = 1; i <= AF_MAX; i++) {
        if ((rnh = rt_tables[i]) != NULL) {
           rnh->rnh_walktree(rnh, CGI_routing_entry, req);
        }
    }
}

#if		MODULE_DHCPS
static void dhcpd_entry(char *buf, int id, void *arg1)
{
	http_req *req = (http_req *)arg1;
	
	if (id>1)
		WEB_printf(req, ",");
		
	WEB_printf(req, "\n'%s'", buf);
}

inline void CGI_show_dhcpd_clist(http_req *req)
{
	DHCPD_dumplease(dhcpd_entry, 0x3, req);
}

inline void CGI_show_dhcpd_dyn_clist(http_req *req)
{
	DHCPD_dumplease(dhcpd_entry, 0x1, req);
}

void CGI_show_dhcpd_leases_num(http_req *req)
{
	int num=0;
	num = DHCPD_dumplease(NULL, 0x3);
	WEB_printf(req, "%d", num);
}
#endif	//MODULE_DHCPS

#if		MODULE_UPNP
void CGI_show_upnp_pmap(http_req *req)
{
	int i=0;
	int maxNum=portmap_num() ;
	char entry[100];
	
	for( i=0; i<maxNum ; i++) {	
	   	getUpnpPmapEntry(i,entry) ;	
		if (i)
			WEB_printf(req, ",");
	   WEB_printf(req, "\n'%s'", entry);
	}
}
#endif

#if		MODULE_SYSLOG
void CGI_print_log(http_req *req, int type)
{ 
    char time_info[32];
    UINT32	ev_dateTime;
	int	diff = ntp_time() - time(0);
	
    EventLogEntry *ev;
	char *p;
	//int i, index, count;
	
	//index = 1;
	//count = 65535;
	
	syslog_mutex_lock();
	//p = FindEvent(type);
	//for (i=1; (p != 0) && (i < index); i++, p = NextEvent(p, type));
	//for (i=0; (p != 0) && (i < count); i++, p = NextEvent(p, type))
	for (p=FindEvent(type); p != 0; p=NextEvent(p, type))
	{
		ev = (EventLogEntry *)p;
		//if ((ev->class & LOG_CLASS_MASK) == LOG_USER)
		//{
	    	ev_dateTime = gmt_translate2localtime(diff + ev->unix_dateTime);
			ctime_r(&ev_dateTime, time_info);
			time_info[strlen(time_info)-1] = 0;  // Strip \n
			WEB_printf (req, "[%s]:%s %s \n",
						time_info, 
						EventModule(ev), 
						ev->text);
		//}
	}
	syslog_mutex_unlock();
}

void CGI_print_log_list(http_req *req, int type)
{ 
    char time_info[32];
	UINT32	ev_dateTime;
	unsigned long curr_time = time(0);
	int	diff = ntp_time() - curr_time;
	
    EventLogEntry *ev;
	char *p;
	//int i, index, count,tt;
	int i;
	int tt;
	
	//index = 1;
	//count = 65535;
	
	tt = Is_ntp_ever_sync();
	
	syslog_mutex_lock();
	//p = FindEvent(type);
	//for (i=1; (p != 0) && (i < index); i++, p = NextEvent(p, type));
	for (i=0, p=FindEvent(type); p != 0; i++, p=NextEvent(p, type))
	{
		ev = (EventLogEntry *)p;
		//p = NextEvent(p, type);
		//if ((ev->class & LOG_CLASS_MASK) == LOG_USER)
		//{
			ev_dateTime = gmt_translate2localtime(diff + ev->unix_dateTime);
			ctime_r(&ev_dateTime, time_info);
			time_info[strlen(time_info)-1] = 0;  // Strip \n
			WEB_printf (req, "%s\n'%d','%d','%d','%d','%s','%s %s'",
						i ? "," : "",
						i+1,
						tt, 
						(curr_time - ev->unix_dateTime), 
						ev_dateTime, 
						time_info, 
						EventModule(ev), 
						ev->text);
		//}
	}
	syslog_mutex_unlock();
}
#endif

char * CGI_get_client_mac(http_req *req)
{
	static char macstr[20];
	struct sockaddr_in addr;
	socklen_t len;
	len = sizeof(addr);
	char mac[6];
	
	if(getpeername(req->conn_fd, (struct sockaddr*)&addr,  &len) < 0)
		return 0;
	
	netarp_get_one(addr.sin_addr.s_addr, mac);
	sprintf( macstr, "%s", ether_ntoa(mac));
	return macstr;
}

int CGI_do_uboot_upgrade(http_req *req)
{
	char *file;
	int len,rc;
		if (upgrade_mem) 
		return CGI_RC_FILE_INVALID; // avoid incorrect cgi call
	diag_printf("CGI_do_uboot_upgrade!!!\n");
	file = WEB_upload_file(req, "files", &len);
	
    	rc = http_put_file(3, req->content, file, len);
	
	upgrade_mem = req->content;
	req->content = 0;
	CGI_upgrade_free();
	if (rc)
		return CGI_RC_FILE_INVALID;
	mon_snd_cmd(MON_CMD_REBOOT);
	return CGI_RC_UPGRADE;
}

static int CGI_do_load_cert(http_req *req)
{
	char *file;
	int len,rc=0;
	
	if (upgrade_mem) 
		return CGI_RC_FILE_INVALID; // avoid incorrect cgi call
	diag_printf("CGI_do_uboot_upgrade!!!\n");
	file = WEB_upload_file(req, "files", &len);
	if(file)
	{
		unsigned char hdr[SSL_CERT_HDR_SIZE]={0};
		
		flsh_hfcfg_erase(SSL_CERT_OFFSET,SSL_CERT_MAX_SIZE);
		hdr[0] = (len>>24)&0xff;
		hdr[1] = (len>>16)&0xff;
		hdr[2] = (len>>8)&0xff;
		hdr[3] = len&0xff;
		
		flsh_hfcfg_write(SSL_CERT_OFFSET, hdr,SSL_CERT_HDR_SIZE);
		flsh_hfcfg_write(SSL_CERT_OFFSET+SSL_CERT_HDR_SIZE, file,len);
	}
	upgrade_mem = req->content;
	req->content = 0;
	CGI_upgrade_free();
	if (rc)
		return CGI_RC_FILE_INVALID;
	
	return CGI_RC_UPGRADE;	
}
	
int CGI_upgrade(http_req *req)
{    
	char *file;
	int len,rc;
	//char val[32];
	if (upgrade_mem) 
		return CGI_RC_FILE_INVALID; // avoid incorrect cgi call
	
	file = WEB_upload_file(req, "files", &len);
	//if (len >= 0xA0000)
	//sprintf(val, "Filesize=%d\n", len);	
	//Joo_uart_send(val);	
	if(len <= (760 * 1024) || len>=FLASH_FWM_MAX_SIZE)
	{
		CGI_upgrade_free();
		return CGI_RC_FILE_INVALID;
	}
   	//rc = CFG_put_file(1, file, len) ;
    rc = http_put_file(1, req->content, file, len);
	
	upgrade_mem = req->content;
	req->content = 0;
	CGI_upgrade_free();
	if (rc)
		return CGI_RC_FILE_INVALID;
	
	mon_snd_cmd(MON_CMD_REBOOT);
#ifdef	CONFIG_ZWEB
	zweb_location = 0;
#endif
	return CGI_RC_UPGRADE;
}

int CGI_SAMD_upgrade(http_req *req)
{    
	char *file;
	int len,rc;
	char val[48];
	
	if (upgrade_mem)
	{
		Joo_uart_send("CGI_RC_FILE_INVALID\n");
		return CGI_RC_FILE_INVALID; // avoid incorrect cgi call
	}
	file = WEB_upload_file(req, "files", &len);

	rc = SAMD_firmware_download(file, len, 0);
#if 1
	if (-1 == rc)
	{
		sprintf(val, "SAMD Upgrade Fails, Filesize=%d\n", len);	
		Joo_uart_send(val);
	}
#endif

#if 0	
	if(len>=FLASH_FWM_MAX_SIZE)
	{
		CGI_upgrade_free();
		return CGI_RC_FILE_INVALID;
	}
   	//rc = CFG_put_file(1, file, len) ;
    rc = http_put_file(1, req->content, file, len);
#endif
	
	upgrade_mem = req->content;
	req->content = 0;
	CGI_upgrade_free();
	if (rc)
	{
		//return CGI_RC_FILE_INVALID;
		return (rc);
	}
	// mon_snd_cmd(MON_CMD_REBOOT);
#ifdef	CONFIG_ZWEB
	// zweb_location = 0;
#endif
	return CGI_RC_UPGRADE;
}

int CGI_TIMO_upgrade(http_req *req)
{    
	char *file;
	int len,rc;
	char val[48];
	
	if (upgrade_mem)
	{
		Joo_uart_send("CGI_RC_FILE_INVALID\n");
		return CGI_RC_FILE_INVALID; // avoid incorrect cgi call
	}
	file = WEB_upload_file(req, "files", &len);

	rc = SAMD_firmware_download(file, len, 1);
#if 1
	if (-1 == rc)
	{
		sprintf(val, "SAMD Upgrade Fails, Filesize=%d\n", len);	
		Joo_uart_send(val);
	}
#endif

#if 0	
	if(len>=FLASH_FWM_MAX_SIZE)
	{
		CGI_upgrade_free();
		return CGI_RC_FILE_INVALID;
	}
   	//rc = CFG_put_file(1, file, len) ;
    rc = http_put_file(1, req->content, file, len);
#endif
	
	upgrade_mem = req->content;
	req->content = 0;
	CGI_upgrade_free();
	if (rc)
		return CGI_RC_FILE_INVALID;
	
	// mon_snd_cmd(MON_CMD_REBOOT);
#ifdef	CONFIG_ZWEB
	zweb_location = 0;
#endif
	return CGI_RC_UPGRADE;
}


int CGI_upgrade_upload(http_req *req)
{
	int len;
	
	upgrade_file = WEB_upload_file(req, "files", &len);
	if (!upgrade_file) 
		return -1;
	
	if (upgrade_mem) 
		return -1; // if previous request are not finished, skip
	
	upgrade_time_stamp=time(0);
	upgrade_file_len = len;
	upgrade_mem = req->content;
	req->content = 0;
	
	timeout(&CGI_upgrade_free, upgrade_time_stamp, 300);
	return 0;
}

int CGI_upgrade_save(http_req *req)
{ 
	int rc;
	
	if (!upgrade_mem) 
		return CGI_RC_FILE_INVALID; // avoid incorrect cgi call
	
	untimeout(&CGI_upgrade_free,upgrade_time_stamp);
	//rc = CFG_put_file(1, upgrade_file, upgrade_file_len);
    rc = http_put_file(1, upgrade_mem, upgrade_file, upgrade_file_len);
	
	CGI_upgrade_free();
	if (rc)
		return CGI_RC_FILE_INVALID;
	
#ifdef	CONFIG_ZWEB
	zweb_location = 0;
#endif
	return CGI_RC_UPGRADE;
}

void CGI_upgrade_free(void)
{
	if (upgrade_mem)
	{
		diag_printf("CGI_upgrade_free() be called!!!\n");
		
        CGI_free_content(upgrade_mem);
        upgrade_mem = 0;
	}
}

#ifdef CONFIG_NTP
int CGI_do_ntp_config(http_req *req)
{
	char *cmd;
	int  c_time;
	
	if ((cmd=WEB_query(req,"TIMES"))==0)
		return 0;
	
	c_time = atol(cmd);
	if(c_time)
		non_ntp_update(c_time);
	
	return CGI_RC_CFG_OK;
}
#endif

char *cgiFunc(webpage_entry* fileptr, http_req *req)
{
//qh add
/*
	int result = 0;
	char * cmd;

	cmd=WEB_query(req,"CMD");
	if (cmd) {
		result = CGI_do_cmd(req);
		if ((result == CGI_RC_REBOOT) || (result == CGI_RC_UPGRADE))
			return "reboot.htm";
		else if (result > 0)
			return 0;// WEB_query(req,"GO");
	}
*/
	return 0;
}

int	cgiFileMiss(http_req *req)
{
	if (!strncmp(req->file, CMD_PAGE, 3))
	{
		extern time_t time_pages_created;
		
		add_headers( req, HTTP_REQUEST_OK, "Ok", (char*) 0, "text/html; charset=", -1, time_pages_created, 1);
		send_response(req);
		WEB_printf(req, "%s", RESTART_MSG_PAGE);
		
		CGI_do_cmd(req);
		return 0;
	}
	
	return -1;
}


typedef struct nat_sessinfo_s {
	uint32_t sip;
	uint32_t dip;
	uint16_t sport;
	uint16_t dport;
	uint16_t prot;
	uint8_t s_tcpstate;
	uint8_t d_tcpstate;
	uint32_t age;
} nat_sessinfo_t;

typedef struct nat_brif_s {
	uint32_t sip;
	uint16_t tcpnum;
	uint16_t udpnum;
} nat_brif_t;

#define MAX_NATBRIF_NUM		512
int CGI_get_nat_session_brief(http_req *req)
{
	nat_sessinfo_t *pnatinfo;
	int buflen, sess_num;
	extern int nat_table_max;
	int sidx, bridx, brifnum;
	nat_brif_t *pbrif;

	buflen = sizeof(nat_sessinfo_t)*nat_table_max +
		 MAX_NATBRIF_NUM*sizeof(nat_brif_t);
	if ((pnatinfo=malloc(buflen)) == NULL) {
		return -1;
	}
	/*  Get NAT session info  */
	if ((sess_num = ipnat_session_list(pnatinfo, buflen)) <= 0)
		goto brif_exit;
	sess_num /= sizeof(nat_sessinfo_t);
	pbrif = (nat_brif_t *) &pnatinfo[nat_table_max];

	/*  Prepare NAT session brief */
	for (sidx=0, brifnum=0; sidx<sess_num; sidx++) {
		for (bridx=0; bridx<brifnum; bridx++) {
			if (pnatinfo[sidx].sip == pbrif[bridx].sip) {
				if (pnatinfo[sidx].prot == 6) {
					/*  TCP  */
					pbrif[bridx].tcpnum ++;
				} else if (pnatinfo[sidx].prot == 17) {
					/*  UDP  */
					pbrif[bridx].udpnum ++;
				}
				break;
			}
		}
		if (bridx == brifnum) {
			pbrif[bridx].sip = pnatinfo[sidx].sip;
			pbrif[bridx].tcpnum = pbrif[bridx].udpnum = 0;
			brifnum ++;
			if (pnatinfo[sidx].prot == 6)
				pbrif[bridx].tcpnum = 1;
			else if (pnatinfo[sidx].prot == 17)
				pbrif[bridx].udpnum = 1;
		}
	}

	/*  Output brief  */
	for (bridx=0; bridx<brifnum; bridx++) {
		if (bridx>0)
			WEB_printf(req, ",");
		WEB_printf(req, "\n'%s;%d;%d'", NSTR(pbrif[bridx].sip), 
				pbrif[bridx].tcpnum, pbrif[bridx].udpnum);
	}

brif_exit:
	free(pnatinfo);
	return 0;
}

CGI_get_nat_session_list(http_req *req)
{
	nat_sessinfo_t *pnatinfo;
	int buflen, sess_num;
	extern int nat_table_max;
	uint32_t client_ip;
	char *pval, *protnamelist[]={"TCP","UDP"}, *protname=NULL;
	int i, cnt;
	struct in_addr claddr;

	if ((pval = WEB_query(req, "client")) == NULL)
		return -1;
	if (inet_aton(pval, &claddr) == 0)
		return -1;
	client_ip = ntohl(claddr.s_addr);

	buflen = sizeof(nat_sessinfo_t)*nat_table_max;
	if ((pnatinfo=malloc(buflen)) == NULL) {
		return -1;
	}
	/*  Get NAT session info  */
	if ((sess_num = ipnat_session_list(pnatinfo, buflen)) <= 0)
		goto sess_exit;
	sess_num /= sizeof(nat_sessinfo_t);

	for (i=0, cnt=0; i<sess_num; i++) {
		if (pnatinfo[i].sip == client_ip) {
			if (pnatinfo[i].prot==6)
				protname = protnamelist[0];
			else if (pnatinfo[i].prot==17)
				protname = protnamelist[1];
			else
				continue;
			if (cnt >0)
				WEB_printf(req, ",");
			WEB_printf(req, "\n'%s;%s;%d;%s;%d;%d'",
					protname, pval, pnatinfo[i].sport, 
					NSTR(pnatinfo[i].dip),
					pnatinfo[i].dport, pnatinfo[i].age);
			cnt++;
		}
	}
	
sess_exit:
	free(pnatinfo);
	return 0;
}

extern int Oray_DDNS_Webshow(int x, unsigned char *buf, int *status);
void CGI_get_ddns_oray_list(http_req *req, char *name)
{
	int x = 0;
	int status = 0;
	unsigned char buf[50], buf_temp[2];
	char val[200];

	//char *name = "Oray_DDNS_LIST";

	do
	{
		Oray_DDNS_Webshow(x, buf, &status);
		if (status > 0)
		{
			memset(val,0,sizeof(val));
			sprintf(buf_temp, "%d", status);
			strcat(val, buf_temp);
			strcat(val, ",");
			strcat(val, buf);
			WEB_printf(req, "<input name='%s%d' value='%s' type='hidden'>", name, x, val);
		}
		x++;
	}while(status > 0);
}

#ifdef CONFIG_DUALWAN
void CGI_get_dwan(http_req *req)
{
	char *buf;
	
	buf=inet_ntoa(*(struct in_addr *)&primary_wan_ip_set[1].ip);
	WEB_printf( req, "%s", buf);
}
#endif

int OidQueryInformation(unsigned short OidQueryCode, char *DeviceName, void *ptr, unsigned long PtrLength)
{
    struct iwreq wrq;

    //strcpy(wrq.ifr_ifrn.ifrn_name, DeviceName);
    wrq.u.data.length = PtrLength;
    wrq.u.data.pointer = (caddr_t) ptr;
    wrq.u.data.flags = OidQueryCode;

	struct eth_drv_sc 		*sc;
	cyg_netdevtab_entry_t 	*t;
	int	found = 0;

#ifdef CONFIG_WIRELESS
	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		
		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
	       }
	}

	if (found == 1)	
		return(rt28xx_ap_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&wrq));
	else
		return -1;
#else
	diag_printf("Wireless IF does not init.\n");
	return -1;
#endif   
}

int  parse_config_file(void)
{
	char	tmp_buffer[260];
	char val[255];     
	val[0] = '\0';
	char	*tok = NULL;
	int		i, new_driver, s, idx, wake_up_dot1x = 0;
	int		bssid_num;
	int		start_uam = 0;
	unsigned char version_number[8] = {0};
	int		wake_up_wapid = 0;

	new_driver = 0;

	if ( OidQueryInformation(RT_OID_QUERY_DRIVER_VERSION, "ra0", &version_number, sizeof(unsigned char)*8) < 0 )
	{
		if ( OidQueryInformation(RT_OID_VERSION_INFO,"ra0", &version_number, sizeof(unsigned char)*8) < 0 )
		{
			new_driver = 0;
		}
	}

	version_number[7] = '\0';

	if (version_number[0] >= '1')
	{
		if (version_number[2] == '5')
		{
			new_driver = 1;
			CFG_set(CFG_WLN_NewVersion, "1");
		}
		else if (version_number[2] >= '6')
		{
			new_driver = 2;
			CFG_set(CFG_WLN_NewVersion, "2");
		}
		else
		{
			new_driver = 0;
			CFG_set(CFG_WLN_NewVersion, "0");
		}
	}
	else
	{
		new_driver = 0;
		CFG_set(CFG_WLN_NewVersion, "0");
	}

	new_driver = 2;
	CFG_set(CFG_WLN_NewVersion, "2");

	g_new_version = new_driver;

	CFG_get_str(CFG_WLN_BssidNum, val);
	
	if (strcmp(val, "") == 0)
	{
		idx = 1;
		bssid_num = 1;
	}
	else
	{
		idx = atoi(val);
		bssid_num = atoi(val);
	}

	/* SSID */
	if (new_driver == 0)
	{
   		bzero(tmp_buffer, sizeof(char)*260);
		memset(val, 0 ,255);
		CFG_get_str(CFG_WLN_SSID, val);

		if (strcmp(val, "") == 0)
			strcpy(tmp_buffer, "Ralink11nInitAP");
		else
			strncpy(tmp_buffer, val, strlen(val));
   
		for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
		{
			mBSSID[i].enable = 1;
			sprintf(mBSSID[i].SSID, "%s", tok);
		}
	}
	else
	{
		for (i = 0; i < bssid_num; i++)
		{
			switch(i)
			{
				memset(val, 0 ,255);
				
				case 0:
					CFG_get_str(CFG_WLN_SSID1, val);
					break;
				case 1:
					CFG_get_str(CFG_WLN_SSID2, val);
					break;
				case 2:
					CFG_get_str(CFG_WLN_SSID3, val);
					break;
				case 3:
					CFG_get_str(CFG_WLN_SSID4, val);
					break;
				default:
					break;
			}
			mBSSID[i].enable = 1;
			if (strcmp(val, "") == 0)
				sprintf(mBSSID[i].SSID, "%s%d", "Ralink-eCosAP", i+1);
			else
				sprintf(mBSSID[i].SSID, "%s", val);
		}
	}

	/* HideSSID */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_HideSSID, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
		mBSSID[i].HideSSID = atoi(tok);
	}

	/* TxRate */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_TxRate, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
		mBSSID[i].TxRate = atoi(tok);
	}

	/* SecurityMode */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_SecurityMode, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
		{
			mBSSID[i].SecurityMode = atoi(tok);
			if (mBSSID[i].SecurityMode == 4)
				start_uam = 1;
		}
	}

	/* AuthMode */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_AuthMode, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "OPEN");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
		{
			sprintf(mBSSID[i].AuthMode, "%s", tok);
			if ((strcmp(tok, "WPA") == 0) || (strcmp(tok, "WPA2") == 0) || (strcmp(tok, "WPA1WPA2") == 0))
				wake_up_dot1x = 1;
				else if ((strcmp(tok, "WAIPSK") == 0) || (strcmp(tok, "WAICERT") == 0))
					wake_up_wapid = 1;
		}
	}

	/* EncrypType */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_EncrypType, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "NONE");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
			sprintf(mBSSID[i].EncrypType, "%s", tok);
	}

	/* WPAPSK */
	if (new_driver == 0)
	{
		bzero(tmp_buffer, sizeof(char)*260);
		memset(val, 0 ,255);
		CFG_get_str(CFG_WLN_WPAPSK, val);

		if (strcmp(val, "") == 0)
		{
			sprintf(mBSSID[0].WPAPSK, "%s", "");
		}
		else
		{
			strncpy(tmp_buffer, val, strlen(val));
			for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
				sprintf(mBSSID[i].WPAPSK, "%s", tok);
		}
	}
	else
	{
		for (i = 0; i < idx; i++)
		{
			if (i < bssid_num)
			{
				switch(i)
				{
					memset(val, 0 ,255);
					
					case 0:
						CFG_get_str(CFG_WLN_WPAPSK1, val);
						break;
					case 1:
						CFG_get_str(CFG_WLN_WPAPSK2, val);
						break;
					case 2:
						CFG_get_str(CFG_WLN_WPAPSK3, val);
						break;
					case 3:
						CFG_get_str(CFG_WLN_WPAPSK4, val);
						break;
					default:
						break;
				}
				if (strcmp(val, "") == 0)
					strcpy(mBSSID[i].WPAPSK, "");
				else
					sprintf(mBSSID[i].WPAPSK, "%s", val);
			}
		}
	}

	/* DefaultKeyID */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_DefaultKeyID, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "1");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
			mBSSID[i].DefaultKeyID = atoi(tok);
	}

	/* Key1Type */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_Key1Type, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
			mBSSID[i].Key1Type = atoi(tok);
	}

	/* Key1Str */
	if (new_driver == 0)
	{
		bzero(tmp_buffer, sizeof(char)*260);
		memset(val, 0 ,255);
		CFG_get_str(CFG_WLN_Key1Str, val);

		if (strcmp(val, "") == 0)
		{
			sprintf(mBSSID[0].Key1Str, "%s", "");
		}
		else
		{
			strncpy(tmp_buffer, val, strlen(val));
			for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
				sprintf(mBSSID[i].Key1Str, "%s", tok);
		}
	}
	else
	{
		for (i = 0; i < idx; i++)
		{
			if (i < bssid_num)
			{
				switch(i)
				{
					memset(val, 0 ,255);
					
					case 0:
						CFG_get_str(CFG_WLN_Key1Str1, val);
						break;
					case 1:
						CFG_get_str(CFG_WLN_Key1Str2, val);
						break;
					case 2:
						CFG_get_str(CFG_WLN_Key1Str3, val);
						break;
					case 3:
						CFG_get_str(CFG_WLN_Key1Str4, val);
						break;
					default:
						break;
				}
				if (strcmp(val, "") == 0)
					strcpy(mBSSID[i].Key1Str, "");
				else
					sprintf(mBSSID[i].Key1Str, "%s", val);
			}
		}
	}

	/* Key2Type */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_Key2Type, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
			mBSSID[i].Key2Type = atoi(tok);
	}

	/* Key2Str */
	if (new_driver == 0)
	{
		bzero(tmp_buffer, sizeof(char)*260);
		memset(val, 0 ,255);
		CFG_get_str(CFG_WLN_Key2Str, val);

		if (strcmp(val, "") == 0)
		{
			sprintf(mBSSID[0].Key2Str, "%s", "");
		}
		else
		{
			strncpy(tmp_buffer, val, strlen(val));
			for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
				sprintf(mBSSID[i].Key2Str, "%s", tok);
		}
	}
	else
	{
		for (i = 0; i < idx; i++)
		{
			if (i < bssid_num)
			{
				switch(i)
				{
					memset(val, 0 ,255);
					
					case 0:
						CFG_get_str(CFG_WLN_Key2Str1, val);
						break;
					case 1:
						CFG_get_str(CFG_WLN_Key2Str2, val);
						break;
					case 2:
						CFG_get_str(CFG_WLN_Key2Str3, val);
						break;
					case 3:
						CFG_get_str(CFG_WLN_Key2Str4, val);
						break;
					default:
						break;
				}

				if (strcmp(val, "") == 0)
					sprintf(mBSSID[i].Key2Str, "%s", "");
				else
					sprintf(mBSSID[i].Key2Str, "%s", val);
			}
		}
	}

	/* Key3Type */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_Key3Type, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
			mBSSID[i].Key3Type = atoi(tok);
	}

	/* Key3Str */
	if (new_driver == 0)
	{
		bzero(tmp_buffer, sizeof(char)*260);
		memset(val, 0 ,255);
		CFG_get_str(CFG_WLN_Key3Str, val);

		if (strcmp(val, "") == 0)
		{
			sprintf(mBSSID[0].Key3Str, "%s", "");
		}
		else
		{
			strncpy(tmp_buffer, val, strlen(val));
			for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
				sprintf(mBSSID[i].Key3Str, "%s", tok);
		}
	}
	else
	{
		for (i = 0; i < idx; i++)
		{
			if (i < bssid_num)
			{
				switch(i)
				{
					memset(val, 0 ,255);
					
					case 0:
						CFG_get_str(CFG_WLN_Key3Str1, val);
						break;
					case 1:
						CFG_get_str(CFG_WLN_Key3Str2, val);
						break;
					case 2:
						CFG_get_str(CFG_WLN_Key3Str3, val);
						break;
					case 3:
						CFG_get_str(CFG_WLN_Key3Str4, val);
						break;
					default:
						break;
				}

				if (strcmp(val, "") == 0)
					sprintf(mBSSID[i].Key3Str, "%s", "");
				else
					sprintf(mBSSID[i].Key3Str, "%s", val);
			}
		}
	}

	/* Key4Type */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_Key4Type, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
			mBSSID[i].Key4Type = atoi(tok);
	}

	/* Key4Str */
	if (new_driver == 0)
	{
		bzero(tmp_buffer, sizeof(char)*260);
		memset(val, 0 ,255);
		CFG_get_str(CFG_WLN_Key4Str, val);

		if (strcmp(val, "") == 0)
		{
			sprintf(mBSSID[0].Key4Str, "%s", "");
		}
		else
		{
			strncpy(tmp_buffer, val, strlen(val));
			for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
				sprintf(mBSSID[i].Key4Str, "%s", tok);
		}
	}
	else
	{
		for (i = 0; i < idx; i++)
		{
			if (i < bssid_num)
			{
				switch(i)
				{
					memset(val, 0 ,255);
					
					case 0:
						CFG_get_str(CFG_WLN_Key4Str1, val);
						break;
					case 1:
						CFG_get_str(CFG_WLN_Key4Str2, val);
						break;
					case 2:
						CFG_get_str(CFG_WLN_Key4Str3, val);
						break;
					case 3:
						CFG_get_str(CFG_WLN_Key4Str4, val);
						break;
					default:
						break;
				}

				if (strcmp(val, "") == 0)
					sprintf(mBSSID[i].Key4Str, "%s", "");
				else
					sprintf(mBSSID[i].Key4Str, "%s", val);
			}
		}
	}

	/* WapiPskType */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_WapiPskType, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
			mBSSID[i].WapiKeyType = atoi(tok);
	}

	/* Wapi Psk key */
	for (i = 0; i < idx; i++)
	{
		if (i < bssid_num)
		{
			switch(i)
			{
				memset(val, 0 ,255);
				
				case 0:
					CFG_get_str(CFG_WLN_WapiPsk1, val);
					break;
				case 1:
					CFG_get_str(CFG_WLN_WapiPsk2, val);
					break;
				case 2:
					CFG_get_str(CFG_WLN_WapiPsk3, val);
					break;
				case 3:
					CFG_get_str(CFG_WLN_WapiPsk4, val);
					break;
				default:
					break;
			}
			if (strcmp(val, "") == 0)
				strcpy(mBSSID[i].WapiKey, "");
			else
				sprintf(mBSSID[i].WapiKey, "%s", val);
		}
	}

	/* IEEE8021X */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_IEEE8021X, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
		{
			mBSSID[i].IEEE8021X = atoi(tok);
			if (atoi(tok) == 1)
				wake_up_dot1x = 1;
		}
	}

	/* PreAuth */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_PreAuth, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
			mBSSID[i].PreAuth = atoi(tok);
	}

	/* PMKCachePeriod */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_PMKCachePeriod, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "10");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
			mBSSID[i].PMKCachePeriod = atoi(tok);
	}

	/* RekeyMethod */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_RekeyMethod, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "DISABLE");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
			sprintf(mBSSID[i].RekeyMethod, "%s", tok);
	}

	/* RekeyInterval */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_RekeyInterval, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "3600");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		if (i < bssid_num)
			mBSSID[i].RekeyInterval = atoi(tok);
	}

	/* Vlan Name */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_VLANName, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "ralink-vlan0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		sprintf(mBSSID[i].VlanName, "%s", tok);
	}

	/* Vlan ID */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_VLANID, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		mBSSID[i].VlanId = atoi(tok);
	}

	/* Vlan Priority */
	bzero(tmp_buffer, sizeof(char)*260);
	memset(val, 0 ,255);
	CFG_get_str(CFG_WLN_VLANPriority, val);

	if (strcmp(val, "") == 0)
		strcpy(tmp_buffer, "0");
	else
		strncpy(tmp_buffer, val, strlen(val));

	for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
	{
		mBSSID[i].VlanPriority = atoi(tok);
	}

	if (new_driver == 2)
	{
		/* Radius Server */
		bzero(tmp_buffer, sizeof(char)*260);
		memset(val, 0 ,255);
		CFG_get_str(CFG_WLN_RADIUS_Server, val);

		if (strcmp(val, "") == 0)
		{
			sprintf(mBSSID[0].radius_server, "%s", "");
		}
		else
		{
			strncpy(tmp_buffer, val, strlen(val));

			for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
			{
				if (i < bssid_num)
					sprintf(mBSSID[i].radius_server, "%s", tok);
			}
		}

		/* Radius Port */
		bzero(tmp_buffer, sizeof(char)*260);
		memset(val, 0 ,255);
		CFG_get_str(CFG_WLN_RADIUS_Port, val);

		if (strcmp(val, "") == 0)
		{
			mBSSID[0].radius_port = 1812;
		}
		else
		{
			strncpy(tmp_buffer, val, strlen(val));

			for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
			{
				if (i < bssid_num)
					mBSSID[i].radius_port = atoi(tok);
			}
		}

		/* Radius Secret */
		for (i = 0; i < idx; i++)
		{
			if (i < bssid_num)
			{
				switch(i)
				{
					memset(val, 0 ,255);
					
					case 0:
						CFG_get_str(CFG_WLN_RADIUS_Key1, val);
						break;
					case 1:
						CFG_get_str(CFG_WLN_RADIUS_Key2, val);
						break;
					case 2:
						CFG_get_str(CFG_WLN_RADIUS_Key3, val);
						break;
					case 3:
						CFG_get_str(CFG_WLN_RADIUS_Key4, val);
						break;
					default:
						break;
				}

				if (strcmp(val, "") == 0)
					sprintf(mBSSID[i].radius_secret, "%s", "");
				else
					sprintf(mBSSID[i].radius_secret, "%s", val);
			}
		}
#if 0 
		bzero(mFT, sizeof(ft_tbl_s)*4);
		/* FtSupport */
		bzero(tmp_buffer, sizeof(char)*260);
		wordlist = nvram_get("FtSupport");
		if ((wordlist == NULL) || (strcmp(wordlist, "") == 0))
			strcpy(tmp_buffer, "0");
		else
			strncpy(tmp_buffer, wordlist, strlen(wordlist));

		for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
		{
			if (i < bssid_num)
			mFT[i].FtSupport = atoi(tok);
		}

		/* FtRic */
		bzero(tmp_buffer, sizeof(char)*260);
		wordlist = nvram_get("FtRic");
		if ((wordlist == NULL) || (strcmp(wordlist, "") == 0))
			strcpy(tmp_buffer, "0");
		else
			strncpy(tmp_buffer, wordlist, strlen(wordlist));

		for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
		{
			if (i < bssid_num)
				mFT[i].FtRic = atoi(tok);
		}

		/* FtOtd */
		bzero(tmp_buffer, sizeof(char)*260);
		wordlist = nvram_get("FtOtd");
		if ((wordlist == NULL) || (strcmp(wordlist, "") == 0))
			strcpy(tmp_buffer, "0");
		else
			strncpy(tmp_buffer, wordlist, strlen(wordlist));
	
		for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
		{
			if (i < bssid_num)
				mFT[i].FtOtd = atoi(tok);
		}

		/* FtMdId */
		for (i = 0; i < idx; i++)
		{
			if (i < bssid_num)
			{
				switch(i)
				{
					case 0:
						wordlist = nvram_get("FtMdId1");
						break;
					case 1:
						wordlist = nvram_get("FtMdId2");
						break;
					case 2:
						wordlist = nvram_get("FtMdId3");
						break;
					case 3:
						wordlist = nvram_get("FtMdId4");
						break;
					default:
						break;
				}
				if ((wordlist == NULL) || (strcmp(wordlist, "") == 0))
					strcpy(mFT[i].FtMdId, "");
				else
					sprintf(mFT[i].FtMdId, "%s", wordlist);
				fprintf(stderr, "FtMdId%d = %s\n", i, wordlist);
			}
		}

		/* FtR0khId */
		for (i = 0; i < idx; i++)
		{
			if (i < bssid_num)
			{
				switch(i)
				{
					case 0:
						wordlist = nvram_get("FtR0khId1");
						break;
					case 1:
						wordlist = nvram_get("FtR0khId2");
						break;
					case 2:
						wordlist = nvram_get("FtR0khId3");
						break;
					case 3:
						wordlist = nvram_get("FtR0khId4");
						break;
					default:
						break;
				}
				if ((wordlist == NULL) || (strcmp(wordlist, "") == 0))
					strcpy(mFT[i].FtR0khId, "");
				else
					sprintf(mFT[i].FtR0khId, "%s", wordlist);
			}
		}
	
		/* FtKeyLifeTime */
		bzero(tmp_buffer, sizeof(char)*260);
		wordlist = nvram_get("FtKeyLifeTime");
		if ((wordlist == NULL) || (strcmp(wordlist, "") == 0))
			strcpy(tmp_buffer, "3600");
		else
			strncpy(tmp_buffer, wordlist, strlen(wordlist));
	
		for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
		{
			if (i < bssid_num)
				mFT[i].FtKeyLifeTime = atoi(tok);
		}
	
		/* FtReassociateDealine */
		bzero(tmp_buffer, sizeof(char)*260);
		wordlist = nvram_get("FtReassociateDealine");
		if ((wordlist == NULL) || (strcmp(wordlist, "") == 0))
			strcpy(tmp_buffer, "100");
		else
			strncpy(tmp_buffer, wordlist, strlen(wordlist));
	
		for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
		{
			if (i < bssid_num)
				mFT[i].FtReassociateDealine = atoi(tok);
		}
#endif
		bzero(mWDS, sizeof(wds_tbl_s)*4);

		/* WDS Peer Link MAC Address */
		bzero(tmp_buffer, sizeof(char)*260);
		memset(val, 0 ,255);
		CFG_get_str(CFG_WLN_WdsList, val);

		idx = 0;
		if (!(strcmp(val, "") == 0))
		{
			strncpy(tmp_buffer, val, strlen(val));

			for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
			{
				sprintf(mWDS[i].PeerMac, "%s", tok);
				mWDS[i].enable = 1;
				idx++;
			}
		}

		/* WDS Peer Link Phy Mode */
		bzero(tmp_buffer, sizeof(char)*260);
		memset(val, 0 ,255);
		CFG_get_str(CFG_WLN_WdsPhyMode, val);

		if (!(strcmp(val, "") == 0))
		{
			strncpy(tmp_buffer, val, strlen(val));

			for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
				sprintf(mWDS[i].PhyMode, "%s", tok);
		}

		/* WDS Peer Link EncrypType */
		bzero(tmp_buffer, sizeof(char)*260);
		memset(val, 0 ,255);
		CFG_get_str(CFG_WLN_WdsEncrypType, val);

		if (!(strcmp(val, "") == 0))
		{
			strncpy(tmp_buffer, val, strlen(val));

			for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
				sprintf(mWDS[i].EncrypType, "%s", tok);
		}

		/* WDS Peer Link Default KeyID */
		bzero(tmp_buffer, sizeof(char)*260);
		memset(val, 0 ,255);
		CFG_get_str(CFG_WLN_WdsDefaultKeyID, val);

		if (!(strcmp(val, "") == 0))
		{
			strncpy(tmp_buffer, val, strlen(val));

			for (i = 0, tok = strtok(tmp_buffer,";"); tok; tok = strtok(NULL,";"), i++)
				mWDS[i].DefaultKeyID = atoi(tok);
		}

		/* WDS Peer Link Encryption Key */
		for (i = 0; i < idx; i++)
		{
			switch(i)
			{
				memset(val, 0 ,255);
				
				case 0:
					CFG_get_str(CFG_WLN_Wds1Key, val);
					break;
				case 1:
					CFG_get_str(CFG_WLN_Wds2Key, val);
					break;
				case 2:
					CFG_get_str(CFG_WLN_Wds3Key, val);
					break;
				case 3:
					CFG_get_str(CFG_WLN_Wds4Key, val);
					break;
				default:
					break;
			}

			if (strcmp(val, "") == 0)
				sprintf(mWDS[i].Key, "%s", "");
			else
				sprintf(mWDS[i].Key, "%s", val);
		}
	}

	if ((wake_up_dot1x == 1) && (start_uam == 0) && (wake_up_wapid == 0))
		return 2;
	else if ((wake_up_dot1x == 0) && (start_uam == 1) && (wake_up_wapid == 0))
		return 3;
	else if ((wake_up_dot1x == 0) && (start_uam == 0) && (wake_up_wapid == 1))
		return 5;
	else if ((wake_up_dot1x == 1) && (start_uam == 1) && (wake_up_wapid == 0))
		return 4;
	else if ((wake_up_wapid == 1) && (wake_up_dot1x == 1) && (start_uam == 0))
		return 6;
	else
		return 1;
}


void CGI_get_fversion(http_req *req)
{
char fver[5];
	int ret;
	
	ret= f_setting_get("FVERSION", fver);
	//ret= 0;
	if (ret== 0)
		WEB_printf( req, "%s", "");
	else
	{
		printf("%s\n", fver);
		WEB_printf( req, "%s", fver);
	}

	return;
}

void CGI_get_flogo(http_req *req)
{
char flogo[5];
	int ret;
	
	ret= f_setting_get("FLOGO", flogo);
	//ret= 0;
	if (ret== 0)
		WEB_printf( req, "%s", "");
	else
	{
		printf("%s\n", flogo);
		WEB_printf( req, "%s", flogo);
	}

	return;
}

void CGI_set_flangzhcn(http_req *req)
{
	char flang[5]="zhcn";
	f_setting_set("FLANG", flang);
	f_setting_save();
	return;
}

void CGI_set_flangen(http_req *req)
{
	char flang[5]="en";
	f_setting_set("FLANG", flang);
	f_setting_save();
	return;
}

void CGI_get_flang(http_req *req)
{
	char flang[5];
	int ret;
	
	ret= f_setting_get("FLANG", flang);
	//ret= 0;
	if (ret== 0)
		WEB_printf( req, "%s", "zhcn");
	else
	{
		printf("%s\n", flang);
		WEB_printf( req, "%s", flang);
	}

	return;
}

//gohuy
static int is_field_end(char *info, int p, int i)
{
	//if (((*(info+p+i+0)== 0x20)&&(*(info+p+i+1)== 0x20))||(*(info+p+i+0)== ','))
	if ((*(info+p+i+0)== ','))
		return 1;
	else
		return 0;
}

static int is_too_long(int para, int i)
{
	switch (para)
	{
	case 0:		// ch
		if (i> 10) return 1;
		break;
	case 1:		// SSID
		if (i> 33) return 1;
		break;
	case 2:		// BSSID
		if (i> 30) return 1;
		break;
	case 3:		// auth & ency
		if (i> 80) return 1;
		break;
	case 4:		// RSSI
		if (i> 8)	return 1;
		break;
	case 5:
	case 6:
	case 7:		// not used
		if (i> 60) return 1;
		break;
	case 8:		// network type
		if (i> 80) return 1;
		break;
	default:
		break;
	}
	return 0;
}

static void getfield(char *info, int *p, int len, int para, char *c)
	{
			int i;
			char tmp[100];
		
			i= 0;
			while ((*(info+*p+i)== 0x0a)||(*(info+*p+i)== 0x0d))
				(*p)++;
			
			while ((*(info+*p+i)!= 0x0a)&&(*(info+*p+i)!= 0x0d))
			{
				if (*(info+*p+i)!=',')
				{
					if(*(unsigned char *)(info+*p+i) == 0xfa)
					{
						*(info+*p+i) = ',';
					}
					*(c+i)= *(info+*p+i);
					i++;
					if (is_too_long(para, i)== 1)
						break;
				}
				else 
					break;
			}
			*(c+i)='\0';
			while ((*(info+*p+i)!=0x0a)&&(*(info+*p+i)!=0x0d))
			{
				if (*(info+*p+i)==',')
				{
					i++;
					if (i>= len)
						break;
				}
				else
					break;
			}
			if ((*(info+*p+i)==0x0a)||(*(info+*p+i)==0x0d))
				i++;
			(*p)+=i;
		}


static void getsecurity(char *c, char *strEncry, char *strAuth)
{
	int len, i, en, j;
	char tmp[10];

	len= strlen(c);
	if (strcmp(c, "NONE")== 0)
	{
		sprintf(strEncry, "NONE");
		sprintf(strAuth, "OPEN");
	}
	else if (strcmp(c, "WEP")== 0)
	{
		sprintf(strEncry, "WEP");
		sprintf(strAuth, "OPEN");
	}
	else if (strcmp(c, "WPAPSK/TKIP")== 0)
	{
		sprintf(strAuth, "WPAPSK");
		sprintf(strEncry, "TKIP");
	}
	else if (strcmp(c, "WPAPSK/AES")== 0)
	{
		sprintf(strAuth, "WPAPSK");
		sprintf(strEncry, "AES");
	}
	else if (strcmp(c, "WPAPSK/TKIPAES")== 0)
	{
		sprintf(strAuth, "WPAPSK");
		sprintf(strEncry, "TKIPAES");
	}
	else
	{
		sprintf(strAuth, "WPA2PSK");
		en= 0;
		j= 0;
		for (i= 0; i< len; i++)
		{
			if ((*(c+i)!= '/')&&(en== 0))
				continue;
			else if (*(c+i)== '/')
			{
				en=1;
				continue;
			}
			else if ((*(c+i)!= '\0')&&(en== 1))
			{
				tmp[j]= *(c+i);
				j++;
			}
		}
		if (strncmp(tmp, "TKIPAES", 7)== 0)
			sprintf(strEncry, "AES");		//sprintf(strEncry, "TKIPAES");
		else if (strncmp(tmp, "TKIP", 4)== 0)
			sprintf(strEncry, "TKIP");
		else if (strncmp(tmp, "AES", 3)== 0)
			sprintf(strEncry, "AES");
		else
			sprintf(strEncry, "AES");
	}
}

static int is_mac_add(char *ch)
{
	int i;

	i= strlen(ch);
	if (i!= 17)
		return 0;
	if (*(ch+2)!= ':')
		return 0;
	if (*(ch+5)!= ':')
		return 0;
	if (*(ch+8)!= ':')
		return 0;
	if (*(ch+11)!= ':')
		return 0;
	if (*(ch+14)!= ':')
		return 0;
	return 1;
}

static void trimup_info(char *info, int *p, int len, char *SSID, char *BSSID, int *nChannel, char *strEncry, char *strAuth, char *NetworkType, char *RSSI)
{
	int i, s, l;
	int para;
	char c[100];

	s= 0;
	para= 0;
	getfield(info, p, len, para, c);
	if (*p >= len)
		return ;
	para++;
	*nChannel= atoi(c);
	getfield(info, p, len, para, SSID);
	if (*p >= len)
		return ;
	para++;
	if (is_mac_add(SSID)== 1)
	{
		l= strlen(SSID);
		memcpy(BSSID, SSID, l+1);
		*SSID= '\0';
	}
	else
	{
		getfield(info, p, len, para, BSSID);
		if (*p >= len)
			return ;
	}
	para++;
	getfield(info, p, len, para, c);
	if (*p >= len)
		return ;
	getsecurity(c, strEncry, strAuth);
	para++;
	getfield(info, p, len, para, RSSI);
	if (*p >= len)
		return ;
	para++;
	getfield(info, p, len, para, c);			// w-mode, not used
	if (*p >= len)
		return ;
	para++;
	getfield(info, p, len, para, c);			// w-mode, not used
	if (*p >= len)
		return ;
	para++;
	getfield(info, p, len, para, c);			// w-mode, not used
	if (*p >= len)
		return ;
	if (strcmp(c, "In")== 0)
		sprintf(NetworkType, "%s", "Infrastructure");
	else
		sprintf(NetworkType, "%s", "Ad Hoc");
	para++;
	while((*(info+*p)!=0x0a)&&(*(info+*p)!=0x0d))
	{
		(*p)++;
	}
	(*p)++;
}

static void trimup_info_title(char *info, int *p, int len)
{
	int i;

	for (i= (*p); i< len; i++)
	{
		if ((*(info+i)== 0x0a)||(*(info+i)== 0x0d))
			break;
	}
	(*p)= i+1;
}

static void do_tmpBSSID(char *m1, char *m2)
{
	int len, i, j;

	len= strlen(m1);
	j= 0;
	for (i= 0; i< len; i++)
	{
		if (*(m1+i)!= ':')
		{
			*(m2+j)= *(m1+i);
			j++;
		}
	}
	*(m2+j)='\0';
}
#if 0
void getsurveylist(http_req *req)
{
	int sta;
	int savefd1, savefd2;

	int nChannel = 1;
	char strAuth[32], strEncry[32];
	char tmpRadio[272], tmpBSSIDII[16], tmpBSSID[35], tmpBSSIDI[35], tmpSSID[64], tmpRSSI[20], tmpChannel[16], tmpAuth[32], tmpEncry[52], tmpNetworkType[24], tmpImg[40];
	char radiocheck[8];
	char tmpSSIDII[35], NetworkType[24], RSSI[10];;
	int len, p;
extern cyg_mutex_t TempString_mutex;
extern char TempString[8192];

	cyg_mutex_lock (&TempString_mutex);
	send_cmd_iwpriv(3, "ra0", "set", "SiteSurvey=1");
	cyg_thread_delay(30);
	send_cmd_iwpriv(2, "ra0", "GetSiteSurvey", NULL);

	len= strlen(TempString);
	p= 0;
	trimup_info_title(TempString, &p, len);		// for title
	trimup_info_title(TempString, &p, len);		// for title
	while(p< len)
	{
		trimup_info(TempString, &p, len, tmpSSIDII, tmpBSSID, &nChannel, strEncry, strAuth, NetworkType, RSSI);
		if (p >= len)
			break;
		sprintf((char *)tmpSSID, "<td>%s</td>",  tmpSSIDII);
		do_tmpBSSID(tmpBSSID, tmpBSSIDII);
		sprintf(tmpBSSIDI, "<td>%s</td>", tmpBSSID);
		sprintf((char *)tmpRSSI,"<td>%s%%</td>", RSSI);
		sprintf((char *)tmpChannel, "<td>%u</td>", nChannel);
		sprintf((char *)tmpAuth, "<td>%s</td>", strAuth);
		sprintf((char *)tmpEncry, "<td>%s</td>", strEncry);
		sprintf((char *)tmpNetworkType, "<td>%s</td>", NetworkType);
		strcpy(radiocheck, "");
		sprintf((char *)tmpRadio, "<td><input type=radio name=selectedSSID %s value=\"%s\" onClick=\"selectedSSIDChange(this.value,'%s','%s',%d,'%s','%s')\"></td>", radiocheck, tmpSSIDII, tmpBSSIDII, NetworkType, nChannel, strEncry, strAuth);
		WEB_printf(req, "<tr> %s %s %s %s %s %s %s %s </tr>\n", tmpRadio, tmpSSID, tmpBSSIDI, tmpRSSI, tmpChannel, tmpEncry, tmpAuth, tmpNetworkType);
	}
	
	cyg_mutex_unlock (&TempString_mutex);
	return ;
}
#endif
static void wscan_trimup_info(char *info, char *rep)
	{
					int len, i, cn;
					int par;
					int w_len;
					
					w_len =0;
					//m2m_cmd_reply(info);
					len= strlen(info);
					if (len > 1024*8)
						len= 1024*8; 
	#if 0
					m2m_printf("\r\n*******info:%d*******\r\n", len);
					for(i=0; i< len;i++)
					{
						m2m_printf("%c", info[i]);
					}
					m2m_printf("\r\n*******end*******\r\n", len);
	#endif
					cn= 0;
					par= -1;
					i= 0;
					while(i< len)
					{
						if (*(info+i)== ' ')
						{
							if (par== -1)
							{
								//i++;
								while (*(info+i)== ' ')
									i++;
								i--;
								par++;
							}
							else if (par== 1)
							{
								//i++;
								if(w_len == 32)
								{
									
									rep[cn]= ',';
									cn++;
									par++;
								}
								else
								{
									
									if(left_is_blank((info+i+1), 32 -(w_len)-1) == 0)
									{
										
										while (*(info+i)== ' ')
										{
											i++;
										}
										i--;
										rep[cn]= ',';
										cn++;
										par++;
									}
									else
									{
										
										while (*(info+i)== ' ')
										{
											i++;
											w_len++;
											rep[cn]= ' ';
											cn++;
										}
										i--;
									}	
									
								}
								
							}
							else //if ((par!= 1)&&(par!= -1))
							{
								//i++;
								while (*(info+i)== ' ')
								{
									i++;
								}	
								i--;
								rep[cn]= ',';
								cn++;
								par++;
							}
						}
						else if ((*(info+i)== 0x0a)||(*(info+i)== 0x0d))
						{
							rep[cn]= 0x0a;
							cn++;
							rep[cn]= 0x0d;
							cn++;
							par= -1;
						}
						else
						{
							if(*(info+i)== ',')
							{
								*(unsigned char *)(rep+cn) = 0xfa;
							}
							else
							{
								rep[cn]= *(info+i);
							}
							
							cn++;
							if (par== -1)
							{
								par= 0;
								w_len = 0;
							}
							if(par== 1)
							{
								w_len++;
							}
						}
						i++;
					}
					rep[cn]= '\0';
				}


void getsurveylist(http_req *req)
{
	int l, p, len;
	char strAuth[32], strEncry[32];
	char tmpRadio[272], tmpBSSIDII[16], tmpBSSID[35], tmpBSSIDI[35], tmpSSID[64], tmpRSSI[20], tmpChannel[16], tmpAuth[32], tmpEncry[52], tmpNetworkType[24], tmpImg[40];
	char radiocheck[8];
	char tmpSSIDII[35], NetworkType[24], RSSI[10];;
	int nChannel = 1;
	char *rep;
extern cyg_mutex_t TempString_mutex;
extern char TempString[];

	rep= (char *)malloc(1024*8);
	cyg_mutex_lock (&TempString_mutex);
	send_cmd_iwpriv(3, "ra0", "set", "SiteSurvey=1");
	cyg_thread_delay(30);
	send_cmd_iwpriv(2, "ra0", "GetSiteSurvey", NULL);

	//l= strlen(TempString);
	printf("get_sta_channel: TempString= \n%s\n", TempString);
	wscan_trimup_info(TempString, rep);
	cyg_mutex_unlock (&TempString_mutex);

	len= strlen(rep);
	//m2m_cmd_reply(rep);
	p= 0;
	while(p< len)
	{
		trimup_info(rep, &p, len, tmpSSIDII, tmpBSSID, &nChannel, strEncry, strAuth, NetworkType, RSSI);
		if (strcmp(tmpSSIDII, "SSID")== 0)
			continue;
		if (p >= len)
			break;
		sprintf((char *)tmpSSID, "<td>%s</td>",  tmpSSIDII);
		do_tmpBSSID(tmpBSSID, tmpBSSIDII);
		sprintf(tmpBSSIDI, "<td>%s</td>", tmpBSSID);
		sprintf((char *)tmpRSSI,"<td>%s%%</td>", RSSI);
		sprintf((char *)tmpChannel, "<td>%u</td>", nChannel);
		sprintf((char *)tmpAuth, "<td>%s</td>", strAuth);
		sprintf((char *)tmpEncry, "<td>%s</td>", strEncry);
		sprintf((char *)tmpNetworkType, "<td>%s</td>", NetworkType);
		strcpy(radiocheck, "");
		sprintf((char *)tmpRadio, "<td><input type=radio name=selectedSSID %s value=\"%s\" onClick=\"selectedSSIDChange(this.value,'%s','%s',%d,'%s','%s')\"></td>", radiocheck, tmpSSIDII, tmpBSSIDII, NetworkType, nChannel, strEncry, strAuth);
		WEB_printf(req, "<tr> %s %s %s %s %s %s %s %s </tr>\n", tmpRadio, tmpSSID, tmpBSSIDI, tmpRSSI, tmpChannel, tmpEncry, tmpAuth, tmpNetworkType);
	}
	free(rep);
	return ;
}
///gohuy

void CGI_get_version(http_req *req)
{
	extern char M2M_VER[];
	extern char M2M_LVER[];

	WEB_printf( req, "%s.%s", M2M_VER,M2M_LVER);//menghui modify

	//WEB_printf( req, "%s", M2M_VER);

	return;
}

void CGI_get_wlan_mac_address(http_req *req)
{
	char if_hw[18] = {0};
	struct ifreq ifr;
	char *ptr;
	int skfd;

	memset(if_hw, 0, 18);

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* hardware address */
	strncpy(ifr.ifr_name, "ra0", IF_NAMESIZE);
	if(ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) 
	{
		perror("SIOCGIFHWADDR");
		return;
	}

	ptr = (char *)&ifr.ifr_addr.sa_data;
	sprintf(if_hw, "%02X:%02X:%02X:%02X:%02X:%02X",
			(ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
			(ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377));

	(void) close(skfd);
	
	WEB_printf( req, "%s", if_hw);

	return;
}

void CGI_get_wlan_11a_channel_list(http_req *req)
{
	int  idx = 0;	
	char val[255];     
	
	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_CountryCode, val);

	if ((val == "") || (strcmp(val, "") == 0) || (strcmp(val, "US") == 0) || (strcmp(val, "FR") == 0) ||
		(strcmp(val, "IE") == 0) || (strcmp(val, "JP") == 0) || (strcmp(val, "HK") == 0))
	{
		for (idx = 0; idx < 4; idx++)
		{
			if (idx == 0)
				WEB_printf(req, "<option value=%d>%dMHz (Channel %d)</option>\n", 36 + 4*idx, 5180 + 20*idx, 36 + 4*idx);
			else
				WEB_printf(req, "<option value=%d>%dMHz (Channel %d)</option>\n", 36 + 4*idx, 5180 + 20*idx, 36 + 4*idx);
		}
	}

	if ((val == "") || (strcmp(val, "") == 0) || (strcmp(val, "US") == 0) || (strcmp(val, "FR") == 0) ||
		(strcmp(val, "IE") == 0) || (strcmp(val, "TW") == 0) || (strcmp(val, "HK") == 0))
	{
		for (idx = 4; idx < 8; idx++)
			WEB_printf(req, "<option value=%d>%dMHz (Channel %d)</option>\n", 36 + 4*idx, 5180 + 20*idx, 36 + 4*idx);
    }

	if ((val == "") || (strcmp(val, "") == 0))
	{
		for (idx = 16; idx < 27; idx++)
			WEB_printf(req, "<option value=%d>%dMHz (Channel %d)</option>\n", 36 + 4*idx, 5180 + 20*idx, 36 + 4*idx);
	}

	if ((val == "") || (strcmp(val, "") == 0) || (strcmp(val, "US") == 0) || (strcmp(val, "TW") == 0) ||
        (strcmp(val, "CN") == 0) || (strcmp(val, "HK") == 0))
	{
		for (idx = 28; idx < 32; idx++)
			WEB_printf(req, "<option value=%d>%dMHz (Channel %d)</option>\n", 36 + 4*idx + 1, 5180 + 20*idx + 5, 36 + 4*idx + 1);
	}

	if ((val == "") || (strcmp(val, "") == 0) || (strcmp(val, "US") == 0) ||
		(strcmp(val, "CN") == 0) || (strcmp(val, "HK") == 0))
	{
		WEB_printf(req, "<option value=165>5825MHz (Channel 165)</option>\n");
	}

	return;
}

void CGI_get_wlan_11b_channel_list(http_req *req)
{
	int  idx = 0;	
	char val[255]; 
	
	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_CountryCode, val);

	if ((val == "") || (strcmp(val, "") == 0) || (strcmp(val, "US") == 0) || (strcmp(val, "JP") == 0) || (strcmp(val, "FR") == 0) ||
		(strcmp(val, "IE") == 0) || (strcmp(val, "TW") == 0) || (strcmp(val, "CN") == 0) || (strcmp(val, "HK") == 0))
	{
		for (idx = 0; idx < 11; idx++)
		{
			if (idx == 5)
				WEB_printf(req, "<option value=%d selected>%dMHz (Channel %d)</option>\n", idx + 1, 2412 + 5*idx, idx+1);
			else
				WEB_printf(req, "<option value=%d>%dMHz (Channel %d)</option>\n", idx + 1, 2412 + 5*idx, idx+1);
		}
	}

	if ((val == "") || (strcmp(val, "") == 0) || (strcmp(val, "JP") == 0) || (strcmp(val, "TW") == 0) || (strcmp(val, "FR") == 0) ||
		(strcmp(val, "IE") == 0) || (strcmp(val, "CN") == 0) || (strcmp(val, "HK") == 0))
	{
		for (idx = 11; idx < 13; idx++)
			WEB_printf(req, "<option value=%d>%dMHz (Channel %d)</option>\n", idx + 1, 2412 + 5*idx, idx+1);
	}

	if ((val == "") || (strcmp(val, "") == 0) || (strcmp(val, "JP") == 0))
	{
		WEB_printf(req, "<option value=14>2484MHz (Channel 14)</option>\n");
	}

	return;
}

void CGI_get_wlan_11g_channel_list(http_req *req)
{
	int  idx = 0;	
	char val[255];    
	
	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_CountryCode, val);

	if ((val == "") || (strcmp(val, "") == 0) || (strcmp(val, "US") == 0) || (strcmp(val, "JP") == 0) || (strcmp(val, "FR") == 0) ||
		(strcmp(val, "IE") == 0) || (strcmp(val, "TW") == 0) || (strcmp(val, "CN") == 0) || (strcmp(val, "HK") == 0))
	{
		for (idx = 0; idx < 11; idx++)
		{
			if (idx == 5)
				WEB_printf(req, "<option value=%d selected>%dMHz (Channel %d)</option>\n", idx + 1, 2412 + 5*idx, idx+1);
			else
				WEB_printf(req, "<option value=%d>%dMHz (Channel %d)</option>\n", idx + 1, 2412 + 5*idx, idx+1);
		}
	}

	if ((val == "") || (strcmp(val, "") == 0) || (strcmp(val, "JP") == 0) || (strcmp(val, "TW") == 0) || (strcmp(val, "FR") == 0) ||
		(strcmp(val, "IE") == 0) || (strcmp(val, "CN") == 0) || (strcmp(val, "HK") == 0))
	{
			for (idx = 11; idx < 13; idx++)
				WEB_printf(req, "<option value=%d>%dMHz (Channel %d)</option>\n", idx + 1, 2412 + 5*idx, idx+1);
	}

	if ((val == "") || (strcmp(val, "") == 0))
	{
		WEB_printf(req, "<option value=14>2484MHz (Channel 14)</option>\n");
	}

	return;
}

void CGI_get_wlan_channel(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_AutoChannelSelect, val);

	if (val == "")
		WEB_printf(req, "%s", "9");
	else
	{
		if (strcmp(val, "1") == 0)
			WEB_printf(req, "%s", "0");
		else
		{
			CFG_get_str(CFG_WLN_Channel, val);
			
			if (val == "")
				WEB_printf(req, "%s", "9");
			else
				WEB_printf(req, "%s", val);
		}
	}

	return;
}

void CGI_get_wlan_wds_default_key(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	int wdsmode = 0;

	CFG_get_str(CFG_WLN_WdsEnable, val);
	
	wdsmode = atoi(val);

	memset(val, 0, 255);

	CFG_get_str(CFG_WLN_WdsDefaultKeyID, val);

	if (g_new_version == 2)
	{
		if ((val == NULL) || (strcmp(val, "") == 0))
			WEB_printf(req, "%s", "");
		else
		{
			if (wdsmode == 4)
			{
				wdsmode = atoi(val);
				WEB_printf(req, "%d", wdsmode);
			}
			else
				WEB_printf(req, "%s", val);
		}
	}
	else
	{
		WEB_printf(req, "%s", val);
	}

	return;
}

void CGI_get_wlan_wds_encryp_key(http_req *req)
{
	char val[255];  
	char tmp_buffer[260];
	int wdsmode = 0;

	val[0] = '\0';
	
	if (g_new_version == 2)
	{
		bzero(tmp_buffer, sizeof(char)*260);

		CFG_get_str(CFG_WLN_WdsEnable, val);
		wdsmode = atoi(val);

		if (wdsmode == 0)
		{
			WEB_printf(req, "%s", "");
			return;
		}
		else if (wdsmode == 4)
		{
			CFG_get_str(CFG_WLN_Wds1Key, val);

			if ((val == "") || (strcmp(val, "") == 0))
				WEB_printf(req, "%s", "");
			else
				WEB_printf(req, "%s", val);

			return;
		}

		CFG_get_str(CFG_WLN_Wds1Key, val);
		if ((val == "") || (strcmp(val, "") == 0))
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", 0);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
		else
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%s", val);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}

		CFG_get_str(CFG_WLN_Wds2Key, val);
		if ((val == "") || (strcmp(val, "") == 0))
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", 0);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
		else
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%s", val);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}

		CFG_get_str(CFG_WLN_Wds3Key, val);
		if ((val == "") || (strcmp(val, "") == 0))
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", 0);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
		else
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%s", val);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}

		CFG_get_str(CFG_WLN_Wds4Key, val);
		if ((val == "") || (strcmp(val, "") == 0))
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", 0);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
		else
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%s", val);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}

		tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
		WEB_printf(req, "%s", tmp_buffer);
	}
	else
	{
		CFG_get_str(CFG_WLN_WdsKey, val);
	}

	if (val == "")
	   WEB_printf(req, "%s", "");
	else
	   WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_all_bss_tkip_or_wep(http_req *req)
{
	char val[255];  
	char *tmpp = NULL;

	val[0] = '\0';

	CFG_get_str(CFG_WLN_EncrypType, val);

	if (val == "")
	   	WEB_printf(req, "%s", "0");
	else
	{
		tmpp = strstr(val, "TKIP");
		if (tmpp == NULL)
			tmpp = strstr(val, "WEP");

		if (tmpp == NULL)
			WEB_printf(req, "%s", "0");
		else
			WEB_printf(req, "%s", "1");
	}

	return;
}

/* save_level 0: only change SSID and HideSSID 1: SSID Number change or Security change */
int save_mbss_value_to_flash(int save_level)
{
	int  idx, s, new_driver;
	char tmp_buffer[260];
	unsigned char version_number[8] = {0};

	new_driver = 0;

	if ( OidQueryInformation(RT_OID_QUERY_DRIVER_VERSION, "ra0", &version_number, sizeof(unsigned char)*8) < 0 )
	{
		if ( OidQueryInformation(RT_OID_VERSION_INFO,"ra0", &version_number, sizeof(unsigned char)*8) < 0 )
		{
			new_driver = 0;
		}
	}

	new_driver = 2;
	CFG_set(CFG_WLN_NewVersion, "2");
	
	g_new_version = new_driver;

	/* SSID */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			if (new_driver == 0)
			{
				sprintf(tmp_buffer+strlen(tmp_buffer),"%s", mBSSID[idx].SSID);
				sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
			}
			else
			{
				switch(idx)
				{
					case 0:
						CFG_set(CFG_WLN_SSID1, mBSSID[idx].SSID);
						break;
					case 1:
						CFG_set(CFG_WLN_SSID2, mBSSID[idx].SSID);
						break;
					case 2:
						CFG_set(CFG_WLN_SSID3, mBSSID[idx].SSID);
						break;
					case 3:
						CFG_set(CFG_WLN_SSID4, mBSSID[idx].SSID);
						break;
					default:
						break;
				}
			}
		}
	}

	if (new_driver == 0)
	{
		tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
		CFG_set(CFG_WLN_SSID, tmp_buffer);
		CFG_set(CFG_WLN_SSID1, "");
		CFG_set(CFG_WLN_SSID2, "");
		CFG_set(CFG_WLN_SSID3, "");
		CFG_set(CFG_WLN_SSID4, "");
	}

	/* TxRate */
/*
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].TxRate);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	nvram_bufset("TxRate", tmp_buffer);
*/

	/* HideSSID */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].HideSSID);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_HideSSID, tmp_buffer);

	/* VlanName */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%s", mBSSID[idx].VlanName);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_VLANName, tmp_buffer);

	/* Vlan ID */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].VlanId);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_VLANID, tmp_buffer);

	/* Vlan Priority */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].VlanPriority);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_VLANPriority, tmp_buffer);
	
	if (save_level == 0)
	{
		CFG_commit(0);
		return 1;
	}

	/* SecurityMode */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].SecurityMode);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_SecurityMode, tmp_buffer);

	/* AuthMode */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%s", mBSSID[idx].AuthMode);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_AuthMode, tmp_buffer);

	/* EncrypType */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%s", mBSSID[idx].EncrypType);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_EncrypType, tmp_buffer);

	/* WPAPSK */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			if (new_driver == 0)
			{
				if (strcmp(mBSSID[idx].WPAPSK, "") == 0)
					sprintf(tmp_buffer+strlen(tmp_buffer),"%s", "0");
				else
					sprintf(tmp_buffer+strlen(tmp_buffer),"%s", mBSSID[idx].WPAPSK);
				sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
			}
			else
			{
				switch(idx)
				{
					case 0:
						if (strcmp(mBSSID[idx].WPAPSK, "") == 0)
							CFG_set(CFG_WLN_WPAPSK1, "0");
						else
							CFG_set(CFG_WLN_WPAPSK1, mBSSID[idx].WPAPSK);
						break;
					case 1:
						if (strcmp(mBSSID[idx].WPAPSK, "") == 0)
							CFG_set(CFG_WLN_WPAPSK2, "0");
						else
							CFG_set(CFG_WLN_WPAPSK2, mBSSID[idx].WPAPSK);
						break;
					case 2:
						if (strcmp(mBSSID[idx].WPAPSK, "") == 0)
							CFG_set(CFG_WLN_WPAPSK3, "0");
						else
							CFG_set(CFG_WLN_WPAPSK3, mBSSID[idx].WPAPSK);
						break;
					case 3:
						if (strcmp(mBSSID[idx].WPAPSK, "") == 0)
							CFG_set(CFG_WLN_WPAPSK4, "0");
						else
							CFG_set(CFG_WLN_WPAPSK4, mBSSID[idx].WPAPSK);
						break;
					default:
						break;
				}
			}
		}
	}

	if (new_driver == 0)
	{
		tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
		CFG_set(CFG_WLN_WPAPSK, tmp_buffer);
		CFG_set(CFG_WLN_WPAPSK1, "");
		CFG_set(CFG_WLN_WPAPSK2, "");
		CFG_set(CFG_WLN_WPAPSK3, "");
		CFG_set(CFG_WLN_WPAPSK4, "");
	}

	/* DefaultKeyID */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].DefaultKeyID);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_DefaultKeyID, tmp_buffer);

	/* Key1Type */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].Key1Type);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_Key1Type, tmp_buffer);

	/* Key1Str */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			if (new_driver == 0)
			{
				if (strcmp(mBSSID[idx].Key1Str, "") == 0)
					sprintf(tmp_buffer+strlen(tmp_buffer),"%s", "0");
				else
					sprintf(tmp_buffer+strlen(tmp_buffer),"%s", mBSSID[idx].Key1Str);
				sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
			}
			else
			{
				switch(idx)
				{
					case 0:
						if (strcmp(mBSSID[idx].Key1Str, "") == 0)
							CFG_set(CFG_WLN_Key1Str1, "0");
						else
							CFG_set(CFG_WLN_Key1Str1, mBSSID[idx].Key1Str);
						break;
					case 1:
						if (strcmp(mBSSID[idx].Key1Str, "") == 0)
							CFG_set(CFG_WLN_Key1Str2, "0");
						else
							CFG_set(CFG_WLN_Key1Str2, mBSSID[idx].Key1Str);
						break;
					case 2:
						if (strcmp(mBSSID[idx].Key1Str, "") == 0)
							CFG_set(CFG_WLN_Key1Str3, "0");
						else
							CFG_set(CFG_WLN_Key1Str3, mBSSID[idx].Key1Str);
						break;
					case 3:
						if (strcmp(mBSSID[idx].Key1Str, "") == 0)
							CFG_set(CFG_WLN_Key1Str4, "0");
						else
							CFG_set(CFG_WLN_Key1Str4, mBSSID[idx].Key1Str);
						break;
					default:
						break;
				}
			}
		}
	}

	if (new_driver == 0)
	{
		tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
		CFG_set(CFG_WLN_Key1Str, tmp_buffer);
		CFG_set(CFG_WLN_Key1Str1, "");
		CFG_set(CFG_WLN_Key1Str2, "");
		CFG_set(CFG_WLN_Key1Str3, "");
		CFG_set(CFG_WLN_Key1Str4, "");
	}

	/* Key2Type */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].Key2Type);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_Key2Type, tmp_buffer);

	/* Key2Str */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			if (new_driver == 0)
			{
				if (strcmp(mBSSID[idx].Key2Str, "") == 0)
					sprintf(tmp_buffer+strlen(tmp_buffer),"%s", "0");
				else
					sprintf(tmp_buffer+strlen(tmp_buffer),"%s", mBSSID[idx].Key2Str);
				sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
			}
			else
			{
				switch(idx)
				{
					case 0:
						if (strcmp(mBSSID[idx].Key2Str, "") == 0)
							CFG_set(CFG_WLN_Key2Str1, "0");
						else
							CFG_set(CFG_WLN_Key2Str1, mBSSID[idx].Key2Str);
						break;
					case 1:
						if (strcmp(mBSSID[idx].Key2Str, "") == 0)
							CFG_set(CFG_WLN_Key2Str2, "0");
						else
							CFG_set(CFG_WLN_Key2Str2, mBSSID[idx].Key2Str);
						break;
					case 2:
						if (strcmp(mBSSID[idx].Key2Str, "") == 0)
							CFG_set(CFG_WLN_Key2Str3, "0");
						else
							CFG_set(CFG_WLN_Key2Str3, mBSSID[idx].Key2Str);
						break;
					case 3:
						if (strcmp(mBSSID[idx].Key2Str, "") == 0)
							CFG_set(CFG_WLN_Key2Str4, "0");
						else
							CFG_set(CFG_WLN_Key2Str4, mBSSID[idx].Key2Str);
						break;
					default:
						break;
				}
			}
		}
	}

	if (new_driver == 0)
	{
		tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
		CFG_set(CFG_WLN_Key2Str, tmp_buffer);
		CFG_set(CFG_WLN_Key2Str1, "");
		CFG_set(CFG_WLN_Key2Str2, "");
		CFG_set(CFG_WLN_Key2Str3, "");
		CFG_set(CFG_WLN_Key2Str4, "");
	}

	/* Key3Type */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].Key3Type);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_Key3Type, tmp_buffer);

	/* Key3Str */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			if (new_driver == 0)
			{
				if (strcmp(mBSSID[idx].Key3Str, "") == 0)
					sprintf(tmp_buffer+strlen(tmp_buffer),"%s", "0");
				else
					sprintf(tmp_buffer+strlen(tmp_buffer),"%s", mBSSID[idx].Key3Str);
				sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
			}
			else
			{
				switch(idx)
				{
					case 0:
						if (strcmp(mBSSID[idx].Key3Str, "") == 0)
							CFG_set(CFG_WLN_Key3Str1, "0");
						else
							CFG_set(CFG_WLN_Key3Str1, mBSSID[idx].Key3Str);
						break;
					case 1:
						if (strcmp(mBSSID[idx].Key3Str, "") == 0)
							CFG_set(CFG_WLN_Key3Str2, "0");
						else
							CFG_set(CFG_WLN_Key3Str2, mBSSID[idx].Key3Str);
						break;
					case 2:
						if (strcmp(mBSSID[idx].Key3Str, "") == 0)
							CFG_set(CFG_WLN_Key3Str3, "0");
						else
							CFG_set(CFG_WLN_Key3Str3, mBSSID[idx].Key3Str);
						break;
					case 3:
						if (strcmp(mBSSID[idx].Key3Str, "") == 0)
							CFG_set(CFG_WLN_Key3Str4, "0");
						else
							CFG_set(CFG_WLN_Key3Str4, mBSSID[idx].Key3Str);
						break;
					default:
						break;
				}
			}
		}
	}

	if (new_driver == 0)
	{
		tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
		CFG_set(CFG_WLN_Key3Str, tmp_buffer);
		CFG_set(CFG_WLN_Key3Str1, "");
		CFG_set(CFG_WLN_Key3Str2, "");
		CFG_set(CFG_WLN_Key3Str3, "");
		CFG_set(CFG_WLN_Key3Str4, "");
	}

	/* Key4Type */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].Key4Type);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_Key4Type, tmp_buffer);

	/* Key4Str */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			if (new_driver == 0)
			{
				if (strcmp(mBSSID[idx].Key4Str, "") == 0)
					sprintf(tmp_buffer+strlen(tmp_buffer),"%s", "0");
				else
					sprintf(tmp_buffer+strlen(tmp_buffer),"%s", mBSSID[idx].Key4Str);
				sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
			}
			else
			{
				switch(idx)
				{
					case 0:
						if (strcmp(mBSSID[idx].Key4Str, "") == 0)
							CFG_set(CFG_WLN_Key4Str1, "0");
						else
							CFG_set(CFG_WLN_Key4Str1, mBSSID[idx].Key4Str);
						break;
					case 1:
						if (strcmp(mBSSID[idx].Key4Str, "") == 0)
							CFG_set(CFG_WLN_Key4Str2, "0");
						else
							CFG_set(CFG_WLN_Key4Str2, mBSSID[idx].Key4Str);
						break;
					case 2:
						if (strcmp(mBSSID[idx].Key4Str, "") == 0)
							CFG_set(CFG_WLN_Key4Str3, "0");
						else
							CFG_set(CFG_WLN_Key4Str3, mBSSID[idx].Key4Str);
						break;
					case 3:
						if (strcmp(mBSSID[idx].Key4Str, "") == 0)
							CFG_set(CFG_WLN_Key4Str4, "0");
						else
							CFG_set(CFG_WLN_Key4Str4, mBSSID[idx].Key4Str);
						break;
					default:
						break;
				}
			}
		}
	}

	if (new_driver == 0)
	{
		tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
		CFG_set(CFG_WLN_Key4Str, tmp_buffer);
		CFG_set(CFG_WLN_Key4Str1, "");
		CFG_set(CFG_WLN_Key4Str2, "");
		CFG_set(CFG_WLN_Key4Str3, "");
		CFG_set(CFG_WLN_Key4Str4, "");
	}

	/* WAPI Key Type */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].WapiKeyType);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_WapiPskType, tmp_buffer);

	/* WAPI Key */
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			switch(idx)
			{
				case 0:
					if (strcmp(mBSSID[idx].WapiKey, "") == 0)
						CFG_set(CFG_WLN_WapiPsk1, "0");
					else
						CFG_set(CFG_WLN_WapiPsk1, mBSSID[idx].WapiKey);
					break;
				case 1:
					if (strcmp(mBSSID[idx].WapiKey, "") == 0)
						CFG_set(CFG_WLN_WapiPsk2, "0");
					else
						CFG_set(CFG_WLN_WapiPsk2, mBSSID[idx].WapiKey);
					break;
				case 2:
					if (strcmp(mBSSID[idx].WapiKey, "") == 0)
						CFG_set(CFG_WLN_WapiPsk3, "0");
					else
						CFG_set(CFG_WLN_WapiPsk3, mBSSID[idx].WapiKey);
					break;
				case 3:
					if (strcmp(mBSSID[idx].WapiKey, "") == 0)
						CFG_set(CFG_WLN_WapiPsk4, "0");
					else
						CFG_set(CFG_WLN_WapiPsk4, mBSSID[idx].WapiKey);
					break;
				default:
					break;
			}
		}
	}

	/* IEEE8021X */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].IEEE8021X);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_IEEE8021X, tmp_buffer);

	/* PreAuth */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].PreAuth);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_PreAuth, tmp_buffer);

	/* PMKCachePeriod */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].PMKCachePeriod);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_PMKCachePeriod, tmp_buffer);

	/* RekeyMethod */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			if (strcmp(mBSSID[idx].RekeyMethod, "") == 0)
				sprintf(tmp_buffer+strlen(tmp_buffer),"%s", "DISABLE");
			else
				sprintf(tmp_buffer+strlen(tmp_buffer),"%s", mBSSID[idx].RekeyMethod);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_RekeyMethod, tmp_buffer);

	/* RekeyInterval */
	bzero(tmp_buffer, sizeof(char)*260);
	for (idx = 0; idx < 4; idx++)
	{
		if (mBSSID[idx].enable == 1)
		{
			sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].RekeyInterval);
			sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
		}
	}
	tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
	CFG_set(CFG_WLN_RekeyInterval, tmp_buffer);

	if (new_driver == 2)
	{
		/* Radius Server */
		bzero(tmp_buffer, sizeof(char)*260);
		for (idx = 0; idx < 4; idx++)
		{
			if (mBSSID[idx].enable == 1)
			{
				if (strcmp(mBSSID[idx].radius_server, "") == 0)
					sprintf(tmp_buffer+strlen(tmp_buffer),"%s", "0");
				else
					sprintf(tmp_buffer+strlen(tmp_buffer),"%s", mBSSID[idx].radius_server);
				sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
			}
		}
		tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
		CFG_set(CFG_WLN_RADIUS_Server, tmp_buffer);

		/* Radius Port */
		bzero(tmp_buffer, sizeof(char)*260);
		for (idx = 0; idx < 4; idx++)
		{
			if (mBSSID[idx].enable == 1)
			{
				sprintf(tmp_buffer+strlen(tmp_buffer),"%d", mBSSID[idx].radius_port);
				sprintf(tmp_buffer+strlen(tmp_buffer),"%c", ';');
			}
		}
		tmp_buffer[strlen(tmp_buffer) - 1] = '\0';
		CFG_set(CFG_WLN_RADIUS_Port, tmp_buffer);

		/* Radius Secret */
		bzero(tmp_buffer, sizeof(char)*260);
		for (idx = 0; idx < 4; idx++)
		{
			if (mBSSID[idx].enable == 1)
			{
				switch(idx)
				{
					case 0:
						if (strcmp(mBSSID[idx].radius_secret, "") == 0)
							CFG_set(CFG_WLN_RADIUS_Key1, "");
						else
							CFG_set(CFG_WLN_RADIUS_Key1, mBSSID[idx].radius_secret);
						break;
					case 1:
						if (strcmp(mBSSID[idx].radius_secret, "") == 0)
							CFG_set(CFG_WLN_RADIUS_Key2, "");
						else
							CFG_set(CFG_WLN_RADIUS_Key2, mBSSID[idx].radius_secret);
						break;
					case 2:
						if (strcmp(mBSSID[idx].radius_secret, "") == 0)
							CFG_set(CFG_WLN_RADIUS_Key3, "");
						else
							CFG_set(CFG_WLN_RADIUS_Key3, mBSSID[idx].radius_secret);
						break;
					case 3:
						if (strcmp(mBSSID[idx].radius_secret, "") == 0)
							CFG_set(CFG_WLN_RADIUS_Key4, "");
						else
							CFG_set(CFG_WLN_RADIUS_Key4, mBSSID[idx].radius_secret);
						break;
					default:
						break;
				}
			}
		}
	}

	CFG_commit(0);

	return 1;
}

void CGI_get_wlan_wsc_config_state(http_req *req)
{
	char val[255];  
	int s, temp_config;
	struct iwreq iwr;
	struct _WSC_CONFIGURED_VALUE wsc_value;
	struct eth_drv_sc 		*sc;
	cyg_netdevtab_entry_t 	*t;
	int	found = 0;

	val[0] = '\0';

#ifdef CONFIG_WIRELESS
	memset(&wsc_value, 0, sizeof(wsc_value));
	memset(&iwr, 0, sizeof(iwr));
	//strncpy(iwr.ifr_name, "ra0", IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) &wsc_value;

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		
		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
	       }
	}

	if (found == 1)	
	{
		if(rt28xx_ap_ioctl(sc, RTPRIV_IOCTL_WSC_PROFILE, (caddr_t)&iwr) < 0)
		{
			diag_printf("Query RTPRIV_IOCTL_WSC_PROFILE has error.\n");
			WEB_printf(req, "%s", "1");
			return;
		}
	}	
	else
	{
		diag_printf("Can't find ra0.\n");
		WEB_printf(req, "%s", "1");
		return;
	}
#else
	diag_printf("Wireless IF does not init.\n");
	return;
#endif	

	CFG_get_str(CFG_WLN_WscConfigured, val);
	
	temp_config = atoi(val);

	if (wsc_value.WscConfigured == 1)
		WEB_printf(req, "%s", "1");
	else if (wsc_value.WscConfigured == 2)
		WEB_printf(req, "%s", "2");
	else
		WEB_printf(req, "%s", "1");

#if 0
	if ((temp_config == 0) && (wsc_value.WscConfigured == 2))
	{
		char wsc_tmp[33];
		char wsc_key_tmp[65];

		CFG_set(CFG_WLN_WscConfigured, "1");

		g_wsc_configured = 1;

		sprintf(mBSSID[0].SSID, "%s", wsc_value.WscSsid);
		memset(wsc_tmp , 0, 33);
		sprintf(wsc_tmp, "%s", wsc_value.WscSsid);

		CFG_set(CFG_WLN_WscSSID, &wsc_tmp[0]);

		if (wsc_value.WscAuthMode == 0x0001)
		{
			CFG_set(CFG_WLN_WscAuthType, "1");
			sprintf(mBSSID[0].AuthMode, "%s", "OPEN");
		}
		else if (wsc_value.WscAuthMode == 0x0002)
		{
			CFG_set(CFG_WLN_WscAuthType, "2");
			sprintf(mBSSID[0].AuthMode, "%s", "WPAPSK");
		}
		else if (wsc_value.WscAuthMode == 0x0004)
		{
			CFG_set(CFG_WLN_WscAuthType, "4");
			sprintf(mBSSID[0].AuthMode, "%s", "SHARED");
		}
		else if (wsc_value.WscAuthMode == 0x0008)
		{
			CFG_set(CFG_WLN_WscAuthType, "8");
			sprintf(mBSSID[0].AuthMode, "%s", "WPA");
		}
		else if (wsc_value.WscAuthMode == 0x0010)
		{
			CFG_set(CFG_WLN_WscAuthType, "16");
			sprintf(mBSSID[0].AuthMode, "%s", "WPA2");
		}
		else if (wsc_value.WscAuthMode == 0x0020)
		{
			CFG_set(CFG_WLN_WscAuthType, "32");
			sprintf(mBSSID[0].AuthMode, "%s", "WPA2PSK");
		}
		else if (wsc_value.WscAuthMode == 0x0022)
		{
			CFG_set(CFG_WLN_WscAuthType, "34");
			sprintf(mBSSID[0].AuthMode, "%s", "WPAPSKWPA2PSK");
		}
		else
		{
			CFG_set(CFG_WLN_WscAuthType, "1");
			sprintf(mBSSID[0].AuthMode, "%s", "OPEN");
		}

		if (wsc_value.WscEncrypType == 0x0001)
		{
			CFG_set(CFG_WLN_WscEncrypType, "1");
			sprintf(mBSSID[0].EncrypType, "%s", "NONE");
			mBSSID[0].DefaultKeyID = 1;
			mBSSID[0].SecurityMode = 0;
		}
		else if (wsc_value.WscEncrypType == 0x0002)
		{
			CFG_set(CFG_WLN_WscEncrypType, "2");
			sprintf(mBSSID[0].EncrypType, "%s", "WEP");

			memset(wsc_key_tmp , 0, 65);
			sprintf(wsc_key_tmp, "%s", wsc_value.WscWPAKey);

			if ((strlen(wsc_key_tmp) == 5) || (strlen(wsc_key_tmp) == 13))
			{
				/* Key Entry Method == ASCII */
				mBSSID[0].Key1Type = 1;
				mBSSID[0].Key2Type = 1;
				mBSSID[0].Key3Type = 1;
				mBSSID[0].Key4Type = 1;
			}
			else if ((strlen(wsc_key_tmp) == 10) || (strlen(wsc_key_tmp) == 26))
			{
				/* Key Entry Method == HEX */
				mBSSID[0].Key1Type = 0;
				mBSSID[0].Key2Type = 0;
				mBSSID[0].Key3Type = 0;
				mBSSID[0].Key4Type = 0;
			}
			else
			{
				/* Key Entry Method == ASCII */
				mBSSID[0].Key1Type = 1;
				mBSSID[0].Key2Type = 1;
				mBSSID[0].Key3Type = 1;
				mBSSID[0].Key4Type = 1;
			}

			if (wsc_value.DefaultKeyIdx == 1)
				sprintf(mBSSID[0].Key1Str, "%s", wsc_value.WscWPAKey);
			else if (wsc_value.DefaultKeyIdx == 2)
				sprintf(mBSSID[0].Key2Str, "%s", wsc_value.WscWPAKey);
			else if (wsc_value.DefaultKeyIdx == 3)
				sprintf(mBSSID[0].Key3Str, "%s", wsc_value.WscWPAKey);
			else if (wsc_value.DefaultKeyIdx == 4)
				sprintf(mBSSID[0].Key4Str, "%s", wsc_value.WscWPAKey);

			mBSSID[0].DefaultKeyID = wsc_value.DefaultKeyIdx;
			mBSSID[0].SecurityMode = 1;
		}
		else if (wsc_value.WscEncrypType == 0x0004)
		{
			CFG_set(CFG_WLN_WscEncrypType, "4");
			sprintf(mBSSID[0].EncrypType, "%s", "TKIP");
			mBSSID[0].DefaultKeyID = 2;
			mBSSID[0].SecurityMode = 3;
			sprintf(mBSSID[0].WPAPSK, "%s", wsc_value.WscWPAKey);
			mBSSID[0].WPAPSK[64] = '\0';
		}
		else if (wsc_value.WscEncrypType == 0x0008)
		{
			CFG_set(CFG_WLN_WscEncrypType, "8");
			sprintf(mBSSID[0].EncrypType, "%s", "AES");
			mBSSID[0].DefaultKeyID = 2;
			mBSSID[0].SecurityMode = 3;
			sprintf(mBSSID[0].WPAPSK, "%s", wsc_value.WscWPAKey);
			mBSSID[0].WPAPSK[64] = '\0';
		}
		else if (wsc_value.WscEncrypType == 0x000c)
		{
			CFG_set(CFG_WLN_WscEncrypType, "12");
			sprintf(mBSSID[0].EncrypType, "%s", "TKIPAES");
			mBSSID[0].DefaultKeyID = 2;
			mBSSID[0].SecurityMode = 3;
			sprintf(mBSSID[0].WPAPSK, "%s", wsc_value.WscWPAKey);
			mBSSID[0].WPAPSK[64] = '\0';
		}
		else
		{
			CFG_set(CFG_WLN_WscEncrypType, "1");
			sprintf(mBSSID[0].EncrypType, "%s", "NONE");
			mBSSID[0].DefaultKeyID = 1;
			mBSSID[0].SecurityMode = 0;
		}
		
		memset(wsc_key_tmp , 0, 65);
		if (wsc_value.WscEncrypType >= 0x0002)
		{
			sprintf(wsc_key_tmp, "%s", wsc_value.WscWPAKey);
			CFG_set(CFG_WLN_WscNewKey, &wsc_key_tmp[0]);
		}
		mBSSID[0].IEEE8021X = 0;

		CFG_commit(0);

		//save_mbss_value_to_flash(1);
	}
	else if ((temp_config == 1) && (wsc_value.WscConfigured == 1))
	{
		CFG_set(CFG_WLN_WscConfigured, "0");
		g_wsc_configured = 0;

		CFG_commit(0);
	}
#endif

	return;
}

void CGI_get_wlan_ssid_num(http_req *req)
{
	char val[255];  

	val[0] = '\0';

	CFG_get_str(CFG_WLN_BssidNum, val);

	if (val == "")
		WEB_printf(req, "%s", "1");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_wds_phy_mode(http_req *req)
{
	char val[255];  

	val[0] = '\0';

	CFG_get_str(CFG_WLN_WdsPhyMode, val);

	if (val == "")
		WEB_printf(req, "%s", "0");
	else if (strcmp(val, "CCK") == 0)
		WEB_printf(req, "%s", "0");
	else if (strcmp(val, "OFDM") == 0)
		WEB_printf(req, "%s", "1");
	else if (strcmp(val, "HTMIX") == 0)
		WEB_printf(req, "%s", "2");
	else if (strcmp(val, "GREENFIELD") == 0)
		WEB_printf(req, "%s", "3");

	return;
}

void CGI_get_wlan_apcli_include(http_req *req)
{
	struct ifreq ifr;
	int s;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (s < 0)
	{
		WEB_printf(req, "%s", "0");
		return;
	}
	
	strcpy(ifr.ifr_name, "apcli0");

	if (ioctl(s, SIOCGIFFLAGS, &ifr))
		WEB_printf(req, "%s", "0");
	else
		WEB_printf(req, "%s", "1");
	
	return;
}

void CGI_get_wlan_ht_sta_associate_list(http_req *req)
{
	int s, idx = 0;
	struct iwreq iwr;
	RT_802_11_MAC_TABLE MacTab;
	struct eth_drv_sc 		*sc;
	cyg_netdevtab_entry_t 	*t;
	int	found = 0;

#ifdef CONFIG_WIRELESS
	memset(&iwr, 0, sizeof(iwr));
	memset(&MacTab, 0, sizeof(RT_802_11_MAC_TABLE));
	//strncpy(iwr.ifr_name, "ra0", IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) &MacTab;

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;

		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
		}
	}

	if (found == 1)
	{
		if(rt28xx_ap_ioctl(sc, RTPRIV_IOCTL_GET_MAC_TABLE, (caddr_t)&iwr) < 0)
		{
			diag_printf("RTPRIV_IOCTL_GET_MAC_TABLE sock fail!!\n");
			return;	
		}
	}
	else
	{
		diag_printf("Can't find ra0!!\n");
		return;	
	}

	for (idx = 0; idx < MacTab.Num; idx++)
	{
		if (MacTab.Entry[idx].TxRate.field.MODE >= 2)
		{
			WEB_printf(req, "<tr style=\"Font-SIZE: 10pt; BACKGROUND: #ffffff\" onClick=\"SelectRow(this, '%02X:%02X:%02X:%02X:%02X:%02X');\"><td align=\"center\" nowrap>",
															MacTab.Entry[idx].Addr[0], MacTab.Entry[idx].Addr[1],
															MacTab.Entry[idx].Addr[2], MacTab.Entry[idx].Addr[3],
															MacTab.Entry[idx].Addr[4], MacTab.Entry[idx].Addr[5]);

			WEB_printf(req, "%02X:%02X:%02X:%02X:%02X:%02X</td>",  MacTab.Entry[idx].Addr[0], MacTab.Entry[idx].Addr[1],
																MacTab.Entry[idx].Addr[2], MacTab.Entry[idx].Addr[3],
																MacTab.Entry[idx].Addr[4], MacTab.Entry[idx].Addr[5]);

			WEB_printf(req, "<td align=\"center\" nowrap>");
			WEB_printf(req, "%d</td>", MacTab.Entry[idx].Aid);
			WEB_printf(req, "<td align=\"center\" nowrap>");
			if ( MacTab.Entry[idx].Psm == 1)
				WEB_printf(req, "YES</td>");
			else
				WEB_printf(req, "NO</td>");
			
			WEB_printf(req, "</tr>");
		}
	}

	return;
#else
	diag_printf("Wireless IF does not init.\n");
	return;
#endif
}

void CGI_get_wlan_ht_ba_ori_sta_info(http_req *req)
{
	int s, tid, idx = 0;
	struct iwreq iwr;
	QUERYBA_TABLE BAT;
	struct eth_drv_sc 		*sc;
	cyg_netdevtab_entry_t 	*t;
	int found = 0;

#ifdef CONFIG_WIRELESS
	memset(&iwr, 0, sizeof(iwr));
	memset(&BAT, 0, sizeof(BAT));
	iwr.u.data.pointer = (caddr_t) &BAT;

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;

		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
		}
	}

	if (found == 1)
	{
		if(rt28xx_ap_ioctl(sc, RTPRIV_IOCTL_QUERY_BATABLE, (caddr_t)&iwr) < 0)
		{
			diag_printf("RTPRIV_IOCTL_QUERY_BATABLE  fail!!!!!\n");
			return;	
		}
	}
	else
	{
		diag_printf("Can't find ra0!!!!!\n");
		return;	
	}
	
	for (idx = 0; idx < BAT.OriNum; idx++)
	{
		WEB_printf(req, "<tr><td align=\"center\" nowrap>");
		WEB_printf(req, "%02X:%02X:%02X:%02X:%02X:%02X</td>",  BAT.BAOriEntry[idx].MACAddr[0], BAT.BAOriEntry[idx].MACAddr[1],
															BAT.BAOriEntry[idx].MACAddr[2], BAT.BAOriEntry[idx].MACAddr[3],
															BAT.BAOriEntry[idx].MACAddr[4], BAT.BAOriEntry[idx].MACAddr[5]);
		for (tid = 0; tid < 8; tid++)
		{
			WEB_printf(req, "<td align=\"center\" nowrap>");
			if (BAT.BAOriEntry[idx].BufSize[tid] == 0)
			WEB_printf(req, "-</td>");
			else
				WEB_printf(req, "%d</td>", BAT.BAOriEntry[idx].BufSize[tid]);
		}
		WEB_printf(req, "</tr>");
	}

	return;
#else
	diag_printf("Wireless IF does not init.\n");
	return;
#endif
}

void CGI_get_wlan_ht_ba_rec_sta_info(http_req *req)
{
	int s, tid, idx = 0;
	struct iwreq iwr;
	QUERYBA_TABLE BAT;
	struct eth_drv_sc 		*sc;
	cyg_netdevtab_entry_t 	*t;
	int found = 0;

#ifdef CONFIG_WIRELESS
	memset(&iwr, 0, sizeof(iwr));
	memset(&BAT, 0, sizeof(BAT));
	iwr.u.data.pointer = (caddr_t) &BAT;

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;

		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
		}
	}

	if (found == 1)
	{
		if(rt28xx_ap_ioctl(sc, RTPRIV_IOCTL_QUERY_BATABLE, (caddr_t)&iwr) < 0)
		{
			diag_printf("RTPRIV_IOCTL_QUERY_BATABLE  fail!!!!!\n");
			return;	
		}
	}
	else
	{
		diag_printf("Can't find ra0!!!!!\n");
		return;	
	}

	for (idx = 0; idx < BAT.RecNum; idx++)
	{
		WEB_printf(req, "<tr><td align=\"center\" nowrap>");
		WEB_printf(req, "%02X:%02X:%02X:%02X:%02X:%02X</td>",  BAT.BARecEntry[idx].MACAddr[0], BAT.BARecEntry[idx].MACAddr[1],
															BAT.BARecEntry[idx].MACAddr[2], BAT.BARecEntry[idx].MACAddr[3],
															BAT.BARecEntry[idx].MACAddr[4], BAT.BARecEntry[idx].MACAddr[5]);
		for (tid = 0; tid < 8; tid++)
		{
			WEB_printf(req, "<td align=\"center\" nowrap>");
			if (BAT.BARecEntry[idx].BufSize[tid] == 0)
				WEB_printf(req, "-</td>");
			else
				WEB_printf(req, "%d</td>", BAT.BARecEntry[idx].BufSize[tid]);
		}
		WEB_printf(req, "</tr>");
	}

	return;
#else
	diag_printf("Wireless IF does not init.\n");
	return;
#endif
}

void CGI_get_wlan_wsc_use_proxy(http_req *req)
{
	char val[255];  

	val[0] = '\0';

	CFG_get_str(CFG_WLN_WscConfMode, val);

	if (val == "")
		WEB_printf(req, "%s", "0");
	else
	{
		if ((strcmp(val, "2") == 0) || (strcmp(val, "3") == 0) || (strcmp(val, "6") == 0) || (strcmp(val, "7") == 0))
			WEB_printf(req, "%s", "1");
		else
			WEB_printf(req, "%s", "0");
	}

	return;
}

void CGI_get_wlan_wsc_use_internal_reg(http_req *req)
{
	char val[255];  

	val[0] = '\0';

	CFG_get_str(CFG_WLN_WscConfMode, val);

	if (val == "")
		WEB_printf(req, "%s", "0");
	else
	{
		if ((strcmp(val, "4") == 0) || (strcmp(val, "5") == 0) || (strcmp(val, "6") == 0) || (strcmp(val, "7") == 0))
			WEB_printf(req, "%s", "1");
		else
			WEB_printf(req, "%s", "0");
	}

	return;
}

void CGI_get_wlan_wsc_pin_code(http_req *req)
{
	struct iwreq iwr;
	unsigned long WscPinCode = 0;
	
#ifdef CONFIG_WIRELESS
	memset(&iwr, 0, sizeof(iwr));
	//strncpy(iwr.ifr_name, "ra0", IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) &WscPinCode;
	iwr.u.data.flags = RT_OID_WSC_PIN_CODE;

	struct eth_drv_sc 		*sc;
	cyg_netdevtab_entry_t 	*t;
	int	found = 0;

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;

		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
		}
	}

	if (found == 1)
	{
		if(rt28xx_ap_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr) < 0)
		{
			diag_printf("RT_PRIV_IOCTL sock fail!!\n");
			WEB_printf(req, "%s", "00000000");
			return;	
		}
	}
	else
	{
		diag_printf("Can't find ra0!!\n");
		WEB_printf(req, "%s", "00000000");
		return;	
	}

	WEB_printf(req, "%08d", (int)WscPinCode);

	return;
#else
	diag_printf("Wireless IF does not init.\n");
	return;
#endif
}

void CGI_get_wlan_wsc_uuid(http_req *req)
{
	struct iwreq	iwr;
	unsigned char	Wsc_Uuid_Str[37];
	struct eth_drv_sc 		*sc;
	cyg_netdevtab_entry_t 	*t;
	int	found = 0;

#ifdef CONFIG_WIRELESS
	memset(Wsc_Uuid_Str, 0, 37);
	memset(&iwr, 0, sizeof(iwr));
	
	//strncpy(iwr.ifr_name, "ra0", IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) &Wsc_Uuid_Str;
	iwr.u.data.flags = RT_OID_WSC_UUID;

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;

		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
		}
	}

	if (found == 1)
	{
		if(rt28xx_ap_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr) < 0)
		{
			diag_printf("RT_PRIV_IOCTL sock fail!!\n");
			WEB_printf(req, "%s", "");
			return;	
		}
	}
	else
	{
		diag_printf("Can't find ra0!!\n");
		WEB_printf(req, "%s", "");
		return;	
	}

	WEB_printf(req, "%s", Wsc_Uuid_Str);

	return;
#else
	diag_printf("Wireless IF does not init.\n");
	return;
#endif
}

void CGI_get_wlan_wsc_dev_pin_code(http_req *req)
{
	struct iwreq iwr;
	struct eth_drv_sc 		*sc;
	cyg_netdevtab_entry_t 	*t;
	char val[255];  
	unsigned long WscPinCode = 0;
	int	found = 0;
	
	val[0] = '\0';

#ifdef CONFIG_WIRELESS
	memset(&iwr, 0, sizeof(iwr));
	//strncpy(iwr.ifr_name, "ra0", IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) &WscPinCode;
	iwr.u.data.flags = RT_OID_WSC_PIN_CODE;

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;

		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
		}
	}

	if (found == 1)
	{
		if(rt28xx_ap_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr) < 0)
		{
			diag_printf("RT_PRIV_IOCTL sock fail!!\n");
			WEB_printf(req, "%s", "00000000");
			return;	
		}
	}
	else
	{
		diag_printf("Can't find ra0!!\n");
		WEB_printf(req, "%s", "00000000");
		return;	
	}

	CFG_get_str(CFG_WLN_WscPinCode, val);

	if ( (strcmp(val, "0") == 0) || (strcmp(val, "") == 0))
		WEB_printf(req, "%08d", (int)WscPinCode);  
	else
		WEB_printf(req, "%s", val); 

	return;
#else
	diag_printf("Wireless IF does not init.\n");
	return;
#endif
}

void CGI_get_wlan_wsc_current_status(http_req *req)
{
	struct iwreq iwr;
	int WscStatus = 0; 
	struct eth_drv_sc 		*sc;
	cyg_netdevtab_entry_t 	*t;
	int	found = 0;

#ifdef CONFIG_WIRELESS
	memset(&iwr, 0, sizeof(iwr));
	//strncpy(iwr.ifr_name, "ra0", IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) &WscStatus;
	iwr.u.data.flags = RT_OID_WSC_QUERY_STATUS;

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;

		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
		}
	}

	if (found == 1)
	{
		if(rt28xx_ap_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr) < 0)
		{
			diag_printf("RT_PRIV_IOCTL sock fail!!!!!\n");
			WEB_printf(req, "%d", WscStatus);
			return;	
		}
	}
	else
	{
		diag_printf("Can't find ra0!!!!!\n");
		WEB_printf(req, "%d", 0);
		return;	
	}

	if (WscStatus == STATUS_WSC_FAIL)
	{
		memset(&iwr, 0, sizeof(iwr));
		//strncpy(iwr.ifr_name, "ra0", IFNAMSIZ);
		iwr.u.data.pointer = (caddr_t) &WscStatus;
		iwr.u.data.flags = RT_OID_WSC_QUERY_STATUS; 
 
		if(rt28xx_ap_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr) < 0)
		{
			diag_printf("RT_PRIV_IOCTL sock fail!!!!!\n");
			WEB_printf(req, "%d", WscStatus);
			return;	
		} 
	}

	WEB_printf(req, "%d", WscStatus);

	return;
#else
	diag_printf("Wireless IF does not init.\n");
	return;
#endif
}

void CGI_get_wlan_wsc_encr_type(http_req *req)
{
}

void CGI_get_wlan_wsc_auth_mode(http_req *req)
{
}

void CGI_get_wlan_wsc_ssid(http_req *req)
{
	char val[255];  

	val[0]='\0'; 

	CFG_get_str(CFG_WLN_WscSSID, val);

	if (val == "")
		WEB_printf(req, "%s", "WscNewAP");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_wsc_pass_phrase(http_req *req)
{ 	
	char val[255];  

	val[0] = '\0'; 

	CFG_get_str(CFG_WLN_WscNewKey, val);

	if (val == "")
		WEB_printf(req, "%s", "");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_wps_process_status(http_req *req)
{ 
	struct iwreq iwr;
	int WscStatus = 0;
	int during=0;
	time_t now_time; 
	struct eth_drv_sc 		*sc;
	cyg_netdevtab_entry_t 	*t;
	int	found = 0;

#ifdef CONFIG_WIRELESS
	memset(&iwr, 0, sizeof(iwr));
	//rncpy(iwr.ifr_name, "ra0", IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) &WscStatus;
	iwr.u.data.flags = RT_OID_WSC_QUERY_STATUS; 

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;

		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
		}
	}

	if (found == 1)
	{
		if(rt28xx_ap_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr) < 0)
		{
			diag_printf("RT_PRIV_IOCTL sock fail!!!!!\n");
			WEB_printf(req, "%d", 0);
			return;	
		}
	}
	else
	{
		diag_printf("Can't find ra0!!!!!\n");
		WEB_printf(req, "%d", WscStatus);
		return;	
	} 

	time(&now_time);
	during = now_time - wps_status_start_time;

	switch (WscStatus)
	{
		case STATUS_WSC_NOTUSED:
			WEB_printf(req, "%s", "WSC NOT USED");
			break;
		case STATUS_WSC_LINK_UP:
			WEB_printf(req, "%s", "WPS AP already Trigger");
			wps_status_flag = 0;
			break;
		case STATUS_WSC_EAPOL_START_SENT:
			WEB_printf(req, "%s", "Sending EAPOL-Start");
			break;
		case STATUS_WSC_EAP_REQ_ID_SENT:
			WEB_printf(req, "%s", "Sending EAP-Rsp(ID)");
			break; 
		//Enrollee
		case STATUS_WSC_EAP_M1_SENT:
			WEB_printf(req, "%s", "Sending M1");
			break;
		case STATUS_WSC_EAP_M2_RECEIVED:
			WEB_printf(req, "%s", "Received M2");
			break;
		case STATUS_WSC_EAP_M2D_RECEIVED:
			WEB_printf(req, "%s", "Received M2D");
			break;
		case STATUS_WSC_EAP_M3_SENT:
			WEB_printf(req, "%s", "Sending M3>");
			break;
		case STATUS_WSC_EAP_M4_RECEIVED:
			WEB_printf(req, "%s", "Received M4");
			break;
		case STATUS_WSC_EAP_M5_SENT:
			WEB_printf(req, "%s", "Sending M5");
			break;
		case STATUS_WSC_EAP_M6_RECEIVED:
			WEB_printf(req, "%s", "Received M6");
			break;
		case STATUS_WSC_EAP_M7_SENT:
			WEB_printf(req, "%s", "Sending M7");
			break;
		case STATUS_WSC_EAP_M8_RECEIVED:
			WEB_printf(req, "%s", "Received M8");
			break; 
		//Registrar
		case STATUS_WSC_EAP_M1_RECEIVED:
			WEB_printf(req, "%s", "Received M1");
			break;
		case STATUS_WSC_EAP_M2_SENT:	
			WEB_printf(req, "%s", "Sending M2");
			break;
		case STATUS_WSC_EAP_M3_RECEIVED:
			WEB_printf(req, "%s", "Received M3");
			break;
		case STATUS_WSC_EAP_M4_SENT:
			WEB_printf(req, "%s", "Sending M4");
			break;
		case STATUS_WSC_EAP_M5_RECEIVED:
			WEB_printf(req, "%s", "Received M5");
			break;
		case STATUS_WSC_EAP_M6_SENT:
			WEB_printf(req, "%s", "Sending M6");
			break;
		case STATUS_WSC_EAP_M7_RECEIVED:
			WEB_printf(req, "%s", "Received M7");
			break;
		case STATUS_WSC_EAP_M8_SENT:
			WEB_printf(req, "%s", "Sending M8");
			break;
		case STATUS_WSC_EAP_RSP_DONE_SENT:
			WEB_printf(req, "%s", "Sending EAP Response (Done)");
			break;
		case STATUS_WSC_CONFIGURED:
			WEB_printf(req, "%s", "Configured");
			break;
		case STATUS_WSC_IDLE:
			if (during >= 119)
			{
				if (wps_status_flag == 0)
				{
					WEB_printf(req, "WPS 2 Mins Timeout !! <br>");
				}

				WEB_printf(req, "<input type=\"button\" value=\"Close Window\" onClick=\"window.close()\">");
			}
			else
				WEB_printf(req, "WPS IDLE!");
			break;
		case STATUS_WSC_FAIL:	
			WEB_printf(req, "WPS FAIL!");
			wps_status_flag = 1;
			break;
		default:
			WEB_printf(req, "%s", "No Suitable message");
			break;
	} 

	return;
#else
	diag_printf("Wireless IF does not init.\n");
	return;
#endif
}

void ConverterStringToDisplay(char *str)
{
	int  len, i;
	char buffer[193];
	char *pOut;

	memset(buffer,0,193);
	len = strlen(str);
	pOut = &buffer[0];

	for (i = 0; i < len; i++)
	{
		/* check special character */

		switch (str[i])
		{
			case '&':
				strcpy (pOut, "&amp;");
				pOut += 5;
				break;

			case '<': 
				strcpy (pOut, "&lt;");
				pOut += 4;
				break;

			case '>': 
				strcpy (pOut, "&gt;");
				pOut += 4;
				break;

			case '"':
				strcpy (pOut, "&quot;");
				pOut += 6;
				break;

			//case ' ':
				//strcpy (pOut, "&nbsp;");
				//pOut += 6;
				//break;

			default:
				if (str[i] <= 31)
				{
					//Device Control Characters
					sprintf(pOut, "&#%02d;", str[i]);
					pOut += 5;
				}
				else if ((str[i]==39) || (str[i]==47) || (str[i]==59) || (str[i]==92))
				{
					// ' / ; (backslash)
					sprintf(pOut, "&#%02d;", str[i]);
					pOut += 5;
				}
				else if (str[i] >= 127)
				{
					//Device Control Characters
					sprintf(pOut, "&#%03d;", str[i]);
					pOut += 6;
				}
				else
				{
					*pOut = str[i];
					pOut++;
				}
				break;
		}
    }

	*pOut = '\0';
	strcpy(str, buffer);

	return;
}

void ConverterStringToDisplay2(char *str)
{
	int  len, i;
	char buffer[193];
	char *pOut;

	memset(buffer,0,193);
	len = strlen(str);
	pOut = &buffer[0];

	for (i = 0; i < len; i++)
	{
		/* check special character */

		switch (str[i])
		{
			case '&':
				strcpy (pOut, "&amp;");
				pOut += 5;
				break;

			case '<':
				strcpy (pOut, "&lt;");
				pOut += 4;
				break;

			case '>':
				strcpy (pOut, "&gt;");
				pOut += 4;
				break;

			case '"':
				strcpy (pOut, "&quot;");
				pOut += 6;
				break;

			case ' ':
				strcpy (pOut, "&nbsp;");
				pOut += 6;
				break;

			default:
				if (str[i] <= 31)
				{
					//Device Control Characters
					sprintf(pOut, "&#%02d;", str[i]);
					pOut += 5;
				}
				else if ((str[i]==39) || (str[i]==47) || (str[i]==59) || (str[i]==92))
				{
					// ' / ; (backslash)
					sprintf(pOut, "&#%02d;", str[i]);
					pOut += 5;
				}
				else if (str[i] >= 127)
				{
					//Device Control Characters
					sprintf(pOut, "&#%03d;", str[i]);
					pOut += 6;
				}
				else
				{
					*pOut = str[i];
					pOut++;
				}
				break;
		}
    }

	*pOut = '\0';
	strcpy(str, buffer);

	return;
}

void CGI_get_wlan_mainssid_for_overview(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_SSID1, val);

	if (val == "")
		WEB_printf(req, "%s", "");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_multissid1_for_overview(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_SSID2, val);

	if (val == "")
		WEB_printf(req, "%s", "");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_multissid2_for_overview(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_SSID3, val);

	if (val == "")
		WEB_printf(req, "%s", "");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_multissid3_for_overview(http_req *req)
{	
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_SSID4, val);

	if (val == "")
		WEB_printf(req, "%s", "");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_driver_version(http_req *req)
{
	unsigned char version_number[16] = {0};

	if (OidQueryInformation(RT_OID_VERSION_INFO, "ra0", &version_number, sizeof(unsigned char)*16) < 0)
	{
		diag_printf("Query RT_OID_VERSION_INFO has error!!!!!\n");
		WEB_printf(req, "Driver not found");
		return;
	}

	WEB_printf(req, "%s", version_number);
	return;
}

void CGI_get_wlan_rc_type(http_req *req)
{
	unsigned char rc_type = -1;

	if (OidQueryInformation(OID_802_11_NETWORK_TYPES_SUPPORTED, "ra0", &rc_type, sizeof(unsigned char)) < 0)
	{
		diag_printf("Query OID_802_11_NETWORK_TYPES_SUPPORTED has error!!!!!\n");
		WEB_printf(req, "Unknow type");
		return;
	}

	if (((int)rc_type == RFIC_2820) || ((int)rc_type == RFIC_2720) 
		|| ((int)rc_type == RFIC_3020) || ((int)rc_type == RFIC_2020)
		|| ((int)rc_type == RFIC_3021) || ((int)rc_type == RFIC_3022)
		|| ((int)rc_type == RFIC_3053) || ((int)rc_type == RFIC_3853))
	{
		WEB_printf(req, "%s", "2.4G Single Band");
	}
	else if (((int)rc_type == RFIC_2850) || ((int)rc_type == RFIC_2750) 
		|| ((int)rc_type == RFIC_3052) || ((int)rc_type == RFIC_2853)
		|| ((int)rc_type == RFIC_3053) || ((int)rc_type == RFIC_3853))
	{
		WEB_printf(req, "%s", "2.4G/5G Dual Band");
	}
	else
	{
		//WEB_printf(req, "%s", "Unknow Type");
		WEB_printf(req, "%s", "2.4G Single Band");
	}

	return;
}

void CGI_get_wlan_current_wireless_mode(http_req *req)
{
	char val[255];  	
	int  w_mode = 2561;

	val[0] = '\0';

	CFG_get_str(CFG_WLN_ModuleName, val);

	if (strcmp(val, "RT2860") == 0 || strcmp(val, "RT2870") == 0)  
		w_mode = 2860;
	else if (strcmp(val, "RT2561") == 0)
		w_mode = 2561;	
	else if (strcmp(val, "RT3090") == 0)
		w_mode = 3090;
	else if (strcmp(val, "RT3070") == 0)
		w_mode = 3070;
	else if (strcmp(val, "RT2070") == 0)
		w_mode = 2070;
	
	CFG_get_str(CFG_WLN_WirelessMode, val);

	if (strlen(val) == 0)
	{
		WEB_printf(req, "%s", "11b/g WiFi");
		return;
	}

	if (strcmp(val, "0") == 0)
	{
		WEB_printf(req, "%s", "11b/g WiFi");
	}
	else if (strcmp(val, "5") == 0)
	{
		WEB_printf(req, "%s", "11b/g Performance");
	}
	else if (strcmp(val, "1") == 0)
	{
		WEB_printf(req, "%s", "11b");
	}
	else if (strcmp(val, "2") == 0)
	{
		if (w_mode == 2860)
			WEB_printf(req, "%s", "11a");
		else
			WEB_printf(req, "%s", "11g Only");
	}
	else if (strcmp(val, "3") == 0)
		WEB_printf(req, "%s", "11a");
	else if (strcmp(val, "4") == 0)
		WEB_printf(req, "%s", "11g Only");
	else if (strcmp(val, "8") == 0)
		WEB_printf(req, "%s", "11a/n Mixed");
	else if (strcmp(val, "9") == 0)
		WEB_printf(req, "%s", "11b/g/n Mixed");
	else
		WEB_printf(req, "%s", "11b/g WiFi");

	return;
}

void CGI_get_wlan_current_channel(http_req *req)
{
	int  channel_index = 0;
	int  module_name = 2561;
	char val[255];  	

	val[0] = '\0';

	CFG_get_str(CFG_WLN_ModuleName, val);

	if (strcmp(val, "RT2860") == 0 || strcmp(val, "RT2870") == 0 )
		module_name = 2860;
	else if (strcmp(val, "RT3090") == 0)
		module_name = 3090;
	else if (strcmp(val, "RT3070") == 0)
		module_name = 3070;
	else if (strcmp(val, "RT2070") == 0)
		module_name = 2070;
	else
		module_name = 2561;

	CFG_get_str(CFG_WLN_AutoChannelSelect, val);

	if ((strcmp(val, "1") == 0) || ((strcmp(val, "2") == 0)))
	{
		WEB_printf(req, "%s", "Auto Select");
		return;
	}

	memset(val, 0, 255);
	CFG_get_str(CFG_WLN_WirelessMode, val);

	if (module_name == 2561)
	{
		if (strcmp(val, "3") == 0)
		{
			memset(val, 0, 255);
			CFG_get_str(CFG_WLN_Channel, val);
			channel_index = atoi(val);
			WEB_printf(req, "%d MHz(Channel %d)", 5000 + (5*channel_index), channel_index);
		}
		else
		{
			memset(val, 0, 255);
			CFG_get_str(CFG_WLN_Channel, val);

			if (strlen(val) == 0)
			{
				WEB_printf(req, "2452MHz (Channel %d)", 9);
				return;
			}

			channel_index = atoi(val);
			if (channel_index == 14)
				WEB_printf(req, "2484 MHz(Channel %d)", channel_index);
			else
				WEB_printf(req, "%d MHz(Channel %d)", 2407 + (5*channel_index), channel_index);
		}
	}
	else
	{
		if ((strcmp(val, "2") == 0) || (strcmp(val, "8") == 0))
		{
			memset(val, 0, 255);
			CFG_get_str(CFG_WLN_Channel, val);
			channel_index = atoi(val);
			WEB_printf(req, "%d MHz(Channel %d)", 5000 + (5*channel_index), channel_index);
		}
		else
		{
			memset(val, 0, 255);
			CFG_get_str(CFG_WLN_Channel, val);

			if (val == "")
			{
				WEB_printf(req, "2452MHz (Channel %d)", 9);
				return;
			}

			channel_index = atoi(val);
			if (channel_index == 14)
				WEB_printf(req, "2484 MHz(Channel %d)", channel_index);
			else
				WEB_printf(req, "%d MHz(Channel %d)", 2407 + (5*channel_index), channel_index);
		}
	}

	return;
}

void CGI_get_wlan_current_broadcast_ssid(http_req *req)
{	
	char val[255];  
	char	*tok = NULL;
	int HideSSIDTemp;

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_HideSSID, val);

	if (val == "")
	{
		WEB_printf(req, "%s", "");
		return;
	}

	tok = strtok(val,";");

	HideSSIDTemp = atoi(tok);
	
	if (HideSSIDTemp == 0)
		WEB_printf(req, "%s", "Enable");
	else if (HideSSIDTemp == 1)
		WEB_printf(req, "%s", "Disable");
		
	return;
}

void CGI_get_wlan_apclient_bssid(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_ApCliBssid, val);
	
	WEB_printf( req, "%s",val );
	
	if (val == "")
		WEB_printf(req, "%s", "");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_apclient_auth(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_ApCliAuthMode, val);
	
	WEB_printf( req, "%s",val );
	
	if (val == "")
		WEB_printf(req, "%s", "OPEN");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_apclient_encryp_type(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_ApCliEncrypType, val);
	
	WEB_printf( req, "%s",val );
	
	if (val == "")
		WEB_printf(req, "%s", "NONE");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_apclient_wep_key_type(http_req *req)
{
	char val[255];  

	val[0]='\0';
	
	CFG_get_str(CFG_WLN_ApCliKey1Type, val);
	
	WEB_printf( req, "%s",val );
	
	if (val == "")
		WEB_printf(req, "%s", "0");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_apclient_wep_key1(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_ApCliKey1Str, val);
	
	WEB_printf( req, "%s",val );
	
	if (val == "")
		WEB_printf(req, "%s", "");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_apclient_wep_key2(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_ApCliKey2Str, val);
	
	WEB_printf( req, "%s",val );
	
	if (val == "")
		WEB_printf(req, "%s", "");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_apclient_wep_key3(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_ApCliKey3Str, val);
	
	WEB_printf( req, "%s",val );
	
	if (val == "")
		WEB_printf(req, "%s", "");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_apclient_wep_key4(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_ApCliKey4Str, val);
	
	WEB_printf( req, "%s",val );
	
	if (val == "")
		WEB_printf(req, "%s", "");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_apclient_default_key(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_ApCliDefaultKeyID, val);
	
	WEB_printf( req, "%s",val );
	
	if (val == "")
		WEB_printf(req, "%s", "1");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_apclient_wpapsk(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_ApCliWPAPSK, val);
	
	WEB_printf( req, "%s",val );
	
	if (val == "")
		WEB_printf(req, "%s", "");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_apclient_ssid(http_req *req)
{
	char val[255];  
	char str_buffer[193] = {0};

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_ApCliSsid, val);
	
	if (strlen(val)==0)
		WEB_printf(req, "%s", "RalinkAPClient");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_wan_connection_mode(http_req *req)
{
	char val[255];  

	val[0] = '\0';
	
	CFG_get_str(CFG_WLN_wanConnectionMode, val);

	if (val == "")
		WEB_printf(req, "%s", "STATIC");
	else
		WEB_printf(req, "%s", val);

	return;
}

void CGI_get_wlan_station_associate_list(http_req *req)
{
	int s, idx = 0;
	struct iwreq iwr;
	RT_802_11_MAC_TABLE MacTab;
#if 0
	char *cck_mode[8] = {"1", "2", "5.5", "11", "1", "2", "5.5", "11"};
	char *ofdm_mode[8] = {"6", "9", "12", "18", "24", "36", "48", "54"};
	char *ht_mode[80]={"6.5" ,"13.0" ,"19.5" ,"26.0" ,"39.0" ,"52.0" ,"58.5" ,"65.0" ,"78" ,"N/A" , // BW=20, GI=800(long)
						"7.2" ,"14.4" ,"21.7" ,"28.9" ,"43.3" ,"57.8" ,"65.0" ,"72.2" ,"86.7" ,"N/A" , // BW=20, GI=400(short)
						"13.5" ,"27.0" ,"40.5" ,"54.0" ,"81.0" ,"108.0" ,"121.5" ,"135.0" ,"162" ,"180"  , // BW=40, GI=800(long)
						"15.0" ,"30.0" ,"45.0" ,"60.0" ,"90.0" ,"120.0" ,"135.0" ,"150.0" ,"180.0" ,"200.0" , // BW=40, GI=400(short)
						"29.3" ,"58.5" ,"87.8" ,"117" ,"175.5" ,"234" ,"263.3" ,"292.5" ,"351" ,"390"  , // BW=80, GI=800(long)
						"32.5" ,"65" ,"97.5" ,"130" ,"195" ,"260" ,"292.5" ,"325" ,"390" ,"433.3" , // BW=80, GI=400(short)
						"58.5" ,"117" ,"175.5" ,"234" ,"351" ,"468" ,"526.5" ,"585" ,"702" ,"780"  , // BW=160, GI=800(long)
						"65" ,"130" ,"195" ,"260" ,"390" ,"520" ,"585" ,"650" ,"780" ,"866.7"  // BW=160, GI=400(short)
						};
#endif
	int during=0;
	time_t now_time; 
	struct eth_drv_sc		*sc;
	cyg_netdevtab_entry_t	*t;
	int found = 0;
	unsigned long TXRate;

#ifdef CONFIG_WIRELESS	

	memset(&iwr, 0, sizeof(iwr));
	memset(&MacTab, 0, sizeof(RT_802_11_MAC_TABLE));
	//srncpy(iwr.ifr_name, "ra0", IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) &MacTab;

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;

		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
		}
	}

	if (found == 1)
	{
		if(rt28xx_ap_ioctl(sc, RTPRIV_IOCTL_GET_MAC_TABLE, (caddr_t)&iwr) < 0)
		{
			diag_printf("RT_PRIV_IOCTL fail!!\n");
			return; 
		}
	}
	else
	{
		diag_printf("Can't find ra0!!\n");
		return; 
	} 

	for (idx = 0; idx < MacTab.Num; idx++)
	{
		WEB_printf(req, "<tr><td align=\"center\" bgcolor=\"#FFFFFF\" nowrap>");
		WEB_printf(req, "<input type=radio name=sta_mac value=1 onClick=\"del_associate_onClick('%02X:%02X:%02X:%02X:%02X:%02X');\"></td>", MacTab.Entry[idx].Addr[0], MacTab.Entry[idx].Addr[1],
																									MacTab.Entry[idx].Addr[2], MacTab.Entry[idx].Addr[3],
																									MacTab.Entry[idx].Addr[4], MacTab.Entry[idx].Addr[5]);
		WEB_printf(req, "<td align=\"center\" bgcolor=\"#FFFFFF\" nowrap>");
		WEB_printf(req, "%02X:%02X:%02X:%02X:%02X:%02X</td>",	MacTab.Entry[idx].Addr[0], MacTab.Entry[idx].Addr[1],
															MacTab.Entry[idx].Addr[2], MacTab.Entry[idx].Addr[3],
															MacTab.Entry[idx].Addr[4], MacTab.Entry[idx].Addr[5]);

		WEB_printf(req, "<td align=\"center\" bgcolor=\"#FFFFFF\" nowrap>");

		if (MacTab.Entry[idx].TxRate.field.MODE == MODE_CCK)
			WEB_printf(req, "CCK</td>");
		else if (MacTab.Entry[idx].TxRate.field.MODE == MODE_OFDM)
			WEB_printf(req, "OFDM</td>");
		else if (MacTab.Entry[idx].TxRate.field.MODE == MODE_HTMIX)
			WEB_printf(req, "HT Mixed</td>");
		else if (MacTab.Entry[idx].TxRate.field.MODE == MODE_HTGREENFIELD)
			WEB_printf(req, "HT GreenField</td>");


		WEB_printf(req, "<td align=\"center\" bgcolor=\"#FFFFFF\" nowrap>");
		WEB_printf(req, "%d</td>", MacTab.Entry[idx].Aid);

		WEB_printf(req, "<td align=\"center\" bgcolor=\"#FFFFFF\" nowrap>");
		if ( MacTab.Entry[idx].Psm == 1)
			WEB_printf(req, "YES</td>");
		else
			WEB_printf(req, "NO</td>");

		WEB_printf(req, "<td align=\"center\" bgcolor=\"#FFFFFF\" nowrap>");
		//WEB_printf(req, "%d</td>", (int)MacTab.Entry[idx].MimoPs);
		if (MacTab.Entry[idx].TxRate.field.MODE >= MODE_HTMIX)
		{
			if ((int)MacTab.Entry[idx].MimoPs == 0)
				WEB_printf(req, "STATIC</td>");
			else if ((int)MacTab.Entry[idx].MimoPs == 1)
				WEB_printf(req, "DYNAMIC</td>");
			else if ((int)MacTab.Entry[idx].MimoPs == 3)
				WEB_printf(req, "NO_LIMIT</td>");
		}
		else
		{
			WEB_printf(req, "NONE</td>");
		}

		WEB_printf(req, "<td align=\"center\" bgcolor=\"#FFFFFF\" nowrap>");
		if (MacTab.Entry[idx].TxRate.field.MODE >= MODE_HTMIX)
		{
			if (MacTab.Entry[idx].TxRate.field.BW == 0)
				WEB_printf(req, "20</td>");
			else if (MacTab.Entry[idx].TxRate.field.BW == 1)
				WEB_printf(req, "40</td>");
			else  if (MacTab.Entry[idx].TxRate.field.BW == 2)
			WEB_printf(req, "80</td>");
			else if (MacTab.Entry[idx].TxRate.field.BW == 3) 
			WEB_printf(req, "160</td>");

		}
		else
		{
			WEB_printf(req, "NONE</td>");
		}
		
		WEB_printf(req, "<td align=\"center\" bgcolor=\"#FFFFFF\" nowrap>");
#if 0
		
		WEB_printf(req, "<td align=\"center\" bgcolor=\"#FFFFFF\" nowrap>");
		if (MacTab.Entry[idx].TxRate.field.MODE >= MODE_HTMIX)
		{
			if (MacTab.Entry[idx].TxRate.field.BW == 0)
			{
				if (MacTab.Entry[idx].TxRate.field.ShortGI == 0)
				{
					switch (MacTab.Entry[idx].TxRate.field.MCS)
					{
						case 0: 	WEB_printf(req, "%s Mbps</td>", ht_mode[0]);	break;
						case 1: 	WEB_printf(req, "%s Mbps</td>", ht_mode[1]);	break;
						case 2: 	WEB_printf(req, "%s Mbps</td>", ht_mode[2]);	break;
						case 3: 	WEB_printf(req, "%s Mbps</td>", ht_mode[3]);	break;
						case 4: 	WEB_printf(req, "%s Mbps</td>", ht_mode[4]);	break;
						case 5: 	WEB_printf(req, "%s Mbps</td>", ht_mode[5]);	break;
						case 6: 	WEB_printf(req, "%s Mbps</td>", ht_mode[6]);	break;
						case 7: 	WEB_printf(req, "%s Mbps</td>", ht_mode[7]);	break;
						case 8: 	WEB_printf(req, "%s Mbps</td>", ht_mode[8]);	break;
						case 9: 	WEB_printf(req, "%s Mbps</td>", ht_mode[9]);	break;
						case 10:		WEB_printf(req, "%s Mbps</td>", ht_mode[10]);	break;
						case 11:		WEB_printf(req, "%s Mbps</td>", ht_mode[11]);	break;
						case 12:		WEB_printf(req, "%s Mbps</td>", ht_mode[12]);	break;
						case 13:		WEB_printf(req, "%s Mbps</td>", ht_mode[13]);	break;
						case 14:		WEB_printf(req, "%s Mbps</td>", ht_mode[14]);	break;
						case 15:		WEB_printf(req, "%s Mbps</td>", ht_mode[15]);	break;
						default:		WEB_printf(req, "Auto</td>");	break;
					}
				}
				else if (MacTab.Entry[idx].TxRate.field.ShortGI == 1)
				{
					switch (MacTab.Entry[idx].TxRate.field.MCS)
					{
						case 0: 	WEB_printf(req, "%s Mbps</td>", ht_mode[16]);	break;
						case 1: 	WEB_printf(req, "%s Mbps</td>", ht_mode[17]);	break;
						case 2: 	WEB_printf(req, "%s Mbps</td>", ht_mode[18]);	break;
						case 3: 	WEB_printf(req, "%s Mbps</td>", ht_mode[19]);	break;
						case 4: 	WEB_printf(req, "%s Mbps</td>", ht_mode[20]);	break;
						case 5: 	WEB_printf(req, "%s Mbps</td>", ht_mode[21]);	break;
						case 6: 	WEB_printf(req, "%s Mbps</td>", ht_mode[22]);	break;
						case 7: 	WEB_printf(req, "%s Mbps</td>", ht_mode[23]);	break;
						case 8: 	WEB_printf(req, "%s Mbps</td>", ht_mode[24]);	break;
						case 9: 	WEB_printf(req, "%s Mbps</td>", ht_mode[25]);	break;
						case 10:		WEB_printf(req, "%s Mbps</td>", ht_mode[26]);	break;
						case 11:		WEB_printf(req, "%s Mbps</td>", ht_mode[27]);	break;
						case 12:		WEB_printf(req, "%s Mbps</td>", ht_mode[28]);	break;
						case 13:		WEB_printf(req, "%s Mbps</td>", ht_mode[29]);	break;
						case 14:		WEB_printf(req, "%s Mbps</td>", ht_mode[30]);	break;
						case 15:		WEB_printf(req, "%s Mbps</td>", ht_mode[31]);	break;
						default:		WEB_printf(req, "Auto</td>");	break;
					}
				}
			}
			else if (MacTab.Entry[idx].TxRate.field.BW == 1)
			{
				if (MacTab.Entry[idx].TxRate.field.ShortGI == 0)
				{
					switch (MacTab.Entry[idx].TxRate.field.MCS)
					{
						case 0: 	WEB_printf(req, "%s Mbps</td>", ht_mode[32]);	break;
						case 1: 	WEB_printf(req, "%s Mbps</td>", ht_mode[33]);	break;
						case 2: 	WEB_printf(req, "%s Mbps</td>", ht_mode[34]);	break;
						case 3: 	WEB_printf(req, "%s Mbps</td>", ht_mode[35]);	break;
						case 4: 	WEB_printf(req, "%s Mbps</td>", ht_mode[36]);	break;
						case 5: 	WEB_printf(req, "%s Mbps</td>", ht_mode[37]);	break;
						case 6: 	WEB_printf(req, "%s Mbps</td>", ht_mode[38]);	break;
						case 7: 	WEB_printf(req, "%s Mbps</td>", ht_mode[39]);	break;
						case 8: 	WEB_printf(req, "%s Mbps</td>", ht_mode[40]);	break;
						case 9: 	WEB_printf(req, "%s Mbps</td>", ht_mode[41]);	break;
						case 10:		WEB_printf(req, "%s Mbps</td>", ht_mode[42]);	break;
						case 11:		WEB_printf(req, "%s Mbps</td>", ht_mode[43]);	break;
						case 12:		WEB_printf(req, "%s Mbps</td>", ht_mode[44]);	break;
						case 13:		WEB_printf(req, "%s Mbps</td>", ht_mode[45]);	break;
						case 14:		WEB_printf(req, "%s Mbps</td>", ht_mode[46]);	break;
						case 15:		WEB_printf(req, "%s Mbps</td>", ht_mode[47]);	break;
						default:		WEB_printf(req, "Auto</td>");	break;
					}
				}
				else if (MacTab.Entry[idx].TxRate.field.ShortGI == 1)
				{
					switch (MacTab.Entry[idx].TxRate.field.MCS)
					{
						case 0: 	WEB_printf(req, "%s Mbps</td>", ht_mode[50]);	break;
						case 1: 	WEB_printf(req, "%s Mbps</td>", ht_mode[51]);	break;
						case 2: 	WEB_printf(req, "%s Mbps</td>", ht_mode[52]);	break;
						case 3: 	WEB_printf(req, "%s Mbps</td>", ht_mode[53]);	break;
						case 4: 	WEB_printf(req, "%s Mbps</td>", ht_mode[54]);	break;
						case 5: 	WEB_printf(req, "%s Mbps</td>", ht_mode[55]);	break;
						case 6: 	WEB_printf(req, "%s Mbps</td>", ht_mode[56]);	break;
						case 7: 	WEB_printf(req, "%s Mbps</td>", ht_mode[57]);	break;
						case 8: 	WEB_printf(req, "%s Mbps</td>", ht_mode[58]);	break;
						case 9: 	WEB_printf(req, "%s Mbps</td>", ht_mode[59]);	break;
						case 10:		WEB_printf(req, "%s Mbps</td>", ht_mode[60]);	break;
						case 11:		WEB_printf(req, "%s Mbps</td>", ht_mode[61]);	break;
						case 12:		WEB_printf(req, "%s Mbps</td>", ht_mode[62]);	break;
						case 13:		WEB_printf(req, "%s Mbps</td>", ht_mode[63]);	break;
						case 14:		WEB_printf(req, "%s Mbps</td>", ht_mode[64]);	break;
						case 15:		WEB_printf(req, "%s Mbps</td>", ht_mode[65]);	break;
						default:		WEB_printf(req, "Auto</td>");	break;
					}
				}
			}
		}
		else if (MacTab.Entry[idx].TxRate.field.MODE == MODE_OFDM)
		{
			switch (MacTab.Entry[idx].TxRate.field.MCS)
			{
				case 0: 	WEB_printf(req, "%s Mbps</td>", ofdm_mode[0]);	break;
				case 1: 	WEB_printf(req, "%s Mbps</td>", ofdm_mode[1]);	break;
				case 2: 	WEB_printf(req, "%s Mbps</td>", ofdm_mode[2]);	break;
				case 3: 	WEB_printf(req, "%s Mbps</td>", ofdm_mode[3]);	break;
				case 4: 	WEB_printf(req, "%s Mbps</td>", ofdm_mode[4]);	break;
				case 5: 	WEB_printf(req, "%s Mbps</td>", ofdm_mode[5]);	break;
				case 6: 	WEB_printf(req, "%s Mbps</td>", ofdm_mode[6]);	break;
				case 7: 	WEB_printf(req, "%s Mbps</td>", ofdm_mode[7]);	break;
				default:		WEB_printf(req, "Auto</td>");	break;
			}
		}
		else
		{
			switch (MacTab.Entry[idx].TxRate.field.MCS)
			{
				case 0: 	WEB_printf(req, "%s Mbps</td>", cck_mode[0]);	break;
				case 1: 	WEB_printf(req, "%s Mbps</td>", cck_mode[1]);	break;
				case 2: 	WEB_printf(req, "%s Mbps</td>", cck_mode[2]);	break;
				case 3: 	WEB_printf(req, "%s Mbps</td>", cck_mode[3]);	break;
				case 8: 	WEB_printf(req, "%s Mbps</td>", cck_mode[4]);	break;
				case 9: 	WEB_printf(req, "%s Mbps</td>", cck_mode[5]);	break;
				case 10:		WEB_printf(req, "%s Mbps</td>", cck_mode[6]);	break;
				case 11:		WEB_printf(req, "%s Mbps</td>", cck_mode[7]);	break;
				default:		WEB_printf(req, "Auto</td>");	break;
			}
		}
		WEB_printf(req, "</tr>");
#else		
	  getRate(MacTab.Entry[idx].TxRate, &TXRate);
	  WEB_printf(req, "%d Mbps</td>", TXRate);
	  WEB_printf(req, "</tr>");
#endif

   }

   return;
#else
	diag_printf("Wireless IF does not init.\n");
	return;
#endif
}


void getWlanMacAddress(char *ifname, char *if_hw)
{
	struct ifreq ifr;
	char *ptr;
	int skfd;
	
	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		diag_printf("Open socket fail!!!!!\n");
		return;
	}

	/* hardware address */
	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
	if(ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) 
	{
		diag_printf("SIOCGIFHWADDR fail!!!!!\n");
		return;
	}

	ptr = (char *)&ifr.ifr_addr.sa_data;
	
	sprintf(if_hw, "%02X:%02X:%02X:%02X:%02X:%02X",
	(ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
	(ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377));

	(void) close(skfd);
}

void CGI_get_wlan_current_mac_address(http_req *req)
{
	memset(if_hw, 0, 18);
	
	getWlanMacAddress("ra0", if_hw);
	
	WEB_printf(req, "%s", if_hw);

	return;
}

char * CGI_get_wlan_tx_rx_count(void)
{
	struct iwreq	iwr;
	NDIS_802_11_STATISTICS pStatistics;
	struct eth_drv_sc 		*sc;
	cyg_netdevtab_entry_t 	*t;
	int found = 0;
	int i;

#ifdef CONFIG_WIRELESS
	memset(&pStatistics, 0, sizeof(pStatistics));
	memset(wlan_tx_rx_count, 0, 100);
	memset(&iwr, 0, sizeof(iwr));
	
	iwr.u.data.pointer = (caddr_t) &pStatistics;
	iwr.u.data.flags = OID_802_11_STATISTICS;
	
	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;

		if (strcmp(sc->dev_name, "ra0") == 0) 
		{
			found = 1;
			break;
		}
	}
	
	if (found == 1)
	{
		if(rt28xx_ap_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr) < 0)
		{
			diag_printf("RT_PRIV_IOCTL sock fail!!\n");
			return NULL;	
		}
	}
	else
	{
		diag_printf("Can't find ra0!!\n");
		return NULL;	
	}

	sprintf(wlan_tx_rx_count, "%lld\n%lld\n", pStatistics.ReceivedFragmentCount.QuadPart, pStatistics.TransmittedFragmentCount.QuadPart);
	return &wlan_tx_rx_count[0];
#else
	diag_printf("Wireless IF does not init.\n");
	return NULL;
#endif
}

void CGI_get_wlan_mcs_list(http_req *req)
{
	int idx;

	for (idx = 0; idx < SYS_TX_ANTENNA*8; idx++)
	{
		WEB_printf(req, "<option value = %d>%d</option>\n", idx, idx);		
	}

	if (SYS_TX_ANTENNA != 1)
		WEB_printf(req, "<option value = %d>%d</option>\n", 32, 32);

	WEB_printf(req, "<option value = %d selected>%s</option>\n", 33, "Auto");

	return;
}

void CGI_get_wlan_tx_stream_list(http_req *req)
{
	int idx;

	for (idx = 1; idx < SYS_TX_ANTENNA + 1; idx++)
	{
		WEB_printf(req, "<option value = %d>%d</option>\n", idx, idx);
	}
	
	return;	
}

void CGI_get_wlan_rx_stream_list(http_req *req)
{
	int idx;

	for (idx = 1; idx < SYS_RX_ANTENNA + 1; idx++)
	{
		WEB_printf(req, "<option value = %d>%d</option>\n", idx, idx);
	}
	
	return;	
}

void CGI_get_wlan_antenna_type(http_req *req)
{
	WEB_printf(req, "%d", SYS_TX_ANTENNA);

	return;
}

int CGI_check_is_admin(http_req *req)
{
	char username[64]={0};
	CFG_get(CFG_SYS_USERS, username);
	
	if(req==NULL)
		return 0;
	if(strcmp(username,req->auth_username)==0)
		return 1;
	else
		return 0;		
}	

void CGI_get_ping_test(http_req *req,char *ip)
{
	int value=-1;
	int dsst;

	if ((dsst = nstr2ip(ip)) != 0)
		value=net_ping_test(dsst, 1, 3000);
	WEB_printf( req, "%d and ip=%s and dsst=%d", value,ip,dsst);
}

void CGI_get_ping_result(http_req *req)
{
	char val[255];  	

	if (PING_Result == "" || PING_Result[0]=='\0')
	{
		WEB_printf(req, "%s", "");
	}
	else
		{

		WEB_printf(req, "%s", PING_Result);
		}

	PING_Result[0] = '\0';

	return;
}
void CGI_set_neth(http_req *req)
{
	char fephy[5]="off";
	f_setting_set("FEPHY", fephy);
	f_setting_set("FVEW", "Disable");
	f_setting_save();
	m2m_cfg_set("M2M_FVEW", "0");
	CFG_save(0);
	return;
}
void CGI_set_eth(http_req *req)
{
	char fephy[5]="on";
	f_setting_set("FEPHY", fephy);
	f_setting_set("FVEW", "Enable");
	f_setting_save();
	m2m_cfg_set("M2M_FVEW", "1");
	CFG_save(0);
	return;
}

void CGI_set_restart(http_req *req)
{
	mon_snd_cmd(MON_CMD_REBOOT);
	cyg_thread_delay(500);
	return;
}

void CGI_flash_read(http_req *req)
{
	char *start,*len;
	int istart,ilen;	
	char *buff=NULL;
	int i;	

	start=query_getvar(req,"START");
	len=query_getvar(req,"LEN");
	istart=ilen=-1;
	if(start!=NULL&&len!=NULL)
	{
		istart=atoi(start);
		ilen=atoi(len);
	}
	
	if(istart<0||ilen<=0||(istart+ilen)>CONFIG_XROUTER_FLASH_SIZE)
	{
		istart=1024-64;
		ilen=64;
	}
	
	buff = (char*)malloc(1024);
	if(buff==NULL)
	{
		WEB_printf(req, "no memory!");
		return;	
	}
	
	for(i=0;i<ilen;i++)
	{
		flsh_read((istart+i)*1024,buff,1024);
		httpd_write(req,buff,1024);
	}
		
	free(buff);
	return;
}

void CGI_m2m_get_cur_profile(http_req *req)
{
	int i;
	char *pkey=NULL;
	static char cval[128]={0};

	WEB_printf(req,"#PROFILE\n#VER_2_1\n");
	
	for(i=2;i<CFG_ID_STR_NUM;i++)
	{
		bzero(cval,sizeof(cval));
		pkey=cfgid_str_table[i].str;
		CFG_get_str(cfgid_str_table[i].id, cval);
		WEB_printf(req,"%s=%s\n",pkey,cval);
	}
	WEB_printf(req,"\n\n");
	return;
}



