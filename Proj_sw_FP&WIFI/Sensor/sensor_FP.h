#ifndef __SENSORFP_H_
#define __SENSORFP_H_

#include "STC15Fxxxx.H"

#define FPID_NULL		255

#define umAddMode_TIMEOUT	10000UL		//指纹模块学习超时设置

typedef enum{

	umStepAdd_A = 1,
	umStepAdd_B,
	umStepAdd_C,
	umStepAdd_D,
	umStepAdd_Fault,
	umStepAdd_Cmp,
}stt_stepAdd;

typedef enum{

	umStepDtt_A = 1,
	umStepDtt_B,
	umStepDtt_Csuccess,
	umStepDtt_Cfail,
}stt_stepDtt;

typedef enum{

	work_Mode_umDetect = 1,
	work_Mode_umAdd,
	work_Mode_umDel,
	work_Mode_NULL,	//用作切换其他状态指示
}threadFP_Mode;

typedef enum{

	cmp_umAdd_tOut = 1,
	cmp_umAdd_Success,
	cmp_NULL,
}tipsFP_completeType;

void FINGERPRINT_Cmd_Get_Img(void); //FINGERPRINT_获得指纹图像命令————1
void FINGERPRINT_Cmd_Img_To_Buffer1(void);//讲图像转换成特征码存放在Buffer1中————2
void FINGERPRINT_Cmd_Img_To_Buffer2(void);//将图像转换成特征码存放在Buffer2中————3
void FINGERPRINT_Cmd_Reg_Model(void);//将BUFFER1 跟 BUFFER2 中的特征码合并成指纹模版————4

void FINGERPRINT_Cmd_Search_Finger(void);//搜索全部用户999枚指纹	————5
void FINGERPRINT_Cmd_Search_Finger_Admin(void);//搜索0-9枚指纹————6

void FINGERPRINT_Cmd_Delete_All_Model(void);//删除指纹模块里的所有指纹模版————7
void FINGERPRINT_Cmd_Delete_Model(unsigned int uiID_temp);//删除指纹模块里的指定指纹模版——参数为双字节————7
void FP_Cmd_Delete_Model(unsigned char n);//删除指纹模板-参数为单字节————7

void FINGERPRINT_Cmd_Get_Templete_Num(void);//获得指纹模板数量————8

void FINGERPRINT_Recevice_Data(unsigned char ucLength);//接收反馈数据缓冲

void FINGERPRINT_Cmd_Save_Finger( unsigned char ucH_Char,unsigned char ucL_Char);//将BUFFER1中的特征码存放到指定的位置  储存模板：将特征文件存放到flash指纹库————9
void FP_Cmd_Save_Finger(unsigned char n);	  //cyacyf修改：改为1个参数————9

void FINGERPRINT_Cmd_Load_Model( unsigned char ucH_Char,unsigned char ucL_Char );
unsigned char FP_Cmd_Load_Model( unsigned char ucH_Char,unsigned char ucL_Char );

bit fpID_allClear(void);

void FP_userModelNew_Add(unsigned char ucH_user,unsigned char ucL_user);
u8 	 FP_Detecting(void);
void thread_moudleFP(void);

void fpModeTrig_umAdd(void);
void fpModeTrig_umDel(void);
void fpModeTrig_umDetect(void);

void logOut_fpID_place(void);

#endif