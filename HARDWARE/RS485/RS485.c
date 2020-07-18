#include "delay.h"
#include "RS485.h"


char equAddrStr[2] = {"01"};//本机地址:（01-32）
char equAddrStr_temp[2] = {0};//本机地址字符串暂存，用于编辑显示，不会影响读过来的本机地址字符串

char baudStr[5][5] = {
	                    " 2400",
	                    " 4800",
	                    " 9600",
	                    "19200",
	                    "38400"
	                      };//波特率:（2400、4800、9600、19200、38400）
//char baudStr_temp[5][5] = {"","","","",""};//波特率字符串暂存，用于编辑显示，不会影响读过来的波特率字符串
unsigned char rs485BaudSel = 2;//RS485波特率选择（默认9600）
unsigned char rs485BaudSel_temp = 2;//RS485波特率选择暂存（默认9600）



