/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  cli_server.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现命令行服务器相关功能
 *     History:
 */

#ifndef __CLI_SERVER_H__
#define __CLI_SERVER_H__

#define MAX_RECV_BUFF 1400

typedef struct tagProcessInfoNode
{
    U32  ulIndex;              /* 控制块索引 */
    BOOL bVaild;              /* 是否启用 */
    BOOL bActive;             /* 是否处于active状态 */

    /* 进程名 */
    S8   szProcessName[DOS_MAX_PROCESS_NAME_LEN];

    /* 进程版本号 */
    S8   szProcessVersion[DOS_MAX_PROCESS_VER_LEN];

    /* 客户端网络地址的长度 */
    U32  uiClientAddrLen;
    /* 客户端网络地址 */
    struct sockaddr_un stClientAddr;
}PROCESS_INFO_NODE_ST;

S32 cli_server_cmd_analyse(U32 ulClientIndex, U32 ulMode, S8 *szBuffer, U32 ulLength);
S32 cli_server_send_broadcast_cmd(U32 ulIndex, S8 *pszCmd, U32 ullength);
U32 cli_cmdset_get_group_num();

#endif
