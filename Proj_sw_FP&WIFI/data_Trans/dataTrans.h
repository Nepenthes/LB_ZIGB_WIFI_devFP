#ifndef __DATATRANS_H_
#define __DATATRANS_H_

#include "STC15Fxxxx.H"
#include "USART.h"

#define BAUD_WIFI	 19200UL   //串口波特率->WIFI模块通讯
#define BAUD_FP		 57600UL   //串口波特率->指纹模块通讯

#define rxBuff_WIFI	RX1_Buffer	 
#define rxBuff_FP	RX2_Buffer	

#define FRAME_HEAD_HEARTBEAT	0xAA
#define FRAME_HEAD_MOBILE		0xAA
#define FRAME_HEAD_SERVER		0xCC

#define dataTransLength_objMOBILE			33
#define dataTransLength_objSERVER			45
#define dataHeartBeatLength_objSERVER		14

#define FRAME_TYPE_MtoS_CMD					0xA0	/*数据类型*///手机至开关
#define FRAME_TYPE_StoM_RCVsuccess			0x0A	/*数据类型*///开关至手机
#define FRAME_TYPE_StoM_RCVfail				0x0C
#define FRAME_TYPE_StoM_upLoad				0x0D
#define FRAME_TYPE_StoM_reaptSMK			0x0E

#define FRAME_HEARTBEAT_cmdOdd				0x23	/*命令*///奇数心跳包
#define FRAME_HEARTBEAT_cmdEven				0x22	/*命令*///偶数心跳包

#define FRAME_MtoSCMD_cmdControl			0x10	/*命令*///控制
#define FRAME_MtoSCMD_cmdFPLearn			0x26	/*命令*///指纹学习
#define FRAME_MtoSCMD_cmdFPDelete			0x29	/*命令*///指纹删除
#define FRAME_MtoSCMD_cmdFPQuery			0x2A	/*命令*///指纹查询
#define FRAME_MtoSCMD_cmdConfigSearch		0x09	/*命令*///配置搜索
#define FRAME_MtoSCMD_cmdQuery				0x11	/*命令*///配置查询
#define FRAME_MtoSCMD_cmdInterface			0x15	/*命令*///配置交互
#define FRAME_MtoSCMD_cmdReset				0x16	/*命令*///复位
#define FRAME_MtoSCMD_cmdLockON				0x17	/*命令*///上锁
#define FRAME_MtoSCMD_cmdLockOFF			0x18	/*命令*///解锁
#define FRAME_MtoSCMD_cmdswTimQuery			0x19	/*命令*///普通开关定时查询
#define FRAME_MtoSCMD_cmdperssionTimQuery	0x32	/*命令*///指纹权限定时查询
#define FRAME_MtoSCMD_cmdConfigAP			0x50	/*命令*///AP配置
#define FRAME_MtoSCMD_cmdBeepsON			0x1A	/*命令*///开提示音
#define FRAME_MtoSCMD_cmdBeepsOFF			0x1B	/*命令*///关提示音
#define FRAME_MtoSCMD_cmdftRecoverRQ		0x22	/*命令*///查询是否支持恢复出厂
#define FRAME_MtoSCMD_cmdRecoverFactory		0x1F	/*命令*///恢复出厂
#define FRAME_MtoSCMD_cmdCfg_swTim			0x14	/*命令*///普通开关定时
#define FRAME_MtoSCMD_cmdCfg_permissionTim	0x31	/*命令*///权限定时

#define	cmdConfigTim_normalSwConfig			0xA0	/*数据1*///普通开关定时辨识-数据1

#define FRAME_StoMRPY_rpyLearnSuccess		0x27	/*回复*///学习成功
#define FRAME_StoMRPY_rpyLearnFail			0x28	/*回复*///学习失败

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
void dtasTX_loadBasic_AUTO(u8 dats_Tx[45],		/*自动填装*///发送包数据缓存基本信息填装
						   u8 frame_Type,
						   u8 frame_CMD,
						   bit ifSpecial_CMD);
void dtasTX_loadBasic_CUST(dataTrans_obj obj_custom,	/*自定义填装*///发送包数据缓存基本信息填装
						   u8 dats_Tx[45],		
						   u8 frame_Type,
						   u8 frame_CMD,
						   bit ifSpecial_CMD);

void thread_LocalStaus_Release(void);
void thread_dataTrans(void);

#endif