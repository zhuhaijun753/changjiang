#ifndef _FAWACP_H_
#define _FAWACP_H_

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/syslog.h>
#include "FAWACP_Data.h"
#include "VehicleData.h"
#include "HMAC_MD5.h"
#include "HmacSHA-1.h"
#include "mcuUart.h"
#include "SQLiteDatabase.h"
#include "TBoxDataPool.h"
#include "GpioWake.h"
#include "common.h"
#include "LTEModuleAtCtrl.h"
#include "encryption_init.h"
#include "libnts_crypt_parse.h"


#ifndef FAWACP_DEBUG
	#define FAWACP_DEBUG 0
#endif


#if FAWACP_DEBUG
	#define FAWACPLOG(format,...) printf("### FAWACP ### %s [%d] "format"\n",__FUNCTION__,__LINE__,##__VA_ARGS__)
	#define FAWACP_NO(format,...) printf(format,##__VA_ARGS__)
#else

	#define FAWACPLOG(format,...)
	#define FAWACP_NO(format,...)
#endif

#define VERSION_FLAG	1		//0,测试;1,正式

#define TBOX_VERSION    "MPU_V1.2.01"//TBOX版本号12字节
#define IVI_VERSION		"IVI_1.0.01"//IVI版本号16字节
#define REISSUE_TIME	2	//数据补发时间间隔
#define SLEEPTIMEOUT	5
#define HEARTBEAT_TMIE	60	//心跳发送周期

typedef int (*callback_RemoteCtrlCmd)();
typedef int (*callback_RemoteConfigCmd)(uint8_t, uint16_t);
/* FAWACP data information */
typedef struct{
	uint8_t Vin[17];		//车辆识别码=车厂标识VIN
	uint8_t IMEI[15];		//SIM卡IMEI
	uint8_t ICCID[20];	        //SIM卡ICCID(20字符,不足后补0x00)
	uint8_t IVISerialNumber[30]; //IVI主机序列号(默认0,ASCII字符0x30,不足后补0x00)
	uint8_t TBoxSerialNumber[30];//Tbox序列号(不足后补0x00)
	uint8_t	AuthToken[4];	//车辆身份令牌
	uint8_t	New_RootKey[16];//新RootKey
	uint8_t	SKey[16];	//会话密钥
//	uint8_t	RootKey[16];//原始根密钥

	VehicleCondData_t	VehicleCondData; //车况数据
	RemoteControlData_t	RemoteControlData;//远程控制命令数据
	RemoteDeviceConfig_t	RemoteDeviceConfigInfo[MAX_DEVICE_NO];//远程配置信息数据
	AcpCodeFault_t			AcpCodeFault;	//故障信息数据

	uint8_t	voltageFaultSRSState;//SRS硬线断开(检测电压值)0:正常1断开
}FAWACPInfo_Handle;

typedef struct LinkTimer{
	uint8_t AcpAppID;
	uint8_t MsgType;
	uint8_t TspSoure;
	time_t  time;
	struct LinkTimer *next;
}LinkTimer_t;


//记录超时节点的超时次数，和TSP source
typedef struct{
	uint8_t RemoteCtrlFlag;
	uint8_t RemoteCtrlTspSource;
	uint8_t	VehQueCondFlag;
	uint8_t VehQueCondSource;
	uint8_t RemoteCpnfigFlag;
	uint8_t RemoteCpnfigSource;
	uint8_t UpdateRootkeyFlag;
	uint8_t UpdateRootkeySource;
	uint8_t VehGPSFlag;
	uint8_t VehGPSSource;
	uint8_t RemoteDiagnoFlag;
	uint8_t RemoteDiagnoSource;
}TimeOutType_t;


class CFAWACP
{
public:
	CFAWACP();
	~CFAWACP();
	/*	连接网络	*/
	int socketConnect();
	/*	断开网络	*/
	int disconnectSocket();


/*******************初始化相关函数************************/
	/* 初始化登陆鉴权数据 */
	void InitAcpTBoxAuthData();
	/* 初始化车身相关数据 */
	void Init_FAWACP_dataInfo();
	/*	初始化参数信息	*/
	int initTBoxParameterInfo();
	/*	读取参数信息	*/
	int readTBoxParameterInfo();
	/*	更新参数信息	*/
	int updateTBoxParameterInfo();

/*********************线程函数***************************/
	/*	接收数据 	*/
	static void *receiveThread(void *arg);
	/*	超时处理 	*/
	static void *TimeOutThread(void *arg);
	
	void system_power_sleep(const char *lock_id);
/******************超时处理链表函数************************/
	/*	循环遍历检测超时节点	*/
	void printf_link(LinkTimer_t **head);
	/*	检测链表是否超时	*/
	int CheckTimer(LinkTimer_t *pb, uint8_t *flag);
	/*	初始化数据并插入节点	*/
	int insertlink(uint8_t applicationID, uint8_t MsgType, uint8_t TspSourceID);
	/*	插入链表节点	*/
	void insert(LinkTimer_t **head,LinkTimer_t *p_new);
	/*	查询链表节点	*/
	LinkTimer_t *search (LinkTimer_t *head,uint8_t AcpAppID);
	/*	删除链表节点	*/
	void delete_link(LinkTimer_t **head,uint8_t AcpAppID);

private:	
	/*	登陆服务器认证 <登陆认证过程中出现任何异常则重新开始认证> */

/*****************发送打包登陆信息相关函数***************************/
	int	sendLoginAuth(uint8_t MsgType, AcpCryptFlag_E CryptFlag);
	/*	TBox认证请求   */
	int	sendLoginAuthApplyMsg();
	/*	CT Message   */
	int	sendLoginCTMsg();
	/*	RS Message   */
	int	sendLoginRSMsg();
	/*	AuthReadyMsg   */
	int	sendLoginAuthReadyMsg();
	
public:	
	int ShowdataTime(uint8_t *pos);
	int getServerIP(int flag, char *ip, int ip_size, int *port, const char *domainName);
	void reportRemoteConfigCmd(callback_RemoteConfigCmd cb_RemoteConfigCmd, uint8_t ConfigItemNum, uint16_t ConfigValue);
	void reportRemoteCtrlCmd(callback_RemoteCtrlCmd cb_RemoteCtrlCmd);

	int VehicleReissueSend();
	int RespondTspPacket(AcpAppID_E applicationID, uint8_t MsgType, AcpCryptFlag_E CryptFlag, uint8_t TspSourceID);
	static void cb_TspRemoteCtrl();
	/****************************************************************
	* Function Name: PackData_Signature
	* Auther	   : liuyj
	* Date		   : 2017-12-01
	* inParam		: dataBuff：待签名的数据缓存;len:数据长度;
	* outParam		：OutBuff：签名后的数据
	* Description  : 对数据体消息PAYLOAD签名(SHA-1算法)
	* Return	   : 签名后的数据长度
	*****************************************************************/
	void PackData_Signature(uint8_t *dataBuff, uint16_t len, uint8_t *OutBuff);
	/****************************************************************
	* Function Name: PackFAWACP_FrameData
	* Auther	   : liuyj
	* Date		   : 2017-12-01
	* Description  : 整数据包
	* inParam	   : dataBuff:整包数据缓存;bufSize:数据长度;
					 applicationID:服务标识号;MsgType:服务消息类型;CryptFlag:加密标志,TspSourceID:Tsp发起的命令source值
	* outParam	   ：dataBuff：打包后数据缓存
	* Return	   : 打包后数据长度
	*****************************************************************/
	uint16_t PackFAWACP_FrameData(uint8_t *dataBuff, uint16_t bufSize, AcpAppID_E applicationID, uint8_t MsgType, AcpCryptFlag_E CryptFlag, uint8_t TspSourceID = 0);
	
	/****************************************************************
	* Function Name	: PackFAWACP_PayloadData
	* Auther		: liuyj
	* Date			: 2017-12-01
	* Description	: 数据体Payload打包
	* inParam		: dataBuff：待打包数据缓存;bufSize:缓存大小;
					  applicationID:服务标识号;MsgType:服务消息类型;CryptFlag:加密标志
	* outParam		：dataBuff：打包后数据缓存
	* Return		: 打包后数据长度
	*****************************************************************/
	uint32_t PackFAWACP_PayloadData(uint8_t *dataBuff, uint16_t bufSize, AcpAppID_E applicationID, uint8_t MsgType, AcpCryptFlag_E CryptFlag, uint8_t TspSourceID = 0);

	/****************************************************************
	* Function Name: Unpack_FAWACP_FrameData
	* Auther	   : liuyj
	* Date		   : 2017-12-01
	* Description  : 解析协议数据包
	* input		   : pData:数据缓存;DataLen:数据长度;
	* Return	   : the length of packed data
	*****************************************************************/
	uint16_t Unpack_FAWACP_FrameData(uint8_t *pData, int32_t DataLen);

	uint16_t unpackDataLenDeal(uint8_t *pTemp, uint16_t &DataLen);
	uint16_t UnpackData_AcpLoginAuthen(uint8_t *PayloadBuff,uint16_t payload_size,uint8_t MsgType);
	uint16_t Unpacket_AcpHeader(AcpAppHeader_t& Header, uint8_t *pData, int32_t DataLen);
	uint16_t UnpackData_AcpRemoteCtrl(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t MsgType);
	uint16_t UnpackData_AcpQueryVehicleCond(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t applicationID, uint8_t MsgType);
	uint16_t UnpackData_AcpRemoteConfig(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t MsgType);
	uint16_t UnpackData_AcpUpdateRootKey(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t MsgType);
	uint16_t UnpackData_AcpRemoteUpgrade(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t MsgType);
	uint16_t UnpackData_AcpRemoteDiagnostic(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t MsgType);
	static uint16_t timingReportingData(uint8_t MsgType, AcpAppID_E AppID);
protected:
	/****************************************************************
	* Function Name: SOFEOFDeal
	* Auther	   : liuyj
	* Date		   : 2017-12-16
	* Description  : 帧头和尾的处理
	* Param        :pTemp:跟随指针, nTempNum:值
	* Return	   : no
	*****************************************************************/
	void SOFEOFDeal(uint8_t* pTemp, uint32_t nTempNum);
	//数据长度处理
	uint16_t addDataLen(uint8_t *pTemp, uint16_t DataLen, uint8_t Identifier, uint8_t flag);
	uint16_t checkReceivedData(uint8_t *pData, uint32_t len);
	//uint16_t unpackDataLenDeal(uint8_t *pTemp, uint16_t &DataLen);
	/****************************************************************
	* Function Name: TimeDeal
	* Auther	   : liuyj
	* Date		   : 2017-12-15
	* Description  : 时间戳处理
	* Return	   : no
	*Param		   ：pTemp：跟随指针
	*****************************************************************/	
	void TimeDeal(uint8_t* pTemp, uint8_t isFlagLen = 1);
	/****************************************************************
	* Function Name: TimeStampPart
	* Auther	   : liuyj
	* Date		   : 2017-12-18
	* Description  : 时间戳处理
	* Return	   : no
	*Param		   ：pTemp：跟随指针, pTimeStamp:时间戳的数据体
	*****************************************************************/
	void TimeStampPart(uint8_t* pTemp, TimeStamp_t *pTimeStamp); 
	/****************************************************************
	* Function Name: AuthTokenDeal
	* Auther	   : liuyj
	* Date		   : 2017-12-16
	* Description  : 车辆身份令牌
	* Param        : pTemp:跟随指针
	* Return	   : no
	*****************************************************************/
	void AuthTokenDeal(uint8_t* pTemp);
	/****************************************************************
	* Function Name: SourceDeal
	* Auther	   : liuyj
	* Date		   : 2017-12-16
	* Description  : 指令源信息
	* Param        : pTemp:跟随指针,TspSourceID:source源,由TSP发起的值。TBox发起的值为0
	* Return	   : no
	*****************************************************************/
	void SourceDeal(uint8_t* pTemp, uint8_t nfalgTsp = 1);
	/****************************************************************
	* Function Name: ErrorCodeDeal
	* Auther	   : liuyj
	* Date		   : 2017-12-16
	* Description  : 错误信息处理
	* Param        : pTemp:跟随指针,nErrorCode:nErrorCode值
	* Return	   : no
	*****************************************************************/
	void ErrorCodeDeal(uint8_t* pTemp, uint8_t nErrorCode);	

	//处理AuthApply数据体(启动时必须先初始化车厂标识和VIN等相关参数)
	uint16_t AuthApplyDataDeal(uint8_t *pTemp);	
	//处理CTMsg数据体
	uint16_t CTMsgDataDeal(uint8_t *pTemp);
	//处理RSMsg数据体
	uint16_t RSMsgDataDeal(uint8_t* pTemp);
	//处理AuthReady数据体
	uint16_t AuthReadyMsgDataDeal(uint8_t* pTemp);
	//远程控制打包
	uint16_t RemoteCtrlCommandDeal(uint8_t *dataBuff);
	//车况上报数据打包
	uint16_t ReportVehicleCondDeal(uint8_t* dataBuff, uint8_t flag);
	//远程配置数据打包
	uint16_t RemoteConfigCommandDeal(uint8_t *dataBuff);
	//RootKey更换
	uint16_t RootKeyDataDeal(uint8_t *dataBuff);
	//远程升级【命令返回】数据结构打包
	uint16_t RemoteUpgrade_ResponseDeal(uint8_t *dataBuff);
	//远程升级【下载开始】数据结构打包
	uint16_t RemoteUpgrade_DownloadStartDeal(uint8_t *dataBuff);
	//远程升级【下载结果】数据结构打包
	uint16_t RemoteUpgrade_DownloadResultDeal(uint8_t *dataBuff);
	//远程升级【升级开始】数据结构打包
	uint16_t RemoteUpgrade_UpdateStartDeal(uint8_t *dataBuff);
	//远程升级【升级结果】数据结构打包
	uint16_t RemoteUpgrade_UpdateResultDeal(uint8_t *dataBuff);
	//位置和车速定时上传---GPS位置信号+实时车速
	uint16_t ReportGPSSpeedDeal(uint8_t* dataBuff);
	//故障信号打包
	uint16_t FaultSignDeal(uint8_t* dataBuff);
	//车辆定位---GPS位置信号
	uint16_t VehicleGPSDeal(uint8_t* dataBuff);
	//远程诊断
	uint16_t RemoteDiagnosticDeal(uint8_t *dataBuff);
	int	sendLoginMsg(uint8_t MsgType, AcpCryptFlag_E CryptFlag);
	uint16_t SignCodeDeal(uint8_t* pTemp, uint8_t inValid, uint16_t SignCde);
	uint16_t addVerTboxOS(uint8_t* pTemp);
	uint16_t addChargeState(uint8_t* pTemp);
	uint16_t addPowerCellsState(uint8_t* pTemp);
	uint16_t addKeepingstateTime(uint8_t* pTemp);
	uint16_t addAirconditionerInfo(uint8_t* pTemp);
	uint16_t addRemainUpkeepMileage(uint8_t* pTemp);
	uint16_t addSafetyBelt(uint8_t* pTemp);
	uint16_t addPowerSupplyMode(uint8_t* pTemp);
	uint16_t addParkingState(uint8_t* pTemp);
	uint16_t addHandBrakeState(uint8_t* pTemp);
	uint16_t addGearState(uint8_t* pTemp);
	uint16_t addEngineSpeed(uint8_t* pTemp);
	uint16_t addPastRecordWheelState(uint8_t* pTemp);
	uint16_t addPastRecordSpeed(uint8_t* pTemp);
	uint16_t addWheelState(uint8_t* pTemp);
	uint16_t addEngineState(uint8_t* pTemp);
	uint16_t addVerTboxMCU(uint8_t* pTemp);
	uint16_t addTyreState(uint8_t* pTemp);
	uint16_t addCarlampState(uint8_t* pTemp);
	uint16_t addWindowState(uint8_t* pTemp);
	uint16_t addSunroofState(uint8_t* pTemp);
	uint16_t addCarLockState(uint8_t* pTemp);
	uint16_t addCarDoorState(uint8_t* pTemp);
	uint16_t addSTAverageOil(uint8_t* pTemp);
	uint16_t addLTAverageOil(uint8_t* pTemp);
	uint16_t addSTAverageSpeed(uint8_t* pTemp);
	uint16_t addLTAverageSpeed(uint8_t* pTemp);
	uint16_t addCurrentSpeed(uint8_t* pTemp);
	uint16_t addBattery(uint8_t* pTemp);
	uint16_t addOdometer(uint8_t* pTemp);
	uint16_t addRemainedOil(uint8_t* pTemp);
	uint16_t add_GPSData(uint8_t* pTemp);
	uint16_t addVerIVI(uint8_t* pTemp);
	uint16_t addChargeConnectState(uint8_t* pTemp);
	uint16_t addBrakePedalSwitch(uint8_t* pTemp);
	uint16_t addAcceleraPedalSwitch(uint8_t* pTemp);
	uint16_t addYaWSensorInfoSwitch(uint8_t* pTemp);
	uint16_t addAmbientTemperat(uint8_t* pTemp);
	uint16_t addPureElecRelayState(uint8_t* pTemp);
	uint16_t addResidualRange(uint8_t* pTemp);
	uint16_t addNewEnergyHeatManage(uint8_t* pTemp);
	uint16_t addVehWorkMode(uint8_t* pTemp);
	uint16_t addMotorWorkState(uint8_t* pTemp);
	uint16_t addHighVoltageState(uint8_t* pTemp);
	uint16_t addFaultSignDeal(uint8_t* dataBuff);

private:
//打包头数据
	void Packet_AcpHeader(AcpAppHeader_t& Header, AcpAppID_E AppID, uint8_t MsgType, AcpCryptFlag_E CryptFlag, uint16_t MsgLength);
	uint16_t AddData_AcpHeader(uint8_t *dataBuff, AcpAppHeader_t Header);
public:	
//	CCrc32			m_Crc32OpData;
	HmacSHA 		m_SHA1OpData;
//	CAES 			*m_AES128OpData;
	HMAC_MD5 		m_Hmacmd5OpData;
	/*	SHA-1加密	*/
	int Encrypt_SHA1Data(uint8_t *InBuff, uint16_t InLen, uint8_t *OutBuff, uint16_t OutLen = 20);
	/*	SHA-1解密	*/
	int Decrypt_SHA1Data(const uint8_t *InBuff, uint16_t InLen, uint8_t *OutBuff, uint16_t OutLen);
	/*	AES128加密	*/
//	void Encrypt_AES128Data(uint8_t *InBuff, uint16_t InLen, uint8_t *OutBuff, uint16_t& OutLen, AcpCryptFlag_E CryptFlag, uint8_t *Key);
	/*	AES128解密	*/
//	void Decrypt_AES128Data(uint8_t *InBuff, uint16_t InLen, uint8_t *OutBuff, uint16_t& OutLen, AcpCryptFlag_E CryptFlag, uint8_t *Key);
	/*	HMACMD5算法	*/
	void HMACMd5_digest(const char *SecretKey, uint8_t *Content, uint16_t InLen, uint8_t OutBuff[16]);
	/*	CRC32校验	*/
	uint32_t Crc32_Count(unsigned long inCrc32, const uint8_t *buf, int size);
protected:
	//接口：MCU写入结构数据
//	ISetVehicleConditionData(VehicleCondition_Data &VehicleData);


public:
	static CFAWACP* cfawacp;
	struct  sockaddr_in 	m_socketaddr;
	time_t HighReportTime;
	time_t LowReportTime;
	time_t HeartBeatTime;
	time_t NowHighReportTime;
	time_t NowLowReportTime;
	time_t NowHeartBeatTime;
	time_t 	NowReConnectTime;
	time_t  LowReConnectTime;
	int ConnectFaultTimes;
	int InLoginFailTimes;
	int sleepeventrport;
	int HeartBeatTemp;
	int SockReconnection;
	uint8_t TspctrlSource;
	int 	m_Socketfd;			//socket ID
	bool 	m_ConnectedState;	//连接状态
	uint8_t m_loginState;		//登陆状态(0:未登陆,1:正在登陆,2:已登陆)
	uint8_t	m_SendMode;	//发送方式
	bool	m_bEnableSendDataFlag;//使能数据发送标志(用于RootKey更换时)
	uint8_t TboxSleepStatus;
	LinkTimer_t *Headerlink;
	TimeOutType_t TimeOutType;
	TimeStamp_t	m_tspTimestamp;//tsp时间

	//登陆鉴权相关数据
	//uint8_t	RootKey[16];	//根密钥(与终端对应)(需存全局)
	uint8_t AuthToken[4];
	uint8_t AKey_TBox[16];
	uint8_t SKey_TBox[16];	 //解密后的会话密钥(AKey_TBox Decrypt SKeyC_Tsp)
	AcpTBoxAuthApplyMsg_t	AcpTBox_AuthApplyMsg;//TBox认证请求【Auth Apply Message】	
	AcpTBoxCTMsg_t			AcpTBox_CTMsg;		 //TBox发起【CT Message】	
	AcpTBoxRSMsg_t			AcpTBox_RSMsg;		 //TBox发起【RS Message】	
	AcpTBoxAuthReadyMsg_t	AcpTBox_AuthReadyMsg;//TBox发起【Auth Ready Message】	

	AcpTspAkeyMsg_t			AcpTsp_AkeyMsg;		//接收TSP的AKey【AKey Message】
	AcpTspCSMsg_t			AcpTsp_CSMsg;		//接收TSP【CS Message】
	AcpTspSKeyMsg_t			AcpTsp_SKeyMsg;		//接收TSP【SKey Message】
	AcpTspAuthReadyACKMsg_t	AcpTsp_AuthReadyACKMsg;//接收TSP【Auth Ready ACK Message】	
/*TSP数据*/
	//// uint8_t	RootKey_Tsp[32];	//根密钥(与终端对应)
	//// uint8_t	AKey_TSP[16];	//随机认证密钥
	//uint8_t	AKeyC_Tsp[32];	//加密后的认证密钥[Send1]
	////uint8_t	Rand1_Tsp[16];	//随机数[Send1]
	
	//uint8_t Rand1CS_Tsp[32];//加密后的随机数(AKey_TSP Encrypt Rand1_Tsp)
	//uint8_t Rand2CS_Tsp[32];//随机数加密(AKey_TSP Encrypt Rand2_Tbox)[Send3]
	//uint8_t RT_Tsp;		  //Rand1CS_Tsp == Rand1CT_TBox[Send3]
	
	////uint8_t	SKey_Tsp[16];	//随机会话密钥
	//uint8_t	SKeyC_Tsp[32];	//加密后的随机回话密钥(AKey_TSP Encrypt SKey_Tsp)[Send5]
	//uint8_t Rand3CS_Tsp[32];//随机数加密(SKey_Tsp Encrypt Rand3_Tbox)[Send5]	
/*TBox数据*/
	//uint8_t	RootKey_Tbox[32];	 //根密钥(与服务器对应)
	//uint8_t AKey_TBox[16];	 //解密后的AKey(RootKey Decrypt AkeyC_Tsp)
	//uint8_t	Rand1CT_TBox[32];//加密后的随机数(AKey_TBox Encrypt Rand1_Tsp)[Send2]
	//uint8_t   Rand2_Tbox[16];  //随机数[Send2]
	
	//uint8_t	Rand2CT_TBox[32];//加密后的随机数(AKey_TBox Encrypt Rand2CS_Tsp)	
	//uint8_t   Rand3_Tbox[16];  //随机数[Send4]
	//uint8_t	RS_TBox;		 //Rand2CT_TBox == Rand2CS_Tsp[Send4]
	

	//uint8_t Rand3CT_TBox[32];//加密后的随机数(SKey_TBox Encrypt Rand3_Tbox)
	//uint8_t	BSKeySuccess;	 //Rand3CT_TBox == Rand3CS_Tsp

	AcpRemoteControlCommand_t	m_AcpRemoteCtrlCommandData;	//远程控制命令数据参数值
	AcpRemoteControl_RawData 	m_AcpRemoteCtrlList;		//远程控制命令列表
	AcpVehicleCondCommandList_t	m_AcpVehicleCondCommandList;//车况查询指令信号列表
	AcpRemoteConfig_RawData		m_AcpRemoteConfig;			//远程配置列表
	AcpRemoteUpgrade_t			m_AcpRemoteUpgrade;			//升级数据
	RemoteDiagnose_t			m_RemoteDiagnose;			//远程诊断命令列表
	RemoteDiagnoseResult_t		m_RemoteDiagnoseResult;		//远程诊断结果


	uint8_t Upgrade_CommandResponse;	//升级命令接收状态0 为成功，1 为失败
	uint8_t Upgrade_Reserve;			//下载开始 默认为 0，无实际意义
	uint8_t Upgrade_DownloadState;		//升级包下载状态
										//0：成功; 1：失败-驾驶员未确认下载
										//2：失败-驾驶员取消下载
										//3：失败-URL 地址相关错误
										//4：失败-下载失败 
										//5：失败-MD5 检测失败
	uint8_t Upgrade_Result;				//升级结果状态
										//0：成功; 
										//1：失败-车主未确认升级
										//2：失败-车主取消升级
										//3：失败-升级过程失败

	pthread_t receiveThreadId;
	pthread_t TimeOutThreadId;
	
};

extern FAWACPInfo_Handle *p_FAWACPInfo_Handle;
extern TBoxDataPool *dataPool;
extern char target_ip[16];
extern LTEModuleAtCtrl *LteAtCtrl;
#endif
