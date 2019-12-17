#ifndef _CLOUDSOCKET_H_
#define _CLOUDSOCKET_H_
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "common.h"
#include "TBoxDataPool.h"
#include "FTPClient.h"
#include "Crc32Count.h"

/*add by qiu------------------*/
#include <syscall.h>
#include <sys/ioctl.h>
#include <linux/reboot.h>
/*-----------------------------------*/

#ifndef OTA_DEBUG
	#define OTA_DEBUG 1
#endif

#if OTA_DEBUG
	#define OTAUPGADE(format,...) printf("### OTA ### %s,%s [%d] "format"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
#else	
	#define OTAUPGADE(format,...)
#endif



#define POTOCAL_HEAD          0x7E7E7E7E
#define GID                   0x01
#define ADDR                  0x01
#define CID                   0xFE
//#define POTOCAL_END         0x0D0A

#define DATA_LEN              1024

#define LTE_FILE_NAME         "/data/LteUpgrade.bin"
#define DELETE_LTE_FILE       "rm -rf /data/LteUpgrade.bin"
#define MCU_FILE_NAME         "/data/MCU.bin"
#define DELETE_MCU_FILE       "rm -rf /data/MCU.bin"





class OTAUpgrade
{
public:
	OTAUpgrade();
	~OTAUpgrade();
	int socketConnect(void);
	int disSocketConnect(void);
	int sendVersionInfo();
	uint16_t packVersionInfo(uint8_t *dataBuff, uint16_t len);
	uint8_t checkXOR(uint8_t *pData, uint16_t len);
	void printMsg(uint8_t *pData, uint16_t len);
	int checkReceiveData(uint8_t *pData, uint32_t size);
	int unpack_protocol(uint8_t *pData, uint16_t len);
	int downloadFile(int flag, char *fileName1, char *destination1,char *fileName2, char *destination2);

public:
	uint8_t mcuVersion[3];
	uint8_t lteVersion[3];
	uint8_t mcuCheckCode[4];
	uint8_t lteCheckCode[4];
	char mcuDownloadPath[32];
	char lteDownloadPath[32];

	char FTP_IP[20];
	uint16_t FTP_PORT;
	char FTP_UsrName[16];
	char FTP_Password[16];

private:
	int socketfd;
	bool isConnected;
	bool isReceivedData;
	int upgradeFlag;
	struct sockaddr_in socketaddr;

};

extern TBoxDataPool *dataPool;



#endif
