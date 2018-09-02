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

//*********************ָ��ģ����ر���������********************/
extern threadFP_Mode	process_FPworkMode;
extern stt_stepAdd 		umNew_AddStep;
extern u8 				FP_ID;

//*********************MAC��ַ��ر���������*********************/
extern u8 xdata 		MAC_ID[6];

//*********************�̵���������ر���������******************/
extern threadRelay_Mode	process_RelayWorkMode;
extern bit 				status_Relay;

//*********************��ʱ�����߳���ر���������****************/
extern	u8 				swTim_onShoot_FLAG;		//��ͨ���ض�ʱһ���Ա�־��������λ��ʶ�ĸ���ʱ��
extern	u8	 			permissionTim_oneShoot_FLAG;	//ָ��Ȩ�޶�ʱһ���Ա�־��������λ��ʶ�ĸ���ʱ��

extern	bit				ifTim_sw_running_FLAG;	//��ͨ���ض�ʱ���б�־λ
extern	bit				ifTim_permission_running_FLAG;	//ָ��Ȩ�޶�ʱ���б�־λ

extern 	bit				FP_umDetect_enFLAG;	//ָ��ģ�鹤��ģʽΪ��������� ģ��ʱ������ʹ�ܱ�־

/**********************�����ļ�����������************************/
//���ڽ��ճ�ʱ��־
bit uartRX_toutFLG 						= 0;
//���ڽ��ճ�ʱ����
bit rxTout_count_EN 					= 0;
u8  rxTout_count 						= 0;
//�������ݻ���
u8	datsRcv_length						= 0;
uartTout_datsRcv xdata datsRcv_ZIGB 	= {{0}, 0};

//��ǰ������λ��ģʽ��ʶ
dataTrans_obj 	dataTrans_objWIFI 		= DATATRANS_objFLAG_MOBILE;

//��λ���´￪�������־
switch_Status	swStatus_fromUsr  		= swStatus_null;	

//Զ��MACID����
u8				Dst_MACID_Temp[6] 		= {0};

//���ݰ���Ӧ�ظ����ȡ���Ĭ�Ͼ��������ݳ���
u8 xdata 		repeatTX_Len			= dataTransLength_objMOBILE;

//�豸����־
bit				deviceLock_flag			= false;

//��������ż��־
bit 			heartBeat_Type 			= 0;	//��ż����־��������1��ż���0
//���������ڼ���ֵ
u16				heartBeat_Cnt			= 0;
//��������������
const 	u16 	heartBeat_Period 		= 8000; //��������������ֵ ���� * ϵ�� �ݴ�ѭ������	
/*--------------------------------------------------------------*/
void uartObjWIFI_Init(void){

	COMx_InitDefine val_uartInit;
	
	//TX�������
	P3M1 &= 0xFD;	
	P3M0 |= 0x02;	
	
	//RX��������
	P3M1 |= 0x01;
	P3M0 &= 0xFE;

	val_uartInit.UART_Mode			= UART_8bit_BRTx;	//ģʽ,         UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
	val_uartInit.UART_BRT_Use		= BRT_Timer1;		//ʹ�ò�����,   BRT_Timer1,BRT_Timer2
	val_uartInit.UART_BaudRate		= BAUD_WIFI;		//������,       ENABLE,DISABLE
	val_uartInit.Morecommunicate	= ENABLE;			//���ͨѶ����, ENABLE,DISABLE
	val_uartInit.UART_RxEnable		= ENABLE;			//�������,   	ENABLE,DISABLE
	val_uartInit.BaudRateDouble		= ENABLE;			//�����ʼӱ�, 	ENABLE,DISABLE
	val_uartInit.UART_Interrupt		= ENABLE;			//�жϿ���,   	ENABLE,DISABLE
	val_uartInit.UART_Polity		= PolityLow;		//���ȼ�,     	PolityLow,PolityHigh
	val_uartInit.UART_P_SW			= UART1_SW_P30_P31;	//�л��˿�,   	UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17(����ʹ���ڲ�ʱ��)
	val_uartInit.UART_RXD_TXD_Short	= DISABLE;			//�ڲ���·RXD��TXD, ���м�, ENABLE,DISABLE
	
	USART_Configuration(USART1, &val_uartInit);
	
	memset(TX1_Buffer, 0, sizeof(char) * COM_TX1_Lenth);
	
	PrintString1("i'm UART1 for wifi data translate !!!");
}

void uartObjFP_Init(void){

	COMx_InitDefine val_uartInit;
	
	//TX�������
	P1M1 &= 0xFD;	
	P1M0 |= 0x02;	
	
	//RX��������
	P1M1 |= 0x01;
	P1M0 &= 0xFE;

	val_uartInit.UART_Mode			= UART_8bit_BRTx;	//ģʽ,         UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
	val_uartInit.UART_BRT_Use		= BRT_Timer2;		//ʹ�ò�����,   BRT_Timer1,BRT_Timer2
	val_uartInit.UART_BaudRate		= BAUD_FP;			//������,       ENABLE,DISABLE
	val_uartInit.Morecommunicate	= DISABLE;			//���ͨѶ����, ENABLE,DISABLE
	val_uartInit.UART_RxEnable		= ENABLE;			//�������,   	ENABLE,DISABLE
	val_uartInit.BaudRateDouble		= ENABLE;			//�����ʼӱ�, 	ENABLE,DISABLE
	val_uartInit.UART_Interrupt		= ENABLE;			//�жϿ���,   	ENABLE,DISABLE
	val_uartInit.UART_Polity		= PolityLow;		//���ȼ�,     	PolityLow,PolityHigh
	val_uartInit.UART_P_SW			= UART2_SW_P10_P11;	//�л��˿�,   	UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17(����ʹ���ڲ�ʱ��)
	val_uartInit.UART_RXD_TXD_Short	= DISABLE;			//�ڲ���·RXD��TXD, ���м�, ENABLE,DISABLE
	
	while(3 != USART_Configuration(USART2, &val_uartInit));
	
	memset(TX2_Buffer, 0, sizeof(char) * COM_TX2_Lenth);
	
	PrintString2("i'm UART2 for fingerprint data translate !!!");
}

/*----------------------------
���ʹ�������
----------------------------*/
void uartObjWIFI_Send_Byte(u8 dat)	//����1
{
	TX1_write2buff(dat);
}

void uartObjFP_Send_Byte(u8 dat)  	//����2
{
	TX2_write2buff(dat);
}

void uartObjWIFI_Send_String(char *s,unsigned char ucLength){	 //����1
	
	uart1_datsSend(s, ucLength);
}

void uartObjFP_Send_String(char *s,unsigned char ucLength){	 //����1
	
	uart2_datsSend(s, ucLength);
}

void rxBuff_WIFI_Clr(void){

	memset(rxBuff_WIFI, 0xff, sizeof(char) * COM_RX1_Lenth);
	COM1.RX_Cnt = 0;
}

/********************* UART1(WIIF)�жϺ���_�Զ����ع�************************/
void UART1_Rountine (void) interrupt UART1_VECTOR
{
	if(RI)
	{
		RI = 0;
		if(COM1.B_RX_OK == 0)
		{
			
//			switch(WIFI_Uart_RcvMode){		//�������ܱ���
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

/********************* �Զ���У��*****************************/
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

/*�����ݷ�װ���������ݰ�����ǰ�����ã��Զ������ϴ����ݴ���ʱ�Ķ���������ݷ�װ*///����У�鱻��ǰ������
void dtasTX_loadBasic_AUTO(u8 dats_Tx[45],		//���Ͱ����ݻ��������Ϣ��װ
						   u8 frame_Type,
						   u8 frame_CMD,
						   bit ifSpecial_CMD){	
						  
	switch(repeatTX_Len){
	
		/*���������ݰ���װ*///45�ֽ�
		case dataTransLength_objSERVER:{
			
			u8 datsTemp[32] = {0};
		
			dats_Tx[0] 	= FRAME_HEAD_SERVER;
			
			memcpy(&datsTemp[0], &dats_Tx[1], 32);
			memcpy(&dats_Tx[13], &datsTemp[0], 32);	//֡ͷ�������ճ�12���ֽ�
			memcpy(&dats_Tx[1],  &Dst_MACID_Temp[0], 6);	//Զ��MACID���
			memcpy(&dats_Tx[8],  &MAC_ID[1], 5);	/*dats_Tx[7] ��ʱ��0*///Զ��MACID���
			
			dats_Tx[1 + 12] = dataTransLength_objSERVER;
			dats_Tx[2 + 12] = frame_Type;
			dats_Tx[3 + 12] = frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10 + 12] = SWITCH_TYPE_FP;	//�����������
			
			memcpy(&dats_Tx[5 + 12], &MAC_ID[1], 5);	//MAC���
								  
			dats_Tx[4 + 12] = frame_Check(&dats_Tx[5 + 12], 28);	
			
		}break;
		
		/*���������ݰ���װ*///33�ֽ�
		case dataTransLength_objMOBILE:{
		
			dats_Tx[0] 	= FRAME_HEAD_MOBILE;
			
			dats_Tx[1] 	= dataTransLength_objMOBILE;
			dats_Tx[2] 	= frame_Type;
			dats_Tx[3] 	= frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10] = SWITCH_TYPE_FP;	//�����������
			
			memcpy(&dats_Tx[5], &MAC_ID[1], 5);	//MAC���
								  
			dats_Tx[4] 	= frame_Check(&dats_Tx[5], 28);
			
		}break;
		
		default:break;
	}	
}
						   
/*�����ݷ�װ���������ݰ�����ǰ�����ã��Զ������������ݷ�װ*///����У�鱻��ǰ������
void dtasTX_loadBasic_CUST(dataTrans_obj obj_custom,	//���Ͱ����ݻ��������Ϣ��װ
						   u8 dats_Tx[45],		
						   u8 frame_Type,
						   u8 frame_CMD,
						   bit ifSpecial_CMD){	
						  
	switch(obj_custom){
	
		/*���������ݰ���װ*///45�ֽ�
		case DATATRANS_objFLAG_SERVER:{
			
			u8 datsTemp[32] = {0};
		
			dats_Tx[0] 	= FRAME_HEAD_SERVER;
			
			memcpy(&datsTemp[0], &dats_Tx[1], 32);
			memcpy(&dats_Tx[13], &datsTemp[0], 32);	//֡ͷ�������ճ�12���ֽ�
			memcpy(&dats_Tx[1],  &Dst_MACID_Temp[0], 6);	//Զ��MACID���
			memcpy(&dats_Tx[8],  &MAC_ID[1], 5);	/*dats_Tx[7] ��ʱ��0*///Զ��MACID���
			
			dats_Tx[1 + 12] = dataTransLength_objSERVER;
			dats_Tx[2 + 12] = frame_Type;
			dats_Tx[3 + 12] = frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10 + 12] = SWITCH_TYPE_FP;	//�����������
			
			memcpy(&dats_Tx[5 + 12], &MAC_ID[1], 5);	//MAC���
								  
			dats_Tx[4 + 12] = frame_Check(&dats_Tx[5 + 12], 28);	
			
		}break;
		
		/*���������ݰ���װ*///33�ֽ�	--����45����33��װ
		case DATATRANS_objFLAG_MOBILE:
		
		default:{
		
			dats_Tx[0] 	= FRAME_HEAD_MOBILE;
			
			dats_Tx[1] 	= dataTransLength_objMOBILE;
			dats_Tx[2] 	= frame_Type;
			dats_Tx[3] 	= frame_CMD;
			
			if(!ifSpecial_CMD)dats_Tx[10] = SWITCH_TYPE_FP;	//�����������
			
			memcpy(&dats_Tx[5], &MAC_ID[1], 5);	//MAC���
								  
			dats_Tx[4] 	= frame_Check(&dats_Tx[5], 28);
		}break;
	}	
}
					  
void datsTX_loadHeartBeat(u8 dats_Tx[dataHeartBeatLength_objSERVER],		//�������������ݻ��������Ϣ��װ
					  u8 frame_Head,
					  u8 frame_Len,
					  u8 frame_CMD,
					  u8 ifLock,
					  u8 infoTcnt,
					  u8 infoStatus,
					  u8 ifGreen,
					  u8 ifMnight){
	
	memset(dats_Tx, 0, dataHeartBeatLength_objSERVER * sizeof(u8));

	dats_Tx[0] = frame_Head;			//֡ͷ���
	dats_Tx[1] = frame_Len;
	dats_Tx[2] = frame_CMD;
						  
//	MAC_ID_Relaes();
	memcpy(&dats_Tx[4], &MAC_ID[1], 5);	//MAC���
						  
	dats_Tx[9] 	= ifLock;
	dats_Tx[10] = infoTcnt;
	dats_Tx[11] = infoStatus;
	dats_Tx[12] = ifGreen;
	dats_Tx[13] = ifMnight;
}

/***********�ֶ��������������**************************/
void heartBeat_Release_Immediately(void){
	
	heartBeat_Cnt 	= heartBeat_Period;	//��������ż����������״̬��
	heartBeat_Type	= 0;	//ż����
}

/***********�������ݼ�����������������**************/
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
		
		heartBeat_Cnt 	= heartBeat_Period;	//��������ż����������״̬��
		heartBeat_Type	= 0;	//ż����
			
		Local_ifTim_sw_running_FLAG 		= ifTim_sw_running_FLAG;
		Local_ifTim_permission_running_FLAG = ifTim_permission_running_FLAG;
		Local_process_RelayWorkMode			= process_RelayWorkMode;
		Local_status_Relay					= status_Relay;
		Local_deviceLock_flag				= deviceLock_flag;
	}
}

/********************* ���ݴ��估����************************/
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
	
	/*********************************���ݰ��ɼ�**********************************/
	if(uartRX_toutFLG){ //��ʱ��֡
	
		uartRX_toutFLG = 0;
		
		memset(rxBuff_WIFI_temp, 0, 45 * sizeof(u8));
		
		if((datsRcv_ZIGB.rcvDats[0] == FRAME_HEAD_SERVER) && 
		   (datsRcv_ZIGB.rcvDats[13] == dataTransLength_objSERVER) &&
		   (datsRcv_ZIGB.rcvDats[14] == FRAME_TYPE_MtoS_CMD) &&
		   (datsRcv_ZIGB.rcvDatsLen >= dataTransLength_objSERVER)){ 
			
			memcpy(&Dst_MACID_Temp[0], &datsRcv_ZIGB.rcvDats[7], 6);  //Զ��MACID�ݴ�	
			rxBuff_WIFI_temp[0] = datsRcv_ZIGB.rcvDats[0]; //֡ͷ��ֵ
			memcpy(&rxBuff_WIFI_temp[1], &datsRcv_ZIGB.rcvDats[13], 32);	//���ݼ��أ�֡���룬�����33�ֽ�
			
			//��Ӧ�ظ��������Ը���	
			dataTrans_objWIFI = DATATRANS_objFLAG_SERVER;
			repeatTX_Len	  = dataTransLength_objSERVER;
			
			frameLength = dataTransLength_objSERVER;
			datsQualified_FLG = 1;
				
		}else
		if((datsRcv_ZIGB.rcvDats[0] == FRAME_HEAD_MOBILE) && 
		   (datsRcv_ZIGB.rcvDats[1] == dataTransLength_objMOBILE) &&
		   (datsRcv_ZIGB.rcvDats[2] == FRAME_TYPE_MtoS_CMD) &&
		   (datsRcv_ZIGB.rcvDatsLen == dataTransLength_objMOBILE)){
		
			memcpy(rxBuff_WIFI_temp, datsRcv_ZIGB.rcvDats, dataTransLength_objMOBILE);	//���ݼ���
			
			//��Ӧ�ظ��������Ը���	
			dataTrans_objWIFI = DATATRANS_objFLAG_MOBILE;
			repeatTX_Len	  = dataTransLength_objMOBILE;
				
			frameLength = dataTransLength_objMOBILE;
			datsQualified_FLG = 1;
				
		}else
		if((datsRcv_ZIGB.rcvDats[0] == FRAME_HEAD_HEARTBEAT) && 
		   (datsRcv_ZIGB.rcvDats[1] == dataHeartBeatLength_objSERVER) &&
		   (datsRcv_ZIGB.rcvDatsLen == dataHeartBeatLength_objSERVER)){
			
			memcpy(rxBuff_WIFI_temp, datsRcv_ZIGB.rcvDats, dataHeartBeatLength_objSERVER);	//���ݼ���
			
			//��Ӧ�ظ��������Ը���	--������repeatTX_Len���£�����
			dataTrans_objWIFI = HEARTBEAT_objFLAG_SERVER;
			   
			frameLength = dataHeartBeatLength_objSERVER;
			datsQualified_FLG = 1;
			   
		}else{
		
//			dataTrans_objWIFI = dataTrans_obj_NULL;
			datsQualified_FLG = 0;
		}
	}
	
	/*********************************���ݰ��������**********************************/
	if(datsQualified_FLG){
		
		bit frameCodeCheck_PASS = 0; //У������ͨ����־
		bit frameMacCheck_PASS  = 0; //mac��ַ�����ͨ����־
		
		datsQualified_FLG = 0;
	
		switch(frameLength){
		
			case dataTransLength_objSERVER:
			case dataTransLength_objMOBILE:{
			
				if(rxBuff_WIFI_temp[4] == frame_Check(&rxBuff_WIFI_temp[5], 28))frameCodeCheck_PASS = 1; //У������
				if(!memcmp(&rxBuff_WIFI_temp[5], &MAC_ID[1], 5))frameMacCheck_PASS  = 1; //mac��ַ���
				
				if(rxBuff_WIFI_temp[3] == FRAME_MtoSCMD_cmdCfg_swTim){ //���ý���У�������ָ��
				
					frameCodeCheck_PASS = 1;	
					
				}
				if(rxBuff_WIFI_temp[3] == FRAME_MtoSCMD_cmdConfigSearch){ //���ý���mac��ַ����ָ��
				
					frameMacCheck_PASS = 1;	
					
				}
				
				if(frameMacCheck_PASS & frameMacCheck_PASS){ //ָ����֤
					
					Parsing_EN = 1;	
					
				}else{
				
//					if(!frameMacCheck_PASS)uartObjWIFI_Send_String("fuck mac", 8);
//					if(!frameMacCheck_PASS)uartObjWIFI_Send_String("fuck code", 9);rxBuff_WIFI_temp
//					uartObjWIFI_Send_String(rxBuff_WIFI_temp, 33);
				}
				
			}break;
			
			case dataHeartBeatLength_objSERVER:{
			
				if(!memcmp(&rxBuff_WIFI_temp[3], &MAC_ID[1], 5)){	//MAC��ַ���رȶ�
					
					Parsing_EN = 1;	
				}
				
			}break;
			
			default:{
		
				Parsing_EN = 0;	
				
			}break;
		}
	}
	
	/*********************************���ݰ���ʼ������Ӧ**********************************/	
	if(Parsing_EN){	//��ʼ����
		
		Parsing_EN = 0;
		
		delayMs(10); //��������ʱ
		
		switch(dataTrans_objWIFI){
		
			//������
			case DATATRANS_objFLAG_SERVER:
			//������
			case DATATRANS_objFLAG_MOBILE:{
				
					bit specialCmd_FLAG = 0;	/*�������ݷ�װ��־*///����1�Ƿ�ռ�ã�ԭΪ�������ͣ�
					bit respond_FLAG	= 1;	/*�Ƿ���Ӧ�ظ���־*///���ݽ��պ��Ƿ������ظ���Ӧ
				
					u8 cmdParing_Temp = rxBuff_WIFI_temp[3];

					switch(cmdParing_Temp){		//���������ʶ��

						/*����ָ��*/
						case FRAME_MtoSCMD_cmdControl:{		
							
							switch(rxBuff_WIFI_temp[11]){
							
								case 0x00:{		//����_��
								
									swStatus_fromUsr = swStatus_off;
								}break;
									
								case 0x01:{		//����_��
								
									swStatus_fromUsr = swStatus_on;
								}break;
								
								default:break;
							}
							
							//������Ӧ���ظ�
							repeatTX_buff[15]	= FPID_NULL;	//����ң�أ�ָ�ƺſ����
							repeatTX_buff[11]	= rxBuff_WIFI_temp[11];		//����״̬�������
							(process_RelayWorkMode == work_Mode_flip)?(repeatTX_buff[14] = 0x01):(repeatTX_buff[14] = 0x02);//���ع���ģʽ��� ��תģʽ��0x01 ��ʱģʽ��0x02
							
							sprintf(log_info, "���յ�����ָ��\n");
							
						}break;
					
						/*ָ��ѧϰָ��*/
						case FRAME_MtoSCMD_cmdFPLearn:{		
							
//							LogDats(rxBuff_WIFI_temp, 33);
						
							sprintf(log_info, "���յ�ָ��ѧϰָ�ѧϰģ��ID��Ϊ��%u\n", (int)rxBuff_WIFI_temp[15] + 1);
							if(FPID_NULL != rxBuff_WIFI_temp[15]){
							
								FP_ID = rxBuff_WIFI_temp[15];
								fpModeTrig_umAdd();	//ָ�������̸߳�Ԥ_����ѧϰģʽ
							}
							
							//������Ӧ���ѽ���ѧϰ¼��״̬
							repeatTX_buff[15]	= FP_ID;
							
						}break;
						
						/*ָ��ɾ��ָ��*/
						case FRAME_MtoSCMD_cmdFPDelete:{	
						
							sprintf(log_info, "���յ�ָ��ģ��ɾ��ָ�ɾ��ģ��ID��Ϊ��%u\n", (int)rxBuff_WIFI_temp[15] + 1);
							if(FPID_NULL != rxBuff_WIFI_temp[15]){
							
								FP_ID = rxBuff_WIFI_temp[15];
								fpModeTrig_umDel();	//ָ�������̸߳�Ԥ_����ɾ��ģʽ
							}
							
							//������Ӧ���ظ���ָ�����������
							respond_FLAG = 0;
							
						}break;
						
						/*ָ�Ʋ�ѯָ��*/
						case FRAME_MtoSCMD_cmdFPQuery:{		
						
							u8 data fpID_Tab[13] = {0};
								
							sprintf(log_info, "���յ�ָ��ģ���ѯָ�������ָ��ռλ����.\n");
							
							//������Ӧ���ظ�
							EEPROM_read_n(EEPROM_ADDR_FP_STORE, &fpID_Tab[0], 13);	
							memcpy(&repeatTX_buff[16], &fpID_Tab[0], 13);
							
						}break;
						
						/*��������ָ��*/
						case FRAME_MtoSCMD_cmdConfigSearch:{	
							
							u8 deviceLock_IF = 0;
											
							EEPROM_read_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
							
							sprintf(log_info, "���յ�smartlink ����/���� ָ��.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							heartBeat_Release_Immediately(); //�������漴����
							
							//�������
							if(!deviceLock_IF){	
								
								u8 xdata serverIP_temp[4] = {0};
								memcpy(&serverIP_temp[0], &rxBuff_WIFI_temp[6], 4);
							
								//��������ַͬ��
								if(WIFI_serverUDP_CHG(serverIP_temp)){
								
									coverEEPROM_write_n(EEPROM_ADDR_serverIP_record, &serverIP_temp[0], 4); //����eeprom
									
									sprintf(log_info,
											"������IPͬ���ɹ�,��ǰ������IPΪ:%d.%d.%d.%d\n",
											(int)serverIP_temp[0],
											(int)serverIP_temp[1],
											(int)serverIP_temp[2],
											(int)serverIP_temp[3]);
								}else{
								
									sprintf(log_info, "������IPͬ��ʧ��,����ʹ�ñ��ش洢�ķ�����IP.\n");
								}
								LogDats(log_info, strlen(log_info));
								memset(log_info, 0, 80 * sizeof(u8));
								
								//ʱ����Ϣͬ��
								timeZone_Hour 	= rxBuff_WIFI_temp[12];		//ʱ������
								timeZone_Minute	= rxBuff_WIFI_temp[13];
								coverEEPROM_write_n(EEPROM_ADDR_timeZone_H, &timeZone_Hour, 1);
								coverEEPROM_write_n(EEPROM_ADDR_timeZone_M, &timeZone_Minute, 1);
								sprintf(log_info, "ʱ������ͬ���������,��ǰʱ��Ϊ:ʱ-%d,��-%d.\n",(int)timeZone_Hour,
																								   (int)timeZone_Minute);
								LogDats(log_info, strlen(log_info));
								memset(log_info, 0, 80 * sizeof(u8));
								
								sprintf(log_info, "������Ϣ����ͬ���������.\n");
								
								//������Ӧ���ظ�
								(process_RelayWorkMode == work_Mode_flip)?(repeatTX_buff[14] = 0x01):(repeatTX_buff[14] = 0x02);//���ع���ģʽ��� ��תģʽ��0x01 ��ʱģʽ��0x02
								
							}else{
								
								respond_FLAG = false;
								sprintf(log_info, "�豸������,�ܾ��ظ���Ӧ\n");
							}
							
						}break;
						
						/*��ѯ��¼ָ��*/
						case FRAME_MtoSCMD_cmdQuery:{	
							
							u8 deviceLock_IF = 0;
							
							EEPROM_read_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);

							if(deviceLock_IF){
							
								//������Ӧ���ظ�
								(status_Relay)?(repeatTX_buff[11] = 0x01):(repeatTX_buff[11] = 0x00);//����״̬���
								(process_RelayWorkMode == work_Mode_flip)?(repeatTX_buff[14] = 0x01):(repeatTX_buff[14] = 0x02);//���ع���ģʽ��� ��תģʽ��0x01 ��ʱģʽ��0x02
								
								sprintf(log_info, "���յ���ѯ����ָ��,�ظ���Ӧ���\n");
								
							}else{
								
								respond_FLAG = false;
								sprintf(log_info, "���յ���ѯ����ָ��,�豸������,�ܾ��ظ���Ӧ.\n");
							}
							
						}break;
						
						case FRAME_MtoSCMD_cmdCfg_swTim:{	/*��ͨ���ض�ʱ����ָ��*/
							
							u8	loop = 0;
							
							//Уʱ
							stt_Time datsTime_temp = {0};
							
							datsTime_temp.time_Year		= rxBuff_WIFI_temp[26];
							datsTime_temp.time_Month 	= rxBuff_WIFI_temp[27];
							datsTime_temp.time_Day 		= rxBuff_WIFI_temp[28];
							datsTime_temp.time_Hour 	= rxBuff_WIFI_temp[29];
							datsTime_temp.time_Minute 	= rxBuff_WIFI_temp[30];
							datsTime_temp.time_Week 	= rxBuff_WIFI_temp[31];
							
							timeSet(datsTime_temp);
							
							sprintf(log_info, "���յ���������ͨ���ض�ʱ����ָ��,Уʱ(����)�����.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							//��ʱ���ݴ�������
							switch(rxBuff_WIFI_temp[13]){
							
								case cmdConfigTim_normalSwConfig:{	/*��ͨ��ʱ*/
									
									for(loop = 0; loop < 4; loop ++){
									
										if(rxBuff_WIFI_temp[14 + loop * 3] == 0x80){	/*һ���Զ�ʱ�ж�*///��ռλΪ�գ�����ʱ�����򿪣�˵����һ����
										
											swTim_onShoot_FLAG 				|= (1 << loop);	//һ���Զ�ʱ��־����
											rxBuff_WIFI_temp[14 + loop * 3] |= (1 << (rxBuff_WIFI_temp[31] - 1)); //ǿ�н��е�ǰ��ռλ������ִ����Ϻ����
										}
									}
									coverEEPROM_write_n(EEPROM_ADDR_swTimeTab, &rxBuff_WIFI_temp[14], 12);	//��ʱ��
									sprintf(log_info, "��ͨ��ʱ���������.\n");
								}break;
								
								default:break;
							}
							
							//������Ӧ���ظ�
							repeatTX_buff[12]	= rxBuff_WIFI_temp[12];
							repeatTX_buff[13]	= rxBuff_WIFI_temp[13];

						}break;
						
						/*ָ��Ȩ�޶�ʱ����ָ��*/
						case FRAME_MtoSCMD_cmdCfg_permissionTim:{	
						
							u8 loop = 0;
							
							//Уʱ
							stt_Time datsTime_temp = {0};
							
							datsTime_temp.time_Year		= rxBuff_WIFI_temp[26];
							datsTime_temp.time_Month 	= rxBuff_WIFI_temp[27];
							datsTime_temp.time_Day 		= rxBuff_WIFI_temp[28];
							datsTime_temp.time_Hour 	= rxBuff_WIFI_temp[29];
							datsTime_temp.time_Minute 	= rxBuff_WIFI_temp[30];
							datsTime_temp.time_Week 	= rxBuff_WIFI_temp[31];
							
							timeSet(datsTime_temp);
							
							sprintf(log_info, "���յ�������ָ��Ȩ�޶�ʱ����ָ��,Уʱ(����)�����.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							//��ʱ���ݴ�������						
							FP_umDetect_enFLAG = 1; //����ʹ�ܣ���ֹ���θ���ʧ��
							
							for(loop = 0; loop < 4; loop ++){	/*һ���Զ�ʱ�ж�*///��ռλΪ�գ�����ʱ�����򿪣�˵����һ����
							
								if(rxBuff_WIFI_temp[14 + loop * 3] == 0x80){	/*һ���Զ�ʱ*///��ռλΪ�գ�����ʱ�����򿪣�˵����һ����
								
									permissionTim_oneShoot_FLAG 	|= (1 << loop);	//һ���Զ�ʱ��־����
									rxBuff_WIFI_temp[14 + loop * 3] |= (1 << (rxBuff_WIFI_temp[31] - 1)); //ǿ�н��е�ǰ��ռλ������ִ����Ϻ����
								}
							}
							coverEEPROM_write_n(EEPROM_ADDR_permissionTimeTab, &rxBuff_WIFI_temp[14], 12);	//��ʱʱ�̱�
							coverEEPROM_write_n(EEPROM_ADDR_permissionKeepTab, &rxBuff_WIFI_temp[10], 4);	//��ʱʱ����
							
							/*log���Դ�ӡ*/
//							sprintf(log_info, "Test_Byte: %02X\n", (int)rxBuff_WIFI_temp[15]);
//							LogDats(log_info, strlen(log_info));
//							memset(log_info, 0, 80 * sizeof(u8));
							
							sprintf(log_info, "ָ��Ȩ�޶�ʱ���������.\n");
						
						}break;
						
						/*���û���ָ��*/
						case FRAME_MtoSCMD_cmdInterface:{	
						
							
						}break;
						
						/*���ظ�λָ��*/
						case FRAME_MtoSCMD_cmdReset:{		
						
							
						}break;
						
						/*�豸����ָ����豸��Ϣ��Ӧʧ��*/
						case FRAME_MtoSCMD_cmdLockON:{		
						
							sprintf(log_info, "���յ��豸����ָ��.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							//���ݴ���������Ӧ
							{
								u8 deviceLock_IF = 1;
								
								deviceLock_flag  = 1;
								coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
							}
							
						}break;
						
						/*�豸����ָ����豸��Ϣ��Ӧʹ��*/
						case FRAME_MtoSCMD_cmdLockOFF:{		
						
							sprintf(log_info, "���յ��豸����ָ��.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));	

							//���ݴ���������Ӧ
							{
								u8 deviceLock_IF = 0;
								
								deviceLock_flag  = 0;
								coverEEPROM_write_n(EEPROM_ADDR_deviceLockFLAG, &deviceLock_IF, 1);
							}
							
						}break;
						
						/*��ͨ���ض�ʱ���ò�ѯָ��*/
						case FRAME_MtoSCMD_cmdswTimQuery:{	
						
							u8 loop = 0;
							
							sprintf(log_info, "���յ���ͨ���ض�ʱ��ѯָ��.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							//������Ӧ���ظ�
							EEPROM_read_n(EEPROM_ADDR_swTimeTab, &repeatTX_buff[14], 12);	//��ʱ��ظ���װ
							
							//�ظ����ݶ��δ������һ���Զ�ʱ���ݣ�
							for(loop = 0; loop < 4; loop ++){
							
								if(swTim_onShoot_FLAG & (1 << loop)){
									
									repeatTX_buff[14 + loop * 3] &= 0x80;
								}
							}
							
							specialCmd_FLAG = 1;
							
//							sprintf(log_info, "��ͨ���ض�ʱ��ظ������.\n");
							
						}break;
						
						/*ָ��Ȩ�޶�ʱ���ò�ѯָ��*/
						case FRAME_MtoSCMD_cmdperssionTimQuery:{

							u8 loop = 0;							
						
							sprintf(log_info, "���յ�ָ��Ȩ�޶�ʱ��ѯָ��.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							//������Ӧ���ظ�
							EEPROM_read_n(EEPROM_ADDR_permissionKeepTab, &repeatTX_buff[10], 4);	//��ʱʱ�̱�ظ���װ
							EEPROM_read_n(EEPROM_ADDR_permissionTimeTab, &repeatTX_buff[14], 12);	//��ʱʱ����ظ���װ
							
							//�ظ����ݶ��δ������һ���Զ�ʱ���ݣ�
							for(loop = 0; loop < 4; loop ++){
							
								if(permissionTim_oneShoot_FLAG & (1 << loop)){
									
									repeatTX_buff[14 + loop * 3] &= 0x80;
								}
							}
							
							specialCmd_FLAG = 1;
							
							sprintf(log_info, "ָ��Ȩ�޶�ʱ��ظ������.\n");
						}break;
						
						/*APģʽ����ָ��*/
						case FRAME_MtoSCMD_cmdConfigAP:{	
						
							//��ָ��ϣ���Ӳ�����뿪�����//
						}break;
						
						/*��ʾ��ʹ��ָ��*/
						case FRAME_MtoSCMD_cmdBeepsON:{		
						
							
						}break;
						
						/*��ʾ��ʧ��ָ��*/
						case FRAME_MtoSCMD_cmdBeepsOFF:{	
						
							
						}break;
						
						/*�ָ����������Ƿ�֧�ֲ�ѯ*/
						case FRAME_MtoSCMD_cmdftRecoverRQ:{	
						
							sprintf(log_info, "���յ��ָ����������Ƿ�֧�ֲ�ѯָ��.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));
							
							sprintf(log_info, "�ָ���������֧�ֲ��ظ�.\n");
						}
						
						/*�ָ���������ָ��*/
						case FRAME_MtoSCMD_cmdRecoverFactory:{	
						
							sprintf(log_info, "���յ��ָ�����ָ��.\n");
							LogDats(log_info, strlen(log_info));
							memset(log_info, 0, 80 * sizeof(u8));

							//���ݴ���������Ӧ
							Factory_recover();
							
							sprintf(log_info, "�ָ����������.\n");
						}break;
						
						default:break;
					}
					
					if(respond_FLAG){	//������Ӧ�ظ�ִ��
					
						dtasTX_loadBasic_AUTO( repeatTX_buff,				/*����ǰ���װ��*///���Ͱ�������Ϣ���
											   FRAME_TYPE_StoM_RCVsuccess,
											   cmdParing_Temp,
											   specialCmd_FLAG
											 );
						uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);//���ݽ��ջظ���Ӧ	
					}
					
					if((HEARTBEAT_objFLAG_SERVER != dataTrans_objWIFI) &&
					   (cmdParing_Temp != FRAME_MtoSCMD_cmdConfigSearch)
					)beeps(1);	//��ʾ��,����������,��������ʱ����

					LogDats(log_info, strlen(log_info));
					memset(log_info, 0, 80 * sizeof(u8));
				
			}break;
			
			//���������ݽ���
			case HEARTBEAT_objFLAG_SERVER:{
			
				switch(rxBuff_WIFI_temp[2]){
				
					case FRAME_HEARTBEAT_cmdOdd:{
					
						
					}break;
						
					case FRAME_HEARTBEAT_cmdEven:{		//��������ʱ��У׼
					
						stt_Time datsTime_temp = {0};
						
//						u8 Y	= rxBuff_WIFI_temp[8];
//						u8 M 	= rxBuff_WIFI_temp[9];
//						u8 D 	= rxBuff_WIFI_temp[10];
//						u8 W 	= Y + (Y / 4) + 5 - 40 + (26 * (M + 1) / 10) + D - 1;	//���չ�ʽ
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
						
						sprintf(log_info, "�����Է�������ʱУ׼�����.\n");
					}break;
					
					default:break;
				}

				LogDats(log_info, strlen(log_info));
				memset(log_info, 0, 80 * sizeof(u8));
			}break;
				
			default:break;
		}
	}
		
	/*************************************������ҵ�����**********************************************/
	if(heartBeat_Cnt < heartBeat_Period);
	else{
	
	  	heartBeat_Cnt = 0;

		if(heartBeat_Type){		//��������
		
			heartBeat_Type = !heartBeat_Type;
			
			datsTX_loadHeartBeat(heartBeat_Pack, 0xAA, 14, 0x22, 0, 0, 0, 0, 0);
			EEPROM_read_n(EEPROM_ADDR_timeZone_H, &timeZone_Hour, 1);
			EEPROM_read_n(EEPROM_ADDR_timeZone_M, &timeZone_Minute, 1);
			heartBeat_Pack[9] 	= timeZone_Hour;	//ʱ��_ʱ
			heartBeat_Pack[10]	= timeZone_Minute;	//ʱ��_��
			
		}else{					//ż������
		
			heartBeat_Type = !heartBeat_Type;
			
			datsTX_loadHeartBeat(heartBeat_Pack, 0xAA, 14, 0x23, 0, 0, 0, 0, 0);
			heartBeat_Pack[9]	= deviceLock_flag;	//����״̬
			heartBeat_Pack[10]	= ifTim_sw_running_FLAG;	//��ͨ���ض�ʱ״̬
			heartBeat_Pack[11]	= status_Relay;	//ָ��Ȩ�޶�ʱ״̬
			heartBeat_Pack[12]	= process_RelayWorkMode;	//���ع�����ʽ
			heartBeat_Pack[13]	= ifTim_permission_running_FLAG;	//����״̬
		}
		
		uartObjWIFI_Send_String(heartBeat_Pack, dataHeartBeatLength_objSERVER);
	}
}

