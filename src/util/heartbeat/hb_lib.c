/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  hb_lib.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 本文件定义心跳服务器相关操作函数集
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "heartbeat.h"

#if INCLUDE_BH_SERVER
VOID hb_recv_timeout(U64 ulParam);
U32 hb_get_max_send_interval();
S32 hb_get_max_timeout();
#endif
#if INCLUDE_BH_CLIENT
VOID hb_heartbeat_recv_timeout(U64 uLParam);
VOID hb_heartbeat_interval_timeout(U64 uLParam);
VOID hb_reg_timeout(U64 uLProcessCB);
U32 hb_get_max_send_interval();
VOID hb_set_connect_flg(BOOL bConnStatus);
#endif


#if INCLUDE_BH_ENABLE


/**
 * 函数：S32 hb_send_msg(U8 *pszBuff, U32 ulBuffLen, struct sockaddr_un *pstAddr, U32 ulAddrLen)
 * 功能：向pstAddr指定的客户端发送消息，内容为pszBuff，长度为ulBuffLen
 * 参数：
 *      U8 *pszBuff：消息体
 *      U32 ulBuffLen：消息长度
 *      struct sockaddr_un *pstAddr：对端地址
 *      U32 ulAddrLen：对端地址长度
 * 返回值：成功返回0.失败返回－1
 */
S32 hb_send_msg(U8 *pszBuff, U32 ulBuffLen, struct sockaddr_un *pstAddr, U32 ulAddrLen, S32 lSocket)
{
    S32 lRet;

    if (lSocket < 0)
    {
        logr_warning("%s", "Invalid socket while send heartbeat to process.");
        DOS_ASSERT(0);
        return -1;
    }

    lRet = sendto(lSocket, pszBuff, ulBuffLen
            , 0, (struct sockaddr *)pstAddr
            , ulAddrLen);
    if (ulBuffLen != lRet)
    {
        logr_warning("Exception occurred while send hreatbeat msg.(%d)", errno);
        DOS_ASSERT(0);

#if INCLUDE_BH_CLIENT
        hb_set_connect_flg(DOS_FALSE);
#endif
        return -1;
    }

    return 0;
}

/**
 *  函数：S32 hb_send_heartbeat(PROCESS_INFO_ST *pstProcessInfo)
 *  功能：向监控进程发送心跳消息
 *  参数：
 *      PROCESS_INFO_ST *pstModInfo：当前进程的控制块
 *  返回值：成功返回0.失败返回－1
 */
S32 hb_send_heartbeat(PROCESS_INFO_ST *pstProcessInfo)
{
    HEARTBEAT_DATA_ST stData;

    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return -1;
    }

    memset((VOID*)&stData, 0, sizeof(stData));
    stData.ulCommand = HEARTBEAT_DATA_HB;
    strncpy(stData.szProcessName, pstProcessInfo->szProcessName, sizeof(stData.szProcessName));
    stData.szProcessName[sizeof(stData.szProcessName) - 1] = '\0';
    strncpy(stData.szProcessVersion, pstProcessInfo->szProcessVersion, sizeof(stData.szProcessVersion));
    stData.szProcessVersion[sizeof(stData.szProcessVersion) - 1] = '\0';

    pstProcessInfo->ulHBCnt++;

#if INCLUDE_BH_CLIENT
     dos_tmr_start(&pstProcessInfo->hTmrRecvTimeout
                , HB_TIMEOUT * 1000
                , hb_heartbeat_recv_timeout
                , 0
                , TIMER_NORMAL_NO_LOOP);

    logr_debug("Start the recv timerout timer for the process \"%s\". %p"
                , pstProcessInfo->szProcessName, pstProcessInfo->hTmrRecvTimeout);
#else
    logr_debug("Response heartbeat for hb client \"%s\".", pstProcessInfo->szProcessName);
#endif

    /* 发送数据 */
    return hb_send_msg((U8*)&stData, sizeof(stData), &pstProcessInfo->stPeerAddr, pstProcessInfo->ulPeerAddrLen, pstProcessInfo->lSocket);
}

#endif

#if INCLUDE_BH_CLIENT
/**
 *  函数：S32 hb_send_reg(PROCESS_INFO_ST *pstProcessInfo)
 *  功能：向监控进程发送注册消息
 *  参数：
 *      PROCESS_INFO_ST *pstModInfo：当前进程的控制块
 *  返回值：成功返回0.失败返回－1
 */
S32 hb_send_reg(PROCESS_INFO_ST *pstProcessInfo)
{
    HEARTBEAT_DATA_ST stData;

    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return -1;
    }

    dos_tmr_start(&pstProcessInfo->hTmrRegInterval
                , HB_TIMEOUT * 1000
                , hb_reg_timeout
                , 0
                , TIMER_NORMAL_NO_LOOP);
    logr_debug("%s", "Heartbeat client start register timer.");


    memset((VOID*)&stData, 0, sizeof(stData));
    stData.ulCommand = HEARTBEAT_DATA_REG;
    strncpy(stData.szProcessName, dos_get_process_name(), sizeof(stData.szProcessName));
    stData.szProcessName[sizeof(stData.szProcessName) - 1] = '\0';
    strncpy(stData.szProcessVersion, dos_get_process_version(), sizeof(stData.szProcessVersion));
    stData.szProcessVersion[sizeof(stData.szProcessVersion) - 1] = '\0';

    logr_debug("%s", "Heartbeat client send register to hb server.");

    return hb_send_msg((U8 *)&stData
                    , sizeof(HEARTBEAT_DATA_ST)
                    , &pstProcessInfo->stPeerAddr
                    , pstProcessInfo->ulPeerAddrLen
                    , pstProcessInfo->lSocket);
}


/**
 *  函数：S32 hb_send_unreg(PROCESS_INFO_ST *pstProcessInfo)
 *  功能：向监控进程发送取消注册消息
 *  参数：
 *      PROCESS_INFO_ST *pstModInfo：当前进程的控制块
 *  返回值：成功返回0.失败返回－1
 */
S32 hb_send_unreg(PROCESS_INFO_ST *pstProcessInfo)
{
    HEARTBEAT_DATA_ST stData;

    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return -1;
    }

    memset((VOID*)&stData, 0, sizeof(stData));
    stData.ulCommand = HEARTBEAT_DATA_UNREG;
    strncpy(stData.szProcessName, dos_get_process_name(), sizeof(stData.szProcessName));
    stData.szProcessName[sizeof(stData.szProcessName) - 1] = '\0';
    strncpy(stData.szProcessVersion, dos_get_process_version(), sizeof(stData.szProcessVersion));
    stData.szProcessVersion[sizeof(stData.szProcessVersion) - 1] = '\0';

    return hb_send_msg((U8 *)&stData
                    , sizeof(HEARTBEAT_DATA_ST)
                    , &pstProcessInfo->stPeerAddr
                    , pstProcessInfo->ulPeerAddrLen
                    , pstProcessInfo->lSocket);
}


/**
 *  函数：S32 hb_reg_responce_proc(PROCESS_INFO_ST *pstModInfo)
 *  功能：处理监控进程发送过来的注册响应消息
 *  参数：
 *      PROCESS_INFO_ST *pstModInfo：当前进程的控制块
 *  返回值：成功返回0.失败返回－1
 */
S32 hb_reg_responce_proc(PROCESS_INFO_ST *pstProcessInfo)
{
    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return -1;
    }

    logr_debug("Process registe response.%p", pstProcessInfo->hTmrRegInterval);

    switch (pstProcessInfo->ulStatus)
    {
        case PROCESS_HB_INIT:

            /* 不需要break */
        case PROCESS_HB_WORKING:
            pstProcessInfo->ulStatus = PROCESS_HB_WORKING;

            if (pstProcessInfo->hTmrRegInterval)
            {
                logr_debug("%s", "Stop registe timer.");
                dos_tmr_stop(&pstProcessInfo->hTmrRegInterval);
                pstProcessInfo->hTmrRegInterval = NULL;
            }

            if (!pstProcessInfo->hTmrSendInterval)
            {
                dos_tmr_start(&pstProcessInfo->hTmrSendInterval
                            , hb_get_max_send_interval() * 1000
                            , hb_heartbeat_interval_timeout
                            , 0
                            , TIMER_NORMAL_LOOP);
            }

            break;
        case PROCESS_HB_DEINIT:
            hb_send_unreg(pstProcessInfo);
            DOS_ASSERT(0);
            break;
        default:
            DOS_ASSERT(0);
            break;
    }

    return 0;
}

/**
 *  函数：S32 hb_unreg_responce_proc(PROCESS_INFO_ST *pstModInfo)
 *  功能：处理监控进程发送过来的取消注册响应消息
 *  参数：
 *      PROCESS_INFO_ST *pstModInfo：当前进程的控制块
 *  返回值：成功返回0.失败返回－1
 */
S32 hb_unreg_responce_proc(PROCESS_INFO_ST *pstProcessInfo)
{
    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return -1;
    }

    switch (pstProcessInfo->ulStatus)
    {
        case PROCESS_HB_INIT:
            DOS_ASSERT(0);
            /* 不需要break */
        case PROCESS_HB_WORKING:
            DOS_ASSERT(0);
        case PROCESS_HB_DEINIT:
            hb_send_unreg(pstProcessInfo);
            pstProcessInfo->ulStatus = PROCESS_HB_DEINIT;
            break;
        default:
            DOS_ASSERT(0);
            break;
    }

    return 0;
}

#endif

#if INCLUDE_BH_SERVER


/**
 *  函数：S32 hb_send_reg_responce(PROCESS_INFO_ST *pstProcessInfo)
 *  功能：向监控进程发送注册消息
 *  参数：
 *      PROCESS_INFO_ST *pstModInfo：当前进程的控制块
 *  返回值：成功返回0.失败返回－1
 */
S32 hb_send_reg_responce(PROCESS_INFO_ST *pstProcessInfo)
{
    HEARTBEAT_DATA_ST stData;

    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return -1;
    }

    logr_debug("Send reg responce to the process \"%s\"", pstProcessInfo->szProcessName);

    memset((VOID*)&stData, 0, sizeof(stData));
    stData.ulCommand = HEARTBEAT_DATA_REG_RESPONCE;
    strncpy(stData.szProcessName, pstProcessInfo->szProcessName, sizeof(stData.szProcessName));
    stData.szProcessName[sizeof(stData.szProcessName) - 1] = '\0';
    strncpy(stData.szProcessVersion, pstProcessInfo->szProcessVersion, sizeof(stData.szProcessVersion));
    stData.szProcessVersion[sizeof(stData.szProcessVersion) - 1] = '\0';

    return hb_send_msg((U8*)&stData, sizeof(stData), &pstProcessInfo->stPeerAddr, pstProcessInfo->ulPeerAddrLen, pstProcessInfo->lSocket);
}


/**
 *  函数：S32 hb_send_unreg_responce(PROCESS_INFO_ST *pstProcessInfo)
 *  功能：向监控进程发送取消注册消息
 *  参数：
 *      PROCESS_INFO_ST *pstModInfo：当前进程的控制块
 *  返回值：成功返回0.失败返回－1
 */
S32 hb_send_unreg_responce(PROCESS_INFO_ST *pstProcessInfo)
{
    HEARTBEAT_DATA_ST stData;

    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return -1;
    }

    memset((VOID*)&stData, 0, sizeof(stData));
    stData.ulCommand = HEARTBEAT_DATA_UNREG;
    strncpy(stData.szProcessName, pstProcessInfo->szProcessName, sizeof(stData.szProcessName));
    stData.szProcessName[sizeof(stData.szProcessName) - 1] = '\0';
    strncpy(stData.szProcessVersion, pstProcessInfo->szProcessVersion, sizeof(stData.szProcessVersion));
    stData.szProcessVersion[sizeof(stData.szProcessVersion) - 1] = '\0';

    return hb_send_msg((U8*)&stData, sizeof(stData), &pstProcessInfo->stPeerAddr, pstProcessInfo->ulPeerAddrLen, pstProcessInfo->lSocket);
}


/**
 *  函数：S32 hb_reg_proc(PROCESS_INFO_ST *pstProcessInfo)
 *  功能：处理监控进程发送过来的注册响应消息
 *  参数：
 *      PROCESS_INFO_ST *pstModInfo：当前进程的控制块
 *  返回值：成功返回0.失败返回－1
 */
S32 hb_reg_proc(PROCESS_INFO_ST *pstProcessInfo)
{
    U32 ulHBTimeout;

    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (!pstProcessInfo->ulVilad)
    {
        DOS_ASSERT(0);
        return -1;
    }

    switch (pstProcessInfo->ulStatus)
    {
        case PROCESS_HB_INIT:

            /* 不需要break */
        case PROCESS_HB_WORKING:
            pstProcessInfo->ulStatus = PROCESS_HB_WORKING;
            hb_send_reg_responce(pstProcessInfo);
            break;
        case PROCESS_HB_DEINIT:
            pstProcessInfo->ulStatus = PROCESS_HB_INIT;
            hb_send_reg_responce(pstProcessInfo);
            break;
        default:
            DOS_ASSERT(0);
            return -1;
    }

    ulHBTimeout = hb_get_max_timeout();

    dos_tmr_start(&pstProcessInfo->hTmrHBTimeout
                        , hb_get_max_timeout() * 1000
                        , hb_recv_timeout
                        , (U64)pstProcessInfo->ulProcessCBNo
                        , TIMER_NORMAL_NO_LOOP);
    logr_debug("Start the heartbeat timeout timer for process \"%s\"", pstProcessInfo->szProcessName);

    return 0;
}


/**
 *  函数：S32 hb_unreg_proc(PROCESS_INFO_ST *pstProcessInfo)
 *  功能：处理监控进程发送过来的取消注册响应消息
 *  参数：
 *      PROCESS_INFO_ST *pstModInfo：当前进程的控制块
 *  返回值：成功返回0.失败返回－1
 */
S32 hb_unreg_proc(PROCESS_INFO_ST *pstProcessInfo)
{
    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return -1;
    }

    switch (pstProcessInfo->ulStatus)
    {
        case PROCESS_HB_INIT:
            /* 不需要break */
        case PROCESS_HB_WORKING:
            /* 不需要break */
        case PROCESS_HB_DEINIT:
            hb_send_unreg_responce(pstProcessInfo);
            dos_tmr_stop(&pstProcessInfo->hTmrHBTimeout);
            pstProcessInfo->ulStatus = PROCESS_HB_DEINIT;
            pstProcessInfo->hTmrHBTimeout = NULL;
            break;
        default:
            DOS_ASSERT(0);
            return -1;
    }

    return 0;
}
#endif

#if INCLUDE_BH_ENABLE

/**
 *  函数：S32 hb_heartbeat_proc(PROCESS_INFO_ST *pstProcessInfo)
 *  功能：处理监控进程发送过来的心跳消息
 *  参数：
 *      PROCESS_INFO_ST *pstModInfo：当前进程的控制块
 *  返回值：成功返回0.失败返回－1
 */
S32 hb_heartbeat_proc(PROCESS_INFO_ST *pstProcessInfo)
{
    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return -1;
    }

    logr_debug("Start process heartbeat msg. Status:%d.", pstProcessInfo->ulStatus);

    switch (pstProcessInfo->ulStatus)
    {
        case PROCESS_HB_INIT:
            DOS_ASSERT(0);
            /* 不需要break */
        case PROCESS_HB_WORKING:
            pstProcessInfo->ulStatus = PROCESS_HB_WORKING;
#if INCLUDE_BH_SERVER
            {
                dos_tmr_start(&pstProcessInfo->hTmrHBTimeout
                        , hb_get_max_timeout() * 1000
                        , hb_recv_timeout
                        , (U64)pstProcessInfo->ulProcessCBNo
                        , TIMER_NORMAL_NO_LOOP);
            }

            hb_send_heartbeat(pstProcessInfo);
#endif
#if INCLUDE_BH_CLIENT
            dos_tmr_stop(&pstProcessInfo->hTmrRecvTimeout);
#endif
            break;
        case PROCESS_HB_DEINIT:
#if INCLUDE_BH_SERVER
            hb_send_unreg_responce(pstProcessInfo);
#elif INCLUDE_BH_CLIENT
            hb_send_unreg(pstProcessInfo);
#endif
            break;
        default:
            DOS_ASSERT(0);
            return -1;
    }

    logr_debug("%s", "Heartbeat message processed.");

    return 0;
}
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
