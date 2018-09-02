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

//*********************数据传输变量引用区**********************/
extern 	u8 xdata 			repeatTX_Len;

//*********************继电器驱动变量引用区**********************/
extern  threadRelay_Mode	process_RelayWorkMode;
extern	bit 				status_Relay;

//*********************串口外设库变量引用区********************/

/**********************本地文件变量定义区************************/
threadFP_Mode		process_FPworkMode			 = work_Mode_NULL;		//指纹模块工作模式
stt_stepAdd 		umNew_AddStep				 = umStepAdd_A;			//指纹模块工作模式为：增/改    模板时，对应的进程指示
stt_stepAdd 		umNew_AddStep_StandBy 		 = umStepAdd_A;			//指纹模块工作模式为：增/改    模板时，对应的预备进程指示
stt_stepDtt			um_dttStep 					 = umStepDtt_A;			//指纹模块工作模式为：主动检测 模板时，对应的进程指示
bit					FP_umDetect_enFLAG			 = 1;					//指纹模块工作模式为：主动检测 模板时，工作使能标志，用于权限定时，初始化时开放检测
bit					FPvalidation_flg			 = 0;					//指纹模块工作模式为：模板识别时，识别完成标志，业务代码调用时必须手动置零
u8 					FP_ID						 = FPID_NULL;			//模板ID号

bit 				umNew_AddStepReapt_cmpHalfEN = 0;					//接收到指纹模块学习指令时，半完成状态时信息回发使能

u16	xdata			Mode_umAdd_timeOutcnt		 = 0;					//用于学习模式时超时检测

tipsFP_completeType FP_modeCompleteTips 		 = cmp_NULL;			//模式触发完成后Tips类别
volatile u16 xdata	FPmodeCmpTips_cnt			 = 0;					//模式触发完成后Tips专用计时，需在timer中调用
u16 code 			FPmodeCmpTips_Period		 = 1500;				//模式触发完成后Tips专用计时最大周期

bit					FP_Rcv_timeOut_FLG			 = 0;					//指纹模块数据接收超时标志		

volatile unsigned char idata FINGERPRINT_RECEVICE_BUFFER[24] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

/*===============================================================*/

//FINGERPRINT通信协议定义
unsigned char code FP_Pack_Head[6] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF};  //协议包头
unsigned char code FP_Get_Img[6] = {0x01,0x00,0x03,0x01,0x0,0x05};    //获得指纹图像————1
unsigned char code FP_Img_To_Buffer1[7]={0x01,0x0,0x04,0x02,0x01,0x0,0x08}; //将图像放入到BUFFER1————2
unsigned char code FP_Img_To_Buffer2[7]={0x01,0x0,0x04,0x02,0x02,0x0,0x09}; //将图像放入到BUFFER2—————3
unsigned char code FP_Reg_Model[6]={0x01,0x0,0x03,0x05,0x0,0x09}; //将BUFFER1跟BUFFER2合成特征模版————4

unsigned char code FP_Search[11]={0x01,0x0,0x08,0x04,0x01,0x0,0x0,0x03,0xA1,0x0,0xB2}; //搜索指纹搜索范围0 - 929  ————5
unsigned char code FP_Search_0_9[11]={0x01,0x0,0x08,0x04,0x01,0x0,0x0,0x0,0x09,0x0,0x17}; //搜索0-9号指纹————6
unsigned char code FP_Search_0_100[11]={0x01,0x0,0x08,0x04,0x01,0x0,0x0,0x0,0x64,0x0,0x72}; //搜索0-9号指纹————6

unsigned char code FP_Delet_All_Model[6]={0x01,0x0,0x03,0x0d,0x00,0x11};//删除指纹模块里所有的模版————7
unsigned char code FP_Templete_Num[6] ={0x01,0x00,0x03,0x1D,0x00,0x21 }; //获得模版总数—————8

volatile unsigned char idata FP_Save_Finger[9]={0x01,0x00,0x06,0x06,0x01,0x00,0x0B,0x00,0x19};//将BUFFER1中的特征码存放到指定的位置————9
volatile unsigned char idata FP_Delete_Model[10]={0x01,0x00,0x07,0x0C,0x0,0x0,0x0,0x1,0x0,0x0}; //删除指定的模版————7

//volatile unsigned char FINGER_NUM;
/*cyacyf增加*/
volatile unsigned char idata FP_Load_Model[9]={0x01,0x0,0x06,0x07,0x01,0x00,0x0B,0x00,0x1A}; //将flash数据库中指定ID号的指纹模板读入到BUFFER1


/*------------------ FINGERPRINT命令字 --------------------------*/

//FINGERPRINT_获得指纹图像命令————1
void FINGERPRINT_Cmd_Get_Img(void)
{
    unsigned char i;

    for(i=0;i<6;i++) //发送包头
       uartObjFP_Send_Byte(FP_Pack_Head[i]);
    
    for(i=0;i<6;i++) //发送命令 0x1d
       uartObjFP_Send_Byte(FP_Get_Img[i]);
}

//讲图像转换成特征码存放在Buffer1中————2
void FINGERPRINT_Cmd_Img_To_Buffer1(void)
{
 	    unsigned char i;
    
	       for(i=0;i<6;i++)    //发送包头
	         {
    	       uartObjFP_Send_Byte(FP_Pack_Head[i]);   
   		     }
           
   		   for(i=0;i<7;i++)   //发送命令 将图像转换成 特征码 存放在 CHAR_buffer1
             {
      		   uartObjFP_Send_Byte(FP_Img_To_Buffer1[i]);
   		     }
}

//将图像转换成特征码存放在Buffer2中————3
void FINGERPRINT_Cmd_Img_To_Buffer2(void)
{
     unsigned char i;
           for(i=0;i<6;i++)    //发送包头
	         {
    	       uartObjFP_Send_Byte(FP_Pack_Head[i]);   
   		     }
           
   		   for(i=0;i<7;i++)   //发送命令 将图像转换成 特征码 存放在 CHAR_buffer1
             {
      		   uartObjFP_Send_Byte(FP_Img_To_Buffer2[i]);
   		     }
}

//将BUFFER1 跟 BUFFER2 中的特征码合并成指纹模版————4
void FINGERPRINT_Cmd_Reg_Model(void)
{
    unsigned char i;    

    for(i=0;i<6;i++) //包头
    {
      uartObjFP_Send_Byte(FP_Pack_Head[i]);   
    }

    for(i=0;i<6;i++) //命令合并指纹模版
    {
      uartObjFP_Send_Byte(FP_Reg_Model[i]);   
    }

}

//搜索全部用户999枚指纹	————5
void FINGERPRINT_Cmd_Search_Finger(void)
{
       unsigned char i;	   
	   for(i=0;i<6;i++)   //发送命令搜索指纹库
           {
    	      uartObjFP_Send_Byte(FP_Pack_Head[i]);   
   		   }

       for(i=0;i<11;i++)
           {
    	      uartObjFP_Send_Byte(FP_Search[i]);   
   		   }


}


//搜索0-9枚指纹————6
void FINGERPRINT_Cmd_Search_Finger_Admin(void)
{
       unsigned char i;	   
	   for(i=0;i<6;i++)   //发送命令搜索指纹库
           {
    	      uartObjFP_Send_Byte(FP_Pack_Head[i]);   
   		   }

       for(i=0;i<11;i++)
           {
    	      uartObjFP_Send_Byte(FP_Search_0_9[i]);   
   		   }


}


//搜索0-100枚指纹————custom
void FINGERPRINT_Cmd_Search_Finger_0_100(void)
{
       unsigned char i;	   
	   for(i=0;i<6;i++)   //发送命令搜索指纹库
           {
    	      uartObjFP_Send_Byte(FP_Pack_Head[i]);   
   		   }

       for(i=0;i<11;i++)
           {
    	      uartObjFP_Send_Byte(FP_Search_0_100[i]);   
   		   }


}

//删除指纹模块里的所有指纹模版————7
void FINGERPRINT_Cmd_Delete_All_Model(void)
{
     unsigned char i;    

    for(i=0;i<6;i++) //包头
      uartObjFP_Send_Byte(FP_Pack_Head[i]);   

    for(i=0;i<6;i++) //命令合并指纹模版
      uartObjFP_Send_Byte(FP_Delet_All_Model[i]);   
}



//删除指纹模块里的指定指纹模版——参数为双字节————7
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
	 

    for(i=0;i<6;i++) //包头
      uartObjFP_Send_Byte(FP_Pack_Head[i]);   

    for(i=0;i<10;i++) //命令合并指纹模版
      uartObjFP_Send_Byte(FP_Delete_Model[i]);   
}

//删除指纹模板-参数为单字节————7
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
	 

    for(i=0;i<6;i++) //包头
      uartObjFP_Send_Byte(FP_Pack_Head[i]);   

    for(i=0;i<10;i++) //命令合并指纹模版
      uartObjFP_Send_Byte(FP_Delete_Model[i]);   
}


//获得指纹模板数量————8
void FINGERPRINT_Cmd_Get_Templete_Num(void)
{  unsigned int i;
//   unsigned char temp[14];

   for(i=0;i<6;i++) //包头
      uartObjFP_Send_Byte(FP_Pack_Head[i]);

   //发送命令 0x1d
   for(i=0;i<6;i++)
     uartObjFP_Send_Byte(FP_Templete_Num[i]);
   
  
}

//接收反馈数据缓冲
void FINGERPRINT_Recevice_Data(unsigned char ucLength){
	
	u8 loop = 0;

	for (loop = 0; loop < ucLength; loop ++){

		FINGERPRINT_RECEVICE_BUFFER[loop] = UART_FP_byteRcv();
		if(FP_Rcv_timeOut_FLG){	//超时处理
			
			delayMs(100);
			
			FP_Rcv_timeOut_FLG  = 0;
			memset(&FINGERPRINT_RECEVICE_BUFFER[0], 0xff, ucLength * sizeof(u8));
			umNew_AddStep	= umStepAdd_A;
			um_dttStep 		= umStepDtt_Cfail;	//响应数据接收超时，强制使失败
			
			delayMs(100);
			
			return;
		}
	}
}

//将BUFFER1中的特征码存放到指定的位置  储存模板：将特征文件存放到flash指纹库————9
void FINGERPRINT_Cmd_Save_Finger( unsigned char ucH_Char,unsigned char ucL_Char )
{
           unsigned long temp = 0;
		   unsigned char i;

//           SAVE_FINGER[9]={0x01,0x00,0x06,0x06,0x01,0x00,0x0B,0x00,0x19};//将BUFFER1中的特征码存放到指定的位置

           FP_Save_Finger[5] = ucH_Char;
           FP_Save_Finger[6] = ucL_Char;
           
		   for(i=0;i<7;i++)   //计算校验和
		   	   temp = temp + FP_Save_Finger[i];
			    
		   FP_Save_Finger[7]=(temp & 0x00FF00) >> 8; //存放校验数据
		   FP_Save_Finger[8]= temp & 0x0000FF;
		   
           for(i=0;i<6;i++)    
    	       uartObjFP_Send_Byte(FP_Pack_Head[i]);        //发送包头

           for(i=0;i<9;i++)  
      		   uartObjFP_Send_Byte(FP_Save_Finger[i]);      //发送命令 将图像转换成 特征码 存放在 CHAR_buffer1
}

void FP_Cmd_Save_Finger(unsigned char n )	  //cyacyf修改：改为1个参数————9
{
           unsigned long temp = 0;
		   unsigned char i;

//           SAVE_FINGER[9]={0x01,0x00,0x06,0x06,0x01,0x00,0x0B,0x00,0x19};//将BUFFER1中的特征码存放到指定的位置

          // FP_Save_Finger[5] = ucH_Char;
           FP_Save_Finger[6] = n;
           
		   for(i=0;i<7;i++)   //计算校验和
		   	   temp = temp + FP_Save_Finger[i];
			    
		   FP_Save_Finger[7]=(temp & 0x00FF00) >> 8; //存放校验数据
		   FP_Save_Finger[8]= temp & 0x0000FF;
		   
           for(i=0;i<6;i++)    
    	       uartObjFP_Send_Byte(FP_Pack_Head[i]);        //发送包头

           for(i=0;i<9;i++)  
      		   uartObjFP_Send_Byte(FP_Save_Finger[i]);      //发送命令 将图像转换成 特征码 存放在 CHAR_buffer1
}

//将flash数据库中指定ID号的指纹模板读入到模板缓冲区BUFFER1
void FINGERPRINT_Cmd_Load_Model( unsigned char ucH_Char,unsigned char ucL_Char )
{
           unsigned long temp = 0;
		   unsigned char i;

//         FP_Load_Model[9]={0x01,0x00,0x06,0x07,0x01,0x00,0x0B,0x00,0x1A};//将flash数据库中指定ID号的指纹模板读入到模板缓冲区BUFFER1

           FP_Load_Model[5] = ucH_Char;
           FP_Load_Model[6] = ucL_Char;
           
		   for(i=0;i<7;i++)   //计算校验和
		   	   temp = temp + FP_Load_Model[i];
			    
		   FP_Load_Model[7]=(temp & 0x00FF00) >> 8; //存放校验数据
		   FP_Load_Model[8]= temp & 0x0000FF;
		   
           for(i=0;i<6;i++)    
    	       uartObjFP_Send_Byte(FP_Pack_Head[i]);        //发送包头

           for(i=0;i<9;i++)  
      		   uartObjFP_Send_Byte(FP_Load_Model[i]);      //发送命令 将指定ID号的指纹模板读入到模板缓冲区BUFFER1
}

unsigned char FP_Cmd_Load_Model( unsigned char ucH_Char,unsigned char ucL_Char)
{
	 FINGERPRINT_Cmd_Load_Model(ucH_Char,ucL_Char );
	 FINGERPRINT_Recevice_Data(12); //接收12个长度的反馈码
	 if(FINGERPRINT_RECEVICE_BUFFER[9]==0x0)//模板读出成功
	 return 0x01;
	 else if(FINGERPRINT_RECEVICE_BUFFER[9]==0x0C)	//模板读出有错或模板无效
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

/*指纹学习*///指纹添加新用户
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
			
				Mode_umAdd_timeOutcnt = umAddMode_TIMEOUT;  //超时检测值更新
				
				FINGERPRINT_Cmd_Img_To_Buffer1(); 	//将图像转换成特征码存放在Buffer1中
				FINGERPRINT_Recevice_Data(12);   	//接收12个长度的反馈码
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
					
					Mode_umAdd_timeOutcnt = umAddMode_TIMEOUT;  //超时检测值更新
				}
			}
		}break;
		
		case umStepAdd_B:{
		
			FP_imageCollection(3);
//			uartObjFP_Send_String(Pointer, 12);
			
			if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
			
				FINGERPRINT_Cmd_Img_To_Buffer2(); 	//将图像转换成特征码存放在Buffer2中
				FINGERPRINT_Recevice_Data(12);   	//接收12个长度的反馈码
//				uartObjFP_Send_String(Pointer, 12);
				
				if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
					
//					Mode_umAdd_timeOutcnt = umAddMode_TIMEOUT;  //超时检测值更新
				
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
				
				beeps(11);	//准完成，提示音
				
				if(FINGERPRINT_RECEVICE_BUFFER[9] == 0x02){
				
					umNew_AddStep = umStepAdd_C;
				}
			}
		}break;
		
		case umStepAdd_C:{					//方案一
		
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
		
//		case umStepAdd_D:{					//方案二
//		
//			FP_imageCollection(2);
//			uartObjFP_Send_String(Pointer, 12);
//			
//			if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
//				
//				FINGERPRINT_Cmd_Delete_Model((u16)ucL_user);	//flag=4时检测到有指纹按下则重新录入
//				FINGERPRINT_Recevice_Data(12);   				//接收12个长度的反馈码
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

//指纹检测程序
u8 FP_Detecting(void){	
	
//	static	u8 	*Pointer;	//回发缓存
	
	static	u8 	FP_ID = 0;
	
	u8 xdata 	log_info[60] 	= {0};
	
//	Pointer = &FINGERPRINT_RECEVICE_BUFFER[0];
	
	if(FPvalidation_flg)return FPID_NULL;	//已识别完成标志未置零，不予检测
			
	if(umStepDtt_A == um_dttStep){
		
		FP_imageCollection(1);
//		uartObjFP_Send_String(Pointer,12);
		
		if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
		
			um_dttStep = umStepDtt_B;
			
			FINGERPRINT_Cmd_Img_To_Buffer1(); //将图像转换成特征码存放在Buffer1中
			FINGERPRINT_Recevice_Data(12);   //接收12个长度的反馈码
//			uartObjFP_Send_String(Pointer, 12);	
			
			if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
			
				FINGERPRINT_Cmd_Search_Finger_0_100();
				FINGERPRINT_Recevice_Data(16);
//				uartObjFP_Send_String(Pointer, 16);
				
				if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
				
					um_dttStep = umStepDtt_Csuccess;
					
					FP_ID = FINGERPRINT_RECEVICE_BUFFER[11];
					
					/*提示音放置*///一次性
					beeps(0);	//提示音
					
				}else{
				
					um_dttStep = umStepDtt_Cfail;
					
					/*提示音放置*///一次性
				}
			}else{
			
				um_dttStep = umStepDtt_A;	//状态恢复
			}
		}
	}
	
	if(umStepDtt_Cfail == um_dttStep){
	
		FP_imageCollection(1);
//		uartObjFP_Send_String(Pointer,12);
		
		/*提示音放置*///循环
		beepTips_s(4, 8, 8);	//提示音
		
		if(FINGERPRINT_RECEVICE_BUFFER[9] == 0x02){
		
			um_dttStep = umStepDtt_A;
			FPvalidation_flg = 0;
			
			sprintf(log_info, "检测到非法指纹触摸！！！\n");
			LogDats(log_info, strlen(log_info));
		}
	}
			
	if(umStepDtt_Csuccess == um_dttStep){
		
		/*根据需要自定义修改逻辑，时要手指离开时响应还是立即响应，立即响应则需要手指释放判断逻辑*/
		
		static bit fp_releas_FLAG = 1;	//手指释放标志
	
		FP_imageCollection(1);
//		uartObjFP_Send_String(Pointer,12);
		
		if(fp_releas_FLAG){
			
			fp_releas_FLAG	 = 0;
			FPvalidation_flg = 1;
		}
		
		/*提示音放置*///循环
		
		if(FINGERPRINT_RECEVICE_BUFFER[9] == 0x02){
			
			um_dttStep 		 = umStepDtt_A;
			
			fp_releas_FLAG 	 = 1;
//			FPvalidation_flg = 1;
			
			sprintf(log_info, "检测到合法指纹触摸，对应指纹ID为： %d.\n", (int)FP_ID);
			LogDats(log_info, strlen(log_info));
			
			return FP_ID;
		}
	}
	
	return FPID_NULL;
}

//指纹模块在主动检测模式下，功能被禁止时所触发的程序
void FP_Detecting_Disable(void){

	FP_imageCollection(1);
	if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
	
		um_dttStep = umStepDtt_B;
		beeps(12);	//提示音
		
	}else{
	
		um_dttStep = umStepDtt_A;
	}
}

/*********************指纹模块驱动主线程***********************/
void thread_moudleFP(void){
	
	u8 data repeatTX_buff[45]		= {0};
	u8 data fpID_Tab[13]			= {0};
	
//	u8 code rptTX_Continue_Delay	= 30;	//连包发送数据间隔_30ms不能再短了
	
	switch(process_FPworkMode){
	
		case work_Mode_umAdd:{
			
			u8 code detect_Period = 100; //非阻塞主动监测周期
			static u8 detect_tCount = 0;
			
			delayMs(2);//必需延时
			if(detect_tCount < detect_Period)detect_tCount ++; //非阻塞周期检测
			else{
			
				FP_userModelNew_Add(0, FP_ID);
				
				if(umNew_AddStep_StandBy == umStepAdd_B){	//单次录入半完成
				
					if(umNew_AddStepReapt_cmpHalfEN){
					
						umNew_AddStepReapt_cmpHalfEN = 0;	//使能关闭，只发一次
						
						beeps(5);	//提示音
						
						//数据响应及回复
						repeatTX_buff[15]	= FP_ID;
						repeatTX_buff[12]	= 0x0A;		//数据1填充半完成状态 
						
						dtasTX_loadBasic_AUTO( repeatTX_buff,				/*发送前最后装载*///发送包基本信息填充
											   FRAME_TYPE_StoM_RCVsuccess,
											   FRAME_MtoSCMD_cmdFPLearn,		
											   0
											 );
						uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);	//半完成，向上位机响应状态
					}
				}
				
				if(umNew_AddStep == umStepAdd_D){	//单次录入全完成
					
					u8 fpStore_addr 	= EEPROM_ADDR_FP_STORE + 12 - FP_ID / 8;
					u8 val_Store 		= 0;
					
					EEPROM_read_n(fpStore_addr, &val_Store, 1);		//模板占位存储更新
					val_Store |= 1 << (FP_ID % 8);	
					coverEEPROM_write_n(fpStore_addr, &val_Store, 1);
					
					FP_modeCompleteTips = cmp_umAdd_Success;		//完成状况 Tips更新
					FPmodeCmpTips_cnt	= FPmodeCmpTips_Period;
					
					//数据响应及回复
					repeatTX_buff[15]	= FP_ID;
					repeatTX_buff[12]	= 0x0B;
					EEPROM_read_n(EEPROM_ADDR_FP_STORE, &fpID_Tab[0], 13);	
					memcpy(&repeatTX_buff[16], &fpID_Tab[0], 13);
					
					dtasTX_loadBasic_AUTO( repeatTX_buff,				/*发送前最后装载*///发送包基本信息填充
										   FRAME_TYPE_StoM_RCVsuccess,
										   FRAME_StoMRPY_rpyLearnSuccess,	//学习成功
										   0	
										 );
					uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);	//全完成，向上位机响应状态
					
					FP_ID 				= FPID_NULL;
					process_FPworkMode 	= work_Mode_NULL;
					umNew_AddStep 		= umStepAdd_A;

					LogString("模板学习成功 !!!\n");
				}
				
				if(!Mode_umAdd_timeOutcnt){	//超时退出
				
					led_darkAll();
					
					beepTips_s(5, 20, 8);	//提示音
					
					FP_modeCompleteTips = cmp_umAdd_tOut;		//完成状况 Tips更新
					FPmodeCmpTips_cnt	= FPmodeCmpTips_Period;
					
					//数据响应及回复
					repeatTX_buff[15]	= FP_ID;
					repeatTX_buff[12]	= 0x0F;
					
					dtasTX_loadBasic_AUTO( repeatTX_buff,			/*发送前最后装载*///发送包基本信息填充
										   FRAME_TYPE_StoM_RCVsuccess,
										   FRAME_StoMRPY_rpyLearnFail,	//学习失败
										   0
										 );
					uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);	//未完成，向上位机响应状态
					
					FP_ID 				= FPID_NULL;
					process_FPworkMode 	= work_Mode_NULL;
					umNew_AddStep 		= umStepAdd_A;
					
					LogString("模板学习失败，超时退出 !!!\n");
				}
			}
			
		}break;
		
		case work_Mode_umDel:{
		
			FP_Cmd_Delete_Model(FP_ID);
			FINGERPRINT_Recevice_Data(12);   				//接收12个长度的反馈码
			
			if(FINGERPRINT_RECEVICE_BUFFER[9] == 0){
			
				u8 fpStore_addr 	= EEPROM_ADDR_FP_STORE + 12 - FP_ID / 8;
				u8 val_Store 		= 0;
				
				EEPROM_read_n(fpStore_addr, &val_Store, 1);		//模板占位删除
				val_Store &= ~(1 << (FP_ID % 8));	
				coverEEPROM_write_n(fpStore_addr, &val_Store, 1);
				
				//数据响应及回复
				repeatTX_buff[15]	= FP_ID;
				repeatTX_buff[12]	= 0x0A;		//数据1填充删除完成
				
				dtasTX_loadBasic_AUTO( repeatTX_buff,				/*发送前最后装载*///发送包基本信息填充
									   FRAME_TYPE_StoM_RCVsuccess,
									   FRAME_MtoSCMD_cmdFPDelete,	//删除成功
									   0
									 );
				uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);	//半完成，向上位机响应状态
//				delayMs(rptTX_Continue_Delay);
//				uartObjWIFI_Send_String(repeatTX_buff, repeatTX_Len);	//多发一次，确保对方接收
				
				FP_ID = FPID_NULL;
				process_FPworkMode 	= work_Mode_NULL;
				fpModeTrig_umDetect();	//指纹模块线程跳回主动检测模式

				LogString("模板删除成功 !!!\n");				
			}
		}break;
			
		case work_Mode_umDetect:{
			
			u8 code detect_Period = 50; //非阻塞主动监测周期
			static u8 detect_tCount = 0;
			
			delayMs(2);//必需延时
			if(detect_tCount < detect_Period)detect_tCount ++; //非阻塞周期检测
			else{
			
				detect_tCount = 0;
				
				if(FP_umDetect_enFLAG){	//是否开启主动检测使能
					
					u8 detect_Result = FP_Detecting();	//继电器响应已在继电器驱动文件处做了耦合处理 
					
					if(detect_Result != FPID_NULL){		
						
						//数据响应及回复
						repeatTX_buff[15] = detect_Result;	//指纹号填充
						(status_Relay)?(repeatTX_buff[11] = 0x00):(repeatTX_buff[11] = 0x01);//开关状态填充（反转预测）
						(process_RelayWorkMode == work_Mode_flip)?(repeatTX_buff[14] = 0x01):(repeatTX_buff[14] = 0x02);//开关工作模式填充 翻转模式：0x01 延时模式：0x02
						dtasTX_loadBasic_CUST( DATATRANS_objFLAG_MOBILE,	/*发送前最后装载*///发送包基本信息填充
											   repeatTX_buff,			
											   FRAME_TYPE_StoM_RCVsuccess,
											   FRAME_MtoSCMD_cmdControl,
											   0
											 );
						uartObjWIFI_Send_String(repeatTX_buff, dataTransLength_objMOBILE);	//手动控制开关，主动上传，不能连发，否则记录多次	--强制33字节
					}
					
				}else{
				
					FP_Detecting_Disable();	//主动检测禁用时，空测
				}	
			}
			
		}break;
		
		default:{
		
			process_FPworkMode = work_Mode_NULL;
			
		}break;
	}
	
	if(FP_modeCompleteTips != cmp_NULL){	//Tips处理
	
		switch(FP_modeCompleteTips){
		
			case cmp_umAdd_Success:{	//学习成功Tips
			
				if(!FPmodeCmpTips_cnt){
				
					FP_modeCompleteTips = cmp_NULL;
					led_darkAll();
					fpModeTrig_umDetect();	//指纹模块线程跳回主动检测模式
					
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
			
			case cmp_umAdd_tOut:{	//学习失败，超时退出Tips
			
				if(!FPmodeCmpTips_cnt){
				
					FP_modeCompleteTips = cmp_NULL;
					led_darkAll();
					fpModeTrig_umDetect();	//指纹模块线程跳回主动检测模式
					
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

/********************指纹模块线程模式触发************************/
void fpModeTrig_umAdd(void){

	Mode_umAdd_timeOutcnt 		 = umAddMode_TIMEOUT;
	process_FPworkMode			 = work_Mode_umAdd;		//指纹模块线程模式切换，启动学习
	umNew_AddStep 				 = umStepAdd_A;
	umNew_AddStep_StandBy 		 = umStepAdd_A;
	
	umNew_AddStepReapt_cmpHalfEN = 1;				//半完成发数据包使能，防止多发，进行标志位限次
	
	FP_modeCompleteTips 		 = cmp_NULL;
}

void fpModeTrig_umDel(void){

	process_FPworkMode		= work_Mode_umDel;		//指纹模块线程模式切换，模板删除
}

void fpModeTrig_umDetect(void){

	process_FPworkMode		= work_Mode_umDetect;	//指纹模块线程模式切换，主动检测
	FPvalidation_flg		= 0;
}

/********************打印指纹占位模板****************************/
void logOut_fpID_place(void){

	unsigned char xdata log_info[100] 		= {0};
	unsigned char xdata log_info_fpID[60] 	= {0};
	unsigned char idata fpID_Tab[13]		= {0};
	
	unsigned char idata loop				= 0; 
	
	EEPROM_read_n(EEPROM_ADDR_FP_STORE, &fpID_Tab[0], 13);
	
	for(loop = 0; loop < 13; loop ++)
		sprintf(&log_info_fpID[loop * 3], "%02X ", (int)fpID_Tab[loop]);
	
	sprintf(log_info, "当前指纹模板占位为：%s \n", log_info_fpID);
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


