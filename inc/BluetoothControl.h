#ifndef _BLUETOOTH_CONTROL_H_
#define _BLUETOOTH_CONTROL_H_
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



#ifndef BT_DEBUG
	#define BT_DEBUG  1
#endif
#if BT_DEBUG
	#define BTLOG(format,...) printf("### BT ### %s,%s [%d] "format"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
	#define BT_NOLOG(format,...) printf(format,##__VA_ARGS__)
#else
	#define BTLOG(format,...)
	#define BT_NOLOG(format,...)
#endif





class BTControl
{
public:
	BTControl();
	~BTControl();

	uint8_t BT_Ctrl_OpenOrClose(uint8_t isOpen);
	uint8_t BT_Ctrl_Mode(uint8_t mode);
	uint8_t BT_STA_SetSSID(char *ssid, uint32_t len = 0);
	uint8_t BT_STA_SetAuth(uint8_t auth, char* pwd);

	uint8_t BT_AP_SetSSID(char *ssid, uint32_t len);
	uint8_t BT_AP_SetAuth(uint8_t auth, char* pwd);



private:


};







#endif

