#include "pcf8563.h"
#include "delay.h"
#include "stdio.h"

#include <string.h>
#include <stdlib.h>

char rtcTempStr[19] = {0};//2018-03-31 16:27:00
char rtcTempStr_temp[19] = {0};//实时时钟字符串暂存，用于编辑显示，不会影响读过来的实时时钟字符串
/***************************************************************************
功    能：初始化时钟芯片PCF8563
输入参数：结构变量m_time
返    回：若配置成功，返回OK；否则返回ERROR
***************************************************************************/
void PCF8563_Init(void)
{
	u8 slaveAddr = PCF8563_SLAVE_ADDR;
	u8 writeAddr = 0x00;
	u8 init_time[16] = {  //2018-01-01 00:00:00
			0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x06, 0x01,
			0x18, 0x80, 0x80, 0x80, 0x80, 0x81, 0x00, 0x00
			};
   
	if (IICWrite(slaveAddr, writeAddr, init_time, sizeof(init_time)) == ERROR)
//    {
//#ifdef DEBUG_PRINT
//        printf("iic init pcf8563 error!\n");
//#endif
//    }
	delay_ms(10);
}

u8 Hex2BCD(u8 hexChar){
	if(hexChar > 99){
		hexChar = 99;
	}
	return ((hexChar/10)<<4) + (hexChar-(hexChar/10)*10);
}

/***************************************************************************
功    能：设置时钟芯片PCF8563
输入参数：结构变量m_time
返    回：若配置成功，返回OK；否则返回ERROR

例：
	PCF8536_RTC_TD rtc_TD_Get;
	rtc_TD_Get.tm_year = 2018 - 2005;
	rtc_TD_Get.tm_mon = 3;
	rtc_TD_Get.tm_mday = 18;
	rtc_TD_Get.tm_hour = 8;
	rtc_TD_Get.tm_min = 10;
	rtc_TD_Get.tm_sec = 30;
    
    PCF8563_Set(&rtc_TD_Get);
    
***************************************************************************/
void PCF8563_Set(PCF8536_RTC_TD *rtc_TD_Set)
{
	u8 slaveAddr = PCF8563_SLAVE_ADDR;
	u8 writeAddr = 0x00;
	u8 init_time[16] = {
			0x00, 0x00, 0x00, 0x07, 0x15, 0x24, 0x06, 0x04,
			0x04, 0x80, 0x80, 0x80, 0x80, 0x81, 0x00, 0x00
			};
   
//   printf("iic init pcf8563\n");
	
//	init_time[2] = ((rtc_TD_Set->tm_sec/10)<<4)+(rtc_TD_Set->tm_sec-(rtc_TD_Set->tm_sec/10)*10);
//	init_time[3] = ((rtc_TD_Set->tm_min/10)<<4)+(rtc_TD_Set->tm_min-(rtc_TD_Set->tm_min/10)*10);
//	init_time[4] = ((rtc_TD_Set->tm_hour/10)<<4)+(rtc_TD_Set->tm_hour-(rtc_TD_Set->tm_hour/10)*10);
//	init_time[5] = ((rtc_TD_Set->tm_mday/10)<<4)+(rtc_TD_Set->tm_mday-(rtc_TD_Set->tm_mday/10)*10);
//	init_time[7] = ((rtc_TD_Set->tm_mon/10)<<4)+(rtc_TD_Set->tm_mon-(rtc_TD_Set->tm_mon/10)*10);
//	init_time[8] = ((rtc_TD_Set->tm_year/10)<<4)+(rtc_TD_Set->tm_year-(rtc_TD_Set->tm_year/10)*10);
			
			
	init_time[2] = Hex2BCD(rtc_TD_Set->tm_sec);
	init_time[3] = Hex2BCD(rtc_TD_Set->tm_min);
	init_time[4] = Hex2BCD(rtc_TD_Set->tm_hour);
	init_time[5] = Hex2BCD(rtc_TD_Set->tm_mday);
	init_time[7] = Hex2BCD(rtc_TD_Set->tm_mon);
	init_time[8] = Hex2BCD(rtc_TD_Set->tm_year);
			

	if ((rtc_TD_Set->tm_mon == 1) || (rtc_TD_Set->tm_mon == 2)){
		rtc_TD_Set->tm_mon += 12;
		rtc_TD_Set->tm_year--;
	}
	init_time[6] = (rtc_TD_Set->tm_mday + 2*rtc_TD_Set->tm_mon + 3*(rtc_TD_Set->tm_mon+1)/5
	                + rtc_TD_Set->tm_year + rtc_TD_Set->tm_year/4 - rtc_TD_Set->tm_year/100 + rtc_TD_Set->tm_year/400) % 7;
	if(init_time[6] == 6)
		init_time[6] = 0;
	else
		init_time[6]++;

	if (IICWrite(slaveAddr, writeAddr, init_time, sizeof(init_time)) == ERROR)
//    {
//#ifdef DEBUG_PRINT
//        printf("iic init pcf8563 error!\n");
//#endif
//    }
	delay_ms(10);
}


/**
 *  以delim为分割符对from_str进行分割，并将分割的字符串转换为int型数组to_int,
 * @return num: int数组个数
 * */
int splitStrToInt(int *to_int, const char *from_str, const char *delim, int base)
{
	int num = 0;
	char *p = NULL;
	int p_int = 0;
//	int len = strlen(from_str);
  	p_int = strtol(strtok((unsigned char *)from_str, delim), NULL, base);
	to_int[num++] = (int)(p_int);
	while(p=strtok(NULL, delim))
	{
		p_int = strtol(p, NULL, base);
		to_int[num++] = (int)(p_int);	
	}
	return num;
}


/***************************************************************************
功    能：设置时钟芯片PCF8563
输入参数：rtc_Str Format: "2018-03-25 03:00:00"
返    回：无
****************************************************************************/

void PCF8563_Set_Str(char *rtc_set_str)
{
	int i = 0;
	char rtcStr[11];
	int td_tmp[3];
//	char *delim = "#";
	PCF8536_RTC_TD rtc_TD_Set;
	
//	for(i = 0; i < 10; i++){
//		rtcStr[i] = *(rtc_set_str + i);
//	}
	strncpy(rtcStr, rtc_set_str, 10);
	rtcStr[10]='\0';
	
	if( rtcStr != NULL ){  //date
		splitStrToInt(td_tmp, rtcStr, "-", 10);
		rtc_TD_Set.tm_year = (u8)((td_tmp[0] - 2000) & 0xff);
		rtc_TD_Set.tm_mon = (u8)(td_tmp[1] & 0xff);
		rtc_TD_Set.tm_mday = (u8)(td_tmp[2] & 0xff);
	}
	
	strncpy(rtcStr, (rtc_set_str + 11), 8);
	
//	for(i = 0; i < 8; i++){
//		rtcStr[i] = *(rtc_set_str + 11 + i);
//	}
	rtcStr[8]='\0';
	
	if( rtcStr != NULL ){//time
		splitStrToInt(td_tmp, rtcStr, ":", 10);
		rtc_TD_Set.tm_hour = (u8)(td_tmp[0] & 0xff);
		rtc_TD_Set.tm_min = (u8)(td_tmp[1] & 0xff);
		rtc_TD_Set.tm_sec = (u8)(td_tmp[2] & 0xff);
	}

	PCF8563_Set(&rtc_TD_Set);
	return;
}


/***************************************************************************
功    能：获取时钟芯片PCF8563
输入参数：无
返    回：返回时钟值到结构变量rtc_TD_Get中
***************************************************************************/
void PCF8563_Get(PCF8536_RTC_TD *rtc_TD_Get)
{
	u8 read_time[11];
	u8 slaveAddr = PCF8563_SLAVE_ADDR;
	u8 readAddr = 0x00;

	if(IICRead (slaveAddr, readAddr, read_time, sizeof(read_time)) == ERROR)
    {
//#ifdef DEBUG_PRINT
//		printf("iic read pcf8563 error!\n");
//#endif
        return;
    }
    //以下都是BCD码
	read_time[2] = read_time[2] & 0x7f;
	read_time[3] = read_time[3] & 0x7f;
	read_time[4] = read_time[4] & 0x3f;
	read_time[5] = read_time[5] & 0x3f;
	read_time[7] = read_time[7] & 0x1f;
	read_time[8] = read_time[8] & 0xff;

	//when.tm_year = read_time[6]+ 100;
	rtc_TD_Get->tm_year=	((read_time[8])>>4)*10 + (read_time[8]&0x0f);
	rtc_TD_Get->tm_mon = 	((read_time[7])>>4)*10 + (read_time[7]&0x0f);
	rtc_TD_Get->tm_mday = ((read_time[5])>>4)*10 + (read_time[5]&0x0f);
	rtc_TD_Get->tm_hour = ((read_time[4])>>4)*10 + (read_time[4]&0x0f);
	rtc_TD_Get->tm_min = 	((read_time[3])>>4)*10 + (read_time[3]&0x0f);
	rtc_TD_Get->tm_sec = 	((read_time[2])>>4)*10 + (read_time[2]&0x0f);


	#ifdef DEBUG_PRINT
//	printf("current time is: 20%02x-%02x-%02x %02x:%02x:%02x\n",read_time[8],read_time[7],read_time[5],read_time[4],read_time[3],read_time[2]);
	#endif
}

/***************************************************************************
功    能：获取时钟芯片PCF8563 时间字符串, Format: "yyyy-mm-dd HH-mm-ss"
输入参数：无
返    回：
***************************************************************************/
void PCF8563_Get_Str(char *rtc_TD_Str)
{
	u8 read_time[11];
	u8 slaveAddr = PCF8563_SLAVE_ADDR;
	u8 readAddr = 0x00;

	if(IICRead (slaveAddr, readAddr, read_time, sizeof(read_time)) == ERROR)
    {
//#ifdef DEBUG_PRINT
//		printf("iic read pcf8563 error!\n");
//#endif
        return;
    }
    //以下都是BCD码
	read_time[2] = read_time[2] & 0x7f;
	read_time[3] = read_time[3] & 0x7f;
	read_time[4] = read_time[4] & 0x3f;
	read_time[5] = read_time[5] & 0x3f;
	read_time[7] = read_time[7] & 0x1f;
	read_time[8] = read_time[8] & 0xff;

	sprintf(rtc_TD_Str, "20%02x-%02x-%02x %02x:%02x:%02x\0", read_time[8],read_time[7],read_time[5],read_time[4],read_time[3],read_time[2]);
}
