/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  debug_cli.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现和统一管理平台的通讯相关功能
 *     History:
 */

#ifndef __DEBUG_CLI_H_
#define __DEBUG_CLI_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#if INCLUDE_DEBUG_CLI

#define UNIX_SOCKET_PATH "/ttcom/run/socket"
#define UNIX_SOCKET_NAME "cli-server.sock"

#define CLI_CMD_PROTO_VERSION "1.0"

extern S32 debug_cli_init(S32 _argc, S8 **_argv);
extern S32 debug_cli_start();
extern S32 debug_cli_stop();
extern S32 debug_cli_send_log(S8 * _ucMsg, S32 _lLength);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
