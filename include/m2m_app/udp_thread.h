#ifndef UDP_THREAD_H
#define UDP_THREAD_H

int m2m_udp_socket;
struct sockaddr_in m2m_udp_toaddr;

void m2m_udp_thread(void *data);
//void udp_agreement_thread(void *data);

#endif

