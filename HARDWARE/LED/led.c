#include "led.h" 
//#include "delay.h"
//
//#define LAN8720_RST 		   	PEout(2) 			//LAN8720复位引脚	


//初始化PF9和PF10为输出口.并使能这两个口的时钟		    
//LED IO初始化
void LED_Init(void)
{    	 
  GPIO_InitTypeDef  GPIOA_InitStructure;
  GPIO_InitTypeDef  GPIOB_InitStructure;
//  GPIO_InitTypeDef  GPIOC_InitStructure;
 

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//使能GPIOB时钟
//  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//使能GPIOC时钟

 
 //GPIOA初始化设置
  GPIOA_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
  GPIOA_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIOA_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIOA_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIOA_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOA, &GPIOA_InitStructure);//初始化GPIOA
  
  GPIO_ResetBits(GPIOA, GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5);//设置低，灯灭
  
  //GPIOB初始化设置
  GPIOB_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
  GPIOB_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIOB_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIOB_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIOB_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOB, &GPIOB_InitStructure);
	
  GPIO_ResetBits(GPIOB,GPIO_Pin_0 | GPIO_Pin_1);//设置低，灯灭
  
}


