#include <stdio.h>
#include "lm75.h"

char crtLM75TempStr[5] = {0};//100.5��-99.5

/***************************************************************************
��    �ܣ������¶�оƬLM75��Pointer
���������pointer
��    �أ�1: OK,  0: error
***************************************************************************/
u8 LM75_SetPointer(u8 pointer)
{
    u8 SlaveAddr = LM75_SLAVE_ADDR;

    u8 err = 0;
    OSMutexPend(iicMutexSem, (INT32U)IIC_MUTEX_TIMEOUT, &err);

    IIC_Start();  
	IIC_Send_Byte(SlaveAddr);   ////����device slave Addr
	if(IIC_Wait_Ack()){  //��Ӧ��
		IIC_Stop();//����һ��ֹͣ����
	    OSMutexPost(iicMutexSem);
		return 0;
	}
	
	IIC_Send_Byte(pointer);   //����pointer byte
	if(IIC_Wait_Ack()){
		IIC_Stop();//����һ��ֹͣ����
	    OSMutexPost(iicMutexSem);
		return 0;
	}
	    	
	IIC_Stop();//����һ��ֹͣ����
    OSMutexPost(iicMutexSem);
	return 1;;
}



/***************************************************************************
��    �ܣ���ȡ�¶�оƬLM75
---------------------------------------
                     Digital Output
Temperature    ------------------------
                  Binary         Hex
                D15  -   D7
---------------------------------------
+125��C          0 1111 1010     0FAh
+25��C           0 0011 0010     032h
+0.5��C          0 0000 0001     001h
0��C             0 0000 0000     000h
-0.5��C          1 1111 1111     1FFh
-25��C           1 1100 1110     1CEh
-55��C           1 1001 0010     192h

D15 D14 D13 D12 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0
MSB Bit7 Bit6 Bit5 Bit4 Bit3 Bit2 Bit1 LSB X X X X X X X
D0�CD6: Undefined
D7�CD15: Temperature Data. One LSB = 0.5��C. Two��s complement format.
���㷽ʽ��if((temp & 0x8000) == 0x0000){������((temp>>8) & 0xff), 0.5����:((temp>>7)& 0x01 == 0x01)?0.5:0.0}
          if((temp & 0x8000) != 0x0000){������(~((temp>>8) & 0xff) + 1), 0.5����:((temp>>7)& 0x01 == 0x01)?0.5:0.0}

�����������
��    �أ������¶�ֵbit15-bit7: �������֣�bit6: {1��0.5}

***************************************************************************/
u16 LM75_ReadTemp(void)
{
    u16 temp=0;	
    u8 SlaveAddr = LM75_SLAVE_ADDR;
    //LM75_SetPointer(0x00);

    u8 err = 0;
    OSMutexPend(iicMutexSem, (INT32U)IIC_MUTEX_TIMEOUT, &err);
    
    IIC_Start();  
	IIC_Send_Byte(SlaveAddr | 0x01);   ////����device slave Addr
	if(IIC_Wait_Ack()){
		IIC_Stop();//����һ��ֹͣ����
		return 0;
	}

    temp = IIC_Read_Byte(1);
	temp = (u8)(temp + LM75_CORRECTION);
	temp = temp<<8;
	temp |=IIC_Read_Byte(0);
    		
    IIC_Stop();//����һ��ֹͣ����

    OSMutexPost(iicMutexSem);
	return temp;
}

/**
*
* ��ȡ�¶��ַ���ֵ
*
**/
void LM75_ReadTempStr(char *tempStr){
	u16 temp = LM75_ReadTemp();
	if((temp & 0x8000) == 0x00){  //����
		sprintf(tempStr, "%d.%d", (u8)(((temp>>8) & 0xff)), (u8)((((temp>>7)& 0x01) == 0x01)?5:0));
	}else{		//����
		sprintf(tempStr, "-%d.%d", (u8)(~((temp>>8) & 0xff) + 1), (u8)((((temp>>7)& 0x01) == 0x01)?5:0));
	}
}


