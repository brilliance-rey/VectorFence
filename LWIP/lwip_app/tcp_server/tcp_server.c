#include "tcp_server.h"
#include "lwip/opt.h"
#include "lwip_comm.h"
#include "lwip/lwip_sys.h"
#include "delay.h"
#include "led.h"
#include "alarm.h"
#include "adc.h"
 
//////////////////////////////////////////////////////////////////////////////////
//NETCONN API编程方式的TCP服务器
//Auther: Eryan
//创建日期:2018/8/27
//版本：V1.0
//*******************************************************************************
//修改信息
//无
//////////////////////////////////////////////////////////////////////////////////

u8 tcp_server_recvbuf[TCP_SERVER_RX_BUFSIZE];	//TCP服务器接收数据缓冲区
u8 tcp_server_sendbuf[TCP_SERVER_RX_BUFSIZE];	//TCP服务器接收数据缓冲区
//u8 tcp_server_flag;								//TCP服务器数据发送标志位

//TCP服务器任务
#define TCPSERVER_PRIO		7
//任务堆栈大小
#define TCPSERVER_STK_SIZE	500
//任务堆栈
OS_STK TCPSERVER_TASK_STK[TCPSERVER_STK_SIZE];

u8 ip_change_flag = 0;

//tcp服务器任务
static void tcp_server_thread(void *arg)
{
//	OS_CPU_SR cpu_sr;
//	u32 data_len = 0;
//	struct pbuf *q;
	err_t err;	//, recv_err
//	u8 remot_addr[4];
	struct netconn *conn, *newconn;
	static ip_addr_t ipaddr;
	static u16_t port;

//	u8 msg_type = MSG_TYPE_SEARCH_DEVICE;
	
//	char rcvd[500] = {0};

	LWIP_UNUSED_ARG(arg);

	conn = netconn_new(NETCONN_TCP);  //创建一个TCP链接
	netconn_bind(conn,IP_ADDR_ANY,TCP_SERVER_PORT);  //绑定端口 8号端口
	netconn_listen(conn);  		//进入监听模式
	conn->recv_timeout = 3000;  	//3s连接超时
	while (1) 
	{
		if(ip_change_flag){
			lwip_reset_netif_ipaddr();
			return;
		}

		err = netconn_accept(conn, &newconn);  //接收连接请求

		if (err == ERR_OK)    //处理新连接的数据
		{
			newconn->recv_timeout = 10;	//禁止阻塞线程 等待10ms
//			struct netbuf *recvbuf;
			netconn_getaddr(newconn, &ipaddr, &port,0); //获取远端IP地址和端口号

//			remot_addr[3] = (uint8_t)(ipaddr.addr >> 24);
//			remot_addr[2] = (uint8_t)(ipaddr.addr>> 16);
//			remot_addr[1] = (uint8_t)(ipaddr.addr >> 8);
//			remot_addr[0] = (uint8_t)(ipaddr.addr);
		//			printf("主机%d.%d.%d.%d连接上服务器,主机端口号为:%d\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3],port);

//			delay_ms(100);
			while(1)
			{
				err = tcp_recv_data(newconn);
				if( ERR_OK == err){  //接收到数据长度不为0
					if(ERR_OK != parse_reply_data(newconn)){
						disconnect_server(newconn);
					}
//					LED_FANG_CHAI = 1;LED_OPEN = 1;
//					delay_ms(2000);
//					LED_FANG_CHAI = 0;LED_OPEN = 0;
//					delay_ms(2000);
//					delay_ms(10);
				}else{
					disconnect_server(newconn);
					break;
				}
			}
		}
//		//调试： 每次连接处理BuFang闪一下。
//		LED_BU_FANG = 1;
//		delay_ms(100);
//		LED_BU_FANG = 0;

		delay_ms(10);
	}
}

void disconnect_server(struct netconn *tcp_conn){
	netconn_close(tcp_conn);
	netconn_delete(tcp_conn);

//	LED_INVD = 1;
//	LED_OPEN = 1;
	//	printf("主机:%d.%d.%d.%d断开与服务器的连接\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
}

err_t tcp_send_err(u8 field_num_tmp, struct netconn *tcp_conn, u8 msg_type){
	err_t err;
	u8 sendLen = 0;

	memset(tcp_server_sendbuf, 0, TCP_SERVER_RX_BUFSIZE);
	tcp_server_sendbuf[sendLen++] = MSG_HEAD;
	tcp_server_sendbuf[sendLen++] = 0;		//msg_len
	tcp_server_sendbuf[sendLen++] = field_num_tmp;	//field_num_tmp
	tcp_server_sendbuf[sendLen++] = DEVICE_TYPE_TENSION;	//device_type
	tcp_server_sendbuf[sendLen++] = msg_type;	//msg_type
	tcp_server_sendbuf[sendLen++] = RESULT_ERROR;	//error

	tcp_server_sendbuf[1] = sendLen - 2;		//msg_len
	err = netconn_write(tcp_conn, tcp_server_sendbuf, sendLen, NETCONN_COPY); //发送tcp_server_sentbuf中的数据
	return err;
}

err_t tcp_send_data(u8 field_num_tmp, struct netconn *tcp_conn, u8 msg_type){
	err_t err;
	u8 sendLen = 0;
	u8 deviceType = DEVICE_TYPE_TENSION;
	u8 fieldIndex = getFieldIndex(field_num_tmp);
	u8 bfFlag = getBFFlag(field_num_tmp);
	u16 alarmDly = getAlarmDelay(field_num_tmp);
	u8 peerFieldNum = getPeerFieldNum(field_num_tmp);


	memset(tcp_server_sendbuf, 0, TCP_SERVER_RX_BUFSIZE);
	tcp_server_sendbuf[sendLen++] = MSG_HEAD;
	tcp_server_sendbuf[sendLen++] = 0;		//msg_len
	tcp_server_sendbuf[sendLen++] = field_num_tmp;	//field_num_tmp
	tcp_server_sendbuf[sendLen++] = deviceType;	//device_type
	tcp_server_sendbuf[sendLen++] = msg_type;	//msg_type
	tcp_server_sendbuf[sendLen++] = RESULT_OK;	//result_flag

	switch(msg_type){
		case MSG_TYPE_SEARCH_DEVICE:
			tcp_server_sendbuf[sendLen++] = field_num[0];  //A区防区号
			tcp_server_sendbuf[sendLen++] = field_num[1];  //同伴防区号:B区防区号
			break;

		case MSG_TYPE_SET_PARAS:
			break;

		case MSG_TYPE_GET_DELAYED_ALARM_STATUS:
			tcp_server_sendbuf[sendLen++] = remote_shutdown? 4:bfFlag;	//defence_status  B,A区布防状态， 1：布防， 0：撤防。
			tcp_server_sendbuf[sendLen++] = getAlarmDelayedStatus(field_num_tmp);	//告警状态
			tcp_server_sendbuf[sendLen++] = (u8)((alarmDly >> 8) & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)(alarmDly & 0xff);

			tcp_server_sendbuf[sendLen++] = alarm_sensitivity[fieldIndex];

			tcp_server_sendbuf[sendLen++] = (u8)((alarm_threshold_up_dif >> 8) & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)(alarm_threshold_up_dif & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)((alarm_threshold_down_dif >> 8) & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)(alarm_threshold_down_dif & 0xff);

			tcp_server_sendbuf[sendLen++] = (u8)((tension_max_range >> 8) & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)(tension_max_range & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)((base_auto_calibrate_time >> 8) & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)(base_auto_calibrate_time & 0xff);

//			tcp_server_sendbuf[sendLen++] = (u8)(ten_val_range[fieldIndex][0]/10) & 0xff;
//			tcp_server_sendbuf[sendLen++] = (u8)(ten_val_range[fieldIndex][1]/10) & 0xff;

			tcp_server_sendbuf[sendLen++] = peerFieldNum;  //同伴防区号
			tcp_server_sendbuf[sendLen++] = SW_Type[0];  //开关量1类型
			tcp_server_sendbuf[sendLen++] = SW_Type[1];  //开关量2类型
			
			break;

		case MSG_TYPE_GET_STATUS:
			tcp_server_sendbuf[sendLen++] = remote_shutdown? 4:bfFlag;	//defence_status  B,A区布防状态， 1：布防， 0：撤防。
			tcp_server_sendbuf[sendLen++] = getAlarmStatus(field_num_tmp);	//告警状态
			tcp_server_sendbuf[sendLen++] = (u8)((alarmDly >> 8) & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)(alarmDly & 0xff);

			tcp_server_sendbuf[sendLen++] = alarm_sensitivity[fieldIndex];

			tcp_server_sendbuf[sendLen++] = (u8)((alarm_threshold_up_dif >> 8) & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)(alarm_threshold_up_dif & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)((alarm_threshold_down_dif >> 8) & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)(alarm_threshold_down_dif & 0xff);

			tcp_server_sendbuf[sendLen++] = (u8)((tension_max_range >> 8) & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)(tension_max_range & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)((base_auto_calibrate_time >> 8) & 0xff);
			tcp_server_sendbuf[sendLen++] = (u8)(base_auto_calibrate_time & 0xff);

//			tcp_server_sendbuf[sendLen++] = (u8)(ten_val_range[fieldIndex][0]/10) & 0xff;
//			tcp_server_sendbuf[sendLen++] = (u8)(ten_val_range[fieldIndex][1]/10) & 0xff;

			tcp_server_sendbuf[sendLen++] = peerFieldNum;  //同伴防区号
			tcp_server_sendbuf[sendLen++] = SW_Type[0];  //开关量1类型
			tcp_server_sendbuf[sendLen++] = SW_Type[1];  //开关量2类型
			break;

		case MSG_TYPE_SET_BUFANG:
		case MSG_TYPE_ALARM_RESET:
			break;

		default:
			break;
	}

	tcp_server_sendbuf[1] = sendLen - 2;		//msg_len
	err = netconn_write(tcp_conn, tcp_server_sendbuf, sendLen, NETCONN_COPY); //发送tcp_server_sentbuf中的数据
	return err;
}

/**
 * receive the data
 * 返回：data_len 接收到的数据长度。
 */
err_t tcp_recv_data(struct netconn *tcp_conn){
	OS_CPU_SR cpu_sr;

	err_t recv_err, err;
	u32 data_len = 0;
	struct pbuf *q;
	struct netbuf *recvbuf;

	tcp_conn->recv_timeout = 3000;
	if ((recv_err = netconn_recv(tcp_conn, &recvbuf)) == ERR_OK){  //接收到数据
		OS_ENTER_CRITICAL(); //关中断
		memset(tcp_server_recvbuf, 0, TCP_SERVER_RX_BUFSIZE);  //数据接收缓冲区清零
		for (q = recvbuf->p; q != NULL; q = q->next){  //遍历完整个pbuf链表
			//判断要拷贝到TCP_SERVER_RX_BUFSIZE中的数据是否大于TCP_SERVER_RX_BUFSIZE的剩余空间，如果大于
			//的话就只拷贝TCP_SERVER_RX_BUFSIZE中剩余长度的数据，否则的话就拷贝所有的数据
			if (q->len > (TCP_SERVER_RX_BUFSIZE - data_len)){
				memcpy(tcp_server_recvbuf + data_len, q->payload, (TCP_SERVER_RX_BUFSIZE - data_len));//拷贝数据
			}else{
				memcpy(tcp_server_recvbuf + data_len, q->payload, q->len);
			}
			data_len += q->len;
			if (data_len > TCP_SERVER_RX_BUFSIZE)
				break; //超出TCP客户端接收数组,跳出
		}
		OS_EXIT_CRITICAL();  //开中断
		netbuf_delete(recvbuf);

		//同步头不对
		if(tcp_server_recvbuf[0] != MSG_HEAD){
			err = tcp_send_err(tcp_server_recvbuf[2], tcp_conn, tcp_server_recvbuf[4]);
			return RESULT_ERROR;
		}

		//长度不对
		if(tcp_server_recvbuf[1] != (data_len - 2)){
			err = tcp_send_err(tcp_server_recvbuf[2], tcp_conn, tcp_server_recvbuf[4]);
			return RESULT_ERROR;
		}

		//设备类型不对
		if(tcp_server_recvbuf[4] != MSG_TYPE_SEARCH_DEVICE && tcp_server_recvbuf[3] != DEVICE_TYPE_TENSION){
			err = tcp_send_err(tcp_server_recvbuf[2], tcp_conn, tcp_server_recvbuf[4]);
			return RESULT_ERROR;
		}
	}
	return recv_err;
}

err_t parse_reply_data(struct netconn *tcp_conn){
	u8 offset = 0;
	u8 msgLen = 0;
	u8 fieldNum = 0, deviceType = 0;
	u8 msgType = 0, defenceStatus = 0;
	err_t err;
	u8 newFieldNum = 0;

	if(tcp_server_recvbuf[offset++] != MSG_HEAD){		//同步头错误
		return RESULT_ERROR;
	}
	msgLen = tcp_server_recvbuf[offset++]; // msg_len
	fieldNum = tcp_server_recvbuf[offset++];
	deviceType = tcp_server_recvbuf[offset++];
	msgType = tcp_server_recvbuf[offset++];

	//防区号不对
	if(msgType != MSG_TYPE_SEARCH_DEVICE && (fieldNum != field_num[0] && fieldNum != field_num[1])){
		err = tcp_send_err(fieldNum, tcp_conn, msgType);
		return err;
	}

	switch(msgType){
		case MSG_TYPE_SEARCH_DEVICE:
			break;
		case MSG_TYPE_SET_BUFANG:
			defenceStatus = tcp_server_recvbuf[offset++];
			offset++; //dfcLevel = tcp_server_recvbuf[offset++];
			switch(defenceStatus){
				case DEFENCE_STATUS_BUFANG:
				case DEFENCE_STATUS_CHEFANG:
					if(!remote_shutdown){
						//if (dfcLevel >= 0 && dfcLevel <= 8) {
						//	setPWMLevelByFieldNum(fieldNum, dfcLevel);
						//}
						modifyBFFlag(fieldNum, defenceStatus);
					}
					break;

				case DEFENCE_STATUS_POWER:
					remoteShutdown(0);
					break;

				case DEFENCE_STATUS_SHUTDOWN:
					remoteShutdown(1);
					break;

				case DEFENCE_STATUS_OFF:
					break;

				default:
					break;
			}
			break;

		case MSG_TYPE_ALARM_RESET:
			removeAlarmRL(fieldNum);
			break;

		case MSG_TYPE_SET_PARAS:
//			new_field_num+new_device_ip+alarm_delay+alarm_sensitivity+alm_thrd_up_dif+alm_thrd_dw_dif+tension_max_range+base_auto_calibrate_time
			newFieldNum = tcp_server_recvbuf[offset++];
			modifyFieldNum(fieldNum, newFieldNum);
			//防区号修改后要用新的防区号回应。
			fieldNum = newFieldNum;

			if (lwipdev.ip[0] != tcp_server_recvbuf[offset]){
				lwipdev.ip[0] = tcp_server_recvbuf[offset];
				ip_change_flag = 1;
			}
			offset++;
			if (lwipdev.ip[1] != tcp_server_recvbuf[offset]){
				lwipdev.ip[1] = tcp_server_recvbuf[offset];
				ip_change_flag = 1;
			}
			offset++;
			if (lwipdev.ip[2] != tcp_server_recvbuf[offset]){
				lwipdev.ip[2] = tcp_server_recvbuf[offset];
				ip_change_flag = 1;
			}
			offset++;
			if (lwipdev.ip[3] != tcp_server_recvbuf[offset]){
				lwipdev.ip[3] = tcp_server_recvbuf[offset];
				ip_change_flag = 1;
			}
			offset++;
			//Error,会将0x1234->0x3412: setAlarmDelay((u16)(tcp_server_recvbuf[offset++] << 8 | tcp_server_recvbuf[offset++]));
			modifyAlarmDelay(fieldNum, (u16)(tcp_server_recvbuf[offset] << 8 | tcp_server_recvbuf[offset+1]));
			offset +=2;
			setAlarmSensitivity(getFieldIndex(fieldNum), tcp_server_recvbuf[offset++]);

			setAlmThrdUpDif((u16)(tcp_server_recvbuf[offset] << 8 | tcp_server_recvbuf[offset+1]));
			offset +=2;
			setAlmThrdDwDif((u16)(tcp_server_recvbuf[offset] << 8 | tcp_server_recvbuf[offset+1]));
			offset +=2;
			setTensionMaxRange((u16)(tcp_server_recvbuf[offset] << 8 | tcp_server_recvbuf[offset+1]));
			offset +=2;
			setBaseAutoCalibrateTime((u16)(tcp_server_recvbuf[offset] << 8 | tcp_server_recvbuf[offset+1]));
			offset +=2;
//			setTenValRange(getFieldIndex(fieldNum), 0, (u16)(tcp_server_recvbuf[offset++] * 10));
//			setTenValRange(getFieldIndex(fieldNum), 1, (u16)(tcp_server_recvbuf[offset++] * 10));

			break;
		case MSG_TYPE_GET_DELAYED_ALARM_STATUS:
		case MSG_TYPE_GET_STATUS:
			//此字段无意义，其实按switch前的fieldNum即可，16,17以后的版本注销此代码。
//			fieldNum = field_num[tcp_server_recvbuf[offset++]];
			break;
		default:
			break;
	}

	err = tcp_send_data(fieldNum, tcp_conn, msgType);
	return err;
}

//创建TCP服务器线程
//返回值:0 TCP服务器创建成功
//		其他 TCP服务器创建失败
u8 tcp_server_init(void)
{
	u8 res;
	OS_CPU_SR cpu_sr;

	OS_ENTER_CRITICAL();	//关中断
	res = OSTaskCreate(tcp_server_thread,(void*)0,(OS_STK*)&TCPSERVER_TASK_STK[TCPSERVER_STK_SIZE-1],TCPSERVER_PRIO); //创建TCP服务器线程
	OS_EXIT_CRITICAL();		//开中断

	return res;
}


