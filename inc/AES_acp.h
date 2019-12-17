#ifndef _AES_H_
#define _AES_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>


// #define the macros below to 1/0 to enable/disable the mode of operation.
//
// CBC enables AES encryption in CBC-mode of operation.
// ECB enables the basic ECB 16-byte block algorithm. Both can be enabled simultaneously.

// The #ifndef-guard allows it to be configured before #include'ing or at compile time.

#ifndef ECB
  #define ECB 1
#endif

#define AES128 1

static uint8_t getSBoxValue(uint8_t num);
static uint8_t getSBoxInvert(uint8_t num);
static void KeyExpansion(void);
static void AddRoundKey(uint8_t round);
static void SubBytes(void);
static void ShiftRows(void);
static uint8_t xtime(uint8_t x);
static void MixColumns(void);
static uint8_t Multiply(uint8_t x, uint8_t y);
static void InvMixColumns(void);
static void InvSubBytes(void);
static void InvShiftRows(void);
static void Cipher(void);
static void InvCipher(void);
uint16_t GetEncryptDataLen(uint16_t src_len, uint8_t* output);
extern uint16_t Encrypt_AES128Data(uint8_t* key, uint8_t* input, uint16_t inputlen, uint8_t* output, uint8_t AcpCryptFlag);
extern uint16_t Decrypt_AES128Data(uint8_t* key, uint8_t* input, uint16_t inputlen, uint8_t* output, uint8_t AcpCryptFlag);


#endif //_AES_H_
