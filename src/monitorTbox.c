#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>


#define TBOX_UPGRADE_SH       "/usr/bin/monitor.sh"
#define LTE_FILE_NAME         "/data/LteUpgrade.bin"


int main(int argc, char **argv)
{
	int ret;
	int status;
	pid_t pid;
	char buff[2];
	FILE *fp;
	int times = 0;
	
	while(1)
	{
		memset(buff, 0, sizeof(buff));
		fp = popen("ps -A|grep tbox |grep -v grep |wc -l","r");
		if(fread(buff, sizeof(char), sizeof(buff), fp) > 0)
		{
			ret = atoi(buff);
		}else{
			ret = 0;
		}
		
		//printf("ret:%d\n",ret);
		pclose(fp);
		
		if(ret == 0)
		{	
			printf("No tbox process!\n");
			pid = fork(); 
			if (pid == -1) {
				fprintf(stderr, "fork error! errno No.:%d error:%s\n", errno, strerror(errno));
				break;
			}
			if (pid == 0) {
				//if(times == 0)
				if((access(LTE_FILE_NAME, F_OK)==0)||(times < 5))
				//if(!access(LTE_FILE_NAME, F_OK))
				{
					printf("#################### run new tbox version! \n");
					ret = execlp(TBOX_UPGRADE_SH, TBOX_UPGRADE_SH, "start", "0", NULL);
				}
				else if(times == 5)
				{
					printf("#################### back to old version! \n");
					ret = execlp(TBOX_UPGRADE_SH, TBOX_UPGRADE_SH, "start", "1", NULL);
				}
				if (ret < 0) {
					fprintf(stderr, "execv ret:%d errno:%d error:%s\n", ret, errno, strerror(errno));
				}
				printf("exit child process!!! execlp return: %d\n", ret);
				exit(0);
			}
			if (pid > 0) {
				pid = wait(&status);
				fprintf(stdout, "Child process id: %d\n", pid);
				printf("Wait child process return\n");
				if(times <5)
				{
					printf("===================== 1111 \n");
					times++;
				}
				else if(times == 5)
				{
					printf("===================== 2222 \n");
					times = 0;
				}
			}
		}
		else
		{
			times = 0;
		}
		
		sleep(1);
    }

	return 0;
}


