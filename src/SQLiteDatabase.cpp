#include "SQLiteDatabase.h"


SqliteDatabase *pSqliteDatabase = new SqliteDatabase;



/*****************************************************************************
* Function Name : ~SqliteDatabase
* Description   : 析构函数
* Input			: None
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
SqliteDatabase::~SqliteDatabase()
{
	sqlite3_close(db);
}

/*****************************************************************************
* Function Name : SqliteDatabase
* Description   : 构造函数
* Input			: None
* Output        : None
* Return        : None
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
SqliteDatabase::SqliteDatabase()
{
	initDatabase();
}

#if 0
SqliteDatabase::SqliteDatabase()
{
	/*const char *createTable = "create table ReportData(
						  ID INTEGER primary key autoincrement,
						  time sqlite3_int64,
						  data BLOB,
						  dataLength INTEGER,
						  toServer INTEGER)"; 

	const char *createTable = "create table ReportData(id sqlite3_int64 primary key autoincrement,
									data blob,
									dataLength INTEGER,
									toServer char
									times INTEGER)";	*/
	int ret;
    char *pErrMsg;
	char *sql = NULL;
	
	//open database
	if ((ret = sqlite3_open(SQLITE_DATABASE, &db)) != SQLITE_OK)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
	}
	printf("open database successfully!\n");
	
	//create table
	sql = sqlite3_mprintf("create table %s(id INTEGER primary key, data blob, dataLen INTEGER, times INTEGER)", TABLE_NAME);
	printf("sql:%s\n", sql);
	
	if ((ret = sqlite3_exec(db,sql,NULL,NULL,&pErrMsg)) != SQLITE_OK)
	{
		fprintf(stderr, "create table error: %s\n", pErrMsg);
	}
	printf("create table ok!\n");

	//test();
}
#endif
#if 1

/*****************************************************************************
* Function Name : initDatabase
* Description   : 数据库初始化
* Input			: None
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int SqliteDatabase::initDatabase()
{
	/*******************************************************
	 * judgment the data whether store in sd or ram        *
	 * flag: default 0 store in flash, 1 store in emmc     *
	 *******************************************************/
	int ret;
    char *pErrMsg;
	char *sql = NULL;
	int flag;
	FILE *fp;
	char buff[10];
	
	memset(buff, 0, sizeof(buff));
	
	fp = popen("ls /dev | grep mmcblk0p1 |grep -v \"grep\" |wc -l","r");
	if(fread(buff, sizeof(char), sizeof(buff), fp) > 0)
		printf("buff:%s\n",buff);
	else
	{
	    pclose(fp);
		return -1;
    }
	pclose(fp);

	flag = atoi(buff);
	printf("flag:%d\n", flag);

	if(flag == 0)
	{
		//open database
		if ((ret = sqlite3_open("/data/database.db", &db)) != SQLITE_OK)
		{
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
		}
		printf("open database successfully!\n");
		
		//create table
		sql = sqlite3_mprintf("create table %s(id INTEGER primary key, data blob, dataLen INTEGER, times INTEGER, dev INTEGER, isSend INTEGER)", TABLE_NAME);
		printf("sql:%s\n", sql);
		
		if ((ret = sqlite3_exec(db,sql,NULL,NULL,&pErrMsg)) != SQLITE_OK)
		{
			fprintf(stderr, "create table error: %s\n", pErrMsg);
		}
		printf("create table ok!\n");
		
		sql = sqlite3_mprintf("create table %s(id INTEGER primary key, data blob, dataLen INTEGER, times INTEGER)", ACP_TABLE);
		printf("sql:%s\n", sql);
		
		if ((ret = sqlite3_exec(db,sql,NULL,NULL,&pErrMsg)) != SQLITE_OK)
		{
			fprintf(stderr, "create table error: %s\n", pErrMsg);
		}
		printf("create table ok!\n");
	}
	else
	{
		//open database
		if ((ret = sqlite3_open(SQLITE_DATABASE, &db)) != SQLITE_OK)
		{
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
		}
		printf("open database successfully!\n");
		
		//create table
		sql = sqlite3_mprintf("create table %s(id INTEGER primary key, data blob, dataLen INTEGER, times INTEGER, dev INTEGER, isSend INTEGER)", TABLE_NAME);
		printf("sql:%s\n", sql);
		
		if ((ret = sqlite3_exec(db,sql,NULL,NULL,&pErrMsg)) != SQLITE_OK)
		{
			fprintf(stderr, "create table error: %s\n", pErrMsg);
		}
		printf("create table ok!\n");
		
		sql = sqlite3_mprintf("create table %s(id INTEGER primary key, data blob, dataLen INTEGER, times INTEGER)", ACP_TABLE);
		printf("sql:%s\n", sql);
		
		if ((ret = sqlite3_exec(db,sql,NULL,NULL,&pErrMsg)) != SQLITE_OK)
		{
			fprintf(stderr, "create table error: %s\n", pErrMsg);
		}
		printf("create table ok!\n");
	}

	return 0;
}
#endif

/*****************************************************************************
* Function Name : insertStandardProData
* Description   : 插入数据
* Input			: unsigned char *pData
*                 int len
*                 int times
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int SqliteDatabase::insertStandardProData(int id, unsigned char *pData, int len, int times, unsigned int dev, int IsSend)
{
	int ret;
	sqlite3_stmt *stmt;
	char *sql = sqlite3_mprintf("insert into %s values(%d,?, %d, %d, %d, %d)", TABLE_NAME, id, len, times, dev, IsSend);
	//printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "insert error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	sqlite3_bind_blob(stmt,1, pData, len, NULL);

	//execute
	ret = sqlite3_step(stmt);
	if(ret == SQLITE_ROW)
		printf("sqlite3_step ok!\n");
	
	//free memory
	sqlite3_finalize(stmt);

	return 0;
}

/*****************************************************************************
* Function Name : queryCount
* Description   : 查询个数
* Input			: int flag
*                 unsigned int id
* Output        : None
* Return        : totalNum
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
unsigned int SqliteDatabase::queryCount(int flag, unsigned int id)
{
	int ret;
	unsigned int totalNum;
	char *sql = NULL;
	sqlite3_stmt *stmt;

	if(flag == 0)
		sql = sqlite3_mprintf("select count(*) from %s", TABLE_NAME);
	else if(flag == 1)
		sql = sqlite3_mprintf("select count(*) from %s where id < %ld", TABLE_NAME, id);
	else
		sql = sqlite3_mprintf("select count(*) from %s where id > %ld", TABLE_NAME, id);
	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "select error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	//execute
	ret = sqlite3_step(stmt);
	if(ret == SQLITE_ROW)
		printf("sqlite3_step ok!\n");

	totalNum = sqlite3_column_int(stmt, 0);
	printf("totalNum:%d\n", totalNum);
	
	sqlite3_finalize(stmt);

	return totalNum;
}

/*****************************************************************************
* Function Name : queryID
* Description   : 查询ID
* Input			: int times
* Output        : None
* Return        : id
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int SqliteDatabase::queryID(int times)
{
	int ret, id;
	sqlite3_stmt *stmt;

	char *sql = sqlite3_mprintf("select id from %s where times = %d", TABLE_NAME, times);
//	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "select error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	//execute
	ret = sqlite3_step(stmt);
//	if(ret == SQLITE_ROW)
//		printf("sqlite3_step ok!\n");

	id = sqlite3_column_int(stmt, 0);
//	printf("id:%d\n", id);
	
	sqlite3_finalize(stmt);

	return id;
}

int SqliteDatabase::queryUnsendID()
{
	int ret, id;
	sqlite3_stmt *stmt;

	char *sql = sqlite3_mprintf("select max(id) from %s where IsSend = 0", TABLE_NAME);
//	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "select error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	//execute
	ret = sqlite3_step(stmt);
//	if(ret == SQLITE_ROW)
//		printf("sqlite3_step ok!\n");

	id = sqlite3_column_int(stmt, 0);
//	printf("id:%d\n", id);
	
	sqlite3_finalize(stmt);

	return id;
}


/*****************************************************************************
* Function Name : queryStandardProMaxID
* Description   : 查询最大ID
* Input			: None
* Output        : None
* Return        : id
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
unsigned int SqliteDatabase::queryStandardProMaxID(int& id, unsigned int& dev)
{
	int ret;
	sqlite3_stmt *stmt;

	char *sql = sqlite3_mprintf("select max(id) from %s where dev = (select max(dev) from %s)", TABLE_NAME, TABLE_NAME);
	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "select error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	//execute
	ret = sqlite3_step(stmt);
	if(ret == SQLITE_ROW)
		printf("sqlite3_step ok!\n");

	id = sqlite3_column_int(stmt, 0);
	printf("id:%d\n", id);
	dev = sqlite3_column_int(stmt, 1);
	printf("id:%d\n", dev);
	
	sqlite3_finalize(stmt);

	return id;
}

unsigned int SqliteDatabase::OnlyqueryMaxID(int& id)
{
	int ret;
	sqlite3_stmt *stmt;

	char *sql = sqlite3_mprintf("select max(id) from %s", TABLE_NAME);
//	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "select error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	//execute
	ret = sqlite3_step(stmt);
//	if(ret == SQLITE_ROW)
//		printf("sqlite3_step ok!\n");

	id = sqlite3_column_int(stmt, 0);
//	printf("id:%d\n", id);
	
	sqlite3_finalize(stmt);

	return id;
}

/*****************************************************************************
* Function Name : queryMaxTimes
* Description   : 查询最大次数
* Input			: None
* Output        : None
* Return        : id
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int SqliteDatabase::queryMaxTimes()
{
	int ret, id;
	sqlite3_stmt *stmt;

	char *sql = sqlite3_mprintf("select max(times) from %s", TABLE_NAME);
//	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "select error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	//execute
	ret = sqlite3_step(stmt);
//	if(ret == SQLITE_ROW)
//		printf("sqlite3_step ok!\n");

	id = sqlite3_column_int(stmt, 0);
//	printf("id:%d\n", id);
	
	sqlite3_finalize(stmt);

	return id;
}

int SqliteDatabase::querydev(unsigned int& id, int &dev)
{
	int ret;
	sqlite3_stmt *stmt;

	char *sql = sqlite3_mprintf("select dev from %s where id = %d", TABLE_NAME, id);
	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "select error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	//execute
	ret = sqlite3_step(stmt);
	if(ret == SQLITE_ROW)
		printf("sqlite3_step ok!\n");

	dev = sqlite3_column_int(stmt, 0);
	printf("dev:%d\n", dev);
	
	sqlite3_finalize(stmt);

	return dev;
}

/*****************************************************************************
* Function Name : queryMaxTimesID
* Description   : 查询最大次数的ID
* Input			: None
* Output        : None
* Return        : id
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
unsigned int SqliteDatabase::queryMaxTimesID()
{
	int ret;
	unsigned int id;
	sqlite3_stmt *stmt;

	char *sql = sqlite3_mprintf("select id from %s where times = (select max(times) from %s)", TABLE_NAME, TABLE_NAME);
//	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "select error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	//execute
	ret = sqlite3_step(stmt);
//	if(ret == SQLITE_ROW)
//		printf("sqlite3_step ok!\n");

	id = sqlite3_column_int(stmt, 0);
//	printf("id:%d\n", id);
	
	sqlite3_finalize(stmt);

	return id;
}

int SqliteDatabase::updatetimes(int id, int times)
{
	int ret;
	sqlite3_stmt *stmt;
	char *sql = sqlite3_mprintf("update %s set times = %d where id = %d", TABLE_NAME,times, id);
	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "insert error: %s\n", sqlite3_errmsg(db));
		return -1;
	}

	//execute
	ret = sqlite3_step(stmt);
	if(ret == SQLITE_ROW)
		printf("sqlite3_step ok!\n");
	
	//free memory
	sqlite3_finalize(stmt);

	return 0;
}

int SqliteDatabase::updateisSend(int isSend, int id)
{
	int ret;
	sqlite3_stmt *stmt;
	char *sql = sqlite3_mprintf("update %s set isSend = %d where id = %d", TABLE_NAME,isSend, id);
	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "insert error: %s\n", sqlite3_errmsg(db));
		return -1;
	}

	//execute
	ret = sqlite3_step(stmt);
	if(ret == SQLITE_ROW)
		printf("sqlite3_step ok!\n");
	
	//free memory
	sqlite3_finalize(stmt);

	return 0;
}

/*****************************************************************************
* Function Name : readStandardProData
* Description   : 读取数据
* Input			: unsigned char *pBuff,
                  int *len,
                  int id
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int SqliteDatabase::readStandardProData(unsigned char *pBuff, int *len, int id)
{
	int i, ret;
	unsigned char data[1024];
	sqlite3_stmt *stmt;
	
	char *sql = sqlite3_mprintf("select data from %s where id = %d and isSend = 0", TABLE_NAME, id);
	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, -1, &stmt, 0)) != SQLITE_OK) //strlen(sql)
	{
		fprintf(stderr, "insert error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	//execute
	if((ret = sqlite3_step(stmt)) == SQLITE_ROW);
//		printf("sqlite3_step ok!\n");

	int count = sqlite3_column_count(stmt);
//	printf("count:%d\n", count);

	*len = sqlite3_column_bytes(stmt, 0);
	printf("len:%d\n",*len);
	if(0 == *len)
	{
		sqlite3_finalize(stmt);
		return -1;
	}
		
	const void *content = (char*)sqlite3_column_blob(stmt, 0);

	memset(data, 0, sizeof(data));
	memcpy(data, content, *len);
	memcpy(pBuff, content, *len);

//	for(i = 0; i<*len; i++)
//		printf("%02x ",data[i]);
//	printf("\n");

	//free memory
	sqlite3_finalize(stmt);

	return 0;
}

/*****************************************************************************
* Function Name : deleteData
* Description   : 删除数据
* Input			: int id
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int SqliteDatabase::deleteData(int id)
{
	int ret;
	char *pErrMsg;
	char *sql = NULL;

	//0: delete all data;
	if(id == 0)
		sql = sqlite3_mprintf("delete from %s", TABLE_NAME);
	else

		sql = sqlite3_mprintf("delete from %s where id = %d", TABLE_NAME, id);
	printf("%s\n", sql);

	ret = sqlite3_exec(db, sql, 0, 0, &pErrMsg);
	if(ret != SQLITE_OK)
	{
		printf("sqlite3_exec_delete error:%s\n",pErrMsg);
		return -1;
	}
	else
		printf("delete data successfully!\n");
	
	return 0;
}


/*****************************************************************************
* Function Name : insertData
* Description   : 插入数据
* Input			: unsigned char *pData
*                 int len
*                 int times
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int SqliteDatabase::insertData(unsigned char *pData, int len, int times)
{
	int ret;
	sqlite3_stmt *stmt;
	char *sql = sqlite3_mprintf("insert into %s values(NULL,?, %d, %d)", ACP_TABLE, len, times);
//	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "insert error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	sqlite3_bind_blob(stmt,1, pData, len, NULL);

	//execute
	ret = sqlite3_step(stmt);
	if(ret == SQLITE_ROW)
		printf("sqlite3_step ok!\n");
	
	//free memory
	sqlite3_finalize(stmt);

	return 0;
}

/*****************************************************************************
* Function Name : queryMaxID
* Description   : 查询最大ID
* Input			: None
* Output        : None
* Return        : id
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
unsigned int SqliteDatabase::queryMaxID()
{
	int ret;
	unsigned int id;
	sqlite3_stmt *stmt;

	char *sql = sqlite3_mprintf("select max(id) from %s", ACP_TABLE);
//	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, strlen(sql), &stmt, 0)) != SQLITE_OK)
	{
		fprintf(stderr, "select error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	//execute
	ret = sqlite3_step(stmt);
//	if(ret == SQLITE_ROW)
//		printf("sqlite3_step ok!\n");

	id = sqlite3_column_int(stmt, 0);
//	printf("id:%d\n", id);
	
	sqlite3_finalize(stmt);

	return id;
}

/*****************************************************************************
* Function Name : readData
* Description   : 读取数据
* Input			: unsigned char *pBuff,
                  int *len,
                  int id
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : ygg
* Date          : 2018.01.18
*****************************************************************************/
int SqliteDatabase::readData(unsigned char *pBuff, int *len, int id)
{
	int i, ret;
	unsigned char data[1024];
	sqlite3_stmt *stmt;
	
	char *sql = sqlite3_mprintf("select data from %s where id = %d", ACP_TABLE, id);
//	printf("%s\n", sql);

	if((ret = sqlite3_prepare(db, sql, -1, &stmt, 0)) != SQLITE_OK) //strlen(sql)
	{
		fprintf(stderr, "insert error: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	//execute
	if((ret = sqlite3_step(stmt)) == SQLITE_ROW);
//		printf("sqlite3_step ok!\n");

	int count = sqlite3_column_count(stmt);
//	printf("count:%d\n", count);

	*len = sqlite3_column_bytes(stmt, 0);
//	printf("len:%d\n",*len);
	
	const void *content = (char*)sqlite3_column_blob(stmt, 0);

	memset(data, 0, sizeof(data));
	memcpy(data, content, *len);
	memcpy(pBuff, content, *len);

//	for(i = 0; i<*len; i++)
//		printf("%02x ",data[i]);
//	printf("\n");

	//free memory
	sqlite3_finalize(stmt);

	return 0;
}
/*
int SqliteDatabase::test()
{
	int i, len;

	unsigned char a[11]={0x0a,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a};
	unsigned char b[11]={0x0b,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a};
	unsigned char c[11]={0x02,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a};
	unsigned char d[11]={0x03,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a};

	deleteData(0);

	insertData(a, 11, 0);
	insertData(b, 11, 0);
	insertData(c, 11, 0);
	insertData(d, 11, 1);

	unsigned char *pBuff = (unsigned char *)malloc(1024);
	if(pBuff == NULL)
	{
		printf("malloc dataBuff error!");
		return -1;
	}

	memset(pBuff, 0, 1024);
	readData(pBuff, &len, 1);
	printf("len:%d, data1\n",len);
	for(i = 0; i<len; i++)
		printf("%02x ",pBuff[i]);
	printf("\n");

	memset(pBuff, 0, 1024);
	readData(pBuff, &len, 2);
	printf("len:%d, data2\n",len);
	for(i = 0; i<len; i++)
		printf("%02x ",pBuff[i]);
	printf("\n");

	memset(pBuff, 0, 1024);
	readData(pBuff, &len, 3);
	printf("len:%d, data3\n",len);
	for(i = 0; i<len; i++)
		printf("%02x ",pBuff[i]);
	printf("\n");

	memset(pBuff, 0, 1024);
	readData(pBuff, &len, 4);
	printf("len:%d, data4\n",len);
	for(i = 0; i<len; i++)
		printf("%02x ",pBuff[i]);
	printf("\n");

	int id = queryMaxID();
	printf("max id:%d\n",id);

	unsigned int count = queryCount(0, 0);
	id = queryID(1);

	printf("count:%d,id:%d\n",count,id);

	deleteData(3);

	free(pBuff);
	pBuff = NULL;

	return 0;
}*/


