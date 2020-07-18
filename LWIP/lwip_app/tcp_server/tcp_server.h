#ifndef __TCP_SERVER_H
#define __TCP_SERVER_H
#include "sys.h"
#include "includes.h"
#include "lwip/api.h"

#define DEVICE_TYPE_PEF		0	//电子围栏
#define DEVICE_TYPE_TENSION	1	//张力围栏

#define TCP_SERVER_RX_BUFSIZE	512		//定义tcp server最大接收数据长度
#define TCP_SERVER_PORT			9990	//定义tcp server的端口
#define LWIP_SEND_DATA			0X80	//定义有数据发送

extern u8 tcp_server_recvbuf[TCP_SERVER_RX_BUFSIZE];	//TCP客户端接收数据缓冲区
//extern u8 tcp_server_flag;			//TCP服务器数据发送标志位

#define MSG_HEAD		0x94
#define RESULT_OK		0x00
#define RESULT_ERROR	0x01

/**
* 搜索主机	0x01
* 设置主机布防状态	0x02
* 报警复位	0x03
* 修改主机参数	0x04
* 获取监控状态	0x05
**/
#define MSG_TYPE_SEARCH_DEVICE				0x01
#define MSG_TYPE_SET_BUFANG					0x02
#define MSG_TYPE_ALARM_RESET				0x03
#define MSG_TYPE_SET_PARAS					0x04
#define MSG_TYPE_GET_DELAYED_ALARM_STATUS	0x05
#define MSG_TYPE_GET_STATUS					0x06
#define MSG_TYPE_NULL				0xff

////定义布防状态：0x04:关闭, 0x03:AB布防, 0x02:B布防, 0x01:A布防, 0x00:AB撤防.
//#define DEFENCE_STATUS_OFF			0x04
//#define DEFENCE_STATUS_BUFANG_B_A	0x03
//#define DEFENCE_STATUS_BUFANG_B_NA	0x02
//#define DEFENCE_STATUS_BUFANG_NB_A	0x01
//#define DEFENCE_STATUS_BUFANG_NB_NA	0x00

//定义布防状态：  2-0: "关闭", "布防", "撤防" //此处布撤防与电子围栏的布撤防一致，为1和0，所以关闭取值为2

#define DEFENCE_STATUS_SHUTDOWN	0x04
#define DEFENCE_STATUS_POWER	0x03
#define DEFENCE_STATUS_OFF		0x02
#define DEFENCE_STATUS_BUFANG	0x01
#define DEFENCE_STATUS_CHEFANG	0x00


void disconnect_server(struct netconn *tcp_conn);

err_t tcp_send_data(u8 field_num, struct netconn *tcp_conn, u8 msg_type);
err_t tcp_recv_data(struct netconn *tcp_conn);
err_t parse_reply_data(struct netconn *tcp_conn);

u8 tcp_server_init(void);		//TCP服务器初始化(创建TCP服务器线程)
#endif

