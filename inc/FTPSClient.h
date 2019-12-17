#ifndef _FTPS_CLIENT_H_
#define _FTPS_CLIENT_H_
#include <stdio.h>
#include <string.h>
#include <stdint.h>
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
#include <openssl/ssl.h>
#include <openssl/err.h>




#ifndef FTPS_DEBUG
	#define FTPS_DEBUG  1
#endif
#if FTPS_DEBUG
	#define FTPSLOG(format,...) printf("### FTPLOG ### %s,%s [%d] "format"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
	#define FTPS_NOLOG(format,...) printf(format,##__VA_ARGS__)
#else
	#define FTPSLOG(format,...)
	#define FTPS_NOLOG(format,...)
#endif



#define MAX_BUFF_LEN      512


class FTPSClient
{
public:
	FTPSClient();
	~FTPSClient();
	FTPSClient(char *ip, int port);
	int connectToFtpServer();
	int FTPSRequest(char *authority, char *transportLayerSecurity);
	void showCerts(SSL *ssl);
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
	SSL_CTX *ctx;
	SSL *ssl;

};







#endif

