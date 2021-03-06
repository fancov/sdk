/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  main.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: DOS 操作系统信号拦截相关函数
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include header files */
#include <dos.h>

#if INCLUDE_EXCEPTION_CATCH

#include <signal.h>
#include <time.h>
#include <execinfo.h>

/**
 * Function: dos_backtrace(S32 iSig)
 *  Params :
 *           iSig 所发生的信号
 *  Return : VOID
 *    Todo : 发生信号是，抓取程序运行堆栈
 */
VOID dos_backtrace(S32 lSig)
{
    void* pArray[100] = {0};
    U32 ulSize = 0;
    char **strFrame = NULL;
    U32 i = 0;
	S8 szBuff[512] = { 0 };
    time_t stTime;
    struct tm *pstTime;
    S8 szTime[32] = { 0 };

    time(&stTime);
    pstTime = localtime(&stTime);
    strftime(szTime, sizeof(szTime), "%Y-%m-%d %H:%M:%S", pstTime);

    ulSize = backtrace(pArray, 100);
    strFrame = (char **)backtrace_symbols(pArray, ulSize);

    dos_snprintf(szBuff, sizeof(szBuff), "--------------------%s-------------------\r\n", szTime);
    dos_syslog(LOG_LEVEL_CIRT, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "Start record call stack backtrace: \r\n");
    dos_syslog(LOG_LEVEL_CIRT, szBuff);
    logr_debug(szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "Recv Signal: %d,  Call stack:\r\n", lSig);
    dos_syslog(LOG_LEVEL_CIRT, szBuff);
    logr_debug(szBuff);

    for (i = 1; i < ulSize; i++)
    {
        dos_snprintf(szBuff, sizeof(szBuff), "frame %d -- %s\r\n", i, strFrame[i]);
        logr_debug(szBuff);
        dos_syslog(LOG_LEVEL_CIRT, szBuff);
    }

    /* 记录断言 */
    dos_assert_record();
}

/**
 * Function: dos_signal_handle(S32 iSig)
 *  Params :
 *           iSig 所发生的信号
 *  Return : VOID
 *    Todo : 发生信号是，分发到不同的处理函数
 */
VOID dos_signal_handle(S32 lSig)
{
    S8 szBuff[215] = { 0 };
    S8 *pszProcessName = NULL;

    switch (lSig)
    {
        case SIGSEGV:
        case SIGPIPE:
        case SIGTERM:
        case SIGQUIT:
        case SIGKILL:
            dos_backtrace(lSig);
            break;
        case SIGINT:
            dos_backtrace(lSig);
            break;
    }

    dos_printf("%s", "\nexit()\n");

    if (dos_get_pid_file_path(szBuff, sizeof(szBuff))
        && (pszProcessName = dos_get_process_name()))
    {
        snprintf(szBuff, sizeof(szBuff), "%s/%s.pid", szBuff, pszProcessName);
        unlink(szBuff);
    }

    dos_syslog(LOG_LEVEL_EMERG, "The programm will be exited soon.\r\n");

    /* 程序退出 */
    exit(lSig);
}

/**
 * Function: dos_signal_handle_reg()
 *  Params :
 *  Return : VOID
 *    Todo : 注册信号处理函数
 */
VOID dos_signal_handle_reg()
{
    struct sigaction act;

    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = dos_signal_handle;
    act.sa_flags   = 0;
    sigemptyset(&act.sa_mask);

    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGKILL, &act, NULL);
    sigaction(SIGSTOP, &act, NULL);
    sigaction(SIGHUP, &act, NULL);

    act.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);
}
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */
