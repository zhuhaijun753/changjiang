/*
*  Project: faw-aes
*  File:    nts_crypt.h
*  Purpose: Aes ctr crypt.include obfuscation.
*  Description: neusoft
*  Author:  wuyukun@neusoft.com
*  Date:    2017-07-28
*  Edit:
*  Copyright (C) Neusoft
*/

#ifndef __NTS_CRYPT_H_
#define __NTS_CRYPT_H_

#ifdef __cplusplus  
extern "C"  //C++  
{
#endif 


/*
* Function:             nts_encrypt_key 
* Description:          aes encrypt 
* Input:
*   @root_key:          the key data which want be encrypted,key length is 16 byte
*   @input_len:         the length of root_key,if use AES128,input_len should be 16
*   @encrypted_key:     encrypted buffer
* Return:
*   >0                  encrypt success,the value is output data's length
*  -1                   root_key data is wrong 
*  -2                   encrypt failed
* Others:               None
*/
int nts_encrypt_key(unsigned char *root_key, int input_len, unsigned char *encrypted_key);


/*
* Function:             nts_decrypt_key 
* Description:          aes decrypt 
* Input:
*   @encrypted_key:     the key data which want be decrypted
*   @input_len:         the length of encrypted_key
*   @root_key:          root key buffer
* Return:
*   >0                  decrypt success,the value is output date's length
*  -1                   decrypted_key data is wrong 
*  -2                   decrypt failed
* Others:               None
*/
int nts_decrypt_key(unsigned char *encrypted_key, int input_len, unsigned char *root_key);

#ifdef __cplusplus  
}
#endif  

#endif /* _NTS_CRYPT_H_ */
