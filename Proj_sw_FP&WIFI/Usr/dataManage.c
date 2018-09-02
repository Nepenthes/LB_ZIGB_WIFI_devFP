#include "dataManage.h"
#include "datasReference.h"
#include "sensor_FP.h"
#include "wifi_LPT220.h"
#include "Tips.h"

#include "string.h"
#include "stdio.h"

#include "soft_uart.h"
#include "eeprom.h"
#include "delay.h"

//*********************ָ��ģ����ر���������********************/
extern threadFP_Mode	process_FPworkMode;

//*******************WIFI��������������***************************/
extern status_Mode		   SYS_status_Mode;	//ϵͳ״̬

/********************�����ļ�����������******************/
unsigned char xdata MAC_ID[6] = {0};

/*------------------------------------------------------------------*/
void software_Reset(void){

	process_FPworkMode	= work_Mode_NULL;			//ָ��ģ��ʧ��
	SYS_status_Mode 	= statusMode_Reset;			//ϵͳ״̬����
	
	Beep_Tips(2, 8, 3, 20, 20);	//��ʾ��
	
	LogString("�����λ�Ѵ���. \n");
	((void(code *)(void))0x0000)();
	
	//����״̬�ָ��������Զ���ʼ��
}

void MAC_ID_Relaes(void){

	unsigned char xdata log_info[50] 	= {0};
	unsigned char code *id_ptr 			= ROMADDR_ROM_STC_ID;
	
	LogDats(log_info, strlen(log_info));

	memcpy(MAC_ID, id_ptr - 6, 6); //˳����ǰ����ǰ����ֻȡ����λ
	
	sprintf(log_info, "MAC��ַ���£�%02X %02X %02X %02X %02X %02X\n", 
			(int)MAC_ID[0],
			(int)MAC_ID[1],
			(int)MAC_ID[2],
			(int)MAC_ID[3],
			(int)MAC_ID[4],
			(int)MAC_ID[5]);
			
	LogDats(log_info, strlen(log_info));
}

void Factory_recover(void){

	u8 xdata log_info[30] 		= {0};
	u8 		 eeprom_buffer[20]	= {0};
	u8 code	 IP_default[4]		= {47,52,5,108};		//Ĭ����۷�����
	
	Beep_Tips(1, 8, 3, 100, 20);	//��ʾ
	
	WIFI_hwRestoreFactory();
	
	EEPROM_SectorErase(EEPROM_ADDR_START);		//�״�����EEPROM����
	delayMs(10);
	
	eeprom_buffer[0] = BIRTHDAY_FLAG;			//���ձ��
	EEPROM_write_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	delayMs(10);
	
	eeprom_buffer[0] = 0;						//�豸���⿪
	EEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &eeprom_buffer[0], 1);
	delayMs(10);	
	
	memset(eeprom_buffer, 0, 12 * sizeof(u8));	//��ͨ���ض�ʱʱ�̱�����
	EEPROM_write_n(EEPROM_ADDR_swTimeTab, &eeprom_buffer[0], 13);
	delayMs(10);
	
	memset(eeprom_buffer, 0, 12 * sizeof(u8));	//ָ��Ȩ�޶�ʱʱ�̱�����
	EEPROM_write_n(EEPROM_ADDR_permissionTimeTab, &eeprom_buffer[0], 13);
	delayMs(10);
	
	memset(eeprom_buffer, 0, 4 * sizeof(u8));	//ָ��Ȩ�޶�ʱʱ��������
	EEPROM_write_n(EEPROM_ADDR_permissionKeepTab, &eeprom_buffer[0], 13);
	delayMs(10);
	
	memset(eeprom_buffer, 0, 13 * sizeof(u8));	//ָ��ģ������
	EEPROM_write_n(EEPROM_ADDR_FP_STORE, &eeprom_buffer[0], 13);
	delayMs(10);
	
	memcpy(eeprom_buffer, IP_default, 4);		//������IPģ������Ĭ��
	EEPROM_write_n(EEPROM_ADDR_serverIP_record, &eeprom_buffer[0], 4);
	delayMs(10);
	
	while(!fpID_allClear());		//���ָ�ƿ�
	delayMs(10);
	
	LogString("EEPROM�ָ���������,�ڲ��洢��գ�\n");
}

void birthDay_Judge(void){
	
	u8 xdata log_info[30] 		= {0};
	u8 		 eeprom_buffer[20]	= {0};
	u8 code	 IP_default[4]		= {47,52,5,108};		//Ĭ����۷�����
	
	delayMs(10);
	
#if(IF_REBORN != 1)		//����ǰ������ա���ת�����ã��޸�ͷ�ļ�IF_REBORN ֵ����
	
	EEPROM_read_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	delayMs(10);
	
	if(BIRTHDAY_FLAG != eeprom_buffer[0]){
	
		EEPROM_SectorErase(EEPROM_ADDR_START);		//�״�����EEPROM����
		delayMs(10);
		
		eeprom_buffer[0] = BIRTHDAY_FLAG;			//���ձ��
		EEPROM_write_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
		delayMs(10);
		
		eeprom_buffer[0] = 0;						//�豸���⿪
		EEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &eeprom_buffer[0], 1);
		delayMs(10);		
		
		memset(eeprom_buffer, 0, 12 * sizeof(u8));	//��ͨ���ض�ʱʱ�̱�����
		EEPROM_write_n(EEPROM_ADDR_swTimeTab, &eeprom_buffer[0], 13);
		delayMs(10);
		
		memset(eeprom_buffer, 0, 12 * sizeof(u8));	//ָ��Ȩ�޶�ʱʱ�̱�����
		EEPROM_write_n(EEPROM_ADDR_permissionTimeTab, &eeprom_buffer[0], 13);
		delayMs(10);
		
		memset(eeprom_buffer, 0, 4 * sizeof(u8));	//ָ��Ȩ�޶�ʱʱ��������
		EEPROM_write_n(EEPROM_ADDR_permissionKeepTab, &eeprom_buffer[0], 13);
		delayMs(10);
		
		memset(eeprom_buffer, 0, 13 * sizeof(u8));	//ָ��ģ������
		EEPROM_write_n(EEPROM_ADDR_FP_STORE, &eeprom_buffer[0], 13);
		delayMs(10);
		
		memcpy(eeprom_buffer, IP_default, 4);		//������IPģ������Ĭ��
		EEPROM_write_n(EEPROM_ADDR_serverIP_record, &eeprom_buffer[0], 4);
		delayMs(10);
		
		while(!fpID_allClear());		//���ָ�ƿ�
		delayMs(10);
		
		sprintf(log_info, "�״�����\n");
		
	}else{
	
		sprintf(log_info, "���״�����\n");
		
	}
	
	LogDats(log_info, strlen(log_info));
	
#else
	
	eeprom_buffer[0] = 0;		//���±�ǳ������Ǳ��
	coverEEPROM_write_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	
	delayMs(10);
	
	sprintf(log_info, "��Ʒ��¼�����,��Ʒ��������¸�λ!!!\n");
	LogDats(log_info, strlen(log_info));
	
	eeprom_buffer[0] = 0;
	EEPROM_read_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	sprintf(log_info, "��ǰ��Ʒ���ձ��Ϊ�� %02X\n", (int)eeprom_buffer[0]);
	LogDats(log_info, strlen(log_info));
	
#endif
}
