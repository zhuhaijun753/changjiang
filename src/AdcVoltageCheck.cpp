#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "AdcVoltageCheck.h"




#if 0
int adc_voltage_check(int *voltage)
{
	int i;
	char buff[20] = {0};
	char tmp[10] ={0};
	char *pos = NULL;
	FILE *fp;
	
	fp = popen("cat /sys/devices/qpnp-vadc-8/pa_therm1 |awk '{print $4}'", "r");
	if(fread(buff, sizeof(char), sizeof(buff), fp) > 0)
	{
		//printf("%s\n",buff);
		pos = strstr(buff, "V:");
		if (pos != NULL)
		{
			pos += strlen("V:");
			for (i = 0; i < sizeof(tmp); i++)
			{
				if (*pos == '\0')
					break;
				tmp[i] = *pos++;
			}
			*voltage = atoi(tmp);
		}
	}else{
		return -1;
	}
	pclose(fp);

	return 0;
}

#endif

#define SYSTEM_PATH    "/sys/devices/qpnp-vadc-8/pa_therm1"

int adc_voltage_check(int *voltage)
{
	int i;
	int fd;
	char buff[50] = {0};
	char tmp[10] ={0};
	char *pos = NULL;

	fd = open(SYSTEM_PATH, O_RDONLY);
	if (fd < 0)
	{
		printf("Open file:%s error.\n", SYSTEM_PATH);
		return -1;
	}
	
	if(read(fd, buff, sizeof(buff)) > 0)
	{
		//printf("%s\n",buff);
		pos = strstr(buff, "V:");
		if (pos != NULL)
		{
			pos += strlen("V:");
			for (i = 0; i < sizeof(tmp); i++)
			{
				if (*pos == '\0')
					break;
				tmp[i] = *pos++;
			}
			*voltage = atoi(tmp);
		}
	}

	close(fd);
	
	return 0;
}


