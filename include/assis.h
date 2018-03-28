/* 
    assis.h 
*/

#ifndef ASSIS_H
#define ASSIS_H
#include <cyg/kernel/kapi.h>
extern int apcli_connected;
extern cyg_mutex_t apcli_connected_mutex;
extern int is_apcli_enable();

extern int f_setting_get(char *feild, char *val);
extern int f_setting_set(char *feild, char *val);
extern void f_setting_save();

#endif

