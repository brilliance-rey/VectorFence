#include "alarm.h"
#include "delay.h"
#include "timer.h"
#include "24cxx.h"
#include "led.h"
#include "lcd.h"
#include "adc.h"
#include "pcf8563.h"
#include "log.h"
//////////////////////////////////////////////////////////////////////////////////	 

u8 field_num[2] = { 0 };
u16 alarm_delay[2] = { ALARM_DURATION_DEF_S, ALARM_DURATION_DEF_S };    //默认30秒

//A,B防区拉力值范围，如：ten_val_range[0][0]为a防区下限，ten_val_range[0][0]为a防区上限, 单位：0.1Kg 如：20->2Kg
u16 ten_val_range[2][2] = { { 100, 150 }, { 100, 150 } };

//1:最灵敏（0.5s）10:最不灵敏（5s)
u8 alarm_sensitivity[2] = { 4, 4 };

/**
 * ALARM_STATE:
 * bit  15     14      13      12     11      10      09      08
 *      --     --      --     --      --      --    B_RLX    A_RLX
 *
 * bit  07      06      05      04      03      02      01      00
 *      --      SW2     SW1     AB_FCH  B_OPN   B_INVD  A_OPN   A_INVD
 */
volatile u16 ALARM_STATE = 0;

/** 延时告警状态，与警号标志同步，通过告警清除Remove_Alarm_RL_A(B)，进行清除。*/
volatile u16 ALARM_STATE_DELAY = 0;

//u16 ALARM_STATE_TEMP = 0;

u16 cur_Alarm_Type_Sum = 0;
u16 old_Alarm_Type_Sum = 0;
u8 flag_fcgjsn = OFF;		//default:防拆告警关闭
u8 flag_fcgjsn_tmp = OFF;	//default:防拆告警关闭

//a,b 防区入侵当前告警标志
static u8 adc_invd_crt[2] = { 0 };
//a,b 防区入侵当前告警标志
static u8 adc_open_crt[2] = { 0 };
//a,b 防区松弛当前告警标志
static u8 adc_relax_crt[2] = { 0 };

volatile u8 BU_FANG_A_FLAG;
volatile u8 BU_FANG_B_FLAG;

//入侵阈值上限相对差值，如：203 -> 20.3Kg
u16 alarm_threshold_up_dif;
//松弛阈值下限相对差值, 如：203 -> 20.3Kg
u16 alarm_threshold_down_dif;

u8 FCAlmVerifyCnt = 0;

u8 adcIvdAlmVerifyCnt[2] = { 0 };
u8 adcOpnAlmVerifyCnt[2] = { 0 };
u8 adcRlxAlmVerifyCnt[2] = { 0 };

s32 alarmingCnt[2] = { 0 };

u8 W4_W6 = IS_W4;

u8 BU_FANG_A_FLAG_TEMP;	//A防区布防开关暂存
u8 BU_FANG_B_FLAG_TEMP;	//B防区布防开关暂存

/**SW告警标记：0：OK, 1:Alarm*/
u8 SW_Alarm[2] = { 0 };

/** ["主动红外探测器","被动红外探测器","双鉴探测器","环境探测器","门磁","紧急按钮","水浸","其他设备"]; */
s8 SW_Type[2] = { 0 }; // 0："主动红外探测器"

u8 remote_shutdown = 0;

void updateSWAlarmState(u8 swNum);

void ALARM_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	u8 controlParaTmp;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOE, ENABLE);

	//告警开关量
	//	PE0（PIN_97）	O	RL_B	B防区告警开关量
	//	PE1（PIN_98）	O	RL_A	A防区告警开关量
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;	//普通输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	//上拉
	GPIO_Init(GPIOE, &GPIO_InitStructure);	//初始化GPIOA

	GPIO_ResetBits(GPIOE, GPIO_Pin_0 | GPIO_Pin_1);

	//防拆
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;	//普通输入模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//100M
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	//上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//4、6线检测	PE3（PIN_2）	I	W4/W6	0:W4；1:W6
	//单双防区检测	PE4（PIN_3）	I	S/D_FIELD	0:双防区；1:单防区
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//红外对射开关量
	//	红外对射接入端口1	PA11（PIN_70）	I	IR1	要求常闭输入 0—	正常1—	报警
	//	红外对射接入端口2	PA12（PIN_71）	I	IR2	要求常闭输入 	0—	正常1—	报警
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;			//普通输入模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;		//100M
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;			//上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	EXTIX_Init();

	//如果读到的数0xFF，则是出厂状态或default了。
	if (AT24CXX_ReadOneByte(FieldParaAddr + 1) == 0xFF || AT24CXX_ReadOneByte(FieldParaAddr + 2) == 0xFF)
	{ //default
		field_num[0] = 0;
		field_num[1] = 0;
		alarm_delay[0] = ALARM_DURATION_DEF_S;
		alarm_delay[1] = ALARM_DURATION_DEF_S;
		WriteFieldParaToEEProm();
	}
	else
	{
		ReadFieldParaFromEEProm();
	}

	//如果读到的数0xFF，则是出厂状态或default了。
	AT24CXX_Read(ControlParaAddr, &controlParaTmp, 1);
	if (controlParaTmp == 0xFF)
	{ //default
		flag_fcgjsn = ON;
		BU_FANG_A_FLAG = ON;
		BU_FANG_B_FLAG = FIELD_FLAG_S0D1 == 0 ? ON : OFF;
		WriteControlParaToEEProm();
	}
	else
	{
		ReadControlParaFromEEProm();
		if (FIELD_FLAG_S0D1 == 1)
		{	//如果是单防区
			BU_FANG_B_FLAG = OFF;
		}
	}

	LED_BU_FANG = BU_FANG_A_FLAG | BU_FANG_B_FLAG;

	//如果读到的数0xFF，则是出厂状态或default了。
	AT24CXX_Read(SWValAddr, &controlParaTmp, 1);
	if (controlParaTmp == 0xFF)
	{ //default
		WriteSWValToEEProm();
	}
	else
	{
		ReadSWValToEEProm();
	}
	//初始化获取开关量告警状态
	updateSWAlarmState(1);
	updateSWAlarmState(0);

	if (((config_code >> 1) & 0x01) == 0)
	{
		W4_W6 = IS_W4;
	}
	else
	{
		W4_W6 = IS_W6;
	}

	//如果读到的数0xFF，则是出厂状态或default了。
	ReadAlmThrdFromEEProm();
	if(alarm_threshold_up_dif == 0xffff){
		alarm_threshold_up_dif = ALARM_THRESHOLD_UP_DEF;
		WriteAlmThrdToEEProm();
	}
	if(alarm_threshold_down_dif == 0xffff){
		alarm_threshold_down_dif = ALARM_THRESHOLD_DOWN_DEF;
		WriteAlmThrdToEEProm();
	}

	TIM5_Int_Init(1000 - 1, 8400 - 1);	//告警响铃定时器。
}

void Alarm_RL_A(void)
{
	RL_A = 1;
	alarmingCnt[0] = 1;
}

void Remove_Alarm_RL_A(void)
{
	RL_A = 0;
	alarmingCnt[0] = -1;
	ALARM_STATE_DELAY = ALARM_STATE;
}

void Alarm_RL_B(void)
{
	RL_B = 1;
	alarmingCnt[1] = 1;
}

void Remove_Alarm_RL_B(void)
{
	RL_B = 0;
	alarmingCnt[1] = -1;
	ALARM_STATE_DELAY = ALARM_STATE;
}

void FangChai_Start(void)
{
	flag_fcgjsn = ON;
	WriteControlParaToEEProm();
}

void FangChai_Stop(void)
{
	flag_fcgjsn = OFF;
	WriteControlParaToEEProm();
}

/**
 * 设置告警状态同时设置ALARM_STATE_DELAY直到告警清除后恢复
 */
void Set_Alarm_State_Util_Clear(u8 mask){
	SET_ALARM_STATE(ALARM_STATE, mask);

	if(!IS_SET_BIT(config_code, 2)){
		SET_ALARM_STATE(ALARM_STATE_DELAY, mask);
	}
}

/**
 * 防拆检测   1:高电平机箱盖打开
 */
u8 FANG_CHAI_Check(void)
{
	u8 FANG_CHAI_last;
	u8 FANG_CHAI_crt;

	FANG_CHAI_last = GET_ALARM_STATE(ALARM_STATE, AB_FCH_MASK);

	if (flag_fcgjsn == ON)
	{
		FANG_CHAI_crt = FANG_CHAI;
		if (FANG_CHAI_crt != FANG_CHAI_last)
		{
			if (FANG_CHAI_crt == ALARM)
			{
				if (FCAlmVerifyCnt == 0)
				{
					FCAlmVerifyCnt = 1;
				}
				if (FCAlmVerifyCnt == 4)
				{	//3 = 4-1, 3秒确认
					Set_Alarm_State_Util_Clear(AB_FCH_MASK);
					WriteLog(LOG_TYPE_FC, ALARM);
					ALARM_Update();
					return ALARM;
				}
				else
				{
					return NO_ALARM;
				}
			}
			else
			{
				FCAlmVerifyCnt = 0;
				CLEAR_ALARM_STATE(ALARM_STATE, AB_FCH_MASK);
				//WriteLog(LOG_TYPE_FC, rtcTempStr, NO_ALARM);
				ALARM_Update();
				return NO_ALARM;
			}
		}
		else
		{
			return FANG_CHAI_last;
		}
	}
	else
	{
		FCAlmVerifyCnt = 0;
		CLEAR_ALARM_STATE(ALARM_STATE, AB_FCH_MASK);
		ALARM_Update();
		return NO_ALARM;
	}
}

/**
 * 每次告警响铃周期为默认30秒，30秒后仍然告警则继续30秒，以此类推。。。
 */
void ALARM_Update(void)
{
//	响铃方案见定时器5，见timer.c
	LED_FANG_CHAI = GET_ALARM_STATE(ALARM_STATE, AB_FCH_MASK);

	LED_OPEN = GET_ALARM_STATE(ALARM_STATE, A_OPN_MASK) | GET_ALARM_STATE(ALARM_STATE, B_OPN_MASK);
	LED_INVD = GET_ALARM_STATE(ALARM_STATE, A_INVD_MASK) | GET_ALARM_STATE(ALARM_STATE, B_INVD_MASK);
}

//布防开启A,B
void Push_AB_Start(void)
{
	Push_A_Start();
	Push_B_Start();
}

//布防关闭A,B
void Push_AB_Stop(void)
{
	Push_A_Stop();
	Push_B_Stop();
}

/**
 *A防区检测启动
 **/
void Push_A_Start(void)
{
	BU_FANG_A_FLAG = ON;
	LED_BU_FANG = 1;
	WriteControlParaToEEProm();
}

/**
 *A防区检测停止
 **/
void Push_A_Stop(void)
{
	BU_FANG_A_FLAG = OFF;
	LED_BU_FANG = BU_FANG_B_FLAG;
	WriteControlParaToEEProm();
}

/**
 *B防区检测启动
 **/
void Push_B_Start(void)
{
	BU_FANG_B_FLAG = ON;
	LED_BU_FANG = 1;
	WriteControlParaToEEProm();
}

void Push_B_Stop(void)
{
	BU_FANG_B_FLAG = OFF;
	LED_BU_FANG = BU_FANG_A_FLAG;
	WriteControlParaToEEProm();
}

/**
 * 设置入侵阈值上限相对差值
 */
void setAlmThrdUpDif(u16 value)
{
	alarm_threshold_up_dif = value;
	WriteAlmThrdToEEProm();
}

/**
 * 设置入侵阈值上限相对差值
 */
void setAlmThrdDwDif(u16 value)
{
	alarm_threshold_down_dif = value;
	WriteAlmThrdToEEProm();
}

void updateTensionParasByMaxValue(){
	u16 tension_max_value_per20 = (tension_max_range / 2) * 20 / 100;

	alarm_threshold_up_dif = tension_max_value_per20;
	alarm_threshold_down_dif = tension_max_value_per20;
	WriteAlmThrdToEEProm();

	ten_val_range[0][0] = ((tension_max_range / 2) - tension_max_value_per20 + 5) / 10 *10;
	ten_val_range[0][1] = ((tension_max_range / 2) + tension_max_value_per20 + 5) / 10 *10;

	ten_val_range[1][0] = ((tension_max_range / 2) - tension_max_value_per20 + 5) / 10 *10;
	ten_val_range[1][1] = ((tension_max_range / 2) + tension_max_value_per20 + 5) / 10 *10;

	WriteFieldParaToEEProm();
}

/**
 * ADC_Check_Handler之前多次检测。
 * field_index: 0:A防区， 1:B防区
 */
void ADC_Check(u8 field_index)
{
	u8 i;
	u8 line_count = (W4_W6 == IS_W4 ? 4 : 6);

	adc_invd_crt[field_index] = NO_ALARM;
	adc_open_crt[field_index] = NO_ALARM;
	adc_relax_crt[field_index] = NO_ALARM;

	//更新A防区状态

	updateCrtSmp(field_index);
	for (i = 0; i < line_count; i++)
	{
		//如果4/6线中有1路线超出上限，报入侵告警
		if (NO_ALARM == adc_invd_crt[field_index] && (crt_val[field_index][i] - base_val[field_index][i]) > alarm_threshold_up_dif)
		{
			adc_invd_crt[field_index] = ALARM;
		}

		//如果4/6线中有1路拉力值小于认为断线的最大拉力值，则报断线告警
		if (NO_ALARM == adc_open_crt[field_index] && (crt_val[field_index][i] < OPEN_VAL_MAX_KG))
		{
			adc_open_crt[field_index] = ALARM;
		}

		//未断线的情况下，再报松弛，如果4/6线中有1路拉力值小于下限，则报松弛告警
		if (NO_ALARM == adc_open_crt[field_index] &&
				NO_ALARM == adc_relax_crt[field_index] && (base_val[field_index][i] - crt_val[field_index][i]) > alarm_threshold_down_dif)
		{
			adc_relax_crt[field_index] = ALARM;
		}
	}
}

/**
 * A防区检测告警处理
 * field_index: 0:A防区， 1:B防区
 */
void ADC_Check_Handler(u8 field_index)
{
	//入侵上一次告警标志
	u8 adc_invd_last = GET_ALARM_STATE(ALARM_STATE, (field_index == 0? A_INVD_MASK : B_INVD_MASK));
	u8 adc_open_last = GET_ALARM_STATE(ALARM_STATE, (field_index == 0? A_OPN_MASK : B_OPN_MASK));
	u8 adc_relax_last = GET_ALARM_STATE(ALARM_STATE, (field_index == 0? A_RLX_MASK : B_RLX_MASK));

	ADC_Check(field_index);

	if ((0 == field_index && BU_FANG_A_FLAG == ON) || (1 == field_index && BU_FANG_B_FLAG == ON))
	{
		//  入侵检测
		if (adc_invd_crt[field_index] != adc_invd_last)
		{
			if (adc_invd_crt[field_index] == ALARM)
			{
				if (adcIvdAlmVerifyCnt[field_index] == 0)
				{
					adcIvdAlmVerifyCnt[field_index] = 1;
				}
				if (adcIvdAlmVerifyCnt[field_index] >= alarm_sensitivity[field_index])
				{	//如：3 = 4-1, 3秒确认
					Set_Alarm_State_Util_Clear(field_index == 0? A_INVD_MASK : B_INVD_MASK);
					WriteLog((field_index == 0 ? LOG_TYPE_AIVD : LOG_TYPE_BIVD), ALARM);
					ALARM_Update();
					//return;
				}
				else
				{
					//return;
				}
			}
			else
			{
				adcIvdAlmVerifyCnt[field_index] = 0;
				CLEAR_ALARM_STATE(ALARM_STATE, (field_index == 0? A_INVD_MASK : B_INVD_MASK));
				ALARM_Update();
				//return;
			}
		}

		//断线检测
		if (adc_open_crt[field_index] != adc_open_last)
		{
			if (adc_open_crt[field_index] == ALARM)
			{
				if (adcOpnAlmVerifyCnt[field_index] == 0)
				{
					adcOpnAlmVerifyCnt[field_index] = 1;
				}
				if (adcOpnAlmVerifyCnt[field_index] >= alarm_sensitivity[field_index])	//如：3 = 4-1, 3秒确认
				{
					Set_Alarm_State_Util_Clear(field_index == 0? A_OPN_MASK : B_OPN_MASK);
					WriteLog((field_index == 0 ? LOG_TYPE_AOPN : LOG_TYPE_BOPN), ALARM);
					ALARM_Update();
					//return;
				}
				else
				{
					//return;
				}
			}
			else
			{
				adcOpnAlmVerifyCnt[field_index] = 0;
				CLEAR_ALARM_STATE(ALARM_STATE, (field_index == 0? A_OPN_MASK : B_OPN_MASK));
				ALARM_Update();
				//return;
			}
		}

		//松弛检测
		if (adc_relax_crt[field_index] != adc_relax_last)
		{
			if (adc_relax_crt[field_index] == ALARM)
			{
				if (adcRlxAlmVerifyCnt[field_index] == 0)
				{
					adcRlxAlmVerifyCnt[field_index] = 1;
				}
				if (adcRlxAlmVerifyCnt[field_index] >= RELAX_VERIFIED_TIME_S)
				{	//如：3 = 4-1, 3秒确认
					Set_Alarm_State_Util_Clear(field_index == 0? A_RLX_MASK : B_RLX_MASK);
					ALARM_Update();
					//return;
				}
				else
				{
					//return;
				}
			}
			else
			{
				adcRlxAlmVerifyCnt[field_index] = 0;
				CLEAR_ALARM_STATE(ALARM_STATE, (field_index == 0? A_RLX_MASK : B_RLX_MASK));
				ALARM_Update();
				//return;
			}
		}
	}
	else
	{
		adcIvdAlmVerifyCnt[field_index] = 0;
		adcOpnAlmVerifyCnt[field_index] = 0;
		adcRlxAlmVerifyCnt[field_index] = 0;
		CLEAR_ALARM_STATE(ALARM_STATE, (field_index == 0? A_OPN_MASK : B_OPN_MASK));
		CLEAR_ALARM_STATE(ALARM_STATE, (field_index == 0? A_INVD_MASK : B_INVD_MASK));
		CLEAR_ALARM_STATE(ALARM_STATE, (field_index == 0? A_RLX_MASK : B_RLX_MASK));
		ALARM_Update();
		//return;
	}
}

void updateSWAlarmState(u8 swNum)
{
	SW_Alarm[swNum] = (swNum == 0 ? SW1 : SW2);
	if (SW_Type[swNum] != 0 && SW_Alarm[swNum])
	{
		Set_Alarm_State_Util_Clear(swNum == 0? SW1_MASK:SW2_MASK);
	}
	else
	{
		CLEAR_ALARM_STATE(ALARM_STATE, (swNum == 0? SW1_MASK:SW2_MASK));
	}
}

//外部中断8服务程序（红外对射接入端口1		PA8）
void EXTI9_5_IRQHandler(void)
{
	updateSWAlarmState(0);
	EXTI_ClearITPendingBit(EXTI_Line8);	//清除LINE8上的中断标志位
}

//外部中断12服务程序（红外对射接入端口2	PA12）
void EXTI15_10_IRQHandler(void)
{
	updateSWAlarmState(1);
	EXTI_ClearITPendingBit(EXTI_Line12);	//清除LINE12上的中断标志位
}

//外部中断初始化程序
//初始化PA8,PA12为中断输入.
void EXTIX_Init(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource8);	//PA8 连接到中断线8
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource12);	//PA12 连接到中断线12

	/* 配置EXTI_Line8 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line8 | EXTI_Line12;	//LINE8,12
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	//中断事件
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;	//(EXTI_Trigger_Rising:上升沿触发,	EXTI_Trigger_Falling:下降沿触发)
	EXTI_InitStructure.EXTI_LineCmd = ENABLE; //使能LINE8
	EXTI_Init(&EXTI_InitStructure); //配置

	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn; //外部中断8
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02; //抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01; //响应优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //使能外部中断通道
	NVIC_Init(&NVIC_InitStructure); //配置

	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn; //外部中断12
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02; //抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01; //响应优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //使能外部中断通道
	NVIC_Init(&NVIC_InitStructure); //配置
}

void setSWValue(u16 swVal)
{
	SW_Type[1] = (swVal >> 8) & 0xff;
	SW_Type[0] = swVal & 0xff;
	//更新告警状态
	updateSWAlarmState(1);
	updateSWAlarmState(0);

	WriteSWValToEEProm();
}

u8 existFieldNum(u8 fieldNum)
{
	if (fieldNum == field_num[0] || fieldNum == field_num[1])
	{
		return 1;
	}
	return 0;
}
void setFieldNum(u8 i, u8 fieldNum)
{
	field_num[i] = fieldNum;
	WriteFieldParaToEEProm();
}

void setAlarmDelay(u8 i, u16 almDly)
{
	alarm_delay[i] = almDly;
	WriteFieldParaToEEProm();
}

/**
 * 设置告警灵敏度
 * field_index: 防区序号： 0：A防区， 1：B防区
 * sensitivity 灵敏度，1-10： 0.5-5s
 */
void setAlarmSensitivity(u8 field_index, u8 sensitivity)
{
	alarm_sensitivity[field_index] = sensitivity;
	WriteFieldParaToEEProm();
}

/**
 * 设置拉力监控范围有效值。
 * field_index: 防区序号： 0：A防区， 1：B防区
 * highLowFlag: 0：下限值， 1：上限值
 */
void setTenValRange(u8 field_index, u8 highLowFlag, u16 tenVal)
{
	ten_val_range[field_index][highLowFlag] = tenVal;
	WriteFieldParaToEEProm();
}

void modifyFieldNum(u8 oldfieldNum, u8 newfieldNum)
{
	if (field_num[0] == oldfieldNum)
	{
		field_num[0] = newfieldNum;
	}
	else if (field_num[1] == oldfieldNum)
	{
		field_num[1] = newfieldNum;
	}
	WriteFieldParaToEEProm();
}

void modifyAlarmDelay(u8 fieldNum, u16 alarmDelay)
{
	if (field_num[0] == fieldNum)
	{
		alarm_delay[0] = alarmDelay;
	}
	else if (field_num[1] == fieldNum)
	{
		alarm_delay[1] = alarmDelay;
	}
	WriteFieldParaToEEProm();
}

u16 getAlarmDelay(u8 fieldNum)
{
	if (field_num[0] == fieldNum)
	{
		return alarm_delay[0];
	}
	else if (field_num[1] == fieldNum)
	{
		return alarm_delay[1];
	}
	else
	{
		return 0;
	}
}

void modifyBFFlag(u8 fieldNum, u16 bfFlag)
{
	if (field_num[0] == fieldNum)
	{
		bfFlag == 1 ? Push_A_Start() : Push_A_Stop();
	}
	else if (field_num[1] == fieldNum)
	{
		bfFlag == 1 ? Push_B_Start() : Push_B_Stop();
	}
}

u8 getBFFlag(u8 fieldNum)
{
	if (field_num[0] == fieldNum)
	{
		return BU_FANG_A_FLAG;
	}
	else if (field_num[1] == fieldNum)
	{
		return BU_FANG_B_FLAG;
	}
	else
	{
		return 0;
	}
}

u8 getAlarmStatus(u8 fieldNum)
{
	if (field_num[0] == fieldNum)
	{
		return ALARM_STATE & 0xF3 | ((ALARM_STATE >> 6) & 0x04);  //bit7-4:--,SW2,SW1,AB_FCH  bit1-0:A_RLX,A_OPN,A_IVD
	}
	else if (field_num[1] == fieldNum)
	{
		return (ALARM_STATE & 0xF0) | ((ALARM_STATE >> 2) & 0x03) | ((ALARM_STATE >> 7) & 0x04);  //bit7-4:--,SW2,SW1,AB_FCH  bit1-0:B_RLX,B_OPN,B_IVD
	}
	else
	{
		return 0;
	}
}



u8 getAlarmDelayedStatus(u8 fieldNum){
	if (field_num[0] == fieldNum) {
		return ALARM_STATE_DELAY & 0xF3 | ((ALARM_STATE_DELAY >> 6) & 0x04);  //bit7-4:--,SW2,SW1,AB_FCH  bit1-0:A_RLX,A_OPN,A_IVD
	} else if (field_num[1] == fieldNum) {
		return (ALARM_STATE_DELAY & 0xF0) | ((ALARM_STATE_DELAY >> 2) & 0x03) | ((ALARM_STATE_DELAY >> 7) & 0x04);  //bit7-4:--,SW2,SW1,AB_FCH  bit1-0:B_RLX,B_OPN,B_IVD
	}else{
		return 0;
	}
}

void removeAlarmRL(u8 fieldNum)
{
	if (field_num[0] == fieldNum)
	{
		Remove_Alarm_RL_A();
	}
	else if (field_num[1] == fieldNum)
	{
		Remove_Alarm_RL_B();
	}
}

u8 getPeerFieldNum(u8 fieldNum)
{
	if (field_num[0] == fieldNum)
	{
		return field_num[1];  //bit7-4:--,SW2,SW1,AB_FCH  bit1-0:A_OPN,A_IVD
	}
	else if (field_num[1] == fieldNum)
	{
		return field_num[0];
	}
	else
	{
		return 0;
	}
}

/**
 * 获取防区号对应的序号
 * filedNum:防区号
 * 返回：（0：A防区号，1：B防区号）
 */
u8 getFieldIndex(u8 fieldNum){
	if (field_num[0] == fieldNum)
	{
		return 0;
	}
	return 1;
}

void remoteShutdown(u8 shutdownFlag)
{
	remote_shutdown = shutdownFlag;
	if (remote_shutdown)
	{ //关闭防拆监控和布防
		flag_fcgjsn = OFF;

		BU_FANG_A_FLAG = OFF;
//		LED_BU_FANG = BU_FANG_B_FLAG;
//		if(LED_BU_FANG == OFF){
//			stopPWM();
//		}
		BU_FANG_B_FLAG = OFF;
//		LED_BU_FANG = BU_FANG_A_FLAG;
//		if(LED_BU_FANG == OFF){
//			stopPWM();
//		}

		LED_FANG_CHAI = 0;
		LED_OPEN = 0;
		LED_INVD = 0;
		LED_BU_FANG = 0;
		LED_RUN = 0;

		Lcd_Clear(FullScreen);    //清全屏
		LCD_BLA_OFF;
	}
	else
	{
		SystemReset();
	}
}

/**
 *
 * 获取防区A参数#防区B参数{防区A,B参数格式 :field_num+alarm_delay+alarm_sensitivity+ten_val_range[0]+ten_val_range[1]}
 */
void getWebFieldParas(char *fieldParas)
{
	sprintf((char*) fieldParas, "%d+%d+%d#%d+%d+%d", field_num[0], alarm_delay[0], alarm_sensitivity[0],/* ten_val_range[0][0], ten_val_range[0][1],*/
			field_num[1], alarm_delay[1], alarm_sensitivity[1]/*, ten_val_range[1][0], ten_val_range[1][1]*/);
}

