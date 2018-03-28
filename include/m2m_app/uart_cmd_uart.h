#ifndef UART_CMD_UART_H
#define UART_CMD_UART_H

void cmd_deal_uart(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_fbaud(char *cmd_line, int len, int op_code, int para_pos, char *rep);

#endif

