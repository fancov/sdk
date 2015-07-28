/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  heartbeat.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 本文件定义心跳模块相关的申明
 *     History:
 */

#ifndef __HEARTBEAT_H__
#define __HEARTBEAT_H__


/* define local macros */
#define MAX_BUFF_LENGTH     1024

/* define heartbeat interval */
#define DEFAULT_HB_INTERVAL_MIN 3
#define DEFAULT_HB_INTERVAL_MAX 30

/* define mac fail count */
#define DEFAULT_HB_FAIL_CNT_MIN 10
#define DEFAULT_HB_FAIL_CNT_MAX 15

/* define heartbeat timeout length */
#define HB_TIMEOUT          3


/* 消息类型 */
enum HEARTBEAT_DATA_TYPE{
    HEARTBEAT_DATA_REG  = 0,
    HEARTBEAT_DATA_REG_RESPONSE,

    HEARTBEAT_DATA_UNREG,
    HEARTBEAT_DATA_UNREG_RESPONSE,

    HEARTBEAT_WARNING_SEND,
    HEARTBEAT_WARNING_SEND_RESPONSE,

    HEARTBEAT_DATA_HB,
};

/* 模块状态 */
enum PROCESS_HB_STATUS{
    PROCESS_HB_INIT,
    PROCESS_HB_WORKING,
    PROCESS_HB_DEINIT,

    PROCESS_HB_BUTT
};

#define HB_MAX_PROCESS_NAME_LEN DOS_MAX_PROCESS_NAME_LEN
#define HB_MAX_PROCESS_VER_LEN  DOS_MAX_PROCESS_VER_LEN

/* 心跳 */
typedef struct tagHeartbeatData{
    U32 ulCommand;                                  /* 控制命令字 */
    U32 ulLength;                                   /* 消息长度 */
    S8  szProcessName[HB_MAX_PROCESS_NAME_LEN];     /* 进程名称 */
    S8  szProcessVersion[HB_MAX_PROCESS_VER_LEN];   /* 版本号 */
    S8  szResv[8];                                  /* 保留 */
}HEARTBEAT_DATA_ST;

/* 控制块 */
typedef struct tagProcessInfo{
    U32                ulStatus;                                  /* 当前进程状态 */

    socklen_t          ulPeerAddrLen;                             /* 对端地址长度 */
    struct sockaddr_un stPeerAddr;                                /* 对端地址 */
    S32                lSocket;                                   /* 对端socket */

    S8                 szProcessName[HB_MAX_PROCESS_NAME_LEN];    /* 进程名称 */
    S8                 szProcessVersion[HB_MAX_PROCESS_VER_LEN];  /* 版本号 */

    U32                ulHBCnt;
    U32                ulHBFailCnt;                               /* 失败统计 */
#if INCLUDE_BH_CLIENT
    DOS_TMR_ST         hTmrRegInterval;                           /* 注册间隔 */
    DOS_TMR_ST         hTmrSendInterval;                          /* 发送间隔 */
    DOS_TMR_ST         hTmrRecvTimeout;                           /* 发送超时定时器 */
#endif
#if INCLUDE_BH_SERVER
    U32                ulProcessCBNo;                             /* 控制块编号 */
    U32                ulVilad;                                   /* 该控制块是否有效 */
    U32                ulActive;                                  /* 该进程是否处于激活状态 */
    DOS_TMR_ST         hTmrHBTimeout;                             /* 心跳超市定时器，每次收到心跳重启 */
#endif
}PROCESS_INFO_ST;

S32 hb_heartbeat_proc(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_send_heartbeat(PROCESS_INFO_ST *pstProcessInfo);

#if INCLUDE_BH_SERVER
S32 hb_recv_external_warning(VOID *pMsg);
S32 hb_unreg_proc(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_reg_proc(PROCESS_INFO_ST *pstProcessInfo);
VOID* hb_external_warning_proc(VOID* ptr);
#endif
#if INCLUDE_BH_CLIENT
S32 hb_send_reg(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_unreg_response_proc(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_reg_response_proc(PROCESS_INFO_ST *pstProcessInfo);
#endif
#endif

