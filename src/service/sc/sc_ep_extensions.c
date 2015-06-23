/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_ep_extensions.c
 *
 *  创建时间: 2015年6月18日15:25:18
 *  作    者: Larry
 *  描    述: 处理FS核心发过来的各种事件
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <esl.h>
#include <sys/time.h>
#include <pthread.h>
#include <bs_pub.h>
#include <libcurl/curl.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"
#include "sc_ep.h"
#include "sc_acd_def.h"


#define SC_EXT_EVENT_LIST "custom sofia::register sofia::unregister"

SC_EP_HANDLE_ST  *g_pstExtMngtHangle  = NULL;
DLL_S            g_stExtMngtMsg;
pthread_mutex_t  g_mutexExtMngtMsg = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t   g_condExtMngtMsg  = PTHREAD_COND_INITIALIZER;
pthread_t        g_pthExtMngtProcTask;


VOID* sc_ep_ext_mgnt(VOID *ptr)
{
    struct timespec     stTimeout;
    DLL_NODE_S          *pstListNode = NULL;
    esl_event_t         *pstEvent    = NULL;

    for (;;)
    {
        pthread_mutex_lock(&g_mutexExtMngtMsg);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condExtMngtMsg, &g_mutexExtMngtMsg, &stTimeout);
        pthread_mutex_unlock(&g_mutexExtMngtMsg);

        while (1)
        {
            if (DLL_Count(&g_stExtMngtMsg) <= 0)
            {
                break;
            }

            pthread_mutex_lock(&g_mutexExtMngtMsg);

            pstListNode = dll_fetch(&g_stExtMngtMsg);
            if (DOS_ADDR_INVALID(pstListNode))
            {
                DOS_ASSERT(0);

                pthread_mutex_unlock(&g_mutexExtMngtMsg);
                continue;
            }

            pthread_mutex_unlock(&g_mutexExtMngtMsg);


            pstEvent = (esl_event_t *)pstListNode->pHandle;
            pstListNode->pHandle = NULL;
            if (pstEvent)
            {
                sc_logr_debug(SC_ACD, "User register: %s", esl_event_get_header(pstEvent, "username"));

                esl_event_destroy(&pstEvent);
                pstEvent = NULL;
            }
            dos_dmem_free(pstListNode);
        }
    }
}


/**
 * 函数: VOID* sc_ep_runtime(VOID *ptr)
 * 功能: ESL事件接收线程主函数
 * 参数:
 * 返回值:
 */
VOID* sc_ep_ext_runtime(VOID *ptr)
{
    U32                  ulRet = ESL_FAIL;
    DLL_NODE_S           *pstDLLNode = NULL;
    // 判断第一次连接是否成功
    static BOOL bFirstConnSucc = DOS_FALSE;

    for (;;)
    {
        /* 如果退出标志被置上，就准备退出了 */
        if (g_pstExtMngtHangle->blIsWaitingExit)
        {
            sc_logr_notice(SC_ESL, "%s", "Event process exit flag has been set. the task will be exit.");
            break;
        }

        /*
         * 检查连接是否正常
         * 如果连接不正常，就准备重连
         **/
        if (!g_pstExtMngtHangle->blIsESLRunning)
        {
            sc_logr_notice(SC_ESL, "%s", "ELS for event connection has been down, re-connect.");
            g_pstExtMngtHangle->stRecvHandle.event_lock = 1;
            ulRet = esl_connect(&g_pstExtMngtHangle->stRecvHandle, "127.0.0.1", 8021, NULL, "ClueCon");
            if (ESL_SUCCESS != ulRet)
            {
                esl_disconnect(&g_pstExtMngtHangle->stRecvHandle);
                sc_logr_notice(SC_ESL, "ELS for event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_pstExtMngtHangle->stRecvHandle.err);

                sleep(1);
                continue;
            }

            g_pstExtMngtHangle->blIsESLRunning = DOS_TRUE;
            g_pstExtMngtHangle->ulESLDebugLevel = ESL_LOG_LEVEL_INFO;
            esl_global_set_default_logger(g_pstExtMngtHangle->ulESLDebugLevel);
            esl_events(&g_pstExtMngtHangle->stRecvHandle, ESL_EVENT_TYPE_PLAIN, SC_EXT_EVENT_LIST);

            sc_logr_notice(SC_ESL, "%s", "ELS for event connect Back to Normal.");
        }

        if (!bFirstConnSucc)
        {
            bFirstConnSucc = DOS_TRUE;
            /* 需要做特殊处理，查询所有的分机 */
        }

        ulRet = esl_recv_event(&g_pstExtMngtHangle->stRecvHandle, 1, NULL);
        if (ESL_FAIL == ulRet)
        {
            sc_logr_info(SC_ESL, "%s", "ESL Recv event fail, continue.");
            g_pstExtMngtHangle->blIsESLRunning = DOS_FALSE;
            continue;
        }

        esl_event_t *pstEvent = g_pstExtMngtHangle->stRecvHandle.last_ievent;
        if (DOS_ADDR_INVALID(pstEvent))
        {
            sc_logr_info(SC_ESL, "%s", "ESL get event fail, continue.");
            g_pstExtMngtHangle->blIsESLRunning = DOS_FALSE;
            continue;
        }

        pstDLLNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            DOS_ASSERT(0);

            continue;
        }

        pthread_mutex_lock(&g_mutexExtMngtMsg);

        DLL_Init_Node(pstDLLNode);
        pstDLLNode->pHandle = NULL;
        esl_event_dup((esl_event_t **)(&pstDLLNode->pHandle), pstEvent);
        DLL_Add(&g_stExtMngtMsg, pstDLLNode);

        pthread_cond_signal(&g_condExtMngtMsg);
        pthread_mutex_unlock(&g_mutexExtMngtMsg);
    }

    /* @TODO 释放资源 */
    return NULL;
}

/* 初始化事件处理模块 */
U32 sc_ep_ext_init()
{    SC_TRACE_IN(0, 0, 0, 0);

    g_pstExtMngtHangle = dos_dmem_alloc(sizeof(SC_EP_HANDLE_ST));
    if (DOS_ADDR_INVALID(g_pstExtMngtHangle))
    {
        DOS_ASSERT(0);

        goto init_fail;
    }

    dos_memzero(g_pstExtMngtHangle, sizeof(SC_EP_HANDLE_ST));
    g_pstExtMngtHangle->blIsESLRunning = DOS_FALSE;
    g_pstExtMngtHangle->blIsWaitingExit = DOS_FALSE;

    DLL_Init(&g_stExtMngtMsg);

    SC_TRACE_OUT();
    return DOS_SUCC;

init_fail:

    return DOS_FAIL;
}

/* 启动事件处理模块 */
U32 sc_ep_ext_start()
{
    SC_TRACE_IN(0, 0, 0, 0);

    if (pthread_create(&g_pthExtMngtProcTask, NULL, sc_ep_ext_mgnt, NULL) < 0)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (pthread_create(&g_pstExtMngtHangle->pthID, NULL, sc_ep_ext_runtime, NULL) < 0)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/* 停止事件处理模块 */
U32 sc_ep_ext_shutdown()
{
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstExtMngtHangle->blIsWaitingExit = DOS_TRUE;

    SC_TRACE_OUT();
    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


