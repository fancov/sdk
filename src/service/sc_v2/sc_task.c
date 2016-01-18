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
#include "sc_res.h"
#include "sc_debug.h"
#include "sc_db.h"
#include "bs_pub.h"
#include "sc_http_api.h"

/* define marcos */

/* define enums */

/* define structs */


/* 任务列表 refer to struct tagTaskCB*/
SC_TASK_CB           *g_pstTaskList  = NULL;
pthread_mutex_t      g_mutexTaskList = PTHREAD_MUTEX_INITIALIZER;

VOID sc_task_update_calledcnt(U64 ulArg)
{
    SC_DB_MSG_TAG_ST    *pstMsg     = NULL;
    SC_TASK_CB          *pstTCB     = NULL;
    S8                  szSQL[512]  = { 0 };

    pstTCB = (SC_TASK_CB *)ulArg;
    if (DOS_ADDR_INVALID(pstTCB))
    {
        return;
    }

    if (pstTCB->ulCalledCountLast == pstTCB->ulCalledCount)
    {
        return;
    }

    pstTCB->ulCalledCountLast = pstTCB->ulCalledCount;

    dos_snprintf(szSQL, sizeof(szSQL), "UPDATE tbl_calltask SET calledcnt=%u WHERE id=%u", pstTCB->ulCalledCount, pstTCB->ulTaskID);

    pstMsg = (SC_DB_MSG_TAG_ST *)dos_dmem_alloc(sizeof(SC_DB_MSG_TAG_ST));
    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        return;
    }
    pstMsg->ulMsgType = SC_MSG_SAVE_TASK_CALLED_COUNT;
    pstMsg->szData = dos_dmem_alloc(dos_strlen(szSQL) + 1);
    if (DOS_ADDR_INVALID(pstMsg->szData))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstMsg->szData);

        return;
    }

    dos_strcpy(pstMsg->szData, szSQL);

    sc_send_msg2db(pstMsg);

    return;
}


/*
 * 函数: SC_TEL_NUM_QUERY_NODE_ST *sc_task_get_callee(SC_TASK_CB_ST *pstTCB)
 * 功能: 获取被叫号码
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 返回值: 成功返回被叫号码控制块指针(已经出队列了，所以使用完之后要释放资源)，否则返回NULL
 * 调用该函数之后，如果返回了合法值，需要释放该资源
 */
SC_TEL_NUM_QUERY_NODE_ST *sc_task_get_callee(SC_TASK_CB *pstTCB)
{
    SC_TEL_NUM_QUERY_NODE_ST *pstCallee = NULL;
    list_t                   *pstList = NULL;
    U32                      ulCount = 0;

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
    {
        ulCount = sc_task_load_callee(pstTCB);
        sc_log(LOG_LEVEL_INFO, "Load callee number for task %d. Load result : %d", pstTCB->ulTaskID, ulCount);
    }

    if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
    {
        pstTCB->ucTaskStatus = SC_TASK_STOP;

        sc_log(LOG_LEVEL_INFO, "Task %d has finished. or there is no callees.", pstTCB->ulTaskID);

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

    sc_log(LOG_LEVEL_INFO, "Select callee %s for new call.", pstCallee->szNumber);

    return pstCallee;
}


/*
 * 函数: U32 sc_task_make_call(SC_TASK_CB_ST *pstTCB)
 * 功能: 申请业务控制块，并将呼叫添加到拨号器模块，等待呼叫
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 返回值: 成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 sc_task_make_call(SC_TASK_CB *pstTCB)
{
    SC_TEL_NUM_QUERY_NODE_ST  *pstCallee = NULL;
    S8  szCaller[SC_NUM_LENGTH]   = {0};

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstCallee = sc_task_get_callee(pstTCB);
    if (!pstCallee)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 只要取到了被叫号码，就应该加一 */
    pstTCB->ulCalledCount++;

    if (sc_get_number_by_callergrp(pstTCB->ulCallerGrpID, szCaller, SC_NUM_LENGTH) != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_NOTIC, "Get caller from caller group(%u) FAIL.", pstTCB->ulCallerGrpID);
        return DOS_FAIL;
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
    SC_TEL_NUM_QUERY_NODE_ST *pstCallee = NULL;
    SC_TASK_CB      *pstTCB        = NULL;
    list_t          *pstList       = NULL;
    U32             ulTaskInterval = 0;
    S32             lResult        = 0;
    U32             ulMinInterval  = 0;
    U32             blStopFlag     = DOS_FALSE;
    BOOL            blPauseFlag    = DOS_FALSE;

    if (!ptr)
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "Fail to start the thread for task, invalid parameter");
        pthread_exit(0);
    }

    pstTCB = (SC_TASK_CB *)ptr;
    if (DOS_ADDR_INVALID(pstTCB))
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "Start task without pointer a TCB.");
        return NULL;
    }

    pstTCB->ucTaskStatus = SC_TASK_WORKING;

    if (sc_serv_ctrl_check(pstTCB->ulCustomID
                                , BS_SERV_AUTO_DIALING
                                , SC_SRV_CTRL_ATTR_TASK_MODE
                                , pstTCB->ucMode
                                , SC_SRV_CTRL_ATTR_INVLID
                                , U32_BUTT))
    {
        sc_log(LOG_LEVEL_NOTIC, "Service not allow.(TaskID:%u) ", pstTCB->ulTaskID);

        goto finished;
    }

    /* 开启一个定时器，将已经呼叫过的号码数量，定时写进数据库中 */
    lResult = dos_tmr_start(&pstTCB->pstTmrHandle
                            , SC_TASK_UPDATE_DB_TIMER * 1000
                            , sc_task_update_calledcnt
                            , (U64)pstTCB
                            , TIMER_NORMAL_LOOP);
    if (lResult < 0)
    {
        sc_log(LOG_LEVEL_ERROR, "Start timer update task(%u) calledcnt FAIL", pstTCB->ulTaskID);
    }

    /*
       这个地方大概的意思是: 假设接续时长为5000ms，需要在这5000ms内部所有坐席需要的呼叫全部震起来，
       因此需要计算一个发起呼叫的时间间隔
       但是呼叫间隔不小于20ms
       至少1000ms检测一次
       如果是不需要坐席的呼叫，则以20CPS的数度发起(当然要现在最低并发数)
    */
    if (pstTCB->usSiteCount * pstTCB->ulCallRate)
    {
        ulMinInterval = 5000 / pstTCB->usSiteCount * pstTCB->ulCallRate;
        ulMinInterval = (ulMinInterval < 20) ? 20 : (ulMinInterval > 1000) ? 1000 : ulMinInterval;
    }
    else
    {
        ulMinInterval = 1000 / 20;
    }

    sc_log(LOG_LEVEL_INFO, "Start run task(%u), Min interval: %ums", pstTCB->ulTaskID, ulMinInterval);

    while (1)
    {
        if (0 == ulTaskInterval)
        {
            ulTaskInterval = ulMinInterval;
        }
        dos_task_delay(ulTaskInterval);
        ulTaskInterval = 0;

        if (!pstTCB->ucValid)
        {
            return NULL;
        }

        /* 根据当前呼叫量，确定发起呼叫的间隔，如果当前任务已经处于受限状态，就要强制调整间隔 */
        if (!sc_task_check_can_call(pstTCB))
        {
            /* 可能会非常快，就不要打印了 */
            /*sc_logr_debug(NULL, SC_TASK, "Cannot make call for reach the max concurrency. Task : %u.", pstTCB->ulTaskID);*/
            continue;
        }

        /* 如果暂停了就继续等待 */
        if (SC_TASK_PAUSED == pstTCB->ucTaskStatus)
        {
            /* 第一次 暂停 时，不结束任务，等待20s */
            if (pstTCB->ulCurrentConcurrency != 0 || !blPauseFlag)
            {
                blPauseFlag = DOS_TRUE;
                sc_log(LOG_LEVEL_DEBUG, "Cannot make call for paused status. Task : %u.", pstTCB->ulTaskID);
                ulTaskInterval = 20000;
                continue;
            }

            break;
        }
        blPauseFlag = DOS_FALSE;

        /* 如果被停止了，就检测还有没有呼叫，如果有呼叫，就等待，等待没有呼叫时退出任务 */
        if (SC_TASK_STOP == pstTCB->ucTaskStatus)
        {
            /* 第一次 SC_TASK_STOP 时，不结束任务，等待20s */
            if (pstTCB->ulCurrentConcurrency != 0 || !blStopFlag)
            {
                blStopFlag = DOS_TRUE;
                sc_log(LOG_LEVEL_DEBUG, "Cannot make call for stoped status. Task : %u, CurrentConcurrency : %u.", pstTCB->ulTaskID, pstTCB->ulCurrentConcurrency);
                ulTaskInterval = 20000;
                continue;
            }

            /* 任务结束了，退出主循环 */
            break;
        }
        blStopFlag = DOS_FALSE;

        /* 检查当前是否在允许的时间段 */
        if (sc_task_check_can_call_by_time(pstTCB) != DOS_TRUE)
        {
            sc_log(LOG_LEVEL_DEBUG, "Cannot make call for invalid time period. Task : %u. %d", pstTCB->ulTaskID, pstTCB->usTCBNo);
            ulTaskInterval = 20000;
            continue;
        }

        /* 检测当时任务是否可以发起呼叫 */
        if (sc_task_check_can_call_by_status(pstTCB) != DOS_TRUE)
        {
            sc_log(LOG_LEVEL_DEBUG, "Cannot make call for system busy. Task : %u.", pstTCB->ulTaskID);
            continue;
        }
#if 1
        /* 发起呼叫 */
        if (sc_task_make_call(pstTCB))
        {
            sc_log(LOG_LEVEL_DEBUG, "%s", "Make call fail.");
        }
#endif
    }

finished:
    sc_log(LOG_LEVEL_NOTIC, "Task %d finished.", pstTCB->ulTaskID);

    /* 释放相关资源 */
    if (DOS_ADDR_VALID(pstTCB->pstTmrHandle))
    {
        dos_tmr_stop(&pstTCB->pstTmrHandle);
        pstTCB->pstTmrHandle = NULL;
    }

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

#if 0
    if (pstTCB->pstCallerNumQuery)
    {
        dos_dmem_free(pstTCB->pstCallerNumQuery);
        pstTCB->pstCallerNumQuery = NULL;
    }
#endif

    pthread_mutex_destroy(&pstTCB->mutexTaskList);

    /* 群呼任务结束后，将呼叫的被叫号码数量，改为被叫号码的总数量 */
    pstTCB->bThreadRunning = DOS_FALSE;
    sc_task_update_calledcnt((U64)pstTCB);
    sc_task_save_status(pstTCB->ulTaskID, blStopFlag ? SC_TASK_STATUS_DB_STOP : SC_TASK_STATUS_DB_PAUSED, NULL);

    sc_tcb_free(pstTCB);
    pstTCB = NULL;

    return NULL;
}

/*
 * 函数: U32 sc_task_start(SC_TASK_CB_ST *pstTCB)
 * 功能: 启动呼叫化任务
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_task_start(SC_TASK_CB *pstTCB)
{
    if (!pstTCB
        || !pstTCB->ucValid)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (pstTCB->bThreadRunning)
    {
        sc_log(LOG_LEVEL_NOTIC, "Task %u already running.", pstTCB->ulTaskID);
    }
    else
    {
        if (pthread_create(&pstTCB->pthID, NULL, sc_task_runtime, pstTCB) < 0)
        {
            DOS_ASSERT(0);

            pstTCB->bThreadRunning = DOS_FALSE;

            sc_log(LOG_LEVEL_NOTIC, "Start task %d faild", pstTCB->ulTaskID);

            return DOS_FAIL;
        }

        pstTCB->bThreadRunning = DOS_TRUE;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_START, NULL);

    sc_log(LOG_LEVEL_NOTIC, "Start task %d finished.", pstTCB->ulTaskID);

    return DOS_SUCC;
}

/*
 * 函数: U32 sc_task_stop(SC_TASK_CB_ST *pstTCB)
 * 功能: 停止呼叫化任务
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_task_stop(SC_TASK_CB *pstTCB)
{
    if (!pstTCB)
    {
        DOS_ASSERT(0);


        return DOS_FAIL;
    }

    if (!pstTCB->ucValid)
    {
        DOS_ASSERT(0);

        sc_log(LOG_LEVEL_ERROR, "Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

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
U32 sc_task_continue(SC_TASK_CB *pstTCB)
{
    if (!pstTCB)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (!pstTCB->ucValid
        || (pstTCB->ucTaskStatus != SC_TASK_PAUSED && pstTCB->ucTaskStatus != SC_TASK_STOP))
    {
        sc_log(LOG_LEVEL_ERROR, "Cannot continue the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_WORKING;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    /* 开始任务 */
    return sc_task_start(pstTCB);
}

/*
 * 函数: U32 sc_task_pause(SC_TASK_CB_ST *pstTCB)
 * 功能: 暂停呼叫化任务
 * 参数:
 *      SC_TASK_CB_ST *pstTCB: 任务控制块
 * 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_task_pause(SC_TASK_CB *pstTCB)
{
    if (!pstTCB)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (!pstTCB->ucValid
        || pstTCB->ucTaskStatus != SC_TASK_WORKING)
    {
        sc_log(LOG_LEVEL_ERROR, "Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        return DOS_FAIL;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_PAUSED, NULL);

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_PAUSED;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    return DOS_SUCC;
}

/*
 * 函数: U32 sc_task_mngt_continue_task(U32 ulTaskID, U32 ulCustomID)
 * 功能: 启动一个已经暂停的任务
 * 参数:
 *      U32 ulTaskID   : 任务ID
 *      U32 ulCustomID : 任务所属客户ID
 * 返回值
 *      返回HTTP API错误码
 * 该函数切勿阻塞
 **/
U32 sc_task_mngt_continue_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB *pstTCB = NULL;
    U32 ulRet = DOS_FAIL;

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        /* 暂停的任务不一定加载了，如果找不到应该重新加载 */
        if (sc_task_load(ulTaskID) != DOS_SUCC)
        {
            DOS_ASSERT(0);
            return SC_HTTP_ERRNO_INVALID_DATA;
        }

        pstTCB = sc_tcb_find_by_taskid(ulTaskID);
        if (!pstTCB)
        {
            DOS_ASSERT(0);
            return SC_HTTP_ERRNO_INVALID_DATA;
        }

        ulRet = sc_task_load_callee(pstTCB);
        if (DOS_SUCC != ulRet)
        {
            sc_log(LOG_LEVEL_ERROR, "SC Task Load Callee FAIL.(TaskID:%u, usNo:%u)", ulTaskID, pstTCB->usTCBNo);
            DOS_ASSERT(0);
            return SC_HTTP_ERRNO_INVALID_DATA;
        }
        sc_log(LOG_LEVEL_ERROR, "SC Task Load callee SUCC.(TaskID:%u, usNo:%u)", ulTaskID, pstTCB->usTCBNo);
    }

    if (pstTCB->ucTaskStatus == SC_TASK_INIT)
    {
        /* 需要加载任务 */
        if (DOS_SUCC != sc_task_load(ulTaskID))
        {
            DOS_ASSERT(0);

            return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
        }
    }

    /* 获取呼叫任务最大并发数 */
    pstTCB->usSiteCount = sc_acd_get_agent_cnt_by_grp(pstTCB->ulAgentQueueID);
    pstTCB->ulMaxConcurrency = pstTCB->usSiteCount * pstTCB->ulCallRate;
    if (0 == pstTCB->ulMaxConcurrency)
    {
        pstTCB->ulMaxConcurrency = SC_MAX_TASK_MAX_CONCURRENCY;
    }

    if (pstTCB->ucTaskStatus != SC_TASK_PAUSED
        && pstTCB->ucTaskStatus != SC_TASK_STOP)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_TASK_STATUS;
    }

    sc_task_continue(pstTCB);

    return SC_HTTP_ERRNO_SUCC;
}


/*
 * 函数: U32 sc_task_mngt_start_task(U32 ulTaskID, U32 ulCustomID)
 * 功能: 暂停一个在运行的任务
 * 参数:
 *      U32 ulTaskID   : 任务ID
 *      U32 ulCustomID : 任务所属客户ID
 * 返回值
 *      返回HTTP API错误码
 * 该函数切勿阻塞
 **/
U32 sc_task_mngt_pause_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB *pstTCB = NULL;

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (pstTCB->ucTaskStatus != SC_TASK_WORKING)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_TASK_STATUS;
    }

    sc_task_pause(pstTCB);

    return SC_HTTP_ERRNO_SUCC;

}

U32 sc_task_mngt_delete_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB *pstTCB = NULL;

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    sc_task_stop(pstTCB);
    return SC_HTTP_ERRNO_SUCC;

}


/*
 * 函数: U32 sc_task_mngt_start_task(U32 ulTaskID, U32 ulCustomID)
 * 功能: 启动一个新的任务
 * 参数:
 *      U32 ulTaskID   : 任务ID
 *      U32 ulCustomID : 任务所属客户ID
 * 返回值
 *      返回HTTP API错误码
 * 该函数切勿阻塞
 **/
U32 sc_task_mngt_start_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB *pstTCB = NULL;

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    if (DOS_SUCC != sc_task_load(ulTaskID))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 获取呼叫任务最大并发数 */
    pstTCB->usSiteCount = sc_acd_get_agent_cnt_by_grp(pstTCB->ulAgentQueueID);
    pstTCB->ulMaxConcurrency = pstTCB->usSiteCount * pstTCB->ulCallRate;
    if (0 == pstTCB->ulMaxConcurrency)
    {
        pstTCB->ulMaxConcurrency = SC_MAX_TASK_MAX_CONCURRENCY;
    }

    if (sc_task_start(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;
}


/*
 * 函数: U32 sc_task_mngt_stop_task(U32 ulTaskID, U32 ulCustomID)
 * 功能: 停止一个在运行的任务
 * 参数:
 *      U32 ulTaskID   : 任务ID
 *      U32 ulCustomID : 任务所属客户ID
 * 返回值
 *      返回HTTP API错误码
 * 该函数切勿阻塞
 **/
U32 sc_task_mngt_stop_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB *pstTCB = NULL;

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (pstTCB->ucTaskStatus == SC_TASK_STOP)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_TASK_STATUS;
    }

    sc_task_stop(pstTCB);

    return SC_HTTP_ERRNO_SUCC;
}

/*
 * 函数: VOID sc_task_mngt_cmd_process(SC_TASK_CTRL_CMD_ST *pstCMD)
 * 功能: 处理任务控制，呼叫控制API
 * 参数:
 *      SC_TASK_CTRL_CMD_ST *pstCMD: API命令数据
 * 返回值
 * 该函数切勿阻塞
 **/
U32 sc_task_mngt_cmd_proc(U32 ulAction, U32 ulCustomerID, U32 ulTaskID)
{
    U32 ulRet = SC_HTTP_ERRNO_INVALID_REQUEST;

    sc_log(LOG_LEVEL_INFO, "Process CMD, Action:%u, Task: %u, CustomID: %u"
                    , ulAction, ulTaskID, ulCustomerID);

    switch (ulAction)
    {
        case SC_API_CMD_ACTION_ADD:
        {
            /* DO Nothing */
            ulRet = SC_HTTP_ERRNO_SUCC;
            break;
        }
        case SC_API_CMD_ACTION_UPDATE:
        {
            /* 如果任务没有被加载到内存，就不要更新了 */
            if (sc_tcb_find_by_taskid(ulTaskID))
            {
                if (sc_task_and_callee_load(ulTaskID) != DOS_SUCC)
                {
                    ulRet = SC_HTTP_ERRNO_INVALID_DATA;
                }

                ulRet = SC_HTTP_ERRNO_SUCC;
            }
            else
            {
                ulRet = SC_HTTP_ERRNO_SUCC;
            }
            break;
        }
        case SC_API_CMD_ACTION_START:
        {
            ulRet = sc_task_mngt_start_task(ulTaskID, ulCustomerID);
            break;
        }
        case SC_API_CMD_ACTION_STOP:
        {
            ulRet = sc_task_mngt_stop_task(ulTaskID, ulCustomerID);
            break;
        }
        case SC_API_CMD_ACTION_CONTINUE:
        {
            ulRet = sc_task_mngt_continue_task(ulTaskID, ulCustomerID);
            break;
        }
        case SC_API_CMD_ACTION_PAUSE:
        {
            ulRet = sc_task_mngt_pause_task(ulTaskID, ulCustomerID);
            break;
        }
        case SC_API_CMD_ACTION_DELETE:
        {
            ulRet = sc_task_mngt_delete_task(ulTaskID, ulCustomerID);
            break;
        }
        default:
        {
            sc_log(LOG_LEVEL_NOTIC, "Action templately not support. ACTION: %d", ulAction, ulAction);
            break;
        }
    }

    sc_log(LOG_LEVEL_DEBUG, "CMD Process finished. Action: %u, ErrCode:%u"
                    , ulAction, ulRet);

    return ulRet;
}


/*
 * 函数: U32 sc_task_mngt_start()
 * 功能: 启动呼叫控制模块，同时启动已经被加载的呼叫任务
 * 参数:
 * 返回值: 成功返回DOS_SUCC， 失败返回DOS_FAIL
 **/
U32 sc_task_mngt_start()
{
    SC_TASK_CB    *pstTCB = NULL;
    U32              ulIndex;

    for (ulIndex = 0; ulIndex < SC_MAX_TASK_NUM; ulIndex++)
    {
        pstTCB = &g_pstTaskList[ulIndex];

        if (pstTCB->ucValid && SC_TCB_HAS_VALID_OWNER(pstTCB))
        {
            if (DOS_SUCC != sc_task_and_callee_load(pstTCB->ulTaskID))
            {
                sc_log(LOG_LEVEL_NOTIC, "Task init fail. Custom ID: %d, Task ID: %d", pstTCB->ulCustomID, pstTCB->ulTaskID);
                sc_tcb_free(pstTCB);
                continue;
            }

            /* 获取呼叫任务最大并发数 */
            pstTCB->usSiteCount = sc_acd_get_agent_cnt_by_grp(pstTCB->ulAgentQueueID);
            pstTCB->ulMaxConcurrency = pstTCB->usSiteCount * pstTCB->ulCallRate;
            if (0 == pstTCB->ulMaxConcurrency)
            {
                pstTCB->ulMaxConcurrency = SC_MAX_TASK_MAX_CONCURRENCY;
            }

            if (pstTCB->ucTaskStatus != SC_TASK_WORKING)
            {
                continue;
            }

            if (sc_task_start(pstTCB) != DOS_SUCC)
            {
                sc_log(LOG_LEVEL_NOTIC, "Task start fail. Custom ID: %d, Task ID: %d", pstTCB->ulCustomID, pstTCB->ulTaskID);

                sc_tcb_free(pstTCB);
                continue;
            }
        }
    }

    sc_log(LOG_LEVEL_INFO, "Start call task mngt service finished.");

    return DOS_SUCC;
}


/*
 * 函数: U32 sc_task_mngt_init()
 * 功能: 初始化呼叫管理模块
 * 参数:
 * 返回值: 成功返回DOS_SUCC， 失败返回DOS_FAIL
 **/
U32 sc_task_mngt_init()
{
    SC_TASK_CB      *pstTCB = NULL;
    U32             ulIndex = 0;

    /* 初始化呼叫控制块和任务控制块 */
    g_pstTaskList = (SC_TASK_CB *)dos_smem_alloc(sizeof(SC_TASK_CB) * SC_MAX_TASK_NUM);
    if (!g_pstTaskList)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    dos_memzero(g_pstTaskList, sizeof(SC_TASK_CB) * SC_MAX_TASK_NUM);
    for (ulIndex = 0; ulIndex < SC_MAX_TASK_NUM; ulIndex++)
    {
        pstTCB = &g_pstTaskList[ulIndex];
        pstTCB->usTCBNo = ulIndex;
        pthread_mutex_init(&pstTCB->mutexTaskList, NULL);
        pstTCB->ucTaskStatus = SC_TASK_BUTT;
        pstTCB->ulTaskID = U32_BUTT;
        pstTCB->ulCustomID = U32_BUTT;

        dos_list_init(&pstTCB->stCalleeNumQuery);
    }

    /* 加载群呼任务 */
    if (sc_task_mngt_load_task() != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_ERROR, "Load call task fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/*
 * 函数: U32 sc_task_mngt_shutdown()
 * 功能: 停止任务管理模块
 * 参数:
 * 返回值: 成功返回DOS_SUCC， 失败返回DOS_FAIL
 **/
U32 sc_task_mngt_stop()
{
    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


