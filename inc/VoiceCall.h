#ifndef _VOICE_CALL_H_
#define _VOICE_CALL_H_
#include "voice_service_v02.h"
#include "simcom_common.h"
#include "TBoxDataPool.h"
#include "LTEModuleAtCtrl.h"


typedef enum {
    VOICE_CALL_HANDUP = 1,
    VOICE_CALL_ANSWER,
    VOICE_CALL_HOLD,
    VOICE_END_ALL,
    VOICE_END_BG,
    VOICE_END_FG
}voice_call_option;

#define VOICE_QMI_TIMEOUT_VALUE 10000

typedef struct {
    uint8_t call_id;
    call_state_enum_v02 call_state;
    call_direction_enum_v02 direction;
    char phone_number[QMI_VOICE_NUMBER_MAX_V02];
}call_info_type;

typedef enum {
    PHONE_TYPE_E = 1,
    PHONE_TYPE_B,
    PHONE_TYPE_I,    
}phone_type;



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
extern int hangUp_answer(voice_call_option op, unsigned char callid);
/*****************************************************************************
* Function Name : voiceCall
* Description   : 拨打电话
* Input         : char *callingNumber
* Input         : phone_type phoneType:呼叫号码类型
* Output        : None
* Return        : -1:failed, 0:success
* Auther        : ygg
* Date          : 2018.03.10
*****************************************************************************/
extern int voiceCall(char *callingNumber, phone_type phoneType);
extern int voiceCall_init();
extern void voice_qmi_release(void);

extern int voice_mtcall_process(voice_call_option option, unsigned char call_id);
extern int voice_get_all_call_info(call_info_type *pcall_info_list);
extern int voice_get_call_info(uint8_t callid, call_info_type *pcall_info);


extern TBoxDataPool *dataPool;
extern LTEModuleAtCtrl *LteAtCtrl;
#endif

