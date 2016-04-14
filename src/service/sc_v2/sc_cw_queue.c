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
DLL_S               g_stCWQMngt;
pthread_t           g_pthCWQMngt;
pthread_mutex_t     g_mutexCWQMngt = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t      g_condCWQMngt  = PTHREAD_COND_INITIALIZER;

/*  定义线程标志 */
static BOOL         g_blCWQWaitingExit = DOS_FALSE;
static BOOL         g_blCWQRunning  = DOS_FALSE;

/*  */
/**
 * 内部函数，在坐席组表中查找坐席组
 * 参数:
 *      VOID *pParam : 参数，这里是一个U32的数据，表示坐席组ID
 *      DLL_NODE_S *pstNode : 当前节点
 * 返回值: 0标示找到一个节点，其他值标示没有找到
 */
static S32 sc_cwq_find_agentgrp(VOID *pParam, DLL_NODE_S *pstNode)
{
    SC_CWQ_NODE_ST *pstCWQNode = NULL;

    if (DOS_ADDR_INVALID(pParam) || DOS_ADDR_INVALID(pstNode) || DOS_ADDR_INVALID(pstNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCWQNode = pstNode->pHandle;

    if (*(U32 *)pParam != pstCWQNode->ulAgentGrpID)
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
U32 sc_cwq_add_call(SC_SRV_CB *pstSCB, U32 ulAgentGrpID, S8 *szCaller, BOOL bIsAddHead)
{
    SC_CWQ_NODE_ST *pstCWQNode = NULL;
    DLL_NODE_S     *pstDLLNode = NULL;
    SC_INCOMING_CALL_NODE_ST *pstCallNode = NULL;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(szCaller))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (0 == ulAgentGrpID || U32_BUTT == ulAgentGrpID)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstDLLNode = dll_find(&g_stCWQMngt, (VOID *)&ulAgentGrpID, sc_cwq_find_agentgrp);
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

        pthread_mutex_lock(&g_mutexCWQMngt);
        if (bIsAddHead)
        {
            DLL_Add_Head(&g_stCWQMngt, pstDLLNode);
        }
        else
        {
            DLL_Add(&g_stCWQMngt, pstDLLNode);
        }
        pthread_mutex_unlock(&g_mutexCWQMngt);
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
        pstCWQNode->ulAgentGrpID = ulAgentGrpID;
        pstCWQNode->ulStartWaitingTime = 0;
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
    dos_strncpy(pstCallNode->szCaller, szCaller, SC_NUM_LENGTH-1);
    pstCallNode->szCaller[SC_NUM_LENGTH-1] = '\0';

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
U32 sc_cwq_del_call(SC_SRV_CB *pstSCB)
{
    DLL_NODE_S                  *pstDLLNode         = NULL;
    DLL_NODE_S                  *pstDLLNode1        = NULL;
    SC_CWQ_NODE_ST              *pstCWQNode         = NULL;
    SC_INCOMING_CALL_NODE_ST    *pstIncomingNode    = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    DLL_Scan(&g_stCWQMngt, pstDLLNode, DLL_NODE_S*)
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

                dos_dmem_free(pstDLLNode1);
                pstDLLNode1= NULL;
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
    DLL_NODE_S              *pstDLLNode  = NULL;
    DLL_NODE_S              *pstDLLNode1 = NULL;
    SC_CWQ_NODE_ST          *pstCWQNode  = NULL;
    SC_SRV_CB               *pstSCB      = NULL;
    SC_MSG_EVT_LEAVE_CALLQUE_ST  stEvtLeaveCallque;
    SC_INCOMING_CALL_NODE_ST *pstCallNode = NULL;
    SC_AGENT_NODE_ST        *pstAgentNode = NULL;

    g_blCWQWaitingExit = DOS_FALSE;
    g_blCWQRunning  = DOS_TRUE;

    while (1)
    {
        if (g_blCWQWaitingExit)
        {
            break;
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

        DLL_Scan(&g_stCWQMngt, pstDLLNode, DLL_NODE_S*)
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

                pstCallNode = pstDLLNode1->pHandle;
                if (DOS_ADDR_INVALID(pstCallNode))
                {
                    DOS_ASSERT(0);
                    continue;
                }

                pstSCB = pstCallNode->pstSCB;
#if 0
                /* 至少一秒后，才接通坐席，避免等待音还没有播放，uuid_break不能将其停止。 */
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "now : %u, ulEnqueuTime : %u, ulDequeuTime : %u", time(NULL), pstSCB->stIncomingQueue.ulEnqueuTime, pstSCB->stIncomingQueue.ulDequeuTime);
                if (time(NULL) - pstSCB->stIncomingQueue.ulEnqueuTime < 3)
                {
                    continue;
                }
#endif
                /* 获取一个坐席 */
                pstAgentNode = sc_agent_select_by_grpid(pstCWQNode->ulAgentGrpID, pstCallNode->szCaller);
                if (DOS_ADDR_INVALID(pstAgentNode)
                    || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
                {
                    pstCWQNode->ulStartWaitingTime = time(0);

                    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "The group %u has no idel agent. scbNo(%u)", pstCWQNode->ulAgentGrpID, pstSCB->ulSCBNo);
                    break;
                }

                pstAgentNode->pstAgentInfo->bSelected = DOS_TRUE;

                dll_delete(&pstCWQNode->stCallWaitingQueue, pstDLLNode1);

                DLL_Init_Node(pstDLLNode1);
                dos_dmem_free(pstDLLNode1);
                pstDLLNode1 = NULL;
                dos_dmem_free(pstCallNode);
                pstCallNode = NULL;

                /* 发送消息，现在可以转坐席了 */
                stEvtLeaveCallque.stMsgTag.ulMsgType = SC_EVT_LEACE_CALL_QUEUE;
                stEvtLeaveCallque.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
                stEvtLeaveCallque.stMsgTag.usInterErr = SC_LEAVE_CALL_QUE_SUCC;
                stEvtLeaveCallque.ulSCBNo = pstSCB->ulSCBNo;
                stEvtLeaveCallque.pstAgentNode = pstAgentNode;

                sc_log(pstAgentNode->pstAgentInfo->bTraceON, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "The group %u send leave call queue. AgentID(%d), scbNo(%u)", pstCWQNode->ulAgentGrpID, pstAgentNode->pstAgentInfo, pstSCB->ulSCBNo);

                if (sc_send_event_leave_call_queue_rsp(&stEvtLeaveCallque) != DOS_SUCC)
                {
                    /* TODO 发送消息失败 */
                }
            }
            pthread_mutex_unlock(&pstCWQNode->mutexCWQMngt);
        }
    }

    g_blCWQRunning  = DOS_FALSE;

    return NULL;
}

U32 sc_cwq_init()
{
    DLL_Init(&g_stCWQMngt);

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


