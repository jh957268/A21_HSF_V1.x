#ifndef CONNECT_THREAD_H
#define CONNECT_THREAD_H

extern int m2m_connect_socket;
extern hfthread_hande_t hfth_handle_t;
extern int socka_start_ok;
extern int sockb_start_ok;

extern int tcpb_client_sock;
extern int tcpb_client_link;
extern cyg_mutex_t tcpb_client_link_lock;

void connect_thread_client(cyg_addrword_t p);
void connect_thread_client_b(cyg_addrword_t p);
int HSF_API start_socketb(uint32_t uxpriority);
int HSF_API start_socketa(uint32_t uxpriority);

#endif

