#ifndef _FTPCLIENT_H_
#define _FTPCLIENT_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#include <time.h>
#include "common.h"



#ifndef FTP_DEBUG
	#define FTP_DEBUG  1
#endif
#if FTP_DEBUG
	#define FTPLOG(format,...) printf("### FTPLOG ### %s,%s [%d] "format"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
	#define FTP_NOLOG(format,...) printf(format,##__VA_ARGS__)
#else
	#define FTPLOG(format,...)
	#define FTP_NOLOG(format,...)
#endif



#define MAX_BUFF_LEN      512


class FTPClient
{
public:
	FTPClient();
	~FTPClient();
	FTPClient(char *ip, int port);
	int connectToFtpServer();
	int loginFTPServer(char *userName, char *password);
	int sendUserName(char *userName);
	int sendPassword(char *password);
	
	/***************************************************************************
	* Function Name: setFTPServerToPasvMode
	* Auther	   : yingaoguo
	* Date		   : 2017-11-2
	* Description  : set ftp server to passive mode
	* Input 	   : PASV_IP:   output ftp server pasv mode ip
	*                PASV_Port: output ftp server pasv mode port.
	* Output	   : None
	* Return	   : 0:success, -1:failed
	****************************************************************************/
	int setFTPServerToPasvMode(char *PASV_IP, int *PASV_Port);
	int getPasvModeIpAndPort(char *pData, int len, char *PasvIp, int *PasvPort);
	int connectPasvServer(char *ip, int port, int *pasvSockfd);
	int setFTPToBinaryTransferMode();
	int getFile(int Pasvfd, char *fileName, char *destination);
	int quitFTP();


private:
	int socketfd;
	bool isConnected;
	bool isReceivedData;
	struct sockaddr_in socketaddr;
	char *ftpServerIP;
	int ftpServerPort;

};







#endif

