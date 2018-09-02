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

//*********************指纹模块相关变量引用区********************/
extern threadFP_Mode	process_FPworkMode;

//*******************WIFI驱动变量引用区***************************/
extern status_Mode		   SYS_status_Mode;	//系统状态

/********************本地文件变量创建区******************/
unsigned char xdata MAC_ID[6] = {0};

/*------------------------------------------------------------------*/
void software_Reset(void){

	process_FPworkMode	= work_Mode_NULL;			//指纹模块失能
	SYS_status_Mode 	= statusMode_Reset;			//系统状态更新
	
	Beep_Tips(2, 8, 3, 20, 20);	//提示音
	
	LogString("软件复位已触发. \n");
	((void(code *)(void))0x0000)();
	
	//无需状态恢复，重启自动初始化
}

void MAC_ID_Relaes(void){

	unsigned char xdata log_info[50] 	= {0};
	unsigned char code *id_ptr 			= ROMADDR_ROM_STC_ID;
	
	LogDats(log_info, strlen(log_info));

	memcpy(MAC_ID, id_ptr - 6, 6); //顺序向前，往前读，只取后六位
	
	sprintf(log_info, "MAC地址更新：%02X %02X %02X %02X %02X %02X\n", 
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
	u8 code	 IP_default[4]		= {47,52,5,108};		//默认香港服务器
	
	Beep_Tips(1, 8, 3, 100, 20);	//提示
	
	WIFI_hwRestoreFactory();
	
	EEPROM_SectorErase(EEPROM_ADDR_START);		//首次启动EEPROM擦除
	delayMs(10);
	
	eeprom_buffer[0] = BIRTHDAY_FLAG;			//生日标记
	EEPROM_write_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	delayMs(10);
	
	eeprom_buffer[0] = 0;						//设备锁解开
	EEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &eeprom_buffer[0], 1);
	delayMs(10);	
	
	memset(eeprom_buffer, 0, 12 * sizeof(u8));	//普通开关定时时刻表清零
	EEPROM_write_n(EEPROM_ADDR_swTimeTab, &eeprom_buffer[0], 13);
	delayMs(10);
	
	memset(eeprom_buffer, 0, 12 * sizeof(u8));	//指纹权限定时时刻表清零
	EEPROM_write_n(EEPROM_ADDR_permissionTimeTab, &eeprom_buffer[0], 13);
	delayMs(10);
	
	memset(eeprom_buffer, 0, 4 * sizeof(u8));	//指纹权限定时时长表清零
	EEPROM_write_n(EEPROM_ADDR_permissionKeepTab, &eeprom_buffer[0], 13);
	delayMs(10);
	
	memset(eeprom_buffer, 0, 13 * sizeof(u8));	//指纹模板清零
	EEPROM_write_n(EEPROM_ADDR_FP_STORE, &eeprom_buffer[0], 13);
	delayMs(10);
	
	memcpy(eeprom_buffer, IP_default, 4);		//服务器IP模板设置默认
	EEPROM_write_n(EEPROM_ADDR_serverIP_record, &eeprom_buffer[0], 4);
	delayMs(10);
	
	while(!fpID_allClear());		//清空指纹库
	delayMs(10);
	
	LogString("EEPROM恢复出厂设置,内部存储清空！\n");
}

void birthDay_Judge(void){
	
	u8 xdata log_info[30] 		= {0};
	u8 		 eeprom_buffer[20]	= {0};
	u8 code	 IP_default[4]		= {47,52,5,108};		//默认香港服务器
	
	delayMs(10);
	
#if(IF_REBORN != 1)		//出厂前配置清空——转世设置，修改头文件IF_REBORN 值即可
	
	EEPROM_read_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	delayMs(10);
	
	if(BIRTHDAY_FLAG != eeprom_buffer[0]){
	
		EEPROM_SectorErase(EEPROM_ADDR_START);		//首次启动EEPROM擦除
		delayMs(10);
		
		eeprom_buffer[0] = BIRTHDAY_FLAG;			//生日标记
		EEPROM_write_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
		delayMs(10);
		
		eeprom_buffer[0] = 0;						//设备锁解开
		EEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &eeprom_buffer[0], 1);
		delayMs(10);		
		
		memset(eeprom_buffer, 0, 12 * sizeof(u8));	//普通开关定时时刻表清零
		EEPROM_write_n(EEPROM_ADDR_swTimeTab, &eeprom_buffer[0], 13);
		delayMs(10);
		
		memset(eeprom_buffer, 0, 12 * sizeof(u8));	//指纹权限定时时刻表清零
		EEPROM_write_n(EEPROM_ADDR_permissionTimeTab, &eeprom_buffer[0], 13);
		delayMs(10);
		
		memset(eeprom_buffer, 0, 4 * sizeof(u8));	//指纹权限定时时长表清零
		EEPROM_write_n(EEPROM_ADDR_permissionKeepTab, &eeprom_buffer[0], 13);
		delayMs(10);
		
		memset(eeprom_buffer, 0, 13 * sizeof(u8));	//指纹模板清零
		EEPROM_write_n(EEPROM_ADDR_FP_STORE, &eeprom_buffer[0], 13);
		delayMs(10);
		
		memcpy(eeprom_buffer, IP_default, 4);		//服务器IP模板设置默认
		EEPROM_write_n(EEPROM_ADDR_serverIP_record, &eeprom_buffer[0], 4);
		delayMs(10);
		
		while(!fpID_allClear());		//清空指纹库
		delayMs(10);
		
		sprintf(log_info, "首次启动\n");
		
	}else{
	
		sprintf(log_info, "非首次启动\n");
		
	}
	
	LogDats(log_info, strlen(log_info));
	
#else
	
	eeprom_buffer[0] = 0;		//重新标记出厂覆盖标记
	coverEEPROM_write_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	
	delayMs(10);
	
	sprintf(log_info, "产品记录已清除,产品标记已重新复位!!!\n");
	LogDats(log_info, strlen(log_info));
	
	eeprom_buffer[0] = 0;
	EEPROM_read_n(EEPROM_ADDR_BirthdayMark, &eeprom_buffer[0], 1);
	sprintf(log_info, "当前产品生日标记为： %02X\n", (int)eeprom_buffer[0]);
	LogDats(log_info, strlen(log_info));
	
#endif
}
