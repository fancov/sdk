/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_cw_queue.c
 *
 *  创建时间: 2015年6月1日20:00:36
 *  作    者: Larry
 *  描    述: 实现呼叫等待队列
 *  修改历史:
 *
 * 等待队列管理策略
 * 1. 固定1秒钟检查一次，如果有可以呼叫的坐席，则接通坐席
 * 2. 如果有呼叫进入队列需要主动通知该模块
 * 3. 内存中的结构
 *  g_stCWQMngt
 *    | --- queue1 --- call1 --- call2 --- call3 --- call4 ...
 *    | --- queue2 --- call1 --- call2 --- call3 --- call4 ...
 *    | --- ......
 *  有全局变量g_stCWQMngt管理坐席组队列，每个坐席组中维护各个坐席组的呼叫队列
 *
 */


#include <dos.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_debug.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif /* __cplusplus */
#endif /* __cplusplus */

/* 定义管理各个呼叫等待队列的管理链表 */
pthread_t           g_pthCWQMngt;
pthread_mutex_t     g_mutexCWQMngt = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t      g_condCWQMngt  = PTHREAD_COND_INITIALIZER;

/*  定义线程标志 */
static BOOL         g_blCWQWaitingExit = DOS_FALSE;
static BOOL         g_blCWQRunning  = DOS_FALSE;

U32 sc_sw_agent_cwq_handle(SC_CWQ_NODE_ST *pstCWQNode, DLL_NODE_S *pstDLLNode);
U32 sc_sw_agentgrp_cwq_handle(SC_CWQ_NODE_ST *pstCWQNode, DLL_NODE_S *pstDLLNode);
U32 sc_sw_sip_cwq_handle(SC_CWQ_NODE_ST *pstCWQNode, DLL_NODE_S *pstDLLNode);
U32 sc_sw_tt_cwq_handle(SC_CWQ_NODE_ST *pstCWQNode, DLL_NODE_S *pstDLLNode);
U32 sc_sw_tel_cwq_handle(SC_CWQ_NODE_ST *pstCWQNode, DLL_NODE_S *pstDLLNode);

SC_CWQ_TABLE_ST g_pstSWCwqTable[] =
{
    {},
    {SC_SW_FORWARD_AGENT,       sc_sw_agent_cwq_handle},
    {SC_SW_FORWARD_AGENT_GROUP, sc_sw_agentgrp_cwq_handle},
    {SC_SW_FORWARD_SIP,         sc_sw_sip_cwq_handle},
    {SC_SW_FORWARD_TT,          sc_sw_tt_cwq_handle},
    {SC_SW_FORWARD_TEL,         sc_sw_tel_cwq_handle},
};


SC_CWQ_TABLE_ST *sc_sw_cwq_api_find(U8 ucForwardType)
{
    U32 ulIndex;

    for (ulIndex=0; ulIndex<(sizeof(g_pstSWCwqTable)/sizeof(SC_CWQ_TABLE_ST)); ulIndex++)
    {
        if (ucForwardType == g_pstSWCwqTable[ulIndex].ulForwardType
            && g_pstSWCwqTable[ulIndex].callback)
        {
            return &g_pstSWCwqTable[ulIndex];
        }
    }
    return NULL;
}


U32 sc_sw_agent_cwq_handle(SC_CWQ_NODE_ST *pstCWQNode, DLL_NODE_S *pstDLLNode)
{
    SC_INCOMING_CALL_NODE_ST *pstCallNode            = NULL;
    SC_SRV_CB                *pstSCB                 = NULL;
    SC_AGENT_NODE_ST         *pstAgentNode           = NULL;
    SC_MSG_EVT_LEAVE_CALLQUE_ST  stEvtLeaveCallque;
    U32  usInterErr                                  = SC_LEAVE_CALL_QUE_SUCC;
    U32  ulTimeNow;

    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallNode = pstDLLNode->pHandle;
    if (DOS_ADDR_INVALID(pstCallNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCB = pstCallNode->pstSCB;


    /* 获取一个坐席 */
    pstAgentNode = sc_agent_get_by_id(pstCWQNode->ulID);

    if (DOS_ADDR_INVALID(pstAgentNode) || pstAgentNode->pstAgentInfo->ucServStatus != SC_ACD_SERV_IDEL
        || pstAgentNode->pstAgentInfo->ucWorkStatus != SC_ACD_WORK_IDEL)
    {
        ulTimeNow = time(0);

        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "ulTimeNow: %d ... pstCWQNode->ulStartWaitingTime : %d", ulTimeNow, pstCWQNode->ulStartWaitingTime);

        if (ulTimeNow - pstCWQNode->ulStartWaitingTime <= SC_CWQ_QUEUE_TIMEOUT)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "There has no idel agent. scbNo(%u)", pstSCB->ulSCBNo);
            return DOS_FAIL;
        }
        else
        {
            usInterErr = SC_LEAVE_CALL_QUE_TIMEOUT;
        }
    }
    else
    {
        pstAgentNode->pstAgentInfo->bSelected = DOS_TRUE;
        stEvtLeaveCallque.pstAgentNode = pstAgentNode;
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "The agent %u send leave call queue.  scbNo(%u)", pstCWQNode->ulID, pstSCB->ulSCBNo);
    }

    dll_delete(&pstCWQNode->stCallWaitingQueue, pstDLLNode);

    DLL_Init_Node(pstDLLNode);
    dos_dmem_free(pstDLLNode);
    pstDLLNode = NULL;
    dos_dmem_free(pstCallNode);
    pstCallNode = NULL;

    /* 发送消息，现在可以转坐席了 */
    stEvtLeaveCallque.stMsgTag.ulMsgType = SC_EVT_LEACE_CALL_QUEUE;
    stEvtLeaveCallque.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stEvtLeaveCallque.stMsgTag.usInterErr = usInterErr;
    stEvtLeaveCallque.ulSCBNo = pstSCB->ulSCBNo;

    if (sc_send_event_leave_call_queue_rsp(&stEvtLeaveCallque) != DOS_SUCC)
    {
        /* 发送消息失败 */
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}


U32 sc_sw_agentgrp_cwq_handle(SC_CWQ_NODE_ST *pstCWQNode, DLL_NODE_S *pstDLLNode)
{
    SC_INCOMING_CALL_NODE_ST     *pstCallNode     = NULL;
    SC_SRV_CB                    *pstSCB          = NULL;
    SC_AGENT_NODE_ST             *pstAgentNode    = NULL;
    SC_MSG_EVT_LEAVE_CALLQUE_ST  stEvtLeaveCallque;
    U32                          ulTimeNow;
    S32                          usInterErr;

    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallNode = pstDLLNode->pHandle;
    if (DOS_ADDR_INVALID(pstCallNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCB = pstCallNode->pstSCB;
    usInterErr = SC_LEAVE_CALL_QUE_SUCC;

    /* 获取一个坐席 */
    pstAgentNode = sc_agent_select_by_grpid(pstCWQNode->ulID, pstCallNode->szCaller, pstCallNode->szCallee);
    if (DOS_ADDR_INVALID(pstAgentNode))
    {
        ulTimeNow = time(0);

        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "ulTimeNow: %d ... pstCWQNode->ulStartWaitingTime : %d", ulTimeNow, pstCWQNode->ulStartWaitingTime);

        if (ulTimeNow - pstCWQNode->ulStartWaitingTime <= SC_CWQ_QUEUE_TIMEOUT)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "The group %u has no idel agent. scbNo(%u)", pstCWQNode->ulID, pstSCB->ulSCBNo);
            return DOS_FAIL;
        }
        else
        {
            usInterErr = SC_LEAVE_CALL_QUE_TIMEOUT;
        }

    }
    else
    {
        pstAgentNode->pstAgentInfo->bSelected = DOS_TRUE;
        stEvtLeaveCallque.pstAgentNode = pstAgentNode;
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "The group %u send leave call queue. AgentID(%d), scbNo(%u)", pstCWQNode->ulID, pstAgentNode->pstAgentInfo, pstSCB->ulSCBNo);
    }

    dll_delete(&pstCWQNode->stCallWaitingQueue, pstDLLNode);

    DLL_Init_Node(pstDLLNode);
    dos_dmem_free(pstDLLNode);
    pstDLLNode = NULL;
    dos_dmem_free(pstCallNode);
    pstCallNode = NULL;

    /* 发送消息，现在可以转坐席了 */
    stEvtLeaveCallque.stMsgTag.ulMsgType = SC_EVT_LEACE_CALL_QUEUE;
    stEvtLeaveCallque.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stEvtLeaveCallque.stMsgTag.usInterErr = usInterErr;
    stEvtLeaveCallque.ulSCBNo = pstSCB->ulSCBNo;

    if (sc_send_event_leave_call_queue_rsp(&stEvtLeaveCallque) != DOS_SUCC)
    {
        /* 发送消息失败 */
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}


U32 sc_sw_sip_cwq_handle(SC_CWQ_NODE_ST *pstCWQNode, DLL_NODE_S *pstDLLNode)
{
    return DOS_SUCC;
}

U32 sc_sw_tt_cwq_handle(SC_CWQ_NODE_ST *pstCWQNode, DLL_NODE_S *pstDLLNode)
{
    return DOS_SUCC;
}

U32 sc_sw_tel_cwq_handle(SC_CWQ_NODE_ST *pstCWQNode, DLL_NODE_S *pstDLLNode)
{
    return DOS_SUCC;
}


/**
 * 内部函数，在坐席组表中查找坐席组
 * 参数:
 *      VOID *pParam : 参数，这里是一个U32的数据，表示坐席组ID
 *      DLL_NODE_S *pstNode : 当前节点
 * 返回值: 0标示找到一个节点，其他值标示没有找到
 */
static S32 sc_cwq_find_dll_node(VOID *pParam, DLL_NODE_S *pstNode)
{
    SC_CWQ_NODE_ST *pstCWQNode = NULL;

    if (DOS_ADDR_INVALID(pParam) || DOS_ADDR_INVALID(pstNode) || DOS_ADDR_INVALID(pstNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCWQNode = pstNode->pHandle;

    if (*(U32 *)pParam != pstCWQNode->ulID)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}


/**
 * 函数: sc_cwq_add_call
 * 功能: 将pstSCB所指向的呼叫添加到呼叫队列
 * 参数:
 *      SC_SCB_ST *pstSCB : 呼叫控制块
 *      U32 ulAgentGrpID : 坐席组ID
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 * !!! 如果ulAgentGrpID所指定的grp不存在，会重新创建
 * !!! 该函数任务 0 和 U32_BUTT 为非法ID
 */
U32 sc_cwq_add_call(SC_SRV_CB *pstSCB, U32 ulID, S8 *szCaller, S8 *szCallee, U8 ucForwardType, BOOL bIsAddHead)
{
    SC_CWQ_NODE_ST              *pstCWQNode     = NULL;
    DLL_NODE_S                  *pstDLLNode     = NULL;
    SC_INCOMING_CALL_NODE_ST    *pstCallNode    = NULL;
    SC_CWQ_TABLE_ST             *pstSWCWQNode   = NULL;
    DLL_S                       *pstCWQMngt     = NULL;

    if (DOS_ADDR_INVALID(pstSCB)
        || ucForwardType >= SC_SW_FORWARD_BUTT)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (0 == ulID || U32_BUTT == ulID)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstSWCWQNode = sc_sw_cwq_api_find(ucForwardType);
    if (DOS_ADDR_INVALID(pstSWCWQNode))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstCWQMngt = &pstSWCWQNode->stSWCwqList;

    pstDLLNode = dll_find(pstCWQMngt, (VOID *)&ulID, sc_cwq_find_dll_node);
    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        pstDLLNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
        DLL_Init_Node(pstDLLNode);
        pstDLLNode->pHandle = NULL;

        pthread_mutex_lock(&pstSWCWQNode->mutexSWCwqList);
        if (bIsAddHead)
        {
            DLL_Add_Head(pstCWQMngt, pstDLLNode);
        }
        else
        {
            DLL_Add(pstCWQMngt, pstDLLNode);
        }
        pthread_mutex_unlock(&pstSWCWQNode->mutexSWCwqList);
    }

    pstCWQNode = pstDLLNode->pHandle;
    if (DOS_ADDR_INVALID(pstCWQNode))
    {
        pstCWQNode = dos_dmem_alloc(sizeof(SC_CWQ_NODE_ST));
        if (DOS_ADDR_INVALID(pstCWQNode))
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }

        pstCWQNode->ulID                = ulID;
        pstCWQNode->ulForwardType       = ucForwardType;
        pstCWQNode->ulStartWaitingTime  = time(0);
        DLL_Init(&pstCWQNode->stCallWaitingQueue);
        pthread_mutex_init(&pstCWQNode->mutexCWQMngt, NULL);

        pstDLLNode->pHandle = pstCWQNode;
    }

    pstDLLNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    DLL_Init_Node(pstDLLNode);

    pstCallNode = (SC_INCOMING_CALL_NODE_ST *)dos_dmem_alloc(sizeof(SC_INCOMING_CALL_NODE_ST));
    if (DOS_ADDR_INVALID(pstCallNode))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstDLLNode);

        return DOS_FAIL;
    }

    pstCallNode->pstSCB = pstSCB;
    if (DOS_ADDR_VALID(szCaller))
    {
        dos_strncpy(pstCallNode->szCaller, szCaller, SC_NUM_LENGTH-1);
        pstCallNode->szCaller[SC_NUM_LENGTH-1] = '\0';
    }
    else
    {
        pstCallNode->szCaller[0] = '\0';
    }

    if (DOS_ADDR_VALID(szCallee))
    {
        dos_strncpy(pstCallNode->szCaller, szCallee, SC_NUM_LENGTH-1);
        pstCallNode->szCallee[SC_NUM_LENGTH-1] = '\0';
    }
    else
    {
        pstCallNode->szCallee[0] = '\0';
    }

    pstCallNode->ulForwardID = ulID;

    pstDLLNode->pHandle = pstCallNode;
    //pstSCB->ulInQueueTime = time(NULL);

    pthread_mutex_lock(&pstCWQNode->mutexCWQMngt);
    if (bIsAddHead)
    {
        DLL_Add_Head(&pstCWQNode->stCallWaitingQueue, pstDLLNode);
    }
    else
    {
        DLL_Add(&pstCWQNode->stCallWaitingQueue, pstDLLNode);
    }
    pthread_mutex_unlock(&pstCWQNode->mutexCWQMngt);

    pthread_mutex_lock(&g_mutexCWQMngt);
    pthread_cond_signal(&g_condCWQMngt);
    pthread_mutex_unlock(&g_mutexCWQMngt);

    return DOS_SUCC;
}

/**
 * 函数: sc_cwq_del_call
 * 功能: 将pstSCB所指向的呼叫从呼叫队列中删除掉
 * 参数:
 *      SC_SCB_ST *pstSCB : 呼叫控制块
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_cwq_del_call(SC_SRV_CB *pstSCB, U32 ulType)
{
    DLL_NODE_S                  *pstDLLNode         = NULL;
    DLL_NODE_S                  *pstDLLNode1        = NULL;
    SC_CWQ_NODE_ST              *pstCWQNode         = NULL;
    SC_INCOMING_CALL_NODE_ST    *pstIncomingNode    = NULL;

    if (DOS_ADDR_INVALID(pstSCB)
        || ulType >= SC_SW_FORWARD_BUTT)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    DLL_Scan(&g_pstSWCwqTable[ulType].stSWCwqList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            break;
        }

        pstCWQNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstCWQNode))
        {
            continue;
        }

        pthread_mutex_lock(&pstCWQNode->mutexCWQMngt);
        DLL_Scan(&pstCWQNode->stCallWaitingQueue, pstDLLNode1, DLL_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstDLLNode1))
            {
                break;
            }

            pstIncomingNode = pstDLLNode1->pHandle;
            if (DOS_ADDR_INVALID(pstIncomingNode))
            {
                continue;
            }

            if (pstIncomingNode->pstSCB == pstSCB)
            {
#if 0
                /* 出对了之后，计算等待时间 */
                pstSCB->ulInQueueTime = time(NULL) - pstSCB->ulInQueueTime;
#endif
                dll_delete(&pstCWQNode->stCallWaitingQueue, pstDLLNode1);
                DLL_Init_Node(pstDLLNode1);
                pstDLLNode1->pHandle = NULL;

                dos_dmem_free(pstDLLNode1);
                pstDLLNode1= NULL;

                dos_dmem_free(pstIncomingNode);
                pstIncomingNode = NULL;

                break;
            }
        }
        pthread_mutex_unlock(&pstCWQNode->mutexCWQMngt);
    }

    return DOS_SUCC;
}

VOID *sc_cwq_runtime(VOID *ptr)
{
    struct timespec         stTimeout;
    DLL_NODE_S              *pstDLLNode     = NULL;
    DLL_NODE_S              *pstDLLNode1    = NULL;
    SC_CWQ_NODE_ST          *pstCWQNode     = NULL;
    DLL_S                   *pstCWQMngt     = NULL;
    U32                     ulIndex         = 0;
    SC_PTHREAD_MSG_ST       *pstPthreadMsg  = NULL;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    pstPthreadMsg = sc_pthread_cb_alloc();
    if (DOS_ADDR_VALID(pstPthreadMsg))
    {
        pstPthreadMsg->ulPthID = pthread_self();
        pstPthreadMsg->func = sc_cwq_runtime;
        pstPthreadMsg->pParam = ptr;
        dos_strcpy(pstPthreadMsg->szName, "sc_cwq_runtime");
    }

    g_blCWQWaitingExit = DOS_FALSE;
    g_blCWQRunning  = DOS_TRUE;

    while (1)
    {
        if (g_blCWQWaitingExit)
        {
            break;
        }

        if (DOS_ADDR_VALID(pstPthreadMsg))
        {
            pstPthreadMsg->ulLastTime = time(NULL);
        }

        pthread_mutex_lock(&g_mutexCWQMngt);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condCWQMngt, &g_mutexCWQMngt, &stTimeout);
        pthread_mutex_unlock(&g_mutexCWQMngt);

        if (g_blCWQWaitingExit)
        {
            break;
        }

        for(ulIndex = 0; ulIndex < sizeof(g_pstSWCwqTable)/sizeof(SC_CWQ_TABLE_ST); ulIndex++)
        {
            pstCWQMngt = &g_pstSWCwqTable[ulIndex].stSWCwqList;
            DLL_Scan(pstCWQMngt, pstDLLNode, DLL_NODE_S*)
            {
                if (DOS_ADDR_INVALID(pstDLLNode))
                {
                    DOS_ASSERT(0);
                    break;
                }

                pstCWQNode = pstDLLNode->pHandle;
                if (DOS_ADDR_INVALID(pstCWQNode))
                {
                    DOS_ASSERT(0);
                    continue;
                }

                pthread_mutex_lock(&pstCWQNode->mutexCWQMngt);
                DLL_Scan(&pstCWQNode->stCallWaitingQueue, pstDLLNode1, DLL_NODE_S*)
                {
                    if (DOS_ADDR_INVALID(pstDLLNode1))
                    {
                        DOS_ASSERT(0);
                        break;
                    }

                    if (g_pstSWCwqTable[ulIndex].callback(pstCWQNode, pstDLLNode1) == DOS_SUCC)
                    {
                        break;
                    }

                }
                pthread_mutex_unlock(&pstCWQNode->mutexCWQMngt);
            }
        }
    }

    g_blCWQRunning  = DOS_FALSE;

    return NULL;
}

U32 sc_cwq_init()
{
    U32 ulIndex;

    //DLL_Init(&g_stCWQMngt);
    for (ulIndex = 0; ulIndex < sizeof(g_pstSWCwqTable)/sizeof(SC_CWQ_TABLE_ST); ulIndex++)
    {
        DLL_Init(&g_pstSWCwqTable[ulIndex].stSWCwqList);
        pthread_mutex_init(&g_pstSWCwqTable[ulIndex].mutexSWCwqList, NULL);
    }

    return DOS_SUCC;
}

U32 sc_cwq_start()
{
    if (g_blCWQRunning)
    {
        DOS_ASSERT(0);

        return DOS_SUCC;
    }

    if (pthread_create(&g_pthCWQMngt, NULL, sc_cwq_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_cwq_stop()
{
    g_blCWQWaitingExit = DOS_TRUE;

    return DOS_SUCC;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */


