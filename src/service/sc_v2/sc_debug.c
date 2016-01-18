/**
 * @file : sc_debug.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 调试函数集合
 *
 * @date: 2016年1月9日
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_def.h"

U32         g_ulSCLogLevel = LOG_LEVEL_DEBUG;

S32 cli_cc_process(U32 ulIndex, S32 argc, S8 **argv)
{
    return -1;
}


/**
 * SC模块打印函数
 *
 * @param const S8 *pszFormat 格式列表
 *
 * return NULL
 */
VOID sc_log(U32 ulLevel, const S8 *pszFormat, ...)
{
    va_list         Arg;
    U32             ulTraceTagLen = 0;
    S8              szTraceStr[1024] = {0, };
    BOOL            bIsOutput = DOS_FALSE;

    if (ulLevel >= LOG_LEVEL_INVAILD)
    {
        return;
    }

    /* warning级别以上强制输出 */
    if (ulLevel <= LOG_LEVEL_WARNING)
    {
        bIsOutput = DOS_TRUE;
    }

    if (!bIsOutput
        && ulLevel <= g_ulSCLogLevel)
    {
        bIsOutput = DOS_TRUE;
    }

    if(!bIsOutput)
    {
        return;
    }

    ulTraceTagLen = dos_strlen(szTraceStr);

    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(ulLevel, LOG_TYPE_RUNINFO, szTraceStr);

}

/**
 * SC模块打印函数
 *
 * @param const S8 *pszFormat 格式列表
 *
 * return NULL
 */
VOID sc_printf(const S8 *pszFormat, ...)
{
    va_list         Arg;
    S8              szTraceStr[1024] = {0, };
    U32             ulTraceTagLen = 0;

    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(LOG_LEVEL_DEBUG, LOG_TYPE_RUNINFO, szTraceStr);
}

/**
 * 跟踪打印业务控制块
 *
 * @parma SC_SRV_CB *pstSCB 业务控制块
 * @param const S8 *pszFormat 格式列表
 *
 * return NULL
 */
VOID sc_trace_scb(SC_SRV_CB *pstSCB, const S8 *pszFormat, ...)
{
    va_list         Arg;
    S8              szTraceStr[1024] = {0, };
    U32             ulTraceTagLen = 0;

    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(LOG_LEVEL_DEBUG, LOG_TYPE_RUNINFO, szTraceStr);
}

/**
 * 跟踪打印LEG控制块
 *
 * @parma SC_LEG_CB *pstLCB   LEG控制块
 * @param const S8 *pszFormat 格式列表
 *
 * return NULL
 */
VOID sc_trace_leg(SC_LEG_CB *pstLCB, const S8 *pszFormat, ...)
{
    va_list         Arg;
    S8              szTraceStr[1024] = {0, };
    U32             ulTraceTagLen = 0;

    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(LOG_LEVEL_DEBUG, LOG_TYPE_RUNINFO, szTraceStr);
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */


