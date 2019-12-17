#ifndef _DATA_CALL_H_
#define _DATA_CALL_H_
#include "simcom_common.h"
#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 0
#endif

typedef enum{
    DATACALL_DISCONNECTED=0,
    DATACALL_CONNECTED
}datacall_status_type;

typedef unsigned char boolean;

typedef struct {
  datacall_status_type status;
  char if_name[32];
  char ip_str[16];
  char pri_dns_str[16];
  char sec_dns_str[16];
  char gw_str[16];
  unsigned int mask;
}datacall_info_type;


/*****************************************************************************
* Function Name : dataCallDail
* Description   : 数据拨号
* Input         : None
* Output        : None
* Return        : 0 
* Auther        : ygg
* Date          : 2018.03.13
*****************************************************************************/
extern int dataCallDail();

/*****************************************************************************
* Function Name : dataCall_init
* Description   : 初始化dataCall
* Input         : None
* Output        : None
* Return        : int 
* Auther        : ygg
* Date          : 2018.03.13
*****************************************************************************/
extern int dataCall_init();
extern int get_datacall_info(datacall_info_type *pcallinfo);
//extern int query_ip_from_dns(char *url, char *pri_dns_ip, char *sec_dns_ip, char *ip);
extern int set_host_route(char *old_ip, char *new_ip, char *if_name);

#endif

