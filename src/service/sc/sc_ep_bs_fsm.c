/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_ep_sb_fsm.c
 *
 *  创建时间: 2015年1月9日11:09:15
 *  作    者: Larry
 *  描    述: SC模块处理SB发送消息相关函数集合
 *  修改历史:
 */


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <bs_pub.h>
#include <sys/select.h>

#include "sc_def.h"
#include "sc_bs_def.h"
#include "sc_debug.h"

extern HASH_TABLE_S     *g_pstMsgList; /* refer to SC_BS_MSG_NODE */

pthread_t       pthSCBSMsgProc;

SC_BS_CLIENT_ST *g_pstSCBSClient[SC_MAX_BS_CLIENT] = { NULL };

U32 sc_bs_auth_rsp_proc(BS_MSG_TAG *pstMsg)
{
    SC_SCB_ST   *pstSCB = NULL;
    BS_MSG_AUTH *pstAuthMsg = NULL;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "%s", "Invalid Msg.");
        return DOS_FAIL;
    }

    if (pstMsg->usMsgLen != sizeof(BS_MSG_AUTH ))
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "Invalid Msg length. Length: %d", pstMsg->usMsgLen);
        return DOS_FAIL;
    }

    pstAuthMsg = (BS_MSG_AUTH *)pstMsg;
    if (DOS_ADDR_INVALID(pstAuthMsg))
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "%s", "Invalid Msg body. ");
        return DOS_FAIL;
    }

    if (pstMsg->ulCRNo >= SC_MAX_SCB_NUM)
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "Invalid RC NO. Max: %d, Current: %s", SC_MAX_SCB_NUM, pstMsg->ulCRNo);
        return DOS_FAIL;
    }

    pstSCB = sc_scb_get(pstMsg->ulCRNo);
    if (!SC_SCB_IS_VALID(pstSCB))
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "%s", "Invalid SCB.");
        return DOS_FAIL;
    }

    sc_logr_debug(SC_BS, "%s", "Start process auth response.");

    pthread_mutex_lock(&pstSCB->mutexSCBLock);
    if (pstMsg->ucErrcode != BS_ERR_SUCC)
    {
        pstSCB->ucTerminationFlag = DOS_TRUE;
    }
    else
    {
        pstSCB->ucTerminationFlag = DOS_FALSE;
    }

    if (pstAuthMsg->ucBalanceWarning)
    {
        pstSCB->bBanlanceWarning = DOS_TRUE;
    }
    else
    {
        pstSCB->bBanlanceWarning = DOS_FALSE;
    }

    pstSCB->ucTerminationCause = pstMsg->ucErrcode;
    pthread_mutex_unlock(&pstSCB->mutexSCBLock);

    sc_logr_debug(SC_BS, "%s", "Process auth response finished.");

    sc_bs_msg_free(pstMsg->ulMsgSeq);

    return DOS_SUCC;
}

U32 sc_bs_billing_start_rsp_proc(BS_MSG_TAG *pstMsg)
{
    SC_SCB_ST   *pstSCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "%s", "Invalid Msg.");
        return DOS_FAIL;
    }

    if (pstMsg->ulCRNo >= SC_MAX_SCB_NUM)
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "Invalid RC NO. Max: %d, Current: %s", SC_MAX_SCB_NUM, pstMsg->ulCRNo);
        return DOS_FAIL;
    }

    pstSCB = sc_scb_get(pstMsg->ulCRNo);
    if (!SC_SCB_IS_VALID(pstSCB))
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "%s", "Invalid SCB.");
        return DOS_FAIL;
    }

    sc_logr_debug(SC_BS, "%s", "Start process billing start response msg.");

    pthread_mutex_lock(&pstSCB->mutexSCBLock);
    if (pstMsg->ucErrcode != BS_ERR_SUCC)
    {
        pstSCB->ucTerminationFlag = DOS_TRUE;
    }
    else
    {
        pstSCB->ucTerminationFlag = DOS_FALSE;
    }

    pstSCB->ucTerminationCause = pstMsg->ucErrcode;
    pthread_mutex_unlock(&pstSCB->mutexSCBLock);

    sc_logr_debug(SC_BS, "%s", "Process billing start response msg finished.");

    /* 通知删除消息 */
    sc_bs_msg_free(pstMsg->ulMsgSeq);

    return DOS_SUCC;
}

U32 sc_bs_billing_update_rsp_proc(BS_MSG_TAG *pstMsg)
{
    SC_SCB_ST   *pstSCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "%s", "Invalid Msg.");
        return DOS_FAIL;
    }

    if (pstMsg->ulCRNo >= SC_MAX_SCB_NUM)
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "%s", "Invalid RC NO. Max: %d, Current: %s", SC_MAX_SCB_NUM, pstMsg->ulCRNo);
        return DOS_FAIL;
    }

    pstSCB = sc_scb_get(pstMsg->ulCRNo);
    if (!SC_SCB_IS_VALID(pstSCB))
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "%s", "Invalid SCB.");
        return DOS_FAIL;
    }

    sc_logr_debug(SC_BS, "%s", "Start process billing update response msg.");

    pthread_mutex_lock(&pstSCB->mutexSCBLock);
    if (pstMsg->ucErrcode != BS_ERR_SUCC)
    {
        pstSCB->ucTerminationFlag = DOS_TRUE;
    }
    else
    {
        pstSCB->ucTerminationFlag = DOS_FALSE;
    }

    pstSCB->ucTerminationCause = pstMsg->ucErrcode;
    pthread_mutex_unlock(&pstSCB->mutexSCBLock);

    sc_logr_debug(SC_BS, "%s", "Process billing update response msg finished.");

    /* 通知删除消息 */
    sc_bs_msg_free(pstMsg->ulMsgSeq);

    return DOS_SUCC;

}

U32 sc_bs_billing_stop_rsp_proc(BS_MSG_TAG *pstMsg)
{
    SC_SCB_ST   *pstSCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "%s", "Invalid Msg.");
        return DOS_FAIL;
    }

    if (pstMsg->ulCRNo >= SC_MAX_SCB_NUM)
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "%s", "Invalid RC NO. Max: %d, Current: %s", SC_MAX_SCB_NUM, pstMsg->ulCRNo);
        return DOS_FAIL;
    }

    pstSCB = sc_scb_get(pstMsg->ulCRNo);
    if (!SC_SCB_IS_VALID(pstSCB))
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_BS, "%s", "Invalid SCB.");
        return DOS_FAIL;
    }

    sc_logr_debug(SC_BS, "%s", "Start process billing stop response msg.");

    pthread_mutex_lock(&pstSCB->mutexSCBLock);
    if (pstMsg->ucErrcode != BS_ERR_SUCC)
    {
        pstSCB->ucTerminationFlag = DOS_TRUE;
    }
    else
    {
        pstSCB->ucTerminationFlag = DOS_FALSE;
    }

    pstSCB->ucTerminationCause = pstMsg->ucErrcode;
    pthread_mutex_unlock(&pstSCB->mutexSCBLock);

    sc_logr_debug(SC_BS, "%s", "Process billing start response msg finished.");

    /* 通知删除消息 */
    sc_bs_msg_free(pstMsg->ulMsgSeq);

    return DOS_SUCC;
}

U32 sc_bs_release_ind_rsp_proc(BS_MSG_TAG *pstMsg)
{
    return sc_send_release_ind2bs(pstMsg);
}

VOID sc_bs_hb_timeout(U64 uLIndex)
{
    if (uLIndex >= SC_MAX_BS_CLIENT)
    {
        return;
    }

    g_pstSCBSClient[uLIndex]->ulHBFailCnt++;
    if (g_pstSCBSClient[uLIndex]->ulHBFailCnt > SC_BS_HB_FAIL_CNT)
    {
        g_pstSCBSClient[uLIndex]->ulStatus = SC_BS_STATUS_LOST;

        sc_logr_notice(SC_BS, "BS Server Lost. Index:%u", uLIndex);
        close(g_pstSCBSClient[uLIndex]->lSocket);
        g_pstSCBSClient[uLIndex]->lSocket = -1;
        return;
    }
}

VOID sc_bs_hb_interval_timeout(U64 uLIndex)
{
    if (uLIndex >= SC_MAX_BS_CLIENT)
    {
        return;
    }

    sc_logr_info(SC_BS, "Heartbeat between BS and SC interval timeout. Client Index : %u", uLIndex);

    sc_send_hello2bs((U32)uLIndex);
    dos_tmr_start(&g_pstSCBSClient[uLIndex]->hTmrHBTimeout, SC_BS_HB_TIMEOUT, sc_bs_hb_timeout, uLIndex, TIMER_NORMAL_NO_LOOP);
}


VOID sc_bs_msg_proc(U8 *pData, U32 ulLength, U32 ulClientIndex)
{
    BS_MSG_TAG *pstMsgHeader;

    if (DOS_ADDR_INVALID(pData) || ulLength < sizeof(BS_MSG_TAG))
    {
        DOS_ASSERT(0);

        sc_logr_info(SC_BS, "Recv Invalid msg. Length:%s", ulLength);
        return;
    }

    if (ulClientIndex >= SC_MAX_BS_CLIENT)
    {
        DOS_ASSERT(0);

        sc_logr_info(SC_BS, "Invalid clinet index. Index:%X", ulClientIndex);
        return;
    }

    pstMsgHeader = (BS_MSG_TAG *)pData;
    if (pstMsgHeader->usMsgLen != ulLength)
    {
        DOS_ASSERT(0);

        sc_logr_info(SC_BS, "Recv Invalid msg. Length:%s, Length in Msg", ulLength, pstMsgHeader->usMsgLen);
        return;
    }

    sc_logr_info(SC_BS, "Recv BS Msg. Version: %X, Port:%d, IP:%X, Seq:%d, CR No:%d, Msg Type:%d, ErrCode: %d, Msg Length:%d"
                    , pstMsgHeader->usVersion
                    , pstMsgHeader->usPort
                    , pstMsgHeader->aulIPAddr[0]
                    , pstMsgHeader->ulMsgSeq
                    , pstMsgHeader->ulCRNo
                    , pstMsgHeader->ucMsgType
                    , pstMsgHeader->ucErrcode
                    , pstMsgHeader->usMsgLen);

    if (BS_MSG_INTERFACE_VERSION != pstMsgHeader->usVersion)
    {
        DOS_ASSERT(0);

        sc_logr_info(SC_BS, "Recv Invalid msg. Version not match. Version:%X, Version in Msg", BS_MSG_INTERFACE_VERSION, pstMsgHeader->usVersion);
        return;
    }

    switch (pstMsgHeader->ucMsgType)
    {

        case BS_MSG_HELLO_RSP:
            /*
             * 1.停定时器
             * 2.维护状态
             **/
            dos_tmr_stop(&g_pstSCBSClient[ulClientIndex]->hTmrHBTimeout);
            g_pstSCBSClient[ulClientIndex]->ulHBFailCnt = 0;
            break;
        case BS_MSG_BALANCE_QUERY_RSP:
        case BS_MSG_USER_AUTH_RSP:
        case BS_MSG_ACCOUNT_AUTH_RSP:
            sc_bs_auth_rsp_proc(pstMsgHeader);
            break;
        case BS_MSG_BILLING_START_RSP:
            sc_bs_billing_start_rsp_proc(pstMsgHeader);
            break;
        case BS_MSG_BILLING_UPDATE_RSP:
            sc_bs_billing_update_rsp_proc(pstMsgHeader);
            break;
        case BS_MSG_BILLING_STOP_RSP:
            sc_bs_billing_stop_rsp_proc(pstMsgHeader);
            break;
        case BS_MSG_BILLING_RELEASE_IND:
            sc_bs_release_ind_rsp_proc(pstMsgHeader);
            break;
        case BS_MSG_HELLO_REQ:
        case BS_MSG_START_UP_NOTIFY:
        case BS_MSG_BALANCE_QUERY_REQ:
        case BS_MSG_USER_AUTH_REQ:
        case BS_MSG_ACCOUNT_AUTH_REQ:
        case BS_MSG_BILLING_START_REQ:
        case BS_MSG_BILLING_UPDATE_REQ:
        case BS_MSG_BILLING_STOP_REQ:
        case BS_MSG_BILLING_RELEASE_ACK:
        default:
            DOS_ASSERT(0);
            sc_logr_info(SC_BS, "Recv Invalid msg. Templetely not support. %d", pstMsgHeader->ucMsgType);
            break;
    }
}


U32 sc_bs_connect(U32 ulIndex)
{
    struct sockaddr_in  stAddr;
    U32    ulAddr;
    S32    lRet;

    if (ulIndex >= SC_MAX_BS_CLIENT)
    {
        return DOS_FAIL;
    }

    if (!g_pstSCBSClient[ulIndex]->blValid)
    {
        return DOS_FAIL;
    }

    if (g_pstSCBSClient[ulIndex]->lSocket <= 0)
    {
        g_pstSCBSClient[ulIndex]->lSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (g_pstSCBSClient[ulIndex]->lSocket < 0)
        {
            return DOS_FAIL;
        }
    }

    dos_strtoipaddr("127.0.0.1", &ulAddr);
    dos_memzero(&stAddr, sizeof(stAddr));
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = dos_htons(52160);
    stAddr.sin_addr.s_addr = dos_htonl(ulAddr);
    lRet = bind(g_pstSCBSClient[ulIndex]->lSocket, (struct sockaddr *)&stAddr, sizeof(struct sockaddr_in));
    if (lRet < 0)
    {
        sc_logr_debug(SC_BS, "%s", "Bind socket fail.");
        close(g_pstSCBSClient[ulIndex]->lSocket);
        return DOS_FAIL;;
    }


    return DOS_SUCC;
#if 0
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = g_pstSCBSClient[ulIndex]->usPort;
    stAddr.sin_addr.s_addr = dos_htonl(g_pstSCBSClient[ulIndex]->aulIPAddr[0]);

    if (connect(g_pstSCBSClient[ulIndex]->lSocket, (struct sockaddr *)&stAddr, sizeof(struct sockaddr_in)) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
#endif
}

VOID *sc_bs_fsm_mainloop(VOID *ptr)
{
    fd_set              stFDSet;
    S32                 lRet;
    U32                 ulIndex;
    U32                 ulAddrLen;
    U32                 ulActiveClientCnt;
    U32                 ulMaxSockFD;
    U8                  aucBuffer[SC_BS_MEG_BUFF_LEN];
    struct sockaddr_in  stAddr;
    struct timeval stTimeout={1, 0};

    while (1)
    {

        for (ulActiveClientCnt=0, ulIndex=0; ulIndex<SC_MAX_BS_CLIENT; ulIndex++)
        {
            if (g_pstSCBSClient[ulIndex]->blValid)
            {
                if (g_pstSCBSClient[ulIndex]->ulStatus != SC_BS_STATUS_CONNECT
                    || g_pstSCBSClient[ulIndex]->lSocket <= 0)
                {
                    if (sc_bs_connect(ulIndex) != DOS_SUCC)
                    {
                        continue;
                    }

                    /* 连接成功之后开启心跳发送定时器 */
                    dos_tmr_start(&g_pstSCBSClient[ulIndex]->hTmrHBInterval, SC_BS_HB_INTERVAL, sc_bs_hb_interval_timeout, ulIndex, TIMER_NORMAL_LOOP);
                    g_pstSCBSClient[ulIndex]->ulStatus = SC_BS_STATUS_CONNECT;
                }
                else
                {
                    ulActiveClientCnt++;
                }
            }
        }

        if (ulActiveClientCnt <= 0)
        {
            dos_task_delay(1*1000);
            continue;
        }

        FD_ZERO(&stFDSet);
        for (ulMaxSockFD=0, ulIndex=0; ulIndex<SC_MAX_BS_CLIENT; ulIndex++)
        {
            if (g_pstSCBSClient[ulIndex]->blValid
                && SC_BS_STATUS_CONNECT == g_pstSCBSClient[ulIndex]->ulStatus
                && g_pstSCBSClient[ulIndex]->lSocket > 0)
            {
                FD_SET(g_pstSCBSClient[ulIndex]->lSocket, &stFDSet);
                if (g_pstSCBSClient[ulIndex]->lSocket > ulMaxSockFD)
                {
                    ulMaxSockFD = g_pstSCBSClient[ulIndex]->lSocket;
                }
            }
        }

        if (ulMaxSockFD <= 0)
        {
            continue;
        }
        stTimeout.tv_sec = 1;
        stTimeout.tv_usec = 0;

        lRet = select(ulMaxSockFD + 1, &stFDSet, NULL, NULL, &stTimeout);
        if (0 == lRet || EINTR == errno)
        {
            continue;
        }
        else if (lRet < 0)
        {
            DOS_ASSERT(0);
            sc_logr_notice(SC_BS, "Select fail. Errno:%d", errno);
            break;
        }

        for (ulActiveClientCnt=0, ulIndex=0; ulIndex<SC_MAX_BS_CLIENT; ulIndex++)
        {
            if (!g_pstSCBSClient[ulIndex]->blValid
                || SC_BS_STATUS_CONNECT != g_pstSCBSClient[ulIndex]->ulStatus
                || g_pstSCBSClient[ulIndex]->lSocket <= 0)
            {
                continue;
            }

            if (FD_ISSET(g_pstSCBSClient[ulIndex]->lSocket, &stFDSet))
            {
                dos_memzero(&stAddr, sizeof(stAddr));
                stAddr.sin_family = AF_INET;
                stAddr.sin_port = htons(g_pstSCBSClient[ulIndex]->usPort);
                stAddr.sin_addr.s_addr = htonl(g_pstSCBSClient[ulIndex]->aulIPAddr[0]);

                lRet = recvfrom(g_pstSCBSClient[ulIndex]->lSocket, aucBuffer, SC_BS_MEG_BUFF_LEN, 0, (struct sockaddr *)&stAddr, &ulAddrLen);
                if (0 == lRet || EINTR == errno || EAGAIN == errno)
                {
                    continue;
                }
                if (lRet < 0)
                {
                    DOS_ASSERT(0);
                    g_pstSCBSClient[ulIndex]->ulStatus = SC_BS_STATUS_LOST;

                    sc_logr_notice(SC_BS, "%s", "Recv data from socket fail.");
                    continue;
                }

                if (lRet < sizeof(BS_MSG_TAG))
                {
                    DOS_ASSERT(0);

                    sc_logr_notice(SC_BS, "%s", "Recv invalid data from socket fail. Length not complete");
                    continue;
                }

                sc_bs_msg_proc(aucBuffer, (U32)lRet, ulIndex);
            }
        }

    }

    return NULL;
}

U32 sc_bs_fsm_start()
{
    if (pthread_create(&pthSCBSMsgProc, NULL, sc_bs_fsm_mainloop, NULL) < 0)
    {
        DOS_ASSERT(0);

        sc_logr_notice(SC_BS, "%s", "Create BS MSG process pthread fail");
        DOS_ASSERT(0);
    }

    return DOS_SUCC;
}

U32 sc_bs_fsm_init()
{
    U8 *pszMem;
    U32 ulIndex;

    if (SC_MAX_BS_CLIENT <= 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    g_pstMsgList = hash_create_table(SC_BS_MSG_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstMsgList))
    {
        DOS_ASSERT(0);

        sc_logr_notice(SC_BS, "%s", "Create the hash table fail for the BS MSG.");
        return DOS_FAIL;
    }

    pszMem = (U8 *)dos_dmem_alloc(sizeof(SC_BS_CLIENT_ST) * SC_MAX_BS_CLIENT);
    if (DOS_ADDR_INVALID(pszMem))
    {
        DOS_ASSERT(0);

        sc_logr_notice(SC_BS, "%s", "Alloc memory fail for the BS Client CB.");
        return DOS_FAIL;
    }
    dos_memzero(pszMem, sizeof(SC_BS_CLIENT_ST) * SC_MAX_BS_CLIENT);

    for (ulIndex=0; ulIndex<SC_MAX_BS_CLIENT; ulIndex++)
    {
        g_pstSCBSClient[ulIndex] = (SC_BS_CLIENT_ST *)(pszMem + sizeof(SC_BS_CLIENT_ST) * ulIndex);
        g_pstSCBSClient[ulIndex]->ulIndex = ulIndex;
        g_pstSCBSClient[ulIndex]->lSocket = -1;
        g_pstSCBSClient[ulIndex]->ulStatus = SC_BS_STATUS_INVALID;
    }

    g_pstSCBSClient[0]->blValid = DOS_TRUE;
    g_pstSCBSClient[0]->usPort = BS_UDP_LINSTEN_PORT;
    g_pstSCBSClient[0]->lSocket = -1;
    dos_strtoipaddr("127.0.0.1", &g_pstSCBSClient[0]->aulIPAddr[0]);
    /*
    g_pstSCBSClient[0]->ulStatus = SC_BS_STATUS_CONNECT;
    g_pstSCBSClient[0]->lSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_pstSCBSClient[0]->lSocket < 0)
    {
        DOS_ASSERT(0);

        g_pstSCBSClient[0]->ulStatus = SC_BS_STATUS_REALSE;
        return DOS_FAIL;
    }
    */

    return DOS_SUCC;
}

U32 sc_bs_fsm_stop()
{
    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


