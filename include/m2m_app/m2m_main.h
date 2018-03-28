#ifndef M2M_APP_MAIN_H
#define M2M_APP_MAIN_H

#define M2M_WIRELESS_MODE_AP			0
#define M2M_WIRELESS_MODE_STA		1

extern  int m2m_wireless_nlink;

extern int m2m_sttc;			// STA auto try to connect

extern char module_id[50];

extern int m2m_wireless_mode;

void m2m_do_hosttoip(void);
void setto_work_state(void);
int m2m_tcpb_enable(void);

#endif

