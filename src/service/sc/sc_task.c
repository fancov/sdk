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

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <esl.h>

/* include private header files */
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"

/* define marcos */

/* define enums */

/* define structs */

extern U32                g_ulMaxConcurrency4Task;



/* declare functions */
inline U32 sc_random(U32 ulMax)
{
    if (!ulMax)
    {
        return U32_BUTT;
    }

    srand((unsigned)time( NULL ));

    return rand() % ulMax;
}

/*
 * 函数: SC_TEL_NUM_QUERY_NODE_ST *sc_task_get_callee(SC_TASK_CB_ST *pstTCB)
 * 功能: 获取被叫号码
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 返回值: 成功返回被叫号码控制块指针(已经出队列了，所以使用完之后要释放资源)，否则返回NULL
 * 调用该函数之后，如果返回了合法值，需要释放该资源
 */
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
        sc_logr_info(SC_TASK, "Load callee number for task %d. Load result : %d", pstTCB->ulTaskID, ulCount);
    }

    if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
    {
        pstTCB->ucTaskStatus = SC_TASK_STOP;

        sc_logr_info(SC_TASK, "Task %d has finished. or there is no callees.", pstTCB->ulTaskID);

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

    sc_logr_info(SC_TASK, "Select callee %s for new call.", pstCallee->szNumber);

    return pstCallee;
}

/*
 * 函数: SC_CALLER_QUERY_NODE_ST *sc_task_get_caller(SC_TASK_CB_ST *pstTCB)
 * 功能: 获取主叫号码，在主叫号码列表里面随即选择一个
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 返回值: 成功返回主叫号码控制块指针，否则返回NULL
 */
SC_CALLER_QUERY_NODE_ST *sc_task_get_caller(SC_TASK_CB_ST *pstTCB)
{
    U32                      ulCallerIndex = 0;
    S32                      lMaxSelectTime  = 0;
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
        lMaxSelectTime++;
        if (lMaxSelectTime > pstTCB->usCallerCount)
        {
            DOS_ASSERT(0);
            SC_TRACE_OUT();
            break;
        }

        ulCallerIndex = sc_random((U32)pstTCB->usCallerCount);
        if (ulCallerIndex >= SC_MAX_CALLER_NUM)
        {
            DOS_ASSERT(0);
            SC_TRACE_OUT();
            break;
        }

        if (!pstTCB->pstCallerNumQuery[ulCallerIndex].bValid)
        {
            continue;
        }

        pstCaller = &pstTCB->pstCallerNumQuery[ulCallerIndex];
        break;
    }

    if (pstCaller)
    {
        sc_logr_info(SC_TASK, "Select caller %s for new call", pstCaller->szNumber);
    }
    else
    {
        sc_logr_info(SC_TASK, "%s", "There is no caller for new call");
    }

    SC_TRACE_OUT();
    return pstCaller;
}

/*
 * 函数: U32 sc_task_make_call(SC_TASK_CB_ST *pstTCB)
 * 功能: 申请业务控制块，并将呼叫添加到拨号器模块，等待呼叫
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 返回值: 成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 sc_task_make_call(SC_TASK_CB_ST *pstTCB)
{
    SC_SCB_ST                 *pstSCB    = NULL;
    SC_TEL_NUM_QUERY_NODE_ST  *pstCallee = NULL;
    SC_CALLER_QUERY_NODE_ST   *pstCaller = NULL;

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

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

    pstSCB = sc_scb_alloc();
    if (!pstSCB)
    {
        sc_logr_notice(SC_TASK, "Make call for task %u fail. Alloc SCB fail.", pstTCB->ulTaskID);
        goto fail;
    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_INIT);

    if (pstTCB->bTraceCallON || pstSCB->bTraceNo
        || pstCallee->ucTraceON || pstCaller->bTraceON)
    {
        pstSCB->bTraceNo  = 1;
    }
    pstSCB->ulCustomID = pstTCB->ulCustomID;
    pstSCB->ulTaskID = pstTCB->ulTaskID;
    pstSCB->usTCBNo = pstTCB->usTCBNo;
    pstSCB->usSiteNo = U16_BUTT;
    pstSCB->ulTrunkID = U32_BUTT;
    pstSCB->ulCallDuration = 0;
    pstSCB->ucLegRole = SC_CALLER;

    dos_strncpy(pstSCB->szCallerNum, pstCaller->szNumber, SC_TEL_NUMBER_LENGTH);
    pstSCB->szCallerNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';
    dos_strncpy(pstSCB->szCalleeNum, pstCallee->szNumber, SC_TEL_NUMBER_LENGTH);
    pstSCB->szCalleeNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';
    pstSCB->szSiteNum[0] = '\0';
    pstSCB->szUUID[0] = '\0';

    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_AUTO_DIALING);
    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL);

    if (!sc_ep_black_list_check(pstSCB->ulCustomID, pstSCB->szCalleeNum))
    {
        DOS_ASSERT(0);
        sc_logr_info(SC_ESL, "Cannot make call for number %s which is in black list.", pstSCB->szCalleeNum);
        goto fail;
    }

    if (sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
    {
        sc_call_trace(pstSCB, "Make call failed.");

        //sc_task_callee_set_recall(pstTCB, pstCallee->ulIndex);
        goto fail;
    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_AUTH);

    sc_call_trace(pstSCB, "Make call for task %u successfully.", pstTCB->ulTaskID);

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

    if (pstSCB)
    {
        DOS_ASSERT(0);
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    return DOS_FAIL;
}

/*
 * 函数: VOID *sc_task_runtime(VOID *ptr)
 * 功能: 单个呼叫任务的线程主函数
 * 参数:
 */
VOID *sc_task_runtime(VOID *ptr)
{
    SC_TEL_NUM_QUERY_NODE_ST *pstCallee;
    SC_TASK_CB_ST   *pstTCB;
    list_t          *pstList;
    U32             ulTaskInterval = 0;

    if (!ptr)
    {
        sc_logr_error(SC_TASK, "%s", "Fail to start the thread for task, invalid parameter");
        pthread_exit(0);
    }

    pstTCB = (SC_TASK_CB_ST *)ptr;
    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_TASK, "%s", "Start task without pointer a TCB.");
        return NULL;
    }

    pstTCB->ucTaskStatus = SC_TASK_WORKING;
    while (1)
    {
        if (0 == ulTaskInterval)
        {
            ulTaskInterval = sc_task_get_call_interval(pstTCB);
        }
        usleep(ulTaskInterval * 1000);
        ulTaskInterval = 0;

        /* 根据当前呼叫量，确定发起呼叫的间隔，如果当前任务已经处于受限状态，就要强制调整间隔 */
        if (pstTCB->ulCurrentConcurrency >= pstTCB->ulMaxConcurrency)
        {
            sc_logr_info(SC_TASK, "Cannot make call for reach the max concurrency. Task : %u.", pstTCB->ulTaskID);
            continue;
        }

        /* 如果暂停了就继续等待 */
        if (SC_TASK_PAUSED == pstTCB->ucTaskStatus)
        {
            sc_logr_info(SC_TASK, "Cannot make call for puased status. Task : %u.", pstTCB->ulTaskID);
            ulTaskInterval = 1000;
            continue;
        }

        /* 如果被停止了，就检测还有没有呼叫，如果有呼叫，就等待，等待没有呼叫时退出任务 */
        if (SC_TASK_STOP == pstTCB->ucTaskStatus)
        {
            if (pstTCB->ulCurrentConcurrency != 0)
            {
                sc_logr_info(SC_TASK, "Cannot make call for stop status. Task : %u.", pstTCB->ulTaskID);
                ulTaskInterval = 1000;
                continue;
            }

            /* 任务结束了，退出主循环 */
            break;
        }

        /* 检查当前是否在允许的时间段 */
        if (sc_task_check_can_call_by_time(pstTCB) != DOS_TRUE)
        {
            sc_logr_info(SC_TASK, "Cannot make call for invalid time period. Task : %u.", pstTCB->ulTaskID);
            ulTaskInterval = 1000;
            continue;
        }

        /* 检测当时任务是否可以发起呼叫 */
        if (sc_task_check_can_call_by_status(pstTCB) != DOS_TRUE)
        {
            sc_logr_info(SC_TASK, "Cannot make call for system busy. Task : %u.", pstTCB->ulTaskID);
            continue;
        }

        /* 发起呼叫 */
        if (sc_task_make_call(pstTCB))
        {
            sc_logr_info(SC_TASK, "%s", "Make call fail.");
        }
    }

    sc_update_task_status(pstTCB->ulTaskID, pstTCB->ucTaskStatus);

    sc_logr_info(SC_TASK, "Task %d finished.", pstTCB->ulTaskID);

    /* 释放相关资源 */
    while (1)
    {
        if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
        {
            break;
        }

        pstList = dos_list_fetch(&pstTCB->stCalleeNumQuery);
        if (DOS_ADDR_INVALID(pstList))
        {
            break;
        }

        pstCallee = dos_list_entry(pstList, SC_TEL_NUM_QUERY_NODE_ST, stLink);
        if (DOS_ADDR_INVALID(pstCallee))
        {
            continue;
        }

        dos_dmem_free(pstCallee);
        pstCallee = NULL;
    }

    if (pstTCB->pstCallerNumQuery)
    {
        dos_dmem_free(pstTCB->pstCallerNumQuery);
        pstTCB->pstCallerNumQuery = NULL;
    }
    pthread_mutex_destroy(&pstTCB->mutexTaskList);

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_STOP, NULL);

    sc_tcb_free(pstTCB);
    pstTCB = NULL;

    return NULL;
}

/*
 * 函数: U32 sc_task_init(SC_TASK_CB_ST *pstTCB)
 * 功能: 初始化呼叫任务
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_task_init(SC_TASK_CB_ST *pstTCB)
{
    U32       ulIndex;
    U32       lRet;
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

    pstTCB->ulLastCalleeIndex = 0;

    lRet = sc_task_load_callee(pstTCB);
    if (lRet != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_TASK, "Load callee for task %d failed, Or there in no callee number.", pstTCB->ulTaskID);

        goto init_fail;
    }
    sc_logr_info(SC_TASK, "Task %d has been loaded %d callee(s).", pstTCB->ulTaskID, pstTCB->ulCalleeCount);

    lRet = sc_task_load_caller(pstTCB);
    if (lRet != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_TASK, "Load caller for task %d failed, Or there in no caller number.", pstTCB->ulTaskID);

        goto init_fail;
    }
    sc_logr_info(SC_TASK, "Task %d has been loaded %d caller(s).", pstTCB->ulTaskID, pstTCB->usCallerCount);

    if (sc_task_load_period(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_TASK, "Load period for task %d failed.", pstTCB->ulTaskID);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (sc_task_load_audio(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_TASK, "Load audio file for task %d FAILED.", pstTCB->ulTaskID);

        goto init_fail;
    }

    if (sc_task_load_agent_info(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_TASK, "Load agent info for task %d FAILED.", pstTCB->ulTaskID);

        goto init_fail;
    }

    if (sc_task_load_other_info(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_TASK, "Load agent info for task %d FAILED.", pstTCB->ulTaskID);

        goto init_fail;
    }

    if (SC_TASK_MODE_AUDIO_ONLY == pstTCB->ucMode)
    {
        pstTCB->ulMaxConcurrency = g_ulMaxConcurrency4Task;
    }
    else
    {
        pstTCB->ulMaxConcurrency = pstTCB->usSiteCount * SC_MAX_CALL_MULTIPLE;
    }



    sc_logr_notice(SC_TASK, "Load data for task %d finished.", pstTCB->ulTaskID);

    SC_TRACE_OUT();
    return DOS_SUCC;

 init_fail:
    if (pstTCB->pstCallerNumQuery)
    {
        dos_dmem_free(pstTCB->pstCallerNumQuery);
        pstTCB->pstCallerNumQuery = NULL;
    }

    SC_TRACE_OUT();
    return DOS_FAIL;
}

/*
 * 函数: U32 sc_task_start(SC_TASK_CB_ST *pstTCB)
 * 功能: 启动呼叫化任务
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
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

        sc_logr_notice(SC_TASK, "Start task %d faild", pstTCB->ulTaskID);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_START, NULL);

    sc_logr_notice(SC_TASK, "Start task %d finished.", pstTCB->ulTaskID);

    return DOS_SUCC;
}

/*
 * 函数: U32 sc_task_stop(SC_TASK_CB_ST *pstTCB)
 * 功能: 停止呼叫化任务
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
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

        sc_logr_info(SC_TASK, "Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_STOP, NULL);

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_STOP;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    return DOS_SUCC;
}

/*
 * 函数: U32 sc_task_continue(SC_TASK_CB_ST *pstTCB)
 * 功能: 恢复呼叫化任务
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
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

        sc_logr_info(SC_TASK, "Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_CONTINUE, NULL);

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_WORKING;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    return DOS_SUCC;
}

/*
 * 函数: U32 sc_task_pause(SC_TASK_CB_ST *pstTCB)
 * 功能: 暂停呼叫化任务
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
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

        sc_logr_info(SC_TASK, "Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_PAUSED, NULL);

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_PAUSED;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


