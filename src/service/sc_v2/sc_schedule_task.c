/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_schedule_task.c
 *
 *  创建时间: 2015年8月6日10:03:57
 *  作    者: Larry
 *  描    述: 业务控制模块审计相关功能集合
 *  修改历史:
 */

/**
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
#include <time.h>
#include "sc_def.h"
#include "sc_debug.h"

/** 定义是否在测试走去任务 */
#define SC_SCHEDULE_TEST 0

/** 定义周期任务的类型 */
typedef enum tagScheduleType{
    SC_SCHEDULE_TYPE_ONCE  = 0,  /**< 循环执行，以系统启动顺序为准，偏移量为s */
    SC_SCHEDULE_TYPE_CYCLE,      /**< 循环执行，以系统启动顺序为准，偏移量即为周期长度 */
    SC_SCHEDULE_TYPE_N_HOUR,     /**< 自然小时，偏移量为分钟 0-59 */
    SC_SCHEDULE_TYPE_N_2HOUR,    /**< 自然两小时，偏移量为分钟 0-199 */
    SC_SCHEDULE_TYPE_N_DAY,      /**< 自然天，偏移量为小时 0-23 */
    SC_SCHEDULE_TYPE_N_WEEK,     /**< 自然周，偏移量为天 0-6 */
    SC_SCHEDULE_TYPE_N_MONTH,    /**< 自然月，偏移量为天 0-31 */
    SC_SCHEDULE_TYPE_N_YEAR,     /**< 自然年，偏移量为天 0-365 */
}SC_AUDIT_TYPE_EN;

/** 定义一些常用时间 */
typedef enum tagScheduleCycle
{
    SC_SCHEDULE_CYCLE_5MIN   = 5 * 60,
    SC_SCHEDULE_CYCLE_10MIN  = 10 * 60,
    SC_SCHEDULE_CYCLE_30MIN  = 30 * 60,
    SC_SCHEDULE_CYCLE_60MIN  = 60 * 60,
    SC_SCHEDULE_CYCLE_120MIN = 120 * 60,
    SC_SCHEDULE_CYCLE_1DAY   = 24 * 60 * 60,
}SC_SCHEDULE_CYCLE_EN;

typedef struct tagScheduleTask
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
}SC_SCHEDULE_TASK_ST;

static U32 g_ulWaitingStop = DOS_TRUE;
static U32 g_ulStartTimestamp = 0;
static pthread_t g_pthAuditTask;

#if SC_SCHEDULE_TEST
U32 sc_test_startup(U32 ulType, VOID *ptr)
{
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SCH), "Test for startup. Startup timestamp: %u", g_ulStartTimestamp);

    return DOS_SUCC;
}
U32 sc_test_startup_delay(U32 ulType, VOID *ptr)
{
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SCH), "Test for startup delay. Startup timestamp: %u, Current: %u", g_ulStartTimestamp, time(NULL));

    return DOS_SUCC;
}

U32 sc_test_cycle(U32 ulType, VOID *ptr)
{
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SCH), "Test for startup cycle. Startup timestamp: %u, Current: %u", g_ulStartTimestamp, time(NULL));

    return DOS_SUCC;
}

U32 sc_test_hour(U32 ulType, VOID *ptr)
{
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SCH), "Test hour. Startup timestamp: %u, Current: %u", g_ulStartTimestamp, time(NULL));

    return DOS_SUCC;
}

U32 sc_test_hour_delay(U32 ulType, VOID *ptr)
{
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SCH), "Test hour delay. Startup timestamp: %u, Current: %u", g_ulStartTimestamp, time(NULL));

    return DOS_SUCC;
}
#endif

U32 sc_num_lmt_stat(U32 ulType, VOID *ptr);
U32 sc_num_lmt_update(U32 ulType, VOID *ptr);
U32 sc_stat_write(U32 ulType, VOID *ptr);
U32 sc_stat_syn(U32 ulType, VOID *ptr);
U32 sc_task_write_stat(U32 ulType, VOID *ptr);


/* 注册审计任务 */
static SC_SCHEDULE_TASK_ST g_stScheduleTaskList[] =
{
#if SC_SCHEDULE_TEST
    {SC_SCHEDULE_TYPE_ONCE,     0,                        sc_test_startup,            "Schedule test for once"},
    {SC_SCHEDULE_TYPE_ONCE,     20,                       sc_test_startup_delay,      "Schedule test for delay"},
    {SC_SCHEDULE_TYPE_CYCLE,    SC_SCHEDULE_CYCLE_10MIN,  sc_test_cycle,              "Schedule test for cycle"},
    {SC_SCHEDULE_TYPE_N_HOUR,   0,                        sc_test_hour,               "Schedule test for hour"},
    {SC_SCHEDULE_TYPE_N_HOUR,   1,                        sc_test_hour_delay,         "Schedule test for hour delay"},
#endif
    {SC_SCHEDULE_TYPE_N_DAY,    1,                        sc_num_lmt_stat,            "Number usage stat"},
    {SC_SCHEDULE_TYPE_N_DAY,    1,                        sc_num_lmt_update,          "Number limitation"},

    /* 6小时写统计数据到磁盘 */
    {SC_SCHEDULE_TYPE_CYCLE,    6 * 60 * 60,              sc_stat_write,              "Write stat info"},

    /* 20分钟同步数据到license模块 */
    {SC_SCHEDULE_TYPE_CYCLE,    20 * 60,                  sc_stat_syn,                "Stat data syn "},

    {SC_SCHEDULE_TYPE_CYCLE,    60,                       sc_task_write_stat,         "Write task stat"},
};


static VOID *sc_schedule_task(VOID *ptr)
{
    time_t ulCurrentTime = 0;
    time_t ulLastTime    = 0;
    U32 i, j;
    struct tm stTime;
    SC_SCHEDULE_TASK_ST *pstScheduleTask = NULL;

    g_ulWaitingStop = DOS_FALSE;

    while (1)
    {
        dos_task_delay(1000);
        if (g_ulWaitingStop)
        {
            break;
        }

        ulCurrentTime = time(NULL);

        if (0 == g_ulStartTimestamp)
        {
            g_ulStartTimestamp = ulCurrentTime;
            ulLastTime = ulCurrentTime - 1;
        }

        /* 可能出现时钟不同步，两次获取的时间戳之差大于1了，就需要检查着之间的每一秒 */
        for (j=ulLastTime+1; j<=ulCurrentTime; j++)
        {
            /* 根据当前时间戳来获取时间 */
            dos_memzero(&stTime, sizeof(stTime));
            localtime_r((time_t *)&j, &stTime);

            /* 检查所有定时任务 */
            for (i=0; i<sizeof(g_stScheduleTaskList)/sizeof(SC_SCHEDULE_TASK_ST); i++)
            {
                pstScheduleTask = NULL;

                switch (g_stScheduleTaskList[i].ulType)
                {
                    case SC_SCHEDULE_TYPE_ONCE:
                        if (g_stScheduleTaskList[i].ulOffset + g_ulStartTimestamp == ulCurrentTime)
                        {
                            pstScheduleTask = &g_stScheduleTaskList[i];
                        }
                        break;

                    case SC_SCHEDULE_TYPE_CYCLE:
                        if (((ulCurrentTime - g_ulStartTimestamp) % g_stScheduleTaskList[i].ulOffset)
                            || (g_ulStartTimestamp == ulCurrentTime))
                        {
                            break;
                        }

                        pstScheduleTask = &g_stScheduleTaskList[i];
                        break;

                    case SC_SCHEDULE_TYPE_N_HOUR:
                        if (g_stScheduleTaskList[i].ulOffset >= 59)
                        {
                            if (0 == stTime.tm_sec && 0 == stTime.tm_min)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec && stTime.tm_min == g_stScheduleTaskList[i].ulOffset)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        break;

                    case SC_SCHEDULE_TYPE_N_2HOUR:
                        if (g_stScheduleTaskList[i].ulOffset >= 119)
                        {
                            if (0 == stTime.tm_sec && 0 == stTime.tm_min && 0 == stTime.tm_hour%2)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        else
                        {
                            if (g_stScheduleTaskList[i].ulOffset >= 60)
                            {
                                if (0 == stTime.tm_sec
                                    && stTime.tm_hour%2
                                    && stTime.tm_min == g_stScheduleTaskList[i].ulOffset - 60)
                                {
                                    pstScheduleTask = &g_stScheduleTaskList[i];
                                }
                            }
                            else
                            {
                                if (0 == stTime.tm_sec
                                    && 0 == stTime.tm_hour%2
                                    && stTime.tm_min == g_stScheduleTaskList[i].ulOffset)
                                {
                                    pstScheduleTask = &g_stScheduleTaskList[i];
                                }
                            }
                        }
                        break;

                    case SC_SCHEDULE_TYPE_N_DAY:
                        if (g_stScheduleTaskList[i].ulOffset >= 23)
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && stTime.tm_hour == g_stScheduleTaskList[i].ulOffset)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        break;

                    case SC_SCHEDULE_TYPE_N_WEEK:
                        if (g_stScheduleTaskList[i].ulOffset >= 6)
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && 0 == stTime.tm_wday)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && stTime.tm_wday == g_stScheduleTaskList[i].ulOffset)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        break;

                    case SC_SCHEDULE_TYPE_N_MONTH:
                        if (g_stScheduleTaskList[i].ulOffset >= 28)
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && 0 == stTime.tm_mday)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && stTime.tm_mday == g_stScheduleTaskList[i].ulOffset)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        break;

                    case SC_SCHEDULE_TYPE_N_YEAR:
                        if (g_stScheduleTaskList[i].ulOffset >= 365)
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && 0 == stTime.tm_yday)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && stTime.tm_yday == g_stScheduleTaskList[i].ulOffset)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        break;

                    default:
                        DOS_ASSERT(0);
                        break;
                }

                if (pstScheduleTask)
                {
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SCH)
                                , "Start audit. Current Timestamp:%u, Type: %u Offset: %u, Task: %s(Handle:%p)"
                                , ulCurrentTime
                                , pstScheduleTask->ulType
                                , pstScheduleTask->ulOffset
                                , pstScheduleTask->pszName
                                , pstScheduleTask->task);

                    if (pstScheduleTask->task)
                    {
                        pstScheduleTask->task(pstScheduleTask->ulType, NULL);
                    }
                }
            }
        }

        ulLastTime = ulCurrentTime;
    }

    return 0;
}

U32 sc_schedule_init()
{
    g_ulWaitingStop = DOS_TRUE;

    return DOS_SUCC;
}

U32 sc_schedule_start()
{
    if (pthread_create(&g_pthAuditTask, NULL, sc_schedule_task, NULL) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_schedule_stop()
{
    g_ulWaitingStop = DOS_TRUE;

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


