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

	task_Func tFunc;		//������
	u8		  tCnt;			//��ʱ����
	u8		  tPeriod;		//��ʱ����
}task_Nature;

void WDT_Confing(void){

	WDT_CONTR  = 0x05;	//�蹷
	WDT_CONTR |= 0x20;	//�Ź�
}

void Bsp_Init(void){		//��ʼ��˳���ܶ�������
	
	ledTips_Init		();	//Tips_LED���ų�ʼ�������ȳ�ʼ����������ʾ��
	Beep_Init			();	//Tips_Deep���ų�ʼ��
	relay_Init			();	//�̵������ų�ʼ��
	appTimer0_Init		();	//Ӳ����ʱ����ʼ��
	WIFI_pinInit		();	//WIFIģ���ⲿ�������ų�ʼ��
	uartObjWIFI_Init	();	//WIFI����Ӳ����ʼ��
	uartObjFP_Init		();	//ָ��ģ�鴮��Ӳ����ʼ��
	
	AUXR 	&= 0xfd;	//xdata ʹ��
	EA 		= 1;		//���жϺ�ſ���ʹ�ô��ڣ�
}

void Config_Init(void){		/*����ʼ��������Bsp_Init() ֮��*///����˳���ܶ�������

	birthDay_Judge		();	//�����ж�
	
#if(IF_REBORN == 1)

	LogString("��Ʒ������ת�������޸ĺ궨������±������ɹ̼���\n");
	while(1);
#endif
	
	//��ȡһ��MAC������״̬����
	MAC_ID_Relaes		();		
	
	//WIFIģ�����ó�ʼ��
	while(!WIFI_configInit	());	
	
	//��ȡһ��ָ��ռλ�洢���ݣ�����״̬����
	logOut_fpID_place	();		
	
	//�Ź�
	WDT_Confing			();		
	
	//������ʱ,ϵͳ׼�����
	delayMs(2000);
	SYS_status_Mode = statusMode_standBy;
	beeps(10);
	SYS_status_Mode = statusMode_NULL;
	
	//ָ��ģ���߳�Ĭ�����������ģʽ
	fpModeTrig_umDetect	();		
}

void threadMain_Necessary(void){	//���������ϸ�Ҫ�󲻸ߵ��߳�

	DcodeScan		();	//����ɨ��
	thread_moudleFP	();	//ָ��ģ�������߳�
	thread_Relay	();	//�̵��������߳�
	UsrKEYScan		(software_Reset, //�ⲿ����ɨ��
					 WIFI_hwSmartLink,
					 Factory_recover);	
}

void thread_Main(void){
	
#define task_Num 3	//�ؼ��߳���
	
//	u16 code countLoop	= 5;
	u8		 task_loop	= 0;

	task_Nature xdata task_Tab[task_Num] = {	/*�ؼ��߳���������*///��������Ҫ��ߵ��߳�
	
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
		
		threadMain_Necessary();		//����������Ҫ���߳�
		
//		delayMs(countLoop);			//�߳�ʱ����ת����
	}
}

/**���Ժ���**/
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
			  
//	WIFI_pinInit		();	//WIFIģ���ⲿ�������ų�ʼ��
//	uartObjWIFI_Init	();	//WIFI����Ӳ����ʼ��
//			  
//	AUXR 	&= 0xfd;	//xdata ʹ��
//	EA 		= 1;		//���жϺ�ſ���ʹ�ô��ڣ�
//	  
//	while(1){
//	
//		WIFI_funcPIN_nReload = WIFI_pinDISEN;
//		WIFI_funcPIN_nRst	= WIFI_pinEN;	//Ӳ����λһ��
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