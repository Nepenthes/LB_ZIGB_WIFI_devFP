#include "sensor_FP.h"
#include "dataTrans.h"
#include "Relay.h"

#include "string.h"
#include "stdio.h"

#include "eeprom.h"
#include "soft_uart.h"
#include "delay.h"
#include "USART.h"

#include "datasReference.h"
#include "dataTrans.h"
#include "Tips.h"
#include "pars_Method.h"

//*********************���ݴ������������**********************/
extern 	u8 xdata 			repeatTX_Len;

//*********************�̵�����������������**********************/
extern  threadRelay_Mode	process_RelayWorkMode;
extern	bit 				status_Relay;

//*********************������������������********************/

/**********************�����ļ�����������************************/
threadFP_Mode		process_FPworkMode			 = work_Mode_NULL;		//ָ��ģ�鹤��ģʽ
stt_stepAdd 		umNew_AddStep				 = umStepAdd_A;			//ָ��ģ�鹤��ģʽΪ����/��    ģ��ʱ����Ӧ�Ľ���ָʾ
stt_stepAdd 		umNew_AddStep_StandBy 		 = umStepAdd_A;			//ָ��ģ�鹤��ģʽΪ����/��    ģ��ʱ����Ӧ��Ԥ������ָʾ
stt_stepDtt			um_dttStep 					 = umStepDtt_A;			//ָ��ģ�鹤��ģʽΪ��������� ģ��ʱ����Ӧ�Ľ���ָʾ
bit					FP_umDetect_enFLAG			 = 1;					//ָ��ģ�鹤��ģʽΪ��������� ģ��ʱ������ʹ�ܱ�־������Ȩ�޶�ʱ����ʼ��ʱ���ż��
bit					FPvalidation_flg			 = 0;					//ָ��ģ�鹤��ģʽΪ��ģ��ʶ��ʱ��ʶ����ɱ�־��ҵ��������ʱ�����ֶ�����
u8 					FP_ID						 = FPID_NULL;			//ģ��ID��

bit 				umNew_AddStepReapt_cmpHalfEN = 0;					//���յ�ָ��ģ��ѧϰָ��ʱ�������״̬ʱ��Ϣ�ط�ʹ��

u16	xdata			Mode_umAdd_timeOutcnt		 = 0;					//����ѧϰģʽʱ��ʱ���

tipsFP_completeType FP_modeCompleteTips 		 = cmp_NULL;			//ģʽ������ɺ�Tips���
volatile u16 xdata	FPmodeCmpTips_cnt			 = 0;					//ģʽ������ɺ�Tipsר�ü�ʱ������timer�е���
u16 code 			FPmodeCmpTips_Period		 = 1500;				//ģʽ������ɺ�Tipsר�ü�ʱ�������

bit					FP_Rcv_timeOut_FLG			 = 0;					//ָ��ģ�����ݽ��ճ�ʱ��־		

volatile unsigned char idata FINGERPRINT_RECEVICE_BUFFER[24] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

/*===============================================================*/

//FINGERPRINTͨ��Э�鶨��
unsigned char code FP_Pack_Head[6] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF};  //Э���ͷ
unsigned char code FP_Get_Img[6] = {0x01,0x00,0x03,0x01,0x0,0x05};    //���ָ��ͼ�񡪡�����1
unsigned char code FP_Img_To_Buffer1[7]={0x01,0x0,0x04,0x02,0x01,0x0,0x08}; //��ͼ����뵽BUFFER1��������2
unsigned char code FP_Img_To_Buffer2[7]={0x01,0x0,0x04,0x02,0x02,0x0,0x09}; //��ͼ����뵽BUFFER2����������3
unsigned char code FP_Reg_Model[6]={0x01,0x0,0x03,0x05,0x0,0x09}; //��BUFFER1��BUFFER2�ϳ�����ģ�桪������4

unsigned char code FP_Search[11]={0x01,0x0,0x08,0x04,0x01,0x0,0x0,0x03,0xA1,0x0,0xB2}; //����ָ��������Χ0 - 929  ��������5
unsigned char code FP_Search_0_9[11]={0x01,0x0,0x08,0x04,0x01,0x0,0x0,0x0,0x09,0x0,0x17}; //����0-9��ָ�ơ�������6
unsigned char code FP_Search_0_100[11]={0x01,0x0,0x08,0x04,0x01,0x0,0x0,0x0,0x64,0x0,0x72}; //����0-9��ָ�ơ�������6

unsigned char code FP_Delet_All_Model[6]={0x01,0x0,0x03,0x0d,0x00,0x11};//ɾ��ָ��ģ�������е�ģ�桪������7
unsigned char code FP_Templete_Num[6] ={0x01,0x00,0x03,0x1D,0x00,0x21 }; //���ģ����������������8

volatile unsigned char idata FP_Save_Finger[9]={0x01,0x00,0x06,0x06,0x01,0x00,0x0B,0x00,0x19};//��BUFFER1�е��������ŵ�ָ����λ�á�������9
volatile unsigned char idata FP_Delete_Model[10]={0x01,0x00,0x07,0x0C,0x0,0x0,0x0,0x1,0x0,0x0}; //ɾ��ָ����ģ�桪������7

//volatile unsigned char FINGER_NUM;
/*cyacyf����*/
volatile unsigned char idata FP_Load_Model[9]={0x01,0x0,0x06,0x07,0x01,0x00,0x0B,0x00,0x1A}; //��flash���ݿ���ָ��ID�ŵ�ָ��ģ����뵽BUFFER1


/*------------------ FINGERPRINT������ --------------------------*/

//FINGERPRINT_���ָ��ͼ�����������1
void FINGERPRINT_Cmd_Get_Img(void)
{
    unsigned char i;

    for(i=0;i<6;i++) //���Ͱ�ͷ
       uartObjFP_Send_Byte(FP_Pack_Head[i]);
    
    for(i=0;i<6;i++) //�������� 0x1d
       uartObjFP_Send_Byte(FP_Get_Img[i]);
}

//��ͼ��ת��������������Buffer1�С�������2
void FINGERPRINT_Cmd_Img_To_Buffer1(void)
{
 	    unsigned char i;
    
	       for(i=0;i<6;i++)    //���Ͱ�ͷ
	         {
    	       uartObjFP_Send_Byte(FP_Pack_Head[i]);   
   		     }
           
   		   for(i=0;i<7;i++)   //�������� ��ͼ��ת���� ������ ����� CHAR_buffer1
             {
      		   uartObjFP_Send_Byte(FP_Img_To_Buffer1[i]);
   		     }
}

//��ͼ��ת��������������Buffer2�С�������3
void FINGERPRINT_Cmd_Img_To_Buffer2(void)
{
     unsigned char i;
           for(i=0;i<6;i++)    //���Ͱ�ͷ
	         {
    	       uartObjFP_Send_Byte(FP_Pack_Head[i]);   
   		     }
           
   		   for(i=0;i<7;i++)   //�������� ��ͼ��ת���� ������ ����� CHAR_buffer1
             {
      		   uartObjFP_Send_Byte(FP_Img_To_Buffer2[i]);
   		     }
}

//��BUFFER1 �� BUFFER2 �е�������ϲ���ָ��ģ�桪������4
void FINGERPRINT_Cmd_Reg_Model(void)
{
    unsigned char i;    

    for(i=0;i<6;i++) //��ͷ
    {
      uartObjFP_Send_Byte(FP_Pack_Head[i]);   
    }

    for(i=0;i<6;i++) //����ϲ�ָ��ģ��
    {
      uartObjFP_Send_Byte(FP_Reg_Model[i]);   
    }

}

//����ȫ���û�999öָ��	��������5
void FINGERPRINT_Cmd_Search_Finger(void)
{
       unsigned char i;	   
	   for(i=0;i<6;i++)   //������������ָ�ƿ�
           {
    	      uartObjFP_Send_Byte(FP_Pack_Head[i]);   
   		   }

       for(i=0;i<11;i++)
           {
    	      uartObjFP_Send_Byte(FP_Search[i]);   
   		   }


}


//����0-9öָ�ơ�������6
void FINGERPRINT_Cmd_Search_Finger_Admin(void)
{
       unsigned char i;	   
	   for(i=0;i<6;i++)   //������������ָ�ƿ�
           {
    	      uartObjFP_Send_Byte(FP_Pack_Head[i]);   
   		   }

       for(i=0;i<11;i++)
           {
    	      uartObjFP_Send_Byte(FP_Search_0_9[i]);   
   		   }


}


//����0-100öָ�ơ�������custom
void FINGERPRINT_Cmd_Search_Finger_0_100(void)
{
       unsigned char i;	   
	   for(i=0;i<6;i++)   //������������ָ�ƿ�
           {
    	      uartObjFP_Send_Byte(FP_Pack_Head[i]);   
   		   }

       for(i=0;i<11;i++)
           {
    	      uartObjFP_Send_Byte(FP_Search_0_100[i]);   
   		   }


}

//ɾ��ָ��ģ���������ָ��ģ�桪������7
void FINGERPRINT_Cmd_Delete_All_Model(void)
{
     unsigned char i;    

    for(i=0;i<6;i++) //��ͷ
      uartObjFP_Send_Byte(FP_Pack_Head[i]);   

    for(i=0;i<6;i++) //����ϲ�ָ��ģ��
      uartObjFP_Send_Byte(FP_Delet_All_Model[i]);   
}



//ɾ��ָ��ģ�����ָ��ָ��ģ�桪������Ϊ˫�ֽڡ�������7
void FINGERPRINT_Cmd_Delete_Model(unsigned int uiID_temp)
{
    volatile unsigned int uiSum_temp = 0;
	unsigned char i;    
	 
	FP_Delete_Model[4]=(uiID_temp&0xFF00)>>8;
	FP_Delete_Model[5]=(uiID_temp&0x00FF);
	
	for(i=0;i<8;i++)
	    uiSum_temp = uiSum_temp + FP_Delete_Model[i];
	
	//UART0_Send_Byte(uiSum_temp);	
			
	FP_Delete_Model[8]=(uiSum_temp&0xFF00)>>8;
	FP_Delete_Model[9]=uiSum_temp&0x00FF;
	 

    for(i=0;i<6;i++) //��ͷ
      uartObjFP_Send_Byte(FP_Pack_Head[i]);   

    for(i=0;i<10;i++) //����ϲ�ָ��ģ��
      uartObjFP_Send_Byte(FP_Delete_Model[i]);   
}

//ɾ��ָ��ģ��-����Ϊ���ֽڡ�������7
void FP_Cmd_Delete_Model(unsigned char n)
{
    volatile unsigned int uiSum_temp = 0;
	unsigned char i;    
	 
//	FP_Delete_Model[4]=(uiID_temp&0xFF00)>>8;
	FP_Delete_Model[5]=(n&0xFF);
	
	for(i=0;i<8;i++)
	    uiSum_temp = uiSum_temp + FP_Delete_Model[i];
	
	//UART0_Send_Byte(uiSum_temp);	
			
	FP_Delete_Model[8]=(uiSum_temp&0xFF00)>>8;
	FP_Delete_Model[9]=uiSum_temp&0x00FF;
	 

    for(i=0;i<6;i++) //��ͷ
      uartObjFP_Send_Byte(FP_Pack_Head[i]);   

    for(i=0;i<10;i++) //����ϲ�ָ��ģ��
      uartObjFP_Send_Byte(FP_Delete_Model[i]);   
}


//���ָ��ģ��������������8
void FINGERPRINT_Cmd_Get_Templete_Num(void)
{  unsigned int i;
//   unsigned char temp[14];

   for(i=0;i<6;i++) //��ͷ
      uartObjFP_Send_Byte(FP_Pack_Head[i]);

   //�������� 0x1d
   for(i=0;i<6;i++)
     uartObjFP_Send_Byte(FP_Templete_Num[i]);
   
  
}

//���շ������ݻ���
void FINGERPRINT_Recevice_Data(unsigned char ucLength){
	
	u8 loop = 0;

	for (loop = 0; loop < ucLength; loop ++){

		FINGERPRINT_RECEVICE_BUFFER[loop] = UART_FP_byteRcv();
		if(FP_Rcv_timeOut_FLG){	//��ʱ����
			
			delayMs(100);
			
			FP_Rcv_timeOut_FLG  = 0;
			memset(&FINGERPRINT_RECEVICE_BUFFER[0], 0xff, ucLength * sizeof(u8));
			umNew_AddStep	= umStepAdd_A;
			um_dttStep 		= umStepDtt_Cfail;	//��Ӧ���ݽ��ճ�ʱ��ǿ��ʹʧ��
			
			delayMs(100);
			
			return;
		}
	}
}

//��BUFFER1�е��������ŵ�ָ����λ��  ����ģ�壺�������ļ���ŵ�flashָ�ƿ⡪������9
void FINGERPRINT_Cmd_Save_Finger( unsigned char ucH_Char,unsigned char ucL_Char )
{
           unsigned long temp = 0;
		   unsigned char i;

//           SAVE_FINGER[9]={0x01,0x00,0x06,0x06,0x01,0x00,0x0B,0x00,0x19};//��BUFFER1�е��������ŵ�ָ����λ��

           FP_Save_Finger[5] = ucH_Char;
           FP_Save_Finger[6] = ucL_Char;
           
		   for(i=0;i<7;i++)   //����У���
		   	   temp = temp + FP_Save_Finger[i];
			    
		   FP_Save_Finger[7]=(temp & 0x00FF00) >> 8; //���У������
		   FP_Save_Finger[8]= temp & 0x0000FF;
		   
           for(i=0;i<6;i++)    
    	       uartObjFP_Send_Byte(FP_Pack_Head[i]);        //���Ͱ�ͷ

           for(i=0;i<9;i++)  
      		   uartObjFP_Send_Byte(FP_Save_Finger[i]);      //�������� ��ͼ��ת���� ������ ����� CHAR_buffer1
}

void FP_Cmd_Save_Finger(unsigned char n )	  //cyacyf�޸ģ���Ϊ1��������������9
{
           unsigned long temp = 0;
		   unsigned char i;

//           SAVE_FINGER[9]={0x01,0x00,0x06,0x06,0x01,0x00,0x0B,0x00,0x19};//��BUFFER1�е��������ŵ�ָ����λ��

          // FP_Save_Finger[5] = ucH_Char;
           FP_Save_Finger[6] = n;
           
		   for(i=0;i<7;i++)   //����У���
		   	   temp = temp + FP_Save_Finger[i];
			    
		   FP_Save_Finger[7]=(temp & 0x00FF00) >> 8; //���У������
		   FP_Save_Finger[8]= temp & 0x0000FF;
		   
           for(i=0;i<6;i++)    
    	       uartObjFP_Send_Byte(FP_Pack_Head[i]);        //���Ͱ�ͷ

           for(i=0;i<9;i++)  
      		   uartObjFP_Send_Byte(FP_Save_Finger[i]);      //�������� ��ͼ��ת���� ������ ����� CHAR_buffer1
}

//��flash���ݿ���ָ��ID�ŵ�ָ��ģ����뵽ģ�建����BUFFER1
void FINGERPRINT_Cmd_Load_Model( unsigned char ucH_Char,unsigned char ucL_Char )
{
           unsigned long temp = 0;
		   unsigned char i;

//         FP_Load_Model[9]={0x01,0x00,0x06,0x07,0x01,0x00,0x0B,0x00,0x1A};//��flash���ݿ���ָ��ID�ŵ�ָ��ģ����뵽ģ�建����BUFFER1

           FP_Load_Model[5] = ucH_Char;
           FP_Load_Model[6] = ucL_Char;
           
		   for(i=0;i<7;i++)   //����У���
		   	   temp = temp + FP_Load_Model[i];
			    
		   FP_Load_Model[7]=(temp & 0x00FF00) >> 8; //���У������
		   FP_Load_Model[8]= temp & 0x0000FF;
		   
           for(i=0;i<6;i++)    
    	       uartObjFP_Send_Byte(FP_Pack_Head[i]);        //���Ͱ�ͷ

           for(i=0;i<9;i++)  
      		   uartObjFP_Send_Byte(FP_Load_Model[i]);      //�������� ��ָ��ID�ŵ�ָ��ģ����뵽ģ�建����BUFFER1
}

unsigned char FP_Cmd_Load_Model( unsigned char ucH_Char,unsigned char ucL_Char)
{
	 FINGERPRINT_Cmd_Load_Model(ucH_Char,ucL_Char );
	 FINGERPRINT_Recevice_Data(12); //����12�����ȵķ�����
	 if(FINGERPRINT_RECEVICE_BUFFER[9]==0x0)//ģ������ɹ�
	 return 0x01;
	 else if(FINGERPRINT_RECEVICE_BUFFER[9]==0x0C)	//ģ������д��ģ����Ч
	 return 0x02;
	 else 
	 return 0;
}

void FP_imageCollection(u8 rp_time){

	const	u8 cmdRepeat_time	= rp_time;
			u8 cmdRepeat_cnt	= 0;
	
	do{
	
		FINGERPRINT_Cmd_Get_Img();
		FINGERPRINT_Recevice_Data(12);
		cmdRepeat_cnt ++;
	}while((FINGERPRINT_RECEVICE_BUFFER[9] != 0) &&
		   (cmdRepeat_cnt < cmdRepeat_time));
}

/*ָ��ѧϰ*///ָ��������û�
void FP_userModelNew_Add(unsigned char ucH_user,unsigned char ucL_user){
	
	static	u8 *Pointer;
		
	const	u8 cmdRepeat_time	= 3;
			u8 cmdRepeat_cnt	= 0;
	
	Pointer = &FINGERPRINT_RECEVICE_BUFFER[0];
	
	switch(umNew_AddStep){
	
		case umStepAdd_A:{
		
			FP_imageCollection(2);
//			uartObjFP_Send_String(Pointer, 12);
			
			if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
			
				Mode_umAdd_timeOutcnt = umAddMode_TIMEOUT;  //��ʱ���ֵ����
				
				FINGERPRINT_Cmd_Img_To_Buffer1(); 	//��ͼ��ת��������������Buffer1��
				FINGERPRINT_Recevice_Data(12);   	//����12�����ȵķ�����
//				uartObjFP_Send_String(Pointer, 12);
				
				if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
				
					umNew_AddStep_StandBy = umStepAdd_B;
				}
			}
			
			if(umStepAdd_B == umNew_AddStep_StandBy){
				
				FP_imageCollection(1);
//				uartObjFP_Send_String(Pointer, 12);
				
				if(FINGERPRINT_RECEVICE_BUFFER[9] == 0x02){
				
					umNew_AddStep = umStepAdd_B;
					
					Mode_umAdd_timeOutcnt = umAddMode_TIMEOUT;  //��ʱ���ֵ����
				}
			}
		}break;
		
		case umStepAdd_B:{
		
			FP_imageCollection(3);
//			uartObjFP_Send_String(Pointer, 12);
			
			if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
			
				FINGERPRINT_Cmd_Img_To_Buffer2(); 	//��ͼ��ת��������������Buffer2��
				FINGERPRINT_Recevice_Data(12);   	//����12�����ȵķ�����
//				uartObjFP_Send_String(Pointer, 12);
				
				if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
					
//					Mode_umAdd_timeOutcnt = umAddMode_TIMEOUT;  //��ʱ���ֵ����
				
					FINGERPRINT_Cmd_Reg_Model();
					FINGERPRINT_Recevice_Data(12); 
//					uartObjFP_Send_String(Pointer, 12);
					
					if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
					
						umNew_AddStep_StandBy = umStepAdd_C;
					}
				}
			}
			
			if(umStepAdd_C == umNew_AddStep_StandBy){
			
				FP_imageCollection(1);
//				uartObjFP_Send_String(Pointer, 12);
				
				beeps(11);	//׼��ɣ���ʾ��
				
				if(FINGERPRINT_RECEVICE_BUFFER[9] == 0x02){
				
					umNew_AddStep = umStepAdd_C;
				}
			}
		}break;
		
		case umStepAdd_C:{					//����һ
		
			FINGERPRINT_Cmd_Save_Finger(ucH_user,ucL_user);
			
			FINGERPRINT_Recevice_Data(12);
//			uartObjFP_Send_String(Pointer, 12);
			
			if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
			
				umNew_AddStep_StandBy = umStepAdd_D;
			}
			
			if(umStepAdd_D == umNew_AddStep_StandBy){
			
				FP_imageCollection(1);
//				uartObjFP_Send_String(Pointer, 12);
				
				if(FINGERPRINT_RECEVICE_BUFFER[9] == 0x02){
				
					umNew_AddStep 		  = umStepAdd_D;
					umNew_AddStep_StandBy = umStepAdd_Cmp;
				}
			}
		}break;
		
//		case umStepAdd_D:{					//������
//		
//			FP_imageCollection(2);
//			uartObjFP_Send_String(Pointer, 12);
//			
//			if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
//				
//				FINGERPRINT_Cmd_Delete_Model((u16)ucL_user);	//flag=4ʱ��⵽��ָ�ư���������¼��
//				FINGERPRINT_Recevice_Data(12);   				//����12�����ȵķ�����
//				
//				if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
//				
//					umNew_AddStep_StandBy = umStepAdd_D;
//				}
//			}
//			
//			if(umStepAdd_D == umNew_AddStep_StandBy){
//			
//				FP_imageCollection(1);
//				uartObjFP_Send_String(Pointer, 12);
//				
//				if(FINGERPRINT_RECEVICE_BUFFER[9] == 0x02){
//				
//					umNew_AddStep = umStepAdd_Fault;
//				}
//			}
//		}break;
		
		default:break;
	}
	
	return;
}

//ָ�Ƽ�����
u8 FP_Detecting(void){	
	
//	static	u8 	*Pointer;	//�ط�����
	
	static	u8 	FP_ID = 0;
	
	u8 xdata 	log_info[60] 	= {0};
	
//	Pointer = &FINGERPRINT_RECEVICE_BUFFER[0];
	
	if(FPvalidation_flg)return FPID_NULL;	//��ʶ����ɱ�־δ���㣬������
			
	if(umStepDtt_A == um_dttStep){
		
		FP_imageCollection(1);
//		uartObjFP_Send_String(Pointer,12);
		
		if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
		
			um_dttStep = umStepDtt_B;
			
			FINGERPRINT_Cmd_Img_To_Buffer1(); //��ͼ��ת��������������Buffer1��
			FINGERPRINT_Recevice_Data(12);   //����12�����ȵķ�����
//			uartObjFP_Send_String(Pointer, 12);	
			
			if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
			
				FINGERPRINT_Cmd_Search_Finger_0_100();
				FINGERPRINT_Recevice_Data(16);
//				uartObjFP_Send_String(Pointer, 16);
				
				if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
				
					um_dttStep = umStepDtt_Csuccess;
					
					FP_ID = FINGERPRINT_RECEVICE_BUFFER[11];
					
					/*��ʾ������*///һ����
					beeps(0);	//��ʾ��
					
				}else{
				
					um_dttStep = umStepDtt_Cfail;
					
					/*��ʾ������*///һ����
				}
			}else{
			
				um_dttStep = umStepDtt_A;	//״̬�ָ�
			}
		}
	}
	
	if(umStepDtt_Cfail == um_dttStep){
	
		FP_imageCollection(1);
//		uartObjFP_Send_String(Pointer,12);
		
		/*��ʾ������*///ѭ��
		beepTips_s(4, 8, 8);	//��ʾ��
		
		if(FINGERPRINT_RECEVICE_BUFFER[9] == 0x02){
		
			um_dttStep = umStepDtt_A;
			FPvalidation_flg = 0;
			
			sprintf(log_info, "��⵽�Ƿ�ָ�ƴ���������\n");
			LogDats(log_info, strlen(log_info));
		}
	}
			
	if(umStepDtt_Csuccess == um_dttStep){
		
		/*������Ҫ�Զ����޸��߼���ʱҪ��ָ�뿪ʱ��Ӧ����������Ӧ��������Ӧ����Ҫ��ָ�ͷ��ж��߼�*/
		
		static bit fp_releas_FLAG = 1;	//��ָ�ͷű�־
	
		FP_imageCollection(1);
//		uartObjFP_Send_String(Pointer,12);
		
		if(fp_releas_FLAG){
			
			fp_releas_FLAG	 = 0;
			FPvalidation_flg = 1;
		}
		
		/*��ʾ������*///ѭ��
		
		if(FINGERPRINT_RECEVICE_BUFFER[9] == 0x02){
			
			um_dttStep 		 = umStepDtt_A;
			
			fp_releas_FLAG 	 = 1;
//			FPvalidation_flg = 1;
			
			sprintf(log_info, "��⵽�Ϸ�ָ�ƴ�������Ӧָ��IDΪ�� %d.\n", (int)FP_ID);
			LogDats(log_info, strlen(log_info));
			
			return FP_ID;
		}
	}
	
	return FPID_NULL;
}

//ָ��ģ�����������ģʽ�£����ܱ���ֹʱ�������ĳ���
void FP_Detecting_Disable(void){

	FP_imageCollection(1);
	if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
	
		um_dttStep = umStepDtt_B;
		beeps(12);	//��ʾ��
		
	}else{
	
		um_dttStep = umStepDtt_A;
	}
}

/*********************ָ��ģ���������߳�***********************/
void thread_moudleFP(void){
	
	u8 data repeatTX_buff[45]		= {0};
	u8 data fpID_Tab[13]			= {0};
	
//	u8 code rptTX_Continue_Delay	= 30;	//�����������ݼ��_30ms�����ٶ���
	
	switch(process_FPworkMode){
	
		case work_Mode_umAdd:{
			
			u8 code detect_Period = 100; //�����������������
			static u8 detect_tCount = 0;
			
			delayMs(2);//������ʱ
			if(detect_tCount < detect_Period)detect_tCount ++; //���������ڼ��
			else{
			
				FP_userModelNew_Add(0, FP_ID);
				
				if(umNew_AddStep_StandBy == umStepAdd_B){	//����¼������
				
					if(umNew_AddStepReapt_cmpHalfEN){
					
						umNew_AddStepReapt_cmpHalfEN = 0;	//ʹ�ܹرգ�ֻ��һ��
						
						beeps(5);	//��ʾ��
						
						//������Ӧ���ظ�
						repeatTX_buff[15]	= FP_ID;
						repeatTX_buff[12]	= 0x0A;		//����1�������״̬ 
						
						dtasTX_loadBasic_AUTO( repeatTX_buff,				/*����ǰ���װ��*///���Ͱ�������Ϣ���
											   FRAME_TYPE_StoM_RCVsuccess,
											   FRAME_MtoSCMD_cmdFPLearn,		
											   0
											 );
						uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);	//����ɣ�����λ����Ӧ״̬
					}
				}
				
				if(umNew_AddStep == umStepAdd_D){	//����¼��ȫ���
					
					u8 fpStore_addr 	= EEPROM_ADDR_FP_STORE + 12 - FP_ID / 8;
					u8 val_Store 		= 0;
					
					EEPROM_read_n(fpStore_addr, &val_Store, 1);		//ģ��ռλ�洢����
					val_Store |= 1 << (FP_ID % 8);	
					coverEEPROM_write_n(fpStore_addr, &val_Store, 1);
					
					FP_modeCompleteTips = cmp_umAdd_Success;		//���״�� Tips����
					FPmodeCmpTips_cnt	= FPmodeCmpTips_Period;
					
					//������Ӧ���ظ�
					repeatTX_buff[15]	= FP_ID;
					repeatTX_buff[12]	= 0x0B;
					EEPROM_read_n(EEPROM_ADDR_FP_STORE, &fpID_Tab[0], 13);	
					memcpy(&repeatTX_buff[16], &fpID_Tab[0], 13);
					
					dtasTX_loadBasic_AUTO( repeatTX_buff,				/*����ǰ���װ��*///���Ͱ�������Ϣ���
										   FRAME_TYPE_StoM_RCVsuccess,
										   FRAME_StoMRPY_rpyLearnSuccess,	//ѧϰ�ɹ�
										   0	
										 );
					uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);	//ȫ��ɣ�����λ����Ӧ״̬
					
					FP_ID 				= FPID_NULL;
					process_FPworkMode 	= work_Mode_NULL;
					umNew_AddStep 		= umStepAdd_A;

					LogString("ģ��ѧϰ�ɹ� !!!\n");
				}
				
				if(!Mode_umAdd_timeOutcnt){	//��ʱ�˳�
				
					led_darkAll();
					
					beepTips_s(5, 20, 8);	//��ʾ��
					
					FP_modeCompleteTips = cmp_umAdd_tOut;		//���״�� Tips����
					FPmodeCmpTips_cnt	= FPmodeCmpTips_Period;
					
					//������Ӧ���ظ�
					repeatTX_buff[15]	= FP_ID;
					repeatTX_buff[12]	= 0x0F;
					
					dtasTX_loadBasic_AUTO( repeatTX_buff,			/*����ǰ���װ��*///���Ͱ�������Ϣ���
										   FRAME_TYPE_StoM_RCVsuccess,
										   FRAME_StoMRPY_rpyLearnFail,	//ѧϰʧ��
										   0
										 );
					uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);	//δ��ɣ�����λ����Ӧ״̬
					
					FP_ID 				= FPID_NULL;
					process_FPworkMode 	= work_Mode_NULL;
					umNew_AddStep 		= umStepAdd_A;
					
					LogString("ģ��ѧϰʧ�ܣ���ʱ�˳� !!!\n");
				}
			}
			
		}break;
		
		case work_Mode_umDel:{
		
			FP_Cmd_Delete_Model(FP_ID);
			FINGERPRINT_Recevice_Data(12);   				//����12�����ȵķ�����
			
			if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
			
				u8 fpStore_addr 	= EEPROM_ADDR_FP_STORE + 12 - FP_ID / 8;
				u8 val_Store 		= 0;
				
				EEPROM_read_n(fpStore_addr, &val_Store, 1);		//ģ��ռλɾ��
				val_Store &= ~(1 << (FP_ID % 8));	
				coverEEPROM_write_n(fpStore_addr, &val_Store, 1);
				
				//������Ӧ���ظ�
				repeatTX_buff[15]	= FP_ID;
				repeatTX_buff[12]	= 0x0A;		//����1���ɾ�����
				
				dtasTX_loadBasic_AUTO( repeatTX_buff,				/*����ǰ���װ��*///���Ͱ�������Ϣ���
									   FRAME_TYPE_StoM_RCVsuccess,
									   FRAME_MtoSCMD_cmdFPDelete,	//ɾ���ɹ�
									   0
									 );
				uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);	//����ɣ�����λ����Ӧ״̬
//				delayMs(rptTX_Continue_Delay);
//				uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);	//�෢һ�Σ�ȷ���Է�����
				
				FP_ID = FPID_NULL;
				process_FPworkMode 	= work_Mode_NULL;
				fpModeTrig_umDetect();	//ָ��ģ���߳������������ģʽ

				LogString("ģ��ɾ���ɹ� !!!\n");				
			}
		}break;
			
		case work_Mode_umDetect:{
			
			u8 code detect_Period = 50; //�����������������
			static u8 detect_tCount = 0;
			
			delayMs(2);//������ʱ
			if(detect_tCount < detect_Period)detect_tCount ++; //���������ڼ��
			else{
			
				detect_tCount = 0;
				
				if(FP_umDetect_enFLAG){	//�Ƿ����������ʹ��
					
					u8 detect_Result = FP_Detecting();	//�̵�����Ӧ���ڼ̵��������ļ���������ϴ��� 
					
					if(detect_Result != FPID_NULL){		
						
						//������Ӧ���ظ�
						repeatTX_buff[15] = detect_Result;	//ָ�ƺ����
						(status_Relay)?(repeatTX_buff[11] = 0x00):(repeatTX_buff[11] = 0x01);//����״̬��䣨��תԤ�⣩
						(process_RelayWorkMode == work_Mode_flip)?(repeatTX_buff[14] = 0x01):(repeatTX_buff[14] = 0x02);//���ع���ģʽ��� ��תģʽ��0x01 ��ʱģʽ��0x02
						dtasTX_loadBasic_CUST( DATATRANS_objFLAG_MOBILE,	/*����ǰ���װ��*///���Ͱ�������Ϣ���
											   repeatTX_buff,			
											   FRAME_TYPE_StoM_RCVsuccess,
											   FRAME_MtoSCMD_cmdControl,
											   0
											 );
						uartObjWIFI_Send_String(repeatTX_buff, dataTransLength_objMOBILE);	//�ֶ����ƿ��أ������ϴ������������������¼���	--ǿ��33�ֽ�
					}
					
				}else{
				
					FP_Detecting_Disable();	//����������ʱ���ղ�
				}	
			}
			
		}break;
		
		default:{
		
			process_FPworkMode = work_Mode_NULL;
			
		}break;
	}
	
	if(FP_modeCompleteTips != cmp_NULL){	//Tips����
	
		switch(FP_modeCompleteTips){
		
			case cmp_umAdd_Success:{	//ѧϰ�ɹ�Tips
			
				if(!FPmodeCmpTips_cnt){
				
					FP_modeCompleteTips = cmp_NULL;
					led_darkAll();
					fpModeTrig_umDetect();	//ָ��ģ���߳������������ģʽ
					
				}else{
				
					if((FPmodeCmpTips_cnt / 150) ==10)led_darkAll();
					if((FPmodeCmpTips_cnt / 150) == 9)led_Act(red,	led_on);
					if((FPmodeCmpTips_cnt / 150) == 8)led_Act(green,led_on);
					if((FPmodeCmpTips_cnt / 150) == 7)led_Act(blue, led_on);
					if((FPmodeCmpTips_cnt / 150) == 6)led_Act(red,	led_on);
					if((FPmodeCmpTips_cnt / 150) == 5)led_Act(green,led_on);
					if((FPmodeCmpTips_cnt / 150) == 4)led_Act(blue, led_on);
					if((FPmodeCmpTips_cnt / 150) == 3)led_Act(red,  led_on);
					if((FPmodeCmpTips_cnt / 150) == 2)led_Act(green,led_on);
					if((FPmodeCmpTips_cnt / 150) == 1)led_Act(blue, led_on);
				}
			}break;
			
			case cmp_umAdd_tOut:{	//ѧϰʧ�ܣ���ʱ�˳�Tips
			
				if(!FPmodeCmpTips_cnt){
				
					FP_modeCompleteTips = cmp_NULL;
					led_darkAll();
					fpModeTrig_umDetect();	//ָ��ģ���߳������������ģʽ
					
				}else{
				
					if((FPmodeCmpTips_cnt / 200) == 7)led_darkAll();
					if((FPmodeCmpTips_cnt / 200) == 6)led_Act(red,	led_on);
					if((FPmodeCmpTips_cnt / 200) == 5)led_darkAll();
					if((FPmodeCmpTips_cnt / 200) == 4)led_Act(red,	led_on);
					if((FPmodeCmpTips_cnt / 200) == 3)led_darkAll();
					if((FPmodeCmpTips_cnt / 200) == 2)led_Act(red,	led_on);
					if((FPmodeCmpTips_cnt / 200) == 1)led_darkAll();
				}
			}break;
			
			default:{
			
				FP_modeCompleteTips = cmp_NULL;
				
			}break;
		}
	}
}

/********************ָ��ģ���߳�ģʽ����************************/
void fpModeTrig_umAdd(void){

	Mode_umAdd_timeOutcnt 		 = umAddMode_TIMEOUT;
	process_FPworkMode			 = work_Mode_umAdd;		//ָ��ģ���߳�ģʽ�л�������ѧϰ
	umNew_AddStep 				 = umStepAdd_A;
	umNew_AddStep_StandBy 		 = umStepAdd_A;
	
	umNew_AddStepReapt_cmpHalfEN = 1;				//����ɷ����ݰ�ʹ�ܣ���ֹ�෢�����б�־λ�޴�
	
	FP_modeCompleteTips 		 = cmp_NULL;
}

void fpModeTrig_umDel(void){

	process_FPworkMode		= work_Mode_umDel;		//ָ��ģ���߳�ģʽ�л���ģ��ɾ��
}

void fpModeTrig_umDetect(void){

	process_FPworkMode		= work_Mode_umDetect;	//ָ��ģ���߳�ģʽ�л����������
	FPvalidation_flg		= 0;
}

/********************��ӡָ��ռλģ��****************************/
void logOut_fpID_place(void){

	unsigned char xdata log_info[100] 		= {0};
	unsigned char xdata log_info_fpID[60] 	= {0};
	unsigned char idata fpID_Tab[13]		= {0};
	
	unsigned char idata loop				= 0; 
	
	EEPROM_read_n(EEPROM_ADDR_FP_STORE, &fpID_Tab[0], 13);
	
	for(loop = 0; loop < 13; loop ++)
		sprintf(&log_info_fpID[loop * 3], "%02X ", (int)fpID_Tab[loop]);
	
	sprintf(log_info, "��ǰָ��ģ��ռλΪ��%s \n", log_info_fpID);
	LogDats(log_info, strlen(log_info));
}


bit fpID_allClear(void){

	u8 loop = 5;
	
	for(loop = 5; loop != 0; loop --){
	
		FINGERPRINT_Cmd_Delete_All_Model();
		FINGERPRINT_Recevice_Data(12);
		
		if(FINGERPRINT_RECEVICE_BUFFER[9] == 0)return 1;
	}
	
	return 0;
}


