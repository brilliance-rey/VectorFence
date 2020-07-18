#ifndef __ADC_H
#define __ADC_H
#include "sys.h"
#include "alarm.h"
#include "malloc.h"

//У׼�������õ�ʱ�䣬��λ������
#define CALIBRATING_TIME 5000

//////////////////////////////////////////////////////////////////////////////////	 
//reneryan
//��������:2019/11/16
//�汾��V1.0
////////////////////////////////////////////////////////////////////////////////// 	 

//	ADCת��ͨ��ѡ��
//		PC8��PIN_65��	O	ADC_CTRL0	������ʱѡͨA��B����ĳһ·��Ϊ���
//		PC9��PIN_66��	O	ADC_CTRL1
//		PA8��PIN_67��	O	ADC_CTRL2	������ʱѡͨA��B��ʣ����·ĳһ·��Ϊ���
//		PA9��PIN_68��	O	ADC_CTRL3
#define ADC_CTRL0 PCout(8)
#define ADC_CTRL1 PCout(9)
#define ADC_CTRL2 PAout(8)
#define ADC_CTRL3 PAout(9)

//ͨ����	ADC1	ADC2	ADC3
//ͨ�� 10	PC0		PC0		PC0
//ͨ�� 12	PC2		PC2		PC2
//ͨ�� 13	PC3		PC3		PC3

//	ADC���
//	PC2��PIN_17��	I	ADC_A_Chn	A������ѹ���
//	PC0��PIN_15��	I	ADC_B_Chn	B������ѹ���
//	PC3��PIN_18��	I	ADC_AB_Chn	����ʱA/B������ѹ���

#define ADC_A_Chn	ADC_Channel_12
#define ADC_B_Chn	ADC_Channel_10
#define ADC_AB_Chn	ADC_Channel_13

#define ADC_TIME_DEFAULT 20

//������ Ĭ��200kg ��Ҳ����3.3V��Ӧ��������λ0.1KG����100��Ӧ10Kg
extern u16 tension_max_range;

//A(0),B(1)�������ֵ(6��), KG
extern s16 zero_val[2][6];

//A(0),B(1)������׼ֵ(6��), KG
extern s16 base_val[2][6];

//A(0),B(1)������ǰֵ(6��)�� KG
extern s16 crt_val[2][6];

//У׼��ǣ�bit7-4: 0��У׼zero, 1:У׼��׼ֵ, bit3-0: 0:A������1:B������ Ĭ��Ϊ��0xFF
extern u8 calibrating_flag;

//У׼ʹ�ܱ�־��1����У׼��0������У׼
extern u8 calibrate_en[2];

//��׼ֵ�Զ�У׼ʱ�䣬��λ����
extern u16 base_auto_calibrate_time;

//ADCͨ����ʼ��
void Adc_Init(void);

/**
 * ����������
 */
void setTensionMaxRange(u16 value);

//���ĳ��ͨ��ֵ
u16	Get_Adc(u8 ch);
//�õ�ĳ��ͨ����������������ƽ��ֵ
u16	Get_Adc_Average(u8 ch,	u8 times);
//�õ�ĳ��ͨ����������������ƽ��ֵ
u16 Get_Adc_Average_Def(u8 ch);


/**
 * ��ȡ����Adcԭʼֵ��δ������ֵ����δ��ȥ���ֵ��
 * field_index: 0:A������ 1:B����
 * times��ȡƽ��ֵ�Ĳ���������0����ΪĬ��ֵ��ADC_TIME_DEFAULT�Σ�
 */
void sampleAdc(u8 field_index, u16 times);

///**
// * ������ȡA������Adc����ԭʼֵ��δ������ֵ����δ��ȥ���ֵ��
// * times��ȡƽ��ֵ�Ĳ���������0����ΪĬ��ֵ��ADC_TIME_DEFAULT�Σ�
// */
//void sampleAdcA(u16 times);
//
///**
// * ������ȡB������Adc����ԭʼֵ��δ������ֵ����δ��ȥ���ֵ��
// * times��ȡƽ��ֵ�Ĳ���������0����ΪĬ��ֵ��ADC_TIME_DEFAULT�Σ�
// */
//void sampleAdcB(u16 times);

/**
 * �������ֵ
 * field_index: 0:A������ 1:B����
 */
void updateZeroVal(u8 field_index);

/**
 * ���»�׼ֵ
 * field_index: 0:A������ 1:B����
 */
void updateBaseVal(u8 field_index);

/**
 * У׼�С�����
 */
void calibrating(void);

/**
 * ����A,B������ǰ����ֵ������ֵ����ȥ���ֵ��
 * field_index: 0:A������ 1:B����
 */
void updateCrtSmp(u8 field_index);
//
///**
// * ����A������ǰ����ֵ������ֵ����ȥ���ֵ��
// */
//void updateCrtSmpA(void);
//
///**
// * ����B������ǰ����ֵ������ֵ����ȥ���ֵ��
// */
//void updateCrtSmpB(void);

/**
 * ��ȡ����web�˵�ADC(����crt, zero, base)��
 * �磺���ߣ�crt@zero@base, ���磺"55+55+56+34+44+55#55+55+56+34+44+55@0+9+3+4+5+3#0+0+2+3+4+3@55+55+56+34+44+55#55+55+56+34+44+55";
 */
void getWebAdcValStr(char *webAdcStr);

void setBaseAutoCalibrateTime(u16 value);

#endif 

