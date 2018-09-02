#ifndef __DATASREFERENCE_H_
#define __DATASREFERENCE_H_

#define		SWITCH_TYPE_FP					 0x20	   	//开关类型：指纹开关

#define 	ROMADDR_ROM_STC_ID				 0x7ff8		//STC单片机 全球ID地址

#define 	EEPROM_USE_OF_NUMBER			 512		//指定EEPROM使用存储长度，用于定义读写缓存

#define		BIRTHDAY_FLAG					 0xA1		//产品出生标记

#define 	EEPROM_ADDR_START				 0x0000		//起始地址

#define		EEPROM_ADDR_BirthdayMark         0x0001		//01H - 01H 是否首次启动					01_Byte
#define  	EEPROM_ADDR_relayStatus          0x0002		//02H - 02H 开关状态存储					01_Byte
#define  	EEPROM_ADDR_timeZone_H           0x0003		//03H - 03H 时区——时						01_Byte
#define  	EEPROM_ADDR_timeZone_M           0x0004		//04H - 04H 时区——分						01_Byte
#define  	EEPROM_ADDR_deviceLockFLAG       0x0005		//05H - 05H 设备锁状态位					01_Byte
#define  	EEPROM_ADDR_swTimeTab          	 0x0010		//10H - 1CH 4组普通定时数据，每组3字节		12_Byte	
#define  	EEPROM_ADDR_permissionTimeTab    0x0020		//20H - 2CH 4组权限定时数据，每组3字节		12_Byte	
#define  	EEPROM_ADDR_permissionKeepTab    0x0030		//30H - 33H 4组权限定时时长数据				04_Byte	
#define		EEPROM_ADDR_serverIP_record    	 0x0040   	//40H - 43H 服务器IP存储					04_Byte
#define		EEPROM_ADDR_FP_STORE	         0x0050		//50H - 5DH 指纹占位存储					13_Byte
#define		EEPROM_ADDR_unDefine05           0x0000
#define		EEPROM_ADDR_unDefine06           0x0000
#define		EEPROM_ADDR_unDefine07           0x0000
#define		EEPROM_ADDR_unDefine08           0x0000
#define		EEPROM_ADDR_unDefine11           0x0000
#define		EEPROM_ADDR_unDefine12           0x0000
#define		EEPROM_ADDR_unDefine13           0x0000

#endif