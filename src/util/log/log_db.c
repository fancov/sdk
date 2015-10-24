/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  log_db.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现将日志输出到数据的功能
 *     History:
 */
#include <dos.h>
#include "log_def.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#if (INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_DB)

#define LOG_DB_DEFAULT_LEVEL LOG_LEVEL_NOTIC

/* 最大buffer长度，包括DLL链表节点头，包括SQL语句部分所以内容 */
#define LOG_DB_MAX_BUFF_LEN  1500

static pthread_t          m_pthDBLogTaskHandle;
static pthread_mutex_t    m_mutexDBLogQueue   = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t     m_condDBLogQueue    = PTHREAD_COND_INITIALIZER;
static DLL_S              m_stDBLogQueue;
static DOS_LOG_MODULE_ST  m_stDBLog;
static DB_HANDLE_ST       *m_pstDBLogHandle   = NULL;

extern char *strptime(const char *buf,const char *format, struct tm *tm);

static VOID *log_db_task(VOID *ptr)
{
    DLL_NODE_S  *pstNode;
    S8          *pszQuery;
    struct timespec         stTimeout;

    m_stDBLog.blIsRunning = DOS_TRUE;

    while (1)
    {
        pthread_mutex_lock(&m_mutexDBLogQueue);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&m_condDBLogQueue, &m_mutexDBLogQueue, &stTimeout);
        pthread_mutex_unlock(&m_mutexDBLogQueue);

        while (1)
        {
            if (DLL_Count(&m_stDBLogQueue) == 0)
            {
                break;
            }

            pthread_mutex_lock(&m_mutexDBLogQueue);
            pstNode = dll_fetch(&m_stDBLogQueue);
            pthread_mutex_unlock(&m_mutexDBLogQueue);

            if (DOS_ADDR_INVALID(pstNode))
            {
                break;
            }

            if (DOS_ADDR_INVALID(pstNode->pHandle))
            {
                free(pstNode);
                pstNode = NULL;
            }

            pszQuery = (S8 *)pstNode->pHandle;
            if ('\0' == pszQuery[0])
            {
                free(pstNode);
                pstNode = NULL;
            }

            db_query(m_pstDBLogHandle, pszQuery, NULL, NULL, NULL);

            /* pHandle和pstNode是同一块内存 */
            pstNode->pHandle = NULL;
            free(pstNode);
            pstNode = NULL;
        }

        if (m_stDBLog.blWaitingExit)
        {
            break;
        }
    }

    m_stDBLog.blIsRunning = DOS_FALSE;
    m_stDBLog.blWaitingExit = DOS_FALSE;

    return NULL;
}


U32 log_db_init()
{
    U16 usDBPort;
    S8  szDBHost[MAX_DB_INFO_LEN] = {0, };
    S8  szDBUsername[MAX_DB_INFO_LEN] = {0, };
    S8  szDBPassword[MAX_DB_INFO_LEN] = {0, };
    S8  szDBName[MAX_DB_INFO_LEN] = {0, };
    S8  szDBSockPath[MAX_DB_INFO_LEN] = {0, };

    DLL_Init(&m_stDBLogQueue);

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


    m_pstDBLogHandle = db_create(DB_TYPE_MYSQL);
    if (!m_pstDBLogHandle)
    {
        DOS_ASSERT(0);
        return -1;
    }

    dos_strncpy(m_pstDBLogHandle->szHost, szDBHost, sizeof(m_pstDBLogHandle->szHost));
    m_pstDBLogHandle->szHost[sizeof(m_pstDBLogHandle->szHost) - 1] = '\0';

    dos_strncpy(m_pstDBLogHandle->szUsername, szDBUsername, sizeof(m_pstDBLogHandle->szUsername));
    m_pstDBLogHandle->szUsername[sizeof(m_pstDBLogHandle->szUsername) - 1] = '\0';

    dos_strncpy(m_pstDBLogHandle->szPassword, szDBPassword, sizeof(m_pstDBLogHandle->szPassword));
    m_pstDBLogHandle->szPassword[sizeof(m_pstDBLogHandle->szPassword) - 1] = '\0';

    dos_strncpy(m_pstDBLogHandle->szDBName, szDBName, sizeof(m_pstDBLogHandle->szDBName));
    m_pstDBLogHandle->szDBName[sizeof(m_pstDBLogHandle->szDBName) - 1] = '\0';

    dos_strncpy(m_pstDBLogHandle->szSockPath, szDBSockPath, sizeof(m_pstDBLogHandle->szSockPath));
    m_pstDBLogHandle->szSockPath[sizeof(m_pstDBLogHandle->szSockPath) - 1] = '\0';

    m_pstDBLogHandle->usPort = usDBPort;

    if (db_open(m_pstDBLogHandle) < 0)
    {
        DOS_ASSERT(0);

        db_destroy(&m_pstDBLogHandle);
        m_pstDBLogHandle = NULL;
        return -1;
    }

    m_stDBLog.blInited      = DOS_TRUE;     /* 是否被初始化了 */
    return DOS_SUCC;

errno_proc:

    return DOS_FAIL;
}

U32 log_db_start()
{
    if (pthread_create(&m_pthDBLogTaskHandle, NULL, log_db_task, NULL) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 log_db_stop()
{
    m_stDBLog.blWaitingExit = DOS_TRUE;    /* 是否在等待退出 */
    return DOS_SUCC;
}


VOID log_db_write_rlog(time_t _stTime, const S8 *_pszType, const S8 *_pszLevel, const S8 *_pszMsg, U32 _ulLevel)
{
    U32         ulLen     = 0;
    S8          *pszQuery = NULL;
    DLL_NODE_S  *pstNode  = NULL;

    if (!m_stDBLog.blInited)
    {
        return;
    }

    if (!m_stDBLog.blIsRunning)
    {
        return;
    }

    if (!m_pstDBLogHandle
        || DB_STATE_CONNECTED != m_pstDBLogHandle->ulDBStatus)
    {
        return;
    }

    if (_ulLevel > m_stDBLog.ulCurrentLevel)
    {
        return;
    }

    /* 申请队列节点和数据所需的内存 */
    pszQuery = (S8 *)malloc(LOG_DB_MAX_BUFF_LEN);
    if (NULL == pszQuery)
    {
        return;
    }

    /* 将内存转化为链表节点类型 */
    pstNode = (DLL_NODE_S *)pszQuery;
    DLL_Init_Node(pstNode);

    /* 让query指向数据域 */
    pszQuery += sizeof(DLL_NODE_S);
    pstNode->pHandle = pszQuery;

    /* 填充数据 */
    ulLen = LOG_DB_MAX_BUFF_LEN - sizeof(DLL_NODE_S);
    dos_snprintf(pszQuery, ulLen, "INSERT INTO %s VALUES(NULL, %u, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")"
                , "tbl_log"
                , _stTime
                , _pszLevel
                , _pszType
                , "init"
                , dos_get_process_name()
                , _pszMsg);

    pstNode->pHandle = pszQuery;
    pthread_mutex_lock(&m_mutexDBLogQueue);
    DLL_Add(&m_stDBLogQueue, pstNode);
    pthread_cond_signal(&m_condDBLogQueue);
    pthread_mutex_unlock(&m_mutexDBLogQueue);

}

VOID log_db_write_olog(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg)
{
    U32         ulLen     = 0;
    S8          *pszQuery = NULL;
    DLL_NODE_S  *pstNode  = NULL;
    struct tm   tm;
    time_t      ulTime;

    if (!m_stDBLog.blInited)
    {
        return;
    }

    if (!m_stDBLog.blIsRunning)
    {
        return;
    }

    if (!m_pstDBLogHandle
        || DB_STATE_CONNECTED != m_pstDBLogHandle->ulDBStatus)
    {
        return;
    }

    /* 申请队列节点和数据所需的内存 */
    pszQuery = (S8 *)malloc(LOG_DB_MAX_BUFF_LEN);
    if (NULL == pszQuery)
    {
        return;
    }

    strptime(_pszTime, "%Y-%m-%d %H:%M:%S", &tm);
    ulTime = mktime(&tm);

    /* 将内存转化为链表节点类型 */
    pstNode = (DLL_NODE_S *)pszQuery;
    DLL_Init_Node(pstNode);

    /* 让query指向数据域 */
    pszQuery += sizeof(DLL_NODE_S);
    pstNode->pHandle = pszQuery;

    /* 填充数据 */
    ulLen = LOG_DB_MAX_BUFF_LEN - sizeof(DLL_NODE_S);
    dos_snprintf(pszQuery, ulLen, "INSERT INTO %s VALUES(NULL, %u, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")"
                    , "tbl_olog"
                    , ulTime
                    , _pszOpterator
                    , dos_get_process_name()
                    , _pszOpterand
                    , _pszResult
                    , _pszMsg);

    pstNode->pHandle = pszQuery;
    pthread_mutex_lock(&m_mutexDBLogQueue);
    DLL_Add(&m_stDBLogQueue, pstNode);
    pthread_cond_signal(&m_condDBLogQueue);
    pthread_mutex_unlock(&m_mutexDBLogQueue);

}

U32 log_db_set_level(U32 ulLevel)
{
    if (ulLevel >= LOG_LEVEL_INVAILD)
    {
        return DOS_FAIL;
    }

    m_stDBLog.ulCurrentLevel = ulLevel;

    return DOS_SUCC;
}

U32 log_db_reg(DOS_LOG_MODULE_ST **pstModuleCB)
{
    m_stDBLog.ulCurrentLevel = LOG_DB_DEFAULT_LEVEL;                /* 当前log级别 */

    m_stDBLog.fnInit      = log_db_init;            /* 模块初始化函数 */
    m_stDBLog.fnStart     = log_db_start;           /* 模块启动函数 */
    m_stDBLog.fnStop      = log_db_stop;            /* 模块停止函数 */
    m_stDBLog.fnWriteRlog = log_db_write_rlog;      /* 模块写运行日志 */
    m_stDBLog.fnWriteOlog = log_db_write_olog;      /* 模块写操作日志 */
    m_stDBLog.fnSetLevel  = log_db_set_level;       /* 模块写操作日志 */

    m_stDBLog.blInited      = DOS_FALSE;
    m_stDBLog.blWaitingExit = DOS_FALSE;
    m_stDBLog.blIsRunning   = DOS_FALSE;

    *pstModuleCB = &m_stDBLog;

    return DOS_SUCC;
}

#endif /* end if INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_DB */

#ifdef __cplusplus
}
#endif /* __cplusplus */


