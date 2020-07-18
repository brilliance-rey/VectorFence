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
#include "includes.h"	 	//ucos ʹ��	  

////////////////////////////////////////////////////////////////////////////////// 	 
 
u8 lm75TempCnt = 0;
u8 rtcTimeCnt = 0;

s32 alarmCheckCnt = 0;
s32 adcCalibrateCnt = 0;

//�Զ�У׼��ʱ�� a,b���60�롣
static volatile s32 baseAutoCalibrateCnt[2] = {0, 60};

/*************************************
 * ��ʱ�����±�־����main.c�е�watch_task����  1��Ч��
 *
 * bit7: ��ȡ�¶ȣ�LM75_ReadTempStr��
 * bit6: ��ʱ�� ��PCF8563_Get_Str��
 * bit5: ��������Ȳ�Ρ�
 * bit4-3: reserved
 * bit2: calibrating();
 * bit1: B_ADC_Check_Handler
 * bit0: A_ADC_Check_Handler
 *
 */
u8 timerHandlerFlag = 0;

//ͨ�ö�ʱ��3�жϳ�ʼ��
//����ʱ��ѡ��ΪAPB1��2������APB1Ϊ42M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//��ʱ�����ʱ����㷽��:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=��ʱ������Ƶ��,��λ:Mhz
//����ʹ�õ��Ƕ�ʱ��3!
//	TIM3_Int_Init(5000-1,8400-1);	//��ʱ��ʱ��84M����Ƶϵ��8400������84M/8400=10Khz�ļ���Ƶ�ʣ�����5000��Ϊ500ms
void TIM3_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///ʹ��TIM3ʱ��

  	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;

	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);

	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //����ʱ��3�����ж�
	TIM_Cmd(TIM3,ENABLE); //ʹ�ܶ�ʱ��3

	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //��ʱ��3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x02; //��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x01; //�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

//��ʱ��3�жϷ������	 
void TIM3_IRQHandler(void)
{
	u8 i;
	OSIntEnter();
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET)//����ж� 
	{
//		//AD����
//		Timer_Sample_Adc(ADC_Channel_12);
		
		LED_RUN = remote_shutdown? 0 : (!LED_RUN);

		run_screen_refresh ++;
		
		if((++lm75TempCnt) == 10){  //5�����һ��
			lm75TempCnt = 0;
			SET_VAR_BIT(timerHandlerFlag, 7);	//			LM75_ReadTempStr(crtLM75TempStr);
		}

		for(i = 0; i < 2; i++){  //A����B��
			if(adcIvdAlmVerifyCnt[i] > 0 && adcIvdAlmVerifyCnt[i] < (alarm_sensitivity[i] + 1)){  //���� alarm_sensitivity ��ȷ��
				adcIvdAlmVerifyCnt[i]++;
			}
			if(adcOpnAlmVerifyCnt[i] > 0 && adcOpnAlmVerifyCnt[i] < (alarm_sensitivity[i] + 1)){  //���� alarm_sensitivity ��ȷ��
				adcOpnAlmVerifyCnt[i]++;
			}
		}

		if((++rtcTimeCnt) == 2){  //1�����һ��
			rtcTimeCnt = 0;
			SET_VAR_BIT(timerHandlerFlag, 6);		//PCF8563_Get_Str(rtcTempStr);
			SET_VAR_BIT(timerHandlerFlag, 5);		//Eth_Link_ITHandler(); //֧������δ�����ߺ������Ȳ�Ρ�

			if(FCAlmVerifyCnt > 0 && FCAlmVerifyCnt < (3 + 1)){  //����ȷ��3��
				FCAlmVerifyCnt++;
			}

			//A����B���ɳڵ�������
			for(i = 0; i < 2; i++){
				if(adcRlxAlmVerifyCnt[i] > 0 && adcRlxAlmVerifyCnt[i] < (RELAX_VERIFIED_TIME_S + 1)){  //�ɳ�ȷ������
					adcRlxAlmVerifyCnt[i]++;
				}
			}
			//�Զ�У׼: ��1.У׼ʱ��Ϊ0ʱ�������Զ�У׼�� 2.�澯ʱ������У׼��
			if(base_auto_calibrate_time > 0){
				for(i = 0; i < 2; i++){
					baseAutoCalibrateCnt[i]++;
					if(baseAutoCalibrateCnt[i] >= (base_auto_calibrate_time * 60)){//�Զ�У׼
						if(calibrating_flag != 0xFF){ //�������У׼�������CALIBRATING_TIME ms,
							baseAutoCalibrateCnt[i] -= CALIBRATING_TIME/1000;
						}else{
							  //A��B���޸澯ʱУ׼
							if((BU_FANG_A_FLAG == ON && i == 0 && (ALARM_STATE & 0x0103) == 0)
									|| (BU_FANG_B_FLAG == ON && i == 1 && (ALARM_STATE & 0x020C) == 0)){
								baseAutoCalibrateCnt[i] = 0;
								updateBaseVal(i);
							}else{  //30����ټ�⣬У׼
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
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //����жϱ�־λ  
	OSIntExit(); 	    		  			    
}


/**
* TIM4_Int_Init(50-1,84-1);	//84M/84=1Mhz�ļ���Ƶ��,����50��Ϊ50us
**/
void TIM4_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);  ///ʹ��TIM4ʱ��
	
  	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM4,&TIM_TimeBaseInitStructure);
	
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE); //����ʱ��4�����ж�
	TIM_Cmd(TIM4,ENABLE); //ʹ�ܶ�ʱ��4
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM4_IRQn; //��ʱ��4�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x02; //��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x00; //�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);							 
}

//��ʱ��4�жϷ������	 //10us��ʱ��
void TIM4_IRQHandler(void)
{
	OSIntEnter();
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET) //����ж�
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

	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);  //����жϱ�־λ
	OSIntExit();
}

//����ʹ�õ��Ƕ�ʱ��5!
//	TIM5_Int_Init(1000-1,8400-1);	//��ʱ��ʱ��84M����Ƶϵ��8400������84M/8400=10Khz�ļ���Ƶ�ʣ�����1000��Ϊ100ms
void TIM5_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);  ///ʹ��TIM5ʱ��

  	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;

	TIM_TimeBaseInit(TIM5,&TIM_TimeBaseInitStructure);

	TIM_ITConfig(TIM5,TIM_IT_Update,ENABLE); //����ʱ��3�����ж�
	TIM_Cmd(TIM5,ENABLE); //ʹ�ܶ�ʱ��5

	NVIC_InitStructure.NVIC_IRQChannel=TIM5_IRQn; //��ʱ��5�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x02; //��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x01; //�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


/**
 * ��ʱ��5�жϷ������  ��ʱʱ�䣺100ms
 * ������ʱ
 */
void TIM5_IRQHandler(void) {
	OSIntEnter();
	if (TIM_GetITStatus(TIM5, TIM_IT_Update ) == SET) { //����ж�
		if (alarmingCnt[0] == -1)
		{
			if ((ALARM_STATE & (IS_SET_BIT(config_code, 3)? 0x13: 0x113)) != 0x00) //A���и澯����ʼ�������ɳڱ����ݲ�����
			{
				Alarm_RL_A();
			}
		} else if (alarmingCnt[0] == alarm_delay[0] * 10)
		{  //Ĭ��30��
			if ((ALARM_STATE & (IS_SET_BIT(config_code, 3)? 0x13: 0x113)) != 0x00)  //������ʱ�������A�����и澯������������ �ɳڱ����ݲ�����
			{
				Alarm_RL_A();
			} else		//������ʱ�������A���澯��ʧ��ֹͣ����
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
		{  //30��
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
	TIM_ClearITPendingBit(TIM5, TIM_IT_Update );  //����жϱ�־λ
	OSIntExit();
}
