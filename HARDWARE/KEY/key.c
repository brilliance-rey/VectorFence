#include "key.h"
#include "delay.h" 
//////////////////////////////////////////////////////////////////////////////////	 

//������ʼ������
void KEY_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOD, ENABLE);
 
//PD13��PIN_60��	I	KEY_UP		�ϼ�
//PD12��PIN_59��	I	KEY_DOWN	�¼�
//PD15��PIN_62��	I	KEY_LEFT	���
//PD14��PIN_61��	I	KEY_RIGHT	�Ҽ�
//PC7��PIN_64��		I	KEY_ENTER	ȷ�ϼ�
//PC6��PIN_63��		I	KEY_ESC		�˳���

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//��ͨ����ģʽ
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOD, &GPIO_InitStructure);//��ʼ��GPIOD12,13,14,15
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_Init(GPIOC, &GPIO_InitStructure);//��ʼ��GPIOC6,7
} 

//����������
//���ذ���ֵ
//mode:0,��֧��������;1,֧��������;

//ע��˺�������Ӧ���ȼ�,KEY_ESC>KEY_ENTER>KEY_UP>KEY_DOWN>KEY_LEFT>KEY_RIGHT
u8 KEY_Scan(u8 mode)
{	 
	static u8 key_up=1;//�������ɿ���־
	if(mode)key_up=1;  //֧������		  
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
 	return KEY_NO_PRESS;// �ް�������
}




















