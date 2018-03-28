#ifndef LISTENING_THREAD_H
#define LISTENING_THREAD_H



//int m2m_MaxSocket;
typedef struct _M2M_CLIENT_SOCKET
{
	int fd;
	uint32_t time_to;
	struct sockaddr_in addr;
}M2M_CLIENT_SOCKET;


#define DEFAULT_MAX_SOCK_CLIENT	32

extern M2M_CLIENT_SOCKET m2m_client_sockets[DEFAULT_MAX_SOCK_CLIENT];
extern cyg_mutex_t m2m_client_socket_mutex;
extern int m2m_listening_socket;
extern int m2m_client_socket_num;

void listening_thread_server(cyg_addrword_t p);

#endif
