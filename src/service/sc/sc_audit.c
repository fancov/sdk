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
 * 定期审计任务工作大致如下:
 *    1. 10分钟审计任务；
 *    2. 30分钟审计任务
 *    3. 60分钟审计任务
 *    4. 120分钟审计任务
 *    5. 1天审计任务
 *
 *  初始化时记录时间戳，之后每秒钟获取一次时间戳，使用当前时间戳和初
 *  始化的时间戳之差和周期求余，如果为0就需要执行对应的任务
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#include <dos.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"

typedef enum tagAuditCycel
{
    SC_AUDIT_CYCLE_10MIN  = 10 * 60,
    SC_AUDIT_CYCLE_30MIN  = 30 * 60,
    SC_AUDIT_CYCLE_60MIN  = 60 * 60,
    SC_AUDIT_CYCLE_120MIN = 120 * 60,
    SC_AUDIT_CYCLE_1DAY   = 24 * 60 * 60,
}SC_AUDIT_CYCLE_EN;

typedef struct tagAuditTask
{
    U32 ulCycel;                         /* 审计任务周期 */
    U32 (*task)(U32 ulCycle, VOID *ptr); /* 任务主函数 */
    S8  *pszName;                        /* 任务名称 */
}SC_AUDIT_TASK_ST;

static U32 g_ulWaitingStop = DOS_TRUE;
static U32 g_ulStartTimestamp = 0;
static pthread_t g_pthAuditTask;

/* 注册审计任务 */
static SC_AUDIT_TASK_ST g_stAuditTaskList[] =
{
    {SC_AUDIT_CYCLE_60MIN,  sc_acd_agent_audit,         "Agent audit"}
};

static VOID *sc_audit_task(VOID *ptr)
{
    U32 ulCurrentTime = 0;
    U32 i;
    g_ulWaitingStop = DOS_FALSE;

    while (1)
    {
        dos_task_delay(1000);
        if (g_ulWaitingStop)
        {
            break;
        }

        ulCurrentTime = time(NULL);

        for (i=0; i<sizeof(g_stAuditTaskList)/sizeof(SC_AUDIT_TASK_ST); i++)
        {
            if (((g_ulStartTimestamp - ulCurrentTime) / g_stAuditTaskList[i].ulCycel))
            {
                continue;
            }

            sc_logr_notice(SC_AUDIT, "Start audit. Current Timestamp:%u, Cycle: %u, Task: %s(Handle:%P)"
                    , ulCurrentTime
                    , g_stAuditTaskList[i].ulCycel
                    , g_stAuditTaskList[i].pszName
                    , g_stAuditTaskList[i].task);
            if (g_stAuditTaskList[i].task)
            {
                g_stAuditTaskList[i].task(g_stAuditTaskList[i].ulCycel, NULL);
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


