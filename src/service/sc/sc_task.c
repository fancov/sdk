/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名：sc_task.c
 *
 *  创建时间: 2014年12月16日10:23:53
 *  作    者: Larry
 *  描    述: 每一个群呼任务的实现
 *  修改历史:
 */
#if 0
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
#endif
/* include public header files */
#include <dos.h>

/* include private header files */
#include "sc_pub.h"
#include "sc_task_pub.h"
#include "sc_debug.h"
#include "sc_event_process.h"
#include "sc_ep_pub.h"

/* define marcos */

/* define enums */

/* define structs */

extern U32 g_ulTaskTraceAll;

/* declare functions */
inline U32 sc_random(U32 ulMax)
{

    srand((unsigned)time( NULL ));

    return rand() % ulMax;
}


SC_TEL_NUM_QUERY_NODE_ST *sc_task_get_callee(SC_TASK_CB_ST *pstTCB)
{
    SC_TEL_NUM_QUERY_NODE_ST *pstCallee = NULL;
    list_t                   *pstList = NULL;
    U32                      ulCount = 0;

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return NULL;
    }

    if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
    {
        ulCount = sc_task_load_callee(pstTCB);
        sc_logr_info("Load callee number for task %d. Load result : %d", pstTCB->ulTaskID, ulCount);
    }

    if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
    {
        pstTCB->ucTaskStatus = SC_TASK_STOP;

        sc_logr_info("Task %d has finished. or there is no callees.", pstTCB->ulTaskID);

        return NULL;
    }

    while (1)
    {
        if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
        {
            break;
        }

        pstList = dos_list_fetch(&pstTCB->stCalleeNumQuery);
        if (!pstList)
        {
            continue;
        }

        pstCallee = dos_list_entry(pstList, SC_TEL_NUM_QUERY_NODE_ST, stLink);
        if (!pstCallee)
        {
            continue;
        }

        break;
    }

    pstCallee->stLink.next = NULL;
    pstCallee->stLink.prev = NULL;

    sc_logr_info("Select callee %s for new call.", pstCallee->szNumber);

    return pstCallee;
}

SC_CALLER_QUERY_NODE_ST *sc_task_get_caller(SC_TASK_CB_ST *pstTCB)
{
    U32                      ulCallerIndex = 0;
    S32                      lMaxSelectTime  = 16;
    SC_CALLER_QUERY_NODE_ST  *pstCaller = NULL;

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return NULL;
    }

    while (1)
    {
        lMaxSelectTime--;
        if (lMaxSelectTime < 0)
        {
            DOS_ASSERT(0);
            SC_TRACE_OUT();
            break;
        }

        ulCallerIndex = sc_random((U32)pstTCB->usCallerCount);
        if (!pstTCB->pstCallerNumQuery[ulCallerIndex].bValid)
        {
            continue;
        }

        pstCaller = &pstTCB->pstCallerNumQuery[ulCallerIndex];
        break;
    }

    if (pstCaller)
    {
        sc_logr_info("Select caller %s for new call", pstCaller->szNumber);
    }
    else
    {
        sc_logr_info("%s", "There is no caller for new call");
    }

    SC_TRACE_OUT();
    return pstCaller;
}

U32 sc_task_make_call(SC_TASK_CB_ST *pstTCB)
{
    SC_CCB_ST                 *pstCCB    = NULL;
    SC_TEL_NUM_QUERY_NODE_ST  *pstCallee = NULL;
    SC_CALLER_QUERY_NODE_ST   *pstCaller = NULL;

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    pstCallee = sc_task_get_callee(pstTCB);
    if (!pstCallee)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstCaller = sc_task_get_caller(pstTCB);
    if (!pstCaller)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstCCB = sc_ccb_alloc();
    if (!pstCCB)
    {
        sc_logr_notice("%s", "Make call for task %d fail. Alloc CCB fail.");
        goto fail;
    }

//    SC_CCB_SET_STATUS(pstCCB, SC_CCB_INIT);

    pthread_mutex_lock(&pstCCB->mutexCCBLock);
    if (pstTCB->bTraceCallON || pstCCB->bTraceNo
        || pstCallee->ucTraceON || pstCaller->bTraceON)
    {
        pstCCB->bTraceNo  = 1;
    }
    pstCCB->ulCustomID = pstTCB->ulCustomID;
    pstCCB->ulTaskID = pstTCB->ulTaskID;
    pstCCB->usTCBNo = pstTCB->usTCBNo;
    pstCCB->usCallerNo = pstCaller->usNo;
    pstCCB->usSiteNo = U16_BUTT;
    pstCCB->ulTrunkID = U32_BUTT;
    pstCCB->ulAuthToken = U32_BUTT;
    pstCCB->ulCallDuration = 0;
    pstCCB->aucServiceType[0] = SC_SERV_OUTBOUND_CALL;
    pstCCB->aucServiceType[1] = SC_SERV_EXTERNAL_CALL;
    pstCCB->aucServiceType[2] = SC_SERV_AUTO_DIALING;

    dos_strncpy(pstCCB->szCallerNum, pstCaller->szNumber, SC_TEL_NUMBER_LENGTH);
    pstCCB->szCallerNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';
    dos_strncpy(pstCCB->szCalleeNum, pstCallee->szNumber, SC_TEL_NUMBER_LENGTH);
    pstCCB->szCalleeNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';
    pstCCB->szSiteNum[0] = '\0';
    pstCCB->szUUID[0] = '\0';

    pthread_mutex_unlock(&pstCCB->mutexCCBLock);

    if (sc_dialer_add_call(pstCCB) != DOS_SUCC)
    {
        sc_call_trace(pstCCB, "Make call failed.");

        sc_task_callee_set_recall(pstTCB, pstCallee->ulIndex);
        goto fail;
    }

    sc_call_trace(pstCCB, "Make call for task %d successfully.", pstTCB->ulTaskID);

    if (pstCallee)
    {
        dos_dmem_free(pstCallee);
        pstCallee = NULL;
    }

    return DOS_SUCC;

fail:
    if (pstCallee)
    {
        dos_dmem_free(pstCallee);
        pstCallee = NULL;
    }

    if (pstCCB)
    {
        sc_ccb_free(pstCCB);
        pstCCB = NULL;
    }

    return DOS_FAIL;
}

VOID *sc_task_runtime(VOID *ptr)
{
    SC_TASK_CB_ST   *pstTCB;
    U32             ulTaskInterval;
    U32             blIsNormal = DOS_TRUE;

    if (!ptr)
    {
        sc_logr_error("%s", "Fail to start the thread for task, invalid parameter");
        pthread_exit(0);
    }

    pstTCB = (SC_TASK_CB_ST *)ptr;

    while (1)
    {
        /* 根据当前呼叫量，确定发起呼叫的间隔，如果当前任务已经处于受限状态，就要强制调整间隔 */
        ulTaskInterval = sc_task_get_call_interval(pstTCB);
        if (!blIsNormal && ulTaskInterval <= 100)
        {
            ulTaskInterval = 1;

            /* 如果呼叫量已经为0就退出任务 */
            if (!pstTCB->ulConcurrency)
            {
                sc_task_trace(pstTCB, "%s", "Task will be finished.");
                sc_logr_notice("Task will be finished.(%lu)", pstTCB->ulTaskID);
                break;
            }
        }
        usleep(ulTaskInterval * 1000);

        /* 如果暂停了就继续等待 */
        if (SC_TASK_PAUSED == pstTCB->ucTaskStatus)
        {
            blIsNormal = DOS_FALSE;
            continue;
        }

        /* 如果被停止了，就检测还有没有呼叫，如果有呼叫，就等待，等待没有呼叫时退出任务 */
        if (SC_TASK_STOP == pstTCB->ucTaskStatus)
        {
            if (pstTCB->ulConcurrency >= 0)
            {
                blIsNormal = DOS_FALSE;
                continue;
            }
            break;
        }

        /* 检查当前是否在允许的时间段 */
        if (sc_task_check_can_call_by_time(pstTCB) != DOS_TRUE)
        {
            blIsNormal = DOS_FALSE;
            continue;
        }

        /* 检测当时任务是否可以发起呼叫 */
        if (sc_task_check_can_call_by_status(pstTCB) != DOS_TRUE)
        {
            blIsNormal = DOS_FALSE;
            continue;
        }

        /* 发起呼叫 */
        if (sc_task_make_call(pstTCB))
        {
            sc_logr_notice("%s", "Make call fail.");
        }
    }

    /* TODO: 释放相关资源 */

    return NULL;
}

U32 sc_task_init(SC_TASK_CB_ST *pstTCB)
{
    U32       ulIndex;
    S32       lCnt;
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (!pstTCB)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 先申请资源 */
    pstTCB->pstCallerNumQuery = (SC_CALLER_QUERY_NODE_ST *)dos_dmem_alloc(sizeof(SC_CALLER_QUERY_NODE_ST) * SC_MAX_CALLER_NUM);
    if (!pstTCB->pstCallerNumQuery )
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_memzero(pstTCB->pstCallerNumQuery, sizeof(SC_CALLER_QUERY_NODE_ST) * SC_MAX_CALLER_NUM);
    for (ulIndex=0; ulIndex<SC_MAX_CALLER_NUM; ulIndex++)
    {
        pstTCB->pstCallerNumQuery[ulIndex].usNo = ulIndex;
        pstTCB->pstCallerNumQuery[ulIndex].bValid = 0;
        pstTCB->pstCallerNumQuery[ulIndex].bTraceON = 0;
        pstTCB->pstCallerNumQuery[ulIndex].szNumber[0] = '\0';
        pstTCB->pstCallerNumQuery[ulIndex].ulIndexInDB = U32_BUTT;
    }

    pstTCB->pstSiteQuery = (SC_SITE_QUERY_NODE_ST *)dos_dmem_alloc(sizeof(SC_SITE_QUERY_NODE_ST) * SC_MAX_SITE_NUM);
    if (!pstTCB->pstSiteQuery)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstTCB->pstCallerNumQuery);
        pstTCB->pstCallerNumQuery = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_memzero(pstTCB->pstSiteQuery, sizeof(SC_SITE_QUERY_NODE_ST) * SC_MAX_SITE_NUM);
    for (ulIndex=0; ulIndex<SC_MAX_CALLER_NUM; ulIndex++)
    {
        pstTCB->pstSiteQuery[ulIndex].usSCBNo = ulIndex;
        pstTCB->pstSiteQuery[ulIndex].bValid = 0;
        pstTCB->pstSiteQuery[ulIndex].szEmpNo[0] = '\0';
        pstTCB->pstSiteQuery[ulIndex].szExtension[0] = '\0';
        pstTCB->pstSiteQuery[ulIndex].szUserID[0] = '\0';
        pstTCB->pstSiteQuery[ulIndex].ulSiteID = U32_BUTT;
        pstTCB->pstSiteQuery[ulIndex].ulStatus = SC_SITE_ACCOM_BUTT;
        pstTCB->pstSiteQuery[ulIndex].bAllowAccompanying = 0;
        pstTCB->pstSiteQuery[ulIndex].bTraceON = 0;
    }

    lCnt = sc_task_load_callee(pstTCB);
    if (lCnt <= 0)
    {
        DOS_ASSERT(0);
        sc_logr_error("Load callee for task %d failed, Or there in no callee number.", pstTCB->ulTaskID);

        goto init_fail;
    }
    sc_logr_info("Task %d has been loaded %d callee(s).", pstTCB->ulTaskID, lCnt);

    lCnt = sc_task_load_caller(pstTCB);
    if (lCnt <= 0)
    {
        DOS_ASSERT(0);
        sc_logr_error("Load caller for task %d failed, Or there in no caller number.", pstTCB->ulTaskID);

        goto init_fail;
    }
    sc_logr_info("Task %d has been loaded %d caller(s).", pstTCB->ulTaskID, lCnt);
    pstTCB->usCallerCount = (U16)lCnt;

    if (sc_task_load_period(pstTCB) <= 0)
    {
        DOS_ASSERT(0);
        sc_logr_error("Load period for task %d failed.", pstTCB->ulTaskID);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    lCnt = sc_task_load_site(pstTCB);
    if (sc_task_load_site(pstTCB) <= 0)
    {
        DOS_ASSERT(0);
        sc_logr_error("Load site queue for task %d failed, or there is no site queue.", pstTCB->ulTaskID);

        goto init_fail;
    }
    sc_logr_info("Task %d has been loaded %d site(s).", pstTCB->ulTaskID, lCnt);
    pstTCB->usSiteCount = (S16)lCnt;

    if (sc_task_load_audio(pstTCB) <= 0)
    {
        DOS_ASSERT(0);
        sc_logr_error("Load audio file for task %d failed, or the do not exist.", pstTCB->ulTaskID);

        goto init_fail;
    }

    sc_logr_notice("Load data for task %d finished.", pstTCB->ulTaskID);
    SC_TRACE_OUT();
    return DOS_SUCC;

 init_fail:
    if (pstTCB->pstCallerNumQuery)
    {
        dos_dmem_free(pstTCB->pstCallerNumQuery);
        pstTCB->pstCallerNumQuery = NULL;
    }

    if (pstTCB->pstSiteQuery)
    {
        dos_dmem_free(pstTCB->pstSiteQuery);
        pstTCB->pstSiteQuery = NULL;
    }

    SC_TRACE_OUT();
    return DOS_FAIL;
}

U32 sc_task_start(SC_TASK_CB_ST *pstTCB)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (!pstTCB
        || !pstTCB->ucValid)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (pthread_create(&pstTCB->pthID, NULL, sc_task_runtime, pstTCB) < 0)
    {
        DOS_ASSERT(0);

        sc_logr_notice("Start task %d faild", pstTCB->ulTaskID);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_logr_notice("Start task %d finished.", pstTCB->ulTaskID);

    return DOS_SUCC;
}

U32 sc_task_stop(SC_TASK_CB_ST *pstTCB)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);
    if (!pstTCB)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (!pstTCB->ucValid
        || pstTCB->ucTaskStatus != SC_TASK_WORKING)
    {
        DOS_ASSERT(0);

        sc_logr_info("Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_STOP;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    return DOS_SUCC;
}

U32 sc_task_continue(SC_TASK_CB_ST *pstTCB)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);
    if (!pstTCB)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (!pstTCB->ucValid
        || pstTCB->ucTaskStatus != SC_TASK_PAUSED)
    {
        DOS_ASSERT(0);

        sc_logr_info("Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }


    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_WORKING;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    return DOS_SUCC;
}

U32 sc_task_pause(SC_TASK_CB_ST *pstTCB)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);
    if (!pstTCB)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (!pstTCB->ucValid
        || pstTCB->ucTaskStatus != SC_TASK_WORKING)
    {
        DOS_ASSERT(0);

        sc_logr_info("Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_PAUSED;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    return DOS_SUCC;
}

#if 0
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
