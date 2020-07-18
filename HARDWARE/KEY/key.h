#ifndef __KEY_H
#define __KEY_H
#include "sys.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//按键输入驱动代码	   
///////////////////////////////////////////////////////////////////////////////// 	 

//PD13（PIN_60）	I	KEY_UP		上键
//PD12（PIN_59）	I	KEY_DOWN	下键
//PD15（PIN_62）	I	KEY_LEFT	左键
//PD14（PIN_61）	I	KEY_RIGHT	右键
//PC7（PIN_64）		I	KEY_ENTER	确认键
//PC6（PIN_63）		I	KEY_ESC		退出键

#define KEY_ESC 	PCin(6)
#define KEY_ENTER	PCin(7)
#define KEY_UP	    PDin(13)
#define KEY_DOWN    PDin(12)
#define KEY_LEFT	PDin(15)
#define KEY_RIGHT   PDin(14)

#define KEY_ESC_PRESS 		1
#define KEY_ENTER_PRESS		2
#define KEY_UP_PRESS	    3
#define KEY_DOWN_PRESS    	4
#define KEY_LEFT_PRESS		5
#define KEY_RIGHT_PRESS   	6
#define KEY_NO_PRESS   		0

void KEY_Init(void);	//IO初始化
u8 KEY_Scan(u8);  		//按键扫描函数				    
#endif
