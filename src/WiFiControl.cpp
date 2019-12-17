#include "WiFiControl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cutils/properties.h>
#include <linux/reboot.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ds_util.h"
#include <glib.h>
#include "qcmap_client_const.h"
#include "qcmap_client_def.h"

#define WIFIDAEMON_WEBCLIENT_SOCK "/data/etc/qcmap_webclient_wifidaemon_file"
#define WEBCLIENT_WIFIDAEMON_SOCK "/data/etc/qcmap_wifidaemon_webclient_file"

//Time to wait for socket responce in secs.
#define SOCK_TIMEOUT 20
#define MAX_BUFFER_SIZE 10240

#define SOCK_OPEN_ERROR "{\"commit\":\"Socket Open Error\"}"
#define SOCK_OPEN_ERROR_LEN 30
#define SOCK_FD_ERROR "{\"commit\":\"Socket FD Error\"}"
#define SOCK_FD_ERROR_LEN 28
#define SOCK_BIND_ERROR "{\"commit\":\"Socket Bind Error\"}"
#define SOCK_BIND_ERROR_LEN 30

#define SOCK_SEND_ERROR "{\"commit\":\"Socket Send Error\"}"
#define SOCK_SEND_ERROR_LEN 30
#define SOCK_SEND_COMPLETE_ERROR "{\"commit\":\"Unable to Send Complete "\
                                 "data through socket\"}"
#define SOCK_SEND_COMPLETE_ERROR_LEN 56
#define SOCK_TIMEOUT_ERROR "{\"commit\":\"Socket TimeOut\"}"
#define SOCK_TIMEOUT_ERROR_LEN 27
#define SOCK_RESPONSE_ERROR "{\"commit\":\"Socket Responce Error\"}"
#define SOCK_RESPONSE_ERROR_LEN 34
#define SOCK_RECEIVE_ERROR "{\"commit\":\"Socket Receive Error\"}"
#define SOCK_RECEIVE_ERROR_LEN 33
#define SUCCESS 0
#define FAIL    -1

#define MAX_ELEMENT_LENGTH 300       //Max Size of any element value
#define MAX_ELEMENT_COUNT  40       //Max Elements can be processed

#define TRUE 1
#define FALSE 0

//typedef unsigned char boolean;
//typedef unsigned char uint8;

const static char *country_name[] = {
	"AL","DZ","AR","AM","AU","AT","AZ","BH","BY","BE",
	"BZ","BO","BR","BN","BG","CA","CL","CN","CO","CR",
	"HR","CY","CZ","DK","DO","EC","EG","SV","EE","FI",
	"FR","DE","GE","GR","GT","HN","HK","HU","IS","IN",
	"ID","IR","IE","IL","IT","JO","KZ","KR","KP","KW",
	"LB","LV","LI","LT","LU","MA","MK","MY","MO","MX",
	"MC","NL","NZ","NO","OM","PK","PA","PE","PH","PL",
	"PT","PR","QA","RO","RU","SA","SG","SK","SI","ZA",
	"ES","SE","CH","SY","TW","TH","TT","TN","TR","UA",
	"AE","GB","US","UY","UZ","VE","VN","YE","ZW","JP"
};
const static int channel_number[] = {
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	10, 11, 12, 13, 36, 40, 44, 48, 52, 56,
	60, 64, 100,104,108,112,116,120,124,128,
	132,136,140,149,153
};

const static int CN_5G_channel_list[] ={
	149,153,157,161,165
};

const char *p_auth[] = { 
	"Auto",
	"Open",
	"Share",
	"WPA-PSK",
	"WPA2-PSK",
	"WPA-WPA2-PSK"
};

const char *p_encrypt[] = {
	"NULL",
	"WEP",
	"TKIP",
	"AES",
	"AES-TKIP"
};

/* Arry for wifi_daemon options, Should always have and even number of elements */
char *wifi_daemon_options_list[] = {
    "1. SET SSID",
    "2. GET SSID",
    "3. SET AUTH",
    "4. GET AUTH",
    "5. SET BCAST",
    "6. GET BCAST",
    "7. SET CHANNEL_MODE",
    "8. GET CHANNEL_MODE",
    "9. GET DHCP",
    "10. GET CLIENT_COUNT",
    "11. OPEN/CLOSE AP",
    "12. GET AP_STATUS",
    "13. GET NET_STATUS",
    "14. SET USER_INFO",
    "15. GET USER_INFO",
    "16. GET MAC_ADDR",
#ifdef WIFI_RTL_DAEMON
	"17. START/STOP STA",
    "18. GET STA_STATUS",
#else
    "17. GET AP_CFG",
    "18. SET AP_CFG",
#endif    
    "19. GET STA_IP",
    "20. SET STA_CFG",
    "21. GET STA_CFG",
    "22. STA SCAN",
    "23. RESTORE",
    "24. EXIT",
};
/*
typedef enum{
	AP_MODE =0,
	APAP_MODE,
	APSTA_MODE
}wifi_mode_type;

typedef enum{
	AP_INDEX_0 = 0,
	AP_INDEX_1,
	AP_INDEX_STA
}ap_index_type;
*/

//static int get_ap_cfg();
static wifi_mode_type get_ap_cfg();


//static int wifidaemon_mifi_configselect = 0;

static void WIFI_LOG(const char *format, ...)
{
#if 0
	printf(format, ...);
#endif
}

static int wifi_daemon_init_send_string(char** send_str, char** recv_str)
{
	*send_str = (char *)malloc(MAX_ELEMENT_COUNT * MAX_ELEMENT_LENGTH);
	if(*send_str == NULL)
	{
		return FAIL;
	}

	*recv_str = (char *)malloc(MAX_ELEMENT_COUNT * MAX_ELEMENT_LENGTH);
	if(*recv_str == NULL)
	{
		free(*send_str);
		return FAIL;
	}

	memset(*send_str, 0, MAX_ELEMENT_COUNT * MAX_ELEMENT_LENGTH);
	memset(*recv_str, 0, MAX_ELEMENT_COUNT * MAX_ELEMENT_LENGTH);
	return SUCCESS;
}

static void wifi_daemon_free_send_string(char *send_str, char* recv_str)
{
    if(send_str)
    {
    	free(send_str);
    }

    if(recv_str)
    {
    	free(recv_str);
    }
}

static int wifi_daemon_send_to_webcli(char *wifidaemon_webcli_buff,
			                   int wifidaemon_webcli_buff_size,
			                   char *webcli_wifidaemon_buff,
			                   int *webcli_wifidaemon_buff_size,
			                   int *webcli_sockfd)
{
	//Socket Address to store address of webclient and atfwd sockets.
	struct sockaddr_un webcli_wifidaemon_sock;
	struct sockaddr_un wifidaemon_webcli_sock;
	//Variables to track data received and sent.
	int bytes_sent_to_cli = 0;
	int sock_addr_len = 0;


	memset(&webcli_wifidaemon_sock,0,sizeof(struct sockaddr_un));
	memset(&wifidaemon_webcli_sock,0,sizeof(struct sockaddr_un));
	
	if ((*webcli_sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
	{
		*webcli_wifidaemon_buff_size = SOCK_OPEN_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_OPEN_ERROR, SOCK_OPEN_ERROR_LEN);
		return FAIL;
	}
	
	if (fcntl(*webcli_sockfd, F_SETFD, FD_CLOEXEC) < 0)
	{
	        close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_FD_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_FD_ERROR, SOCK_FD_ERROR_LEN);
		return FAIL;
	}

	webcli_wifidaemon_sock.sun_family = AF_UNIX;
       memset(webcli_wifidaemon_sock.sun_path, 0, sizeof(webcli_wifidaemon_sock.sun_path));
	strncat(webcli_wifidaemon_sock.sun_path, WEBCLIENT_WIFIDAEMON_SOCK,strlen(WEBCLIENT_WIFIDAEMON_SOCK));
	unlink(webcli_wifidaemon_sock.sun_path);
	sock_addr_len = strlen(webcli_wifidaemon_sock.sun_path) + sizeof(webcli_wifidaemon_sock.sun_family);
	if (bind(*webcli_sockfd, (struct sockaddr *)&webcli_wifidaemon_sock, sock_addr_len) == -1)
	{
    	close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_BIND_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_BIND_ERROR, SOCK_BIND_ERROR_LEN);
		return FAIL;
	}
	
	wifidaemon_webcli_sock.sun_family = AF_UNIX;
       memset(wifidaemon_webcli_sock.sun_path, 0, sizeof(wifidaemon_webcli_sock.sun_path));
	strncat(wifidaemon_webcli_sock.sun_path, WIFIDAEMON_WEBCLIENT_SOCK,strlen(WIFIDAEMON_WEBCLIENT_SOCK));
	
	/*d rwx rwx rwx = dec
	0 110 110 110 = 666
	*/
	
	//system("chmod 777 /etc/qcmap_cgi_webclient_file");
	sock_addr_len = strlen(wifidaemon_webcli_sock.sun_path) + sizeof(wifidaemon_webcli_sock.sun_family);
	if ((bytes_sent_to_cli = sendto(*webcli_sockfd, wifidaemon_webcli_buff, wifidaemon_webcli_buff_size, 0,
	(struct sockaddr *)&wifidaemon_webcli_sock, sock_addr_len)) == -1)
	{
		close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_SEND_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_SEND_ERROR, SOCK_SEND_ERROR_LEN);
		return FAIL;
	}

	if (bytes_sent_to_cli == wifidaemon_webcli_buff_size)
	{
		return SUCCESS;
	}
	else
	{
		close((int)*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_SEND_COMPLETE_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_SEND_COMPLETE_ERROR, SOCK_SEND_COMPLETE_ERROR_LEN);
		return FAIL;
	}
}


static int wifi_daemon_recv_from_webcli(char *webcli_wifidaemon_buff,
				              int *webcli_wifidaemon_buff_size,
				              int *webcli_sockfd)
{
	//FD set used to hold sockets for select.
	fd_set wifidaemon_sockfd;

	//Time out for atfwd response.
	struct timeval webcli_sock_timeout;

	//To evaluate webclient socket responce(timed out, error, ....)
	int webcli_sock_resp_status = 0;

	//Variables to track data received and sent.
	int bytes_recv_from_cli = 0;

	int retry = 0;
	do{
		FD_ZERO(&wifidaemon_sockfd);
		if( *webcli_sockfd < 0 )
       	{
         	*webcli_wifidaemon_buff_size = SOCK_FD_ERROR_LEN;
         	strlcat(webcli_wifidaemon_buff, SOCK_FD_ERROR, MAX_BUFFER_SIZE);
         	return FAIL;
       	}
		FD_SET(*webcli_sockfd, &wifidaemon_sockfd);
		webcli_sock_timeout.tv_sec = 30;//SOCK_TIMEOUT;
		webcli_sock_timeout.tv_usec = 0;

		webcli_sock_resp_status = select(((int)(*webcli_sockfd))+1,
		                               &wifidaemon_sockfd, NULL, NULL,
		                               &webcli_sock_timeout);
		retry++;

		if(retry > 20)
			break;
		if(webcli_sock_resp_status == -1)
			WIFI_LOG("atfwd_select_from_webcli==============%s \r\n",strerror(errno));
	}while((webcli_sock_resp_status == -1) && (errno == EINTR));

	if (webcli_sock_resp_status == 0)
	{
		FD_CLR(*webcli_sockfd, &wifidaemon_sockfd);
		close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_TIMEOUT_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_TIMEOUT_ERROR, SOCK_TIMEOUT_ERROR_LEN);
		return FAIL;  //Time out
	}
	else if  (webcli_sock_resp_status == -1)
	{
	    FD_CLR(*webcli_sockfd, &wifidaemon_sockfd);
		close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_RESPONSE_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_RESPONSE_ERROR, SOCK_RESPONSE_ERROR_LEN);
		return FAIL;  //Error
	}

	memset(webcli_wifidaemon_buff, 0, MAX_BUFFER_SIZE);
	bytes_recv_from_cli = recv(*webcli_sockfd, webcli_wifidaemon_buff, MAX_BUFFER_SIZE, 0);
	if (bytes_recv_from_cli == -1)
	{
	    FD_CLR(*webcli_sockfd, &wifidaemon_sockfd);
		close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_RECEIVE_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_RECEIVE_ERROR, SOCK_RECEIVE_ERROR_LEN);
		return FAIL;
	}
	else
	{
	    FD_CLR(*webcli_sockfd, &wifidaemon_sockfd);
		close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = bytes_recv_from_cli;
		webcli_wifidaemon_buff[bytes_recv_from_cli] = 0;
		return SUCCESS;
	}
}


static int wifi_daemon_webcli_communication(  char *wifidaemon_webcli_buff,
				                            int  wifidaemon_webcli_buff_size,
				                            char *webcli_wifidaemon_buff,
				                            int  *webcli_wifidaemon_buff_size)
{
	//Socket FD for Webclient socket. Used to communicate with Webclient socket.
	int webcli_sockfd = 0;
	int ret = FAIL;

	//Send data to WEB_CLI socket
	if (wifi_daemon_send_to_webcli( wifidaemon_webcli_buff, wifidaemon_webcli_buff_size,
						webcli_wifidaemon_buff, webcli_wifidaemon_buff_size,
						&webcli_sockfd) == 0)
	{
		//Receive data from Webclient
		ret = wifi_daemon_recv_from_webcli(webcli_wifidaemon_buff, webcli_wifidaemon_buff_size,&webcli_sockfd);
        WIFI_LOG("======wifi_daemon_webcli_communication return[%d]\r\n", ret);
	}
    WIFI_LOG("wifi_daemon_webcli_communication: ========ret[%d]\r\n", ret);
	return ret;
}

static int wifi_daemon_get_key_value(char* str_buf, const char *key, char *value, int *value_len)
{
	char *offset;
	char *offset1;
	if(str_buf == NULL)
	{
		return FALSE;
	}
	
	offset = strstr(str_buf, key);
	if(offset == NULL)
	{
		return FALSE;
	}

	offset = strstr(offset, "\":\"");
	if(offset == NULL)
	{
		return FALSE;
	}

	offset += 3;
	
	offset1 = strstr(offset, "\",");//fix bug: when ssid include "("test", not test), can't parse the ssid
	if(offset1 == NULL)
	{
		return FALSE;
	}

	if(offset1 == offset)
	{
		return TRUE;
	}

	*value_len = offset1 - offset;	
	memcpy(value, offset, *value_len);
	
	return TRUE;
	
}

static void wifi_daemon_set_key_value(char * send_str, const char *key, const char *value)
{
	if(send_str == NULL)
	{
		return;
	}
	if(strlen(send_str) != 0)
	{
		strncat(send_str, "&",  1);
	}

	strncat(send_str, key, strlen(key));
	strncat(send_str, "=",  1);

	if(value != NULL)
	{
		strncat(send_str, value, strlen(value));
	}
}

//////////////////////////////////////////////////////////////////////////////
static int is_Hex_String(char *str, int len)
{
	int  i;
	char c;
	int  ret = TRUE;
	for(i = 0; i < len; i++)
	{
		c = *(str + i);
		if(   !( c >= 0x30 && c <= 0x39) 
			&&!( c >= 0x41 && c <= 0x46)
			&&!( c >= 0x61 && c <= 0x66) ) 
		{
			ret = FALSE;
			break;
		}
 	}
 	return ret;
}

static int is_ascii_key_string(char *str, int len)
{
	int i;
	char c;
	int ret = TRUE;
	for(i = 0; i < len; i++)
	{
		c = *(str + i);
		if( c < 0x20 || c > 0x7F)
		{
			ret = FALSE;
		}
	}

	return ret;
}

static unsigned char HexNumberToAscii(unsigned char value)
{
	unsigned char ret;
	if(value < 10){
		ret = value + 0x30; 
	}else if(value < 0x10){
		ret = value + 0x57;
	}else{
		ret = 0;
	}
	return ret;
}

static int check_wep_key_string(char *str, int len, int cur_wep_index, int loop)
{
	if(len != 0 && len != 0+2 && len != 5+2 && len != 10+2 && len != 13+2 && len != 26+2)
	{
		return FALSE;
	}

	if(len == 0)
	{
		if(cur_wep_index == loop)
		{
			return FALSE;
		}
	}
	else if(len == 2)
	{
		if(cur_wep_index == loop)
		{
			return FALSE;
		}
		else if(*str != '\"' || *(str + 1) != '\"')
		{
			return FALSE;
		}
	}
	else 
	{
		if((*str != '\"') || (*(str + len - 1) != '\"'))
		{
			return FALSE;
		}

		if(len == 5+2 || len == 13+2)  //ASCII
		{
			return is_ascii_key_string(str + 1, len - 2);
		}
		else   //hex digit
		{
			return is_Hex_String(str + 1, len - 2);
		}
	}
	
	return 	TRUE;	
}

static int check_valid_channel(const char *country_code, int mode, int channel)
{
	int i;
	
	if(channel == 0)
	{
		return TRUE;
	}

	/*current just check china channel*/
	if((mode == 1) || (mode ==5))         // a/n/ac
	{
		for(i = 0; i < sizeof(CN_5G_channel_list); i++)
		{
			if(	channel == 	CN_5G_channel_list[i])
			{
				return TRUE;
			}
		}
	}

	if(mode > 1 && mode <= 4)           // b  b/g   b/g/n
  	{
		if(channel >= 1 && channel <=13)
		{
			return TRUE;
		}
	}

	return FALSE;
}

static boolean check_valid_apindex(int ap_index)
{
    wifi_mode_type mode;
    if(ap_index < AP_INDEX_0 || ap_index > AP_INDEX_STA)
    {
        return FALSE;
    }
    mode = get_ap_cfg();
    if(mode == AP_MODE)
    {
        if(ap_index != AP_INDEX_0)
        {
            return FALSE;
        }
    }
    else if(mode == APAP_MODE)
    {
        if(ap_index == AP_INDEX_STA)
        {
            return FALSE;
        }
    }
    else
    {
         if(ap_index != AP_INDEX_STA)
        {
            return FALSE;
        }       
    }
    return TRUE;
}
///////////////////////////////////////////////////////////////////////////////
//set the ssid
boolean set_ssid(char *SSID, ap_index_type ap_index)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;

    boolean is_chines = FALSE;
    int chines_cnt = 0;
    int  ssid_param_len;
    int  old_ssid_param_len;
    
    char ssid[65] = {0};
    char mask_flag[8] = {0};

    char ap_index_str[8] = {0};
    
    WIFI_LOG("\n1. set_ssid[%s], ap_index[%d]\n", SSID, ap_index);

    if(!check_valid_apindex(ap_index))
    {
        return FALSE;
    }  
    ssid_param_len = strlen(SSID);
    old_ssid_param_len = ssid_param_len;
    WIFI_LOG("\r\n1. ===========ssid_param_len[%d]\r\n", ssid_param_len);
    {
        int i = 1;
        while(i<old_ssid_param_len-2)
        {
            if(SSID[i]>=0xb0 && SSID[i]<=0xf7 && SSID[i+1]>=0xa1 && SSID[i+1]<=0xfe)
            {
                is_chines = TRUE;
                chines_cnt++;

                ssid_param_len--;
                i += 2;
            }
            else
            {
                i++;
            }
            WIFI_LOG("\r\n==============i=[%d], ssid_param_len[%d]\r\n", i, ssid_param_len);
        }
    }
    WIFI_LOG("\r\n======ssid_param_len[%d], is_chines[%d], chines_cnt[%d]\r\n", ssid_param_len, is_chines, chines_cnt);

    if(chines_cnt > 10)         /* Chines in ssid len 1~10 */
    {
        printf("\r\n1. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
        return FALSE;				
    }

    if(is_chines)
    {
        int max_len = chines_cnt + 2 + (10-chines_cnt)*2;
        WIFI_LOG("\r\n====max_len[%d]\r\n", max_len);
        if(ssid_param_len < 1 || ssid_param_len > max_len)         /* ssid len 1~22(include Chinese and not Chinese) */
        {	
            printf("\r\n2. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
            return FALSE;				
        }
    }
    else                
    {
        if(ssid_param_len < 1 || ssid_param_len > 32)         /* ssid len 1~32 */
        {	
            printf("\r\n3. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
            return FALSE;				
        }
    }

    ssid_param_len = old_ssid_param_len;

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    memset(ap_index_str, 0, sizeof(ap_index_str));
    sprintf(ap_index_str, "%d", ap_index);

    strcpy(ssid, SSID);
    WIFI_LOG("\r\nssid=%s\r\n",ssid);
    //fix bug: set & in ssid, can't write to the hostapd.conf
    {
        int i = 0;
        for(i=0; i<strlen(SSID); i++)
        {
            if(ssid[i] == '&')
            {
                ssid[i] = 8;//convert to BS (backspace, asscii code is 8)
            }
        }
    }

    snprintf(mask_flag,31,"%d",FIELD_MASK_1);   //ssid

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], SETHOSTAPDBASECFG);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_SSID], ssid);
    printf("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									
    WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

//get the ssid
boolean get_ssid(char *str_SSID, ap_index_type ap_index)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;
    
    int value_len = 0;
    char ssid[69] = {0};
    char mask_flag[32];

    char ap_index_str[8] = {0};
    
    WIFI_LOG("\n get_ssid: 1.ap_index[%d]\n", ap_index);
	if(!check_valid_apindex(ap_index))
	{
		printf("check apindex fail");
		return FALSE;
	}

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    memset(ap_index_str, 0, sizeof(ap_index_str));
    sprintf(ap_index_str, "%d", ap_index);

    snprintf(mask_flag,31,"%d",FIELD_MASK_1);   //ssid

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], GETHOSTAPDBASECFG);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }	
	
    WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);
    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_SSID], ssid, &value_len))
    {
        WIFI_LOG("\r\n==============\r\nssid = %s\r\n===========\r\n", ssid);
        if(value_len > 0)
        {
            strcpy(str_SSID, ssid);
        }
        else
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            return FALSE;			
        }
    }

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

//set the auth type and password when connect to the wifi
boolean set_auth(char *str_pwd, int auth_type, int  encrypt_mode, ap_index_type ap_index)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;
    char mask_flag[32] = {0};

    char ap_index_str[8] = {0};
    
    char auth_type_str[16] = {0};
    char encrypt_mode_str[16] = {0};

    char wpa_pw[129]  = {0};
    char wep_pw[4][55];// = {0};

    char cur_wep_index_str[8] = {0};
    int  cur_wep_index;
    int	 i;
    
    WIFI_LOG("\n1. set_auth[%d, %d, %d], ap_index[%d]\n", strlen(str_pwd), 
            auth_type, encrypt_mode,ap_index);

    if(!check_valid_apindex(ap_index))
    {
        return FALSE;
    }

    //auth type
    if(auth_type < AUTH_MIN || auth_type > AUTH_MAX)
    {
        printf("\nauth_type error!\n");
        return FALSE;;			
    }
    sprintf(auth_type_str, "%s", p_auth[auth_type]);
    WIFI_LOG("\n1. auth_type_str[%s]\n", auth_type_str);
    
    //
    if(auth_type >= AUTH_WPA)  // wpa 
    {
        if(encrypt_mode < E_TKIP || encrypt_mode > E_AES_TKIP)           // 		2: "TKIP", 2: "AES", 4: "AES-TKIP"
        {
            printf("\n1. encrypt_mode error\n");
            return FALSE;					
        }
    }
    else   		// WEP
    {
        if(encrypt_mode < E_NULL || encrypt_mode > E_WEP)   
        {
            printf("\n2. encrypt_mode error\n");
            return FALSE;					
        }

        /* share mode need set wep key */
        if(auth_type == AUTH_SHARE && encrypt_mode == E_NULL)
        {
            printf("\n3. encrypt_mode error\n");
            return FALSE;				
        }
    }
    sprintf(encrypt_mode_str, "%s", p_encrypt[encrypt_mode]);
    WIFI_LOG("\n1. encrypt_mode_str[%s]\n", encrypt_mode_str);

    //----------- password ---------------------------------------------
    if( auth_type >= AUTH_WPA )
    {
        int   len;
        char *pw_tmp;
        int i = 0;

        len = strlen(str_pwd);
        pw_tmp = str_pwd;
        WIFI_LOG("\n====str_pwd length[%d]\n", len);
        if(len < 8 || len > 64)         /* key len 8~64 */
        {
            printf("\npwd length error\n");
            return FALSE;				
        }

        for(i=0; i<len; i++)
        {
            if(str_pwd[i] < 32 || str_pwd[i] > 126)
            {
                printf("\nnot char in asscii code 32-126\n");
                return FALSE;
            }
        }

        if(len == 64)
        {
            if(is_Hex_String(pw_tmp, 64) == FALSE)
            {
                printf("\nnot hex string\n");
                return FALSE;						
            }
        }
        else
        {
            if(is_ascii_key_string(pw_tmp, len) == FALSE)
            {
                printf("\nnot ascii string\n");
                return FALSE;						
            }
        }

        for(i = 0; i < len; i++)
        {
            wpa_pw[i * 2]     = HexNumberToAscii(*(uint8*)(pw_tmp + i) / 0x10);
            wpa_pw[i * 2 + 1] = HexNumberToAscii(*(uint8*)(pw_tmp + i) % 0x10);
        }
    }
    else
    {
        int   len;
        char *pw_tmp;

        if(encrypt_mode == E_WEP)
        {
            const int wep_key_num = 1;
            
            cur_wep_index = 1;
            sprintf(cur_wep_index_str, "%d", cur_wep_index);

            for(i = 0; i < wep_key_num; i++)
            {
                int j;
                len = strlen(str_pwd);
                pw_tmp = str_pwd;

                WIFI_LOG("\n==== i = %d, cur_wep_index = %d, len = %d\n",i, cur_wep_index,len);
                WIFI_LOG("\npw_tmp = %s\n", pw_tmp);

                if(FALSE == check_wep_key_string(pw_tmp, len, cur_wep_index, (i + 1)))
                {
                    printf("\nwep key error\n");
                    return FALSE;					
                }
               
                if( len >=5 )
                {
                    for(j = 0; j < len; j++)
                    {
                        wep_pw[i][j * 2] = HexNumberToAscii(*(uint8*)(pw_tmp + j) / 0x10);
                        wep_pw[i][j * 2 + 1] = HexNumberToAscii(*(uint8*)(pw_tmp + j) % 0x10);
                    }					
                }					
            }
        }
    }

    //////////////////////////
    for(i = 0; i < 4; i++)
    {
        memset(wep_pw[i], 0, sizeof(char)*55);
    }

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    memset(ap_index_str, 0, sizeof(ap_index_str));
    sprintf(ap_index_str, "%d", ap_index);

    // ------------  mask  --------------------
    snprintf(mask_flag,31,"%d",FIELD_MASK_3);   // auth set

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], SETHOSTAPDBASECFG);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_SSID], NULL);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_BROADCAST], NULL);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AUTH_TYPE], auth_type_str);

    WIFI_LOG("\r\n=====gggggggggggg encrypt_mode = %d====\r\n",encrypt_mode);

    if(encrypt_mode >= E_TKIP && encrypt_mode <= E_AES_TKIP)
    {
        wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PAIRWISE], p_encrypt[encrypt_mode]);
        wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY_STATUS], NULL);
        wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_DEFAULT_KEY], NULL);
        wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PASSPHRASE], wpa_pw);
    }
    else
    {	
        WIFI_LOG("\r\n======hhhhhhhh 22: cur_wep_index_str[%s], wep_pw[0]=[%s]\r\n", cur_wep_index_str, wep_pw[0]);
        wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PAIRWISE], NULL);
        wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY_STATUS], p_encrypt[encrypt_mode]);
        wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_DEFAULT_KEY], cur_wep_index_str);
        wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PASSPHRASE], NULL);
        if(encrypt_mode == E_WEP)
        {
            wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY0], wep_pw[0]);
            wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY1], wep_pw[1]);
            wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY2], wep_pw[2]);
            wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY3], wep_pw[3]);
        }
    }
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									
    WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

//get the auth type and password when connect to the wifi
boolean get_auth(int *auth_type_ptr, int *encrypt_mode_ptr, char *pwd_str, ap_index_type ap_index)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;
    
    int value_len = 0;
    char mask_flag[32] = {0};

    char auth_type_str[16] = {0};
    int auth_type = 0;
    char encrypt_mode_str[16] = {0};
    int encrypt_mode = 0;

    char cur_wep_index_str[8] = {0};
    int cur_wep_index = 0;

    char ap_index_str[8] = {0};
    
    WIFI_LOG("\n get_auth: 1.ap_index[%d]\n", ap_index);

    if(!check_valid_apindex(ap_index))
    {
        return FALSE;
    }

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    memset(ap_index_str, 0, sizeof(ap_index_str));
    sprintf(ap_index_str, "%d", ap_index);

    snprintf(mask_flag,31,"%d",FIELD_MASK_3);   //auth		
		
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], GETHOSTAPDBASECFG);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);

    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff), 
    							             webcli_wifidaemon_buff, &webcli_wifidaemon_buff_size);
									
    if(ret == FAIL)
    {
        wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
        return FALSE;				
    }		
    WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

    // ----------------------  authentication ------------------------------------
    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AUTH_TYPE], auth_type_str, &value_len))
    {
    	WIFI_LOG("\r\n======sizeof p_auth = %d, auth_type_str[%s]\r\n",sizeof(p_auth) /sizeof(char *), auth_type_str);
    	if(value_len <= 0)
    	{
    	    printf("\n1. ERROR: value_len <= 0\n");
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            return FALSE;					
    	}
    	
    	for(auth_type = 0; auth_type < (sizeof(p_auth) / sizeof(char *)); auth_type++)
    	{
    		if(strncmp(auth_type_str, p_auth[auth_type], strlen(p_auth[auth_type])) == 0)
    		{
    			break;
    		}
    	}
    }

    // -------------------- encrypt mode -------------------------------------------
    // 0: NULL 1: WEP  2: TKIP  3: AES  4: TKIP-AES  
    if(auth_type >= AUTH_WPA)
    {
        if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PAIRWISE], encrypt_mode_str, &value_len))
        {
            if(value_len <= 0)
            {
                printf("\n2. ERROR: value_len <= 0\n");
                wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
                return FALSE;					
            }
        }
    }
    else
    {
        if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY_STATUS], encrypt_mode_str, &value_len))
        {
            if(value_len <= 0)
            {
                printf("\n3. ERROR: value_len <= 0\n");
                wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
                return FALSE;					
            }
        }			
    }

    for(encrypt_mode = sizeof(p_encrypt) / sizeof(char *) - 1; encrypt_mode >= 0; encrypt_mode--)
    {
        if(strncmp(encrypt_mode_str, p_encrypt[encrypt_mode], strlen(p_encrypt[encrypt_mode])) == 0)
        {
            break;
        }
    }	
    
    *auth_type_ptr = auth_type;
    *encrypt_mode_ptr = encrypt_mode;
    // -------------------------- password ------------------------------------------------------------
    WIFI_LOG("\r\n=======auth_type[%d], encrypt_mode[%d]======\r\n", auth_type, encrypt_mode);
    if(auth_type >= AUTH_WPA)
    {
        if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PASSPHRASE], pwd_str, &value_len))
        {
            if(value_len <= 0)
            {
                printf("\n4.ERROR: value_len <= 0\n");
                wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
                return FALSE;					
            }
        }
        //printf("\nwpa_pw[%s]\n", pwd_str);				
    }
    else
    {
        char wep_pw[4][55];
        WIFI_LOG("\n=========encrypt_mode is E_WEP===========\n");
        if(encrypt_mode == E_WEP)       // WEP 
        {
            if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_DEFAULT_KEY], cur_wep_index_str, &value_len))
            {
                if(value_len <= 0)
                {
                    printf("\n5. ERROR: value_len <= 0\n");
                    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
                    return FALSE;					
                }
                cur_wep_index = atoi(cur_wep_index_str);
            }

#ifdef WEP_KEY0_ONLY
            if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY0], wep_pw[0], &value_len))
            {
                if(value_len <= 0)
                {
                    printf("\n6. ERROR: value_len <= 0\n");
                    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
                    return FALSE;					
                }
            }
            strcpy(pwd_str, wep_pw[0]);
#else

            for(i = 0; i < 4; i++)
            {
                if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY0 + i], wep_pw[i], &value_len))
                {
                    if(value_len <= 0)
                    {
                        printf("\n7. ERROR: value_len <= 0\n");
                        wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
                        return FALSE;					
                    }
                }
            }			
            sprintf(pwd_str, ",%d,\"%s\",\"%s\",\"%s\",\"%s\"\n", cur_wep_index,wep_pw[0],wep_pw[1],wep_pw[2],wep_pw[3]);
#endif
        }
        else //no password
        {
            sprintf(pwd_str, "\n");
        }
    }

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

//broadcast setting: 0-disable; 1-enable
boolean set_bcast(int broadcast, ap_index_type ap_index)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;
    
    char mask_flag[8] = {0};

    char ap_index_str[8] = {0};
    
    WIFI_LOG("\n1. set_bcast[%d], ap_index[%d]\n", broadcast, ap_index);
    
    if(!check_valid_apindex(ap_index))
    {
        return FALSE;
    }

    if(broadcast < 0 || broadcast > 1)
    {
        printf("\nERROR: broadcast error!\n");
        return FALSE;			
    }

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    memset(ap_index_str, 0, sizeof(ap_index_str));
    sprintf(ap_index_str, "%d", ap_index);

    snprintf(mask_flag,31,"%d",FIELD_MASK_2);   //broadcast

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], SETHOSTAPDBASECFG);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_SSID], NULL);
    if( broadcast )
    {
        wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_BROADCAST], "enabled");
    }
    else
    {
        wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_BROADCAST], "disabled");
    }
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									
    WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

//get broadcast setting: 0-disable; 1-enable
boolean get_bcast(int *broadcast,ap_index_type ap_index)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;
    
    int value_len = 0;
    char broadcast_str[64] = {0};
    char mask_flag[32];

    char ap_index_str[8] = {0};
    
    WIFI_LOG("\n get_bcast: 1.ap_index[%d]\n", ap_index);

    if(!check_valid_apindex(ap_index))
    {
        return FALSE;
    }

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    memset(ap_index_str, 0, sizeof(ap_index_str));
    sprintf(ap_index_str, "%d", ap_index);

    snprintf(mask_flag,31,"%d",FIELD_MASK_2);   //broadcast	

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], GETHOSTAPDBASECFG);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									


    WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);

    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_BROADCAST], broadcast_str, &value_len))
    {
        printf("\r\n==============\r\nbroadcast_str = %s\r\n===========\r\n", broadcast_str);
        if(value_len <= 0)
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            return FALSE;
        }

        if(strncmp(broadcast_str, "enabled", strlen("enabled")) == 0)
        {
            *broadcast = 1;
        }
        else
        {
            *broadcast = 0;
        }
        WIFI_LOG("\n*broadcast[%d]\n", *broadcast);
    }

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

//set channel and mode
boolean set_channel_mode(int channel, int mode,ap_index_type ap_index)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;
    int i = 0;
    
    char mask_flag[32] = {0};
    char channel_str[8] = {0};	
    char 	hw_mode_str[8] = {0};

    char ap_index_str[8] = {0};
    
    WIFI_LOG("\n1. set_channel_mode[%d], ap_index[%d]\n", channel, ap_index);
    if(!check_valid_apindex(ap_index))
    {
        return FALSE;
    }

    for(i = 0; i < sizeof(channel_number) / sizeof(int); i++)
    {
        if(channel_number[i] == channel)
        {
            break;		
        }
    }

    if(i >= sizeof(channel_number) / sizeof(int))
    {
        printf("\n ERROR: 1. channel error!\n");
        return FALSE;			
    }

    if(mode < 1 || mode > 5)
    {
        printf("\n ERROR: 2. mode error!\n");
        return FALSE;			
    }

    if((mode == 1) || (mode == 5))
    {
        if(channel != 149 && channel != 153 && channel != 157 && channel != 161 && channel != 165)
        {
            printf("\n ERROR: 3. channel error!\n");
            return FALSE;	
        }
    }
    else
    {
        if(channel < 0 || channel > 11)
        {
            printf("\n ERROR: 4. channel error!\n");
            return FALSE;			
        }
    }

    switch( mode )
    {
        case 1:
            hw_mode_str[0] = 'a';
        break;
        
        case 2:
            hw_mode_str[0] = 'b';
        break;	
        
        case 3:
            hw_mode_str[0] = 'g';
        break;	
        
        case 4:
            hw_mode_str[0] = 'n';
        break;
        
        case 5:
            hw_mode_str[0] = 'c';
        break;	
        
        default:
            hw_mode_str[0] = 'n';
        break;
    }

    if(check_valid_channel("CN", mode, channel) == FALSE)
    {
        printf("\n ERROR: 5. channel error!\n");
        return FALSE;			
    }

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    memset(ap_index_str, 0, sizeof(ap_index_str));
    sprintf(ap_index_str, "%d", ap_index);

    snprintf(mask_flag,31,"%d",FIELD_MASK_3 | FIELD_MASK_2);   //hw_mode and channel

    sprintf(channel_str, "%d", channel);

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_PAGE], SETHOSTAPDEXTCFG);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_AP_INDEX], ap_index_str);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_COUNTRY], NULL);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_CHANNEL], channel_str);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_HW_MODE], hw_mode_str);
  
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));
    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									
    WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

//get channel and mode
boolean get_channel_mode(char *channel_str, int *mode,ap_index_type ap_index)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;
    
    int value_len = 0;
    char mask_flag[32];
    char 	hw_mode_str[8] = {0};
    
    char ap_index_str[8] = {0};
    
    WIFI_LOG("\n get_channel_mode: 1.ap_index[%d]\n", ap_index);
    if(!check_valid_apindex(ap_index))
    {
        return FALSE;
    }

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    memset(ap_index_str, 0, sizeof(ap_index_str));
    sprintf(ap_index_str, "%d", ap_index);

    snprintf(mask_flag,31,"%d",FIELD_MASK_3 | FIELD_MASK_2);   //hw_mode and channel

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_PAGE], GETHOSTAPDEXTCFG);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_AP_INDEX], ap_index_str);
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									


    WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);
    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_HW_MODE], hw_mode_str, &value_len))
    {
        if(value_len <= 0)
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            return FALSE;
        }

        if(value_len > 0)
        {
            if(strncmp(hw_mode_str, "n", 1) == 0)
            {
                *mode = 4;
            }
            else if(strncmp(hw_mode_str, "g", 1) == 0)
            {
                *mode = 3;
            }
            else if(strncmp(hw_mode_str, "b", 1) == 0)
            {
                *mode = 2;
            }
            else if(strncmp(hw_mode_str, "a", 1) == 0)
            {
                *mode = 1;
            }				
        }
    }
    
    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_CHANNEL], channel_str, &value_len))
    {
        if(value_len <= 0)
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            return FALSE;
        }
    }

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

//get the current DHCP configuration
boolean get_dhcp(char *host_ip_str, char *start_ip_str, char *end_ip_str, char *time_str)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;
    
    int value_len = 0;
    
    WIFI_LOG("\n get_dhcp: 1.\n");

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, dhcp_config_str[DHCP_PAGE_PAGE], GETDHCP_CONFIG);
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									


    WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);

    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, dhcp_config_str[DHCP_PAGE_GW_IP], host_ip_str, &value_len))
    {
        if(value_len <= 0)
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            printf("\n 1. host_ip_str error!\n");
            return FALSE;
        }
    }

    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, dhcp_config_str[DHCP_PAGE_DHCP_START_IP], start_ip_str, &value_len))
    {
        if(value_len <= 0)
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            printf("\n 2. start_ip_str error!\n");
            return FALSE;
        }
    }

    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, dhcp_config_str[DHCP_PAGE_DHCP_END_IP], end_ip_str, &value_len))
    {
        if(value_len <= 0)
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            printf("\n 3. end_ip_str error!\n");
            return FALSE;
        }
    }

    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, dhcp_config_str[DHCP_PAGE_DHCP_LEASE_TIME], time_str, &value_len))
    {
        if(value_len <= 0)
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            printf("\n 4. time_str error!\n");
            return FALSE;
        }
    }

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

//get the client number that connected to the wifi
int get_client_count(ap_index_type ap_index)
{
    int	client_num;
    int	interface_num = 0;
    char command[100]={0};
    char buf[16];
    FILE *fp;

    if(!check_valid_apindex(ap_index))
    {
    	printf("invalid index\n");
        return FALSE;
    }

    if(ap_index == AP_INDEX_STA)
    {
        interface_num = 1;  //wlan1
    }
    else if(ap_index == AP_INDEX_1)
    {
        interface_num = 1;  //wlan1
    }
    else if(ap_index == AP_INDEX_0)
    {
        interface_num = 0;  //wlan0
    }

    sprintf(command, "hostapd_cli -i wlan%d all_sta | awk -v RS='AUTHORIZED' 'END {print --NR}'", interface_num);
    WIFI_LOG("command=%s",command);

    fp=popen(command,"r");
    if(fp == NULL)
    {
        perror("\n popen error! \n");
        return 0;	
    }

	//printf("buf %s\n",buf);
    fgets(buf,sizeof(buf),fp);
    pclose(fp); 

 //   endstr = strchr(buf,'\n');
  //  if(endstr != NULL)
//    {
 //       *endstr = 0;
//    }

    client_num = atoi(buf);
    if(client_num < 0)    //at+cwmap=0
        client_num = 0;
    
    return client_num;
}

//enable or disable AP
static boolean set_map(int map_enable)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;

    char map_enable_str[sizeof(int) + 1] ={0};
    char mask_flag[sizeof(int) + 1] = {0};      // FIELD_MASK_1 : set   FIELD_MASK_2: get	
    
    if(map_enable < 0 || map_enable > 1)
    {
        printf("\n map_enable value invalid! \n");
        return FALSE;			
    }

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    snprintf(map_enable_str,sizeof(int),"%d",map_enable);
    snprintf(mask_flag,sizeof(int),"%d",FIELD_MASK_1); 

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, map_status_string[MAP_STATUS_PAGE], SETMOBILEAP);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, map_status_string[MAP_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, map_status_string[MAP_STATUS], map_enable_str);
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									
    WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

//get AP status
static boolean get_map_status(uint8 *flag)
{
    FILE *fp = NULL; 

    fp = fopen("/data/mifi/cwmap.log", "r");
    if(fp == NULL)
    {
        printf("\nERROR: ====== fopen[/data/mifi/cwmap.log] failed\n");
        return FALSE;
    }
    else
    {
        fread(flag, 1, 1, fp);
        fclose(fp);
    }

    return TRUE;
}

//get net status
static boolean get_net_status(char *net_enable_str)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;
    
    int value_len = 0;
    //char net_enable_str[sizeof(int) + 1] ={0};
    char mask_flag[sizeof(int) + 1] = {0};      // FIELD_MASK_1 : set   FIELD_MASK_2: get	
    
    WIFI_LOG("\n get_net_status: 1.]\n");

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    snprintf(mask_flag,sizeof(int),"%d",FIELD_MASK_2);  

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, net_status_string[NET_STATUS_PAGE], NETCNCT);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, net_status_string[NET_MASK], mask_flag);
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									

    WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);
    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, net_status_string[NET_STATUS], net_enable_str, &value_len))
    {
        WIFI_LOG("\r\n==============\r\net_enable_str = %s\r\n===========\r\n", net_enable_str);
        if(value_len <= 0)
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            return FALSE;
        }
    }

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

//get the user name and password that used in the authtica
static boolean wifi_daemon_read_usr_name_pwd(char *usr_name, int name_len, char *usr_pwd, int pwd_len)
{
    char *usr_name_pwd_file_name = "/data/mifi/usr_name_pwd.cfg";
    FILE *fp = NULL;
    size_t read_size;

    char *get_ptr;
    
    boolean ret = TRUE;

    WIFI_LOG("\n wifi_daemon_read_usr_name_pwd[%s], [%d, %d] \n", usr_name_pwd_file_name, name_len, pwd_len);
    if(usr_name == NULL || usr_pwd == NULL)
    {
        printf("/n ERROR: usr_name or usr_pwd is NULL \n");
        return FALSE;
    }
    
    fp = fopen(usr_name_pwd_file_name, "r");
    if(fp == NULL)
    {
        printf(" /n ERROR: ====== fopen[%s] failed \n", usr_name_pwd_file_name);
        return FALSE;
    }

    do
    {
        get_ptr = fgets(usr_name, name_len, fp);
       WIFI_LOG("\n read user name[%d]\n", read_size);
        if (!ferror(fp) && (read_size > 0))
        {
            usr_name[strlen(usr_name)-1]=0;
        }
        else
        {
            printf("\n 1. ERROR: ====== fread failed \n");
            ret = FALSE;
            break;
        }

        get_ptr = fgets(usr_pwd, pwd_len, fp);
        WIFI_LOG("\n read user pwd[%d]\n", read_size);
        if (!ferror(fp) && (read_size > 0))
        {
            usr_pwd[strlen(usr_pwd)-1]=0;
        }
        else
        {
            printf("\n 2. ERROR: ====== fread failed\n");
            ret = FALSE;
            break;
        }
    }while(0);
    
    fclose(fp);
    return ret;
}

static boolean wifi_daemon_save_usr_name_pwd(char *usr_name, char *usr_pwd)
{
    char *usr_name_pwd_file_name = "/data/mifi/usr_name_pwd.cfg";
    FILE *fp = NULL;
    size_t read_size;

    char buf[130] = {0};

    boolean ret = TRUE;

    WIFI_LOG("\n wifi_daemon_save_usr_name_pwd[%s] \n", usr_name_pwd_file_name);
    if(usr_name == NULL || usr_pwd == NULL)
    {
        printf("\n usr_name or usr_pwd is NULL \n");
        return FALSE;
    }
    
    fp = fopen(usr_name_pwd_file_name, "w");
    if(fp == NULL)
    {
        printf("\n ERROR: ====== fopen[%s] failed \n", usr_name_pwd_file_name);
        return FALSE;
    }

    do
    {
        //sprintf(buf, "%s\n", usr_name);
        sprintf(buf, "%s", usr_name);
        read_size = fputs(buf/*usr_name*/, fp);
        WIFI_LOG("\n write user name[%d] \n", read_size);
        if (!ferror(fp) && (read_size > 0))
        {
            WIFI_LOG("\n write user name OK \n");
        }
        else
        {
            printf("\n ERROR: ====== fputs failed \n");
            ret = FALSE;
            break;
        }

        memset(buf, 0, sizeof(buf));
        //sprintf(buf, "%s\n", usr_pwd);
        sprintf(buf, "%s", usr_pwd);
        read_size = fputs(buf/*usr_pwd*/, fp);
        WIFI_LOG("\n write user pwd[%d] \n", read_size);
        if (!ferror(fp) && (read_size > 0))
        {
             WIFI_LOG("\n 2. write user pwd OK \n");
        }
        else
        {
            printf("\n ERROR: ====== fputs failed \n");
            ret = FALSE;
            break;
        }
    }while(0);
    
    fclose(fp);
    return ret;
}

//set the usr name and password used when wifi data call in CDMA/EVDO net mode
static boolean set_user_name_pwd(char *sz_usrname, char *sz_usrpwd)
{
    if( strlen(sz_usrname) < 1 || strlen(sz_usrname) > 127
        || strlen(sz_usrpwd) < 1 || strlen(sz_usrpwd) > 127)
    {
        printf("\n usr name or pwd length error!\n");
        return FALSE;
    }

    if( !wifi_daemon_save_usr_name_pwd(sz_usrname, sz_usrpwd) )
    {
        printf("\n wifi_daemon_save_usr_name_pwd error \n");
        return FALSE;
    }

    return TRUE;
}

//get the usr name and password used when wifi data call in CDMA/EVDO net mode
static boolean get_user_name_pwd(char *sz_usrname, int len_name, char *sz_usrpwd, int len_pwd)
{
    if( !wifi_daemon_read_usr_name_pwd(sz_usrname, len_name, sz_usrpwd, len_pwd) )
    {
        strcpy(sz_usrname, "ctnet@mycdma.cn");
        strcpy(sz_usrpwd, "vnet.mobi");
    }

    return TRUE;
}

//get the mac address
boolean get_mac_addr(char *mac_addr, ap_index_type ap_index)
{
    int  num = 0;
    int  interface_num = 0;
    char command[100]={0};
    char buf[100]={0};
    char tmp[30]={0};
    
    char *endstr; 
    FILE *fp;

    if(ap_index == AP_INDEX_STA)
    {
        interface_num = 1;  //wlan1
    }
    else if(ap_index == AP_INDEX_1)
    {
        interface_num = 1;  //wlan1
    }
    else if(ap_index == AP_INDEX_0)
    {
        interface_num = 0;  //wlan0
    }

    //AP mac
    sprintf(command, "ifconfig wlan%d | grep \"HWaddr\" | awk '{ print $5}'", interface_num);
    WIFI_LOG("command=%s",command);

    fp=popen(command,"r");
    if(fp == NULL)
    {
        printf("\n get_mac_addr: 1. popen error! \n");
        return FALSE;	
    }

    fgets(buf,sizeof(buf),fp); 
    pclose(fp);

    endstr = strchr(buf,'\n');
    if(endstr != NULL)
    {
        *endstr = 0;  
    }

    if(strlen(buf) == 0)
    {
        printf("\n get_mac_addr: 2. buf length is 0! \n");
        return FALSE;
    }

    sprintf(tmp, "%d,%s\r\n", num, buf);
    num ++;
    strcpy(mac_addr, tmp);
    WIFI_LOG("\nget_mac_addr: 1.num[%d]\n", num);

    //client mac
    sprintf(command, "hostapd_cli -i wlan%d all_sta | grep \"dot11RSNAStatsSTAAddress\" | awk -F= '{print $2}'", interface_num);
    WIFI_LOG("command=%s",command);

    fp=popen(command,"r");
    if(fp == NULL)
    {
        printf("\n get_mac_addr: 3. popen error! \n");
        return FALSE;    
    }

    while(!feof(fp))
    {
        memset(buf, 0, 100);
        fgets(buf,sizeof(buf),fp);

        endstr = strchr(buf,'\n');
        if(endstr != NULL)
        {
            *endstr = 0;  
        }
        
        if(strlen(buf) == 0)
        {
            break;
        }
        
        sprintf(mac_addr, "%d,%s\r\n", num, buf);
        num ++;
    } 
    pclose(fp);
    WIFI_LOG("\nget_mac_addr: 2.num[%d]\n", num);

    return TRUE;
}

#ifndef WIFI_RTL_DAEMON
/*======================
get_ap_cfg 
NOTE: this api is not avaliable in W58L
=======================*/
wifi_mode_type get_ap_cfg()
{
    FILE *fp = NULL;
    size_t read_size;
    char read_buf[2] = {0};
    wifi_mode_type mode = AP_MODE;

    //check wifi mode
    fp = fopen(MAP_CFG_FILE, "r");
    if(fp)
    {
        read_size = fread(read_buf,1,1,fp);
        if (!ferror(fp) && (read_size > 0))
        {
            if(read_buf[0] == 0x30)   // 0: AP 1: AP-AP 2:AP-STA
            {
                mode = AP_MODE;
            }
            else if(read_buf[0] == 0x31)   // 0: AP 1: AP-AP 2:AP-STA
            {
                mode = APAP_MODE;
            }
            else if(read_buf[0] == 0x32)   // 0: AP 1: AP-AP 2:AP-STA
            {
                mode = APSTA_MODE;
            }
        }
        fclose(fp);
    }
    //enable ssid2, configselect
    return mode; 	
}

/*======================
set_ap_cfg 
NOTE: this api is not avaliable in W58L
=======================*/
static boolean set_ap_cfg(wifi_mode_type mode)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;
    char mask_flag[sizeof(int) + 1] = {0};      // FIELD_MASK_3 : set   FIELD_MASK_2: get	
    char wlan_mode_str[sizeof(int) + 1] = {0};
    char cfg_str[16]= {0};
    FILE *fp = NULL;
    size_t write_size;

    //write to mobileap_cfg.xml
    snprintf(mask_flag, sizeof(int),"%d", FIELD_MASK_3);  
    snprintf(wlan_mode_str, sizeof(int), "%d",(int)mode + 1);  
    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
        return FALSE;
    }

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_PAGE], SETLAN_CONFIG);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_LAN_AP_GW_IP], "");
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_LAN_AP_SUBNET_MASK], "");
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_LAN_AP_DHCP_MODE], "");
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_LAN_AP_DHCP_START_IP], "");
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_LAN_AP_DHCP_END_IP], "");
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_LAN_AP_DHCP_LEASE_TIME], "");
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_WLAN_ENABLE_DISABLE], "");
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_WLAN_MODE], wlan_mode_str);

    WIFI_LOG("\nsend_str:%s len=%d\n", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff), 
                                                                webcli_wifidaemon_buff, &webcli_wifidaemon_buff_size);  
    if(ret == FAIL)
    {
        wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
        return FALSE;               
    }                                           
    WIFI_LOG("\nrecv_str = %s\n", webcli_wifidaemon_buff);
    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);

    //write file MAP_CFG_FILE
    fp = fopen(MAP_CFG_FILE, "w");
    if(fp == NULL)
    {
        printf("\n set_ap_cfg ERROR: open [%s] file error", MAP_CFG_FILE);
        return FALSE;
    }
    sprintf(cfg_str,"%d",mode);

    write_size = fwrite(cfg_str,1,1,fp);
    if (ferror(fp) || (write_size <= 0))
    {
        printf("\n set_ap_cfg ERROR: write to [%s] file error", MAP_CFG_FILE);
        return FALSE;
    }
    fclose(fp);

    ds_system_call("sync",strlen("sync")); 
              
    return TRUE;
}
#endif /*WIFI_RTL_DAEMON*/

#ifdef WIFI_RTL_DAEMON
/*======================
sta_init: open/close station mode

NOTE: this api is only avaliable in W58L
=======================*/
static boolean sta_init(int sta_enable)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;

    char sta_enable_str[sizeof(int) + 1] ={0};
    char mask_flag[sizeof(int) + 1] = {0};      // FIELD_MASK_1 : set   FIELD_MASK_2: get	
    
    if(sta_enable < 0 || sta_enable > 1)
    {
        printf("\n map_enable value invalid! \n");
        return FALSE;			
    }

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    snprintf(sta_enable_str,sizeof(int),"%d",sta_enable);
    snprintf(mask_flag,sizeof(int),"%d",FIELD_MASK_1); 

    wifi_daemon_set_key_value(wifidaemon_webcli_buff,  rtl_stainit_status_string[STA_INIT_STATUS_PAGE], RTLSTAINIT);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, rtl_stainit_status_string[STA_INIT_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, rtl_stainit_status_string[STA_INIT_STATUS], sta_enable_str);
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									
    WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

/*======================
sta_init: get sta status

NOTE: this api is only avaliable in W58L
=======================*/
static boolean get_sta_status(uint8 *flag)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;

    char sta_init_str[sizeof(int) + 1] ={0};
    int  value_len;
    char mask_flag[sizeof(int) + 1] = {0};      // FIELD_MASK_1 : set   FIELD_MASK_2: get	

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    snprintf(mask_flag,sizeof(int),"%d",FIELD_MASK_2); 

    wifi_daemon_set_key_value(wifidaemon_webcli_buff,  rtl_stainit_status_string[STA_INIT_STATUS_PAGE], RTLSTAINIT);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, rtl_stainit_status_string[STA_INIT_MASK], mask_flag);
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									
    WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

     if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, rtl_stainit_status_string[STA_INIT_STATUS], sta_init_str, &value_len))
    {
        WIFI_LOG("\r\n==============\r\nsta_init_str = %s\r\n===========\r\n", sta_init_str);
        if(value_len <= 0)
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            return FALSE;
        }

        *flag = atoi(sta_init_str);
        WIFI_LOG("\n*flag[%d]\n", *flag);
    }

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}
#endif

boolean get_sta_ip(char *ip_str, int len)
{
     char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;

    char   sta_ip_str[30] = {0};
    int value_len = 0;
    char tmp[30]={0};

    char mask_flag[32];

    if(get_ap_cfg() != APSTA_MODE)
    {
        return FALSE;
    }
    
    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
        return FALSE;
    }

    snprintf(mask_flag,31,"%d",FIELD_MASK_1);   

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, sta_ip_string[STA_IP_PAGE], GETSTAIP);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, sta_ip_string[STA_IP_MASK], mask_flag);
    WIFI_LOG("====\nsend_str:%s len=%d\n", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(   wifidaemon_webcli_buff, 
    					strlen(wifidaemon_webcli_buff), 
    					webcli_wifidaemon_buff, 
    					&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
        wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
        return FALSE;				
    }

    WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);
    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, sta_ip_string[STA_IP_ADDR], sta_ip_str, &value_len))
    {
        if(value_len < 0)
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            return FALSE;			
        }
    }

    WIFI_LOG("\n====sta_ip_str[%s]====\n", sta_ip_str);

    strncpy(ip_str, sta_ip_str, len);
    return TRUE;
}

boolean set_sta_cfg(char *ssid_value, char *psk_value)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;
    
    char ssid[67] = {0};
    int  ssid_param_len;
    int  old_ssid_param_len;
    boolean is_chines = FALSE;
    char mask_flag[8] = {0};

    char ap_index_str[8] = {0};
    
    WIFI_LOG("\n1. set_sta_cfg[%s],\n", ssid_value);
    if(get_ap_cfg() != APSTA_MODE)
    {
        return FALSE;
    }

    ssid_param_len = strlen(ssid_value);
	if(ssid_param_len < 1)
	{
		return FALSE;
	}
    old_ssid_param_len = ssid_param_len;

    WIFI_LOG("\r\n1. ===========ssid_param_len[%d]\r\n", ssid_param_len);
    {
        int i = 0;
        while(i<old_ssid_param_len)
        {
            WIFI_LOG("\r\n========[%0x, %0x]\r\n", ssid_value[i], ssid_value[i+1]);
            if((ssid_value[i] & 0x80) && (ssid_value[i+1] & 0x80))
            {
                is_chines = TRUE;

                ssid_param_len--;
                i += 2;
            }
            else
            {
                i++;
            }
            WIFI_LOG("\r\n==============i=[%d], ssid_param_len[%d]\r\n", i, ssid_param_len);
        }
    }
    
    WIFI_LOG("\r\n======ssid_param_len[%d], is_chines[%d]\r\n", ssid_param_len, is_chines);
    if(is_chines)
    {
        if(ssid_param_len < 1 || ssid_param_len > 10)         /* ssid len 1~10 */
        {
            printf("\r\n1. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
            return FALSE;				
        }
    }
    else
    {
        if(ssid_param_len < 1 || ssid_param_len > 32)         /* ssid len 1~32 */
        {	
            printf("\r\n2. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
            return FALSE;				
        }
    }
              
    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    memset(ap_index_str, 0, sizeof(ap_index_str));
    sprintf(ap_index_str, "%d", 2);

    strcpy(ssid, ssid_value);
    WIFI_LOG("\r\nssid=%s\r\n",ssid);
    //fix bug: set & in ssid, can't write to the hostapd.conf
    {
        int i = 0;
        for(i=0; i<strlen(ssid_value); i++)
        {
            if(ssid[i] == '&')
            {
                ssid[i] = 8;//convert to BS (backspace, asscii code is 8)
            }
        }
    }

    snprintf(mask_flag,31,"%d",FIELD_MASK_1);   //ssid

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_PAGE], SETWPASUPPLICANTCFG);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_AP_INDEX], ap_index_str);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_SSID], ssid);

    if(psk_value != NULL && strlen(psk_value) >= 5 &&  strlen(psk_value) <= 64)
    { 
	    char sim_psk_value[67] = {0};
    	wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_WEP_KEY_STATUS], NULL);
        wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_AUTH_TYPE], NULL);
        
        printf("psk_value = %s\n", psk_value);
        #if 0
        sim_psk_value[0] = '\"';
        strcpy(sim_psk_value+1, psk_value);
        sim_psk_value[strlen(psk_value) + 1] = '\"';
        #else
        strcpy(sim_psk_value, psk_value);
        #endif
        
        printf("sim_psk_value = %s\n", sim_psk_value);
        wifi_daemon_set_key_value(wifidaemon_webcli_buff, "basecfg_wpa_passphrase"/*wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_WPA_PASSPHRASE]*/, sim_psk_value);
    }
        
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									
    WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

boolean get_sta_cfg(char *ssid_str, char *psk_value)
{
    char * wifidaemon_webcli_buff = NULL;
    char * webcli_wifidaemon_buff = NULL;
    int webcli_wifidaemon_buff_size;
    int ret;
    
    int value_len = 0;
    char vptr[69] = {0};
    char mask_flag[32];

    char ap_index_str[8] = {0};
    
    if(get_ap_cfg() != APSTA_MODE)
    {
        return FALSE;
    }

    if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
    {
    	return FALSE;
    }

    memset(ap_index_str, 0, sizeof(ap_index_str));
    sprintf(ap_index_str, "%d", AP_INDEX_STA);

    snprintf(mask_flag,31,"%d",FIELD_MASK_1);   //ssid

    wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_PAGE], GETWPASUPPLICANTCFG);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_MASK], mask_flag);
    wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_AP_INDEX], ap_index_str);
    WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

    ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
    									strlen(wifidaemon_webcli_buff), 
    									webcli_wifidaemon_buff, 
    									&webcli_wifidaemon_buff_size);
    if(ret == FAIL)
    {
    	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    	return FALSE;				
    }									


    WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);
    //ssid
    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_SSID], vptr, &value_len))
    {
        WIFI_LOG("\r\n==============\r\nssid = %s\r\n===========\r\n", vptr);
        if(value_len > 0)
        {
            strcpy(ssid_str, vptr);
        }
        else
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            return FALSE;				
        }
    }

    //psk
    memset(vptr, 0, sizeof(vptr));
    if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_WPA_PASSPHRASE], vptr, &value_len))
    {
        WIFI_LOG("\r\n==============\r\npsk = %s\r\n===========\r\n", vptr);
        if(value_len > 0)
        {
            strcpy(psk_value, vptr);
        }
        else
        {
            wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
            return FALSE;				
        }
    }

    wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
    return TRUE;
}

#define SCAN_TEMP_FILE "/data/mifi/scan_results_temp"
boolean sta_scan(char *list_str)
{
    char command[100]={0};
    char buf[100]={0};
    //char tmp[100]={0};
    FILE *fp;
    char *endstr;
    int i,count = 0;

    if(get_ap_cfg() != APSTA_MODE)
    {
        return FALSE;
    }

    //command
    system("wpa_cli scan");
    sprintf(command, "wpa_cli scan_result > %s", SCAN_TEMP_FILE);
    system(command);

    //count
    memset(command, 0, sizeof(command));
    sprintf(command, "wc -l %s | awk '{ print $1}'", SCAN_TEMP_FILE);

    fp=popen(command,"r");

    if(fp == NULL)
    {
        printf("\n 1. sta_scan ERROR: popen faile \n");
        return FALSE;	
    }

    fgets(buf,sizeof(buf),fp); 
    pclose(fp);

    count = atoi(buf);
    if(count < 3)
    {
        printf("\n 2. sta_scan ERROR: count[%d] is less than 3\n", count);
        return FALSE;
    }

    strcpy(list_str, "\r\n");

    for(i=3; i<=count; i++)
    {
        //bssid
     /*   memset(command, 0, sizeof(command));
        sprintf(command, "sed -n \"%d,%dp\" %s | awk '{ print $1}'", i, i, SCAN_TEMP_FILE);

        fp=popen(command,"r");

        if(fp == NULL)
        {
            printf("\n 3. sta_scan ERROR: popen faile \n");
            return FALSE;	
        }
        
        memset(buf, 0, sizeof(buf));
        fgets(buf,sizeof(buf),fp); 
        pclose(fp);
        endstr = strchr(buf,'\n');
        if(endstr != NULL)
        {
            *endstr = 0;  
        }			

        strcat(list_str, buf);*/

        //ssid
        memset(command, 0, sizeof(command));
        sprintf(command, "sed -n \"%d,%dp\" %s | awk '{ print $5}'", i, i, SCAN_TEMP_FILE);

        fp=popen(command,"r");

        if(fp == NULL)
        {
            printf("\n 4. sta_scan ERROR: popen faile \n");
            return FALSE;	
        }
        memset(buf, 0, sizeof(buf));
        fgets(buf,sizeof(buf),fp); 
        pclose(fp);
        endstr = strchr(buf,'\n');
        if(endstr != NULL)
        {
            *endstr = 0;  
        }			

        strcat(list_str, buf);
        strcat(list_str, "\r\n");
    }
}

//restore the wifi settings
void restore_wifi()
{
    char command[100] = {0};
    char command1[100] = {0};

    sprintf(command, "%s", "cp /etc/default_mobileap_cfg.xml /data/mobileap_cfg.xml"); 
    ds_system_call(command, strlen(command));

    sprintf(command1,"%s", "cp /etc/default_hostapd.conf /data/mifi/hostapd.conf");
    ds_system_call(command1, strlen(command1));

    //for enablessid2: AP-AP mode
    sprintf(command1,"%s", "cp /etc/default_hostapd-wlan1.conf /data/mifi/hostapd-wlan1.conf");
    ds_system_call(command1, strlen(command1));

    //for enablessid2: AP-STA mode
    sprintf(command1,"%s", "cp /etc/default_sta_mode_hostapd.conf /data/mifi/sta_mode_hostapd.conf");
    ds_system_call(command1, strlen(command1));
    sprintf(command1,"%s", "cp /etc/default_wpa_supplicant.conf /data/mifi/wpa_supplicant.conf");
    ds_system_call(command1, strlen(command1));

    sprintf(command, "rm %s", MAP_CFG_FILE);
    ds_system_call(command, strlen(command));

    memset(command, 0, sizeof(command));
    sprintf(command, "rm %s", MAP_FIX_CFG_FILE);
    ds_system_call(command, strlen(command));

    ds_system_call("sync", strlen("sync"));		

    ds_system_call("shutdown -r now", strlen("shutdown -r now"));
}

static boolean select_ap_index(ap_index_type *ap_index)
{
    char ap_index_str[64] = {0};
    wifi_mode_type mode = get_ap_cfg();
    int ap_index_val;
    if(mode == APAP_MODE)
    {
        printf("Please input the ap_index: ");
        fgets(ap_index_str, sizeof(ap_index_str), stdin);
           
        ap_index_val == atoi(ap_index_str);
        if(ap_index_val != AP_INDEX_0 && ap_index_val != AP_INDEX_1)
        {
            printf("ap_index input wrong!\r\n");
            return FALSE;
        }
        else
        {
            *ap_index = (ap_index_type)ap_index_val;
        }
    }
    else if(mode == APSTA_MODE)
    {
        *ap_index = AP_INDEX_STA;
    }
    else if(mode == AP_MODE)
    {
        *ap_index = AP_INDEX_0;
    }
    else
    {
         printf("ap mode is invalid!\r\n");
         return FALSE;
    }

    return TRUE;
}

#if 0
int main_no_use(int argc,char *argv[])
{
    char scan_string[100] = {0};
    int opt = 0;
    boolean ret = FALSE;
    int i = 0;
    int j = 0;
    int array_size = sizeof(wifi_daemon_options_list)/sizeof(wifi_daemon_options_list[0]);
    boolean be_exit = FALSE;

    while(TRUE)
    {
        /* Display menu of options. */
        printf("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
        printf("\nPlease select an option to test from the items listed below.\n\n");

        for (i=0; i<(array_size/2); i++)
        {
            printf("%s",wifi_daemon_options_list[i]);
            for(j=30-strlen(wifi_daemon_options_list[i]); j>0; j--)
            {
                printf(" ");
            }
            printf("%s\n",wifi_daemon_options_list[i+(array_size/2)]);
        }
        printf("Option > ");
         
         /* Read the option from the standard input. */
        memset(scan_string, 0, sizeof(scan_string));
        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
            continue;

        /* Convert the option to an integer, and switch on the option entered. */
        opt = atoi(scan_string);
        switch(opt)
        {
            case 1:
                {
                    char ssid[65] = {0};
                    ap_index_type    ap_index;
                    wifi_mode_type   mode;
                    printf("   Please input the SSID: ");
                    memset(scan_string, 0, sizeof(scan_string));
                    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                        continue;

                    memcpy(ssid, scan_string, strlen(scan_string)-1);

                    mode = get_ap_cfg();
                    if(mode == APAP_MODE)
                    {
                         printf("   Please input the index: ");
                         memset(scan_string, 0, sizeof(scan_string));
                         if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                             continue;
                          ap_index == atoi(scan_string);
                        if(ap_index != AP_INDEX_0 && ap_index != AP_INDEX_1)
						  {
						    // error index
						  	break;
						  }
                    }
                    else if(mode == AP_MODE)
                    {
                        ap_index = AP_INDEX_0;
                    }
                    else
                    {
                        ap_index =  AP_INDEX_STA;
                    }
                    ret = set_ssid(ssid, ap_index);
                    if(ret)
                    {
                        printf("\nset SSID success!\n");
                    }
                    else
                    {
                        printf("\nset SSID failed!\n");
                    }
                }
                break;

            case 2:
                {
                    char ssid[69] = {0};
                    ap_index_type ap_index;
                    if(!select_ap_index(&ap_index))
                    {
                        break;
                    }
                    ret = get_ssid(ssid, ap_index);
                    if(ret)
                    {
                        printf("\nSSID: %s\n", ssid);
                    }
                    else
                    {
                        printf("\nget SSID failed!\n");
                    }
                }
                break;

            case 3:
                {
                    int auth_type = 0;
                    int encrypt_mode = 0;
                    char pwd_str[65] = {0};
                    ap_index_type ap_index;
                    if(!select_ap_index(&ap_index))
                    {
                        break;
                    }             
                    printf("Please input the auth type(%d-%d): ", AUTH_MIN, AUTH_MAX);
                    memset(scan_string, 0, sizeof(scan_string));
                    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                        continue;
                    auth_type = atoi(scan_string);

                    if(auth_type >= AUTH_WPA)  // wpa 
                    {
                        printf("   Please input the encrypt mode(%d-%d): ", E_TKIP, E_AES_TKIP);
                    }
                    else
                    {
                        printf("   Please input the encrypt mode(%d-%d): ", E_NULL, E_WEP);
                    }
                    memset(scan_string, 0, sizeof(scan_string));
                    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                        continue;
                    encrypt_mode = atoi(scan_string);

                    /* share mode need set wep key */
                    if( !(auth_type == AUTH_SHARE && encrypt_mode == E_NULL) )
                    {
                        printf("   Please input the password(8-64): ");
                        memset(scan_string, 0, sizeof(scan_string));
                        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                            continue;

                        memcpy(pwd_str, scan_string, strlen(scan_string)-1);
                    }

                     ret = set_auth(pwd_str, auth_type, encrypt_mode, ap_index);
                    if(ret)
                    {
                        printf("\nset AUTH success!\n");
                    }
                    else
                    {
                        printf("\nset AUTH failed!\n");
                    }
                }
                break;

            case 4:
                {
                    int auth_type = 0;
                    int encrypt_mode = 0;
                    char pwd_str[65] = {0};
                    ap_index_type ap_index;
                    if(!select_ap_index(&ap_index))
                    {
                        break;
                    }
                    ret = get_auth(&auth_type, &encrypt_mode, pwd_str, ap_index);
                    if(ret)
                    {
                        printf("\nAUTH: %d,%d,\"%s\"\n", auth_type, encrypt_mode, pwd_str);
                    }
                    else
                    {
                        printf("\nget AUTH failed!\n");
                    }
                }
                break;

            case 5:
                {
                    int broadcast = 0;
                    ap_index_type ap_index;
                    if(!select_ap_index(&ap_index))
                    {
                        break;
                    }                  
                    printf("   Please input the broadcast: ");
                    memset(scan_string, 0, sizeof(scan_string));
                    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                        continue;

                    broadcast = atoi(scan_string);

                    ret = set_bcast(broadcast, ap_index);
                    if(ret)
                    {
                        printf("\nset set_bcast success!\n");
                    }
                    else
                    {
                        printf("\nset set_bcast failed!\n");
                    }
                }
                break;

            case 6:
                {
                    int broadcast = 0;
                    ap_index_type ap_index;
                    if(!select_ap_index(&ap_index))
                    {
                        break;
                    }

                    ret = get_bcast(&broadcast, ap_index);
                    if(ret)
                    {
                        printf("\nbroadcast: %d\n", broadcast);
                    }
                    else
                    {
                        printf("\nget broadcast failed!\n");
                    }
                }
                break;

            case 7:
                {
                    int channel = 0;
                    int mode = 0;
                    ap_index_type ap_index;
                    if(!select_ap_index(&ap_index))
                    {
                        break;
                    }                    
                    printf("   Please input the channel: ");
                    memset(scan_string, 0, sizeof(scan_string));
                    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                        continue;
                    channel = atoi(scan_string);

                    
                    printf("   Please input the mode: ");
                    memset(scan_string, 0, sizeof(scan_string));
                    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                        continue;
                    mode = atoi(scan_string);

                    ret = set_channel_mode(channel, mode, ap_index);
                    if(ret)
                    {
                        printf("\nset set_channel_mode success!\n");
                    }
                    else
                    {
                        printf("\nset set_channel_mode failed!\n");
                    }
                }
                break;

            case 8:
                {
                    char channel_str[8] = {0};
                    int mode = 0;
                    ap_index_type ap_index;
                    if(!select_ap_index(&ap_index))
                    {
                        break;
                    }

                    ret = get_channel_mode(channel_str, &mode, ap_index);
                    if(ret)
                    {
                        printf("\n channel_mode: %s,%d\n", channel_str, mode);
                    }
                    else
                    {
                        printf("\n get_channel_mode failed!\n");
                    }
                }
                break;

            case 9:
                {
                    char   host_ip_str[16] = {0};
                    char   start_ip_str[16] = {0};
                    char   end_ip_str[16] = {0};
                    char   time_str[16] = {0};

                    ret = get_dhcp(host_ip_str, start_ip_str, end_ip_str, time_str);
                    if(ret)
                    {
                        printf("\n DHCP: \"%s\",\"%s\",\"%s\",\"%s\"\n", host_ip_str, start_ip_str, end_ip_str, time_str);
                    }
                    else
                    {
                        printf("\n get_dhcp failed!\n");
                    }
                }
                break;

            case 10:
                {
                    ap_index_type ap_index;
                    int count1,count2;
                    wifi_mode_type mode = get_ap_cfg();
                    if(mode == AP_MODE || mode == APSTA_MODE)
                    {
                        printf("\n count: %d\n", count1);
                    }
                    else
                    {
                        count1 = get_client_count(AP_INDEX_0);
                        count1 = get_client_count(AP_INDEX_0);
                        printf("\n AP1_count: %d  AP2_count\n", count1, count2);
                    }
                }
                break;

            case 11:
                {
                    int flag = 0;
                    printf("   Please input the operation( 0-Disable AP; 1-Enable AP): ");
                    memset(scan_string, 0, sizeof(scan_string));
                    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                        continue;
                    flag = atoi(scan_string);

                    ret = set_map(flag);
                    if(ret)
                    {
                        printf("\n set_map success!\n");
                    }
                    else
                    {
                        printf("\n set_map failed!\n");
                    }
                }
                break;

            case 12:
                {
                    uint8 flag = 0;

                    ret = get_map_status(&flag);
                    if(ret)
                    {
                        printf("\n MAP_STATUS: %d\n", flag);
                    }
                    else
                    {
                        printf("\n get_map_status failed!\n");
                    }
                }
                break;

            case 13:
                {
                    char net_enable_str[sizeof(int) + 1] ={0};
                    ret = get_net_status(net_enable_str);
                    if(ret)
                    {
                        printf("\n NET_STATUS: %s\n", net_enable_str);
                    }
                    else
                    {
                        printf("\n get_net_status failed!\n");
                    }
                }
                break;

            case 14:
                {
                    char sz_usrname[128] = {0};
                    char sz_usrpwd[128] = {0};
                    
                    printf("   Please input the user name(length 1-127): ");
                    memset(scan_string, 0, sizeof(scan_string));
                    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                        continue;
                    strncpy(sz_usrname, scan_string, sizeof(sz_usrname)-1);

                    
                    printf("   Please input the password(length 1-127): ");
                    memset(scan_string, 0, sizeof(scan_string));
                    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                        continue;
                    strncpy(sz_usrpwd, scan_string, sizeof(sz_usrpwd)-1);

                    ret = set_user_name_pwd(sz_usrname, sz_usrpwd);
                    if(ret)
                    {
                        printf("\n set_user_name_pwd success!\n");
                    }
                    else
                    {
                        printf("\n set_user_name_pwd failed!\n");
                    }
                }
                break;

            case 15:
                {
                    char sz_usrname[128] = {0};
                    char sz_usrpwd[128] = {0};

                    ret = get_user_name_pwd(sz_usrname, sizeof(sz_usrname), sz_usrpwd, sizeof(sz_usrpwd));
                    if(ret)
                    {
                        printf("\n USER NAME/PASSWORD: \"%s\",\"%s\"\n", sz_usrname, sz_usrpwd);
                    }
                    else
                    {
                        printf("\n get_user_name_pwd failed!\n");
                    }
                }
                break;

            case 16:
                {
                    char mac_addr[1024] = {0};
                    ap_index_type ap_index;
                    if(!select_ap_index(&ap_index))
                    {
                        break;
                    }

                    ret = get_mac_addr(mac_addr, ap_index);
                    if(ret)
                    {
                        printf("\n MAC_ADDR: %s\n", mac_addr);
                    }
                    else
                    {
                        printf("\n get_mac_addr failed!\n");
                    }
                }
                break;
				
#ifdef WIFI_RTL_DAEMON
			case 17:
                {
                    int op = 0;
                    printf("   Please input the station operation: 1 - start station mode; 0 - stop station mode\n");
                    memset(scan_string, 0, sizeof(scan_string));
                    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                        continue;

                    op = atoi(scan_string);

                    ret = sta_init(op);
                    if(ret)
                    {
                        printf("\nset sta_init success!\n");
                    }
                    else
                    {
                        printf("\nset sta_init failed!\n");
                    }
                }
                break;

			case 18:
                {
                    uint8 status = 0;

                    ret = get_sta_status(&status);
                    if(ret)
                    {
                        printf("\nsta status: %d\n", status);
                    }
                    else
                    {
                        printf("\nget status failed!\n");
                    }
                }
                break;
#else
            case 17:
                {
                    wifi_mode_type mode = get_ap_cfg();
                    printf("\n wifi mode: %d,%d \n", mode);
                }
                break;

            case 18:
                {
                    int i = 0;
                    
                    int mode;

                    
                    printf("   Please input the wifi mode (0: AP  1:AP-AP  2:AP-STA) >");
                    memset(scan_string, 0, sizeof(scan_string));
                    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                        continue;

                    mode = atoi(scan_string);
                    if(mode < 0 || mode > 2)
					{
							
							
							
                        printf("input mode wrong");
							break;
					}


                    ret = set_ap_cfg((wifi_mode_type)mode);
                    if(ret)
                    {
                        printf("\n set_ap_cfg success!\n");
                    }
                    else
                    {
                        printf("\n set_ap_cfg failed!\n");
                    }
                }
                break;
#endif /*WIFI_RTL_DAEMON*/
            case 19:
                {
                    char ip_addr[30] = {0};
                    if(get_ap_cfg() != APSTA_MODE)
                    {
                        printf("mode is not AP-STA\r\n");
                        break;
                    }
                    ret = get_sta_ip(ip_addr, sizeof(ip_addr));
                    if(ret)
                    {
                        printf("\n STA_IP: %s\n", ip_addr);
                    }
                    else
                    {
                        printf("\n get_sta_ip failed!\n");
                    }
                }
                break;

            case 20:
                {
                    char ssid[65] = {0};
                    char psk_value[65] = {0};;
                    if(get_ap_cfg() != APSTA_MODE)
                    {
                        printf("mode is not AP-STA\r\n");
                        break;
                    }                 
                    printf("   Please input the ssid string(1-32): ");
                    memset(scan_string, 0, sizeof(scan_string));
                    if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                        continue;

					if(strlen(scan_string) > 32+1)
					{
						printf("\n ssid is too long!\n");
						break;
					}
					
					//ssid[0] = '\"';
                    strncpy(ssid, scan_string, strlen(scan_string)-1);
					//ssid[strlen(ssid)] = '\"';
					WIFI_LOG("\n ssid[%s]\n", ssid);



                        printf("   Please input the password of external wireless network(8-64): ");
                        memset(scan_string, 0, sizeof(scan_string));
                        if (fgets(scan_string, sizeof(scan_string), stdin) == NULL)
                            continue;

					//psk_value[0] = '\"';
                    strncpy(psk_value, scan_string, strlen(scan_string)-1);
					//psk_value[strlen(psk_value)] = '\"';
						WIFI_LOG("\n psk_value[%s]\n", psk_value);

                    ret = set_sta_cfg(ssid, psk_value);
                    if(ret)
                    {
                        printf("\n set_sta_cfg success!\n");
                    }
                    else
                    {
                        printf("\n set_sta_cfg failed!\n");
                    }
                }
                break;

            case 21:
                {
                    char ssid_str[69] = {0};
                    char psk_value[69] = {0};
                    if(get_ap_cfg() != APSTA_MODE)
                    {
                        printf("mode is not AP-STA\r\n");
                        break;
                    }

                    ret = get_sta_cfg(ssid_str, psk_value);
                    if(ret)
                    {
                        printf("\n STA_CFG: %s, %s\n", ssid_str, psk_value);
                    }
                    else
                    {
                        printf("\n get_sta_cfg failed!\n");
                    }
                }
                break;

            case 22:
                {
                    char list_str[300] = {0};
                    if(get_ap_cfg() != APSTA_MODE)
                    {
                        printf("mode is not AP-STA\r\n");
                        break;
                    }

                    ret = sta_scan(list_str);
                    if(ret)
                    {
                        printf("\n STA_SCAN LIST: %s\n", list_str);
                    }
                    else
                    {
                        printf("\n sta_scan failed!\n");
                    }
                }
                break;

            case 23:
                {
                    printf("\nPlease wait to restore...\n");
                    restore_wifi();
                    printf("\nshut down...\n");
                }
                break;

            case 24:
                {
                    be_exit = TRUE;
                    printf("\nexit\n");
                }
                break;

            default:
                printf("\ninvalid selection!\n");
                break;
        };

        if(be_exit)
        {
            break;
        }
    };
     
    printf("\n");
    return 0;
}

#endif






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
int wifi_get_ap_auth(int *authType, int *encryptMode, char *password, ap_index_type ap_index)
{
	int ret;
	//int auth_type = 0;
	//int encrypt_mode = 0;
	//char password[65] = {0};

	ret = get_auth(authType, encryptMode, password, ap_index);
	if(ret)
	{
		//printf("\nAUTH: %d,%d,\"%s\"\n", *authType, *encryptMode, password);
		return 0;
	}
	else
	{
		//printf("\nget AUTH failed!\n");
		return -1;
	}
	
	return 0;
}

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
int wifi_set_ap_auth(int authType, int encryptMode, char *password, ap_index_type ap_index)
{
    int ret;

	if(authType < AUTH_MIN || authType > AUTH_MAX){
		printf("input the auth type error!\n");
		return -1;
	}

    if(authType >= AUTH_WPA)  //use encrypt mode, such as: wpa 
    {
		if(encryptMode < E_TKIP || encryptMode > E_AES_TKIP)
		{
			printf("input the encryptMode error!\n");
		return -1;
    }
    }
	else   //don't use encrypt mode
	{
		if(encryptMode < E_NULL || encryptMode > E_WEP)
		{
			printf("input the encryptMode error!\n");
		return -1;
	}
	}

	ret = set_auth(password, authType, encryptMode, ap_index);
	if(ret)
	{
		printf("\nset AUTH success!\n");
		return 0;
	}
	else
	{
		printf("\nset AUTH failed!\n");
		return -1;
	}
	
	return 0;
}

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
int wifi_get_ap_ssid(char *ssid, ap_index_type ap_index)
{
	int ret = get_ssid(ssid, ap_index);
	if(ret)
	{
		//printf("\nSSID: %s\n", ssid);
		return 0;
	}
	else
	{
		//printf("\nget SSID failed!\n");
		return -1;
	}
	return 0;
}

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
int wifi_set_ap_ssid(char *ssid, ap_index_type ap_index)
{
	int ret = set_ssid(ssid, ap_index);
	if(ret)
	{
		//printf("\nset SSID success!\n");
		return 0;
	}
	else
	{
		//printf("\nset SSID failed!\n");
		return -1;
	}
	return 0;
}

/*****************************************************************************
* Function Name : wifi_init
* Description   : 开启wifi,    1:打开wifi    0:关闭wifi
* Input			: uint8_t wifiMode
* Output        : None
* Return        : 0:success, -1:failed
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int wifi_OpenOrClose(uint8_t isOpen)
{
	int ret = set_map(isOpen);
	if(ret)
	{
		printf("\nwifi_OpenOrClose success!\n");
		return 0;
	}
	else
	{
		printf("\nwifi_OpenOrClose failed!\n");
		return -1;
	}
	
	return 0;
}


/*****************************************************************************
* Function Name : Wifi_Set_Mode
* Description   : 设置wifi模式, 0:Ap mode 1: Ap & AP, 2:Ap & sta
* Input			: uint8_t mode
* Output        : None
* Return        : id
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int Wifi_Set_Mode(uint8 mode)
{
	int ret;
	if(mode < AP_MODE || mode > APSTA_MODE)
	{
		//error
		printf("\ninput mode error!\n");
	}
	
	ret = set_ap_cfg(mode);
	
	if(mode == AP_MODE)
	{
		if(ret)
		{
			printf("\nset wifi Ap mode success!\n");
		}
		else
		{
			printf("\nset wifi Ap mode failed!\n");
		}
	}
	else if(mode == APAP_MODE)
	{
		if(ret)
		{
			printf("\nset wifi Ap & AP mode success!\n");
		}
		else
		{
			printf("\nset wifi Ap & AP mode failed!\n");
		}
	}
	else if(mode == 2)
	{
		if(ret)
		{
			printf("\nset wifi Ap & sta mode success!\n");
		}
		else
		{
			printf("\nset wifi Ap & sta mode failed!\n");
		}
	}

	return 0;
}

/*****************************************************************************
* Function Name : Wifi_AP_AP_Control
* Description   : AP_AP_Control
* Input			: None
* Output        : None
* Return        : id
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void Wifi_AP_AP_Control()
{
	char ssid[69] = {0};
	int authType = 0;
	int encryptMode = 0;
	char password[65] = {0};

	//get the first ap's ssid
	wifi_get_ap_ssid(ssid, 0);
	//wifi_set_ap_ssid("TBox_AP_Demo_0", 0);
	
	wifi_get_ap_auth(&authType, &encryptMode, password, 0);
	wifi_set_ap_auth(authType, encryptMode, "12345678", 0);
	memset(password, 0, sizeof(password));
	wifi_get_ap_auth(&authType, &encryptMode, password, 0);
	
	int count = get_client_count(0);
	printf("\n CLIENT_COUNT: %d\n", count);

	//get the second ap's ssid
	memset(ssid, 0, sizeof(ssid));
	wifi_get_ap_ssid(ssid, 1);
	//wifi_set_ap_ssid("TBox_AP_Demo_1", 1);	
	memset(password, 0, sizeof(password));
	
	wifi_get_ap_auth(&authType, &encryptMode, password, 1);
	wifi_set_ap_auth(authType, encryptMode, "123456789", 1);
	memset(password, 0, sizeof(password));
	wifi_get_ap_auth(&authType, &encryptMode, password, 1);

	count = get_client_count(1);
	printf("\n CLIENT_COUNT: %d\n", count);

}

/*****************************************************************************
* Function Name : Wifi_AP_AP_Control
* Description   : AP_AP_Control
* Input			: None
* Output        : None
* Return        : id
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void Wifi_AP_STA_Control()
{
	char ssid[69] = {0};
	int authType = 5;
	int encryptMode = 4;
	int get_authType, get_encryptMode;
	char password[65] = {0};

	wifi_get_ap_ssid(ssid, AP_INDEX_STA);
	wifi_set_ap_ssid("TBox_AP_STA_Demo", AP_INDEX_STA);
	
	wifi_get_ap_auth(&get_authType, &get_encryptMode, password, AP_INDEX_STA);

	wifi_set_ap_auth(authType, encryptMode, "12345678", AP_INDEX_STA);

	wifi_get_ap_auth(&get_authType, &get_encryptMode, password, AP_INDEX_STA);

	//set_sta_cfg("APC201708230938", "1234567890");
    set_sta_cfg("Yeedon", "123456ab");
		
	char get_sta_ssid_str[128] = {0};
	char get_sta_psk_str[65] = {0};
	get_sta_cfg(get_sta_ssid_str, get_sta_psk_str);
	printf("sta_ssid:%s, sta_psk:%s", get_sta_ssid_str, get_sta_psk_str);
	//sleep(10);
	
	int count = get_client_count(AP_INDEX_STA);
	printf("\n CLIENT_COUNT: %d\n", count);


    char list_str[300] = {0};
    if(get_ap_cfg() != APSTA_MODE)
    {
        printf("mode is not AP-STA\r\n");
    }

    int ret = sta_scan(list_str);
    if(ret)
    {
        printf("\n STA_SCAN LIST: %s\n", list_str);
    }
    else
    {
        printf("\n sta_scan failed!\n");
    }

	char ip_addr[30] = {0};
	
	ret = get_sta_ip(ip_addr, sizeof(ip_addr));
	if(ret)
	{
		printf("\n STA_IP: %s\n", ip_addr);
	}
	else
	{
		printf("\n get_sta_ip failed!\n");
	}
}

/*****************************************************************************
* Function Name : Wifi_AP_Control
* Description   : AP_Control
* Input			: None
* Output        : None
* Return        : id
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void Wifi_AP_Control()
{
	char ssid[69] = {0};
	int authType = 0;
	int encryptMode = 0;
	char password[65] = {0};

	wifi_get_ap_ssid(ssid, AP_INDEX_0);
	wifi_set_ap_ssid("TBox_AP_Demo", AP_INDEX_0);
	
	wifi_get_ap_auth(&authType, &encryptMode, password, AP_INDEX_0);
	wifi_set_ap_auth(authType, encryptMode, "12345678", AP_INDEX_0);
	memset(password, 0, sizeof(password));
	wifi_get_ap_auth(&authType, &encryptMode, password, AP_INDEX_0);

	sleep(10);
	
	int count = get_client_count(AP_INDEX_0);
	printf("\n CLIENT_COUNT: %d\n", count);

}

/*****************************************************************************
* Function Name : wifi_default_route
* Description   : wifi默认路由检测
* Input			: None
* Output        : None
* Return        : 0: success, -1: error
* Auther        : ygg
* Date          : 2018.03.18
*****************************************************************************/
int wifi_default_route()
{
	FILE *fp;
	char buff[50];
	int ret = -1;
	memset(buff, 0, sizeof(buff));
	
	fp = popen("route | sed -n '/default/p' | awk '{print $8}'","r");
    if(fp == NULL)
    {
        return -1;
    }
	if(fread(buff, sizeof(char), sizeof(buff), fp) > 0)
	{
		if(strstr(buff, "wlan0") != NULL)
		{
			ret = 0;
		}
	}

	printf("wifi_default_route ret:%d\n",ret);

    pclose(fp);
	return ret;
}

/*****************************************************************************
* Function Name : wifi_connected_dataExchange
* Description   : wifi数据交互状态检测
* Input			: None
* Output        : None
* Return        : 0: success, -1: error
* Auther        : ygg
* Date          : 2018.03.18
*****************************************************************************/
int wifi_connected_dataExchange()
{
	int ret;
	if(!access("/sys/class/net/wlan0", F_OK))
	{

		ret = get_client_count(AP_INDEX_STA);
		
		if((ret != 0)/*||(wifi_default_route() == 0)*/)
		{
			if(wifiLedStatus != 2)
			{
				wifi_led_blink(500, 500);
			}
		}
		else
		{
			if(wifiLedStatus != 1)
			{
				wifi_led_on();
			}
		}
	}
	else
	{
		if(wifiLedStatus != 0)
		{
			wifi_led_off(1);
		}
	}
	return 0;
}

/*****************************************************************************
* Function Name : wifi_startState_check
* Description   : wifi开启状态检测
* Input			: None
* Output        : None
* Return        : 0: success, -1: error
* Auther        : ygg
* Date          : 2018.03.18
*****************************************************************************/
#if 0
int wifi_startState_check()
{
	FILE *fp;
	char buff[50];
	int  ret = -1;
	memset(buff, 0, sizeof(buff));
	
	//fp = popen("ifconfig |sed -n '/wlan0/p'| awk '{print $1}'","r");
	fp = popen("ifconfig |sed -n '/wlan0/p'","r");
	if(fp == NULL)
	{
		printf("111111111 wifi_startState_check  error\n");
	    return -1;
	}
	
	if(fread(buff, sizeof(char), sizeof(buff), fp) > 0)
	{
		printf("22222 wifi_startState_check  wifiLedStatus:%d \n", wifiLedStatus);
		if(wifiLedStatus == 0)
		{
			wifi_led_on();
	    }
		ret = 0;
	}
    
    pclose(fp);
    return ret;
}
#endif
int wifi_startState_check()
{
	#define WLAN0_PATH	"/sys/class/net/wlan0"

	if(!access(WLAN0_PATH, F_OK)) //return 0 or -1
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

/*****************************************************************************
* Function Name : wifi_name_pwd_init
* Description   : wifi名字和密码初始化
* Input			: None
* Output        : None
* Return        : 0: success, -1: error
* Auther        : ygg
* Date          : 2018.05.24
*****************************************************************************/
int wifi_name_pwd_init()
{
	#define WIFI_PREFIX   "T-BOX-"
	#define DEFAULT_SSID     "TBOX_AP_STA"
	#define DEFAULT_PWD      "TCMSGDMN123"

	int i, j, len,length;
	char ssid[20] = {0};
	char iccid[20] = {0};
	char password[20] = {0};
	char SaveSsid[20] = {0};
	char SavePassWord[20] = {0};
	
	char RandomSeed[] = { '0','1','2','3','4','5','6','7','8','9',
						  'a','b','c','d','e','f','g','h','i','j',
						  'k','l','m','n','o','p','q','r','s','t',
						  'u','v','w','x','y','z','A','B','C','D',
						  'E','F','G','H','I','J','K','L','M','N',
						  'O','P','Q','R','S','T','U','V','W','X',
						  'Y','Z' };

	//set wifi prefix
	strcat(ssid, WIFI_PREFIX);
	len = strlen(WIFI_PREFIX);

    dataPool->getPara(SIM_ICCID_INFO, (void *)iccid, sizeof(iccid));
    //printf("\n 1111111111111111111 iccid:%s\n", iccid);
	length = strlen(iccid);

    //Take out the ICCID's back 4 data 
    for(i = (length-4), j=0; i<length; i++,j++)
    {
        ssid[len+j] = iccid[i];
    }
    printf("\n wifi  ssid:%s\n", ssid);

	srand((unsigned)time(NULL));
#if 1
	//wifi password max length is 16
	for(i = 0; i< 8; i++)
	{
		password[i] = RandomSeed[(rand()+getpid())%sizeof(RandomSeed)];
	}
#endif

    printf("\n wifi  ssid:%s\n", ssid);
	printf("\n wifi password:%s\n", password);
	
#if 0
	memset(SaveSsid, 0, sizeof(SaveSsid));
	dataPool->getPara(wifi_APMode_Name_Id, (void *)SaveSsid, sizeof(SaveSsid));
	printf("\n wifi  SaveSsid:%s\n", SaveSsid);
	if(strcmp(SaveSsid, DEFAULT_SSID) == 0){
		wifi_set_ap_ssid(ssid, AP_INDEX_STA);
	}

	memset(SavePassWord, 0, sizeof(SavePassWord));
	printf("\n wifi  SavePassWord:%s\n", SavePassWord);
	dataPool->getPara(wifiAPModePassswordId, (void *)SavePassWord, sizeof(SavePassWord));
	if(strcmp(SavePassWord, DEFAULT_PWD)== 0){ 
		wifi_set_ap_auth(5, 4, password, AP_INDEX_STA);
	}
#endif

	if(TBoxPara.detectionTime.IsWifiModification == 0)
	{
		wifi_set_ap_ssid(ssid, AP_INDEX_STA);
		dataPool->setPara(wifi_APMode_Name_Id, (void *)ssid, sizeof(ssid));
		wifi_set_ap_auth(5, 4, password, AP_INDEX_STA);
		dataPool->setPara(wifiAPModePassswordId, (void *)password, sizeof(password));
	}

	return 0;
}

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
int check_wifi_ssid_pwd_default_value()
{	
//	int get_authType;
//	int get_encryptMode;
//	char ssid[20] = {0};
//	char password[20] = {0};

//	wifi_get_ap_ssid(ssid, AP_INDEX_STA);
//	wifi_get_ap_auth(&get_authType, &get_encryptMode, password, AP_INDEX_STA);

	//if((strcmp(ssid, DEFAULT_SSID) != 0) && (strcmp(password, DEFAULT_PWD) != 0))
//	{
		wifi_name_pwd_init();
//	}

	return 0;
}

/*****************************************************************************
* Function Name : wifi_init
* Description   : 初始化wifi
* Input			: None
* Output        : None
* Return        : id
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
void wifi_init()
{
	//0:AP; 1:AP&AP; 2:AP&STA
	#define WIFI_MODE   2

	//wifi_OpenOrClose(1);
	Wifi_Set_Mode(WIFI_MODE);

	if(WIFI_MODE == 0)
	{
		Wifi_AP_Control();
	}
	else if(WIFI_MODE == 1)
	{
		Wifi_AP_AP_Control();
	}
	else
	{
		Wifi_AP_STA_Control();
	}
}



