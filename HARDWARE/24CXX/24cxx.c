#include "24cxx.h"
#include "lwip_comm.h"
#include "sys.h"
#include "delay.h"
#include "alarm.h"
#include "adc.h"


//初始化IIC接口
void AT24CXX_Init(void)
{
//	IIC_Init();//IIC初始化
}
//在AT24CXX指定地址读出一个数据
//ReadAddr:开始读数的地址  
//返回值  :读到的数据
u8 AT24CXX_ReadOneByte(u16 ReadAddr)
{				  
	u8 temp=0;

	u8 err = 0;
	OSMutexPend(iicMutexSem, (INT32U)IIC_MUTEX_TIMEOUT, &err);

    IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(AT24CXX_SLAVER_ADDR);	   //发送写命令
		IIC_Wait_Ack();
		IIC_Send_Byte(ReadAddr>>8);//发送高地址	    
	}else{
		IIC_Send_Byte(AT24CXX_SLAVER_ADDR+((ReadAddr/256)<<1));   //发送器件地址0XA0,写数据 	
	}
	IIC_Wait_Ack(); 
  
    IIC_Send_Byte(ReadAddr%256);   //发送低地址
	IIC_Wait_Ack();
	IIC_Start();
	IIC_Send_Byte(AT24CXX_SLAVER_ADDR | 0x01);           //进入接收模式			   
	IIC_Wait_Ack();
    temp=IIC_Read_Byte(0);
    IIC_Stop();//产生一个停止条件

    OSMutexPost(iicMutexSem);

	return temp;
}

//在AT24CXX指定地址写入一个数据
//WriteAddr  :写入数据的目的地址    
//DataToWrite:要写入的数据
void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite)
{
    u8 err = 0;
    OSMutexPend(iicMutexSem, (INT32U)IIC_MUTEX_TIMEOUT, &err);

    IIC_Start();
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(AT24CXX_SLAVER_ADDR);	    //发送写命令
		IIC_Wait_Ack();
		IIC_Send_Byte(WriteAddr>>8);//发送高地址	  
	}else{
		IIC_Send_Byte(AT24CXX_SLAVER_ADDR+((WriteAddr/256)<<1));   //发送器件地址0XA0,写数据 	 
	}
	IIC_Wait_Ack();	   
    IIC_Send_Byte(WriteAddr%256);   //发送低地址
	IIC_Wait_Ack(); 	 										  		   
	IIC_Send_Byte(DataToWrite);     //发送字节							   
	IIC_Wait_Ack();  		    	   
    IIC_Stop();//产生一个停止条件 
	delay_us(10000);//在os里面,这里必须用delay us 代替

    OSMutexPost(iicMutexSem);
}


//在AT24CXX里面的指定地址开始写入长度为Len的数据
//该函数用于写入16bit或者32bit的数据.
//WriteAddr  :开始写入的地址  
//DataToWrite:数据数组首地址
//Len        :要写入数据的长度2,4
void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len)
{  	
	u8 t;
	for(t=0;t<Len;t++)
	{
		AT24CXX_WriteOneByte(WriteAddr+t,(DataToWrite>>(8*t))&0xff);
	}												    
}

//在AT24CXX里面的指定地址开始读出长度为Len的数据
//该函数用于读出16bit或者32bit的数据.
//ReadAddr   :开始读出的地址 
//返回值     :数据
//Len        :要读出数据的长度2,4
u32 AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len)
{  	
	u8 t;
	u32 temp=0;
	for(t=0;t<Len;t++)
	{
		temp<<=8;
		temp+=AT24CXX_ReadOneByte(ReadAddr+Len-t-1); 	 				   
	}
	return temp;												    
}

//检查AT24CXX是否正常
//这里用了24XX的最后一个地址(255)来存储标志字.
//如果用其他24C系列,这个地址要修改
//返回1:检测失败
//返回0:检测成功
u8 AT24CXX_Check(void)
{
	u8 temp;
	AT24CXX_WriteOneByte(EE_ADDR_MAX,0X55);
	temp=AT24CXX_ReadOneByte(EE_ADDR_MAX);//避免每次开机都写AT24CXX			   
	if(temp==0X55)return 0;		   
	else//排除第一次初始化的情况
	{
		AT24CXX_WriteOneByte(EE_ADDR_MAX,0X55);
	    temp=AT24CXX_ReadOneByte(EE_ADDR_MAX);	  
		if(temp==0X55)return 0;
	}
	return 1;											  
}

//在AT24CXX里面的指定地址开始读出指定个数的数据
//ReadAddr :开始读出的地址 对24c02为0~255
//pBuffer  :数据数组首地址
//NumToRead:要读出数据的个数
void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead)
{
	while(NumToRead)
	{
		*pBuffer++=AT24CXX_ReadOneByte(ReadAddr++);	
		NumToRead--;
	}
}  
//在AT24CXX里面的指定地址开始写入指定个数的数据
//WriteAddr :开始写入的地址 对24c02为0~255
//pBuffer   :数据数组首地址
//NumToWrite:要写入数据的个数
void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite)
{
	while(NumToWrite--)
	{
		AT24CXX_WriteOneByte(WriteAddr,*pBuffer);
		WriteAddr++;
		pBuffer++;
	}
}

void FormatEEProm(void){
	u16 formatAddr = EE_ADDR_MAX + 1;
	while(formatAddr--)
	{
		AT24CXX_WriteOneByte(formatAddr, 0xFF);
	}
}

/**
* 擦除eraseAddr开始的长度为eraseLen的数据
*
**/
void EraseEEProm(u16 eraseAddr, u16 eraseLen){
	while(eraseLen--)
	{
		AT24CXX_WriteOneByte(eraseAddr, 0xFF);
		eraseAddr++;
	}
}

void EraseEEPormParas(void){
	EraseEEProm(NetParaAddr + 6, (18 - 6)); //MAC不恢复擦除，不恢复默认设置
	EraseEEProm(ADC_BASE_ADDR, 48);
	EraseEEProm(ALM_THRD_ADDR, 4);
	EraseEEProm(TENSION_MAX_RANGE_ADDR, 2);
	EraseEEProm(BASE_AUTO_CALIBRATE_TIME_ADDR, 2);
	EraseEEProm(FieldParaAddr, 4);
	EraseEEProm(ControlParaAddr, 1);
	EraseEEProm(SWValAddr, 2);
}

void WriteNetParasToEEProm(void){
	u16 addrOff = NetParaAddr;
	AT24CXX_Write(addrOff, lwipdev.mac, 6);
	addrOff +=6;
	AT24CXX_Write(addrOff, lwipdev.ip, 4);
	addrOff +=4;
	AT24CXX_Write(addrOff, lwipdev.netmask, 4);
	addrOff +=4;
	AT24CXX_Write(addrOff, lwipdev.gateway, 4);
	
}

void ReadNetParasFromEEProm(void){
	u16 addrOff = NetParaAddr;
	AT24CXX_Read(addrOff, lwipdev.mac, 6);
	addrOff +=6;
	AT24CXX_Read(addrOff, lwipdev.ip, 4);
	addrOff +=4;
	AT24CXX_Read(addrOff, lwipdev.netmask, 4);
	addrOff +=4;
	AT24CXX_Read(addrOff, lwipdev.gateway, 4);
}

/**
 *
 * 保存零点值(6线), KG u16 a_zero_val[6]; u16 b_zero_val[6];
 * field_index: 0:A防区， 1:B防区
 */
void WriteAdcZeroToEEProm(u8 field_index){
	AT24CXX_Write((field_index == 0? A_ADC_ZERO_ADDR : B_ADC_ZERO_ADDR), (u8 *)zero_val[field_index], sizeof(zero_val[field_index]));
}

/**
 *
 * 读取零点值(6线), KG u16 a_zero_val[6]; u16 b_zero_val[6];
 * field_index: 0:A防区， 1:B防区
 */
void ReadAdcZeroFromEEProm(u8 field_index){
	AT24CXX_Read((field_index == 0? A_ADC_ZERO_ADDR : B_ADC_ZERO_ADDR), (u8 *)zero_val[field_index], sizeof(zero_val[field_index]));
}

/**
 *
 * 保存零点值(6线), KG u16 a_base_val[6]; u16 b_base_val[6];
 * field_index: 0:A防区， 1:B防区
 */
void WriteAdcBaseToEEProm(u8 field_index){
	AT24CXX_Write((field_index == 0? A_ADC_BASE_ADDR : B_ADC_BASE_ADDR), (u8 *)base_val[field_index], sizeof(base_val[field_index]));
}

/**
 *
 * 读取零点值(6线), KG u16 a_base_val[6]; u16 b_base_val[6];
 * field_index: 0:A防区， 1:B防区
 */
void ReadAdcBaseFromEEProm(u8 field_index){
	AT24CXX_Read((field_index == 0? A_ADC_BASE_ADDR : B_ADC_BASE_ADDR), (u8 *)base_val[field_index], sizeof(base_val[field_index]));
}

void WriteAlmThrdToEEProm(void){
	AT24CXX_WriteOneByte(ALM_THRD_ADDR, (u8)((alarm_threshold_up_dif>>8) & 0xff));
	AT24CXX_WriteOneByte(ALM_THRD_ADDR + 1, (u8)(alarm_threshold_up_dif & 0xff));
	AT24CXX_WriteOneByte(ALM_THRD_ADDR + 2, (u8)((alarm_threshold_down_dif>>8) & 0xff));
	AT24CXX_WriteOneByte(ALM_THRD_ADDR + 3, (u8)(alarm_threshold_down_dif & 0xff));
}

void ReadAlmThrdFromEEProm(void){
	alarm_threshold_up_dif = AT24CXX_ReadOneByte(ALM_THRD_ADDR);
	alarm_threshold_up_dif = (alarm_threshold_up_dif << 8) | AT24CXX_ReadOneByte(ALM_THRD_ADDR + 1);
	alarm_threshold_down_dif = AT24CXX_ReadOneByte(ALM_THRD_ADDR + 2);
	alarm_threshold_down_dif = (alarm_threshold_down_dif << 8) | AT24CXX_ReadOneByte(ALM_THRD_ADDR + 3);
}

void WriteTensionMaxRangeToEEProm(void){
	AT24CXX_WriteOneByte(TENSION_MAX_RANGE_ADDR, (u8)((tension_max_range>>8) & 0xff));
	AT24CXX_WriteOneByte(TENSION_MAX_RANGE_ADDR + 1, (u8)(tension_max_range & 0xff));
}

void ReadTensionMaxRangeFromEEProm(void){
	tension_max_range = AT24CXX_ReadOneByte(TENSION_MAX_RANGE_ADDR);
	tension_max_range = (tension_max_range << 8) | AT24CXX_ReadOneByte(TENSION_MAX_RANGE_ADDR + 1);
}

void WriteBaseAutoCalibrateTimeToEEProm(void){
	AT24CXX_WriteOneByte(BASE_AUTO_CALIBRATE_TIME_ADDR, (u8)((base_auto_calibrate_time>>8) & 0xff));
	AT24CXX_WriteOneByte(BASE_AUTO_CALIBRATE_TIME_ADDR + 1, (u8)(base_auto_calibrate_time & 0xff));
}

void ReadBaseAutoCalibrateTimeFromEEProm(void){
	base_auto_calibrate_time = AT24CXX_ReadOneByte(BASE_AUTO_CALIBRATE_TIME_ADDR);
	base_auto_calibrate_time = (base_auto_calibrate_time << 8) | AT24CXX_ReadOneByte(BASE_AUTO_CALIBRATE_TIME_ADDR + 1);
}


//写入防区参数：8bytes, field_num(1)+alarm_delay(2)+alarm_sensitivity(1)+ten_val_range[0](2)+ten_val_range[1](2)}
void WriteFieldParaToEEProm(void){
	//field_num为8位， FieldParaAddr预留空。
	u8 addrTmp = FieldParaAddr;
	u8 i;
	for(i = 0; i < 2; i++){
		AT24CXX_WriteOneByte(addrTmp++, field_num[i]);
		AT24CXX_WriteOneByte(addrTmp++, (u8)((alarm_delay[i]>>8) & 0xff));
		AT24CXX_WriteOneByte(addrTmp++, (u8)(alarm_delay[i] & 0xff));
		AT24CXX_WriteOneByte(addrTmp++, alarm_sensitivity[i]);
		AT24CXX_WriteOneByte(addrTmp++, (u8)((ten_val_range[i][0]>>8) & 0xff));
		AT24CXX_WriteOneByte(addrTmp++, (u8)(ten_val_range[i][0] & 0xff));
		AT24CXX_WriteOneByte(addrTmp++, (u8)((ten_val_range[i][1]>>8) & 0xff));
		AT24CXX_WriteOneByte(addrTmp++, (u8)(ten_val_range[i][1] & 0xff));
	}
}

//读取防区参数：8bytes, field_num(1)+alarm_delay(2)+alarm_sensitivity(1)+ten_val_range[0](2)+ten_val_range[1](2)}
void ReadFieldParaFromEEProm(void){
	u8 addrTmp = FieldParaAddr;
	u8 i;
	for(i = 0; i < 2; i++){
		field_num[i] = AT24CXX_ReadOneByte(addrTmp++);
		alarm_delay[i] = AT24CXX_ReadOneByte(addrTmp++);
		alarm_delay[i] = (alarm_delay[i] << 8) | AT24CXX_ReadOneByte(addrTmp++);
		alarm_sensitivity[i] = AT24CXX_ReadOneByte(addrTmp++);
		ten_val_range[i][0] = AT24CXX_ReadOneByte(addrTmp++);
		ten_val_range[i][0] = (ten_val_range[i][0] << 8) | AT24CXX_ReadOneByte(addrTmp++);
		ten_val_range[i][1] = AT24CXX_ReadOneByte(addrTmp++);
		ten_val_range[i][1] = (ten_val_range[i][1] << 8) | AT24CXX_ReadOneByte(addrTmp++);
	}
}

void WriteControlParaToEEProm(void){
	u8 controlPara = (u8)(((flag_fcgjsn & 0x01)<<2) | ((BU_FANG_B_FLAG & 0x01)<<1) | (BU_FANG_A_FLAG & 0x01));
	AT24CXX_Write(ControlParaAddr, &controlPara, 1);
}

void ReadControlParaFromEEProm(void){
	u8 controlPara;
	AT24CXX_Read(ControlParaAddr, &controlPara, 1);
	flag_fcgjsn = (controlPara>>2) & 0x01;
	BU_FANG_B_FLAG = (controlPara>>1) & 0x01;
	BU_FANG_A_FLAG = controlPara & 0x01;
}

void WriteSWValToEEProm(void){
	AT24CXX_WriteOneByte(SWValAddr, SW_Type[0]);
	AT24CXX_WriteOneByte(SWValAddr + 1, SW_Type[1]);
}

void ReadSWValToEEProm(void){
	SW_Type[0] = AT24CXX_ReadOneByte(SWValAddr);
	SW_Type[1] = AT24CXX_ReadOneByte(SWValAddr + 1);
}

void WriteConfigCodeToEEProm(void){
	AT24CXX_WriteOneByte(ConfigCodeAddr, config_code);
}

void ReadConfigCodeToEEProm(void){
	config_code = AT24CXX_ReadOneByte(ConfigCodeAddr);
}
