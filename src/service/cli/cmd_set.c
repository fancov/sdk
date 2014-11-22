/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  cmd_set.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现命令行服务器相关功能
 *     History:
 */

#include <dos.h>
#include <sys/un.h>
#include "cli_server.h"

#if INCLUDE_DEBUG_CLI_SERVER

extern S32 cli_enter_log_mode(U32 ulIndex, S32 argc, S8 **argv);
extern S32 cli_exit_log_mode(U32 ulIndex, S32 argc, S8 **argv);
extern S32 cli_cmd_mem(U32 ulIndex, S32 argc, S8 **argv);
extern S32 cli_server_process_print(U32 ulIndex, S32 argc, S8 **argv);

COMMAND_ST g_stConfigCommand[] = {
    {NULL, "^ada",          "Enter the log mode",              cli_enter_log_mode},
    {NULL, "memory",        "Show memory usage",               cli_cmd_mem},
    {NULL, "process",       "Show process info",               cli_server_process_print}
};

COMMAND_ST g_stLogCommand[] = {
    {NULL, "exit",          "Exit the log mode",               cli_exit_log_mode}
};

COMMAND_GROUP_ST g_stCmdRootGrp[] = {
    /* 配置级别 */
    {NULL, g_stConfigCommand,   sizeof(g_stConfigCommand)/sizeof(COMMAND_ST)},

    /* log打印级别 */
    {NULL, g_stLogCommand,      sizeof(g_stLogCommand)/sizeof(COMMAND_ST)}
};


U32 cli_cmdset_get_group_num()
{
    return (sizeof(g_stCmdRootGrp)/sizeof(COMMAND_GROUP_ST));
}

#endif
