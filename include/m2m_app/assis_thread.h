#ifndef ASSIS_THREAD_H
#define ASSIS_THREAD_H


int m2m_assis_socket;
char m2m_lan_ip[20];
char m2m_wan_ip[20];
char m2m_wmac[20];

int m2m_assis_working;

hfthread_hande_t hfth_assis_handle;


void m2m_assis_thread(cyg_addrword_t p);
int m2m_assis_write(char *buf, int len);
int m2m_assis_read(char *buf, int len);
int m2m_assis_read_try(char *buf, int len, int tm);
int  start_assis_thread(uint16_t port);


#endif
