#include "OTAWiFi.h"


OTAWiFi::~OTAWiFi(){}

OTAWiFi::OTAWiFi()
{
	int ret;
	pthread_t serverThreadId;

    sockfd = -1;
    accept_fd = -1;
    isExsistMcuFile = false;
    isExsistTboxFile = false;

	u_fileInfo.updataType = 0; 
	u_fileInfo.mcuCheckSum = 0;
	u_fileInfo.mcuFileSize = 0;
	u_fileInfo.tboxCheckSum = 0;
	u_fileInfo.tboxFileSize = 0;

#if 1
	ret = pthread_create(&serverThreadId, NULL, serverThread, this);
	if(0 != ret) 
	{
		printf("can't create thread: %s\n",strerror(ret)); 
		exit(-1);
	}
#endif
	
}

void OTAWiFi::et_process(struct epoll_event *events, int number, int epoll_fd, int socketfd)
{
    int i;
    int ret;
    uint8_t buff[BUFFER_SIZE] = {0};

	/*uint8_t *buff = (uint8_t *)malloc(BUFFER_SIZE);
	if(buff == NULL)
	{
		EPOLL_LOG("malloc dataBuff error!");
		return;
	}*/
	//memset(buff, 0, BUFFER_SIZE);
    
    for(i = 0; i < number; i++)
    {
        if(events[i].data.fd == socketfd)
        {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            int connfd = accept(socketfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
            add_socketFd(epoll_fd, connfd);
        }
        else if(events[i].events & EPOLLIN)
        {
            EPOLL_LOG("et mode: event trigger once!\n");
            memset(buff, 0, BUFFER_SIZE);
            int dataLen = recv(events[i].data.fd, buff, BUFFER_SIZE, 0);
            accept_fd = events[i].data.fd;
			EPOLL_LOG("dataLen:%d\n", dataLen);
            if(dataLen <= 0)
            {
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL); 
                close(events[i].data.fd);
            }
            else
            {
                EPOLL_LOG("Recevied data len %d:\n", dataLen);
				for(i = 0; i < dataLen; ++i)
					EPOLL("%02x ", *(buff + i));
				EPOLL("\n\n");
				
	            if(checksum(buff, dataLen))
	            {
	                unpack_WifiData(buff, dataLen);
	            }
            }
        }
        else
        {
            printf("something unexpected happened!\n");
        }
    }
}

int OTAWiFi::set_non_block(int fd)
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

void OTAWiFi::add_socketFd(int epoll_fd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    if(set_non_block(fd) == 0)
    	EPOLL_LOG("set_non_block ok!\n");
}

#if 0

void *OTAWiFi::serverThread(void *arg)
{
    pthread_detach(pthread_self());
    pthread_t clientThreadId;
    OTAWiFi *p_upgrade = (OTAWiFi *)arg;
    char buf[BUFFER_SIZE] = {0};
    int i;
    
	if(p_upgrade->socketConnect() == 0)
		EPOLL_LOG("create socket ok!\n");

	while(1)
	{
        struct sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);

        int fd = accept(p_upgrade->sockfd, (struct sockaddr*) &clientAddr, &len);
        if (-1 == fd)
        {
            perror("accept");
			exit(-1);
        }
        p_upgrade->accept_fd = fd;
        EPOLL_LOG("server: got connection from %s\n", inet_ntoa(clientAddr.sin_addr));

		if (!fork())
		{
			while(1)
			{
				int dataLen = recv(fd, buf, BUFFER_SIZE, 0);
				if(dataLen <= 0){
					EPOLL_LOG("dataLen < 0\n");
					break;
					close(fd);
				}
				EPOLL_LOG("Recevied data len %d:\n", dataLen);
				for(i = 0; i < dataLen; ++i)
					EPOLL("%02x ", *(buf + i));
				if(p_upgrade->checksum(buf, dataLen))
				{
					p_upgrade->unpack_WifiData(buf, dataLen);
				}
			}
		}
		waitpid(-1, NULL, WNOHANG);
	}
}

#else
void *OTAWiFi::serverThread(void *arg)
{
    pthread_detach(pthread_self());
    prctl(PR_SET_NAME, "OtaByWifi");
    OTAWiFi *p_upgrade = (OTAWiFi *)arg;
	int epoll_fd;
	struct epoll_event events[MAX_EVENT_NUMBER];
	
    while(1){
	    if(p_upgrade->socketConnect() == 0)
	    	EPOLL_LOG("create socket ok!\n");
		
	    epoll_fd = epoll_create(5);
	    if(epoll_fd == -1)
	    {
	        printf("fail to create epoll!\n");
	        close(p_upgrade->sockfd);
	        sleep(1);
	        //return -1;
	    }else{
	    	break;
	    }
    }

    p_upgrade->add_socketFd(epoll_fd, p_upgrade->sockfd);

	while(1)
	{
		int num = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
		if(num < 0)
		{
			printf("epoll failure!\n");
			break;
		}
		p_upgrade->et_process(events, num, epoll_fd, p_upgrade->sockfd);
	}
	
    close(p_upgrade->sockfd);
}
#endif

int OTAWiFi::socketConnect()
{
    int opt = 1;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        EPOLL_LOG("socket error!");
        return -1;
    }
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_WIFI_IP);
    serverAddr.sin_port = htons(SERVER_LISTEN_PORT);

    if(bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        EPOLL_ERROR("bind failed.\n");
        close(sockfd);
        return -1;
    }

    if(listen(sockfd, LISTEN_BACKLOG) == -1)
    {
        EPOLL_ERROR("listen failed.\n");
        close(sockfd);
        return -1;
    }

    return 0;
}

int OTAWiFi::disconnectSocket()
{
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
	EPOLL_LOG("Close socket sockfd=%d",sockfd);
	return 0;
}

int8_t OTAWiFi::unpack_WifiData(uint8_t *pData, int32_t len)
{
	uint8_t cmdId;
	uint8_t *pos = pData;
	int totalDataLen;
	static uint32_t upgradePackageSize = 0;
	
	cmdId = pData[0];
	
	switch(cmdId)
	{
		case 0x01:
			break;
		case 0x02:
			{
				EPOLL_LOG("2222222222222222222222222222222\n");
				//totalDataLen = *(pos+1)<<24 + *(pos+2)<<16 + *(pos+3)<<8 + *(pos+4);
				
				//reset exsist file flag
				isExsistMcuFile = false;
				isExsistTboxFile = false;

				totalDataLen = *(pos+1)<<24;
				totalDataLen += *(pos+2)<<16;
				totalDataLen += *(pos+3)<<8;
				totalDataLen += *(pos+4)<<0;
				
				EPOLL_LOG("totalDataLen: %d, | %d,%d,%d,%d\n", totalDataLen, *(pos+1),*(pos+2),*(pos+3),*(pos+4));
				
				if(*(pos+5) == 1)
				{
					EPOLL_LOG("tbox info!\n");
					
					u_fileInfo.updataType = 1;
					u_fileInfo.tboxFileSize = *(pos+6)<<24;
					u_fileInfo.tboxFileSize += *(pos+7)<<16;
					u_fileInfo.tboxFileSize += *(pos+8)<<8;
					u_fileInfo.tboxFileSize += *(pos+9)<<0;

					u_fileInfo.tboxCheckSum = *(pos+10);
					EPOLL_LOG("u_fileInfo.tboxFileSize %d, u_fileInfo.tboxCheckSum :%d\n", u_fileInfo.tboxFileSize, u_fileInfo.tboxCheckSum);
					
				}else if(*(pos+5) == 2){
					EPOLL_LOG("mcu info!\n");
					
					u_fileInfo.updataType = 2;
					u_fileInfo.mcuFileSize = *(pos+6)<<24;
					u_fileInfo.mcuFileSize += *(pos+7)<<16;
					u_fileInfo.mcuFileSize += *(pos+8)<<8;
					u_fileInfo.mcuFileSize += *(pos+9)<<0;

					u_fileInfo.mcuCheckSum = *(pos+10);
					EPOLL_LOG("u_fileInfo.mcuFileSize %d, u_fileInfo.mcuCheckSum :%d\n", u_fileInfo.mcuFileSize, u_fileInfo.mcuCheckSum);
				}
				
				packetDataApp(0x12,0,0);
			}
			break;
		case 0x03:
			{
				EPOLL_LOG("3333333333333333333333333333333\n");
				//totalDataLen = *(pos+1)<<24 + *(pos+2)<<16 + *(pos+3)<<8 + *(pos+4);
				totalDataLen = *(pos+1)<<24;
				totalDataLen += *(pos+2)<<16;
				totalDataLen += *(pos+3)<<8;
				totalDataLen += *(pos+4)<<0;
				EPOLL_LOG("totalDataLen %d | %d,%d,%d,%d\n", totalDataLen, *(pos+1),*(pos+2),*(pos+3),*(pos+4));
				
				int dataLen = totalDataLen - 2;

				if(*(pos+5) == 1)
				{
					upgradePackageSize += dataLen;
					EPOLL_LOG("totalDataLen: %d, dataLen:%d,u_fileInfo.tboxFileSize:%d, upgradePackageSize:%d\n", totalDataLen, dataLen, u_fileInfo.tboxFileSize,upgradePackageSize);
					if(isExsistTboxFile == false)
					{
						if (!access(LTE_FILE_NAME, F_OK))
						{
							EPOLL_LOG("Exisist tbox upgrade file, remove it!\n");
							system(RM_LTE_FILE);
						}
						isExsistTboxFile = true;
					}
					if(storageFile(LTE_FILE_NAME, pos+7, dataLen) == 0){
						EPOLL_LOG("get tbox file ok!");
					}

					if(upgradePackageSize == u_fileInfo.tboxFileSize)
					{
						EPOLL_LOG("11111111111111111111111111111111\n");
						//first to check the file.
						//if(check_file_result(LTE_FILE_NAME, u_fileInfo.tboxCheckSum, NULL) == 1)
						if(1)
						{
							EPOLL_LOG("go to upgrade tbox!\n");
							wds_qmi_release();
							nas_qmi_release();
							voice_qmi_release();
							mcm_sms_release();
							mcuUart::m_mcuUart->close_uart();
							LteAtCtrl->~LTEModuleAtCtrl();
							printf("################### exit 2 ###########################\n");
							
							exit(0);

						}else{
							EPOLL_LOG("222222222222222222222222222\n");
							system(RM_LTE_FILE);
						}
					}
				}
				else if(*(pos+5) == 2)
				{
					upgradePackageSize += dataLen;
					EPOLL_LOG("totalDataLen: %d, dataLen:%d,u_fileInfo.mcuFileSize:%d, upgradePackageSize:%d\n", totalDataLen, dataLen, u_fileInfo.mcuFileSize,upgradePackageSize);
					if(isExsistMcuFile == false)
					{
						if (!access(MCU_FILE_NAME, F_OK))
						{
							EPOLL_LOG("Exisist mcu upgrade file, remove it!\n");
							system(RM_MCU_FILE);
						}
						isExsistMcuFile = true;
					}
					if(storageFile(MCU_FILE_NAME, pos+7, dataLen) == 0){
						EPOLL_LOG("get mcu file ok!");
					}
					
					if(upgradePackageSize == u_fileInfo.mcuFileSize)
					{
						uint16_t crc16CheckCode;
						EPOLL_LOG("check sum ok\n");

						//No check,only for get crc16CheckCode;
						check_file_result(MCU_FILE_NAME, u_fileInfo.mcuCheckSum, &crc16CheckCode);
						//if(check_file_result(MCU_FILE_NAME, u_fileInfo.mcuCheckSum, &crc16CheckCode) == 1)
						if(1)
						{
							EPOLL_LOG("go to upgrade mcu\n");

							memcpy(upgradeInfo.newMcuVersion, "V1.0.1",strlen("V1.0.1"));
							EPOLL_LOG("###################### 1111 #####################\n");
							//文件长度
							upgradeInfo.mcuVersionSize[0] = (u_fileInfo.mcuFileSize>>24)&0xff;
							upgradeInfo.mcuVersionSize[1] = (u_fileInfo.mcuFileSize>>16)&0xff;
							upgradeInfo.mcuVersionSize[2] = (u_fileInfo.mcuFileSize>>8)&0xff;
							upgradeInfo.mcuVersionSize[3] = (u_fileInfo.mcuFileSize>>0)&0xff;
							
							upgradeInfo.mcuVersionCrc16[0] = (crc16CheckCode>>8)&0xff;
							upgradeInfo.mcuVersionCrc16[1] = (crc16CheckCode>>0)&0xff;
							EPOLL_LOG("###################### 22222 #####################\n");

							mcuUart::m_mcuUart->pack_mcuUart_upgrade_data(0x8a, true, 0);
							
							EPOLL_LOG("###################### 33333 #####################\n");

						}else{
							EPOLL_LOG("rm mcu\n");
							system(RM_MCU_FILE);
						}
					}
				}

				packetDataApp(0x13, 0, *(pos+6));
			}
			break;
		case 0x04:
			EPOLL_LOG("5555555555555555555555555555555555555\n");
			
			packetDataApp(0x14, 0, 0);
			break;
		case 0x05:
			{
				/*if(flen == )
				{
					printf("文件校验成功\n");
					packetDataApp(0x15, 0, 0);
				}
				else
				{
					packetDataApp(0x15, 1, 0);
				}*/
			}
			break;
		case 0x06:
			//packetDataApp(0x16, 0, 0);
			break;
		default:
			EPOLL_LOG("unpack Wifidata error!");
			break;
	}
	return 0;
}

uint8_t OTAWiFi::checksum(uint8_t *pData, uint32_t len)
{
    uint16_t i;
    uint8_t sum = 0x00;
    uint8_t *pos = pData;

    for(i = 0; i < len - 1; i++)
    {
        sum ^= *(pos + i);
    }

	if(sum == *(pos+len-1))
	{
		EPOLL_LOG("checksum = %d\n", sum);
		return 1;
	}

	return 0;
}

uint8_t OTAWiFi::checkPackDatasum(uint8_t *pData, uint32_t len)
{
    uint16_t i;
    uint8_t sum = 0x00;
    uint8_t *pos = pData;

    for(i = 0; i < len; i++)
    {
        sum ^= *(pos + i);
    }

	return sum;
}

void OTAWiFi::packetDataApp(uint8_t cmd, uint8_t responseFlag, uint8_t serialnum)
{
	int length = 0;
	uint8_t dataBuff[DATA_BUFF_LEN] = {0};

	/*uint8_t *dataBuff = (uint8_t *)malloc(DATA_BUFF_LEN);
	if(dataBuff == NULL)
	{
		EPOLL_LOG("malloc dataBuff error!");
		return;
	}
	memset(dataBuff, 0, DATA_BUFF_LEN);*/
	
	uint8_t *pos = dataBuff;

	switch(cmd)
	{
		case 0x11:
		case 0x12:
			{
				EPOLL_LOG("packetDataApp 0x12!\n");
				*pos++ = cmd;
				*pos++ = 0;
				*pos++ = 0;
				*pos++ = 0;
				*pos++ = 1;

				*pos++ = responseFlag;

				uint8_t checkSumResult = checkPackDatasum(dataBuff, 6);
				EPOLL_LOG("checkSumResult:%d\n", checkSumResult);

				*pos++ = checkSumResult;

				length = pos - dataBuff;
				EPOLL_LOG("length:%d\n", length);
			}
			break;			
		case 0x13:
			{
				EPOLL_LOG("packetDataApp 0x13!\n");
				*pos++ = cmd;
				*pos++ = 0;
				*pos++ = 0;
				*pos++ = 0;
				*pos++ = 2;

				*pos++ = responseFlag;
				*pos++ = serialnum;

				uint8_t checkSumResult = checkPackDatasum(dataBuff, 7);
				EPOLL_LOG("checkSumResult:%d\n", checkSumResult);

				*pos++ = checkSumResult;
				
				length = pos - dataBuff;
				EPOLL_LOG("length:%d\n", length);
			}
			break;
		case 0x14:
		case 0x15:
		case 0x16:
			{
				if(cmd == 0x14)
					EPOLL_LOG("packetDataApp 0x13!\n");
				else if(cmd == 0x15)
					EPOLL_LOG("packetDataApp 0x15!\n");
				else
					EPOLL_LOG("packetDataApp 0x16!\n");
					
				*pos++ = cmd;
				*pos++ = 0;
				*pos++ = 0;
				*pos++ = 0;
				*pos++ = 1;
				
				uint8_t checkSumResult = checkPackDatasum(dataBuff, 5);
				EPOLL_LOG("checkSumResult:%d\n", checkSumResult);
				
				*pos++ = checkSumResult;
				
				length = pos - dataBuff;
				EPOLL_LOG("length:%d\n", length);
			}
			break;
		default:
			break;
	}

	if((length = send(accept_fd, dataBuff, length, 0)) < 0)
	{
		close(accept_fd);
	}
	else
	{
		EPOLL_LOG("Send App command %d data ok\n",length);
	}
	
	/*if(dataBuff != NULL){
		free(dataBuff);
		dataBuff = NULL;
	}*/
}

int OTAWiFi::check_file_result(char *fileName, uint8_t fileCheckCode, uint16_t *crc16Check)
{
	int nRead;
	uint8_t checkcode;
	unsigned char buff[DATA_BUFF_LEN];

	int fd = open(fileName, O_RDONLY);
	if (fd < 0)
	{
		printf("Open file:%s error.\n", fileName);
		return -1;
	} 

	while ((nRead = read(fd, buff, DATA_BUFF_LEN)) > 0)
	{
		checkcode = checkPackDatasum(buff, nRead);
		if(crc16Check != NULL)
			*crc16Check = Crc16Check(buff, nRead);
	}
	
	close(fd);

	EPOLL_LOG("fileCheckCode:%d, checkcode:%d",fileCheckCode, checkcode);
	if(crc16Check != NULL)
		EPOLL_LOG("crc16Check:%d",*crc16Check);

	if(checkcode == fileCheckCode){
		EPOLL_LOG("file checke ok!");
		return 1;
	}

	return 0;
}

unsigned int OTAWiFi::Crc16Check(unsigned char* pData, uint32_t len)
{
	unsigned int sumInitCrc = 0xffff;
	unsigned int ui16Crc = 0;
	unsigned int i;
	unsigned char j;
	unsigned char shiftBit;
	
	for(i = 0; i<len; i++)
	{		
		sumInitCrc ^= pData[i];
		for(j=0; j<8; j++)
		{
			shiftBit = sumInitCrc&0x01;
			sumInitCrc >>= 1;
			if(shiftBit != 0)
			{
				sumInitCrc ^= 0xa001;
			}		
		}
	}
	
	ui16Crc = sumInitCrc;
	return ui16Crc;
}

#if 1
int OTAWiFi::storageFile(uint8_t *fileName, uint8_t *pData, int32_t len)
{
	int fd = open(fileName, O_RDWR | O_CREAT | O_APPEND, 0777);
	if (-1 == fd)
	{
		printf(" OTAWiFi storageFile open file failed\n");
		return -1;
	}
	if (len != write(fd, pData, len))
	{
		printf("write file error!\n");
		return -1;
	}

	close(fd);

	return 0;
}
#else
int OTAWiFi::storageFile(uint8_t *fileName, uint8_t *pData, int32_t len)
{
	int retval;
	FILE *fp = fopen(fileName, "ab+");
	if(fp != NULL)
		printf("Open file!\n");

	if((retval = fwrite(pData, len, 1, fp)) > 0){
		printf("Write %d data to %s.\n",retval, fileName);
	}else{
		printf("Write error!\n");
		return -1;
	}
	
	fclose(fp);

	return 0;
}
#endif




unsigned long get_file_size(const char *path)
{
	unsigned long filesize = -1;
	
	FILE *fp;
	fp = fopen(path, "r");
	if(fp == NULL)
		return filesize;

	fseek(fp, 0L, SEEK_END);
	filesize = ftell(fp);
	fclose(fp);
	
	return filesize;
}

unsigned int Crc16Check(unsigned char *pData, unsigned int len)
{
	unsigned int u16InitCrc = 0xffff;
	unsigned int i;
	unsigned char j;
	unsigned char u8ShitBit;
	
	for(i = 0; i<len; i++)
	{		
		u16InitCrc ^= pData[i];
		for(j=0; j<8; j++)
		{
			u8ShitBit = u16InitCrc&0x01;
			u16InitCrc >>= 1;
			if(u8ShitBit != 0)
			{
				u16InitCrc ^= 0xa001;
			}		
		}
	}
	
	return u16InitCrc;
}

int calculate_files_CRC(char *fileName, unsigned int *crc)
{
	int nRead;
	uint8_t buff[1024];
	int fd = 0;

	fd = open(fileName, O_RDONLY);
	if (fd < 0)
	{
		printf("Open file:%s error.\n", fileName);
		return -1;
	}else
		printf("Open file:%s success.\n", fileName);

	memset(buff, 0, sizeof(buff));
	while ((nRead = read(fd, buff, sizeof(buff))) > 0)
	{
		*crc = Crc16Check(buff, nRead);
	}

	close(fd);

	return 0;
}


int check_mcu_file_is_exist()
{
	unsigned int crc;
	unsigned long filesize;

	if(!access(MCU_FILE_NAME, F_OK))
	{
		printf("Exisist mcu upgrade file.\n");
	}

	filesize = get_file_size(MCU_FILE_NAME);
	if(filesize == -1)
		printf("get file size error!");

	calculate_files_CRC(MCU_FILE_NAME, &crc);


	printf("go to upgrade mcu\n");

	memcpy(upgradeInfo.newMcuVersion, "V1.0.1",strlen("V1.0.1"));
	printf("###################### 1111 #####################\n");
	//文件长度
	upgradeInfo.mcuVersionSize[0] = (filesize>>24)&0xff;
	upgradeInfo.mcuVersionSize[1] = (filesize>>16)&0xff;
	upgradeInfo.mcuVersionSize[2] = (filesize>>8)&0xff;
	upgradeInfo.mcuVersionSize[3] = (filesize>>0)&0xff;
	
	upgradeInfo.mcuVersionCrc16[0] = (crc>>8)&0xff;
	upgradeInfo.mcuVersionCrc16[1] = (crc>>0)&0xff;

	mcuUart::m_mcuUart->pack_mcuUart_upgrade_data(0x8a, true, 0);
	
	printf("###################### 33333 #####################\n");


	return 0;
}




