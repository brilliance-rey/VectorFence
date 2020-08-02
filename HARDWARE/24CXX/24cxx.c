#include "24cxx.h"
#include "lwip_comm.h"
#include "sys.h"
#include "delay.h"
#include "alarm.h"
#include "adc.h"


//��ʼ��IIC�ӿ�
void AT24CXX_Init(void)
{
//	IIC_Init();//IIC��ʼ��
}
//��AT24CXXָ����ַ����һ������
//ReadAddr:��ʼ�����ĵ�ַ  
//����ֵ  :����������
u8 AT24CXX_ReadOneByte(u16 ReadAddr)
{				  
	u8 temp=0;

	u8 err = 0;
	OSMutexPend(iicMutexSem, (INT32U)IIC_MUTEX_TIMEOUT, &err);

    IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(AT24CXX_SLAVER_ADDR);	   //����д����
		IIC_Wait_Ack();
		IIC_Send_Byte(ReadAddr>>8);//���͸ߵ�ַ	    
	}else{
		IIC_Send_Byte(AT24CXX_SLAVER_ADDR+((ReadAddr/256)<<1));   //����������ַ0XA0,д���� 	
	}
	IIC_Wait_Ack(); 
  
    IIC_Send_Byte(ReadAddr%256);   //���͵͵�ַ
	IIC_Wait_Ack();
	IIC_Start();
	IIC_Send_Byte(AT24CXX_SLAVER_ADDR | 0x01);           //�������ģʽ			   
	IIC_Wait_Ack();
    temp=IIC_Read_Byte(0);
    IIC_Stop();//����һ��ֹͣ����

    OSMutexPost(iicMutexSem);

	return temp;
}

//��AT24CXXָ����ַд��һ������
//WriteAddr  :д�����ݵ�Ŀ�ĵ�ַ    
//DataToWrite:Ҫд�������
void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite)
{
    u8 err = 0;
    OSMutexPend(iicMutexSem, (INT32U)IIC_MUTEX_TIMEOUT, &err);

    IIC_Start();
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(AT24CXX_SLAVER_ADDR);	    //����д����
		IIC_Wait_Ack();
		IIC_Send_Byte(WriteAddr>>8);//���͸ߵ�ַ	  
	}else{
		IIC_Send_Byte(AT24CXX_SLAVER_ADDR+((WriteAddr/256)<<1));   //����������ַ0XA0,д���� 	 
	}
	IIC_Wait_Ack();	   
    IIC_Send_Byte(WriteAddr%256);   //���͵͵�ַ
	IIC_Wait_Ack(); 	 										  		   
	IIC_Send_Byte(DataToWrite);     //�����ֽ�							   
	IIC_Wait_Ack();  		    	   
    IIC_Stop();//����һ��ֹͣ���� 
	delay_us(10000);//��os����,���������delay us ����

    OSMutexPost(iicMutexSem);
}


//��AT24CXX�����ָ����ַ��ʼд�볤��ΪLen������
//�ú�������д��16bit����32bit������.
//WriteAddr  :��ʼд��ĵ�ַ  
//DataToWrite:���������׵�ַ
//Len        :Ҫд�����ݵĳ���2,4
void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len)
{  	
	u8 t;
	for(t=0;t<Len;t++)
	{
		AT24CXX_WriteOneByte(WriteAddr+t,(DataToWrite>>(8*t))&0xff);
	}												    
}

//��AT24CXX�����ָ����ַ��ʼ��������ΪLen������
//�ú������ڶ���16bit����32bit������.
//ReadAddr   :��ʼ�����ĵ�ַ 
//����ֵ     :����
//Len        :Ҫ�������ݵĳ���2,4
u32 AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len)
{  	
	u8 t;
	u32 temp=0;
	for(t=0;t<Len;t++)
	{
		temp<<=8;
		temp+=AT24CXX_ReadOneByte(ReadAddr+Len-t-1); 	 				   
	}
	return temp;												    
}

//���AT24CXX�Ƿ�����
//��������24XX�����һ����ַ(255)���洢��־��.
//���������24Cϵ��,�����ַҪ�޸�
//����1:���ʧ��
//����0:���ɹ�
u8 AT24CXX_Check(void)
{
	u8 temp;
	AT24CXX_WriteOneByte(EE_ADDR_MAX,0X55);
	temp=AT24CXX_ReadOneByte(EE_ADDR_MAX);//����ÿ�ο�����дAT24CXX			   
	if(temp==0X55)return 0;		   
	else//�ų���һ�γ�ʼ�������
	{
		AT24CXX_WriteOneByte(EE_ADDR_MAX,0X55);
	    temp=AT24CXX_ReadOneByte(EE_ADDR_MAX);	  
		if(temp==0X55)return 0;
	}
	return 1;											  
}

//��AT24CXX�����ָ����ַ��ʼ����ָ������������
//ReadAddr :��ʼ�����ĵ�ַ ��24c02Ϊ0~255
//pBuffer  :���������׵�ַ
//NumToRead:Ҫ�������ݵĸ���
void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead)
{
	while(NumToRead)
	{
		*pBuffer++=AT24CXX_ReadOneByte(ReadAddr++);	
		NumToRead--;
	}
}  
//��AT24CXX�����ָ����ַ��ʼд��ָ������������
//WriteAddr :��ʼд��ĵ�ַ ��24c02Ϊ0~255
//pBuffer   :���������׵�ַ
//NumToWrite:Ҫд�����ݵĸ���
void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite)
{
	while(NumToWrite--)
	{
		AT24CXX_WriteOneByte(WriteAddr,*pBuffer);
		WriteAddr++;
		pBuffer++;
	}
}

void FormatEEProm(void){
	u16 formatAddr = EE_ADDR_MAX + 1;
	while(formatAddr--)
	{
		AT24CXX_WriteOneByte(formatAddr, 0xFF);
	}
}

/**
* ����eraseAddr��ʼ�ĳ���ΪeraseLen������
*
**/
void EraseEEProm(u16 eraseAddr, u16 eraseLen){
	while(eraseLen--)
	{
		AT24CXX_WriteOneByte(eraseAddr, 0xFF);
		eraseAddr++;
	}
}

void EraseEEPormParas(void){
	EraseEEProm(NetParaAddr + 6, (18 - 6)); //MAC���ָ����������ָ�Ĭ������
	EraseEEProm(ADC_BASE_ADDR, 48);
	EraseEEProm(ALM_THRD_ADDR, 4);
	EraseEEProm(TENSION_MAX_RANGE_ADDR, 2);
	EraseEEProm(BASE_AUTO_CALIBRATE_TIME_ADDR, 2);
	EraseEEProm(FieldParaAddr, 4);
	EraseEEProm(ControlParaAddr, 1);
	EraseEEProm(SWValAddr, 2);
}

void WriteNetParasToEEProm(void){
	u16 addrOff = NetParaAddr;
	AT24CXX_Write(addrOff, lwipdev.mac, 6);
	addrOff +=6;
	AT24CXX_Write(addrOff, lwipdev.ip, 4);
	addrOff +=4;
	AT24CXX_Write(addrOff, lwipdev.netmask, 4);
	addrOff +=4;
	AT24CXX_Write(addrOff, lwipdev.gateway, 4);
	
}

void ReadNetParasFromEEProm(void){
	u16 addrOff = NetParaAddr;
	AT24CXX_Read(addrOff, lwipdev.mac, 6);
	addrOff +=6;
	AT24CXX_Read(addrOff, lwipdev.ip, 4);
	addrOff +=4;
	AT24CXX_Read(addrOff, lwipdev.netmask, 4);
	addrOff +=4;
	AT24CXX_Read(addrOff, lwipdev.gateway, 4);
}

/**
 *
 * �������ֵ(6��), KG u16 a_zero_val[6]; u16 b_zero_val[6];
 * axix_index: 0:X�ᣬ 1:Y��
 */
void WriteAdcZeroToEEProm(u8 axix_index){
	AT24CXX_Write((axix_index == 0? A_ADC_ZERO_ADDR : B_ADC_ZERO_ADDR), (u8 *)zero_val[axix_index], sizeof(zero_val[axix_index]));
}

/**
 *
 * ��ȡ���ֵ(6��), KG u16 a_zero_val[6]; u16 b_zero_val[6];
 * axix_index: 0:X�ᣬ 1:Y��
 */
void ReadAdcZeroFromEEProm(u8 axix_index){
	AT24CXX_Read((axix_index == 0? A_ADC_ZERO_ADDR : B_ADC_ZERO_ADDR), (u8 *)zero_val[axix_index], sizeof(zero_val[axix_index]));
}

/**
 *
 * �������ֵ(6��), KG u16 a_base_val[6]; u16 b_base_val[6];
 * axix_index: 0:X�ᣬ 1:Y��
 */
void WriteAdcBaseToEEProm(u8 axix_index){
	AT24CXX_Write((axix_index == 0? A_ADC_BASE_ADDR : B_ADC_BASE_ADDR), (u8 *)base_val[axix_index], sizeof(base_val[axix_index]));
}

/**
 *
 * ��ȡ���ֵ(6��), KG u16 a_base_val[6]; u16 b_base_val[6];
 * axix_index: 0:X�ᣬ 1:Y��
 */
void ReadAdcBaseFromEEProm(u8 axix_index){
	AT24CXX_Read((axix_index == 0? A_ADC_BASE_ADDR : B_ADC_BASE_ADDR), (u8 *)base_val[axix_index], sizeof(base_val[axix_index]));
}

void WriteAlmThrdToEEProm(void){
	AT24CXX_WriteOneByte(ALM_THRD_ADDR, (u8)((alarm_threshold_up_dif>>8) & 0xff));
	AT24CXX_WriteOneByte(ALM_THRD_ADDR + 1, (u8)(alarm_threshold_up_dif & 0xff));
	AT24CXX_WriteOneByte(ALM_THRD_ADDR + 2, (u8)((alarm_threshold_down_dif>>8) & 0xff));
	AT24CXX_WriteOneByte(ALM_THRD_ADDR + 3, (u8)(alarm_threshold_down_dif & 0xff));
}

void ReadAlmThrdFromEEProm(void){
	alarm_threshold_up_dif = AT24CXX_ReadOneByte(ALM_THRD_ADDR);
	alarm_threshold_up_dif = (alarm_threshold_up_dif << 8) | AT24CXX_ReadOneByte(ALM_THRD_ADDR + 1);
	alarm_threshold_down_dif = AT24CXX_ReadOneByte(ALM_THRD_ADDR + 2);
	alarm_threshold_down_dif = (alarm_threshold_down_dif << 8) | AT24CXX_ReadOneByte(ALM_THRD_ADDR + 3);
}

void WriteTensionMaxRangeToEEProm(void){
	AT24CXX_WriteOneByte(TENSION_MAX_RANGE_ADDR, (u8)((tension_max_range>>8) & 0xff));
	AT24CXX_WriteOneByte(TENSION_MAX_RANGE_ADDR + 1, (u8)(tension_max_range & 0xff));
}

void ReadTensionMaxRangeFromEEProm(void){
	tension_max_range = AT24CXX_ReadOneByte(TENSION_MAX_RANGE_ADDR);
	tension_max_range = (tension_max_range << 8) | AT24CXX_ReadOneByte(TENSION_MAX_RANGE_ADDR + 1);
}

void WriteBaseAutoCalibrateTimeToEEProm(void){
	AT24CXX_WriteOneByte(BASE_AUTO_CALIBRATE_TIME_ADDR, (u8)((base_auto_calibrate_time>>8) & 0xff));
	AT24CXX_WriteOneByte(BASE_AUTO_CALIBRATE_TIME_ADDR + 1, (u8)(base_auto_calibrate_time & 0xff));
}

void ReadBaseAutoCalibrateTimeFromEEProm(void){
	base_auto_calibrate_time = AT24CXX_ReadOneByte(BASE_AUTO_CALIBRATE_TIME_ADDR);
	base_auto_calibrate_time = (base_auto_calibrate_time << 8) | AT24CXX_ReadOneByte(BASE_AUTO_CALIBRATE_TIME_ADDR + 1);
}


//д�����������8bytes, field_num(1)+alarm_delay(2)+alarm_sensitivity(1)+ten_val_range[0](2)+ten_val_range[1](2)}
void WriteFieldParaToEEProm(void){
	//field_numΪ8λ�� FieldParaAddrԤ���ա�
	u8 addrTmp = FieldParaAddr;
	u8 i;
	for(i = 0; i < 2; i++){
		AT24CXX_WriteOneByte(addrTmp++, field_num[i]);
		AT24CXX_WriteOneByte(addrTmp++, (u8)((alarm_delay[i]>>8) & 0xff));
		AT24CXX_WriteOneByte(addrTmp++, (u8)(alarm_delay[i] & 0xff));
		AT24CXX_WriteOneByte(addrTmp++, alarm_sensitivity[i]);
		AT24CXX_WriteOneByte(addrTmp++, (u8)((ten_val_range[i][0]>>8) & 0xff));
		AT24CXX_WriteOneByte(addrTmp++, (u8)(ten_val_range[i][0] & 0xff));
		AT24CXX_WriteOneByte(addrTmp++, (u8)((ten_val_range[i][1]>>8) & 0xff));
		AT24CXX_WriteOneByte(addrTmp++, (u8)(ten_val_range[i][1] & 0xff));
	}
}

//��ȡ����������8bytes, field_num(1)+alarm_delay(2)+alarm_sensitivity(1)+ten_val_range[0](2)+ten_val_range[1](2)}
void ReadFieldParaFromEEProm(void){
	u8 addrTmp = FieldParaAddr;
	u8 i;
	for(i = 0; i < 2; i++){
		field_num[i] = AT24CXX_ReadOneByte(addrTmp++);
		alarm_delay[i] = AT24CXX_ReadOneByte(addrTmp++);
		alarm_delay[i] = (alarm_delay[i] << 8) | AT24CXX_ReadOneByte(addrTmp++);
		alarm_sensitivity[i] = AT24CXX_ReadOneByte(addrTmp++);
		ten_val_range[i][0] = AT24CXX_ReadOneByte(addrTmp++);
		ten_val_range[i][0] = (ten_val_range[i][0] << 8) | AT24CXX_ReadOneByte(addrTmp++);
		ten_val_range[i][1] = AT24CXX_ReadOneByte(addrTmp++);
		ten_val_range[i][1] = (ten_val_range[i][1] << 8) | AT24CXX_ReadOneByte(addrTmp++);
	}
}

void WriteControlParaToEEProm(void){
	u8 controlPara = (u8)(((flag_fcgjsn & 0x01)<<2) | ((BU_FANG_B_FLAG & 0x01)<<1) | (BU_FANG_A_FLAG & 0x01));
	AT24CXX_Write(ControlParaAddr, &controlPara, 1);
}

void ReadControlParaFromEEProm(void){
	u8 controlPara;
	AT24CXX_Read(ControlParaAddr, &controlPara, 1);
	flag_fcgjsn = (controlPara>>2) & 0x01;
	BU_FANG_B_FLAG = (controlPara>>1) & 0x01;
	BU_FANG_A_FLAG = controlPara & 0x01;
}

void WriteSWValToEEProm(void){
	AT24CXX_WriteOneByte(SWValAddr, SW_Type[0]);
	AT24CXX_WriteOneByte(SWValAddr + 1, SW_Type[1]);
}

void ReadSWValToEEProm(void){
	SW_Type[0] = AT24CXX_ReadOneByte(SWValAddr);
	SW_Type[1] = AT24CXX_ReadOneByte(SWValAddr + 1);
}

void WriteConfigCodeToEEProm(void){
	AT24CXX_WriteOneByte(ConfigCodeAddr, config_code);
}

void ReadConfigCodeToEEProm(void){
	config_code = AT24CXX_ReadOneByte(ConfigCodeAddr);
}
