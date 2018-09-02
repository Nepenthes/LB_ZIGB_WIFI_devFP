#include "dataTrans.h"

#include "stdio.h"
#include "string.h"

#include "datasReference.h"
#include "pars_Method.h"
#include "sensor_FP.h"
#include "dataManage.h"
#include "wifi_LPT220.h"
#include "HYM8564.h"
#include "Relay.h"
#include "Tips.h"

#include "USART.h"
#include "soft_uart.h"
#include "GPIO.h"
#include "eeprom.h"
#include "delay.h"

//*********************指纹模块相关变量引用区********************/
extern threadFP_Mode	process_FPworkMode;
extern stt_stepAdd 		umNew_AddStep;
extern u8 				FP_ID;

//*********************MAC地址相关变量引用区*********************/
extern u8 xdata 		MAC_ID[6];

//*********************继电器驱动相关变量引用区******************/
extern threadRelay_Mode	process_RelayWorkMode;
extern bit 				status_Relay;

//*********************定时动作线程相关变量引用区****************/
extern	u8 				swTim_onShoot_FLAG;		//普通开关定时一次性标志——低四位标识四个定时器
extern	u8	 			permissionTim_oneShoot_FLAG;	//指纹权限定时一次性标志——低四位标识四个定时器

extern	bit				ifTim_sw_running_FLAG;	//普通开关定时运行标志位
extern	bit				ifTim_permission_running_FLAG;	//指纹权限定时运行标志位

extern 	bit				FP_umDetect_enFLAG;	//指纹模块工作模式为：主动检测 模板时，工作使能标志

/**********************本地文件变量定义区************************/
//串口接收超时标志
bit uartRX_toutFLG 						= 0;
//串口接收超时计数
bit rxTout_count_EN 					= 0;
u8  rxTout_count 						= 0;
//串口数据缓存
u8	datsRcv_length						= 0;
uartTout_datsRcv xdata datsRcv_ZIGB 	= {{0}, 0};

//当前接收上位机模式标识
dataTrans_obj 	dataTrans_objWIFI 		= DATATRANS_objFLAG_MOBILE;

//上位机下达开关命令标志
switch_Status	swStatus_fromUsr  		= swStatus_null;	

//远端MACID缓存
u8				Dst_MACID_Temp[6] 		= {0};

//数据包响应回复长度——默认局域网数据长度
u8 xdata 		repeatTX_Len			= dataTransLength_objMOBILE;

//设备锁标志
bit				deviceLock_flag			= false;

//心跳包奇偶标志
bit 			heartBeat_Type 			= 0;	//奇偶包标志，奇数包1，偶书包0
//心跳包周期计数值
u16				heartBeat_Cnt			= 0;
//心跳包计数周期
const 	u16 	heartBeat_Period 		= 8000; //心跳包周期设置值 秒数 * 系数 据大循环而定	
/*--------------------------------------------------------------*/
void uartObjWIFI_Init(void){

	COMx_InitDefine val_uartInit;
	
	//TX推挽输出
	P3M1 &= 0xFD;	
	P3M0 |= 0x02;	
	
	//RX高阻输入
	P3M1 |= 0x01;
	P3M0 &= 0xFE;

	val_uartInit.UART_Mode			= UART_8bit_BRTx;	//模式,         UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
	val_uartInit.UART_BRT_Use		= BRT_Timer1;		//使用波特率,   BRT_Timer1,BRT_Timer2
	val_uartInit.UART_BaudRate		= BAUD_WIFI;		//波特率,       ENABLE,DISABLE
	val_uartInit.Morecommunicate	= ENABLE;			//多机通讯允许, ENABLE,DISABLE
	val_uartInit.UART_RxEnable		= ENABLE;			//允许接收,   	ENABLE,DISABLE
	val_uartInit.BaudRateDouble		= ENABLE;			//波特率加倍, 	ENABLE,DISABLE
	val_uartInit.UART_Interrupt		= ENABLE;			//中断控制,   	ENABLE,DISABLE
	val_uartInit.UART_Polity		= PolityLow;		//优先级,     	PolityLow,PolityHigh
	val_uartInit.UART_P_SW			= UART1_SW_P30_P31;	//切换端口,   	UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17(必须使用内部时钟)
	val_uartInit.UART_RXD_TXD_Short	= DISABLE;			//内部短路RXD与TXD, 做中继, ENABLE,DISABLE
	
	USART_Configuration(USART1, &val_uartInit);
	
	memset(TX1_Buffer, 0, sizeof(char) * COM_TX1_Lenth);
	
	PrintString1("i'm UART1 for wifi data translate !!!");
}

void uartObjFP_Init(void){

	COMx_InitDefine val_uartInit;
	
	//TX推挽输出
	P1M1 &= 0xFD;	
	P1M0 |= 0x02;	
	
	//RX高阻输入
	P1M1 |= 0x01;
	P1M0 &= 0xFE;

	val_uartInit.UART_Mode			= UART_8bit_BRTx;	//模式,         UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
	val_uartInit.UART_BRT_Use		= BRT_Timer2;		//使用波特率,   BRT_Timer1,BRT_Timer2
	val_uartInit.UART_BaudRate		= BAUD_FP;			//波特率,       ENABLE,DISABLE
	val_uartInit.Morecommunicate	= DISABLE;			//多机通讯允许, ENABLE,DISABLE
	val_uartInit.UART_RxEnable		= ENABLE;			//允许接收,   	ENABLE,DISABLE
	val_uartInit.BaudRateDouble		= ENABLE;			//波特率加倍, 	ENABLE,DISABLE
	val_uartInit.UART_Interrupt		= ENABLE;			//中断控制,   	ENABLE,DISABLE
	val_uartInit.UART_Polity		= PolityLow;		//优先级,     	PolityLow,PolityHigh
	val_uartInit.UART_P_SW			= UART2_SW_P10_P11;	//切换端口,   	UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17(必须使用内部时钟)
	val_uartInit.UART_RXD_TXD_Short	= DISABLE;			//内部短路RXD与TXD, 做中继, ENABLE,DISABLE
	
	while(3 != USART_Configuration(USART2, &val_uartInit));
	
	memset(TX2_Buffer, 0, sizeof(char) * COM_TX2_Lenth);
	
	PrintString2("i'm UART2 for fingerprint data translate !!!");
}

/*----------------------------
发送串口数据
----------------------------*/
void uartObjWIFI_Send_Byte(u8 dat)	//串口1
{
	TX1_write2buff(dat);
}

void uartObjFP_Send_Byte(u8 dat)  	//串口2
{
	TX2_write2buff(dat);
}

void uartObjWIFI_Send_String(char *s,unsigned char ucLength){	 //串口1
	
	uart1_datsSend(s, ucLength);
}

void uartObjFP_Send_String(char *s,unsigned char ucLength){	 //串口1
	
	uart2_datsSend(s, ucLength);
}

void rxBuff_WIFI_Clr(void){

	memset(rxBuff_WIFI, 0xff, sizeof(char) * COM_RX1_Lenth);
	COM1.RX_Cnt = 0;
}

/********************* UART1(WIIF)中断函数_自定义重构************************/
void UART1_Rountine (void) interrupt UART1_VECTOR
{
	if(RI)
	{
		RI = 0;
		if(COM1.B_RX_OK == 0)
		{
			
//			switch(WIFI_Uart_RcvMode){		//区别处理功能保留
//			
//				case Mode_cmdAT:{
//	
//					
//				}break;
//				
//				case Mode_datParsing:{
//				
//					
//				}break;
//				
//				default:break;
//			}
			
//			if(COM1.RX_Cnt >= COM_RX1_Lenth)	COM1.RX_Cnt = 0;
//			RX1_Buffer[COM1.RX_Cnt++] = SBUF;
//			COM1.RX_TimeOut = TimeOutSet1;
			
			if(!rxTout_count_EN){
			
				rxTout_count_EN = 1;
				rxTout_count 	= 0;
				datsRcv_length  = 0;
				
				memset(RX1_Buffer, 0xff, sizeof(char) * COM_RX1_Lenth);
			}
			
			RX1_Buffer[datsRcv_length ++] = SBUF;
			rxTout_count = 0;	
		}
	}

	if(TI)
	{
		TI = 0;
		if(COM1.TX_read != COM1.TX_write)
		{
		 	SBUF = TX1_Buffer[COM1.TX_read];
			if(++COM1.TX_read >= COM_TX1_Lenth)		COM1.TX_read = 0;
		}
		else	COM1.B_TX_busy = 0;
	}
}

/********************* 自定义校验*****************************/
unsigned char frame_Check(unsigned char frame_temp[], u8 check_num){
  
	unsigned char loop 		= 0;
	unsigned char val_Check = 0;
	
	for(loop = 0; loop < check_num; loop ++){
	
		val_Check += frame_temp[loop];
	}
	
	val_Check  = ~val_Check;
	val_Check ^= 0xa7;
	
	return val_Check;
}

/*此数据封装必须在数据包发送前最后调用，自动根据上次数据传输时的对象进行数据封装*///避免校验被提前而出错
void dtasTX_loadBasic_AUTO(u8 dats_Tx[45],		//发送包数据缓存基本信息填装
						   u8 frame_Type,
						   u8 frame_CMD,
						   bit ifSpecial_CMD){	
						  
	switch(repeatTX_Len){
	
		/*服务器数据包填装*///45字节
		case dataTransLength_objSERVER:{
			
			u8 datsTemp[32] = {0};
		
			dats_Tx[0] 	= FRAME_HEAD_SERVER;
			
			memcpy(&datsTemp[0], &dats_Tx[1], 32);
			memcpy(&dats_Tx[13], &datsTemp[0], 32);	//帧头后拉开空出12个字节
			memcpy(&dats_Tx[1],  &Dst_MACID_Temp[0], 6);	//远端MACID填充
			memcpy(&dats_Tx[8],  &MAC_ID[1], 5);	/*dats_Tx[7] 暂时填0*///远端MACID填充
			
			dats_Tx[1 + 12] = dataTransLength_objSERVER;
			dats_Tx[2 + 12] = frame_Type;
			dats_Tx[3 + 12] = frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10 + 12] = SWITCH_TYPE_FP;	//开关类型填充
			
			memcpy(&dats_Tx[5 + 12], &MAC_ID[1], 5);	//MAC填充
								  
			dats_Tx[4 + 12] = frame_Check(&dats_Tx[5 + 12], 28);	
			
		}break;
		
		/*局域网数据包填装*///33字节
		case dataTransLength_objMOBILE:{
		
			dats_Tx[0] 	= FRAME_HEAD_MOBILE;
			
			dats_Tx[1] 	= dataTransLength_objMOBILE;
			dats_Tx[2] 	= frame_Type;
			dats_Tx[3] 	= frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10] = SWITCH_TYPE_FP;	//开关类型填充
			
			memcpy(&dats_Tx[5], &MAC_ID[1], 5);	//MAC填充
								  
			dats_Tx[4] 	= frame_Check(&dats_Tx[5], 28);
			
		}break;
		
		default:break;
	}	
}
						   
/*此数据封装必须在数据包发送前最后调用，自定义对象进行数据封装*///避免校验被提前而出错
void dtasTX_loadBasic_CUST(dataTrans_obj obj_custom,	//发送包数据缓存基本信息填装
						   u8 dats_Tx[45],		
						   u8 frame_Type,
						   u8 frame_CMD,
						   bit ifSpecial_CMD){	
						  
	switch(obj_custom){
	
		/*服务器数据包填装*///45字节
		case DATATRANS_objFLAG_SERVER:{
			
			u8 datsTemp[32] = {0};
		
			dats_Tx[0] 	= FRAME_HEAD_SERVER;
			
			memcpy(&datsTemp[0], &dats_Tx[1], 32);
			memcpy(&dats_Tx[13], &datsTemp[0], 32);	//帧头后拉开空出12个字节
			memcpy(&dats_Tx[1],  &Dst_MACID_Temp[0], 6);	//远端MACID填充
			memcpy(&dats_Tx[8],  &MAC_ID[1], 5);	/*dats_Tx[7] 暂时填0*///远端MACID填充
			
			dats_Tx[1 + 12] = dataTransLength_objSERVER;
			dats_Tx[2 + 12] = frame_Type;
			dats_Tx[3 + 12] = frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10 + 12] = SWITCH_TYPE_FP;	//开关类型填充
			
			memcpy(&dats_Tx[5 + 12], &MAC_ID[1], 5);	//MAC填充
								  
			dats_Tx[4 + 12] = frame_Check(&dats_Tx[5 + 12], 28);	
			
		}break;
		
		/*局域网数据包填装*///33字节	--除了45都是33填装
		case DATATRANS_objFLAG_MOBILE:
		
		default:{
		
			dats_Tx[0] 	= FRAME_HEAD_MOBILE;
			
			dats_Tx[1] 	= dataTransLength_objMOBILE;
			dats_Tx[2] 	= frame_Type;
			dats_Tx[3] 	= frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10] = SWITCH_TYPE_FP;	//开关类型填充
			
			memcpy(&dats_Tx[5], &MAC_ID[1], 5);	//MAC填充
								  
			dats_Tx[4] 	= frame_Check(&dats_Tx[5], 28);
		}break;
	}	
}
					  
void datsTX_loadHeartBeat(u8 dats_Tx[dataHeartBeatLength_objSERVER],		//发送心跳包数据缓存基本信息填装
					  u8 frame_Head,
					  u8 frame_Len,
					  u8 frame_CMD,
					  u8 ifLock,
					  u8 infoTcnt,
					  u8 infoStatus,
					  u8 ifGreen,
					  u8 ifMnight){
	
	memset(dats_Tx, 0, dataHeartBeatLength_objSERVER * sizeof(u8));

	dats_Tx[0] = frame_Head;			//帧头填充
	dats_Tx[1] = frame_Len;
	dats_Tx[2] = frame_CMD;
						  
//	MAC_ID_Relaes();
	memcpy(&dats_Tx[4], &MAC_ID[1], 5);	//MAC填充
						  
	dats_Tx[9] 	= ifLock;
	dats_Tx[10] = infoTcnt;
	dats_Tx[11] = infoStatus;
	dats_Tx[12] = ifGreen;
	dats_Tx[13] = ifMnight;
}

/***********手动立马更新心跳包**************************/
void heartBeat_Release_Immediately(void){
	
	heartBeat_Cnt 	= heartBeat_Period;	//立即更新偶数心跳——状态包
	heartBeat_Type	= 0;	//偶数包
}

/***********主动数据检测检测立马更新心跳包**************/
void thread_LocalStaus_Release(void){

	static bit				Local_ifTim_sw_running_FLAG			= 0;
	static bit				Local_ifTim_permission_running_FLAG	= 0;
	static threadRelay_Mode	Local_process_RelayWorkMode 		= work_Mode_flip;
	static bit 				Local_status_Relay 					= false;
	static bit				Local_deviceLock_flag  				= false;
	
	if((Local_ifTim_sw_running_FLAG 		!= ifTim_sw_running_FLAG) ||
	   (Local_ifTim_permission_running_FLAG != ifTim_permission_running_FLAG) ||
	   (Local_process_RelayWorkMode			!= process_RelayWorkMode) ||
	   (Local_status_Relay					!= status_Relay) ||
	   (Local_deviceLock_flag				!= deviceLock_flag) 
	){
		
		heartBeat_Cnt 	= heartBeat_Period;	//立即更新偶数心跳——状态包
		heartBeat_Type	= 0;	//偶数包
			
		Local_ifTim_sw_running_FLAG 		= ifTim_sw_running_FLAG;
		Local_ifTim_permission_running_FLAG = ifTim_permission_running_FLAG;
		Local_process_RelayWorkMode			= process_RelayWorkMode;
		Local_status_Relay					= status_Relay;
		Local_deviceLock_flag				= deviceLock_flag;
	}
}

/********************* 数据传输及解析************************/
void thread_dataTrans(void){
	
	u8 data frameFun_Dats_KEY = 0;
	
	unsigned char timeZone_Hour 			= 0;
	unsigned char timeZone_Minute			= 0;
	
	unsigned char xdata log_info[80] 		= {0};
	unsigned char xdata repeatTX_buff[45]	= {0};
	
	unsigned char rxBuff_WIFI_temp[45]		= {0};
		
	unsigned char frameLength				= 0;

	bit datsQualified_FLG					= 0;	
	bit Parsing_EN 							= 0;
	
	data u8 heartBeat_Pack[14] 				= {0};	
	
	/*********************************数据包采集**********************************/
	if(uartRX_toutFLG){ //超时断帧
	
		uartRX_toutFLG = 0;
		
		memset(rxBuff_WIFI_temp, 0, 45 * sizeof(u8));
		
		if((datsRcv_ZIGB.rcvDats[0] == FRAME_HEAD_SERVER) && 
		   (datsRcv_ZIGB.rcvDats[13] == dataTransLength_objSERVER) &&
		   (datsRcv_ZIGB.rcvDats[14] == FRAME_TYPE_MtoS_CMD) &&
		   (datsRcv_ZIGB.rcvDatsLen >= dataTransLength_objSERVER)){ 
			
			memcpy(&Dst_MACID_Temp[0], &datsRcv_ZIGB.rcvDats[7], 6);  //远端MACID暂存	
			rxBuff_WIFI_temp[0] = datsRcv_ZIGB.rcvDats[0]; //帧头赋值
			memcpy(&rxBuff_WIFI_temp[1], &datsRcv_ZIGB.rcvDats[13], 32);	//数据加载，帧对齐，对齐成33字节
			
			//响应回复对象属性更新	
			dataTrans_objWIFI = DATATRANS_objFLAG_SERVER;
			repeatTX_Len	  = dataTransLength_objSERVER;
			
			frameLength = dataTransLength_objSERVER;
			datsQualified_FLG = 1;
				
		}else
		if((datsRcv_ZIGB.rcvDats[0] == FRAME_HEAD_MOBILE) && 
		   (datsRcv_ZIGB.rcvDats[1] == dataTransLength_objMOBILE) &&
		   (datsRcv_ZIGB.rcvDats[2] == FRAME_TYPE_MtoS_CMD) &&
		   (datsRcv_ZIGB.rcvDatsLen == dataTransLength_objMOBILE)){
		
			memcpy(rxBuff_WIFI_temp, datsRcv_ZIGB.rcvDats, dataTransLength_objMOBILE);	//数据加载
			
			//响应回复对象属性更新	
			dataTrans_objWIFI = DATATRANS_objFLAG_MOBILE;
			repeatTX_Len	  = dataTransLength_objMOBILE;
				
			frameLength = dataTransLength_objMOBILE;
			datsQualified_FLG = 1;
				
		}else
		if((datsRcv_ZIGB.rcvDats[0] == FRAME_HEAD_HEARTBEAT) && 
		   (datsRcv_ZIGB.rcvDats[1] == dataHeartBeatLength_objSERVER) &&
		   (datsRcv_ZIGB.rcvDatsLen == dataHeartBeatLength_objSERVER)){
			
			memcpy(rxBuff_WIFI_temp, datsRcv_ZIGB.rcvDats, dataHeartBeatLength_objSERVER);	//数据加载
			
			//响应回复对象属性更新	--不进行repeatTX_Len更新！！！
			dataTrans_objWIFI = HEARTBEAT_objFLAG_SERVER;
			   
			frameLength = dataHeartBeatLength_objSERVER;
			datsQualified_FLG = 1;
			   
		}else{
		
//			dataTrans_objWIFI = dataTrans_obj_NULL;
			datsQualified_FLG = 0;
		}
	}
	
	/*********************************数据包类型甄别**********************************/
	if(datsQualified_FLG){
		
		bit frameCodeCheck_PASS = 0; //校验码检查通过标志
		bit frameMacCheck_PASS  = 0; //mac地址待检查通过标志
		
		datsQualified_FLG = 0;
	
		switch(frameLength){
		
			case dataTransLength_objSERVER:
			case dataTransLength_objMOBILE:{
			
				if(rxBuff_WIFI_temp[4] == frame_Check(&rxBuff_WIFI_temp[5], 28))frameCodeCheck_PASS = 1; //校验码检查
				if(!memcmp(&rxBuff_WIFI_temp[5], &MAC_ID[1], 5))frameMacCheck_PASS  = 1; //mac地址检查
				
				if(rxBuff_WIFI_temp[3] == FRAME_MtoSCMD_cmdCfg_swTim){ //不用进行校验码检查的指令
				
					frameCodeCheck_PASS = 1;	
					
				}
				if(rxBuff_WIFI_temp[3] == FRAME_MtoSCMD_cmdConfigSearch){ //不用进行mac地址检查的指令
				
					frameMacCheck_PASS = 1;	
					
				}
				
				if(frameMacCheck_PASS & frameMacCheck_PASS){ //指令验证
					
					Parsing_EN = 1;	
					
				}else{
				
//					if(!frameMacCheck_PASS)uartObjWIFI_Send_String("fuck mac", 8);
//					if(!frameMacCheck_PASS)uartObjWIFI_Send_String("fuck code", 9);rxBuff_WIFI_temp
//					uartObjWIFI_Send_String(rxBuff_WIFI_temp, 33);
				}
				
			}break;
			
			case dataHeartBeatLength_objSERVER:{
			
				if(!memcmp(&rxBuff_WIFI_temp[3], &MAC_ID[1], 5)){	//MAC地址本地比对
					
					Parsing_EN = 1;	
				}
				
			}break;
			
			default:{
		
				Parsing_EN = 0;	
				
			}break;
		}
	}
	
	/*********************************数据包开始解析响应**********************************/	
	if(Parsing_EN){	//开始解析
		
		Parsing_EN = 0;
		
		delayMs(10); //防卡顿延时
		
		switch(dataTrans_objWIFI){
		
			//服务器
			case DATATRANS_objFLAG_SERVER:
			//局域网
			case DATATRANS_objFLAG_MOBILE:{
				
					bit specialCmd_FLAG = 0;	/*特殊数据封装标志*///数据1是否占用（原为开关类型）
					bit respond_FLAG	= 1;	/*是否相应回复标志*///数据接收后是否主动回复响应
				
					u8 cmdParing_Temp = rxBuff_WIFI_temp[3];

					switch(cmdParing_Temp){		//命令解析及识别

						/*控制指令*/
						case FRAME_MtoSCMD_cmdControl:{		
							
							switch(rxBuff_WIFI_temp[11]){
							
								case 0x00:{		//开关_开
								
									swStatus_fromUsr = swStatus_off;
								}break;
									
								case 0x01:{		//开关_关
								
									swStatus_fromUsr = swStatus_on;
								}break;
								
								default:break;
							}
							
							//数据响应及回复
							repeatTX_buff[15]	= FPID_NULL;	//无线遥控，指纹号空填充
							repeatTX_buff[11]	= rxBuff_WIFI_temp[11];		//开关状态更新填充
							(process_RelayWorkMode == work_Mode_flip)?(repeatTX_buff[14] = 0x01):(repeatTX_buff[14] = 0x02);//开关工作模式填充 翻转模式：0x01 延时模式：0x02
							
							sprintf(log_info, "接收到控制指令\n");
							
						}break;
					
						/*指纹学习指令*/
						case FRAME_MtoSCMD_cmdFPLearn:{		
							
//							LogDats(rxBuff_WIFI_temp, 33);
						
							sprintf(log_info, "接收到指纹学习指令，学习模板ID号为：%u\n", (int)rxBuff_WIFI_temp[15] + 1);
							if(FPID_NULL != rxBuff_WIFI_temp[15]){
							
								FP_ID = rxBuff_WIFI_temp[15];
								fpModeTrig_umAdd();	//指纹驱动线程干预_触发学习模式
							}
							
							//数据响应，已进入学习录入状态
							repeatTX_buff[15]	= FP_ID;
							
						}break;
						
						/*指纹删除指令*/
						case FRAME_MtoSCMD_cmdFPDelete:{	
						
							sprintf(log_info, "接收到指纹模板删除指令，删除模板ID号为：%u\n", (int)rxBuff_WIFI_temp[15] + 1);
							if(FPID_NULL != rxBuff_WIFI_temp[15]){
							
								FP_ID = rxBuff_WIFI_temp[15];
								fpModeTrig_umDel();	//指纹驱动线程干预_触发删除模式
							}
							
							//数据响应及回复在指纹驱动内完成
							respond_FLAG = 0;
							
						}break;
						
						/*指纹查询指令*/
						case FRAME_MtoSCMD_cmdFPQuery:{		
						
							u8 data fpID_Tab[13] = {0};
								
							sprintf(log_info, "接收到指纹模板查询指令，将返回指纹占位数据.\n");
							
							//数据响应及回复
							EEPROM_read_n(EEPROM_ADDR_FP_STORE, &fpID_Tab[0], 13);	
							memcpy(&repeatTX_buff[16], &fpID_Tab[0], 13);
							
						}break;
						
						/*配置搜索指令*/
						case FRAME_MtoSCMD_cmdConfigSearch:{	
							
							u8 deviceLock_IF = 0;
											
							EEPROM_read_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
							
							sprintf(log_info, "接收到smartlink 配置/搜索 指令.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							heartBeat_Release_Immediately(); //心跳包随即更新
							
							//上锁检测
							if(!deviceLock_IF){	
								
								u8 xdata serverIP_temp[4] = {0};
								memcpy(&serverIP_temp[0], &rxBuff_WIFI_temp[6], 4);
							
								//服务器地址同步
								if(WIFI_serverUDP_CHG(serverIP_temp)){
								
									coverEEPROM_write_n(EEPROM_ADDR_serverIP_record, &serverIP_temp[0], 4); //更新eeprom
									
									sprintf(log_info,
											"服务器IP同步成功,当前服务器IP为:%d.%d.%d.%d\n",
											(int)serverIP_temp[0],
											(int)serverIP_temp[1],
											(int)serverIP_temp[2],
											(int)serverIP_temp[3]);
								}else{
								
									sprintf(log_info, "服务器IP同步失败,继续使用本地存储的服务器IP.\n");
								}
								LogDats(log_info, strlen(log_info));
								memset(log_info, 0, 80 * sizeof(u8));
								
								//时间信息同步
								timeZone_Hour 	= rxBuff_WIFI_temp[12];		//时区更新
								timeZone_Minute	= rxBuff_WIFI_temp[13];
								coverEEPROM_write_n(EEPROM_ADDR_timeZone_H, &timeZone_Hour, 1);
								coverEEPROM_write_n(EEPROM_ADDR_timeZone_M, &timeZone_Minute, 1);
								sprintf(log_info, "时区本地同步更新完成,当前时区为:时-%d,分-%d.\n",(int)timeZone_Hour,
																								   (int)timeZone_Minute);
								LogDats(log_info, strlen(log_info));
								memset(log_info, 0, 80 * sizeof(u8));
								
								sprintf(log_info, "配置信息本地同步更新完成.\n");
								
								//数据响应及回复
								(process_RelayWorkMode == work_Mode_flip)?(repeatTX_buff[14] = 0x01):(repeatTX_buff[14] = 0x02);//开关工作模式填充 翻转模式：0x01 延时模式：0x02
								
							}else{
								
								respond_FLAG = false;
								sprintf(log_info, "设备已上锁,拒绝回复响应\n");
							}
							
						}break;
						
						/*查询登录指令*/
						case FRAME_MtoSCMD_cmdQuery:{	
							
							u8 deviceLock_IF = 0;
							
							EEPROM_read_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);

							if(deviceLock_IF){
							
								//数据响应及回复
								(status_Relay)?(repeatTX_buff[11] = 0x01):(repeatTX_buff[11] = 0x00);//开关状态填充
								(process_RelayWorkMode == work_Mode_flip)?(repeatTX_buff[14] = 0x01):(repeatTX_buff[14] = 0x02);//开关工作模式填充 翻转模式：0x01 延时模式：0x02
								
								sprintf(log_info, "接收到查询搜索指令,回复响应完成\n");
								
							}else{
								
								respond_FLAG = false;
								sprintf(log_info, "接收到查询搜索指令,设备已上锁,拒绝回复响应.\n");
							}
							
						}break;
						
						case FRAME_MtoSCMD_cmdCfg_swTim:{	/*普通开关定时配置指令*/
							
							u8	loop = 0;
							
							//校时
							stt_Time datsTime_temp = {0};
							
							datsTime_temp.time_Year		= rxBuff_WIFI_temp[26];
							datsTime_temp.time_Month 	= rxBuff_WIFI_temp[27];
							datsTime_temp.time_Day 		= rxBuff_WIFI_temp[28];
							datsTime_temp.time_Hour 	= rxBuff_WIFI_temp[29];
							datsTime_temp.time_Minute 	= rxBuff_WIFI_temp[30];
							datsTime_temp.time_Week 	= rxBuff_WIFI_temp[31];
							
							timeSet(datsTime_temp);
							
							sprintf(log_info, "接收到局域网普通开关定时配置指令,校时(无秒)已完成.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							//定时数据处理及更新
							switch(rxBuff_WIFI_temp[13]){
							
								case cmdConfigTim_normalSwConfig:{	/*普通定时*/
									
									for(loop = 0; loop < 4; loop ++){
									
										if(rxBuff_WIFI_temp[14 + loop * 3] == 0x80){	/*一次性定时判断*///周占位为空，而定时器被打开，说明是一次性
										
											swTim_onShoot_FLAG 				|= (1 << loop);	//一次性定时标志开启
											rxBuff_WIFI_temp[14 + loop * 3] |= (1 << (rxBuff_WIFI_temp[31] - 1)); //强行进行当前周占位，当次执行完毕后清除
										}
									}
									coverEEPROM_write_n(EEPROM_ADDR_swTimeTab, &rxBuff_WIFI_temp[14], 12);	//定时表
									sprintf(log_info, "普通定时更新已完成.\n");
								}break;
								
								default:break;
							}
							
							//数据响应及回复
							repeatTX_buff[12]	= rxBuff_WIFI_temp[12];
							repeatTX_buff[13]	= rxBuff_WIFI_temp[13];

						}break;
						
						/*指纹权限定时配置指令*/
						case FRAME_MtoSCMD_cmdCfg_permissionTim:{	
						
							u8 loop = 0;
							
							//校时
							stt_Time datsTime_temp = {0};
							
							datsTime_temp.time_Year		= rxBuff_WIFI_temp[26];
							datsTime_temp.time_Month 	= rxBuff_WIFI_temp[27];
							datsTime_temp.time_Day 		= rxBuff_WIFI_temp[28];
							datsTime_temp.time_Hour 	= rxBuff_WIFI_temp[29];
							datsTime_temp.time_Minute 	= rxBuff_WIFI_temp[30];
							datsTime_temp.time_Week 	= rxBuff_WIFI_temp[31];
							
							timeSet(datsTime_temp);
							
							sprintf(log_info, "接收到局域网指纹权限定时配置指令,校时(无秒)已完成.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							//定时数据处理及更新						
							FP_umDetect_enFLAG = 1; //首先使能，防止二次更新失灵
							
							for(loop = 0; loop < 4; loop ++){	/*一次性定时判断*///周占位为空，而定时器被打开，说明是一次性
							
								if(rxBuff_WIFI_temp[14 + loop * 3] == 0x80){	/*一次性定时*///周占位为空，而定时器被打开，说明是一次性
								
									permissionTim_oneShoot_FLAG 	|= (1 << loop);	//一次性定时标志开启
									rxBuff_WIFI_temp[14 + loop * 3] |= (1 << (rxBuff_WIFI_temp[31] - 1)); //强行进行当前周占位，当次执行完毕后清除
								}
							}
							coverEEPROM_write_n(EEPROM_ADDR_permissionTimeTab, &rxBuff_WIFI_temp[14], 12);	//定时时刻表
							coverEEPROM_write_n(EEPROM_ADDR_permissionKeepTab, &rxBuff_WIFI_temp[10], 4);	//定时时长表
							
							/*log调试打印*/
//							sprintf(log_info, "Test_Byte: %02X\n", (int)rxBuff_WIFI_temp[15]);
//							LogDats(log_info, strlen(log_info));
//							memset(log_info, 0, 80 * sizeof(u8));
							
							sprintf(log_info, "指纹权限定时更新已完成.\n");
						
						}break;
						
						/*配置互控指令*/
						case FRAME_MtoSCMD_cmdInterface:{	
						
							
						}break;
						
						/*开关复位指令*/
						case FRAME_MtoSCMD_cmdReset:{		
						
							
						}break;
						
						/*设备锁定指令——设备信息响应失能*/
						case FRAME_MtoSCMD_cmdLockON:{		
						
							sprintf(log_info, "接收到设备上锁指令.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							//数据处理及动作响应
							{
								u8 deviceLock_IF = 1;
								
								deviceLock_flag  = 1;
								coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
							}
							
						}break;
						
						/*设备解锁指令——设备信息响应使能*/
						case FRAME_MtoSCMD_cmdLockOFF:{		
						
							sprintf(log_info, "接收到设备解锁指令.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));	

							//数据处理及动作响应
							{
								u8 deviceLock_IF = 0;
								
								deviceLock_flag  = 0;
								coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
							}
							
						}break;
						
						/*普通开关定时配置查询指令*/
						case FRAME_MtoSCMD_cmdswTimQuery:{	
						
							u8 loop = 0;
							
							sprintf(log_info, "接收到普通开关定时查询指令.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							//数据响应及回复
							EEPROM_read_n(EEPROM_ADDR_swTimeTab, &repeatTX_buff[14], 12);	//定时表回复填装
							
							//回复数据二次处理（针对一次性定时数据）
							for(loop = 0; loop < 4; loop ++){
							
								if(swTim_onShoot_FLAG & (1 << loop)){
									
									repeatTX_buff[14 + loop * 3] &= 0x80;
								}
							}
							
							specialCmd_FLAG = 1;
							
//							sprintf(log_info, "普通开关定时表回复已完成.\n");
							
						}break;
						
						/*指纹权限定时配置查询指令*/
						case FRAME_MtoSCMD_cmdperssionTimQuery:{

							u8 loop = 0;							
						
							sprintf(log_info, "接收到指纹权限定时查询指令.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							//数据响应及回复
							EEPROM_read_n(EEPROM_ADDR_permissionKeepTab, &repeatTX_buff[10], 4);	//定时时刻表回复填装
							EEPROM_read_n(EEPROM_ADDR_permissionTimeTab, &repeatTX_buff[14], 12);	//定时时长表回复填装
							
							//回复数据二次处理（针对一次性定时数据）
							for(loop = 0; loop < 4; loop ++){
							
								if(permissionTim_oneShoot_FLAG & (1 << loop)){
									
									repeatTX_buff[14 + loop * 3] &= 0x80;
								}
							}
							
							specialCmd_FLAG = 1;
							
							sprintf(log_info, "指纹权限定时表回复已完成.\n");
						}break;
						
						/*AP模式配置指令*/
						case FRAME_MtoSCMD_cmdConfigAP:{	
						
							//此指令报废，被硬件拨码开关替代//
						}break;
						
						/*提示音使能指令*/
						case FRAME_MtoSCMD_cmdBeepsON:{		
						
							
						}break;
						
						/*提示音失能指令*/
						case FRAME_MtoSCMD_cmdBeepsOFF:{	
						
							
						}break;
						
						/*恢复出厂本地是否支持查询*/
						case FRAME_MtoSCMD_cmdftRecoverRQ:{	
						
							sprintf(log_info, "接收到恢复出厂本地是否支持查询指令.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							sprintf(log_info, "恢复出厂本地支持并回复.\n");
						}
						
						/*恢复出厂设置指令*/
						case FRAME_MtoSCMD_cmdRecoverFactory:{	
						
							sprintf(log_info, "接收到恢复出厂指令.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));

							//数据处理及动作响应
							Factory_recover();
							
							sprintf(log_info, "恢复出厂已完成.\n");
						}break;
						
						default:break;
					}
					
					if(respond_FLAG){	//主动响应回复执行
					
						dtasTX_loadBasic_AUTO( repeatTX_buff,				/*发送前最后装载*///发送包基本信息填充
											   FRAME_TYPE_StoM_RCVsuccess,
											   cmdParing_Temp,
											   specialCmd_FLAG
											 );
						uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);//数据接收回复响应	
					}
					
					if((HEARTBEAT_objFLAG_SERVER != dataTrans_objWIFI) &&
					   (cmdParing_Temp != FRAME_MtoSCMD_cmdConfigSearch)
					)beeps(1);	//提示音,心跳包不响,搜索配置时不响

					LogDats(log_info, strlen(log_info));
					memset(log_info, 0, 80 * sizeof(u8));
				
			}break;
			
			//心跳包数据解析
			case HEARTBEAT_objFLAG_SERVER:{
			
				switch(rxBuff_WIFI_temp[2]){
				
					case FRAME_HEARTBEAT_cmdOdd:{
					
						
					}break;
						
					case FRAME_HEARTBEAT_cmdEven:{		//奇数心跳时间校准
					
						stt_Time datsTime_temp = {0};
						
//						u8 Y	= rxBuff_WIFI_temp[8];
//						u8 M 	= rxBuff_WIFI_temp[9];
//						u8 D 	= rxBuff_WIFI_temp[10];
//						u8 W 	= Y + (Y / 4) + 5 - 40 + (26 * (M + 1) / 10) + D - 1;	//蔡勒公式
//						   W   %= 7; 
//					
//						W?(datsTime_temp.time_Week = W):(datsTime_temp.time_Week = 7);
						
						datsTime_temp.time_Year		= 18;
						datsTime_temp.time_Week		= rxBuff_WIFI_temp[8];
						datsTime_temp.time_Month 	= rxBuff_WIFI_temp[9];
						datsTime_temp.time_Day 		= rxBuff_WIFI_temp[10];
						datsTime_temp.time_Hour 	= rxBuff_WIFI_temp[11];
						datsTime_temp.time_Minute 	= rxBuff_WIFI_temp[12];
						datsTime_temp.time_Second 	= rxBuff_WIFI_temp[13];
						
						timeSet(datsTime_temp);
						
						sprintf(log_info, "周期性服务器授时校准已完成.\n");
					}break;
					
					default:break;
				}

				LogDats(log_info, strlen(log_info));
				memset(log_info, 0, 80 * sizeof(u8));
			}break;
				
			default:break;
		}
	}
		
	/*************************************心跳包业务代码**********************************************/
	if(heartBeat_Cnt < heartBeat_Period);
	else{
	
	  	heartBeat_Cnt = 0;

		if(heartBeat_Type){		//奇数心跳
		
			heartBeat_Type = !heartBeat_Type;
			
			datsTX_loadHeartBeat(heartBeat_Pack, 0xAA, 14, 0x22, 0, 0, 0, 0, 0);
			EEPROM_read_n(EEPROM_ADDR_timeZone_H, &timeZone_Hour, 1);
			EEPROM_read_n(EEPROM_ADDR_timeZone_M, &timeZone_Minute, 1);
			heartBeat_Pack[9] 	= timeZone_Hour;	//时区_时
			heartBeat_Pack[10]	= timeZone_Minute;	//时区_分
			
		}else{					//偶数心跳
		
			heartBeat_Type = !heartBeat_Type;
			
			datsTX_loadHeartBeat(heartBeat_Pack, 0xAA, 14, 0x23, 0, 0, 0, 0, 0);
			heartBeat_Pack[9]	= deviceLock_flag;	//上锁状态
			heartBeat_Pack[10]	= ifTim_sw_running_FLAG;	//普通开关定时状态
			heartBeat_Pack[11]	= status_Relay;	//指纹权限定时状态
			heartBeat_Pack[12]	= process_RelayWorkMode;	//开关工作方式
			heartBeat_Pack[13]	= ifTim_permission_running_FLAG;	//开关状态
		}
		
		uartObjWIFI_Send_String(heartBeat_Pack, dataHeartBeatLength_objSERVER);
	}
}

