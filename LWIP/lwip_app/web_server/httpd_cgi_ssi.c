#include "lwip/debug.h"
#include "sys.h"
#include "version.h"
#include "httpd.h"
#include "lwip/tcp.h"
#include "fs.h"
#include "lwip_comm.h"
#include "24cxx.h"
#include "led.h"
#include "alarm.h"
#include "adc.h"
#include "log.h"
#include "adc.h"
#include "lm75.h"
#include "pcf8563.h"
#include "malloc.h"

//#include "beep.h"
//#include "adc.h"
//#include "rtc.h"
//#include "lcd.h"


#include <string.h>
#include <stdlib.h>


#define NUM_CONFIG_CGI_URIS	(sizeof(ppcURLs) / sizeof(tCGI))
#define NUM_CONFIG_SSI_TAGS	(sizeof(ppcTAGs) / sizeof(char *))
	
//extern short Get_Temprate(void);  //声明Get_Temperate()函数


PCF8536_RTC_TD rtc_TD_Set;
 
//控制LED的CGI handler
const char* Network_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* Datetime_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* Control_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* Default_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* Log_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* Switch_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* Shutdown_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

//const char* BEEP_CGI_Handler(int iIndex,int iNumParams,char *pcParam[],char *pcValue[]);

static const char *ppcTAGs[]=  //SSI的Tag
{	//备选：o,q,u,y
	"m",	//0.MAC
	"i",	//1.IP
	"s",	//2.SubMask
	"g",	//3.Gateway
	"e",	//4.temprature 温度
	"d",	//5.DataTime
	"a",	//6.AlarmState
	"p",	//7.满量程、报警阈值上限/下限偏移量， 格式："tension_max_range#alarm_threshold_up_dif#alarm_threshold_down_dif" 如： "20#30"
	"c",	//8.ControlState
	"h",	//9.hardware version
	"v",	//10.software version
	"y",	//11.LogType
	"l",	//12.Log
	"w",	//13.web page flag
	"r",	//14.IR1,IR2 类型值
	"f",	//15.防区参数
	"n",	//16.web log page num
	"t",	//17.web log page total
	"x",	//18.型号：bit1(4,6线标志：IS_W4	0， IS_W6	1)； bi0( FIELD_FLAG_S0D1，   双防区：0，XB-100WS, 单防区：1，XB-100WD)
	"z",	//19.remote_shutdown  1:关机， 0：开机
	"b",	//20.configcode bit7-1：reserves, bit0:(1:显示联系我们，0：不显示联系我们)
	"j",	//21. a,b防区拉力值（*0.1kg） 如："55+55+56+34+44+55#55+55+56+34+44+55"，55=5.5kg.
	"k",	//22.  bit15-12：B防区校准使能，bit11-8：A防区校准使能， bit7-4: 0：校准标记calibrating_flag：校准zero, 1:校准基准值,  bit3-0: 0:A防区，1:B防区
};


static const tCGI ppcURLs[]= //cgi程序
{
	{"/network.cgi",Network_CGI_Handler},
	{"/datatime.cgi",Datetime_CGI_Handler},
	{"/control.cgi",Control_CGI_Handler},
	{"/default.cgi",Default_CGI_Handler},
	{"/log.cgi",Log_Handler},
	{"/switch.cgi",Switch_Handler},
	{"/shutdown.cgi",Shutdown_Handler}
};


//当web客户端请求浏览器的时候,使用此函数被CGI handler调用
static int FindCGIParameter(const char *pcToFind,char *pcParam[],int iNumParams)
{
	int iLoop;
	for(iLoop = 0;iLoop < iNumParams;iLoop ++ )
	{
		if(strcmp(pcToFind,pcParam[iLoop]) == 0)
		{
			return (iLoop); //返回iLOOP
		}
	}
	return (-1);
}

//SSIHandler中需要用到的处理内部温度传感器的函数
void Temperate_Handler(char *pcInsert)
{
	LM75_ReadTempStr(pcInsert);
}

//SSIHandler中需要用到的处理RTC时间的函数
void RTCTime_Handler(char *pcInsert)
{
	PCF8563_Get_Str(pcInsert);
}

//SSI的Handler句柄
static u16_t SSIHandler(int iIndex,char *pcInsert,int iInsertLen)
{
	switch(iIndex)
	{
		case 0: 
			sprintf((char*)pcInsert,"%02X:%02X:%02X:%02X:%02X:%02X",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);//MAC
			break;
		case 1:
			sprintf((char*)pcInsert,"%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//IP
			break;
		case 2:
			sprintf((char*)pcInsert,"%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);//submask
			break;
		case 3:
			sprintf((char*)pcInsert,"%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);//gateway
			break;
		case 4:
			Temperate_Handler(pcInsert);
			break;
		case 5:
			//RTCTime_Handler(pcInsert);
			sprintf((char*)pcInsert, "%s", rtcTempStr);
			pcInsert[sizeof(rtcTempStr)] = '\0';	//防止字节对齐问题。
			break;
		case 6:  //AlarmState
			sprintf((char*)pcInsert, "%d", ALARM_STATE);
			break;
		case 7:  //报警阈值上限/下限偏移量/基准值自动校准时间， 格式："tension_max_range#alarm_threshold_up_dif#alarm_threshold_down_dif#base_auto_calibrate_time" 如： "20#30#30#30"
			sprintf((char*)pcInsert, "%d#%d#%d#%d", tension_max_range, alarm_threshold_up_dif, alarm_threshold_down_dif, base_auto_calibrate_time);
			break;
		case 8:  ///** bit7-bit3 无   bit2: Fangchai   bit1:BuFangB    bit0:BuFangA   1:ON  0:OFF **/
			sprintf((char*)pcInsert, "%d", ((flag_fcgjsn & 0x01)<<2) | ((BU_FANG_B_FLAG & 0x01)<<1) | (BU_FANG_A_FLAG & 0x01));
			break;
		case 9:  //hardware version
			sprintf((char*)pcInsert, "%s", hardware_ver);
			pcInsert[sizeof(hardware_ver)] = '\0';	//防止字节对齐问题。
			break;
		case 10:  //software version
			sprintf((char*)pcInsert, "%s", software_ver);
			pcInsert[sizeof(software_ver)] = '\0';	//防止字节对齐问题。
			break;
		case 11:  //Log
			sprintf((char*)pcInsert, "%d", logType);
			break;
		case 12:  //Log
			if(*crtWebPageLogData == 0){
				ReadWebFirstPageLog(logType);
			}
			strcpy((char*)pcInsert, (char*)crtWebPageLogData);
			break;

		case 13:  //webLogPageFlag bit1: FirstPage, bit0:LastPage
			sprintf((char*)pcInsert, "%d", webLogPageFlag);
			break;

		case 14:  //开关量2,1
			sprintf((char*) pcInsert, "%d", ((SW_Type[1] << 8) | SW_Type[0]));
			break;

		case 15:  //防区A参数#防区B参数
			getWebFieldParas((char*) pcInsert);
			break;
		case 16:  //"n",  //16.web log page num
			sprintf((char*)pcInsert, "%d", crtWebLogPageNum);
			break;
		case 17:  //"t",  //17.web log page total
			sprintf((char*)pcInsert, "%d", (u8)getWebLogPageTotal(logType));
			break;
		case 18:  //18.型号：bit1(4,6线标志：IS_W4	0， IS_W6	1)； bi0( FIELD_FLAG_S0D1，   双防区：0，XC-100WS, 单防区：1，XC-100WD)
			sprintf((char*) pcInsert, "%d", ((W4_W6 << 1) & 0x02) | (FIELD_FLAG_S0D1 & 0x01));
			break;
		case 19:  //19.remote_shutdown
			sprintf((char*)pcInsert, "%d", remote_shutdown);
			break;
		case 20:  //20.configcode bit7-1：reserves, bit0:(1:显示联系我们，0：不显示联系我们)
			sprintf((char*)pcInsert, "%d", config_code);
			break;
		case 21:  //21. a,b防区拉力值（*0.1kg） crt@zero@base, 例如："55+55+56+34+44+55#55+55+56+34+44+55@0+9+3+4+5+3#0+0+2+3+4+3@55+55+56+34+44+55#55+55+56+34+44+55";
			getWebAdcValStr((char*) pcInsert);
			break;
		case 22:  //22. 校准标记calibrating_flag  ： 校准标记：bit7-4: 0：校准zero, 1:校准基准值,  bit3-0: 0:A防区，1:B防区
			sprintf((char*) pcInsert, "%d", (calibrate_en[1] << 12) | (calibrate_en[0] << 8) | calibrating_flag);
			break;

	}
	return strlen(pcInsert);
}


/**
 *  convert ip to ip_int, such as :　"192.168.0.22"--> {192,168,0,22}
 * @return -1: error, 0: ok.
 * */
int ipStrToInt(unsigned char *ip_int, const char *ip)
{
	int section = 0;  //每一节的十进制值
	int dot = 0;       //几个点分隔符
	while (*ip)
	{
		if (*ip == '.')
		{
			dot++;
			if (dot > 3 || *(ip + 1) == '.') //防止连续的两个点的情况
			{
				return -1;
			}
			if (section >= 0 && section <= 255)
			{
				ip_int[dot-1] = section;
				section = 0;
			}
			else
			{
				return -1;
			}
		}
		else if (*ip >= '0' && *ip <= '9')
		{
			section = section * 10 + *ip - '0';
		}
		else
		{
			return -1;
		}
		ip++;
	}

	if (section >= 0 && section <= 255)
	{
		ip_int[3] = section;
		return 0;
	}
	else
	{
		return -1;
	}
}


/**
 *  convert mac to mac_int, such as :　"02:00:00:31:00:28"--> {02,00,00,31,00,28}
 * @return -1: error, 0: ok.
 * */
int macStrToInt(unsigned char *mac_int, const char *mac)
{
	int section = 0;  //每一节的十进制值
	int dot = 0;       //几个点分隔符
	while (*mac)
	{
		if (*mac == ':')
		{
			dot++;
			if (dot > 5 || *(mac + 1) == ':') //防止连续的两个点的情况
			{
				return -1;
			}
			if (section >= 0 && section <= 255)
			{
				mac_int[dot-1] = section;
				section = 0;
			}
			else
			{
				return -1;
			}
		}
		else if (*mac >= '0' && *mac <= '9')
		{
			section = section * 16 + *mac - '0';
		}
		else if ( *mac >= 'a' && *mac <= 'f'){
			section = section * 16 + *mac - 'a' + 0x0a;
		}		
		else if (*mac >= 'A' && *mac <= 'F'){
			section = section * 16 + *mac - 'A' + 0x0a;
		}
		else
		{
			return -1;
		}
		mac++;
	}

	if (section >= 0 && section <= 255)
	{
		mac_int[5] = section;
		return 0;
	}
	else
	{
		return -1;
	}
}
//CGI netpara句柄
const char* Network_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	u8 i=0;  //注意根据自己的GET的参数的多少来选择i值范围
	u8 modify_flag = 0;
	
	iIndex = FindCGIParameter("LAN",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++) //检查CGI参数: example GET /leds.cgi?led=2&led=4 */
		{
			if (strcmp(pcParam[i] , "MAC")==0){
//				if (strcmp((char *)lwipdev.mac, pcValue[i])!=0){ //判断表达式有误，需修正
					macStrToInt(lwipdev.mac, pcValue[i]);
					modify_flag = 1;
//				}
			}
			if (strcmp(pcParam[i] , "IP")==0){
//				if (strcmp((char *)lwipdev.ip, pcValue[i])!=0){
					ipStrToInt(lwipdev.ip, pcValue[i]);
					modify_flag = 1;
//				}
			}
			if (strcmp(pcParam[i] , "SUBMASK")==0){
//				if (strcmp((char *)lwipdev.netmask, pcValue[i])==0){
					ipStrToInt(lwipdev.netmask, pcValue[i]);
					modify_flag = 1;
//				}
			}
			if (strcmp(pcParam[i] , "GATEWEY")==0){
//				if (strcmp((char *)lwipdev.gateway, pcValue[i])==0){
					ipStrToInt(lwipdev.gateway, pcValue[i]);
					modify_flag = 1;
//				}
			}
		}
	 }
	if(modify_flag){
		lwip_reset_netif_ipaddr();
//		Sys_Soft_Reset();
//		SystemReset();
		//sprintf(rtn, "http://%s/parameters.shtml", lwipdev.ip);
		//return "http://192.168.1.60/index.stml";
	}
	
	return "/parameters.shtml";
	
}


//CGI Datetime句柄
const char* Datetime_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	u8 i=0;  //注意根据自己的GET的参数的多少来选择i值范围
//	char ss[] = "2018-03-25 03:00:00";
	char *rtcStr;
	int td_tmp[3];
	iIndex = FindCGIParameter("DATE",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++) //检查CGI参数: example GET /leds.cgi?led=2&led=4 */
		{
			if (strcmp(pcParam[i] , "DATE")==0){
				//"2018-03-25 03:00:00"
				rtcStr = pcValue[i];
	  			if( rtcStr != NULL ){  //date
	  				splitStrToInt(td_tmp, rtcStr, "-", 10);
	  				rtc_TD_Set.tm_year = (u8)((td_tmp[0] - 2000) & 0xff);
					rtc_TD_Set.tm_mon = (u8)(td_tmp[1] & 0xff);
					rtc_TD_Set.tm_mday = (u8)(td_tmp[2] & 0xff);
	  			}  
			}
			if (strcmp(pcParam[i] , "TIME")==0){
				rtcStr = pcValue[i];
    			if( rtcStr != NULL ){//time
    				splitStrToInt(td_tmp, rtcStr, ":", 10);
    				rtc_TD_Set.tm_hour = (u8)(td_tmp[0] & 0xff);
					rtc_TD_Set.tm_min = (u8)(td_tmp[1] & 0xff);
					rtc_TD_Set.tm_sec = (u8)(td_tmp[2] & 0xff);
    			}  
			}
		}
//		PCF8563_Init();
	    PCF8563_Set(&rtc_TD_Set);
	 }
	 
	
	return "/parameters.shtml";
	
}

const char* Switch_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	u8 i=0;  //注意根据自己的GET的参数的多少来选择i值范围
	iIndex = FindCGIParameter("SWITCH",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if ( strcmp(pcParam[i] , "SWITCH") == 0){
				setSWValue((u16)atoi(pcValue[i]));
			}
		}
	}
	return "/parameters.shtml";
}

const char* Shutdown_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	u8 i=0;  //注意根据自己的GET的参数的多少来选择i值范围
	u8 shutdown = 0;
	iIndex = FindCGIParameter("shutdown",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if ( strcmp(pcParam[i] , "shutdown") == 0){
				shutdown = (u8)atoi(pcValue[i]);
				remoteShutdown(shutdown);
			}
		}
	}
	if(shutdown == 1){
		return "/parameters.shtml";
	}else{
		return "/remotepower.shtml";
	}
}

/**************************************************************
* bit7-bit3 无   bit2: Fangchai   bit1:BuFangB    bit0:BuFangA   1:ON  0:OFF 
* 
***/
const char* Control_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	u8 i=0;  //注意根据自己的GET的参数的多少来选择i值范围
	u8 ctrTypIndex = 0, ctrValIndex = 0;
	iIndex = FindCGIParameter("field_num_0",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if (strcmp(pcParam[i] , "field_num_0")==0){
				field_num[0] = (u8)atoi(pcValue[i]);
			}else if (strcmp(pcParam[i] , "alm_dly_0")==0){
				alarm_delay[0] = (u16)atoi(pcValue[i]);
			}else if (strcmp(pcParam[i] , "sensitivity_0")==0){
				alarm_sensitivity[0] = (u16)atoi(pcValue[i]);
//			}else if (strcmp(pcParam[i] , "ten_val_range_0_0")==0){
//				ten_val_range[0][0] = (u16)atoi(pcValue[i]);
//			}else if (strcmp(pcParam[i] , "ten_val_range_0_1")==0){
//				ten_val_range[0][1] = (u16)atoi(pcValue[i]);
			}else if (strcmp(pcParam[i] , "field_num_1")==0){
				field_num[1] = (u8)atoi(pcValue[i]);
			}else if (strcmp(pcParam[i] , "alm_dly_1")==0){
				alarm_delay[1] = (u16)atoi(pcValue[i]);
			}else if (strcmp(pcParam[i] , "sensitivity_1")==0){
				alarm_sensitivity[1] = (u16)atoi(pcValue[i]);
			}
//			else if (strcmp(pcParam[i] , "ten_val_range_1_0")==0){
//				ten_val_range[1][0] = (u16)atoi(pcValue[i]);
//			}else if (strcmp(pcParam[i] , "ten_val_range_1_1")==0){
//				ten_val_range[1][1] = (u16)atoi(pcValue[i]);
//			}
		}
		//统一保存
		WriteFieldParaToEEProm();
		return "/control.shtml";
	}

	iIndex = FindCGIParameter("alarm_reset",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if (strcmp(pcParam[i] , "alarm_reset")==0 && strcmp(pcValue[i] , "0")==0){
				Remove_Alarm_RL_A();
			}
			else if (strcmp(pcParam[i] , "alarm_reset")==0 && strcmp(pcValue[i] , "1")==0){
				Remove_Alarm_RL_B();
			}
		}
		return "/control.shtml";
	}

	iIndex = FindCGIParameter("CTR_TYPE",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if (strcmp(pcParam[i] , "CTR_TYPE")==0){
				ctrTypIndex = i;
			}else if (strcmp(pcParam[i] , "CTR_VALUE")==0){
				ctrValIndex = i;
			}
		}
		
		if(strcmp(pcValue[ctrTypIndex] , "fc")==0){
			(atoi(pcValue[ctrValIndex]) == 1)? FangChai_Start():FangChai_Stop();
		}
		else if(strcmp(pcValue[ctrTypIndex] , "b_bf")==0){
			(atoi(pcValue[ctrValIndex]) == 1)? Push_B_Start():Push_B_Stop();
		}
		else if(strcmp(pcValue[ctrTypIndex] , "a_bf")==0){
			(atoi(pcValue[ctrValIndex]) == 1)? Push_A_Start():Push_A_Stop();
		}
		return "/control.shtml";
	}
	
	iIndex = FindCGIParameter("THRESHOLD_UP",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if (strcmp(pcParam[i] , "THRESHOLD_UP")==0){
				setAlmThrdUpDif((u16)atoi(pcValue[i]));
			}
			if (strcmp(pcParam[i] , "THRESHOLD_DOWN")==0){
				setAlmThrdDwDif((u16)atoi(pcValue[i]));
			}
		}
		return "/control.shtml";
	}

	iIndex = FindCGIParameter("TENSION_MAX_RANGE",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if (strcmp(pcParam[i] , "TENSION_MAX_RANGE")==0){
				setTensionMaxRange((u16)atoi(pcValue[i]));
			}
			if (strcmp(pcParam[i] , "BASE_AUTO_CALIBRATE_TIME")==0){
				setBaseAutoCalibrateTime((u16)atoi(pcValue[i]));
			}
		}
		return "/control.shtml";
	}

	iIndex = FindCGIParameter("zero_reset",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if (strcmp(pcParam[i] , "zero_reset")==0){
				updateZeroVal((u8)atoi(pcValue[i]));
			}
		}
		return "/state.shtml";
	}

	iIndex = FindCGIParameter("base_reset",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if (strcmp(pcParam[i] , "base_reset")==0){
				updateBaseVal((u8)atoi(pcValue[i]));
			}
		}
		return "/state.shtml";
	}

	return "/control.shtml";
}

const char* Default_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	u8 i=0;  //注意根据自己的GET的参数的多少来选择i值范围
	iIndex = FindCGIParameter("DEFAULT",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if ( strcmp(pcParam[i] , "DEFAULT") == 0 && strcmp(pcValue[i] , "1") == 0 ){
				systemDefault();
			}
		}
	}
	
	iIndex = FindCGIParameter("configcode",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if ( strcmp(pcParam[i] , "configcode") == 0 ){
				setConfigCode(atoi(pcValue[i]));
				return "/adminsetting.shtml";
			}
		}
	}
	return "/control.shtml";
}

const char* Log_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	u8 i=0;  //注意根据自己的GET的参数的多少来选择i值范围
	iIndex = FindCGIParameter("LOGTYPE",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if ( strcmp(pcParam[i] , "LOGTYPE") == 0){
				logType = atoi(pcValue[i]);
				mymemset(crtWebPageLogData, 0, sizeof(crtWebPageLogData));
				lastWebPageReadDir = WEB_PAGE_READ_DOWN;
				
				break;
			}
		}
		return "/log.shtml";
	}
	
	
	iIndex = FindCGIParameter("CLEAR",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if ( strcmp(pcParam[i] , "CLEAR") == 0 && strcmp(pcValue[i] , "1") == 0 ){
				ClearLog(logType);
			}
		}
		return "/log.shtml";
	}
	
	iIndex = FindCGIParameter("GETLOG",pcParam,iNumParams);
	if (iIndex != -1)
	{
		for (i=0; i<iNumParams; i++)
		{
			if(strcmp(pcParam[i] , "GETLOG") == 0){
				if(strcmp(pcValue[i] , "0") == 0 ){
					ReadWebPreviousPageLog(logType);
					
				}else if(strcmp(pcValue[i] , "1") == 0 ){
					ReadWebNextPageLog(logType);
				}
			}
		}
		return "/log.shtml";
		
	}
	return "/log.shtml";
}


//SSI句柄初始化
void httpd_ssi_init(void)
{  
	//配置network的SSI句柄
	http_set_ssi_handler(SSIHandler,ppcTAGs,NUM_CONFIG_SSI_TAGS);
}

//CGI句柄初始化
void httpd_cgi_init(void)
{ 
  //配置CGI句柄LEDs control CGI) */
  http_set_cgi_handlers(ppcURLs, NUM_CONFIG_CGI_URIS);
}


