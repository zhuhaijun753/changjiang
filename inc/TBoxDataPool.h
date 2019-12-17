#ifndef _TBOX_DATA_POOL_H_
#define _TBOX_DATA_POOL_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <common.h>
#include <fcntl.h>


#define TBOX_PARA_FILE              "/data/TBoxPara"

/***************************************************************************
 The vehicle identify number (vin) will not be reset with factory default
 settings, it only be set.
 车辆信息VIN，不会随恢复出厂设置而更改，它只支持设置        
****************************************************************************/
#define VEHICLE_IDENTIFY_NUM_FILE   "/data/TboxConfigInfo"

typedef enum
{
	VinID	= 0,
	ConfigCodeID = 1,	
	E_Call_info	 = 2,
	B_Call_info	 = 3,
	emmcStatusID = 4,
	encryptionID = 5,
	SUPPLYPN_ID	 = 6,	
}TboxConfigID;

//配置码
typedef union
{
	uint8_t 	ConfigCode;
	struct
	{
		uint8_t		Gear_Mode:2;
		uint8_t 	Engine_Type:3;
		uint8_t 	Peps:1;
		uint8_t 	Tpms:1;
		uint8_t 	Roof:1;
	}bitState;
}ConfigCode_t;


//TBOX配置信息
typedef struct
{
	uint8_t 	Veh_VinInfo[20];//车辆VIN信息
	ConfigCode_t	ConfigCode;	//配置码
	uint8_t 	emmcPartitionStatus;  // 0:为没有格式化,1:为已经格式化
	uint8_t 	encryptionStatus;  // 0:为加密IC (ATECC608A)没有配置, 1:为已经配置
	uint8_t		supplyPN[16];//供应商零件号
}TboxConfigInfo_t;




typedef enum
{
	wifi_APMode_Name_Id = 0,
	wifiAPModePassswordId,
	wifiStaModeNameId,
	wifiStaModePassswordId,
	bluetoothNameId,
	WIFI_APMODE_USED_IP,
	IMEI_INFO,
	SIM_ICCID_INFO,
	CIMI_INFO,
	E_CALL_ID,
	B_CALL_ID,
	I_CALL_ID,
	TBoxSerialNumber_ID,
	IVISerialNumber_ID,
	SERVER_IP_PORT_ID,
	OTA_IP_PORT_ID,
		
	/* For TBoxDetectionTime */
	LteModuleDetectionTime_ID,
	LteAntennaDetectionTime_ID,
	USimDetectionTime_ID,
	AirBagBusInputDetectionTime_ID,
	EmmcDetectionTime_ID,
	CanCommunicationDetectionTime_ID,
	IVICommunicationDetectionTime_ID,
	PEPSCommunicationDetectionTime_ID,
	CollectReportTime_Id,
	GpsSpeedReportTime_Id,
	WifiModification_Id,
}TBoxDataId;

/***************************************************************************
* Function Name: TBoxDataParameter
* Auther       : yingaoguo
* Date         : 2017.11.15
* Description  : 需存储的数据
****************************************************************************/
typedef struct
{
	char wifiAPModeName[200];
	char wifiAPModePasssword[200];
	char wifiStaModeName[200];
	char wifiStaModePasssword[200];
	char bluetoothName[200];
	uint8_t APModeUsedIP[4];
	char imei[20];
	char iccid[20];
	char cimi[15];
	char E_CALL[20];
	char B_CALL[20];
	char I_CALL[20];
	char TBoxSerialNumber[40];
	char IVISerialNumber[40];
	char serverIpAndPort[6];     //服务器IP和端口
	char OTAIpAndPort[6];        //OTA服务器IP和端口
}TBoxDataParameter;


/***************************************************************************
* Function Name: TBoxDetectionTime
* Auther       : yingaoguo
* Date         : 2017.11.15
* Description  : 存储的自检时间
****************************************************************************/
typedef struct
{
	uint16_t LteModuleDetectionTime;
	uint16_t LteAntennaDetectionTime;
	uint16_t USimDetectionTime;
	uint16_t AirBagBusInputDetectionTime;
	uint16_t EmmcDetectionTime;
	uint16_t CanCommunicationDetectionTime;
	uint16_t IVICommunicationDetectionTime;
	uint16_t PEPSCommunicationDetectionTime;
	uint8_t CollectReport_Time;
	uint8_t GpsSpeedReport_Time;
	//WIFI是否修改用户名和密码	0：未修改	1：修改
	uint8_t IsWifiModification;
}TBoxDetectionTime;


typedef struct
{
	TBoxDataParameter dataPara;
	TBoxDetectionTime detectionTime;

}TBoxParameter;


/***************************************************************************
* Function Name: structure instruction
* Auther       : yingaoguo
* Date         : 2017.11.15
* Description  : store temporatory data,such as gps data, networking data
*                and so on.              
****************************************************************************/
typedef enum
{
	NETWORK_NULL = 0,
	NETWORK_LTE  = 1,
	NETWORK_WIFI = 2
}networkType;

typedef enum
{
	WIFI_NETWORK_DEFAULT          = 0,
	WIFI_NETWORK_AP_CONNECTED     = 1,
	WIFI_NETWORK_STA_CONNECTED    = 2,
	WIFI_NETWORK_AP_STA_CONNECTED = 3,
	WIFI_NETWORK_AP_AP_CONNECTED  = 4,
}wifiNetworkStatus;

typedef struct
{
	float x;
	float y;
	float z;
}TBoxVector;

typedef struct
{
    //GPRMC
    char fix_valid_ch; //定位状态
    uint16_t lat_deg; //纬度
    uint16_t lat_min; //纬度分
    uint16_t lat_min_frac; //纬度分小数
    uint16_t lon_deg; //经度
    uint16_t lon_min; //经度分
    uint16_t lon_min_frac; //经度分小数
    char lat_sense_ch; //N北纬，S南纬
    char lon_sense_ch; //E东经,W西经
    float speed; //速度KM/H
    float course; //地面航向度

    //GGA
    float height; //m
    uint8_t num_sats_used; //使用卫星数

    //GSA
    float pdop;
    float hdop;
    float vdop;

    //GSV
    uint8_t visible_sat; //可视卫星数
    uint8_t sat_info[32+24][2]; //可视卫星编号及信号强度
}TBoxGpsInfo;

//用于记录获取的时时状态信息
typedef struct
{
	//SIM卡是否拔出
	uint8_t isSIMCardPulledOut;
	//4G是否连网 0:没有网络, 1: lte网络正常
	uint8_t isLteNetworkAvailable;
	//网络连接类型 0:没有网络, 1: lte连接网络上网, 2: wifi sta连接网络上网
	uint8_t NetworkConnectionType;
	//Wifi连接状态
	uint8_t wifiConnectionType;
	//LTE注网状态      0,未注册网络, 1:注册上网络
	uint8_t networkRegSts;
	//网络信号强度
	uint8_t signalStrength;
}TBoxNetworkStatus;

typedef struct
{
	//emmc分区及升级标志
	volatile int isUpgrading;
	//tbox是否进入休眠        0:休眠 1:唤醒, 
	volatile int isGoToSleep;
	//tbox唤醒源 -1:初始值 0:gpio唤醒, 1:电话唤醒, 2:短信唤醒, 3:其它唤醒
	volatile int wakeupSource;
	//wifi启动状态 -1:wifi未起来, 0:wifi启动成功
	volatile int wifiStartStatus;
	//E-CALL: 1 /B-CALL: 2 /I-CALL: 3状态 0:初始值
	uint8_t phoneType;
}TBoxOperationStatus;



typedef struct
{
	TBoxNetworkStatus networkStatus; //网络连接状态信息
	TBoxOperationStatus operateionStatus;
	TBoxGpsInfo GpsInfo;       //GPS信息
	TBoxVector  acceData;      //加速度值
	TBoxVector  gyroData;      //角速度值

}TBoxInfo;

typedef struct
{
    //For LTE & MCU
    int upgradeFlag;
    unsigned char newLteVersion[4];
    unsigned char newMcuVersion[6];
    unsigned char newPLCVersion[20];

    unsigned char lteVersionSize[4];
    unsigned char mcuVersionSize[4];
    unsigned char plcVersionSize[4];

    unsigned char lteVersionCrc16[2];
    unsigned char mcuVersionCrc16[2];
    unsigned char plcVersionCrc16[2];

    //For PLC
    int sendPlcDataToMCUFlag;
    unsigned char newPlcVersion[20];
    unsigned char verionSize[4];
    unsigned char checkCodeCrc16[2];
} upgradeInfomation;







class TBoxDataPool
{
public:
	TBoxDataPool();
	~TBoxDataPool();
	int initTBoxInfo();
	int initPara(void);
	int readPara(void);
	int updatePara(void);
	int setPara(TBoxDataId id, void *pPara, uint32_t size);
	int getPara(TBoxDataId id, void *pPara, uint32_t size);
	int setTboxConfigInfo(TboxConfigID id, void *pPara, uint32_t size);
	int getTboxConfigInfo(TboxConfigID id, void *pPara, uint32_t size);
	int updateTboxConfig();

};

extern TBoxParameter TBoxPara; 
extern TBoxInfo tboxInfo;
extern upgradeInfomation upgradeInfo;
extern TboxConfigInfo_t TboxConfigInfo;

#endif

