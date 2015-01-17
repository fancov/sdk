/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  cli_lib.c
 *
 *  Created on: 2014-12-15
 *      Author: Larry
 *        Todo: cli模块公共函数
 *     History:
 */
#include <dos.h>

#if (INCLUDE_DEBUG_CLI || INCLUDE_DEBUG_CLI_SERVER)

static U32 g_ulCliCurrentLogLevel = LOG_LEVEL_NOTIC;

DLLEXPORT VOID cli_log_set_level(U32 ulLevel)
{
    g_ulCliCurrentLogLevel = ulLevel;
}

DLLEXPORT U32 cli_log_current_level()
{
    return g_ulCliCurrentLogLevel;
}


DLLEXPORT VOID cli_logr_write(U32 ulLevel, S8 *pszFormat, ...)
{
    va_list argptr;
    char szBuf[1024];

    va_start(argptr, pszFormat);
    vsnprintf(szBuf, sizeof(szBuf), pszFormat, argptr);
    va_end(argptr);
    szBuf[sizeof(szBuf) -1] = '\0';

    if (ulLevel <= g_ulCliCurrentLogLevel)
    {
        dos_log(ulLevel, LOG_TYPE_RUNINFO, szBuf);
    }
}

#endif

