#ifndef _SELF_CHECK_H_
#define _SELF_CHECK_H_
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
#include <pthread.h>



#ifndef OTA_DEBUG
	#define OTA_DEBUG 1
#endif

#if OTA_DEBUG
	#define OTAUPGADE(format,...) printf("### Check ### %s,%s [%d] "format"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
#else	
	#define OTAUPGADE(format,...)
#endif




#if 0	
#define Lte_Module_STATUS         "LteModuleStatus:"
#define Lte_Antenna_STATUS        "LteAntennaStatus:"
#define USIM_STATUS               "USIMStatus:"
#define AirBagBusInput_STATUS     "AirBagBusInputStatus:"
#define Emmc_STATUS               "EmmcStatus:"
#define CanCommunication_STATUS   "CanCommunicationStatus:"
#define IVICommunication_STATUS   "IVICommunicationStatus:"
#define PEPSCommunication_STATUS  "PEPSCommunicationStatus:"
#endif

#define Lte_Module_STATUS         "主通信模块的工作状态:"
#define Lte_Antenna_STATUS        "主通信天线的连接状态:"
#define USIM_STATUS               "USIM卡的工作状态:"
#define AirBagBusInput_STATUS     "安全气囊硬线输入状态:"
#define Emmc_STATUS               "存储器的工作状态:"
#define CanCommunication_STATUS   "CAN通信状态:"
#define IVICommunication_STATUS   "IVI通信状态:"
#define PEPSCommunication_STATUS  "PEPS通信状态:"

	

#define SELF_CHEK_FILE   "./selfCheckResult"


typedef struct
{
	int LteModuleTrigger;
	int LteAntennaTrigger;
	int USimTrigger;
	int AirBagBusInputTrigger;
	int EmmcTrigger;
	int CanCommunicationTrigger;
	int IVICommunicationTrigger;
	int PEPSCommunicationTrigger;

}DetectionTrigger;






class SelfCheck
{
public:
	SelfCheck();
	~SelfCheck();
	int selfCheckInit();
	static void *LteModuleStatusCheck(void *args);
	static void *LteAntennaStatusCheck(void *args);
	static void *USIMStatusCheck(void *args);
	static void *AirBagBusInputStatusCheck(void *args);
	static void *EmmcStatusCheckThread(void *args);
	static void *CanCommunicationStatusCheck(void *args);
	static void *IVICommunicationStatusCheck(void *args);
	static void *PEPSCommunicationStatusCheck(void *args);
	int EmmcStatusCheck();

	//file operation
	int fileInit();
	int initParameterTable(FILE *fp);
	/***************************************************************************
	* Function Name: updateParameter
	* Auther	   : yingaoguo
	* Date		   : 2017-11-17
	* Description  : update file content
	* Input 	   : paraId: parameter ID, 
	*                        id = 1  update  Lte_Module_STATUS
	*                        id = 2  update  Lte_Antenna_STATUS
	*                        id = 3  update  USIM_STATUS
	*                        id = 4  update  AirBagBusInput_STATUS
	*                        id = 5  update  Emmc_STATUS
	*                        id = 6  update  CanCommunication_STATUS
	*                        id = 7  update  IVICommunication_STATUS
	*                        id = 8  update  PEPSCommunication_STATUS
	*                content: update content
	* Output	   : None
	* Return	   : 0:success, -1:failed
	****************************************************************************/
	int updateParameter(int paraId, char *content);
	int fileTest();
	void printMsg(uint8_t *pData, uint16_t len);

};

extern TBoxDataPool *dataPool;



#endif
