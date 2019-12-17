#include "GBT32960.h"
#include "GpioWake.h"
#include <stdlib.h>
#include "mcuUart.h"



#define SAVE_PERIOD 604800		//15 days
#define PORT 		20060
#define IP			"120.77.176.145"


//#define PORT 		8877
//#define IP			"171.217.92.76"


//#define PORT 		19007
//#define IP			"123.127.164.36"


time_t testTime;

int faultLevel = 0;
bool alarmlever = false;
int alarmcount =0;
GBT32960_handle gbt32960Handle;
GBT32960_handle *p_GBT32960_handle = &gbt32960Handle;
extern pthread_mutex_t Goldenlock;
int closeTbox = 0;
bool resend = false;
bool reissue = false;

bool testnet = true;
bool testout = true;

/*****************************************************************************
* Function Name : GBT32960
* Description   : 构造函数
* Input			: None
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
GBT32960::GBT32960()
{
	
	

	printf("version : %s\n", VER_GOLDEN);
	socketfd = -1;
	isConnected = false;
	recvFlag = -1;
	timeoutTimes = 0;
	loginState = 0;
	ReportingFlagCount = 0;
	SettoLoginState = 0;
	controltime[6] = {0};
	HighReportTime = 0;
	LowReportTime = 0;
	NowHighReportTime = 0;
	NowLowReportTime = 0;
	testTime = 0;
	if(readTBoxParameterInfo() == -1)
	{
		GBTLOG("readTBoxParameterInfo is fail\n");
		initTBoxParameterInfo();
	}
	dataPool->getTboxConfigInfo(VinID, p_GBT32960_handle->dataInfo.vin, sizeof(p_GBT32960_handle->dataInfo.vin));

	while(1)
	{
		if(p_GBT32960_handle->dataInfo.positionInfo.positionValid.bitState.valid || tboxInfo.networkStatus.isLteNetworkAvailable != NETWORK_LTE)
			sleep(1);
		else
		{
			printf("okokokokokokoko\n\n");
			break;
		}
		
	}
	socketConnect();	//连接服务器
	

	/*int ret;
	FILE *fp;
	char buff[2];
	memset(buff, 0, sizeof(buff));
	fp = popen("ps -A|grep tbox |grep -v grep |wc -l","r");
	if(fread(buff, sizeof(char), sizeof(buff), fp) > 0)
	{
		ret = atoi(buff);
	}
	else
	{
		ret = 0;
	}
		
	//printf("ret:%d\n",ret);
	//pclose(fp);
	
	if(ret == 1)
	{	
			//FILE *fp;
		fp = popen("/data/tbox","r");
		pclose(fp);
	}
	else{
		pclose(fp);
	}*/



	if(pthread_create(&receiveThreadId, NULL, receiveThread, this) != 0)
		GBTLOG("Cannot creat receiveThread:%s\n", strerror(errno));
	
	sleep(10);
	
	if(pthread_create(&timeoutThreadId, NULL, timeoutThread, this) != 0)
		GBTLOG("Cannot creat receiveThread:%s\n", strerror(errno));

	while(1)
	{
		//GBTLOG("time 1 = %d  time 2 = %d\n",p_GBT32960_handle->configInfo.timingReportTimeInterval,p_GBT32960_handle->configInfo.heartbeatInterval);
		if(p_GBT32960_handle->configInfo.timingReportTimeInterval<=1)
			p_GBT32960_handle->configInfo.timingReportTimeInterval = 10;
		if(p_GBT32960_handle->configInfo.heartbeatInterval<=1)
			p_GBT32960_handle->configInfo.heartbeatInterval = 30;
		usleep(100000);
		if(!testnet)
		{
			//disconnectSocket();
			
			close(socketfd);
			isConnected = false;
			resend = false;
			sleep(5);
			continue;
			
		}
		if(isConnected==false||loginState==0||ReportingFlagCount!=0)
		{
			resend = false;
		}
		if(isConnected == true)
		{
			if(loginState == 0)
			{			
				GBTLOG("loginState is fail timeoutTimes:%d\n\n\n",timeoutTimes);
				GBTLOG("logonFailureInterval:%d\n",p_GBT32960_handle->configInfo.logonFailureInterval);
		    	GBTLOG("loginTimeInterval:%d \n\n",p_GBT32960_handle->configInfo.loginTimeInterval);
				if(timeoutTimes == 3)
				{
					GBTLOG("login three timeouts");
					timeoutTimes = 0;
					sleep(p_GBT32960_handle->configInfo.logonFailureInterval);
					sendLoginData();
				}
				else
				{
					GBTLOG("login less than three timeouts");
				    sleep(p_GBT32960_handle->configInfo.loginTimeInterval);
					sendLoginData();
				}
			}
			else if(loginState == 1)
			{
				time(&NowHighReportTime);
				time(&NowLowReportTime);
				
				
				if((faultLevel >= 0 && faultLevel < 3) || alarmlever)
				{
					if(faultLevel >= 0 && faultLevel < 3)
						alarmlever = false;

					
					
					if(NowLowReportTime - LowReportTime >= p_GBT32960_handle->configInfo.timingReportTimeInterval)
					{
						if(ReportingFlagCount == 0)//已上报
						{
							GBTLOG("Real-time reporting\n");
							timingReport(GBT32960_RealTimeInfoReportID);
							time(&LowReportTime);
							//if(p_GBT32960_handle->configInfo.timingReportTimeInterval>20)
							{
								timingReport(GBT32960_ReissueInfoReportID);
							}
							//timingReport(GBT32960_ReissueInfoReportID);
							//sleep(p_GBT32960_handle->configInfo.timingReportTimeInterval);
							

						}
						else if(ReportingFlagCount <= 3)//上报失败
						{
							timingReport(GBT32960_RealTimeInfoReportID);
							time(&LowReportTime);
							//if(p_GBT32960_handle->configInfo.timingReportTimeInterval>20)
							{
								timingReport(GBT32960_ReissueInfoReportID);
							}
							//timingReport(GBT32960_ReissueInfoReportID);
							sleep(2 * p_GBT32960_handle->configInfo.timingReportTimeInterval);
							if(ReportingFlagCount == 3)
							{
								ReportingFlagCount = 0;
							}
						}
					}
					if(faultLevel == 3)
					{
							if(NowLowReportTime - testTime >=1)
						{
							//printf("))))))))))))))))))))\n");
							time(&testTime);
							timingReport(GBT32960_ReissueInfoReportID);
						}
					}
					
				
					
				}
				else  //faultLevel: 3
				{
					if((faultLevel == 3) && (!alarmlever))
					{
						GBTLOG("three level alarm\n");
						//alarmReport();
						if(NowLowReportTime - LowReportTime >= 1)
						{
							timingReport(GBT32960_RealTimeInfoReportID);
							time(&LowReportTime);
							if(++alarmcount == 1)
								timingReport(GBT32960_ReissueInfoReportID);
							if(alarmcount == 30)
							{
								alarmlever = true;
								alarmcount =0;
								p_GBT32960_handle->configInfo.localStorePeriod = 30000;
							}
							GBTLOG("alarmcount is %d  \n",alarmcount );
						}
						
					}
				}
					if(NowHighReportTime - HighReportTime >= p_GBT32960_handle->configInfo.heartbeatInterval)
					{
						timingReport(GB32960_HeartBeatID);
						time(&HighReportTime);
					}
					if(NowLowReportTime < LowReportTime) 
					{
						time(&LowReportTime);
					}
					if(NowHighReportTime < HighReportTime)
					{
						time(&HighReportTime);
					}
			}
		}
		else
		{
			socketConnect();
			if(pthread_create(&receiveThreadId, NULL, receiveThread, this) != 0)
				GBTLOG("Cannot creat receiveThread:%s\n", strerror(errno));
			timeoutTimes = 0;
			sleep(30);
		}
	}

}

GBT32960::~GBT32960()
{

}
/*****************************************************************************
* Function Name : socketConnect
* Description   : 创建SOCKET
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int GBT32960::socketConnect()
{
	int fd;
	char ServerIP[16] = {0};
	int port;

#if 1
	//sleep used for waiting networking
	sleep(5);
	if(getServerIP(1, ServerIP, sizeof(ServerIP), NULL, (char *)"rmweb.gzchangjiangev.com") == -1)
		return -1;
//	port = 8877; //(int)SERVER_PORT;
	port = 8899;	//fpp 191106	
	printf("jason add ServerIP:%s port =%d \n",ServerIP,port);
#else
	if(getServerIP(0, ServerIP, sizeof(ServerIP), &port, NULL) == -1)
		return -1;
	printf("ServerIP:%s, port:%d\n",ServerIP, port);
	
#endif
	
	memset(&socketaddr, 0, sizeof(socketaddr));
	
#if 0
   socketaddr.sin_family = AF_INET;
	socketaddr.sin_port = htons(PORT);
	socketaddr.sin_addr.s_addr = inet_addr(IP);
#else
	socketaddr.sin_family = AF_INET;
//	socketaddr.sin_port = htons(8877);
	socketaddr.sin_port = htons(port);  //fpp 191107
	socketaddr.sin_addr.s_addr = inet_addr(ServerIP);
	
#endif

	if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) != -1)
	{
		GBTLOG("socketfd:%d\n",socketfd);
			printf("jason add server connect ^^^^^^^^^^^^^^^^^^^^^^^^^ \r\n");
		if (connect(socketfd, (struct sockaddr*) &socketaddr, sizeof(socketaddr)) != -1)
		{
			GBTLOG("Connect to server successfully,server IP:%s,PORT:%d\n",inet_ntoa(socketaddr.sin_addr),ntohs(socketaddr.sin_port));
			
			isConnected = true;
			resend = true;
			struct tm *p_tm = NULL;
			time_t tmp_time;
			tmp_time = time(NULL);
			p_tm = gmtime(&tmp_time);
			


			GBTLOG("Current TIME:%0d-%d-%d:%02d:%02d:%02d",p_tm->tm_year+1900, p_tm->tm_mon,p_tm->tm_mday, p_tm->tm_hour,p_tm->tm_min,p_tm->tm_sec);
			GBTLOG("Current date:%0d-%d-%d",p_tm->tm_year+1900, p_tm->tm_mon+1,p_tm->tm_mday);
			GBTLOG("SYS TIME:%0d-%d-%d",p_tm->tm_year, p_tm->tm_mon,p_tm->tm_mday);
			GBTLOG("FILE TIME:%0d-%d-%d",p_GBT32960_handle->statusInfo.year, p_GBT32960_handle->statusInfo.month,p_GBT32960_handle->statusInfo.today);

			fd = open(PARAMETER_INFO_PATH, O_RDWR, 777);
			if (-1 == fd)
			{
				printf("open file failed\n");
				return -1;
			}

			//set login serial number
			if ((p_GBT32960_handle->statusInfo.year == 0) && (p_GBT32960_handle->statusInfo.month == 0)
				 && (p_GBT32960_handle->statusInfo.today == 0))
			{
				p_GBT32960_handle->statusInfo.year = p_tm->tm_year;
				p_GBT32960_handle->statusInfo.month = p_tm->tm_mon;
				p_GBT32960_handle->statusInfo.today = p_tm->tm_mday;

				GBTLOG("serialNumber:%d", p_GBT32960_handle->dataInfo.serialNumber);
				p_GBT32960_handle->dataInfo.serialNumber++;
			}
			else if ((p_GBT32960_handle->statusInfo.year == p_tm->tm_year) &&
				     (p_GBT32960_handle->statusInfo.month == p_tm->tm_mon) &&
				     (p_GBT32960_handle->statusInfo.today == p_tm->tm_mday) &&
					 (p_GBT32960_handle->dataInfo.serialNumber <= 65531))
			{
				GBTLOG("serialNumber:%d", p_GBT32960_handle->dataInfo.serialNumber);
				p_GBT32960_handle->dataInfo.serialNumber++;
			}
			else
			{
				p_GBT32960_handle->statusInfo.year = p_tm->tm_year;
				p_GBT32960_handle->statusInfo.month = p_tm->tm_mon;
				p_GBT32960_handle->statusInfo.today = p_tm->tm_mday;
				GBTLOG("serialNumber:%d", p_GBT32960_handle->dataInfo.serialNumber);
				p_GBT32960_handle->dataInfo.serialNumber = 1;
			}  

			if (-1 == write(fd, p_GBT32960_handle, sizeof(GBT32960_handle)))
			{
				printf("write file error!\n");
				return -1;
			}
			
			close(fd);
			system("sync");

			sendLoginData();
		}
		else
		{
			printf("jason add server connect faid @@@@@@@@@@@@@@@ \r\n");
			close(socketfd);
			isConnected = false;
			resend = false;
			loginState = 0;
			GBTLOG("connect failed close socketfd!");
			return -1;
		}
	}

	return 0;
}

int GBT32960::closeConnect()
{
	close(socketfd);
}
/*****************************************************************************
* Function Name : getServerIP
* Description   : 获得服务器IP
* Input			: int getIPByDomainName  
*                 char *ip               
*                 int ip_size            
*                 int *port              
*                 char *domainName  
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int GBT32960::getServerIP(int getIPByDomainName, char *ip, int ip_size, int *port, char *domainName)
{
	if(getIPByDomainName == 0)
	{
		if((ip != NULL) && (port != NULL) && (ip_size >= 16))
		{
			uint8_t u8IPAndPort[6];
		
			dataPool->getPara(SERVER_IP_PORT_ID, u8IPAndPort, 6);
			
			memset(ip, 0, sizeof(ip));
			sprintf(ip, "%d.%d.%d.%d", u8IPAndPort[0],u8IPAndPort[1],u8IPAndPort[2],u8IPAndPort[3]);
		
			*port = (u8IPAndPort[4] << 8) + u8IPAndPort[5];
		}
	}
	else if(getIPByDomainName == 1)
	{
		if((ip != NULL) && (domainName != NULL) && (ip_size >= 16))
		{
			struct hostent *server = gethostbyname(domainName);
			if (!server)
			{
				printf("get ip address fail\n");
				return -1;
			}
			else
			{
				strcpy(ip, inet_ntoa(*((struct in_addr*)server->h_addr)));
			}
		}
	}
	else
	{
		return -1;
	}
	
	return 0;
}

/*****************************************************************************
* Function Name : disconnectSocket
* Description   : 断开socket
* Input			: int getIPByDomainName  
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int GBT32960::disconnectSocket()
{
	shutdown(socketfd, SHUT_RDWR);
	close(socketfd);
	isConnected = false;
	resend = false;
	GBTLOG("Close socket socketfd=%d",socketfd);
	return 0;
}

/*****************************************************************************
* Function Name : initTBoxParameterInfo
* Description   : 初始化tbox参数
* Input			: None 
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int GBT32960::initTBoxParameterInfo()
{
	int fd;
	fd = open(PARAMETER_INFO_PATH, O_RDWR | O_CREAT, 0666);
	if (-1 == fd)
	{
		printf("open file failed\n");
		return -1;
	}

	GBT32960_dataInit();
	
    if (-1 == write(fd, p_GBT32960_handle, sizeof(GBT32960_handle)))
    {
		printf("write file error!\n");
        return -1;
    }

	close(fd);

	return 0;
}

/*****************************************************************************
* Function Name : readTBoxParameterInfo
* Description   : 读取参数
* Input			: None 
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int GBT32960::readTBoxParameterInfo()
{
    int fd;
	int retval;
	
	fd = open(PARAMETER_INFO_PATH, O_RDONLY, 0666);
	if(fd < 0)
	{
		GBTLOG("File does not exist!\n");
		return -1;
	}
	else
	{
		retval = read(fd, p_GBT32960_handle, sizeof(GBT32960_handle));
		if(retval > 0)
		{
			GBTLOG("retval:%d\n",retval);
		}

		close(fd);
	}

	return 0;
}

/*****************************************************************************
* Function Name : updateTBoxParameterInfo
* Description   : 更新参数
* Input			: None 
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int GBT32960::updateTBoxParameterInfo()
{
	int fd;
	fd = open(PARAMETER_INFO_PATH, O_RDWR, 0666);
	if (-1 == fd)
	{
		printf("open file failed\n");
		return -1;
	}
	
    if (-1 == write(fd, p_GBT32960_handle, sizeof(GBT32960_handle)))
    {
		printf("write file error!\n");
        return -1;
    }

	close(fd);
	system("sync");

	return 0;
}

/*****************************************************************************
* Function Name : timeoutThread
* Description   : 计时线程
* Input			: void *arg  
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void *GBT32960::timeoutThread(void *arg)
{
	pthread_detach(pthread_self());
	GBTLOG();
	
	bool isTimeout = false;
	GBT32960 *pGBT32960 = (GBT32960*)arg;
	
	while(1)
	{
		if(pGBT32960->isConnected == true)
		{
			if((pGBT32960->recvFlag == 0) && (isTimeout == false))
			{
				sleep(p_GBT32960_handle->configInfo.terminalResponseTimeout * 3);

				if(pGBT32960->recvFlag == 0)
				{
					pGBT32960->timeoutTimes++;
					pGBT32960->recvFlag = 2;
					isTimeout = true;
					GBTLOG("timeoutTimes:%d\n\n\n",pGBT32960->timeoutTimes);
				}
			}
			
			if(pGBT32960->recvFlag == 2)
				isTimeout = false;
		}
	}

	pthread_exit(0);
}

/*****************************************************************************
* Function Name : receiveThread
* Description   : socket接收线程
* Input			: void *arg  
* Output        : None
* Return        : void
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void *GBT32960::receiveThread(void *arg)
{
	pthread_detach(pthread_self());
	GBTLOG();

	int i,length;
	GBT32960 *pGBT32960 = (GBT32960*)arg;

	uint8_t *dataBuff = (uint8_t *)malloc(DATA_BUFF_LEN);
	if(dataBuff == NULL)
	{
		GBTLOG("malloc dataBuff error!");
	}

	while(1)
	{
		if(pGBT32960->isConnected == true)
		{
			memset(dataBuff, 0, DATA_BUFF_LEN);
			length = recv(pGBT32960->socketfd, dataBuff, DATA_BUFF_LEN, 0);
			if(length > 0)
			{
				pGBT32960->recvFlag = 1;
				pGBT32960->timeoutTimes = 0;
				GBTLOG("recvFlag:%d   timeoutTimes:%d",pGBT32960->recvFlag,pGBT32960->timeoutTimes);
				
				GBTLOG("Receive data --->:");
				for(i = 0; i < length; ++i)
					GBT_NO("%02x ", *(dataBuff+i));
				GBT_NO("\n");
				
				if(pGBT32960->checkReceivedData(dataBuff, length) == 0)
				{
					pGBT32960->unpack_GBT32960FrameData(dataBuff, length);
				}
			}
            else if(0 == length)
            {
                printf("====== ret = 0 program exit ======.\n");
                pGBT32960->closeConnect();
                pGBT32960->isConnected = false; 
				resend = false;
                pGBT32960->loginState = 0;
                break;
            }
            else if(length < 0)
            {
            	if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
            	{
            		continue;
            	}
            	else
            	{
            		printf("====== ret < 0 socket error program exit ======.\n");
            		pGBT32960->closeConnect();
					pGBT32960->isConnected = false;
					resend = false;
					pGBT32960->loginState = 0;
                	break;
            	}
            }
		}
	}

	pthread_exit(0);
}

/*****************************************************************************
* Function Name : timingReport
* Description   : 定时上报函数
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int8_t GBT32960::timingReport(uint8_t responseSign)
{
	int length;
	GBTLOG();
	uint8_t *dataBuff = (uint8_t *)malloc(DATA_BUFF_LEN);
	if(dataBuff == NULL)
	{
		GBTLOG("malloc dataBuff error!");
		return -1;
	}
	memset(dataBuff, 0, DATA_BUFF_LEN);
	
	uint8_t *pIndataBuff = (uint8_t *)malloc(DATA_BUFF_LEN);
	if(pIndataBuff == NULL)
	{
		GBTLOG("malloc dataBuff error!");
		return -1;
	}
	memset(dataBuff, 0, DATA_BUFF_LEN);

	length = pack_GBT32960FrameData(dataBuff, DATA_BUFF_LEN, responseSign, GBT32960_RESPONSE_CMD, pIndataBuff,DATA_BUFF_LEN);

	dataBuff[2] = responseSign;
	
	GBTLOG("pack data length:%d\n",length);

	if(length < 50 && responseSign == GBT32960_ReissueInfoReportID)
	{
		ReportingFlagCount = 0;//此处设定发送成功标志为数据上报专用
	
		free(pIndataBuff);
		if(dataBuff != NULL)
		{
			free(dataBuff);
			dataBuff = NULL;
		}

		return 0;
	}

	if((length = send(socketfd, dataBuff, length, 0)) < 0)
	{
		GBTLOG("Send data failed.\n");
		free(dataBuff);
		dataBuff = NULL;
		close(socketfd);
		isConnected = false;
		resend = false;
		ReportingFlagCount++;//发送失败+1
		return -1;
	}
	GBTLOG("Send data ok.length:%d\n",length);
	
	if(length > 50)
	{
		printf("Send data ok.length:%d\n",length);
		for(int i=0;i<length;i++)
		{
			printf(" %02x",dataBuff[i]);
		}
		printf("\n\n");
	}
	
	sys_mylog(dataBuff,length);
	
	//recvFlag = 0;
	ReportingFlagCount = 0;//此处设定发送成功标志为数据上报专用
	
	free(pIndataBuff);
	if(dataBuff != NULL)
	{
		free(dataBuff);
		dataBuff = NULL;
	}

	return 0;
}

/*****************************************************************************
* Function Name : setTimer
* Description   : 定时函数
* Input			: uint32_t seconds   
*                 uint32_t mseconds 
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void GBT32960::setTimer(uint32_t seconds, uint32_t mseconds)
{
	struct timeval temp;
	temp.tv_sec = seconds;
	temp.tv_usec = mseconds;

	select(0, NULL, NULL, NULL, &temp);
}

/*****************************************************************************
* Function Name : sendLoginData
* Description   : 登录服务器
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int GBT32960::sendLoginData()
{
	int length;

	uint8_t *dataBuff = (uint8_t *)malloc(DATA_BUFF_LEN);
	if(dataBuff == NULL)
	{
		GBTLOG("malloc dataBuff error!");
		return -1;
	}
	memset(dataBuff, 0, DATA_BUFF_LEN);

	memset(p_GBT32960_handle->dataInfo.ICCID, 0, sizeof(p_GBT32960_handle->dataInfo.ICCID));
	dataPool->getPara(SIM_ICCID_INFO, p_GBT32960_handle->dataInfo.ICCID, sizeof(p_GBT32960_handle->dataInfo.ICCID));

	length = pack_GBT32960FrameData(dataBuff, DATA_BUFF_LEN, GBT32960_VehiceLoginID, GBT32960_RESPONSE_CMD,NULL,0);
	
	GBTLOG("pack data length:%d\n",length);

	if((length = send(socketfd, dataBuff, length, 0)) < 0)
	{
		GBTLOG("Send data failed.\n");
		free(dataBuff);
		dataBuff = NULL;
		close(socketfd);
		isConnected = false;
		resend = false;
		return -1;
	}
	GBTLOG("Send data ok.length:%d\n",length);
	
	recvFlag = 0;
	GBTLOG("recvFlag:%d\n",recvFlag);

	free(dataBuff);
	dataBuff = NULL;

	return 0;
}

/*****************************************************************************
* Function Name : sendLogoutData
* Description   : 登录服务器
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int GBT32960::sendLogoutData()
{
	int length;

	uint8_t *dataBuff = (uint8_t *)malloc(DATA_BUFF_LEN);
	if(dataBuff == NULL)
	{
		GBTLOG("malloc dataBuff error!");
		return -1;
	}
	memset(dataBuff, 0, DATA_BUFF_LEN);


	length = pack_GBT32960FrameData(dataBuff, DATA_BUFF_LEN, GBT32960_VehiceLogoutID, GBT32960_RESPONSE_CMD,NULL,0);
	
	GBTLOG("pack data length:%d\n",length);

	if((length = send(socketfd, dataBuff, length, 0)) < 0)
	{
		GBTLOG("Send data failed.\n");
		free(dataBuff);
		dataBuff = NULL;
		close(socketfd);
		isConnected = false;
		resend = false;
		return -1;
	}
	GBTLOG("Send data ok.length:%d\n",length);
	
	recvFlag = 0;
	GBTLOG("recvFlag:%d\n",recvFlag);

	free(dataBuff);
	dataBuff = NULL;
	sleep(300);
	return 0;
}

/*****************************************************************************
* Function Name : GBT32960_dataInit
* Description   : 协议数据初始化
* Input			: None
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint8_t GBT32960::GBT32960_dataInit()
{
	GBTLOG("Init param GBT32960\n");

	//init tboxStatusInfo
	p_GBT32960_handle->statusInfo.year = 0;
	p_GBT32960_handle->statusInfo.month = 0;
	p_GBT32960_handle->statusInfo.today = 0;
	
	//init GBT32960_dataInfo
#define DEV_NO 0
		
#if DEV_NO == 1
	memcpy(p_GBT32960_handle->dataInfo.vin, "LFPZ1APCXH5E90107", strlen("LSVFA49J232037001"));
	memcpy(p_GBT32960_handle->dataInfo.ICCID, "89860117750028636463", strlen("89860117750028632728"));
/*	memcpy(p_GBT32960_handle->dataInfo.vin, "LSVFA49J232037001", strlen("LSVFA49J232037001"));
	memcpy(p_GBT32960_handle->dataInfo.ICCID, "89860317947553240376", strlen("89860317947553240376"));*/
#elif DEV_NO == 2
	memcpy(p_GBT32960_handle->dataInfo.vin, "LDPZYB6D9JCES0001", strlen("LSVFA49J232037002"));
	memcpy(p_GBT32960_handle->dataInfo.ICCID, "89860618000013179630", strlen("89860117750028632728"));
#elif DEV_NO == 3
	memcpy(p_GBT32960_handle->dataInfo.vin, "LSVFA49J232037003", strlen("LSVFA49J232037003"));
	memcpy(p_GBT32960_handle->dataInfo.ICCID, "89860117750028632728", strlen("89860117750028632728"));
/*	memcpy(p_GBT32960_handle->dataInfo.vin, "LSVFA49J232037003", strlen("LSVFA49J232037003"));
	memcpy(p_GBT32960_handle->dataInfo.ICCID, "89860117851047582223", strlen("89860117851047582223"));*/
#endif

	p_GBT32960_handle->dataInfo.serialNumber = 1;
	p_GBT32960_handle->dataInfo.rechargeableEnergyNum = 1;
	p_GBT32960_handle->dataInfo.rechargeableEnergyLen = 0;

	//Initialize vehicleDataInfo struct
	p_GBT32960_handle->dataInfo.vehicleInfo.vehicleState = 0x1;				 	 //*车辆状态
	p_GBT32960_handle->dataInfo.vehicleInfo.chargeState = 0x3;					 //*充电状态
	p_GBT32960_handle->dataInfo.vehicleInfo.runMode = 0x1;						 //*运行模式*/
	p_GBT32960_handle->dataInfo.vehicleInfo.vehicleSpeed = 0xaf;				 //*车速
	p_GBT32960_handle->dataInfo.vehicleInfo.vehicleMileage = 0x156f;		 //*里程*/
	p_GBT32960_handle->dataInfo.vehicleInfo.vehicleVoltage = 0xce4;			 //*总电压
	p_GBT32960_handle->dataInfo.vehicleInfo.vehicleCurrent = 0x44c;			 //*总电流
	p_GBT32960_handle->dataInfo.vehicleInfo.SOC = 0x32;							 //*SOC*/
	p_GBT32960_handle->dataInfo.vehicleInfo.DcDcState = 0x1; 					 //*DC/DC状态
	p_GBT32960_handle->dataInfo.vehicleInfo.gear = 0xe;						 //*档位*/
	p_GBT32960_handle->dataInfo.vehicleInfo.insulationResistance = 0x7cf;			 //*绝缘电阻*/
	p_GBT32960_handle->dataInfo.vehicleInfo.acceleratorPedalTravelValue = 0xf;	 //*加速踏板行程值
	p_GBT32960_handle->dataInfo.vehicleInfo.brakePedalState = 0x0;				 //*制动踏板状态

	//Initialize driveMotorDataInfo struct
	p_GBT32960_handle->dataInfo.driveMotorInfo.totalDriveMotor = 1;
	p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[0].driveMotorSerialNum = 1;             //*驱动电机序号*/
	p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[0].driveMotorState = 0x1;			  //*驱动电机状态
	p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[0].driveMotorControllerTemp = 0x50;     //*驱动电机控制器温度
	p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[0].driveMotorRotationalSpeed = 0x530a;  //*驱动电机转速
	p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[0].driveMotorTorque = 0x898; 		  //*驱动电机转矩*/
	p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[0].driveMotorTemp = 0x51;				  //*驱动电机温度*/
	p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[0].motorControllerVin = 0xdac;		  //*驱动控制器输入电压
	p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[0].motorControllerDC = 0x460;		  //*驱动控制器直流母线电流

	//Initialize fuelCellDataInfo struct
	p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellVoltage = 0xffff;					//*燃料电池电压*/
	p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellCurrent = 0xffff;					//*燃料电池电流*/
	p_GBT32960_handle->dataInfo.fuelCellInfo.fuelConsumptionRate = 0xffff;				//*燃料消耗率*/
	p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellTempProbeNum = 0x00; 				//*燃料电池温度探针总数*/
	p_GBT32960_handle->dataInfo.fuelCellInfo.ProbeTemperature[0] = 0x00;    			//*探针温度值
	p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenSysMaxTemperature = 0xff;		//*氢系统中最高温值
	p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenSysMaxTemperatureProbeNum = 0xff;  //*氢系统中最高温度探针代号
	p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxSolubility = 0xff;			//*氢气最高浓度
	p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxSolubilitySensorNum = 0xff;     //*氢气最高溶度传感器代号*/
	p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxPressure = 0;				    //*氢气最高压值
	p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxPressureSensorNum = 0xff;       //*氢气最高压力传感器代号*/
	p_GBT32960_handle->dataInfo.fuelCellInfo.HighPressureDcDcState = 0xff;              //*高压DC/DC 状态
	//Initialize engineDataInfo struct
	p_GBT32960_handle->dataInfo.engineInfo.engineState = 0xff;               //*发动机状态
	p_GBT32960_handle->dataInfo.engineInfo.engineCrankshaftSpeed = 0x00;   //*曲轴转速
	p_GBT32960_handle->dataInfo.engineInfo.fuelConsumption = 0x00;         //*燃料消耗率*/

	//Initialize positionDataInfo struct
	p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState = 0x00;	//位置状态
	p_GBT32960_handle->dataInfo.positionInfo.Lon = 0xFFFFFFFF;	//经度
	p_GBT32960_handle->dataInfo.positionInfo.Lat = 0xFFFFFFFF;	//维度
	
	//Initialize extremeDataInfo struct
	p_GBT32960_handle->dataInfo.extremeInfo.MaxVolBatterySubsysNumber = 0x1; 		//*最高电压电池子系统号
	p_GBT32960_handle->dataInfo.extremeInfo.MaxVolBattery = 0x3; 					//*最高电压电池单体代号
	p_GBT32960_handle->dataInfo.extremeInfo.MaxBatteryVoltageValue = 0xed3;		   //*电池单体电压最高值
	p_GBT32960_handle->dataInfo.extremeInfo.MinVolBatterySubsysNumber = 0x1; 		//*最低电压电池子系统号
	p_GBT32960_handle->dataInfo.extremeInfo.MinVolBattery = 0x5c; 					//*最低电压电池单体代号
	p_GBT32960_handle->dataInfo.extremeInfo.MinBatteryVoltageValue = 0xebf;		   //*电池单体电压最低值
	p_GBT32960_handle->dataInfo.extremeInfo.MaxTemperatureSubsysNumber = 0x1;		//*最高温度子系统号
	p_GBT32960_handle->dataInfo.extremeInfo.MaxTemperatureProbe = 0xd;			    //*最高温度探针序号
	p_GBT32960_handle->dataInfo.extremeInfo.MaxTemperatureValue = 0x39;			    //*最高温度值
	p_GBT32960_handle->dataInfo.extremeInfo.MinTemperatureSubsysNumber = 0x1;      //*最低温度子系统号
	p_GBT32960_handle->dataInfo.extremeInfo.MinTemperatureProbe = 0x6;			    //*最低温度探针序号
	p_GBT32960_handle->dataInfo.extremeInfo.MinTemperatureValue = 0x35;			    //*最低温度值

	//Initialize alarmDataInfo struct
	p_GBT32960_handle->dataInfo.alarmInfo.maxAlarmLevel = 0xFE;						 //*最高报警等级
	p_GBT32960_handle->dataInfo.alarmInfo.generalAlarm.AlarmFlag = 0;				 //*通用报警标志*/
	p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEnergyStorageDev = 0xFE;    //*可充电储能装置故障总数*/
	p_GBT32960_handle->dataInfo.alarmInfo.energyStorageDevFault[0].fault = 0;        //*可充电储能装置代码列表
	p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfDriveMotor = 0xFE;		     //*驱动电机故障总数*/
	p_GBT32960_handle->dataInfo.alarmInfo.driveMotorFault[0].fault = 0;		         //*驱动电机故障代码列表*/
	p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEngine = 0;              //*发动机故障总数*/
	p_GBT32960_handle->dataInfo.alarmInfo.engineFault[0].fault = 0;                  //*发动机故障列表
	p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfOther = 0;				 //*其他故障总数*/
	p_GBT32960_handle->dataInfo.alarmInfo.otherFault[0].fault = 0;			         //*其他故障代码列表*/

	//Initialize energyStorageDevSubsysDataInfo struct
	p_GBT32960_handle->dataInfo.ESDSubsysInfo.totalEnergySubsys = 1;
	p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[0].subsysNumber = 0x01;             //*可充电储能装置子系统数
	p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[0].subsysVol = 0x21;              	//*可充电储能装置子系统电压*/
	p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[0].subsysCurrent = 0x271a;          //*可充电储能装置子系统电流*/
	p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[0].totalNumOfSingleCell = 0x1;   	//*单体电池总数*/
	//p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[0].frameStartCellNumber[0] = 1;     //*本帧起始电池序号*/
	p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[0].frameStartCellNumber = 1;     //*本帧起始电池序号*/
	
	p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[0].frameStartCellTotal = 1;         //*本帧单体电池总数*/
	p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[0].monomerCellVol[0] = 0xce4;      //*单体电池电压*/
	
	p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[0].subsysProbeNumber = 1; 	         //*可充电储能装置子系统温度探针个数*/
	p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[0].subsysProbeTemper[0] = 0x32;     //*可充电储能装置子系统温度探针温度置

	//init GBT32960_ConfigInfo
	p_GBT32960_handle->configInfo.loginTimeInterval = 60;			//*登录服务器时间周期，单位S*/
	p_GBT32960_handle->configInfo.localStorePeriod = 30000; 			//*本地存储时间周期，单位MS*/
	p_GBT32960_handle->configInfo.timingReportTimeInterval = 10;    //*信息上报时间周期，单位S*/
	p_GBT32960_handle->configInfo.alarmInfoReportPeriod = 1000;		//*报警信息上报时间周期，单位mS*/
	p_GBT32960_handle->configInfo.remoteServiceDomainNameLen = 0;	//*远程服务与管理平台域名长度
	
	memset(p_GBT32960_handle->configInfo.remoteServiceDomainName,0,sizeof(p_GBT32960_handle->configInfo.remoteServiceDomainName));//*远程服务与管理平台域名
	p_GBT32960_handle->configInfo.remoteServicePort = 0xff;			//*远程服务与管理平台端口
	memset(p_GBT32960_handle->configInfo.hardwareVersion,0,sizeof(p_GBT32960_handle->configInfo.hardwareVersion));		//硬件版本，不可设置
    memset(p_GBT32960_handle->configInfo.firmwareVersion,0,sizeof(p_GBT32960_handle->configInfo.firmwareVersion));		//固件版本，不可设置
    p_GBT32960_handle->configInfo.heartbeatInterval = 30;			//*车载终端心跳发送周期，单位S*/	
	p_GBT32960_handle->configInfo.terminalResponseTimeout = 60; 		//*终端应答超时时间，单位S*/
	p_GBT32960_handle->configInfo.platformResponseTimeout = 20; 		//*平台应答超时时间，单位S*/
	p_GBT32960_handle->configInfo.logonFailureInterval = 30; 		//*连续三次登录失败，到下次登录的间隔时间，单位min*/	
	p_GBT32960_handle->configInfo.publicDomainNameLen = 0; 			//*公共平台域名长度*/
	memset(p_GBT32960_handle->configInfo.publicDomainName,0,sizeof(p_GBT32960_handle->configInfo.publicDomainName));		//公共平台域名
  	p_GBT32960_handle->configInfo.publicPort = 0xffff;				//*公共平台端口*/
	p_GBT32960_handle->configInfo.IsSamplingCheck = 0xff;			//*是否处于抽样检测中*/


	memset(p_GBT32960_handle->ftp.URLAddress, 0, sizeof(p_GBT32960_handle->ftp.URLAddress));//FTPURL地址
	memset(p_GBT32960_handle->ftp.fileName, 0, sizeof(p_GBT32960_handle->ftp.fileName));//FTP文件名字
	memset(p_GBT32960_handle->ftp.Apn, 0, sizeof(p_GBT32960_handle->ftp.Apn));			//FTPAPN
	memset(p_GBT32960_handle->ftp.userName, 0, sizeof(p_GBT32960_handle->ftp.userName));//FTP用户名
	memset(p_GBT32960_handle->ftp.password, 0, sizeof(p_GBT32960_handle->ftp.password));//FTP密码
	memset(p_GBT32960_handle->ftp.domainIP, 0, sizeof(p_GBT32960_handle->ftp.domainIP));//FTP地址
	p_GBT32960_handle->ftp.port	=	0;	///FTP端口(0-FFFB)
	memset(p_GBT32960_handle->ftp.manufacturer, 0, sizeof(p_GBT32960_handle->ftp.manufacturer));//FTP制造商ID
	memset(p_GBT32960_handle->ftp.hardwareVersion, 0, sizeof(p_GBT32960_handle->ftp.hardwareVersion));//硬件版本，不可设置
	memset(p_GBT32960_handle->ftp.firmwareVersion, 0, sizeof(p_GBT32960_handle->ftp.firmwareVersion));//固件版本，不可设置	
	p_GBT32960_handle->ftp.connectTimeOut	=	0;	///FTP连接服务器有效时限
	
	return 0;
}

/*****************************************************************************
* Function Name : checkSum_BCC
* Description   : Check the data with BCC
* Input			: uint8_t *pData
*                 uint16_t len
* Output        : None
* Return        : checkSum
* Auther        : ygg
* Date          : 2017.06.23
*****************************************************************************/
uint8_t GBT32960::checkSum_BCC(uint8_t *pData, uint16_t len)
{
	uint8_t *pos = pData;
	uint8_t checkSum = *pos++;
	
	for(uint16_t i = 0; i<(len-1); i++)
	{
		checkSum ^= *pos++;
	}

	return checkSum;
}

/*****************************************************************************
* Function Name : pack_GBT32960FrameData
* Description   : 打包一帧数据
* Input			: uint8_t *dataBuff     
*                 uint16_t len          
*                 uint8_t cmd           
*                 uint8_t responseSign  
*                 uint8_t *pInBuff      
*                 uint16_t size   
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::pack_GBT32960FrameData(uint8_t *dataBuff, uint16_t len, uint8_t cmd, uint8_t responseSign, uint8_t *pInBuff, uint16_t size)
{
	uint8_t *pos = dataBuff;
	uint16_t headLen;
	uint16_t dataLen;
	uint16_t totalLen;
	uint8_t checkCode;
	uint8_t i;

	if(cmd == GB32960_SetCommandID || cmd == GB32960_ControlCommandID)
	{
		if(responseSign == GBT32960_RESPONSE_SUCCESS || responseSign == GBT32960_RESPONSE_ERROR)
		{
			
			struct tm *p_tm = NULL;
			time_t tmp_time;
			tmp_time = time(NULL);
			p_tm = gmtime(&tmp_time);
			GBTLOG("TIME:%0d-%d-%d:%02d:%02d:%02d\n",p_tm->tm_year+1900, p_tm->tm_mon,p_tm->tm_mday, p_tm->tm_hour,p_tm->tm_min,p_tm->tm_sec);

			*(pInBuff+3) = GBT32960_RESPONSE_SUCCESS;
			//time
			/*
	  		*(pInBuff+24) = p_tm->tm_year - 100;
			*(pInBuff+25) = p_tm->tm_mon + 1;
			*(pInBuff+26) = p_tm->tm_mday;
			*(pInBuff+27) = p_tm->tm_hour;
			*(pInBuff+28) = p_tm->tm_min;
			*(pInBuff+29) = p_tm->tm_sec;*/
			for(int i =0; i <6;i++)
			{
				*(pInBuff+24+i) = controltime[i];
			}


			checkCode = checkSum_BCC(pInBuff+2, size-3);
			GBTLOG("checkCode:%02x ",checkCode);
			*(pInBuff+size-1) = checkCode;

			memcpy(pos, pInBuff, size);
			//pos += size;

			GBTLOG("Pack data -->:");
			for(i = 0; i<size; i++)
				GBT_NO("%02x ",*(dataBuff+i));
			GBT_NO("\n");

			return size;
		}
	}
	else
	{
		//Potocal header
		*pos++ = (GBT32960_PROTOCOL_HEADER >>8) & 0xFF;
		*pos++ = (GBT32960_PROTOCOL_HEADER >>0) & 0xFF;
		//Command
		*pos++ = cmd;
		*pos++ = responseSign;
		//Vehicle identification number

		GBTLOG("GBT32960 VIN:%s,Vin Len:%d", p_GBT32960_handle->dataInfo.vin,sizeof(p_GBT32960_handle->dataInfo.vin));
		memcpy(pos, p_GBT32960_handle->dataInfo.vin, sizeof(p_GBT32960_handle->dataInfo.vin));
		pos += sizeof(p_GBT32960_handle->dataInfo.vin);

		//Encryption method
		*pos++ = 0x01;
		//Data length
		*pos++ = 0;
		*pos++ = 0;

		headLen = pos - dataBuff;
		
		dataLen = pack_GBT32960DataBody(cmd, pos, len-headLen, pInBuff, size);

		//Fill in total data length
		dataBuff[headLen - 2] = (dataLen>>8) & 0xFF;
		dataBuff[headLen - 1] = (dataLen>>0) & 0xFF;

		//get the total length
		totalLen = dataLen + headLen;

		//Data Validate
		checkCode = checkSum_BCC(dataBuff+2, totalLen-2);
		GBTLOG("headLen:%d, dataLen:%d,totalLen:%d,checkCode:%02x", headLen,dataLen,totalLen,checkCode);

		pos += dataLen;
		*pos++ = checkCode;

		//GBTLOG("Pack data -->:");
		//for(i = 0; i<(pos-dataBuff); i++)
			//GBT_NO("%02x ",*(dataBuff+i));
		//GBT_NO("\n");

		return pos - dataBuff;
	}

	return 0;
}

/*****************************************************************************
* Function Name : pack_GBT32960DataBody
* Description   : 打包协议数据体
* Input			: uint8_t cmd          
*                 uint8_t *dataBuff           
*                 uint16_t buffSize  
*                 uint8_t *pInBuff      
*                 uint16_t len   
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::pack_GBT32960DataBody(uint8_t cmd, uint8_t *dataBuff, uint16_t buffSize, uint8_t *pInBuff, uint16_t len)
{
	uint16_t dataLen = 0;

	switch (cmd)
	{
		case GBT32960_VehiceLoginID:
			dataLen = pack_GBT32960_VehiceLogin(dataBuff, buffSize);
			break;
		case GBT32960_RealTimeInfoReportID:
			GBTLOG("GBT32960 Real-time reporting\n");
			dataLen = pack_GBT32960_RealTimeInfoReport(dataBuff, buffSize);
			break;
		case GBT32960_ReissueInfoReportID:
			dataLen = pack_GBT32960_ReissueInfoReport(dataBuff, buffSize);
			break;
		case GBT32960_VehiceLogoutID:
			dataLen = pack_GBT32960_VehiceLogout(dataBuff, buffSize);
			break;
		/*case GBT32960_PlatformLoginID:
			dataLen = pack_GBT32960_PlatformLogin(dataBuff,buffSize);
			break;
		case GBT32960_PlatformLogoutID:
			dataLen = pack_GBT32960_PlatformLogout(dataBuff,buffSize);
			break;*/
		case GB32960_HeartBeatID:
			break;
		case GB32960_TerminalCalibrationTime:
			//dataLen = pack_GBT32960_TerminalCalibrationTime(dataBuff, buffSize);
			break;
		case GB32960_QueryCommandID:	//查询命令
			dataLen = pack_GBT32960_QueryCommand(dataBuff, buffSize, pInBuff, len);
			break;
		case GB32960_SetCommandID:	//设置命令
			//dataLen = pack_GBT32960_SetCommand(dataBuff, buffSize, pInBuff, len);
			break;
		case GB32960_ControlCommandID://控制命令
			//dataLen = pack_GBT32960_ControlCommand(dataBuff, buffSize, pInBuff, len);
			break;
		default:
			GBTLOG("cmd error!");
			break;
	}

	return dataLen;
}

/*****************************************************************************
* Function Name : pack_GBT32960_VehiceLogin
* Description   : 打包车辆登录信息
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::pack_GBT32960_VehiceLogin(uint8_t *dataBuff, uint16_t buffSize)
{
	uint8_t i;
	uint8_t *pos = dataBuff;

	struct tm *p_tm = NULL;
	time_t tmp_time;
	tmp_time = time(NULL);
	p_tm = gmtime(&tmp_time);
	GBTLOG("TIME:%0d-%d-%d:%02d:%02d:%02d",p_tm->tm_year+1900, p_tm->tm_mon,p_tm->tm_mday, p_tm->tm_hour,p_tm->tm_min,p_tm->tm_sec);

	//time
	*pos++ = p_tm->tm_year - 100;
	*pos++ = p_tm->tm_mon + 1;
	*pos++ = p_tm->tm_mday;
	*pos++ = p_tm->tm_hour;
	*pos++ = p_tm->tm_min;
	*pos++ = p_tm->tm_sec;

	//login serial number
	*pos++ = (p_GBT32960_handle->dataInfo.serialNumber >> 8)&0xFF;
	*pos++ = (p_GBT32960_handle->dataInfo.serialNumber >> 0)&0xFF;
	GBTLOG("GBT32960 serialNumber:%d", p_GBT32960_handle->dataInfo.serialNumber);


	GBTLOG("GBT32960 ICCID:%s,ICCID Len:%d", p_GBT32960_handle->dataInfo.ICCID,sizeof(p_GBT32960_handle->dataInfo.ICCID));
	memcpy(pos, p_GBT32960_handle->dataInfo.ICCID, sizeof(p_GBT32960_handle->dataInfo.ICCID));
	pos += sizeof(p_GBT32960_handle->dataInfo.ICCID);

	//The number of rechargeable energy storage systems
	if(p_GBT32960_handle->dataInfo.rechargeableEnergyNum > MAX_STOREENERGY_NUM)
		p_GBT32960_handle->dataInfo.rechargeableEnergyNum = MAX_STOREENERGY_NUM;
	*pos++ = p_GBT32960_handle->dataInfo.rechargeableEnergyNum;

	if(p_GBT32960_handle->dataInfo.rechargeableEnergyLen > MAX_STOREENERGY_LEN)
		p_GBT32960_handle->dataInfo.rechargeableEnergyLen = MAX_STOREENERGY_LEN;
	*pos++ = p_GBT32960_handle->dataInfo.rechargeableEnergyLen;

	GBTLOG("rechargeableEnergyLen:%d,rechargeableEnergyNum:%d",p_GBT32960_handle->dataInfo.rechargeableEnergyLen,
		p_GBT32960_handle->dataInfo.rechargeableEnergyNum);
	
	//The length of rechargeable energy storage systems code
	if(p_GBT32960_handle->dataInfo.rechargeableEnergyLen != 0)
	{
		//Rechargeable energy storage system code
		for(i = 0; i<p_GBT32960_handle->dataInfo.rechargeableEnergyNum; i++)
		{
			GBTLOG();
			memcpy(pos, &p_GBT32960_handle->dataInfo.rechargeableEnergyCode[i][0], p_GBT32960_handle->dataInfo.rechargeableEnergyLen);
			pos += p_GBT32960_handle->dataInfo.rechargeableEnergyLen;
		}
	}

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : pack_GBT32960_RealTimeInfoReport
* Description   : 打包实时上报数据包
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::pack_GBT32960_RealTimeInfoReport(uint8_t *dataBuff, uint16_t buffSize)
{
	uint8_t *pos = dataBuff;
	uint16_t dataLen = 0;
	uint16_t offset;
	reissue = false;
	struct tm *p_tm = NULL;
	time_t tmp_time;
	tmp_time = time(NULL);
	p_tm = gmtime(&tmp_time);
	GBTLOG("TIME:%0d-%d-%d:%02d:%02d:%02d",p_tm->tm_year+1900, p_tm->tm_mon,p_tm->tm_mday, p_tm->tm_hour,p_tm->tm_min,p_tm->tm_sec);
	printf("[ TIME is :%0d-%d-%d:%02d:%02d:%02d ]\n",p_tm->tm_year+1900, p_tm->tm_mon,p_tm->tm_mday, p_tm->tm_hour,p_tm->tm_min,p_tm->tm_sec);

	//time
	*pos++ = p_tm->tm_year - 100;
	*pos++ = p_tm->tm_mon + 1;
	*pos++ = p_tm->tm_mday;
	*pos++ = p_tm->tm_hour;
	*pos++ = p_tm->tm_min;
	*pos++ = p_tm->tm_sec;

	//整车数据0x01
	offset = pos-dataBuff;
	GBTLOG("pos:%x,pos+offset=%x,offset:%d\n", pos,(pos+offset),offset);	
	if((dataLen = add_VehicleData(pos, buffSize-offset)) == 0)
	{
		GBTLOG("add_VehicleData error!");
		return 0;
	}
	pos += dataLen;


	//驱动电机数据0x02
	offset = pos-dataBuff;
	GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
	if((dataLen = add_DriveMotorData(pos, buffSize-offset)) == 0)
	{
		GBTLOG("add_VehicleData error!");
		return 0;
	}
	pos += dataLen;


	#if 1
	//燃料电池数据0x03
	offset = pos-dataBuff;
	GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);

	
	if((dataLen = add_FuelCellData(pos, buffSize-offset)) == 0)
	{
		GBTLOG("add_FuelCellData error!");
		return 0;
	}
	pos += dataLen;

	//发动机数据0x04
	offset = pos-dataBuff;
	GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);

	# endif
	/*if((dataLen = add_EngineData(pos, buffSize-offset)) == 0)
	{
		GBTLOG("add_EngineData error!");
		return 0;
	}
	pos += dataLen;

	//车辆位置数据0x05
	offset = pos-dataBuff;*/
	GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
	if((dataLen = add_PositionData(pos, buffSize-offset)) == 0)
	{
		GBTLOG("add_PositionData error!");
		return 0;
	}
	pos += dataLen;
	
	//极值数据x06
	offset = pos-dataBuff;
	GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
	if((dataLen = add_ExtremeData(pos, buffSize-offset)) == 0)
	{
		GBTLOG("add_ExtremeData error!");
		return 0;
	}
	pos += dataLen;

	//报警数据0x07
	offset = pos-dataBuff;
	GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
	if((dataLen = add_AlarmData(pos, buffSize-offset)) == 0)
	{
		GBTLOG("add_AlarmData error!");
		return 0;
	}
	pos += dataLen;

	//可充电储能装置电压数0x08
	offset = pos-dataBuff;
	GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
	if((dataLen = add_EnergyStorageDevSubsysVolData(pos, buffSize-offset)) == 0)
	{
		GBTLOG("add_EnergyStorageDevSubsysData error!");
		return 0;
	}
	pos += dataLen;

	//可充电储能装置温度数0x09
	offset = pos-dataBuff;
	GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
	if((dataLen = add_EnergyStorageDevSubsysTempData(pos, buffSize-offset)) == 0)
	{
		GBTLOG("add_EnergyStorageDevSubsysData error!");
		return 0;
	}
	pos += dataLen;

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : add_VehicleData
* Description   : 打包整车数据
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::add_VehicleData(uint8_t *dataBuff, uint16_t buffSize)
{
	uint16_t i;
	uint8_t *pos = dataBuff;
	
	GBTLOG("VehicleData size:%d\n", buffSize);
	//if(buffSize < 21)
	//	return 0;
	
	*pos++ = GB32960_VehicleData;
	*pos++ = p_GBT32960_handle->dataInfo.vehicleInfo.vehicleState;					 //*车辆状态
	*pos++ = p_GBT32960_handle->dataInfo.vehicleInfo.chargeState;					 //*充电状态
	*pos++ = p_GBT32960_handle->dataInfo.vehicleInfo.runMode;						 //*运行模式

	if(2 == p_GBT32960_handle->dataInfo.vehicleInfo.vehicleState)
	{
		p_GBT32960_handle->dataInfo.vehicleInfo.vehicleSpeed = 0;
	}
	
	*pos++ = (p_GBT32960_handle->dataInfo.vehicleInfo.vehicleSpeed >> 8)&0xff;		 //*车速
	*pos++ = (p_GBT32960_handle->dataInfo.vehicleInfo.vehicleSpeed >> 0)&0xff;

	*pos++ = (p_GBT32960_handle->dataInfo.vehicleInfo.vehicleMileage >>24)&0xff;	 //*里程*/
	*pos++ = (p_GBT32960_handle->dataInfo.vehicleInfo.vehicleMileage >>16)&0xff;
	*pos++ = (p_GBT32960_handle->dataInfo.vehicleInfo.vehicleMileage >> 8)&0xff;
	*pos++ = (p_GBT32960_handle->dataInfo.vehicleInfo.vehicleMileage >> 0)&0xff;

	*pos++ = (p_GBT32960_handle->dataInfo.vehicleInfo.vehicleVoltage >> 8)&0xff;     //*总电压
	*pos++ = (p_GBT32960_handle->dataInfo.vehicleInfo.vehicleVoltage >> 0)&0xff;

	*pos++ = (p_GBT32960_handle->dataInfo.vehicleInfo.vehicleCurrent >> 8)&0xff;	 //*总电流
	*pos++ = (p_GBT32960_handle->dataInfo.vehicleInfo.vehicleCurrent >> 0)&0xff;

	*pos++ = p_GBT32960_handle->dataInfo.vehicleInfo.SOC;							 //*SOC*/
	*pos++ = p_GBT32960_handle->dataInfo.vehicleInfo.DcDcState; 					 //*DC/DC状态
	*pos++ = p_GBT32960_handle->dataInfo.vehicleInfo.gear;							 //*档位*/

	
	*pos++ = (p_GBT32960_handle->dataInfo.vehicleInfo.insulationResistance >> 8)&0xff; //*绝缘电阻*/
	*pos++ = (p_GBT32960_handle->dataInfo.vehicleInfo.insulationResistance >> 0)&0xff;

	*pos++ = p_GBT32960_handle->dataInfo.vehicleInfo.acceleratorPedalTravelValue;	 //*加速踏板行程值
	*pos++ = p_GBT32960_handle->dataInfo.vehicleInfo.brakePedalState;				 //*制动踏板状态
	
	if(!testout)
	{
		sendLogoutData();
	}

	GBTLOG("VehicleData:");
	for(i = 0; i<(pos-dataBuff); i++)
		GBT_NO("%02x ",*(dataBuff+i));
	GBT_NO("\n\n\n");

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : add_DriveMotorData
* Description   : 增加驱动马达数据
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::add_DriveMotorData(uint8_t *dataBuff, uint16_t buffSize)
{
	int i;
	uint8_t *pos = dataBuff;

	GBTLOG("DriveMotorData size:%d\n", buffSize);
	//if(buffSize < 13)
	//	return 0;
	
	*pos++ = GB32960_DriveMotorData;	
	if (p_GBT32960_handle->dataInfo.driveMotorInfo.totalDriveMotor > MAX_DRIVEMOTOR_NUM)
		p_GBT32960_handle->dataInfo.driveMotorInfo.totalDriveMotor = MAX_DRIVEMOTOR_NUM;
	*pos++ = p_GBT32960_handle->dataInfo.driveMotorInfo.totalDriveMotor;

	for (i = 0; i < p_GBT32960_handle->dataInfo.driveMotorInfo.totalDriveMotor; i++)
	{
		*pos++ = p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorSerialNum; 	   //*驱动电机序号*/
		*pos++ = p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorState; 		   //*驱动电机状态
		*pos++ = p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorControllerTemp;   //*驱动电机控制器温度
		*pos++ = (p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorRotationalSpeed >> 8)&0xff;//*驱动电机转速
		*pos++ = (p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorRotationalSpeed >> 0)&0xff;

		*pos++ = (p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorTorque >> 8)&0xff; //*驱动电机转矩*/
		*pos++ = (p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorTorque >> 0)&0xff;

		*pos++ = p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorTemp;				 //*驱动电机温度*/

		*pos++ = (p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].motorControllerVin >> 8)&0xff; //*驱动控制器输入电压
		*pos++ = (p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].motorControllerVin >> 0)&0xff;

		*pos++ = (p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].motorControllerDC >> 8)&0xff; //*驱动控制器直流母线电流
		*pos++ = (p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].motorControllerDC >> 0)&0xff;
	}

	GBTLOG("add_DriveMotorData:");
	for(i = 0; i<(pos-dataBuff); i++)
		GBT_NO("%02x ",*(dataBuff+i));
	GBT_NO("\n\n\n");

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : add_FuelCellData
* Description   : 增加燃料电池数据
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::add_FuelCellData(uint8_t *dataBuff, uint16_t buffSize)
{
	int i;
	uint8_t *pos = dataBuff;

	GBTLOG("FuelCellData size:%d\n", buffSize);
//	printf("====== begin add fuel. ======\n");
	*pos++ = GB32960_FuelCellData;
	*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellVoltage >> 8)&0xff; 		 //*燃料电池电压*/
	*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellVoltage >> 0)&0xff;
	*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellCurrent >> 8)&0xff; 		 //*燃料电池电流*/
	*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellCurrent >> 0)&0xff;
	*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.fuelConsumptionRate >> 8)&0xff; 	 //*燃料消耗率*/
	*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.fuelConsumptionRate >> 0)&0xff;

//	printf("======1 begin add fuel. ======\n");
	if((p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellTempProbeNum == 0xFFFF) ||		//*燃料电池温度探针总数
		(p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellTempProbeNum == 0xFFFE))
	{
		*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellTempProbeNum >> 8)&0xff;
		*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellTempProbeNum >> 0)&0xff;
	}
	else//有效个数
	{
		if(p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellTempProbeNum > MAX_PROBE_NUM)     //*燃料电池温度探针总数*/
			p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellTempProbeNum = MAX_PROBE_NUM;
		
		*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellTempProbeNum >> 8)&0xff;
		*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellTempProbeNum >> 0)&0xff;
//		printf("====== 12in add fuel. ======\n");
		for(i = 0;i < ( p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellTempProbeNum); i++)        //*探针温度值
		{
			*pos++ = p_GBT32960_handle->dataInfo.fuelCellInfo.ProbeTemperature[i]&0xff;
		}
	}
	
//	printf("====== 2begin add fuel. ======\n");
	*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenSysMaxTemperature >> 8)&0xff; //*氢系统中最高温度
	*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenSysMaxTemperature >> 0)&0xff;

	*pos++ = p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenSysMaxTemperatureProbeNum; //*氢系统中最高温度探针代号

	*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxSolubility >> 8)&0xff; //*氢气最高浓度
	*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxSolubility >> 0)&0xff;

	*pos++ = p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxSolubilitySensorNum;    //*氢气最高溶度传感器代号*/

	*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxPressure >> 8)&0xff;   //*氢气最高压值
	*pos++ = (p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxPressure >> 0)&0xff;

	*pos++ = p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxPressureSensorNum;	     //*氢气最高压力传感器代号*/
	*pos++ = p_GBT32960_handle->dataInfo.fuelCellInfo.HighPressureDcDcState;			 //*高压DC/DC 状态
	
//	printf("====== FuelCellData Add Finish. ======\n");
	GBTLOG("add_FuelCellData:");
	for(i = 0; i<(pos-dataBuff); i++)
		GBT_NO("%02x ",*(dataBuff+i));
	GBT_NO("\n\n\n");

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : add_EngineData
* Description   : 添加发动机数据包
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::add_EngineData(uint8_t *dataBuff, uint16_t buffSize)
{
	uint16_t i;
	uint8_t *pos = dataBuff;

	GBTLOG("EngineData size:%d\n", buffSize);

	*pos++ = GB32960_EngineData;
	*pos++ = p_GBT32960_handle->dataInfo.engineInfo.engineState;  //*发动机状态
	*pos++ = (p_GBT32960_handle->dataInfo.engineInfo.engineCrankshaftSpeed >> 8)&0xff;  //*曲轴转速
	*pos++ = (p_GBT32960_handle->dataInfo.engineInfo.engineCrankshaftSpeed >> 0)&0xff;
	*pos++ = (p_GBT32960_handle->dataInfo.engineInfo.fuelConsumption >> 8)&0xff;        //*燃料消耗率*/
	*pos++ = (p_GBT32960_handle->dataInfo.engineInfo.fuelConsumption >> 0)&0xff;

	GBTLOG("add_EngineData:");
	for(i = 0; i<(pos-dataBuff); i++)
		GBT_NO("%02x ",*(dataBuff+i));
	GBT_NO("\n\n\n");

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : add_PositionData
* Description   : 增加车辆位置数据包
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::add_PositionData(uint8_t *dataBuff, uint16_t buffSize)
{
	uint16_t i;
	uint8_t *pos = dataBuff;

	GBTLOG("PositionData size:%d\n", buffSize);

	*pos++ = GB32960_PositionInfoData;
	*pos++ = p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState;
	*pos++ = (p_GBT32960_handle->dataInfo.positionInfo.Lon >>24)&0xffffffff;
	*pos++ = (p_GBT32960_handle->dataInfo.positionInfo.Lon >>16)&0xffffff;
	*pos++ = (p_GBT32960_handle->dataInfo.positionInfo.Lon >> 8)&0xffff;
	*pos++ = (p_GBT32960_handle->dataInfo.positionInfo.Lon >> 0)&0xff;
	*pos++ = (p_GBT32960_handle->dataInfo.positionInfo.Lat >>24)&0xffffffff;
	*pos++ = (p_GBT32960_handle->dataInfo.positionInfo.Lat >>16)&0xffffff;
	*pos++ = (p_GBT32960_handle->dataInfo.positionInfo.Lat >> 8)&0xffff;
	*pos++ = (p_GBT32960_handle->dataInfo.positionInfo.Lat >> 0)&0xff;

	//printf("add_PositionData:");
	//for(i = 0; i<(pos-dataBuff); i++)
		//printf("%02x ",*(dataBuff+i));
	//printf("\n\n\n"); //GBT_NO

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : add_ExtremeData
* Description   : 增加极值数据包
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::add_ExtremeData(uint8_t *dataBuff, uint16_t buffSize)
{
	uint16_t i;
	uint8_t *pos = dataBuff;
	GBTLOG("ExtremeData size:%d\n", buffSize);

	*pos++ = GB32960_ExtremeValueData;
	*pos++ = p_GBT32960_handle->dataInfo.extremeInfo.MaxVolBatterySubsysNumber;
	*pos++ = p_GBT32960_handle->dataInfo.extremeInfo.MaxVolBattery;
	*pos++ = (p_GBT32960_handle->dataInfo.extremeInfo.MaxBatteryVoltageValue >>8)&0xff;
	*pos++ = (p_GBT32960_handle->dataInfo.extremeInfo.MaxBatteryVoltageValue >>0)&0xff;
	*pos++ = p_GBT32960_handle->dataInfo.extremeInfo.MinVolBatterySubsysNumber;
	*pos++ = p_GBT32960_handle->dataInfo.extremeInfo.MinVolBattery;
	*pos++ = (p_GBT32960_handle->dataInfo.extremeInfo.MinBatteryVoltageValue >>8)&0xff;
	*pos++ = (p_GBT32960_handle->dataInfo.extremeInfo.MinBatteryVoltageValue >>0)&0xff;
	*pos++ = p_GBT32960_handle->dataInfo.extremeInfo.MaxTemperatureSubsysNumber;
	*pos++ = p_GBT32960_handle->dataInfo.extremeInfo.MaxTemperatureProbe;
	*pos++ = p_GBT32960_handle->dataInfo.extremeInfo.MaxTemperatureValue;
	*pos++ = p_GBT32960_handle->dataInfo.extremeInfo.MinTemperatureSubsysNumber;
	*pos++ = p_GBT32960_handle->dataInfo.extremeInfo.MinTemperatureProbe;
	*pos++ = p_GBT32960_handle->dataInfo.extremeInfo.MinTemperatureValue;

	GBTLOG("add_ExtremeData:");
	for(i = 0; i<(pos-dataBuff); i++)
		GBT_NO("%02x ",*(dataBuff+i));
	GBT_NO("\n\n\n");

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : add_AlarmData
* Description   : 增加报警数据包
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::add_AlarmData(uint8_t *dataBuff, uint16_t buffSize)
{
	uint16_t i;
	uint8_t *pos = dataBuff;
	
	GBTLOG("AlarmData size:%d\n", buffSize);

	*pos++ = GB32960_AlarmData;
	*pos++ = p_GBT32960_handle->dataInfo.alarmInfo.maxAlarmLevel;
	*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.generalAlarm.AlarmFlag >>24)&0xFF;
	*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.generalAlarm.AlarmFlag >>16)&0xFF;
	*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.generalAlarm.AlarmFlag >> 8)&0xFF;
	*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.generalAlarm.AlarmFlag >> 0)&0xFF;
	// 可充电储能装置故障
	if((p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEnergyStorageDev == 0xFF) ||
		(p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEnergyStorageDev == 0xFE))
	{
		*pos++ = p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEnergyStorageDev;
	}
	else
	{
		if(p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEnergyStorageDev > MAX_ALARM_NUM)
			p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEnergyStorageDev = MAX_ALARM_NUM;
		*pos++ = p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEnergyStorageDev;
		for(i = 0; i < p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEnergyStorageDev; i++)
		{
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.energyStorageDevFault[i].fault >>24)&0xFF;
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.energyStorageDevFault[i].fault >>16)&0xFF;
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.energyStorageDevFault[i].fault >> 8)&0xFF;
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.energyStorageDevFault[i].fault >> 0)&0xFF;
		}
	}
	//驱动电机故障
	if((p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfDriveMotor == 0xFF) ||
		(p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfDriveMotor == 0xFE))
	{
		*pos++ = p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfDriveMotor;
	}
	else
	{
		if(p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfDriveMotor > MAX_ALARM_NUM)
			p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfDriveMotor = MAX_ALARM_NUM;
		*pos++ = p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfDriveMotor;
		for(i = 0; i < p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfDriveMotor; i++)
		{
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.driveMotorFault[i].fault >>24)&0xFF;
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.driveMotorFault[i].fault >>16)&0xFF;
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.driveMotorFault[i].fault >> 8)&0xFF;
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.driveMotorFault[i].fault >> 0)&0xFF;
		}
		
	}
	//发动机故障
	if((p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEngine == 0xFF) ||
		(p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEngine == 0xFE))
	{	
		*pos++ = p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEngine;
	}
	else
	{
		if(p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEngine > MAX_ALARM_NUM)
			p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEngine = MAX_ALARM_NUM;
		*pos++ = p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEngine;

		for(i = 0; i < p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEngine; i++)
		{
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.engineFault[i].fault >>24)&0xFF;
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.engineFault[i].fault >>16)&0xFF;
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.engineFault[i].fault >> 8)&0xFF;
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.engineFault[i].fault >> 0)&0xFF;
		}
	}
	//其他故障
	if((p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfOther == 0xFF) ||
		(p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfOther == 0xFE))
	{
		*pos++ = p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfOther;
	}
	else
	{
		if(p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfOther > MAX_ALARM_NUM)		
			p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfOther = MAX_ALARM_NUM;
		*pos++ = p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfOther;

		for(i = 0; i < p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfOther; i++)
		{
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.otherFault[i].fault >>24)&0xFF;
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.otherFault[i].fault >>16)&0xFF;
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.otherFault[i].fault >> 8)&0xFF;
			*pos++ = (p_GBT32960_handle->dataInfo.alarmInfo.otherFault[i].fault >> 0)&0xFF;
		}
	}

	GBTLOG("add_AlarmData:");
	for(i = 0; i<(pos-dataBuff); i++)
		GBT_NO("%02x ",*(dataBuff+i));
	GBT_NO("\n\n\n");

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : add_EnergyStorageDevSubsysVolData
* Description   : 可充电储能装置电压数据包
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::add_EnergyStorageDevSubsysVolData(uint8_t *dataBuff, uint16_t buffSize)
{
	uint8_t i,j;
	uint8_t *pos = dataBuff;

	GBTLOG("EnergyStorageDevSubsysVolData size:%d\n", buffSize);
	
	*pos++ = GB32960_RechargeableDevVoltage;

	if(p_GBT32960_handle->dataInfo.ESDSubsysInfo.totalEnergySubsys > MAX_STOREENERGY_NUM)
		p_GBT32960_handle->dataInfo.ESDSubsysInfo.totalEnergySubsys = MAX_STOREENERGY_NUM;
	*pos++ = p_GBT32960_handle->dataInfo.ESDSubsysInfo.totalEnergySubsys;
	

	for(i = 0; i< p_GBT32960_handle->dataInfo.ESDSubsysInfo.totalEnergySubsys; i++)
	{
		if(p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysNumber > 250)
		{
			p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysNumber = 250;
		}
		*pos++ = p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysNumber;
		*pos++ = (p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysVol >>8)&0xff;
		*pos++ = (p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysVol >>0)&0xff;
		*pos++ = (p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysCurrent >>8)&0xff;
		*pos++ = (p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysCurrent >>0)&0xff;
		*pos++ = (p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].totalNumOfSingleCell >>8)&0xff;
		*pos++ = (p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].totalNumOfSingleCell >>0)&0xff;

		*pos++ = (p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].frameStartCellNumber >>8)&0xff;
		*pos++ = (p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].frameStartCellNumber >>0)&0xff;

		if(p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].frameStartCellTotal > MAX_FRAMESTARTCELL_TOTAL)
			p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].frameStartCellTotal = MAX_FRAMESTARTCELL_TOTAL;
		*pos++ = p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].frameStartCellTotal;
		
		for(j = 0; j < p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].frameStartCellTotal; j++)
		{
			
			*pos++ = (p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].monomerCellVol[j]>>8)&0xff;
			*pos++ = (p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].monomerCellVol[j]>>0)&0xff;
		}
	}

	GBTLOG("add_EnergyStorageDevSubsysVolData: %d\n",pos-dataBuff);
	/*for(i = 0; i<(pos-dataBuff); i++)
		GBT_NO("%02x ",*(dataBuff+i));
	GBT_NO("\n\n\n");*/

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : add_EnergyStorageDevSubsysVolData
* Description   : 可充电储能装置温度数据包
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::add_EnergyStorageDevSubsysTempData(uint8_t *dataBuff, uint16_t buffSize)
{
	uint8_t i,j;
	uint8_t *pos = dataBuff;

	GBTLOG("EnergyStorageDevSubsysTempData size:%d\n", buffSize);

	*pos++ = GB32960_RechargeableDevTemp;

	if(p_GBT32960_handle->dataInfo.ESDSubsysInfo.totalEnergySubsys > MAX_STOREENERGY_NUM)
		p_GBT32960_handle->dataInfo.ESDSubsysInfo.totalEnergySubsys = MAX_STOREENERGY_NUM;
	*pos++ = p_GBT32960_handle->dataInfo.ESDSubsysInfo.totalEnergySubsys;

	for(i = 0; i< p_GBT32960_handle->dataInfo.ESDSubsysInfo.totalEnergySubsys; i++)
	{
		if(p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysNumber > 250)
		{
			p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysNumber = 250;
		}
		*pos++ = p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysNumber;

		if(p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysProbeNumber > MAX_PROBE_NUM)
			p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysProbeNumber = MAX_PROBE_NUM;

		*pos++ = (p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysProbeNumber >>8)&0xff;
		*pos++ = (p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysProbeNumber >>0)&0xff;

		
		for(j = 0; j < p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysProbeNumber; j++)
		{
			*pos++ = p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysProbeTemper[j];
		}
	}

	GBTLOG("add_EnergyStorageDevSubsysTempData:");
	/*for(i = 0; i<(pos-dataBuff); i++)
		GBT_NO("%02x ",*(dataBuff+i));
	GBT_NO("\n\n\n");*/

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : pack_GBT32960_ReissueInfoReport
* Description   : 打包补报信息
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::pack_GBT32960_ReissueInfoReport(uint8_t *dataBuff, uint16_t buffSize)
{
uint8_t *pos = dataBuff;
	uint16_t dataLen = 0;
	uint16_t offset;

	

	int len;
	unsigned int id;
	uint8_t *pBuff = (uint8_t *)malloc(DATA_BUFF_LEN);
	pthread_mutex_lock(&Goldenlock);
	id = pSqliteDatabase->queryUnsendID();
	printf("id is %d\n",id);
	if(faultLevel == 3 && !alarmlever)
	{
		printf("12312312312312 ididididid\n");
		id = pSqliteDatabase->queryMaxTimesID();
		printf("id is %d\n",id);
		if(id == 0)
		{
			pthread_mutex_unlock(&Goldenlock);
			return 0;
		}
		if(id > 40)
		{
				for(int i =0;i<40;i++)
			{
				pSqliteDatabase->updateisSend(0,id - i);
			}
		}
		else
		{
			for(int i =0;i<id;i++)
			{
				pSqliteDatabase->updateisSend(0,id - i);
			}
		}
		
		
		
	}
	if(id == 0)
	{
		pthread_mutex_unlock(&Goldenlock);
		return 0;
	}
	
	
	memset(pBuff, 0, DATA_BUFF_LEN);
	if(!pSqliteDatabase->readStandardProData(pBuff, &len, id))
		{
			reissue = true;
			//mcuUart::m_mcuUart->unpack_updatePositionInfo(pBuff, len);
			uint16_t WTempData = 0;
			WTempData = *(pBuff+20);
			WTempData = ((WTempData&0xff) << 8) | *(pBuff+21);
			if(WTempData > 2200)
			{
				WTempData = 0xfffe;
			}
			p_GBT32960_handle->dataInfo.vehicleInfo.vehicleSpeed = WTempData;

			mcuUart::m_mcuUart->unpack_updateGBT32960PositionInfo(pBuff, len);
			mcuUart::m_mcuUart->unpack_updateTimingInfo(pBuff + 30, len);
			pSqliteDatabase->updateisSend(1,id);

			#if 0
			struct tm *p_tm = NULL;
			p_tm->tm_year = pBuff[24]; /* 年份，其值等于实际年份减去1900 */
			p_tm->tm_mon = pBuff[25];    /* 月份（从一月开始，0代表一月）-取值区间为[0,11] */
			p_tm->tm_mday = pBuff[26];     /* 一个月中的日期 - 取值区间为[1,31] */
			p_tm->tm_hour = pBuff[27];     /* 时 - 取值区间为[0,23] */
			p_tm->tm_min = pBuff[28];      /* 分 - 取值区间为[0,59] */
			p_tm->tm_sec = pBuff[29];      /* 秒 – 取值区间为[0,59] */

			
			//time_t tmp_time;
			//tmp_time = time(NULL);
			//p_tm = gmtime(&tmp_time);
			//GBTLOG("TIME:%0d-%d-%d:%02d:%02d:%02d",p_tm->tm_year+1900, p_tm->tm_mon,p_tm->tm_mday, p_tm->tm_hour,p_tm->tm_min,p_tm->tm_sec);
			
			
			//time
			*pos++ = p_tm->tm_year;
			*pos++ = p_tm->tm_mon;
			*pos++ = p_tm->tm_mday;
			*pos++ = p_tm->tm_hour;
			*pos++ = p_tm->tm_min;
			*pos++ = p_tm->tm_sec;
			#endif
			*pos++ = *(pBuff+24);
			*pos++ = *(pBuff+25);
			*pos++ = *(pBuff+26);
			*pos++ = *(pBuff+27);
			*pos++ = *(pBuff+28);
			*pos++ = *(pBuff+29);



			//整车数据0x01
			offset = pos-dataBuff;
			GBTLOG("pos:%x,pos+offset=%x,offset:%d\n", pos,(pos+offset),offset);	
			if((dataLen = add_VehicleData(pos, buffSize-offset)) == 0)
			{
				GBTLOG("add_VehicleData error!");
				return 0;
			}
			pos += dataLen;

			//驱动电机数据0x02
			offset = pos-dataBuff;
			GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
			if((dataLen = add_DriveMotorData(pos, buffSize-offset)) == 0)
			{
				GBTLOG("add_VehicleData error!");
				return 0;
			}
			pos += dataLen;
			
			#if 1
			//燃料电池数据0x03
			offset = pos-dataBuff;
			GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
			if((dataLen = add_FuelCellData(pos, buffSize-offset)) == 0)
			{
				GBTLOG("add_FuelCellData error!");
				return 0;
			}
			pos += dataLen;
			#endif

			//发动机数据0x04
			offset = pos-dataBuff;
			GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);

			/*if((dataLen = add_EngineData(pos, buffSize-offset)) == 0)
			{
				GBTLOG("add_EngineData error!");
				return 0;
			}
			pos += dataLen;

			//车辆位置数据0x05
			offset = pos-dataBuff;*/
			GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
			if((dataLen = add_PositionData(pos, buffSize-offset)) == 0)
			{
				GBTLOG("add_PositionData error!");
				return 0;
			}
			pos += dataLen;
	
			//极值数据x06
			offset = pos-dataBuff;
			GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
			if((dataLen = add_ExtremeData(pos, buffSize-offset)) == 0)
			{
				GBTLOG("add_ExtremeData error!");
				return 0;
			}
			pos += dataLen;

			//报警数据0x07
			offset = pos-dataBuff;
			GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
			if((dataLen = add_AlarmData(pos, buffSize-offset)) == 0)
			{
				GBTLOG("add_AlarmData error!");
				return 0;
			}
			pos += dataLen;

			//可充电储能装置电压数0x08
			offset = pos-dataBuff;
			GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
			if((dataLen = add_EnergyStorageDevSubsysVolData(pos, buffSize-offset)) == 0)
			{
				GBTLOG("add_EnergyStorageDevSubsysData error!");
				return 0;
			}
			pos += dataLen;

			//可充电储能装置温度数0x09
			offset = pos-dataBuff;
			GBTLOG("offset:%d,dataLen:%d\n",offset,dataLen);
			if((dataLen = add_EnergyStorageDevSubsysTempData(pos, buffSize-offset)) == 0)
			{
				GBTLOG("add_EnergyStorageDevSubsysData error!");
				return 0;
			}
			pos += dataLen;

			pthread_mutex_unlock(&Goldenlock);
			//pthread_mutex_unlock(&Goldenlock);
			//timingReport(GBT32960_ReissueInfoReportID);
		}
		else
		{
			pthread_mutex_unlock(&Goldenlock);
		}


		if(pBuff != NULL)
	{
		free(pBuff);
		pBuff = NULL;
	}
	
	
	reissue = false;
	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : pack_GBT32960_VehiceLogout
* Description   : 打包车辆登出
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::pack_GBT32960_VehiceLogout(uint8_t *dataBuff, uint16_t buffSize)
{
	uint8_t *pos = dataBuff;

	struct tm *p_tm = NULL;
	time_t tmp_time;
	tmp_time = time(NULL);
	p_tm = gmtime(&tmp_time);
	GBTLOG("TIME:%0d-%d-%d:%02d:%02d:%02d",p_tm->tm_year+1900, p_tm->tm_mon+1,p_tm->tm_mday, p_tm->tm_hour,p_tm->tm_min,p_tm->tm_sec);

	//time
	*pos++ = p_tm->tm_year - 100;
	*pos++ = p_tm->tm_mon + 1;
	*pos++ = p_tm->tm_mday;
	*pos++ = p_tm->tm_hour;
	*pos++ = p_tm->tm_min;
	*pos++ = p_tm->tm_sec;

	//login serial number
	*pos++ = (p_GBT32960_handle->dataInfo.serialNumber >> 8)&0xFF;
	*pos++ = (p_GBT32960_handle->dataInfo.serialNumber >> 0)&0xFF;

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : pack_GBT32960_QueryCommand
* Description   : 打包查询参数
* Input			: uint8_t *dataBuff           
*                 uint16_t buffSize
*                 uint8_t *pInBuff
*                 uint16_t len
* Output        : None
* Return        : uint16_t 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t GBT32960::pack_GBT32960_QueryCommand(uint8_t *dataBuff, uint16_t buffSize, uint8_t *pInBuff, uint16_t len)
{
	uint16_t i;
	uint8_t *pos = dataBuff;
	uint8_t *posIn = pInBuff;
	uint8_t totalNum = pInBuff[30];
	uint8_t queryPara;

	struct tm *p_tm = NULL;
	time_t tmp_time;
	tmp_time = time(NULL);
	p_tm = gmtime(&tmp_time);

	//Query time
	/**pos++ = p_tm->tm_year - 100;
	*pos++ = p_tm->tm_mon + 1;
	*pos++ = p_tm->tm_mday;
	*pos++ = p_tm->tm_hour;
	*pos++ = p_tm->tm_min;
	*pos++ = p_tm->tm_sec;*/
	for(int i =0; i <6;i++)
	{
			*pos++ = controltime[i];
	}
	

	//Total number of query
	GBTLOG("pack GBT32960 QueryCommand->posIn:%d,totalNum:%d\n",*(posIn+30), totalNum);
	posIn += 30;
	GBTLOG("posIn:%d\n",*posIn);
	*pos++ = *posIn++;

	for (i = 0; i < totalNum; i++)
	{
		queryPara = *posIn++;
		GBTLOG("queryPara:%02x\n",queryPara);
		switch (queryPara)
		{
			case 0x01:
				*pos++ = queryPara;
	       	    *pos++ = (p_GBT32960_handle->configInfo.localStorePeriod &0xff00)>>8;
	       	    *pos++ = (p_GBT32960_handle->configInfo.localStorePeriod &0xff)>>0;
				break;
			case 0x02:
				*pos++ = queryPara;
	       	    *pos++ = (p_GBT32960_handle->configInfo.timingReportTimeInterval &0xff00)>>8;
	       	    *pos++ = (p_GBT32960_handle->configInfo.timingReportTimeInterval &0xff)>>0;
				break;
			case 0x03:
				*pos++ = queryPara;
	       	    *pos++ = (p_GBT32960_handle->configInfo.alarmInfoReportPeriod &0xff00)>>8;
	       	    *pos++ = (p_GBT32960_handle->configInfo.alarmInfoReportPeriod &0xff)>>0;
				break;
			case 0x04:
				*pos++ = queryPara;
	       	    *pos++ = p_GBT32960_handle->configInfo.remoteServiceDomainNameLen;
				break;
			case 0x05:
				*pos++ = queryPara;
				memcpy(pos,p_GBT32960_handle->configInfo.remoteServiceDomainName, p_GBT32960_handle->configInfo.remoteServiceDomainNameLen);
       	    	pos += p_GBT32960_handle->configInfo.remoteServiceDomainNameLen;
				break;
			case 0x06:
				*pos++ = queryPara;
	       	    *pos++ = (p_GBT32960_handle->configInfo.remoteServicePort &0xff00)>>8;
	       	    *pos++ = (p_GBT32960_handle->configInfo.remoteServicePort &0xff)>>0;
				break;
			case 0x07:
				*pos++ = queryPara;
				//memcpy(pos, p_GBT32960_handle->configInfo.hardwareVersion, sizeof(p_GBT32960_handle->configInfo.hardwareVersion));
       	    	//pos += sizeof(p_GBT32960_handle->configInfo.hardwareVersion);
				memcpy(pos,"V1.00",5);
				pos += 5;
				//*pos++ = '\0';
				break;
			case 0x08:
				*pos++ = queryPara;
				//memcpy(pos, p_GBT32960_handle->configInfo.firmwareVersion, sizeof(p_GBT32960_handle->configInfo.firmwareVersion));
       	    	//pos += sizeof(p_GBT32960_handle->configInfo.firmwareVersion);
				memcpy(pos,"V1.00",5);
				pos += 5;
				//*pos++ = '\0';
				break;
			case 0x09:
				*pos++ = queryPara;
       	    	*pos++ = p_GBT32960_handle->configInfo.heartbeatInterval;
				break;
			case 0x0A:
				*pos++ = queryPara;
	       	    *pos++ = (p_GBT32960_handle->configInfo.terminalResponseTimeout &0xff00)>>8;
	       	    *pos++ = (p_GBT32960_handle->configInfo.terminalResponseTimeout &0xff)>>0;
				break;
			case 0x0B:
				*pos++ = queryPara;
	       	    *pos++ = (p_GBT32960_handle->configInfo.platformResponseTimeout &0xff00)>>8;
	       	    *pos++ = (p_GBT32960_handle->configInfo.platformResponseTimeout &0xff)>>0;
				break;
			case 0x0C:
				*pos++ = queryPara;
	       	    *pos++ = p_GBT32960_handle->configInfo.logonFailureInterval;
				break;
			case 0x0D:
				*pos++ = queryPara;
	       	    *pos++ = p_GBT32960_handle->configInfo.publicDomainNameLen;
				break;
			case 0x0E:
				*pos++ = queryPara;
				memcpy(pos, p_GBT32960_handle->configInfo.publicDomainName, p_GBT32960_handle->configInfo.publicDomainNameLen);
       	    	pos += p_GBT32960_handle->configInfo.publicDomainNameLen;
				break;
			case 0x0F:
				*pos++ = queryPara;
	       	    *pos++ = (p_GBT32960_handle->configInfo.publicPort &0xff00)>>8;
	       	    *pos++ = (p_GBT32960_handle->configInfo.publicPort &0xff)>>0;
				break;
			case 0x10:
				*pos++ = queryPara;
	       	    *pos++ = p_GBT32960_handle->configInfo.IsSamplingCheck;
				break;
			default:
				break;
		}
	}
	
	return (uint16_t)(pos-dataBuff);
}

/****************************************************************
*              unpack data packet
*****************************************************************/

/*****************************************************************************
* Function Name : checkReceivedData
* Description   : 接收数据的检验
* Input			: uint8_t *pData           
*                 int32_t len
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int8_t GBT32960::checkReceivedData(uint8_t *pData, int32_t len)
{
	uint8_t *pos = pData;
	//uint16_t dataLen;
	uint8_t packetCheckCode;

	if(len < 24)
		return -1;
	
	if((pData[0] == 0x23) && (pData[1] == 0x23))
	{
		if(memcmp(p_GBT32960_handle->dataInfo.vin, pos+4, 17) != 0)
		{
			GBTLOG("Received data vin error.\n");
			return -1;
		}

		packetCheckCode = pData[len-1];
		if(packetCheckCode != checkSum_BCC(pos+2, len-2-1))
		{
			GBTLOG("Check data error!");
			return -1;
		}
		
		GBTLOG("Check data ok,packetCheckCode:%x, checkSum_BCC:%x.\n", packetCheckCode, checkSum_BCC(pos+2, len-2-1));

		return 0;
	}

	return -1;
}

/*****************************************************************************
* Function Name : unpack_GBT32960FrameData
* Description   : 解析协议
* Input			: uint8_t *pData           
*                 int32_t len
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int8_t GBT32960::unpack_GBT32960FrameData(uint8_t *pData, int32_t len)
{
	uint8_t cmdId;
	uint8_t responseSigns;
	
	cmdId = pData[2];
	responseSigns = pData[3];

	GBTLOG("unpack GBT32960 FrameData cmdId:%02x, responseSigns:%02x",cmdId, responseSigns);

	switch(cmdId)
	{
		//服务器应答的指令
		case GBT32960_VehiceLoginID:	//车辆登陆
			GBTLOG("unpack GBT32960 Data VehiceLoginID.\n");
			if(responseSigns == GBT32960_RESPONSE_SUCCESS)
			{
				loginState = 1;
				SettoLoginState = 1;
				GBTLOG("GBT32960 VehiceLoginID success!\n");
			}
			else
			{
				GBTLOG("GBT32960 VehiceLoginID fail,re-login after 1 minute .\n");
			}
			break;
		case GBT32960_RealTimeInfoReportID:	//实时信息上报数据返回
			GBTLOG("unpack GBT32960 Data RealTimeInfoReportID.\n");
			if(responseSigns == GBT32960_RESPONSE_ERROR)
			{
					
				GBTLOG("GBT32960 RealTimeInfoReportID fail,re-Report after 1 minute .\n");
			}
			break;
		case GBT32960_ReissueInfoReportID://补发数据
			break;
		case GBT32960_VehiceLogoutID:
			if(responseSigns == GBT32960_RESPONSE_SUCCESS)
				loginState = 2;
			break;
		/*case GBT32960_PlatformLoginID:
			break;
		case GBT32960_PlatformLogoutID:
			break;*/
		case GB32960_HeartBeatID:
			break;
		case GB32960_TerminalCalibrationTime:
			break;
		//服务器下发的指令
		case GB32960_QueryCommandID:	//查询参数
			GBTLOG("unpack GBT32960 Data QueryCommandID.\n");
			unpack_QueryCommand(pData, len);
			break;
		case GB32960_SetCommandID:		//设置参数
			GBTLOG("unpack GBT32960 Data SetCommandID.\n");
			unpack_SetCommand(pData, len);
			break;
		case GB32960_ControlCommandID:	//控制命令
			GBTLOG("unpack GBT32960 Data ControlCommandID.\n");
			unpack_ControlCommand(pData, len);
			break;
		default:
			GBTLOG("unpack command error!");
			break;
		
	}
	
	return 0;
}

/*****************************************************************************
* Function Name : unpack_QueryCommand
* Description   : 解析查询参数命令
* Input			: uint8_t *pData
*                 int32_t len
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void GBT32960::unpack_QueryCommand(uint8_t *pData, int32_t len)
{
	int length;
	
	memcpy(controltime,pData+24,6);
	for(int i =0;i<6;i++)
	{
		printf("control time is %d\n",controltime[i]);
	}
	
	uint8_t *dataBuff = (uint8_t *)malloc(DATA_BUFF_LEN);
	if(dataBuff == NULL)
	{
		GBTLOG("malloc dataBuff error!");
		return;
	}
	memset(dataBuff, 0, DATA_BUFF_LEN);

	length = pack_GBT32960FrameData(dataBuff, DATA_BUFF_LEN, GB32960_QueryCommandID, GBT32960_RESPONSE_SUCCESS,pData,len);
	if((length = send(socketfd, dataBuff, length, 0)) < 0)
	{
		GBTLOG("Send Querycommand data failed.\n");
		free(dataBuff);
		dataBuff = NULL;
		close(socketfd);
		isConnected = false;
		resend = false;
		return;
	}
	else
	{
		GBTLOG("Send Querycommand %d data ok\n",length);
		recvFlag = 0;
		
		free(dataBuff);
		dataBuff = NULL;
	}
}

/*****************************************************************************
* Function Name : unpack_SetCommand
* Description   : 解析设置参数
* Input			: uint8_t *pData
*                 int32_t len
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void GBT32960::unpack_SetCommand(uint8_t *pData, int32_t len)
{
	uint8_t i;
	memcpy(controltime,pData+24,6);
	for(int i =0;i<6;i++)
	{
		printf("control time is %d\n",controltime[i]);
	}
	int length;
	uint8_t totalParameter,paraId;
	uint8_t *pos = pData;
	int ReLogIn = 0;  
	SettoLoginState = 0;

	pos += 30;
	totalParameter = *pos++;
	GBTLOG("unpack SetCommand totalParameter:%d\n",totalParameter);

	//update parameter
	updateTBoxParameterInfo();

	for (i = 0; i < totalParameter; i++)
	{
		paraId = pos[0];
		GBTLOG("paraId:%02x,",paraId);
		
		pos++;
		switch (paraId)
		{
			case 0x01:
				for(int len = 0; len < 2; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				p_GBT32960_handle->configInfo.localStorePeriod = (pos[0] << 8) + pos[1];
				pos += 2;
				break;
			case 0x02:
				for(int len = 0; len < 2; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				p_GBT32960_handle->configInfo.timingReportTimeInterval = (pos[0] << 8) + pos[1];
				pos += 2;
				break;
			case 0x03:
				for(int len = 0; len < 2; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				p_GBT32960_handle->configInfo.alarmInfoReportPeriod = (pos[0] << 8) + pos[1];
				pos += 2;
				break;
			case 0x04:	//服务器平台域名
				for(int len = 0; len < 1; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				p_GBT32960_handle->configInfo.remoteServiceDomainNameLen  = pos[0];
				pos++;
				break;
			case 0x05:
				//域名值
				for(int len = 0; len < p_GBT32960_handle->configInfo.remoteServiceDomainNameLen; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				if(p_GBT32960_handle->configInfo.remoteServiceDomainNameLen > DOMAIN_LEN)
					p_GBT32960_handle->configInfo.remoteServiceDomainNameLen = DOMAIN_LEN;
				memset(p_GBT32960_handle->configInfo.remoteServiceDomainName, 0, p_GBT32960_handle->configInfo.remoteServiceDomainNameLen);
		        memcpy(p_GBT32960_handle->configInfo.remoteServiceDomainName, pos, p_GBT32960_handle->configInfo.remoteServiceDomainNameLen);
				pos += p_GBT32960_handle->configInfo.remoteServiceDomainNameLen;
				ReLogIn++;
				break;
			case 0x06:
				for(int len = 0; len < 2; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				p_GBT32960_handle->configInfo.remoteServicePort = (pos[0] << 8) + pos[1];
				pos += 2;
				ReLogIn++;
				break;
				//不作设置
			/*case 0x07:
				memset(p_GBT32960_handle->configInfo.hardwareVersion, 0, sizeof(p_GBT32960_handle->configInfo.hardwareVersion));
				memcpy(p_GBT32960_handle->configInfo.hardwareVersion, pos, sizeof(p_GBT32960_handle->configInfo.hardwareVersion));
				pos += sizeof(p_GBT32960_handle->configInfo.hardwareVersion);
				break;
			case 0x08:
				memset(p_GBT32960_handle->configInfo.firmwareVersion, 0, sizeof(p_GBT32960_handle->configInfo.firmwareVersion));
				memcpy(p_GBT32960_handle->configInfo.firmwareVersion, pos, sizeof(p_GBT32960_handle->configInfo.firmwareVersion));
				pos += sizeof(p_GBT32960_handle->configInfo.firmwareVersion);
				break;*/
			case 0x09:
				for(int len = 0; len < 1; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				p_GBT32960_handle->configInfo.heartbeatInterval = pos[0];
				pos++;
				break;
			case 0x0A:
				for(int len = 0; len < 2; ++len)
					printf("%02x ", *(pos+len));
				printf("\n");
				p_GBT32960_handle->configInfo.terminalResponseTimeout = (pos[0] << 8) + pos[1];
				pos += 2;
				break;
			case 0x0B:
				for(int len = 0; len < 2; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				p_GBT32960_handle->configInfo.platformResponseTimeout = (pos[0] << 8) + pos[1];
				pos += 2;
				break;
			case 0x0C:
				for(int len = 0; len < 1; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				p_GBT32960_handle->configInfo.logonFailureInterval = pos[0];
				pos++;
				break;
			case 0x0D://公共平台域名
				for(int len = 0; len < 1; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				p_GBT32960_handle->configInfo.publicDomainNameLen = pos[0];
				pos++;
				break;
			case 0x0E:
				//域名值
				for(int len = 0; len < p_GBT32960_handle->configInfo.publicDomainNameLen; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				if(p_GBT32960_handle->configInfo.publicDomainNameLen > DOMAIN_LEN)
					p_GBT32960_handle->configInfo.publicDomainNameLen = DOMAIN_LEN;
				memset(p_GBT32960_handle->configInfo.publicDomainName, 0, p_GBT32960_handle->configInfo.publicDomainNameLen);
		        memcpy(p_GBT32960_handle->configInfo.publicDomainName, pos, p_GBT32960_handle->configInfo.publicDomainNameLen);
				pos += p_GBT32960_handle->configInfo.publicDomainNameLen;
				
				break;
			/*case 0x0E:
				memset(p_GBT32960_handle->configInfo.publicDomainName, 0, sizeof(p_GBT32960_handle->configInfo.publicDomainName));
		        memcpy(p_GBT32960_handle->configInfo.publicDomainName, pos, sizeof(p_GBT32960_handle->configInfo.publicDomainName));
				pos += sizeof(p_GBT32960_handle->configInfo.publicDomainName);
				break;*/
			case 0x0F:
				for(int len = 0; len < 2; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				p_GBT32960_handle->configInfo.publicPort = (pos[0] << 8) + pos[1];
				pos += 2;
				break;
			case 0x10:
				for(int len = 0; len < 1; ++len)
					printf("%02x ", *(pos+len));
                printf("\n");
				p_GBT32960_handle->configInfo.IsSamplingCheck = pos[0];
				pos++;
				break;
			default:
				break;
		}
	}

	uint8_t *dataBuff = (uint8_t *)malloc(DATA_BUFF_LEN);
	if(dataBuff == NULL)
	{
		GBTLOG("malloc dataBuff error!");
		return;
	}
	memset(dataBuff, 0, DATA_BUFF_LEN);

	length = pack_GBT32960FrameData(dataBuff, DATA_BUFF_LEN, GB32960_SetCommandID, GBT32960_RESPONSE_SUCCESS,pData,len);
	
	if((length = send(socketfd, dataBuff, length, 0)) < 0)
	{
		GBTLOG("Send data failed.\n");
		free(dataBuff);
		dataBuff = NULL;
		close(socketfd);
		isConnected = false;
		resend = false;
		return;
	}
	GBTLOG("Send %d data ok!\n",length);
	recvFlag = 0;
	updateTBoxParameterInfo();
	if(2 == ReLogIn)
	{
		sendLoginData();	
		if(1 == SettoLoginState)
		{
			//update parameter
			updateTBoxParameterInfo();
		}
		else
		{
			memset(dataBuff, 0, DATA_BUFF_LEN);

			length = pack_GBT32960FrameData(dataBuff, DATA_BUFF_LEN, GB32960_SetCommandID, GBT32960_RESPONSE_ERROR,pData,len);
			
			if((length = send(socketfd, dataBuff, length, 0)) < 0)
			{
				GBTLOG("Send data failed.\n");
				free(dataBuff);
				dataBuff = NULL;
				close(socketfd);
				isConnected = false;
				resend = false;
				return;
			}
			GBTLOG("Send %d data ok!\n",length);
			recvFlag = 0;
	
			//get previous data
			readTBoxParameterInfo();
		}
	}
	
	free(dataBuff);
	dataBuff = NULL;
}

/*****************************************************************************
* Function Name : unpack_ControlCommand
* Description   : 解包获取控制命令
* Input			: uint8_t *pData
*                 int32_t len
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void GBT32960::unpack_ControlCommand(uint8_t *pData, int32_t len)
{
	int length;
	uint8_t cmdId;
	memcpy(controltime,pData+24,6);
	for(int i =0;i<6;i++)
	{
		printf("control time is %d\n",controltime[i]);
	}
	uint8_t alarm;
	cmdId = pData[30];
	alarm = pData[31];
	GBTLOG("unpack  Control Command cmdId:%d\n",cmdId);
	

	int Pasvfd;
	int PasvPort;
	char PasvIp[32];
	memset(PasvIp, 0, sizeof(PasvIp));
	uint8_t cmd[256];
	uint8_t output[16]; 
	switch (cmdId)
	{
		case 0x01:			//远程升级
			{			
			/*GBTLOG("222222\n");
			unpack_ftpUpgrade(&pData[30], len-30);
			FTPClient * ftpclient = new FTPClient(p_GBT32960_handle->ftp.domainIP, p_GBT32960_handle->ftp.port);
			if(ftpclient->loginFTPServer(p_GBT32960_handle->ftp.userName, p_GBT32960_handle->ftp.password) == -1)
			{
				return -1;
			}
			if(ftpclient->setFTPServerToPasvMode(PasvIp, &PasvPort) == -1)
			{
				return -1;
			}
			if(ftpclient->connectPasvServer(PasvIp, PasvPort, &Pasvfd) != -1)
			{
					
			GBTLOG("download lte version fileName1:%s,",p_GBT32960_handle->ftp.URLAddress);
				if(ftpclient->getFile(Pasvfd, p_GBT32960_handle->ftp.URLAddress, "/data/tbox_180625") == 0)
				{
					GBTLOG("getfile failed");
				}
			ftpclient->quitFTP();		
			}*/
				/*unpack_ftpUpgrade(&pData[30], len-30);
				if(p_GBT32960_handle->ftp.userName[0] == 0)
				{
					if (!access(LTE_FILE_NAME, F_OK))
					{
						printf("22Exisist tbox upgrade file, remove it!\n");
						system(RM_LTE_FILE);
					}
					sprintf(cmd, "wget \"%s\" -O \"%s\"", p_GBT32960_handle->ftp.URLAddress, "/data/LteUpgrade.bin");
					printf("cmd is %s\n",cmd);
					system(cmd);
					memset(output,0,16);
					m_Hmacmd5OpData.md5test("/data/LteUpgrade.bin",output);
					for(int j=0;j<16;j++)
					{
						if(output[j]!=p_GBT32960_handle->ftp.fileName[j])
						{
							system(RM_LTE_FILE);
							printf("md5 error\n");
							return 0;
						}
					}
					system("sync");
					wds_qmi_release();
					nas_qmi_release();
					voice_qmi_release();
					mcm_sms_release();
					mcuUart::m_mcuUart->close_uart();
					LteAtCtrl->~LTEModuleAtCtrl();
					printf("################### exit 2 ###########################\n");
					//Headerlink=NULL;
					system("reboot");
					exit(0);
				}*/
			}	
			break;
		case 0x02:			//车载终端关机
			mcuUart::m_mcuUart->packProtocolData(0x82,0x51,NULL,1,0);
			closeTbox = 1;
			break;
		case 0x03:			//车载终端复位
			mcuUart::m_mcuUart->packProtocolData(0x82,0x56,NULL,1,0);
			closeTbox = 2;
			sleep(2);
    		system("reboot");
			break;
		case 0x04:			//恢复出厂设置

			/*system(DELETE_PARAMETER);
			system("sync");
			disconnectSocket();
			system("pkill tbox");
			initTBoxParameterInfo();*/
			break;
		case 0x05:			//断开数据通信链路
			//close(socketfd);
			closeTbox = 3;
			//disconnectSocket();
			break;
		case 0x06:			//车载终端预警
			if(alarm == 0x01)
				{//closeTbox=4;
					//p_GBT32960_handle->dataInfo.alarmInfo.maxAlarmLevel = 1;
				}
			else if(alarm == 0x02)
				{//closeTbox=5;
					//p_GBT32960_handle->dataInfo.alarmInfo.maxAlarmLevel = 2;
				}
			else if(alarm == 0x03)
				//closeTbox=6;
				{
					//p_GBT32960_handle->dataInfo.alarmInfo.maxAlarmLevel = 3;
				}
			break;
		case 0x07:			//开启抽样监测链路
			break;
		default:
			break;
	}

	
	uint8_t *dataBuff = (uint8_t *)malloc(DATA_BUFF_LEN);
	if(dataBuff == NULL)
	{
		GBTLOG("malloc dataBuff error!");
		return;
	}
	memset(dataBuff, 0, DATA_BUFF_LEN);

	length = pack_GBT32960FrameData(dataBuff, DATA_BUFF_LEN, GB32960_ControlCommandID, GBT32960_RESPONSE_SUCCESS,pData,len);
	
	if((length = send(socketfd, dataBuff, length, 0)) < 0)
	{
		GBTLOG("Send data failed.\n");
		free(dataBuff);
		dataBuff = NULL;
		close(socketfd);
		isConnected = false;
		resend = false;
		return;
	}
	GBTLOG("Send %d data ok!\n",length);
	recvFlag = 0;

	free(dataBuff);
	dataBuff = NULL; 

	if(cmdId == 0x04)
	{
		system(DELETE_PARAMETER);
		system("sync");
		disconnectSocket();
		system("pkill tbox");
		initTBoxParameterInfo();
	}

}

/*****************************************************************************
* Function Name : unpack_ftpUpgrade
* Description   : 解包获取ftp升级信息
* Input			: uint8_t *pData
*                 int32_t len
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void GBT32960::unpack_ftpUpgrade(uint8_t *pData, int32_t len)
{
	uint8_t i;
	uint8_t *posHead = NULL;
	uint8_t *posTail = NULL;
	uint8_t *pos = NULL;
	char buff[4];

	//url升级地址
	posHead = getAddrFromBuff(pData,len,0x01,1);
	posTail = getAddrFromBuff(pData,len,';',1);

	if(posHead != NULL)
		posHead++;
	if((posTail > posHead)&&(posTail <= posHead+sizeof(p_GBT32960_handle->ftp.URLAddress)))
	{
		memset(p_GBT32960_handle->ftp.URLAddress, 0, sizeof(p_GBT32960_handle->ftp.URLAddress));
		memcpy(p_GBT32960_handle->ftp.URLAddress, posHead, posTail-posHead);

		GBTLOG("UpgradeInfo:URL : ");
		for(i = 0; i < (posTail-posHead); i++)
			GBT_NO("%02x ", p_GBT32960_handle->ftp.URLAddress[i]);
		GBT_NO("\n");
		//p_GBT32960_handle->ftp.URLAddress[posTail-posHead]="\0";
		GBT_NO("%s\n", p_GBT32960_handle->ftp.URLAddress);
	}

	//*拨号点名称
	posHead = getAddrFromBuff(pData,len,';',1);
	posTail = getAddrFromBuff(pData,len,';',2);

	if(posHead != NULL)
		posHead++;
	if((posTail > posHead)&&(posTail <= posHead+sizeof(p_GBT32960_handle->ftp.fileName)))
	{
		memcpy(p_GBT32960_handle->ftp.fileName, posHead, posTail-posHead);

		GBTLOG("UpgradeInfo:fileName : ");
		for(i = 0; i < (posTail-posHead); i++)
			GBT_NO("%02x ", p_GBT32960_handle->ftp.fileName[i]);
		GBT_NO("\n");
		GBT_NO("%s\n", p_GBT32960_handle->ftp.fileName);
	}

	//*用户名
	posHead = getAddrFromBuff(pData,len,';',2);
	posTail = getAddrFromBuff(pData,len,';',3);

	if(posHead != NULL)
		posHead++;
	if((posTail > posHead)&&(posTail <= posHead+sizeof(p_GBT32960_handle->ftp.userName)))
	{
		memcpy(p_GBT32960_handle->ftp.userName, posHead, posTail-posHead);

		GBTLOG("UpgradeInfo:userName : ");
		for(i = 0; i < (posTail-posHead); i++)
			GBT_NO("%02x ", p_GBT32960_handle->ftp.userName[i]);
		GBT_NO("\n");
		GBT_NO("userName : %s\n", p_GBT32960_handle->ftp.userName);
	}

	/*密码*/
	posHead = getAddrFromBuff(pData,len,';',3);
	posTail = getAddrFromBuff(pData,len,';',4);

	if(posHead != NULL)
		posHead++;
	if((posTail > posHead)&&(posTail <= posHead+sizeof(p_GBT32960_handle->ftp.password)))
	{
		memcpy(p_GBT32960_handle->ftp.password, posHead, posTail-posHead);

		GBTLOG("UpgradeInfo:password : ");
		for(i = 0; i < (posTail-posHead); i++)
			GBT_NO("%02x ", p_GBT32960_handle->ftp.password[i]);
		GBT_NO("\n");
		GBT_NO("password : %s\n", p_GBT32960_handle->ftp.password);
	}

	//IP地址或域名
	posHead = getAddrFromBuff(pData,len,';',4);
	posTail = getAddrFromBuff(pData,len,';',5);

	if(posHead != NULL)
		posHead++;
	if((posTail - posHead) == 6)
	{
		if((posHead != NULL)&&(posTail > posHead)&&(posTail <= posHead+sizeof(p_GBT32960_handle->ftp.domainIP)))
		{
			//get IPV4 addr,the first 2bit is 0, so get the ip from the 3bit.
			pos = posHead;
			pos += 2;
			memset(p_GBT32960_handle->ftp.domainIP, 0, sizeof(p_GBT32960_handle->ftp.domainIP));
			memset(buff, 0, 4);

			snprintf(buff, sizeof(buff), "%d", *pos);
			strcat(p_GBT32960_handle->ftp.domainIP, buff);
			strcat(p_GBT32960_handle->ftp.domainIP, ".");
			
			memset(buff, 0, 4);
			snprintf(buff, sizeof(buff), "%d", *(pos+1));
			strcat(p_GBT32960_handle->ftp.domainIP, buff);
			strcat(p_GBT32960_handle->ftp.domainIP, ".");

			memset(buff, 0, 4);
			snprintf(buff, sizeof(buff), "%d", *(pos+2));
			strcat(p_GBT32960_handle->ftp.domainIP, buff);
			strcat(p_GBT32960_handle->ftp.domainIP, ".");

			memset(buff, 0, 4);
			snprintf(buff, sizeof(buff), "%d", *(pos+3));
			strcat(p_GBT32960_handle->ftp.domainIP, buff);
			
			GBTLOG("UpgradeInfo:IPV4 :%s\n", p_GBT32960_handle->ftp.domainIP);
		}
	}

	/*端口*/
	posHead = getAddrFromBuff(pData, len, ';', 5);
	posTail = getAddrFromBuff(pData, len, ';', 6);
	
	if (posHead != NULL)
	{
		posHead++;
	}
	if ((posTail - posHead) == 2)
	{
		if ((posHead != NULL) && (posTail > posHead) && (posTail <= posHead + 2))
		{
			pos = posHead;			
			p_GBT32960_handle->ftp.port = (pos[0]<<8) + pos[1];			
			//char buffport[8];
			//memset(buffport, 0, 8);
			//memcpy(buffport, posHead, posTail-posHead);
			//p_GBT32960_handle->ftp.port = strtol(buffport, NULL, 10);
			GBTLOG("UpgradeInfo:port = %d\n", p_GBT32960_handle->ftp.port);
		}
	}
	
	/*厂商代码*/
	posHead = getAddrFromBuff(pData, len, ';', 6);
	posTail = getAddrFromBuff(pData, len, ';', 7);
	if (posHead != NULL)
		posHead++;
	if ((posTail - posHead) == 4)
	{
		if ((posHead != NULL) && (posTail > posHead) && (posTail <= posHead + sizeof(p_GBT32960_handle->ftp.manufacturer)))
		{
			pos = posHead;
			memcpy(p_GBT32960_handle->ftp.manufacturer, pos, posTail - posHead);
	
			GBTLOG("UpgradeInfo:Manufacturer : ");
			for(i = 0; i < (posTail-posHead); i++)
				GBT_NO("%02x ", p_GBT32960_handle->ftp.manufacturer[i]);
			GBT_NO("\n");
			GBT_NO("manufacturer : %s\n", p_GBT32960_handle->ftp.manufacturer);
		}
	}
	
	/*硬件版本*/
	posHead = getAddrFromBuff(pData, len, ';', 7);
	posTail = getAddrFromBuff(pData, len, ';', 8);
	if (posHead != NULL)
		posHead++;
	if ((posTail - posHead) == 5)
	{
		if ((posHead != NULL) && (posTail > posHead) && (posTail <= posHead + sizeof(p_GBT32960_handle->ftp.hardwareVersion)))
		{
			pos = posHead;
			memcpy(p_GBT32960_handle->ftp.hardwareVersion, pos, posTail - posHead);
	
			GBTLOG("UpgradeInfo:HardwareVersion : ");
			
			for(i = 0; i < (posTail-posHead); i++)
				GBT_NO("%02x ", p_GBT32960_handle->ftp.hardwareVersion[i]);
			GBT_NO("\n");
			GBT_NO("%s\n", p_GBT32960_handle->ftp.hardwareVersion);
		}
	}
	
	/*固件版本*/
	posHead = getAddrFromBuff(pData, len, ';', 8);
	posTail = getAddrFromBuff(pData, len, ';', 9);
	if (posHead != NULL)
		posHead++;
	if ((posTail - posHead) == 5)
	{
		if ((posHead != NULL) && (posTail > posHead) && (posTail <= posHead + sizeof(p_GBT32960_handle->ftp.firmwareVersion)))
		{
			pos = posHead;
			memcpy(p_GBT32960_handle->ftp.firmwareVersion, pos, posTail - posHead);

			GBTLOG("UpgradeInfo:firmwareVersion : ");
			for(i = 0; i < (posTail-posHead); i++)
				GBT_NO("%02x ", p_GBT32960_handle->ftp.firmwareVersion[i]);
			GBT_NO("\n");
			GBT_NO("%s\n", p_GBT32960_handle->ftp.firmwareVersion);
		}
	}
	
	/*升级时限*/
	posHead = getAddrFromBuff(pData, len, ';', 9);
	if (posHead != NULL)
	{
		pos = posHead + 1;
		p_GBT32960_handle->ftp.connectTimeOut = (pos[0] << 8) + pos[1];
		//memset(buff, 0, 4);
		//memcpy(buff, pos, 2);
		//p_GBT32960_handle->ftp.connectTimeOut = strtol(buff, NULL, 10);
		GBTLOG("UpgradeInfo:ConnectTimeOut = %d \r\n", p_GBT32960_handle->ftp.connectTimeOut);
	}
}

/*****************************************************************************
* Function Name : getAddrFromBuff
* Description   : 获得地址信息
* Input			: uint8_t *pData
*                 int32_t len
*                 uint8_t content  
*                 int32_t times
* Output        : None
* Return        : uint8_t * 数据起始地址
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint8_t *GBT32960::getAddrFromBuff(uint8_t *pData, int32_t len, uint8_t content, int32_t times)
{
	uint8_t i;
	uint8_t *pos = pData;
	uint8_t count = 0;
		
	for(i = 0; i<len; i++)
	{
		if(*pos == content)
		{
			count++;
			if(count == times)
			{
				break;
				//return pos;
			}
		}
		pos++;
	}

	return pos;
}

/*****************************************************************************
* Function Name : alarmReport
* Description   : 报警信息上报
* Input			: None
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int GBT32960::alarmReport()
{
	int len;
	unsigned int id;
	unsigned int maxId;
	unsigned int tempValue = 1;
	unsigned int tempId;
	int deleteId = 0;
	int Quotient = 0;
	

	uint8_t *pBuff = (uint8_t *)malloc(DATA_BUFF_LEN);
	if(pBuff == NULL)
	{
		GBTLOG("malloc dataBuff error!");
		return -1;
	}
	
	pthread_mutex_lock(&Goldenlock);
	id = pSqliteDatabase->queryMaxTimesID();
	tempId = id - 30;
	printf("id:%d\n", id);
	if(0 == id)
	{
		pthread_mutex_unlock(&Goldenlock);
		return -1;
	}
	
	if(id < 30)
	{
		if(pSqliteDatabase->querydev(id, Quotient))
		{
			tempId = SAVE_PERIOD - (30 - id);
		}
		else
		{
			tempId = 1;
		}
	}
	pthread_mutex_unlock(&Goldenlock);
	
//	recvFlag = 1;
	while(1)
	{
		pthread_mutex_lock(&Goldenlock);
		memset(pBuff, 0, DATA_BUFF_LEN);
		if(!pSqliteDatabase->readStandardProData(pBuff, &len, tempId))
		{
			mcuUart::m_mcuUart->unpack_updatePositionInfo(pBuff, len);
			mcuUart::m_mcuUart->unpack_updateTimingInfo(pBuff + 30, len);
			pSqliteDatabase->updateisSend(1,tempId);
			pthread_mutex_unlock(&Goldenlock);
			timingReport(GBT32960_ReissueInfoReportID);
		}
		else
		{
			pthread_mutex_unlock(&Goldenlock);
		}
		
		if(id < 30)
		{		
			pSqliteDatabase->querydev(id, Quotient);		
			if(Quotient > 0)
			{				
				if(SAVE_PERIOD == tempId)
				{
					tempId = 0;
				}	
			}				
		}
		tempId++;
		if(tempId > SAVE_PERIOD)	
		{
			tempId = tempId % SAVE_PERIOD;
		}		
		if(id == tempId)
		{
			break;
		}
		
		usleep(100);
	}
	
	if(id > (SAVE_PERIOD - 30) && id <= SAVE_PERIOD)
	{
		tempValue = 30 - (SAVE_PERIOD - id);
	}
	
	tempId = id;
	tempId++;
	
	while(1)
	{
		pthread_mutex_lock(&Goldenlock);
		memset(pBuff, 0, DATA_BUFF_LEN);
		if(!pSqliteDatabase->readStandardProData(pBuff, &len, tempId))
		{
			mcuUart::m_mcuUart->unpack_updatePositionInfo(pBuff, len);
			mcuUart::m_mcuUart->unpack_updateTimingInfo(pBuff + 30, len);
			pSqliteDatabase->updateisSend(1, tempId);
			pthread_mutex_unlock(&Goldenlock);
			timingReport(GBT32960_RealTimeInfoReportID);
		}
		else
		{
			pthread_mutex_unlock(&Goldenlock);
		}
		
		if(SAVE_PERIOD == tempId)
		{
			tempId = 0;
		}
		
		tempId++;
		
		if(id > (SAVE_PERIOD - 30) && id <= SAVE_PERIOD)
		{
			if(tempId == tempValue)
			{
				break;
			}
		}
		else if(tempId == (id + 30))
		{
			break;
		}
		usleep(100);
	}

	if(pBuff != NULL)
	{
		free(pBuff);
		pBuff = NULL;
	}

	return 0;
}

/*****************************************************************************
* Function Name : sendTimingReportData
* Description   : 发送定时上报数据
* Input			: uint8_t *dataBuff  
*                 uint16_t len
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int GBT32960::sendTimingReportData(uint8_t *dataBuff,  uint16_t len)
{
	int length;

	if((length = send(socketfd, dataBuff, len, 0)) < 0)
	{
		GBTLOG("Send data failed.\n");
		close(socketfd);
		isConnected = false;
		resend = false;
		return -1;
	}
	GBTLOG("Send data ok.length:%d\n",length);
	recvFlag = 0;
	
	return 0;
}


/**************************************************************
 * @description LOG
 * @author fpp
 * @date 2019/12/17
 ***************************************************************/


static int gbtLog(char *fplog)
{

#if 1
    time_t t;
    struct tm * lt;
    time (&t);
    lt = localtime (&t);//转为时间结构。
//    char cTime[64];

//    sprintf (cTime,"%d/%d/%d %d:%d:%d ",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);//输出结果

    #define LOG_PATH	"/data/fplog"
	char buff[1024] = "";
	sprintf(buff, "%d/%d/%d %d:%d:%d == %s ==\n",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec, fplog);
	FILE *flogfd;
	flogfd = fopen(LOG_PATH, "a+");
	if(flogfd == NULL)
	{
		printf("filelog open failed\n");
		return -1;
	}

	fwrite(buff, 1, strlen(buff), flogfd);

	fclose(flogfd);
#endif
    return 0;
}