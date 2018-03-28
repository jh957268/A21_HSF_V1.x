#ifndef F_SETTING_H
#define F_SETTING_H

struct _FSETTING
{
	char feild[20];
	char val[70];
};

int f_setting_get(char *feild, char *val);
int f_setting_set(char *feild, char *val);
void f_setting_save();
void f_setting_clear();

#endif

