#ifndef UART_CMD_LANN_H
#define UART_CMD_LANN_H

void cmd_deal_lann(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_dhcpgw(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_langw(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_flan(char *cmd_line, int len, int op_code, int para_pos, char *rep);
//void cmd_deal_fgw(char *cmd_line, int len, int op_code, int para_pos, char *rep);

#endif

