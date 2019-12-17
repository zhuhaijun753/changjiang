#include "common.h"


const unsigned char sysVerNumber[3] = {1, 0, 0};
unsigned char mcuVerNumber[3] = {1, 0, 0};


/*****************************************************************************
* Function Name : getDate
* Description   : 获取当前日期
* Input			: char *pDest
*                 int size
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void getDate(char *pDest, int size)
{
	unsigned char i;
	unsigned char sysMonth = 0;
	char *pos = pDest;
	char year[5] = {0};
	char month[5] = {0};
	char day[5] = {0};
	const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug","Sep", "Oct", "Nov", "Dec"};
	char date[] = __DATE__;
	char time[] = __TIME__;

	//printf("Date:%s,Time:%s\n", date, time);

	sscanf(date, "%s %s %s", month, day, year);
	//printf("month:%s,day:%s,year:%s\n", month, day, year);

	for(i = 0; i<12; i++)
	{
		if(strncmp(month, months[i], 3) == 0)
		{
			sysMonth = i+1;
			break;
		}
	}
	//printf("%02x\n", sysMonth);

	memset(pDest, 0, size);
	memcpy(pos, year, 4);
	pos = pos + 4;

	*pos++ = (char)((sysMonth%100)/10+0x30);
	*pos++ = (char)(sysMonth%10+0x30);

	//printf("day len %d\n",(int)strlen(day));
	if(strlen(day) < 2)
	{
		*pos++ = 0x30; // 0x30-> ASCII: 0
		*pos++ = day[0];
	}
	else
	{
		memcpy(pos, day, 2);
		pos += 2;
	}
	
	//*pos++ = 0x5F;    // 0x5F-> ASCII: _
	//memcpy(pos, time, 8);
	
	//printf("pDest:%s\n", pDest);
}

#if 0
/*****************************************************************************
* Function Name : getSoftwareVerion
* Description   : 获取当前日期
* Input			: char *pBuff
*                 unsigned int size
* Output        : None
* Return        : 0:success
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int getSoftwareVerion(char *pBuff, unsigned int size)
{
	char sysVerion[12] = {0};
	char sysDateTime[32] = {0};

	memset(pBuff, 0, size);
	
	strcat(pBuff, RELEASE_NOTE);
	sprintf(sysVerion, "V%d.%d.%d", sysVerNumber[0], sysVerNumber[1],sysVerNumber[2]);
	strcat(pBuff, sysVerion);
	strcat(pBuff, BUILD_NOTE);
	getDate(sysDateTime, sizeof(sysDateTime));
	strcat(pBuff, sysDateTime);

	return 0;
}

#endif


/*****************************************************************************
* Function Name : getSoftwareVerion
* Description   : 获取当前日期
* Input			: char *pBuff
*                 unsigned int size
* Output        : None
* Return        : 0:success, -1:failed
* Auther        : ygg
* Date          : 2018.04.09
*****************************************************************************/
int getSoftwareVerion(char *pBuff, unsigned int size)
{
	char sysDateTime[32] = {0};
	if(pBuff == NULL)
		return -1;
	
	memset(pBuff, 0, size);
	strcat(pBuff, PROJECT_NAME);
	strcat(pBuff, ".");
//	strcat(pBuff, PLATFORM_CODE);
//	strcat(pBuff, ".");
	strcat(pBuff, SUPPORT_GBT32960_VER);
	strcat(pBuff, ".");
	strcat(pBuff, SUPPORT_ACP_VER);
	strcat(pBuff, ".");
//	strcat(pBuff, FILE_VER_PREFIX);
	strcat(pBuff, FILE_VERSION);
	strcat(pBuff, ".");
	getDate(sysDateTime, sizeof(sysDateTime));
	strcat(pBuff, sysDateTime);
	
	return 0;
}


void StrToHex(unsigned char *Strsrc, unsigned char *Strdst, int Strlen)
{
	if(Strlen % 2 != 0)
		return ;
	char temp = 0;
	char tp = 0;
	
	while(Strlen > 0){
		if('0' <= *Strsrc && *Strsrc <= '9')
		{
			temp = *Strsrc - '0';
		}
		else if('A' <= *Strsrc && *Strsrc <= 'F')
		{
			temp = (*Strsrc - 'A') + 10;
		}
		else
		{
			temp = (*Strsrc - 'a') + 10;
		}
		Strlen--;
		Strsrc++;
		if(Strlen % 2 != 0)
		{
			tp = temp;
		}
		else
		{
			*Strdst++ = ((tp << 4) & 0xF0) | (temp & 0x0F);
		}
	}
}

int sys_mylog(char *plog, int length)
{
	/*#define LOG_PATH	"/data/Fatorymylog"
	char buff[1024] = "";
	sprintf(buff, "== %s ==%s==\n",__TIME__, plog);
	FILE *flogfd;
	flogfd = fopen(LOG_PATH, "a+");
	if(flogfd == NULL)
	{
		printf("filelog open failed\n");
		return -1;
	}

	fwrite(buff, 1, strlen(buff), flogfd);

	fclose(flogfd);
	return 0;*/


	#define LOG_PATH	"/data/mylog"

	int glDebugLogSwitch = 0;
	char buff[1024] = "";
	struct tm *p_tm = NULL; 
	time_t tmp_time;
	tmp_time = time(NULL);
	p_tm = gmtime(&tmp_time);

	FILE *flogfd;

    struct stat buf;
	
    sprintf(buff,"%d-%d-%d-%2d:%2d:%2d\r\n",p_tm->tm_year+1900,p_tm->tm_mon+1,p_tm->tm_mday,p_tm->tm_hour,p_tm->tm_min,p_tm->tm_sec);
	
	
	if( 1)
	{
		stat(LOG_PATH, &buf);
		if(buf.st_size> 100*1000*1000)
		{ // Ã¨Â¶â€¦Ã¨Â¿â€?10 Ã¥â€¦â€ Ã¥Ë†â„¢Ã¦Â¸â€¦Ã§Â©ÂºÃ¥â€ â€¦Ã¥Â®Â?
			flogfd = fopen("/data/mylog", "w");
		} 
		else 
		{
			flogfd = fopen("/data/mylog", "at");
		}
		if(flogfd == NULL)
		{
			//printf("jason +++++++++++++++++++ filelog open failed\n");
			return -1;
		}
		//flogfd = fopen(LOG_PATH, "a+");
		//printf("jason +++++++++++++++++++ filelog open \r\n");

		fwrite(buff, 1, strlen(buff), flogfd);
		for(int i=0;i<length;i++)
		{
			fprintf(flogfd, "%02x\t", plog[i]);
		}
		fprintf(flogfd, "\r\n");

		//fsync(flogfd);
	    fflush(flogfd);

		fclose(flogfd);
	}
	else
	{
		//printf("jason +++++++++++++++++++ filelog close \r\n");
	}
	return 0;
}



#if 1
int getSystemVerion(char *pBuff)
{
	#define SYSTEMVER	"BOOT_V"
	char sysDateTime[32] = {0};
	int Len = 0;
	if(pBuff == NULL)
		return -1;
	
	int fd = open("/etc/version", O_RDONLY);
	if(fd < 0){
		printf("open error\n");
		return -1;
	}
	
	Len = read(fd, sysDateTime, sizeof(sysDateTime));
	if(Len <= 0)
	{
		printf("read error\n");
		close(fd);
		return -1;
	}
	
	memcpy(pBuff, SYSTEMVER, 6);
	memcpy(pBuff+6, sysDateTime, Len);

	return 0;
}
#endif
