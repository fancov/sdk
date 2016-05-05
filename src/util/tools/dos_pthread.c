/**
 * @file : sc_ini.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 业务控制模块启动
 *
 * @date: 2016年1月15日
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
/* include private header files */

/* define marcos */
#define DOS_PTHREAD_MSG_MAX_COUNT       32
/* 线程的最大阻塞时间 秒 */
#define DOS_PTHREAD_BIOCK_MAX_TIME      15
#define DOS_PTHREAD_CHECK_TIME          5
#define DOD_PTHREAD_EXIT_ERRNO          9
#define DOS_PTHREAD_WARN_PATH           "/var/log/ipcc_error"

/* define structs */
pthread_t            g_pthPthread;
SC_PTHREAD_MSG_ST    astPthreadMsg[DOS_PTHREAD_MSG_MAX_COUNT];

VOID dos_pthread_msg_init(SC_PTHREAD_MSG_ST *pstPthreadMsg)
{
    if (DOS_ADDR_VALID(pstPthreadMsg))
    {
        pstPthreadMsg->bIsValid = DOS_FALSE;
        pstPthreadMsg->ulPthID = 0;
        pstPthreadMsg->ulLastTime = 0;
        pstPthreadMsg->pParam = NULL;
        pstPthreadMsg->func = NULL;
        pstPthreadMsg->szName[0] = '\0';
    }
}

U32 dos_pthread_init()
{
#if INCLUDE_PTHREAD_CHECK
    U32 ulIndex = 0;

    for (ulIndex=0; ulIndex<DOS_PTHREAD_MSG_MAX_COUNT; ulIndex++)
    {
        dos_pthread_msg_init(&astPthreadMsg[ulIndex]);
    }
#endif

    return DOS_SUCC;
}

SC_PTHREAD_MSG_ST *dos_pthread_cb_alloc()
{
    U32 ulIndex = 0;

    for (ulIndex=0; ulIndex<DOS_PTHREAD_MSG_MAX_COUNT; ulIndex++)
    {
        if (!astPthreadMsg[ulIndex].bIsValid)
        {
            astPthreadMsg[ulIndex].bIsValid = DOS_TRUE;
            return &astPthreadMsg[ulIndex];
        }
    }

    return NULL;
}

VOID *dos_pthread_runtime(VOID *ptr)
{
    U32     ulIndex         = 0;
    time_t  ulCurrTime      = 0;
    FILE    *pstLogFile     = NULL;
    S8      szMsg[512]      = {0,};

    while (1)
    {
        ulCurrTime = time(NULL);
        for (ulIndex=0; ulIndex<DOS_PTHREAD_MSG_MAX_COUNT; ulIndex++)
        {
            if (!astPthreadMsg[ulIndex].bIsValid
                || !astPthreadMsg[ulIndex].ulLastTime)
            {
                continue;
            }

            if (ulCurrTime - astPthreadMsg[ulIndex].ulLastTime > DOS_PTHREAD_BIOCK_MAX_TIME)
            {
                /* 线程阻塞，警告写入文档，退出程序 */
                pstLogFile = fopen(DOS_PTHREAD_WARN_PATH, "a");
                if (DOS_ADDR_INVALID(pstLogFile))
                {
                    exit(DOD_PTHREAD_EXIT_ERRNO);
                }

                dos_snprintf(szMsg, sizeof(szMsg), "Pthread lock %s\r\n", astPthreadMsg[ulIndex].szName);

                fseek(pstLogFile, 0L, SEEK_END);

                fwrite(szMsg, dos_strlen(szMsg), 1, pstLogFile);
                fclose(pstLogFile);

                exit(DOD_PTHREAD_EXIT_ERRNO);
            }
        }

        sleep(DOS_PTHREAD_CHECK_TIME);
    }
}

U32 dos_pthread_start()
{
#if INCLUDE_PTHREAD_CHECK
    if (pthread_create(&g_pthPthread, NULL, dos_pthread_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
#endif

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */



