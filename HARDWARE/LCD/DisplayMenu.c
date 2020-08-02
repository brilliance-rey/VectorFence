#include "delay.h"
#include <string.h>
#include "sys.h"
#include "version.h"
#include "lwip_comm.h"
#include "lcd.h"
#include "log.h"
#include "zk.h"
#include "DisplayMenu.h"
#include "alarm.h"
#include "lm75.h"
#include "pcf8563.h"
#include "RS485.h"
#include "adc.h"

unsigned char flag_PageState = NormalState;//ҳ��״̬��ʶ��1:������ʾ/0:�༭��ʾ��
unsigned char row_point = 0;//��ָ��
signed char row_fx_pos = 0;//ĳһ���ϵķ�����ʾλ�ã������޸�����ʱ��ʱ��ÿһ�л���3���ɱ༭�ķ���λ��


void DisplayInitMenu(void)//�ϵ��ʼ���˵�����ʾ��˾��Ϣ
{
	char init_msg[] = "���ڳ�ʼ����";
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_3,16,init_msg,DISPLAY_POSITIVE);
}

unsigned char run_screen_refresh = 0;
void DisplayRunMenu(void)
{
	//˫������0��XC-100WS, ��������1��XC-100WD
	char line_1D[] = "    XC-100WD    ";  
	char line_1S[] = "    XC-100WS    ";
	char line_2[] = "  ʸ��Χ������  ";
	char line_3[19] = {0};
	char line_4[] = "�豸�¶ȣ�    ��";
	strcpy(line_3,rtcTempStr);
	line_3[16] = '\0';

	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_8x16(Page_0,0,(FIELD_FLAG_S0D1?line_1D:line_1S),DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_2,0,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4,0,line_3,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_6,0,line_4,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6,5*16,crtLM75TempStr,DISPLAY_POSITIVE);
}

unsigned char UserId = OrdinaryUser;//�û����͡���Ĭ����ͨ�û�
void DisplayUserLogin(void)
{
	char line_1[] = "���û���¼��";
	char line_3[16];
	switch(UserId)
	{
		case SuperUser:
			strcpy(line_3,"�����û�");
			break;
		case OrdinaryUser:
			strcpy(line_3,"��ͨ�û�");
			break;
		
		default:
			break;
	}
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_0,16,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,16 * 2,line_3,DISPLAY_NEGATIVE);
}
/**************************�����û���Ŀ¼�˵�****************************/
void DisplaySuperUserRootMenu(void)
{
	char line_1[] = "���˵���";
	
	char list[4][16] = {
										"�趨",
										"��ѯ",
										"�������",
										"�汾��Ϣ"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 2,line_1,DISPLAY_POSITIVE);
	
	if(row_point > 3)
		{
			Display_Line_Index_16x16(Page_2,16 * 1,list[row_point - 3],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 1,list[row_point - 2],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_6,16 * 1,list[row_point - 1],DISPLAY_POSITIVE);
		}
	else
		{
			Display_Line_Index_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_6,16 * 1,list[2],DISPLAY_POSITIVE);
		}

}
/**************************��ͨ�û���Ŀ¼�˵�****************************/
void DisplayOrdinaryUserRootMenu(void)
{
	char line_1[] = "���˵���";
	
	char list[3][16] = {
										"��ѯ",
										"�������",
										"�汾��Ϣ"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 2,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_6,16 * 1,list[2],DISPLAY_POSITIVE);
}
/****************************һ���˵�****************************/
void DisplaySetMenu(void)
{
	char line_1[] = "���趨��";
	
	char list[9][16] = {
										"ϵͳ����",
										"��������",
										"У׼����",
										"��������",
										"�澯��ֵ",
										"�����¼",
										"�����ⲿ�豸",
										"����������ֵ",
										"��׼��Уʱ��"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 2,line_1,DISPLAY_POSITIVE);
	
	if(row_point > 3)
		{
			Display_Line_Index_16x16(Page_2,16 * 1,list[row_point - 3],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 1,list[row_point - 2],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_6,16 * 1,list[row_point - 1],DISPLAY_POSITIVE);
		}
	else
		{
			Display_Line_Index_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_6,16 * 1,list[2],DISPLAY_POSITIVE);
		}
}

void DisplayInquireMenu(void)
{
	char line_1[] = "����ѯ��";
	
	char list[9][16] = {
										"ϵͳ����",
										"��������",
										"��������",
										"�澯��¼",
										"�澯��ֵ",
										"У׼ֵ",
										"�����ⲿ�豸",
										"����������ֵ",
										"��׼��Уʱ��"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 2,line_1,DISPLAY_POSITIVE);
	
	if(row_point > 3)
		{
			Display_Line_Index_16x16(Page_2,16 * 1,list[row_point - 3],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 1,list[row_point - 2],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_6,16 * 1,list[row_point - 1],DISPLAY_POSITIVE);
		}
	else
		{
			Display_Line_Index_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_6,16 * 1,list[2],DISPLAY_POSITIVE);
		}
}

void DisplayTensionMonitor(void)
{
	char line_1[] = "��������ء�";
	char line_2[] = "����";
	char line_3[] = "B����";
	
	Lcd_Clear(FullScreen);
	
	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_2,16 * 1,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,16 * 1,line_3,DISPLAY_POSITIVE);
}

void DisplayVersionMenu(void)
{
	char line_1[] = "���汾��Ϣ��";
	
	char list[3][16] = {
										"Ӳ���汾��",
										"����汾��",
										"�������ã�",
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_16x16(Page_2,0,list[0],DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,0,list[1],DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_6,0,list[2],DISPLAY_POSITIVE);

	Display_Line_Index_8x16(Page_2,16*5,hardware_ver,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4,16*5,software_ver,DISPLAY_POSITIVE);
	
	memcpy(&list[2][10], FIELD_FLAG_S0D1 == 0?"˫":"��", 2);
	memcpy(&list[2][12], W4_W6 == 0?"��":"��", 2);
	
	list[2][14] = '\0';

	Display_Line_Index_16x16(Page_6,16*5, &list[2][10],DISPLAY_POSITIVE);
}
/****************************************************************/
/****************************�����˵�****************************/
void DisplaySetSysPara(void)
{
	char line_1[] = "��ϵͳ������";
	
	char list[2][16] = {
										"����ʱ��",
										"�������"
//										"RS485����"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	if(row_point > 3)
		{
			Display_Line_Index_16x16(Page_2,16 * 1,list[row_point - 3],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 1,list[row_point - 2],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_6,16 * 1,list[row_point - 1],DISPLAY_POSITIVE);
		}
	else
		{
			Display_Line_Index_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
//			Display_Line_Index_16x16(Page_6,16 * 1,list[2],DISPLAY_POSITIVE);
		}
}

void DiaplaySetFangQuPara(void)
{
	char line_1[] = "������������";
	char line_2[] = "����";
	char line_3[] = "B����";
	
//	char char_A[] = "A";
//	char char_B[] = "B";
//	char qu[] = "��";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_16x16(Page_2,16 * 1,line_2,DISPLAY_POSITIVE);
//	Display_Line_Index_8x16(Page_2,16 * 1,char_A,DISPLAY_POSITIVE);
//	Display_Line_Index_16x16(Page_2,8 * 3,qu,DISPLAY_POSITIVE);
	if(FIELD_FLAG_S0D1)
		{
			//����������ʾB����
		}
	else
		{
			Display_Line_Index_8x16_16x16(Page_4,16 * 1,line_3,DISPLAY_POSITIVE);
//			Display_Line_Index_8x16(Page_4,16 * 1,char_B,DISPLAY_POSITIVE);
//			Display_Line_Index_16x16(Page_4,8 * 3,qu,DISPLAY_POSITIVE);
		}
	//���½��ͨ����������ֻ�޸ķ�����ʱ��ʹ�澯��ʱ�ָ�ΪĬ��ֵ��bug
	memcpy(field_num_temp,field_num,sizeof(field_num_temp));//���������ŵ��ݴ�
	memcpy(alarm_delay_temp,alarm_delay,sizeof(alarm_delay_temp));//�����澯��ʱ���ݴ�

}

void DiaplayCalibrationSet(void)
{
	char line_1[] = "��У׼���á�";
	
	char list[2][16] = {
										"���ֵУ׼",
										"��׼ֵУ׼"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
}

u8 field_num_temp[2] = {0};  //A��B����Ӧ�ķ����ţ�ȡֵ��1-80�� 0:������
u16 alarm_delay_temp[2] = {ALARM_DURATION_DEF_S, ALARM_DURATION_DEF_S};	//A,B��������ʱ��Ĭ��30��, 0 ~ 999
u16 ten_val_range_temp[2][2];//A,B������Ӧ����Ч������Χ
u8 alarm_sensitivity_temp[2];//A,B������Ӧ��������

void DisplaySetAFangQuPara(void)
{
	char line_1[] = "��A ����������";
	char fangquhao[3] = "";
	char gaojingyanshi[4] = "";
	char lingmindu[3] = "";
	
	char list[3][16] = {
										"������: ",
										"�澯��ʱ:   ��",
										"������:  "
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_8x16_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	if(row_point > 3)
		{
			Display_Line_Index_8x16_16x16(Page_2,16 * 1,list[row_point - 3],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_4,16 * 1,list[row_point - 2],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_6,16 * 1,list[row_point - 1],DISPLAY_POSITIVE);
		}
	else
		{
			Display_Line_Index_8x16_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_6,16 * 1,list[2],DISPLAY_POSITIVE);
		}
	
	if(flag_PageState)//������ʾ
		{
			sprintf(fangquhao,"%02d",field_num[0]);
			sprintf(gaojingyanshi,"%03d",alarm_delay[0]);
			sprintf(lingmindu,"%02d",alarm_sensitivity[0]);
					
			Display_Line_Index_8x16(Page_2,16 * 5,fangquhao,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_4,8 * 11,gaojingyanshi,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_6,8 * 9,lingmindu,DISPLAY_POSITIVE);
		}
	else//�༭��ʾ
		{
			switch(row_point)
			{
				case 1:
					sprintf(fangquhao,"%02d",field_num_temp[0]);
					sprintf(gaojingyanshi,"%03d",alarm_delay[0]);
					sprintf(lingmindu,"%02d",alarm_sensitivity_temp[0]);
					Display_Line_Index_8x16(Page_2,16 * 5,fangquhao,DISPLAY_NEGATIVE);
					Display_Line_Index_8x16(Page_4,8 * 11,gaojingyanshi,DISPLAY_POSITIVE);
					Display_Line_Index_8x16(Page_6,8 * 9,lingmindu,DISPLAY_POSITIVE);
					break;
				case 2:
					sprintf(fangquhao,"%02d",field_num[0]);
					sprintf(gaojingyanshi,"%03d",alarm_delay_temp[0]);
					sprintf(lingmindu,"%02d",alarm_sensitivity_temp[0]);
					Display_Line_Index_8x16(Page_2,16 * 5,fangquhao,DISPLAY_POSITIVE);
					Display_Line_Index_8x16(Page_4,8 * 11,gaojingyanshi,DISPLAY_NEGATIVE);
					Display_Line_Index_8x16(Page_6,8 * 9,lingmindu,DISPLAY_POSITIVE);
					break;
				case 3:
					sprintf(fangquhao,"%02d",field_num[0]);
					sprintf(gaojingyanshi,"%03d",alarm_delay[0]);
					sprintf(lingmindu,"%02d",alarm_sensitivity_temp[0]);
					Display_Line_Index_8x16(Page_2,16 * 5,fangquhao,DISPLAY_POSITIVE);
					Display_Line_Index_8x16(Page_4,8 * 11,gaojingyanshi,DISPLAY_POSITIVE);
					Display_Line_Index_8x16(Page_6,8 * 9,lingmindu,DISPLAY_NEGATIVE);
					break;
				
				default:
					break;
			}
		}
}

void DisplaySetBFangQuPara(void)
{
	char line_1[] = "��B ����������";
	char fangquhao[3] = "";
	char gaojingyanshi[4] = "";
	char lingmindu[3] = "";
	
	char list[3][16] = {
										"������: ",
										"�澯��ʱ:   ��",
										"������:  "
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_8x16_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	if(row_point > 3)
		{
			Display_Line_Index_8x16_16x16(Page_2,16 * 1,list[row_point - 3],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_4,16 * 1,list[row_point - 2],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_6,16 * 1,list[row_point - 1],DISPLAY_POSITIVE);
		}
	else
		{
			Display_Line_Index_8x16_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_6,16 * 1,list[2],DISPLAY_POSITIVE);
		}
	
	if(flag_PageState)//������ʾ
		{
			sprintf(fangquhao,"%02d",field_num[1]);
			sprintf(gaojingyanshi,"%03d",alarm_delay[1]);
			sprintf(lingmindu,"%02d",alarm_sensitivity[1]);
					
			Display_Line_Index_8x16(Page_2,16 * 5,fangquhao,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_4,8 * 11,gaojingyanshi,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_6,8 * 9,lingmindu,DISPLAY_POSITIVE);
		}
	else//�༭��ʾ
		{
			switch(row_point)
			{
				case 1:
					sprintf(fangquhao,"%02d",field_num_temp[1]);
					sprintf(gaojingyanshi,"%03d",alarm_delay[1]);
					sprintf(lingmindu,"%02d",alarm_sensitivity_temp[1]);
					Display_Line_Index_8x16(Page_2,16 * 5,fangquhao,DISPLAY_NEGATIVE);
					Display_Line_Index_8x16(Page_4,8 * 11,gaojingyanshi,DISPLAY_POSITIVE);
					Display_Line_Index_8x16(Page_6,8 * 9,lingmindu,DISPLAY_POSITIVE);
					break;
				case 2:
					sprintf(fangquhao,"%02d",field_num[1]);
					sprintf(gaojingyanshi,"%03d",alarm_delay_temp[1]);
					sprintf(lingmindu,"%02d",alarm_sensitivity_temp[1]);
					Display_Line_Index_8x16(Page_2,16 * 5,fangquhao,DISPLAY_POSITIVE);
					Display_Line_Index_8x16(Page_4,8 * 11,gaojingyanshi,DISPLAY_NEGATIVE);
					Display_Line_Index_8x16(Page_6,8 * 9,lingmindu,DISPLAY_POSITIVE);
					break;
				case 3:
					sprintf(fangquhao,"%02d",field_num[1]);
					sprintf(gaojingyanshi,"%03d",alarm_delay[1]);
					sprintf(lingmindu,"%02d",alarm_sensitivity_temp[1]);
					Display_Line_Index_8x16(Page_2,16 * 5,fangquhao,DISPLAY_POSITIVE);
					Display_Line_Index_8x16(Page_4,8 * 11,gaojingyanshi,DISPLAY_POSITIVE);
					Display_Line_Index_8x16(Page_6,8 * 9,lingmindu,DISPLAY_NEGATIVE);
					break;
				
				default:
					break;
			}
		}
}

void DisplayiLlegalInput(void)
{
	char line_1[] = "�Ƿ���Χ";
	char line_2[] = "���������룡";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_2,16 * 2,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,16,line_2,DISPLAY_POSITIVE);
}

void DisplayZeroCalibration(void)
{
	char line_1[] = "�����У׼��";
	
	char list[2][16] = {
										"A ����",
										"B ����"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
}

void DisplayAareaZeroVal(void)
{
	char line_1[] = "��A �������ֵ��";
	char line_2[] = "L1:  .  L2:  . ";
	char line_3[] = "L3:  .  L4:  . ";
	char line_4[] = "L5:  .  L6:  . ";
	
	char wire_1[3] = "";
	char wire_1_dot[2] = "";
	char wire_2[3] = "";
	char wire_2_dot[2] = "";
	char wire_3[3] = "";
	char wire_3_dot[2] = "";
	char wire_4[3] = "";
	char wire_4_dot[2] = "";
	char wire_5[3] = "";
	char wire_5_dot[2] = "";
	char wire_6[3] = "";
	char wire_6_dot[2] = "";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_8x16_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_16x16(Page_2,0,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,0,line_3,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_6,0,line_4,DISPLAY_POSITIVE);
	
	sprintf(wire_1,"%02d",zero_val[0][0] / 10);
	sprintf(wire_1_dot,"%01d",zero_val[0][0] % 10);
	sprintf(wire_2,"%02d",zero_val[0][1] / 10);
	sprintf(wire_2_dot,"%01d",zero_val[0][1] % 10);
	sprintf(wire_3,"%02d",zero_val[0][2] / 10);
	sprintf(wire_3_dot,"%01d",zero_val[0][2] % 10);
	sprintf(wire_4,"%02d",zero_val[0][3] / 10);
	sprintf(wire_4_dot,"%01d",zero_val[0][3] % 10);
	sprintf(wire_5,"%02d",zero_val[0][4] / 10);
	sprintf(wire_5_dot,"%01d",zero_val[0][4] % 10);
	sprintf(wire_6,"%02d",zero_val[0][5] / 10);
	sprintf(wire_6_dot,"%01d",zero_val[0][5] % 10);
	
	Display_Line_Index_8x16(Page_2, 8 * 3, wire_1, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 6, wire_1_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 11, wire_2, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 14, wire_2_dot, DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_4, 8 * 3, wire_3, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 6, wire_3_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 11, wire_4, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 14, wire_4_dot, DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_6, 8 * 3, wire_5, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 6, wire_5_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 11, wire_6, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 14, wire_6_dot, DISPLAY_POSITIVE);
}

void DisplayBareaZeroVal(void)
{
	char line_1[] = "��B �������ֵ��";
	char line_2[] = "L1:  .  L2:  . ";
	char line_3[] = "L3:  .  L4:  . ";
	char line_4[] = "L5:  .  L6:  . ";
	
	char wire_1[3] = "";
	char wire_1_dot[2] = "";
	char wire_2[3] = "";
	char wire_2_dot[2] = "";
	char wire_3[3] = "";
	char wire_3_dot[2] = "";
	char wire_4[3] = "";
	char wire_4_dot[2] = "";
	char wire_5[3] = "";
	char wire_5_dot[2] = "";
	char wire_6[3] = "";
	char wire_6_dot[2] = "";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_8x16_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_16x16(Page_2,0,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,0,line_3,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_6,0,line_4,DISPLAY_POSITIVE);
	
	sprintf(wire_1,"%02d",zero_val[1][0] / 10);
	sprintf(wire_1_dot,"%01d",zero_val[0][0] % 10);
	sprintf(wire_2,"%02d",zero_val[1][1] / 10);
	sprintf(wire_2_dot,"%01d",zero_val[0][1] % 10);
	sprintf(wire_3,"%02d",zero_val[1][2] / 10);
	sprintf(wire_3_dot,"%01d",zero_val[0][2] % 10);
	sprintf(wire_4,"%02d",zero_val[1][3] / 10);
	sprintf(wire_4_dot,"%01d",zero_val[0][3] % 10);
	sprintf(wire_5,"%02d",zero_val[1][4] / 10);
	sprintf(wire_5_dot,"%01d",zero_val[0][4] % 10);
	sprintf(wire_6,"%02d",zero_val[1][5] / 10);
	sprintf(wire_6_dot,"%01d",zero_val[1][5] % 10);
	
	Display_Line_Index_8x16(Page_2, 8 * 3, wire_1, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 6, wire_1_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 11, wire_2, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 14, wire_2_dot, DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_4, 8 * 3, wire_3, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 6, wire_3_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 11, wire_4, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 14, wire_4_dot, DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_6, 8 * 3, wire_5, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 6, wire_5_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 11, wire_6, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 14, wire_6_dot, DISPLAY_POSITIVE);
}

void DisplayDatumCalibration(void)
{
	char line_1[] = "����׼У׼��";
	
	char list[2][16] = {
										"A ����",
										"B ����"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
}

void Displayzhengzaijiaozhun(void)
{
	char line_1[] = "����У׼";
	char line_2[] = "���Ժ�";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_2,16 * 2,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,16 * 2,line_2,DISPLAY_POSITIVE);
}

void DisplayAareaDatumVal(void)
{
	char line_1[] = "��A ������׼ֵ��";
	char line_2[] = "L1:  .  L2:  . ";
	char line_3[] = "L3:  .  L4:  . ";
	char line_4[] = "L5:  .  L6:  . ";
	
	char wire_1[3] = "";
	char wire_1_dot[2] = "";
	char wire_2[3] = "";
	char wire_2_dot[2] = "";
	char wire_3[3] = "";
	char wire_3_dot[2] = "";
	char wire_4[3] = "";
	char wire_4_dot[2] = "";
	char wire_5[3] = "";
	char wire_5_dot[2] = "";
	char wire_6[3] = "";
	char wire_6_dot[2] = "";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_8x16_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_16x16(Page_2,0,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,0,line_3,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_6,0,line_4,DISPLAY_POSITIVE);
	
	sprintf(wire_1,"%02d",base_val[0][0] / 10);
	sprintf(wire_1_dot,"%01d",zero_val[0][0] % 10);
	sprintf(wire_2,"%02d",base_val[0][1] / 10);
	sprintf(wire_2_dot,"%01d",zero_val[0][1] % 10);
	sprintf(wire_3,"%02d",base_val[0][2] / 10);
	sprintf(wire_3_dot,"%01d",zero_val[0][2] % 10);
	sprintf(wire_4,"%02d",base_val[0][3] / 10);
	sprintf(wire_4_dot,"%01d",base_val[0][3] % 10);
	sprintf(wire_5,"%02d",base_val[0][4] / 10);
	sprintf(wire_5_dot,"%01d",base_val[0][4] % 10);
	sprintf(wire_6,"%02d",base_val[0][5] / 10);
	sprintf(wire_6_dot,"%01d",base_val[0][5] % 10);
	
	Display_Line_Index_8x16(Page_2, 8 * 3, wire_1, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 6, wire_1_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 11, wire_2, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 14, wire_2_dot, DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_4, 8 * 3, wire_3, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 6, wire_3_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 11, wire_4, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 14, wire_4_dot, DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_6, 8 * 3, wire_5, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 6, wire_5_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 11, wire_6, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 14, wire_6_dot, DISPLAY_POSITIVE);
}

void DisplayBareaDatumVal(void)
{
	char line_1[] = "��B ������׼ֵ��";
	char line_2[] = "L1:  .  L2:  . ";
	char line_3[] = "L3:  .  L4:  . ";
	char line_4[] = "L5:  .  L6:  . ";
	
	char wire_1[3] = "";
	char wire_1_dot[2] = "";
	char wire_2[3] = "";
	char wire_2_dot[2] = "";
	char wire_3[3] = "";
	char wire_3_dot[2] = "";
	char wire_4[3] = "";
	char wire_4_dot[2] = "";
	char wire_5[3] = "";
	char wire_5_dot[2] = "";
	char wire_6[3] = "";
	char wire_6_dot[2] = "";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_8x16_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_16x16(Page_2,0,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,0,line_3,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_6,0,line_4,DISPLAY_POSITIVE);
	
	sprintf(wire_1,"%02d",base_val[1][0] / 10);
	sprintf(wire_1_dot,"%01d",base_val[0][0] % 10);
	sprintf(wire_2,"%02d",base_val[1][1] / 10);
	sprintf(wire_2_dot,"%01d",base_val[0][1] % 10);
	sprintf(wire_3,"%02d",base_val[1][2] / 10);
	sprintf(wire_3_dot,"%01d",base_val[0][2] % 10);
	sprintf(wire_4,"%02d",base_val[1][3] / 10);
	sprintf(wire_4_dot,"%01d",base_val[0][3] % 10);
	sprintf(wire_5,"%02d",base_val[1][4] / 10);
	sprintf(wire_5_dot,"%01d",base_val[0][4] % 10);
	sprintf(wire_6,"%02d",base_val[1][5] / 10);
	sprintf(wire_6_dot,"%01d",base_val[1][5] % 10);
	
	Display_Line_Index_8x16(Page_2, 8 * 3, wire_1, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 6, wire_1_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 11, wire_2, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 14, wire_2_dot, DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_4, 8 * 3, wire_3, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 6, wire_3_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 11, wire_4, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 14, wire_4_dot, DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_6, 8 * 3, wire_5, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 6, wire_5_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 11, wire_6, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 14, wire_6_dot, DISPLAY_POSITIVE);
}

void DisplayNotAtValidRange(void)
{
	char line_1[] = "δ����Ч��Χ";
	char line_2[] = "���������ֵ";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_2,16 * 1,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,16 * 1,line_2,DISPLAY_POSITIVE);
}

void DiaplaySetBuFangPara(void)
{
	char line_1[] = "������������";
	char char_A[] = "A";
	char char_B[] = "B";
	char char_maohao[] = ":";
	char kai[] = "��";
	char guan[] = "��";
	char fangqu[] = "����";
	char fangchaigaojing[] = "����澯";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_2,2*8,char_A,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_2,3 * 8,fangqu,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2,7*8,char_maohao,DISPLAY_POSITIVE);
	
	if(FIELD_FLAG_S0D1)
		{
			//����������ʾB����
		}
	else
		{
			Display_Line_Index_8x16(Page_4,2*8,char_B,DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,3 * 8,fangqu,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_4,7*8,char_maohao,DISPLAY_POSITIVE);
		}
	
	Display_Line_Index_16x16(Page_6,2 * 8,fangchaigaojing,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6,5*16,char_maohao,DISPLAY_POSITIVE);
	
	if(flag_PageState)//������ʾʱ����Ҫ��ʾflag_fcgjsn������
		{
			if(BU_FANG_A_FLAG == ON)//�ж�A�������ر��
				{
					Display_Line_Index_16x16(Page_2,16 * 4,kai,DISPLAY_POSITIVE);//��ʾ������
				}
			else
				{
					Display_Line_Index_16x16(Page_2,16 * 4,guan,DISPLAY_POSITIVE);//��ʾ���ء�
				}
			
			//Display_Line_Index_8x16(Page_2,5*16,volStepStr[APWM_Level],DISPLAY_POSITIVE);

			if(FIELD_FLAG_S0D1)
				{
					//����������ʾB����
				}
			else
				{
					if(BU_FANG_B_FLAG == ON)//�ж�B�������ر��
						{
							Display_Line_Index_16x16(Page_4,16 * 4,kai,DISPLAY_POSITIVE);//��ʾ������
						}
					else
						{
							Display_Line_Index_16x16(Page_4,16 * 4,guan,DISPLAY_POSITIVE);//��ʾ���ء�
						}
					
					//Display_Line_Index_8x16(Page_4,5*16,volStepStr[BPWM_Level],DISPLAY_POSITIVE);
				}
			
			if(flag_fcgjsn == ON)//�жϷ���澯ʹ�ܱ�ʶ
				{
					Display_Line_Index_16x16(Page_6,16 * 6,kai,DISPLAY_POSITIVE);//��ʾ������
				}
			else
				{
					Display_Line_Index_16x16(Page_6,16 * 6,guan,DISPLAY_POSITIVE);//��ʾ���ء�
				}
		}
	else//�༭��ʾʱ����Ҫ��ʾ
		{
			switch(row_point)
			{
				case 1://A������Ҫ�༭
					if(row_fx_pos == 1)//��һ��λ����Ҫ������ʾ
						{
							if(BU_FANG_A_FLAG_TEMP == ON)//�ж�A�������ر��
								{
									Display_Line_Index_16x16(Page_2,16 * 4,kai,DISPLAY_NEGATIVE);//��ʾ������
								}
							else
								{
									Display_Line_Index_16x16(Page_2,16 * 4,guan,DISPLAY_NEGATIVE);//��ʾ���ء�
								}
//							Display_Line_Index_8x16(Page_2,5*16,volStepStr[BU_FANG_VOL_TEMP_A],DISPLAY_POSITIVE);
						}
					else if(row_fx_pos == 2)//�ڶ���λ����Ҫ������ʾ
						{
							if(BU_FANG_A_FLAG_TEMP == ON)//�ж�A�������ر��
								{
									Display_Line_Index_16x16(Page_2,16 * 4,kai,DISPLAY_POSITIVE);//��ʾ������
								}
							else
								{
									Display_Line_Index_16x16(Page_2,16 * 4,guan,DISPLAY_POSITIVE);//��ʾ���ء�
								}
//							Display_Line_Index_8x16(Page_2,5*16,volStepStr[BU_FANG_VOL_TEMP_A],DISPLAY_NEGATIVE);
						}
						//����Ϊ��ʱ��A�����ػ��ѹ�ȼ��ɱ༭��������ʾʱֻ��A�����غ͵�ѹ�ȼ�ʹ��TEMP����ʾ�����಻����TEMP����ʾ��
					if(FIELD_FLAG_S0D1)
						{
							//����������ʾB����
						}
					else
						{
							if(BU_FANG_B_FLAG == ON)//�ж�B�������ر�ǣ�����ʹ��BU_FANG_B_FLAG_TEMP�������жϣ�
								{
									Display_Line_Index_16x16(Page_4,16 * 4,kai,DISPLAY_POSITIVE);//��ʾ������
								}
							else
								{
									Display_Line_Index_16x16(Page_4,16 * 4,guan,DISPLAY_POSITIVE);//��ʾ���ء�
								}
//							Display_Line_Index_8x16(Page_4,5*16,volStepStr[BPWM_Level],DISPLAY_POSITIVE);
						}
					
					if(flag_fcgjsn == ON)//�жϷ���澯ʹ�ܱ�ʶ��������ʹ��flag_fcgjsn_tmp�������жϣ�
						{
							Display_Line_Index_16x16(Page_6,16 * 6,kai,DISPLAY_POSITIVE);//��ʾ������
						}
					else
						{
							Display_Line_Index_16x16(Page_6,16 * 6,guan,DISPLAY_POSITIVE);//��ʾ���ء�
						}
					break;
				case 2://B������Ҫ�༭
					if(row_fx_pos == 1)//��һ��λ����Ҫ������ʾ
						{
							if(BU_FANG_B_FLAG_TEMP == ON)//�ж�B�������ر��
								{
									Display_Line_Index_16x16(Page_4,16 * 4,kai,DISPLAY_NEGATIVE);//��ʾ������
								}
							else
								{
									Display_Line_Index_16x16(Page_4,16 * 4,guan,DISPLAY_NEGATIVE);//��ʾ���ء�
								}
//							Display_Line_Index_8x16(Page_4,5*16,volStepStr[BU_FANG_VOL_TEMP_B],DISPLAY_POSITIVE);
						}
					else if(row_fx_pos == 2)//�ڶ���λ����Ҫ������ʾ
						{
							if(BU_FANG_B_FLAG_TEMP == ON)//�ж�B�������ر��
								{
									Display_Line_Index_16x16(Page_4,16 * 4,kai,DISPLAY_POSITIVE);//��ʾ������
								}
							else
								{
									Display_Line_Index_16x16(Page_4,16 * 4,guan,DISPLAY_POSITIVE);//��ʾ���ء�
								}
//							Display_Line_Index_8x16(Page_4,5*16,volStepStr[BU_FANG_VOL_TEMP_B],DISPLAY_NEGATIVE);
						}
						
					if(BU_FANG_A_FLAG == ON)//�ж�A�������ر��
						{
							Display_Line_Index_16x16(Page_2,16 * 4,kai,DISPLAY_POSITIVE);//��ʾ������
						}
					else
						{
							Display_Line_Index_16x16(Page_2,16 * 4,guan,DISPLAY_POSITIVE);//��ʾ���ء�
						}
//					Display_Line_Index_8x16(Page_2,5*16,volStepStr[APWM_Level],DISPLAY_POSITIVE);
					
					if(flag_fcgjsn == ON)//�жϷ���澯ʹ�ܱ�ʶ
						{
							Display_Line_Index_16x16(Page_6,16 * 6,kai,DISPLAY_POSITIVE);//��ʾ������
						}
					else
						{
							Display_Line_Index_16x16(Page_6,16 * 6,guan,DISPLAY_POSITIVE);//��ʾ���ء�
						}
					break;
				case 3://����澯ʹ����Ҫ�༭
					if(BU_FANG_A_FLAG == ON)//�ж�A�������ر��
						{
							Display_Line_Index_16x16(Page_2,16 * 4,kai,DISPLAY_POSITIVE);//��ʾ������
						}
					else
						{
							Display_Line_Index_16x16(Page_2,16 * 4,guan,DISPLAY_POSITIVE);//��ʾ���ء�
						}
//					Display_Line_Index_8x16(Page_2,5*16,volStepStr[APWM_Level],DISPLAY_POSITIVE);
					
					if(FIELD_FLAG_S0D1)
						{
							//����������ʾB����
						}
					else
						{
							if(BU_FANG_B_FLAG == ON)//�ж�B�������ر��
								{
									Display_Line_Index_16x16(Page_4,16 * 4,kai,DISPLAY_POSITIVE);//��ʾ������
								}
							else
								{
									Display_Line_Index_16x16(Page_4,16 * 4,guan,DISPLAY_POSITIVE);//��ʾ���ء�
								}
//							Display_Line_Index_8x16(Page_4,5*16,volStepStr[BPWM_Level],DISPLAY_POSITIVE);
						}
					
					if(flag_fcgjsn_tmp == ON)//�жϷ���澯ʹ�ܱ�ʶ
						{
							Display_Line_Index_16x16(Page_6,16 * 6,kai,DISPLAY_NEGATIVE);//��ʾ������
						}
					else
						{
							Display_Line_Index_16x16(Page_6,16 * 6,guan,DISPLAY_NEGATIVE);//��ʾ���ء�
						}
					break;
				default:
					break;
			}
		}
}

//������ֵ������Բ�ֵ���磺203 -> 20.3Kg
u16 alarm_threshold_up_dif_temp;
//�ɳ���ֵ������Բ�ֵ, �磺203 -> 20.3Kg
u16 alarm_threshold_down_dif_temp;
void DiaplaySetAlarmThreshold(void)
{
	char line_1[] = "���澯��ֵ��";
	char line_2[] = "��ƫ��:   . Kg";
	char line_3[] = "��ƫ��:   . Kg";
	
	char shangpianyi[3] = "";
	char shangpianyi_dot[2] = "";
	char xiapianyi[3] = "";
	char xiapianyi_dot[2] = "";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_2,16 * 1,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,16 * 1,line_3,DISPLAY_POSITIVE);
	
	if(flag_PageState)//������ʾ
		{
			sprintf(shangpianyi,"%02d",(alarm_threshold_up_dif / 10));
			sprintf(shangpianyi_dot,"%01d",(alarm_threshold_up_dif % 10));
			sprintf(xiapianyi,"%02d",(alarm_threshold_down_dif / 10));
			sprintf(xiapianyi_dot,"%01d",(alarm_threshold_down_dif % 10));
			
			Display_Line_Index_8x16(Page_2, 8 * 10, shangpianyi, DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_2, 8 * 13, shangpianyi_dot, DISPLAY_POSITIVE);
			
			Display_Line_Index_8x16(Page_4, 8 * 10, xiapianyi, DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_4, 8 * 13, xiapianyi_dot, DISPLAY_POSITIVE);
		}
	else//�༭��ʾ
		{
			switch(row_point)
			{
				case 1:
					sprintf(shangpianyi,"%02d",(alarm_threshold_up_dif_temp / 10));
					sprintf(shangpianyi_dot,"%01d",(alarm_threshold_up_dif_temp % 10));
					sprintf(xiapianyi,"%02d",(alarm_threshold_down_dif / 10));
					sprintf(xiapianyi_dot,"%01d",(alarm_threshold_down_dif % 10));
					switch(row_fx_pos)
					{
						case 1://��һ������λ��
							Display_Line_Index_8x16(Page_2, 8 * 10, shangpianyi, DISPLAY_NEGATIVE);
							Display_Line_Index_8x16(Page_2, 8 * 13, shangpianyi_dot, DISPLAY_POSITIVE);
							break;
						case 2://�ڶ�������λ��
							Display_Line_Index_8x16(Page_2, 8 * 10, shangpianyi, DISPLAY_POSITIVE);
							Display_Line_Index_8x16(Page_2, 8 * 13, shangpianyi_dot, DISPLAY_NEGATIVE);
							break;
						
						default:
							break;
					}
					Display_Line_Index_8x16(Page_4, 8 * 10, xiapianyi, DISPLAY_POSITIVE);
					Display_Line_Index_8x16(Page_4, 8 * 13, xiapianyi_dot, DISPLAY_POSITIVE);
					break;
				case 2:
					sprintf(shangpianyi,"%02d",(alarm_threshold_up_dif / 10));
					sprintf(shangpianyi_dot,"%01d",(alarm_threshold_up_dif % 10));
					sprintf(xiapianyi,"%02d",(alarm_threshold_down_dif_temp / 10));
					sprintf(xiapianyi_dot,"%01d",(alarm_threshold_down_dif_temp % 10));
					Display_Line_Index_8x16(Page_2, 8 * 10, shangpianyi, DISPLAY_POSITIVE);
					Display_Line_Index_8x16(Page_2, 8 * 13, shangpianyi_dot, DISPLAY_POSITIVE);
					switch(row_fx_pos)
					{
						case 1://��һ������λ��
							Display_Line_Index_8x16(Page_4, 8 * 10, xiapianyi, DISPLAY_NEGATIVE);
							Display_Line_Index_8x16(Page_4, 8 * 13, xiapianyi_dot, DISPLAY_POSITIVE);
							break;
						case 2://�ڶ�������λ��
							Display_Line_Index_8x16(Page_4, 8 * 10, xiapianyi, DISPLAY_POSITIVE);
							Display_Line_Index_8x16(Page_4, 8 * 13, xiapianyi_dot, DISPLAY_NEGATIVE);
							break;
						
						default:
							break;
					}
					break;
				
				default:
					break;
			}
		}
}

void DisplaySetClearRecord(void)
{
	char line_1[] = "�������¼��";
	
	char list[3][16] = {
										"���ָ澯",
										"ƫ�Ƹ澯",
										"����澯"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	if(row_point > 3)
		{
			Display_Line_Index_16x16(Page_2,16 * 1,list[row_point - 3],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 1,list[row_point - 2],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_6,16 * 1,list[row_point - 1],DISPLAY_POSITIVE);
		}
	else
		{
			Display_Line_Index_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_6,16 * 1,list[2],DISPLAY_POSITIVE);
		}
}

void DisplayZhengZaiQingChu(void)
{
	char list[2][16] = {
										"�������",
										"���Ժ󣡣�"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_2,16 * 2,list[0],DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,16 * 2,list[1],DISPLAY_POSITIVE);
}

void DisplayInqSysPara(void)
{
	
	char line_1[] = "��ϵͳ������";
	
	char list[1][16] = {
										"�������"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
}

void DiaplayInqFangQuPara(void)
{
	char line_1[] = "������������";
	
	char char_A[] = "A";
	char char_B[] = "B";
	char fangqu[] = "����";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_2,16 * 1,char_A,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_2,8 * 3,fangqu,DISPLAY_POSITIVE);
	if(FIELD_FLAG_S0D1)
		{
			//����������ʾB����
		}
	else
		{
			Display_Line_Index_8x16(Page_4,16 * 1,char_B,DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,8 * 3,fangqu,DISPLAY_POSITIVE);
		}
}

void DiaplayInqAFangQuPara(void)
{
	char line_1[] = "��A ����������";
	char fangquhao[3] = "";
	char gaojingyanshi[4] = "";
	char min_tension[3] = "";
	char max_tension[3] = "";
	char lingmindu[3] = "";
	
	char list[4][16] = {
										"������: ",
										"�澯��ʱ:   ��",
										"����ֵ:  -  Kg",
										"������:  "
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_8x16_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	if(row_point > 3)
		{
			Display_Line_Index_8x16_16x16(Page_2,16 * 1,list[row_point - 3],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_4,16 * 1,list[row_point - 2],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_6,16 * 1,list[row_point - 1],DISPLAY_POSITIVE);
			
			sprintf(gaojingyanshi,"%03d",alarm_delay[0]);
			sprintf(min_tension,"%02d",ten_val_range[0][0] / 10);
			sprintf(max_tension,"%02d",ten_val_range[0][1] / 10);
			sprintf(lingmindu,"%02d",alarm_sensitivity[0]);
			
			Display_Line_Index_8x16(Page_2,8 * 11,gaojingyanshi,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_4,8 * 9,min_tension,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_4,8 * 12,max_tension,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_6,8 * 9,lingmindu,DISPLAY_POSITIVE);
		}
	else
		{
			Display_Line_Index_8x16_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_6,16 * 1,list[2],DISPLAY_POSITIVE);
			
			sprintf(fangquhao,"%02d",field_num[0]);
			sprintf(gaojingyanshi,"%03d",alarm_delay[0]);
			sprintf(min_tension,"%02d",ten_val_range[0][0] / 10);
			sprintf(max_tension,"%02d",ten_val_range[0][1] / 10);
			
			Display_Line_Index_8x16(Page_2,16 * 5,fangquhao,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_4,8 * 11,gaojingyanshi,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_6,8 * 9,min_tension,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_6,8 * 12,max_tension,DISPLAY_POSITIVE);
		}
}

void DiaplayInqBFangQuPara(void)
{
	char line_1[] = "��B ����������";
	char fangquhao[3] = "";
	char gaojingyanshi[4] = "";
	char min_tension[3] = "";
	char max_tension[3] = "";
	char lingmindu[3] = "";
	
	char list[4][16] = {
										"������: ",
										"�澯��ʱ:   ��",
										"����ֵ:  -  Kg",
										"������:  "
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_8x16_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	if(row_point > 3)
		{
			Display_Line_Index_8x16_16x16(Page_2,16 * 1,list[row_point - 3],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_4,16 * 1,list[row_point - 2],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_6,16 * 1,list[row_point - 1],DISPLAY_POSITIVE);
			
			sprintf(gaojingyanshi,"%03d",alarm_delay[1]);
			sprintf(min_tension,"%02d",ten_val_range[1][0] / 10);
			sprintf(max_tension,"%02d",ten_val_range[1][1] / 10);
			sprintf(lingmindu,"%02d",alarm_sensitivity[1]);
			
			Display_Line_Index_8x16(Page_2,8 * 11,gaojingyanshi,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_4,8 * 9,min_tension,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_4,8 * 12,max_tension,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_6,8 * 9,lingmindu,DISPLAY_POSITIVE);
		}
	else
		{
			Display_Line_Index_8x16_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
			Display_Line_Index_8x16_16x16(Page_6,16 * 1,list[2],DISPLAY_POSITIVE);
			
			sprintf(fangquhao,"%02d",field_num[1]);
			sprintf(gaojingyanshi,"%03d",alarm_delay[1]);
			sprintf(min_tension,"%02d",ten_val_range[1][0] / 10);
			sprintf(max_tension,"%02d",ten_val_range[1][1] / 10);
			
			Display_Line_Index_8x16(Page_2,16 * 5,fangquhao,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_4,8 * 11,gaojingyanshi,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_6,8 * 9,min_tension,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_6,8 * 12,max_tension,DISPLAY_POSITIVE);
		}
}

void DisplayInqBuFangPara(void)
{
	char line_1[] = "������������";
	char char_A[] = "A";
	char char_B[] = "B";
	char char_maohao[] = ":";
	char kai[] = "��";
	char guan[] = "��";
	char list[3][16] = {
										"����    ",
										"����    ",
										"����澯"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_16x16(Page_2,3 * 8,list[0],DISPLAY_POSITIVE);
	if(FIELD_FLAG_S0D1)
		{
			//����������ʾB����
		}
	else
		{
			Display_Line_Index_16x16(Page_4,3 * 8,list[1],DISPLAY_POSITIVE);//˫������ʾB����
		}
	Display_Line_Index_16x16(Page_6,16 * 1,list[2],DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_2,2*8,char_A,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2,7*8,char_maohao,DISPLAY_POSITIVE);
	
	if(FIELD_FLAG_S0D1)
		{
			//����������ʾB����
		}
	else
		{
			Display_Line_Index_8x16(Page_4,2*8,char_B,DISPLAY_POSITIVE);
			Display_Line_Index_8x16(Page_4,7*8,char_maohao,DISPLAY_POSITIVE);
		}
	
	
	Display_Line_Index_8x16(Page_6,5*16,char_maohao,DISPLAY_POSITIVE);
	
	if(BU_FANG_A_FLAG == ON)//�ж�A�������ر��
		{
			Display_Line_Index_16x16(Page_2,16 * 4,kai,DISPLAY_POSITIVE);//��ʾ������
		}
	else
		{
			Display_Line_Index_16x16(Page_2,16 * 4,guan,DISPLAY_POSITIVE);//��ʾ���ء�
		}
	
//	Display_Line_Index_8x16(Page_2,5*16,volStepStr[APWM_Level],DISPLAY_POSITIVE);

	if(FIELD_FLAG_S0D1)
		{
			//����������ʾB����
		}
	else
		{
			if(BU_FANG_B_FLAG == ON)//�ж�B�������ر��
				{
					Display_Line_Index_16x16(Page_4,16 * 4,kai,DISPLAY_POSITIVE);//��ʾ������
				}
			else
				{
					Display_Line_Index_16x16(Page_4,16 * 4,guan,DISPLAY_POSITIVE);//��ʾ���ء�
				}
			
//			Display_Line_Index_8x16(Page_4,5*16,volStepStr[BPWM_Level],DISPLAY_POSITIVE);
		}
	
	if(flag_fcgjsn == ON)//�жϷ���澯ʹ�ܱ�ʶ
		{
			Display_Line_Index_16x16(Page_6,16 * 6,kai,DISPLAY_POSITIVE);//��ʾ������
		}
	else
		{
			Display_Line_Index_16x16(Page_6,16 * 6,guan,DISPLAY_POSITIVE);//��ʾ���ء�
		}
}

void DisplayInqAlarmRecord(void)
{
	char line_1[] = "���澯��¼��";
	
	char list[3][16] = {
										"���ָ澯",
										"ƫ�Ƹ澯",
										"����澯"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_6,16 * 1,list[2],DISPLAY_POSITIVE);
}

void DisplayInqAlarmThreshold(void)
{
	char line_1[] = "���澯��ֵ��";
	char line_2[] = "��ƫ��:   . Kg";
	char line_3[] = "��ƫ��:   . Kg";
	
	char shangpianyi[3] = "";
	char shangpianyi_dot[2] = "";
	char xiapianyi[3] = "";
	char xiapianyi_dot[2] = "";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_2,16 * 1,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,16 * 1,line_3,DISPLAY_POSITIVE);
	
	sprintf(shangpianyi,"%02d",(alarm_threshold_up_dif / 10));
	sprintf(shangpianyi_dot,"%01d",(alarm_threshold_up_dif % 10));
	sprintf(xiapianyi,"%02d",(alarm_threshold_down_dif / 10));
	sprintf(xiapianyi_dot,"%01d",(alarm_threshold_down_dif % 10));
			
	Display_Line_Index_8x16(Page_2, 8 * 10, shangpianyi, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 13, shangpianyi_dot, DISPLAY_POSITIVE);
			
	Display_Line_Index_8x16(Page_4, 8 * 10, xiapianyi, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 13, xiapianyi_dot, DISPLAY_POSITIVE);
}

void DisplayInqCalibrationVal(void)
{
	char line_1[] = "��У׼ֵ��";
	
	char list[2][16] = {
										"���ֵ",
										"��׼ֵ"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
}

void DisplayInqZeroVal(void)
{
	char line_1[] = "�����ֵ��";
	
	char list[2][16] = {
										"����",
										"B����"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
	
	
}

void DiaplayInqDatumVal(void)
{
	char line_1[] = "����׼ֵ��";
	
	char list[2][16] = {
										"����",
										"B����"
		};
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_16x16(Page_2,16 * 1,list[0],DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,16 * 1,list[1],DISPLAY_POSITIVE);
	
	
}

void DisplayInqExtDeviceType(void)
{
	char line_1[] = "�������ⲿ�豸��";
	char line_2[] = "����:";
	char line_2_2[] = "";
	char line_3[] = "B����:";
	char line_3_2[] = "";
	char line_4[] = "�������Ϳ�������";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 0,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_16x16(Page_2,16 * 1,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,16 * 1,line_3,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_6,0,line_4,DISPLAY_POSITIVE);
	
	switch(SW_Type[0])
	{
		case 0:
			strcpy(line_2_2,"���豸");
			break;
		case 1:
			strcpy(line_2_2,"��������");
			break;
		case 2:
			strcpy(line_2_2,"��������");
			break;
		case 3:
			strcpy(line_2_2,"˫��̽��");
			break;
		case 4:
			strcpy(line_2_2,"����̽��");
			break;
		case 5:
			strcpy(line_2_2,"�Ŵ�̽��");
			break;
		case 6:
			strcpy(line_2_2,"������ť");
			break;
		case 7:
			strcpy(line_2_2,"ˮ��̽��");
			break;
		case 8:
			strcpy(line_2_2,"�����豸");
			break;
		default:
			break;
	}
	Display_Line_Index_16x16(Page_2,16 * 4,line_2_2,DISPLAY_POSITIVE);
	if(FIELD_FLAG_S0D1)
		{
			//����������ʾB����
		}
	else
		{
			switch(SW_Type[1])
			{
				case 0:
					strcpy(line_3_2,"���豸");
					break;
				case 1:
					strcpy(line_3_2,"��������");
					break;
				case 2:
					strcpy(line_3_2,"��������");
					break;
				case 3:
					strcpy(line_3_2,"˫��̽��");
					break;
				case 4:
					strcpy(line_3_2,"����̽��");
					break;
				case 5:
					strcpy(line_3_2,"�Ŵ�̽��");
					break;
				case 6:
					strcpy(line_3_2,"������ť");
					break;
				case 7:
					strcpy(line_3_2,"ˮ��̽��");
					break;
				case 8:
					strcpy(line_3_2,"�����豸");
					break;
				default:
					break;
			}
			Display_Line_Index_16x16(Page_4,16 * 4,line_3_2,DISPLAY_POSITIVE);
		}
}

void DisplayInqFullScale(void)
{
	char line_1[] = "������������ֵ��";
	char line_2[] = "         KG      ";
	char fullscale[] = "";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4,0,line_2,DISPLAY_POSITIVE);
	sprintf(fullscale,"%03d",tension_max_range / 10);
	Display_Line_Index_8x16(Page_4,8*5,fullscale,DISPLAY_POSITIVE);
}

void DisplayInqBaseAutoCalibrateTime(void)
{
	char line_1[] = "����׼��Уʱ�䡷";
	char fenzhong[] = "����";
	char calibratetime[] = "";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,5*16,fenzhong,DISPLAY_POSITIVE);
	sprintf(calibratetime,"%05d",base_auto_calibrate_time);
	Display_Line_Index_8x16(Page_4,8*3,calibratetime,DISPLAY_POSITIVE);
}

void DiaplayAFangQuMonitor(void)
{
	char line_1[] = "A ����������Kg��";
	char line_2[] = "L1:  .  L2:  . ";
	char line_3[] = "L3:  .  L4:  . ";
	char line_4[] = "L5:  .  L6:  . ";
	
	char wire_1[3] = "";
	char wire_1_dot[2] = "";
	char wire_2[3] = "";
	char wire_2_dot[2] = "";
	char wire_3[3] = "";
	char wire_3_dot[2] = "";
	char wire_4[3] = "";
	char wire_4_dot[2] = "";
	char wire_5[3] = "";
	char wire_5_dot[2] = "";
	char wire_6[3] = "";
	char wire_6_dot[2] = "";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_8x16_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_16x16(Page_2,0,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,0,line_3,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_6,0,line_4,DISPLAY_POSITIVE);
	
	sprintf(wire_1,"%02d",crt_val[0][0] / 10);
	sprintf(wire_1_dot,"%01d",crt_val[0][0] % 10);
	sprintf(wire_2,"%02d",crt_val[0][1] / 10);
	sprintf(wire_2_dot,"%01d",crt_val[0][1] % 10);
	sprintf(wire_3,"%02d",crt_val[0][2] / 10);
	sprintf(wire_3_dot,"%01d",crt_val[0][2] % 10);
	sprintf(wire_4,"%02d",crt_val[0][3] / 10);
	sprintf(wire_4_dot,"%01d",crt_val[0][3] % 10);
	sprintf(wire_5,"%02d",crt_val[0][4] / 10);
	sprintf(wire_5_dot,"%01d",crt_val[0][4] % 10);
	sprintf(wire_6,"%02d",crt_val[0][5] / 10);
	sprintf(wire_6_dot,"%01d",crt_val[0][5] % 10);
	
	Display_Line_Index_8x16(Page_2, 8 * 3, wire_1, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 6, wire_1_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 11, wire_2, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 14, wire_2_dot, DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_4, 8 * 3, wire_3, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 6, wire_3_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 11, wire_4, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 14, wire_4_dot, DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_6, 8 * 3, wire_5, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 6, wire_5_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 11, wire_6, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 14, wire_6_dot, DISPLAY_POSITIVE);
}

void DiaplayBFangQuMonitor(void)
{
	char line_1[] = "B ����������Kg��";
	char line_2[] = "L1:  .  L2:  . ";
	char line_3[] = "L3:  .  L4:  . ";
	char line_4[] = "L5:  .  L6:  . ";
	
	char wire_1[3] = "";
	char wire_1_dot[2] = "";
	char wire_2[3] = "";
	char wire_2_dot[2] = "";
	char wire_3[3] = "";
	char wire_3_dot[2] = "";
	char wire_4[3] = "";
	char wire_4_dot[2] = "";
	char wire_5[3] = "";
	char wire_5_dot[2] = "";
	char wire_6[3] = "";
	char wire_6_dot[2] = "";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_8x16_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_16x16(Page_2,0,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_4,0,line_3,DISPLAY_POSITIVE);
	Display_Line_Index_8x16_16x16(Page_6,0,line_4,DISPLAY_POSITIVE);
	
	sprintf(wire_1,"%02d",crt_val[1][0] / 10);
	sprintf(wire_1_dot,"%01d",crt_val[1][0] % 10);
	sprintf(wire_2,"%02d",crt_val[1][1] / 10);
	sprintf(wire_2_dot,"%01d",crt_val[1][1] % 10);
	sprintf(wire_3,"%02d",crt_val[1][2] / 10);
	sprintf(wire_3_dot,"%01d",crt_val[1][2] % 10);
	sprintf(wire_4,"%02d",crt_val[1][3] / 10);
	sprintf(wire_4_dot,"%01d",crt_val[1][3] % 10);
	sprintf(wire_5,"%02d",crt_val[1][4] / 10);
	sprintf(wire_5_dot,"%01d",crt_val[1][4] % 10);
	sprintf(wire_6,"%02d",crt_val[1][5] / 10);
	sprintf(wire_6_dot,"%01d",crt_val[1][5] % 10);
	
	Display_Line_Index_8x16(Page_2, 8 * 3, wire_1, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 6, wire_1_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 11, wire_2, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2, 8 * 14, wire_2_dot, DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_4, 8 * 3, wire_3, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 6, wire_3_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 11, wire_4, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4, 8 * 14, wire_4_dot, DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_6, 8 * 3, wire_5, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 6, wire_5_dot, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 11, wire_6, DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_6, 8 * 14, wire_6_dot, DISPLAY_POSITIVE);
}
/****************************************************************/
/****************************�����˵�****************************/

char rtcTempStr_temp_test[19] = "";
void DisplaySetDateTime(void)
{
	char line_1[] = "������ʱ�䡷";
	unsigned char i = 0;
	unsigned char j = 0;
	unsigned char k = 0;

	Lcd_Clear(FullScreen);    //��ȫ��
	//------����ʱ��------
	//  2018-03-31 16:27:00
	//  2018-03-31
	//  13:40:00
	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	if(flag_PageState)//������ʾʱ����Ҫ��ʾrtcTempStr������
		{
//			Display_Line_Index_8x16 (Page_2,16 * 1,line_3,DISPLAY_POSITIVE);
			for(i=0;i<10;i++)//�����գ�10���ַ�
			{
				for(j=0;j<sizeof(zifu) / 17;j++)
				{
					if(rtcTempStr[i] == zifu[j * 17])
						{
							k = i + 3;//Ϊ�˾�����ʾ
							if(k<8)
								{
									Display_8x16(LeftScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
								}
							else
								{
									Display_8x16(RighScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
								}
						}
				}
			}
			for(i=0;i<8;i++)//ʱ���룬8���ַ�
			{
				for(j=0;j<sizeof(zifu) / 17;j++)
				{
					if(rtcTempStr[i + 11] == zifu[j * 17])
						{
							k = i + 4;//Ϊ�˾�����ʾ
							if(k<8)
								{
									Display_8x16(LeftScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
								}
							else
								{
									Display_8x16(RighScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
								}
						}
				}
			}
		}
	else//�༭��ʾʱ����Ҫ��ʾrtcTempStr_temp������
		{
			switch(row_point)
			{
				case 1://��������Ҫ�༭
					for(i=0;i<10;i++)//�����գ�10���ַ�����Ҫ�༭��
					{
						for(j=0;j<sizeof(zifu) / 17;j++)
						{
							if(rtcTempStr_temp_test[i] == zifu[j * 17])
								{
									k = i + 3;//Ϊ�˾�����ʾ
									if(k<8)
										{
											if(row_fx_pos == 1)//��һ��λ����Ҫ������ʾ
												{
													if((k == 5) || (k == 6))//����ĺ���λ������ʾ
														{
															Display_8x16(LeftScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_NEGATIVE);
														}
													else//����������ʾ
														{
															Display_8x16(LeftScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
														}
												}
											else//��һ��λ�ò���Ҫ������ʾ
												{
													Display_8x16(LeftScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
												}
										}
									else
										{
											if(row_fx_pos == 2)//�ڶ���λ����Ҫ������ʾ
												{
													if((k == 8) || (k == 9))//���·�����ʾ
														{
															Display_8x16(RighScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_NEGATIVE);
														}
													else//����������ʾ
														{
															Display_8x16(RighScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
														}
												}
											else if(row_fx_pos == 3)//������λ����Ҫ������ʾ
												{
													if((k == 11) || (k == 12))//���շ�����ʾ
														{
															Display_8x16(RighScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_NEGATIVE);
														}
													else//����������ʾ
														{
															Display_8x16(RighScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
														}
												}
											else//������Ҫ������ʾ
												{
													Display_8x16(RighScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
												}
										}
								}
						}
					}
					for(i=0;i<8;i++)//ʱ���룬8���ַ�������Ҫ�༭��
					{
						for(j=0;j<sizeof(zifu) / 17;j++)
						{
							if(rtcTempStr_temp_test[i + 11] == zifu[j * 17])
								{
									k = i + 4;//Ϊ�˾�����ʾ
									if(k<8)
										{
											Display_8x16(LeftScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
										}
									else
										{
											Display_8x16(RighScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
										}
								}
						}
					}
					break;
				case 2://ʱ������Ҫ�༭
					for(i=0;i<10;i++)//�����գ�10���ַ�������Ҫ�༭��
					{
						for(j=0;j<sizeof(zifu) / 17;j++)
						{
							if(rtcTempStr_temp_test[i] == zifu[j * 17])
								{
									k = i + 3;//Ϊ�˾�����ʾ
									if(k<8)
										{
											Display_8x16(LeftScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
										}
									else
										{
											Display_8x16(RighScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
										}
								}
						}
					}
					for(i=0;i<8;i++)//ʱ���룬8���ַ�����Ҫ�༭��
					{
						for(j=0;j<sizeof(zifu) / 17;j++)
						{
							if(rtcTempStr_temp_test[i + 11] == zifu[j * 17])
								{
									k = i + 4;//Ϊ�˾�����ʾ
									if(k<8)//�����
										{
											if(row_fx_pos == 1)//��һ��λ����Ҫ������ʾ
												{
													if((k == 4) || (k == 5))//��ʱ������ʾ
														{
															Display_8x16(LeftScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_NEGATIVE);
														}
													else//����������ʾ
														{
															Display_8x16(LeftScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
														}
												}
											else if(row_fx_pos == 2)//�ڶ���λ����Ҫ������ʾ
												{
													if(k == 7)//���ӵ�ʮλ������ʾ
														{
															Display_8x16(LeftScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_NEGATIVE);
														}
													else//����������ʾ
														{
															Display_8x16(LeftScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
														}
												}
											else//������Ҫ������ʾ
												{
													Display_8x16(LeftScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
												}
										}
									else
										{
											if(row_fx_pos == 2)//�ڶ���λ����Ҫ������ʾ
												{
													if(k == 8)//�����ӵĸ�λ������ʾ
														{
															Display_8x16(RighScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_NEGATIVE);
														}
													else//����������ʾ
														{
															Display_8x16(RighScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
														}
												}
											else if(row_fx_pos == 3)//������λ����Ҫ������ʾ
												{
													if((k == 10) || (k == 11))//���뷴����ʾ
														{
															Display_8x16(RighScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_NEGATIVE);
														}
													else//����������ʾ
														{
															Display_8x16(RighScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
														}
												}
											else//������Ҫ������ʾ
												{
													Display_8x16(RighScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
												}
										}
								}
						}
					}
					break;
				default:
					break;
			}
		}
}

void DisplaySetEthPara(void)
{
	char line_1[] = "���������������";
	char line_2[] = "MAC";
	char line_3[] = "IP";
	char dizhi[] = "��ַ";
	char line_4[] = "��������";
	char line_5[] = "����";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	
	if(row_point > 3)
		{
			Display_Line_Index_8x16 (Page_2,16 * 1,line_3,DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_2,16 * 3,dizhi,DISPLAY_POSITIVE);
			
			Display_Line_Index_16x16(Page_4,16 * 1,line_4,DISPLAY_POSITIVE);
			
			Display_Line_Index_16x16(Page_6,16 * 1,line_5,DISPLAY_POSITIVE);
		}
	else
		{
			Display_Line_Index_8x16 (Page_2,16 * 1,line_2,DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_2,16 * 3,dizhi,DISPLAY_POSITIVE);
			
			Display_Line_Index_8x16 (Page_4,16 * 1,line_3,DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 3,dizhi,DISPLAY_POSITIVE);
			
			Display_Line_Index_16x16(Page_6,16 * 1,line_4,DISPLAY_POSITIVE);
		}
}

//void DisplaySetRS485Para(void)
//{
//	unsigned char i;
//	unsigned char j;
//	unsigned char k;
//	Lcd_Clear(FullScreen);    //��ȫ��
//	//��RS485������
//	//������ַ:��01-32��
//	//������:��2400��4800��9600��19200��38400��
//	Display_16x16(LeftScreen,Page_0,1*16,zuo_kuo_hao,DISPLAY_POSITIVE);
//	Display_Line_8x16(Page_0,2*16,RS485_CanShu,(sizeof(RS485_CanShu) - 64));
//	Display_Line_16x16(Page_0,5*16,(RS485_CanShu + 80),64,DISPLAY_POSITIVE);
//	Display_16x16(RighScreen,Page_0,7*16,you_kuo_hao,DISPLAY_POSITIVE);
//	
//	Display_Line_16x16(Page_2,1*16,BenJiDiZhi,sizeof(BenJiDiZhi),DISPLAY_POSITIVE);
//	Display_8x16(RighScreen, Page_2, 5*16, mao_hao, DISPLAY_POSITIVE);
//	
//	Display_Line_16x16(Page_4,1*16,BoTeLv,sizeof(BoTeLv),DISPLAY_POSITIVE);
//	Display_8x16(RighScreen, Page_4, 4*16, mao_hao, DISPLAY_POSITIVE);
//	
//	if(flag_PageState)//������ʾʱ����Ҫ��ʾequAddrStr������
//		{
//			for(i=0;i<2;i++)//������ַ:��01-32����2���ַ�
//			{
//				for(j=0;j<sizeof(zifu) / 17;j++)
//				{
//					if(equAddrStr[i] == zifu[j * 17])
//						{
//							k = i + 12;//
//							Display_8x16(RighScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
//						}
//				}
//			}
//			for(i=0;i<5;i++)//������:��2400��4800��9600��19200��38400����5���ַ�
//			{
//				for(j=0;j<sizeof(zifu) / 17;j++)
//				{
//					if(baudStr[rs485BaudSel][i] == zifu[j * 17])
//						{
//							k = i + 10;//
//							Display_8x16(RighScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
//						}
//				}
//			}
//		}
//	else//�༭��ʾʱ����Ҫ��ʾequAddrStr_temp������
//		{
//			switch(row_point)
//			{
//				case 1://������ַ��Ҫ�༭
//					for(i=0;i<2;i++)//������ַ��2���ַ�����Ҫ�༭��
//					{
//						for(j=0;j<sizeof(zifu) / 17;j++)
//						{
//							if(equAddrStr_temp[i] == zifu[j * 17])
//								{
//									k = i + 12;//
//									Display_8x16(RighScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_NEGATIVE);
//								}
//						}
//					}
//					for(i=0;i<5;i++)//������:��2400��4800��9600��19200��38400����5���ַ�������Ҫ�༭��
//					{
//						for(j=0;j<sizeof(zifu) / 17;j++)
//						{
//							if(baudStr[rs485BaudSel][i] == zifu[j * 17])
//								{
//									k = i + 10;//
//									Display_8x16(RighScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
//								}
//						}
//					}
//					break;
//				case 2://��������Ҫ�༭
//					for(i=0;i<2;i++)//������ַ:��01-32����2���ַ�������Ҫ�༭��
//					{
//						for(j=0;j<sizeof(zifu) / 17;j++)
//						{
//							if(equAddrStr[i] == zifu[j * 17])
//								{
//									k = i + 12;//
//									Display_8x16(RighScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
//								}
//						}
//					}
//					for(i=0;i<5;i++)//�����ʣ�5���ַ�����Ҫ�༭��
//					{
//						for(j=0;j<sizeof(zifu) / 17;j++)
//						{
//							if(baudStr[rs485BaudSel_temp][i] == zifu[j * 17])
//								{
//									k = i + 10;//
//									Display_8x16(RighScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_NEGATIVE);
//								}
//						}
//					}
//					break;
//				default:
//					break;
//			}
//		}
//}
//
void DisplayClearRuQinAlarmRecord(void)
{
	char line_1[] = "��������ָ澯��";
	char char_A[] = "A";
	char char_B[] = "B";
	char hanzi_qu[] = "��";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 0,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_2,16 * 1,char_A,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_2,16 * 2,hanzi_qu,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_4,16 * 1,char_B,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,16 * 2,hanzi_qu,DISPLAY_POSITIVE);
}

void DisplayClearDuanXianAlarmRecord(void)
{
	char line_1[] = "�����ƫ�Ƹ澯��";
	char char_A[] = "A";
	char char_B[] = "B";
	char hanzi_qu[] = "��";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 0,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_2,16 * 1,char_A,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_2,16 * 2,hanzi_qu,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_4,16 * 1,char_B,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,16 * 2,hanzi_qu,DISPLAY_POSITIVE);
}

void DisplayInqEthPara(void)
{
	char line_1[] = "����ѯ���������";
	char line_2[] = "MAC";
	char line_3[] = "IP";
	char dizhi[] = "��ַ";
	char line_4[] = "��������";
	char line_5[] = "����";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	
	if(row_point > 3)
		{
			Display_Line_Index_8x16 (Page_2,16 * 1,line_3,DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_2,16 * 3,dizhi,DISPLAY_POSITIVE);
			
			Display_Line_Index_16x16(Page_4,16 * 1,line_4,DISPLAY_POSITIVE);
			
			Display_Line_Index_16x16(Page_6,16 * 1,line_5,DISPLAY_POSITIVE);
		}
	else
		{
			Display_Line_Index_8x16 (Page_2,16 * 1,line_2,DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_2,16 * 3,dizhi,DISPLAY_POSITIVE);
			
			Display_Line_Index_8x16 (Page_4,16 * 1,line_3,DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 3,dizhi,DISPLAY_POSITIVE);
			
			Display_Line_Index_16x16(Page_6,16 * 1,line_4,DISPLAY_POSITIVE);
		}
}

//void DisplayInqRS485Para(void)
//{
//	unsigned char i;
//	unsigned char j;
//	unsigned char k;
//	Lcd_Clear(FullScreen);    //��ȫ��
//	//��RS485������
//	//������ַ:��01-32��
//	//������:��2400��4800��9600��19200��38400��
//	Display_16x16(LeftScreen,Page_0,1*16,zuo_kuo_hao,DISPLAY_POSITIVE);
//	Display_Line_8x16(Page_0,2*16,RS485_CanShu,(sizeof(RS485_CanShu) - 64));
//	Display_Line_16x16(Page_0,5*16,(RS485_CanShu + 80),64,DISPLAY_POSITIVE);
//	Display_16x16(RighScreen,Page_0,7*16,you_kuo_hao,DISPLAY_POSITIVE);
//	
//	Display_Line_16x16(Page_2,1*16,BenJiDiZhi,sizeof(BenJiDiZhi),DISPLAY_POSITIVE);
//	Display_8x16(RighScreen, Page_2, 5*16, mao_hao, DISPLAY_POSITIVE);
//	
//	Display_Line_16x16(Page_4,1*16,BoTeLv,sizeof(BoTeLv),DISPLAY_POSITIVE);
//	Display_8x16(RighScreen, Page_4, 4*16, mao_hao, DISPLAY_POSITIVE);
//	
//	for(i=0;i<2;i++)//������ַ:��01-32����2���ַ�
//	{
//		for(j=0;j<sizeof(zifu) / 17;j++)
//		{
//			if(equAddrStr[i] == zifu[j * 17])
//				{
//					k = i + 12;//
//					Display_8x16(RighScreen, Page_2, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
//				}
//		}
//	}
//	for(i=0;i<5;i++)//������:��2400��4800��9600��19200��38400����5���ַ�
//	{
//		for(j=0;j<sizeof(zifu) / 17;j++)
//		{
//			if(baudStr[rs485BaudSel][i] == zifu[j * 17])
//				{
//					k = i + 10;//
//					Display_8x16(RighScreen, Page_4, k*8, zifu + j *17 + 1, DISPLAY_POSITIVE);
//				}
//		}
//	}
//}

void DisplayInqRuQinAlarmRecord(void)
{
	char line_1[] = "�����ָ澯��";
	char char_A[] = "A";
	char char_B[] = "B";
	char hanzi_qu[] = "��";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_2,16 * 1,char_A,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_2,16 * 2,hanzi_qu,DISPLAY_POSITIVE);
	
	if(FIELD_FLAG_S0D1)
		{
			//����������ʾB����
		}
	else
		{
			Display_Line_Index_8x16(Page_4,16 * 1,char_B,DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 2,hanzi_qu,DISPLAY_POSITIVE);
		}
}

void DisplayInqDuanXianAlarmRecord(void)
{
	char line_1[] = "��ƫ�Ƹ澯��";
	char char_A[] = "A";
	char char_B[] = "B";
	char hanzi_qu[] = "��";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_2,16 * 1,char_A,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_2,16 * 2,hanzi_qu,DISPLAY_POSITIVE);
	
	if(FIELD_FLAG_S0D1)
		{
			//����������ʾB����
		}
	else
		{
			Display_Line_Index_8x16(Page_4,16 * 1,char_B,DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,16 * 2,hanzi_qu,DISPLAY_POSITIVE);
		}
}

void DisplayInqFangChaiAlarmRecord(void)
{
	char line_1[] = "������澯��";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);

	UpdateAlarmRecord();
}
/****************************************************************/
/****************************�ļ��˵�****************************/
void DisplaySetMacAddr(void)
{
	char line_1_1[] = "��    ��ַ��";
	char line_1_2[] = "MAC";
	char line_2[9] = " ";
	char line_3[9] = " ";
	char line_4[] = "��ֻ����";
	
	MacAddrInsertMaohao();
	
	strncpy(line_2,eth_mac_addr,8);
	strncpy(line_3,eth_mac_addr + 9,8);
	line_2[8] = '\0';
	line_3[8] = '\0';
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_0,16 * 1,line_1_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_0,16 * 2,line_1_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_2,16 * 2,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4,16 * 2,line_3,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_6,16 * 2,line_4,DISPLAY_POSITIVE);
}

void DisplaySetIpAddr(void)
{
	char line_1_1[] = "��    ��ַ��";
	char line_1_2[] = "IP";
	char line_3[16] = " ";
	
	memcpy(line_3,eth_ip_addr,sizeof(eth_ip_addr));
	line_3[15] = '\0';
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_0,16 * 1,line_1_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_0,16 * 2,line_1_2,DISPLAY_POSITIVE);
	
	switch(row_fx_pos)
	{
		case 1:
		case 2:
		case 3:
			Display_Line_Index_8x16_pn(Page_4,0,line_3,row_fx_pos - 1);
			break;
		case 5:
		case 6:
		case 7:
			Display_Line_Index_8x16_pn(Page_4,0,line_3,row_fx_pos - 1);
			break;
		case 9:
		case 10:
		case 11:
			Display_Line_Index_8x16_pn(Page_4,0,line_3,row_fx_pos - 1);
			break;
		case 13:
		case 14:
		case 15:
			Display_Line_Index_8x16_pn(Page_4,0,line_3,row_fx_pos - 1);
			break;
		
		default:
			break;
	}
}

void DisplaySetSubnetMask(void)
{
	char line_1[] = "���������롷";
	char line_3[16] = " ";
	
	memcpy(line_3,eth_subnet_mask,sizeof(eth_subnet_mask));
	line_3[15] = '\0';
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	switch(row_fx_pos)
	{
		case 1:
		case 2:
		case 3:
			Display_Line_Index_8x16_pn(Page_4,0,line_3,row_fx_pos - 1);
			break;
		case 5:
		case 6:
		case 7:
			Display_Line_Index_8x16_pn(Page_4,0,line_3,row_fx_pos - 1);
			break;
		case 9:
		case 10:
		case 11:
			Display_Line_Index_8x16_pn(Page_4,0,line_3,row_fx_pos - 1);
			break;
		case 13:
		case 14:
		case 15:
			Display_Line_Index_8x16_pn(Page_4,0,line_3,row_fx_pos - 1);
			break;
		
		default:
			break;
	}
}

void DisplaySetGateWay(void)
{
	char line_1[] = "�����ء�";
	char line_3[16] = " ";
	
	memcpy(line_3,eth_gateway,sizeof(eth_gateway));
	line_3[15] = '\0';
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_16x16(Page_0,16 * 2,line_1,DISPLAY_POSITIVE);
	switch(row_fx_pos)
	{
		case 1:
		case 2:
		case 3:
			Display_Line_Index_8x16_pn(Page_4,0,line_3,row_fx_pos - 1);
			break;
		case 5:
		case 6:
		case 7:
			Display_Line_Index_8x16_pn(Page_4,0,line_3,row_fx_pos - 1);
			break;
		case 9:
		case 10:
		case 11:
			Display_Line_Index_8x16_pn(Page_4,0,line_3,row_fx_pos - 1);
			break;
		case 13:
		case 14:
		case 15:
			Display_Line_Index_8x16_pn(Page_4,0,line_3,row_fx_pos - 1);
			break;
		
		default:
			break;
	}
}

void DisplayInqMacAddr(void)
{
	char line_1_1[] = "��    ��ַ��";
	char line_1_2[] = "MAC";
	char line_2[9] = " ";
	char line_3[9] = " ";
	
	MacAddrInsertMaohao();
	
	strncpy(line_2,eth_mac_addr,8);
	strncpy(line_3,eth_mac_addr + 9,8);
	line_2[8] = '\0';
	line_3[8] = '\0';
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_0,16 * 1,line_1_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_0,16 * 2,line_1_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_3,16 * 2,line_2,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_5,16 * 2,line_3,DISPLAY_POSITIVE);
}

void DisplayInqIpAddr(void)
{
	char line_1_1[] = "��    ��ַ��";
	char line_1_2[] = "IP";
	char line_3[16] = " ";
	
	memcpy(line_3,eth_ip_addr,sizeof(eth_ip_addr));
	line_3[15] = '\0';
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_0,16 * 1,line_1_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_0,16 * 2,line_1_2,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_4,0,line_3,DISPLAY_POSITIVE);
}

void DisplayInqSubnetMask(void)
{
	char line_1[] = "���������롷";
	char line_3[16] = " ";
	
	memcpy(line_3,eth_subnet_mask,sizeof(eth_subnet_mask));
	line_3[15] = '\0';
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_16x16(Page_0,16 * 1,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4,0,line_3,DISPLAY_POSITIVE);
}

void DisplayInqGateWay(void)
{
	char line_1[] = "�����ء�";
	char line_3[16] = " ";
	
	memcpy(line_3,eth_gateway,sizeof(eth_gateway));
	line_3[15] = '\0';
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_16x16(Page_0,16 * 2,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4,0,line_3,DISPLAY_POSITIVE);
}

void UpdateAlarmRecord(void)
{
	crtLCDPageLogData[0][16] = '\0';
	crtLCDPageLogData[1][16] = '\0';
	crtLCDPageLogData[2][16] = '\0';
	Display_Line_Index_8x16 (Page_2,0,crtLCDPageLogData[0],DISPLAY_POSITIVE);
	Display_Line_Index_8x16 (Page_4,0,crtLCDPageLogData[1],DISPLAY_POSITIVE);
	Display_Line_Index_8x16 (Page_6,0,crtLCDPageLogData[2],DISPLAY_POSITIVE);
}

void DisplayInqAAreaRuQinAlarmRecord(void)
{
	//��A�����ָ澯��
	//��һ��
	//�ڶ���
	//...
	char line_1[] = "��  �����ָ澯��";
	char char_A[] = "A";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 0,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_0,16 * 1,char_A,DISPLAY_POSITIVE);

	UpdateAlarmRecord();
}

void DisplayInqBAreaRuQinAlarmRecord(void)
{
	//��B�����ָ澯��
	//��һ��
	//�ڶ���
	//...
	char line_1[] = "��  �����ָ澯��";
	char char_B[] = "B";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 0,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_0,16 * 1,char_B,DISPLAY_POSITIVE);

	UpdateAlarmRecord();
}

void DisplayInqAAreaDuanXianAlarmRecord(void)
{
	//��A��ƫ�Ƹ澯��
	//��һ��
	//�ڶ���
	//...
	char line_1[] = "��  ��ƫ�Ƹ澯��";
	char char_A[] = "A";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 0,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_0,16 * 1,char_A,DISPLAY_POSITIVE);

	UpdateAlarmRecord();
}

void DisplayInqBAreaDuanXianAlarmRecord(void)
{
	char line_1[] = "��  ��ƫ�Ƹ澯��";
	char char_B[] = "B";
	
	Lcd_Clear(FullScreen);    //��ȫ��

	Display_Line_Index_16x16(Page_0,16 * 0,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_0,16 * 1,char_B,DISPLAY_POSITIVE);

	UpdateAlarmRecord();
}




unsigned char password_en_dis = passwordEnable;
char lcd_super_password[] = "123456";
char lcd_ordinary_password[] = "000000";
char inputPassword[] = "0$$$$$";
char display_xinghao[] = " $$$$$";
unsigned char validPasswordCnt = 0;
void DisplayInputPassword(void)
{
	char line_1[] = "���������룺";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_1,16,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_4,8*5,inputPassword,DISPLAY_NEGATIVE);
}

void DisplayPasswordError(void)
{
	char line_1[] = "�������";
	char line_2[] = "���������룡";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_2,16 * 2,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,16,line_2,DISPLAY_POSITIVE);
}

unsigned char ascii_data[2];
void HEX2ASCII(unsigned char hex_data)
{
//	unsigned char ascii_data[2];
	ascii_data[1] = hex_data & 0xf0;//��4λ
	ascii_data[1] >>= 4;
	ascii_data[0] = hex_data & 0x0f;//��4λ
	
	ascii_data[1] = HEX2ASCII_2(ascii_data[1]);
	ascii_data[0] = HEX2ASCII_2(ascii_data[0]);
}

unsigned char HEX2ASCII_2(unsigned char tn_data)
{
	unsigned char half_data;
	half_data = tn_data;
	if(half_data > 9)
		{
			half_data += 0x37;//A->Z
		}
	else
		{
			half_data += 0x30;//0->9
		}
	return half_data;
}

char eth_mac_addr[18];//��MAC��ַ�в���ð��
void MacAddrInsertMaohao(void)
{
	unsigned char i;
	for(i=0;i<sizeof(lwipdev.mac);i++)//12���ַ�,6 byte
	{
		HEX2ASCII(lwipdev.mac[i]);
		eth_mac_addr[i * 3 + 2] = ':';
		eth_mac_addr[i * 3 + 1] = ascii_data[0];
		eth_mac_addr[i * 3] = ascii_data[1];
	}
}

char eth_ip_addr[16];
void GetIpAddr(void)
{
	unsigned char i;
	for(i=0;i<sizeof(lwipdev.ip);i++)//4 byte
	{
		eth_ip_addr[i * 4] = 0x30 + lwipdev.ip[i] / 100;
		eth_ip_addr[i * 4 + 1] = 0x30 + (lwipdev.ip[i] % 100) / 10;
		eth_ip_addr[i * 4 + 2] = 0x30 + (lwipdev.ip[i] % 100) % 10;
		eth_ip_addr[i * 4 + 3] = '.';
	}
}

void SetIpAddr(void)
{
	lwipdev.ip[0] = (eth_ip_addr[0] - 0x30) * 100 + (eth_ip_addr[1] - 0x30) * 10 + (eth_ip_addr[2] - 0x30);
	lwipdev.ip[1] = (eth_ip_addr[4] - 0x30) * 100 + (eth_ip_addr[5] - 0x30) * 10 + (eth_ip_addr[6] - 0x30);
	lwipdev.ip[2] = (eth_ip_addr[8] - 0x30) * 100 + (eth_ip_addr[9] - 0x30) * 10 + (eth_ip_addr[10] - 0x30);
	lwipdev.ip[3] = (eth_ip_addr[12] - 0x30) * 100 + (eth_ip_addr[13] - 0x30) * 10 + (eth_ip_addr[14] - 0x30);
}

char eth_subnet_mask[16];
void GetSubNetMask(void)
{
	unsigned char i;
	for(i=0;i<sizeof(lwipdev.netmask);i++)//4 byte
	{
		eth_subnet_mask[i * 4] = 0x30 + lwipdev.netmask[i] / 100;
		eth_subnet_mask[i * 4 + 1] = 0x30 + (lwipdev.netmask[i] % 100) / 10;
		eth_subnet_mask[i * 4 + 2] = 0x30 + (lwipdev.netmask[i] % 100) % 10;
		eth_subnet_mask[i * 4 + 3] = '.';
	}
}

void SetSubNetMask(void)
{
	lwipdev.netmask[0] = (eth_subnet_mask[0] - 0x30) * 100 + (eth_subnet_mask[1] - 0x30) * 10 + (eth_subnet_mask[2] - 0x30);
	lwipdev.netmask[1] = (eth_subnet_mask[4] - 0x30) * 100 + (eth_subnet_mask[5] - 0x30) * 10 + (eth_subnet_mask[6] - 0x30);
	lwipdev.netmask[2] = (eth_subnet_mask[8] - 0x30) * 100 + (eth_subnet_mask[9] - 0x30) * 10 + (eth_subnet_mask[10] - 0x30);
	lwipdev.netmask[3] = (eth_subnet_mask[12] - 0x30) * 100 + (eth_subnet_mask[13] - 0x30) * 10 + (eth_subnet_mask[14] - 0x30);
}

char eth_gateway[16];
void GetGateWay(void)
{
	unsigned char i;
	for(i=0;i<sizeof(lwipdev.gateway);i++)//4 byte
	{
		eth_gateway[i * 4] = 0x30 + lwipdev.gateway[i] / 100;
		eth_gateway[i * 4 + 1] = 0x30 + (lwipdev.gateway[i] % 100) / 10;
		eth_gateway[i * 4 + 2] = 0x30 + (lwipdev.gateway[i] % 100) % 10;
		eth_gateway[i * 4 + 3] = '.';
	}
}

void SetGateWay(void)
{
	lwipdev.gateway[0] = (eth_gateway[0] - 0x30) * 100 + (eth_gateway[1] - 0x30) * 10 + (eth_gateway[2] - 0x30);
	lwipdev.gateway[1] = (eth_gateway[4] - 0x30) * 100 + (eth_gateway[5] - 0x30) * 10 + (eth_gateway[6] - 0x30);
	lwipdev.gateway[2] = (eth_gateway[8] - 0x30) * 100 + (eth_gateway[9] - 0x30) * 10 + (eth_gateway[10] - 0x30);
	lwipdev.gateway[3] = (eth_gateway[12] - 0x30) * 100 + (eth_gateway[13] - 0x30) * 10 + (eth_gateway[14] - 0x30);
}

s8 alarm_rol = 0;
u8 alarm_type_cnt = 0;//�澯��������
s8 alarm_key_adj = 0;//���澯���ʹ���3��ʱ��ͨ��������������ʾ��Ŀ

void DisplayAlarmTypeList(void)
{

	//���澯��Ϣ�б�������3���Ŀ�ͨ�����ϡ��¼��ɲ鿴��
	//A��������
	//A����ƫ��
	//B��������
	//B����ƫ��
	//����澯
	//A���������
	//B���������
	char line_1[] = "���澯��Ϣ�б�";
	char char_A[] = "A";
	char char_B[] = "B";
	char qu[] = "��";
	char fangquduanluAlarm[] = "�������ָ澯";
	char fangquduanxianAlarm[] = "����ƫ�Ƹ澯";
	char fangqusongchiAlarm[] = "�����ɳڸ澯";
	char fangchaiAlarm[] = "����澯";
	char sw1Name[] = "������1";
	char sw2Name[] = "������2";
	char zhudonghongwaiAlarm[] = "��������澯";
	char beidonghongwaiAlarm[] = "��������澯";
	char shuangjianAlarm[] = "˫��̽��澯";
	char huanjingAlarm[] = "����̽��澯";
	char menciAlarm[] = "�Ŵ�̽��澯";
	char jinjianniuAlarm[] = "������ť�澯";
	char shuijinAlarm[] = "ˮ��̽��澯";
	char qitashebeiAlarm[] = "�����豸�澯";
	s8 i;
	static unsigned char alarm_sequence_buf[10] = {0};
	unsigned char dis_buf[3] = {0x02,0x04,0x06};
	
//	cur_Alarm_Type_Sum |= ALARM_STATE;
	cur_Alarm_Type_Sum = IS_SET_BIT(config_code, 2)? ALARM_STATE : ALARM_STATE_DELAY;  //ֻ��¼��ǰ�澯��
	
	Lcd_Clear(FullScreen);    //��ȫ��
	Display_Line_Index_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	

	if(old_Alarm_Type_Sum != cur_Alarm_Type_Sum)
		{
			alarm_type_cnt = 0;
			for(i = 9; i >= 0; i--){
				if(((cur_Alarm_Type_Sum >> i) & 0x01) == 0x01 ){
					alarm_sequence_buf[alarm_type_cnt] = i;
					alarm_type_cnt ++;
				}
			}

			old_Alarm_Type_Sum = cur_Alarm_Type_Sum;
		}
	else
		{
			alarm_rol = alarm_type_cnt - 1;
			if(alarm_type_cnt < 4)
				{
					for(i=alarm_rol; i>=0; i--)
					{
						switch(alarm_sequence_buf[i])
						{
							case 0x00:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i],0,char_A,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i],1 * 16,fangquduanluAlarm,DISPLAY_POSITIVE);
								break;
							case 0x01:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i],0,char_A,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i],1 * 16,fangquduanxianAlarm,DISPLAY_POSITIVE);
								break;
							case 0x02:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i],0,char_B,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i],1 * 16,fangquduanluAlarm,DISPLAY_POSITIVE);
								break;
							case 0x03:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i],0,char_B,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i],1 * 16,fangquduanxianAlarm,DISPLAY_POSITIVE);
								break;
							case 0x04:
								Display_Line_Index_16x16(dis_buf[alarm_rol - i],0 * 16,fangchaiAlarm,DISPLAY_POSITIVE);
								break;
							case 0x05:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i],0,char_A,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i],1 * 16,qu,DISPLAY_POSITIVE);
								switch(SW_Type[0])
								{
									case 0:
										Display_Line_Index_8x16_16x16(dis_buf[alarm_rol - i],2 * 16,sw1Name,DISPLAY_POSITIVE);
										break;
									case 1:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,zhudonghongwaiAlarm,DISPLAY_POSITIVE);
										break;
									case 2:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,beidonghongwaiAlarm,DISPLAY_POSITIVE);
										break;
									case 3:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,shuangjianAlarm,DISPLAY_POSITIVE);
										break;
									case 4:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,huanjingAlarm,DISPLAY_POSITIVE);
										break;
									case 5:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,menciAlarm,DISPLAY_POSITIVE);
										break;
									case 6:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,jinjianniuAlarm,DISPLAY_POSITIVE);
										break;
									case 7:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,shuijinAlarm,DISPLAY_POSITIVE);
										break;
									case 8:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,qitashebeiAlarm,DISPLAY_POSITIVE);
										break;
									default:
										break;
								}
								break;
							case 0x06:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i],0,char_B,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i],1 * 16,qu,DISPLAY_POSITIVE);
								switch(SW_Type[1])
								{
									case 0:
										Display_Line_Index_8x16_16x16(dis_buf[alarm_rol - i],2 * 16,sw2Name,DISPLAY_POSITIVE);
										break;
									case 1:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,zhudonghongwaiAlarm,DISPLAY_POSITIVE);
										break;
									case 2:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,beidonghongwaiAlarm,DISPLAY_POSITIVE);
										break;
									case 3:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,shuangjianAlarm,DISPLAY_POSITIVE);
										break;
									case 4:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,huanjingAlarm,DISPLAY_POSITIVE);
										break;
									case 5:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,menciAlarm,DISPLAY_POSITIVE);
										break;
									case 6:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,jinjianniuAlarm,DISPLAY_POSITIVE);
										break;
									case 7:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,shuijinAlarm,DISPLAY_POSITIVE);
										break;
									case 8:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i],2 * 16,qitashebeiAlarm,DISPLAY_POSITIVE);
										break;
									default:
										break;
								}
								break;
							case 0x08:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i],0,char_A,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i],1 * 16,fangqusongchiAlarm,DISPLAY_POSITIVE);
								break;
							case 0x09:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i],0,char_B,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i],1 * 16,fangqusongchiAlarm,DISPLAY_POSITIVE);
								break;

							default:
								break;
						}
					}
				}
			else
				{
					for(i=(alarm_rol - alarm_key_adj);i>(alarm_rol - alarm_key_adj) - 3;i--)//ͨ���ð�������alarm_key_adj���ֵ���ﵽ������ʾĿ��//(i=alarm_rol;i>alarm_rol - 3;i--)//����ʾ��ǰ����3���澯
					{
						switch(alarm_sequence_buf[i])
						{
							case 0x00:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i - alarm_key_adj],0,char_A,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],1 * 16,fangquduanluAlarm,DISPLAY_POSITIVE);
								break;
							case 0x01:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i - alarm_key_adj],0,char_A,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],1 * 16,fangquduanxianAlarm,DISPLAY_POSITIVE);
								break;
							case 0x02:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i - alarm_key_adj],0,char_B,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],1 * 16,fangquduanluAlarm,DISPLAY_POSITIVE);
								break;
							case 0x03:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i - alarm_key_adj],0,char_B,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],1 * 16,fangquduanxianAlarm,DISPLAY_POSITIVE);
								break;
							case 0x04:
								Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],0 * 16,fangchaiAlarm,DISPLAY_POSITIVE);
								break;
							case 0x05:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i - alarm_key_adj],0,char_A,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],1 * 16,qu,DISPLAY_POSITIVE);
								switch(SW_Type[0])
								{
									case 1:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,zhudonghongwaiAlarm,DISPLAY_POSITIVE);
										break;
									case 2:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,beidonghongwaiAlarm,DISPLAY_POSITIVE);
										break;
									case 3:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,shuangjianAlarm,DISPLAY_POSITIVE);
										break;
									case 4:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,huanjingAlarm,DISPLAY_POSITIVE);
										break;
									case 5:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,menciAlarm,DISPLAY_POSITIVE);
										break;
									case 6:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,jinjianniuAlarm,DISPLAY_POSITIVE);
										break;
									case 7:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,shuijinAlarm,DISPLAY_POSITIVE);
										break;
									case 8:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,qitashebeiAlarm,DISPLAY_POSITIVE);
										break;
									default:
										break;
								}
								break;
							case 0x06:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i - alarm_key_adj],0,char_B,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],1 * 16,qu,DISPLAY_POSITIVE);
								switch(SW_Type[1])
								{
									case 1:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,zhudonghongwaiAlarm,DISPLAY_POSITIVE);
										break;
									case 2:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,beidonghongwaiAlarm,DISPLAY_POSITIVE);
										break;
									case 3:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,shuangjianAlarm,DISPLAY_POSITIVE);
										break;
									case 4:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,huanjingAlarm,DISPLAY_POSITIVE);
										break;
									case 5:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,menciAlarm,DISPLAY_POSITIVE);
										break;
									case 6:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,jinjianniuAlarm,DISPLAY_POSITIVE);
										break;
									case 7:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,shuijinAlarm,DISPLAY_POSITIVE);
										break;
									case 8:
										Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],2 * 16,qitashebeiAlarm,DISPLAY_POSITIVE);
										break;
									default:
										break;
								}
								break;
							case 0x08:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i - alarm_key_adj],0,char_A,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],1 * 16,fangqusongchiAlarm,DISPLAY_POSITIVE);
								break;
							case 0x09:
								Display_Line_Index_8x16(dis_buf[alarm_rol - i - alarm_key_adj],0,char_B,DISPLAY_POSITIVE);
								Display_Line_Index_16x16(dis_buf[alarm_rol - i - alarm_key_adj],1 * 16,fangqusongchiAlarm,DISPLAY_POSITIVE);
								break;
							default:
								break;
						}
					}
				}
		}
}

void DiaplaySetExtDeviceTypeScreen(void)
{
	//�������ⲿ�豸��
	//  A��:   ��������
	//  B��:   ��������
	//  �������Ϳ�������
	char line_1[] = "�������ⲿ�豸��";
	char line_2[16];
	char line_3[16];
	char char_A[] = "A";
	char char_B[] = "B";
	char char_qu[] = "����";
	char line_4[] = "�������Ϳ�������";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16(Page_2,2*8,char_A,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_2,3 * 8,char_qu,DISPLAY_POSITIVE);
	
	if(FIELD_FLAG_S0D1)
		{
			//����������ʾB����
		}
	else
		{
			Display_Line_Index_8x16(Page_4,2*8,char_B,DISPLAY_POSITIVE);
			Display_Line_Index_16x16(Page_4,3 * 8,char_qu,DISPLAY_POSITIVE);
		}
	
	Display_Line_Index_16x16(Page_6,0,line_4,DISPLAY_POSITIVE);
	
	if(flag_PageState)//������ʾʱ����Ҫ��ʾflag_fcgjsn������
		{
			switch(SW_Type[0])
			{
				case 0:
					strcpy(line_2,"���豸");
					break;
				case 1:
					strcpy(line_2,"��������");
					break;
				case 2:
					strcpy(line_2,"��������");
					break;
				case 3:
					strcpy(line_2,"˫��̽��");
					break;
				case 4:
					strcpy(line_2,"����̽��");
					break;
				case 5:
					strcpy(line_2,"�Ŵ�̽��");
					break;
				case 6:
					strcpy(line_2,"������ť");
					break;
				case 7:
					strcpy(line_2,"ˮ��̽��");
					break;
				case 8:
					strcpy(line_2,"�����豸");
					break;
				default:
					break;
			}
			Display_Line_Index_16x16(Page_2,16 * 4,line_2,DISPLAY_POSITIVE);
			if(FIELD_FLAG_S0D1)
				{
					//����������ʾB����
				}
			else
				{
					switch(SW_Type[1])
					{
						case 0:
							strcpy(line_3,"���豸");
							break;
						case 1:
							strcpy(line_3,"��������");
							break;
						case 2:
							strcpy(line_3,"��������");
							break;
						case 3:
							strcpy(line_3,"˫��̽��");
							break;
						case 4:
							strcpy(line_3,"����̽��");
							break;
						case 5:
							strcpy(line_3,"�Ŵ�̽��");
							break;
						case 6:
							strcpy(line_3,"������ť");
							break;
						case 7:
							strcpy(line_3,"ˮ��̽��");
							break;
						case 8:
							strcpy(line_3,"�����豸");
							break;
						default:
							break;
					}
					Display_Line_Index_16x16(Page_4,16 * 4,line_3,DISPLAY_POSITIVE);
				}
		}
	else//�༭��ʾʱ����Ҫ��ʾ
		{
			switch(row_point)
			{
				case 1://A������Ҫ�༭
					switch(SW_Type[0])
					{
						case 0:
							strcpy(line_2,"���豸");
							break;
						case 1:
							strcpy(line_2,"��������");
							break;
						case 2:
							strcpy(line_2,"��������");
							break;
						case 3:
							strcpy(line_2,"˫��̽��");
							break;
						case 4:
							strcpy(line_2,"����̽��");
							break;
						case 5:
							strcpy(line_2,"�Ŵ�̽��");
							break;
						case 6:
							strcpy(line_2,"������ť");
							break;
						case 7:
							strcpy(line_2,"ˮ��̽��");
							break;
						case 8:
							strcpy(line_2,"�����豸");
							break;
						default:
							break;
					}
					Display_Line_Index_16x16(Page_2,16 * 4,line_2,DISPLAY_NEGATIVE);
					switch(SW_Type[1])
					{
						case 0:
							strcpy(line_3,"���豸");
							break;
						case 1:
							strcpy(line_3,"��������");
							break;
						case 2:
							strcpy(line_3,"��������");
							break;
						case 3:
							strcpy(line_3,"˫��̽��");
							break;
						case 4:
							strcpy(line_3,"����̽��");
							break;
						case 5:
							strcpy(line_3,"�Ŵ�̽��");
							break;
						case 6:
							strcpy(line_3,"������ť");
							break;
						case 7:
							strcpy(line_3,"ˮ��̽��");
							break;
						case 8:
							strcpy(line_3,"�����豸");
							break;
						default:
							break;
					}
					Display_Line_Index_16x16(Page_4,16 * 4,line_3,DISPLAY_POSITIVE);
					break;
				case 2://B������Ҫ�༭
					switch(SW_Type[0])
					{
						case 0:
							strcpy(line_2,"���豸");
							break;
						case 1:
							strcpy(line_2,"��������");
							break;
						case 2:
							strcpy(line_2,"��������");
							break;
						case 3:
							strcpy(line_2,"˫��̽��");
							break;
						case 4:
							strcpy(line_2,"����̽��");
							break;
						case 5:
							strcpy(line_2,"�Ŵ�̽��");
							break;
						case 6:
							strcpy(line_2,"������ť");
							break;
						case 7:
							strcpy(line_2,"ˮ��̽��");
							break;
						case 8:
							strcpy(line_2,"�����豸");
							break;
						default:
							break;
					}
					Display_Line_Index_16x16(Page_2,16 * 4,line_2,DISPLAY_POSITIVE);
					switch(SW_Type[1])
					{
						case 0:
							strcpy(line_3,"���豸");
							break;
						case 1:
							strcpy(line_3,"��������");
							break;
						case 2:
							strcpy(line_3,"��������");
							break;
						case 3:
							strcpy(line_3,"˫��̽��");
							break;
						case 4:
							strcpy(line_3,"����̽��");
							break;
						case 5:
							strcpy(line_3,"�Ŵ�̽��");
							break;
						case 6:
							strcpy(line_3,"������ť");
							break;
						case 7:
							strcpy(line_3,"ˮ��̽��");
							break;
						case 8:
							strcpy(line_3,"�����豸");
							break;
						default:
							break;
					}
					Display_Line_Index_16x16(Page_4,16 * 4,line_3,DISPLAY_NEGATIVE);
					break;

				default:
					break;
			}
		}
}
char tension_max_range_temp[4] = "";//������ ����ֵ�ݴ�
void DisplaySetFullScale(void)
{
	char line_1[] = "������������ֵ��";
	char line_2[] = "         KG      ";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_8x16(Page_4,0,line_2,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_pn(Page_4,8*5,tension_max_range_temp,row_fx_pos - 1);
}

char base_auto_calibrate_time_tmp[6] = "";//��׼ֵ�Զ�У׼ʱ���ݴ�
void DisplaySetBaseAutoCalibrateTime(void)
{
	char line_1[] = "����׼��Уʱ�䡷";
	char fenzhong[] = "����";
	
	Lcd_Clear(FullScreen);    //��ȫ��
	
	Display_Line_Index_16x16(Page_0,0,line_1,DISPLAY_POSITIVE);
	Display_Line_Index_16x16(Page_4,5*16,fenzhong,DISPLAY_POSITIVE);
	
	Display_Line_Index_8x16_pn(Page_4,8*3,base_auto_calibrate_time_tmp,row_fx_pos - 1);
}

