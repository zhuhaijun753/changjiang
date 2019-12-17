#ifndef _GBT32960_H_
#define _GBT32960_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include "TBoxDataPool.h"
#include "FTPClient.h"
#include "SQLiteDatabase.h"
#include <netdb.h>
#include "HMAC_MD5.h"
#include "common.h"

#define VER_GOLDEN	"ver 1.1.1"

#ifndef GBT_DEBUG
	#define GBT_DEBUG 0
#endif


#if GBT_DEBUG
	//#define GBTLOG(format,...) printf("### GBTLOG ### %s,%s [%d] "format"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
	#define GBTLOG(format,...) printf("### GBTLOG ### %s [%d] "format"\n",__FUNCTION__,__LINE__,##__VA_ARGS__)
	#define GBT_NO(format,...) printf(format,##__VA_ARGS__)
#else

	#define GBTLOG(format,...)
	#define GBT_NO(format,...)
#endif




#define SERVER_PORT                      8877  // 20060 //jason change
#define SERVER_IP                        "112.74.171.112"
#define PARAMETER_INFO_PATH              "/data/GB32960Para"
#define DELETE_PARAMETER                 "rm -rf /data/GB32960Para"

#define DATA_BUFF_LEN                    1024
#define GBT32960_PROTOCOL_HEADER         0x2323

/* response signs */
#define GBT32960_RESPONSE_SUCCESS        0x01 //应答成功
#define GBT32960_RESPONSE_ERROR          0x02 //设置未成功
#define GBT32960_RESPONSE_REPEAT         0x03 //VIN重复
#define GBT32960_RESPONSE_CMD            0XFE //命令

/* command - upstream */
#define GBT32960_VehiceLoginID           0x01 //车辆登录
#define GBT32960_RealTimeInfoReportID    0x02 //实时信息上报
#define GBT32960_ReissueInfoReportID     0x03 //补发信息上报
#define GBT32960_VehiceLogoutID          0x04 //车辆登出
//#define GBT32960_PlatformLoginID         0x05 //平台登录
//#define GBT32960_PlatformLogoutID        0x06 //平台登出
#define GB32960_HeartBeatID       		 0x07 //心跳
#define GB32960_TerminalCalibrationTime  0x08 //终端校时
/* command - downstream */
#define GB32960_QueryCommandID           0x80 //查询命令
#define GB32960_SetCommandID             0x81 //设置命令
#define GB32960_ControlCommandID         0x82 //控制命令

/* message type sign */
#define GB32960_VehicleData       	     0x01 //整车数据
#define GB32960_DriveMotorData           0x02 //驱动电机数据
#define GB32960_FuelCellData             0x03 //燃料电池数据
#define GB32960_EngineData               0x04 //发动机数
#define GB32960_PositionInfoData         0x05 //位置信息数据
#define GB32960_ExtremeValueData      	 0x06 //极值数据
#define GB32960_AlarmData       		 0x07 //报警数据
#define GB32960_RechargeableDevVoltage   0x08 //可充电储能装置电压数
#define GB32960_RechargeableDevTemp		 0x09 //可充电储能装置温度数

/* 自定义GBT32960相关数据 */
#define MAX_STOREENERGY_NUM              15   //储能系统个数
#define MAX_STOREENERGY_LEN              50   //储能系统编码长度
#define MAX_DRIVEMOTOR_NUM               4 	  //驱动电机最大个数
#define MAX_PROBE_NUM                    50   //最大探针个数
#define MAX_ALARM_NUM                    35   //报警故障最大个数
#define MAX_FRAMESTARTCELL_TOTAL         256  //本帧单体电池总数
#define DOMAIN_LEN                       64   //域名长度



/* 整车数据格式 */
typedef struct{
	uint8_t vehicleState;					 //车辆状态
	uint8_t chargeState;					 //充电状态
	uint8_t runMode;						 //运行模式
	uint16_t vehicleSpeed;					 //车速
	uint32_t vehicleMileage;				 //里程
	uint16_t vehicleVoltage;				 //总电压
	uint16_t vehicleCurrent;				 //总电流
	uint8_t SOC;							 //SOC
	uint8_t DcDcState;						 //DC/DC状态
	uint8_t gear;							 //档位
	uint16_t insulationResistance;			 //绝缘电阻
  	uint8_t acceleratorPedalTravelValue;     //加速踏板行程值
  	uint8_t brakePedalState;                 //制动踏板状态	

}vehicleDataInfo;

/* 驱动电机数据格式 */
typedef struct{
	uint8_t driveMotorSerialNum;			  //驱动电机序号
	uint8_t driveMotorState;				  //驱动电机状态
	uint8_t driveMotorControllerTemp;		  //驱动电机控制器温度
	uint16_t driveMotorRotationalSpeed; 	  //驱动电机转速
	uint16_t driveMotorTorque;				  //驱动电机转矩
	uint8_t driveMotorTemp; 				  //驱动电机温度
	uint16_t motorControllerVin;			  //驱动控制器输入电压
	uint16_t motorControllerDC; 			  //驱动控制器直流母线电流

}driveMotorInfo;

typedef struct{
	uint8_t totalDriveMotor;				       /*驱动电机个数*/
	driveMotorInfo driveMotor[MAX_DRIVEMOTOR_NUM]; /*驱动电机数据*/
}driveMotorDataInfo;

/* 燃料电池数据 */
typedef struct{
	uint16_t fuelCellVoltage;                  /*燃料电池电压*/
	uint16_t fuelCellCurrent;                  /*燃料电池电流*/
	uint16_t fuelConsumptionRate;              /*燃料消耗率*/
	uint16_t fuelCellTempProbeNum;             /*燃料电池温度探针总数 */
	uint8_t ProbeTemperature[MAX_PROBE_NUM];   /*探针温度值【个数取决于探针总数】*/
	uint16_t HydrogenSysMaxTemperature;        /*氢系统中最高温度*/
	uint8_t HydrogenSysMaxTemperatureProbeNum; /*氢系统中最高温度探针代号 */
	uint16_t HydrogenMaxSolubility;            /*氢气最高浓度*/
	uint8_t HydrogenMaxSolubilitySensorNum;    /*氢气最高浓度传感器代号*/
	uint16_t HydrogenMaxPressure;              /*氢气最高压力*/
	uint8_t HydrogenMaxPressureSensorNum;      /*氢气最高压力传感器代号*/
	uint8_t HighPressureDcDcState;             /*高压DC/DC 状态*/
	
}fuelCellDataInfo;

/* 发动机数据 */
typedef struct{
	uint8_t engineState;                        /*发动机状态 */
  	uint16_t engineCrankshaftSpeed;             /*曲轴转速*/
  	uint16_t fuelConsumption;					/*燃料消耗率*/
	
}engineDataInfo;

/* 车辆位置数据 */
typedef union{ 
	uint8_t positionState;
 	struct{
	  uint8_t valid 		:1;	  		        /*定位状态 */
	  uint8_t lat 			:1;  		        /*纬度*/
	  uint8_t lon 			:1; 	        	/*经度*/
	  uint8_t backup		:5;			        /*备用*/
  	}bitState; 	
}positionValidState;

typedef struct{ 
	positionValidState positionValid;    		/*位置状态 */
	uint32_t Lon;  								/*经度*/
	uint32_t Lat;								/*纬度*/
}positionDataInfo;

/* 极值数据*/
typedef struct{
	uint8_t MaxVolBatterySubsysNumber;  		/*最高电压电池子系统号 */
	uint8_t MaxVolBattery;						/*最高电压电池单体代号 */
	uint16_t MaxBatteryVoltageValue;  			/*电池单体电压最高值 */
	uint8_t MinVolBatterySubsysNumber;			/*最低电压电池子系统号 */
	uint8_t MinVolBattery;  					/*最低电压电池单体代号 */
	uint16_t MinBatteryVoltageValue;			/*电池单体电压最低值 */
	uint8_t MaxTemperatureSubsysNumber;  		/*最高温度子系统号 */
	uint8_t MaxTemperatureProbe;				/*最高温度探针序号 */
	uint8_t MaxTemperatureValue;  				/*最高温度值 */
	uint8_t MinTemperatureSubsysNumber;			/*最低温度子系统号 */
	uint8_t MinTemperatureProbe;  				/*最低温度探针序号 */
	uint8_t MinTemperatureValue;				/*最低温度值 */
}extremeDataInfo;

/* 报警数据     ------> start */
typedef union{
	uint32_t AlarmFlag;
	struct{
		//uint32_t Backup								:9;			/*备用*/
		//uint32_t EnergyStorageDeviceOvercharge		:1;  		/*车载储能装置过充报警*/
		//uint32_t MicroShortCircuit					:1;	  		/*微短路报警*/
		//uint32_t BatteryLow							:1; 	  	/*蓄电池电量低报警*/
		//uint32_t WakeUp								:1;  		/*唤醒异常报警*/
		uint32_t Backup								:13;		/*备用*/
		uint32_t EnergyStorageDeviceTypeOvercharge	:1;	  		/*车载储能装置类型过充报警*/	
		uint32_t DriveMotorTemperature				:1; 	  	/*驱动电机温度报警*/
		uint32_t HighVoltageInterlockState 			:1;  		/*高压互锁状态报警*/
		uint32_t DriveMotorMontrollerTemperature	:1;	  		/*驱动电机控制器温度报警*/
		uint32_t DCDCStatus							:1; 		/*DCDC状态报警*/
		uint32_t BrakeSystem						:1;		  	/*制动系统报警*/
		uint32_t DCDCTemperature					:1;		  	/*DCDC温度报警*/
		uint32_t Insulation							:1; 	  	/*绝缘报警*/
		uint32_t SingleCellConsistency				:1;  		/*单体电池一致性差报警*/
		uint32_t EnergyStorageMismatch				:1;	  		/*可充电储能系统不匹配报警*/	
		uint32_t SOCJump 							:1; 	  	/*SOC跳变报警*/
		uint32_t SOCHigh 							:1;  		/*SOC过高报警*/
		uint32_t SingleBatteryUndervoltage 			:1;	  		/*单体蓄电池欠压报警*/
		uint32_t SingleBatteryOvervoltage 			:1; 	  	/*单体蓄电池过压报警*/
		uint32_t SOCLow								:1;  		/*SOC低报警*/
		uint32_t EnergyStorageDeviceTypeUndervoltage:1;	  		/*车载储能装置类型欠压报警*/	
		uint32_t EnergyStorageDeviceTypeOvervoltage :1; 	  	/*车载储能装置类型过压报警*/
		uint32_t BatteryHighTemperature				:1;  		/*电池高温报警*/
		uint32_t TemperatureDiff					:1;	  		/*温度差异报警*/		
	}AlarmFlagBit;
}generalAlarmInfo;

//可充电储能装置故障代码列表
typedef union{
	uint32_t fault;
	struct{
 		uint32_t faultCode:         24;       /*故障码*/
 		uint32_t faultLevel:        8;        /*故障等级*/
	}faultBit;
}energyStorageDevFaultList;
//驱动电机故障代码列表
typedef union{
	uint32_t fault;
	struct{
		uint32_t faultCode:         24;       /*故障码*/
 		uint32_t faultLevel:        8;        /*故障等级*/
	}faultBit;
}driveMotorFaultList;
//发动机故障列表
typedef union{
	uint32_t fault;
	struct{
		uint32_t faultCode:         24;       //故障码
 		uint32_t faultLevel:        8;        /*故障等级*/
	}faultBit;
}engineFaultList;
//其他故障列表
typedef union{
	uint32_t fault;
	struct{
		uint32_t faultCode:         24;       /*故障码*/
 		uint32_t faultLevel:        8;        /*故障等级*/
	}faultBit;
}otherFaultList;

typedef struct{
	uint8_t maxAlarmLevel;  										 /*最高报警等级*/
	generalAlarmInfo generalAlarm;									 /*通用报警标志*/
	uint8_t totalFaultNumOfEnergyStorageDev;  						 /*可充电储能装置故障总数*/
	energyStorageDevFaultList energyStorageDevFault[MAX_ALARM_NUM];  /*可充电储能装置代码列表*/
	uint8_t totalFaultNumOfDriveMotor;  							 /*驱动电机故障总数*/
	driveMotorFaultList driveMotorFault[MAX_ALARM_NUM];              /*驱动电机故障代码列表*/
	uint8_t totalFaultNumOfEngine;  								 /*发动机故障总数*/
	engineFaultList engineFault[MAX_ALARM_NUM];						 /*发动机故障列表*/
	uint8_t totalFaultNumOfOther;  									 /*其他故障总数*/
	otherFaultList otherFault[MAX_ALARM_NUM];						 /*其他故障代码列表*/
}alarmDataInfo;
/* 报警数据     <------ end */

/* 可充电储能装置电压数据和温度数据 ------> Start */
typedef struct{
	uint8_t subsysNumber;  					    /*可充电储能装置子系统*/
	
	uint16_t subsysVol;						    /*可充电储能装置子系统电压*/
	uint16_t subsysCurrent;  				    /*可充电储能装置子系统电流*/
	uint16_t totalNumOfSingleCell;              /*单体电池总数*/
	//uint16_t frameStartCellNumber[MAX_FRAMESTARTCELL_TOTAL]; /*本帧起始电池序号*/
	uint16_t frameStartCellNumber;
	uint8_t frameStartCellTotal;				/*本帧单体电池总数*/
	uint16_t monomerCellVol[MAX_FRAMESTARTCELL_TOTAL]; /*单体电池电压*/
	
	uint16_t subsysProbeNumber;				    /*可充电储能装置子系统温度探针个数*/
	uint8_t subsysProbeTemper[MAX_PROBE_NUM];   /*可充电储能装置子系统温度探针温度*/
}energyStorageDevSubsysVolList;

typedef struct{
	uint8_t totalEnergySubsys;				           /*可充电储能装置子系统个数*/
	energyStorageDevSubsysVolList ESDevSubsysVol[MAX_STOREENERGY_NUM];  /*可充电储能装置子系统电压/温度信息列表*/
}energyStorageDevSubsysDataInfo;
/* 可充电储能装置电压数值<------ end */


/* GBT32960 data information */
typedef struct{
	uint8_t vin[17];                     /*车辆识别码 */
	uint8_t ICCID[20];                   /*设备ICCID*/
	uint16_t serialNumber;               /*登录流水号*/
	uint8_t rechargeableEnergyNum;       /*可充电储能个数*/
	uint8_t rechargeableEnergyLen;       /*可充电储能编码长度*/
	int8_t rechargeableEnergyCode[MAX_STOREENERGY_NUM][MAX_STOREENERGY_LEN]; /*可充电储能编码*/

	vehicleDataInfo vehicleInfo;         /*整车数据*/
	driveMotorDataInfo driveMotorInfo;   /*驱动电机数据*/
	fuelCellDataInfo fuelCellInfo;       /*燃料电池数据*/
	engineDataInfo engineInfo;           /*发动机数据*/
	positionDataInfo positionInfo;       /*车辆位置数据*/
	extremeDataInfo extremeInfo;         /*极值数据*/
	alarmDataInfo alarmInfo;             /*报警数据*/
	energyStorageDevSubsysDataInfo ESDSubsysInfo;  /*可充电储能装置数据*/	

}GBT32960_dataInfo;

/* 配置参数信息（查询和设置命令)*/
typedef struct{
	uint16_t loginTimeInterval;             /*登录服务器时间周期，单位S*/
	uint16_t localStorePeriod;				/*本地存储时间周期，单位MS*/
	uint16_t timingReportTimeInterval;      /*信息上报时间周期，单位S*/
	uint16_t alarmInfoReportPeriod;         /*报警信息上报时间周期，单位mS*/
	uint8_t remoteServiceDomainNameLen;		/*远程服务与管理平台域名长度*/
	char remoteServiceDomainName[DOMAIN_LEN];  /*远程服务与管理平台域名*/
	uint16_t remoteServicePort; 			/*远程服务与管理平台端口*/	
	uint8_t hardwareVersion[5];				/*硬件版本，不可设置*/
	uint8_t firmwareVersion[5];				/*固件版本，不可设置*/
	uint8_t heartbeatInterval;			    /*车载终端心跳发送周期，单位S*/	
	uint16_t terminalResponseTimeout;		/*终端应答超时时间，单位S*/
	uint16_t platformResponseTimeout;		/*平台应答超时时间，单位S*/
	uint8_t logonFailureInterval; 			/*连续三次登录失败，到下次登录的间隔时间，单位min*/	
	uint8_t publicDomainNameLen;		    /*公共平台域名长度*/
	char publicDomainName[DOMAIN_LEN];	    /*公共平台域名*/
	uint16_t publicPort; 					/*公共平台端口*/
	uint8_t IsSamplingCheck;			    /*是否处于抽样检测中*/

}GBT32960_ConfigInfo;

typedef struct{
	char URLAddress[DOMAIN_LEN]; /*FTPURL地址*/
	char fileName[64];			 /*FTP文件名字*/
    char Apn[12];                /*FTPAPN*/
    char userName[12];           /*FTP用户名*/
    char password[20];           /*FTP密码*/
    char domainIP[DOMAIN_LEN];   /*FTP地址*/
    uint16_t port;               /*FTP端口*/
    char manufacturer[4];        /*FTP制造商ID*/
	char hardwareVersion[5];  /*硬件版本，不可设置*/
	char firmwareVersion[5];  /*固件版本，不可设置*/	
    uint16_t connectTimeOut;     /*FTP连接服务器有效时限*/

}Gb32960_FtpConfig;


typedef struct{
	uint8_t year;
	uint8_t month;
	uint8_t today;
}tboxStatusInfo;


typedef struct{
	tboxStatusInfo statusInfo;		//采集时间
	GBT32960_dataInfo dataInfo;		//TBox信息
	GBT32960_ConfigInfo configInfo;	//与服务器相关的参数信息
	Gb32960_FtpConfig ftp;			//升级相关信息

}GBT32960_handle;







class GBT32960
{
public:
	GBT32960();
	~GBT32960();
	int socketConnect();
	int closeConnect();
	int getServerIP(int getIPByDomainName, char *ip, int ip_size, int *port, char *domainName);
	int disconnectSocket();
	int sendLoginData();
	int sendLogoutData();
	int initTBoxParameterInfo();
	int readTBoxParameterInfo();
	int updateTBoxParameterInfo();
	/****************************************************************
	* Function Name: GBT32960_dataInit
	* Auther	   : yingaoguo
	* Date		   : 2017-06-23
	* Description  : initialization GBT32960 potocal interface data
	* Return	   : 
	*****************************************************************/
	uint8_t GBT32960_dataInit();
	static void *receiveThread(void *arg);
	static void *timeoutThread(void *arg);
	int8_t timingReport(uint8_t responseSign);
	/****************************************************************
	* Function Name: setTimer
	* Auther	   : yingaoguo
	* Date		   : 2017-06-23
	* Description  : the function can achieve the functionality
	*				 same as sleep and usleep.
	* Return	   : void
	*****************************************************************/
	void setTimer(uint32_t seconds = 300, uint32_t mseconds = 0);
	uint8_t checkSum_BCC(uint8_t *pData, uint16_t len);
	
	/****************************************************************
	* Function Name: pack_GBT32960FrameData
	* Auther	   : yingaoguo
	* Date		   : 2017-06-23
	* Description  : pack one frame data
	* Return	   : the length of packed data
	*****************************************************************/
	//pack function
	uint16_t pack_GBT32960FrameData(uint8_t *dataBuff, uint16_t len, uint8_t cmd, uint8_t responseSign,uint8_t *pInBuff, uint16_t size);
	/****************************************************************
	* Function Name: pack_GBT32960DataBody
	* Auther	   : yingaoguo
	* Date		   : 2017-06-23
	* Description  : pack the frame body
	* Return	   : the length of packed data
	*****************************************************************/
	uint16_t pack_GBT32960DataBody(uint8_t cmd, uint8_t *dataBuff, uint16_t buffSize, uint8_t *pInBuff, uint16_t len);
	uint16_t add_VehicleData(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t add_DriveMotorData(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t add_FuelCellData(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t add_EngineData(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t add_PositionData(uint8_t *dataBuff, uint16_t buffSize);//位置数据
	uint16_t add_ExtremeData(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t add_AlarmData(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t add_EnergyStorageDevSubsysVolData(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t add_EnergyStorageDevSubsysTempData(uint8_t *dataBuff, uint16_t buffSize);

	uint16_t pack_GBT32960_VehiceLogin(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t pack_GBT32960_RealTimeInfoReport(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t pack_GBT32960_ReissueInfoReport(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t pack_GBT32960_VehiceLogout(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t pack_GBT32960_PlatformLogin(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t pack_GBT32960_PlatformLogout(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t pack_GBT32960_TerminalCalibrationTime(uint8_t *dataBuff, uint16_t buffSize);
	uint16_t pack_GBT32960_QueryCommand(uint8_t *dataBuff, uint16_t buffSize, uint8_t *pInBuff, uint16_t len);
	uint16_t pack_GBT32960_SetCommand(uint8_t *dataBuff, uint16_t buffSize, uint8_t *pInBuff, uint16_t len);
	uint16_t pack_GBT32960_ControlCommand(uint8_t *dataBuff, uint16_t buffSize, uint8_t *pInBuff, uint16_t len);

	//unpack function
	int8_t checkReceivedData(uint8_t *pData, int32_t len);
	int8_t unpack_GBT32960FrameData(uint8_t *pData, int32_t len);
	void unpack_QueryCommand(uint8_t *pData, int32_t len);
	void unpack_SetCommand(uint8_t *pData, int32_t len);
	void unpack_ControlCommand(uint8_t *pData, int32_t len);
	void unpack_ftpUpgrade(uint8_t *pData, int32_t len);

	//log
    static 	int gbtLog(char *fplog);

    /****************************************************************
	* Function Name: getAddrFromBuff
	* Auther	   : yingaoguo
	* Date		   : 2017-06-23
	* Description  : get addr from buffer
	* Input        : pData:data, len:data len, content: query content
					 times: the times of the content
	* Return	   : the content address
	*****************************************************************/
	uint8_t *getAddrFromBuff(uint8_t *pData, int32_t len, uint8_t content, int32_t times);
	//database operation:
	int alarmReport();
	int sendTimingReportData(uint8_t *dataBuff,         uint16_t len);
//	int packTimingReportData();
	HMAC_MD5 		m_Hmacmd5OpData;
	time_t HighReportTime;
	time_t LowReportTime;
	time_t NowHighReportTime;
	time_t NowLowReportTime;
private:
	int socketfd;
	bool isConnected;
	volatile int recvFlag;
	int	ReportingFlagCount;//上报数据标志计数
	int timeoutTimes;
	int loginState;
	pthread_t receiveThreadId;
	pthread_t timeoutThreadId;
	struct sockaddr_in socketaddr;
	int SettoLoginState;
	uint8_t controltime[6];
	
};


extern int faultLevel;
extern GBT32960_handle *p_GBT32960_handle;
extern TBoxDataPool *dataPool;
extern int closeTbox;
extern bool resend;  //补发标志位
#endif

