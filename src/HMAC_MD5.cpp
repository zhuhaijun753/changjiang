#include "HMAC_MD5.h"
#include <fstream>

// 名称：MD5_memcpy
// 功能：输入输出字节拷贝
// 参数：output: 指向unsigned char类型输出缓冲区
// input:指向unsigned char类型输入缓冲区
// len:  输入数据长度(字节)
// 返回：无 
void HMAC_MD5::MD5_memcpy( unsigned char *output, unsigned char *input, unsigned int len )
{
	unsigned int i;
	for( i = 0; i < len; i++ )
	output[i] = input[i];
}
 
// 名称：MD5_memset
// 功能：对MD5运算缓冲区中填充数据
// 参数：output: 指向unsigned char类型缓冲区
// value:数据
// len:  填充大小
// 返回：无
void HMAC_MD5::MD5_memset( unsigned char *output, int value, unsigned int len )
{
	unsigned int i;
	for( i = 0; i < len; i++ )
		((char *)output)[i] = (char)value;
}
 
// 名称：Encode
// 功能：数据类型轮换(unsigned long int -> unsigned char)
// 参数：output: 指向unsigned char类型输出缓冲区
// input:指向unsigned long int类型输入缓冲区
// len:  输入数据长度(字节)
// 返回：无 
void HMAC_MD5::Encode( unsigned char *output, unsigned long int *input, unsigned int len )
{
	unsigned int i, j;
	for(i = 0, j = 0; j < len; i++, j += 4) 
	{
		output[j] = (unsigned char)(input[i] & 0xff);
		output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
		output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
		output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
	}
}
 
// 名称：Decode
// 功能：数据类型轮换(unsigned char -> unsigned long int)
// 参数：output: 指向unsigned long int类型输出缓冲区
// input:指向unsigned char类型输入缓冲区
// len:  输入数据长度(字节)
// 返回：无 
void HMAC_MD5::Decode( unsigned long int *output, unsigned char *input, unsigned int len )
{
	unsigned int i, j;
	for( i=0, j=0; j<len; i++, j+=4 )
		output[i] = ((unsigned long int)input[j]) | (((unsigned long int)input[j+1]) << 8) | 
			    (((unsigned long int)input[j+2]) << 16) | (((unsigned long int)input[j+3]) << 24);
}

 
// 名称：MD5Init
// 功能：初始链接变量赋值；初始化填充位
// 参数：指向MD5状态数据变量
// 返回：无
// 备注：填充位第1位为1,其余位为0 
void HMAC_MD5::MD5_Init( MD5_State *s )
{
	s->count[0] = s->count[1] = 0;
	//! 初始链接变量
	s->state[0] = 0x67452301;
	s->state[1] = 0xefcdab89;
	s->state[2] = 0x98badcfe;
	s->state[3] = 0x10325476;

	//! 初始填充位(目标形式: 0x80000000......，共计512位)
	MD5_memset( s->PADDING, 0, sizeof(s->PADDING) );
	*(s->PADDING)=0x80;
	//  s->PADDING = {
	// 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	// 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
}

 
// 名称：MD5Transform
// 功能：MD5 4轮运算
// 参数：state: 链接变量；block: 子明文分组
// 返回：无
// 备注：4轮共计64步运算 
void HMAC_MD5::MD5_Transform( unsigned long int state[4], unsigned char block[64] )
{
	unsigned long int a = state[0], b = state[1], c = state[2], d = state[3], x[16];
	
	Decode( x, block, 64 );
	
	//! 第1轮
	FF (a, b, c, d, x[0],  S11, 0xd76aa478); // 1
	FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); // 2
	FF (c, d, a, b, x[ 2], S13, 0x242070db); // 3
	FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); // 4
	FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); // 5
	FF (d, a, b, c, x[ 5], S12, 0x4787c62a); // 6
	FF (c, d, a, b, x[ 6], S13, 0xa8304613); // 7
	FF (b, c, d, a, x[ 7], S14, 0xfd469501); // 8
	FF (a, b, c, d, x[ 8], S11, 0x698098d8); // 9
	FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); // 10
	FF (c, d, a, b, x[10], S13, 0xffff5bb1); // 11
	FF (b, c, d, a, x[11], S14, 0x895cd7be); // 12
	FF (a, b, c, d, x[12], S11, 0x6b901122); // 13
	FF (d, a, b, c, x[13], S12, 0xfd987193); // 14
	FF (c, d, a, b, x[14], S13, 0xa679438e); // 15
	FF (b, c, d, a, x[15], S14, 0x49b40821); // 16

	//! 第2轮
	GG (a, b, c, d, x[ 1], S21, 0xf61e2562); // 17
	GG (d, a, b, c, x[ 6], S22, 0xc040b340); // 18
	GG (c, d, a, b, x[11], S23, 0x265e5a51); // 19
	GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); // 20
	GG (a, b, c, d, x[ 5], S21, 0xd62f105d); // 21
	GG (d, a, b, c, x[10], S22, 0x02441453); // 22
	GG (c, d, a, b, x[15], S23, 0xd8a1e681); // 23
	GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); // 24
	GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); // 25
	GG (d, a, b, c, x[14], S22, 0xc33707d6); // 26
	GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); // 27
	GG (b, c, d, a, x[ 8], S24, 0x455a14ed); // 28
	GG (a, b, c, d, x[13], S21, 0xa9e3e905); // 29
	GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); // 30
	GG (c, d, a, b, x[ 7], S23, 0x676f02d9); // 31
	GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); // 32

	//! 第3轮
	HH (a, b, c, d, x[ 5], S31, 0xfffa3942); // 33
	HH (d, a, b, c, x[ 8], S32, 0x8771f681); // 34
	HH (c, d, a, b, x[11], S33, 0x6d9d6122); // 35
	HH (b, c, d, a, x[14], S34, 0xfde5380c); // 36
	HH (a, b, c, d, x[ 1], S31, 0xa4beea44); // 37
	HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); // 38
	HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); // 39
	HH (b, c, d, a, x[10], S34, 0xbebfbc70); // 40
	HH (a, b, c, d, x[13], S31, 0x289b7ec6); // 41
	HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); // 42
	HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); // 43
	HH (b, c, d, a, x[ 6], S34, 0x04881d05); // 44
	HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); // 45
	HH (d, a, b, c, x[12], S32, 0xe6db99e5); // 46
	HH (c, d, a, b, x[15], S33, 0x1fa27cf8); // 47
	HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); // 48

	//! 第4轮
	II (a, b, c, d, x[ 0], S41, 0xf4292244); // 49
	II (d, a, b, c, x[ 7], S42, 0x432aff97); // 50
	II (c, d, a, b, x[14], S43, 0xab9423a7); // 51
	II (b, c, d, a, x[ 5], S44, 0xfc93a039); // 52
	II (a, b, c, d, x[12], S41, 0x655b59c3); // 53
	II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); // 54
	II (c, d, a, b, x[10], S43, 0xffeff47d); // 55
	II (b, c, d, a, x[ 1], S44, 0x85845dd1); // 56
	II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); // 57
	II (d, a, b, c, x[15], S42, 0xfe2ce6e0); // 58
	II (c, d, a, b, x[ 6], S43, 0xa3014314); // 59
	II (b, c, d, a, x[13], S44, 0x4e0811a1); // 60
	II (a, b, c, d, x[ 4], S41, 0xf7537e82); // 61
	II (d, a, b, c, x[11], S42, 0xbd3af235); // 62
	II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); // 63
	II (b, c, d, a, x[ 9], S44, 0xeb86d391); // 64

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	MD5_memset( (unsigned char*)x, 0, sizeof (x) );
}

 
// 名称：MD5_Update
// 功能：明文填充，明文分组，16个子明文分组
// 参数：指向SHA状态变量
// 返回：无 
void HMAC_MD5::MD5_Update( MD5_State *s, unsigned char *input, unsigned int inputLen )
{
	unsigned int i, index, partLen;
	//! 明文填充
	//! 计算字节数 mod 64
	index = (unsigned int)((s->count[0] >> 3) & 0x3F);

	//! 更新数据位数
	if( ( s->count[0] += ((unsigned long int)inputLen << 3) ) < ( (unsigned long int)inputLen << 3 ) )
	{
		s->count[1]++;
	}
	s->count[1] += ((unsigned long int)inputLen >> 29);

	partLen = 64 - index;
	//! MD5 4轮运算
	if (inputLen >= partLen)
	{
		MD5_memcpy( (unsigned char*)&s->buffer[index],  (unsigned char*)input, partLen );
		MD5_Transform( s->state, s->buffer );

		for( i = partLen; i + 63 < inputLen; i += 64 )
		{
			MD5_Transform( s->state, &input[i] );
		}
		index = 0;
	}
	else
	{
		i = 0;
	}
	MD5_memcpy ((unsigned char*)&s->buffer[index], (unsigned char*)&input[i], inputLen-i);
}

 
// 名称：MD5_Final
// 功能：MD5最后变换
// 参数：strContent:指向文件内容缓冲区; iLength:文件内容长度; output:摘要输出缓冲区
// 返回：无 
void HMAC_MD5::MD5_Final( MD5_State *s, unsigned char digest[16] )
{
	unsigned char bits[8];
	unsigned int index, padLen;

	Encode (bits, s->count, 8);

	//! 长度小于448位(mod 512),对明文进行填充
	index = (unsigned int)((s->count[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	MD5_Update( s, s->PADDING, padLen );

	MD5_Update( s, bits, 8);
	Encode( digest, s->state, 16 );

	MD5_memset ((unsigned char*)s, 0, sizeof (*s));
	MD5_Init( s );
}

//! 文件最大2M

 
 
// 名称：FileIn
// 功能：读取磁盘文件到内存
// 参数：strFile:文件名称；inBuff:指向文件内容缓冲区
// 返回：实际读取内容大小(字节) 
int HMAC_MD5::FileIn( const char *strFile, unsigned char *&inBuff )
{
	int iFileLen=0, iBuffLen=0;

	std::ifstream t;
	t.open(strFile);
	t.seekg(0,std::ios::end);
	iFileLen = t.tellg();
	t.seekg(0,std::ios::beg);
	t.read(inBuff,iFileLen);
	t.close();
 
	/*//! 打开密文文件
	CFile file( strFile, CFile::modeRead );
	iFileLen = ( int )file.GetLength();
	if( iFileLen>MAX_FILE )
	{
		printf( "文件长度不能大于 %dM,!\n", MAX_FILE/(1024*1024) );
		goto out;
	}
	iBuffLen = iFileLen;
	inBuff = new unsigned char[iBuffLen];
	if( !inBuff )
		goto out;
	ZeroMemory( inBuff, iBuffLen );	 
	file.Read( inBuff, iFileLen );
	file.Close();
	out:
		return iBuffLen;*/

		return iFileLen;
}
 
// 名称：usage
// 功能：帮助信息
// 参数：应用程序名称
// 返回：无 
void HMAC_MD5::Usage( const char *appname )
{
	printf( "\n\tusage: md5 文件\n" );
}
 
// 名称：CheckParse
// 功能：校验应用程序入口参数
// 参数：argc等于main主函数argc参数，argv指向main主函数argv参数
// 返回：若参数合法返回true，否则返回false
// 备注：简单的入口参数校验
 
bool HMAC_MD5::CheckParse( int argc, char** argv )
{
	if( argc != 2 )
	{
		Usage( argv[0] );
		return false;
	}
	return true;
}


// 名称：SHA_digest
// 功能：生成文件摘要
// 参数：strContent:指向文件内容缓冲区; iLength:文件内容长度; output:摘要输出缓冲区
// 返回：无
void HMAC_MD5::md5_digest( void const *strContent, unsigned int iLength, unsigned char output[16] )

{
	unsigned char *q = (unsigned char*)strContent;
	MD5_State s;

	MD5_Init( &s );
	MD5_Update( &s, q, iLength );
	MD5_Final( &s, output );
}


int HMAC_MD5::md5test( const char * testfile, unsigned char *output )
{
	int len , i;
	
	//unsigned char output[16]; //输出摘要
	unsigned char *inBuffer = (unsigned char *)malloc(MAX_FILE); //输入缓冲区

	len = FileIn(testfile, inBuffer);
	md5_digest(inBuffer, len, output);

	printf("Digest: ");
	for(i=0;i<16;i++)
	{
		if(output[i]<0x10)
		printf("%d",0);
		printf("%x\n",output[i]);
	}
	printf("\n");
	return 0;
}

#if 0
//主程序
int main( int argc, char **argv )
{
	int len , i ,j;
	char FileName[20];
	const char *ky = "TSP_Akey_SendTBox"; //初始密钥 TSP_Rand1_SendTBox TSP_Akey_SendTBox
	unsigned char output[16]; //输出摘要
	unsigned char *inBuffer = (unsigned char *)malloc(MAX_FILE); //输入缓冲区
	unsigned char *tempBuffer = (unsigned char *)malloc(MAX_FILE + 64); //第一次HASH的参数
	unsigned char Buffer2[80]; //第二次HASH
	unsigned char key[16];
	unsigned char ipad[64], opad[64];
	 
	inBuffer = (unsigned char *)("TSPVINAKEY");
	len = sizeof(inBuffer);
	 
	 
	if(strlen(ky) > 16 )
		md5_digest(ky, strlen(ky), key);
	else if(strlen(ky) < 16 )
	{
		i=0;
		while(ky[i]!='\0')
		{
			key[i]=ky[i];
			i++;
		}
		while(i<16)
		{
			key[i]=0x00;
			i++;
		}
	}
	else
	{
		for(i=0;i<16;i++)
		key[i]=ky[i];
	}
	 
	for(i=0;i<64;i++)
	{
		ipad[i]=0x36;
		opad[i]=0x5c;
	}
	 
	for(i=0;i<16;i++)
	{
		ipad[i]=key[i] ^ ipad[i];   //K ⊕ ipad
		opad[i]=key[i] ^ opad[i];   //K ⊕ opad
	}
	 
	for(i=0;i<64;i++)
		tempBuffer[i]=ipad[i];
	for(i=64;i<len+64;i++)
		tempBuffer[i]=inBuffer[i-64];
	 
	md5_digest(tempBuffer, len+64, output);
	 
	for(j=0; j < 64; j++)
		Buffer2[j] = opad[j];
	for(i = 64; i< 80; i++)
		Buffer2[i] = output[i-64];
	 
	md5_digest(Buffer2, 80, output);
	 
	 
	printf("Digest: ");
	for(i=0;i<16;i++)
	{
		if(output[i]<0x10)
		printf("%d",0);
		printf("%x",output[i]);
	}
	printf("\n");
	return 0;
}
#endif



