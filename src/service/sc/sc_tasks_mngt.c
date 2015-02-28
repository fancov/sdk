/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_tasks_mngt.c
 *
 *  创建时间: 2014年12月16日10:24:40
 *  作    者: Larry
 *  描    述: 对群呼任务进行管理
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <sys/time.h>


/* include private header files */
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_http_api.h"


/* define global variable */
/* 业务控制模块数据库句柄 */
extern DB_HANDLE_ST         *g_pstSCDBHandle;

/* 任务控制模块相关控制全集变量 */
SC_TASK_MNGT_ST   *g_pstTaskMngtInfo;

/* declare functions */
/*
 * 函数: S32 sc_task_load_callback(VOID *pArg, S32 argc, S8 **argv, S8 **columnNames)
 * 功能: 任务控制模块加载任务回调函数
 * 参数:
 * 返回值
 *      成功返回DOS_SUCC,失败返回DOS_FAIL
 **/
static S32 sc_task_load_callback(VOID *pArg, S32 argc, S8 **argv, S8 **columnNames)
{
    SC_TASK_CB_ST *pstTCB;
    U32 ulIndex;
    U32 ulTaskID = U32_BUTT, ulCustomID = U32_BUTT;

    SC_TRACE_IN((U64)pArg, (U32)argc, (U64)argv, (U64)columnNames);

    for (ulIndex=0; ulIndex<argc; ulIndex++)
    {
        if (dos_strcmp(columnNames[ulIndex], "id") == 0)
        {
            if (dos_atoul(argv[ulIndex], &ulTaskID) < 0)
            {
                SC_TRACE_OUT();
                return DOS_FAIL;
            }
        }
        else if (dos_strcmp(columnNames[ulIndex], "customer_id") == 0)
        {
            if (dos_atoul(argv[ulIndex], &ulCustomID) < 0)
            {
                SC_TRACE_OUT();
                return DOS_FAIL;
            }
        }
    }

    pstTCB = sc_tcb_alloc();
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_task_set_owner(pstTCB, ulTaskID, ulCustomID);


    return DOS_SUCC;
}

/*
 * 函数: U32 sc_task_mngt_load_task()
 * 功能: 初始化时任务控制模块加载任务
 * 参数:
 * 返回值
 *      成功返回DOS_SUCC,失败返回DOS_FAIL
 **/
U32 sc_task_mngt_load_task()
{
    S8 szSqlQuery[128]= { 0 };
    U32 ulLength, ulResult;

    SC_TRACE_IN(0, 0, 0, 0);

    ulLength = dos_snprintf(szSqlQuery
                , sizeof(szSqlQuery)
                , "SELECT tbl_calltask.id, tbl_calltask.customer_id from tbl_calltask;");

    ulResult = db_query(g_pstSCDBHandle
                            , szSqlQuery
                            , sc_task_load_callback
                            , NULL
                            , NULL);

    if (ulResult != DOS_SUCC)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
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
    SC_TASK_CB_ST *pstTCB = NULL;
    SC_TRACE_IN((U32)ulTaskID, ulCustomID, 0, 0);

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (pstTCB->ucTaskStatus != SC_TASK_PAUSED)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_TASK_STATUS;
    }

    sc_task_continue(pstTCB);

    SC_TRACE_OUT();
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
    SC_TASK_CB_ST *pstTCB = NULL;

    SC_TRACE_IN((U32)ulTaskID, 0, 0, 0);

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (pstTCB->ucTaskStatus != SC_TASK_WORKING)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_TASK_STATUS;
    }

    sc_task_pause(pstTCB);

    SC_TRACE_OUT();
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
    SC_TASK_CB_ST *pstTCB = NULL;
    SC_TRACE_IN((U32)ulTaskID, 0, 0, 0);

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_alloc();
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    sc_task_set_owner(pstTCB, ulTaskID, ulCustomID);

    if (sc_task_init(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_tcb_free(pstTCB);

        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    if (sc_task_start(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_tcb_free(pstTCB);

        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    SC_TRACE_OUT();
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
    SC_TASK_CB_ST *pstTCB = NULL;

    SC_TRACE_IN(ulTaskID, ulCustomID, 0, 0);

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (pstTCB->ucTaskStatus != SC_TASK_WORKING)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return SC_HTTP_ERRNO_INVALID_TASK_STATUS;
    }

    sc_task_stop(pstTCB);

    SC_TRACE_OUT();
    return SC_HTTP_ERRNO_SUCC;
}

/*
 * 函数: U32 sc_task_cmd_queue_add(SC_TASK_CTRL_CMD_ST *pstCMD)
 * 功能: 向API命令列表中添加命令
 * 参数:
 *      SC_TASK_CTRL_CMD_ST *pstCMD: API命令数据
 * 返回值
 *      成功返回DOS_SUC，失败返回DOS_FAIL
 * 该函数切勿阻塞
 **/
U32 sc_task_cmd_queue_add(SC_TASK_CTRL_CMD_ST *pstCMD)
{
    SC_TRACE_IN((U64)pstCMD, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstCMD))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCMDList);
    dos_list_add_head(&g_pstTaskMngtInfo->stCMDList, &pstCMD->stLink);
    pthread_cond_signal(&g_pstTaskMngtInfo->condCMDList);
    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCMDList);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/*
 * 函数: U32 sc_task_cmd_queue_del(SC_TASK_CTRL_CMD_ST *pstCMD)
 * 功能: 从API命令列表中删除命令
 * 参数:
 *      SC_TASK_CTRL_CMD_ST *pstCMD: API命令数据
 * 返回值
 *      成功返回DOS_SUC，失败返回DOS_FAIL
 * 该函数切勿阻塞
 **/
U32 sc_task_cmd_queue_del(SC_TASK_CTRL_CMD_ST *pstCMD)
{
    list_t                *pstListNode;
    SC_TASK_CTRL_CMD_ST   *pstCMDNode;
    U32                    ulResult;

    SC_TRACE_IN((U64)pstCMD, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstCMD))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCMDList);
    pstListNode = &g_pstTaskMngtInfo->stCMDList;
    while (1)
    {
        pstListNode = dos_list_work(&g_pstTaskMngtInfo->stCMDList, pstListNode);
        if (!pstListNode)
        {
            break;
        }

        pstCMDNode = dos_list_entry(pstListNode, SC_TASK_CTRL_CMD_ST, stLink);

        sc_logr_debug(SC_TASK_MNGT, "Work list, Current Node Addr: %p, (%p)", pstCMDNode, pstCMD);
        if (pstCMDNode && pstCMDNode == pstCMD)
        {
            break;
        }
    }

    if (pstCMDNode && pstCMDNode == pstCMD)
    {
        dos_list_del(pstListNode);

        ulResult = DOS_SUCC;
    }
    else
    {
        DOS_ASSERT(0);
        ulResult = DOS_FAIL;
    }

    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCMDList);

    SC_TRACE_OUT();
    return ulResult;
}

/*
 * 函数: VOID sc_task_mngt_cmd_process(SC_TASK_CTRL_CMD_ST *pstCMD)
 * 功能: 处理任务控制，呼叫控制API
 * 参数:
 *      SC_TASK_CTRL_CMD_ST *pstCMD: API命令数据
 * 返回值
 * 该函数切勿阻塞
 **/
VOID sc_task_mngt_cmd_process(SC_TASK_CTRL_CMD_ST *pstCMD)
{
    SC_TRACE_IN((U64)pstCMD, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstCMD))
    {
        SC_TRACE_OUT();
        return;
    }

    sc_logr_debug(SC_TASK_MNGT, "Process CMD. CMD: %d, Action:%d, Task: %d, CustomID: %d"
                    , pstCMD->ulCMD, pstCMD->ulAction, pstCMD->ulTaskID, pstCMD->ulCustomID);

    switch (pstCMD->ulCMD)
    {
        case SC_API_CMD_TASK_CTRL:
        {
            switch (pstCMD->ulAction)
            {
                case SC_API_CMD_ACTION_START:
                {
                    pstCMD->ulCMDErrCode = sc_task_mngt_start_task(pstCMD->ulTaskID, pstCMD->ulCustomID);
                    break;
                }
                case SC_API_CMD_ACTION_STOP:
                {
                    pstCMD->ulCMDErrCode = sc_task_mngt_stop_task(pstCMD->ulTaskID, pstCMD->ulCustomID);
                    break;
                }
                case SC_API_CMD_ACTION_CONTINUE:
                {
                    pstCMD->ulCMDErrCode = sc_task_mngt_continue_task(pstCMD->ulTaskID, pstCMD->ulCustomID);
                    break;
                }
                case SC_API_CMD_ACTION_PAUSE:
                {
                    pstCMD->ulCMDErrCode = sc_task_mngt_pause_task(pstCMD->ulTaskID, pstCMD->ulCustomID);
                    break;
                }
                default:
                {
                    sc_logr_notice(SC_TASK_MNGT, "CMD templately not support. CMD:%d, ACTION: %d", pstCMD->ulCMD, pstCMD->ulAction);
                    DOS_ASSERT(0);
                    break;
                }
            }

            break;
        }
        case SC_API_CMD_CALL_CTRL:
        {
            switch (pstCMD->ulAction)
            {
                case SC_API_CMD_ACTION_MONITORING:
                {
                    break;
                }
                case SC_API_CMD_ACTION_HUNGUP:
                {
                    break;
                }
                default:
                {
                    sc_logr_notice(SC_TASK_MNGT, "CMD templately not support. CMD:%d, ACTION: %d", pstCMD->ulCMD, pstCMD->ulAction);
                    DOS_ASSERT(0);
                    break;
                }
            }

            break;
        }
        default:
        {
            sc_logr_notice(SC_TASK_MNGT, "CMD templately not support. CMD:%d, ACTION: %d", pstCMD->ulCMD, pstCMD->ulAction);

            DOS_ASSERT(0);
            break;
        }
    }

    sc_logr_debug(SC_TASK_MNGT, "CMD Process finished. CMD:%d , Action: %d, ErrCode:%d"
                    , pstCMD->ulCMD, pstCMD->ulAction, pstCMD->ulCMDErrCode);
}

/*
 * 函数: VOID *sc_task_mngt_runtime(VOID *ptr)
 * 功能: 呼叫任务控制模块线程主函数
 * 参数:
 **/
VOID *sc_task_mngt_runtime(VOID *ptr)
{
    struct timespec       stTimeout;
    U32                   ulTaskIndex;
    list_t                *pstListNode;
    SC_TASK_CTRL_CMD_ST   *pstCMD;

    SC_TRACE_IN(0, 0, 0, 0);

    while(1)
    {
        pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCMDList);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_pstTaskMngtInfo->condCMDList, &g_pstTaskMngtInfo->mutexCMDList, &stTimeout);

        /* 处理命令队列 */
        if (!dos_list_is_empty(&g_pstTaskMngtInfo->stCMDList))
        {
            pstListNode = &g_pstTaskMngtInfo->stCMDList;
            while (1)
            {
                if (dos_list_is_empty(&g_pstTaskMngtInfo->stCMDList))
                {
                    break;
                }

                pstListNode = dos_list_work(&g_pstTaskMngtInfo->stCMDList, pstListNode);
                if (!pstListNode)
                {
                    break;
                }

                pstCMD = dos_list_entry(pstListNode, SC_TASK_CTRL_CMD_ST, stLink);
                if (!pstCMD)
                {
                    break;
                }

                sc_task_mngt_cmd_process(pstCMD);

                sem_post(&pstCMD->semCMDExecNotify);
            }
        }

        /* 处理系统状态 */
        g_pstTaskMngtInfo->enSystemStatus = sc_check_sys_stat();

        /* 处理退出标志 */
        if (g_pstTaskMngtInfo->blWaitingExitFlag)
        {
            pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCMDList);
            break;
        }

        pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCMDList);
    }

    for (ulTaskIndex=0; ulTaskIndex<SC_MAX_TASK_NUM; ulTaskIndex++)
    {
        if (g_pstTaskMngtInfo->pstTaskList[ulTaskIndex].ucValid)
        {
            sc_task_stop(&g_pstTaskMngtInfo->pstTaskList[ulTaskIndex]);
        }
    }

    SC_TRACE_OUT();

    return NULL;
}

/*
 * 函数: U32 sc_task_mngt_start()
 * 功能: 启动呼叫控制模块，同时启动已经被加载的呼叫任务
 * 参数:
 * 返回值: 成功返回DOS_SUCC， 失败返回DOS_FAIL
 **/
U32 sc_task_mngt_start()
{
    SC_TASK_CB_ST    *pstTCB = NULL;
    U32              ulIndex;

    SC_TRACE_IN(0, 0, 0, 0);

    pthread_create(&g_pstTaskMngtInfo->pthID, NULL, sc_task_mngt_runtime, NULL);

    for (ulIndex=0; ulIndex<SC_MAX_TASK_NUM; ulIndex++)
    {
        pstTCB = &g_pstTaskMngtInfo->pstTaskList[ulIndex];

        if (pstTCB->ucValid
            && SC_TCB_HAS_VALID_OWNER(pstTCB))
        {
            if (sc_task_init(pstTCB) != DOS_SUCC)
            {
                SC_TASK_TRACE(pstTCB, "%s", "Task init fail");
                sc_logr_notice(SC_TASK_MNGT, "Task init fail. Custom ID: %d, Task ID: %d", pstTCB->ulCustomID, pstTCB->ulTaskID);

                sc_tcb_free(pstTCB);
                continue;
            }

            if (sc_task_start(pstTCB) != DOS_SUCC)
            {
                SC_TASK_TRACE(pstTCB, "%s", "Task init fail");
                sc_logr_notice(SC_TASK_MNGT, "Task start fail. Custom ID: %d, Task ID: %d", pstTCB->ulCustomID, pstTCB->ulTaskID);

                sc_tcb_free(pstTCB);
                continue;
            }

            g_pstTaskMngtInfo->ulTaskCount++;
        }
    }

    sc_logr_info(SC_TASK, "Start call task mngt service finished. Total load %d call task(s).", g_pstTaskMngtInfo->ulTaskCount);

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
    SC_TASK_CB_ST   *pstTCB = NULL;
    SC_SCB_ST       *pstSCB = NULL;
    U32             ulIndex    = 0;

    SC_TRACE_IN(0, 0, 0, 0);

    /* 申请管理任务的相关变量 */
    g_pstTaskMngtInfo = (SC_TASK_MNGT_ST *)dos_dmem_alloc(sizeof(SC_TASK_MNGT_ST));
    if (!g_pstTaskMngtInfo)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    dos_memzero(g_pstTaskMngtInfo, sizeof(SC_TASK_MNGT_ST));


    /* 初始化其他全局变量 */
    pthread_mutex_init(&g_pstTaskMngtInfo->mutexCMDList, NULL);
    pthread_mutex_init(&g_pstTaskMngtInfo->mutexTCBList, NULL);
    pthread_mutex_init(&g_pstTaskMngtInfo->mutexCallList, NULL);
    pthread_mutex_init(&g_pstTaskMngtInfo->mutexCallHash, NULL);
    pthread_cond_init(&g_pstTaskMngtInfo->condCMDList, NULL);
    dos_list_init(&g_pstTaskMngtInfo->stCMDList);

    g_pstTaskMngtInfo->pstCallSCBHash = hash_create_table(SC_MAX_SCB_HASH_NUM, NULL);
    if (!g_pstTaskMngtInfo->pstCallSCBHash)
    {
        DOS_ASSERT(0);

        dos_smem_free(g_pstTaskMngtInfo);
        g_pstTaskMngtInfo = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;

    }

    /* 初始化呼叫控制块和任务控制块 */
    g_pstTaskMngtInfo->pstTaskList = (SC_TASK_CB_ST *)dos_smem_alloc(sizeof(SC_TASK_CB_ST) * SC_MAX_TASK_NUM);
    g_pstTaskMngtInfo->pstCallSCBList = (SC_SCB_ST *)dos_smem_alloc(sizeof(SC_SCB_ST) * SC_MAX_SCB_NUM);
    if (!g_pstTaskMngtInfo->pstTaskList || !g_pstTaskMngtInfo->pstCallSCBList)
    {
        DOS_ASSERT(0);

        if (g_pstTaskMngtInfo->pstTaskList)
        {
            dos_smem_free(g_pstTaskMngtInfo->pstTaskList);
            g_pstTaskMngtInfo->pstTaskList = NULL;
        }

        if (g_pstTaskMngtInfo->pstCallSCBList)
        {
            dos_smem_free(g_pstTaskMngtInfo->pstCallSCBList);
            g_pstTaskMngtInfo->pstCallSCBList = NULL;
        }

        dos_smem_free(g_pstTaskMngtInfo);
        g_pstTaskMngtInfo = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_memzero(g_pstTaskMngtInfo->pstCallSCBList, sizeof(SC_SCB_ST) * SC_MAX_SCB_NUM);
    for (ulIndex = 0; ulIndex < SC_MAX_SCB_NUM; ulIndex++)
    {
        pstSCB = &g_pstTaskMngtInfo->pstCallSCBList[ulIndex];
        pthread_mutex_init(&pstSCB->mutexSCBLock, NULL);
        pthread_mutex_lock(&pstSCB->mutexSCBLock);
        pstSCB->usSCBNo = (U16)ulIndex;
        sc_scb_init(pstSCB);
        pthread_mutex_unlock(&pstSCB->mutexSCBLock);
    }

    dos_memzero(g_pstTaskMngtInfo->pstTaskList, sizeof(SC_TASK_CB_ST) * SC_MAX_TASK_NUM);
    for (ulIndex = 0; ulIndex < SC_MAX_TASK_NUM; ulIndex++)
    {
        pstTCB = &g_pstTaskMngtInfo->pstTaskList[ulIndex];
        pstTCB->usTCBNo = ulIndex;
        pthread_mutex_init(&pstTCB->mutexTaskList, NULL);
        pstTCB->ucTaskStatus = SC_TASK_BUTT;
        pstTCB->ulTaskID = U32_BUTT;
        pstTCB->ulCustomID = U32_BUTT;

        dos_list_init(&pstTCB->stCalleeNumQuery);
    }

    g_pstTaskMngtInfo->ulTaskCount = 0;
    g_pstTaskMngtInfo->ulMaxCall = 0;
    g_pstTaskMngtInfo->blWaitingExitFlag = 0;

    sc_task_mngt_load_task();

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/*
 * 函数: U32 sc_task_mngt_shutdown()
 * 功能: 停止任务管理模块
 * 参数:
 * 返回值: 成功返回DOS_SUCC， 失败返回DOS_FAIL
 **/
U32 sc_task_mngt_shutdown()
{
    g_pstTaskMngtInfo->blWaitingExitFlag = 1;

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

