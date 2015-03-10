/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  _log_cli.cpp
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现将日志输出到统一管理平台
 *     History:
 */

#include "_log_cli.h"

#if (INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_CLI)

extern "C" S32 debug_cli_send_log(S8 * _pszMsg, S32 _lLength);

CLogCli::CLogCli()
{
    this->ulCurrentLevel = LOG_LEVEL_INVAILD;
}

CLogCli::~CLogCli()
{

}

S32 CLogCli::log_init()
{
    this->ulCurrentLevel = LOG_LEVEL_INVAILD;
    return 0;
}

S32 CLogCli::log_init(S32 _iArgc, S8 **_pszArgv)
{
    return 0;
}

S32 CLogCli::log_set_level(U32 ulLevel)
{
    this->ulCurrentLevel = ulLevel;

    return 0;
}

VOID CLogCli::log_write(const S8 *_pszTime, const S8 *_pszType, const S8 *_pszLevel, const S8 *_pszMsg, U32 _ulLevel)
{
#if INCLUDE_DEBUG_CLI
    S8 szBuff[1024] = { 0 };
    U32 ulLength;

    if (LOG_LEVEL_INVAILD == this->ulCurrentLevel
        || _ulLevel > this->ulCurrentLevel)
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
#endif
}

VOID CLogCli::log_write(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg)
{
#if INCLUDE_DEBUG_CLI
    S8 szBuff[1024] = { 0 };
    U32 ulLength;

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
#endif
}

#endif

