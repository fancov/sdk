/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  _log_console.cpp
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现将日志输出到命令行的功能
 *     History:
 */

#include "_log_console.h"

#if (INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_CONSOLE)

/* 构造函数 */
CLogConsole::CLogConsole()
{
    // Nothing
    this->ulLogLevel = LOG_LEVEL_DEBUG;
}

/* 释放函数 */
CLogConsole::~CLogConsole()
{
    // Nothing
}


S32 CLogConsole::log_init()
{
    return 0;
}

S32 CLogConsole::log_init(S32 argc, S8 **argv)
{
    return 0;
}

S32 CLogConsole::log_set_level(U32 ulLevel)
{
    ulLogLevel = ulLevel;
    return 0;
}

VOID CLogConsole::log_write(const S8 *_pszTime, const S8 *_pszType, const S8 *_pszLevel, const S8 *_pszMsg, U32 _ulLevel)
{
    if (_ulLevel > this->ulLogLevel)
    {
        return;
    }

    printf("%-20s[%-8s][%-10s][%-8s] %s\n"
            , _pszTime, _pszLevel, dos_get_process_name(), _pszType, _pszMsg);
}

VOID CLogConsole::log_write(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg)
{
    printf("%-20s[%-8s][%-10s][%-8s][%s][%s][%s]%s\n"
            , _pszTime
            , "NOTICE"
            , dos_get_process_name()
            , "OPTERATION"
            , _pszOpterator
            , _pszOpterator
            , _pszResult
            , _pszMsg);
}

#endif

