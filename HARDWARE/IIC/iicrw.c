#include "iicrw.h"

/***************************************************************************
��    �ܣ�IIC��ȡָ��оƬ����
���������SlaveAddr�����豸оƬ��ַ��ReadAddr:��ȡ��ַ, *RBuf:��ȡ���壬RBufSize:��ȡ���ݴ�С
��    �أ��ɹ�����ȡ�ֽ�����ʧ�ܣ�(ERROR:-1)
***************************************************************************/
u8 IICRead(u8 SlaveAddr, u8 ReadAddr, u8 *RBuf, u16 RBufSize)
{
    u8 tempSize = RBufSize;	
    
    u8 err = 0;
    OSMutexPend(iicMutexSem, (INT32U)IIC_MUTEX_TIMEOUT, &err);

    IIC_Start();  
	IIC_Send_Byte(SlaveAddr);   //����device SlaveAddr	
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
	IIC_Send_Byte(SlaveAddr | 0x01);   //����device SlaveAddr	
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
    IIC_Stop();//����һ��ֹͣ����
    OSMutexPost(iicMutexSem);

	return RBufSize;
}


/***********************************************************************************************************
��    �ܣ�IICд��ָ��оƬ����
���������SlaveAddr�����豸оƬ��ַ��WriteAddr:��ȡ��ַ, *WBuf:��ȡ���壬WBufSize:��ȡ���ݴ�С
��    �أ��ɹ���д���ֽ�����ʧ�ܣ�(ERROR:-1)
*************************************************************************************************************/
u8 IICWrite(u8 SlaveAddr, u8 WriteAddr, u8 *WBuf, u16 WBufSize)
{
	u8 tempSize = WBufSize;

    u8 err = 0;
    OSMutexPend(iicMutexSem, (INT32U)IIC_MUTEX_TIMEOUT, &err);
    
    IIC_Start();  
	IIC_Send_Byte(SlaveAddr);   //����device SlaveAddr	
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
	
    IIC_Stop();//����һ��ֹͣ����
    OSMutexPost(iicMutexSem);

	return WBufSize;
}
