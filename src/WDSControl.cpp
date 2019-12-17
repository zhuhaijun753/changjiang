/*
  SIMCOM SIM7600 QMI PROFILE DEMO
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#include "WDSControl.h"
#define LOG printf
#define WDS_QMI_TIMEOUT_VALUE  10000


static qmi_idl_service_object_type wds_service_object = NULL;
static qmi_client_type             wds_svc_client;
static qmi_client_os_params        wds_os_params;
static qmi_client_type             wds_notifier;

int wds_qmi_init(void)
{
    unsigned int num_services = 0, num_entries = 0;
    qmi_client_error_type qmi_error = QMI_NO_ERR;
    int num_retries = 0;

    if(wds_svc_client != NULL)
    {
        return 0;
    }
    
    wds_service_object = wds_get_service_object_v01();
    if(wds_service_object == NULL)
    {
        return -1;
    }
    
    do {
        if ( num_retries != 0) 
        {
            sleep(1);
            LOG("qmi_client_init_instance status retry : %d", num_retries);
        }

        LOG("qmi_client_init_instance....\n ");
        qmi_error = qmi_client_init_instance(wds_service_object,
                                           QMI_CLIENT_INSTANCE_ANY,
                                           NULL,
                                           NULL,
                                           &wds_os_params,
                                           (int) 4,
                                           &wds_svc_client);
        num_retries++;
    } while ( (qmi_error != QMI_NO_ERR) && (num_retries < 2) );


    if ( qmi_error == QMI_NO_ERR )
    {
        qmi_error =  qmi_client_notifier_init(wds_service_object,
                                              &wds_os_params,
                                              &wds_notifier
                                              );
        LOG("qmi_client_notifier_init status: %d\n", (int) qmi_error);
    }
    else
    {
        wds_service_object = NULL;
        return -1;
    }
    return 0;
}


void wds_qmi_release(void)
{
    qmi_client_release(wds_svc_client);
    wds_svc_client = NULL;
}

static boolean wds_client_GetProfileList(qmi_client_type qmi_wds_handle, 
                                        wds_get_profile_list_req_msg_v01 *req_msg,
   										wds_get_profile_list_resp_msg_v01 *profilelist, 
   										qmi_error_type_v01 *qmi_err_num)
{
	qmi_client_error_type qmi_error, qmi_err_code = QMI_NO_ERR;
  	int i;
  	
	qmi_error = qmi_client_send_msg_sync(qmi_wds_handle,
											QMI_WDS_GET_PROFILE_LIST_REQ_V01,
											(void*)req_msg,
											sizeof(wds_get_profile_list_req_msg_v01),
											(void*)profilelist,
											sizeof(wds_get_profile_list_resp_msg_v01),
											WDS_QMI_TIMEOUT_VALUE); 
			
	if ( ( qmi_error == QMI_TIMEOUT_ERR ) ||
	   ( qmi_error != QMI_NO_ERR ) ||
	   ( profilelist->resp.result != QMI_NO_ERR ) )
	{
		LOG("qiu wds Can not get profile list %d : %d",qmi_error, profilelist->resp.error);        
		*qmi_err_num = profilelist->resp.error;
		return FALSE;
	}

	LOG("qiu wds get profile_list_len = %d\n",profilelist->profile_list_len);
	for(i = 0; i < profilelist->profile_list_len; i++)
	{
    	LOG("list[%d]: profile_index=%d, profile_name=%s\n",i, 
                	    profilelist->profile_list[i].profile_index,
                	    profilelist->profile_list[i].profile_name);
	}

	return TRUE;
}


static boolean wds_client_GetProfileSettings(qmi_client_type qmi_wds_handle, 
                              wds_get_profile_settings_req_msg_v01 *req_msg, 
   							  wds_get_profile_settings_resp_msg_v01 *resp_msg,
   							  qmi_error_type_v01 *qmi_err_num)
{

	qmi_client_error_type qmi_error, qmi_err_code = QMI_NO_ERR;

	qmi_error = qmi_client_send_msg_sync(qmi_wds_handle,
	                                   QMI_WDS_GET_PROFILE_SETTINGS_REQ_V01,
	                                   (void*)req_msg,
	                                   sizeof(wds_get_profile_settings_req_msg_v01),
	                                   (void*)resp_msg,
	                                   sizeof(wds_get_profile_settings_resp_msg_v01),
	                                   WDS_QMI_TIMEOUT_VALUE); 

	if ( ( qmi_error == QMI_TIMEOUT_ERR ) ||
	   ( qmi_error != QMI_NO_ERR ) ||
	   ( resp_msg->resp.result != QMI_NO_ERR ) )
	{
		LOG("qiu wds Can not get profile settings %d : %d\n",
	    qmi_error, resp_msg->resp.error, 0);
	    *qmi_err_num = resp_msg->resp.error;
	    return FALSE;
	}

	LOG("wds pro_name_valid = %d, pro_name = %s\n",resp_msg->profile_name_valid, resp_msg->profile_name, 0);
	LOG("wds pdp_type_valid = %d, pdp_type = %d\n",resp_msg->pdp_type_valid, resp_msg->pdp_type, 0);
	LOG("wds apn_name_valid = %d, apn_name = %s\n",resp_msg->apn_name_valid, resp_msg->apn_name, 0);
	LOG("wds authentication_preference_valid = %d, authentication_preference = %d\n",
                                  resp_msg->authentication_preference_valid, resp_msg->authentication_preference, 0);

	if(req_msg->profile.profile_type == WDS_PROFILE_TYPE_3GPP_V01)
	{
		LOG("wds username_valid = %d, username = %s\n",resp_msg->username_valid, resp_msg->username, 0);
		LOG("wds password_valid = %d, password = %s\n",resp_msg->password_valid, resp_msg->password, 0);	
	}
	else
	{
		LOG("wds user_id_valid = %d, user_id = %s\n",resp_msg->user_id_valid, resp_msg->user_id, 0);
		LOG("wds password_valid = %d, password = %s\n",resp_msg->password_valid, resp_msg->password, 0);			
	}

  	return TRUE;
}


int wds_GetAPNInfo(int profile_index, int* pdp_type, char* apn_str, char *username, char *password)
{
    boolean ret = TRUE;
    qmi_error_type_v01 qmi_err_num;
    int i = 0;

    wds_get_profile_list_req_msg_v01  req_profilelist;
    wds_get_profile_list_resp_msg_v01 resp_profilelist;

    wds_profile_identifier_type_v01 profile_identifier;
    wds_get_profile_settings_req_msg_v01 req_msg;
    wds_get_profile_settings_resp_msg_v01 resp_msg; 

    memset(&req_profilelist,0,sizeof(wds_get_profile_list_req_msg_v01));
    memset(&resp_profilelist,0,sizeof(wds_get_profile_list_resp_msg_v01));

    req_profilelist.profile_type = WDS_PROFILE_TYPE_3GPP_V01;
    req_profilelist.profile_type_valid = 1;

    
    if(!wds_client_GetProfileList(wds_svc_client, &req_profilelist, &resp_profilelist, &qmi_err_num))
    {
        LOG("wds_client_GetProfileList error[%0x]\n", qmi_err_num);
        return FALSE;
    }

    for(i=0; i < resp_profilelist.profile_list_len; i++)
    { 
        if(resp_profilelist.profile_list[i].profile_index != profile_index)
        {
            continue;
        }
        memset(&req_msg, 0, sizeof(wds_get_profile_settings_req_msg_v01));
        memset(&resp_msg, 0, sizeof(wds_get_profile_settings_resp_msg_v01));
        req_msg.profile.profile_type = resp_profilelist.profile_list[i].profile_type;
        req_msg.profile.profile_index = resp_profilelist.profile_list[i].profile_index;
        ret = wds_client_GetProfileSettings(wds_svc_client, &req_msg, &resp_msg, &qmi_err_num);
        if(!ret)
        {
            LOG("wds_client_GetProfileSettings error[%0x]\n", qmi_err_num, 0, 0);
            ret = FALSE;
            break;
        }
        
        if(resp_msg.pdp_type_valid)
        {
            *pdp_type = (int)resp_msg.pdp_type;
        }
        
        if(resp_msg.apn_name_valid)
        {
            strcpy(apn_str, resp_msg.apn_name);
        }

        if(resp_msg.username_valid)
        {
            strcpy(username, resp_msg.username);           
        }
        if(resp_msg.password_valid)
        {
            strcpy(password, resp_msg.password);         
        }       
        break;
    }
    
    return ret;
}

static boolean wds_client_CreateProfile( qmi_client_type qmi_wds_handle, 
                                        wds_create_profile_req_msg_v01   *req_msg, 
										wds_create_profile_resp_msg_v01	 *resp_msg,
										qmi_error_type_v01 *qmi_err_num)
{
	qmi_client_error_type qmi_error, qmi_err_code = QMI_NO_ERR;
  
	qmi_error = qmi_client_send_msg_sync(qmi_wds_handle,
											QMI_WDS_CREATE_PROFILE_REQ_V01,
											(void*)req_msg,
											sizeof(wds_create_profile_req_msg_v01),
											(void*)resp_msg,
											sizeof(wds_create_profile_resp_msg_v01),
											WDS_QMI_TIMEOUT_VALUE); 

	if ( ( qmi_error == QMI_TIMEOUT_ERR ) ||
	   ( qmi_error != QMI_NO_ERR ) ||
	   ( resp_msg->resp.result != QMI_NO_ERR ) )
	{
		LOG("qiu wds Can not create profile %d : %d\n",qmi_error, resp_msg->resp.error, 0);	
		*qmi_err_num = resp_msg->resp.error;
		return FALSE;
	}
	
	return TRUE;
	
}

static boolean wds_client_ModifyProfileSettings(qmi_client_type qmi_wds_handle, 
                                                wds_modify_profile_settings_req_msg_v01 *req_msg, 
											    qmi_error_type_v01 *qmi_err_num)
{
	qmi_client_error_type qmi_error, qmi_err_code = QMI_NO_ERR;

	wds_modify_profile_settings_resp_msg_v01 resp_msg;

	memset(&resp_msg, 0, sizeof(wds_modify_profile_settings_resp_msg_v01));
  
	LOG("qiu wds modify profile index =%d\n",req_msg->profile.profile_index);
    LOG("qiu wds modify profile name =%s\n",req_msg->profile_name);
    LOG("qiu wds modify profile apnname =%s\n",req_msg->apn_name);
    LOG("qiu wds modify profile user name =%s\n",req_msg->username);
    LOG("qiu wds modify profile password =%s\n",req_msg->password);


	qmi_error = qmi_client_send_msg_sync(qmi_wds_handle,
	                                   QMI_WDS_MODIFY_PROFILE_SETTINGS_REQ_V01,
	                                   (void*)req_msg,
	                                   sizeof(wds_modify_profile_settings_req_msg_v01),
	                                   (void*)&resp_msg,
	                                   sizeof(wds_modify_profile_settings_resp_msg_v01),
	                                   WDS_QMI_TIMEOUT_VALUE);

	if ( ( qmi_error == QMI_TIMEOUT_ERR ) ||
	   ( qmi_error != QMI_NO_ERR ) ||
	   ( resp_msg.resp.result != QMI_NO_ERR ) )
	{
		LOG("qiu wds Can not modify profile settings %d : %d\n",qmi_error, resp_msg.resp.error);					
		*qmi_err_num = resp_msg.resp.error;
		return FALSE;
	}

 	return TRUE;
 	
}

/*---------------------------------------------------------------
wds_SetAPNInfo  
   profile_index: 1~6
   apn_str: apn string
   ip_op:   "IP" / "IPV4V6"
   pdp_type: 
---------------------------------------------------------------*/
int wds_SetAPNInfo(int profile_index, int pdp_type, char* apn_str, char *username, char *password)
{
    boolean ret = TRUE;
    qmi_error_type_v01 qmi_err_num;
    int i = 0, j = 0;
    char *get_apn_str;
    char *get_ip_op;
    int get_pdp_type;
    int list_cnt;
    boolean is_profile_exist = FALSE;

    wds_create_profile_req_msg_v01 create_req_msg;
    wds_create_profile_resp_msg_v01 create_resp_msg;
    //wds_profile_identifier_type_v01 create_profile;
    wds_modify_profile_settings_req_msg_v01 modify_profile_setting_req_msg;
    wds_get_profile_list_req_msg_v01  req_profilelist;
    wds_get_profile_list_resp_msg_v01 resp_profilelist;

    memset(&create_req_msg, 0,sizeof(wds_create_profile_req_msg_v01));
    memset(&create_resp_msg, 0,sizeof(wds_create_profile_resp_msg_v01));
    //memset(&create_profile,0,sizeof(wds_profile_identifier_type_v01)); 
    
    memset(&req_profilelist,0,sizeof(wds_get_profile_list_req_msg_v01));
    memset(&resp_profilelist,0,sizeof(wds_get_profile_list_resp_msg_v01));   
    memset(&modify_profile_setting_req_msg, 0, sizeof(modify_profile_setting_req_msg));

    if((apn_str == NULL) || strlen(apn_str) < 1)
    {
        //return FALSE;
    }

    //-----------get profile list ---------------------------
    req_profilelist.profile_type = WDS_PROFILE_TYPE_3GPP_V01;
    req_profilelist.profile_type_valid = 1;    
    if(!wds_client_GetProfileList(wds_svc_client, &req_profilelist, &resp_profilelist, &qmi_err_num))
    {
        return FALSE;
    }

    for(i = 0; i < resp_profilelist.profile_list_len; i++)
    {
        if(resp_profilelist.profile_list[i].profile_index == profile_index)
        {
            is_profile_exist = TRUE;
            break;
        }
    }
    
    for(i = 1; i < profile_index; i++)
    {
        boolean has_set = FALSE;
        for(j = 0; j < resp_profilelist.profile_list_len; j++)
        {
            if(i == resp_profilelist.profile_list[j].profile_index)
            {
                has_set = TRUE;
                break;
            }
        }
    
        LOG("=====[index%d, has_set=%d]\n", i, has_set, 0);
        if(!has_set)
        {
            create_req_msg.pdp_type_valid = 1;
            create_req_msg.pdp_type = 0;
            ret = wds_client_CreateProfile(wds_svc_client, &create_req_msg, &create_resp_msg, &qmi_err_num);
            if(ret != TRUE)
            {
                LOG("create empty apn fail i = %d\n",i);
                return FALSE;
            }
        }       
    }
    
    #if 0
    wds_GetAPNInfo(int profile_index,get_apn_str,get_ip_op, &get_pdp_type);

    if((strcmp(apn_str, get_apn_str) == 0) &&
        (strcmp(apn_str, get_apn_str) &&
        (pdp_type == get_pdp_type))
    {
        // current value same as the set value, ignore setting
        return TRUE;
    }
    #endif
    if(is_profile_exist == FALSE)
    {
        create_req_msg.profile_type = WDS_PROFILE_TYPE_3GPP_V01; //WDS_PROFILE_TYPE_EPC_V01;//WDS_PROFILE_TYPE_3GPP_V01;

        if(apn_str != NULL && strlen(apn_str) > 0)
        {
            create_req_msg.profile_name_valid = 1;
            strcpy(create_req_msg.profile_name, apn_str);
            create_req_msg.apn_name_valid = 1;
            strcpy(create_req_msg.apn_name, apn_str);
        }

        create_req_msg.pdp_type_valid = 1;
        create_req_msg.pdp_type = 0;        
        if(username != 0 && strlen(username) != 0 && password != NULL && strlen(password) != 0)
        {
            create_req_msg.username_valid = 1;
            strcpy(create_req_msg.username, username);
            create_req_msg.password_valid = 1;
            strcpy(create_req_msg.password, password);
        }
        
        ret = wds_client_CreateProfile(wds_svc_client, &create_req_msg, &create_resp_msg,&qmi_err_num);
        if(ret != TRUE)
        {
            LOG("create apn fail i = %d\n",i);
            return FALSE;
        }        
    }
    else
    {
        // -----modify  apn -------------------
        LOG("modify apn: %s, pdp_type: %d\n", apn_str, pdp_type);
        
        memset(&modify_profile_setting_req_msg, 0, sizeof(modify_profile_setting_req_msg));
        modify_profile_setting_req_msg.profile.profile_type = WDS_PROFILE_TYPE_3GPP_V01;
        modify_profile_setting_req_msg.profile.profile_index = profile_index;

        modify_profile_setting_req_msg.profile_name_valid = 1;
        modify_profile_setting_req_msg.pdp_type_valid = 1;
        modify_profile_setting_req_msg.pdp_type = pdp_type;
        if(apn_str != NULL && strlen(apn_str) > 0)
        {
            modify_profile_setting_req_msg.apn_name_valid = 1;   
            strncpy(modify_profile_setting_req_msg.apn_name, apn_str, strlen(apn_str));
        }
        ret = wds_client_ModifyProfileSettings(wds_svc_client, &modify_profile_setting_req_msg,&qmi_err_num);
        if(ret != TRUE)
        {
            return FALSE;
        }
    }

    return TRUE;
}


#if 0
int main(int argc , char ** argv)
{
	if(0)
	{
		int i;
		char apn[20];
		char username[64];
		char password[64];
		int pdp_type;
		ret = wds_qmi_init();
		if(ret != 0)
		{
			printf("wds init fail");
		}
		
		for(i = 1; i < 8; i++)
		{
			memset(apn, 0, sizeof(apn));
			memset(username, 0, sizeof(username));
			memset(password, 0, sizeof(password));
			ret = wds_GetAPNInfo(i, &pdp_type, apn, username,password); //4 or 6(外网)
			if(ret == FALSE)
			{
				printf("wds_GetAPNInfo Fail\n");
			}
			else
			{
				printf(">>>>>> apn[%d]=%s, pdp_type = %d, username=%s, password=%s\n", 
						i, apn, pdp_type,username,password);
			}			 
		}
	}

	if(0)
	{
		int apn_index = 4;
		char apn[32] = {0};
		int pdp_type = 0;

		char scan_string[10] = {0};
		while(1)
		{
			printf("\nplease intput apn_index (1~6):");
			memset(scan_string, 0, sizeof(scan_string));	   
			if (fgets(scan_string, sizeof(scan_string), stdin) != NULL) 	  
			{		   
				apn_index = atoi(scan_string);
				if(apn_index >= 1 && apn_index < 10)
				{
					break;
				} 
			}
		}
		while(1)
		{
			printf("\nplease intput apn string:");
			memset(scan_string, 0, sizeof(scan_string));	   
			if (fgets(scan_string, sizeof(scan_string), stdin) != NULL) 	  
			{
				strncpy(apn, scan_string, strlen(scan_string) - 1);
				break;
			}
		}
		
		ret = wds_qmi_init();
		if(ret != 0)
		{
			printf("wds init fail");
		}

		ret = wds_SetAPNInfo(apn_index, pdp_type, apn, NULL, NULL);
		if(ret == FALSE)
		{
			printf("wds_SetAPNInfo Fail\n");
		}
		else
		{
			printf("wds_SetAPNInfo Success\n");
		}
	  
	}
}

#endif
