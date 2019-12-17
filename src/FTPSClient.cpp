#include "FTPSClient.h"

//FTPSClient::FTPSClient(){}

/*****************************************************************************
* Function Name : ~FTPSClient
* Description   : 析构函数
* Input			: None
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
FTPSClient::~FTPSClient()
{
	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(socketfd);
	SSL_CTX_free(ctx);
}

/*****************************************************************************
* Function Name : FTPSClient
* Description   : 构造函数
* Input			: char *ip
*                 int port
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
FTPSClient::FTPSClient(char *ip, int port)
{
	socketfd = -1;
	ftpServerIP = ip;
	ftpServerPort = port;
	isConnected = false;
	isReceivedData = false;

	/* SSL库初始化 */
	SSL_library_init();
	/* 载入所有SSL算法 */
	OpenSSL_add_all_algorithms();
	/* 载入所有SSL错误消息 */
	SSL_load_error_strings();
	/* 以SSL V2和V3标准兼容方式产生一个SSL_CTX，即SSL Content Text */
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		ERR_print_errors_fp(stdout);
		exit(1);
	}

	connectToFtpServer();

	while(!isReceivedData)
	{
		if(isConnected == false)
			connectToFtpServer();

		sleep(5);
	}
}

/*****************************************************************************
* Function Name : connectToFtpServer
* Description   : 连接到FTP服务器
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int FTPSClient::connectToFtpServer()
{
	int ftpCode;
	uint8_t buff[MAX_BUFF_LEN] = {0};
	memset(&socketaddr, 0, sizeof(socketaddr));
	socketaddr.sin_family = AF_INET;
	socketaddr.sin_port = htons(ftpServerPort);
	socketaddr.sin_addr.s_addr = inet_addr(ftpServerIP);

	if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) != -1)
	{
		FTPSLOG("socketfd:%d\n",socketfd);
		if (connect(socketfd, (struct sockaddr*) &socketaddr, sizeof(socketaddr)) != -1)
		{
			FTPSLOG("Connect to server successfully,server IP:%s,PORT:%d\n",ftpServerIP,ftpServerPort);
			isConnected = true;

			if(recv(socketfd, buff, MAX_BUFF_LEN, 0) > 0)
			{
				FTPSLOG("%s",buff);
				isReceivedData = true;

				/*sscanf((char *)buff,"%d", &ftpCode);
				FTPSLOG("ftpCode:%d\n", ftpCode);

				if(ftpCode != 220)
				{
					FTPSLOG("220 connect is error!");
					return -1;
				}*/
			}
			
			/* 基于ctx产生一个新的SSL */
			ssl = SSL_new(ctx);
			SSL_set_fd(ssl, socketfd);
			/* 建立SSL连接 */
			if (SSL_connect(ssl) == -1)
				ERR_print_errors_fp(stderr);
			else
			{
				printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
				showCerts(ssl);
			}			
		}
		else
		{
			close(socketfd);
			shutdown(socketfd, SHUT_RDWR);
			isConnected = false;
			FTPSLOG("connect failed close socketfd!");
			return -1;
		}
	}

	return 0;
}

/*****************************************************************************
* Function Name : FTPSRequest
* Description   : 发送Auth及TLS到FTP服务器
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int FTPSClient::FTPSRequest(char *authority, char *transportLayerSecurity)
{
	int length;
	int ftpCode;
	char buff[MAX_BUFF_LEN] = {0};
	char *ptr = NULL;
	fd_set fds;
	struct timeval timeout;// = {0, 0};
	timeout.tv_sec	= 5;
	timeout.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(socketfd, &fds);

	sprintf(buff, "%s %s\r\n", authority, transportLayerSecurity);

	length = send(socketfd, buff, strlen(buff), 0);
	if (length < 0)
	{
		perror("Error sending authority command to server.");
		return -1;
	}
	FTPSLOG("Send authority OK, length:%d",length);

	switch(select(socketfd+1, &fds, NULL, NULL, &timeout))
	{
		case -1:
			FTPSLOG("Select error.\n");
			return -1;
		case 0:
			FTPSLOG("Select timeout times.\n");
			return -1;
		default:
			if(FD_ISSET(socketfd, &fds))
			{
				memset(buff, 0, MAX_BUFF_LEN);
				length = recv(socketfd, buff, MAX_BUFF_LEN, 0);
				if (length < 0)
				{
					perror("Error recv.");
					return -1;
				}

				FTPSLOG("recv data:%s, length:%d",buff,length);
			}
	}

	return 0;
}

/*****************************************************************************
* Function Name : showCerts
* Description   : 显示数字证书信息
* Input			: SSL *ssl
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void FTPSClient::showCerts(SSL *ssl)
{
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL)
    {
        printf("数字证书信息:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("证书: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("颁发者: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("无证书信息！\n");
}

/*****************************************************************************
* Function Name : loginFTPServer
* Description   : 登录到FTP服务器
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int FTPSClient::loginFTPServer(char *userName, char *password)
{
	if(sendUserName(userName) < 0)
	{
		FTPSLOG("send user name error!\n");
		return -1;
	}
	
	if(sendPassword(password) < 0)
	{
		FTPSLOG("send pass word error!\n");
		return -1;
	}

	return 0;
}

/*****************************************************************************
* Function Name : sendUserName
* Description   : 发送用户名到FTP服务器
* Input			: char *userName
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int FTPSClient::sendUserName(char *userName)
{
	int length;
	int ftpCode;
	char buff[MAX_BUFF_LEN] = {0};
	char *ptr = NULL;
	fd_set fds;
	struct timeval timeout;// = {0, 0};
	timeout.tv_sec	= 5;
	timeout.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(socketfd, &fds);

	sprintf(buff, "USER %s\r\n", userName);

	length = send(socketfd, buff, strlen(buff), 0);
	if (length < 0)
	{
		perror("Error sending sendUserName command to server.");
		return -1;
	}
	FTPSLOG("Send sendUserName OK, length:%d",length);

	switch(select(socketfd+1, &fds, NULL, NULL, &timeout))
	{
		case -1:
			FTPSLOG("Select error.\n");
			return -1;
		case 0:
			FTPSLOG("Select timeout times.\n");
			return -1;
		default:
			if(FD_ISSET(socketfd, &fds))
			{
				memset(buff, 0, MAX_BUFF_LEN);
				length = recv(socketfd, buff, MAX_BUFF_LEN, 0);
				if (length < 0)
				{
					perror("Error recv.");
					return -1;
				}
				/* recv server's code and info. eg:331 User name okay, need password.*/
				FTPSLOG("recv data:%s, length:%d",buff,length);
				
				sscanf(buff,"%d", &ftpCode);
				FTPSLOG("ftpCode:%d\n", ftpCode);

				if(ftpCode != 331)
				{
					FTPSLOG("The User does not have enough permissions!");
					return -1;
				}
			}
	}

	return 0;
}

/*****************************************************************************
* Function Name : sendUserName
* Description   : 发送用户密码到FTP服务器
* Input			: char *password
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int FTPSClient::sendPassword(char *password)
{
	int length;
	int ftpCode;
	char buff[MAX_BUFF_LEN] = {0};
	fd_set fds;
	struct timeval timeout;// = {0, 0};
	timeout.tv_sec	= 5;
	timeout.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(socketfd, &fds);

	sprintf(buff, "PASS %s\r\n", password);

	length = send(socketfd, buff, strlen(buff), 0);
	if (length < 0)
	{
		perror("Error sending sendPassword command to server");
		return -1;
	}
	FTPSLOG("Send data OK, length:%d",length);

	switch(select(socketfd+1, &fds, NULL, NULL, &timeout))
	{
		case -1:
			FTPSLOG("Select error.\n");
			return -1;
		case 0:
			FTPSLOG("Select timeout times.\n");
			return -1;
		default:
			if(FD_ISSET(socketfd, &fds))
			{
				memset(buff, 0, MAX_BUFF_LEN);
				length = recv(socketfd, buff, MAX_BUFF_LEN, 0);
				if (length < 0)
				{
					perror("Error recv.");
					return -1;
				}
				/* recv server's code and info. eg:230 User logged in, proceed.*/
				FTPSLOG("recv data:%s, length:%d",buff,length);
				
				sscanf(buff,"%d", &ftpCode);
				FTPSLOG("ftpCode:%d\n", ftpCode);

				if(ftpCode != 230)
				{
					FTPSLOG("password error!");
					return -1;
				}
			}
	}

	return 0;
}

/*****************************************************************************
* Function Name : setFTPServerToPasvMode
* Description   : 设置FTP服务器为被动模式
* Input			: char *PASV_IP
*                 int *PASV_Port
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int FTPSClient::setFTPServerToPasvMode(char *PASV_IP, int *PASV_Port)
{
	int length;
	int ftpCode;
	char buff[MAX_BUFF_LEN] = {0};
	fd_set fds;
	struct timeval timeout;// = {0, 0};
	timeout.tv_sec	= 5;
	timeout.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(socketfd, &fds);

	sprintf(buff, "PASV\r\n");

	length = send(socketfd, buff, strlen(buff), 0);
	if (length < 0)
	{
		perror("Error sending setFTPServerToPasvMode command to server");
		return -1;
	}
	FTPSLOG("Send data OK, length:%d",length);

	switch(select(socketfd+1, &fds, NULL, NULL, &timeout))
	{
		case -1:
			FTPSLOG("Select error.\n");
			return -1;
		case 0:
			FTPSLOG("Select timeout times.\n");
			return -1;
		default:
			if(FD_ISSET(socketfd, &fds))
			{
				memset(buff, 0, MAX_BUFF_LEN);
				length = recv(socketfd, buff, MAX_BUFF_LEN, 0);
				if (length < 0)
				{
					perror("Error recv.");
					return -1;
				}
				/* 227 Entering passive mode.*/
				FTPSLOG("recv data:%s, length:%d",buff,length);
				
				sscanf(buff,"%d", &ftpCode);
				FTPSLOG("ftpCode:%d\n", ftpCode);

				if(ftpCode != 227)
				{
					FTPSLOG("Set passive mode failed!");
					return -1;
				}
				else
				{
					getPasvModeIpAndPort(buff, length, PASV_IP, PASV_Port);
				}
			}
	}

	return 0;
}

/*****************************************************************************
* Function Name : setFTPServerToPasvMode
* Description   : 以被动模式连接FTP
* Input			: char *ip
*                 int port
*                 int *pasvSockfd
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int FTPSClient::connectPasvServer(char *ip, int port, int *pasvSockfd)
{
	int sockfd;
	struct sockaddr_in serveraddr;
	uint8_t buff[MAX_BUFF_LEN] = {0};
	memset(&serveraddr, 0, sizeof(serveraddr));

	FTPSLOG("ip:%s, port:%d\n", ip, port);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = inet_addr(ip);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) != -1)
	{
		*pasvSockfd = sockfd;
		FTPSLOG("sockfd:%d, *pasvSockfd: %d\n",sockfd, *pasvSockfd);
		
		if (connect(sockfd, (struct sockaddr*) &serveraddr, sizeof(sockaddr_in)) != -1)
		{
			FTPSLOG("Connect to pasv server,IP:%s,PORT:%d\n",ip,port);
			setFTPToBinaryTransferMode();
		}
		else
		{
			close(sockfd);
			shutdown(sockfd, SHUT_RDWR);
			FTPSLOG("connect failed close socketfd!");
			return -1;
		}
	}

	return 0;
}

/*****************************************************************************
* Function Name : getPasvModeIpAndPort
* Description   : 获得被动模式IP和端口
* Input			: char *pData
*                 int len
*                 char *PasvIp
*                 int *PasvPort
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int FTPSClient::getPasvModeIpAndPort(char *pData, int len, char *PasvIp, int *PasvPort)
{ 
    int i;
	int count = 0;
	char *pos = pData;
	char tmp1[4] = {0};
	char tmp2[4] = {0};
	int port1;
	int port2;
	
	//example: char str[] = "227 Entering Passive Mode (210,74,1,30,118,26)";

	while(1)
	{
		if(*pos == '(')
		{
			break;
		}
		pos++;
	}
	
	pos = pos+1;
	for(i = 0; i<len; i++)
	{
		if(*pos == ',') 
		{ 
			count++;
			if(count == 4)
			{
				break;
			}
			PasvIp[i] = '.';
		}
		else
		{
			PasvIp[i] = *pos;
		}
		pos++;
	}
	
	pos += 1;
	for(i = 0; i<len; i++)
	{
		if(*pos == ',') 
		{
			port1 = atoi(tmp1);
			break;
		}
		tmp1[i] = *pos;
		pos++;
	}
	
   	pos += 1;
	for(i = 0; i<len; i++)
	{
		if(*pos == ')') 
		{
			port2 = atoi(tmp2);
			break;
		}
		tmp2[i] = *pos;
		pos++;
	}

	*PasvPort = port1*256+port2;
	FTPSLOG("port1:%d, port2:%d\n", port1, port2);
	FTPSLOG("ip:%s, port:%d\n", PasvIp, *PasvPort);
	
    return 0; 
}

/*****************************************************************************
* Function Name : setFTPToBinaryTransferMode
* Description   : 设置FTP为二进制传输模式
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int FTPSClient::setFTPToBinaryTransferMode()
{
	int length;
	int ftpCode;
	char buff[MAX_BUFF_LEN] = {0};
	fd_set fds;
	struct timeval timeout;// = {0, 0};
	timeout.tv_sec	= 5;
	timeout.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(socketfd, &fds);

	sprintf(buff, "TYPE I\r\n");

	length = send(socketfd, buff, strlen(buff), 0);
	if (length < 0)
	{
		perror("Error sending setFTPToBinaryTransferMode command to server");
		return -1;
	}
	FTPSLOG("Send data OK, length:%d",length);

	switch(select(socketfd+1, &fds, NULL, NULL, &timeout))
	{
		case -1:
			FTPSLOG("Select error.\n");
			return -1;
		case 0:
			FTPSLOG("Select timeout times.\n");
			return -1;
		default:
			if(FD_ISSET(socketfd, &fds))
			{
				memset(buff, 0, MAX_BUFF_LEN);
				length = recv(socketfd, buff, MAX_BUFF_LEN, 0);
				if (length < 0)
				{
					perror("Error recv.");
					return -1;
				}
				FTPSLOG("recv data:%s, length:%d",buff,length);
				
				sscanf(buff,"%d", &ftpCode);
				FTPSLOG("ftpCode:%d\n", ftpCode);

				if(ftpCode != 200)
				{
					FTPSLOG("type recv is error!\n");
					return -1;
				}
				else
				{
					FTPSLOG("use binary mode to transmission data.\n");
				}
			}
	}

	return 0;
}

/*****************************************************************************
* Function Name : getFile
* Description   : 下载文件
* Input			: int Pasvfd
*                 char *fileName
*                 char *destination
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int FTPSClient::getFile(int Pasvfd, char *fileName, char *destination)
{
	int fd;
	int length;
	int ftpCode;
	char buff[MAX_BUFF_LEN] = {0};
	fd_set fds;
	struct timeval timeout;// = {0, 0};
	timeout.tv_sec	= 5;
	timeout.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(socketfd, &fds);

	sprintf(buff, "RETR %s\r\n",fileName);

	length = send(socketfd, buff, strlen(buff), 0);
	if (length < 0)
	{
		perror("Error sending getFile command to server");
		return -1;
	}
	FTPSLOG("Send data OK, length:%d",length);

	switch(select(socketfd+1, &fds, NULL, NULL, &timeout))
	{
		case -1:
			FTPSLOG("Select error.\n");
			return -1;
		case 0:
			FTPSLOG("Select timeout times.\n");
			return -1;
		default:
			if(FD_ISSET(socketfd, &fds))
			{
				memset(buff, 0, MAX_BUFF_LEN);
				length = recv(socketfd, buff, MAX_BUFF_LEN, 0);
				if (length < 0)
				{
					perror("Error recv.");
					return -1;
				}
				/* 227 Entering passive mode.*/
				FTPSLOG("recv data:%s, length:%d",buff,length);
				
				sscanf(buff,"%d", &ftpCode);
				FTPSLOG("ftpCode:%d\n", ftpCode);

				if(ftpCode != 150)
				{
					FTPSLOG("can get file!");
					return -1;
				}

				if(0 > (fd = open(destination, O_CREAT|O_TRUNC|O_WRONLY,S_IRUSR|S_IWUSR)))
				{
					perror("open");
					close(Pasvfd);
					return -1;
				}

				SSL *pasvSSL = SSL_new(ctx);       /* create new SSL connection state */
				SSL_set_fd(pasvSSL, Pasvfd); /* attach the socket descriptor */
				if (SSL_connect(pasvSSL) == -1)	   /* perform the connection */
					ERR_print_errors_fp(stderr);

				while(1)
				{
					memset(buff, '\0', MAX_BUFF_LEN);
					length = recv(Pasvfd, buff, sizeof(buff), 0);
					if(length > 0)
					{
						write(fd, buff, length);
					}
					
					if(length <= 0)
					{
						shutdown(Pasvfd, SHUT_WR);
						close(Pasvfd);
						close(fd);
						SSL_shutdown(pasvSSL);
						SSL_free(pasvSSL);
						break;
					}
				}

				memset(buff, 0, MAX_BUFF_LEN);
				length = recv(socketfd, buff, MAX_BUFF_LEN, 0);
				if (length < 0)
				{
					perror("Error recv.");
					return -1;
				}
				/* 226 Entering passive mode.*/
				FTPSLOG("recv data:%s, length:%d",buff,length);
				
				sscanf(buff,"%d", &ftpCode);
				FTPSLOG("ftpCode:%d\n", ftpCode);
			}
	}

	return 0;
}

/*****************************************************************************
* Function Name : quitFTP
* Description   : 下载文件
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int FTPSClient::quitFTP()
{
	int length;
	char buff[MAX_BUFF_LEN] = {0};

	sprintf(buff, "QUIT\r\n");

	length = send(socketfd, buff, (int)strlen(buff), 0);
	if (length < 0)
	{
		perror("Error sending command to server");
		return -1;
	}
	FTPSLOG("Send data OK, length:%d",length);

	memset(buff, 0, MAX_BUFF_LEN);

	if(recv(socketfd, buff, sizeof(buff), 0 ) < 0)
	{
		perror("Error: can't recv");
		return -1;
	}
	else 
		printf("%s",buff);

	//close(socketfd);
	this->~FTPSClient();
	
	return 0;
}


