#ifndef _WDS_CONTROL_H_
#define _WDS_CONTROL_H_
#include "qmi_client.h"
#include "wireless_data_service_v01.h"

#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 0
#endif

typedef unsigned char boolean;
typedef unsigned char uint8;

extern int wds_qmi_init(void);
extern int wds_GetAPNInfo(int profile_index, int* pdp_type, char* apn_str, char *username, char *password);
extern int wds_SetAPNInfo(int profile_index, int pdp_type, char* apn_str, char *username, char *password);
extern void wds_qmi_release(void);

#endif


