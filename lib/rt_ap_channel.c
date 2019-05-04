/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    rt_main_end.c

    Abstract:
    Create and register network interface.

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#include "rt_config.h"

void Joo_uart_send(char *data);

static char ecosbuffer[128];
#define eprintf(fmt, ...) do {  \
	                               sprintf(ecosbuffer, fmt, ## __VA_ARGS__); \
	                               Joo_uart_send((char *)ecosbuffer); \                               
	                             } while (0)

/*
========================================================================
Routine Description:
    Close raxx interface.

Arguments:
	*net_dev			the raxx interface pointer

Return Value:
    0					Open OK
	otherwise			Open Fail

Note:
	1. if open fail, kernel will not call the close function.
	2. Free memory for
		(1) Mlme Memory Handler:		MlmeHalt()
		(2) TX & RX:					RTMPFreeTxRxRingMemory()
		(3) BA Reordering: 				ba_reordering_resource_release()
========================================================================
*/
int rt28xx_get_wifi_channel(IN VOID *dev)
{
	PNET_DEV net_dev = (PNET_DEV)dev;
	RTMP_ADAPTER	*pAd = NULL;
	UCHAR ch;
	unsigned int *ptmp, macVersionAddr;;
	int i, j;
	
	/* Sanity check for pAd */
	//pAd = (PRTMP_ADAPTER) (net_dev->driver_private);
	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(net_dev);	
	if (pAd == NULL)
		return -1; /* close ok */
	ptmp = (unsigned int *)pAd;
	//ch = pAd->CommonCfg.Channel;
	//ch = pAd->LatchRfRegs.Channel;
	ch = pAd->OpMode;
	eprintf("pAd Addr=%08x\n", (unsigned int)pAd);
	eprintf("OpMode = %d\n", (int)ch);
	eprintf("CSRBaseAddress=%08x\n", (unsigned int)(pAd->CSRBaseAddress));
	eprintf("MACVersion=%08x\n", (unsigned int)(pAd->MACVersion));
	eprintf("MACVersion Addr=%08x\n", (unsigned int)&(pAd->MACVersion));
	macVersionAddr = (unsigned int)&(pAd->MACVersion);
	eprintf("Intftype=%d\n", (unsigned int)(pAd->infType));
	
	for (i = 0; i < 32; i++)
	{
		for (j = 0; j < 8; j++)
		{
			eprintf("%08x ", *ptmp);
			ptmp++;
		}
		eprintf("\n");
	}
	ptmp = (unsigned int *)(macVersionAddr - 1024);
	for (i = 0; i < 64; i++)
	{
		for (j = 0; j < 8; j++)
		{
			eprintf("%08x ", *ptmp);
			ptmp++;
		}
		eprintf("\n");
	}	
	pAd = (RTMP_ADAPTER	*)((unsigned int)pAd - 0x18);
	eprintf("MACVersion=%08x\n", (unsigned int)(pAd->MACVersion));
	eprintf("MACVersion Addr=%08x\n", (unsigned int)&(pAd->MACVersion));
	eprintf("Intftype=%d\n", (unsigned int)(pAd->infType));
	//ch = AP_AUTO_CH_SEL(pAd, 2);
	//ch = AP_AUTO_CH_SEL(pAd, pAd->ApCfg.AutoChannelAlg);
	//pAd->chipOps.ChipSwitchChannel(pAd, ch, FALSE);
	return((int)ch);
}


