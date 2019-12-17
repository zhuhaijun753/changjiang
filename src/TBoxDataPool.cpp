#include "TBoxDataPool.h"

upgradeInfomation upgradeInfo;
TBoxParameter TBoxPara;
TBoxInfo tboxInfo;
TboxConfigInfo_t TboxConfigInfo;
uint8_t isUsbConnectionStatus;
uint8_t InitStateVoiceCall;//记录当前电话状态



/*****************************************************************************
* Function Name : TBoxDataPool
* Description   : 构造函数
* Input			: None
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
TBoxDataPool::TBoxDataPool()
{
	initTBoxInfo();

	if (-1 == readPara())
	{
		initPara();
	}
}

//TBoxDataPool::~TBoxDataPool(){}

/*****************************************************************************
* Function Name : readPara
* Description   : 读取参数信息
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int TBoxDataPool::readPara(void)
{
    int fd = open(TBOX_PARA_FILE, O_RDWR | O_CREAT, 0664);
	
    if (-1 == fd)
    {
        DEBUGLOG("readPara open file failed\n");
        return -1;
    }
    DEBUGLOG("open file success\n");
	
    if (read(fd, &TBoxPara, sizeof(TBoxParameter)) <= 0)
    {
        DEBUGLOG("read failed!\n");
		close(fd);
        return -1;
    }
	close(fd);

    return 0;
}

/*****************************************************************************
* Function Name : initTBoxInfo
* Description   : 初始化TBOX信息
* Input			: None
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int TBoxDataPool::initTBoxInfo()
{
	memset(&TboxConfigInfo, 0, sizeof(TboxConfigInfo_t));
	isUsbConnectionStatus = 0;
	InitStateVoiceCall	  = 0;
	
	//初始化网络信息
	tboxInfo.networkStatus.isSIMCardPulledOut = 0;
	tboxInfo.networkStatus.isLteNetworkAvailable = NETWORK_NULL;//  NETWORK_LTE;
	tboxInfo.networkStatus.NetworkConnectionType = NETWORK_NULL;
	tboxInfo.networkStatus.wifiConnectionType = WIFI_NETWORK_DEFAULT; //WIFI_NETWORK_INIT;
	tboxInfo.networkStatus.networkRegSts = 0;
	tboxInfo.networkStatus.signalStrength = 0;

	tboxInfo.operateionStatus.isUpgrading = 0;
	tboxInfo.operateionStatus.isGoToSleep = -1;
	tboxInfo.operateionStatus.wakeupSource = -1;
	tboxInfo.operateionStatus.wifiStartStatus = -1;
	tboxInfo.operateionStatus.phoneType = 0;

	return 0;
}

/*****************************************************************************
* Function Name : initPara
* Description   : 初始化参数
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int TBoxDataPool::initPara(void)
{
	uint8_t buff[200];
	uint16_t temp;
	setPara(wifi_APMode_Name_Id, (void *)("TBOX_AP_STA"), strlen("TBOX_AP_STA"));
	setPara(wifiAPModePassswordId, (void *)("TCMSGDMN123"), strlen("TCMSGDMN123"));
	setPara(wifiStaModeNameId, (void *)("TEST1"), strlen("TEST1"));
	setPara(wifiStaModePassswordId, (void *)("ABCD1234abcd"), strlen("ABCD1234abcd"));
	setPara(bluetoothNameId, (void *)("blueetooth"), strlen("blueetooth"));
	
	memset(buff, 0, sizeof(buff)); 
	buff[0] = 0xc0;
	buff[1] = 0xa8;
	buff[2] = 0x04;
	buff[3] = 0x01;
	setPara(WIFI_APMODE_USED_IP, buff, 4);
	
	setPara(IMEI_INFO, (void *)("012345678901234"), strlen("012345678901234"));
	setPara(SIM_ICCID_INFO, (void *)("01234567890123456789"), strlen("01234567890123456789"));
	setPara(CIMI_INFO, (void *)("012345678901234"), strlen("012345678901234"));
	//printf("%s\n", TBoxPara.dataPara.cimi);

	setPara(E_CALL_ID, (void *)("4006518000"), strlen("4006518000"));
	setPara(B_CALL_ID, (void *)("4006518000"), strlen("4006518000"));
	setPara(I_CALL_ID, (void *)("4006518000"), strlen("4006518000"));
	memset(TBoxPara.dataPara.TBoxSerialNumber, 0, sizeof(TBoxPara.dataPara.TBoxSerialNumber));
	memset(TBoxPara.dataPara.IVISerialNumber, 0, sizeof(TBoxPara.dataPara.IVISerialNumber));
	//初始化服务器IP和端口
	uint16_t u16Port = 10003;
	uint8_t u8IP[4] = {113,116,77,246};

	memset(buff, 0, sizeof(buff)); 
	buff[0] = u8IP[0];
	buff[1] = u8IP[1];
	buff[2] = u8IP[2];
	buff[3] = u8IP[3];
	buff[4] = (u16Port & 0xff00)>>8;
	buff[5] = u16Port & 0xff;

	setPara(SERVER_IP_PORT_ID, buff, 6);
	DEBUGLOG("SERVER_IP_PORT_ID |%d,%d,%d,%d,%d,%d",buff[0],buff[1],buff[2],buff[3],buff[4],buff[5]);

	//初始化OTA服务器IP和端口
	u16Port = 2603;
	//u8IP[4] = {222,240,204,195};
	u8IP[0] = 222,
	u8IP[1] = 252,
	u8IP[2] = 100,
	u8IP[3] = 111,
	
	memset(buff, 0, sizeof(buff)); 
	buff[0] = u8IP[0];
	buff[1] = u8IP[1];
	buff[2] = u8IP[2];
	buff[3] = u8IP[3];
	buff[4] = (u16Port & 0xff00)>>8;
	buff[5] = u16Port & 0xff;

	setPara(OTA_IP_PORT_ID, buff, 6);
	DEBUGLOG("OTA_IP_PORT_ID |%d,%d,%d,%d,%d,%d",buff[0],buff[1],buff[2],buff[3],buff[4],buff[5]);

	/***************************************************************************
	*                                                                          *
	* Description  : intitialization TBoxDetectionTime structure               *
	*                                                                          *
	****************************************************************************/
	temp = 60;
	setPara(LteModuleDetectionTime_ID, &temp, sizeof(temp));
	temp = 60;
	setPara(LteAntennaDetectionTime_ID, &temp, sizeof(temp));
	temp = 60;
	setPara(USimDetectionTime_ID, &temp, sizeof(temp));
	temp = 60;
	setPara(AirBagBusInputDetectionTime_ID, &temp, sizeof(temp));
	temp = 60;
	setPara(EmmcDetectionTime_ID, &temp, sizeof(temp));
	temp = 60;
	setPara(CanCommunicationDetectionTime_ID, &temp, sizeof(temp));
	temp = 60;
	setPara(IVICommunicationDetectionTime_ID, &temp, sizeof(temp));
	temp = 60;
	setPara(PEPSCommunicationDetectionTime_ID, &temp, sizeof(temp));
	temp = 10;
	setPara(CollectReportTime_Id, &temp, sizeof(temp));
	temp = 5;
	setPara(GpsSpeedReportTime_Id, &temp, sizeof(temp));
	temp = 0;
	setPara(WifiModification_Id, &temp, sizeof(temp));
	/*printf("%d\n", TBoxPara.detectionTime.LteModuleDetectionTime);
	printf("%d\n", TBoxPara.detectionTime.LteAntennaDetectionTime);
	printf("%d\n", TBoxPara.detectionTime.USimDetectionTime);
	printf("%d\n", TBoxPara.detectionTime.AirBagBusInputDetectionTime);
	printf("%d\n", TBoxPara.detectionTime.EmmcDetectionTime);
	printf("%d\n", TBoxPara.detectionTime.CanCommunicationDetectionTime);
	printf("%d\n", TBoxPara.detectionTime.IVICommunicationDetectionTime);
	printf("%d\n", TBoxPara.detectionTime.PEPSCommunicationDetectionTime);*/

	return 0;
}

/*****************************************************************************
* Function Name : updatePara
* Description   : 更新参数
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int TBoxDataPool::updatePara(void)
{
    int fd = open(TBOX_PARA_FILE, O_RDWR);
    if ( -1 == fd)
    {
		printf("updatePara open file failed\n");
        return -1;
    }
    
    if ( -1 == write(fd, &TBoxPara, sizeof(TBoxParameter)))
    {
		printf("updatePara write file error!\n");
		close(fd);
        return -1;
    }
    close(fd);
	system("sync");

	return 0;
}

/*****************************************************************************
* Function Name : setPara
* Description   : 设置参数
* Input			: TBoxDataId id;
*                 void *pPara;
*                 uint32_t size
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int TBoxDataPool::setPara(TBoxDataId id, void *pPara, uint32_t size)
{
	switch (id)
	{
		case wifi_APMode_Name_Id:
			memset(TBoxPara.dataPara.wifiAPModeName, 0, sizeof(TBoxPara.dataPara.wifiAPModeName));
            memcpy(TBoxPara.dataPara.wifiAPModeName, pPara, (size>sizeof(TBoxPara.dataPara.wifiAPModeName))?sizeof(TBoxPara.dataPara.wifiAPModeName):size);
			break;
		case wifiAPModePassswordId:
			memset(TBoxPara.dataPara.wifiAPModePasssword, 0, sizeof(TBoxPara.dataPara.wifiAPModePasssword));
            memcpy(TBoxPara.dataPara.wifiAPModePasssword, pPara, (size>sizeof(TBoxPara.dataPara.wifiAPModePasssword))?sizeof(TBoxPara.dataPara.wifiAPModePasssword):size);
			break;
		case wifiStaModeNameId:
			memset(TBoxPara.dataPara.wifiStaModeName, 0, sizeof(TBoxPara.dataPara.wifiStaModeName));
            memcpy(TBoxPara.dataPara.wifiStaModeName, pPara, (size>sizeof(TBoxPara.dataPara.wifiStaModeName))?sizeof(TBoxPara.dataPara.wifiStaModeName):size);
			break;
		case wifiStaModePassswordId:
			memset(TBoxPara.dataPara.wifiStaModePasssword, 0, sizeof(TBoxPara.dataPara.wifiStaModePasssword));
            memcpy(TBoxPara.dataPara.wifiStaModePasssword, pPara, (size>sizeof(TBoxPara.dataPara.wifiStaModePasssword))?sizeof(TBoxPara.dataPara.wifiStaModePasssword):size);
			break;
		case bluetoothNameId:
			memset(TBoxPara.dataPara.bluetoothName, 0, sizeof(TBoxPara.dataPara.bluetoothName));
            memcpy(TBoxPara.dataPara.bluetoothName, pPara, (size>sizeof(TBoxPara.dataPara.bluetoothName))?sizeof(TBoxPara.dataPara.bluetoothName):size);
			break;
		case WIFI_APMODE_USED_IP:
			memset(TBoxPara.dataPara.APModeUsedIP, 0, sizeof(TBoxPara.dataPara.APModeUsedIP));
			memcpy(TBoxPara.dataPara.APModeUsedIP, pPara, size);
			break;
		case IMEI_INFO:
			memset(TBoxPara.dataPara.imei, 0, sizeof(TBoxPara.dataPara.imei));
			memcpy(TBoxPara.dataPara.imei, pPara, (size>sizeof(TBoxPara.dataPara.imei))?sizeof(TBoxPara.dataPara.imei):size);
			break;
		case SIM_ICCID_INFO:
			memset(TBoxPara.dataPara.iccid, 0, sizeof(TBoxPara.dataPara.iccid));
			memcpy(TBoxPara.dataPara.iccid, pPara, (size>sizeof(TBoxPara.dataPara.iccid))?sizeof(TBoxPara.dataPara.iccid):size);
			break;
		case CIMI_INFO:
			memset(TBoxPara.dataPara.cimi, 0, sizeof(TBoxPara.dataPara.cimi));
			memcpy(TBoxPara.dataPara.cimi, pPara, (size>sizeof(TBoxPara.dataPara.cimi))?sizeof(TBoxPara.dataPara.cimi):size);
			break;
		case E_CALL_ID:
			memset(TBoxPara.dataPara.E_CALL, 0, sizeof(TBoxPara.dataPara.E_CALL));
			memcpy(TBoxPara.dataPara.E_CALL, pPara, (size>sizeof(TBoxPara.dataPara.E_CALL))?sizeof(TBoxPara.dataPara.E_CALL):size);
			break;
		case B_CALL_ID:
			memset(TBoxPara.dataPara.B_CALL, 0, sizeof(TBoxPara.dataPara.B_CALL));
			memcpy(TBoxPara.dataPara.B_CALL, pPara, (size>sizeof(TBoxPara.dataPara.B_CALL))?sizeof(TBoxPara.dataPara.B_CALL):size);
			break;
		case I_CALL_ID:
			memset(TBoxPara.dataPara.I_CALL, 0, sizeof(TBoxPara.dataPara.I_CALL));
			memcpy(TBoxPara.dataPara.I_CALL, pPara, (size>sizeof(TBoxPara.dataPara.I_CALL))?sizeof(TBoxPara.dataPara.I_CALL):size);
			break;
		case TBoxSerialNumber_ID:
			memset(TBoxPara.dataPara.TBoxSerialNumber, 0, sizeof(TBoxPara.dataPara.TBoxSerialNumber));
			memcpy(TBoxPara.dataPara.TBoxSerialNumber, pPara, (size>sizeof(TBoxPara.dataPara.TBoxSerialNumber))?sizeof(TBoxPara.dataPara.TBoxSerialNumber):size);
			break;
		case IVISerialNumber_ID:
			memset(TBoxPara.dataPara.IVISerialNumber, 0, sizeof(TBoxPara.dataPara.IVISerialNumber));
			memcpy(TBoxPara.dataPara.IVISerialNumber, pPara, (size>sizeof(TBoxPara.dataPara.IVISerialNumber))?sizeof(TBoxPara.dataPara.IVISerialNumber):size);
			break;
		case SERVER_IP_PORT_ID:
			memcpy(TBoxPara.dataPara.serverIpAndPort, pPara, (size>sizeof(TBoxPara.dataPara.serverIpAndPort))?sizeof(TBoxPara.dataPara.serverIpAndPort):size);
			break;
		case OTA_IP_PORT_ID:
			memcpy(TBoxPara.dataPara.OTAIpAndPort, pPara, (size>sizeof(TBoxPara.dataPara.OTAIpAndPort))?sizeof(TBoxPara.dataPara.OTAIpAndPort):size);
			break;
		// For TBoxDetectionTime
		case LteModuleDetectionTime_ID:
			TBoxPara.detectionTime.LteModuleDetectionTime = *(uint16_t *)pPara;
			break;
		case LteAntennaDetectionTime_ID:
			TBoxPara.detectionTime.LteAntennaDetectionTime = *(uint16_t *)pPara;
			break;
		case USimDetectionTime_ID:
			TBoxPara.detectionTime.USimDetectionTime = *(uint16_t *)pPara;
			break;
		case AirBagBusInputDetectionTime_ID:
			TBoxPara.detectionTime.AirBagBusInputDetectionTime = *(uint16_t *)pPara;
			break;
		case EmmcDetectionTime_ID:
			TBoxPara.detectionTime.EmmcDetectionTime = *(uint16_t *)pPara;
			break;
		case CanCommunicationDetectionTime_ID:
			TBoxPara.detectionTime.CanCommunicationDetectionTime = *(uint16_t *)pPara;
			break;
		case IVICommunicationDetectionTime_ID:
			TBoxPara.detectionTime.IVICommunicationDetectionTime = *(uint16_t *)pPara;
			break;
		case PEPSCommunicationDetectionTime_ID:
			TBoxPara.detectionTime.PEPSCommunicationDetectionTime = *(uint16_t *)pPara;
			break;
		case CollectReportTime_Id:
			TBoxPara.detectionTime.CollectReport_Time = *(uint8_t *)pPara;
			break;
		case GpsSpeedReportTime_Id:
			TBoxPara.detectionTime.GpsSpeedReport_Time = *(uint8_t *)pPara;
			break;
		case WifiModification_Id:
			TBoxPara.detectionTime.IsWifiModification = *(uint8_t *)pPara;
			break;
		default:
			break;
	}

	if(updatePara() == 0)
		DEBUGLOG("update parameter successfully!");
	
	return 0;
}

/*****************************************************************************
* Function Name : getPara
* Description   : 获得参数
* Input			: TBoxDataId id;
*                 void *pPara;
*                 uint32_t size
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int TBoxDataPool::getPara(TBoxDataId id, void *pPara, uint32_t size)
{
	switch (id)
	{
		case wifi_APMode_Name_Id:
            memcpy(pPara, TBoxPara.dataPara.wifiAPModeName, size);
			break;
		case wifiAPModePassswordId:
            memcpy(pPara, TBoxPara.dataPara.wifiAPModePasssword, size);
			break;
		case wifiStaModeNameId:
            memcpy(pPara, TBoxPara.dataPara.wifiStaModeName, sizeof(TBoxPara.dataPara.wifiStaModeName));
			break;
		case wifiStaModePassswordId:
            memcpy(pPara, TBoxPara.dataPara.wifiStaModePasssword, sizeof(TBoxPara.dataPara.wifiStaModePasssword));
			break;
		case bluetoothNameId:
            memcpy(pPara, TBoxPara.dataPara.bluetoothName, sizeof(TBoxPara.dataPara.bluetoothName));
			break;
		case WIFI_APMODE_USED_IP:
			memcpy(pPara, TBoxPara.dataPara.APModeUsedIP, sizeof(TBoxPara.dataPara.APModeUsedIP));
			break;
		case IMEI_INFO:
			memcpy(pPara, TBoxPara.dataPara.imei, /*sizeof(TBoxPara.dataPara.imei)*/size);
			break;
		case SIM_ICCID_INFO:
			memcpy(pPara, TBoxPara.dataPara.iccid, /*sizeof(TBoxPara.dataPara.iccid)*/size);
			break;
		case CIMI_INFO:
			memcpy(pPara, TBoxPara.dataPara.cimi, size);
			break;
		case E_CALL_ID:
			memcpy(pPara, TBoxPara.dataPara.E_CALL, size);
			break;
		case B_CALL_ID:
			memcpy(pPara, TBoxPara.dataPara.B_CALL, size);
			break;
		case I_CALL_ID:
			memcpy(pPara, TBoxPara.dataPara.I_CALL, size);
			break;
		case TBoxSerialNumber_ID:
			memcpy(pPara, TBoxPara.dataPara.TBoxSerialNumber, size);
			break;
		case IVISerialNumber_ID:
			memcpy(pPara, TBoxPara.dataPara.IVISerialNumber, size);
			break;
		case SERVER_IP_PORT_ID:
			memcpy(pPara, TBoxPara.dataPara.serverIpAndPort, sizeof(TBoxPara.dataPara.serverIpAndPort));
			break;
		case OTA_IP_PORT_ID:
			memcpy(pPara, TBoxPara.dataPara.OTAIpAndPort, sizeof(TBoxPara.dataPara.OTAIpAndPort));
			break;
		// For TBoxDetectionTime
		case LteModuleDetectionTime_ID:
			*(uint16_t *)pPara = TBoxPara.detectionTime.LteModuleDetectionTime;
			break;
		case LteAntennaDetectionTime_ID:
			*(uint16_t *)pPara = TBoxPara.detectionTime.LteAntennaDetectionTime;
			break;
		case USimDetectionTime_ID:
			*(uint16_t *)pPara = TBoxPara.detectionTime.USimDetectionTime;
			break;
		case AirBagBusInputDetectionTime_ID:
			*(uint16_t *)pPara = TBoxPara.detectionTime.AirBagBusInputDetectionTime;
			break;
		case EmmcDetectionTime_ID:
			*(uint16_t *)pPara = TBoxPara.detectionTime.EmmcDetectionTime;
			break;
		case CanCommunicationDetectionTime_ID:
			*(uint16_t *)pPara = TBoxPara.detectionTime.CanCommunicationDetectionTime;
			break;
		case IVICommunicationDetectionTime_ID:
			*(uint16_t *)pPara = TBoxPara.detectionTime.IVICommunicationDetectionTime;
			break;
		case PEPSCommunicationDetectionTime_ID:
			*(uint16_t *)pPara = TBoxPara.detectionTime.PEPSCommunicationDetectionTime;
			break;
		case CollectReportTime_Id:
			*(uint8_t *)pPara = TBoxPara.detectionTime.CollectReport_Time;
			break;
		case GpsSpeedReportTime_Id:
			*(uint8_t *)pPara = TBoxPara.detectionTime.GpsSpeedReport_Time;
			break;
		case WifiModification_Id:
			*(uint8_t *)pPara = TBoxPara.detectionTime.IsWifiModification;
			break;
		default:
			break;
	}

	return 0;
}

/*****************************************************************************
* Function Name : updateTboxConfig
* Description   : 车辆信息VIN
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int TBoxDataPool::updateTboxConfig()
{
    int fd = open(VEHICLE_IDENTIFY_NUM_FILE, O_RDWR|O_CREAT|O_TRUNC, 0664);
    if ( -1 == fd)
    {
		DEBUGLOG("updateTboxConfig open file failed\n");
        return -1;
    }
    
    if ( -1 == write(fd, &TboxConfigInfo, sizeof(TboxConfigInfo_t)))
    {
		DEBUGLOG("write file error!\n");
		close(fd);
        return -1;
    }
	DEBUGLOG("vin set ok!\n");

    close(fd);
	system("sync");

	return 0;
}

int TBoxDataPool::setTboxConfigInfo(TboxConfigID id, void *pPara, uint32_t size)
{
	switch(id)
	{
		case VinID:
			memcpy(TboxConfigInfo.Veh_VinInfo, pPara, size);
			break;
		case ConfigCodeID:
			 TboxConfigInfo.ConfigCode.ConfigCode  = *(uint8_t *)pPara;
			break;		
		case emmcStatusID:
			 TboxConfigInfo.emmcPartitionStatus  = *(uint8_t *)pPara;
			break;
		case encryptionID:
			 TboxConfigInfo.encryptionStatus  = *(uint8_t *)pPara;
			break;
		case SUPPLYPN_ID:
			memcpy(TboxConfigInfo.supplyPN, pPara, size);
			break;		
		default:
			break;
	}
	return 0;
}

int TBoxDataPool::getTboxConfigInfo(TboxConfigID id, void *pPara, uint32_t size)
{
	int fd = open(VEHICLE_IDENTIFY_NUM_FILE, O_RDWR | O_CREAT, 0666);
	if(fd < 0)
	{
		DEBUGLOG("File does not exist!\n");
		return -1;
	}

	int retval = read(fd, &TboxConfigInfo, sizeof(TboxConfigInfo_t));
	if(retval < 0)
	{
		DEBUGLOG("retval:%d\n",retval);
	}

	switch(id)
	{
		case VinID:
			memcpy(pPara, TboxConfigInfo.Veh_VinInfo, size);
			break;
		case ConfigCodeID:
			*(uint8_t *)pPara = TboxConfigInfo.ConfigCode.ConfigCode;
			break;
		case emmcStatusID:
			*(uint8_t *)pPara = TboxConfigInfo.emmcPartitionStatus;
			break;
		case encryptionID:
			*(uint8_t *)pPara = TboxConfigInfo.encryptionStatus;
			break;
		case SUPPLYPN_ID:
			memcpy(pPara, TboxConfigInfo.supplyPN, size);
			break;
		default:
			break;
	}
	close(fd);
	return 0;
}

