/*
  SIMCOM SIM7600 QMI PROFILE DEMO
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "NASControl.h"

#define LOG printf


static qmi_idl_service_object_type nas_service_object = NULL;
static qmi_client_type             nas_svc_client;
static qmi_client_os_params        nas_os_params;
static qmi_client_type             nas_notifier;

static void nas_ind_cb
(
    qmi_client_type                user_handle,
    unsigned int                   msg_id,
    void                          *ind_buf,
    unsigned int                   ind_buf_len,
    void                          *ind_cb_data
);


int nas_qmi_init(void)
{
    unsigned int num_services = 0, num_entries = 0;
    qmi_client_error_type qmi_error = QMI_NO_ERR;
    int num_retries = 0;

    if(nas_svc_client != NULL)
    {
        return 0;
    }
    
    nas_service_object = nas_get_service_object_v01();
    if(nas_service_object == NULL)
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
        qmi_error = qmi_client_init_instance(nas_service_object,
                                           QMI_CLIENT_INSTANCE_ANY,
                                           nas_ind_cb,
                                           NULL,
                                           &nas_os_params,
                                           (int) 4,
                                           &nas_svc_client);
        num_retries++;
    } while ( (qmi_error != QMI_NO_ERR) && (num_retries < 2) );


    if ( qmi_error == QMI_NO_ERR )
    {
        qmi_error =  qmi_client_notifier_init(nas_service_object,
                                              &nas_os_params,
                                              &nas_notifier);
                                              
        LOG("qmi_client_notifier_init status: %d\n", (int) qmi_error);
    }
    else
    {
        nas_service_object = NULL;
        return -1;
    }
    return 0;
}


void nas_qmi_release(void)
{
    qmi_client_release(nas_svc_client);
    nas_service_object = NULL;
}


static void nas_ind_cb
(
    qmi_client_type                user_handle,
    unsigned int                   msg_id,
    void                          *ind_buf,
    unsigned int                   ind_buf_len,
    void                          *ind_cb_data
)
{
    qmi_client_error_type           qmi_error = QMI_NO_ERR;
    nas_sys_info_ind_msg_v01        sys_info_ind;
    nas_rf_band_info_ind_msg_v01    band_info_ind;
    nas_serving_system_ind_msg_v01  serving_sys_info_ind;
    nas_sig_info_ind_msg_v01        sig_info_ind;
    nas_service_domain_enum_type_v01 srv_domain         = NAS_SERVICE_DOMAIN_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_domain_enum_type_v01 srv_domain_lte     = NAS_SERVICE_DOMAIN_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_domain_enum_type_v01 srv_domain_hdr     = NAS_SERVICE_DOMAIN_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_domain_enum_type_v01 srv_domain_cdma    = NAS_SERVICE_DOMAIN_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_domain_enum_type_v01 srv_domain_wcdma   = NAS_SERVICE_DOMAIN_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_domain_enum_type_v01 srv_domain_gsm     = NAS_SERVICE_DOMAIN_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_domain_enum_type_v01 srv_domain_tdscdma = NAS_SERVICE_DOMAIN_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_status_enum_type_v01 srv_status         = NAS_SERVICE_STATUS_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_status_enum_type_v01 srv_status_lte     = NAS_SERVICE_STATUS_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_status_enum_type_v01 srv_status_hdr     = NAS_SERVICE_STATUS_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_status_enum_type_v01 srv_status_cdma    = NAS_SERVICE_STATUS_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_status_enum_type_v01 srv_status_wcdma   = NAS_SERVICE_STATUS_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_status_enum_type_v01 srv_status_gsm     = NAS_SERVICE_STATUS_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_service_status_enum_type_v01 srv_status_tdscdma = NAS_SERVICE_STATUS_ENUM_TYPE_MIN_ENUM_VAL_V01;
    nas_roam_status_enum_type_v01 roam_status = NAS_ROAM_STATUS_ENUM_TYPE_MIN_ENUM_VAL_V01;

    int                 err;
    int                 *qcmap_cm_errno = &err;
    int                 ret_val;
    qmi_error_type_v01  qmi_err_num;
    int                 i;
    network_info_type   network_info;
    
    memset(&sys_info_ind, 0, sizeof(nas_sys_info_ind_msg_v01));
    memset(&band_info_ind, 0, sizeof(nas_rf_band_info_ind_msg_v01));
    memset(&serving_sys_info_ind, 0, sizeof(serving_sys_info_ind));
    memset(&sig_info_ind, 0, sizeof(nas_sig_info_ind_msg_v01));

    switch (msg_id)
    {
        case QMI_NAS_SYS_INFO_IND_MSG_V01:
            printf("-----------------------------\n");
            qmi_error = qmi_client_message_decode(user_handle,
                                                  QMI_IDL_INDICATION,
                                                  msg_id,
                                                  ind_buf,
                                                  ind_buf_len,
                                                  &sys_info_ind,
                                                  sizeof(nas_sys_info_ind_msg_v01));
            if (qmi_error == QMI_NO_ERR)
            {
                /* Local domain and status variables */
                srv_domain_lte     = sys_info_ind.lte_sys_info.common_sys_info.srv_domain;
                srv_domain_hdr     = sys_info_ind.hdr_sys_info.common_sys_info.srv_domain;
                srv_domain_cdma    = sys_info_ind.cdma_sys_info.common_sys_info.srv_domain;
                srv_domain_wcdma   = sys_info_ind.wcdma_sys_info.common_sys_info.srv_domain;
                srv_domain_gsm     = sys_info_ind.gsm_sys_info.common_sys_info.srv_domain;
                srv_domain_tdscdma = sys_info_ind.tdscdma_sys_info.common_sys_info.srv_domain;
                srv_status_lte     = sys_info_ind.lte_srv_status_info.srv_status;
                srv_status_hdr     = sys_info_ind.hdr_srv_status_info.srv_status;
                srv_status_cdma    = sys_info_ind.cdma_srv_status_info.srv_status;
                srv_status_wcdma   = sys_info_ind.wcdma_srv_status_info.srv_status;
                srv_status_gsm     = sys_info_ind.gsm_srv_status_info.srv_status;
                srv_status_tdscdma = sys_info_ind.tdscdma_srv_status_info.srv_status;

                /* First Get the Service Domain. */
                /* If the LTE System Info is valid, check the LTE Service Domain. */
                if (sys_info_ind.lte_sys_info_valid == TRUE &&
                    sys_info_ind.lte_sys_info.common_sys_info.srv_domain_valid == TRUE &&
                    srv_status_lte == NAS_SYS_SRV_STATUS_SRV_V01 &&
                    (srv_domain_lte == SYS_SRV_DOMAIN_PS_ONLY_V01 || srv_domain_lte == SYS_SRV_DOMAIN_CS_PS_V01))
                {
                    srv_domain = sys_info_ind.lte_sys_info.common_sys_info.srv_domain;
                    srv_status = sys_info_ind.lte_srv_status_info.srv_status;
                    if (sys_info_ind.lte_sys_info.common_sys_info.roam_status_valid == TRUE)
                    {
                        roam_status = sys_info_ind.lte_sys_info.common_sys_info.roam_status;
                    }
                    LOG("SYS_INFO_IND: LTE Service Domain %d Status %d, roam status %d\n", srv_domain, srv_status, roam_status);
                }
                /* If the HDR System Info is valid, check the HDR Service Domain. */
                else if (sys_info_ind.hdr_sys_info_valid == TRUE &&
                         sys_info_ind.hdr_sys_info.common_sys_info.srv_domain_valid == TRUE &&
                         srv_status_hdr == NAS_SYS_SRV_STATUS_SRV_V01 &&
                        (srv_domain_hdr == SYS_SRV_DOMAIN_PS_ONLY_V01 || srv_domain_hdr == SYS_SRV_DOMAIN_CS_PS_V01))
                {
                    srv_domain = sys_info_ind.hdr_sys_info.common_sys_info.srv_domain;
                    srv_status = sys_info_ind.hdr_srv_status_info.srv_status;
                    if (sys_info_ind.hdr_sys_info.common_sys_info.roam_status_valid == TRUE)
                    {
                        roam_status = sys_info_ind.hdr_sys_info.common_sys_info.roam_status;
                    }
                    LOG("SYS_INFO_IND: HDR Service Domain %d Status %d, roam status %d\n", srv_domain, srv_status, roam_status);
                }
                /* If the CDMA System Info is valid, check the CDMA Service Domain. */
                else if (sys_info_ind.cdma_sys_info_valid == TRUE &&
                        sys_info_ind.cdma_sys_info.common_sys_info.srv_domain_valid == TRUE &&
                        srv_status_cdma == NAS_SYS_SRV_STATUS_SRV_V01 &&
                        (srv_domain_cdma == SYS_SRV_DOMAIN_PS_ONLY_V01 || srv_domain_cdma == SYS_SRV_DOMAIN_CS_PS_V01))
                {
                    srv_domain = sys_info_ind.cdma_sys_info.common_sys_info.srv_domain;
                    srv_status = sys_info_ind.cdma_srv_status_info.srv_status;
                    if (sys_info_ind.cdma_sys_info.common_sys_info.roam_status_valid == TRUE)
                    {
                        roam_status = sys_info_ind.cdma_sys_info.common_sys_info.roam_status;
                    }
                    LOG("SYS_INFO_IND: CDMA Service Domain %d Status %d, roam status %d\n", srv_domain, srv_status, roam_status);
                }
                /* If the WCDMA System Info is valid, check the WCDMA Service Domain. */
                else if (sys_info_ind.wcdma_sys_info_valid == TRUE &&
                        sys_info_ind.wcdma_sys_info.common_sys_info.srv_domain_valid == TRUE &&
                        srv_status_wcdma == NAS_SYS_SRV_STATUS_SRV_V01 &&
                        (srv_domain_wcdma == SYS_SRV_DOMAIN_PS_ONLY_V01 || srv_domain_wcdma == SYS_SRV_DOMAIN_CS_PS_V01))
                {
                    srv_domain = sys_info_ind.wcdma_sys_info.common_sys_info.srv_domain;
                    srv_status = sys_info_ind.wcdma_srv_status_info.srv_status;
                    if (sys_info_ind.wcdma_sys_info.common_sys_info.roam_status_valid == TRUE)
                    {
                        roam_status = sys_info_ind.wcdma_sys_info.common_sys_info.roam_status;
                    }
                    LOG("SYS_INFO_IND: WCDMA Service Domain %d Status %d, roam status %d\n", srv_domain, srv_status, roam_status);
                }
                /* If the GSM System Info is valid, check the GSM Service Domain. */
                else if (sys_info_ind.gsm_sys_info_valid == TRUE &&
                      sys_info_ind.gsm_sys_info.common_sys_info.srv_domain_valid == TRUE &&
                      srv_status_gsm == NAS_SYS_SRV_STATUS_SRV_V01 &&
                      (srv_domain_gsm == SYS_SRV_DOMAIN_PS_ONLY_V01 || srv_domain_gsm == SYS_SRV_DOMAIN_CS_PS_V01))
                {
                    srv_domain = sys_info_ind.gsm_sys_info.common_sys_info.srv_domain;
                    srv_status = sys_info_ind.gsm_srv_status_info.srv_status;
                    if (sys_info_ind.gsm_sys_info.common_sys_info.roam_status_valid == TRUE)
                    {
                        roam_status = sys_info_ind.gsm_sys_info.common_sys_info.roam_status;
                    }
                    LOG("SYS_INFO_IND: GSM Service Domain %d Status %d, roam status %d\n", srv_domain, srv_status, roam_status);
                }
                /* If the TDSCDMA System Info is valid, check the TDSCDMA Service Domain. */
                else if (sys_info_ind.tdscdma_sys_info_valid == TRUE &&
                        sys_info_ind.tdscdma_sys_info.common_sys_info.srv_domain_valid == TRUE &&
                        srv_status_tdscdma == NAS_SYS_SRV_STATUS_SRV_V01 &&
                        (srv_domain_tdscdma == SYS_SRV_DOMAIN_PS_ONLY_V01 || srv_domain_tdscdma == SYS_SRV_DOMAIN_CS_PS_V01))
                {
                    srv_domain = sys_info_ind.tdscdma_sys_info.common_sys_info.srv_domain;
                    srv_status = sys_info_ind.tdscdma_srv_status_info.srv_status;
                    if (sys_info_ind.tdscdma_sys_info.common_sys_info.roam_status_valid == TRUE)
                    {
                        roam_status = sys_info_ind.tdscdma_sys_info.common_sys_info.roam_status;
                    }
                    LOG("SYS_INFO_IND: TDSCDMA Service Domain %d Status %d, roam status %d\n", srv_domain, srv_status, roam_status);
                }

                LOG("SYS_INFO_IND:srv_status[%d], srv_domain[%d]\n", srv_status, srv_domain, 0);
                if ((srv_status == NAS_SYS_SRV_STATUS_SRV_V01) &&
                    (srv_domain == SYS_SRV_DOMAIN_PS_ONLY_V01 || srv_domain == SYS_SRV_DOMAIN_CS_PS_V01))
                {
                    LOG("Modem in service NAS indication received\n");    
                }
            }
            else
            {
                LOG("SYS_INFO_IND: could not decode message %d\n", qmi_error, 0, 0);
            }
            break;


        case QMI_NAS_SERVING_SYSTEM_IND_MSG_V01:
            LOG("*********************************\n");
            
            memset(&network_info, 0, sizeof(network_info_type));
            qmi_error = qmi_client_message_decode(user_handle,
                                              QMI_IDL_INDICATION,
                                              msg_id,
                                              ind_buf,
                                              ind_buf_len,
                                              &serving_sys_info_ind,
                                              sizeof(nas_serving_system_ind_msg_v01));
            if (qmi_error == QMI_NO_ERR)
            {
                nas_registration_state_enum_v01 reg_state = serving_sys_info_ind.serving_system.registration_state;
                nas_ps_attach_state_enum_v01 ps_satat = serving_sys_info_ind.serving_system.ps_attach_state;

                LOG("SERVING_SYSTEM_IND: reg_state[%d], ps_satat[%d]\n", reg_state, ps_satat, 0);

                network_info.registration_state = serving_sys_info_ind.serving_system.registration_state;
                network_info.ps_attach_state = serving_sys_info_ind.serving_system.ps_attach_state;
                network_info.cs_attach_state = serving_sys_info_ind.serving_system.cs_attach_state;
                if(serving_sys_info_ind.serving_system.radio_if_len == 1)
                {
                    network_info.radio_if_type = (radio_if_type_enum)serving_sys_info_ind.serving_system.radio_if[0];
                }
                else if(serving_sys_info_ind.serving_system.radio_if_len > 1)
                {   
                    nas_radio_if_enum_v01 rf0 = serving_sys_info_ind.serving_system.radio_if[0];
                    nas_radio_if_enum_v01 rf1 = serving_sys_info_ind.serving_system.radio_if[1];
                    
                    if((rf0 == NAS_RADIO_IF_CDMA_1X_V01 && rf1 == NAS_RADIO_IF_LTE_V01) ||
                       (rf0 == NAS_RADIO_IF_LTE_V01 && rf1 == NAS_RADIO_IF_CDMA_1X_V01))
                    {
                        network_info.radio_if_type = RADIO_IF_1XLTE;
                    }
                }

                process_simcom_ind_message(SIMCOM_EVENT_NETWORK_IND,&network_info);

            }
            else
            {
                LOG("qcmap_cm_process_qmi_nas_ind: could not decode message %d", qmi_error, 0, 0);
            }
            break;
        case QMI_NAS_SIG_INFO_IND_MSG_V01:
            LOG("*********************************\n");
            memset(&network_info, 0, sizeof(network_info_type));
            
            qmi_error = qmi_client_message_decode(user_handle,
                                              QMI_IDL_INDICATION,
                                              msg_id,
                                              ind_buf,
                                              ind_buf_len,
                                              &sig_info_ind,
                                              sizeof(nas_sig_info_ind_msg_v01));
            if (qmi_error == QMI_NO_ERR)
            {
                LOG("**********sig ind*****\n");
                //process_simcom_ind_message(SIMCOM_EVENT_NETWORK_IND,&network_info);
            }
            else
            {
                LOG("qcmap_cm_process_qmi_nas_ind: could not decode message %d", qmi_error, 0, 0);
            }
            break;
            
        case QMI_NAS_RF_BAND_INFO_IND_V01:
            LOG("Receive QMI_NAS_RF_BAND_INFO_IND_V01\n");                
            break;            
        default:
            LOG("nas_ind_cb msg_id = 0x%X\n", msg_id);
            /* Ignore all other indications */
            break;
    }

}

int nas_ind_register(void)
{
    nas_indication_register_req_msg_v01 nas_ind_req_msg;
    nas_indication_register_resp_msg_v01 nas_ind_resp_msg;
    qmi_client_error_type           qmi_error = QMI_NO_ERR;
    
    memset(&nas_ind_req_msg, 0, sizeof(nas_indication_register_req_msg_v01));
    memset(&nas_ind_resp_msg, 0, sizeof(nas_indication_register_resp_msg_v01));
    #if 0
    nas_ind_req_msg.dual_standby_pref_valid = TRUE;
    nas_ind_req_msg.dual_standby_pref = 0x00;
    nas_ind_req_msg.err_rate_valid = TRUE;
    nas_ind_req_msg.err_rate = 0x00;
    nas_ind_req_msg.network_reject_valid = TRUE;
    nas_ind_req_msg.network_reject.reg_network_reject = 0x00;
    nas_ind_req_msg.reg_csp_plmn_mode_bit_valid = TRUE;
    nas_ind_req_msg.reg_csp_plmn_mode_bit =0x00;
    nas_ind_req_msg.reg_current_plmn_name_valid = TRUE;
    nas_ind_req_msg.reg_current_plmn_name = 0x00;
    nas_ind_req_msg.reg_ddtm_events_valid = TRUE;
    nas_ind_req_msg.reg_ddtm_events = 0x00;
    nas_ind_req_msg.reg_embms_status_valid = TRUE;
    nas_ind_req_msg.reg_embms_status = 0x00;
    nas_ind_req_msg.reg_hdr_session_close_valid = TRUE;
    nas_ind_req_msg.reg_hdr_session_close = 0x00;
    nas_ind_req_msg.reg_hdr_uati_valid = TRUE;
    nas_ind_req_msg.reg_hdr_uati = 0x00;
    nas_ind_req_msg.reg_managed_roaming_valid = TRUE;
    nas_ind_req_msg.reg_managed_roaming = 0x00;
    nas_ind_req_msg.reg_network_time_valid = TRUE;
    nas_ind_req_msg.reg_network_time = 0x00;
    nas_ind_req_msg.reg_operator_name_data_valid = TRUE;
    nas_ind_req_msg.reg_operator_name_data = 0x00;
    nas_ind_req_msg.reg_rf_band_info_valid = TRUE;
    nas_ind_req_msg.reg_rf_band_info = 0x00;
    nas_ind_req_msg.reg_rtre_cfg_valid = TRUE;
    nas_ind_req_msg.reg_rtre_cfg = 0x00;
    nas_ind_req_msg.reg_sys_sel_pref_valid = TRUE;
    nas_ind_req_msg.reg_sys_sel_pref = 0x00;

    nas_ind_req_msg.subscription_info_valid = TRUE;
    nas_ind_req_msg.subscription_info = 0x00;
    #endif
    #if 0
    nas_ind_req_msg.sys_info_valid = TRUE;
    nas_ind_req_msg.sys_info = 0x01;
    #endif
    
    nas_ind_req_msg.req_serving_system_valid = TRUE;
    nas_ind_req_msg.req_serving_system = 0x01;
    nas_ind_req_msg.sig_info_valid = TRUE;
    nas_ind_req_msg.sig_info = 0x00;   

    qmi_error = qmi_client_send_msg_sync(nas_svc_client,
                                         QMI_NAS_INDICATION_REGISTER_REQ_MSG_V01,
                                         &nas_ind_req_msg,
                                         sizeof(nas_indication_register_req_msg_v01),
                                         &nas_ind_resp_msg,
                                          sizeof(nas_indication_register_resp_msg_v01),
                                         NAS_QMI_TIMEOUT_VALUE);
   if (qmi_error != QMI_NO_ERR)
   {
       return -1;
   }
   return 0;
}

int nas_get_SignalStrength(int *level, int* mode)
{
    nas_get_signal_strength_req_msg_v01  req_msg;
    nas_get_signal_strength_resp_msg_v01 resp_msg;
    qmi_client_error_type     qmi_error = QMI_NO_ERR;


    memset(&req_msg, 0, sizeof(nas_get_signal_strength_req_msg_v01));
    memset(&resp_msg, 0, sizeof(nas_get_signal_strength_resp_msg_v01));

    
    req_msg.request_mask_valid = 1;
    req_msg.request_mask =  QMI_NAS_REQUEST_SIG_INFO_LTE_RSRP_MASK_V01|
                            QMI_NAS_REQUEST_SIG_INFO_RSSI_MASK_V01;

    qmi_error = qmi_client_send_msg_sync(nas_svc_client,
                                        QMI_NAS_GET_SIGNAL_STRENGTH_REQ_MSG_V01, 
                                        &req_msg, 
                                        sizeof(nas_get_signal_strength_req_msg_v01), 
                                        &resp_msg, 
                                        sizeof(nas_get_signal_strength_resp_msg_v01),
                                        NAS_QMI_TIMEOUT_VALUE);

    if (QMI_NO_ERR != qmi_error)
    {    
        LOG("qmi get sig err=%d\n",qmi_error);
        return -1;
                      
    }  
    else if (QMI_NO_ERR != resp_msg.resp.result)
    {   
        LOG("qmi get sig: failed response err=%d\n",resp_msg.resp.error);
        return -1;
    }  
    //printf("sig_strength %d, radio if 0x%x\n", resp_msg.signal_strength.sig_strength, resp_msg.signal_strength.radio_if);

    #if 0
    for(int i = 0; resp_msg.rssi_valid && i < resp_msg.rssi_len; i++)
    {
        printf("rssi-rssi[%d] rssi-radio_if[0x%x]\n",  resp_msg.rssi[i].rssi, resp_msg.rssi[i].radio_if);
    }
    #endif
    
    //printf("lte_rsrp_valid = %d, lte_rsrp = %d\n",  resp_msg.lte_rsrp_valid, resp_msg.lte_rsrp);
    
   /* if(resp_msg.lte_rsrp_valid)
    {
        int d_val = 0;
        if(resp_msg.lte_rsrp >= LTE_DB_MIN)
        {
            d_val = resp_msg.lte_rsrp - LTE_DB_MIN;
        }

        if(resp_msg.lte_rsrp > LTE_DB_MAX)
        {
            d_val = LTE_DB_MAX - LTE_DB_MIN - 1;
        }
        
        *level = d_val * 100 / (LTE_DB_MAX - LTE_DB_MIN);
        *mode = 1;  //LTE
        //printf("lte level = %d\n", *level);
    }
    else*/
    {
        int d_val = 0;
        if(resp_msg.signal_strength.sig_strength >= GSM_WCDMA_DB_MIN)
        {
            d_val = resp_msg.signal_strength.sig_strength - GSM_WCDMA_DB_MIN;
        }

        if(resp_msg.signal_strength.sig_strength > GSM_WCDMA_DB_MAX)
        {
            d_val = GSM_WCDMA_DB_MAX - GSM_WCDMA_DB_MIN - 1;
        }
        *mode = 0;  //GSM WCDMA
        *level = d_val * 100 / (GSM_WCDMA_DB_MAX - GSM_WCDMA_DB_MIN);
        //printf("gsm level = %d\n", *level);        
    }
    return 0;
}

int nas_get_NetworkType(nas_serving_system_type_v01 *serving_system)
{
    nas_get_serving_system_req_msg_v01  req_msg;
    nas_get_serving_system_resp_msg_v01 resp_msg;
    qmi_client_error_type     qmi_error = QMI_NO_ERR;

    if(serving_system == NULL)
    {
        return -1;
    }
    
    memset(&req_msg, 0, sizeof(nas_get_serving_system_req_msg_v01));
    memset(&resp_msg, 0, sizeof(nas_get_serving_system_resp_msg_v01));

    qmi_error = qmi_client_send_msg_sync(nas_svc_client,
                                        QMI_NAS_GET_SERVING_SYSTEM_REQ_MSG_V01, 
                                        &req_msg, 
                                        sizeof(nas_get_serving_system_req_msg_v01), 
                                        &resp_msg, 
                                        sizeof(nas_get_serving_system_resp_msg_v01),
                                        NAS_QMI_TIMEOUT_VALUE);   
    
    if (QMI_NO_ERR != qmi_error)
    {    
        LOG("qmi get serving system err=%d\n",qmi_error);
        return -1;
                      
    }  
    else if (QMI_NO_ERR != resp_msg.resp.result)
    {   
        LOG("qmi get serving system: failed response err=%d\n",resp_msg.resp.error);
        return -1;
    }

    memcpy(serving_system, &resp_msg.serving_system, sizeof(nas_serving_system_type_v01));

    return 0;
}

int nas_init()
{
	if(nas_qmi_init() == -1)
		return -1;
	if(nas_ind_register() == -1)
		return -1;

	return 0;
}

