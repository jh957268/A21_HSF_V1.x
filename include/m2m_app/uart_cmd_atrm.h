#ifndef UART_CMD_ATRM_H
#define UART_CMD_ATRM_H

void cmd_deal_atrm(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_fnetp(char *cmd_line, int len, int op_code, int para_pos, char *rep);

#endif

