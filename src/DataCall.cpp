/**
  @file
  app_data_call.c

  @brief
  This file provides a demo to inplement a data call in mdm9607 embedded linux
  plane.

*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "dsi_netctrl.h"
#include "DataCall.h"
#include <arpa/inet.h>
#include <signal.h>
#include "ds_util.h"

#include <sys/ioctl.h>

#include <net/if.h>
#include <ifaddrs.h>
#include <netinet/in.h>

#define SA_FAMILY(addr)         (addr).sa_family
#define SA_DATA(addr)           (addr).sa_data
#define SASTORAGE_FAMILY(addr)  (addr).ss_family
#define SASTORAGE_DATA(addr)    (addr).__ss_padding

#define RMNET0_NAME "rmnet_data0"
#define RMNET1_NAME "rmnet_data1"
enum app_tech_e {
  app_tech_min = 0,
  app_tech_umts = app_tech_min,
  app_tech_cdma,
  app_tech_1x,
  app_tech_do,
  app_tech_lte,
  app_tech_auto,
  app_tech_max
};

typedef struct {
  enum app_tech_e tech;
  unsigned int dsi_tech_val;
} app_tech_map_t;

app_tech_map_t tech_map[] = {
  {app_tech_umts, DSI_RADIO_TECH_UMTS},
  {app_tech_cdma, DSI_RADIO_TECH_CDMA},
  {app_tech_1x, DSI_RADIO_TECH_1X},
  {app_tech_do, DSI_RADIO_TECH_DO},
  {app_tech_lte, DSI_RADIO_TECH_LTE},
  {app_tech_auto, DSI_RADIO_TECH_UNKNOWN}
};

enum app_call_status_e
{
  app_call_status_idle,
  app_call_status_connecting,
  app_call_status_connected,
  app_call_status_disconnecting
};

typedef struct {
  dsi_hndl_t handle;
  enum app_tech_e tech;
  int family;
  int profile;
  enum app_call_status_e call_status;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  struct {
    dsi_qos_id_type flow_id;
    dsi_qos_granted_info_type qos_granted_info;
  } qos_info;
} app_call_info_t;


struct event_strings_s
{
  dsi_net_evt_t evt;
  char * str;
};

pthread_t route_setting_thread;

struct event_strings_s event_string_tbl[DSI_EVT_MAX] =
{
  { DSI_EVT_INVALID, "DSI_EVT_INVALID" },
  { DSI_EVT_NET_IS_CONN, "DSI_EVT_NET_IS_CONN" },
  { DSI_EVT_NET_NO_NET, "DSI_EVT_NET_NO_NET" },
  { DSI_EVT_PHYSLINK_DOWN_STATE, "DSI_EVT_PHYSLINK_DOWN_STATE" },
  { DSI_EVT_PHYSLINK_UP_STATE, "DSI_EVT_PHYSLINK_UP_STATE" },
  { DSI_EVT_NET_RECONFIGURED, "DSI_EVT_NET_RECONFIGURED" },
  { DSI_EVT_QOS_STATUS_IND, "DSI_EVT_QOS_STATUS_IND" },
  { DSI_EVT_NET_NEWADDR, "DSI_EVT_NET_NEWADDR" },
  { DSI_EVT_NET_DELADDR, "DSI_EVT_NET_DELADDR" },
  { DSI_NET_TMGI_ACTIVATED, "DSI_NET_TMGI_ACTIVATED" },
  { DSI_NET_TMGI_DEACTIVATED, "DSI_NET_TMGI_DEACTIVATED" },
  { DSI_NET_TMGI_LIST_CHANGED, "DSI_NET_TMGI_LIST_CHANGED" },
};


static pthread_mutex_t datacall_lock; 

/* demo apn */
static char apn[] = "wonet";
static datacall_info_type s_datacall_info;

app_call_info_t app_call_info;

static datacall_status_type really_connect_status;
int check_ip_and_name_match(char *if_name, char *if_ip)
{
	FILE *fp;
	int i;
	char buff[16] = {0};
	memset(buff, 0, sizeof(buff));
    char command[512]={0};

    if(strlen(if_ip) < 7)
    {
        return FALSE;
    }
	sprintf(command,"ifconfig %s|sed -n '/inet addr/p' |awk '{print $2}'|awk -F: '{print $2}'|awk '{print $1}'",if_name);
    //sprintf(command,"hostapd_cli -i wlan%d all_sta | grep \"dot11RSNAStatsSTAAddress\" | awk -F= '{print $2}'", interface_num);
    printf("command:%s\n",command);
	fp = popen(command,"r");
    if(fp == NULL)
    {
        return FALSE;
    }
    
	if(fread(buff, sizeof(char), 15, fp) > 0)
	{
	    if(strlen(buff) < 7)
	    {
	        pclose(fp);
	        return FALSE;
	    }
	    
        for(i = 0;i < strlen(buff); i++)
        {
            if((buff[i] != '.') && !(buff[i] >= 0x30 && buff[i] <= 0x39))
            {
                buff[i] = 0;
            }
        }
        //printf("if_ip: %s, len = %d\n", if_ip, strlen(if_ip));
        //printf("buff: %s, len = %d\n", buff, strlen(buff));
        //printf("buff:%02X%02X%02X\n", buff[strlen(buff)-3],buff[strlen(buff)-2],buff[strlen(buff)-1]);

	    if(strlen(buff) < 7)
	    {
	        pclose(fp);
	        return FALSE;
	    }

	    if(strlen(buff) == strlen(if_ip) && strcmp(buff, if_ip) == 0)
	    {
	        //printf("match buff = %s\n",buff);
			pclose(fp);
			return TRUE;
		}
	}
	pclose(fp);
	return FALSE;  
}

static boolean check_valid_ip(char* ip)
{
    int a=-1,b=-1,c=-1,d=-1;
    char s[16]={0};

    if(ip == NULL || strlen(ip) < 7 || strlen(ip) > 15)
    {
        return FALSE;
    }

    sscanf(ip,"%d.%d.%d.%d%s",&a,&b,&c,&d,s);
    if(a>255 || a<0 || b>255 || b<0 || c>255 || c<0 ||d>255 || d<0 ) 
        return FALSE;
    if(s[0]!=0) 
        return FALSE;
     
    return TRUE;
}

#if 0
int query_ip_from_dns(char *url, char *pri_dns_ip, char *sec_dns_ip, char *ip)
{
	FILE *fp;
	char buff[32];
	memset(buff, 0, sizeof(buff));
    char command[512];
    int query_result = 0;
    int i;
    
    if(url == NULL || strlen(url) < 2)
    {
        return -1;
    }
    if((pri_dns_ip == NULL || strlen(pri_dns_ip) < 7) &&
       (pri_dns_ip == NULL || strlen(sec_dns_ip) < 7))
    {
        return -1;
    }

    if(ip == NULL)
    {
        return -1;
    }

    //printf("query_ip_from_dns: url=%s\n", url);
    //printf("query_ip_from_dns: dns1=%s\n", pri_dns_ip);
    //printf("query_ip_from_dns: dns2=%s\n", sec_dns_ip);
    if(pri_dns_ip != NULL)
    {
        memset(buff, 0, sizeof(buff));
        memset(command, 0, sizeof(command));
        sprintf(command,"nslookup %s %s|awk '{L[NR]=$0}END{for (i=5;i<=NR;i++){print L[i]}}'|grep Address|awk -F\": \" '{print $2}'",
                         url, pri_dns_ip);
        //printf("cmd=%s", command);
        fp = popen(command,"r");

        if(fread(buff, sizeof(char), 15, fp) >= 7)
        {
            //printf("dns1 buff:%s buf_len=%d, buff[len-1]=%d\n",buff,strlen(buff),buff[strlen(buff) - 1]);
            
            for(i = 0;i < strlen(buff); i++)
            {
                if((buff[i] != '.') && !(buff[i] >= 0x30 && buff[i] <= 0x39))
                {
                    buff[i] = 0;
                }
            }
            if(strlen(buff) >= 7 && check_valid_ip(buff))
            {
                strcpy(ip,buff);
                query_result = 1;
            }
        }
        pclose(fp);
    }

    if(query_result == 0 && sec_dns_ip != NULL)
    {
        memset(buff, 0, sizeof(buff));
        memset(command, 0, sizeof(command));
        sprintf(command,"nslookup %s %s|awk '{L[NR]=$0}END{for (i=5;i<=NR;i++){print L[i]}}'|grep Address|awk -F\": \" '{print $2}'",
                         url, sec_dns_ip);
        //printf("cmd=%s", command);                 
        fp = popen(command,"r");
    
        if(fread(buff, sizeof(char), 15, fp) >= 7)
        {
            //printf("dns2 buff:%s buf_len=%d, buff[len-1]=%d\n",buff,strlen(buff),buff[strlen(buff) - 1]);
            
            for(i = 0;i < strlen(buff); i++)
            {
                if((buff[i] != '.') && !(buff[i] >= 0x30 && buff[i] <= 0x39))
                {
                    buff[i] = 0;
                }
            }
            if(strlen(buff) >= 7 && check_valid_ip(buff))
            {
                strcpy(ip,buff);
                query_result = 1;
            }
        }
        pclose(fp);

    }

	if(query_result)
	    return 0;
	else
	    return -1;
}
#endif

int set_host_route(char *old_ip, char *new_ip, char *if_name)
{
    FILE *fp;
    char buff[16];
    char command[512];
    int old_ip_route_exist = 0;
    memset(buff, 0, sizeof(buff));
    memset(command, 0, sizeof(command));

    if(old_ip != NULL && strlen(old_ip) >= 7)
    {
        sprintf(command,"route -n|grep %s|grep UH|grep %s|awk '{print $1}'",old_ip, if_name);

        fp = popen(command,"r"); 
        if(fread(buff, sizeof(char), 15, fp) >= 7)
        {     
            if(strlen(buff) == strlen(old_ip))
            {
                old_ip_route_exist = 1;
            }
        }
        pclose(fp);
        if(old_ip_route_exist)
        {
            memset(command, 0, sizeof(command));
            sprintf(command,"route del -host %s dev %s",old_ip, if_name);
            ds_system_call(command, strlen(command));
        }
    }
    
    if(new_ip != NULL && strlen(new_ip) >= 7 && strlen(new_ip) <= 15)
    {
        if(check_valid_ip(new_ip))
        {
            memset(command, 0,sizeof(command));
            sprintf(command,"route add -host %s dev %s",new_ip, if_name);
            ds_system_call(command, strlen(command));        
        }
    }   
}


static void set_really_connect_status(datacall_status_type status)
{
    really_connect_status = status;
}
static datacall_status_type get_really_connect_status()
{
    return really_connect_status;
}


static void set_datacall_info(dsi_hndl_t handle)
{
    char device_name[DSI_CALL_INFO_DEVICE_NAME_MAX_LEN + 2] = {0};
    int index = -1;
    int ret;
    dsi_addr_info_t addr_info;
    char cmd[128] = {0};
    datacall_info_type datacall_info;
    datacall_info_type *pcallinfo = &s_datacall_info;
    memset(&datacall_info, 0, sizeof(datacall_info_type));

    
    memset(&addr_info, 0, sizeof(dsi_addr_info_t));
    
    //ret = dsi_get_device_name(handle,device_name,DSI_CALL_INFO_DEVICE_NAME_MAX_LEN);
    //printf("get device name: ret = %d, device_name=%s\n",ret, device_name);
  
    ret = dsi_get_ip_addr(handle,&addr_info,1);
    //printf("get ip addr ret = %d\n",ret);

    
    if (addr_info.iface_addr_s.valid_addr)
    {
        if (SASTORAGE_FAMILY(addr_info.iface_addr_s.addr) == AF_INET)
        {
            sprintf(datacall_info.ip_str,  "%d.%d.%d.%d", SASTORAGE_DATA(addr_info.iface_addr_s.addr)[0], SASTORAGE_DATA(addr_info.iface_addr_s.addr)[1], SASTORAGE_DATA(addr_info.iface_addr_s.addr)[2], SASTORAGE_DATA(addr_info.iface_addr_s.addr)[3]);
            //printf("ip_str = %s\n", datacall_info.ip_str);        
        }
    }
    if (addr_info.dnsp_addr_s.valid_addr)
    {
        sprintf(datacall_info.pri_dns_str,"%d.%d.%d.%d", SASTORAGE_DATA(addr_info.dnsp_addr_s.addr)[0], SASTORAGE_DATA(addr_info.dnsp_addr_s.addr)[1], SASTORAGE_DATA(addr_info.dnsp_addr_s.addr)[2], SASTORAGE_DATA(addr_info.dnsp_addr_s.addr)[3]);
        //printf("pri_dns_str = %s\n", datacall_info.pri_dns_str);
    }
    if (addr_info.dnss_addr_s.valid_addr)
    {
        sprintf(datacall_info.sec_dns_str, "%d.%d.%d.%d", SASTORAGE_DATA(addr_info.dnss_addr_s.addr)[0], SASTORAGE_DATA(addr_info.dnss_addr_s.addr)[1], SASTORAGE_DATA(addr_info.dnss_addr_s.addr)[2], SASTORAGE_DATA(addr_info.dnss_addr_s.addr)[3]);
        //printf("sec_dns_str = %s\n", datacall_info.sec_dns_str);
    }
    if (addr_info.gtwy_addr_s.valid_addr)
    {
        sprintf(datacall_info.gw_str, "%d.%d.%d.%d", SASTORAGE_DATA(addr_info.gtwy_addr_s.addr)[0], SASTORAGE_DATA(addr_info.gtwy_addr_s.addr)[1], SASTORAGE_DATA(addr_info.gtwy_addr_s.addr)[2], SASTORAGE_DATA(addr_info.gtwy_addr_s.addr)[3]);
        //printf("gw = %s\n", datacall_info.gw_str);
    }

    datacall_info.mask = addr_info.iface_mask;
    
   // printf("ip netmask: %d\n", datacall_info.mask);
   // printf("gw netmask: %d\n", addr_info.gtwy_mask);

    //ret = get_current_device_info(ip_str, if_name);

    if(check_ip_and_name_match(RMNET1_NAME,datacall_info.ip_str))
    {
        index = 1;
        strcpy(datacall_info.if_name, RMNET1_NAME);
    }
    else if(check_ip_and_name_match(RMNET0_NAME,datacall_info.ip_str))
    {   
        index = 0;
        strcpy(datacall_info.if_name, RMNET0_NAME);
    }
    else
    {
        //printf("no match\n");
        return;
    }

   // printf("index = %d\n", index);

    memset(pcallinfo->if_name, 0, sizeof(pcallinfo->if_name));
    strcpy(pcallinfo->if_name,  datacall_info.if_name);

    if(strlen(datacall_info.ip_str) >= 7)
    {
        memset(pcallinfo->ip_str,0,sizeof(pcallinfo->ip_str));
        strcpy(pcallinfo->ip_str, datacall_info.ip_str);
    }
    
    if(strlen(datacall_info.gw_str) >= 7)
    {
        memset(pcallinfo->gw_str,0,sizeof(pcallinfo->gw_str));
        strcpy(pcallinfo->gw_str, datacall_info.gw_str);
    }

    pcallinfo->mask = datacall_info.mask;

    if(strlen(pcallinfo->pri_dns_str) >= 7)
    {
        memset(cmd, 0,sizeof(cmd));
        sprintf(cmd, "route del -host %s dev %s%d",pcallinfo->pri_dns_str, "rmnet_data", index);
       // printf("cmd=%s\n", cmd);
        ds_system_call(cmd, strlen(cmd));        
    }
    if(strlen(datacall_info.pri_dns_str) >= 7)
    {
        memset(cmd, 0,sizeof(cmd));
        sprintf(cmd, "route add -host %s dev %s%d",datacall_info.pri_dns_str, "rmnet_data", index);
       // printf("cmd=%s\n", cmd);
        ds_system_call(cmd, strlen(cmd));
        strcpy(pcallinfo->pri_dns_str,datacall_info.pri_dns_str);
    }
    else
    {
        memset(pcallinfo->pri_dns_str, 0, sizeof(pcallinfo->pri_dns_str));
    }
    if(strlen(pcallinfo->sec_dns_str) >=7)
    {
        memset(cmd, 0,sizeof(cmd));
        sprintf(cmd, "route del -host %s dev %s%d",pcallinfo->sec_dns_str, "rmnet_data", index);
      //  printf("cmd=%s\n", cmd);
        ds_system_call(cmd, strlen(cmd));        
    }
    if(strlen(datacall_info.sec_dns_str) >= 7)
    {
        memset(cmd, 0,sizeof(cmd));
        sprintf(cmd, "route add -host %s dev %s%d",datacall_info.sec_dns_str, "rmnet_data", index);
      //  printf("cmd=%s\n", cmd);
        ds_system_call(cmd, strlen(cmd));
        strcpy(pcallinfo->sec_dns_str,datacall_info.sec_dns_str);
    }
    else
    {
        memset(pcallinfo->sec_dns_str, 0, sizeof(pcallinfo->sec_dns_str));
    }

    pcallinfo->status = DATACALL_CONNECTED;
}

void datacall_connected_fun(void *pData)
{
    dsi_hndl_t handle = (dsi_hndl_t)pData;
    pthread_detach(pthread_self());
    pthread_mutex_lock(&datacall_lock);
    if(get_really_connect_status() == DATACALL_CONNECTED)
    {
        set_datacall_info(handle);
    }
    pthread_mutex_unlock(&datacall_lock);
    process_simcom_ind_message(SIMCOM_EVENT_DATACALL_IND, NULL);
}

void datacall_disconnected_fun()
{
    datacall_info_type *pcallinfo = &s_datacall_info;
    char cmd[256]={0};
    pthread_mutex_lock(&datacall_lock);
    set_really_connect_status(DATACALL_DISCONNECTED);
    if(strlen(pcallinfo->ip_str) > 0)
    {
        memset(pcallinfo->ip_str,0,sizeof(pcallinfo->ip_str));
    }
    
    if(strlen(pcallinfo->gw_str) > 0)
    {
        memset(pcallinfo->gw_str,0,sizeof(pcallinfo->gw_str));
    }

    if(strlen(pcallinfo->pri_dns_str) >=7)
    {
        sprintf(cmd, "route del -host %s dev %s%",pcallinfo->pri_dns_str, pcallinfo->if_name);
        ds_system_call(cmd, strlen(cmd));
        memset(pcallinfo->pri_dns_str,0,sizeof(pcallinfo->pri_dns_str));
    }
    if(strlen(pcallinfo->sec_dns_str) >=7)
    {
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, "route del -host %s dev %s%",pcallinfo->sec_dns_str, pcallinfo->if_name);
        ds_system_call(cmd, strlen(cmd)); 
        memset(pcallinfo->pri_dns_str,0,sizeof(pcallinfo->pri_dns_str));
    }

    pcallinfo->status = DATACALL_DISCONNECTED;
    pthread_mutex_unlock(&datacall_lock);
    process_simcom_ind_message(SIMCOM_EVENT_DATACALL_IND, NULL);
}



static void app_net_ev_cb( dsi_hndl_t hndl,
                     void * user_data,
                     dsi_net_evt_t evt,
                     dsi_evt_payload_t *payload_ptr)
{
    int i;
    char suffix[256];

    if (evt > DSI_EVT_INVALID && evt < DSI_EVT_MAX)
    {
        app_call_info_t* call = (app_call_info_t*) user_data;
        memset( suffix, 0x0, sizeof(suffix) );

        /* Update call state */

        switch( evt )
        {
            case DSI_EVT_NET_NO_NET:	//2
                {
                    call->call_status =  app_call_status_idle;
                    datacall_disconnected_fun(); 
                }
                break;

            case DSI_EVT_NET_IS_CONN:	//1
                {
                    call->call_status = app_call_status_connected;
                    pthread_mutex_lock(&datacall_lock);
                    set_really_connect_status(DATACALL_CONNECTED);
                    pthread_mutex_unlock(&datacall_lock);
                    pthread_create(&route_setting_thread, NULL, datacall_connected_fun, hndl);
                }
                break;

            case DSI_EVT_QOS_STATUS_IND:		//6
            case DSI_EVT_PHYSLINK_DOWN_STATE:	//3
            case DSI_EVT_PHYSLINK_UP_STATE:		//4
            case DSI_EVT_NET_RECONFIGURED:		//5
            case DSI_EVT_NET_NEWADDR:			//7
            case DSI_EVT_NET_DELADDR:			//8
            case DSI_NET_TMGI_ACTIVATED:		//10
            case DSI_NET_TMGI_DEACTIVATED:		//11
            case DSI_NET_TMGI_LIST_CHANGED:		//13
                break;

            default:
                printf("<<< callback received unknown event, evt= %d\n",evt);
                return;
        }
#if 0
        /* Debug message */
        for (i=0; i < DSI_EVT_MAX; i++)
        {
            if (event_string_tbl[i].evt == evt)
            {
                printf("<<< callback received event [%s]%s\n",event_string_tbl[i].str, suffix);         
                break;
            }
        }
#endif
    }
}


int get_datacall_info(datacall_info_type *pcallinfo)
{
    int ret;
    pthread_mutex_lock(&datacall_lock);
    if(pcallinfo != NULL)
    {
        memcpy(pcallinfo, &s_datacall_info, sizeof(datacall_info_type));
    }
    pthread_mutex_unlock(&datacall_lock);
}

static void app_create_data_call(enum app_tech_e tech, int ip_family, int profile)
{
    dsi_call_param_value_t param_info;

    /* obtain data service handle */
	printf("enter app_create_data_call !\n");
    app_call_info.handle = dsi_get_data_srvc_hndl(app_net_ev_cb, (void*) &app_call_info);


    printf("app_call_info.handle 0x%x\n",app_call_info.handle );
    app_call_info.tech = tech;
    app_call_info.profile = profile;
    app_call_info.call_status = app_call_status_idle;


    /* set data call param - tech pref */
    param_info.buf_val = NULL;
    param_info.num_val = (int)tech_map[tech].dsi_tech_val;
    //printf("Setting tech to %d\n", tech_map[tech].dsi_tech_val);
    dsi_set_data_call_param(app_call_info.handle, DSI_CALL_INFO_TECH_PREF, &param_info);

    /* set data call param - other */
    switch (app_call_info.tech)
    {
        case app_tech_umts:
        case app_tech_lte:
            #if 0
            param_info.buf_val = apn;
            param_info.num_val = 1;
            //printf("Setting APN to %s\n", apn);
            dsi_set_data_call_param(app_call_info.handle, DSI_CALL_INFO_APN_NAME, &param_info);
            #endif
            break;

        case app_tech_cdma:
            param_info.buf_val = NULL;
            param_info.num_val = DSI_AUTH_PREF_PAP_CHAP_BOTH_ALLOWED;
            //printf("%s\n","Setting auth pref to both allowed");
            dsi_set_data_call_param(app_call_info.handle, DSI_CALL_INFO_AUTH_PREF, &param_info);
            break;

        default:
            break;
    }

    switch (ip_family)
    {
        case DSI_IP_VERSION_4:
            app_call_info.family = ip_family;
            param_info.buf_val = NULL;
            param_info.num_val = ip_family;
            //printf("Setting family to %d\n", ip_family);
            dsi_set_data_call_param(app_call_info.handle, DSI_CALL_INFO_IP_VERSION, &param_info);
            break;

        case DSI_IP_VERSION_6:
            app_call_info.family = ip_family;
            param_info.buf_val = NULL;
            param_info.num_val = ip_family;
            //printf("Setting family to %d\n", ip_family);
            dsi_set_data_call_param(app_call_info.handle, DSI_CALL_INFO_IP_VERSION, &param_info);
            break;

        case DSI_IP_VERSION_4_6:
            app_call_info.family = ip_family;
            param_info.buf_val = NULL;
            param_info.num_val = ip_family;
            //printf("Setting family to %d\n", ip_family);
            dsi_set_data_call_param(app_call_info.handle, DSI_CALL_INFO_IP_VERSION, &param_info);
            break;

        default:
            /* don't set anything for IPv4 */
            break;
    }

#define DSI_PROFILE_3GPP2_OFFSET (1000)

    if (profile > 0)
    {
        if( profile > DSI_PROFILE_3GPP2_OFFSET )
        {
            param_info.num_val = (profile - DSI_PROFILE_3GPP2_OFFSET);
            //printf("Setting 3GPP2 PROFILE to %d\n", param_info.num_val);
            dsi_set_data_call_param(app_call_info.handle, DSI_CALL_INFO_CDMA_PROFILE_IDX, &param_info);
        }
        else
        {
            param_info.num_val = profile;
            //printf("Setting 3GPP PROFILE to %d\n", param_info.num_val);
            dsi_set_data_call_param(app_call_info.handle, DSI_CALL_INFO_UMTS_PROFILE_IDX, &param_info);
        }
    }
}

#if 0
int main(int argc, char * argv[])
{
  memset( &app_call_info, 0x0, sizeof(app_call_info) );

 (void)pthread_mutex_init( &app_call_info.mutex, NULL );
 (void)pthread_cond_init( &app_call_info.cond, NULL );

  if (DSI_SUCCESS !=  dsi_init(DSI_MODE_GENERAL))
  {
    //printf("%s","dsi_init failed !!\n");
    return -1;
  }
   
  sleep(2);
  
  app_create_data_call(app_tech_auto, DSI_IP_VERSION_4, 0);
  
  //printf("Doing Net UP\n");
  
  dsi_start_data_call(app_call_info.handle);
  app_call_info.call_status = app_call_status_connecting;
 
	do{
		sleep (10000000);
	}while(1);

}
#endif


int dataCallDail()
{
    app_create_data_call(app_tech_auto, DSI_IP_VERSION_4, 4);

    printf("Doing Net UP\n");
    dsi_start_data_call(app_call_info.handle);
    app_call_info.call_status = app_call_status_connecting;

	return 0;
}

int dataCall_init()
{
    memset( &app_call_info, 0x0, sizeof(app_call_info) );
    (void)pthread_mutex_init(&app_call_info.mutex, NULL);
    (void)pthread_cond_init(&app_call_info.cond, NULL);

    if (DSI_SUCCESS !=  dsi_init(DSI_MODE_GENERAL))
    {
        //printf("%s", "dsi_init failed !!\n");
        return -1;
    }
    memset(&s_datacall_info,0,sizeof(datacall_info_type));
    pthread_mutex_lock(&datacall_lock);
    set_really_connect_status(DATACALL_CONNECTED);
    pthread_mutex_unlock(&datacall_lock);

    sleep(2);

	return 0;
}



