#ifndef SQLITEDATABASE_H_
#define SQLITEDATABASE_H_
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include "common.h"



#define SQLITE_DATABASE     "/media/card/database.db"    //"./database.db"
//#define SQLITE_DATABASE     "/data/database.db"    //"./database.db"
#define TABLE_NAME          "ReportData"
#define ACP_TABLE          	"AcpData"


class SqliteDatabase
{
public:
	SqliteDatabase();
	~SqliteDatabase();
	int initDatabase();
	
	int insertStandardProData(int id, unsigned char *pData, int len, int times, unsigned int dev, int IsSend);
	/****************************************************************
	* Function Name: queryCount
	* Auther	   : yingaoguo
	* Date		   : 2017-08-4
	* Description  : query total num, flag = 0: query all data.
	*                flag = 1: query less than id total number.
	*                flag = 2: query greater than id total number.
	* Return	   : total number
	*****************************************************************/
	unsigned int queryCount(int flag, unsigned int id);
	int queryID(int times);
	int queryUnsendID();
	unsigned int queryStandardProMaxID(int& id, unsigned int& dev);
	int queryMaxTimes();
	int querydev(unsigned int& id, int& dev);
	unsigned int queryMaxTimesID();
	int updateisSend(int isSend, int id);
	int updatetimes(int id, int times);
	int readStandardProData(unsigned char *pBuff, int *len, int id);
	unsigned int OnlyqueryMaxID(int& id);
	/****************************************************************
	* Function Name: deleteData
	* Auther	   : yingaoguo
	* Date		   : 2017-08-4
	* Description  : delete data, if id = 0 then delete all data.
	* Return	   : 0
	*****************************************************************/
	int deleteData(int id);
	int test();
	int insertData(unsigned char *pData, int len, int times);
	unsigned int queryMaxID();
	int readData(unsigned char *pBuff, int *len, int id);
private:
	sqlite3 *db;

};

extern SqliteDatabase *pSqliteDatabase;


#endif

