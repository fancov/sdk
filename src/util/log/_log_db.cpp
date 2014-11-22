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
    sz_table_name[0] = '\0';
    blInited = 0;
    memset(&mysql, 0, sizeof(mysql));

    this->ulLogLevel = LOG_LEVEL_INFO;
}


CLogDB::~CLogDB()
{
    mysql_close(&mysql);
}


S32 CLogDB::log_init()
{
    char value = 1;
    U16 usDBPort;
    S8 szDBHost[MAX_DB_INFO_LEN] = {0, };
    S8 szDBUsername[MAX_DB_INFO_LEN] = {0, };
    S8 szDBPassword[MAX_DB_INFO_LEN] = {0, };
    S8 szDBName[MAX_DB_INFO_LEN] = {0, };


    if (config_get_mysql_host(szDBHost, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (config_get_mysql_user(szDBUsername, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (config_get_mysql_password(szDBPassword, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (config_get_mysql_dbname(szDBName, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    usDBPort = config_get_mysql_port();
    if (0 == usDBPort || U16_BUTT == usDBPort)
    {
        usDBPort = 3306;
    }

    mysql_init(&mysql);

    mysql_options(&mysql, MYSQL_OPT_RECONNECT, &value);

    if (!mysql_real_connect(&mysql, szDBHost, szDBUsername, szDBPassword, szDBName, 0, NULL, 0))
    {
        dos_printf( "Error connecting to database: %s\n", mysql_error(&mysql));
        return -1;
    }

    blInited = 1;

    return 0;
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
    S8 szQuery[1024];

    if (!blInited)
    {
        return;
    }

    if (_ulLevel > this->ulLogLevel)
    {
        return;
    }

    snprintf(szQuery, sizeof(szQuery), "INSERT INTO %s VALUES(NULL, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\");"
                , "dos_log"
                , _pszTime
                , _pszLevel
                , _pszType
                , "init"
                , dos_get_process_name()
                , _pszMsg);

    //printf("%s\n", szQuery);

    mysql_real_query(&mysql, szQuery, (unsigned int)strlen(szQuery));
}

VOID CLogDB::log_write(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg)
{
    S8 szQuery[1024];

    if (!blInited)
    {
        return;
    }

    snprintf(szQuery, sizeof(szQuery), "INSERT INTO %s VALUES(NULL, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\");"
                    , "dos_olog"
                    , _pszTime
                    , _pszOpterator
                    , dos_get_process_name()
                    , _pszOpterand
                    , _pszResult
                    , _pszMsg);

    //printf("%s", szQuery);

    mysql_real_query(&mysql, szQuery, (unsigned int)strlen(szQuery));

}

#endif

