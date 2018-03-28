
#ifndef _M2M_DEBUG_H_H_
#define _M2M_DEBUG_H_H_

#define ENABLE_M2M_DEBUG

void m2m_printf(const char *fmt,...);
int m2m_debug_get_log_msg(char **plog);
void m2m_debug_clear_log(void);

#ifdef CONFIG_M2M_DEBUG
#define M2M_PRINTF	diag_printf
#else
#define M2M_PRINTF	m2m_printf	
#endif

#ifdef ENABLE_M2M_DEBUG
#define M2M_DEBUG(a)	M2M_PRINTF a	
#else
#define M2M_DEBUG(a)
#endif

//#define M2M_LOG(a)		M2M_PRINTF a
#define M2M_LOG(__a__)	 	do{M2M_PRINTF("[%d]",(uint32_t)(cyg_current_time()*10)) ;M2M_PRINTF __a__;}while(0)
#define M2M_ERROR(__a__)	M2M_PRINTF("[ERROR:%s %d]",__FUNCTION__,__LINE__);M2M_PRINTF __a__
#define M2M_WARN(__a__)	M2M_PRINTF("[WARN: %s %d]",__FUNCTION__,__LINE__);M2M_PRINTF __a__

#ifdef CONFIG_M2M_DEBUG
#define M2M_DEBUGONLY(a)	M2M_PRINTF a
#else
#define M2M_DEBUGONLY(a)
#endif

extern cyg_mutex_t m2m_debug_mutex;
extern char msg_buff[1024*2];
extern int init_debug;

#endif

