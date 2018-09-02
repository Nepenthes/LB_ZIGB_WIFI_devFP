#ifndef __SENSORFP_H_
#define __SENSORFP_H_

#include "STC15Fxxxx.H"

#define FPID_NULL		255

#define umAddMode_TIMEOUT	10000UL		//ָ��ģ��ѧϰ��ʱ����

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
	work_Mode_NULL,	//�����л�����״ָ̬ʾ
}threadFP_Mode;

typedef enum{

	cmp_umAdd_tOut = 1,
	cmp_umAdd_Success,
	cmp_NULL,
}tipsFP_completeType;

void FINGERPRINT_Cmd_Get_Img(void); //FINGERPRINT_���ָ��ͼ�����������1
void FINGERPRINT_Cmd_Img_To_Buffer1(void);//��ͼ��ת��������������Buffer1�С�������2
void FINGERPRINT_Cmd_Img_To_Buffer2(void);//��ͼ��ת��������������Buffer2�С�������3
void FINGERPRINT_Cmd_Reg_Model(void);//��BUFFER1 �� BUFFER2 �е�������ϲ���ָ��ģ�桪������4

void FINGERPRINT_Cmd_Search_Finger(void);//����ȫ���û�999öָ��	��������5
void FINGERPRINT_Cmd_Search_Finger_Admin(void);//����0-9öָ�ơ�������6

void FINGERPRINT_Cmd_Delete_All_Model(void);//ɾ��ָ��ģ���������ָ��ģ�桪������7
void FINGERPRINT_Cmd_Delete_Model(unsigned int uiID_temp);//ɾ��ָ��ģ�����ָ��ָ��ģ�桪������Ϊ˫�ֽڡ�������7
void FP_Cmd_Delete_Model(unsigned char n);//ɾ��ָ��ģ��-����Ϊ���ֽڡ�������7

void FINGERPRINT_Cmd_Get_Templete_Num(void);//���ָ��ģ��������������8

void FINGERPRINT_Recevice_Data(unsigned char ucLength);//���շ������ݻ���

void FINGERPRINT_Cmd_Save_Finger( unsigned char ucH_Char,unsigned char ucL_Char);//��BUFFER1�е��������ŵ�ָ����λ��  ����ģ�壺�������ļ���ŵ�flashָ�ƿ⡪������9
void FP_Cmd_Save_Finger(unsigned char n);	  //cyacyf�޸ģ���Ϊ1��������������9

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