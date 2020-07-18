#ifndef __PCF8563_H
#define __PCF8563_H
#include "iicrw.h"

#define PCF8563_SLAVE_ADDR 0xa2

typedef struct{
    u8 tm_year;
    u8 tm_mon;
    u8 tm_mday;
    u8 tm_hour;
    u8 tm_min;
    u8 tm_sec;
} PCF8536_RTC_TD;

extern    char rtcTempStr[19];//2018-03-31 16:27:00
extern	  char rtcTempStr_temp[19];//实时时钟字符串暂存，用于编辑显示，不会影响读过来的实时时钟字符串

void PCF8563_Init(void);
int splitStrToInt(int *to_int, const char *from_str, const char *delim, int base);
void PCF8563_Set_Str(char *rtc_Set_Str);
void PCF8563_Set(PCF8536_RTC_TD *rtc_TD_Set);
void PCF8563_Get(PCF8536_RTC_TD *rtc_TD_Get);
void PCF8563_Get_Str(char *rtc_TD_Str);

#endif
















