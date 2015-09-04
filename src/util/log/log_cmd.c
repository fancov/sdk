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

S32 cli_set_log_level(U32 ulIndex, S32 argc, S8 **argv)
{
    U32   ulLeval;
    S8   szBuffer[128];

    if (2 == argc)
    {
        ulLeval = (U32)atol(argv[1]);
        if (ulLeval >= LOG_LEVEL_INVAILD)
        {
            goto usage;
        }

        if (dos_log_set_cli_level(ulLeval) < 0)
        {
            cli_out_string(ulIndex, (S8 *)"Setting failed.\r\n");
            return -1;
        }

        dos_snprintf(szBuffer, sizeof(szBuffer), "Setting successfully. Change to: %u\r\n", ulLeval);
        cli_out_string(ulIndex, szBuffer);
    }
    else if (3 == argc)
    {
        ulLeval = (U32)atol(argv[2]);
        if (ulLeval >= LOG_LEVEL_INVAILD)
        {
            goto usage;
        }

        if (0 == dos_stricmp(argv[1], "hb"))
        {
#if (INCLUDE_BH_ENABLE)
            hb_log_set_level(ulLeval);
#endif
        }
        else if (0 == dos_stricmp(argv[1], "cli"))
        {
#if (INCLUDE_DEBUG_CLI || INCLUDE_DEBUG_CLI_SERVER)
            cli_log_set_level(ulLeval);
#endif
        }
        else if (0 == dos_stricmp(argv[1], "timer"))
        {}
        else if (0 == dos_stricmp(argv[1], "memory"))
        {}
    }

    return 0;
usage:
    cli_out_string(ulIndex, (S8 *)"Invalid parameters\r\n");
    cli_out_string(ulIndex, (S8 *)"Usage:\r\n");
    cli_out_string(ulIndex, (S8 *)"    debug [<module>] {0|1|2|3|4|5|6|7} \r\n");

    return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

