/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_debug.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: DOS调试相关函数集合
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include dos header files */
#include <dos.h>
#include <syslog.h>

/**
 * 通用打印函数
 */
VOID dos_vprintf(const S8 *pszFile, S32 lLine, const S8 *pszFormat, ...)
{
    va_list argptr;
    S8 szBuff[1024];

    va_start( argptr, pszFormat );
    vsnprintf( szBuff, 511, pszFormat, argptr );
    va_end( argptr );
    szBuff[sizeof(szBuff) -1] = '\0';

    //printf("%s\n", szBuff);
    logr_debug("%s (File:%s, Line:%d)", szBuff, pszFile, lLine);
}

VOID dos_syslog(S32 lLevel, S8 *pszLog)
{
    syslog(lLevel, "%s", pszLog);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */
