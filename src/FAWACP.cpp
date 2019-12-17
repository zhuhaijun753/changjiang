#include "FAWACP.h"
#include "AES_acp.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
CFAWACP* CFAWACP::cfawacp = NULL;
FAWACPInfo_Handle FAWAcpHandle;
FAWACPInfo_Handle *p_FAWACPInfo_Handle = &FAWAcpHandle;
extern bool APN2test;

CFAWACP::CFAWACP()
{
	m_Socketfd = -1;
	m_loginState = 0;
	m_SendMode = 0;		//发送模式0:即时发送	1:补发
	m_ConnectedState = false;
	m_bEnableSendDataFlag = true;   //发送使能标志
	cfawacp = this;			//全局对象指针赋值
	Headerlink = NULL;
	ConnectFaultTimes = 0;
	TboxSleepStatus = 1;
	InLoginFailTimes = 0;
	sleepeventrport = 0;
	HeartBeatTemp = 0;
	HighReportTime = 0;
	LowReportTime = 0;
	HeartBeatTime = 0;
	NowHighReportTime = 0;
	NowLowReportTime = 0;
	NowHeartBeatTime = 0;

	InitAcpTBoxAuthData();
	Init_FAWACP_dataInfo();

	if(pthread_create(&TimeOutThreadId, NULL, TimeOutThread, this) != 0)
		FAWACPLOG("Cannot creat TimeOutThread:%s\n", strerror(errno));
	
	socketConnect();	//连接服务器
	sleep(1);
	if(pthread_create(&receiveThreadId, NULL, receiveThread, this) != 0)
		FAWACPLOG("Cannot creat receiveThread:%s\n", strerror(errno));
	
	sleep(3);
//保持连接-
	while(1)
	{
		if(m_ConnectedState == true && tboxInfo.operateionStatus.isGoToSleep == 1)
		{
			if((m_loginState == 1) || (m_loginState == 0))
			{
				sleep(1);
				HeartBeatTemp = 0;
				sleepeventrport = 0;
				InLoginFailTimes++;
				//等待10秒若TSP无回馈则判定为超时,断开重连
				if(InLoginFailTimes == 10){
					InLoginFailTimes = 0;
					m_loginState = 0;
					m_ConnectedState = false;
				}
			}
			else if(m_loginState == 2)
			{
				InLoginFailTimes = 0;
				sleepeventrport = 0;
				HeartBeatTemp = 0;
				time(&NowHighReportTime);
				time(&NowLowReportTime);
				if(NowHighReportTime - HighReportTime >= TBoxPara.detectionTime.GpsSpeedReport_Time)
				{
					cfawacp->timingReportingData(MESSAGE_TYPE_START + 1, ACPApp_VehicleCondUploadID);
					time(&HighReportTime);
        		}
				if(NowLowReportTime - LowReportTime >= TBoxPara.detectionTime.CollectReport_Time)
				{
					cfawacp->timingReportingData(MESSAGE_TYPE_START + 2, ACPApp_VehicleCondUploadID);
					time(&LowReportTime);
				}				
				if(TboxSleepStatus != tboxInfo.operateionStatus.isGoToSleep)
				{
					if(tboxInfo.operateionStatus.isGoToSleep == 1)
						cfawacp->timingReportingData(MESSAGE_TYPE_START + 3, ACPApp_VehicleCondUploadID);
					TboxSleepStatus = tboxInfo.operateionStatus.isGoToSleep;
				}
			}
		}
		else if(m_ConnectedState == false && tboxInfo.operateionStatus.isGoToSleep == 1)
		{
			sleepeventrport = 0;
			HeartBeatTemp = 0;
			InLoginFailTimes = 0;
			disconnectSocket();
			InitAcpTBoxAuthData();
			sleep(5);
			socketConnect();
			sleep(1);
			if(pthread_create(&receiveThreadId, NULL, receiveThread, this) != 0)
				FAWACPLOG("Cannot creat receiveThread:%s\n", strerror(errno));
			FAWACPLOG("m_loginState == %d, m_loginState");

		}
		else/* if(tboxInfo.operateionStatus.isGoToSleep == 0)*/
		{
			if(m_ConnectedState == true)
			{
				InLoginFailTimes = 0;
				TboxSleepStatus = tboxInfo.operateionStatus.isGoToSleep;
				if(sleepeventrport == 0){
					cfawacp->timingReportingData(MESSAGE_TYPE_START + 19, ACPApp_VehicleCondUploadID);
					sleepeventrport = 1;
					HeartBeatTemp = 0;
				}
				time(&NowHeartBeatTime);
				if(NowHeartBeatTime - HeartBeatTime >= HEARTBEAT_TMIE){
					cfawacp->timingReportingData(MESSAGE_TYPE_START + 1, ACPApp_HeartBeatID);
					HeartBeatTemp++;
					time(&HeartBeatTime);
				}
				if(HeartBeatTemp >= 3 || m_loginState != 2){
					disconnectSocket();
				}
			}
			else
			{
				InLoginFailTimes = 0;
				sleepeventrport = 0;
				HeartBeatTemp = 0;
				TboxSleepStatus = tboxInfo.operateionStatus.isGoToSleep;
				system_power_sleep("FC_LOCK");
				sleep(1);
			}
		}
		usleep(1000);
	}
}


void CFAWACP::system_power_sleep(const char *lock_id)
{
	int fd;
	fd = open("/sys/power/wake_unlock", O_WRONLY);
	if (fd < 0) {
		FAWACPLOG("wake_unlock,error %d\n",fd);
		return;
	}

	write(fd, lock_id, strlen(lock_id));

	close(fd);
}


CFAWACP::~CFAWACP()
{
	pthread_mutex_destroy(&mutex);
}

#if 0   //不使用
int CFAWACP::VehicleReissueSend()
{
	int id = 0;
	int dataLen = 0;
	m_SendMode = 1;
	unsigned char *databuff = NULL;
	databuff = (unsigned char *)malloc(DATA_BUFF_SIZE);
	if(NULL == databuff){
		FAWACPLOG("databuff malloc error\n");
		return -1;
	}
	id = pSqliteDatabase->queryMinisSendID();
	if(id == 0){
		m_SendMode = 0;
		free(databuff);
		databuff = NULL;
		return -1;
	}
	pSqliteDatabase->updataisSend(0, id);
	pSqliteDatabase->readData(databuff, &dataLen, id);
	mcuUart::m_mcuUart->unpack_updatePositionInfo(databuff, dataLen);
	cfawacp->timingReportingData(MESSAGE_TYPE_START + 2, ACPApp_VehicleCondUploadID);
	m_SendMode = 0;
	if(NULL != databuff){
		free(databuff);
		databuff = NULL;
	}
	return 0;
}
#endif

/***************************************************************************
* Function Name: TimeOutThread
* Function Introduction:
*              间隔1s轮询检测链表节点，判断是否有超时节点，
			    并记录超时次数，执行指令动作，从链表中删除超时节点
	Data:
****************************************************************************/
void *CFAWACP::TimeOutThread(void *arg)
{
	pthread_detach(pthread_self());
	CFAWACP *pCFAWACP = (CFAWACP*)arg;

	while(1)
	{
		sleep(1);
		if(pCFAWACP->m_ConnectedState == true && pCFAWACP->m_loginState == 2)
		{
			//间隔1s轮询检测链表节点是否超时，并处理超时节点
			pCFAWACP->ConnectFaultTimes = 0;
			pCFAWACP->printf_link(&pCFAWACP->Headerlink);
			sleep(1);
		}
		else
		{
			/*2018-11-21修改:TSP长时间连接不上时，60s后重启tbox进程*/
			if(tboxInfo.operateionStatus.isGoToSleep == 1)
			{
				if(pCFAWACP->m_ConnectedState == false)
					pCFAWACP->ConnectFaultTimes++;
				else
					pCFAWACP->ConnectFaultTimes = 0;
			}
			else
			{
				pCFAWACP->ConnectFaultTimes = 0;
			}
		}

		//if(pCFAWACP->ConnectFaultTimes == 60)
			//system("pkill -9 tbox");
	}
	pthread_exit(0);
}



int CFAWACP::CheckTimer(LinkTimer_t *pb, uint8_t *flag)
{
	time_t NewTime = 0;
	time(&NewTime);
	switch(*flag)
	{
		case 0:
			if((NewTime - pb->time) >= 10)
			{
				*flag = 1;
				FAWACPLOG("ACP Application:%d First timeout\n",pb->AcpAppID);
				RespondTspPacket((AcpAppID_E)pb->AcpAppID, pb->MsgType, AcpCryptFlag_IS, pb->TspSoure);
				return 0;
			}
			break;
		case 1:
			if((NewTime - pb->time) >= 20)
			{
				*flag = 2;
				FAWACPLOG("ACP Application:%d second timeout\n",pb->AcpAppID);
				m_loginState = 0;
				m_ConnectedState = false;
				return 0;
			}
			break;
		default:
			break;
	}
	return -1;
}

void CFAWACP::printf_link(LinkTimer_t **head)
{
	int ret = -1;
	LinkTimer_t *pb, *pf;
	pf = *head;
	FAWACPLOG("head == %p", *head);

	while(pf != NULL)
	{
		switch(pf->AcpAppID)
		{
			case 2:
					ret = CheckTimer(pf, &TimeOutType.RemoteCtrlFlag);
					break;
			case 3:
					ret = CheckTimer(pf, &TimeOutType.VehQueCondFlag);
					break;
			case 4:
					ret = CheckTimer(pf, &TimeOutType.RemoteCpnfigFlag);
					break;
			case 5:
					ret = CheckTimer(pf, &TimeOutType.UpdateRootkeyFlag);
					break;
			case 9:
					ret = CheckTimer(pf, &TimeOutType.VehGPSFlag);
					break;
			case 11:
					ret = CheckTimer(pf, &TimeOutType.RemoteDiagnoFlag);
					break;
			default:
					break;
		}

		if(ret == 0)
		{
			pthread_mutex_lock(&mutex);
			*head = pf->next;
			free(pf);
			pf = *head;
			pthread_mutex_unlock(&mutex);
			ret = -1;
		}
		else
		{
			while(pf->next != NULL)
			{
				pb = pf;
				pf = pf->next;
				switch(pf->AcpAppID)
				{
					case 2:
						ret = CheckTimer(pf, &TimeOutType.RemoteCtrlFlag);
						break;
					case 3:
						ret = CheckTimer(pf, &TimeOutType.VehQueCondFlag);
						break;
					case 4:
						ret = CheckTimer(pf, &TimeOutType.RemoteCpnfigFlag);
						break;
					case 5:
						ret = CheckTimer(pf, &TimeOutType.UpdateRootkeyFlag);
						break;
					case 9:
						ret = CheckTimer(pf, &TimeOutType.VehGPSFlag);
						break;
					case 11:
						ret = CheckTimer(pf, &TimeOutType.RemoteDiagnoFlag);
						break;
					default:
						break;
				}
				if(ret == 0)
				{
					pthread_mutex_lock(&mutex);
					pb->next = pf->next;
					free(pf);
					pthread_mutex_lock(&mutex);
					ret = -1;
				}
			}
			return ;
		}
	}
}


void CFAWACP::insert(LinkTimer_t **head,LinkTimer_t *p_new)
{
	LinkTimer_t *pb;
	pb = *head;
	
	if(*head == NULL)
	{
		*head = p_new;
		p_new->next = NULL;
		return ;
	}
	
	while(pb->next != NULL)
	{
		pb = pb->next;
	}
	
	pb->next = p_new;
	p_new->next = NULL;
}

LinkTimer_t *CFAWACP::search (LinkTimer_t *head,uint8_t AcpAppID)
{
	LinkTimer_t *pb;
	pb = head;
	
	if(head == NULL)
		return NULL;
	
	while((pb->AcpAppID!=AcpAppID) && (pb->next!=NULL ))
	{
		pb = pb->next;
	}
	
	if(pb->AcpAppID == AcpAppID)
	{
		return pb;
	}
	return NULL;
}

void CFAWACP::delete_link(LinkTimer_t **head,uint8_t AcpAppID)
{
	LinkTimer_t *pb, *pf;
	pb = *head;
	if(*head == NULL)
		return ;

	while((pb->AcpAppID != AcpAppID) && (pb->next != NULL ))
	{
		pf = pb;
		pb = pb->next;
	}
	
	if(pb->AcpAppID == AcpAppID)
	{
		if(pb == *head)
		{
			*head = pb->next;
		}
		else
		{
			pf->next = pb->next;
		}
		free(pb);
	}
}


int CFAWACP::socketConnect()
{
	int fd;
#if 0
	char ServerIP[20];
	int  port;
	if(getServerIP(1, ServerIP, sizeof(ServerIP), NULL, "https://znwl-uat-exttj.faw.cn") == -1)  //(const char *)/*"https://znwl-uat-exttj.faw.cn"*/
	{
		FAWACPLOG("get ServerIP Fail\n"); 
		return -1;
	}
	port = (int )FAW_SERVER_PORT;
	FAWACPLOG("ServerIP:%s\n",ServerIP);

	if(getServerIP(0, ServerIP, sizeof(ServerIP), &port, NULL) == -1)
		return -1;
	FAWACPLOG("ServerIP: %s, port: %d\n", ServerIP, port);
#endif
#define DEBUG_TEST 1

#if DEBUG_TEST == 0
	//本地服务器调试
	memset(&m_socketaddr, 0, sizeof(m_socketaddr));
	m_socketaddr.sin_family = AF_INET;
	m_socketaddr.sin_port = htons(10003);
	m_socketaddr.sin_addr.s_addr = inet_addr("113.88.178.84");
#elif DEBUG_TEST == 1
//等待获取内网IP
	while(1)
	{
		if(strlen(target_ip) != 0)
			break;
		sleep(1);
	}
	memset(&m_socketaddr, 0, sizeof(m_socketaddr));
	m_socketaddr.sin_family = AF_INET;
	m_socketaddr.sin_port = htons(FAW_SERVER_PORT);
	m_socketaddr.sin_addr.s_addr = inet_addr(target_ip);
#endif
	int ret_socketConnect = -1;
	FAWACPLOG("Connect to ACP server: server IP:%s,PORT:%d\n",inet_ntoa(m_socketaddr.sin_addr),ntohs(m_socketaddr.sin_port));
	if((m_Socketfd = socket(AF_INET, SOCK_STREAM, 0)) != -1)
	{
		FAWACPLOG("ACP m_Socketfd:%d\n",m_Socketfd);
		ret_socketConnect = connect(m_Socketfd, (struct sockaddr*) &m_socketaddr, sizeof(m_socketaddr));

		FAWACPLOG("ACP ret_socketConnect:%d\n",ret_socketConnect);
		if (ret_socketConnect != -1)
		{
			APN2test = true;
			FAWACPLOG("Connect to server successfully,server IP:%s,PORT:%d\n",inet_ntoa(m_socketaddr.sin_addr),ntohs(m_socketaddr.sin_port));
			m_ConnectedState = true;
			ConnectFaultTimes = 0;
			
			sendLoginAuthApplyMsg();
		}
		else
		{
			m_ConnectedState = false;
			m_loginState = 0;
			FAWACPLOG("connect failed close m_Socketfd: %d\n",ret_socketConnect);
			return -1;
		}
	}
	return 0;
}
#if 0
//根据域名解析ip地址
int CFAWACP::getServerIP(int flag, char *ip, int ip_size, int *port, const char *domainName)
{
	FAWACPLOG("domainName = %s\n",domainName);
	if(flag == 0)
	{
		if((ip != NULL) && (port != NULL) && (ip_size >= 16))
		{
			uint8_t IP_Port[6];
			
			dataPool->getPara(SERVER_IP_PORT_ID, IP_Port, 6);
			
			memset(ip, 0, sizeof(ip));
			sprintf(ip, "%d.%d.%d.%d", IP_Port[0], IP_Port[1], IP_Port[2], IP_Port[3]);
			
			*port = (IP_Port[4] << 8) + IP_Port[5];
		}
	}
	else if(flag == 1)
	{
		if((ip != NULL) && (domainName != NULL) && (ip_size >= 16))
		{
			struct hostent *server = gethostbyname(domainName);
			if(server == NULL)
			{
				FAWACPLOG("get ip address fail\n");
			}
			else
			{
				FAWACPLOG("get ip address succees\n");
				strcpy(ip, inet_ntoa(*((struct in_addr *)server->h_addr_list)));
				FAWACPLOG("ServerIP:%s\n",ip);
			}
		}
	}
	else
	{
		return -1;
	}
}

#endif

int CFAWACP::disconnectSocket()
{
	shutdown(m_Socketfd, SHUT_RDWR);
	close(m_Socketfd);
	m_ConnectedState = false;
	m_loginState = 0;
	FAWACPLOG("Close socket m_Socketfd=%d",m_Socketfd);
	return 0;
}

int CFAWACP::initTBoxParameterInfo()
{
	int fd;
	FAWACPLOG("Init TBox para!!!!!!!!!!!!!!!!!!");
	fd = open(ACPPARAMETER_INFO_PATH, O_RDWR | O_CREAT|O_TRUNC, 0666);
	if (-1 == fd)
	{
		FAWACPLOG("FAWACP init open file failed\n");
		return -1;
	}

	Init_FAWACP_dataInfo();

    	if (-1 == write(fd, p_FAWACPInfo_Handle, sizeof(FAWACPInfo_Handle)))
    	{
			FAWACPLOG("write file error!\n");
        	return -1;
    	}
	system("sync");
		
	close(fd);
	return 0;
}

int CFAWACP::readTBoxParameterInfo()
{
	int fd;
	int retval;

	fd = open(ACPPARAMETER_INFO_PATH, O_RDONLY, 0666);
	if(fd < 0)
	{
		FAWACPLOG("File does not exist!\n");
		return -1;
	}
	else
	{
		FAWACPLOG("File Open success\n");
		retval = read(fd, p_FAWACPInfo_Handle, sizeof(FAWACPInfo_Handle));
		if(retval > 0)
		{
			FAWACPLOG("retval:%d\n",retval);
		}

		close(fd);
	}

	return 0;
}

int CFAWACP::updateTBoxParameterInfo()
{
	int fd;

	fd = open(ACPPARAMETER_INFO_PATH, O_RDWR, 0666);
	if(-1 == fd)
	{
		FAWACPLOG("FAWACP update open file failed\n");
		return -1;
	}
	
	if(-1 == write(fd, p_FAWACPInfo_Handle, sizeof(FAWACPInfo_Handle)))
	{
		FAWACPLOG("write file error!\n");
       		return -1;
	}

	system("sync");
	
	close(fd);

	return 0;
}

/****************************************************************
**初始化处理
****************************************************************/
//TBOX数据初始化，Tbox启动初始化一次
void CFAWACP::Init_FAWACP_dataInfo()
{
	uint8_t VerTboxMCU[12] = {0};
	uint8_t VerTboxOS[12] = {0x0};
	uint8_t EmergedCall[20] = {0};
	uint8_t LoadCell[20] = {0};
	uint8_t InformationCell[20] = {0};
	uint8_t WhitelistCall[15] = {0};
	uint8_t VerIVI[16] = {0};

	//新RootKey只初始化一次，重连不进行初始化
	memset(p_FAWACPInfo_Handle->New_RootKey, 0, sizeof(p_FAWACPInfo_Handle->New_RootKey));
	struct tm *p_tm = NULL; //时间的处理
	time_t tmp_time;
	tmp_time = time(NULL);
	p_tm = gmtime(&tmp_time);
	//采集信息时间
	struct timeval Time;
	gettimeofday(&Time, NULL);//获取时间距离微秒
#if 1
	//初始化TSP传的时间
	m_tspTimestamp.Year = p_tm->tm_year-90;//这里有函数处理
	m_tspTimestamp.Month = p_tm->tm_mon+1;//月，日，时，分，秒，分会变动
	m_tspTimestamp.Day = p_tm->tm_mday;
	m_tspTimestamp.Hour = p_tm->tm_hour;
	m_tspTimestamp.Minutes = p_tm->tm_min;
	m_tspTimestamp.Seconds = p_tm->tm_sec;
	m_tspTimestamp.msSeconds = Time.tv_usec / 1000; //毫秒
#endif
	//车况数据GPS数据
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.degree = 0;	//方向Last Value
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.second = p_tm->tm_sec;
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.minute = p_tm->tm_min;
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.hour = p_tm->tm_hour;
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.day = p_tm->tm_mday;
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.month = p_tm->tm_mon;
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.year = p_tm->tm_year - 90;
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.altitude = 0;	//高度last value
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitudeState = 0x2;//纬度状态0x0: 北纬0x1: 南纬0x2-0x3: 无效值
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitude = 0;	//纬度last value
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitudeState = 0x2;//经度状态0x0: 东经0x1: 西经0x2-0x3: 无效值last value
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitude = 0;	//经度last value
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.GPSState = 0x0;//定位状态0x0: 未定位 0x1: 定位 0x2-0x3: 无效值
	
	p_FAWACPInfo_Handle->VehicleCondData.RemainedOil.RemainedOilValue = 0x7F;//剩余油量数值 0x7F：初始值 0x7F：无效值 
	p_FAWACPInfo_Handle->VehicleCondData.RemainedOil.RemainedOilGrade = 0;//剩余油量等级
	p_FAWACPInfo_Handle->VehicleCondData.Odometer = 0;	//总里程Last Value
	p_FAWACPInfo_Handle->VehicleCondData.Battery = 0xFF;//蓄电池电量0xFF：初始值 0xFF：无效值
	p_FAWACPInfo_Handle->VehicleCondData.CurrentSpeed = 0x7FFE;//实时车速0x7FFE：初始值 0x7FFF：无效值
	p_FAWACPInfo_Handle->VehicleCondData.LTAverageSpeed = 0x7FFF;//长时平均车速0x7FFE：初始值 0x7FFF：无效值
	p_FAWACPInfo_Handle->VehicleCondData.STAverageSpeed = 0x7FFF;//短时平均车速0x7FFE：初始值 0x7FFF：无效值
	p_FAWACPInfo_Handle->VehicleCondData.LTAverageOil = 0xFFFE;//长时平均油耗0xFFFE：初始值 0xFFFF：无效值
	p_FAWACPInfo_Handle->VehicleCondData.STAverageOil = 0xFFFE;//短时平均油耗0xFFFE：初始值 0xFFFF：无效值
	//车门状态数据
	p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.bitState.drivingDoor = 0X0;//驾驶门开关状态0x0：初始值 0x0：关闭 0x1：开启
	p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.bitState.copilotDoor = 0X0;//副驾驶门开关状态0x0：初始值 0x0：关闭 0x1：开启
	p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.bitState.leftRearDoor = 0X0;//左后门开关状态0x0：初始值 0x0：关闭 0x1：开启
	p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.bitState.rightRearDoor = 0X0;//右后门开关状态0x0：初始值 0x0：关闭 0x1：开启
	p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.bitState.rearCanopy = 0X0;//后舱盖状态0x0：初始值 0x0：关闭 0x1：开启
	p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.bitState.engineCover = 0X0;//引擎盖状态0x0：初始值 0x0：关闭 0x1：开启
	//车锁状态数据
	p_FAWACPInfo_Handle->VehicleCondData.CarLockState.bitState.rightRearLock = 0X0;//右后门车锁状态0x0：初始值 0x0: 闭锁 0x1: 解锁 0x2-0x3: 无效值
	p_FAWACPInfo_Handle->VehicleCondData.CarLockState.bitState.leftRearLock = 0X0;//左后门车锁状态0x0：初始值 0x0: 闭锁 0x1: 解锁 0x2-0x3: 无效值
	p_FAWACPInfo_Handle->VehicleCondData.CarLockState.bitState.copilotLock = 0X0;//副驾驶门车锁状态0x0：初始值 0x0: 闭锁 0x1: 解锁 0x2-0x3: 无效值
	p_FAWACPInfo_Handle->VehicleCondData.CarLockState.bitState.drivingLock = 0X0;//驾驶员门车锁状态0x0：初始值 0x0: 闭锁 0x1: 解锁 0x2-0x3: 无效值
	p_FAWACPInfo_Handle->VehicleCondData.sunroofState = 0x0;//天窗状态(0x0关闭,0x1开启,0x2翘起,0x3其他)
	//车窗状态数据
	p_FAWACPInfo_Handle->VehicleCondData.WindowState.bitState.leftFrontWindow = 0x0;//左前车窗状态0x0: 关闭0x1: 开启0x2: 防夹0x3: 堵转0x4-0x7: 其他
	p_FAWACPInfo_Handle->VehicleCondData.WindowState.bitState.rightFrontWindow = 0x0;//右前车窗状态0x0: 关闭0x1: 开启0x2: 防夹0x3: 堵转0x4-0x7: 其他
	p_FAWACPInfo_Handle->VehicleCondData.WindowState.bitState.leftRearWindow = 0x0;//左后车窗状态0x0: 关闭0x1: 开启0x2: 防夹0x3: 堵转0x4-0x7: 其他
	p_FAWACPInfo_Handle->VehicleCondData.WindowState.bitState.rightRearWindow = 0x0;//右后车窗状态0x0: 关闭0x1: 开启0x2: 防夹0x3: 堵转0x4-0x7: 其他
	//车灯状态数据
	p_FAWACPInfo_Handle->VehicleCondData.CarlampState.bitState.headlights = 0x0;//远光灯0x0: 关闭0x1: 开启
	p_FAWACPInfo_Handle->VehicleCondData.CarlampState.bitState.positionlights = 0x0;//位置灯0x0: 关闭0x1: 开启
	p_FAWACPInfo_Handle->VehicleCondData.CarlampState.bitState.nearlights = 0x0;//近光灯0x0: 关闭0x1: 开启
	p_FAWACPInfo_Handle->VehicleCondData.CarlampState.bitState.rearfoglights = 0x0;//后雾灯0x0: 关闭0x1: 开启
	p_FAWACPInfo_Handle->VehicleCondData.CarlampState.bitState.frontfoglights = 0x0;//前雾灯0x0: 关闭0x1: 开启
	//轮胎信息数据0xFF：初始值 0xFF：无效值
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.rightrearTyrePress = 0xFF;//左前胎压温度
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.leftrearTyrePress = 0xFF;//右前胎压温度
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.rightfrontTyrePress = 0xFF;//左后胎压温度
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.leftfrontTyrePress = 0xFF;//右后胎压温度
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.rightrearTemperature = 0xFF;//左前胎压压力
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.leftrearTemperature = 0xFF;//右前胎压压力
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.rightfrontTemperature = 0xFF;//左后胎压压力
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.leftfrontTemperature = 0xFF;//右后胎压压力
	//TBox_MCU版本
	memcpy(p_FAWACPInfo_Handle->VehicleCondData.VerTboxMCU,VerTboxMCU,sizeof(VerTboxMCU));
	p_FAWACPInfo_Handle->VehicleCondData.EngineState = 0x0;//发动机状态(0x0关闭,0x1起动机拖动,0x2发动机运行,0x3正在停机,0x4-0x6保留,0x7无效)
	//实时方向盘转角数据
	p_FAWACPInfo_Handle->VehicleCondData.WheelState.bitState.wheeldegree = 0x0;//方向盘转角(度)
	p_FAWACPInfo_Handle->VehicleCondData.WheelState.bitState.wheeldirection = 0x0;//方向盘方向0x0：左 0x1：右
	//历史车速Kph(记录19次车速)
	for(uint8_t i = 0; i < 19; i++){
		p_FAWACPInfo_Handle->VehicleCondData.PastRecordSpeed[i] = 0x7FFF;
	}
	//历史方向盘转角(记录19次车速)
	for(uint8_t i = 0; i < 19; i++){
		p_FAWACPInfo_Handle->VehicleCondData.PastRecordWheel[i].bitState.wheeldegree = 0x0;
		p_FAWACPInfo_Handle->VehicleCondData.PastRecordWheel[i].bitState.wheeldirection = 0x0;
	}
	p_FAWACPInfo_Handle->VehicleCondData.EngineSpeed = 0x0;//发动机转速
	p_FAWACPInfo_Handle->VehicleCondData.Gearstate = 0x0;//档位信息(0x0:P,0x1:R,0x2:N,0x3:D,0x4:S,0x5:M,0x6:W,0x7:NoConnect,0x8-0xE:保留,0xF:无效)
	p_FAWACPInfo_Handle->VehicleCondData.HandbrakeState = 0x0;//手刹状态(0x0:未激活,0x1:激活,0x2:保留,0x3:错误)
	p_FAWACPInfo_Handle->VehicleCondData.ParkingState = 0x0;//驻车状态(0x0:加紧,0x1:正在加紧/释放,0x2:释放,0x3:错误)
	p_FAWACPInfo_Handle->VehicleCondData.Powersupplymode = 0x0;//供电模式0x0--0x8,0x9---0xF
	p_FAWACPInfo_Handle->VehicleCondData.Safetybeltstate = 0x0;//安全带状态0x0:未锁,0x1:锁止,0x2/0x3:保留
	p_FAWACPInfo_Handle->VehicleCondData.RemainUpkeepMileage = 0x0;//剩余保养里程
	//空调相关信息
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature = 0X7E;//空调温度设定
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.blowingMode = 0x0;//空调吹风模式
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.blowingLevel = 0x0;//空调吹风等级
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.inOutCirculateState = 0x0;//空调内外循环状态
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.defrostState = 0x0;//空调强制除霜状态
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.autoState = 0x0;//空调Auto状态
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.compressorState = 0x0;//空调压缩机工作状态
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.airconditionerState = 0x0;//空调工作状态
	//持续时间信息 除在事件结束时发真实时间，其余皆发送初始值
	p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.overspeedTime = 0xFFFE;//超速持续时间
	p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.wheelTime = 0xFE;//急转弯持续时间
	p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.decelerateTime = 0xFE;//急减速持续时间
	p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.accelerateTime = 0xFFFE;//急加速持续时间
	//动力电池状态数据
	p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.BatAveraTempera = 0x3C;//电池包平均温度
	p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.elecTempera = 0x3C;//电池进水口温度
	p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.elecSOH = 0xFF;//电池SOH
	p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.quantity = 0xFF;//电池电量
	p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.electricity = 0x1F40;//电池电流 0x3FFF：无效值
	p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.voltage = 0x14A;//电池电压 0x3FF：无效值

	p_FAWACPInfo_Handle->VehicleCondData.ChargeState.chargeState = 0x0;//充电状态
	p_FAWACPInfo_Handle->VehicleCondData.ChargeState.remainChargeTime = 0x0;//剩余充电时间minute 0xFF：无效值
	p_FAWACPInfo_Handle->VehicleCondData.ChargeState.chargeMode = 0x0;	//充电模式
	memcpy(p_FAWACPInfo_Handle->VehicleCondData.VerTboxOS,VerTboxOS,sizeof(VerTboxOS));//TBOX版本号
	memcpy(p_FAWACPInfo_Handle->VehicleCondData.VerIVI, VerIVI, sizeof(VerIVI));//IVI版本号
	p_FAWACPInfo_Handle->VehicleCondData.ChargeConnectState = 0x0;//充电枪连接状态
	p_FAWACPInfo_Handle->VehicleCondData.BrakePedalSwitch = 0x0;	//制动踏板开关
	p_FAWACPInfo_Handle->VehicleCondData.AcceleraPedalSwitch = 0x0;	//加速踏板开关
	//Yaw传感器信息
	p_FAWACPInfo_Handle->VehicleCondData.YaWSensorInfoSwitch.TransverseAccele = 0xFFE;//横向加速度
	p_FAWACPInfo_Handle->VehicleCondData.YaWSensorInfoSwitch.LongituAccele = 0xFFE;//纵向加速度
	p_FAWACPInfo_Handle->VehicleCondData.YaWSensorInfoSwitch.YawVelocity = 0xFFE;//横摆角速度
	//环境温度
	p_FAWACPInfo_Handle->VehicleCondData.AmbientTemperat.AmbientTemperat = 0x7FE;
	//纯电动继电器及线圈相关状态
	p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.MainPositRelayCoilState = 0;
	p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.MainNegaRelayCoilState = 0;
	p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.PrefillRelayCoilState = 0;
	p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.RechargePositRelayCoilState = 0;
	p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.RechargeNegaRelayCoilState = 0;
	//剩余续航里程(12bit)
	p_FAWACPInfo_Handle->VehicleCondData.ResidualRange = 0;
	//新能源热管理相关请求
	p_FAWACPInfo_Handle->VehicleCondData.NewEnergyHeatManage.BatteryHeatRequest = 0;
	p_FAWACPInfo_Handle->VehicleCondData.NewEnergyHeatManage.Motor1CoolRequest = 0;
	p_FAWACPInfo_Handle->VehicleCondData.NewEnergyHeatManage.Motor2CoolRequest = 0;
	//车辆工作模式
	p_FAWACPInfo_Handle->VehicleCondData.VehWorkMode.VehWorkMode = 0x0;
	//电机工作状态
	p_FAWACPInfo_Handle->VehicleCondData.MotorWorkState.Motor1Workstate = 0x0;
	p_FAWACPInfo_Handle->VehicleCondData.MotorWorkState.Motor2Workstate = 0x0;
	//高压系统准备状态
	p_FAWACPInfo_Handle->VehicleCondData.HighVoltageState = 0x0;
	//远程控制命令数据
	//车身
	p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_Lock = 0;//车锁
	p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_Window = 0;//车窗
	p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_Sunroof = 0;//天窗
	p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_TrackingCar = 0;//寻车
	p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_Lowbeam = 0;//近光灯
	p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_LuggageCar = 0;//行李箱
	//空调控制
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Control.dataBit.dataState = 0;//空调控制
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Control.dataBit.flag = 0;
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_CompressorSwitch.dataBit.dataState = 0;//压缩机开关
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_CompressorSwitch.dataBit.flag = 0;
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Temperature.dataBit.dataState = 0x0;//温度调节
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Temperature.dataBit.flag = 0;
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_SetAirVolume.dataBit.dataState = 0x0;//设定风量
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_SetAirVolume.dataBit.flag = 0;
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_FrontDefrostSwitch.dataBit.dataState = 0;//前除霜开关
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_FrontDefrostSwitch.dataBit.flag = 0;
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Heatedrear.dataBit.dataState = 0;//后窗加热
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Heatedrear.dataBit.flag = 0;
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_BlowingMode.dataBit.dataState = 0x0;//吹风模式
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_BlowingMode.dataBit.flag = 0;
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_InOutCirculate.dataBit.dataState = 0x0;//内外循环
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_InOutCirculate.dataBit.flag = 0;
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_AutoSwitch.dataBit.dataState = 0;//Auto开关
	p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_AutoSwitch.dataBit.flag = 0;
	//发动机
	p_FAWACPInfo_Handle->RemoteControlData.EngineState.EngineState_Switch = 0;//发动机状态启停
	//座椅
	p_FAWACPInfo_Handle->RemoteControlData.VehicleSeat.VehicleSeat_DrivingSeat = 0;//主驾驶座椅加热
	p_FAWACPInfo_Handle->RemoteControlData.VehicleSeat.VehicleSeat_Copilotseat = 0;//副驾驶座椅加热
	p_FAWACPInfo_Handle->RemoteControlData.VehicleSeat.VehicleSeat_DrivingSeatMemory = 0;//主驾驶座椅记忆
	//预约充电
	p_FAWACPInfo_Handle->RemoteControlData.VehicleCharge.VehicleCharge_Immediate = 0;//即时充电
	p_FAWACPInfo_Handle->RemoteControlData.VehicleCharge.VehicleCharge_Appointment = 0;//预约充电
	//自动泊车
	p_FAWACPInfo_Handle->RemoteControlData.VehicleAutopark.VehicleWifiStatus = 0;//wifi状态
	p_FAWACPInfo_Handle->RemoteControlData.VehicleAutopark.VehicleAutoOut = 0;//自动出车
	//功能配置状态
	p_FAWACPInfo_Handle->RemoteControlData.FunctionConfigStatus = 0;//功能配置状态
	//远程配置信息数据0 == TBOX 1 == ECU
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].DeviceNo = 0;//设备编号
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].EngineStartTime = 15;//发动机远程启动时长,单位min
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].SamplingSwitch = 0;//电动车国标抽检开关

	dataPool->getPara(B_CALL_ID, (void *)LoadCell, sizeof(LoadCell));
	dataPool->getPara(E_CALL_ID, (void *)EmergedCall, sizeof(EmergedCall));
	memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].EmergedCall,EmergedCall,15);//紧急服务号码配置(ASCII码)
	memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].WhitelistCall,WhitelistCall,15);//白名单号码配置(ASCII码)
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].CollectTimeInterval = 10;//车况定时采集间隔时间,单位S,默认10
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].ReportTimeInterval = 10;//车况定时上报间隔时间,单位S,默认10
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].CollectTimeGpsSpeedInterval = 5;//定时采集(位置和车速)间隔时间,单位S,默认5
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].ReportTimeGpsSpeedInterval = 5;//定时上报(位置和车速)间隔时间,单位S,默认5
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].ChargeMode = 0x0;//充电模式
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].ChargeSchedule.ScheduChargStartData = 0;//预约充电开始日1-7—>周一到周日
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].ChargeSchedule.ScheduChargStartTime = 0;//预约充电开始时间,单位分钟
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].ChargeSchedule.ScheduChargEndData = 0;//预约充电结束日
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].ChargeSchedule.ScheduChargEndTime = 0;//预约充电结束时间
	memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].LoadCell,LoadCell,15);//B-Cell(ASCII码)
	memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].InformationCell,InformationCell,15);//I-Cell(ASCII码)

	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].DeviceNo = 1;//设备编号
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].EngineStartTime = 15;//发动机远程启动时长,单位min
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].SamplingSwitch = 0;//电动车国标抽检开关
	memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].EmergedCall, EmergedCall, sizeof(EmergedCall));//紧急服务号码配置(ASCII码)
	memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].WhitelistCall, WhitelistCall, sizeof(WhitelistCall));//白名单号码配置(ASCII码)
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].CollectTimeInterval = 10;//车况定时采集间隔时间,单位S,默认10
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].ReportTimeInterval = 10;//车况定时上报间隔时间,单位S,默认10
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].CollectTimeGpsSpeedInterval = 5;//定时采集(位置和车速)间隔时间,单位S,默认5
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].ReportTimeGpsSpeedInterval = 5;//定时上报(位置和车速)间隔时间,单位S,默认5
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].ChargeMode = 0x0;//充电模式
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].ChargeSchedule.ScheduChargStartData = 0;//预约充电开始日1-7—>周一到周日
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].ChargeSchedule.ScheduChargStartTime = 0;//预约充电开始时间,单位分钟
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].ChargeSchedule.ScheduChargEndData = 0;//预约充电结束日
	p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].ChargeSchedule.ScheduChargEndTime = 0;//预约充电结束时间
	memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].LoadCell,LoadCell,15);//B-Cell(ASCII码)
	memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[1].InformationCell,InformationCell,15);//I-Cell(ASCII码)

	//**********************故障信息数据*******************//
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEMSState = 0x0;//发动机管理系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTCUState = 0x0;//变速箱控制单元故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEmissionState = 0x0;//排放系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSRSState = 0x0;//安全气囊系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESPState = 0x0;//电子文档系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTABSState = 0x0;//防抱死刹车系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEPASState = 0x0;//电子助力转向系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTOilPressureState = 0x0;//机油压力报警0x0: 未报警0x1: 报警0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLowOilIDState = 0x0;//油量低报警0x0: 未报警0x1: 报警0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBrakeFluidLevelState = 0x0;//制动液位报警0x0: 未报警0x1: 报警0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBatteryChargeState = 0x0;//蓄电池充电故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBBWState = 0x0;//制动系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTPMSState = 0x0;//胎压系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSTTState = 0x0;//启停系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTExtLightState = 0x0;//外部灯光故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESCLState = 0x0;//电子转向柱锁故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEngineOverwaterState = 0x0;//发动机水温过高报警0x0: 未报警0x1: 报警0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTElecParkUnitState = 0x0;//电子驻车单元系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAHBState = 0x0;//智能远光系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTACSState = 0x0;//自适应巡航系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWSState = 0x0;//前碰撞预警系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLDWState = 0x0;//道路偏离预警系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBlindSpotDetectState = 0x0;//盲区检测系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAirconManualState = 0x0;//空调人为操作0x0: 无操作0x1: 有操作0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVSystemState = 0x0;//高压系统故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVInsulateState = 0x0;//高压绝缘故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVILState = 0x0;//高压互锁故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBatteryChargeState = 0x0;//蓄电池充电故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellState = 0x0;//动力电池故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPowerMotorState = 0x0;//动力电机故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEParkState = 0x0;//E-Park故障0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellLowBatteryState = 0x0;//动力电池电量过低报警0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellOverTemperateState = 0x0;//动力电池温度过高报警0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPowerMotorOverTemperateState = 0x0;//动力电机温度过高报警0x0: 无故障0x1: 有故障0x2: 预留0x3: 预留
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTConstantSpeedSystemFailState = 0x0;//定速巡航系统故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTChargerFaultState = 0x0;			 //充电机故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAirFailureState = 0x0;			 //空调故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlternateAuxSystemFailState = 0x0; //换道辅助系统故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAutoEmergeSystemFailState = 0x0;	 //自动紧急制动系统故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTReverRadarSystemFailState = 0x0;	 //倒车雷达系统故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTElecGearSystemFailState = 0x0;	 //电机换挡器系统故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftFrontTirePressAlarm = 0x0;//左前胎压报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftFrontTireTempAlarm = 0x0; //左前胎温报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightFrontTirePressAlarm = 0x0;//右前胎压报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightrontTireTempAlarm = 0x0;  //右前胎温报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftRearTirePressAlarm = 0x0;  //左后胎压报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftRearTireTempAlarm = 0x0;	//左后胎温报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightRearTirePressAlarm = 0x0; //右后胎压报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightRearTireTempAlarm = 0x0;  //右后胎温报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTDCDCConverterFaultState = 0x0;	//直流-直流转换器故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTVehControllerFailState = 0x0;		//整车控制器故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainPositRelayCoilFault = 0x0;//主正继电器线圈故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainNegaRelayCoilFault = 0x0; //主负继电器线圈故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.PrefillRelayCoilFault = 0x0;  //预充继电器线圈故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.RechargePositRelayCoilFault = 0x0;//充电正继电器线圈故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.RechargeNegaRelayCoilFault = 0x0; //充电负继电器线圈故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainPositiveRelayFault = 0x0;	   //正主继电器故障
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainNagetiveRelayFault = 0x0;	   //正负继电器故障
	
	//********************驾驶行为特殊事件信号*******************//
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTSROverSpeedAlarmState = 0x0;	//TSR超速报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTSRLimitSpeedState = 0x0;		//TSR限速
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAEBInterventionState = 0x0;	//AEB介入
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTABSInterventionState = 0x0;	//ABS介入
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTASRInterventionState = 0x0;	//ASR介入
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESPInterventionState = 0x0;	//ESP介入
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTDSMAlarmState = 0x0;			//DSM报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTowHandOffDiskState = 0x0;	//双手离开方向盘提示
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTACCState = 0x0;				//ACC状态
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTACCSetSpeedState = 0x0;		//ACC设定速度
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWAlarmState = 0x0;			//FCW报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWState = 0x0;				//FCW状态
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWAlarmAccePedalFallState = 0x0;//FCW报警后加速踏板陡降时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWAlarmFirstBrakeState = 0x0;	//FCW报警后首次刹车时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSLDWState = 0x0;				//LDW状态
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLDWAlarmState = 0x0;			//LDW报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLDWAlarmDireDiskResponseState = 0x0;//LDW报警后方向盘响应时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLKAState = 0x0;				//LKA状态
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLKAInterventionState = 0x0;	//LKA介入
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLKADriverTakeOverPromptState = 0x0;//LKA驾驶员接管提示
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLKADriverResponsState = 0x0;	//LKA驾驶员接管提示后方向盘响应时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLBSDState = 0x0;				//BSD状态
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDLeftSideAlarmState = 0x0;	//BSD左侧报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDRightSideAlarmState = 0x0; //BSD右侧报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDAlarmReftWheelRespState = 0x0;//BSD报警后方向盘响应时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDAlarmFirstBrakeState = 0x0;	//BSD报警后首次刹车时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDAlarmPedalAcceState = 0x0;		//BSD报警后加速踏板开始陡降时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossLeftReportState = 0x0;	//交叉车流预警左侧报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossRightReportState = 0x0;	//交叉车流预警右侧报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossAlarmWhellState = 0x0;	//交叉车流预警报警后方向盘响应时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossAlarmStopState = 0x0;	//交叉车流预警报警后首次刹车时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossAlarmAcceTreadleState = 0x0;//交叉车流预警报警后加速踏板开始陡降时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistLeftAlarmState = 0x0;//变道辅助左侧报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistRightAlarmState = 0x0;//变道辅助右侧报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistDireRepsonState = 0x0;//变道辅助报警后方向盘响应时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistFirstStopState = 0x0;	//变道辅助报警后首次刹车时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistAcceDropState = 0x0;	//变道辅助报警后加速踏板开始陡降时长
}
//TimeOutType不能放在初始化鉴权中，记录超时次数，断开重连时重发数据包
//初始化登陆鉴权数据
#define DEV_NO 10
void CFAWACP::InitAcpTBoxAuthData()
{
#if DEV_NO == 1
	memcpy(p_FAWACPInfo_Handle->Vin, "LFPZ1APCXH5F90106", strlen("LFPZ1APCXH5F90106"));
	memcpy(p_FAWACPInfo_Handle->ICCID, "89860198700303143983", strlen("89860198700303143983"));
	memcpy(p_FAWACPInfo_Handle->IMEI, "861477035453602", strlen("861477035453602"));
	memcpy(p_FAWACPInfo_Handle->TBoxSerialNumber, "D2061812280A431760000000000001", strlen("D2061812280A431760000000000001"));
	memcpy(p_FAWACPInfo_Handle->IVISerialNumber, "000000000000000000000000000001", strlen("000000000000000000000000000001"));
#elif DEV_NO == 2//实车验证
	memcpy(p_FAWACPInfo_Handle->Vin, "LFPZ1APCXH5F90107", strlen("LFPZ1APCXH5F90107"));
	memcpy(p_FAWACPInfo_Handle->ICCID, "89860198700303143868", strlen("89860198700303143868"));
	memcpy(p_FAWACPInfo_Handle->IMEI, "861477034976124", strlen("861477034976124"));
	memcpy(p_FAWACPInfo_Handle->TBoxSerialNumber, "D2061812280A431760000000000002", strlen("D2061812280A431760000000000002"));
	memcpy(p_FAWACPInfo_Handle->IVISerialNumber, "000000000000000000000000000002", strlen("000000000000000000000000000002"));
#elif DEV_NO == 3//送样测试
	memcpy(p_FAWACPInfo_Handle->Vin, "LMGFE3G88D1000601", strlen("LMGFE3G88D1000601"));
	memcpy(p_FAWACPInfo_Handle->ICCID, "89860117750028636521", strlen("89860117750028636521"));
	memcpy(p_FAWACPInfo_Handle->IMEI, "861477035453859", strlen("861477035453859"));
	memcpy(p_FAWACPInfo_Handle->TBoxSerialNumber, "D2061812280A431760000000000003", strlen("D2061812280A431760000000000003"));
	memcpy(p_FAWACPInfo_Handle->IVISerialNumber, "000000000000000000000000000003", strlen("000000000000000000000000000003"));
#elif DEV_NO == 5
	memcpy(p_FAWACPInfo_Handle->Vin, "LFPZ1APCXH5F90109", strlen("LFPZ1APCXH5F90109"));
	memcpy(p_FAWACPInfo_Handle->ICCID, "89860918700300514749", strlen("89860918700300514749"));
	memcpy(p_FAWACPInfo_Handle->IMEI, "861477035992781", strlen("861477035992781"));
	memcpy(p_FAWACPInfo_Handle->TBoxSerialNumber, "D2061812280A431760000000000016", strlen("D2061812280A431760000000000016"));
	memcpy(p_FAWACPInfo_Handle->IVISerialNumber, "000000000000000000000000000016", strlen("000000000000000000000000000016"));
#elif DEV_NO == 6
	memcpy(p_FAWACPInfo_Handle->Vin, "LFPZ1APCXH5F90110", strlen("LFPZ1APCXH5F90110"));
	memcpy(p_FAWACPInfo_Handle->ICCID, "89860918700300514491", strlen("89860918700300514491"));
	memcpy(p_FAWACPInfo_Handle->IMEI, "861477035454936", strlen("861477035454936"));
	memcpy(p_FAWACPInfo_Handle->TBoxSerialNumber, "D2061812280A4317600000000000017", strlen("D2061812280A4317600000000000017"));
	memcpy(p_FAWACPInfo_Handle->IVISerialNumber, "0000000000000000000000000000017", strlen("0000000000000000000000000000017"));
#elif DEV_NO == 7
	memcpy(p_FAWACPInfo_Handle->Vin, "LFPZ1APCXH5F90103", strlen("LFPZ1APCXH5F90103"));
	memcpy(p_FAWACPInfo_Handle->ICCID, "89860918700300514806", strlen("89860918700300514806"));
	memcpy(p_FAWACPInfo_Handle->IMEI, "861477035994803", strlen("861477035994803"));
	memcpy(p_FAWACPInfo_Handle->TBoxSerialNumber, "D2061812280A4317600000000000012", strlen("D2061812280A4317600000000000012"));
	memcpy(p_FAWACPInfo_Handle->IVISerialNumber, "0000000000000000000000000000012", strlen("0000000000000000000000000000012"));
#elif DEV_NO == 8
	memcpy(p_FAWACPInfo_Handle->Vin, "LFPZ1APCXH5F90104", strlen("LFPZ1APCXH5F90104"));
	memcpy(p_FAWACPInfo_Handle->ICCID, "89860918700300514707", strlen("89860918700300514707"));
	memcpy(p_FAWACPInfo_Handle->IMEI, "861477035994118", strlen("861477035994118"));
	memcpy(p_FAWACPInfo_Handle->TBoxSerialNumber, "D2061812280A4317600000000000013", sizeof(p_FAWACPInfo_Handle->TBoxSerialNumber));
	memcpy(p_FAWACPInfo_Handle->IVISerialNumber, "0000000000000000000000000000013", sizeof(p_FAWACPInfo_Handle->IVISerialNumber));
#elif DEV_NO == 9
	memcpy(p_FAWACPInfo_Handle->TBoxSerialNumber, "7905030LPB95XHZ180929B00070001", strlen("7905030LPB95XHZ180929B00070001"));
	memcpy(p_FAWACPInfo_Handle->IVISerialNumber, "KH2A43210000000100000000000000", strlen("KH2A43210000000100000000000000"));
#elif DEV_NO == 10
	memcpy(p_FAWACPInfo_Handle->TBoxSerialNumber, "D2061812280A431760000000020018", strlen("D2061812280A431760000000020018"));
	memcpy(p_FAWACPInfo_Handle->IVISerialNumber, "000000000000000000000000020018", strlen("000000000000000000000000020018"));
#endif
#if 1
	//生产时版本打开
	dataPool->getTboxConfigInfo(VinID, p_FAWACPInfo_Handle->Vin, sizeof(p_FAWACPInfo_Handle->Vin));
	//get ICCID
	memset(p_FAWACPInfo_Handle->ICCID, 0, sizeof(p_FAWACPInfo_Handle->ICCID));
	dataPool->getPara(SIM_ICCID_INFO, p_FAWACPInfo_Handle->ICCID, sizeof(p_FAWACPInfo_Handle->ICCID));
	//get IMEI
	memset(p_FAWACPInfo_Handle->IMEI, 0, sizeof(p_FAWACPInfo_Handle->IMEI));
	dataPool->getPara(IMEI_INFO, p_FAWACPInfo_Handle->IMEI, sizeof(p_FAWACPInfo_Handle->IMEI));
	//get TBoxSerialNumber
	memset(p_FAWACPInfo_Handle->TBoxSerialNumber, 0, sizeof(p_FAWACPInfo_Handle->TBoxSerialNumber));
	dataPool->getPara(TBoxSerialNumber_ID, p_FAWACPInfo_Handle->TBoxSerialNumber, sizeof(p_FAWACPInfo_Handle->TBoxSerialNumber));
	//get IVISerialNumber
	memset(p_FAWACPInfo_Handle->IVISerialNumber, 0, sizeof(p_FAWACPInfo_Handle->IVISerialNumber));
	dataPool->getPara(IVISerialNumber_ID, p_FAWACPInfo_Handle->IVISerialNumber, sizeof(p_FAWACPInfo_Handle->IVISerialNumber));
#endif

	memset(p_FAWACPInfo_Handle->AuthToken, 0, sizeof(p_FAWACPInfo_Handle->AuthToken));
	memset(p_FAWACPInfo_Handle->SKey, 0, sizeof(p_FAWACPInfo_Handle->SKey));

	uint16_t tempLen = 0;
	//【Auth Apply Message】 AcpTBoxAuthApplyMsg_t
	tempLen = 17;
	AcpTBox_AuthApplyMsg.CarManufactureID.Element_LenInfo.Element_Len = 1;
	AcpTBox_AuthApplyMsg.CarManufactureID.CarManufactureID = AcpCarManufactureID_TianJing;
	
	AcpTBox_AuthApplyMsg.TU_VIN.Element_LenInfo.elementLenBit.Identifier = 1;
	AcpTBox_AuthApplyMsg.TU_VIN.Element_LenInfo.elementLenBit.MoreFlag = 0;
	AcpTBox_AuthApplyMsg.TU_VIN.Element_LenInfo.elementLenBit.DataLen = tempLen&0xFF1F;
	memcpy(AcpTBox_AuthApplyMsg.TU_VIN.CarTU_ID, p_FAWACPInfo_Handle->Vin, sizeof(p_FAWACPInfo_Handle->Vin));
	
	//【AKey Message】 AcpTspAkeyMsg_t
	tempLen = 48;
	AcpTsp_AkeyMsg.Len_High.elementLenBit.Identifier = 0;
	AcpTsp_AkeyMsg.Len_High.elementLenBit.MoreFlag = 1;
	AcpTsp_AkeyMsg.Len_High.elementLenBit.DataLen = ((tempLen<<4)>>11)&0xFF1F;
	AcpTsp_AkeyMsg.Len_Low.elementLenBit.MoreFlag = 0;
	AcpTsp_AkeyMsg.Len_Low.elementLenBit.DataLen  = ((tempLen<<9)>>9)&0xFF7F;
	memset(AcpTsp_AkeyMsg.AkeyC_Tsp, 0, sizeof(AcpTsp_AkeyMsg.AkeyC_Tsp));
	memset(AcpTsp_AkeyMsg.Rand1_Tsp, 0, sizeof(AcpTsp_AkeyMsg.Rand1_Tsp));
	
	//TBox发起 【 CT Message 】
	tempLen = 48;
	AcpTBox_CTMsg.Len_High.elementLenBit.Identifier = 0;
	AcpTBox_CTMsg.Len_High.elementLenBit.MoreFlag = 1;
	AcpTBox_CTMsg.Len_High.elementLenBit.DataLen = ((tempLen<<4)>>11)&0xFF1F;
	AcpTBox_CTMsg.Len_Low.elementLenBit.MoreFlag = 0;
	AcpTBox_CTMsg.Len_Low.elementLenBit.DataLen  = ((tempLen<<9)>>9)&0xFF7F;
	memset(AcpTBox_CTMsg.Rand1CT_TBox, 0, sizeof(AcpTBox_CTMsg.Rand1CT_TBox));
	memset(AcpTBox_CTMsg.Rand2_Tbox, 0, sizeof(AcpTBox_CTMsg.Rand2_Tbox));
	
	//【CS Message】 AcpTspCSMsg_t
	tempLen = 33;
	AcpTsp_CSMsg.Len_High.elementLenBit.Identifier = 0;
	AcpTsp_CSMsg.Len_High.elementLenBit.MoreFlag = 1;
	AcpTsp_CSMsg.Len_High.elementLenBit.DataLen = ((tempLen<<4)>>11)&0xFF1F;
	AcpTsp_CSMsg.Len_Low.elementLenBit.MoreFlag = 0;
	AcpTsp_CSMsg.Len_Low.elementLenBit.DataLen  = ((tempLen<<9)>>9)&0xFF7F;
	memset(AcpTsp_CSMsg.Rand2CS_Tsp, 0, sizeof(AcpTsp_CSMsg.Rand2CS_Tsp));
	AcpTsp_CSMsg.RT_Tsp = 0;
	
	//TBox发起【RS Message】
	tempLen = 17;
	AcpTBox_RSMsg.LenInfo.elementLenBit.Identifier = 0;
	AcpTBox_RSMsg.LenInfo.elementLenBit.MoreFlag = 0;
	AcpTBox_RSMsg.LenInfo.elementLenBit.DataLen = tempLen&0xFF1F;
	memset(AcpTBox_RSMsg.Rand3_TBox, 0, sizeof(AcpTBox_RSMsg.Rand3_TBox));
	AcpTBox_RSMsg.RS_Tbox = 0;
	
	//接收TSP【SKey Message】 AcpTspSKeyMsg_t
	tempLen = 64;
	AcpTsp_SKeyMsg.Len_High.elementLenBit.Identifier = 0;
	AcpTsp_SKeyMsg.Len_High.elementLenBit.MoreFlag = 1;
	AcpTsp_SKeyMsg.Len_High.elementLenBit.DataLen = ((tempLen<<4)>>11)&0xFF1F;
	AcpTsp_SKeyMsg.Len_Low.elementLenBit.MoreFlag = 0;
	AcpTsp_SKeyMsg.Len_Low.elementLenBit.DataLen  = ((tempLen<<9)>>9)&0xFF7F;
	memset(AcpTsp_SKeyMsg.SkeyC_Tsp, 0, sizeof(AcpTsp_SKeyMsg.SkeyC_Tsp));
	memset(AcpTsp_SKeyMsg.Rand3CS_Tsp, 0, sizeof(AcpTsp_SKeyMsg.Rand3CS_Tsp));
	
	//TBox发起【Auth Ready Message】
	tempLen = 112;
	AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.Element_Len_High.elementLenBit.Identifier = 1;
	AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.Element_Len_High.elementLenBit.MoreFlag = 1;
	AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.Element_Len_High.elementLenBit.DataLen = ((tempLen<<4)>>11)&0xFF1F;
	AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.Element_Len_Low.elementLenBit.MoreFlag = 0;
	AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.Element_Len_Low.elementLenBit.DataLen  = ((tempLen<<9)>>9)&0xFF7F;
	
	memcpy(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.Vin,p_FAWACPInfo_Handle->Vin,sizeof(p_FAWACPInfo_Handle->Vin));
	memcpy(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.IMEI,p_FAWACPInfo_Handle->IMEI,sizeof(p_FAWACPInfo_Handle->IMEI));
	memcpy(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.ICCID,p_FAWACPInfo_Handle->ICCID,sizeof(p_FAWACPInfo_Handle->ICCID));
	memcpy(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.IVISerialNumber,p_FAWACPInfo_Handle->IVISerialNumber,sizeof(p_FAWACPInfo_Handle->IVISerialNumber));
	memcpy(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.TBoxSerialNumber,p_FAWACPInfo_Handle->TBoxSerialNumber,sizeof(p_FAWACPInfo_Handle->TBoxSerialNumber));
	
	//接收TSP【Auth Ready ACK Message】 AcpTspAuthReadyACKMsg_t
	tempLen = 4;
	AcpTsp_AuthReadyACKMsg.ACKMsg_AuthToken.Element_LenInfo.elementLenBit.Identifier = 0;
	AcpTsp_AuthReadyACKMsg.ACKMsg_AuthToken.Element_LenInfo.elementLenBit.MoreFlag = 0;
	AcpTsp_AuthReadyACKMsg.ACKMsg_AuthToken.Element_LenInfo.elementLenBit.DataLen = tempLen&0xFF1F;
	memset(AcpTsp_AuthReadyACKMsg.ACKMsg_AuthToken.AuthToken, 0, sizeof(AcpTsp_AuthReadyACKMsg.ACKMsg_AuthToken.AuthToken));
}

//接收线程
void *CFAWACP::receiveThread(void *arg)
{
	CFAWACP *pCFAWACP = (CFAWACP*)arg;
	pthread_detach(pthread_self());
	//pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	int length;
	uint8_t *dataBuff = (uint8_t *)malloc(DATA_BUFF_SIZE);
	if(dataBuff == NULL){
		FAWACPLOG("malloc dataBuff error!");
	}

	while(1)
	{
		if(pCFAWACP->m_ConnectedState == true)
		{
			FAWACPLOG("Waiting to receive data:");
			memset(dataBuff, 0, DATA_BUFF_SIZE);
			length = recv(pCFAWACP->m_Socketfd, dataBuff, DATA_BUFF_SIZE, 0);
			FAWACPLOG("Receive data --->length == %d:",length);
			if(length > 0)
			{
				for(uint16_t i = 0; i< length; i++)
					FAWACP_NO("%02x", *(dataBuff + i));
				FAWACP_NO("\n\n");
				
				if(pCFAWACP->checkReceivedData(dataBuff, length) == 0)
				{
					pCFAWACP->Unpack_FAWACP_FrameData(dataBuff+DATA_SOFEOF_SIZE+SIGNATURE_SIZE, length - 2*DATA_SOFEOF_SIZE - SIGNATURE_SIZE);
				}
			}
		    else
			{
				if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
					continue;
				else{
					FAWACPLOG("Receive data length = %d.\n", length);
					pCFAWACP->m_loginState = 0;
					pCFAWACP->m_ConnectedState = false;
					break;
				}
			}
		}
		else
			break;
		//sleep(1);
	}
	if(NULL != dataBuff){
		free(dataBuff);
		dataBuff = NULL;
	}
	pthread_exit(0);
}

uint16_t CFAWACP::checkReceivedData(uint8_t *pData, uint32_t len)
{
	uint8_t *pos = pData;
	uint8_t ProtocolSignature[SIGNATURE_SIZE] = {0};
	//长度判断
	if(len < 2*DATA_SOFEOF_SIZE + SIGNATURE_SIZE)
	{
		FAWACPLOG("Received data Length error:%d\n",len);
		return -1;
	}

	uint32_t Sof = FAWACP_PROTOCOL_SOF;
	if(Sof != htonl(*(uint32_t *)pos))
		return -1;

	uint32_t Eof = FAWACP_PROTOCOL_EOF;
	if(Eof != htonl(*(uint32_t *)(pos+len-DATA_SOFEOF_SIZE)))
		return -1;

	FAWACPLOG("Data SOFEOF check success!\n");
	//签名数据
	uint16_t Len_Payload = len - 2*DATA_SOFEOF_SIZE - SIGNATURE_SIZE;
	PackData_Signature(pos + DATA_SOFEOF_SIZE + SIGNATURE_SIZE, Len_Payload, ProtocolSignature);

	if(memcmp(ProtocolSignature, pos+DATA_SOFEOF_SIZE, SIGNATURE_SIZE) != 0)
	{
		FAWACPLOG("Received data signature error.\n");
		return -1;
	}
	FAWACPLOG("Received data signature check success.\n");

	return 0;
}

//登陆认证逻辑
//登陆服务器认证 <登陆认证过程中出现任何异常则重新开始认证>
int	CFAWACP::sendLoginAuth(uint8_t MsgType, AcpCryptFlag_E CryptFlag)
{
	uint16_t dataLen;
	uint8_t *dataBuff = (uint8_t *)malloc(MALLOC_DATA_LEN);
	if(dataBuff == NULL)
	{
		FAWACPLOG("malloc dataBuff error!");
		return -1;
	}
	memset(dataBuff, 0, sizeof(dataBuff));
	//打包数据
	dataLen = PackFAWACP_FrameData(dataBuff, MALLOC_DATA_LEN, ACPApp_AuthenID, MsgType, CryptFlag);

	if((dataLen = send(m_Socketfd, dataBuff, dataLen, 0)) < 0)
	{
		FAWACPLOG("Send data failed.dataLen:%d\n",dataLen);
		free(dataBuff);
		dataBuff = NULL;
		m_loginState = 0;
		m_ConnectedState = false;
		return -1;
	}
	free(dataBuff);
	dataBuff = NULL;
	return 0;
}
//	TBox认证请求
int	CFAWACP::sendLoginAuthApplyMsg()
{
	sendLoginAuth(MESSAGE_TYPE_START+1, AcpCryptFlag_NULL);
	return 0;
}
//	CT Message
int	CFAWACP::sendLoginCTMsg()
{
//	uint16_t AkeyLen = 0;
//	uint16_t Rand1CTLen;
	//uint8_t i = 0;
	//RootKey解密AKeyC_Tsp->AKey_TBox得到AKey_TBox
	//Decrypt_AES128Data(p_FAWACPInfo_Handle->RootKey, AcpTsp_AkeyMsg.AkeyC_Tsp, sizeof(AcpTsp_AkeyMsg.AkeyC_Tsp), AKey_TBox, AcpCryptFlag_IS);
	//加密芯片解密AkeyC_Tsp得到AKey_TBox
	decryptTbox_Data(AcpTsp_AkeyMsg.AkeyC_Tsp, 1, AKey_TBox);
	//AKey_TBox加密Rand1_Tsp->Rand1CT_TBox
	Encrypt_AES128Data(AKey_TBox, AcpTsp_AkeyMsg.Rand1_Tsp, sizeof(AcpTsp_AkeyMsg.Rand1_Tsp), AcpTBox_CTMsg.Rand1CT_TBox, AcpCryptFlag_IS);
	//生成随机数Rand2_Tbox
	HMACMd5_digest((const char *)AKey_TBox, AcpTBox_CTMsg.Rand1CT_TBox, sizeof(AcpTBox_CTMsg.Rand1CT_TBox), AcpTBox_CTMsg.Rand2_Tbox);

	sendLoginAuth(MESSAGE_TYPE_START+3, AcpCryptFlag_NULL);
	return 0;
}

//	RS Message
int	CFAWACP::sendLoginRSMsg()
{
	uint8_t	Rand2CT_TBox[32];

	//AKey_TBox加密Rand2_Tbox->Rand2CT_TBox 使用Akey加密
	Encrypt_AES128Data(AKey_TBox, AcpTBox_CTMsg.Rand2_Tbox, sizeof(AcpTBox_CTMsg.Rand2_Tbox), Rand2CT_TBox, AcpCryptFlag_IS);

	//比较Rand2CT_TBox与Rand2CS_Tsp->RS_Tbox
	AcpTBox_RSMsg.RS_Tbox = memcmp(Rand2CT_TBox,AcpTsp_CSMsg.Rand2CS_Tsp, sizeof(Rand2CT_TBox));

	if(AcpTBox_RSMsg.RS_Tbox != 0){
		FAWACPLOG("Rand2CT_TBox与Rand2CS_Tsp校验错误！");
		m_loginState = 0;
		return -1;
	}
	//生成随机数Rand3_Tbox 使用RootKey加密
	HMACMd5_digest((const char *)AKey_TBox, Rand2CT_TBox, sizeof(Rand2CT_TBox), AcpTBox_RSMsg.Rand3_TBox);

	sendLoginAuth(MESSAGE_TYPE_START+5, AcpCryptFlag_NULL);
	return 0;
}

//	AuthReadyMsg 7/8
int	CFAWACP::sendLoginAuthReadyMsg()
{
	uint8_t  Rand3CT_TBox[32];
	//AKey_TBox解密SkeyC_Tsp->SKey_TBox 【使用AKey_TBox解密】
	Decrypt_AES128Data(AKey_TBox, AcpTsp_SKeyMsg.SkeyC_Tsp, sizeof(AcpTsp_SKeyMsg.SkeyC_Tsp), SKey_TBox, AcpCryptFlag_IS);
	//SKey_TBox加密Rand3_Tbox->Rand3CT_TBox 【使用SKey_TBox加密】
	Encrypt_AES128Data(SKey_TBox, AcpTBox_RSMsg.Rand3_TBox, sizeof(AcpTBox_RSMsg.Rand3_TBox), Rand3CT_TBox, AcpCryptFlag_IS);
	//比较Rand3CT_TBox与Rand3CS_Tsp->true:OK,false:fail
	if(0 != memcmp(Rand3CT_TBox, AcpTsp_SKeyMsg.Rand3CS_Tsp, sizeof(Rand3CT_TBox)))
	{
		FAWACPLOG("Rand3CT_TBox与Rand3CS_Tsp校验错误！");
		m_loginState = 0;
		return -1;
	}
	//更新skey
	memcpy(p_FAWACPInfo_Handle->SKey, SKey_TBox, sizeof(SKey_TBox));
//使用Skey_Tbox加密车辆数据
	sendLoginAuth(MESSAGE_TYPE_START+7, AcpCryptFlag_IS);
	return 0;
}

//车况数据上报
uint16_t	CFAWACP::timingReportingData(uint8_t MsgType, AcpAppID_E AppID)
{
	if(false == cfawacp->m_bEnableSendDataFlag)
		return -1;
	uint16_t dataLen = 0;
	uint8_t *dataBuff = (uint8_t *)malloc(MALLOC_DATA_LEN);
	if(dataBuff == NULL){
		FAWACPLOG("malloc dataBuff error!");
		return -1;
	}
	memset(dataBuff, 0, MALLOC_DATA_LEN);
	//打包数据
	dataLen = cfawacp->PackFAWACP_FrameData(dataBuff, MALLOC_DATA_LEN, AppID, MsgType, AcpCryptFlag_IS);

	if((dataLen = send(cfawacp->m_Socketfd, dataBuff, dataLen, 0)) < 0)
	{
		FAWACPLOG("Send data failed.dataLen:%d\n",dataLen);
		free(dataBuff);
		dataBuff = NULL;
		cfawacp->m_loginState = 0;
		cfawacp->m_ConnectedState = false;
		return -1;
	}
	FAWACPLOG("Send data ok.dataLen:%d\n",dataLen);
	free(dataBuff);
	dataBuff = NULL;

	return 0;
}


//MCU回调函数发送远程控制应答
void CFAWACP::cb_TspRemoteCtrl()
{
	cfawacp->RespondTspPacket(ACPApp_RemoteCtrlID, 2, AcpCryptFlag_IS, cfawacp->TspctrlSource);
}


//回应TSP命令
int CFAWACP::RespondTspPacket(AcpAppID_E applicationID, uint8_t MsgType, AcpCryptFlag_E CryptFlag, uint8_t TspSourceID)
{
	if((false == cfawacp->m_bEnableSendDataFlag) && (applicationID != ACPApp_UpdateKeyID))
		return -1;

	uint16_t dataLen;
	uint8_t *dataBuff = (uint8_t *)malloc(MALLOC_DATA_LEN);
	if(dataBuff == NULL)
	{
		FAWACPLOG("malloc dataBuff error!");
		return -1;
	}
	memset(dataBuff, 0, MALLOC_DATA_LEN);
	//打包数据
	dataLen = PackFAWACP_FrameData(dataBuff, MALLOC_DATA_LEN, applicationID, MsgType, CryptFlag, TspSourceID);

	if((dataLen = send(m_Socketfd, dataBuff, dataLen, 0)) < 0)
	{
		FAWACPLOG("Send data failed.dataLen:%d\n",dataLen);
		free(dataBuff);
		dataBuff = NULL;
		m_loginState = 0;
		m_ConnectedState = false;
		return -1;
	}
	FAWACPLOG("Send data ok.dataLen:%d\n",dataLen);
	free(dataBuff);
	dataBuff = NULL;

	return 0;
}

/**************************     打包数据     **************************************/
//	签名
void CFAWACP::PackData_Signature(uint8_t *dataBuff, uint16_t len, uint8_t *OutBuff)
{
	uint8_t *pos = dataBuff;
	//SHA-1算法签名
	m_SHA1OpData.Encrpy_SHA1Data(dataBuff, len, OutBuff);
}
//打包头数据
void CFAWACP::Packet_AcpHeader(AcpAppHeader_t& Header, AcpAppID_E AppID, uint8_t MsgType, AcpCryptFlag_E CryptFlag, uint16_t MsgLength)
{
	//服务标识号
	Header.Reserved_ID = 0;
	Header.PrivateFlag = 1;
	Header.Acp_AppID 	= AppID;
	//服务消息标识(测试+加密+服务)
	Header.Reserved_T = 0;
	Header.TestFlag = VERSION_FLAG;
	Header.CryptFlag = CryptFlag;
	Header.MsgType	 =	MsgType;
	//消息控制位
	Header.VersionFlag	= 1;
	Header.Version		= 0;
	Header.MsgCtrlFlag	= 2;
	//长度定义
	Header.MsgLength	=	MsgLength;
}
//添加头数据
uint16_t CFAWACP::AddData_AcpHeader(uint8_t *dataBuff, AcpAppHeader_t Header)
{
	uint8_t *pos = dataBuff;
	
	/*服务标识号*/
	*pos++ = ((Header.Reserved_ID << 7) & 0x80) |		//保留位
			 ((Header.PrivateFlag << 6) & 0x40)|//私有标识
			 (Header.Acp_AppID & 0x3F);  //Acp服务标识号
	/*服务消息标识(测试+加密+服务)*/
	*pos++ = ((Header.Reserved_T << 7) & 0x80) |		//保留位
			 ((Header.TestFlag << 6) & 0x40) |	//测试报文标识
			 ((Header.CryptFlag << 5) & 0x20)|	//加密标识
			 (Header.MsgType & 0x1F);	//服务消息类型
	/*消息控制位*/
	*pos++ = ((Header.VersionFlag << 7) & 0x80) |		//启用版本号标识(0:启用,1:不用)
			 ((Header.Version << 4) & 0x70) |	//版本号(set 0-7)
			 (Header.MsgCtrlFlag & 0x0F);//消息控制掩码(0000/0010)
	/*长度定义*/
	*pos++ = (Header.MsgLength >> 8) & 0xFF;	//高8位
	*pos++ = Header.MsgLength & 0xFF;			//低8位
	
	return (uint16_t)(pos-dataBuff);
}

int CFAWACP::insertlink(uint8_t applicationID, uint8_t MsgType, uint8_t TspSourceID)
{
	time_t BuffTime = 0;
	LinkTimer_t *p_new;
	//该链表插入的节点不能在此处释放，在接收到回馈后删除节点释放，或判定超时后删除节点释放。
	p_new = (LinkTimer_t *)malloc(sizeof(LinkTimer_t));
	if(NULL == p_new)
	{
		FAWACPLOG("malloc ElementBuff error!");
		return -1;
	}
	memset(p_new, 0, sizeof(LinkTimer_t));
	
	p_new->AcpAppID = applicationID;
	p_new->MsgType = MsgType;
	p_new->TspSoure = TspSourceID;

	time(&BuffTime);
	p_new->time = BuffTime;

	insert(&Headerlink, p_new);
	return 0;
}
//数据体Payload打包
uint32_t CFAWACP::PackFAWACP_PayloadData(uint8_t *dataBuff, uint16_t bufSize, AcpAppID_E applicationID, uint8_t MsgType, AcpCryptFlag_E CryptFlag, uint8_t TspSourceID)
{
	uint32_t dataLen = 0;
	uint8_t *pos = dataBuff;
	//Element
	//1)打包数据体	2)加密 3)计算加密后的长度
	uint8_t *ElementBuff = (uint8_t *)malloc(MALLOC_DATA_LEN);
	if(ElementBuff == NULL)
	{
		FAWACPLOG("malloc ElementBuff error!");
		return -1;
	}
	memset(ElementBuff, 0, sizeof(ElementBuff));

FAWACPLOG("PackFAWACP_PayloadData:AppID = %d, MsgID = %d, CryptFlag = %d \n",applicationID, MsgType, CryptFlag );
	
//PayloadBuff 执行了加密后的数据
	switch (applicationID)
	{
		case ACPApp_AuthenID://连接认证
			switch (MsgType)
			{
				case 1:			//TBox Auth Apply Message
					dataLen = AuthApplyDataDeal(ElementBuff);
					break;
				case 3:			//CT Message
					//时间戳处理
					TimeDeal(ElementBuff);
					dataLen = CTMsgDataDeal(ElementBuff + DATA_TIMESTAMP_SIZE) + DATA_TIMESTAMP_SIZE;
					break;
				case 5:			//RS Message
					//时间戳处理
					TimeDeal(ElementBuff);
					dataLen = RSMsgDataDeal(ElementBuff + DATA_TIMESTAMP_SIZE) + DATA_TIMESTAMP_SIZE;
					break;
				case 7:			//Auth Ready Message
					//时间戳处理
					TimeDeal(ElementBuff);
					dataLen = AuthReadyMsgDataDeal(ElementBuff + DATA_TIMESTAMP_SIZE) + DATA_TIMESTAMP_SIZE;
					break;
				default:
					break;
			}
			break;
		case ACPApp_HeartBeatID://连接保持(心跳)
			//时间戳
			TimeDeal(ElementBuff);
			AuthTokenDeal(ElementBuff + DATA_TIMESTAMP_SIZE);
			dataLen += DATA_TIMESTAMP_SIZE+DATA_AUTHTOKEN_SIZE;
			break;
		case ACPApp_RemoteCtrlID://远程控制
			//时间戳
			TimeDeal(ElementBuff);
			AuthTokenDeal(ElementBuff + DATA_TIMESTAMP_SIZE);
			SourceDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, TspSourceID);
			ErrorCodeDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE, mcuUart::m_mcuUart->RCtrlErrorCode);
			//错误码
			dataLen = RemoteCtrlCommandDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE);
			dataLen += DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE;
			TimeOutType.RemoteCtrlTspSource = TspSourceID;
			pthread_mutex_lock(&mutex);
			insertlink( ACPApp_RemoteCtrlID, MsgType, TspSourceID);
			pthread_mutex_unlock(&mutex);
			break;
		case ACPApp_QueryVehicleCondID://车况查询
			//时间戳
			TimeDeal(ElementBuff);
			AuthTokenDeal(ElementBuff + DATA_TIMESTAMP_SIZE);
			SourceDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, TspSourceID);
			ErrorCodeDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE, 0);
			//全部信号数据
			dataLen = ReportVehicleCondDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE, 0);
			dataLen += DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE;
			TimeOutType.VehQueCondSource = TspSourceID;
			pthread_mutex_lock(&mutex);
			insertlink( ACPApp_QueryVehicleCondID, MsgType, TspSourceID);
			pthread_mutex_unlock(&mutex);
			break;
		case ACPApp_RemoteConfigID://远程配置
			//时间戳
			TimeDeal(ElementBuff);
			AuthTokenDeal(ElementBuff + DATA_TIMESTAMP_SIZE);
			SourceDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, TspSourceID);
			ErrorCodeDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE, 0);
			dataLen = RemoteConfigCommandDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE);
			dataLen += DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE;
			TimeOutType.RemoteCpnfigSource = TspSourceID;
			pthread_mutex_lock(&mutex);
			insertlink( ACPApp_RemoteConfigID, MsgType, TspSourceID);
			pthread_mutex_unlock(&mutex);
			break;
		case ACPApp_UpdateKeyID:
			switch(MsgType)
			{
				case 1://Rootkey更换-请求
					TimeDeal(ElementBuff);
					AuthTokenDeal(ElementBuff + DATA_TIMESTAMP_SIZE);
					dataLen += DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE;
					break;
				case 3://Rootkey更换-执行
					//时间戳
					TimeDeal(ElementBuff);
					AuthTokenDeal(ElementBuff + DATA_TIMESTAMP_SIZE);
					SourceDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, TspSourceID);
					ErrorCodeDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE, 0);
					//RootKeyData处理
					dataLen = RootKeyDataDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE);
					dataLen += DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE;
					TimeOutType.UpdateRootkeySource = TspSourceID;
					pthread_mutex_lock(&mutex);
					insertlink( ACPApp_UpdateKeyID, MsgType, TspSourceID);
					pthread_mutex_unlock(&mutex);
					break;
				default:
					break;
			}
			break;
		case ACPApp_RemoteUpgradeID://远程升级
			//时间戳
			TimeDeal(ElementBuff);
			AuthTokenDeal(ElementBuff + DATA_TIMESTAMP_SIZE);
			SourceDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, TspSourceID);
			ErrorCodeDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE, 0);
			switch(MsgType)
			{
				case 2:
					dataLen = RemoteUpgrade_ResponseDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE);
					break;
				case 3:
					dataLen = RemoteUpgrade_DownloadStartDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE);
					break;
				case 4:
					dataLen = RemoteUpgrade_DownloadResultDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE);
					break;
				case 5:
					dataLen = RemoteUpgrade_UpdateStartDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE);
					break;
				case 6:
					dataLen = RemoteUpgrade_UpdateResultDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE);
					break;
				default:
					break;
			}
			dataLen += DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE;
			break;
		case ACPApp_EmergencyDataID://紧急救援数据上报
			//时间戳
			TimeDeal(ElementBuff);
			AuthTokenDeal(ElementBuff + DATA_TIMESTAMP_SIZE);
			//全部信号数据
			dataLen = ReportVehicleCondDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, 0);
			dataLen += DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE;
			break;
		case ACPApp_VehicleCondUploadID://车况数据上报 1)事件触发上报 2)普通车况10S定时上报 3) 位置和车速高频5s上报
			//时间戳
			TimeDeal(ElementBuff);
			AuthTokenDeal(ElementBuff + DATA_TIMESTAMP_SIZE);
			//只上报位置和车速定时信息,5s/
			if(1 == MsgType){
				dataLen = ReportGPSSpeedDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE);
			}//低频上报，所有车况数据
			else if(2 == MsgType){
				dataLen = ReportVehicleCondDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, 0);
			}//上报车况信息和故障事件(不包括历史信息)
			else if(18 == MsgType){
				dataLen = ReportVehicleCondDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, 1);
			}//上报车况信息和驾驶行为分析特殊事件(不包括历史信息)
			else if(20 == MsgType){
				dataLen = ReportVehicleCondDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, 2);
			}//上报车况信息不包括历史车速(不包括历史信息)
			else{
				dataLen = ReportVehicleCondDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, 3);
			}
			dataLen += DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE;
			break;
		case ACPApp_GPSID: 	//车辆定位
			//时间戳
			TimeDeal(ElementBuff);
			AuthTokenDeal(ElementBuff + DATA_TIMESTAMP_SIZE);
			SourceDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, TspSourceID);
			ErrorCodeDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE, 0);
			//GPS位置数据上报
			dataLen = VehicleGPSDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE);
			dataLen += DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE;
			TimeOutType.VehGPSSource = TspSourceID;
			pthread_mutex_lock(&mutex);
			insertlink( ACPApp_GPSID, MsgType, TspSourceID);
			pthread_mutex_unlock(&mutex);
			break;
		case ACPApp_VehicleAlarmID://车辆告警
			//时间戳
			TimeDeal(ElementBuff);
			AuthTokenDeal(ElementBuff + DATA_TIMESTAMP_SIZE);
			//全部信号数据
			dataLen = ReportVehicleCondDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, 0);
			dataLen += DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE;
			break;
		case ACPApp_RemoteDiagnosticID://远程诊断
			//时间戳
			TimeDeal(ElementBuff);
			AuthTokenDeal(ElementBuff + DATA_TIMESTAMP_SIZE);
			SourceDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE, TspSourceID);
			ErrorCodeDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE, 0);
			dataLen = RemoteDiagnosticDeal(ElementBuff + DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE);
			dataLen += DATA_TIMESTAMP_SIZE + DATA_AUTHTOKEN_SIZE + DATA_SOURCE_SIZE + DATA_ERRORCODE_SIZE;
			TimeOutType.RemoteDiagnoSource = TspSourceID;
			pthread_mutex_lock(&mutex);
			insertlink( ACPApp_RemoteDiagnosticID, MsgType, TspSourceID);
			pthread_mutex_unlock(&mutex);
			break;
		default:
			break;
	}
	//2)加密处理
	uint16_t Encrypt_len = 0;
	uint8_t *EncryptBuff = (uint8_t *)malloc(MALLOC_DATA_LEN);
	if(EncryptBuff == NULL){
		FAWACPLOG("malloc EncryptBuff error!");
		return -1;
	}

	FAWACPLOG("before Encrypt PayloadData length==%d\n",dataLen);
	for(uint16_t i = 0; i < dataLen; i++)
		FAWACP_NO("%02x ", *(ElementBuff + i));
	FAWACP_NO("\n\n");
	
	Encrypt_len = Encrypt_AES128Data(p_FAWACPInfo_Handle->SKey, ElementBuff, dataLen, EncryptBuff, CryptFlag);
	
	//加入数据头
	AcpAppHeader_t Header;
	memset(&Header, 0, sizeof(AcpAppHeader_t));
	Packet_AcpHeader(Header, applicationID, MsgType, CryptFlag, DATA_HEADER_SIZE+Encrypt_len);
	AddData_AcpHeader(pos, Header);
	pos += DATA_HEADER_SIZE;
	//加入数据体
	memcpy(pos, EncryptBuff, Encrypt_len);
	pos += Encrypt_len;

	if(NULL	!= EncryptBuff){
	free(EncryptBuff);
	EncryptBuff = NULL;
	}
	if(NULL != ElementBuff){
	free(ElementBuff);
	ElementBuff = NULL;
	}
	return (uint16_t)(pos-dataBuff);
}


//整数据包
uint16_t CFAWACP::PackFAWACP_FrameData(uint8_t *dataBuff, uint16_t bufSize, AcpAppID_E applicationID, uint8_t MsgType, AcpCryptFlag_E CryptFlag, uint8_t TspSourceID)
{
	uint16_t PayloadLen;
	uint8_t *pos = dataBuff;
// Packet SOF
	uint32_t Sof = FAWACP_PROTOCOL_SOF;
	SOFEOFDeal(pos, Sof);
	pos += 	DATA_SOFEOF_SIZE;
//Packet PAYLOAD
	uint8_t *PayloadBuff = (uint8_t *)malloc(MALLOC_DATA_LEN);
	if(PayloadBuff == NULL){
		FAWACPLOG("malloc PayloadBuff error!");
		return -1;
	}
	memset(PayloadBuff, 0, sizeof(PayloadBuff));
	//执行了加密后的数据
	PayloadLen = PackFAWACP_PayloadData(PayloadBuff, MALLOC_DATA_LEN, applicationID, MsgType, CryptFlag, TspSourceID);
// signature
	uint8_t *SignatureBuff = (uint8_t *)malloc(SIGNATURE_SIZE);
	if(SignatureBuff == NULL){
		FAWACPLOG("malloc SignatureBuff error!");
		return -1;
	}
	memset(SignatureBuff, 0, SIGNATURE_SIZE);
	PackData_Signature(PayloadBuff, PayloadLen, SignatureBuff);
	memcpy(pos, SignatureBuff, SIGNATURE_SIZE);
	pos += SIGNATURE_SIZE;
// Payload
	memcpy(pos, PayloadBuff, PayloadLen);
	pos += PayloadLen;
// Packet EOF
	uint32_t Eof = FAWACP_PROTOCOL_EOF;
	SOFEOFDeal(pos, Eof);
	pos += DATA_SOFEOF_SIZE;

	if(NULL != SignatureBuff){
	free(SignatureBuff);
	SignatureBuff = NULL;
	}
	if(NULL != PayloadBuff){
	free(PayloadBuff);
	PayloadBuff = NULL;
	}

	return (uint16_t)(pos-dataBuff);
}


/**************************    解析数据包     **************************************/
//解析数据头
uint16_t	CFAWACP::Unpacket_AcpHeader(AcpAppHeader_t& Header, uint8_t *pData, int32_t DataLen)
{
	uint8_t *pos = pData;
	Header.Reserved_ID = (*pos >> 7) & 0x01;
	Header.PrivateFlag = (*pos >> 6) & 0x01;
	Header.Acp_AppID 	 = *pos++ & 0x3F;

	Header.Reserved_T = (*pos >> 7) & 0x01;
	Header.TestFlag = (*pos >> 6) & 0x01;
	Header.CryptFlag = (*pos >> 5) & 0x01;
	Header.MsgType = *pos++ & 0x1F;

	Header.VersionFlag = (*pos >> 7) & 0x01;
	Header.Version = (*pos >> 4) & 0x07;
	Header.MsgCtrlFlag = *pos++ & 0x0F;

	Header.MsgLength = ((pos[0] << 8) & 0xFF00) | (pos[1] & 0x00FF);
	pos += 2;

	return 0;
}
//数据长度处理
uint16_t CFAWACP::unpackDataLenDeal(uint8_t *pTemp, uint16_t &DataLen)
{
	uint8_t *pos = pTemp;
	uint flag = (*pos >> 5) & 0x01;
	if(1 == flag){
		DataLen = ((pos[0] << 7) & 0x0F80) | (pos[1] & 0x7F);
		pos += 2;
	}else{
		DataLen = *pos++ & 0x1F;
	}
	return uint16_t(pos - pTemp);
}
//解析协议数据
uint16_t CFAWACP::Unpack_FAWACP_FrameData(uint8_t *pData, int32_t DataLen)
{
	uint8_t *pos = pData;
	uint8_t *PayloadBuff;
	//数据头
	AcpAppHeader_t Header;
	Unpacket_AcpHeader(Header, pos, DATA_HEADER_SIZE);

	uint8_t	applicationID = Header.Acp_AppID;
	int32_t payload_size = Header.MsgLength - DATA_HEADER_SIZE;
	uint8_t MsgType = Header.MsgType;
	FAWACPLOG("MsgLength == %d",Header.MsgLength);
	FAWACPLOG("Acp_AppID == %d",Header.Acp_AppID);
	FAWACPLOG("MsgType == %d",Header.MsgType);
	FAWACPLOG("CryptFlag == %d",Header.CryptFlag);

	//服务器唤醒系统
	if(tboxInfo.operateionStatus.isGoToSleep == 0 && applicationID != ACPApp_AuthenID)
	{
		tboxInfo.operateionStatus.isGoToSleep = 1;
		tboxInfo.operateionStatus.wakeupSource = 2;
		lowPowerMode(1, 1, 0);
		modem_ri_notify_mcu();
		sleep(2);
	}

	//Payload 加密前长度判断
	if(DataLen != Header.MsgLength){
		FAWACPLOG("MsgLength error!\n");
		return -1;
	}
	//数据体,解密->数据信息	
	//1)解密处理
	uint16_t Decrypt_len;
	PayloadBuff = (uint8_t *)malloc(MALLOC_DATA_LEN);
	if(NULL == PayloadBuff){
		FAWACPLOG("malloc PayloadBuff error!");
		return -1;
	}

	Decrypt_len = Decrypt_AES128Data(p_FAWACPInfo_Handle->SKey, (pos+DATA_HEADER_SIZE), payload_size, PayloadBuff, (AcpCryptFlag_E)Header.CryptFlag);
	payload_size = Decrypt_len;

	// Packet PAYLOAD
	FAWACPLOG("unpack CFAWACP FrameData applicationID:%02x", applicationID);

	//当处于登陆状态时，不接收除登陆包之外的任何数据报文
	if(m_loginState != 2 && applicationID != ACPApp_AuthenID)
		return -1;

	switch (applicationID)
	{
		case ACPApp_AuthenID://连接认证
			UnpackData_AcpLoginAuthen(PayloadBuff, payload_size, MsgType);
			break;
		case ACPApp_HeartBeatID://连接保持(心跳)
			break;
		case ACPApp_RemoteCtrlID://远程控制
			UnpackData_AcpRemoteCtrl(PayloadBuff, payload_size, MsgType);
			break;
		case ACPApp_QueryVehicleCondID://车况查询
			UnpackData_AcpQueryVehicleCond(PayloadBuff, payload_size, applicationID, MsgType);
			break;
		case ACPApp_RemoteConfigID://远程配置
			UnpackData_AcpRemoteConfig(PayloadBuff, payload_size, MsgType);
			break;
		case ACPApp_UpdateKeyID:	//RootKey更换
			UnpackData_AcpUpdateRootKey(PayloadBuff, payload_size, MsgType);
			break;
		case ACPApp_RemoteUpgradeID://远程升级
			UnpackData_AcpRemoteUpgrade(PayloadBuff, payload_size, MsgType);
			break;
		case ACPApp_EmergencyDataID://紧急救援数据上报
			break;
		case ACPApp_VehicleCondUploadID://车况数据上报
			break;
		case ACPApp_GPSID: 	//车辆定位
			UnpackData_AcpQueryVehicleCond(PayloadBuff, payload_size, applicationID, MsgType);
			break;
		case ACPApp_VehicleAlarmID://车辆告警
			break;
		case ACPApp_RemoteDiagnosticID://远程诊断
			UnpackData_AcpRemoteDiagnostic(PayloadBuff, payload_size, MsgType);
			break;
		default:
			break;
	}
	if(NULL != PayloadBuff){
	free(PayloadBuff);
	PayloadBuff = NULL;
	}
	return 0;
} 

/***********************************************************************************
**基础数据打包处理
************************************************************************************/

//处理AuthApply数据体
uint16_t CFAWACP::AuthApplyDataDeal(uint8_t *pTemp)
{
	uint8_t *pos = pTemp;
	*pos++ = AcpTBox_AuthApplyMsg.CarManufactureID.Element_LenInfo.Element_Len;
	*pos++ = AcpTBox_AuthApplyMsg.CarManufactureID.CarManufactureID;
	*pos++ = AcpTBox_AuthApplyMsg.TU_VIN.Element_LenInfo.Element_Len;
	memcpy(pos, AcpTBox_AuthApplyMsg.TU_VIN.CarTU_ID, sizeof(AcpTBox_AuthApplyMsg.TU_VIN.CarTU_ID));
	pos += sizeof(AcpTBox_AuthApplyMsg.TU_VIN.CarTU_ID);

	return (uint16_t)(pos-pTemp);
}
//处理CTMsg数据体
uint16_t CFAWACP::CTMsgDataDeal(uint8_t *pTemp)
{
	uint8_t *pos = pTemp;
	
	*pos++ = AcpTBox_CTMsg.Len_High.Element_Len;
	*pos++ = AcpTBox_CTMsg.Len_Low.Element_Len_Extend;

	memcpy(pos, AcpTBox_CTMsg.Rand1CT_TBox, sizeof(AcpTBox_CTMsg.Rand1CT_TBox));
	pos += sizeof(AcpTBox_CTMsg.Rand1CT_TBox);
	memcpy(pos, AcpTBox_CTMsg.Rand2_Tbox, sizeof(AcpTBox_CTMsg.Rand2_Tbox));
	pos += sizeof(AcpTBox_CTMsg.Rand2_Tbox);
	return (uint16_t)(pos-pTemp);
}
//处理RSMsg数据体
uint16_t CFAWACP::RSMsgDataDeal(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	
	*pos++ = AcpTBox_RSMsg.LenInfo.Element_Len;

	memcpy(pos, AcpTBox_RSMsg.Rand3_TBox, sizeof(AcpTBox_RSMsg.Rand3_TBox));
	pos += sizeof(AcpTBox_RSMsg.Rand3_TBox);
	*pos++ = AcpTBox_RSMsg.RS_Tbox;

	return (uint16_t)(pos-pTemp);
}
//处理AuthReady数据体
uint16_t CFAWACP::AuthReadyMsgDataDeal(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	
	*pos++ = AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.Element_Len_High.Element_Len;
	*pos++ = AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.Element_Len_Low.Element_Len_Extend;
	memcpy(pos,AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.Vin,sizeof(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.Vin));
	pos += sizeof(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.Vin);
	memcpy(pos,AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.IMEI,sizeof(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.IMEI));
	pos += sizeof(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.IMEI);
	memcpy(pos,AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.ICCID,sizeof(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.ICCID));
	pos += sizeof(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.ICCID);
	memcpy(pos,AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.IVISerialNumber,sizeof(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.IVISerialNumber));
	pos += sizeof(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.IVISerialNumber);
	memcpy(pos,AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.TBoxSerialNumber,sizeof(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.TBoxSerialNumber));
	pos += sizeof(AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.TBoxSerialNumber);
	//*pos ++ = AcpTBox_AuthReadyMsg.AuthReadyMsg_VehicleDescriptor.RawData & 0xFF;

	return (uint16_t)(pos-pTemp);
}
// 帧头和尾的处理
void CFAWACP::SOFEOFDeal(uint8_t* pTemp, uint32_t nTempNum)
{
	*pTemp++ = (nTempNum >> 24)&0xff;
	*pTemp++ = (nTempNum >> 16)&0xff;
	*pTemp++ = (nTempNum >> 8)&0xff;
	*pTemp++ = (nTempNum >> 0)&0xff;
}
//数据长度处理
uint16_t CFAWACP::addDataLen(uint8_t *pTemp, uint16_t DataLen, uint8_t Identifier, uint8_t flag)
{
	uint8_t *pos = pTemp;
	ElementLen_t ElementLen;
	if(1 == flag){
		ElementLen.Element_Len_High.elementLenBit.Identifier = Identifier;
		ElementLen.Element_Len_High.elementLenBit.MoreFlag = flag;
		ElementLen.Element_Len_High.elementLenBit.DataLen = ((DataLen << 4) >> 11) & 0xFF1F;

		ElementLen.Element_Len_Low.elementLenBit.MoreFlag = 0;
		ElementLen.Element_Len_Low.elementLenBit.DataLen = ((DataLen << 9) >> 9) & 0xFF7F;

		*pos++ = ElementLen.Element_Len_High.Element_Len;
		*pos++ = ElementLen.Element_Len_Low.Element_Len_Extend;
	}else{
		ElementLen.Element_Len_High.elementLenBit.Identifier = Identifier;
		ElementLen.Element_Len_High.elementLenBit.MoreFlag = flag;
		ElementLen.Element_Len_High.elementLenBit.DataLen = DataLen;

		*pos++ = ElementLen.Element_Len_High.Element_Len;
	}

	return uint16_t(pos - pTemp);
}

//时间戳处理timestamp
void CFAWACP::TimeDeal(uint8_t* pTemp, uint8_t isFlagLen)
{
	AcpTimeStamp_t  AcpTimeStamp;  //TimeStamp时间戳
	memset(&AcpTimeStamp, 0, sizeof(AcpTimeStamp));
	if(isFlagLen == 1)
	{
		AcpTimeStamp.Element_LenInfo.Element_Len = 6;
		*pTemp++ = AcpTimeStamp.Element_LenInfo.Element_Len;
	}		
	TimeStampPart(pTemp, &(AcpTimeStamp.TimeStampInfo));
}

void CFAWACP::TimeStampPart(uint8_t* pTemp, TimeStamp_t *pTimeStamp)
{
	if(NULL != pTemp && NULL != pTimeStamp)
	{
		struct tm *p_tm = NULL; //时间的处理
		time_t tmp_time;
		tmp_time = time(NULL);
		p_tm = gmtime(&tmp_time);
		struct timeval Time;
		//Get时间的月份为【0-11】上报数据时间时需要处理+1
		gettimeofday(&Time, NULL);//获取时间距离微秒

		if(m_loginState == 1 && p_FAWACPInfo_Handle->VehicleCondData.GPSData.GPSState == 0){
			pTimeStamp->Year = m_tspTimestamp.Year;
			pTimeStamp->Month = m_tspTimestamp.Month;
			pTimeStamp->Day = m_tspTimestamp.Day;
			pTimeStamp->Hour = m_tspTimestamp.Hour;
			pTimeStamp->Minutes = m_tspTimestamp.Minutes;
			pTimeStamp->Seconds = m_tspTimestamp.Seconds;
			pTimeStamp->msSeconds = m_tspTimestamp.msSeconds;
		}else{
			if(tboxInfo.operateionStatus.isGoToSleep == 0){
				pTimeStamp->Year = p_tm->tm_year - 90;
				pTimeStamp->Month = p_tm->tm_mon + 1;
				pTimeStamp->Day = p_tm->tm_mday;
				pTimeStamp->Hour = p_tm->tm_hour;
				pTimeStamp->Minutes = p_tm->tm_min;
				pTimeStamp->Seconds = p_tm->tm_sec;
				pTimeStamp->msSeconds = Time.tv_usec / 1000; //毫秒
			}else{
				pTimeStamp->Year = p_FAWACPInfo_Handle->VehicleCondData.GPSData.year;
				pTimeStamp->Month = p_FAWACPInfo_Handle->VehicleCondData.GPSData.month;
				pTimeStamp->Day = p_FAWACPInfo_Handle->VehicleCondData.GPSData.day;
				pTimeStamp->Hour = p_FAWACPInfo_Handle->VehicleCondData.GPSData.hour;
				pTimeStamp->Minutes = p_FAWACPInfo_Handle->VehicleCondData.GPSData.minute;
				pTimeStamp->Seconds = p_FAWACPInfo_Handle->VehicleCondData.GPSData.second;
				pTimeStamp->msSeconds = 0;
			}
		}
		*pTemp++ = ((pTimeStamp->Year)<<2)|(((pTimeStamp->Month)>>2)&0x03);//时间第一字节
		*pTemp++ = (((pTimeStamp->Month)<<6)&0xc0)|((pTimeStamp->Day)<<1)|(((pTimeStamp->Hour)>>4)&0x01);//时间第二字节
		*pTemp++ = (((pTimeStamp->Hour)<<4)&0xf0)|(((pTimeStamp->Minutes)>>2)&0x0f);//时间第二字节
		*pTemp++ = (((pTimeStamp->Minutes)<<6)&0xc0)|(pTimeStamp->Seconds);//时间第四字节
		*pTemp++ = (((pTimeStamp->msSeconds)>>8)&0xff);
		*pTemp++ = (((pTimeStamp->msSeconds)>>0)&0xff);
	}
}


//车辆身份令牌打包AuthToken
void CFAWACP::AuthTokenDeal(uint8_t* pTemp)
{
	uint16_t AuthTokenlen = 4;
	
	*pTemp++ = AuthTokenlen & 0xFF;
	memcpy(pTemp, AcpTsp_AuthReadyACKMsg.ACKMsg_AuthToken.AuthToken, sizeof(AcpTsp_AuthReadyACKMsg.ACKMsg_AuthToken.AuthToken));
}
//指令源信息打包Source
void CFAWACP::SourceDeal(uint8_t* pTemp, uint8_t TspSourceID)
{
	AcpSourceID_t AcpSourceID;
	memset(&AcpSourceID, 0, sizeof(AcpSourceID));
	AcpSourceID.Element_LenInfo.Element_Len = 1;
	AcpSourceID.SourceID = TspSourceID;
	
	*pTemp++ = AcpSourceID.Element_LenInfo.Element_Len;
	*pTemp++ = AcpSourceID.SourceID;
}
//错误信息处理打包
void CFAWACP::ErrorCodeDeal(uint8_t* pTemp, uint8_t nErrorCode)
{
	AcpErrorCode_t  AcpErrorCode;			//ErrorCode
	memset(&AcpErrorCode, 0, sizeof(AcpErrorCode));
	AcpErrorCode.Element_LenInfo.Element_Len = 1;
	AcpErrorCode.ErrorCode = nErrorCode; 
	
	*pTemp++ = AcpErrorCode.Element_LenInfo.Element_Len;
	*pTemp++ = AcpErrorCode.ErrorCode;
}
/***************************信号值****************************/
//采集信号编码处理
uint16_t CFAWACP::SignCodeDeal(uint8_t* pTemp, uint8_t inValid, uint16_t SignCde)
{
	uint8_t *pos = pTemp;
	
	SignCodeBit_U SignCode;
	memset(&SignCode, 0, sizeof(SignCode));

	SignCode.SignCodeBit = inValid;
	SignCode.SignCodeBit = SignCode.SignCodeBit << 13 | SignCde;
	//采集信号编码
	*pos++ = (SignCode.SignCodeBit >> 8) & 0xFF;
	*pos++ = (SignCode.SignCodeBit >> 0) & 0xFF;
	
	return uint16_t(pos - pTemp);
}
//打包GPS数据
uint16_t CFAWACP::add_GPSData(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_GPSID);
	
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.GPSState <<5 ) & 0x60)| ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitude >> 24 ) & 0x1F);//从高地址开始存的
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitude >>16) & 0xFF);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitude >> 8) & 0xFF);
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitude & 0xFF);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitudeState << 6 ) & 0xC0) | ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitude >> 23 ) & 0x3F);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitude >> 15) & 0xFF);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitude >> 7) & 0xFF);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitude << 1) & 0xFE) | ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitudeState >> 1) & 0X01);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitudeState << 7) & 0X80) | ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.altitude >> 7) & 0X7F);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.altitude << 1) & 0xFE) | ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.year >> 5) & 0X01);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.year << 3) & 0XF8) | ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.month >> 1) & 0X07);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.month << 7) & 0X80) | ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.day << 2) & 0X7C) | ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.hour >> 3) & 0X03);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.hour << 5) & 0XE0) | ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.minute >> 1) & 0X1F);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.minute << 7) & 0X80) | ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.second << 1) & 0X7E) | ((p_FAWACPInfo_Handle->VehicleCondData.GPSData.degree >> 8) & 0X01);
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.GPSData.degree & 0XFF);

	return uint16_t(pos - pTemp);
}

//打包剩余油量数据
uint16_t CFAWACP::addRemainedOil(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_RemainedOilID);
	
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.RemainedOil.RemainedOilGrade;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.RemainedOil.RemainedOilValue;

	return uint16_t(pos - pTemp);
}

//打包总里程数据
uint16_t CFAWACP::addOdometer(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_OdometerID);

	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.Odometer >> 24) & 0XFF);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.Odometer >> 16) & 0xFF);
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.Odometer >> 8) & 0xFF);
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.Odometer & 0xFF);

	return uint16_t(pos - pTemp);
}

//打包蓄电池电量数据
uint16_t CFAWACP::addBattery(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_BatteryID);

	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.Battery >> 8) & 0xFF);
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.Battery & 0xFF);

	return uint16_t(pos - pTemp);
}

//打包实时车速数据
uint16_t CFAWACP::addCurrentSpeed(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_CurrentSpeedID);

	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.CurrentSpeed >> 8) & 0xFF);
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.CurrentSpeed & 0xFF);

	return uint16_t(pos - pTemp);
}

//打包长时平均车速数据
uint16_t CFAWACP::addLTAverageSpeed(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_LTAverageSpeedID);

	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.LTAverageSpeed >> 8) & 0xFF);
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.LTAverageSpeed & 0xFF);

	return uint16_t(pos - pTemp);
}

//打包短时平均车速数据
uint16_t CFAWACP::addSTAverageSpeed(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_STAverageSpeedID);

	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.STAverageSpeed >> 8) & 0xFF);
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.STAverageSpeed & 0xFF);
	
	return uint16_t(pos - pTemp);
}

//打包长时平均油耗数据
uint16_t CFAWACP::addLTAverageOil(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_LTAverageOilID);

	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.LTAverageOil >> 8) & 0xFF);
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.LTAverageOil & 0xFF);
	
	return uint16_t(pos - pTemp);
}

//打包短时平均油耗数据
uint16_t CFAWACP::addSTAverageOil(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_STAverageOilID);

	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.STAverageOil >> 8) & 0xFF);
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.STAverageOil & 0xFF);
	
	return uint16_t(pos - pTemp);
}

//打包车门状态数据
uint16_t CFAWACP::addCarDoorState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_CarDoorStateID);
	
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.CarDoorState;
	
	return uint16_t(pos - pTemp);
}

//打包车辆状态数据
uint16_t CFAWACP::addCarLockState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_CarLockState);

	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.CarLockState.CarLockState;
	
	return uint16_t(pos - pTemp);
}
//打包天窗状态数据
uint16_t CFAWACP::addSunroofState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_SunroofStateID);

	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.sunroofState;
	
	return uint16_t(pos - pTemp);
}
//打包车窗状态数据
uint16_t CFAWACP::addWindowState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_WindowStateID);

	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.WindowState.WindowState >> 8) & 0xFF;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.WindowState.WindowState  & 0xFF;
	
	return uint16_t(pos - pTemp);
}
//打包车灯状态数据
uint16_t CFAWACP::addCarlampState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_CarlampStateID);

	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.CarlampState.CarlampState;
	
	return uint16_t(pos - pTemp);
}
//打包轮胎信息数据
uint16_t CFAWACP::addTyreState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_TyreStateID);

	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.TyreState.leftfrontTemperature;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.TyreState.rightfrontTemperature;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.TyreState.leftrearTemperature;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.TyreState.rightrearTemperature;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.TyreState.leftfrontTyrePress;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.TyreState.rightfrontTyrePress;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.TyreState.leftrearTyrePress;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.TyreState.rightrearTyrePress;

	return uint16_t(pos - pTemp);
}
//打包TBOX-MCU版本数据
uint16_t CFAWACP::addVerTboxMCU(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_VerTboxMCUID);

	memcpy(pos,p_FAWACPInfo_Handle->VehicleCondData.VerTboxMCU, sizeof(p_FAWACPInfo_Handle->VehicleCondData.VerTboxMCU));
	pos += sizeof(p_FAWACPInfo_Handle->VehicleCondData.VerTboxMCU);

	return uint16_t(pos - pTemp);
}
//打包发动机状态数据
uint16_t CFAWACP::addEngineState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_EngineStateID);

	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.EngineState;
	
	return uint16_t(pos - pTemp);
}
//打包实时方向盘转角数据
uint16_t CFAWACP::addWheelState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_WheelStateID);

	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.WheelState.WheelState >> 8) & 0xFF;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.WheelState.WheelState & 0xFF;
	
	return uint16_t(pos - pTemp);
}
//打包历史车速数据
uint16_t CFAWACP::addPastRecordSpeed(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_PastRecordSpeedID);

	for(int i = 0; i < 19; i++){
		*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.PastRecordSpeed[i] >> 8) & 0xFF;
		*pos++ = p_FAWACPInfo_Handle->VehicleCondData.PastRecordSpeed[i] & 0xFF;
	}

	return uint16_t(pos - pTemp);
}
//打包历史方向盘转角数据
uint16_t CFAWACP::addPastRecordWheelState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_PastRecordWheelStateID);

	for(int i = 0;i < 19; i++){
		*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.PastRecordWheel[i].WheelState >> 8) & 0xFF;
		*pos++ = p_FAWACPInfo_Handle->VehicleCondData.PastRecordWheel[i].WheelState & 0xFF;
	}

	return uint16_t(pos - pTemp);
}
//打包发动机转速数据
uint16_t CFAWACP::addEngineSpeed(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_EngineSpeedID);

	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.EngineSpeed >> 8) & 0xFF;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.EngineSpeed & 0xFF;
	
	return uint16_t(pos - pTemp);
}
//打包档位信息数据
uint16_t CFAWACP::addGearState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_GearStateID);

	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.Gearstate;
	
	return uint16_t(pos - pTemp);
}
//打包手刹状态数据
uint16_t CFAWACP::addHandBrakeState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_HandBrakeStateID);

	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.HandbrakeState;
	
	return uint16_t(pos - pTemp);
}
//打包电子驻车状态数据
uint16_t CFAWACP::addParkingState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_ParkingStateID);

	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.ParkingState;
	
	return uint16_t(pos - pTemp);
}
//打包供电模式数据
uint16_t CFAWACP::addPowerSupplyMode(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_PowerSupplyModeID);

	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.Powersupplymode;
	
	return uint16_t(pos - pTemp);
}
//打包安全带状态数据
uint16_t CFAWACP::addSafetyBelt(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_SafetyBeltID);

	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.Safetybeltstate;
	
	return uint16_t(pos - pTemp);
}
//打包剩余保养里程数据
uint16_t CFAWACP::addRemainUpkeepMileage(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_RemainUpkeepMileageID);

	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.RemainUpkeepMileage >> 8) & 0xFF;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.RemainUpkeepMileage & 0xFF;
	
	return uint16_t(pos - pTemp);
}
//打包空调相关信息数据
uint16_t CFAWACP::addAirconditionerInfo(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_AirconditionerInfoID);

	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature >> 5) & 0x03);

			 
	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature << 3) & 0xF8) |
			 (p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.blowingMode  & 0x07);

	*pos++ = ((p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.blowingLevel << 5) & 0XE0) |
			((p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.inOutCirculateState << 4) & 0X10) |
			((p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.defrostState << 3) & 0X08) |
			((p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.autoState << 2) & 0X04) |
			((p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.compressorState << 1) & 0X02) |
			(p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.airconditionerState & 0X01);

	return uint16_t(pos - pTemp);
}
//打包持续时间数据
uint16_t CFAWACP::addKeepingstateTime(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_KeepingstateTimeID);
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.overspeedTime >> 8) & 0xFF;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.overspeedTime & 0XFF;	
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.wheelTime;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.decelerateTime;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.accelerateTime;

	return uint16_t(pos - pTemp);
}
//打包动力电池状态数据
uint16_t CFAWACP::addPowerCellsState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_PowerCellsStateID);
	
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.BatAveraTempera;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.elecTempera;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.elecSOH;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.quantity;
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.electricity >> 8) & 0xFF;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.electricity & 0XFF;
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.voltage >> 8) & 0xFF;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.voltage & 0XFF;
	
	return uint16_t(pos - pTemp);
}
//打包充电状态数据
uint16_t CFAWACP::addChargeState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;	
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_ChargeStateID);

	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.ChargeState.chargeMode;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.ChargeState.remainChargeTime;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.ChargeState.chargeState;
	
	return uint16_t(pos - pTemp);
}
//打包TBOX-OS版本数据
uint16_t CFAWACP::addVerTboxOS(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_VerTboxOSID);
	memcpy(pos, p_FAWACPInfo_Handle->VehicleCondData.VerTboxOS, sizeof(p_FAWACPInfo_Handle->VehicleCondData.VerTboxOS));
	pos += sizeof(p_FAWACPInfo_Handle->VehicleCondData.VerTboxOS);
	
	return uint16_t(pos - pTemp);
}

//打包IVI版本数据
uint16_t CFAWACP::addVerIVI(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_VerIVIID);
	
	memcpy(pos, p_FAWACPInfo_Handle->VehicleCondData.VerIVI, sizeof(p_FAWACPInfo_Handle->VehicleCondData.VerIVI));
	pos += sizeof(p_FAWACPInfo_Handle->VehicleCondData.VerIVI);
	
	return uint16_t(pos - pTemp);
}

//打包充电枪连接状态数据
uint16_t CFAWACP::addChargeConnectState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_ChargeConnetStateID);
	
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.ChargeConnectState;
	
	return uint16_t(pos - pTemp);
}

//打包制动踏板开关数据
uint16_t CFAWACP::addBrakePedalSwitch(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_BrakePedalSwitchID);
	
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.BrakePedalSwitch;
	
	return uint16_t(pos - pTemp);
}

//打包加速踏板开关数据
uint16_t CFAWACP::addAcceleraPedalSwitch(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_AcceleraPedalSwitchID);
	
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.AcceleraPedalSwitch;
	
	return uint16_t(pos - pTemp);
}

//打包Yaw传感器信息数据
uint16_t CFAWACP::addYaWSensorInfoSwitch(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_YaWSensorInfoSwitchID);
	//横向加速度
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.YaWSensorInfoSwitch.TransverseAccele >> 8) & 0x0F;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.YaWSensorInfoSwitch.TransverseAccele & 0xFF;
	//纵向加速度
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.YaWSensorInfoSwitch.LongituAccele >> 8) & 0x0F;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.YaWSensorInfoSwitch.LongituAccele & 0xFF;
	//横摆角速度
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.YaWSensorInfoSwitch.YawVelocity >> 8) & 0x0F;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.YaWSensorInfoSwitch.YawVelocity & 0xFF;
	
	return uint16_t(pos - pTemp);
}

//打包环境温度数据
uint16_t CFAWACP::addAmbientTemperat(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_AmbientTemperatID);
	//环境温度
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.AmbientTemperat.AmbientTemperat >> 8) & 0x07;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.AmbientTemperat.AmbientTemperat & 0xFF;
	
	return uint16_t(pos - pTemp);
}

//打包纯电动继电器及线圈相关状态数据
uint16_t CFAWACP::addPureElecRelayState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_PureElecRelayID);

	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.RechargeNegaRelayCoilState << 4) | 
		(p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.RechargePositRelayCoilState << 3) |
		(p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.PrefillRelayCoilState << 2) |
		(p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.MainNegaRelayCoilState << 1) |
		p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.MainPositRelayCoilState;

	return uint16_t(pos - pTemp);
}

//打包剩余续航里程(12bit)数据
uint16_t CFAWACP::addResidualRange(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_RemainderRangeID);
	//剩余续航里程(12bit)
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.ResidualRange >> 8) & 0xFF;
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.ResidualRange & 0xFF;

	return uint16_t(pos - pTemp);
}

//打包新能源热管理相关请求数据
uint16_t CFAWACP::addNewEnergyHeatManage(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_NewEnergyHeatManageID);
	//电池加热请求	//电机1冷却请求	//电机2冷却请求
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.NewEnergyHeatManage.Motor2CoolRequest << 4) |
		(p_FAWACPInfo_Handle->VehicleCondData.NewEnergyHeatManage.Motor1CoolRequest << 1) |
		(p_FAWACPInfo_Handle->VehicleCondData.NewEnergyHeatManage.BatteryHeatRequest);

	return uint16_t(pos - pTemp);
}

//打包车辆工作模式数据
uint16_t CFAWACP::addVehWorkMode(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_VehWorkModeID);
	//车辆工作模式
	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.VehWorkMode.VehWorkMode;

	return uint16_t(pos - pTemp);
}

//打包电机工作状态数据
uint16_t CFAWACP::addMotorWorkState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	//采集信号编码
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_MotorWorkStateID);
	//电机1工作状态	//电机2工作状态
	*pos++ = (p_FAWACPInfo_Handle->VehicleCondData.MotorWorkState.Motor2Workstate << 3) |
		p_FAWACPInfo_Handle->VehicleCondData.MotorWorkState.Motor1Workstate;

	return uint16_t(pos - pTemp);
}

//打包高压系统准备状态
uint16_t CFAWACP::addHighVoltageState(uint8_t* pTemp)
{
	uint8_t *pos = pTemp;
	pos += SignCodeDeal(pos, 0, ACPSIGNCODE_HighVolageStateID);

	*pos++ = p_FAWACPInfo_Handle->VehicleCondData.HighVoltageState;

	return uint16_t(pos - pTemp);
}



//远程控制打包
uint16_t CFAWACP::RemoteCtrlCommandDeal(uint8_t *dataBuff)
{
	uint8_t *pos = dataBuff;	
	
	uint16_t tempLen = (m_AcpRemoteCtrlList.SubitemTotal *2) +1;
	pos += addDataLen(pos, tempLen, 0, 1); //添加数据长度值

	*pos++ = m_AcpRemoteCtrlList.SubitemTotal;
	for(uint16_t i = 0; i < m_AcpRemoteCtrlList.SubitemTotal; i++)
	{
		*pos++ = m_AcpRemoteCtrlList.SubitemCode[i] & 0xFF;	//子项编码
		
		switch(m_AcpRemoteCtrlList.SubitemCode[i])
		{
			case VehicleBody_LockID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_Lock;//车锁
				break;
			case VehicleBody_WindowID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_Window;//车窗
				break;
			case VehicleBody_SunroofID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_Sunroof;//天窗
				break;
			case VehicleBody_TrackingCarID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_TrackingCar;//寻车
				break;
			case VehicleBody_LowbeamID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_Lowbeam;//近光灯
				break;
			case VehicleBody_LuggageCarID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_LuggageCar;//行李箱
				break;			
			case Airconditioner_ControlID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Control.BitType;//空调控制
				break;
			case Airconditioner_CompressorSwitchID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_CompressorSwitch.BitType;//空调压缩机开关
				break;
			case Airconditioner_TemperatureID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Temperature.BitType;//温度调节
				break;
			case Airconditioner_SetAirVolumeID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_SetAirVolume.BitType;//设定风量
				break;
			case Airconditioner_FrontDefrostSwitchID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_FrontDefrostSwitch.BitType;//前除霜
				break;
			case Airconditioner_HeatedrearID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Heatedrear.BitType;//后窗加热
				break;
			case Airconditioner_BlowingModeID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_BlowingMode.BitType;//吹风模式
				break;
			case Airconditioner_InOutCirculateID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_InOutCirculate.BitType;//内外循环
				break;
			case Airconditioner_AutoSwitchID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_AutoSwitch.BitType;//Auto开关
				break;
			case EngineState_SwitchID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.EngineState.EngineState_Switch;//发动机启停
				break;
			case VehicleSeat_DrivingSeatID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleSeat.VehicleSeat_DrivingSeat;//主驾驶座椅加热
				break;
			case VehicleSeat_CopilotseatID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleSeat.VehicleSeat_Copilotseat;//副驾驶座椅加热
				break;
			case VehicleSeat_DrivingSeatMomeryID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleSeat.VehicleSeat_DrivingSeatMemory;
				break;
			case VehicleChargeMode_ImmediateID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleCharge.VehicleCharge_Immediate;//即时充电
				break;
			case VehicleChargeMode_AppointmentID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleCharge.VehicleCharge_Appointment;//预约充电
				break;
			case VehicleChargeMode_EmergencyCharg:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleCharge.VehicleCharge_EmergenCharg;//紧急充电
				break;
			case VehicleWIFIStatusID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleAutopark.VehicleWifiStatus;//WIFI状态
				break;
			case VehicleAutoOUTID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.VehicleAutopark.VehicleAutoOut;//自动出车
				break;
			case FeatureConfig_StateID:
				*pos++ = p_FAWACPInfo_Handle->RemoteControlData.FunctionConfigStatus;	//功能配置状态
				break;
			default:
				break;
		}
	}
	return uint16_t(pos - dataBuff);
}


//车况上报数据打包
uint16_t CFAWACP::ReportVehicleCondDeal(uint8_t* dataBuff, uint8_t flag)
{
	uint16_t tempLen = 0;
	static uint8_t CollectPacketSN = 0;
	uint16_t signCodeTotal = 0;
	uint8_t *pos = dataBuff;
	uint8_t *pData = dataBuff;

	*pos++ = 0;
	*pos++ = 0;
	//发送方式0,即时发送 1,补发
	*pos++ = m_SendMode;//AcpVehicleCondition.SendMode;
	if(CollectPacketSN == 254)
		CollectPacketSN = 0;
	//采集包序号
	*pos++ = CollectPacketSN++;
	//时间戳不含时间长度
	TimeDeal(pos, 0);
	pos += DATA_TIMESTAMP_SIZE - 1;
	//采集信号总数
	if(flag == 0)
		signCodeTotal = MAX_SIGNCODE_TOTAL;
	else if(flag == 1)
		signCodeTotal = MAX_SIGNCODE_TOTAL - 2 + MAX_FAULT_SIGNA + MAX_DRIVEACTION_SIGNA;//特别注意
	else if(flag == 2)
		signCodeTotal = MAX_SIGNCODE_TOTAL - 2 + MAX_DRIVEACTION_SIGNA;
	else
		signCodeTotal = MAX_SIGNCODE_TOTAL - 2;

	*pos++	= (signCodeTotal >> 8) & 0xFF;
	*pos++	= signCodeTotal & 0xFF;
	for(uint16_t i = 0; i < MAX_SIGNCODE_TOTAL; i++)
	{
		switch(i)
		{
			case ACPSIGNCODE_GPSID:
				pos += add_GPSData(pos);				
				break;
			case ACPSIGNCODE_RemainedOilID:
				pos += addRemainedOil(pos);
				break;
			case ACPSIGNCODE_OdometerID:
				pos += addOdometer(pos);
				break;
			case ACPSIGNCODE_BatteryID:
				pos += addBattery(pos);
				break;
			case ACPSIGNCODE_CurrentSpeedID:
				pos += addCurrentSpeed(pos);
				break;
			case ACPSIGNCODE_LTAverageSpeedID:
				pos += addLTAverageSpeed(pos);
				break;
			case ACPSIGNCODE_STAverageSpeedID:
				pos += addSTAverageSpeed(pos);
				break;
			case ACPSIGNCODE_LTAverageOilID:
				pos += addLTAverageOil(pos);
				break;
			case ACPSIGNCODE_STAverageOilID:
				pos += addSTAverageOil(pos);
				break;
			case ACPSIGNCODE_CarDoorStateID:
				pos += addCarDoorState(pos);
				break;
			case ACPSIGNCODE_CarLockState:
				pos += addCarLockState(pos);
				break;
			case ACPSIGNCODE_SunroofStateID:
				pos += addSunroofState(pos);
				break;
			case ACPSIGNCODE_WindowStateID:
				pos += addWindowState(pos);
				break;
			case ACPSIGNCODE_CarlampStateID:
				pos += addCarlampState(pos);
				break;
			case ACPSIGNCODE_TyreStateID:
				pos += addTyreState(pos);
				break;
			case ACPSIGNCODE_VerTboxMCUID:
				pos += addVerTboxMCU(pos);
				break;
			case ACPSIGNCODE_EngineStateID:
				pos += addEngineState(pos);
				break;
			case ACPSIGNCODE_WheelStateID:
				pos += addWheelState(pos);
				break;
			case ACPSIGNCODE_PastRecordSpeedID:
				if(flag == 0)
				{
					pos += addPastRecordSpeed(pos);
				}
				break;
			case ACPSIGNCODE_PastRecordWheelStateID:
				if(flag == 0)
				{
					pos += addPastRecordWheelState(pos);
				}
				break;
			case ACPSIGNCODE_EngineSpeedID:
				pos += addEngineSpeed(pos);
				break;
			case ACPSIGNCODE_GearStateID:
				pos += addGearState(pos);
				break;
			case ACPSIGNCODE_HandBrakeStateID:
				pos += addHandBrakeState(pos);
				break;
			case ACPSIGNCODE_ParkingStateID:
				pos += addParkingState(pos);
				break;
			case ACPSIGNCODE_PowerSupplyModeID:
				pos += addPowerSupplyMode(pos);
				break;
			case ACPSIGNCODE_SafetyBeltID:
				pos += addSafetyBelt(pos);
				break;
			case ACPSIGNCODE_RemainUpkeepMileageID:
				pos += addRemainUpkeepMileage(pos);
				break;
			case ACPSIGNCODE_AirconditionerInfoID:
				pos += addAirconditionerInfo(pos);
				break;
			case ACPSIGNCODE_KeepingstateTimeID:
				pos += addKeepingstateTime(pos);
				break;
			case ACPSIGNCODE_PowerCellsStateID:
				pos += addPowerCellsState(pos);
				break;
			case ACPSIGNCODE_ChargeStateID:
				pos += addChargeState(pos);
				break;
			case ACPSIGNCODE_VerTboxOSID:
				pos += addVerTboxOS(pos);
				break;
			case ACPSIGNCODE_VerIVIID:
				pos += addVerIVI(pos);
				break;
			case ACPSIGNCODE_ChargeConnetStateID:
				pos += addChargeConnectState(pos);
				break;
			case ACPSIGNCODE_BrakePedalSwitchID:
				pos += addBrakePedalSwitch(pos);	
				break;
			case ACPSIGNCODE_AcceleraPedalSwitchID:
				pos += addAcceleraPedalSwitch(pos);
				break;
			case ACPSIGNCODE_YaWSensorInfoSwitchID:
				pos += addYaWSensorInfoSwitch(pos);
				break;
			case ACPSIGNCODE_AmbientTemperatID:
				pos += addAmbientTemperat(pos);
				break;
			case ACPSIGNCODE_PureElecRelayID:
				pos +=addPureElecRelayState(pos);
				break;
			case ACPSIGNCODE_RemainderRangeID:
				pos += addResidualRange(pos);
				break;
			case ACPSIGNCODE_NewEnergyHeatManageID:
				pos += addNewEnergyHeatManage(pos);
				break;
			case ACPSIGNCODE_VehWorkModeID:
				pos += addVehWorkMode(pos);
				break;
			case ACPSIGNCODE_MotorWorkStateID:
				pos += addMotorWorkState(pos);
				break;
			case ACPSIGNCODE_HighVolageStateID:
				pos += addHighVoltageState(pos);
				break;
			default:
				break;
		}
	}
	//加入故障信号
	if(flag == 1)
	{
		pos += FaultSignDeal(pos);
	}
				
	if(flag == 2)
	{
		pos += addFaultSignDeal(pos);
	}
	
	tempLen = uint16_t(pos - dataBuff) - 2;//数据长度不包含自身长度值长度
	
	pData[0] = ((tempLen >> 7) | 0x20);
	pData[1] = (tempLen & 0x7F);

	FAWACPLOG("data Response tempLen==%d\n",tempLen);
	FAWACP_NO("pData[0]: %02x pData[1]: %02x", pData[0], pData[1]);
	FAWACP_NO("\n\n");

	return uint16_t(pos - dataBuff);
}


//远程配置数据打包	TBOX+MCU
uint16_t CFAWACP::RemoteConfigCommandDeal(uint8_t *dataBuff)
{
	uint8_t *pos = dataBuff;
	
	uint16_t tempLen = 0;
//	pos += addDataLen(pos, tempLen, 0, 1);
	*pos++ = 0;
	*pos++ = 0;
	
	*pos++ = m_AcpRemoteConfig.DeviceTotal;		//配置的设备总数
	for(uint8_t i = 0; i < m_AcpRemoteConfig.DeviceTotal; i++)
	{
		for(uint8_t k = 0; k < m_AcpRemoteConfig.DeviceTotal; k++)
		{
			//匹配设备编号
			if(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].DeviceNo == m_AcpRemoteConfig.DeviceConfig[i].DeviceNo)
			{
				*pos++ = (m_AcpRemoteConfig.DeviceConfig[i].DeviceNo >> 8) & 0xFF;//设备编号
				*pos++ = m_AcpRemoteConfig.DeviceConfig[i].DeviceNo & 0xFF;		
				*pos++ = m_AcpRemoteConfig.DeviceConfig[i].ConfigCount;			//配置项数量
				for(uint8_t j = 0; j < m_AcpRemoteConfig.DeviceConfig[i].ConfigCount; j++)
				{
					*pos++ = m_AcpRemoteConfig.DeviceConfig[i].ConfigItem[j].ConfigItemNum;	//配置项编号
					switch(m_AcpRemoteConfig.DeviceConfig[i].ConfigItem[j].ConfigItemNum)
					{
						case ACPCfgItem_EngineStartTimeID:		//发动机远程启动时长,单位min
							*pos++ = (p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].EngineStartTime >> 8) & 0xFF;
							*pos++ = p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].EngineStartTime & 0xFF;
							break;
						case ACPCfgItem_SamplingSwitchID:		//电动车国标抽检开关
							*pos++ = p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].SamplingSwitch;
							break;
						case ACPCfgItem_EmergedCallID:			//紧急服务号码配置(ASCII码)
							//dataPool->getPara(E_CALL_ID, (void *)EmergedCall, sizeof(EmergedCall));
							//memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].EmergedCall,EmergedCall,15);//紧急服务号码配置(ASCII码)
							memcpy(pos, p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].EmergedCall, 15);
							pos += 15;
							break;
						case ACPCfgItem_WhitelistCallID:		//白名单号码配置(ASCII码)
							memcpy(pos, p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].WhitelistCall, 15);
							pos += 15;
							break;
						case ACPCfgItem_CollectTimeIntervalID:	//车况定时采集间隔时间,单位S,默认10
							*pos++ = p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].CollectTimeInterval;
							break;
						case ACPCfgItem_ReportTimeIntervalID:	//车况定时上报间隔时间,单位S,默认10
							*pos++ = p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].ReportTimeInterval;
							break;
						case ACPCfgItem_CollectTimeGpsSpeedIntervalID://定时采集(位置和车速)间隔时间,单位S,默认5
							*pos++ = p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].CollectTimeGpsSpeedInterval;
							break;
						case ACPCfgItem_ReportTimeGpsSpeedIntervalID://定时上报(位置和车速)间隔时间,单位S,默认5
							*pos++ = p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].ReportTimeGpsSpeedInterval;
							break;
						case ACPCfgItem_ChargeModeID:				//充电模式
							*pos++ = p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].ChargeMode;
							break;
						case ACPCfgItem_ChargeScheduleID:			//预约充电时间表
							*pos++ = p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].ChargeSchedule.ScheduChargStartData;
							*pos++ = (p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].ChargeSchedule.ScheduChargStartTime >> 8) & 0xFF;
							*pos++ = p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].ChargeSchedule.ScheduChargStartTime & 0xFF;
							*pos++ = p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].ChargeSchedule.ScheduChargEndData;
							*pos++ = (p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].ChargeSchedule.ScheduChargEndTime >> 8) & 0xFF;
							*pos++ = p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].ChargeSchedule.ScheduChargEndTime & 0xFF;
							break;
						case ACPCfgItem_LoadCellID:
							memcpy(pos, p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].LoadCell, 15);
							pos += 15;
							break;
						case ACPCfgItem_InformationCellID:
							memcpy(pos, p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[k].InformationCell, 15);
							pos += 15;
							break;
						default:
							break;
					}
				}
			}
		}
	}
	tempLen = uint16_t(pos - dataBuff) - 2;
	dataBuff[0] = ((tempLen >> 7) | 0x20);
	dataBuff[1] = (tempLen & 0x7F);

	return uint16_t(pos - dataBuff);
}

//RootKey更换
uint16_t CFAWACP::RootKeyDataDeal(uint8_t *dataBuff)
{
  uint8_t *pos = dataBuff;
  //数据长度
  uint16_t tempLen = 32;
  pos += addDataLen(pos, tempLen, 0, 1);
  //采用新Rootkey加密新的Rootkey
 Encrypt_AES128Data(p_FAWACPInfo_Handle->New_RootKey, p_FAWACPInfo_Handle->New_RootKey, sizeof(p_FAWACPInfo_Handle->New_RootKey), pos, AcpCryptFlag_IS);

  pos += tempLen;

  return uint16_t(pos - dataBuff);
}

// 打包远程升级【命令返回】数据结构 
uint16_t CFAWACP::RemoteUpgrade_ResponseDeal(uint8_t *dataBuff)
{
	uint8_t *pos = dataBuff;

	uint16_t tempLen = 4 + m_AcpRemoteUpgrade.UpdateFile_Len + m_AcpRemoteUpgrade.Ver_Len;
	pos += addDataLen(pos, tempLen, 1, 1);

	*pos++ = m_AcpRemoteUpgrade.DeviceNo;
	*pos++ = m_AcpRemoteUpgrade.UpdateFile_Len;
	memcpy(pos, m_AcpRemoteUpgrade.UpdateFile_Param, m_AcpRemoteUpgrade.UpdateFile_Len);
	pos += m_AcpRemoteUpgrade.UpdateFile_Len;
	*pos++ = m_AcpRemoteUpgrade.Ver_Len;
	memcpy(pos, m_AcpRemoteUpgrade.VerNo, m_AcpRemoteUpgrade.Ver_Len);
	pos += m_AcpRemoteUpgrade.Ver_Len;
	*pos++ = Upgrade_CommandResponse;

	return uint16_t(pos - dataBuff);
}

/* 打包远程升级【下载开始】数据结构 */
uint16_t CFAWACP::RemoteUpgrade_DownloadStartDeal(uint8_t *dataBuff)
{
	uint8_t *pos = dataBuff;
	
	uint16_t tempLen = 4 + m_AcpRemoteUpgrade.UpdateFile_Len + m_AcpRemoteUpgrade.Ver_Len;
	pos += addDataLen(pos, tempLen, 1, 1);

	*pos++ = m_AcpRemoteUpgrade.DeviceNo;
	*pos++ = m_AcpRemoteUpgrade.UpdateFile_Len;
	memcpy(pos, m_AcpRemoteUpgrade.UpdateFile_Param, m_AcpRemoteUpgrade.UpdateFile_Len);
	pos += m_AcpRemoteUpgrade.UpdateFile_Len;
	*pos++ = m_AcpRemoteUpgrade.Ver_Len;
	memcpy(pos, m_AcpRemoteUpgrade.VerNo, m_AcpRemoteUpgrade.Ver_Len);
	pos += m_AcpRemoteUpgrade.Ver_Len;
	*pos++ = Upgrade_Reserve;
	
	return uint16_t(pos - dataBuff);
}

/* 打包远程升级【下载结果】数据结构 */
uint16_t CFAWACP::RemoteUpgrade_DownloadResultDeal(uint8_t *dataBuff)
{
	uint8_t *pos = dataBuff;
	
	uint16_t tempLen = 4 + m_AcpRemoteUpgrade.UpdateFile_Len + m_AcpRemoteUpgrade.Ver_Len;
	pos += addDataLen(pos, tempLen, 1, 1);

	*pos++ = m_AcpRemoteUpgrade.DeviceNo;
	*pos++ = m_AcpRemoteUpgrade.UpdateFile_Len;
	memcpy(pos, m_AcpRemoteUpgrade.UpdateFile_Param, m_AcpRemoteUpgrade.UpdateFile_Len);
	pos += m_AcpRemoteUpgrade.UpdateFile_Len;
	*pos++ = m_AcpRemoteUpgrade.Ver_Len;
	memcpy(pos, m_AcpRemoteUpgrade.VerNo, m_AcpRemoteUpgrade.Ver_Len);
	pos += m_AcpRemoteUpgrade.Ver_Len;
	*pos++ = Upgrade_DownloadState;
	
	return uint16_t(pos - dataBuff);
}

/* 打包远程升级【升级开始】数据结构 */
uint16_t CFAWACP::RemoteUpgrade_UpdateStartDeal(uint8_t *dataBuff)
{
	uint8_t *pos = dataBuff;
	
	uint16_t tempLen = 4 + m_AcpRemoteUpgrade.UpdateFile_Len + m_AcpRemoteUpgrade.Ver_Len;
	pos += addDataLen(pos, tempLen, 1, 1);

	*pos++ = m_AcpRemoteUpgrade.DeviceNo;
	*pos++ = m_AcpRemoteUpgrade.UpdateFile_Len;
	memcpy(pos, m_AcpRemoteUpgrade.UpdateFile_Param, m_AcpRemoteUpgrade.UpdateFile_Len);
	pos += m_AcpRemoteUpgrade.UpdateFile_Len;
	*pos++ = m_AcpRemoteUpgrade.Ver_Len;
	memcpy(pos, m_AcpRemoteUpgrade.VerNo, m_AcpRemoteUpgrade.Ver_Len);
	pos += m_AcpRemoteUpgrade.Ver_Len;
	*pos++ = Upgrade_Reserve;
	
	return uint16_t(pos - dataBuff);
}

/* 打包远程升级【升级结果】数据结构 */
uint16_t CFAWACP::RemoteUpgrade_UpdateResultDeal(uint8_t *dataBuff)
{
	uint8_t *pos = dataBuff;
	
	uint16_t tempLen = 4 + m_AcpRemoteUpgrade.UpdateFile_Len + m_AcpRemoteUpgrade.Ver_Len;
	pos += addDataLen(pos, tempLen, 1, 1);

	*pos++ = m_AcpRemoteUpgrade.DeviceNo;
	*pos++ = m_AcpRemoteUpgrade.UpdateFile_Len;
	memcpy(pos, m_AcpRemoteUpgrade.UpdateFile_Param, m_AcpRemoteUpgrade.UpdateFile_Len);
	pos += m_AcpRemoteUpgrade.UpdateFile_Len;
	*pos++ = m_AcpRemoteUpgrade.Ver_Len;
	memcpy(pos, m_AcpRemoteUpgrade.VerNo, m_AcpRemoteUpgrade.Ver_Len);
	pos += m_AcpRemoteUpgrade.Ver_Len;
	*pos++ = Upgrade_Result;
	
	return uint16_t(pos - dataBuff);
}
//位置和车速定时上传---GPS位置信号+实时车速
uint16_t CFAWACP::ReportGPSSpeedDeal(uint8_t* dataBuff)
{
	static uint8_t CollectPacketSN = 0;
	uint8_t *pos = dataBuff;
	uint16_t SignTotal = 2;
	uint16_t tempLen = 31;

	pos += addDataLen(pos, tempLen, 0, 0);
	//发送方式0,即时发送 1,补发
	*pos++ = m_SendMode;
	//采集包序号
	if(CollectPacketSN == 254)
		CollectPacketSN = 0;
	*pos++ = CollectPacketSN++;
	//去掉数据长度部分
	TimeDeal(pos, 0);
	pos += DATA_TIMESTAMP_SIZE - 1;
	//采集信号总数
	*pos++ = (SignTotal >> 8) & 0xFF;
	*pos++ = SignTotal & 0xFF;
	//GPS位置数据
	pos += add_GPSData(pos);
	//实时速度
	pos += addCurrentSpeed(pos);

	return uint16_t(pos - dataBuff);
}


/*打包故障信号*/
uint16_t CFAWACP::FaultSignDeal(uint8_t* dataBuff)
{
	uint8_t *pos = dataBuff;
	uint16_t datalen = 0;

	for(uint16_t i = 100; i < 145; i++)
	{
		switch(i)
		{
			case ACPCODEFAULT_EMSID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_EMSID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEMSState;
				break;
			case ACPCODEFAULT_TCUID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_TCUID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTCUState;
				break;
			case ACPCODEFAULT_EmissionID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_EmissionID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEmissionState;
				break;
			case ACPCODEFAULT_SRSID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_SRSID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSRSState;
				break;
			case ACPCODEFAULT_ESPID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ESPID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESPState;
				break;
			case ACPCODEFAULT_ABSID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ABSID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTABSState;
				break;
			case ACPCODEFAULT_EPASID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_EPASID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEPASState;
				break;
			case ACPCODEFAULT_OilPressureID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_OilPressureID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTOilPressureState;
				break;
			case ACPCODEFAULT_LowOilID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_LowOilID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLowOilIDState;
				break;
			case ACPCODEFAULT_BrakeFluidLevelID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_BrakeFluidLevelID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBrakeFluidLevelState;
				break;
			case ACPCODEFAULT_BatteryChargeID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_BatteryChargeID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBatteryChargeState;
				break;
			case ACPCODEFAULT_BBWID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_BBWID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBBWState;
				break;
			case ACPCODEFAULT_TPMSID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_TPMSID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTPMSState;
				break;
			case ACPCODEFAULT_STTID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_STTID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSTTState;
				break;
			case ACPCODEFAULT_ExtLightID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ExtLightID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTExtLightState;
				break;
			case ACPCODEFAULT_ESCLID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ESCLID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESCLState;
				break;
			case ACPCODEFAULT_EngineOverwaterID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_EngineOverwaterID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEngineOverwaterState;
				break;
			case ACPCODEFAULT_ElecParkUnitID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ElecParkUnitID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTElecParkUnitState;
				break;
			case ACPCODEFAULT_AHBID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_AHBID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAHBState;
				break;
			case ACPCODEFAULT_ACCID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ACCID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTACSState;
				break;
			case ACPCODEFAULT_FCWSID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_FCWSID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWSState;
				break;
			case ACPCODEFAULT_LDWID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_LDWID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLDWState;
				break;
			case ACPCODEFAULT_BlindSpotDetectID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_BlindSpotDetectID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBlindSpotDetectState;
				break;
			case ACPCODEFAULT_AirconManualID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_AirconManualID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAirconManualState;
				break;
			case ACPCODEFAULT_HVSystemID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_HVSystemID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVSystemState;
				break;
			case ACPCODEFAULT_HVInsulateID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_HVInsulateID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVInsulateState;
				break;
			case ACPCODEFAULT_HVILID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_HVILID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVILState;
				break;
//			case ACPCODEFAULT_BatteryChargeID:
//				SignCodelen = SignCodeDeal(pos, 1, ACPCODEFAULT_BatteryChargeID);
//				pos += SignCodelen;
//				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBatteryChargeState;
//				break;
			case ACPCODEFAULT_EVCellID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_EVCellID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellState;
				break;
			case ACPCODEFAULT_PowerMotorID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_PowerMotorID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPowerMotorState;
				break;
			case ACPCODEFAULT_EParkID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_EParkID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEParkState;
				break;
			case ACPCODEFAULT_EVCellLowBatteryID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_EVCellLowBatteryID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellLowBatteryState;
				break;
			case ACPCODEFAULT_EVCellOverTemperateID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_EVCellOverTemperateID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellOverTemperateState;
				break;
			case ACPCODEFAULT_PowerMotorOverTemperateID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_PowerMotorOverTemperateID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPowerMotorOverTemperateState;
				break;
			case ACPCODEFAULT_ConstantSpeedSystemFailID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ConstantSpeedSystemFailID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTConstantSpeedSystemFailState;
				break;
			case ACPCODEFAULT_ChargerFaultID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ChargerFaultID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTChargerFaultState;
				break;
			case ACPCODEFAULT_AirFailureID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_AirFailureID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAirFailureState;
				break;
			case ACPCODEFAULT_AlternateAuxSystemFailID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_AlternateAuxSystemFailID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlternateAuxSystemFailState;
				break;
			case ACPCODEFAULT_AutoEmergeSystemFailID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_AutoEmergeSystemFailID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAutoEmergeSystemFailState;
				break;
			case ACPCODEFAULT_ReverRadarSystemFailID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ReverRadarSystemFailID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTReverRadarSystemFailState;
				break;
			case ACPCODEFAULT_ElecGearSystemFailID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ElecGearSystemFailID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTElecGearSystemFailState;
				break;
			case ACPCODEFAULT_TyreAlarmID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_TyreAlarmID);				
				*pos++ = (p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightRearTireTempAlarm << 3) |
					p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightRearTirePressAlarm;
				*pos++ = (p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftRearTireTempAlarm << 3) |
					p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftRearTirePressAlarm;
				*pos++ = (p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightrontTireTempAlarm << 3) |
					p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightFrontTirePressAlarm;
				*pos++ = (p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftFrontTireTempAlarm << 3) |
					p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftFrontTirePressAlarm;
				break;
			case ACPCODEFAULT_DCDCConverterFaultID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_DCDCConverterFaultID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTDCDCConverterFaultState;
				break;
			case ACPCODEFAULT_VehControllerFailID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_VehControllerFailID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTVehControllerFailState;
				break;
			case ACPCODEFAULT_PureElecRelayCoilFualtID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_PureElecRelayCoilFualtID);				
				*pos++ = (p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainNagetiveRelayFault << 6) |
					(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainPositiveRelayFault << 5) |
				(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.RechargeNegaRelayCoilFault << 4) |
				(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.RechargePositRelayCoilFault << 3) |
					(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.PrefillRelayCoilFault << 2) |
					(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainNegaRelayCoilFault << 1) |
					p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainPositRelayCoilFault;
				break;
			default:
				break;
		}		
	}

	pos += addFaultSignDeal(pos);
	

	return uint16_t(pos - dataBuff);
}


//驾驶行为特殊事件
uint16_t CFAWACP::addFaultSignDeal(uint8_t* dataBuff)
{
	uint8_t *pos = dataBuff;

	for(uint16_t i = 500; i < 537; i++)
	{
		switch(i)
		{
			case ACPCODEFAULT_TSROverSpeedAlarmID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_TSROverSpeedAlarmID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTSROverSpeedAlarmState;
				break;
			case ACPCODEFAULT_TSRLimitSpeedID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_TSRLimitSpeedID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTSRLimitSpeedState;
				break;
			case ACPCODEFAULT_AEBInterventionID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_AEBInterventionID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAEBInterventionState;
				break;
			case ACPCODEFAULT_ABSInterventionID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ABSInterventionID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTABSInterventionState;
				break;
			case ACPCODEFAULT_ASRInterventionID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ASRInterventionID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTASRInterventionState;
				break;
			case ACPCODEFAULT_ESPInterventionID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ESPInterventionID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTASRInterventionState;
				break;
			case ACPCODEFAULT_DSMAlarmID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_DSMAlarmID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTDSMAlarmState;
				break;
			case ACPCODEFAULT_TowHandOffDiskID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_TowHandOffDiskID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTowHandOffDiskState;
				break;
			case ACPCODEFAULT_ACCStateID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ACCStateID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTACCState;
				break;
			case ACPCODEFAULT_ACCSetSpeedID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_ACCSetSpeedID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTACCSetSpeedState;
				break;
			case ACPCODEFAULT_FCWAlarmID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_FCWAlarmID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWAlarmState;
				break;
			case ACPCODEFAULT_FCWStateID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_FCWStateID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWState;
				break;
			case ACPCODEFAULT_FCWAlarmAccePedalFallID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_FCWAlarmAccePedalFallID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWAlarmAccePedalFallState;
				break;
			case ACPCODEFAULT_FCWAlarmFirstBrakeID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_FCWAlarmFirstBrakeID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWAlarmFirstBrakeState;
				break;
			case ACPCODEFAULT_LDWStateID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_LDWStateID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSLDWState;
				break;
			case ACPCODEFAULT_LDWAlarmID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_LDWAlarmID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLDWAlarmState;
				break;
			case	ACPCODEFAULT_LDWAlarmDireDiskResponseID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_LDWAlarmDireDiskResponseID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLDWAlarmDireDiskResponseState;
				break;
			case ACPCODEFAULT_LKAStateID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_LKAStateID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLKAState;
				break;
			case ACPCODEFAULT_LKAInterventionID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_LKAInterventionID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLKAInterventionState;
				break;
			case ACPCODEFAULT_LKADriverTakeOverPromptID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_LKADriverTakeOverPromptID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLKADriverTakeOverPromptState;
				break;
			case ACPCODEFAULT_LKADriverResponsID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_LKADriverResponsID);				
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLKADriverResponsState;
				break;
			case ACPCODEFAULT_BSDStateID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_BSDStateID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLBSDState;
				break;
			case ACPCODEFAULT_BSDLeftSideAlarmID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_BSDLeftSideAlarmID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDLeftSideAlarmState;
				break;
			case ACPCODEFAULT_BSDRightSideAlarmID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_BSDRightSideAlarmID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDRightSideAlarmState;
				break;
			case ACPCODEFAULT_BSDAlarmReftWheelRespID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_BSDAlarmReftWheelRespID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDAlarmReftWheelRespState;
				break;
			case ACPCODEFAULT_BSDAlarmFirstBrakeID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_BSDAlarmFirstBrakeID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDAlarmFirstBrakeState;
				break;
			case ACPCODEFAULT_BSDAlarmPedalAcceID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_BSDAlarmPedalAcceID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDAlarmPedalAcceState;
				break;
			case ACPCODEFAULT_CrossLeftReportID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_CrossLeftReportID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossLeftReportState;
				break;
			case ACPCODEFAULT_CrossRightReportID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_CrossRightReportID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossRightReportState;
				break;
			case ACPCODEFAULT_CrossAlarmWhellID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_CrossAlarmWhellID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossAlarmWhellState;
				break;
			case ACPCODEFAULT_CrossAlarmStopID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_CrossAlarmStopID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossAlarmStopState;
				break;
			case ACPCODEFAULT_CrossAlarmAcceTreadleID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_CrossAlarmAcceTreadleID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossAlarmAcceTreadleState;
				break;
			case ACPCODEFAULT_AlterTrackAssistLeftAlarmID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_AlterTrackAssistLeftAlarmID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistLeftAlarmState;
				break;
			case ACPCODEFAULT_AlterTrackAssistRightAlarmID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_AlterTrackAssistRightAlarmID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistRightAlarmState;
				break;

			case ACPCODEFAULT_AlterTrackAssistDireRepsonID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_AlterTrackAssistDireRepsonID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistDireRepsonState;
				break;
			case ACPCODEFAULT_AlterTrackAssistFirstStopID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_AlterTrackAssistFirstStopID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistFirstStopState;
				break;
			case ACPCODEFAULT_AlterTrackAssistAcceDropID:
				pos += SignCodeDeal(pos, 0, ACPCODEFAULT_AlterTrackAssistAcceDropID);
				*pos++ = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistAcceDropState;
				break;
			default:
				break;
		}
	}
	return uint16_t(pos - dataBuff);
}




/*车辆定位---GPS位置信号*/
uint16_t CFAWACP::VehicleGPSDeal(uint8_t* dataBuff)
{
	static uint8_t CollectPacketSN = 0;
	uint8_t *pos = dataBuff;
	uint16_t tempLen = 27;
	uint16_t SignTotal = 1;

	pos += addDataLen(pos, tempLen, 0, 0);
	//发送方式0,即时发送 1,补发
	*pos++ = m_SendMode;
	//采集包序号
	if(CollectPacketSN == 254)
		CollectPacketSN = 0;
	*pos++ = CollectPacketSN++;
	//去掉数据长度部分
	TimeDeal(pos, 0);
	pos += DATA_TIMESTAMP_SIZE - 1;
	//采集信号总数
	*pos++ = (SignTotal >> 8) & 0xFF;
	*pos++ = SignTotal & 0xFF;
	//GPS位置数据
	pos += add_GPSData(pos);

	return uint16_t(pos - dataBuff);
}

/*远程诊断*/
uint16_t CFAWACP::RemoteDiagnosticDeal(uint8_t *dataBuff)
{
	uint16_t dataLen = 0;
	uint32_t temp = 0;
	uint8_t *pos = dataBuff;
	uint8_t *pTemp = dataBuff;
	//数据长度
	*pos++ = 0;
	*pos++ = 0;
	//应答诊断控制器总数
	*pos++ = m_RemoteDiagnoseResult.DiagnoseTotal;

	for(uint8_t i = 0; i < m_RemoteDiagnoseResult.DiagnoseTotal; i++)
	{
		*pos++ = m_RemoteDiagnoseResult.DiagnoseCodeFault[i].DiagnoseCode;			//诊断控制器编码
		*pos++ = m_RemoteDiagnoseResult.DiagnoseCodeFault[i].DiagnoseCodeFaultTotal;	//诊断控制器编码故障总数
		for(uint8_t k = 0; k < m_RemoteDiagnoseResult.DiagnoseCodeFault[i].DiagnoseCodeFaultTotal; k++)
		{
			temp = m_RemoteDiagnoseResult.DiagnoseCodeFault[i].DiagnoseCodeFault[k];
			*pos++ = (temp >> 24) & 0xFF;//诊断控制器编码故障
			*pos++ = (temp >> 16) & 0xFF;
			*pos++ = (temp >> 8) & 0xFF;
			*pos++ = temp & 0xFF;
		}
	}
	//数据长度赋值
	dataLen = uint16_t(pos - dataBuff) - 2;
	pTemp[0] = ((dataLen >> 7) | 0x20);
	pTemp[1] = (dataLen & 0x7F);

	return uint16_t(pos - dataBuff);
}


/***********************************************************************************
 ***************            解析协议数据包处理            ***************
************************************************************************************/
//显示数据时间
int CFAWACP::ShowdataTime(uint8_t *pos)
{
	uint8_t Tbuff[32] = {};
	
	int year = (int )((pos[1] >> 2) & 0x3F) + 1990;
	int month = ((pos[1] << 2) & 0x0C) | ((pos[2] >> 6) & 0x03);
	int day = (pos[2] >> 1) & 0x1F;
	int hour = ((pos[2] << 4) & 0x10) | ((pos[3] >> 4) & 0x0F);
	int minutes = ((pos[3] << 2) & 0x3C) | ((pos[4] >> 6) & 0x03);
	int seconds = (pos[4] & 0x3F);
	int msSeconds = pos[5]|pos[6];
	
	sprintf((char *)Tbuff, "%d-%d-%d-%d-%d-%d-%d",year, month, day, hour, minutes, seconds, msSeconds);
	FAWACPLOG("Time Stamp: %s\n", Tbuff);
#if 1
	if(m_loginState == 1 && p_FAWACPInfo_Handle->VehicleCondData.GPSData.GPSState == 0){
		m_tspTimestamp.Year = year - 1990;
		m_tspTimestamp.Month = month;
		m_tspTimestamp.Day = day;
		m_tspTimestamp.Hour = hour;
		m_tspTimestamp.Minutes = minutes;
		m_tspTimestamp.Seconds = seconds;
		m_tspTimestamp.msSeconds = msSeconds;
	}
#endif
	return 0;
}

/*******unpack登陆认证*******/
uint16_t 	CFAWACP::UnpackData_AcpLoginAuthen(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t MsgType)
{
	uint8_t *pos = PayloadBuff;
	uint16_t DataLen = 0;
	m_loginState = 1;
	//显示时间戳部分
	ShowdataTime(pos);	
	pos += 7;
	//解析数据长度
	pos += unpackDataLenDeal(pos,DataLen);
	FAWACPLOG("DataLen == %d\n",DataLen);
	switch(MsgType)
	{
		case 2:		//AKey Apply Message
			memcpy(AcpTsp_AkeyMsg.AkeyC_Tsp, pos, sizeof(AcpTsp_AkeyMsg.AkeyC_Tsp));	//AKEY(C)
			pos += sizeof(AcpTsp_AkeyMsg.AkeyC_Tsp);
			memcpy(AcpTsp_AkeyMsg.Rand1_Tsp, pos, sizeof(AcpTsp_AkeyMsg.Rand1_Tsp));	//Randl
			pos += sizeof(AcpTsp_AkeyMsg.Rand1_Tsp);
			sendLoginCTMsg();
			break;
		case 4:		//CS Message
			memcpy(AcpTsp_CSMsg.Rand2CS_Tsp, pos, sizeof(AcpTsp_CSMsg.Rand2CS_Tsp));	//Rand2(CS)
			pos += sizeof(AcpTsp_CSMsg.Rand2CS_Tsp);
			AcpTsp_CSMsg.RT_Tsp = *pos++;				//RT
			sendLoginRSMsg();
			break;
		case 6:		//Skey Message
			memcpy(AcpTsp_SKeyMsg.SkeyC_Tsp, pos, sizeof(AcpTsp_SKeyMsg.SkeyC_Tsp));	//SKey(C)
			pos += sizeof(AcpTsp_SKeyMsg.SkeyC_Tsp);
			memcpy(AcpTsp_SKeyMsg.Rand3CS_Tsp, pos, sizeof(AcpTsp_SKeyMsg.Rand3CS_Tsp));//Rand3(CS)
			pos += sizeof(AcpTsp_SKeyMsg.Rand3CS_Tsp);
			sendLoginAuthReadyMsg();			
			break;
		case 8:		//Auth Ready ACK Message
			memcpy(AcpTsp_AuthReadyACKMsg.ACKMsg_AuthToken.AuthToken, pos, sizeof(AcpTsp_AuthReadyACKMsg.ACKMsg_AuthToken.AuthToken));	//SKey(C)
			pos += sizeof(AcpTsp_AuthReadyACKMsg.ACKMsg_AuthToken.AuthToken);
			
			memcpy(p_FAWACPInfo_Handle->AuthToken, AcpTsp_AuthReadyACKMsg.ACKMsg_AuthToken.AuthToken, sizeof(p_FAWACPInfo_Handle->AuthToken));//AuthToken
			m_loginState = 2;//登陆成功标志
			m_bEnableSendDataFlag = true;//发送使能
			time(&HighReportTime);
			time(&LowReportTime);
			//检测是否有重发的数据
			if(TimeOutType.RemoteCtrlFlag == 2)
			{
				FAWACPLOG("TimeOut RemoteCtrlFlag");
				RespondTspPacket(ACPApp_RemoteCtrlID, 1, AcpCryptFlag_IS, TimeOutType.RemoteCtrlTspSource);
				delete_link(&Headerlink, ACPApp_RemoteCtrlID);
			}
			if(TimeOutType.VehQueCondFlag == 2)
			{
				FAWACPLOG("TimeOut VehQueCondFlag");
				RespondTspPacket(ACPApp_QueryVehicleCondID, 1, AcpCryptFlag_IS, TimeOutType.VehQueCondSource);
				delete_link(&Headerlink, ACPApp_QueryVehicleCondID);
			}
			if(TimeOutType.RemoteCpnfigFlag == 2)
			{
				FAWACPLOG("TimeOut RemoteCpnfigFlag");
				RespondTspPacket(ACPApp_RemoteConfigID, 1, AcpCryptFlag_IS, TimeOutType.RemoteCpnfigSource);
				delete_link(&Headerlink, ACPApp_RemoteConfigID);
			}
			if(TimeOutType.UpdateRootkeyFlag == 2)
			{
				mcuUart::m_mcuUart->packProtocolData(TBOX_ROOTKRY_RESULT, MCU_INFROM_ROOTKEY, NULL, -1, mcuUart::m_mcuUart->RootkeyserialNumber);
				FAWACPLOG("TimeOut UpdateRootkeyFlag");
				RespondTspPacket(ACPApp_UpdateKeyID, 3, AcpCryptFlag_IS, TimeOutType.UpdateRootkeySource);
				delete_link(&Headerlink, ACPApp_UpdateKeyID);
			}
			if(TimeOutType.VehGPSFlag == 2)
			{
				FAWACPLOG("TimeOut VehGPSFlag");
				RespondTspPacket(ACPApp_GPSID, 1, AcpCryptFlag_IS, TimeOutType.VehGPSSource);
				delete_link(&Headerlink, ACPApp_GPSID);
			}
			if(TimeOutType.RemoteDiagnoFlag == 2)
			{
				FAWACPLOG("TimeOut RemoteDiagnoFlag");
				RespondTspPacket(ACPApp_RemoteDiagnosticID, 1, AcpCryptFlag_IS, TimeOutType.RemoteDiagnoSource);
				delete_link(&Headerlink, ACPApp_RemoteDiagnosticID);
			}
			memset(&TimeOutType, 0, sizeof(TimeOutType_t));

			FAWACPLOG("login success !!!\n\n\n");
			break;
		default:
			break;
	}
	return 0;
}

/*******unpack远程控制*******/
uint16_t CFAWACP::UnpackData_AcpRemoteCtrl(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t MsgType)
{
	uint8_t *pos = PayloadBuff;
	uint16_t DataLen = 0;
	uint8_t RequestSource = 0;
	//时间戳部分
	ShowdataTime(pos);
	pos += DATA_TIMESTAMP_SIZE;
	//AuthToken
	pos++;
	memcpy(AuthToken, pos, sizeof(AuthToken));
	if( memcmp(p_FAWACPInfo_Handle->AuthToken, AuthToken, sizeof(AuthToken)) != 0)
		FAWACPLOG("AuthToken is fault !");
	pos += sizeof(AuthToken);
	//Source
	pos++;
	TspctrlSource = *pos++;
	FAWACPLOG("AcpRemoteCtrl analysis:");
	switch(MsgType)
	{
		case 1:
			if(search(Headerlink, ACPApp_RemoteCtrlID) != NULL)
				return 0;
			//数据长度
			pos += unpackDataLenDeal(pos, DataLen);
			FAWACPLOG("DataLen == %d\n",DataLen);
			//Request Source 0:调试系统1:管理员 Portal 2:用户 Portal 3:手机 APP
			RequestSource = *pos++;
			FAWACPLOG("Request Message Source -- %d\n",RequestSource);

			memset(&m_AcpRemoteCtrlList, 0, sizeof(AcpRemoteControl_RawData));
			m_AcpRemoteCtrlList.SubitemTotal = *pos++;
			FAWACPLOG("Subitem total == %d",m_AcpRemoteCtrlList.SubitemTotal);
			for(uint16_t i = 0; i < m_AcpRemoteCtrlList.SubitemTotal; i++)
			{
				m_AcpRemoteCtrlList.SubitemCode[i] = *pos++;
				FAWACPLOG("Subitem Code == %d",m_AcpRemoteCtrlList.SubitemCode[i]);
				switch(m_AcpRemoteCtrlList.SubitemCode[i])
				{
					case VehicleBody_LockID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleBody.VehicleBody_Lock = *pos++;
						FAWACPLOG("VehicleBody_Lock == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case VehicleBody_WindowID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleBody.VehicleBody_Window = *pos++;
						FAWACPLOG("VehicleBody_Window == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case VehicleBody_SunroofID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleBody.VehicleBody_Sunroof = *pos++;
						FAWACPLOG("VehicleBody_Sunroof == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case VehicleBody_TrackingCarID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleBody.VehicleBody_TrackingCar = *pos++;
						FAWACPLOG("VehicleBody_TrackingCar == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case VehicleBody_LowbeamID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleBody.VehicleBody_Lowbeam = *pos++;
						FAWACPLOG("VehicleBody_Lowbeam == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case VehicleBody_LuggageCarID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleBody.VehicleBody_LuggageCar = *pos++;
						FAWACPLOG("VehicleBody_LuggageCar == %d\n",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;					
					case Airconditioner_ControlID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.Airconditioner.Airconditioner_Control = *pos++;
						FAWACPLOG("Airconditioner_Control == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case Airconditioner_CompressorSwitchID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.Airconditioner.Airconditioner_CompressorSwitch = *pos++;
						FAWACPLOG("Airconditioner_CompressorSwitch == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case Airconditioner_TemperatureID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.Airconditioner.Airconditioner_Temperature.BitType = *pos++;
						FAWACPLOG("Airconditioner_Temperature == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case Airconditioner_SetAirVolumeID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.Airconditioner.Airconditioner_SetAirVolume.BitType = *pos++;
						FAWACPLOG("Airconditioner_SetAirVolume == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case Airconditioner_FrontDefrostSwitchID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.Airconditioner.Airconditioner_FrontDefrostSwitch = *pos++;
						FAWACPLOG("Airconditioner_FrontDefrostSwitch == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case Airconditioner_HeatedrearID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.Airconditioner.Airconditioner_Heatedrear = *pos++;
						FAWACPLOG("Airconditioner_Heatedrear == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case Airconditioner_BlowingModeID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.Airconditioner.Airconditioner_BlowingMode.BitType = *pos++;
						FAWACPLOG("Airconditioner_BlowingMode == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case Airconditioner_InOutCirculateID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.Airconditioner.Airconditioner_InOutCirculate.BitType = *pos++;
						FAWACPLOG("Airconditioner_InOutCirculate == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case Airconditioner_AutoSwitchID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.Airconditioner.Airconditioner_AutoSwitch = *pos++;
						FAWACPLOG("Airconditioner_AutoSwitch == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case EngineState_SwitchID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.EngineState.EngineState_Switch = *pos++;
						FAWACPLOG("EngineState_Switch == %d",m_AcpRemoteCtrlList.SubitemValue[i]);				
						break;
					case VehicleSeat_DrivingSeatID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleSeat.VehicleSeat_DrivingSeat = *pos++;
						FAWACPLOG("VehicleSeat_DrivingSeat == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case VehicleSeat_CopilotseatID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleSeat.VehicleSeat_Copilotseat = *pos++;	
						FAWACPLOG("VehicleSeat_Copilotseat == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case VehicleSeat_DrivingSeatMomeryID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleSeat.VehicleSeat_DrivingSeatMemory = *pos++;
						FAWACPLOG("VehicleSeat_DrivingSeatMemory == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case VehicleChargeMode_ImmediateID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleCharge.VehicleCharge_Immediate = *pos++;
						FAWACPLOG("VehicleCharge_Immediate == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case VehicleChargeMode_AppointmentID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleCharge.VehicleCharge_Appointment = *pos++;
						FAWACPLOG("VehicleCharge_Appointment == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case VehicleChargeMode_EmergencyCharg:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleCharge.VehicleCharge_EmergenCharg = *pos++;
						FAWACPLOG("VehicleCharge_EmergenCharg == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case VehicleWIFIStatusID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleAutopark.VehicleWifiStatus = *pos++;
						FAWACPLOG("VehicleWifiStatus == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					case VehicleAutoOUTID:
						m_AcpRemoteCtrlList.SubitemValue[i] = m_AcpRemoteCtrlCommandData.VehicleAutopark.VehicleAutoOut = *pos++;
						FAWACPLOG("VehicleAutoOut == %d",m_AcpRemoteCtrlList.SubitemValue[i]);
						break;
					default:
						break;
				}
			}
			reportRemoteCtrlCmd(mcuUart::m_mcuUart->cb_RemoteCtrlCmd);
			break;
		case 0:
			if(search(Headerlink, ACPApp_RemoteCtrlID) != NULL)
			{
				pthread_mutex_lock(&mutex);
				delete_link(&Headerlink, ACPApp_RemoteCtrlID);
				pthread_mutex_unlock(&mutex);
				TimeOutType.RemoteCtrlFlag = 0;
			}
			FAWACPLOG("AcpRemoteControl ack is ok!\n");
			break;
		default:
			break;
	}
	return 0;
}

//回调函数控制MCU发送远程控制指定
void CFAWACP::reportRemoteCtrlCmd(callback_RemoteCtrlCmd cb_RemoteCtrlCmd)
{
	FAWACPLOG("CFAWACP: reportRemoteCtrlCmd");
	cb_RemoteCtrlCmd();
}

/*******unpack车况查询********/
uint16_t CFAWACP::UnpackData_AcpQueryVehicleCond(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t applicationID, uint8_t MsgType)
{
	uint8_t *pos = PayloadBuff;
	uint16_t DataLen = 0;
	uint8_t RequestSource = 0;
	uint8_t TspSourceID   =	0;
	//时间戳部分
	ShowdataTime(pos);
	pos += DATA_TIMESTAMP_SIZE;
	//AuthToken
	pos++;
	memcpy(AuthToken, pos, sizeof(AuthToken));
	if( memcmp(p_FAWACPInfo_Handle->AuthToken, AuthToken, sizeof(AuthToken)) != 0)
		FAWACPLOG("AuthToken is fault !");
	pos += sizeof(AuthToken);
	//Source
	pos++;
	TspSourceID = *pos++;

	switch(MsgType)
	{
		case 1:
			if(search(Headerlink, applicationID) != NULL)
				return 0;
			//数据长度 DataLen
			pos += unpackDataLenDeal(pos,DataLen);
			//Request Source 0:调试系统1:管理员 Portal 2:用户 Portal 3:手机 APP
			RequestSource = *pos++;
			FAWACPLOG("Request Message Source -- %d\n",RequestSource);
			m_AcpVehicleCondCommandList.CollectTime = ((pos[0] << 8) & 0xFF00) | (pos[1] & 0x00FF);//采集时间间隔
			pos += 2;
			m_AcpVehicleCondCommandList.CollectCount = ((pos[0] << 8) & 0xFF00) | (pos[1] & 0x00FF);//采集次数
			pos += 2;
			m_AcpVehicleCondCommandList.SignTotal = ((pos[0] << 8) & 0xFF00) | (pos[1] & 0x00FF);//采集信号总数
			pos += 2;

			RespondTspPacket((AcpAppID_E )applicationID, MsgType+1, AcpCryptFlag_IS, TspSourceID);
			break;
		case 0:
			if(search(Headerlink, applicationID) != NULL)
			{	
				pthread_mutex_lock(&mutex);
				delete_link(&Headerlink, applicationID);
				pthread_mutex_unlock(&mutex);
				if(applicationID == ACPApp_GPSID)
				{
					TimeOutType.VehGPSFlag = 0;
				}
				else
				{
					TimeOutType.VehQueCondFlag = 0;
				}
			}
			FAWACPLOG("AcpQueryVehicleCond ack is OK！ \n");
			break;
		default:
			break;
	}
	return 0;
}


/*******unpack远程配置*******/
uint16_t CFAWACP::UnpackData_AcpRemoteConfig(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t MsgType)
{
	uint8_t *pos = PayloadBuff;
	uint16_t DataLen = 0;
	uint8_t RequestSource = 0;
	uint8_t TspSourceID   =	0;
	//时间戳部分
	ShowdataTime(pos);
	pos += DATA_TIMESTAMP_SIZE;
	//AuthToken
	pos++;
	memcpy(AuthToken, pos, sizeof(AuthToken));
	if( memcmp(p_FAWACPInfo_Handle->AuthToken, AuthToken, sizeof(AuthToken)) != 0)
		FAWACPLOG("AuthToken is fault !");
	pos += sizeof(AuthToken);
	//Source
	pos++;
	TspSourceID = *pos++;
	//数据长度
	switch(MsgType)
	{
		case 1:
			if(search(Headerlink, ACPApp_RemoteConfigID) != NULL)
				return 0;
			//数据长度 DataLen
			pos += unpackDataLenDeal(pos,DataLen);
			//Request Source 0:调试系统1:管理员 Portal 2:用户 Portal 3:手机 APP
			RequestSource = *pos++;
			FAWACPLOG("Request Message Source -- %d\n",RequestSource);

			m_AcpRemoteConfig.DeviceTotal = *pos++;	//设备总数
			FAWACPLOG("Device Total == %d\n",m_AcpRemoteConfig.DeviceTotal);
			for(uint8_t i = 0; i < m_AcpRemoteConfig.DeviceTotal; i++)
			{
				m_AcpRemoteConfig.DeviceConfig[i].DeviceNo = ((pos[0] << 8) & 0xFF00) | (pos[1] & 0x00FF);
				pos += 2;
				FAWACPLOG("DeviceNo == %d\n",m_AcpRemoteConfig.DeviceConfig[i].DeviceNo);
				m_AcpRemoteConfig.DeviceConfig[i].ConfigCount = *pos++;//设备配置数量
				FAWACPLOG("ConfigCount == %d\n",m_AcpRemoteConfig.DeviceConfig[i].ConfigCount);
				for(uint8_t k = 0; k < m_AcpRemoteConfig.DeviceConfig[i].ConfigCount; k++)
				{
					m_AcpRemoteConfig.DeviceConfig[i].ConfigItem[k].ConfigItemNum = *pos++;//远程配置项编码
					FAWACPLOG("ConfigItemNum == %d\n",m_AcpRemoteConfig.DeviceConfig[i].ConfigItem[k].ConfigItemNum);
					switch(m_AcpRemoteConfig.DeviceConfig[i].ConfigItem[k].ConfigItemNum)
					{
						case ACPCfgItem_EngineStartTimeID:
							//发动机远程启动时长 
							p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].EngineStartTime = ((pos[0] << 8) & 0xFF00) | (pos[1] & 0x00FF);
							pos += 2;
							FAWACPLOG("EngineStartTime == %d\n",p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].EngineStartTime);
							switch(i)
							{
								case 0:	//tbox
									reportRemoteConfigCmd(mcuUart::m_mcuUart->cb_RemoteConfigCmd, 0x01, p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].EngineStartTime);
									break;
								default:
									break;
							}
							break;
						case ACPCfgItem_SamplingSwitchID:
							//电动车国标抽检开关
							p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].SamplingSwitch = *pos++;
							FAWACPLOG("SamplingSwitch == %d\n",p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].SamplingSwitch);
							break;
						case ACPCfgItem_EmergedCallID:
							//紧急服务号码配置(E-CALL)
							memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].EmergedCall,pos,15);
							dataPool->setPara(E_CALL_ID, (void *)p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].EmergedCall, strlen(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].EmergedCall));	
							FAWACPLOG("EmergedCall == %s\n",p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].EmergedCall);
							pos += 15;
							break;
						case ACPCfgItem_WhitelistCallID:
							//白名单号码配置 
							memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].WhitelistCall,pos,15);
							FAWACPLOG("WhitelistCall == %s\n",p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].WhitelistCall);
							pos += 15;
							break;
						case ACPCfgItem_CollectTimeIntervalID:
							//车况定时采集间隔时间
							p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].CollectTimeInterval = *pos++;
							FAWACPLOG("CollectTimeInterval == %d\n",p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].CollectTimeInterval);
							break;
						case ACPCfgItem_ReportTimeIntervalID:
							//车况定时上报间隔时间
							p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].ReportTimeInterval = *pos++;
							dataPool->setPara(CollectReportTime_Id, &p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].ReportTimeInterval, sizeof(int));
							FAWACPLOG("ReportTimeInterval == %d\n",p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].ReportTimeInterval);
							switch(i)
							{
								case 0:	//tbox
									reportRemoteConfigCmd(mcuUart::m_mcuUart->cb_RemoteConfigCmd, 0x02, p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].CollectTimeInterval);
									break;
								default:
									break;
							}
							break;
						case ACPCfgItem_CollectTimeGpsSpeedIntervalID:
							//位置和车速定时采集间隔时间
							p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].CollectTimeGpsSpeedInterval = *pos++;
							FAWACPLOG("CollectTime GpsSpeed Interval == %d\n",p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].CollectTimeGpsSpeedInterval);
							break;
						case ACPCfgItem_ReportTimeGpsSpeedIntervalID:
							//位置和车速定时上报间隔时间
							p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].ReportTimeGpsSpeedInterval = *pos++;
							dataPool->setPara(GpsSpeedReportTime_Id, &p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].ReportTimeGpsSpeedInterval, sizeof(int));
							FAWACPLOG("ReportTime GpsSpeed Interval == %d\n",p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].ReportTimeGpsSpeedInterval);
							break;
						case ACPCfgItem_ChargeModeID:
							//充电模式
							p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].ChargeMode = *pos++;
							FAWACPLOG("ChargeMode == %d\n",p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].ChargeMode);
							break;
						case ACPCfgItem_ChargeScheduleID:
							//预约充电时间表
							p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].ChargeSchedule.ScheduChargStartData = *pos++;//预约充电日
							p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].ChargeSchedule.ScheduChargStartTime = ((pos[0] << 8) & 0xFF00) | (pos[1] & 0x00FF);//预约充电开始时间
							pos += 2;
							p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].ChargeSchedule.ScheduChargEndData = *pos++;//预约充电日
							p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].ChargeSchedule.ScheduChargEndTime = ((pos[0] << 8) & 0xFF00) | (pos[1] & 0x00FF);//充电持续时间
							pos += 2;
							break;
						case ACPCfgItem_LoadCellID:	//B-Call号码
							memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].LoadCell,pos,15);
							dataPool->setPara(B_CALL_ID, (void *)p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].LoadCell, strlen(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].LoadCell));	
							FAWACPLOG("LoadCell == %s\n",p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].LoadCell);
							pos += 15;
							break;
						case ACPCfgItem_InformationCellID: //I-Call号码
							memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].InformationCell,pos,15);
							dataPool->setPara(I_CALL_ID, (void *)p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].InformationCell, strlen(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[i].InformationCell));
							pos += 15;
							break;
						default:
							break;
					}
				}				
			}

			RespondTspPacket(ACPApp_RemoteConfigID, MsgType+1, AcpCryptFlag_IS, TspSourceID);
			break;
		case 0:
			if(search(Headerlink, ACPApp_RemoteConfigID) != NULL)
			{
				pthread_mutex_lock(&mutex);
				delete_link(&Headerlink, ACPApp_RemoteConfigID);
				pthread_mutex_unlock(&mutex);
				TimeOutType.RemoteCpnfigFlag = 0;
			}
			FAWACPLOG("AcpRemoteConfig ack is ok!\n");
			break;
		default:
			break;
	}
	return 0;
}
//回调MCU发送远程配置功能到8090
void CFAWACP::reportRemoteConfigCmd(callback_RemoteConfigCmd cb_RemoteConfigCmd, uint8_t ConfigItemNum, uint16_t ConfigValue)
{
	cb_RemoteConfigCmd(ConfigItemNum, ConfigValue);
}
/*******unpack RootkeyData********/
uint16_t CFAWACP::UnpackData_AcpUpdateRootKey(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t MsgType)
{
	uint8_t *pos = PayloadBuff;
	uint16_t DataLen = 0;
	uint8_t RequestSource = 0;
	uint8_t TspSourceID   =	0;
	uint8_t New_RootKey[32] = {0};
	uint8_t EcrpytRootKey[16] = {0};
	//时间戳部分
	ShowdataTime(pos);
	pos += DATA_TIMESTAMP_SIZE;
	//AuthToken
	pos++;
	memcpy(AuthToken, pos, sizeof(AuthToken));
	if(memcmp(p_FAWACPInfo_Handle->AuthToken, AuthToken, sizeof(AuthToken)) != 0)
		FAWACPLOG("AuthToken is fault !");
	pos += sizeof(AuthToken);
	//Source
	pos++;
	TspSourceID = *pos++;
	switch(MsgType)
	{
		case 2:
			if(search(Headerlink, ACPApp_UpdateKeyID) != NULL)
				return 0;
			memset(p_FAWACPInfo_Handle->New_RootKey, 0, sizeof(p_FAWACPInfo_Handle->New_RootKey));
			m_bEnableSendDataFlag = false;//发送使能标识
			//数据长度 DataLen
			pos += unpackDataLenDeal(pos,DataLen);
			//Request Source 0:调试系统1:管理员 Portal 2:用户 Portal 3:手机 APP
			RequestSource = *pos++;
			//加密芯片解密新Rootkey
			memcpy(EcrpytRootKey, pos, 16);
			decryptTbox_Data(EcrpytRootKey, 1, p_FAWACPInfo_Handle->New_RootKey);
			pos += 32;

			RespondTspPacket(ACPApp_UpdateKeyID, MsgType + 1, AcpCryptFlag_IS, TspSourceID);//RootKey更新回应
			break;
		case 0:
			if(search(Headerlink, ACPApp_UpdateKeyID) != NULL)
			{
				pthread_mutex_lock(&mutex);
				delete_link(&Headerlink, ACPApp_UpdateKeyID);
				pthread_mutex_unlock(&mutex);
				TimeOutType.UpdateRootkeyFlag = 0;
			}
			memcpy(&New_RootKey[0], p_FAWACPInfo_Handle->New_RootKey, sizeof(p_FAWACPInfo_Handle->New_RootKey));
			memcpy(&New_RootKey[16], p_FAWACPInfo_Handle->New_RootKey, sizeof(p_FAWACPInfo_Handle->New_RootKey));
			writeTsp_rootkey(New_RootKey, 32);

			m_bEnableSendDataFlag = true;
			mcuUart::m_mcuUart->packProtocolData(TBOX_ROOTKRY_RESULT, MCU_INFROM_ROOTKEY, NULL, 1, mcuUart::m_mcuUart->RootkeyserialNumber);
			FAWACPLOG("UpdateRootKey ack is ok! \n");
			break;
		default:
			break;
	}
	return 0;
}

/*******unpack远程升级********/
uint16_t CFAWACP::UnpackData_AcpRemoteUpgrade(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t MsgType)
{
	uint8_t *pos = PayloadBuff;
	uint16_t DataLen = 0;
	uint8_t RequestSource = 0;
	uint8_t TspSourceID   =	0;
	//时间戳部分
	ShowdataTime(pos);
	pos += DATA_TIMESTAMP_SIZE;
	//AuthToken
	pos++;
	memcpy(AuthToken, pos, sizeof(AuthToken));
	if( memcmp(p_FAWACPInfo_Handle->AuthToken, AuthToken, sizeof(AuthToken)) != 0)
		FAWACPLOG("AuthToken is fault !");
	pos += sizeof(AuthToken);
	//Source
	pos++;
	TspSourceID = *pos++;
	switch(MsgType)
	{
		case 1:
			//数据长度 DataLen
			pos += unpackDataLenDeal(pos,DataLen);
			//Request Source 0:调试系统1:管理员 Portal 2:用户 Portal 3:手机 APP
			RequestSource = *pos++;
			FAWACPLOG("Request Message Source -- %d\n",RequestSource);

			m_AcpRemoteUpgrade.DeviceNo = *pos++;
			m_AcpRemoteUpgrade.NeedCheck = *pos++;
			memcpy(m_AcpRemoteUpgrade.UpdateFile_MD5, pos, 16);
			pos += 16;
			m_AcpRemoteUpgrade.UpdateFile_Len = *pos++;//升级文件名称长度
			FAWACPLOG("UpdateFile_Len：%d\n",m_AcpRemoteUpgrade.UpdateFile_Len);
			memcpy(m_AcpRemoteUpgrade.UpdateFile_Param, pos, m_AcpRemoteUpgrade.UpdateFile_Len);//升级文件名称
			pos += m_AcpRemoteUpgrade.UpdateFile_Len;
			m_AcpRemoteUpgrade.Ver_Len = *pos++;//版本
			FAWACPLOG("Ver_Len：%d\n",m_AcpRemoteUpgrade.Ver_Len);
			memcpy(m_AcpRemoteUpgrade.VerNo, pos, m_AcpRemoteUpgrade.Ver_Len);
			pos += m_AcpRemoteUpgrade.Ver_Len;
			m_AcpRemoteUpgrade.URL_Len = *pos++;
			FAWACPLOG("URL_Len：%d\n",m_AcpRemoteUpgrade.URL_Len);
			memcpy(m_AcpRemoteUpgrade.URL_Param, pos, m_AcpRemoteUpgrade.URL_Len);
			pos += m_AcpRemoteUpgrade.URL_Len;
			
			//回应升级命令接收状态
			RespondTspPacket(ACPApp_RemoteUpgradeID, MsgType+1, AcpCryptFlag_IS, TspSourceID);

			/*if()//用户确认，下载开始
			{
				RespondTspPacket(ACPApp_RemoteUpgradeID, 3, AcpCryptFlag_IS, 0);
				//根据URL下载升级文件(FTPS下载)
				for(uint8_t i = 0; i < 3; i++)
				{
					Upgrade_DownloadState = FTPS_Download();
					if(Upgrade_DownloadState == 1)//下载成功
						break;
				}
				if(1 == Upgrade_DownloadState)
				{
					//发送下载结果(TCP ACk)
					Upgrade_DownloadState = 0;
					RespondTspPacket(ACPApp_RemoteUpgradeID, 4, AcpCryptFlag_IS, 0);
					//发送升级开始升级(TCP ACk)
					RespondTspPacket(ACPApp_RemoteUpgradeID, 5, AcpCryptFlag_IS, 0);
					//断开socket，完成升级，重启，建立连接认证，发送升级结果
					//发送升级升级结果

					RespondTspPacket(ACPApp_RemoteUpgradeID, 6, AcpCryptFlag_IS, 0);
				}
				else
				{
					Upgrade_DownloadState = 4;
					RespondTspPacket(ACPApp_RemoteUpgradeID, 4, AcpCryptFlag_IS, 0);
					FAWACPLOG("下载失败 错误参数 == %d\n",Upgrade_DownloadState);
				}
			}
			else
			{
				Upgrade_Result = 1;
				RespondTspPacket(ACPApp_RemoteUpgradeID, 6, AcpCryptFlag_IS, 0);
			}*/
			break;
		case 0:
			FAWACPLOG("AcpRemoteUpgrade ACK is OK！\n");
			break;
		default:
			break;
	}
	return 0;
}

/*******unpack远程诊断********/
uint16_t CFAWACP::UnpackData_AcpRemoteDiagnostic(uint8_t *PayloadBuff, uint16_t payload_size, uint8_t MsgType)
{
	uint8_t *pos = PayloadBuff;
	uint16_t DataLen = 0;
	uint8_t RequestSource = 0;
	uint8_t TspSourceID   =	0;
	//时间戳部分
	ShowdataTime(pos);
	pos += DATA_TIMESTAMP_SIZE;
	//AuthToken
	pos++;
	memcpy(AuthToken, pos, sizeof(AuthToken));
	if( memcmp(p_FAWACPInfo_Handle->AuthToken, AuthToken, sizeof(AuthToken)) != 0)
		FAWACPLOG("AuthToken is fault !");
	pos += sizeof(AuthToken);
	//Source
	pos++;
	TspSourceID = *pos++;
	switch(MsgType)
	{
		case 1:
			if(search(Headerlink, ACPApp_RemoteDiagnosticID) != NULL)
				return 0;
			//数据长度 DataLen
			pos += unpackDataLenDeal(pos,DataLen);
			//Request Source 0:调试系统1:管理员 Portal 2:用户 Portal 3:手机 APP
			RequestSource = *pos++;
			FAWACPLOG("Request Message Source -- %d\n",RequestSource);

			m_RemoteDiagnose.DiagnoseTotal = *pos++;	//请求诊断控制器总数目
			for(uint8_t i = 0; i < m_RemoteDiagnose.DiagnoseTotal; i++)
				m_RemoteDiagnose.DiagnoseCode[i] = *pos++;	//诊断控制器编码

			//执行诊断动作,诊断结果由m_RemoteDiagnoseResult保存
			RespondTspPacket(ACPApp_RemoteDiagnosticID, MsgType+1, AcpCryptFlag_IS, TspSourceID);
			break;
		case 0:
			if(search(Headerlink, ACPApp_RemoteDiagnosticID) != NULL)
			{
				pthread_mutex_lock(&mutex);
				delete_link(&Headerlink, ACPApp_RemoteDiagnosticID);
				pthread_mutex_unlock(&mutex);
				TimeOutType.RemoteDiagnoFlag = 0;
			}
			FAWACPLOG("AcpRemoteDiagnostic ack is ok!\n");
			break;
		default:
			break;
	}
	return 0;
}


/***********************加密解密相关处理*****************************************/
//HMACMD5算法
void CFAWACP::HMACMd5_digest(const char *SecretKey, unsigned char *Content, uint16_t InLen, uint8_t OutBuff[16])
{
	int i ,j;
	unsigned char *tempBuffer = (unsigned char *)malloc(MAX_FILE + 64); //第一次HASH的参数
	unsigned char Buffer2[80]; //第二次HASH
	unsigned char key[16];
	unsigned char ipad[64], opad[64];

	if(strlen(SecretKey) > 16 )
		m_Hmacmd5OpData.md5_digest(SecretKey, strlen(SecretKey), key);
	else if(strlen(SecretKey) < 16 )
	{
		i=0;
		while(SecretKey[i]!='\0')
		{
			key[i]=SecretKey[i];
			i++;
		}
		while(i<16)
		{
			key[i]=0x00;
			i++;
		}
	}
	else
	{
		for(i=0;i<16;i++)
		key[i]=SecretKey[i];
	}
	 
	for(i=0;i<64;i++)
	{
		ipad[i]=0x36;
		opad[i]=0x5c;
	}
	 
	for(i=0;i<16;i++)
	{
		ipad[i]=key[i] ^ ipad[i];   //K ⊕ ipad
		opad[i]=key[i] ^ opad[i];   //K ⊕ opad
	}
	 
	for(i=0;i<64;i++)
		tempBuffer[i]=ipad[i];
	for(i=64;i<InLen+64;i++)
		tempBuffer[i]=Content[i-64];
	 
	m_Hmacmd5OpData.md5_digest(tempBuffer, InLen+64, OutBuff);
	 
	for(j=0; j < 64; j++)
		Buffer2[j] = opad[j];
	for(i = 64; i< 80; i++)
		Buffer2[i] = OutBuff[i-64];
	 
	m_Hmacmd5OpData.md5_digest(Buffer2, 80, OutBuff);
}



