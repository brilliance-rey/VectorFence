#ifndef __LOG_H
#define __LOG_H
#include "sys.h" 

//Log Count Addr(0x0080 ~ 0x009F): 	0x0080-0x0081:BOpnLogCntAddr;	0x0082-0x0083:BClsLogCntAddr;
//	 								0x0084-0x0085:AOpnLogCntAddr; 	0x0086-0x0087:AClsLogCntAddr;
//								 	0x0088-0x0089:FCLogCntAddr; 

//每个日志块写满后，又回滚到日志块首地址，覆盖最旧的日志。

#define LOG_TYPE_BOPN	0
#define LOG_TYPE_BIVD	1
#define LOG_TYPE_AOPN	2
#define LOG_TYPE_AIVD	3
#define LOG_TYPE_FC		4

#define LOG_MIDDLE	0x00
#define LOG_END		0x01
#define LOG_HEAD	0x02

#define LOG_BLOCK_START_ADDR 		0x00E0

//#define BOpnLOG_NEXT_W_ADDR	(LOG_BLOCK_START_ADDR + LOG_TYPE_BOPN * 2)
//#define BClsLOG_NEXT_W_ADDR  	(LOG_BLOCK_START_ADDR + LOG_TYPE_BIVD * 2)
//#define AOpnLOG_NEXT_W_ADDR  	(LOG_BLOCK_START_ADDR + LOG_TYPE_AOPN * 2)
//#define AClsLOG_NEXT_W_ADDR  	(LOG_BLOCK_START_ADDR + LOG_TYPE_AIVD * 2)
//#define FCLOG_NEXT_W_ADDR		(LOG_BLOCK_START_ADDR + LOG_TYPE_FC * 2)

#define LOG_NEXT_W_ADDR(logType)	(LOG_BLOCK_START_ADDR + logType * 2)


/**
*LogBlockFullFlag:
* bit  07      06      05      04      03      02      01      00
*      --      --      --      FC    B_OPN   B_CLS   A_OPN   A_CLS
*  0:未写满， 1：写满回滚继续写
*/

#define EACH_LOG_SIZE	20
#define EACH_TYPE_LOG_TOTAL	300		// 300 * 20 * 5 = 30000字节 = 0x7530字节，EEprom总容量32*1024 = 32768 = 0x8000
#define EACH_TYPE_LOG_BLOCK_SIZE (EACH_TYPE_LOG_TOTAL * EACH_LOG_SIZE)  //300条

#define LOG_DATA_BASED_ADDR 	0x0100	//从0x0100写日志，0x7630日志结束。

//#define BOpnEACH_TYPE_LOG_BASED_ADDR  (LOG_DATA_BASED_ADDR +  EACH_TYPE_LOG_BLOCK_SIZE * LOG_TYPE_BOPN)	//0x0100
//#define BClsEACH_TYPE_LOG_BASED_ADDR  (LOG_DATA_BASED_ADDR +  EACH_TYPE_LOG_BLOCK_SIZE * LOG_TYPE_BIVD)	//0x1870
//#define AOpnEACH_TYPE_LOG_BASED_ADDR  (LOG_DATA_BASED_ADDR +  EACH_TYPE_LOG_BLOCK_SIZE * LOG_TYPE_AOPN)	//0x2FE0
//#define AClsEACH_TYPE_LOG_BASED_ADDR  (LOG_DATA_BASED_ADDR +  EACH_TYPE_LOG_BLOCK_SIZE * LOG_TYPE_AIVD)	//0x4750
//#define FCEACH_TYPE_LOG_BASED_ADDR	  (LOG_DATA_BASED_ADDR +  EACH_TYPE_LOG_BLOCK_SIZE * LOG_TYPE_FC)	//0x5EC0

#define EACH_TYPE_LOG_BASED_ADDR(logType)  (LOG_DATA_BASED_ADDR +  EACH_TYPE_LOG_BLOCK_SIZE * logType)//0x0100,0x1870,0x2FE0,0x4750,0x5EC0

//#define BOpnEACH_TYPE_LOG_MAX_ADDR  (BOpnEACH_TYPE_LOG_BASED_ADDR + EACH_TYPE_LOG_BLOCK_SIZE - EACH_LOG_SIZE)	//0x1870-0x14 = 0x185C
//#define BClsEACH_TYPE_LOG_MAX_ADDR  (BClsEACH_TYPE_LOG_BASED_ADDR + EACH_TYPE_LOG_BLOCK_SIZE - EACH_LOG_SIZE)	//0x2FE0-0x14 = 0x2FCC
//#define AOpnEACH_TYPE_LOG_MAX_ADDR  (AOpnEACH_TYPE_LOG_BASED_ADDR + EACH_TYPE_LOG_BLOCK_SIZE - EACH_LOG_SIZE)	//0x4750-0x14 = 0x473C
//#define AClsEACH_TYPE_LOG_MAX_ADDR  (AClsEACH_TYPE_LOG_BASED_ADDR + EACH_TYPE_LOG_BLOCK_SIZE - EACH_LOG_SIZE)	//0x5EC0-0x14 = 0x5EAC
//#define FCEACH_TYPE_LOG_MAX_ADDR	(FCEACH_TYPE_LOG_BASED_ADDR + EACH_TYPE_LOG_BLOCK_SIZE - EACH_LOG_SIZE)		//0x7630-0x14 = 0x761C

#define EACH_TYPE_LOG_MAX_ADDR(logType)  (EACH_TYPE_LOG_BASED_ADDR(logType) + EACH_TYPE_LOG_BLOCK_SIZE - EACH_LOG_SIZE)	//0x1870-0x14 = 0x185C,0x2FCC,0x473C,0x5EAC,0x761C

#define EACH_TYPE_LOG_END_ADDR (LOG_DATA_BASED_ADDR + EACH_TYPE_LOG_BLOCK_SIZE * 5 - 1)

#define LOG_DATA_BYTE_LEN (EACH_TYPE_LOG_END_ADDR - LOG_BLOCK_START_ADDR)

#define EACH_WEB_PAGE_LOG_NUM 10
#define EACH_WEB_PAGE_LOG_BYTE_LEN 	(EACH_WEB_PAGE_LOG_NUM * EACH_LOG_SIZE)
#define WEB_PAGE_MAX_NUM		(EACH_TYPE_LOG_TOTAL / EACH_WEB_PAGE_LOG_NUM)

#define EACH_LCD_PAGE_LOG_NUM 3
//#define EachLCDPageLogByteLen (EACH_LCD_PAGE_LOG_NUM * EACH_LOG_SIZE)

#define WEB_PAGE_READ_UP	0
#define WEB_PAGE_READ_DOWN	1

#define LCD_PAGE_READ_UP	0
#define LCD_PAGE_READ_DOWN	1

//当前web读取的log页数
extern u8 crtWebLogPageNum;

extern u8 logType;
extern u8 lastWebPageReadDir;
extern u8 lastLCDPageReadDir;

extern u8 crtWebPageLogData[EACH_WEB_PAGE_LOG_BYTE_LEN];
extern u8 crtLCDPageLogData[EACH_LCD_PAGE_LOG_NUM][EACH_LOG_SIZE];

extern u8 webLogPageFlag;
extern u8 LCDLogPageFlag;

void Log_Init(void);
void updateLogPageFlag(u8 logType, u8 logFlag);
void WriteLog(u8 logType, u8 alarmFlag);
void ReadFirstLog(u8 logType, u8 *logDataStr);
void ReadNextLog(u8 logType, u8 *logDataStr);
void ReadPreviousLog(u8 logType, u8 *logDataStr);
void ClearAllLog(void);
void ClearLog(u8 logType);
void FormatLogEEprom(void);
void FastClearLog(u8 logType);
void FastEraseLogEEProm(u16 eraseAddr, u16 eraseLen);

u8 isLogFull(u8 logType);
u8 isReadLoopEnd(u8 logType);
u8 isWebLogPageOverOne(u8 logType);
u8 isLCDLogPageOverOne(u8 logType);

void ReadWebFirstPageLog(u8 logType);
void ReadWebNextPageLog(u8 logType);
void ReadWebPreviousPageLog(u8 logType);

void ReadLCDFirstPageLog(u8 logType);
void ReadLCDNextPageLog(u8 logType);
void ReadLCDPreviousPageLog(u8 logType);
//获取Web日志页总数
u8 getWebLogPageTotal(u8 logType);
	
#endif
