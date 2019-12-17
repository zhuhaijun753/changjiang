#ifndef _FACTORYPATTERN_COMMUNICATION_H_
#define _FACTORYPATTERN_COMMUNICATION_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <time.h>

#include "mcuUart.h"
#include "TBoxDataPool.h"
#include "VoiceCall.h"
#include "WiFiControl.h"
#include "dsi_netctrl.h"
#include "LTEModuleAtCtrl.h"
#include <sys/syslog.h>
#include <sys/prctl.h>
#include "FAWACP.h"



#define FACTORYPATTERN_DEBUG_EN  0

#if FACTORYPATTERN_DEBUG_EN
	#define FACTORYPATTERN_LOG(format,...) printf("### IVI ### %s, %s[%d] "format"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
	#define FACTORYPATTERN_ERROR(format,...) fprintf(stderr, "### IVI ### %s, %s[%d] "format"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
	#define FACTORYPATTERN(format,...) printf(format,##__VA_ARGS__)
#else
	#define FACTORYPATTERN_LOG(format,...)
	#define FACTORYPATTERN_ERROR(format,...)
	#define FACTORYPATTERN(format,...)
#endif




#define FACTORYPATTERN_SERVER            "192.168.100.1"
#define FACTORYPATTERN_PORT              20001
#define LISTEN_BACKLOG        1
#define MAX_EVENT_NUMBER      10
#define BUFFER_SIZE           1024

#define MAX_SATELLITE_NUM	  40	

/* Protocol Related */
#define MSG_HEADER_A_ID     0x55AA	//平台发给终端
#define MSG_HEADER_T_ID     0xAA55	//终端发给平台

#define	MSG_COMMAND_ModeID		0x01	//模式选择
#define MSG_COMMAND_Detecting	0x02	//生产配置&测试


#define Pattern_FILE            "/data/PatternCfg"

#define	TAGID_CONFIG			0x01
#define	TAGID_CONFIG_Rep		0x61
#define	TAGID_CANSTATUS			0x02
#define	TAGID_CANSTATUS_Rep		0x62
#define	TAGID_CANDATA			0x03
#define	TAGID_CANDATA_Rep		0x63
#define	TAGID_SIXSENSOR			0x04
#define	TAGID_SIXSENSOR_Rep		0x64
#define	TAGID_Emmc				0x05
#define	TAGID_Emmc_Rep			0x65
#define	TAGID_WIFI				0x06
#define	TAGID_WIFI_Rep			0x66
#define	TAGID_BT				0x07
#define	TAGID_BT_Rep			0x67
#define	TAGID_IVISTATUS			0x08
#define	TAGID_IVISTATUS_Rep		0x68
#define	TAGID_APN2				0x09
#define	TAGID_APN2_Rep			0x69
#define	TAGID_ECall				0x0A
#define	TAGID_ECall_Rep			0x6A
#define	TAGID_GPSOpen			0x0B
#define	TAGID_GPSOpen_Rep		0x6B
#define	TAGID_GPSInterrupt		0x0C
#define	TAGID_GPSInterrupt_Rep	0x6C
#define	TAGID_GPSSIGN			0x0D
#define	TAGID_GPSSIN_Rep		0x6D
#define	TAGID_MAINPOWER			0x0E
#define	TAGID_MAINPOWER_Rep		0x6E
#define	TAGID_RESERVEPOWER		0x0F
#define	TAGID_RESERVEPOWER_Rep	0x6F
#define	TAGID_ACCOFF			0x10
#define	TAGID_ACCOFF_Rep		0x70
#define	TAGID_TBOXINFO			0x11
#define	TAGID_TBOXINFO_Rep		0x71








//TLV数据格式
typedef struct
{
	uint8_t		TagID;
	uint8_t		TLVLen;
	uint8_t		TLVData[128];
}TBox_TLV_ST;

typedef struct
{
	//uint8_t		VIN[17];		//VIN
	//uint8_t		VehicleType;	//车型号
	//uint8_t		Sim_Num[11];	//Sim卡号
	uint8_t		Sim_IMEI[15];	//IMEI	
	uint8_t		ICCID[20];		//ICCID
	uint8_t		Sim_IMSI[15];	//IMSI
	uint8_t		Supply_PN[11];	//供应商零件号
	uint8_t     TBoxSerialNumber[30]; //tbox序列号
}TBox_PatterInfo;

//生产配置
typedef struct
{
	uint8_t TBoxSerialNumber_Len; //tbox序列号长度
	uint8_t TBoxSerialNumber[30]; //tbox序列号
	/*uint8_t ServerDomain_Len;     //服务器域名长度
	uint8_t ServerDomain[30];     //服务器域名
	uint8_t Port_Len;             //端口长度
	uint16_t Port;                //端口号
	uint8_t APN1_Len;             //APN1长度
	uint8_t APN1[22];             //APN1
	uint8_t APN2_Len;             //APN2长度
	uint8_t APN2[22];			  //APN2
	uint8_t TBoxVIN_Len;	      //VIN长度
	uint8_t Tbox_VIN[17];	      //VIN*/
	//uint8_t	VehicleType_Len;      //车型号长度
	//uint8_t	VehicleType;	      //车型号
	uint8_t PowerDomain_Len;      //电源域长度
	uint8_t PowerDomain;	      //电源域
	uint8_t	RootKey_Len;          //ROOTKEY长度
	uint8_t	RootKey[64];          //ROOTKEY
	uint8_t SK_Len;               //SK长度
	uint8_t SK[6];                //SK
	uint8_t SupplierSN_Len;       //SN长度
	uint8_t SupplierSN[11];       //供应商序列号SN
}TBox_Config_ST;

//wifi状态监测
typedef struct
{
	uint8_t ssid[20];	 //wifi用户名
	uint8_t password[20];//wifi密码
	uint8_t Wifistate;	 //Wifi状态	
}TBox_Detecting_Wifi;

//GPS信号监测
typedef struct
{
	uint8_t SatelliteNum;	 //卫星总数0-40
	uint8_t SatelliteSign[2];//卫星信号强度	BYTE[0]:可视卫星编号；BYTE[1]:卫星星号强度；
}TBox_Detecting_GPS;

//生产测试指标检测
typedef struct
{
	uint8_t CanCommunication_Status;//测试CAN通讯连通状态【高4bit 为Can1 低4bit 为Can0，0x00：成功，0x01：失败】
	uint8_t CanCommunication_Data;	//发送CAN通讯测试数据
	uint8_t SixAxesSensor[12];	//测试六轴传感器【加速度 XYZ(精度0.1)角速度 XYZ(精度0.1)】
	uint8_t EmmcTrigger;		//测试EMMC【取消】
	TBox_Detecting_Wifi WifiTrigger;//测试wifi
	uint8_t BTTrigger;		//测试蓝牙【取消】
	uint8_t	IVICommunication_Status;//测试IVI联网【A5自检网络】
	uint8_t	APN2Trigger;	//测试APN2【A5自检网络】
	uint8_t	ECallTrigger;	//测试安全气囊
	uint8_t	GPSOpenTrigger;	//测试GPS开路【0x00: 正常0x01：开路0x02：短路】
	uint8_t	GPSShortTrigger;//测试GPS短路【0x00:未定位 0x01：已定位】
	TBox_Detecting_GPS	GPSSignTrigger[MAX_SATELLITE_NUM];	//测试GPS信号【n个(0-255)】
	uint8_t	MainPowerTrigger[2];//测试主电电源【0-60.0V，分辨率0.1V】
	uint8_t	ReservePowerTrigger;//测试备用电源【0-10.0V，分辨率0.1V】
	uint8_t	AccOffTrigger;	//测试ACC OFF
	TBox_PatterInfo	TBoxInfoTrigger;//获取TBOX信息
}TBox_Detecting_ST;
class FactoryPattern_Communication
{
public:
    FactoryPattern_Communication();
    ~FactoryPattern_Communication();
    int FactoryPattern_Communication_Init();
    int socketConnect();
	void et_process(struct epoll_event *events, int number, int epoll_fd, int socketfd);
    int set_non_block(int fd);
    void add_socketFd(int epoll_fd, int fd);

	uint8_t checkSum(uint8_t *pData, int len);
    uint8_t checkSum_BCC(uint8_t *pData, uint16_t len);
    uint8_t unpack_Protocol_Analysis(uint8_t *pData, int len);	
	//切换生产模式
	uint8_t unpack_ChangeTboxMode(uint8_t *pData, int len);
	//生产配置测试
	uint8_t unpack_Analysis_Detecting(uint8_t *pData, int len);
	uint8_t unpack_Analysis_Config(uint8_t *pData, int len);
	
	uint8_t pack_hande_data_and_send(uint16_t MsgID, uint8_t state);
	
	uint16_t pack_Protocol_Data(uint8_t *pData, int len, uint16_t MsgID, uint8_t state);
	uint16_t pack_mode_data(uint8_t *pData, int len);
	uint16_t pack_Config_data(uint8_t *pData, int len);
	uint16_t pack_Detect_data(uint8_t *pData, int len, uint8_t state);

	uint16_t pack_CANSTATUS_data(uint8_t *pData, int len);
	uint16_t pack_CANDATA_data(uint8_t *pData, int len);
	uint16_t pack_SIXSENSOR_data(uint8_t *pData, int len);
	uint16_t pack_Emmc_data(uint8_t *pData, int len);
	uint16_t pack_WIFI_data(uint8_t *pData, int len);
	uint16_t pack_BT_data(uint8_t *pData, int len);
	uint16_t pack_IVISTATUS_data(uint8_t *pData, int len);
	uint16_t pack_APN2_data(uint8_t *pData, int len);
	uint16_t pack_ECall_data(uint8_t *pData, int len);
	uint16_t pack_GPSOpen_data(uint8_t *pData, int len);
	uint16_t pack_GPSSHORT_data(uint8_t *pData, int len);
	uint16_t pack_GPSSIGN_data(uint8_t *pData, int len);
	uint16_t pack_MAINPOWER_data(uint8_t *pData, int len);
	uint16_t pack_RESERVEPOWER_data(uint8_t *pData, int len);
	uint16_t pack_ACCOFF_data(uint8_t *pData, int len);
	uint16_t pack_TBOXINFO_data(uint8_t *pData, int len);

	//测试wifi
	uint8_t GetDetecting_Wifi(TBox_Detecting_Wifi &detecting_Wifi);
	uint8_t GetDetecting_TboxInfo(TBox_PatterInfo PatterInfo);

	void StrToHex(unsigned char *Strsrc, unsigned char *Strdst, int Strlen);

private:
    int sockfd;
    int accept_fd;
    struct sockaddr_in serverAddr;

	
	uint8_t MsgSecurityVerion;

};

extern TBoxDataPool *dataPool;
extern LTEModuleAtCtrl *LteAtCtrl;
extern int decrypttest(unsigned char *pEncryptedKey, int input_len, int num, unsigned char *OutputKey);



#endif

