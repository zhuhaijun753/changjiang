 
#define FAW_SERVER_PORT				9503
//#define FAW_SERVER_IP               "202.98.11.132"
#define ACPPARAMETER_INFO_PATH         "/data/AcpPara"
#define DELETE_ACPPARAMETER            "rm -rf /data/AcpPara"

#define FAWACP_PROTOCOL_SOF			0x7D6E2C5F	//帧头
#define FAWACP_PROTOCOL_EOF			0x0D0C554F	//帧尾
#define MESSAGE_TYPE_START			0			//服务消息类型
#define DATA_BUFF_SIZE				1024*4
#define MALLOC_DATA_LEN				1024
#define	SIGNATURE_SIZE				20
#define DATA_SOFEOF_SIZE			4	//头尾数据长度
#define DATA_HEADER_SIZE			5	//数据头长度
#define DATA_TIMESTAMP_SIZE			7	//时间戳长度
#define DATA_AUTHTOKEN_SIZE			5	//AuthToken长度
#define DATA_SOURCE_SIZE			2	//Source指令源长度
#define DATA_ERRORCODE_SIZE			2	//错误码信息长度
#define ROOTKEY_SIZE				16

//Service Request
typedef enum
{
	TspRequestSource_Test 	= 	0,	//调试系统
    TspRequestSource_Admin 	= 	1,	//管理员Protal
    TspRequestSource_User 	= 	2,	//用户Protal
    TspRequestSource_App	=	3,	//手机App	
}ServiceRequestSourceID_E;
/**************************************************************************
**消息头
**************************************************************************/
//服务标识号
typedef enum
{
	ACPApp_AuthenID 			= 	0,	//连接认证
    ACPApp_HeartBeatID 			= 	1,	//连接保持(心跳)
    ACPApp_RemoteCtrlID 		= 	2,	//远程控制
	ACPApp_QueryVehicleCondID	=	3,	//车况查询
	ACPApp_RemoteConfigID		=	4,	//远程配置
	ACPApp_UpdateKeyID			=	5,	//RootKey更换
	ACPApp_RemoteUpgradeID		=	6,	//远程升级
	ACPApp_EmergencyDataID		=	7,	//紧急救援数据上报
	ACPApp_VehicleCondUploadID	=	8,	//车况数据上报
	ACPApp_GPSID				=	9, 	//车辆定位
	ACPApp_VehicleAlarmID		=	10,	//车辆告警
	ACPApp_RemoteDiagnosticID	=	11,	//远程诊断
}AcpAppID_E;
//加密标识
typedef enum
{
	AcpCryptFlag_NULL 			= 	0,	//非加密
	AcpCryptFlag_IS 			= 	1,	//加密
}AcpCryptFlag_E;
//车厂标识
typedef enum
{
	AcpCarManufactureID_JiLin 			= 	1,	//一汽吉林
	AcpCarManufactureID_JiaoChe 		= 	2,	//一汽轿车
	AcpCarManufactureID_HongQi 			= 	3,	//一汽红旗
	AcpCarManufactureID_TianJing 		= 	4,	//一汽天津
}AcpCarManufactureID_E;
//Application Header
typedef struct
{
	//服务标识号
	uint8_t Acp_AppID 	:6; //Acp的服务标识号
	uint8_t PrivateFlag	:1;	//私有标识
	uint8_t Reserved_ID	:1;	//保留位	

	//服务消息标识(测试+加密+服务)
	uint8_t MsgType 	:5; //服务消息类型
	uint8_t CryptFlag	:1;	//加密标识
	uint8_t TestFlag	:1;	//测试报文标识
	uint8_t Reserved_T 	:1;	//保留位

	//消息控制位
	uint8_t MsgCtrlFlag :4; //消息控制掩码(0000/0010)
	uint8_t Version		:3;	//版本号(set 0-7)
	uint8_t VersionFlag	:1;	//启用版本号标识(0:启用,1:不用)

	//长度定义
	uint16_t	MsgLength	:16;//消息长度(AppHeader+Element)
}AcpAppHeader_t;

/**************************************************************************
**					消息元素数据			**	
** 说明：消息元素分两种结构，长数据结构和短数据结构
**************************************************************************/
typedef union{
	uint8_t Element_Len;
	struct{ 
		uint8_t DataLen 	:5; //数据长度
		uint8_t MoreFlag	:1;	//数据长度标识选择
		uint8_t Identifier  :2; //元素数据标识
	}elementLenBit;
}ElementLenInfo;
typedef union{
	uint8_t Element_Len_Extend;
 	struct{
		uint8_t DataLen 	:7; //数据长度
		uint8_t MoreFlag	:1;	//数据长度标识选择(set 0)
	}elementLenBit;
}ElementLenInfo_Ext;
typedef struct
{
	ElementLenInfo Element_Len_High;
	ElementLenInfo_Ext Element_Len_Low;
}ElementLen_t;
/**************************************************************************
**					基础数据结构体
**************************************************************************/
//消息元素数据【车厂信息标识】
typedef struct
{
	ElementLenInfo	Element_LenInfo;	//长度
	uint8_t			CarManufactureID;	//车厂标识
}AcpCarManufactureID_t;
//消息元素数据【终端标识信息】
typedef struct
{
	ElementLenInfo	Element_LenInfo;//长度
	uint8_t			CarTU_ID[17];	//车厂标识VIN
}AcpTUID_t;
//消息元素数据【车辆信息】
typedef struct
{
	ElementLenInfo		Element_Len_High;	//长度值高位
	ElementLenInfo_Ext	Element_Len_Low;	//长度值低位
	uint8_t 			Vin[17];			//车辆识别码=车厂标识VIN
	uint8_t 			IMEI[15];			//SIM卡IMEI
	uint8_t 			ICCID[20];			//SIM卡ICCID(20字符,不足后补0x00)
	uint8_t 			IVISerialNumber[30];//IVI主机序列号(默认0,ASCII字符0x30,不足后补0x00)
	uint8_t 			TBoxSerialNumber[30];//Tbox序列号(不足后补0x00)
	uint8_t				RawData;			 //补充信息(选填)
}AcpVehicleDescriptor_t;
//消息元素数据【指令源信息】
typedef struct
{
	ElementLenInfo	Element_LenInfo;//长度
	uint8_t			SourceID;		//指令源值0-255
}AcpSourceID_t;
//消息元素数据【车辆身份令牌信息】
typedef struct
{
	ElementLenInfo	Element_LenInfo;//长度
	uint8_t			AuthToken[4];	//车辆身份令牌
}AcpAuthToken_t;
//消息元素数据【错误码信息】
typedef struct
{
	ElementLenInfo	Element_LenInfo;//长度
	uint8_t			ErrorCode;		//错误码
}AcpErrorCode_t;
//消息元素数据【时间戳信息】
typedef struct{
	uint8_t Year 	:6; //年(0=1990,1=1991,62=2052)
	uint8_t Month	:4;	//月
	uint8_t Day 	:5;	//日
	uint8_t Hour 	:5;	//时
	uint8_t Minutes :6;	//分
	uint8_t Seconds :6;	//秒
	uint16_t msSeconds :16;//毫秒0-1000
}TimeStamp_t;
typedef struct
{
	ElementLenInfo	Element_LenInfo;//长度
	TimeStamp_t		TimeStampInfo;
}AcpTimeStamp_t;

#if 0
//消息元素数据【服务请求信息】
typedef struct
{
	ElementLenInfo	Element_LenInfo;//长度
	uint8_t			RequestSourceID;//请求服务源(0:调试系统,1:管理员Protal,2:用户Protal,3:手机App)
	uint8_t			RequestRawData[32];//数据
}AcpServiceReq_t;
//消息元素数据【服务请求信息(长信息)】
typedef struct
{
	ElementLenInfo		Element_Len_High;	//长度值高位
	ElementLenInfo_Ext	Element_Len_Low;	//长度值低位
	uint8_t				RequestSourceID;	//请求服务源(0:调试系统,1:管理员Protal,2:用户Protal,3:手机App)
	uint8_t				RequestRawData[4096];//数据
}AcpServiceReqExt_t;
#endif

//消息元素数据【数据应答信息】
typedef struct
{
	ElementLenInfo	Element_LenInfo;//长度
	uint8_t			ResponseRawData[32];//数据
}AcpRawResponse_t;
//消息元素数据【数据应答信息(长信息)】
typedef struct
{
	ElementLenInfo		Element_Len_High;	//长度值高位
	ElementLenInfo_Ext	Element_Len_Low;	//长度值低位
	uint8_t				ResponseRawData[4096];//数据
}AcpRawResponseExt_t;

 
/**************************************************************************
**					连接认证逻辑结构
**************************************************************************/
//TBox认证请求【Auth Apply Message】
typedef struct
{
	AcpCarManufactureID_t	CarManufactureID;	//车厂标识
	AcpTUID_t				TU_VIN;				//车辆VIN
}AcpTBoxAuthApplyMsg_t;

//接收TSP的AKey【AKey Message】
typedef struct
{
	ElementLenInfo		Len_High;	//长度值高位
	ElementLenInfo_Ext	Len_Low;	//长度值低位
	uint8_t	AkeyC_Tsp[32];//Akey Msg数据(加密后的认证密钥[Send1])
	uint8_t	Rand1_Tsp[16];//Rand1(随机数[Send1])
}AcpTspAkeyMsg_t;

//TBox发起【CT Message】
typedef struct
{
	ElementLenInfo		Len_High;	//长度值高位
	ElementLenInfo_Ext	Len_Low;	//长度值低位
	uint8_t	Rand1CT_TBox[32];//Rand1CT数据(加密后的随机数(AKey_TBox Encrypt Rand1_Tsp)[Send2])
	uint8_t	Rand2_Tbox[16];  //Rand2(随机数[Send2])
}AcpTBoxCTMsg_t;

//接收TSP【CS Message】
typedef struct
{
	ElementLenInfo		Len_High;	//长度值高位
	ElementLenInfo_Ext	Len_Low;	//长度值低位
	uint8_t	Rand2CS_Tsp[32];//Rand2CS数据(随机数加密(AKey_TSP Encrypt Rand2_Tbox)[Send3])
	uint8_t	RT_Tsp;//RT(Rand1CS_Tsp == Rand1CT_TBox[Send3])
}AcpTspCSMsg_t;

//TBox发起【RS Message】
typedef struct
{
	ElementLenInfo	LenInfo;//长度
	uint8_t	Rand3_TBox[16];//Rand3数据(随机数[Send4])
	uint8_t	RS_Tbox;  //RS(Rand2CT_TBox == Rand2CS_Tsp[Send4])
}AcpTBoxRSMsg_t;

//接收TSP【SKey Message】
typedef struct
{
	ElementLenInfo		Len_High;	//长度值高位
	ElementLenInfo_Ext	Len_Low;	//长度值低位
	uint8_t	SkeyC_Tsp[32];//SkeyC数据(加密后的随机会话密钥(AKey_TSP Encrypt SKey_Tsp)[Send5])
	uint8_t	Rand3CS_Tsp[32];//Rand3CS(随机数加密(SKey_Tsp Encrypt Rand3_Tbox)[Send5])
}AcpTspSKeyMsg_t;

//TBox发起【Auth Ready Message】
typedef struct
{
	AcpVehicleDescriptor_t	AuthReadyMsg_VehicleDescriptor;//车辆信息
}AcpTBoxAuthReadyMsg_t;

//接收TSP【Auth Ready ACK Message】
typedef struct
{
	AcpAuthToken_t		ACKMsg_AuthToken;//车辆身份令牌
}AcpTspAuthReadyACKMsg_t;


/**************************************************************************************/
