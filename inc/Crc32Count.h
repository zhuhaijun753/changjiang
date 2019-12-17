#ifndef _CRC32_COUNT_H_
#define _CRC32_COUNT_H_
#include <stdint.h>


#ifndef __int8_t_defined
# define __int8_t_defined
typedef signed char     int8_t;
typedef short int       int16_t;
typedef int             int32_t;
# if __WORDSIZE == 64
typedef long int        int64_t;
# else
__extension__
typedef long long int   int64_t;
# endif
#endif

/* Unsigned. */
typedef unsigned char       uint8_t;
typedef unsigned short int  uint16_t;
#ifndef __uint32_t_defined
typedef unsigned int        uint32_t;
# define __uint32_t_defined
#endif
#if __WORDSIZE == 64
typedef unsigned long int   uint64_t;
#else
__extension__
typedef unsigned long long int uint64_t;
#endif



extern uint32_t Crc32_Count(unsigned long inCrc32, const uint8_t *buf, int size);
extern int Crc32_CountFile(char *fileName, unsigned long *outCrc32);
extern int compareCrc32CheckCode(char *fileName, unsigned char *fileCrc32Checkcode);





#endif

