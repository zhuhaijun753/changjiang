#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <time.h>
#include "SystemTime.h"



/*****************************************************************************
* Function Name : setSystemTime
* Description   : 设置系统时间
* Input			: struct tm *pTm
* Output        : None
* Return        : 0:success, -1:failed
* Auther        : ygg
* Date         : 2018.04.11
*****************************************************************************/
int setSystemTime(struct tm *pTm)
{
    time_t timep;
    struct timeval tv;

    timep = mktime(pTm);
    tv.tv_sec = timep - 86400 * (365 * 15 + 5);     //减去20年时间，这个值只要固定就好，等获取的时候加回去
    tv.tv_usec = 0;
    if (settimeofday(&tv, (struct timezone*)0) < 0)
    {
        printf("Set system datatime error!\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
* Function Name : getSystemTime
* Description   : 获得系统时间
* Input			: struct tm *pTm
* Output        : None
* Return        : struct tm*
* Auther        : ygg
* Date         : 2018.04.11
*****************************************************************************/
struct tm* getSystemTime()
{
	time_t timep ;
	time_t tmp_time;
	struct tm *pTm;
	time(&timep);
	tmp_time = timep + 86400 * (365 * 15 + 5);
	pTm = gmtime(&tmp_time);
	return pTm;
}

/*****************************************************************************
* Function Name : getSystem_real_Time
* Description   : 获取当前系统真实时间
* Input			: struct tm *pTm
* Output        : None
* Return        : struct tm*
* Auther        : ygg
* Date         : 2018.04.11
*****************************************************************************/
struct tm* getSystem_real_Time()
{
	time_t timep;
	struct tm *pTm;
	time(&timep);
	pTm = gmtime(&timep);
	return pTm;
}

#if 0
int set_time_test()
{
		char scan_string[32];
		struct tm _set_tm;
		int m_Year;
		int m_Month;
		int m_Day;
		int m_Hour;
		int m_Min;
		int m_Sec;
		while(1)
		{
				printf("please input Year:");
		    memset(scan_string, 0, sizeof(scan_string));
		    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
		        continue;		
			  m_Year = atoi(scan_string);
				
				printf("please input Month:");
		    memset(scan_string, 0, sizeof(scan_string));
		    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
		        continue;		
			  m_Month = atoi(scan_string);
			  
				printf("please input day:");
		    memset(scan_string, 0, sizeof(scan_string));
		    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
		        continue;		
			  m_Day = atoi(scan_string);
				
				printf("please input Hour:");
		    memset(scan_string, 0, sizeof(scan_string));
		    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
		        continue;		
			  m_Hour = atoi(scan_string);
			  
				printf("please input min:");
		    memset(scan_string, 0, sizeof(scan_string));
		    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
		        continue;		
			  m_Min = atoi(scan_string);
			  
				printf("please input sec:");
		    memset(scan_string, 0, sizeof(scan_string));
		    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
		        continue;		
			  m_Sec = atoi(scan_string);
			  break;	  	  	  	  	  
	  }
		
		
		_set_tm.tm_year = m_Year - 1900;
		_set_tm.tm_mon = m_Month + 1;   
		_set_tm.tm_mday = m_Day;    
		_set_tm.tm_hour = m_Hour;   
		_set_tm.tm_min = m_Min;    
		_set_tm.tm_sec = m_Sec;     
		
		setSystemTime(&_set_tm);
}

int main(int argc, char **argv)
{
		int option;
		char scan_string[32];
		struct tm *pTm;
		
		while(1)
		{
			printf("\npls select option:  1: get time 2: set_time: 3: get_real_time>");
			memset(scan_string, 0, sizeof(scan_string));
    	if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
        continue;	
			option = atoi(scan_string);
			
			if(option < 1 || option > 3) 
			{
				continue;	
			}
			
			if(option == 1)
			{
					pTm = getSystemTime();
					
					printf("current time: %04d-%02d-%02d, %02d:%02d:%02d\n", pTm->tm_year + 1900,
																                   pTm->tm_mon,
																                   pTm->tm_mday,
																                   pTm->tm_hour,
																                   pTm->tm_min,
																                   pTm->tm_sec);
					
			}
			else if(option == 2)
			{
					set_time_test();
			}
			else
			{
					pTm = getSystem_real_Time();
					printf("real time: %04d-%02d-%02d, %02d:%02d:%02d\n", pTm->tm_year + 1900,
																                   pTm->tm_mon,
																                   pTm->tm_mday,
																                   pTm->tm_hour,
																                   pTm->tm_min,
																                   pTm->tm_sec);					
			}
	  }
		
    return 0;
}

#endif
