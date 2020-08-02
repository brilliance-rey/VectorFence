#ifndef __LED_H
#define __LED_H
#include "sys.h"


/**
LED端口定义:
PB0（PIN_35）	O	LED_FANG_CHAI		防拆告警灯	机箱盖被打开时亮
PB1（PIN_36）	O	LED_DEVIATE			偏移告警灯	检测到长时间内电压偏离中心电压时亮
PA4（PIN_29）	O	LED_INVD			入侵告警灯	检测到短时间内电压有大的波动时亮
PA5（PIN_30）	O	LED_BU_FANG			布防指示灯	高压回路开始运行时亮
PA3（PIN_18）	O	LED_RUN				运行指示灯	以0.5S的间隔闪烁
*/
#define LED_FANG_CHAI   PBout(0)  	//PB0（PIN_35）	O	LED_FANG_CHAI		防拆告警灯  机箱盖被打开时亮
#define LED_OPEN 	    PBout(1)	//PB1（PIN_36）	O	LED_OPEN				偏移告警灯	检测 高压回路偏移时亮
#define LED_INVD 		PAout(4)	//PA4（PIN_29）	O	LED_INVD				短路告警灯	检测到高压回路有短路时亮
#define LED_BU_FANG 	PAout(5)	//PA5（PIN_30）	O	LED_BU_FANG				布防指示灯	高压回路开始运行时亮
#define LED_RUN 		PAout(3)	//PA3（PIN_26）	O	LED_RUN					运行指示灯	以0.5S的间隔闪烁


void LED_Init(void);//初始化
#endif
