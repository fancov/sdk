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
#include "sc_def.h"
#include "sc_debug.h"

/* define marcos */
#define SC_PTHREAD_MSG_MAX_COUNT       32
/* 线程的最大阻塞时间 秒 */
#define SC_PTHREAD_BIOCK_MAX_TIME      30
#define SC_PTHREAD_CHECK_TIME          30

/* define structs */
pthread_t            g_pthPthread;
SC_PTHREAD_MSG_ST    astPthreadMsg[SC_PTHREAD_MSG_MAX_COUNT];

VOID sc_pthread_msg_init(SC_PTHREAD_MSG_ST *pstPthreadMsg)
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

U32 sc_pthread_init()
{
    U32 ulIndex = 0;

    for (ulIndex=0; ulIndex<SC_PTHREAD_MSG_MAX_COUNT; ulIndex++)
    {
        sc_pthread_msg_init(&astPthreadMsg[ulIndex]);
    }

    return DOS_SUCC;
}

SC_PTHREAD_MSG_ST *sc_pthread_cb_alloc()
{
    U32 ulIndex = 0;

    for (ulIndex=0; ulIndex<SC_PTHREAD_MSG_MAX_COUNT; ulIndex++)
    {
        if (!astPthreadMsg[ulIndex].bIsValid)
        {
            astPthreadMsg[ulIndex].bIsValid = DOS_TRUE;
            return &astPthreadMsg[ulIndex];
        }
    }

    return NULL;
}

VOID *sc_pthread_runtime(VOID *ptr)
{
    U32 ulIndex = 0;
    time_t ulCurrTime = 0;

    while (1)
    {
        ulCurrTime = time(NULL);
        for (ulIndex=0; ulIndex<SC_PTHREAD_MSG_MAX_COUNT; ulIndex++)
        {
            if (!astPthreadMsg[ulIndex].bIsValid
                || !astPthreadMsg[ulIndex].ulLastTime)
            {
                continue;
            }

            if (ulCurrTime - astPthreadMsg[ulIndex].ulLastTime > SC_PTHREAD_BIOCK_MAX_TIME)
            {
                /* 线程阻塞，停止，重启 */
                if (!pthread_cancel(astPthreadMsg[ulIndex].ulPthID))
                {
                    sc_log(DOS_TRUE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Pthread Exit SUCC!!!. %s", astPthreadMsg[ulIndex].szName);
                }
                else
                {
                    sc_log(DOS_TRUE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Pthread Exit FAIL!!!. %s", astPthreadMsg[ulIndex].szName);
                }

                astPthreadMsg[ulIndex].ulLastTime = 0;

                if (pthread_create(&astPthreadMsg[ulIndex].ulPthID, NULL, astPthreadMsg[ulIndex].func, astPthreadMsg[ulIndex].pParam) < 0)
                {
                    DOS_ASSERT(0);
                    sc_log(DOS_TRUE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Start Pthread FAIL!!!. %s", astPthreadMsg[ulIndex].szName);
                }
                else
                {
                    sc_log(DOS_TRUE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Start Pthread SUCC!!!. %s", astPthreadMsg[ulIndex].szName);
                }
            }
        }

        sleep(SC_PTHREAD_CHECK_TIME);
    }
}

U32 sc_pthread_start()
{
    if (pthread_create(&g_pthPthread, NULL, sc_pthread_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */



