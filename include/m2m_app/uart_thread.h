#ifndef UART_THREAD_H
#define UART_THREAD_H



cyg_io_handle_t m2m_uart_fd;
cyg_io_handle_t m2m_uart1_fd;

int m2m_cmd_mode_echo;
int m2m_uartte;
#define M2M_UARTTE_NORMAL		0
#define M2M_UARTTE_FAST		1

#define M2M_MAXUARTFRAME 4*1024
//char m2m_uart_recv[M2M_MAXUARTFRAME];

char *m2m_uart_recv;
hfthread_hande_t hfth_handle_uart;

void m2m_uart_thread(cyg_addrword_t p);
int start_uart(uint32_t uxpriority);
int start_uart_ex(uint32_t uxpriority, int stack_size);
int netsocka_send(char *ptr, int nbytes);

//int m2m_getout_through_mode(char *buf, int *num);

#endif

