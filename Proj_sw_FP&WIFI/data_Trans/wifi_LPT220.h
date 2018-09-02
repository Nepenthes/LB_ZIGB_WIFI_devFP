#ifndef __WIFI_LPT220_H_
#define __WIFI_LPT220_H_

#include "STC15Fxxxx.H"

#define WIFI_funcPIN_nRst		P17
#define WIFI_funcPIN_nReload	P12
#define WIFI_tipsPIN_nReady		P13

#define WIFI_pinEN		0
#define WIFI_pinDISEN	1

typedef struct{

	u8	*wifiInit_reqCMD;
	u8 	*wifiInit_REPLY[2];
	u8	REPLY_DATLENTAB[2];
	u16	timeTab_waitAnsr;
}datsAttr_wifiInit;

typedef enum{

	statusMode_Reset = 1,
	statusMode_smartLink,
	statusMode_ftRecover,
	statusMode_standBy,
	statusMode_apSwitch,
	statusMode_NULL,	//用作切换其他状态指示
}status_Mode;

extern status_Mode	SYS_status_Mode;

void WIFI_pinInit(void);
void WIFI_hwRestoreFactory(void);
void WIFI_hwSmartLink(void);
void rxBuff_WIFI_Clr(void);
bit WIFI_ENTM_OUT(unsigned char repeatLoop);
bit cmdAT_validation(		  char *cmd, 
							  char *reply[2], 
					 unsigned char replyLen[2], 
					 unsigned int  timeWait,
					 unsigned char repeatLoop);	 
bit cmdPackAT_validation(datsAttr_wifiInit cmdPack, unsigned char repeatLoop);
					 
bit WIFI_configInit(void);
bit WIFI_WMODE_CHG(bit ifAP);
bit WIFI_serverUDP_CHG(unsigned char ip[4]);

#endif