#ifndef _WIFI_CONTROL_H_
#define _WIFI_CONTROL_H_
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "TBoxDataPool.h"
#include "LedControl.h"


typedef enum{
	AP_MODE = 0,
	APAP_MODE,
	APSTA_MODE
}wifi_mode_type;

typedef enum{
	AP_INDEX_0 = 0,
	AP_INDEX_1,
	AP_INDEX_STA
}ap_index_type;


typedef unsigned char boolean;
typedef unsigned char uint8;






/*****************************************************************************
* Function Name : wifi_init
* Description   : 开启wifi,    1:打开wifi    0:关闭wifi
* Input			: uint8_t wifiMode
* Output        : None
* Return        : 0:success, -1:failed
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern int wifi_OpenOrClose(uint8_t isOpen);

//--------------------------------- AP ------------------------------------------

/*****************************************************************************
* Function Name : Wifi_Set_Mode
* Description   : 设置wifi模式, 0:Ap mode 1: Ap & AP, 2:Ap & sta
* Input			: uint8_t mode
* Output        : None
* Return        : id
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern int Wifi_Set_Mode(uint8 mode);

/*****************************************************************************
* Function Name : get_client_count
* Description   : get the client number that connected to the wifi
* Input         : ap_index_type ap_index
* Output        : None
* Return        : int 
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern int get_client_count(ap_index_type ap_index);

/*****************************************************************************
* Function Name : get_mac_addr
* Description   : 获取本地MAC及连接上的MAC
* Input         : char *mac_addr, ap_index_type ap_index
* Output        : None
* Return        : boolean
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern boolean get_mac_addr(char *mac_addr, ap_index_type ap_index);


/*****************************************************************************
* Function Name : set_bcast
* Description   : 设置广播
				  broadcast: 0-disable; 1-enable; ap_index_type ap_index
* Input			: int broadcast
* Output        : None
* Return        : boolean
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern boolean set_bcast(int broadcast,ap_index_type ap_index);

/*****************************************************************************
* Function Name : get_bcast
* Description   : 获取广播
				  broadcast: 0-disable; 1-enable, ap_index_type ap_index
* Input         : int *broadcast
* Output        : None
* Return        : boolean
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern boolean get_bcast(int *broadcast,ap_index_type ap_index);


/*****************************************************************************
* Function Name : get_dhcps
* Description   : 获取dhcp
				  char *host_ip_str, char *start_ip_str,
				  char *end_ip_str, char *time_str
* Input         : int *broadcast
* Output        : None
* Return        : boolean
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern boolean get_dhcp(char *host_ip_str, char *start_ip_str, char *end_ip_str, char *time_str);

/*****************************************************************************
* Function Name : wifi_get_ap_ssid
* Description   : 获取AP ssid 
* Input			: char *ssid, ap_index_type ap_index
                   ap_index_type: 0-> ap
                                  1-> ap & ap
                                  2-> ap & sta
* Output        : None
* Return        : 0:success, -1:failed
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern int wifi_get_ap_ssid(char *ssid, ap_index_type ap_index);

/*****************************************************************************
* Function Name : wifi_set_ap_ssid
* Description   : 设置AP ssid
* Input			: char *ssid, ap_index_type ap_index
                   ap_index_type: 0-> ap
                                  1-> ap & ap
                                  2-> ap & sta
* Output        : None
* Return        : 0:success, -1:failed
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern int wifi_set_ap_ssid(char *ssid, ap_index_type ap_index);

/*****************************************************************************
* Function Name : wifi_set_ap_auth
* Description   : 获取AP auth类型, 加密模式, 密码

	1.当auth type输入为0或1时，encrypt mode的输入值为0或1；
	2.当auth type输入为2时，encrypt mode的输入值只能为1；
	3.当auth type的输入值大于3时，encrypt mode的输入值必须大于等于2；
	4.当encrypt mode输入为0时，不需要输入password；
	5.当encrypt mode输入为1时，password必须输入。
	     Password输入格式必须满足：长度为5或者13的ASCII编码字符串，
	     或者长度为10或者26的十六进制编码字符串；
	6.当encrypt mode的输入值大于等于2时，password必须输入。
	     Password输入的格式必须满足：长度为8到63的ASCII编码字符串，
	     或者长度为64的十六进制编码字符串。

	Default value :
		int authType = 5;
		int encryptMode = 4;
		ap_index_type: 0-> ap
					   1-> ap & ap
					   2-> ap & sta
* Input			: int authType, int encryptMode, char *password
* Output        : None
* Return        : 0:success, -1:failed
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern int wifi_set_ap_auth(int authType, int encryptMode, char *password, ap_index_type ap_index);

/*****************************************************************************
* Function Name : wifi_get_ap_auth
* Description   : 获取AP auth类型, 加密模式, 密码
* Input			: int *authType, int *encryptMode, char *password,
                  ap_index_type: 0-> ap
					             1-> ap & ap
					             2-> ap & sta
* Output        : None
* Return        : 0:success, -1:failed
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern int wifi_get_ap_auth(int *authType, int *encryptMode, char *password, ap_index_type ap_index);




//--------------------------------- STA ------------------------------------------

/*****************************************************************************
* Function Name : get_sta_ip
* Description   : 获取STA IP
				  char *ip_str, int len
* Input         : int *broadcast
* Output        : None
* Return        : boolean
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern boolean get_sta_ip(char *ip_str, int len);

/*****************************************************************************
* Function Name : sta_scan
* Description   : sta搜索
* Input         : char *list_str
* Output        : None
* Return        : boolean
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern boolean sta_scan(char *list_str);

/*****************************************************************************
* Function Name : set_sta_cfg
* Description   : 设置STA连接外部AP
* Input         : char *SSID, int key_mgmt_value, 
                  int proto_value, char *psk_value
* Output        : None
* Return        : boolean
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern boolean set_sta_cfg(char *SSID, char *psk_value);

/*****************************************************************************
* Function Name : get_sta_cfg
* Description   : 获取外部AP的协议
* Input         : char *SSID, int key_mgmt_value, 
                  int proto_value, char *psk_value
* Output        : None
* Return        : boolean
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
extern boolean get_sta_cfg(char *ssid_str, char *psk_value);







/*****************************************************************************
* Function Name : restore_wifi
* Description   : 恢复wifi出厂设置
* Input         : None
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.03.14
*****************************************************************************/
extern void restore_wifi();

/*****************************************************************************
* Function Name : wifi_default_route
* Description   : wifi默认路由检测
* Input			: None
* Output        : None
* Return        : 0: success, -1: error
* Auther        : ygg
* Date          : 2018.03.18
*****************************************************************************/
extern int wifi_default_route();

/*****************************************************************************
* Function Name : wifi_connected_dataExchange
* Description   : wifi数据交互状态检测
* Input			: None
* Output        : None
* Return        : 0: success, -1: error
* Auther        : ygg
* Date          : 2018.03.18
*****************************************************************************/
extern int wifi_connected_dataExchange();

/*****************************************************************************
* Function Name : wifi_startState_check
* Description   : wifi开启状态检测
* Input			: None
* Output        : None
* Return        : 0: success, -1: error
* Auther        : ygg
* Date          : 2018.03.18
*****************************************************************************/
extern int wifi_startState_check();

/*****************************************************************************
* Function Name : wifi_name_pwd_init
* Description   : wifi名字和密码初始化
* Input			: None
* Output        : None
* Return        : 0: success, -1: error
* Auther        : ygg
* Date          : 2018.05.24
*****************************************************************************/
extern int wifi_name_pwd_init();

/*****************************************************************************
* Function Name : check_wifi_ssid_pwd_default_value
* Description   : check whether the wifi's ssid and password is the default
                  value, if it is then use the randomly generated's value to
                  set the wifi's ssid and password.
* Input			: None
* Output        : None
* Return        : 0: success, -1: error
* Auther        : ygg
* Date          : 2018.05.24
*****************************************************************************/
extern int check_wifi_ssid_pwd_default_value();


extern void wifi_init();

extern int wifi_name_pwd_init();

extern TBoxDataPool *dataPool;

#endif

