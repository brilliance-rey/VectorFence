#ifndef __KEY_H
#define __KEY_H
#include "sys.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F407������
//����������������	   
///////////////////////////////////////////////////////////////////////////////// 	 

//PD13��PIN_60��	I	KEY_UP		�ϼ�
//PD12��PIN_59��	I	KEY_DOWN	�¼�
//PD15��PIN_62��	I	KEY_LEFT	���
//PD14��PIN_61��	I	KEY_RIGHT	�Ҽ�
//PC7��PIN_64��		I	KEY_ENTER	ȷ�ϼ�
//PC6��PIN_63��		I	KEY_ESC		�˳���

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

void KEY_Init(void);	//IO��ʼ��
u8 KEY_Scan(u8);  		//����ɨ�躯��				    
#endif
