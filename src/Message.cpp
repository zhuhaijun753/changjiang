/******************************************************************************

                            M C M _ SMS _ T E S T . C

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "Message.h"

#define TRUE 1
#define FALSE 0

#define MCM_SMS_SUCCESS 0
#define MCM_SMS_ERROR -1

#define MCM_SMS_TEST_LOG_ERROR(...) \
printf("ERROR:");           \
printf( __VA_ARGS__); \
printf("\n")
#define MCM_SMS_TEST_LOG(...) \
printf( __VA_ARGS__); \
printf("\n")
#define MCM_SMS_TEST_LOG_FUNC_ENTRY() \
MCM_SMS_TEST_LOG("\n%s: ENTRY", __FUNCTION__);
#define MCM_SMS_TEST_LOG_FUNC_EXIT() \
MCM_SMS_TEST_LOG("\n%s: EXIT", __FUNCTION__);

typedef struct {
  mcm_client_handle_type         mcm_client_handle;
} mcm_sms_conf_t;

mcm_sms_conf_t mcm_sms_config ={0};
static int token_id;

int codeconvert_get_utf8_size(uint8 *in)
{
   unsigned char c = *in;
    if(c< 0x80) return 0;
    if(c>=0x80 && c<0xC0) return 1;
    if(c>=0xC0 && c<0xE0) return 2;
    if(c>=0xE0 && c<0xF0) return 3;
    if(c>=0xF0 && c<0xF8) return 4;
    if(c>=0xF8 && c<0xFC) return 5;
    if(c>=0xFC) return 6;
}
static int codeconvert_utf82unicodeStr(uint8 *in, uint16 *out, int *outsize)  
{  
    uint8 *pInput = in;  
    unsigned char *pOutput = (unsigned char *)out;
    int utfbytes = 0; 
    char b1, b2, b3, b4, b5, b6;
    
    int resultsize = 0;  
    uint16 *tmp = out;  

    if(in == NULL || out == NULL || outsize == NULL)
    {
        return -1;
    }

    while(*pInput)  
    {  
        utfbytes = codeconvert_get_utf8_size(pInput);
        pOutput = (unsigned char *)(tmp);
        switch ( utfbytes )  
        {  
            case 0:  
                *pOutput     = *pInput;  
                utfbytes    += 1;  
                break;  
                
            case 2:  
                b1 = *pInput;  
                b2 = *(pInput + 1);  
                if ( (b2 & 0xE0) != 0x80 )  
                    return 0;  
                *pOutput     = (b1 << 6) + (b2 & 0x3F);  
                *(pOutput+1) = (b1 >> 2) & 0x07;  
                break;  
                
            case 3:  
                b1 = *pInput;  
                b2 = *(pInput + 1);  
                b3 = *(pInput + 2);  
                if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) )  
                    return 0;  
                *pOutput     = (b2 << 6) + (b3 & 0x3F);  
                *(pOutput+1) = (b1 << 4) + ((b2 >> 2) & 0x0F);  
                break;  
                
            case 4:  
                b1 = *pInput;  
                b2 = *(pInput + 1);  
                b3 = *(pInput + 2);  
                b4 = *(pInput + 3);  
                if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)  
                        || ((b4 & 0xC0) != 0x80) )  
                    return 0;  
                *pOutput     = (b3 << 6) + (b4 & 0x3F);  
                *(pOutput+1) = (b2 << 4) + ((b3 >> 2) & 0x0F);  
                *(pOutput+2) = ((b1 << 2) & 0x1C)  + ((b2 >> 4) & 0x03);  
                break;  
                
            case 5:  
                b1 = *pInput;  
                b2 = *(pInput + 1);  
                b3 = *(pInput + 2);  
                b4 = *(pInput + 3);  
                b5 = *(pInput + 4);  
                if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)  
                        || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80) )  
                    return 0;  
                *pOutput     = (b4 << 6) + (b5 & 0x3F);  
                *(pOutput+1) = (b3 << 4) + ((b4 >> 2) & 0x0F);  
                *(pOutput+2) = (b2 << 2) + ((b3 >> 4) & 0x03);  
                *(pOutput+3) = (b1 << 6);  
                break;  
                
            case 6:  
                b1 = *pInput;  
                b2 = *(pInput + 1);  
                b3 = *(pInput + 2);  
                b4 = *(pInput + 3);  
                b5 = *(pInput + 4);  
                b6 = *(pInput + 5);  
                if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)  
                        || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80)  
                        || ((b6 & 0xC0) != 0x80) )  
                    return 0;  
                *pOutput     = (b5 << 6) + (b6 & 0x3F);  
                *(pOutput+1) = (b5 << 4) + ((b6 >> 2) & 0x0F);  
                *(pOutput+2) = (b3 << 2) + ((b4 >> 4) & 0x03);  
                *(pOutput+3) = ((b1 << 6) & 0x40) + (b2 & 0x3F);  
                break;  
                
            default:  
                return 0;  
                break;  
        }   

        tmp++;
        pInput += utfbytes;  
        resultsize++;
    }  
  
    *outsize = resultsize;   
    return 0;  
}

static int codeconvert_unicode2utf8Str(uint16 *in, int insize, uint8 *out)  
{  
    int i = 0;  
    int outsize = 0;  
    int charscount = 0;  
    //uint8_t *result = NULL;  
    uint8_t *tmp = NULL;  

    if(in == NULL || out == NULL)
    {
        return -1;
    }
    
    charscount = insize;  
    //result = (uint8_t *)malloc(charscount * 3 + 1);  
    //memset(result, 0, charscount * 3 + 1);  
    tmp = out;//result;  
    for (i = 0; i < charscount; i++)  
    {  
         uint16_t unicode = in[i]>>8|(in[i]&0xFF)<<8;  
         if ( unicode <= 0x0000007F )  
         {  
             // * U-00000000 - U-0000007F:  0xxxxxxx  
             *tmp     = (unicode & 0x7F);  
             tmp++;  
         }  
         else if ( unicode >= 0x00000080 && unicode <= 0x000007FF )  
         {  
             // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx  
             *(tmp+1) = (unicode & 0x3F) | 0x80;  
             *tmp     = ((unicode >> 6) & 0x1F) | 0xC0;  
             tmp += 2;
         }  
         else if ( unicode >= 0x00000800 && unicode <= 0x0000FFFF )  
         {  
             // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx  
             *(tmp+2) = (unicode & 0x3F) | 0x80;  
             *(tmp+1) = ((unicode >>  6) & 0x3F) | 0x80;  
             *tmp     = ((unicode >> 12) & 0x0F) | 0xE0;   
             tmp += 3;
         }  
         else if ( unicode >= 0x00010000 && unicode <= 0x001FFFFF )  
         {  
             // * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  
             *(tmp+3) = (unicode & 0x3F) | 0x80;  
             *(tmp+2) = ((unicode >>  6) & 0x3F) | 0x80;  
             *(tmp+1) = ((unicode >> 12) & 0x3F) | 0x80;  
             *tmp     = ((unicode >> 18) & 0x07) | 0xF0;  
             tmp += 4;
         }  
         else if ( unicode >= 0x00200000 && unicode <= 0x03FFFFFF )  
         {  
             // * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
             *(tmp+4) = (unicode & 0x3F) | 0x80;  
             *(tmp+3) = ((unicode >>  6) & 0x3F) | 0x80;  
             *(tmp+2) = ((unicode >> 12) & 0x3F) | 0x80;  
             *(tmp+1) = ((unicode >> 18) & 0x3F) | 0x80;  
             *tmp     = ((unicode >> 24) & 0x03) | 0xF8;   
             tmp += 5;
         }  
         else if ( unicode >= 0x04000000 && unicode <= 0x7FFFFFFF )  
         {  
             // * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
             *(tmp+5) = (unicode & 0x3F) | 0x80;  
             *(tmp+4) = ((unicode >>  6) & 0x3F) | 0x80;  
             *(tmp+3) = ((unicode >> 12) & 0x3F) | 0x80;  
             *(tmp+2) = ((unicode >> 18) & 0x3F) | 0x80;  
             *(tmp+1) = ((unicode >> 24) & 0x3F) | 0x80;  
             *tmp     = ((unicode >> 30) & 0x01) | 0xFC; 
             tmp += 6;
         }  
    }  
  
    //*tmp = '\0';  
    //*out = result;  
    return 0;  
}

char OCTET_TO_CHAR(char octet)
{
  char rc;
  
  if (octet>=0 && octet<=9)  //0-9
  {
    rc = octet + '0';
  }else if (octet>=10 && octet<=15) //A-F
  {
    rc = 'A' + (octet - 10);
  }else
  {
    printf("error octet\n");
  }

  return rc;
}

 char CHAR_TO_OCTET( char ch)
{
  char rc;
  
  if (ch>='0' && ch<='9')  //0-9
  {
    rc = ch - '0';
  }else if (ch>='A' && ch<='F') //A-F
  {
    rc = ch - 'A' + 10;
  }else if (ch>='a' && ch<='f') //a-f
  {
    rc = ch - 'a' + 10;
  }else
  {
    printf("error char\n");
  }

  return rc;
}

int mcm_hex_to_bin
(
  const unsigned char * in_str,
  const int      in_str_len,
  unsigned char *         out_str,
  int              out_max_len
)
{
  int i,j;
  
  if (in_str_len%2 !=0) return 0;

  if (out_max_len < in_str_len/2) return 0;

  for (i=0,j=0; i<in_str_len; i+=2)
  {
    out_str[j++] =CHAR_TO_OCTET(in_str[i])*16 + CHAR_TO_OCTET(in_str[i+1]);
  }
  return j;
  
}

void mcm_bin_to_hex
(
  const  char * in_str,
  const int      in_str_len,
  char *         out_str,
  int              out_max_len
)
{
  int i,j;
  
  if (out_max_len < in_str_len*2) return;

  for (i=0,j=0; i<in_str_len; i++)
  {
    out_str[j++] = OCTET_TO_CHAR(in_str[i]/16);
    out_str[j++] = OCTET_TO_CHAR(in_str[i]%16);
  }
}

int mcm_get_unicode_len(unsigned char *inStr)
{
  int i,len=0;

  for (i=0;;i+=2)
  {
    if ((inStr[i] == 0) && (inStr[i+1] == 0))
      break;
    len += 2;
  }
  return len;
}

static void mcm_sms_test_cli_ind_cb
(
    mcm_client_handle_type hndl,         /* QMI user handle       */
    uint32                 msg_id,       /* Indicator message ID  */
    void                   *ind_data,    /* Raw indication data   */
    unsigned int           ind_buf_len   /* Raw data length       */
)
{
    int i,len;
    mcm_sms_pp_ind_msg_v01 *pp_sms;
    MCM_SMS_TEST_LOG_FUNC_ENTRY();
    switch(msg_id)
    {
        case MCM_SMS_PP_IND_V01:
            sms_info_type sms_info;
            memset(&sms_info, 0, sizeof(sms_info_type));
            pp_sms = (mcm_sms_pp_ind_msg_v01 *)ind_data;
            MCM_SMS_TEST_LOG("=== Received a pp message ===\n");
            MCM_SMS_TEST_LOG("message_format = %d\n",pp_sms->message_format);
            MCM_SMS_TEST_LOG("message_id = %llu\n",pp_sms->message_id);
            if (pp_sms->message_format == MCM_SMS_MSG_FORMAT_TEXT_UTF8_V01)
            {   
                unsigned char utf8_msg[256] = {0};
                char content_tmp[MCM_SMS_MAX_MT_MSG_LENGTH_V01 + 1] = {0};
                char tmp_val;
                len = mcm_get_unicode_len((unsigned char *)(pp_sms->message_content));
                printf("len = %d, message_content=", len);
                for (i=0; i<len; i++)
                printf("%02X ", pp_sms->message_content[i]);
                printf("\n");
                

                codeconvert_unicode2utf8Str((uint16 *)pp_sms->message_content,
                                            len,
                                            utf8_msg); 
                #if 0                      
                for(i = 0; i < strlen(utf8_msg); i++)
                {
                    printf("utf8_str: %02X\n",utf8_msg[i]);
                }
                #endif
                strcpy(sms_info.message_content, utf8_msg);
        
            }
            else if (pp_sms->message_format == MCM_SMS_MSG_FORMAT_TEXT_ASCII_V01) 
            {
                //MCM_SMS_TEST_LOG("message_content = %s", pp_sms->message_content);
                strcpy(sms_info.message_content,pp_sms->message_content);
            }
            //MCM_SMS_TEST_LOG("source_address = %s", pp_sms->source_address);
            strcpy(sms_info.source_address,pp_sms->source_address);
            if (pp_sms->message_class_valid)
                MCM_SMS_TEST_LOG("message_class = %d",pp_sms->message_class);
            MCM_SMS_TEST_LOG("=========END==========");
            process_simcom_ind_message(SIMCOM_EVENT_SMS_PP_IND,&sms_info);
            break;
            
        case MCM_SMS_CB_IND_V01:
            MCM_SMS_TEST_LOG("=== Received a cb message ===");
            break;
            
        case MCM_SMS_CB_CMAS_IND_V01:
            MCM_SMS_TEST_LOG("=== Received a cmas message ===");
            break;
            
        default:
            MCM_SMS_TEST_LOG("=== Received a unknow message ===");
            break;
    }
    MCM_SMS_TEST_LOG_FUNC_EXIT();
}

static void mcm_sms_test_async_cb
(
    mcm_client_handle_type hndl,
    uint32                 msg_id,
    void                  *resp_c_struct,
    uint32                 resp_c_struct_len,
    void                  *token_id
)
{
    //TODO 
}

static int mcm_sms_init()
{
    MCM_SMS_TEST_LOG_FUNC_ENTRY();

    mcm_error_t_v01 mcm_error;

    if(mcm_sms_config.mcm_client_handle != 0)
    {
        MCM_SMS_TEST_LOG("Client already initialized successfully 0x%x",
                        &mcm_sms_config.mcm_client_handle,0,0);
        return MCM_SMS_SUCCESS;
    }

    mcm_error = (mcm_error_t_v01)mcm_client_init(&mcm_sms_config.mcm_client_handle,
                                mcm_sms_test_cli_ind_cb,
                                mcm_sms_test_async_cb);

    /* Continue only if client initialized for atleast one mcm service */
    if ((mcm_error != MCM_SUCCESS_V01) && (mcm_error != MCM_SUCCESS_CONDITIONAL_SUCCESS_V01))
    {
        MCM_SMS_TEST_LOG("mcm_client_init: Failed with error %d", mcm_error);
        return MCM_SMS_ERROR;
    }
    else
    {
        MCM_SMS_TEST_LOG("mcm_client_init: Succeded with client_handle 0x%x",
                        mcm_sms_config.mcm_client_handle);
    }
    MCM_SMS_TEST_LOG_FUNC_EXIT();
    return MCM_SMS_SUCCESS;
}

//void mcm_sms_send(mcm_sms_msg_format_t_v01 format, char *dest_addr, char *data)
int mcm_sms_send(mcm_sms_msg_format_t_v01 format, char *dest_addr, unsigned char *data, int data_len)
{
    mcm_error_t_v01 mcm_error;
    mcm_sms_send_mo_msg_req_msg_v01 sms_req;
    mcm_sms_send_mo_msg_resp_msg_v01 sms_resp;
    int i;

    memset(&sms_req, 0, sizeof(mcm_sms_send_mo_msg_req_msg_v01));
    memset(&sms_resp, 0, sizeof(mcm_sms_send_mo_msg_resp_msg_v01));

    sms_req.message_format = format;
    if (format == MCM_SMS_MSG_FORMAT_TEXT_ASCII_V01)
    {
        memcpy(sms_req.message_content, data, data_len);
    }
    else if (format == MCM_SMS_MSG_FORMAT_TEXT_UTF8_V01)
    {
        //mcm_hex_to_bin((const unsigned char *)data, strlen(data), (unsigned char *)sms_req.message_content, MCM_SMS_MAX_MO_MSG_LENGTH_V01);
        memcpy((unsigned char *)sms_req.message_content, data, data_len);
    }
    memcpy(sms_req.destination, dest_addr, strlen(dest_addr));
  
    mcm_error =  (mcm_error_t_v01)MCM_CLIENT_EXECUTE_COMMAND_SYNC(mcm_sms_config.mcm_client_handle,
                            MCM_SMS_SEND_MO_MSG_REQ_V01,
                            &sms_req, &sms_resp);
    if (mcm_error == MCM_SUCCESS_V01)
    {
        if (sms_resp.response.result == MCM_RESULT_SUCCESS_V01)
        {
            MCM_SMS_TEST_LOG("send sms successful!!!");
            return 0;
        }
        else
        {
            MCM_SMS_TEST_LOG_ERROR("send sms failure error=%d",sms_resp.response.error);
	  return 1;
    }
  }
  else
  {
    MCM_SMS_TEST_LOG_ERROR("send sms failure mcm_err=%d",mcm_error);
	return -1;
  }
}

void mcm_sms_set_service_center()
{
    mcm_error_t_v01 mcm_error;
    mcm_sms_set_service_center_cfg_type_req_msg_v01 smsc_req;
    mcm_sms_set_service_center_cfg_type_resp_msg_v01 smsc_resp;

    memset(&smsc_req, 0, sizeof(mcm_sms_set_service_center_cfg_type_req_msg_v01));
    memset(&smsc_resp, 0, sizeof(mcm_sms_set_service_center_cfg_type_resp_msg_v01));

    strncpy(smsc_req.service_center_addr, "+8613800210500", strlen("+8613800210500"));
    smsc_req.validity_time_valid = FALSE;

    mcm_error =  (mcm_error_t_v01)MCM_CLIENT_EXECUTE_COMMAND_SYNC(mcm_sms_config.mcm_client_handle,
                            MCM_SMS_SET_SERVICE_CENTER_CFG_TYPE_REQ_V01,
                            &smsc_req, &smsc_resp);
    if (mcm_error == MCM_SUCCESS_V01)
    {
        if (smsc_resp.response.result == MCM_RESULT_SUCCESS_V01)
        {
            MCM_SMS_TEST_LOG("set smsc successful!!!");
        }
        else
        {
          MCM_SMS_TEST_LOG_ERROR("set smsc failure error=%d",smsc_resp.response.error);
        }
    }
    else
    {
        MCM_SMS_TEST_LOG_ERROR("set smsc failure mcm_err=%d",mcm_error);
    }
}

int mcm_get_sms_service_center()
{
    mcm_error_t_v01 mcm_error;
    mcm_sms_get_service_center_cfg_type_req_msg_v01 smsc_req;
    mcm_sms_get_service_center_cfg_type_resp_msg_v01 smsc_resp;


    memset(&smsc_req, 0, sizeof(mcm_sms_get_service_center_cfg_type_req_msg_v01));
    memset(&smsc_resp, 0, sizeof(mcm_sms_set_service_center_cfg_type_resp_msg_v01));
  
    mcm_error =  (mcm_error_t_v01)MCM_CLIENT_EXECUTE_COMMAND_SYNC(mcm_sms_config.mcm_client_handle,
                            MCM_SMS_GET_SERVICE_CENTER_CFG_TYPE_REQ_V01,
                            &smsc_req, &smsc_resp);
    if (mcm_error == MCM_SUCCESS_V01)
    {
        if (smsc_resp.response.result == MCM_RESULT_SUCCESS_V01)
        {
            if (smsc_resp.service_center_addr_valid == TRUE)
            {
                MCM_SMS_TEST_LOG("get smsc addr %s", smsc_resp.service_center_addr);
            }
            
            if (smsc_resp.validity_time_valid== TRUE)
            {
                MCM_SMS_TEST_LOG("get smsc time %ld", smsc_resp.validity_time);
            }
        }
        else
        {
            MCM_SMS_TEST_LOG_ERROR("set smsc failure error=%d",smsc_resp.response.error);
        }
    }
    else
    {
        MCM_SMS_TEST_LOG_ERROR("set smsc failure mcm_err=%d",mcm_error);
    }
}

int mcm_sms_event_register()
{
    mcm_error_t_v01 mcm_error;
    mcm_sms_event_register_req_msg_v01 reg_req;
    mcm_sms_event_register_resp_msg_v01 reg_resp;

    memset(&reg_req, 0, sizeof(mcm_sms_event_register_req_msg_v01));
    memset(&reg_resp, 0, sizeof(mcm_sms_event_register_resp_msg_v01));

    reg_req.register_sms_pp_event_valid = TRUE;
    reg_req.register_sms_pp_event = TRUE;

    mcm_error =  (mcm_error_t_v01)MCM_CLIENT_EXECUTE_COMMAND_SYNC(mcm_sms_config.mcm_client_handle,
                                MCM_SMS_EVENT_REGISTER_REQ_V01,
                                &reg_req, &reg_resp);
    
    if (mcm_error == MCM_SUCCESS_V01)
    {
        if (reg_resp.response.result == MCM_RESULT_SUCCESS_V01)
        {
            MCM_SMS_TEST_LOG("register sms event successful");
        }
        else
        {
            MCM_SMS_TEST_LOG_ERROR("register sms event failure error=%d",reg_resp.response.error);
            return MCM_SMS_ERROR;
        }
    }
    else
    {
        MCM_SMS_TEST_LOG_ERROR("register sms event failure mcm_err=%d",mcm_error);
        return MCM_SMS_ERROR;
    }

    return MCM_SMS_SUCCESS;
}

#if 0
void mcm_sms_start_menu()
  {
    int32 result = MCM_SMS_SUCCESS;
    char phone_num[25+1] = {0};
    char send_msg[500+1] = {0};
    char format = 0;

    int i;

    while(1)
    {
      memset(phone_num, 0, sizeof(phone_num));
      memset(send_msg, 0, sizeof(send_msg));
	  
	  while(1)
	  {
		  printf("\nPlease Enter the format(1:7bit 2:ucs2):\n\n");
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
			return;

		  format = phone_num[0] -'0';
		  
		  if (format ==1 || format == 2)
			break;
	  }
      
	  while(1)
	  {
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
			return;

		  if ( strlen(phone_num) != 11)
		  {
			MCM_SMS_TEST_LOG("Error: phone num length must be 11 ");
			continue;
		  }
		  
		  break;
	  }

      printf("\nPlease Enter the msg content:\n\n");
      printf("Option > ");
      
      /* Read the option from the standard input. */
      if (fgets(send_msg, sizeof(send_msg), stdin) == NULL)
        continue;

      send_msg[strlen(send_msg)-1] = 0;

      mcm_sms_send((mcm_sms_msg_format_t_v01)format, phone_num, send_msg);
    }
}
#endif

void mcm_sms_release()
{
    mcm_client_release(mcm_sms_config.mcm_client_handle);
    mcm_sms_config.mcm_client_handle = 0;
}

#if 0
int main(int argc,  char **argv)
{
  MCM_SMS_TEST_LOG("mcm sms main start");
  mcm_sms_test_init();
  mcm_sms_event_register();
  mcm_sms_start_menu();
  mcm_sms_release();
  MCM_SMS_TEST_LOG("main exit");
  return MCM_SMS_SUCCESS;
}
#endif





/*****************************************************************************
* Function Name : send_message
* Description   : 发送短信
                  smsMode->1 只能发送英文
                  smsMode->2 可以发送UCS-2的文字（可发送中文只支持UCS2类型的文字）
* Input         : int smsMode, char *phoneNumber, char *content, int content_len
* Output        : None
* Return        : -1:error, 0: success, 1:发送出错
* Auther        : ygg
* Date          : 2018.03.10
*****************************************************************************/
int send_message(int smsMode, char *phoneNumber, unsigned char *content, int content_len)
{
	int ucs_len;
	unsigned short ucs_data[140] = {0};

	if (smsMode < 1 || smsMode > 2)
	{
		MCM_SMS_TEST_LOG("Error: smsMode must be 0 or 1\n");
		return -1;
	}

	if (strlen(phoneNumber) != 11)
	{
		MCM_SMS_TEST_LOG("Error: phone num length must be 11\n");
		return -1;
	}

	if(smsMode == 1)
	{
		return mcm_sms_send((mcm_sms_msg_format_t_v01)smsMode, phoneNumber,content, content_len);
	}
	else
	{
		codeconvert_utf82unicodeStr(content, ucs_data, &ucs_len);
		for(int i = 0; i < ucs_len; i++)
		{
			ucs_data[i] = ucs_data[i]>>8|(ucs_data[i]&0xFF)<<8;
		}

        printf("\nUTF8:%02X %02X %02X %02X %02X %2X\n",content[0],
                                                content[1],
                                                content[2],
                                                content[3],
                                                content[4],
                                                content[5]
                                               );
        printf("UCS2:%04X %04X\n", ucs_data[0],ucs_data[1]);                                       
		return mcm_sms_send((mcm_sms_msg_format_t_v01)smsMode, phoneNumber,(unsigned char *)ucs_data, ucs_len*2);
	}
	
}

/*****************************************************************************
* Function Name : message_init
* Description   : 短信初始化函数
* Input         : None
* Output        : None
* Return        : -1:failed, 0:success
* Auther        : ygg
* Date          : 2018.03.10
*****************************************************************************/
int message_init()
{
	MCM_SMS_TEST_LOG("mcm sms main start");
	if(MCM_SMS_SUCCESS != mcm_sms_init())
	{
	    printf("mcm_sms_init fail\n");
	    return MCM_SMS_ERROR;
	}
	if(MCM_SMS_SUCCESS != mcm_sms_event_register())
	{
	    printf("mcm_sms_event_register fail\n");
	    return MCM_SMS_ERROR;	    
	}
	return MCM_SMS_SUCCESS;
}



