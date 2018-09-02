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

//***************ָ���̱߳���������*********************/
extern threadFP_Mode 	process_FPworkMode;		//ָ��ģ�鹤��ģʽ

extern stt_stepAdd 		umNew_AddStep;			//ѧϰģʽʱ_����ָʾ
extern stt_stepAdd 		umNew_AddStep_StandBy;	//ѧϰģʽʱ_Ԥ������ָʾ
extern bit				FP_umDetect_enFLAG;		//ָ��ģ�鹤��ģʽΪ��������� ģ��ʱ������ʹ�ܱ�־������Ȩ�޶�ʱ
extern stt_stepDtt		um_dttStep;				//ָ��ģ�鹤��ģʽΪ��������� ģ��ʱ��_����ָʾ
extern bit 				FPvalidation_flg;		//ָ��ģ�鹤��ģʽΪ��������� ģ��ʱ��_ʶ����ɱ�־

extern u16 xdata		Mode_umAdd_timeOutcnt;	//����ѧϰģʽʱ��ʱ���

extern u16 xdata		FPmodeCmpTips_cnt;		//ģʽ������ɺ�Tipsר�ü�ʱ������timer�е���

//***************�̵����̱߳���������*******************/
extern rly_timFun  		relayTIM;
extern bit 				status_Relay;

//***************�û������̱߳���������*****************/
extern bit				ledBackground_method;
extern bit		 		switch_NONC_method;
extern bit		 		usrKeyCount_EN;
extern u16				usrKeyCount;

//***************WIFIģ�鹦�����Ű����̱߳���������*****/
extern u16 xdata		funKey_WIFI_cnt;

//***************WIFI��������������***************************/
extern status_Mode		SYS_status_Mode;	//ϵͳ״̬

//***************���ݴ������������***************************/
extern u16	   			heartBeat_Cnt;	//���������ڼ���

//***************���ݴ������������***************************/
extern bit 				rxTout_count_EN;	
extern u8  				rxTout_count;	//���ڽ��ճ�ʱ����
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
	ET0	  = 1;	//���ж�
	PT0   = 0;	//�����ȼ��ж�
	
	TR0   = 1;	
}

/********************* Timer0�жϺ���************************///1ms
void timer0_Rountine (void) interrupt TIMER0_VECTOR{
	
	u8 code   period_1ms 			= 20;		//1msר��
	static u8 count_1ms 			= 0;
	
	static u16 	timTipsFP_Cnt 	= 0;
	const  u16	Period_TipsFP 	= 120;
	
	static u16 	timTipsSYS_Cnt 	= 0;
	const  u16	Period_TipsSYS 	= 80;
	
	static u16 	timRelay_Cnt 	= 0;
	
	const  u8	WDT_Period		= 150;
	static u8	WDT_cnt			= 0;	
	
	//*******************��׼1�������ҵ��**************************/
	if(count_1ms < period_1ms)count_1ms ++;
	else{
	
		count_1ms = 0;
		
		/*���������ڼ���ҵ��*/
		heartBeat_Cnt++;

		/*ι��ҵ��*/
		if(WDT_cnt < WDT_Period)WDT_cnt ++;
		else{
		
			WDT_cnt 	= 	0;
			
			WDT_CONTR	|=	(1 << 4);	//ι��
		}
		
		/*ָ��ģ��ָʾ��ҵ�����*/
		if(timTipsFP_Cnt < Period_TipsFP)timTipsFP_Cnt ++;
		else{
		
			timTipsFP_Cnt = 0;
			
			switch(process_FPworkMode){
				
				case work_Mode_NULL:{
				
				//��ʾ��������
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
					
					if(FP_umDetect_enFLAG){		/*ָ��ģ���������ʹ��*///ָ��ģ�鹤�����������ģʽ�£����δ������
					
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
							
							//��ʾ��������
							}break;
						}
					}else{	/*ָ��ģ���������ʧ��*///ָ��ģ�鹤�����������ģʽ�£���ⱻ����
					
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
		
		/*ϵͳ����ָʾ��ҵ�����*/
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
				
					//��ʾ��������
				}break;
			}
		}
		
		/*ָ��ģ��ҵ�����*/
		if(Mode_umAdd_timeOutcnt)Mode_umAdd_timeOutcnt --;
		if(FPmodeCmpTips_cnt)FPmodeCmpTips_cnt --;
		
		/*�̵���ҵ�����*/
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
		
		/*WIFIģ�鹦������ģ�ⰴ������ҵ����*/
		if(funKey_WIFI_cnt)funKey_WIFI_cnt --;
		else{
		
			WIFI_funcPIN_nReload = WIFI_pinDISEN;	//WIFI�ⲿ���ſ���ҵ��
		}
		
		/*�û�����ʱ������*/
		if(usrKeyCount_EN)usrKeyCount ++;
	}
	
	//***************���ڽ��ճ�ʱʱ������*******************************//
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

///********************* Timer1�жϺ���************************/
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

///********************* Timer2�жϺ���************************/
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