/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  log_cmd.c
 *
 *  Created on: 2014-11-11
 *      Author: Larry
 *        Todo: 日志模块命令行回调函数实现
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if INCLUDE_DEBUG_CLI

S32 cli_set_log_level(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulLeval;

    if (argc <= 1)
    {
        goto usage;
    }

    ulLeval = (U32)atol(argv[1]);
    if (ulLeval >= LOG_LEVEL_INVAILD)
    {
        goto usage;
    }

    if (dos_log_set_cli_level(ulLeval) < 0)
    {
        cli_out_string(ulIndex, "Setting failed.\r\n");
        return -1;
    }

    cli_out_string(ulIndex, "Setting successfully.\r\n");

    return 0;
usage:
    cli_out_string(ulIndex, "Invalid parameters\r\n");
    cli_out_string(ulIndex, "Usage:\r\n");
    cli_out_string(ulIndex, "    debug {0|1|2|3|4|5|6|7} \r\n");

    return 0;
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

