/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  _log_db.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现将日志输出到数据的功能
 *     History:
 */

#include "_log_db.h"

#if (INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_DB)

CLogDB::CLogDB()
{
    blInited = 0;
    pstDBHandle = NULL;

    this->ulLogLevel = LOG_LEVEL_DEBUG;
}


CLogDB::~CLogDB()
{
    if (pstDBHandle)
    {
        db_destroy(&pstDBHandle);
    }
}


S32 CLogDB::log_init()
{
    U16 usDBPort;
    S8  szDBHost[MAX_DB_INFO_LEN] = {0, };
    S8  szDBUsername[MAX_DB_INFO_LEN] = {0, };
    S8  szDBPassword[MAX_DB_INFO_LEN] = {0, };
    S8  szDBName[MAX_DB_INFO_LEN] = {0, };
	S8  szDBSockPath[MAX_DB_INFO_LEN] = {0, };

    if (config_get_db_host(szDBHost, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    if (config_get_db_user(szDBUsername, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    if (config_get_db_password(szDBPassword, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    usDBPort = config_get_db_port();
    if (0 == usDBPort || U16_BUTT == usDBPort)
    {
        usDBPort = 3306;
    }

    if (config_get_db_dbname(szDBName, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

	if (config_get_mysqlsock_path(szDBSockPath, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }


    pstDBHandle = db_create(DB_TYPE_MYSQL);
    if (!pstDBHandle)
    {
        DOS_ASSERT(0);
        return -1;
    }

    dos_strncpy(pstDBHandle->szHost, szDBHost, sizeof(pstDBHandle->szHost));
    pstDBHandle->szHost[sizeof(pstDBHandle->szHost) - 1] = '\0';

    dos_strncpy(pstDBHandle->szUsername, szDBUsername, sizeof(pstDBHandle->szUsername));
    pstDBHandle->szUsername[sizeof(pstDBHandle->szUsername) - 1] = '\0';

    dos_strncpy(pstDBHandle->szPassword, szDBPassword, sizeof(pstDBHandle->szPassword));
    pstDBHandle->szPassword[sizeof(pstDBHandle->szPassword) - 1] = '\0';

    dos_strncpy(pstDBHandle->szDBName, szDBName, sizeof(pstDBHandle->szDBName));
    pstDBHandle->szDBName[sizeof(pstDBHandle->szDBName) - 1] = '\0';

	dos_strncpy(pstDBHandle->szSockPath, szDBSockPath, sizeof(pstDBHandle->szSockPath));
    pstDBHandle->szSockPath[sizeof(pstDBHandle->szSockPath) - 1] = '\0';

    pstDBHandle->usPort = usDBPort;

    if (db_open(pstDBHandle) < 0)
    {
        DOS_ASSERT(0);

        db_destroy(&pstDBHandle);
        pstDBHandle = NULL;
        return -1;
    }

    blInited = 1;

    return 0;

errno_proc:

    return -1;
}

/**
 * 重载初始化函数，连接到mysql数据库，并保持长链接
 * 参数集：
 *    参数1：数据库服务器地址（ip）
 *    参数2：端口 （如果填写0，表示使用默认端口）
 *    参数3：数据库
 *    参数4：用户名
 *    参数5：密码
 * 返回值：
 *   成功返回0，失败返回－1
 */
S32 CLogDB::log_init(S32 argc, S8 **argv)
{
    return 0;
}

S32 CLogDB::log_set_level(U32 ulLevel)
{
    if (ulLevel >= LOG_LEVEL_INVAILD)
    {
        return -1;
    }

    this->ulLogLevel = ulLevel;

    return 0;
}

/**
 * 写日志函数
 */
void CLogDB::log_write(const S8 *_pszTime, const S8 *_pszType, const S8 *_pszLevel, const S8 *_pszMsg, U32 _ulLevel)
{
    U32 ulLen = 0;
	S8  *pszQuery = NULL;

    if (!blInited)
    {
        return;
    }

    if (!pstDBHandle
        || DB_STATE_CONNECTED != pstDBHandle->ulDBStatus)
    {
        return;
    }

    if (_ulLevel > this->ulLogLevel)
    {
        return;
    }

    ulLen = dos_strlen(_pszMsg) + EXTRA_LEN;
    pszQuery = new S8[ulLen];
	
	if (!pszQuery)
	{
		return;
	}

    dos_snprintf(pszQuery, ulLen, "INSERT INTO %s VALUES(NULL, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")"
                , "tbl_log"
                , _pszTime
                , _pszLevel
                , _pszType
                , "init"
                , dos_get_process_name()
                , _pszMsg);

    db_query(pstDBHandle, pszQuery, NULL, NULL, NULL);
	
	delete [] pszQuery;
}

VOID CLogDB::log_write(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg)
{
    S8  *pszQuery = NULL;
	U32 ulLen = 0;

    if (!blInited)
    {
        return;
    }

    if (!pstDBHandle
        || DB_STATE_CONNECTED != pstDBHandle->ulDBStatus)
    {
        return;
    }

	ulLen = dos_strlen(_pszMsg) + EXTRA_LEN;
	pszQuery = new S8[ulLen];

    dos_snprintf(pszQuery, sizeof(pszQuery), "INSERT INTO %s VALUES(NULL, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")"
                    , "tbl_olog"
                    , _pszTime
                    , _pszOpterator
                    , dos_get_process_name()
                    , _pszOpterand
                    , _pszResult
                    , _pszMsg);
    db_query(pstDBHandle, pszQuery, NULL, NULL, NULL);

    //printf("%s", szQuery);
    delete [] pszQuery;
}

#endif

