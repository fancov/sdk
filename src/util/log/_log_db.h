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

#ifndef __LOG_DB_H__
#define __LOG_DB_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern "C"{
#include <dos.h>
#include <dos/dos_debug.h>
#include <dos/dos_def.h>
#include <dos/dos_db.h>
}

#if (INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_DB)
#include <mysql/mysql.h>
#include "_log_base.h"

#define MAX_DB_INFO_NUM  6
#define MAX_DB_INFO_LEN  48
#define EXTRA_LEN        128 /*定义额外的字符串长度*/

class CLogDB : virtual public CLog
{
private:
    DB_HANDLE_ST  *pstDBHandle;
    BOOL          blInited;       /* mysql是否被初始化了 */
    U32           ulLogLevel;
public:
    CLogDB();
    ~CLogDB();
    S32 log_init();
    S32 log_init(S32 _lArgc, S8 **_pszArgv);
    S32 log_set_level(U32 ulLevel);
    VOID log_write(const S8 *_pszTime, const S8 *_pszType, const S8 *_pszLevel, const S8 *_pszMsg, U32 _ulLevel);
    VOID log_write(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg);
};

#endif

#endif

