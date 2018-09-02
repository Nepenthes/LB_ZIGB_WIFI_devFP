#include "usrKin.h"
#include "Tips.h"

#include "sensor_FP.h"
#include "wifi_LPT220.h"
#include "relay.h"
#include "delay.h"

#include "soft_uart.h"

#include "stdio.h"
#include "string.h"

//*********************ָ��ģ����ر���������******************/
extern threadFP_Mode	process_FPworkMode;

//*********************���ض�ʱ���Ա���������******************/
extern rly_timFun 		relayTIM;

//*********************�̵���������ر�������******************/
extern threadRelay_Mode	process_RelayWorkMode;
extern status_ifSave	relayStatus_ifSave;
extern bit 				status_Relay;

/**********************�����ļ�����������**********************/
u8 idata val_DcodeCfm 			= 0;  //����ֵ
bit		 ledBackground_method	= 0;  //��������ɫ���� //Ϊ1ʱ����-�� ��-��   Ϊ0ʱ����-�� ��-��
bit		 switch_NONC_method		= 0;
bit		 usrKeyCount_EN			= 0;
u16		 usrKeyCount			= 0;

u8 DcodeScan_oneShoot(void){

	u8 val_Dcode = 0;
	
	if(!Dcode0)val_Dcode |= 1 << 0;
	else val_Dcode &= ~(1 << 0);
	
	if(!Dcode1)val_Dcode |= 1 << 1;
	else val_Dcode &= ~(1 << 1);
	
	if(!Dcode2)val_Dcode |= 1 << 2;
	else val_Dcode &= ~(1 << 2);
	
	if(!Dcode3)val_Dcode |= 1 << 3;
	else val_Dcode &= ~(1 << 3);
	
	if(!Dcode4)val_Dcode |= 1 << 4;
	else val_Dcode &= ~(1 << 4);
	
	if(!Dcode5)val_Dcode |= 1 << 5;
	else val_Dcode &= ~(1 << 5);
	
	return val_Dcode;
}

bit UsrKEYScan_oneShoot(void){

	return Usr_key;
}

void DcodeScan(void){

	static u8 	val_Dcode_Local 	= 0,
				comfirm_Cnt			= 0;
	const  u8 	comfirm_Period		= 3;	//����ʱ����������ȡ�������̵߳�������
		
		   u8 	val_Dcode_differ	= 0;
	
		   bit	val_CHG				= 0;
	
	val_DcodeCfm = DcodeScan_oneShoot();
	
	if(val_Dcode_Local != val_DcodeCfm){
	
		if(comfirm_Cnt < comfirm_Period)comfirm_Cnt ++;
		else{
		
			comfirm_Cnt = 0;
			val_CHG		= 1;
		}
	}
	
	if(val_CHG){
		
		val_CHG				= 0;
	
		val_Dcode_differ 	= val_Dcode_Local ^ val_DcodeCfm;
		val_Dcode_Local		= val_DcodeCfm;
		
		beeps(0);

		if(val_Dcode_differ & Dcode_FLG_ifAP){
			
			u8 tOut = 3; 
		
			if(val_Dcode_Local & Dcode_FLG_ifAP){
			
				while(!WIFI_WMODE_CHG(1) && tOut){delayMs(2000); tOut --;}
				LogString("WIFIģ���������� APģʽ,�ȴ�����\n");
			}else{
			
				while(!WIFI_WMODE_CHG(0) && tOut){delayMs(2000);  tOut --;}
				LogString("WIFIģ���������� STAģʽ,�ȴ�����\n");
			}
			
			if(!tOut)((void(code *)(void))0x0000)();
			
			process_FPworkMode	= work_Mode_umDetect;		//ָ��ģ�鹤��ģʽ�ָ����������ģʽ
			SYS_status_Mode 	= statusMode_NULL;			//ϵͳ״̬���»ָ�����ʼ��
		}
		
		if(val_Dcode_differ & Dcode_FLG_ifNONC){
			
			if(Dcode_modeMix(val_Dcode_Local) == 2 || Dcode_modeMix(val_Dcode_Local) == 3){ //��ʱģʽ�� NONC��Ч
			
				status_Relay = !status_Relay;
				CON_RELAY = status_Relay;
			
				if(val_Dcode_Local & Dcode_FLG_ifNONC){
					
//					ledBackground_method = 1;
					switch_NONC_method = 1;
					LogString("�л������ⷽ��A.\n");
					
				}else{
				
//					ledBackground_method = 0;
					switch_NONC_method = 0;
					LogString("�л������ⷽ��B.\n");
				}
			}
		}
		
		if(val_Dcode_differ & Dcode_FLG_saveRelay_MODEX){
		
			switch(Dcode_modeMix(val_Dcode_Local)){
			
				case 0:{
				
					process_RelayWorkMode = work_Mode_flip;
					relayStatus_ifSave	  = statusSave_disable;
					LogString("�������л�����תģʽ.\n");
					LogString("���ؼ��书���ѹر�.\n");
					
				}break;
					
				case 1:{
				
					process_RelayWorkMode = work_Mode_flip;
					relayStatus_ifSave	  = statusSave_enable;
					LogString("���ؼ��书���ѿ���.\n");
					
				}break;
					
				case 2:{
				
					process_RelayWorkMode = work_Mode_conuter;
					relayStatus_ifSave	  = statusSave_disable;
					LogString("�������л�����ʱģʽ.\n");
					
				}break;
					
				case 3:{
					
					process_RelayWorkMode = work_Mode_conuter;
					relayStatus_ifSave	  = statusSave_disable;				
					LogString("�������л�����ʱģʽ.\n");
					LogString("��ʱģʽ�£����ؼ��������壬���ؼ��书���ѹر�.\n");
					
				}break;
					
				default:break;
			}
		}
		
		if(val_Dcode_differ & Dcode_FLG_swDelay){
		
			switch(Dcode_cntTim(val_Dcode_Local)){
			
				case 0:{
				
					relayTIM.tim_Period = 1000;
					LogString("��ǰ��ʱ�����ѵ��� 1��.\n");
				}break;
					
				case 1:{
				
					relayTIM.tim_Period = 3000;
					LogString("��ǰ��ʱ�����ѵ��� 3��.\n");
				}break;
					
				case 2:{
				
					relayTIM.tim_Period = 5000;
					LogString("��ǰ��ʱ�����ѵ��� 5��.\n");
				}break;
					
				case 3:{
				
					relayTIM.tim_Period = 10000;
					LogString("��ǰ��ʱ�����ѵ��� 10��.\n");
				}break;
					
				default:break;
			}LogString("���ش�����ʱģʽʱʱ��Ч.\n");
		}
	}
}

void UsrKEYScan(funKey_Callback funCB_Short, funKey_Callback funCB_LongA, funKey_Callback funCB_LongB){
	
	code	u16	keyCfrmLoop_Short 	= 1,	//�̰�����ʱ��,�ݴ�ѭ������
				keyCfrmLoop_LongA 	= 3000,	//����Aʱ��,�ݴ�ѭ������
				keyCfrmLoop_LongB 	= 15000,//����Bʱ��,�ݴ�ѭ������
				keyCfrmLoop_MAX	 	= 60000;//��ʱ�ⶥ
	
	static	bit LongA_FLG = 0;
	static	bit LongB_FLG = 0;
	
	static	bit keyPress_FLG = 0;

	if(!UsrKEYScan_oneShoot()){		//ѡ����ʾ
		
		keyPress_FLG = 1;
	
		if(!usrKeyCount_EN) usrKeyCount_EN= 1;	//��ʱ
		
		if((usrKeyCount >= keyCfrmLoop_LongA) && (usrKeyCount <= keyCfrmLoop_LongB) && !LongA_FLG){
		
			process_FPworkMode	= work_Mode_NULL;			//ָ��ģ��ʧ��
			SYS_status_Mode 	= statusMode_smartLink;		//ϵͳ״̬����
			
			funCB_LongA();
			
			LongA_FLG = 1;
		}	
		
		if((usrKeyCount >= keyCfrmLoop_LongB) && (usrKeyCount <= keyCfrmLoop_MAX) && !LongB_FLG){
		
			process_FPworkMode	= work_Mode_NULL;			//ָ��ģ��ʧ��
			SYS_status_Mode 	= statusMode_ftRecover;		//ϵͳ״̬����
			
			funCB_LongB();
			
			LongB_FLG = 1;
		}
		
	}else{		//ѡ����Ӧ
		
		usrKeyCount_EN = 0;
		
		if(keyPress_FLG){
		
			keyPress_FLG = 0;
			
//			if(usrKeyCount < keyCfrmLoop_LongA && usrKeyCount > keyCfrmLoop_Short)funCB_Short();
			
			usrKeyCount = 0;
			LongA_FLG 	= 0;
			LongB_FLG 	= 0;
			
			SYS_status_Mode 	= statusMode_NULL;			//ϵͳ״̬���»ָ�����ʼ��
			process_FPworkMode	= work_Mode_umDetect;		//ָ��ģ�鹤��ģʽ�ָ����������ģʽ
		}
	}
}


