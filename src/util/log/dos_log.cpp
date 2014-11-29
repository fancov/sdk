/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_log.cpp
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现日志模块相关函数集合
 *     History:
 */

/* system header files */
#include <sys/types.h>
#include <pthread.h>

/* system header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>

/* dos header files */
#include <dos/dos_types.h>
#include <syscfg.h>
#include <dos/dos_def.h>
#include <dos/dos_debug.h>
#include <dos/dos_def.h>
#include <dos/dos_string.h>
#include <dos/dos_tmr.h>
#include <dos/dos_cli.h>
#include <dos/dos_log.h>
#include <dos/dos_string.h>

/* 操作对象缓存长度 */
#define MAX_OPERAND_LENGTH    32

/* 操作员缓存长度 */
#define MAX_OPERATOR_LENGTH   32

/* include private header file */
#include "_log_base.h"
#include "_log_console.h"
#include "_log_file.h"
#include "_log_db.h"
#include "_log_cli.h"

enum tagLogMod{
    LOG_MOD_CONSOLE,
    LOG_MOD_CLI,
    LOG_MOD_DB,

    LOG_MOD_BUTT
}LOG_MOD_LIST;

#if INCLUDE_SYSLOG_ENABLE

/* 模块内日志正在的形式 */
typedef struct tagLogDataNode
{
    time_t stTime;
    S32    lLevel;
    S32    lType;
    S8     *pszMsg;

    S8     szOperand[MAX_OPERAND_LENGTH];
    S8     szOperator[MAX_OPERATOR_LENGTH];
    U32    ulResult;

    struct tagLogDataNode *pstPrev;
    struct tagLogDataNode *pstNext;
}LOG_DATA_NODE_ST;


#define MAX_LOG_TYPE   4         /* log的种类个数 */
#define MAX_LOG_CACHE  128       /* 最大log缓存个数 */

/* 定义模块内部的全局变量 */
static CLog              *g_pstLogModList[MAX_LOG_TYPE] = {NULL}; /* 日志方式列表 */
static S32               g_lLogWaitingExit = 1;                   /* 等待退出标准 */
static S32               g_lLogInitFinished = 0;                  /* 初始化完成标志 */
static U32               g_ulLogCnt = 0;
static LOG_DATA_NODE_ST  *g_LogCacheList = NULL;
static pthread_mutex_t   g_mutexLogTask = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t    g_condLogTask = PTHREAD_COND_INITIALIZER;
static pthread_t         g_pthIDLogTask;

static const S8 *g_pszLogType[] =
{
    "RUNINFO",
    "WARNING",
    "SERVICE",
    "OPTERATION",
    ""
};

static const S8 *g_pszLogLevel[] =
{
    "EMERG",
    "ALERT",
    "CIRT",
    "ERROR",
    "WARNING",
    "NOTICE",
    "INFO",
    "DEBUG",
    ""
};


/* 本地函数申明 */
static char * dos_log_get_time(time_t _time, S8 *sz_time, S32 i_len);

/**
 * 函数：S32 dos_log_set_cli_level(U32 ulLeval)
 * 功能：设置cli日志类型的记录级别
 * 参数：
 *      U32 ulLeval：日志级别
 * 返回值：成功返回0，失败返回－1
 */
S32 dos_log_set_cli_level(U32 ulLeval)
{
    if (!g_pstLogModList[LOG_MOD_CLI])
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (ulLeval >= LOG_LEVEL_INVAILD)
    {
        DOS_ASSERT(0);
        return -1;
    }

    return g_pstLogModList[LOG_MOD_CLI]->log_set_level(ulLeval);
}

/**
 * 函数：S32 dos_log_set_console_level(U32 ulLeval)
 * 功能：设置cli日志类型的记录级别
 * 参数：
 *      U32 ulLeval：日志级别
 * 返回值：成功返回0，失败返回－1
 */
S32 dos_log_set_console_level(U32 ulLeval)
{
    if (!g_pstLogModList[LOG_MOD_CONSOLE])
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (ulLeval >= LOG_LEVEL_INVAILD)
    {
        DOS_ASSERT(0);
        return -1;
    }

    return g_pstLogModList[LOG_MOD_CONSOLE]->log_set_level(ulLeval);
}

/**
 * 函数：S32 dos_log_set_db_level(U32 ulLeval)
 * 功能：设置cli日志类型的记录级别
 * 参数：
 *      U32 ulLeval：日志级别
 * 返回值：成功返回0，失败返回－1
 */
S32 dos_log_set_db_level(U32 ulLeval)
{
    if (!g_pstLogModList[LOG_MOD_DB])
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (ulLeval >= LOG_LEVEL_INVAILD)
    {
        DOS_ASSERT(0);
        return -1;
    }

    return g_pstLogModList[LOG_MOD_DB]->log_set_level(ulLeval);
}

/**
 * 函数：S32 dos_log_init()
 * Todo：初始化日志模块
 * 参数：
 * 返回值：
 *  成功返回0，失败返回－1
 * */
S32 dos_log_init()
{
    g_lLogWaitingExit = 0;
    g_ulLogCnt = 0;

#if INCLUDE_SYSLOG_CONSOLE
    /* 初始化命令后打印 */
    g_pstLogModList[LOG_MOD_CONSOLE] = new CLogConsole();
    if (g_pstLogModList[LOG_MOD_CONSOLE]->log_init() < 0)
    {
        delete(g_pstLogModList[LOG_MOD_CONSOLE]);
        g_pstLogModList[LOG_MOD_CONSOLE] = NULL;
    }
#endif

#if INCLUDE_SYSLOG_CLI
    /* 初始化统一管理平台log */
    g_pstLogModList[LOG_MOD_CLI] = new CLogCli();
    if (g_pstLogModList[LOG_MOD_CLI]->log_init() < 0)
    {
        delete(g_pstLogModList[LOG_MOD_CLI]);
        g_pstLogModList[LOG_MOD_CLI] = NULL;
    }
#endif

#if INCLUDE_SYSLOG_DB
    /* 初始化数据库日志 */
    g_pstLogModList[LOG_MOD_DB] = new CLogDB();
    if (g_pstLogModList[LOG_MOD_DB]->log_init() < 0)
    {
        delete(g_pstLogModList[LOG_MOD_DB]);
        g_pstLogModList[LOG_MOD_DB] = NULL;
    }
#endif

    /* 初始化日志缓存 */
    g_LogCacheList = (LOG_DATA_NODE_ST *)malloc(sizeof(LOG_DATA_NODE_ST));
    if (!g_LogCacheList)
    {
        return -1;
    }
    memset(g_LogCacheList, 0, sizeof(LOG_DATA_NODE_ST));
    g_LogCacheList->pstNext = g_LogCacheList;
    g_LogCacheList->pstPrev = g_LogCacheList;

    g_lLogInitFinished = 1;

    return 0;
}

/**
 * 函数：void *dos_log_main_loop(void *_ptr)
 * Todo：日志模块线程函数
 * 参数：
 * 返回值：
 * */
void *dos_log_main_loop(void *_ptr)
{
    U32 i;
    LOG_DATA_NODE_ST *pstLogData = NULL;
    S8 szTime[32] = { 0 };

    /* 置等待退出标志 */
    pthread_mutex_lock(&g_mutexLogTask);
    g_lLogWaitingExit = 0;
    pthread_mutex_unlock(&g_mutexLogTask);

    while(1)
    {
        pthread_mutex_lock(&g_mutexLogTask);
        i=pthread_cond_wait(&g_condLogTask, &g_mutexLogTask);
        //printf("wait:%p, %p, ret:%d\n", g_LogCacheList->pstNext, g_LogCacheList, i);

        /* 顺序处理所有日志 */
        while(g_LogCacheList->pstNext != g_LogCacheList)
        {
            pstLogData = g_LogCacheList->pstNext;

            g_LogCacheList->pstNext = pstLogData->pstNext;
            pstLogData->pstNext->pstPrev = g_LogCacheList;

            pstLogData->pstNext = NULL;
            pstLogData->pstPrev = NULL;

            for (i=0; i<MAX_LOG_TYPE; i++)
            {
                if (g_pstLogModList[i])
                {
                    if (LOG_TYPE_OPTERATION != pstLogData->lType)
                    {
                        g_pstLogModList[i]->log_write(dos_log_get_time(pstLogData->stTime, szTime, sizeof(szTime))
                                                , pstLogData->lType >= LOG_TYPE_INVAILD ? "" : g_pszLogType[pstLogData->lType]
                                                , pstLogData->lLevel >= LOG_LEVEL_INVAILD ? "" : g_pszLogLevel[pstLogData->lLevel]
                                                , pstLogData->pszMsg
                                                , pstLogData->lLevel);
                    }
                    else
                    {
                        g_pstLogModList[i]->log_write(dos_log_get_time(pstLogData->stTime, szTime, sizeof(szTime))
                                                , pstLogData->szOperator
                                                , pstLogData->szOperand
                                                , pstLogData->ulResult ? "SUCC" : "FAIL"
                                                , pstLogData->pszMsg);
                    }
                }
            }

            //printf("%p, %p\n", log->msg, log);
            //printf("%s\n", log->msg);

            g_ulLogCnt--;
            //printf("acnt %d\n", g_uiLogCnt);
            //printf("flag:%s\n", g_LogCacheList->next != g_LogCacheList ? "true" : "false");

            free(pstLogData->pszMsg);
            pstLogData->pszMsg = NULL;
            free(pstLogData);
            pstLogData = NULL;
        }

        /* 检查退出标志 */
        if (g_lLogWaitingExit)
        {
            dos_printf("%s", "Log task finished flag has been set.");
            pthread_mutex_unlock(&g_mutexLogTask);
            break;
        }

        pthread_mutex_unlock(&g_mutexLogTask);
    }


    /* 重置退出标志 */
    pthread_mutex_lock(&g_mutexLogTask);
    g_lLogWaitingExit = 0;
    pthread_mutex_unlock(&g_mutexLogTask);

    dos_printf("%s", (S8 *)"Log task goodbye!");
    return NULL;
}

/**
 * 函数：S32 log_task_start()
 * Todo：启动日志模块
 * 参数：
 * 返回值：
 *  成功返回0，失败返回－1
 * */
S32 dos_log_start()
{
    S32 lResult = 0;

    lResult = pthread_create(&g_pthIDLogTask, NULL, dos_log_main_loop, NULL);
    if (lResult != 0)
    {
        dos_printf("%s", "Fail to create the log task.");
        return -1;
    }
/*
    i_result = pthread_join(pthIDLogTask, NULL);
    if (i_result != 0)
    {
        printf("Fail to set the log task into join mode.\n");
        return -1;
    }
*/
    return 0;
}

/**
 * 函数：VOID log_task_stop()
 * Todo：停止日志模块
 * 参数：
 * 返回值：
 * */
VOID dos_log_stop()
{
    pthread_mutex_lock(&g_mutexLogTask);
    g_lLogWaitingExit = 1;
    pthread_mutex_unlock(&g_mutexLogTask);

    dos_log(LOG_LEVEL_NOTIC, LOG_TYPE_RUNINFO, (S8 *)"log task will be stopped!");

    pthread_join(g_pthIDLogTask, NULL);
}

/**
 * 函数：void dos_log(S32 _iLevel, S32 _iType, S8 *_pszMsg)
 * Todo：日志纪录函数。该函数将日志加入日志缓存，并同志日志任务处理
 * 参数：
 * 返回值：
 * */
VOID dos_log(S32 _lLevel, S32 _lType, const S8 *_pszMsg)
{
    LOG_DATA_NODE_ST *pstLogData = NULL;
    S32 lMsgLen = 0;
    S32 lRet = 0;

    if (!g_lLogInitFinished)
    {
        dos_syslog(_lLevel, _pszMsg);
        printf("%s\r\n", _pszMsg);

        return;
    }

    if (!_pszMsg)
    {
        /* 不要断言，这里的断言可能造成死循环 */
        return;
    }

    if (_lType >= LOG_TYPE_INVAILD)
    {
        /* 不要断言，这里的断言可能造成死循环 */
        return;
    }

    if (_lLevel >= LOG_LEVEL_INVAILD)
    {
        /* 不要断言，这里的断言可能造成死循环 */
        return;
    }

    pstLogData = (LOG_DATA_NODE_ST *)malloc(sizeof(LOG_DATA_NODE_ST));
    if (!pstLogData)
    {
        dos_printf("%s", "Alloc memory fail.");
        return;
    }

    /* 申请内存，四字节对齐 */
    lMsgLen = ((dos_strlen(_pszMsg) + 4) & 0xFFFFFFFC);
    pstLogData->pszMsg = (S8 *)malloc(lMsgLen);
    if (!pstLogData->pszMsg)
    {
        free(pstLogData);
        return;
    }

    /* 初始化 */
    pstLogData->stTime = time(NULL);
    pstLogData->lLevel = _lLevel;
    pstLogData->lType  = _lType;
    memcpy(pstLogData->pszMsg, _pszMsg, strlen(_pszMsg));
    pstLogData->pszMsg[strlen(_pszMsg)] = '\0';

    /* 加入队列 */
    pthread_mutex_lock(&g_mutexLogTask);
    if (g_LogCacheList == g_LogCacheList->pstNext)
    {
        pstLogData->pstNext = g_LogCacheList;
        pstLogData->pstPrev = g_LogCacheList;
        g_LogCacheList->pstNext = pstLogData;
        g_LogCacheList->pstPrev = pstLogData;
    }
    else
    {
        pstLogData->pstNext = g_LogCacheList;
        pstLogData->pstPrev = g_LogCacheList->pstPrev;
        pstLogData->pstPrev->pstNext = pstLogData;
        g_LogCacheList->pstPrev = pstLogData;
    }
    g_ulLogCnt++;
    //dos_printf("cnt %d, %p %p, %p\n", g_ulLogCnt, pstLogData->pszMsg, pstLogData, pstLogData->pstNext);
    lRet = pthread_cond_signal(&g_condLogTask);
    if (lRet < 0)
    {
        //dos_printf("Send signal ret: %d\n", lRet);
    }
    pthread_mutex_unlock(&g_mutexLogTask);

    return;
}


/**
 * 函数：VOID dos_volog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *format, ...)
 * Todo：操作日志纪录函数。该函数将日志加入日志缓存，并同志日志任务处理
 * 参数：
 *      S32 _lLevel： 级别
 *      S8 *pszOpterator： 操作员
 *      S8 *pszOpterand：操作对象
 *      U32 ulResult : 操作结果
 * 返回值：
 * */
VOID dos_olog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *_pszMsg)
{
    LOG_DATA_NODE_ST *pstLogData = NULL;
    S32 lMsgLen = 0;
    S32 lRet = 0;

    if (!g_lLogInitFinished)
    {
        dos_syslog(_lLevel, _pszMsg);

        return;
    }

    if (!_pszMsg)
    {
        /* 不要断言，这里的断言可能造成死循环 */
        return;
    }

    if (_lLevel >= LOG_LEVEL_INVAILD)
    {
        /* 不要断言，这里的断言可能造成死循环 */
        return;
    }

    pstLogData = (LOG_DATA_NODE_ST *)malloc(sizeof(LOG_DATA_NODE_ST));
    if (!pstLogData)
    {
        dos_printf("%s", "Alloc memory fail.");
        return;
    }

    /* 申请内存，四字节对齐 */
    lMsgLen = ((dos_strlen(_pszMsg) + 4) & 0xFFFFFFFC);
    pstLogData->pszMsg = (S8 *)malloc(lMsgLen);
    if (!pstLogData->pszMsg)
    {
        free(pstLogData);
        return;
    }

    /* 初始化 */
    pstLogData->stTime = time(NULL);
    pstLogData->lLevel = _lLevel;
    pstLogData->lType  = LOG_TYPE_OPTERATION;
    pstLogData->ulResult = ulResult;
    if (pszOpterator && pszOpterator[0] != '\0')
    {
        dos_strncpy(pstLogData->szOperator, pszOpterator, sizeof(pstLogData->szOperator));
        pstLogData->szOperator[sizeof(pstLogData->szOperator) - 1] = '\0';
    }
    else
    {
        pstLogData->szOperator[0] = '\0';
    }

    if (pszOpterand && pszOpterand[0] != '\0')
    {
        dos_strncpy(pstLogData->szOperand, pszOpterand, sizeof(pstLogData->szOperand));
        pstLogData->szOperand[sizeof(pstLogData->szOperand) - 1] = '\0';
    }
    else
    {
        pstLogData->szOperator[0] = '\0';
    }
    memcpy(pstLogData->pszMsg, _pszMsg, dos_strlen(_pszMsg));
    pstLogData->pszMsg[dos_strlen(_pszMsg)] = '\0';

    /* 加入队列 */
    pthread_mutex_lock(&g_mutexLogTask);
    if (g_LogCacheList == g_LogCacheList->pstNext)
    {
        pstLogData->pstNext = g_LogCacheList;
        pstLogData->pstPrev = g_LogCacheList;
        g_LogCacheList->pstNext = pstLogData;
        g_LogCacheList->pstPrev = pstLogData;
    }
    else
    {
        pstLogData->pstNext = g_LogCacheList;
        pstLogData->pstPrev = g_LogCacheList->pstPrev;
        pstLogData->pstPrev->pstNext = pstLogData;
        g_LogCacheList->pstPrev = pstLogData;
    }
    g_ulLogCnt++;
    //dos_printf("cnt %d, %p %p, %p\n", g_ulLogCnt, pstLogData->pszMsg, pstLogData, pstLogData->pstNext);
    lRet = pthread_cond_signal(&g_condLogTask);
    if (lRet < 0)
    {
    	//dos_printf("Send signal ret: %d\n", lRet);
    }
    pthread_mutex_unlock(&g_mutexLogTask);

    return;
}

/**
 * 函数：VOID dos_volog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *format, ...)
 * Todo：操作日志纪录函数。该函数将日志加入日志缓存，并同志日志任务处理
 * 参数：
 *      S32 _lLevel： 级别
 *      S8 *pszOpterator： 操作员
 *      S8 *pszOpterand：操作对象
 *      U32 ulResult : 操作结果
 * 返回值：
 * */
VOID dos_volog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *format, ...)
{
    va_list argptr;
    S8 szBuff[1024];

    va_start( argptr, format );
    vsnprintf( szBuff, 511, format, argptr );
    va_end( argptr );
    szBuff[sizeof(szBuff) -1] = '\0';

    dos_olog(_lLevel, pszOpterator, pszOpterand, ulResult, szBuff);
}

/**
 * 函数：void dos_vlog(S32 _iLevel, S32 _iType, S8 *format, ...)
 * Todo：日志纪录函数。该函数将日志加入日志缓存，并同志日志任务处理
 * 参数：
 * 返回值：
 * */
VOID dos_vlog(S32 _lLevel, S32 _lType, const S8 *format, ...)
{
    va_list argptr;
    char buf[1024];

    va_start( argptr, format );
    vsnprintf( buf, 511, format, argptr );
    va_end( argptr );
    buf[sizeof(buf) -1] = '\0';

    dos_log(_lLevel, _lType, buf);
}


/**
 * 函数：static char * dos_log_get_time(time_t _stTime, S8 *szTime, S32 iLen)
 * Todo：时间格式化为字符串
 * 参数：
 *      time_t _stTime 时间戳
 *      S8 *szTime ：输出缓存buff
 *      S32 iLen   ：输出缓存长度
 * 返回值：
 *      返回输出缓存的首地址
 * */
static char * dos_log_get_time(time_t _stTime, S8 *szTime, S32 lLen)
{
    struct tm *t = NULL;

    t = localtime(&_stTime);
    snprintf(szTime, lLen, "%04d-%02d-%02d %02d:%02d:%02d"
            , t->tm_year + 1900
            , t->tm_mon + 1
            , t->tm_mday
            , t->tm_hour
            , t->tm_min
            , t->tm_sec);

    return szTime;
}


#else
VOID dos_log(S32 _lLevel, S32 _lType, S8 *_pszMsg)
{

}

VOID dos_vlog(S32 _lLevel, S32 _lType, S8 *format, ...)
{

}
#endif


