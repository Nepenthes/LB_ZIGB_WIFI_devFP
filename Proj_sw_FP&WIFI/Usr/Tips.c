#include "Tips.h"

#include "wifi_LPT220.h"

#include "GPIO.h"
#include "delay.h"

//*******************WIFI驱动变量引用区***************************/
extern status_Mode		   SYS_status_Mode;	//系统状态

#define spect_size 15
const u8 spect_len[spect_size] 	= {2,2,2,4,4,4,7,7,7,6,5,3,3};
const u8 spect[spect_size][8]	= {

	{3,1},//0
	{7,2},//1
	{3,3},//2
	{1,2,4,5},//3
	{5,4,3,2},//4
	{1,2,6,7},//5
	{5,5,6,4,3,1,5},//6
	{7,7,5,6,1,2,4},//7
	{6,6,2,3,1,4,7},//8
	{1,2,5,6,3,4},//9
	{1,2,3,4,6},//10
	{5,2,2},//11
	{7,4,4},//12
};

void ledTips_Init(void){

	GPIO_InitTypeDef GPIO_Tips;
	
	GPIO_Tips.Mode 	= GPIO_OUT_OD;
	GPIO_Tips.Pin	= GPIO_Pin_1 | GPIO_Pin_3;
	GPIO_Inilize(GPIO_P2, &GPIO_Tips);
	
	GPIO_Tips.Pin	= GPIO_Pin_4;
	GPIO_Inilize(GPIO_P1, &GPIO_Tips);

	led_darkAll();	//Tips_LED全关
	led_Act(red, led_on);
	
	SYS_status_Mode = statusMode_NULL;	//系统状态初始化
}	

void Beep_Init(void){

	P2M1 &= 0xBF;
	P2M0 &= 0xBF;
	
	beepTips_s(5, 10, 6);
}

void led_darkAll(void){

	LED_R = LED_G = LED_B = led_statusOFF;
}

void beeps_delay(u16 cnt){

	while(-- cnt){
		
		NOP20();
	}
}

void led_Act(led_color color,led_actMethod method){

	static bit ledStatus_R = led_statusOFF,
			   ledStatus_G = led_statusOFF,
			   ledStatus_B = led_statusOFF;
	
	switch(color){
	
		case red:{
			
			LED_G = LED_B = led_statusOFF;
		
			switch(method){
			
				case led_on:	ledStatus_R = led_statusON;
					
				case led_off:	ledStatus_R = led_statusOFF;
					
				case led_flip:	ledStatus_R = !ledStatus_R;
				
				default:break;
			}LED_R = ledStatus_R;
		}break;
			
		case green:{
		
			LED_R = LED_B = led_statusOFF;
			
			switch(method){
			
				case led_on:	ledStatus_G = led_statusON;
					
				case led_off:	ledStatus_G = led_statusOFF;
					
				case led_flip:	ledStatus_G = !ledStatus_G;
				
				default:break;
			}LED_G = ledStatus_G;
		}break;
			
		case blue:{
			
			LED_R = LED_G = led_statusOFF;
		
			switch(method){
			
				case led_on:	ledStatus_B = led_statusON;
					
				case led_off:	ledStatus_B = led_statusOFF;
					
				case led_flip:	ledStatus_B = !ledStatus_B;
				
				default:break;
			}LED_B = ledStatus_B;
		}break;
			
		default:break;
	}
}

void Beep_Tips(u8 method, u8 loud, u8 repeat, u8 delay_A, u8 delay_B){	//音调， 音量， 次数， 响时长， 静时长

	u16 loop 	= 0,
	    loopA	= 0,
	    loopB	= 0;
	
	for(loop = repeat; loop != 0; loop --){

		for(loopB = 10 * delay_A; loopB != 0; loopB --){
		
			for(loopA = (u16)method * 100; loopA != 0; loopA --){
			
				_nop_();
				(loopA >= loud * 10)?(PIN_BEEP = 1):(PIN_BEEP = 0);
			}
		}
		
		if(delay_B)delayMs(delay_B * 10);
	}
	
	PIN_BEEP = 1;
}

void beepTips(u8 tones, u16 time, u8 vol){	//音调， 时长， 音量

	const u8 vol_fa = 10;
	u16 tones_base  = tones * 50 + 100;
	u16 cycle		= time * 1000 / tones_base;
	u16 loop;
	
	for(loop = 0; loop < cycle; loop ++){
	
		PIN_BEEP = 0;
		beeps_delay(tones_base / vol_fa * vol);
		PIN_BEEP = 1;
		beeps_delay(tones_base / vol_fa * (vol_fa - vol));
	}
}

void beepTips_s(u8 tones, u16 time, u8 vol){	//音调， 时长*100， 音量

	u8 loop = time;
	
	for(loop = time; loop != 0; loop --)beepTips(tones, 100, vol);
}

void beeps(u8 num){

	u8 loop;
	
	for(loop = 0; loop < spect_len[num]; loop ++)
		beepTips_s(spect[num][loop], 3, 8);
}

void Beep_Test(void){

	beeps(6);
	beeps(7);
	beeps(8);
}
