#ifndef    _DISPLAYMENU_H_
#define    _DISPLAYMENU_H_

#define    InitScreen    0
#define    RunScreen     1
//#define    RootScreen    2
#define    SetScreen     3
#define    InqScreen     4
#define    VerScreen     5
#define    SetSysScreen    6
#define    SetFangQuParaScreen	7
#define    SetAFangQuParaScreen	8
#define    SetBFangQuParaScreen	9
#define    SetBuFangParaScreen    10
#define    SetClearRecordScreen    11
#define    InqSysParaScreen        12
#define    InqFangQuParaScreen	13
#define    InqAFangQuParaScreen	14
#define    InqBFangQuParaScreen	15
#define    InqBuFangParaScreen        16
#define    InqAlarmRecordScreen        17
#define    SetDateTimeScreen    18
#define    SetEthParaScreen    19
#define    SetRS485ParaScreen    20
#define    SetAAreaBuFangParaScreen    21
#define    SetBAreaBuFangParaScreen    22
#define    InqAAreaBuFangParaScreen    23
#define    InqBAreaBuFangParaScreen    24
#define    InqDuanLuAlarmScreen    25
#define    InqDuanXianAlarmScreen    26
#define    InqFangChaiAlarmRecordScreen    27
#define    InqEthParaScreen    28
#define    InqRS485ParaScreen    29
#define    SetMacAddrScreen    30
#define    SetIpAddrScreen    31
#define    SetSubnetMaskScreen    32
#define    SetGateWayScreen    33
#define    InqMacAddrScreen    34
#define    InqIpAddrScreen    35                      //
#define    InqSubnetMaskScreen    36                  //
#define    InqGateWayScreen    37                     //
#define    InqAAreaDuanLuAlarmRecordScreen    38      //
#define    InqBAreaDuanLuAlarmRecordScreen    39      //
#define    InqAAreaDuanXianAlarmRecordScreen    40    //
#define    InqBAreaDuanXianAlarmRecordScreen    41    //
#define    ClearDuanLuAlarmRecordScreen    42         //
#define    ClearDuanXianAlarmRecordScreen    43       //
#define    InputPasswordScreen    44
#define    PasswordErrorScreen    45
#define    AlarmTypeListScreen    46
#define    SetExtDeviceTypeScreen    47               //�趨����豸������

#define    UserLoginScreen    48
#define    SuperUserRootScreen    49
#define    OrdinaryUserRootScreen    50

#define    TensionMonitorScreen    100
#define    AFangQuMonitorScreen    101
#define    BFangQuMonitorScreen    102
#define    SetAlarmThresholdScreen    103//���ø澯��ֵ��ʾ��
#define    CalibrationSetScreen    104//У׼������ʾ��
#define    ZeroCalibrationScreen    105//���У׼��ʾ��
#define    DatumCalibrationScreen    106//��׼ֵУ׼��ʾ��
#define    InqAlarmThresholdScreen    107//��ѯ�澯��ֵ��ʾ��
#define    InqCalibrationValScreen    108//��ѯУ׼ֵ��ʾ��
#define    InqExtDeviceTypeScreen    109//��ѯ�����ⲿ�豸��ʾ��
#define    InqZeroValScreen    110//��ѯ���У׼ֵ��ʾ��
#define    InqDatumValScreen    111//��ѯ��׼У׼ֵ
#define    AzeroCalibrationScreen    112//A�������У׼��ʾ��
#define    BzeroCalibrationScreen    113//B�������У׼��ʾ��
#define    AdatumCalibrationScreen    114//A������׼У׼��ʾ��
#define    BdatumCalibrationScreen    115//B������׼У׼��ʾ��
#define    NotAtValidRangeScreen    116//δ����Ч��Χ��ʾ��

#define    CaliOverAareaZeroValScreen    117//���У׼��ɺ���ʾ��A�������ֵ��ʾ�������ѯA�������У׼ֵ��ʾ������ͬһ����������֮ͬ�����ڰ�����ת��ͬ
#define    CaliOverBareaZeroValScreen    118//���У׼��ɺ���ʾ��B�������ֵ��ʾ�������ѯB�������У׼ֵ��ʾ������ͬһ����������֮ͬ�����ڰ�����ת��ͬ
#define    CaliOverAareaDatumValScreen    119//��׼У׼��ɺ���ʾ��A������׼ֵ��ʾ�������ѯA������׼У׼ֵ��ʾ������ͬһ����������֮ͬ�����ڰ�����ת��ͬ
#define    CaliOverBareaDatumValScreen    120//��׼У׼��ɺ���ʾ��B������׼ֵ��ʾ�������ѯB������׼У׼ֵ��ʾ������ͬһ����������֮ͬ�����ڰ�����ת��ͬ

#define    InqAareaZeroValScreen    121//��ѯA�������ֵ��ʾ��
#define    InqBareaZeroValScreen    122//��ѯB�������ֵ��ʾ��
#define    InqAareaDatumValScreen    123//��ѯA������׼ֵ��ʾ��
#define    InqBareaDatumValScreen    124//��ѯB������׼ֵ��ʾ��

#define    SetFullScaleScreen    125//��������������ֵ��ʾ��
#define    InqFullScaleScreen    126//��ѯ����������ֵ��ʾ��
#define    SetBaseAutoCalibrateTimeScreen    127//���û�׼ֵ�Զ�У׼ʱ����ʾ��
#define    InqBaseAutoCalibrateTimeScreen    128//��ѯ��׼ֵ�Զ�У׼ʱ����ʾ��

#define    passwordEnable    1
#define    passwordDisable    0
extern unsigned char password_en_dis;

extern unsigned char UserId;//�û�����
#define    SuperUser    0
#define    OrdinaryUser    1

extern char lcd_super_password[];
extern char lcd_ordinary_password[];
extern char inputPassword[];
extern char display_xinghao[];
extern unsigned char validPasswordCnt;//��Ч����λ��

extern char rtcTempStr_temp_test[19];

extern u8 field_num_temp[2];  //A��B����Ӧ�ķ����ţ�ȡֵ��1-80�� 0:������
extern u16 alarm_delay_temp[2];	//A,B��������ʱ��Ĭ��30��, 0 ~ 999
extern u16 ten_val_range_temp[2][2];//A,B������Ӧ����Ч������Χ
extern u8 alarm_sensitivity_temp[2];//A,B������Ӧ��������

//������ֵ������Բ�ֵ���磺203 -> 20.3Kg
extern u16 alarm_threshold_up_dif_temp;
//�ɳ���ֵ������Բ�ֵ, �磺203 -> 20.3Kg
extern u16 alarm_threshold_down_dif_temp;

extern char tension_max_range_temp[4];//������ ����ֵ�ݴ�
extern char base_auto_calibrate_time_tmp[6];//��׼ֵ�Զ�У׼ʱ���ݴ�

extern    unsigned char flag_PageState;//ҳ��״̬��ʶ��1:������ʾ/0:�༭��ʾ��
#define    NormalState    1
#define    ModifyState    0

extern    unsigned char row_point;//��ָ��
extern    signed char row_fx_pos;//ĳһ���ϵķ�����ʾλ�ã������޸�����ʱ��ʱ��ÿһ�л���3���ɱ༭�ķ���λ��

extern    unsigned char ascii_data[2];

extern    char eth_mac_addr[18];//��MAC��ַ�в���ð��

extern    char eth_ip_addr[16];
extern    char eth_subnet_mask[16];
extern    char eth_gateway[16];

//extern    unsigned char alarm_sequence_buf[7];
extern    s8 alarm_rol;
extern    s8 alarm_key_adj;//���澯���ʹ���3��ʱ��ͨ��������������ʾ��Ŀ

extern unsigned char run_screen_refresh;

void DisplayInitMenu(void);//��ʼ���˵�
void DisplayRunMenu(void);//���в˵�
void DisplayUserLogin(void);//�û���¼
void DisplaySuperUserRootMenu(void);//�����û���Ŀ¼
void DisplayOrdinaryUserRootMenu(void);//��ͨ�û���Ŀ¼
void DisplayInputPassword(void);//��������
void DisplayPasswordError(void);//�������
void DisplaySetMenu(void);//�趨�˵�
void DisplayInquireMenu(void);//��ѯ�˵�
void DisplayTensionMonitor(void);//�������
void DisplayVersionMenu(void);//�汾��Ϣ
void DisplaySetSysPara(void);//����ϵͳ����
void DiaplaySetFangQuPara(void);//���÷�������
void DiaplayCalibrationSet(void);//У׼����
void DiaplaySetAlarmThreshold(void);//���ø澯��ֵ����
void DisplaySetAFangQuPara(void);//����A��������
void DisplaySetBFangQuPara(void);//����B��������
void DisplayiLlegalInput(void);//�Ƿ����룬����������
void DisplayZeroCalibration(void);//���ֵУ׼
void DisplayDatumCalibration(void);//��׼ֵУ׼
void Displayzhengzaijiaozhun(void);//����У׼
void DisplayNotAtValidRange(void);//δ����Ч��Χ
void DiaplaySetBuFangPara(void);//���ò�������
void DisplaySetClearRecord(void);//�����¼
void DisplayZhengZaiQingChu(void);//�������
void DisplayInqSysPara(void);//��ѯϵͳ����
void DiaplayInqFangQuPara(void);//��ѯ��������
void DiaplayInqAFangQuPara(void);//��ѯA��������
void DiaplayInqBFangQuPara(void);//��ѯB��������
void DisplayInqBuFangPara(void);//��ѯ��������
void DisplayInqAlarmRecord(void);//��ѯ�澯��¼
void DisplayInqAlarmThreshold(void);//��ѯ�澯��ֵ
void DisplayInqCalibrationVal(void);//��ѯУ׼ֵ
void DisplayInqZeroVal(void);//��ѯ���У׼ֵ
void DiaplayInqDatumVal(void);//��ѯ��׼У׼ֵ
void DisplayInqExtDeviceType(void);//��ѯ�����ⲿ�豸
void DiaplayAFangQuMonitor(void);//A�����������
void DiaplayBFangQuMonitor(void);//B�����������
void DisplaySetDateTime(void);//�趨����ʱ��
void DisplaySetEthPara(void);//�����������
void DisplaySetRS485Para(void);//����RS485����
void DisplayClearRuQinAlarmRecord(void);//������ָ澯��¼
void DisplayClearDuanXianAlarmRecord(void);//������߸澯��¼
void DisplayInqEthPara(void);//��ѯ�������
void DisplayInqRS485Para(void);//��ѯRS485����
void DisplayInqRuQinAlarmRecord(void);//��ѯ���ָ澯��¼
void DisplayInqDuanXianAlarmRecord(void);//��ѯ���߸澯��¼
void DisplayInqFangChaiAlarmRecord(void);//��ѯ����澯��¼
void DisplaySetMacAddr(void);//����MAC��ַ
void DisplaySetIpAddr(void);//����IP��ַ
void DisplaySetSubnetMask(void);//������������
void DisplaySetGateWay(void);//��������
void DisplayInqMacAddr(void);//��ѯMAC��ַ
void DisplayInqIpAddr(void);//��ѯIP��ַ
void DisplayInqSubnetMask(void);//��ѯ��������
void DisplayInqGateWay(void);//��ѯ����
void UpdateAlarmRecord(void);//���¸澯��¼
void DisplayInqAAreaRuQinAlarmRecord(void);//��ѯA�������ָ澯��¼
void DisplayInqBAreaRuQinAlarmRecord(void);//��ѯB�������ָ澯��¼
void DisplayInqAAreaDuanXianAlarmRecord(void);//��ѯA�������߸澯��¼
void DisplayInqBAreaDuanXianAlarmRecord(void);//��ѯB�������߸澯��¼

void DisplayAareaZeroVal(void);//��ʾA�������ֵ
void DisplayBareaZeroVal(void);//��ʾB�������ֵ
void DisplayAareaDatumVal(void);//��ʾA������׼ֵ
void DisplayBareaDatumVal(void);//��ʾB������׼ֵ

void DisplaySetFullScale(void);//��������������ֵ
void DisplayInqFullScale(void);//��ѯ����������ֵ
void DisplaySetBaseAutoCalibrateTime(void);//���û�׼ֵ�Զ�У׼ʱ��
void DisplayInqBaseAutoCalibrateTime(void);//��ѯ��׼ֵ�Զ�У׼ʱ��

void HEX2ASCII(unsigned char hex_data);//ʮ������תASCII
unsigned char HEX2ASCII_2(unsigned char tn_data);//

void MacAddrInsertMaohao(void);//MAC��ַ����ð��

void GetIpAddr(void);//��ȡIP��ַ
void SetIpAddr(void);//
void GetSubNetMask(void);//
void SetSubNetMask(void);//
void GetGateWay(void);//
void SetGateWay(void);//

void DisplayAlarmTypeList(void);//

void DiaplaySetExtDeviceTypeScreen(void);//

#endif
