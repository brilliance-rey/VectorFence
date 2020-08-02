#include "includes.h" 
#include <string.h>
#include "malloc.h"  
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "adc.h"
#include "lcd.h"
#include "alarm.h"
#include "log.h"
#include "DisplayMenu.h"
#include "led.h"
#include "key.h"
#include "24cxx.h"
#include "lm75.h"
#include "pcf8563.h"
#include "lwip_comm.h"
#include "httpd.h"
#include "lan8720.h"
#include "RS485.h"

#include "timer.h"
#include "lwip/netif.h"

#include "lwipopts.h"
#include "tcp_server.h"
//#include "tcp_client.h"

/////////////////////////UCOSII��������///////////////////////////////////
//START ����
//�����������ȼ�
#define START_TASK_PRIO      			10 //��ʼ��������ȼ�����Ϊ���
//���������ջ��С
#define START_STK_SIZE  				64
//�����ջ��8�ֽڶ���	
__align(8) static OS_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *pdata);
 			   
////��������
////�����������ȼ�
//#define USART_TASK_PRIO       			7 
////���������ջ��С
//#define USART_STK_SIZE  		    	128
////�����ջ��8�ֽڶ���	
//__align(8) static OS_STK USART_TASK_STK[USART_STK_SIZE];
////������
//void usart_task(void *pdata);
//							 
//������
//�����������ȼ�
#define MAIN_TASK_PRIO       			6 
//���������ջ��С
#define MAIN_STK_SIZE  					1200
//�����ջ��8�ֽڶ���	
__align(8) static OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
//������
void main_task(void *pdata);

//����ɨ������
//�����������ȼ�
#define KEY_TASK_PRIO       			4
//���������ջ��С
#define KEY_STK_SIZE  					64
//�����ջ	
OS_STK KEY_TASK_STK[KEY_STK_SIZE];
//������
void key_task(void *pdata);
//////////////////////////////////////////////////////////////////////////////
OS_EVENT * msg_key;			//���������¼���ָ��
//////////////////////////////////////////////////////////////////////////////	 

//��������
//�����������ȼ�
#define WATCH_TASK_PRIO       			3
//���������ջ��С
#define WATCH_STK_SIZE  		   		256
//�����ջ��8�ֽڶ���	
__align(8) static OS_STK WATCH_TASK_STK[WATCH_STK_SIZE];
//������
void watch_task(void *pdata);
////////////////////////////////////////////////////////////////////////////////	 

//ϵͳ��ʼ��
void system_init(void)
{

	delay_init(168);		  //��ʼ����ʱ����

	LCD_Init(); 			//LCD��ʼ��
	DisplayInitMenu();//�ϵ��ʼ���˵�
	uart_init(9600);
	LED_Init();		        //��ʼ��LED�˿�
	KEY_Init();  			//������ʼ��
	
	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
//	my_mem_init(SRAMEX);		//��ʼ���ⲿ�ڴ��
	my_mem_init(SRAMCCM);	//��ʼ��CCM�ڴ��
	
	//ע��һ�´�����Ⱥ�˳��,��ȡEEPromһ��Ҫ��IIC_Init()��
	IIC_Init();		//IIC��ʼ��
 	while(AT24CXX_Check()){
		LED_RUN=1;
	}
	LED_RUN=0;
	//***ע�⣺ EEProm��ʼ��������Ҫ��ȡconfigCode.
	readConfigCode();
	LM75_ReadTempStr(crtLM75TempStr);
	PCF8563_Get_Str(rtcTempStr);
	Log_Init();
	Adc_Init();         //��ʼ��ADC
	ALARM_Init();
	
	TIM3_Int_Init(5000-1,8400-1);	//��ʱ��ʱ��84M����Ƶϵ��8400������84M/8400=10Khz�ļ���Ƶ�ʣ�����5000��Ϊ500ms     
	TIM4_Int_Init(10-1,84-1);	//84M/84=1Mhz�ļ���Ƶ��,����10��Ϊ10us
//	TIM1_PWM_Init(500-1,84-1);	//84M/84=1Mhz�ļ���Ƶ��,��װ��ֵ500������PWMƵ��Ϊ 1M/500=2Khz.

}

int main(void)
{ 	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2

	system_init();		//ϵͳ��ʼ�� 
 	OSInit();

	lwip_comm_init();
	
// 	while(lwip_comm_init()) 	//lwip��ʼ�� LwIP_Initһ��Ҫ��OSInit֮�������LWIP�̴߳���֮ǰ��ʼ��!!!!!!!!  
//	{
//		delay_ms(500);
//	}
//	httpd_init();
	
 	OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//������ʼ����
 	tcp_server_init();
// 	tcp_client_init();

	OSStart();	  						    
}

//��ʼ����
void start_task(void *pdata)
{
	u8 err;
    OS_CPU_SR cpu_sr=0;
	pdata = pdata;

	msg_key=OSMboxCreate((void*)0);	//������Ϣ����
	iicMutexSem = OSMutexCreate(IIC_MUTEX_PRIO, &err);		//��iicSem = OSSemCreate(1);

	OSStatInit();		//��ʼ��ͳ������.�������ʱ1��������	
	
	OS_ENTER_CRITICAL();//�����ٽ���(�޷����жϴ��)   
	 
//#if	LWIP_DHCP
//	lwip_comm_dhcp_creat();	//����DHCP����
//#endif
 	
 	OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);
 	
// 	OSTaskCreate(usart_task,(void *)0,(OS_STK*)&USART_TASK_STK[USART_STK_SIZE-1],USART_TASK_PRIO);
	OSTaskCreate(watch_task,(void *)0,(OS_STK*)&WATCH_TASK_STK[WATCH_STK_SIZE-1],WATCH_TASK_PRIO);
	
	OSTaskCreate(key_task,(void *)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);
   
	OSTaskSuspend(START_TASK_PRIO);	//������ʼ����.
	OS_EXIT_CRITICAL();	//�˳��ٽ���(���Ա��жϴ��)
}

//ָʾ��ͷ
unsigned char arrows[]={
		0x00,0x08,0x30,0xE0,0xC0,0x80,0x00,0x00,0x00,0x20,0x18,0x0E,0x07,0x03,0x01,0x00,
};
//������
void main_task(void *pdata)
{

	u32 key=0;
	static u32 no_key_num = 0;	
	u8 err;	
	unsigned char current_screen = InitScreen;//
	current_screen = RunScreen;//�ϵ�Ĭ�ϴ������н���
				    
	while(1)
	{
		key=(u32)OSMboxPend(msg_key,10,&err); 
		RS485_TX_EN;  
		//printf("hello world \n");
		if(key == KEY_NO_PRESS)
			{
				if(no_key_num < 50)
					{
						no_key_num++;
					}
			}
		else
			{
				no_key_num = 0;
			}
		if((no_key_num == 50) && (current_screen == RunScreen))//��������ʾ��ʱ����Լ5��û�а�������
			{
				if(0 == (IS_SET_BIT(config_code, 2)? ALARM_STATE: ALARM_STATE_DELAY))  //����޸澯���ص���ʾ��
					{
						
					}
				else
					{
						current_screen = AlarmTypeListScreen;//�и澯����ø澯��ʾ��
						row_point = 1;//��ָ��ָ��ڶ���
						DisplayAlarmTypeList();
					}
			}
		
		switch(current_screen)
		{
			case RunScreen://����������ʾ����
				if(run_screen_refresh >= 10)
					{
						DisplayRunMenu();
						run_screen_refresh = 0;
					}
				switch(key)
				{
					case KEY_ENTER_PRESS:
						current_screen = UserLoginScreen;//��ǰ��Ϊ�û���¼��ʾ��
						DisplayUserLogin();
						break;
					case KEY_LEFT_PRESS:
						Remove_Alarm_RL_A();
						break;

					case KEY_RIGHT_PRESS:
						Remove_Alarm_RL_B();
						break;

					default:
						break;
				}
				break;

			case UserLoginScreen:
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								current_screen = RunScreen;//�ص�������ʾ��
								strcpy(inputPassword,"      ");
								strcpy(display_xinghao," $$$$$");
								DisplayRunMenu();
								break;

							case KEY_ENTER_PRESS:
								current_screen = InputPasswordScreen;//��ǰ��Ϊ����������ʾ��
								strcpy(inputPassword,"0$$$$$");
								row_fx_pos = 0;//�����һλ������ʾ
								DisplayInputPassword();
								break;

							case KEY_UP_PRESS:
								UserId ? (UserId = SuperUser) : (UserId = OrdinaryUser);
								DisplayUserLogin();
								break;

							case KEY_DOWN_PRESS:
								UserId ? (UserId = SuperUser) : (UserId = OrdinaryUser);
								DisplayUserLogin();
								break;


							default:
								break;
						}
					}
				break;

			case InputPasswordScreen://����������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								current_screen = RunScreen;//�ص�������ʾ��
								strcpy(inputPassword,"0$$$$$");
								strcpy(display_xinghao," $$$$$");
								DisplayRunMenu();
								break;

							case KEY_ENTER_PRESS:
								switch(UserId)
								{
									case SuperUser:
										if(strcmp(inputPassword,lcd_super_password) == 0)
											{
												DisplaySuperUserRootMenu();
												row_point = 1;//��ָ��ָ��ڶ���
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
												current_screen = SuperUserRootScreen;//��ǰ��Ϊ�����û���Ŀ¼�˵�
											}
										else
											{
												DisplayPasswordError();
												current_screen = PasswordErrorScreen;//��ǰ��Ϊ���������ʾ��
											}
										break;
									case OrdinaryUser:
										if(strcmp(inputPassword,lcd_ordinary_password) == 0)
											{
												DisplayOrdinaryUserRootMenu();
												row_point = 1;//��ָ��ָ��ڶ���
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
												current_screen = OrdinaryUserRootScreen;//��ǰ��Ϊ��ͨ�û���Ŀ¼�˵�
											}
										else
											{
												DisplayPasswordError();
												current_screen = PasswordErrorScreen;//��ǰ��Ϊ���������ʾ��
											}
										break;

									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								inputPassword[row_fx_pos] ++;
//								display_xinghao[row_fx_pos] ++;
								if(inputPassword[row_fx_pos] > '9')
									{
										inputPassword[row_fx_pos] = '0';
//										display_xinghao[row_fx_pos] = '0';
									}
								DisplayInputPassword();//ֻ���ڱ༭��ǰҳ�棬�Ҳ���ҳʱˢ�µ�ǰҳ��
								break;

							case KEY_DOWN_PRESS:
								inputPassword[row_fx_pos] --;
//								display_xinghao[row_fx_pos] --;
								if(inputPassword[row_fx_pos] < '0')
									{
										inputPassword[row_fx_pos] = '9';
//										display_xinghao[row_fx_pos] = '9';
									}
								DisplayInputPassword();//ֻ���ڱ༭��ǰҳ�棬�Ҳ���ҳʱˢ�µ�ǰҳ��
								break;

							case KEY_LEFT_PRESS:
								row_fx_pos --;
								if(row_fx_pos < 0)
									{
										row_fx_pos = 5;
									}
								inputPassword[row_fx_pos] = '0';
//								display_xinghao[row_fx_pos] = '*';
								DisplayInputPassword();//ֻ���ڱ༭��ǰҳ�棬�Ҳ���ҳʱˢ�µ�ǰҳ��
								break;

							case KEY_RIGHT_PRESS:
								row_fx_pos ++;
								if(row_fx_pos > 5)
									{
										row_fx_pos = 0;
									}
								inputPassword[row_fx_pos] = '0';
//								display_xinghao[row_fx_pos] = '*';
								DisplayInputPassword();//ֻ���ڱ༭��ǰҳ�棬�Ҳ���ҳʱˢ�µ�ǰҳ��
								break;

							default:
								break;
						}
					}
				break;

			case PasswordErrorScreen://���������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
							case KEY_ENTER_PRESS:
								strcpy(inputPassword,"0$$$$$");
								strcpy(display_xinghao," $$$$$");
								row_fx_pos = 0;//�����һλ������ʾ
								DisplayInputPassword();
								current_screen = InputPasswordScreen;//�ص�����������ʾ��
								break;


							default:
								break;
						}
					}
				break;

			case SuperUserRootScreen://��ǰ��Ϊ�����û���Ŀ¼�˵�
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								DisplayUserLogin();
								current_screen = UserLoginScreen;//�ص��û���¼����
								strcpy(inputPassword,"0$$$$$");
								strcpy(display_xinghao," $$$$$");
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://�趨
										DisplaySetMenu();
										row_point = 1;//��ָ��ָ��ڶ���
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetScreen;//��ǰ��Ϊ�趨��ʾ��
										break;
									case 2://��ѯ
										DisplayInquireMenu();
										row_point = 1;//��ָ��ָ��ڶ���
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqScreen;//��ǰ��Ϊ��ѯ��ʾ��
										break;
									case 3://�������
										DisplayTensionMonitor();
										row_point = 1;//��ָ��ָ��ڶ���
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = TensionMonitorScreen;//��ǰ��Ϊ�������
										break;
									case 4://�汾��Ϣ
										DisplayVersionMenu();
										current_screen = VerScreen;//��ǰ��Ϊ�汾��Ϣ
										break;

									default:
										break;
								}
								break;
								
							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 4;
									}
								DisplaySuperUserRootMenu();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;
								
							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 4)
									{
										row_point = 1;
									}
								DisplaySuperUserRootMenu();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;
								
							
							default:
								break;
						}
					}
				break;

			case OrdinaryUserRootScreen://��ǰ��Ϊ��ͨ�û���Ŀ¼�˵�
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								DisplayUserLogin();
								current_screen = UserLoginScreen;//�ص��û���¼����
								strcpy(inputPassword,"0$$$$$");
								strcpy(display_xinghao," $$$$$");
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://��ѯ
										DisplayInquireMenu();
										row_point = 1;//��ָ��ָ��ڶ���
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqScreen;//��ǰ��Ϊ��ѯ��ʾ��
										break;
									case 2://�������
										DisplayTensionMonitor();
										row_point = 1;//��ָ��ָ��ڶ���
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = TensionMonitorScreen;//��ǰ��Ϊ�������
										break;
									case 3://�汾��Ϣ
										DisplayVersionMenu();
										current_screen = VerScreen;//
										break;
									
									default:
										break;
								}
								break;
								
							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 3;
									}
								DisplayOrdinaryUserRootMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;
								
							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 3)
									{
										row_point = 1;
									}
								DisplayOrdinaryUserRootMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;
								
							default:
								break;
						}
					}
				break;
			
			case SetScreen://�趨��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySuperUserRootMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SuperUserRootScreen;//�ص������û���Ŀ¼�˵�
								break;
							
							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://ϵͳ����
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetSysPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetSysScreen;//��ǰ��Ϊ�趨ϵͳ�����˵�
										break;
									case 2://��������
										row_point = 1;//��ָ��ָ��ڶ���
										DiaplaySetFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetFangQuParaScreen;//��ǰ��Ϊ�趨���������˵�
										break;
									case 3://У׼����
										row_point = 1;//��ָ��ָ��ڶ���
										DiaplayCalibrationSet();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = CalibrationSetScreen;//��ǰ��ΪУ׼���ò˵�
										break;
									case 4://��������
										row_point = 1;//��ָ��ָ��ڶ���
										DiaplaySetBuFangPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetBuFangParaScreen;//��ǰ��Ϊ�趨���������˵�
										break;
									case 5://�澯��ֵ
										row_point = 1;//��ָ��ָ��ڶ���
										DiaplaySetAlarmThreshold();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetAlarmThresholdScreen;//��ǰ��Ϊ�趨�澯��ֵ�����˵�
										break;
									case 6://�����¼
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetClearRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetClearRecordScreen;//��ǰ��Ϊ�����¼�˵�
										break;
									case 7://�����ⲿ�豸
										row_point = 1;//��ָ��ָ��ڶ���
										flag_PageState = NormalState;
										DiaplaySetExtDeviceTypeScreen();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//��ʾ��ָ��
										current_screen = SetExtDeviceTypeScreen;//��ǰ��Ϊ�趨����豸���Ͳ˵�
										break;
									case 8://����������ֵ
										sprintf(tension_max_range_temp,"%03d",tension_max_range / 10);
										row_fx_pos = 1;//��һ��λ�÷�����ʾ
										DisplaySetFullScale();
										current_screen = SetFullScaleScreen;//��ǰ��Ϊ��������������ֵ
										break;
									case 9://��׼��Уʱ��  
										sprintf(base_auto_calibrate_time_tmp,"%05d",base_auto_calibrate_time);
										row_fx_pos = 1;//��һ��λ�÷�����ʾ
										DisplaySetBaseAutoCalibrateTime();
										current_screen = SetBaseAutoCalibrateTimeScreen;//��ǰ��Ϊ��׼ֵ�Զ�У׼ʱ��
										break;
										
									default:
										break;
								}
								break;
							
							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 9;
									}
								DisplaySetMenu();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;
							
							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 9)
									{
										row_point = 1;
									}
								DisplaySetMenu();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqScreen://��ѯ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								if(UserId == SuperUser)
									{
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySuperUserRootMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SuperUserRootScreen;//�ص������û���Ŀ¼�˵�
									}
								else
									{
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayOrdinaryUserRootMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = OrdinaryUserRootScreen;//�ص���ͨ�û���Ŀ¼�˵�
									}
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://ϵͳ����
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayInqSysPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqSysParaScreen;//��ǰ��Ϊ��ѯϵͳ�����˵�
										break;
									case 2://��������
										row_point = 1;//��ָ��ָ��ڶ���
										DiaplayInqFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqFangQuParaScreen;//��ǰ��Ϊ��ѯ���������˵�
										break;
									case 3://��������
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayInqBuFangPara();
//										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqBuFangParaScreen;//��ǰ��Ϊ��ѯ���������˵�
										break;
									case 4://�澯��¼
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayInqAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqAlarmRecordScreen;//��ǰ��Ϊ��ѯ�澯��¼�˵�
										break;
									case 5://�澯��ֵ
										DisplayInqAlarmThreshold();
										current_screen = InqAlarmThresholdScreen;//��ǰ��Ϊ��ѯ�澯��ֵ�˵�
										break;
									case 6://У׼ֵ
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayInqCalibrationVal();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqCalibrationValScreen;//��ǰ��Ϊ��ѯУ׼ֵ�˵�
										break;
									case 7://�����ⲿ�豸
										DisplayInqExtDeviceType();
										current_screen = InqExtDeviceTypeScreen;//��ǰ��Ϊ��ѯ�����ⲿ�豸�˵�
										break;
									case 8://����������ֵ
										DisplayInqFullScale();
										current_screen = InqFullScaleScreen;//��ǰ��Ϊ��ѯ����������ֵ�˵�
										break;
									case 9://��׼ֵ�Զ�У׼ʱ��
										DisplayInqBaseAutoCalibrateTime();
										current_screen = InqBaseAutoCalibrateTimeScreen;//��ǰ��Ϊ��ѯ��׼ֵ�Զ�У׼ʱ��˵�
										break;
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 9;
									}
								DisplayInquireMenu();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 9)
									{
										row_point = 1;
									}
								DisplayInquireMenu();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			
			case TensionMonitorScreen://���������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								if(UserId == SuperUser)
									{
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySuperUserRootMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SuperUserRootScreen;//�ص������û���Ŀ¼�˵�
									}
								else
									{
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayOrdinaryUserRootMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = OrdinaryUserRootScreen;//�ص���ͨ�û���Ŀ¼�˵�
									}
								break;
								
							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A��
										DiaplayAFangQuMonitor();
										current_screen = AFangQuMonitorScreen;//��ǰ��ΪA����������ز˵�
										break;
									case 2://B��
										DiaplayBFangQuMonitor();
										current_screen = BFangQuMonitorScreen;//��ǰ��ΪB����������ز˵�
										break;
									default:
										break;
								}
								break;
								
							case KEY_UP_PRESS:
								row_point --;
								if(FIELD_FLAG_S0D1)
									{//����������ʾB����
										if(row_point < 1)
											{
												row_point = 1;
											}
									}
								else
									{
										if(row_point < 1)
											{
												row_point = 2;
											}
									}
								DisplayTensionMonitor();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;
								
							case KEY_DOWN_PRESS:
								row_point ++;
								if(FIELD_FLAG_S0D1)
									{//����������ʾB����
										if(row_point > 1)
											{
												row_point = 1;
											}
									}
								else
									{
										if(row_point > 2)
											{
												row_point = 1;
											}
									}
								DisplayTensionMonitor();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;
								
							case KEY_LEFT_PRESS:
								break;
								
							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			
			case VerScreen://�汾����ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								if(UserId == SuperUser)
									{
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySuperUserRootMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SuperUserRootScreen;//�ص������û���Ŀ¼�˵�
									}
								else
									{
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayOrdinaryUserRootMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = OrdinaryUserRootScreen;//�ص���ͨ�û���Ŀ¼�˵�
									}
								break;

							default:
								break;
						}
					}
				break;
			case SetSysScreen://�趨ϵͳ������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetScreen;//�ص��趨�˵�
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://����ʱ��
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetDateTime();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetDateTimeScreen;//��ǰ��Ϊ�趨����ʱ��˵�
										break;
									case 2://�������
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetEthPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetEthParaScreen;//��ǰ��Ϊ�趨��������˵�
										break;
//									case 3://RS485����
//										current_screen = SetRS485ParaScreen;//��ǰ��Ϊ�趨RS485�����˵�
//										row_point = 1;//��ָ��ָ��ڶ���
//										break;

									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 2;
									}
								DisplaySetSysPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 2)
									{
										row_point = 1;
									}
								DisplaySetSysPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;

			case SetFangQuParaScreen://�趨����������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetScreen;//�ص��趨�˵�
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A��
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetAFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetAFangQuParaScreen;//��ǰ��Ϊ����A���������˵�
										break;
									case 2://B��
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetBFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetBFangQuParaScreen;//��ǰ��Ϊ����B���������˵�
										break;
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(FIELD_FLAG_S0D1)
									{//����������ʾB����
										if(row_point < 1)
											{
												row_point = 1;
											}
									}
								else
									{
										if(row_point < 1)
											{
												row_point = 2;
											}
									}
								DiaplaySetFangQuPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(FIELD_FLAG_S0D1)
									{//����������ʾB����
										if(row_point > 1)
											{
												row_point = 1;
											}
									}
								else
									{
										if(row_point > 2)
											{
												row_point = 1;
											}
									}
								DiaplaySetFangQuPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case CalibrationSetScreen://У׼������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetScreen;//�ص��趨�˵�
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://���ֵУ׼
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayZeroCalibration();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ZeroCalibrationScreen;//��ǰ��Ϊ���ֵУ׼�˵�
										break;
									case 2://��׼ֵУ׼
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayDatumCalibration();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = DatumCalibrationScreen;//��ǰ��Ϊ��׼ֵУ׼�˵�
										break;

									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 2;
									}
								DiaplayCalibrationSet();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 2)
									{
										row_point = 1;
									}
								DiaplayCalibrationSet();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case SetBuFangParaScreen://�趨����������ʾ��
				if(key != KEY_NO_PRESS)
					{
						if(flag_PageState)//�ж�ҳ��״̬��������ʾʱ��
							{
								switch(key)
								{
									case KEY_ESC_PRESS://���˳���
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetScreen;//�ص��趨�˵�
										break;

									case KEY_ENTER_PRESS:
										BU_FANG_A_FLAG_TEMP = BU_FANG_A_FLAG;
										BU_FANG_B_FLAG_TEMP = BU_FANG_B_FLAG;
										flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
										switch(row_point)
										{
											case 1://A�����༭
												//A��������,���ѹ�ȼ�������ʾ���ɱ༭
												BU_FANG_A_FLAG_TEMP = BU_FANG_A_FLAG;//�������ݴ�
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												row_fx_pos = 1;//��һ��������ʾλ��
												break;
											case 2://B�����༭
												//B��������,���ѹ�ȼ�������ʾ���ɱ༭
												BU_FANG_B_FLAG_TEMP = BU_FANG_B_FLAG;//�������ݴ�
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												row_fx_pos = 1;//��һ��������ʾλ��
												break;
											case 3://����澯:����/�ر�
												//����澯���ط�����ʾ���ɱ༭
												flag_fcgjsn_tmp = flag_fcgjsn;//�������ݴ�
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												break;
											default:
												break;
										}
										DiaplaySetBuFangPara();
										break;

									case KEY_UP_PRESS:
										row_point --;
										if(FIELD_FLAG_S0D1)
											{//����������ʾB����
												if(row_point < 1)
													{
														row_point = 3;
													}
												if(row_point == 2)
													{
														row_point = 1;
													}
											}
										else
											{
												if(row_point < 1)
													{
														row_point = 3;
													}
											}
										DiaplaySetBuFangPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										break;

									case KEY_DOWN_PRESS:
										row_point ++;
										if(FIELD_FLAG_S0D1)
											{//����������ʾB����
												if(row_point > 3)
													{
														row_point = 1;
													}
												if(row_point == 2)
													{
														row_point = 3;
													}
											}
										else
											{
												if(row_point > 3)
													{
														row_point = 1;
													}
											}
										DiaplaySetBuFangPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										break;

									case KEY_LEFT_PRESS:
										break;

									case KEY_RIGHT_PRESS:
										break;
									default:
										break;
								}
							}
						else//�ж�ҳ��״̬���༭��ʾʱ��
							{
								//����ʾ��ָ��
								switch(key)
								{
									case KEY_ESC_PRESS://���˳���
										flag_PageState = NormalState;//ҳ��״̬�ɱ༭��ʾģʽת��Ϊ������ʾģʽ
										flag_fcgjsn_tmp = flag_fcgjsn;//ҳ��������ʾʱҪ��flag_fcgjsn��ֵ��flag_fcgjsn_tmp�������ٴν���༭��ʾ�᲻��
										BU_FANG_A_FLAG_TEMP = BU_FANG_A_FLAG;
										BU_FANG_B_FLAG_TEMP = BU_FANG_B_FLAG;
										DiaplaySetBuFangPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										break;

									case KEY_ENTER_PRESS:
										if(flag_fcgjsn != flag_fcgjsn_tmp)
											{
												flag_fcgjsn = flag_fcgjsn_tmp;
												flag_fcgjsn ? FangChai_Start():FangChai_Stop();
											}
										if(BU_FANG_A_FLAG != BU_FANG_A_FLAG_TEMP)
											{
												BU_FANG_A_FLAG = BU_FANG_A_FLAG_TEMP;
												BU_FANG_A_FLAG ? Push_A_Start():Push_A_Stop();
											}
										if(BU_FANG_B_FLAG != BU_FANG_B_FLAG_TEMP)
											{
												BU_FANG_B_FLAG = BU_FANG_B_FLAG_TEMP;
												BU_FANG_B_FLAG ? Push_B_Start():Push_B_Stop();
											}
										flag_PageState = NormalState;//ҳ��״̬�ɱ༭��ʾģʽת��Ϊ������ʾģʽ
										row_fx_pos = 0;//�޷�����ʾ����
										break;

									case KEY_UP_PRESS:
										//�ж��Ƿ������أ����Ǹ�ѹ�ȼ������Ƿ���澯����
										switch(row_point)
										{
											case 1://�༭��һ�У�A�������ؼ���ѹ�ȼ���
												switch(row_fx_pos)
												{
													case 1://A��������ѡ��
														if(BU_FANG_A_FLAG_TEMP == ON)
															{
																BU_FANG_A_FLAG_TEMP = OFF;
															}
														else
															{
																BU_FANG_A_FLAG_TEMP = ON;
															}
														break;
													default:
														break;
												}
												break;
											case 2://�༭�ڶ��У�B�������ؼ���ѹ�ȼ���
												switch(row_fx_pos)
												{
													case 1://B��������ѡ��
														if(BU_FANG_B_FLAG_TEMP == ON)
															{
																BU_FANG_B_FLAG_TEMP = OFF;
															}
														else
															{
																BU_FANG_B_FLAG_TEMP = ON;
															}
														break;
													default:
														break;
												}
												break;
											case 3://�༭�����У�����ʹ�ܿ��أ�
												if(flag_fcgjsn_tmp == ON)
													{
														flag_fcgjsn_tmp = OFF;
													}
												else
													{
														flag_fcgjsn_tmp = ON;
													}
												break;
											default:
												break;
										}
										DiaplaySetBuFangPara();
										break;

									case KEY_DOWN_PRESS:
										//�ж��Ƿ������أ����Ǹ�ѹ�ȼ������Ƿ���澯����
										switch(row_point)
										{
											case 1://�༭��һ�У�A�������ؼ���ѹ�ȼ���
												switch(row_fx_pos)
												{
													case 1://A��������ѡ��
														if(BU_FANG_A_FLAG_TEMP == OFF)
															{
																BU_FANG_A_FLAG_TEMP = ON;
															}
														else
															{
																BU_FANG_A_FLAG_TEMP = OFF;
															}
														break;
													default:
														break;
												}
												break;
											case 2://�༭�ڶ��У�B�������ؼ���ѹ�ȼ���
												switch(row_fx_pos)
												{
													case 1://B��������ѡ��
														if(BU_FANG_B_FLAG_TEMP == OFF)
															{
																BU_FANG_B_FLAG_TEMP = ON;
															}
														else
															{
																BU_FANG_B_FLAG_TEMP = OFF;
															}
														break;
													default:
														break;
												}
												break;
											case 3://�༭�����У�����ʹ�ܿ��أ�
												if(flag_fcgjsn_tmp == OFF)
													{
														flag_fcgjsn_tmp = ON;
													}
												else
													{
														flag_fcgjsn_tmp = OFF;
													}
												break;
											default:
												break;
										}
										DiaplaySetBuFangPara();
										break;

									case KEY_LEFT_PRESS:
										//���ױ༭����
										row_fx_pos --;
										if(row_fx_pos < 1)
											{
												row_fx_pos = 2;
											}
										DiaplaySetBuFangPara();
										break;

									case KEY_RIGHT_PRESS:
										//���ױ༭����
										row_fx_pos ++;
										if(row_fx_pos > 2)
											{
												row_fx_pos = 1;
											}
										DiaplaySetBuFangPara();
										break;
									default:
										break;
								}
							}
					}
				break;
			case SetAlarmThresholdScreen://�趨�澯��ֵ������ʾ��
				if(key != KEY_NO_PRESS)
					{
						if(flag_PageState)//�ж�ҳ��״̬��������ʾʱ��
							{
								switch(key)
								{
									case KEY_ESC_PRESS://���˳���
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetScreen;//�ص��趨�˵�
										break;
										
									case KEY_ENTER_PRESS:
										flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
										switch(row_point)
										{
											case 1://����ƫ��
												//����ƫ�Ʒ�����ʾ���ɱ༭
												alarm_threshold_up_dif_temp = alarm_threshold_up_dif;//�������ݴ�
												row_fx_pos = 1;//��һ��������ʾλ��
												break;
											case 2://����ƫ��
												//����ƫ�Ʒ�����ʾ���ɱ༭
												alarm_threshold_down_dif_temp = alarm_threshold_down_dif;//�������ݴ�
												row_fx_pos = 1;//��һ��������ʾλ��
												break;
											default:
												break;
										}
										DiaplaySetAlarmThreshold();
										break;
										
									case KEY_UP_PRESS:
										if(row_point > 1)
											{
												row_point --;
											}
										else
											{
												row_point = 2;
											}
										DiaplaySetAlarmThreshold();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										break;
										
									case KEY_DOWN_PRESS:
										if(row_point < 2)
											{
												row_point ++;
											}
										else
											{
												row_point = 1;
											}
										DiaplaySetAlarmThreshold();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										break;
										
									case KEY_LEFT_PRESS:
										break;
										
									case KEY_RIGHT_PRESS:
										break;
									default:
										break;
								}
							}
						else//�ж�ҳ��״̬���༭��ʾʱ��
							{
								//����ʾ��ָ��
								switch(key)
								{
									case KEY_ESC_PRESS://���˳���
										flag_PageState = NormalState;//ҳ��״̬�ɱ༭��ʾģʽת��Ϊ������ʾģʽ
										alarm_threshold_up_dif_temp = alarm_threshold_up_dif;
										alarm_threshold_down_dif_temp = alarm_threshold_down_dif;
										DiaplaySetAlarmThreshold();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										break;
										
									case KEY_ENTER_PRESS:
										//�������
										alarm_threshold_up_dif = alarm_threshold_up_dif_temp;
										alarm_threshold_down_dif = alarm_threshold_down_dif_temp;
										setAlmThrdUpDif(alarm_threshold_up_dif);
										setAlmThrdDwDif(alarm_threshold_down_dif);
										flag_PageState = NormalState;//ҳ��״̬�ɱ༭��ʾģʽת��Ϊ������ʾģʽ
										row_fx_pos = 0;//�޷�����ʾ����
										DiaplaySetAlarmThreshold();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//��ʾ��ָ��
										break;
										
									case KEY_UP_PRESS:
										switch(row_point)
										{
											case 1://�༭��һ�У�����ֵ��
												switch(row_fx_pos)
												{
													case 1://��ƫ�Ƶ�����λ
														if((alarm_threshold_up_dif_temp / 10) < 49)
															{
																alarm_threshold_up_dif_temp += 10;
															}
														else
															{
																alarm_threshold_up_dif_temp %= 10;
															}
														break;
													case 2://��ƫ�Ƶ�С��λ
														if((alarm_threshold_up_dif_temp % 10) < 9)
															{
																alarm_threshold_up_dif_temp ++;
															}
														else
															{
																alarm_threshold_up_dif_temp = (alarm_threshold_up_dif_temp / 10) * 10;
															}
														break;
														
													default:
														break;
												}
												break;
											case 2://�༭�ڶ��У�����ֵ��
												switch(row_fx_pos)
												{
													case 1://��ƫ�Ƶ�����λ
														if((alarm_threshold_down_dif_temp / 10) < 49)
															{
																alarm_threshold_down_dif_temp += 10;
															}
														else
															{
																alarm_threshold_down_dif_temp %= 10;
															}
														break;
													case 2://��ƫ�Ƶ�С��λ
														if((alarm_threshold_down_dif_temp % 10) < 9)
															{
																alarm_threshold_down_dif_temp ++;
															}
														else
															{
																alarm_threshold_down_dif_temp = (alarm_threshold_down_dif_temp / 10) * 10;
															}
														break;
														
													default:
														break;
												}
												break;
											default:
												break;
										}
										DiaplaySetAlarmThreshold();
										break;
										
									case KEY_DOWN_PRESS:
										switch(row_point)
										{
											case 1://�༭��һ�У���ƫ�ƣ�
												switch(row_fx_pos)
												{
													case 1://��ƫ�Ƶ�����λ
														if((alarm_threshold_up_dif_temp / 10) > 1)
															{
																alarm_threshold_up_dif_temp -= 10;
															}
														else
															{
																alarm_threshold_up_dif_temp = 490 + alarm_threshold_up_dif_temp % 10;
															}
														break;
													case 2://��ƫ�Ƶ�С��λ
														if((alarm_threshold_up_dif_temp % 10) > 0)
															{
																alarm_threshold_up_dif_temp --;
															}
														else
															{
																alarm_threshold_up_dif_temp = alarm_threshold_up_dif_temp + 9;
															}
														break;
														
													default:
														break;
												}
												break;
											case 2://�༭�ڶ��У���ƫ�ƣ�
												switch(row_fx_pos)
												{
													case 1://��ƫ�Ƶ�����λ
														if((alarm_threshold_down_dif_temp / 10) > 1)
															{
																alarm_threshold_down_dif_temp -= 10;
															}
														else
															{
																alarm_threshold_down_dif_temp = 490 + alarm_threshold_down_dif_temp % 10;
															}
														break;
													case 2://��ƫ�Ƶ�С��λ
														if((alarm_threshold_down_dif_temp % 10) > 0)
															{
																alarm_threshold_down_dif_temp --;
															}
														else
															{
																alarm_threshold_down_dif_temp = alarm_threshold_down_dif_temp + 9;
															}
														break;
														
													default:
														break;
												}
												break;
											default:
												break;
										}
										DiaplaySetAlarmThreshold();
										break;
										
									case KEY_LEFT_PRESS:
										//���ױ༭����
										row_fx_pos --;
										if(row_fx_pos < 1)
											{
												row_fx_pos = 2;
											}
										DiaplaySetAlarmThreshold();
										break;
										
									case KEY_RIGHT_PRESS:
										//���ױ༭����
										row_fx_pos ++;
										if(row_fx_pos > 2)
											{
												row_fx_pos = 1;
											}
										DiaplaySetAlarmThreshold();
										break;
										
									default:
										break;
								}
							}
					}
				break;
			case SetClearRecordScreen://�����¼��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetScreen;//�ص��趨�˵�
								break;
								
							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://��·�澯
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayClearRuQinAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ClearDuanLuAlarmRecordScreen;//�����·�澯��¼��ʾ��
										break;
									case 2://ƫ�Ƹ澯
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayClearDuanXianAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ClearDuanXianAlarmRecordScreen;//�����·�澯��¼��ʾ��
										break;
									case 3://����澯
										DisplayZhengZaiQingChu();
										//����EEPROM
										ClearLog(LOG_TYPE_FC);
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetClearRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetClearRecordScreen;//�ص������¼��ʾ��
										break;
									default:
										break;
								}
								break;
								
							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 3;
									}
								DisplaySetClearRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 3)
									{
										row_point = 1;
									}
								DisplaySetClearRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqSysParaScreen://��ѯϵͳ������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//�ص���ѯ��ʾ��
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://�������
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayInqEthPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqEthParaScreen;//��ǰ��Ϊ��ѯ���������ʾ�˵�
										break;

									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 1;
									}
								DisplayInqSysPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 1)
									{
										row_point = 1;
									}
								DisplayInqSysPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqFangQuParaScreen://��ѯ����������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//�ص���ѯ��ʾ��
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A��
										row_point = 1;//��ָ��ָ��ڶ���
										DiaplayInqAFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqAFangQuParaScreen;//��ǰ��Ϊ��ѯA���������˵�
										break;
									case 2://B��
										row_point = 1;//��ָ��ָ��ڶ���
										DiaplayInqBFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqBFangQuParaScreen;//��ǰ��Ϊ��ѯB���������˵�
										break;
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(FIELD_FLAG_S0D1)
									{//����������ʾB����
										if(row_point < 1)
											{
												row_point = 1;
											}
									}
								else
									{
										if(row_point < 1)
											{
												row_point = 2;
											}
									}
								DiaplayInqFangQuPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(FIELD_FLAG_S0D1)
									{//����������ʾB����
										if(row_point > 1)
											{
												row_point = 1;
											}
									}
								else
									{
										if(row_point > 2)
											{
												row_point = 1;
											}
									}
								DiaplayInqFangQuPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqBuFangParaScreen://��ѯ����������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//�ص���ѯ��ʾ��

								break;

							case KEY_ENTER_PRESS:
								
								break;

							case KEY_UP_PRESS:

								break;

							case KEY_DOWN_PRESS:

								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqAlarmRecordScreen://��ѯ�澯��¼��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//�ص���ѯ��ʾ��
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://��·�澯
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayInqRuQinAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqDuanLuAlarmScreen;//��ǰ��Ϊ��ѯ��·�澯�˵�
										break;
									case 2://��·�澯
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayInqDuanXianAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqDuanXianAlarmScreen;//��ǰ��Ϊ��ѯ��·�澯�˵�
										break;
									case 3://����澯
										row_point = 1;//��ָ��ָ��ڶ���
										ReadLCDFirstPageLog(LOG_TYPE_FC);
										DisplayInqFangChaiAlarmRecord();
//										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqFangChaiAlarmRecordScreen;//��ǰ��Ϊ��ѯ����澯�˵�
										break;
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 3;
									}
								DisplayInqAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 3)
									{
										row_point = 1;
									}
								DisplayInqAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqAlarmThresholdScreen://��ѯ�澯��ֵ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//�ص���ѯ��ʾ��
								break;

							case KEY_ENTER_PRESS:
								
								break;

							case KEY_UP_PRESS:
								
								break;

							case KEY_DOWN_PRESS:
								
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqCalibrationValScreen://��ѯУ׼ֵ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//�ص���ѯ��ʾ��
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://���ֵ
										DisplayInqZeroVal();//��ʾ���У׼ֵ
										row_point = 1;//��ָ��ָ��ڶ���
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqZeroValScreen;//��ǰ��Ϊ��ѯ���У׼ֵ�˵�
										break;
									case 2://��׼ֵ
										DiaplayInqDatumVal();//��ʾ��׼У׼ֵ
										row_point = 1;//��ָ��ָ��ڶ���
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqDatumValScreen;//��ǰ��Ϊ��ѯ��׼У׼ֵ�˵�
										break;
									
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 2;
									}
								DisplayInqCalibrationVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 2)
									{
										row_point = 1;
									}
								DisplayInqCalibrationVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqZeroValScreen://��ѯ���ֵ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqCalibrationVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqCalibrationValScreen;//�ص���ѯУ׼ֵ��ʾ��
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A����
										DisplayAareaZeroVal();//��ʾA�������У׼ֵ
										current_screen = InqAareaZeroValScreen;//��ǰ��Ϊ��ѯA�������ֵ��ʾ��
										break;
									case 2://B����
										DisplayBareaZeroVal();//��ʾB�������У׼ֵ
										current_screen = InqBareaZeroValScreen;//��ǰ��Ϊ��ѯB�������ֵ��ʾ��
										break;
									
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 2;
									}
								DisplayInqZeroVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 2)
									{
										row_point = 1;
									}
								DisplayInqZeroVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqDatumValScreen://��ѯ��׼ֵ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqCalibrationVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqCalibrationValScreen;//�ص���ѯУ׼ֵ��ʾ��
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A����
										DisplayAareaDatumVal();//��ʾA������׼У׼ֵ
										current_screen = InqAareaDatumValScreen;//��ǰ��Ϊ��ѯA������׼ֵ��ʾ��
										break;
									case 2://B����
										DisplayBareaDatumVal();//��ʾB������׼У׼ֵ
										current_screen = InqBareaDatumValScreen;//��ǰ��Ϊ��ѯB������׼ֵ��ʾ��
										break;
									
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 2;
									}
								DiaplayInqDatumVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 2)
									{
										row_point = 1;
									}
								DiaplayInqDatumVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqAareaZeroValScreen://��ѯA�������ֵ��ʾ��
			case InqBareaZeroValScreen://��ѯB�������ֵ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqZeroVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqZeroValScreen;//�ص���ѯ���ֵ��ʾ��
								break;

							case KEY_ENTER_PRESS:
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqZeroVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqZeroValScreen;//�ص���ѯ���ֵ��ʾ��
								break;

							case KEY_UP_PRESS:
								
								break;

							case KEY_DOWN_PRESS:
								
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqAareaDatumValScreen://��ѯA������׼ֵ��ʾ��
			case InqBareaDatumValScreen://��ѯB������׼ֵ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DiaplayInqDatumVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqDatumValScreen;//�ص���ѯ��׼ֵ��ʾ��
								break;

							case KEY_ENTER_PRESS:
								row_point = 1;//��ָ��ָ��ڶ���
								DiaplayInqDatumVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqDatumValScreen;//�ص���ѯ��׼ֵ��ʾ��
								break;

							case KEY_UP_PRESS:
								
								break;

							case KEY_DOWN_PRESS:
								
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			
			
			
			case InqExtDeviceTypeScreen://��ѯ�����ⲿ�豸��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//�ص���ѯ��ʾ��
								break;

							case KEY_ENTER_PRESS:
								
								break;

							case KEY_UP_PRESS:
								
								break;

							case KEY_DOWN_PRESS:
								
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			
			case InqFullScaleScreen://��ѯ����������ֵ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//�ص���ѯ��ʾ��
								break;

							case KEY_ENTER_PRESS:
								break;

							case KEY_UP_PRESS:
								break;

							case KEY_DOWN_PRESS:
								
								break;

							case KEY_LEFT_PRESS:
								
								break;

							case KEY_RIGHT_PRESS:
								
								break;
							default:
								break;
						}
					}
				break;
			
			case InqBaseAutoCalibrateTimeScreen://��ѯ��׼ֵ�Զ�У׼ʱ����ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//�ص���ѯ��ʾ��
								break;

							case KEY_ENTER_PRESS:
								break;

							case KEY_UP_PRESS:
								break;

							case KEY_DOWN_PRESS:
								
								break;

							case KEY_LEFT_PRESS:
								
								break;

							case KEY_RIGHT_PRESS:
								
								break;
							default:
								break;
						}
					}
				break;
			
			case AFangQuMonitorScreen://A�������������ʾ��
				DiaplayAFangQuMonitor();//��Ҫһֱˢ����ʾֵ
				delay_ms(500);
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayTensionMonitor();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = TensionMonitorScreen;//�ص����������ʾ��
								break;
							
							default:
								break;
						}
					}
				break;
			
			case BFangQuMonitorScreen://B�������������ʾ��
				DiaplayBFangQuMonitor();//��Ҫһֱˢ����ʾֵ
				delay_ms(500);
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayTensionMonitor();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = TensionMonitorScreen;//�ص����������ʾ��
								break;
							
							default:
								break;
						}
					}
				break;
			
			case SetDateTimeScreen://�趨����ʱ����ʾ��
				if(key != KEY_NO_PRESS)
					{
						if(flag_PageState)//�ж�ҳ��״̬��������ʾʱ��
							{
								switch(key)
								{
									case KEY_ESC_PRESS://���˳���
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetSysPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetSysScreen;//�ص��趨ϵͳ������ʾ��
										row_fx_pos = 0;//�޷�����ʾ����
										break;

									case KEY_ENTER_PRESS:
										switch(row_point)
										{
											case 1://��/��/��
												//�귴����ʾ���ɱ༭

												memcpy(rtcTempStr_temp_test,rtcTempStr,sizeof(rtcTempStr));//����ʵʱʱ�䵽ʱ���ݴ�
		//										memcpy(rtcTempStr_temp,rtcTempStr,sizeof(rtcTempStr));//����ʵʱʱ�䵽ʱ���ݴ�
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												row_fx_pos = 1;//��һ��������ʾλ��
												break;
											case 2://ʱ/��/��
												//ʱ������ʾ���ɱ༭
												memcpy(rtcTempStr_temp_test,rtcTempStr,sizeof(rtcTempStr));//����ʵʱʱ�䵽ʱ���ݴ�
		//										memcpy(rtcTempStr_temp,rtcTempStr,sizeof(rtcTempStr));//����ʵʱʱ�䵽ʱ���ݴ�
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												row_fx_pos = 1;//��һ��������ʾλ��
												break;

											default:
												break;
										}
										DisplaySetDateTime();
										break;

									case KEY_UP_PRESS:
										row_point --;
										if(row_point < 1)
											{
												row_point = 2;
											}
										DisplaySetDateTime();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//��ʾ��ָ��
										break;

									case KEY_DOWN_PRESS:
										row_point ++;
										if(row_point > 2)
											{
												row_point = 1;
											}
										DisplaySetDateTime();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//��ʾ��ָ��
										break;

									case KEY_LEFT_PRESS:
										break;

									case KEY_RIGHT_PRESS:
										break;
									default:
										break;
								}
							}
						else//�ж�ҳ��״̬���༭��ʾʱ��
							{
								//����ʾ��ָ��

								switch(key)
								{
									case KEY_ESC_PRESS://���˳���
										flag_PageState = NormalState;//ҳ��״̬�ɱ༭��ʾģʽת��Ϊ������ʾģʽ
										memcpy(rtcTempStr_temp_test,rtcTempStr,sizeof(rtcTempStr));//����ʵʱʱ���ַ�����ʱ���ݴ�
										row_fx_pos = 0;//�޷�����ʾ����
										DisplaySetDateTime();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//��ʾ��ָ��
										break;

									case KEY_ENTER_PRESS:
										//ȥ����PCF8563
										memcpy(rtcTempStr,rtcTempStr_temp_test,sizeof(rtcTempStr_temp_test));//����ʵʱʱ���ݴ��ַ�����ʱ��
										PCF8563_Set_Str(rtcTempStr);
										flag_PageState = NormalState;//ҳ��״̬�ɱ༭��ʾģʽת��Ϊ������ʾģʽ
										row_fx_pos = 0;//�޷�����ʾ����
										DisplaySetDateTime();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//��ʾ��ָ��
										break;

									case KEY_UP_PRESS:
										//�ж�������ʱ��������ϼ�
										switch(row_point)
										{
											case 1://�༭��һ��
												switch(row_fx_pos)
												{
													case 1://��һ�У������һ��λ�ã��꣩
														if(rtcTempStr_temp_test[2] < '9')
															{
																if(rtcTempStr_temp_test[3] < '9')
																	{
																		rtcTempStr_temp_test[3] ++;
																	}
																else
																	{
																		rtcTempStr_temp_test[2] ++;
																		rtcTempStr_temp_test[3] = '0';
																	}
															}
														else if(rtcTempStr_temp_test[2] == '9')
															{
																if(rtcTempStr_temp_test[3] < '9')
																	{
																		rtcTempStr_temp_test[3] ++;
																	}
																else
																	{
																		rtcTempStr_temp_test[2] = '1';
																		rtcTempStr_temp_test[3] = '0';
																	}
															}
														break;
													case 2://��һ�У�����ڶ���λ�ã��£�
														if(rtcTempStr_temp_test[5] < '1')
															{
																if(rtcTempStr_temp_test[6] < '9')
																	{
																		rtcTempStr_temp_test[6] ++;
																	}
																else
																	{
																		rtcTempStr_temp_test[5] ++;
																		rtcTempStr_temp_test[6] = '0';
																	}
															}
														else if(rtcTempStr_temp_test[5] == '1')
															{
																if(rtcTempStr_temp_test[6] < '2')
																	{
																		rtcTempStr_temp_test[6] ++;
																	}
																else
																	{
																		rtcTempStr_temp_test[5] = '0';
																		rtcTempStr_temp_test[6] = '1';
																	}
															}
														break;
													case 3://��һ�У����������λ�ã��գ�
														if(rtcTempStr_temp_test[8] < '3')
															{
																if(rtcTempStr_temp_test[9] < '9')
																	{
																		rtcTempStr_temp_test[9] ++;
																	}
																else
																	{
																		rtcTempStr_temp_test[8] ++;
																		rtcTempStr_temp_test[9] = '0';
																	}
															}
														else if(rtcTempStr_temp_test[8] == '3')
															{
																if(rtcTempStr_temp_test[9] < '1')
																	{
																		rtcTempStr_temp_test[9] ++;
																	}
																else
																	{
																		rtcTempStr_temp_test[8] = '0';
																		rtcTempStr_temp_test[9] = '1';
																	}
															}
														break;
													default:
														break;
												}
												break;
											case 2://�༭�ڶ���
												switch(row_fx_pos)
												{
													case 1://�ڶ��У������һ��λ�ã�ʱ��
														if(rtcTempStr_temp_test[11] < '2')
															{
																if(rtcTempStr_temp_test[12] < '9')
																	{
																		rtcTempStr_temp_test[12] ++;
																	}
																else
																	{
																		rtcTempStr_temp_test[11] ++;
																		rtcTempStr_temp_test[12] = '0';
																	}
															}
														else if(rtcTempStr_temp_test[11] == '2')
															{
																if(rtcTempStr_temp_test[12] < '3')
																	{
																		rtcTempStr_temp_test[12] ++;
																	}
																else
																	{
																		rtcTempStr_temp_test[11] = '0';
																		rtcTempStr_temp_test[12] = '0';
																	}
															}
														break;
													case 2://�ڶ��У�����ڶ���λ�ã��֣�
														if(rtcTempStr_temp_test[14] < '5')
															{
																if(rtcTempStr_temp_test[15] < '9')
																	{
																		rtcTempStr_temp_test[15] ++;
																	}
																else
																	{
																		rtcTempStr_temp_test[14] ++;
																		rtcTempStr_temp_test[15] = '0';
																	}
															}
														else if(rtcTempStr_temp_test[14] == '5')
															{
																if(rtcTempStr_temp_test[15] < '9')
																	{
																		rtcTempStr_temp_test[15] ++;
																	}
																else
																	{
																		rtcTempStr_temp_test[14] = '0';
																		rtcTempStr_temp_test[15] = '0';
																	}
															}
														break;
													case 3://�ڶ��У����������λ�ã��룩
														if(rtcTempStr_temp_test[17] < '5')
															{
																if(rtcTempStr_temp_test[18] < '9')
																	{
																		rtcTempStr_temp_test[18] ++;
																	}
																else
																	{
																		rtcTempStr_temp_test[17] ++;
																		rtcTempStr_temp_test[18] = '0';
																	}
															}
														else if(rtcTempStr_temp_test[17] == '5')
															{
																if(rtcTempStr_temp_test[18] < '9')
																	{
																		rtcTempStr_temp_test[18] ++;
																	}
																else
																	{
																		rtcTempStr_temp_test[17] = '0';
																		rtcTempStr_temp_test[18] = '0';
																	}
															}
														break;
													default:
														break;
												}
												break;
											default:
												break;
										}
										DisplaySetDateTime();
										break;

									case KEY_DOWN_PRESS:
										//�ж�������ʱ��������¼�
										switch(row_point)
										{
											case 1://�༭��һ��
												switch(row_fx_pos)
												{
													case 1://��һ�У������һ��λ�ã��꣩
														if(rtcTempStr_temp_test[2] > '1')
															{
																if(rtcTempStr_temp_test[3] > '0')
																	{
																		rtcTempStr_temp_test[3] --;
																	}
																else
																	{
																		rtcTempStr_temp_test[2] --;
																		rtcTempStr_temp_test[3] = '9';
																	}
															}
														else if(rtcTempStr_temp_test[2] == '1')
															{
																if(rtcTempStr_temp_test[3] > '0')
																	{
																		rtcTempStr_temp_test[3] --;
																	}
																else
																	{
																		rtcTempStr_temp_test[2] = '9';
																		rtcTempStr_temp_test[3] = '9';
																	}
															}
														break;
													case 2://��һ�У�����ڶ���λ�ã��£�
														if(rtcTempStr_temp_test[5] > '0')
															{
																if(rtcTempStr_temp_test[6] > '0')
																	{
																		rtcTempStr_temp_test[6] --;
																	}
																else
																	{
																		rtcTempStr_temp_test[5] --;
																		rtcTempStr_temp_test[6] = '9';
																	}
															}
														else if(rtcTempStr_temp_test[5] == '0')
															{
																if(rtcTempStr_temp_test[6] > '1')
																	{
																		rtcTempStr_temp_test[6] --;
																	}
																else
																	{
																		rtcTempStr_temp_test[5] = '1';
																		rtcTempStr_temp_test[6] = '2';
																	}
															}
														break;
													case 3://��һ�У����������λ�ã��գ�
														if(rtcTempStr_temp_test[8] > '0')
															{
																if(rtcTempStr_temp_test[9] > '0')
																	{
																		rtcTempStr_temp_test[9] --;
																	}
																else
																	{
																		rtcTempStr_temp_test[8] --;
																		rtcTempStr_temp_test[9] = '9';
																	}
															}
														else if(rtcTempStr_temp_test[8] == '0')
															{
																if(rtcTempStr_temp_test[9] > '1')
																	{
																		rtcTempStr_temp_test[9] --;
																	}
																else
																	{
																		rtcTempStr_temp_test[8] = '3';
																		rtcTempStr_temp_test[9] = '1';
																	}
															}
														break;
													default:
														break;
												}
												break;
											case 2://�༭�ڶ���
												switch(row_fx_pos)
												{
													case 1://�ڶ��У������һ��λ�ã�ʱ��
														if(rtcTempStr_temp_test[11] > '0')
															{
																if(rtcTempStr_temp_test[12] > '0')
																	{
																		rtcTempStr_temp_test[12] --;
																	}
																else
																	{
																		rtcTempStr_temp_test[11] --;
																		rtcTempStr_temp_test[12] = '9';
																	}
															}
														else if(rtcTempStr_temp_test[11] == '0')
															{
																if(rtcTempStr_temp_test[12] > '0')
																	{
																		rtcTempStr_temp_test[12] --;
																	}
																else
																	{
																		rtcTempStr_temp_test[11] = '2';
																		rtcTempStr_temp_test[12] = '3';
																	}
															}
														break;
													case 2://�ڶ��У�����ڶ���λ�ã��֣�
														if(rtcTempStr_temp_test[14] > '0')
															{
																if(rtcTempStr_temp_test[15] > '0')
																	{
																		rtcTempStr_temp_test[15] --;
																	}
																else
																	{
																		rtcTempStr_temp_test[14] --;
																		rtcTempStr_temp_test[15] = '9';
																	}
															}
														else if(rtcTempStr_temp_test[14] == '0')
															{
																if(rtcTempStr_temp_test[15] > '0')
																	{
																		rtcTempStr_temp_test[15] --;
																	}
																else
																	{
																		rtcTempStr_temp_test[14] = '5';
																		rtcTempStr_temp_test[15] = '9';
																	}
															}
														break;
													case 3://�ڶ��У����������λ�ã��룩
														if(rtcTempStr_temp_test[17] > '0')
															{
																if(rtcTempStr_temp_test[18] > '0')
																	{
																		rtcTempStr_temp_test[18] --;
																	}
																else
																	{
																		rtcTempStr_temp_test[17] --;
																		rtcTempStr_temp_test[18] = '9';
																	}
															}
														else if(rtcTempStr_temp_test[17] == '0')
															{
																if(rtcTempStr_temp_test[18] > '0')
																	{
																		rtcTempStr_temp_test[18] --;
																	}
																else
																	{
																		rtcTempStr_temp_test[17] = '5';
																		rtcTempStr_temp_test[18] = '9';
																	}
															}
														break;
													default:
														break;
												}
												break;
											default:
												break;
										}
										DisplaySetDateTime();
										break;

									case KEY_LEFT_PRESS:
										//���ױ༭����
										row_fx_pos --;
										if(row_fx_pos < 1)
											{
												row_fx_pos = 3;
											}
										DisplaySetDateTime();
										break;

									case KEY_RIGHT_PRESS:
										//���ױ༭����
										row_fx_pos ++;
										if(row_fx_pos > 3)
											{
												row_fx_pos = 1;
											}
										DisplaySetDateTime();
										break;
									default:
										break;
								}
							}
					}
				break;
			case SetEthParaScreen://�趨���������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetSysPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetSysScreen;//�ص��趨ϵͳ������ʾ��
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://MAC��ַ
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetMacAddr();
										MacAddrInsertMaohao();
										current_screen = SetMacAddrScreen;//��ǰ��Ϊ�趨Mac��ַ�˵�
										break;
									case 2://IP��ַ
										GetIpAddr();
										row_point = 1;//��ָ��ָ��ڶ���
										row_fx_pos = 1;//��һ��λ�÷�����ʾ
										DisplaySetIpAddr();
										current_screen = SetIpAddrScreen;//��ǰ��Ϊ�趨IP��ַ�˵�
										break;
									case 3://��������
										GetSubNetMask();
										row_point = 1;//��ָ��ָ��ڶ���
										row_fx_pos = 1;//��һ��λ�÷�����ʾ
										DisplaySetSubnetMask();
										current_screen = SetSubnetMaskScreen;//��ǰ��Ϊ�趨��������˵�
										break;
									case 4://����
										GetGateWay();
										row_point = 1;//��ָ��ָ��ڶ���
										row_fx_pos = 1;//��һ��λ�÷�����ʾ
										DisplaySetGateWay();
										current_screen = SetGateWayScreen;//��ǰ��Ϊ�趨���ز˵�
										break;
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 4;
									}
								DisplaySetEthPara();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 4)
									{
										row_point = 1;
									}
								DisplaySetEthPara();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case SetAFangQuParaScreen://�趨A����������ʾ��
				if(key != KEY_NO_PRESS)
					{
						if(flag_PageState)//�ж�ҳ��״̬��������ʾʱ��
							{
								switch(key)
								{
									case KEY_ESC_PRESS://���˳���
										row_point = 1;//��ָ��ָ��ڶ���
										DiaplaySetFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetFangQuParaScreen;//�ص��趨����������ʾ��
										row_fx_pos = 0;//�޷�����ʾ����
										break;

									case KEY_ENTER_PRESS:
										memcpy(ten_val_range_temp,ten_val_range,sizeof(ten_val_range_temp));//������Ч������Χ���ݴ棨����д���˴�����Ȼ��ʾ���ᱨ�Ƿ�����
										memcpy(alarm_sensitivity_temp,alarm_sensitivity,sizeof(alarm_sensitivity_temp));//���������ȵ��ݴ棨����д���˴�����Ȼ��ʾ���ᱨ�Ƿ�����
										switch(row_point)
										{
											case 1://������
												//�����ŷ�����ʾ���ɱ༭
												memcpy(field_num_temp,field_num,sizeof(field_num_temp));//���������ŵ��ݴ�
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												row_fx_pos = 1;//��һ��������ʾλ��
												break;
											case 2://�澯��ʱ
												//�澯��ʱ������ʾ���ɱ༭
												memcpy(alarm_delay_temp,alarm_delay,sizeof(alarm_delay_temp));//�����澯��ʱ���ݴ�
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												row_fx_pos = 1;//��һ��������ʾλ��
												break;
											case 3://������
												//�����ȷ�����ʾ���ɱ༭
//												memcpy(alarm_sensitivity_temp,alarm_sensitivity,sizeof(alarm_sensitivity_temp));//���������ȵ��ݴ�
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												row_fx_pos = 1;//��һ��������ʾλ��
												break;
											
											default:
												break;
										}
										DisplaySetAFangQuPara();
										break;
										
									case KEY_UP_PRESS:
										row_point --;
										if(row_point < 1)
											{
												row_point = 3;
											}
										DisplaySetAFangQuPara();
										if(row_point > 3)//���ѡ�����3��
											{
												Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
											}
										else
											{
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
											}
										break;
										
									case KEY_DOWN_PRESS:
										row_point ++;
										if(row_point > 3)
											{
												row_point = 1;
											}
										DisplaySetAFangQuPara();
										if(row_point > 3)//���ѡ�����3��
											{
												Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
											}
										else
											{
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
											}
										break;
										
									case KEY_LEFT_PRESS:
										break;
										
									case KEY_RIGHT_PRESS:
										break;
									default:
										break;
								}
							}
						else//�ж�ҳ��״̬���༭��ʾʱ��
							{
								//����ʾ��ָ��
								switch(key)
								{
									case KEY_ESC_PRESS://���˳���
										flag_PageState = NormalState;//ҳ��״̬�ɱ༭��ʾģʽת��Ϊ������ʾģʽ
										row_fx_pos = 0;//�޷�����ʾ����
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetAFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//��ʾ��ָ��
										break;

									case KEY_ENTER_PRESS:
										//
										memcpy(field_num,field_num_temp,sizeof(field_num_temp));//�����������ݴ浽������
										memcpy(alarm_delay,alarm_delay_temp,sizeof(alarm_delay_temp));//�����澯��ʱ�ݴ浽�澯��ʱ
										memcpy(alarm_sensitivity,alarm_sensitivity_temp,sizeof(alarm_sensitivity_temp));//�����������ݴ浽������
//										if(ten_val_range_temp[0][0] < ten_val_range_temp[0][1])
//											{
//												memcpy(ten_val_range[0],ten_val_range_temp[0],sizeof(ten_val_range_temp[0]));//������Ч������Χ�ݴ浽��Ч������Χ
//												WriteFieldParaToEEProm();
//											}
//										else
//											{
//												DisplayiLlegalInput();
//												delay_ms(1000);
//											}
										setFieldNum(0,field_num[0]);
										setAlarmDelay(0,alarm_delay[0]);
										WriteFieldParaToEEProm();
										flag_PageState = NormalState;//ҳ��״̬�ɱ༭��ʾģʽת��Ϊ������ʾģʽ
										row_fx_pos = 0;//�޷�����ʾ����
										DisplaySetAFangQuPara();
										if(row_point > 3)//�����ָ�����3�У���ʼ�ձ��������һ��
											{
												Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
											}
										else
											{
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
											}
										break;
										
									case KEY_UP_PRESS:
										//�жϷ����ź͸澯��ʱ���ϼ�
										switch(row_point)
										{
											case 1://�༭��һ�У�������
												switch(row_fx_pos)
												{
													case 1://��һ������λ��
														if(field_num_temp[0] < 80)
															{
																field_num_temp[0] ++;
															}
														else
															{
																field_num_temp[0] = 1;
															}
														break;

													default:
														break;
												}
												break;
											case 2://�༭�ڶ��У��澯��ʱ
												switch(row_fx_pos)
												{
													case 1://��һ������λ��
														if(alarm_delay_temp[0] < 999)
															{
																alarm_delay_temp[0] ++;
															}
														else
															{
																alarm_delay_temp[0] = 1;
															}
														break;

													default:
														break;
												}
												break;
											case 3://�༭�����У�������
												switch(row_fx_pos)
												{
													case 1://��һ������λ��
														if(alarm_sensitivity_temp[0] < 10)
															{
																alarm_sensitivity_temp[0] ++;
															}
														else
															{
																alarm_sensitivity_temp[0] = 1;
															}
														break;
													
													default:
														break;
												}
												break;
											
											default:
												break;
										}
										DisplaySetAFangQuPara();
										break;
										
									case KEY_DOWN_PRESS:
										//�жϷ����ź͸澯��ʱ���¼�
										switch(row_point)
										{
											case 1://�༭��һ�У�������
												switch(row_fx_pos)
												{
													case 1://��һ������λ��
														if(field_num_temp[0] > 1)
															{
																field_num_temp[0] --;
															}
														else
															{
																field_num_temp[0] = 80;
															}
														break;

													default:
														break;
												}
												break;
											case 2://�༭�ڶ���
												switch(row_fx_pos)
												{
													case 1://��һ������λ��
														if(alarm_delay_temp[0] > 1)
															{
																alarm_delay_temp[0] --;
															}
														else
															{
																alarm_delay_temp[0] = 999;
															}
														break;
													
													default:
														break;
												}
												break;
											case 3://�༭�����У�������
												switch(row_fx_pos)
												{
													case 1://��һ������λ��
														if(alarm_sensitivity_temp[0] > 1)
															{
																alarm_sensitivity_temp[0] --;
															}
														else
															{
																alarm_sensitivity_temp[0] = 10;
															}
														break;
													
													default:
														break;
												}
												break;
											default:
												break;
										}
										DisplaySetAFangQuPara();
										break;
										
									case KEY_LEFT_PRESS:
										//���ױ༭����
										row_fx_pos --;
										if(row_fx_pos < 1)
											{
												row_fx_pos = 2;
											}
										DisplaySetAFangQuPara();
										break;
										
									case KEY_RIGHT_PRESS:
										//���ױ༭����
										row_fx_pos ++;
										if(row_fx_pos > 2)
											{
												row_fx_pos = 1;
											}
										DisplaySetAFangQuPara();
										break;
									default:
										break;
								}
							}
					}
				break;
			case SetBFangQuParaScreen://�趨B����������ʾ��
				if(key != KEY_NO_PRESS)
					{
						if(flag_PageState)//�ж�ҳ��״̬��������ʾʱ��
							{
								switch(key)
								{
									case KEY_ESC_PRESS://���˳���
										row_point = 1;//��ָ��ָ��ڶ���
										DiaplaySetFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetFangQuParaScreen;//�ص��趨����������ʾ��
										row_fx_pos = 0;//�޷�����ʾ����
										break;

									case KEY_ENTER_PRESS:
										memcpy(alarm_sensitivity_temp,alarm_sensitivity,sizeof(alarm_sensitivity_temp));//���������ȵ��ݴ棨����д���˴�����Ȼ��ʾ���ᱨ�Ƿ�����
										switch(row_point)
										{
											case 1://������
												//�����ŷ�����ʾ���ɱ༭
												memcpy(field_num_temp,field_num,sizeof(field_num_temp));//���������ŵ��ݴ�
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												row_fx_pos = 1;//��һ��������ʾλ��
												break;
											case 2://�澯��ʱ
												//�澯��ʱ������ʾ���ɱ༭
												memcpy(alarm_delay_temp,alarm_delay,sizeof(alarm_delay_temp));//�����澯��ʱ���ݴ�
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												row_fx_pos = 1;//��һ��������ʾλ��
												break;
											case 3://������
												//�����ȷ�����ʾ���ɱ༭
//												memcpy(alarm_sensitivity_temp,alarm_sensitivity,sizeof(alarm_sensitivity_temp));//���������ȵ��ݴ�
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												row_fx_pos = 1;//��һ��������ʾλ��
												break;
											
											default:
												break;
										}
										DisplaySetBFangQuPara();
										break;
										
									case KEY_UP_PRESS:
										row_point --;
										if(row_point < 1)
											{
												row_point = 3;
											}
										DisplaySetBFangQuPara();
										if(row_point > 3)//���ѡ�����3��
											{
												Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
											}
										else
											{
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
											}
										break;
										
									case KEY_DOWN_PRESS:
										row_point ++;
										if(row_point > 3)
											{
												row_point = 1;
											}
										DisplaySetBFangQuPara();
										if(row_point > 3)//���ѡ�����3��
											{
												Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
											}
										else
											{
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
											}
										break;
										
									case KEY_LEFT_PRESS:
										break;
										
									case KEY_RIGHT_PRESS:
										break;
									default:
										break;
								}
							}
						else//�ж�ҳ��״̬���༭��ʾʱ��
							{
								//����ʾ��ָ��
								switch(key)
								{
									case KEY_ESC_PRESS://���˳���
										flag_PageState = NormalState;//ҳ��״̬�ɱ༭��ʾģʽת��Ϊ������ʾģʽ
										row_fx_pos = 0;//�޷�����ʾ����
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetBFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//��ʾ��ָ��
										break;

									case KEY_ENTER_PRESS:
										//
										memcpy(field_num,field_num_temp,sizeof(field_num_temp));//�����������ݴ浽������
										memcpy(alarm_delay,alarm_delay_temp,sizeof(alarm_delay_temp));//�����澯��ʱ�ݴ浽�澯��ʱ
										memcpy(alarm_sensitivity,alarm_sensitivity_temp,sizeof(alarm_sensitivity_temp));//�����������ݴ浽������
//										if(ten_val_range_temp[1][0] < ten_val_range_temp[1][1])
//											{
//												memcpy(ten_val_range[1],ten_val_range_temp[1],sizeof(ten_val_range_temp[1]));//������Ч������Χ�ݴ浽��Ч������Χ
//												WriteFieldParaToEEProm();
//											}
//										else
//											{
//												DisplayiLlegalInput();
//												delay_ms(1000);
//											}
										setFieldNum(0,field_num[0]);
										setAlarmDelay(0,alarm_delay[0]);
										WriteFieldParaToEEProm();
										flag_PageState = NormalState;//ҳ��״̬�ɱ༭��ʾģʽת��Ϊ������ʾģʽ
										row_fx_pos = 0;//�޷�����ʾ����
										DisplaySetBFangQuPara();
										if(row_point > 3)//�����ָ�����3�У���ʼ�ձ��������һ��
											{
												Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
											}
										else
											{
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
											}
										break;
										
									case KEY_UP_PRESS:
										//�жϷ����ź͸澯��ʱ���ϼ�
										switch(row_point)
										{
											case 1://�༭��һ�У�������
												switch(row_fx_pos)
												{
													case 1://��һ������λ��
														if(field_num_temp[1] < 80)
															{
																field_num_temp[1] ++;
															}
														else
															{
																field_num_temp[1] = 1;
															}
														break;

													default:
														break;
												}
												break;
											case 2://�༭�ڶ��У��澯��ʱ
												switch(row_fx_pos)
												{
													case 1://��һ������λ��
														if(alarm_delay_temp[1] < 999)
															{
																alarm_delay_temp[1] ++;
															}
														else
															{
																alarm_delay_temp[1] = 1;
															}
														break;

													default:
														break;
												}
												break;
											case 3://�༭�����У�������
												switch(row_fx_pos)
												{
													case 1://��һ������λ��
														if(alarm_sensitivity_temp[1] < 10)
															{
																alarm_sensitivity_temp[1] ++;
															}
														else
															{
																alarm_sensitivity_temp[1] = 1;
															}
														break;
													
													default:
														break;
												}
												break;
											
											default:
												break;
										}
										DisplaySetBFangQuPara();
										break;
										
									case KEY_DOWN_PRESS:
										//�жϷ����ź͸澯��ʱ���¼�
										switch(row_point)
										{
											case 1://�༭��һ�У�������
												switch(row_fx_pos)
												{
													case 1://��һ������λ��
														if(field_num_temp[1] > 1)
															{
																field_num_temp[1] --;
															}
														else
															{
																field_num_temp[1] = 80;
															}
														break;

													default:
														break;
												}
												break;
											case 2://�༭�ڶ���
												switch(row_fx_pos)
												{
													case 1://��һ������λ��
														if(alarm_delay_temp[1] > 1)
															{
																alarm_delay_temp[1] --;
															}
														else
															{
																alarm_delay_temp[1] = 999;
															}
														break;
													
													default:
														break;
												}
												break;
											case 3://�༭�����У�������
												switch(row_fx_pos)
												{
													case 1://��һ������λ��
														if(alarm_sensitivity_temp[1] > 1)
															{
																alarm_sensitivity_temp[1] --;
															}
														else
															{
																alarm_sensitivity_temp[1] = 10;
															}
														break;
													
													default:
														break;
												}
												break;
											default:
												break;
										}
										DisplaySetBFangQuPara();
										break;
										
									case KEY_LEFT_PRESS:
										//���ױ༭����
										row_fx_pos --;
										if(row_fx_pos < 1)
											{
												row_fx_pos = 2;
											}
										DisplaySetBFangQuPara();
										break;
										
									case KEY_RIGHT_PRESS:
										//���ױ༭����
										row_fx_pos ++;
										if(row_fx_pos > 2)
											{
												row_fx_pos = 1;
											}
										DisplaySetBFangQuPara();
										break;
									default:
										break;
								}
							}
					}
				break;
			case ZeroCalibrationScreen://���ֵУ׼��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DiaplayCalibrationSet();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = CalibrationSetScreen;//�ص�У׼������ʾ��
								row_fx_pos = 0;//�޷�����ʾ����
								break;
		
							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A����
										//A�����������У׼
										updateZeroVal(0);
										Displayzhengzaijiaozhun();
										current_screen = AzeroCalibrationScreen;//��ת��A�������У׼��ʾ��
										break;
									case 2://B����
										//B�����������У׼
										updateZeroVal(1);
										Displayzhengzaijiaozhun();
										current_screen = BzeroCalibrationScreen;//��ת��B�������У׼��ʾ��
										break;
									
									default:
										break;
								}
								break;
								
							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 2;
									}
								DisplayZeroCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//
								break;
								
							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 2)
									{
										row_point = 1;
									}
								DisplayZeroCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//
								break;
								
							case KEY_LEFT_PRESS:
								break;
								
							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case DatumCalibrationScreen://��׼ֵУ׼��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DiaplayCalibrationSet();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = CalibrationSetScreen;//�ص�У׼������ʾ��
								row_fx_pos = 0;//�޷�����ʾ����
								break;
		
							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A����
										if(!calibrate_en[0])
											{
												DisplayNotAtValidRange();
												current_screen = NotAtValidRangeScreen;//��ת��δ����Ч��Χ��ʾ��
											}
										else
											{
												//A�������ڻ�׼ֵУ׼
												updateBaseVal(0);
												Displayzhengzaijiaozhun();
												current_screen = AdatumCalibrationScreen;//��ת��A������׼У׼��ʾ��
											}
										break;
									case 2://B����
										if(!calibrate_en[1])
											{
												DisplayNotAtValidRange();
												current_screen = NotAtValidRangeScreen;//��ת��δ����Ч��Χ��ʾ��
											}
										else
											{
												//B�������ڻ�׼ֵУ׼
												updateBaseVal(1);
												Displayzhengzaijiaozhun();
												current_screen = BdatumCalibrationScreen;//��ת��B������׼У׼��ʾ��
											}
										break;
									
									default:
										break;
								}
								break;
								
							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 2;
									}
								DisplayDatumCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//
								break;
								
							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 2)
									{
										row_point = 1;
									}
								DisplayDatumCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//
								break;
								
							case KEY_LEFT_PRESS:
								break;
								
							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case AzeroCalibrationScreen://A�������У׼��ʾ������ʾ������У׼�����Ժ󣡡�
				if(calibrating_flag == 0xff)//У׼���
					{
						DisplayAareaZeroVal();
						current_screen = CaliOverAareaZeroValScreen;//���У׼��ɺ���ʾ��A�������ֵ��ʾ��
					}
				break;
			case BzeroCalibrationScreen://B�������У׼��ʾ������ʾ������У׼�����Ժ󣡡�
				if(calibrating_flag == 0xff)//У׼���
					{
						DisplayBareaZeroVal();
						current_screen = CaliOverBareaZeroValScreen;//���У׼��ɺ���ʾ��B�������ֵ��ʾ��
					}
				break;
			case AdatumCalibrationScreen://A������׼У׼��ʾ��
				if(calibrating_flag == 0xff)//У׼���
					{
						DisplayAareaDatumVal();
						current_screen = CaliOverAareaDatumValScreen;//��׼У׼��ɺ���ʾ��A������׼ֵ��ʾ��
					}
				break;
			case BdatumCalibrationScreen://B������׼У׼��ʾ��
				if(calibrating_flag == 0xff)//У׼���
					{
						DisplayBareaDatumVal();
						current_screen = CaliOverBareaDatumValScreen;//��׼У׼��ɺ���ʾ��B������׼ֵ��ʾ��
					}
				break;
			case NotAtValidRangeScreen://δ����Ч��Χ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayDatumCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = DatumCalibrationScreen;//�ص���׼ֵУ׼��ʾ��
								break;
		
							case KEY_ENTER_PRESS:
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayDatumCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = DatumCalibrationScreen;//�ص���׼ֵУ׼��ʾ��
								break;
								
							case KEY_UP_PRESS:
								
								break;
								
							case KEY_DOWN_PRESS:
								
								break;
								
							case KEY_LEFT_PRESS:
								break;
								
							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case CaliOverAareaZeroValScreen://���У׼��ɺ���ʾ��A�������ֵ��ʾ��
			case CaliOverBareaZeroValScreen://���У׼��ɺ���ʾ��B�������ֵ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayZeroCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = ZeroCalibrationScreen;//�ص����ֵУ׼��ʾ��
								break;

							case KEY_ENTER_PRESS:
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayZeroCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = ZeroCalibrationScreen;//�ص����ֵУ׼��ʾ��
								break;

							case KEY_UP_PRESS:
								
								break;

							case KEY_DOWN_PRESS:
								
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case CaliOverAareaDatumValScreen://��׼У׼��ɺ���ʾ��A������׼ֵ��ʾ��
			case CaliOverBareaDatumValScreen://��׼У׼��ɺ���ʾ��B������׼ֵ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayDatumCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = DatumCalibrationScreen;//�ص���׼ֵУ׼��ʾ��
								break;

							case KEY_ENTER_PRESS:
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayDatumCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = DatumCalibrationScreen;//�ص���׼ֵУ׼��ʾ��
								break;

							case KEY_UP_PRESS:
								
								break;

							case KEY_DOWN_PRESS:
								
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			
			
			
			
			
			
			
			case SetRS485ParaScreen://�趨RS485������ʾ��
				
				break;
			case InqEthParaScreen://��ѯ���������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqSysPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqSysParaScreen;//�ص���ѯϵͳ������ʾ��
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://MAC��ַ
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayInqMacAddr();
										current_screen = InqMacAddrScreen;//��ǰ��Ϊ��ѯMac��ַ�˵�
										break;
									case 2://IP��ַ
										row_point = 1;//��ָ��ָ��ڶ���
										GetIpAddr();
										DisplayInqIpAddr();
										current_screen = InqIpAddrScreen;//��ǰ��Ϊ��ѯIP��ַ�˵�
										break;
									case 3://��������
										row_point = 1;//��ָ��ָ��ڶ���
										GetSubNetMask();
										DisplayInqSubnetMask();
										current_screen = InqSubnetMaskScreen;//��ǰ��Ϊ��ѯ��������˵�
										break;
									case 4://����
										row_point = 1;//��ָ��ָ��ڶ���
										GetGateWay();
										DisplayInqGateWay();
										current_screen = InqGateWayScreen;//��ǰ��Ϊ��ѯ���ز˵�
										break;
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 4;
									}
								DisplayInqEthPara();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 4)
									{
										row_point = 1;
									}
								DisplayInqEthPara();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqRS485ParaScreen://��ѯRS485������ʾ��
				
				break;
			case InqDuanLuAlarmScreen://��ѯ��·�澯��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqAlarmRecordScreen;//�ص���ѯ�澯��¼��ʾ��
								break;
								
							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A��
										row_point = 1;//��ָ��ָ��ڶ���
										ReadLCDFirstPageLog(LOG_TYPE_AIVD);
										DisplayInqAAreaRuQinAlarmRecord();
										current_screen = InqAAreaDuanLuAlarmRecordScreen;//��ǰ��Ϊ��ѯA����·�澯��¼�˵�
										break;
									case 2://B��
										row_point = 1;//��ָ��ָ��ڶ���
										ReadLCDFirstPageLog(LOG_TYPE_BIVD);
										DisplayInqBAreaRuQinAlarmRecord();
										current_screen = InqBAreaDuanLuAlarmRecordScreen;//��ǰ��Ϊ��ѯB����·�澯��¼�˵�
										break;
									default:
										break;
								}
								break;
								
							case KEY_UP_PRESS:
								row_point --;
								if(FIELD_FLAG_S0D1)
									{//����������ʾB����
										if(row_point < 1)
											{
												row_point = 1;
											}
									}
								else
									{
										if(row_point < 1)
											{
												row_point = 2;
											}
									}
								DisplayInqRuQinAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;
								
							case KEY_DOWN_PRESS:
								row_point ++;
								if(FIELD_FLAG_S0D1)
									{//����������ʾB����
										if(row_point > 1)
											{
												row_point = 1;
											}
									}
								else
									{
										if(row_point > 2)
											{
												row_point = 1;
											}
									}
								DisplayInqRuQinAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;
								
							case KEY_LEFT_PRESS:
								break;
								
							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqDuanXianAlarmScreen://��ѯ��·�澯��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqAlarmRecordScreen;//�ص���ѯ�澯��¼��ʾ��
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A��
										row_point = 1;//��ָ��ָ��ڶ���
										ReadLCDFirstPageLog(LOG_TYPE_AOPN);
										DisplayInqAAreaDuanXianAlarmRecord();
										current_screen = InqAAreaDuanXianAlarmRecordScreen;//��ǰ��Ϊ��ѯA����·�澯��¼�˵�
										break;
									case 2://B��
										row_point = 1;//��ָ��ָ��ڶ���
										ReadLCDFirstPageLog(LOG_TYPE_BOPN);
										DisplayInqBAreaDuanXianAlarmRecord();
										current_screen = InqBAreaDuanXianAlarmRecordScreen;//��ǰ��Ϊ��ѯB����·�澯��¼�˵�
										break;
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(FIELD_FLAG_S0D1)
									{//����������ʾB����
										if(row_point < 1)
											{
												row_point = 1;
											}
									}
								else
									{
										if(row_point < 1)
											{
												row_point = 2;
											}
									}
								DisplayInqDuanXianAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(FIELD_FLAG_S0D1)
									{//����������ʾB����
										if(row_point > 1)
											{
												row_point = 1;
											}
									}
								else
									{
										if(row_point > 2)
											{
												row_point = 1;
											}
									}
								DisplayInqDuanXianAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case InqFangChaiAlarmRecordScreen://��ѯ����澯��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqAlarmRecordScreen;//�ص���ѯ�澯��¼��ʾ��
								break;

							case KEY_ENTER_PRESS:
								break;

							case KEY_UP_PRESS:
								if (LCDLogPageFlag != LOG_HEAD) {
									ReadLCDPreviousPageLog(LOG_TYPE_FC);
								}
								DisplayInqFangChaiAlarmRecord();
								break;

							case KEY_DOWN_PRESS:
								if (LCDLogPageFlag != LOG_END) {
									ReadLCDNextPageLog(LOG_TYPE_FC);
								}
								DisplayInqFangChaiAlarmRecord();
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case SetMacAddrScreen://�趨MAC��ַ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetEthPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetEthParaScreen;//�ص��趨��̫��������ʾ��
								break;

							case KEY_ENTER_PRESS:
								//����MAC��ַ��EEPROM
								break;

							case KEY_UP_PRESS:

								break;

							case KEY_DOWN_PRESS:

								break;

							case KEY_LEFT_PRESS:

								break;

							case KEY_RIGHT_PRESS:

								break;
							default:
								break;
						}
					}
				break;
			case SetIpAddrScreen://�趨IP��ַ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetEthPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetEthParaScreen;//�ص��趨��̫��������ʾ��
								break;

							case KEY_ENTER_PRESS:
								//����IP��ַ��EEPROM
								SetIpAddr();
								lwip_reset_netif_ipaddr();
								row_fx_pos = 1;//��һ��λ�÷�����ʾ
								break;

							case KEY_UP_PRESS:
								switch(row_fx_pos)
								{
									case 1:
									case 2:
									case 3:
									case 5:
									case 6:
									case 7:
									case 9:
									case 10:
									case 11:
									case 13:
									case 14:
									case 15:
										if((row_fx_pos % 4) == 1)
											{
												eth_ip_addr[row_fx_pos - 1] ++;
												if(eth_ip_addr[row_fx_pos - 1] > '2')
													{
														eth_ip_addr[row_fx_pos- 1] = '0';
													}
												if(eth_ip_addr[row_fx_pos- 1] == '2')
													{
														if(eth_ip_addr[row_fx_pos] > '5')
															{
																eth_ip_addr[row_fx_pos] = '0';
															}
													}
											}
										else if((row_fx_pos % 4) == 2)
											{
												if(eth_ip_addr[row_fx_pos- 2] == '2')
													{
														eth_ip_addr[row_fx_pos - 1] ++;
														if(eth_ip_addr[row_fx_pos - 1] > '5')
															{
																eth_ip_addr[row_fx_pos- 1] = '0';
															}
													}
												else
													{
														eth_ip_addr[row_fx_pos - 1] ++;
														if(eth_ip_addr[row_fx_pos - 1] > '9')
															{
																eth_ip_addr[row_fx_pos- 1] = '0';
															}
													}
											}
										else if((row_fx_pos % 4) == 3)
											{
												eth_ip_addr[row_fx_pos - 1] ++;
												if(eth_ip_addr[row_fx_pos - 1] > '9')
													{
														eth_ip_addr[row_fx_pos- 1] = '0';
													}
											}
										else
											{

											}
										break;

									default:
										break;
								}
								DisplaySetIpAddr();
								break;

							case KEY_DOWN_PRESS:
								switch(row_fx_pos)
								{
									case 1:
									case 2:
									case 3:
									case 5:
									case 6:
									case 7:
									case 9:
									case 10:
									case 11:
									case 13:
									case 14:
									case 15:
										if((row_fx_pos % 4) == 1)
											{
												eth_ip_addr[row_fx_pos - 1] --;
												if(eth_ip_addr[row_fx_pos - 1] < '0')
													{
														eth_ip_addr[row_fx_pos- 1] = '2';
													}
												if(eth_ip_addr[row_fx_pos- 1] == '2')
													{
														if(eth_ip_addr[row_fx_pos] > '5')
															{
																eth_ip_addr[row_fx_pos] = '0';
															}
													}
											}
										else if((row_fx_pos % 4) == 2)
											{
												if(eth_ip_addr[row_fx_pos- 2] == '2')
													{
														eth_ip_addr[row_fx_pos - 1] --;
														if(eth_ip_addr[row_fx_pos - 1] < '0')
															{
																eth_ip_addr[row_fx_pos- 1] = '5';
															}
													}
												else
													{
														eth_ip_addr[row_fx_pos - 1] --;
														if(eth_ip_addr[row_fx_pos - 1] < '0')
															{
																eth_ip_addr[row_fx_pos- 1] = '9';
															}
													}
											}
										else if((row_fx_pos % 4) == 3)
											{
												eth_ip_addr[row_fx_pos - 1] --;
												if(eth_ip_addr[row_fx_pos - 1] < '0')
													{
														eth_ip_addr[row_fx_pos- 1] = '9';
													}
											}
										else
											{

											}
										break;

									default:
										break;
								}
								DisplaySetIpAddr();
								break;

							case KEY_LEFT_PRESS:
								if((row_fx_pos == 5) || (row_fx_pos == 9) || (row_fx_pos == 13))
									{
										row_fx_pos --;
									}
								row_fx_pos --;
								if(row_fx_pos < 1)
									{
										row_fx_pos = 15;//12���ɱ༭���� +3�����ɱ༭�ġ�.��
									}
								DisplaySetIpAddr();
								break;

							case KEY_RIGHT_PRESS:
								if((row_fx_pos == 3) || (row_fx_pos == 7) || (row_fx_pos == 11))
									{
										row_fx_pos ++;
									}
								row_fx_pos ++;
								if(row_fx_pos > 15)//12���ɱ༭���� +3�����ɱ༭�ġ�.��
									{
										row_fx_pos = 1;
									}
								DisplaySetIpAddr();
								break;
							default:
								break;
						}
					}
				break;
			case SetSubnetMaskScreen://�趨����������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetEthPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetEthParaScreen;//�ص��趨��̫��������ʾ��
								break;

							case KEY_ENTER_PRESS:
								//����IP��ַ��EEPROM
								SetSubNetMask();
								lwip_reset_netif_ipaddr();
								row_fx_pos = 1;//��һ��λ�÷�����ʾ
								break;

							case KEY_UP_PRESS:
								switch(row_fx_pos)
								{
									case 1:
									case 2:
									case 3:
									case 5:
									case 6:
									case 7:
									case 9:
									case 10:
									case 11:
									case 13:
									case 14:
									case 15:
										if((row_fx_pos % 4) == 1)
											{
												eth_subnet_mask[row_fx_pos - 1] ++;
												if(eth_subnet_mask[row_fx_pos - 1] > '2')
													{
														eth_subnet_mask[row_fx_pos- 1] = '0';
													}
												if(eth_subnet_mask[row_fx_pos- 1] == '2')
													{
														if(eth_subnet_mask[row_fx_pos] > '5')
															{
																eth_subnet_mask[row_fx_pos] = '0';
															}
													}
											}
										else if((row_fx_pos % 4) == 2)
											{
												if(eth_subnet_mask[row_fx_pos- 2] == '2')
													{
														eth_subnet_mask[row_fx_pos - 1] ++;
														if(eth_subnet_mask[row_fx_pos - 1] > '5')
															{
																eth_subnet_mask[row_fx_pos- 1] = '0';
															}
													}
												else
													{
														eth_subnet_mask[row_fx_pos - 1] ++;
														if(eth_subnet_mask[row_fx_pos - 1] > '9')
															{
																eth_subnet_mask[row_fx_pos- 1] = '0';
															}
													}
											}
										else if((row_fx_pos % 4) == 3)
											{
												eth_subnet_mask[row_fx_pos - 1] ++;
												if(eth_subnet_mask[row_fx_pos - 1] > '9')
													{
														eth_subnet_mask[row_fx_pos- 1] = '0';
													}
											}
										else
											{

											}
										break;

									default:
										break;
								}
								DisplaySetSubnetMask();
								break;

							case KEY_DOWN_PRESS:
								switch(row_fx_pos)
								{
									case 1:
									case 2:
									case 3:
									case 5:
									case 6:
									case 7:
									case 9:
									case 10:
									case 11:
									case 13:
									case 14:
									case 15:
										if((row_fx_pos % 4) == 1)
											{
												eth_subnet_mask[row_fx_pos - 1] --;
												if(eth_subnet_mask[row_fx_pos - 1] < '0')
													{
														eth_subnet_mask[row_fx_pos- 1] = '2';
													}
												if(eth_subnet_mask[row_fx_pos- 1] == '2')
													{
														if(eth_subnet_mask[row_fx_pos] > '5')
															{
																eth_subnet_mask[row_fx_pos] = '0';
															}
													}
											}
										else if((row_fx_pos % 4) == 2)
											{
												if(eth_subnet_mask[row_fx_pos- 2] == '2')
													{
														eth_subnet_mask[row_fx_pos - 1] --;
														if(eth_subnet_mask[row_fx_pos - 1] < '0')
															{
																eth_subnet_mask[row_fx_pos- 1] = '5';
															}
													}
												else
													{
														eth_subnet_mask[row_fx_pos - 1] --;
														if(eth_subnet_mask[row_fx_pos - 1] < '0')
															{
																eth_subnet_mask[row_fx_pos- 1] = '9';
															}
													}
											}
										else if((row_fx_pos % 4) == 3)
											{
												eth_subnet_mask[row_fx_pos - 1] --;
												if(eth_subnet_mask[row_fx_pos - 1] < '0')
													{
														eth_subnet_mask[row_fx_pos- 1] = '9';
													}
											}
										else
											{

											}
										break;

									default:
										break;
								}
								DisplaySetSubnetMask();
								break;

							case KEY_LEFT_PRESS:
								if((row_fx_pos == 5) || (row_fx_pos == 9) || (row_fx_pos == 13))
									{
										row_fx_pos --;
									}
								row_fx_pos --;
								if(row_fx_pos < 1)
									{
										row_fx_pos = 15;//12���ɱ༭���� +3�����ɱ༭�ġ�.��
									}
								DisplaySetSubnetMask();
								break;

							case KEY_RIGHT_PRESS:
								if((row_fx_pos == 3) || (row_fx_pos == 7) || (row_fx_pos == 11))
									{
										row_fx_pos ++;
									}
								row_fx_pos ++;
								if(row_fx_pos > 15)//12���ɱ༭���� +3�����ɱ༭�ġ�.��
									{
										row_fx_pos = 1;
									}
								DisplaySetSubnetMask();
								break;
							default:
								break;
						}
					}
				break;
			case SetGateWayScreen://�趨������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetEthPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetEthParaScreen;//�ص��趨��̫��������ʾ��
								break;

							case KEY_ENTER_PRESS:
								//����IP��ַ��EEPROM
								SetGateWay();
								lwip_reset_netif_ipaddr();
								row_fx_pos = 1;//��һ��λ�÷�����ʾ
								break;

							case KEY_UP_PRESS:
								switch(row_fx_pos)
								{
									case 1:
									case 2:
									case 3:
									case 5:
									case 6:
									case 7:
									case 9:
									case 10:
									case 11:
									case 13:
									case 14:
									case 15:
										if((row_fx_pos % 4) == 1)
											{
												eth_gateway[row_fx_pos - 1] ++;
												if(eth_gateway[row_fx_pos - 1] > '2')
													{
														eth_gateway[row_fx_pos- 1] = '0';
													}
												if(eth_gateway[row_fx_pos- 1] == '2')
													{
														if(eth_gateway[row_fx_pos] > '5')
															{
																eth_gateway[row_fx_pos] = '0';
															}
													}
											}
										else if((row_fx_pos % 4) == 2)
											{
												if(eth_gateway[row_fx_pos- 2] == '2')
													{
														eth_gateway[row_fx_pos - 1] ++;
														if(eth_gateway[row_fx_pos - 1] > '5')
															{
																eth_gateway[row_fx_pos- 1] = '0';
															}
													}
												else
													{
														eth_gateway[row_fx_pos - 1] ++;
														if(eth_gateway[row_fx_pos - 1] > '9')
															{
																eth_gateway[row_fx_pos- 1] = '0';
															}
													}
											}
										else if((row_fx_pos % 4) == 3)
											{
												eth_gateway[row_fx_pos - 1] ++;
												if(eth_gateway[row_fx_pos - 1] > '9')
													{
														eth_gateway[row_fx_pos- 1] = '0';
													}
											}
										else
											{

											}
										break;

									default:
										break;
								}
								DisplaySetGateWay();
								break;

							case KEY_DOWN_PRESS:
								switch(row_fx_pos)
								{
									case 1:
									case 2:
									case 3:
									case 5:
									case 6:
									case 7:
									case 9:
									case 10:
									case 11:
									case 13:
									case 14:
									case 15:
										if((row_fx_pos % 4) == 1)
											{
												eth_gateway[row_fx_pos - 1] --;
												if(eth_gateway[row_fx_pos - 1] < '0')
													{
														eth_gateway[row_fx_pos- 1] = '2';
													}
												if(eth_gateway[row_fx_pos- 1] == '2')
													{
														if(eth_gateway[row_fx_pos] > '5')
															{
																eth_gateway[row_fx_pos] = '0';
															}
													}
											}
										else if((row_fx_pos % 4) == 2)
											{
												if(eth_gateway[row_fx_pos- 2] == '2')
													{
														eth_gateway[row_fx_pos - 1] --;
														if(eth_gateway[row_fx_pos - 1] < '0')
															{
																eth_gateway[row_fx_pos- 1] = '5';
															}
													}
												else
													{
														eth_gateway[row_fx_pos - 1] --;
														if(eth_gateway[row_fx_pos - 1] < '0')
															{
																eth_gateway[row_fx_pos- 1] = '9';
															}
													}
											}
										else if((row_fx_pos % 4) == 3)
											{
												eth_gateway[row_fx_pos - 1] --;
												if(eth_gateway[row_fx_pos - 1] < '0')
													{
														eth_gateway[row_fx_pos- 1] = '9';
													}
											}
										else
											{

											}
										break;

									default:
										break;
								}
								DisplaySetGateWay();
								break;

							case KEY_LEFT_PRESS:
								if((row_fx_pos == 5) || (row_fx_pos == 9) || (row_fx_pos == 13))
									{
										row_fx_pos --;
									}
								row_fx_pos --;
								if(row_fx_pos < 1)
									{
										row_fx_pos = 15;//12���ɱ༭���� +3�����ɱ༭�ġ�.��
									}
								DisplaySetGateWay();
								break;

							case KEY_RIGHT_PRESS:
								if((row_fx_pos == 3) || (row_fx_pos == 7) || (row_fx_pos == 11))
									{
										row_fx_pos ++;
									}
								row_fx_pos ++;
								if(row_fx_pos > 15)//12���ɱ༭���� +3�����ɱ༭�ġ�.��
									{
										row_fx_pos = 1;
									}
								DisplaySetGateWay();
								break;
							default:
								break;
						}
					}
				break;
			case InqMacAddrScreen://��ѯMAC��ַ��ʾ��
			case InqIpAddrScreen://��ѯIP��ַ��ʾ��
			case InqSubnetMaskScreen://��ѯ����������ʾ��
			case InqGateWayScreen://��ѯ������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
							case KEY_ENTER_PRESS:
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqEthPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqEthParaScreen;//�ص���ѯ��̫��������ʾ��
								break;

							case KEY_UP_PRESS:

								break;

							case KEY_DOWN_PRESS:

								break;

							case KEY_LEFT_PRESS:

								break;

							case KEY_RIGHT_PRESS:

								break;
							default:
								break;
						}
					}
				break;
			case InqAFangQuParaScreen://��ѯA����������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DiaplayInqFangQuPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqFangQuParaScreen;//�ص���ѯ����������ʾ��
								break;

							case KEY_ENTER_PRESS:

								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 4;
									}
								DiaplayInqAFangQuPara();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;
										
							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 4)
									{
										row_point = 1;
									}
								DiaplayInqAFangQuPara();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;

							case KEY_LEFT_PRESS:

								break;

							case KEY_RIGHT_PRESS:

								break;
							default:
								break;
						}
					}
				break;
			case InqBFangQuParaScreen://��ѯB����������ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DiaplayInqFangQuPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqFangQuParaScreen;//�ص���ѯ����������ʾ��
								break;

							case KEY_ENTER_PRESS:

								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 4;
									}
								DiaplayInqBFangQuPara();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;
										
							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 4)
									{
										row_point = 1;
									}
								DiaplayInqBFangQuPara();
								if(row_point > 3)//���ѡ�����3��
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//�м�ͷʼ�ձ�������ʾ�������һ��
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//����˳����ʾ
									}
								break;

							case KEY_LEFT_PRESS:

								break;

							case KEY_RIGHT_PRESS:

								break;
							default:
								break;
						}
					}
				break;
			case InqAAreaDuanLuAlarmRecordScreen://��ѯA������·�澯��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqRuQinAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqDuanLuAlarmScreen;//�ص���ѯ��·�澯��¼��ʾ��
								break;
								
							case KEY_ENTER_PRESS:
								
								break;
								
							case KEY_UP_PRESS:
								if(LCDLogPageFlag != LOG_HEAD){
									ReadLCDPreviousPageLog(LOG_TYPE_AIVD);
								}
								DisplayInqAAreaRuQinAlarmRecord();
								break;
								
							case KEY_DOWN_PRESS:
								if(LCDLogPageFlag != LOG_END){
									ReadLCDNextPageLog(LOG_TYPE_AIVD);
								}
								DisplayInqAAreaRuQinAlarmRecord();
								break;
								
							case KEY_LEFT_PRESS:
								
								break;
								
							case KEY_RIGHT_PRESS:
								
								break;
							default:
								break;
						}
					}
				break;
			case InqBAreaDuanLuAlarmRecordScreen://��ѯB������·�澯��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqRuQinAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqDuanLuAlarmScreen;//�ص���ѯ��·�澯��¼��ʾ��
								break;
								
							case KEY_ENTER_PRESS:
								
								break;
								
							case KEY_UP_PRESS:
								if (LCDLogPageFlag != LOG_HEAD) {
									ReadLCDPreviousPageLog(LOG_TYPE_BIVD);
								}
								DisplayInqBAreaRuQinAlarmRecord();
								break;
		
							case KEY_DOWN_PRESS:
								if (LCDLogPageFlag != LOG_END) {
									ReadLCDNextPageLog(LOG_TYPE_BIVD);
								}
								DisplayInqBAreaRuQinAlarmRecord();
								break;
								
							case KEY_LEFT_PRESS:
								
								break;
								
							case KEY_RIGHT_PRESS:
								
								break;
							default:
								break;
						}
					}
				break;
			case InqAAreaDuanXianAlarmRecordScreen://��ѯA������·�澯��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqDuanXianAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqDuanXianAlarmScreen;//�ص���ѯ��·�澯��¼��ʾ��
								break;
								
							case KEY_ENTER_PRESS:
								
								break;
								
							case KEY_UP_PRESS:
								if (LCDLogPageFlag != LOG_HEAD) {
									ReadLCDPreviousPageLog(LOG_TYPE_AOPN);
								}
								DisplayInqAAreaDuanXianAlarmRecord();
								break;

							case KEY_DOWN_PRESS:
								if (LCDLogPageFlag != LOG_END) {
									ReadLCDNextPageLog(LOG_TYPE_AOPN);
								}
								DisplayInqAAreaDuanXianAlarmRecord();
								break;

							case KEY_LEFT_PRESS:

								break;

							case KEY_RIGHT_PRESS:

								break;
							default:
								break;
						}
					}
				break;
			case InqBAreaDuanXianAlarmRecordScreen://��ѯB������·�澯��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqDuanXianAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqDuanXianAlarmScreen;//�ص���ѯ��·�澯��¼��ʾ��
								break;

							case KEY_ENTER_PRESS:

								break;

							case KEY_UP_PRESS:
								if (LCDLogPageFlag != LOG_HEAD) {
									ReadLCDPreviousPageLog(LOG_TYPE_BOPN);
								}
								DisplayInqBAreaDuanXianAlarmRecord();
								break;

							case KEY_DOWN_PRESS:
								if (LCDLogPageFlag != LOG_END) {
									ReadLCDNextPageLog(LOG_TYPE_BOPN);
								}
								DisplayInqBAreaDuanXianAlarmRecord();
								break;

							case KEY_LEFT_PRESS:

								break;

							case KEY_RIGHT_PRESS:

								break;
							default:
								break;
						}
					}
				break;
			case ClearDuanLuAlarmRecordScreen://�����·�澯��¼��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetClearRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetClearRecordScreen;//�ص������¼��ʾ��
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A��
										DisplayZhengZaiQingChu();
										//���EEPROM
										ClearLog(LOG_TYPE_AIVD);
										
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayClearRuQinAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ClearDuanLuAlarmRecordScreen;//�����·�澯��¼��ʾ��
										break;
									case 2://B��
										DisplayZhengZaiQingChu();
										//���EEPROM
										ClearLog(LOG_TYPE_BIVD);
										
										row_point = 1;//��ָ��ָ��ڶ���
										DisplayClearRuQinAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ClearDuanLuAlarmRecordScreen;//�����·�澯��¼��ʾ��
										break;
		
									default:
										break;
								}
								break;
								
							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 2;
									}
								DisplayClearRuQinAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;
								
							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 2)
									{
										row_point = 1;
									}
								DisplayClearRuQinAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;
								
							case KEY_LEFT_PRESS:
								break;
								
							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case ClearDuanXianAlarmRecordScreen://�����·�澯��¼��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetClearRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetClearRecordScreen;//�ص������¼��ʾ��
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A��
										DisplayZhengZaiQingChu();
										//���EEPROM
										ClearLog(LOG_TYPE_AOPN);

										row_point = 1;//��ָ��ָ��ڶ���
										DisplayClearDuanXianAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ClearDuanXianAlarmRecordScreen;//�����·�澯��¼��ʾ��
										break;
									case 2://B��
										DisplayZhengZaiQingChu();
										//���EEPROM
										ClearLog(LOG_TYPE_BOPN);

										row_point = 1;//��ָ��ָ��ڶ���
										DisplayClearDuanXianAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ClearDuanXianAlarmRecordScreen;//�����·�澯��¼��ʾ��
										break;

									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(row_point < 1)
									{
										row_point = 2;
									}
								DisplayClearDuanXianAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 2)
									{
										row_point = 1;
									}
								DisplayClearDuanXianAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								break;

							case KEY_LEFT_PRESS:
								break;

							case KEY_RIGHT_PRESS:
								break;
							default:
								break;
						}
					}
				break;
			case AlarmTypeListScreen://�澯��Ϣ�б���ʾ��
				if(0 == (IS_SET_BIT(config_code, 2)? ALARM_STATE: ALARM_STATE_DELAY)){  //����޸澯���ص���ʾ��
					alarm_type_cnt = 0;
					old_Alarm_Type_Sum = 0;
					cur_Alarm_Type_Sum = 0;
					alarm_key_adj = 0;
					current_screen = RunScreen;//�ص�������ʾ��
					DisplayRunMenu();
					break;
				}

//				if(old_Alarm_Type_Sum != cur_Alarm_Type_Sum)
//					{
						DisplayAlarmTypeList();
//					}
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								current_screen = RunScreen;//�ص�������ʾ��
								DisplayRunMenu();
								break;

							case KEY_ENTER_PRESS:
								row_point = 1;//��ָ��ָ��ڶ���
								DisplayInqAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqAlarmRecordScreen;//�ص���ѯ�澯��¼��ʾ��
								break;

							case KEY_UP_PRESS:
								alarm_key_adj --;
								if(alarm_key_adj < 0)
									{
										alarm_key_adj = alarm_rol - 2;
									}
								DisplayAlarmTypeList();
								break;

							case KEY_DOWN_PRESS:
								alarm_key_adj ++;
								if(alarm_key_adj > alarm_rol - 2)
									{
										alarm_key_adj = 0;
									}
								DisplayAlarmTypeList();
								break;

							case KEY_LEFT_PRESS:
								Remove_Alarm_RL_A();
								break;

							case KEY_RIGHT_PRESS:
								Remove_Alarm_RL_B();
								break;
							default:
								break;
						}
					}
				break;

			case SetExtDeviceTypeScreen://�趨�����ⲿ�豸��ʾ��
				if(key != KEY_NO_PRESS)
					{
						if(flag_PageState)//�ж�ҳ��״̬��������ʾʱ��
							{
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//��ʾ��ָ��

								switch(key)
								{
									case KEY_ESC_PRESS://���˳���
										row_point = 1;//��ָ��ָ��ڶ���
										DisplaySetMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetScreen;//�ص��趨�˵�
										break;

									case KEY_ENTER_PRESS:
										switch(row_point)
										{
											case 1://A�����༭
												//A���������ⲿ�豸���ͷ�����ʾ���ɱ༭
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												break;
											case 2://B�����༭
												//B���������ⲿ�豸���ͷ�����ʾ���ɱ༭
												flag_PageState = ModifyState;//ҳ��״̬��������ʾģʽת��Ϊ�༭��ʾģʽ
												break;

											default:
												break;
										}
										DiaplaySetExtDeviceTypeScreen();
										break;

									case KEY_UP_PRESS:
										row_point --;
										if(FIELD_FLAG_S0D1)
											{//����������ʾB����
												if(row_point < 1)
													{
														row_point = 1;
													}
											}
										else
											{
												if(row_point < 1)
													{
														row_point = 2;
													}
											}
										DiaplaySetExtDeviceTypeScreen();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										break;

									case KEY_DOWN_PRESS:
										row_point ++;
										if(FIELD_FLAG_S0D1)
											{//����������ʾB����
												if(row_point > 1)
													{
														row_point = 1;
													}
											}
										else
											{
												if(row_point > 2)
													{
														row_point = 1;
													}
											}
										DiaplaySetExtDeviceTypeScreen();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										break;

									case KEY_LEFT_PRESS:
										break;

									case KEY_RIGHT_PRESS:
										break;
									default:
										break;
								}
							}
						else//�ж�ҳ��״̬���༭��ʾʱ��
							{
								//����ʾ��ָ��
								switch(key)
								{
									case KEY_ESC_PRESS://���˳���
										row_point = 1;//��ָ��ָ��ڶ���
										flag_PageState = NormalState;
										DiaplaySetExtDeviceTypeScreen();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//��ʾ��ָ��
										current_screen = SetExtDeviceTypeScreen;//��ǰ��Ϊ�趨����豸���Ͳ˵�
										break;

									case KEY_ENTER_PRESS:
										setSWValue((SW_Type[1] << 8) | SW_Type[0]);
										flag_PageState = NormalState;//ҳ��״̬�ɱ༭��ʾģʽת��Ϊ������ʾģʽ
										DiaplaySetExtDeviceTypeScreen();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//��ʾ��ָ��
										break;

									case KEY_UP_PRESS:
										//�ж��ĸ�����
										switch(row_point)
										{
											case 1://�༭��һ�У�A���������ⲿ�豸���ͣ�
												SW_Type[0]++;
												if(SW_Type[0] > 8)
													{
														SW_Type[0] = 0;
													}
												break;
											case 2://�༭�ڶ��У�B���������ⲿ�豸���ͣ�
												SW_Type[1]++;
												if(SW_Type[1] > 8)
													{
														SW_Type[1] = 0;
													}
												break;

											default:
												break;
										}
										DiaplaySetExtDeviceTypeScreen();
										break;

									case KEY_DOWN_PRESS:
										//�ж����ĸ�����
										switch(row_point)
										{
											case 1://�༭��һ�У�A���������ⲿ�豸���ͣ�
												SW_Type[0]--;
												if(SW_Type[0] < 0)
													{
														SW_Type[0] = 8;
													}
												break;
											case 2://�༭�ڶ��У�B���������ⲿ�豸���ͣ�
												SW_Type[1]--;
												if(SW_Type[1] < 0)
													{
														SW_Type[1] = 8;
													}
												break;

											default:
												break;
										}
										DiaplaySetExtDeviceTypeScreen();
										break;

									case KEY_LEFT_PRESS:

										break;

									case KEY_RIGHT_PRESS:

										break;
									default:
										break;
								}
							}
					}
				break;
			
			case SetFullScaleScreen://��������������ֵ��ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetScreen;//�ص��趨��ʾ��
								break;

							case KEY_ENTER_PRESS:
								setTensionMaxRange((u16)(atoi(tension_max_range_temp) * 10));
								row_fx_pos = 1;//��һ��λ�÷�����ʾ
								DisplaySetFullScale();
								break;

							case KEY_UP_PRESS:
								switch(row_fx_pos)
								{
									case 1:
										tension_max_range_temp[row_fx_pos - 1] ++;
										if(tension_max_range_temp[row_fx_pos - 1] > '1')
											{
												tension_max_range_temp[row_fx_pos- 1] = '0';
											}
										break;
									case 2:
									case 3:
										tension_max_range_temp[row_fx_pos - 1] ++;
										if(tension_max_range_temp[row_fx_pos - 1] > '9')
											{
												tension_max_range_temp[row_fx_pos- 1] = '0';
											}
										break;
									

									default:
										break;
								}
								DisplaySetFullScale();
								break;

							case KEY_DOWN_PRESS:
								switch(row_fx_pos)
								{
									case 1:
										tension_max_range_temp[row_fx_pos - 1] --;
										if(tension_max_range_temp[row_fx_pos - 1] < '0')
											{
												tension_max_range_temp[row_fx_pos- 1] = '1';
											}
										break;
									case 2:
									case 3:
										tension_max_range_temp[row_fx_pos - 1] --;
										if(tension_max_range_temp[row_fx_pos - 1] < '0')
											{
												tension_max_range_temp[row_fx_pos- 1] = '9';
											}
										break;

									default:
										break;
								}
								DisplaySetFullScale();
								break;

							case KEY_LEFT_PRESS:
								row_fx_pos --;
								if(row_fx_pos == 0){
									row_fx_pos = 3;
								}
								DisplaySetFullScale();
								break;

							case KEY_RIGHT_PRESS:
								row_fx_pos ++;
								if(row_fx_pos > 3){
									row_fx_pos = 1;
								}
								DisplaySetFullScale();
								break;
							default:
								break;
						}
					}
				break;
			
			case SetBaseAutoCalibrateTimeScreen://���û�׼ֵ�Զ�У׼ʱ����ʾ��
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://���˳���
								row_point = 1;//��ָ��ָ��ڶ���
								DisplaySetMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetScreen;//�ص��趨��ʾ��
								break;

							case KEY_ENTER_PRESS:
								setBaseAutoCalibrateTime((u16)atoi(base_auto_calibrate_time_tmp));
								row_fx_pos = 1;//��һ��λ�÷�����ʾ
								DisplaySetBaseAutoCalibrateTime();
								break;

							case KEY_UP_PRESS:
								switch(row_fx_pos)
								{
									case 1:
										base_auto_calibrate_time_tmp[row_fx_pos - 1] ++;
										if(base_auto_calibrate_time_tmp[row_fx_pos - 1] > '5')
											{
												base_auto_calibrate_time_tmp[row_fx_pos- 1] = '0';
											}
										break;
									case 2:
									case 3:
									case 4:
									case 5:
										base_auto_calibrate_time_tmp[row_fx_pos - 1] ++;
										if(base_auto_calibrate_time_tmp[row_fx_pos - 1] > '9')
											{
												base_auto_calibrate_time_tmp[row_fx_pos- 1] = '0';
											}
										break;
									

									default:
										break;
								}
								DisplaySetBaseAutoCalibrateTime();
								break;

							case KEY_DOWN_PRESS:
								switch(row_fx_pos)
								{
									case 1:
										base_auto_calibrate_time_tmp[row_fx_pos - 1] --;
										if(base_auto_calibrate_time_tmp[row_fx_pos - 1] < '0')
											{
												base_auto_calibrate_time_tmp[row_fx_pos- 1] = '5';
											}
										break;
									case 2:
									case 3:
									case 4:
									case 5:
										base_auto_calibrate_time_tmp[row_fx_pos - 1] --;
										if(base_auto_calibrate_time_tmp[row_fx_pos - 1] < '0')
											{
												base_auto_calibrate_time_tmp[row_fx_pos- 1] = '9';
											}
										break;

									default:
										break;
								}
								DisplaySetBaseAutoCalibrateTime();
								break;

							case KEY_LEFT_PRESS:
								row_fx_pos --;
								if(row_fx_pos == 0){
									row_fx_pos = 5;
								}
								DisplaySetBaseAutoCalibrateTime();
								break;

							case KEY_RIGHT_PRESS:
								row_fx_pos ++;
								if(row_fx_pos > 5){
									row_fx_pos = 1;
								}
								DisplaySetBaseAutoCalibrateTime();
								break;
							default:
								break;
						}
					}
				break;

			default:
				break;
		}
		delay_ms(10);
	}
	
} 


//���ӡ��澯ɨ������
void watch_task(void *pdata)
{	
	pdata=pdata;
	while(1){

		FANG_CHAI_Check();
		
		//�����¶�
		if (IS_SET_VAR_BIT(timerHandlerFlag,7))
		{
			CLEAR_VAR_BIT(timerHandlerFlag,7);
			LM75_ReadTempStr(crtLM75TempStr);
		}

		//��������ʱ��
		if (IS_SET_VAR_BIT(timerHandlerFlag,6))
		{
			CLEAR_VAR_BIT(timerHandlerFlag,6);
			PCF8563_Get_Str(rtcTempStr);
		}
		//��������Ȳ��
		if (IS_SET_VAR_BIT(timerHandlerFlag,5))
		{
			CLEAR_VAR_BIT(timerHandlerFlag, 5);
			Eth_Link_ITHandler(); //֧������δ�����ߺ������Ȳ�Ρ�
		}

		if (IS_SET_VAR_BIT(timerHandlerFlag, 2))
		{
			calibrating();
			CLEAR_VAR_BIT(timerHandlerFlag, 2);
		}

		if (IS_SET_VAR_BIT(timerHandlerFlag, 1))
		{
			ADC_Check_Handler(1);
			CLEAR_VAR_BIT(timerHandlerFlag, 1);
		}

		if (IS_SET_VAR_BIT(timerHandlerFlag, 0))
		{
			ADC_Check_Handler(0);
			CLEAR_VAR_BIT(timerHandlerFlag, 0);
		}

		delay_ms(100);
//		//д��־����
//		if(flag_fcgjsn == ON){
//			WriteLog(LOG_TYPE_AOPN, ALARM);
//			WriteLog(LOG_TYPE_AIVD, ALARM);
//			WriteLog(LOG_TYPE_BOPN, ALARM);
//			WriteLog(LOG_TYPE_BIVD, ALARM);
//			WriteLog(LOG_TYPE_FC, ALARM);
//		}

// 		//���Ի�����
//		LED_BU_FANG = 1;
//		delay_ms(100);
//		LED_BU_FANG = 0;
//		delay_ms(100);
//		OSMutexPend(iicMutexSem, (INT32U) IIC_MUTEX_TIMEOUT, &key);
//		LED_BU_FANG = 1;
//		delay_ms(1000);
//		LED_BU_FANG = 0;
//		OSMutexPost(iicMutexSem);

	}
}


//����ɨ������
void key_task(void *pdata)
{	
	u8 key;
	pdata=pdata;
	while(1)
	{
		key=KEY_Scan(0);
		if(key) OSMboxPost(msg_key, (void*)key);//������Ϣ
 		delay_ms(10);

// 		//���Ի�����
// 		LED_RUN = 1;
//		delay_ms(100);
// 		LED_RUN = 0;
// 		delay_ms(100);
// 		OSMutexPend(iicMutexSem, (INT32U)IIC_MUTEX_TIMEOUT, &key);
//		LED_RUN = 1;
//		delay_ms(1000);
//		LED_RUN = 0;
// 		OSMutexPost(iicMutexSem);

	}
}

//Ӳ��������
void HardFault_Handler(void)
{
	u32 i;
	u8 t=0;
	u32 temp;
	temp=SCB->CFSR;					//fault״̬�Ĵ���(@0XE000ED28)����:MMSR,BFSR,UFSR
 	printf("CFSR:%8X\r\n",temp);	//��ʾ����ֵ
	temp=SCB->HFSR;					//Ӳ��fault״̬�Ĵ���
 	printf("HFSR:%8X\r\n",temp);	//��ʾ����ֵ
 	temp=SCB->DFSR;					//����fault״̬�Ĵ���
 	printf("DFSR:%8X\r\n",temp);	//��ʾ����ֵ
   	temp=SCB->AFSR;					//����fault״̬�Ĵ���
 	printf("AFSR:%8X\r\n",temp);	//��ʾ����ֵ
 
 	while(t<5)
	{
		t++;
		LED_RUN=!LED_RUN;
		for(i=0;i<0X1FFFFF;i++);
 	}
 	LED_RUN = 0;

}
