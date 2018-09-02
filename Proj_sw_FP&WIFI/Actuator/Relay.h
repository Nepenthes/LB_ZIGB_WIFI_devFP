#ifndef __RELAY_H_
#define __RELAY_H_

#include "STC15Fxxxx.H"

sbit CON_RELAY = P3^3;

typedef bit bool;

#define false	0
#define true	1	
	
#define actRelay_ON		1
#define actRelay_OFF	0

typedef enum{

	swStatus_on = 0,
	swStatus_off,
	swStatus_null,
}switch_Status;

typedef enum{

	relay_flip = 1,
	relay_on,
	relay_off,
}rly_methodType;

typedef struct{

	 u8  tim_EN;
	u16	 tim_Period;
}rly_timFun;

typedef enum{

	work_Mode_flip = 1,
	work_Mode_conuter,
}threadRelay_Mode;

typedef enum{

	statusSave_enable = 1,
	statusSave_disable,
}status_ifSave;

void relay_Init(void);
void relay_Act(rly_methodType act_Method, bool timer_EN);
void thread_Relay(void);

#endif