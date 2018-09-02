#ifndef __DATASREFERENCE_H_
#define __DATASREFERENCE_H_

#define		SWITCH_TYPE_FP					 0x20	   	//�������ͣ�ָ�ƿ���

#define 	ROMADDR_ROM_STC_ID				 0x7ff8		//STC��Ƭ�� ȫ��ID��ַ

#define 	EEPROM_USE_OF_NUMBER			 512		//ָ��EEPROMʹ�ô洢���ȣ����ڶ����д����

#define		BIRTHDAY_FLAG					 0xA1		//��Ʒ�������

#define 	EEPROM_ADDR_START				 0x0000		//��ʼ��ַ

#define		EEPROM_ADDR_BirthdayMark         0x0001		//01H - 01H �Ƿ��״�����					01_Byte
#define  	EEPROM_ADDR_relayStatus          0x0002		//02H - 02H ����״̬�洢					01_Byte
#define  	EEPROM_ADDR_timeZone_H           0x0003		//03H - 03H ʱ������ʱ						01_Byte
#define  	EEPROM_ADDR_timeZone_M           0x0004		//04H - 04H ʱ��������						01_Byte
#define  	EEPROM_ADDR_deviceLockFLAG       0x0005		//05H - 05H �豸��״̬λ					01_Byte
#define  	EEPROM_ADDR_swTimeTab          	 0x0010		//10H - 1CH 4����ͨ��ʱ���ݣ�ÿ��3�ֽ�		12_Byte	
#define  	EEPROM_ADDR_permissionTimeTab    0x0020		//20H - 2CH 4��Ȩ�޶�ʱ���ݣ�ÿ��3�ֽ�		12_Byte	
#define  	EEPROM_ADDR_permissionKeepTab    0x0030		//30H - 33H 4��Ȩ�޶�ʱʱ������				04_Byte	
#define		EEPROM_ADDR_serverIP_record    	 0x0040   	//40H - 43H ������IP�洢					04_Byte
#define		EEPROM_ADDR_FP_STORE	         0x0050		//50H - 5DH ָ��ռλ�洢					13_Byte
#define		EEPROM_ADDR_unDefine05           0x0000
#define		EEPROM_ADDR_unDefine06           0x0000
#define		EEPROM_ADDR_unDefine07           0x0000
#define		EEPROM_ADDR_unDefine08           0x0000
#define		EEPROM_ADDR_unDefine11           0x0000
#define		EEPROM_ADDR_unDefine12           0x0000
#define		EEPROM_ADDR_unDefine13           0x0000

#endif