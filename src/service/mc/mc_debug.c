/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  mc_debug.c
 *
 *  Created on: 2015-8-11 16:05:35
 *      Author: Larry
 *        Todo: 媒体处理任务调试函数集合
 *     History:
 */

#include <dos.h>
#include <mc_pub.h>
#include "mc_def.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* 调度任务控制块 */
extern MC_SERVER_CB   *g_pstMasterTask;

/* 工作任务控制块 */
extern MC_SERVER_CB   *g_pstWorkingTask;


static U32  g_ulMCLogLevel = LOG_LEVEL_DEBUG;

S32 mc_cmd_status(U32 ulIndex, S32 argc, S8 **argv)
{
    S8 szBuffer[128];

    cli_out_string(ulIndex, "\r\n\r\nStatus since startup:");

    dos_snprintf(szBuffer, sizeof(szBuffer), "\r\nTask count: %u", MC_MAX_WORKING_TASK_CNT);
    cli_out_string(ulIndex, szBuffer);

    dos_snprintf(szBuffer, sizeof(szBuffer), "\r\nTotal load: %u", g_pstMasterTask->ulTotalProc);
    cli_out_string(ulIndex, szBuffer);

    dos_snprintf(szBuffer, sizeof(szBuffer), "\r\nTotal processed: %u", g_pstMasterTask->ulSuccessProc);
    cli_out_string(ulIndex, szBuffer);

    return 0;
}

S32 mc_cmd_stat(U32 ulIndex, S32 argc, S8 **argv)
{
    S8 szBuffer[128];
    U32 i;

    cli_out_string(ulIndex, "\r\n\r\nStat for the working task:");

    dos_snprintf(szBuffer, sizeof(szBuffer), "\r\n%8s%20s%20s", "ID", "Total Processed", "Success");
    cli_out_string(ulIndex, szBuffer);

    cli_out_string(ulIndex, "\r\n------------------------------------------------");
    for (i=0; i<MC_MAX_WORKING_TASK_CNT; i++)
    {
        dos_snprintf(szBuffer, sizeof(szBuffer)
                        , "\r\n%8u%20u%20u"
                        , i
                        , g_pstWorkingTask[i].ulTotalProc
                        , g_pstWorkingTask[i].ulSuccessProc);
        cli_out_string(ulIndex, szBuffer);
    }
    cli_out_string(ulIndex, "\r\n------------------------------------------------\r\n\r\n");

    return 0;
}

S32 mc_set_log_level(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulLogLevel = LOG_LEVEL_INVAILD;

    if (argc < 4)
    {
        goto proc_fail;
    }

    if (dos_strnicmp(argv[3], "debug", dos_strlen("debug")) == 0)
    {
        ulLogLevel = LOG_LEVEL_DEBUG;
    }
    else if (dos_strnicmp(argv[3], "info", dos_strlen("info")) == 0)
    {
        ulLogLevel = LOG_LEVEL_INFO;
    }
    else if (dos_strnicmp(argv[3], "notice", dos_strlen("notice")) == 0)
    {
        ulLogLevel = LOG_LEVEL_NOTIC;
    }
    else if (dos_strnicmp(argv[3], "warning", dos_strlen("warning")) == 0)
    {
        ulLogLevel = LOG_LEVEL_WARNING;
    }
    else if (dos_strnicmp(argv[3], "error", dos_strlen("error")) == 0)
    {
        ulLogLevel = LOG_LEVEL_ERROR;
    }
    else if (dos_strnicmp(argv[3], "cirt", dos_strlen("cirt")) == 0)
    {
        ulLogLevel = LOG_LEVEL_CIRT;
    }
    else if (dos_strnicmp(argv[3], "alert", dos_strlen("alert")) == 0)
    {
        ulLogLevel = LOG_LEVEL_ALERT;
    }
    else if (dos_strnicmp(argv[3], "emerg", dos_strlen("emerg")) == 0)
    {
        ulLogLevel = LOG_LEVEL_EMERG;
    }

    if (LOG_LEVEL_INVAILD == ulLogLevel)
    {
        goto proc_fail;
    }

    g_ulMCLogLevel = ulLogLevel;

    cli_out_string(ulIndex, "\r\n\r\nSet MC log level success.\r\n");

    return 0;

proc_fail:
    cli_out_string(ulIndex, "\r\n\r\nSet MC log level FAIL\r\n");
    return -1;
}

S32 mc_cmd_set(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulRet = 0;

    if (argc < 3)
    {
        goto helper;
    }

    if (0 == dos_strnicmp(argv[1], "show", dos_strlen("show")))
    {
        if (0 == dos_strnicmp(argv[2], "status", dos_strlen("status")))
        {
            ulRet = mc_cmd_status(ulIndex, argc, argv);
        }
        else if (0 == dos_strnicmp(argv[2], "stat", dos_strlen("stat")))
        {
            ulRet = mc_cmd_stat(ulIndex, argc, argv);
        }
        else
        {
            goto helper;
        }
    }
    else if (0 == dos_strnicmp(argv[1], "set", dos_strlen("set")))
    {
        if (0 == dos_strnicmp(argv[2], "loglevel", dos_strlen("loglevel")))
        {
            ulRet = mc_set_log_level(ulIndex, argc, argv);
        }
        else
        {
            goto helper;
        }
    }

    if (ulRet != 0)
    {
        goto helper;
    }

    return 0;

helper:
    cli_out_string(ulIndex, "\r\n\r\nUse this command as:");
    cli_out_string(ulIndex, "\r\n    show status|stat");
    cli_out_string(ulIndex, "\r\n    set loglevel debug|info|notice|warning|error|cirt|alert|emerg");
    cli_out_string(ulIndex, "\r\n\r\n");

    return -1;
}

VOID mc_log(BOOL blMaster, U32 ulLevel, S8 *szFormat, ...)
{
    BOOL            bIsOutput = DOS_FALSE;
    va_list         Arg;
    S8              szTraceStr[1024] = {0, };

    if (ulLevel >= LOG_LEVEL_INVAILD)
    {
        return;
    }

    /* warning级别以上强制输出 */
    if (ulLevel <= LOG_LEVEL_WARNING)
    {
        bIsOutput = DOS_TRUE;
    }

    if (ulLevel <= g_ulMCLogLevel)
    {
        bIsOutput = DOS_TRUE;
    }

    if (!bIsOutput)
    {
        return;
    }

    va_start(Arg, szFormat);
    vsnprintf(szTraceStr, sizeof(szTraceStr), szFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(ulLevel, LOG_TYPE_RUNINFO, szTraceStr);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


