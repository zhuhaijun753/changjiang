#ifndef _MCUUART_H
#define _MCUUART_H
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <sys/sysinfo.h>s
#include "common.h"
#include "TBoxDataPool.h"
#include "LTEModuleAtCtrl.h"
#include "GBT32960.h"
#include "FAWACP.h"
#include "VoiceCall.h"
#include "Message.h"
#include "DataCall.h"
#include "WDSControl.h"
#include "NASControl.h"
#include "dsi_netctrl.h"



#ifndef MCU_DEBUG
	#define MCU_DEBUG 0
#endif

#if MCU_DEBUG 	
	#define MCULOG(format,...) printf("== MCU == FILE: %s, FUN: %s, LINE: %d "format"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
	#define MCU_NO(format,...) printf(format,##__VA_ARGS__)
#else	
	#define MCULOG(format,...)
	#define MCU_NO(format,...)
#endif


#define MCU_UART_DEVICE	        "/dev/ttyHS1"    /* for mcu uart */
//#define BLUETOOTH_UART_DEVICE   "/dev/ttyHS0"     /* for bluetooth */

#define PLC_UPGRADE_FILE        "/data/plc.bin"
#define MCU_UPGRADE_FILE        "/data/MCU.bin"
#define RM_MCU_FILE             "rm -rf /data/MCU.bin"
#define LTE_FILE_NAME           "/data/LteUpgrade.bin"
#define RM_LTE_FILE             "rm -rf /data/LteUpgrade.bin"
#define MCU_REMOTE_REPORT_SUCCESS	0
#define MCU_REMOTE_REPORT_FAULT		1
#define CTRL_NUMBER_MAX			9


#define MCU_UART_SPEED	        460800
#define MCU_UART_STOP_BITS      1
#define MCU_UART_DATA_BITS      8
#define MCU_UART_CHECK_BIT      'n'
#define MCU_UART_TIMEOUT_MSECONDS  50

#define BUFF_LEN                1024
#define CONTENT_MAX_LEN         200

#define START_SYNC_CMD          0x00	//从机同步参数指令
#define MCU_GENERAL_RESP        0x01	//从机通用应答
#define TBOX_GENERAL_RESP       0x81	//主机通用应答
#define HEART_BEAT_CMD          0x10	//从机车况信息上报
#define L32960data               0x13    
#define TBOX_REPORT_4G_STATE    0x85	//主机状态汇报
#define TBOX_REPLY_MESSAGES_ID  0x8B	//主机应答短信发送消息
#define TBOX_REMOTECTRL_CMD	0x04	//从机控制应答
#define MCU_SND_TBOXINFO	0x12	//从机发送TBOX信息
#define MCU_SND_CONTROL  	0x15	//从机发送控制

#define close_Tbox               0x66
#define reset_Tbox               0x67
#define alarm1                   0x68
#define alarm2                   0x69
#define alarm3                   0x70


#define MCU_SND_MESSAGES_ID     0x0B	//从机发送短信消息
#define TEXT_TO_SPEECH_ID       0x0C	//从机发送语音消息
#define MCU_STATUS_REPORT		0x40	//从机状态汇报

#define MCU_INFROM_ROOTKEY		0x0F	//从机通知主机Rootkey更换
#define TBOX_ROOTKRY_RESULT		0x8F	//主机回复从机Rootkey更换结果

#define TBOX_SEND_UPGRADE_CMD   0x8a
#define TBOX_RECV_MCU_APPLY_FOR 0x86
#define MCU_SEND_COMPLETE       0x0A
#define MCU_REPORT_DRIVERDATA	0x07	//从机驾驶行为数据上报

#define MCU_Factory_Test    	0x82    //自动化测试模式
#define TBOX_REQ_CFGINFO    	0x92    //主机向从机请求获取Tbox信息
#define MCU_QUERY_SLEEP_MODE    0x0D  //
#define TBOX_QUERY_SLEEP_MODE   0x87   //


/* mcu upgrade tbox*/
#define MCU_SND_UPGRADE_INFO    0x0D
#define TBOX_REPLY_ID           0x8E
#define MCU_SND_UPGRADE_DATA    0x0E
#define MCU_SND_UPGRADE_CMPL    0x08     //COMPLETE
#define TBox_CHECK_REPORT	0xA0
#define MCU_CHECK_REPORT	0x20


/*Remote Ctrl Command MCU 8090*/
#define MCUVehBody_Lock		0x01	//车锁
#define MCUVehBody_Window 	0x02	//车窗
#define MCUVehBody_Sunroof	0x03	//天窗
#define MCUVehBody_TrackingCar	0x04	//寻车
#define MCUVehBody_Lowbeam	0x05	//近光灯
#define MCUAir_Control		0x06	//空调控制
#if 0 //
#define MCUAir_CompressorSwitch	0x07	//压缩机开关
#define MCUAir_Temperature	0X08	//温度调节
#define MCUAir_SetAirVolume	0x09	//设定风量
#define MCUAir_FrontDefrostSwitch 0x0a	//前除霜开关
#define MCUAir_Heatedrear	0x0b	//后窗加热
#define MCUAir_BlowingMode	0x0c	//吹风模式
#define MCUAir_InOutCirculate	0x0d	//内外循环
#define MCUAir_AutoSwitch	0x0e	//Auto开关
#endif
#define MCUEngineState_Switch	0x0f	//发动机状态启停
#define MCUVehSeat_DrivingSeat	0x10	//主驾驶座椅加热
#define MCUVehSeat_Copilotseat	0x11	//副驾驶座椅加热
#define MCUVehChargeMode_Immediate   0x12 //即时充电
#define MCUVehChargeMode_Appointment 0x13 //预约充电
#define MCUVehChargeMode_EmergeCharg 0x14 //紧急充电
#define MCUVeh_WIFIStatus	0x15	//WIFI状态
#define MCUVeh_AutoOUT		0x16	//自动出车
#define MCUVehBody_LuggageCar	0x17	//行李箱
#define MCUVehSeat_DrivingSeatMomery 0x18 //主驾驶座位记忆调去
#define MCUVeh_EnterRemoteModeFail   0xa0 //进入远程模式失败


/***************************************************************************
* Function Name: callBack_reportDate
* Function Introduction:
*               callback function, report the data which need by client 
*               per second. 
* Parameter description:
*	            pData  : data buffer.
*               length : data length
* Function return value: 
*                        void
* Data : 2017.10.10
****************************************************************************/
typedef void (*callBack_reportDate)(uint8_t *pData, uint16_t length);
typedef void (* callback_ReplayRemoteCtrl)();
typedef uint16_t (* callback_EventCmd)(uint8_t, AcpAppID_E);

//TBOX事件触发信号结构
typedef struct
{
	uint8_t s_TimeOutFileAlarm; //定时熄火信号
	//车辆告警及提醒
	uint8_t s_DoorIntrusAlarm;	//车门入侵告警
	uint8_t s_VehMoveAlarm;		//车辆异动提醒
	uint8_t s_VehHealthRemAlarm;	//剩余保养里程
	uint8_t s_LowOilAlarm;		//低油量报警
	uint8_t s_VehOutFileAlarm;	//紧急熄火
	uint8_t s_VehNeedStartEngine;	//需求启动发动机提示
	uint8_t s_VehSkylightUnclose;	//天窗未关告警
	//紧急救援数据上报
	uint8_t s_VehCollideAlarm;	//车辆碰撞
	uint8_t s_LoadRescueAlarm;	//道路救援上报
	//车况上报事件触发信号
	uint8_t s_VehAwakeEvent;	 //车辆唤醒事件上报
	uint8_t s_VehPowerEvent;	 //车辆上电事件上报
	uint8_t s_VehStartEvent;	 //车辆启动事件上报
	uint8_t s_TravelStartEvent;	 //行程开始事件上报
	uint8_t s_RapidDeceStartEvent;   //急减速开始事件上报
	uint8_t s_RapidDeceEndEvent;	 //急减速结束事件上报
	uint8_t s_RapidAcceStartEvent;	 //急加速开始事件上报
	uint8_t s_RapidAcceEndEvent;	 //急加速结束事件上报
	uint8_t s_RapidTurnStartEvent;	 //急转弯开始事件上报
	uint8_t s_RapidTurnEndEvent;	 //急转弯结束事件上报
	uint8_t s_OverSpeedStartEvent;	 //超速开始事件上报
	uint8_t s_OverSpeedEndEvent;	 //超速结束事件上报
	uint8_t s_VehOutFireEvent;	 //车辆熄火事件上报
	uint8_t s_TravelEndEvent;	 //行程结束事件上报
	uint8_t s_VehDownPowerEvent;	 //车辆下电事件上报
	uint8_t s_FailEvent;	    	 //故障事件上报
	uint8_t s_VehsleepEvent;    	 //车辆睡眠事件上报
}SIG_Event_t;


//故障信号列表
typedef struct
{
	uint8_t ACPCODEFAULTEMSState; //发动机管理系统故障
	uint8_t ACPCODEFAULTTCUState; //变速箱控制单元故障
	uint8_t ACPCODEFAULTEmissionState;//排放系统故障
	uint8_t ACPCODEFAULTSRSState;	//安全气囊系统故障
	uint8_t ACPCODEFAULTESPState;	//电子稳定系统故障
	uint8_t ACPCODEFAULTABSState;	//防抱死刹车系统
	uint8_t ACPCODEFAULTEPASState;	//电子助力转向系统
	uint8_t ACPCODEFAULTOilPressureState;//机油压力报警
	uint8_t ACPCODEFAULTLowOilIDState;	 //油量低报警
	uint8_t ACPCODEFAULTBrakeFluidLevelState;//制动液位报警
	uint8_t ACPCODEFAULTBattChargeState;	//蓄电池电量低报警
	uint8_t ACPCODEFAULTBBWState;	 //制动系统故障
	uint8_t ACPCODEFAULTTPMSState;	 //胎压系统故障
	uint8_t ACPCODEFAULTSTTState;    //启停系统故障
	uint8_t ACPCODEFAULTExtLightState;//外部灯光故障
	uint8_t ACPCODEFAULTESCLState;	 //电子转向柱锁故障
	uint8_t ACPCODEFAULTEngineOverwaterState;//发动机水温过高报警
	uint8_t ACPCODEFAULTElecParkUnitState;//电子驻车单元系统故障
	uint8_t ACPCODEFAULTAHBState;	//智能远光系统故障
	uint8_t ACPCODEFAULTACSState;	//自适应巡航系统故障
	uint8_t ACPCODEFAULTFCWSState;	//前碰撞预警系统故障
	uint8_t ACPCODEFAULTLDWState;	//道路偏离预警系统故障
	uint8_t ACPCODEFAULTBlindSpotDetectState;//盲区检测系统故障
	uint8_t ACPCODEFAULTAirconManualState;//空调人为操作
	uint8_t ACPCODEFAULTHVSystemState;//高压系统故障
	uint8_t ACPCODEFAULTHVInsulateState;//高压绝缘故障
	
	uint8_t ACPCODEFAULTHVILState; //高压互锁故障
	uint8_t ACPCODEFAULTEVCellState; //动力电池故障
	uint8_t ACPCODEFAULTPowerMotorState;//动力电机故障
	uint8_t ACPCODEFAULTEParkState;	//E-Park故障
	uint8_t ACPCODEFAULTEVCellLowBatteryState;//动力电池电量过低报警
	uint8_t ACPCODEFAULTEVCellOverTemperateState;//动力电池温度过高报警
	uint8_t ACPCODEFAULTPowerMotorOverTemperateState;//动力电机温度过高报警
	uint8_t ACPCODEFAULTConstantSpeedSystemFailState;//定速巡航系统故障
	uint8_t ACPCODEFAULTChargerFaultState;//充电机故障
	uint8_t ACPCODEFAULTAirFailureState;//空调故障
	uint8_t ACPCODEFAULTAlternateAuxSystemFailState;//换道辅助系统故障
	uint8_t ACPCODEFAULTAutoEmergeSystemFailState;//自动紧急制动系统故障
	uint8_t ACPCODEFAULTReverRadarSystemFailState;//倒车雷达系统故障
	uint8_t ACPCODEFAULTElecGearSystemFailState;//电子换挡器系统故障
	uint8_t LeftFrontTirePressAlarm;	 //左前胎压报警
	uint8_t LeftFrontTireTempAlarm;      //左前胎温报警
	uint8_t RightFrontTirePressAlarm;//右前胎压报警
	uint8_t RightrontTireTempAlarm;	//右前胎温报警
	uint8_t LeftRearTirePressAlarm;	//左后胎压报警
	uint8_t LeftRearTireTempAlarm;	//左后胎温报警
	uint8_t RightRearTirePressAlarm;//右后胎压报警
	uint8_t RightRearTireTempAlarm; //右后胎温报警
	uint8_t ACPCODEFAULTDCDCConverterFaultState;//直流直流转换器故障
	uint8_t ACPCODEFAULTVehControllerFailState;//整车控制器故障
	uint8_t MainPositRelayCoilFault;//主正继电器线圈状态
	uint8_t MainNegaRelayCoilFault;//主负继电器线圈状态
	uint8_t PrefillRelayCoilFault;//预充继电器线圈状态
	uint8_t RechargePositRelayCoilFault; //充电正继电器线圈状态
	uint8_t RechargeNegaRelayCoilFault;//充电负继电器线圈状态
	uint8_t MainPositiveRelayFault;//主正继电器故障
	uint8_t MainNagetiveRelayFault;//主负继电器故障
}Fault_Sigan_t;

typedef struct
{
	uint16_t MainPower;		//主电电源
	uint8_t  DeputyPower;	//备电电源
	uint8_t  CANStatus;		//Can通讯状态
	uint8_t  GyroscopeData[12];//陀螺仪数据
	uint8_t  McuVer[14];		//MCU版本
	uint8_t  HardweraVer[14];	//硬件版本
	uint8_t  GpsAntenna;	//Gps天线
	uint8_t  GpsPoist;		//Gps定位状态
	uint8_t  GpsSatelliteNumber;	//Gps卫星数目
	uint16_t GpsSatelliteInter[40];	//Gps第n个卫星编号及强度	
	uint8_t  GpsNumber[40];	//Gps卫星number
	uint8_t  GpsStrength[40];	//Gpsstrength
	uint16_t GPStime;		//gps启动时间

}McuStatus_t;



class mcuUart
{
public:
    mcuUart();
	static mcuUart * m_mcuUart;
	int mcuUartInit();
	int setUartSpeed(int fd, int speed);
	int setUartParity(int fd, int databits, int stopbits, int parity);
	int mcuUartReceiveData();
	int registerCallback_reportDate(callBack_reportDate func);
	uint32_t checkMcuUartData(unsigned char *pData, unsigned int size);
	int unpackMcuUartData(uint8_t *pData, unsigned int datalen);
	unsigned int Crc16Check(unsigned char* pData, uint32_t len);
	int unpack_syncParameter(unsigned char *pData, unsigned int len);
	int unpack_updatePositionInfo(unsigned char *pData, unsigned int len);
	int setSystemTime(struct tm *pTm);
	int unpack_updateGBT32960PositionInfo(unsigned char *pData, unsigned int len);
	int unpack_updateTimingInfo(unsigned char *pData, unsigned int len);
	int unpack_updateFAWACPPositionInfo(unsigned char *pData, unsigned int len);
	int unpack_updateFAWACPVehCondInfo(unsigned char* pData, unsigned int len);
	int unpack_text_messages(unsigned char* pData, unsigned int len); 
	int unpack_tts_voice(unsigned char* pData, unsigned int len);
	int unpack_querySleepMode(unsigned char* pData, unsigned int len);
	int unpack_TboxConfigInfo(unsigned char *pData, unsigned int len);
	int unpack_MCU_DriverInfo(unsigned char *pData, uint16_t datalen);
	int unpack_MCU_Upgrade_Check(unsigned char* pData, unsigned int len);
	int unpack_MCU_StatusReport(unsigned char *pData, uint16_t datalen);
	int packDataWithRespone(uint8_t responeCmd, uint8_t subCmd, uint8_t *needToPackData, uint16_t len, uint16_t serialNumber);
	int packProtocolData(uint8_t responeCmd, uint8_t subCmd, uint8_t *needToPackData, uint16_t len, uint16_t serialNum);
	int escape_mcuUart_data(unsigned char *pData, int len);
	uint16_t pack_report4GState(unsigned char *pBuff, int length);
	uint16_t pack_successMark(unsigned char *pBuff, uint8_t *pData, uint16_t len);
	int reportEventCmd(callback_EventCmd cb_EventCmd, uint8_t MsgType, AcpAppID_E AppID);

	static void *CheckEventThread(void *arg);
	void ReplayRemoteCtrl(callback_ReplayRemoteCtrl cb_TspRemoteCtrl);
	static int cb_RemoteConfigCmd(uint8_t SubitemCode, uint16_t SubitemVal);
	static int cb_RemoteCtrlCmd();
	static void Sig_CtrlAlarm(int signo);
	int unpack_RemoteCtrl(unsigned char *pData, uint16_t datalen);
	void close_uart();

	/*检测PWM波形*/
	static void *CheckPWMThread(void *arg);

	/* For tbox upgrade MCU */
	/*****************************************************************************
	* Function Name : pack_mcuUart_upgrade_data
	* Description	: pack upgrade data. 
	* Input 		: unsigned char cmd:升级指令
	                  bool isReceivedDataCorrect:是否收到了数据
	                  int mcuOrPlcFlag:升级mcu或者plc
	                                   0: MCU
	                                   1: PLC 
	* Output		: None
	* Return		: 0:success
	* Auther		: ygg
	* Date			: 2018.04.10
	*****************************************************************************/
	int pack_mcuUart_upgrade_data(unsigned char cmd, bool isReceivedDataCorrect, int mcuOrPlcFlag);
	unsigned short int pack_upgrade_cmd(unsigned char *pData, int len, int mcuOrPlcFlag);
	unsigned short int pack_upgrade_data(unsigned char *pData, int len, bool isReceivedDataCorrect, int mcuOrPlcFlag);
	void mcu_apply_for_data(unsigned char* pData, uint32_t len);
	unsigned char dataPacketOffsetAddr[4];
	unsigned char dataPacketLen[2];
	unsigned char fileName[64];

	/* For MCU upgrade tbox */
	int unpack_MCU_SND_Upgrade_Info(unsigned char* pData, unsigned int len);
	int unpack_MCU_SND_Upgrade_Data(unsigned char* pData, unsigned int len);
	int unpack_MCU_SND_Upgrade_CMPL();
	int calculate_files_CRC(char *fileName, uint32_t *crc);
	//unsigned int crc32Check(unsigned char *buff, unsigned int size);
	unsigned int crc32Check(unsigned int crc, unsigned char *buff, unsigned int size);
	int storageFile(uint8_t *fileName, uint8_t *pData, uint32_t len);
	uint32_t dataSize;
	uint32_t crc32;
	int m_id;
	uint8_t AccStatus;
	char tboxVersion[30];
	uint8_t RootkeyserialNumber;
	uint8_t 	RCtrlErrorCode;	//远程控制错误码
	int McuReportTimeOut;
	uint8_t	flagGetTboxCfgInfo;	//是否获取Tbox配置的信息(从MCU获取)
    
private:
	int fd;
	pthread_t CheckPWMThreadId;
	SIG_Event_t SigEvent;
	Fault_Sigan_t FaultSigan;
	callBack_reportDate reportDataFunc;
	pthread_t CheckEventThreadId;
	uint8_t Subitemnumber;
	int m_Remainder;	//id index
	int m_Id;			//record index
	int m_AlarmCount;	//record alarm index
	int storecount;
};


extern TBoxDataPool *dataPool;
extern LTEModuleAtCtrl *LteAtCtrl;



#endif
