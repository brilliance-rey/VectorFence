#ifndef __ALARM_H
#define __ALARM_H
#include "sys.h"
#include "includes.h" 

//1：单防区， 2：双防区
#define FIELD_COUNT	1

//告警确认时间，秒，防拆固定3s
#define ALARM_VERIFY_COUNT	2

//松弛确认时间, 单位：S
#define RELAX_VERIFIED_TIME_S	2

//标识为偏移的最大值拉力值, 拉力低于此值为偏移。单位 0.1Kg，
#define OPEN_VAL_MAX_KG		10

//满量程， 单位：0.1Kg
#define TENSION_MAX_RANGE_DEF    200

//入侵阈值上限相对差值默认值，单位：0.1Kg
#define ALARM_THRESHOLD_UP_DEF		50
//入侵阈值下限相对差值默认值， 单位：0.1Kg
#define ALARM_THRESHOLD_DOWN_DEF	50


/**
	开关量
	PE0（PIN_97）	O	RL_B	B防区告警开关量
	PE1（PIN_98）	O	RL_A	A防区告警开关量
*/
#define RL_A    PEout(1)
//#define RL_B    PEout(0)

/**
	防拆检测
	PA10（PIN_69）	I	FANG_CHAI	防拆检测    低电平机箱盖打开
 */
#define FANG_CHAI   PAin(10)


#define B_RLX_MASK		9
#define A_RLX_MASK		8

#define SW2_MASK		6
#define SW1_MASK		5
#define AB_FCH_MASK 	4
#define B_OPN_MASK  	3
#define B_INVD_MASK  	2
#define A_OPN_MASK  	1
#define A_INVD_MASK  	0

#define NO_ALARM 	0
#define ALARM 		1
//#define OPEN_ALARM 	2
//#define INVADE_ALARM 3

#define Open 1
#define Close 2

#define OFF 0
#define ON 1

#define ALARM_DURATION_DEF_S 30	//告警响铃持续时间。

#define SET_ALARM_STATE(STATE, STATE_MASK) ((STATE) |= (1<<STATE_MASK))
#define CLEAR_ALARM_STATE(STATE, STATE_MASK) ((STATE) &= ~(1<<STATE_MASK))

#define GET_ALARM_STATE(STATE, STATE_MASK) ((STATE >> STATE_MASK) & 0x01)

extern u8 field_num[2];  	//A，B区对应的防区号，取值：1-80， 0:无设置
extern u16 alarm_delay[2];	//A,B区警号延时，默认30秒, 0 ~ 999

//extern u8 ten_val_range_a[2];//A防区拉力值范围ten_val_range_a[0]为下限，ten_val_range_a[1]为上限
//extern u8 ten_val_range_b[2];//B防区拉力值范围ten_val_range_b[0]为下限，ten_val_range_b[1]为上限

//A,B防区拉力值范围，如：ten_val_range[0][0]为a防区下限，ten_val_range[0][0]为a防区上限, 单位：0.1Kg 如：20->2Kg
extern u16 ten_val_range[2][2];

//1:最灵敏（0.5s）10:最不灵敏（5s)
extern u8 alarm_sensitivity[2];

/**
* ALARM_STATE:
* bit  15     14      13      12     11      10      09      08
*      --     --      --     --      --      --    B_RLX    A_RLX
*
* bit  07      06      05      04      03      02      01      00
*      --      SW2     SW1     AB_FCH  B_OPN   B_INVD  A_OPN   A_INVD
*/
extern volatile u16 ALARM_STATE;

/** 延时告警状态，与警号标志同步，通过告警清除Remove_Alarm_RL_A(B)，进行清除。*/
extern volatile u16 ALARM_STATE_DELAY;

//extern u8 ALARM_STATE_TEMP;
extern u16 cur_Alarm_Type_Sum;
extern u16 old_Alarm_Type_Sum;
extern u8 alarm_type_cnt;//告警类型数量

extern u8 flag_fcgjsn;//防拆告警使能标识
extern u8 flag_fcgjsn_tmp;//防拆告警使能标识暂存

//入侵阈值上限相对差值，如：203 -> 20.3Kg
extern u16 alarm_threshold_up_dif;
//松弛阈值下限相对差值, 如：203 -> 20.3Kg
extern u16 alarm_threshold_down_dif;

extern u8 FCAlmVerifyCnt;

extern u8 adcIvdAlmVerifyCnt[2];
extern u8 adcOpnAlmVerifyCnt[2];
extern u8 adcRlxAlmVerifyCnt[2];

//A区, B区警号延时计数器
extern s32 alarmingCnt[2];

void FangChai_Start(void);
void FangChai_Stop(void);
	
void ALARM_Init(void);	//IO初始化
void Alarm_RL_A(void);
void Remove_Alarm_RL_A(void);
void Alarm_RL_B(void);
void Remove_Alarm_RL_B(void);

u8 FANG_CHAI_Check(void);
void ALARM_Update(void);

//extern OS_EVENT * msg_AHV;			//AHV邮箱事件块指针
/**
	4、6线检测	PE3（PIN_2）	I	W4/W6	0:W4；1:W6
	开关量(红外对射)接入端口1	PA11（PIN_70）	I	SW1	要求常闭输入		0—	正常		1—	报警
	开关量(红外对射)接入端口2	PA12（PIN_71）	I	SW2	要求常闭输入		0—	正常		1—	报警
*/

#define PUSHA 	PAout(9)
#define PUSHB 	PCout(8)
//#define PWM	    PAout(11)

//#define W4_W6			PEin(3)
#define FIELD_FLAG_S0D1	PEin(4)

#define IS_W4	0
#define IS_W6	1

#define SW1 	PAin(11)
#define SW2		PAin(12)

#define W4_STATE_MASK	0x03
#define W6_STATE_MASK	0x0f

extern u8 W4_W6;

extern volatile u8 BU_FANG_A_FLAG;
extern volatile u8 BU_FANG_B_FLAG;

extern u8 BU_FANG_A_FLAG_TEMP;//A防区布防开关暂存
extern u8 BU_FANG_B_FLAG_TEMP;//B防区布防开关暂存

void Push_AB_Start(void);
void Push_AB_Stop(void);

void Push_A_Start(void);
//void Push_A_Run(void);
void Push_A_Stop(void);
void Push_B_Start(void);
//void Push_B_Run(void);
void Push_B_Stop(void);

/**
 * ADC_Check_Handler之前多次检测。
 * axix_index: 0:X轴， 1:Y轴
 */
void ADC_Check(u8 axix_index);

void ADC_Check_Handler(u8 axix_index);

/**SW告警标记：0：OK, 1:Alarm*/
extern u8 SW_Alarm[2];

/** ["禁用", "主动红外探测器","被动红外探测器","双鉴探测器","环境探测器","门磁","紧急按钮","水浸","其他设备"]; */
extern s8 SW_Type[2];

/** 远程关机状态 0：开机，1：关机 **/
extern u8 remote_shutdown;

void EXTIX_Init(void);	//外部中断初始化

/**
 * 设置开关量类型：
 * bit15-8:开关量2，   bit7-0: 开关量1
 */
void setSWValue(u16 swVal);

/**
 * 是否存在此防区号
 */
u8 existFieldNum(u8 fieldNum);

/**
 * 设置防区号
 * i: 0:A区，1：B区
 * fieldNum: 防区号：1-80
 */
void setFieldNum(u8 i, u8 fieldNum);

/**
 * 设置警号延时，单位：秒  范围：0-999
 * i: 0:A区，1：B区
 * almDly：警号延时
 */
void setAlarmDelay(u8 i, u16 almDly);

/**
 * 设置告警灵敏度
 * axix_index: 防区序号： 0：A防区， 1：B防区
 * sensitivity 灵敏度，1-10： 0.5-5s
 */
void setAlarmSensitivity(u8 axix_index, u8 sensitivity);

/**
 * 设置拉力监控范围有效值。
 * axix_index: 防区序号： 0：A防区， 1：B防区
 * highLowFlag: 0：下限值， 1：上限值
 */
void setTenValRange(u8 axix_index, u8 highLowFlag, u16 tenVal);

/**
 * 修改防区号
 * oldfieldNum：旧防区号
 * newfieldNum：新防区号
 */
void modifyFieldNum(u8 oldfieldNum, u8 newfieldNum);

/**
 * 修改警号延时
 * fieldNum：防区号
 * alarmDelay：新防区号
 */
void modifyAlarmDelay(u8 fieldNum, u16 alarmDelay);

/**
 * 根据防区号获取警号延时
 * filedNum:防区号
 */
u16 getAlarmDelay(u8 fieldNum);

/**
 * 修改布防开关标志
 * fieldNum：防区号
 * bfFlag：新布防开关
 */
void modifyBFFlag(u8 fieldNum, u16 bfFlag);

/**
 * 根据防区号获取布防开关标志
 * filedNum:防区号
 * 返回：布防开关标志
 */
u8 getBFFlag(u8 fieldNum);

/**
 * 根据防区号获取告警状态 ：//bit7-0:--,SW2,SW1,AB_FCH,--,RLX,OPN,IVD
 * filedNum:防区号
 * 返回：告警状态 ：bit7-0:--,SW2,SW1,AB_FCH,--,RLX,OPN,IVD
 */
u8 getAlarmStatus(u8 fieldNum);


/**
 * 根据防区号获取告警状态, for key, 延时清除，和警号同步。
 */
u8 getAlarmDelayedStatus(u8 fieldNum);

/**
 * 根据防区号清除告警响铃
 * filedNum:防区号
 */
void removeAlarmRL(u8 fieldNum);

/**
 * 根据同伴防区号，即，如果fieldNum是A区，返回B区防区号，如果fieldNum是B区，返回A区防区号，
 * filedNum:防区号
 * 返回：
 */
u8 getPeerFieldNum(u8 fieldNum);

/**
 * 获取防区号对应的序号
 * filedNum:防区号
 * 返回：（0：A防区号，1：B防区号）
 */
u8 getFieldIndex(u8 fieldNum);


/**
 * 远程开关机
 * remote_shutdwon 0:开机， 1：关机
 */
void remoteShutdown(u8 shutdownFlag);

/**
 * 设置入侵阈值上限相对差值
 */
void setAlmThrdUpDif(u16 value);

/**
 * 设置入侵阈值上限相对差值
 */
void setAlmThrdDwDif(u16 value);

/**
 * 根据最大量程更新阈值和拉力范围参数
 */
void updateTensionParasByMaxValue(void);

/**
 *
 *获取 防区A参数#防区B参数{防区A,B参数格式 :field_num+alarm_delay+alarm_sensitivity+ten_val_range[0]+ten_val_range[1]}
 */
void getWebFieldParas(char *fieldParas);

#endif
