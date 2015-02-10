/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_ep_sb_adapter.c
 *
 *  创建时间: 2015年1月9日11:09:15
 *  作    者: Larry
 *  描    述: SC模块向SB发送消息相关函数集合
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <sys/select.h>


#include <dos.h>
#include <bs_pub.h>
#include <switch.h>

#include "sc_pub.h"
#include "bs_pub.h"
#include "sc_bs_pub.h"
#include "sc_task_pub.h"
#include "sc_event_process.h"
#include "sc_debug.h"

HASH_TABLE_S     *g_pstMsgList; /* refer to SC_BS_MSG_NODE */
pthread_mutex_t  g_mutexMsgList = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t   g_condMsgList  = PTHREAD_COND_INITIALIZER;
U32              g_ulMsgSeq     = 0;

extern SC_BS_CLIENT_ST *g_pstSCBSClient[SC_MAX_BS_CLIENT];

/* 消息hash表的hash函数 */
static U32 sc_bs_msg_hash_func(U32 ulSeq)
{
    return ulSeq % SC_BS_MSG_HASH_SIZE;
}

/* hash查找函数 */
static S32 sc_bs_msg_hash_find(VOID *pKey, HASH_NODE_S *pstNode)
{
    U32 ulIndex;
    SC_BS_MSG_NODE *pstMsgNode  = NULL;

    if (DOS_ADDR_INVALID(pKey) || DOS_ADDR_INVALID(pstNode))
    {
        return -1;
    }

    ulIndex = *(U32 *)pKey;
    pstMsgNode = (SC_BS_MSG_NODE *)pstNode;

    if (ulIndex == pstMsgNode->ulSeq)
    {
        return 0;
    }

    return -1;
}

/* 发送数据到BS */
static U32 sc_send_msg2bs(BS_MSG_TAG *pstMsgTag, U32 ulLength)
{
    U32 ulIndex;
    S32 lRet;
    struct sockaddr_in stAddr;

    for (ulIndex=0; ulIndex<SC_MAX_BS_CLIENT; ulIndex++)
    {
        if (g_pstSCBSClient[ulIndex]
            && g_pstSCBSClient[ulIndex]->blValid
            && SC_BS_STATUS_CONNECT == g_pstSCBSClient[ulIndex]->ulStatus)
        {
            dos_memzero(&stAddr, sizeof(stAddr));
            stAddr.sin_family = AF_INET;
            stAddr.sin_port = htons(g_pstSCBSClient[ulIndex]->usPort);
            stAddr.sin_addr.s_addr = htonl(g_pstSCBSClient[ulIndex]->aulIPAddr[0]);

            lRet = sendto(g_pstSCBSClient[ulIndex]->lSocket, (U8 *)pstMsgTag, ulLength, 0, (struct sockaddr*)&stAddr, sizeof(stAddr));
            if (lRet < 0)
            {
                continue;
            }
            else
            {
                return DOS_SUCC;
            }
        }
    }

    return DOS_FAIL;
}

/* 重发数据到BS */
static VOID sc_resend_msg2bs(U64 uLMsgNodeAddr)
{
    SC_BS_MSG_NODE *pstMsgNode  = NULL;
    U32            ulHashIndex  = 0;
    VOID           *ptr;

    ptr = (VOID *)uLMsgNodeAddr;
    pstMsgNode = (VOID *)ptr;
    if (DOS_ADDR_INVALID(pstMsgNode))
    {
        DOS_ASSERT(0);

        return;
    }

    if (DOS_ADDR_INVALID(pstMsgNode->pData))
    {
        DOS_ASSERT(0);

        return;
    }
    pthread_mutex_lock(&g_mutexMsgList);
    pstMsgNode->ulFailCnt++;
    if (pstMsgNode->ulFailCnt >= SC_BS_SEND_FAIL_CNT)
    {
        /* 出hash表，解锁, 停止定时器，释放资源，退出 */
        ulHashIndex = sc_bs_msg_hash_func(pstMsgNode->ulLength);
        hash_delete_node(g_pstMsgList, (HASH_NODE_S *)pstMsgNode, ulHashIndex);

        dos_tmr_stop(&pstMsgNode->hTmrSendInterval);

        if (pstMsgNode->pData)
        {
            dos_dmem_free(pstMsgNode->pData);
        }
        pstMsgNode->pData = NULL;

        if (pstMsgNode->blNeedSyn)
        {
            sem_post(&pstMsgNode->semSyn);
        }

        dos_dmem_free(pstMsgNode);
        pstMsgNode = NULL;

        pthread_mutex_unlock(&g_mutexMsgList);

printf("\r\nDelte msg ..................................\r\n");

        return;
    }

    sc_send_msg2bs((BS_MSG_TAG *)pstMsgNode->pData, pstMsgNode->ulLength);
    pthread_mutex_unlock(&g_mutexMsgList);

    return;
}

/* 通知释放消息 */
U32 sc_bs_msg_free(U32 ulSeq)
{
    SC_BS_MSG_NODE *pstMsgNode  = NULL;
    U32            ulHashIndex = 0;

    pthread_mutex_lock(&g_mutexMsgList);
    ulHashIndex = sc_bs_msg_hash_func(ulSeq);
    pstMsgNode = (SC_BS_MSG_NODE *)hash_find_node(g_pstMsgList, ulHashIndex, &ulSeq, sc_bs_msg_hash_find);
    if (DOS_ADDR_INVALID(pstMsgNode))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&g_mutexMsgList);
        sc_logr_notice("Cannot find the msg with the seq %d", ulSeq);
        return DOS_FAIL;
    }

    dos_tmr_stop(&pstMsgNode->hTmrSendInterval);

    dos_dmem_free(pstMsgNode->pData);
    pstMsgNode->pData = NULL;

    if (pstMsgNode->blNeedSyn)
    {
        sem_post(&pstMsgNode->semSyn);
    }

    dos_dmem_free(pstMsgNode);
    pstMsgNode = NULL;

    pthread_mutex_unlock(&g_mutexMsgList);

    return DOS_SUCC;
}

/* 发送认证消息到数据到BS */
U32 sc_send_acc_auth2bs(SC_CCB_ST *pstCCB)
{
    return DOS_FAIL;
}

/* 发送认证消息到数据到BS */
U32 sc_send_usr_auth2bs(SC_CCB_ST *pstCCB)
{
    BS_MSG_AUTH    *pstAuthMsg = NULL;
    SC_BS_MSG_NODE *pstListNode = NULL;
    U32            ulHashIndex = 0;

    if (!SC_CCB_IS_VALID(pstCCB))
    {
        DOS_ASSERT(0);

        sc_logr_warning("%s", "Invalid SCB while send the auth msg.");
        return DOS_FAIL;
    }

    pstAuthMsg = (BS_MSG_AUTH *)dos_dmem_alloc(sizeof(BS_MSG_AUTH));
    if (DOS_ADDR_INVALID(pstAuthMsg))
    {
        DOS_ASSERT(0);

        sc_logr_warning("%s", "Alloc memory fail.");
        return DOS_FAIL;
    }

    dos_memzero(pstAuthMsg, sizeof(BS_MSG_AUTH));
    pstAuthMsg->stMsgTag.usVersion = BS_MSG_INTERFACE_VERSION;
    pstAuthMsg->stMsgTag.ulMsgSeq  = g_ulMsgSeq++;
    pstAuthMsg->stMsgTag.ulCRNo    = U32_BUTT;
    pstAuthMsg->stMsgTag.ucMsgType = BS_MSG_USER_AUTH_REQ;
    pstAuthMsg->stMsgTag.ucErrcode = BS_ERR_SUCC;
    pstAuthMsg->stMsgTag.usMsgLen  = sizeof(BS_MSG_AUTH);
    pstAuthMsg->ulUserID           = pstCCB->ulCustomID;
    pstAuthMsg->ulAgentID          = pstCCB->ulAgentID;
    pstAuthMsg->ulCustomerID       = pstCCB->ulCustomID;
    pstAuthMsg->ulAccountID        = pstCCB->ulCustomID;
    pstAuthMsg->ulTaskID           = pstCCB->ulTaskID;
    pstAuthMsg->lBalance           = 0;
    pstAuthMsg->ulTimeStamp        = time(NULL);
    pstAuthMsg->ulSessionNum       = 0;
    pstAuthMsg->ulMaxSession       = 0;
    pstAuthMsg->ucBalanceWarning   = 0;

    dos_strncpy(pstAuthMsg->szSessionID, pstCCB->szUUID, sizeof(pstAuthMsg->szSessionID));
    pstAuthMsg->szSessionID[sizeof(pstAuthMsg->szSessionID) - 1] = '\0';

    dos_strncpy(pstAuthMsg->szCaller, pstCCB->szCallerNum, sizeof(pstAuthMsg->szCaller));
    pstAuthMsg->szCaller[sizeof(pstAuthMsg->szCaller) - 1] = '\0';

    dos_strncpy(pstAuthMsg->szCallee, pstCCB->szCalleeNum, sizeof(pstAuthMsg->szCallee));
    pstAuthMsg->szCallee[sizeof(pstAuthMsg->szCallee) - 1] = '\0';

    dos_strncpy(pstAuthMsg->szCID, pstCCB->szANINum, sizeof(pstAuthMsg->szCID));
    pstAuthMsg->szCID[sizeof(pstAuthMsg->szCID) - 1] = '\0';

    dos_strncpy(pstAuthMsg->szAgentNum, pstCCB->szSiteNum, sizeof(pstAuthMsg->szAgentNum));
    pstAuthMsg->szAgentNum[sizeof(pstAuthMsg->szAgentNum) - 1] = '\0';


    pstListNode = dos_dmem_alloc(sizeof(SC_BS_MSG_NODE));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        DOS_ASSERT(0);

        sc_logr_warning("%s", "Alloc memory for list node fail.");
        return DOS_FAIL;
    }
    pstListNode->pData = (VOID *)pstAuthMsg;
    pstListNode->ulFailCnt = 0;
    pstListNode->ulRCNo = pstCCB->usCCBNo;
    pstListNode->ulLength = sizeof(BS_MSG_AUTH);
    pstListNode->ulSeq = pstAuthMsg->stMsgTag.ulMsgSeq;
    pstListNode->blNeedSyn = DOS_FALSE;
    sem_init(&pstListNode->semSyn, 0, 0);
    dos_tmr_init(&pstListNode->hTmrSendInterval);
    HASH_INIT_NODE((HASH_NODE_S *)pstListNode);

    pthread_mutex_lock(&g_mutexMsgList);
    ulHashIndex = sc_bs_msg_hash_func(pstListNode->ulSeq);
    hash_add_node(g_pstMsgList, (HASH_NODE_S *)pstListNode, ulHashIndex, NULL);
    pthread_mutex_unlock(&g_mutexMsgList);

    /* 启动定时器 */
    if (dos_tmr_start(&pstListNode->hTmrSendInterval, SC_BS_SEND_INTERVAL, sc_resend_msg2bs, (U64)pstListNode, TIMER_NORMAL_LOOP) < 0)
    {
        DOS_ASSERT(0);

        pthread_mutex_lock(&g_mutexMsgList);
        hash_delete_node(g_pstMsgList, (HASH_NODE_S *)pstListNode, ulHashIndex);

        if (pstListNode->pData)
        {
            dos_dmem_free(pstListNode->pData);
        }
        pstAuthMsg = NULL;
        pstListNode->pData = NULL;

        /* 释放资源 */
        dos_dmem_free(pstListNode);
        pstListNode = NULL;

        pthread_mutex_unlock(&g_mutexMsgList);

        sc_logr_notice("%s", "Start the timer fail while send the auth msg.");
        return DOS_FAIL;
    }

    if (sc_send_msg2bs((BS_MSG_TAG *)pstAuthMsg, sizeof(BS_MSG_AUTH)) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        /* 停定时器 */
        dos_tmr_stop(&pstListNode->hTmrSendInterval);

        /* 删除缓存 */
        pthread_mutex_lock(&g_mutexMsgList);
        hash_delete_node(g_pstMsgList, (HASH_NODE_S *)pstListNode, ulHashIndex);

        if (pstListNode->pData)
        {
            dos_dmem_free(pstListNode->pData);
        }
        pstAuthMsg = NULL;
        pstListNode->pData = NULL;

        /* 释放资源 */
        dos_dmem_free(pstListNode);
        pstListNode = NULL;
        pthread_mutex_unlock(&g_mutexMsgList);

        sc_logr_notice("%s", "Send Auth msg fail.");
        return DOS_FAIL;
    }

    sc_logr_info("%s", "Sleeped for bs msg syn.");
    pstListNode->blNeedSyn = DOS_TRUE;
    sem_wait(&pstListNode->semSyn);
    sc_logr_info("%s", "weekup for bs msg syn.");

    return DOS_SUCC;
}

/* 发送心跳消息到数据到BS */
U32 sc_send_hello2bs(U32 ulClientIndex)
{
    BS_MSG_TAG stMsgHello;

    if (ulClientIndex >= SC_MAX_BS_CLIENT)
    {
        DOS_ASSERT(0);

        sc_logr_debug("Invalid client index. Max : %d, Current : %d", SC_MAX_BS_CLIENT, ulClientIndex);
        return DOS_FAIL;
    }

    dos_memzero(&stMsgHello, sizeof(stMsgHello));

    stMsgHello.usVersion = BS_MSG_INTERFACE_VERSION;
    stMsgHello.ulMsgSeq  = g_ulMsgSeq++;
    stMsgHello.ulCRNo    = U32_BUTT;
    stMsgHello.ucMsgType = BS_MSG_HELLO_REQ;
    stMsgHello.ucErrcode = BS_ERR_SUCC;
    stMsgHello.usMsgLen  = sizeof(BS_MSG_TAG);

    sc_send_msg2bs(&stMsgHello, sizeof(stMsgHello));

    return DOS_SUCC;
}


/* SC启动时发送启动指示消息 */
U32 sc_send_startup2bs()
{
    /* 暂时不实现 */
    return DOS_SUCC;
}

/* 发送余额查询消息 */
U32 sc_send_balance_query2bs(U32 ulUserID, U32 ulCustomID, U32 ulAccountID)
{
    BS_MSG_BALANCE_QUERY *pstQueryMsg = NULL;
    SC_BS_MSG_NODE       *pstListNode = NULL;
    U32                  ulHashIndex = 0;

    pstQueryMsg = dos_dmem_alloc(sizeof(BS_MSG_BALANCE_QUERY));
    if (DOS_ADDR_INVALID(pstQueryMsg))
    {
        DOS_ASSERT(0);

        sc_logr_warning("%s", "Alloc memory fail.");
        return DOS_FAIL;
    }

    dos_memzero(pstQueryMsg, sizeof(SC_CCB_ST));
    pstQueryMsg->stMsgTag.usVersion = BS_MSG_INTERFACE_VERSION;
    pstQueryMsg->stMsgTag.ulMsgSeq  = g_ulMsgSeq++;
    pstQueryMsg->stMsgTag.ulCRNo    = U32_BUTT;
    pstQueryMsg->stMsgTag.ucMsgType = BS_MSG_BALANCE_QUERY_REQ;
    pstQueryMsg->stMsgTag.ucErrcode = BS_ERR_SUCC;
    pstQueryMsg->stMsgTag.usMsgLen  = sizeof(BS_MSG_BALANCE_QUERY);
    pstQueryMsg->ulUserID           = ulUserID;
    pstQueryMsg->ulCustomerID       = ulCustomID;
    pstQueryMsg->ulAccountID        = ulAccountID;
    pstQueryMsg->lBalance           = 0;
    pstQueryMsg->ucBalanceWarning   = 0;

    pstListNode = dos_dmem_alloc(sizeof(SC_BS_MSG_NODE));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        DOS_ASSERT(0);

        sc_logr_warning("%s", "Alloc memory for list node fail.");
        return DOS_FAIL;
    }
    pstListNode->pData = (VOID *)pstQueryMsg;
    pstListNode->ulFailCnt = 0;
    pstListNode->ulRCNo = U32_BUTT;
    pstListNode->ulLength = sizeof(BS_MSG_BALANCE_QUERY);
    pstListNode->ulSeq = pstQueryMsg->stMsgTag.ulMsgSeq;
    sem_init(&pstListNode->semSyn, 0, 0);
    pstListNode->blNeedSyn = DOS_FALSE;
    dos_tmr_init(&pstListNode->hTmrSendInterval);
    HASH_INIT_NODE((HASH_NODE_S *)pstListNode);

    pthread_mutex_lock(&g_mutexMsgList);
    ulHashIndex = sc_bs_msg_hash_func(pstListNode->ulSeq);
    hash_add_node(g_pstMsgList, (HASH_NODE_S *)pstListNode, ulHashIndex, NULL);
    pthread_mutex_unlock(&g_mutexMsgList);

    /* 启动定时器 */
    if (dos_tmr_start(&pstListNode->hTmrSendInterval, SC_BS_SEND_INTERVAL, sc_resend_msg2bs, (U64)pstListNode, TIMER_NORMAL_LOOP) < 0)
    {
        DOS_ASSERT(0);

        pthread_mutex_lock(&g_mutexMsgList);
        hash_delete_node(g_pstMsgList, (HASH_NODE_S *)pstListNode, ulHashIndex);

        if (pstListNode->pData)
        {
            dos_dmem_free(pstListNode->pData);
        }
        pstQueryMsg = NULL;
        pstListNode->pData = NULL;

        /* 释放资源 */
        dos_dmem_free(pstListNode);
        pstListNode = NULL;

        pthread_mutex_unlock(&g_mutexMsgList);
        sc_logr_notice("%s", "Start the timer fail while send the auth msg.");
        return DOS_FAIL;
    }

    if (sc_send_msg2bs((BS_MSG_TAG *)pstQueryMsg, sizeof(BS_MSG_BALANCE_QUERY)) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        /* 停定时器 */
        dos_tmr_stop(&pstListNode->hTmrSendInterval);

        /* 删除缓存 */
        pthread_mutex_lock(&g_mutexMsgList);
        hash_delete_node(g_pstMsgList, (HASH_NODE_S *)pstListNode, ulHashIndex);

        if (pstListNode->pData)
        {
            dos_dmem_free(pstListNode->pData);
        }
        pstQueryMsg = NULL;
        pstListNode->pData = NULL;

        /* 释放资源 */
        dos_dmem_free(pstListNode);
        pstListNode = NULL;

        pthread_mutex_unlock(&g_mutexMsgList);

        sc_logr_notice("%s", "Send Auth msg fail.");
        return DOS_FAIL;
    }


    sc_logr_info("%s", "Sleeped for bs msg syn.");
    pstListNode->blNeedSyn = DOS_TRUE;
    sem_wait(&pstListNode->semSyn);
    sc_logr_info("%s", "weekup for bs msg syn.");

    return DOS_SUCC;

}

/* 发送终止计费消息 */
U32 sc_send_billing_stop2bs(SC_CCB_ST *pstCCB)
{
    BOOL blExternnalLeg = DOS_FALSE, blOutboundCall = DOS_FALSE, blVMCall = DOS_FALSE;
    U32  ulCurrentLeg = 0, ulHashIndex = 0;
    SC_CCB_ST             *pstCCB2 = NULL, *pstFirstCCB = NULL, *pstSecondCCB = NULL;
    BS_MSG_CDR            *pstCDRMsg = NULL;
    SC_BS_MSG_NODE        *pstListNode = NULL;
    switch_channel_t      *pstChannel = NULL;
    switch_core_session_t *pstSession = NULL;
    switch_channel_timetable_t *pstTimeTable = NULL;
    switch_codec_implementation_t stCodecImp;

    /* 当前呼叫没有关联CCB时，就直接吧当前业务控制块作为主LEG */
    if (U32_BUTT == pstCCB->ulOtherLegID)
    {
        pstFirstCCB = pstCCB;

        goto prepare_msg;
    }

    pstCCB2 = sc_ccb_get(pstCCB->ulOtherLegID);
    if (DOS_ADDR_INVALID(pstCCB2))
    {
        pstFirstCCB = pstCCB;

        goto prepare_msg;
    }

    /* 确定业务类型，决定LEG的顺序，业务控制模块的较为详细，BS模块较为粗略 */
    /* \Voice Mail 记录，需要将呼入作为主LEG，VM记录是辅助LEG */
    blVMCall = sc_call_check_service(pstCCB, SC_SERV_VOICE_MAIL_RECORD);
    if (blVMCall)
    {
        pstFirstCCB = pstCCB2;
        goto prepare_msg;
    }

    blVMCall = sc_call_check_service(pstCCB2, SC_SERV_VOICE_MAIL_RECORD);
    if (blVMCall)
    {
        pstFirstCCB = pstCCB;
        goto prepare_msg;
    }

    /* \Voice Mail 获取，需要将呼入作为辅助LEG，VM获取是主LEG */
    blVMCall = sc_call_check_service(pstCCB, SC_SERV_VOICE_MAIL_RECORD);
    if (blVMCall)
    {
        pstFirstCCB = pstCCB;
        goto prepare_msg;
    }

    blVMCall = sc_call_check_service(pstCCB2, SC_SERV_VOICE_MAIL_RECORD);
    if (blVMCall)
    {
        pstFirstCCB = pstCCB2;
        goto prepare_msg;
    }

    /* \PSTN呼入，呼出到PSTN等类型呼叫，需要使用和PSTN通讯的leg作为主leg */
    blExternnalLeg = sc_call_check_service(pstCCB, SC_SERV_EXTERNAL_CALL);
    if (DOS_TRUE == blExternnalLeg)
    {
        pstFirstCCB = pstCCB;
        goto prepare_msg;
    }

    blExternnalLeg = sc_call_check_service(pstCCB2, SC_SERV_EXTERNAL_CALL);
    if (blExternnalLeg)
    {
        pstFirstCCB = pstCCB2;
        goto prepare_msg;
    }

    /* \内部呼叫，需要使用被叫的leg作为主leg */
    blOutboundCall = sc_call_check_service(pstCCB, SC_SERV_OUTBOUND_CALL);
    if (blOutboundCall)
    {
        pstFirstCCB = pstCCB;
        goto prepare_msg;
    }

    blOutboundCall = sc_call_check_service(pstCCB2, SC_SERV_OUTBOUND_CALL);
    if (blOutboundCall)
    {
        pstFirstCCB = pstCCB2;
        goto prepare_msg;
    }

    /* @TODO: 其他情况 */

    /* 默认情况，将当前CCB作为主LEG */
    pstFirstCCB = pstCCB;

prepare_msg:
    /* 申请资源 */
    pstCDRMsg = dos_dmem_alloc(sizeof(BS_MSG_CDR));
    if (DOS_ADDR_INVALID(pstCDRMsg))
    {
        DOS_ASSERT(0);

        sc_logr_notice("%s", "Alloc memory for the CDR msg fail.");
        return DOS_FAIL;
    }

    if (pstFirstCCB != pstCCB)
    {
        pstSecondCCB = pstCCB;
    }

    ulCurrentLeg = 0;

    if (DOS_ADDR_VALID(pstFirstCCB))
    {
        /* 第一个leg */
        pstSession = switch_core_session_locate(pstFirstCCB->szUUID);
        if (!pstSession)
        {
            DOS_ASSERT(0);
        }
        else
        {
            pstChannel = switch_core_session_get_channel(pstSession);
            switch_core_session_get_read_impl(pstSession, &stCodecImp);
        }

        if (!pstChannel)
        {
            DOS_ASSERT(0);
        }

        if (pstChannel)
        {
            pstTimeTable = switch_channel_get_timetable(pstChannel);
        }

        if (!pstTimeTable)
        {
            DOS_ASSERT(0);
        }

        /* 填充数据 */
        pstCDRMsg->stMsgTag.usVersion = BS_MSG_INTERFACE_VERSION;
        pstCDRMsg->stMsgTag.ulMsgSeq  = g_ulMsgSeq++;
        pstCDRMsg->stMsgTag.ulCRNo    = pstFirstCCB->usCCBNo;
        pstCDRMsg->stMsgTag.ucMsgType = BS_MSG_BILLING_STOP_REQ;
        pstCDRMsg->stMsgTag.ucErrcode = BS_ERR_SUCC;
        pstCDRMsg->stMsgTag.usMsgLen  = sizeof(BS_MSG_BALANCE_QUERY);

        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulCDRMark = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulUserID = pstFirstCCB->ulCustomID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAgentID = pstFirstCCB->ulAgentID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulCustomerID = pstFirstCCB->ulCustomID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAccountID = pstFirstCCB->ulCustomID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulTaskID = pstFirstCCB->ulTaskID;
        //pstCDRMsg->astSessionLeg[ulCurrentLeg].szRecordFile;

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID, pstFirstCCB->szUUID, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller, pstFirstCCB->szCallerNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee, pstFirstCCB->szCalleeNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID, pstFirstCCB->szANINum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum, pstFirstCCB->szSiteNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum) - 1] = '\0';

        if (pstTimeTable)
        {
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulStartTimeStamp = pstTimeTable->created;
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulRingTimeStamp = pstTimeTable->progress;
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAnswerTimeStamp = pstTimeTable->progress_media;
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulIVRFinishTimeStamp= 0;
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulDTMFTimeStamp = pstTimeTable->created;
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulBridgeTimeStamp = pstTimeTable->bridged;
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulByeTimeStamp = pstTimeTable->hungup;
        }

        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulHoldCnt = pstFirstCCB->usHoldCnt;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulHoldTimeLen = pstFirstCCB->usHoldTotalTime;
        //pstCDRMsg->astSessionLeg[ulCurrentLeg].aulPeerIP[4];
        pstCDRMsg->astSessionLeg[ulCurrentLeg].usPeerTrunkID = pstFirstCCB->ulTrunkID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].usTerminateCause = pstFirstCCB->ucTerminationCause;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ucReleasePart = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ucPayloadType = stCodecImp.ianacode;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ucPacketLossRate = 0;
        dos_memcpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].aucServType, pstFirstCCB->aucServiceType, BS_MAX_SERVICE_TYPE_IN_SESSION);

        ulCurrentLeg++;
    }

    if (DOS_ADDR_VALID(pstSecondCCB))
    {
        /* 第二个leg */
        pstSession = switch_core_session_locate(pstSecondCCB->szUUID);
        if (!pstSession)
        {
            DOS_ASSERT(0);
        }
        else
        {
            pstChannel = switch_core_session_get_channel(pstSession);
            switch_core_session_get_read_impl(pstSession, &stCodecImp);
        }

        if (!pstChannel)
        {
            DOS_ASSERT(0);
        }


        if (pstChannel)
        {
            pstTimeTable = switch_channel_get_timetable(pstChannel);
        }

        if (!pstTimeTable)
        {
            DOS_ASSERT(0);
        }


        /* 填充数据 */
        pstCDRMsg->stMsgTag.usVersion = BS_MSG_INTERFACE_VERSION;
        pstCDRMsg->stMsgTag.ulMsgSeq  = g_ulMsgSeq++;
        pstCDRMsg->stMsgTag.ulCRNo    = pstSecondCCB->usCCBNo;
        pstCDRMsg->stMsgTag.ucMsgType = BS_MSG_BILLING_STOP_REQ;
        pstCDRMsg->stMsgTag.ucErrcode = BS_ERR_SUCC;
        pstCDRMsg->stMsgTag.usMsgLen  = sizeof(BS_MSG_BALANCE_QUERY);

        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulCDRMark = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulUserID = pstSecondCCB->ulCustomID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAgentID = pstSecondCCB->ulAgentID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulCustomerID = pstSecondCCB->ulCustomID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAccountID = pstSecondCCB->ulCustomID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulTaskID = pstSecondCCB->ulTaskID;
        //pstCDRMsg->astSessionLeg[ulCurrentLeg].szRecordFile;

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID, pstSecondCCB->szUUID, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller, pstSecondCCB->szCallerNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee, pstSecondCCB->szCalleeNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID, pstSecondCCB->szANINum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum, pstSecondCCB->szSiteNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum) - 1] = '\0';

        if (pstTimeTable)
        {
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulStartTimeStamp = pstTimeTable->created;
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulRingTimeStamp = pstTimeTable->progress;
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAnswerTimeStamp = pstTimeTable->progress_media;
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulIVRFinishTimeStamp = 0;
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulDTMFTimeStamp = pstTimeTable->created;
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulBridgeTimeStamp = pstTimeTable->bridged;
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulByeTimeStamp = pstTimeTable->hungup;
        }

        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulHoldCnt = pstSecondCCB->usHoldCnt;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulHoldTimeLen = pstSecondCCB->usHoldTotalTime;
        //pstCDRMsg->astSessionLeg[ulCurrentLeg].aulPeerIP[4];
        pstCDRMsg->astSessionLeg[ulCurrentLeg].usPeerTrunkID = pstSecondCCB->ulTrunkID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].usTerminateCause = pstSecondCCB->ucTerminationCause;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ucReleasePart = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ucPayloadType = stCodecImp.ianacode;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ucPacketLossRate = 0;
        dos_memcpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].aucServType, pstFirstCCB->aucServiceType, BS_MAX_SERVICE_TYPE_IN_SESSION);

        ulCurrentLeg++;
    }

    /* 如果没有leg被加入，就放弃 */
    if (ulCurrentLeg <= 0)
    {
        DOS_ASSERT(0);

        sc_logr_notice("%s", "There no leg created while send cdr msg.");
        return DOS_FAIL;
    }

    /* 记录当前有几个LEG */
    pstCDRMsg->ucLegNum = ulCurrentLeg;

    /* 将消息存放hash表的节点 */
    pstListNode = dos_dmem_alloc(sizeof(SC_BS_MSG_NODE));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        DOS_ASSERT(0);

        sc_logr_warning("%s", "Alloc memory for list node fail.");
        return DOS_FAIL;
    }
    pstListNode->pData = (VOID *)pstCDRMsg;
    pstListNode->ulFailCnt = 0;
    pstListNode->ulRCNo = U32_BUTT;
    pstListNode->ulLength = sizeof(BS_MSG_BALANCE_QUERY);
    pstListNode->ulSeq = pstCDRMsg->stMsgTag.ulMsgSeq;
    pstListNode->blNeedSyn = DOS_FALSE;
    sem_init(&pstListNode->semSyn, 0, 0);
    dos_tmr_init(&pstListNode->hTmrSendInterval);
    HASH_INIT_NODE((HASH_NODE_S *)pstListNode);

    /* 加入HASH表 */
    pthread_mutex_lock(&g_mutexMsgList);
    ulHashIndex = sc_bs_msg_hash_func(pstListNode->ulSeq);
    hash_add_node(g_pstMsgList, (HASH_NODE_S *)pstListNode, ulHashIndex, NULL);
    pthread_mutex_unlock(&g_mutexMsgList);

    /* 启动定时器 */
    if (dos_tmr_start(&pstListNode->hTmrSendInterval, SC_BS_SEND_INTERVAL, sc_resend_msg2bs, (U64)pstListNode, TIMER_NORMAL_LOOP) < 0)
    {
        DOS_ASSERT(0);

        pthread_mutex_lock(&g_mutexMsgList);
        hash_delete_node(g_pstMsgList, (HASH_NODE_S *)pstListNode, ulHashIndex);


        pstListNode->pData = NULL;
        dos_dmem_free(pstListNode);
        pstListNode = NULL;
        dos_dmem_free(pstCDRMsg);
        pstCDRMsg = NULL;

        pthread_mutex_unlock(&g_mutexMsgList);

        sc_logr_notice("%s", "Start the timer fail while send the auth msg.");
        return DOS_FAIL;
    }

    /* 发送数据 */
    if (sc_send_msg2bs((BS_MSG_TAG *)pstCDRMsg, sizeof(BS_MSG_BALANCE_QUERY)) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        /* 停定时器 */
        dos_tmr_stop(&pstListNode->hTmrSendInterval);

        /* 删除缓存 */
        pthread_mutex_lock(&g_mutexMsgList);
        hash_delete_node(g_pstMsgList, (HASH_NODE_S *)pstListNode, ulHashIndex);


        pstListNode->pData = NULL;
        dos_dmem_free(pstListNode);
        pstListNode = NULL;
        dos_dmem_free(pstCDRMsg);
        pstCDRMsg = NULL;


        pthread_mutex_unlock(&g_mutexMsgList);

        sc_logr_notice("%s", "Send Auth msg fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/* 发送初始计费消息 */
U32 sc_send_billing_start2bs(SC_CCB_ST *pstCCB)
{
    return sc_send_billing_stop2bs(pstCCB);
}

/* 发送中间计费消息 */
U32 sc_send_billing_update2bs(SC_CCB_ST *pstCCB)
{
    return sc_send_billing_stop2bs(pstCCB);
}


/* 发送业务释放消息 */
U32 sc_send_release_ind2bs(BS_MSG_TAG *pstMsg)
{
    return DOS_SUCC;
}

/* 发送业务释放确认消息 */
U32 sc_send_release_ack2bs(BS_MSG_TAG *pstMsg)
{
    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


