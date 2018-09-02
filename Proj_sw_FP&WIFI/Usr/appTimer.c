#include "timer.h"
#include "USART.h"

#include "wifi_LPT220.h"
#include "usrKin.h"
#include "appTimer.h"
#include "Tips.h"
#include "Relay.h"

#include "stdio.h"
#include "string.h"

#include "soft_uart.h"

//***************指纹线程变量引用区*********************/
extern threadFP_Mode 	process_FPworkMode;		//指纹模块工作模式

extern stt_stepAdd 		umNew_AddStep;			//学习模式时_进程指示
extern stt_stepAdd 		umNew_AddStep_StandBy;	//学习模式时_预备进程指示
extern bit				FP_umDetect_enFLAG;		//指纹模块工作模式为：主动检测 模板时，工作使能标志，用于权限定时
extern stt_stepDtt		um_dttStep;				//指纹模块工作模式为：主动检测 模板时，_进程指示
extern bit 				FPvalidation_flg;		//指纹模块工作模式为：主动检测 模板时，_识别完成标志

extern u16 xdata		Mode_umAdd_timeOutcnt;	//用于学习模式时超时检测

extern u16 xdata		FPmodeCmpTips_cnt;		//模式触发完成后Tips专用计时，需在timer中调用

//***************继电器线程变量引用区*******************/
extern rly_timFun  		relayTIM;
extern bit 				status_Relay;

//***************用户按键线程变量引用区*****************/
extern bit				ledBackground_method;
extern bit		 		switch_NONC_method;
extern bit		 		usrKeyCount_EN;
extern u16				usrKeyCount;

//***************WIFI模块功能引脚按键线程变量引用区*****/
extern u16 xdata		funKey_WIFI_cnt;

//***************WIFI驱动变量引用区***************************/
extern status_Mode		SYS_status_Mode;	//系统状态

//***************数据传输变量引用区***************************/
extern u16	   			heartBeat_Cnt;	//心跳包周期计数

//***************数据传输变量引用区***************************/
extern bit 				rxTout_count_EN;	
extern u8  				rxTout_count;	//串口接收超时计数
extern bit 				uartRX_toutFLG;
extern u8 				datsRcv_length;
extern uartTout_datsRcv xdata datsRcv_ZIGB;

/*----------------------------------------------------------------------------------------*/
void appTimer0_Init(void){

	AUXR |= 0x80;		
	TMOD &= 0xF0;		
	TL0   = 0x50;		
	TH0   = 0xFB;	
	TF0   = 0;	
	ET0	  = 1;	//开中断
	PT0   = 0;	//高优先级中断
	
	TR0   = 1;	
}

/********************* Timer0中断函数************************///1ms
void timer0_Rountine (void) interrupt TIMER0_VECTOR{
	
	u8 code   period_1ms 			= 20;		//1ms专用
	static u8 count_1ms 			= 0;
	
	static u16 	timTipsFP_Cnt 	= 0;
	const  u16	Period_TipsFP 	= 120;
	
	static u16 	timTipsSYS_Cnt 	= 0;
	const  u16	Period_TipsSYS 	= 80;
	
	static u16 	timRelay_Cnt 	= 0;
	
	const  u8	WDT_Period		= 150;
	static u8	WDT_cnt			= 0;	
	
	//*******************标准1毫秒计数业务**************************/
	if(count_1ms < period_1ms)count_1ms ++;
	else{
	
		count_1ms = 0;
		
		/*心跳包周期计数业务*/
		heartBeat_Cnt++;

		/*喂狗业务*/
		if(WDT_cnt < WDT_Period)WDT_cnt ++;
		else{
		
			WDT_cnt 	= 	0;
			
			WDT_CONTR	|=	(1 << 4);	//喂狗
		}
		
		/*指纹模块指示灯业务代码*/
		if(timTipsFP_Cnt < Period_TipsFP)timTipsFP_Cnt ++;
		else{
		
			timTipsFP_Cnt = 0;
			
			switch(process_FPworkMode){
				
				case work_Mode_NULL:{
				
				//提示动作保留
				}break;
			
				case work_Mode_umAdd:{
					
					switch(umNew_AddStep_StandBy){
					
						case umStepAdd_A:{
						
							led_Act(blue, led_flip);
						}break;
						
						case umStepAdd_B:{
						
							led_Act(green, led_flip);
						}break;
						case umStepAdd_C:{
						
							led_Act(green, led_on);
						}break;
						
						case umStepAdd_D:{
						
							
						}break;
						
						case umStepAdd_Fault:{
						
							led_Act(red, led_on);
						}break;
						
						default:{
						
						}break;
					}
				}break;
				
				case work_Mode_umDetect:{
					
					if(FP_umDetect_enFLAG){		/*指纹模块主动检测使能*///指纹模块工作在主动检测模式下，检测未被禁用
					
						switch(um_dttStep){
						
							case umStepDtt_A:{
							
								if(!ledBackground_method){
								
									if(status_Relay){
									
										led_Act(blue, led_on);
									}else{
									
										led_Act(green, led_on);
									}
								}else{
									
									if(status_Relay){
									
										led_Act(green, led_on);
									}else{
									
										led_Act(blue, led_on);
									}
								}
							}break;
							
							case umStepDtt_B:{
								
								led_darkAll();
							}break;
							
							case umStepDtt_Csuccess:{
							
								if(!ledBackground_method){
								
									if(status_Relay){
									
										led_Act(blue, led_flip);
									}else{
									
										led_Act(green, led_flip);
									}
								}else{
									
									if(status_Relay){
									
										led_Act(green, led_flip);
									}else{
									
										led_Act(blue, led_flip);
									}
								}
							}break;
							
							case umStepDtt_Cfail:{
							
								led_Act(red, led_flip);
							}break;
							
							default:{
							
							//提示动作保留
							}break;
						}
					}else{	/*指纹模块主动检测失能*///指纹模块工作在主动检测模式下，检测被禁用
					
						switch(um_dttStep){
						
							case umStepDtt_A:{
							
								if(!ledBackground_method){
								
									if(status_Relay){
									
										led_Act(green, led_on);
									}else{
									
										led_Act(blue, led_on);
									}
								}else{
								
									if(status_Relay){
									
										led_Act(blue, led_on);
									}else{
									
										led_Act(green, led_on);
									}
								}
							}break;
							
							case umStepDtt_B:{
								
								led_Act(red, led_on);
							}break;
						}
					}
				}break;
				
				default:break;
			}
		}
		
		/*系统任务指示灯业务代码*/
		if(timTipsSYS_Cnt < Period_TipsSYS)timTipsSYS_Cnt ++;
		else{
		
			timTipsSYS_Cnt = 0;
			
			switch(SYS_status_Mode){
			
				case statusMode_Reset:{
					
					static u8 cnt_A	= 1;
					
					if(--cnt_A);
					else{
					
						cnt_A = 1;
						led_Act(red, led_flip);
					}
				}break;
				
				case statusMode_smartLink:{
					
					static u8 cnt_B = 4;
					
					if(--cnt_B){
						
						if(!Usr_key)(cnt_B > 1)?(led_Act(red, led_on)):(led_Act(green, led_on));
						else{
						
							led_Act(red, led_on);
						}
					}
					else{
					
						cnt_B = 4;
					}
				}break;
				
				case statusMode_ftRecover:{
				
					static u8 cnt_B = 5;
					
					if(--cnt_B){
					
						if(!Usr_key)(cnt_B > 2)?(led_Act(red, led_on)):(led_Act(blue, led_on));
						else{
						
							led_Act(red, led_on);
						}
					}
					else{
					
						cnt_B = 5;
					}
				}break;
				
				case statusMode_standBy:{
				
					static u8 cnt_D = 3;
					
					if(--cnt_D){
					
						(cnt_D > 1)?(led_darkAll()):(led_Act(red, led_on));
					}
					else{
					
						cnt_D = 3;
					}
				}break;
				
				case statusMode_apSwitch:{
				
					led_Act(red, led_on);
				}break;
				
				default:{
				
					//提示动作保留
				}break;
			}
		}
		
		/*指纹模块业务代码*/
		if(Mode_umAdd_timeOutcnt)Mode_umAdd_timeOutcnt --;
		if(FPmodeCmpTips_cnt)FPmodeCmpTips_cnt --;
		
		/*继电器业务代码*/
		if(relayTIM.tim_EN){
		
			if(timRelay_Cnt < relayTIM.tim_Period)timRelay_Cnt++;
			else{
				
				timRelay_Cnt 	= 0;
				relayTIM.tim_EN	= 0;
				
				(switch_NONC_method)?(status_Relay = actRelay_ON):(status_Relay = actRelay_OFF);
				CON_RELAY 		= status_Relay;
			}
		}else{
		
			timRelay_Cnt = 0;
		}
		
		/*WIFI模块功能引脚模拟按键计数业务区*/
		if(funKey_WIFI_cnt)funKey_WIFI_cnt --;
		else{
		
			WIFI_funcPIN_nReload = WIFI_pinDISEN;	//WIFI外部引脚控制业务
		}
		
		/*用户按键时长计数*/
		if(usrKeyCount_EN)usrKeyCount ++;
	}
	
	//***************串口接收超时时长计数*******************************//
	if(rxTout_count_EN){
	
		if(rxTout_count < TimeOutSet1)rxTout_count ++;
		else{
			
			if(!uartRX_toutFLG){
			
				uartRX_toutFLG = 1;
				
				memset(datsRcv_ZIGB.rcvDats, 0xff, sizeof(char) * COM_RX1_Lenth);
				memcpy(datsRcv_ZIGB.rcvDats, RX1_Buffer, datsRcv_length);
				datsRcv_ZIGB.rcvDatsLen = datsRcv_length;
			}
			rxTout_count_EN = 0;
		}
	}
}

///********************* Timer1中断函数************************/
//void timer1_Rountine (void) interrupt TIMER1_VECTOR
//{
//	static u16 	tim_Cnt 	= 0;
//	const  u16	Period_Tips = 200;
//	
//	if(tim_Cnt < Period_Tips)tim_Cnt ++;
//	else{
//	
//		tim_Cnt = 0;
//		
//		led_Act(green, led_flip);
//	}
//}

///********************* Timer2中断函数************************/
//void timer2_Rountine (void) interrupt TIMER2_VECTOR
//{
//	static u16 	tim_Cnt 	= 0;
//	const  u16	Period_Tips = 200;
//	
//	if(tim_Cnt < Period_Tips)tim_Cnt ++;
//	else{
//	
//		tim_Cnt = 0;
//		
//		led_Act(blue, led_flip);
//	}	
//}