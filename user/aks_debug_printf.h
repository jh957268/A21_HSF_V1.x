#ifndef __AKS_DEBUG_PRINTF__
#define __AKS_DEBUG_PRINTF__
#include <stdio.h>

#if 0
#define eprintf(fmt, args...) do {
							sprintf (ebuffer, fmt, args);
							Joo_uart_send((char *)ebuffer);
						}

#endif

#define E_DEBUG_PRINT	0

#if E_DEBUG_PRINT
static char aks_print_buffer[128];
#define eprintf(fmt, ...) do {  \
	                               sprintf(aks_print_buffer, fmt, ## __VA_ARGS__); \
	                               Joo_uart_send((char *)aks_print_buffer); \                               
	                             } while (0)
#else
#define eprintf(fmt, ...)
#endif


#endif
