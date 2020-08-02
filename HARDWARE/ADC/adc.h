#ifndef __ADC_H
#define __ADC_H
#include "sys.h"
#include "alarm.h"
#include "malloc.h"

//校准持续采用的时间，单位：毫秒
#define CALIBRATING_TIME 5000

//////////////////////////////////////////////////////////////////////////////////	 
//reneryan
//创建日期:2019/11/16
//版本：V1.0
////////////////////////////////////////////////////////////////////////////////// 	 

//	ADC转换通道选择：
//		PC8（PIN_65）	O	ADC_CTRL0	四线制时选通A和Y轴某一路作为输出
//		PC9（PIN_66）	O	ADC_CTRL1
//		PA8（PIN_67）	O	ADC_CTRL2	六线制时选通A或B各剩余两路某一路作为输出
//		PA9（PIN_68）	O	ADC_CTRL3
#define ADC_CTRL0 PCout(8)
#define ADC_CTRL1 PCout(9)
#define ADC_CTRL2 PAout(8)
#define ADC_CTRL3 PAout(9)

//通道号	ADC1	ADC2	ADC3
//通道 10	PC0		PC0		PC0
//通道 12	PC2		PC2		PC2
//通道 13	PC3		PC3		PC3

//	ADC检测
//	PC2（PIN_17）	I	ADC_A_Chn	X轴电压监测
//	PC0（PIN_15）	I	ADC_B_Chn	Y轴电压监测
//	PC3（PIN_18）	I	ADC_AB_Chn	六线时X/Y轴电压监测

#define ADC_A_Chn	ADC_Channel_12
#define ADC_B_Chn	ADC_Channel_10
#define ADC_AB_Chn	ADC_Channel_13

#define ADC_TIME_DEFAULT 20

//满量程 默认200kg ，也就是3.3V对应的力，单位0.1KG，如100对应10Kg
extern u16 tension_max_range;

//A(0),B(1)防区零点值(6线), KG
extern s16 zero_val[2][6];

//A(0),B(1)防区基准值(6线), KG
extern s16 base_val[2][6];

//A(0),B(1)防区当前值(6线)， KG
extern s16 crt_val[2][6];

//校准标记：bit7-4: 0：校准zero, 1:校准基准值, bit3-0: 0:X轴，1:Y轴， 默认为：0xFF
extern u8 calibrating_flag;

//校准使能标志，1：能校准，0：不能校准
extern u8 calibrate_en[2];

//基准值自动校准时间，单位：分
extern u16 base_auto_calibrate_time;

//ADC通道初始化
void Adc_Init(void);

/**
 * 设置满量程
 */
void setTensionMaxRange(u16 value);

//获得某个通道值
u16	Get_Adc(u8 ch);
//得到某个通道给定次数采样的平均值
u16	Get_Adc_Average(u8 ch,	u8 times);
//得到某个通道给定次数采样的平均值
u16 Get_Adc_Average_Def(u8 ch);


/**
 * 获取采样Adc原始值，未修正的值，即未减去零点值。
 * axix_index: 0:X轴， 1:Y轴
 * times：取平均值的采样次数，0：是为默认值（ADC_TIME_DEFAULT次）
 */
void sampleAdc(u8 axix_index, u16 times);

///**
// * 单独获取X轴的Adc采样原始值，未修正的值，即未减去零点值。
// * times：取平均值的采样次数，0：是为默认值（ADC_TIME_DEFAULT次）
// */
//void sampleAdcA(u16 times);
//
///**
// * 单独获取Y轴的Adc采样原始值，未修正的值，即未减去零点值。
// * times：取平均值的采样次数，0：是为默认值（ADC_TIME_DEFAULT次）
// */
//void sampleAdcB(u16 times);

/**
 * 更新零点值
 * axix_index: 0:X轴， 1:Y轴
 */
void updateZeroVal(u8 axix_index);

/**
 * 更新基准值
 * axix_index: 0:X轴， 1:Y轴
 */
void updateBaseVal(u8 axix_index);

/**
 * 校准中。。。
 */
void calibrating(void);

/**
 * 更新A,Y轴当前采样值的修正值（减去零点值）
 * axix_index: 0:X轴， 1:Y轴
 */
void updateCrtSmp(u8 axix_index);
//
///**
// * 更新X轴当前采样值的修正值（减去零点值）
// */
//void updateCrtSmpA(void);
//
///**
// * 更新Y轴当前采样值的修正值（减去零点值）
// */
//void updateCrtSmpB(void);

/**
 * 获取返回web端的ADC(包括crt, zero, base)。
 * 如：六线：crt@zero@base, 例如："55+55+56+34+44+55#55+55+56+34+44+55@0+9+3+4+5+3#0+0+2+3+4+3@55+55+56+34+44+55#55+55+56+34+44+55";
 */
void getWebAdcValStr(char *webAdcStr);

void setBaseAutoCalibrateTime(u16 value);

#endif 

