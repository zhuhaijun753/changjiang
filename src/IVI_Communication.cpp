#include "IVI_Communication.h"



pthread_mutex_t Ivimutex = PTHREAD_MUTEX_INITIALIZER;
MSG_HEADER_ST msgHeader;
DISPATCHER_MSG_ST dispatcherMsg;
extern uint8_t isUsbConnectionStatus;
extern uint8_t InitStateVoiceCall;//记录当前电话状态

int keepalive = 1;
int keepidle = 5;
int keepinterval = 2;
int keepcount = 1;

//==========================================================================//

int IVI_Communication::cycle_init(CYCLE_BUF_ST *cyc, int size)
{
	cyc->head = NULL;
	cyc->rx_offset = NULL;
	cyc->tx_offset = NULL;
	cyc->buf_size = size;
	cyc->using_size = 0;

	cyc->head = malloc(size);
	memset(cyc->head,0,size);
	if(cyc->head == NULL)
	{
		printf("error :molloc buf fail \n");
		return -1;
	}
	cyc->rx_offset = cyc->tx_offset = cyc->head;
	cyc->last = cyc->head + size - 1;
	return 0;
}

int IVI_Communication::cycle_write_buf(CYCLE_BUF_ST *cyc, uint8_t *buf, int len)
{
	int ret = 0;
	int length = 0;

	if(len > cyc->buf_size)
	{
		printf("error :wirte buf size too big \n");
		return -1;
	}

	if(cyc->using_size == cyc->buf_size)
	{
		printf("waring : write fail buf full \n");
		return 0;
	}

	/*判断是否会有数据的溢出*/
	if((cyc->buf_size - cyc->using_size) < len)
	{
		printf("waring : write buf size over\n");
		/*丢弃tx部分数据不占用rx后的数据*/
		return 0;
		len = cyc->buf_size - cyc->using_size;
	}

	if((cyc->tx_offset + len - 1) <= cyc->last)
	{
		memcpy(cyc->tx_offset, buf, len);
	
		if((cyc->tx_offset + len - 1) == cyc->last)
		cyc->tx_offset = cyc->head;
		else
		cyc->tx_offset = cyc->tx_offset + len;
	}
	else
	{
		int tmp_len = cyc->last - cyc->tx_offset + 1;
		memcpy(cyc->tx_offset,buf,tmp_len);
		memcpy(cyc->head,buf + tmp_len, len - tmp_len);
		cyc->tx_offset = cyc->head + len - tmp_len;
	}
	
	cyc->using_size = len + cyc->using_size;
	return len;
}

int IVI_Communication::cycle_read_buf(CYCLE_BUF_ST *cyc, uint8_t *buf, int len)
{
	int ret = 0;
	
	if(len > cyc->buf_size)
	{
		printf("error read buf size too big \n");
		return -1;
	}
	
	/*没有可用数据*/
	if(cyc->using_size == 0)
	{
		printf("waring :buf is empty \n");
		return 0;
	}
	/*数据不足*/
	if(len > cyc->using_size)
	{
		printf("waring :buf not enough\n");
		len = cyc->using_size;
	}

	if((cyc->rx_offset + len - 1) <= cyc->last)
	{	
		memcpy(buf, cyc->rx_offset, len);
		
		if((cyc->rx_offset + len -1) == cyc->last)
			cyc->rx_offset = cyc->head;
		else
		cyc->rx_offset = cyc->rx_offset + len;
	}
	else
	{
		int tmp_len = cyc->last - cyc->rx_offset + 1;
		memcpy(buf, cyc->rx_offset, tmp_len);
		memcpy(buf + tmp_len, cyc->head, len - tmp_len);
		cyc->rx_offset = cyc->head + len - tmp_len;
	}

	cyc->using_size = cyc->using_size - len;
	return len;
}

//==========================================================================//

/*****************************************************************************
* Function Name : ~IVI_Communication
* Description   : 析构函数
* Input			: None
* Output        : None
* Return        : NULL
* Auther        : ygg
* Date          : 2018.05.21
*****************************************************************************/
IVI_Communication::~IVI_Communication()
{
	close(sockfd);
}

/*****************************************************************************
* Function Name : IVI_Communication
* Description   : 构造函数
* Input			: None
* Output        : None
* Return        : NULL
* Auther        : ygg
* Date          : 2018.05.21
*****************************************************************************/
IVI_Communication::IVI_Communication()
{
    sockfd = -1;
	epoll_fd = -1;
    accept_fd = -1;
	DialCall = 0;
	wifiSetSsid = 0;
	alertingstate = 0;
    HeartBeatTimes = 0;
	voiceCallState = 2;
	cycle_init(&IviReceiveBuff,1024*2);
    memset(incomingPhoneNum, 0, sizeof(incomingPhoneNum));
}

void *IVI_Communication::CheckHeartBeat(void *arg)
{
	static int SpkTimes;
	static int DialTimes = 0;
	uint8_t phoneNum[20] = {0};
	pthread_detach(pthread_self());
	IVI_Communication *p_IVI = (IVI_Communication *)arg;
	while(1)
	{
		//printf("p_IVI->voiceCallState == %d\n", p_IVI->voiceCallState);
		pthread_mutex_lock(&Ivimutex);
		p_IVI->HeartBeatTimes++;
		if(p_IVI->HeartBeatTimes >= 20)
		{
			isUsbConnectionStatus = 0;
		}
		pthread_mutex_unlock(&Ivimutex);
		if(isUsbConnectionStatus == 1 && InitStateVoiceCall == 0)
		{
			if(p_IVI->voiceCallState == 1) //此时通话状态
				p_IVI->TBOX_Call_State_Report(NULL, 0, 3);
			InitStateVoiceCall = 1;//只发第一次连接上的状态
		}
		else if(isUsbConnectionStatus == 0)
		{
			InitStateVoiceCall = 0;
		}
		
		if(p_IVI->alertingstate == 1)
		{
			SpkTimes ++;
		}
		if(SpkTimes >= 8)
		{
			p_IVI->alertingstate = 0;
			SpkTimes = 0;
			LteAtCtrl->switch_stepSPK(1);
		}

		/*DialCall : 1 == 点击拨号  2 == 拨号过程  0 == 点击挂断*/
		if(p_IVI->DialCall == 1)
			DialTimes++;
		else
			DialTimes = 0;

		if(DialTimes >= 6) /*3s after*/
		{
			if(p_IVI->voiceCallState == 2) /*hang up state*/
			{
				dataPool->getPara(B_CALL_ID, (void *)phoneNum, sizeof(phoneNum));
				voiceCall(phoneNum, PHONE_TYPE_B);
			}
			DialTimes = 0;
			p_IVI->DialCall = 2;
		}
		
		usleep(1000*500);
	}
	pthread_exit(0);
}

void *IVI_Communication::ReceiveDataUnpackFunc(void *arg)
{
	int dataLen,templen;
	
	uint8_t FrameData[1024];
	pthread_detach(pthread_self());
	IVI_Communication *p_IVI = (IVI_Communication *)arg;
	while(1)
	{
		while (p_IVI->IviReceiveBuff.using_size > DATAHEADLEN)
		{
			if((p_IVI->IviReceiveBuff.rx_offset[0] == FIR_HEADDATA) && (p_IVI->IviReceiveBuff.rx_offset[1] == SEC_HEADDATA) \
				&& (p_IVI->IviReceiveBuff.rx_offset[2] == THI_HEADDATA)&& (p_IVI->IviReceiveBuff.rx_offset[3] == FOU_HEADDATA))
				{
					dataLen = (p_IVI->IviReceiveBuff.rx_offset[5] << 8) | (p_IVI->IviReceiveBuff.rx_offset[6]);
					if(dataLen <= p_IVI->IviReceiveBuff.using_size - DATAHEADLEN - 1)
					{
						templen =p_IVI->cycle_read_buf(&p_IVI->IviReceiveBuff,FrameData,dataLen + DATAHEADLEN + 1);
						if(p_IVI->checkSum(FrameData, DATAHEADLEN + dataLen + 1))
						{
							IVI_LOG("checkSum return true!\n");
							p_IVI->unpack_Protocol_Analysis(FrameData, DATAHEADLEN + dataLen + 1);
						}
						else
						{
							IVI_LOG("checkSum return failed!\n");
						}
					}
					else
					{
						break;
					}
				}
				else
				{
					p_IVI->cycle_read_buf(&p_IVI->IviReceiveBuff,FrameData,1);
				}
		}
		usleep(200*1000);
	}
	pthread_exit(0);
}

void *IVI_Communication::WifiOpenCloseFunc(void *arg)
{
	char ssid[20] = {0};
	char password[20] = {0};	
	char buff[20];
	int authType = 5;
	int encryptMode = 4;
	pthread_detach(pthread_self());
	IVI_Communication *p_IVI = (IVI_Communication *)arg;
	while(1)
	{
		if(tboxInfo.operateionStatus.isGoToSleep == 0)
		{
			if(!access("/sys/class/net/wlan0", F_OK))//判断文件存在
			{
				wifi_OpenOrClose(0);
				wifi_led_off(1);
			}

			sleep(1);
			continue;
		}
//===============================================================//

	//if(tboxInfo.operateionStatus.wifiStartStatus == 0)
	{
		wifi_connected_dataExchange();
	}

//================================================================//


		if(tboxInfo.operateionStatus.wifiStartStatus == -1)
		{
       	 	if(!access("/sys/class/net/wlan0", F_OK))//判断文件存在
       	 	{
				wifi_OpenOrClose(0);
			}
		}
		else
		{
			if(!access("/sys/class/net/wlan0", F_OK))//判断文件存在
			{
				//if(p_IVI->wifiSetSsid == 1)
				{
					memset(ssid, 0, sizeof(ssid));
					dataPool->getPara(wifi_APMode_Name_Id, ssid, sizeof(ssid));
					memset(buff, 0, sizeof(buff));
					wifi_get_ap_ssid(buff, AP_INDEX_STA);
					if(strcmp(buff, ssid) != 0 && ssid[0] != 0)
					{
						wifi_set_ap_ssid(ssid, AP_INDEX_STA);
					}
					
					memset(buff, 0, sizeof(buff));
					wifi_get_ap_auth(&authType, &encryptMode, buff, AP_INDEX_STA);
					memset(password, 0, sizeof(password));
					dataPool->getPara(wifiAPModePassswordId, password, sizeof(password));
					if(strcmp(buff, password) != 0 && password[0] != 0)
					{
					  wifi_set_ap_auth(authType, encryptMode, password, AP_INDEX_STA);
					}
//					p_IVI->wifiSetSsid = 0;
				}
			}
			else
			{
				wifi_OpenOrClose(1);
			}
		}
		usleep(1000*1000);
	}
	pthread_exit(0);
}

/*****************************************************************************
* Function Name : IVI_Communication_Init
* Description   : 初始化IVI_Communication
* Input			: None
* Output        : None
* Return        : NULL
* Auther        : ygg
* Date          : 2018.05.21
*****************************************************************************/
int IVI_Communication::IVI_Communication_Init()
{
	struct epoll_event events[MAX_EVENT_NUMBER];
	static int isflagThreadHeart = 0;
	static int isflagDataUnpack = 0;	
	static int isflagWifiOpenClose = 0;
    if( (isflagThreadHeart == 0) && (pthread_create(&CheckHeartBeatID, NULL, CheckHeartBeat, this) != 0) )
	{
		isflagThreadHeart = 1;
		IVI_LOG("Cannot creat receiveThread:%s\n", strerror(errno));
	}
	if( (isflagDataUnpack == 0) && (pthread_create(&ReceiveDataUnpackId, NULL, ReceiveDataUnpackFunc, this) != 0) )
	{
		isflagDataUnpack = 1;
		IVI_LOG("Cannot creat ReceiveDataUnpackFunc:%s\n", strerror(errno));
	}
	 
	if((isflagWifiOpenClose == 0) && (pthread_create(&WifiId, NULL, WifiOpenCloseFunc, this) != 0))
	{
		isflagWifiOpenClose = 1;
	}
    while(1)
	{
	    if(socketConnect() == 0)
	    {
		    epoll_fd = epoll_create(10);
			if(epoll_fd == -1)
			{
		        IVI_LOG("fail to create epoll!\n");
				close(epoll_fd);
		        close(sockfd);
		    }
			else
			{
		    	break;
		    }
	    }
	    sleep(1);
    }

    add_socketFd(epoll_fd, sockfd);

	//system("tcpdump -i bridge0 src or dst 192.168.100.1 and port 20000 -w /data/TboxNetData.cap");

	while(1)
	{
		int num = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
		if(num < 0)
		{
			IVI_LOG("epoll failure!\n");
			break;
		}
		et_process(events, num, epoll_fd, sockfd);
	}	
	close(events[0].data.fd);
	close(epoll_fd);
    close(sockfd);
	
	return 0;
}

int IVI_Communication::set_non_block(int fd)
{
    int old_flag = fcntl(fd, F_GETFL);
    if(old_flag < 0)
    {
        perror("fcntl");
        return -1;
    }
    if(fcntl(fd, F_SETFL, old_flag | O_NONBLOCK) < 0)
    {
        perror("fcntl");
        return -1;
    }

    return 0;
}

void IVI_Communication::add_socketFd(int epoll_fd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	//event.events = EPOLLIN;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    if(set_non_block(fd) == 0)
    	IVI_LOG("set_non_block ok!\n");
}

int IVI_Communication::socketConnect()
{
    int opt = 1;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        IVI_ERROR("socket error!");
        return -1;
    }
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(IVI_SERVER);
    serverAddr.sin_port = htons(IVI_PORT);

    if(bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        IVI_ERROR("bind failed.\n");
        close(sockfd);
        return -1;
    }

    if(listen(sockfd, LISTEN_BACKLOG) == -1)
    {
        IVI_ERROR("listen failed.\n");
        close(sockfd);
        return -1;
    }

    return 0;
}

void IVI_Communication::et_process(struct epoll_event *events, int number, int epoll_fd, int socketfd)
{
    int i;
    int dataLen = 0;
    uint8_t buff[BUFFER_SIZE] = {0};
    
    for(i = 0; i < number; i++)
    {
        if(events[i].data.fd == socketfd)
        {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            int connfd = accept(socketfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
            add_socketFd(epoll_fd, connfd);

			setsockopt(connfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
			setsockopt(connfd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&keepidle, sizeof(keepidle));
			setsockopt(connfd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepinterval, sizeof(keepinterval));
			setsockopt(connfd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepcount, sizeof(keepcount));

        }
        else if(events[i].events & EPOLLIN)//接收到数据,读socket
        {
            IVI_LOG("et mode: event trigger once!\n");
			bool rs = true;
			while(rs){
				memset(buff, 0, BUFFER_SIZE);
				dataLen = recv(events[i].data.fd, buff, BUFFER_SIZE, 0);
				accept_fd = events[i].data.fd;
				IVI_LOG("==== dataLen == %d ====\n");
				for(int i = 0; i< dataLen; i++)
					IVI("%02x ", buff[i]);
				IVI_LOG("\n");

				if(dataLen <= 0)
				{
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
					close(events[i].data.fd);
					events[i].data.fd = -1;
					return -1;
				}

				AcquireData(buff, dataLen);
				if(dataLen == BUFFER_SIZE){
					continue;
				}else
					break;
			}
        }
        else
        {
            IVI_LOG("something unexpected happened!\n");
        }
    }
}

void IVI_Communication::AcquireData(unsigned char *Data, int DataLen)
{
	unsigned char *pos = NULL;
	int times = 0;
	int datalen = 0;
	int i = 0;
	#if 0
	//更新缓冲区数据
	memcpy(&RevcBuff[RevcBuffLen], Data, DataLen);
	RevcBuffLen += DataLen;
	pos = &RevcBuff[0];
	
	while(RevcBuffLen > DATAHEADLEN){
		while(RevcBuffLen - times > DATAHEADLEN){
			if((pos[0] == FIR_HEADDATA) && (pos[1] == SEC_HEADDATA) && (pos[2] == THI_HEADDATA) && (pos[3] == FOU_HEADDATA)){
				break;
			}else{
				times++;
				pos++;
			}
		}

		RevcBuffLen -= times;
		datalen = ((pos[5] << 8) | pos[6]);
		if(datalen <= RevcBuffLen - DATAHEADLEN - 1){
			if(checkSum(pos, DATAHEADLEN + datalen + 1)){
				IVI_LOG("checkSum return true!\n");
				unpack_Protocol_Analysis(pos, DATAHEADLEN + datalen + 1);
			}else
				IVI_LOG("checkSum return failed!\n");
			pos += (DATAHEADLEN + datalen + 1);
			RevcBuffLen -= (DATAHEADLEN + datalen + 1);
			times = 0;
			datalen = 0;
			i++;
			if(i == 5)
				break;
			continue;
		}else{
			break;
		}
	}
	//取出数据完毕,更新缓冲区
	memset(RevcBuff, 0, sizeof(RevcBuff));
	RevcBuffLen = 0;
	//memcpy(RevcBuff, pos, RevcBuffLen);
	#else
	cycle_write_buf(&IviReceiveBuff,Data,DataLen);
	#endif
	return 0;
}

#if 0
uint16_t IVI_Communication::received_and_check_data(int fd, uint8_t *buff, int size)
{
	uint16_t len;
	uint8_t data;
	uint8_t state = 0;
	uint8_t *pos = buff;

	bool start = true;
	static int isfirstFrame = 0;

	while(start)
	{
		if(recv(fd, &data, 1, 0) > 0)
		{
			//IVI("buff:%p, pos:%p, len:%d, isfirstFrame: %d, data:%02x \n",buff, pos, pos-buff, isfirstFrame, data);
			//IVI("%02x ", data);
			if(isfirstFrame == 0)
			{
				switch (state)
				{
					case 0:
						if(data == 0x46)
						{
							pos = buff;
							*pos = data;
							state = 1;
						}
						break;
					case 1:
						if(data == 0x4c)
						{
							*++pos = data;
							state = 2;
						}
						break;
					case 2:
						if(data == 0x43)
						{
							*++pos = data;
							state = 3;
						}
						break;
					case 3:
						if(data == 0x46)
						{
							isfirstFrame = 1;
						}
						*++pos = data;
						
						break;
					default:
						state = 0;
						break;
						
				}
			}
			else if(isfirstFrame == 1) //check the data is the second frame
			{
				if(data == 0x4c)
				{
					start = false;
					isfirstFrame = 2;
					break;
				}
				else
				{
					isfirstFrame = 0;
					state = 3;
					*++pos = data;
				}
			}
			else if(isfirstFrame == 2)
			{
				pos = buff;
				*pos++ = 0x46;
				*pos = 0x4c;
				state = 2;
				isfirstFrame = 0;
			}
		}
	}

	if(start == false)
		len = pos-buff;
	
	IVI("\n@@@@@@@@@@@@@@@@@@@@@@@@@ received frame data, data len:%d\n", len);
	for(int i=0; i<len; i++)
		IVI("%02x ", *(buff+i));
	IVI("\n\n");
	
	return len;
}
#endif

uint8_t IVI_Communication::checkSum(uint8_t *pData, int len)
{
	int i;
	uint8_t *pos = pData;
	uint8_t sum = 0x00;

	for(i = 0; i<len-1; i++)
	{
        sum ^= *(pos+i);
	}

	IVI_LOG("check sum = %02x", sum);

	if(sum == *(pos+len-1))
	{
		IVI_LOG("check sum ok!\n");
		return 1;
	}else{
		IVI_LOG("check sum error!\n");
	}

	return 0;
}

uint8_t IVI_Communication::checkSum_BCC(uint8_t *pData, uint16_t len)
{
	uint8_t *pos = pData;
	uint8_t checkSum = *pos++;
	
	for(uint16_t i = 0; i<(len-1); i++)
	{
		checkSum ^= *pos++;
	}

	return checkSum;
}

uint8_t IVI_Communication::unpack_Protocol_Analysis(uint8_t *pData, int len)
{
	int dataLen;
	uint16_t DispatcherMsgLen;
	uint8_t *pos = pData;
	uint8_t IVISerialNumber[30] = {0};

	if((pData[0] == 0x46) && (pData[1] == 0x4C) && (pData[2] == 0x43) &&(pData[3] == 0x2E))
	{
		IVI_LOG("Msg header check ok!\n");
	}else{
		return 1;
	}

	dataLen = (*(pos+5)<<8) + *(pos+6);
	//IVI_LOG("msgHeader.MsgSize = %d\n", dataLen);

/*	if(dataLen != (len-11))
	{
		IVI_LOG("Received Msg data error!\n");
		return 1;
	}
*/	
	//dataLen = (pData[5]<<8) + pData[6];
	//IVI_LOG("MsgSize:%d\n", dataLen);

	DispatcherMsgLen = (*(pos+7)<<8) + *(pos+8);
	//IVI_LOG("DispatcherMsgLen = %d\n", DispatcherMsgLen);

	memset(&msgHeader, 0, sizeof(msgHeader));
	memset(&dispatcherMsg, 0, sizeof(dispatcherMsg));

	memcpy(&msgHeader.MessageHeaderID, pos, 4);
	msgHeader.TestFlag = *(pos+4);
	msgHeader.MsgSize = dataLen;
	msgHeader.DispatcherMsgLen = DispatcherMsgLen;
	msgHeader.MsgSecurityVer = *(pos+9);

	/* save for TBOX send */
	MsgSecurityVerion = *(pos+9);

	dispatcherMsg.EventCreationTime = (*(pos+10)<<24)+(*(pos+11)<<16)+(*(pos+12)<<8)+(*(pos+13)<<0);
	//IVI_LOG("dispatcherMsg.EventCreationTime: %d\n", dispatcherMsg.EventCreationTime);
	dispatcherMsg.EventID = *(pos+14);
	//IVI_LOG("dispatcherMsg.EventID: %d\n", dispatcherMsg.EventID);
	dispatcherMsg.ApplicationID = (*(pos+15)<<8)+(*(pos+16)<<0);
	//IVI_LOG("dispatcherMsg.ApplicationID: 0x%04x\n", dispatcherMsg.ApplicationID);
	dispatcherMsg.MessageID = (*(pos+17)<<8)+(*(pos+18)<<0);
	//IVI_LOG("dispatcherMsg.MessageID: %d\n", dispatcherMsg.MessageID);
	dispatcherMsg.MsgCounter.uplinkCounter = *(pos+19);
	//IVI_LOG("uplinkCounter: %d\n", dispatcherMsg.MsgCounter.uplinkCounter);
	dispatcherMsg.MsgCounter.downlinkCounter = *(pos+20);
	//IVI_LOG("downlinkCounter: %d\n", dispatcherMsg.MsgCounter.downlinkCounter);
	dispatcherMsg.ApplicationDataLength = (*(pos+21)<<8)+*(pos+22);
	//IVI_LOG("dispatcherMsg.ApplicationDataLength: %d\n", dispatcherMsg.ApplicationDataLength);
	dispatcherMsg.Result = (*(pos+23)<<8)+*(pos+24);
	//IVI_LOG("dispatcherMsg.Result: %d\n", dispatcherMsg.Result);

	switch (dispatcherMsg.ApplicationID)
	{
		case CallStateReport:	//电话状态汇报
			TBOX_Call_State_Report(pos+25, dispatcherMsg.ApplicationDataLength, dispatcherMsg.MessageID);
			break;
		case NetworkStateQuery:
			TBOX_Network_State_Query();
			break;
		case VehicleVINCodeQuery:
			TBOX_Vehicle_VINCode_Query();
			break;
		case TelephoneNumQuery:
			TBOX_Telephone_Num_Query();
			break;
		case EcallStateReport:
			TBOX_Ecall_State_Report();
			break;
		case TBOXInfoQuery:
			TBOX_General_Info_Query();
			break;
		case WIFIInfoQuery:
			TBOX_WIFI_Info_Query(pos+25, dispatcherMsg.ApplicationDataLength, dispatcherMsg.MessageID);
			break;
		case IVISerialNumberQuery:
			if(dispatcherMsg.ApplicationDataLength >= 30)
				memcpy(IVISerialNumber, pos+25, 30);
			else{
				memset(IVISerialNumber, 0, sizeof(IVISerialNumber));
				memcpy(IVISerialNumber, pos+25, dispatcherMsg.ApplicationDataLength);
			}
			if(memcmp(TBoxPara.dataPara.IVISerialNumber, IVISerialNumber, sizeof(IVISerialNumber)) != 0)
				dataPool->setPara(IVISerialNumber_ID, IVISerialNumber, sizeof(IVISerialNumber));
		break;
		default:
			IVI_LOG("ApplicationID error!\n");
			break;		
	}

	return 0;
}

uint8_t IVI_Communication::TBOX_Call_State_Report(uint8_t *pData, int len, uint16_t msgType)
{
	int i;
	int phoneNumLen;
	int state = 1;
	uint8_t *pos = NULL;
	char phoneNum[20];

	pos = pData;
	memset(phoneNum, 0, sizeof(phoneNum));
	if(msgType == TBOX_CallCommandReq)
	{
		if(pData[1] > 0)
		{
			memcpy(phoneNum, pos+1, pData[1]);
			IVI_LOG("phoneNum: %s\n", phoneNum);
		}
		
		switch (pData[0])
		{
			case 0: //拨打电话
				/**
				 * Modify by the customer request
				 * when call send the state as 3,
				 * show the calling connecting.
				 */
//				 if(voiceCallState != 2)
//					 return -1;
				DialCall = 1;
                pack_TBOX_Data_Report(TEST_FLAG, 3, 3);/*上报IVI，电话连接中*/

				if(CFAWACP::cfawacp->m_loginState == 2){
					CFAWACP::cfawacp->timingReportingData(MESSAGE_TYPE_START + 2, ACPApp_EmergencyDataID);
				}
				break;
			case 1: //接听电话
				state = hangUp_answer(VOICE_CALL_ANSWER, callID);
				break;
			case 2: //挂断电话
				DialCall = 0;
				if(voiceCallState != 2)
					state = hangUp_answer(VOICE_CALL_HANDUP, callID);
				else
					pack_TBOX_Data_Report(TEST_FLAG, 3, 2);
				break;
			default:
				IVI_LOG("Call state error!\n");
				break;
		}
        /**
         * Modify by the customer request
         * when call state is -1, show the calling error, then
         * send hang up telephone.
         */
        
		if(state == -1)		//回应挂断电话信息
		{
            pack_TBOX_Data_Report(TEST_FLAG, 3, 2);
		}
		else				//回应动作ACK消息 STATE：1:SUCCESS 0:DEFEAT
		{
            pack_hande_data_and_send(2, state);
		}
	}
	else if(msgType == TBOX_CallStateReport)
	{	/*当前电话状态上报voiceCallState：0:来电 1:电话接通 2:电话挂断 3:电话连接中*/
		pack_TBOX_Data_Report(TEST_FLAG, 3, 0);
	}
	return 0;
}

uint8_t IVI_Communication::TBOX_Network_State_Query()
{
	pthread_mutex_lock(&Ivimutex);
	HeartBeatTimes = 0;
	if(isUsbConnectionStatus != 1)
		isUsbConnectionStatus = 1;
	pthread_mutex_unlock(&Ivimutex);

	pack_hande_data_and_send(2, 0);

	return 0;
}

uint8_t IVI_Communication::TBOX_Vehicle_VINCode_Query()
{
	pack_hande_data_and_send(2, 0);

	return 0;
}

uint8_t IVI_Communication::TBOX_Telephone_Num_Query()
{
	pack_hande_data_and_send(2, 0);

	return 0;
}

uint8_t IVI_Communication::TBOX_Ecall_State_Report()
{
	pack_hande_data_and_send(2, 0);

	return 0;
}

uint8_t IVI_Communication::TBOX_General_Info_Query()
{
	pack_hande_data_and_send(2, 0);

	return 0;
}

uint8_t IVI_Communication::TBOX_WIFI_Info_Query(uint8_t *pData, int len, uint16_t msgType)
{
	int i;
	int ret;
	uint8_t temp = 1;
	int authType = 5;
	int encryptMode = 4;
	uint8_t *pos = pData;
	char ssid[20] = {0};
	char password[20] = {0};	
	static struct timespec timeout_val = {0, 0};
    struct timespec temptime_val = {0, 0};
	if(msgType == GET_TBOX_WIFI) /* 500 ms 查询*/
	{
		clock_gettime(CLOCK_MONOTONIC, &temptime_val);
		if(temptime_val.tv_sec > (timeout_val.tv_sec +2))
	    {
			pack_hande_data_and_send(msgType, 0);
	    }
	}
	else if(msgType == SET_TBOX_WIFI)
	{
		clock_gettime(CLOCK_MONOTONIC, &timeout_val);

		if(pData[0] == 0)
		{
			tboxInfo.operateionStatus.wifiStartStatus = -1;
			wifiSetSsid =0;
		}
		else if(pData[0] == 1)
		{
			if(tboxInfo.operateionStatus.wifiStartStatus == 0)
				wifiSetSsid = 1;
			else
				tboxInfo.operateionStatus.wifiStartStatus = 0;
		}
	
		if(wifiSetSsid == 1)
		{
			pos = pos+1;		
			//set wifi ssid
			for(i = 0; i<15; i++)
			{
				if(*pos == 0x00)
					break;
				ssid[i] = *pos++;
			}
			ret = strlen(ssid);

			if(ssid[0] != 0)
				dataPool->setPara(wifi_APMode_Name_Id, ssid, sizeof(ssid));
			pos += (15-ret);

			//set wifi password
			for(i = 0; i<16; i++)
			{
				if(*pos == 0x00)
					break;
				password[i] = *pos++;
			}
			IVI_LOG("Set wifi password:%s\n", password);
			ret = strlen(password);

	         if(password[0] != 0)
				dataPool->setPara(wifiAPModePassswordId, password, sizeof(password));

			dataPool->setPara(WifiModification_Id, &temp, sizeof(temp));
		}
	}
	return 0;
}

uint8_t IVI_Communication::pack_TBOX_Data_Report(uint8_t TestFlag, uint16_t MsgID, uint8_t callState)
{
	int i;
	uint32_t seconds;
	uint16_t dataLen;
	int length;
	static uint8_t MsgCount = 0;
	static uint8_t EventID = 0;
	
	DISPATCHER_MSG_ST dispatcher_msg;
	memset(&dispatcher_msg, 0, sizeof(dispatcher_msg));

	uint8_t *dataBuff = (uint8_t *)malloc(BUFFER_SIZE);
	if(dataBuff == NULL)
	{
		IVI_LOG("malloc dataBuff error!");
		return 1;
	}
	memset(dataBuff, 0, BUFFER_SIZE);

	seconds = Get_Seconds_from_1970();
	dispatcher_msg.EventCreationTime = seconds;

	if(TestFlag == 1){
		dispatcher_msg.EventID = 0x00;
	}else{
		dispatcher_msg.EventID = EventID;
		EventID++;
		if(EventID == 64)
			EventID = 0;
	}
		
	dispatcher_msg.ApplicationID = CallStateReport;
	dispatcher_msg.MessageID = MsgID;
	dispatcher_msg.MsgCounter.uplinkCounter = 0x00;
	dispatcher_msg.MsgCounter.downlinkCounter = MsgCount;
	
	MsgCount++;
	if(MsgCount == 255)
		MsgCount = 0;
		
	dispatcher_msg.ApplicationDataLength = 0x00; // = dispatcher_msg.ApplicationDataLength;
	dispatcher_msg.Result = 0x00;
	
	//IVI_LOG("111 dispatcher_msg.ApplicationDataLength:%d\n", dispatcher_msg.ApplicationDataLength);
	
	dataLen = pack_Protocol_Data(dataBuff, BUFFER_SIZE, &dispatcher_msg, callState);
	//IVI_LOG("After dispatcher_msg.ApplicationDataLength:%d\n", dispatcher_msg.ApplicationDataLength);
		
	if((length = send(accept_fd, dataBuff, dataLen, 0)) < 0)
	{
		close(accept_fd);
		accept_fd = -1;
	}
	else
	{
		IVI_LOG("Send data ok,length:%d\n", length);
	}

	if(dataBuff != NULL)
	{
		free(dataBuff);
		dataBuff = NULL;
	}
	
	return 0;
}

uint8_t IVI_Communication::pack_hande_data_and_send(uint16_t MsgID, uint8_t state)
{
	uint16_t dataLen;
	int length;

	uint8_t *dataBuff = (uint8_t *)malloc(BUFFER_SIZE);
	if(dataBuff == NULL)
	{
		IVI_LOG("malloc dataBuff error!");
		return 1;
	}
	memset(dataBuff, 0, BUFFER_SIZE);

	dispatcherMsg.MessageID = MsgID;
	
	dataLen = pack_Protocol_Data(dataBuff, BUFFER_SIZE, &dispatcherMsg, state);
	
	if((length = send(accept_fd, dataBuff, dataLen, 0)) < 0)
	{
		close(accept_fd);
		accept_fd = -1;
	}
	else
	{
		IVI_LOG("Send data ok,length:%d\n", length);
	}

	if(dataBuff != NULL)
	{
		free(dataBuff);
		dataBuff = NULL;
	}
	
	return 0;
}

uint16_t IVI_Communication::pack_Protocol_Data(uint8_t *pData, int len, DISPATCHER_MSG_ST *dispatchMsg, uint8_t state)
{
	int i, ret;
	uint16_t dataLen;
	uint16_t totalLen;
	char buff[50];
	int get_authType, get_encryptMode;
    int BCallLen;
	char BCall[20] = {0};

	//For machine telephone
	char phoneNumber[] = "13800000000";

	uint8_t *pos = pData;
	uint8_t *ptr = NULL;

	*pos++ = (MESSAGE_HEADER_ID>>24) & 0xFF;
	*pos++ = (MESSAGE_HEADER_ID>>16) & 0xFF;
	*pos++ = (MESSAGE_HEADER_ID>>8)  & 0xFF;
	*pos++ = (MESSAGE_HEADER_ID>>0)  & 0xFF;

	if((dispatchMsg->ApplicationID == CallStateReport) && (dispatchMsg->MessageID == 0x03)){
		*pos++ = TEST_FLAG;
	}else{
		*pos++ = msgHeader.TestFlag;
	}

	//Msg size;
	*pos++ = 0x00;
	*pos++ = 0x00;

	//dispather length
	*pos++ = 0x00;
	*pos++ = 0x00;

	//security version
	if((dispatchMsg->ApplicationID == CallStateReport) &&(dispatchMsg->MessageID == 0x03)){
		*pos++ = MsgSecurityVerion;
	}else{
		*pos++ = msgHeader.MsgSecurityVer;
	}
	
	/* dispather Msg */
	//Event time
	*pos++ = (dispatchMsg->EventCreationTime>>24) & 0xFF;
	*pos++ = (dispatchMsg->EventCreationTime>>16) & 0xFF;
	*pos++ = (dispatchMsg->EventCreationTime>>8)  & 0xFF;
	*pos++ = (dispatchMsg->EventCreationTime>>0)  & 0xFF;

	//event id
	*pos++ = dispatchMsg->EventID;
	
	//application id
	*pos++ = (dispatchMsg->ApplicationID>>8) & 0xFF;
	*pos++ = (dispatchMsg->ApplicationID>>0) & 0xFF;
	
	//message id
	*pos++ = (dispatchMsg->MessageID>>8) & 0xFF;
	*pos++ = (dispatchMsg->MessageID>>0) & 0xFF;

	//message counter
	*pos++ = dispatchMsg->MsgCounter.uplinkCounter;
	*pos++ = dispatchMsg->MsgCounter.downlinkCounter;

	//application data len
	*pos++ = 0x00; //(dispatchMsg.ApplicationDataLength>>8) & 0xFF;
	*pos++ = 0x00; //(dispatchMsg.ApplicationDataLength>>0) & 0xFF;

	//Result
	*pos++ = (dispatchMsg->Result>>8) & 0xFF;
	*pos++ = (dispatchMsg->Result>>0) & 0xFF;

	//calculation dispather Msg length and fill the length
	dataLen = pos-pData-10;
	IVI_LOG("dispather Msg length: %d\n", dataLen);
	
	pData[7] = (dataLen>>8) & 0xFF;
	pData[8] = (dataLen>>0) & 0xFF;
	/* dispather Msg end*/

	//For calculation MSG size
	ptr = pos;

	//application data
	switch (dispatchMsg->ApplicationID)
	{
		case CallStateReport:
			IVI_LOG("  11111111111111111111111111111111111111111\n");

			if(dispatchMsg->MessageID == 0x02)
			{
				*pos++ = state;
			}
			else if(dispatchMsg->MessageID == 0x03)
			{
                if(state == 0)
                {
					//voicall state
					*pos++ = voiceCallState;
					//printf("\n voiceCallState:%d\n",voiceCallState);
					ret = strlen(incomingPhoneNum);
					IVI_LOG("incomingPhoneNum length: %d\n", ret);
					
					//phone length
					*pos++ = ret;
					
					if(ret > 0)
					{
						memcpy(pos, incomingPhoneNum, ret);
						pos += ret;

						//the remaining byte fill in 0x00
						ret = 18 - ret;
						IVI_LOG("remaining length: %d\n", ret);

						for(i=0; i<ret; i++)
							*pos++ = 0x00;
					}
					else
					{
						for(i=0; i<18; i++)
						{
							*pos++ = 0x00;
						}
					}
				}
    			else if(state == 3)
    			{
                    *pos++ = state;
                    dataPool->getPara(B_CALL_ID, (void *)BCall, sizeof(BCall));
                    BCallLen = strlen(BCall);
                    *pos++ = BCallLen;

                    memcpy(pos, BCall, BCallLen);
                    pos += BCallLen;

                    //the remaining byte fill in 0x00
                    BCallLen = 18 - BCallLen;
                    IVI_LOG("remaining length: %d\n", BCallLen);

                    for(i=0; i<BCallLen; i++)
                        *pos++ = 0x00;
    			}
                else if(state == 2)
                {
                    *pos++ = state;
                    for(i=0; i<19; i++)
                        *pos++ = 0x00;
                }
			}

			ret = pos - ptr;
			dispatchMsg->ApplicationDataLength = ret;
			IVI_LOG("dispatchMsg->ApplicationDataLength:%d, ret:%d\n",dispatchMsg->ApplicationDataLength, ret);
			
			break;
		case NetworkStateQuery:
			IVI_LOG("  22222222222222222222222222222222222222222\n");
			*pos++ = tboxInfo.networkStatus.networkRegSts;
			*pos++ = tboxInfo.networkStatus.signalStrength;

			//IVI_LOG("  tboxInfo.networkStatus.signalStrength  :%d\n", tboxInfo.networkStatus.signalStrength);
		    syslog(LOG_DEBUG, "111111111  tboxInfo.networkStatus.signalStrength  :%d\n", tboxInfo.networkStatus.signalStrength);

			ret = pos - ptr;
			dispatchMsg->ApplicationDataLength = ret;
			IVI_LOG("dispatchMsg->ApplicationDataLength:%d, ret:%d\n",dispatchMsg->ApplicationDataLength, ret);
			
			break;
		case VehicleVINCodeQuery:
			IVI_LOG("  333333333333333333333333333333333333333333333\n");
			memset(buff, 0, sizeof(buff));
			ret = dataPool->getTboxConfigInfo(VinID, buff, sizeof(buff));
			IVI_LOG("vin:%s, vin len:%d, ret:%d\n", buff, strlen(buff), ret);

			ret = strlen(buff);
			if(ret > 0)
			{
				memcpy(pos, buff, ret);
				pos += ret;
			}
			else
			{
				for(i=0; i<17; i++)
				{
					*pos++ = 0xFF;
				}
			}

			ret = pos - ptr;
			dispatchMsg->ApplicationDataLength = ret;
			IVI_LOG("dispatchMsg->ApplicationDataLength:%d, ret:%d\n",dispatchMsg->ApplicationDataLength, ret);
			
			break;
		case TelephoneNumQuery:
			IVI_LOG("  44444444444444444444444444444444444444444444\n");
			dataLen = strlen(phoneNumber);
			IVI_LOG("phoneNumber length:%d\n", dataLen);
			
			memcpy(pos, phoneNumber, dataLen);
			pos += dataLen;

			ret = pos - ptr;

			dispatchMsg->ApplicationDataLength = dataLen;
			IVI_LOG("dispatchMsg->ApplicationDataLength:%d\n", dispatchMsg->ApplicationDataLength);

			dispatchMsg->ApplicationDataLength = ret;
			IVI_LOG("dispatchMsg->ApplicationDataLength:%d, ret:%d\n",dispatchMsg->ApplicationDataLength, ret);
			break;
		case EcallStateReport:
			IVI_LOG("  5555555555555555555555555555555555555555555555\n");
			if(tboxInfo.operateionStatus.phoneType == 1)
			{
				*pos++ = 0x01;
			}
			else
			{
				*pos++ = 0x00;
			}

			memset(buff, 0, sizeof(buff));
			dataPool->getPara(E_CALL_ID, (void *)buff, sizeof(buff));
			IVI_LOG("E_CALL telephone number: %s\n", buff);
			
			ret = strlen(buff);
			memcpy(pos, buff, ret);
			pos += ret;
			
			//the remaining byte fill in 0x00
			ret = 18 - ret;
			IVI_LOG("length: %d\n", ret);
			
			for(i=0; i<ret; i++)
				*pos++ = 0x00;
			
			ret = pos - ptr;
			dispatchMsg->ApplicationDataLength = ret;
			IVI_LOG("dispatchMsg->ApplicationDataLength:%d, ret:%d\n",dispatchMsg->ApplicationDataLength, ret);
			
			break;
		case TBOXInfoQuery:
			IVI_LOG("  6666666666666666666666666666666666666666666666\n");
			memset(buff, 0, sizeof(buff));
			if(!dataPool->getPara(SIM_ICCID_INFO, (void *)buff, sizeof(buff)))
			{
				IVI_LOG("ICCID:%s\n", buff);
			}

			ret = strlen(buff);
			memcpy(pos, buff, ret);
			pos += ret;
			IVI_LOG("ICCID len:%d\n", ret);

			dispatchMsg->ApplicationDataLength = ret;
			
			memset(buff, 0, sizeof(buff));
			if(!dataPool->getPara(CIMI_INFO, (void *)buff, sizeof(buff)))
			{
				IVI_LOG("IMSI:%s\n", buff);
			}

			ret = strlen(buff);
			memcpy(pos, buff, ret);
			pos += ret;
			IVI_LOG("IMSI len:%d\n", ret);
			
			dispatchMsg->ApplicationDataLength += ret;
			IVI_LOG("111111  dispatchMsg->ApplicationDataLength:%d\n", dispatchMsg->ApplicationDataLength);

			ret = pos - ptr;
			dispatchMsg->ApplicationDataLength = ret;
			IVI_LOG("dispatchMsg->ApplicationDataLength:%d, ret:%d\n",dispatchMsg->ApplicationDataLength, ret);
			
			break;
		case WIFIInfoQuery:
			IVI_LOG("  77777777777777777777777777777777777 \n");
			if(dispatchMsg->MessageID == GET_TBOX_WIFI)
			{
				printf("  88888888888888888888  %d \n", tboxInfo.operateionStatus.wifiStartStatus);

				if(tboxInfo.operateionStatus.wifiStartStatus == -1)
				{
					*pos++ = 0x00;
					 for(i=0; i<31; i++)
							*pos++ = 0x00; 
					 printf("\r\n===tboxInfo.operateionStatus.wifiStartStatus == -1===!!\r\n");
				}
				else if(tboxInfo.operateionStatus.wifiStartStatus == 0)
				{
					*pos++ = 0x01;
					memset(buff, 0, sizeof(buff));
//					ret = wifi_get_ap_ssid(buff, AP_INDEX_STA);
					dataPool->getPara(wifi_APMode_Name_Id, buff, sizeof(buff));
					printf("Get wifi ssid:%s\n", buff);
#if 1
					for(i=0; i<15; i++)
					{
						*pos++ = buff[i];
					}
#else
					ret = strlen(buff);
					printf("ssid len:%d\n", ret);
					memcpy(pos, buff, ret);
					pos += ret;

					if(ret < 15)
					{
						for(i=0; i<(15-ret); i++)
							*pos++ = 0x00; 
					}
#endif
					memset(buff, 0, sizeof(buff));
//					ret = wifi_get_ap_auth(&get_authType, &get_encryptMode, buff, AP_INDEX_STA);
					dataPool->getPara(wifiAPModePassswordId, buff, sizeof(buff));
					printf("\r\nGet wifi password:%s\r\n", buff);
#if 1
					for(i=0; i<16; i++)
					{
						*pos++ = buff[i];
					}
#else
					ret = strlen(buff);
					printf("password len:%d\n", ret);
					memcpy(pos, buff, ret);
					pos += ret;

					if(ret < 16)
					{
						for(i=0; i<(16-ret); i++)
							*pos++ = 0x00; 
					}
#endif
				}
				ret = pos - ptr;
				dispatchMsg->ApplicationDataLength = ret;
				IVI_LOG("ApplicationDataLength :%d\n", ret);
			}
		
			break;	
	}

	pData[21] = (dispatchMsg->ApplicationDataLength>>8) & 0xFF;
	pData[22] = (dispatchMsg->ApplicationDataLength>>0) & 0xFF;
	IVI_LOG("pData[21]: %02x, pData[22]: %02x\n", pData[21], pData[22]);

	//fill in Msg size
	dataLen = pos-pData-10;
	IVI_LOG("Msg size: %d\n", dataLen);

	pData[5] = (dataLen>>8) & 0xFF;
	pData[6] = (dataLen>>0) & 0xFF;

	dataLen = pos-pData;
	IVI_LOG("data Len: %d\n", dataLen);

	//calculation checkSum
	ret = checkSum_BCC(pData, dataLen);
	*pos++ = checkSum_BCC(pData, dataLen);
	IVI_LOG("checkSum_BCC: %02x\n", ret);

	totalLen = pos-pData;
	IVI_LOG("total data Len: %d\n", totalLen);
	
	IVI("\n+++++++++++++++++++++++++++++++++++++++++\n");
	for(i = 0; i < totalLen; ++i)
		IVI("%02x ", *(pData + i));
	IVI("\n\n");
	IVI("+++++++++++++++++++++++++++++++++++++++++\n");

	return totalLen;
}
/*
	电话状态回调函数
*/
uint8_t IVI_Communication::TBOX_Voicall_State(uint8_t *pData)
{
	int i;
	call_info_type call_list[QMI_VOICE_CALL_INFO_MAX_V02];
	
	memcpy((void *)call_list, pData, sizeof(call_list));

	for(i = 0; i < QMI_VOICE_CALL_INFO_MAX_V02; i++)
	{
		if(call_list[i].call_id != 0)
		{
			#if 0
			printf(">>>>>>voice callback:\n");
			printf("index[%d], call_id=%d, state=%d, direction = %d, number=%s\n",
				  i,
				  call_list[i].call_id,
				  call_list[i].call_state,
				  call_list[i].direction,
				  strlen(call_list[i].phone_number) >= 1?call_list[i].phone_number:"unkonw");
			printf("<<<<<<<<<<< \n");
			#endif
			callID = call_list[i].call_id;
			memset(incomingPhoneNum, 0, sizeof(incomingPhoneNum));
			if(strlen(call_list[i].phone_number) >= 1)
			{
				memset(incomingPhoneNum, 0, sizeof(incomingPhoneNum));
				memcpy(incomingPhoneNum, call_list[i].phone_number, strlen(call_list[i].phone_number));
			}

			switch (call_list[i].call_state)
			{
				case 0x01:						/*发起通话*/
					//voiceCallState = 0;
					break;
				case CALL_STATE_INCOMING_V02:  //来电 0x02
					voiceCallState = 0;
                    TBOX_Call_State_Report(NULL, 0, 3);
					break;
				case CALL_STATE_CONVERSATION_V02:	//通话中 0x03
					voiceCallState = 1;
                    TBOX_Call_State_Report(NULL, 0, 3);
					break;
				case CALL_STATE_DISCONNECTING_V02:	//挂断 0x08
				case CALL_STATE_END_V02:	        //对方挂断 0x09
					voiceCallState = 2;
					alertingstate = 0;
					LteAtCtrl->CallMute();
					tboxInfo.operateionStatus.phoneType = 0;
                    TBOX_Call_State_Report(NULL, 0, 3);
					break;
				case CALL_STATE_ALERTING_V02://alerting 0x05
					voiceCallState = 3;
					alertingstate = 1;
                    TBOX_Call_State_Report(NULL, 0, 3);
					break;
				case CALL_STATE_CC_IN_PROGRESS_V02: //0x04
						break;
				default:
					break;
			}
		}
	} 
	return 0;
}

static void handler2(union sigval v)
{
    int Callaction= (int)v.sival_int;
	uint8_t phoneNum[20] = {0};
	int state;

	if(Callaction == 0x00)  //B-Call
	{
		if(CFAWACP::cfawacp->m_loginState == 2){
			CFAWACP::cfawacp->timingReportingData(MESSAGE_TYPE_START + 2, ACPApp_EmergencyDataID);
		}
		dataPool->getPara(B_CALL_ID, (void *)phoneNum, sizeof(phoneNum));
		state = voiceCall(phoneNum, PHONE_TYPE_B);
	}

}

int IVI_Communication::CreateThreadTimer(struct itimerspec its,void *action, pthHandler handler)
{
    struct sigevent sev;
    int ret=0;
 
    memset(&sev,0,sizeof(sev));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function=handler;//处理函数
    sev.sigev_value.sival_int = *(int *)action;//传递给处理函数的参数，指针型
    //sev.sigev_value.sival_int=timerid;
    if (timer_create(CLOCKID, &sev, &timerid) == -1)
    {
        perror("timer_create\n");
        ret=-1;
    }
 
    if (timer_settime(timerid, 0, &its, NULL) == -1)
    {
        perror("timer_settime\n");
        ret=-1;
    }
 
    return ret;
}

int IVI_Communication::GetTimerTime(struct itimerspec *ts)
{
	return timer_gettime(timerid, ts);
}

int IVI_Communication::DeleteTimer()
{
	return timer_delete(timerid);
}

uint32_t IVI_Communication::Get_Seconds_from_1970()
{
	uint32_t seconds;
    time_t tv;
   //struct tm *p;

	time(&tv); /*当前time_t类型UTC时间*/  
	//IVI_LOG("time():%d\n",tv);

	seconds = (uint32_t)tv;
	IVI_LOG("seconds:%d\n", seconds);

	//p = localtime(&tv); /*转换为本地的tm结构的时间按*/  
	//tv = mktime(p); /*重新转换为time_t类型的UTC时间，这里有一个时区的转换,将struct tm 结构的时间转换为从1970年至p的秒数 */ 
	//printf("time()->localtime()->mktime(): %d\n", tv);

    return seconds;
}

int IVI_Communication::sys_mylog(char *plog)
{
	#define LOG_PATH	"/data/mylog"
	char buff[1024] = "";
	sprintf(buff, "== %s ==\n", plog);
	FILE *flogfd;
	flogfd = fopen(LOG_PATH, "a+");
	if(flogfd == NULL)
	{
		printf("filelog open failed\n");
		return -1;
	}

	fwrite(buff, 1, strlen(buff), flogfd);

	fclose(flogfd);
	return 0;
}


