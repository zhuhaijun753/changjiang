#include "OTAUpgrade.h"



/*****************************************************************************
* Function Name : ~OTAUpgrade
* Description   : 析构函数
* Input			: None
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
OTAUpgrade::~OTAUpgrade()
{
    close(socketfd);
}

/*****************************************************************************
* Function Name : OTAUpgrade
* Description   : 构造函数
* Input			: None
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
OTAUpgrade::OTAUpgrade()
{
	int count = 0;
    socketfd = -1;
	isConnected = false;
	isReceivedData = false;
	upgradeFlag = 0;

	socketConnect();

	while (!isReceivedData)
    {
    	if(isConnected == false)
		{
			count++;
			
			OTAUPGADE("count=%d",count);
			if(count == 3)
			{
				break;
				this->~OTAUpgrade();
			}
			socketConnect();
		}
		
        sleep(1*10);
    }
}

/*****************************************************************************
* Function Name : disSocketConnect
* Description   : 断开socket
* Input			: None
* Output        : None
* Return        : 0
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int OTAUpgrade::disSocketConnect(void)
{
	shutdown(socketfd, SHUT_RDWR);
	close(socketfd);
	isConnected = false;
	OTAUPGADE("Close socket socketfd=%d",socketfd);

	return 0;
}

/*****************************************************************************
* Function Name : socketConnect
* Description   : 创建socket函数
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int OTAUpgrade::socketConnect(void)
{/*
	uint16_t OTAServerPort = 0;
	char OTA_IP[32];
	uint8_t u8IPAndPort[6];

	dataPool->getPara(OTA_IP_PORT_ID, u8IPAndPort, 6);
	
	memset(OTA_IP, 0, sizeof(OTA_IP));
	sprintf(OTA_IP,"%d.%d.%d.%d",u8IPAndPort[0],u8IPAndPort[1],u8IPAndPort[2],u8IPAndPort[3]);

	OTAServerPort = (u8IPAndPort[4] << 8) + u8IPAndPort[5];
	OTAUPGADE("OTA_IP:%s, OTAServerPort:%d\n",OTA_IP,(int)OTAServerPort);
	*/
	socketaddr.sin_family = AF_INET;
	//socketaddr.sin_port = htons(OTAServerPort);
	//socketaddr.sin_addr.s_addr = inet_addr(OTA_IP);
	socketaddr.sin_port = htons(8108);
	socketaddr.sin_addr.s_addr = inet_addr("210.74.1.30");
 	//ip:"210.74.1.30", port:8108

	socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if ( -1 != socketfd)
	{
		OTAUPGADE("socketfd:%d\n",socketfd);
		if (connect(socketfd, (struct sockaddr*) &socketaddr, sizeof(socketaddr)) != -1)
		{
			isConnected = true;
			OTAUPGADE("Connect to server successfully!\n");

			if(sendVersionInfo() == -1)
				this->~OTAUpgrade();
		}
		else
		{
			OTAUPGADE("connect failed close socketfd!");
			disSocketConnect();
		}
	}

	return 0;
}

/*****************************************************************************
* Function Name : sendVersionInfo
* Description   : 发送版本函数
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int OTAUpgrade::sendVersionInfo()
{
	uint16_t len;
	int dataLen;

	uint8_t *dataBuff = (uint8_t *)malloc(DATA_LEN);
	if(dataBuff == NULL)
	{
		OTAUPGADE("malloc dataBuff error!");
		return -1;
	}

	memset(dataBuff, 0, DATA_LEN);

	len = packVersionInfo(dataBuff, DATA_LEN);
	OTAUPGADE("len:%d!",len);

	dataLen = send(socketfd, dataBuff, len, 0);
	if(dataLen > 0)
	{
		memset(dataBuff, 0, DATA_LEN);
		
		dataLen = recv(socketfd, dataBuff, DATA_LEN, 0);
		if(dataLen > 0)
		{
			OTAUPGADE("received data, dataLen:%d",dataLen);
			printMsg(dataBuff, dataLen);

			isReceivedData = true;
			if(checkReceiveData(dataBuff, dataLen) == 0)
			{
				if(unpack_protocol(dataBuff, dataLen) == 0)
				{
					if(dataBuff != NULL)
					{
						OTAUPGADE("free memory!");
						free(dataBuff);
						dataBuff = NULL;
					}
					
					//调用FTP进行升级
					if(upgradeFlag == 1)
					{
						if(downloadFile(1, lteDownloadPath, (char *)LTE_FILE_NAME, NULL, NULL) == -1)
							return -1;
					}
					else if(upgradeFlag == 2)
					{
						if(downloadFile(2, NULL, NULL, mcuDownloadPath, (char *)MCU_FILE_NAME) == -1)
							return -1;
					}
					else //(upgradeFlag == 3)
					{
						if(downloadFile(3, lteDownloadPath, (char *)LTE_FILE_NAME, mcuDownloadPath, (char *)MCU_FILE_NAME) == -1)
							return -1;
					}
				}
			}
			else
			{
				if(dataBuff != NULL)
				{
					OTAUPGADE("free memory!");
					free(dataBuff);
					dataBuff = NULL;
				}
				
				disSocketConnect();
			}
		}
		else
		{
			if(dataBuff != NULL)
			{
				OTAUPGADE("free memory!");
				free(dataBuff);
				dataBuff = NULL;
			}
			disSocketConnect();
		}
	}
	else
	{
		OTAUPGADE("send data failed!");
		disSocketConnect();
		
		if(dataBuff != NULL)
		{
			OTAUPGADE("free memory!");
			free(dataBuff);
			dataBuff = NULL;
		}
	}

	return 0;
}

/*****************************************************************************
* Function Name : packVersionInfo
* Description   : 打包软件版本信息
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint16_t OTAUpgrade::packVersionInfo(uint8_t *dataBuff, uint16_t len)
{
	OTAUPGADE();
	uint8_t checkSum;
	uint16_t dataLen;
	uint8_t *pos = dataBuff;

	*pos++ = (POTOCAL_HEAD >>24) & 0xFF;
	*pos++ = (POTOCAL_HEAD >>16) & 0xFF;
	*pos++ = (POTOCAL_HEAD >>8)  & 0xFF;
	*pos++ = (POTOCAL_HEAD >>0)  & 0xFF;

	*pos++ = GID;
	*pos++ = ADDR;
	*pos++ = CID;

	//len
	*pos++ = 0;
	*pos++ = 0;

	//INFO
	*pos++ = 0x01;//sysVerNumber[0];  // lte Major_Version
	*pos++ = 0x02;//sysVerNumber[1];  // lte Minor_Version
	*pos++ = sysVerNumber[2];  // lte Minor_Version
	
	*pos++ = sysVerNumber[0]; //mcuVerNumber[0];  // MCU Major_Version
	*pos++ = sysVerNumber[1]; //mcuVerNumber[1];  // MCU Minor_Version
	*pos++ = sysVerNumber[2]; //mcuVerNumber[2];  // MCU Minor_Version

	OTAUPGADE("sysVerNumber[0]:%02x,sysVerNumber[1]:%02x,sysVerNumber[2]:%02x\n",sysVerNumber[0],sysVerNumber[1],sysVerNumber[2]);
	OTAUPGADE("mcuVerNumber[0]:%02x,mcuVerNumber[1]:%02x,mcuVerNumber[2]:%02x\n",mcuVerNumber[0],mcuVerNumber[1],mcuVerNumber[2]);

	//fill data length
	dataLen = pos - dataBuff - 9;
	dataBuff[7] = dataLen & 0xff;
	dataBuff[8] = (dataLen & 0xff00)>>8;
	OTAUPGADE("dataLen: %d\n",dataLen);

	//check sum
	checkSum = checkXOR(dataBuff, pos-dataBuff);
	OTAUPGADE("checkSum: %0x\n",checkSum);
	
	*pos++ = checkSum;

	//*pos++ = (POTOCAL_END >>8)  & 0xFF;
	//*pos++ = (POTOCAL_END >>0)  & 0xFF;

	OTAUPGADE("total len: %d\n",(uint16_t)(pos-dataBuff));
	printMsg(dataBuff, (uint16_t)(pos-dataBuff));

	return (uint16_t)(pos-dataBuff);
}

/*****************************************************************************
* Function Name : checkXOR
* Description   : 数据检验
* Input			: uint8_t *pData
*                 uint16_t len;
* Output        : None
* Return        : check_sum
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
uint8_t OTAUpgrade::checkXOR(uint8_t *pData, uint16_t len)
{
    uint8_t check_sum;
    uint16_t check_sum_cnt;

    check_sum = pData[0];
    for (check_sum_cnt = 1; check_sum_cnt < len; check_sum_cnt++)
    {
        check_sum ^= pData[check_sum_cnt];
    }

    return check_sum;
}

/*****************************************************************************
* Function Name : printMsg
* Description   : 打印数据包内容
* Input			: uint8_t *pData
*                 uint16_t len;
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void OTAUpgrade::printMsg(uint8_t *pData, uint16_t len)
{
	int i;

	for(i=0; i<len; i++)
		printf("%02x ",*(pData+i));
	printf("\n");
}

/*****************************************************************************
* Function Name : checkReceiveData
* Description   : 检查接收到的数据
* Input			: uint8_t *pData
*                 uint32_t size;
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int OTAUpgrade::checkReceiveData(uint8_t *pData, uint32_t size)
{
	uint16_t dataLen;
	uint8_t *pos = pData;

	if((*pos == 0x7e) && (*(pos+1) == 0x7e) && (*(pos+2) == 0x7e) && (*(pos+3) == 0x7e))
		OTAUPGADE("check head ok!");

	if(*pos++ == GID)
	{
		OTAUPGADE("GID check error!\n");
		return -1;
	}

	if(*pos++ == ADDR)
	{
		OTAUPGADE("ADDR check error!\n");
		return -1;
	}

	if(*pos++ == CID)
	{
		OTAUPGADE("CID check error!\n");
		return -1;
	}

	dataLen = *pos;
	dataLen += *(pos+1) << 8;
	OTAUPGADE("dataLen :%d\n", dataLen);

	pos = pos+dataLen;
	//check sum
	if(*pos == checkXOR(pData, dataLen+9))
	{
		OTAUPGADE("check sum ok: %02x\n", *pos);
	}
	else
	{
		OTAUPGADE("check sum error\n");
		return -1;
	}

/*	pos += 1;
	if(memcmp((uint8_t *)POTOCAL_END, pos, 2) != 0)
	{
		OTAUPGADE("potocal end check error!\n");
		return -1;
	} */

	return 0;
}

/*****************************************************************************
* Function Name : unpack_protocol
* Description   : 解包函数
* Input			: uint8_t *pData
*                 uint16_t len;
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int OTAUpgrade::unpack_protocol(uint8_t *pData, uint16_t len)
{
	OTAUPGADE();
	uint8_t i;
	uint8_t *pos = pData;
	char port[6];

	pos = pos+9;
	if(*pos == 1)
	{
		OTAUPGADE("upgrade let version!\n");
		//get lte version
		pos = pos+1;
		memcpy(lteVersion, pos, 3);
		OTAUPGADE("lteVersion version!\n");
		printMsg(lteVersion, 3);
		
		//get lte upgrade check code
		pos += 3;
		memcpy(lteCheckCode, pos, 4);
		OTAUPGADE("lteCheckCode version!\n");
		printMsg(lteCheckCode, 4);

		pos += 4;

		memset(lteDownloadPath, 0, sizeof(lteDownloadPath));
		for(i = 0; i<32; i++)
		{
			lteDownloadPath[i] = *(pos+i);
			if(*(pos+i) == 0x00)
				break;
		}

		OTAUPGADE("lteDownloadPath:%s\n", lteDownloadPath);
		//set upgrade flag
		upgradeFlag = 1;

		//judgment the mcu version
		pos += 32;

		if(*pos == 1)
		{
			OTAUPGADE("upgrade mcu version!\n");
			//get mcu version
			pos = pos+1;
			memcpy(mcuVersion, pos, 3);
			OTAUPGADE("mcuVersion version!\n");
			printMsg(mcuVersion, 3);
			//get mcu upgrade check code
			pos += 3;
			memcpy(mcuCheckCode, pos, 4);
			OTAUPGADE("mcuCheckCode version!\n");
			printMsg(mcuCheckCode, 4);
			
			pos += 4;

			memset(mcuDownloadPath, 0, sizeof(mcuDownloadPath));
			for(i = 0; i<32; i++)
			{
				mcuDownloadPath[i] = *(pos+i);
				if(*(pos+i) == 0x00)
					break;
			}

			OTAUPGADE("mcuDownloadPath:%s\n", mcuDownloadPath);
			upgradeFlag = 3;
		}
	}
	else  //*pos == 0 不需要升级,检测mcu version
	{
		OTAUPGADE("Don not upgrade let version!\n");
		pos += 40;

		if(*pos == 1)
		{
			OTAUPGADE("upgrade mcu version!\n");
			//get mcu version
			pos = pos+1;
			memcpy(mcuVersion, pos, 3);
			OTAUPGADE("mcuVersion version!\n");
			printMsg(mcuVersion, 3);
			//get mcu upgrade check code
			pos += 3;
			memcpy(mcuCheckCode, pos, 4);
			OTAUPGADE("mcuCheckCode version!\n");
			printMsg(mcuCheckCode, 4);
			
			pos += 4;

			memset(mcuDownloadPath, 0, sizeof(mcuDownloadPath));
			for(i = 0; i<32; i++)
			{
				mcuDownloadPath[i] = *(pos+i);
				if(*(pos+i) == 0x00)
					break;
			}

			OTAUPGADE("mcuDownloadPath:%s\n", mcuDownloadPath);
			upgradeFlag = 2;
		}
		else  //*pos == 0 不需要升级
		{
			OTAUPGADE("Don not upgrade mcu version!\n");
		}
	}
		
	//get ftp ip, port, user name, password 
	if((upgradeFlag == 1) ||(upgradeFlag == 2) || (upgradeFlag == 3))
	{
		pos = pData;
		pos = pos+80+9;

		memset(FTP_IP, 0, sizeof(FTP_IP));
		for(i = 0; i<20; i++)
		{
			FTP_IP[i] = *(pos+i);
			if(*(pos+i) == 0x00)
				break;
		}
		OTAUPGADE("ftp ip:%s\n",FTP_IP);
		
		pos += 20;
		memset(port, 0, sizeof(port));
		for(i = 0; i<6; i++)
		{
			port[i] = *(pos+i);
			if(*(pos+i) == 0x00)
				break;
		}
		OTAUPGADE("ftp port:%s\n", port);

		FTP_PORT = atoi(port);
		OTAUPGADE("ftp port:%d\n", FTP_PORT);

		pos += 6;
		memset(FTP_UsrName, 0, sizeof(FTP_UsrName));
		for(i = 0; i<16; i++)
		{
			FTP_UsrName[i] = *(pos+i);
			if(*(pos+i) == 0x00)
				break;
		}
		OTAUPGADE("FTP_UsrName:%s!\n",FTP_UsrName);

		pos += 16;
		memset(FTP_Password, 0, sizeof(FTP_Password));
		for(i = 0; i<16; i++)
		{
			FTP_Password[i] = *(pos+i);
			if(*(pos+i) == 0x00)
				break;
		}
		OTAUPGADE("FTP_Password:%s!\n",FTP_Password);

	}

	return 0;
}

/*****************************************************************************
* Function Name : downloadFile
* Description   : 下载文件
* Input			: int flag,
*                 char *fileName1;
*                 char *destination1,
*                 char *fileName2, 
*                 char *destination2
* Output        : None
* Return        : 0:success, -1:failed
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int OTAUpgrade::downloadFile(int flag, char *fileName1, char *destination1,char *fileName2, char *destination2)
{
	int Pasvfd;
	int PasvPort;
	char PasvIp[32];

	memset(PasvIp, 0, sizeof(PasvIp));
	FTPClient ftpClient(FTP_IP, FTP_PORT);
	if(ftpClient.loginFTPServer(FTP_UsrName, FTP_Password) == -1)
		return -1;
	if(ftpClient.setFTPServerToPasvMode(PasvIp, &PasvPort) == -1)
		return -1;
	if(ftpClient.connectPasvServer(PasvIp, PasvPort, &Pasvfd) != -1)
	{
		if((flag == 1) && (fileName1 != NULL) && (destination1 != NULL))
		{
			OTAUPGADE("download lte version fileName1:%s, destination1:%s\n",fileName1, destination1);
			if(ftpClient.getFile(Pasvfd, fileName1, destination1) == 0)
			{
				if(compareCrc32CheckCode(destination1, lteCheckCode) != 0)
					system(DELETE_LTE_FILE);
			}

			ftpClient.quitFTP();
		}
		else if((flag == 2) && (fileName2 != NULL) && (destination2 != NULL))
		{
			OTAUPGADE("download mcu version fileName2:%s, destination2:%s\n",fileName2, destination2);
			if(ftpClient.getFile(Pasvfd, fileName2, destination2) == 0)
			{
				if(compareCrc32CheckCode(destination2, mcuCheckCode) != 0)
					system(DELETE_MCU_FILE);
			}
			
			ftpClient.quitFTP();
		}
		else if((flag == 3) && (fileName1 != NULL) && (destination1 != NULL) && (fileName2 != NULL) && (destination2 != NULL))
		{
			OTAUPGADE("download all version !\n");
			if(ftpClient.getFile(Pasvfd, fileName1, destination1) == 0)
			{
				if(compareCrc32CheckCode(destination1, lteCheckCode) != 0)
					system(DELETE_LTE_FILE);
			}

			if(ftpClient.getFile(Pasvfd, fileName2, destination2) == 0)
			{
				if(compareCrc32CheckCode(destination2, mcuCheckCode) != 0)
					system(DELETE_MCU_FILE);
			}

			ftpClient.quitFTP();
		}
		else
		{
			return -1;
		}
	}

	return 0;
}


