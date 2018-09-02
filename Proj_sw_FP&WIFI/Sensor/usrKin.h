#ifndef __USRKIN_H_
#define __USRKIN_H_

#include "STC15Fxxxx.H"
#include "typeDf.h"

sbit Dcode0		= P5^4;
sbit Dcode1 	= P2^7;
sbit Dcode2 	= P0^3;
sbit Dcode3 	= P3^2;
sbit Dcode4 	= P0^0;
sbit Dcode5 	= P0^1;

sbit Usr_key 	= P2^4;

#define Dcode_FLG_ifAP					0x01
#define Dcode_FLG_ifLED					0x02
#define Dcode_FLG_ifNONC				0x02
#define Dcode_FLG_saveRelay_MODEX		0x0C			
#define Dcode_FLG_swDelay				0x30

#define Dcode_cntTim(x)			((x & 0x30) >> 4) //延时时间
#define Dcode_modeMix(x)		((x & 0x0C) >> 2) //记忆与延时，合并读取方便比较延时模式下，记忆无意义

typedef void *funKey_Callback(void);

typedef enum{

	press_Null = 1,
	press_Short,
	press_Long,
}keyCfrm_Type;

bit UsrKEYScan_oneShoot(void);
u8 DcodeScan_oneShoot(void);

void DcodeScan(void);
void UsrKEYScan(funKey_Callback funCB_Short, funKey_Callback funCB_LongA, funKey_Callback funCB_LongB);

#endif