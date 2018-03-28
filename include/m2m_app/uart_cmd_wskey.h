#ifndef UART_CMD_WSKEY_H
#define UART_CMD_WSKEY_H

void cmd_deal_wskey(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_wskey1(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_wskey2(char *cmd_line, int len, int op_code, int para_pos, char *rep);

#endif

