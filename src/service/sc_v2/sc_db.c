/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名
 *
 *  创建时间:
 *  作    者:
 *  描    述:
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <sc_pub.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_db.h"

#define SC_MAX_SQL_LEN  1024

static BOOL             g_blExitFlag = DOS_TRUE;
DLL_S            g_stDBRequestQueue;
static pthread_t        g_pthreadDB;
static pthread_mutex_t  g_mutexDBRequestQueue = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   g_condDBRequestQueue  = PTHREAD_COND_INITIALIZER;

extern DB_HANDLE_ST         *g_pstSCDBHandle;

static U32 sc_db_save_call_result(SC_DB_MSG_TAG_ST *pstMsg)
{
    SC_DB_MSG_CALL_RESULT_ST *pstCallResult;
    S8                       szSQL[SC_MAX_SQL_LEN];

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallResult = (SC_DB_MSG_CALL_RESULT_ST *)pstMsg;

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_DB_WQ), "Save call result. Customer:%u, Task: %u, Agent:%s(%u), Caller:%s, Callee:%s, TerminateCause: %u, Result: %u"
                    , pstCallResult->ulCustomerID, pstCallResult->ulTaskID, pstCallResult->szAgentNum, pstCallResult->ulAgentID
                    , pstCallResult->szCaller, pstCallResult->szCallee, pstCallResult->usTerminateCause, pstCallResult->ulResult);

    dos_snprintf(szSQL, sizeof(szSQL),
                    "INSERT INTO tbl_calltask_result(`id`,`customer_id`,`task_id`,`caller`,`callee`,`agent_num`,`pdd_len`,"
                    "`ring_times`,`answer_time`,`ivr_end_time`,`dtmf_time`,`wait_agent_times`,`time_len`,"
                    "`hold_cnt`,`hold_times`,`release_part`,`terminate_cause`,`result`, `agent_id`) VALUES(NULL, %u, %u, "
                    "\"%s\", \"%s\", \"%s\", %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u)"
                  , pstCallResult->ulCustomerID, pstCallResult->ulTaskID, pstCallResult->szCaller
                  , pstCallResult->szCallee, pstCallResult->szAgentNum, pstCallResult->ulPDDLen
                  , pstCallResult->ulRingTime, pstCallResult->ulAnswerTimeStamp, pstCallResult->ulIVRFinishTime
                  , pstCallResult->ulFirstDTMFTime, pstCallResult->ulWaitAgentTime, pstCallResult->ulTimeLen
                  , pstCallResult->ulHoldCnt, pstCallResult->ulHoldTimeLen, pstCallResult->ucReleasePart
                  , pstCallResult->usTerminateCause, pstCallResult->ulResult, pstCallResult->ulAgentID);

    if (db_query(g_pstSCDBHandle, szSQL, NULL, NULL, 0) != DOS_SUCC)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DB_WQ), "%s", "Save call result FAIL.");

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

S32 sc_is_exit_in_db_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    U32 *pulCount = NULL;

    if (DOS_ADDR_INVALID(pArg)
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pulCount = (U32 *)pArg;
    if (dos_atoul(aszValues[0], pulCount) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

static U32 sc_db_save_task_status(SC_DB_MSG_TAG_ST *pstMsg)
{
    SC_DB_MSG_TASK_STATUS_ST *pstTaskStatus         = NULL;
    S8                       szSQL[SC_MAX_SQL_LEN]  = {0,};
    S32                      lRet                   = DOS_FAIL;
    U32                      ulCount                = 0;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstTaskStatus = (SC_DB_MSG_TASK_STATUS_ST *)pstMsg;

    dos_snprintf(szSQL, sizeof(szSQL), "SELECT COUNT(id) FROM tmp_task_status WHERE task_id=%u;", pstTaskStatus->ulTaskID);
    lRet = db_query(g_pstSCDBHandle, szSQL, sc_is_exit_in_db_cb, &ulCount, NULL);
    if (DB_ERR_SUCC != lRet)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DB_WQ), "Get numbers of task FAIL.(task ID:%u)", pstTaskStatus->ulTaskID);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 判断是否存在 */
    if (0 == ulCount)
    {
        dos_snprintf(szSQL, sizeof(szSQL),
                    "INSERT INTO tmp_task_status(`id`,`task_id`,`max_concurrent`,`cur_concurrent`,`total_agent`,`idel_agent`,`called_count`)"
                    "VALUES(NULL, %u, %u, %u, %u, %u, %u)"
                    , pstTaskStatus->ulTaskID
                    , pstTaskStatus->ulMaxConcurrency
                    , pstTaskStatus->ulCurrentConcurrency
                    , pstTaskStatus->ulTotalAgent
                    , pstTaskStatus->ulIdleAgent
                    , pstTaskStatus->ulCalledCount);
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "UPDATE tmp_task_status SET max_concurrent=%u, cur_concurrent=%u, total_agent=%u, idel_agent=%u, called_count=%u WHERE task_id=%u"
                    , pstTaskStatus->ulMaxConcurrency
                    , pstTaskStatus->ulCurrentConcurrency
                    , pstTaskStatus->ulTotalAgent
                    , pstTaskStatus->ulIdleAgent
                    , pstTaskStatus->ulCalledCount
                    , pstTaskStatus->ulTaskID);
    }

    if (db_query(g_pstSCDBHandle, szSQL, NULL, NULL, 0) != DOS_SUCC)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DB_WQ), "%s", "Save call task status FAIL.");

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static U32 sc_db_save_agent_status(SC_DB_MSG_TAG_ST *pstMsg)
{
    SC_DB_MSG_AGENT_STATUS_ST   *pstAgentStatus        = NULL;
    S8                          szSQL[SC_MAX_SQL_LEN]  = {0,};
    S32                         lRet                   = DOS_FAIL;
    U32                         ulCount                = 0;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstAgentStatus = (SC_DB_MSG_AGENT_STATUS_ST *)pstMsg;

    dos_snprintf(szSQL, sizeof(szSQL), "SELECT COUNT(id) FROM tmp_agent_status WHERE agent_id=%u;", pstAgentStatus->ulAgentID);
    lRet = db_query(g_pstSCDBHandle, szSQL, sc_is_exit_in_db_cb, &ulCount, NULL);
    if (DB_ERR_SUCC != lRet)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DB_WQ), "Get numbers of task FAIL.(agent ID:%u)", pstAgentStatus->ulAgentID);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 判断是否存在 */
    if (0 == ulCount)
    {
        dos_snprintf(szSQL, sizeof(szSQL),
                    "INSERT INTO tmp_agent_status(`id`,`agent_id`,`work_status`,`server_status`,`singin`,`interception`,`whisper`, `job_number`)"
                    "VALUES(NULL, %u, %u, %u, %u, %u, %u, \"%s\")"
                    , pstAgentStatus->ulAgentID
                    , pstAgentStatus->ulWorkStatus
                    , pstAgentStatus->ulServStatus
                    , pstAgentStatus->bIsSigin
                    , pstAgentStatus->bIsInterception
                    , pstAgentStatus->bIsWhisper
                    , pstAgentStatus->szEmpNo);
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "UPDATE tmp_agent_status SET work_status=%u, server_status=%u, singin=%u, interception=%u, whisper=%u, job_number=\"%s\" WHERE agent_id=%u"
                    , pstAgentStatus->ulWorkStatus
                    , pstAgentStatus->ulServStatus
                    , pstAgentStatus->bIsSigin
                    , pstAgentStatus->bIsInterception
                    , pstAgentStatus->bIsWhisper
                    , pstAgentStatus->szEmpNo
                    , pstAgentStatus->ulAgentID);
    }

    if (db_query(g_pstSCDBHandle, szSQL, NULL, NULL, 0) != DOS_SUCC)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DB_WQ), "%s", "Save call agent status FAIL.");

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static U32 sc_db_execute_sql(SC_DB_MSG_TAG_ST *pstMsg)
{
    if (DOS_ADDR_INVALID(pstMsg)
        || DOS_ADDR_INVALID(pstMsg->szData))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (db_query(g_pstSCDBHandle, pstMsg->szData, NULL, NULL, 0) != DOS_SUCC)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DB_WQ), "Execute sql FAIL. %s", pstMsg->szData);

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static VOID sc_db_request_proc(SC_DB_MSG_TAG_ST *pstMsg)
{
    U32 ulResult;
    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return;
    }

    switch (pstMsg->ulMsgType)
    {
        case SC_MSG_SAVE_CALL_RESULT:
            ulResult = sc_db_save_call_result(pstMsg);
            break;
        case SC_MSG_SAVE_AGENT_STATUS:
            ulResult = sc_db_save_agent_status(pstMsg);
            break;
        case SC_MSG_SAVE_TASK_CALLED_COUNT:
        case SC_MSG_SAVE_SIP_IPADDR:
        case SC_MSG_SAVE_TRUNK_STATUS:
        case SC_MSG_SAVE_STAT_LIMIT_CALLER:
            ulResult = sc_db_execute_sql(pstMsg);
            break;
        case SC_MSG_SACE_TASK_STATUS:
            ulResult = sc_db_save_task_status(pstMsg);
            break;
        default:
            sc_log(DOS_FALSE, LOG_LEVEL_WARNING, "Invalid msg type(%u)", pstMsg->ulMsgType);
            break;
    }

    if (DOS_ADDR_VALID(pstMsg->szData))
    {
        dos_dmem_free(pstMsg->szData);
        pstMsg->szData = NULL;
    }
    dos_dmem_free(pstMsg);
    pstMsg = NULL;
}

static VOID *sc_db_runtime(VOID *ptr)
{
    DLL_NODE_S       *pstDLLNode = NULL;
    SC_DB_MSG_TAG_ST *pstMsg     = NULL;
    SC_PTHREAD_MSG_ST   *pstPthreadMsg = NULL;
    struct timespec stTimeout;

    pstPthreadMsg = dos_pthread_cb_alloc();
    if (DOS_ADDR_VALID(pstPthreadMsg))
    {
        pstPthreadMsg->ulPthID = pthread_self();
        pstPthreadMsg->func = sc_db_runtime;
        pstPthreadMsg->pParam = ptr;
        dos_strcpy(pstPthreadMsg->szName, "sc_db_runtime");
    }

    g_blExitFlag = DOS_FALSE;

    while (1)
    {
        if (g_blExitFlag)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_DB_WQ), "%s", "Request exit.");
            break;
        }

        if (DOS_ADDR_VALID(pstPthreadMsg))
        {
            pstPthreadMsg->ulLastTime = time(NULL);
        }

        pthread_mutex_lock(&g_mutexDBRequestQueue);
        stTimeout.tv_sec = time(0) + 5;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condDBRequestQueue, &g_mutexDBRequestQueue, &stTimeout);
        pthread_mutex_unlock(&g_mutexDBRequestQueue);

        if (0 == DLL_Count(&g_stDBRequestQueue))
        {
            continue;
        }

        while (1)
        {
            if (0 == DLL_Count(&g_stDBRequestQueue))
            {
                break;
            }

            pthread_mutex_lock(&g_mutexDBRequestQueue);
            pstDLLNode = dll_fetch(&g_stDBRequestQueue);
            pthread_mutex_unlock(&g_mutexDBRequestQueue);

            if (DOS_ADDR_INVALID(pstDLLNode))
            {
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DB_WQ), "%s", "DB request queue error.");
                break;
            }

            if (DOS_ADDR_INVALID(pstDLLNode->pHandle))
            {
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DB_WQ), "%s", "DB request is empty.");
                break;
            }

            pstMsg = (SC_DB_MSG_TAG_ST *)pstDLLNode->pHandle;

            DLL_Init_Node(pstDLLNode);
            dos_dmem_free(pstDLLNode);
            pstDLLNode = NULL;

            sc_db_request_proc(pstMsg);
        }
    }

    return NULL;
}

U32 sc_send_msg2db(SC_DB_MSG_TAG_ST *pstMsg)
{
    DLL_NODE_S  *pstNode;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstNode))
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DB_WQ), "%s", "Send msg to db fail. Alloc memory fail");
        return DOS_FAIL;
    }

    DLL_Init_Node(pstNode);
    pstNode->pHandle = pstMsg;

    pthread_mutex_lock(&g_mutexDBRequestQueue);
    DLL_Add(&g_stDBRequestQueue, pstNode);
    pthread_cond_signal(&g_condDBRequestQueue);
    pthread_mutex_unlock(&g_mutexDBRequestQueue);

    return DOS_SUCC;
}

U32 sc_db_init()
{
    DLL_Init(&g_stDBRequestQueue);

    return DOS_SUCC;
}

U32 sc_db_start()
{
    if (pthread_create(&g_pthreadDB, NULL, sc_db_runtime, NULL) < 0)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DB_WQ), "%s", "Start the DB task FAIL.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_db_stop()
{
    g_blExitFlag = DOS_TRUE;

    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


