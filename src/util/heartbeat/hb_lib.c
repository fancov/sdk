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
#include "../../service/monitor/mon_notification.h"

#if INCLUDE_BH_ENABLE

static U32 g_ulHBCurrentLogLevel = LOG_LEVEL_NOTIC;
DLL_S  *g_pstNotifyList = NULL;
static pthread_mutex_t  g_mutexMsgSend = PTHREAD_MUTEX_INITIALIZER;


/* 唯一标识告警消息的序列号 */
static U32 g_ulSerialNo = 0;
extern sem_t g_stSem;
extern U32 mon_notify_customer(MON_NOTIFY_MSG_ST *pstMsg);

VOID hb_log_set_level(U32 ulLevel)
{
    g_ulHBCurrentLogLevel = ulLevel;
}

U32 hb_log_current_level()
{
    return g_ulHBCurrentLogLevel;
}


VOID hb_logr_write(U32 ulLevel, S8 *pszFormat, ...)
{
    va_list argptr;
    char szBuf[1024];

    va_start(argptr, pszFormat);
    vsnprintf(szBuf, sizeof(szBuf), pszFormat, argptr);
    va_end(argptr);
    szBuf[sizeof(szBuf) -1] = '\0';

    if (ulLevel <= g_ulHBCurrentLogLevel)
    {
        dos_log(ulLevel, LOG_TYPE_RUNINFO, szBuf);
    }
}


#endif


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
        hb_logr_warning("%s", "Invalid socket while send heartbeat to process.");
        DOS_ASSERT(0);
        return -1;
    }

    lRet = sendto(lSocket, pszBuff, ulBuffLen
            , 0, (struct sockaddr *)pstAddr
            , ulAddrLen);
    if (ulBuffLen != lRet)
    {
        hb_logr_warning("Exception occurred while send hreatbeat msg.(%d)", errno);
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

    hb_logr_debug("Start the recv timerout timer for the process \"%s\". %p"
                , pstProcessInfo->szProcessName, pstProcessInfo->hTmrRecvTimeout);
#else
    hb_logr_debug("Response heartbeat for hb client \"%s\".", pstProcessInfo->szProcessName);
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
    hb_logr_debug("%s", "Heartbeat client start register timer.");


    memset((VOID*)&stData, 0, sizeof(stData));
    stData.ulCommand = HEARTBEAT_DATA_REG;
    strncpy(stData.szProcessName, dos_get_process_name(), sizeof(stData.szProcessName));
    stData.szProcessName[sizeof(stData.szProcessName) - 1] = '\0';
    strncpy(stData.szProcessVersion, dos_get_process_version(), sizeof(stData.szProcessVersion));
    stData.szProcessVersion[sizeof(stData.szProcessVersion) - 1] = '\0';

    hb_logr_debug("%s", "Heartbeat client send register to hb server.");

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

S32 hb_send_external_warning(PROCESS_INFO_ST *pstProcessInfo, MON_NOTIFY_MSG_ST *pstMsg)
{
    HEARTBEAT_DATA_ST stData;
    U8   szData[1024] = {0};

    if (!pstProcessInfo || !pstMsg)
    {
        DOS_ASSERT(0);
        hb_logr_error("Invalid param. pstProcessInfo:%p; pstMsg:%p.", pstProcessInfo, pstMsg);
        return -1;
    }

    dos_memzero(&stData, sizeof(HEARTBEAT_DATA_ST));
    stData.ulCommand = HEARTBEAT_WARNING_SEND;
    stData.ulLength  = sizeof(MON_NOTIFY_MSG_ST);
    dos_snprintf(stData.szProcessName, sizeof(stData.szProcessName)
                    , "%s", pstProcessInfo->szProcessName);
    dos_snprintf(stData.szProcessVersion, sizeof(stData.szProcessVersion)
                    , "%s", pstProcessInfo->szProcessVersion);

    dos_memcpy(szData, stData, sizeof(stData));
    dos_memcpy(szData + sizeof(szData), pstMsg ,sizeof(MON_NOTIFY_MSG_ST));
    szData[sizeof(szData) - 1] = '\0';

    return hb_send_msg(szData, sizeof(HEARTBEAT_DATA_ST) + sizeof(MON_NOTIFY_MSG_ST)
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
S32 hb_reg_response_proc(PROCESS_INFO_ST *pstProcessInfo)
{
    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return -1;
    }

    hb_logr_debug("Process registe response.%p", pstProcessInfo->hTmrRegInterval);

    switch (pstProcessInfo->ulStatus)
    {
        case PROCESS_HB_INIT:

            /* 不需要break */
        case PROCESS_HB_WORKING:
            pstProcessInfo->ulStatus = PROCESS_HB_WORKING;

            if (pstProcessInfo->hTmrRegInterval)
            {
                hb_logr_debug("%s", "Stop registe timer.");
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
S32 hb_unreg_response_proc(PROCESS_INFO_ST *pstProcessInfo)
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
S32 hb_send_reg_response(PROCESS_INFO_ST *pstProcessInfo)
{
    HEARTBEAT_DATA_ST stData;

    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return -1;
    }

    hb_logr_debug("Send reg responce to the process \"%s\"", pstProcessInfo->szProcessName);

    memset((VOID*)&stData, 0, sizeof(stData));
    stData.ulCommand = HEARTBEAT_DATA_REG_RESPONSE;
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
S32 hb_send_unreg_response(PROCESS_INFO_ST *pstProcessInfo)
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
            hb_send_reg_response(pstProcessInfo);
            break;
        case PROCESS_HB_DEINIT:
            pstProcessInfo->ulStatus = PROCESS_HB_INIT;
            hb_send_reg_response(pstProcessInfo);
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
    hb_logr_debug("Start the heartbeat timeout timer for process \"%s\"", pstProcessInfo->szProcessName);

    return 0;
}


/**
 *  函数：S32 hb_recv_external_warning(VOID *pMsg)
 *  功能：接收消息
 *  参数：
 *      VOID *pMsg  消息
 *  返回值：成功返回0.失败返回－1
 */
S32 hb_recv_external_warning(VOID *pMsg)
{
    HEARTBEAT_DATA_ST stData;
    MON_NOTIFY_MSG_ST *pstMsg = NULL;
    DLL_NODE_S *pstNode = NULL;

    if (DOS_ADDR_INVALID(pMsg))
    {
        DOS_ASSERT(0);
        hb_logr_error("Invalid Msg. pMsg:%p", pMsg);
        return DOS_FAIL;
    }

    dos_memcpy(&stData, pMsg, sizeof(HEARTBEAT_DATA_ST));

    pstMsg = (MON_NOTIFY_MSG_ST *)dos_dmem_alloc(sizeof(MON_NOTIFY_MSG_ST));
    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    dos_memzero(pstMsg, sizeof(MON_NOTIFY_MSG_ST));

    pstNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    DLL_Init_Node(pstNode);
    dos_memcpy(pstMsg, pMsg + sizeof(HEARTBEAT_DATA_ST), sizeof(HEARTBEAT_DATA_ST));
    pstMsg->ulSeq = g_ulSerialNo;
    pstNode->pHandle = pstMsg;

    DLL_Add(g_pstNotifyList, pstNode);
    ++g_ulSerialNo;

    /* 每产生一个消息，则发送一个信号量 */
    sem_post(&g_stSem);

    return DOS_SUCC;
}


/**
 *  函数：VOID* hb_external_warning_proc(VOID* ptr)
 *  功能：处理其他模块发送过来的外部告警
 *  参数：
 *      VOID* ptr  线程参数
 *  返回值：
 */
VOID* hb_external_warning_proc(VOID* ptr)
{
    MON_NOTIFY_MSG_ST *pstMsg = NULL;
    DLL_NODE_S *pstNode = NULL;
    U32  ulRet;

    while (DLL_Count(g_pstNotifyList) > 0)
    {
        /* 等待信号量 */
        sem_wait(&g_stSem);

        pthread_mutex_lock(&g_mutexMsgSend);
        pstNode = dll_fetch(g_pstNotifyList);
        if (DOS_ADDR_INVALID(pstNode->pHandle))
        {
            continue;
        }

        pstMsg = (MON_NOTIFY_MSG_ST *)pstNode->pHandle;

        ulRet = mon_notify_customer(pstMsg);
        if (ulRet != DOS_SUCC)
        {
            hb_logr_error("Notify customer FAIL.");
            DOS_ASSERT(0);
            pthread_mutex_unlock(&g_mutexMsgSend);
        }
        pthread_mutex_unlock(&g_mutexMsgSend);
    }

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
            hb_send_unreg_response(pstProcessInfo);
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

    hb_logr_debug("Start process heartbeat msg. Status:%d.", pstProcessInfo->ulStatus);

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
            hb_send_unreg_response(pstProcessInfo);
#elif INCLUDE_BH_CLIENT
            hb_send_unreg(pstProcessInfo);
#endif
            break;
        default:
            DOS_ASSERT(0);
            return -1;
    }

    hb_logr_debug("%s", "Heartbeat message processed.");

    return 0;
}
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
