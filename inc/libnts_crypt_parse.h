#ifndef LIBNTS_CRYPT_PARSE_H
#define LIBNTS_CRYPT_PARSE_H
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "nts_crypt.h"


//#define LIB_NTS_PATH  "/data/libnts_crypt.so"
#define LIB_NTS_PATH  "/usr/lib/libnts_crypt.so"

typedef int (*callback_encrypt_key)(unsigned char *root_key, int input_len, unsigned char *encrypted_key);
typedef int (*callback_decrypt_key)(unsigned char *encrypted_key, int input_len, unsigned char *root_key);

extern int encrypt_key(unsigned char *root_key, int input_len, unsigned char *encrypted_key);
extern int decrypt_key(unsigned char *encrypted_key, int input_len, unsigned char *root_key);



#endif