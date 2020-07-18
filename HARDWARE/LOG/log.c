#include "string.h"
#include "log.h"
#include "24cxx.h"
#include "stdio.h"
#include "delay.h"
#include "malloc.h"
#include "pcf8563.h"


u32 nextWriteAddrVal[5];
u32 crtReadAddrVal[5];

//当前web读取的log页数
u8 crtWebLogPageNum = 0;

u8 lastWebPageLogNum;
u8 lastLCDPageLogNum;

//webLogPageFlag bit1: FirstPage, bit0:LastPage, 0:否，1：是
u8 webLogPageFlag = LOG_HEAD;

//webLogPageFlag bit1: FirstPage, bit0:LastPage, 0:否，1：是
u8 LCDLogPageFlag = LOG_HEAD;

u8 logType = LOG_TYPE_FC;

u8 lastWebPageReadDir = WEB_PAGE_READ_DOWN;
u8 lastLCDPageReadDir = LCD_PAGE_READ_DOWN;



u8 crtWebPageLogData[EACH_WEB_PAGE_LOG_NUM * EACH_LOG_SIZE] = {0};
u8 crtWebPageLogDataTmp[EACH_WEB_PAGE_LOG_NUM * EACH_LOG_SIZE] = {0};

u8 crtLCDPageLogData[EACH_LCD_PAGE_LOG_NUM][EACH_LOG_SIZE];

void Log_Init(void){
	int logType = 0;
	
	for(logType = 0; logType < 5; logType++){
		nextWriteAddrVal[logType] = AT24CXX_ReadLenByte(LOG_NEXT_W_ADDR(logType), 2);
		if(nextWriteAddrVal[logType] == 0xffff){
			nextWriteAddrVal[logType] = EACH_TYPE_LOG_BASED_ADDR(logType);
			AT24CXX_WriteLenByte(LOG_NEXT_W_ADDR(logType), nextWriteAddrVal[logType], 2);
		}
	}

	lastWebPageReadDir = WEB_PAGE_READ_DOWN;
	lastLCDPageReadDir = LCD_PAGE_READ_DOWN;
}

/**
* 写告警日志，每条日志只写告警发生的时间，前19个字节写时间， 第20个字节写alarmFlag标记： 1:ALARM, 0:NOALARM
**/
void WriteLog(u8 logType, u8 alarmFlag){
	u8 logDataStr[EACH_LOG_SIZE];
	mymemcpy(logDataStr, rtcTempStr, sizeof(rtcTempStr));
	logDataStr[EACH_LOG_SIZE - 1] = alarmFlag;
		
	AT24CXX_Write(nextWriteAddrVal[logType], logDataStr, EACH_LOG_SIZE); 
	nextWriteAddrVal[logType] += EACH_LOG_SIZE;
	if(nextWriteAddrVal[logType] > EACH_TYPE_LOG_MAX_ADDR(logType)){
		nextWriteAddrVal[logType] = EACH_TYPE_LOG_BASED_ADDR(logType);
	}
	AT24CXX_WriteLenByte(LOG_NEXT_W_ADDR(logType), nextWriteAddrVal[logType], 2);

	//新增日志后要将日志重新加载
	mymemset(crtWebPageLogData, 0, sizeof(crtWebPageLogData));
	lastWebPageReadDir = WEB_PAGE_READ_DOWN;

	mymemset(crtLCDPageLogData, 0, sizeof(crtLCDPageLogData));
	lastLCDPageReadDir = LCD_PAGE_READ_DOWN;
}

/**
* 读取第一条logType的日志
**/
void ReadFirstLog(u8 logType, u8 *logDataStr){

	mymemset(logDataStr, 0, EACH_LOG_SIZE);

	crtReadAddrVal[logType] = nextWriteAddrVal[logType];
	ReadNextLog(logType, logDataStr);
}

//void ReadNextLog(u8 logType, u8 *logDataStr){
//	mymemset(logDataStr, 0, EACH_LOG_SIZE);
//	if(crtReadAddrVal[logType] == EACH_TYPE_LOG_BASED_ADDR(logType)){//如果是基地址
//		if(isLogFull(logType)){		//曾被写满
//			crtReadAddrVal[logType] = EACH_TYPE_LOG_MAX_ADDR(logType);	//绕到最高地址
//		}else{			//未曾被写满
////			if(isLogPageOverOne(logType)){//超过一页
////				webLogPageFlag = LOG_END;//标记到末页
////			}else{
////				webLogPageFlag |= LOG_END;
////			}
//			updateLogPageFlag(logType, LOG_END);
//			logDataStr[0] = '\0';
//			return;
//		}
//	}else{   //不是基地址
//		crtReadAddrVal[logType] -= EACH_LOG_SIZE;//回退一条日志
//		if(crtReadAddrVal[logType] == nextWriteAddrVal[logType]){//如果是当前写日志地址
//			crtReadAddrVal[logType] += EACH_LOG_SIZE;//取消回退
////			if(isLogPageOverOne(logType)){
////				webLogPageFlag = LOG_END;  //标记为末页
////			}else{
////				webLogPageFlag |= LOG_END;
////			}
//			updateLogPageFlag(logType, LOG_END);
//			logDataStr[0] = '\0';
//			return;
//		}
//	}
//
//	AT24CXX_Read(crtReadAddrVal[logType], logDataStr, EACH_LOG_SIZE);	//读当前日志。
//	logDataStr[EACH_LOG_SIZE-1] = '\0';
////	if(logDataStr[0] == 0xFF){//没有日志
////		crtReadAddrVal[logType] += EACH_LOG_SIZE;//取消回退
////		logDataStr[0] = '\0';
////	}
//
//	if(crtReadAddrVal[logType] == EACH_TYPE_LOG_BASED_ADDR(logType)){//如果是基地址
//		if(!isLogFull(logType)){		//曾被写满
////			if(isLogPageOverOne(logType)){//超过一页
////				webLogPageFlag = LOG_END;//标记到末页
////			}else{
////				webLogPageFlag |= LOG_END;
////			}
//			updateLogPageFlag(logType, LOG_END);
//		}
//	}
//}

//解决写满日志的情况下，少读最后一条日志的Bug
void ReadNextLog(u8 logType, u8 *logDataStr){
	mymemset(logDataStr, 0, EACH_LOG_SIZE);
	if(crtReadAddrVal[logType] == EACH_TYPE_LOG_BASED_ADDR(logType)){//如果是基地址
		if(isLogFull(logType)){		//曾被写满
			crtReadAddrVal[logType] = EACH_TYPE_LOG_MAX_ADDR(logType);	//绕到最高地址
		}else{			//未曾被写满
			updateLogPageFlag(logType, LOG_END);//标记结束
			logDataStr[0] = '\0';
			return;
		}
	}else{   //不是基地址
		crtReadAddrVal[logType] -= EACH_LOG_SIZE;//回退一条日志
	}

	AT24CXX_Read(crtReadAddrVal[logType], logDataStr, EACH_LOG_SIZE);	//读当前日志。
	logDataStr[EACH_LOG_SIZE-1] = '\0';
	if(isLogFull(logType)){		//曾被写满
		if(crtReadAddrVal[logType] == nextWriteAddrVal[logType]){//如果读到已写日志的最后一条
			updateLogPageFlag(logType, LOG_END);//标记结束
//			logDataStr[0] = '\0';
			return;
		}
	}else{
		if(crtReadAddrVal[logType] == EACH_TYPE_LOG_BASED_ADDR(logType)){//如果是基地址
			updateLogPageFlag(logType, LOG_END);
			return;
		}
	}
}

void ReadPreviousLog(u8 logType, u8 *logDataStr){
	mymemset(logDataStr, 0, EACH_LOG_SIZE);
	if(crtReadAddrVal[logType] == EACH_TYPE_LOG_MAX_ADDR(logType)){//如果是最大地址
		crtReadAddrVal[logType] = EACH_TYPE_LOG_BASED_ADDR(logType);  //回到基地址
	}else{
		if(crtReadAddrVal[logType] == (nextWriteAddrVal[logType] - EACH_LOG_SIZE)){//如果是最新的一条日志
			crtReadAddrVal[logType] = nextWriteAddrVal[logType];
			updateLogPageFlag(logType, LOG_HEAD);
			logDataStr[0] = '\0';
			return;
		}else{
			crtReadAddrVal[logType] += EACH_LOG_SIZE;
		}
	}
	AT24CXX_Read(crtReadAddrVal[logType], logDataStr, EACH_LOG_SIZE);	//读当前日志。
	logDataStr[EACH_LOG_SIZE-1] = '\0';
	
	if(crtReadAddrVal[logType] == (nextWriteAddrVal[logType] - EACH_LOG_SIZE)){//如果是最新的一条日志
		updateLogPageFlag(logType, LOG_HEAD);
	}
}

//获取Web日志页总数
u8 getWebLogPageTotal(u8 logType){
	if(isLogFull(logType)){
		return WEB_PAGE_MAX_NUM;
	}else{
		if(nextWriteAddrVal[logType] == EACH_TYPE_LOG_BASED_ADDR(logType)){
			return 1;
		}else{
			return (u8)((nextWriteAddrVal[logType] - EACH_LOG_SIZE - EACH_TYPE_LOG_BASED_ADDR(logType)) / EACH_LOG_SIZE / EACH_WEB_PAGE_LOG_NUM + 1);
		}
	}
}

void updateLogPageFlag(u8 logType, u8 logFlag){
	if (isWebLogPageOverOne(logType)) {
		webLogPageFlag = logFlag;
	} else {
		webLogPageFlag |= logFlag;
	}

	if (isLCDLogPageOverOne(logType)) {
		LCDLogPageFlag = logFlag;
	} else {
		LCDLogPageFlag |= logFlag;
	}
}


//判断是否读到当前写地址，当日志写满的情况下，读一轮结束标志， 1：结束，0：未结束。
u8 isReadLoopEnd(u8 logType){
	return crtReadAddrVal[logType] == nextWriteAddrVal[logType]? 1:0;
}

//判断是否读日志是否写满的 1：已写满，0：未写满。
u8 isLogFull(u8 logType){
	return (AT24CXX_ReadOneByte(EACH_TYPE_LOG_MAX_ADDR(logType)) != 0xFF)? 1:0;
}

//判断是否Web日志页数大于1
u8 isWebLogPageOverOne(u8 logType){
	if(isLogFull(logType)){
		return 1;
	}else{
		return (EACH_TYPE_LOG_BASED_ADDR(logType) + (EACH_WEB_PAGE_LOG_NUM * EACH_LOG_SIZE)) < nextWriteAddrVal[logType]? 1:0;
	}
}


u8 isLCDLogPageOverOne(u8 logType){
	if(isLogFull(logType)){
		return 1;
	}else{
		return (EACH_TYPE_LOG_BASED_ADDR(logType) + (EACH_LCD_PAGE_LOG_NUM * EACH_LOG_SIZE)) < nextWriteAddrVal[logType]? 1:0;
	}
}


void ReadWebFirstPageLog(u8 logType){
	u8 logTmp[EACH_LOG_SIZE];
	u8 i = 0;

	crtWebLogPageNum = 1;
	webLogPageFlag = LOG_HEAD;
	lastWebPageLogNum = 0;

	mymemset(crtWebPageLogData, 0, sizeof(crtWebPageLogData));
	lastWebPageReadDir = WEB_PAGE_READ_DOWN;

	ReadFirstLog(logType, logTmp);
	if(*logTmp == '\0'){
		return;
	}
	strcat(crtWebPageLogData, logTmp);
	strcat(crtWebPageLogData, "#");
	delay_us(2);
	for(i = 1; i < EACH_WEB_PAGE_LOG_NUM; i++){
		ReadNextLog(logType, logTmp);
		if(*logTmp == '\0'){
			break;
		}
//		if(isReadLoopEnd(logType)){
//			break;
//		}
		strcat(crtWebPageLogData, logTmp);
		strcat(crtWebPageLogData, "#");
		delay_us(2);
	}
	lastWebPageLogNum = i;
}

void ReadWebNextPageLog(u8 logType){
	u8 logTmp[EACH_LOG_SIZE];
	u8 i = 0;

	crtWebLogPageNum++;
	webLogPageFlag = LOG_MIDDLE;
	if(lastWebPageReadDir == WEB_PAGE_READ_UP){
		crtReadAddrVal[logType] -= (lastWebPageLogNum - 1) * EACH_LOG_SIZE;  //前移当前页日志数
		lastWebPageReadDir = WEB_PAGE_READ_DOWN;
	}
	
	mymemset(crtWebPageLogData, 0, sizeof(crtWebPageLogData));
	for(i = 0; i < EACH_WEB_PAGE_LOG_NUM; i++){
		ReadNextLog(logType, logTmp);
		if(*logTmp == '\0'){
			break;
		}
//		if(isReadLoopEnd(logType)){
//			break;
//		}
		strcat(crtWebPageLogData, logTmp);
		strcat(crtWebPageLogData, "#");
		delay_us(2);
	}
	lastWebPageLogNum = i;
		
}

void ReadWebPreviousPageLog(u8 logType){
	u8 logTmp[EACH_LOG_SIZE];
	u8 i = 0;

	crtWebLogPageNum--;
	webLogPageFlag = LOG_MIDDLE;
	if(lastWebPageReadDir == WEB_PAGE_READ_DOWN){
		crtReadAddrVal[logType] += (lastWebPageLogNum - 1) * EACH_LOG_SIZE;  //前移当前页日志数 //-1为了让 执行ReadPreviousLog前后退一条
		lastWebPageReadDir = WEB_PAGE_READ_UP;
	}
	
	mymemset(crtWebPageLogData, 0, sizeof(crtWebPageLogData));
	
	for(i = 0; i < EACH_WEB_PAGE_LOG_NUM; i++){
		ReadPreviousLog(logType, logTmp);
		if(*logTmp == '\0'){
			break;
		}
//		if(isReadLoopEnd(logType)){
//			break;
//		}
		
		mymemset(crtWebPageLogDataTmp, 0, sizeof(crtWebPageLogDataTmp));
		mymemcpy(crtWebPageLogDataTmp, crtWebPageLogData, sizeof(crtWebPageLogData));
		
		mymemset(crtWebPageLogData, 0, sizeof(crtWebPageLogData));
		strcat(crtWebPageLogData, logTmp);
		strcat(crtWebPageLogData, "#");
		strcat(crtWebPageLogData, crtWebPageLogDataTmp);
		
		delay_us(2);
	}
	lastWebPageLogNum = i;
	
//	ReadWebNextPageLog(logType);
}

void ReadLCDFirstPageLog(u8 logType){
	u8 logTmp[EACH_LOG_SIZE];
	u8 i = 0;

	LCDLogPageFlag = LOG_HEAD;
	mymemset(crtLCDPageLogData, 0, sizeof(crtLCDPageLogData));
	lastLCDPageReadDir = LCD_PAGE_READ_DOWN;

	ReadFirstLog(logType, logTmp);
	if(*logTmp == '\0'){
		return;
	}
	mymemcpy(crtLCDPageLogData[0], logTmp, sizeof(logTmp));

	delay_us(2);
	for(i = 1; i < EACH_LCD_PAGE_LOG_NUM; i++){
		ReadNextLog(logType, logTmp);
		if(*logTmp == '\0'){
			break;
		}
		mymemcpy(crtLCDPageLogData[i], logTmp, sizeof(logTmp));
		delay_us(2);
	}
	lastLCDPageLogNum = i;
}

void ReadLCDNextPageLog(u8 logType){
	u8 logTmp[EACH_LOG_SIZE];
	u8 i = 0;

	LCDLogPageFlag = LOG_MIDDLE;
	if(lastLCDPageReadDir == LCD_PAGE_READ_UP){
		crtReadAddrVal[logType] -= (lastLCDPageLogNum - 1) * EACH_LOG_SIZE;  //前移当前页日志数
		lastLCDPageReadDir = LCD_PAGE_READ_DOWN;
	}

	mymemset(crtLCDPageLogData, 0, sizeof(crtLCDPageLogData));
	for(i = 0; i < EACH_LCD_PAGE_LOG_NUM; i++){
		ReadNextLog(logType, logTmp);
		if(*logTmp == '\0'){
			break;
		}
		mymemcpy(crtLCDPageLogData[i], logTmp, sizeof(logTmp));
		delay_us(2);
	}
	lastLCDPageLogNum = i;

}

void ReadLCDPreviousPageLog(u8 logType){
	u8 logTmp[EACH_LOG_SIZE];
	u8 i = 0;

	LCDLogPageFlag = LOG_MIDDLE;
	if(lastLCDPageReadDir == LCD_PAGE_READ_DOWN){
		crtReadAddrVal[logType] += (lastLCDPageLogNum - 1) * EACH_LOG_SIZE;  //前移当前页日志数 //-1为了让 执行ReadPreviousLog前后退一条
		lastLCDPageReadDir = LCD_PAGE_READ_UP;
	}

	mymemset(crtLCDPageLogData, 0, sizeof(crtLCDPageLogData));

	for(i = 0; i < EACH_LCD_PAGE_LOG_NUM; i++){
		ReadPreviousLog(logType, logTmp);
		if(*logTmp == '\0'){
			break;
		}
		mymemcpy(crtLCDPageLogData[EACH_LCD_PAGE_LOG_NUM -1 - i], logTmp, sizeof(logTmp));
		delay_us(2);
	}
	lastLCDPageLogNum = i;
}

void ClearAllLog(void){
	FormatLogEEprom();
}

void FormatLogEEprom(void){
	FastClearLog(LOG_TYPE_BOPN);
	FastClearLog(LOG_TYPE_BIVD);
	FastClearLog(LOG_TYPE_AOPN);
	FastClearLog(LOG_TYPE_AIVD);
	FastClearLog(LOG_TYPE_FC);
}

void ClearLog(u8 logType){
	FastClearLog(logType);
}


void FastClearLog(u8 logType){

	EraseEEProm(LOG_NEXT_W_ADDR(logType), 2);
	if(AT24CXX_ReadOneByte(nextWriteAddrVal[logType]) != 0xFF){//如果写满又回写了，则全部清除
		FastEraseLogEEProm(EACH_TYPE_LOG_BASED_ADDR(logType), EACH_TYPE_LOG_BLOCK_SIZE);
	}else{//如果未写满，则，只清除已写日志
		FastEraseLogEEProm(EACH_TYPE_LOG_BASED_ADDR(logType), (nextWriteAddrVal[logType] - EACH_TYPE_LOG_BASED_ADDR(logType)));
	}
	mymemset(crtWebPageLogData, 0, sizeof(crtWebPageLogData));
	Log_Init();
}

//只erase每条日志第一个字节。
void FastEraseLogEEProm(u16 eraseAddr, u16 eraseLen){
	while(eraseLen)
	{
		AT24CXX_WriteOneByte(eraseAddr, 0xFF);
		eraseAddr+=EACH_LOG_SIZE;
		eraseLen-=EACH_LOG_SIZE;
	}
}

