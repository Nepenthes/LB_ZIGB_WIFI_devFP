#ifndef __TIMERACT_H_
#define __TIMERACT_H_

#include "STC15Fxxxx.H"

#define timCount_ENABLE		0x01
#define timCount_DISABLE	0x00

typedef enum{

	timTabObj_swNormal = 0,
	timTabObj_permission,
}timingTabObj;

typedef struct{
	
	u8 tim_EN:1;
	u8 Week_Num;
	u8 time_Hour;
	u8 time_Minute;
}time_Calibration;

typedef struct{

	u8 		  Week_Num	:7;	//��ֵռλ
	u8		  if_Timing	:1;	//�Ƿ񿪶�ʱ
	u8		  Status_Act:3;	//��ʱʱ�̿�����Ҫ��Ӧ��״̬
	u8		  Hour		:5;	//ʱ�̡���Сʱ
	u8		  Minute;		//ʱ�̡�����
}timing_Dats;

void thread_Timing(void);

#endif