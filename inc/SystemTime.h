#ifndef _SYSTEM_TIME_H_
#define _SYSTEM_TIME_H_
#include <unistd.h>
#include <sys/time.h>



extern struct tm* getSystem_real_Time();
extern struct tm* getSystemTime();
extern int setSystemTime(struct tm *pTm);



#endif

