#include "SelfCheck.h"


//TBoxDetectionTime detectionTime;
DetectionTrigger trigger;




//SelfCheck::~SelfCheck(){}


/*****************************************************************************
* Function Name : SelfCheck
* Description   : 构造函数
* Input			: None
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
SelfCheck::SelfCheck()
{
	//trigger initialization
	trigger.LteModuleTrigger = 0;
	trigger.LteAntennaTrigger = 0;
	trigger.USimTrigger = 0;
	trigger.AirBagBusInputTrigger = 0;
	trigger.EmmcTrigger = 0;
	trigger.CanCommunicationTrigger = 0;
	trigger.IVICommunicationTrigger = 0;
	trigger.PEPSCommunicationTrigger= 0;
}

/*****************************************************************************
* Function Name : LteModuleStatusCheck
* Description   : LTE检测线程
* Input			: void *args
* Output        : None
* Return        : 0
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void *SelfCheck::LteModuleStatusCheck(void *args)
{
	pthread_detach(pthread_self());
	SelfCheck *pSelfCheck = (SelfCheck *)args;

	while(1)
	{
		sleep(TBoxPara.detectionTime.LteModuleDetectionTime);
	}

	return (void *)0;
}

/*****************************************************************************
* Function Name : LteAntennaStatusCheck
* Description   : 天线检测线程
* Input			: void *args
* Output        : None
* Return        : 0
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void *SelfCheck::LteAntennaStatusCheck(void *args)
{
	pthread_detach(pthread_self());
	SelfCheck *pSelfCheck = (SelfCheck *)args;

	while(1)
	{
		sleep(TBoxPara.detectionTime.LteAntennaDetectionTime);
	}

	return (void *)0;
}

/*****************************************************************************
* Function Name : USIMStatusCheck
* Description   : sim卡检测线程
* Input			: void *args
* Output        : None
* Return        : 0
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void *SelfCheck::USIMStatusCheck(void *args)
{
	pthread_detach(pthread_self());
	SelfCheck *pSelfCheck = (SelfCheck *)args;

	while(1)
	{
		sleep(TBoxPara.detectionTime.USimDetectionTime);
	}

	return (void *)0;
}

/*****************************************************************************
* Function Name : AirBagBusInputStatusCheck
* Description   : 安全汽囊检测线程
* Input			: void *args
* Output        : None
* Return        : 0
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void *SelfCheck::AirBagBusInputStatusCheck(void *args)
{
	pthread_detach(pthread_self());
	SelfCheck *pSelfCheck = (SelfCheck *)args;
	int ret;

	while(1)
	{
		if(trigger.AirBagBusInputTrigger == 1)
		{
			//ret = pSelfCheck->EmmcStatusCheck();

			if(ret == 0)
			{
				pSelfCheck->updateParameter(4, (char *)"故障");
			}
			else if(ret == 1)
			{
				pSelfCheck->updateParameter(4, (char *)"正常");
			}
			else //-1
			{
				printf("Emmc check eoor!");
			}
			//sleep(detectionTime.AirBagBusInputDetectionTime);

			trigger.AirBagBusInputTrigger = 0;
		}
		else if(trigger.AirBagBusInputTrigger = 0)
		{
			//ret = pSelfCheck->EmmcStatusCheck();

			if(ret == 0)
			{
				pSelfCheck->updateParameter(4, (char *)"故障");
			}
			else if(ret == 1)
			{
				pSelfCheck->updateParameter(4, (char *)"正常");
			}
			else //-1
			{
				printf("Emmc check eoor!");
			}

			sleep(TBoxPara.detectionTime.AirBagBusInputDetectionTime);
		}
	}

	return (void *)0;
}

/*****************************************************************************
* Function Name : EmmcStatusCheckThread
* Description   : EMMC检测线程
* Input			: void *args
* Output        : None
* Return        : 0
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void *SelfCheck::EmmcStatusCheckThread(void *args)
{
	pthread_detach(pthread_self());
	SelfCheck *pSelfCheck = (SelfCheck *)args;
	int ret;

	while(1)
	{
		if(trigger.EmmcTrigger == 1)
		{
			ret = pSelfCheck->EmmcStatusCheck();

			if(ret == 0)
			{
				pSelfCheck->updateParameter(5, (char *)"故障");
			}
			else if(ret == 1)
			{
				pSelfCheck->updateParameter(5, (char *)"正常");
			}
			else //-1
			{
				printf("Emmc check eoor!");
			}
			//sleep(detectionTime.EmmcDetectionTime);

			trigger.EmmcTrigger = 0;
		}
		else if(trigger.EmmcTrigger = 0)
		{
			ret = pSelfCheck->EmmcStatusCheck();

			if(ret == 0)
			{
				pSelfCheck->updateParameter(5, (char *)"故障");
			}
			else if(ret == 1)
			{
				pSelfCheck->updateParameter(5, (char *)"正常");
			}
			else //-1
			{
				printf("Emmc check eoor!");
			}

			sleep(TBoxPara.detectionTime.EmmcDetectionTime);
		}
	}

	return (void *)0;
}

/*****************************************************************************
* Function Name : CanCommunicationStatusCheck
* Description   : CAN通信检测线程
* Input			: void *args
* Output        : None
* Return        : 0
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void *SelfCheck::CanCommunicationStatusCheck(void *args)
{
	pthread_detach(pthread_self());
	SelfCheck *pSelfCheck = (SelfCheck *)args;

	while(1)
	{
		sleep(TBoxPara.detectionTime.CanCommunicationDetectionTime);
	}

	return (void *)0;
}

/*****************************************************************************
* Function Name : IVICommunicationStatusCheck
* Description   : IVI通信检测线程
* Input			: void *args
* Output        : None
* Return        : 0
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void *SelfCheck::IVICommunicationStatusCheck(void *args)
{
	pthread_detach(pthread_self());
	SelfCheck *pSelfCheck = (SelfCheck *)args;

	while(1)
	{
		sleep(TBoxPara.detectionTime.IVICommunicationDetectionTime);
	}

	return (void *)0;
}

/*****************************************************************************
* Function Name : PEPSCommunicationStatusCheck
* Description   : PEPS通信检测线程
* Input			: void *args
* Output        : None
* Return        : 0
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void *SelfCheck::PEPSCommunicationStatusCheck(void *args)
{
	pthread_detach(pthread_self());
	SelfCheck *pSelfCheck = (SelfCheck *)args;

	while(1)
	{
		sleep(TBoxPara.detectionTime.PEPSCommunicationDetectionTime);
		//pSelfCheck->updateParameter(8, char * content);
	}

	return (void *)0;
}

/*****************************************************************************
* Function Name : selfCheckInit
* Description   : 自检初始化
* Input			: None
* Output        : None
* Return        : 0: success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int SelfCheck::selfCheckInit()
{
    int ret;
    pthread_t LteModuleStatus;
    pthread_t LteAntennaStatus;
    pthread_t USIMStatus;
    pthread_t AirBagBusInputStatus;
    pthread_t EmmcStatus;
    pthread_t CanCommunicationStatus;
    pthread_t IVICommunicationStatus;
    pthread_t PEPSCommunicationStatus;

    ret = pthread_create(&LteModuleStatus, NULL, LteModuleStatusCheck, this);
    if (0 != ret)
    {
        printf("can't create thread: %s\n", strerror(ret));
        exit(-1);
    }

    ret = pthread_create(&LteAntennaStatus, NULL, LteAntennaStatusCheck, this);
    if (0 != ret)
    {
        printf("can't create thread: %s\n", strerror(ret));
        exit(-1);
    }

    ret = pthread_create(&USIMStatus, NULL, USIMStatusCheck, this);
    if (0 != ret)
    {
        printf("can't create thread: %s\n", strerror(ret));
        exit(-1);
    }

    ret = pthread_create(&AirBagBusInputStatus, NULL, AirBagBusInputStatusCheck, this);
    if (0 != ret)
    {
        printf("can't create thread: %s\n", strerror(ret));
        exit(-1);
    }

    ret = pthread_create(&EmmcStatus, NULL, EmmcStatusCheckThread, this);
    if (0 != ret)
    {
        printf("can't create thread: %s\n", strerror(ret));
        exit(-1);
    }

    ret = pthread_create(&CanCommunicationStatus, NULL, CanCommunicationStatusCheck, this);
    if (0 != ret)
    {
        printf("can't create thread: %s\n", strerror(ret));
        exit(-1);
    }

    ret = pthread_create(&IVICommunicationStatus, NULL, IVICommunicationStatusCheck, this);
    if (0 != ret)
    {
        printf("can't create thread: %s\n", strerror(ret));
        exit(-1);
    }

    ret = pthread_create(&PEPSCommunicationStatus, NULL, PEPSCommunicationStatusCheck, this);
    if (0 != ret)
    {
        printf("can't create thread: %s\n", strerror(ret));
        exit(-1);
    }

    fileInit();

    return 0;
}

/*****************************************************************************
* Function Name : EmmcStatusCheck
* Description   : emmc状态检测
* Input			: None
* Output        : None
* Return        : int
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int SelfCheck::EmmcStatusCheck()
{
	int flag;
	FILE *fp;
	char buff[4];
	
	memset(buff, 0, sizeof(buff));
	
	fp = popen("ls /dev | grep mmcblk0p1 |grep -v \"grep\" |wc -l","r");
	if(fread(buff, sizeof(char), sizeof(buff), fp) > 0)
		printf("buff:%s\n",buff);
	else
	{
	    pclose(fp);
		return -1;
	}
	
	pclose(fp);
	
	flag = atoi(buff);
	printf("flag:%d\n", flag);
	
	return flag;
}

/*****************************************************************************
* Function Name : printMsg
* Description   : 打印数据内容
* Input			: uint8_t *pData
*                 uint16_t len
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void SelfCheck::printMsg(uint8_t *pData, uint16_t len)
{
	int i;

	for(i=0; i<len; i++)
		printf("%02x ",*(pData+i));
	printf("\n");
}

int SelfCheck::fileInit()
{
	FILE *fp = fopen(SELF_CHEK_FILE, "r+");
	if ( NULL == fp)
    {
        printf("open file failed, please create file firstly.\n");
		
		fp = fopen(SELF_CHEK_FILE, "w+");
		if( NULL == fp)
		{
			printf("open %s file failed!\n", SELF_CHEK_FILE);
			return -1;
		}
		
		initParameterTable(fp);
    }
    else
    {
		printf("open %s file success\n", SELF_CHEK_FILE);
	}
		
	return 0;
}

/*****************************************************************************
* Function Name : initParameterTable
* Description   : 初始化参数表
* Input			: FILE *fp
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int SelfCheck::initParameterTable(FILE *fp)
{
	char buff[50] = {0};
	
	sprintf(buff, "%sNULL\n", Lte_Module_STATUS);
	//printf("buff:%s", buff);
	if(fwrite(buff, sizeof(char), strlen(buff), fp) > 0)
	{
		printf("write %s success.\n", Lte_Module_STATUS);
	}
	else
	{
		printf("write %s failed.\n", Lte_Module_STATUS);
		return -1;
	}
	
	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%sNULL\n", Lte_Antenna_STATUS);
	//printf("buff:%s", buff);
	if(fwrite(buff, sizeof(char), strlen(buff), fp) > 0)
	{
		printf("write %s success.\n", Lte_Antenna_STATUS);
	}
	else
	{
		printf("write %s failed.\n", Lte_Antenna_STATUS);
		return -1;
	}
	
	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%sNULL\n", USIM_STATUS);
	//printf("buff:%s", buff);
	if(fwrite(buff, sizeof(char), strlen(buff), fp) > 0)
	{
		printf("write %s success.\n", USIM_STATUS);
	}
	else
	{
		printf("write %s failed.\n", USIM_STATUS);
		return -1;
	}
	
	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%sNULL\n", AirBagBusInput_STATUS);
	//printf("buff:%s", buff);
	if(fwrite(buff, sizeof(char), strlen(buff), fp) > 0)
	{
		printf("write %s success.\n", AirBagBusInput_STATUS);
	}
	else
	{
		printf("write %s failed.\n", AirBagBusInput_STATUS);
		return -1;
	}
	
	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%sNULL\n", Emmc_STATUS);
	//printf("buff:%s", buff);
	if(fwrite(buff, sizeof(char), strlen(buff), fp) > 0)
	{
		printf("write %s success.\n", Emmc_STATUS);
	}
	else
	{
		printf("write %s failed.\n", Emmc_STATUS);
		return -1;
	}

	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%sNULL\n", CanCommunication_STATUS);
	//printf("buff:%s", buff);
	if(fwrite(buff, sizeof(char), strlen(buff), fp) > 0)
	{
		printf("write %s success.\n", CanCommunication_STATUS);
	}
	else
	{
		printf("write %s failed.\n", CanCommunication_STATUS);
		return -1;
	}
	
	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%sNULL\n", IVICommunication_STATUS);
	//printf("buff:%s", buff);
	if(fwrite(buff, sizeof(char), strlen(buff), fp) > 0)
	{
		printf("write %s success.\n", IVICommunication_STATUS);
	}
	else
	{
		printf("write %s failed.\n", IVICommunication_STATUS);
		return -1;
	}

	memset(buff, 0, sizeof(buff));
	sprintf(buff, "%sNULL\n", PEPSCommunication_STATUS);
	//printf("buff:%s", buff);
	if(fwrite(buff, sizeof(char), strlen(buff), fp) > 0)
	{
		printf("write %s success.\n", (char *)PEPSCommunication_STATUS);
	}
	else
	{
		printf("write %s failed.\n", PEPSCommunication_STATUS);
		return -1;
	}

	fclose(fp);
	system("sync");
	
	return 0;
}

/*****************************************************************************
* Function Name : updateParameter
* Description   : 更新参数
* Input			: int paraId,
*                 char *content
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int SelfCheck::updateParameter(int paraId, char *content)
{
	int maxRow;
	int rowNumber = 0;
	char lineBuff[512] = {0};
	char buff[10][512];
	char tmpContent[512] = {0};

	printf("updateParameter!\n");

	if((paraId == 0) && (content == NULL))
		return -1;
	
	FILE *fp = fopen(SELF_CHEK_FILE, "r+");
	if( NULL == fp)
	{
		printf("open %s file failed!\n", SELF_CHEK_FILE);
		return -1;
	}

	memset(buff, 0, sizeof(buff));

	while(fgets(lineBuff, sizeof(lineBuff), fp) != NULL)
	{
		strcpy(buff[rowNumber], lineBuff);
		memset(lineBuff, 0, sizeof(lineBuff));

		//printf("lineBuff:%s", lineBuff);
		//printf("buff[rowNumber]:%s", buff[rowNumber]);

		if(paraId == 1)
		{
			if(strstr(buff[rowNumber], Lte_Module_STATUS) != NULL)
			{
				sprintf(tmpContent, "%s%s\n", Lte_Module_STATUS, content);
				memset(buff[rowNumber], 0, 512);
				strcpy(buff[rowNumber], tmpContent);
			}
		}
		else if(paraId == 2)
		{
			if(strstr(buff[rowNumber], Lte_Antenna_STATUS) != NULL)
			{
				sprintf(tmpContent, "%s%s\n", Lte_Antenna_STATUS, content);
				memset(buff[rowNumber], 0, 512);
				strcpy(buff[rowNumber], tmpContent);
			}
		}
		else if(paraId == 3)
		{
			if(strstr(buff[rowNumber], USIM_STATUS) != NULL)
			{
				sprintf(tmpContent, "%s%s\n", USIM_STATUS, content);
				memset(buff[rowNumber], 0, 512);
				strcpy(buff[rowNumber], tmpContent);
			}
		}
		else if(paraId == 4)
		{
			if(strstr(buff[rowNumber], AirBagBusInput_STATUS) != NULL)
			{
				sprintf(tmpContent, "%s%s\n", AirBagBusInput_STATUS, content);
				memset(buff[rowNumber], 0, 512);
				strcpy(buff[rowNumber], tmpContent);
			}
		}
		else if(paraId == 5)
		{
			if(strstr(buff[rowNumber], Emmc_STATUS) != NULL)
			{
				sprintf(tmpContent, "%s%s\n", Emmc_STATUS, content);
				memset(buff[rowNumber], 0, 512);
				strcpy(buff[rowNumber], tmpContent);
			}
		}
		else if(paraId == 6)
		{
			if(strstr(buff[rowNumber], CanCommunication_STATUS) != NULL)
			{
				sprintf(tmpContent, "%s%s\n", CanCommunication_STATUS, content);
				memset(buff[rowNumber], 0, 512);
				strcpy(buff[rowNumber], tmpContent);
			}
		}
		else if(paraId == 7)
		{
			if(strstr(buff[rowNumber], IVICommunication_STATUS) != NULL)
			{
				sprintf(tmpContent, "%s%s\n", IVICommunication_STATUS, content);
				memset(buff[rowNumber], 0, 512);
				strcpy(buff[rowNumber], tmpContent);
			}
		}
		else if(paraId == 8)
		{
			if(strstr(buff[rowNumber], PEPSCommunication_STATUS) != NULL)
			{
				sprintf(tmpContent, "%s%s\n", PEPSCommunication_STATUS, content);
				memset(buff[rowNumber], 0, 512);
				strcpy(buff[rowNumber], tmpContent);
			}
		}

		rowNumber++;
	}

	maxRow = rowNumber;
    rewind(fp);
    for (rowNumber = 0; rowNumber <= maxRow; rowNumber++)
    {
        fputs(buff[rowNumber], fp);
    }

	fclose(fp);
	
	return 0;
}

/*
int SelfCheck::fileTest()
{
	fileInit();
		
	updateParameter(1, (char *)"正常");

	updateParameter(2, (char *)"断开");

	updateParameter(6, (char *)"CAN bus off");

	updateParameter(8, (char *)"PEPS节点丢失");

    return 0;
}*/




