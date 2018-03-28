#ifndef UART_CMD_WSCAN_H
#define UART_CMD_WSCAN_H

void cmd_deal_wscan(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_wscanb(char *cmd_line, int len, int op_code, int para_pos, char *rep);

#endif

