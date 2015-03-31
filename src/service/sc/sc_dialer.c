/*
*            (C) Copyright 2014, DIPCC . Co., Ltd.
*                    ALL RIGHTS RESERVED
*
*  文件名: sc_dialer.h
*
*  创建时间: 2014年12月16日10:19:49
*  作    者: Larry
*  描    述: 群呼任务拨号器
*  修改历史:
*/

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <sys/time.h>

/* include private header files */
#include <esl.h>
#include <bs_pub.h>
#include "sc_def.h"
#include "sc_debug.h"

/* define marcos */

/* define enums */

/* define structs */
/* 呼叫队列 */
typedef struct tagSCCallQueueNode
{
    list_t      stList;                         /* 呼叫队列链表 */
    SC_SCB_ST   *pstSCB;                        /* 呼叫控制块 */
}SC_CALL_QUEUE_NODE_ST;

/* dialer模块控制块 */
typedef struct tagSCDialerHandle
{
    esl_handle_t        stHandle;                /*  esl 句柄 */
    pthread_t           pthID;
    pthread_mutex_t     mutexCallQueue;          /* 互斥锁 */
    pthread_cond_t      condCallQueue;           /* 条件变量 */
    list_t              stCallList;              /* 呼叫队列 */

    BOOL                blIsESLRunning;          /* ESL是否连接正常 */
    BOOL                blIsWaitingExit;         /* 任务是否正在等待退出 */
    S8                  *pszCMDBuff;
}SC_DIALER_HANDLE_ST;

/* dialer模块控制块示例 */
SC_DIALER_HANDLE_ST  *g_pstDialerHandle = NULL;

/* 这个地方有个问题。 g_pstDialerHandle->stHandle 被多个线程使用，会不会出现，一个线程刚刚发送了呼叫命令。名外一个线程收到了响应?*/
U32 sc_dial_make_call2ip(S8 *pszCaller, S8 *pszCallee, U32 ulMainService)
{
    S8    szCMDBuff[SC_ESL_CMD_BUFF_LEN] = { 0 };
    S8    *pszEventHeader = NULL, *pszEventBody = NULL;

    if (DOS_ADDR_INVALID(pszCaller)
        || DOS_ADDR_INVALID(pszCallee))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    dos_snprintf(szCMDBuff, sizeof(szCMDBuff)
                    , "bgapi originate {main_service=%s,origination_caller_id_number=%s,origination_caller_id_name=%s}user/%s &park \r\n"
                    , ulMainService
                    , pszCaller
                    , pszCaller
                    , pszCallee);

    sc_logr_debug(SC_DIALER, "ESL CMD: %s", szCMDBuff);

    if (esl_send_recv(&g_pstDialerHandle->stHandle, szCMDBuff) != ESL_SUCCESS)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call FAIL.Msg:%s(%d)", g_pstDialerHandle->stHandle.err, g_pstDialerHandle->stHandle.errnum);
        g_pstDialerHandle->blIsESLRunning = DOS_FALSE;
        goto esl_exec_fail;
    }

    if (!g_pstDialerHandle->stHandle.last_sr_event)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "%s", "ESL request call successfully. But the reply event is NULL.");

        goto esl_exec_fail;
    }

    pszEventHeader = esl_event_get_header(g_pstDialerHandle->stHandle.last_sr_event, "Content-Type");
    if (!pszEventHeader || '\0' == pszEventHeader[0]
        || dos_strcmp(pszEventHeader, "command/reply") != 0)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call successfully. But the reply event an invalid content-type.(Type:%s)", pszEventHeader);

        goto esl_exec_fail;

    }

    pszEventBody = esl_event_get_header(g_pstDialerHandle->stHandle.last_sr_event, "reply-text");
    if (!pszEventBody || '\0' == pszEventBody[0])
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call successfully. But the reply event an invalid reply-text.(Type:%s)", pszEventBody);

        goto esl_exec_fail;
    }

    if (dos_strnicmp(pszEventBody, "+OK", dos_strlen("+OK")) != 0)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL exec fail. (Reply-Text:%s)", pszEventBody);

        goto esl_exec_fail;
    }

    sc_logr_info(SC_DIALER, "Make call successfully. Caller:%s, Callee:%s", pszCaller, pszCallee);

    SC_TRACE_OUT();
    return DOS_SUCC;

esl_exec_fail:

    sc_logr_info(SC_DIALER, "%s", "ESL Exec fail, the call will be FREE.");

    SC_TRACE_OUT();
    return DOS_FAIL;

}

U32 sc_dialer_make_call2pstn(SC_SCB_ST *pstSCB, U32 ulMainService)
{
    S8    szCMDBuff[SC_ESL_CMD_BUFF_LEN] = { 0 };
    S8    szCallString[SC_ESL_CMD_BUFF_LEN] = { 0 };
    U32   ulRouteID = U32_BUTT;
    S8    *pszEventHeader = NULL, *pszEventBody = NULL;
    SC_SCB_ST *pstSCBOther = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if ('\0' == pstSCB->szCalleeNum[0]
        || '\0' == pstSCB->szCallerNum[0])
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }


    ulRouteID = sc_ep_search_route(pstSCB);
    if (U32_BUTT == ulRouteID)
    {
        DOS_ASSERT(0);
        sc_logr_info(SC_ESL, "%s", "Search route fail while make call to patn.");
        return DOS_FAIL;
    }

    if (sc_ep_get_callee_string(ulRouteID, pstSCB->szCalleeNum, szCallString, sizeof(szCallString)) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* 如果不是auto dialing，就直接bridge就好 */
    if (!sc_call_check_service(pstSCB, SC_SERV_AUTO_DIALING))
    {
#if 0
        dos_snprintf(szCMDBuff, sizeof(szCMDBuff)
                        , "bgapi originate {instant_ringback=true,scb_number=%d,other_leg_scb=%d,main_service=%d,origination_caller_id_number=%s,origination_caller_id_name=%s,waiting_park=true}%s &park \r\n"
                        , pstSCB->usSCBNo
                        , pstSCB->usOtherSCBNo
                        , ulMainService
                        , pstSCB->szCallerNum
                        , pstSCB->szCallerNum
                        , szCallString);
#endif
        pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
        if (DOS_ADDR_INVALID(pstSCBOther))
        {
            DOS_ASSERT(0);

            sc_logr_info(SC_ESL, "Invalid originate SCB while make call to pstn. Current SCB:%d, Other SCB:%d", pstSCB->usSCBNo, pstSCB->usOtherSCBNo);

            sc_scb_free(pstSCB);

            return DOS_FAIL;
        }

        dos_snprintf(szCMDBuff, sizeof(szCMDBuff)
                        , "{instant_ringback=true,scb_number=%d,other_leg_scb=%d,main_service=%d,origination_caller_id_number=%s,origination_caller_id_name=%s}%s"
                        , pstSCB->usSCBNo
                        , pstSCB->usOtherSCBNo
                        , ulMainService
                        , pstSCB->szCallerNum
                        , pstSCB->szCallerNum
                        , szCallString);

        sc_ep_esl_execute("bridge", szCMDBuff, pstSCBOther->szUUID);

        return DOS_SUCC;
    }

    dos_snprintf(szCMDBuff, sizeof(szCMDBuff)
                        , "bgapi originate {scb_number=%d,main_service=%d,origination_caller_id_number=%s,origination_caller_id_name=%s,waiting_park=true}%s &park \r\n"
                        , pstSCB->usSCBNo
                        , ulMainService
                        , pstSCB->szCallerNum
                        , pstSCB->szCallerNum
                        , szCallString);

    sc_logr_debug(SC_DIALER, "ESL CMD: %s", szCMDBuff);

    if (esl_send_recv(&g_pstDialerHandle->stHandle, szCMDBuff) != ESL_SUCCESS)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call FAIL.Msg:%s(%d)", g_pstDialerHandle->stHandle.err, g_pstDialerHandle->stHandle.errnum);

        g_pstDialerHandle->blIsESLRunning = DOS_FALSE;

        goto esl_exec_fail;
    }

    if (!g_pstDialerHandle->stHandle.last_sr_event)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "%s", "ESL request call successfully. But the reply event is NULL.");

        goto esl_exec_fail;
    }

    pszEventHeader = esl_event_get_header(g_pstDialerHandle->stHandle.last_sr_event, "Content-Type");
    if (!pszEventHeader || '\0' == pszEventHeader[0]
        || dos_strcmp(pszEventHeader, "command/reply") != 0)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call successfully. But the reply event an invalid content-type.(Type:%s)", pszEventHeader);

        goto esl_exec_fail;

    }

    pszEventBody = esl_event_get_header(g_pstDialerHandle->stHandle.last_sr_event, "reply-text");
    if (!pszEventBody || '\0' == pszEventBody[0])
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call successfully. But the reply event an invalid reply-text.(Type:%s)", pszEventBody);

        goto esl_exec_fail;
    }

    if (dos_strnicmp(pszEventBody, "+OK", dos_strlen("+OK")) != 0)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL exec fail. (Reply-Text:%s)", pszEventBody);

        goto esl_exec_fail;
    }

    /* 这个地方客户那边已经接通了，所以直接到ACTIVE */
    SC_SCB_SET_STATUS(pstSCB, SC_SCB_ACTIVE);

    sc_logr_info(SC_DIALER, "Make call successfully. Caller:%s, Callee:%s", pstSCB->szCallerNum, pstSCB->szCalleeNum);
    sc_call_trace(pstSCB, "Make call successfully.");

    SC_TRACE_OUT();
    return DOS_SUCC;

esl_exec_fail:

    sc_logr_info(SC_DIALER, "%s", "ESL Exec fail, the call will be FREE.");

    SC_TRACE_OUT();
    return DOS_FAIL;
}

/*
 * 函数: VOID *sc_dialer_runtime(VOID * ptr)
 * 功能: 拨号模块主函数
 * 参数:
 */
VOID *sc_dialer_runtime(VOID * ptr)
{
    list_t                  *pstList;
    SC_CALL_QUEUE_NODE_ST   *pstListNode;
    SC_SCB_ST               *pstSCB;
    struct timespec         stTimeout;
    U32                     ulRet = ESL_FAIL;

    while(1)
    {
        /* 如果退出标志被置上，就准备退出了 */
        if (g_pstDialerHandle->blIsWaitingExit)
        {
            sc_logr_notice(SC_DIALER, "%s", "Dialer exit flag has been set. the task will be exit.");
            break;
        }

        /*
         * 检查连接是否正常
         * 如果连接不正常，就准备重连
         **/
        if (!g_pstDialerHandle->blIsESLRunning)
        {
            sc_logr_notice(SC_DIALER, "%s", "ELS connection has been down, re-connect.");
            ulRet = esl_connect(&g_pstDialerHandle->stHandle, "127.0.0.1", 8021, NULL, "ClueCon");
            if (ESL_SUCCESS != ulRet)
            {
                esl_disconnect(&g_pstDialerHandle->stHandle);
                sc_logr_notice(SC_DIALER, "ELS re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_pstDialerHandle->stHandle.err);

                sleep(1);
                continue;
            }

            g_pstDialerHandle->blIsESLRunning = DOS_TRUE;

            sc_logr_notice(SC_DIALER, "%s", "ELS connect Back to Normal.");
        }

        pthread_mutex_lock(&g_pstDialerHandle->mutexCallQueue);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_pstDialerHandle->condCallQueue, &g_pstDialerHandle->mutexCallQueue, &stTimeout);

        if (dos_list_is_empty(&g_pstDialerHandle->stCallList))
        {
            pthread_mutex_unlock(&g_pstDialerHandle->mutexCallQueue);
            continue;
        }

        while (1)
        {
            if (dos_list_is_empty(&g_pstDialerHandle->stCallList))
            {
                break;
            }

            pstList = dos_list_fetch(&g_pstDialerHandle->stCallList);
            if (!pstList)
            {
                continue;
            }

            pstListNode = dos_list_entry(pstList, SC_CALL_QUEUE_NODE_ST, stList);
            if (!pstList)
            {
                continue;
            }

            pstSCB = pstListNode->pstSCB;
            if (!pstSCB)
            {
                continue;
            }

            /* 认证 */
            if (sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
            {
                pstSCB->ucTerminationFlag = DOS_TRUE;
                pstSCB->ucTerminationCause = BS_ERR_SYSTEM;

                SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);

                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
            else
            {
                SC_SCB_SET_STATUS(pstSCB, SC_SCB_AUTH);
            }

            //sc_dialer_make_call2pstn(pstSCB, SC_SERV_AUTO_DIALING);

            /* SCB是预分配的，所以这里只需要把队列节点释放一下就好 */
            pstListNode->pstSCB = NULL;
            pstListNode->stList.next = NULL;
            pstListNode->stList.prev = NULL;
            dos_dmem_free(pstListNode);
            pstListNode = NULL;
        }

        pthread_mutex_unlock(&g_pstDialerHandle->mutexCallQueue);
    }

    return NULL;
}

/*
 * 函数: U32 sc_dialer_add_call(SC_SCB_ST *pstSCB)
 * 功能: 向拨号模块添加一个呼叫
 * 参数:
 *      SC_SCB_ST *pstSCB: 呼叫控制块
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_dialer_add_call(SC_SCB_ST *pstSCB)
{
    SC_CALL_QUEUE_NODE_ST *pstNode;
    SC_TRACE_IN((U64)pstSCB, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_call_trace(pstSCB, "Dialer recv call request.");

    pstNode = (SC_CALL_QUEUE_NODE_ST *)dos_dmem_alloc(sizeof(SC_CALL_QUEUE_NODE_ST));
    if (!pstNode)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstNode->pstSCB = pstSCB;
    pthread_mutex_lock(&g_pstDialerHandle->mutexCallQueue);
    dos_list_add_tail(&g_pstDialerHandle->stCallList, &(pstNode->stList));

    sc_call_trace(pstSCB, "Call request has been accepted by the dialer.");

    pthread_cond_signal(&g_pstDialerHandle->condCallQueue);
    pthread_mutex_unlock(&g_pstDialerHandle->mutexCallQueue);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/*
 * 函数: U32 sc_dialer_init()
 * 功能: 初始化拨号模块
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_dialer_init()
{
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstDialerHandle = (SC_DIALER_HANDLE_ST *)dos_dmem_alloc(sizeof(SC_DIALER_HANDLE_ST));
    if (!g_pstDialerHandle)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    g_pstDialerHandle->pszCMDBuff = (S8 *)dos_dmem_alloc(SC_ESL_CMD_BUFF_LEN);
    if (!g_pstDialerHandle->pszCMDBuff)
    {
        DOS_ASSERT(0);

        dos_dmem_free(g_pstDialerHandle);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_memzero(g_pstDialerHandle, sizeof(SC_DIALER_HANDLE_ST));

    pthread_mutex_init(&g_pstDialerHandle->mutexCallQueue, NULL);
    pthread_cond_init(&g_pstDialerHandle->condCallQueue, NULL);
    dos_list_init(&g_pstDialerHandle->stCallList);
    g_pstDialerHandle->blIsESLRunning = DOS_FALSE;
    g_pstDialerHandle->blIsWaitingExit = DOS_FALSE;

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/*
 * 函数: U32 sc_dialer_start()
 * 功能: 启动拨号模块
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_dialer_start()
{
    SC_TRACE_IN(0, 0, 0, 0);
    if (pthread_create(&g_pstDialerHandle->pthID, NULL, sc_dialer_runtime, NULL) < 0)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/*
 * 函数: U32 sc_dialer_shutdown()
 * 功能: 停止拨号模块
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_dialer_shutdown()
{
    SC_TRACE_IN(0, 0, 0, 0);

    pthread_mutex_lock(&g_pstDialerHandle->mutexCallQueue);
    g_pstDialerHandle->blIsWaitingExit = DOS_TRUE;
    pthread_mutex_unlock(&g_pstDialerHandle->mutexCallQueue);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


