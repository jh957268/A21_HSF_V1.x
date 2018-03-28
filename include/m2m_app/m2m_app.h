
#ifndef _M2M_APP_H_H_
#define _M2M_APP_H_H_


#define STACK_SIZE 0x1000
#define M2M_THREAD_PRI_HIGH		3
#define M2M_THREAD_PRI_MID		6
#define M2M_THREAD_PRI_LOW		14



int m2m_start_is_ok(void);


//http update
int  m2m_http_remote_update(const char *url);

//
typedef int (*uart_read_callback)(char *data,int len);

void m2m_uart_set_read_callback(uart_read_callback p_callback);

//uart 
int m2m_uart_write1(char *buf, int len);

//m2m string
int atoi16(const char *str);

int m2m_udpate_write_config_to_flash(char *file,int len);


#endif

