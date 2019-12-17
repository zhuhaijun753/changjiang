/* 
车辆车况相关的数据信息结构
*/
#define	MAX_SUBITEM_NUM		32
#define	MAX_SIGNDATA_LEN	32
#define	MAX_DIAGNOSECODE	200
#define	MAX_DIAGNOSECODEFAULT	200
#define	MAX_UPGRADEPARAM_LEN	0xFF
#define	MAX_CONFIGDATA_LEN	32

#define	MAX_SIGNCODE_TOTAL	44
#define MAX_FAULT_SIGNA		44
#define MAX_DRIVEACTION_SIGNA	37

typedef union{ 
	uint8_t BitType;
 	struct{
		uint8_t dataState 	:7; //数据
		uint8_t flag		:1;	//数据标识
  	}dataBit;
}BitType_U;

//车况信号编码ID列表
typedef enum
{
	ACPSIGNCODE_GPSID			= 	0,	//GPS信息
	ACPSIGNCODE_RemainedOilID	= 	1,	//剩余油量
	ACPSIGNCODE_OdometerID		= 	2,	//总里程
	ACPSIGNCODE_BatteryID		= 	3,	//蓄电池电量
	ACPSIGNCODE_CurrentSpeedID	= 	4,	//实时车速
	ACPSIGNCODE_LTAverageSpeedID= 	5,	//长时平均车速
	ACPSIGNCODE_STAverageSpeedID= 	6,	//短时平均车速
	ACPSIGNCODE_LTAverageOilID	= 	7,	//长时平均油耗
	ACPSIGNCODE_STAverageOilID	= 	8,	//短时平均油耗
	ACPSIGNCODE_CarDoorStateID	=	9,	//车门状态
	ACPSIGNCODE_CarLockState	=	10,	//车锁状态
	ACPSIGNCODE_SunroofStateID	=	11, //天窗状态
	ACPSIGNCODE_WindowStateID	=	12,	//车窗状态
	ACPSIGNCODE_CarlampStateID	=	13,	//车灯状态
	ACPSIGNCODE_TyreStateID		=	14,	//轮胎信息
	ACPSIGNCODE_VerTboxMCUID	=	15,	//TBox_MCU版本
	ACPSIGNCODE_EngineStateID	=	16,	//发动机状态
	ACPSIGNCODE_WheelStateID	=	17,	//实时方向盘转角信息
	ACPSIGNCODE_PastRecordSpeedID	  =	18,	//历史车速
	ACPSIGNCODE_PastRecordWheelStateID=	19,	//历史方向盘转角
	ACPSIGNCODE_EngineSpeedID	=	20,	//发动机转速
	ACPSIGNCODE_GearStateID		=	21,	//档位信息
	ACPSIGNCODE_HandBrakeStateID=	22,	//手刹状态
	ACPSIGNCODE_ParkingStateID	=	23,	//电子驻车状态
	ACPSIGNCODE_PowerSupplyModeID=	24, //供电模式
	ACPSIGNCODE_SafetyBeltID	=	25,	//安全带状态
	ACPSIGNCODE_RemainUpkeepMileageID=	26,//剩余保养里程
	ACPSIGNCODE_AirconditionerInfoID =	27,//空调相关信息
	ACPSIGNCODE_KeepingstateTimeID	 =	28,//持续时间信息
	ACPSIGNCODE_PowerCellsStateID	 =	29,//动力电池状态
	ACPSIGNCODE_ChargeStateID	 =	30,//充电状态
	ACPSIGNCODE_VerTboxOSID		 =	31,//TBox_OS版本
	ACPSIGNCODE_VerIVIID		 = 	32,//IVI版本号
	ACPSIGNCODE_ChargeConnetStateID	 =	33,//充电枪连接状态
	ACPSIGNCODE_BrakePedalSwitchID	 = 	34,//制动踏板开关
	ACPSIGNCODE_AcceleraPedalSwitchID=	35,//加速踏板开关
	ACPSIGNCODE_YaWSensorInfoSwitchID= 	36,//Yaw传感器信息
	ACPSIGNCODE_AmbientTemperatID	 = 	37,//环境温度
	ACPSIGNCODE_PureElecRelayID	 = 	38,//纯电动继电器及线圈相关状态
	ACPSIGNCODE_RemainderRangeID	 = 	39,//剩余续航里程
	ACPSIGNCODE_NewEnergyHeatManageID= 	40,//新能源热管理相关请求
	ACPSIGNCODE_VehWorkModeID	 	= 	41,//车辆工作模式
	ACPSIGNCODE_MotorWorkStateID	 = 	42,//电机工作状态
	ACPSIGNCODE_HighVolageStateID	 = 	43,//高压系统准备状态 
}AcpSignCodeID_E;

//远程配置项ID列表
typedef enum
{
	ACPCfgItem_EngineStartTimeID	= 	0,	//发动机远程启动时长
	ACPCfgItem_SamplingSwitchID		= 	1,	//电动车国标抽检开关
	ACPCfgItem_EmergedCallID		= 	2,	//紧急服务号码配置
	ACPCfgItem_WhitelistCallID		= 	3,	//白名单号码配置
	ACPCfgItem_CollectTimeIntervalID= 	4,	//车况定时采集间隔时间
	ACPCfgItem_ReportTimeIntervalID	= 	5,	//车况定时上报间隔时间
	ACPCfgItem_CollectTimeGpsSpeedIntervalID= 	6,	//定时采集(位置和车速)间隔时间
	ACPCfgItem_ReportTimeGpsSpeedIntervalID = 	7,	//定时上报(位置和车速)间隔时间
	ACPCfgItem_ChargeModeID		= 	8,	//充电模式
    ACPCfgItem_ChargeScheduleID	=	9,	//预约充电时间表	
	ACPCfgItem_LoadCellID		= 	10,	//B-Cell
	ACPCfgItem_InformationCellID= 	11,	//I-Cell
}AcpConfigItemID_E;

//远程控制子项目编码ID列表
typedef enum
{
	//车身
	VehicleBody_LockID 			= 	1,	//车锁
    VehicleBody_WindowID 		= 	2,	//车窗
    VehicleBody_SunroofID 		= 	3,	//天窗
    VehicleBody_TrackingCarID	=	4,	//寻车
	VehicleBody_LowbeamID		=	5,	//近光灯
	VehicleBody_LuggageCarID	=	6,	//行李箱
	//空调
	Airconditioner_ControlID 			= 	20,	//空调控制
    Airconditioner_CompressorSwitchID 	= 	21,	//压缩机开关
    Airconditioner_TemperatureID 		= 	22,	//温度调节
    Airconditioner_SetAirVolumeID		=	23,	//设定风量
	Airconditioner_FrontDefrostSwitchID	=	24,	//前除霜开关
	Airconditioner_HeatedrearID			=	25,	//后窗加热
	Airconditioner_BlowingModeID		=	26,	//吹风模式
	Airconditioner_InOutCirculateID		=	27,	//内外循环
	Airconditioner_AutoSwitchID			=	28,	//Auto开关
	//发动机
	EngineState_SwitchID 			= 	30,	//发动机状态启停
	//座椅
	VehicleSeat_DrivingSeatID 		= 	40,	//主驾驶座椅加热
	VehicleSeat_CopilotseatID		= 	41,	//副驾驶座椅加热
	VehicleSeat_DrivingSeatMomeryID	=	42,	//主驾驶座椅记忆调去
	//充电
	VehicleChargeMode_ImmediateID 	= 	50,	//即时充电
	VehicleChargeMode_AppointmentID	= 	51,	//预约充电
	VehicleChargeMode_EmergencyCharg =  52,	//紧急充电
	//自动泊车
	VehicleWIFIStatusID				= 	60,	//WIFI状态
	VehicleAutoOUTID				=	61,	//自动出车
	//功能配置状态
	FeatureConfig_StateID			=	255,//功能状态
}SubitemCode_E;

/**************************************************************************************************
*****远程控制
**************************************************************************************************/
//车身
typedef struct
{
    uint8_t	VehicleBody_Lock;	//车锁
    uint8_t	VehicleBody_Window;	//车窗
    uint8_t	VehicleBody_Sunroof;//天窗
    uint8_t	VehicleBody_TrackingCar;//寻车
	uint8_t	VehicleBody_Lowbeam;	//近光灯
	uint8_t VehicleBody_LuggageCar; //行李箱
}VehicleBodyList_t;
//空调
typedef struct
{
	BitType_U	Airconditioner_Control;			//空调控制
    BitType_U	Airconditioner_CompressorSwitch;//压缩机开关
    BitType_U	Airconditioner_Temperature;		//温度调节
    BitType_U	Airconditioner_SetAirVolume;	//设定风量
	BitType_U	Airconditioner_FrontDefrostSwitch;//前除霜开关
	BitType_U	Airconditioner_Heatedrear;		//后窗加热
	BitType_U	Airconditioner_BlowingMode;	  	//吹风模式
	BitType_U	Airconditioner_InOutCirculate;	//内外循环
	BitType_U	Airconditioner_AutoSwitch;		//Auto开关
}AirconditionerList_t;
//空调(Tsp Command)
typedef struct
{
	uint8_t		Airconditioner_Control;			//空调控制
    uint8_t		Airconditioner_CompressorSwitch;//压缩机开关
    BitType_U	Airconditioner_Temperature;		//温度调节
    BitType_U	Airconditioner_SetAirVolume;	//设定风量
	uint8_t		Airconditioner_FrontDefrostSwitch;//前除霜开关
	uint8_t		Airconditioner_Heatedrear;		//后窗加热
	BitType_U	Airconditioner_BlowingMode;	  	//吹风模式
	BitType_U	Airconditioner_InOutCirculate;	//内外循环
	uint8_t		Airconditioner_AutoSwitch;		//Auto开关
}TspAirconditionerList_t;
//发动机
typedef struct
{
	uint8_t	EngineState_Switch;//发动机状态启停
}EngineStateList_t;
//座椅
typedef struct
{
	uint8_t	VehicleSeat_DrivingSeat;//主驾驶座椅加热
	uint8_t	VehicleSeat_Copilotseat;//副驾驶座椅加热
	uint8_t VehicleSeat_DrivingSeatMemory;//主驾驶座椅记忆
}VehicleSeatList_t;
//充电
typedef struct
{
	uint8_t	VehicleCharge_Immediate;  //即时充电
	uint8_t	VehicleCharge_Appointment;//预约充电
	uint8_t VehicleCharge_EmergenCharg;//紧急充电
}VehicleChargeList_t;
//自动泊车
typedef struct
{
	uint8_t VehicleWifiStatus;	//WIFI状态
	uint8_t VehicleAutoOut;		//自动出车
}VehicleAutopark_t;
//远程控制数据
typedef struct
{
	VehicleBodyList_t		VehicleBody;	//车身数据
	AirconditionerList_t	Airconditioner;	//空调数据
	EngineStateList_t		EngineState;	//发动机数据
	VehicleSeatList_t		VehicleSeat;	//座椅数据
	VehicleChargeList_t		VehicleCharge;	//充电数据
	VehicleAutopark_t		VehicleAutopark;//自动泊车
	uint8_t 				FunctionConfigStatus;//功能配置状态
}RemoteControlData_t;

//远程控制命令Tsp
typedef struct
{
	VehicleBodyList_t		VehicleBody;	//车身数据
	TspAirconditionerList_t	Airconditioner;	//空调数据
	EngineStateList_t		EngineState;	//发动机数据
	VehicleSeatList_t		VehicleSeat;	//座椅数据
	VehicleChargeList_t		VehicleCharge;	//充电数据
	VehicleAutopark_t		VehicleAutopark;//自动泊车
}AcpRemoteControlCommand_t;

/*	远程控制数据结构体列表	*/
typedef struct
{
	uint8_t			SubitemTotal;//需要配置的子项目总数
	uint8_t			SubitemCode[MAX_SUBITEM_NUM];//子项目编码
	uint8_t			SubitemValue[MAX_SUBITEM_NUM];//子项目值
}AcpRemoteControl_RawData;
/**************************************************************************************************/

/**************************************************************************************************
*****车况查询
**************************************************************************************************/

/* 车况【查询指令】数据结构 */
typedef struct
{
	uint16_t	CollectTime;	//采集时间间隔ms 
	uint16_t 	CollectCount;	//采集次数
	uint16_t	SignTotal;		//采集信号总数
	uint8_t		SignCode[MAX_SIGNCODE_TOTAL];//采集信号编码
}AcpVehicleCondCommandList_t;

/* 车况【查询结果】数据结构 */
//信号编码参数
typedef union{
	uint16_t SignCodeBit;
 	struct{
		uint8_t	SignCodeL	:8;	//信号编码低位
		uint8_t SignCodeH 	:5; //信号编码高位
		uint8_t Signflag	:3;	//信号标识(0:有效1:无效2:预留3:预留)
  	}codeBit;
}SignCodeBit_U;
typedef struct
{
	SignCodeBit_U 	SignCode; //采集信号编码
	uint8_t			SignData[MAX_SIGNDATA_LEN];//采集信号编码对应值
}SignCode_t;
typedef struct
{
	uint8_t		SendMode;	//发送方式0,即时发送 1,补发
	uint8_t 	CollectPacketSN;//采集包序号
	TimeStamp_t		CollectTime;//采集时间(需拆分数据元素组装)
	uint16_t		SignTotal;	//采集信号总数
	SignCode_t		SignCodeData[MAX_SIGNCODE_TOTAL];//采集信号编码和值
}AcpVehicleCondition_RawData;


//GPS信息数据结构
typedef struct
{
	uint16_t 	degree		:9;	//方向
	uint8_t 	second		:6;	//秒
	uint8_t 	minute		:6;	//分 
	uint8_t 	hour		:5;	//时
	uint8_t 	day			:5; //日
	uint8_t 	month		:4; //月
	uint8_t 	year		:6; //年
	uint32_t	altitude	:14;	//高度
	uint8_t		latitudeState	:2;	//纬度状态
	uint32_t	latitude		:29;	//纬度
	uint8_t		longitudeState	:2;	//经度状态
	uint32_t	longitude 		:29;	//经度
	uint8_t		GPSState		:2;	//定位状态
	uint8_t						:1;	//保留
}GPSPositionData_t;

//剩余油量数据结构
typedef struct
{
	uint8_t 	RemainedOilValue;//剩余油量数值
	uint8_t		RemainedOilGrade : 4;//剩余油量等级
	uint8_t						 : 4;//预留
}RemainedOil_t;


//车门状态数据结构
typedef union
{
	uint8_t CarDoorState;
 	struct{
		uint8_t 	drivingDoor		:1;	//驾驶门开关状态
		uint8_t 	copilotDoor		:1;	//副驾驶门开关状态
		uint8_t 	leftRearDoor	:1;	//左后门开关状态
		uint8_t 	rightRearDoor	:1;	//右后门开关状态
		uint8_t 	rearCanopy		:1; //后舱盖开关状态
		uint8_t 	engineCover		:1; //引擎盖开关状态
	}bitState;
}CarDoorState_t;
//车锁状态数据结构
typedef union
{
	uint8_t CarLockState;
 	struct{
		uint8_t 	rightRearLock	:2;	//右后门车锁状态
		uint8_t 	leftRearLock	:2;	//左后门车锁状态
		uint8_t 	copilotLock		:2;	//副驾驶车锁状态
		uint8_t 	drivingLock		:2;	//驾驶门车锁状态
	}bitState;
}CarLockState_t;

//车窗状态数据结构
typedef union
{
	uint16_t WindowState;
 	struct{
		uint16_t 	leftFrontWindow	:3;	//左前车窗状态
		uint16_t 	rightFrontWindow:3;	//右前车窗状态
		uint16_t 	leftRearWindow	:3;	//左后车窗状态
		uint16_t 	rightRearWindow	:3;	//右后车窗状态
		uint16_t					:4;	//保留
	}bitState;
}WindowState_t;
//车灯状态数据结构
typedef union
{
	uint8_t	CarlampState;
	struct{
		uint8_t 	headlights		:1;	//远光灯状态
		uint8_t 	positionlights	:1;	//位置灯状态
		uint8_t 	nearlights		:1;	//近光灯状态
		uint8_t 	rearfoglights	:1;	//后雾灯状态
		uint8_t 	frontfoglights	:1;	//前雾灯状态
	}bitState;
}CarlampState_t;
//轮胎信息数据结构
typedef struct
{
	uint8_t 	rightrearTyrePress	;	//右后胎压压力
	uint8_t 	leftrearTyrePress	;	//左后胎压压力
	uint8_t 	rightfrontTyrePress	;	//右前胎压压力
	uint8_t 	leftfrontTyrePress	;	//左前胎压压力
	
	uint8_t 	rightrearTemperature;	//右后胎压温度
	uint8_t 	leftrearTemperature	;	//左后胎压温度
	uint8_t 	rightfrontTemperature;	//右前胎压温度
	uint8_t 	leftfrontTemperature;	//左前胎压温度
}TyreState_t;

//实时方向盘转角数据结构
typedef union
{
	uint16_t	WheelState;
	struct{
		uint16_t 	wheeldegree 	:15;//方向盘转角(度)
		uint8_t 	wheeldirection	:1;	//方向盘方向
	}bitState;
}WheelState_t;

//空调相关信息数据结构
typedef struct
{
	uint8_t 	airconditionerState	:1;	//空调工作状态
	uint8_t 	compressorState		:1;	//空调压缩机工作状态
	uint8_t 	autoState			:1;	//空调Auto状态
	uint8_t 	defrostState		:1;	//空调强制除霜状态
	uint8_t 	inOutCirculateState	:1;	//空调内外循环状态
	uint8_t 	blowingLevel		:3;	//空调吹风等级
	uint8_t 	blowingMode 		:3;	//空调吹风模式
	uint8_t 	Temperature			:7;	//空调温度设定
	uint8_t 						:6;	//保留
}AirconditionerInfo_t;

//持续时间信息数据结构(事件结束时发送真实时间.默认发送初始值)
typedef struct
{
	uint8_t 	accelerateTime	;	//急加速持续时间
	uint8_t 	decelerateTime	;	//急减速持续时间
	uint8_t 	wheelTime		;	//急转弯持续时间
	uint16_t 	overspeedTime 	;	//超速持续时间
}KeepingstateTime_t;
//动力电池状态数据结构
typedef struct
{
	uint16_t 	voltage		;	//电池电压
	uint16_t 	electricity	;	//电池电流
	uint8_t 	quantity	;	//电池电量
	uint8_t 	elecSOH		;	//电池SOH
	uint8_t 	elecTempera	;	//电池进水口温度
	uint8_t 	BatAveraTempera	;	//电池包平均温度
}PowerCellsState_t;
//充电状态数据结构
typedef struct
{
	uint8_t 	chargeState		;	//充电状态
	uint8_t 	remainChargeTime;	//剩余充电时间
	uint8_t		chargeMode	:	2;	//充电模式
	uint8_t					:	6;	//预留
}ChargeState_t;

//Yaw传感器信息数据结构
typedef struct
{
	uint16_t YawVelocity		:12;	//横摆角速度
	uint8_t				:4;	
	uint16_t LongituAccele		:12;	//纵向加速度
	uint8_t				:4;
	uint16_t TransverseAccele	:12;	//横向加速度
	uint8_t				:4;
}YaWSensorInfoSwitch_t;

//环境温度数据结构
typedef struct
{
	uint16_t AmbientTemperat	:11;	//环境温度
	uint8_t 			:5;
}AmbientTemperat_t;

//纯电动继电器及线圈相关状态
typedef struct
{
	uint8_t MainPositRelayCoilState	:1;//主正继电器线圈状态
	uint8_t MainNegaRelayCoilState	:1;//主负继电器线圈状态
	uint8_t PrefillRelayCoilState	:1;//预充继电器线圈状态
	uint8_t RechargePositRelayCoilState	:1;//充电正继电器线圈状态
	uint8_t RechargeNegaRelayCoilState	:1;//充电负继电器线圈状态
	uint8_t				:3;//预留
}PureElecRelayState_t;

//新能源热管理相关请求
typedef struct
{
	uint8_t 	BatteryHeatRequest :1;	//电池加热请求
	uint8_t		Motor1CoolRequest  :3;	//电机1冷却请求
	uint8_t		Motor2CoolRequest  :3;	//电机2冷却请求
	uint8_t 			   :1;	//预留
}NewEnergyHeatManage_t;

//车辆工作模式
typedef struct
{
	uint8_t 	VehWorkMode	:4;	//车辆工作模式
	uint8_t				:4;	//预留
}VehWorkMode_t;

//电机工作状态
typedef struct
{
	uint8_t 	Motor1Workstate	:3;	//电机1工作状态
	uint8_t 	Motor2Workstate	:3;	//电机2工作状态
	uint8_t 			:2;	//预留
}MotorWorkState_t;

/*车况数据信号*/
typedef struct
{
	GPSPositionData_t	GPSData;		//GPS信息数据
	RemainedOil_t		RemainedOil;	//剩余油量
	uint32_t			Odometer;		//总里程
	uint16_t			Battery;		//蓄电池电量
	uint16_t			CurrentSpeed;	//实时车速
	uint16_t			LTAverageSpeed;	//长时平均车速
	uint16_t			STAverageSpeed;	//短时平均车速
	uint16_t			LTAverageOil;	//长时平均油耗
	uint16_t			STAverageOil;	//短时平均油耗
	CarDoorState_t		CarDoorState;	//车门状态数据
	CarLockState_t		CarLockState;	//车锁状态数据
	uint8_t 			sunroofState;	//天窗状态(0x0关闭,0x1开启,0x2翘起,0x3其他)
	WindowState_t		WindowState;	//车窗状态数据
	CarlampState_t		CarlampState;	//车灯状态数据
	TyreState_t			TyreState;		//轮胎信息数据
	uint8_t				VerTboxMCU[12];	//TBox_MCU版本
	uint8_t 			EngineState;	//发动机状态(0x0关闭,0x1起动机拖动,0x2发动机运行,0x3正在停机,0x4-0x6保留,0x7无效)
	WheelState_t		WheelState;		//实时方向盘转角数据
	uint16_t 			PastRecordSpeed[19];//历史车速Kph(记录19次车速)
	WheelState_t 		PastRecordWheel[19];//历史方向盘转角(记录19次车速)
	uint16_t			EngineSpeed;		//发动机转速
	uint8_t 			Gearstate;			//档位信息(0x0:P,0x1:R,0x2:N,0x3:D,0x4:S,0x5:M,0x6:W,0x7:NoConnect,0x8-0xE:保留,0xF:无效)
	uint8_t 			HandbrakeState;		//手刹状态(0x0:未激活,0x1:激活,0x2:保留,0x3:错误)
	uint8_t 			ParkingState;		//驻车状态(0x0:加紧,0x1:正在加紧/释放,0x2:释放,0x3:错误)
	uint8_t 			Powersupplymode;	//供电模式0x0--0x8,0x9---0xF
	uint8_t 			Safetybeltstate;	//安全带状态0x0:未锁,0x1:锁止,0x2/0x3:保留
	uint16_t			RemainUpkeepMileage;//剩余保养里程
	AirconditionerInfo_t AirconditionerInfo;//空调相关信息
	KeepingstateTime_t	KeepingstateTime;	//持续时间信息
	PowerCellsState_t	PowerCellsState;	//动力电池状态
	ChargeState_t		ChargeState;		//充电状态数据
	uint8_t			VerTboxOS[12];		//TBox_OS版本
	uint8_t 		VerIVI[16];		//IVI版本
	uint8_t 		ChargeConnectState;	//充电枪连接状态
	uint8_t			BrakePedalSwitch;	//制动踏板开关
	uint8_t			AcceleraPedalSwitch;	//加速踏板开关
	YaWSensorInfoSwitch_t	YaWSensorInfoSwitch;	//Yaw传感器信息
	AmbientTemperat_t	AmbientTemperat;	//环境温度
	PureElecRelayState_t	PureElecRelayState;	//纯电动继电器及线圈相关状态
	uint16_t		ResidualRange;		//剩余续航里程(12bit)
	NewEnergyHeatManage_t	NewEnergyHeatManage;	//新能源热管理相关请求
	VehWorkMode_t		VehWorkMode;		//车辆工作模式
	MotorWorkState_t	MotorWorkState;		//电机工作状态
	uint8_t 		HighVoltageState;	//高压系统准备状态
}VehicleCondData_t;


/**************************************************************************************************
*****远程配置
**************************************************************************************************/
#define		Unit_TBox	0
#define		Unit_ECU	1
#define		MAX_DEVICE_NO	32	//设备编码(车辆上的电子单元TBox,ECU等)
#define		MAX_DEVICE_CONFIGITEM	64	//设备配置项数量

/* 预约充电时间表 */
typedef struct{
	uint8_t	 ScheduChargStartData;	//预约充电开始日
	uint16_t ScheduChargStartTime;	//预约充电开始时间,单位分钟
	uint8_t	 ScheduChargEndData;	//预约充电结束日
	uint16_t ScheduChargEndTime;	//预约充电结束时间,单位分钟
}AppointmentCharge_U;

/* 远程配置数据结构 */
typedef struct
{
	uint16_t	DeviceNo;//设备编号
	uint16_t 	EngineStartTime	;	//发动机远程启动时长,单位min
	uint8_t 	SamplingSwitch	;	//电动车国标抽检开关
	uint8_t		EmergedCall[15]	;	//紧急服务号码配置(ASCII码)
	uint8_t		WhitelistCall[15];	//白名单号码配置(ASCII码)
	uint8_t		CollectTimeInterval; //车况定时采集间隔时间,单位S,默认10
	uint8_t		ReportTimeInterval;  //车况定时上报间隔时间,单位S,默认10
	uint8_t		CollectTimeGpsSpeedInterval; //定时采集(位置和车速)间隔时间,单位S,默认5
	uint8_t		ReportTimeGpsSpeedInterval;  //定时上报(位置和车速)间隔时间,单位S,默认5
	uint8_t		ChargeMode;			 	//充电模式
	AppointmentCharge_U	ChargeSchedule; //预约充电时间表
	uint8_t 	LoadCell[15];			//道路救援紧急呼救号码（B-Cell）
	uint8_t 	InformationCell[15];	//信息咨询号码（I-Cell）
}RemoteDeviceConfig_t;
/*	远程配置数据结构体列表	*/
typedef struct
{
	uint8_t 		ConfigItemNum;	//配置项编号
	uint8_t 		ConfigItemData[MAX_CONFIGDATA_LEN];	//配置项值
}ConfigItem_t;
typedef struct
{
	uint16_t		DeviceNo;//设备编号
	uint8_t 		ConfigCount;//配置项数量
	ConfigItem_t	ConfigItem[MAX_DEVICE_CONFIGITEM];
}AcpConfigItem_t;
typedef struct
{
	uint8_t			DeviceTotal;//配置的设备总数
	AcpConfigItem_t	DeviceConfig[MAX_DEVICE_NO];//设备配置
}AcpRemoteConfig_RawData;

/**************************************************************************************************
*****远程升级
**************************************************************************************************/

/* 远程升级数据结构 */
typedef struct
{
	uint8_t 	DeviceNo;	//设备编号(0:TBox_OS,1:TBox_MCU,2:IVI)
	uint8_t		NeedCheck;	//是否需要用户确认
	uint8_t		UpdateFile_MD5[16];	//升级文件MD5值
	uint8_t		UpdateFile_Len;		//升级文件长度
	uint8_t		UpdateFile_Param[MAX_UPGRADEPARAM_LEN];	//升级参数(ASCII码)
	uint8_t		Ver_Len;			//版本号长度
	uint8_t		VerNo[MAX_UPGRADEPARAM_LEN];				//版本号(ASCII码)
	uint8_t		URL_Len;			//URL长度
	uint8_t		URL_Param[MAX_UPGRADEPARAM_LEN];			//URL地址(ASCII码)
}AcpRemoteUpgrade_t;
#if 0
/* 远程升级【命令返回】数据结构 */
typedef struct
{
	uint8_t 	DeviceNo;			//设备编号(0:TBox_OS,1:TBox_MCU,2:IVI)
	uint8_t		UpdateFile_Len;		//升级参数长度
	uint8_t		UpdateFile_Param[MAX_UPGRADEPARAM_LEN];	//升级参数(ASCII码)
	uint8_t		Ver_Len;			//版本号长度
	uint8_t		VerNo[MAX_UPGRADEPARAM_LEN];//版本号
	uint8_t		CommandResponse;	//命令接收状态0成功1失败
}AcpRemoteUpgrade_Response_t;

/* 远程升级【下载开始】数据结构 */
typedef struct
{
	uint8_t 	DeviceNo;	//设备编号(0:TBox_OS,1:TBox_MCU,2:IVI)
	uint8_t		UpdateFile_Len;		//升级参数长度
	uint8_t		UpdateFile_Param[MAX_UPGRADEPARAM_LEN];	//升级参数(ASCII码)
	uint8_t		Ver_Len;			//版本号长度
	uint8_t		VerNo[MAX_UPGRADEPARAM_LEN];//版本号
	uint8_t		Reserve;			//默认0
}AcpRemoteUpgrade_DownloadStart_t;
/* 远程升级【下载结果】数据结构 */
typedef struct
{
	uint8_t 	DeviceNo;			//设备编号(0:TBox_OS,1:TBox_MCU,2:IVI)
	uint8_t		UpdateFile_Len;		//升级参数长度
	uint8_t		UpdateFile_Param[MAX_UPGRADEPARAM_LEN];	//升级参数(ASCII码)
	uint8_t		Ver_Len;			//版本号长度
	uint8_t		VerNo[MAX_UPGRADEPARAM_LEN];//版本号
	uint8_t		DownloadState;		//下载状态(0成功1失败,驾驶员未确认下载2失败,驾驶员取消下载3失败,URL错误4下载失败5MD5检测失败)
}AcpRemoteUpgrade_DownloadResult_t;

/* 远程升级【升级开始】数据结构 */
typedef struct
{
	uint8_t 	DeviceNo;			//设备编号(0:TBox_OS,1:TBox_MCU,2:IVI)
	uint8_t		UpdateFile_Len;		//升级参数长度
	uint8_t		UpdateFile_Param[MAX_UPGRADEPARAM_LEN];	//升级参数(ASCII码)
	uint8_t		Ver_Len;			//版本号长度
	uint8_t		VerNo[MAX_UPGRADEPARAM_LEN];//版本号
	uint8_t		Reserve;			//默认0
}AcpRemoteUpgrade_UpdateStart_t;
/* 远程升级【升级结果】数据结构 */
typedef struct
{
	uint8_t 	DeviceNo;			//设备编号(0:TBox_OS,1:TBox_MCU,2:IVI)	
	uint8_t		UpdateFile_Len;		//升级参数长度
	uint8_t		UpdateFile_Param[MAX_UPGRADEPARAM_LEN];	//升级参数(ASCII码)
	uint8_t		Ver_Len;			//版本号长度
	uint8_t		VerNo[MAX_UPGRADEPARAM_LEN];				//版本号
	uint8_t		UpdateResult;		//升级结果(0成功1失败,车主未确认升级2失败,车主取消升级3失败,升级过程失败)
}AcpRemoteUpgrade_UpdateResult_t;
#endif
/**************************************************************************************************
*****紧急救援数据上传
** 使用 车况查询结果的数据结构 AcpVehicleCondition_RawData, 上传所有车况信号数据
**************************************************************************************************/

/**************************************************************************************************
*****车况上报配置
** 使用 车况查询结果的数据结构 AcpVehicleCondition_RawData
** 触发条件：1）车辆唤醒
			 2）车辆上电
			 3）车辆启动
			 4）行程开始事件
			 5）行驶过程定时上传
			 6）急减速开始事件
			 7）急减速结束事件
			 8）急加速开始事件
			 9）急加速结束事件
			 10）急转弯开始事件
			 11）急转弯结束事件
			 12）超速事件开始
			 13）超速事件结束
			 14）车辆熄火事件
			 15）行程结束事件
			 16）车辆下电
			 以上列表事件上传车况信号列表
			 17）位置和车速定时上传信号列表“编号0”（GPS位置信号），“编号4”（实时车速）
			 18）故障事件上传：当时的车况和故障信号
**************************************************************************************************/


/**************************************************************************************************
*****车辆定位
** 使用 车况【查询指令】数据结构 AcpVehicleCondition_RawData.
** 指令设定为：
		CollectTime	=	0;	//采集时间间隔ms 
		CollectCount=	1;	//采集次数
		SignTotal	=	1;	//采集信号总数
		SignCode[0]	=	0;  //采集信号编码
**************************************************************************************************/

/**************************************************************************************************
*****车辆定位数据上报
** 使用 车况查询结果的数据结构 AcpVehicleCondition_RawData,上传编号=0的GPS位置信号数据
**************************************************************************************************/

/**************************************************************************************************
*****车辆告警
** 使用 车况查询结果的数据结构 AcpVehicleCondition_RawData
** 触发条件：
   1)车门入侵告警
   2)车辆异动告警
   3)保养提醒
   4)低油量告警
   5)紧急熄火
**************************************************************************************************/
/**************************************************************************************************
*****故障信号列表结构
**************************************************************************************************/
//故障信号编码ID列表
typedef enum
{
	ACPCODEFAULT_EMSID 			= 	100,	//发动机管理系统故障
    ACPCODEFAULT_TCUID 			= 	101,	//变速箱控制单元故障
    ACPCODEFAULT_EmissionID 	= 	102,	//排放系统故障
    ACPCODEFAULT_SRSID			=	103,	//安全气囊系统故障
	ACPCODEFAULT_ESPID			=	104,	//电子稳定系统故障
	ACPCODEFAULT_ABSID			=	105,	//防抱死刹车系统
	ACPCODEFAULT_EPASID			=	106,	//电子助力转向系统
	ACPCODEFAULT_OilPressureID	=	107,	//机油压力报警
	ACPCODEFAULT_LowOilID		=	108,	//油量低报警
	ACPCODEFAULT_BrakeFluidLevelID	=	109, //制动液位报警
	ACPCODEFAULT_BatteryChargeID	=	110,//蓄电池充电故障
	ACPCODEFAULT_BBWID			=	111,	//制动系统故障
	ACPCODEFAULT_TPMSID			=	112,	//胎压系统故障
	ACPCODEFAULT_STTID			=	113,	//启停系统故障
	ACPCODEFAULT_ExtLightID		=	114,	//外部灯光故障
	ACPCODEFAULT_ESCLID			=	115,	//电子转向柱锁故障
	ACPCODEFAULT_EngineOverwaterID	=	116,//发动机水温过高
	ACPCODEFAULT_ElecParkUnitID		=	117,//电子驻车单元系统故障
	ACPCODEFAULT_AHBID			=	118,	//智能远光系统故障
	ACPCODEFAULT_ACCID			=	119,	//自适应巡航系统故障
	ACPCODEFAULT_FCWSID			=	120,	//前碰撞预警系统故障
	ACPCODEFAULT_LDWID			=	121,	//道路偏离预警系统故障
	ACPCODEFAULT_BlindSpotDetectID	=	122,//盲区检测系统故障
	ACPCODEFAULT_AirconManualID		=	123,//空调人为操作
	ACPCODEFAULT_HVSystemID			=	124,//高压系统故障
	ACPCODEFAULT_HVInsulateID		=	125,//高压绝缘故障
	ACPCODEFAULT_HVILID			=	126,	//高压互锁故障
//	ACPCODEFAULT_BatteryChargeID=	127,	//蓄电池充电故障
	ACPCODEFAULT_EVCellID		=	128,	//动力电池故障
	ACPCODEFAULT_PowerMotorID	=	129,	//动力电机故障
	ACPCODEFAULT_EParkID		=	130,	//E-Park故障
	ACPCODEFAULT_EVCellLowBatteryID		=	131,//动力电池电量过低报警
	ACPCODEFAULT_EVCellOverTemperateID	=	132,//动力电池温度过高报警
	ACPCODEFAULT_PowerMotorOverTemperateID	=	133,//动力电机温度过高报警
	ACPCODEFAULT_ConstantSpeedSystemFailID	= 	134,//定速巡航系统故障
	ACPCODEFAULT_ChargerFaultID		= 	135,//充电机故障
	ACPCODEFAULT_AirFailureID		= 	136,//空调故障
	ACPCODEFAULT_AlternateAuxSystemFailID   = 	137,//换道辅助系统故障
	ACPCODEFAULT_AutoEmergeSystemFailID	= 	138,//自动紧急制动系统故障
	ACPCODEFAULT_ReverRadarSystemFailID	= 	139,//倒车雷达系统故障
	ACPCODEFAULT_ElecGearSystemFailID	= 	140,//电子换档器系统故障
	ACPCODEFAULT_TyreAlarmID		= 	141,//轮胎报警
	ACPCODEFAULT_DCDCConverterFaultID	= 	142,//直流直流转换器故障
	ACPCODEFAULT_VehControllerFailID	= 	143,//整车控制器故障
	ACPCODEFAULT_PureElecRelayCoilFualtID	= 	144,//纯电动继电器及线圈相关故障

	ACPCODEFAULT_TSROverSpeedAlarmID	= 	500,//TSR超速报警
	ACPCODEFAULT_TSRLimitSpeedID		= 	501,//TSR限速
	ACPCODEFAULT_AEBInterventionID	 	= 	502,//AEB介入
	ACPCODEFAULT_ABSInterventionID	 	= 	503,//ABS介入
	ACPCODEFAULT_ASRInterventionID	 	= 	504,//ASR介入
	ACPCODEFAULT_ESPInterventionID	 	= 	505,//ESP介入
	ACPCODEFAULT_DSMAlarmID			= 	506,//DSM报警
	ACPCODEFAULT_TowHandOffDiskID		= 	507,//双手离开方向盘提示
	ACPCODEFAULT_ACCStateID			= 	508,//ACC状态
	ACPCODEFAULT_ACCSetSpeedID		= 	509,//ACC设定速度
	ACPCODEFAULT_FCWAlarmID			= 	510,//FCW报警
	ACPCODEFAULT_FCWStateID			= 	511,//FCW状态
	ACPCODEFAULT_FCWAlarmAccePedalFallID	= 	512,//FCW报警后加速踏板陡降时长
	ACPCODEFAULT_FCWAlarmFirstBrakeID	= 	513,//FCW报警后首次刹车时长
	ACPCODEFAULT_LDWStateID			= 	514,//LDW状态
	ACPCODEFAULT_LDWAlarmID			= 	515,//LDW报警
	ACPCODEFAULT_LDWAlarmDireDiskResponseID = 	516,//LDW报警后方向盘响应
	ACPCODEFAULT_LKAStateID			=	517,//LKA状态
	ACPCODEFAULT_LKAInterventionID		= 	518,//LKA介入
	ACPCODEFAULT_LKADriverTakeOverPromptID	= 	519,//LKA驾驶员接管提示
	ACPCODEFAULT_LKADriverResponsID		= 	520,//LKA驾驶员接管提示后方向盘响应时
	ACPCODEFAULT_BSDStateID			= 	521,//BSD状态
	ACPCODEFAULT_BSDLeftSideAlarmID		= 	522,//BSD左侧报警
	ACPCODEFAULT_BSDRightSideAlarmID	= 	523,//BSD右侧报警
	ACPCODEFAULT_BSDAlarmReftWheelRespID	= 	524,//BSD报警后方向盘响应时长
	ACPCODEFAULT_BSDAlarmFirstBrakeID	= 	525,//BSD报警后首次刹车时长
	ACPCODEFAULT_BSDAlarmPedalAcceID	= 	526,//BSD报警后加速踏板开始陡降时长
	ACPCODEFAULT_CrossLeftReportID		= 	527,//交叉车流预警左侧报警
	ACPCODEFAULT_CrossRightReportID		= 	528,//交叉车流预警右侧报警
	ACPCODEFAULT_CrossAlarmWhellID		= 	529,//交叉车流预警后方向盘报警
	ACPCODEFAULT_CrossAlarmStopID		= 	530,//交叉车流预警报警后首次刹车时长
	ACPCODEFAULT_CrossAlarmAcceTreadleID	= 	531,//交叉车流预警后加速踏板开始陡降时长
	ACPCODEFAULT_AlterTrackAssistLeftAlarmID = 	532,//变道辅助左侧报警
	ACPCODEFAULT_AlterTrackAssistRightAlarmID= 	533,//变道辅助右侧报警
	ACPCODEFAULT_AlterTrackAssistDireRepsonID= 	534,//变道辅助报警后方向盘响应
	ACPCODEFAULT_AlterTrackAssistFirstStopID = 	535,//变道辅助报警后首次刹车时长
	ACPCODEFAULT_AlterTrackAssistAcceDropID  = 	536,//变道辅助报警后加速踏板开始陡降时长
}AcpCodeFaultID_E;



//轮胎报警数据结构
typedef struct
{
	uint8_t 	LeftFrontTirePressAlarm	:3;	//左前胎压报警
	uint8_t 	LeftFrontTireTempAlarm  :3;	//左前胎温报警
	uint8_t 				:2;	//预留
	uint8_t 	RightFrontTirePressAlarm:3;	//右前胎压报警
	uint8_t 	RightrontTireTempAlarm	:3;	//右前胎温报警
	uint8_t 				:2;	//预留
	uint8_t 	LeftRearTirePressAlarm	:3;	//左后胎压报警
	uint8_t 	LeftRearTireTempAlarm   :3;	//左后胎温报警
	uint8_t 				:2;	//预留
	uint8_t 	RightRearTirePressAlarm :3;	//右后胎压报警
	uint8_t 	RightRearTireTempAlarm	:3;	//右后胎温报警
	uint8_t 				:2;	//预留
}TyreAlarmState_t;

//纯电动继电器及线圈相关故障
typedef struct
{
	uint8_t 	MainPositRelayCoilFault :1;	//主正继电器线圈状态
	uint8_t 	MainNegaRelayCoilFault	:1;	//主负继电器线圈状态
	uint8_t 	PrefillRelayCoilFault	:1;	//预充继电器线圈状态
	uint8_t 	RechargePositRelayCoilFault :1; //充电正继电器线圈状态
	uint8_t 	RechargeNegaRelayCoilFault  :1;	//充电负继电器线圈状态
	uint8_t 	MainPositiveRelayFault	    :1;	//主正继电器故障
	uint8_t 	MainNagetiveRelayFault	    :1;	//主负继电器故障
	uint8_t 				    :1;	//预留
}PureElecRelayCoilFualt_t;

//故障信号结构
typedef struct
{
  uint8_t   ACPCODEFAULTEMSState	:2;	//发动机管理系统故障
  uint8_t   ACPCODEFAULTTCUState	:2;	//变速箱控制单元故障
  uint8_t   ACPCODEFAULTEmissionState:2;//排放系统故障
  uint8_t   ACPCODEFAULTSRSState	:2;	//安全气囊系统故障
  uint8_t   ACPCODEFAULTESPState	:2;	//电子稳定系统故障
  uint8_t   ACPCODEFAULTABSState	:2;	//防抱死刹车系统
  uint8_t   ACPCODEFAULTEPASState	:2;	//电子助力转向系统
  uint8_t   ACPCODEFAULTOilPressureState:2;	//机油压力报警
  uint8_t   ACPCODEFAULTLowOilIDState	:2;	//油量低报警
  uint8_t   ACPCODEFAULTBrakeFluidLevelState:2;	//制动液位报警
  uint8_t   ACPCODEFAULTBatteryChargeState	:2;	//蓄电池充电故障
  uint8_t   ACPCODEFAULTBBWState	:2;	//制动系统故障
  uint8_t   ACPCODEFAULTTPMSState	:2;	//胎压系统故障
  uint8_t   ACPCODEFAULTSTTState	:2;	//启停系统故障
  uint8_t   ACPCODEFAULTExtLightState	:2;	//外部灯光故障
  uint8_t   ACPCODEFAULTESCLState	:2;	//电子转向柱锁故障
  uint8_t   ACPCODEFAULTEngineOverwaterState:2;	//发动机水温过高
  uint8_t   ACPCODEFAULTElecParkUnitState	:2;	//电子驻车单元系统故障
  uint8_t   ACPCODEFAULTAHBState	:2;	//智能远光系统故障
  uint8_t   ACPCODEFAULTACSState	:2;	//自适应巡航系统故障
  uint8_t   ACPCODEFAULTFCWSState	:2;	//前碰撞预警系统故障
  uint8_t   ACPCODEFAULTLDWState	:2;	//道路偏离预警系统故障
  uint8_t   ACPCODEFAULTBlindSpotDetectState:2;	//盲区检测系统故障
  uint8_t   ACPCODEFAULTAirconManualState	:2;	//空调人为操作
  uint8_t   ACPCODEFAULTHVSystemState	:2;	//高压系统故障
  uint8_t   ACPCODEFAULTHVInsulateState	:2;	//高压绝缘故障
  uint8_t   ACPCODEFAULTHVILState	:2;	//高压互锁故障
 // uint8_t   ACPCODEFAULTBatteryChargeState:2;
  uint8_t   ACPCODEFAULTEVCellState	:2;	//动力电池故障
  uint8_t   ACPCODEFAULTPowerMotorState:2;	//动力电机故障
  uint8_t   ACPCODEFAULTEParkState	:2;	//E-Park故障
  uint8_t   ACPCODEFAULTEVCellLowBatteryState	:2;	//动力电池电量过低报警
  uint8_t   ACPCODEFAULTEVCellOverTemperateState:2;	//动力电池温度过高报警
  uint8_t   ACPCODEFAULTPowerMotorOverTemperateState:2;	//动力电机温度过高报警

  uint8_t   ACPCODEFAULTConstantSpeedSystemFailState:2;	//定速巡航系统故障
  uint8_t   ACPCODEFAULTChargerFaultState	    :2;	//充电机故障
  uint8_t   ACPCODEFAULTAirFailureState		    :2; //空调故障
  uint8_t   ACPCODEFAULTAlternateAuxSystemFailState :2; //换道辅助系统故障
  uint8_t   ACPCODEFAULTAutoEmergeSystemFailState   :2;	//自动紧急制动系统故障
  uint8_t   ACPCODEFAULTReverRadarSystemFailState   :2;	//倒车雷达系统故障
  uint8_t   ACPCODEFAULTElecGearSystemFailState	    :2;	//电子换档器系统故障
  TyreAlarmState_t   	    ACPCODEFAULTTyreAlarmState; //轮胎报警
  uint8_t   ACPCODEFAULTDCDCConverterFaultState	    :2;	//直流直流转换器故障
  uint8_t   ACPCODEFAULTVehControllerFailState	    :2; //整车控制器故障
  PureElecRelayCoilFualt_t	ACPCODEFAULTPureElecRelayCoilState; //纯电动继电器及线圈相关故障

  uint8_t   ACPCODEFAULTTSROverSpeedAlarmState	    :2; //TSR超速报警
  uint8_t   ACPCODEFAULTTSRLimitSpeedState;		//TSR限速
  uint8_t   ACPCODEFAULTAEBInterventionState	    :1;	//AEB介入
  uint8_t   ACPCODEFAULTABSInterventionState	    :1;	//ABS介入
  uint8_t   ACPCODEFAULTASRInterventionState	    :1;	//ASR介入
  uint8_t   ACPCODEFAULTESPInterventionState	    :1;	//ESP介入
  uint8_t   ACPCODEFAULTDSMAlarmState		    :2;	//DSM报警
  uint8_t   ACPCODEFAULTTowHandOffDiskState	    :1;	//双手离开方向盘提示
  uint8_t   ACPCODEFAULTACCState		    :3;	//ACC状态
  uint8_t   ACPCODEFAULTACCSetSpeedState;		//ACC速度设定
  uint8_t   ACPCODEFAULTFCWAlarmState		    :2; //FCW报警
  uint8_t   ACPCODEFAULTFCWState		    :1;	//FCW状态
  uint8_t   ACPCODEFAULTFCWAlarmAccePedalFallState;	//FCW报警后加速踏板陡降时长
  uint8_t   ACPCODEFAULTFCWAlarmFirstBrakeState;	//FCW报警后首次刹车时长
  uint8_t   ACPCODEFAULTSLDWState		    :3;	//LDW状态
  uint8_t   ACPCODEFAULTLDWAlarmState		    :2; //LDW报警
  uint8_t   ACPCODEFAULTLDWAlarmDireDiskResponseState;  //LDW报警后方向盘响应
  uint8_t   ACPCODEFAULTLKAState		    :3; //LKA状态
  uint8_t   ACPCODEFAULTLKAInterventionState	    :2; //LKA介入
  uint8_t   ACPCODEFAULTLKADriverTakeOverPromptState :1;//LKA驾驶员接管提示
  uint8_t   ACPCODEFAULTLKADriverResponsState;		//LKA驾驶员接管提示后方向盘响应时
  uint8_t   ACPCODEFAULTLBSDState		    :1; //BSD状态
  uint8_t   ACPCODEFAULTBSDLeftSideAlarmState	    :2;	//BSD左侧报警
  uint8_t   ACPCODEFAULTBSDRightSideAlarmState	    :2;	//BSD右侧报警
  uint8_t   ACPCODEFAULTBSDAlarmReftWheelRespState;	//BSD报警后方向盘响应时长
  uint8_t   ACPCODEFAULTBSDAlarmFirstBrakeState;	//BSD报警后首次刹车时长
  uint8_t   ACPCODEFAULTBSDAlarmPedalAcceState;		//BSD报警后加速踏板开始陡降时长
  uint8_t   ACPCODEFAULTCrossLeftReportState	    :1; //交叉车流预警左侧报警
  uint8_t   ACPCODEFAULTCrossRightReportState	    :1; //交叉车流预警右侧报警
  uint8_t   ACPCODEFAULTCrossAlarmWhellState;		//交叉车流预警后方向盘报警时长
  uint8_t   ACPCODEFAULTCrossAlarmStopState;		//交叉车流预警报警后首次刹车时长
  uint8_t   ACPCODEFAULTCrossAlarmAcceTreadleState;	//交叉车流预警后加速踏板开始陡降时长
  uint8_t   ACPCODEFAULTAlterTrackAssistLeftAlarmState  :2;//变道辅助左侧报警
  uint8_t   ACPCODEFAULTAlterTrackAssistRightAlarmState :2;//变道辅助右侧报警
  uint8_t   ACPCODEFAULTAlterTrackAssistDireRepsonState;//变道辅助报警后方向盘响应
  uint8_t   ACPCODEFAULTAlterTrackAssistFirstStopState; //变道辅助报警后首次刹车时长
  uint8_t   ACPCODEFAULTAlterTrackAssistAcceDropState;	//变道辅助报警后加速踏板开始陡降时长

}AcpCodeFault_t;


/**************************************************************************************************
*****远程诊断配置表

**************************************************************************************************/
/* 远程诊断【命令请求】数据结构 */
typedef struct
{
	uint8_t 	DiagnoseTotal;		//请求诊断控制器总数
	uint8_t		DiagnoseCode[MAX_DIAGNOSECODE];//诊断控制器编码
}RemoteDiagnose_t;

/* 远程诊断结果【诊断应答】数据结构 */
/* 1:发动机控制单元，2:变速箱控制单元*/
typedef struct
{
	uint8_t 	DiagnoseCode;		//诊断控制器编码
	uint8_t 	DiagnoseCodeFaultTotal;	//诊断控制器编码故障总数
	uint8_t 	DiagnoseCodeFault[MAX_DIAGNOSECODEFAULT];	//诊断控制器编码故障
}DiagnoseCodeFault_t;
typedef struct
{
	uint8_t 			DiagnoseTotal;		//应答诊断控制器总数
	DiagnoseCodeFault_t	DiagnoseCodeFault[MAX_DIAGNOSECODE];//诊断控制器编码故障
}RemoteDiagnoseResult_t;


typedef enum
{
	ACPCODE_ENGINECONTROLUNIT1	= 1,	//发动机控制单元1
	ACPCODE_ENGINECONTROLUNIT2	= 2,	//发动机控制单元2
	ACPCODE_TRANSCONTROLUNIT	= 3,	//变速箱控制单元
	ACPCODE_VEHICLECONTROLUNIT	= 4,	//整车控制器

	ACPCODE_SHIFTCONTROLUNIT	= 9,	//换挡控制器
	ACPCODE_SHIFTPERFORMUNIT	= 10,	//换挡执行器
	ACPCODE_DCTOILPUMPUNIT		= 11,	//DCT油汞

	ACPCODE_FORUDRIVECONTROLUNIT	= 13,	//四驱控制器

	ACPCODE_ATTACHPACKCONTROLUNIT	= 15,	//传动系驻车控制器

	ACPCODE_MOTORCONTROLUNIT	= 17,	//电机控制器
	ACPCODE_MOTORCONTROLUNIT2	= 18,	//电机控制器2

	ACPCODE_BATTERYCONTROLUNIT	= 25,	//电池控制器
	ACPCODE_DCTRANFORMUNIT		= 26,	//直流转换器
	ACPCODE_CARCHARGERUNIT		= 27,	//车载充电机
	
	ACPCODE_PARKINGCONTROLUNIT	= 33,	//驻车控制器
	ACPCODE_CORNERCONTROLUNIT	= 34,	//转角控制器
	ACPCODE_AIRSUSPENSIONUNIT	= 35,	//空气悬架
	ACPCODE_VEHSTABLECONTROLUNIT	= 37,	//车身稳定控制器
	ACPCODE_AUTODRIVERCONTROLUNIT	= 38,	//自动驾驶控制器
	ACPCODE_DRIVERMODESELECTUNIT	= 39,	//驾驶模式选择控制器
	
	ACPCODE_AIRBAGCONTROLUNIT	= 41,	//安全气囊控制器
	ACPCODE_STARTTHEFTCONTROLUNIT	= 42,	//一键启动/防盗控制器
	ACPCODE_ACSCONTROLSYSTEM	= 43,	//自适应巡航/多功能控制系统
	ACPCODE_LOADOFFSETCONTROLUNIT	= 44,	//车道偏离控制器
	ACPCODE_BLIINDPROBEUNIT		= 45,	//盲区探测/换道辅助
	ACPCODE_REVERRADARSYSTEMUNIT	= 46,	//倒车雷达/半自动泊车
	ACPCODE_PANORAMICIMAGEUNIT	= 47,	//全景影像控制器
	ACPCODE_TYREPRESSSYSTEMUNIT	= 48,	//胎压系统
	ACPCODE_ADAPTHEADLIGHTUNIT	= 49,	//自适应大灯

	ACPCODE_ACPCODE_BLIINDPROBEUNIT2= 53,	//盲区探测2

	ACPCODE_NIGHTVISIONSYSTEMUNIT	= 56,	//夜视系统
	ACPCODE_VEHCONTROLUNIT1		= 57,	//车身控制器1
	ACPCODE_VEHCONTROLUNIT2		= 58,	//车身控制器2
	ACPCODE_COMBINATIONSWITCHUNIT	= 59,	//组合开关
	ACPCODE_STEERCOLUMNCONTROLUNIT	= 60,	//转向柱调节控制器
	ACPCODE_DRIVERDOORCONTROLUNIT	= 61,	//驾驶员车门控制器
	ACPCODE_COPILOPDOORCONTROLUNIT	= 62,	//副驾驶员车门控制器
	ACPCODE_LEFTRAERDOORCONTROLUNIT	= 63,	//左后车门控制器
	ACPCODE_RIGHTRAERDOORCONTROLUNIT= 64,	//右后车门控制器
	ACPCODE_DRIVERseatCONTROLUNIT	= 65,	//驾驶员座椅控制器
	ACPCODE_REARSEATCONTROLUNIT	= 66,	//后座椅控制器
	ACPCODE_SEATENVIRONCONTROLUNIT	= 67,	//座椅环境控制器
	ACPCODE_COPILOPSEATCONTROLUNIT	= 68,	//副驾驶座椅控制器
	ACPCODE_LUGGAGECONTROLUNIT	= 69,	//行李箱控制器

	ACPCODE_ACCONTROLUNIT1		= 73,	//空调控制器1
	ACPCODE_ACCONTROLUNIT2		= 74,	//空调控制器2
	ACPCODE_FRONTACPANELUNIT	= 75,	//前空调控制面板
	ACPCODE_REARACPANELUNIT		= 76,	//后空调控制面板

	ACPCODE_INSTRUMENTUNIT		= 81,	//仪表
	ACPCODE_ENTERTAINHOUTUNIT	= 82,	//娱乐主机
	ACPCODE_STEREOUNIT		= 83,	//音响
	ACPCODE_TBOXUNIT		= 84,	//TBOX
	ACPCODE_LOOKUPSHOWUNIT		= 85,	//抬头显示
	ACPCODE_VEHICLERECORDERUNIT	= 86,	//行车记录仪

	ACPCODE_GATEWAYCONTROLUNIT	= 89,	//网关控制器
	ACPCODE_BATTERYSENSORUNIT1	= 90,	//蓄电池传感器1
	ACPCODE_BATTERYSENSORUNIT2	= 91,	//蓄电池传感器2
	ACPCODE_ELECTRONICWIPERUNIT	= 92,	//电子雨刮控制器
	ACPCODE_SUNRAINCONTROLUNIT	= 93,	//阳光雨量控制器
	ACPCODE_SHADESCREENSUNIT	= 94,	//遮阳帘控制器
	ACPCODE_SKYLIGHTUNIT		= 95,	//天窗控制器
}RemoteDiagnoseCodeID_E;






