#ifndef __MYIIC_H
#define __MYIIC_H
#include "sys.h"
#include "includes.h"

//IO��������
#define SDA_IN()  {GPIOD->MODER&=~(3<<(6*2));GPIOD->MODER|=0<<6*2;}	//PD6����ģʽ
#define SDA_OUT() {GPIOD->MODER&=~(3<<(6*2));GPIOD->MODER|=1<<6*2;} //PD6���ģʽ
//IO��������	 
#define IIC_SCL    PDout(7) //SCL
#define IIC_SDA    PDout(6) //SDA	 
#define READ_SDA   PDin(6)  //����SDA 

#define IIC_MUTEX_PRIO		1
#define IIC_MUTEX_TIMEOUT	0
extern OS_EVENT *iicMutexSem;

//IIC���в�������
void IIC_Init(void);                //��ʼ��IIC��IO��				 
void IIC_Start(void);				//����IIC��ʼ�ź�
void IIC_Stop(void);	  			//����IICֹͣ�ź�
void IIC_Send_Byte(u8 txd);			//IIC����һ���ֽ�
u8 IIC_Read_Byte(unsigned char ack);//IIC��ȡһ���ֽ�
u8 IIC_Wait_Ack(void); 				//IIC�ȴ�ACK�ź�
void IIC_Ack(void);					//IIC����ACK�ź�
void IIC_NAck(void);				//IIC������ACK�ź�

void IIC_Write_One_Byte(u8 daddr,u8 addr,u8 data);
u8 IIC_Read_One_Byte(u8 daddr,u8 addr);	  
#endif
















