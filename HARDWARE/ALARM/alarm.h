#ifndef __ALARM_H
#define __ALARM_H
#include "sys.h"
#include "includes.h" 

//�澯ȷ��ʱ�䣬�룬����̶�3s
#define ALARM_VERIFY_COUNT	2

//�ɳ�ȷ��ʱ��, ��λ��S
#define RELAX_VERIFIED_TIME_S	2

//��ʶΪ���ߵ����ֵ����ֵ, �������ڴ�ֵΪ���ߡ���λ 0.1Kg��
#define OPEN_VAL_MAX_KG		10

//�����̣� ��λ��0.1Kg
#define TENSION_MAX_RANGE_DEF    200

//������ֵ������Բ�ֵĬ��ֵ����λ��0.1Kg
#define ALARM_THRESHOLD_UP_DEF		50
//������ֵ������Բ�ֵĬ��ֵ�� ��λ��0.1Kg
#define ALARM_THRESHOLD_DOWN_DEF	50


/**
	������
	PE0��PIN_97��	O	RL_B	B�����澯������
	PE1��PIN_98��	O	RL_A	A�����澯������
*/
#define RL_A    PEout(1)
#define RL_B    PEout(0)

/**
	������
	PA10��PIN_69��	I	FANG_CHAI	������    �͵�ƽ����Ǵ�
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

#define ALARM_DURATION_DEF_S 30	//�澯�������ʱ�䡣

#define SET_ALARM_STATE(STATE, STATE_MASK) ((STATE) |= (1<<STATE_MASK))
#define CLEAR_ALARM_STATE(STATE, STATE_MASK) ((STATE) &= ~(1<<STATE_MASK))

#define GET_ALARM_STATE(STATE, STATE_MASK) ((STATE >> STATE_MASK) & 0x01)

extern u8 field_num[2];  	//A��B����Ӧ�ķ����ţ�ȡֵ��1-80�� 0:������
extern u16 alarm_delay[2];	//A,B��������ʱ��Ĭ��30��, 0 ~ 999

//extern u8 ten_val_range_a[2];//A��������ֵ��Χten_val_range_a[0]Ϊ���ޣ�ten_val_range_a[1]Ϊ����
//extern u8 ten_val_range_b[2];//B��������ֵ��Χten_val_range_b[0]Ϊ���ޣ�ten_val_range_b[1]Ϊ����

//A,B��������ֵ��Χ���磺ten_val_range[0][0]Ϊa�������ޣ�ten_val_range[0][0]Ϊa��������, ��λ��0.1Kg �磺20->2Kg
extern u16 ten_val_range[2][2];

//1:��������0.5s��10:�������5s)
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

/** ��ʱ�澯״̬���뾯�ű�־ͬ����ͨ���澯���Remove_Alarm_RL_A(B)�����������*/
extern volatile u16 ALARM_STATE_DELAY;

//extern u8 ALARM_STATE_TEMP;
extern u16 cur_Alarm_Type_Sum;
extern u16 old_Alarm_Type_Sum;
extern u8 alarm_type_cnt;//�澯��������

extern u8 flag_fcgjsn;//����澯ʹ�ܱ�ʶ
extern u8 flag_fcgjsn_tmp;//����澯ʹ�ܱ�ʶ�ݴ�

//������ֵ������Բ�ֵ���磺203 -> 20.3Kg
extern u16 alarm_threshold_up_dif;
//�ɳ���ֵ������Բ�ֵ, �磺203 -> 20.3Kg
extern u16 alarm_threshold_down_dif;

extern u8 FCAlmVerifyCnt;

extern u8 adcIvdAlmVerifyCnt[2];
extern u8 adcOpnAlmVerifyCnt[2];
extern u8 adcRlxAlmVerifyCnt[2];

//A��, B��������ʱ������
extern s32 alarmingCnt[2];

void FangChai_Start(void);
void FangChai_Stop(void);
	
void ALARM_Init(void);	//IO��ʼ��
void Alarm_RL_A(void);
void Remove_Alarm_RL_A(void);
void Alarm_RL_B(void);
void Remove_Alarm_RL_B(void);

u8 FANG_CHAI_Check(void);
void ALARM_Update(void);

//extern OS_EVENT * msg_AHV;			//AHV�����¼���ָ��
/**
	4��6�߼��	PE3��PIN_2��	I	W4/W6	0:W4��1:W6
	������(�������)����˿�1	PA11��PIN_70��	I	SW1	Ҫ�󳣱�����		0��	����		1��	����
	������(�������)����˿�2	PA12��PIN_71��	I	SW2	Ҫ�󳣱�����		0��	����		1��	����
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

extern u8 BU_FANG_A_FLAG_TEMP;//A�������������ݴ�
extern u8 BU_FANG_B_FLAG_TEMP;//B�������������ݴ�

void Push_AB_Start(void);
void Push_AB_Stop(void);

void Push_A_Start(void);
//void Push_A_Run(void);
void Push_A_Stop(void);
void Push_B_Start(void);
//void Push_B_Run(void);
void Push_B_Stop(void);

/**
 * ADC_Check_Handler֮ǰ��μ�⡣
 * field_index: 0:A������ 1:B����
 */
void ADC_Check(u8 field_index);

void ADC_Check_Handler(u8 field_index);

/**SW�澯��ǣ�0��OK, 1:Alarm*/
extern u8 SW_Alarm[2];

/** ["����", "��������̽����","��������̽����","˫��̽����","����̽����","�Ŵ�","������ť","ˮ��","�����豸"]; */
extern s8 SW_Type[2];

/** Զ�̹ػ�״̬ 0��������1���ػ� **/
extern u8 remote_shutdown;

void EXTIX_Init(void);	//�ⲿ�жϳ�ʼ��

/**
 * ���ÿ��������ͣ�
 * bit15-8:������2��   bit7-0: ������1
 */
void setSWValue(u16 swVal);

/**
 * �Ƿ���ڴ˷�����
 */
u8 existFieldNum(u8 fieldNum);

/**
 * ���÷�����
 * i: 0:A����1��B��
 * fieldNum: �����ţ�1-80
 */
void setFieldNum(u8 i, u8 fieldNum);

/**
 * ���þ�����ʱ����λ����  ��Χ��0-999
 * i: 0:A����1��B��
 * almDly��������ʱ
 */
void setAlarmDelay(u8 i, u16 almDly);

/**
 * ���ø澯������
 * field_index: ������ţ� 0��A������ 1��B����
 * sensitivity �����ȣ�1-10�� 0.5-5s
 */
void setAlarmSensitivity(u8 field_index, u8 sensitivity);

/**
 * ����������ط�Χ��Чֵ��
 * field_index: ������ţ� 0��A������ 1��B����
 * highLowFlag: 0������ֵ�� 1������ֵ
 */
void setTenValRange(u8 field_index, u8 highLowFlag, u16 tenVal);

/**
 * �޸ķ�����
 * oldfieldNum���ɷ�����
 * newfieldNum���·�����
 */
void modifyFieldNum(u8 oldfieldNum, u8 newfieldNum);

/**
 * �޸ľ�����ʱ
 * fieldNum��������
 * alarmDelay���·�����
 */
void modifyAlarmDelay(u8 fieldNum, u16 alarmDelay);

/**
 * ���ݷ����Ż�ȡ������ʱ
 * filedNum:������
 */
u16 getAlarmDelay(u8 fieldNum);

/**
 * �޸Ĳ������ر�־
 * fieldNum��������
 * bfFlag���²�������
 */
void modifyBFFlag(u8 fieldNum, u16 bfFlag);

/**
 * ���ݷ����Ż�ȡ�������ر�־
 * filedNum:������
 * ���أ��������ر�־
 */
u8 getBFFlag(u8 fieldNum);

/**
 * ���ݷ����Ż�ȡ�澯״̬ ��//bit7-0:--,SW2,SW1,AB_FCH,--,RLX,OPN,IVD
 * filedNum:������
 * ���أ��澯״̬ ��bit7-0:--,SW2,SW1,AB_FCH,--,RLX,OPN,IVD
 */
u8 getAlarmStatus(u8 fieldNum);


/**
 * ���ݷ����Ż�ȡ�澯״̬, for key, ��ʱ������;���ͬ����
 */
u8 getAlarmDelayedStatus(u8 fieldNum);

/**
 * ���ݷ���������澯����
 * filedNum:������
 */
void removeAlarmRL(u8 fieldNum);

/**
 * ����ͬ������ţ��������fieldNum��A��������B�������ţ����fieldNum��B��������A�������ţ�
 * filedNum:������
 * ���أ�
 */
u8 getPeerFieldNum(u8 fieldNum);

/**
 * ��ȡ�����Ŷ�Ӧ�����
 * filedNum:������
 * ���أ���0��A�����ţ�1��B�����ţ�
 */
u8 getFieldIndex(u8 fieldNum);


/**
 * Զ�̿��ػ�
 * remote_shutdwon 0:������ 1���ػ�
 */
void remoteShutdown(u8 shutdownFlag);

/**
 * ����������ֵ������Բ�ֵ
 */
void setAlmThrdUpDif(u16 value);

/**
 * ����������ֵ������Բ�ֵ
 */
void setAlmThrdDwDif(u16 value);

/**
 * ����������̸�����ֵ��������Χ����
 */
void updateTensionParasByMaxValue(void);

/**
 *
 *��ȡ ����A����#����B����{����A,B������ʽ :field_num+alarm_delay+alarm_sensitivity+ten_val_range[0]+ten_val_range[1]}
 */
void getWebFieldParas(char *fieldParas);

#endif
