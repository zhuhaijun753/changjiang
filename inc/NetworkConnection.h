#ifndef _NETWORK_CONNECTION_H_
#define _NETWORK_CONNECTION_H_
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>



/***************************************************************************
* Function Name: callback_networkConnectionStatus					
* Function Introduction:
*               Callback for application to indicate if network is ready.
* Parameter description:
*	             result: 1 network status succeed.
* Function return value: no
* Data : 2017.10.10
****************************************************************************/
typedef void (*callback_networkConnectionStatus)(int result);


/***************************************************************************
* Function Name: enableMobileNetwork
* Function Introduction:
*               Start enable Mobile network, and detect if network is ready. 
* Parameter description:
*	          timeout  : second.
*      cb_networkConnectionStatus : 
*                      callback function for indication of network status.
* Function return value: 
*                     0: succeed, -1:failed
* Data : 2017.10.10
****************************************************************************/
int enableMobileNetwork(int timeout, callback_networkConnectionStatus cb_networkConnectionStatus);






#endif  //_NETWORK_CONNECTION_H_

