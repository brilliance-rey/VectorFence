#include "sys.h"  
#include "24cxx.h"
#include "alarm.h"
#include "log.h"

//********************************************************************************
//�޸�˵��
//��
//////////////////////////////////////////////////////////////////////////////////  
u8 config_code = 0;

//THUMBָ�֧�ֻ������
//�������·���ʵ��ִ�л��ָ��WFI  
__asm void WFI_SET(void)
{
	WFI;		  
}
//�ر������ж�(���ǲ�����fault��NMI�ж�)
__asm void INTX_DISABLE(void)
{
	CPSID   I
	BX      LR	  
}
//���������ж�
__asm void INTX_ENABLE(void)
{
	CPSIE   I
	BX      LR  
}
//����ջ����ַ
//addr:ջ����ַ
__asm void MSR_MSP(u32 addr) 
{
	MSR MSP, r0 			//set Main Stack value
	BX r14
}


//ϵͳ��λ   
void Sys_Soft_Reset(void)
{   
	SCB->AIRCR =0X05FA0000|(u32)0x04;	  
} 		

void SystemReset(void){
	NVIC_SystemReset();
}

void readConfigCode(void){
	ReadConfigCodeToEEProm();
	if(config_code == 0xFF){
		config_code = 0x01;  //default
		WriteConfigCodeToEEProm();
	}
}

void setConfigCode(u8 value){
	config_code = value;
	WriteConfigCodeToEEProm();

	W4_W6 = (config_code >> 1) & 0x01;
}

void systemDefault(void){
	EraseEEPormParas();
//	ClearAllLog();
	SystemReset();
}
