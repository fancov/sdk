/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  task.c
 *
 *  Created on: 2014-11-6
 *      Author: Larry
 *        Todo: 线程管理相关实现
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dosutil.h>
#include "task.h"

struct tagThreadMngtCB astThreadList[]={
#if INCLUDE_SERVICE_TIMER
    {tmr_task_init,       tmr_task_start,       tmr_task_stop,       NULL,          "timer"},
#endif

#if (INCLUDE_DEBUG_CLI_SERVER)
    {telnetd_init,        telnetd_start,        telnetd_stop,        NULL,           "telnetd"},
    {cli_server_init,     cli_server_start,     cli_server_stop,     NULL,           "cli-server"},
#endif

#if (INCLUDE_DEBUG_CLI)
    {debug_cli_init,      debug_cli_start,      debug_cli_stop,      NULL,           "cli_client"},
#endif

#if INCLUDE_SYSLOG_ENABLE
    {dos_log_init,        dos_log_start,        dos_log_stop         NULL,           "log"}
#endif
};

S32 dos_threads_init()
{
    U32 i;

    dos_printf("%s", "Start init the thread list ...");

    for (i=0; i<sizeof(astThreadList)/sizeof(struct tagThreadMngtCB); i++)
    {
        if (astThreadList[i].init)
        {
            if (astThreadList[i].init() < 0)
            {
                dos_printf("Exception accoured while init the task %s(%d)"
                        , astThreadList[i].pszThreadName, i);
                exit(0);
            }
        }
    }

    dos_printf("%s", "Init the thread list finished.");
    return 0;
}

S32 dos_thread_start()
{
    U32 i;

    dos_printf("%s", "Start init the thread list ...");

    for (i=0; i<sizeof(astThreadList)/sizeof(struct tagThreadMngtCB); i++)
    {
        if (astThreadList[i].init)
        {
            if (astThreadList[i].init() < 0)
            {
                dos_printf("Exception accoured while init the task %s(%d)"
                        , astThreadList[i].pszThreadName, i);
                exit(0);
            }
        }
    }

    dos_printf("%s", "Init the thread list finished.");
    return 0;
}

S32 dos_thread_stop()
{
    return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
