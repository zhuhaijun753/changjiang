#ifndef _OTAWIFI_H_
#define _OTAWIFI_H_
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
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/prctl.h> 

#include "mcuUart.h"
#include "TBoxDataPool.h"
#include "LTEModuleAtCtrl.h"
#include "LedControl.h"
#include "VoiceCall.h"
#include "simcom_common.h"
#include "WiFiControl.h"
#include "Message.h"
#include "DataCall.h"
#include "WDSControl.h"
#include "NASControl.h"
#include "dsi_netctrl.h"


#define TBOX_DEBUG_EN   1

#if TBOX_DEBUG_EN
	#define EPOLL_DEBUG_EN  1
#endif


#if EPOLL_DEBUG_EN
	#define EPOLL_LOG(format,...) printf("### EPOLL ### %s, %s[%d] "format"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
	#define EPOLL_ERROR(format,...) fprintf(stderr, "### EPOLL ### %s, %s[%d] "format"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
	#define EPOLL(format,...) printf(format,##__VA_ARGS__)
#else
	#define EPOLL_LOG(format,...)
	#define EPOLL_ERROR(format,...)
	#define EPOLL(format,...)
#endif



#define LTE_FILE_NAME         "/data/LteUpgrade.bin"
#define RM_LTE_FILE           "rm -rf /data/LteUpgrade.bin"
#define MCU_FILE_NAME         "/data/MCU.bin"
#define RM_MCU_FILE           "rm -rf /data/MCU.bin"

#define SERVER_LISTEN_PORT    10000
#define SERVER_WIFI_IP        "192.168.100.1"
#define LISTEN_BACKLOG        1
#define MAX_EVENT_NUMBER      10
#define BUFFER_SIZE           1024+20
#define DATA_BUFF_LEN         100




typedef struct{
	uint8_t updataType; 
	uint8_t mcuCheckSum;
	uint32_t mcuFileSize;
	uint8_t tboxCheckSum;
	uint32_t tboxFileSize;
}UpgradeFileInfo;




class OTAWiFi
{
public:
    OTAWiFi();
    ~OTAWiFi();
    int socketConnect();
    int disconnectSocket();
    uint8_t checksum(uint8_t *pData, uint32_t len);
	uint8_t checkPackDatasum(uint8_t *pData, uint32_t len);
    int8_t unpack_WifiData(uint8_t *, int32_t );
    void packetDataApp(uint8_t, uint8_t, uint8_t);
    static void *serverThread(void *arg);
    void et_process(struct epoll_event * events, int number, int epoll_fd, int socketfd);
    int set_non_block(int fd);
    void add_socketFd(int epoll_fd, int fd);
	int storageFile(uint8_t *fileName, uint8_t *pData, int32_t len);
	int check_file_result(char *fileName, uint8_t fileCheckCode, uint16_t *crc16Check);
	unsigned int Crc16Check(unsigned char* pData, uint32_t len);

private:
    int sockfd;
    int accept_fd;
	bool isExsistMcuFile;
	bool isExsistTboxFile;
    UpgradeFileInfo u_fileInfo;
    struct sockaddr_in serverAddr;

};

extern LTEModuleAtCtrl *LteAtCtrl;




extern unsigned long get_file_size(const char *path);
extern unsigned int Crc16Check(unsigned char *pData, unsigned int len);
extern int calculate_files_CRC(char *fileName, unsigned int *crc);
extern int check_mcu_file_is_exist();


#endif

