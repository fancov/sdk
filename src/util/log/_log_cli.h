/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  _log_cli.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现向统一控制平台发送日志的功能类的定义
 *     History:
 */

#ifndef __DEBUG_CLI_H__
#define __DEBUG_CLI_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <dos/dos_types.h>
#include <syscfg.h>
extern "C"{
#include <dos/dos_debug.h>
#include <dos/dos_def.h>
#include <dos/dos_cli.h>
}



#if (INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_CLI)

#include "_log_base.h"

/**
 * 定义cli日志类
 */
class CLogCli : public CLog
{
private:
    U32 ulCurrentLevel;
public:
    CLogCli();
    virtual ~CLogCli();
    S32 log_init();
    S32 log_init(S32 _lArgc, S8 **_pszArgv);
    S32 log_set_level(U32 ulLevel);
    VOID log_write(const S8 *_pszTime, const S8 *_pszType, const S8 *_pszLevel, const S8 *_pszMsg, U32 _ulLevel);
    VOID log_write(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg);
};

#endif

#endif
