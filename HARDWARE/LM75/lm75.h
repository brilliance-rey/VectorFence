#ifndef __LM75_H
#define __LM75_H
#include "myiic.h"   

#define LM75_SLAVE_ADDR 0x90
#define LM75_CORRECTION (-5)

extern char crtLM75TempStr[5];	//100.5��-99.5
u8 LM75_SetPointer(u8 pointer);  //�����¶�оƬLM75��Pointer
u16 LM75_ReadTemp(void);  //��ȡLM75�¶�
void LM75_ReadTempStr(char *temp);//��ȡLM75�¶��ַ���

#endif
















