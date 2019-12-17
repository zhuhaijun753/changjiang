#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "libnts_crypt_parse.h"



int encrypt_key(unsigned char *root_key, int input_len, unsigned char *encrypted_key)
{
    printf("----------------Begin encrypt-----------------------\n");
/*
	int ret = 0;
	unsigned char *encrypted_key;
	encrypted_key = (unsigned char *)malloc(256);

	printf("root_key is %s\n",root_key);
    ret = nts_encrypt_key(root_key, 32, encrypted_key);
    if (ret < 0) {
        printf("error\n");
        free(encrypted_key);
        return ret;
    }
    printf("ret is %d\n",ret);
    printf("encrypted_key is %s\n",encrypted_key);
    int i = 0;
    printf("encrypted_key is:\n");
    for (i = 0;i < ret;i++) {
        printf("%02x",encrypted_key[i]);
    }
*/
	int retLen = -1;
	void *handle;
	char *perr = NULL;
	callback_encrypt_key cb_encrypt_key;

	handle = dlopen(LIB_NTS_PATH, RTLD_LAZY);
	if(handle == NULL)
	{
		printf("error:%s \n", dlerror());
		return -1;
	}
	dlerror();
	
	cb_encrypt_key = (callback_encrypt_key)dlsym(handle, "nts_encrypt_key");
	if(cb_encrypt_key == NULL)
	{
		printf("encrypt dlsym error:%s \n", dlerror());
		return -1;
	}
	perr = dlerror();
	if(perr != NULL)
	{
		printf("%s\n", perr);
		return -1;
	}
   	printf("load libnts_crypt.so succsee!\n");
	retLen = cb_encrypt_key(root_key, input_len, encrypted_key);

	printf("encrypted_key retLen is: %d\n",retLen);
    int i = 0;
    printf("encrypted_key is:\n");
    for (i = 0; i < retLen; i++) {
        printf("%02x",encrypted_key[i]);
    }
	printf("\n");	
	dlclose(handle);	
	return retLen;
}

int decrypt_key(unsigned char *encrypted_key, int input_len, unsigned char *root_key)
{
	printf("----------------Begin decrypt-----------------------\n");

	int retLen = -1;
	void *handle;
	char *perr = NULL;
	callback_decrypt_key cb_decrypt_key = NULL;
	
	handle = dlopen(LIB_NTS_PATH, RTLD_LAZY);
	if(handle == NULL)
	{
		printf("error:%s \n", dlerror());
		return -1;
	}
	cb_decrypt_key = dlsym(handle, "nts_decrypt_key");
	if(cb_decrypt_key == NULL)
	{
		printf("dlsym error:%s \n", dlerror());
		return -1;
	}
	perr = dlerror();
	if(perr != NULL)
	{
		printf("%s\n", perr);
		return -1;
	}
	printf("load libnts_crypt.so succsee!\n");
	retLen = cb_decrypt_key(encrypted_key, input_len, root_key);
	
	printf("decrypt_key retLen is: %d\n",retLen);
    int i = 0;
    printf("decrypt_key is:\n");
    for (i = 0; i < retLen; i++) {
        printf("%02x",root_key[i]);
    }
	printf("------------------------------------------------\n");
	dlclose(handle);	
	return retLen;
}

