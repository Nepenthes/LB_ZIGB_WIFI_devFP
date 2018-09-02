#include "Relay.h"
#include "Tips.h"
#include "usrKin.h"

#include "eeprom.h"

#include "dataTrans.h"
#include "datasReference.h"

//*********************指纹模块相关变量引用区*********************/
extern bit				FPvalidation_flg;

//*********************拨码部分相关变量引用区*********************/
extern u8 idata 		val_DcodeCfm;
extern bit		 		switch_NONC_method;

//*********************数据传输相关变量引用区*********************/
extern switch_Status	swStatus_fromUsr; //上位机直接下达的开关命令

//*********************定时检测进程相关变量引用区*********************/
extern switch_Status	swStatus_fromTim;  //定时时刻到达时开关响应动作状态

/********************************本地文件变量定义区********************************/
rly_timFun 			relayTIM 				= {false, 1000};		//定时属性
threadRelay_Mode	process_RelayWorkMode 	= work_Mode_flip;		//主线程工作模式
status_ifSave		relayStatus_ifSave		= statusSave_disable;	//开关记忆使能变量
bit 				status_Relay 			= false;				//继电器状态_仅仅暴露给其它线程读取状态使用，如需更变状态必须调用函数relay_Act();,切不可直接改写本状态值

/*----------------------------------------------------------------*/
/*开关继电器初始化*/
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

/*开关动作*/
void relay_Act(rly_methodType act_Method, bool timer_EN){
	
	u8 statusTemp = 0;
	
	Beep_Tips(1, 8, 2, 20, 10);	//提示音
	
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
	
	if(relayStatus_ifSave == statusSave_enable){	//开关状态记忆
	
		statusTemp = status_Relay;
		coverEEPROM_write_n(EEPROM_ADDR_relayStatus, &statusTemp, 1);
	}
}

/*继电器主线程*/
void thread_Relay(void){

	switch(process_RelayWorkMode){
	
		/*翻转模式*///开关开启或关闭全部依赖指令，指令来一次开，再来一次就关，依此反复
		case work_Mode_flip:{
		
			if(FPvalidation_flg){	//本地触摸开关业务逻辑
			
				FPvalidation_flg = 0;
				relay_Act(relay_flip, 0);
			}
			
			switch(swStatus_fromUsr){	//上位机下达开关业务逻辑
			
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
			
			switch(swStatus_fromTim){	//定时时刻表触发开关业务逻辑
			
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
		
		/*延时模式*///延时时间到达后自动关闭
		case work_Mode_conuter:{	//本地触摸开关业务逻辑
		
			if(FPvalidation_flg){
			
				FPvalidation_flg = 0;
				relay_Act(relay_on, 1);
			}
			
			switch(swStatus_fromUsr){	//上位机下达开关业务逻辑
			
				case swStatus_on:{
				
					swStatus_fromUsr = swStatus_null;
					relay_Act(relay_on, 1);//延时使能
				}break;
				
				case swStatus_off:{
				
					swStatus_fromUsr = swStatus_null;
					relay_Act(relay_off, 0);
				}break;
				
				default:break;
			}
			
			switch(swStatus_fromTim){	//定时时刻表触发开关业务逻辑
			
				case swStatus_on:{
				
					swStatus_fromTim = swStatus_null;
					relay_Act(relay_on, 1);//延时使能
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