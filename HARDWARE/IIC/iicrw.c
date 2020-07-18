#include "iicrw.h"

/***************************************************************************
功    能：IIC读取指定芯片函数
输入参数：SlaveAddr：从设备芯片地址，ReadAddr:读取地址, *RBuf:读取缓冲，RBufSize:读取数据大小
返    回：成功：读取字节数，失败：(ERROR:-1)
***************************************************************************/
u8 IICRead(u8 SlaveAddr, u8 ReadAddr, u8 *RBuf, u16 RBufSize)
{
    u8 tempSize = RBufSize;	
    
    u8 err = 0;
    OSMutexPend(iicMutexSem, (INT32U)IIC_MUTEX_TIMEOUT, &err);

    IIC_Start();  
	IIC_Send_Byte(SlaveAddr);   //发送device SlaveAddr	
	if(IIC_Wait_Ack()){
		IIC_Stop();
	    OSMutexPost(iicMutexSem);
		return (u8)-1;
	}
	  
    IIC_Send_Byte(ReadAddr);   //ReadAddr
	if(IIC_Wait_Ack()){
		IIC_Stop();
	    OSMutexPost(iicMutexSem);
		return (u8)-1;
	}
	
	IIC_Start();  
	IIC_Send_Byte(SlaveAddr | 0x01);   //发送device SlaveAddr	
	if(IIC_Wait_Ack()){
		IIC_Stop();
	    OSMutexPost(iicMutexSem);
		return (u8)-1;
	}
	
	while(tempSize > 1)
	{
		*RBuf++=IIC_Read_Byte(1);	
		tempSize--;
	}
	
	*RBuf++=IIC_Read_Byte(0);	
    IIC_Stop();//产生一个停止条件
    OSMutexPost(iicMutexSem);

	return RBufSize;
}


/***********************************************************************************************************
功    能：IIC写入指定芯片函数
输入参数：SlaveAddr：从设备芯片地址，WriteAddr:读取地址, *WBuf:读取缓冲，WBufSize:读取数据大小
返    回：成功：写入字节数，失败：(ERROR:-1)
*************************************************************************************************************/
u8 IICWrite(u8 SlaveAddr, u8 WriteAddr, u8 *WBuf, u16 WBufSize)
{
	u8 tempSize = WBufSize;

    u8 err = 0;
    OSMutexPend(iicMutexSem, (INT32U)IIC_MUTEX_TIMEOUT, &err);
    
    IIC_Start();  
	IIC_Send_Byte(SlaveAddr);   //发送device SlaveAddr	
	if(IIC_Wait_Ack()){
		IIC_Stop();
	    OSMutexPost(iicMutexSem);
		return (u8)-1;
	}
	  
    IIC_Send_Byte(WriteAddr);   //WriteAddr
	if(IIC_Wait_Ack()){
		IIC_Stop();
	    OSMutexPost(iicMutexSem);
		return (u8)-1;
	}
	
	while(tempSize)
	{
		IIC_Send_Byte(*WBuf++);
		if(IIC_Wait_Ack()){
			IIC_Stop();
		    OSMutexPost(iicMutexSem);
			return (u8)-1;
		}
		tempSize--;
	}
	
    IIC_Stop();//产生一个停止条件
    OSMutexPost(iicMutexSem);

	return WBufSize;
}
