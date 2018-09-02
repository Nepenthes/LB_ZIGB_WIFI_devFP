#ifndef __DATATRANS_H_
#define __DATATRANS_H_

#include "STC15Fxxxx.H"
#include "USART.h"

#define BAUD_WIFI	 19200UL   //���ڲ�����->WIFIģ��ͨѶ
#define BAUD_FP		 57600UL   //���ڲ�����->ָ��ģ��ͨѶ

#define rxBuff_WIFI	RX1_Buffer	 
#define rxBuff_FP	RX2_Buffer	

#define FRAME_HEAD_HEARTBEAT	0xAA
#define FRAME_HEAD_MOBILE		0xAA
#define FRAME_HEAD_SERVER		0xCC

#define dataTransLength_objMOBILE			33
#define dataTransLength_objSERVER			45
#define dataHeartBeatLength_objSERVER		14

#define FRAME_TYPE_MtoS_CMD					0xA0	/*��������*///�ֻ�������
#define FRAME_TYPE_StoM_RCVsuccess			0x0A	/*��������*///�������ֻ�
#define FRAME_TYPE_StoM_RCVfail				0x0C
#define FRAME_TYPE_StoM_upLoad				0x0D
#define FRAME_TYPE_StoM_reaptSMK			0x0E

#define FRAME_HEARTBEAT_cmdOdd				0x23	/*����*///����������
#define FRAME_HEARTBEAT_cmdEven				0x22	/*����*///ż��������

#define FRAME_MtoSCMD_cmdControl			0x10	/*����*///����
#define FRAME_MtoSCMD_cmdFPLearn			0x26	/*����*///ָ��ѧϰ
#define FRAME_MtoSCMD_cmdFPDelete			0x29	/*����*///ָ��ɾ��
#define FRAME_MtoSCMD_cmdFPQuery			0x2A	/*����*///ָ�Ʋ�ѯ
#define FRAME_MtoSCMD_cmdConfigSearch		0x09	/*����*///��������
#define FRAME_MtoSCMD_cmdQuery				0x11	/*����*///���ò�ѯ
#define FRAME_MtoSCMD_cmdInterface			0x15	/*����*///���ý���
#define FRAME_MtoSCMD_cmdReset				0x16	/*����*///��λ
#define FRAME_MtoSCMD_cmdLockON				0x17	/*����*///����
#define FRAME_MtoSCMD_cmdLockOFF			0x18	/*����*///����
#define FRAME_MtoSCMD_cmdswTimQuery			0x19	/*����*///��ͨ���ض�ʱ��ѯ
#define FRAME_MtoSCMD_cmdperssionTimQuery	0x32	/*����*///ָ��Ȩ�޶�ʱ��ѯ
#define FRAME_MtoSCMD_cmdConfigAP			0x50	/*����*///AP����
#define FRAME_MtoSCMD_cmdBeepsON			0x1A	/*����*///����ʾ��
#define FRAME_MtoSCMD_cmdBeepsOFF			0x1B	/*����*///����ʾ��
#define FRAME_MtoSCMD_cmdftRecoverRQ		0x22	/*����*///��ѯ�Ƿ�֧�ָֻ�����
#define FRAME_MtoSCMD_cmdRecoverFactory		0x1F	/*����*///�ָ�����
#define FRAME_MtoSCMD_cmdCfg_swTim			0x14	/*����*///��ͨ���ض�ʱ
#define FRAME_MtoSCMD_cmdCfg_permissionTim	0x31	/*����*///Ȩ�޶�ʱ

#define	cmdConfigTim_normalSwConfig			0xA0	/*����1*///��ͨ���ض�ʱ��ʶ-����1

#define FRAME_StoMRPY_rpyLearnSuccess		0x27	/*�ظ�*///ѧϰ�ɹ�
#define FRAME_StoMRPY_rpyLearnFail			0x28	/*�ظ�*///ѧϰʧ��

typedef struct{

	u8 rcvDats[COM_TX1_Lenth];
	u8 rcvDatsLen;
}uartTout_datsRcv;
	
typedef enum{

	DATATRANS_objFLAG_SERVER = 0,
	DATATRANS_objFLAG_MOBILE,
	HEARTBEAT_objFLAG_SERVER,
	dataTrans_obj_NULL,
}dataTrans_obj;

extern uartTout_datsRcv xdata datsRcv_ZIGB;
extern bit 				uartRX_toutFLG;

void rxBuff_WIFI_Clr(void);

void uartObjWIFI_Init(void);
void uartObjFP_Init(void);

void uartObjWIFI_Send_Byte(u8 dat);
void uartObjWIFI_Send_String(char *s,unsigned char ucLength);

void uartObjFP_Send_Byte(u8 dat);
u8 UART_FP_byteRcv(void);
void uartObjFP_Send_String(char *s,unsigned char ucLength);

unsigned char frame_Check(unsigned char frame_temp[], u8 check_num);
u8 uartObjFP_Receive_Byte(void);
void dtasTX_loadBasic_AUTO(u8 dats_Tx[45],		/*�Զ���װ*///���Ͱ����ݻ��������Ϣ��װ
						   u8 frame_Type,
						   u8 frame_CMD,
						   bit ifSpecial_CMD);
void dtasTX_loadBasic_CUST(dataTrans_obj obj_custom,	/*�Զ�����װ*///���Ͱ����ݻ��������Ϣ��װ
						   u8 dats_Tx[45],		
						   u8 frame_Type,
						   u8 frame_CMD,
						   bit ifSpecial_CMD);

void thread_LocalStaus_Release(void);
void thread_dataTrans(void);

#endif