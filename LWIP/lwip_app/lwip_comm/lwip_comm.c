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
//�޸���Ϣ
//��
////////////////////////////////////////////////////////////////////////////////// 	   
   
__lwip_dev lwipdev;		//lwip���ƽṹ��
struct netif lwip_netif;				//����һ��ȫ�ֵ�����ӿ�
 
extern sys_mbox_t mbox;  				//��Ϣ���� ȫ�ֱ���(��tcpip.c���涨��)
extern u32 memp_get_memorysize(void);	//��memp.c���涨��
extern u8_t *memp_memory;				//��memp.c���涨��.
extern u8_t *ram_heap;					//��mem.c���涨��.
extern sys_mbox_t mbox;  				//��Ϣ���� ȫ�ֱ���(��tcpip.c���涨��)

extern u8_t netif_num;                  //netif.c�ļ�
extern struct tcp_seg inseg;            //tcp_in.c�ļ�
extern struct tcp_hdr *tcphdr;          //tcp_in.c�ļ�
extern struct ip_hdr *iphdr;			//tcp_in.c�ļ�
extern u32_t seqno, ackno;				//tcp_in.c�ļ�
extern u16_t tcplen;					//tcp_in.c�ļ�
extern u8_t recv_flags;					//tcp_in.c�ļ�
extern struct pbuf *recv_data;			//tcp_in.c�ļ�
extern u8_t etharp_cached_entry;        //etharp.c�ļ�
extern u16_t ip_id;                     //ip.c�ļ�
extern struct ip_reassdata *reassdatagrams; //ip_frag.c�ļ�
extern u16_t ip_reass_pbufcount;        //ip_frag.c�ļ�
extern u8_t *ram;						//mem.c�ļ�
extern struct mem *ram_end;				//mem.c�ļ�
extern struct mem *lfree;				//mem.c�ļ�
extern sys_mutex_t mem_mutex;			//mem.c�ļ�

extern void ip_reass_timer(void *arg);  //timers.c�ļ�
extern void arp_timer(void *arg);		//timers.c�ļ�
extern void dhcp_timer_coarse(void *arg);//timers.c�ļ�
extern void dhcp_timer_fine(void *arg); //timers.c�ļ�
extern struct sys_timeo *next_timeout;  //timers.c�ļ�
extern int tcpip_tcp_timer_active;      //timers.c�ļ�
/////////////////////////////////////////////////////////////////////////////////

u8 EthInitStatus = 0;
u8 lastLinkStatus = 0;
u8 crtLinkStatus = 0;

//MAC��ַ�����ֽڣ� �̶�Ϊ:55.39.72(ERROR,huawei������������),�����ֽ���STM32ΨһID)
//������: 44:54:A5:XX:XX:XX/44:54:54:XX:XX:XX/44:54:56:XX:XX:XX/78:45:C4:XX:XX:XX
u8 mac_def[3] = {0x44, 0x54, 0xA5};

//lwip����������(�ں������DHCP����)

//lwip�ں������ջ(���ȼ��Ͷ�ջ��С��lwipopts.h������) 
OS_STK * TCPIP_THREAD_TASK_STK;	 

//lwip DHCP����
//�����������ȼ�
#define LWIP_DHCP_TASK_PRIO       		8
//���������ջ��С
#define LWIP_DHCP_STK_SIZE  		    128
//�����ջ�������ڴ����ķ�ʽ��������	
OS_STK * LWIP_DHCP_TASK_STK;	
//������
void lwip_dhcp_task(void *pdata); 


//������̫���жϵ���
void lwip_packet_handler(void)
{
	ethernetif_input(&lwip_netif);
}
//lwip�ں˲���,�ڴ�����
//����ֵ:0,�ɹ�;
//    ����,ʧ��
u8 lwip_comm_mem_malloc(void)
{
	u32 mempsize;
	u32 ramheapsize; 
	mempsize=memp_get_memorysize();			//�õ�memp_memory�����С
	memp_memory=mymalloc(SRAMIN,mempsize);	//Ϊmemp_memory�����ڴ�
	ramheapsize=LWIP_MEM_ALIGN_SIZE(MEM_SIZE)+2*LWIP_MEM_ALIGN_SIZE(4*3)+MEM_ALIGNMENT;//�õ�ram heap��С
	ram_heap=mymalloc(SRAMIN,ramheapsize);	//Ϊram_heap�����ڴ� 
	TCPIP_THREAD_TASK_STK=mymalloc(SRAMIN,TCPIP_THREAD_STACKSIZE*4);//���ں����������ջ 
	LWIP_DHCP_TASK_STK=mymalloc(SRAMIN,LWIP_DHCP_STK_SIZE*4);		//��dhcp�����ջ�����ڴ�ռ�
	if(!memp_memory||!ram_heap||!TCPIP_THREAD_TASK_STK||!LWIP_DHCP_TASK_STK)//������ʧ�ܵ�
	{
		lwip_comm_mem_free();
		return 1;
	}
	return 0;	
}
//lwip�ں˲���,�ڴ��ͷ�
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
	
	//MAC�Զ�������ԭ55:39:72��huawei�������Ƿ���
	if(lwipdev.mac[0] == 0x55 && lwipdev.mac[1] == 0x39 && lwipdev.mac[2] == 0x72){
		lwipdev.mac[0]=mac_def[0];
		lwipdev.mac[1]=mac_def[1];
		lwipdev.mac[2]=mac_def[2];
		WriteNetParasToEEProm();
	}
	
	//�����������0xFF�����ǳ���״̬��default�ˡ�
	if(lwipdev.ip[0] == 0xFF && lwipdev.ip[1] == 0xFF && lwipdev.ip[2] == 0xFF && lwipdev.ip[3] == 0xFF){
		lwip_comm_default_ip_set(&lwipdev);
	}
}

//lwip Ĭ��IP����
//lwipx:lwip���ƽṹ��ָ��
void lwip_comm_default_ip_set(__lwip_dev *lwipx)
{
	u32 sn0 = *(vu32*)(0x1FFF7A10);//��ȡSTM32��ΨһID��ǰ24λ��ΪMAC��ַ�����ֽ�
	//Ĭ��Զ��IPΪ:192.168.1.90
	lwipx->remoteip[0]=192;	
	lwipx->remoteip[1]=168;
	lwipx->remoteip[2]=0;
	lwipx->remoteip[3]=90;
	//MAC��ַ����(�����ֽ�(IEEE��֮Ϊ��֯ΨһID,OUI)�̶������ָ��������ֽ���STM32ΨһID)
	if(lwipdev.mac[0] == 0xFF && lwipdev.mac[1] == 0xFF && lwipdev.mac[2] == 0xFF){
		lwipx->mac[0]=mac_def[0];
		lwipx->mac[1]=mac_def[1];
		lwipx->mac[2]=mac_def[2];
	}
	lwipx->mac[3]=(sn0>>16)&0XFF;
	lwipx->mac[4]=(sn0>>8)&0XFFF;
	lwipx->mac[5]=sn0&0XFF;
	//Ĭ�ϱ���IPΪ:192.168.0.30
	lwipx->ip[0]=192;
	lwipx->ip[1]=168;
	lwipx->ip[2]=0;
	lwipx->ip[3]=30;
	//Ĭ����������:255.255.255.0
	lwipx->netmask[0]=255;	
	lwipx->netmask[1]=255;
	lwipx->netmask[2]=255;
	lwipx->netmask[3]=0;
	//Ĭ������:192.168.0.1
	lwipx->gateway[0]=192;
	lwipx->gateway[1]=168;
	lwipx->gateway[2]=0;
	lwipx->gateway[3]=1;
	lwipx->dhcpstatus=0;//û��DHCP	
	
	WriteNetParasToEEProm();
} 


//LWIP��ʼ��(LWIP������ʱ��ʹ��)
//����ֵ:0,�ɹ�
//      1,�ڴ����
//      2,LAN8720��ʼ��ʧ��
//      3,�������ʧ��.
u8 lwip_comm_init(void)
{
	OS_CPU_SR cpu_sr;
	struct netif *Netif_Init_Flag;		//����netif_add()����ʱ�ķ���ֵ,�����ж������ʼ���Ƿ�ɹ�
	struct ip_addr ipaddr;  			//ip��ַ
	struct ip_addr netmask; 			//��������
	struct ip_addr gw;      			//Ĭ������ 
	if(ETH_Mem_Malloc())return 1;		//�ڴ�����ʧ��
	if(lwip_comm_mem_malloc())return 1;	//�ڴ�����ʧ��
	//if(LAN8720_Init())return 2;			//��ʼ��LAN8720ʧ�� 
	
	LAN8720_Init();
	tcpip_init(NULL,NULL);				//��ʼ��tcp ip�ں�,�ú�������ᴴ��tcpip_thread�ں�����
//	lwip_init();						//��ʼ��LWIP�ں�

//	lwip_comm_default_ip_set(&lwipdev);  	//��������Ĭ��IP����Ϣ
	lwip_comm_ip_init();	//init IP����Ϣ
//	ReadNetParasFromEEProm();
#if LWIP_DHCP		//ʹ�ö�̬IP
	ipaddr.addr = 0;
	netmask.addr = 0;
	gw.addr = 0;
#else				//ʹ�þ�̬IP
	IP4_ADDR(&ipaddr,lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
	IP4_ADDR(&netmask,lwipdev.netmask[0],lwipdev.netmask[1] ,lwipdev.netmask[2],lwipdev.netmask[3]);
	IP4_ADDR(&gw,lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
	//printf("����en��MAC��ַΪ:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
	//printf("��̬IP��ַ........................%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
	//printf("��������..........................%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
	//printf("Ĭ������..........................%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
#endif
	OS_ENTER_CRITICAL();  //�����ٽ���
	Netif_Init_Flag=netif_add(&lwip_netif,&ipaddr,&netmask,&gw,NULL,&ethernetif_init,&tcpip_input);//�������б������һ������
	
	OS_EXIT_CRITICAL();  //�˳��ٽ���
	if(Netif_Init_Flag==NULL)return 3;//�������ʧ�� 
	else//������ӳɹ���,����netifΪĬ��ֵ,���Ҵ�netif����
	{
		netif_set_default(&lwip_netif); //����netifΪĬ������
		netif_set_up(&lwip_netif);		//��netif����
		
		/*set the link up or link down callback function - xuk*/
		netif_set_link_callback(&lwip_netif, (netif_status_callback_fn)Eth_re_link);
	}
	
#if	LWIP_DHCP
	lwip_comm_dhcp_creat();	//����DHCP����
#endif

	return 0;//����OK.
}   
//���ʹ����DHCP
#if LWIP_DHCP
//����DHCP����
void lwip_comm_dhcp_creat(void)
{
	OS_CPU_SR cpu_sr;
	OS_ENTER_CRITICAL();  //�����ٽ���
	OSTaskCreate(lwip_dhcp_task,(void*)0,(OS_STK*)&LWIP_DHCP_TASK_STK[LWIP_DHCP_STK_SIZE-1],LWIP_DHCP_TASK_PRIO);//����DHCP���� 
	OS_EXIT_CRITICAL();  //�˳��ٽ���
}
//ɾ��DHCP����
void lwip_comm_dhcp_delete(void)
{
	dhcp_stop(&lwip_netif); 		//�ر�DHCP
	OSTaskDel(LWIP_DHCP_TASK_PRIO);	//ɾ��DHCP����
}
//DHCP��������
void lwip_dhcp_task(void *pdata)
{
	u32 ip=0,netmask=0,gw=0;
	dhcp_start(&lwip_netif);//����DHCP 
	lwipdev.dhcpstatus=0;	//����DHCP
//	printf("���ڲ���DHCP������,���Ե�...........\r\n");   
	lwipdev.dhcpstatus=1;	//����DHCP��ȡ��
	while(1)
	{ 
//		printf("���ڻ�ȡ��ַ...\r\n");
		ip=lwip_netif.ip_addr.addr;		//��ȡ��IP��ַ
		netmask=lwip_netif.netmask.addr;//��ȡ��������
		gw=lwip_netif.gw.addr;			//��ȡĬ������ 
		if(ip!=0)   					//����ȷ��ȡ��IP��ַ��ʱ��
		{
			lwipdev.dhcpstatus=2;	//DHCP�ɹ�
// 			printf("����en��MAC��ַΪ:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
			//������ͨ��DHCP��ȡ����IP��ַ
			lwipdev.ip[3]=(uint8_t)(ip>>24); 
			lwipdev.ip[2]=(uint8_t)(ip>>16);
			lwipdev.ip[1]=(uint8_t)(ip>>8);
			lwipdev.ip[0]=(uint8_t)(ip);
//			printf("ͨ��DHCP��ȡ��IP��ַ..............%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			//����ͨ��DHCP��ȡ�������������ַ
			lwipdev.netmask[3]=(uint8_t)(netmask>>24);
			lwipdev.netmask[2]=(uint8_t)(netmask>>16);
			lwipdev.netmask[1]=(uint8_t)(netmask>>8);
			lwipdev.netmask[0]=(uint8_t)(netmask);
//			printf("ͨ��DHCP��ȡ����������............%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			//������ͨ��DHCP��ȡ����Ĭ������
			lwipdev.gateway[3]=(uint8_t)(gw>>24);
			lwipdev.gateway[2]=(uint8_t)(gw>>16);
			lwipdev.gateway[1]=(uint8_t)(gw>>8);
			lwipdev.gateway[0]=(uint8_t)(gw);
//			printf("ͨ��DHCP��ȡ����Ĭ������..........%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			break;
		}else if(lwip_netif.dhcp->tries>LWIP_MAX_DHCP_TRIES) //ͨ��DHCP�����ȡIP��ַʧ��,�ҳ�������Դ���
		{  
			lwipdev.dhcpstatus=0XFF;//DHCPʧ��.
			//ʹ�þ�̬IP��ַ
			IP4_ADDR(&(lwip_netif.ip_addr),lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			IP4_ADDR(&(lwip_netif.netmask),lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			IP4_ADDR(&(lwip_netif.gw),lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
//			printf("DHCP����ʱ,ʹ�þ�̬IP��ַ!\r\n");
//			printf("����en��MAC��ַΪ:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
//			printf("��̬IP��ַ........................%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
//			printf("��������..........................%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
//			printf("Ĭ������..........................%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			break;
		}  
		delay_ms(500); //��ʱ250ms
	}
	lwip_comm_dhcp_delete();//ɾ��DHCP���� 
}
#endif 
//�ر�LWIP,���ͷ��ڴ�
//�˳�LWIPʱʹ��
void lwip_comm_destroy(void)
{
	u8 err;
#if LWIP_DHCP
	lwip_comm_dhcp_delete();		//dhcp����ɾ�� 
#endif
	ETH_DeInit();  					//��λ��̫��
	OSTaskDel(TCPIP_THREAD_PRIO); 	//ɾ��LWIP�ں��߳�
 	sys_mbox_free(&mbox);  			//ɾ��mbox��Ϣ����(��tcpip.c���涨��)
	lwip_comm_delete_next_timeout();//ɾ����ʱ�¼������һ���¼� 
	netif_remove(&lwip_netif);  	//ɾ��lwip_netif����
	//���tcp_pcb���ĸ���������(��tcp.c�ļ�����) 
	tcp_ticks=0;
	tcp_bound_pcbs=NULL;
	tcp_listen_pcbs.pcbs=NULL;
	tcp_active_pcbs=NULL;
	tcp_tw_pcbs=NULL;	
	//ɾ�������б�(netif.c�ļ���ȫ�ֱ���)
	netif_default=NULL; //Ĭ����������
	netif_list=NULL;  	//�����������
	netif_num=0;      	//���netif_num����
	//���tcp_in.c�ļ���ȫ�ֱ���
	memset(&inseg,0,sizeof(struct tcp_seg));
	tcphdr=NULL;
	iphdr=NULL;
	seqno=0;
	ackno=0;
	tcplen=0;
	recv_flags=0;	
	recv_data=NULL;
	tcp_input_pcb=NULL;
	//���etharp.c��ȫ�ֱ���
	etharp_cached_entry=0;
	//���ip.c��ȫ�ֱ���
	memset(current_netif,0,sizeof(struct netif));
	current_netif = NULL;
	memset((void*)current_header,0,sizeof(struct ip_hdr));
	current_header=NULL;
	memset(&current_iphdr_src, 0,sizeof(ip_addr_t));
	memset(&current_iphdr_dest,0,sizeof(ip_addr_t));
	ip_id=0;
	//���ip_frag.c��ȫ�ֱ���
	memset(reassdatagrams,0,sizeof(struct ip_reassdata));
	reassdatagrams=NULL;
	ip_reass_pbufcount=0;
	//���mem.c��ȫ�ֱ���
	ram=NULL;
	ram_end=NULL;
	lfree=NULL; 
	OSSemDel(mem_mutex,OS_DEL_ALWAYS,&err);//ɾ�������ź���.
	lwip_comm_mem_free();	//�ͷ��ڴ�.
 	ETH_Mem_Free();//�ͷ��ڴ�
} 
//ɾ��next_timeout()���ݽṹ(������times.c�ļ�)
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
* ��������Ȱβ�
**/
void Eth_Link_ITHandler(void)
{
//	u16 status;
//	/* Check whether the link interrupt has occurred or not */
//	if(((ETH_ReadPHYRegister(LAN8720_PHY_ADDRESS, PHY_MISR)) & PHY_LINK_STATUS) != 0){/*������ж�*/
//		status = ETH_ReadPHYRegister(LAN8720_PHY_ADDRESS, PHY_BSR);
//		if(status & (PHY_AutoNego_Complete | PHY_Linked_Status)){/*��⵽��������*/
//			if(EthInitStatus == 0){/*֮ǰδ�ɹ���ʼ����*/
//				/*Reinit PHY*/
//				ETH_Reinit();
//			}
//			else{/*֮ǰ�Ѿ��ɹ���ʼ��*/
//				/*set link up for re link callbalk function*/
//				netif_set_link_up(&lwip_netif);
//			}
//		}
//		else{/*���߶Ͽ�*/\
//			/*set link down for re link callbalk function*/
//			netif_set_link_down(&lwip_netif);
//		}
//	}
	crtLinkStatus = ETH_Get_Link_Status();
	
	if(lastLinkStatus != crtLinkStatus){
		if(crtLinkStatus){/*��⵽��������*/
			if(EthInitStatus == 0){/*֮ǰδ�ɹ���ʼ����*/
				/*Reinit PHY*/
					EthInitStatus = ETH_Reinit();
	//				delay_ms(50);
					httpd_init();
			}
			else{/*֮ǰ�Ѿ��ɹ���ʼ��*/
				/*set link up for re link callbalk function*/
				netif_set_link_up(&lwip_netif);
			}
		}
		else{/*���߶Ͽ�*/\
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
