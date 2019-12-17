#ifndef _SIM_DNS_RESOLV_H_
#define _SIM_DNS_RESOLV_H_

#include "simcom_common.h"
#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 0
#endif

#define T_A 1 //Ipv4 address
#define T_NS 2 //Nameserver
#define T_CNAME 5 // canonical name
#define T_SOA 6 /* start of authority zone */
#define T_PTR 12 /* domain name pointer */
#define T_MX 15 //Mail server

extern int query_ip_from_dns(unsigned char *host, char *pri_dns_ip, char *sec_dns_ip, char *ip);


#endif


