/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_audit.h
 *
 *  创建时间: 2015年8月6日10:03:57
 *  作    者: Larry
 *  描    述: 业务控制模块审计相关功能集合
 *  修改历史:
 */

/*
 * 大致分为两大类，自然时间执行；周期执行。
 * 周期执行审计任务工作大致如下:
 *    1. 10分钟审计任务；
 *    2. 30分钟审计任务
 *    3. 60分钟审计任务
 *    4. 120分钟审计任务
 *    5. 1天审计任务
 *  初始化时记录时间戳，之后每秒钟获取一次时间戳，使用当前时间戳和初
 *  始化的时间戳之差和周期求余，如果为0就需要执行对应的任务
 *
 * 自然事件执行如下:
 *    在整点时执行，需要注意偏移量
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#include <dos.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"

typedef enum tagAuditType{
    SC_AUDIT_TYPE_CYCLE = 0,  /* 循环执行，以系统启动顺序为准 */
    SC_AUDIT_TYPE_N_HOUR,     /* 自然小时 */
    SC_AUDIT_TYPE_N_2HOUR,    /* 自然两小时 */
    SC_AUDIT_TYPE_N_DAY,      /* 自然天 */
    SC_AUDIT_TYPE_N_WEEK,     /* 自然周 */
    SC_AUDIT_TYPE_N_MONTH,    /* 自然月 */
    SC_AUDIT_TYPE_N_YEAR,     /* 自然年 */
}SC_AUDIT_TYPE_EN;

typedef enum tagAuditCycle
{
    SC_AUDIT_CYCLE_10MIN  = 10 * 60,
    SC_AUDIT_CYCLE_30MIN  = 30 * 60,
    SC_AUDIT_CYCLE_60MIN  = 60 * 60,
    SC_AUDIT_CYCLE_120MIN = 120 * 60,
    SC_AUDIT_CYCLE_1DAY   = 24 * 60 * 60,
}SC_AUDIT_CYCLE_EN;

typedef struct tagAuditTask
{
    /* 类型 refer SC_AUDIT_TYPE_EN */
    U32 ulType;

    /* 时间偏移,如果type为SC_AUDIT_TYPE_CYCLE，就是周期，其他则为偏移量
        如果表达偏移量，则规则如下: 偏移量的单位比类型的单位小1， 如:
        type为SC_AUDIT_TYPE_N_HOUR，则偏移量为0-59秒，对于范围不确定的，如
        闰月，大月，小月，则偏移量为最小值*/
    U32 ulOffset;

    /* 任务主函数 */
    U32 (*task)(U32 ulType, VOID *ptr);

    /* 任务名称 */
    S8  *pszName;
}SC_AUDIT_TASK_ST;

static U32 g_ulWaitingStop = DOS_TRUE;
static U32 g_ulStartTimestamp = 0;
static pthread_t g_pthAuditTask;

/* 注册审计任务 */
static SC_AUDIT_TASK_ST g_stAuditTaskList[] =
{
    {SC_AUDIT_TYPE_CYCLE,    SC_AUDIT_CYCLE_60MIN,  sc_acd_agent_audit,         "Agent audit"}
};

static VOID *sc_audit_task(VOID *ptr)
{
    U32 ulCurrentTime = 0;
    U32 i;
    struct tm *t = NULL;
    SC_AUDIT_TASK_ST *pstAuditTask = NULL;

    g_ulWaitingStop = DOS_FALSE;

    while (1)
    {
        dos_task_delay(1000);
        if (g_ulWaitingStop)
        {
            break;
        }

        ulCurrentTime = time(NULL);
        t = localtime((time_t *)&ulCurrentTime);
        pstAuditTask = NULL;

        for (i=0; i<sizeof(g_stAuditTaskList)/sizeof(SC_AUDIT_TASK_ST); i++)
        {
            switch (g_stAuditTaskList[i].ulType)
            {
                case SC_AUDIT_TYPE_CYCLE:
                    if (((g_ulStartTimestamp - ulCurrentTime) / g_stAuditTaskList[i].ulOffset))
                    {
                        break;
                    }

                    pstAuditTask = &g_stAuditTaskList[i];
                    break;

                case SC_AUDIT_TYPE_N_HOUR:
                    if (g_stAuditTaskList[i].ulOffset >= 59)
                    {
                        if (0 == t->tm_min)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    else
                    {
                        if (t->tm_min == g_stAuditTaskList[i].ulOffset)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    break;

                case SC_AUDIT_TYPE_N_2HOUR:
                    if (g_stAuditTaskList[i].ulOffset >= 119)
                    {
                        if (0 == t->tm_min && 0 == t->tm_hour/2)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    else
                    {
                        if (g_stAuditTaskList[i].ulOffset >= 60)
                        {
                            if (t->tm_hour/2
                                && t->tm_min == g_stAuditTaskList[i].ulOffset - 60)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == t->tm_hour/2
                                && t->tm_min == g_stAuditTaskList[i].ulOffset)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                    }
                    break;

                case SC_AUDIT_TYPE_N_DAY:
                    if (g_stAuditTaskList[i].ulOffset >= 23)
                    {
                        if (0 == t->tm_hour)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    else
                    {
                        if (t->tm_hour == g_stAuditTaskList[i].ulOffset)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    break;

                case SC_AUDIT_TYPE_N_WEEK:
                    if (g_stAuditTaskList[i].ulOffset >= 6)
                    {
                        if (0 == t->tm_wday)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    else
                    {
                        if (t->tm_wday == g_stAuditTaskList[i].ulOffset)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    break;

                case SC_AUDIT_TYPE_N_MONTH:
                    if (g_stAuditTaskList[i].ulOffset >= 28)
                    {
                        if (0 == t->tm_mday)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    else
                    {
                        if (t->tm_mday == g_stAuditTaskList[i].ulOffset)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    break;

                case SC_AUDIT_TYPE_N_YEAR:
                    if (g_stAuditTaskList[i].ulOffset >= 365)
                    {
                        if (0 == t->tm_yday)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    else
                    {
                        if (t->tm_yday == g_stAuditTaskList[i].ulOffset)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    break;

                default:
                    DOS_ASSERT(0);
                    break;
            }

            if (pstAuditTask)
            {
                sc_logr_notice(SC_AUDIT, "Start audit. Current Timestamp:%u, Type: %u Offset: %u, Task: %s(Handle:%P)"
                                    , ulCurrentTime
                                    , pstAuditTask->ulType
                                    , pstAuditTask->ulOffset
                                    , pstAuditTask->pszName
                                    , pstAuditTask->task);

                if (pstAuditTask->task)
                {
                    pstAuditTask->task(pstAuditTask->ulType, NULL);
                }
            }
        }
    }

    return 0;
}

U32 sc_audit_init()
{
    g_ulWaitingStop = DOS_TRUE;
    g_ulStartTimestamp = time(NULL);

    return DOS_SUCC;
}

U32 sc_audit_start()
{
    if (pthread_create(&g_pthAuditTask, NULL, sc_audit_task, NULL) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_audit_stop()
{
    g_ulWaitingStop = DOS_TRUE;

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


