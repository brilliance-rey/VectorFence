#include "adc.h"
#include "delay.h"
#include "alarm.h"
#include "24cxx.h"
//////////////////////////////////////////////////////////////////////////////////	 
//reneryan
//��������:2019/11/16
//�汾��V1.0
////////////////////////////////////////////////////////////////////////////////// 	

//������ Ĭ��200kg
u16 tension_max_range;

//A(0),B(1)�������ֵ(6��), KG
s16 zero_val[2][6];

//A(0),B(1)������׼ֵ(6��), KG
s16 base_val[2][6];

//A(0),B(1)������ǰ����ԭʼֵ(δ����ֵ����δ��ȥ���ֵ)(6��)�� KG
s16 crt_smp[2][6];

//A(0),B(1)������ǰֵ(6��)�� KG
s16 crt_val[2][6];

//����У׼��ǣ�bit7-bit4(0:У׼zero, 1:У׼��׼ֵ)��bit3-bit0(0:X�ᣬ 1:Y��)��Ҫͬ��������ģ�飨web�˵ȣ���У׼ʱ���ܼ�ز����� Ĭ��Ϊ��0xFF
u8 calibrating_flag = 0xFF;

u8 calibrate_en[2];

//�Զ�У׼ʱ�䣬��λ���֣�Ĭ��30����
u16 base_auto_calibrate_time = 30;

static s32 calibrate_temp_val[6] = { 0 };
static u16 calibrate_adc_cnt = 0;

static u8 init_done = 0;

/**����ֵ, 20��*/
//u32 ad_sample_val[AD_SAMPLE_COUNT];
//static u16 sample_cnt = 0;
//��ʼ��ADC															   
void Adc_Init(void)
{
	u8 i = 0;

	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC, ENABLE); //ʹ��GPIOCʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); //ʹ��ADC1ʱ��

	//	ADC���
	//	PC2��PIN_17��	I	ADC_A_Chn	X���ѹ���     			ͨ��12
	//	PC0��PIN_15��	I	ADC_B_Chn	Y���ѹ���			ͨ��10
	//	PC3��PIN_18��	I	ADC_AB_Chn	����ʱX/Y���ѹ���		ͨ��13
	//�ȳ�ʼ��ADC1ͨ��IO��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_0 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN; //ģ������
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //����������
	GPIO_Init(GPIOC, &GPIO_InitStructure); //��ʼ��

	//	ADCת��ͨ��ѡ��
	//		PC8��PIN_65��	O	ADC_CTRL0	������ʱѡͨA��Y��ĳһ·��Ϊ���
	//		PC9��PIN_66��	O	ADC_CTRL1
	//		PA8��PIN_67��	O	ADC_CTRL2	������ʱѡͨA��B��ʣ����·ĳһ·��Ϊ���
	//		PA9��PIN_68��	O	ADC_CTRL3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;	//��ͨ���ģʽ
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	//�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	//����
	GPIO_Init(GPIOC, &GPIO_InitStructure);	//������

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	//������

	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, ENABLE);	  	//ADC1��λ
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, DISABLE);		//��λ����

	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;	//����ģʽ
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;	//���������׶�֮����ӳ�5��ʱ��
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; //DMAʧ��
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4; //Ԥ��Ƶ4��Ƶ��ADCCLK=PCLK2/4=84/4=21Mhz,ADCʱ����ò�Ҫ����36Mhz
	ADC_CommonInit(&ADC_CommonInitStructure); //��ʼ��

	//ADC��ͨ��DMA�ɼ�ʱADֵ����4095�������������ο�https://blog.csdn.net/u014470361/article/details/81987313
	ADC_StructInit(&ADC_InitStructure); //����1 �����ӵ�,
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b; //12λģʽ
	ADC_InitStructure.ADC_ScanConvMode = DISABLE; //��ɨ��ģʽ
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; //�ر�����ת��
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; //��ֹ������⣬ʹ���������
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; //�Ҷ���
	ADC_InitStructure.ADC_NbrOfConversion = 1; //1��ת���ڹ��������� Ҳ����ֻת����������1
//	ADC_InitStructure.ADC_ExternalTrigConv =  ������Ϊ������ѡ���ⲿ�¼���

	//ADC��ͨ��DMA�ɼ�ʱADֵ����4095�������������ο�https://blog.csdn.net/u014470361/article/details/81987313
//	ADC_InitStructure.ADC_ExternalTrigConv = 0; //����2 �����ӵ�,

	ADC_Init(ADC1, &ADC_InitStructure); //ADC��ʼ��

	ADC_Cmd(ADC1, ENABLE); //����ADת����

	//�����������0xFF�����ǳ���״̬��default�ˡ�

	for (i = 0; i < 2; i++)
	{
//		ReadAdcZeroFromEEProm(i);
//		if ((zero_val[i][0] & 0xffff) == 0xffff)
//		{
//			mymemset(zero_val[i], 0, sizeof(zero_val[i]));
//			WriteAdcZeroToEEProm(i);
//		}
//
//		ReadAdcBaseFromEEProm(i);
//		if ((base_val[i][0] & 0xffff) == 0xffff)
//		{
//			mymemset(base_val[i], 0, sizeof(base_val[i]));
//			WriteAdcBaseToEEProm(i);
//		}
		calibrate_en[i] = 1;
	}


	//�����������0xFF�����ǳ���״̬��default�ˡ�
	ReadTensionMaxRangeFromEEProm();
	if(tension_max_range == 0xffff){
		tension_max_range = TENSION_MAX_RANGE_DEF;
		WriteTensionMaxRangeToEEProm();
	}

	//�����������0xFF�����ǳ���״̬��default�ˡ�
	ReadBaseAutoCalibrateTimeFromEEProm();
	if(base_auto_calibrate_time == 0xffff){
		base_auto_calibrate_time = 30;
		WriteBaseAutoCalibrateTimeToEEProm();
	}
	init_done = 1;
}

/**
 * ����������
 */
void setTensionMaxRange(u16 value)
{
	tension_max_range = value;
//	ȡ���Զ�����
//	updateTensionParasByMaxValue();
	WriteTensionMaxRangeToEEProm();

}

//���ADCֵ
//ch: @ref ADC_channels 
//ͨ��ֵ 0~16ȡֵ��ΧΪ��ADC_Channel_0~ADC_Channel_16
//����ֵ:ת�����
u16 Get_Adc(u8 ch)
{
	if(!init_done){
		return 0;
	}
	//����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles);	//ADC1,ADCͨ��,480������,��߲���ʱ�������߾�ȷ��

	ADC_SoftwareStartConv(ADC1);		//ʹ��ָ����ADC1�����ת����������	

	while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC))
		;		//�ȴ�ת������

	return ADC_GetConversionValue(ADC1);	//�������һ��ADC1�������ת�����
}

//��ȡͨ��ch��ת��ֵ��ȡtimes��,Ȼ��ƽ�� 
//ch:ͨ�����
//times:��ȡ����
//����ֵ:ͨ��ch��times��ת�����ƽ��ֵ
u16 Get_Adc_Average(u8 ch, u8 times)
{
	u32 temp_val = 0;
	u8 t;
	for (t = 0; t < times; t++)
	{
		temp_val += Get_Adc(ch);
		delay_ms(5);
	}
	return temp_val / times;
}

//��ȡͨ��ch��ת��ֵ��ȡ20��,Ȼ��ƽ��
//ch:ͨ�����
//times:��ȡ����
//����ֵ:ͨ��ch��times��ת�����ƽ��ֵ
u16 Get_Adc_Average_Def(u8 ch)
{
	u32 temp_val = 0;
	u8 t;
	for (t = 0; t < ADC_TIME_DEFAULT; t++)
	{
		temp_val += Get_Adc(ch);
		delay_ms(5);
	}
	return temp_val / ADC_TIME_DEFAULT;
}

/**
 * X��ADC���ʹ�ܣ� �ο�<<Detection.pdf>>
 * line_inde: ����ţ����ߣ�0-3, ���ߣ�0-5��
 */
void A_ADC_CK_EN(u8 line_index)
{
	switch (line_index)
	{
		case 0:
		{
			ADC_CTRL1 = 1;
			ADC_CTRL0 = 0;
			break;
		}
		case 1:
		{
			ADC_CTRL1 = 0;
			ADC_CTRL0 = 1;
			break;
		}
		case 2:
		{
			ADC_CTRL1 = 0;
			ADC_CTRL0 = 0;
			break;
		}
		case 3:
		{
			ADC_CTRL1 = 1;
			ADC_CTRL0 = 1;
			break;
		}
		case 4:
		{
			ADC_CTRL3 = 1;
			ADC_CTRL2 = 0;
			break;
		}
		case 5:
		{
			ADC_CTRL3 = 0;
			ADC_CTRL2 = 0;
			break;
		}
		default:
			break;
	}
}

/**
 * Y��ADC���ʹ�ܣ� �ο�<<Detection.pdf>>
 * line_inde: ����ţ����ߣ�0-3, ���ߣ�0-5��
 */
void B_ADC_CK_EN(u8 line_index)
{
	switch (line_index)
	{
		case 0:
		{
			ADC_CTRL1 = 0;
			ADC_CTRL0 = 1;
			break;
		}
		case 1:
		{
			ADC_CTRL1 = 1;
			ADC_CTRL0 = 1;
			break;
		}
		case 2:
		{
			ADC_CTRL1 = 1;
			ADC_CTRL0 = 0;
			break;
		}
		case 3:
		{
			ADC_CTRL1 = 0;
			ADC_CTRL0 = 0;
			break;
		}
		case 4:
		{
			ADC_CTRL3 = 1;
			ADC_CTRL2 = 1;
			break;
		}
		case 5:
		{
			ADC_CTRL3 = 0;
			ADC_CTRL2 = 1;
			break;
		}
		default:
			break;
	}
}

/**
 * ��ȡ����Adcԭʼֵ��δ������ֵ����δ��ȥ���ֵ��
 * axix_index: 0:X�ᣬ 1:Y��
 * times��ȡƽ��ֵ�Ĳ���������0����ΪĬ��ֵ��ADC_TIME_DEFAULT�Σ�
 */
void sampleAdc(u8 axix_index, u16 times)
{
	s32 temp_val[6] = { 0 };
	u8 t, i = 0;
	u8 line_count = (W4_W6 == IS_W4 ? 4 : 6);

	if (0 == times)
	{
		times = ADC_TIME_DEFAULT;
	}

	//��β���ȡƽ��ֵ
	for (t = 0; t < times; t++)
	{
		//ÿ�β���X,Y��·��ֵ��
		for (i = 0; i < line_count; i++)
		{
			//����X���ֵ
			if(0 == axix_index){
				A_ADC_CK_EN(i);
				if (i < 4)	//4��
				{
					temp_val[i] += Get_Adc(ADC_A_Chn);
				}
				else
				{
					temp_val[i] += Get_Adc(ADC_AB_Chn);
				}
			}
			//����Y���ֵ
			else if(1 == axix_index){
				B_ADC_CK_EN(i);
				if (i < 4)  //4��
				{
					temp_val[i] += Get_Adc(ADC_B_Chn);
				}
				else
				{
					temp_val[i] += Get_Adc(ADC_AB_Chn);
				}
			}
		}
		delay_ms(2);
	}

	//��ȡ������ʵ��KGֵ
	for (i = 0; i < line_count; i++)
	{
		crt_smp[axix_index][i] = (u16) ((temp_val[i] / times) * tension_max_range / 4096);
	}
}

/**
 * �������ֵ�� ��Ԥ��
 * axix_index: 0:X�ᣬ 1:Y��
 */
void updateZeroVal(u8 axix_index)
{
	calibrating_flag = 0x00 | axix_index;
}


/**
 * У׼
 */
void calibrating(void)
{
	u8 i = 0;
	u8 line_count = 6;
	u8 axix_index = 0;

	if (calibrating_flag == 0xFF)
	{
		return;
	}

//	line_count = (W4_W6 == IS_W4 ? 4 : 6);
	axix_index = calibrating_flag & 0x01;

	//У����ɡ�������������
	if ((calibrating_flag & 0x80) == 0x80)
	{
		//zero
		if ((calibrating_flag & 0x10) == 0x00)
		{
			for (i = 0; i < line_count; i++)
			{
				//��ƽ��ֵ
				zero_val[axix_index][i] = (calibrate_temp_val[i] / calibrate_adc_cnt);
			}
			WriteAdcZeroToEEProm(axix_index);
		}
		//base
		if ((calibrating_flag & 0x10) == 0x10)
		{
			for (i = 0; i < line_count; i++)
			{
				//��ƽ��ֵ, ��ȥ���ֵ��ʵ��KGֵ
				base_val[axix_index][i] = (s16)((calibrate_temp_val[i] / calibrate_adc_cnt) - zero_val[axix_index][i]);
				if(base_val[axix_index][i] < 0){
					base_val[axix_index][i] = 0;
				}
			}
			WriteAdcBaseToEEProm(axix_index);
		}

		calibrate_adc_cnt = 0;
		memset(calibrate_temp_val, 0, sizeof(calibrate_temp_val));
		calibrating_flag = 0xFF;

		return;
	}
	//����
	sampleAdc(axix_index, 0);
	//�����ۼ�
	for (i = 0; i < line_count; i++)
	{
		calibrate_temp_val[i] += crt_smp[axix_index][i];
	}
	calibrate_adc_cnt++;
}

/**
 * ���»�׼ֵ. ��Ԥ��
 * axix_index: 0:X�ᣬ 1:Y��
 */
void updateBaseVal(u8 axix_index)
{
	calibrating_flag = 0x10 | axix_index;
}

/**
 *  ����X,Y�ᵱǰ����ֵ������ֵ����ȥ���ֵ��
 * axis_index: 0:X�ᣬ 1:Y��
 */
void updateCrtSmp(u8 axix_index)
{
	u8 i;
	u8 line_count = 6;
	if(calibrating_flag == 0xff){
		sampleAdc(axix_index, 0);
		calibrate_en[axix_index] = 1;
		for (i = 0; i < line_count; i++)
		{
	//��ȡ������ʵ��KGֵ
			crt_val[axix_index][i] = (s16)(crt_smp[axix_index][i] - zero_val[axix_index][i]);
			if(crt_val[axix_index][i] < 0){
				crt_val[axix_index][i] = 0;
			}

//			if(crt_val[axix_index][i] < ten_val_range[axix_index][0] || crt_val[axix_index][i] > ten_val_range[axix_index][1]){
//				calibrate_en[axix_index] = 0;
//			}
//			//��Ч��Χ�޸�Ϊ2Kg - maxKg
//			if(crt_val[axix_index][i] < 20 || crt_val[axix_index][i] > tension_max_range){
//				calibrate_en[axix_index] = 0;
//			}
		}
	}
}

/**
 * ��ȡ����web�˵�ADC(����crt, zero, base)��
 * �磺���ߣ�crt@zero@base, ���磺"55+55+56+34+44+55#55+55+56+34+44+55@0+9+3+4+5+3#0+0+2+3+4+3@55+55+56+34+44+55#55+55+56+34+44+55";
 */
void getWebAdcValStr(char *adcWebStr)
{
	sprintf((char*) adcWebStr, "%d+%d+%d+%d+%d+%d#%d+%d+%d+%d+%d+%d",
//			"@%d+%d+%d+%d+%d+%d#%d+%d+%d+%d+%d+%d@%d+%d+%d+%d+%d+%d#%d+%d+%d+%d+%d+%d",
			crt_val[0][0], crt_val[0][1], crt_val[0][2], crt_val[0][3], crt_val[0][4], crt_val[0][5], crt_val[1][0], crt_val[1][1], crt_val[1][2], crt_val[1][3], crt_val[1][4], crt_val[1][5],
//			zero_val[0][0], zero_val[0][1], zero_val[0][2], zero_val[0][3], zero_val[0][4], zero_val[0][5], zero_val[1][0], zero_val[1][1], zero_val[1][2], zero_val[1][3], zero_val[1][4], zero_val[1][5],
//			base_val[0][0], base_val[0][1], base_val[0][2], base_val[0][3], base_val[0][4], base_val[0][5], base_val[1][0], base_val[1][1], base_val[1][2], base_val[1][3], base_val[1][4], base_val[1][5]
	);
}

/**
 * �����Զ�У׼ʱ�䣬Ԥ��
 */
void setBaseAutoCalibrateTime(u16 value){
	base_auto_calibrate_time = value;
	WriteBaseAutoCalibrateTimeToEEProm();
}

