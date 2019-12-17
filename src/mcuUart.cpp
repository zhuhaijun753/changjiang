#include "mcuUart.h"
#include "AdcVoltageCheck.h"
#include "FactoryPattern_Communication.h"

#define SAVE_PERIOD 604800
pthread_mutex_t GoReissueLock = PTHREAD_MUTEX_INITIALIZER;
mcuUart * mcuUart::m_mcuUart =NULL;
McuStatus_t McuStatus;
extern GBT32960 *p_GBT32960;
extern uint8_t isUsbConnectionStatus;

extern TBox_Config_ST ConfigShow;

extern bool testecall;
extern char Detecting_Ecall[20];
extern int faultLevel;
pthread_mutex_t Goldenlock;
extern int closeTbox;
extern bool resend;
extern bool reissue;
extern bool testnet;
extern bool testout;


bool GPStime = false;
/*****************************************************************************
* Function Name : mcuUart
* Description   : 构造函数
* Input			: None
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
mcuUart::mcuUart()
{
	fd = -1;
	m_id = 0;
	memset(&SigEvent, 0, sizeof(SIG_Event_t));
	m_mcuUart = this;
	McuReportTimeOut = 1;
	flagGetTboxCfgInfo = 0;
	RootkeyserialNumber = 0;
	int nread;
	storecount = 0;
	unsigned int len;
	unsigned char *p_mcuBuffer = (unsigned char*)malloc(BUFF_LEN);
	if(p_mcuBuffer == NULL)
	{
		MCULOG("malloc p_mcuBuffer failed.\n");
	}
	signal(SIGALRM, Sig_CtrlAlarm);
	memset(&SigEvent, 0, sizeof(SIG_Event_t));
    memset(&FaultSigan, 0, sizeof(Fault_Sigan_t));
	m_Remainder  = 0;
	m_Id = 0;
	m_AlarmCount = 0;
	mcuUartInit();

	//if(pthread_create(&CheckEventThreadId, NULL, CheckEventThread, this) != 0)
	//	MCULOG("Cannot creat CheckEventThread:%s\n", strerror(errno));
	
	if(pthread_create(&CheckPWMThreadId, NULL, CheckPWMThread, this) != 0)
		MCULOG("Cannot creat CheckPWMThread:%s\n", strerror(errno));

	uint8_t vinvin[20];
    memset(vinvin,0,20);
    memcpy(vinvin,"LDPZYB6D9JF255129",17);
	//packProtocolData(0x82, 0x53, vinvin, 1, 0); 

	
	while(1)
	{
		if(flagGetTboxCfgInfo == 0)
		{
			uint8_t data = 0x01;
			packProtocolData(TBOX_REQ_CFGINFO, 0, &data, 1, 0);
		}
		
		//mcuUartReceiveData();
#if 1
	    memset(p_mcuBuffer, 0, BUFF_LEN);

	    len = checkMcuUartData(p_mcuBuffer, BUFF_LEN);
	    if (len > 0)
	    {
	        nread = unpackMcuUartData(p_mcuBuffer, len);
	        if (nread == -1)
	        {
	            MCULOG("unpack data failed!");
	        }
	    }
		usleep(1000);
#endif
	}
	if (p_mcuBuffer != NULL){
		free(p_mcuBuffer);
		p_mcuBuffer = NULL;
	}
}

/*****************************************************************************
* Function Name : close_uart
* Description   : 关闭打开的串口用于升级完重启系统
* Input			: None
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void mcuUart::close_uart()
{
	close(fd);
}

void *mcuUart::CheckPWMThread(void *arg)
{
	pthread_detach(pthread_self()); 
	mcuUart *pmcuUart = (mcuUart*)arg;

	int nVoltage_SRS 	 = 0;//检测SRS电压值
	int nPeriodCheckPWM  = 0;//检测高电平的周期
	int iLoop = 0;

	int HighVol = 0;
	int LowVol = 0;
	int recordHighVolCount = 0;
	int recordLowVolCount = 0;
	int undetectedVolCount = 0;
	uint8_t EmergedCall[20] = {0};
	
	usleep(200000);
	
	while(1)
	{
		usleep(40000);
		//检测SRS电压
		if(adc_voltage_check(&nVoltage_SRS) == 0)
		{
			if(nVoltage_SRS < 1800 && nVoltage_SRS > 1200)//高电平
			{
				HighVol++;
				recordHighVolCount = HighVol;
				LowVol = 0;
			}
			else if(nVoltage_SRS < 400)
			{
				LowVol++;
				recordLowVolCount = LowVol;
				HighVol = 0;
			}
		
			if((recordHighVolCount == 5) && (recordLowVolCount == 1))
			{
				HighVol = 0;
				LowVol = 0;
			}
		
			if((recordHighVolCount == 1) && (recordLowVolCount == 5))
			{
				nPeriodCheckPWM++;
		
				if(nPeriodCheckPWM >= 20)
				{
					//printf("^^^^^^^Deployment Cammand SRS ECall^^^^^^^\n");
					if(CFAWACP::cfawacp->m_loginState == 2)
					{
						pmcuUart->reportEventCmd(CFAWACP::cfawacp->timingReportingData, 1, ACPApp_EmergencyDataID);
					}
					dataPool->getPara(E_CALL_ID, (void *)EmergedCall, sizeof(EmergedCall));
					memcpy(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].EmergedCall,EmergedCall,15);
										
					for(iLoop = 0; iLoop < 3; iLoop++)
		 			{
						 if(testecall)
						 {
							if(0 == voiceCall(Detecting_Ecall, PHONE_TYPE_E))
								break;
						 }
						 else
                         {
						    if(0 == voiceCall(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].EmergedCall, PHONE_TYPE_E))
						        break;
		 		 	     }						
		 		 	}
		 		 	nPeriodCheckPWM = 0;
	 		 	}
			}			
			undetectedVolCount = 0;
		}
		else
		{
			HighVol = 0;
			LowVol = 0;
			recordHighVolCount = 0;
	        recordLowVolCount = 0;
	        undetectedVolCount++;
	        
			if(undetectedVolCount >= 750)
			{
				p_FAWACPInfo_Handle->voltageFaultSRSState = 1;//SRS硬线断开(检测电压值)
				undetectedVolCount = 0;
			}			
		}
	}

	pthread_exit(0);
}


//TSP下发命令->TBOX解析发送到MCU(上锁)->MCU下发至8090->8090执行动作反馈结果至MCU->MCU将数据设置TBOX结构(解锁)->TBOX反馈结果TSP
int mcuUart::reportEventCmd(callback_EventCmd cb_EventCmd, uint8_t MsgType, AcpAppID_E AppID)
{
	//回调函数控制MCU发送远程控制指定
	return cb_EventCmd(MsgType, AppID);
}

/*****************************************************************************
* Function Name : mcuUartInit
* Description   : mcu串口初始化
* Input			: None
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::mcuUartInit()
{
    unsigned int ui32count = 0;

    while (ui32count++ < 10)
    {
        if ((fd = open(MCU_UART_DEVICE, O_RDWR | O_NOCTTY)) == -1)
        {
			usleep(200);
            MCULOG("Can't Open Serial Port!,ui32count = %d\n", ui32count);
			if(ui32count == 9)
				exit(-1);
        }
        else
        {
            MCULOG("fd:%d, pid:%d\n", fd, getpid());
            break;
        }
    }

    ui32count = 0;
    while (ui32count++ < 10)
    {
        if (0 > setUartSpeed(fd, MCU_UART_SPEED))
        {
            MCULOG("set mcu uart speed failed!ui32count = %d\n", ui32count);
			if(ui32count == 9)
				exit(-1);
            sleep(1);
        }
        else
        {
            MCULOG("set uart speed sucessed!");
            break;
        }
    }

    ui32count = 0;
    while (ui32count++ < 10)
    {
        if (setUartParity(fd, MCU_UART_DATA_BITS, MCU_UART_STOP_BITS, MCU_UART_CHECK_BIT) == -1)
        {
            MCULOG("set mcu uart speed failed!,ui32count=%d", ui32count);
			if(ui32count == 9)
				exit(-1);
            sleep(1);
        }
        else
        {
            MCULOG("set uart parity sucessed!");
            break;
        }
    }

    return 0;
}

/*****************************************************************************
* Function Name : setUartSpeed
* Description   : 设置串口波特率
* Input			: int fd,
*                 int speed
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::setUartSpeed(int fd, int speed)
{
	int speed_arr[] = { B921600, B460800, B230400, B115200, B38400, B19200,
						B9600,	 B4800,   B2400,   B1200,	B300,	B38400,
						B19200,  B9600,   B4800,   B2400,	B1200,	B300};
	
	int name_arr[] = { 921600, 460800, 230400, 115200, 38400, 19200,
					   9600,   4800,   2400,   1200,   300,   38400,
					   19200,  9600,   4800,   2400,   1200,  300};
    int status;
    struct termios Opt;
    tcgetattr(fd, &Opt);
	
    for (unsigned int i = 0; i < sizeof(speed_arr) / sizeof(int); i++)
    {
        if (speed == name_arr[i])
        {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&Opt, speed_arr[i]);
            cfsetospeed(&Opt, speed_arr[i]);
            if ((status = tcsetattr(fd, TCSANOW, &Opt)) != 0)
            {
                perror("tcsetattr fd!");
                return -1;
            }
            tcflush(fd, TCIOFLUSH);
        }
    }
	MCULOG("SET SPEED OK!!!!!\n");
	
    return 0;
}

/*****************************************************************************
* Function Name : setUartParity
* Description   : 设置串奇偶性、数据位和停止位
* Input			: int fd,
*                 int databits,
*                 int stopbits,
*                 int parity
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::setUartParity(int fd, int databits, int stopbits, int parity)
{
    struct termios options;
    if (tcgetattr(fd, &options) != 0)
    {
        perror("Setup Serial!");
        return -1;
    } 
	options.c_cflag &= ~CSIZE;

    switch (databits)
    {
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            fprintf(stderr, "Unsupported data size.\n");
            return -1;
    }

    switch (parity)
    {
        case 'n':
        case 'N':
            options.c_cflag &= ~PARENB; /* Clear parity enable */
            options.c_iflag &= ~INPCK;  /* Enable parity checking */
            break;
        case 'o':
        case 'O':
            options.c_cflag |= (PARODD | PARENB);/* set to odd parity check */
            options.c_iflag |= INPCK;            /* Disnable parity checking */
            break;
        case 'e':
        case 'E':
            options.c_cflag |= PARENB;  /* Enable parity */
            options.c_cflag &= ~PARODD; /* convert to event parity check */
            options.c_iflag |= INPCK;   /* Disnable parity checking */
            break;
        case 's':
        case 'S':
            /* as no parity */
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break;
        default:
            fprintf(stderr, "Unsupported parity.\n");
            return -1;
    }

    switch (stopbits)
    {
        case 1:
            options.c_cflag &= ~CSTOPB;
            break;
        case 2:
            options.c_cflag |= CSTOPB;
            break;
        default:
            fprintf(stderr, "Unsupported stop bits.\n");
            return -1;
    }

    /* Set input parity option */
    if (parity != 'n')
        options.c_iflag |= INPCK;
    tcflush(fd, TCIFLUSH);
    options.c_cc[VTIME] = MCU_UART_TIMEOUT_MSECONDS; /* set timeout for 50 mseconds */
    options.c_cc[VMIN] = 0;
    options.c_cflag |= (CLOCAL | CREAD);

    /* Select the line input */
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    /* Select the line output */
    options.c_oflag &= ~OPOST;
    /* prevant 0d mandatory to 0a */
    options.c_iflag &= ~ICRNL;

    /* Cancellation of software flow control
     * options.c_iflag &=~(IXON | IXOFF | IXANY);
     */
    options.c_iflag &= ~IXON;

    if (tcsetattr(fd, TCSANOW, &options) != 0)
    {
        perror("Activate Serial failed.\n");
        return -1;
    }
	
    return 0;
}

/*****************************************************************************
* Function Name : mcuUartReceiveData
* Description   : 设置串奇偶性、数据位和停止位
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::mcuUartReceiveData(void)
{
    int nread;
    unsigned int len;

    unsigned char *p_mcuBuffer = (unsigned char*)malloc(BUFF_LEN);
	if(p_mcuBuffer == NULL)
	{
		MCULOG("malloc p_mcuBuffer failed.\n");
		return -1;
	}
    memset(p_mcuBuffer, 0, BUFF_LEN);

  //  MCULOG("start to read mcu data\n");

    len = checkMcuUartData(p_mcuBuffer, BUFF_LEN);
    if (len > 0)
    {
        nread = unpackMcuUartData(p_mcuBuffer, len);
        if (nread == -1)
        {
            MCULOG("unpack data failed!");
        }
    }
	if (p_mcuBuffer != NULL){
		free(p_mcuBuffer);
		p_mcuBuffer = NULL;
	}

	return 0;
}

/*
int mcuUart::registerCallback_reportDate(callBack_reportDate func)
{
	if (func == NULL)
		return -1;
	
	reportDataFunc = func;

	return 0;
}*/

/*****************************************************************************
* Function Name : checkMcuUartData
* Description   : 数据检查
* Input			: unsigned char *pData
*                 unsigned int size
* Output        : None
* Return        : retval:数据长度, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint32_t mcuUart::checkMcuUartData(unsigned char *pData, unsigned int size)
{
	bool startToRead = false;
	unsigned char tempData = 0;
	unsigned int retval;
	unsigned char *pos = pData;
	unsigned char runState = 0;
	bool escapeCode = false;
	while (!startToRead)
	{
		if ((uint32_t)(pos-pData) >= size)
			break;
		if (read(fd, &tempData, 1) > 0)
		{
			//printf("%02x ",tempData);
			if (runState == 0)
			{
				if (tempData == 0x7e)
				{
					pos = pData;
					*pos = tempData;
					runState = 1;
				}
			}
			else if (runState == 1)
			{
				if (tempData == 0x7e)
				{
					if (pos == pData)
					{
						pos = pData;
						*pos = tempData;
						runState = 1;
					}
					else
					{
						*++pos = tempData;
						startToRead = true;
						break;
					}
				}
				else
				{
					if (escapeCode == true)
					{
						if (tempData == 0x01)
							*pos = 0x7D;
						else if (tempData == 0x02)
							*pos = 0x7E;
						else
							runState = 0;
						escapeCode = false;
					}
					else
					{
						if (tempData == 0x7d)
							escapeCode = true;
						*++pos = tempData;
					}
				}
			}
			else
			{
				runState = 0;
			}
		}
	}

	if ((startToRead == true) && (pos > pData + 1))
		retval = pos - pData + 1;
	else
		retval = 0;
	
	//MCULOG(" @@@@@@@@@@@@@@@@@@@@@@@@@ MCU received data! retval=%d", retval);
	//for(int i=0; i<(int)retval; i++)
	//	MCU_NO("%02x ",*(pData+i));
	//MCU_NO("\n\n");
	
	return retval;
}

/*****************************************************************************
* Function Name : unpackMcuUartData
* Description   : 解包mcu数据
* Input			: uint8_t *pData
*                 unsigned int datalen
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::unpackMcuUartData(uint8_t *pData, unsigned int datalen)
{
	//static int count = 0;
    uint16_t checkCrc;
	uint16_t serialNumber = 0;
    uint16_t bodylen = 0;
    uint8_t cmdId;
	int ret = -1;

    if ((pData == NULL) || (datalen <= 4))
        return -1;

    bodylen = (pData[1] << 8) + pData[2];
    if ((uint32_t)(bodylen+11)!= datalen)
        return -1;

    checkCrc = (pData[1+7+bodylen] << 8) + pData[1+7+bodylen+1];
    if (checkCrc != Crc16Check(&pData[1], datalen-4))
    {
        MCULOG("check crc16 failed!\n");
        return -1;
    }
	else
	{
//		MCULOG("check crc16 success!\n");
	}

    cmdId = pData[3];
	MCULOG("cmdId:%02x \n",cmdId);
	for(int i=0;i<datalen;i++)
	{
		MCULOG("data %d is %02x\n",i,pData[i]);
	}
    switch (cmdId)
    {
		case START_SYNC_CMD:	//0x00
			unpack_syncParameter(pData, datalen);
			break;
		case HEART_BEAT_CMD:	//0x10
		{
#if 0
			//for(int i = 0; i< datalen; i++)
			//	printf("%02x ", pData[i]);
			//printf("\n\n\n");
			pthread_mutex_lock(&GoReissueLock);
			#define SAVE_PERIOD	1296000
			int isSend = 0;
			int m_Remainder = 0;
			unsigned int Quotient = 0;
			if(0 == m_id)
			{
				pSqliteDatabase->queryMaxDevandID(m_Remainder, Quotient);
				m_id = Quotient * SAVE_PERIOD + m_Remainder;
				printf("m_id ======= %d\n", m_id);
			}
			m_id++;
			if(CFAWACP::cfawacp->m_loginState == 2)
				isSend = 0;
			else
				isSend = 1;

			if(SAVE_PERIOD <= m_id)
			{
				Quotient = m_id / SAVE_PERIOD;
				m_Remainder = m_id % SAVE_PERIOD;
				pSqliteDatabase->deleteData(m_Remainder);
			}
			else
				m_Remainder = m_id;

			pSqliteDatabase->insertData(m_Remainder, pData, datalen, Quotient, isSend);
#endif
			serialNumber = (pData[4] << 8) + pData[5];
			packDataWithRespone(TBOX_REPORT_4G_STATE, 0x01, NULL, 0, serialNumber);
			
//reportDataFunc(pData, datalen);
			unpack_updatePositionInfo(pData, datalen);
			
			//32960
			//datalen -= 30;
			//unpack_updateTimingInfo(pData + 30, datalen);
		}
			break;
		case L32960data:
		{
			pthread_mutex_lock(&Goldenlock);
			serialNumber = (pData[4] << 8) + pData[5];
			//packDataWithRespone(TBOX_REPORT_4G_STATE, 0x01, NULL, 0, serialNumber);
			if(closeTbox==1)
			{
				packDataWithRespone(close_Tbox, 0x01, NULL, 0, serialNumber);
			}
			else if(closeTbox==2)
			{
				packDataWithRespone(reset_Tbox, 0x01, NULL, 0, serialNumber);
			}
			/*else if(closeTbox==4)
			{
				packDataWithRespone(alarm1, 0x01, NULL, 0, serialNumber);
			}
			else if(closeTbox==5)
			{
				packDataWithRespone(alarm2, 0x01, NULL, 0, serialNumber);
			}
			else if(closeTbox==6)
			{
				packDataWithRespone(alarm3, 0x01, NULL, 0, serialNumber);
			}*/
			else
			{
				packDataWithRespone(TBOX_REPORT_4G_STATE, 0x01, NULL, 0, serialNumber);
			}
			//reportDataFunc(pData, datalen);
			
//			Remainder = 0;
			unsigned int Quotient = 0;
			if(p_GBT32960_handle->configInfo.localStorePeriod<1000)
				p_GBT32960_handle->configInfo.localStorePeriod = 30000;
			//if(++storecount >= (p_GBT32960_handle->configInfo.localStorePeriod/1000))
			if(++storecount >= 1)
			{
				storecount =0;
				if(resend==false)
				{
					if(0 == m_Id)
					{
						pSqliteDatabase->queryStandardProMaxID(m_Remainder, Quotient);
						m_Id = Quotient * SAVE_PERIOD + m_Remainder;
						m_AlarmCount = pSqliteDatabase->queryMaxTimes();
					}
				
					m_Id++;
				
					if(m_Id > SAVE_PERIOD)
					{
						Quotient = 0;
						m_Remainder = 0;
						Quotient = m_Id / SAVE_PERIOD;
						m_Remainder = m_Id % SAVE_PERIOD;
					}
				
					if(m_Id > SAVE_PERIOD)
					{
						if(0 == m_Remainder)
						{
							pSqliteDatabase->deleteData(SAVE_PERIOD);
							pSqliteDatabase->insertStandardProData(SAVE_PERIOD, pData, datalen, 0, Quotient - 1, 0);
						}
						else
						{
							if(!pSqliteDatabase->deleteData(m_Remainder))
								pSqliteDatabase->insertStandardProData(m_Remainder, pData, datalen, 0, Quotient, 0);
						}
					}
					else
					{
						m_Remainder = m_Id;
						pSqliteDatabase->insertStandardProData(m_Remainder, pData, datalen, 0, Quotient, 0);
						//pSqliteDatabase->insertStandardProData(m_Remainder, pData, datalen, 0, Quotient, 0);
						//pSqliteDatabase->insertStandardProData(m_Remainder, pData, datalen, 0, Quotient, 0);

					}
				
				}
				else
				{
					if(0 == m_Id)
					{
						pSqliteDatabase->queryStandardProMaxID(m_Remainder, Quotient);
						m_Id = Quotient * SAVE_PERIOD + m_Remainder;
						m_AlarmCount = pSqliteDatabase->queryMaxTimes();
					}
				
					m_Id++;
				
					if(m_Id > SAVE_PERIOD)
					{
						Quotient = 0;
						m_Remainder = 0;
						Quotient = m_Id / SAVE_PERIOD;
						m_Remainder = m_Id % SAVE_PERIOD;
					}
				
					if(m_Id > SAVE_PERIOD)
					{
						if(0 == m_Remainder)
						{
							pSqliteDatabase->deleteData(SAVE_PERIOD);
							pSqliteDatabase->insertStandardProData(SAVE_PERIOD, pData, datalen, 0, Quotient - 1, 1);
						}
						else
						{
							if(!pSqliteDatabase->deleteData(m_Remainder))
								pSqliteDatabase->insertStandardProData(m_Remainder, pData, datalen, 0, Quotient, 1);
						}
					}
					else
					{
						m_Remainder = m_Id;
						pSqliteDatabase->insertStandardProData(m_Remainder, pData, datalen, 0, Quotient, 1);
						//pSqliteDatabase->insertStandardProData(m_Remainder, pData, datalen, 0, Quotient, 1);
						//pSqliteDatabase->insertStandardProData(m_Remainder, pData, datalen, 0, Quotient, 1);
						//pSqliteDatabase->insertStandardProData(m_Remainder, pData, datalen, 0, Quotient, 1);	
					}
				}
			}
			MCULOG("123123123123 storeperiod is %d  id is %d  mremainder is %d\n",p_GBT32960_handle->configInfo.localStorePeriod,m_Id,m_Remainder);
			struct tm *p_tm = NULL;
			time_t tmp_time;
			tmp_time = time(NULL);
			p_tm = gmtime(&tmp_time);
			MCULOG("store  TIME:%0d-%d-%d:%02d:%02d:%02d",p_tm->tm_year+1900, p_tm->tm_mon,p_tm->tm_mday, p_tm->tm_hour,p_tm->tm_min,p_tm->tm_sec);
													
			
			unpack_updatePositionInfo(pData, datalen);
			
			datalen -= 30;
			MCULOG("**********START TO UPDATETIMINGINFO\n");
			unpack_updateTimingInfo(pData + 30, datalen);
			pthread_mutex_unlock(&Goldenlock);

			/*if(p_GBT32960_handle->dataInfo.vehicleInfo.gear == 0x00)
			{
				testnet = true;
			}
			else if(p_GBT32960_handle->dataInfo.vehicleInfo.gear == 0x0d)
			{
				testnet = false;
			}*/
			

			/*struct tm *p_tm = NULL;
			time_t tmp_time;
			tmp_time = time(NULL);
			p_tm = gmtime(&tmp_time);
			
			ofstream haha;
  		 	haha.open("/data/log_time.txt", ios::app); 
  			  haha << "current time is " << p_tm->tm_mon << p_tm->tm_mday << p_tm->tm_hour << p_tm->tm_min << p_tm->tm_sec << "\n"; 
  		 	haha.close(); // 输出完毕后关闭这个文件*/
		}
			break;
		case MCU_SND_MESSAGES_ID:	//0x0B
			unpack_text_messages(pData, datalen);
			break;
		case TEXT_TO_SPEECH_ID:		//0x0C
			unpack_tts_voice(pData, datalen);
			break;
		case MCU_SND_UPGRADE_INFO:	//0x0D
			unpack_MCU_SND_Upgrade_Info(pData, datalen);
			break;
		case MCU_SND_UPGRADE_DATA:	//0x0E
			unpack_MCU_SND_Upgrade_Data(pData, datalen);
			break;
		case MCU_SND_UPGRADE_CMPL:	//0x08
			unpack_MCU_SND_Upgrade_CMPL();
			break;
		case MCU_CHECK_REPORT:		//0x20
			unpack_MCU_Upgrade_Check(pData, datalen);
			break;
		case TBOX_REMOTECTRL_CMD:	//0x04
		{
			serialNumber = (pData[4] << 8) + pData[5];
			packProtocolData(TBOX_GENERAL_RESP, 0x04, NULL, 0, serialNumber);
			unpack_RemoteCtrl(pData, datalen);
		}
			break;
		case 0x06:
			mcu_apply_for_data(pData, datalen);
			break;
		case MCU_SND_TBOXINFO:		//0x12
			if(access("/data/9011.log",F_OK)!=0)
			{
				FILE* fd;
				fd = fopen("/data/9011.log", "w");
				fflush(fd);
  				fclose(fd);
  				fd = NULL;
				system("sync");
				LteAtCtrl->switch9011();
			}
			unpack_TboxConfigInfo(pData, datalen);
			serialNumber = (pData[4] << 8) + pData[5];
			packProtocolData(TBOX_GENERAL_RESP, 0x12, NULL, 0, serialNumber);
			break;
		case MCU_REPORT_DRIVERDATA:	//0x07-MCU驾驶行为数据上报
			unpack_MCU_DriverInfo(pData, datalen);
			break;
		case MCU_STATUS_REPORT:
			unpack_MCU_StatusReport(pData, datalen);
			break;
		case MCU_INFROM_ROOTKEY:
		{
			RootkeyserialNumber = (pData[4] << 8) + pData[5];
			if(CFAWACP::cfawacp->m_loginState == 2){
				if(reportEventCmd(CFAWACP::cfawacp->timingReportingData, 1, ACPApp_UpdateKeyID) != 0)
					packProtocolData(TBOX_ROOTKRY_RESULT, MCU_INFROM_ROOTKEY, NULL, -1, RootkeyserialNumber);
			}else{
				packProtocolData(TBOX_ROOTKRY_RESULT, MCU_INFROM_ROOTKEY, NULL, -1, RootkeyserialNumber);
			}
		}
			break;
		case MCU_SND_CONTROL:
		{
			switch (pData[9])
			{
				case 0x01:
					/* code */
					printf("send 01010101 \n\n");
					//p_GBT32960->sendLogoutData();
					testout = false;
					
					break;
				case 0x02:
					printf("send 02020202 \n\n");
					/* code */
					testnet = false;
					break;
				case 0x03:
					printf("send 03030303 \n\n");
					/* code */
					testnet = true;
					break;
			
				default:
					break;
			}
			break;
		}
		default:
            MCULOG("cmdid error!\n");
            break;
    }
	
    return 0;
}

/*****************************************************************************
* Function Name : Crc16Check
* Description   : 数据检验
* Input			: uint8_t *pData
*                 uint32_t len
* Output        : None
* Return        : ui16Crc:校验码
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
unsigned int mcuUart::Crc16Check(unsigned char* pData, uint32_t len)
{
	unsigned int ui16InitCrc = 0xffff;
	unsigned int ui16Crc = 0;
	unsigned int ui16i;
	unsigned char ui8j;
	unsigned char ui8ShiftBit;
	
	for(ui16i = 0;ui16i<len;ui16i++)
	{		
		ui16InitCrc ^= pData[ui16i];
		for(ui8j=0;ui8j<8;ui8j++)
		{
			ui8ShiftBit = ui16InitCrc&0x01;
			ui16InitCrc >>= 1;
			if(ui8ShiftBit != 0)
			{
				ui16InitCrc ^= 0xa001;
			}		
		}
	}
	
	ui16Crc = ui16InitCrc;
	return ui16Crc;
}

/*****************************************************************************
* Function Name : unpack_syncParameter
* Description   : 解包同步参数
* Input			: unsigned char *pData
*                 unsigned int len
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::unpack_syncParameter(unsigned char *pData, unsigned int len)
{
	int i;
	unsigned short serialNumber;
	unsigned char attr[2];
	unsigned char subDataLen;
	unsigned char *pos = NULL;
	unsigned int tempData = 0;
	unsigned char u8Array[32];
	memset(u8Array,0,sizeof(u8Array));

	serialNumber = (pData[4] << 8) + pData[5];
	MCULOG("serialNumber = %d\n",serialNumber);

	memset(attr, 0, sizeof(attr));
	attr[0] = pData[6];
	attr[1] = pData[7];

	pos = pData;

	if(*(pos+8) == 0x01)
		//setSystemTime(pData);
	subDataLen = *(pos+8+1);
	pos = pos+8+1+subDataLen+1;

	if(*pos == 0x02)
		;
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	if(*pos == 0x03)
	{
		tempData = (*(pos+2)<<24) + (*(pos+3)<<16) + (*(pos+4)<<8) + (*(pos+5)<<0);
		tempData = tempData/3600;
		MCULOG("The lock car time show the terminal didn't connect to the server:%d\n",tempData);
		//dataPool->setPara(TBOX_DATA_NO_GPRS_SIGNAL_HOURS_INFO, &tempData, sizeof(tempData));	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x04)
	{
		;
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;
	
	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x05)
	{
		;
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;
	
	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x06)
	{
		;
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;
	
	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x07)
	{
		;
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;
	
	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x08)
	{
		tempData = (*(pos+2)<<24) + (*(pos+3)<<16) + (*(pos+4)<<8) + (*(pos+5)<<0);
		MCULOG("Can baud Rate:%d\n",tempData);
		//dataPool->setPara(TBOX_CANBAUDRATE_PARAID, &tempData, sizeof(tempData));
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;
	
	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x09)
	{
		;	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;
	
	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x0A)
	{
		;	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x0B)
	{
		;	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x0C)
	{
		tempData = *(pos+2);
		//dataPool->setPara(TBOX_REMOTE_UPGRADE_PARAID, &tempData, sizeof(tempData));
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;
	
	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x0D)
	{
		tempData = *(pos+2);
		//dataPool->setPara(TBOX_POWER_SAVING_MODE_PARAID, &tempData, sizeof(tempData));
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x0E)
	{
		;	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;
	
	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x0F)
	{
		;	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x10)
	{
		;	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;
	
	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x11)
	{
		;	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x12 || *pos == 0x13)
	{
		tempData = (*(pos+2)<<8) + *(pos+3);
		//dataPool->setPara(TBOX_MAINPWR_LOW_THRES_PARAID, &tempData, sizeof(tempData));
		MCU_NO("tempDatas = %02x\n",tempData);
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;
	
	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x14)
	{
		;	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x15)
	{
		tempData = (*(pos+2)<<24) + (*(pos+3)<<16) + (*(pos+4)<<8) + (*(pos+5)<<0);
		//dataPool->setPara(TBOX_SLEEP_REPORTPOSI_INTERVAL_PARAID, &tempData, sizeof(tempData));
		MCU_NO("back power interval = %d\n",tempData);
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x16)
	{
		tempData = *(pos+2);
		//dataPool->setPara(TBOX_PLC_PWRUP_CHECKLEVEL_PARAID, &tempData, sizeof(tempData));	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x17)
	{
		;	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x18)
	{
		tempData = (*(pos+2)<<24) + (*(pos+3)<<16) + (*(pos+4)<<8) + (*(pos+5)<<0);
		//dataPool->setPara(TBOX_BACKPWRUP_REPORTPOSI_INTERVAL_PARAID, &tempData, sizeof(tempData));
		MCU_NO("back power interval = %d\n",tempData);
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x19)
	{
		;	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x1A)
	{
		subDataLen = *(pos+1);
		memset(mcuVerNumber, 0, sizeof(mcuVerNumber));
		//MCULOG("Set mcu version & ver length:%d", subDataLen);
		//for (i = 0; i < subDataLen; i++)
		//	MCU_NO("%c",*(pos+1+1+i));
			//MCU_NO("%02x ",*(pos+1+1+i));
		//MCU_NO("\n\n");
		/*********************************************************************
		 *  mcu version example:V1.0.0.0_Build2017041714:00:00,
		 *  It need to get the version number form the mcu version's 2bit,
		 *  4bit,6bit and 8bit,then convert to number and stored it in u8Array
		 *  array.
		 *********************************************************************/
		mcuVerNumber[0] = *(pos+1+1+1)-'0';
		mcuVerNumber[1] = *(pos+1+1+3)-'0';
		mcuVerNumber[2] = *(pos+1+1+5)-'0';
		//mcuVerNumber[3] = *(pos+1+1+7)-'0';
		MCU_NO("%02x %02x %02x %02x ",mcuVerNumber[0],mcuVerNumber[1],mcuVerNumber[2]); //,u8Array[3]);
		//dataPool->setPara(TBOX_STA8090_VERSION_PARAID, u8Array, 4);
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;
	
	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x1B)
	{
		subDataLen = *(pos+1);
		memset(u8Array, 0, sizeof(u8Array));
		MCULOG("Set plc version & ver length:%d", subDataLen);
		for (i = 0; i < subDataLen; i++)
			MCU_NO("%02x ",*(pos+1+1+i));
		MCU_NO("\n\n");
		memcpy(u8Array, pos+1+1, 20);
		//dataPool->setPara(TBOX_PLC_VERSION_PARAID, u8Array, subDataLen);
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x1C)
	{
		;	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x1D)
	{
		;	
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x1E)
	{
		tempData = (*(pos+2)<<24) + (*(pos+3)<<16) + (*(pos+4)<<8) + (*(pos+5)<<0);
		MCULOG("Can bus minimum filtering value:%d\n",tempData);
		//dataPool->setPara(TBOX_CAN_MINID_PARAID, &tempData, sizeof(tempData));
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x1F)
	{
		tempData = (*(pos+2)<<24) + (*(pos+3)<<16) + (*(pos+4)<<8) + (*(pos+5)<<0);
		MCULOG("Can bus maximun filtering value:%d\n",tempData);
		//dataPool->setPara(TBOX_CAN_MAXID_PARAID, &tempData, sizeof(tempData));
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;
	
	MCU_NO("*pos = %02x\n",*pos);
	if(*pos == 0x20)
	{
		//dataPool->getInfo(TBOX_DATA_EXT_STATE_INFO,&tempData,sizeof(tempData));
		if(*(pos+2) == 0)
			tempData &= (~(0x01 << 0));
		else if(*(pos+2) == 1)
			tempData &= (0x01 << 0);
		MCULOG("Lock car time, *(pos+2):%02x,temData:%d\n",*(pos+2),tempData);
		//dataPool->setInfo(TBOX_DATA_EXT_STATE_INFO,&tempData,sizeof(tempData));
	}
	subDataLen = *(pos+1);
	pos = pos+1+subDataLen+1;

	packProtocolData(TBOX_GENERAL_RESP, 0x00, NULL, 0, serialNumber);
	
	return 0;
}

//解析MCU发送的TBOX信息	VIN和配置码
int mcuUart::unpack_TboxConfigInfo(unsigned char *pData, unsigned int len)
{
	uint8_t *pTemp = pData;
	uint8_t DataLen = 0;
	
	//获取VIN	
	DataLen = 17;
	dataPool->setTboxConfigInfo(VinID, &pTemp[8], DataLen);

	DataLen = 1;
	dataPool->setTboxConfigInfo(ConfigCodeID, &pTemp[25], DataLen);
#if 0
	DataLen = 15;
	dataPool->setPara(E_CALL_ID, &pTemp[26], DataLen);
	
	DataLen = 15;
	dataPool->setPara(B_CALL_ID, &pTemp[41], DataLen);
#endif
	DataLen = 16;
	dataPool->setTboxConfigInfo(SUPPLYPN_ID, &pTemp[65], DataLen);

	if(dataPool->updateTboxConfig() != 0)
	{
		MCULOG("update tboxconfiginfo error\n");
		return -1;
	}

	flagGetTboxCfgInfo = 1;
	return 0;
}


/*****************************************************************************
* Function Name : unpack_updatePositionInfo
* Description   : 解包位置信息
* Input			: unsigned char *pData
*                 unsigned int len
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::unpack_updatePositionInfo(unsigned char *pData, unsigned int len)
{
	struct tm _tm;
	uint32_t u32Status = (pData[8]<<24) + (pData[9]<<16) + (pData[10]<<8) +pData[11];
	//定位状态 positioning status

	struct sysinfo info;
	char buff[128];
	

	if(u32Status & (0x01 <<18))
	{
		sysinfo(&info);
		printf("GPS ok time is %d\n\n\n\n",info.uptime);
		if(!GPStime)
		{
			
			GPStime = true;
			McuStatus.GPStime = info.uptime;
			printf("GPS ok time is %d\n",McuStatus.GPStime);
			sprintf(buff,"echo %d > /data/111.log",McuStatus.GPStime);
			system(buff);
		}

		
		MCULOG("GPS positioning successful!\n");
		//设置系统时间
		_tm.tm_year = pData[24]+100; /* 年份，其值等于实际年份减去1900 */
		//8090已处理月份为当月，不需+1
		_tm.tm_mon = pData[25] - 1;    /* 月份（从一月开始，0代表一月）-取值区间为[1,12] */
		_tm.tm_mday = pData[26];     /* 一个月中的日期 - 取值区间为[1,31] */
		_tm.tm_hour = pData[27];     /* 时 - 取值区间为[0,23] */
		_tm.tm_min = pData[28];      /* 分 - 取值区间为[0,59] */
		_tm.tm_sec = pData[29];      /* 秒 – 取值区间为[0,59] */
		
		MCULOG("======================================================================================================\n");
		MCULOG("======================================================================================================\n");
		MCULOG("======================================================================================================\n");
		MCULOG("======================================================================================================\n");
		MCULOG("======================================================================================================\n");
		
		MCULOG("TIME:%0d-%d-%d:%02d:%02d:%02d",_tm.tm_year, _tm.tm_mon,_tm.tm_mday, _tm.tm_hour,_tm.tm_min,_tm.tm_sec);
		setSystemTime(&_tm);
	}
	else
	{
		MCULOG("GPS positioning failed!\n");
	}
	
	//p_GBT32960_handle->dataInfo.vehicleInfo.vehicleSpeed = pData[20] + pData[21];
	uint16_t WTempData = 0;
	//unsigned char *pos = pData;
	WTempData = pData[20];
	WTempData = ((WTempData&0xff) << 8) | pData[21];
	if(WTempData > 2200)
	{
		WTempData = 0xfffe;
	}
	p_GBT32960_handle->dataInfo.vehicleInfo.vehicleSpeed = WTempData;

	MCULOG("vehiceSpeed is %02x\n",p_GBT32960_handle->dataInfo.vehicleInfo.vehicleSpeed);
	
	unpack_updateGBT32960PositionInfo(pData, len);
	unpack_updateFAWACPPositionInfo(pData, len);
	unpack_updateFAWACPVehCondInfo(pData, len);

	return 0;
}

/*****************************************************************************
* Function Name : setSystemTime
* Description   : 设置系统时间
* Input			: struct tm *pTm
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::setSystemTime(struct tm *pTm)
{
    time_t timep;
    struct timeval tv;

    timep = mktime(pTm);
    tv.tv_sec = timep;
    tv.tv_usec = 0;
    if (settimeofday(&tv, (struct timezone*)0) < 0)
    {
        printf("Set system datatime error!\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
* Function Name : unpack_updateGBT32960PositionInfo
* Description   : 更新位置信息
* Input			: unsigned char *pData
*                 unsigned int len
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::unpack_updateGBT32960PositionInfo(unsigned char *pData, unsigned int len)
{
	uint16_t i;
	
	//for (i = 0; i < len; i++)
	//	MCU_NO("%02x ",*(pData+i));
	//MCU_NO("\n\n");

	uint32_t u32PositionStatus = (pData[8]<<24) + (pData[9]<<16) + (pData[10]<<8) +pData[11];
//	MCULOG("u32PositionStatus:%u\n",u32PositionStatus);

	//定位状态 positioning status
	if(u32PositionStatus & (0x01 <<18)){
		//p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState &= (0<<0);
		p_GBT32960_handle->dataInfo.positionInfo.positionValid.bitState.valid = 0;
//		MCULOG("111 positionState:%d\n",p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState);
	}
	else{
		//p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState |= (1<<0);
		p_GBT32960_handle->dataInfo.positionInfo.positionValid.bitState.valid = 1;
//		MCULOG("222 positionState:%d\n",p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState);
	}
	
	//南纬或北纬 South latitude or north latitude
	if(u32PositionStatus & (0x01 <<0)){
		//p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState |= (1<<1);
		p_GBT32960_handle->dataInfo.positionInfo.positionValid.bitState.lat = 1;
//		MCULOG("333 positionState:%d\n",p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState);
	}
	else{
		//p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState &= (0<<1);
		p_GBT32960_handle->dataInfo.positionInfo.positionValid.bitState.lat = 0;
//		MCULOG("444 positionState:%d\n",p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState);
	}

	//东经或西经 East longitude or west longitude
	if(u32PositionStatus & (0x01 <<1)){
		//p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState |= (1<<2);
		p_GBT32960_handle->dataInfo.positionInfo.positionValid.bitState.lon = 1;
//		MCULOG("555 positionState:%d\n",p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState);
	}
	else{
		//p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState &= (0<<2);
		p_GBT32960_handle->dataInfo.positionInfo.positionValid.bitState.lon = 0;
//		MCULOG("666 positionState:%d\n",p_GBT32960_handle->dataInfo.positionInfo.positionValid.positionState);
	}

	/****************************************************************
	* 定位状态说明:
	* pData[12]: 纬度
	* pData[13]: 纬度分
	* ((pData[14] << 8) + pData[15]): 纬度分小数
	* pData[16]: 经度
	* pData[17]: 经度分
	* ((pData[18] << 8) + pData[19]): 经度分小数
	*****************************************************************/
	p_GBT32960_handle->dataInfo.positionInfo.Lat = (uint32_t)pData[12]*1000000+
												(((float)pData[13]+((float)((pData[14]<<8)+pData[15])/100000))/60)*1000000;

	p_GBT32960_handle->dataInfo.positionInfo.Lon = (uint32_t)pData[16]*1000000+
												(((float)pData[17]+((float)((pData[18]<<8)+pData[19])/100000))/60)*1000000;

//	MCULOG("Lat:%lu\n",p_GBT32960_handle->dataInfo.positionInfo.Lat);
//	MCULOG("Lon:%lu\n",p_GBT32960_handle->dataInfo.positionInfo.Lon);

	return 0;
}

int mcuUart::unpack_updateFAWACPPositionInfo(unsigned char *pData, unsigned int len)
{
	uint32_t u32PositionStatus = (pData[8]<<24) + (pData[9]<<16) + (pData[10]<<8) +pData[11];
//	MCULOG("u32PositionStatus:%u\n",u32PositionStatus);
	//ACC状态	0:OFF, 1:ON
	AccStatus = (u32PositionStatus >> 4) & 0X00000001;
	//定位状态 positioning status
	if((u32PositionStatus >> 18) & 0x00000001){
		p_FAWACPInfo_Handle->VehicleCondData.GPSData.GPSState = 1;
	//	MCULOG("111 positionState:%d\n",p_FAWACPInfo_Handle->VehicleCondData.GPSData.GPSState);
	}
	else{
		p_FAWACPInfo_Handle->VehicleCondData.GPSData.GPSState = 0;
	//	MCULOG("222 positionState:%d\n",p_FAWACPInfo_Handle->VehicleCondData.GPSData.GPSState);
	}
	//南纬或北纬 South latitude or north latitude
	if((u32PositionStatus >> 0) & 0x00000001){
		p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitudeState = 1;
	//	MCULOG("333 positionState:%d\n",p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitudeState);
	}
	else{
		p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitudeState = 0;
	//	MCULOG("444 positionState:%d\n",p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitudeState);
	}
	//东经或西经 East longitude or west longitude
	if((u32PositionStatus >> 1) & 0x00000001){
		p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitudeState = 1;
	//	MCULOG("555 positionState:%d\n",p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitudeState);
	}
	else{
		p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitudeState = 0;
	//	MCULOG("666 positionState:%d\n",p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitudeState);
	}
	/****************************************************************
	* 定位状态说明:
	* pData[12]: 纬度
	* pData[13]: 纬度分
	* ((pData[14] << 8) + pData[15]): 纬度分小数
	* pData[16]: 经度
	* pData[17]: 经度分
	* ((pData[18] << 8) + pData[19]): 经度分小数
	*****************************************************************/
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitude = (uint32_t)pData[12]*1000000+
												(((float)pData[13]+((float)((pData[14]<<8)+pData[15])/50000))/60)*1000000;
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitude = (uint32_t)pData[16]*1000000+
												(((float)pData[17]+((float)((pData[18]<<8)+pData[19])/50000))/60)*1000000;
	//MCULOG("Lat:%lu\n",p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitude);
	//MCULOG("Lon:%lu\n",p_FAWACPInfo_Handle->VehicleCondData.GPSData.longitude);
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.altitude = (pData[20] << 8) + pData[21];
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.degree = (pData[22] << 8) + pData[23];
	//MCULOG("altitude:%lu\n",p_FAWACPInfo_Handle->VehicleCondData.GPSData.altitude);
	//MCULOG("Degree:%lu\n",p_FAWACPInfo_Handle->VehicleCondData.GPSData.degree);
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.year = pData[24]+10;
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.month = pData[25];
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.day = pData[26];
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.hour = pData[27];
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.minute = pData[28];
	p_FAWACPInfo_Handle->VehicleCondData.GPSData.second = pData[29];
	#if 0
	printf("GPS_latitude == %d\n", p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitude);
	printf("GPS_longitude == %d\n", p_FAWACPInfo_Handle->VehicleCondData.GPSData.latitude);
	printf("GPS_altitude == %d\n", p_FAWACPInfo_Handle->VehicleCondData.GPSData.altitude);
	printf("GPS_degree == %d\n", p_FAWACPInfo_Handle->VehicleCondData.GPSData.degree);
	printf("GPS_year == %d\n", p_FAWACPInfo_Handle->VehicleCondData.GPSData.year);
	printf("GPS_month == %d\n", p_FAWACPInfo_Handle->VehicleCondData.GPSData.month);
	printf("GPS_day == %d\n", p_FAWACPInfo_Handle->VehicleCondData.GPSData.day);
	printf("GPS_hour == %d\n", p_FAWACPInfo_Handle->VehicleCondData.GPSData.hour);
	printf("GPS_minute == %d\n", p_FAWACPInfo_Handle->VehicleCondData.GPSData.minute);
	printf("GPS_second == %d\n", p_FAWACPInfo_Handle->VehicleCondData.GPSData.second);
	#endif
	return 0;
}


int mcuUart::unpack_updateFAWACPVehCondInfo(unsigned char* pData, unsigned int len)
{
	static uint8_t VehVoiceDataCall = 0;
	static uint8_t PowerONStatus = 0;
	static uint8_t EngineState = 0;
	static uint8_t BCELLStatus = 0;
	uint8_t VehSkylightUnclose = 0;
	uint8_t DoorIntrusAlarm = 0;
	uint8_t VehOutFileAlarm = 0;
	uint8_t VehCollideAlarm = 0;
	uint8_t VehMoveAlarm = 0;
	static int i = 0;
	//************************车况数据***************************//
	p_FAWACPInfo_Handle->VehicleCondData.RemainedOil.RemainedOilGrade = pData[30] & 0x0F;//剩余油量等级
	p_FAWACPInfo_Handle->VehicleCondData.RemainedOil.RemainedOilValue = pData[31];	//剩余油量数值
	p_FAWACPInfo_Handle->VehicleCondData.Odometer = (pData[32] << 24) + (pData[33] << 16) + (pData[34] << 8) + pData[35];//总里程Last Value
	p_FAWACPInfo_Handle->VehicleCondData.Battery = ((pData[36] << 8) & 0xFF00) | (pData[37] & 0xFF);	   //蓄电池电量
	p_FAWACPInfo_Handle->VehicleCondData.CurrentSpeed = ((pData[38] << 8) & 0xFF00) | (pData[39] & 0xFF);  //实时车速
	//历史车速
	p_FAWACPInfo_Handle->VehicleCondData.PastRecordSpeed[i%19] = p_FAWACPInfo_Handle->VehicleCondData.CurrentSpeed;
	
	p_FAWACPInfo_Handle->VehicleCondData.LTAverageSpeed = ((pData[40] << 8) & 0xFF00) | (pData[41] & 0xFF);	//长时平均速度
	p_FAWACPInfo_Handle->VehicleCondData.STAverageSpeed = ((pData[42] << 8) & 0xFF00) | (pData[43] & 0xFF);	//短时平均速度
	p_FAWACPInfo_Handle->VehicleCondData.LTAverageOil = ((pData[44] << 8) & 0xFF00) | (pData[45] & 0xFF);//长时平均油耗
	p_FAWACPInfo_Handle->VehicleCondData.STAverageOil = ((pData[46] << 8) & 0xFF00) | (pData[47] & 0xFF);//短时平均油耗
	//车门状态
	p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.bitState.drivingDoor = pData[48] & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.bitState.copilotDoor = (pData[48] >> 1) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.bitState.leftRearDoor = (pData[48] >> 2) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.bitState.rightRearDoor = (pData[48] >> 3) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.bitState.rearCanopy = (pData[48] >> 4) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.CarDoorState.bitState.engineCover = (pData[48] >> 5) & 0x01;
	//车锁状态
	p_FAWACPInfo_Handle->VehicleCondData.CarLockState.bitState.rightRearLock = pData[49] & 0x03;
	p_FAWACPInfo_Handle->VehicleCondData.CarLockState.bitState.leftRearLock = (pData[49] >> 2) & 0x03;
	p_FAWACPInfo_Handle->VehicleCondData.CarLockState.bitState.copilotLock = (pData[49] >> 4) & 0x03;
	p_FAWACPInfo_Handle->VehicleCondData.CarLockState.bitState.drivingLock = (pData[49] >> 6) & 0x03;
	//天窗状态
	p_FAWACPInfo_Handle->VehicleCondData.sunroofState = pData[50];
	//车窗状态数据
	p_FAWACPInfo_Handle->VehicleCondData.WindowState.bitState.leftFrontWindow = pData[52] & 0x07;
	p_FAWACPInfo_Handle->VehicleCondData.WindowState.bitState.rightFrontWindow = (pData[52] >> 3) & 0x07;
	p_FAWACPInfo_Handle->VehicleCondData.WindowState.bitState.leftRearWindow = ((pData[52] >> 6) & 0x03) + ((pData[51] << 2) & 0x04);
	p_FAWACPInfo_Handle->VehicleCondData.WindowState.bitState.rightRearWindow = ((pData[51] >> 1) & 0x07);
	//车灯状态数据
	p_FAWACPInfo_Handle->VehicleCondData.CarlampState.bitState.headlights = pData[53] & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.CarlampState.bitState.positionlights = (pData[53] >> 1) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.CarlampState.bitState.nearlights = (pData[53] >> 2) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.CarlampState.bitState.rearfoglights = (pData[53] >> 3) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.CarlampState.bitState.frontfoglights = (pData[53] >> 4) & 0x01;
	//轮胎信息数据
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.rightrearTyrePress = pData[61];
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.leftrearTyrePress = pData[60];
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.rightfrontTyrePress = pData[59];
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.leftfrontTyrePress = pData[58];
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.rightrearTemperature = pData[57];
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.leftrearTemperature = pData[56];
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.rightfrontTemperature = pData[55];
	p_FAWACPInfo_Handle->VehicleCondData.TyreState.leftfrontTemperature = pData[54];
	//TBox_MCU版本
	memcpy(p_FAWACPInfo_Handle->VehicleCondData.VerTboxMCU, &pData[62], 12);
	//发动机状态
	p_FAWACPInfo_Handle->VehicleCondData.EngineState = pData[74] & 0x07;
	//实时方向盘转角数据
	p_FAWACPInfo_Handle->VehicleCondData.WheelState.bitState.wheeldegree = ((pData[75] << 8) & 0x7F00) | (pData[76] & 0xFF);
	p_FAWACPInfo_Handle->VehicleCondData.WheelState.bitState.wheeldirection = (pData[75] >> 7) & 0x01;
	//历史方向盘转角数据
	p_FAWACPInfo_Handle->VehicleCondData.PastRecordWheel[i%19].WheelState = p_FAWACPInfo_Handle->VehicleCondData.WheelState.WheelState;
	//发动机转速
	p_FAWACPInfo_Handle->VehicleCondData.EngineSpeed = (pData[77] << 8) + pData[78];
	//档位信息
	p_FAWACPInfo_Handle->VehicleCondData.Gearstate = pData[79] & 0x0F;
	//手刹状态
	p_FAWACPInfo_Handle->VehicleCondData.HandbrakeState = pData[80] & 0x03;
	//电子驻车状态
	p_FAWACPInfo_Handle->VehicleCondData.ParkingState = pData[81] & 0x03;
	//供电模式 OFF == Lock， ACC == Accessory， ON == on，点火挡 == start，其余状态传无效值
	p_FAWACPInfo_Handle->VehicleCondData.Powersupplymode = pData[82] & 0x0F;
	if(i%4 == 0)
	{
		if(!PowerONStatus){
			if(CFAWACP::cfawacp->m_loginState == 2 && AccStatus == 1){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 4, ACPApp_VehicleCondUploadID);//上报车辆上电事件
				PowerONStatus++;
			}
		}else{
			if(CFAWACP::cfawacp->m_loginState == 2 && AccStatus == 0){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 17, ACPApp_VehicleCondUploadID);//上报车辆下电事件
				PowerONStatus = 0;
			}
		}
	}
	//安全带状态
	p_FAWACPInfo_Handle->VehicleCondData.Safetybeltstate = pData[83] & 0x03;
	//剩余保养里程
	p_FAWACPInfo_Handle->VehicleCondData.RemainUpkeepMileage = (pData[84] << 8) + pData[85];
	//空调相关信息
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.airconditionerState = pData[88] & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.compressorState = (pData[88] >> 1) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.autoState = (pData[88] >> 2) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.defrostState = (pData[88] >> 3) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.inOutCirculateState = (pData[88] >> 4) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.blowingLevel = (pData[88] >> 5) & 0x07;
	p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.blowingMode = pData[87] & 0x07;
	uint8_t ACTemperature = ((pData[87] >> 3) & 0x1F) + ((pData[86] & 0x03) << 5);
	switch(ACTemperature)
	{
		case 0x00:
			p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature = 32;
		break;
		case 0x01:
			p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature = 33;
		break;
		case 0x02:
			p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature = 34;
		break;
		case 0x03:
			p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature = 35;
		break;
		case 0x04:
			p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature = 36;
		break;
		case 0x05:
			p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature = 37;
		break;
		case 0x06:
			p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature = 38;
		break;
		case 0x07:
			p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature = 39;
		break;
		case 0x08:
			p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature = 40;
		break;
		default:
			p_FAWACPInfo_Handle->VehicleCondData.AirconditionerInfo.Temperature = 41;
		break;
	}
	//持续时间信息
	p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.accelerateTime = pData[93];
	p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.decelerateTime = pData[92];
	p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.wheelTime = pData[91];
	p_FAWACPInfo_Handle->VehicleCondData.KeepingstateTime.overspeedTime = ((pData[89] << 8) & 0xFF00) | pData[90];
	//动力电池
	p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.BatAveraTempera = pData[94];
	p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.elecTempera = pData[95];
	p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.elecSOH = pData[96];
	p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.quantity = pData[97];
	p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.electricity = ((pData[98] << 8) & 0xFF00) | (pData[99] & 0xFF);
	p_FAWACPInfo_Handle->VehicleCondData.PowerCellsState.voltage = ((pData[100] << 8) & 0xFF00) | (pData[101] & 0xFF);
	//充电状态数据
	p_FAWACPInfo_Handle->VehicleCondData.ChargeState.chargeState = pData[104];
	p_FAWACPInfo_Handle->VehicleCondData.ChargeState.remainChargeTime = pData[103];
	p_FAWACPInfo_Handle->VehicleCondData.ChargeState.chargeMode = pData[102] & 0x03;
	//TBOX-OS版本
	//memcpy(p_FAWACPInfo_Handle->VehicleCondData.VerTboxOS, &pData[105], sizeof(p_FAWACPInfo_Handle->VehicleCondData.VerTboxOS));
	memcpy(p_FAWACPInfo_Handle->VehicleCondData.VerTboxOS, TBOX_VERSION, sizeof(p_FAWACPInfo_Handle->VehicleCondData.VerTboxOS));
	//IVI版本
	//memcpy(p_FAWACPInfo_Handle->VehicleCondData.VerIVI, &pData[117], sizeof(p_FAWACPInfo_Handle->VehicleCondData.VerIVI));
	memcpy(p_FAWACPInfo_Handle->VehicleCondData.VerIVI, IVI_VERSION, sizeof(p_FAWACPInfo_Handle->VehicleCondData.VerIVI));
	//充电枪连接状态
	p_FAWACPInfo_Handle->VehicleCondData.ChargeConnectState = pData[133] & 0x0F;
	//制动踏板开关
	p_FAWACPInfo_Handle->VehicleCondData.BrakePedalSwitch = pData[134] & 0x03;
	//加速踏板开关
	p_FAWACPInfo_Handle->VehicleCondData.AcceleraPedalSwitch = pData[135];

	if(i%4 >= 2)
	{
	if(EngineState != p_FAWACPInfo_Handle->VehicleCondData.EngineState){
		//发动机状态由OFF,Cranking,stoping,InValid变为RUN(Cranking和stoping瞬时状态)
		if(CFAWACP::cfawacp->m_loginState == 2 && p_FAWACPInfo_Handle->VehicleCondData.EngineState == 2)
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 5, ACPApp_VehicleCondUploadID);
		//发动机状态由RUN，Cranking，stoping，InValid变为OFF
		if(CFAWACP::cfawacp->m_loginState == 2 && EngineState == 2)
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 15, ACPApp_VehicleCondUploadID);
		EngineState = p_FAWACPInfo_Handle->VehicleCondData.EngineState;
	}
	}
	i++;
	//YAW传感器信号
	p_FAWACPInfo_Handle->VehicleCondData.YaWSensorInfoSwitch.TransverseAccele = ((pData[136] << 8) & 0x0F00) | pData[137];
	p_FAWACPInfo_Handle->VehicleCondData.YaWSensorInfoSwitch.LongituAccele = ((pData[138] << 8) & 0x0F00) | pData[139];
	p_FAWACPInfo_Handle->VehicleCondData.YaWSensorInfoSwitch.YawVelocity = ((pData[140] << 8) & 0x0F00) | pData[141];
	//环境温度
	p_FAWACPInfo_Handle->VehicleCondData.AmbientTemperat.AmbientTemperat = ((pData[142] << 8) & 0x0700) | pData[143];
	//纯电动继电器及线圈状态
	p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.MainPositRelayCoilState = pData[144] & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.MainNegaRelayCoilState = (pData[144] >> 1) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.PrefillRelayCoilState = (pData[144] >> 2) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.RechargePositRelayCoilState = (pData[144] >> 3) & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.PureElecRelayState.RechargeNegaRelayCoilState = (pData[144] >> 4) & 0x01;
	//剩余续航里程
	p_FAWACPInfo_Handle->VehicleCondData.ResidualRange = ((pData[145] << 8) & 0xFF00) | pData[146];
	//新能源热管理请求
	p_FAWACPInfo_Handle->VehicleCondData.NewEnergyHeatManage.BatteryHeatRequest = pData[147] & 0x01;
	p_FAWACPInfo_Handle->VehicleCondData.NewEnergyHeatManage.Motor1CoolRequest = (pData[147] >> 1) & 0x07;
	p_FAWACPInfo_Handle->VehicleCondData.NewEnergyHeatManage.Motor2CoolRequest = (pData[147] >> 4) & 0x07;
	//车辆工作模式
	p_FAWACPInfo_Handle->VehicleCondData.VehWorkMode.VehWorkMode = pData[148] & 0x0F;
	//电机工作状态
	p_FAWACPInfo_Handle->VehicleCondData.MotorWorkState.Motor1Workstate = pData[149] & 0x07;
	p_FAWACPInfo_Handle->VehicleCondData.MotorWorkState.Motor2Workstate = (pData[149] >> 3) & 0x07;
	//高压系统准备状态
	p_FAWACPInfo_Handle->VehicleCondData.HighVoltageState = pData[150] & 0x03;

	//************************故障信息***************************//		
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEMSState = pData[151];		//发动机管理系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEMSState != FaultSigan.ACPCODEFAULTEMSState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEMSState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTEMSState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEMSState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTCUState = pData[152];		//变速箱控制单元故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTCUState != FaultSigan.ACPCODEFAULTTCUState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTCUState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTTCUState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTCUState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEmissionState = pData[153];//排放系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEmissionState != FaultSigan.ACPCODEFAULTEmissionState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEmissionState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTEmissionState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEmissionState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSRSState = pData[154];		//安全气囊系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSRSState != FaultSigan.ACPCODEFAULTSRSState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSRSState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTSRSState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSRSState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESPState = pData[155];		//电子稳定系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESPState != FaultSigan.ACPCODEFAULTESPState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESPState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTESPState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESPState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTABSState = pData[156];		//防抱死刹车系统
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTABSState != FaultSigan.ACPCODEFAULTABSState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTABSState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTABSState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTABSState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEPASState = pData[157];		//电子助力转向系统
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEPASState != FaultSigan.ACPCODEFAULTEPASState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEPASState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTEPASState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEPASState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTOilPressureState = pData[158];	//机油压力报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTOilPressureState != FaultSigan.ACPCODEFAULTOilPressureState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTOilPressureState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTOilPressureState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTOilPressureState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLowOilIDState = pData[159];	//油量低报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLowOilIDState != FaultSigan.ACPCODEFAULTLowOilIDState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLowOilIDState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTLowOilIDState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLowOilIDState;
		}
		
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBrakeFluidLevelState = pData[160];	//制动液位报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBrakeFluidLevelState != FaultSigan.ACPCODEFAULTBrakeFluidLevelState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBrakeFluidLevelState != 0&&CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTBrakeFluidLevelState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBrakeFluidLevelState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBatteryChargeState = pData[161];		//蓄电池电量低报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBatteryChargeState != FaultSigan.ACPCODEFAULTBattChargeState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBatteryChargeState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTBattChargeState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBatteryChargeState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBBWState = pData[162];		//制动系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBBWState != FaultSigan.ACPCODEFAULTBBWState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBBWState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTBBWState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBBWState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTPMSState = pData[163];		//胎压系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTPMSState != FaultSigan.ACPCODEFAULTTPMSState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTPMSState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTTPMSState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTPMSState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSTTState = pData[164];		//启停系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSTTState != FaultSigan.ACPCODEFAULTSTTState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSTTState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTSTTState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSTTState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTExtLightState = pData[165];//外部灯光故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTExtLightState != FaultSigan.ACPCODEFAULTExtLightState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTExtLightState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTExtLightState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTExtLightState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESCLState = pData[166];		//电子转向柱锁故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESCLState != FaultSigan.ACPCODEFAULTESCLState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESCLState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTESCLState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESCLState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEngineOverwaterState = pData[167];//发动机水温过高报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEngineOverwaterState != FaultSigan.ACPCODEFAULTEngineOverwaterState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEngineOverwaterState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTEngineOverwaterState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEngineOverwaterState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTElecParkUnitState = pData[168];	//电子驻车单元系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTElecParkUnitState != FaultSigan.ACPCODEFAULTElecParkUnitState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTElecParkUnitState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTElecParkUnitState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTElecParkUnitState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAHBState = pData[169];	//智能远光系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAHBState != FaultSigan.ACPCODEFAULTAHBState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAHBState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTAHBState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAHBState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTACSState = pData[170];	//自适应巡航系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTACSState != FaultSigan.ACPCODEFAULTACSState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTACSState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTACSState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTACSState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWSState = pData[171];	//前碰撞预警系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWSState != FaultSigan.ACPCODEFAULTFCWSState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWSState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTFCWSState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWSState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLDWState = pData[172];	//道路偏离预警系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLDWState != FaultSigan.ACPCODEFAULTLDWState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLDWState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTLDWState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLDWState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBlindSpotDetectState = pData[173];//盲区检测系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBlindSpotDetectState != FaultSigan.ACPCODEFAULTBlindSpotDetectState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBlindSpotDetectState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTBlindSpotDetectState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBlindSpotDetectState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAirconManualState = pData[174];	//空调人为操作
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAirconManualState != FaultSigan.ACPCODEFAULTAirconManualState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAirconManualState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTAirconManualState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAirconManualState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVSystemState = pData[175];	//高压系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVSystemState != FaultSigan.ACPCODEFAULTHVSystemState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVSystemState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTHVSystemState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVSystemState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVInsulateState = pData[176];	//高压绝缘故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVInsulateState != FaultSigan.ACPCODEFAULTHVInsulateState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVInsulateState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTHVInsulateState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVInsulateState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVILState = pData[177];	//高压互锁故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVILState != FaultSigan.ACPCODEFAULTHVILState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVILState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTHVILState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTHVILState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellState = pData[178];	//动力电池故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellState != FaultSigan.ACPCODEFAULTEVCellState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTEVCellState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPowerMotorState = pData[179];	//动力电机故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPowerMotorState != FaultSigan.ACPCODEFAULTPowerMotorState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPowerMotorState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTPowerMotorState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPowerMotorState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEParkState = pData[180];	//E-Park故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEParkState != FaultSigan.ACPCODEFAULTEParkState){
			if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEParkState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTEParkState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEParkState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellLowBatteryState = pData[181];	//动力电池电量过低报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellLowBatteryState != FaultSigan.ACPCODEFAULTEVCellLowBatteryState){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellLowBatteryState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}	
			FaultSigan.ACPCODEFAULTEVCellLowBatteryState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellLowBatteryState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellOverTemperateState = pData[182];	//动力电池温度过高报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellOverTemperateState != FaultSigan.ACPCODEFAULTEVCellOverTemperateState){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellOverTemperateState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
		}	
			FaultSigan.ACPCODEFAULTEVCellOverTemperateState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTEVCellOverTemperateState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPowerMotorOverTemperateState = pData[183];//动力电机温度过高报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPowerMotorOverTemperateState != FaultSigan.ACPCODEFAULTPowerMotorOverTemperateState){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPowerMotorOverTemperateState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
		}
			FaultSigan.ACPCODEFAULTPowerMotorOverTemperateState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPowerMotorOverTemperateState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTConstantSpeedSystemFailState = pData[184];//定速巡航系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTConstantSpeedSystemFailState != FaultSigan.ACPCODEFAULTConstantSpeedSystemFailState){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTConstantSpeedSystemFailState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
		}
			FaultSigan.ACPCODEFAULTConstantSpeedSystemFailState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTConstantSpeedSystemFailState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTChargerFaultState = pData[185];//充电机故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTChargerFaultState != FaultSigan.ACPCODEFAULTChargerFaultState){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTChargerFaultState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTChargerFaultState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTChargerFaultState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAirFailureState = pData[186];	 //空调故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAirFailureState != FaultSigan.ACPCODEFAULTAirFailureState){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAirFailureState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTAirFailureState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAirFailureState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlternateAuxSystemFailState = pData[187];	 //换道辅助系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlternateAuxSystemFailState != FaultSigan.ACPCODEFAULTAlternateAuxSystemFailState){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlternateAuxSystemFailState != 0 && CFAWACP::cfawacp->m_loginState == 2){
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			}
			FaultSigan.ACPCODEFAULTAlternateAuxSystemFailState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlternateAuxSystemFailState;
		}

		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAutoEmergeSystemFailState = pData[188];	 //自动紧急制动系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAutoEmergeSystemFailState != FaultSigan.ACPCODEFAULTAutoEmergeSystemFailState){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAutoEmergeSystemFailState != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.ACPCODEFAULTAutoEmergeSystemFailState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAutoEmergeSystemFailState;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTReverRadarSystemFailState = pData[189];	 //倒车雷达系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTReverRadarSystemFailState != FaultSigan.ACPCODEFAULTReverRadarSystemFailState){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTReverRadarSystemFailState != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.ACPCODEFAULTReverRadarSystemFailState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTReverRadarSystemFailState;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTElecGearSystemFailState = pData[190];	 //电子换挡器系统故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTElecGearSystemFailState != FaultSigan.ACPCODEFAULTElecGearSystemFailState){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTElecGearSystemFailState != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.ACPCODEFAULTElecGearSystemFailState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTElecGearSystemFailState;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftFrontTirePressAlarm = pData[194] & 0x07;	 //左前胎压报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftFrontTirePressAlarm != FaultSigan.LeftFrontTirePressAlarm){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftFrontTirePressAlarm != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.LeftFrontTirePressAlarm = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftFrontTirePressAlarm;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftFrontTireTempAlarm = (pData[194] >> 3) & 0x07;	 //左前胎温报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftFrontTireTempAlarm != FaultSigan.LeftFrontTireTempAlarm){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftFrontTireTempAlarm != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.LeftFrontTireTempAlarm = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftFrontTireTempAlarm;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightFrontTirePressAlarm = pData[193] & 0x07;	 //右前胎压报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightFrontTirePressAlarm != FaultSigan.RightFrontTirePressAlarm){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightFrontTirePressAlarm != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.RightFrontTirePressAlarm = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightFrontTirePressAlarm;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightrontTireTempAlarm = (pData[193] >> 3) & 0x07;	 //右前胎温报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightrontTireTempAlarm != FaultSigan.RightrontTireTempAlarm){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightrontTireTempAlarm != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.RightrontTireTempAlarm = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightrontTireTempAlarm;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftRearTirePressAlarm = pData[192] & 0x07;	 //左后胎压报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftRearTirePressAlarm != FaultSigan.LeftRearTirePressAlarm){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftRearTirePressAlarm != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.LeftRearTirePressAlarm = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftRearTirePressAlarm;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftRearTireTempAlarm = (pData[192] >> 3) & 0x07;	 //左后胎温报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftRearTireTempAlarm != FaultSigan.LeftRearTireTempAlarm){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftRearTireTempAlarm != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.LeftRearTireTempAlarm = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.LeftRearTireTempAlarm;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightRearTirePressAlarm = pData[191] & 0x07;	 //右后胎压报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightRearTirePressAlarm != FaultSigan.RightRearTirePressAlarm){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightRearTirePressAlarm != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.RightRearTirePressAlarm = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightRearTirePressAlarm;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightRearTireTempAlarm = (pData[191] >> 3) & 0x07;	 //右后胎温报警
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightRearTireTempAlarm != FaultSigan.RightRearTireTempAlarm){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightRearTireTempAlarm != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.RightRearTireTempAlarm = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTyreAlarmState.RightRearTireTempAlarm;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTDCDCConverterFaultState = pData[195];	 //直流直流转换器故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTDCDCConverterFaultState != FaultSigan.ACPCODEFAULTDCDCConverterFaultState){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTDCDCConverterFaultState != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.ACPCODEFAULTDCDCConverterFaultState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTDCDCConverterFaultState;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTVehControllerFailState = pData[196];	 //整车控制器故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTVehControllerFailState != FaultSigan.ACPCODEFAULTVehControllerFailState){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTVehControllerFailState != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.ACPCODEFAULTVehControllerFailState = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTVehControllerFailState;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainPositRelayCoilFault = pData[197] & 0x01;	 //主正继电器线圈状态
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainPositRelayCoilFault != FaultSigan.MainPositRelayCoilFault){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainPositRelayCoilFault != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.MainPositRelayCoilFault = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainPositRelayCoilFault;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainNegaRelayCoilFault = (pData[197] >> 1) & 0x01;	 //主负继电器线圈状态
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainNegaRelayCoilFault != FaultSigan.MainNegaRelayCoilFault){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainNegaRelayCoilFault != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.MainNegaRelayCoilFault = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainNegaRelayCoilFault;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.PrefillRelayCoilFault = (pData[197] >> 2) & 0x01;	 //预充继电器线圈状态
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.PrefillRelayCoilFault != FaultSigan.PrefillRelayCoilFault){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.PrefillRelayCoilFault != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.PrefillRelayCoilFault = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.PrefillRelayCoilFault;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.RechargePositRelayCoilFault = (pData[197] >> 3) & 0x01;	 //充电正继电器线圈状态
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.RechargePositRelayCoilFault != FaultSigan.RechargePositRelayCoilFault){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.RechargePositRelayCoilFault != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.RechargePositRelayCoilFault = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.RechargePositRelayCoilFault;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.RechargeNegaRelayCoilFault = (pData[197] >> 4) & 0x01;	 //充电负继电器线圈状态
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.RechargeNegaRelayCoilFault != FaultSigan.RechargeNegaRelayCoilFault){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.RechargeNegaRelayCoilFault != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.RechargeNegaRelayCoilFault = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.RechargeNegaRelayCoilFault;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainPositiveRelayFault = (pData[197] >> 5) & 0x01;	 //主正继电器故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainPositiveRelayFault != FaultSigan.MainPositiveRelayFault){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainPositiveRelayFault != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.MainPositiveRelayFault = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainPositiveRelayFault;
		}
		p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainNagetiveRelayFault = (pData[197] >> 6) & 0x01;	 //主负继电器故障
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainNagetiveRelayFault != FaultSigan.MainNagetiveRelayFault){
		if(p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainNagetiveRelayFault != 0 && CFAWACP::cfawacp->m_loginState == 2)
				reportEventCmd(CFAWACP::cfawacp->timingReportingData, 18, ACPApp_VehicleCondUploadID);
			FaultSigan.MainNagetiveRelayFault = p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTPureElecRelayCoilState.MainNagetiveRelayFault;
		}

	//************************驾驶行为特殊事件分析***************************//
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTSROverSpeedAlarmState = pData[198];	  //TSR超速报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTSRLimitSpeedState = pData[199];		  //TSR限速
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAEBInterventionState = pData[200];	  //AEB介入
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTABSInterventionState = pData[201];	  //ABS介入
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTASRInterventionState = pData[202];	  //ASR介入
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTESPInterventionState = pData[203];	  //ESP介入
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTDSMAlarmState = pData[204];			  //DSM报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTTowHandOffDiskState = pData[205];		  //双手离开方向盘提示
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTACCState = pData[206];				  //ACC状态
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTACCSetSpeedState = pData[207];		  //ACC速度设定
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWAlarmState = pData[208];		      //FCW报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWState = pData[209];				  //FCW状态
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWAlarmAccePedalFallState = pData[210];//FCW报警后加速踏板陡降时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTFCWAlarmFirstBrakeState = pData[211];   //FCW报警后首次刹车时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTSLDWState = pData[212];				  //LDW状态
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLDWAlarmState = pData[213];			  //LDW报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLDWAlarmDireDiskResponseState = pData[214];//LDW报警后方向盘响应
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLKAState = pData[215];				//LKA状态
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLKAInterventionState = pData[216];	//LKA介入
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLKADriverTakeOverPromptState = pData[217];//LKA驾驶员接管提示
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLKADriverResponsState = pData[218];	//LKA驾驶员接管提示后方向盘响应时
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTLBSDState = pData[219];				//BSD状态
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDLeftSideAlarmState = pData[220];	//BSD左侧报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDRightSideAlarmState = pData[221];	//BSD右侧报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDAlarmReftWheelRespState = pData[222];//BSD报警后方向盘响应时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDAlarmFirstBrakeState = pData[223];	//BSD报警后首次刹车时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTBSDAlarmPedalAcceState = pData[224];	//BSD报警后加速踏板开始陡降时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossLeftReportState = pData[225];	//交叉车流预警左侧报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossRightReportState = pData[226];	//交叉车流预警右侧报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossAlarmWhellState = pData[227];	//交叉车流预警后方向盘报警时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossAlarmStopState = pData[228];		//交叉车流预警报警后首次刹车时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTCrossAlarmAcceTreadleState = pData[229];//交叉车流预警后加速踏板开始陡降时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistLeftAlarmState = pData[230];	//变道辅助左侧报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistRightAlarmState = pData[231];//变道辅助右侧报警
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistDireRepsonState = pData[232];//变道辅助报警后方向盘响应
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistFirstStopState = pData[233];//变道辅助报警后首次刹车时长
	p_FAWACPInfo_Handle->AcpCodeFault.ACPCODEFAULTAlterTrackAssistAcceDropState = pData[234];//变道辅助报警后加速踏板开始陡降时长

	DoorIntrusAlarm = pData[235];	//车门入侵报警
	if(DoorIntrusAlarm != SigEvent.s_DoorIntrusAlarm){
		if(DoorIntrusAlarm != 0 && CFAWACP::cfawacp->m_loginState == 2)
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 1, ACPApp_VehicleAlarmID);
		SigEvent.s_DoorIntrusAlarm = DoorIntrusAlarm;
	}

	VehOutFileAlarm = pData[236];	//紧急熄火报警
	if(VehOutFileAlarm != SigEvent.s_VehOutFileAlarm){
		if(VehOutFileAlarm != 0 && CFAWACP::cfawacp->m_loginState == 2)
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 5, ACPApp_VehicleAlarmID);
		SigEvent.s_VehOutFileAlarm = VehOutFileAlarm;
	}

	VehCollideAlarm = pData[237];	//碰撞信号告警
	if(VehCollideAlarm != SigEvent.s_VehCollideAlarm){
		if(VehCollideAlarm == 1 && CFAWACP::cfawacp->m_loginState == 2)
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 1, ACPApp_EmergencyDataID);
		SigEvent.s_VehCollideAlarm = VehCollideAlarm;
		}

	if(VehVoiceDataCall != VehCollideAlarm){
		if(VehCollideAlarm == 1){
			for(uint8_t iLoop = 0; iLoop < 3; iLoop++){
			if(voiceCall(p_FAWACPInfo_Handle->RemoteDeviceConfigInfo[0].EmergedCall, PHONE_TYPE_E) == 0){
				break;
		}
	}
}
		VehVoiceDataCall = VehCollideAlarm;
	}

	VehMoveAlarm = pData[238];	//车辆异动告警
	if(VehMoveAlarm != SigEvent.s_VehMoveAlarm){
		if(VehMoveAlarm != 0 && CFAWACP::cfawacp->m_loginState == 2)
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 2, ACPApp_VehicleAlarmID);
		SigEvent.s_VehMoveAlarm = VehMoveAlarm;
	}

	VehSkylightUnclose = pData[239];	//天窗未关提示
	if(VehSkylightUnclose != SigEvent.s_VehSkylightUnclose){
		if(VehSkylightUnclose != 0 && CFAWACP::cfawacp->m_loginState == 2)
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 7, ACPApp_VehicleAlarmID);
		SigEvent.s_VehSkylightUnclose = VehSkylightUnclose;
		}
	//B-Cell道路救援信息上报
//	if(BCELLStatus != tboxInfo.operateionStatus.phoneType){
//		if(CFAWACP::cfawacp->m_loginState == 2 && tboxInfo.operateionStatus.phoneType == 2)
//			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 2, ACPApp_EmergencyDataID);
//		BCELLStatus = tboxInfo.operateionStatus.phoneType;
//	}
}



/*****************************************************************************
* Function Name : unpack_text_messages
* Description   : 短信内容
* Input			: unsigned char *pData
*                 unsigned int len
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::unpack_text_messages(unsigned char* pData, unsigned int len)
{
	unsigned char *pos = pData;
	uint8_t retval;
	unsigned char phoneNumberLen;
	unsigned char messageLen;
	char phoneNumber[CONTENT_MAX_LEN];
	char message[CONTENT_MAX_LEN];
	
	memset(phoneNumber, 0, CONTENT_MAX_LEN);
	memset(message, 0, CONTENT_MAX_LEN);

	phoneNumberLen = *(pos+8);
	MCULOG("phoneNumber length:%02x\n", phoneNumberLen);
	memcpy(phoneNumber, (pos+8+1), phoneNumberLen);
	MCULOG("phoneNumber:%s\n", phoneNumber);

	messageLen = *(pos+8+1+phoneNumberLen);
	MCULOG("message length:%02x\n", messageLen);
	memcpy(message, (pos+8+1+phoneNumberLen+1), messageLen);
	MCULOG("message content:%s\n", message);

	if(phoneNumberLen > 0 && messageLen > 0)
	{
		retval = 0;
		packDataWithRespone(TBOX_REPLY_MESSAGES_ID, 0, &retval, 1, 0);
	}
	else
	{
		retval = 1;
		packDataWithRespone(TBOX_REPLY_MESSAGES_ID, 0, &retval, 1, 0);
	}
	
	return 0;
}

/*****************************************************************************
* Function Name : unpack_text_messages
* Description   : tts语音内容
* Input			: unsigned char *pData
*                 unsigned int len
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::unpack_tts_voice(unsigned char* pData, unsigned int len)
{
	unsigned char *pos = pData;
	unsigned char ttsLen;
	char ttsContent[CONTENT_MAX_LEN];
	memset(ttsContent, 0, CONTENT_MAX_LEN);

	ttsLen = *(pos+8);
	MCULOG("tts length:%02x\n", ttsLen);

	memcpy(ttsContent, (pos+8+1), ttsLen);
	MCULOG("ttsContent:%s\n", ttsContent);

	LteAtCtrl->audioPlayTTS(ttsContent);

	return 0;
}

/*****************************************************************************
* Function Name : unpack_querySleepMode
* Description   : 查询是否进入sleep mode
* Input			: unsigned char *pData
*                 unsigned int len
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::unpack_querySleepMode(unsigned char* pData, unsigned int len)
{
	uint16_t serialNumber;

	serialNumber = (pData[4] << 8) + pData[5];
	MCULOG("serialNumber:%d\n",serialNumber);
	
	//if(phoneNumberLen > 0 && messageLen > 0)
	{
		packProtocolData(TBOX_GENERAL_RESP, 0x00, NULL, 0, serialNumber);
		//sleepOrWakeupSystem(2);
	}
	/*else
	{
		packProtocolData(TBOX_GENERAL_RESP, 0x01, NULL, 0, serialNumber);
	}*/

	return 0;
}

/*****************************************************************************
* Function Name : packDataWithRespone
* Description   : 应答函数
* Input			: uint8_t responeCmd
*                 uint8_t subCmd
*                 uint8_t *needToPackData
*                 uint16_t len
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::packDataWithRespone(uint8_t responeCmd, uint8_t subCmd, uint8_t *needToPackData, uint16_t len, uint16_t serialNumber)
{
	int i,headLen;
	int totalDataLen;
	unsigned char *pos = NULL;
	unsigned short int dataLen;
	unsigned int checkCode;
	
	static unsigned short int serialNO = 0;
	unsigned char *pBuff = (unsigned char *)malloc(BUFF_LEN);
	if(pBuff == NULL)
		return -1;
    memset(pBuff, 0, BUFF_LEN);
	
	pos = pBuff;
	
	*pos++ = 0x7e;
	//data length
	*pos++ = 0;
	*pos++ = 0;
	//cmd
	*pos++ = responeCmd;
	//serial NO.
	*pos++ = (serialNO & 0xff00) >> 8;
	*pos++ = (serialNO & 0xff) >> 0;

	//data property: the data which be send has been encrypted
	*pos++ = 0;
	*pos++ = 0;
	*pos++ = (serialNumber & 0xff00) >> 8;
	*pos++ = (serialNumber & 0xff) >> 0;

	headLen = pos-pBuff;
	MCULOG("headLen = %d \t subCmd:%02x\n", headLen, subCmd);

	switch (responeCmd)
	{
		case TBOX_REPORT_4G_STATE:
			dataLen = pack_report4GState(pos, BUFF_LEN-headLen);
			
			break;
		case TBOX_REPLY_MESSAGES_ID:
			dataLen = pack_successMark(pos, needToPackData, len);
			
			break;
		case close_Tbox:
			dataLen = pack_report4GState(pos, BUFF_LEN-headLen);
			closeTbox=0;
			break;
		case reset_Tbox:
			dataLen = pack_report4GState(pos, BUFF_LEN-headLen);
			closeTbox=0;
			break;
		case alarm1:
			dataLen = pack_report4GState(pos, BUFF_LEN-headLen);
			closeTbox=0;
			break;
		case alarm2:
			dataLen = pack_report4GState(pos, BUFF_LEN-headLen);
			closeTbox=0;
			break;
		case alarm3:
			dataLen = pack_report4GState(pos, BUFF_LEN-headLen);
			closeTbox=0;
			break;
		default:
			MCULOG("The cmd error!\n");
			break;
	}

	//MCULOG("dataLen = %d\n",dataLen);
	if(subCmd == 0x01)
	{
	pBuff[1] = (dataLen & 0xff00)>>8;
	pBuff[2] = (dataLen & 0xff)>>0;
	}
	pos += dataLen;
	
	//Calculated check code
	checkCode = Crc16Check(&pBuff[1], pos-pBuff-1);
	MCULOG("checkCode = %0x\n", checkCode);
	
	*pos++ = (checkCode & 0xff00)>>8;
	*pos++ = (checkCode & 0xff)>>0;

	*pos++ = 0x7e;

	totalDataLen = pos-pBuff;
	MCULOG("totalDataLen =%d", totalDataLen);

	MCULOG("\n\n\nBefore escape Data:");
	for(i = 0; i < totalDataLen; i++)
		MCU_NO("%02x ", *(pBuff+i));
	MCU_NO("\n");

	escape_mcuUart_data(pBuff, totalDataLen);

	serialNO++;
	if (serialNO > 10000)
		serialNO = 0;

	if (pBuff != NULL)
	{
		free(pBuff);
		pBuff = NULL;
	}

    return 0;
}

/*****************************************************************************
* Function Name : packProtocolData
* Description   : 打包协议函数
* Input			: uint8_t responeCmd
*                 uint8_t subCmd
*                 uint8_t *needToPackData
*                 uint16_t len
*                 uint16_t serialNum
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::packProtocolData(uint8_t responeCmd, uint8_t subCmd, uint8_t *needToPackData, uint16_t len, uint16_t serialNum)
{
	int i, totalDataLen;
	unsigned char headLen;
	unsigned char *pos = NULL;
	unsigned short int dataLen;
	unsigned int checkCode;
	unsigned short int attribution = 0;
	static unsigned short int serialNO = 0;
	
	unsigned char *pData = (unsigned char *)malloc(BUFF_LEN);
	if(pData == NULL)
		return -1;
	memset(pData, 0, BUFF_LEN);

	pos = pData;
	
	*pos++ = 0x7e;
	//data length
	*pos++ = 0;
	*pos++ = 0;
	//cmd
	*pos++ = responeCmd;
	//serial NO.
	*pos++ = (serialNO & 0xff00) >> 8;
	*pos++ = (serialNO & 0xff) >> 0;

	//data property: the data which be send has been encrypted
	*pos++ = 0;
	*pos++ = 0;

	headLen = pos-pData;
	MCULOG("headLen = %d\n", headLen);

	switch (responeCmd)
	{
		case TBOX_GENERAL_RESP:
			//Response command
			*pos++ = subCmd;
			//Response serial number
			*pos++ = (serialNum & 0xff00) >> 8;
			*pos++ = (serialNum & 0xff) >> 0;
			*pos++ = 0x00;
			dataLen = pos-pData-headLen;
			break;
		case TBOX_REPLY_ID:
			*pos++ = *needToPackData;
			dataLen = pos-pData-headLen;
			break;
		case TBox_CHECK_REPORT:
			*pos++ = *needToPackData;
			dataLen = pos-pData-headLen;
			break;
		case MCU_Factory_Test:
			*pos++ = 0x01;
			*pos++ = subCmd;
			switch(subCmd)
			{
				case 0x50:
					*pos++=0x01;
					if(len==0)
						*pos++=0x00;
					else
						*pos++=0x01;
					break;
				case 0x51:
					*pos++=0x01;
					if(len==0)
						*pos++=0x00;
					else
						*pos++=0x01;
					break;
				case 0x52:
					*pos++=0x01;				
					*pos++=ConfigShow.PowerDomain;			
					break;
				case 0x53:
					*pos++=0x11;
					memcpy(pos,needToPackData,17);
					//memcpy(pos,"LDPZYB6D9JF255129",17);
					pos+=17;
					break;
				case 0x54:
					*pos++=0x06;
					memcpy(pos,ConfigShow.SK,6);	
					pos += 6;
					break;
				case 0x55:
					*pos++=0x01;
					//*pos++=ConfigShow.VehicleType;
					//memcpy(pos,"0000000000000000",16);
					//pos+=16;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					*pos++=0x00;
					break;
				case 0x56:
					*pos++=0x01;
					if(len==0)
						*pos++=0x00;
					else
						*pos++=0x01;
					break;
				default:
					break;
			}
			dataLen = pos-pData-headLen;
			break;
		case TBOX_REQ_CFGINFO:
			*pos++ = *needToPackData;
			dataLen = pos-pData-headLen;
			break;
		case TBOX_ROOTKRY_RESULT:
			if(len == -1)
				*pos++ = 1;		//1:失败	 0:成功
			else
				*pos++ = 0;
			break;
		default:
			break;
	}
	MCULOG("dataLen = %d\n",dataLen);
	pData[1] = (dataLen & 0xff00)>>8;
	pData[2] = (dataLen & 0xff)>>0;

	//pos += dataLen;
	
	//Calculated check code
	checkCode = Crc16Check(&pData[1], pos-pData-1);
	MCULOG("checkCode = %0x\n", checkCode);
	
	*pos++ = (checkCode & 0xff00)>>8;
	*pos++ = (checkCode & 0xff)>>0;

	*pos++ = 0x7e;

	totalDataLen = pos-pData;
	MCULOG("totalDataLen =%d", totalDataLen);

	MCULOG("Before escape Data:");
	for(i = 0; i < totalDataLen; i++)
		MCU_NO("%02x ", *(pData+i));
	MCU_NO("\n");

	serialNO++;
	if (serialNO > 10000)
		serialNO = 0;

	escape_mcuUart_data(pData, totalDataLen);

	if (pData != NULL)
	{
		free(pData);
		pData = NULL;
	}

	return 0;
}

/*****************************************************************************
* Function Name : escape_mcuUart_data
* Description   : 数据转义函数
* Input			: unsigned char *pData
*                 int len
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int mcuUart::escape_mcuUart_data(unsigned char *pData, int len)
{
	int i;
	int totalEscapeDataLen;
	int escapeDataLen;
	int escapeTimes = 0;
	unsigned char *pBuffData = NULL;

	unsigned char *pBuff = (unsigned char *)malloc(BUFF_LEN);
	if(pBuff == NULL)
		return -1;
    memset(pBuff, 0, BUFF_LEN);

	pBuffData = pBuff;
	*pBuffData = 0x7e;
	//MCULOG("Escape *pBuffData:%02x, pBuff=%02x addr:%x\n",*pBuffData,*pBuff, pBuffData);
	pBuffData++;

	escapeDataLen = len-2;
	//MCULOG("escapeDataLen:%d\n", escapeDataLen);

	for(i = 0; i < escapeDataLen; i++)
	{
		if((pData[i+1] != 0x7e) && (pData[i+1] != 0x7d))
		{
			*pBuffData = pData[i+1];
			pBuffData++;
		}
		else
		{
			if(pData[i+1] == 0x7e)
			{
				*pBuffData++ = 0x7d;
				*pBuffData++ = 0x02;
			}
			else if(pData[i+1] == 0x7d)
			{
				*pBuffData++ = 0x7d;
				*pBuffData++ = 0x01;
			}
			escapeTimes++;
			MCULOG("escapeTimes:%d\n", escapeTimes);
		}		
	}

	*pBuffData++ = 0x7e;

	//MCULOG("escapeTimes:%d\n", escapeTimes);
	totalEscapeDataLen = escapeDataLen+escapeTimes+2;
	//MCULOG("totalEscapeDataLen:%d\n", totalEscapeDataLen);
	
	write(fd, pBuff, totalEscapeDataLen);
	//MCULOG("write data ok!\n");

	/*AES_CBC_encrypt_decrypt();

	printOutMsg(pBuff, totalEscapeDataLen);

	uint8_t outputData[1024];
	uint16_t len1;
	memset(outputData, 0,sizeof(outputData));

	aes_encrypt_cbc(outputData, &len1, pBuff, totalEscapeDataLen);

	printf("\n\nlen1 = %d\n\n", len1);

	uint8_t outputData1[1024];
	uint16_t len2;
	memset(outputData1, 0,sizeof(outputData1));

	aes_decrypt_cbc(outputData1, &len2, outputData, len1);
	
	printf("\n\nlen2 = %d\n\n", len2); */

	if (pBuff != NULL)
	{
		free(pBuff);
		pBuff = NULL;
	}

	return 0;
}


/*****************************************************************************
* Function Name : pack_report4GState
* Description   : 打包4G状态函数
* Input			: unsigned char *pBuff
*                 int length
* Output        : None
* Return        : 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t mcuUart::pack_report4GState(unsigned char *pBuff, int length)
{
uint8_t *pos = pBuff;

	//4G network register status
	if(tboxInfo.networkStatus.networkRegSts == 1)
		*pos++ = 0;
	else
		*pos++ = 1;
	//4G call status
    *pos++ = tboxInfo.operateionStatus.phoneType;

	//4G signal strength
	*pos++ = tboxInfo.networkStatus.signalStrength;

	//wifi status
	if(tboxInfo.operateionStatus.wifiStartStatus == -1)
		*pos++ = 1;
	else
		*pos++ = 0;
	//SIM卡状态
	if(tboxInfo.networkStatus.isSIMCardPulledOut == 1)
		*pos++ = 0;
	else
		*pos++ = 1;
	//USB连接故障
	if(isUsbConnectionStatus == 1)
	*pos++ = 0;
	else
		*pos++ = 1;
	//BT硬件故障
	*pos++ = 0;

	memcpy(pos, TBOX_VERSION, strlen(TBOX_VERSION));
	pos += 11;

    return (uint16_t)(pos - pBuff);
}


/*****************************************************************************
* Function Name : pack_successMark
* Description   : 打包成功标志函数
* Input			: unsigned char *pBuff
*                 uint8_t *pData
*                 uint16_t len
* Output        : None
* Return        : 数据长度
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t mcuUart::pack_successMark(unsigned char *pBuff, uint8_t *pData, uint16_t len)
{
    uint8_t *pos = pBuff;

	*pos++ = (len & 0xff) >> 0;
	*pos++ = *pData;

    return (uint16_t)(pos - pBuff);
}

int mcuUart::unpack_updateTimingInfo(unsigned char *pData, unsigned int len)
{
	uint16_t i;
	
/*	for (i = 0; i < len; i++)
		MCU_NO("%02x ",*(pData+i));
	MCU_NO("\n\n");*/
//	printf("====== Begin Show unpack_updateTimingInfo data ======\n");
	
//	for(int j = 0; j < len; j++)
//	{
//		printf("%02x ", pData[j]);
//		MCULOG("%02x ",pData[j]);
		
//	}
	
//	printf("====== End Show unpack_updateTimingInfo data ======\n");

	unsigned char *pos = pData;
	uint8_t TempData = 0;
	uint16_t WTempData = 0;
	uint32_t DWTempData = 0;
	
	if(NULL != pData && len > 0)
	{
		TempData = *pos++;
		if(TempData == 0)
			TempData = 0xff;
		p_GBT32960_handle->dataInfo.vehicleInfo.vehicleState = TempData;
//		printf("p_GBT32960_handle->dataInfo.vehicleInfo.vehicleState : %02x\n", p_GBT32960_handle->dataInfo.vehicleInfo.vehicleState);
		
//		MCULOG("p_GBT32960_handle->dataInfo.vehicleInfo.vehicleState : %02x\n",p_GBT32960_handle->dataInfo.vehicleInfo.vehicleState);
		
		TempData = *pos++;
		if(TempData == 0)
			TempData = 0xff;
		p_GBT32960_handle->dataInfo.vehicleInfo.chargeState = TempData;
//		printf("p_GBT32960_handle->dataInfo.vehicleInfo.chargeState : %02x\n", p_GBT32960_handle->dataInfo.vehicleInfo.chargeState);
		
		TempData = *pos++;
		if(TempData == 0)
			TempData = 0xff;
		p_GBT32960_handle->dataInfo.vehicleInfo.runMode = TempData;
//		printf("p_GBT32960_handle->dataInfo.vehicleInfo.runMode : %02x\n", p_GBT32960_handle->dataInfo.vehicleInfo.runMode);
		DWTempData = *pos++;
		DWTempData = ((DWTempData&0xff) << 8) | (*pos++);
		DWTempData = ((DWTempData&0xffff) << 8) | (*pos++);
		DWTempData = ((DWTempData&0xffffff) << 8) | (*pos++);		
		//p_GBT32960_handle->dataInfo.vehicleInfo.vehicleMileage = DWTempData * 0.05;
		if(DWTempData>9999999)
		{
			DWTempData=0xfffffffe;
		}
		p_GBT32960_handle->dataInfo.vehicleInfo.vehicleMileage = DWTempData;
		//printf("p_GBT32960_handle->dataInfo.vehicleInfo.vehicleMileage : %02x\n", p_GBT32960_handle->dataInfo.vehicleInfo.vehicleMileage);
		
		WTempData = *pos++;
		WTempData = ((WTempData&0xff) << 8) | (*pos++);
		if(WTempData>10000)
			WTempData = 0xfffe;
		p_GBT32960_handle->dataInfo.vehicleInfo.vehicleVoltage = WTempData ;
		//printf("p_GBT32960_handle->dataInfo.vehicleInfo.vehicleVoltage : %02x\n", p_GBT32960_handle->dataInfo.vehicleInfo.vehicleVoltage);
		
		WTempData = *pos++;
		WTempData = ((WTempData&0xff) << 8) | (*pos++);
		if(WTempData>20000)
			WTempData = 0xfffe;
		p_GBT32960_handle->dataInfo.vehicleInfo.vehicleCurrent = WTempData ;
		//printf("p_GBT32960_handle->dataInfo.vehicleInfo.vehicleCurrent : %02x\n", p_GBT32960_handle->dataInfo.vehicleInfo.vehicleCurrent);
		TempData = *pos++;
		p_GBT32960_handle->dataInfo.vehicleInfo.SOC = TempData;
//		printf("p_GBT32960_handle->dataInfo.vehicleInfo.SOC : %02x\n", p_GBT32960_handle->dataInfo.vehicleInfo.SOC);
		TempData = *pos++;
		if(TempData == 0)
			TempData = 0xff;
		p_GBT32960_handle->dataInfo.vehicleInfo.DcDcState = TempData;
//		printf("p_GBT32960_handle->dataInfo.vehicleInfo.DcDcState : %02x\n", p_GBT32960_handle->dataInfo.vehicleInfo.DcDcState);
		TempData = *pos++;
		p_GBT32960_handle->dataInfo.vehicleInfo.gear = TempData;
	


//		printf("p_GBT32960_handle->dataInfo.vehicleInfo.gear : %02x\n", p_GBT32960_handle->dataInfo.vehicleInfo.gear);
		WTempData = *pos++;
		WTempData = ((WTempData&0xff) << 8) | (*pos++);
		if(WTempData>60000)
			WTempData = 60000;	
		p_GBT32960_handle->dataInfo.vehicleInfo.insulationResistance = WTempData;
		//printf("p_GBT32960_handle->dataInfo.vehicleInfo.insulationResistance : %02x\n", p_GBT32960_handle->dataInfo.vehicleInfo.insulationResistance);
		TempData = *pos++;
		p_GBT32960_handle->dataInfo.vehicleInfo.acceleratorPedalTravelValue = TempData;
		TempData = *pos++;
		p_GBT32960_handle->dataInfo.vehicleInfo.brakePedalState = TempData;
		TempData = *pos++;
		p_GBT32960_handle->dataInfo.driveMotorInfo.totalDriveMotor = TempData;
//		printf("p_GBT32960_handle->dataInfo.driveMotorInfo.totalDriveMotor : %02x\n", p_GBT32960_handle->dataInfo.driveMotorInfo.totalDriveMotor);
		for(i = 0; i < p_GBT32960_handle->dataInfo.driveMotorInfo.totalDriveMotor; i++)
		{
			TempData = *pos++;
			if(TempData == 0)
				TempData = 0xff;
			p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorState = TempData;
			TempData = *pos++;
			if(TempData>250)
				TempData = 0xfe;
			p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorControllerTemp = TempData;
			WTempData = *pos++;
			WTempData = ((WTempData&0xff) << 8) | (*pos++);
			if(WTempData > 65531)
				WTempData = 0xfffe;
			p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorRotationalSpeed = WTempData;
			WTempData = *pos++;
			WTempData = ((WTempData&0xff) << 8) | (*pos++);
			if(WTempData > 65531)
				WTempData = 0xfffe;
			p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorTorque = WTempData;
			TempData = *pos++;
			if(TempData>250)
				TempData = 0xfe;
			p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].driveMotorTemp = TempData;
			WTempData = *pos++;
			WTempData = ((WTempData&0xff) << 8) | (*pos++);
			if(WTempData > 60000)
				WTempData = 0xfffe;
			p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].motorControllerVin = WTempData;
			WTempData = *pos++;
			WTempData = ((WTempData&0xff) << 8) | (*pos++);
			if(WTempData > 20000)
				WTempData = 0xfffe;
			p_GBT32960_handle->dataInfo.driveMotorInfo.driveMotor[i].motorControllerDC = WTempData;
		}

		TempData = *pos++;
		if(TempData > 250)
			TempData = 0xfe;
		p_GBT32960_handle->dataInfo.extremeInfo.MaxVolBatterySubsysNumber = TempData;
		//printf("xtremeInfo.MaxVolBatterySubsysNumber : %02x\n", p_GBT32960_handle->dataInfo.extremeInfo.MaxVolBatterySubsysNumber);
		TempData = *pos++;
		if(TempData > 250)
			TempData = 0xfe;
		p_GBT32960_handle->dataInfo.extremeInfo.MaxVolBattery = TempData;
		WTempData = *pos++;
		WTempData = ((WTempData&0xff) << 8) | (*pos++);
		if(WTempData > 15000)
			WTempData = 0xfffe;
		//p_GBT32960_handle->dataInfo.extremeInfo.MaxBatteryVoltageValue = WTempData * 0.001;
		p_GBT32960_handle->dataInfo.extremeInfo.MaxBatteryVoltageValue = WTempData;
		TempData = *pos++;
		if(TempData > 250)
			TempData = 0xfe;
		p_GBT32960_handle->dataInfo.extremeInfo.MinVolBatterySubsysNumber = TempData;
		TempData = *pos++;
		if(TempData > 250)
			TempData = 0xfe;
		p_GBT32960_handle->dataInfo.extremeInfo.MinVolBattery = TempData;
		WTempData = *pos++;
		WTempData = ((WTempData&0xff) << 8) | (*pos++);
		if(WTempData > 15000)
			WTempData = 0xfffe;
		//p_GBT32960_handle->dataInfo.extremeInfo.MinBatteryVoltageValue = WTempData * 0.001;
		p_GBT32960_handle->dataInfo.extremeInfo.MinBatteryVoltageValue = WTempData;
		TempData = *pos++;
		if(TempData > 250)
			TempData = 0xfe;
		p_GBT32960_handle->dataInfo.extremeInfo.MaxTemperatureSubsysNumber = TempData;
		TempData = *pos++;
		if(TempData > 250)
			TempData = 0xfe;
		p_GBT32960_handle->dataInfo.extremeInfo.MaxTemperatureProbe = TempData;
		TempData = *pos++;
		if(TempData > 250)
			TempData = 0xfe;
		p_GBT32960_handle->dataInfo.extremeInfo.MaxTemperatureValue = TempData;
		TempData = *pos++;
		if(TempData > 250)
			TempData = 0xfe;
		p_GBT32960_handle->dataInfo.extremeInfo.MinTemperatureSubsysNumber = TempData;
		TempData = *pos++;
		if(TempData > 250)
			TempData = 0xfe;
		p_GBT32960_handle->dataInfo.extremeInfo.MinTemperatureProbe = TempData;
		TempData = *pos++;
		if(TempData > 250)
			TempData = 0xfe;
		p_GBT32960_handle->dataInfo.extremeInfo.MinTemperatureValue = TempData;

		
		TempData = *pos++;
		p_GBT32960_handle->dataInfo.alarmInfo.maxAlarmLevel = TempData;
		if(!reissue)
		{
				if((3 == p_GBT32960_handle->dataInfo.alarmInfo.maxAlarmLevel) && (faultLevel!=3))
			{
				faultLevel =3;
				m_AlarmCount++;
				pSqliteDatabase->updatetimes(m_Remainder,m_AlarmCount);	
				p_GBT32960_handle->configInfo.localStorePeriod = 1000;
			}
			if((3 != p_GBT32960_handle->dataInfo.alarmInfo.maxAlarmLevel) && (faultLevel==3))
			{
				faultLevel = p_GBT32960_handle->dataInfo.alarmInfo.maxAlarmLevel;
				//p_GBT32960_handle->configInfo.localStorePeriod = 30000;
			}
			//printf("*****faultlever is %d  lever is %d   alarmcount is %d  id is  %d\n",faultLevel,p_GBT32960_handle->dataInfo.alarmInfo.maxAlarmLevel,m_AlarmCount,m_Remainder);
		}
		

		
		DWTempData = *pos++;
		DWTempData = ((DWTempData & 0xff)<< 8) | *pos++;
		DWTempData = ((DWTempData & 0xffff)<< 8) | *pos++;
		DWTempData = ((DWTempData & 0xffffff)<< 8) | *pos++;
		p_GBT32960_handle->dataInfo.alarmInfo.generalAlarm.AlarmFlag = DWTempData;	
//		printf("====== fault. ======\n");
		pos += 10;  //从电池故障开始
		TempData = *pos++;
		p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEnergyStorageDev = TempData;
//		printf("alarmInfo.totalFaultNumOfEnergyStorageDev : %02x\n", p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEnergyStorageDev);
		for(i = 0; i < p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEnergyStorageDev; i++ )
		{
			DWTempData = *pos++;
			DWTempData = ((DWTempData & 0xff)<< 8) | *pos++;
			DWTempData = ((DWTempData & 0xffff)<< 8) | *pos++;
			DWTempData = ((DWTempData & 0xffffff)<< 8) | *pos++;
			p_GBT32960_handle->dataInfo.alarmInfo.energyStorageDevFault[i].fault = DWTempData;
		}
		
		if(0 == p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfEnergyStorageDev)
		{
			pos += 4;
		}
//		printf("alarmInfo.energyStorageDevFault[i].fault : %08x\n", p_GBT32960_handle->dataInfo.alarmInfo.energyStorageDevFault[i].fault);
		
		TempData = *pos++;
		p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfDriveMotor = TempData;
		for(i = 0; i < p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfDriveMotor; i++ )
		{
			DWTempData = *pos++;
			DWTempData = ((DWTempData & 0xff)<< 8) | *pos++;
			DWTempData = ((DWTempData & 0xffff)<< 8) | *pos++;
			DWTempData = ((DWTempData & 0xffffff)<< 8) | *pos++;
			p_GBT32960_handle->dataInfo.alarmInfo.driveMotorFault[i].fault = DWTempData;
		}
		
		if(0 == p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfDriveMotor)
		{
			pos += 4;
		}
		/*TempData = *pos++;
		p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfOther = TempData;
		for(i = 0; i < p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfOther; i++ )
		{
			DWTempData = *pos++;
			DWTempData = ((DWTempData & 0xff)<< 8) | *pos++;
			DWTempData = ((DWTempData & 0xffff)<< 8) | *pos++;
			DWTempData = ((DWTempData & 0xffffff)<< 8) | *pos++;
			p_GBT32960_handle->dataInfo.alarmInfo.otherFault[i].fault = DWTempData;
		}
		if(0 == p_GBT32960_handle->dataInfo.alarmInfo.totalFaultNumOfDriveMotor)
		{
			pos += 4;
		}
		pos += 95; //忽略其它故障*/
		pos += 90; //忽略其它故障
		MCULOG("====== hahahahahha%02x ======\n",*pos);
		MCULOG("====== Begin Show unpack_updateTimingInfo after fault data ======\n");
		uint16_t leftLen = len - (pos - pData);
		MCULOG("Len : %d\n", leftLen);
/*		for(j = 0; j < leftLen; j++)
		{
			MCULOG("%02x ", pos[j]);
		}
		MCULOG("\n");*/
		
		TempData = *pos++;
		if(TempData > 1)
		{
			TempData = 1;
		}
		p_GBT32960_handle->dataInfo.ESDSubsysInfo.totalEnergySubsys = TempData;
		
		for(i =0; i < p_GBT32960_handle->dataInfo.ESDSubsysInfo.totalEnergySubsys; i++)
		{
			TempData = *pos++;
			p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysNumber = TempData;
			WTempData = *pos++;
			WTempData = ((WTempData&0xff) << 8) | (*pos++);
			p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysVol = WTempData;
			//printf("subsysVol : %04x\n", p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysVol);
			WTempData = *pos++;
			WTempData = ((WTempData&0xff) << 8) | (*pos++);
			p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysCurrent = WTempData;
			//printf("Current : %04x\n", p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysCurrent);
			WTempData = *pos++;
			WTempData = ((WTempData&0xff) << 8) | (*pos++);
			/*if(WTempData > 1)
			{
				WTempData = 1;
			}*/
			p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].totalNumOfSingleCell = WTempData;
			WTempData = *pos++;
			WTempData = ((WTempData&0xff) << 8) | (*pos++);
			if(WTempData == 0)
			{
				WTempData = 1;
			}
			p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].frameStartCellNumber = WTempData;
			TempData = *pos++;
			/*if(TempData > 1)
			{
				TempData = 1;
			}*/
			
			p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].frameStartCellTotal = TempData;
			
			int j = 0; 
			for(j = 0; j < p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].frameStartCellTotal ; j++)
			{
				WTempData = *pos++;
				WTempData = ((WTempData&0xff) << 8) | (*pos++);
				p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].monomerCellVol[j] = WTempData;
				
			}
			
			WTempData = *pos++;
			WTempData = ((WTempData&0xff) << 8) | (*pos++);
			/*if(WTempData > 1)
			{
				WTempData = 1;
			}*/
			p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysProbeNumber = WTempData;
			
			//printf("ESDSubsysInfo.ESDevSubsysVol[i].subsysProbeNumber : %04x\n", p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysProbeNumber);
			for(j = 0; j < p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysProbeNumber; j++)
			{
				TempData = *pos++;
				if(TempData > 250)
					TempData = 0xfe;
				p_GBT32960_handle->dataInfo.ESDSubsysInfo.ESDevSubsysVol[i].subsysProbeTemper[j] = TempData;
			}	
		}

		WTempData = *pos++;
		WTempData = ((WTempData&0xff) << 8) | (*pos++);
		if(WTempData < 0 || WTempData > 20000)
			WTempData = 0xffff;
		p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellVoltage = WTempData;

		WTempData = *pos++;
		WTempData = ((WTempData&0xff) << 8) | (*pos++);
		if(WTempData < 0 || WTempData > 20000)
			WTempData = 0xffff;
		p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellCurrent = WTempData;

		WTempData = *pos++;
		WTempData = ((WTempData&0xff) << 8) | (*pos++);
		if(WTempData < 0 || WTempData > 60000)
			WTempData = 0xffff;
		p_GBT32960_handle->dataInfo.fuelCellInfo.fuelConsumptionRate = WTempData;

		WTempData = *pos++;
		WTempData = ((WTempData&0xff) << 8) | (*pos++);
		if(WTempData < 0 || WTempData > 2400)
			WTempData = 0xffff;
		p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenSysMaxTemperature = WTempData;

		TempData = *pos++;
		if(TempData<1 || TempData > 252)
			TempData = 0xff;
		p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenSysMaxTemperatureProbeNum = TempData;

		WTempData = *pos++;
		WTempData = ((WTempData&0xff) << 8) | (*pos++);
		if(WTempData < 0 || WTempData > 60000)
			WTempData = 0xffff;
		p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxSolubility = WTempData;

		TempData = *pos++;
		if(TempData<1 || TempData > 252)
			TempData = 0xff;
		p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxSolubilitySensorNum = TempData;

		WTempData = *pos++;
		WTempData = ((WTempData&0xff) << 8) | (*pos++);
		if(WTempData < 0 || WTempData > 1000)
			WTempData = 0xffff;
		p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxPressure = WTempData;

		TempData = *pos++;
		if(TempData<1 || TempData > 252)
			TempData = 0xff;
		p_GBT32960_handle->dataInfo.fuelCellInfo.HydrogenMaxPressureSensorNum = TempData;

		TempData = *pos++;
		p_GBT32960_handle->dataInfo.fuelCellInfo.HighPressureDcDcState = TempData;

		WTempData = *pos++;
		WTempData = ((WTempData&0xff) << 8) | (*pos++);
		if(WTempData < 0 || WTempData > 65531)
			WTempData = 0xffff;
		p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellTempProbeNum = WTempData;

		for(i = 0;i < ( p_GBT32960_handle->dataInfo.fuelCellInfo.fuelCellTempProbeNum); i++)        //*探针温度值
		{
			TempData = *pos++;
				if(TempData > 240)
					TempData = 0xff;
			p_GBT32960_handle->dataInfo.fuelCellInfo.ProbeTemperature[i] = TempData;
		}

		p_GBT32960->updateTBoxParameterInfo();
		
	}
	
	return 0;
}

//int mcuUart::unpack_MCU_Rootkey(unsigned char *pData, uint16_t datalen)
//{
//	unsigned char *pos = pData;
//}



int mcuUart::unpack_RemoteCtrl(unsigned char *pData, uint16_t datalen)
{
	unsigned char *pos = pData;
	uint8_t SubitemCode;
	uint8_t SubitemCodeParam;
	int RemoteCtrlStatus = 0;

	int ret = alarm(0);
	McuReportTimeOut = 1;
	SubitemCode = pData[8];		//信号编码
	SubitemCodeParam = pData[9];//信号值

	switch(SubitemCode)
	{
		case MCUVehBody_Lock:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_Lock = SubitemCodeParam;
			break;
		case MCUVehBody_Window:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_Window = SubitemCodeParam;
			break;
		case MCUVehBody_Sunroof:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_Sunroof = SubitemCodeParam;
			break;
		case MCUVehBody_TrackingCar:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_TrackingCar = SubitemCodeParam;
			break;
		case MCUVehBody_Lowbeam:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_Lowbeam = SubitemCodeParam;
			break;
		case MCUAir_Control:
			memset(&p_FAWACPInfo_Handle->RemoteControlData.Airconditioner, 0, sizeof(AirconditionerList_t));
			pos += 10;
			for(int i = 0; i< SubitemCodeParam; i++)
			{
				switch(*pos++)
				{
					case 0x01:
					//参数含义
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Control.dataBit.dataState = CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemValue[i];
					//控制结果
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Control.dataBit.flag = *pos++;
					break;
					case 0x02:
					//参数含义
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_CompressorSwitch.dataBit.dataState = CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemValue[i];
					//控制结果
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_CompressorSwitch.dataBit.flag = *pos++;
					break;
					case 0x03:
					//参数含义
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Temperature.dataBit.dataState = (CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemValue[i] & 0x7F);
					//控制结果
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Temperature.dataBit.flag = *pos++;
					break;
					case 0x04:
					//参数含义
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_SetAirVolume.dataBit.dataState = (CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemValue[i] & 0x7F);
					//控制结果
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_SetAirVolume.dataBit.flag = *pos++;
					break;
					case 0x05:
					//参数含义
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_FrontDefrostSwitch.dataBit.dataState = CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemValue[i];
					//控制结果
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_FrontDefrostSwitch.dataBit.flag = *pos++;
					break;
					case 0x06:
					//参数含义
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Heatedrear.dataBit.dataState = CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemValue[i];
					//控制结果
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_Heatedrear.dataBit.flag = *pos++;
					break;
					case 0x07:
					//参数含义
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_BlowingMode.dataBit.dataState = (CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemValue[i] & 0x7F);
					//控制结果
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_BlowingMode.dataBit.flag = *pos++;
					break;
					case 0x08:
					//参数含义
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_InOutCirculate.dataBit.dataState = (CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemValue[i] & 0x7F);
					//控制结果
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_InOutCirculate.dataBit.flag = *pos++;
					break;
					case 0x09:
					//参数含义
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_AutoSwitch.dataBit.dataState = CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemValue[i];
					//控制结果
					p_FAWACPInfo_Handle->RemoteControlData.Airconditioner.Airconditioner_AutoSwitch.dataBit.flag = *pos++;
					break;
					default:
					break;
				}
			}
			break;
		case MCUEngineState_Switch:
			p_FAWACPInfo_Handle->RemoteControlData.EngineState.EngineState_Switch = SubitemCodeParam;
			break;
		case MCUVehSeat_DrivingSeat:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleSeat.VehicleSeat_DrivingSeat = SubitemCodeParam;
			break;
		case MCUVehSeat_Copilotseat:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleSeat.VehicleSeat_Copilotseat = SubitemCodeParam;
			break;
		case MCUVehChargeMode_Immediate:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleCharge.VehicleCharge_Immediate = SubitemCodeParam;
			break;
		case MCUVehChargeMode_Appointment:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleCharge.VehicleCharge_Appointment = SubitemCodeParam;
			break;
		case MCUVehChargeMode_EmergeCharg:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleCharge.VehicleCharge_EmergenCharg = SubitemCodeParam;
			break;
		case MCUVeh_WIFIStatus:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleAutopark.VehicleWifiStatus = SubitemCodeParam;
			break;
		case MCUVeh_AutoOUT:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleAutopark.VehicleAutoOut = SubitemCodeParam;
			break;
		case MCUVehBody_LuggageCar:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleBody.VehicleBody_LuggageCar = SubitemCodeParam;
			break;
		case MCUVehSeat_DrivingSeatMomery:
			p_FAWACPInfo_Handle->RemoteControlData.VehicleSeat.VehicleSeat_DrivingSeatMemory = SubitemCodeParam;
			break;
		case MCUVeh_EnterRemoteModeFail:
		{
			switch(SubitemCodeParam)
			{
				case 0:		//车辆处于本地模式
					RCtrlErrorCode = 2;
					break;
				case 1:		//车辆远程认证不匹配
					RCtrlErrorCode = 3;
					break;
				default:
					RCtrlErrorCode = 4;
					break;
			}
		}
			break;
		default:
			break;
	}

	if(MCUVeh_EnterRemoteModeFail != SubitemCode)
	{
		if(SubitemCode == MCUAir_Control)
		{
			for(int i = 0; i < SubitemCodeParam; i++)
			{
				RemoteCtrlStatus += pData[11+(i*2)];
			}
		}
		else
		{
			RemoteCtrlStatus = SubitemCodeParam;
		}
		if(RemoteCtrlStatus > 0)
			RCtrlErrorCode = 1;
		else
			RCtrlErrorCode = 0;
	}

	ReplayRemoteCtrl(CFAWACP::cfawacp->cb_TspRemoteCtrl);

	return 0;
}

//回调ACP发送远程控制应答指定
void mcuUart::ReplayRemoteCtrl(callback_ReplayRemoteCtrl cb_TspRemoteCtrl)
{
	cb_TspRemoteCtrl();	
}

int mcuUart::cb_RemoteCtrlCmd()
{
	int totalDataLen;
	unsigned char *pos = NULL;
	unsigned short dataLen, headLen;
	unsigned int checkCode;
	int SubitemTotal = CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemTotal;
	static unsigned short serialNO = 0;
	uint8_t TempLuggageCarValue = 0;
	unsigned char *pBuff = (unsigned char *)malloc(BUFF_LEN);
	if(pBuff == NULL)
		return -1;
	memset(pBuff, 0, BUFF_LEN);
	pos = pBuff;
	*pos++ = 0x7e;
	//data length
	*pos++ = 0;
	*pos++ = 0;
	//cmd
	*pos++ = 0x84;
	//serial NO.
	*pos++ = (serialNO & 0xff00) >> 8;
	*pos++ = (serialNO & 0xff) >> 0;

	//data property: the data which be send has been encrypted
	*pos++ = 0;
	*pos++ = 0;

	headLen = pos-pBuff;
	for(int i = 0; i < SubitemTotal; i++)
	{
		switch(CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemCode[i])
		{
		case VehicleBody_LockID:
			*pos++ = 0x01;
			break;
		case VehicleBody_WindowID:
			*pos++ = 0x02;
			break;
		case VehicleBody_SunroofID:
			*pos++ = 0x03;
			break;
			case VehicleBody_TrackingCarID:
			*pos++ = 0x04;
				if(!CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemValue[i])
				{
					CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemTotal = 1;
					CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemCode[0] = 255;
					p_FAWACPInfo_Handle->RemoteControlData.FunctionConfigStatus = 0;
					m_mcuUart->ReplayRemoteCtrl(CFAWACP::cfawacp->cb_TspRemoteCtrl);
				    goto NONSUPPORT_FUNCTION;
				}
			break;
		case VehicleBody_LowbeamID:
			*pos++ = 0x05;
			break;
		case Airconditioner_ControlID:
			*pos++ = 0x06;
		    *pos++ = SubitemTotal;
			*pos++ = 0x01;
			break;
		case Airconditioner_CompressorSwitchID:
			*pos++ = 0x02;
			break;
		case Airconditioner_TemperatureID:
		    *pos++ = 0x03;
			break;
		case Airconditioner_SetAirVolumeID:
		    *pos++ = 0x04;
			break;
		case Airconditioner_FrontDefrostSwitchID:
		    *pos++ = 0x05;
			break;
		case Airconditioner_HeatedrearID:
		    *pos++ = 0x06;
			break;
		case Airconditioner_BlowingModeID:
		    *pos++ = 0x07;
			break;
		case Airconditioner_InOutCirculateID:
		    *pos++ = 0x08;
			break;
		case Airconditioner_AutoSwitchID:
		    *pos++ = 0x09;
			break;
		case EngineState_SwitchID:
			*pos++ = 0x0F;
			break;
		case VehicleSeat_DrivingSeatID:
			*pos++ = 0x10;
			break;
		case VehicleSeat_CopilotseatID:
			*pos++ = 0x11;
			break;
		case VehicleBody_LuggageCarID:
			*pos++ = 0x17;
			if(!CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemValue[i])
			{
				TempLuggageCarValue = 1;
			}else{
				CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemTotal = 1;
				CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemCode[0] = 255;
				p_FAWACPInfo_Handle->RemoteControlData.FunctionConfigStatus = 0;
				m_mcuUart->ReplayRemoteCtrl(CFAWACP::cfawacp->cb_TspRemoteCtrl);
				goto NONSUPPORT_FUNCTION;
			}
			break;
		case VehicleChargeMode_ImmediateID:
		case VehicleChargeMode_AppointmentID:
		case VehicleChargeMode_EmergencyCharg:
		case VehicleWIFIStatusID:
		case VehicleAutoOUTID:
		case VehicleSeat_DrivingSeatMomeryID:
		{
			CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemTotal = 1;
			CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemCode[0] = 255;
			p_FAWACPInfo_Handle->RemoteControlData.FunctionConfigStatus = 0;
			m_mcuUart->ReplayRemoteCtrl(CFAWACP::cfawacp->cb_TspRemoteCtrl);
		    goto NONSUPPORT_FUNCTION;
		}
			break;			
		default:
			goto NONSUPPORT_FUNCTION;
			break;
		}
		if(CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemCode[i] == VehicleBody_LuggageCarID)
			*pos++ = TempLuggageCarValue;
		else
			*pos++ = CFAWACP::cfawacp->m_AcpRemoteCtrlList.SubitemValue[i];
	}

	serialNO++;
		dataLen = (unsigned short)(pos-pBuff) - headLen;

		pBuff[1] = (dataLen & 0xff00)>>8;
		pBuff[2] = (dataLen & 0xff)>>0;

		//Calculated check code
		checkCode = m_mcuUart->Crc16Check(&pBuff[1], pos-pBuff-1);
		//MCULOG("checkCode = %0x\n", checkCode);

		*pos++ = (checkCode & 0xff00)>>8;
		*pos++ = (checkCode & 0xff)>>0;

		*pos++ = 0x7e;

		totalDataLen = pos-pBuff;

		if(m_mcuUart->escape_mcuUart_data(pBuff, totalDataLen) == -1)
		{
			MCULOG();
			return -1;
		}

NONSUPPORT_FUNCTION:
	if (serialNO > 65534)
		serialNO = 0;

	if (pBuff != NULL)
	{
		free(pBuff);
		pBuff = NULL;
	}
#if 1
	if(m_mcuUart->McuReportTimeOut == 1){
		alarm(5);
	}
#endif
	return 0;
}
#if 1
void mcuUart::Sig_CtrlAlarm(int signo)
{
	m_mcuUart->McuReportTimeOut = 0;
	cb_RemoteCtrlCmd();
	m_mcuUart->McuReportTimeOut = 1;
}
#endif

int mcuUart::cb_RemoteConfigCmd(uint8_t SubitemCode, uint16_t SubitemVal)
{
	int i,headLen;
	int totalDataLen;
	unsigned char *pos = NULL;
	unsigned short int dataLen = 2;
	unsigned int checkCode;
	static unsigned short int serialNO = 0;
	unsigned char *pBuff = (unsigned char *)malloc(BUFF_LEN);
	if(pBuff == NULL)
		return ;

	memset(pBuff, 0, BUFF_LEN);
	pos = pBuff;

	*pos++ = 0x7e;
	//data length
	*pos++ = 0;
	*pos++ = 0;
	//cmd
	*pos++ = 0x87;
	//serial NO.
	*pos++ = (serialNO & 0xff00) >> 8;
	*pos++ = (serialNO & 0xff) >> 0;

	//data property: the data which be send has been encrypted
	*pos++ = 0;
	*pos++ = 0;

	headLen = pos-pBuff;
	//MCULOG("headLen = %d \t subCmd:%02x\n", headLen, subCmd);

	*pos++ = SubitemCode;
//	*pos++ = (SubitemVal >> 8) & 0xFF;
	*pos++ = SubitemVal & 0xFF;

	MCULOG("dataLen = %d\n",dataLen);

	pBuff[1] = (dataLen & 0xff00)>>8;
	pBuff[2] = (dataLen & 0xff)>>0;

//	pos += dataLen;
	
	//Calculated check code
	checkCode = m_mcuUart->Crc16Check(&pBuff[1], pos-pBuff-1);
	//MCULOG("checkCode = %0x\n", checkCode);
	
	*pos++ = (checkCode & 0xff00)>>8;
	*pos++ = (checkCode & 0xff)>>0;

	*pos++ = 0x7e;

	totalDataLen = pos-pBuff;

	if(m_mcuUart->escape_mcuUart_data(pBuff, totalDataLen) == -1)
	{
		MCULOG();
		return -1;
	}

	serialNO++;
	if (serialNO > 65534)
		serialNO = 0;

	if (pBuff != NULL)
	{
		free(pBuff);
		pBuff = NULL;
	}
	return 0;
}


//MCU驾驶行为数据解析
int mcuUart::unpack_MCU_DriverInfo(unsigned char *pData, uint16_t datalen)
{
	uint8_t SubitemCode = 0;
	uint8_t SubitemValue = 0;
	uint8_t dataLen = 0;

	SubitemCode = pData[8];
	dataLen = pData[9];
	SubitemValue = pData[10];

	switch(SubitemCode)
	{
		case 0x01:	//驾驶循环事件0x0:开始 0x1:结束
		MCULOG("SubitemValue == %d\n", SubitemValue);
		break;
		case 0x02:	//驾驶循环平均速度
		break;
		case 0x03:	//急加速信息上报0x1:开始 0x0:结束
		if(SubitemValue){
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 9, ACPApp_VehicleCondUploadID);
		}else{
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 10, ACPApp_VehicleCondUploadID);
		}		
		break;
		case 0x04:	//急减速信息上报0x1:开始 0x0:结束
		if(SubitemValue){
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 7, ACPApp_VehicleCondUploadID);
		}else{
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 8, ACPApp_VehicleCondUploadID);
		}
		break;
		case 0x05:	//急转弯信息上报0x1:开始 0x0:结束
		if(SubitemValue){
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 11, ACPApp_VehicleCondUploadID);
		}else{
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 12, ACPApp_VehicleCondUploadID);
		}
		break;
		case 0x06:	//超速信息上报0x1:开始 0x0:结束
		if(SubitemValue){
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 13, ACPApp_VehicleCondUploadID);
		}else{
			reportEventCmd(CFAWACP::cfawacp->timingReportingData, 14, ACPApp_VehicleCondUploadID);
		}
		break;
		case 0x07:	//TSR超速事件
		break;
		case 0x08:	//驾驶循环车外温度
		break;
		default:
		break;
	}
}


int mcuUart::unpack_MCU_StatusReport(unsigned char *pData, uint16_t datalen)
{
	uint8_t *pos = &pData[8];
	McuStatus.MainPower = (*pos++ << 8) | *(pos++);
	McuStatus.DeputyPower = *pos++;
	McuStatus.CANStatus   = *pos++;
	memcpy(McuStatus.GyroscopeData, pos, 12);
	pos += 12;
	memcpy(McuStatus.McuVer, pos, 14);
	pos += 14;
	memcpy(McuStatus.HardweraVer, pos, 14);
	pos += 14;
	McuStatus.GpsAntenna = *pos++;
	McuStatus.GpsPoist   = *pos++;
	McuStatus.GpsSatelliteNumber = *pos++;
	for(int i = 0; i < McuStatus.GpsSatelliteNumber; i++)
	{

		
		McuStatus.GpsNumber[i] = *pos++;
		McuStatus.GpsStrength[i] = *pos++;

		//McuStatus.GpsSatelliteInter[i] = (*pos++ << 8) | *(++pos);
	}
	for(int i =0;i<datalen;i++)
	{
		MCULOG("MCU_Factory %d = %02x\n",i,*(pData+i));
	}
	

	return 0;
}

//============================================= mcu upgrade ======================================================
#if 1
int mcuUart::pack_mcuUart_upgrade_data(unsigned char cmd, bool isReceivedDataCorrect, int mcuOrPlcFlag)
{
	int i,headLen;
	int totalDataLen;
	unsigned char *pos = NULL;
	unsigned short int dataLen;
	unsigned int checkCode;
	
	static unsigned short int serialNo = 0;
	unsigned char *pData = (unsigned char *)malloc(BUFF_LEN);
	if(pData == NULL)
		return -1;
    memset(pData, 0, BUFF_LEN);
	
	MCULOG("mcuOrPlcFlag = %d\n",mcuOrPlcFlag);
	pos = pData;
	
	*pos++ = 0x7e;
	//data length
	*pos++ = 0;
	*pos++ = 0;
	//cmd
	*pos++ = cmd;
	//serial NO.
	*pos++ = (serialNo & 0xff00) >> 8;
	*pos++ = (serialNo & 0xff) >> 0;
	//data property: the data which be send has been encrypted
	*pos++ = 0;
	*pos++ = 0;

	headLen = pos-pData;
	MCULOG("headLen = %d\n", headLen);
	
	switch (cmd)
	{
		case TBOX_SEND_UPGRADE_CMD:
			dataLen = pack_upgrade_cmd(pData, headLen, mcuOrPlcFlag);
			break;
		case TBOX_RECV_MCU_APPLY_FOR:
			dataLen = pack_upgrade_data(pData, headLen, isReceivedDataCorrect, mcuOrPlcFlag);
			break;
		case MCU_SEND_COMPLETE:
			system(RM_MCU_FILE);
			break;
		default:
			MCULOG("The cmd error!\n");
			break;
	}
	
	MCULOG("dataLen = %d\n",dataLen);

	pData[1] = (dataLen & 0xff00)>>8;
	pData[2] = (dataLen & 0xff)>>0;

	pos += dataLen;
	
	//Calculated check code
	checkCode = Crc16Check(&pData[1], pos-pData-1);
	MCULOG("checkCode = %0x\n", checkCode);
	
	*pos++ = (checkCode & 0xff00)>>8;
	*pos++ = (checkCode & 0xff)>>0;

	*pos++ = 0x7e;

	totalDataLen = pos-pData;
	MCULOG("totalDataLen =%d", totalDataLen);

	MCULOG("Before escape Data:");
	for(i = 0; i < totalDataLen; i++)
		MCU_NO("%02x ", *(pData+i));
	MCU_NO("\n");
	
	escape_mcuUart_data(pData, totalDataLen);

    serialNo++;
    if (serialNo > 10000)
        serialNo = 0;

	if (pData != NULL)
	{
		free(pData);
		pData = NULL;
	}

    return 0;
}

unsigned short int mcuUart::pack_upgrade_cmd(unsigned char *pData, int len, int mcuOrPlcFlag)
{
	unsigned char fileNameLen;
	unsigned char *pos = NULL;

	if(len != 8)
		return -1;

	pos = pData;
	pos += len;

	if(mcuOrPlcFlag == 0)
	{
		fileNameLen = strlen(MCU_UPGRADE_FILE);
		MCULOG("fileNameLen:%d\n", fileNameLen);
	
		*pos++ = 0x00;
		memcpy(pos, upgradeInfo.mcuVersionSize, 4);
		pos += 4;
		//Check code
		memcpy(pos, upgradeInfo.mcuVersionCrc16, 2);
		pos += 2;
		//MCU upgrade file name length
		*pos++ = fileNameLen;
		memcpy(pos, MCU_UPGRADE_FILE, fileNameLen);
		pos+= fileNameLen;
		//MCU version length
		*pos++ = 0x04;
		//MCU version
		memcpy(pos, upgradeInfo.newMcuVersion, 4);
		pos += 4;
	}
	else if(mcuOrPlcFlag == 1)
	{
		fileNameLen = strlen(PLC_UPGRADE_FILE);
		MCULOG("fileNameLen:%d\n", fileNameLen);
		
		*pos++ = 0x01;
		memcpy(pos, upgradeInfo.verionSize, 4);
		pos += 4;
		//Check code
		memcpy(pos, upgradeInfo.checkCodeCrc16, 2);
		pos += 2;
		//Plc upgrade file name length
		*pos++ = fileNameLen;
		memcpy(pos, PLC_UPGRADE_FILE, fileNameLen);
		pos+= fileNameLen;
		//plc version length
		*pos++ = 0x14;
		//plc version
		memcpy(pos, upgradeInfo.newPlcVersion, 20);
		pos += 20;
	}
	
	//Calculated data length
	MCULOG("Pack data len:%d\n",(int)(pos-pData-len));
	
	return (unsigned short int)(pos-pData-len);
}

unsigned short int mcuUart::pack_upgrade_data(unsigned char *pData, int len, bool isReceivedDataCorrect, int mcuOrPlcFlag)
{
	int nRead;
	unsigned char *pos = NULL;
	unsigned int offsetAddr;
	unsigned short int packetLen;
	unsigned char buff[BUFF_LEN];
	int fileFd = -1;

	if(len != 8)
		return -1;

	pos = pData;
	pos += len;

	if(isReceivedDataCorrect == true)
	{
		*pos++ = 0x00;
		memcpy(pos, dataPacketOffsetAddr, 4);
		pos += 4;
		memcpy(pos, dataPacketLen, 2);
		pos += 2;
		
		//get plc upgrade data's offset addr and len
		offsetAddr	= (dataPacketOffsetAddr[0]) << 24;
		offsetAddr += (dataPacketOffsetAddr[1]) << 16;
		offsetAddr += (dataPacketOffsetAddr[2]) << 8;
		offsetAddr += (dataPacketOffsetAddr[3]) << 0;
		MCULOG("offsetAddr = %d\n", offsetAddr);
		
		packetLen  = (dataPacketLen[0]) << 8;
		packetLen += (dataPacketLen[1]) << 0;
		MCULOG("packetLen = %d\n", packetLen);
		
		if(mcuOrPlcFlag == 0)
		{
			fileFd = open(MCU_UPGRADE_FILE, O_RDONLY);
			if (fileFd < 0)
			{
				MCULOG("Open file:%s error.\n", MCU_UPGRADE_FILE);
				return -1;
			}

			lseek(fileFd, offsetAddr, SEEK_SET);

			memset(buff, 0, BUFF_LEN);
			if((nRead = read(fileFd, buff, BUFF_LEN)) != -1)
			{
				MCULOG("read file %s, nRead = %d\n", MCU_UPGRADE_FILE, nRead);
				memcpy(pos, buff, packetLen);
				pos += packetLen;
			}
			
			close(fileFd);
		}
		else if(mcuOrPlcFlag == 1)
		{
			fileFd = open(PLC_UPGRADE_FILE, O_RDONLY);
			if (fileFd < 0)
			{
				MCULOG("Open file:%s error.\n", PLC_UPGRADE_FILE);
				return -1;
			}

			lseek(fileFd, offsetAddr, SEEK_SET);

			memset(buff, 0, BUFF_LEN);
			if((nRead = read(fileFd, buff, BUFF_LEN)) != -1)
			{
				MCULOG("read file %s, nRead = %d\n", PLC_UPGRADE_FILE, nRead);
				memcpy(pos, buff, packetLen);
				pos += packetLen;
			}
			close(fileFd);
		}
	}
	else
	{
		*pos++ = 0x01;
		MCULOG("Can received mcu respond!\n");
	}
	
	MCULOG("Pack data len:%d\n",(int)(pos-pData-len));
	return (unsigned short int)(pos-pData-len);
}

void mcuUart::mcu_apply_for_data(unsigned char* pData, uint32_t len)
{
	int i;
	unsigned int addr;
	unsigned short int packetLen; 
	unsigned char *pos = pData;
	unsigned int verionLength;
	unsigned int fileDatalen;
	unsigned char fileLen;
	
	fileLen = *(pos+8);
	MCULOG("fileLen:%d\n", fileLen);

	memset(fileName, 0, 64);
	memcpy(fileName, pos+8+1, fileLen);
	MCULOG("fileName:%s\n", fileName);

	//get plc upgrade data's addr and len
	addr  = *(pos+8+1+fileLen) << 24;
	addr += *(pos+8+1+fileLen+1) << 16;
	addr += *(pos+8+1+fileLen+2) << 8;
	addr += *(pos+8+1+fileLen+3) << 0;
	MCULOG("offsetAddr = %d\n", addr);
	
	packetLen  = *(pos+8+1+fileLen+4) << 8;
	packetLen += *(pos+8+1+fileLen+5) << 0;
	MCULOG("packetLen = %d\n", packetLen);

	fileDatalen = addr + (unsigned int)packetLen;
	
	if(strcmp((char*)fileName, MCU_UPGRADE_FILE) == 0)
	{
		upgradeInfo.upgradeFlag = 0;
		verionLength  = (upgradeInfo.mcuVersionSize[0]) << 24;
		verionLength += (upgradeInfo.mcuVersionSize[1]) << 16;
		verionLength += (upgradeInfo.mcuVersionSize[2]) << 8;
		verionLength += (upgradeInfo.mcuVersionSize[3]) << 0;
		
		len = strlen(MCU_UPGRADE_FILE); 
		MCULOG("len:%d\n", len);
		
		if((fileLen == len) && (memcmp(MCU_UPGRADE_FILE, fileName, len) == 0) && \
			(fileDatalen <= verionLength))
		{
			MCULOG("Data offset addr and length:\n");
			memcpy(dataPacketOffsetAddr, pos+8+1+fileLen, 4);
			
			for(i = 0; i<4; i++)
				MCU_NO("%02x ",*(dataPacketOffsetAddr+i));
			MCU_NO("\n");
			
			memcpy(dataPacketLen, pos+8+1+fileLen+4, 2);
			
			for(i = 0; i<2; i++)
				MCU_NO("%02x ",*(dataPacketLen+i));
			MCU_NO("\n");
		
			pack_mcuUart_upgrade_data(TBOX_RECV_MCU_APPLY_FOR, true, 0);	
		}
		else
		{
			MCULOG("Received the file name and len error form mcu!\n");
			pack_mcuUart_upgrade_data(TBOX_RECV_MCU_APPLY_FOR, false, 0);
		}
	}
	else if(strcmp((char*)fileName, PLC_UPGRADE_FILE) == 0)
	{
		upgradeInfo.sendPlcDataToMCUFlag = 0;
		verionLength  = (upgradeInfo.verionSize[0]) << 24;
		verionLength += (upgradeInfo.verionSize[1]) << 16;
		verionLength += (upgradeInfo.verionSize[2]) << 8;
		verionLength += (upgradeInfo.verionSize[3]) << 0;

		len = strlen(PLC_UPGRADE_FILE); 
		MCULOG("len:%d\n", len);

		if((fileLen == len) && (memcmp(PLC_UPGRADE_FILE, fileName, len) == 0) && \
			(fileDatalen <= verionLength))
		{
			MCULOG("Data offset addr and length:\n");
			memcpy(dataPacketOffsetAddr, pos+8+1+fileLen, 4);
			
			for(i = 0; i<4; i++)
				MCU_NO("%02x ",*(dataPacketOffsetAddr+i));
			MCU_NO("\n");
			
			memcpy(dataPacketLen, pos+8+1+fileLen+4, 2);
			
			for(i = 0; i<2; i++)
				MCU_NO("%02x ",*(dataPacketLen+i));
			MCU_NO("\n");
		
			pack_mcuUart_upgrade_data(TBOX_RECV_MCU_APPLY_FOR, true, 1);	
		}
		else
		{
			MCULOG("Received the file name and len error form mcu!\n");
			pack_mcuUart_upgrade_data(TBOX_RECV_MCU_APPLY_FOR, false, 1);
		}
	}
}


#endif


#if 1
/*****************************************************************************
* Function Name : unpack_MCU_SND_Upgrade_Info
* Description   : get upgrade info
* Input			: unsigned char *pData
*                 unsigned int len
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.05.22
*****************************************************************************/
int mcuUart::unpack_MCU_SND_Upgrade_Info(unsigned char* pData, unsigned int len)
{
	//uint16_t SN;
	uint8_t fileName[50];
	uint8_t length;
	uint8_t *pos = pData;
	
	if (!access(LTE_FILE_NAME, F_OK))
	{
		MCULOG("Exisist tbox upgrade file, remove it!\n");
		system(RM_LTE_FILE);
	}
	
	if(*(pos+8) == 0x00)
	{
		MCULOG("upgrade tbox\n");
	}
	else
		MCULOG("upgrade other\n");

	MCULOG("%02x, %02x, %02x, %02x,\n", *(pos+9),*(pos+10),*(pos+11), *(pos+12));

	dataSize = (*(pos+9)<<24)+(*(pos+10)<<16)+(*(pos+11)<<8)+*(pos+12);
	MCULOG("dataSize:%d\n", dataSize);

	crc32 = (*(pos+13)<<24)+(*(pos+14)<<16)+(*(pos+15)<<8)+*(pos+16);
	MCULOG("crc32:%04x, %d\n", crc32, crc32);

	length = *(pos+17);
	MCULOG("file name len:%d\n", *(pos+17));

	memset(fileName, 0, sizeof(fileName));
	memcpy(fileName, pos+18, length);
	MCULOG("file name:%s\n", fileName);

	pos = pos+18+length;
	
	length = *pos;
	MCULOG("file version len:%d\n", length);

	memset(tboxVersion, 0, sizeof(tboxVersion));
	memcpy(tboxVersion, pos+1, length);
	MCULOG("file version:%s\n", tboxVersion);

	//SN = (pData[4] << 8) + pData[5];
	//MCULOG("serialNumber:%d\n",SN);

	//set the result as successfully
	length = 0x00;
	packProtocolData(TBOX_REPLY_ID, 0, &length, 1, 0); //SN);

	return 0;
}

/*****************************************************************************
* Function Name : unpack_MCU_SND_Upgrade_Data
* Description   : get upgrade data
* Input			: unsigned char *pData
*                 unsigned int len
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.05.22
*****************************************************************************/
int mcuUart::unpack_MCU_SND_Upgrade_Data(unsigned char* pData, unsigned int len)
{
	int ret;
	uint16_t length;
	uint8_t result;
	
	length = (pData[8]<<8)+pData[9];
	
	ret = storageFile(LTE_FILE_NAME, &pData[10], length);

	if(ret == -1)
		result = 0x01;
	else	
		result = 0x00;

	packProtocolData(TBOX_REPLY_ID, 0, &result, 1, 0); //SN);

	return 0;
}

int mcuUart::storageFile(uint8_t *fileName, uint8_t *pData, uint32_t len)
{
	int fd = open(fileName, O_RDWR | O_CREAT | O_APPEND, 0777);
	if (-1 == fd)
	{
		MCULOG("storageFile open file failed !\n");
		return -1;
	}
	if (len != write(fd, pData, len))
	{
		MCULOG("write file error!\n");
		return -1;
	}

	close(fd);

	return 0;
}

/*****************************************************************************
* Function Name : unpack_MCU_SND_Upgrade_CMPL
* Description   : send upgrade completely
* Input			: None
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.05.22
*****************************************************************************/
int mcuUart::unpack_MCU_SND_Upgrade_CMPL()
{
	int ret;
	uint32_t crc;
	uint8_t result = 0;
	
	ret = calculate_files_CRC(LTE_FILE_NAME, &crc);
	if(ret != 0)
	{
		result = 0x01;
		MCULOG("result: %d\n", result);
	}
	else
	{
		if(crc == crc32)
		{
			result = 0x00;
			MCULOG("result: %d\n", result);
		}
		else{		
			result = 0x01;
			MCULOG("result: %d\n", result);
			system(RM_LTE_FILE);
		}
	}

	MCULOG("Upgrade result: %d\n", result);

	packProtocolData(TBox_CHECK_REPORT, 0, &result, 1, 0);
#if 0
	if(result == 0)
	{
		wds_qmi_release();
		nas_qmi_release();
		voice_qmi_release();
		mcm_sms_release();
		mcuUart::m_mcuUart->close_uart();
		LteAtCtrl->~LTEModuleAtCtrl();
		MCULOG("################### exit 2 ###########################\n");
		
		exit(0);
	}
#endif
	return 0;
}


int mcuUart::unpack_MCU_Upgrade_Check(unsigned char* pData, unsigned int len)
{
	int ret;
	uint8_t result;
	result = pData[8];
	if(result == 0)
	{
		wds_qmi_release();
		nas_qmi_release();
		voice_qmi_release();
		mcm_sms_release();
		mcuUart::m_mcuUart->close_uart();
		LteAtCtrl->~LTEModuleAtCtrl();
		MCULOG("################### exit 2 ###########################\n");

		exit(0);
	}
	else
	{
		system(RM_LTE_FILE);
	}
	return 0;
}




/*****************************************************************************
* Function Name : calculate_files_CRC
* Description   : calculat
* Input			: None
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.05.22
*****************************************************************************/
int mcuUart::calculate_files_CRC(char *fileName, uint32_t *crc)
{
	int nRead;
	uint8_t buff[1024];
	int fd = 0;
	int isFirst = 0;

	fd = open(fileName, O_RDONLY);
	if (fd < 0)
	{
		MCULOG("Open file:%s error.\n", fileName);
		return -1;
	}else
		MCULOG("Open file:%s success.\n", fileName);

	memset(buff, 0, sizeof(buff));

	while(1)
	{
		if((nRead = read(fd, buff, sizeof(buff))) > 0)
		{
			if(isFirst == 0)
			{
				*crc = crc32Check(0xFFFFFFFF, buff, nRead);
				isFirst = 1;
			}
			else
			{
				*crc = crc32Check(*crc, buff, nRead);
			}
		}
		else
		{
			isFirst = 0;
			break;
		}
	}

	/*while ((nRead = read(fd, buff, sizeof(buff))) > 0)
	{
		*crc = crc32Check(buff, nRead);
	}*/

	close(fd);

	return 0;
}

static const unsigned int crc32tab[] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL,
	0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
	0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
	0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
	0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
	0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
	0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
	0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
	0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
	0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
	0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
	0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
	0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
	0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
	0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
	0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L,
	0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
	0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
	0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
	0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
	0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
	0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
	0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
	0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L,
	0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L,
	0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
	0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
	0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
	0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
	0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
	0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
	0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
	0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L,
	0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
	0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
	0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
	0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
	0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
	0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
	0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL,
	0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
	0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
	0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
	0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
	0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
	0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
	0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
	0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L,
	0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L,
	0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
	0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
	0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
};

#if 0
unsigned int mcuUart::crc32Check(unsigned char *buff, unsigned int size)
{
    unsigned int i, crc;
    crc = 0xFFFFFFFF;

    for (i = 0; i < size; i++)
        crc = crc32tab[(crc ^ buff[i]) &0xff] ^ (crc >> 8);

    return crc ^ 0xFFFFFFFF;
}
#endif

unsigned int mcuUart::crc32Check(unsigned int crc, unsigned char *buff, unsigned int size)
{
    unsigned int i;

    for (i = 0; i < size; i++)
        crc = crc32tab[(crc ^ buff[i]) &0xff] ^ (crc >> 8);

    return crc;
}

#endif

