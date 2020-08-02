#ifndef __TCP_SERVER_H
#define __TCP_SERVER_H
#include "sys.h"
#include "includes.h"
#include "lwip/api.h"

#define DEVICE_TYPE_PEF		0	//����Χ��
#define DEVICE_TYPE_TENSION	1	//����Χ��
#define DEVICE_TYPE_VERTOR	2	//ʸ��Χ��

#define TCP_SERVER_RX_BUFSIZE	512		//����tcp server���������ݳ���
#define TCP_SERVER_PORT			9990	//����tcp server�Ķ˿�
#define LWIP_SEND_DATA			0X80	//���������ݷ���

extern u8 tcp_server_recvbuf[TCP_SERVER_RX_BUFSIZE];	//TCP�ͻ��˽������ݻ�����
//extern u8 tcp_server_flag;			//TCP���������ݷ��ͱ�־λ

#define MSG_HEAD		0x94
#define RESULT_OK		0x00
#define RESULT_ERROR	0x01

/**
* ��������	0x01
* ������������״̬	0x02
* ������λ	0x03
* �޸���������	0x04
* ��ȡ���״̬	0x05
**/
#define MSG_TYPE_SEARCH_DEVICE				0x01
#define MSG_TYPE_SET_BUFANG					0x02
#define MSG_TYPE_ALARM_RESET				0x03
#define MSG_TYPE_SET_PARAS					0x04
#define MSG_TYPE_GET_DELAYED_ALARM_STATUS	0x05
#define MSG_TYPE_GET_STATUS					0x06
#define MSG_TYPE_NULL				0xff

////���岼��״̬��0x04:�ر�, 0x03:AB����, 0x02:B����, 0x01:A����, 0x00:AB����.
//#define DEFENCE_STATUS_OFF			0x04
//#define DEFENCE_STATUS_BUFANG_B_A	0x03
//#define DEFENCE_STATUS_BUFANG_B_NA	0x02
//#define DEFENCE_STATUS_BUFANG_NB_A	0x01
//#define DEFENCE_STATUS_BUFANG_NB_NA	0x00

//���岼��״̬��  2-0: "�ر�", "����", "����" //�˴������������Χ���Ĳ�����һ�£�Ϊ1��0�����Թر�ȡֵΪ2

#define DEFENCE_STATUS_SHUTDOWN	0x04
#define DEFENCE_STATUS_POWER	0x03
#define DEFENCE_STATUS_OFF		0x02
#define DEFENCE_STATUS_BUFANG	0x01
#define DEFENCE_STATUS_CHEFANG	0x00


void disconnect_server(struct netconn *tcp_conn);

err_t tcp_send_data(u8 field_num, struct netconn *tcp_conn, u8 msg_type);
err_t tcp_recv_data(struct netconn *tcp_conn);
err_t parse_reply_data(struct netconn *tcp_conn);

u8 tcp_server_init(void);		//TCP��������ʼ��(����TCP�������߳�)
#endif

