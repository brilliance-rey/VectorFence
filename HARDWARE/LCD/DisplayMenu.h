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
#define    SetExtDeviceTypeScreen    47               //设定外接设备类型屏

#define    UserLoginScreen    48
#define    SuperUserRootScreen    49
#define    OrdinaryUserRootScreen    50

#define    TensionMonitorScreen    100
#define    AFangQuMonitorScreen    101
#define    BFangQuMonitorScreen    102
#define    SetAlarmThresholdScreen    103//设置告警阈值显示屏
#define    CalibrationSetScreen    104//校准设置显示屏
#define    ZeroCalibrationScreen    105//零点校准显示屏
#define    DatumCalibrationScreen    106//基准值校准显示屏
#define    InqAlarmThresholdScreen    107//查询告警阈值显示屏
#define    InqCalibrationValScreen    108//查询校准值显示屏
#define    InqExtDeviceTypeScreen    109//查询兼容外部设备显示屏
#define    InqZeroValScreen    110//查询零点校准值显示屏
#define    InqDatumValScreen    111//查询基准校准值
#define    AzeroCalibrationScreen    112//A防区零点校准显示屏
#define    BzeroCalibrationScreen    113//B防区零点校准显示屏
#define    AdatumCalibrationScreen    114//A防区基准校准显示屏
#define    BdatumCalibrationScreen    115//B防区基准校准显示屏
#define    NotAtValidRangeScreen    116//未在有效范围显示屏

#define    CaliOverAareaZeroValScreen    117//零点校准完成后显示的A防区零点值显示屏，与查询A防区零点校准值显示屏调用同一个函数，不同之处在于按键跳转不同
#define    CaliOverBareaZeroValScreen    118//零点校准完成后显示的B防区零点值显示屏，与查询B防区零点校准值显示屏调用同一个函数，不同之处在于按键跳转不同
#define    CaliOverAareaDatumValScreen    119//基准校准完成后显示的A防区基准值显示屏，与查询A防区基准校准值显示屏调用同一个函数，不同之处在于按键跳转不同
#define    CaliOverBareaDatumValScreen    120//基准校准完成后显示的B防区基准值显示屏，与查询B防区基准校准值显示屏调用同一个函数，不同之处在于按键跳转不同

#define    InqAareaZeroValScreen    121//查询A防区零点值显示屏
#define    InqBareaZeroValScreen    122//查询B防区零点值显示屏
#define    InqAareaDatumValScreen    123//查询A防区基准值显示屏
#define    InqBareaDatumValScreen    124//查询B防区基准值显示屏

#define    SetFullScaleScreen    125//设置满量程拉力值显示屏
#define    InqFullScaleScreen    126//查询满量程拉力值显示屏
#define    SetBaseAutoCalibrateTimeScreen    127//设置基准值自动校准时间显示屏
#define    InqBaseAutoCalibrateTimeScreen    128//查询基准值自动校准时间显示屏

#define    passwordEnable    1
#define    passwordDisable    0
extern unsigned char password_en_dis;

extern unsigned char UserId;//用户类型
#define    SuperUser    0
#define    OrdinaryUser    1

extern char lcd_super_password[];
extern char lcd_ordinary_password[];
extern char inputPassword[];
extern char display_xinghao[];
extern unsigned char validPasswordCnt;//有效密码位数

extern char rtcTempStr_temp_test[19];

extern u8 field_num_temp[2];  //A，B区对应的防区号，取值：1-80， 0:无设置
extern u16 alarm_delay_temp[2];	//A,B区警号延时，默认30秒, 0 ~ 999
extern u16 ten_val_range_temp[2][2];//A,B防区对应的有效拉力范围
extern u8 alarm_sensitivity_temp[2];//A,B防区对应的灵敏度

//入侵阈值上限相对差值，如：203 -> 20.3Kg
extern u16 alarm_threshold_up_dif_temp;
//松弛阈值下限相对差值, 如：203 -> 20.3Kg
extern u16 alarm_threshold_down_dif_temp;

extern char tension_max_range_temp[4];//满量程 拉力值暂存
extern char base_auto_calibrate_time_tmp[6];//基准值自动校准时间暂存

extern    unsigned char flag_PageState;//页面状态标识（1:正常显示/0:编辑显示）
#define    NormalState    1
#define    ModifyState    0

extern    unsigned char row_point;//行指针
extern    signed char row_fx_pos;//某一行上的反白显示位置，比如修改日期时间时，每一行会有3个可编辑的反白位置

extern    unsigned char ascii_data[2];

extern    char eth_mac_addr[18];//将MAC地址中插入冒号

extern    char eth_ip_addr[16];
extern    char eth_subnet_mask[16];
extern    char eth_gateway[16];

//extern    unsigned char alarm_sequence_buf[7];
extern    s8 alarm_rol;
extern    s8 alarm_key_adj;//当告警类型大于3条时，通过按键来调整显示条目

extern unsigned char run_screen_refresh;

void DisplayInitMenu(void);//初始化菜单
void DisplayRunMenu(void);//运行菜单
void DisplayUserLogin(void);//用户登录
void DisplaySuperUserRootMenu(void);//超级用户根目录
void DisplayOrdinaryUserRootMenu(void);//普通用户根目录
void DisplayInputPassword(void);//输入密码
void DisplayPasswordError(void);//密码错误
void DisplaySetMenu(void);//设定菜单
void DisplayInquireMenu(void);//查询菜单
void DisplayTensionMonitor(void);//拉力监控
void DisplayVersionMenu(void);//版本信息
void DisplaySetSysPara(void);//设置系统参数
void DiaplaySetFangQuPara(void);//设置防区参数
void DiaplayCalibrationSet(void);//校准设置
void DiaplaySetAlarmThreshold(void);//设置告警阈值参数
void DisplaySetAFangQuPara(void);//设置A防区参数
void DisplaySetBFangQuPara(void);//设置B防区参数
void DisplayiLlegalInput(void);//非法输入，请重新输入
void DisplayZeroCalibration(void);//零点值校准
void DisplayDatumCalibration(void);//基准值校准
void Displayzhengzaijiaozhun(void);//正在校准
void DisplayNotAtValidRange(void);//未在有效范围
void DiaplaySetBuFangPara(void);//设置布防参数
void DisplaySetClearRecord(void);//清除记录
void DisplayZhengZaiQingChu(void);//正在清除
void DisplayInqSysPara(void);//查询系统参数
void DiaplayInqFangQuPara(void);//查询防区参数
void DiaplayInqAFangQuPara(void);//查询A防区参数
void DiaplayInqBFangQuPara(void);//查询B防区参数
void DisplayInqBuFangPara(void);//查询布防参数
void DisplayInqAlarmRecord(void);//查询告警记录
void DisplayInqAlarmThreshold(void);//查询告警阈值
void DisplayInqCalibrationVal(void);//查询校准值
void DisplayInqZeroVal(void);//查询零点校准值
void DiaplayInqDatumVal(void);//查询基准校准值
void DisplayInqExtDeviceType(void);//查询兼容外部设备
void DiaplayAFangQuMonitor(void);//A防区拉力监控
void DiaplayBFangQuMonitor(void);//B防区拉力监控
void DisplaySetDateTime(void);//设定日期时间
void DisplaySetEthPara(void);//设置网络参数
void DisplaySetRS485Para(void);//设置RS485参数
void DisplayClearRuQinAlarmRecord(void);//清除入侵告警记录
void DisplayClearDuanXianAlarmRecord(void);//清除偏移告警记录
void DisplayInqEthPara(void);//查询网络参数
void DisplayInqRS485Para(void);//查询RS485参数
void DisplayInqRuQinAlarmRecord(void);//查询入侵告警记录
void DisplayInqDuanXianAlarmRecord(void);//查询偏移告警记录
void DisplayInqFangChaiAlarmRecord(void);//查询防拆告警记录
void DisplaySetMacAddr(void);//设置MAC地址
void DisplaySetIpAddr(void);//设置IP地址
void DisplaySetSubnetMask(void);//设置子网掩码
void DisplaySetGateWay(void);//设置网关
void DisplayInqMacAddr(void);//查询MAC地址
void DisplayInqIpAddr(void);//查询IP地址
void DisplayInqSubnetMask(void);//查询子网掩码
void DisplayInqGateWay(void);//查询网关
void UpdateAlarmRecord(void);//更新告警记录
void DisplayInqAAreaRuQinAlarmRecord(void);//查询A防区入侵告警记录
void DisplayInqBAreaRuQinAlarmRecord(void);//查询B防区入侵告警记录
void DisplayInqAAreaDuanXianAlarmRecord(void);//查询A防区偏移告警记录
void DisplayInqBAreaDuanXianAlarmRecord(void);//查询B防区偏移告警记录

void DisplayAareaZeroVal(void);//显示A防区零点值
void DisplayBareaZeroVal(void);//显示B防区零点值
void DisplayAareaDatumVal(void);//显示A防区基准值
void DisplayBareaDatumVal(void);//显示B防区基准值

void DisplaySetFullScale(void);//设置满量程拉力值
void DisplayInqFullScale(void);//查询满量程拉力值
void DisplaySetBaseAutoCalibrateTime(void);//设置基准值自动校准时间
void DisplayInqBaseAutoCalibrateTime(void);//查询基准值自动校准时间

void HEX2ASCII(unsigned char hex_data);//十六进制转ASCII
unsigned char HEX2ASCII_2(unsigned char tn_data);//

void MacAddrInsertMaohao(void);//MAC地址插入冒号

void GetIpAddr(void);//获取IP地址
void SetIpAddr(void);//
void GetSubNetMask(void);//
void SetSubNetMask(void);//
void GetGateWay(void);//
void SetGateWay(void);//

void DisplayAlarmTypeList(void);//

void DiaplaySetExtDeviceTypeScreen(void);//

#endif
