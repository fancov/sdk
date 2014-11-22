/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  _log_base.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 日志模块基础类定义，定义为虚类，添加日志模块时继承该类，并实现相关函数即可
 *     History:
 */


#ifndef __LOG_BASE_H__
#define __LOG_BASE_H__

#include <dos.h>

#if INCLUDE_SYSLOG_ENABLE

class CLog
{
public:
    virtual ~CLog(){};
    virtual S32 log_init(){ return 0; };
    virtual S32 log_init(S32 _lArgc, S8 **_pszArgv){  return 0; };
    virtual S32 log_set_level(U32 ulLevel){  return 0; };
    virtual VOID log_write(const S8 *_pszTime, const S8 *_pszType, const S8 *_pszLevel, const S8 *_pszMsg, U32 _ulLevel){};
    virtual VOID log_write(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg){};
};

#endif


#endif
