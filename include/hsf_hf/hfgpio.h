
/* hfgpio.h
 *
 * Copyright (C) 2013-2020 ShangHai High-flying Electronics Technology Co.,Ltd.
 *				  	All rights reserved.
 *
 * This file is part of HSF.
 *
 *
 */

#ifndef _HF_GPIO_H_H_H_
#define _HF_GPIO_H_H_H_

typedef void (*hfgpio_interrupt_func)(uint32_t,uint32_t);
//gpio function define

#define HFM_TYPE_LPB100	0
#define HFM_TYPE_LPT100	1
#define HFM_TYPE_LPT200	2
#define HFM_TYPE_A21	3

#define HFM_PIN_NUMBER		(26)

#define HFM_MAX_FUNC_CODE	(HFM_PIN_NUMBER*2)

enum HF_GPIO_FUNC_E
{
	//////fix/////////////////////
	HFGPIO_F_JTAG_TCK=0,
	HFGPIO_F_JTAG_TDO=1,
	HFGPIO_F_JTAG_TDI,			// 2
	HFGPIO_F_JTAG_TMS,			// 3
	HFGPIO_F_USBDP,				// 4
	HFGPIO_F_USBDM,				// 5
	HFGPIO_F_UART0_TX,			// 6
	HFGPIO_F_UART0_RTS,			// 7
	HFGPIO_F_UART0_RX,			// 8
	HFGPIO_F_UART0_CTS,			// 9
	HFGPIO_F_SPI_MISO,			// 10
	HFGPIO_F_SPI_CLK,			// 11
	HFGPIO_F_SPI_CS,			// 12
	HFGPIO_F_SPI_MOSI,			// 13
	HFGPIO_F_UART1_TX,			// 14
	HFGPIO_F_UART1_RTS,			// 15
	HFGPIO_F_UART1_RX,			// 16
	HFGPIO_F_UART1_CTS,			// 17
	////////////////////////////////
	HFGPIO_F_NLINK,				// 18
	HFGPIO_F_NREADY,			// 19
	HFGPIO_F_NRELOAD,			// 20
	HFGPIO_F_SLEEP_RQ,			// 21
	HFGPIO_F_SLEEP_ON,			// 22
	HFGPIO_F_WPS,				// 23
	HFGPIO_F_IR,				// 24
	HFGPIO_F_GPIO_0,			// 25
	HFGPIO_F_I2C_SD,			// 26
	HFGPIO_F_I2C_SCLK,			// 27
	HFGPIO_F_GPIO_1,			// 28
	HFGPIO_F_USER_DEFINE		// 29
};

#define F_GPI			(0x00010000)
#define F_GPO			(0x00020000)
//GND
#define F_GND			(0x00040000)
//use to Peripherals interface
#define F_PI			(0x00080000)
//vcc
#define F_VCC			(0x00100000)
#define F_NC			(0x00200000)
//use to system reset
#define F_RST			(0x00400000)
//use to interrupt input pin
#define F_IT			(0x00800000|F_GPI)

#define F_GPIO			(F_GPI|F_GPO)

#define F_PWM			(0x01000000)

#define F_ADC			(0x02000000)

#define HFM_NOPIN			(0)
#define HFM_PIN1			(F_VCC|1)
#define HFM_PIN2			(F_VCC|2)
#define HFM_PIN3			(F_GND|3)
#define HFM_PIN4			(F_PI|F_GPIO|F_IT|4)
#define HFM_PIN5			(F_PI|F_GPIO|F_IT|5)
#define HFM_PIN6			(F_GPIO|F_IT|6)
#define HFM_PIN7			(F_NC|7)
#define HFM_PIN8			(F_PI|8)
#define HFM_PIN9			(F_PI|9)
#define HFM_PIN10			(F_PI|10)
#define HFM_PIN11			(F_PI|11)
#define HFM_PIN12			(F_PI|12)
#define HFM_PIN13			(F_PI|13)
#define HFM_PIN14			(F_GPIO|F_IT|14)
#define HFM_PIN15			(F_RST|15)
#define HFM_PIN16			(F_GPIO|F_IT|16)
#define HFM_PIN17			(F_GPIO|F_IT|17)
#define HFM_PIN18			(F_GND|18)
#define HFM_PIN19			(F_PI|F_GPIO|F_IT|19)
#define HFM_PIN20			(F_PI|F_GPIO|F_IT|20)
#define HFM_PIN21			(F_PI|F_GPIO|F_IT|21)
#define HFM_PIN22			(F_PI|F_GPIO|F_IT|22)
#define HFM_PIN23			(F_GPIO|F_IT|23)
#define HFM_PIN24			(F_GND|24)
#define HFM_PIN25			(F_PI|25)
#define HFM_PIN26			(F_GND|26)



#define HF_M_PINNO(_pin)	((_pin)&0xFF)

#define HFM_VALID_PINNO(_pinno)	((_pinno)>0&&(_pinno)<=HFM_PIN_NUMBER)

#define HF_M_PIN(_no)		HFM_PIN##_no

#define HFM_PIN_NUM		(26)

/*  Default pin configuration (no attribute). */
#define HFPIO_DEFAULT               (0u << 0)
/*  The internal pin pull-up is active. */
#define HFPIO_PULLUP                  (1u << 0)
#define HFPIO_PULLDOWN                  (1u << 1)


/*  Low level interrupt is active */
#define	HFPIO_IT_LOW_LEVEL          (1u<<0)
/*  High level interrupt is active */
#define HFPIO_IT_HIGH_LEVEL	    (1u<<1)
/*  Falling edge interrupt is active */
#define HFPIO_IT_FALL_EDGE            (1u<<2)
/*  Rising edge interrupt is active */
#define HFPIO_IT_RISE_EDGE             (1u<<3)
/*Interrupt Edge detection is active.*/
#define HFPIO_IT_EDGE			    (1u<<4)


#define HFPIO_TYPE_Pos                27
/* PIO Type Mask */
#define HFPIO_TYPE_Msk                (0xFu << HFPIO_TYPE_Pos)

/*   The pin is an input. */
#define   HFM_IO_TYPE_INPUT       (0x01 << HFPIO_TYPE_Pos)
/*   The pin is an output and has a default level of 0.*/
#define   HFM_IO_OUTPUT_0         (0x02 << HFPIO_TYPE_Pos)
/*   The pin is an output and has a default level of 1.*/
#define   HFM_IO_OUTPUT_1         (0x04 << HFPIO_TYPE_Pos)

#define   HFPIO_DS				  (0x08 << HFPIO_TYPE_Pos)


/*MT7628  GENERALl GPIONUM */
#define HF_GPIO0 0
#define HF_GPIO1 1
#define HF_GPIO2 2
#define HF_GPIO3 3
#define HF_GPIO4 4
#define HF_GPIO5 5
#define HF_GPIO6 6
#define HF_GPIO7 7
#define HF_GPIO8 8
#define HF_GPIO9 9
#define HF_GPIO10 10
#define HF_GPIO11 11
#define HF_GPIO12 12
#define HF_GPIO13 13
#define HF_GPIO14 14
#define HF_GPIO15 15
#define HF_GPIO16 16
#define HF_GPIO17 17
#define HF_GPIO18 18
#define HF_GPIO19 19
#define HF_GPIO20 20
#define HF_GPIO21 21
#define HF_GPIO22 22
#define HF_GPIO23 23
#define HF_GPIO24 24
#define HF_GPIO25 25
#define HF_GPIO26 26 
#define HF_GPIO27 27
#define HF_GPIO28 28
#define HF_GPIO29 29
#define HF_GPIO30 30
#define HF_GPIO31 31
#define HF_GPIO32 32
#define HF_GPIO33 33
#define HF_GPIO34 34
#define HF_GPIO35 35
#define HF_GPIO36 36
#define HF_GPIO37 37
#define HF_GPIO38 38
#define HF_GPIO39 39
#define HF_GPIO40 40
#define HF_GPIO41 41
#define HF_GPIO42 42
#define HF_GPIO43 43
#define HF_GPIO44 44
#define HF_GPIO45 45
#define HF_GPIO46 46
#define HF_GPIO47 47




#define M2M_GPIO_DIR0_REG		0x0000
#define M2M_GPIO_DIR1_REG		0x0004

#define M2M_GPIO_DATA0_REG		0x0020
#define M2M_GPIO_DATA1_REG		0x0024


#define M2M_GPIO_DSET0_REG		0x0030
#define M2M_GPIO_DSET1_REG		0x0034


#define M2M_GPIO_DCLR0_REG		0x0040
#define M2M_GPIO_DCLR1_REG		0x0044




#define M2M_GPIO_IT_LOW_LEVEL0_REG	 0x0080
#define M2M_GPIO_IT_LOW_LEVEL1_REG	 0x0084
#define M2M_GPIO_IT_LOW_LEVEL2_REG	 0x0088


#define M2M_GPIO_IT_HIGH_LEVEL0_REG	 0x0070
#define M2M_GPIO_IT_HIGH_LEVEL1_REG	 0x0074
#define M2M_GPIO_IT_HIGH_LEVEL2_REG	 0x0078


#define M2M_GPIO_IT_FALL_EDGE0_REG	 0x0060  
#define M2M_GPIO_IT_FALL_EDGE1_REG	 0x0064  
#define M2M_GPIO_IT_FALL_EDGE2_REG	 0x0068 


#define M2M_GPIO_IT_RISE_EDGE0_REG	 0x0050
#define M2M_GPIO_IT_RISE_EDGE1_REG	 0x0054
#define M2M_GPIO_IT_RISE_EDGE2_REG	 0x0058





//�ڲ�ʹ�ú���


void HSF_IAPI SetGpioOutput(int gpio);
void HSF_IAPI SetGpioInput(int gpio);
void HSF_IAPI SetGpioNoPull(int gpio);
void HSF_IAPI SetGpioPullUp(int gpio);
void HSF_IAPI SetGpioPullDown(int gpio);
void HSF_IAPI Set_Gpio_High(int gpio);
void HSF_IAPI Set_Gpio_Low(int gpio);
void HSF_IAPI SetGpioNoInputOutput(int gpio);
int HSF_IAPI Get_Gpio_Status(int gpio);

//void HSF_IAPI Set_Gpio_DS(int bank, int pin, int enable);




/*
 *���ݹ���������PIN
 *return value:
 *@HF_SUCCESS:���óɹ�
 *@HF_E_INVAL: fid�Ƿ�,��������Ӧ��PIN�ŷǷ�
 *@HF_FAIL:����ʧ��
 *@HF_E_ACCES:Ҫ���õ�PIN�������ó�ֱ����flags������GND,VCC PIN��,
			  ֻ��������ĽŲ������ó������.
*/
int HSF_API hfgpio_configure_fpin(int fid,int flags);

int HSF_API hfgpio_fconfigure_get(int fid);

int HSF_API hfgpio_fpin_add_feature(int fid,int flags);

int HSF_API hfgpio_fpin_clear_feature(int fid,int flags);

/*
 *���ݹ��������ö�ӦPINΪ����ߵ�ƽ
 *return value:
 *@HF_SUCCESS:���óɹ�
 *@HF_E_INVAL: fid�Ƿ�,��������Ӧ��PIN�ŷǷ�
 *@HF_FAIL:����ʧ��
*/
//int hfgpio_fset_out_high(int fid);
#define hfgpio_fset_out_high(__fid)	hfgpio_configure_fpin(__fid,HFPIO_DEFAULT|HFM_IO_OUTPUT_1)
/*
 *���ݹ��������ö�ӦPINΪ����͵�ƽ
 *return value:
 *@HF_SUCCESS:���óɹ�
 *@HF_E_INVAL: fid�Ƿ�,��������Ӧ��PIN�ŷǷ�
 *@HF_FAIL:����ʧ��
*/

//int hfgpio_fset_out_low(int fid);
#define hfgpio_fset_out_low(__fid)	hfgpio_configure_fpin(__fid,HFPIO_DEFAULT|HFM_IO_OUTPUT_0)


/*
 *����PIN��ID����PIN(�ڲ�ʹ�ã��ⲿ��ʹ��hfgpio_configure_fpin)
 *flags:
		HFPIO_IT_LOW_LEVEL:�͵�ƽ����
		HFPIO_IT_HIGH_LEVEL:�ߵ�ƽ����
		HFPIO_IT_FALL_EDGE:�½��ش���
		HFPIO_IT_RISE_EDGE:�����ش���
		HFPIO_IT_EDGE:���ش���
 *handle:
		�жϴ�����ʱ����ã��������治������ʱ������ʱ������ܶ� 		
 *return value:
 *@HF_SUCCESS:���óɹ�
 *@HF_E_INVAL: pid�Ƿ�
 *@HF_FAIL:����ʧ��
 *@HF_E_ACCES:Ҫ���õ�PIN�������ó��жϣ�PIN�Ų��߱�F_IT����,
*/
int hfgpio_configure_fpin_interrupt(int fid,uint32_t flags,hfgpio_interrupt_func handle,int enable);

/*
 *ʹ���ж�,��ʹ���ж�֮ǰһ��Ҫ�ȵ���hfgpio_configure_fpin_interrupt
 *return value:
 *@HF_SUCCESS:���óɹ�
 *@HF_E_INVAL: fid�Ƿ�,��������Ӧ��PIN�ŷǷ�
 *@HF_FAIL:����ʧ��
*/
int hfgpio_fenable_interrupt(int fid);

/*
 *��ֹ�ж�
 *return value:
 *@HF_SUCCESS:���óɹ�
 *@HF_E_INVAL: fid�Ƿ�,��������Ӧ��PIN�ŷǷ�
 *@HF_FAIL:����ʧ��
*/
int hfgpio_fdisable_interrupt(int fid);

/*
 *��ֹ���е�GPIO �ж�
 *return value:
 *@HF_SUCCESS:���óɹ�
 *@HF_FAIL:����ʧ��
*/
int HSF_API hfgpio_disabel_all_interrupt(void);

/*
 *�������ӦPIN�Ƿ�Ϊ�ߵ�ƽ
 *return value:
 *@>=1:�ߵ�ƽ
 *@<=0:�͵�ƽ,����fid�Ƿ���fid��Ӧ��pin��û�ж�����߲���gpio
*/
int hfgpio_fpin_is_high(int fid);

/*
 *��鹦���뵽PIN��ӳ���Ϸ��У�HFGPIO_F_JTAG_TCK~HFGPIO_F_UART1_CTS��ֻ�����ù̶�
 *��PIN�ţ�����HFM_NOPIN,һ��PIN��ֻ�ܶ�Ӧһ�������롣
 *return value:
 *@1:�̶�������ӳ���PIN�Ŵ���
 *@2:���д��������������Ӧͬһ��PIN��
 *@0:�Ϸ�
*/
int hfgpio_fmap_check(int type);

//#ifdef __BUILD_HSF_SDK__

/*
 *����PIN��ID����PIN(�ڲ�ʹ�ã��ⲿ��ʹ��hfgpio_configure_fpin)
 *return value:
 *@HF_SUCCESS:���óɹ�
 *@HF_E_INVAL: pid�Ƿ�
 *@HF_FAIL:����ʧ��
 *@HF_E_ACCES:Ҫ���õ�PIN�������ó�ֱ����flags������GND,VCC PIN��,
			  ,ֻ��������ĽŲ������ó������.
*/
int HSF_IAPI hfgpio_configure_pin(int pid,int flags);


#define  hfgpio_set_out_high(pid)	hfgpio_configure_pin(pid,HFPIO_DEFAULT|HFM_IO_OUTPUT_1)

#define  hfgpio_set_out_low(pid)	hfgpio_configure_pin(pid,HFPIO_DEFAULT|HFM_IO_OUTPUT_0)

int HSF_IAPI hfgpio_get_pid(int pin_no);

void HSF_IAPI hfgpio_init_gpio_pin(void);

int HSF_IAPI hfgpio_pin_is_high(int pid);

//#endif


//PWM
int HSF_API  hfgpio_pwm_disable(int fid);

int HSF_API hfgpio_pwm_enable(int fid, int freq, int hrate);

//ADC
int HSF_API hfgpio_adc_enable(int fid);

int  HSF_API  hfgpio_adc_get_value(int fid);

void HSF_IAPI GpioInterruptInit(void);



#endif

