#include "timer.h"
#include "adc.h"
#include "led.h"
#include "lm75.h"
#include "pcf8563.h"
#include "alarm.h"
#include "log.h"
#include "lwip_comm.h"
#include "DisplayMenu.h"
#include "usart.h"
#include "includes.h"	 	//ucos 使用	  

////////////////////////////////////////////////////////////////////////////////// 	 
 
u8 lm75TempCnt = 0;
u8 rtcTimeCnt = 0;

s32 alarmCheckCnt = 0;
s32 adcCalibrateCnt = 0;

//自动校准计时器 a,b间隔60秒。
static volatile s32 baseAutoCalibrateCnt[2] = {0, 60};

/*************************************
 * 定时器更新标志，由main.c中的watch_task处理，  1有效。
 *
 * bit7: 读取温度（LM75_ReadTempStr）
 * bit6: 读时间 （PCF8563_Get_Str）
 * bit5: 检测网线热插拔。
 * bit4-3: reserved
 * bit2: calibrating();
 * bit1: B_ADC_Check_Handler
 * bit0: A_ADC_Check_Handler
 *
 */
u8 timerHandlerFlag = 0;

//通用定时器3中断初始化
//这里时钟选择为APB1的2倍，而APB1为42M
//arr：自动重装值。
//psc：时钟预分频数
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=定时器工作频率,单位:Mhz
//这里使用的是定时器3!
//	TIM3_Int_Init(5000-1,8400-1);	//定时器时钟84M，分频系数8400，所以84M/8400=10Khz的计数频率，计数5000次为500ms
void TIM3_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///使能TIM3时钟

  	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//自动重装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;

	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);

	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //允许定时器3更新中断
	TIM_Cmd(TIM3,ENABLE); //使能定时器3

	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //定时器3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x02; //抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x01; //子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

//定时器3中断服务程序	 
void TIM3_IRQHandler(void)
{
	u8 i;
	OSIntEnter();
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET)//溢出中断 
	{
//		//AD采样
//		Timer_Sample_Adc(ADC_Channel_12);
		
		LED_RUN = remote_shutdown? 0 : (!LED_RUN);

		run_screen_refresh ++;
		
		if((++lm75TempCnt) == 10){  //5秒更新一次
			lm75TempCnt = 0;
			SET_VAR_BIT(timerHandlerFlag, 7);	//			LM75_ReadTempStr(crtLM75TempStr);
		}

		for(i = 0; i < 2; i++){  //A区和B区
			if(adcIvdAlmVerifyCnt[i] > 0 && adcIvdAlmVerifyCnt[i] < (alarm_sensitivity[i] + 1)){  //入侵 alarm_sensitivity 秒确认
				adcIvdAlmVerifyCnt[i]++;
			}
			if(adcOpnAlmVerifyCnt[i] > 0 && adcOpnAlmVerifyCnt[i] < (alarm_sensitivity[i] + 1)){  //断线 alarm_sensitivity 秒确认
				adcOpnAlmVerifyCnt[i]++;
			}
		}

		if((++rtcTimeCnt) == 2){  //1秒更新一次
			rtcTimeCnt = 0;
			SET_VAR_BIT(timerHandlerFlag, 6);		//PCF8563_Get_Str(rtcTempStr);
			SET_VAR_BIT(timerHandlerFlag, 5);		//Eth_Link_ITHandler(); //支持启动未插网线和网线热插拔。

			if(FCAlmVerifyCnt > 0 && FCAlmVerifyCnt < (3 + 1)){  //防拆确认3秒
				FCAlmVerifyCnt++;
			}

			//A区和B区松弛单独计算
			for(i = 0; i < 2; i++){
				if(adcRlxAlmVerifyCnt[i] > 0 && adcRlxAlmVerifyCnt[i] < (RELAX_VERIFIED_TIME_S + 1)){  //松弛确认秒数
					adcRlxAlmVerifyCnt[i]++;
				}
			}
			//自动校准: （1.校准时间为0时不进行自动校准； 2.告警时不进行校准。
			if(base_auto_calibrate_time > 0){
				for(i = 0; i < 2; i++){
					baseAutoCalibrateCnt[i]++;
					if(baseAutoCalibrateCnt[i] >= (base_auto_calibrate_time * 60)){//自动校准
						if(calibrating_flag != 0xFF){ //如果正在校准，则回退CALIBRATING_TIME ms,
							baseAutoCalibrateCnt[i] -= CALIBRATING_TIME/1000;
						}else{
							  //A，B区无告警时校准
							if((BU_FANG_A_FLAG == ON && i == 0 && (ALARM_STATE & 0x0103) == 0)
									|| (BU_FANG_B_FLAG == ON && i == 1 && (ALARM_STATE & 0x020C) == 0)){
								baseAutoCalibrateCnt[i] = 0;
								updateBaseVal(i);
							}else{  //30秒后再检测，校准
								baseAutoCalibrateCnt[i] = (base_auto_calibrate_time * 60) - 60;
							}
						}
					}
				}
			}else{
				baseAutoCalibrateCnt[0] = 0;
				baseAutoCalibrateCnt[0] = 60;
			}
		}
	} 
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位  
	OSIntExit(); 	    		  			    
}


/**
* TIM4_Int_Init(50-1,84-1);	//84M/84=1Mhz的计数频率,计数50次为50us
**/
void TIM4_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);  ///使能TIM4时钟
	
  	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//自动重装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM4,&TIM_TimeBaseInitStructure);
	
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE); //允许定时器4更新中断
	TIM_Cmd(TIM4,ENABLE); //使能定时器4
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM4_IRQn; //定时器4中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x02; //抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x00; //子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);							 
}

//定时器4中断服务程序	 //10us定时器
void TIM4_IRQHandler(void)
{
	OSIntEnter();
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET) //溢出中断
	{
		if(calibrating_flag != 0xFF && (calibrating_flag & 80) != 0x80){
			if(adcCalibrateCnt >= CALIBRATING_TIME * 100) //CALIBRATING_TIME  ms
			{
				calibrating_flag |= 0x80;
				adcCalibrateCnt = 0;
			}
			else if(adcCalibrateCnt % 500 == 0){
				SET_VAR_BIT(timerHandlerFlag, 2);
			}
			adcCalibrateCnt++;
		}

		switch (alarmCheckCnt)
		{
			case 0:
				SET_VAR_BIT(timerHandlerFlag, 0);	//A_ADC_Check_Handler();
				break;
			case 25000:	//250ms
				SET_VAR_BIT(timerHandlerFlag, 1);	//B_ADC_Check_Handler();
				break;
			case 49999:	//500ms
				alarmCheckCnt = -1;
				break;
			default:
				break;
		}
		alarmCheckCnt++;
	}

	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);  //清除中断标志位
	OSIntExit();
}

//这里使用的是定时器5!
//	TIM5_Int_Init(1000-1,8400-1);	//定时器时钟84M，分频系数8400，所以84M/8400=10Khz的计数频率，计数1000次为100ms
void TIM5_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);  ///使能TIM5时钟

  	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//自动重装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;

	TIM_TimeBaseInit(TIM5,&TIM_TimeBaseInitStructure);

	TIM_ITConfig(TIM5,TIM_IT_Update,ENABLE); //允许定时器3更新中断
	TIM_Cmd(TIM5,ENABLE); //使能定时器5

	NVIC_InitStructure.NVIC_IRQChannel=TIM5_IRQn; //定时器5中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x02; //抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x01; //子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


/**
 * 定时器5中断服务程序  定时时间：100ms
 * 报警延时
 */
void TIM5_IRQHandler(void) {
	OSIntEnter();
	if (TIM_GetITStatus(TIM5, TIM_IT_Update ) == SET) { //溢出中断
		if (alarmingCnt[0] == -1)
		{
			if ((ALARM_STATE & (IS_SET_BIT(config_code, 3)? 0x13: 0x113)) != 0x00) //A区有告警，开始报警，松弛报警暂不响铃
			{
				Alarm_RL_A();
			}
		} else if (alarmingCnt[0] == alarm_delay[0] * 10)
		{  //默认30秒
			if ((ALARM_STATE & (IS_SET_BIT(config_code, 3)? 0x13: 0x113)) != 0x00)  //警号延时到，如果A区还有告警，继续报警， 松弛报警暂不响铃
			{
				Alarm_RL_A();
			} else		//警号延时到，如果A区告警消失，停止报警
			{
				Remove_Alarm_RL_A();
			}
		} else
		{
			alarmingCnt[0]++;
		}

		if (alarmingCnt[1] == -1)
		{
			if ((ALARM_STATE & (IS_SET_BIT(config_code, 3)? 0x1C: 0x21C)) != 0x00)
			{
				Alarm_RL_B();
			}
		} else if (alarmingCnt[1] == alarm_delay[1] * 10)
		{  //30秒
			if ((ALARM_STATE & (IS_SET_BIT(config_code, 3)? 0x1C: 0x21C)) != 0x00)
			{
				Alarm_RL_B();
			} else
			{
				Remove_Alarm_RL_B();
			}
		} else
		{
			alarmingCnt[1]++;
		}

	}
	TIM_ClearITPendingBit(TIM5, TIM_IT_Update );  //清除中断标志位
	OSIntExit();
}
