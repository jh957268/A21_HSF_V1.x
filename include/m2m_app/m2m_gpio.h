#ifndef M2M_GPIO_H
#define M2M_GPIO_H

#include "m2m_head.h"

#ifdef CONFIG_M2M_HW_SMT
#define M2M_HW_SMT
#endif

#define M2M_GPIO_DIR0_REG			0x0000
#define M2M_GPIO_DIR1_REG			0x0004
#define M2M_GPIO_DIR2_REG			0x0008

#define M2M_GPIO_DATA0_REG		0x0020
#define M2M_GPIO_DATA1_REG		0x0024
#define M2M_GPIO_DATA2_REG		0x0028

#define M2M_GPIO_DSET0_REG		0x0030
#define M2M_GPIO_DSET1_REG		0x0034
#define M2M_GPIO_DSET2_REG		0x0038

#define M2M_GPIO_DCLR0_REG		0x0040
#define M2M_GPIO_DCLR1_REG		0x0044
#define M2M_GPIO_DCLR2_REG		0x0048


#ifndef M2M_HW_SMT
#define M2M_GPIO_NREADY		35
#define M2M_GPIO_RELOAD		34
#define M2M_GPIO_NLINK			38
#define M2M_GPIO_RTS			0		
#define M2M_GPIO_CTS			37
#else
#define M2M_GPIO_NREADY		35
#define M2M_GPIO_RELOAD		34
#define M2M_GPIO_NLINK			38
#define M2M_GPIO_RTS			0
#define M2M_GPIO_CTS			37
#endif

#define M2M_GPIO3	8		// TXD
#define M2M_GPIO4	10		// RXD,  only input
#define M2M_GPIO5	7		// RTS
#define M2M_GPIO6	9		// CTS
#define M2M_GPIO8	11		// nLink
#define M2M_GPIO9	1		// nReady
#define M2M_GPIO10	2		// nReload,  only input

#define M2M_GPIO_IN				0x0
#define M2M_GPIO_OUT			0x1
#define M2M_GPIO_OUTHIGH		0x11
#define M2M_GPIO_OUTLOW		0x12
#define M2M_GPIO_OUTPWM		0x13
#define M2M_GPIO_OUTSW		0x14
#define M2M_GPIO_GET			0x2



#define M2M_GPIO_I2C_SD		5
#define M2M_GPIO_I2C_SCLK	4
#define M2M_GPI_RELOAD		34
#define M2M_GPIO_NREADY		35
#define M2M_GPIO_NLINK		38
#define M2M_GPIO_GPIO_0		11
#define M2M_GPIO_GPIO_1		37

#define M2M_GPIO_UART_TXD1		45
#define M2M_GPIO_UART_RXD1		46

#define M2M_GPIO_UART_TXD0		12
#define M2M_GPIO_UART_RXD0		13



struct STRGPIO
{
	int acted;
	int gpio;
	int m_state;		// main state, 
	int n_state;		// now is high or low
	int high_tm;
	int low_tm;
	int tm_tmp;
};

extern struct STRGPIO m2m_strgpio3, m2m_strgpio4, m2m_strgpio5, m2m_strgpio6, m2m_strgpio8, m2m_strgpio9, m2m_strgpio10;


void m2m_gpio_init(void);
void m2m_set_gpio_high(int gpio);
void m2m_set_gpio_low(int gpio);
void m2m_set_gpio_dir_out(int gpio);
void m2m_set_gpio_dir_in(int gpio);

int m2m_get_gpio(int gpio);
void m2m_gpio_thread(cyg_addrword_t p);
int m2m_gpio_pars(char *cmd, int len);
void m2m_gpio_state(int gpio, char *ret);

#endif

