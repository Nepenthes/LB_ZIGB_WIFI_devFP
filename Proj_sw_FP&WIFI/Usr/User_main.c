#include "STC15Fxxxx.H"

#include "typeDf.h"
#include "datasReference.h"
#include "Tips.h"
#include "dataTrans.h"
#include "Timer.h"
#include "usrKin.h"
#include "Relay.h"
#include "appTimer.h"
#include "wifi_LPT220.h"
#include "dataManage.h"
#include "HYM8564.h"
#include "timerAct.h"

#include "delay.h"
#include "EEPROM.h"
#include "soft_uart.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

extern bit 				UsrKEY_comfirm;
extern threadFP_Mode	process_FPworkMode;
extern stt_stepAdd 		umNew_AddStep;

extern u8 idata 		val_DcodeCfm;

typedef const void (*task_Func)(void);

typedef struct{

	task_Func tFunc;		//任务函数
	u8		  tCnt;			//计时缓存
	u8		  tPeriod;		//计时周期
}task_Nature;

void WDT_Confing(void){

	WDT_CONTR  = 0x05;	//设狗
	WDT_CONTR |= 0x20;	//放狗
}

void Bsp_Init(void){		//初始化顺序不能动！！！
	
	ledTips_Init		();	//Tips_LED引脚初始化，首先初始化，更新提示灯
	Beep_Init			();	//Tips_Deep引脚初始化
	relay_Init			();	//继电器引脚初始化
	appTimer0_Init		();	//硬件定时器初始化
	WIFI_pinInit		();	//WIFI模块外部控制引脚初始化
	uartObjWIFI_Init	();	//WIFI串口硬件初始化
	uartObjFP_Init		();	//指纹模块串口硬件初始化
	
	AUXR 	&= 0xfd;	//xdata 使能
	EA 		= 1;		//开中断后才可以使用串口！
}

void Config_Init(void){		/*本初始化必须在Bsp_Init() 之后！*///配置顺序不能动！！！

	birthDay_Judge		();	//生日判断
	
#if(IF_REBORN == 1)

	LogString("产品已重新转世，请修改宏定义后重新编译生成固件！\n");
	while(1);
#endif
	
	//读取一次MAC，进行状态更新
	MAC_ID_Relaes		();		
	
	//WIFI模块配置初始化
	while(!WIFI_configInit	());	
	
	//读取一次指纹占位存储数据，进行状态更新
	logOut_fpID_place	();		
	
	//放狗
	WDT_Confing			();		
	
	//过渡延时,系统准备完成
	delayMs(2000);
	SYS_status_Mode = statusMode_standBy;
	beeps(10);
	SYS_status_Mode = statusMode_NULL;
	
	//指纹模块线程默认至主动检测模式
	fpModeTrig_umDetect	();		
}

void threadMain_Necessary(void){	//运行周期严格要求不高的线程

	DcodeScan		();	//拨码扫描
	thread_moudleFP	();	//指纹模块驱动线程
	thread_Relay	();	//继电器驱动线程
	UsrKEYScan		(software_Reset, //外部按键扫描
					 WIFI_hwSmartLink,
					 Factory_recover);	
}

void thread_Main(void){
	
#define task_Num 3	//关键线程数
	
//	u16 code countLoop	= 5;
	u8		 task_loop	= 0;

	task_Nature xdata task_Tab[task_Num] = {	/*关键线程任务函数表*///运行周期要求高的线程
	
		{thread_dataTrans,			0,	2},
		{thread_Timing,				0,  1},
		{thread_LocalStaus_Release,	0,	1},
	};
	
	for(;;){
		
		for(task_loop = 0; task_loop < task_Num; task_loop ++){
		
			if(task_Tab[task_loop].tCnt < task_Tab[task_loop].tPeriod)task_Tab[task_loop].tCnt ++;
			else{
			
				task_Tab[task_loop].tCnt = 0;
				
				/******task running here******/
				task_Tab[task_loop].tFunc();
			}			
		}
		
		threadMain_Necessary();		//无运行周期要求线程
		
//		delayMs(countLoop);			//线程时间轮转周期
	}
}

/**测试函数**/
void test_process(void){
	
	uartObjWIFI_Init	();
	EA 		= 1;
	
	HYM8564_Test	();
	
//	Beep_Init();
//	Beep_Test();
//	Beep_Tips(3, 8, 3, 10, 10);
}

/***main***/
int main(void){
	
	const u16 log_Period 	= 1;
		  u16 log_Cnt	 	= 0;
		  u8  log_imfo[40]	= {0},
			  test_ip[4]	= {0};
			  
//	WIFI_pinInit		();	//WIFI模块外部控制引脚初始化
//	uartObjWIFI_Init	();	//WIFI串口硬件初始化
//			  
//	AUXR 	&= 0xfd;	//xdata 使能
//	EA 		= 1;		//开中断后才可以使用串口！
//	  
//	while(1){
//	
//		WIFI_funcPIN_nReload = WIFI_pinDISEN;
//		WIFI_funcPIN_nRst	= WIFI_pinEN;	//硬件复位一次
//		delayMs(1000);
//		WIFI_funcPIN_nRst	= WIFI_pinDISEN;
//		delayMs(5000);

////		while(WIFI_tipsPIN_nReady)delayMs(100);
//		
//		WIFI_funcPIN_nReload = WIFI_pinEN;
//		delayMs(5000);
//		WIFI_funcPIN_nReload = WIFI_pinDISEN;
//		delayMs(10000);
//		
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//		uartObjWIFI_Send_Byte('+');
//		delayMs(200);
//	}
	
	LogString("i'm uart for log !!!\n");
			  
	while(!P34);
	
	Bsp_Init();		  
	Config_Init();
	
	LogString("Product_Lanbon initialize complete, Hellow World!!!\n");
	
	thread_Main();
	
//	test_process();
//	while(1);
	
	return 0;
}