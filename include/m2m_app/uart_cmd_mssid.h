#ifndef UART_CMD_MSSID_H
#define UART_CMD_MSSID_H

void cmd_deal_massid(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_wakeyb(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_wakeyc(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_wakeyd(char *cmd_line, int len, int op_code, int para_pos, char *rep);
void cmd_deal_wahd(char *cmd_line, int len, int op_code, int para_pos, char *rep);

#endif

