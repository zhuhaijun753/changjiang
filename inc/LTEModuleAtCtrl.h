#ifndef _LTE_NETWORK_MANAGE_H_
#define _LTE_NETWORK_MANAGE_H_
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include "common.h"
#include "TBoxDataPool.h"
#include "NetworkConnection.h"
#include "LedControl.h"



#define QUEC_AT_PORT    "/dev/smd8"
#define ARRAY_SIZE      100




class LTEModuleAtCtrl
{
public:
	LTEModuleAtCtrl();
	~LTEModuleAtCtrl();
	int atCtrlInit();
	int NetworkInit();
	static void *timingTaskThread(void *args);
	
	/***************************************************************************
	* Function Name: sendATCmd 					
	* Function Introduction: send at command to LTE module
	* Parameter description:
	*     atCmd    : at command
	*     finalRsp : at command's response result.
	*     buff     : if need function return result, you should pass the buff,
	*                then the function will store the result into buff 
	*     buffSize : buff size, At least 100bits.
	*     timeout_ms: time out
	* Function return value:  -1: failed; 0: return error,
	*                         other: return data length.
	* Data : 2017.09.08									
	****************************************************************************/
	int sendATCmd(char* atCmd, char* finalRsp, char *buff, uint32_t buffSize, long timeout_ms);
	int getLTEModuleRevision(char *pBuff, int size);
	int queryIsInsertSIM();
	int queryCSQ(uint8_t *rssi, uint8_t *ber);
	int queryReg();
	int emmcDeviceCheck();
	int emmcPartitionCheck();
	int switch9011();
	int atReset();
	int checkEmmcMountFolderCanBeWrite();
	int cfdisk(int partitionNum, int partitionSize);
	int switchCwiic();//test  call micout
	int voiceLoopBack();
	int switchVoiceChannel();
	int getICCID(char *pBuff, int size);
	int getIMEI(char *pBuff, int size);
	int getCIMI(char *pBuff, int size);
	int callPhone(char *phoneNumber);
	int answerOrHangUpPhone(bool isConnect);

	/***************************************************************************
	* Function Name: sendSMS 					
	* Function Introduction: send message
	* Parameter description:
	*     phoneNum    : phone number
	*     pData       : the message
	*     datalen     : message size
	*     SMSMode     : 0 PDU mode; 1 Text mode
	* Function return value:  -1: failed; 0: return error,
	*                         other: return data length.
	* Data : 2017.09.08									
	****************************************************************************/
	int sendSMS(char *phoneNum, uint8_t *pData, uint32_t datalen, unsigned char SMSMode);
	int audioPlayTTS(char *tts);
	int switchCodec();//关闭codec
	//静音设置 0【关闭】，1【静音】
	int switchVmute(int muteflag);
	int switchCSDVC1();
	int switchCSDVC3();
	//开启/关闭SPk: 0【关闭】，1【开启】
	int switchSPK(int spkflag);
	//步进开启/关闭SPk: 0【关闭】，1【开启】
	int switch_stepSPK(int spkflag);
	int CallMute(void);
	//check status
	int getNetworkingType();

	int sys_mylog(char *plog);

private:
	int fd;
	pthread_t timingThreadId; 


};

extern void networkConnectionStatus(int result);
extern TBoxDataPool *dataPool;
extern TBoxNetworkStatus networkStatus;


#endif


