#ifndef UART_CMD_WANN_H
#define UART_CMD_WANN_H

void cmd_deal_wann(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_fwan(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_fwangw(char *cmd_line, int len, int op_code, int para_pos, char *rep);

#endif

