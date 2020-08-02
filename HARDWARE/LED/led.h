#ifndef __LED_H
#define __LED_H
#include "sys.h"


/**
LED�˿ڶ���:
PB0��PIN_35��	O	LED_FANG_CHAI		����澯��	����Ǳ���ʱ��
PB1��PIN_36��	O	LED_DEVIATE			ƫ�Ƹ澯��	��⵽��ʱ���ڵ�ѹƫ�����ĵ�ѹʱ��
PA4��PIN_29��	O	LED_INVD			���ָ澯��	��⵽��ʱ���ڵ�ѹ�д�Ĳ���ʱ��
PA5��PIN_30��	O	LED_BU_FANG			����ָʾ��	��ѹ��·��ʼ����ʱ��
PA3��PIN_18��	O	LED_RUN				����ָʾ��	��0.5S�ļ����˸
*/
#define LED_FANG_CHAI   PBout(0)  	//PB0��PIN_35��	O	LED_FANG_CHAI		����澯��  ����Ǳ���ʱ��
#define LED_OPEN 	    PBout(1)	//PB1��PIN_36��	O	LED_OPEN				ƫ�Ƹ澯��	��� ��ѹ��·ƫ��ʱ��
#define LED_INVD 		PAout(4)	//PA4��PIN_29��	O	LED_INVD				��·�澯��	��⵽��ѹ��·�ж�·ʱ��
#define LED_BU_FANG 	PAout(5)	//PA5��PIN_30��	O	LED_BU_FANG				����ָʾ��	��ѹ��·��ʼ����ʱ��
#define LED_RUN 		PAout(3)	//PA3��PIN_26��	O	LED_RUN					����ָʾ��	��0.5S�ļ����˸


void LED_Init(void);//��ʼ��
#endif
