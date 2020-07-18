#ifndef __IICRW_H
#define __IICRW_H
#include "myiic.h"   

#define OK     1
#define ERROR  0

u8 IICRead(u8 SlaveAddr, u8 ReadAddr, u8 *RBuf, u16 RBufSize);
u8 IICWrite(u8 SlaveAddr, u8 WriteAddr, u8 *WBuf, u16 WBufSize);

#endif
















