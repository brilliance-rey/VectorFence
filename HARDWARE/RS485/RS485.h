#ifndef    _RS485_H_
#define    _RS485_H_

extern    char equAddrStr[2];//������ַ:��01-32��
extern	  char equAddrStr_temp[2];//������ַ�ַ����ݴ棬���ڱ༭��ʾ������Ӱ��������ı�����ַ�ַ���

extern    char baudStr[5][5];//������:��2400��4800��9600��19200��38400��
//extern	  char baudStr_temp[5][5];//�������ַ����ݴ棬���ڱ༭��ʾ������Ӱ��������Ĳ������ַ���
extern    unsigned char rs485BaudSel;//RS485������ѡ��
extern    unsigned char rs485BaudSel_temp;//RS485������ѡ���ݴ�

#define    RS485_TX_RX_SEL    PBout(5)
#define    RS485_TX_EN   (RS485_TX_RX_SEL = 1)//����ʹ��
#define    RS485_RX_EN   (RS485_TX_RX_SEL = 0)//����ʹ��


#endif
