#include "lwip_comm.h" 
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/init.h"
#include "ethernetif.h" 
#include "lwip/timers.h"
#include "lwip/tcp_impl.h"
#include "lwip/ip_frag.h"
#include "lwip/tcpip.h"
#include "24cxx.h"
#include "malloc.h"
#include "httpd.h"
#include "delay.h"
#include "usart.h"  
#include <stdio.h>
#include "ucos_ii.h" 
//*******************************************************************************
//修改信息
//无
////////////////////////////////////////////////////////////////////////////////// 	   
   
__lwip_dev lwipdev;		//lwip控制结构体
struct netif lwip_netif;				//定义一个全局的网络接口
 
extern sys_mbox_t mbox;  				//消息邮箱 全局变量(在tcpip.c里面定义)
extern u32 memp_get_memorysize(void);	//在memp.c里面定义
extern u8_t *memp_memory;				//在memp.c里面定义.
extern u8_t *ram_heap;					//在mem.c里面定义.
extern sys_mbox_t mbox;  				//消息邮箱 全局变量(在tcpip.c里面定义)

extern u8_t netif_num;                  //netif.c文件
extern struct tcp_seg inseg;            //tcp_in.c文件
extern struct tcp_hdr *tcphdr;          //tcp_in.c文件
extern struct ip_hdr *iphdr;			//tcp_in.c文件
extern u32_t seqno, ackno;				//tcp_in.c文件
extern u16_t tcplen;					//tcp_in.c文件
extern u8_t recv_flags;					//tcp_in.c文件
extern struct pbuf *recv_data;			//tcp_in.c文件
extern u8_t etharp_cached_entry;        //etharp.c文件
extern u16_t ip_id;                     //ip.c文件
extern struct ip_reassdata *reassdatagrams; //ip_frag.c文件
extern u16_t ip_reass_pbufcount;        //ip_frag.c文件
extern u8_t *ram;						//mem.c文件
extern struct mem *ram_end;				//mem.c文件
extern struct mem *lfree;				//mem.c文件
extern sys_mutex_t mem_mutex;			//mem.c文件

extern void ip_reass_timer(void *arg);  //timers.c文件
extern void arp_timer(void *arg);		//timers.c文件
extern void dhcp_timer_coarse(void *arg);//timers.c文件
extern void dhcp_timer_fine(void *arg); //timers.c文件
extern struct sys_timeo *next_timeout;  //timers.c文件
extern int tcpip_tcp_timer_active;      //timers.c文件
/////////////////////////////////////////////////////////////////////////////////

u8 EthInitStatus = 0;
u8 lastLinkStatus = 0;
u8 crtLinkStatus = 0;

//MAC地址高三字节： 固定为:55.39.72(ERROR,huawei交换机不让用),低三字节用STM32唯一ID)
//用以下: 44:54:A5:XX:XX:XX/44:54:54:XX:XX:XX/44:54:56:XX:XX:XX/78:45:C4:XX:XX:XX
u8 mac_def[3] = {0x44, 0x54, 0xA5};

//lwip两个任务定义(内核任务和DHCP任务)

//lwip内核任务堆栈(优先级和堆栈大小在lwipopts.h定义了) 
OS_STK * TCPIP_THREAD_TASK_STK;	 

//lwip DHCP任务
//设置任务优先级
#define LWIP_DHCP_TASK_PRIO       		8
//设置任务堆栈大小
#define LWIP_DHCP_STK_SIZE  		    128
//任务堆栈，采用内存管理的方式控制申请	
OS_STK * LWIP_DHCP_TASK_STK;	
//任务函数
void lwip_dhcp_task(void *pdata); 


//用于以太网中断调用
void lwip_packet_handler(void)
{
	ethernetif_input(&lwip_netif);
}
//lwip内核部分,内存申请
//返回值:0,成功;
//    其他,失败
u8 lwip_comm_mem_malloc(void)
{
	u32 mempsize;
	u32 ramheapsize; 
	mempsize=memp_get_memorysize();			//得到memp_memory数组大小
	memp_memory=mymalloc(SRAMIN,mempsize);	//为memp_memory申请内存
	ramheapsize=LWIP_MEM_ALIGN_SIZE(MEM_SIZE)+2*LWIP_MEM_ALIGN_SIZE(4*3)+MEM_ALIGNMENT;//得到ram heap大小
	ram_heap=mymalloc(SRAMIN,ramheapsize);	//为ram_heap申请内存 
	TCPIP_THREAD_TASK_STK=mymalloc(SRAMIN,TCPIP_THREAD_STACKSIZE*4);//给内核任务申请堆栈 
	LWIP_DHCP_TASK_STK=mymalloc(SRAMIN,LWIP_DHCP_STK_SIZE*4);		//给dhcp任务堆栈申请内存空间
	if(!memp_memory||!ram_heap||!TCPIP_THREAD_TASK_STK||!LWIP_DHCP_TASK_STK)//有申请失败的
	{
		lwip_comm_mem_free();
		return 1;
	}
	return 0;	
}
//lwip内核部分,内存释放
void lwip_comm_mem_free(void)
{ 	
	myfree(SRAMIN,memp_memory);
	myfree(SRAMIN,ram_heap);
	myfree(SRAMIN,TCPIP_THREAD_TASK_STK);
	myfree(SRAMIN,LWIP_DHCP_TASK_STK);
}


void lwip_comm_ip_init()
{
	ReadNetParasFromEEProm();
	
	//MAC自动修正，原55:39:72在huawei交换机非法。
	if(lwipdev.mac[0] == 0x55 && lwipdev.mac[1] == 0x39 && lwipdev.mac[2] == 0x72){
		lwipdev.mac[0]=mac_def[0];
		lwipdev.mac[1]=mac_def[1];
		lwipdev.mac[2]=mac_def[2];
		WriteNetParasToEEProm();
	}
	
	//如果读到的数0xFF，则是出厂状态或default了。
	if(lwipdev.ip[0] == 0xFF && lwipdev.ip[1] == 0xFF && lwipdev.ip[2] == 0xFF && lwipdev.ip[3] == 0xFF){
		lwip_comm_default_ip_set(&lwipdev);
	}
}

//lwip 默认IP设置
//lwipx:lwip控制结构体指针
void lwip_comm_default_ip_set(__lwip_dev *lwipx)
{
	u32 sn0 = *(vu32*)(0x1FFF7A10);//获取STM32的唯一ID的前24位作为MAC地址后三字节
	//默认远端IP为:192.168.1.90
	lwipx->remoteip[0]=192;	
	lwipx->remoteip[1]=168;
	lwipx->remoteip[2]=0;
	lwipx->remoteip[3]=90;
	//MAC地址设置(高三字节(IEEE称之为组织唯一ID,OUI)固定，不恢复，低三字节用STM32唯一ID)
	if(lwipdev.mac[0] == 0xFF && lwipdev.mac[1] == 0xFF && lwipdev.mac[2] == 0xFF){
		lwipx->mac[0]=mac_def[0];
		lwipx->mac[1]=mac_def[1];
		lwipx->mac[2]=mac_def[2];
	}
	lwipx->mac[3]=(sn0>>16)&0XFF;
	lwipx->mac[4]=(sn0>>8)&0XFFF;
	lwipx->mac[5]=sn0&0XFF;
	//默认本地IP为:192.168.0.30
	lwipx->ip[0]=192;
	lwipx->ip[1]=168;
	lwipx->ip[2]=0;
	lwipx->ip[3]=30;
	//默认子网掩码:255.255.255.0
	lwipx->netmask[0]=255;	
	lwipx->netmask[1]=255;
	lwipx->netmask[2]=255;
	lwipx->netmask[3]=0;
	//默认网关:192.168.0.1
	lwipx->gateway[0]=192;
	lwipx->gateway[1]=168;
	lwipx->gateway[2]=0;
	lwipx->gateway[3]=1;
	lwipx->dhcpstatus=0;//没有DHCP	
	
	WriteNetParasToEEProm();
} 


//LWIP初始化(LWIP启动的时候使用)
//返回值:0,成功
//      1,内存错误
//      2,LAN8720初始化失败
//      3,网卡添加失败.
u8 lwip_comm_init(void)
{
	OS_CPU_SR cpu_sr;
	struct netif *Netif_Init_Flag;		//调用netif_add()函数时的返回值,用于判断网络初始化是否成功
	struct ip_addr ipaddr;  			//ip地址
	struct ip_addr netmask; 			//子网掩码
	struct ip_addr gw;      			//默认网关 
	if(ETH_Mem_Malloc())return 1;		//内存申请失败
	if(lwip_comm_mem_malloc())return 1;	//内存申请失败
	//if(LAN8720_Init())return 2;			//初始化LAN8720失败 
	
	LAN8720_Init();
	tcpip_init(NULL,NULL);				//初始化tcp ip内核,该函数里面会创建tcpip_thread内核任务
//	lwip_init();						//初始化LWIP内核

//	lwip_comm_default_ip_set(&lwipdev);  	//出厂设置默认IP等信息
	lwip_comm_ip_init();	//init IP等信息
//	ReadNetParasFromEEProm();
#if LWIP_DHCP		//使用动态IP
	ipaddr.addr = 0;
	netmask.addr = 0;
	gw.addr = 0;
#else				//使用静态IP
	IP4_ADDR(&ipaddr,lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
	IP4_ADDR(&netmask,lwipdev.netmask[0],lwipdev.netmask[1] ,lwipdev.netmask[2],lwipdev.netmask[3]);
	IP4_ADDR(&gw,lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
	//printf("网卡en的MAC地址为:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
	//printf("静态IP地址........................%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
	//printf("子网掩码..........................%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
	//printf("默认网关..........................%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
#endif
	OS_ENTER_CRITICAL();  //进入临界区
	Netif_Init_Flag=netif_add(&lwip_netif,&ipaddr,&netmask,&gw,NULL,&ethernetif_init,&tcpip_input);//向网卡列表中添加一个网口
	
	OS_EXIT_CRITICAL();  //退出临界区
	if(Netif_Init_Flag==NULL)return 3;//网卡添加失败 
	else//网口添加成功后,设置netif为默认值,并且打开netif网口
	{
		netif_set_default(&lwip_netif); //设置netif为默认网口
		netif_set_up(&lwip_netif);		//打开netif网口
		
		/*set the link up or link down callback function - xuk*/
		netif_set_link_callback(&lwip_netif, (netif_status_callback_fn)Eth_re_link);
	}
	
#if	LWIP_DHCP
	lwip_comm_dhcp_creat();	//创建DHCP任务
#endif

	return 0;//操作OK.
}   
//如果使能了DHCP
#if LWIP_DHCP
//创建DHCP任务
void lwip_comm_dhcp_creat(void)
{
	OS_CPU_SR cpu_sr;
	OS_ENTER_CRITICAL();  //进入临界区
	OSTaskCreate(lwip_dhcp_task,(void*)0,(OS_STK*)&LWIP_DHCP_TASK_STK[LWIP_DHCP_STK_SIZE-1],LWIP_DHCP_TASK_PRIO);//创建DHCP任务 
	OS_EXIT_CRITICAL();  //退出临界区
}
//删除DHCP任务
void lwip_comm_dhcp_delete(void)
{
	dhcp_stop(&lwip_netif); 		//关闭DHCP
	OSTaskDel(LWIP_DHCP_TASK_PRIO);	//删除DHCP任务
}
//DHCP处理任务
void lwip_dhcp_task(void *pdata)
{
	u32 ip=0,netmask=0,gw=0;
	dhcp_start(&lwip_netif);//开启DHCP 
	lwipdev.dhcpstatus=0;	//正在DHCP
//	printf("正在查找DHCP服务器,请稍等...........\r\n");   
	lwipdev.dhcpstatus=1;	//正在DHCP获取中
	while(1)
	{ 
//		printf("正在获取地址...\r\n");
		ip=lwip_netif.ip_addr.addr;		//读取新IP地址
		netmask=lwip_netif.netmask.addr;//读取子网掩码
		gw=lwip_netif.gw.addr;			//读取默认网关 
		if(ip!=0)   					//当正确读取到IP地址的时候
		{
			lwipdev.dhcpstatus=2;	//DHCP成功
// 			printf("网卡en的MAC地址为:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
			//解析出通过DHCP获取到的IP地址
			lwipdev.ip[3]=(uint8_t)(ip>>24); 
			lwipdev.ip[2]=(uint8_t)(ip>>16);
			lwipdev.ip[1]=(uint8_t)(ip>>8);
			lwipdev.ip[0]=(uint8_t)(ip);
//			printf("通过DHCP获取到IP地址..............%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			//解析通过DHCP获取到的子网掩码地址
			lwipdev.netmask[3]=(uint8_t)(netmask>>24);
			lwipdev.netmask[2]=(uint8_t)(netmask>>16);
			lwipdev.netmask[1]=(uint8_t)(netmask>>8);
			lwipdev.netmask[0]=(uint8_t)(netmask);
//			printf("通过DHCP获取到子网掩码............%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			//解析出通过DHCP获取到的默认网关
			lwipdev.gateway[3]=(uint8_t)(gw>>24);
			lwipdev.gateway[2]=(uint8_t)(gw>>16);
			lwipdev.gateway[1]=(uint8_t)(gw>>8);
			lwipdev.gateway[0]=(uint8_t)(gw);
//			printf("通过DHCP获取到的默认网关..........%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			break;
		}else if(lwip_netif.dhcp->tries>LWIP_MAX_DHCP_TRIES) //通过DHCP服务获取IP地址失败,且超过最大尝试次数
		{  
			lwipdev.dhcpstatus=0XFF;//DHCP失败.
			//使用静态IP地址
			IP4_ADDR(&(lwip_netif.ip_addr),lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			IP4_ADDR(&(lwip_netif.netmask),lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			IP4_ADDR(&(lwip_netif.gw),lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
//			printf("DHCP服务超时,使用静态IP地址!\r\n");
//			printf("网卡en的MAC地址为:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
//			printf("静态IP地址........................%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
//			printf("子网掩码..........................%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
//			printf("默认网关..........................%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			break;
		}  
		delay_ms(500); //延时250ms
	}
	lwip_comm_dhcp_delete();//删除DHCP任务 
}
#endif 
//关闭LWIP,并释放内存
//退出LWIP时使用
void lwip_comm_destroy(void)
{
	u8 err;
#if LWIP_DHCP
	lwip_comm_dhcp_delete();		//dhcp任务删除 
#endif
	ETH_DeInit();  					//复位以太网
	OSTaskDel(TCPIP_THREAD_PRIO); 	//删除LWIP内核线程
 	sys_mbox_free(&mbox);  			//删除mbox消息邮箱(在tcpip.c里面定义)
	lwip_comm_delete_next_timeout();//删除定时事件链表第一个事件 
	netif_remove(&lwip_netif);  	//删除lwip_netif网卡
	//清空tcp_pcb的四个连接链表(在tcp.c文件定义) 
	tcp_ticks=0;
	tcp_bound_pcbs=NULL;
	tcp_listen_pcbs.pcbs=NULL;
	tcp_active_pcbs=NULL;
	tcp_tw_pcbs=NULL;	
	//删除网卡列表(netif.c文件中全局变量)
	netif_default=NULL; //默认网卡清零
	netif_list=NULL;  	//清除连接链表
	netif_num=0;      	//清除netif_num数量
	//清除tcp_in.c文件中全局变量
	memset(&inseg,0,sizeof(struct tcp_seg));
	tcphdr=NULL;
	iphdr=NULL;
	seqno=0;
	ackno=0;
	tcplen=0;
	recv_flags=0;	
	recv_data=NULL;
	tcp_input_pcb=NULL;
	//清除etharp.c中全局变量
	etharp_cached_entry=0;
	//清除ip.c中全局变量
	memset(current_netif,0,sizeof(struct netif));
	current_netif = NULL;
	memset((void*)current_header,0,sizeof(struct ip_hdr));
	current_header=NULL;
	memset(&current_iphdr_src, 0,sizeof(ip_addr_t));
	memset(&current_iphdr_dest,0,sizeof(ip_addr_t));
	ip_id=0;
	//清除ip_frag.c中全局变量
	memset(reassdatagrams,0,sizeof(struct ip_reassdata));
	reassdatagrams=NULL;
	ip_reass_pbufcount=0;
	//清除mem.c中全局变量
	ram=NULL;
	ram_end=NULL;
	lfree=NULL; 
	OSSemDel(mem_mutex,OS_DEL_ALWAYS,&err);//删除互斥信号量.
	lwip_comm_mem_free();	//释放内存.
 	ETH_Mem_Free();//释放内存
} 
//删除next_timeout()数据结构(变量在times.c文件)
void lwip_comm_delete_next_timeout(void)
{
#if IP_REASSEMBLY   //IP_PREASSEMBLY = 1
	sys_untimeout(ip_reass_timer,NULL); 
#endif 
#if LWIP_ARP		//LWIP_ARP = 1
	sys_untimeout(arp_timer,NULL); 
#endif  
#if LWIP_DHCP      	//LWIP_DHCP = 1
	sys_untimeout(dhcp_timer_coarse,NULL); 
	sys_untimeout(dhcp_timer_fine,NULL); 
#endif   
#if LWIP_IGMP      	//LWIP_IGMP = 1
	sys_untimeout(igmp_timer,NULL);
#endif  
#if LWIP_DNS       	//LWIP_DNS = 1
	sys_untimeout(dns_timer,NULL);
#endif  
	if(next_timeout!=NULL)next_timeout=NULL;
	tcpip_tcp_timer_active=0;
}


//void modify_lwip_netif(u32 ip, u32 netmask, u32 gw){
//	
//	lwipdev.ip[3]=(uint8_t)(ip>>24); 
//	lwipdev.ip[2]=(uint8_t)(ip>>16);
//	lwipdev.ip[1]=(uint8_t)(ip>>8);
//	lwipdev.ip[0]=(uint8_t)(ip);
//	
//	lwipdev.netmask[3]=(uint8_t)(netmask>>24);
//	lwipdev.netmask[2]=(uint8_t)(netmask>>16);
//	lwipdev.netmask[1]=(uint8_t)(netmask>>8);
//	lwipdev.netmask[0]=(uint8_t)(netmask);
//	
//	lwipdev.gateway[3]=(uint8_t)(gw>>24);
//	lwipdev.gateway[2]=(uint8_t)(gw>>16);
//	lwipdev.gateway[1]=(uint8_t)(gw>>8);
//	lwipdev.gateway[0]=(uint8_t)(gw);
//				
//	IP4_ADDR(&(lwip_netif.ip_addr),lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
//	IP4_ADDR(&(lwip_netif.netmask),lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
//	IP4_ADDR(&(lwip_netif.gw),lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
//}

//void lwip_reset_netif_ipaddr(u32 ip, u32 netmask, u32 gw){
void lwip_reset_netif_ipaddr(void){

//	IP4_ADDR(&(lwip_netif.ip_addr),lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
//	IP4_ADDR(&(lwip_netif.gw),lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
//	IP4_ADDR(&(lwip_netif.netmask),lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
//
//	netif_set_down(&lwip_netif);
//	netif_set_addr(&lwip_netif, &(lwip_netif.ip_addr), &(lwip_netif.netmask),&(lwip_netif.gw));
//	netif_set_up(&lwip_netif);

	WriteNetParasToEEProm();
	Sys_Soft_Reset();
}


/**
* 解决网线热拔插
**/
void Eth_Link_ITHandler(void)
{
//	u16 status;
//	/* Check whether the link interrupt has occurred or not */
//	if(((ETH_ReadPHYRegister(LAN8720_PHY_ADDRESS, PHY_MISR)) & PHY_LINK_STATUS) != 0){/*检测插拔中断*/
//		status = ETH_ReadPHYRegister(LAN8720_PHY_ADDRESS, PHY_BSR);
//		if(status & (PHY_AutoNego_Complete | PHY_Linked_Status)){/*检测到网线连接*/
//			if(EthInitStatus == 0){/*之前未成功初始化过*/
//				/*Reinit PHY*/
//				ETH_Reinit();
//			}
//			else{/*之前已经成功初始化*/
//				/*set link up for re link callbalk function*/
//				netif_set_link_up(&lwip_netif);
//			}
//		}
//		else{/*网线断开*/\
//			/*set link down for re link callbalk function*/
//			netif_set_link_down(&lwip_netif);
//		}
//	}
	crtLinkStatus = ETH_Get_Link_Status();
	
	if(lastLinkStatus != crtLinkStatus){
		if(crtLinkStatus){/*检测到网线连接*/
			if(EthInitStatus == 0){/*之前未成功初始化过*/
				/*Reinit PHY*/
					EthInitStatus = ETH_Reinit();
	//				delay_ms(50);
					httpd_init();
			}
			else{/*之前已经成功初始化*/
				/*set link up for re link callbalk function*/
				netif_set_link_up(&lwip_netif);
			}
		}
		else{/*网线断开*/\
			/*set link down for re link callbalk function*/
			netif_set_link_down(&lwip_netif);
		}
		lastLinkStatus = crtLinkStatus;
	}
}

/**
* @brief : process the relink of eth
* @param : netif - - specify the ETH netif
*
* @retval : none
* @author : xuk
*/
void Eth_re_link(){
	__IO uint32_t tickstart = 0;
	uint32_t regvalue = 0, tmpreg = 0;
	if(netif_is_link_up(&lwip_netif)){/*link up process*/
		if(ETH_InitStructure.ETH_AutoNegotiation == ETH_AutoNegotiation_Enable){/*AutoNegotiation_Enable*/
			/* Enable Auto-Negotiation */
			
//			LED_CLOSE = 1;
			ETH_WritePHYRegister(LAN8720_PHY_ADDRESS, PHY_BCR, PHY_AutoNegotiation);
			/* Wait until the auto-negotiation will be completed */
		do
		{
			
//			LED_OPEN = !LED_OPEN;
			tickstart++;
		} while (!(ETH_ReadPHYRegister(LAN8720_PHY_ADDRESS, PHY_BSR) & PHY_AutoNego_Complete) && (tickstart < (uint32_t)PHY_READ_TO));
		/* Return ERROR in case of timeout */
		if(tickstart == PHY_READ_TO)
		{
			// return ETH_ERROR;
		}
		/* Reset Timeout counter */
		tickstart = 0;
		/* Read the result of the auto-negotiation */
		regvalue = ETH_ReadPHYRegister(LAN8720_PHY_ADDRESS, PHY_SR);
		/* Configure the MAC with the Duplex Mode fixed by the auto-negotiation process */
		if((regvalue & PHY_DUPLEX_STATUS) != (uint32_t)RESET)
		{
			/* Set Ethernet duplex mode to Full-duplex following the auto-negotiation */
			ETH_InitStructure.ETH_Mode = ETH_Mode_FullDuplex;
		}
		else
		{
			/* Set Ethernet duplex mode to Half-duplex following the auto-negotiation */
			ETH_InitStructure.ETH_Mode = ETH_Mode_HalfDuplex;
		}
		
		/* Configure the MAC with the speed fixed by the auto-negotiation process */
		if(regvalue & PHY_SPEED_STATUS)
		{
			/* Set Ethernet speed to 10M following the auto-negotiation */
			ETH_InitStructure.ETH_Speed = ETH_Speed_10M;
		}
		else
		{
			/* Set Ethernet speed to 100M following the auto-negotiation */
			ETH_InitStructure.ETH_Speed = ETH_Speed_100M;
		}
	}
	else{/*AutoNegotiation_Disable*/
		if(!ETH_WritePHYRegister(LAN8720_PHY_ADDRESS, PHY_BCR, ((uint16_t)(ETH_InitStructure.ETH_Mode >> 3) |
		(uint16_t)(ETH_InitStructure.ETH_Speed >> 1))))
		{
			/* Return ERROR in case of write timeout */
			// return ETH_ERROR;
		}
			/* Delay to assure PHY configuration */
			// _eth_delay_(PHY_CONFIG_DELAY);
		}
		/*------------------------ ETHERNET MACCR Configuration --------------------*/
		/* Get the ETHERNET MACCR value */
		tmpreg = ETH->MACCR;
		/* Clear WD, PCE, PS, TE and RE bits */
		tmpreg &= MACCR_CLEAR_MASK;
		/* Set the WD bit according to ETH_Watchdog value */
		/* Set the JD: bit according to ETH_Jabber value */
		/* Set the IFG bit according to ETH_InterFrameGap value */
		/* Set the DCRS bit according to ETH_CarrierSense value */
		/* Set the FES bit according to ETH_Speed value */
		/* Set the DO bit according to ETH_ReceiveOwn value */
		/* Set the LM bit according to ETH_LoopbackMode value */
		/* Set the DM bit according to ETH_Mode value */
		/* Set the IPCO bit according to ETH_ChecksumOffload value */
		/* Set the DR bit according to ETH_RetryTransmission value */
		/* Set the ACS bit according to ETH_AutomaticPadCRCStrip value */
		/* Set the BL bit according to ETH_BackOffLimit value */
		/* Set the DC bit according to ETH_DeferralCheck value */
		tmpreg |= (uint32_t)(ETH_InitStructure.ETH_Watchdog |
		ETH_InitStructure.ETH_Jabber |
		ETH_InitStructure.ETH_InterFrameGap |
		ETH_InitStructure.ETH_CarrierSense |
		ETH_InitStructure.ETH_Speed |
		ETH_InitStructure.ETH_ReceiveOwn |
		ETH_InitStructure.ETH_LoopbackMode |
		ETH_InitStructure.ETH_Mode |
		ETH_InitStructure.ETH_ChecksumOffload |
		ETH_InitStructure.ETH_RetryTransmission |
		ETH_InitStructure.ETH_AutomaticPadCRCStrip |
		ETH_InitStructure.ETH_BackOffLimit |
		ETH_InitStructure.ETH_DeferralCheck);
		/* Write to ETHERNET MACCR */
		ETH->MACCR = (uint32_t)tmpreg;
		/*----------------------- ETHERNET MACFFR Configuration --------------------*/
		/* Set the RA bit according to ETH_ReceiveAll value */
		/* Set the SAF and SAIF bits according to ETH_SourceAddrFilter value */
		/* Set the PCF bit according to ETH_PassControlFrames value */
		/* Set the DBF bit according to ETH_BroadcastFramesReception value */
		/* Set the DAIF bit according to ETH_DestinationAddrFilter value */
		/* Set the PR bit according to ETH_PromiscuousMode value */
		/* Set the PM, HMC and HPF bits according to ETH_MulticastFramesFilter value */
		/* Set the HUC and HPF bits according to ETH_UnicastFramesFilter value */
		/* Write to ETHERNET MACFFR */
		ETH->MACFFR = (uint32_t)(ETH_InitStructure.ETH_ReceiveAll |
		ETH_InitStructure.ETH_SourceAddrFilter |
		ETH_InitStructure.ETH_PassControlFrames |
		ETH_InitStructure.ETH_BroadcastFramesReception |
		ETH_InitStructure.ETH_DestinationAddrFilter |
		ETH_InitStructure.ETH_PromiscuousMode |
		ETH_InitStructure.ETH_MulticastFramesFilter |
		ETH_InitStructure.ETH_UnicastFramesFilter);
		/*--------------- ETHERNET MACHTHR and MACHTLR Configuration ---------------*/
		/* Write to ETHERNET MACHTHR */
		ETH->MACHTHR = (uint32_t)ETH_InitStructure.ETH_HashTableHigh;
		/* Write to ETHERNET MACHTLR */
		ETH->MACHTLR = (uint32_t)ETH_InitStructure.ETH_HashTableLow;
		/*----------------------- ETHERNET MACFCR Configuration --------------------*/
		/* Get the ETHERNET MACFCR value */
		tmpreg = ETH->MACFCR;
		/* Clear xx bits */
		tmpreg &= MACFCR_CLEAR_MASK;
		/* Set the PT bit according to ETH_PauseTime value */
		/* Set the DZPQ bit according to ETH_ZeroQuantaPause value */
		/* Set the PLT bit according to ETH_PauseLowThreshold value */
		/* Set the UP bit according to ETH_UnicastPauseFrameDetect value */
		/* Set the RFE bit according to ETH_ReceiveFlowControl value */
		/* Set the TFE bit according to ETH_TransmitFlowControl value */
		tmpreg |= (uint32_t)((ETH_InitStructure.ETH_PauseTime << 16) |
		ETH_InitStructure.ETH_ZeroQuantaPause |
		ETH_InitStructure.ETH_PauseLowThreshold |
		ETH_InitStructure.ETH_UnicastPauseFrameDetect |
		ETH_InitStructure.ETH_ReceiveFlowControl |
		ETH_InitStructure.ETH_TransmitFlowControl);
		/* Write to ETHERNET MACFCR */
		ETH->MACFCR = (uint32_t)tmpreg;
		/*----------------------- ETHERNET MACVLANTR Configuration -----------------*/
		/* Set the ETV bit according to ETH_VLANTagComparison value */
		/* Set the VL bit according to ETH_VLANTagIdentifier value */
		ETH->MACVLANTR = (uint32_t)(ETH_InitStructure.ETH_VLANTagComparison |
		ETH_InitStructure.ETH_VLANTagIdentifier);
		/*-------------------------------- DMA Config ------------------------------*/
		/*----------------------- ETHERNET DMAOMR Configuration --------------------*/
		/* Get the ETHERNET DMAOMR value */
		tmpreg = ETH->DMAOMR;
		/* Clear xx bits */
		tmpreg &= DMAOMR_CLEAR_MASK;
		/* Set the DT bit according to ETH_DropTCPIPChecksumErrorFrame value */
		/* Set the RSF bit according to ETH_ReceiveStoreForward value */
		/* Set the DFF bit according to ETH_FlushReceivedFrame value */
		/* Set the TSF bit according to ETH_TransmitStoreForward value */
		/* Set the TTC bit according to ETH_TransmitThresholdControl value */
		/* Set the FEF bit according to ETH_ForwardErrorFrames value */
		/* Set the FUF bit according to ETH_ForwardUndersizedGoodFrames value */
		/* Set the RTC bit according to ETH_ReceiveThresholdControl value */
		/* Set the OSF bit according to ETH_SecondFrameOperate value */
		tmpreg |= (uint32_t)(ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame |
		ETH_InitStructure.ETH_ReceiveStoreForward |
		ETH_InitStructure.ETH_FlushReceivedFrame |
		ETH_InitStructure.ETH_TransmitStoreForward |
		ETH_InitStructure.ETH_TransmitThresholdControl |
		ETH_InitStructure.ETH_ForwardErrorFrames |
		ETH_InitStructure.ETH_ForwardUndersizedGoodFrames |
		ETH_InitStructure.ETH_ReceiveThresholdControl |
		ETH_InitStructure.ETH_SecondFrameOperate);
		/* Write to ETHERNET DMAOMR */
		ETH->DMAOMR = (uint32_t)tmpreg;
		/*----------------------- ETHERNET DMABMR Configuration --------------------*/
		/* Set the AAL bit according to ETH_AddressAlignedBeats value */
		/* Set the FB bit according to ETH_FixedBurst value */
		/* Set the RPBL and 4*PBL bits according to ETH_RxDMABurstLength value */
		/* Set the PBL and 4*PBL bits according to ETH_TxDMABurstLength value */
		/* Set the DSL bit according to ETH_DesciptorSkipLength value */
		/* Set the PR and DA bits according to ETH_DMAArbitration value */
		ETH->DMABMR = (uint32_t)(ETH_InitStructure.ETH_AddressAlignedBeats |
		ETH_InitStructure.ETH_FixedBurst |
		ETH_InitStructure.ETH_RxDMABurstLength | /* !! if 4xPBL is selected for Tx or Rx it is applied for the other */
		ETH_InitStructure.ETH_TxDMABurstLength |
		(ETH_InitStructure.ETH_DescriptorSkipLength << 2) |
		ETH_InitStructure.ETH_DMAArbitration |
		ETH_DMABMR_USP); /* Enable use of separate PBL for Rx and Tx */
		#ifdef USE_ENHANCED_DMA_DESCRIPTORS
		/* Enable the Enhanced DMA descriptors */
		ETH->DMABMR |= ETH_DMABMR_EDE;
		#endif /* USE_ENHANCED_DMA_DESCRIPTORS */
		/* Return Ethernet configuration success */
		// return ETH_SUCCESS;
		// ETH_Start();
	}
	else{/*link down process*/
	}
}
