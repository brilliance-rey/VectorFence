#include "sys.h"  
#include "24cxx.h"
#include "alarm.h"
#include "log.h"

//********************************************************************************
//修改说明
//无
//////////////////////////////////////////////////////////////////////////////////  
u8 config_code = 0;

//THUMB指令不支持汇编内联
//采用如下方法实现执行汇编指令WFI  
__asm void WFI_SET(void)
{
	WFI;		  
}
//关闭所有中断(但是不包括fault和NMI中断)
__asm void INTX_DISABLE(void)
{
	CPSID   I
	BX      LR	  
}
//开启所有中断
__asm void INTX_ENABLE(void)
{
	CPSIE   I
	BX      LR  
}
//设置栈顶地址
//addr:栈顶地址
__asm void MSR_MSP(u32 addr) 
{
	MSR MSP, r0 			//set Main Stack value
	BX r14
}


//系统软复位   
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
