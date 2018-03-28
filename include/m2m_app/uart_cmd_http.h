
#ifndef __UART_CMD_HTTP__
#define __UART_CMD_HTTP__


#include "uart_cmd.h"
#include "uart_thread.h"

			
void cmd_deal_httpurl(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_httptp(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_httpph(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_httpcn(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_httpua(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_httpdt(char *cmd_line, int len, int op_code, int para_pos, char *rep);

#endif
