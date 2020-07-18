#include "string.h"
#include "log.h"
#include "24cxx.h"
#include "stdio.h"
#include "delay.h"
#include "malloc.h"
#include "pcf8563.h"


u32 nextWriteAddrVal[5];
u32 crtReadAddrVal[5];

//��ǰweb��ȡ��logҳ��
u8 crtWebLogPageNum = 0;

u8 lastWebPageLogNum;
u8 lastLCDPageLogNum;

//webLogPageFlag bit1: FirstPage, bit0:LastPage, 0:��1����
u8 webLogPageFlag = LOG_HEAD;

//webLogPageFlag bit1: FirstPage, bit0:LastPage, 0:��1����
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
* д�澯��־��ÿ����־ֻд�澯������ʱ�䣬ǰ19���ֽ�дʱ�䣬 ��20���ֽ�дalarmFlag��ǣ� 1:ALARM, 0:NOALARM
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

	//������־��Ҫ����־���¼���
	mymemset(crtWebPageLogData, 0, sizeof(crtWebPageLogData));
	lastWebPageReadDir = WEB_PAGE_READ_DOWN;

	mymemset(crtLCDPageLogData, 0, sizeof(crtLCDPageLogData));
	lastLCDPageReadDir = LCD_PAGE_READ_DOWN;
}

/**
* ��ȡ��һ��logType����־
**/
void ReadFirstLog(u8 logType, u8 *logDataStr){

	mymemset(logDataStr, 0, EACH_LOG_SIZE);

	crtReadAddrVal[logType] = nextWriteAddrVal[logType];
	ReadNextLog(logType, logDataStr);
}

//void ReadNextLog(u8 logType, u8 *logDataStr){
//	mymemset(logDataStr, 0, EACH_LOG_SIZE);
//	if(crtReadAddrVal[logType] == EACH_TYPE_LOG_BASED_ADDR(logType)){//����ǻ���ַ
//		if(isLogFull(logType)){		//����д��
//			crtReadAddrVal[logType] = EACH_TYPE_LOG_MAX_ADDR(logType);	//�Ƶ���ߵ�ַ
//		}else{			//δ����д��
////			if(isLogPageOverOne(logType)){//����һҳ
////				webLogPageFlag = LOG_END;//��ǵ�ĩҳ
////			}else{
////				webLogPageFlag |= LOG_END;
////			}
//			updateLogPageFlag(logType, LOG_END);
//			logDataStr[0] = '\0';
//			return;
//		}
//	}else{   //���ǻ���ַ
//		crtReadAddrVal[logType] -= EACH_LOG_SIZE;//����һ����־
//		if(crtReadAddrVal[logType] == nextWriteAddrVal[logType]){//����ǵ�ǰд��־��ַ
//			crtReadAddrVal[logType] += EACH_LOG_SIZE;//ȡ������
////			if(isLogPageOverOne(logType)){
////				webLogPageFlag = LOG_END;  //���Ϊĩҳ
////			}else{
////				webLogPageFlag |= LOG_END;
////			}
//			updateLogPageFlag(logType, LOG_END);
//			logDataStr[0] = '\0';
//			return;
//		}
//	}
//
//	AT24CXX_Read(crtReadAddrVal[logType], logDataStr, EACH_LOG_SIZE);	//����ǰ��־��
//	logDataStr[EACH_LOG_SIZE-1] = '\0';
////	if(logDataStr[0] == 0xFF){//û����־
////		crtReadAddrVal[logType] += EACH_LOG_SIZE;//ȡ������
////		logDataStr[0] = '\0';
////	}
//
//	if(crtReadAddrVal[logType] == EACH_TYPE_LOG_BASED_ADDR(logType)){//����ǻ���ַ
//		if(!isLogFull(logType)){		//����д��
////			if(isLogPageOverOne(logType)){//����һҳ
////				webLogPageFlag = LOG_END;//��ǵ�ĩҳ
////			}else{
////				webLogPageFlag |= LOG_END;
////			}
//			updateLogPageFlag(logType, LOG_END);
//		}
//	}
//}

//���д����־������£��ٶ����һ����־��Bug
void ReadNextLog(u8 logType, u8 *logDataStr){
	mymemset(logDataStr, 0, EACH_LOG_SIZE);
	if(crtReadAddrVal[logType] == EACH_TYPE_LOG_BASED_ADDR(logType)){//����ǻ���ַ
		if(isLogFull(logType)){		//����д��
			crtReadAddrVal[logType] = EACH_TYPE_LOG_MAX_ADDR(logType);	//�Ƶ���ߵ�ַ
		}else{			//δ����д��
			updateLogPageFlag(logType, LOG_END);//��ǽ���
			logDataStr[0] = '\0';
			return;
		}
	}else{   //���ǻ���ַ
		crtReadAddrVal[logType] -= EACH_LOG_SIZE;//����һ����־
	}

	AT24CXX_Read(crtReadAddrVal[logType], logDataStr, EACH_LOG_SIZE);	//����ǰ��־��
	logDataStr[EACH_LOG_SIZE-1] = '\0';
	if(isLogFull(logType)){		//����д��
		if(crtReadAddrVal[logType] == nextWriteAddrVal[logType]){//���������д��־�����һ��
			updateLogPageFlag(logType, LOG_END);//��ǽ���
//			logDataStr[0] = '\0';
			return;
		}
	}else{
		if(crtReadAddrVal[logType] == EACH_TYPE_LOG_BASED_ADDR(logType)){//����ǻ���ַ
			updateLogPageFlag(logType, LOG_END);
			return;
		}
	}
}

void ReadPreviousLog(u8 logType, u8 *logDataStr){
	mymemset(logDataStr, 0, EACH_LOG_SIZE);
	if(crtReadAddrVal[logType] == EACH_TYPE_LOG_MAX_ADDR(logType)){//���������ַ
		crtReadAddrVal[logType] = EACH_TYPE_LOG_BASED_ADDR(logType);  //�ص�����ַ
	}else{
		if(crtReadAddrVal[logType] == (nextWriteAddrVal[logType] - EACH_LOG_SIZE)){//��������µ�һ����־
			crtReadAddrVal[logType] = nextWriteAddrVal[logType];
			updateLogPageFlag(logType, LOG_HEAD);
			logDataStr[0] = '\0';
			return;
		}else{
			crtReadAddrVal[logType] += EACH_LOG_SIZE;
		}
	}
	AT24CXX_Read(crtReadAddrVal[logType], logDataStr, EACH_LOG_SIZE);	//����ǰ��־��
	logDataStr[EACH_LOG_SIZE-1] = '\0';
	
	if(crtReadAddrVal[logType] == (nextWriteAddrVal[logType] - EACH_LOG_SIZE)){//��������µ�һ����־
		updateLogPageFlag(logType, LOG_HEAD);
	}
}

//��ȡWeb��־ҳ����
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


//�ж��Ƿ������ǰд��ַ������־д��������£���һ�ֽ�����־�� 1��������0��δ������
u8 isReadLoopEnd(u8 logType){
	return crtReadAddrVal[logType] == nextWriteAddrVal[logType]? 1:0;
}

//�ж��Ƿ����־�Ƿ�д���� 1����д����0��δд����
u8 isLogFull(u8 logType){
	return (AT24CXX_ReadOneByte(EACH_TYPE_LOG_MAX_ADDR(logType)) != 0xFF)? 1:0;
}

//�ж��Ƿ�Web��־ҳ������1
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
		crtReadAddrVal[logType] -= (lastWebPageLogNum - 1) * EACH_LOG_SIZE;  //ǰ�Ƶ�ǰҳ��־��
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
		crtReadAddrVal[logType] += (lastWebPageLogNum - 1) * EACH_LOG_SIZE;  //ǰ�Ƶ�ǰҳ��־�� //-1Ϊ���� ִ��ReadPreviousLogǰ����һ��
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
		crtReadAddrVal[logType] -= (lastLCDPageLogNum - 1) * EACH_LOG_SIZE;  //ǰ�Ƶ�ǰҳ��־��
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
		crtReadAddrVal[logType] += (lastLCDPageLogNum - 1) * EACH_LOG_SIZE;  //ǰ�Ƶ�ǰҳ��־�� //-1Ϊ���� ִ��ReadPreviousLogǰ����һ��
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
	if(AT24CXX_ReadOneByte(nextWriteAddrVal[logType]) != 0xFF){//���д���ֻ�д�ˣ���ȫ�����
		FastEraseLogEEProm(EACH_TYPE_LOG_BASED_ADDR(logType), EACH_TYPE_LOG_BLOCK_SIZE);
	}else{//���δд������ֻ�����д��־
		FastEraseLogEEProm(EACH_TYPE_LOG_BASED_ADDR(logType), (nextWriteAddrVal[logType] - EACH_TYPE_LOG_BASED_ADDR(logType)));
	}
	mymemset(crtWebPageLogData, 0, sizeof(crtWebPageLogData));
	Log_Init();
}

//ֻeraseÿ����־��һ���ֽڡ�
void FastEraseLogEEProm(u16 eraseAddr, u16 eraseLen){
	while(eraseLen)
	{
		AT24CXX_WriteOneByte(eraseAddr, 0xFF);
		eraseAddr+=EACH_LOG_SIZE;
		eraseLen-=EACH_LOG_SIZE;
	}
}

