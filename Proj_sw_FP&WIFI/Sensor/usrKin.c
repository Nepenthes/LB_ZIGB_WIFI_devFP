#include "usrKin.h"
#include "Tips.h"

#include "sensor_FP.h"
#include "wifi_LPT220.h"
#include "relay.h"
#include "delay.h"

#include "soft_uart.h"

#include "stdio.h"
#include "string.h"

//*********************指纹模块相关变量引用区******************/
extern threadFP_Mode	process_FPworkMode;

//*********************开关定时属性变量引用区******************/
extern rly_timFun 		relayTIM;

//*********************继电器驱动相关变量引用******************/
extern threadRelay_Mode	process_RelayWorkMode;
extern status_ifSave	relayStatus_ifSave;
extern bit 				status_Relay;

/**********************本地文件变量定义区**********************/
u8 idata val_DcodeCfm 			= 0;  //拨码值
bit		 ledBackground_method	= 0;  //背景灯颜色方案 //为1时：开-绿 关-蓝   为0时：开-蓝 关-绿
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
	const  u8 	comfirm_Period		= 3;	//消抖时间因数——取决于主线程调度周期
		
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
				LogString("WIFI模块已配置至 AP模式,等待重启\n");
			}else{
			
				while(!WIFI_WMODE_CHG(0) && tOut){delayMs(2000);  tOut --;}
				LogString("WIFI模块已配置至 STA模式,等待重启\n");
			}
			
			if(!tOut)((void(code *)(void))0x0000)();
			
			process_FPworkMode	= work_Mode_umDetect;		//指纹模块工作模式恢复到主动检测模式
			SYS_status_Mode 	= statusMode_NULL;			//系统状态更新恢复至初始空
		}
		
		if(val_Dcode_differ & Dcode_FLG_ifNONC){
			
			if(Dcode_modeMix(val_Dcode_Local) == 2 || Dcode_modeMix(val_Dcode_Local) == 3){ //延时模式下 NONC有效
			
				status_Relay = !status_Relay;
				CON_RELAY = status_Relay;
			
				if(val_Dcode_Local & Dcode_FLG_ifNONC){
					
//					ledBackground_method = 1;
					switch_NONC_method = 1;
					LogString("切换至背光方案A.\n");
					
				}else{
				
//					ledBackground_method = 0;
					switch_NONC_method = 0;
					LogString("切换至背光方案B.\n");
				}
			}
		}
		
		if(val_Dcode_differ & Dcode_FLG_saveRelay_MODEX){
		
			switch(Dcode_modeMix(val_Dcode_Local)){
			
				case 0:{
				
					process_RelayWorkMode = work_Mode_flip;
					relayStatus_ifSave	  = statusSave_disable;
					LogString("开关已切换至翻转模式.\n");
					LogString("开关记忆功能已关闭.\n");
					
				}break;
					
				case 1:{
				
					process_RelayWorkMode = work_Mode_flip;
					relayStatus_ifSave	  = statusSave_enable;
					LogString("开关记忆功能已开启.\n");
					
				}break;
					
				case 2:{
				
					process_RelayWorkMode = work_Mode_conuter;
					relayStatus_ifSave	  = statusSave_disable;
					LogString("开关已切换至延时模式.\n");
					
				}break;
					
				case 3:{
					
					process_RelayWorkMode = work_Mode_conuter;
					relayStatus_ifSave	  = statusSave_disable;				
					LogString("开关已切换至延时模式.\n");
					LogString("延时模式下，开关记忆无意义，开关记忆功能已关闭.\n");
					
				}break;
					
				default:break;
			}
		}
		
		if(val_Dcode_differ & Dcode_FLG_swDelay){
		
			switch(Dcode_cntTim(val_Dcode_Local)){
			
				case 0:{
				
					relayTIM.tim_Period = 1000;
					LogString("当前延时拨码已调至 1秒.\n");
				}break;
					
				case 1:{
				
					relayTIM.tim_Period = 3000;
					LogString("当前延时拨码已调至 3秒.\n");
				}break;
					
				case 2:{
				
					relayTIM.tim_Period = 5000;
					LogString("当前延时拨码已调至 5秒.\n");
				}break;
					
				case 3:{
				
					relayTIM.tim_Period = 10000;
					LogString("当前延时拨码已调至 10秒.\n");
				}break;
					
				default:break;
			}LogString("开关处于延时模式时时生效.\n");
		}
	}
}

void UsrKEYScan(funKey_Callback funCB_Short, funKey_Callback funCB_LongA, funKey_Callback funCB_LongB){
	
	code	u16	keyCfrmLoop_Short 	= 1,	//短按消抖时间,据大循环而定
				keyCfrmLoop_LongA 	= 3000,	//长按A时间,据大循环而定
				keyCfrmLoop_LongB 	= 15000,//长按B时间,据大循环而定
				keyCfrmLoop_MAX	 	= 60000;//计时封顶
	
	static	bit LongA_FLG = 0;
	static	bit LongB_FLG = 0;
	
	static	bit keyPress_FLG = 0;

	if(!UsrKEYScan_oneShoot()){		//选择提示
		
		keyPress_FLG = 1;
	
		if(!usrKeyCount_EN) usrKeyCount_EN= 1;	//计时
		
		if((usrKeyCount >= keyCfrmLoop_LongA) && (usrKeyCount <= keyCfrmLoop_LongB) && !LongA_FLG){
		
			process_FPworkMode	= work_Mode_NULL;			//指纹模块失能
			SYS_status_Mode 	= statusMode_smartLink;		//系统状态更新
			
			funCB_LongA();
			
			LongA_FLG = 1;
		}	
		
		if((usrKeyCount >= keyCfrmLoop_LongB) && (usrKeyCount <= keyCfrmLoop_MAX) && !LongB_FLG){
		
			process_FPworkMode	= work_Mode_NULL;			//指纹模块失能
			SYS_status_Mode 	= statusMode_ftRecover;		//系统状态更新
			
			funCB_LongB();
			
			LongB_FLG = 1;
		}
		
	}else{		//选择响应
		
		usrKeyCount_EN = 0;
		
		if(keyPress_FLG){
		
			keyPress_FLG = 0;
			
//			if(usrKeyCount < keyCfrmLoop_LongA && usrKeyCount > keyCfrmLoop_Short)funCB_Short();
			
			usrKeyCount = 0;
			LongA_FLG 	= 0;
			LongB_FLG 	= 0;
			
			SYS_status_Mode 	= statusMode_NULL;			//系统状态更新恢复至初始空
			process_FPworkMode	= work_Mode_umDetect;		//指纹模块工作模式恢复到主动检测模式
		}
	}
}


