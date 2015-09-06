/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  log_cli.c
 *
 *  Created on: 2015年9月4日10:46:21
 *      Author: Larry
 *        Todo: 实现将日志输出到统一管理平台
 *     History:
 */

#include <dos.h>
#include "log_def.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#if (INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_CLI && INCLUDE_DEBUG_CLI)

#define LOG_CLI_DEFAULT_LEVEL LOG_LEVEL_INFO

static DOS_LOG_MODULE_ST m_stCliLog;

U32 log_cli_init()
{
    m_stCliLog.blInited      = DOS_TRUE;     /* 是否被初始化了 */
    return DOS_SUCC;
}

U32 log_cli_start()
{
     m_stCliLog.blWaitingExit = DOS_FALSE;    /* 是否在等待退出 */
     m_stCliLog.blIsRunning   = DOS_TRUE;     /* 是否在正常运行 */
    return DOS_SUCC;
}

U32 log_cli_stop()
{
    return DOS_SUCC;
}


VOID log_cli_write_rlog(const S8 *_pszTime, const S8 *_pszType, const S8 *_pszLevel, const S8 *_pszMsg, U32 _ulLevel)
{
    S8 szBuff[1024] = { 0 };
    U32 ulLength;

    if (!m_stCliLog.blInited || !m_stCliLog.blIsRunning)
    {
        return;
    }

    if (LOG_LEVEL_INVAILD == m_stCliLog.ulCurrentLevel
        || _ulLevel > m_stCliLog.ulCurrentLevel)
    {
        return;
    }

    /* snprintf返回处理的字节数，不包括 字符串结束符的 */
    ulLength = snprintf(szBuff, sizeof(szBuff), "%-20s[%-8s][%-10s][%-8s] %s\r\n"
                         , _pszTime, _pszLevel, dos_get_process_name(), _pszType, _pszMsg);
    if (ulLength < sizeof(szBuff))
    {
        szBuff[ulLength] = '\0';
        ulLength++;
    }
    else
    {
        szBuff[sizeof(szBuff) - 1] = '\0';
    }

    debug_cli_send_log(szBuff, ulLength);
}

VOID log_cli_write_olog(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg)
{
    S8 szBuff[1024] = { 0 };
    U32 ulLength;

    if (!m_stCliLog.blInited || !m_stCliLog.blIsRunning)
    {
        return;
    }

    /* snprintf返回处理的字节数，不包括 字符串结束符的 */
    ulLength = snprintf(szBuff, sizeof(szBuff)
                         , "%-20s[%-8s][%-10s][%-8s][%s][%s][%s]%s\n"
                         , _pszTime
                         , "NOTICE"
                         , dos_get_process_name()
                         , "OPTERATION"
                         , _pszOpterator
                         , _pszOpterator
                         , _pszResult
                         , _pszMsg);
    if (ulLength < sizeof(szBuff))
    {
        szBuff[ulLength] = '\0';
        ulLength++;
    }
    else
    {
        szBuff[sizeof(szBuff) - 1] = '\0';
    }

    debug_cli_send_log(szBuff, ulLength);
}

U32 log_cli_set_level(U32 ulLevel)
{
    if (ulLevel >= LOG_LEVEL_INVAILD)
    {
        return DOS_FAIL;
    }

    m_stCliLog.ulCurrentLevel = ulLevel;

    return DOS_SUCC;
}


U32 log_cli_reg(DOS_LOG_MODULE_ST **pstModuleCB)
{
    m_stCliLog.ulCurrentLevel = LOG_CLI_DEFAULT_LEVEL;                /* 当前log级别 */

    m_stCliLog.fnInit      = log_cli_init;            /* 模块初始化函数 */
    m_stCliLog.fnStart     = log_cli_start;           /* 模块启动函数 */
    m_stCliLog.fnStop      = log_cli_stop;            /* 模块停止函数 */
    m_stCliLog.fnWriteRlog = log_cli_write_rlog;      /* 模块写运行日志 */
    m_stCliLog.fnWriteOlog = log_cli_write_olog;      /* 模块写操作日志 */
    m_stCliLog.fnSetLevel  = log_cli_set_level;

    *pstModuleCB = &m_stCliLog;

    return DOS_SUCC;
}

#endif /* end of INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_CLI && INCLUDE_DEBUG_CLI */

#ifdef __cplusplus
}
#endif /* __cplusplus */


