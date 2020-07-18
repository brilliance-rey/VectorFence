#ifndef    _RS485_H_
#define    _RS485_H_

extern    char equAddrStr[2];//本机地址:（01-32）
extern	  char equAddrStr_temp[2];//本机地址字符串暂存，用于编辑显示，不会影响读过来的本机地址字符串

extern    char baudStr[5][5];//波特率:（2400、4800、9600、19200、38400）
//extern	  char baudStr_temp[5][5];//波特率字符串暂存，用于编辑显示，不会影响读过来的波特率字符串
extern    unsigned char rs485BaudSel;//RS485波特率选择
extern    unsigned char rs485BaudSel_temp;//RS485波特率选择暂存

#define    RS485_TX_RX_SEL    PBout(5)
#define    RS485_TX_EN   (RS485_TX_RX_SEL = 1)//发送使能
#define    RS485_RX_EN   (RS485_TX_RX_SEL = 0)//接收使能


#endif
