#include "timerAct.h"

#include "sensor_FP.h"
#include "HYM8564.h"
#include "datasReference.h"
#include "Relay.h"

#include "eeprom.h"
#include "soft_uart.h"

#include "stdio.h"
#include "string.h"

/****************本地文件变量定义区*************************/
switch_Status	swStatus_fromTim  				= swStatus_null;	//定时器更新开关命令标志，定时时刻到达时开关响应动作状态

u8 				swTim_onShoot_FLAG				= 0;	//普通开关定时一次性标志——低四位标识四个定时器
	
u8	 			permissionTim_oneShoot_FLAG 	= 0;	//指纹权限定时一次性标志——低四位标识四个定时器

bit				ifTim_sw_running_FLAG			= 0;	//普通开关定时运行标志位
bit				ifTim_permission_running_FLAG	= 0;	//指纹权限定时运行标志位

//***************指纹模块驱动线程相关变量引用区*************/

extern bit		FP_umDetect_enFLAG;		//指纹模块工作模式为：主动检测 模板时，工作使能标志，用于权限定时

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
	
		timDats_tab[loop].Week_Num	= (dats_Temp[loop * 3 + 0] & 0x7f) >> 0;	/*周占位数据*///低七位
		timDats_tab[loop].if_Timing = (dats_Temp[loop * 3 + 0] & 0x80) >> 7;	/*是否开启定时器数据*///高一位
		timDats_tab[loop].Status_Act= (dats_Temp[loop * 3 + 1] & 0xe0) >> 5;	/*定时响应状态数据*///高三位
		timDats_tab[loop].Hour		= (dats_Temp[loop * 3 + 1] & 0x1f) >> 0;	/*定时时刻_小时*///低五位
		timDats_tab[loop].Minute	= (dats_Temp[loop * 3 + 2] & 0xff) >> 0;	/*定时时刻_分*///全八位
	}
}

/*周占位判断*///判断当前周值是否在占位字节中
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

#define TIME_LOGOUT_EN	0	//当前时间log使能
	
	u8 loop = 0;
	
	stt_Time 			valTime_Local					= {0};	//当前时间缓存
	
	timing_Dats 		timDatsTemp_CalibrateTab[4] 	= {0};	/*定时起始时刻表缓存*///起始时刻及属性
	u16					Minute_Temp						= 0;	//分 计算缓存
	
	u8 					timDatsTemp_KeepTab[4]			= {0};	//定时时长表缓存

#if(TIME_LOGOUT_EN == 1)
	code	u8 			time_Logout_Period 				= 5;	//时间log周期——秒系数倍乘（据大循环手动调节）
	static 	u8 			time_Logout_Cnt					= 0;	//时间log等待周期计时
#endif
	
	timeRead(&valTime_Local);	//当前时间获取

#if(TIME_LOGOUT_EN == 1)	
	if(time_Logout_Cnt < time_Logout_Period)time_Logout_Cnt ++;
	else{
	
		time_Logout_Cnt = 0;
		time_Logout(valTime_Local);
	}
#endif
	
	/*普通开关定时*///四段数据=======================================================================================================<<<
	datsTiming_read_eeprom(timDatsTemp_CalibrateTab, timTabObj_swNormal);	/*普通开关*///时刻表读取
	
	/*判断是否所有普通开关定时都为关*/
	if((timDatsTemp_CalibrateTab[0].if_Timing == 0) &&	//全关，置标志位
	   (timDatsTemp_CalibrateTab[1].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[2].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[3].if_Timing == 0)
	  ){
	  
		ifTim_sw_running_FLAG = 0; 
		  
	}else{	//非全关，置标志位，并执行定时逻辑
		
		ifTim_sw_running_FLAG = 1; 
	
		for(loop = 0; loop < 4; loop ++){
			
			if(weekend_judge(valTime_Local.time_Week, timDatsTemp_CalibrateTab[loop].Week_Num)){	//周占位比对，成功才进行下一步
			
				if(timCount_ENABLE == timDatsTemp_CalibrateTab[loop].if_Timing){	//是否开启定时
					
					u8 log_dats[80] = {0};
					
					sprintf(log_dats, "有效定时：%d号, 定_时:%d, 定_分:%d \n", (int)loop, 
																			   (int)timDatsTemp_CalibrateTab[loop].Hour, 
																			   (int)timDatsTemp_CalibrateTab[loop].Minute);
					/*log调试打印*///普通定时定时信息
//					LogDats(log_dats, strlen(log_dats));
				
					if((valTime_Local.time_Hour 	== timDatsTemp_CalibrateTab[loop].Hour)&&	//时刻比对
					   (valTime_Local.time_Minute	== timDatsTemp_CalibrateTab[loop].Minute)&&
					   (valTime_Local.time_Second	<= 2)){	 //时刻比对时间限在前2秒
						   
//						LogString("time_UP!!!");
						
						//一次性定时判断
						if(swTim_onShoot_FLAG & (1 << loop)){	//是否为一次性定时，是则清空本段定时信息
							
							u8 code dats_Temp = 0;
							
							swTim_onShoot_FLAG &= ~(1 << loop);
							coverEEPROM_write_n(EEPROM_ADDR_swTimeTab + loop * 3, &dats_Temp, 1); //定时信息清空
						}
					   
						//普通开关动作响应
						if(timDatsTemp_CalibrateTab[loop].Status_Act & 0x01){	/*开启*/
						
							swStatus_fromTim = swStatus_on;
							
						}else{		/*关闭*/
						
							swStatus_fromTim = swStatus_off;
						}
					}
				}
			}
		}
	}

	/*指纹权限定时*///四段数据=======================================================================================================<<<
	datsTiming_read_eeprom(timDatsTemp_CalibrateTab, timTabObj_permission);	/*指纹权限*///时刻表读取
	EEPROM_read_n(EEPROM_ADDR_permissionKeepTab, timDatsTemp_KeepTab, 4);	/*指纹权限*///定时时长表读取
	
	/*判断是否所有指纹权限定时都为关*/
	if((timDatsTemp_CalibrateTab[0].if_Timing == 0) &&	//全关，置标志位
	   (timDatsTemp_CalibrateTab[1].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[2].if_Timing == 0) &&
	   (timDatsTemp_CalibrateTab[3].if_Timing == 0)
	  ){
	  
		ifTim_permission_running_FLAG = 0; 
		  
	}else{	//非全关，置标志位，并执行定时逻辑
		
		u16 tt_minuteStamp 				= 0,
			tt_minuteTemp_cmpA[4] 		= {0},
			tt_minuteTemp_cmpB[4] 		= {0};
		
		ifTim_permission_running_FLAG 	= 1; 
		
		/*当前时间转分为单位，易于区间比对*/
		tt_minuteStamp = valTime_Local.time_Hour * 60 + valTime_Local.time_Minute;
			
		memset(tt_minuteTemp_cmpA, 0, 4 * sizeof(u16));
		memset(tt_minuteTemp_cmpB, 0, 4 * sizeof(u16));
		for(loop = 0; loop < 4; loop ++){
		
			if(weekend_judge(valTime_Local.time_Week, timDatsTemp_CalibrateTab[loop].Week_Num)){	/*前周占位*///周占位比对，成功才进行下一步
				
				if(timDatsTemp_CalibrateTab[loop].if_Timing){
					
					if(timDatsTemp_KeepTab[loop] == 255){
					
						tt_minuteTemp_cmpA[loop] = 0;
						tt_minuteTemp_cmpB[loop] = 1439;
					}else{
					
						tt_minuteTemp_cmpA[loop] = timDatsTemp_CalibrateTab[loop].Hour * 60 + timDatsTemp_CalibrateTab[loop].Minute;	//定时_起点
						tt_minuteTemp_cmpB[loop] = tt_minuteTemp_cmpA[loop] + timDatsTemp_KeepTab[loop];
					}
				}
			}
		}
		
		/*log调试打印*///权限定时数据打印
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
			  
				/*权限动作响应*/
				if(timDatsTemp_CalibrateTab[loop].Status_Act & 0x01){	/*指纹触摸禁用使能*/
				
					FP_umDetect_enFLAG = 1;
					
				}else{	/*指纹触摸禁用失能*/
				
					FP_umDetect_enFLAG = 0;
				}
				
		  }else{
		  
				/*权限动作响应*///反转恢复
				if(timDatsTemp_CalibrateTab[loop].Status_Act & 0x01){	/*指纹触摸禁用失能*/

					FP_umDetect_enFLAG = 0;
					
				}else{	/*指纹触摸禁用使能*/

					FP_umDetect_enFLAG = 1;
				}
		  }
		
		for(loop = 0; loop < 4; loop ++){
				
			if(timDatsTemp_CalibrateTab[loop].if_Timing){
				
				if(tt_minuteStamp >= tt_minuteTemp_cmpB[loop]){
				
					if(permissionTim_oneShoot_FLAG & (1 << loop)){	//是否为一次性定时，是则清空本段定时信息
						
						u8 code dats_Temp = 0;
						
						permissionTim_oneShoot_FLAG &= ~(1 << loop);
						coverEEPROM_write_n(EEPROM_ADDR_permissionTimeTab + loop * 3, &dats_Temp, 1); //定时信息清空
						coverEEPROM_write_n(EEPROM_ADDR_permissionKeepTab + loop * 3, &dats_Temp, 1);
					}
				}
			}
		}
	}
}
