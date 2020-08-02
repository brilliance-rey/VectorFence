#include "adc.h"
#include "delay.h"
#include "alarm.h"
#include "24cxx.h"
//////////////////////////////////////////////////////////////////////////////////	 
//reneryan
//创建日期:2019/11/16
//版本：V1.0
////////////////////////////////////////////////////////////////////////////////// 	

//满量程 默认200kg
u16 tension_max_range;

//A(0),B(1)防区零点值(6线), KG
s16 zero_val[2][6];

//A(0),B(1)防区基准值(6线), KG
s16 base_val[2][6];

//A(0),B(1)防区当前采样原始值(未修正值，即未减去零点值)(6线)， KG
s16 crt_smp[2][6];

//A(0),B(1)防区当前值(6线)， KG
s16 crt_val[2][6];

//正在校准标记：bit7-bit4(0:校准zero, 1:校准基准值)，bit3-bit0(0:X轴， 1:Y轴)，要同步到其他模块（web端等），校准时不能监控操作。 默认为：0xFF
u8 calibrating_flag = 0xFF;

u8 calibrate_en[2];

//自动校准时间，单位：分，默认30分钟
u16 base_auto_calibrate_time = 30;

static s32 calibrate_temp_val[6] = { 0 };
static u16 calibrate_adc_cnt = 0;

static u8 init_done = 0;

/**采样值, 20次*/
//u32 ad_sample_val[AD_SAMPLE_COUNT];
//static u16 sample_cnt = 0;
//初始化ADC															   
void Adc_Init(void)
{
	u8 i = 0;

	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC, ENABLE); //使能GPIOC时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); //使能ADC1时钟

	//	ADC检测
	//	PC2（PIN_17）	I	ADC_A_Chn	X轴电压监测     			通道12
	//	PC0（PIN_15）	I	ADC_B_Chn	Y轴电压监测			通道10
	//	PC3（PIN_18）	I	ADC_AB_Chn	六线时X/Y轴电压监测		通道13
	//先初始化ADC1通道IO口
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_0 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN; //模拟输入
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //不带上下拉
	GPIO_Init(GPIOC, &GPIO_InitStructure); //初始化

	//	ADC转换通道选择：
	//		PC8（PIN_65）	O	ADC_CTRL0	四线制时选通A和Y轴某一路作为输出
	//		PC9（PIN_66）	O	ADC_CTRL1
	//		PA8（PIN_67）	O	ADC_CTRL2	六线制时选通A或B各剩余两路某一路作为输出
	//		PA9（PIN_68）	O	ADC_CTRL3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;	//普通输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	//上拉
	GPIO_Init(GPIOC, &GPIO_InitStructure);	//四线制

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	//六线制

	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, ENABLE);	  	//ADC1复位
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1, DISABLE);		//复位结束

	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;	//独立模式
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;	//两个采样阶段之间的延迟5个时钟
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; //DMA失能
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4; //预分频4分频。ADCCLK=PCLK2/4=84/4=21Mhz,ADC时钟最好不要超过36Mhz
	ADC_CommonInit(&ADC_CommonInitStructure); //初始化

	//ADC多通道DMA采集时AD值大于4095的问题解决方法参考https://blog.csdn.net/u014470361/article/details/81987313
	ADC_StructInit(&ADC_InitStructure); //方法1 新增加的,
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b; //12位模式
	ADC_InitStructure.ADC_ScanConvMode = DISABLE; //非扫描模式
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; //关闭连续转换
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; //禁止触发检测，使用软件触发
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; //右对齐
	ADC_InitStructure.ADC_NbrOfConversion = 1; //1个转换在规则序列中 也就是只转换规则序列1
//	ADC_InitStructure.ADC_ExternalTrigConv =  是用来为规则组选择外部事件。

	//ADC多通道DMA采集时AD值大于4095的问题解决方法参考https://blog.csdn.net/u014470361/article/details/81987313
//	ADC_InitStructure.ADC_ExternalTrigConv = 0; //方法2 新增加的,

	ADC_Init(ADC1, &ADC_InitStructure); //ADC初始化

	ADC_Cmd(ADC1, ENABLE); //开启AD转换器

	//如果读到的数0xFF，则是出厂状态或default了。

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


	//如果读到的数0xFF，则是出厂状态或default了。
	ReadTensionMaxRangeFromEEProm();
	if(tension_max_range == 0xffff){
		tension_max_range = TENSION_MAX_RANGE_DEF;
		WriteTensionMaxRangeToEEProm();
	}

	//如果读到的数0xFF，则是出厂状态或default了。
	ReadBaseAutoCalibrateTimeFromEEProm();
	if(base_auto_calibrate_time == 0xffff){
		base_auto_calibrate_time = 30;
		WriteBaseAutoCalibrateTimeToEEProm();
	}
	init_done = 1;
}

/**
 * 设置满量程
 */
void setTensionMaxRange(u16 value)
{
	tension_max_range = value;
//	取消自动更新
//	updateTensionParasByMaxValue();
	WriteTensionMaxRangeToEEProm();

}

//获得ADC值
//ch: @ref ADC_channels 
//通道值 0~16取值范围为：ADC_Channel_0~ADC_Channel_16
//返回值:转换结果
u16 Get_Adc(u8 ch)
{
	if(!init_done){
		return 0;
	}
	//设置指定ADC的规则组通道，一个序列，采样时间
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles);	//ADC1,ADC通道,480个周期,提高采样时间可以提高精确度

	ADC_SoftwareStartConv(ADC1);		//使能指定的ADC1的软件转换启动功能	

	while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC))
		;		//等待转换结束

	return ADC_GetConversionValue(ADC1);	//返回最近一次ADC1规则组的转换结果
}

//获取通道ch的转换值，取times次,然后平均 
//ch:通道编号
//times:获取次数
//返回值:通道ch的times次转换结果平均值
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

//获取通道ch的转换值，取20次,然后平均
//ch:通道编号
//times:获取次数
//返回值:通道ch的times次转换结果平均值
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
 * X轴ADC检测使能， 参考<<Detection.pdf>>
 * line_inde: 线序号（四线：0-3, 六线：0-5）
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
 * Y轴ADC检测使能， 参考<<Detection.pdf>>
 * line_inde: 线序号（四线：0-3, 六线：0-5）
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
 * 获取采样Adc原始值，未修正的值，即未减去零点值。
 * axix_index: 0:X轴， 1:Y轴
 * times：取平均值的采样次数，0：是为默认值（ADC_TIME_DEFAULT次）
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

	//多次采样取平均值
	for (t = 0; t < times; t++)
	{
		//每次采样X,Y两路的值，
		for (i = 0; i < line_count; i++)
		{
			//采样X轴的值
			if(0 == axix_index){
				A_ADC_CK_EN(i);
				if (i < 4)	//4线
				{
					temp_val[i] += Get_Adc(ADC_A_Chn);
				}
				else
				{
					temp_val[i] += Get_Adc(ADC_AB_Chn);
				}
			}
			//采样Y轴的值
			else if(1 == axix_index){
				B_ADC_CK_EN(i);
				if (i < 4)  //4线
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

	//获取计算后的实际KG值
	for (i = 0; i < line_count; i++)
	{
		crt_smp[axix_index][i] = (u16) ((temp_val[i] / times) * tension_max_range / 4096);
	}
}

/**
 * 更新零点值， ，预留
 * axix_index: 0:X轴， 1:Y轴
 */
void updateZeroVal(u8 axix_index)
{
	calibrating_flag = 0x00 | axix_index;
}


/**
 * 校准
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

	//校验完成。。。，计算结果
	if ((calibrating_flag & 0x80) == 0x80)
	{
		//zero
		if ((calibrating_flag & 0x10) == 0x00)
		{
			for (i = 0; i < line_count; i++)
			{
				//求平均值
				zero_val[axix_index][i] = (calibrate_temp_val[i] / calibrate_adc_cnt);
			}
			WriteAdcZeroToEEProm(axix_index);
		}
		//base
		if ((calibrating_flag & 0x10) == 0x10)
		{
			for (i = 0; i < line_count; i++)
			{
				//求平均值, 减去零点值得实际KG值
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
	//采样
	sampleAdc(axix_index, 0);
	//连续累加
	for (i = 0; i < line_count; i++)
	{
		calibrate_temp_val[i] += crt_smp[axix_index][i];
	}
	calibrate_adc_cnt++;
}

/**
 * 更新基准值. ，预留
 * axix_index: 0:X轴， 1:Y轴
 */
void updateBaseVal(u8 axix_index)
{
	calibrating_flag = 0x10 | axix_index;
}

/**
 *  更新X,Y轴当前采样值的修正值（减去零点值）
 * axis_index: 0:X轴， 1:Y轴
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
	//获取计算后的实际KG值
			crt_val[axix_index][i] = (s16)(crt_smp[axix_index][i] - zero_val[axix_index][i]);
			if(crt_val[axix_index][i] < 0){
				crt_val[axix_index][i] = 0;
			}

//			if(crt_val[axix_index][i] < ten_val_range[axix_index][0] || crt_val[axix_index][i] > ten_val_range[axix_index][1]){
//				calibrate_en[axix_index] = 0;
//			}
//			//有效范围修改为2Kg - maxKg
//			if(crt_val[axix_index][i] < 20 || crt_val[axix_index][i] > tension_max_range){
//				calibrate_en[axix_index] = 0;
//			}
		}
	}
}

/**
 * 获取返回web端的ADC(包括crt, zero, base)。
 * 如：六线：crt@zero@base, 例如："55+55+56+34+44+55#55+55+56+34+44+55@0+9+3+4+5+3#0+0+2+3+4+3@55+55+56+34+44+55#55+55+56+34+44+55";
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
 * 设置自动校准时间，预留
 */
void setBaseAutoCalibrateTime(u16 value){
	base_auto_calibrate_time = value;
	WriteBaseAutoCalibrateTimeToEEProm();
}

