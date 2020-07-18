#ifndef __24CXX_H
#define __24CXX_H
#include "myiic.h"   


#define AT24C01		127
#define AT24C02		255
#define AT24C04		511
#define AT24C08		1023
#define AT24C16		2047
#define AT24C32		4095
#define AT24C64	  	8191
#define AT24C128	16383
#define AT24C256	32767  
//Mini STM32开发板使用的是24c02，所以定义EE_TYPE为AT24C02
#define EE_TYPE AT24C256

#define EE_ADDR_MAX AT24C256

#define AT24CXX_SLAVER_ADDR 0xA0

#define NetParaAddr  0x0000  //6+4+4+4=18 bytes =0x12 bytes
#define ConfigCodeAddr 0x001E  //1byte

#define ADC_BASE_ADDR	0x0020
//A零点值：2byte * 6 = 12byte
#define A_ADC_ZERO_ADDR	ADC_BASE_ADDR
//B零点值：2byte * 6 = 12byte
#define B_ADC_ZERO_ADDR	(ADC_BASE_ADDR + 12)
//A基准值：2byte * 6 = 12byte
#define A_ADC_BASE_ADDR	(ADC_BASE_ADDR + 24)
//B基准值：2byte * 6 = 12byte
#define B_ADC_BASE_ADDR	(ADC_BASE_ADDR + 36)

//4 bytes alarm_threshold_up_dif(2bytes) + alarm_threshold_down_dif(2bytes)
#define ALM_THRD_ADDR  (ADC_BASE_ADDR + 48)	//0x0020+48 = 0x0050

#define ControlParaAddr	0x0054	//control: 1byte ///** bit7-bit3 无   bit2: Fangchai   bit1:BuFangB    bit0:BuFangA   1:ON  0:OFF **/
#define SWValAddr		0x0056	//switch value. 2byte, sw2,sw1.

#define TENSION_MAX_RANGE_ADDR	0x005A
#define BASE_AUTO_CALIBRATE_TIME_ADDR	0x005C

//8bytes, field_num(1)+alarm_delay(2)+alarm_sensitivity(1)+ten_val_range[0](2)+ten_val_range[1](2)}
#define FieldParaAddr	0x0060

u8 AT24CXX_ReadOneByte(u16 ReadAddr);							//指定地址读取一个字节
void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite);		//指定地址写入一个字节
void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len);//指定地址开始写入指定长度的数据
u32 AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len);					//指定地址开始读取指定长度数据
void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite);	//从指定地址开始写入指定长度的数据
void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead);   	//从指定地址开始读出指定长度的数据

u8 AT24CXX_Check(void);  //检查器件
void AT24CXX_Init(void); //初始化IIC

void FormatEEProm(void);
void EraseEEProm(u16 eraseAddr, u16 eraseLen);
void EraseEEPormParas(void);
	
void WriteNetParasToEEProm(void);
void ReadNetParasFromEEProm(void);

void WriteAdcZeroToEEProm(u8 field_index);
void ReadAdcZeroFromEEProm(u8 field_index);

void WriteAdcBaseToEEProm(u8 field_index);
void ReadAdcBaseFromEEProm(u8 field_index);

void WriteAlmThrdToEEProm(void);
void ReadAlmThrdFromEEProm(void);

void WriteTensionMaxRangeToEEProm(void);
void ReadTensionMaxRangeFromEEProm(void);

void WriteBaseAutoCalibrateTimeToEEProm(void);
void ReadBaseAutoCalibrateTimeFromEEProm(void);

void WriteFieldParaToEEProm(void);
void ReadFieldParaFromEEProm(void);

void WriteControlParaToEEProm(void);
void ReadControlParaFromEEProm(void);

void WriteSWValToEEProm(void);
void ReadSWValToEEProm(void);

void WriteConfigCodeToEEProm(void);
void ReadConfigCodeToEEProm(void);

#endif
