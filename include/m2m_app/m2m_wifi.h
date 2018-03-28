
#ifndef _M2M_WIFI_H_H_
#define _M2M_WIFI_H_H_

#define MAX_AP_LIST_DEPTH		(50)

#define GPIO2_SETUP_AP_SSID	"HF_GPIO2_SETUP_AXX_V4X"

#define AUTH_STR_LEN		(16)
#define ENCRY_STR_LEN		(16)
#define BSSID_STR_LEN		(32)
#define SSID_STR_LEN		(32)

extern char m2m_wireless_ssid[35];
extern char m2m_wireless_Bssid[35];
extern char m2m_wireless_auth[10];
extern char m2m_wireless_encryp[10];
extern char m2m_wireless_key[70];
extern char m2m_wireless_setBssid[35];

void m2m_wifi_init(void);

#define AP_ITEM_TIMEOUT	(6)

typedef struct _AP_CONFIG
{
	char valid;
	char auth[AUTH_STR_LEN+4];
	char encry[ENCRY_STR_LEN+4];
	char bssid[BSSID_STR_LEN+4];
	char ssid[SSID_STR_LEN+4];
	int  rssi;
	int channel;
	time_t wscan_time;
	int  timeouts;
}AP_CONFIG,*PAP_CONFIG;

extern AP_CONFIG staWscanAps[MAX_AP_LIST_DEPTH];

extern cyg_mutex_t wscan_mutex;

void m2m_wifi_init(void);

int m2m_wifi_sta_init(void);

 void m2m_sta_update(int sta);

int m2m_wscan(int retry_times,int iforce,int asyn);

int m2m_wscan_json(char *json,int len);

void m2m_sta_enable_auto_connect(int enable);
void m2m_wscan_show(AP_CONFIG *aps,int cnt);


//WiFi STA API 

void m2m_sta_load_config(void);


int m2m_sta_auto_connect_to_ap(char *ssid, char *bssid, char *auth, char *encry,int channel,int times);
int m2m_chk_sta_connect(void);

int m2m_sta_connect_to_gpio2_setup_ap(void);

 int is_encry_ok(char *ma, char *me, char *sa, char *se);

int m2m_wscan_special_aps(PAP_CONFIG *aplist,int cnt,char *ssid, char *bssid, char *auth, char *encry,int need_wscan);

void trimup_info_new(char *info, int *p, int len, char *SSID, char *BSSID, int *nChannel, char *strEncry, char *strAuth, char *NetworkType, char *RSSI);

void wscan_trimup_info_new(char *info, char *rep);
int m2m_wscan_atcmd(char *str,int len,char *ssid,char *format);



#endif


