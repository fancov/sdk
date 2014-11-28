/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  timer.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: dos中定时器模块相关函数
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* DOS header files */
#include <dos.h>


#if INCLUDE_SERVICE_TIMER

/* include system header file */
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <pthread.h>


/* 定时器精度，10毫秒 */
#define TIMER_ACCURACY 10

#define EPOLL_MAX_EVENT 5

typedef struct tagTimerListNode{
    TIMER_HANDLE_ST tmrHandle;
    S64 lTimerPassby;

    struct tagTimerListNode *prev;
    struct tagTimerListNode *next;
}TIMER_LIST_NODE_ST;

static pthread_t        g_pthIDTimer;
static pthread_mutex_t  g_mutexTmrList = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  g_mutexTmrWaitAddList = PTHREAD_MUTEX_INITIALIZER;
static TIMER_LIST_NODE_ST *g_TmrWorkingList;
static TIMER_LIST_NODE_ST *g_TmrWaitAddList;
static S32  g_lTmrWaitingExit;
static S32  g_lEpollFD = -1;
static S32  g_lTmrCurrentCnt = 0;

static S8 *g_pszTimerType[] = 
{
    "Normal",
    "Loop",
    NULL
};

S32 tmr_task_init();
S32 tmr_task_start();
S32 tmr_task_stop();
static VOID * tmr_task_loop(VOID *ptr);

S32 dos_tmr_start(DOS_TMR_ST *hTmrHandle, U32 ulInterval, VOID (*func)(U64 ), U64 uLParam, U32 ulType);
S32 dos_tmr_stop(DOS_TMR_ST *hTmrHandle);

S32 cli_show_timer(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulTotleCnt = 0, ulType;
    S8 szBuff[128] = { 0 };
    S8 szTitle[128] = { 0 };
    S8 *pszType;
    TIMER_LIST_NODE_ST *pstTmrNode;

    dos_snprintf(szTitle, sizeof(szTitle)
                , "%5s%20s%10s%10s%10s\r\n"
                , "No.", "Handle", "Interval", "Passby", "Type");

    cli_out_string(ulIndex, "\r\nShow timer usage:\r\n");
    cli_out_string(ulIndex, "=======================================================\r\n");
    cli_out_string(ulIndex, "Timer with waiting add status:\r\n");
    cli_out_string(ulIndex, "-------------------------------------------------------\r\n");
    cli_out_string(ulIndex, szTitle);
    cli_out_string(ulIndex, "-------------------------------------------------------\r\n");
    
    ulTotleCnt = 0;
    pthread_mutex_lock(&g_mutexTmrWaitAddList);
    pstTmrNode = g_TmrWaitAddList->next;
    while(g_TmrWaitAddList != pstTmrNode)
    {
        ulType = pstTmrNode->tmrHandle.eType;
        pszType = ulType >= TIMER_NORMAL_BUTT ? "" : g_pszTimerType[ulType];
        dos_snprintf(szBuff, sizeof(szBuff)
                    , "%5d%20X%10d%10d%10s\r\n"
                    , ulTotleCnt
                    , &(pstTmrNode->tmrHandle)
                    , pstTmrNode->tmrHandle.lInterval
                    , pstTmrNode->lTimerPassby
                    , pszType);

        ulTotleCnt++;
        pstTmrNode = pstTmrNode->next;

        cli_out_string(ulIndex, szBuff);
    }
    pthread_mutex_unlock(&g_mutexTmrWaitAddList);
    cli_out_string(ulIndex, "\r\n");
     

    cli_out_string(ulIndex, "=======================================================\r\n");
    cli_out_string(ulIndex, "Timer with working add status:\r\n");
    cli_out_string(ulIndex, "-------------------------------------------------------\r\n");
    cli_out_string(ulIndex, szTitle);
    cli_out_string(ulIndex, "-------------------------------------------------------\r\n");
    pthread_mutex_lock(&g_mutexTmrList);
    pstTmrNode = g_TmrWorkingList->next;
    while(g_TmrWorkingList != pstTmrNode)
    {
        ulType = pstTmrNode->tmrHandle.eType;
        pszType = ulType >= TIMER_NORMAL_BUTT ? "" : g_pszTimerType[ulType];
        dos_snprintf(szBuff, sizeof(szBuff)
                    , "%5d%20X%10d%10d%10s\r\n"
                    , ulTotleCnt
                    , &(pstTmrNode->tmrHandle)
                    , pstTmrNode->tmrHandle.lInterval
                    , pstTmrNode->lTimerPassby
                    , pszType);

        ulTotleCnt++;
        pstTmrNode = pstTmrNode->next;

        cli_out_string(ulIndex, szBuff);
    }
    pthread_mutex_unlock(&g_mutexTmrList);
    cli_out_string(ulIndex, "-------------------------------------------------------\r\n");
    cli_out_string(ulIndex, "\r\n");

    return 0;

}


S32 tmr_task_init()
{
    g_lTmrWaitingExit = 1;
    g_lTmrCurrentCnt = 0;

    /* 初始化工作队列 */
    g_TmrWorkingList = (TIMER_LIST_NODE_ST *)malloc(sizeof(TIMER_LIST_NODE_ST));
    if (!g_TmrWorkingList)
    {
        DOS_ASSERT(0);
        return -1;
    }
    g_TmrWorkingList->next = g_TmrWorkingList;
    g_TmrWorkingList->prev = g_TmrWorkingList;

    /* 初始化等待添加队列 */
    g_TmrWaitAddList = (TIMER_LIST_NODE_ST *)malloc(sizeof(TIMER_LIST_NODE_ST));
    if (!g_TmrWaitAddList)
    {
        DOS_ASSERT(0);
        return -1;
    }
    g_TmrWaitAddList->next = g_TmrWaitAddList;
    g_TmrWaitAddList->prev = g_TmrWaitAddList;

    /* 初始化epoll句柄 */
    g_lEpollFD = epoll_create(EPOLL_MAX_EVENT);
    if (g_lEpollFD < 0)
    {
        return -1;
    }

    return 0;
}

S32 tmr_task_start()
{
    S32 lResult = 0;

    lResult = pthread_create(&g_pthIDTimer, NULL, tmr_task_loop, NULL);
    if (lResult < 0)
    {
        perror("Start the timer task fail.");
        return -1;
    }

    return 0;
}

S32 tmr_task_stop()
{
    pthread_mutex_lock(&g_mutexTmrList);
    g_lTmrWaitingExit = 1;
    pthread_mutex_unlock(&g_mutexTmrList);

    pthread_join(g_pthIDTimer, NULL);

    return 0;
}

S32 tmr_add2work_list()
{
    TIMER_LIST_NODE_ST *tmrNode = NULL;

    pthread_mutex_lock(&g_mutexTmrList);
    pthread_mutex_lock(&g_mutexTmrWaitAddList);

    /* 修改定时器状态 */
    tmrNode = g_TmrWaitAddList->next;
    while(g_TmrWaitAddList != tmrNode)
    {
        tmrNode->tmrHandle.ulTmrStatus = TIMER_STATUS_WORKING;

        tmrNode = tmrNode->next;
    }

    /* 如果等待队列里面有定时器，就需要合并一下 */
    if (g_TmrWaitAddList->next != g_TmrWaitAddList)
    {
        /* 将工作队列的尾指向等待队列的头，将等待队列的手元素指向工作队列的头 */
        if (g_TmrWorkingList->next == g_TmrWorkingList)
        {
            g_TmrWorkingList->next = g_TmrWaitAddList->next;
            g_TmrWorkingList->prev = g_TmrWaitAddList->prev;

            g_TmrWaitAddList->next->prev = g_TmrWorkingList;
            g_TmrWaitAddList->prev->next = g_TmrWorkingList;
        }
        else
        {
            g_TmrWorkingList->prev->next = g_TmrWaitAddList->next;
            g_TmrWaitAddList->next->prev = g_TmrWorkingList->prev;

            g_TmrWaitAddList->prev->next = g_TmrWorkingList;
            g_TmrWorkingList->prev = g_TmrWaitAddList->prev;
        }

        g_TmrWaitAddList->next = g_TmrWaitAddList;
        g_TmrWaitAddList->prev = g_TmrWaitAddList;
    }

    pthread_mutex_unlock(&g_mutexTmrWaitAddList);
    pthread_mutex_unlock(&g_mutexTmrList);

    return 0;
}

static VOID * tmr_task_loop(VOID *ptr)
{
    TIMER_LIST_NODE_ST *tmrNode = NULL, *tmrTmp = NULL;
    U32 ulCnt;

    pthread_mutex_lock(&g_mutexTmrList);
    g_lTmrWaitingExit = 0;
    pthread_mutex_unlock(&g_mutexTmrList);

    while (1)
    {
        pthread_mutex_lock(&g_mutexTmrList);
        if (g_lTmrWaitingExit)
        {
            dos_printf("%s", "Timer task finished flag has been set.");
            pthread_mutex_unlock(&g_mutexTmrList);
            break;
        }
        pthread_mutex_unlock(&g_mutexTmrList);

        tmr_add2work_list();

#if 0
        usleep(TIMER_ACCURACY);
#else
        struct epoll_event events[1];
        epoll_wait(g_lEpollFD, events, EPOLL_MAX_EVENT, TIMER_ACCURACY);
#endif

        pthread_mutex_lock(&g_mutexTmrList);
        tmrNode = g_TmrWorkingList->next;
        while(g_TmrWorkingList != tmrNode)
        {
            /* 如果已经被删除掉了，就删除 */
            if (TIMER_STATUS_WAITING_DEL == tmrNode->tmrHandle.ulTmrStatus)
            {
                /* 缓存一下,方便下次循环 */
                tmrTmp = tmrNode->next;


                tmrNode->next->prev = tmrNode->prev;
                tmrNode->prev->next = tmrNode->next;

                tmrNode->next = NULL;
                tmrNode->prev = NULL;

                if (&tmrNode->tmrHandle == *(tmrNode->tmrHandle.hTmr))
                {
                    *(tmrNode->tmrHandle.hTmr) = NULL;
                }

                logr_debug("Timer stop. Handle:%p", tmrNode->tmrHandle);

                dos_dmem_free(tmrNode);
                tmrNode = tmrTmp;

                if (g_lTmrCurrentCnt <= 0)
                {
                    DOS_ASSERT(0);
                    g_lTmrCurrentCnt = 0;
                }
                else
                {
                    g_lTmrCurrentCnt--;
                }

                pthread_mutex_unlock(&g_mutexTmrList);
                continue;
            }
            /* 已经超时了，置delete状态 */
            else if (TIMER_STATUS_TIMEOUT == tmrNode->tmrHandle.ulTmrStatus)
            {
                tmrNode->tmrHandle.ulTmrStatus = TIMER_STATUS_WAITING_DEL;
            }
            /* 运行中的定时器 */
            else if (TIMER_STATUS_WORKING == tmrNode->tmrHandle.ulTmrStatus)
            {
                tmrNode->lTimerPassby -= TIMER_ACCURACY;

                /* 发现定时器超时了 */
                /* 执行回调函数，需要解锁，然后执行，在返回之后要lock */
                if (tmrNode->lTimerPassby <= 0)
                {
                    tmrNode->tmrHandle.ulTmrStatus = TIMER_STATUS_TIMEOUT;
                    pthread_mutex_unlock(&g_mutexTmrList);
                    if (tmrNode->tmrHandle.func)
                    {
                        tmrNode->tmrHandle.func(tmrNode->tmrHandle.ulData);
                    }
                    pthread_mutex_lock(&g_mutexTmrList);

                    /* 如果是循环定时器，就重置 */
                    if (TIMER_NORMAL_LOOP == tmrNode->tmrHandle.eType)
                    {
                        tmrNode->lTimerPassby = tmrNode->tmrHandle.lInterval;
                        tmrNode->tmrHandle.ulTmrStatus = TIMER_STATUS_WORKING;
                    }
                    else
                    {
                        tmrNode->tmrHandle.ulTmrStatus = TIMER_STATUS_TIMEOUT;
                    }
                }
            }
            /* 其他视为不合法的状态，都置delete标志 */
            else
            {
                tmrNode->tmrHandle.ulTmrStatus = TIMER_STATUS_WAITING_DEL;
            }

            tmrNode = tmrNode->next;
        }

        pthread_mutex_unlock(&g_mutexTmrList);
    }

    pthread_mutex_lock(&g_mutexTmrList);
    g_lTmrWaitingExit = 1;

    ulCnt = 0;
    while(g_TmrWorkingList != g_TmrWorkingList->next)
    {
        tmrNode = g_TmrWorkingList->next;

        tmrNode->next->prev = g_TmrWorkingList;
        g_TmrWorkingList->next = tmrNode->next;

        tmrNode->next = NULL;
        tmrNode->prev = NULL;
        dos_dmem_free(tmrNode);

        ulCnt++;
    }
    pthread_mutex_unlock(&g_mutexTmrList);

    close(g_lEpollFD);

    dos_printf("%s", "Timer Task Finished.");
    dos_printf("There are %d timer has been deleted.", ulCnt);

    return NULL;
}



S32 dos_tmr_start(DOS_TMR_ST *hTmrHandle
                    , U32 ulInterval
                    , VOID (*func)(U64 )
                    , U64 uLParam
                    , U32 ulType)
{
    TIMER_LIST_NODE_ST *tmrNode;

    /* 必须有回调函数 */
    if (!func)
    {
        DOS_ASSERT(0);
        return -1;
    }

    /* 如果定时器已经存在，就要先删除，然后在添加 */
    if (NULL != hTmrHandle && NULL != *hTmrHandle)
    {
        dos_tmr_stop(hTmrHandle);
    }

    /* 定时器类型 */
    if (ulType >= TIMER_NORMAL_BUTT)
    {
        DOS_ASSERT(0);
        return -1;
    }

    /* 检查资源 */
    if (g_lTmrCurrentCnt > DOS_TIMER_MAX_NUM)
    {
        DOS_ASSERT(0);
        return -1;
    }

    /* 分配资源 */
    tmrNode = (TIMER_LIST_NODE_ST *)dos_dmem_alloc(sizeof(TIMER_LIST_NODE_ST));
    if (!tmrNode)
    {
        DOS_ASSERT(0);
        return -1;
    }

    /* 初始化定时器 */
    dos_memzero(tmrNode, sizeof(TIMER_LIST_NODE_ST));
    tmrNode->tmrHandle.eType = ulType;
    tmrNode->tmrHandle.func = func;
    tmrNode->tmrHandle.ulData = uLParam;
    tmrNode->tmrHandle.lInterval = ulInterval;
    tmrNode->tmrHandle.ulTmrStatus = TIMER_STATUS_WAITING_ADD;
    tmrNode->lTimerPassby = ulInterval;
    tmrNode->tmrHandle.hTmr = hTmrHandle;

    logr_debug("Timer start. addr:%p, handle:%p", tmrNode, tmrNode->tmrHandle);

    /* 加入等待队列 */
    pthread_mutex_lock(&g_mutexTmrWaitAddList);
    if (g_TmrWaitAddList == g_TmrWaitAddList->next)
    {
        g_TmrWaitAddList->next = tmrNode;
        g_TmrWaitAddList->prev = tmrNode;
        tmrNode->next = g_TmrWaitAddList;
        tmrNode->prev = g_TmrWaitAddList;
    }
    else
    {
        tmrNode->next = g_TmrWaitAddList;
        tmrNode->prev = g_TmrWaitAddList->prev;
        g_TmrWaitAddList->prev->next = tmrNode;
        g_TmrWaitAddList->prev = tmrNode;
    }

    g_lTmrCurrentCnt++;

    pthread_mutex_unlock(&g_mutexTmrWaitAddList);

    *hTmrHandle = &tmrNode->tmrHandle;

    return 0;
}

S32 dos_tmr_stop(DOS_TMR_ST *hTmrHandle)
{
    TIMER_LIST_NODE_ST *tmrNode;
    DOS_TMR_ST hTmr = NULL;

    if (NULL == hTmrHandle)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (NULL == *hTmrHandle)
    {
        DOS_ASSERT(0);
        return -1;
    }

    hTmr = *hTmrHandle;

    /* 还没加入工作队列，就直接删除 */
    if (TIMER_STATUS_WAITING_ADD == hTmr->ulTmrStatus)
    {
        pthread_mutex_lock(&g_mutexTmrWaitAddList);
        tmrNode = g_TmrWaitAddList->next;
        while(tmrNode != tmrNode->next)
        {
            if (&(tmrNode->tmrHandle) != hTmr)
            {
                continue;
            }

            tmrNode->next->prev = tmrNode->prev;
            tmrNode->prev->next = tmrNode->next;

            tmrNode->next = NULL;
            tmrNode->prev = NULL;

            logr_debug("Timer droped. Status: Waiting. handle:%p", tmrNode->tmrHandle);

            dos_dmem_free(tmrNode);
            tmrNode = NULL;

            break;

        }
        pthread_mutex_unlock(&g_mutexTmrWaitAddList);

        if (g_lTmrCurrentCnt <= 0)
        {
            DOS_ASSERT(0);
            g_lTmrCurrentCnt = 0;
        }
        else
        {
            g_lTmrCurrentCnt--;
        }
    }
    else /* 其他状态，就直接给置delete状态 */
    {
        pthread_mutex_lock(&g_mutexTmrList);
        hTmr->ulTmrStatus = TIMER_STATUS_WAITING_DEL;
        pthread_mutex_unlock(&g_mutexTmrList);

        /* 这个地方不要维护定时器个数 */
    }

    *hTmrHandle = NULL;

    return 0;
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

