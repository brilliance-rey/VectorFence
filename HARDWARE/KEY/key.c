#include "key.h"
#include "delay.h" 
//////////////////////////////////////////////////////////////////////////////////	 

//按键初始化函数
void KEY_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOD, ENABLE);
 
//PD13（PIN_60）	I	KEY_UP		上键
//PD12（PIN_59）	I	KEY_DOWN	下键
//PD15（PIN_62）	I	KEY_LEFT	左键
//PD14（PIN_61）	I	KEY_RIGHT	右键
//PC7（PIN_64）		I	KEY_ENTER	确认键
//PC6（PIN_63）		I	KEY_ESC		退出键

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//普通输入模式
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOD, &GPIO_InitStructure);//初始化GPIOD12,13,14,15
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化GPIOC6,7
} 

//按键处理函数
//返回按键值
//mode:0,不支持连续按;1,支持连续按;

//注意此函数有响应优先级,KEY_ESC>KEY_ENTER>KEY_UP>KEY_DOWN>KEY_LEFT>KEY_RIGHT
u8 KEY_Scan(u8 mode)
{	 
	static u8 key_up=1;//按键按松开标志
	if(mode)key_up=1;  //支持连按		  
	if(key_up&&(KEY_ESC==0 || KEY_ENTER==0 || KEY_UP==0 || KEY_DOWN==0 || KEY_LEFT==0 || KEY_RIGHT==0))
	{
		delay_ms(10);
		key_up=0;
		if(KEY_ESC==0)			return KEY_ESC_PRESS;
		else if(KEY_ENTER==0)	return KEY_ENTER_PRESS;
		else if(KEY_UP==0)		return KEY_UP_PRESS;
		else if(KEY_DOWN==0)	return KEY_DOWN_PRESS;
		else if(KEY_LEFT==0)	return KEY_LEFT_PRESS;
		else if(KEY_RIGHT==0)	return KEY_RIGHT_PRESS;
			
	}else if(KEY_ESC==1 && KEY_ENTER==1 && KEY_UP==1 && KEY_DOWN==1 && KEY_LEFT==1 && KEY_RIGHT==1){
		key_up=1;
	}
 	return KEY_NO_PRESS;// 无按键按下
}




















