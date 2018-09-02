#include "wifi_LPT220.h"

#include "dataTrans.h"
#include "datasReference.h"
#include "usrKin.h"
#include "pars_Method.h"
#include "sensor_FP.h"
#include "Tips.h"
#include "Relay.h"

#include "delay.h"
#include "eeprom.h"
#include "USART.h"
#include "soft_uart.h"

#include "string.h"
#include "stdio.h"

//*******************MAC_ID引用区*****************************/
extern u8				xdata MAC_ID[6];

//*******************拨码变量引用区***************************/
extern u8 idata 		val_DcodeCfm;

//*******************串口变量引用区***************************/
extern COMx_Define 		COM1;

//*******************指纹模块变量引用区***********************/
extern threadFP_Mode	process_FPworkMode;		//用于WMODE切换时关闭指纹模块工作

//*******************数据传输变量引用区***********************/
extern bit				deviceLock_flag;		//设备锁标志

/********************本地文件变量创建区******************/
unsigned int xdata funKey_WIFI_cnt 		= 0;	//wifi模块物理按键连接引脚模拟按键时长计数值
bit				   IF_wifiConfigCMP_FLG = 0; 	//WIFI配置初始化完成标志

status_Mode		   SYS_status_Mode		= statusMode_NULL;	//系统状态指示

/*/------------------------------------------------------/*/
#define cmdAT_packLenth 5
const datsAttr_wifiInit wifiCMD_dats[cmdAT_packLenth] = {		//顺序禁止更改！！！

	{{"AT+E\r\n"},									{"AT+E\r\n\r+ok", "+ok"},				{10,3},		500},
	{{"AT+WMODE=AP\r\n"},							{"+ok\r\n",	"ok"},						{5,2},		500},
	{{"AT+WMODE=STA\r\n"},							{"+ok\r\n",	"ok"},						{5,2},		500},
	{{"AT+Z\r\n"},									{"+ok\r\n",	"ok"},						{5,2},		500},
	
	{{"AT+NETP=UDP,SERVER,8866,192.168.5.1\r\n"},	{"+ok\r\n",	"ok"},						{5,2},		500},
};

/*********************WIFI模块控制引脚初始化******************/
void WIFI_pinInit(void){
	
	P1M1 &= 0x7F; 
	P1M0 |= 0x80;
	
	P1M1 &= 0xFB; 
	P1M0 |= 0x04;
	
	P1M1 |= 0x08; 
	P1M0 &= 0xF3;
	
	WIFI_funcPIN_nRst = WIFI_funcPIN_nReload = WIFI_pinDISEN;
}

/*********************WIFI模块硬件控制smartlink******************/
void WIFI_hwSmartLink(void){
	
	//重新解开设备锁
	{
		u8 deviceLock_IF = 0;
		
		deviceLock_flag  = 0;
		coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
	}
	
	WIFI_funcPIN_nReload = WIFI_pinEN;
	funKey_WIFI_cnt = 1000;
	LogString("WIFI模块smartlink已启动！\n");
	
	beepTips_s(3, 20, 6);
	
//	SYS_status_Mode 	= statusMode_NULL;			//系统状态更新恢复至初始空
//	process_FPworkMode	= work_Mode_umDetect;		//指纹模块工作模式恢复到主动检测模式
}

/*********************WIFI模块硬件控制恢复出厂设置******************/
void WIFI_hwRestoreFactory(void){

	WIFI_funcPIN_nReload = WIFI_pinEN;
	funKey_WIFI_cnt = 5000;
	LogString("WIFI模块恢复出厂设置！\n");
	
	relay_Act(relay_off, 0); //关闭继电器
	
	Beep_Tips(3, 8, 3, 100, 0);		//提示，语句下置，与上面动作同步进行
	delayMs(2000);
	
//	SYS_status_Mode 	= statusMode_NULL;			//系统状态更新恢复至初始空
//	process_FPworkMode	= work_Mode_umDetect;		//指纹模块工作模式恢复到主动检测模式
}

/*********************退透传******************/
bit WIFI_ENTM_OUT(unsigned char repeatLoop){
	
	u8 loopOut = repeatLoop;
	
	while(SBUF != 'a' && (-- loopOut)){
	
		uartObjWIFI_Send_Byte('+');
		delayMs(200);
	}
	
	if(!loopOut)return 0;
	else loopOut = repeatLoop;
	
	while(SBUF != 'A' && (-- loopOut)){
	
		uartObjWIFI_Send_Byte('a');
		delayMs(100);
	}
	
	if(!loopOut)return 0;
	else return 1;
	
	return 0;
}

/*********************单条AT指令下达及验证响应******************/
bit cmdAT_validation(char *cmd, char *reply[2], unsigned char replyLen[2], unsigned int timeWait, unsigned char repeatLoop){

	unsigned char loop 	= 0,
				  loopa	= 0;
	
	u16 Local_Delay = 0;		
	
	uartRX_toutFLG = 0;
	for(loop = 0; loop < repeatLoop; loop ++){
		
		Local_Delay = timeWait;	
	
		uartObjWIFI_Send_String(cmd, strlen(cmd));
		
		while(Local_Delay --){
		
			delayMs(2);
			
			if(uartRX_toutFLG){
			
				uartRX_toutFLG = 0;
				for(loopa = 0; loopa < 2; loopa ++){
				
					if(memmem(datsRcv_ZIGB.rcvDats, datsRcv_ZIGB.rcvDatsLen, reply[loopa], replyLen[loopa])){
						
						return 1;
					}	
				}	
			}		
		}
	}	
	
	return 0;
}

/*********************单条AT指令下达及验证响应——形参结构体打包******************/
bit cmdPackAT_validation(const datsAttr_wifiInit cmdPack, const u8 repeatLoop){

	return cmdAT_validation(cmdPack.wifiInit_reqCMD,
							cmdPack.wifiInit_REPLY,
							cmdPack.REPLY_DATLENTAB,
							cmdPack.timeTab_waitAnsr,
							repeatLoop);
}

/*********************WIFI模块网络参数配置初始化******************/
bit WIFI_configInit(void){
	
	u8 xdata serverIP_temp[4] 	= {0};
	u8 xdata ATCMD_temp[40]		= {0};
	
	u16 data tOut_cnt			= 5000;
	
	static u8 reInit_loop = 0;
	
	WIFI_funcPIN_nRst	= WIFI_pinEN;	//硬件复位一次
	delayMs(200);
	WIFI_funcPIN_nRst	= WIFI_pinDISEN;
	delayMs(3000);
	WIFI_funcPIN_nReload = WIFI_pinEN;
	delayMs(1000);
	WIFI_funcPIN_nReload = WIFI_pinDISEN;
	
	if(reInit_loop < 3)reInit_loop ++;
	else{
	
		reInit_loop = 0;
		WIFI_funcPIN_nReload = WIFI_pinEN;
		delayMs(7000);
		WIFI_funcPIN_nReload = WIFI_pinDISEN;
		delayMs(3000);
	}
//	WIFI_funcPIN_nReload = WIFI_pinEN;
//	delayMs(5000);
//	WIFI_funcPIN_nReload = WIFI_pinDISEN;

//	delayMs(5000);
	
	while(WIFI_tipsPIN_nReady && (tOut_cnt --))delayMs(1);
	if(!tOut_cnt)return 0;
	delayMs(200);

	if(!WIFI_ENTM_OUT(15))return 0;								//退透传
	if(!cmdPackAT_validation(wifiCMD_dats[0], 5))return 0;		//消回传
	
	/*AP模式废除*/
	if(!Dcode0){	//WMODE配置
//	if(1){	//WMODE配置
		
		if(!cmdPackAT_validation(wifiCMD_dats[1], 5))return 0;
		
		sprintf(ATCMD_temp, "AT+WAP=11BGN,LANBON_FPSW_%d,AUTO\r\n", (int)MAC_ID[5]);
		if(!cmdAT_validation(ATCMD_temp,						//AP_SSID配置
							 wifiCMD_dats[4].wifiInit_REPLY,
							 wifiCMD_dats[4].REPLY_DATLENTAB,
							 wifiCMD_dats[4].timeTab_waitAnsr,
							 5))return 0; 
		
		
		sprintf(ATCMD_temp, "AT+WAKEY=WPA2PSK,AES,lanbon123\r\n");
		if(!cmdAT_validation(ATCMD_temp,						//AP_密码配置
							 wifiCMD_dats[4].wifiInit_REPLY,
							 wifiCMD_dats[4].REPLY_DATLENTAB,
							 wifiCMD_dats[4].timeTab_waitAnsr,
							 5))return 0; 
		
		IF_wifiConfigCMP_FLG = 1;	//WIFI工作模式配置标志预置
		
	}else{
	
		if(!cmdPackAT_validation(wifiCMD_dats[2], 5))return 0;
	}
	
	if(!cmdPackAT_validation(wifiCMD_dats[4], 5))return 0;	//socket_A配置
	
	EEPROM_read_n(EEPROM_ADDR_serverIP_record, &serverIP_temp[0], 4);
	sprintf(ATCMD_temp, "AT+SOCKB=UDP,80,%d.%d.%d.%d\r\n", (int)serverIP_temp[0], 
														   (int)serverIP_temp[1], 
														   (int)serverIP_temp[2], 
														   (int)serverIP_temp[3]);
														   
	if(!cmdAT_validation(ATCMD_temp,						//socket_B配置
						 wifiCMD_dats[4].wifiInit_REPLY,
						 wifiCMD_dats[4].REPLY_DATLENTAB,
						 wifiCMD_dats[4].timeTab_waitAnsr,
						 5))return 0; 
	
		
//	if(!cmdPackAT_validation(wifiCMD_dats[3], 5))return 0;		//wifi重启
	
	uartObjWIFI_Send_String(wifiCMD_dats[3].wifiInit_reqCMD, strlen(wifiCMD_dats[3].wifiInit_reqCMD));
	delayMs(200);
	uartObjWIFI_Send_String(wifiCMD_dats[3].wifiInit_reqCMD, strlen(wifiCMD_dats[3].wifiInit_reqCMD));
	
	return 1;
}

/*********************WMODE更变******************/
bit WIFI_WMODE_CHG(bit ifAP){
	
	u8 xdata str_temp[40] = {0}; 
	
	if(!IF_wifiConfigCMP_FLG){		//判断开机已经初始化判断
		
		u16 data tOut_cnt	= 5000;
		
		process_FPworkMode	= work_Mode_NULL;			//指纹模块失能
		SYS_status_Mode 	= statusMode_apSwitch;		//系统状态更新
		
		WIFI_funcPIN_nRst	= WIFI_pinEN;	//硬件复位一次
		delayMs(200);
		WIFI_funcPIN_nRst	= WIFI_pinDISEN;
		delayMs(3000);
		WIFI_funcPIN_nReload = WIFI_pinEN;
		delayMs(1000);
		WIFI_funcPIN_nReload = WIFI_pinDISEN;
		
		while(WIFI_tipsPIN_nReady && (tOut_cnt --))delayMs(10);
		if(!tOut_cnt)return 0;
		
		delayMs(1000);
	
		if(!WIFI_ENTM_OUT(10))return 0;								//退透传
		if(!cmdPackAT_validation(wifiCMD_dats[0], 5))return 0;		//消回传
		
		if(ifAP){
				
			if(!cmdPackAT_validation(wifiCMD_dats[1], 5))return 0;	//配置到AP模式
			
			sprintf(str_temp, "AT+WAP=11BGN,LANBON_FPSW_%d,AUTO\r\n", (int)MAC_ID[5]);
			if(!cmdAT_validation(str_temp,						//AP_SSID配置
								 wifiCMD_dats[4].wifiInit_REPLY,
								 wifiCMD_dats[4].REPLY_DATLENTAB,
								 wifiCMD_dats[4].timeTab_waitAnsr,
								 5))return 0; 
			
			
			sprintf(str_temp, "AT+WAKEY=WPA2PSK,AES,lanbon123\r\n");
			if(!cmdAT_validation(str_temp,						//AP_密码配置
								 wifiCMD_dats[4].wifiInit_REPLY,
								 wifiCMD_dats[4].REPLY_DATLENTAB,
								 wifiCMD_dats[4].timeTab_waitAnsr,
								 5))return 0; 
		}else{

			if(!cmdPackAT_validation(wifiCMD_dats[2], 5))return 0;	//配置到STA模式
		}
		
//		if(!cmdPackAT_validation(wifiCMD_dats[3], 5))return 0;		//wifi重启
		
		uartObjWIFI_Send_String(wifiCMD_dats[3].wifiInit_reqCMD, strlen(wifiCMD_dats[3].wifiInit_reqCMD));
		delayMs(200);
		uartObjWIFI_Send_String(wifiCMD_dats[3].wifiInit_reqCMD, strlen(wifiCMD_dats[3].wifiInit_reqCMD));
		
	}else{
	
		IF_wifiConfigCMP_FLG = 0;
	}	
	
	return 1;
}

/*********************UDP_IP更变******************/
bit WIFI_serverUDP_CHG(unsigned char ip[4]){

	unsigned char idata ipNem_temp[40] 		= {0},
				  idata ipRecord_temp[4]	= {0};
				  
	EEPROM_read_n(EEPROM_ADDR_serverIP_record, ipRecord_temp, 4);
	if(!memcmp(ipRecord_temp, ip, 4))return 1;
	else{
		
		coverEEPROM_write_n(EEPROM_ADDR_serverIP_record, ip, 4);
	
		sprintf(ipNem_temp, "AT+SOCKB=UDP,80,%d.%d.%d.%d\r\n", (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3]);
		
		if(!WIFI_ENTM_OUT(10))return 0;
		if(!cmdPackAT_validation(wifiCMD_dats[0], 5))return 0;
		
		if(!cmdAT_validation(ipNem_temp,						//socket_B配置
							 wifiCMD_dats[4].wifiInit_REPLY,
							 wifiCMD_dats[4].REPLY_DATLENTAB,
							 wifiCMD_dats[4].timeTab_waitAnsr,
							 5))return 0; 
		
//		if(!cmdPackAT_validation(wifiCMD_dats[3], 5))return 0;
		
		uartObjWIFI_Send_String(wifiCMD_dats[3].wifiInit_reqCMD, strlen(wifiCMD_dats[3].wifiInit_reqCMD));
		delayMs(200);
		uartObjWIFI_Send_String(wifiCMD_dats[3].wifiInit_reqCMD, strlen(wifiCMD_dats[3].wifiInit_reqCMD));
		while(WIFI_tipsPIN_nReady);
		delayMs(3000);
		
		return 1;
	}
}

