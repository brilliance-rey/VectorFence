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

/////////////////////////UCOSII任务设置///////////////////////////////////
//START 任务
//设置任务优先级
#define START_TASK_PRIO      			10 //开始任务的优先级设置为最低
//设置任务堆栈大小
#define START_STK_SIZE  				64
//任务堆栈，8字节对齐	
__align(8) static OS_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *pdata);
 			   
////串口任务
////设置任务优先级
//#define USART_TASK_PRIO       			7 
////设置任务堆栈大小
//#define USART_STK_SIZE  		    	128
////任务堆栈，8字节对齐	
//__align(8) static OS_STK USART_TASK_STK[USART_STK_SIZE];
////任务函数
//void usart_task(void *pdata);
//							 
//主任务
//设置任务优先级
#define MAIN_TASK_PRIO       			6 
//设置任务堆栈大小
#define MAIN_STK_SIZE  					1200
//任务堆栈，8字节对齐	
__align(8) static OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
//任务函数
void main_task(void *pdata);

//按键扫描任务
//设置任务优先级
#define KEY_TASK_PRIO       			4
//设置任务堆栈大小
#define KEY_STK_SIZE  					64
//任务堆栈	
OS_STK KEY_TASK_STK[KEY_STK_SIZE];
//任务函数
void key_task(void *pdata);
//////////////////////////////////////////////////////////////////////////////
OS_EVENT * msg_key;			//按键邮箱事件块指针
//////////////////////////////////////////////////////////////////////////////	 

//监视任务
//设置任务优先级
#define WATCH_TASK_PRIO       			3
//设置任务堆栈大小
#define WATCH_STK_SIZE  		   		256
//任务堆栈，8字节对齐	
__align(8) static OS_STK WATCH_TASK_STK[WATCH_STK_SIZE];
//任务函数
void watch_task(void *pdata);
////////////////////////////////////////////////////////////////////////////////	 

//系统初始化
void system_init(void)
{

	delay_init(168);		  //初始化延时函数

	LCD_Init(); 			//LCD初始化
	DisplayInitMenu();//上电初始化菜单
	uart_init(9600);
	LED_Init();		        //初始化LED端口
	KEY_Init();  			//按键初始化
	
	my_mem_init(SRAMIN);		//初始化内部内存池
//	my_mem_init(SRAMEX);		//初始化外部内存池
	my_mem_init(SRAMCCM);	//初始化CCM内存池
	
	//注意一下代码的先后顺序,读取EEProm一定要在IIC_Init()后
	IIC_Init();		//IIC初始化
 	while(AT24CXX_Check()){
		LED_RUN=1;
	}
	LED_RUN=0;
	//***注意： EEProm初始化后，首先要读取configCode.
	readConfigCode();
	LM75_ReadTempStr(crtLM75TempStr);
	PCF8563_Get_Str(rtcTempStr);
	Log_Init();
	Adc_Init();         //初始化ADC
	ALARM_Init();
	
	TIM3_Int_Init(5000-1,8400-1);	//定时器时钟84M，分频系数8400，所以84M/8400=10Khz的计数频率，计数5000次为500ms     
	TIM4_Int_Init(10-1,84-1);	//84M/84=1Mhz的计数频率,计数10次为10us
//	TIM1_PWM_Init(500-1,84-1);	//84M/84=1Mhz的计数频率,重装载值500，所以PWM频率为 1M/500=2Khz.

}

int main(void)
{ 	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2

	system_init();		//系统初始化 
 	OSInit();

	lwip_comm_init();
	
// 	while(lwip_comm_init()) 	//lwip初始化 LwIP_Init一定要在OSInit之后和其他LWIP线程创建之前初始化!!!!!!!!  
//	{
//		delay_ms(500);
//	}
//	httpd_init();
	
 	OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//创建起始任务
 	tcp_server_init();
// 	tcp_client_init();

	OSStart();	  						    
}

//开始任务
void start_task(void *pdata)
{
	u8 err;
    OS_CPU_SR cpu_sr=0;
	pdata = pdata;

	msg_key=OSMboxCreate((void*)0);	//创建消息邮箱
	iicMutexSem = OSMutexCreate(IIC_MUTEX_PRIO, &err);		//或，iicSem = OSSemCreate(1);

	OSStatInit();		//初始化统计任务.这里会延时1秒钟左右	
	
	OS_ENTER_CRITICAL();//进入临界区(无法被中断打断)   
	 
//#if	LWIP_DHCP
//	lwip_comm_dhcp_creat();	//创建DHCP任务
//#endif
 	
 	OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);
 	
// 	OSTaskCreate(usart_task,(void *)0,(OS_STK*)&USART_TASK_STK[USART_STK_SIZE-1],USART_TASK_PRIO);
	OSTaskCreate(watch_task,(void *)0,(OS_STK*)&WATCH_TASK_STK[WATCH_STK_SIZE-1],WATCH_TASK_PRIO);
	
	OSTaskCreate(key_task,(void *)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);
   
	OSTaskSuspend(START_TASK_PRIO);	//挂起起始任务.
	OS_EXIT_CRITICAL();	//退出临界区(可以被中断打断)
}

//指示箭头
unsigned char arrows[]={
		0x00,0x08,0x30,0xE0,0xC0,0x80,0x00,0x00,0x00,0x20,0x18,0x0E,0x07,0x03,0x01,0x00,
};
//主任务
void main_task(void *pdata)
{

	u32 key=0;
	static u32 no_key_num = 0;	
	u8 err;	
	unsigned char current_screen = InitScreen;//
	current_screen = RunScreen;//上电默认处在运行界面
				    
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
		if((no_key_num == 50) && (current_screen == RunScreen))//在运行显示屏时，大约5秒没有按键按下
			{
				if(0 == (IS_SET_BIT(config_code, 2)? ALARM_STATE: ALARM_STATE_DELAY))  //如果无告警，回到显示屏
					{
						
					}
				else
					{
						current_screen = AlarmTypeListScreen;//有告警则调用告警显示屏
						row_point = 1;//行指针指向第二行
						DisplayAlarmTypeList();
					}
			}
		
		switch(current_screen)
		{
			case RunScreen://正常运行显示界面
				if(run_screen_refresh >= 10)
					{
						DisplayRunMenu();
						run_screen_refresh = 0;
					}
				switch(key)
				{
					case KEY_ENTER_PRESS:
						current_screen = UserLoginScreen;//当前屏为用户登录显示屏
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
								current_screen = RunScreen;//回到运行显示屏
								strcpy(inputPassword,"      ");
								strcpy(display_xinghao," $$$$$");
								DisplayRunMenu();
								break;

							case KEY_ENTER_PRESS:
								current_screen = InputPasswordScreen;//当前屏为输入密码显示屏
								strcpy(inputPassword,"0$$$$$");
								row_fx_pos = 0;//密码第一位反白显示
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

			case InputPasswordScreen://输入密码显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								current_screen = RunScreen;//回到运行显示屏
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
												row_point = 1;//行指针指向第二行
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
												current_screen = SuperUserRootScreen;//当前屏为超级用户根目录菜单
											}
										else
											{
												DisplayPasswordError();
												current_screen = PasswordErrorScreen;//当前屏为密码错误显示屏
											}
										break;
									case OrdinaryUser:
										if(strcmp(inputPassword,lcd_ordinary_password) == 0)
											{
												DisplayOrdinaryUserRootMenu();
												row_point = 1;//行指针指向第二行
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
												current_screen = OrdinaryUserRootScreen;//当前屏为普通用户根目录菜单
											}
										else
											{
												DisplayPasswordError();
												current_screen = PasswordErrorScreen;//当前屏为密码错误显示屏
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
								DisplayInputPassword();//只有在编辑当前页面，且不换页时刷新当前页面
								break;

							case KEY_DOWN_PRESS:
								inputPassword[row_fx_pos] --;
//								display_xinghao[row_fx_pos] --;
								if(inputPassword[row_fx_pos] < '0')
									{
										inputPassword[row_fx_pos] = '9';
//										display_xinghao[row_fx_pos] = '9';
									}
								DisplayInputPassword();//只有在编辑当前页面，且不换页时刷新当前页面
								break;

							case KEY_LEFT_PRESS:
								row_fx_pos --;
								if(row_fx_pos < 0)
									{
										row_fx_pos = 5;
									}
								inputPassword[row_fx_pos] = '0';
//								display_xinghao[row_fx_pos] = '*';
								DisplayInputPassword();//只有在编辑当前页面，且不换页时刷新当前页面
								break;

							case KEY_RIGHT_PRESS:
								row_fx_pos ++;
								if(row_fx_pos > 5)
									{
										row_fx_pos = 0;
									}
								inputPassword[row_fx_pos] = '0';
//								display_xinghao[row_fx_pos] = '*';
								DisplayInputPassword();//只有在编辑当前页面，且不换页时刷新当前页面
								break;

							default:
								break;
						}
					}
				break;

			case PasswordErrorScreen://密码错误显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
							case KEY_ENTER_PRESS:
								strcpy(inputPassword,"0$$$$$");
								strcpy(display_xinghao," $$$$$");
								row_fx_pos = 0;//密码第一位反白显示
								DisplayInputPassword();
								current_screen = InputPasswordScreen;//回到输入密码显示屏
								break;


							default:
								break;
						}
					}
				break;

			case SuperUserRootScreen://当前屏为超级用户根目录菜单
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								DisplayUserLogin();
								current_screen = UserLoginScreen;//回到用户登录界面
								strcpy(inputPassword,"0$$$$$");
								strcpy(display_xinghao," $$$$$");
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://设定
										DisplaySetMenu();
										row_point = 1;//行指针指向第二行
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetScreen;//当前屏为设定显示屏
										break;
									case 2://查询
										DisplayInquireMenu();
										row_point = 1;//行指针指向第二行
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqScreen;//当前屏为查询显示屏
										break;
									case 3://拉力监控
										DisplayTensionMonitor();
										row_point = 1;//行指针指向第二行
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = TensionMonitorScreen;//当前屏为拉力监控
										break;
									case 4://版本信息
										DisplayVersionMenu();
										current_screen = VerScreen;//当前屏为版本信息
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
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
									}
								break;
								
							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 4)
									{
										row_point = 1;
									}
								DisplaySuperUserRootMenu();
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
									}
								break;
								
							
							default:
								break;
						}
					}
				break;

			case OrdinaryUserRootScreen://当前屏为普通用户根目录菜单
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								DisplayUserLogin();
								current_screen = UserLoginScreen;//回到用户登录界面
								strcpy(inputPassword,"0$$$$$");
								strcpy(display_xinghao," $$$$$");
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://查询
										DisplayInquireMenu();
										row_point = 1;//行指针指向第二行
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqScreen;//当前屏为查询显示屏
										break;
									case 2://拉力监控
										DisplayTensionMonitor();
										row_point = 1;//行指针指向第二行
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = TensionMonitorScreen;//当前屏为拉力监控
										break;
									case 3://版本信息
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
			
			case SetScreen://设定显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySuperUserRootMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SuperUserRootScreen;//回到超级用户根目录菜单
								break;
							
							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://系统参数
										row_point = 1;//行指针指向第二行
										DisplaySetSysPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetSysScreen;//当前屏为设定系统参数菜单
										break;
									case 2://防区参数
										row_point = 1;//行指针指向第二行
										DiaplaySetFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetFangQuParaScreen;//当前屏为设定防区参数菜单
										break;
									case 3://校准设置
										row_point = 1;//行指针指向第二行
										DiaplayCalibrationSet();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = CalibrationSetScreen;//当前屏为校准设置菜单
										break;
									case 4://布防参数
										row_point = 1;//行指针指向第二行
										DiaplaySetBuFangPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetBuFangParaScreen;//当前屏为设定布防参数菜单
										break;
									case 5://告警阈值
										row_point = 1;//行指针指向第二行
										DiaplaySetAlarmThreshold();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetAlarmThresholdScreen;//当前屏为设定告警阈值参数菜单
										break;
									case 6://清除记录
										row_point = 1;//行指针指向第二行
										DisplaySetClearRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetClearRecordScreen;//当前屏为清除记录菜单
										break;
									case 7://兼容外部设备
										row_point = 1;//行指针指向第二行
										flag_PageState = NormalState;
										DiaplaySetExtDeviceTypeScreen();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//显示行指针
										current_screen = SetExtDeviceTypeScreen;//当前屏为设定外接设备类型菜单
										break;
									case 8://满量程拉力值
										sprintf(tension_max_range_temp,"%03d",tension_max_range / 10);
										row_fx_pos = 1;//第一个位置反白显示
										DisplaySetFullScale();
										current_screen = SetFullScaleScreen;//当前屏为设置满量程拉力值
										break;
									case 9://基准自校时间  
										sprintf(base_auto_calibrate_time_tmp,"%05d",base_auto_calibrate_time);
										row_fx_pos = 1;//第一个位置反白显示
										DisplaySetBaseAutoCalibrateTime();
										current_screen = SetBaseAutoCalibrateTimeScreen;//当前屏为基准值自动校准时间
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
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
									}
								break;
							
							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 9)
									{
										row_point = 1;
									}
								DisplaySetMenu();
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
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
			case InqScreen://查询显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								if(UserId == SuperUser)
									{
										row_point = 1;//行指针指向第二行
										DisplaySuperUserRootMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SuperUserRootScreen;//回到超级用户根目录菜单
									}
								else
									{
										row_point = 1;//行指针指向第二行
										DisplayOrdinaryUserRootMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = OrdinaryUserRootScreen;//回到普通用户根目录菜单
									}
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://系统参数
										row_point = 1;//行指针指向第二行
										DisplayInqSysPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqSysParaScreen;//当前屏为查询系统参数菜单
										break;
									case 2://防区参数
										row_point = 1;//行指针指向第二行
										DiaplayInqFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqFangQuParaScreen;//当前屏为查询防区参数菜单
										break;
									case 3://布防参数
										row_point = 1;//行指针指向第二行
										DisplayInqBuFangPara();
//										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqBuFangParaScreen;//当前屏为查询布防参数菜单
										break;
									case 4://告警记录
										row_point = 1;//行指针指向第二行
										DisplayInqAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqAlarmRecordScreen;//当前屏为查询告警记录菜单
										break;
									case 5://告警阈值
										DisplayInqAlarmThreshold();
										current_screen = InqAlarmThresholdScreen;//当前屏为查询告警阈值菜单
										break;
									case 6://校准值
										row_point = 1;//行指针指向第二行
										DisplayInqCalibrationVal();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqCalibrationValScreen;//当前屏为查询校准值菜单
										break;
									case 7://兼容外部设备
										DisplayInqExtDeviceType();
										current_screen = InqExtDeviceTypeScreen;//当前屏为查询兼容外部设备菜单
										break;
									case 8://满量程拉力值
										DisplayInqFullScale();
										current_screen = InqFullScaleScreen;//当前屏为查询满量程拉力值菜单
										break;
									case 9://基准值自动校准时间
										DisplayInqBaseAutoCalibrateTime();
										current_screen = InqBaseAutoCalibrateTimeScreen;//当前屏为查询基准值自动校准时间菜单
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
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
									}
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 9)
									{
										row_point = 1;
									}
								DisplayInquireMenu();
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
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
			
			case TensionMonitorScreen://拉力监控显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								if(UserId == SuperUser)
									{
										row_point = 1;//行指针指向第二行
										DisplaySuperUserRootMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SuperUserRootScreen;//回到超级用户根目录菜单
									}
								else
									{
										row_point = 1;//行指针指向第二行
										DisplayOrdinaryUserRootMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = OrdinaryUserRootScreen;//回到普通用户根目录菜单
									}
								break;
								
							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A区
										DiaplayAFangQuMonitor();
										current_screen = AFangQuMonitorScreen;//当前屏为A防区拉力监控菜单
										break;
									case 2://B区
										DiaplayBFangQuMonitor();
										current_screen = BFangQuMonitorScreen;//当前屏为B防区拉力监控菜单
										break;
									default:
										break;
								}
								break;
								
							case KEY_UP_PRESS:
								row_point --;
								if(FIELD_FLAG_S0D1)
									{//单防区不显示B防区
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
									{//单防区不显示B防区
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
			
			case VerScreen://版本号显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								if(UserId == SuperUser)
									{
										row_point = 1;//行指针指向第二行
										DisplaySuperUserRootMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SuperUserRootScreen;//回到超级用户根目录菜单
									}
								else
									{
										row_point = 1;//行指针指向第二行
										DisplayOrdinaryUserRootMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = OrdinaryUserRootScreen;//回到普通用户根目录菜单
									}
								break;

							default:
								break;
						}
					}
				break;
			case SetSysScreen://设定系统参数显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetScreen;//回到设定菜单
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://日期时间
										row_point = 1;//行指针指向第二行
										DisplaySetDateTime();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetDateTimeScreen;//当前屏为设定日期时间菜单
										break;
									case 2://网络参数
										row_point = 1;//行指针指向第二行
										DisplaySetEthPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetEthParaScreen;//当前屏为设定网络参数菜单
										break;
//									case 3://RS485参数
//										current_screen = SetRS485ParaScreen;//当前屏为设定RS485参数菜单
//										row_point = 1;//行指针指向第二行
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

			case SetFangQuParaScreen://设定防区参数显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetScreen;//回到设定菜单
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A区
										row_point = 1;//行指针指向第二行
										DisplaySetAFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetAFangQuParaScreen;//当前屏为设置A防区参数菜单
										break;
									case 2://B区
										row_point = 1;//行指针指向第二行
										DisplaySetBFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetBFangQuParaScreen;//当前屏为设置B防区参数菜单
										break;
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(FIELD_FLAG_S0D1)
									{//单防区不显示B防区
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
									{//单防区不显示B防区
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
			case CalibrationSetScreen://校准设置显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetScreen;//回到设定菜单
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://零点值校准
										row_point = 1;//行指针指向第二行
										DisplayZeroCalibration();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ZeroCalibrationScreen;//当前屏为零点值校准菜单
										break;
									case 2://基准值校准
										row_point = 1;//行指针指向第二行
										DisplayDatumCalibration();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = DatumCalibrationScreen;//当前屏为基准值校准菜单
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
			case SetBuFangParaScreen://设定布防参数显示屏
				if(key != KEY_NO_PRESS)
					{
						if(flag_PageState)//判断页面状态，正常显示时：
							{
								switch(key)
								{
									case KEY_ESC_PRESS://按退出键
										row_point = 1;//行指针指向第二行
										DisplaySetMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetScreen;//回到设定菜单
										break;

									case KEY_ENTER_PRESS:
										BU_FANG_A_FLAG_TEMP = BU_FANG_A_FLAG;
										BU_FANG_B_FLAG_TEMP = BU_FANG_B_FLAG;
										flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
										switch(row_point)
										{
											case 1://A防区编辑
												//A防区开关,或高压等级反白显示，可编辑
												BU_FANG_A_FLAG_TEMP = BU_FANG_A_FLAG;//拷贝到暂存
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												row_fx_pos = 1;//第一个反白显示位置
												break;
											case 2://B防区编辑
												//B防区开关,或高压等级反白显示，可编辑
												BU_FANG_B_FLAG_TEMP = BU_FANG_B_FLAG;//拷贝到暂存
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												row_fx_pos = 1;//第一个反白显示位置
												break;
											case 3://防拆告警:开启/关闭
												//防拆告警开关反白显示，可编辑
												flag_fcgjsn_tmp = flag_fcgjsn;//拷贝到暂存
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												break;
											default:
												break;
										}
										DiaplaySetBuFangPara();
										break;

									case KEY_UP_PRESS:
										row_point --;
										if(FIELD_FLAG_S0D1)
											{//单防区不显示B防区
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
											{//单防区不显示B防区
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
						else//判断页面状态，编辑显示时：
							{
								//不显示行指针
								switch(key)
								{
									case KEY_ESC_PRESS://按退出键
										flag_PageState = NormalState;//页面状态由编辑显示模式转换为正常显示模式
										flag_fcgjsn_tmp = flag_fcgjsn;//页面正常显示时要将flag_fcgjsn赋值给flag_fcgjsn_tmp，否则再次进入编辑显示会不对
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
										flag_PageState = NormalState;//页面状态由编辑显示模式转换为正常显示模式
										row_fx_pos = 0;//无反白显示内容
										break;

									case KEY_UP_PRESS:
										//判断是防区开关，还是高压等级，还是防拆告警开关
										switch(row_point)
										{
											case 1://编辑第一行（A防区开关及高压等级）
												switch(row_fx_pos)
												{
													case 1://A防区开关选择
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
											case 2://编辑第二行（B防区开关及高压等级）
												switch(row_fx_pos)
												{
													case 1://B防区开关选择
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
											case 3://编辑第三行（防拆使能开关）
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
										//判断是防区开关，还是高压等级，还是防拆告警开关
										switch(row_point)
										{
											case 1://编辑第一行（A防区开关及高压等级）
												switch(row_fx_pos)
												{
													case 1://A防区开关选择
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
											case 2://编辑第二行（B防区开关及高压等级）
												switch(row_fx_pos)
												{
													case 1://B防区开关选择
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
											case 3://编辑第三行（防拆使能开关）
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
										//反白编辑左移
										row_fx_pos --;
										if(row_fx_pos < 1)
											{
												row_fx_pos = 2;
											}
										DiaplaySetBuFangPara();
										break;

									case KEY_RIGHT_PRESS:
										//反白编辑右移
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
			case SetAlarmThresholdScreen://设定告警阈值参数显示屏
				if(key != KEY_NO_PRESS)
					{
						if(flag_PageState)//判断页面状态，正常显示时：
							{
								switch(key)
								{
									case KEY_ESC_PRESS://按退出键
										row_point = 1;//行指针指向第二行
										DisplaySetMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetScreen;//回到设定菜单
										break;
										
									case KEY_ENTER_PRESS:
										flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
										switch(row_point)
										{
											case 1://上限偏移
												//上限偏移反白显示，可编辑
												alarm_threshold_up_dif_temp = alarm_threshold_up_dif;//拷贝到暂存
												row_fx_pos = 1;//第一个反白显示位置
												break;
											case 2://下限偏移
												//下限偏移反白显示，可编辑
												alarm_threshold_down_dif_temp = alarm_threshold_down_dif;//拷贝到暂存
												row_fx_pos = 1;//第一个反白显示位置
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
						else//判断页面状态，编辑显示时：
							{
								//不显示行指针
								switch(key)
								{
									case KEY_ESC_PRESS://按退出键
										flag_PageState = NormalState;//页面状态由编辑显示模式转换为正常显示模式
										alarm_threshold_up_dif_temp = alarm_threshold_up_dif;
										alarm_threshold_down_dif_temp = alarm_threshold_down_dif;
										DiaplaySetAlarmThreshold();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										break;
										
									case KEY_ENTER_PRESS:
										//保存参数
										alarm_threshold_up_dif = alarm_threshold_up_dif_temp;
										alarm_threshold_down_dif = alarm_threshold_down_dif_temp;
										setAlmThrdUpDif(alarm_threshold_up_dif);
										setAlmThrdDwDif(alarm_threshold_down_dif);
										flag_PageState = NormalState;//页面状态由编辑显示模式转换为正常显示模式
										row_fx_pos = 0;//无反白显示内容
										DiaplaySetAlarmThreshold();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//显示行指针
										break;
										
									case KEY_UP_PRESS:
										switch(row_point)
										{
											case 1://编辑第一行（上阈值）
												switch(row_fx_pos)
												{
													case 1://上偏移的整数位
														if((alarm_threshold_up_dif_temp / 10) < 49)
															{
																alarm_threshold_up_dif_temp += 10;
															}
														else
															{
																alarm_threshold_up_dif_temp %= 10;
															}
														break;
													case 2://上偏移的小数位
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
											case 2://编辑第二行（下阈值）
												switch(row_fx_pos)
												{
													case 1://下偏移的整数位
														if((alarm_threshold_down_dif_temp / 10) < 49)
															{
																alarm_threshold_down_dif_temp += 10;
															}
														else
															{
																alarm_threshold_down_dif_temp %= 10;
															}
														break;
													case 2://下偏移的小数位
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
											case 1://编辑第一行（上偏移）
												switch(row_fx_pos)
												{
													case 1://上偏移的整数位
														if((alarm_threshold_up_dif_temp / 10) > 1)
															{
																alarm_threshold_up_dif_temp -= 10;
															}
														else
															{
																alarm_threshold_up_dif_temp = 490 + alarm_threshold_up_dif_temp % 10;
															}
														break;
													case 2://上偏移的小数位
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
											case 2://编辑第二行（下偏移）
												switch(row_fx_pos)
												{
													case 1://下偏移的整数位
														if((alarm_threshold_down_dif_temp / 10) > 1)
															{
																alarm_threshold_down_dif_temp -= 10;
															}
														else
															{
																alarm_threshold_down_dif_temp = 490 + alarm_threshold_down_dif_temp % 10;
															}
														break;
													case 2://下偏移的小数位
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
										//反白编辑左移
										row_fx_pos --;
										if(row_fx_pos < 1)
											{
												row_fx_pos = 2;
											}
										DiaplaySetAlarmThreshold();
										break;
										
									case KEY_RIGHT_PRESS:
										//反白编辑右移
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
			case SetClearRecordScreen://清除记录显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetScreen;//回到设定菜单
								break;
								
							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://短路告警
										row_point = 1;//行指针指向第二行
										DisplayClearRuQinAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ClearDuanLuAlarmRecordScreen;//清除短路告警记录显示屏
										break;
									case 2://偏移告警
										row_point = 1;//行指针指向第二行
										DisplayClearDuanXianAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ClearDuanXianAlarmRecordScreen;//清除断路告警记录显示屏
										break;
									case 3://防拆告警
										DisplayZhengZaiQingChu();
										//擦除EEPROM
										ClearLog(LOG_TYPE_FC);
										row_point = 1;//行指针指向第二行
										DisplaySetClearRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetClearRecordScreen;//回到清除记录显示屏
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
			case InqSysParaScreen://查询系统参数显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//回到查询显示屏
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://网络参数
										row_point = 1;//行指针指向第二行
										DisplayInqEthPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqEthParaScreen;//当前屏为查询网络参数显示菜单
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
			case InqFangQuParaScreen://查询防区参数显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//回到查询显示屏
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A区
										row_point = 1;//行指针指向第二行
										DiaplayInqAFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqAFangQuParaScreen;//当前屏为查询A防区参数菜单
										break;
									case 2://B区
										row_point = 1;//行指针指向第二行
										DiaplayInqBFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqBFangQuParaScreen;//当前屏为查询B防区参数菜单
										break;
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(FIELD_FLAG_S0D1)
									{//单防区不显示B防区
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
									{//单防区不显示B防区
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
			case InqBuFangParaScreen://查询布防参数显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//回到查询显示屏

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
			case InqAlarmRecordScreen://查询告警记录显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//回到查询显示屏
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://短路告警
										row_point = 1;//行指针指向第二行
										DisplayInqRuQinAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqDuanLuAlarmScreen;//当前屏为查询短路告警菜单
										break;
									case 2://断路告警
										row_point = 1;//行指针指向第二行
										DisplayInqDuanXianAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqDuanXianAlarmScreen;//当前屏为查询断路告警菜单
										break;
									case 3://防拆告警
										row_point = 1;//行指针指向第二行
										ReadLCDFirstPageLog(LOG_TYPE_FC);
										DisplayInqFangChaiAlarmRecord();
//										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqFangChaiAlarmRecordScreen;//当前屏为查询防拆告警菜单
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
			case InqAlarmThresholdScreen://查询告警阈值显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//回到查询显示屏
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
			case InqCalibrationValScreen://查询校准值显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//回到查询显示屏
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://零点值
										DisplayInqZeroVal();//显示零点校准值
										row_point = 1;//行指针指向第二行
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqZeroValScreen;//当前屏为查询零点校准值菜单
										break;
									case 2://基准值
										DiaplayInqDatumVal();//显示基准校准值
										row_point = 1;//行指针指向第二行
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = InqDatumValScreen;//当前屏为查询基准校准值菜单
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
			case InqZeroValScreen://查询零点值显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInqCalibrationVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqCalibrationValScreen;//回到查询校准值显示屏
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A防区
										DisplayAareaZeroVal();//显示A防区零点校准值
										current_screen = InqAareaZeroValScreen;//当前屏为查询A防区零点值显示屏
										break;
									case 2://B防区
										DisplayBareaZeroVal();//显示B防区零点校准值
										current_screen = InqBareaZeroValScreen;//当前屏为查询B防区零点值显示屏
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
			case InqDatumValScreen://查询基准值显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInqCalibrationVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqCalibrationValScreen;//回到查询校准值显示屏
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A防区
										DisplayAareaDatumVal();//显示A防区基准校准值
										current_screen = InqAareaDatumValScreen;//当前屏为查询A防区基准值显示屏
										break;
									case 2://B防区
										DisplayBareaDatumVal();//显示B防区基准校准值
										current_screen = InqBareaDatumValScreen;//当前屏为查询B防区基准值显示屏
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
			case InqAareaZeroValScreen://查询A防区零点值显示屏
			case InqBareaZeroValScreen://查询B防区零点值显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInqZeroVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqZeroValScreen;//回到查询零点值显示屏
								break;

							case KEY_ENTER_PRESS:
								row_point = 1;//行指针指向第二行
								DisplayInqZeroVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqZeroValScreen;//回到查询零点值显示屏
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
			case InqAareaDatumValScreen://查询A防区基准值显示屏
			case InqBareaDatumValScreen://查询B防区基准值显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DiaplayInqDatumVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqDatumValScreen;//回到查询基准值显示屏
								break;

							case KEY_ENTER_PRESS:
								row_point = 1;//行指针指向第二行
								DiaplayInqDatumVal();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqDatumValScreen;//回到查询基准值显示屏
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
			
			
			
			case InqExtDeviceTypeScreen://查询兼容外部设备显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//回到查询显示屏
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
			
			case InqFullScaleScreen://查询满量程拉力值显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//回到查询显示屏
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
			
			case InqBaseAutoCalibrateTimeScreen://查询基准值自动校准时间显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInquireMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqScreen;//回到查询显示屏
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
			
			case AFangQuMonitorScreen://A防区拉力监控显示屏
				DiaplayAFangQuMonitor();//需要一直刷新显示值
				delay_ms(500);
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								row_point = 1;//行指针指向第二行
								DisplayTensionMonitor();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = TensionMonitorScreen;//回到拉力监控显示屏
								break;
							
							default:
								break;
						}
					}
				break;
			
			case BFangQuMonitorScreen://B防区拉力监控显示屏
				DiaplayBFangQuMonitor();//需要一直刷新显示值
				delay_ms(500);
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS:
								row_point = 1;//行指针指向第二行
								DisplayTensionMonitor();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = TensionMonitorScreen;//回到拉力监控显示屏
								break;
							
							default:
								break;
						}
					}
				break;
			
			case SetDateTimeScreen://设定日期时间显示屏
				if(key != KEY_NO_PRESS)
					{
						if(flag_PageState)//判断页面状态，正常显示时：
							{
								switch(key)
								{
									case KEY_ESC_PRESS://按退出键
										row_point = 1;//行指针指向第二行
										DisplaySetSysPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetSysScreen;//回到设定系统参数显示屏
										row_fx_pos = 0;//无反白显示内容
										break;

									case KEY_ENTER_PRESS:
										switch(row_point)
										{
											case 1://年/月/日
												//年反白显示，可编辑

												memcpy(rtcTempStr_temp_test,rtcTempStr,sizeof(rtcTempStr));//拷贝实时时间到时间暂存
		//										memcpy(rtcTempStr_temp,rtcTempStr,sizeof(rtcTempStr));//拷贝实时时间到时间暂存
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												row_fx_pos = 1;//第一个反白显示位置
												break;
											case 2://时/分/秒
												//时反白显示，可编辑
												memcpy(rtcTempStr_temp_test,rtcTempStr,sizeof(rtcTempStr));//拷贝实时时间到时间暂存
		//										memcpy(rtcTempStr_temp,rtcTempStr,sizeof(rtcTempStr));//拷贝实时时间到时间暂存
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												row_fx_pos = 1;//第一个反白显示位置
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
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//显示行指针
										break;

									case KEY_DOWN_PRESS:
										row_point ++;
										if(row_point > 2)
											{
												row_point = 1;
											}
										DisplaySetDateTime();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//显示行指针
										break;

									case KEY_LEFT_PRESS:
										break;

									case KEY_RIGHT_PRESS:
										break;
									default:
										break;
								}
							}
						else//判断页面状态，编辑显示时：
							{
								//不显示行指针

								switch(key)
								{
									case KEY_ESC_PRESS://按退出键
										flag_PageState = NormalState;//页面状态由编辑显示模式转换为正常显示模式
										memcpy(rtcTempStr_temp_test,rtcTempStr,sizeof(rtcTempStr));//拷贝实时时间字符串到时间暂存
										row_fx_pos = 0;//无反白显示内容
										DisplaySetDateTime();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//显示行指针
										break;

									case KEY_ENTER_PRESS:
										//去设置PCF8563
										memcpy(rtcTempStr,rtcTempStr_temp_test,sizeof(rtcTempStr_temp_test));//拷贝实时时间暂存字符串到时间
										PCF8563_Set_Str(rtcTempStr);
										flag_PageState = NormalState;//页面状态由编辑显示模式转换为正常显示模式
										row_fx_pos = 0;//无反白显示内容
										DisplaySetDateTime();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//显示行指针
										break;

									case KEY_UP_PRESS:
										//判断年月日时分秒后向上加
										switch(row_point)
										{
											case 1://编辑第一行
												switch(row_fx_pos)
												{
													case 1://第一行，左起第一个位置（年）
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
													case 2://第一行，左起第二个位置（月）
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
													case 3://第一行，左起第三个位置（日）
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
											case 2://编辑第二行
												switch(row_fx_pos)
												{
													case 1://第二行，左起第一个位置（时）
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
													case 2://第二行，左起第二个位置（分）
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
													case 3://第二行，左起第三个位置（秒）
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
										//判断年月日时分秒后向下减
										switch(row_point)
										{
											case 1://编辑第一行
												switch(row_fx_pos)
												{
													case 1://第一行，左起第一个位置（年）
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
													case 2://第一行，左起第二个位置（月）
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
													case 3://第一行，左起第三个位置（日）
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
											case 2://编辑第二行
												switch(row_fx_pos)
												{
													case 1://第二行，左起第一个位置（时）
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
													case 2://第二行，左起第二个位置（分）
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
													case 3://第二行，左起第三个位置（秒）
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
										//反白编辑左移
										row_fx_pos --;
										if(row_fx_pos < 1)
											{
												row_fx_pos = 3;
											}
										DisplaySetDateTime();
										break;

									case KEY_RIGHT_PRESS:
										//反白编辑右移
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
			case SetEthParaScreen://设定网络参数显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetSysPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetSysScreen;//回到设定系统参数显示屏
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://MAC地址
										row_point = 1;//行指针指向第二行
										DisplaySetMacAddr();
										MacAddrInsertMaohao();
										current_screen = SetMacAddrScreen;//当前屏为设定Mac地址菜单
										break;
									case 2://IP地址
										GetIpAddr();
										row_point = 1;//行指针指向第二行
										row_fx_pos = 1;//第一个位置反白显示
										DisplaySetIpAddr();
										current_screen = SetIpAddrScreen;//当前屏为设定IP地址菜单
										break;
									case 3://子网掩码
										GetSubNetMask();
										row_point = 1;//行指针指向第二行
										row_fx_pos = 1;//第一个位置反白显示
										DisplaySetSubnetMask();
										current_screen = SetSubnetMaskScreen;//当前屏为设定子网掩码菜单
										break;
									case 4://网关
										GetGateWay();
										row_point = 1;//行指针指向第二行
										row_fx_pos = 1;//第一个位置反白显示
										DisplaySetGateWay();
										current_screen = SetGateWayScreen;//当前屏为设定网关菜单
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
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
									}
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 4)
									{
										row_point = 1;
									}
								DisplaySetEthPara();
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
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
			case SetAFangQuParaScreen://设定A防区参数显示屏
				if(key != KEY_NO_PRESS)
					{
						if(flag_PageState)//判断页面状态，正常显示时：
							{
								switch(key)
								{
									case KEY_ESC_PRESS://按退出键
										row_point = 1;//行指针指向第二行
										DiaplaySetFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetFangQuParaScreen;//回到设定防区参数显示屏
										row_fx_pos = 0;//无反白显示内容
										break;

									case KEY_ENTER_PRESS:
										memcpy(ten_val_range_temp,ten_val_range,sizeof(ten_val_range_temp));//拷贝有效拉力范围到暂存（必须写到此处，不然显示屏会报非法错误）
										memcpy(alarm_sensitivity_temp,alarm_sensitivity,sizeof(alarm_sensitivity_temp));//拷贝灵敏度到暂存（必须写到此处，不然显示屏会报非法错误）
										switch(row_point)
										{
											case 1://防区号
												//防区号反白显示，可编辑
												memcpy(field_num_temp,field_num,sizeof(field_num_temp));//拷贝防区号到暂存
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												row_fx_pos = 1;//第一个反白显示位置
												break;
											case 2://告警延时
												//告警延时反白显示，可编辑
												memcpy(alarm_delay_temp,alarm_delay,sizeof(alarm_delay_temp));//拷贝告警延时到暂存
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												row_fx_pos = 1;//第一个反白显示位置
												break;
											case 3://灵敏度
												//灵敏度反白显示，可编辑
//												memcpy(alarm_sensitivity_temp,alarm_sensitivity,sizeof(alarm_sensitivity_temp));//拷贝灵敏度到暂存
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												row_fx_pos = 1;//第一个反白显示位置
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
										if(row_point > 3)//如果选项多于3行
											{
												Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
											}
										else
											{
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
											}
										break;
										
									case KEY_DOWN_PRESS:
										row_point ++;
										if(row_point > 3)
											{
												row_point = 1;
											}
										DisplaySetAFangQuPara();
										if(row_point > 3)//如果选项多于3行
											{
												Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
											}
										else
											{
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
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
						else//判断页面状态，编辑显示时：
							{
								//不显示行指针
								switch(key)
								{
									case KEY_ESC_PRESS://按退出键
										flag_PageState = NormalState;//页面状态由编辑显示模式转换为正常显示模式
										row_fx_pos = 0;//无反白显示内容
										row_point = 1;//行指针指向第二行
										DisplaySetAFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//显示行指针
										break;

									case KEY_ENTER_PRESS:
										//
										memcpy(field_num,field_num_temp,sizeof(field_num_temp));//拷贝防区号暂存到防区号
										memcpy(alarm_delay,alarm_delay_temp,sizeof(alarm_delay_temp));//拷贝告警延时暂存到告警延时
										memcpy(alarm_sensitivity,alarm_sensitivity_temp,sizeof(alarm_sensitivity_temp));//拷贝灵敏度暂存到灵敏度
//										if(ten_val_range_temp[0][0] < ten_val_range_temp[0][1])
//											{
//												memcpy(ten_val_range[0],ten_val_range_temp[0],sizeof(ten_val_range_temp[0]));//拷贝有效拉力范围暂存到有效拉力范围
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
										flag_PageState = NormalState;//页面状态由编辑显示模式转换为正常显示模式
										row_fx_pos = 0;//无反白显示内容
										DisplaySetAFangQuPara();
										if(row_point > 3)//如果行指针大于3行，则始终保持在最后一行
											{
												Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
											}
										else
											{
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
											}
										break;
										
									case KEY_UP_PRESS:
										//判断防区号和告警延时向上加
										switch(row_point)
										{
											case 1://编辑第一行，防区号
												switch(row_fx_pos)
												{
													case 1://第一个反白位置
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
											case 2://编辑第二行，告警延时
												switch(row_fx_pos)
												{
													case 1://第一个反白位置
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
											case 3://编辑第三行，灵敏度
												switch(row_fx_pos)
												{
													case 1://第一个反白位置
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
										//判断防区号和告警延时向下减
										switch(row_point)
										{
											case 1://编辑第一行，防区号
												switch(row_fx_pos)
												{
													case 1://第一个反白位置
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
											case 2://编辑第二行
												switch(row_fx_pos)
												{
													case 1://第一个反白位置
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
											case 3://编辑第三行，灵敏度
												switch(row_fx_pos)
												{
													case 1://第一个反白位置
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
										//反白编辑左移
										row_fx_pos --;
										if(row_fx_pos < 1)
											{
												row_fx_pos = 2;
											}
										DisplaySetAFangQuPara();
										break;
										
									case KEY_RIGHT_PRESS:
										//反白编辑右移
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
			case SetBFangQuParaScreen://设定B防区参数显示屏
				if(key != KEY_NO_PRESS)
					{
						if(flag_PageState)//判断页面状态，正常显示时：
							{
								switch(key)
								{
									case KEY_ESC_PRESS://按退出键
										row_point = 1;//行指针指向第二行
										DiaplaySetFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetFangQuParaScreen;//回到设定防区参数显示屏
										row_fx_pos = 0;//无反白显示内容
										break;

									case KEY_ENTER_PRESS:
										memcpy(alarm_sensitivity_temp,alarm_sensitivity,sizeof(alarm_sensitivity_temp));//拷贝灵敏度到暂存（必须写到此处，不然显示屏会报非法错误）
										switch(row_point)
										{
											case 1://防区号
												//防区号反白显示，可编辑
												memcpy(field_num_temp,field_num,sizeof(field_num_temp));//拷贝防区号到暂存
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												row_fx_pos = 1;//第一个反白显示位置
												break;
											case 2://告警延时
												//告警延时反白显示，可编辑
												memcpy(alarm_delay_temp,alarm_delay,sizeof(alarm_delay_temp));//拷贝告警延时到暂存
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												row_fx_pos = 1;//第一个反白显示位置
												break;
											case 3://灵敏度
												//灵敏度反白显示，可编辑
//												memcpy(alarm_sensitivity_temp,alarm_sensitivity,sizeof(alarm_sensitivity_temp));//拷贝灵敏度到暂存
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												row_fx_pos = 1;//第一个反白显示位置
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
										if(row_point > 3)//如果选项多于3行
											{
												Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
											}
										else
											{
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
											}
										break;
										
									case KEY_DOWN_PRESS:
										row_point ++;
										if(row_point > 3)
											{
												row_point = 1;
											}
										DisplaySetBFangQuPara();
										if(row_point > 3)//如果选项多于3行
											{
												Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
											}
										else
											{
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
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
						else//判断页面状态，编辑显示时：
							{
								//不显示行指针
								switch(key)
								{
									case KEY_ESC_PRESS://按退出键
										flag_PageState = NormalState;//页面状态由编辑显示模式转换为正常显示模式
										row_fx_pos = 0;//无反白显示内容
										row_point = 1;//行指针指向第二行
										DisplaySetBFangQuPara();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//显示行指针
										break;

									case KEY_ENTER_PRESS:
										//
										memcpy(field_num,field_num_temp,sizeof(field_num_temp));//拷贝防区号暂存到防区号
										memcpy(alarm_delay,alarm_delay_temp,sizeof(alarm_delay_temp));//拷贝告警延时暂存到告警延时
										memcpy(alarm_sensitivity,alarm_sensitivity_temp,sizeof(alarm_sensitivity_temp));//拷贝灵敏度暂存到灵敏度
//										if(ten_val_range_temp[1][0] < ten_val_range_temp[1][1])
//											{
//												memcpy(ten_val_range[1],ten_val_range_temp[1],sizeof(ten_val_range_temp[1]));//拷贝有效拉力范围暂存到有效拉力范围
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
										flag_PageState = NormalState;//页面状态由编辑显示模式转换为正常显示模式
										row_fx_pos = 0;//无反白显示内容
										DisplaySetBFangQuPara();
										if(row_point > 3)//如果行指针大于3行，则始终保持在最后一行
											{
												Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
											}
										else
											{
												Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
											}
										break;
										
									case KEY_UP_PRESS:
										//判断防区号和告警延时向上加
										switch(row_point)
										{
											case 1://编辑第一行，防区号
												switch(row_fx_pos)
												{
													case 1://第一个反白位置
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
											case 2://编辑第二行，告警延时
												switch(row_fx_pos)
												{
													case 1://第一个反白位置
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
											case 3://编辑第三行，灵敏度
												switch(row_fx_pos)
												{
													case 1://第一个反白位置
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
										//判断防区号和告警延时向下减
										switch(row_point)
										{
											case 1://编辑第一行，防区号
												switch(row_fx_pos)
												{
													case 1://第一个反白位置
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
											case 2://编辑第二行
												switch(row_fx_pos)
												{
													case 1://第一个反白位置
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
											case 3://编辑第三行，灵敏度
												switch(row_fx_pos)
												{
													case 1://第一个反白位置
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
										//反白编辑左移
										row_fx_pos --;
										if(row_fx_pos < 1)
											{
												row_fx_pos = 2;
											}
										DisplaySetBFangQuPara();
										break;
										
									case KEY_RIGHT_PRESS:
										//反白编辑右移
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
			case ZeroCalibrationScreen://零点值校准显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DiaplayCalibrationSet();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = CalibrationSetScreen;//回到校准设置显示屏
								row_fx_pos = 0;//无反白显示内容
								break;
		
							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A防区
										//A防区正在零点校准
										updateZeroVal(0);
										Displayzhengzaijiaozhun();
										current_screen = AzeroCalibrationScreen;//跳转到A防区零点校准显示屏
										break;
									case 2://B防区
										//B防区正在零点校准
										updateZeroVal(1);
										Displayzhengzaijiaozhun();
										current_screen = BzeroCalibrationScreen;//跳转到B防区零点校准显示屏
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
			case DatumCalibrationScreen://基准值校准显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DiaplayCalibrationSet();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = CalibrationSetScreen;//回到校准设置显示屏
								row_fx_pos = 0;//无反白显示内容
								break;
		
							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A防区
										if(!calibrate_en[0])
											{
												DisplayNotAtValidRange();
												current_screen = NotAtValidRangeScreen;//跳转到未在有效范围显示屏
											}
										else
											{
												//A防区正在基准值校准
												updateBaseVal(0);
												Displayzhengzaijiaozhun();
												current_screen = AdatumCalibrationScreen;//跳转到A防区基准校准显示屏
											}
										break;
									case 2://B防区
										if(!calibrate_en[1])
											{
												DisplayNotAtValidRange();
												current_screen = NotAtValidRangeScreen;//跳转到未在有效范围显示屏
											}
										else
											{
												//B防区正在基准值校准
												updateBaseVal(1);
												Displayzhengzaijiaozhun();
												current_screen = BdatumCalibrationScreen;//跳转到B防区基准校准显示屏
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
			case AzeroCalibrationScreen://A防区零点校准显示屏，显示“正在校准，请稍后！”
				if(calibrating_flag == 0xff)//校准完成
					{
						DisplayAareaZeroVal();
						current_screen = CaliOverAareaZeroValScreen;//零点校准完成后显示的A防区零点值显示屏
					}
				break;
			case BzeroCalibrationScreen://B防区零点校准显示屏，显示“正在校准，请稍后！”
				if(calibrating_flag == 0xff)//校准完成
					{
						DisplayBareaZeroVal();
						current_screen = CaliOverBareaZeroValScreen;//零点校准完成后显示的B防区零点值显示屏
					}
				break;
			case AdatumCalibrationScreen://A防区基准校准显示屏
				if(calibrating_flag == 0xff)//校准完成
					{
						DisplayAareaDatumVal();
						current_screen = CaliOverAareaDatumValScreen;//基准校准完成后显示的A防区基准值显示屏
					}
				break;
			case BdatumCalibrationScreen://B防区基准校准显示屏
				if(calibrating_flag == 0xff)//校准完成
					{
						DisplayBareaDatumVal();
						current_screen = CaliOverBareaDatumValScreen;//基准校准完成后显示的B防区基准值显示屏
					}
				break;
			case NotAtValidRangeScreen://未在有效范围显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayDatumCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = DatumCalibrationScreen;//回到基准值校准显示屏
								break;
		
							case KEY_ENTER_PRESS:
								row_point = 1;//行指针指向第二行
								DisplayDatumCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = DatumCalibrationScreen;//回到基准值校准显示屏
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
			case CaliOverAareaZeroValScreen://零点校准完成后显示的A防区零点值显示屏
			case CaliOverBareaZeroValScreen://零点校准完成后显示的B防区零点值显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayZeroCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = ZeroCalibrationScreen;//回到零点值校准显示屏
								break;

							case KEY_ENTER_PRESS:
								row_point = 1;//行指针指向第二行
								DisplayZeroCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = ZeroCalibrationScreen;//回到零点值校准显示屏
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
			case CaliOverAareaDatumValScreen://基准校准完成后显示的A防区基准值显示屏
			case CaliOverBareaDatumValScreen://基准校准完成后显示的B防区基准值显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayDatumCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = DatumCalibrationScreen;//回到基准值校准显示屏
								break;

							case KEY_ENTER_PRESS:
								row_point = 1;//行指针指向第二行
								DisplayDatumCalibration();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = DatumCalibrationScreen;//回到基准值校准显示屏
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
			
			
			
			
			
			
			
			case SetRS485ParaScreen://设定RS485参数显示屏
				
				break;
			case InqEthParaScreen://查询网络参数显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInqSysPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqSysParaScreen;//回到查询系统参数显示屏
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://MAC地址
										row_point = 1;//行指针指向第二行
										DisplayInqMacAddr();
										current_screen = InqMacAddrScreen;//当前屏为查询Mac地址菜单
										break;
									case 2://IP地址
										row_point = 1;//行指针指向第二行
										GetIpAddr();
										DisplayInqIpAddr();
										current_screen = InqIpAddrScreen;//当前屏为查询IP地址菜单
										break;
									case 3://子网掩码
										row_point = 1;//行指针指向第二行
										GetSubNetMask();
										DisplayInqSubnetMask();
										current_screen = InqSubnetMaskScreen;//当前屏为查询子网掩码菜单
										break;
									case 4://网关
										row_point = 1;//行指针指向第二行
										GetGateWay();
										DisplayInqGateWay();
										current_screen = InqGateWayScreen;//当前屏为查询网关菜单
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
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
									}
								break;

							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 4)
									{
										row_point = 1;
									}
								DisplayInqEthPara();
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
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
			case InqRS485ParaScreen://查询RS485参数显示屏
				
				break;
			case InqDuanLuAlarmScreen://查询短路告警显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInqAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqAlarmRecordScreen;//回到查询告警记录显示屏
								break;
								
							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A区
										row_point = 1;//行指针指向第二行
										ReadLCDFirstPageLog(LOG_TYPE_AIVD);
										DisplayInqAAreaRuQinAlarmRecord();
										current_screen = InqAAreaDuanLuAlarmRecordScreen;//当前屏为查询A区短路告警记录菜单
										break;
									case 2://B区
										row_point = 1;//行指针指向第二行
										ReadLCDFirstPageLog(LOG_TYPE_BIVD);
										DisplayInqBAreaRuQinAlarmRecord();
										current_screen = InqBAreaDuanLuAlarmRecordScreen;//当前屏为查询B区短路告警记录菜单
										break;
									default:
										break;
								}
								break;
								
							case KEY_UP_PRESS:
								row_point --;
								if(FIELD_FLAG_S0D1)
									{//单防区不显示B防区
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
									{//单防区不显示B防区
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
			case InqDuanXianAlarmScreen://查询断路告警显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInqAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqAlarmRecordScreen;//回到查询告警记录显示屏
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A区
										row_point = 1;//行指针指向第二行
										ReadLCDFirstPageLog(LOG_TYPE_AOPN);
										DisplayInqAAreaDuanXianAlarmRecord();
										current_screen = InqAAreaDuanXianAlarmRecordScreen;//当前屏为查询A区断路告警记录菜单
										break;
									case 2://B区
										row_point = 1;//行指针指向第二行
										ReadLCDFirstPageLog(LOG_TYPE_BOPN);
										DisplayInqBAreaDuanXianAlarmRecord();
										current_screen = InqBAreaDuanXianAlarmRecordScreen;//当前屏为查询B区断路告警记录菜单
										break;
									default:
										break;
								}
								break;

							case KEY_UP_PRESS:
								row_point --;
								if(FIELD_FLAG_S0D1)
									{//单防区不显示B防区
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
									{//单防区不显示B防区
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
			case InqFangChaiAlarmRecordScreen://查询防拆告警显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInqAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqAlarmRecordScreen;//回到查询告警记录显示屏
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
			case SetMacAddrScreen://设定MAC地址显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetEthPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetEthParaScreen;//回到设定以太网参数显示屏
								break;

							case KEY_ENTER_PRESS:
								//保存MAC地址到EEPROM
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
			case SetIpAddrScreen://设定IP地址显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetEthPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetEthParaScreen;//回到设定以太网参数显示屏
								break;

							case KEY_ENTER_PRESS:
								//保存IP地址到EEPROM
								SetIpAddr();
								lwip_reset_netif_ipaddr();
								row_fx_pos = 1;//第一个位置反白显示
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
										row_fx_pos = 15;//12个可编辑数字 +3个不可编辑的“.”
									}
								DisplaySetIpAddr();
								break;

							case KEY_RIGHT_PRESS:
								if((row_fx_pos == 3) || (row_fx_pos == 7) || (row_fx_pos == 11))
									{
										row_fx_pos ++;
									}
								row_fx_pos ++;
								if(row_fx_pos > 15)//12个可编辑数字 +3个不可编辑的“.”
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
			case SetSubnetMaskScreen://设定子网掩码显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetEthPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetEthParaScreen;//回到设定以太网参数显示屏
								break;

							case KEY_ENTER_PRESS:
								//保存IP地址到EEPROM
								SetSubNetMask();
								lwip_reset_netif_ipaddr();
								row_fx_pos = 1;//第一个位置反白显示
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
										row_fx_pos = 15;//12个可编辑数字 +3个不可编辑的“.”
									}
								DisplaySetSubnetMask();
								break;

							case KEY_RIGHT_PRESS:
								if((row_fx_pos == 3) || (row_fx_pos == 7) || (row_fx_pos == 11))
									{
										row_fx_pos ++;
									}
								row_fx_pos ++;
								if(row_fx_pos > 15)//12个可编辑数字 +3个不可编辑的“.”
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
			case SetGateWayScreen://设定网关显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetEthPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetEthParaScreen;//回到设定以太网参数显示屏
								break;

							case KEY_ENTER_PRESS:
								//保存IP地址到EEPROM
								SetGateWay();
								lwip_reset_netif_ipaddr();
								row_fx_pos = 1;//第一个位置反白显示
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
										row_fx_pos = 15;//12个可编辑数字 +3个不可编辑的“.”
									}
								DisplaySetGateWay();
								break;

							case KEY_RIGHT_PRESS:
								if((row_fx_pos == 3) || (row_fx_pos == 7) || (row_fx_pos == 11))
									{
										row_fx_pos ++;
									}
								row_fx_pos ++;
								if(row_fx_pos > 15)//12个可编辑数字 +3个不可编辑的“.”
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
			case InqMacAddrScreen://查询MAC地址显示屏
			case InqIpAddrScreen://查询IP地址显示屏
			case InqSubnetMaskScreen://查询子网掩码显示屏
			case InqGateWayScreen://查询网关显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
							case KEY_ENTER_PRESS:
								row_point = 1;//行指针指向第二行
								DisplayInqEthPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqEthParaScreen;//回到查询以太网参数显示屏
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
			case InqAFangQuParaScreen://查询A防区参数显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DiaplayInqFangQuPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqFangQuParaScreen;//回到查询防区参数显示屏
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
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
									}
								break;
										
							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 4)
									{
										row_point = 1;
									}
								DiaplayInqAFangQuPara();
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
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
			case InqBFangQuParaScreen://查询B防区参数显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DiaplayInqFangQuPara();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqFangQuParaScreen;//回到查询防区参数显示屏
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
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
									}
								break;
										
							case KEY_DOWN_PRESS:
								row_point ++;
								if(row_point > 4)
									{
										row_point = 1;
									}
								DiaplayInqBFangQuPara();
								if(row_point > 3)//如果选项多于3行
									{
										Display_8x16(LeftScreen,Page_6,4,arrows,DISPLAY_POSITIVE);//行箭头始终保持在显示屏的最后一行
									}
								else
									{
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//否则按顺序显示
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
			case InqAAreaDuanLuAlarmRecordScreen://查询A防区短路告警显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInqRuQinAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqDuanLuAlarmScreen;//回到查询短路告警记录显示屏
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
			case InqBAreaDuanLuAlarmRecordScreen://查询B防区短路告警显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInqRuQinAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqDuanLuAlarmScreen;//回到查询短路告警记录显示屏
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
			case InqAAreaDuanXianAlarmRecordScreen://查询A防区断路告警显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInqDuanXianAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqDuanXianAlarmScreen;//回到查询断路告警记录显示屏
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
			case InqBAreaDuanXianAlarmRecordScreen://查询B防区断路告警显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplayInqDuanXianAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqDuanXianAlarmScreen;//回到查询断路告警记录显示屏
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
			case ClearDuanLuAlarmRecordScreen://清除短路告警记录显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetClearRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetClearRecordScreen;//回到清除记录显示屏
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A区
										DisplayZhengZaiQingChu();
										//清除EEPROM
										ClearLog(LOG_TYPE_AIVD);
										
										row_point = 1;//行指针指向第二行
										DisplayClearRuQinAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ClearDuanLuAlarmRecordScreen;//清除短路告警记录显示屏
										break;
									case 2://B区
										DisplayZhengZaiQingChu();
										//清除EEPROM
										ClearLog(LOG_TYPE_BIVD);
										
										row_point = 1;//行指针指向第二行
										DisplayClearRuQinAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ClearDuanLuAlarmRecordScreen;//清除短路告警记录显示屏
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
			case ClearDuanXianAlarmRecordScreen://清除断路告警记录显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetClearRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetClearRecordScreen;//回到清除记录显示屏
								break;

							case KEY_ENTER_PRESS:
								switch(row_point)
								{
									case 1://A区
										DisplayZhengZaiQingChu();
										//清除EEPROM
										ClearLog(LOG_TYPE_AOPN);

										row_point = 1;//行指针指向第二行
										DisplayClearDuanXianAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ClearDuanXianAlarmRecordScreen;//清除断路告警记录显示屏
										break;
									case 2://B区
										DisplayZhengZaiQingChu();
										//清除EEPROM
										ClearLog(LOG_TYPE_BOPN);

										row_point = 1;//行指针指向第二行
										DisplayClearDuanXianAlarmRecord();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = ClearDuanXianAlarmRecordScreen;//清除断路告警记录显示屏
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
			case AlarmTypeListScreen://告警信息列表显示屏
				if(0 == (IS_SET_BIT(config_code, 2)? ALARM_STATE: ALARM_STATE_DELAY)){  //如果无告警，回到显示屏
					alarm_type_cnt = 0;
					old_Alarm_Type_Sum = 0;
					cur_Alarm_Type_Sum = 0;
					alarm_key_adj = 0;
					current_screen = RunScreen;//回到运行显示屏
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
							case KEY_ESC_PRESS://按退出键
								current_screen = RunScreen;//回到运行显示屏
								DisplayRunMenu();
								break;

							case KEY_ENTER_PRESS:
								row_point = 1;//行指针指向第二行
								DisplayInqAlarmRecord();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = InqAlarmRecordScreen;//回到查询告警记录显示屏
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

			case SetExtDeviceTypeScreen://设定兼容外部设备显示屏
				if(key != KEY_NO_PRESS)
					{
						if(flag_PageState)//判断页面状态，正常显示时：
							{
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//显示行指针

								switch(key)
								{
									case KEY_ESC_PRESS://按退出键
										row_point = 1;//行指针指向第二行
										DisplaySetMenu();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
										current_screen = SetScreen;//回到设定菜单
										break;

									case KEY_ENTER_PRESS:
										switch(row_point)
										{
											case 1://A防区编辑
												//A防区兼容外部设备类型反白显示，可编辑
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												break;
											case 2://B防区编辑
												//B防区兼容外部设备类型反白显示，可编辑
												flag_PageState = ModifyState;//页面状态由正常显示模式转换为编辑显示模式
												break;

											default:
												break;
										}
										DiaplaySetExtDeviceTypeScreen();
										break;

									case KEY_UP_PRESS:
										row_point --;
										if(FIELD_FLAG_S0D1)
											{//单防区不显示B防区
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
											{//单防区不显示B防区
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
						else//判断页面状态，编辑显示时：
							{
								//不显示行指针
								switch(key)
								{
									case KEY_ESC_PRESS://按退出键
										row_point = 1;//行指针指向第二行
										flag_PageState = NormalState;
										DiaplaySetExtDeviceTypeScreen();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//显示行指针
										current_screen = SetExtDeviceTypeScreen;//当前屏为设定外接设备类型菜单
										break;

									case KEY_ENTER_PRESS:
										setSWValue((SW_Type[1] << 8) | SW_Type[0]);
										flag_PageState = NormalState;//页面状态由编辑显示模式转换为正常显示模式
										DiaplaySetExtDeviceTypeScreen();
										Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);//显示行指针
										break;

									case KEY_UP_PRESS:
										//判断哪个防区
										switch(row_point)
										{
											case 1://编辑第一行（A防区兼容外部设备类型）
												SW_Type[0]++;
												if(SW_Type[0] > 8)
													{
														SW_Type[0] = 0;
													}
												break;
											case 2://编辑第二行（B防区兼容外部设备类型）
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
										//判断是哪个防区
										switch(row_point)
										{
											case 1://编辑第一行（A防区兼容外部设备类型）
												SW_Type[0]--;
												if(SW_Type[0] < 0)
													{
														SW_Type[0] = 8;
													}
												break;
											case 2://编辑第二行（B防区兼容外部设备类型）
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
			
			case SetFullScaleScreen://设置满量程拉力值显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetScreen;//回到设定显示屏
								break;

							case KEY_ENTER_PRESS:
								setTensionMaxRange((u16)(atoi(tension_max_range_temp) * 10));
								row_fx_pos = 1;//第一个位置反白显示
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
			
			case SetBaseAutoCalibrateTimeScreen://设置基准值自动校准时间显示屏
				if(key != KEY_NO_PRESS)
					{
						switch(key)
						{
							case KEY_ESC_PRESS://按退出键
								row_point = 1;//行指针指向第二行
								DisplaySetMenu();
								Display_8x16(LeftScreen,row_point * 2,4,arrows,DISPLAY_POSITIVE);
								current_screen = SetScreen;//回到设定显示屏
								break;

							case KEY_ENTER_PRESS:
								setBaseAutoCalibrateTime((u16)atoi(base_auto_calibrate_time_tmp));
								row_fx_pos = 1;//第一个位置反白显示
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


//监视、告警扫描任务
void watch_task(void *pdata)
{	
	pdata=pdata;
	while(1){

		FANG_CHAI_Check();
		
		//更新温度
		if (IS_SET_VAR_BIT(timerHandlerFlag,7))
		{
			CLEAR_VAR_BIT(timerHandlerFlag,7);
			LM75_ReadTempStr(crtLM75TempStr);
		}

		//更新日期时间
		if (IS_SET_VAR_BIT(timerHandlerFlag,6))
		{
			CLEAR_VAR_BIT(timerHandlerFlag,6);
			PCF8563_Get_Str(rtcTempStr);
		}
		//检测网线热插拔
		if (IS_SET_VAR_BIT(timerHandlerFlag,5))
		{
			CLEAR_VAR_BIT(timerHandlerFlag, 5);
			Eth_Link_ITHandler(); //支持启动未插网线和网线热插拔。
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
//		//写日志测试
//		if(flag_fcgjsn == ON){
//			WriteLog(LOG_TYPE_AOPN, ALARM);
//			WriteLog(LOG_TYPE_AIVD, ALARM);
//			WriteLog(LOG_TYPE_BOPN, ALARM);
//			WriteLog(LOG_TYPE_BIVD, ALARM);
//			WriteLog(LOG_TYPE_FC, ALARM);
//		}

// 		//测试互斥量
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


//按键扫描任务
void key_task(void *pdata)
{	
	u8 key;
	pdata=pdata;
	while(1)
	{
		key=KEY_Scan(0);
		if(key) OSMboxPost(msg_key, (void*)key);//发送消息
 		delay_ms(10);

// 		//测试互斥量
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

//硬件错误处理
void HardFault_Handler(void)
{
	u32 i;
	u8 t=0;
	u32 temp;
	temp=SCB->CFSR;					//fault状态寄存器(@0XE000ED28)包括:MMSR,BFSR,UFSR
 	printf("CFSR:%8X\r\n",temp);	//显示错误值
	temp=SCB->HFSR;					//硬件fault状态寄存器
 	printf("HFSR:%8X\r\n",temp);	//显示错误值
 	temp=SCB->DFSR;					//调试fault状态寄存器
 	printf("DFSR:%8X\r\n",temp);	//显示错误值
   	temp=SCB->AFSR;					//辅助fault状态寄存器
 	printf("AFSR:%8X\r\n",temp);	//显示错误值
 
 	while(t<5)
	{
		t++;
		LED_RUN=!LED_RUN;
		for(i=0;i<0X1FFFFF;i++);
 	}
 	LED_RUN = 0;

}
