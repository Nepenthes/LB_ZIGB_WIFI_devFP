#include "Relay.h"
#include "Tips.h"
#include "usrKin.h"

#include "eeprom.h"

#include "dataTrans.h"
#include "datasReference.h"

//*********************ָ��ģ����ر���������*********************/
extern bit				FPvalidation_flg;

//*********************���벿����ر���������*********************/
extern u8 idata 		val_DcodeCfm;
extern bit		 		switch_NONC_method;

//*********************���ݴ�����ر���������*********************/
extern switch_Status	swStatus_fromUsr; //��λ��ֱ���´�Ŀ�������

//*********************��ʱ��������ر���������*********************/
extern switch_Status	swStatus_fromTim;  //��ʱʱ�̵���ʱ������Ӧ����״̬

/********************************�����ļ�����������********************************/
rly_timFun 			relayTIM 				= {false, 1000};		//��ʱ����
threadRelay_Mode	process_RelayWorkMode 	= work_Mode_flip;		//���̹߳���ģʽ
status_ifSave		relayStatus_ifSave		= statusSave_disable;	//���ؼ���ʹ�ܱ���
bit 				status_Relay 			= false;				//�̵���״̬_������¶�������̶߳�ȡ״̬ʹ�ã��������״̬������ú���relay_Act();,�в���ֱ�Ӹ�д��״ֵ̬

/*----------------------------------------------------------------*/
/*���ؼ̵�����ʼ��*/
void relay_Init(void){

	u8 statusTemp = 0;
	
	P3M1 &= 0xF7; 
	P3M0 |= 0x08;
	
	CON_RELAY = 0;
	
	if(!Dcode2){
		
		EEPROM_read_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		status_Relay = statusTemp;
		CON_RELAY	 = status_Relay;
	}else{
	
		statusTemp 	 = 0;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
		CON_RELAY	 = status_Relay;
	}
}

/*���ض���*/
void relay_Act(rly_methodType act_Method, bool timer_EN){
	
	u8 statusTemp = 0;
	
	Beep_Tips(1, 8, 2, 20, 10);	//��ʾ��
	
	switch(act_Method){
	
		case relay_flip:{
		
			status_Relay = !status_Relay;
			
			relayTIM.tim_EN = false;
		}break;
		
		case relay_on:{
		
			(switch_NONC_method)?(status_Relay = actRelay_OFF):(status_Relay = actRelay_ON);
			
			if(timer_EN)relayTIM.tim_EN = true;
		}break;
		
		case relay_off:{
		
			(switch_NONC_method)?(status_Relay = actRelay_ON):(status_Relay = actRelay_OFF);
			
			relayTIM.tim_EN = false;
		}break;
	}
	
	CON_RELAY = status_Relay;
	
	if(relayStatus_ifSave == statusSave_enable){	//����״̬����
	
		statusTemp = status_Relay;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
	}
}

/*�̵������߳�*/
void thread_Relay(void){

	switch(process_RelayWorkMode){
	
		/*��תģʽ*///���ؿ�����ر�ȫ������ָ�ָ����һ�ο�������һ�ξ͹أ����˷���
		case work_Mode_flip:{
		
			if(FPvalidation_flg){	//���ش�������ҵ���߼�
			
				FPvalidation_flg = 0;
				relay_Act(relay_flip, 0);
			}
			
			switch(swStatus_fromUsr){	//��λ���´￪��ҵ���߼�
			
				case swStatus_on:{
				
					swStatus_fromUsr = swStatus_null;
					relay_Act(relay_on, 0);
				}break;
				
				case swStatus_off:{
				
					swStatus_fromUsr = swStatus_null;
					relay_Act(relay_off, 0);
				}break;
				
				default:break;
			}
			
			switch(swStatus_fromTim){	//��ʱʱ�̱�������ҵ���߼�
			
				case swStatus_on:{
				
					swStatus_fromTim = swStatus_null;
					relay_Act(relay_on, 0);
				}break;
				
				case swStatus_off:{
				
					swStatus_fromTim = swStatus_null;
					relay_Act(relay_off, 0);
				}break;
				
				default:break;
			}
		}break;
		
		/*��ʱģʽ*///��ʱʱ�䵽����Զ��ر�
		case work_Mode_conuter:{	//���ش�������ҵ���߼�
		
			if(FPvalidation_flg){
			
				FPvalidation_flg = 0;
				relay_Act(relay_on, 1);
			}
			
			switch(swStatus_fromUsr){	//��λ���´￪��ҵ���߼�
			
				case swStatus_on:{
				
					swStatus_fromUsr = swStatus_null;
					relay_Act(relay_on, 1);//��ʱʹ��
				}break;
				
				case swStatus_off:{
				
					swStatus_fromUsr = swStatus_null;
					relay_Act(relay_off, 0);
				}break;
				
				default:break;
			}
			
			switch(swStatus_fromTim){	//��ʱʱ�̱�������ҵ���߼�
			
				case swStatus_on:{
				
					swStatus_fromTim = swStatus_null;
					relay_Act(relay_on, 1);//��ʱʹ��
				}break;
				
				case swStatus_off:{
				
					swStatus_fromTim = swStatus_null;
					relay_Act(relay_off, 0);
				}break;
				
				default:break;
			}
		}break;
		
		default:break;
	}
}