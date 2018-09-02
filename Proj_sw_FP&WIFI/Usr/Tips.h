#ifndef __TIPS_H_
#define __TIPS_H_

#include "STC15Fxxxx.H"
#include "typeDf.h"

sbit LED_R = P1^4;
sbit LED_G = P2^1;
sbit LED_B = P2^3;

sbit PIN_BEEP = P2^6;

#define led_statusON	0
#define led_statusOFF	1

typedef enum{

	red = 0,
	green,
	blue,
}led_color;

typedef enum{

	led_on	= 0,
	led_off,
	led_flip,
}led_actMethod;

void ledTips_Init(void);
void Beep_Init(void);
void led_darkAll(void);
void led_Act(led_color color,led_actMethod method);
void Beep_Tips(u8 method, u8 loud, u8 repeat, u8 delay_A, u8 delay_B);
void beepTips_s(u8 tones, u16 time, u8 vol);
void beepTips(u8 tones, u16 time, u8 vol);
void beeps(u8 num);

void Beep_Test(void);

#endif