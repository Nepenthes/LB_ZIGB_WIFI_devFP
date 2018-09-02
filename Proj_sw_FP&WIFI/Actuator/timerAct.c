#include "timerAct.h"

#include "sensor_FP.h"
#include "HYM8564.h"
#include "datasReference.h"
#include "Relay.h"

#include "eeprom.h"
#include "soft_uart.h"

#include "stdio.h"
#include "string.h"

/****************�����ļ�����������*************************/
switch_Status	swStatus_fromTim  				= swStatus_null;	//��ʱ�����¿��������־����ʱʱ�̵���ʱ������Ӧ����״̬

u8 				swTim_onShoot_FLAG				= 0;	//��ͨ���ض�ʱһ���Ա�־��������λ��ʶ�ĸ���ʱ��
	
u8	 			permissionTim_oneShoot_FLAG 	= 0;	//ָ��Ȩ�޶�ʱһ���Ա�־��������λ��ʶ�ĸ���ʱ��

bit				ifTim_sw_running_FLAG			= 0;	//��ͨ���ض�ʱ���б�־λ
bit				ifTim_permission_running_FLAG	= 0;	//ָ��Ȩ�޶�ʱ���б�־λ

//***************ָ��ģ�������߳���ر���������*************/

extern bit		FP_umDetect_enFLAG;		//ָ��ģ�鹤��ģʽΪ��������� ģ��ʱ������ʹ�ܱ�־������Ȩ�޶�ʱ

/*-----------------------------------------------------------------------------------------------*/
void datsTiming_read_eeprom(timing_Dats timDats_tab[4], timingTabObj timCountObj){

	u8 dats_Temp[12];
	u8 loop;
	
	switch(timCountObj){
	
		case timTabObj_swNormal:{
		
			EEPROM_read_n(EEPROM_ADDR_swTimeTab, dats_Temp, 12);
		}break;
		
		case timTabObj_permission:{
		
			EEPROM_read_n(EEPROM_ADDR_permissionTimeTab, dats_Temp, 12);
		}break;
		
		default:break;
	}
	
	for(loop = 0; loop < 4; loop ++){
	
		timDats_tab[loop].Week_Num	= (dats_Temp[loop * 3 + 0] & 0x7f) >> 0;	/*��ռλ����*///����λ
		timDats_tab[loop].if_Timing = (dats_Temp[loop * 3 + 0] & 0x80) >> 7;	/*�Ƿ�����ʱ������*///��һλ
		timDats_tab[loop].Status_Act= (dats_Temp[loop * 3 + 1] & 0xe0) >> 5;	/*��ʱ��Ӧ״̬����*///����λ
		timDats_tab[loop].Hour		= (dats_Temp[loop * 3 + 1] & 0x1f) >> 0;	/*��ʱʱ��_Сʱ*///����λ
		timDats_tab[loop].Minute	= (dats_Temp[loop * 3 + 2] & 0xff) >> 0;	/*��ʱʱ��_��*///ȫ��λ
	}
}

/*��ռλ�ж�*///�жϵ�ǰ��ֵ�Ƿ���ռλ�ֽ���
bit weekend_judge(u8 weekNum, u8 HoldNum){

	u8 loop;
	
	weekNum --;
	for(loop = 0; loop < 7; loop ++){
	
		if(HoldNum & (1 << loop)){
			
			if(loop == weekNum)return 1;
		}
	}
	
	return 0;
}

void thread_Timing(void){

#define TIME_LOGOUT_EN	0	//��ǰʱ��logʹ��
	
	u8 loop = 0;
	
	stt_Time 			valTime_Local					= {0};	//��ǰʱ�仺��
	
	timing_Dats 		timDatsTemp_CalibrateTab[4] 	= {0};	/*��ʱ��ʼʱ�̱���*///��ʼʱ�̼�����
	u16					Minute_Temp						= 0;	//�� ���㻺��
	
	u8 					timDatsTemp_KeepTab[4]			= {0};	//��ʱʱ������

#if(TIME_LOGOUT_EN == 1)
	code	u8 			time_Logout_Period 				= 5;	//ʱ��log���ڡ�����ϵ�����ˣ��ݴ�ѭ���ֶ����ڣ�
	static 	u8 			time_Logout_Cnt					= 0;	//ʱ��log�ȴ����ڼ�ʱ
#endif
	
	timeRead(&valTime_Local);	//��ǰʱ���ȡ

#if(TIME_LOGOUT_EN == 1)	
	if(time_Logout_Cnt < time_Logout_Period)time_Logout_Cnt ++;
	else{
	
		time_Logout_Cnt = 0;
		time_Logout(valTime_Local);
	}
#endif
	
	/*��ͨ���ض�ʱ*///�Ķ�����=======================================================================================================<<<
	datsTiming_read_eeprom(timDatsTemp_CalibrateTab, timTabObj_swNormal);	/*��ͨ����*///ʱ�̱��ȡ
	
	/*�ж��Ƿ�������ͨ���ض�ʱ��Ϊ��*/
	if((timDatsTemp_CalibrateTab[0].if_Timing == 0) &&	//ȫ�أ��ñ�־λ
	   (timDatsTemp_CalibrateTab[1].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[2].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[3].if_Timing == 0)
	  ){
	  
		ifTim_sw_running_FLAG = 0; 
		  
	}else{	//��ȫ�أ��ñ�־λ����ִ�ж�ʱ�߼�
		
		ifTim_sw_running_FLAG = 1; 
	
		for(loop = 0; loop < 4; loop ++){
			
			if(weekend_judge(valTime_Local.time_Week, timDatsTemp_CalibrateTab[loop].Week_Num)){	//��ռλ�ȶԣ��ɹ��Ž�����һ��
			
				if(timCount_ENABLE == timDatsTemp_CalibrateTab[loop].if_Timing){	//�Ƿ�����ʱ
					
					u8 log_dats[80] = {0};
					
					sprintf(log_dats, "��Ч��ʱ��%d��, ��_ʱ:%d, ��_��:%d \n", (int)loop, 
																			   (int)timDatsTemp_CalibrateTab[loop].Hour, 
																			   (int)timDatsTemp_CalibrateTab[loop].Minute);
					/*log���Դ�ӡ*///��ͨ��ʱ��ʱ��Ϣ
//					LogDats(log_dats, strlen(log_dats));
				
					if((valTime_Local.time_Hour 	== timDatsTemp_CalibrateTab[loop].Hour)&&	//ʱ�̱ȶ�
					   (valTime_Local.time_Minute	== timDatsTemp_CalibrateTab[loop].Minute)&&
					   (valTime_Local.time_Second	<= 2)){	 //ʱ�̱ȶ�ʱ������ǰ2��
						   
//						LogString("time_UP!!!");
						
						//һ���Զ�ʱ�ж�
						if(swTim_onShoot_FLAG & (1 << loop)){	//�Ƿ�Ϊһ���Զ�ʱ��������ձ��ζ�ʱ��Ϣ
							
							u8 code dats_Temp = 0;
							
							swTim_onShoot_FLAG &= ~(1 << loop);
							coverEEPROM_write_n(EEPROM_ADDR_swTimeTab + loop * 3, &dats_Temp, 1); //��ʱ��Ϣ���
						}
					   
						//��ͨ���ض�����Ӧ
						if(timDatsTemp_CalibrateTab[loop].Status_Act & 0x01){	/*����*/
						
							swStatus_fromTim = swStatus_on;
							
						}else{		/*�ر�*/
						
							swStatus_fromTim = swStatus_off;
						}
					}
				}
			}
		}
	}

	/*ָ��Ȩ�޶�ʱ*///�Ķ�����=======================================================================================================<<<
	datsTiming_read_eeprom(timDatsTemp_CalibrateTab, timTabObj_permission);	/*ָ��Ȩ��*///ʱ�̱��ȡ
	EEPROM_read_n(EEPROM_ADDR_permissionKeepTab, timDatsTemp_KeepTab, 4);	/*ָ��Ȩ��*///��ʱʱ�����ȡ
	
	/*�ж��Ƿ�����ָ��Ȩ�޶�ʱ��Ϊ��*/
	if((timDatsTemp_CalibrateTab[0].if_Timing == 0) &&	//ȫ�أ��ñ�־λ
	   (timDatsTemp_CalibrateTab[1].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[2].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[3].if_Timing == 0)
	  ){
	  
		ifTim_permission_running_FLAG = 0; 
		  
	}else{	//��ȫ�أ��ñ�־λ����ִ�ж�ʱ�߼�
		
		u16 tt_minuteStamp 				= 0,
			tt_minuteTemp_cmpA[4] 		= {0},
			tt_minuteTemp_cmpB[4] 		= {0};
		
		ifTim_permission_running_FLAG 	= 1; 
		
		/*��ǰʱ��ת��Ϊ��λ����������ȶ�*/
		tt_minuteStamp = valTime_Local.time_Hour * 60 + valTime_Local.time_Minute;
			
		memset(tt_minuteTemp_cmpA, 0, 4 * sizeof(u16));
		memset(tt_minuteTemp_cmpB, 0, 4 * sizeof(u16));
		for(loop = 0; loop < 4; loop ++){
		
			if(weekend_judge(valTime_Local.time_Week, timDatsTemp_CalibrateTab[loop].Week_Num)){	/*ǰ��ռλ*///��ռλ�ȶԣ��ɹ��Ž�����һ��
				
				if(timDatsTemp_CalibrateTab[loop].if_Timing){
					
					if(timDatsTemp_KeepTab[loop] == 255){
					
						tt_minuteTemp_cmpA[loop] = 0;
						tt_minuteTemp_cmpB[loop] = 1439;
					}else{
					
						tt_minuteTemp_cmpA[loop] = timDatsTemp_CalibrateTab[loop].Hour * 60 + timDatsTemp_CalibrateTab[loop].Minute;	//��ʱ_���
						tt_minuteTemp_cmpB[loop] = tt_minuteTemp_cmpA[loop] + timDatsTemp_KeepTab[loop];
					}
				}
			}
		}
		
		/*log���Դ�ӡ*///Ȩ�޶�ʱ���ݴ�ӡ
		{
			u8 log_dats[80] = {0};
			static u8 cnt_disp = 0;
			
			sprintf(log_dats, "time_A1: %d, time_B1: %d\n time_A2: %d, time_B2: %d\n time_A3: %d, time_B3: %d\n time_A4: %d, time_B4: %d\n time_NOW: %d\n=========log_dats<<<\n\n",
							  (int)tt_minuteTemp_cmpA[0], (int)tt_minuteTemp_cmpB[0], 
							  (int)tt_minuteTemp_cmpA[1], (int)tt_minuteTemp_cmpB[1],
							  (int)tt_minuteTemp_cmpA[2], (int)tt_minuteTemp_cmpB[2], 
							  (int)tt_minuteTemp_cmpA[3], (int)tt_minuteTemp_cmpB[3], 
						      (int)tt_minuteStamp);
							  
			if(cnt_disp < 10)cnt_disp ++;
			else{
			
				cnt_disp = 0;
//				LogDats(log_dats, strlen(log_dats));
			}
		}
		
		if((tt_minuteStamp >= tt_minuteTemp_cmpA[0] && tt_minuteStamp < tt_minuteTemp_cmpB[0])||
		   (tt_minuteStamp >= tt_minuteTemp_cmpA[1] && tt_minuteStamp < tt_minuteTemp_cmpB[1])||
		   (tt_minuteStamp >= tt_minuteTemp_cmpA[2] && tt_minuteStamp < tt_minuteTemp_cmpB[2])||
		   (tt_minuteStamp >= tt_minuteTemp_cmpA[3] && tt_minuteStamp < tt_minuteTemp_cmpB[3])
		  ){
			  
				/*Ȩ�޶�����Ӧ*/
				if(timDatsTemp_CalibrateTab[loop].Status_Act & 0x01){	/*ָ�ƴ�������ʹ��*/
				
					FP_umDetect_enFLAG = 1;
					
				}else{	/*ָ�ƴ�������ʧ��*/
				
					FP_umDetect_enFLAG = 0;
				}
				
		  }else{
		  
				/*Ȩ�޶�����Ӧ*///��ת�ָ�
				if(timDatsTemp_CalibrateTab[loop].Status_Act & 0x01){	/*ָ�ƴ�������ʧ��*/

					FP_umDetect_enFLAG = 0;
					
				}else{	/*ָ�ƴ�������ʹ��*/

					FP_umDetect_enFLAG = 1;
				}
		  }
		
		for(loop = 0; loop < 4; loop ++){
				
			if(timDatsTemp_CalibrateTab[loop].if_Timing){
				
				if(tt_minuteStamp >= tt_minuteTemp_cmpB[loop]){
				
					if(permissionTim_oneShoot_FLAG & (1 << loop)){	//�Ƿ�Ϊһ���Զ�ʱ��������ձ��ζ�ʱ��Ϣ
						
						u8 code dats_Temp = 0;
						
						permissionTim_oneShoot_FLAG &= ~(1 << loop);
						coverEEPROM_write_n(EEPROM_ADDR_permissionTimeTab + loop * 3, &dats_Temp, 1); //��ʱ��Ϣ���
						coverEEPROM_write_n(EEPROM_ADDR_permissionKeepTab + loop * 3, &dats_Temp, 1);
					}
				}
			}
		}
	}
}
