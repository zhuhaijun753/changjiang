#include "NetworkConnection.h"


static callback_networkConnectionStatus networkConnection = NULL;
static int timeoutCount;
bool threadContinued = true;
static pthread_mutex_t nc_lock = PTHREAD_MUTEX_INITIALIZER;

static int get_threadContinued()
{
    pthread_mutex_lock(&nc_lock);
    return threadContinued;
    pthread_mutex_unlock(&nc_lock);    
}

static int set_threadContinued(int val)
{
    pthread_mutex_lock(&nc_lock);
    threadContinued = val;
    pthread_mutex_unlock(&nc_lock);
}

/*****************************************************************************
* Function Name : callback_onTimer
* Description   : 信号处理回调函数
* Input			: int signum
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
static void callback_onTimer(int signum)
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s, n;
    char host[NI_MAXHOST];
    int foundIP = 0;
	
	printf("catch signal %d\n", signum);

    timeoutCount--;
    
	if (getifaddrs(&ifaddr) == -1)
	{
		perror("getifaddrs");
		return;
	}else
		printf("getifaddrs ok!\n");

    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
    {
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

        /* Display interface name and family (including symbolic
        * form of the latter for the common families)
        */
        printf("%-8s %s (%d)\n",
              ifa->ifa_name,
              (family == AF_PACKET) ? "AF_PACKET" :
              (family == AF_INET) ? "AF_INET" :
              (family == AF_INET6) ? "AF_INET6" : "???",
              family);
			  
		/* Find the "rmnet_data0" network adapter */
		if (strncmp(ifa->ifa_name, "rmnet_data0", 11) == 0)
		{
			foundIP = 1;
			break;
		}
    }

    if (foundIP)
    {
		s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
		                host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (s != 0)
        {
           printf("getnameinfo() failed: %s\n", gai_strerror(s));
        }
		
        printf("Get IP address:%s \n", host);
		
        if (networkConnection)
        {
            //set_threadContinued(false);
			threadContinued = false;
            networkConnection(1);
		}
    } 
	else
	{
		printf("< Not find IP, timeoutCount:%d >\n", timeoutCount);

		if (timeoutCount > 0)
		{
			networkConnection(0);
			alarm(1);
		}
		else
		{
			printf("timeout, can not get ip adress!\n");
            //set_threadContinued(false);
			threadContinued = false;
			networkConnection(0);
		}
	}
	
   freeifaddrs(ifaddr);
}

/*****************************************************************************
* Function Name : detectNetworkStatus
* Description   : 检测网络状态线程
* Input			: void *arg
* Output        : None
* Return        : NULL
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void *detectNetworkStatus(void *arg)
{
	pthread_detach(pthread_self());

	signal(SIGALRM, callback_onTimer);
	alarm(1);

	//while(get_threadContinued())
	while(threadContinued)
	{
		usleep(5000);
	}
	
	return NULL;
    //pthread_exit((void *)1);
}

/*****************************************************************************
* Function Name : enableMobileNetwork
* Description   : 判断网络是否正常函数
* Input			: int timeout
*                 callback_networkConnectionStatus cb_networkConnectionStatus
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int enableMobileNetwork(int timeout, callback_networkConnectionStatus cb_networkConnectionStatus)
{
	//FILE *fp;
	//pid_t status;
	pthread_t threadId;
	//char buff[5];
	//int isProgramRunning = 0;
	//void *retval;

	timeoutCount = timeout;
	networkConnection = cb_networkConnectionStatus;

#if 0	

	/*memset(buff,0,sizeof(buff));
	fp = popen("ps -ef |grep Quectel_QCMAP_CLI |grep -v \"grep\" |wc -l","r");
	if(fread(buff, sizeof(char), sizeof(buff), fp) > 0)
		;//printf("buff:%s\n",buff);
	else
		return -1;

	pclose(fp);

	isProgramRunning = atoi(buff);
	//printf("isProgramRunning:%d\n", isProgramRunning);

	if(isProgramRunning == 0)
		status = system("start-stop-daemon -S -b -a Quectel_QCMAP_CLI &");*/

	status = system("start-stop-daemon -S -b -a Quectel_QCMAP_CLI &");
	
	if (-1 == status)
	{
		printf("system error!");
	}
	else
	{
		printf("exit status = [0x%x], WIFEXITED(status)=%d, WEXITSTATUS(status) = [%d]\n",status,WIFEXITED(status),WEXITSTATUS(status));
		
		if (WIFEXITED(status))
		{
			if (0 == WEXITSTATUS(status))
			{
				printf("Starting Mobile, please waiting...\n");

				// Start a thread to detect linux network status
				if (pthread_create(&threadId, NULL, detectNetworkStatus, NULL) != 0)
				{
					printf("Fail to create thread!\n");
					return -1;
				}
				
				/*
				// Wait for the end of thread, and put the return value in retval
				if (pthread_join(threadId, &retval) != 0)
				{
					printf("Join threadId error!\n");
					return -3;
				}
				printf("thrd_nw exit code: %d\n", *((int *)retval)); */
			} 
			else
			{
				printf("Fail to start Quectel_QCMAP_CLI, error code: %d\n", WEXITSTATUS(status));
				return -2;
			}
		}
	}
#endif

	// Start a thread to detect linux network status
	if (pthread_create(&threadId, NULL, detectNetworkStatus, NULL) != 0)
	{
		printf("Fail to create thread!\n");
		return -1;
	}	

	return 0;
}

