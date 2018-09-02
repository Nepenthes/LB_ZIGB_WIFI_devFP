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

	u8 		  Week_Num	:7;	//周值占位
	u8		  if_Timing	:1;	//是否开定时
	u8		  Status_Act:3;	//定时时刻开关需要响应的状态
	u8		  Hour		:5;	//时刻——小时
	u8		  Minute;		//时刻——分
}timing_Dats;

void thread_Timing(void);

#endif