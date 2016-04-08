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
#include <dos.h>
#include "log_def.h"

#if INCLUDE_SYSLOG_ENABLE

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

/* 定义模块内部的全局变量 */

/* 日志方式列表 */
static DOS_LOG_MODULE_NODE_ST       g_astLogModList[] = {
#if INCLUDE_SYSLOG_CONSOLE
    {LOG_MOD_CONSOLE,    log_console_reg,        NULL},
#endif
#if INCLUDE_SYSLOG_CLI
    {LOG_MOD_CLI,        log_cli_reg,            NULL},
#endif
#if INCLUDE_SYSLOG_DB
    {LOG_MOD_DB,         log_db_reg,             NULL},
#endif

    {LOG_MOD_BUTT,       NULL,             NULL},
};

/* 日志任务是否在等待退出 */
static S32               g_lLogWaitingExit  = 1;

/* 日志模块是否初始化完成 */
static S32               g_lLogInitFinished = 0;

/* 日志缓存链表 */
static DLL_S             g_stLogCacheList;

/* 日志任务控 */
static pthread_mutex_t   g_mutexLogTask     = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t    g_condLogTask      = PTHREAD_COND_INITIALIZER;
static pthread_t         g_pthIDLogTask;


/* 本地函数申明 */
//static char * dos_log_get_time(time_t _time, S8 *sz_time, S32 i_len);

/**
 * 函数：S32 dos_log_set_cli_level(U32 ulLeval)
 * 功能：设置cli日志类型的记录级别
 * 参数：
 *      U32 ulLeval：日志级别
 * 返回值：成功返回0，失败返回－1
 */
DLLEXPORT S32 dos_log_set_cli_level(U32 ulLeval)
{
    U32 ulLogModule;

    if (ulLeval >= LOG_LEVEL_INVAILD)
    {
        DOS_ASSERT(0);
        return -1;
    }

    for (ulLogModule=0; ulLogModule<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulLogModule++)
    {
        if (LOG_MOD_CLI == g_astLogModList[ulLogModule].ulLogModule)
        {
            if (g_astLogModList[ulLogModule].pstLogModule
                && g_astLogModList[ulLogModule].pstLogModule->fnSetLevel)
            {
                g_astLogModList[ulLogModule].pstLogModule->fnSetLevel(ulLeval);

                break;
            }
        }
    }

    return 0;
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
    U32 ulLogModule;

    if (ulLeval >= LOG_LEVEL_INVAILD)
    {
        DOS_ASSERT(0);
        return -1;
    }

    for (ulLogModule=0; ulLogModule<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulLogModule++)
    {
        if (LOG_MOD_CONSOLE == g_astLogModList[ulLogModule].ulLogModule)
        {
            if (g_astLogModList[ulLogModule].pstLogModule
                && g_astLogModList[ulLogModule].pstLogModule->fnSetLevel)
            {
                g_astLogModList[ulLogModule].pstLogModule->fnSetLevel(ulLeval);

                break;
            }
        }
    }

    return 0;

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
    U32 ulLogModule;

    if (ulLeval >= LOG_LEVEL_INVAILD)
    {
        DOS_ASSERT(0);
        return -1;
    }

    for (ulLogModule=0; ulLogModule<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulLogModule++)
    {
        if (LOG_MOD_DB == g_astLogModList[ulLogModule].ulLogModule)
        {
            if (g_astLogModList[ulLogModule].pstLogModule
                && g_astLogModList[ulLogModule].pstLogModule->fnSetLevel)
            {
                g_astLogModList[ulLogModule].pstLogModule->fnSetLevel(ulLeval);

                break;
            }
        }
    }

    return 0;

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
    U32 ulIndex;
    U32 ulRet;

    g_lLogWaitingExit = 0;

    DLL_Init(&g_stLogCacheList);

    /* 先让所有模块注册上来 */
    for (ulIndex=0; ulIndex<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulIndex++)
    {
        if (g_astLogModList[ulIndex].fnReg)
        {
            g_astLogModList[ulIndex].fnReg(&g_astLogModList[ulIndex].pstLogModule);
        }
    }

    /* 初始化各个模块 */
    for (ulIndex=0; ulIndex<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulIndex++)
    {
        if (g_astLogModList[ulIndex].pstLogModule
            && g_astLogModList[ulIndex].pstLogModule->fnInit)
        {
            ulRet = g_astLogModList[ulIndex].pstLogModule->fnInit();

            if (DOS_SUCC != ulRet)
            {
                return DOS_FAIL;
            }
        }
    }


    /* 启动各个模块 */
    for (ulIndex=0; ulIndex<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulIndex++)
    {
        if (g_astLogModList[ulIndex].pstLogModule
            && g_astLogModList[ulIndex].pstLogModule->fnStart)
        {
            ulRet = g_astLogModList[ulIndex].pstLogModule->fnStart();

            if (DOS_SUCC != ulRet)
            {
                return DOS_FAIL;
            }
        }
    }

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
    U32                 ulIndex;
    struct timespec     stTimeout;
    LOG_DATA_NODE_ST    *pstLogData = NULL;
    DLL_NODE_S          *pstDLLNode = NULL;
    S8                  szTime[32] = { 0 };

    /* 置等待退出标志 */
    g_lLogWaitingExit = 0;

    while(1)
    {
        pthread_mutex_lock(&g_mutexLogTask);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condLogTask, &g_mutexLogTask, &stTimeout);
        pthread_mutex_unlock(&g_mutexLogTask);

        /* 顺序处理所有日志 */
        while(1)
        {
            if (0 == DLL_Count(&g_stLogCacheList))
            {
                break;
            }

            pthread_mutex_lock(&g_mutexLogTask);
            pstDLLNode = dll_fetch(&g_stLogCacheList);
            pthread_mutex_unlock(&g_mutexLogTask);
            if (DOS_ADDR_INVALID(pstDLLNode))
            {
                break;
            }

            pstLogData = (LOG_DATA_NODE_ST *)pstDLLNode;
            for (ulIndex=0; ulIndex<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulIndex++)
            {
                if (!g_astLogModList[ulIndex].pstLogModule)
                {
                    continue;
                }


                if (LOG_TYPE_OPTERATION != pstLogData->lType)
                {
                    if (g_astLogModList[ulIndex].pstLogModule->fnWriteRlog)
                    {
                        g_astLogModList[ulIndex].pstLogModule->fnWriteRlog(pstLogData->stTime
                                            , pstLogData->lType >= LOG_TYPE_INVAILD ? "" : g_pszLogType[pstLogData->lType]
                                            , pstLogData->lLevel >= LOG_LEVEL_INVAILD ? "" : g_pszLogLevel[pstLogData->lLevel]
                                            , pstLogData->pszMsg
                                            , pstLogData->lLevel);
                    }
                }
                else
                {
                    if (g_astLogModList[ulIndex].pstLogModule->fnWriteOlog)
                    {
                        g_astLogModList[ulIndex].pstLogModule->fnWriteOlog(dos_log_get_time(pstLogData->stTime, szTime, sizeof(szTime))
                                            , pstLogData->szOperator
                                            , pstLogData->szOperand
                                            , pstLogData->ulResult ? "SUCC" : "FAIL"
                                            , pstLogData->pszMsg);
                    }
                }
            }

            free(pstLogData->pszMsg);
            pstLogData->pszMsg = NULL;
            free(pstLogData);
            pstLogData = NULL;
            pstDLLNode = NULL;
        }

        /* 检查退出标志 */
        if (g_lLogWaitingExit)
        {
            dos_printf("%s", "Log task finished flag has been set.");
            break;
        }
    }


    /* 重置退出标志 */
    g_lLogWaitingExit = 0;

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
    g_lLogInitFinished = 0;

    dos_log(LOG_LEVEL_NOTIC, LOG_TYPE_RUNINFO, (S8 *)"log task will be stopped!");

    pthread_join(g_pthIDLogTask, NULL);
}

/**
 * 函数：void dos_log(S32 _iLevel, S32 _iType, S8 *_pszMsg)
 * Todo：日志纪录函数。该函数将日志加入日志缓存，并同志日志任务处理
 * 参数：
 * 返回值：
 * */
DLLEXPORT VOID dos_log(S32 _lLevel, S32 _lType, const S8 *_pszMsg)
{
    LOG_DATA_NODE_ST *pstLogData = NULL;
    DLL_NODE_S       *pstDLLNode = NULL;

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
        return;
    }

    /* 申请内存 */
    pstLogData->pszMsg = (S8 *)malloc(LOG_MAX_LENGTH);
    if (!pstLogData->pszMsg)
    {
        free(pstLogData);
        return;
    }

    /* 初始化 */
    pstLogData->stTime = time(NULL);
    pstLogData->lLevel = _lLevel;
    pstLogData->lType  = _lType;
    strncpy(pstLogData->pszMsg, _pszMsg, LOG_MAX_LENGTH);
    pstLogData->pszMsg[LOG_MAX_LENGTH - 1] = '\0';

    /* 加入队列 */
    pstDLLNode = (DLL_NODE_S *)pstLogData;
    pthread_mutex_lock(&g_mutexLogTask);
    DLL_Add(&g_stLogCacheList, pstDLLNode);
    pthread_cond_signal(&g_condLogTask);
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
DLLEXPORT VOID dos_olog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *_pszMsg)
{
    LOG_DATA_NODE_ST *pstLogData = NULL;
    DLL_NODE_S       *pstDLLNode = NULL;

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
        return;
    }

    /* 申请内存，四字节对齐 */
    pstLogData->pszMsg = (S8 *)malloc(LOG_MAX_LENGTH);
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
    strncpy(pstLogData->pszMsg, _pszMsg, LOG_MAX_LENGTH);
    pstLogData->pszMsg[LOG_MAX_LENGTH - 1] = '\0';

    /* 加入队列 */
    pstDLLNode = (DLL_NODE_S *)pstLogData;
    pthread_mutex_lock(&g_mutexLogTask);
    DLL_Add(&g_stLogCacheList, pstDLLNode);
    pthread_cond_signal(&g_condLogTask);
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
DLLEXPORT VOID dos_volog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *format, ...)
{
    va_list argptr;
    S8 szBuff[1024];

    va_start( argptr, format );
    vsnprintf( szBuff, sizeof(szBuff), format, argptr );
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
DLLEXPORT VOID dos_vlog(S32 _lLevel, S32 _lType, const S8 *format, ...)
{
    va_list argptr;
    char buf[1024];

    va_start( argptr, format );
    vsnprintf( buf, sizeof(buf), format, argptr );
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
S8 * dos_log_get_time(time_t _stTime, S8 *szTime, S32 lLen)
{
    struct tm *t        = NULL;
    struct tm stTime;

    t = dos_get_localtime_struct(_stTime, &stTime);

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

DLLEXPORT S32 dos_log_set_cli_level(U32 ulLeval)
{
    return 1;
}


#endif


