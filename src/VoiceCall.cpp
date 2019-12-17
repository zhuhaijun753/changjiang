/*
  SIMCOM SIM7600 QMI VOICE CALL DEMO
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "VoiceCall.h"
#include "qmi_client.h"


#define LOG printf

static qmi_idl_service_object_type voice_svc_obj = NULL;
static qmi_client_type             voice_svc_client;
static qmi_client_os_params        voice_os_params;
static qmi_client_type             voice_notifier;

call_state_enum_v02 Now_Call_State;
uint8_t Now_call_id;


int voice_mtcall_process(voice_call_option option, unsigned char call_id);

/*
  Callback func.
  When qmi got unsolited message to deliver to ap from modem,
  this function will be invoked.
  NOTE: this func is called in a separate thread from the main thread,
  this thread is created by qmi libraries
 
*/
static void voice_ind_cb
(
    qmi_client_type                user_handle,
    unsigned int                   msg_id,
    void                          *ind_buf,
    unsigned int                   ind_buf_len,
    void                          *ind_cb_data
)
{
    voice_all_call_status_ind_msg_v02 call_info_msg;
    voice_info_rec_ind_msg_v02  info_rec;
    qmi_client_error_type qmi_error = QMI_NO_ERR;
    int i,j;
    int bGetNumVal = 0;

    if(QMI_VOICE_ALL_CALL_STATUS_IND_V02 == msg_id)
    {
        call_info_type call_list[QMI_VOICE_CALL_INFO_MAX_V02];
        memset(call_list, 0, sizeof(call_list));
        LOG("\n>>>>>>QMI_VOICE_ALL_CALL_STATUS_IND_V02 -------------\n");
        qmi_error = qmi_client_message_decode(voice_svc_client,
                                          QMI_IDL_INDICATION,
                                          QMI_VOICE_ALL_CALL_STATUS_IND_V02,
                                          ind_buf,
                                          ind_buf_len,
                                          &call_info_msg,
                                          sizeof(call_info_msg));

        if(qmi_error != QMI_NO_ERR)
        {
            //printf("QMI_VOICE_ALL_CALL_STATUS_IND_V02 decode fail!\n");
            return;
        }

      //  printf("call_info_len = %d\n",call_info_msg.call_info_len);
        for(i = 0; i < call_info_msg.call_info_len; i++)
        {   
            
           printf("index[%d] id=%d, state=%d, dir=%d",i,call_info_msg.call_info[i].call_id,
                                           call_info_msg.call_info[i].call_state,
                                           call_info_msg.call_info[i].direction);
            

            call_list[i].call_id = call_info_msg.call_info[i].call_id;
            call_list[i].call_state = call_info_msg.call_info[i].call_state;
            call_list[i].direction = call_info_msg.call_info[i].direction;
	    Now_call_id = call_list[0].call_id;
	    Now_Call_State = call_list[0].call_state;
            bGetNumVal = 0;                            
            if(call_info_msg.remote_party_number_valid)
            {
                for(j = 0; j < call_info_msg.remote_party_number_len; j++)
                {
                    if((call_info_msg.remote_party_number[j].call_id == call_info_msg.call_info[i].call_id) &&
                       call_info_msg.remote_party_number[i].number_len > 0)
                    {
                        //LOG(" number = %s\n", call_info_msg.remote_party_number[j].number);
                        bGetNumVal = 1;
                        strcpy(call_list[i].phone_number,call_info_msg.remote_party_number[j].number);
                        break;
                    }
                }
            }
		//来电挂断
		if(call_list[i].call_state == CALL_STATE_INCOMING_V02)
			hangUp_answer(VOICE_CALL_HANDUP, call_list[i].call_id);
        }
		process_simcom_ind_message(SIMCOM_EVENT_VOICE_CALL_IND,call_list);
}
  else if(QMI_VOICE_INFO_REC_IND_V02 == msg_id)
  {
        #if 0
        LOG(">>>>>>QMI_VOICE_INFO_REC_IND_V02\n");
        qmi_client_message_decode(voice_svc_client,
                                  QMI_IDL_INDICATION,
                                  QMI_VOICE_INFO_REC_IND_V02,
                                  ind_buf,
                                  ind_buf_len,
                                  &info_rec,
                                  sizeof(info_rec));

        if(info_rec.calling_party_info_valid)
        {
            printf("num is %s\n", info_rec.calling_party_info.num);
        }
        #endif

        #if 0
    	if ((info_rec.call_id == g_MT.callid) && info_rec.calling_party_info_valid)
    		printf("num is %s\n", info_rec.calling_party_info.num);
    	if ((info_rec.call_id == g_MO.callid))
    		printf("Call established\n");
        #endif
		#if 0
        LOG("call id =%d\n", info_rec.call_id);
        LOG("calling_party_info_valid =%d\n", info_rec.calling_party_info_valid);
        if (info_rec.calling_party_info_valid)
        
    	LOG("calling_party_info =%s\n", info_rec.calling_party_info.num);
	    callId = info_rec.call_id;
	    #endif
  }
  else
  {
    LOG("unknow msg_id=%d\n", msg_id);
  }
  
  return;
}

int voice_mtcall_process(voice_call_option option, unsigned char call_id)
{
    voice_manage_calls_req_msg_v02 hold_req;
    voice_manage_calls_resp_msg_v02 hold_resp;
	qmi_client_error_type qmi_error; 
    int ret = 0;
	switch(option)
	{
	    case VOICE_CALL_HANDUP:
        	{
                voice_end_call_req_msg_v02 endcall_req;
                voice_end_call_resp_msg_v02 endcall_resp;
                memset(&endcall_req, 0, sizeof(voice_end_call_req_msg_v02));
                memset(&endcall_resp, 0, sizeof(voice_end_call_resp_msg_v02));
        		endcall_req.call_id = call_id;
        		endcall_req.end_cause_valid = 1;
        		endcall_req.end_cause = VOICE_REJECT_CAUSE_USER_REJECT_V02;
        	  	qmi_error = qmi_client_send_msg_sync(voice_svc_client,
                    	                            QMI_VOICE_END_CALL_REQ_V02, 
                    	                            &endcall_req, 
                    	                            sizeof(endcall_req), 
                    	                            &endcall_resp, 
                    	                            sizeof(endcall_resp),
                    	                            VOICE_QMI_TIMEOUT_VALUE);

                if (QMI_NO_ERR != qmi_error)
                {
                    LOG("VOICE_CALL_HANDUP FAILED== qmi end call qmi_error=%d\n", qmi_error);
                    ret = -1;
                }
                if (endcall_resp.resp.result != QMI_RESULT_SUCCESS_V01 )
                {
                    LOG("qmi end call err=%d\n", endcall_resp.resp.error);
                    ret = -1;
                }
                LOG("VOICE_CALL_HANDUP SUCCESS== qmi end call id=%d\n", endcall_resp.call_id);
        	}
        	break;
	    case VOICE_CALL_ANSWER:
        	{
            	voice_answer_call_req_msg_v02 answercall_req;
            	voice_answer_call_resp_msg_v02 answercall_resp;	
                memset(&answercall_req, 0, sizeof(voice_answer_call_req_msg_v02));
                memset(&answercall_resp, 0, sizeof(voice_answer_call_resp_msg_v02));

        		answercall_req.call_id = call_id;
        	  	qmi_error = qmi_client_send_msg_sync(voice_svc_client,
                    	                            QMI_VOICE_ANSWER_CALL_REQ_V02, 
                    	                            &answercall_req, 
                    	                            sizeof(voice_answer_call_req_msg_v02), 
                    	                            &answercall_resp, 
                    	                            sizeof(voice_answer_call_resp_msg_v02),
                    	                            VOICE_QMI_TIMEOUT_VALUE);

                if (QMI_NO_ERR != qmi_error)
                {
                    LOG("qmi answer call qmi_error=%d\n", qmi_error);
                    ret = -1;
                }
                if (answercall_resp.resp.result != QMI_RESULT_SUCCESS_V01 )
                {
                    LOG("qmi answer call err=%d\n", answercall_resp.resp.error);
                    ret = -1;
                }
                LOG("qmi answer call id=%d\n", answercall_resp.call_id);
            }
            break;
            
        case VOICE_CALL_HOLD:
        case VOICE_END_BG:
        case VOICE_END_FG:
        case VOICE_END_ALL:
            {
                voice_manage_calls_req_msg_v02 hold_req;
                voice_manage_calls_resp_msg_v02 hold_resp;
                memset(&hold_req, 0, sizeof(voice_manage_calls_req_msg_v02));
                memset(&hold_resp, 0, sizeof(voice_manage_calls_resp_msg_v02));

                if(option == VOICE_CALL_HOLD)
                {
        		    hold_req.sups_type = SUPS_TYPE_HOLD_ACTIVE_ACCEPT_WAITING_OR_HELD_V02;
        		}
        		else if(option == VOICE_END_BG)    //release held or waitting
                {
                    hold_req.sups_type = SUPS_TYPE_RELEASE_HELD_OR_WAITING_V02;
                }
        		else if(option == VOICE_END_FG)    //release active and accept held or waitting
        		{
        		    hold_req.sups_type = SUPS_TYPE_RELEASE_ACTIVE_ACCEPT_HELD_OR_WAITING_V02;
        		}
        		else if(option == VOICE_END_ALL)
        		{
        		    hold_req.sups_type = SUPS_TYPE_END_ALL_CALLS_V02;
        		}
        		
        	  	qmi_error = qmi_client_send_msg_sync(voice_svc_client,
                    	                            QMI_VOICE_MANAGE_CALLS_REQ_V02, 
                    	                            &hold_req, 
                    	                            sizeof(voice_manage_calls_req_msg_v02), 
                    	                            &hold_resp, 
                    	                            sizeof(voice_manage_calls_resp_msg_v02),
                    	                            VOICE_QMI_TIMEOUT_VALUE);
                if (QMI_NO_ERR != qmi_error)
                {
                    LOG("voice manage qmi_error=%d\n", qmi_error);
                    ret = -1;
                }
                if (hold_resp.resp.result != QMI_RESULT_SUCCESS_V01 )
                {
                    LOG("voice manage err=%d\n", hold_resp.resp.error);
                    ret = -1;
                }	                
            }
            break;
        default:
            ret = -1;
            break;   
	}
    return ret;
}

int voice_qmi_init(void)
{
    qmi_client_error_type client_err = QMI_NO_ERR;
    int num_retries = 0;

    if(voice_svc_obj != NULL)
    {
        return 0;
    }

    voice_svc_obj = voice_get_service_object_v02();

    do 
    {
        if ( num_retries != 0) 
        {
            sleep(1);
            LOG("qmi_client_init_instance status retry : %d", num_retries);
        }

        LOG("qmi_client_init_instance....\n ");
        client_err = qmi_client_init_instance(voice_svc_obj,
                                               QMI_CLIENT_INSTANCE_ANY,
                                               voice_ind_cb,
                                               NULL,
                                               &voice_os_params,
                                               (int) 4,
                                               &voice_svc_client);
        num_retries++;
    } while ( (client_err != QMI_NO_ERR) && (num_retries < 2) );


    if ( client_err == QMI_NO_ERR )
    {
        client_err =  qmi_client_notifier_init(voice_svc_obj,
                                              &voice_os_params,
                                              &voice_notifier
                                              );
        LOG("qmi_client_notifier_init status: %d\n", (int) client_err);
        return 0;
    }

    return -1;
}

int voice_qmi_ind_register(void)
{
	voice_indication_register_req_msg_v02 ind_regiter_req;
	voice_indication_register_resp_msg_v02 ind_regiter_resp;
	qmi_client_error_type rc;
    memset(&ind_regiter_req, 0, sizeof(voice_indication_register_req_msg_v02)); 
    memset(&ind_regiter_resp, 0, sizeof(voice_indication_register_resp_msg_v02));
   
    ind_regiter_req.call_events_valid =1;
    ind_regiter_req.call_events =1;
   
    rc = qmi_client_send_msg_sync(voice_svc_client,
                            QMI_VOICE_INDICATION_REGISTER_REQ_V02, 
                            &ind_regiter_req, 
                            sizeof(ind_regiter_req), 
                            &ind_regiter_resp, 
                            sizeof(ind_regiter_resp),
                            10000);
    if (QMI_NO_ERR != rc)
    {
        LOG("qmi ind register err=%d\n", rc);
        return -1;
    }
    else if (QMI_NO_ERR != ind_regiter_resp.resp.result)
    {
        LOG("qmi ind register failed response err=%d\n",ind_regiter_resp.resp.error);
        return -1;             
    }
    else
    {
        LOG("voice_qmi_ind_register success!");
    }

  return 0;
}

uint8_t voice_dial(char *callingnumber)
{
    voice_dial_call_req_msg_v02     req_msg;
    voice_dial_call_resp_msg_v02    resp_msg;
    qmi_client_error_type           qmi_error = QMI_NO_ERR;
    uint8_t callid = 0;

    memset(&req_msg,  0, sizeof(voice_dial_call_req_msg_v02)); 
    memset(&resp_msg, 0, sizeof(voice_dial_call_resp_msg_v02));

    strcpy(req_msg.calling_number, callingnumber);


	struct tm *p_tm = NULL;
	time_t tmp_time;
	tmp_time = time(NULL);
	p_tm = gmtime(&tmp_time);
	//printf("@@@@@@@@@ simcom voice_dial  @@@@@@@@@@@@@\n");
	//printf(" %0d-%d-%d-%d-%d-%d\n",p_tm->tm_year+1900, p_tm->tm_mon, p_tm->tm_mday,p_tm->tm_hour,p_tm->tm_min,p_tm->tm_sec);

    qmi_error = qmi_client_send_msg_sync(voice_svc_client,
                                        QMI_VOICE_DIAL_CALL_REQ_V02, 
                                        &req_msg, 
                                        sizeof(voice_dial_call_req_msg_v02), 
                                        &resp_msg, 
                                        sizeof(voice_dial_call_resp_msg_v02),
                                        VOICE_QMI_TIMEOUT_VALUE);
	
    if (QMI_NO_ERR != qmi_error)
    {
        //LOG("qmi voice call err=%d\n", qmi_error);
        return -1;
    }
    else if (QMI_NO_ERR != resp_msg.resp.result)
    {
        //LOG("qmi voice call: failed response err=%d\n",resp_msg.resp.error);
        return -1;
    }
    else
    {
        if(resp_msg.call_id_valid)
        {
            //LOG("===call id =%d\n", resp_msg.call_id);
            callid = resp_msg.call_id;
        }
    }

    return callid;
}

int voice_get_call_info(uint8_t callid, call_info_type *pcall_info)
{
    voice_get_call_info_req_msg_v02  req_msg;
    voice_get_call_info_resp_msg_v02 resp_msg;
    qmi_client_error_type            qmi_error; 
    int                              i;
    LOG(">>>>>>voice_get_call_info");
    memset(&req_msg,  0, sizeof(voice_get_call_info_req_msg_v02)); 
    memset(&resp_msg, 0, sizeof(voice_get_call_info_resp_msg_v02));

    req_msg.call_id = callid;
    qmi_error = qmi_client_send_msg_sync(voice_svc_client,
                                        QMI_VOICE_GET_CALL_INFO_REQ_V02, 
                                        &req_msg, 
                                        sizeof(voice_get_call_info_req_msg_v02), 
                                        &resp_msg, 
                                        sizeof(voice_get_call_info_resp_msg_v02),
                                        VOICE_QMI_TIMEOUT_VALUE);

    if (QMI_NO_ERR != qmi_error)
    {
        LOG("qmi get call info err=%d\n", qmi_error);
        return -1;
    }
    else if (QMI_NO_ERR != resp_msg.resp.result)
    {
        LOG("qmi get call info: failed response err=%d\n",resp_msg.resp.error); 
        return -1;
    }
    else
    {
        if(resp_msg.call_info_valid)
        {
            pcall_info->call_id = resp_msg.call_info.call_id;
            pcall_info->call_state = resp_msg.call_info.call_state;
            pcall_info->direction = resp_msg.call_info.direction;
            if(resp_msg.remote_party_number_valid && resp_msg.remote_party_number.number_len > 0)
            {
                strcpy(pcall_info->phone_number, resp_msg.remote_party_number.number);
            } 
    
            printf("pcall_info: call_id = %d, status = %d, phone_number=%s\n",
                    pcall_info->call_id, pcall_info->call_state,pcall_info->phone_number);
        }
    }
    return 0;
}

int voice_get_all_call_info(call_info_type *pcall_info_list)
{
    voice_get_all_call_info_req_msg_v02   req_msg;
    voice_get_all_call_info_resp_msg_v02  resp_msg;
    qmi_client_error_type                 qmi_error; 
    int                                   i, j;
    
    LOG(">>>>>>voice_get_all_call_info:\n");
    memset(&req_msg,  0, sizeof(voice_get_all_call_info_req_msg_v02)); 
    memset(&resp_msg, 0, sizeof(voice_get_all_call_info_resp_msg_v02));

    qmi_error = qmi_client_send_msg_sync(voice_svc_client,
                                        QMI_VOICE_GET_ALL_CALL_INFO_REQ_V02, 
                                        &req_msg, 
                                        sizeof(voice_get_all_call_info_req_msg_v02), 
                                        &resp_msg, 
                                        sizeof(voice_get_all_call_info_resp_msg_v02),
                                        VOICE_QMI_TIMEOUT_VALUE);
    if (QMI_NO_ERR != qmi_error)
    {
        LOG("qmi get call info err=%d\n", qmi_error);
        return -1;
    }
    else if (QMI_NO_ERR != resp_msg.resp.result)
    {
        LOG("qmi get call info: failed response err=%d\n",resp_msg.resp.error); 
        return -1;
    }
    else
    {
        if(resp_msg.call_info_valid)
        {
            for(i = 0; i < resp_msg.call_info_len; i++)
            {
                pcall_info_list[i].call_id = resp_msg.call_info[i].call_id;
                pcall_info_list[i].call_state = resp_msg.call_info[i].call_state;
                pcall_info_list[i].direction = resp_msg.call_info[i].direction;
                if(resp_msg.remote_party_number_valid)
                {
                    for(j = 0; j < resp_msg.remote_party_number_len; j++)
                    {
                        if((resp_msg.remote_party_number[j].call_id == resp_msg.call_info[i].call_id) &&
                           resp_msg.remote_party_number[j].number_len > 0)
                        {
                            strcpy(pcall_info_list[i].phone_number, resp_msg.remote_party_number[j].number);
                            break;
                        }
                    }
                } 

                LOG("call_info_list[%d]: call_id = %d, status = %d, phone_number=%s\n",
                        i,pcall_info_list[i].call_id, pcall_info_list[i].call_state,pcall_info_list[i].phone_number);
            }
        }
    }
    return 0;
}

void voice_qmi_release(void)
{
    qmi_client_release(voice_svc_client);
    voice_svc_obj = NULL;
}






/*****************************************************************************
* Function Name : hangUp_answer
* Description   : 接听或电话
* Input         : unsigned char isAnswer
                  isAnswer -->1  挂断
                  isAnswer -->2  接听
* Output        : None
* Return        : -1:failed, 0:success
* Auther        : ygg
* Date          : 2018.03.15
*****************************************************************************/
int hangUp_answer(voice_call_option op, unsigned char callid)
{
	if((op != VOICE_CALL_HANDUP) && (op != VOICE_CALL_ANSWER))
	{
		return -1;
	}

	LteAtCtrl->switch_stepSPK(0); /* spk 音量缓慢降低 到 静音*/

	return voice_mtcall_process(op, callid);
}

/*****************************************************************************
* Function Name : voiceCall
* Description   : 拨打电话
* Input         : char *callingNumber
* Output        : None
* Return        : -1:failed, 0:success
* Auther        : ygg
* Date          : 2018.03.10
*****************************************************************************/
int voiceCall(char *callingNumber, phone_type phoneType)
{
	
	if(PHONE_TYPE_E == phoneType && tboxInfo.operateionStatus.phoneType != 1){
			hangUp_answer(VOICE_CALL_HANDUP, Now_call_id);
	}
	struct tm *p_tm = NULL;
	time_t tmp_time;
	tmp_time = time(NULL);
	p_tm = gmtime(&tmp_time);
	
	char ECALL[20];
	char BCALL[20];
	if(callingNumber == NULL)
		return -1;

	memset(ECALL, 0, sizeof(ECALL));
	dataPool->getPara(E_CALL_ID, (void *)ECALL, sizeof(ECALL));

	memset(BCALL, 0, sizeof(BCALL));
	dataPool->getPara(B_CALL_ID, (void *)BCALL, sizeof(BCALL));
	
	if(tboxInfo.operateionStatus.phoneType == 0)
	{
		int iret = voice_dial(callingNumber);
		
		if( iret == -1)
		{
			tboxInfo.operateionStatus.phoneType = 0;
			//printf("===voice_dial failed!===callingNumber == %s === \n", callingNumber);
			return -1;
		}
		else
		{
			if(PHONE_TYPE_E == phoneType)
			{
				tboxInfo.operateionStatus.phoneType = 1;
			}
			else if(PHONE_TYPE_B == phoneType)
			{
				tboxInfo.operateionStatus.phoneType = 2;
			}
			else
			{
				tboxInfo.operateionStatus.phoneType = 3;
			}
			//printf("===voice_dial success!===callingNumber == %s === \n", callingNumber);
		}
	}

	return 0;
}

/*****************************************************************************
* Function Name : voiceCall_init
* Description   : 初始化电话功能
* Input         : None
* Output        : None
* Return        : -1:failed, 0:success
* Auther        : ygg
* Date          : 2018.03.10
*****************************************************************************/
int voiceCall_init()
{
	if(voice_qmi_init() == -1)
		return -1;
	if(voice_qmi_ind_register() == -1)
		return -1;

	return 0;
}

#if 0
int main(int argc , char ** argv)
{
  char phone_num[64] = {0};
  voice_qmi_init();

  voice_qmi_ind_register();
  
  while(1)
  {
	  memset(phone_num, 0, sizeof(phone_num));
	  printf("\nPlease Enter the phone num:\n\n");
	  printf("Option > ");

	  /* Read the option from the standard input. */
	  if (fgets(phone_num, sizeof(phone_num), stdin) == NULL)
		continue;

	  //input enter
	  if ((phone_num[0] == 0x0A) && strlen(phone_num) == 1)
		continue;

	  phone_num[strlen(phone_num)-1] = 0;

	  //Q or q will quit the while
	  if (((phone_num[0] == 0x71) || (phone_num[0] == 0x51)) && strlen(phone_num) == 1)
		break;
      voice_dial(phone_num);
		continue;
  }

  voice_qmi_release();

  return 1;
}
#endif
