#include "led.h" 
//#include "delay.h"
//
//#define LAN8720_RST 		   	PEout(2) 			//LAN8720��λ����	


//��ʼ��PF9��PF10Ϊ�����.��ʹ���������ڵ�ʱ��		    
//LED IO��ʼ��
void LED_Init(void)
{    	 
  GPIO_InitTypeDef  GPIOA_InitStructure;
  GPIO_InitTypeDef  GPIOB_InitStructure;
//  GPIO_InitTypeDef  GPIOC_InitStructure;
 

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//ʹ��GPIOAʱ��
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//ʹ��GPIOBʱ��
//  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//ʹ��GPIOCʱ��

 
 //GPIOA��ʼ������
  GPIOA_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
  GPIOA_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
  GPIOA_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
  GPIOA_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIOA_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOA, &GPIOA_InitStructure);//��ʼ��GPIOA
  
  GPIO_ResetBits(GPIOA, GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5);//���õͣ�����
  
  //GPIOB��ʼ������
  GPIOB_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
  GPIOB_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
  GPIOB_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
  GPIOB_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIOB_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOB, &GPIOB_InitStructure);
	
  GPIO_ResetBits(GPIOB,GPIO_Pin_0 | GPIO_Pin_1);//���õͣ�����
  
}


