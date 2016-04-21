/**
 * @file : sc_event_fsm.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 业务控制模块各个业务状态机实现
 *
 * @date: 2016年1月13日
 * @arthur: Larry
 */
#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_def.h"
#include "sc_pub.h"
#include "bs_pub.h"
#include "sc_res.h"
#include "sc_hint.h"
#include "sc_debug.h"
#include "sc_pub.h"
#include "sc_bs.h"

extern SC_ACCESS_CODE_LIST_ST astSCAccessList[];

VOID sc_hungup_third_leg(U32 ulScbNo)
{
    SC_SRV_CB *pstSCB = NULL;
    SC_LEG_CB *pstLeg = NULL;

    pstSCB = sc_scb_get(ulScbNo);
    if (DOS_ADDR_INVALID(pstSCB))
    {
        return;
    }

    /* 先查看是哪种业务，现在只有监听和耳语两种 */
    if (pstSCB->stInterception.stSCBTag.bValid)
    {
        pstLeg = sc_lcb_get(pstSCB->stInterception.ulLegNo);
        if (DOS_ADDR_INVALID(pstLeg))
        {
            sc_scb_free(pstSCB);
            return;
        }

        switch (pstSCB->stInterception.stSCBTag.usStatus)
        {
            case SC_INTERCEPTION_IDEL:
            case SC_INTERCEPTION_AUTH:
            case SC_INTERCEPTION_EXEC:
                /* 释放掉就行了 */
                sc_scb_free(pstSCB);
                sc_lcb_free(pstLeg);
                break;

            case SC_INTERCEPTION_PROC:
            case SC_INTERCEPTION_ALERTING:
            case SC_INTERCEPTION_ACTIVE:
            case SC_INTERCEPTION_RELEASE:
                /* 挂断电话 */
                sc_req_hungup(pstSCB->ulSCBNo, pstLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                break;

            default:
                break;
        }
    }
    else if (pstSCB->stWhispered.stSCBTag.bValid)
    {
        pstLeg = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
        if (DOS_ADDR_INVALID(pstLeg))
        {
            sc_scb_free(pstSCB);
            return;
        }

        switch (pstSCB->stWhispered.stSCBTag.usStatus)
        {
            case SC_WHISPER_IDEL:
            case SC_WHISPER_AUTH:
            case SC_WHISPER_EXEC:
                /* 释放掉就行了 */
                sc_scb_free(pstSCB);
                sc_lcb_free(pstLeg);
                break;

            case SC_WHISPER_PROC:
            case SC_WHISPER_ALERTING:
            case SC_WHISPER_ACTIVE:
            case SC_WHISPER_RELEASE:
                sc_req_hungup(pstSCB->ulSCBNo, pstLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                break;

            default:
                break;
        }
    }
    else
    {
        /* 暂时不处理 */
    }

    return;
}

U32 sc_errcode_transfer_from_intererr(U32 ulInterErr)
{
    U32 ulErrNo = CC_ERR_NO_REASON;

    switch (ulInterErr)
    {
        case SC_ERR_INVALID_MSG:
        case SC_ERR_ALLOC_RES_FAIL:
        case SC_ERR_EXEC_FAIL:
        case SC_ERR_LEG_NOT_EXIST:
            ulErrNo = CC_ERR_SC_MESSAGE_PARAM_ERR;
            break;
        case SC_ERR_CALL_FAIL:
        case SC_ERR_BRIDGE_FAIL:
        case SC_ERR_RECORD_FAIL:
        case SC_ERR_BREAK_FAIL:
            break;
        default:
            break;
    }

    return ulErrNo;
}

/**
  * 先判断是够符合客户标记的条件。
  *  1、按键的是坐席
  *  2、对端是客户
  * 如果不符合条件，不需要处理
  */
U32 sc_access_mark_customer(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB, U32 ulKey)
{
    BOOL                bIsMatch        = DOS_FALSE;
    SC_AGENT_NODE_ST    *pstAgentNode   = NULL;
    U32                 ulCustomerLegNo;

    if (DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstLegCB)
        || ulKey > 9)
    {
        return DOS_FAIL;
    }

    pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_ACTIVE;

    /* 判断是否符合条件 */
    if (pstSCB->stCall.stSCBTag.bValid)
    {
        /* 基础呼叫 */
        if (pstLegCB->ulCBNo == pstSCB->stCall.ulCallingLegNo
            && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
            && DOS_ADDR_INVALID(pstSCB->stCall.pstAgentCallee))
        {
            ulCustomerLegNo = pstSCB->stCall.ulCalleeLegNo;
            bIsMatch = DOS_TRUE;
        }
        else if (pstLegCB->ulCBNo == pstSCB->stCall.ulCalleeLegNo
            && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
            && DOS_ADDR_INVALID(pstSCB->stCall.pstAgentCalling))
        {
            ulCustomerLegNo = pstSCB->stCall.ulCallingLegNo;
            bIsMatch = DOS_TRUE;
        }
    }
    else if (pstSCB->stPreviewCall.stSCBTag.bValid)
    {
        /* 预览外呼 */
        if (pstLegCB->ulCBNo == pstSCB->stPreviewCall.ulCallingLegNo)
        {
            ulCustomerLegNo = pstSCB->stPreviewCall.ulCalleeLegNo;
            bIsMatch = DOS_TRUE;
        }
    }
    else if (pstSCB->stAutoCall.stSCBTag.bValid)
    {
        /* 群呼任务 */
        if (pstLegCB->ulCBNo == pstSCB->stAutoCall.ulCalleeLegNo)
        {
            ulCustomerLegNo = pstSCB->stAutoCall.ulCallingLegNo;
            bIsMatch = DOS_TRUE;
        }
    }
    else if (pstSCB->stDemoTask.stSCBTag.bValid)
    {
        /* 群呼任务demo */
        if (pstLegCB->ulCBNo == pstSCB->stDemoTask.ulCalleeLegNo)
        {
            ulCustomerLegNo = pstSCB->stDemoTask.ulCallingLegNo;
            bIsMatch = DOS_TRUE;
        }
    }
    else if (pstSCB->stAutoPreview.stSCBTag.bValid)
    {
        /* 预览外呼群呼任务 */
        if (pstLegCB->ulCBNo == pstSCB->stAutoPreview.ulCallingLegNo)
        {
            ulCustomerLegNo = pstSCB->stAutoPreview.ulCalleeLegNo;
            bIsMatch = DOS_TRUE;
        }
    }

    if (!bIsMatch)
    {
        return DOS_FAIL;
    }

    pstAgentNode = sc_agent_get_by_id(pstSCB->stAccessCode.ulAgentID);
    if (DOS_ADDR_INVALID(pstAgentNode) || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        return DOS_FAIL;
    }

    /* 客户标记 */
    sc_agent_marker_update_req(pstSCB->ulCustomerID, pstSCB->stAccessCode.ulAgentID, ulKey, pstAgentNode->pstAgentInfo->szLastCustomerNum);
    pstAgentNode->pstAgentInfo->bMarkCustomer = DOS_TRUE;

    /* 挂断客户的电话 */
    sc_req_hungup(pstSCB->ulSCBNo, ulCustomerLegNo, CC_ERR_NORMAL_CLEAR);

    return DOS_SUCC;
}

U32 sc_access_balance_enquiry(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    return sc_send_balance_query2bs(pstSCB, pstLegCB);
}

U32 sc_access_hungup(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    U32                 ulOtherLegNo    = U32_BUTT;
    SC_LEG_CB           *pstOtherLeg    = NULL;

    if (pstSCB->stCall.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stCall.ulCallingLegNo)
        {
            ulOtherLegNo = pstSCB->stCall.ulCalleeLegNo;
        }
        else
        {
            ulOtherLegNo = pstSCB->stCall.ulCallingLegNo;
        }
    }
    else if (pstSCB->stPreviewCall.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stPreviewCall.ulCallingLegNo)
        {
            ulOtherLegNo = pstSCB->stPreviewCall.ulCalleeLegNo;
        }
    }
    else if (pstSCB->stAutoCall.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stAutoCall.ulCalleeLegNo)
        {
            ulOtherLegNo = pstSCB->stAutoCall.ulCallingLegNo;
        }
    }
    else if (pstSCB->stTransfer.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stTransfer.ulPublishLegNo)
        {
            ulOtherLegNo = pstSCB->stTransfer.ulSubLegNo;
        }
    }
    else if (pstSCB->stAutoPreview.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stAutoPreview.ulCallingLegNo)
        {
            ulOtherLegNo = pstSCB->stAutoPreview.ulCalleeLegNo;
        }
    }
    else if (pstSCB->stCorSwitchboard.stSCBTag.bValid)
    {
        if (pstLegCB->ulCBNo == pstSCB->stCorSwitchboard.ulCalleeLegNo)
        {
            ulOtherLegNo = pstSCB->stCorSwitchboard.ulCallingLegNo;
        }
    }

    pstOtherLeg = sc_lcb_get(ulOtherLegNo);
    if (DOS_ADDR_INVALID(pstOtherLeg))
    {
        return DOS_SUCC;
    }

    sc_req_hungup(pstSCB->ulSCBNo, ulOtherLegNo, CC_ERR_NORMAL_CLEAR);

    return DOS_SUCC;
}

/** 转接
*/
U32 sc_access_transfer(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    S8                  pszEmpNum[SC_NUM_LENGTH]    = {0};
    SC_AGENT_NODE_ST    *pstAgentNode               = NULL;
    U32                 ulRet                       = DOS_FAIL;
    SC_LEG_CB           *pstPublishLeg              = NULL;
    SC_CALL_TRANSFER_ST stTransfer;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCB->stAccessCode.stSCBTag.bWaitingExit = DOS_TRUE;
    /* 获得要转接的坐席的工号 */
    if (dos_sscanf(pstSCB->stAccessCode.szDialCache, "*%*[^*]*%[^#]s", pszEmpNum) != 1)
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "POTS, format error : %s", pstSCB->stAccessCode.szDialCache);

        return DOS_FAIL;
    }

    /* 根据工号找到坐席 */
    pstAgentNode = sc_agent_get_by_emp_num(pstSCB->ulCustomerID, pszEmpNum);
    if (DOS_ADDR_INVALID(pstAgentNode))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "POTS, Can not find agent. customer id(%u), empNum(%s)", pstSCB->ulCustomerID, pszEmpNum);

        /* 判断一下是不是分机号 */
        if (!sc_sip_account_be_is_exit(pstSCB->ulCustomerID, pszEmpNum))
        {
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "POTS, empNum(%s) is not sip", pszEmpNum);
            return DOS_FAIL;
        }
    }
    else
    {
        /* 判断坐席的状态 */
        if (!SC_ACD_SITE_IS_USEABLE(pstAgentNode->pstAgentInfo))
        {
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "The agent is not useable.(Agent %u)", pstAgentNode->pstAgentInfo->ulAgentID);

            return DOS_FAIL;
        }
    }

    if (pstSCB->stTransfer.stSCBTag.bValid)
    {
        /* 说明已经存在一个转接业务，将转接业务拷贝到 stTransfer 中，初始化 pstSCB->stTransfer */
        stTransfer = pstSCB->stTransfer;

        pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_IDEL;
        pstSCB->stTransfer.ulType = U32_BUTT;
        pstSCB->stTransfer.ulSubLegNo = U32_BUTT;
        pstSCB->stTransfer.ulPublishLegNo = U32_BUTT;
        pstSCB->stTransfer.ulNotifyLegNo = U32_BUTT;
        pstSCB->stTransfer.ulSubAgentID = 0;
        pstSCB->stTransfer.ulPublishAgentID = 0;
        pstSCB->stTransfer.ulNotifyAgentID = 0;
    }
    else
    {
        stTransfer = pstSCB->stTransfer;
        stTransfer.stSCBTag.bValid = DOS_FALSE;
    }

    if (DOS_ADDR_VALID(pstAgentNode))
    {
        pstSCB->stTransfer.ulPublishType = SC_TRANSFER_PUBLISH_AGENT;
        pstSCB->stTransfer.ulPublishAgentID = pstAgentNode->pstAgentInfo->ulAgentID;
    }
    else
    {
        pstSCB->stTransfer.ulPublishType = SC_TRANSFER_PUBLISH_SIP;
        dos_strncpy(pstSCB->stTransfer.szSipNum, pszEmpNum, SC_NUM_LENGTH-1);
        pstSCB->stTransfer.szSipNum[SC_NUM_LENGTH-1] = '\0';
    }

    pstSCB->stTransfer.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stTransfer.ulNotifyLegNo = pstLegCB->ulCBNo;
    pstSCB->stTransfer.ulType = pstSCB->stAccessCode.ulSrvType;

    pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_IDEL;

    /* 从之前的业务中拷贝信息到转接业务中 */
    if (pstSCB->stCall.stSCBTag.bValid)
    {
        if (pstSCB->stCall.stSCBTag.usStatus == SC_CALL_ACTIVE)
        {
            if (pstLegCB->ulCBNo == pstSCB->stCall.ulCallingLegNo)
            {
                pstSCB->stTransfer.ulSubLegNo = pstSCB->stCall.ulCalleeLegNo;
                if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                    && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
                {
                    pstSCB->stTransfer.ulSubAgentID = pstSCB->stCall.pstAgentCallee->pstAgentInfo->ulAgentID;
                }

                if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                    && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
                {
                    pstSCB->stTransfer.ulNotifyAgentID = pstSCB->stCall.pstAgentCalling->pstAgentInfo->ulAgentID;
                }
            }
            else
            {
                pstSCB->stTransfer.ulSubLegNo = pstSCB->stCall.ulCallingLegNo;
                if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                    && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
                {
                    pstSCB->stTransfer.ulSubAgentID = pstSCB->stCall.pstAgentCalling->pstAgentInfo->ulAgentID;
                }

                if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                    && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
                {
                    pstSCB->stTransfer.ulNotifyAgentID = pstSCB->stCall.pstAgentCallee->pstAgentInfo->ulAgentID;
                }
            }
        }
    }
    else if (pstSCB->stPreviewCall.stSCBTag.bValid)
    {
        if (pstSCB->stPreviewCall.stSCBTag.usStatus == SC_PREVIEW_CALL_CONNECTED)
        {
            if (pstLegCB->ulCBNo == pstSCB->stPreviewCall.ulCallingLegNo)
            {
                pstSCB->stTransfer.ulSubLegNo = pstSCB->stPreviewCall.ulCalleeLegNo;
                pstSCB->stTransfer.ulNotifyAgentID = pstSCB->stPreviewCall.ulAgentID;
            }
            else
            {
                pstSCB->stTransfer.ulSubLegNo = pstSCB->stPreviewCall.ulCallingLegNo;
                pstSCB->stTransfer.ulSubAgentID = pstSCB->stPreviewCall.ulAgentID;
            }
        }
    }
    else if (pstSCB->stAutoCall.stSCBTag.bValid)
    {
        if (pstSCB->stAutoCall.stSCBTag.usStatus == SC_AUTO_CALL_CONNECTED)
        {
            if (pstLegCB->ulCBNo == pstSCB->stAutoCall.ulCallingLegNo)
            {
                pstSCB->stTransfer.ulSubLegNo = pstSCB->stAutoCall.ulCalleeLegNo;
                pstSCB->stTransfer.ulSubAgentID = pstSCB->stAutoCall.ulAgentID;
            }
            else
            {
                pstSCB->stTransfer.ulSubLegNo = pstSCB->stAutoCall.ulCallingLegNo;
                pstSCB->stTransfer.ulNotifyAgentID = pstSCB->stAutoCall.ulAgentID;
            }
        }
    }
    else if (pstSCB->stCallAgent.stSCBTag.bValid)
    {
        if (pstSCB->stCallAgent.stSCBTag.usStatus == SC_CALL_AGENT_CONNECTED)
        {
            if (pstLegCB->ulCBNo == pstSCB->stCallAgent.ulCallingLegNo)
            {
                pstSCB->stTransfer.ulSubLegNo = pstSCB->stCallAgent.ulCalleeLegNo;

                if (DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCalling)
                    && DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo))
                {
                    pstSCB->stTransfer.ulNotifyAgentID = pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo->ulAgentID;
                }

                if (DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCallee)
                    && DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCallee->pstAgentInfo))
                {
                    pstSCB->stTransfer.ulSubAgentID = pstSCB->stCallAgent.pstAgentCallee->pstAgentInfo->ulAgentID;
                }
            }
            else
            {
                pstSCB->stTransfer.ulSubLegNo = pstSCB->stCallAgent.ulCallingLegNo;

                if (DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCalling)
                    && DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo))
                {
                    pstSCB->stTransfer.ulSubAgentID = pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo->ulAgentID;
                }

                if (DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCallee)
                    && DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCallee->pstAgentInfo))
                {
                    pstSCB->stTransfer.ulNotifyAgentID = pstSCB->stCallAgent.pstAgentCallee->pstAgentInfo->ulAgentID;
                }
            }
        }
    }
    else if (stTransfer.stSCBTag.bValid)
    {
        if (pstSCB->stTransfer.stSCBTag.usStatus == SC_TRANSFER_FINISHED)
        {
            /* 之前存在的是转接业务 */
            if (pstLegCB->ulCBNo == stTransfer.ulPublishLegNo)
            {
                pstSCB->stTransfer.ulSubLegNo = stTransfer.ulSubLegNo;
                pstSCB->stTransfer.ulSubAgentID = stTransfer.ulSubAgentID;
                pstSCB->stTransfer.ulNotifyAgentID = stTransfer.ulPublishAgentID;
            }
            else
            {
                pstSCB->stTransfer.ulSubLegNo = stTransfer.ulPublishLegNo;
                pstSCB->stTransfer.ulSubAgentID = stTransfer.ulPublishAgentID;
                pstSCB->stTransfer.ulNotifyAgentID = stTransfer.ulSubAgentID;
            }
        }
    }
    else if (pstSCB->stAutoPreview.stSCBTag.bValid)
    {
        if (pstSCB->stAutoPreview.stSCBTag.usStatus == SC_AUTO_PREVIEW_CONNECTED)
        {
            if (pstLegCB->ulCBNo == pstSCB->stAutoPreview.ulCallingLegNo)
            {
                pstSCB->stTransfer.ulSubLegNo = pstSCB->stAutoPreview.ulCalleeLegNo;
                pstSCB->stTransfer.ulNotifyAgentID = pstSCB->stAutoPreview.ulAgentID;
            }
            else
            {
                pstSCB->stTransfer.ulSubLegNo = pstSCB->stAutoPreview.ulCallingLegNo;
                pstSCB->stTransfer.ulSubAgentID = pstSCB->stAutoPreview.ulAgentID;
            }
        }
    }

    if (pstSCB->stTransfer.ulSubLegNo == U32_BUTT)
    {
        /* 没有找到 */
        DOS_ASSERT(0);
        goto proc_fail;
    }

    if (pstSCB->stTransfer.ulPublishType == SC_TRANSFER_PUBLISH_AGENT)
    {
        /* 判断坐席是否是长签 */
        if (pstAgentNode->pstAgentInfo->bConnected
            && pstAgentNode->pstAgentInfo->ulLegNo < SC_LEG_CB_SIZE)
        {
            pstPublishLeg = sc_lcb_get(pstAgentNode->pstAgentInfo->ulLegNo);
            if (DOS_ADDR_VALID(pstPublishLeg) && pstPublishLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签 */
                pstSCB->stTransfer.ulPublishLegNo = pstAgentNode->pstAgentInfo->ulLegNo;
            }
        }

        if (pstSCB->stTransfer.ulPublishLegNo == U32_BUTT)
        {
            pstPublishLeg = sc_lcb_alloc();
            if (DOS_ADDR_INVALID(pstPublishLeg))
            {
                DOS_ASSERT(0);
                goto proc_fail;
            }

            /* 主被叫号码 */
            pstPublishLeg->stCall.bValid = DOS_TRUE;
            pstPublishLeg->ulSCBNo = pstSCB->ulSCBNo;
            /* 主被叫号码 */
            ulRet = sc_caller_setting_select_number(pstAgentNode->pstAgentInfo->ulCustomerID, pstAgentNode->pstAgentInfo->ulAgentID, SC_SRC_CALLER_TYPE_AGENT, pstPublishLeg->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
            if (ulRet != DOS_SUCC)
            {
                /* TODO 没有找到主叫号码，错误处理 */
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_HTTP_API), "Agent signin customID(%u) get caller number FAIL by agent(%u)", pstAgentNode->pstAgentInfo->ulCustomerID, pstAgentNode->pstAgentInfo->ulAgentID);
                pstLegCB->ulSCBNo = pstSCB->ulSCBNo;
                sc_lcb_free(pstPublishLeg);

                goto proc_fail;
            }

            switch (pstAgentNode->pstAgentInfo->ucBindType)
            {
                case AGENT_BIND_SIP:
                    dos_snprintf(pstPublishLeg->stCall.stNumInfo.szOriginalCallee, sizeof(pstPublishLeg->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szUserID);
                    pstPublishLeg->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;

                    break;

                case AGENT_BIND_TELE:
                    dos_snprintf(pstPublishLeg->stCall.stNumInfo.szOriginalCallee, sizeof(pstPublishLeg->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szTelePhone);
                    pstPublishLeg->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
                    sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);

                    break;

                case AGENT_BIND_MOBILE:
                    dos_snprintf(pstPublishLeg->stCall.stNumInfo.szOriginalCallee, sizeof(pstPublishLeg->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szMobile);
                    pstPublishLeg->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
                    sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                    break;

                case AGENT_BIND_TT_NUMBER:
                    dos_snprintf(pstPublishLeg->stCall.stNumInfo.szOriginalCallee, sizeof(pstPublishLeg->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szTTNumber);
                    pstPublishLeg->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
                    break;

                default:
                    break;
            }
            pstPublishLeg->stCall.stNumInfo.szOriginalCallee[sizeof(pstPublishLeg->stCall.stNumInfo.szOriginalCallee)-1] = '\0';

            pstSCB->stTransfer.ulPublishLegNo = pstPublishLeg->ulCBNo;
        }
    }
    else
    {
        /* 转接sip分机 */
        pstPublishLeg = sc_lcb_alloc();
        if (DOS_ADDR_INVALID(pstPublishLeg))
        {
            DOS_ASSERT(0);
            goto proc_fail;
        }

        /* 主被叫号码 */
        pstPublishLeg->stCall.bValid = DOS_TRUE;
        pstPublishLeg->ulSCBNo = pstSCB->ulSCBNo;
        /* 主被叫号码 */
        ulRet = sc_caller_setting_select_number(pstSCB->ulCustomerID, 0, SC_SRC_CALLER_TYPE_ALL, pstPublishLeg->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
        if (ulRet != DOS_SUCC)
        {
            /* TODO 没有找到主叫号码，错误处理 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_HTTP_API), "Agent signin customID(%u) get caller number FAIL.", pstSCB->ulSCBNo);
            pstLegCB->ulSCBNo = pstSCB->ulSCBNo;
            sc_lcb_free(pstPublishLeg);

            goto proc_fail;
        }

        dos_snprintf(pstPublishLeg->stCall.stNumInfo.szOriginalCallee, sizeof(pstPublishLeg->stCall.stNumInfo.szOriginalCallee), pstSCB->stTransfer.szSipNum);
        pstPublishLeg->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;

        pstPublishLeg->stCall.stNumInfo.szOriginalCallee[sizeof(pstPublishLeg->stCall.stNumInfo.szOriginalCallee)-1] = '\0';

        pstSCB->stTransfer.ulPublishLegNo = pstPublishLeg->ulCBNo;
    }

    /* 业务切换，直接用 转接业务 替换掉第一个业务 */
    pstSCB->pstServiceList[0] = &pstSCB->stTransfer.stSCBTag;

    sc_scb_set_service(pstSCB, BS_SERV_CALL_TRANSFER);
    pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_AUTH;
    ulRet = sc_send_usr_auth2bs(pstSCB, pstLegCB);
    if (ulRet != DOS_SUCC)
    {
        /* 错误处理 */
        goto proc_fail;
    }

    return DOS_SUCC;

proc_fail:

    return DOS_FAIL;
}

U32 sc_access_agent_proc(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    SC_AGENT_NODE_ST    *pstAgent = NULL;
    U32                 ulResult  = DOS_FAIL;
    BOOL                bPlayRes  = DOS_TRUE;
    U32                 ulKey;

    if (pstSCB->stAccessCode.ulAgentID == 0)
    {
        /* 根据主叫号码获得坐席 */
        if (SC_LEG_PEER_INBOUND_INTERNAL == pstLegCB->stCall.ucPeerType)
        {
            pstAgent = sc_agent_get_by_sip_acc(pstLegCB->stCall.stNumInfo.szOriginalCalling);
        }
        else
        {
            pstAgent = sc_agent_get_by_tt_num(pstLegCB->stCall.stNumInfo.szOriginalCalling);
        }
    }
    else
    {
        pstAgent = sc_agent_get_by_id(pstSCB->stAccessCode.ulAgentID);
    }

    if (DOS_ADDR_INVALID(pstAgent) || DOS_ADDR_INVALID(pstAgent->pstAgentInfo))
    {
        /* 没有找到坐席，放提示音 */
        ulResult = DOS_FAIL;
    }
    else
    {
        switch (pstSCB->stAccessCode.ulSrvType)
        {
            case SC_ACCESS_AGENT_ONLINE:
                ulResult = sc_agent_status_update(SC_ACTION_AGENT_LOGIN, pstAgent->pstAgentInfo->ulAgentID, OPERATING_TYPE_PHONE);
                break;
            case SC_ACCESS_AGENT_OFFLINE:
                ulResult = sc_agent_status_update(SC_ACTION_AGENT_LOGOUT, pstAgent->pstAgentInfo->ulAgentID, OPERATING_TYPE_PHONE);
                break;
            case SC_ACCESS_AGENT_EN_QUEUE:
                ulResult = sc_agent_status_update(SC_ACTION_AGENT_IDLE, pstAgent->pstAgentInfo->ulAgentID, OPERATING_TYPE_PHONE);
                break;
            case SC_ACCESS_AGENT_DN_QUEUE:
                ulResult = sc_agent_status_update(SC_ACTION_AGENT_BUSY, pstAgent->pstAgentInfo->ulAgentID, OPERATING_TYPE_PHONE);
                break;
            case SC_ACCESS_AGENT_SIGNIN:
                ulResult = sc_agent_access_set_sigin(pstAgent, pstSCB, pstLegCB);
                if (ulResult == DOS_SUCC)
                {
                    /* answer */
                    sc_req_answer_call(pstSCB->ulSCBNo, pstSCB->stSigin.ulLegNo);
                    bPlayRes = DOS_FALSE;
                }
                break;
            case SC_ACCESS_AGENT_SIGNOUT:
                ulResult = sc_agent_status_update(SC_ACTION_AGENT_SIGNOUT, pstAgent->pstAgentInfo->ulAgentID, OPERATING_TYPE_PHONE);
                break;
            case SC_ACCESS_MARK_CUSTOMER:
                if (pstAgent->pstAgentInfo->szLastCustomerNum[0] == '\0')
                {
                    ulResult = DOS_FAIL;
                    break;
                }

                if (1 != dos_sscanf(pstLegCB->stCall.stNumInfo.szOriginalCallee+dos_strlen(astSCAccessList[SC_ACCESS_MARK_CUSTOMER].szCodeFormat), "%d", &ulKey) && ulKey <= 9)
                {
                    /* 格式错误 */
                    ulResult = DOS_FAIL;
                    break;
                }

                /* 修改主被叫号码 */
                dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee), "*%u#", pstLegCB->stCall.stNumInfo.szOriginalCallee);
                dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCalling), pstAgent->pstAgentInfo->szLastCustomerNum);

                dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstLegCB->stCall.stNumInfo.szRealCallee), pstLegCB->stCall.stNumInfo.szOriginalCallee);
                dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstLegCB->stCall.stNumInfo.szRealCalling), pstLegCB->stCall.stNumInfo.szOriginalCalling);

                dos_snprintf(pstLegCB->stCall.stNumInfo.szCallee, sizeof(pstLegCB->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szOriginalCallee);
                dos_snprintf(pstLegCB->stCall.stNumInfo.szCalling, sizeof(pstLegCB->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szOriginalCalling);

                /* 标记客户 */
                ulResult = sc_agent_marker_update_req(pstSCB->ulCustomerID, pstAgent->pstAgentInfo->ulAgentID, ulKey, pstAgent->pstAgentInfo->szLastCustomerNum);
                break;
        }
    }

    if (DOS_SUCC != ulResult)
    {
        sc_req_play_sound(pstSCB->ulSCBNo, pstLegCB->ulCBNo, SC_SND_SET_FAIL, 1, 0, 0);
    }
    else if (bPlayRes)
    {
        sc_req_play_sound(pstSCB->ulSCBNo, pstLegCB->ulCBNo, SC_SND_SET_SUCC, 1, 0, 0);
    }

    return ulResult;
}

U32 sc_ringing_timeout_proc(SC_SRV_CB *pstSCB)
{
    SC_LEG_CB                       *pstLegCB               = NULL;
    SC_LEG_CB                       *pstCallingLegCB        = NULL;
    SC_LEG_CB                       *pstCalleeLegCB         = NULL;
    SC_SRV_CB                       *pstNewSCB              = NULL;
    SC_AGENT_NODE_ST                *pstAgentCall           = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call agent ringing timeout event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    if (pstSCB->stAutoCall.stSCBTag.usStatus != SC_AUTO_CALL_ALERTING2
        && pstSCB->stAutoCall.stSCBTag.usStatus != SC_AUTO_CALL_EXEC2
        && pstSCB->stAutoCall.stSCBTag.usStatus != SC_AUTO_CALL_PORC2)
    {
        sc_trace_scb(pstSCB, "Status is not SC_AUTO_CALL_ALERTING2/SC_AUTO_CALL_EXEC2, no processing.");
        return DOS_SUCC;
    }

    pstCallingLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstNewSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstNewSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 拷贝 */
    sc_scb_copy(pstNewSCB, pstSCB);

    if (pstSCB->stAutoCall.stSCBTag.usStatus == SC_AUTO_CALL_EXEC2)
    {
        /* 坐席没有呼叫通 */
        pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoCall.ulAgentID);
        if (DOS_ADDR_VALID(pstAgentCall)
             && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
        {
            pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
            sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_CALL);
        }

        pstCalleeLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
        if (DOS_ADDR_VALID(pstCalleeLegCB))
        {
            sc_lcb_free(pstCalleeLegCB);
        }

        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }
    else
    {
        /* 挂断被叫坐席，重新加入到呼叫队列 */
        pstLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
        if (DOS_ADDR_VALID(pstLegCB))
        {
            pstSCB->stAutoCall.ulCallingLegNo = U32_BUTT;
            sc_req_playback_stop(pstSCB->ulSCBNo, pstLegCB->ulCBNo);
            sc_req_hungup(pstSCB->ulSCBNo, pstLegCB->ulCBNo, CC_ERR_SIP_BUSY_HERE);
        }
        else
        {
            sc_scb_free(pstSCB);
            pstSCB = NULL;
        }
    }

    /* 将呼叫重新放回队列 */
    pstCallingLegCB->ulSCBNo = pstNewSCB->ulSCBNo;

    pstNewSCB->stAutoCall.ulCalleeLegNo = U32_BUTT;
    pstNewSCB->stAutoCall.ulAgentID = 0;
    pstNewSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_AFTER_KEY;
    pstNewSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
    pstNewSCB->ulCurrentSrv++;
    pstNewSCB->pstServiceList[pstNewSCB->ulCurrentSrv] = &pstNewSCB->stIncomingQueue.stSCBTag;
    pstNewSCB->stIncomingQueue.ulLegNo = pstNewSCB->stAutoCall.ulCallingLegNo;
    pstNewSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
    pstNewSCB->stIncomingQueue.ulQueueType = SC_SW_FORWARD_AGENT_GROUP;
    pstNewSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
    if (sc_cwq_add_call(pstNewSCB, sc_task_get_agent_queue(pstNewSCB->stAutoCall.ulTcbID), pstCallingLegCB->stCall.stNumInfo.szRealCallee, pstNewSCB->stIncomingQueue.ulQueueType, DOS_TRUE) != DOS_SUCC)
    {
        /* 加入队列失败 */
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    else
    {
        /* 放音提示客户等待 */
        pstNewSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
        sc_req_play_sound(pstNewSCB->ulSCBNo, pstNewSCB->stIncomingQueue.ulLegNo, SC_SND_CALL_QUEUE_WAIT, 1, 0, 0);
    }

    return DOS_SUCC;
}


U32 sc_call_access_code(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB, S8 *szNum, BOOL bIsSecondDial)
{
    U32     i                        = 0;
    U32     ulKey                    = U32_BUTT;
    S8      szDealNum[SC_NUM_LENGTH] = {0,};

    if (DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstCallingLegCB)
        || DOS_ADDR_INVALID(szNum))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (!bIsSecondDial)
    {
        /* 根据分机号，获取customer */
        if (SC_LEG_PEER_INBOUND_INTERNAL == pstCallingLegCB->stCall.ucPeerType)
        {
            pstSCB->ulCustomerID = sc_sip_account_get_customer(pstCallingLegCB->stCall.stNumInfo.szOriginalCalling, NULL);
        }
        else if (SC_LEG_PEER_INBOUND == pstCallingLegCB->stCall.ucPeerType)
        {
            /* external可能是TT号呼入哦 */
            pstSCB->ulCustomerID = sc_did_get_custom(pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
        }
    }

    sc_trace_scb(pstSCB, "Access code: %s. customer: %u, secondDial: %u", szNum, pstSCB->ulCustomerID, bIsSecondDial);
    sc_log_digest_print_only(pstSCB, "Access code: %s. secondDial: %u", szNum, bIsSecondDial);

    if (pstSCB->ulCustomerID == U32_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (!pstSCB->stAccessCode.stSCBTag.bValid )
    {
        pstSCB->stCall.stSCBTag.usStatus = SC_CALL_ACTIVE;
        pstSCB->ulCurrentSrv++;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;
        pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
        pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
        pstSCB->stAccessCode.ulLegNo = pstCallingLegCB->ulCBNo;
    }
    else
    {
        pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
    }

    dos_strncpy(szDealNum, szNum, SC_NUM_LENGTH-1);
    if (szDealNum[dos_strlen(szDealNum)] == '#'
        || szDealNum[dos_strlen(szDealNum)] == '*')
    {
        szDealNum[dos_strlen(szDealNum)] = '\0';
    }

    /* 判断业务 */
    for (i=0; i<SC_ACCESS_BUTT; i++)
    {
        if (!astSCAccessList[i].bValid)
        {
            continue;
        }

        if ((bIsSecondDial && !astSCAccessList[i].bSupportSecondDial)
            || (!bIsSecondDial && !astSCAccessList[i].bSupportDirectDial))
        {
            continue;
        }

        if (astSCAccessList[i].bExactMatch)
        {
            if (dos_strcmp(szDealNum, astSCAccessList[i].szCodeFormat) == 0)
            {
                break;
            }
        }
        else
        {
            if (dos_strncmp(szDealNum, astSCAccessList[i].szCodeFormat, dos_strlen(astSCAccessList[i].szCodeFormat)) == 0)
            {
                break;
            }
        }
    }

    if (i >= SC_ACCESS_BUTT)
    {
        /* 判断一下是不是客户标记 */
        if (bIsSecondDial
            && szDealNum[0] == '*'
            && szDealNum[2] == '\0'
            && 1 == dos_sscanf(szDealNum+1, "%u", &ulKey)
            && ulKey <= 9)
        {
            if (sc_access_mark_customer(pstSCB, pstCallingLegCB, ulKey) != DOS_SUCC)
            {
                pstSCB->stAccessCode.stSCBTag.bWaitingExit = DOS_TRUE;
            }

            return DOS_SUCC;
        }

        /* 没有找个匹配的, 提示操作失败 */
        pstSCB->stAccessCode.stSCBTag.bWaitingExit = DOS_TRUE;
#if 0
        pstSCB->stAccessCode.bIsSecondDial = bIsSecondDial;
        pstSCB->stAccessCode.ulSrvType = SC_ACCESS_BUTT;
        pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_ACTIVE;
        sc_req_play_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, SC_SND_SET_FAIL, 1, 0, 0);
#endif
        return DOS_FAIL;
    }

    pstSCB->stAccessCode.bIsSecondDial = bIsSecondDial;
    pstSCB->stAccessCode.ulSrvType = astSCAccessList[i].ulType;
    pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_ACTIVE;
    if (DOS_ADDR_VALID(astSCAccessList[i].fn_init))
    {
        astSCAccessList[i].fn_init(pstSCB, pstCallingLegCB);
    }

    return DOS_SUCC;
}


/**
 * 基本呼叫业务中呼叫建立消息
 *
 * @param SC_MSG_TAG_ST *pstMsg
 * @param SC_SRV_CB *pstSCB
 *
 * @return 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_call_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB   *pstCallingLegCB = NULL;
    SC_MSG_EVT_CALL_ST  *pstCallSetup;
    U32         ulCallSrc        = U32_BUTT;
    U32         ulCallDst        = U32_BUTT;
    U32         ulRet            = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallSetup = (SC_MSG_EVT_CALL_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Processing the call setup msg. status: %u", pstSCB->stCall.stSCBTag.usStatus);

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
            break;

        case SC_CALL_PROC:
            /*  结果地方就是呼入 */
            pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstCallingLegCB))
            {
                sc_trace_scb(pstSCB, "Cannot find the LCB. (%s)", pstSCB->stCall.ulCallingLegNo);

                goto proc_fail;
            }

            sc_log_digest_print_only(pstSCB, "Auto Call. calling Num: %s, callee Num: %s", pstCallingLegCB->stCall.stNumInfo.szOriginalCalling, pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);

            /* 如果是接入码业务需要发起接入码业务 */
            if ('*' == pstCallingLegCB->stCall.stNumInfo.szOriginalCallee[0])
            {
                pstCallingLegCB->stCall.stTimeInfo.ulAnswerTime = pstCallingLegCB->stCall.stTimeInfo.ulStartTime;
                if (SC_LEG_PEER_INBOUND_INTERNAL == pstCallingLegCB->stCall.ucPeerType
                    || SC_LEG_PEER_INBOUND_TT == pstCallingLegCB->stCall.ucPeerType)
                {
                    sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
                }
                else
                {
                    sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);
                }

                sc_log_digest_print_only(pstSCB, "Call access: %s", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
                ulRet = sc_call_access_code(pstSCB, pstCallingLegCB, pstCallingLegCB->stCall.stNumInfo.szOriginalCallee, DOS_FALSE);
                goto proc_finished;
            }

            ulCallSrc = sc_leg_get_source(pstSCB, pstCallingLegCB);
            /* 一定要通过主叫把客户信息分析出来，否则不能在进行了  */
            if (U32_BUTT == pstSCB->ulCustomerID)
            {
                sc_trace_scb(pstSCB, "Get source is %d", ulCallSrc);
                ulRet = sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_SC_NO_SERV_RIGHTS);
                goto proc_finished;
            }

            /* 判断 客户/坐席/主被叫号码 是否需要跟踪 */
            if (!pstSCB->bTrace)
            {
                pstSCB->bTrace = sc_customer_get_trace(pstSCB->ulCustomerID);
                if (!pstSCB->bTrace
                    && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                    && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
                {
                    pstSCB->bTrace = pstSCB->stCall.pstAgentCalling->pstAgentInfo->bTraceON;
                }

                if (!pstSCB->bTrace)
                {
                    pstSCB->bTrace = sc_trace_check_caller(pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);
                }

                if (!pstSCB->bTrace)
                {
                    pstSCB->bTrace = sc_trace_check_callee(pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
                }
            }

            ulCallDst = sc_leg_get_destination(pstSCB, pstCallingLegCB);

            sc_trace_scb(pstSCB, "Get call source and dest. Customer: %u, Source: %d, Dest: %d", pstSCB->ulCustomerID, ulCallSrc, ulCallDst);

            /* 保存到scb中，后面会用到 */
            pstSCB->stCall.ulCallSrc = ulCallSrc;
            pstSCB->stCall.ulCallDst = ulCallDst;

            /* 出局呼叫 */
            if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_PSTN == ulCallDst)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_WARNING, SC_MOD_EVENT, SC_LOG_DISIST), "Call out");
                /* 禁止呼叫国际长途 */
                if (pstCallingLegCB->stCall.stNumInfo.szOriginalCallee[0] == '\0'
                    || (pstCallingLegCB->stCall.stNumInfo.szOriginalCallee[0] == '0'
                        && pstCallingLegCB->stCall.stNumInfo.szOriginalCallee[1] == '0'))
                {
                    /* 禁止呼叫 */
                    sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Callee is %s. Not alloc call", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
                    sc_req_hungup(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, CC_ERR_SC_CALLEE_NUMBER_ILLEGAL);
                    break;
                }

                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_AUTH;

                sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);

                ulRet = sc_send_usr_auth2bs(pstSCB, pstCallingLegCB);
                goto proc_finished;
            }
            /* 呼入 */
            else if (SC_DIRECTION_PSTN == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_WARNING, SC_MOD_EVENT, SC_LOG_DISIST), "Call in");
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_AUTH;

                sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);

                ulRet = sc_send_usr_auth2bs(pstSCB, pstCallingLegCB);
                goto proc_finished;
            }
            /* 内部呼叫 */
            else if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_WARNING, SC_MOD_EVENT, SC_LOG_DISIST), "Call internal");
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

                sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);

                ulRet = sc_internal_call_process(pstSCB, pstCallingLegCB);
                goto proc_finished;
            }
            else
            {
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_RELEASE;

                ulRet = sc_req_hungup_with_sound(U32_BUTT, pstSCB->stCall.ulCallingLegNo, CC_ERR_SC_NO_SERV_RIGHTS);
                goto proc_finished;
            }
            break;

        case SC_CALL_AUTH:
            break;
        case SC_CALL_EXEC:
            /* 这个地方应该是被叫的LEG创建了, 更新一下被叫的LEG */
            pstSCB->stCall.ulCalleeLegNo = pstCallSetup->ulLegNo;
            break;

        case SC_CALL_ALERTING:
            break;

        case SC_CALL_ACTIVE:
        case SC_CALL_PROCESS:
        case SC_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            ulRet = DOS_FAIL;
            sc_trace_scb(pstSCB, "Invalid status.%u", pstSCB->stCall.stSCBTag.usStatus);
            break;
    }

proc_finished:
    return DOS_SUCC;

proc_fail:
    sc_scb_free(pstSCB);
    pstSCB = NULL;
    return DOS_FAIL;
}

U32 sc_call_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB = NULL;
    SC_LEG_CB                  *pstCalleeLegCB = NULL;
    U32                         ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;
    pstLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        pstSCB->stCall.bIsError = DOS_TRUE;
        sc_trace_scb(pstSCB, "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        sc_log_digest_print_only(pstSCB, "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* 注意通过偏移量，找到CC统一定义的错误码 */
        sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_BS_HEAD + pstAuthRsp->stMsgTag.usInterErr);
        return DOS_SUCC;
    }

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_AUTH:
            if (SC_DIRECTION_PSTN == pstSCB->stCall.ulCallSrc && SC_DIRECTION_SIP == pstSCB->stCall.ulCallDst)
            {
                /* 入局呼叫 */
                ulRet = sc_incoming_call_proc(pstSCB, pstLegCB);
            }
            else
            {
                if (pstAuthRsp->ucBalanceWarning)
                {
                    /* 余额告警 */
                    pstSCB->stBalanceWarning.stSCBTag.bValid = DOS_TRUE;
                    return sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, SC_SND_LOW_BALANCE, 1, 0, 0);
                }

                ulRet = sc_outgoing_call_process(pstSCB, pstLegCB);
            }
            break;

         case SC_CALL_AUTH2:
            /* 呼叫被叫 */
            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

            pstCalleeLegCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCalleeLegCB))
            {
                return DOS_FAIL;
            }
            ulRet = sc_make_call2pstn(pstSCB, pstCalleeLegCB);
            break;

         default:
            break;
    }

    return ulRet;
}


U32 sc_call_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_RINGING_ST *pstEvent         = NULL;
    SC_LEG_CB             *pstCalleeLegCB   = NULL;
    SC_LEG_CB             *pstCallingLegCB  = NULL;
    U32                   lRes              = DOS_FAIL;
    BOOL                  bIsConnected      = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEvent = (SC_MSG_EVT_RINGING_ST *)pstMsg;

    sc_trace_scb(pstSCB, "process alerting msg. calling leg: %u, callee leg: %u, status : %u"
                        , pstSCB->stCall.ulCallingLegNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.stSCBTag.usStatus);

    sc_log_digest_print_only(pstSCB, "process alerting msg. calling leg: %u, callee leg: %u, status: %u. with media: %u"
                        , pstSCB->stCall.ulCallingLegNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.stSCBTag.usStatus, pstEvent->ulWithMedia);

    pstCalleeLegCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
    pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCalleeLegCB) || DOS_ADDR_INVALID(pstCallingLegCB))
    {
        sc_trace_scb(pstSCB, "alerting with only one leg.");
        return DOS_SUCC;
    }

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
        case SC_CALL_PROC:
        case SC_CALL_AUTH:
        case SC_CALL_AUTH2:
        case SC_CALL_EXEC:
        case SC_CALL_ALERTING:
            if (pstEvent->ulLegNo == pstSCB->stCall.ulCalleeLegNo)
            {
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_ALERTING;

                if (pstSCB->stBalanceWarning.stSCBTag.bValid)
                {
                    bIsConnected = DOS_TRUE;
                }

                if (sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, bIsConnected, pstEvent->ulWithMedia) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Send ringback request fail.");
                }

                if (pstEvent->ulWithMedia)
                {
                    pstCalleeLegCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
                    if (DOS_ADDR_VALID(pstCalleeLegCB))
                    {
                        pstCalleeLegCB->stCall.bEarlyMedia = DOS_TRUE;
                        if (SC_DIRECTION_PSTN == pstSCB->stCall.ulCallSrc && SC_DIRECTION_SIP == pstSCB->stCall.ulCallDst)
                        {
                            /* 入局呼叫 */
                            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
                            {
                                sc_agent_serv_status_update(pstSCB->stCall.pstAgentCallee->pstAgentInfo, SC_ACD_SERV_CALL_IN, SC_SRV_CALL);
                            }
                        }
                        else if (SC_DIRECTION_SIP == pstSCB->stCall.ulCallSrc && SC_DIRECTION_PSTN == pstSCB->stCall.ulCallDst)
                        {
                            /* 出局呼叫 */
                            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
                            {
                                sc_agent_serv_status_update(pstSCB->stCall.pstAgentCalling->pstAgentInfo, SC_ACD_SERV_CALL_OUT, SC_SRV_CALL);
                            }
                        }

                        if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo) != DOS_SUCC)
                        {
                            sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                            goto proc_fail;
                        }
                    }
                }

                if (pstSCB->stCall.bIsRingTimer
                    && DOS_ADDR_INVALID(pstSCB->stCall.stTmrHandle))
                {
                    /* 开启定时器 */
                    sc_trace_scb(pstSCB, "%s", "Start ringting timer.");
                    lRes = dos_tmr_start(&pstSCB->stCall.stTmrHandle, SC_AGENT_RINGING_TIMEOUT, sc_agent_ringing_timeout_callback, (U64)pstEvent->ulLegNo, TIMER_NORMAL_NO_LOOP);
                    if (lRes < 0)
                    {
                        DOS_ASSERT(0);
                        pstSCB->stCall.stTmrHandle = NULL;
                    }
                }
            }

            break;

        case SC_CALL_ACTIVE:
        case SC_CALL_PROCESS:
        case SC_CALL_RELEASE:
            break;
    }

    return DOS_SUCC;

proc_fail:
    /* 这个地方不用挂断呼叫，早期媒体的事情，不影响的 */
    return DOS_FAIL;
}

U32 sc_call_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB             *pstCalleeLegCB = NULL;
    SC_MSG_EVT_ANSWER_ST  *pstEvtAnswer   = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
        case SC_CALL_PROC:
        case SC_CALL_AUTH:
        case SC_CALL_AUTH2:
        case SC_CALL_EXEC:
        case SC_CALL_ALERTING:
            pstEvtAnswer = (SC_MSG_EVT_ANSWER_ST *)pstMsg;

            pstSCB->stCall.ulCalleeLegNo = pstEvtAnswer->ulLegNo;

            /* 应答主叫 */
            sc_req_answer_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo);

            /* 连接主被叫 */
            pstCalleeLegCB = sc_lcb_get(pstEvtAnswer->ulLegNo);
            if (DOS_ADDR_VALID(pstCalleeLegCB)
                && !pstCalleeLegCB->stCall.bEarlyMedia)
            {
                if (SC_DIRECTION_PSTN == pstSCB->stCall.ulCallSrc && SC_DIRECTION_SIP == pstSCB->stCall.ulCallDst)
                {
                    /* 入局呼叫 */
                    if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                        && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
                    {
                        sc_agent_serv_status_update(pstSCB->stCall.pstAgentCallee->pstAgentInfo, SC_ACD_SERV_CALL_IN, SC_SRV_CALL);
                    }
                }
                else if (SC_DIRECTION_SIP == pstSCB->stCall.ulCallSrc && SC_DIRECTION_PSTN == pstSCB->stCall.ulCallDst)
                {
                    /* 出局呼叫 */
                    if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                        && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
                    {
                        sc_agent_serv_status_update(pstSCB->stCall.pstAgentCalling->pstAgentInfo, SC_ACD_SERV_CALL_OUT, SC_SRV_CALL);
                    }
                }
                else
                {
                    /* 内部呼叫 */
                    if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                        && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
                    {
                        sc_agent_serv_status_update(pstSCB->stCall.pstAgentCallee->pstAgentInfo, SC_ACD_SERV_CALL_IN, SC_SRV_CALL);
                    }

                    if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                        && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
                    {
                        sc_agent_serv_status_update(pstSCB->stCall.pstAgentCalling->pstAgentInfo, SC_ACD_SERV_CALL_OUT, SC_SRV_CALL);
                    }
                }

                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_SC_SYSTEM_ABNORMAL);
                    goto proc_fail;
                }

            }

            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_ACTIVE;
            break;

        case SC_CALL_ACTIVE:
        case SC_CALL_PROCESS:
        case SC_CALL_RELEASE:
            sc_trace_scb(pstSCB, "Calling has been answered");
            break;
    }

    return DOS_SUCC;
proc_fail:

    return DOS_FAIL;
}

U32 sc_call_bridge(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* DO nothing */
    return DOS_SUCC;
}

U32 sc_call_unbridge(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* DO nothing */
    return DOS_SUCC;
}

U32 sc_call_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HUNGUP_ST *pstHungup         = NULL;
    SC_LEG_CB            *pstCallee         = NULL;
    SC_LEG_CB            *pstCalling        = NULL;
    SC_LEG_CB            *pstHungupLeg      = NULL;
    SC_LEG_CB            *pstOtherLeg       = NULL;
    SC_AGENT_NODE_ST     *pstAgentCall      = NULL;
    SC_AGENT_NODE_ST     *pstAgentHungup    = NULL;
    S32                  i                  = 0;
    S32                  lRes               = DOS_FAIL;
    U32                  ulReleasePart;
    U32                  ulErrCode;

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHungup) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulErrCode = pstHungup->ulErrCode;
    sc_trace_scb(pstSCB, "Leg %u has hungup. Legs:%u-%u, status : %u", pstHungup->ulLegNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo, pstSCB->stCall.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Leg %u has hungup. Legs:%u-%u, status : %u", pstHungup->ulLegNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo, pstSCB->stCall.stSCBTag.usStatus);

    if (pstHungup->ulLegNo == pstSCB->stCall.ulCallingLegNo)
    {
        ulReleasePart = SC_CALLING;
    }
    else
    {
        ulReleasePart = SC_CALLEE;
    }

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
        case SC_CALL_PROC:
        case SC_CALL_AUTH:
            /* 还没有呼叫被叫, 生成话单 */
            if (pstSCB->stCall.bIsRingTimer
                && pstHungup->ulLegNo != pstSCB->stCall.ulCalleeLegNo
                && pstHungup->ulLegNo != pstSCB->stCall.ulCallingLegNo)
            {
                /* 坐席振铃超时，挂断，呼叫另一个坐席 */
                /* 修改坐席状态 */
                if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                    && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstSCB->stCall.pstAgentCallee->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL);
                }

                pstHungupLeg = sc_lcb_get(pstHungup->ulLegNo);
                if (DOS_ADDR_VALID(pstHungupLeg))
                {
                    sc_lcb_free(pstHungupLeg);
                }

                break;
            }

            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstSCB->stCall.pstAgentCalling->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL);
            }

            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                pstCalling->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalling, NULL, ulReleasePart);
                sc_lcb_free(pstCalling);
                pstCalling = NULL;
            }
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_CALL_EXEC:
            /* 这个地方应该是被叫异常了，清理资源，修改坐席状态 */
            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                pstCalling->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalling, NULL, ulReleasePart);

                sc_lcb_free(pstCalling);
                pstCalling = NULL;
            }

            pstCallee = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCallee))
            {
                sc_lcb_free(pstCallee);
                pstCallee = NULL;
            }

            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstSCB->stCall.pstAgentCalling->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL);
            }

            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstSCB->stCall.pstAgentCallee->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL);
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_CALL_ALERTING:
        case SC_CALL_TONE:
        case SC_CALL_ACTIVE:
            pstCallee = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstCallee)
                || DOS_ADDR_INVALID(pstCalling))
            {
                if (DOS_ADDR_VALID(pstCalling))
                {
                    pstCalling->stCall.ulCause = ulErrCode;
                    sc_send_billing_stop2bs(pstSCB, pstCalling, NULL, ulReleasePart);
                    sc_lcb_free(pstCalling);
                    pstCalling = NULL;
                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                    break;
                }
                else if (DOS_ADDR_VALID(pstCallee))
                {
                    /* 有可能是坐席振铃超时，修改坐席状态 */
                    if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee))
                    {
                        sc_agent_serv_status_update(pstSCB->stCall.pstAgentCallee->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL);
                    }

                    sc_lcb_free(pstCallee);
                    pstCallee = NULL;
                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                    break;
                }
            }

            if (pstCallee->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCallee->ulOtherSCBNo);
                pstCallee->ulOtherSCBNo = U32_BUTT;
            }

            if (pstCalling->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCalling->ulOtherSCBNo);
                pstCalling->ulOtherSCBNo = U32_BUTT;
            }

            if (pstSCB->stCall.ulCallingLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCalling;
                pstOtherLeg = pstCallee;
                pstAgentCall = pstSCB->stCall.pstAgentCallee;
                pstAgentHungup = pstSCB->stCall.pstAgentCalling;
            }
            else
            {
                pstHungupLeg = pstCallee;
                pstOtherLeg = pstCalling;
                pstAgentCall = pstSCB->stCall.pstAgentCalling;
                pstAgentHungup = pstSCB->stCall.pstAgentCallee;
            }

            /* 生成话单 */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            if (DOS_ADDR_VALID(pstAgentHungup))
            {
                pstOtherLeg->stCall.stTimeInfo.ulByeTime = pstHungupLeg->stCall.stTimeInfo.ulByeTime;
                /* 尽量用客户的leg生成话单 */
                pstOtherLeg->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstOtherLeg, NULL, ulReleasePart);
            }
            else
            {
                /* 尽量用客户的leg生成话单 */
                pstHungupLeg->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstHungupLeg, NULL, ulReleasePart);
            }

            /* 到这里，说明两个leg都OK */
            /*
              * 需要看看是否长签等问题，如果主/被叫LEG都长签了，需要申请SCB，将被叫LEG挂到新的SCB中
              * 否则，将需要长签的LEG作为当前业务控制块的主叫LEG，挂断另外一条LEG
              * 可能需要处理客户标记
              */
            /* release 时，肯定是有一条leg hungup了，现在的leg需要释放掉，判断另一条是不是坐席长签，如果不是需要挂断 */
            /* 判断是否需要进行，客户标记。1、是客户一端先挂断的(基础呼叫中，客户只能是PSTN，坐席只能是SIP) */
            if ((pstHungupLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND
                || pstHungupLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                && DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && pstAgentCall->pstAgentInfo->ucProcesingTime != 0
                && !pstAgentCall->pstAgentInfo->bMarkCustomer
                && pstOtherLeg->stCall.stTimeInfo.ulAnswerTime != 0)
            {
                /* 客户标记 */
                pstSCB->stMarkCustom.stSCBTag.bValid = DOS_TRUE;
                pstSCB->stMarkCustom.ulLegNo = pstOtherLeg->ulCBNo;
                pstSCB->stMarkCustom.pstAgentCall = pstAgentCall;
                pstSCB->ulCurrentSrv++;
                pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stMarkCustom.stSCBTag;

                if (pstOtherLeg->ulIndSCBNo == U32_BUTT)
                {
                    /* 非长签时，要把坐席对应的leg的结束时间，赋值给开始时间，出标记话单时使用 */
                    pstOtherLeg->stCall.stTimeInfo.ulAnswerTime = pstHungupLeg->stCall.stTimeInfo.ulByeTime;
                    for (i=0; i<SC_MAX_SERVICE_TYPE; i++)
                    {
                        pstSCB->aucServType[i] = 0;
                    }

                    if (pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);
                    }
                    else if(pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                    }
                    else
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
                    }

                    /* 将客户的号码改为主叫号码 */
                    dos_strcpy(pstOtherLeg->stCall.stNumInfo.szOriginalCalling, pstAgentCall->pstAgentInfo->szLastCustomerNum);
                }

                /* 修改坐席状态为 proc，播放 标记背景音 */
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_PROC, SC_SRV_CALL);
                sc_req_play_sound(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, SC_SND_CALL_OVER, 1, 0, 0);
                pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_IDEL;

                /* 开启定时器 */
                lRes = dos_tmr_start(&pstSCB->stMarkCustom.stTmrHandle, pstAgentCall->pstAgentInfo->ucProcesingTime * 1000, sc_agent_mark_custom_callback, (U64)pstOtherLeg->ulCBNo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    pstSCB->stMarkCustom.stTmrHandle = NULL;
                }

                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;

                if (pstSCB->stCall.ulCalleeLegNo == pstHungup->ulLegNo)
                {
                    pstSCB->stCall.ulCalleeLegNo = U32_BUTT;
                }
                else
                {
                    pstSCB->stCall.ulCallingLegNo = U32_BUTT;
                }

                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_PROCESS;

                break;
            }

            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                pstAgentCall->pstAgentInfo->bMarkCustomer = DOS_FALSE;
            }

            /* 不需要客户标记 */
            if (pstSCB->stCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstSCB->stCall.ulCalleeLegNo = U32_BUTT;
            }
            else
            {
                pstSCB->stCall.ulCallingLegNo = U32_BUTT;
            }

            /* 如果是坐席先挂断的，判断坐席是否是长签的 */
            if (DOS_ADDR_VALID(pstAgentHungup)
                && DOS_ADDR_VALID(pstAgentHungup->pstAgentInfo)
                && pstAgentHungup->pstAgentInfo->bNeedConnected)
            {
                pstHungupLeg->ulSCBNo = U32_BUTT;
            }

            /* 修改坐席的状态 */
            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstSCB->stCall.pstAgentCallee->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL);
                if (!pstSCB->stCall.pstAgentCallee->pstAgentInfo->bNeedConnected)
                {
                    pstSCB->stCall.pstAgentCallee->pstAgentInfo->ulLegNo = U32_BUTT;
                }
            }

            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstSCB->stCall.pstAgentCalling->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL);
                if (!pstSCB->stCall.pstAgentCalling->pstAgentInfo->bNeedConnected)
                {
                    pstSCB->stCall.pstAgentCalling->pstAgentInfo->ulLegNo = U32_BUTT;
                }
            }

            if (pstOtherLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签，继续放音 */
                pstOtherLeg->ulSCBNo = U32_BUTT;
                sc_req_play_sound(pstOtherLeg->ulIndSCBNo, pstOtherLeg->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                /* 释放掉 SCB */
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
            else if (pstHungupLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签的坐席挂断了电话，解除关系 */
                if (pstHungupLeg == pstCallee)
                {
                    pstSCB->stCall.ulCalleeLegNo = U32_BUTT;
                }
                else
                {
                    pstSCB->stCall.ulCallingLegNo = U32_BUTT;
                }
                pstHungupLeg->ulSCBNo = U32_BUTT;
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_RELEASE;
            }
            else
            {
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_RELEASE;
            }

            break;

        case SC_CALL_PROCESS:
            /* 修改坐席状态, 两个LEG都挂断了 */
            pstCallee = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCallee))
            {
                sc_lcb_free(pstCallee);
                pstCallee = NULL;
            }

            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                sc_lcb_free(pstCalling);
                pstCalling = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_CALL_RELEASE:
            pstCallee = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCallee))
            {
                sc_lcb_free(pstCallee);
                pstCallee = NULL;
            }

            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                sc_lcb_free(pstCalling);
                pstCalling = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;
        default:
            DOS_ASSERT(0);
            break;
    }

    sc_trace_scb(pstSCB, "Leg %u has hunguped. ", pstHungup->ulLegNo);

    return DOS_SUCC;
}

U32 sc_call_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HOLD_ST   *pstHold        = NULL;
    SC_LEG_CB            *pstLeg         = NULL;

    pstHold = (SC_MSG_EVT_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstHold->bIsHold)
    {
        /* 如果是被HOLD的，需要激活HOLD业务哦 */
        pstSCB->stHold.stSCBTag.bValid = DOS_TRUE;
        pstSCB->stHold.stSCBTag.bWaitingExit = DOS_FALSE;
        pstSCB->stHold.stSCBTag.usStatus = SC_HOLD_ACTIVE;
        pstSCB->stHold.ulCallLegNo = pstHold->ulLegNo;
        pstSCB->stHold.ulHoldCount++;

        pstSCB->ulCurrentSrv++;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stHold.stSCBTag;

        /* 给HOLD方 播放拨号音 */
        /* 给HOLD对方 播放呼叫保持音 */
    }
    else
    {
        /* 如果是被UNHOLD的，已经没有HOLD业务了，单纯处理呼叫就好 */
        pstLeg = sc_lcb_get(pstHold->ulLegNo);
        if (DOS_ADDR_INVALID(pstLeg))
        {
            return DOS_FAIL;
        }

        if (pstLeg->stHold.ulHoldTime != 0
            && pstLeg->stHold.ulUnHoldTime > pstLeg->stHold.ulHoldTime)
        {
            pstSCB->stHold.ulHoldTotalTime += (pstLeg->stHold.ulUnHoldTime - pstLeg->stHold.ulHoldTime);
        }

        pstLeg->stHold.ulUnHoldTime = 0;
        pstLeg->stHold.ulHoldTime = 0;
    }

    return DOS_SUCC;
}

U32 sc_call_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstLCB       =  NULL;
    U32                    ulAgentID    = U32_BUTT;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing the call dtmf msg. status: %u", pstSCB->stCall.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 开启接入码业务 */
    if (pstDTMF->cDTMFVal != '*'
        && pstDTMF->cDTMFVal != '#' )
    {
        /* 第一个字符不是 '*' 或者 '#' 不保存  */
        return DOS_SUCC;
    }

    /* 只有坐席对应的leg执行接入码业务 */
    if (pstDTMF->ulLegNo == pstSCB->stCall.ulCallingLegNo)
    {
        if (DOS_ADDR_INVALID(pstSCB->stCall.pstAgentCalling)
            || DOS_ADDR_INVALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
        {
            return DOS_SUCC;
        }
        ulAgentID = pstSCB->stCall.pstAgentCalling->pstAgentInfo->ulAgentID;
    }

    if (pstDTMF->ulLegNo == pstSCB->stCall.ulCalleeLegNo)
    {
        if (DOS_ADDR_INVALID(pstSCB->stCall.pstAgentCallee)
            || DOS_ADDR_INVALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
        {
            return DOS_SUCC;
        }
        ulAgentID = pstSCB->stCall.pstAgentCallee->pstAgentInfo->ulAgentID;
    }

    pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stAccessCode.szDialCache[0] = '\0';
    pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
    pstSCB->stAccessCode.ulAgentID = ulAgentID;
    pstSCB->ulCurrentSrv++;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;

    return DOS_SUCC;
}

U32 sc_call_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_CMD_RECORD_ST *pstRecord = NULL;
    SC_LEG_CB            *pstLCB    = NULL;

    pstRecord = (SC_MSG_CMD_RECORD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstRecord) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstRecord->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        return DOS_FAIL;
    }

    /* 生成录音话单 */
    sc_send_special_billing_stop2bs(pstSCB, pstLCB, BS_SERV_RECORDING);
    sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);

    return DOS_SUCC;
}

U32 sc_call_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstCallingLegCB = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstCallSetup    = NULL;
    U32                    ulRet            = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallSetup = (SC_MSG_EVT_PLAYBACK_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Processing the call playback stop msg. status: %u", pstSCB->stCall.stSCBTag.usStatus);

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
            break;

        case SC_CALL_PROC:
            break;

        case SC_CALL_AUTH:
            if (!pstSCB->stCall.bIsError)
            {
                ulRet = sc_outgoing_call_process(pstSCB, pstCallingLegCB);
            }
            break;

        case SC_CALL_EXEC:
            break;

        case SC_CALL_TONE:
            /* 坐席长签时，给坐席放提示音结束 */
            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstSCB->stCall.pstAgentCallee->pstAgentInfo, SC_ACD_SERV_CALL_IN, SC_SRV_CALL);
            }

            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstSCB->stCall.pstAgentCalling->pstAgentInfo, SC_ACD_SERV_CALL_OUT, SC_SRV_CALL);
            }

            if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo) != DOS_SUCC)
            {
                sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                return DOS_FAIL;
            }
            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_ACTIVE;
            break;

        case SC_CALL_ALERTING:
            break;
        case SC_CALL_ACTIVE:
            break;
        case SC_CALL_PROCESS:
            break;
        case SC_CALL_RELEASE:
            break;

        default:
            ulRet = DOS_FAIL;
            sc_trace_scb(pstSCB, "Invalid status.%u", pstSCB->stCall.stSCBTag.usStatus);
            break;
    }

    return DOS_SUCC;
}

U32 sc_call_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvtCall = NULL;
    U32                 ulRet               = DOS_SUCC;
    SC_LEG_CB           *pstCallingLegCB    = NULL;
    U32                 ulErrCode           = CC_ERR_NO_REASON;

    pstEvtCall = (SC_MSG_EVT_LEAVE_CALLQUE_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call queue event. status : %u", pstSCB->stCall.stSCBTag.usStatus);

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_AUTH:
            if (SC_LEAVE_CALL_QUE_TIMEOUT == pstMsg->usInterErr)
            {
                /* 加入队列超时 */
                ulErrCode = CC_ERR_SIP_REQUEST_TIMEOUT;
                ulRet = DOS_FAIL;
            }
            else if (SC_LEAVE_CALL_QUE_SUCC == pstMsg->usInterErr)
            {
                if (DOS_ADDR_INVALID(pstEvtCall->pstAgentNode))
                {
                    /* 错误 */
                    ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                    ulRet = DOS_FAIL;
                }
                else
                {
                    pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
                    /* 呼叫坐席 */
                    ulRet = sc_agent_call_by_id(pstSCB, pstCallingLegCB, pstEvtCall->pstAgentNode->pstAgentInfo->ulAgentID, &ulErrCode);
                    if (ulRet == DOS_SUCC
                        && pstSCB->stCall.stSCBTag.usStatus != SC_CALL_TONE)
                    {
                        pstSCB->stCall.bIsRingTimer = DOS_TRUE;
                        sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo);
                        //sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, DOS_TRUE, DOS_FALSE);
                    }
                }
            }
        default:
            break;

    }

    sc_trace_scb(pstSCB, "Proccessed call queue event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstEvtCall->pstAgentNode)
            && DOS_ADDR_VALID(pstEvtCall->pstAgentNode->pstAgentInfo))
        {
            pstEvtCall->pstAgentNode->pstAgentInfo->bSelected = DOS_FALSE;
        }

        sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, ulErrCode);
    }

    return ulRet;

}

U32 sc_call_ringing_timeout(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_RINGING_TIMEOUT_ST   *pstEvtRingingTimeOut   = NULL;
    SC_LEG_CB                       *pstLegCB               = NULL;
    SC_LEG_CB                       *pstCallingLegCB        = NULL;
    SC_SRV_CB                       *pstNewSCB              = NULL;

    pstEvtRingingTimeOut = (SC_MSG_EVT_RINGING_TIMEOUT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtRingingTimeOut) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call agent ringing timeout event. status : %u", pstSCB->stCall.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Processing call agent ringing timeout event. status : %u", pstSCB->stCall.stSCBTag.usStatus);

    if (pstSCB->stCall.stSCBTag.usStatus != SC_CALL_ALERTING)
    {
        sc_trace_scb(pstSCB, "Status is not SC_CALL_ALERTING, no processing.");
        return DOS_SUCC;
    }

    pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstNewSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstNewSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 拷贝 */
    sc_scb_copy(pstNewSCB, pstSCB);

    /* 挂断被叫坐席，重新加入到呼叫队列 */
    pstLegCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
    if (DOS_ADDR_VALID(pstLegCB))
    {
        pstSCB->stCall.ulCallingLegNo = U32_BUTT;
        sc_req_playback_stop(pstSCB->ulSCBNo, pstLegCB->ulCBNo);
        sc_req_hungup(pstSCB->ulSCBNo, pstLegCB->ulCBNo, CC_ERR_SIP_BUSY_HERE);
    }

    /* 将呼叫重新放回队列 */
    pstCallingLegCB->ulSCBNo = pstNewSCB->ulSCBNo;

    pstNewSCB->stCall.ulCalleeLegNo = U32_BUTT;
    pstNewSCB->stCall.pstAgentCallee = NULL;
    pstNewSCB->stCall.stSCBTag.usStatus = SC_CALL_AUTH;
    pstNewSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
    pstNewSCB->ulCurrentSrv++;
    pstNewSCB->pstServiceList[pstNewSCB->ulCurrentSrv] = &pstNewSCB->stIncomingQueue.stSCBTag;
    pstNewSCB->stIncomingQueue.ulLegNo = pstNewSCB->stCall.ulCallingLegNo;
    pstNewSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
    pstNewSCB->stIncomingQueue.ulQueueType = SC_SW_FORWARD_AGENT_GROUP;
    pstNewSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
    if (sc_cwq_add_call(pstNewSCB, pstNewSCB->stCall.ulAgentGrpID, pstCallingLegCB->stCall.stNumInfo.szRealCallee, pstNewSCB->stIncomingQueue.ulQueueType, DOS_TRUE) != DOS_SUCC)
    {
        /* 加入队列失败 */
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    else
    {
        /* 放音提示客户等待 */
        pstNewSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
        sc_req_play_sound(pstNewSCB->ulSCBNo, pstNewSCB->stIncomingQueue.ulLegNo, SC_SND_CALL_QUEUE_WAIT, 1, 0, 0);
    }

    return DOS_SUCC;
}

U32 sc_call_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;
    SC_LEG_CB                   *pstCalleeLegCB     = NULL;
    SC_LEG_CB                   *pstCallingLegCB    = NULL;
    SC_LEG_CB                   *pstRecordLegCB     = NULL;
    SC_MSG_CMD_RECORD_ST        stRecordRsp;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call error event. status : %u", pstSCB->stCall.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Processing call error event. status : %u", pstSCB->stCall.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */
        if (!sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
        {
            return DOS_SUCC;
        }

        pstCalleeLegCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
        if (pstCalleeLegCB->stRecord.bValid)
        {
            pstRecordLegCB = pstCalleeLegCB;
        }
        else
        {
            pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingLegCB) && pstCallingLegCB->stRecord.bValid)
            {
                pstRecordLegCB = pstCallingLegCB;
            }
        }

        if (DOS_ADDR_VALID(pstRecordLegCB))
        {
            stRecordRsp.stMsgTag.ulMsgType = SC_CMD_RECORD;
            stRecordRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stRecordRsp.stMsgTag.usInterErr = 0;
            stRecordRsp.ulSCBNo = pstSCB->ulSCBNo;
            stRecordRsp.ulLegNo = pstRecordLegCB->ulCBNo;

            if (sc_send_cmd_record(&stRecordRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_INFO, SC_MOD_EVENT, SC_LOG_DISIST), "Send record cmd FAIL! SCBNo : %u", pstSCB->ulSCBNo);
            }
        }

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
        case SC_CALL_PROC:
        case SC_CALL_AUTH:
            ulRet = sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, ulErrCode);
            break;
        case SC_CALL_AUTH2:
        case SC_CALL_EXEC:
            /* 呼叫被叫时失败，给主叫放提示音挂断 */
            sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo);
            ulRet = sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, ulErrCode);
            break;
        case SC_CALL_ALERTING:
        case SC_CALL_TONE:
        case SC_CALL_ACTIVE:
            sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo);
            ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, ulErrCode);
            break;
        case SC_CALL_PROCESS:
        case SC_CALL_RELEASE:
            if (pstSCB->stCall.ulCalleeLegNo != U32_BUTT)
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, ulErrCode);
            }
            else
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, ulErrCode);
            }
            break;
        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_preview_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    U32                 ulRet           = DOS_FAIL;
    SC_LEG_CB           *pstLCB         = NULL;
    SC_LEG_CB           *pstCalleeLCB   = NULL;
    SC_AGENT_NODE_ST    *pstAgentNode   = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing Preview auth event. status : %u", pstSCB->stPreviewCall.stSCBTag.usStatus);

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;
    pstLCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_ERROR, SC_MOD_EVENT, SC_LOG_DISIST), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
         /* 注意通过偏移量，找到CC统一定义的错误码。
            需要判断坐席是否是长签，如果是长签的就不能挂断 */
        pstAgentNode = sc_agent_get_by_id(pstSCB->stPreviewCall.ulAgentID);
        if (pstSCB->stPreviewCall.ulCalleeLegNo != U32_BUTT)
        {
            /* 解除关系，坐席置闲 */
            if (DOS_ADDR_INVALID(pstAgentNode)
                || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo)
                || pstLCB->ulIndSCBNo == U32_BUTT)
            {
                /* 没有找到坐席，挂断吧 */
                DOS_ASSERT(0);
                sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCallingLegNo, CC_ERR_BS_HEAD + pstAuthRsp->stMsgTag.usInterErr);
                return DOS_SUCC;
            }

            pstLCB->ulCBNo = U32_BUTT;
            /* 坐席置闲，继续放音 */
            sc_agent_serv_status_update(pstAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_PREVIEW_CALL);
            sc_req_play_sound(pstLCB->ulIndSCBNo, pstLCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);

            pstCalleeLCB = sc_lcb_get(pstSCB->stPreviewCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeLCB))
            {
                sc_lcb_free(pstCalleeLCB);
                pstCalleeLCB = NULL;
            }
            pstSCB->stPreviewCall.ulCalleeLegNo = U32_BUTT;
            sc_scb_free(pstSCB);
            pstSCB = NULL;
        }
        else
        {
            if (DOS_ADDR_VALID(pstAgentNode)
                && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
            {
                /* 没有找到坐席，挂断吧 */
                pstAgentNode->pstAgentInfo->ulLegNo = U32_BUTT;
                sc_agent_serv_status_update(pstAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_PREVIEW_CALL);
            }

            sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_BS_HEAD + pstAuthRsp->stMsgTag.usInterErr);
        }
        return DOS_SUCC;
    }

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_AUTH:
            if (pstSCB->stPreviewCall.ulCalleeLegNo != U32_BUTT)
            {
                /* 存在被叫，说明坐席是长签，直接呼叫客户就行了 */
                pstCalleeLCB = sc_lcb_get(pstSCB->stPreviewCall.ulCalleeLegNo);
                if (DOS_ADDR_INVALID(pstCalleeLCB))
                {
                    /* TODO 错误处理 */
                    break;
                }

                ulRet = sc_make_call2pstn(pstSCB, pstCalleeLCB);
                pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_CONNECTING;

                /* 放回铃音给坐席 */
                sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCallingLegNo);
                sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCallingLegNo, DOS_TRUE, DOS_FALSE);
            }
            else
            {
                switch (pstLCB->stCall.ucPeerType)
                {
                    case SC_LEG_PEER_OUTBOUND:
                        ulRet = sc_make_call2pstn(pstSCB, pstLCB);
                        break;

                    case SC_LEG_PEER_OUTBOUND_TT:
                        ulRet = sc_make_call2eix(pstSCB, pstLCB);
                        break;

                    case SC_LEG_PEER_OUTBOUND_INTERNAL:
                        ulRet = sc_make_call2sip(pstSCB, pstLCB);
                        break;

                    default:
                        sc_trace_scb(pstSCB, "Invalid perr type. %u", pstLCB->stCall.ucPeerType);
                        goto process_fail;
                        break;
                }

                pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_EXEC;
            }
            break;

        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PROC:
        case SC_PREVIEW_CALL_ALERTING:
        case SC_PREVIEW_CALL_ACTIVE:
        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
        case SC_PREVIEW_CALL_CONNECTED:
        case SC_PREVIEW_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard auth event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed Preview auth event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

process_fail:
    if (DOS_ADDR_VALID(pstLCB))
    {
        sc_lcb_free(pstLCB);
        pstLCB = NULL;
    }

    if (DOS_ADDR_VALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    return DOS_FAIL;
}

U32 sc_preview_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstCallingCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing preview call setup event event. status : %u", pstSCB->stPreviewCall.stSCBTag.usStatus);

    pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto fail_proc;
    }

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
            /* 未认证通过，放音挂断呼叫 */
            goto fail_proc;
            break;

        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PROC:
        case SC_PREVIEW_CALL_ALERTING:
            /* 迁移状态到proc */
            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_ACTIVE:
        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
        case SC_PREVIEW_CALL_CONNECTED:
        case SC_PREVIEW_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

fail_proc:
    return DOS_FAIL;

}


U32 sc_preview_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                 ulRet           = DOS_FAIL;
    SC_LEG_CB           *pstCallingCB   = NULL;
    SC_LEG_CB           *pstCalleeCB    = NULL;
    SC_AGENT_NODE_ST    *pstAgentNode   = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing preview call answer event event. status : %u", pstSCB->stPreviewCall.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing preview call answer event event. status : %u", pstSCB->stPreviewCall.stSCBTag.usStatus);

    pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto fail_proc;
    }

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
            ulRet = DOS_FAIL;
            goto fail_proc;
            break;

        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PROC:
        case SC_PREVIEW_CALL_ALERTING:
            /* 坐席接通之后的处理 */
            /* 1. 发起PSTN的呼叫 */
            /* 2. 迁移状态到CONNTECTING */
            pstCalleeCB = sc_lcb_alloc();
            if (DOS_ADDR_INVALID(pstCalleeCB))
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc lcb fail");
                goto fail_proc;
            }

            pstCalleeCB->stCall.bValid = DOS_TRUE;
            pstCalleeCB->stCall.ucStatus = SC_LEG_INIT;
            pstCalleeCB->ulSCBNo = pstSCB->ulSCBNo;
            pstSCB->stPreviewCall.ulCalleeLegNo = pstCalleeCB->ulCBNo;

            pstAgentNode = sc_agent_get_by_id(pstSCB->stPreviewCall.ulAgentID);
            if (DOS_ADDR_INVALID(pstAgentNode)
                || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
            {
                /* TODO 坐席没找到，错误 */
                DOS_ASSERT(0);
                break;
            }

            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szLastCustomerNum);
            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeCB->stCall.stNumInfo.szOriginalCalling), pstCallingCB->stCall.stNumInfo.szOriginalCalling);

            /* @TODO 通过号码设定选择主叫号码，设置到 pstCalleeCB->stCall.stNumInfo.szRealCalling */
            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeCB->stCall.stNumInfo.szRealCalling), pstCallingCB->stCall.stNumInfo.szOriginalCalling);

            if (sc_make_call2pstn(pstSCB, pstCalleeCB) != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Make call to pstn fail.");
                /* 给主叫放提示音，并挂断 */
                pstSCB->stPreviewCall.ulCalleeLegNo = U32_BUTT;
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
                sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCallingLegNo, CC_ERR_SC_NO_ROUTE);
                goto fail_proc;
            }

            sc_agent_serv_status_update(pstAgentNode->pstAgentInfo, SC_ACD_SERV_RINGBACK, SC_SRV_PREVIEW_CALL);
            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_CONNECTING;
            break;

        case SC_PREVIEW_CALL_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
            pstCalleeCB = sc_lcb_get(pstSCB->stPreviewCall.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCalleeCB))
            {
                sc_trace_scb(pstSCB, "There is no calling leg.");

                goto fail_proc;
            }

            pstAgentNode = sc_agent_get_by_id(pstSCB->stPreviewCall.ulAgentID);
            if (DOS_ADDR_INVALID(pstAgentNode)
                || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
            {
                /* TODO 坐席没找到，错误 */
                DOS_ASSERT(0);
                break;
            }

            /* 修改坐席的业务状态 */
            sc_agent_serv_status_update(pstAgentNode->pstAgentInfo, SC_ACD_SERV_CALL_OUT, SC_SRV_PREVIEW_CALL);
            if (!pstCalleeCB->stCall.bEarlyMedia)
            {
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCalleeLegNo, pstSCB->stPreviewCall.ulCallingLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    goto fail_proc;
                }
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_CONNECTED;
            break;

        case SC_PREVIEW_CALL_CONNECTED:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_PROCESS:
            /* 处理长签之内的一个事情 */
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call answer event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

fail_proc:
    /* TODO 错误处理 */
    return DOS_FAIL;
}

U32 sc_preview_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_MSG_EVT_RINGING_ST   *pstRinging;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstRinging = (SC_MSG_EVT_RINGING_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Proccessing preview call setup event event. status : %u", pstSCB->stPreviewCall.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing preview call setup event event. status : %u. WithMedia : %u", pstSCB->stPreviewCall.stSCBTag.usStatus, pstRinging->ulWithMedia);

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
            /* 未认证通过，放音挂断呼叫 */
            ulRet = DOS_FAIL;
            goto fail_proc;
            break;

        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PROC:
            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_ALERTING;

            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_ALERTING:
            break;

        case SC_PREVIEW_CALL_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
            /* 迁移到alerting状态 */
            /* 如果有媒体需要bridge呼叫，否则给主动放回铃音 */
            sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCallingLegNo, DOS_TRUE, pstRinging->ulWithMedia);
            if (pstRinging->ulWithMedia)
            {
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCalleeLegNo, pstSCB->stPreviewCall.ulCallingLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    goto fail_proc;
                }
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_ALERTING2;
            break;

        case SC_PREVIEW_CALL_CONNECTED:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call ringing event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

fail_proc:
    return DOS_FAIL;

}

U32 sc_preview_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                     ulRet           = DOS_SUCC;
    SC_LEG_CB               *pstCallingCB   = NULL;
    SC_LEG_CB               *pstCalleeCB    = NULL;
    SC_LEG_CB               *pstHungupLeg   = NULL;
    SC_LEG_CB               *pstOtherLeg    = NULL;
    SC_MSG_EVT_HUNGUP_ST    *pstHungup      = NULL;
    SC_AGENT_NODE_ST        *pstAgentCall   = NULL;
    S32                     i               = 0;
    S32                     lRes            = DOS_FAIL;
    U32                     ulReleasePart;
    U32                     ulErrCode;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    ulErrCode = pstHungup->ulErrCode;
    sc_trace_scb(pstSCB, "Proccessing preview call hungup event. status : %u", pstSCB->stPreviewCall.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing preview call hungup event. status : %u", pstSCB->stPreviewCall.stSCBTag.usStatus);

    if (pstHungup->ulLegNo == pstSCB->stPreviewCall.ulCallingLegNo)
    {
        ulReleasePart = SC_CALLING;
    }
    else
    {
        ulReleasePart = SC_CALLEE;
    }

    if (pstSCB->stPreviewCall.stSCBTag.usStatus != SC_PREVIEW_CALL_PROCESS
        && pstSCB->stPreviewCall.stSCBTag.usStatus != SC_PREVIEW_CALL_RELEASE)
    {
        pstHungupLeg = sc_lcb_get(pstHungup->ulLegNo);
        if (DOS_ADDR_VALID(pstHungupLeg)
            && pstHungupLeg->stCall.stTimeInfo.ulAnswerTime > 0)
        {
            sc_send_client_contect_req(pstSCB->ulClientID, pstSCB->ulClientID, pstSCB->szClientNum, pstHungupLeg->stCall.stTimeInfo.ulAnswerTime);
        }
    }

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PROC:
        case SC_PREVIEW_CALL_ALERTING:
        case SC_PREVIEW_CALL_ACTIVE:
            /* 这个时候挂断只会是坐席的LEG，修改坐席的状态 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent not connected.");
            pstAgentCall = sc_agent_get_by_id(pstSCB->stPreviewCall.ulAgentID);
            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_PREVIEW_CALL);
                pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stPreviewCall.ulCalleeLegNo);
            if (pstCallingCB)
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            if (pstCalleeCB)
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
            /* 这个时候挂断，可能是坐席也可能客户，如果是客户需要注意LEG的状态 */
            pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stPreviewCall.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCallingCB) || DOS_ADDR_INVALID(pstCalleeCB))
            {
                /* 异常 */
                DOS_ASSERT(0);
                if (DOS_ADDR_VALID(pstCallingCB))
                {
                    sc_lcb_free(pstCallingCB);
                }
                if (DOS_ADDR_VALID(pstCalleeCB))
                {
                    sc_lcb_free(pstCalleeCB);
                }

                sc_scb_free(pstSCB);
                pstSCB = NULL;
                break;
            }

            if (pstSCB->stPreviewCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCalleeCB;
                pstOtherLeg  = pstCallingCB;
                pstCallingCB->stCall.stTimeInfo.ulByeTime = pstCalleeCB->stCall.stTimeInfo.ulByeTime;
            }
            else
            {
                pstHungupLeg = pstCallingCB;
                pstOtherLeg  = pstCalleeCB;
                pstCalleeCB->stCall.stTimeInfo.ulByeTime = pstCallingCB->stCall.stTimeInfo.ulByeTime;
            }

            /* 生成话单 */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            if (sc_scb_is_exit_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                /* 如果有出局呼叫，应该先将出局呼叫删除 */
                sc_scb_remove_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCalleeCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalleeCB, NULL, ulReleasePart);
                /* 出局呼叫的话单应该用坐席那条leg产生 */
                sc_scb_remove_service(pstSCB, BS_SERV_PREVIEW_DIALING);
                sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);
            }
            else
            {
                pstCalleeCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalleeCB, NULL, ulReleasePart);
            }

            pstAgentCall = sc_agent_get_by_id(pstSCB->stPreviewCall.ulAgentID);
            if (pstSCB->stPreviewCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                /* 客户挂断的，判断坐席是否长签，修改坐席的状态，挂断电话等 */
                if (DOS_ADDR_INVALID(pstAgentCall) || DOS_ADDR_INVALID(pstAgentCall->pstAgentInfo))
                {
                    sc_req_playback_stop(pstSCB->ulSCBNo, pstCallingCB->ulCBNo);
                    sc_req_hungup(pstSCB->ulSCBNo, pstCallingCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                    pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_RELEASE;
                }

                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_PREVIEW_CALL);

                if (pstCallingCB->ulIndSCBNo != U32_BUTT)
                {
                    /* 长签 */
                    pstCallingCB->ulSCBNo = U32_BUTT;
                    if (DOS_ADDR_VALID(pstCalleeCB))
                    {
                        sc_lcb_free(pstCalleeCB);
                        pstCalleeCB = NULL;
                    }

                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                    sc_req_playback_stop(pstCallingCB->ulIndSCBNo, pstCallingCB->ulCBNo);
                    sc_req_play_sound(pstCallingCB->ulIndSCBNo, pstCallingCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);

                    return DOS_SUCC;
                }

                pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                if (DOS_ADDR_VALID(pstCalleeCB))
                {
                    sc_lcb_free(pstCalleeCB);
                    pstCalleeCB = NULL;
                }
                pstSCB->stPreviewCall.ulCalleeLegNo = U32_BUTT;
                pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_RELEASE;
                /* 可能在放回铃音，需要手动停止 */
                sc_req_playback_stop(pstSCB->ulSCBNo, pstCallingCB->ulCBNo);
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstCallingCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
            }
            else
            {
                /* 坐席挂断，修改坐席的状态，挂断客户的电话 */
                if (DOS_ADDR_VALID(pstAgentCall) && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_PREVIEW_CALL);
                    pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                }

                if (DOS_ADDR_VALID(pstCallingCB))
                {
                    sc_lcb_free(pstCallingCB);
                    pstCallingCB = NULL;
                }
                pstSCB->stPreviewCall.ulCallingLegNo = U32_BUTT;
                pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_RELEASE;
                sc_req_hungup(pstSCB->ulSCBNo, pstCalleeCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
            }

            break;

        case SC_PREVIEW_CALL_CONNECTED:
            /* 这个时候挂断，就是正常释放的节奏，处理完就好 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent connected.");

            pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stPreviewCall.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCallingCB) || DOS_ADDR_INVALID(pstCalleeCB))
            {
                /* 异常 */
                DOS_ASSERT(0);
                if (DOS_ADDR_VALID(pstCallingCB))
                {
                    sc_lcb_free(pstCallingCB);
                }

                if (DOS_ADDR_VALID(pstCalleeCB))
                {
                    sc_lcb_free(pstCalleeCB);
                }

                sc_scb_free(pstSCB);
                pstSCB = NULL;
                break;
            }

            if (pstSCB->stPreviewCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCalleeCB;
                pstOtherLeg  = pstCallingCB;
                pstCallingCB->stCall.stTimeInfo.ulByeTime = pstCalleeCB->stCall.stTimeInfo.ulByeTime;
            }
            else
            {
                pstHungupLeg = pstCallingCB;
                pstOtherLeg  = pstCalleeCB;
                pstCalleeCB->stCall.stTimeInfo.ulByeTime = pstCallingCB->stCall.stTimeInfo.ulByeTime;
            }

            if (pstCalleeCB->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCalleeCB->ulOtherSCBNo);
                pstCalleeCB->ulOtherSCBNo = U32_BUTT;
            }

            if (pstCallingCB->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCallingCB->ulOtherSCBNo);
                pstCallingCB->ulOtherSCBNo = U32_BUTT;
            }

            /* 生成话单 */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            if (sc_scb_is_exit_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                /* 如果有出局呼叫，应该先将出局呼叫删除 */
                sc_scb_remove_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCalleeCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalleeCB, NULL, ulReleasePart);
                /* 出局呼叫的话单应该用坐席那条leg产生 */
                sc_scb_remove_service(pstSCB, BS_SERV_PREVIEW_DIALING);
                sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);
            }
            else
            {
                pstCalleeCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalleeCB, NULL, ulReleasePart);
            }

            /* 到这里，说明两个leg都OK */
            /*
              * 需要看看是否长签等问题，如果主/被叫LEG都长签了，需要申请SCB，将被叫LEG挂到新的SCB中
              * 否则，将需要长签的LEG作为当前业务控制块的主叫LEG，挂断另外一条LEG
              * 可能需要处理客户标记
              */
            /* release 时，肯定是有一条leg hungup了，现在的leg需要释放掉，判断另一条是不是坐席长签，如果不是需要挂断 */
            pstAgentCall = sc_agent_get_by_id(pstSCB->stPreviewCall.ulAgentID);
            if (DOS_ADDR_INVALID(pstAgentCall) || DOS_ADDR_INVALID(pstAgentCall->pstAgentInfo))
            {
                /* 没有找到坐席 */
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Can not found agent by id(%u)", pstSCB->stPreviewCall.ulAgentID);
            }

            /* 判断是否需要进行，客户标记。1、是客户一端先挂断的(基础呼叫中，客户只能是PSTN，坐席只能是SIP) */
            if (pstHungupLeg == pstCalleeCB
                && DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && pstAgentCall->pstAgentInfo->ucProcesingTime != 0
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                /* 客户标记 */
                pstSCB->stMarkCustom.stSCBTag.bValid = DOS_TRUE;
                pstSCB->stMarkCustom.ulLegNo = pstOtherLeg->ulCBNo;
                pstSCB->stMarkCustom.pstAgentCall = pstAgentCall;
                pstSCB->ulCurrentSrv++;
                pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stMarkCustom.stSCBTag;

                if (pstOtherLeg->ulIndSCBNo == U32_BUTT)
                {
                    /* 非长签时，要把坐席对应的leg的结束时间，赋值给开始时间，出标记话单时使用 */
                    pstOtherLeg->stCall.stTimeInfo.ulAnswerTime = pstHungupLeg->stCall.stTimeInfo.ulByeTime;
                    for (i=0; i<SC_MAX_SERVICE_TYPE; i++)
                    {
                        pstSCB->aucServType[i] = 0;
                    }

                    if (pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);
                    }
                    else if(pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                    }
                    else
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
                    }

                    /* 将客户的号码改为主叫号码 */
                    dos_strcpy(pstOtherLeg->stCall.stNumInfo.szOriginalCalling, pstAgentCall->pstAgentInfo->szLastCustomerNum);
                }

                /* 修改坐席状态为 proc，播放 标记背景音 */
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_PROC, SC_SRV_PREVIEW_CALL);
                sc_req_play_sound(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, SC_SND_CALL_OVER, 1, 0, 0);
                pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_IDEL;

                /* 开启定时器 */
                lRes = dos_tmr_start(&pstSCB->stMarkCustom.stTmrHandle, pstAgentCall->pstAgentInfo->ucProcesingTime * 1000, sc_agent_mark_custom_callback, (U64)pstOtherLeg->ulCBNo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    pstSCB->stMarkCustom.stTmrHandle = NULL;
                }

                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;

                if (pstSCB->stPreviewCall.ulCalleeLegNo == pstHungup->ulLegNo)
                {
                    pstSCB->stPreviewCall.ulCalleeLegNo = U32_BUTT;
                }
                else
                {
                    pstSCB->stPreviewCall.ulCallingLegNo = U32_BUTT;
                }

                pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_PROCESS;

                break;
            }

            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                pstAgentCall->pstAgentInfo->bMarkCustomer = DOS_FALSE;
            }

            /* 不需要客户标记 */
            if (pstSCB->stPreviewCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstSCB->stPreviewCall.ulCalleeLegNo = U32_BUTT;
            }
            else
            {
                pstSCB->stPreviewCall.ulCallingLegNo = U32_BUTT;
            }

            /* 修改坐席的状态 */
            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_PREVIEW_CALL);
            }

            if (pstOtherLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签，继续放音 */
                pstOtherLeg->ulSCBNo = U32_BUTT;
                sc_req_play_sound(pstOtherLeg->ulIndSCBNo, pstOtherLeg->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                /* 释放掉 SCB */
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
            else if (pstHungupLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签的坐席挂断了电话，不要释放leg，解除关系就行 */
                pstHungupLeg->ulSCBNo = U32_BUTT;
                pstSCB->stPreviewCall.ulCallingLegNo = U32_BUTT;
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_RELEASE;
            }
            else
            {
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_RELEASE;
            }

            break;

        case SC_PREVIEW_CALL_PROCESS:
            /* 坐席处理完了，挂断 */
            pstCalleeCB = sc_lcb_get(pstSCB->stPreviewCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_PREVIEW_CALL_RELEASE:
            pstCalleeCB = sc_lcb_get(pstSCB->stPreviewCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call hungup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call release event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

}

U32 sc_preview_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstLCB       =  NULL;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing preview call dtmf event. status : %u", pstSCB->stPreviewCall.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 开启接入码业务 */
    if (pstDTMF->cDTMFVal != '*'
        && pstDTMF->cDTMFVal != '#' )
    {
        /* 第一个字符不是 '*' 或者 '#' 不保存  */
        return DOS_SUCC;
    }

    /* 只有坐席对应的leg执行接入码业务 */
    if (pstDTMF->ulLegNo != pstSCB->stPreviewCall.ulCallingLegNo)
    {
        return DOS_SUCC;
    }

    pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stAccessCode.szDialCache[0] = '\0';
    pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
    pstSCB->stAccessCode.ulAgentID = pstSCB->stPreviewCall.ulAgentID;
    pstSCB->ulCurrentSrv++;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;

    return DOS_SUCC;
}

U32 sc_preview_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HOLD_ST   *pstHold        = NULL;
    SC_LEG_CB            *pstLeg         = NULL;

    pstHold = (SC_MSG_EVT_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstHold->bIsHold)
    {
        /* 如果是被HOLD的，需要激活HOLD业务哦 */
        pstSCB->stHold.stSCBTag.bValid = DOS_TRUE;
        pstSCB->stHold.stSCBTag.bWaitingExit = DOS_FALSE;
        pstSCB->stHold.stSCBTag.usStatus = SC_HOLD_ACTIVE;
        pstSCB->stHold.ulCallLegNo = pstHold->ulLegNo;
        pstSCB->stHold.ulHoldCount++;

        pstSCB->ulCurrentSrv++;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stHold.stSCBTag;

        /* 给HOLD方 播放拨号音 */
        /* 给HOLD对方 播放呼叫保持音 */
    }
    else
    {
        /* 如果是被UNHOLD的，已经没有HOLD业务了，单纯处理呼叫就好 */
        pstLeg = sc_lcb_get(pstHold->ulLegNo);
        if (DOS_ADDR_INVALID(pstLeg))
        {
            return DOS_FAIL;
        }

        if (pstLeg->stHold.ulHoldTime != 0
            && pstLeg->stHold.ulUnHoldTime > pstLeg->stHold.ulHoldTime)
        {
            pstSCB->stHold.ulHoldTotalTime += (pstLeg->stHold.ulUnHoldTime - pstLeg->stHold.ulHoldTime);
        }

        pstLeg->stHold.ulUnHoldTime = 0;
        pstLeg->stHold.ulHoldTime = 0;
    }

    return DOS_SUCC;
}

U32 sc_preview_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* 处理录音结束 */
    SC_MSG_CMD_RECORD_ST *pstRecord = NULL;
    SC_LEG_CB            *pstLCB    = NULL;

    pstRecord = (SC_MSG_CMD_RECORD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstRecord) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing preview call record stop event. status : %u", pstSCB->stPreviewCall.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstRecord->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        return DOS_FAIL;
    }

    /* 生成录音话单 */
    sc_send_special_billing_stop2bs(pstSCB, pstLCB, BS_SERV_RECORDING);
    sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);

    return DOS_SUCC;
}

U32 sc_preview_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB                   *pstCallingCB       = NULL;
    SC_AGENT_NODE_ST            *pstAgentCall       = NULL;

    /* 处理放音结束 */
    sc_trace_scb(pstSCB, "Proccessing preview playback stop event. status : %u", pstSCB->stPreviewCall.stSCBTag.usStatus);

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_RELEASE:
            pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                pstAgentCall = sc_agent_get_by_id(pstSCB->stPreviewCall.ulAgentID);
                if (DOS_ADDR_VALID(pstAgentCall)
                    && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
                {
                    if (pstCallingCB->ulIndSCBNo != U32_BUTT)
                    {
                        /* 坐席长签的时候，呼叫对端失败，方提示音后到这里，修改坐席的状态，放提示音 */
                        pstCallingCB->ulSCBNo = U32_BUTT;
                        sc_scb_free(pstSCB);
                        pstSCB = NULL;
                        sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_PREVIEW_CALL);
                        sc_req_play_sound(pstCallingCB->ulIndSCBNo, pstCallingCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                    }
                    else
                    {
                        sc_req_hungup(pstSCB->ulSCBNo, pstCallingCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                        sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_PREVIEW_CALL);
                        sc_lcb_free(pstCallingCB);
                        pstCallingCB = NULL;
                        sc_scb_free(pstSCB);
                        pstSCB = NULL;
                    }
                }
            }
            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_preview_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;
    SC_LEG_CB                   *pstCallingCB       = NULL;
    SC_LEG_CB                   *pstCalleeCB        = NULL;
    SC_AGENT_NODE_ST            *pstAgentCall       = NULL;
    SC_MSG_CMD_RECORD_ST        stRecordRsp;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing preview error event. status : %u, msg type : %u, interErr : %u"
        , pstSCB->stPreviewCall.stSCBTag.usStatus, pstErrReport->stMsgTag.ulMsgType, pstErrReport->stMsgTag.usInterErr);
    sc_log_digest_print_only(pstSCB, "Proccessing preview error event. status : %u, msg type : %u, interErr : %u"
        , pstSCB->stPreviewCall.stSCBTag.usStatus, pstErrReport->stMsgTag.ulMsgType, pstErrReport->stMsgTag.usInterErr);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */
        if (!sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
        {
            return DOS_SUCC;
        }

        pstCalleeCB = sc_lcb_get(pstSCB->stPreviewCall.ulCalleeLegNo);
        if (DOS_ADDR_VALID(pstCalleeCB))
        {
            stRecordRsp.stMsgTag.ulMsgType = SC_CMD_RECORD;
            stRecordRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stRecordRsp.stMsgTag.usInterErr = 0;
            stRecordRsp.ulSCBNo = pstSCB->ulSCBNo;
            stRecordRsp.ulLegNo = pstCalleeCB->ulCBNo;

            if (sc_send_cmd_record(&stRecordRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_INFO, SC_MOD_EVENT, SC_LOG_DISIST), "Send record cmd FAIL! SCBNo : %u", pstSCB->ulSCBNo);
            }
        }

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
        case SC_PREVIEW_CALL_EXEC:
            /* 发起呼叫失败，直接释放资源 */
            pstAgentCall = sc_agent_get_by_id(pstSCB->stPreviewCall.ulAgentID);
            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_PREVIEW_CALL);
            }

            pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
            }
            sc_scb_free(pstSCB);
            break;

        case SC_PREVIEW_CALL_PROC:
        case SC_PREVIEW_CALL_ALERTING:
            /* 坐席没有接听，不用处理，release中会处理 */
            break;

        case SC_PREVIEW_CALL_ACTIVE:
        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
            /* 呼叫客户失败 */
            pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                pstAgentCall = sc_agent_get_by_id(pstSCB->stPreviewCall.ulAgentID);
                if (DOS_ADDR_VALID(pstAgentCall)
                    && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
                {
                    /* 释放掉被叫leg */
                    pstCalleeCB = sc_lcb_get(pstSCB->stPreviewCall.ulCalleeLegNo);
                    if (DOS_ADDR_VALID(pstCalleeCB))
                    {
                        sc_lcb_free(pstCalleeCB);
                        pstCalleeCB = NULL;
                        pstSCB->stPreviewCall.ulCalleeLegNo = U32_BUTT;
                    }

                    if (pstCallingCB->ulIndSCBNo != U32_BUTT)
                    {
                        /* 坐席长签的时候，呼叫对端失败，方提示音后到这里，修改坐席的状态，放提示音 */
                        pstCallingCB->ulSCBNo = U32_BUTT;
                        sc_scb_free(pstSCB);
                        pstSCB = NULL;
                        sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_PREVIEW_CALL);
                        sc_req_play_sound(pstCallingCB->ulIndSCBNo, pstCallingCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                    }
                    else
                    {
                        sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCallingLegNo);
                        sc_req_hungup(pstSCB->ulSCBNo, pstCallingCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                        sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_PREVIEW_CALL);
                    }
                    break;
                }
            }
            ulRet = sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCallingLegNo, ulErrCode);
            break;

        case SC_PREVIEW_CALL_CONNECTED:
        case SC_PREVIEW_CALL_PROCESS:
        case SC_PREVIEW_CALL_RELEASE:
            if (pstSCB->stPreviewCall.ulCalleeLegNo != U32_BUTT)
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCalleeLegNo, ulErrCode);
            }
            else
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCallingLegNo, ulErrCode);
            }
            break;
        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_voice_verify_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB                   *pstLCB     = NULL;
    U32                         ulRet       = DOS_FAIL;
    SC_MSG_EVT_AUTH_RESULT_ST   *pstAuthRsp = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    sc_trace_scb(pstSCB, "Processing auth rsp event for voice verify. status : %u", pstSCB->stVoiceVerify.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        goto proc_finishe;
    }


    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_ERROR, SC_MOD_EVENT, SC_LOG_DISIST), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        sc_log_digest_print_only(pstSCB, "Processing auth rsp event for voice verify. error : %u", pstAuthRsp->stMsgTag.usInterErr);
        ulRet = DOS_FAIL;

        goto proc_finishe;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
            break;

        case SC_VOICE_VERIFY_AUTH:
            ulRet = sc_make_call2pstn(pstSCB, pstLCB);
            if (ulRet != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Make call for voice verify fail.");
                goto proc_finishe;
            }

            pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_EXEC;
            break;

        case SC_VOICE_VERIFY_EXEC:
            break;

        case SC_VOICE_VERIFY_PROC:
            break;

        case SC_VOICE_VERIFY_ALERTING:
            break;

        case SC_VOICE_VERIFY_ACTIVE:
            break;

        case SC_VOICE_VERIFY_RELEASE:
            break;
    }

proc_finishe:

    if (ulRet != DOS_SUCC)
    {
        if (pstLCB)
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }

        if (pstSCB)
        {
            sc_scb_free(pstSCB);
            pstSCB = NULL;
        }
    }

    sc_trace_scb(pstSCB, "Processed auth rsp event for voice verify. Ret: %s", ulRet != DOS_SUCC ? "FAIL" : "succ");

    return ulRet;
}

U32 sc_voice_verify_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call setup event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        goto proc_finishe;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
            break;

        case SC_VOICE_VERIFY_AUTH:
            break;

        case SC_VOICE_VERIFY_EXEC:
            pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_VOICE_VERIFY_PROC:
            break;

        case SC_VOICE_VERIFY_ALERTING:
            break;

        case SC_VOICE_VERIFY_ACTIVE:
            break;

        case SC_VOICE_VERIFY_RELEASE:
            break;
    }


proc_finishe:
    sc_trace_scb(pstSCB, "Processed call setup event for voice verify. Ret: %s", (ulRet == DOS_SUCC) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB) && pstLCB->stCall.bValid && pstLCB->stCall.ucStatus >= SC_LEG_PROC)
        {
            sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_SC_FORBIDDEN);
        }
        else
        {
            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }

            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
        }
    }

    return ulRet;
}

U32 sc_voice_verify_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet   = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing verify ringing event for voice verify. status : %u", pstSCB->stVoiceVerify.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Processing verify ringing event for voice verify. status : %u", pstSCB->stVoiceVerify.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        goto proc_finishe;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
            break;

        case SC_VOICE_VERIFY_AUTH:
            break;

        case SC_VOICE_VERIFY_EXEC:
        case SC_VOICE_VERIFY_PROC:
        case SC_VOICE_VERIFY_ALERTING:
            pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_ALERTING;
            ulRet = DOS_SUCC;
            break;

        case SC_VOICE_VERIFY_ACTIVE:
            break;

        case SC_VOICE_VERIFY_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Processed verify ringing event for voice verify.");

    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB) && pstLCB->stCall.bValid && pstLCB->stCall.ucStatus >= SC_LEG_PROC)
        {
            sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_SC_FORBIDDEN);
        }
        else
        {
            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }

            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
        }
    }

    return ulRet;
}

U32 sc_voice_verify_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet   = DOS_FAIL;
    U32        ulLoop  = DOS_FAIL;
    SC_MSG_CMD_PLAYBACK_ST  stPlaybackRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call answer event for voice verify. status : %u", pstSCB->stVoiceVerify.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Processing call answer event for voice verify. status : %u", pstSCB->stVoiceVerify.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        goto proc_finishe;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
            break;

        case SC_VOICE_VERIFY_AUTH:
            break;

        case SC_VOICE_VERIFY_EXEC:
        case SC_VOICE_VERIFY_PROC:
        case SC_VOICE_VERIFY_ALERTING:
            pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_ACTIVE;

            stPlaybackRsp.stMsgTag.ulMsgType = SC_CMD_PLAYBACK;
            stPlaybackRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stPlaybackRsp.stMsgTag.usInterErr = 0;
            stPlaybackRsp.ulMode = 0;
            stPlaybackRsp.ulSCBNo = pstSCB->ulSCBNo;
            stPlaybackRsp.ulLegNo = pstLCB->ulCBNo;
            stPlaybackRsp.ulLoopCnt = SC_NUM_VERIFY_TIME;
            stPlaybackRsp.ulInterval = 0;
            stPlaybackRsp.ulSilence  = 0;
            stPlaybackRsp.enType = SC_CND_PLAYBACK_SYSTEM;
            stPlaybackRsp.ulTotalAudioCnt = 0;
            stPlaybackRsp.blNeedDTMF = DOS_FALSE;

            stPlaybackRsp.aulAudioList[0] = pstSCB->stVoiceVerify.ulTipsHitNo1;
            stPlaybackRsp.ulTotalAudioCnt++;
            for (ulLoop=0; ulLoop<SC_MAX_AUDIO_NUM; ulLoop++)
            {
                if ('\0' == pstSCB->stVoiceVerify.szVerifyCode[ulLoop])
                {
                    break;
                }

                switch (pstSCB->stVoiceVerify.szVerifyCode[ulLoop])
                {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        stPlaybackRsp.aulAudioList[stPlaybackRsp.ulTotalAudioCnt] = SC_SND_0 + (pstSCB->stVoiceVerify.szVerifyCode[ulLoop] - '0');
                        stPlaybackRsp.ulTotalAudioCnt++;
                        break;
                    default:
                        DOS_ASSERT(0);
                        break;
                }
            }

            if (sc_send_cmd_playback(&stPlaybackRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                goto proc_finishe;
            }

            ulRet = DOS_SUCC;

            break;

        case SC_VOICE_VERIFY_ACTIVE:
            break;

        case SC_VOICE_VERIFY_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Processed call answer event for voice verify.");

    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB) && pstLCB->stCall.bValid && pstLCB->stCall.ucStatus >= SC_LEG_PROC)
        {
            sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_SC_FORBIDDEN);
        }
        else
        {
            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }

            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
        }
    }

    return ulRet;
}

U32 sc_voice_verify_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB               *pstLCB         = NULL;
    SC_MSG_EVT_HUNGUP_ST    *pstHungup      = NULL;
    U32                     ulErrCode;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;

    ulErrCode = pstHungup->ulErrCode;
    sc_trace_scb(pstSCB, "Processing call release event for voice verify. status : %u", pstSCB->stVoiceVerify.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Processing call release event for voice verify. status : %u", pstSCB->stVoiceVerify.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;

        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        return DOS_FAIL;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
        case SC_VOICE_VERIFY_AUTH:
        case SC_VOICE_VERIFY_EXEC:
        case SC_VOICE_VERIFY_PROC:
        case SC_VOICE_VERIFY_ALERTING:
            /* 释放资源 */
            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }

            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }
            break;

        case SC_VOICE_VERIFY_ACTIVE:
        case SC_VOICE_VERIFY_RELEASE:
            /* 发送话单 */
            pstLCB->stCall.ulCause = ulErrCode;
            sc_send_billing_stop2bs(pstSCB, pstLCB, NULL, SC_CALLEE);

            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }

            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }
            break;
    }

    sc_trace_scb(pstSCB, "Processed call release event for voice verify.");

    return DOS_SUCC;
}

U32 sc_voice_verify_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call release event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;

        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        return DOS_FAIL;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
        case SC_VOICE_VERIFY_AUTH:
        case SC_VOICE_VERIFY_EXEC:
        case SC_VOICE_VERIFY_PROC:
        case SC_VOICE_VERIFY_ALERTING:
            break;
        case SC_VOICE_VERIFY_ACTIVE:
            sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
            break;
        case SC_VOICE_VERIFY_RELEASE:
            /* 发送话单 */
            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }

            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }
            break;
        default:
            break;
    }

    sc_trace_scb(pstSCB, "Processed call release event for voice verify.");

    return DOS_SUCC;
}

U32 sc_voice_verify_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;
    SC_LEG_CB                   *pstCallingCB       = NULL;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing verify error event. status : %u", pstSCB->stVoiceVerify.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing verify error event. status : %u", pstSCB->stVoiceVerify.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
        case SC_VOICE_VERIFY_AUTH:
        case SC_VOICE_VERIFY_EXEC:
            /* 发起呼叫失败，直接释放资源 */
            pstCallingCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
            }
            sc_scb_free(pstSCB);
            break;

        case SC_VOICE_VERIFY_PROC:
        case SC_VOICE_VERIFY_ALERTING:
        case SC_VOICE_VERIFY_ACTIVE:
        case SC_VOICE_VERIFY_RELEASE:
            ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stVoiceVerify.ulLegNo, ulErrCode);
            break;
        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_switchboard_start(SC_SRV_CB *pstSCB, U32 ulBindID)
{
    U8                 ucIndex;
    SC_COR_SW_NODE_ST  *pstCorSWNode = NULL;
    SC_SW_IVR_NODE_ST  *pstCorSWIVRNode = NULL;
    SC_SW_IVR_NODE_ST  *pstCorSWDefaultIVRNode = NULL;
    SC_LEG_CB           *pstLCB = NULL;

    SC_MSG_CMD_PLAYBACK_ST  stPlaybackRsp;
    U32          ulErrCode   = CC_ERR_NO_REASON;

    pstCorSWNode = sc_get_sw_by_id(ulBindID);
    if (DOS_ADDR_INVALID(pstCorSWNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstSCB->stCorSwitchboard.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        return DOS_FAIL;
    }

    for (ucIndex = 0; ucIndex < pstCorSWNode->ucIndex; ucIndex++)
    {
        if (!pstCorSWNode->pstSWPeriodNode[ucIndex]->bDefault)
        {
            if (sc_ivr_check_can_call_by_time(pstCorSWNode->pstSWPeriodNode[ucIndex]))
            {
                pstCorSWIVRNode = pstCorSWNode->pstSWPeriodNode[ucIndex];
                break;
            }
        }
        else
        {
                pstCorSWDefaultIVRNode = pstCorSWNode->pstSWPeriodNode[ucIndex];
        }
    }

    if (DOS_ADDR_INVALID(pstCorSWIVRNode))
    {
        pstCorSWIVRNode = pstCorSWDefaultIVRNode;
        sc_trace_scb(pstSCB, "Use the default period.");
    }

    if (DOS_ADDR_INVALID(pstCorSWIVRNode))
    {
        sc_trace_scb(pstSCB, "Cannot find the switch board.");
        return DOS_FAIL;
    }

    stPlaybackRsp.stMsgTag.ulMsgType = SC_CMD_PLAYBACK;
    stPlaybackRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stPlaybackRsp.stMsgTag.usInterErr = 0;
    stPlaybackRsp.ulMode = 0;
    stPlaybackRsp.ulSCBNo = pstSCB->ulSCBNo;
    stPlaybackRsp.ulLegNo = pstLCB->ulCBNo;
    stPlaybackRsp.ulLoopCnt = 1;
    stPlaybackRsp.ulInterval = 0;
    stPlaybackRsp.ulSilence  = 0;
    stPlaybackRsp.enType = SC_CND_PLAYBACK_FILE;
    stPlaybackRsp.blNeedDTMF = DOS_TRUE;
    stPlaybackRsp.ulTotalAudioCnt++;

    dos_snprintf(stPlaybackRsp.szAudioFile, SC_MAX_AUDIO_FILENAME_LEN, "%s/%u/%u", SC_IVR_AUDIO_PATH , pstSCB->ulCustomerID, pstCorSWIVRNode->ulIvrAudioID);

    stPlaybackRsp.szAudioFile[SC_MAX_AUDIO_FILENAME_LEN - 1] = '\0';

    if (sc_send_cmd_playback(&stPlaybackRsp.stMsgTag) != DOS_SUCC)
    {
        ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
        return DOS_FAIL;
    }
    pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_PLAY_AUDIO;
    pstSCB->stCorSwitchboard.pstSWPeriodNode = pstCorSWIVRNode;

    return DOS_SUCC;
}

U32 sc_switchboard_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstCallingCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing switchboard setup event. status : %u", pstSCB->stCorSwitchboard.stSCBTag.usStatus);

    pstCallingCB = sc_lcb_get(pstSCB->stCorSwitchboard.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto fail_proc;
    }

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_COR_SWITCHBOARD_IDEL:
        case SC_COR_SWITCHBOARD_PLAY_AUDIO:
        case SC_COR_SWITCHBOARD_AFTER_KEY:
        case SC_COR_SWITCHBOARD_AUTH2:
            break;

        case SC_COR_SWITCHBOARD_EXEC:
            /* 迁移状态到proc */
            pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_COR_SWITCHBOARD_ALERTING:
        case SC_COR_SWITCHBOARD_CONNECTED:
        case SC_COR_SWITCHBOARD_PROCESS:
        case SC_COR_SWITCHBOARD_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

fail_proc:
    return DOS_FAIL;
}


U32 sc_switchboard_play_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB           *pstCallingLCB     = NULL;
    SC_LEG_CB           *pstCalleeLegCB    = NULL;
    SC_SW_IVR_NODE_ST   *pstSWPeriodNode   = NULL;
    SC_AGENT_NODE_ST    *pstAgentNode      = NULL;
    S8                  szCallee[32] = { 0, };
    U32                 ulForwardID = 0;
    U32                 ulForwardType = 0;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing audio play stop event for switchboard.");

    pstSWPeriodNode = pstSCB->stCorSwitchboard.pstSWPeriodNode;
    if (DOS_ADDR_INVALID(pstSWPeriodNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallingLCB = sc_lcb_get(pstSCB->stCorSwitchboard.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingLCB))
    {
        DOS_ASSERT(0);
        sc_scb_free(pstSCB);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing audio play stop event for switchboard. TransferType: %d", pstSWPeriodNode->ucTransferType);

    switch (pstSCB->stCorSwitchboard.stSCBTag.usStatus)
    {
        case SC_COR_SWITCHBOARD_IDEL:
            break;

        case SC_COR_SWITCHBOARD_PLAY_AUDIO:
            switch(pstSWPeriodNode->ucTransferType)
            {
                case SC_SW_DIRECT_TRANSFER:
                    if (DOS_ADDR_INVALID(pstSWPeriodNode->pstSWKeymapNode[0]))
                    {
                        DOS_ASSERT(0);
                        return DOS_FAIL;
                    }
                    ulForwardID = pstSWPeriodNode->pstSWKeymapNode[0]->ulKeyMap;
                    ulForwardType = pstSWPeriodNode->pstSWKeymapNode[0]->ucKeyMapType;
                    if (ulForwardType == SC_SW_FORWARD_SIP)
                    {
                        if (DOS_SUCC != sc_sip_account_get_by_id(ulForwardID, szCallee, sizeof(szCallee)))
                        {
                            DOS_ASSERT(0);

                            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "The switchboard forwardID is not available.");
                            return DOS_FAIL;
                        }

                        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Start find agent by userid(%s)", szCallee);
                        pstAgentNode = sc_agent_get_by_sip_acc(szCallee);
                        if (DOS_ADDR_VALID(pstAgentNode)
                            && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo)
                            && AGENT_BIND_SIP == pstAgentNode->pstAgentInfo->ucBindType)
                        {
                            pstSCB->stCorSwitchboard.pstAgentCallee = pstAgentNode;
                            ulForwardType = SC_SW_FORWARD_AGENT;
                            ulForwardID = pstAgentNode->pstAgentInfo->ulAgentID;

                            /* 开启呼入队列队列业务控制块 */
                            pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                            pstSCB->ulCurrentSrv++;
                            pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                            pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                            pstSCB->stIncomingQueue.ulLegNo = pstSCB->stCall.ulCallingLegNo;
                            pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                            pstSCB->stIncomingQueue.ulQueueType = ulForwardType;
                            if (sc_cwq_add_call(pstSCB, ulForwardID, pstCallingLCB->stCall.stNumInfo.szRealCalling,
                                ulForwardType, DOS_FALSE) != DOS_SUCC)
                            {
                                 /* 加入队列失败 */
                                DOS_ASSERT(0);
                                return DOS_FAIL;
                            }
                            else
                            {
                                /* 放音提示客户等待 */
                                pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                                sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CALL_QUEUE_WAIT, 1, 0, 0);
                                return  DOS_SUCC;
                            }
                        }
                        else
                        {
                            pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_EXEC;
                            /* 申请一个新的leg，发起呼叫 */
                            pstCalleeLegCB = sc_lcb_alloc();
                            if (DOS_ADDR_INVALID(pstCalleeLegCB))
                            {
                                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Alloc SCB fail.");

                                return DOS_FAIL;
                            }

                            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
                            pstCalleeLegCB->stCall.bValid = DOS_TRUE;
                            pstCalleeLegCB->stCall.ucStatus = SC_LEG_INIT;
                            pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;

                                /* 新LEG处理一下号码 */
                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLCB->stCall.stNumInfo.szRealCalling);

                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLCB->stCall.stNumInfo.szRealCalling);

                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLCB->stCall.stNumInfo.szRealCalling);

                            pstSCB->stCorSwitchboard.ulCalleeLegNo = pstCalleeLegCB->ulCBNo;

                            sc_make_call2sip(pstSCB, pstCalleeLegCB);

                        }
                        break;
                    }
                    else if (ulForwardType == SC_SW_FORWARD_TT)
                    {
                        //sc_agent_get_by_tt_num();
                        break;
                    }
                    else if (ulForwardType == SC_SW_FORWARD_AGENT
                        || ulForwardType == SC_SW_FORWARD_AGENT_GROUP)
                    {
                        /* 开启呼入队列队列业务控制块 */
                        pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                        pstSCB->ulCurrentSrv++;
                        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                        pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                        pstSCB->stIncomingQueue.ulLegNo = pstSCB->stCall.ulCallingLegNo;
                        pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                        pstSCB->stIncomingQueue.ulQueueType = ulForwardType;
                        if (sc_cwq_add_call(pstSCB, ulForwardID, pstCallingLCB->stCall.stNumInfo.szRealCalling,
                            ulForwardType, DOS_FALSE) != DOS_SUCC)
                        {
                             /* 加入队列失败 */
                            DOS_ASSERT(0);
                            return DOS_FAIL;
                        }
                        else
                        {
                            /* 放音提示客户等待 */
                            pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                            sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CALL_QUEUE_WAIT, 1, 0, 0);
                            return  DOS_SUCC;
                        }
                        break;
                    }
                    break;

                case SC_SW_BUTTON_TRANSFER:
                    /* S */
                    break;

                case SC_SW_DIAL_EXTENSION:
                    break;
            }
            break;

        default:
            break;
    }

    sc_trace_scb(pstSCB, "Processed audio play end event for switchboard.");
    return DOS_SUCC;

}


U32 sc_switchboard_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{

    SC_MSG_EVT_LEAVE_CALLQUE_ST     *pstEvtCall         = NULL;
    U32                             ulRet               = DOS_FAIL;
    U32                             ulErrCode           = CC_ERR_NO_REASON;
    SC_LEG_CB                       *pstCallingLegCB    = NULL;
    U32                             ulAgentID           = U32_BUTT;

    pstEvtCall = (SC_MSG_EVT_LEAVE_CALLQUE_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing switchboard queue event. status : %u", pstSCB->stCorSwitchboard.stSCBTag.usStatus);

    switch (pstSCB->stCorSwitchboard.stSCBTag.usStatus)
    {
        case SC_COR_SWITCHBOARD_PLAY_AUDIO:      /*播放语音*/
        case SC_COR_SWITCHBOARD_AFTER_KEY:       /*按键之后操作*/
            if (SC_LEAVE_CALL_QUE_TIMEOUT == pstMsg->usInterErr)
            {

            }
            else if (SC_LEAVE_CALL_QUE_SUCC == pstMsg->usInterErr)
            {
                if (DOS_ADDR_INVALID(pstEvtCall->pstAgentNode))
                {
                    /* 错误 */
                }
                else
                {
                    pstSCB->stCorSwitchboard.pstAgentCallee = pstEvtCall->pstAgentNode;
                    pstCallingLegCB = sc_lcb_get(pstSCB->stCorSwitchboard.ulCallingLegNo);
                    ulAgentID = pstEvtCall->pstAgentNode->pstAgentInfo->ulAgentID;

                    /* 呼叫坐席 */
                    ulRet = sc_switchboard_call_agent(pstSCB, pstCallingLegCB, ulAgentID, &ulErrCode);
                    if (ulRet == DOS_SUCC
                        && pstSCB->stCall.stSCBTag.usStatus != SC_CALL_TONE)
                    {
                        //pstSCB->stCall.bIsRingTimer = DOS_TRUE;
                        sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stCorSwitchboard.ulCallingLegNo);
                        //sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, DOS_TRUE, DOS_FALSE);
                    }
                }
            }
            break;

        case SC_COR_SWITCHBOARD_EXEC:
        case SC_COR_SWITCHBOARD_ALERTING:
        case SC_COR_SWITCHBOARD_CONNECTED:
        case SC_COR_SWITCHBOARD_PROCESS:
        case SC_COR_SWITCHBOARD_RELEASE:
            break;

        default:
            break;

     }

    sc_trace_scb(pstSCB, "Proccessed switchboard answer event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        /* TODO 失败的处理 */
    }

    return ulRet;
}

U32 sc_switchboard_answer(SC_MSG_TAG_ST *pstMsg,  SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ANSWER_ST  *pstEvtAnswer   = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stCorSwitchboard.stSCBTag.usStatus)
    {
        case SC_COR_SWITCHBOARD_IDEL:
            sc_switchboard_start(pstSCB,pstSCB->stCorSwitchboard.ulDidBindID);
            break;
        case SC_COR_SWITCHBOARD_PLAY_AUDIO:
        case SC_COR_SWITCHBOARD_AFTER_KEY:       /*按键之后操作*/
        case SC_COR_SWITCHBOARD_AUTH2:           /*认证被叫*/
            break;

        case SC_COR_SWITCHBOARD_EXEC:            /*认证成功，开始呼叫被叫*/
        case SC_COR_SWITCHBOARD_PROC:
        case SC_COR_SWITCHBOARD_ALERTING:
            pstEvtAnswer = (SC_MSG_EVT_ANSWER_ST *)pstMsg;

            pstSCB->stCorSwitchboard.ulCalleeLegNo = pstEvtAnswer->ulLegNo;

            if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCorSwitchboard.ulCalleeLegNo, pstSCB->stCorSwitchboard.ulCallingLegNo) != DOS_SUCC)
            {
                sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                goto proc_fail;
            }

            if (pstSCB->stCorSwitchboard.pstAgentCallee && pstSCB->stCorSwitchboard.pstAgentCallee->pstAgentInfo)
            {
                sc_agent_serv_status_update(pstSCB->stCorSwitchboard.pstAgentCallee->pstAgentInfo, SC_ACD_SERV_CALL_IN, SC_SRV_COR_SWITCHBOARD);
            }

            pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_CONNECTED;     /*通话*/
            break;
        case SC_COR_SWITCHBOARD_PROCESS:         /*结束后收尾*/
        case SC_COR_SWITCHBOARD_RELEASE:          /*释放资源*/
            sc_trace_scb(pstSCB, "Calling has been answered");
            break;
    }
    return DOS_SUCC;
proc_fail:
    /* 挂断两方 */
    return DOS_FAIL;
}

U32 sc_switchboard_ring(SC_MSG_TAG_ST *pstMsg,  SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_RINGING_ST  *pstEvent = NULL;
    SC_LEG_CB              *pstCalleeLegCB = NULL;
    SC_LEG_CB              *pstCallingLegCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEvent = (SC_MSG_EVT_RINGING_ST *)pstMsg;

    sc_trace_scb(pstSCB, "process alerting msg. calling leg: %u, callee leg: %u, status : %u"
                        , pstSCB->stCall.ulCallingLegNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.stSCBTag.usStatus);

    pstCalleeLegCB = sc_lcb_get(pstSCB->stCorSwitchboard.ulCalleeLegNo);
    pstCallingLegCB = sc_lcb_get(pstSCB->stCorSwitchboard.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCalleeLegCB) || DOS_ADDR_INVALID(pstCallingLegCB))
    {
        sc_trace_scb(pstSCB, "alerting with only one leg.");
        return DOS_SUCC;
    }

    switch (pstSCB->stCorSwitchboard.stSCBTag.usStatus)
    {

        case SC_COR_SWITCHBOARD_IDEL:
        case SC_COR_SWITCHBOARD_PLAY_AUDIO:
        case SC_COR_SWITCHBOARD_AFTER_KEY:       /*按键之后操作*/
            break;

        case SC_COR_SWITCHBOARD_AUTH2:           /*认证被叫*/
            break;

        case SC_COR_SWITCHBOARD_PROC:
        case SC_COR_SWITCHBOARD_EXEC:            /*认证成功，开始呼叫被叫*/
            pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_ALERTING;
            sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stCorSwitchboard.ulCallingLegNo);

            sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stCorSwitchboard.ulCallingLegNo, DOS_TRUE, pstEvent->ulWithMedia);

            if (pstEvent->ulWithMedia)
            {
                pstCalleeLegCB = sc_lcb_get(pstSCB->stCorSwitchboard.ulCalleeLegNo);
                if (DOS_ADDR_VALID(pstCalleeLegCB))
                {
                    if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCorSwitchboard.ulCalleeLegNo, pstSCB->stCorSwitchboard.ulCallingLegNo) != DOS_SUCC)
                    {
                        sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                        goto proc_fail;
                    }
                }
            }

            break;

        case SC_COR_SWITCHBOARD_ALERTING:
            /* 主叫LEG状态变换 */
            sc_trace_scb(pstSCB, "Calling has been ringback.");
            break;

        case SC_COR_SWITCHBOARD_CONNECTED:
        case SC_COR_SWITCHBOARD_PROCESS:
        case SC_COR_SWITCHBOARD_RELEASE:
            break;
    }
    return DOS_SUCC;
proc_fail:
    return DOS_FAIL;
}

U32 sc_switchboard_timeout_callback(U64 uLParam)
{
    return DOS_SUCC;
}

U32 sc_switchboard_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstCallingLegCB       =  NULL;
    SC_LEG_CB             *pstCalleeLegCB  =  NULL;
    SC_SW_IVR_NODE_ST     *pstSWPeriodNode = NULL;
    SC_IVR_KEY_MAP_ST     *pstSWKeymapNode = NULL;
    U32                   ulTrfrType    = U32_BUTT;
    S32                   lKey          = 0;
    S8                    szCallee[32] = { 0, };
    SC_AGENT_NODE_ST      *pstAgentNode      = NULL;
    U32                   ulErrCode = CC_ERR_NO_REASON;
    U32                   ulIndex;
    SC_USER_ID_NODE_ST    *pstUserIDNode = NULL;
    U32                   ulForwardType = 0;
    U32                   ulForwardID = 0;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    pstSWPeriodNode = pstSCB->stCorSwitchboard.pstSWPeriodNode;

    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSWPeriodNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallingLegCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstCallingLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    lKey = pstDTMF->cDTMFVal - '0';

    switch (pstSCB->stCorSwitchboard.stSCBTag.usStatus)
    {
        case SC_COR_SWITCHBOARD_PLAY_AUDIO:
            ulTrfrType = pstSWPeriodNode->ucTransferType;
            switch (ulTrfrType)
            {
                case SC_SW_DIRECT_TRANSFER:
                    break;

                case SC_SW_BUTTON_TRANSFER:
                    for (ulIndex = 0; ulIndex < pstSWPeriodNode-> ulIndex; ulIndex ++)
                    {
                        if (DOS_ADDR_INVALID(pstSWPeriodNode->pstSWKeymapNode[ulIndex]))
                        {
                            DOS_ASSERT(0);
                            continue;
                        }

                        sc_trace_scb(pstSCB, "switchboard lkey : %d, uckey : %u", lKey,pstSWPeriodNode->pstSWKeymapNode[ulIndex]->ulKey);

                        if (lKey == pstSWPeriodNode->pstSWKeymapNode[ulIndex]->ulKey)
                        {
                            pstSWKeymapNode = pstSWPeriodNode->pstSWKeymapNode[ulIndex];
                            ulForwardID = pstSWKeymapNode->ulKeyMap;
                            ulForwardType = pstSWKeymapNode->ucKeyMapType;
                            break;
                        }
                    }


                    if (ulForwardType == SC_SW_FORWARD_SIP)
                    {
                        if (DOS_SUCC != sc_sip_account_get_by_id(ulForwardID, szCallee, sizeof(szCallee)))
                         {
                             DOS_ASSERT(0);

                             sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "The switchboard forwardID is not available.");
                             ulErrCode = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;

                             goto proc_fail;
                         }

                         sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Start find agent by userid(%s)", szCallee);
                         pstAgentNode = sc_agent_get_by_sip_acc(szCallee);
                         if (DOS_ADDR_VALID(pstAgentNode)
                             && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo)
                             && AGENT_BIND_SIP == pstAgentNode->pstAgentInfo->ucBindType)
                         {
                             pstSCB->stCorSwitchboard.pstAgentCallee = pstAgentNode;
                             ulForwardType = SC_SW_FORWARD_AGENT;
                             ulForwardID = pstAgentNode->pstAgentInfo->ulAgentID;

                             pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_AFTER_KEY;
                             pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                             pstSCB->ulCurrentSrv++;
                             pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                             pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                             pstSCB->stIncomingQueue.ulLegNo = pstSCB->stCorSwitchboard.ulCallingLegNo;
                             pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                             pstSCB->stIncomingQueue.ulQueueType = ulForwardType;
                             if (sc_cwq_add_call(pstSCB, ulForwardID, pstCallingLegCB->stCall.stNumInfo.szCallee,
                                 pstSCB->stIncomingQueue.ulQueueType, ulForwardType) != DOS_SUCC)
                             {
                                 /* 加入队列失败 */
                                 DOS_ASSERT(0);
                             }
                             else
                             {
                                 /* 放音提示客户等待 */
                                 pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                                 sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CALL_QUEUE_WAIT, 1, 0, 0);
                             }
                             break;
                         }
                         else
                         {
                            pstCalleeLegCB = sc_lcb_alloc();
                            if (DOS_ADDR_INVALID(pstCalleeLegCB))
                            {
                                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Alloc SCB fail.");

                                return DOS_FAIL;
                            }

                            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
                            pstCalleeLegCB->stCall.bValid = DOS_TRUE;
                            pstCalleeLegCB->stCall.ucStatus = SC_LEG_INIT;
                            pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;

                            /* 新LEG处理一下号码 */
                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);


                            pstSCB->stCorSwitchboard.ulCalleeLegNo = pstCalleeLegCB->ulCBNo;

                            sc_make_call2sip(pstSCB, pstCalleeLegCB);

                            pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_EXEC;

                            break;
                         }
                    }
                    else if (ulForwardType == SC_SW_FORWARD_TT)
                    {

                    }
                    else if (ulForwardType == SC_SW_FORWARD_TEL)
                    {

                    }
                    else if (ulForwardType == SC_SW_FORWARD_AGENT
                        || ulForwardType == SC_SW_FORWARD_AGENT_GROUP)
                    {
                         pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_AFTER_KEY;
                         pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                         pstSCB->ulCurrentSrv++;
                         pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                         pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                         pstSCB->stIncomingQueue.ulLegNo = pstSCB->stCorSwitchboard.ulCallingLegNo;
                         pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                         pstSCB->stIncomingQueue.ulQueueType = ulForwardType;
                         if (sc_cwq_add_call(pstSCB, ulForwardID, pstCallingLegCB->stCall.stNumInfo.szCallee,
                             pstSCB->stIncomingQueue.ulQueueType, ulForwardType) != DOS_SUCC)
                         {
                             /* 加入队列失败 */
                             DOS_ASSERT(0);
                         }
                         else
                         {
                             /* 放音提示客户等待 */
                             pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                             sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CALL_QUEUE_WAIT, 1, 0, 0);
                         }
                         break;
                    }

                case SC_SW_DIAL_EXTENSION:

                    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "switchboard dmtf lkey : %d ", lKey);

                    if (lKey < 0 || lKey > 9)
                    {
                       break;
                    }

                    //得到分机号码，入队

                    if (pstSCB->stCorSwitchboard.ucIndex < SC_NUM_LENGTH)
                    {
                        pstSCB->stCorSwitchboard.szExtensionNum[pstSCB->stCorSwitchboard.ucIndex++] = lKey + '0';
                    }

                    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "switchboard dmtf sip num : %s ", pstSCB->stCorSwitchboard.szExtensionNum);

                    //if (pstSCB->stCorSwitchboard.ucIndex >= pstSCB->stCorSwitchboard.ucExtensionNumLength)
                    if (pstSCB->stCorSwitchboard.ucIndex >= 4)
                    {
                        pstSCB->stCorSwitchboard.szExtensionNum[pstSCB->stCorSwitchboard.ucIndex] = '\0';
                        dos_snprintf(szCallee, sizeof(szCallee), pstSCB->stCorSwitchboard.szExtensionNum);
                        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "get the sip num switchboard dmtf sip num : %s ", szCallee);

                        pstUserIDNode = sc_sip_node_get_by_extension(pstSCB->stCorSwitchboard.pstSWPeriodNode->ulCustomerID, szCallee);

                        if (DOS_ADDR_INVALID(pstUserIDNode))
                        {
                            //没有这个节点
                            DOS_ASSERT(0);
                            return DOS_FAIL;
                        }
                       else if (!pstUserIDNode->enStatus == SC_SIP_STATUS_TYPE_REGISTER)
                       {
                            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "pstUserIDNode->enStatus %d ", pstUserIDNode->enStatus);
                            //没注册
                            DOS_ASSERT(0);
                            return DOS_FAIL;
                       }

                        pstAgentNode = sc_agent_get_by_sip_acc(pstSCB->stCorSwitchboard.szExtensionNum);
                        if (DOS_ADDR_VALID(pstAgentNode)
                            && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo)
                            && AGENT_BIND_SIP == pstAgentNode->pstAgentInfo->ucBindType)
                        {
                            pstSCB->stCorSwitchboard.pstAgentCallee = pstAgentNode;
                            ulForwardType = SC_SW_FORWARD_AGENT;
                            ulForwardID = pstAgentNode->pstAgentInfo->ulAgentID;

                            pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                            pstSCB->ulCurrentSrv++;
                            pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                            pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                            pstSCB->stIncomingQueue.ulLegNo = pstSCB->stCorSwitchboard.ulCallingLegNo;
                            pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                            if (sc_cwq_add_call(pstSCB, ulForwardID,
                                pstCallingLegCB->stCall.stNumInfo.szCallee, ulForwardType, ulForwardType) != DOS_SUCC)
                            {
                                /* 加入队列失败 */
                                DOS_ASSERT(0);
                            }
                            else
                            {
                                /* 放音提示客户等待 */
                                pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                                sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CALL_QUEUE_WAIT, 1, 0, 0);
                            }
                            break;
                        }
                        else
                        {

                            pstCalleeLegCB = sc_lcb_alloc();
                            if (DOS_ADDR_INVALID(pstCalleeLegCB))
                            {
                              sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Alloc SCB fail.");

                              return DOS_FAIL;
                            }

                            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
                            pstCalleeLegCB->stCall.bValid = DOS_TRUE;
                            pstCalleeLegCB->stCall.ucStatus = SC_LEG_INIT;
                            pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;

                            /* 新LEG处理一下号码 */
                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
                            dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);


                            pstSCB->stCorSwitchboard.ulCalleeLegNo = pstCalleeLegCB->ulCBNo;

                            sc_make_call2sip(pstSCB, pstCalleeLegCB);

                            pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_EXEC;

                            break;
                        }

                    }
                    break;

                default:
                    break;
            }
            break;
         case SC_COR_SWITCHBOARD_CONNECTED:
            /* 开启 接入码 业务 */
            if (!pstSCB->stAccessCode.stSCBTag.bValid)
            {
                /* 开启接入码业务 */
                if (pstDTMF->cDTMFVal != '*'
                    && pstDTMF->cDTMFVal != '#' )
                {
                    /* 第一个字符不是 '*' 或者 '#' 不保存  */
                    return DOS_SUCC;
                }

                /* 只有坐席对应的leg执行接入码业务 */
                if (pstDTMF->ulLegNo != pstSCB->stCorSwitchboard.ulCalleeLegNo)
                {
                    return DOS_SUCC;
                }

                pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
                pstSCB->stAccessCode.szDialCache[0] = '\0';
                pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
                if (DOS_ADDR_VALID(pstSCB->stCorSwitchboard.pstAgentCallee)
                     && DOS_ADDR_VALID(pstSCB->stCorSwitchboard.pstAgentCallee->pstAgentInfo))
                {
                    pstSCB->stAccessCode.ulAgentID = pstSCB->stCorSwitchboard.pstAgentCallee->pstAgentInfo->ulAgentID;
                }
                pstSCB->ulCurrentSrv++;
                pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;
            }
            break;
         default:
            break;
    }

    return DOS_SUCC;

proc_fail:
    return DOS_FAIL;
}


U32 sc_switchboard_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HUNGUP_ST *pstHungup         = NULL;
    SC_LEG_CB            *pstCallee         = NULL;
    SC_LEG_CB            *pstCalling        = NULL;
    SC_LEG_CB            *pstHungupLeg      = NULL;
    SC_LEG_CB            *pstOtherLeg       = NULL;
    SC_AGENT_NODE_ST     *pstAgentCall      = NULL;
    SC_AGENT_NODE_ST     *pstAgentHungup    = NULL;
    S32                  lIndex             = 0;
    S32                  lRes               = DOS_FAIL;
    U32                  ulErrCode;

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHungup) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulErrCode = pstHungup->ulErrCode;
    sc_trace_scb(pstSCB, "Leg %u has hungup. Legs:%u-%u, status : %u", pstHungup->ulLegNo,
        pstSCB->stCorSwitchboard.ulCalleeLegNo, pstSCB->stCorSwitchboard.ulCallingLegNo, pstSCB->stCorSwitchboard.stSCBTag.usStatus);

    switch (pstSCB->stCorSwitchboard.stSCBTag.usStatus)
    {
        case SC_COR_SWITCHBOARD_IDEL:
        case SC_COR_SWITCHBOARD_PLAY_AUDIO:
        case SC_COR_SWITCHBOARD_AFTER_KEY:       /*按键之后操作*/
        case SC_COR_SWITCHBOARD_AUTH2:           /*认证被叫*/
            pstCalling = sc_lcb_get(pstSCB->stCorSwitchboard.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                pstCalling->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalling, NULL, SC_CALLEE);
                sc_lcb_free(pstCalling);
                pstCalling = NULL;
            }
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_COR_SWITCHBOARD_EXEC:            /*认证成功，开始呼叫被叫*/
            /* 这个地方应该是被叫异常了 */
            pstCalling = sc_lcb_get(pstSCB->stCorSwitchboard.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                pstCalling->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalling, NULL, SC_CALLEE);
                if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCorSwitchboard.ulCallingLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                {
                    sc_lcb_free(pstCalling);
                    pstCalling = NULL;

                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                }
            }

            if (DOS_ADDR_VALID(pstSCB))
            {
                pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_RELEASE;
            }
            break;

        case SC_COR_SWITCHBOARD_ALERTING:
            pstCallee = sc_lcb_get(pstSCB->stCorSwitchboard.ulCalleeLegNo);
            pstCalling = sc_lcb_get(pstSCB->stCorSwitchboard.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstCallee)
                || DOS_ADDR_INVALID(pstCalling))
            {
                if (DOS_ADDR_VALID(pstCalling))
                {
                    pstCalling->stCall.ulCause = ulErrCode;
                    sc_send_billing_stop2bs(pstSCB, pstCalling, NULL, SC_CALLEE);
                    sc_lcb_free(pstCalling);
                    sc_scb_free(pstSCB);
                    break;
                }
                /* 错误处理 */
            }

            if (pstCallee->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCallee->ulOtherSCBNo);
                pstCallee->ulOtherSCBNo = U32_BUTT;
            }

            if (pstCalling->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCalling->ulOtherSCBNo);
                pstCalling->ulOtherSCBNo = U32_BUTT;
            }

            if (pstSCB->stCorSwitchboard.ulCallingLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCalling;
                pstOtherLeg = pstCallee;
                pstAgentCall = pstSCB->stCorSwitchboard.pstAgentCallee;
            }
            else
            {
                pstHungupLeg = pstCallee;
                pstOtherLeg = pstCalling;
                pstAgentHungup = pstSCB->stCorSwitchboard.pstAgentCallee;
            }

            /* 生成话单 */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            pstHungupLeg->stCall.ulCause = ulErrCode;
            sc_send_billing_stop2bs(pstSCB, pstHungupLeg, NULL, SC_CALLEE);

            /* 到这里，说明两个leg都OK */
            /*
              * 需要看看是否长签等问题，如果主/被叫LEG都长签了，需要申请SCB，将被叫LEG挂到新的SCB中
              * 否则，将需要长签的LEG作为当前业务控制块的主叫LEG，挂断另外一条LEG
              * 可能需要处理客户标记
              */
            /* release 时，肯定是有一条leg hungup了，现在的leg需要释放掉，判断另一条是不是坐席长签，如果不是需要挂断 */
            /* 判断是否需要进行，客户标记。1、是客户一端先挂断的(基础呼叫中，客户只能是PSTN，坐席只能是SIP) */
            if ((pstHungupLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND
                || pstHungupLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                && DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && pstAgentCall->pstAgentInfo->ucProcesingTime != 0
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                /* 客户标记 */
                pstSCB->stMarkCustom.stSCBTag.bValid = DOS_TRUE;
                pstSCB->stMarkCustom.ulLegNo = pstOtherLeg->ulCBNo;
                pstSCB->stMarkCustom.pstAgentCall = pstAgentCall;
                pstSCB->ulCurrentSrv++;
                pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stMarkCustom.stSCBTag;

                if (pstOtherLeg->ulIndSCBNo == U32_BUTT)
                {
                    /* 非长签时，要把坐席对应的leg的结束时间，赋值给开始时间，出标记话单时使用 */
                    pstOtherLeg->stCall.stTimeInfo.ulAnswerTime = pstHungupLeg->stCall.stTimeInfo.ulByeTime;
                    for (lIndex=0; lIndex<SC_MAX_SERVICE_TYPE; lIndex++)
                    {
                        pstSCB->aucServType[lIndex] = 0;
                    }

                    if (pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);
                    }
                    else if(pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                    }
                    else
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
                    }

                    /* 将客户的号码改为主叫号码 */
                    dos_strcpy(pstOtherLeg->stCall.stNumInfo.szOriginalCalling, pstAgentCall->pstAgentInfo->szLastCustomerNum);
                }

                /* 修改坐席状态为 proc，播放 标记背景音 */
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_PROC, SC_SRV_CALL);
                sc_req_play_sound(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, SC_SND_CALL_OVER, 1, 0, 0);
                pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_IDEL;

                /* 开启定时器 */
                lRes = dos_tmr_start(&pstSCB->stMarkCustom.stTmrHandle, pstAgentCall->pstAgentInfo->ucProcesingTime * 1000, sc_agent_mark_custom_callback, (U64)pstOtherLeg->ulCBNo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    pstSCB->stMarkCustom.stTmrHandle = NULL;
                }

                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;

                if (pstSCB->stCorSwitchboard.ulCalleeLegNo == pstHungup->ulLegNo)
                {
                    pstSCB->stCorSwitchboard.ulCalleeLegNo = U32_BUTT;
                }
                else
                {
                    pstSCB->stCorSwitchboard.ulCallingLegNo = U32_BUTT;
                }

                pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_PROCESS;

                break;
            }

            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                pstAgentCall->pstAgentInfo->bMarkCustomer = DOS_FALSE;
            }

            /* 不需要客户标记 */
            if (pstSCB->stCorSwitchboard.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstSCB->stCorSwitchboard.ulCalleeLegNo = U32_BUTT;
            }
            else
            {
                pstSCB->stCorSwitchboard.ulCallingLegNo = U32_BUTT;
            }

            /* 如果是坐席先挂断的，判断坐席是否是长签的 */
            if (DOS_ADDR_VALID(pstAgentHungup)
                && DOS_ADDR_VALID(pstAgentHungup->pstAgentInfo)
                && pstAgentHungup->pstAgentInfo->bNeedConnected)
            {
                pstHungupLeg->ulSCBNo = U32_BUTT;
            }

            /* 修改坐席的状态 */
            if (DOS_ADDR_VALID(pstSCB->stCorSwitchboard.pstAgentCallee)
                && DOS_ADDR_VALID(pstSCB->stCorSwitchboard.pstAgentCallee->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstSCB->stCorSwitchboard.pstAgentCallee->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_COR_SWITCHBOARD);
                if (!pstSCB->stCorSwitchboard.pstAgentCallee->pstAgentInfo->bNeedConnected)
                {
                    pstSCB->stCorSwitchboard.pstAgentCallee->pstAgentInfo->ulLegNo = U32_BUTT;
                }
            }

            if (pstOtherLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签，继续放音 */
                pstOtherLeg->ulSCBNo = U32_BUTT;
                sc_req_play_sound(pstOtherLeg->ulIndSCBNo, pstOtherLeg->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                /* 释放掉 SCB */
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
            else if (pstHungupLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签的坐席挂断了电话，解除关系 */
                if (pstHungupLeg == pstCallee)
                {
                    pstSCB->stCorSwitchboard.ulCalleeLegNo = U32_BUTT;
                }
                else
                {
                    pstSCB->stCorSwitchboard.ulCallingLegNo = U32_BUTT;
                }
                sc_req_playback_stop(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo);
                sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_RELEASE;
            }
            else
            {
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_req_playback_stop(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo);
                sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_RELEASE;
            }

            break;
        case SC_COR_SWITCHBOARD_CONNECTED:
            /* 这个时候挂断，就是正常释放的节奏，处理完就好 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent connected.");

            pstCalling = sc_lcb_get(pstSCB->stCorSwitchboard.ulCallingLegNo);
            pstCallee = sc_lcb_get(pstSCB->stCorSwitchboard.ulCalleeLegNo);

            if (DOS_ADDR_INVALID(pstCalling) || DOS_ADDR_INVALID(pstCallee))
            {
                /* 异常 */
                DOS_ASSERT(0);
                if (DOS_ADDR_VALID(pstCalling))
                {
                    sc_lcb_free(pstCalling);
                }

                if (DOS_ADDR_VALID(pstCallee))
                {
                    sc_lcb_free(pstCallee);
                }

                sc_scb_free(pstSCB);
                pstSCB = NULL;
                break;
            }

            if (pstSCB->stCorSwitchboard.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCallee;
                pstOtherLeg  = pstCalling;
                pstCalling->stCall.stTimeInfo.ulByeTime = pstCallee->stCall.stTimeInfo.ulByeTime;
            }
            else
            {
                pstHungupLeg = pstCalling;
                pstOtherLeg  = pstCallee;
                pstCallee->stCall.stTimeInfo.ulByeTime = pstCalling->stCall.stTimeInfo.ulByeTime;
            }

            if (pstCallee->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCallee->ulOtherSCBNo);
                pstCallee->ulOtherSCBNo = U32_BUTT;
            }

            if (pstCalling->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCalling->ulOtherSCBNo);
                pstCalling->ulOtherSCBNo = U32_BUTT;
            }

            /* 生成话单 */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            if (sc_scb_is_exit_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                /* 如果有出局呼叫，应该先将出局呼叫删除 */
                sc_scb_remove_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCallee->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallee, NULL, SC_CALLEE);
                /* 出局呼叫的话单应该用坐席那条leg产生 */
                sc_scb_remove_service(pstSCB, BS_SERV_PREVIEW_DIALING);
                sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCalling->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalling, NULL, SC_CALLEE);
            }
            else
            {
                pstCallee->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallee, NULL, SC_CALLEE);
            }

            /* 到这里，说明两个leg都OK */
            /*
              * 需要看看是否长签等问题，如果主/被叫LEG都长签了，需要申请SCB，将被叫LEG挂到新的SCB中
              * 否则，将需要长签的LEG作为当前业务控制块的主叫LEG，挂断另外一条LEG
              * 可能需要处理客户标记
              */
            /* release 时，肯定是有一条leg hungup了，现在的leg需要释放掉，判断另一条是不是坐席长签，如果不是需要挂断 */
            pstAgentCall = pstSCB->stCorSwitchboard.pstAgentCallee;
            if (DOS_ADDR_INVALID(pstAgentCall) || DOS_ADDR_INVALID(pstAgentCall->pstAgentInfo))
            {
                /* 没有找到坐席 */
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Can not found agent by id(%u)", pstSCB->stPreviewCall.ulAgentID);
            }

            /* 判断是否需要进行，客户标记。1、是客户一端先挂断的(基础呼叫中，客户只能是PSTN，坐席只能是SIP) */
            if (pstHungupLeg == pstCalling
                && DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && pstAgentCall->pstAgentInfo->ucProcesingTime != 0
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                /* 客户标记 */
                pstSCB->stMarkCustom.stSCBTag.bValid = DOS_TRUE;
                pstSCB->stMarkCustom.ulLegNo = pstOtherLeg->ulCBNo;
                pstSCB->stMarkCustom.pstAgentCall = pstAgentCall;
                pstSCB->ulCurrentSrv++;
                pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stMarkCustom.stSCBTag;

                if (pstOtherLeg->ulIndSCBNo == U32_BUTT)
                {
                    /* 非长签时，要把坐席对应的leg的结束时间，赋值给开始时间，出标记话单时使用 */
                    pstOtherLeg->stCall.stTimeInfo.ulAnswerTime = pstHungupLeg->stCall.stTimeInfo.ulByeTime;
                    for (lIndex=0; lIndex<SC_MAX_SERVICE_TYPE; lIndex++)
                    {
                        pstSCB->aucServType[lIndex] = 0;
                    }

                    if (pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);
                    }
                    else if(pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                    }
                    else
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
                    }

                    /* 将客户的号码改为主叫号码 */
                    dos_strcpy(pstOtherLeg->stCall.stNumInfo.szOriginalCalling, pstAgentCall->pstAgentInfo->szLastCustomerNum);
                }

                /* 修改坐席状态为 proc，播放 标记背景音 */
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_PROC, SC_SRV_PREVIEW_CALL);
                sc_req_play_sound(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, SC_SND_CALL_OVER, 1, 0, 0);
                pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_IDEL;

                /* 开启定时器 */
                lRes = dos_tmr_start(&pstSCB->stMarkCustom.stTmrHandle, pstAgentCall->pstAgentInfo->ucProcesingTime * 1000, sc_agent_mark_custom_callback, (U64)pstOtherLeg->ulCBNo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    pstSCB->stMarkCustom.stTmrHandle = NULL;
                }

                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;

                if (pstSCB->stCorSwitchboard.ulCalleeLegNo == pstHungup->ulLegNo)
                {
                    pstSCB->stCorSwitchboard.ulCalleeLegNo = U32_BUTT;
                }
                else
                {
                    pstSCB->stCorSwitchboard.ulCallingLegNo = U32_BUTT;
                }

                pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_PROCESS;

                break;
            }

            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                pstAgentCall->pstAgentInfo->bMarkCustomer = DOS_FALSE;
            }

            /* 不需要客户标记 */
            if (pstSCB->stCorSwitchboard.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstSCB->stCorSwitchboard.ulCalleeLegNo = U32_BUTT;
            }
            else
            {
                pstSCB->stCorSwitchboard.ulCallingLegNo = U32_BUTT;
            }

            /* 修改坐席的状态 */
            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_PREVIEW_CALL);
            }

            if (pstOtherLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签，继续放音 */
                pstOtherLeg->ulSCBNo = U32_BUTT;
                sc_req_play_sound(pstOtherLeg->ulIndSCBNo, pstOtherLeg->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                /* 释放掉 SCB */
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
            else if (pstHungupLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签的坐席挂断了电话，不要释放leg，解除关系就行 */
                pstHungupLeg->ulSCBNo = U32_BUTT;
                pstSCB->stCorSwitchboard.ulCallingLegNo = U32_BUTT;
                pstHungupLeg->ulCBNo = U32_BUTT;
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_RELEASE;
            }
            else
            {
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stCorSwitchboard.stSCBTag.usStatus = SC_COR_SWITCHBOARD_RELEASE;
            }

            break;

        case SC_COR_SWITCHBOARD_PROCESS:
            pstCallee = sc_lcb_get(pstSCB->stCorSwitchboard.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCallee))
            {
                sc_lcb_free(pstCallee);
                pstCallee = NULL;
            }

            pstCalling = sc_lcb_get(pstSCB->stCorSwitchboard.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                sc_lcb_free(pstCalling);
                pstCalling = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_COR_SWITCHBOARD_RELEASE:
            pstCallee = sc_lcb_get(pstSCB->stCorSwitchboard.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCallee))
            {
                sc_lcb_free(pstCallee);
                pstCallee = NULL;
            }

            pstCalling = sc_lcb_get(pstSCB->stCorSwitchboard.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                sc_lcb_free(pstCalling);
                pstCalling = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;
        default:
            DOS_ASSERT(0);
            break;
    }

    sc_trace_scb(pstSCB, "Leg %u has hunguped. ", pstHungup->ulLegNo);

    return DOS_SUCC;
}


U32 sc_switchboard_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;
    SC_LEG_CB                   *pstCalleeLegCB     = NULL;
    SC_LEG_CB                   *pstCallingLegCB    = NULL;
    SC_LEG_CB                   *pstRecordLegCB     = NULL;
    SC_MSG_CMD_RECORD_ST        stRecordRsp;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing switchboard error event. status : %u", pstSCB->stCorSwitchboard.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */
        if (!sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
        {
            return DOS_SUCC;
        }

        pstCalleeLegCB = sc_lcb_get(pstSCB->stCorSwitchboard.ulCalleeLegNo);
        if (pstCalleeLegCB->stRecord.bValid)
        {
            pstRecordLegCB = pstCalleeLegCB;
        }
        else
        {
            pstCallingLegCB = sc_lcb_get(pstSCB->stCorSwitchboard.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingLegCB) && pstCallingLegCB->stRecord.bValid)
            {
                pstRecordLegCB = pstCallingLegCB;
            }
        }

        if (DOS_ADDR_VALID(pstRecordLegCB))
        {
            stRecordRsp.stMsgTag.ulMsgType = SC_CMD_RECORD;
            stRecordRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stRecordRsp.stMsgTag.usInterErr = 0;
            stRecordRsp.ulSCBNo = pstSCB->ulSCBNo;
            stRecordRsp.ulLegNo = pstRecordLegCB->ulCBNo;

            if (sc_send_cmd_record(&stRecordRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_INFO, SC_MOD_EVENT, SC_LOG_DISIST), "Send record cmd FAIL! SCBNo : %u", pstSCB->ulSCBNo);
            }
        }

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stCorSwitchboard.stSCBTag.usStatus)
    {
        case SC_COR_SWITCHBOARD_IDEL:
        case SC_COR_SWITCHBOARD_PLAY_AUDIO:
            ulRet = sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stCorSwitchboard.ulCallingLegNo, ulErrCode);
            break;

        case SC_COR_SWITCHBOARD_AFTER_KEY:       /*按键之后操作*/
        case SC_COR_SWITCHBOARD_AUTH2:           /*认证被叫*/
        case SC_COR_SWITCHBOARD_EXEC:            /*认证成功，开始呼叫被叫*/
        case SC_COR_SWITCHBOARD_PROC:
        case SC_COR_SWITCHBOARD_ALERTING:        /*被叫开始振铃*/
        case SC_COR_SWITCHBOARD_CONNECTED:       /*通话*/
            /* 呼叫被叫时失败，给主叫放提示音挂断 */
            sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stCorSwitchboard.ulCallingLegNo);
            ulRet = sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stCorSwitchboard.ulCallingLegNo, ulErrCode);
            break;

        case SC_COR_SWITCHBOARD_PROCESS:
        case SC_COR_SWITCHBOARD_RELEASE:
            if (pstSCB->stCorSwitchboard.ulCalleeLegNo != U32_BUTT)
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCorSwitchboard.ulCalleeLegNo, ulErrCode);
            }
            else
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCorSwitchboard.ulCallingLegNo, ulErrCode);
            }
            break;

        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed switchboard error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}


U32 sc_interception_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                         ulRet       = DOS_FAIL;
    SC_LEG_CB                   *pstLCB     = NULL;
    SC_MSG_EVT_AUTH_RESULT_ST   *pstAuthRsp = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing interception auth event. status : %u", pstSCB->stInterception.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;
    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_ERROR, SC_MOD_EVENT, SC_LOG_DISIST), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        sc_log_digest_print_only(pstSCB, "Processing auth rsp event for interception. error : %u", pstAuthRsp->stMsgTag.usInterErr);
        ulRet = DOS_FAIL;

        goto proc_finishe;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
            ulRet = DOS_SUCC;
            break;

        case SC_INTERCEPTION_AUTH:
            switch (pstLCB->stCall.ucPeerType)
            {
                case SC_LEG_PEER_OUTBOUND:
                    ulRet = sc_make_call2pstn(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_TT:
                    ulRet = sc_make_call2eix(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_INTERNAL:
                    ulRet = sc_make_call2sip(pstSCB, pstLCB);
                    break;

                default:
                    sc_trace_scb(pstSCB, "Invalid perr type. %u", pstLCB->stCall.ucPeerType);
                    goto proc_finishe;
                    break;
            }

            pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_EXEC;
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
        case SC_INTERCEPTION_ACTIVE:
        case SC_INTERCEPTION_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard auth event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }

        if (pstSCB)
        {
            sc_scb_free(pstSCB);
        }
    }

    return ulRet;
}

U32 sc_interception_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing interception call setup event event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
        case SC_INTERCEPTION_AUTH:
            /* 未认证通过，放音挂断呼叫 */
            goto proc_finishe;
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
            /* 迁移状态到proc */
            pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_INTERCEPTION_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_INTERCEPTION_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;

}

U32 sc_interception_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet   = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception ringing event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
            break;

        case SC_INTERCEPTION_AUTH:
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
            pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_ALERTING;
            ulRet = DOS_SUCC;
            break;

        case SC_INTERCEPTION_ACTIVE:
            break;

        case SC_INTERCEPTION_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_interception_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB      = NULL;
    SC_LEG_CB  *pstAgentLCB = NULL;
    U32        ulRet        = DOS_FAIL;
    SC_MSG_CMD_MUX_ST stInterceptRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception answer event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    pstAgentLCB = sc_lcb_get(pstSCB->stInterception.ulAgentLegNo);
    if (DOS_ADDR_INVALID(pstAgentLCB))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
            break;

        case SC_INTERCEPTION_AUTH:
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
            pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_ACTIVE;

            stInterceptRsp.stMsgTag.ulMsgType = SC_CMD_MUX;
            stInterceptRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stInterceptRsp.stMsgTag.usInterErr = 0;

            stInterceptRsp.ulMode = SC_MUX_CMD_INTERCEPT;
            stInterceptRsp.ulSCBNo = pstSCB->ulSCBNo;
            stInterceptRsp.ulLegNo = pstLCB->ulCBNo;
            stInterceptRsp.ulAgentLegNo = pstAgentLCB->ulCBNo;

            if (sc_send_cmd_mux(&stInterceptRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                goto proc_finishe;
            }

            if (DOS_ADDR_VALID(pstSCB->stInterception.pstAgentInfo)
                && DOS_ADDR_VALID(pstSCB->stInterception.pstAgentInfo->pstAgentInfo))
            {
                pstSCB->stInterception.pstAgentInfo->pstAgentInfo->bIsInterception = DOS_TRUE;
                sc_agent_update_status_db(pstSCB->stInterception.pstAgentInfo->pstAgentInfo);
            }

            ulRet = DOS_SUCC;

            break;

        case SC_INTERCEPTION_ACTIVE:
            break;

        case SC_INTERCEPTION_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_interception_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB               *pstLCB         = NULL;
    SC_LEG_CB               *pstAgentLCB    = NULL;
    U32                     ulRet           = DOS_FAIL;
    SC_MSG_EVT_HUNGUP_ST    *pstHungup      = NULL;
    U32                     ulErrCode;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    ulErrCode = pstHungup->ulErrCode;

    sc_trace_scb(pstSCB, "Processing interception release event. status : %u", pstSCB->stInterception.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Processing interception release event. status : %u", pstSCB->stInterception.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        return DOS_FAIL;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
        case SC_INTERCEPTION_AUTH:
        case SC_INTERCEPTION_EXEC:
            break;

        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
        case SC_INTERCEPTION_ACTIVE:
        case SC_INTERCEPTION_RELEASE:
            /* 发送话单 */
            pstLCB->stCall.ulCause = ulErrCode;
            sc_send_billing_stop2bs(pstSCB, pstLCB, NULL, SC_CALLEE);

            pstAgentLCB = sc_lcb_get(pstSCB->stInterception.ulAgentLegNo);
            if (DOS_ADDR_VALID(pstAgentLCB))
            {
                pstAgentLCB->ulOtherSCBNo = U32_BUTT;
            }

            if (DOS_ADDR_VALID(pstSCB->stInterception.pstAgentInfo)
                && DOS_ADDR_VALID(pstSCB->stInterception.pstAgentInfo->pstAgentInfo))
            {
                pstSCB->stInterception.pstAgentInfo->pstAgentInfo->bIsInterception = DOS_FALSE;
                sc_agent_update_status_db(pstSCB->stInterception.pstAgentInfo->pstAgentInfo);
            }

            sc_lcb_free(pstLCB);
            pstLCB = NULL;
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;

}

U32 sc_interception_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;
    SC_LEG_CB                   *pstCallingCB       = NULL;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing interception error event. status : %u", pstSCB->stInterception.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing interception error event. status : %u", pstSCB->stInterception.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
        case SC_INTERCEPTION_AUTH:
        case SC_INTERCEPTION_EXEC:
            /* 发起呼叫失败，直接释放资源 */
            pstCallingCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
            }
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
        case SC_INTERCEPTION_ACTIVE:
        case SC_INTERCEPTION_RELEASE:
            ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stInterception.ulLegNo, ulErrCode);
            break;
        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_whisper_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                         ulRet       = DOS_FAIL;
    SC_LEG_CB                   *pstLCB     = NULL;
    SC_MSG_EVT_AUTH_RESULT_ST   *pstAuthRsp = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing whisper auth event. status : %u", pstSCB->stWhispered.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;
    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_ERROR, SC_MOD_EVENT, SC_LOG_DISIST), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        sc_log_digest_print_only(pstSCB, "Processing auth rsp event for interception. error : %u", pstAuthRsp->stMsgTag.usInterErr);
        ulRet = DOS_FAIL;

        goto proc_finishe;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
            ulRet = DOS_SUCC;
            break;

        case SC_WHISPER_AUTH:
            switch (pstLCB->stCall.ucPeerType)
            {
                case SC_LEG_PEER_OUTBOUND:
                    ulRet = sc_make_call2pstn(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_TT:
                    ulRet = sc_make_call2eix(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_INTERNAL:
                    ulRet = sc_make_call2sip(pstSCB, pstLCB);
                    break;

                default:
                    sc_trace_scb(pstSCB, "Invalid perr type. %u", pstLCB->stCall.ucPeerType);
                    goto proc_finishe;
                    break;
            }

            pstSCB->stWhispered.stSCBTag.usStatus = SC_WHISPER_EXEC;
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
        case SC_WHISPER_ACTIVE:
        case SC_WHISPER_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard auth event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }

        if (DOS_ADDR_VALID(pstSCB))
        {
            sc_scb_free(pstSCB);
            pstSCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_whisper_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing interception call setup event event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
        case SC_WHISPER_AUTH:
            /* 未认证通过，放音挂断呼叫 */
            goto proc_finishe;
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
            /* 迁移状态到proc */
            pstSCB->stWhispered.stSCBTag.usStatus = SC_WHISPER_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_WHISPER_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_WHISPER_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_whisper_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet   = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception ringing event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
            break;

        case SC_WHISPER_AUTH:
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
            pstSCB->stWhispered.stSCBTag.usStatus = SC_WHISPER_ALERTING;
            ulRet = DOS_SUCC;
            break;

        case SC_WHISPER_ACTIVE:
            break;

        case SC_WHISPER_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_whisper_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB      = NULL;
    SC_LEG_CB  *pstAgentLCB = NULL;
    U32        ulRet        = DOS_FAIL;
    SC_MSG_CMD_MUX_ST stInterceptRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception answer event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    pstAgentLCB = sc_lcb_get(pstSCB->stWhispered.ulAgentLegNo);
    if (DOS_ADDR_INVALID(pstAgentLCB))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
            break;

        case SC_WHISPER_AUTH:
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
            pstSCB->stWhispered.stSCBTag.usStatus = SC_WHISPER_ACTIVE;

            stInterceptRsp.stMsgTag.ulMsgType = SC_CMD_MUX;
            stInterceptRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stInterceptRsp.stMsgTag.usInterErr = 0;

            stInterceptRsp.ulMode = SC_MUX_CMD_WHISPER;
            stInterceptRsp.ulSCBNo = pstSCB->ulSCBNo;
            stInterceptRsp.ulLegNo = pstLCB->ulCBNo;
            stInterceptRsp.ulAgentLegNo = pstAgentLCB->ulCBNo;

            if (sc_send_cmd_mux(&stInterceptRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                goto proc_finishe;
            }

            if (DOS_ADDR_VALID(pstSCB->stWhispered.pstAgentInfo)
                && DOS_ADDR_VALID(pstSCB->stWhispered.pstAgentInfo->pstAgentInfo))
            {
                pstSCB->stWhispered.pstAgentInfo->pstAgentInfo->bIsWhisper = DOS_TRUE;
                sc_agent_update_status_db(pstSCB->stWhispered.pstAgentInfo->pstAgentInfo);
            }

            ulRet = DOS_SUCC;

            break;

        case SC_WHISPER_ACTIVE:
            break;

        case SC_WHISPER_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_whisper_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB               *pstLCB         = NULL;
    SC_LEG_CB               *pstAgentLCB    = NULL;
    U32                     ulRet           = DOS_FAIL;
    SC_MSG_EVT_HUNGUP_ST    *pstHungup      = NULL;
    U32                     ulErrCode;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    ulErrCode = pstHungup->ulErrCode;

    sc_trace_scb(pstSCB, "Processing whisper release event. status : %u", pstSCB->stWhispered.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Processing whisper release event. status : %u", pstSCB->stWhispered.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        return DOS_FAIL;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
            break;

        case SC_WHISPER_AUTH:
        case SC_WHISPER_EXEC:
            break;

        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
        case SC_WHISPER_ACTIVE:
        case SC_WHISPER_RELEASE:
            /* 发送话单 */
            pstLCB->stCall.ulCause = ulErrCode;
            sc_send_billing_stop2bs(pstSCB, pstLCB, NULL, SC_CALLEE);

            if (DOS_ADDR_VALID(pstSCB->stWhispered.pstAgentInfo)
                && DOS_ADDR_VALID(pstSCB->stWhispered.pstAgentInfo->pstAgentInfo))
            {
                pstSCB->stWhispered.pstAgentInfo->pstAgentInfo->bIsWhisper = DOS_FALSE;
                sc_agent_update_status_db(pstSCB->stWhispered.pstAgentInfo->pstAgentInfo);
            }

            pstAgentLCB = sc_lcb_get(pstSCB->stWhispered.ulAgentLegNo);
            if (DOS_ADDR_VALID(pstAgentLCB))
            {
                pstAgentLCB->ulOtherSCBNo = U32_BUTT;
            }

            sc_lcb_free(pstLCB);
            pstLCB = NULL;
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;

}

U32 sc_whisper_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;
    SC_LEG_CB                   *pstCallingCB       = NULL;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing whisper error event. status : %u", pstSCB->stWhispered.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing whisper error event. status : %u", pstSCB->stWhispered.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
        case SC_WHISPER_AUTH:
        case SC_WHISPER_EXEC:
            /* 发起呼叫失败，直接释放资源 */
            pstCallingCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
            }
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
        case SC_WHISPER_ACTIVE:
        case SC_WHISPER_RELEASE:
            ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stWhispered.ulLegNo, ulErrCode);
            break;

        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_auto_call_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB = NULL;
    SC_LEG_CB                  *pstCalleeLegCB = NULL;
    U32                         ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call auth event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_ERROR, SC_MOD_EVENT, SC_LOG_DISIST), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* 注意通过偏移量，找到CC统一定义的错误码 */

        /* 分析呼叫结果 */
        sc_task_call_result(pstSCB, pstSCB->stAutoCall.ulCallingLegNo, pstAuthRsp->stMsgTag.usInterErr + CC_ERR_BS_HEAD, pstSCB->stAutoCall.stSCBTag.usStatus);

        pstLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
        if (DOS_ADDR_VALID(pstLegCB))
        {
            sc_lcb_free(pstLegCB);
        }
        sc_scb_free(pstSCB);
        pstSCB = NULL;
        return DOS_SUCC;
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_AUTH:
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_EXEC;
            pstLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstLegCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
                DOS_ASSERT(0);

                return DOS_FAIL;
            }
            ulRet = sc_make_call2pstn(pstSCB, pstLegCB);
            if (ulRet != DOS_SUCC)
            {
                /* 发起呼叫失败 */
                sc_task_call_result(pstSCB, pstSCB->stAutoCall.ulCallingLegNo, pstLegCB->stCall.ulCause, pstSCB->stAutoCall.stSCBTag.usStatus);
                sc_lcb_free(pstLegCB);
                pstLegCB = NULL;
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
            break;

        case SC_AUTO_CALL_AUTH2:
            /* 呼叫坐席时，进行的认证，呼叫坐席 */
            pstCalleeLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCalleeLegCB))
            {
                /* TODO */
                return DOS_FAIL;
            }
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_EXEC2;
            ulRet = sc_make_call2pstn(pstSCB, pstCalleeLegCB);
            if (ulRet != DOS_SUCC)
            {
                /* 挂断 客户 */
                sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, pstCalleeLegCB->stCall.ulCause);
            }

            break;

        default:
            break;
    }

    return ulRet;
}

U32 sc_auto_call_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call stup event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
            /* 未认证通过，放音挂断呼叫 */
            goto proc_finishe;
            break;

        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PROC:
        case SC_AUTO_CALL_ALERTING:
            /* 迁移状态到proc */
            sc_task_concurrency_add(pstSCB->stAutoCall.ulTcbID);
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_AUTO_CALL_ACTIVE:
        case SC_AUTO_CALL_AFTER_KEY:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_EXEC2:
            /* 坐席的leg创建 */
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_PORC2;
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_auto_call_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32         ulRet   = DOS_FAIL;
    S32         lRes    = DOS_FAIL;
    SC_MSG_EVT_RINGING_ST *pstEvent = NULL;
    SC_LEG_CB   *pstCalleeLegCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEvent = (SC_MSG_EVT_RINGING_ST *)pstMsg;

    sc_trace_scb(pstSCB, "Processing auto call ringing event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Processing auto call ringing event. status : %u. WithMedia: %u", pstSCB->stAutoCall.stSCBTag.usStatus, pstEvent->ulWithMedia);

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
            /* 未认证通过，放音挂断呼叫 */
            goto proc_finishe;
            break;

        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PROC:
        case SC_AUTO_CALL_ALERTING:
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_ALERTING;
            if (DOS_ADDR_INVALID(pstSCB->stAutoCall.stCusTmrHandle))
            {
                lRes = dos_tmr_start(&pstSCB->stAutoCall.stCusTmrHandle, SC_AUTO_CALL_RINGING_TIMEOUT, sc_auto_call_ringing_timeout_callback, (U64)pstEvent->ulLegNo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    pstSCB->stAutoCall.stCusTmrHandle = NULL;
                }
            }
            break;

        case SC_AUTO_CALL_ACTIVE:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_AFTER_KEY:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_PORC2:
        case SC_AUTO_CALL_ALERTING2:
            /* 坐席振铃, 给客户放回铃音 */
            sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo);
            sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, DOS_TRUE, pstEvent->ulWithMedia);

            if (pstEvent->ulWithMedia)
            {
                pstCalleeLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
                if (DOS_ADDR_VALID(pstCalleeLegCB))
                {
                    pstCalleeLegCB->stCall.bEarlyMedia = DOS_TRUE;

                    if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCalleeLegNo, pstSCB->stAutoCall.ulCallingLegNo) != DOS_SUCC)
                    {
                        sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                        ulRet = DOS_FAIL;
                        goto proc_finishe;
                    }
                }
            }

            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_ALERTING2;

            if (pstSCB->stAutoCall.bIsRingTimer)
            {
                /* 开启定时器 */
                sc_trace_scb(pstSCB, "%s", "Start ringting timer.");
                lRes = dos_tmr_start(&pstSCB->stAutoCall.stAgentTmrHandle, SC_AGENT_RINGING_TIMEOUT, sc_agent_ringing_timeout_callback, (U64)pstEvent->ulLegNo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    pstSCB->stAutoCall.stAgentTmrHandle = NULL;
                }
            }
            break;

        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto call ringing event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_auto_call_ringing_timeout(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_RINGING_TIMEOUT_ST   *pstEvtRingingTimeOut   = NULL;
    U32                 ulRet              = DOS_SUCC;

    pstEvtRingingTimeOut = (SC_MSG_EVT_RINGING_TIMEOUT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtRingingTimeOut) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call agent ringing timeout event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Processing auto call agent ringing timeout event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_ALERTING:
            /* 客户振铃超时，挂断客户电话 */
            sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo);
            ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, CC_ERR_SIP_BUSY_HERE);
            break;

        default:
            ulRet = sc_ringing_timeout_proc(pstSCB);
            break;

    }
    return ulRet;
}


void sc_auto_call_recall_agent_callback(U64 arg)
{
    U32                 ulScbNo    = U32_BUTT;
    SC_SRV_CB           *pstSCB    = NULL;

    ulScbNo = (U32)arg;

    pstSCB = sc_scb_get(ulScbNo);
    if (DOS_ADDR_INVALID(pstSCB))
    {
        return;
    }

    /* 呼叫坐席失败，重新加入队列 */
    sc_ringing_timeout_proc(pstSCB);

    return;
}

U32 sc_auto_call_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ANSWER_ST *pstEvtAnswer     = NULL;
    U32                 ulRet              = DOS_SUCC;
    U32                 ulTaskMode         = U32_BUTT;
    SC_LEG_CB           *pstLCB            = NULL;
    SC_LEG_CB           *pstCalleeLegCB    = NULL;
    SC_AGENT_NODE_ST    *pstAgentCallee    = NULL;
    SC_MSG_CMD_PLAYBACK_ST  stPlaybackRsp;
    U32          ulErrCode   = CC_ERR_NO_REASON;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call answer event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
            /* 未认证通过，放音挂断呼叫 */
            goto proc_finishe;
            break;

        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PROC:
        case SC_AUTO_CALL_ALERTING:
            /* 播放语音 */
            if (DOS_ADDR_VALID(pstSCB->stAutoCall.stCusTmrHandle))
            {
                dos_tmr_stop(&pstSCB->stAutoCall.stCusTmrHandle);
                pstSCB->stAutoCall.stCusTmrHandle = NULL;
            }

            ulTaskMode = sc_task_get_mode(pstSCB->stAutoCall.ulTcbID);
            if (ulTaskMode >= SC_TASK_MODE_BUTT)
            {
                DOS_ASSERT(0);

                ulErrCode = CC_ERR_SC_CONFIG_ERR;
                ulRet = DOS_FAIL;
                goto proc_finishe;
            }

            switch (ulTaskMode)
            {
                /* 需要放音的，统一先放音。在放音结束后请处理后续流程 */
                case SC_TASK_MODE_KEY4AGENT:
                case SC_TASK_MODE_KEY4AGENT1:
                case SC_TASK_MODE_AUDIO_ONLY:
                case SC_TASK_MODE_AGENT_AFTER_AUDIO:
                    stPlaybackRsp.stMsgTag.ulMsgType = SC_CMD_PLAYBACK;
                    stPlaybackRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
                    stPlaybackRsp.stMsgTag.usInterErr = 0;
                    stPlaybackRsp.ulMode = 0;
                    stPlaybackRsp.ulSCBNo = pstSCB->ulSCBNo;
                    stPlaybackRsp.ulLegNo = pstLCB->ulCBNo;
                    stPlaybackRsp.ulLoopCnt = sc_task_get_playcnt(pstSCB->stAutoCall.ulTcbID);
                    stPlaybackRsp.ulInterval = 0;
                    stPlaybackRsp.ulSilence  = 0;
                    stPlaybackRsp.enType = SC_CND_PLAYBACK_FILE;
                    stPlaybackRsp.blNeedDTMF = DOS_TRUE;
                    stPlaybackRsp.ulTotalAudioCnt++;
                    if (sc_task_get_audio_file(pstSCB->stAutoCall.ulTcbID) == NULL)
                    {
                        ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                        ulRet = DOS_FAIL;
                        goto proc_finishe;
                    }

                    dos_strncpy(stPlaybackRsp.szAudioFile, sc_task_get_audio_file(pstSCB->stAutoCall.ulTcbID), SC_MAX_AUDIO_FILENAME_LEN-1);
                    stPlaybackRsp.szAudioFile[SC_MAX_AUDIO_FILENAME_LEN - 1] = '\0';

                    if (sc_send_cmd_playback(&stPlaybackRsp.stMsgTag) != DOS_SUCC)
                    {
                        ulRet = DOS_FAIL;
                        ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                        goto proc_finishe;
                    }

                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_ACTIVE;
                    break;

                /* 直接接通坐席 */
                case SC_TASK_MODE_DIRECT4AGETN:
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_AFTER_KEY;
                    /* 开启呼入队列队列业务控制块 */
                    pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                    pstSCB->ulCurrentSrv++;
                    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                    pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                    pstSCB->stIncomingQueue.ulLegNo = pstSCB->stAutoCall.ulCallingLegNo;
                    pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                    pstSCB->stIncomingQueue.ulQueueType = SC_SW_FORWARD_AGENT_GROUP;
                    if (sc_cwq_add_call(pstSCB, sc_task_get_agent_queue(pstSCB->stAutoCall.ulTcbID), pstLCB->stCall.stNumInfo.szRealCallee, pstSCB->stIncomingQueue.ulQueueType, DOS_FALSE) != DOS_SUCC)
                    {
                        /* 加入队列失败 */
                        DOS_ASSERT(0);
                        ulRet = DOS_FAIL;
                    }
                    else
                    {
                        /* 放音提示客户等待 */
                        pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                        sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CALL_QUEUE_WAIT, 1, 0, 0);
                    }

                    break;
                default:
                    DOS_ASSERT(0);
                    ulRet = DOS_FAIL;
                    ulErrCode = CC_ERR_SC_CONFIG_ERR;
                    goto proc_finishe;
            }

            break;
        case SC_AUTO_CALL_ACTIVE:
        case SC_AUTO_CALL_AFTER_KEY:
            /* TODO */
            break;

        case SC_AUTO_CALL_PORC2:
        case SC_AUTO_CALL_ALERTING2:
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_CONNECTED;
            pstEvtAnswer = (SC_MSG_EVT_ANSWER_ST *)pstMsg;

            /* 关闭定时器 */
            if (DOS_ADDR_VALID(pstSCB->stAutoCall.stAgentTmrHandle))
            {
                dos_tmr_stop(&pstSCB->stAutoCall.stAgentTmrHandle);
                pstSCB->stAutoCall.stAgentTmrHandle = NULL;
            }
            /* 应答主叫 */
            sc_req_answer_call(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo);

            /* 连接主被叫 */
            pstCalleeLegCB = sc_lcb_get(pstEvtAnswer->ulLegNo);
            if (DOS_ADDR_VALID(pstCalleeLegCB)
                && !pstCalleeLegCB->stCall.bEarlyMedia)
            {
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCalleeLegNo, pstSCB->stAutoCall.ulCallingLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    ulRet = DOS_FAIL;
                    ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                    goto proc_finishe;
                }

                pstAgentCallee = sc_agent_get_by_id(pstSCB->stAutoCall.ulAgentID);
                if (DOS_ADDR_VALID(pstAgentCallee)
                    && DOS_ADDR_VALID(pstAgentCallee->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstAgentCallee->pstAgentInfo, SC_ACD_SERV_CALL_IN, SC_SRV_AUTO_CALL);
                }
            }
            break;

        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
        case SC_AUTO_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto call answer event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        /* 失败的处理 */
        sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo);
        ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, ulErrCode);
    }

    return ulRet;
}

U32 sc_auto_call_palayback_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* 判断一下群呼任务的模式，如果是呼叫后，转坐席，则转坐席，否则通话结束 */
    SC_LEG_CB              *pstLCB          = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstRlayback     = NULL;
    U32                    ulTaskMode       = U32_BUTT;
    U32                    ulErrCode        = CC_ERR_NO_REASON;
    U32                    ulRet            = DOS_SUCC;
    SC_AGENT_NODE_ST       *pstAgentCall    = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the auto call playback stop msg. status: %u", pstSCB->stAutoCall.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "process the auto call playback stop msg. status: %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstRlayback = (SC_MSG_EVT_PLAYBACK_ST *)pstMsg;

    if (pstRlayback->stMsgTag.usInterErr != U16_BUTT)
    {
        /* 挂断了，这里不用处理 */
        return DOS_SUCC;
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_ACTIVE:
            pstLCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstLCB))
            {
                sc_trace_scb(pstSCB, "There is no calling leg.");

                goto proc_finishe;
            }

            ulTaskMode = sc_task_get_mode(pstSCB->stAutoCall.ulTcbID);
            if (ulTaskMode >= SC_TASK_MODE_BUTT)
            {
                DOS_ASSERT(0);

                ulErrCode = CC_ERR_SC_CONFIG_ERR;
                ulRet = DOS_FAIL;
                goto proc_finishe;
            }

            switch (ulTaskMode)
            {
                /* 需要放音的，统一先放音。在放音结束后请处理后续流程 */
                case SC_TASK_MODE_AGENT_AFTER_AUDIO:
                    /* 转坐席 */
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_AFTER_KEY;
                    /* 开启呼入队列队列业务控制块 */
                    pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                    pstSCB->ulCurrentSrv++;
                    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                    pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                    pstSCB->stIncomingQueue.ulLegNo = pstSCB->stAutoCall.ulCallingLegNo;
                    pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                    pstSCB->stIncomingQueue.ulQueueType = SC_SW_FORWARD_AGENT_GROUP;
                    if (sc_cwq_add_call(pstSCB, sc_task_get_agent_queue(pstSCB->stAutoCall.ulTcbID), pstLCB->stCall.stNumInfo.szRealCallee, pstSCB->stIncomingQueue.ulQueueType, DOS_FALSE) != DOS_SUCC)
                    {
                        /* 加入队列失败 */
                        DOS_ASSERT(0);
                        ulRet = DOS_FAIL;
                    }
                    else
                    {
                        /* 放音提示客户等待 */
                        pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                        sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CALL_QUEUE_WAIT, 1, 0, 0);
                        ulRet = DOS_SUCC;
                    }

                    break;
                case SC_TASK_MODE_AUDIO_ONLY:
                    /* 挂断客户 */
                    sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR);

                    break;
                default:
                    break;
            }
            break;

        case SC_AUTO_CALL_TONE:
            if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCalleeLegNo, pstSCB->stAutoCall.ulCallingLegNo) != DOS_SUCC)
            {
                sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                ulRet = DOS_FAIL;
            }

            /* 修改坐席状态 */
            pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoCall.ulAgentID);
            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_CALL_IN, SC_SRV_AUTO_CALL);
            }

            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_CONNECTED;
            break;

        case SC_AUTO_CALL_RELEASE:
            /* 坐席放忙音结束，挂断坐席的电话 */
            sc_req_hungup(pstSCB->ulSCBNo, pstRlayback->ulLegNo, CC_ERR_NORMAL_CLEAR);
            break;

        default:
            break;
    }

proc_finishe:

    sc_trace_scb(pstSCB, "Proccessed auto call playk stop event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

U32 sc_auto_call_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvtCall = NULL;
    U32                   ulRet         = DOS_FAIL;

    pstEvtCall = (SC_MSG_EVT_LEAVE_CALLQUE_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call queue event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Processing auto call queue event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_AFTER_KEY:
            if (SC_LEAVE_CALL_QUE_TIMEOUT == pstMsg->usInterErr)
            {
                /* 加入队列超时 */
            }
            else if (SC_LEAVE_CALL_QUE_SUCC == pstMsg->usInterErr)
            {
                if (DOS_ADDR_INVALID(pstEvtCall->pstAgentNode))
                {
                    /* 错误 */
                }
                else
                {
                    /* 呼叫坐席 */
                    ulRet = sc_agent_auto_callback(pstSCB, pstEvtCall->pstAgentNode);
                    if (ulRet == DOS_SUCC
                        && pstSCB->stAutoCall.stSCBTag.usStatus != SC_AUTO_CALL_TONE)
                    {
                        pstSCB->stAutoCall.bIsRingTimer = DOS_TRUE;
                        //sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo);
                        //sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, DOS_TRUE, DOS_FALSE);
                    }
                }
            }
        default:
            break;

     }

    sc_trace_scb(pstSCB, "Proccessed auto call answer event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        /* TODO 失败的处理，修改坐席状态 */
        if (DOS_ADDR_VALID(pstEvtCall->pstAgentNode)
            && DOS_ADDR_VALID(pstEvtCall->pstAgentNode->pstAgentInfo))
        {
            pstEvtCall->pstAgentNode->pstAgentInfo->bSelected = DOS_FALSE;
        }

    }

    return ulRet;

}

U32 sc_auto_call_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstLCB       =  NULL;
    U32                   ulTaskMode    = U32_BUTT;
    S32                   lKey          = 0;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call dtmf event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    lKey = pstDTMF->cDTMFVal - '0';

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_ACTIVE:
            ulTaskMode = sc_task_get_mode(pstSCB->stAutoCall.ulTcbID);
            if (ulTaskMode >= SC_TASK_MODE_BUTT)
            {
                /* TODO */
                DOS_ASSERT(0);
                //ulErrCode = CC_ERR_SC_CONFIG_ERR;
                return DOS_FAIL;
            }

            switch (ulTaskMode)
            {
                case SC_TASK_MODE_KEY4AGENT1:
                    if (lKey != 0)
                    {
                        break;
                    }
                case SC_TASK_MODE_KEY4AGENT:
                    /* 转坐席 */
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_AFTER_KEY;
                    pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                    pstSCB->ulCurrentSrv++;
                    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                    pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                    pstSCB->stIncomingQueue.ulLegNo = pstSCB->stAutoCall.ulCallingLegNo;
                    pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                    pstSCB->stIncomingQueue.ulQueueType = SC_SW_FORWARD_AGENT_GROUP;
                    if (sc_cwq_add_call(pstSCB, sc_task_get_agent_queue(pstSCB->stAutoCall.ulTcbID), pstLCB->stCall.stNumInfo.szCallee, pstSCB->stIncomingQueue.ulQueueType, DOS_FALSE) != DOS_SUCC)
                    {
                        /* 加入队列失败 */
                        DOS_ASSERT(0);
                    }
                    else
                    {
                        /* 放音提示客户等待 */
                        pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                        sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CALL_QUEUE_WAIT, 1, 0, 0);
                    }
                    break;

                default:
                    break;
            }
            break;

         case SC_AUTO_CALL_CONNECTED:
            /* 开启接入码业务 */
            if (pstDTMF->cDTMFVal != '*'
                && pstDTMF->cDTMFVal != '#' )
            {
                /* 第一个字符不是 '*' 或者 '#' 不保存  */
                return DOS_SUCC;
            }

            /* 只有坐席对应的leg执行接入码业务 */
            if (pstDTMF->ulLegNo != pstSCB->stAutoCall.ulCalleeLegNo)
            {
                return DOS_SUCC;
            }

            pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
            pstSCB->stAccessCode.szDialCache[0] = '\0';
            pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
            pstSCB->stAccessCode.ulAgentID = pstSCB->stAutoCall.ulAgentID;
            pstSCB->ulCurrentSrv++;
            pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;
            break;

         default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_auto_call_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HOLD_ST   *pstHold        = NULL;
    SC_LEG_CB            *pstLeg         = NULL;

    pstHold = (SC_MSG_EVT_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstHold->bIsHold)
    {
        /* 如果是被HOLD的，需要激活HOLD业务哦 */
        pstSCB->stHold.stSCBTag.bValid = DOS_TRUE;
        pstSCB->stHold.stSCBTag.bWaitingExit = DOS_FALSE;
        pstSCB->stHold.stSCBTag.usStatus = SC_HOLD_ACTIVE;
        pstSCB->stHold.ulCallLegNo = pstHold->ulLegNo;
        pstSCB->stHold.ulHoldCount++;

        pstSCB->ulCurrentSrv++;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stHold.stSCBTag;

        /* 给HOLD方 播放拨号音 */
        /* 给HOLD对方 播放呼叫保持音 */
    }
    else
    {
        /* 如果是被UNHOLD的，已经没有HOLD业务了，单纯处理呼叫就好 */
        pstLeg = sc_lcb_get(pstHold->ulLegNo);
        if (DOS_ADDR_INVALID(pstLeg))
        {
            return DOS_FAIL;
        }

        if (pstLeg->stHold.ulHoldTime != 0
            && pstLeg->stHold.ulUnHoldTime > pstLeg->stHold.ulHoldTime)
        {
            pstSCB->stHold.ulHoldTotalTime += (pstLeg->stHold.ulUnHoldTime - pstLeg->stHold.ulHoldTime);
        }

        pstLeg->stHold.ulUnHoldTime = 0;
        pstLeg->stHold.ulHoldTime = 0;
    }

    return DOS_SUCC;
}

U32 sc_auto_call_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* 处理录音结束 */
    SC_MSG_CMD_RECORD_ST *pstRecord = NULL;
    SC_LEG_CB            *pstLCB    = NULL;

    pstRecord = (SC_MSG_CMD_RECORD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstRecord) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstRecord->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        return DOS_FAIL;
    }

    /* 生成录音话单 */
    sc_send_special_billing_stop2bs(pstSCB, pstLCB, BS_SERV_RECORDING);
    sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);

    return DOS_SUCC;
}

U32 sc_auto_call_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                     ulRet           = DOS_FAIL;
    SC_LEG_CB               *pstCallingCB   = NULL;
    SC_LEG_CB               *pstCalleeCB    = NULL;
    SC_LEG_CB               *pstHungupLeg   = NULL;
    SC_LEG_CB               *pstOtherLeg    = NULL;
    SC_MSG_EVT_HUNGUP_ST    *pstHungup      = NULL;
    SC_AGENT_NODE_ST        *pstAgentCall   = NULL;
    S32                     i               = 0;
    S32                     lRes            = DOS_FAIL;
    U32                     ulReleasePart;
    U32                     ulErrCode;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;

    ulErrCode = pstHungup->ulErrCode;

    sc_trace_scb(pstSCB, "Proccessing auto call hungup event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing auto call hungup event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    if (pstHungup->ulLegNo == pstSCB->stAutoCall.ulCallingLegNo)
    {
        ulReleasePart = SC_CALLING;
    }
    else
    {
        ulReleasePart = SC_CALLEE;
    }

    if (pstSCB->stAutoCall.ulCallingLegNo != U32_BUTT)
    {
        /* 呼叫结果 */
        if (SC_AUTO_CALL_PROCESS != pstSCB->stAutoCall.stSCBTag.usStatus
            && SC_AUTO_CALL_RELEASE != pstSCB->stAutoCall.stSCBTag.usStatus)
        {
            sc_task_concurrency_minus(pstSCB->stAutoCall.ulTcbID);
            pstHungupLeg = sc_lcb_get(pstHungup->ulLegNo);
            if (DOS_ADDR_VALID(pstHungupLeg))
            {
                sc_task_call_result(pstSCB, pstHungupLeg->ulCBNo, pstHungup->ulErrCode, pstSCB->stAutoCall.stSCBTag.usStatus);
            }
        }
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PROC:
            /* 这个时候挂断只会是客户的LEG清理资源即可 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent not connected.");

            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (pstCallingCB)
            {
                sc_lcb_free(pstCallingCB);
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_AUTO_CALL_ALERTING:
        case SC_AUTO_CALL_ACTIVE:
            /* 客户挂断了电话，需要生成话单 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent not connected. Need create cdr");

            /* 生成话单 */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);
            }

            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
            }
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_AUTO_CALL_AFTER_KEY:
        case SC_AUTO_CALL_AUTH2:
        case SC_AUTO_CALL_EXEC2:
        case SC_AUTO_CALL_PORC2:
        case SC_AUTO_CALL_ALERTING2:
            /* 这个时候挂断，可能是坐席也可能客户，如果是客户需要注意LEG的状态 */
            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCallingCB)
                && DOS_ADDR_VALID(pstCalleeCB))
            {
                /* 坐席振铃超时 */
                pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoCall.ulAgentID);
                if (DOS_ADDR_VALID(pstAgentCall))
                {
                    sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL);
                }

                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
                sc_scb_free(pstSCB);
                pstSCB = NULL;
                break;
            }

            /* 判断一下错误码，是不是坐席电话打不通 */
            if (pstSCB->stAutoCall.stSCBTag.usStatus == SC_AUTO_CALL_PORC2
                && SC_CALLEE == ulReleasePart
                && pstSCB->stAutoCall.ulReCallAgent <= SC_AUTO_CALL_AGENT_MAX_COUNT)
            {
                /* 重新加入到呼叫队列 */
                pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoCall.ulAgentID);
                if (DOS_ADDR_VALID(pstAgentCall)
                     && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
                {
                    pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                    sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_CALL);
                }

                sc_lcb_free(pstCalleeCB);
                pstSCB->stAutoCall.ulCalleeLegNo = U32_BUTT;

                break;
            }

            if (pstHungup->ulLegNo == pstSCB->stAutoCall.ulCallingLegNo)
            {
                /* 客户挂断的，判断是否存在坐席那条leg，如果存在，判断坐席是否长签等 */
                /* 生成话单 */
                if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
                {
                    sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
                }

                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);

                if (DOS_ADDR_INVALID(pstCalleeCB))
                {
                    /* 不存在坐席，直接释放资源就行了 */
                    if (DOS_ADDR_INVALID(pstCallingCB))
                    {
                        sc_lcb_free(pstCallingCB);
                        pstCalleeCB = NULL;
                    }
                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                    break;
                }

                pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoCall.ulAgentID);
                if (DOS_ADDR_INVALID(pstAgentCall)
                    || DOS_ADDR_INVALID(pstAgentCall->pstAgentInfo))
                {
                    /* 全部释放掉 */
                    if (DOS_ADDR_INVALID(pstCallingCB))
                    {
                        sc_lcb_free(pstCallingCB);
                        pstCallingCB = NULL;
                    }
                    sc_lcb_free(pstCalleeCB);
                    pstCalleeCB = NULL;
                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                    break;
                }

                /* 坐席置闲 */
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_CALL);

                if (pstCalleeCB->ulIndSCBNo != U32_BUTT)
                {
                    /* 坐席长签，继续放音 */
                    pstCalleeCB->ulSCBNo = U32_BUTT;
                    if (DOS_ADDR_VALID(pstCallingCB))
                    {
                        sc_lcb_free(pstCallingCB);
                        pstCallingCB = NULL;
                    }
                    sc_scb_free(pstSCB);
                    pstCalleeCB = NULL;
                    sc_req_playback_stop(pstCalleeCB->ulIndSCBNo, pstCalleeCB->ulCBNo);
                    sc_req_play_sound(pstCalleeCB->ulIndSCBNo, pstCalleeCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);

                    break;
                }

                pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                if (DOS_ADDR_VALID(pstCallingCB))
                {
                    sc_lcb_free(pstCallingCB);
                    pstCallingCB = NULL;
                }
                pstSCB->stAutoCall.ulCallingLegNo = U32_BUTT;
                if (pstSCB->stAutoCall.stSCBTag.usStatus < SC_AUTO_CALL_PORC2)
                {
                    /* 被叫没有呼叫成功，直接释放就行了 */
                    sc_lcb_free(pstCalleeCB);
                    pstCalleeCB = NULL;
                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                }
                else
                {
                    /* 可能在放回铃音，需要手动停止 */
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
                    sc_req_playback_stop(pstSCB->ulSCBNo, pstCalleeCB->ulCBNo);
                    ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstCalleeCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                }
            }
            else
            {
                /* 坐席先挂断的，挂断客户的电话，生成话单，修改坐席状态 */
                if (DOS_ADDR_INVALID(pstCallingCB) || DOS_ADDR_INVALID(pstCalleeCB))
                {
                    /* TODO 错误处理 */
                    break;
                }

                pstCallingCB->stCall.stTimeInfo.ulByeTime = pstCalleeCB->stCall.stTimeInfo.ulByeTime;
                if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
                {
                    sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
                }

                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);

                pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoCall.ulAgentID);
                if (DOS_ADDR_VALID(pstAgentCall) && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_CALL);
                    if (pstCalleeCB->ulIndSCBNo != U32_BUTT)
                    {
                        /* 长签的电话挂断了，取消关系就行了，不用释放 */
                        pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                        pstSCB->stAutoCall.ulCalleeLegNo = U32_BUTT;
                        pstCalleeCB->ulSCBNo = U32_BUTT;
                    }
                    else
                    {
                        pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                        pstSCB->stAutoCall.ulCalleeLegNo = U32_BUTT;
                        sc_lcb_free(pstCalleeCB);
                    }
                }

                /* 挂断客户的电话 */
                sc_req_playback_stop(pstSCB->ulSCBNo, pstCallingCB->ulCBNo);
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstCallingCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
            }
            break;

        case SC_AUTO_CALL_CONNECTED:
            /* 这个时候挂断，就是正常释放的节奏，处理完就好，判断坐席的状态 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent connected.");
            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCalleeCB)
                || DOS_ADDR_INVALID(pstCallingCB))
            {
                /* 系统异常 */
                DOS_ASSERT(0);

                if (DOS_ADDR_VALID(pstCallingCB))
                {
                    sc_req_hungup(pstSCB->ulSCBNo, pstCallingCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
                    sc_lcb_free(pstCallingCB);
                }

                if (DOS_ADDR_VALID(pstCalleeCB))
                {
                    sc_req_hungup(pstSCB->ulSCBNo, pstCalleeCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
                    sc_lcb_free(pstCalleeCB);
                }

                sc_scb_free(pstSCB);
                break;
            }

            if (pstSCB->stAutoCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCalleeCB;
                pstOtherLeg  = pstCallingCB;
                pstCallingCB->stCall.stTimeInfo.ulByeTime = pstCalleeCB->stCall.stTimeInfo.ulByeTime;
            }
            else
            {
                pstHungupLeg = pstCallingCB;
                pstOtherLeg  = pstCalleeCB;
                pstCalleeCB->stCall.stTimeInfo.ulByeTime = pstCallingCB->stCall.stTimeInfo.ulByeTime;
            }

            if (pstCalleeCB->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCalleeCB->ulOtherSCBNo);
                pstCalleeCB->ulOtherSCBNo = U32_BUTT;
            }

            if (pstCallingCB->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCallingCB->ulOtherSCBNo);
                pstCallingCB->ulOtherSCBNo = U32_BUTT;
            }

            /* 生成话单 */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            if (sc_scb_is_exit_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                /* 如果有出局呼叫，应该先将出局呼叫删除，应为出局呼叫是 */
                sc_scb_remove_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);
                /* 出局呼叫的话单应该用坐席那条leg产生 */
                sc_scb_remove_service(pstSCB, BS_SERV_AUTO_DIALING);
                sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCalleeCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalleeCB, NULL, ulReleasePart);
            }
            else
            {
                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);
            }

            /* 到这里，说明两个leg都OK */
            /*
              * 需要看看是否长签等问题，如果主/被叫LEG都长签了，需要申请SCB，将被叫LEG挂到新的SCB中
              * 否则，将需要长签的LEG作为当前业务控制块的主叫LEG，挂断另外一条LEG
              * 可能需要处理客户标记
              */
            /* release 时，肯定是有一条leg hungup了，现在的leg需要释放掉，判断另一条是不是坐席长签，如果不是需要挂断 */
            pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoCall.ulAgentID);
            if (DOS_ADDR_INVALID(pstAgentCall)
                || DOS_ADDR_INVALID(pstAgentCall->pstAgentInfo))
            {
                /* 没有找到坐席 */
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Can not found agent by id(%u)", pstSCB->stAutoCall.ulAgentID);
            }

            /* 判断是否需要进行，客户标记。1、是客户一端先挂断的(基础呼叫中，客户只能是PSTN，坐席只能是SIP) */
            if (pstHungupLeg == pstCallingCB
                && DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && pstAgentCall->pstAgentInfo->ucProcesingTime != 0
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                /* 客户标记 */
                pstSCB->stMarkCustom.stSCBTag.bValid = DOS_TRUE;
                pstSCB->stMarkCustom.ulLegNo = pstOtherLeg->ulCBNo;
                pstSCB->stMarkCustom.pstAgentCall = pstAgentCall;
                pstSCB->ulCurrentSrv++;
                pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stMarkCustom.stSCBTag;

                if (pstOtherLeg->ulIndSCBNo == U32_BUTT)
                {
                    /* 非长签时，要把坐席对应的leg的结束时间，赋值给开始时间，出标记话单时使用 */
                    pstOtherLeg->stCall.stTimeInfo.ulAnswerTime = pstHungupLeg->stCall.stTimeInfo.ulByeTime;
                    for (i=0; i<SC_MAX_SERVICE_TYPE; i++)
                    {
                        pstSCB->aucServType[i] = 0;
                    }

                    if (pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);
                    }
                    else if(pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                    }
                    else
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
                    }

                    /* 将客户的号码改为主叫号码 */
                    dos_strcpy(pstOtherLeg->stCall.stNumInfo.szOriginalCalling, pstAgentCall->pstAgentInfo->szLastCustomerNum);
                }

                /* 修改坐席状态为 proc，播放 标记背景音 */
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_PROC, SC_SRV_AUTO_CALL);
                sc_req_play_sound(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, SC_SND_CALL_OVER, 1, 0, 0);
                pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_IDEL;

                /* 开启定时器 */
                lRes = dos_tmr_start(&pstSCB->stMarkCustom.stTmrHandle, pstAgentCall->pstAgentInfo->ucProcesingTime * 1000, sc_agent_mark_custom_callback, (U64)pstOtherLeg->ulCBNo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    pstSCB->stMarkCustom.stTmrHandle = NULL;
                }

                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;

                if (pstSCB->stAutoCall.ulCalleeLegNo == pstHungup->ulLegNo)
                {
                    pstSCB->stAutoCall.ulCalleeLegNo = U32_BUTT;
                }
                else
                {
                    pstSCB->stAutoCall.ulCallingLegNo = U32_BUTT;
                }

                pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_PROCESS;

                break;
            }

            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                pstAgentCall->pstAgentInfo->bMarkCustomer = DOS_FALSE;
            }

            /* 不需要客户标记 */
            if (pstSCB->stAutoCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstSCB->stAutoCall.ulCalleeLegNo = U32_BUTT;
            }
            else
            {
                pstSCB->stAutoCall.ulCallingLegNo = U32_BUTT;
            }


            if (pstOtherLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 修改坐席的状态 */
                if (DOS_ADDR_VALID(pstAgentCall)
                    && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_CALL);
                }

                /* 长签，继续放音 */
                pstOtherLeg->ulSCBNo = U32_BUTT;
                sc_req_play_sound(pstOtherLeg->ulIndSCBNo, pstOtherLeg->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                /* 释放掉 SCB */
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
            else if (pstHungupLeg->ulIndSCBNo != U32_BUTT)
            {
                if (DOS_ADDR_VALID(pstAgentCall)
                    && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_CALL);
                }

                /* 长签的坐席挂断了电话，不要释放leg，解除关系就行 */
                pstSCB->stAutoCall.ulCalleeLegNo = U32_BUTT;
                pstHungupLeg->ulSCBNo = U32_BUTT;
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
            }
            else if (pstSCB->stAutoCall.ulCallingLegNo == U32_BUTT)
            {
                /* 客户挂断，不修改坐席状态，坐席置忙音 */
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;

                sc_req_busy_tone(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCalleeLegNo);
                pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
            }
            else
            {
                if (DOS_ADDR_VALID(pstAgentCall)
                    && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_CALL);
                }

                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
            }
            break;

        case SC_AUTO_CALL_PROCESS:
            /* 坐席处理完了，挂断 */
            pstCalleeCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_AUTO_CALL_RELEASE:
            pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoCall.ulAgentID);
            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_CALL);
            }

            pstCalleeCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;

            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call hungup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed auto call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

U32 sc_auto_call_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    S32                         lRes                = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;
    SC_LEG_CB                   *pstCallingCB       = NULL;
    SC_LEG_CB                   *pstCalleeCB        = NULL;
    SC_LEG_CB                   *pstRecordLegCB     = NULL;
    SC_LEG_CB                   *pstHungupLeg       = NULL;
    SC_MSG_CMD_RECORD_ST        stRecordRsp;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing auto call error event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing auto call error event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */
        if (!sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
        {
            return DOS_SUCC;
        }

        /* 判断是否需要录音 */
        pstCalleeCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
        if (DOS_ADDR_VALID(pstCalleeCB)
            && pstCalleeCB->stRecord.bValid)
        {
            pstRecordLegCB = pstCalleeCB;
        }
        else
        {
            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB) && pstCallingCB->stRecord.bValid)
            {
                pstRecordLegCB = pstCallingCB;
            }
        }

        if (DOS_ADDR_VALID(pstRecordLegCB))
        {
            stRecordRsp.stMsgTag.ulMsgType = SC_CMD_RECORD;
            stRecordRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stRecordRsp.stMsgTag.usInterErr = 0;
            stRecordRsp.ulSCBNo = pstSCB->ulSCBNo;
            stRecordRsp.ulLegNo = pstRecordLegCB->ulCBNo;

            if (sc_send_cmd_record(&stRecordRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_INFO, SC_MOD_EVENT, SC_LOG_DISIST), "Send record cmd FAIL! SCBNo : %u", pstSCB->ulSCBNo);
            }
        }

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
        case SC_AUTO_CALL_EXEC:
            /* 发起呼叫失败，生成呼叫结果，释放资源 */
            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                pstCallingCB->stCall.stTimeInfo.ulByeTime = pstCallingCB->stCall.stTimeInfo.ulStartTime;
                sc_task_call_result(pstSCB, pstCallingCB->ulCBNo, ulErrCode, pstSCB->stAutoCall.stSCBTag.usStatus);
                sc_lcb_free(pstCallingCB);
            }
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_AUTO_CALL_PROC:
            /* 呼叫结果 */
            sc_task_concurrency_minus(pstSCB->stAutoCall.ulTcbID);
            pstHungupLeg = sc_lcb_get(pstErrReport->ulLegNo);
            if (DOS_ADDR_VALID(pstHungupLeg))
            {
                sc_task_call_result(pstSCB, pstHungupLeg->ulCBNo, ulErrCode, pstSCB->stAutoCall.stSCBTag.usStatus);
            }

            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                pstCallingCB->stCall.stTimeInfo.ulByeTime = pstCallingCB->stCall.stTimeInfo.ulStartTime;
                sc_task_call_result(pstSCB, pstCallingCB->ulCBNo, ulErrCode, pstSCB->stAutoCall.stSCBTag.usStatus);
                sc_lcb_free(pstCallingCB);
            }
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_AUTO_CALL_ALERTING:
            sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo);
            ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, ulErrCode);
            break;

        case SC_AUTO_CALL_ACTIVE:
        case SC_AUTO_CALL_AFTER_KEY:
        case SC_AUTO_CALL_AUTH2:
            sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo);
            ulRet = sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, ulErrCode);
            break;

        case SC_AUTO_CALL_EXEC2:
        case SC_AUTO_CALL_PORC2:
            if (pstSCB->stAutoCall.ulReCallAgent > SC_AUTO_CALL_AGENT_MAX_COUNT)
            {
                sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo);
                ulRet = sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, ulErrCode);
                break;
            }
            else
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Re call count : %u", pstSCB->stAutoCall.ulReCallAgent);
                pstSCB->stAutoCall.ulReCallAgent++;
                if (DOS_ADDR_VALID(pstSCB->stAutoCall.stCusTmrHandle))
                {
                    dos_tmr_stop(&pstSCB->stAutoCall.stCusTmrHandle);
                    pstSCB->stAutoCall.stCusTmrHandle = NULL;
                }

                lRes = dos_tmr_start(&pstSCB->stAutoCall.stCusTmrHandle, SC_AUTO_CALL_RECALL_TIME, sc_auto_call_recall_agent_callback, (U64)pstSCB->ulSCBNo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    pstSCB->stAutoCall.stCusTmrHandle = NULL;
                }
            }
            break;

        case SC_AUTO_CALL_ALERTING2:
            sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo);
            ulRet = sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, ulErrCode);
            break;

        case SC_AUTO_CALL_TONE:
            break;

        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
        case SC_AUTO_CALL_RELEASE:
            if (pstSCB->stAutoCall.ulCalleeLegNo != U32_BUTT)
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCalleeLegNo, ulErrCode);
            }
            else
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, ulErrCode);
            }
            break;

        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_sigin_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB = NULL;
    U32                         ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    sc_trace_scb(pstSCB, "Processing the sigin auth msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    if (!pstSCB->stSigin.stSCBTag.bValid)
    {
        /* 没有启动 长签业务 */
        sc_scb_free(pstSCB);

        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Scb(%u) not have sigin server.", pstSCB->ulSCBNo);
        return DOS_FAIL;
    }

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_scb_free(pstSCB);

        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* 注意通过偏移量，找到CC统一定义的错误码 */

        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_AUTH:
            /* 发起呼叫 */
            pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_EXEC;
            ulRet = sc_make_call2pstn(pstSCB, pstLegCB);
            break;
         default:
            break;
    }

    return ulRet;
}

U32 sc_sigin_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB   *pstLegCB = NULL;
    SC_MSG_EVT_CALL_ST  *pstCallSetup;
    U32         ulRet            = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallSetup = (SC_MSG_EVT_CALL_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Processing the sigin setup msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_EXEC:
            /* 发起呼叫 */
            pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_PORC;
            ulRet = DOS_SUCC;
            break;
         default:
            break;
    }

    return ulRet;
}

U32 sc_sigin_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_RINGING_ST *pstEvent = NULL;
    SC_LEG_CB             *pstLegCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEvent = (SC_MSG_EVT_RINGING_ST *)pstMsg;

    sc_trace_scb(pstSCB, "process the sigin alerting msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCB->stSigin.pstAgentNode->pstAgentInfo->ulLegNo = pstSCB->stSigin.ulLegNo;

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_IDEL:
        case SC_SIGIN_AUTH:
        case SC_SIGIN_PORC:
        case SC_SIGIN_EXEC:
            pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_ALERTING;
            break;

        case SC_SIGIN_ALERTING:
        case SC_SIGIN_ACTIVE:
        case SC_SIGIN_RELEASE:
            break;
    }

    return DOS_SUCC;
}

U32 sc_sigin_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ANSWER_ST  *pstEvtAnswer   = NULL;
    SC_LEG_CB             *pstLegCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the sigin answer msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstEvtAnswer = (SC_MSG_EVT_ANSWER_ST *)pstMsg;

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_ALERTING:
            pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_ACTIVE;

            if (DOS_ADDR_VALID(pstSCB->stSigin.pstAgentNode))
            {
                pstSCB->stSigin.pstAgentNode->pstAgentInfo->bConnected = DOS_TRUE;
                /* 长签统计 */
                sc_agent_stat(SC_AGENT_STAT_SIGNIN, pstSCB->stSigin.pstAgentNode->pstAgentInfo, pstSCB->stSigin.pstAgentNode->pstAgentInfo->ulAgentID, 0);
            }

            sc_agent_serv_status_update(pstSCB->stSigin.pstAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AGENT_SIGIN);
            /* 放长签音 */
            sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stSigin.ulLegNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);

            break;
        default:
            break;
    }

    return DOS_SUCC;
}


U32 sc_sigin_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstLegCB        = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstRlayback     = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the sigin playback stop msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstRlayback = (SC_MSG_EVT_PLAYBACK_ST *)pstMsg;

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_ACTIVE:
            if (DOS_ADDR_INVALID(pstSCB->stSigin.pstAgentNode))
            {
                break;
            }

            if (pstSCB->stSigin.pstAgentNode->pstAgentInfo->bNeedConnected)
            {
                if (pstLegCB->stPlayback.usStatus == SC_SU_PLAYBACK_INIT)
                {
                    /* 放长签音 */
                    sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stSigin.ulLegNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                }
            }
            else
            {

            }

            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_sigin_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF = NULL;
    SC_LEG_CB             *pstLCB =  NULL;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing sigin dtmf.");

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 长签电话没有业务时，才有效 */
    if (pstLCB->ulSCBNo != U32_BUTT)
    {
        return DOS_SUCC;
    }

    if (DOS_ADDR_INVALID(pstSCB->stSigin.pstAgentNode)
        || DOS_ADDR_INVALID(pstSCB->stSigin.pstAgentNode->pstAgentInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* 开启 接入码 业务 */
    if (pstDTMF->cDTMFVal != '*')
    {
        /* 长签时，接入码的第一个字符必须是 '*'  */
        return DOS_SUCC;
    }

    pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stAccessCode.szDialCache[0] = '\0';
    pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
    pstSCB->stAccessCode.ulLegNo = pstSCB->stSigin.ulLegNo;
    pstSCB->stAccessCode.ulAgentID = pstSCB->stSigin.pstAgentNode->pstAgentInfo->ulAgentID;
    pstSCB->ulCurrentSrv++;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;

    return DOS_SUCC;
}

U32 sc_sigin_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstLegCB        = NULL;
    U32                     ulRet           = DOS_FAIL;
    SC_MSG_EVT_HUNGUP_ST    *pstHungup      = NULL;
    U32                     ulErrCode;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    ulErrCode = pstHungup->ulErrCode;

    sc_trace_scb(pstSCB, "process the sigin release msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstSCB->stSigin.pstAgentNode))
    {
        sc_scb_free(pstSCB);
        sc_lcb_free(pstLegCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 挂断时，生成长签的话单 */
    pstLegCB->stCall.ulCause = ulErrCode;
    sc_send_billing_stop2bs(pstSCB, pstLegCB, NULL, SC_CALLEE);

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_ALERTING:
        case SC_SIGIN_ACTIVE:
            if (pstSCB->stSigin.pstAgentNode->pstAgentInfo->bNeedConnected)
            {
                /* 需要重新呼叫坐席，进行长签 */
                pstLegCB->stPlayback.usStatus = SC_SU_PLAYBACK_INIT;
                pstSCB->stSigin.pstAgentNode->pstAgentInfo->bConnected = DOS_FALSE;

                sc_agent_serv_status_update(pstSCB->stSigin.pstAgentNode->pstAgentInfo, SC_ACD_SERV_RINGING, SC_SRV_AGENT_SIGIN);

                if (pstLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                {
                    /* 需要认证 */
                    pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_AUTH;
                    ulRet = sc_send_usr_auth2bs(pstSCB, pstLegCB);
                    if (ulRet != DOS_SUCC)
                    {
                        sc_scb_free(pstSCB);
                        sc_lcb_free(pstLegCB);

                        return DOS_FAIL;
                    }
                }
                else if (pstLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND_TT)
                {
                    ulRet = sc_make_call2eix(pstSCB, pstLegCB);
                    pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_EXEC;
                }
                else
                {
                    ulRet = sc_make_call2sip(pstSCB, pstLegCB);
                    pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_EXEC;
                }
            }
            else
            {
                /* 释放 */
                pstSCB->stSigin.pstAgentNode->pstAgentInfo->bConnected = DOS_FALSE;
                pstSCB->stSigin.pstAgentNode->pstAgentInfo->ulLegNo = U32_BUTT;
                sc_scb_free(pstSCB);
                sc_lcb_free(pstLegCB);
            }
            break;

        default:
            /* 释放 */
            pstSCB->stSigin.pstAgentNode->pstAgentInfo->bConnected = DOS_FALSE;
            pstSCB->stSigin.pstAgentNode->pstAgentInfo->bNeedConnected = DOS_FALSE;
            sc_scb_free(pstSCB);
            sc_lcb_free(pstLegCB);
            break;
    }

    return DOS_SUCC;
}

U32 sc_sigin_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing sigin error event. status : %u", pstSCB->stSigin.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_IDEL:
        case SC_SIGIN_AUTH:
        case SC_SIGIN_EXEC:
        case SC_SIGIN_PORC:
        case SC_SIGIN_ALERTING:
        case SC_SIGIN_ACTIVE:
        case SC_SIGIN_RELEASE:
            ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stSigin.ulLegNo, ulErrCode);
            break;
        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_incoming_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing incoming playback stop event. status : %u", pstSCB->stIncomingQueue.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stIncomingQueue.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;

        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        return DOS_FAIL;
    }

    switch (pstSCB->stIncomingQueue.stSCBTag.usStatus)
    {
        case SC_INQUEUE_IDEL:
        case SC_INQUEUE_ACTIVE:
            /* 给客户放提示音 */
            sc_req_play_sound(pstLCB->ulSCBNo, pstLCB->ulCBNo, SC_SND_CALL_QUEUE_WAIT, 1, 0, 0);
            break;
        default:
            break;
    }

    sc_trace_scb(pstSCB, "Processed call release event for voice verify.");

    return DOS_SUCC;
}

U32 sc_incoming_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvtCall = NULL;

    pstEvtCall = (SC_MSG_EVT_LEAVE_CALLQUE_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing leave call queue event. status : %u", pstSCB->stIncomingQueue.stSCBTag.usStatus);

    switch (pstSCB->stIncomingQueue.stSCBTag.usStatus)
    {
        case SC_INQUEUE_ACTIVE:
            pstSCB->stIncomingQueue.ulDequeuTime = time(NULL);
            pstSCB->stIncomingQueue.stSCBTag.bWaitingExit = DOS_TRUE;
            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_incoming_queue_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HUNGUP_ST *pstEvtCall = NULL;

    pstEvtCall = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing incoming realse event. status : %u", pstSCB->stIncomingQueue.stSCBTag.usStatus);

    switch (pstSCB->stIncomingQueue.stSCBTag.usStatus)
    {
        case SC_INQUEUE_ACTIVE:
            /* 从队列中删除 */

        case SC_INQUEUE_IDEL:
        case SC_INQUEUE_RELEASE:
            sc_cwq_del_call(pstSCB, pstSCB->stIncomingQueue.ulQueueType);
            pstSCB->stIncomingQueue.stSCBTag.bWaitingExit = DOS_TRUE;
            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_mark_custom_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstLCB       = NULL;
    U32                   ulRet         = DOS_SUCC;
    U32                   ulKey         = 0;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing mark custom dtmf event. status : %u", pstSCB->stMarkCustom.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stMarkCustom.stSCBTag.usStatus)
    {
        case SC_MAKR_CUSTOM_PROC:
            /* 客户标记，第一个字符必须为 '*' */
            if (pstLCB->stCall.stNumInfo.szDial[0] != '*')
            {
                pstLCB->stCall.stNumInfo.szDial[0] = '\0';
                break;
            }

            dos_strcpy(pstSCB->stMarkCustom.szDialCache, pstLCB->stCall.stNumInfo.szDial);

            if (pstSCB->stMarkCustom.szDialCache[0] == '\0'
                && pstDTMF->cDTMFVal != '*')
            {
                /* 第一个字符不是 * 或者 #，不用保存 */
                break;
            }

            dos_strcpy(pstSCB->stMarkCustom.szDialCache, pstLCB->stCall.stNumInfo.szDial);

            /* 如果为 * 或者 #，判断是否符合  [*|#]D[*|#] */
            if ((pstDTMF->cDTMFVal == '*' || pstDTMF->cDTMFVal == '#')
                    && dos_strlen(pstSCB->stMarkCustom.szDialCache) > 1)
            {
                if (pstSCB->stMarkCustom.szDialCache[0] == '*'
                    && pstSCB->stMarkCustom.szDialCache[3]  == '\0'
                    && 1 == dos_sscanf(pstSCB->stMarkCustom.szDialCache+1, "%u", &ulKey)
                    && ulKey <= 9)
                {
                    /* 客户标记 */
                    if (DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall)
                         && DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo))
                    {
                        sc_agent_marker_update_req(pstSCB->ulCustomerID, pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->ulAgentID, ulKey, pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->szLastCustomerNum);
                    }

                    /* 停止定时器 */
                    if (DOS_ADDR_VALID(pstSCB->stMarkCustom.stTmrHandle))
                    {
                        dos_tmr_stop(&pstSCB->stMarkCustom.stTmrHandle);
                        pstSCB->stMarkCustom.stTmrHandle = NULL;
                    }

                    /* 停止放音 */
                    sc_req_playback_stop(pstSCB->ulSCBNo, pstLCB->ulCBNo);

                    /* 判断坐席是否是长签，如果不是则挂断电话 */
                    if (DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall)
                         && DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo))
                    {
                        sc_agent_serv_status_update(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_MARK_CUSTOM);
                    }

                    /* 放提示音 */
                    sc_req_play_sound(pstLCB->ulSCBNo, pstLCB->ulCBNo, SC_SND_SET_SUCC, 1, 0, 0);
                    pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_ACTIVE;
                }
                else
                {
                    /* 格式错误，清空缓存 */
                    pstSCB->stMarkCustom.szDialCache[0] = '\0';
                    pstLCB->stCall.stNumInfo.szDial[0] = '\0';
                }
            }

            break;
         default:
            break;
    }

    return ulRet;
}

U32 sc_mark_custom_playback_start(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    sc_trace_scb(pstSCB, "Processing mark custom play start event. status : %u", pstSCB->stMarkCustom.stSCBTag.usStatus);

    switch (pstSCB->stMarkCustom.stSCBTag.usStatus)
    {
        case SC_MAKR_CUSTOM_IDEL:
            pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_PROC;
            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_mark_custom_playback_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstLCB          = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstRlayback     = NULL;

    pstRlayback = (SC_MSG_EVT_PLAYBACK_ST *)pstMsg;

    sc_trace_scb(pstSCB, "Processing mark custom play end event. status : %u", pstSCB->stMarkCustom.stSCBTag.usStatus);

    switch (pstSCB->stMarkCustom.stSCBTag.usStatus)
    {
        case SC_MAKR_CUSTOM_PROC:
            pstLCB = sc_lcb_get(pstRlayback->ulLegNo);
            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_req_play_sound(pstSCB->ulSCBNo, (pstLCB)->ulCBNo, SC_SND_CALL_OVER, 1, 0, 0);
            }
            break;

        case SC_MAKR_CUSTOM_ACTIVE:
            pstLCB = sc_lcb_get(pstRlayback->ulLegNo);
            if (DOS_ADDR_VALID(pstLCB))
            {
                if (pstLCB->ulIndSCBNo != U32_BUTT)
                {
                    /* 长签，继续放音 */
                    pstLCB->ulSCBNo = U32_BUTT;
                    sc_req_play_sound(pstLCB->ulIndSCBNo, pstLCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                    /* 释放掉 SCB */
                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                }
                else
                {
                    pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                    //sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                    sc_req_busy_tone(pstSCB->ulSCBNo, pstLCB->ulCBNo);
                    pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_RELEASE;
                }
            }
            break;

        case SC_MAKR_CUSTOM_RELEASE:
            /* 坐席放忙音结束，挂断电话 */
            pstLCB = sc_lcb_get(pstRlayback->ulLegNo);
            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
            }

        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_mark_custom_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB               *pstMarkLeg     = NULL;
    SC_MSG_EVT_HUNGUP_ST    *pstHungup      = NULL;
    U32                     ulErrCode;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    ulErrCode = pstHungup->ulErrCode;

    sc_trace_scb(pstSCB, "Processing mark custom realse event. status : %u", pstSCB->stMarkCustom.stSCBTag.usStatus);

    switch (pstSCB->stMarkCustom.stSCBTag.usStatus)
    {
        case SC_MAKR_CUSTOM_IDEL:
            pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_PROC;
            break;

        case SC_MAKR_CUSTOM_PROC:
        case SC_MAKR_CUSTOM_ACTIVE:
        case SC_MAKR_CUSTOM_RELEASE:
            /* 需要生成客户标记的话单，leg在其它业务中释放 */
            pstMarkLeg = sc_lcb_get(pstSCB->stMarkCustom.ulLegNo);
            if (DOS_ADDR_VALID(pstMarkLeg))
            {
                /* 被叫号码改为为 客户标记 */
                if (pstSCB->stMarkCustom.szDialCache[0] == '*'
                    && pstSCB->stMarkCustom.szDialCache[1] >= '0'
                    && pstSCB->stMarkCustom.szDialCache[1] <= '9'
                    && (pstSCB->stMarkCustom.szDialCache[2] == '*' || pstSCB->stMarkCustom.szDialCache[2] == '#')
                    && pstSCB->stMarkCustom.szDialCache[3] == '\0')
                {
                    dos_strcpy(pstMarkLeg->stCall.stNumInfo.szOriginalCallee, pstSCB->stMarkCustom.szDialCache);
                }
                else
                {
                    dos_strcpy(pstMarkLeg->stCall.stNumInfo.szOriginalCallee, "*#");
                }

                pstMarkLeg->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstMarkLeg, NULL, SC_CALLEE);
            }

            if (DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall)
                && DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_MARK_CUSTOM);
                pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
            }

            pstSCB->stMarkCustom.stSCBTag.bWaitingExit = DOS_TRUE;
            break;

        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_mark_custom_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing mark customer error event. status : %u", pstSCB->stMarkCustom.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stMarkCustom.stSCBTag.usStatus)
    {
        case SC_MAKR_CUSTOM_IDEL:
        case SC_MAKR_CUSTOM_PROC:
        case SC_MAKR_CUSTOM_ACTIVE:
        case SC_MAKR_CUSTOM_RELEASE:
            ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stMarkCustom.ulLegNo, ulErrCode);
            break;
        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_play_balance(U64 LBalance, U32 aulAudioList[], U32 *pulCount)
{
    U64   i             = 0;
    S32   j             = 0;
    S32   lInteger      = 0;
    S64   LRemainder    = 0;
    BOOL  bIsGetMSB     = DOS_FALSE;
    BOOL  bIsZero       = DOS_FALSE;
    U32   ulIndex       = 0;

    /* 根据余额拼接出语音 */
    aulAudioList[ulIndex++] = SC_SND_YOUR_BALANCE;

    if (LBalance < 0)
    {
        LBalance = 0 - LBalance;
        aulAudioList[ulIndex++] = SC_SND_FU;
    }

    LRemainder = LBalance;
    bIsGetMSB = DOS_FALSE;

    for (i=100000000000,j=0; j<10; i/=10,j++)
    {
        lInteger = LRemainder / i;
        LRemainder = LRemainder % i;

        if ((0 == lInteger || lInteger > 9) && DOS_FALSE == bIsGetMSB)
        {
            /* TODO 大于9时，说明余额操作了一个亿 */
            continue;
        }

        bIsGetMSB = DOS_TRUE;

        if (0 == lInteger)
        {
            bIsZero = DOS_TRUE;
        }
        else
        {
            if (bIsZero == DOS_TRUE)
            {
                bIsZero = DOS_FALSE;
                aulAudioList[ulIndex++] = SC_SND_0;
                aulAudioList[ulIndex++] = SC_SND_0 + lInteger;
            }
            else
            {
                aulAudioList[ulIndex++] = SC_SND_0 + lInteger;
            }
        }

        switch (j)
        {
            case 0:
            case 4:
                if (lInteger != 0)
                {
                    aulAudioList[ulIndex++] = SC_SND_QIAN;
                }
                break;
            case 1:
            case 5:
                if (lInteger != 0)
                {
                    aulAudioList[ulIndex++] = SC_SND_BAI;
                }
                break;
            case 2:
            case 6:
                if (lInteger != 0)
                {
                    aulAudioList[ulIndex++] = SC_SND_SHI;
                }
                break;
            case 3:
                aulAudioList[ulIndex++] = SC_SND_WAN;
                bIsZero = DOS_FALSE;
                break;
            case 7:
                aulAudioList[ulIndex++] = SC_SND_YUAN;
                bIsZero = DOS_FALSE;
                break;
            case 8:
                if (lInteger != 0)
                {
                    aulAudioList[ulIndex++] = SC_SND_JIAO;
                }
                bIsZero = DOS_FALSE;
                break;
            case 9:
                if (lInteger != 0)
                {
                    aulAudioList[ulIndex++] = SC_SND_FEN;
                }
                break;
            default:
                break;
        }
    }

    if (DOS_FALSE == bIsGetMSB)
    {
        /* 余额是0 */
        aulAudioList[ulIndex++] = SC_SND_0;
        aulAudioList[ulIndex++] = SC_SND_YUAN;
    }

    *pulCount = ulIndex;

    return DOS_SUCC;
}

U32 sc_access_code_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB = NULL;
    U32                         ulRet = DOS_FAIL;
    SC_MSG_CMD_PLAYBACK_ST      stPlaybackRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing access code event. status : %u", pstSCB->stAccessCode.stSCBTag.usStatus);

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* 注意通过偏移量，找到CC统一定义的错误码 */

        return DOS_SUCC;
    }

    pstLegCB = sc_lcb_get(pstSCB->stAccessCode.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        /* 错误处理 */
    }

    switch (pstSCB->stAccessCode.stSCBTag.usStatus)
    {
        case SC_ACCESS_CODE_ACTIVE:
            pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_RELEASE;
            switch (pstSCB->stAccessCode.ulSrvType)
            {
                case SC_ACCESS_BALANCE_ENQUIRY:
                    /* 播放余额 */
                    stPlaybackRsp.stMsgTag.ulMsgType = SC_CMD_PLAYBACK;
                    stPlaybackRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
                    stPlaybackRsp.stMsgTag.usInterErr = 0;
                    stPlaybackRsp.ulMode = 0;
                    stPlaybackRsp.ulSCBNo = pstSCB->ulSCBNo;
                    stPlaybackRsp.ulLegNo = pstLegCB->ulCBNo;
                    stPlaybackRsp.ulLoopCnt = 1;
                    stPlaybackRsp.ulInterval = 0;
                    stPlaybackRsp.ulSilence  = 0;
                    stPlaybackRsp.enType = SC_CND_PLAYBACK_SYSTEM;
                    stPlaybackRsp.blNeedDTMF = DOS_TRUE;
                    sc_play_balance(pstAuthRsp->LBalance, stPlaybackRsp.aulAudioList, &stPlaybackRsp.ulTotalAudioCnt);

                    if (sc_send_cmd_playback(&stPlaybackRsp.stMsgTag) != DOS_SUCC)
                    {
                        /* 错误处理 */
                    }
                    break;
                default:
                    break;
            }

            break;
        default:
            break;
    }

    return ulRet;
}

U32 sc_access_code_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF = NULL;
    SC_LEG_CB             *pstLCB  =  NULL;
    U32                   ulRet    = DOS_SUCC;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing access code dtmf. status : %u", pstSCB->stAccessCode.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stAccessCode.stSCBTag.usStatus)
    {
        case SC_ACCESS_CODE_OVERLAP:
            /* 第一个字符必须为 '*' 或者 '#' */
            if (pstLCB->stCall.stNumInfo.szDial[0] != '*' && pstLCB->stCall.stNumInfo.szDial[0] != '#')
            {
                pstLCB->stCall.stNumInfo.szDial[0] = '\0';
                break;
            }

            dos_strcpy(pstSCB->stAccessCode.szDialCache, pstLCB->stCall.stNumInfo.szDial);

            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Secondary dialing. caller : %s, DialNum : %s", pstLCB->stCall.stNumInfo.szRealCalling, pstSCB->stAccessCode.szDialCache);

            if (dos_strcmp(pstSCB->stAccessCode.szDialCache, astSCAccessList[SC_ACCESS_HANGUP_CUSTOMER1].szCodeFormat) == 0
                 || dos_strcmp(pstSCB->stAccessCode.szDialCache, astSCAccessList[SC_ACCESS_HANGUP_CUSTOMER2].szCodeFormat) == 0)
            {
                /* ## / ** */
                pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_ACTIVE;
                astSCAccessList[SC_ACCESS_HANGUP_CUSTOMER1].fn_init(pstSCB, pstLCB);
                /* 清空缓存 */
                pstLCB->stCall.stNumInfo.szDial[0] = '\0';
            }
            else if (pstDTMF->cDTMFVal == '#' && dos_strlen(pstSCB->stAccessCode.szDialCache) > 1)
            {
                /* # 为结束符，收到后，就应该去解析, 特别的，如果第一个字符为#,不需要去解析 */
                /* 不保存最后一个 # */
                pstSCB->stAccessCode.szDialCache[dos_strlen(pstSCB->stAccessCode.szDialCache) - 1] = '\0';
                ulRet = sc_call_access_code(pstSCB, pstLCB, pstSCB->stAccessCode.szDialCache, DOS_TRUE);
                /* 清空缓存 */
                pstLCB->stCall.stNumInfo.szDial[0] = '\0';
            }
            else if (pstDTMF->cDTMFVal == '*' && dos_strlen(pstSCB->stAccessCode.szDialCache) > 1)
            {
                /* 判断接收到 * 号，是否需要解析 */
                if (dos_strcmp(pstSCB->stAccessCode.szDialCache, astSCAccessList[SC_ACCESS_MARK_CUSTOMER].szCodeFormat)
                    && dos_strcmp(pstSCB->stAccessCode.szDialCache, astSCAccessList[SC_ACCESS_BLIND_TRANSFER].szCodeFormat)
                    && dos_strcmp(pstSCB->stAccessCode.szDialCache, astSCAccessList[SC_ACCESS_ATTENDED_TRANSFER].szCodeFormat))
                {
                    /* 不保存最后一个 * */
                    pstSCB->stAccessCode.szDialCache[dos_strlen(pstSCB->stAccessCode.szDialCache) - 1] = '\0';
                    ulRet = sc_call_access_code(pstSCB, pstLCB, pstSCB->stAccessCode.szDialCache, DOS_TRUE);
                    /* 清空缓存 */
                    pstLCB->stCall.stNumInfo.szDial[0] = '\0';
                }
            }
            break;
        default:
            break;
     }

    return ulRet;
}

U32 sc_access_code_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstLegCB        = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstRlayback     = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the access code playback stop msg. status: %u", pstSCB->stAccessCode.stSCBTag.usStatus);

    pstRlayback = (SC_MSG_EVT_PLAYBACK_ST *)pstMsg;

    pstLegCB = sc_lcb_get(pstSCB->stAccessCode.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stAccessCode.stSCBTag.usStatus)
    {
        case SC_ACCESS_CODE_ACTIVE:
        case SC_ACCESS_CODE_RELEASE:
            switch (pstSCB->stAccessCode.ulSrvType)
            {
                case SC_ACCESS_BALANCE_ENQUIRY:
                case SC_ACCESS_AGENT_ONLINE:
                case SC_ACCESS_AGENT_OFFLINE:
                case SC_ACCESS_MARK_CUSTOMER:
                    /* 挂断电话 */
                    pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_RELEASE;
                    sc_req_hungup(pstSCB->ulSCBNo, pstLegCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                    break;
                case SC_ACCESS_AGENT_EN_QUEUE:
                case SC_ACCESS_AGENT_DN_QUEUE:
                case SC_ACCESS_AGENT_SIGNOUT:
                    if (pstSCB->stAccessCode.bIsSecondDial)
                    {
                        /* 二次拨号，直接退出接入码业务就行了 */
                        pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_RELEASE;
                        pstSCB->stAccessCode.stSCBTag.bWaitingExit = DOS_TRUE;
                    }
                    else
                    {
                        /* 挂断电话 */
                        pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_RELEASE;
                        sc_req_hungup(pstSCB->ulSCBNo, pstLegCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                    }
                    break;
                case SC_ACCESS_AGENT_SIGNIN:
                case SC_ACCESS_BLIND_TRANSFER:
                case SC_ACCESS_ATTENDED_TRANSFER:
                case SC_ACCESS_HANGUP_CUSTOMER1:
                case SC_ACCESS_HANGUP_CUSTOMER2:
                    break;
                default:
                    pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_RELEASE;
                    sc_req_hungup(pstSCB->ulSCBNo, pstLegCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                    break;
            }

            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_access_code_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HUNGUP_ST *pstEvtCall = NULL;

    pstEvtCall = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing access code realse event. status : %u", pstSCB->stIncomingQueue.stSCBTag.usStatus);

    switch (pstSCB->stAccessCode.stSCBTag.usStatus)
    {
        case SC_ACCESS_CODE_IDEL:
        case SC_ACCESS_CODE_OVERLAP:
        case SC_ACCESS_CODE_ACTIVE:
        case SC_ACCESS_CODE_RELEASE:
            pstSCB->stAccessCode.stSCBTag.bWaitingExit = DOS_TRUE;
            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_access_code_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing access code error event. status : %u", pstSCB->stAccessCode.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stAccessCode.stSCBTag.usStatus)
    {
        case SC_ACCESS_CODE_IDEL:
        case SC_ACCESS_CODE_OVERLAP:
        case SC_ACCESS_CODE_ACTIVE:
        case SC_ACCESS_CODE_RELEASE:
            ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAccessCode.ulLegNo, ulErrCode);
            break;
        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_transfer_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp      = NULL;
    SC_LEG_CB                  *pstSubLeg       = NULL;
    SC_LEG_CB                  *pstPublishLeg   = NULL;
    //SC_AGENT_NODE_ST           *pstAgentNode    = NULL;
    U32                         ulRet           = DOS_SUCC;
    //SC_MSG_CMD_CALL_ST          stCallMsg;
    SC_MSG_CMD_TRANSFER_ST      stTransferRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing transfer auth event. status : %u", pstSCB->stTransfer.stSCBTag.usStatus);

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* 注意通过偏移量，找到CC统一定义的错误码 */

        return DOS_SUCC;
    }

    switch (pstSCB->stTransfer.stSCBTag.usStatus)
    {
        case SC_TRANSFER_AUTH:
            pstSubLeg = sc_lcb_get(pstSCB->stTransfer.ulSubLegNo);
            pstPublishLeg = sc_lcb_get(pstSCB->stTransfer.ulPublishLegNo);
            if (DOS_ADDR_INVALID(pstSubLeg)
                || DOS_ADDR_INVALID(pstPublishLeg))
            {
                /* TODO 错误处理 */
                return DOS_FAIL;
            }

            /* 执行 transfer 命令 */
            stTransferRsp.stMsgTag.ulMsgType = SC_CMD_TRANSFER;
            stTransferRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stTransferRsp.stMsgTag.usInterErr = 0;

            stTransferRsp.ulSCBNo = pstSCB->ulSCBNo;
            stTransferRsp.ulLegNo = pstSubLeg->ulCBNo;
            dos_strncpy(stTransferRsp.szCalleeNum, pstPublishLeg->stCall.stNumInfo.szOriginalCallee, SC_NUM_LENGTH-1);
            stTransferRsp.szCalleeNum[SC_NUM_LENGTH-1] = '\0';

            if (sc_send_cmd_transfer(&stTransferRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                /* TODO 错误处理 */
                return DOS_FAIL;
            }
            pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_TRANSFERRING;
            break;

        default:
            break;
    }

    return ulRet;
}

U32 sc_transfer_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32        ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing transfer setup event. status : %u", pstSCB->stTransfer.stSCBTag.usStatus);

    switch (pstSCB->stTransfer.stSCBTag.usStatus)
    {
        case SC_TRANSFER_EXEC:
            pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_PROC;
            break;
        default:
            break;
    }

    return ulRet;
}

U32 sc_transfer_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HOLD_ST  *pstHold    = NULL;
    SC_LEG_CB            *pstLeg    = NULL;

    pstHold = (SC_MSG_EVT_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stTransfer.stSCBTag.usStatus)
    {
        case SC_TRANSFER_EXEC:
            /* 转接过程中，hold被转接方，直接放提示音就行了 */
            sc_req_play_sound(pstSCB->ulSCBNo, pstHold->ulLegNo, SC_SND_MUSIC_HOLD, 1, 0, 0);
            break;

        default:
            if (pstHold->bIsHold)
            {
                /* 如果是被HOLD的，需要激活HOLD业务哦 */
                pstSCB->stHold.stSCBTag.bValid = DOS_TRUE;
                pstSCB->stHold.stSCBTag.bWaitingExit = DOS_FALSE;
                pstSCB->stHold.stSCBTag.usStatus = SC_HOLD_ACTIVE;
                pstSCB->stHold.ulCallLegNo = pstHold->ulLegNo;
                pstSCB->stHold.ulHoldCount++;

                pstSCB->ulCurrentSrv++;
                pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stHold.stSCBTag;

                /* 给HOLD方 播放拨号音 */
                /* 给HOLD对方 播放呼叫保持音 */
            }
            else
            {
                /* 如果是被UNHOLD的，已经没有HOLD业务了，单纯处理呼叫就好 */
                pstLeg = sc_lcb_get(pstHold->ulLegNo);
                if (DOS_ADDR_INVALID(pstLeg))
                {
                    return DOS_FAIL;
                }

                if (pstLeg->stHold.ulHoldTime != 0
                    && pstLeg->stHold.ulUnHoldTime > pstLeg->stHold.ulHoldTime)
                {
                    pstSCB->stHold.ulHoldTotalTime += (pstLeg->stHold.ulUnHoldTime - pstLeg->stHold.ulHoldTime);
                }

                pstLeg->stHold.ulUnHoldTime = 0;
                pstLeg->stHold.ulHoldTime = 0;
            }
            break;
    }

    return DOS_SUCC;
}

U32 sc_transfer_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                 ulRet               = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing transfer play stop event. status : %u", pstSCB->stTransfer.stSCBTag.usStatus);

    switch (pstSCB->stTransfer.stSCBTag.usStatus)
    {
        case SC_TRANSFER_TONE:
            if (SC_ACCESS_BLIND_TRANSFER == pstSCB->stTransfer.ulType)
            {
                /* 盲转，直接bridge */
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stTransfer.ulSubLegNo, pstSCB->stTransfer.ulPublishLegNo) != DOS_SUCC)
                {
                    /* 错误处理 */
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    ulRet = DOS_FAIL;
                }
                pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_FINISHED;
            }
            else
            {
                /* 协商转 */
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stTransfer.ulNotifyLegNo, pstSCB->stTransfer.ulPublishLegNo) != DOS_SUCC)
                {
                    /* 错误处理 */
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    ulRet = DOS_FAIL;
                }
                pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_TRANSFER;
            }

            break;

        default:
            break;
    }

    return ulRet;
}

U32 sc_transfer_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                     ulRet                   = DOS_SUCC;
    SC_MSG_EVT_RINGING_ST   *pstEvent               = NULL;
    SC_LEG_CB               *pstPublishLeg          = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEvent = (SC_MSG_EVT_RINGING_ST *)pstMsg;

    sc_trace_scb(pstSCB, "Processing transfer ringing event. status : %u. with media: %u", pstSCB->stTransfer.stSCBTag.usStatus, pstEvent->ulWithMedia);

    switch (pstSCB->stTransfer.stSCBTag.usStatus)
    {
        case SC_TRANSFER_PROC:
        case SC_TRANSFER_ALERTING:
            if (SC_ACCESS_BLIND_TRANSFER == pstSCB->stTransfer.ulType)
            {
                sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stTransfer.ulSubLegNo, DOS_TRUE, pstEvent->ulWithMedia);
            }
            else
            {
                sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stTransfer.ulNotifyLegNo, DOS_TRUE, pstEvent->ulWithMedia);
            }

            if (pstEvent->ulWithMedia)
            {
                sc_trace_scb(pstSCB, "witch media : %u.", pstEvent->ulWithMedia);
                pstPublishLeg = sc_lcb_get(pstSCB->stTransfer.ulPublishLegNo);
                if (DOS_ADDR_VALID(pstPublishLeg))
                {
                    pstPublishLeg->stCall.bEarlyMedia = DOS_TRUE;

                    if (SC_ACCESS_BLIND_TRANSFER == pstSCB->stTransfer.ulType)
                    {
                        if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stTransfer.ulPublishLegNo, pstSCB->stTransfer.ulSubLegNo) != DOS_SUCC)
                        {
                            sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                            ulRet = DOS_FAIL;
                        }
                    }
                    else
                    {
                        if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stTransfer.ulPublishLegNo, pstSCB->stTransfer.ulNotifyLegNo) != DOS_SUCC)
                        {
                            sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                            ulRet = DOS_FAIL;
                        }
                    }
                }
            }

            pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_ALERTING;

            break;
        default:
            break;
    }

    return ulRet;
}

U32 sc_transfer_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                 ulRet                   = DOS_SUCC;
    SC_LEG_CB           *pstPublishLeg          = NULL;
    SC_LEG_CB           *pstNotifyLeg           = NULL;
    SC_AGENT_NODE_ST    *pstNotifyAgentNode     = NULL;
    SC_AGENT_NODE_ST    *pstPublishAgentNode    = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing transfer answer event. status : %u", pstSCB->stTransfer.stSCBTag.usStatus);

    switch (pstSCB->stTransfer.stSCBTag.usStatus)
    {
        case SC_TRANSFER_TRANSFERRING:
            pstPublishLeg = sc_lcb_get(pstSCB->stTransfer.ulPublishLegNo);
            pstNotifyLeg = sc_lcb_get(pstSCB->stTransfer.ulNotifyLegNo);
            if (DOS_ADDR_INVALID(pstPublishLeg)
                || DOS_ADDR_INVALID(pstNotifyLeg))
            {
                /* TODO 错误处理 */
                DOS_ASSERT(0);
                break;
            }

            pstNotifyLeg->stCall.stTimeInfo.ulTransferStartTime = time(NULL);

            if (SC_TRANSFER_PUBLISH_AGENT == pstSCB->stTransfer.ulPublishType)
            {
                pstPublishAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulPublishAgentID);
            }

            if (SC_ACCESS_BLIND_TRANSFER == pstSCB->stTransfer.ulType)
            {
                if (SC_TRANSFER_PUBLISH_SIP == pstSCB->stTransfer.ulPublishType)
                {
                    /* 转接sip分机 */
                    ulRet = sc_make_call2sip(pstSCB, pstPublishLeg);
                    pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_EXEC;
                }
                else
                {
                    /* 判断是否是长签 */
                    if (pstPublishLeg->ulIndSCBNo != U32_BUTT)
                    {
                        /* 长签，将被叫坐席置忙，放提示音 */
                        if (DOS_ADDR_VALID(pstPublishAgentNode)
                            && DOS_ADDR_VALID(pstPublishAgentNode->pstAgentInfo))
                        {
                            sc_agent_serv_status_update(pstPublishAgentNode->pstAgentInfo, SC_ACD_SERV_CALL_IN, SC_SRV_TRANSFER);
                        }

                        pstPublishLeg->ulSCBNo = pstSCB->ulSCBNo;
                        sc_req_playback_stop(pstSCB->ulSCBNo, pstPublishLeg->ulCBNo);
                        sc_req_play_sound(pstSCB->ulSCBNo, pstPublishLeg->ulCBNo, SC_SND_INCOMING_CALL_TIP, 1, 0, 0);
                        pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_TONE;
                    }
                    else
                    {
                        /* 呼叫需要转接的坐席 */
                        switch (pstPublishLeg->stCall.ucPeerType)
                        {
                            case SC_LEG_PEER_OUTBOUND:
                                ulRet = sc_make_call2pstn(pstSCB, pstPublishLeg);
                                break;

                            case SC_LEG_PEER_OUTBOUND_TT:
                                ulRet = sc_make_call2eix(pstSCB, pstPublishLeg);
                                break;

                            case SC_LEG_PEER_OUTBOUND_INTERNAL:
                                ulRet = sc_make_call2sip(pstSCB, pstPublishLeg);
                                break;

                            default:
                                sc_trace_scb(pstSCB, "Invalid perr type. %u", pstPublishLeg->stCall.ucPeerType);
                                break;
                        }

                        if (DOS_ADDR_VALID(pstPublishAgentNode)
                            && DOS_ADDR_VALID(pstPublishAgentNode->pstAgentInfo))
                        {
                            sc_agent_serv_status_update(pstPublishAgentNode->pstAgentInfo, SC_ACD_SERV_RINGING, SC_SRV_TRANSFER);
                        }

                        pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_EXEC;
                    }
                }

                /* 修改主叫坐席的状态 */
                pstNotifyAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulNotifyAgentID);
                if (DOS_ADDR_VALID(pstNotifyAgentNode)
                    && DOS_ADDR_VALID(pstNotifyAgentNode->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstNotifyAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_TRANSFER);
                }

                /* 判断 ulNotifyLegNo 是不是长签 */
                if (pstNotifyLeg->ulIndSCBNo != U32_BUTT)
                {
                    pstSCB->stTransfer.ulNotifyAgentID = 0;
                    pstSCB->stTransfer.ulNotifyLegNo = U32_BUTT;
                    pstNotifyLeg->ulSCBNo = U32_BUTT;
                    sc_req_play_sound(pstNotifyLeg->ulIndSCBNo, pstNotifyLeg->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);

                    /* TODO 这里需要生成转接的话单 */
                }
                else
                {
                    /* 挂断电话 */
                    sc_req_hungup(pstSCB->ulSCBNo, pstNotifyLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                }
            }
            else
            {
                /* 协商转，hold ulSubAgentID */
                sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stTransfer.ulSubLegNo, SC_SND_MUSIC_HOLD, 1, 0, 0);

                if (SC_TRANSFER_PUBLISH_SIP == pstSCB->stTransfer.ulPublishType)
                {
                    /* 转接sip分机 */
                    ulRet = sc_make_call2sip(pstSCB, pstPublishLeg);
                    pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_EXEC;
                }
                else
                {
                    if (pstPublishLeg->ulIndSCBNo != U32_BUTT)
                    {
                        /* 长签，将被叫坐席置忙，放提示音 */
                        if (DOS_ADDR_VALID(pstPublishAgentNode)
                            && DOS_ADDR_VALID(pstPublishAgentNode->pstAgentInfo))
                        {
                            sc_agent_serv_status_update(pstPublishAgentNode->pstAgentInfo, SC_ACD_SERV_CALL_IN, SC_SRV_TRANSFER);
                        }

                        pstPublishLeg->ulSCBNo = pstSCB->ulSCBNo;
                        sc_req_playback_stop(pstSCB->ulSCBNo, pstPublishLeg->ulCBNo);
                        sc_req_play_sound(pstSCB->ulSCBNo, pstPublishLeg->ulCBNo, SC_SND_INCOMING_CALL_TIP, 1, 0, 0);
                        pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_TONE;
                    }
                    else
                    {
                        /* 呼叫需要转接的坐席 */
                        switch (pstPublishLeg->stCall.ucPeerType)
                        {
                            case SC_LEG_PEER_OUTBOUND:
                                ulRet = sc_make_call2pstn(pstSCB, pstPublishLeg);
                                break;

                            case SC_LEG_PEER_OUTBOUND_TT:
                                ulRet = sc_make_call2eix(pstSCB, pstPublishLeg);
                                break;

                            case SC_LEG_PEER_OUTBOUND_INTERNAL:
                                ulRet = sc_make_call2sip(pstSCB, pstPublishLeg);
                                break;

                            default:
                                sc_trace_scb(pstSCB, "Invalid perr type. %u", pstPublishLeg->stCall.ucPeerType);
                                break;
                        }

                        if (DOS_ADDR_VALID(pstPublishAgentNode)
                            && DOS_ADDR_VALID(pstPublishAgentNode->pstAgentInfo))
                        {
                            sc_agent_serv_status_update(pstPublishAgentNode->pstAgentInfo, SC_ACD_SERV_RINGING, SC_SRV_TRANSFER);
                        }

                        pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_EXEC;
                    }
                }

            }

            break;

        case SC_TRANSFER_PROC:
        case SC_TRANSFER_ALERTING:
            pstPublishLeg = sc_lcb_get(pstSCB->stTransfer.ulPublishLegNo);
            if (SC_ACCESS_BLIND_TRANSFER == pstSCB->stTransfer.ulType)
            {
                if (DOS_ADDR_VALID(pstPublishLeg)
                    && !pstPublishLeg->stCall.bEarlyMedia)
                {
                    /* 盲转，将 A 和 C 接通 */
                    if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stTransfer.ulPublishLegNo, pstSCB->stTransfer.ulSubLegNo) != DOS_SUCC)
                    {
                        /* 错误处理 */
                        sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                        ulRet = DOS_FAIL;
                    }
                }

                pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_FINISHED;
            }
            else
            {
                if (DOS_ADDR_VALID(pstPublishLeg)
                    && !pstPublishLeg->stCall.bEarlyMedia)
                {
                    /* 协商转，将B和C接通 */
                    if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stTransfer.ulNotifyLegNo, pstSCB->stTransfer.ulPublishLegNo) != DOS_SUCC)
                    {
                        /* 错误处理 */
                        sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                        ulRet = DOS_FAIL;
                    }
                }
                pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_TRANSFER;
            }

            pstPublishAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulPublishAgentID);
            if (DOS_ADDR_VALID(pstPublishAgentNode)
                && DOS_ADDR_VALID(pstPublishAgentNode->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstPublishAgentNode->pstAgentInfo, SC_ACD_SERV_CALL_IN, SC_SRV_TRANSFER);
            }

            break;

        default:
            break;
    }

    return ulRet;
}

U32 sc_transfer_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstLCB       =  NULL;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing transfer dtmf event. status : %u", pstSCB->stTransfer.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 开启接入码业务 */
    if (pstDTMF->cDTMFVal != '*'
        && pstDTMF->cDTMFVal != '#' )
    {
        /* 第一个字符不是 '*' 或者 '#' 不保存  */
        return DOS_SUCC;
    }

    /* 只有坐席对应的leg执行接入码业务 */
    if (pstSCB->stTransfer.stSCBTag.usStatus != SC_TRANSFER_FINISHED)
    {
        return DOS_SUCC;
    }

    pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stAccessCode.szDialCache[0] = '\0';
    pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
    if (pstSCB->stTransfer.ulNotifyLegNo != U32_BUTT)
    {
        pstSCB->stAccessCode.ulAgentID = pstSCB->stTransfer.ulNotifyAgentID;
    }
    else
    {
        pstSCB->stAccessCode.ulAgentID = pstSCB->stTransfer.ulPublishAgentID;
    }

    pstSCB->ulCurrentSrv++;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;

    return DOS_SUCC;
}

U32 sc_transfer_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* 处理录音结束 */
    SC_MSG_CMD_RECORD_ST *pstRecord = NULL;
    SC_LEG_CB            *pstLCB    = NULL;

    pstRecord = (SC_MSG_CMD_RECORD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstRecord) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstRecord->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        return DOS_FAIL;
    }

    /* 生成录音话单 */
    sc_send_special_billing_stop2bs(pstSCB, pstLCB, BS_SERV_RECORDING);
    sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);

    return DOS_SUCC;
}

U32 sc_transfer_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HUNGUP_ST    *pstEvtCall           = NULL;
    SC_LEG_CB               *psthungLegCB         = NULL;
    SC_LEG_CB               *pstOtherLegCB        = NULL;
    SC_LEG_CB               *pstNotifyLegCB       = NULL;
    SC_AGENT_NODE_ST        *pstNotifyAgentNode   = NULL;
    SC_AGENT_NODE_ST        *pstPublishAgentNode  = NULL;
    SC_AGENT_NODE_ST        *pstSubAgentNode      = NULL;
    SC_AGENT_NODE_ST        *pstHungAgentNode     = NULL;
    SC_AGENT_NODE_ST        *pstOtherAgentNode    = NULL;
    U32                     ulRet                 = DOS_SUCC;
    U32                     ulReleasePart;
    U32                     ulErrCode;

    pstEvtCall = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulErrCode = pstEvtCall->ulErrCode;
    sc_trace_scb(pstSCB, "Processing transfer realse event. status : %u", pstSCB->stTransfer.stSCBTag.usStatus);

    if (pstEvtCall->ulLegNo == pstSCB->stTransfer.ulNotifyLegNo)
    {
        ulReleasePart = SC_CALLING;
    }
    else
    {
        ulReleasePart = SC_CALLEE;
    }

    psthungLegCB = sc_lcb_get(pstEvtCall->ulLegNo);
    if (DOS_ADDR_INVALID(psthungLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 因为盲转和协商转同一状态下的处理可能不相同，故完全分开 */
    if (SC_ACCESS_BLIND_TRANSFER == pstSCB->stTransfer.ulType)
    {
        /* 盲转 */
        switch (pstSCB->stTransfer.stSCBTag.usStatus)
        {
            case SC_TRANSFER_AUTH:
            case SC_TRANSFER_TRANSFERRING:
                /* TODO 异常处理 */
                break;
            case SC_TRANSFER_EXEC:
            case SC_TRANSFER_PROC:
            case SC_TRANSFER_ALERTING:
            case SC_TRANSFER_TONE:
                if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulNotifyLegNo)
                {
                    /* 发起方 挂断，生成转接的话单;生成话单后应该删除转接业务和B对应的业务；如果存在 ulNotifyAgentID，则需要修改坐席的状态 */
                    pstNotifyAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulNotifyAgentID);
                    if (DOS_ADDR_VALID(pstNotifyAgentNode)
                        && DOS_ADDR_VALID(pstNotifyAgentNode->pstAgentInfo))
                    {
                        sc_agent_serv_status_update(pstNotifyAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_TRANSFER);
                    }

                    pstSCB->stTransfer.ulNotifyLegNo = U32_BUTT;
                    pstSCB->stTransfer.ulNotifyAgentID = 0;

                    /* 生成转接的话单，盲转就按1秒计算 */
                    psthungLegCB->stCall.stTimeInfo.ulTransferEndTime = psthungLegCB->stCall.stTimeInfo.ulTransferStartTime + 1;
                    sc_send_special_billing_stop2bs(pstSCB, psthungLegCB, BS_SERV_CALL_TRANSFER);
                    sc_scb_remove_service(pstSCB, BS_SERV_CALL_TRANSFER);

                    /* 出 ulNotifyLegNo 自己的话单  */
                    if (psthungLegCB->ulIndSCBNo == U32_BUTT)
                    {
                        if (psthungLegCB->stCall.ucPeerType == SC_LEG_PEER_INBOUND)
                        {
                            sc_send_special_billing_stop2bs(pstSCB, psthungLegCB, BS_SERV_INBAND_CALL);
                        }
                        else if (psthungLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                        {
                            sc_send_special_billing_stop2bs(pstSCB, psthungLegCB, BS_SERV_OUTBAND_CALL);
                        }
                        else
                        {
                            /* 内部呼叫就不用生成话单了 */
                        }
                    }
                    else
                    {
                        /* 长签时，会在长签业务中生成话单，这里就不需要了 */
                    }
                }
                else
                {
                    /* 修改坐席状态，释放 */
                    pstPublishAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulPublishAgentID);
                    if (DOS_ADDR_VALID(pstPublishAgentNode)
                        && DOS_ADDR_VALID(pstPublishAgentNode->pstAgentInfo))
                    {
                        sc_agent_serv_status_update(pstPublishAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_TRANSFER);
                    }

                    pstSubAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulSubAgentID);
                    if (DOS_ADDR_VALID(pstSubAgentNode)
                        && DOS_ADDR_VALID(pstSubAgentNode->pstAgentInfo))
                    {
                        sc_agent_serv_status_update(pstSubAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_TRANSFER);
                    }

                    pstSCB->stTransfer.ulPublishAgentID = 0;
                    pstSCB->stTransfer.ulSubAgentID = 0;

                    if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulPublishLegNo)
                    {
                        pstSCB->stTransfer.ulPublishLegNo = U32_BUTT;

                        pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulSubLegNo);
                        if (DOS_ADDR_VALID(pstOtherLegCB))
                        {
                            pstOtherLegCB->stCall.stTimeInfo.ulByeTime = psthungLegCB->stCall.stTimeInfo.ulByeTime;
                            /* ulSubLegNo 挂断，生成 ulSubLegNo 对应的话单 */
                            pstOtherLegCB->stCall.ulCause = ulErrCode;
                            sc_send_billing_stop2bs(pstSCB, pstOtherLegCB, NULL, ulReleasePart);
                        }

                        /* 挂断 ulPublishLegNo */
                        sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stTransfer.ulSubLegNo);
                        sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stTransfer.ulSubLegNo, CC_ERR_NORMAL_CLEAR);
                    }
                    else
                    {
                        pstSCB->stTransfer.ulSubLegNo = U32_BUTT;

                        /* ulSubLegNo 挂断，生成 ulSubLegNo 对应的话单 */
                        psthungLegCB->stCall.ulCause = ulErrCode;
                        sc_send_billing_stop2bs(pstSCB, psthungLegCB, NULL, ulReleasePart);

                        /* 挂断 ulPublishLegNo */
                        sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stTransfer.ulPublishLegNo, CC_ERR_NORMAL_CLEAR);
                    }

                    pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_RELEASE;
                }

                if (psthungLegCB->ulIndSCBNo != U32_BUTT)
                {
                    /* 挂断的坐席是长签，这里不需要释放 */
                    psthungLegCB->ulSCBNo = U32_BUTT;
                }
                else
                {
                    sc_lcb_free(psthungLegCB);
                }

                break;

            case SC_TRANSFER_FINISHED:
                /* 正常挂断，判断坐席是否长签，判断是否需要进行客户标记等 */
                if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulSubLegNo)
                {
                    pstHungAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulSubAgentID);
                    pstOtherAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulPublishAgentID);
                    pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulPublishLegNo);

                    if (DOS_ADDR_VALID(pstOtherLegCB))
                    {
                        pstOtherLegCB->stCall.stTimeInfo.ulByeTime = psthungLegCB->stCall.stTimeInfo.ulByeTime;

                        /* 生成话单 */
                        if (SC_LEG_PEER_OUTBOUND == pstOtherLegCB->stCall.ucPeerType)
                        {
                            sc_send_special_billing_stop2bs(pstSCB, pstOtherLegCB, BS_SERV_OUTBAND_CALL);
                        }
                    }
                    psthungLegCB->stCall.ulCause = ulErrCode;
                    sc_send_billing_stop2bs(pstSCB, psthungLegCB, NULL, ulReleasePart);
                }
                else
                {
                    pstHungAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulPublishAgentID);
                    pstOtherAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulSubAgentID);
                    pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulSubLegNo);

                    if (DOS_ADDR_VALID(pstOtherLegCB))
                    {
                        pstOtherLegCB->stCall.stTimeInfo.ulByeTime = psthungLegCB->stCall.stTimeInfo.ulByeTime;
                    }

                    /* 生成话单 */
                    if (SC_LEG_PEER_OUTBOUND == psthungLegCB->stCall.ucPeerType)
                    {
                        sc_send_special_billing_stop2bs(pstSCB, psthungLegCB, BS_SERV_OUTBAND_CALL);
                    }

                    if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
                    {
                        sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
                    }

                    if (DOS_ADDR_VALID(pstOtherLegCB))
                    {
                        pstOtherLegCB->stCall.ulCause = ulErrCode;
                        sc_send_billing_stop2bs(pstSCB, pstOtherLegCB, NULL, ulReleasePart);
                    }
                }

                if (psthungLegCB->ulOtherSCBNo != U32_BUTT)
                {
                    sc_hungup_third_leg(psthungLegCB->ulOtherSCBNo);
                    psthungLegCB->ulOtherSCBNo = U32_BUTT;
                }

                if (DOS_ADDR_VALID(pstOtherLegCB))
                {
                    if (pstOtherLegCB->ulOtherSCBNo != U32_BUTT)
                    {
                        sc_hungup_third_leg(pstOtherLegCB->ulOtherSCBNo);
                        pstOtherLegCB->ulOtherSCBNo = U32_BUTT;
                    }
                }

                if (DOS_ADDR_VALID(pstHungAgentNode) && DOS_ADDR_VALID(pstHungAgentNode->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstHungAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_TRANSFER);
                }

                if (DOS_ADDR_VALID(pstOtherAgentNode) && DOS_ADDR_VALID(pstOtherAgentNode->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstOtherAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_TRANSFER);
                }

                if (DOS_ADDR_VALID(pstOtherLegCB))
                {
                    if (pstOtherLegCB->ulIndSCBNo != U32_BUTT)
                    {
                        /* 长签，继续放音 */
                        pstOtherLegCB->ulSCBNo = U32_BUTT;
                        sc_req_play_sound(pstOtherLegCB->ulIndSCBNo, pstOtherLegCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                        /* 释放掉 SCB */
                        sc_scb_free(pstSCB);
                        pstSCB = NULL;
                        break;
                    }
                    else
                    {
                        sc_req_hungup(pstSCB->ulSCBNo, pstOtherLegCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                    }
                }
                else
                {
                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                    break;
                }

                pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_RELEASE;
                break;

            case SC_TRANSFER_RELEASE:
                if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulSubLegNo)
                {
                    pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulPublishLegNo);
                }
                else
                {
                    pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulSubLegNo);
                }

                sc_lcb_free(psthungLegCB);
                psthungLegCB = NULL;
                if (DOS_ADDR_VALID(pstOtherLegCB))
                {
                    sc_lcb_free(pstOtherLegCB);
                    pstOtherLegCB = NULL;
                }
                sc_scb_free(pstSCB);
                pstSCB = NULL;
                break;

            default:
                break;
        }
    }
    else
    {
        switch (pstSCB->stTransfer.stSCBTag.usStatus)
        {
            case SC_TRANSFER_AUTH:
                break;
            case SC_TRANSFER_EXEC:
            case SC_TRANSFER_PROC:
            case SC_TRANSFER_ALERTING:
            case SC_TRANSFER_TONE:
            case SC_TRANSFER_TRANSFER:
                pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_FINISHED;
                if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulNotifyLegNo)
                {
                    /* 接通 A 和 C，生成 B 的话单 */
                    if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stTransfer.ulPublishLegNo, pstSCB->stTransfer.ulSubLegNo) != DOS_SUCC)
                    {
                        /* 错误处理 */
                        sc_trace_scb(pstSCB, "Bridge call fail.");
                        ulRet = DOS_FAIL;
                        break;
                    }

                    /* 修改 A 对应的坐席的状态 */
                    pstNotifyAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulNotifyAgentID);
                    if (DOS_ADDR_VALID(pstNotifyAgentNode)
                        && DOS_ADDR_VALID(pstNotifyAgentNode->pstAgentInfo))
                    {
                        sc_agent_serv_status_update(pstNotifyAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_TRANSFER);
                    }

                    pstSCB->stTransfer.ulNotifyLegNo = U32_BUTT;
                    pstSCB->stTransfer.ulNotifyAgentID = 0;

                    psthungLegCB->stCall.stTimeInfo.ulTransferEndTime = psthungLegCB->stCall.stTimeInfo.ulByeTime;
                    /* 生成转接换单 */
                    sc_send_special_billing_stop2bs(pstSCB, psthungLegCB, BS_SERV_CALL_TRANSFER);
                    sc_scb_remove_service(pstSCB, BS_SERV_CALL_TRANSFER);

                    /* 出 ulNotifyLegNo 自己的话单  */
                    if (psthungLegCB->ulIndSCBNo == U32_BUTT)
                    {
                        if (psthungLegCB->stCall.ucPeerType == SC_LEG_PEER_INBOUND)
                        {
                            sc_send_special_billing_stop2bs(pstSCB, psthungLegCB, BS_SERV_INBAND_CALL);
                            sc_scb_remove_service(pstSCB, BS_SERV_INBAND_CALL);
                        }
                        else if (psthungLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                        {
                            sc_send_special_billing_stop2bs(pstSCB, psthungLegCB, BS_SERV_OUTBAND_CALL);
                            sc_scb_remove_service(pstSCB, BS_SERV_OUTBAND_CALL);
                        }
                        else
                        {
                            /* 内部呼叫就不用生成话单了 */
                        }
                    }
                    else
                    {
                        /* 长签时，会在长签业务中生成话单，这里就不需要了 */
                    }
                }
                else if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulPublishLegNo)
                {
                    /* 第三方挂断，重新连接 A 和 B */
                    if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stTransfer.ulNotifyLegNo, pstSCB->stTransfer.ulSubLegNo) != DOS_SUCC)
                    {
                        /* 错误处理 */
                        sc_trace_scb(pstSCB, "Bridge call fail.");
                        ulRet = DOS_FAIL;
                        break;
                    }

                    /* 修改 C 对应的坐席的状态 */
                    pstPublishAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulPublishAgentID);
                    if (DOS_ADDR_VALID(pstPublishAgentNode)
                        && DOS_ADDR_VALID(pstPublishAgentNode->pstAgentInfo))
                    {
                        sc_agent_serv_status_update(pstPublishAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_TRANSFER);
                    }

                    pstSCB->stTransfer.ulPublishLegNo = U32_BUTT;
                    pstSCB->stTransfer.ulPublishAgentID = 0;

                    pstNotifyLegCB = sc_lcb_get(pstSCB->stTransfer.ulNotifyLegNo);
                    if (DOS_ADDR_VALID(pstNotifyLegCB))
                    {
                        pstNotifyLegCB->stCall.stTimeInfo.ulTransferEndTime = psthungLegCB->stCall.stTimeInfo.ulByeTime;
                        /* 生成转接换单 */
                        sc_send_special_billing_stop2bs(pstSCB, psthungLegCB, BS_SERV_CALL_TRANSFER);
                        sc_scb_remove_service(pstSCB, BS_SERV_CALL_TRANSFER);
                    }

                    /* 生成C的话单，呼叫C的时候没有将C的业务存放到scb中,所以这里不用删除 */
                    if (psthungLegCB->ulIndSCBNo == U32_BUTT)
                    {
                        if (psthungLegCB->stCall.ucPeerType == SC_LEG_PEER_INBOUND)
                        {
                            sc_send_special_billing_stop2bs(pstSCB, psthungLegCB, BS_SERV_INBAND_CALL);
                        }
                        else if (psthungLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                        {
                            sc_send_special_billing_stop2bs(pstSCB, psthungLegCB, BS_SERV_OUTBAND_CALL);
                        }
                        else
                        {
                            /* 内部呼叫就不用生成话单了 */
                        }
                    }
                }
                else
                {
                    /* 第一方挂断，通话结束了，将其它的都挂断吧 */
                    pstSubAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulSubAgentID);
                    if (DOS_ADDR_VALID(pstSubAgentNode)
                        && DOS_ADDR_VALID(pstSubAgentNode->pstAgentInfo))
                    {
                        sc_agent_serv_status_update(pstSubAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_TRANSFER);
                    }

                    pstSCB->stTransfer.ulSubLegNo = U32_BUTT;
                    pstSCB->stTransfer.ulSubAgentID = 0;

                    /* 挂断发起方吧 */
                    sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stTransfer.ulNotifyLegNo, CC_ERR_NORMAL_CLEAR);

                    /* 生成转接话单 */
                    pstNotifyLegCB = sc_lcb_get(pstSCB->stTransfer.ulNotifyLegNo);
                    if (DOS_ADDR_VALID(pstNotifyLegCB))
                    {
                        pstNotifyLegCB->stCall.stTimeInfo.ulTransferEndTime = psthungLegCB->stCall.stTimeInfo.ulByeTime;
                        /* 生成转接换单 */
                        sc_send_special_billing_stop2bs(pstSCB, psthungLegCB, BS_SERV_CALL_TRANSFER);
                        sc_scb_remove_service(pstSCB, BS_SERV_CALL_TRANSFER);
                    }

                    psthungLegCB->stCall.ulCause = ulErrCode;
                    sc_send_billing_stop2bs(pstSCB, psthungLegCB, NULL, ulReleasePart);
                }

                if (psthungLegCB->ulIndSCBNo != U32_BUTT)
                {
                    /* 挂断的坐席是长签，这里不需要释放 */
                    psthungLegCB->ulSCBNo = U32_BUTT;
                }
                else
                {
                    sc_lcb_free(psthungLegCB);
                }

                break;

            case SC_TRANSFER_FINISHED:
                /* 在这个状态，一定只有两通电话而且必须有被转接的那个电话 */
                if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulSubLegNo)
                {
                    pstHungAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulSubAgentID);
                    if (pstSCB->stTransfer.ulPublishLegNo != U32_BUTT)
                    {
                        pstOtherAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulPublishAgentID);
                        pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulPublishLegNo);
                    }
                    else
                    {
                        pstOtherAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulNotifyAgentID);
                        pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulNotifyLegNo);
                    }

                    if (DOS_ADDR_VALID(pstOtherLegCB))
                    {
                        pstOtherLegCB->stCall.stTimeInfo.ulByeTime = psthungLegCB->stCall.stTimeInfo.ulByeTime;

                        /* 生成话单 */
                        if (SC_LEG_PEER_OUTBOUND == pstOtherLegCB->stCall.ucPeerType)
                        {
                            sc_send_special_billing_stop2bs(pstSCB, pstOtherLegCB, BS_SERV_OUTBAND_CALL);
                        }
                    }
                    psthungLegCB->stCall.ulCause = ulErrCode;
                    sc_send_billing_stop2bs(pstSCB, psthungLegCB, NULL, ulReleasePart);
                }
                else
                {
                    if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulPublishLegNo)
                    {
                        pstHungAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulPublishAgentID);
                    }
                    else
                    {
                        pstHungAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulNotifyAgentID);
                    }

                    pstOtherAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulSubAgentID);
                    pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulSubLegNo);

                    if (DOS_ADDR_VALID(pstOtherLegCB))
                    {
                        pstOtherLegCB->stCall.stTimeInfo.ulByeTime = psthungLegCB->stCall.stTimeInfo.ulByeTime;
                    }

                    /* 生成话单 */
                    if (SC_LEG_PEER_OUTBOUND == psthungLegCB->stCall.ucPeerType)
                    {
                        sc_send_special_billing_stop2bs(pstSCB, psthungLegCB, BS_SERV_OUTBAND_CALL);
                    }

                    if (DOS_ADDR_VALID(pstOtherLegCB))
                    {
                        pstOtherLegCB->stCall.ulCause = ulErrCode;
                        sc_send_billing_stop2bs(pstSCB, pstOtherLegCB, NULL, ulReleasePart);
                    }
                }

                if (DOS_ADDR_VALID(pstHungAgentNode) && DOS_ADDR_VALID(pstHungAgentNode->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstHungAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_TRANSFER);
                }

                if (DOS_ADDR_VALID(pstOtherAgentNode) && DOS_ADDR_VALID(pstOtherAgentNode->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstOtherAgentNode->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_TRANSFER);
                }

                if (DOS_ADDR_VALID(pstOtherLegCB))
                {
                    if (pstOtherLegCB->ulIndSCBNo != U32_BUTT)
                    {
                        /* 长签，继续放音 */
                        pstOtherLegCB->ulSCBNo = U32_BUTT;
                        sc_req_play_sound(pstOtherLegCB->ulIndSCBNo, pstOtherLegCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                        /* 释放掉 SCB */
                        sc_scb_free(pstSCB);
                        pstSCB = NULL;
                        break;
                    }
                    else
                    {
                        sc_req_hungup(pstSCB->ulSCBNo, pstOtherLegCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                    }
                }
                else
                {
                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                    break;
                }

                pstSCB->stTransfer.stSCBTag.usStatus = SC_TRANSFER_RELEASE;
                break;
            case SC_TRANSFER_RELEASE:
                if (psthungLegCB->ulCBNo == pstSCB->stTransfer.ulSubLegNo)
                {
                    pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulPublishLegNo);
                }
                else
                {
                    pstOtherLegCB = sc_lcb_get(pstSCB->stTransfer.ulSubLegNo);
                }

                sc_lcb_free(psthungLegCB);
                psthungLegCB = NULL;
                if (DOS_ADDR_VALID(pstOtherLegCB))
                {
                    sc_lcb_free(pstOtherLegCB);
                    pstOtherLegCB = NULL;
                }
                sc_scb_free(pstSCB);

                break;
            default:
                break;
        }
    }

    return DOS_SUCC;
}

U32 sc_transfer_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing transfer call error event. status : %u", pstSCB->stTransfer.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */
        if (!sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
        {
            return DOS_SUCC;
        }

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stTransfer.stSCBTag.usStatus)
    {
        case SC_TRANSFER_IDEL:
        case SC_TRANSFER_AUTH:
        case SC_TRANSFER_TRANSFERRING:
            break;

        case SC_TRANSFER_EXEC:
        case SC_TRANSFER_PROC:
        case SC_TRANSFER_ALERTING:
            /* TODO 暂时不处理 */
            //ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stDemoTask.ulCallingLegNo, ulErrCode);
            break;

        case SC_TRANSFER_TONE:
            break;
        case SC_TRANSFER_TRANSFER:
        case SC_TRANSFER_FINISHED:
        case SC_TRANSFER_RELEASE:
            /* 暂时不处理 */
            break;

        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_demo_task_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB = NULL;
    SC_LEG_CB                  *pstCalleeLegCB = NULL;
    U32                         ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing demo task auth event. status : %u", pstSCB->stDemoTask.stSCBTag.usStatus);

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* 注意通过偏移量，找到CC统一定义的错误码 */
        pstLegCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
        if (DOS_ADDR_VALID(pstLegCB))
        {
            sc_lcb_free(pstLegCB);
        }
        sc_scb_free(pstSCB);

        return DOS_SUCC;
    }

    switch (pstSCB->stDemoTask.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_AUTH:
            pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_EXEC;
            pstLegCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstLegCB))
            {
                sc_scb_free(pstSCB);

                DOS_ASSERT(0);

                return DOS_FAIL;
            }
            ulRet = sc_make_call2pstn(pstSCB, pstLegCB);
            break;
        case SC_AUTO_CALL_AUTH2:
            /* 呼叫坐席时，进行的认证，呼叫坐席 */
            pstCalleeLegCB = sc_lcb_get(pstSCB->stDemoTask.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCalleeLegCB))
            {
                /* TODO */
                return DOS_FAIL;
            }
            pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_EXEC2;
            ulRet = sc_make_call2pstn(pstSCB, pstCalleeLegCB);
            break;

        default:
            break;
    }

    return ulRet;
}

U32 sc_demo_task_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32             ulRet   = DOS_FAIL;
    SC_LEG_CB       *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing demo task stup event. status : %u", pstSCB->stDemoTask.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stDemoTask.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
            /* 未认证通过，放音挂断呼叫 */
            goto proc_finishe;
            break;

        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PROC:
        case SC_AUTO_CALL_ALERTING:
            /* 迁移状态到proc */
            pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_AUTO_CALL_ACTIVE:
        case SC_AUTO_CALL_AFTER_KEY:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_EXEC2:
            /* 坐席的leg创建 */
            pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_PORC2;
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_demo_task_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ANSWER_ST *pstEvtAnswer = NULL;
    U32          ulRet              = DOS_FAIL;
    SC_LEG_CB    *pstLCB            = NULL;
    SC_LEG_CB    *pstCalleeLegCB    = NULL;
    SC_MSG_CMD_PLAYBACK_ST  stPlaybackRsp;
    U32          ulErrCode   = CC_ERR_NO_REASON;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing demo task answer event. status : %u", pstSCB->stDemoTask.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stDemoTask.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
            /* 未认证通过，放音挂断呼叫 */
            goto proc_finishe;
            break;

        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PROC:
        case SC_AUTO_CALL_ALERTING:
            /* 播放语音 */
            stPlaybackRsp.stMsgTag.ulMsgType = SC_CMD_PLAYBACK;
            stPlaybackRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stPlaybackRsp.stMsgTag.usInterErr = 0;
            stPlaybackRsp.ulMode = 0;
            stPlaybackRsp.ulSCBNo = pstSCB->ulSCBNo;
            stPlaybackRsp.ulLegNo = pstLCB->ulCBNo;
            stPlaybackRsp.ulLoopCnt = SC_DEMOE_TASK_COUNT;
            stPlaybackRsp.ulInterval = 0;
            stPlaybackRsp.ulSilence  = 0;
            stPlaybackRsp.enType = SC_CND_PLAYBACK_FILE;
            stPlaybackRsp.blNeedDTMF = DOS_TRUE;
            stPlaybackRsp.ulTotalAudioCnt++;

            dos_strncpy(stPlaybackRsp.szAudioFile, SC_DEMOE_TASK_FILE, SC_MAX_AUDIO_FILENAME_LEN-1);
            stPlaybackRsp.szAudioFile[SC_MAX_AUDIO_FILENAME_LEN - 1] = '\0';

            if (sc_send_cmd_playback(&stPlaybackRsp.stMsgTag) != DOS_SUCC)
            {
                ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                goto proc_finishe;
            }

            pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_ACTIVE;
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_ACTIVE:
        case SC_AUTO_CALL_AFTER_KEY:
            /* TODO */
            break;
        case SC_AUTO_CALL_ALERTING2:
            pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_CONNECTED;
            pstEvtAnswer = (SC_MSG_EVT_ANSWER_ST *)pstMsg;

            /* 应答主叫 */
            sc_req_answer_call(pstSCB->ulSCBNo, pstSCB->stDemoTask.ulCallingLegNo);

            /* 连接主被叫 */
            pstCalleeLegCB = sc_lcb_get(pstEvtAnswer->ulLegNo);
            if (DOS_ADDR_VALID(pstCalleeLegCB)
                && !pstCalleeLegCB->stCall.bEarlyMedia)
            {
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stDemoTask.ulCalleeLegNo, pstSCB->stDemoTask.ulCallingLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    ulRet = DOS_FAIL;
                    goto proc_finishe;
                }
            }
            break;
        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto call answer event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        /* TODO 失败的处理 */
    }

    return ulRet;
}

U32 sc_demo_task_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32         ulRet   = DOS_FAIL;
    SC_MSG_EVT_RINGING_ST *pstEvent = NULL;
    SC_LEG_CB   *pstCalleeLegCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEvent = (SC_MSG_EVT_RINGING_ST *)pstMsg;

    sc_trace_scb(pstSCB, "Processing demo task ringing event. status : %u. with media: %u", pstSCB->stDemoTask.stSCBTag.usStatus, pstEvent->ulWithMedia);

    switch (pstSCB->stDemoTask.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
            /* 未认证通过，放音挂断呼叫 */
            goto proc_finishe;
            break;

        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PROC:
        case SC_AUTO_CALL_ALERTING:
            pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_ALERTING;
            ulRet = DOS_SUCC;
            break;

        case SC_AUTO_CALL_ACTIVE:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_AFTER_KEY:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_PORC2:
        case SC_AUTO_CALL_ALERTING2:
            /* 坐席振铃 */
            sc_req_playback_stop(pstSCB->ulSCBNo, pstSCB->stDemoTask.ulCallingLegNo);
            sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stDemoTask.ulCallingLegNo, DOS_TRUE, pstEvent->ulWithMedia);

            if (pstEvent->ulWithMedia)
            {
                pstCalleeLegCB = sc_lcb_get(pstSCB->stDemoTask.ulCalleeLegNo);
                if (DOS_ADDR_VALID(pstCalleeLegCB))
                {
                    pstCalleeLegCB->stCall.bEarlyMedia = DOS_TRUE;

                    if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stDemoTask.ulCalleeLegNo, pstSCB->stDemoTask.ulCallingLegNo) != DOS_SUCC)
                    {
                        sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                        ulRet = DOS_FAIL;
                        goto proc_finishe;
                    }
                }
            }

            pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_ALERTING2;
            break;

        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto call ringing event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_demo_task_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstLCB       = NULL;
    S32                   lKey          = 0;
    SC_AGENT_NODE_ST      *pstAgentNode = NULL;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing demo task dtmf event. status : %u", pstSCB->stDemoTask.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    lKey = pstDTMF->cDTMFVal - '0';

    switch (pstSCB->stDemoTask.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_ACTIVE:
            /* 呼叫坐席 */
            pstAgentNode = sc_agent_get_by_id(pstSCB->stDemoTask.ulAgentID);
            if (DOS_ADDR_INVALID(pstAgentNode)
                || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
            {
                /* TODO 没找到坐席 */
            }
            else
            {
                pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_EXEC2;
                sc_demo_task_callback(pstSCB, pstAgentNode);
            }
            break;

         case SC_AUTO_CALL_CONNECTED:
            /* 开启接入码业务 */
            if (pstDTMF->cDTMFVal != '*'
                && pstDTMF->cDTMFVal != '#' )
            {
                /* 第一个字符不是 '*' 或者 '#' 不保存  */
                return DOS_SUCC;
            }

            /* 只有坐席对应的leg执行接入码业务 */
            if (pstDTMF->ulLegNo != pstSCB->stDemoTask.ulCalleeLegNo)
            {
                return DOS_SUCC;
            }

            pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
            pstSCB->stAccessCode.szDialCache[0] = '\0';
            pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
            pstSCB->stAccessCode.ulAgentID = pstSCB->stDemoTask.ulAgentID;
            pstSCB->ulCurrentSrv++;
            pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;

            break;

         default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_demo_task_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HOLD_ST   *pstHold        = NULL;
    SC_LEG_CB            *pstLeg         = NULL;

    pstHold = (SC_MSG_EVT_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstHold->bIsHold)
    {
        /* 如果是被HOLD的，需要激活HOLD业务哦 */
        pstSCB->stHold.stSCBTag.bValid = DOS_TRUE;
        pstSCB->stHold.stSCBTag.bWaitingExit = DOS_FALSE;
        pstSCB->stHold.stSCBTag.usStatus = SC_HOLD_ACTIVE;
        pstSCB->stHold.ulCallLegNo = pstHold->ulLegNo;
        pstSCB->stHold.ulHoldCount++;

        pstSCB->ulCurrentSrv++;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stHold.stSCBTag;

        /* 给HOLD方 播放拨号音 */
        /* 给HOLD对方 播放呼叫保持音 */
    }
    else
    {
        /* 如果是被UNHOLD的，已经没有HOLD业务了，单纯处理呼叫就好 */
        pstLeg = sc_lcb_get(pstHold->ulLegNo);
        if (DOS_ADDR_INVALID(pstLeg))
        {
            return DOS_FAIL;
        }

        if (pstLeg->stHold.ulHoldTime != 0
            && pstLeg->stHold.ulUnHoldTime > pstLeg->stHold.ulHoldTime)
        {
            pstSCB->stHold.ulHoldTotalTime += (pstLeg->stHold.ulUnHoldTime - pstLeg->stHold.ulHoldTime);
        }

        pstLeg->stHold.ulUnHoldTime = 0;
        pstLeg->stHold.ulHoldTime = 0;
    }

    return DOS_SUCC;
}

U32 sc_demo_task_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* 处理录音结束 */
    SC_MSG_CMD_RECORD_ST *pstRecord = NULL;
    SC_LEG_CB            *pstLCB    = NULL;

    pstRecord = (SC_MSG_CMD_RECORD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstRecord) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstRecord->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        return DOS_FAIL;
    }

    /* 生成录音话单 */
    sc_send_special_billing_stop2bs(pstSCB, pstLCB, BS_SERV_RECORDING);
    sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);

    return DOS_SUCC;
}

U32 sc_demo_task_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                     ulRet           = DOS_FAIL;
    SC_LEG_CB               *pstCallingCB   = NULL;
    SC_LEG_CB               *pstCalleeCB    = NULL;
    SC_LEG_CB               *pstHungupLeg   = NULL;
    SC_LEG_CB               *pstOtherLeg    = NULL;
    SC_MSG_EVT_HUNGUP_ST    *pstHungup      = NULL;
    SC_AGENT_NODE_ST        *pstAgentCall   = NULL;
    S32                     i               = 0;
    S32                     lRes            = DOS_FAIL;
    U32                     ulReleasePart;
    U32                     ulErrCode;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;

    ulErrCode = pstHungup->ulErrCode;

    sc_trace_scb(pstSCB, "Proccessing demo task hungup event. status : %u", pstSCB->stDemoTask.stSCBTag.usStatus);

    if (pstHungup->ulLegNo == pstSCB->stDemoTask.ulCallingLegNo)
    {
        ulReleasePart = SC_CALLING;
    }
    else
    {
        ulReleasePart = SC_CALLEE;
    }

    switch (pstSCB->stDemoTask.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PROC:
        case SC_AUTO_CALL_ALERTING:
            /* 这个时候挂断只会是客户的LEG清理资源即可 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent not connected.");

            pstCallingCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
            }

            sc_scb_free(pstSCB);
            break;
        case SC_AUTO_CALL_ACTIVE:
            /* 客户挂断了电话，需要生成话单 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent not connected. Need create cdr");

            /* 生成话单 */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            pstCallingCB->stCall.ulCause = ulErrCode;
            sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);

            pstCallingCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
            }

            sc_scb_free(pstSCB);
            break;
        case SC_AUTO_CALL_AFTER_KEY:
        case SC_AUTO_CALL_AUTH2:
        case SC_AUTO_CALL_EXEC2:
        case SC_AUTO_CALL_PORC2:
        case SC_AUTO_CALL_ALERTING2:
            /* 这个时候挂断，可能是坐席也可能客户，如果是客户需要注意LEG的状态 */
            pstCallingCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stDemoTask.ulCalleeLegNo);
            if (pstHungup->ulLegNo == pstSCB->stDemoTask.ulCallingLegNo)
            {
                /* 客户挂断的，判断是否存在坐席那条leg，如果存在，判断坐席是否长签等 */
                /* 生成话单 */
                if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
                {
                    sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
                }

                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);

                if (DOS_ADDR_INVALID(pstCalleeCB))
                {
                    /* 不存在坐席，直接释放资源就行了 */
                    if (DOS_ADDR_INVALID(pstCallingCB))
                    {
                        sc_lcb_free(pstCallingCB);
                    }
                    sc_scb_free(pstSCB);
                    break;
                }

                pstAgentCall = sc_agent_get_by_id(pstSCB->stDemoTask.ulAgentID);
                if (DOS_ADDR_INVALID(pstAgentCall) || DOS_ADDR_INVALID(pstAgentCall->pstAgentInfo))
                {
                    /* 全部释放掉 */
                    if (DOS_ADDR_INVALID(pstCallingCB))
                    {
                        sc_lcb_free(pstCallingCB);
                        pstCallingCB = NULL;
                    }
                    sc_lcb_free(pstCalleeCB);
                    sc_scb_free(pstSCB);
                }

                /* 坐席置闲 */
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_DEMO_TASK);

                if (pstCalleeCB->ulIndSCBNo != U32_BUTT)
                {
                    /* 坐席长签，继续放音 */
                    pstCalleeCB->ulSCBNo = U32_BUTT;
                    if (DOS_ADDR_VALID(pstCallingCB))
                    {
                        sc_lcb_free(pstCallingCB);
                        pstCallingCB = NULL;
                    }
                    sc_scb_free(pstSCB);
                    sc_req_playback_stop(pstSCB->ulSCBNo, pstCalleeCB->ulCBNo);
                    sc_req_play_sound(pstCalleeCB->ulIndSCBNo, pstCalleeCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                    break;
                }

                pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                if (DOS_ADDR_VALID(pstCallingCB))
                {
                    sc_lcb_free(pstCallingCB);
                    pstCallingCB = NULL;
                }
                pstSCB->stDemoTask.ulCalleeLegNo = U32_BUTT;
                pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
                /* 可能在放回铃音，需要手动停止 */
                sc_req_playback_stop(pstSCB->ulSCBNo, pstCalleeCB->ulCBNo);
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstCalleeCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
            }
            else
            {
                /* 坐席先挂断的，挂断客户的电话，生成话单，修改坐席状态 */
                if (DOS_ADDR_INVALID(pstCallingCB) || DOS_ADDR_INVALID(pstCalleeCB))
                {
                    /* TODO 错误处理 */
                    break;
                }

                pstCallingCB->stCall.stTimeInfo.ulByeTime = pstCalleeCB->stCall.stTimeInfo.ulByeTime;
                if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
                {
                    sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
                }

                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);

                pstAgentCall = sc_agent_get_by_id(pstSCB->stDemoTask.ulAgentID);
                if (DOS_ADDR_VALID(pstAgentCall) && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_DEMO_TASK);
                    if (pstCalleeCB->ulIndSCBNo != U32_BUTT)
                    {
                        /* 长签的电话挂断了，取消关系就行了，不用释放 */
                        pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                        pstSCB->stDemoTask.ulCalleeLegNo = U32_BUTT;
                        pstCalleeCB->ulSCBNo = U32_BUTT;
                    }
                    else
                    {
                        pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                        pstSCB->stDemoTask.ulCalleeLegNo = U32_BUTT;
                        sc_lcb_free(pstCalleeCB);
                    }
                }

                /* 挂断客户的电话 */
                sc_req_playback_stop(pstSCB->ulSCBNo, pstCallingCB->ulCBNo);
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstCallingCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
            }
            break;

        case SC_AUTO_CALL_CONNECTED:
            /* 这个时候挂断，就是正常释放的节奏，处理完就好，判断坐席的状态 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent connected.");
            pstCallingCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stDemoTask.ulCalleeLegNo);
            if (pstSCB->stDemoTask.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCalleeCB;
                pstOtherLeg  = pstCallingCB;
                pstCallingCB->stCall.stTimeInfo.ulByeTime = pstCalleeCB->stCall.stTimeInfo.ulByeTime;
            }
            else
            {
                pstHungupLeg = pstCallingCB;
                pstOtherLeg  = pstCalleeCB;
                pstCalleeCB->stCall.stTimeInfo.ulByeTime = pstCallingCB->stCall.stTimeInfo.ulByeTime;
            }

            if (pstCallingCB->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCallingCB->ulOtherSCBNo);
                pstCallingCB->ulOtherSCBNo = U32_BUTT;
            }

            if (pstCalleeCB->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCalleeCB->ulOtherSCBNo);
                pstCalleeCB->ulOtherSCBNo = U32_BUTT;
            }

            /* 生成话单 */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            if (sc_scb_is_exit_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                /* 如果有出局呼叫，应该先将出局呼叫删除，应为出局呼叫是 */
                sc_scb_remove_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);
                /* 出局呼叫的话单应该用坐席那条leg产生 */
                sc_scb_remove_service(pstSCB, BS_SERV_AUTO_DIALING);
                sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCalleeCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalleeCB, NULL, ulReleasePart);
            }
            else
            {
                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);
            }

            /* 到这里，说明两个leg都OK */
            /*
              * 需要看看是否长签等问题，如果主/被叫LEG都长签了，需要申请SCB，将被叫LEG挂到新的SCB中
              * 否则，将需要长签的LEG作为当前业务控制块的主叫LEG，挂断另外一条LEG
              * 可能需要处理客户标记
              */
            /* release 时，肯定是有一条leg hungup了，现在的leg需要释放掉，判断另一条是不是坐席长签，如果不是需要挂断 */
            pstAgentCall = sc_agent_get_by_id(pstSCB->stDemoTask.ulAgentID);
            if (DOS_ADDR_INVALID(pstAgentCall) || DOS_ADDR_INVALID(pstAgentCall->pstAgentInfo))
            {
                /* 没有找到坐席 */
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Can not found agent by id(%u)", pstSCB->stDemoTask.ulAgentID);
            }

            /* 判断是否需要进行，客户标记。1、是客户一端先挂断的(基础呼叫中，客户只能是PSTN，坐席只能是SIP) */
            if (pstHungupLeg == pstCallingCB
                && DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && pstAgentCall->pstAgentInfo->ucProcesingTime != 0
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                /* 客户标记 */
                pstSCB->stMarkCustom.stSCBTag.bValid = DOS_TRUE;
                pstSCB->stMarkCustom.ulLegNo = pstOtherLeg->ulCBNo;
                pstSCB->stMarkCustom.pstAgentCall = pstAgentCall;
                pstSCB->ulCurrentSrv++;
                pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stMarkCustom.stSCBTag;

                if (pstOtherLeg->ulIndSCBNo == U32_BUTT)
                {
                    /* 非长签时，要把坐席对应的leg的结束时间，赋值给开始时间，出标记话单时使用 */
                    pstOtherLeg->stCall.stTimeInfo.ulAnswerTime = pstHungupLeg->stCall.stTimeInfo.ulByeTime;
                    for (i=0; i<SC_MAX_SERVICE_TYPE; i++)
                    {
                        pstSCB->aucServType[i] = 0;
                    }

                    if (pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);
                    }
                    else if(pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                    }
                    else
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
                    }

                    /* 将客户的号码改为主叫号码 */
                    dos_strcpy(pstOtherLeg->stCall.stNumInfo.szOriginalCalling, pstAgentCall->pstAgentInfo->szLastCustomerNum);
                }

                /* 修改坐席状态为 proc，播放 标记背景音 */
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_PROC, SC_SRV_DEMO_TASK);
                sc_req_play_sound(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, SC_SND_CALL_OVER, 1, 0, 0);
                pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_IDEL;

                /* 开启定时器 */
                lRes = dos_tmr_start(&pstSCB->stMarkCustom.stTmrHandle, pstAgentCall->pstAgentInfo->ucProcesingTime * 1000, sc_agent_mark_custom_callback, (U64)pstOtherLeg->ulCBNo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    pstSCB->stMarkCustom.stTmrHandle = NULL;
                }

                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;

                if (pstSCB->stDemoTask.ulCalleeLegNo == pstHungup->ulLegNo)
                {
                    pstSCB->stDemoTask.ulCalleeLegNo = U32_BUTT;
                }
                else
                {
                    pstSCB->stDemoTask.ulCallingLegNo = U32_BUTT;
                }

                pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_PROCESS;

                break;
            }

            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                pstAgentCall->pstAgentInfo->bMarkCustomer = DOS_FALSE;
            }

            /* 不需要客户标记 */
            if (pstSCB->stDemoTask.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstSCB->stDemoTask.ulCalleeLegNo = U32_BUTT;
            }
            else
            {
                pstSCB->stDemoTask.ulCallingLegNo = U32_BUTT;
            }

            /* 修改坐席的状态 */
            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_DEMO_TASK);
            }

            if (pstOtherLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签，继续放音 */
                pstOtherLeg->ulSCBNo = U32_BUTT;
                sc_req_play_sound(pstOtherLeg->ulIndSCBNo, pstOtherLeg->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                /* 释放掉 SCB */
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
            else if (pstHungupLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签的坐席挂断了电话，不要释放leg，解除关系就行 */
                pstSCB->stDemoTask.ulCallingLegNo = U32_BUTT;
                pstHungupLeg->ulCBNo = U32_BUTT;
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
            }
            else
            {
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
            }
            break;

        case SC_AUTO_CALL_PROCESS:
            /* 坐席处理完了，挂断 */
            pstCalleeCB = sc_lcb_get(pstSCB->stDemoTask.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;
        case SC_AUTO_CALL_RELEASE:
            pstCalleeCB = sc_lcb_get(pstSCB->stDemoTask.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;

            break;
        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call hungup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed auto call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

U32 sc_demo_task_palayback_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* 判断一下群呼任务的模式，如果是呼叫后，转坐席，则转坐席，否则通话结束 */
    SC_LEG_CB              *pstLCB          = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstRlayback     = NULL;
    U32                    ulRet            = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the demo task playback stop msg. status: %u", pstSCB->stDemoTask.stSCBTag.usStatus);

    pstRlayback = (SC_MSG_EVT_PLAYBACK_ST *)pstMsg;

    pstLCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stDemoTask.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_ACTIVE:
            if (pstLCB->stPlayback.usStatus == SC_SU_PLAYBACK_INIT)
            {
                /* 这里是 silence_stream 延时的stop事件，不需要处理。 这里存在问题，正常的playstop事件没有上传 */
                ulRet = DOS_SUCC;

                goto proc_finishe;
            }

            /* 挂断电话 */
            pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
            sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stDemoTask.ulCallingLegNo, CC_ERR_NORMAL_CLEAR);

            break;

        case SC_AUTO_CALL_TONE:
            if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stDemoTask.ulCalleeLegNo, pstSCB->stDemoTask.ulCallingLegNo) != DOS_SUCC)
            {
                sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                ulRet = DOS_FAIL;
            }

            pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_CONNECTED;
            break;
        default:
            break;
    }

proc_finishe:

    sc_trace_scb(pstSCB, "Proccessed auto call playk stop event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

U32 sc_demo_task_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;
    SC_LEG_CB                   *pstCallingCB       = NULL;
    SC_LEG_CB                   *pstCalleeCB        = NULL;
    SC_LEG_CB                   *pstRecordLegCB     = NULL;
    SC_MSG_CMD_RECORD_ST        stRecordRsp;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing demo task call error event. status : %u", pstSCB->stDemoTask.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */
        if (!sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
        {
            return DOS_SUCC;
        }

        pstCalleeCB = sc_lcb_get(pstSCB->stDemoTask.ulCalleeLegNo);
        if (DOS_ADDR_VALID(pstCalleeCB)
            && pstCalleeCB->stRecord.bValid)
        {
            pstRecordLegCB = pstCalleeCB;
        }
        else
        {
            pstCallingCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB) && pstCallingCB->stRecord.bValid)
            {
                pstRecordLegCB = pstCallingCB;
            }
        }

        if (DOS_ADDR_VALID(pstRecordLegCB))
        {
            stRecordRsp.stMsgTag.ulMsgType = SC_CMD_RECORD;
            stRecordRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stRecordRsp.stMsgTag.usInterErr = 0;
            stRecordRsp.ulSCBNo = pstSCB->ulSCBNo;
            stRecordRsp.ulLegNo = pstRecordLegCB->ulCBNo;

            if (sc_send_cmd_record(&stRecordRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_INFO, SC_MOD_EVENT, SC_LOG_DISIST), "Send record cmd FAIL! SCBNo : %u", pstSCB->ulSCBNo);
            }
        }

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stDemoTask.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
        case SC_AUTO_CALL_EXEC:
            /* 发起呼叫失败，直接释放资源 */
            pstCallingCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
            }
            sc_scb_free(pstSCB);
            break;

        case SC_AUTO_CALL_PROC:
        case SC_AUTO_CALL_ALERTING:
            ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stDemoTask.ulCallingLegNo, ulErrCode);
            break;

        case SC_AUTO_CALL_ACTIVE:
        case SC_AUTO_CALL_AFTER_KEY:
        case SC_AUTO_CALL_AUTH2:
        case SC_AUTO_CALL_EXEC2:
        case SC_AUTO_CALL_PORC2:
        case SC_AUTO_CALL_ALERTING2:
            ulRet = sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stDemoTask.ulCallingLegNo, ulErrCode);
            break;

        case SC_AUTO_CALL_TONE:
            break;

        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
        case SC_AUTO_CALL_RELEASE:
            if (pstSCB->stDemoTask.ulCalleeLegNo != U32_BUTT)
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stDemoTask.ulCalleeLegNo, ulErrCode);
            }
            else
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stDemoTask.ulCallingLegNo, ulErrCode);
            }
            break;

        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_hold_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HOLD_ST  *pstHold        = NULL;
    U32                 ulRet           = DOS_SUCC;
    U32                 ulLegCBNo       = U32_BUTT;
    U32                 ulOtherLegNo    = U32_BUTT;

    pstHold = (SC_MSG_EVT_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing hold hold event. status : %u", pstSCB->stHold.stSCBTag.usStatus);

    ulLegCBNo = pstHold->ulLegNo;

    switch (pstSCB->stHold.stSCBTag.usStatus)
    {
        case SC_HOLD_PROC:
            if (pstHold->bIsHold)
            {
                /* hold，需要找到另外一个leg，放播放拨号音  */
                pstSCB->stHold.stSCBTag.usStatus = SC_HOLD_ACTIVE;

                if (pstSCB->stCall.stSCBTag.bValid)
                {
                    if (ulLegCBNo == pstSCB->stCall.ulCallingLegNo)
                    {
                        ulOtherLegNo = pstSCB->stCall.ulCalleeLegNo;
                    }
                    else
                    {
                        ulOtherLegNo = pstSCB->stCall.ulCallingLegNo;
                    }
                }
                else if (pstSCB->stPreviewCall.stSCBTag.bValid)
                {
                    if (ulLegCBNo == pstSCB->stPreviewCall.ulCallingLegNo)
                    {
                        ulOtherLegNo = pstSCB->stPreviewCall.ulCalleeLegNo;
                    }
                }
                else if (pstSCB->stAutoCall.stSCBTag.bValid)
                {
                    if (ulLegCBNo == pstSCB->stAutoCall.ulCallingLegNo)
                    {
                        ulOtherLegNo = pstSCB->stAutoCall.ulCalleeLegNo;
                    }
                }
                else if (pstSCB->stTransfer.stSCBTag.bValid)
                {
                    if (ulLegCBNo == pstSCB->stTransfer.ulPublishLegNo)
                    {
                        ulOtherLegNo = pstSCB->stTransfer.ulSubLegNo;
                    }
                }

                /* TODO 放拨号音 */

            }
            break;
        case SC_HOLD_ACTIVE:
        case SC_HOLD_RELEASE:
            if (!pstHold->bIsHold)
            {
                /* hold */
                pstSCB->stHold.stSCBTag.bWaitingExit = DOS_TRUE;
            }
            break;
        default:
            break;
    }

    return ulRet;
}

U32 sc_hold_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HOLD_ST  *pstHold = NULL;
    U32                 ulRet    = DOS_SUCC;

    pstHold = (SC_MSG_EVT_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing hold release event. status : %u", pstSCB->stHold.stSCBTag.usStatus);

    switch (pstSCB->stHold.stSCBTag.usStatus)
    {
        case SC_HOLD_IDEL:
        case SC_HOLD_PROC:
        case SC_HOLD_ACTIVE:
        case SC_HOLD_RELEASE:
            /* hold */
            pstSCB->stHold.stSCBTag.bWaitingExit = DOS_TRUE;
            break;
        default:
            break;
    }

    return ulRet;
}

U32 sc_hold_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST  *pstErrReport = NULL;
    U32                 ulRet    = DOS_SUCC;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing hold error event. status : %u", pstSCB->stHold.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */

        return DOS_SUCC;
    }

    switch (pstSCB->stHold.stSCBTag.usStatus)
    {
        case SC_HOLD_IDEL:
            break;
        case SC_HOLD_PROC:
            /* 可能发送或者执行hold命令失败 */
            pstSCB->stHold.stSCBTag.bWaitingExit = DOS_TRUE;
            break;
        case SC_HOLD_ACTIVE:
            /* 可能发送或者执行unhold命令失败 */
            pstSCB->stHold.stSCBTag.bWaitingExit = DOS_TRUE;
            break;
        case SC_HOLD_RELEASE:
            /* hold */
            pstSCB->stHold.stSCBTag.bWaitingExit = DOS_TRUE;
            break;
        default:
            break;
    }

    return ulRet;
}


U32 sc_call_agent_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    U32                 ulRet           = DOS_FAIL;
    SC_LEG_CB           *pstLCB         = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing Call agent auth event.");

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;
    pstLCB = sc_lcb_get(pstSCB->stCallAgent.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_ERROR, SC_MOD_EVENT, SC_LOG_DISIST), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
         /* 注意通过偏移量，找到CC统一定义的错误码。
            需要判断坐席是否是长签，如果是长签的就不能挂断 */
        switch (pstSCB->stCallAgent.stSCBTag.usStatus)
        {
            case SC_CALL_AGENT_AUTH:
                /* 呼叫主叫坐席认证失败 */
                if (DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCalling)
                    && DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo))
                {
                    /* 没有找到坐席，挂断吧 */
                    pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo->ulLegNo = U32_BUTT;
                    sc_agent_serv_status_update(pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL_AGENT);
                }

                sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stCallAgent.ulCallingLegNo, CC_ERR_BS_HEAD + pstAuthRsp->stMsgTag.usInterErr);
                break;
            case SC_CALL_AGENT_AUTH2:
                /* 被叫坐席认证失败 TODO */

                break;
            default:
                break;
        }

        return DOS_SUCC;
    }

    switch (pstSCB->stCallAgent.stSCBTag.usStatus)
    {
        case SC_CALL_AGENT_AUTH:
            /* 呼叫主叫坐席 */
            switch (pstLCB->stCall.ucPeerType)
            {
                case SC_LEG_PEER_OUTBOUND:
                    ulRet = sc_make_call2pstn(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_TT:
                    ulRet = sc_make_call2eix(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_INTERNAL:
                    ulRet = sc_make_call2sip(pstSCB, pstLCB);
                    break;

                default:
                    sc_trace_scb(pstSCB, "Invalid perr type. %u", pstLCB->stCall.ucPeerType);
                    goto process_fail;
                    break;
            }

            pstSCB->stCallAgent.stSCBTag.usStatus = SC_CALL_AGENT_EXEC;
            break;

        case SC_CALL_AGENT_AUTH2:
            /* 呼叫被叫坐席 */
            pstSCB->stCallAgent.stSCBTag.usStatus = SC_CALL_AGENT_EXEC2;
            switch (pstLCB->stCall.ucPeerType)
            {
                case SC_LEG_PEER_OUTBOUND:
                    ulRet = sc_make_call2pstn(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_TT:
                    ulRet = sc_make_call2eix(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_INTERNAL:
                    ulRet = sc_make_call2sip(pstSCB, pstLCB);
                    break;

                default:
                    sc_trace_scb(pstSCB, "Invalid perr type. %u", pstLCB->stCall.ucPeerType);
                    goto process_fail;
                    break;
            }
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard auth event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed Preview auth event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

process_fail:
    if (DOS_ADDR_VALID(pstLCB))
    {
        sc_lcb_free(pstLCB);
        pstLCB = NULL;
    }

    if (DOS_ADDR_VALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    return DOS_FAIL;
}

U32 sc_call_agent_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstCallingCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing call agent setup event event. status : %u", pstSCB->stCallAgent.stSCBTag.usStatus);

    pstCallingCB = sc_lcb_get(pstSCB->stCallAgent.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto fail_proc;
    }

    switch (pstSCB->stCallAgent.stSCBTag.usStatus)
    {
        case SC_CALL_AGENT_IDEL:
        case SC_CALL_AGENT_AUTH:
            /* 未认证通过，放音挂断呼叫 */
            goto fail_proc;
            break;

        case SC_CALL_AGENT_EXEC:
        case SC_CALL_AGENT_PROC:
            /* 迁移状态到proc */
            pstSCB->stCallAgent.stSCBTag.usStatus = SC_CALL_AGENT_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_CALL_AGENT_AUTH2:
            goto fail_proc;
            break;

        case SC_CALL_AGENT_EXEC2:
        case SC_CALL_AGENT_PORC2:
            pstSCB->stCallAgent.stSCBTag.usStatus = SC_CALL_AGENT_PORC2;
            ulRet = DOS_SUCC;
            break;

        default:
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call agent setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

fail_proc:
    return DOS_FAIL;

}


U32 sc_call_agent_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                 ulRet           = DOS_FAIL;
    SC_LEG_CB           *pstCallingCB   = NULL;
    SC_LEG_CB           *pstCalleeCB    = NULL;
    SC_AGENT_NODE_ST    *pstAgentNode   = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing call agent setup event event. status : %u", pstSCB->stCallAgent.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing call agent setup event event. status : %u", pstSCB->stCallAgent.stSCBTag.usStatus);

    pstCallingCB = sc_lcb_get(pstSCB->stCallAgent.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto fail_proc;
    }

    switch (pstSCB->stCallAgent.stSCBTag.usStatus)
    {
        case SC_CALL_AGENT_IDEL:
        case SC_CALL_AGENT_AUTH:
            ulRet = DOS_FAIL;
            goto fail_proc;
            break;

        case SC_CALL_AGENT_EXEC:
        case SC_CALL_AGENT_PROC:
        case SC_CALL_AGENT_ALERTING:
            /* 坐席接通之后的处理 */
            if (pstSCB->stCallAgent.ulCalleeType == SC_SRV_CALL_TYEP_AGENT)
            {
                pstAgentNode = pstSCB->stCallAgent.pstAgentCallee;
                if (DOS_ADDR_INVALID(pstAgentNode)
                    || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
                {
                    /* TODO 异常处理 */
                    break;
                }

                pstCalleeCB = sc_lcb_get(pstAgentNode->pstAgentInfo->ulLegNo);
                /* 判断被叫坐席是不是长签 */
                if (DOS_ADDR_VALID(pstCalleeCB)
                    && pstCalleeCB->ulIndSCBNo != U32_BUTT)
                {
                    /* 修改坐席的状态，给被叫坐席放提示音 */
                    pstCalleeCB->ulSCBNo = pstSCB->ulSCBNo;
                    pstSCB->stCallAgent.ulCalleeLegNo = pstCalleeCB->ulCBNo;
                    sc_req_playback_stop(pstSCB->ulSCBNo, pstCalleeCB->ulCBNo);
                    sc_req_play_sound(pstSCB->ulSCBNo, pstCalleeCB->ulCBNo, SC_SND_INCOMING_CALL_TIP, 1, 0, 0);
                    sc_agent_serv_status_update(pstAgentNode->pstAgentInfo, SC_ACD_SERV_RINGING, SC_SRV_CALL_AGENT);
                    pstSCB->stCallAgent.stSCBTag.usStatus = SC_CALL_AGENT_TONE;

                    /* 给主叫坐席放回铃音 */
                    sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stCallAgent.ulCallingLegNo, DOS_TRUE, DOS_FALSE);
                    sc_agent_serv_status_update(pstAgentNode->pstAgentInfo, SC_ACD_SERV_RINGING, SC_SRV_CALL_AGENT);

                    break;
                }

                if (DOS_ADDR_VALID(pstCalleeCB))
                {
                    /* TODO 异常处理 */
                    break;
                }
            }

            /* 呼叫被叫坐席/SIP */
            pstCalleeCB = sc_lcb_alloc();
            if (DOS_ADDR_INVALID(pstCalleeCB))
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc lcb fail");
                goto fail_proc;
            }

            pstCalleeCB->stCall.bValid = DOS_TRUE;
            pstCalleeCB->stCall.ucStatus = SC_LEG_INIT;
            pstCalleeCB->ulSCBNo = pstSCB->ulSCBNo;
            pstSCB->stCallAgent.ulCalleeLegNo = pstCalleeCB->ulCBNo;

            if (pstSCB->stCallAgent.ulCalleeType == SC_SRV_CALL_TYEP_SIP)
            {
                dos_snprintf(pstCalleeCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeCB->stCall.stNumInfo.szOriginalCallee), pstSCB->stCallAgent.szCalleeNum);
                pstCalleeCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
            }
            else
            {
                switch (pstAgentNode->pstAgentInfo->ucBindType)
                {
                    case AGENT_BIND_SIP:
                        dos_snprintf(pstCalleeCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szUserID);
                        pstCalleeCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
                        break;

                    case AGENT_BIND_TELE:
                        dos_snprintf(pstCalleeCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szTelePhone);
                        pstCalleeCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
                        if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
                        {
                            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");

                            goto fail_proc;
                        }
                        break;

                    case AGENT_BIND_MOBILE:
                        dos_snprintf(pstCalleeCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szMobile);
                        pstCalleeCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
                        if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
                        {
                            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");

                            goto fail_proc;
                        }
                        break;

                    case AGENT_BIND_TT_NUMBER:
                        dos_snprintf(pstCalleeCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szTTNumber);
                        pstCalleeCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
                        break;

                    default:
                        break;
                }
            }

            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeCB->stCall.stNumInfo.szOriginalCalling), pstCallingCB->stCall.stNumInfo.szOriginalCalling);

            /* 新LEG处理一下号码 */
            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeCB->stCall.stNumInfo.szRealCallee), pstCalleeCB->stCall.stNumInfo.szOriginalCallee);
            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeCB->stCall.stNumInfo.szRealCalling), pstCalleeCB->stCall.stNumInfo.szOriginalCalling);

            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szCallee, sizeof(pstCalleeCB->stCall.stNumInfo.szCallee), pstCalleeCB->stCall.stNumInfo.szRealCallee);
            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szCalling, sizeof(pstCalleeCB->stCall.stNumInfo.szCalling), pstCalleeCB->stCall.stNumInfo.szRealCalling);

            if (pstCalleeCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
            {
                if (sc_send_usr_auth2bs(pstSCB, pstCalleeCB) != DOS_SUCC)
                {
                    sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth fail.");

                    goto fail_proc;
                }

                if (DOS_ADDR_VALID(pstAgentNode)
                    && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstAgentNode->pstAgentInfo, SC_ACD_SERV_RINGBACK, SC_SRV_CALL_AGENT);
                    pstAgentNode->pstAgentInfo->ulLegNo = pstCalleeCB->ulCBNo;
                }
                pstSCB->stCallAgent.stSCBTag.usStatus = SC_CALL_AGENT_AUTH2;
            }
            else
            {
                if (pstCalleeCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND_TT)
                {
                    ulRet = sc_make_call2eix(pstSCB, pstCalleeCB);
                }
                else
                {
                    ulRet = sc_make_call2sip(pstSCB, pstCalleeCB);
                }

                if (DOS_ADDR_VALID(pstAgentNode)
                     && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
                {
                    pstAgentNode->pstAgentInfo->ulLegNo = pstCalleeCB->ulCBNo;
                    sc_agent_serv_status_update(pstAgentNode->pstAgentInfo, SC_ACD_SERV_RINGING, SC_SRV_CALL_AGENT);
                }
                pstSCB->stCallAgent.stSCBTag.usStatus = SC_CALL_AGENT_EXEC2;

            }

            /* 给坐席放回铃音 */
            sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stCallAgent.ulCallingLegNo, DOS_TRUE, DOS_FALSE);
            break;

        case SC_CALL_AGENT_PORC2:
        case SC_CALL_AGENT_ALERTING2:
            pstCalleeCB = sc_lcb_get(pstSCB->stCallAgent.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCalleeCB))
            {
                sc_trace_scb(pstSCB, "There is no calling leg.");

                goto fail_proc;
            }
            if (pstSCB->stCallAgent.ulCalleeType == SC_SRV_CALL_TYEP_AGENT)
            {
                pstAgentNode = pstSCB->stCallAgent.pstAgentCallee;
                if (DOS_ADDR_INVALID(pstAgentNode)
                    || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
                {
                    /* TODO 坐席没找到，错误 */
                    DOS_ASSERT(0);
                    break;
                }
                sc_agent_serv_status_update(pstAgentNode->pstAgentInfo, SC_ACD_SERV_CALL_IN, SC_SRV_CALL_AGENT);
            }

            /* 修改坐席的业务状态 */
            sc_agent_serv_status_update(pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo, SC_ACD_SERV_CALL_OUT, SC_SRV_CALL_AGENT);

            if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCallAgent.ulCalleeLegNo, pstSCB->stCallAgent.ulCallingLegNo) != DOS_SUCC)
            {
                sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                goto fail_proc;
            }

            pstSCB->stCallAgent.stSCBTag.usStatus = SC_CALL_AGENT_CONNECTED;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call agent answer event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
fail_proc:
    /* TODO 错误处理 */
    return DOS_FAIL;
}

U32 sc_call_agent_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_MSG_EVT_RINGING_ST   *pstRinging;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstRinging = (SC_MSG_EVT_RINGING_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Proccessing call agent setup event event. status : %u. with media: %u", pstSCB->stCallAgent.stSCBTag.usStatus, pstRinging->ulWithMedia);

    switch (pstSCB->stCallAgent.stSCBTag.usStatus)
    {
        case SC_CALL_AGENT_EXEC:
        case SC_CALL_AGENT_PROC:
            pstSCB->stCallAgent.stSCBTag.usStatus = SC_CALL_AGENT_ALERTING;

            ulRet = DOS_SUCC;
            break;

        case SC_CALL_AGENT_PORC2:
        case SC_CALL_AGENT_ALERTING2:
            /* 迁移到alerting状态 */
            /* 如果有媒体需要bridge呼叫，否则给主动放回铃音 */
            sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stCallAgent.ulCallingLegNo, DOS_TRUE, pstRinging->ulWithMedia);

            if (pstRinging->ulWithMedia)
            {
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCallAgent.ulCalleeLegNo, pstSCB->stCallAgent.ulCallingLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    goto fail_proc;
                }
            }

            pstSCB->stCallAgent.stSCBTag.usStatus = SC_CALL_AGENT_ALERTING2;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call agent ringing event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

fail_proc:
    return DOS_FAIL;

}

U32 sc_call_agent_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                     ulRet           = DOS_SUCC;
    SC_LEG_CB               *pstCallingCB   = NULL;
    SC_LEG_CB               *pstCalleeCB    = NULL;
    SC_LEG_CB               *pstHungupLeg   = NULL;
    SC_LEG_CB               *pstOtherLeg    = NULL;
    SC_MSG_EVT_HUNGUP_ST    *pstHungup      = NULL;
    SC_AGENT_NODE_ST        *pstOtherAgent  = NULL;
    SC_AGENT_NODE_ST        *pstHungupAgent = NULL;
    U32                     ulReleasePart;
    U32                     ulErrCode;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;

    ulErrCode = pstHungup->ulErrCode;

    sc_trace_scb(pstSCB, "Proccessing call agent hungup event. status : %u", pstSCB->stCallAgent.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing call agent hungup event. status : %u", pstSCB->stCallAgent.stSCBTag.usStatus);

    if (pstHungup->ulLegNo == pstSCB->stCallAgent.ulCallingLegNo)
    {
        ulReleasePart = SC_CALLING;
    }
    else
    {
        ulReleasePart = SC_CALLEE;
    }

    switch (pstSCB->stCallAgent.stSCBTag.usStatus)
    {
        case SC_CALL_AGENT_IDEL:
        case SC_CALL_AGENT_AUTH:
        case SC_CALL_AGENT_EXEC:
        case SC_CALL_AGENT_PROC:
        case SC_CALL_AGENT_ALERTING:
        case SC_CALL_AGENT_ACTIVE:
        case SC_CALL_AGENT_AUTH2:
        case SC_CALL_AGENT_EXEC2:
            /* 这个时候挂断只会是坐席的LEG清理资源即可 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent not connected.");
            pstCallingCB = sc_lcb_get(pstSCB->stCallAgent.ulCallingLegNo);
            if (pstCallingCB)
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            /* 如果存在坐席，则要修改坐席的状态 */
            if (DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCalling)
                && DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo))
            {
                pstSCB->stCallAgent.pstAgentCallee->pstAgentInfo->ulLegNo = U32_BUTT;
                sc_agent_serv_status_update(pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL_AGENT);
            }

            if (DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCallee)
                && DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCallee->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstSCB->stCallAgent.pstAgentCallee->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL_AGENT);
            }

            sc_scb_free(pstSCB);
            break;

        case SC_CALL_AGENT_PORC2:
        case SC_CALL_AGENT_ALERTING2:
        case SC_CALL_AGENT_TONE:
        case SC_CALL_AGENT_CONNECTED:
            /* 这个时候挂断，可能是坐席也可能客户，如果是客户需要注意LEG的状态 */
            pstCallingCB = sc_lcb_get(pstSCB->stCallAgent.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stCallAgent.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCallingCB) || DOS_ADDR_INVALID(pstCalleeCB))
            {
                /* 异常 */
                DOS_ASSERT(0);
                if (DOS_ADDR_VALID(pstCallingCB))
                {
                    sc_lcb_free(pstCallingCB);
                }
                if (DOS_ADDR_VALID(pstCalleeCB))
                {
                    sc_lcb_free(pstCalleeCB);
                }

                sc_scb_free(pstSCB);
                break;
            }

            if (pstCallingCB->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCallingCB->ulOtherSCBNo);
                pstCallingCB->ulOtherSCBNo = U32_BUTT;
            }

            if (pstCalleeCB->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCalleeCB->ulOtherSCBNo);
                pstCalleeCB->ulOtherSCBNo = U32_BUTT;
            }

            if (pstSCB->stCallAgent.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCalleeCB;
                pstOtherLeg  = pstCallingCB;
                pstHungupAgent = pstSCB->stCallAgent.pstAgentCallee;
                pstOtherAgent = pstSCB->stCallAgent.pstAgentCalling;
            }
            else
            {
                pstHungupLeg = pstCallingCB;
                pstOtherLeg  = pstCalleeCB;
                pstHungupAgent = pstSCB->stCallAgent.pstAgentCalling;
                pstOtherAgent = pstSCB->stCallAgent.pstAgentCallee;
            }

            /* 生成话单 */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            if (!sc_scb_is_exit_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
            }

            pstHungupLeg->stCall.ulCause = ulErrCode;
            sc_send_billing_stop2bs(pstSCB, pstHungupLeg, NULL, ulReleasePart);

            /* 判断挂断的坐席是不是长签 */
            if (DOS_ADDR_VALID(pstHungupAgent)
                && DOS_ADDR_VALID(pstHungupAgent->pstAgentInfo)
                && pstHungupAgent->pstAgentInfo->bNeedConnected)
            {
                pstHungupLeg->ulSCBNo = U32_BUTT;
            }

            /* 判断另外一个坐席是不是长签 */
            if (pstOtherLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签 */
                pstOtherLeg->ulSCBNo = U32_BUTT;
                sc_req_playback_stop(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo);
                sc_req_play_sound(pstOtherLeg->ulIndSCBNo, pstOtherLeg->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);

            }
            else
            {
                /* 挂断另一条leg */
                if (DOS_ADDR_VALID(pstOtherAgent))
                {
                    pstOtherAgent->pstAgentInfo->ulLegNo = U32_BUTT;
                }

                pstSCB->stCallAgent.ulCalleeLegNo = U32_BUTT;
                pstSCB->stCallAgent.stSCBTag.usStatus = SC_CALL_AGENT_RELEASE;
                /* 可能在放回铃音，需要手动停止 */
                sc_req_playback_stop(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo);
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
            }

            if (DOS_ADDR_VALID(pstHungupAgent))
            {
                sc_agent_serv_status_update(pstHungupAgent->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL_AGENT);
                pstHungupAgent->pstAgentInfo->ulLegNo = U32_BUTT;
            }

            if (DOS_ADDR_VALID(pstOtherAgent))
            {
                sc_agent_serv_status_update(pstOtherAgent->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL_AGENT);
            }

            if (pstHungupLeg->ulIndSCBNo == U32_BUTT)
            {
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
            }

            break;

        case SC_CALL_AGENT_RELEASE:
            pstCalleeCB = sc_lcb_get(pstSCB->stCallAgent.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stCallAgent.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;
        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call hungup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call agent setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

}

U32 sc_call_agent_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstLCB       =  NULL;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing preview call dtmf event. status : %u", pstSCB->stCallAgent.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 开启接入码业务 */
    if (pstDTMF->cDTMFVal != '*'
        && pstDTMF->cDTMFVal != '#' )
    {
        /* 第一个字符不是 '*' 或者 '#' 不保存  */
        return DOS_SUCC;
    }

    pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stAccessCode.szDialCache[0] = '\0';
    pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
    if (pstDTMF->ulLegNo == pstSCB->stCallAgent.ulCallingLegNo)
    {
        if (DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCalling)
            && DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo))
        {
            pstSCB->stAccessCode.ulAgentID = pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo->ulAgentID;
        }
    }
    else
    {
        if (DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCallee)
            && DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCallee->pstAgentInfo))
        {
            pstSCB->stAccessCode.ulAgentID = pstSCB->stCallAgent.pstAgentCallee->pstAgentInfo->ulAgentID;
        }
    }

    pstSCB->ulCurrentSrv++;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;

    return DOS_SUCC;
}

U32 sc_call_agent_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HOLD_ST   *pstHold        = NULL;
    SC_LEG_CB            *pstLeg         = NULL;

    pstHold = (SC_MSG_EVT_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstHold->bIsHold)
    {
        /* 如果是被HOLD的，需要激活HOLD业务哦 */
        pstSCB->stHold.stSCBTag.bValid = DOS_TRUE;
        pstSCB->stHold.stSCBTag.bWaitingExit = DOS_FALSE;
        pstSCB->stHold.stSCBTag.usStatus = SC_HOLD_ACTIVE;
        pstSCB->stHold.ulCallLegNo = pstHold->ulLegNo;
        pstSCB->stHold.ulHoldCount++;

        pstSCB->ulCurrentSrv++;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stHold.stSCBTag;

        /* 给HOLD方 播放拨号音 */
        /* 给HOLD对方 播放呼叫保持音 */
    }
    else
    {
        /* 如果是被UNHOLD的，已经没有HOLD业务了，单纯处理呼叫就好 */
        pstLeg = sc_lcb_get(pstHold->ulLegNo);
        if (DOS_ADDR_INVALID(pstLeg))
        {
            return DOS_FAIL;
        }

        if (pstLeg->stHold.ulHoldTime != 0
            && pstLeg->stHold.ulUnHoldTime > pstLeg->stHold.ulHoldTime)
        {
            pstSCB->stHold.ulHoldTotalTime += (pstLeg->stHold.ulUnHoldTime - pstLeg->stHold.ulHoldTime);
        }

        pstLeg->stHold.ulUnHoldTime = 0;
        pstLeg->stHold.ulHoldTime = 0;
    }

    return DOS_SUCC;
}

U32 sc_call_agent_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstCallingLegCB = NULL;
    SC_LEG_CB              *pstCalleeLegCB  = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstCallSetup    = NULL;
    U32                    ulRet            = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallingLegCB = sc_lcb_get(pstSCB->stCallAgent.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallSetup = (SC_MSG_EVT_PLAYBACK_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Processing the call playback stop msg. status: %u", pstSCB->stCallAgent.stSCBTag.usStatus);

    switch (pstSCB->stCallAgent.stSCBTag.usStatus)
    {
        case SC_CALL_AGENT_TONE:
            /* 坐席长签时，给坐席放提示音结束，需要录音 */
            pstCalleeLegCB = sc_lcb_get(pstSCB->stCallAgent.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCallee)
                && DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCallee->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstSCB->stCallAgent.pstAgentCallee->pstAgentInfo, SC_ACD_SERV_CALL_IN, SC_SRV_CALL_AGENT);
            }

            if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCallAgent.ulCalleeLegNo, pstSCB->stCallAgent.ulCallingLegNo) != DOS_SUCC)
            {
                sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                return DOS_FAIL;
            }
            pstSCB->stCallAgent.stSCBTag.usStatus = SC_CALL_AGENT_CONNECTED;
            break;

        default:
            ulRet = DOS_FAIL;
            sc_trace_scb(pstSCB, "Invalid status.%u", pstSCB->stCallAgent.stSCBTag.usStatus);
            break;
    }

    return DOS_SUCC;
}

U32 sc_call_agent_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;
    SC_LEG_CB                   *pstCallingCB       = NULL;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing call agent error event. status : %u", pstSCB->stCallAgent.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing call agent error event. status : %u", pstSCB->stCallAgent.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stCallAgent.stSCBTag.usStatus)
    {
        case SC_CALL_AGENT_IDEL:
        case SC_CALL_AGENT_AUTH:
        case SC_CALL_AGENT_EXEC:
            /* 发起呼叫失败，直接释放资源 */
            if (DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCalling)
                && DOS_ADDR_VALID(pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_CALL_AGENT);
                pstSCB->stCallAgent.pstAgentCalling->pstAgentInfo->ulLegNo = U32_BUTT;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stCallAgent.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
            }
            sc_scb_free(pstSCB);
            break;
        case SC_CALL_AGENT_PROC:
        case SC_CALL_AGENT_ALERTING:
            ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCallAgent.ulCalleeLegNo, ulErrCode);
            break;
        case SC_CALL_AGENT_ACTIVE:
        case SC_CALL_AGENT_AUTH2:
        case SC_CALL_AGENT_EXEC2:
        case SC_CALL_AGENT_PORC2:
        case SC_CALL_AGENT_ALERTING2:
        case SC_CALL_AGENT_TONE:
            ulRet = sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stCallAgent.ulCallingLegNo, ulErrCode);
            break;
        case SC_CALL_AGENT_CONNECTED:
        case SC_CALL_AGENT_RELEASE:
            if (pstSCB->stCallAgent.ulCalleeLegNo != U32_BUTT)
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCallAgent.ulCalleeLegNo, ulErrCode);
            }
            else
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCallAgent.ulCallingLegNo, ulErrCode);
            }
            break;
        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

U32 sc_auto_preview_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB        = NULL;
    SC_LEG_CB                  *pstCallingLegCB = NULL;
    U32                         ulRet           = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto preview auth event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Processing auto preview auth event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    pstLegCB = sc_lcb_get(pstSCB->stAutoPreview.ulCalleeLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        sc_scb_free(pstSCB);

        return DOS_FAIL;
    }

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* 注意通过偏移量，找到CC统一定义的错误码 */

        /* 分析呼叫结果 */
        sc_preview_task_call_result(pstSCB, pstSCB->stAutoPreview.ulCalleeLegNo, pstAuthRsp->stMsgTag.usInterErr + CC_ERR_BS_HEAD);

        sc_lcb_free(pstLegCB);
        sc_scb_free(pstSCB);

        return DOS_SUCC;
    }

    switch (pstSCB->stAutoPreview.stSCBTag.usStatus)
    {
        case SC_AUTO_PREVIEW_AUTH:
            /* 认证通过，加入呼叫队列 */
            pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_QUEUE;
            pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
            pstSCB->ulCurrentSrv++;
            pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
            pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
            pstSCB->stIncomingQueue.ulLegNo = pstSCB->stAutoPreview.ulCalleeLegNo;
            pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
            pstSCB->stIncomingQueue.ulQueueType = SC_SW_FORWARD_AGENT_GROUP;
            if (sc_cwq_add_call(pstSCB, sc_task_get_agent_queue(pstSCB->stAutoPreview.ulTcbID), pstLegCB->stCall.stNumInfo.szCallee, pstSCB->stIncomingQueue.ulQueueType, DOS_FALSE) != DOS_SUCC)
            {
                /* 加入队列失败 */
                DOS_ASSERT(0);
                sc_preview_task_call_result(pstSCB, pstSCB->stAutoPreview.ulCalleeLegNo, CC_ERR_SC_RESOURCE_EXCEED);
                sc_lcb_free(pstLegCB);
                sc_scb_free(pstSCB);
            }
            else
            {
                pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CALL_QUEUE_WAIT, 1, 0, 0);
            }
            break;

        case SC_AUTO_PREVIEW_AUTH2:
            /* 呼叫坐席 */
            pstCallingLegCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstCallingLegCB))
            {
                /* TODO */
                return DOS_FAIL;
            }
            pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_EXEC;
            ulRet = sc_make_call2pstn(pstSCB, pstCallingLegCB);
            if (ulRet != DOS_SUCC)
            {
                /* 挂断 客户 */
                sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoPreview.ulCallingLegNo, pstCallingLegCB->stCall.ulCause);
            }
            break;

        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_auto_preview_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvtCall = NULL;
    U32                   ulRet         = DOS_FAIL;

    pstEvtCall = (SC_MSG_EVT_LEAVE_CALLQUE_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto preview queue event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);

    switch (pstSCB->stAutoPreview.stSCBTag.usStatus)
    {
        case SC_AUTO_PREVIEW_QUEUE:
            if (SC_LEAVE_CALL_QUE_TIMEOUT == pstMsg->usInterErr)
            {
                /* 加入队列超时 */
            }
            else if (SC_LEAVE_CALL_QUE_SUCC == pstMsg->usInterErr)
            {
                if (DOS_ADDR_INVALID(pstEvtCall->pstAgentNode))
                {
                    /* 错误 */
                }
                else
                {
                    /* 呼叫坐席 */
                    ulRet = sc_agent_auto_preview_callback(pstSCB, pstEvtCall->pstAgentNode);
                }
            }
        default:
            break;

     }

    sc_trace_scb(pstSCB, "Proccessed auto preview answer event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        /* TODO 失败的处理 */
        if (DOS_ADDR_VALID(pstEvtCall->pstAgentNode)
            && DOS_ADDR_VALID(pstEvtCall->pstAgentNode->pstAgentInfo))
        {
            pstEvtCall->pstAgentNode->pstAgentInfo->bSelected = DOS_FALSE;
        }
    }

    return ulRet;
}

U32 sc_auto_preview_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto preview stup event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stAutoPreview.stSCBTag.usStatus)
    {
        case SC_AUTO_PREVIEW_AUTH2:
        case SC_AUTO_PREVIEW_EXEC:
            /* 迁移状态到proc */
            pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_AUTO_PREVIEW_ACTIVE:
            pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_CONNECTING;
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto preview setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_auto_preview_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_MSG_EVT_RINGING_ST   *pstRinging;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstRinging = (SC_MSG_EVT_RINGING_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Proccessing auto preview setup event event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing auto preview setup event event. status : %u. with media:%u", pstSCB->stAutoPreview.stSCBTag.usStatus, pstRinging->ulWithMedia);

    switch (pstSCB->stAutoPreview.stSCBTag.usStatus)
    {
        case SC_AUTO_PREVIEW_IDEL:
        case SC_AUTO_PREVIEW_AUTH:
        case SC_AUTO_PREVIEW_QUEUE:
        case SC_AUTO_PREVIEW_AUTH2:
            /* 未认证通过，放音挂断呼叫 */
            ulRet = DOS_FAIL;
            goto fail_proc;
            break;

        case SC_AUTO_PREVIEW_EXEC:
        case SC_AUTO_PREVIEW_PROC:
            pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_ALERTING;

            ulRet = DOS_SUCC;
            break;

        case SC_AUTO_PREVIEW_CONNECTING:
        case SC_AUTO_PREVIEW_ALERTING2:
            /* 迁移到alerting状态 */
            /* 如果有媒体需要bridge呼叫，否则给主动放回铃音 */
            sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stAutoPreview.ulCallingLegNo, DOS_TRUE, pstRinging->ulWithMedia);
            if (pstRinging->ulWithMedia)
            {
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stAutoPreview.ulCalleeLegNo, pstSCB->stAutoPreview.ulCallingLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    goto fail_proc;
                }
            }

            pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_ALERTING2;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed auto preview ringing event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

fail_proc:
    return DOS_FAIL;

}

U32 sc_auto_preview_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                 ulRet           = DOS_FAIL;
    SC_LEG_CB           *pstCallingCB   = NULL;
    SC_LEG_CB           *pstCalleeCB    = NULL;
    SC_AGENT_NODE_ST    *pstAgentNode   = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing auto preview setup event event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing auto preview setup event event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);

    pstCallingCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto fail_proc;
    }

    switch (pstSCB->stAutoPreview.stSCBTag.usStatus)
    {
        case SC_AUTO_PREVIEW_IDEL:
        case SC_AUTO_PREVIEW_AUTH:
        case SC_AUTO_PREVIEW_QUEUE:
        case SC_AUTO_PREVIEW_AUTH2:
            ulRet = DOS_FAIL;
            goto fail_proc;
            break;

        case SC_AUTO_PREVIEW_EXEC:
        case SC_AUTO_PREVIEW_PROC:
        case SC_AUTO_PREVIEW_ALERTING:
            /* 坐席接通之后的处理 */
            /* 1. 发起PSTN的呼叫 */
            /* 2. 迁移状态到CONNTECTING */
            pstCalleeCB = sc_lcb_get(pstSCB->stAutoPreview.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCalleeCB))
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc lcb fail");
                goto fail_proc;
            }

            pstCalleeCB->ulSCBNo = pstSCB->ulSCBNo;

            pstAgentNode = sc_agent_get_by_id(pstSCB->stAutoPreview.ulAgentID);
            if (DOS_ADDR_INVALID(pstAgentNode)
                || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
            {
                /* TODO 坐席没找到，错误 */
                DOS_ASSERT(0);
                break;
            }

            if (sc_make_call2pstn(pstSCB, pstCalleeCB) != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Make call to pstn fail.");
                goto fail_proc;
            }

            sc_agent_serv_status_update(pstAgentNode->pstAgentInfo, SC_ACD_SERV_RINGBACK, SC_SRV_AUTO_PREVIEW);
            pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_ACTIVE;
            break;

        case SC_AUTO_PREVIEW_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_AUTO_PREVIEW_CONNECTING:
        case SC_AUTO_PREVIEW_ALERTING2:
            pstCalleeCB = sc_lcb_get(pstSCB->stAutoPreview.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCalleeCB))
            {
                sc_trace_scb(pstSCB, "There is no calling leg.");

                goto fail_proc;
            }

            pstAgentNode = sc_agent_get_by_id(pstSCB->stAutoPreview.ulAgentID);
            if (DOS_ADDR_INVALID(pstAgentNode)
                || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
            {
                /* TODO 坐席没找到，错误 */
                DOS_ASSERT(0);
                break;
            }

            /* 修改坐席的业务状态 */
            sc_agent_serv_status_update(pstAgentNode->pstAgentInfo, SC_ACD_SERV_CALL_OUT, SC_SRV_AUTO_PREVIEW);

            /* 没有早期媒体，需要桥接呼叫 */
            if (DOS_ADDR_VALID(pstCalleeCB)
                && !pstCalleeCB->stCall.bEarlyMedia)
            {
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stAutoPreview.ulCalleeLegNo, pstSCB->stAutoPreview.ulCallingLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    goto fail_proc;
                }
            }

            pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_CONNECTED;
            break;

        case SC_AUTO_PREVIEW_CONNECTED:
            ulRet = DOS_SUCC;
            break;

        case SC_AUTO_PREVIEW_PROCESS:
            /* 处理长签之内的一个事情 */
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed auto preview answer event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

fail_proc:
    /* TODO 错误处理 */
    return DOS_FAIL;
}

U32 sc_auto_preview_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* 处理录音结束 */
    SC_MSG_CMD_RECORD_ST *pstRecord = NULL;
    SC_LEG_CB            *pstLCB    = NULL;

    pstRecord = (SC_MSG_CMD_RECORD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstRecord) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing auto preview record stop event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstRecord->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        return DOS_FAIL;
    }

    /* 生成录音话单 */
    sc_send_special_billing_stop2bs(pstSCB, pstLCB, BS_SERV_RECORDING);
    sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);

    return DOS_SUCC;
}

U32 sc_auto_preview_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB                   *pstCallingCB       = NULL;
    SC_AGENT_NODE_ST            *pstAgentCall       = NULL;

    /* 处理放音结束 */
    sc_trace_scb(pstSCB, "Proccessing auto preview playback stop event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);

    switch (pstSCB->stAutoPreview.stSCBTag.usStatus)
    {
        case SC_AUTO_PREVIEW_RELEASE:
            pstCallingCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB)
                && pstCallingCB->ulIndSCBNo != U32_BUTT)
            {
                pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoPreview.ulAgentID);
                if (DOS_ADDR_VALID(pstAgentCall)
                    && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
                {
                    if (pstCallingCB->ulIndSCBNo != U32_BUTT)
                    {
                        /* 坐席长签的时候，呼叫对端失败，方提示音后到这里，修改坐席的状态，放提示音 */
                        pstCallingCB->ulSCBNo = U32_BUTT;
                        sc_scb_free(pstSCB);
                        pstSCB = NULL;
                        sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_PREVIEW);
                        sc_req_play_sound(pstCallingCB->ulIndSCBNo, pstCallingCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                    }
                    else
                    {
                        sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_PREVIEW);
                        sc_lcb_free(pstCallingCB);
                        pstCallingCB = NULL;
                        sc_scb_free(pstSCB);
                        pstSCB = NULL;
                    }
                }
            }
            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_auto_preview_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstLCB       =  NULL;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing auto preview dtmf event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 开启接入码业务 */
    if (pstDTMF->cDTMFVal != '*'
        && pstDTMF->cDTMFVal != '#' )
    {
        /* 第一个字符不是 '*' 或者 '#' 不保存  */
        return DOS_SUCC;
    }

    /* 只有坐席对应的leg执行接入码业务 */
    if (pstDTMF->ulLegNo != pstSCB->stAutoPreview.ulCallingLegNo)
    {
        return DOS_SUCC;
    }

    pstSCB->stAccessCode.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stAccessCode.szDialCache[0] = '\0';
    pstSCB->stAccessCode.stSCBTag.usStatus = SC_ACCESS_CODE_OVERLAP;
    pstSCB->stAccessCode.ulAgentID = pstSCB->stAutoPreview.ulAgentID;
    pstSCB->ulCurrentSrv++;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAccessCode.stSCBTag;

    return DOS_SUCC;
}

U32 sc_auto_preview_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HOLD_ST   *pstHold        = NULL;
    SC_LEG_CB            *pstLeg         = NULL;

    pstHold = (SC_MSG_EVT_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstHold->bIsHold)
    {
        /* 如果是被HOLD的，需要激活HOLD业务哦 */
        pstSCB->stHold.stSCBTag.bValid = DOS_TRUE;
        pstSCB->stHold.stSCBTag.bWaitingExit = DOS_FALSE;
        pstSCB->stHold.stSCBTag.usStatus = SC_HOLD_ACTIVE;
        pstSCB->stHold.ulCallLegNo = pstHold->ulLegNo;
        pstSCB->stHold.ulHoldCount++;

        pstSCB->ulCurrentSrv++;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stHold.stSCBTag;

        /* 给HOLD方 播放拨号音 */
        /* 给HOLD对方 播放呼叫保持音 */
    }
    else
    {
        /* 如果是被UNHOLD的，已经没有HOLD业务了，单纯处理呼叫就好 */
        pstLeg = sc_lcb_get(pstHold->ulLegNo);
        if (DOS_ADDR_INVALID(pstLeg))
        {
            return DOS_FAIL;
        }

        if (pstLeg->stHold.ulHoldTime != 0
            && pstLeg->stHold.ulUnHoldTime > pstLeg->stHold.ulHoldTime)
        {
            pstSCB->stHold.ulHoldTotalTime += (pstLeg->stHold.ulUnHoldTime - pstLeg->stHold.ulHoldTime);
        }

        pstLeg->stHold.ulUnHoldTime = 0;
        pstLeg->stHold.ulHoldTime = 0;
    }

    return DOS_SUCC;
}

U32 sc_auto_preview_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32                     ulRet           = DOS_SUCC;
    SC_LEG_CB               *pstCallingCB   = NULL;
    SC_LEG_CB               *pstCalleeCB    = NULL;
    SC_LEG_CB               *pstHungupLeg   = NULL;
    SC_LEG_CB               *pstOtherLeg    = NULL;
    SC_MSG_EVT_HUNGUP_ST    *pstHungup      = NULL;
    SC_AGENT_NODE_ST        *pstAgentCall   = NULL;
    S32                     i               = 0;
    S32                     lRes            = DOS_FAIL;
    U32                     ulReleasePart;
    U32                     ulErrCode;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;

    ulErrCode = pstHungup->ulErrCode;

    sc_trace_scb(pstSCB, "Proccessing auto preview hungup event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing auto preview hungup event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);

    if (pstHungup->ulLegNo == pstSCB->stAutoPreview.ulCallingLegNo)
    {
        ulReleasePart = SC_CALLING;
    }
    else
    {
        ulReleasePart = SC_CALLEE;
    }

    /* 呼叫结果 */
    if (SC_AUTO_PREVIEW_PROCESS != pstSCB->stAutoPreview.stSCBTag.usStatus
        && SC_AUTO_PREVIEW_RELEASE != pstSCB->stAutoPreview.stSCBTag.usStatus)
    {
        pstHungupLeg = sc_lcb_get(pstHungup->ulLegNo);
        if (DOS_ADDR_VALID(pstHungupLeg))
        {
            sc_preview_task_call_result(pstSCB, pstHungupLeg->ulCBNo, pstHungup->ulErrCode);
        }
    }

    switch (pstSCB->stAutoPreview.stSCBTag.usStatus)
    {
        case SC_AUTO_PREVIEW_IDEL:
        case SC_AUTO_PREVIEW_AUTH:
        case SC_AUTO_PREVIEW_QUEUE:
        case SC_AUTO_PREVIEW_AUTH2:
        case SC_AUTO_PREVIEW_EXEC:
            /* 基本不可能到这里 */
            DOS_ASSERT(0);
            break;

        case SC_AUTO_PREVIEW_PROC:
        case SC_AUTO_PREVIEW_ALERTING:
        case SC_AUTO_PREVIEW_ACTIVE:
            /* 这个时候挂断只会是坐席的LEG，修改坐席的状态 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent not connected.");
            pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoPreview.ulAgentID);
            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_PREVIEW);
                pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stAutoPreview.ulCalleeLegNo);
            if (pstCallingCB)
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            if (pstCalleeCB)
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            sc_scb_free(pstSCB);
            break;

        case SC_AUTO_PREVIEW_CONNECTING:
        case SC_AUTO_PREVIEW_ALERTING2:
            /* 这个时候挂断，可能是坐席也可能客户，如果是客户需要注意LEG的状态 */
            pstCallingCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stAutoPreview.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCallingCB) || DOS_ADDR_INVALID(pstCalleeCB))
            {
                /* 异常 */
                DOS_ASSERT(0);
                if (DOS_ADDR_VALID(pstCallingCB))
                {
                    sc_lcb_free(pstCallingCB);
                }
                if (DOS_ADDR_VALID(pstCalleeCB))
                {
                    sc_lcb_free(pstCalleeCB);
                }

                sc_scb_free(pstSCB);
                break;
            }

            if (pstSCB->stAutoPreview.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCalleeCB;
                pstOtherLeg  = pstCallingCB;
                pstCallingCB->stCall.stTimeInfo.ulByeTime = pstCalleeCB->stCall.stTimeInfo.ulByeTime;
            }
            else
            {
                pstHungupLeg = pstCallingCB;
                pstOtherLeg  = pstCalleeCB;
                pstCalleeCB->stCall.stTimeInfo.ulByeTime = pstCallingCB->stCall.stTimeInfo.ulByeTime;
            }

            /* 生成话单 */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            if (sc_scb_is_exit_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                /* 如果有出局呼叫，应该先将出局呼叫删除 */
                sc_scb_remove_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCalleeCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalleeCB, NULL, ulReleasePart);
                /* 出局呼叫的话单应该用坐席那条leg产生 */
                sc_scb_remove_service(pstSCB, BS_SERV_PREVIEW_DIALING);
                sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);
            }
            else
            {
                pstCalleeCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalleeCB, NULL, ulReleasePart);
            }

            pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoPreview.ulAgentID);
            if (pstSCB->stAutoPreview.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                /* 客户挂断的，判断坐席是否长签，修改坐席的状态，挂断电话等 */
                if (DOS_ADDR_INVALID(pstAgentCall) || DOS_ADDR_INVALID(pstAgentCall->pstAgentInfo))
                {
                    sc_req_playback_stop(pstSCB->ulSCBNo, pstCallingCB->ulCBNo);
                    sc_req_hungup(pstSCB->ulSCBNo, pstCallingCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                    pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_RELEASE;
                }

                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_PREVIEW);

                if (pstCallingCB->ulIndSCBNo != U32_BUTT)
                {
                    /* 长签 */
                    pstCallingCB->ulSCBNo = U32_BUTT;
                    if (DOS_ADDR_VALID(pstCalleeCB))
                    {
                        sc_lcb_free(pstCalleeCB);
                        pstCalleeCB = NULL;
                    }
                    sc_scb_free(pstSCB);
                    sc_req_playback_stop(pstSCB->ulSCBNo, pstCallingCB->ulCBNo);
                    sc_req_play_sound(pstCallingCB->ulIndSCBNo, pstCallingCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);

                    return DOS_SUCC;
                }

                pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                if (DOS_ADDR_VALID(pstCalleeCB))
                {
                    sc_lcb_free(pstCalleeCB);
                    pstCalleeCB = NULL;
                }
                pstSCB->stAutoPreview.ulCalleeLegNo = U32_BUTT;
                pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_RELEASE;
                /* 可能在放回铃音，需要手动停止 */
                sc_req_playback_stop(pstSCB->ulSCBNo, pstCallingCB->ulCBNo);
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstCallingCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
            }
            else
            {
                /* 坐席挂断，修改坐席的状态，挂断客户的电话 */
                if (DOS_ADDR_VALID(pstAgentCall) && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
                {
                    sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_PREVIEW);
                    pstAgentCall->pstAgentInfo->ulLegNo = U32_BUTT;
                }

                if (DOS_ADDR_VALID(pstCallingCB))
                {
                    sc_lcb_free(pstCallingCB);
                    pstCallingCB = NULL;
                }
                pstSCB->stAutoPreview.ulCallingLegNo = U32_BUTT;
                pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_RELEASE;
                sc_req_hungup(pstSCB->ulSCBNo, pstCalleeCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
            }

            break;

        case SC_AUTO_PREVIEW_CONNECTED:
            /* 这个时候挂断，就是正常释放的节奏，处理完就好 */
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent connected.");

            pstCallingCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stAutoPreview.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCallingCB) || DOS_ADDR_INVALID(pstCalleeCB))
            {
                /* 异常 */
                DOS_ASSERT(0);
                if (DOS_ADDR_VALID(pstCallingCB))
                {
                    sc_lcb_free(pstCallingCB);
                }

                if (DOS_ADDR_VALID(pstCalleeCB))
                {
                    sc_lcb_free(pstCalleeCB);
                }

                sc_scb_free(pstSCB);
                break;
            }

            if (pstSCB->stAutoPreview.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCalleeCB;
                pstOtherLeg  = pstCallingCB;
                pstCallingCB->stCall.stTimeInfo.ulByeTime = pstCalleeCB->stCall.stTimeInfo.ulByeTime;
            }
            else
            {
                pstHungupLeg = pstCallingCB;
                pstOtherLeg  = pstCalleeCB;
                pstCalleeCB->stCall.stTimeInfo.ulByeTime = pstCallingCB->stCall.stTimeInfo.ulByeTime;
            }

            if (pstCalleeCB->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCalleeCB->ulOtherSCBNo);
                pstCalleeCB->ulOtherSCBNo = U32_BUTT;
            }

            if (pstCallingCB->ulOtherSCBNo != U32_BUTT)
            {
                sc_hungup_third_leg(pstCallingCB->ulOtherSCBNo);
                pstCallingCB->ulOtherSCBNo = U32_BUTT;
            }

            /* 生成话单 */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            if (sc_scb_is_exit_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                /* 如果有出局呼叫，应该先将出局呼叫删除 */
                sc_scb_remove_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCalleeCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalleeCB, NULL, ulReleasePart);
                /* 出局呼叫的话单应该用坐席那条leg产生 */
                sc_scb_remove_service(pstSCB, BS_SERV_PREVIEW_DIALING);
                sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                pstCallingCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCallingCB, NULL, ulReleasePart);
            }
            else
            {
                pstCalleeCB->stCall.ulCause = ulErrCode;
                sc_send_billing_stop2bs(pstSCB, pstCalleeCB, NULL, ulReleasePart);
            }

            /* 到这里，说明两个leg都OK */
            /*
              * 需要看看是否长签等问题，如果主/被叫LEG都长签了，需要申请SCB，将被叫LEG挂到新的SCB中
              * 否则，将需要长签的LEG作为当前业务控制块的主叫LEG，挂断另外一条LEG
              * 可能需要处理客户标记
              */
            /* release 时，肯定是有一条leg hungup了，现在的leg需要释放掉，判断另一条是不是坐席长签，如果不是需要挂断 */
            pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoPreview.ulAgentID);
            if (DOS_ADDR_INVALID(pstAgentCall) || DOS_ADDR_INVALID(pstAgentCall->pstAgentInfo))
            {
                /* 没有找到坐席 */
                sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Can not found agent by id(%u)", pstSCB->stAutoPreview.ulAgentID);
            }

            /* 判断是否需要进行，客户标记。1、是客户一端先挂断的(基础呼叫中，客户只能是PSTN，坐席只能是SIP) */
            if (pstHungupLeg == pstCalleeCB
                && DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && pstAgentCall->pstAgentInfo->ucProcesingTime != 0
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                /* 客户标记 */
                pstSCB->stMarkCustom.stSCBTag.bValid = DOS_TRUE;
                pstSCB->stMarkCustom.ulLegNo = pstOtherLeg->ulCBNo;
                pstSCB->stMarkCustom.pstAgentCall = pstAgentCall;
                pstSCB->ulCurrentSrv++;
                pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stMarkCustom.stSCBTag;

                if (pstOtherLeg->ulIndSCBNo == U32_BUTT)
                {
                    /* 非长签时，要把坐席对应的leg的结束时间，赋值给开始时间，出标记话单时使用 */
                    pstOtherLeg->stCall.stTimeInfo.ulAnswerTime = pstHungupLeg->stCall.stTimeInfo.ulByeTime;
                    for (i=0; i<SC_MAX_SERVICE_TYPE; i++)
                    {
                        pstSCB->aucServType[i] = 0;
                    }

                    if (pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);
                    }
                    else if(pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                    }
                    else
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
                    }

                    /* 将客户的号码改为主叫号码 */
                    dos_strcpy(pstOtherLeg->stCall.stNumInfo.szOriginalCalling, pstAgentCall->pstAgentInfo->szLastCustomerNum);
                }

                /* 修改坐席状态为 proc，播放 标记背景音 */
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_PROC, SC_SRV_AUTO_PREVIEW);
                sc_req_play_sound(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, SC_SND_CALL_OVER, 1, 0, 0);
                pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_IDEL;

                /* 开启定时器 */
                lRes = dos_tmr_start(&pstSCB->stMarkCustom.stTmrHandle, pstAgentCall->pstAgentInfo->ucProcesingTime * 1000, sc_agent_mark_custom_callback, (U64)pstOtherLeg->ulCBNo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    pstSCB->stMarkCustom.stTmrHandle = NULL;
                }

                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;

                if (pstSCB->stAutoPreview.ulCalleeLegNo == pstHungup->ulLegNo)
                {
                    pstSCB->stAutoPreview.ulCalleeLegNo = U32_BUTT;
                }
                else
                {
                    pstSCB->stAutoPreview.ulCallingLegNo = U32_BUTT;
                }

                pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_PROCESS;

                break;
            }

            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && !pstAgentCall->pstAgentInfo->bMarkCustomer)
            {
                pstAgentCall->pstAgentInfo->bMarkCustomer = DOS_FALSE;
            }

            /* 不需要客户标记 */
            if (pstSCB->stAutoPreview.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstSCB->stAutoPreview.ulCalleeLegNo = U32_BUTT;
            }
            else
            {
                pstSCB->stAutoPreview.ulCallingLegNo = U32_BUTT;
            }

            /* 修改坐席的状态 */
            if (DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
            {
                sc_agent_serv_status_update(pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_AUTO_PREVIEW);
            }

            if (pstOtherLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签，继续放音 */
                pstOtherLeg->ulSCBNo = U32_BUTT;
                sc_req_play_sound(pstOtherLeg->ulIndSCBNo, pstOtherLeg->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                /* 释放掉 SCB */
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
            else if (pstHungupLeg->ulIndSCBNo != U32_BUTT)
            {
                /* 长签的坐席挂断了电话，不要释放leg，解除关系就行 */
                pstHungupLeg->ulSCBNo = U32_BUTT;
                pstSCB->stAutoPreview.ulCallingLegNo = U32_BUTT;
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_RELEASE;
            }
            else
            {
                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_RELEASE;
            }

            break;

        case SC_AUTO_PREVIEW_PROCESS:
            /* 坐席处理完了，挂断 */
            pstCalleeCB = sc_lcb_get(pstSCB->stAutoPreview.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_AUTO_PREVIEW_RELEASE:
            pstCalleeCB = sc_lcb_get(pstSCB->stAutoPreview.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        default:
            sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call hungup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call release event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

U32 sc_auto_preview_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ERR_REPORT_ST    *pstErrReport       = NULL;
    U32                         ulRet               = DOS_SUCC;
    U32                         ulErrCode           = CC_ERR_NO_REASON;
    SC_LEG_CB                   *pstCallingCB       = NULL;
    SC_LEG_CB                   *pstCalleeCB        = NULL;
    SC_LEG_CB                   *pstRecordLegCB     = NULL;
    SC_MSG_CMD_RECORD_ST        stRecordRsp;

    pstErrReport = (SC_MSG_EVT_ERR_REPORT_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstErrReport) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing auto preview error event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);
    sc_log_digest_print_only(pstSCB, "Proccessing auto preview error event. status : %u", pstSCB->stAutoPreview.stSCBTag.usStatus);

    if (pstErrReport->stMsgTag.usInterErr == SC_ERR_BRIDGE_SUCC)
    {
        /* bridge 成功，判断是否需要录音 */
        if (!sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
        {
            return DOS_SUCC;
        }

        /* 判断是否需要录音 */
        pstCalleeCB = sc_lcb_get(pstSCB->stAutoPreview.ulCalleeLegNo);
        if (DOS_ADDR_VALID(pstCalleeCB)
            && pstCalleeCB->stRecord.bValid)
        {
            pstRecordLegCB = pstCalleeCB;
        }
        else
        {
            pstCallingCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB) && pstCallingCB->stRecord.bValid)
            {
                pstRecordLegCB = pstCallingCB;
            }
        }

        if (DOS_ADDR_VALID(pstRecordLegCB))
        {
            stRecordRsp.stMsgTag.ulMsgType = SC_CMD_RECORD;
            stRecordRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stRecordRsp.stMsgTag.usInterErr = 0;
            stRecordRsp.ulSCBNo = pstSCB->ulSCBNo;
            stRecordRsp.ulLegNo = pstRecordLegCB->ulCBNo;

            if (sc_send_cmd_record(&stRecordRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(pstSCB->bTrace, SC_LOG_SET_FLAG(LOG_LEVEL_INFO, SC_MOD_EVENT, SC_LOG_DISIST), "Send record cmd FAIL! SCBNo : %u", pstSCB->ulSCBNo);
            }
        }

        return DOS_SUCC;
    }

    /* 记录错误码 */
    ulErrCode = sc_errcode_transfer_from_intererr(pstErrReport->stMsgTag.usInterErr);

    switch (pstSCB->stAutoPreview.stSCBTag.usStatus)
    {
        case SC_AUTO_PREVIEW_IDEL:
        case SC_AUTO_PREVIEW_AUTH:
        case SC_AUTO_PREVIEW_QUEUE:
        case SC_AUTO_PREVIEW_AUTH2:
        case SC_AUTO_PREVIEW_EXEC:
        case SC_AUTO_PREVIEW_PROC:
            /* 发起呼叫失败，生成呼叫结果，释放资源 */
            pstCallingCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                pstCallingCB->stCall.stTimeInfo.ulByeTime = pstCallingCB->stCall.stTimeInfo.ulStartTime;
                sc_preview_task_call_result(pstSCB, pstCallingCB->ulCBNo, ulErrCode);
                sc_lcb_free(pstCallingCB);
            }

            pstCalleeCB = sc_lcb_get(pstSCB->stAutoPreview.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }
            sc_scb_free(pstSCB);
            break;

        case SC_AUTO_PREVIEW_ALERTING:
            ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoPreview.ulCallingLegNo, ulErrCode);
            break;

        case SC_AUTO_PREVIEW_ACTIVE:
        case SC_AUTO_PREVIEW_CONNECTING:
        case SC_AUTO_PREVIEW_ALERTING2:
            ulRet = sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stAutoPreview.ulCallingLegNo, ulErrCode);
            break;

        case SC_AUTO_PREVIEW_CONNECTED:
        case SC_AUTO_PREVIEW_PROCESS:
        case SC_AUTO_PREVIEW_RELEASE:
            if (pstSCB->stAutoPreview.ulCalleeLegNo != U32_BUTT)
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoPreview.ulCalleeLegNo, ulErrCode);
            }
            else
            {
                ulRet = sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoPreview.ulCallingLegNo, ulErrCode);
            }
            break;

        default:
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed call error event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */


