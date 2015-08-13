/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  mc_ini.c
 *
 *  Created on: 2015-8-11 09:04:41
 *      Author: Larry
 *        Todo: 媒体处理任务主线程
 *     History:
 */

#include <dos.h>
#include <mc_pub.h>
#include "mc_def.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* 只是该模块是否正在等待停止 */
BOOL g_blMCWaitingExit = DOS_FALSE;

/* 当前任务的工作状态 */
U32  g_blWorkingStatus = MC_WORKING_STATUS_WAITING;

/* 调度任务控制块 */
MC_SERVER_CB   *g_pstMasterTask   = NULL;

/* 工作任务控制块 */
MC_SERVER_CB   *g_pstWorkingTask  = NULL;

/**
 * 函数: U32 mc_load_data()
 * 功能: 从文件系统中查找没有被处理的文件，并将其加入到调度任务队列中
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
static U32 mc_load_data()
{
    FILE *pFile              = NULL;
    S8   szFileName[200]     = { 0, };
    DLL_NODE_S *pstDLLNode   = NULL;

    dos_snprintf(szFileName
                , sizeof(szFileName)
                , "find %s \\( -name \\*.G729 -o -name \\*.G723 -o -name \\*.PCMA -o -name \\*.PCMU \\)"
                , MC_RECORD_FILE_PATH);

    pFile = popen(szFileName, "r");
    if (DOS_ADDR_INVALID(pFile))
    {
        DOS_ASSERT(0);

        mc_log(DOS_TRUE, LOG_LEVEL_WARNING, "Execute command fail. %s", szFileName);
        return DOS_FAIL;
    }

    while (!feof(pFile))
    {
        pstDLLNode = (DLL_NODE_S *)dos_dmem_alloc(MC_MAX_FILENAME_BUFF_LEN);
        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            break;
        }

        if (NULL == fgets((S8 *)pstDLLNode->pHandle, MC_MAX_FILENAME_LEN, pFile))
        {
            break;
        }

        DLL_Add(&g_pstMasterTask->stQueue, pstDLLNode);
        g_pstMasterTask->ulTotalProc++;
    }

    pclose(pFile);

    return DOS_SUCC;
}

/**
 * 函数: mc_add_data()
 * 功能: 将调度任务队列中的数据移动到工作任务中
 * 参数:
 *     MC_SERVER_CB *pstTaskCB : 工作任务控制块
 *     U32 ulMaxSize           : 最大数据个数
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
static U32 mc_add_data(MC_SERVER_CB *pstTaskCB, U32 ulMaxSize)
{
    U32 ulData               = ulMaxSize;
    DLL_NODE_S   *pstDLLNode = NULL;

    while (ulData-- > 0)
    {
        pthread_mutex_lock(&g_pstMasterTask->mutexQueue);
        if (&g_pstMasterTask->stQueue.ulCount == 0 )
        {
            pthread_mutex_unlock(&g_pstMasterTask->mutexQueue);

            mc_log(DOS_TRUE, LOG_LEVEL_DEBUG, "The queue for the master task is empty while add data to task.");
            break;
        }

        pstDLLNode = dll_fetch(&g_pstMasterTask->stQueue);
        g_pstMasterTask->ulSuccessProc++;
        pthread_mutex_unlock(&g_pstMasterTask->mutexQueue);

        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            pthread_mutex_lock(&pstTaskCB->mutexQueue);
            DLL_Add(&pstTaskCB->stQueue, pstDLLNode);
            pthread_mutex_unlock(&pstTaskCB->mutexQueue);
        }
    }

    return DOS_SUCC;
}

/**
 * 函数: mc_working_task()
 * 功能: 工作任务主函数
 * 参数:
 *     VOID *ptr : 工作任务控制块
 * 返回值:
 */
static VOID *mc_working_task(VOID *ptr)
{
    MC_SERVER_CB *pstTaskCB  = NULL;
    DLL_NODE_S   *pstDLLNode = NULL;
    FILE         *pFileFD    = NULL;
    S8           *pszFileName= NULL;
    S8           *pszEnd     = NULL;
    S8           szFile[MC_MAX_FILENAME_LEN]   = { 0, };
    S8           szPath[MC_MAX_FILENAME_LEN]   = { 0, };
    S8           szCMDBuff[1024]               = { 0, };
    U32          ulIndex;
    U32          ulExecResult= 0;
    struct timespec     stTimeout;

    if (DOS_ADDR_INVALID(ptr))
    {
        return NULL;
    }
    pstTaskCB = ptr;

    pthread_mutex_lock(&pstTaskCB->mutexQueue);
    pstTaskCB->ulStatus = MC_TSK_STATUS_WAITING;
    pthread_mutex_unlock(&pstTaskCB->mutexQueue);

    while (1)
    {
        if (g_blMCWaitingExit)
        {
            break;
        }

        pthread_mutex_lock(&pstTaskCB->mutexQueue);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&pstTaskCB->condQueue, &pstTaskCB->mutexQueue, &stTimeout);
        pthread_mutex_unlock(&pstTaskCB->mutexQueue);

        if (MC_WORKING_STATUS_WAITING == g_blWorkingStatus)
        {
            continue;
        }

        while (1)
        {
            pthread_mutex_lock(&pstTaskCB->mutexQueue);
            if (pstTaskCB->stQueue.ulCount == 0)
            {
                pstTaskCB->ulStatus = MC_TSK_STATUS_WAITING;
                pthread_mutex_unlock(&pstTaskCB->mutexQueue);
                break;
            }

            pstTaskCB->ulStatus = MC_TSK_STATUS_RUNNING;

            pstDLLNode = dll_fetch(&pstTaskCB->stQueue);
            if (DOS_ADDR_INVALID(pstDLLNode))
            {
                pthread_mutex_unlock(&pstTaskCB->mutexQueue);
                continue;
            }
            pstTaskCB->ulTotalProc++;
            pthread_mutex_unlock(&pstTaskCB->mutexQueue);

            if (DOS_ADDR_INVALID(pstDLLNode->pHandle))
            {
                continue;
            }

            /* 文件不存在是有可能的, 一次就处理两个了 */
            pszFileName = pstDLLNode->pHandle;
            if (access(pszFileName, F_OK) < 0)
            {
                goto prcess_finished;
            }

            /* 文件存在，需要处理，步骤如下: */
            /* 获取文件名 */
            pszEnd = dos_strstr(pszFileName, "-out");
            if (DOS_ADDR_INVALID(pszEnd))
            {
                pszEnd = dos_strstr(pszFileName, "-in");
            }
            if (DOS_ADDR_INVALID(pszEnd))
            {
                pszEnd = dos_strstr(pszFileName, "-OUT");
            }
            if (DOS_ADDR_INVALID(pszEnd))
            {
                pszEnd = dos_strstr(pszFileName, "-IN");
            }

            ulExecResult = U32_BUTT;
            if (DOS_ADDR_INVALID(pszEnd)
                || pszEnd - pszFileName >= MC_MAX_FILENAME_LEN)
            {
                /* 故障了 */
                DOS_ASSERT(0);
                goto prcess_finished;
            }

            dos_strncpy(szPath, pszFileName, pszEnd - pszFileName);
            szPath[pszEnd - pszFileName] = '\0';

            ulIndex = dos_strlastindexof(szPath, '/', dos_strlen(szPath));
            if (ulIndex < 0 || '\0' == szPath[ulIndex+1])
            {
                DOS_ASSERT(0);
                goto prcess_finished;
            }

            szPath[ulIndex] = '\0';

            dos_strncpy(szFile, szPath+ulIndex+1, sizeof(szFile));
            szFile[sizeof(szFile) - 1] = '\0';

            /* 调用脚本处理,并删除 */
            dos_snprintf(szCMDBuff, sizeof(szCMDBuff)
                        , "%s decodec %s %s %s %s && rm -rf %s/%s"
                        , MC_SCRIPT_PATH
                        , szPath, szFile
                        , szPath, szFile
                        , szPath, szFile);

            pFileFD = popen(szCMDBuff, "r");
            if (DOS_ADDR_INVALID(pFileFD))
            {
                goto prcess_finished;
            }
            ulExecResult = pclose(pFileFD);

prcess_finished:
            if (0 == ulExecResult)
            {
                /* 执行成功 */
                pstTaskCB->ulSuccessProc++;
            }
            else
            {
                /* 失败 */
            }

            /* 释放资源继续执行 */
            dos_dmem_free(pstDLLNode);
            pstDLLNode = NULL;
            pszFileName = NULL;

            continue;
        }
    }

    pthread_mutex_lock(&pstTaskCB->mutexQueue);
    pstTaskCB->ulStatus = MC_TSK_STATUS_EXITED;
    pthread_mutex_unlock(&pstTaskCB->mutexQueue);

    return NULL;
}

/**
 * 函数: mc_master_task()
 * 功能: 调度任务主函数
 * 参数:
 *     VOID *ptr : NULL
 * 返回值:
 */
static VOID *mc_master_task(VOID *ptr)
{
    U32 ulLastTimestamp;
    U32 ulWorkingTaskCnt;
    U32 ulCPUUsage, i;

    g_blWorkingStatus = MC_WORKING_STATUS_WAITING;
    ulLastTimestamp   = U32_BUTT;

    while (1)
    {
        dos_task_delay(1000);

        /* CPU超过70%，就停止所有任务 */
        ulCPUUsage = 100 - (dos_get_cpu_idel_percentage() / 100);
        if (ulCPUUsage > 70)
        {
            g_blWorkingStatus = MC_WORKING_STATUS_WAITING;
            ulLastTimestamp   = U32_BUTT;

            mc_log(DOS_TRUE, LOG_LEVEL_NOTIC, "All the task come into waiting status. CPU:%u%%", ulCPUUsage);

            continue;
        }

        /* CPU占用小于20%，就准备下一次工作 */
        if (ulCPUUsage < 20 && U32_BUTT == ulLastTimestamp)
        {
            ulLastTimestamp = time(NULL);

            mc_log(DOS_TRUE, LOG_LEVEL_DEBUG, "Update timestamp. CPU:%u%%", ulCPUUsage);

            continue;
        }

        if (time(NULL) - ulLastTimestamp < MC_WAITING_TIME)
        {
            continue;
        }

        if (g_blWorkingStatus != MC_WORKING_STATUS_WORKING)
        {
            g_blWorkingStatus = MC_WORKING_STATUS_WORKING;
            mc_log(DOS_TRUE, LOG_LEVEL_NOTIC, "All the task come into working status.");
        }

        pthread_mutex_lock(&g_pstMasterTask->mutexQueue);
        if (g_pstMasterTask->stQueue.ulCount <= 0)
        {
            mc_load_data();
            mc_log(DOS_TRUE, LOG_LEVEL_DEBUG, "Load data finished. size:%u", g_pstMasterTask->stQueue.ulCount);
        }

        if (g_pstMasterTask->stQueue.ulCount <= 0)
        {
            pthread_mutex_unlock(&g_pstMasterTask->mutexQueue);

            mc_log(DOS_TRUE, LOG_LEVEL_DEBUG, "Continue for there is no data to process.");
            continue;
        }
        pthread_mutex_unlock(&g_pstMasterTask->mutexQueue);

        for (ulWorkingTaskCnt=0, i=0; i<MC_MAX_WORKING_TASK_CNT; i++)
        {
            if (g_pstWorkingTask[i].ulStatus == MC_TSK_STATUS_WAITING)
            {
                if (mc_add_data(&g_pstWorkingTask[i], MC_MAX_DATA_PRE_TIME) != DOS_SUCC)
                {
                    DOS_ASSERT(0);

                    mc_log(DOS_TRUE, LOG_LEVEL_NOTIC, "All data to working task fail.");
                }
            }
        }
    }

    return NULL;
}

/**
 * 函数: mc_init()
 * 功能: 初始化媒体控制任务
 * 参数:
 *     VOID *ptr : NULL
 * 返回值:
 */
U32 mc_init()
{
    U32 i;

    g_pstMasterTask = dos_dmem_alloc(sizeof(MC_SERVER_CB));
    if (DOS_ADDR_INVALID(g_pstMasterTask))
    {
        DOS_ASSERT(0);

        mc_log(DOS_TRUE, LOG_LEVEL_ERROR, "Alloc memory for the master task fail.");
        return DOS_FAIL;
    }

    pthread_mutex_init(&g_pstMasterTask->mutexQueue, NULL);
    pthread_cond_init(&g_pstMasterTask->condQueue, NULL);
    g_pstMasterTask->ulStatus = MC_TSK_STATUS_INIT;
    g_pstMasterTask->ulSuccessProc = 0;
    g_pstMasterTask->ulTotalProc   = 0;
    DLL_Init(&g_pstMasterTask->stQueue);

    g_pstWorkingTask = dos_dmem_alloc(sizeof(MC_SERVER_CB) * MC_MAX_WORKING_TASK_CNT);
    if (DOS_ADDR_INVALID(g_pstWorkingTask))
    {
        DOS_ASSERT(0);

        mc_log(DOS_TRUE, LOG_LEVEL_ERROR, "Alloc memory for the working task fail.");

        dos_dmem_free(g_pstMasterTask);
        return DOS_FAIL;
    }

    for (i=0; i<MC_MAX_WORKING_TASK_CNT; i++)
    {
        pthread_mutex_init(&g_pstWorkingTask[i].mutexQueue, NULL);
        pthread_cond_init(&g_pstWorkingTask[i].condQueue, NULL);
        g_pstWorkingTask[i].ulStatus = MC_TSK_STATUS_INIT;
        g_pstWorkingTask[i].ulSuccessProc = 0;
        g_pstWorkingTask[i].ulTotalProc   = 0;
        DLL_Init(&g_pstWorkingTask[i].stQueue);
    }

    return DOS_SUCC;
}

/**
 * 函数: mc_start()
 * 功能: 启动媒体控制任务
 * 参数:
 *     VOID *ptr : NULL
 * 返回值:
 */
U32 mc_start()
{
    U32 i;

    /* 先启动所有工作线程 */
    for (i=0; i<MC_MAX_WORKING_TASK_CNT; i++)
    {
        if (pthread_create(&g_pstWorkingTask[i].pthServer, NULL, mc_working_task, &g_pstWorkingTask[i]) < 0)
        {
            g_blMCWaitingExit = DOS_TRUE;

            mc_log(DOS_TRUE, LOG_LEVEL_ERROR, "Start working task fail, all the task will be exit. Index: %u.", i);
            return DOS_FAIL;
        }
    }

    /* 启动主线程 */
    if (pthread_create(&g_pstMasterTask->pthServer, NULL, mc_master_task, NULL) < 0)
    {
        g_blMCWaitingExit = DOS_TRUE;

        mc_log(DOS_TRUE, LOG_LEVEL_ERROR, "Start master task fail, all the task will be exit.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 函数: mc_start()
 * 功能: 停止
 * 参数:
 *     VOID *ptr : NULL
 * 返回值:
 */
U32 mc_stop()
{
    g_blMCWaitingExit = DOS_TRUE;


    mc_log(DOS_TRUE, LOG_LEVEL_ERROR, "Request stop all the task Acceptted.");

    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


