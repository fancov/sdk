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
#include <esl.h>
#include <bs_pub.h>

#include "sc_def.h"
#include "sc_acd.h"
#include "sc_res.h"
#include "sc_debug.h"
#include "sc_bs.h"

HASH_TABLE_S     *g_pstMsgList; /* refer to SC_BS_MSG_NODE */
pthread_mutex_t  g_mutexMsgList = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t   g_condMsgList  = PTHREAD_COND_INITIALIZER;
U32              g_ulMsgSeq     = 0;

extern SC_BS_CLIENT_ST *g_pstSCBSClient[SC_MAX_BS_CLIENT];
extern SC_BS_MSG_STAT_ST stBSMsgStat;

/* 发送数据到BS */
static U32 sc_send_msg2bs(BS_MSG_TAG *pstMsgTag, U32 ulLength)
{
    U32 ulIndex;
    S32 lRet;
    S32 lAddrLen;
    struct sockaddr_in stAddr;
    struct sockaddr_un stUnAddr;
    S8     szBuffSockPath[256] = { 0 };

    for (ulIndex=0; ulIndex<SC_MAX_BS_CLIENT; ulIndex++)
    {
        if (g_pstSCBSClient[ulIndex]
            && g_pstSCBSClient[ulIndex]->blValid
            && SC_BS_STATUS_CONNECT == g_pstSCBSClient[ulIndex]->ulStatus)
        {
            switch (g_pstSCBSClient[ulIndex]->usCommProto)
            {
                case BSCOMM_PROTO_UDP:
                    dos_memzero(&stAddr, sizeof(stAddr));
                    stAddr.sin_family = AF_INET;
                    stAddr.sin_port = dos_htons(g_pstSCBSClient[ulIndex]->usPort);
                    stAddr.sin_addr.s_addr = dos_htonl(g_pstSCBSClient[ulIndex]->aulIPAddr[0]);
                    pstMsgTag->usPort = 0;
                    pstMsgTag->aulIPAddr[0] = 0;
                    pstMsgTag->aulIPAddr[1] = 0;
                    pstMsgTag->aulIPAddr[2] = 0;
                    pstMsgTag->aulIPAddr[3] = 0;

                    lRet = sendto(g_pstSCBSClient[ulIndex]->lSocket, (U8 *)pstMsgTag, ulLength, 0, (struct sockaddr*)&stAddr, sizeof(stAddr));
                    if (lRet < 0)
                    {
                        continue;
                    }
                    else
                    {
                        return DOS_SUCC;
                    }
                    break;

                case BSCOMM_PROTO_TCP:
                    /* 暂时不实现 */
                    break;
                default:
                    pstMsgTag->usPort = 0;
                    pstMsgTag->aulIPAddr[0] = g_pstSCBSClient[ulIndex]->aulIPAddr[0];
                    pstMsgTag->aulIPAddr[1] = g_pstSCBSClient[ulIndex]->aulIPAddr[1];
                    pstMsgTag->aulIPAddr[2] = g_pstSCBSClient[ulIndex]->aulIPAddr[2];
                    pstMsgTag->aulIPAddr[3] = g_pstSCBSClient[ulIndex]->aulIPAddr[3];

                    dos_snprintf(szBuffSockPath, sizeof(szBuffSockPath), "%s/var/run/socket/bs.sock", dos_get_sys_root_path());
                    stUnAddr.sun_family = AF_UNIX;
                    dos_strcpy(stUnAddr.sun_path, szBuffSockPath);
                    lAddrLen = offsetof(struct sockaddr_un, sun_path) + strlen(szBuffSockPath);
                    lRet = sendto(g_pstSCBSClient[ulIndex]->lSocket, (U8 *)pstMsgTag, ulLength, 0, (struct sockaddr*)&stUnAddr, lAddrLen);
                    if (lRet < 0)
                    {
                        DOS_ASSERT(0);
                        continue;
                    }
                    else
                    {
                        return DOS_SUCC;
                    }
                    break;

            }
        }
    }

    return DOS_FAIL;
}

#if 0

#if SC_BS_NEED_RESEND
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

/* 重发数据到BS */
static VOID sc_resend_msg2bs(U64 uLMsgNodeAddr)
{
    SC_BS_MSG_NODE *pstMsgNode  = NULL;
    U32            ulHashIndex  = 0;
    SC_SCB_ST      *pstSCB = NULL;
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


        pstSCB = sc_scb_get(pstMsgNode->ulRCNo);
        if (DOS_ADDR_VALID(pstSCB))
        {
            //sc_ep_terminate_call(pstSCB);
        }

        dos_dmem_free(pstMsgNode);
        pstMsgNode = NULL;

        pthread_mutex_unlock(&g_mutexMsgList);

        sc_log(LOG_LEVEL_INFO, "%s", "Delete msg!");

        return;
    }

    sc_send_msg2bs((BS_MSG_TAG *)pstMsgNode->pData, pstMsgNode->ulLength);

    sc_log(LOG_LEVEL_INFO, "Re-send msg to BS. Fail Cnt: %u, RCNo: %u, Seq: %u"
                    , pstMsgNode->ulFailCnt, pstMsgNode->ulRCNo, pstMsgNode->ulSeq);

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
        pthread_mutex_unlock(&g_mutexMsgList);
        sc_log(LOG_LEVEL_NOTIC, "Cannot find the msg with the seq %d", ulSeq);
        return DOS_FAIL;
    }

    dos_tmr_stop(&pstMsgNode->hTmrSendInterval);

    dos_dmem_free(pstMsgNode->pData);
    pstMsgNode->pData = NULL;

#if 0
    if (pstMsgNode->blNeedSyn)
    {
        sem_post(&pstMsgNode->semSyn);
    }
#endif

    hash_delete_node(g_pstMsgList, (HASH_NODE_S *)pstMsgNode, ulHashIndex);

    dos_dmem_free(pstMsgNode);
    pstMsgNode = NULL;

    pthread_mutex_unlock(&g_mutexMsgList);

    return DOS_SUCC;
}

#endif

U32 sc_bs_srv_type_adapter(U8 *aucSCSrvList, U32 ulSCSrvCnt, U8 *aucBSSrvList, U32 ulBSSrvCnt)
{
    U32        ulIndex          = 0;
    U32        ulBSSrvIndex     = 0;
    BOOL       blIsOutboundCall = DOS_FALSE;
    BOOL       blIsExternalCall = DOS_FALSE;
    BOOL       blNeedOutbond    = DOS_TRUE;
    BOOL       bIsExit          = DOS_FALSE;

    if (DOS_ADDR_INVALID(aucSCSrvList)
        || ulSCSrvCnt <= 0
        || DOS_ADDR_INVALID(aucBSSrvList)
        || ulBSSrvCnt <= 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulBSSrvIndex = 0;
    memset(aucBSSrvList, U8_BUTT, ulBSSrvCnt);

    for (ulIndex=0; ulIndex<ulSCSrvCnt; ulIndex++)
    {
        switch (aucSCSrvList[ulIndex])
        {
            case SC_SERV_OUTBOUND_CALL:
            case SC_SERV_INBOUND_CALL:
            case SC_SERV_INTERNAL_CALL:
            case SC_SERV_EXTERNAL_CALL:
                break;

            case SC_SERV_AUTO_DIALING:
            case SC_SERV_DEMO_TASK:
                if (ulBSSrvIndex < ulBSSrvCnt)
                {
                    aucBSSrvList[ulBSSrvIndex] = BS_SERV_AUTO_DIALING;
                    ulBSSrvIndex++;
                }

                blNeedOutbond = DOS_FALSE;
                break;

            case SC_SERV_PREVIEW_DIALING:
                if (ulBSSrvIndex < ulBSSrvCnt)
                {
                    aucBSSrvList[ulBSSrvIndex] = BS_SERV_PREVIEW_DIALING;
                    ulBSSrvIndex++;
                }

                blNeedOutbond = DOS_FALSE;
                break;

            case SC_SERV_PREDICTIVE_DIALING:
                if (ulBSSrvIndex < ulBSSrvCnt)
                {
                    aucBSSrvList[ulBSSrvIndex] = BS_SERV_PREDICTIVE_DIALING;
                    ulBSSrvIndex++;
                }

                blNeedOutbond = DOS_FALSE;
                break;

            case SC_SERV_RECORDING:
                /* 这个是辅助业务，在后面处理 */
                break;

            case SC_SERV_FORWORD_CFB:
            case SC_SERV_FORWORD_CFU:
            case SC_SERV_FORWORD_CFNR:
                if (ulBSSrvIndex < ulBSSrvCnt)
                {
                    aucBSSrvList[ulBSSrvIndex] = BS_SERV_CALL_FORWARD;
                    ulBSSrvIndex++;
                }
                break;

            case SC_SERV_BLIND_TRANSFER:
            case SC_SERV_ATTEND_TRANSFER:
                if (ulBSSrvIndex < ulBSSrvCnt)
                {
                    aucBSSrvList[ulBSSrvIndex] = BS_SERV_CALL_TRANSFER;
                    ulBSSrvIndex++;
                }
                break;

            case SC_SERV_PICK_UP:
                if (ulBSSrvIndex < ulBSSrvCnt)
                {
                    aucBSSrvList[ulBSSrvIndex] = BS_SERV_PICK_UP;
                    ulBSSrvIndex++;
                }
                break;

            case SC_SERV_CONFERENCE:
                if (ulBSSrvIndex < ulBSSrvCnt)
                {
                    aucBSSrvList[ulBSSrvIndex] = BS_SERV_CONFERENCE;
                    ulBSSrvIndex++;
                }
                break;

            case SC_SERV_VOICE_MAIL_RECORD:
            case SC_SERV_VOICE_MAIL_GET:
                if (ulBSSrvIndex < ulBSSrvCnt)
                {
                    aucBSSrvList[ulBSSrvIndex] = BS_SERV_VOICE_MAIL;
                    ulBSSrvIndex++;
                }
                break;

            case SC_SERV_SMS_RECV:
            case SC_SERV_SMS_SEND:
            case SC_SERV_MMS_RECV:
            case SC_SERV_MMS_SNED:
                DOS_ASSERT(0);
                break;

            case SC_SERV_FAX:
            case SC_SERV_INTERNAL_SERVICE:
                DOS_ASSERT(0);
                break;

            case SC_SERV_NUM_VERIFY:
                if (ulBSSrvIndex < ulBSSrvCnt)
                {
                    aucBSSrvList[ulBSSrvIndex] = BS_SERV_VERIFY;
                    ulBSSrvIndex++;
                }
                bIsExit = DOS_TRUE;
                break;

            default:
                break;
        }
    }

    if (bIsExit)
    {
        return DOS_SUCC;
    }

    for (ulIndex=0; ulIndex<ulSCSrvCnt; ulIndex++)
    {
        if (SC_SERV_OUTBOUND_CALL == aucSCSrvList[ulIndex])
        {
            blIsOutboundCall = DOS_TRUE;
        }
        else if (SC_SERV_EXTERNAL_CALL == aucSCSrvList[ulIndex])
        {
            blIsExternalCall = DOS_TRUE;
        }
    }

    if (blIsOutboundCall && blIsExternalCall)
    {
        if (blNeedOutbond
            && ulBSSrvIndex < ulBSSrvCnt)
        {
            aucBSSrvList[ulBSSrvIndex] = BS_SERV_OUTBAND_CALL;
            ulBSSrvIndex++;
        }
    }
    else if (!blIsOutboundCall && blIsExternalCall)
    {
        if (ulBSSrvIndex < ulBSSrvCnt)
        {
            aucBSSrvList[ulBSSrvIndex] = BS_SERV_INBAND_CALL;
            ulBSSrvIndex++;
        }
    }
    else
    {
        if (ulBSSrvIndex < ulBSSrvCnt)
        {
            aucBSSrvList[ulBSSrvIndex] = BS_SERV_INTER_CALL;
            ulBSSrvIndex++;
        }
    }

    for (ulIndex=0; ulIndex<ulSCSrvCnt; ulIndex++)
    {
        switch (aucSCSrvList[ulIndex])
        {
            case SC_SERV_OUTBOUND_CALL:
            case SC_SERV_INBOUND_CALL:
            case SC_SERV_INTERNAL_CALL:
            case SC_SERV_EXTERNAL_CALL:
            case SC_SERV_AUTO_DIALING:
            case SC_SERV_DEMO_TASK:
                break;

            case SC_SERV_PREVIEW_DIALING:
            case SC_SERV_PREDICTIVE_DIALING:
                //DOS_ASSERT(0);
                break;

            case SC_SERV_RECORDING:
                if (ulBSSrvIndex < ulBSSrvCnt)
                {
                    aucBSSrvList[ulBSSrvIndex] = BS_SERV_RECORDING;
                    ulBSSrvIndex++;
                }
                break;

            case SC_SERV_FORWORD_CFB:
            case SC_SERV_FORWORD_CFU:
            case SC_SERV_FORWORD_CFNR:
                break;

            case SC_SERV_BLIND_TRANSFER:
            case SC_SERV_ATTEND_TRANSFER:
                break;

            case SC_SERV_PICK_UP:
                break;

            case SC_SERV_CONFERENCE:
                break;

            case SC_SERV_VOICE_MAIL_RECORD:
            case SC_SERV_VOICE_MAIL_GET:
                break;

            case SC_SERV_SMS_RECV:
            case SC_SERV_SMS_SEND:
            case SC_SERV_MMS_RECV:
            case SC_SERV_MMS_SNED:
                DOS_ASSERT(0);
                break;

            case SC_SERV_FAX:
            case SC_SERV_INTERNAL_SERVICE:
                DOS_ASSERT(0);
                break;

            default:
                break;
        }
    }


    return DOS_SUCC;
}

/* 转接时的特殊处理，根据 sc 的 server type 直接获得对应的BS的server type */
U32 sc_bs_srv_type_adapter_for_transfer(U8 *aucSCSrvList, U32 ulSCSrvCnt, U8 *pucBSSrv)
{
    U32        ulIndex          = 0;
    U32        ulBSSrvIndex     = 0;
    BOOL       blIsOutboundCall = DOS_FALSE;
    BOOL       blIsExternalCall = DOS_FALSE;
    BOOL       blNeedOutbond    = DOS_TRUE;
    U8         ucBSSrv          = U8_BUTT;

    if (DOS_ADDR_INVALID(aucSCSrvList)
        || ulSCSrvCnt <= 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulBSSrvIndex = 0;

    for (ulIndex=0; ulIndex<ulSCSrvCnt; ulIndex++)
    {
        if (SC_SERV_OUTBOUND_CALL == aucSCSrvList[ulIndex])
        {
            blIsOutboundCall = DOS_TRUE;
        }
        else if (SC_SERV_EXTERNAL_CALL == aucSCSrvList[ulIndex])
        {
            blIsExternalCall = DOS_TRUE;
        }
    }

    if (blIsOutboundCall && blIsExternalCall)
    {
        if (blNeedOutbond)
        {
            ucBSSrv = BS_SERV_OUTBAND_CALL;
        }
    }
    else if (!blIsOutboundCall && blIsExternalCall)
    {
        ucBSSrv = BS_SERV_INBAND_CALL;
    }
    else
    {
        ucBSSrv = BS_SERV_INTER_CALL;
    }

    *pucBSSrv = ucBSSrv;

    return DOS_SUCC;
}

U32 sc_get_current_service(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB, U8 *aucServType, U32 ulLen)
{
    U32  ulSrvIndex = 0;

    if (!DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLegCB)
        || DOS_ADDR_INVALID(aucServType) || 0 == ulLen)
    {
        DOS_ASSERT(0);

        sc_log(LOG_LEVEL_WARNING, "%s", "Invalid SCB while send the auth msg.");
        return DOS_FAIL;
    }

    switch (pstSCB->pstServiceList[pstSCB->ulCurrentSrv]->usSrvType)
    {
        case SC_SRV_CALL:
            if (pstLegCB)
            break;
        case SC_SRV_PREVIEW_CALL:
            break;
        case SC_SRV_AUTO_CALL:
            break;
        case SC_SRV_VOICE_VERIFY:
            break;
        case SC_SRV_ACCESS_CODE:
            break;
        case SC_SRV_HOLD:
            break;
        case SC_SRV_TRANSFER:
            break;
        case SC_SRV_INCOMING_QUEUE:
            break;
        case SC_SRV_INTERCEPTION:
            break;
        case SC_SRV_WHISPER:
            break;
        case SC_SRV_MARK_CUSTOM:

            break;
    }

    return BS_SERV_BUTT;
}

/* 发送认证消息到数据到BS */
U32 sc_send_acc_auth2bs(SC_SRV_CB *pstSCB)
{
    return DOS_FAIL;
}
#endif

/* 发送心跳消息到数据到BS */
U32 sc_send_hello2bs(U32 ulClientIndex)
{
    BS_MSG_TAG stMsgHello;

    if (ulClientIndex >= SC_MAX_BS_CLIENT)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_BS), "Invalid client index. Max : %d, Current : %d", SC_MAX_BS_CLIENT, ulClientIndex);
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

    stBSMsgStat.ulHBReq++;;

    return DOS_SUCC;
}



/* 发送认证消息到数据到BS */
U32 sc_send_usr_auth2bs(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    BS_MSG_AUTH    *pstAuthMsg = NULL;
    U32            ulIndex, ulLoop;
#if SC_BS_NEED_RESEND
    SC_BS_MSG_NODE *pstListNode = NULL;
    U32            ulHashIndex = 0;
#endif

    stBSMsgStat.ulAuthReq++;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_BS), "%s", "Invalid SCB while send the auth msg.");
        return DOS_FAIL;
    }

    pstAuthMsg = (BS_MSG_AUTH *)dos_dmem_alloc(sizeof(BS_MSG_AUTH));
    if (DOS_ADDR_INVALID(pstAuthMsg))
    {
        DOS_ASSERT(0);

        sc_trace_scb(pstSCB, "%s", "Alloc memory fail.");
        return DOS_FAIL;
    }

    dos_memzero(pstAuthMsg, sizeof(BS_MSG_AUTH));
    pstAuthMsg->stMsgTag.usVersion = BS_MSG_INTERFACE_VERSION;
    pstAuthMsg->stMsgTag.ulMsgSeq  = g_ulMsgSeq++;
    pstAuthMsg->stMsgTag.ulCRNo    = pstLegCB->ulCBNo;
    pstAuthMsg->stMsgTag.ucMsgType = BS_MSG_USER_AUTH_REQ;
    pstAuthMsg->stMsgTag.ucErrcode = BS_ERR_SUCC;
    pstAuthMsg->stMsgTag.usMsgLen  = sizeof(BS_MSG_AUTH);
    pstAuthMsg->ulUserID           = pstSCB->ulCustomerID;
    pstAuthMsg->ulAgentID          = pstSCB->ulAgentID;
    pstAuthMsg->ulCustomerID       = pstSCB->ulCustomerID;
    pstAuthMsg->ulAccountID        = pstSCB->ulCustomerID;
    pstAuthMsg->ulTaskID           = U32_BUTT;
    pstAuthMsg->lBalance           = 0;
    pstAuthMsg->ulTimeStamp        = time(NULL);
    pstAuthMsg->ulSessionNum       = 0;
    pstAuthMsg->ulMaxSession       = 0;
    pstAuthMsg->ucBalanceWarning   = 0;

    for (ulIndex=0, ulLoop=0; ulLoop<SC_MAX_SERVICE_TYPE; ulLoop++)
    {
        if (pstSCB->aucServType[ulLoop] != 0 && pstSCB->aucServType[ulLoop] < BS_SERV_BUTT)
        {
            pstAuthMsg->aucServType[ulIndex] = pstSCB->aucServType[ulLoop];
            ulLoop++;
        }
    }

    dos_strncpy(pstAuthMsg->szSessionID, pstLegCB->szUUID, sizeof(pstAuthMsg->szSessionID));
    pstAuthMsg->szSessionID[sizeof(pstAuthMsg->szSessionID) - 1] = '\0';

    dos_strncpy(pstAuthMsg->szCaller, pstLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstAuthMsg->szCaller));
    pstAuthMsg->szCaller[sizeof(pstAuthMsg->szCaller) - 1] = '\0';

    dos_strncpy(pstAuthMsg->szCallee, pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstAuthMsg->szCallee));
    pstAuthMsg->szCallee[sizeof(pstAuthMsg->szCallee) - 1] = '\0';

    dos_strncpy(pstAuthMsg->szCID, pstLegCB->stCall.stNumInfo.szANI, sizeof(pstAuthMsg->szCID));
    pstAuthMsg->szCID[sizeof(pstAuthMsg->szCID) - 1] = '\0';
#if 0
    if (pstLegCB->ulCBNo == pstSCB->stCall.ulCallingLegNo)
    {
        if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling) && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
        {
            dos_strncpy(pstAuthMsg->szAgentNum, pstSCB->stCall.pstAgentCalling->pstAgentInfo->szEmpNo, sizeof(pstAuthMsg->szAgentNum));
            pstAuthMsg->szAgentNum[sizeof(pstAuthMsg->szAgentNum) - 1] = '\0';
        }
        else
        {
            pstAuthMsg->szAgentNum[0] = '\0';
        }
    }
    else
    {
        if (DOS_ADDR_INVALID(pstSCB->stCall.pstAgentCallee) || pstSCB->stCall.pstAgentCallee->pstAgentInfo)
        {
            dos_strncpy(pstAuthMsg->szAgentNum, pstSCB->stCall.pstAgentCallee->pstAgentInfo->szEmpNo, sizeof(pstAuthMsg->szAgentNum));
            pstAuthMsg->szAgentNum[sizeof(pstAuthMsg->szAgentNum) - 1] = '\0';
        }
        else
        {
            pstAuthMsg->szAgentNum[0] = '\0';
        }
    }
#endif
#if SC_BS_NEED_RESEND
    pstListNode = dos_dmem_alloc(sizeof(SC_BS_MSG_NODE));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        DOS_ASSERT(0);

        sc_trace_scb(pstSCB, "%s", "Alloc memory for list node fail.");
        return DOS_FAIL;
    }
    pstListNode->pData = (VOID *)pstAuthMsg;
    pstListNode->ulFailCnt = 0;
    pstListNode->ulRCNo = pstSCB->usSCBNo;
    pstListNode->ulLength = sizeof(BS_MSG_AUTH);
    pstListNode->ulSeq = pstAuthMsg->stMsgTag.ulMsgSeq;
    pstListNode->blNeedSyn = DOS_FALSE;
    pstListNode->hTmrSendInterval = NULL;
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

        sc_trace_scb(pstSCB, "%s", "Start the timer fail while send the auth msg.");
        return DOS_FAIL;
    }
#endif

    if (sc_send_msg2bs((BS_MSG_TAG *)pstAuthMsg, sizeof(BS_MSG_AUTH)) != DOS_SUCC)
    {
        DOS_ASSERT(0);

#if SC_BS_NEED_RESEND
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
#endif

        dos_dmem_free(pstAuthMsg);
        pstAuthMsg = NULL;

        sc_trace_scb(pstSCB, "%s", "Send Auth msg FAIL.");
        return DOS_FAIL;
    }

    stBSMsgStat.ulAuthReqSend++;

#if (!SC_BS_NEED_RESEND)
    dos_dmem_free(pstAuthMsg);
    pstAuthMsg = NULL;
#endif

    sc_trace_scb(pstSCB, "%s", "Send Auth msg SUCC.");

    return DOS_SUCC;
}

#if 0
/* 发送心跳消息到数据到BS */
U32 sc_send_hello2bs(U32 ulClientIndex)
{
    BS_MSG_TAG stMsgHello;

    if (ulClientIndex >= SC_MAX_BS_CLIENT)
    {
        DOS_ASSERT(0);

        sc_logr_debug(NULL, SC_BS, "Invalid client index. Max : %d, Current : %d", SC_MAX_BS_CLIENT, ulClientIndex);
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

    stBSMsgStat.ulHBReq++;;

    return DOS_SUCC;
}


/* SC启动时发送启动指示消息 */
U32 sc_send_startup2bs()
{
    /* 暂时不实现 */
    return DOS_SUCC;
}

/* 发送余额查询消息 */
U32 sc_send_balance_query2bs(SC_SCB_ST *pstSCB)
{
    BS_MSG_BALANCE_QUERY *pstQueryMsg = NULL;
#if SC_BS_NEED_RESEND
    SC_BS_MSG_NODE       *pstListNode = NULL;
    U32                  ulHashIndex = 0;
#endif

    if (DOS_ADDR_INVALID(pstSCB))
    {
        return DOS_FAIL;
    }

    sc_log_digest_print("Start send balance query to BS. customer : %u"
        , pstSCB->ulCustomID);

    pstQueryMsg = dos_dmem_alloc(sizeof(BS_MSG_BALANCE_QUERY));
    if (DOS_ADDR_INVALID(pstQueryMsg))
    {
        DOS_ASSERT(0);

        sc_logr_warning(pstSCB, SC_BS, "%s", "Alloc memory fail.");
        return DOS_FAIL;
    }

    dos_memzero(pstQueryMsg, sizeof(BS_MSG_BALANCE_QUERY));
    pstQueryMsg->stMsgTag.usVersion = BS_MSG_INTERFACE_VERSION;
    pstQueryMsg->stMsgTag.ulMsgSeq  = g_ulMsgSeq++;
    pstQueryMsg->stMsgTag.ulCRNo    = pstSCB->usSCBNo;
    pstQueryMsg->stMsgTag.ucMsgType = BS_MSG_BALANCE_QUERY_REQ;
    pstQueryMsg->stMsgTag.ucErrcode = BS_ERR_SUCC;
    pstQueryMsg->stMsgTag.usMsgLen  = sizeof(BS_MSG_BALANCE_QUERY);
    pstQueryMsg->ulUserID           = pstSCB->ulCustomID;
    pstQueryMsg->ulCustomerID       = pstSCB->ulCustomID;
    pstQueryMsg->ulAccountID        = pstSCB->ulCustomID;
    pstQueryMsg->LBalance           = 100;
    pstQueryMsg->ucBalanceWarning   = 0;

#if SC_BS_NEED_RESEND
    pstListNode = dos_dmem_alloc(sizeof(SC_BS_MSG_NODE));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        DOS_ASSERT(0);

        sc_logr_warning(pstSCB, SC_BS, "%s", "Alloc memory for list node fail.");
        return DOS_FAIL;
    }
    pstListNode->pData = (VOID *)pstQueryMsg;
    pstListNode->ulFailCnt = 0;
    pstListNode->ulRCNo = U32_BUTT;
    pstListNode->ulLength = sizeof(BS_MSG_BALANCE_QUERY);
    pstListNode->ulSeq = pstQueryMsg->stMsgTag.ulMsgSeq;
    pstListNode->blNeedSyn = DOS_FALSE;
    pstListNode->hTmrSendInterval = NULL;
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
        sc_logr_notice(pstSCB, SC_BS, "%s", "Start the timer fail while send the auth msg.");
        return DOS_FAIL;
    }
#endif

    if (sc_send_msg2bs((BS_MSG_TAG *)pstQueryMsg, sizeof(BS_MSG_BALANCE_QUERY)) != DOS_SUCC)
    {
        DOS_ASSERT(0);

#if SC_BS_NEED_RESEND
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
#endif

        dos_dmem_free(pstQueryMsg);
        pstQueryMsg = NULL;

        sc_logr_notice(pstSCB, SC_BS, "%s", "Send Auth msg fail.");
        return DOS_FAIL;
    }

#if (!SC_BS_NEED_RESEND)
    dos_dmem_free(pstQueryMsg);
    pstQueryMsg = NULL;
#endif

    return DOS_SUCC;

}

#define SC_CHECK_SERVICE(pstSCB, ulSrvType)                        \
        ((pstSCB)                                                 \
            && ((pstSCB)->aucServiceType[0] == (ulSrvType)         \
            || (pstSCB)->aucServiceType[1] == (ulSrvType)          \
            || (pstSCB)->aucServiceType[2] == (ulSrvType)          \
            || (pstSCB)->aucServiceType[3] == (ulSrvType)))

/* 发送终止计费消息 */
U32 sc_send_billing_stop2bs(SC_SCB_ST *pstSCB)
{
    U32                   ulCurrentLeg = 0;
    SC_SCB_ST             *pstSCB2 = NULL, *pstFirstSCB = NULL, *pstSecondSCB = NULL;
    BS_MSG_CDR            *pstCDRMsg = NULL;
    S32                   i = 0;
#if SC_BS_NEED_RESEND
    SC_BS_MSG_NODE        *pstListNode = NULL;
    U32                   ulHashIndex = 0;
#endif

    stBSMsgStat.ulBillingReq++;

    /* 当前呼叫没有关联SCB时，就直接吧当前业务控制块作为主LEG */
    if (U16_BUTT == pstSCB->usOtherSCBNo)
    {
        pstFirstSCB = pstSCB;

        goto prepare_msg;
    }

    pstSCB2 = sc_scb_get(pstSCB->usOtherSCBNo);
    if (DOS_ADDR_INVALID(pstSCB2))
    {
        pstFirstSCB = pstSCB;

        goto prepare_msg;
    }

    if (pstSCB->bIsFristSCB)
    {
        pstFirstSCB = pstSCB;
        pstSecondSCB = pstSCB2;

        goto prepare_msg;
    }

    if (pstSCB2->bIsFristSCB)
    {
        pstFirstSCB = pstSCB2;
        pstSecondSCB = pstSCB;

        goto prepare_msg;
    }

    /* 确定业务类型，决定LEG的顺序，业务控制模块的较为详细，BS模块较为粗略 */
    /* \Voice Mail 记录，需要将呼入作为主LEG，VM记录是辅助LEG */
    if (SC_CHECK_SERVICE(pstSCB, SC_SERV_VOICE_MAIL_RECORD))
    {
        pstFirstSCB = pstSCB2;
        goto prepare_msg;
    }

    if (SC_CHECK_SERVICE(pstSCB2, SC_SERV_VOICE_MAIL_RECORD))
    {
        pstFirstSCB = pstSCB;
        goto prepare_msg;
    }

    /* \Voice Mail 获取，需要将呼入作为辅助LEG，VM获取是主LEG */
    if (SC_CHECK_SERVICE(pstSCB, SC_SERV_VOICE_MAIL_GET))
    {
        pstFirstSCB = pstSCB;
        goto prepare_msg;
    }

    if (SC_CHECK_SERVICE(pstSCB2, SC_SERV_VOICE_MAIL_GET))
    {
        pstFirstSCB = pstSCB2;
        goto prepare_msg;
    }

    /* \如果包含特殊业务，就先处理特殊业务 */
    if (SC_CHECK_SERVICE(pstSCB, SC_SERV_AUTO_DIALING)
        || SC_CHECK_SERVICE(pstSCB, SC_SERV_PREDICTIVE_DIALING)
        || SC_CHECK_SERVICE(pstSCB, SC_SERV_PREVIEW_DIALING))
    {
        pstFirstSCB = pstSCB;
        goto prepare_msg;
    }

    if (SC_CHECK_SERVICE(pstSCB2, SC_SERV_AUTO_DIALING)
        || SC_CHECK_SERVICE(pstSCB2, SC_SERV_PREDICTIVE_DIALING)
        || SC_CHECK_SERVICE(pstSCB2, SC_SERV_PREVIEW_DIALING))
    {
        pstFirstSCB = pstSCB2;
        goto prepare_msg;
    }

    /* \PSTN呼入，到PSTN等类型呼叫，需要使用和PSTN通讯的leg作为主leg */
    if (SC_CHECK_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL)
        && SC_CHECK_SERVICE(pstSCB, SC_SERV_INBOUND_CALL)
        && SC_CALLER == pstSCB->ucLegRole)
    {
        pstFirstSCB = pstSCB;
        goto prepare_msg;
    }

    if (SC_CHECK_SERVICE(pstSCB2, SC_SERV_EXTERNAL_CALL)
        && SC_CHECK_SERVICE(pstSCB2, SC_SERV_INBOUND_CALL)
        && SC_CALLER == pstSCB2->ucLegRole)
    {
        pstFirstSCB = pstSCB2;
        goto prepare_msg;
    }

    /* \PSTN呼出，到PSTN等类型呼叫，需要使用和PSTN通讯的leg作为主leg */
    if (SC_CHECK_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL)
        && SC_CHECK_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL)
        && SC_CALLEE == pstSCB->ucLegRole)
    {
        pstFirstSCB = pstSCB;
        goto prepare_msg;
    }

    if (SC_CHECK_SERVICE(pstSCB2, SC_SERV_EXTERNAL_CALL)
        && SC_CHECK_SERVICE(pstSCB2, SC_SERV_OUTBOUND_CALL)
        && SC_CALLEE == pstSCB2->ucLegRole)
    {
        pstFirstSCB = pstSCB2;
        goto prepare_msg;
    }

    /* \内部呼叫，需要使用被叫的leg作为主leg */
    if (SC_CHECK_SERVICE(pstSCB, SC_SERV_INTERNAL_CALL))
    {
        pstFirstSCB = pstSCB;
        goto prepare_msg;
    }

    if (SC_CHECK_SERVICE(pstSCB2, SC_SERV_INTERNAL_CALL))
    {
        pstFirstSCB = pstSCB2;
        goto prepare_msg;
    }

    /* @TODO: 其他情况 */


    /* 默认情况，将当前SCB作为主LEG */
    pstFirstSCB = pstSCB;

prepare_msg:
    /* 申请资源 */
    pstCDRMsg = dos_dmem_alloc(sizeof(BS_MSG_CDR));
    if (DOS_ADDR_INVALID(pstCDRMsg))
    {
        DOS_ASSERT(0);

        sc_logr_notice(pstSCB, SC_BS, "%s", "Alloc memory for the CDR msg fail.");
        return DOS_FAIL;
    }

    dos_memzero(pstCDRMsg, sizeof(BS_MSG_CDR));

    if (pstFirstSCB != pstSCB)
    {
        pstSecondSCB = pstSCB;
    }
    else
    {
        pstSecondSCB = pstSCB2;
    }

    ulCurrentLeg = 0;
    pstCDRMsg->ucLegNum = 0;

    if (DOS_ADDR_VALID(pstFirstSCB))
    {
        /* 填充数据 */
        pstCDRMsg->stMsgTag.usVersion = BS_MSG_INTERFACE_VERSION;
        pstCDRMsg->stMsgTag.ulMsgSeq  = g_ulMsgSeq++;
        pstCDRMsg->stMsgTag.ulCRNo    = pstFirstSCB->usSCBNo;
        pstCDRMsg->stMsgTag.ucMsgType = BS_MSG_BILLING_STOP_REQ;
        pstCDRMsg->stMsgTag.ucErrcode = BS_ERR_SUCC;
        pstCDRMsg->stMsgTag.usMsgLen  = sizeof(BS_MSG_BALANCE_QUERY);

        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulCDRMark = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulUserID = (U32_BUTT == pstFirstSCB->ulCustomID) ? 0 : pstFirstSCB->ulCustomID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAgentID = (U32_BUTT == pstFirstSCB->ulAgentID) ? 0 : pstFirstSCB->ulAgentID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulCustomerID = (U32_BUTT == pstFirstSCB->ulCustomID) ? 0 : pstFirstSCB->ulCustomID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAccountID = (U32_BUTT == pstFirstSCB->ulCustomID) ? 0 : pstFirstSCB->ulCustomID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulTaskID = (U32_BUTT == pstFirstSCB->ulTaskID) ? 0 : pstFirstSCB->ulTaskID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szRecordFile[0] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID, pstFirstSCB->szUUID, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller, pstFirstSCB->szCallerNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee, pstFirstSCB->szCalleeNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID, pstFirstSCB->szANINum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum, pstFirstSCB->szSiteNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum) - 1] = '\0';

        pthread_mutex_lock(&pstSCB->mutexSCBLock);
        if (pstFirstSCB->pstExtraData)
        {
            if (pstFirstSCB->pstExtraData->ulStartTimeStamp == 0)
            {
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulStartTimeStamp = time(NULL);

                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulRingTimeStamp = pstFirstSCB->pstExtraData->ulRingTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAnswerTimeStamp = time(NULL);
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulIVRFinishTimeStamp= pstFirstSCB->pstExtraData->ulIVRFinishTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulDTMFTimeStamp = pstFirstSCB->pstExtraData->ulDTMFTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulBridgeTimeStamp = pstFirstSCB->pstExtraData->ulBridgeTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulByeTimeStamp = time(NULL);
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ucPayloadType = pstFirstSCB->pstExtraData->ucPayloadType;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ucPacketLossRate = pstFirstSCB->pstExtraData->ucPacketLossRate;

            }
            else
            {
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulStartTimeStamp = pstFirstSCB->pstExtraData->ulStartTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulRingTimeStamp = pstFirstSCB->pstExtraData->ulRingTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAnswerTimeStamp = pstFirstSCB->pstExtraData->ulAnswerTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulIVRFinishTimeStamp= pstFirstSCB->pstExtraData->ulIVRFinishTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulDTMFTimeStamp = pstFirstSCB->pstExtraData->ulDTMFTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulBridgeTimeStamp = pstFirstSCB->pstExtraData->ulBridgeTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulByeTimeStamp = pstFirstSCB->pstExtraData->ulByeTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ucPayloadType = pstFirstSCB->pstExtraData->ucPayloadType;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ucPacketLossRate = pstFirstSCB->pstExtraData->ucPacketLossRate;
            }
        }
        else
        {
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulStartTimeStamp = time(NULL);
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAnswerTimeStamp = time(NULL);
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulByeTimeStamp = time(NULL);
        }

        if (pstFirstSCB->pszRecordFile)
        {
            dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szRecordFile, pstFirstSCB->pszRecordFile, BS_MAX_RECORD_FILE_NAME_LEN);
            pstCDRMsg->astSessionLeg[ulCurrentLeg].szRecordFile[BS_MAX_RECORD_FILE_NAME_LEN - 1] = '\0';
        }
        pthread_mutex_unlock(&pstSCB->mutexSCBLock);

        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulHoldCnt = pstFirstSCB->usHoldCnt;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulHoldTimeLen = pstFirstSCB->usHoldTotalTime;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].aulPeerIP[0] = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].aulPeerIP[1] = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].aulPeerIP[2] = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].aulPeerIP[3] = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].usPeerTrunkID = (U32_BUTT == pstFirstSCB->ulTrunkID) ? 0 : pstFirstSCB->ulTrunkID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].usTerminateCause = pstFirstSCB->usTerminationCause;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ucReleasePart = 0;
        if (!pstFirstSCB->bIsNotSrvAdapter)
        {
            sc_bs_srv_type_adapter(pstFirstSCB->aucServiceType
                                , sizeof(pstFirstSCB->aucServiceType)
                                , pstCDRMsg->astSessionLeg[ulCurrentLeg].aucServType
                                , BS_MAX_SERVICE_TYPE_IN_SESSION);

        }
        else
        {
            for (i=0; i<BS_MAX_SERVICE_TYPE_IN_SESSION; i++)
            {
                pstCDRMsg->astSessionLeg[ulCurrentLeg].aucServType[i] = pstFirstSCB->aucServiceType[i];
            }
        }

        ulCurrentLeg++;
        pstCDRMsg->ucLegNum++;
    }

    if (DOS_ADDR_VALID(pstSecondSCB))
    {
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulCDRMark = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulUserID = (U32_BUTT == pstSecondSCB->ulCustomID) ? 0 : pstSecondSCB->ulCustomID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAgentID = (U32_BUTT == pstSecondSCB->ulAgentID) ? 0 : pstSecondSCB->ulAgentID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulCustomerID = (U32_BUTT == pstSecondSCB->ulCustomID) ? 0 : pstSecondSCB->ulCustomID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAccountID = (U32_BUTT == pstSecondSCB->ulCustomID) ? 0 : pstSecondSCB->ulCustomID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulTaskID = (U32_BUTT == pstSecondSCB->ulTaskID) ? 0 : pstSecondSCB->ulTaskID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szRecordFile[0] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID, pstSecondSCB->szUUID, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szSessionID) - 1] = '\0';

        if (!SC_CHECK_SERVICE(pstSecondSCB, SC_SERV_EXTERNAL_CALL))
        {
            /* 内部呼叫 */
            dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee, pstFirstSCB->szCalleeNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee));
            pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee) - 1] = '\0';
            dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller, pstSecondSCB->szCallerNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller));
            pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller) - 1] = '\0';
        }
        else
        {
            dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee, pstSecondSCB->szCalleeNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee));
            pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCallee) - 1] = '\0';
            dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller, pstSecondSCB->szCallerNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller));
            pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCaller) - 1] = '\0';
        }


        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID, pstSecondSCB->szANINum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szCID) - 1] = '\0';

        dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum, pstSecondSCB->szSiteNum, sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum));
        pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum[sizeof(pstCDRMsg->astSessionLeg[ulCurrentLeg].szAgentNum) - 1] = '\0';

        pthread_mutex_lock(&pstSCB->mutexSCBLock);
        if (pstSecondSCB->pstExtraData)
        {
            if (pstSecondSCB->pstExtraData->ulStartTimeStamp == 0)
            {
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulStartTimeStamp = time(NULL);
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulRingTimeStamp = pstSecondSCB->pstExtraData->ulRingTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAnswerTimeStamp = time(NULL);
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulIVRFinishTimeStamp= pstSecondSCB->pstExtraData->ulIVRFinishTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulDTMFTimeStamp = pstSecondSCB->pstExtraData->ulDTMFTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulBridgeTimeStamp = pstSecondSCB->pstExtraData->ulBridgeTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulByeTimeStamp = time(NULL);
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ucPayloadType = pstSecondSCB->pstExtraData->ucPayloadType;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ucPacketLossRate = pstSecondSCB->pstExtraData->ucPacketLossRate;
            }
            else
            {
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulStartTimeStamp = pstSecondSCB->pstExtraData->ulStartTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulRingTimeStamp = pstSecondSCB->pstExtraData->ulRingTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAnswerTimeStamp = pstSecondSCB->pstExtraData->ulAnswerTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulIVRFinishTimeStamp= pstSecondSCB->pstExtraData->ulIVRFinishTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulDTMFTimeStamp = pstSecondSCB->pstExtraData->ulDTMFTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulBridgeTimeStamp = pstSecondSCB->pstExtraData->ulBridgeTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ulByeTimeStamp = pstSecondSCB->pstExtraData->ulByeTimeStamp;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ucPayloadType = pstSecondSCB->pstExtraData->ucPayloadType;
                pstCDRMsg->astSessionLeg[ulCurrentLeg].ucPacketLossRate = pstSecondSCB->pstExtraData->ucPacketLossRate;
            }
        }
        else
        {
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulStartTimeStamp = time(NULL);
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulAnswerTimeStamp = time(NULL);
            pstCDRMsg->astSessionLeg[ulCurrentLeg].ulByeTimeStamp = time(NULL);
        }

        if (pstSecondSCB->pszRecordFile)
        {
            dos_strncpy(pstCDRMsg->astSessionLeg[ulCurrentLeg].szRecordFile, pstSecondSCB->pszRecordFile, BS_MAX_RECORD_FILE_NAME_LEN);
            pstCDRMsg->astSessionLeg[ulCurrentLeg].szRecordFile[BS_MAX_RECORD_FILE_NAME_LEN - 1] = '\0';
        }
        pthread_mutex_unlock(&pstSCB->mutexSCBLock);

        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulHoldCnt = pstSecondSCB->usHoldCnt;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ulHoldTimeLen = pstSecondSCB->usHoldTotalTime;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].aulPeerIP[0] = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].aulPeerIP[1] = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].aulPeerIP[2] = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].aulPeerIP[3] = 0;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].usPeerTrunkID = (U32_BUTT == pstSecondSCB->ulTrunkID) ? 0 : pstSecondSCB->ulTrunkID;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].usTerminateCause = pstSecondSCB->usTerminationCause;
        pstCDRMsg->astSessionLeg[ulCurrentLeg].ucReleasePart = 0;
        if (!pstFirstSCB->bIsNotSrvAdapter)
        {
            sc_bs_srv_type_adapter(pstSecondSCB->aucServiceType
                                , sizeof(pstSecondSCB->aucServiceType)
                                , pstCDRMsg->astSessionLeg[ulCurrentLeg].aucServType
                                , BS_MAX_SERVICE_TYPE_IN_SESSION);
        }
        else
        {
            for (i=0; i<BS_MAX_SERVICE_TYPE_IN_SESSION; i++)
            {
                pstCDRMsg->astSessionLeg[ulCurrentLeg].aucServType[i] = pstSecondSCB->aucServiceType[i];
            }
        }

        ulCurrentLeg++;
        pstCDRMsg->ucLegNum++;
    }

    /* 如果没有leg被加入，就放弃 */
    if (ulCurrentLeg <= 0)
    {
        DOS_ASSERT(0);

        sc_logr_notice(pstSCB, SC_BS, "%s", "There no leg created while send cdr msg.");
        return DOS_FAIL;
    }

    /* 记录当前有几个LEG */
    //pstCDRMsg->ucLegNum = (U8)ulCurrentLeg;

#if SC_BS_NEED_RESEND
    /* 将消息存放hash表的节点 */
    pstListNode = dos_dmem_alloc(sizeof(SC_BS_MSG_NODE));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        DOS_ASSERT(0);

        sc_logr_warning(pstSCB, SC_BS, "%s", "Alloc memory for list node fail.");
        return DOS_FAIL;
    }
    pstListNode->pData = (VOID *)pstCDRMsg;
    pstListNode->ulFailCnt = 0;
    pstListNode->ulRCNo = U32_BUTT;
    pstListNode->ulLength = sizeof(BS_MSG_BALANCE_QUERY);
    pstListNode->ulSeq = pstCDRMsg->stMsgTag.ulMsgSeq;
    pstListNode->blNeedSyn = DOS_FALSE;
    pstListNode->hTmrSendInterval = NULL;
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

        sc_logr_notice(pstSCB, SC_BS, "%s", "Start the timer fail while send the auth msg.");
        return DOS_FAIL;
    }
#endif

    /* 发送数据 */
    if (sc_send_msg2bs((BS_MSG_TAG *)pstCDRMsg, sizeof(BS_MSG_CDR)) != DOS_SUCC)
    {
        DOS_ASSERT(0);

#if SC_BS_NEED_RESEND
        /* 停定时器 */
        dos_tmr_stop(&pstListNode->hTmrSendInterval);

        /* 删除缓存 */
        pthread_mutex_lock(&g_mutexMsgList);
        hash_delete_node(g_pstMsgList, (HASH_NODE_S *)pstListNode, ulHashIndex);


        pstListNode->pData = NULL;
        dos_dmem_free(pstListNode);
        pstListNode = NULL;

        pthread_mutex_unlock(&g_mutexMsgList);
#endif

        dos_dmem_free(pstCDRMsg);
        pstCDRMsg = NULL;

        sc_logr_notice(pstSCB, SC_BS, "%s", "Send Auth msg fail.");
        return DOS_FAIL;
    }

    stBSMsgStat.ulBillingReqSend++;

#if (!SC_BS_NEED_RESEND)
    dos_dmem_free(pstCDRMsg);
    pstCDRMsg = NULL;
#endif

    return DOS_SUCC;
}



/* 发送初始计费消息 */
U32 sc_send_billing_start2bs(SC_SCB_ST *pstSCB)
{
    return sc_send_billing_stop2bs(pstSCB);
}

/* 发送中间计费消息 */
U32 sc_send_billing_update2bs(SC_SCB_ST *pstSCB)
{
    return sc_send_billing_stop2bs(pstSCB);
}

#endif

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


