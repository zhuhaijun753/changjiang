/**
 * \file
 * encryption_init.c initialize the ATECC608A
 * and how to operate it.
 *
 */


#ifndef ENCRYPTION_INIT_H
#define ENCRYPTION_INIT_H
#include "cryptoauthlib.h"


#ifdef __cplusplus
extern "C" {
#endif


#define cmdQ_SIZE    512

int processCmd(void);

/*volatile struct
{
    uint8_t m_getIdx;
    uint8_t m_putIdx;
    uint8_t m_entry[ cmdQ_SIZE ];
} cmdQ;*/

typedef void (*fp_menu_handler)(void);

typedef struct
{
    const char*     menu_cmd;
    const char*     menu_cmd_description;
    fp_menu_handler fp_handler;
}t_menu_info;


int run_tests(int test);

#define ATCA_CONFIG_SIZE            (128)                               //!< size of configuration zone

#if defined(ATCA_HAL_CUSTOM)
extern ATCAIfaceCfg g_cfg_atsha204a_custom;
extern ATCAIfaceCfg g_cfg_atecc108a_custom;
extern ATCAIfaceCfg g_cfg_atecc508a_custom;
extern ATCAIfaceCfg g_cfg_atecc608a_custom;
#endif


extern void write_rKey_testing(void);
extern void write_sKey_testing(void);
extern void lock_config(void);
extern void read_config(void);
extern void write_config(void);
//初始化加密IC
extern int encryption_init(int nFlagCfg_Encryption);
//写入Rootkey,可修改但不能读取 slot 1
extern int writeTsp_rootkey(uint8_t * data, uint16_t len);
//写入skey,可修改但不能读取 slot 0
extern int writeTsp_skey(void);
//加密接口0:Skey加密 1:Rootkey加密	
extern int encryptTbox_Data(uint8_t *plaintext, uint16_t plaintextLen, uint16_t keyslot, uint8_t *encryptTbox_Data);
//解密接口0:Skey解密 1:Rootkey解密
extern int decryptTbox_Data(uint8_t *plaintext, uint16_t keyslot, uint8_t *decrypted_data_out);

#ifdef __cplusplus
}
#endif


#endif

