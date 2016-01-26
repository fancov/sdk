/**
 * @file : sc_event_lib.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 实现业务控制模块公共函数
 *
 *
 * @date: 2016年1月18日
 * @arthur: Larry
 */


#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_http_api.h"
#include "bs_pub.h"
#include "sc_pub.h"
#include "sc_res.h"
#include "sc_hint.h"

/**
 * 接续内部呼叫，发起一个往外部的呼叫，以 @a pstLegCB 为主叫，向被叫发起呼叫
 *
 * @param pstMsg
 * @param pstLegCB
 *
 * @return 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_outgoing_call_process(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB)
{
    SC_MSG_CMD_CALL_ST          stCallMsg;
    SC_LEG_CB                   *pstCalleeLegCB = NULL;
    U32                         ulRet = DOS_FAIL;

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.stMsgTag.usInterErr = 0;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;

    pstCalleeLegCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstCalleeLegCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc SCB fail.");

        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, CC_ERR_SC_SYSTEM_BUSY);
    }

    stCallMsg.ulLCBNo = pstCalleeLegCB->ulCBNo;

    pstCalleeLegCB->stCall.bValid = DOS_TRUE;
    pstCalleeLegCB->stCall.ucStatus = SC_LEG_INIT;
    pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
    pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;

    pstSCB->stCall.ulCalleeLegNo = pstCalleeLegCB->ulCBNo;

    /* 维护一下主叫号码 */
    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCalling
                    , sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCalling)
                    , pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCallee
                    , sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCallee)
                    , pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);

    /* 新LEG处理一下号码 */
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    /* 出局呼叫需要找路由 */
    pstSCB->stCall.ulRouteID = sc_route_search(pstSCB, pstCalleeLegCB->stCall.stNumInfo.szRealCalling, pstCalleeLegCB->stCall.stNumInfo.szRealCallee);
    if (U32_BUTT == pstSCB->stCall.ulRouteID)
    {
        sc_trace_scb(pstSCB, "no route to pstn.");

        sc_lcb_free(pstCalleeLegCB);

        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, CC_ERR_SC_NO_ROUTE);
    }

    /* 如果没有找到中继，需要拒绝护叫 */
    pstCalleeLegCB->stCall.ulTrunkCnt = sc_route_get_trunks(pstSCB->stCall.ulRouteID, pstCalleeLegCB->stCall.aulTrunkList, SC_MAX_TRUCK_NUM);
    if (0 == pstCalleeLegCB->stCall.ulTrunkCnt)
    {
        sc_trace_scb(pstSCB, "no trunk to pstn. route id:%u", pstSCB->stCall.ulRouteID);

        sc_lcb_free(pstCalleeLegCB);

        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, CC_ERR_SC_NO_TRUNK);
    }

    ulRet = sc_send_cmd_new_call(&stCallMsg.stMsgTag);
    if (ulRet == DOS_SUCC)
    {
        pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;
    }

    return ulRet;
}

/**
 * 呼叫接续，呼叫一个内部的SIP账户，以 @a pstLegCB 为主叫，向被叫发起呼叫
 *
 * @param pstMsg
 * @param pstLegCB
 *
 * @return 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_internal_call_process(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    SC_AGENT_NODE_ST *pstAgent    = NULL;
    SC_MSG_CMD_CALL_ST          stCallMsg;
    SC_SRV_CB                  *pstSCBAgent = NULL;
    SC_LEG_CB                  *pstLegCBAgent = NULL;
    SC_LEG_CB                  *pstCalleeLeg = NULL;
    U32                        ulCustomerID;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* 查找被叫，如果被叫已经长签了，直接连接就好，主意业务控制块合并 */
    /* 被叫有可能是分机号，SIP账户 */
    ulCustomerID = sc_sip_account_get_customer(pstLegCB->stCall.stNumInfo.szOriginalCallee);
    if (U32_BUTT == ulCustomerID)
    {
        /* 如果不能通过SIP账户获取到用户ID，需要查看是否是分机号，同时将分机号对应的SIP账户作为真实呼叫使用的号码 */
        if (sc_sip_account_get_by_extension(pstSCB->ulCustomerID
                        , pstLegCB->stCall.stNumInfo.szOriginalCallee
                        , pstLegCB->stCall.stNumInfo.szRealCallee
                        , sizeof(pstLegCB->stCall.stNumInfo.szRealCallee)) != DOS_SUCC)
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
    }
    else
    {
        dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCallee
                        , sizeof(pstLegCB->stCall.stNumInfo.szRealCallee)
                        , pstLegCB->stCall.stNumInfo.szOriginalCallee);
    }

    /* 维护一下主叫号码 */
    dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCalling
                    , sizeof(pstLegCB->stCall.stNumInfo.szRealCalling)
                    , pstLegCB->stCall.stNumInfo.szOriginalCalling);

    pstAgent = sc_agent_get_by_sip_acc(pstLegCB->stCall.stNumInfo.szRealCallee);
    if (DOS_ADDR_INVALID(pstAgent)
        || DOS_ADDR_INVALID(pstAgent->pstAgentInfo))
    {
        goto processing;
    }

    if (pstAgent->pstAgentInfo->ulLegNo < SC_LEG_CB_SIZE)
    {
        pstLegCBAgent = sc_lcb_get(pstAgent->pstAgentInfo->ulLegNo);
        if (DOS_ADDR_INVALID(pstLegCBAgent) || pstLegCBAgent->stCall.ucLocalMode != SC_LEG_LOCAL_SIGNIN)
        {
            goto processing;
        }

        pstSCBAgent = sc_scb_get(pstLegCBAgent->ulSCBNo);
        if (DOS_ADDR_INVALID(pstSCBAgent))
        {
            goto processing;
        }
    }

processing:
    /* 被叫有业务 */
    if (DOS_ADDR_VALID(pstSCBAgent))
    {
        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstLegCB->ulCBNo, CC_ERR_SC_USER_BUSY);
    }

    pstCalleeLeg = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstCalleeLeg))
    {
        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstLegCB->ulCBNo, CC_ERR_SC_SYSTEM_BUSY);
    }

    pstCalleeLeg->ulSCBNo = pstSCB->ulSCBNo;
    pstCalleeLeg->stCall.bValid = DOS_TRUE;
    pstCalleeLeg->stCall.ucStatus = SC_LEG_INIT;
    pstCalleeLeg->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
    pstCalleeLeg->stCall.ulCodecCnt = 0;
    pstCalleeLeg->stCall.ulTrunkCnt = 0;
    pstCalleeLeg->stCall.szEIXAddr[0] = '\0';

    dos_snprintf(pstCalleeLeg->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLeg->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(pstCalleeLeg->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLeg->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(pstCalleeLeg->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLeg->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(pstCalleeLeg->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLeg->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(pstCalleeLeg->stCall.stNumInfo.szCallee, sizeof(pstCalleeLeg->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(pstCalleeLeg->stCall.stNumInfo.szCalling, sizeof(pstCalleeLeg->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szRealCalling);

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.stMsgTag.usInterErr = 0;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.ulLCBNo = pstCalleeLeg->ulCBNo;

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        sc_lcb_free(pstCalleeLeg);
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send new call request fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static U32 sc_incoming_call_sip_proc(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB, U32 ulSipID, U32 *pulErrCode)
{
    S8    szCallee[32] = { 0, };
    SC_AGENT_NODE_ST *pstAgentInfo = NULL;
    SC_LEG_CB *pstCalleeLegCB = NULL;
    SC_MSG_CMD_CALL_ST stCallMsg;

    if (DOS_SUCC != sc_sip_account_get_by_id(ulSipID, szCallee, sizeof(szCallee)))
    {
        DOS_ASSERT(0);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "DID number %s seems donot bind a SIP User ID, Reject Call.", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
        *pulErrCode = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Start find agent by userid(%s)", szCallee);
    pstAgentInfo = sc_agent_get_by_sip_acc(szCallee);
    if (DOS_ADDR_VALID(pstAgentInfo))
    {
        /* TODO 绑定了坐席。判断是否录音，坐席是否长签，坐席弹屏 */
    }

    /* 申请一个新的leg，发起呼叫 */
    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.stMsgTag.usInterErr = 0;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;

    pstCalleeLegCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstCalleeLegCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc SCB fail.");

        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, CC_ERR_SC_SYSTEM_BUSY);
    }

    stCallMsg.ulLCBNo = pstCalleeLegCB->ulCBNo;

    pstCalleeLegCB->stCall.bValid = DOS_TRUE;
    pstCalleeLegCB->stCall.ucStatus = SC_LEG_INIT;
    pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
    pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;

    pstSCB->stCall.ulCalleeLegNo = pstCalleeLegCB->ulCBNo;

    /* 维护一下主叫号码 */
    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCalling), pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);
    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCallee), pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);

    /* 新LEG处理一下号码 */
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        *pulErrCode = CC_ERR_SC_MESSAGE_SENT_ERR;

        return DOS_FAIL;
    }

    pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

    return DOS_SUCC;
}

U32 sc_agent_call_by_id(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB, U32 ulAgentID)
{
    SC_AGENT_NODE_ST *pstAgentNode = NULL;
    SC_LEG_CB *pstCalleeLegCB = NULL;
    SC_MSG_CMD_CALL_ST stCallMsg;
    U32 ulErrCode = CC_ERR_NO_REASON;
    S8 szCallee[SC_NUM_LENGTH] = {0,};

    if (DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstCallingLegCB))
    {
        return DOS_FAIL;
    }

    pstAgentNode = sc_agent_get_by_id(ulAgentID);
    if (DOS_ADDR_INVALID(pstAgentNode))
    {
        /* 没有找到坐席 */
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Not found agnet by id %u", ulAgentID);

        return DOS_FAIL;
    }

    /* 判断坐席的状态 */

    /* 判断坐席是否长签 */

    /* 申请一个新的leg，发起呼叫 */
    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.stMsgTag.usInterErr = 0;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;

    pstCalleeLegCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstCalleeLegCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc SCB fail.");
        ulErrCode = CC_ERR_SC_SYSTEM_BUSY;

        goto process_fail;
    }

    stCallMsg.ulLCBNo = pstCalleeLegCB->ulCBNo;

    pstCalleeLegCB->stCall.bValid = DOS_TRUE;
    pstCalleeLegCB->stCall.ucStatus = SC_LEG_INIT;
    //pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
    pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;

    pstSCB->stCall.ulCalleeLegNo = pstCalleeLegCB->ulCBNo;

    /* 是否需要录音 */
    if (pstAgentNode->pstAgentInfo->bRecord)
    {
        if (sc_scb_set_service(pstSCB, BS_SERV_RECORDING))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add record service fail.");
        }
        else
        {
            pstCalleeLegCB->stRecord.bValid = DOS_TRUE;
        }
    }

    switch (pstAgentNode->pstAgentInfo->ucBindType)
    {
        case AGENT_BIND_SIP:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szUserID);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
            break;

        case AGENT_BIND_TELE:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szTelePhone);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");
                ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                goto process_fail;
            }
            break;

        case AGENT_BIND_MOBILE:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szMobile);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");
                ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                goto process_fail;
            }
            break;

        case AGENT_BIND_TT_NUMBER:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szTTNumber);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
            break;

        default:
            break;
    }

    /* 维护一下主叫号码 */
    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCalling), pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);
    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCallee), szCallee);

    /* 新LEG处理一下号码 */
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    if (pstCalleeLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
    {
        /* 这里需要去认证 */
        if (sc_send_usr_auth2bs(pstSCB, pstCalleeLegCB) != DOS_SUCC)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth fail.");

            goto process_fail;
        }
        pstSCB->stCall.stSCBTag.usStatus = SC_CALL_AUTH_CALLEE;

        return DOS_SUCC;
    }

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        ulErrCode = CC_ERR_SC_MESSAGE_SENT_ERR;

        return DOS_FAIL;
    }

    pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

    return DOS_SUCC;

process_fail:

    sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, ulErrCode);
    return DOS_FAIL;
}

/**
 * 函数: U32 sc_ep_incoming_call_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理由PSTN呼入到SIP测的呼叫
 * 参数:
 *      esl_event_t *pstEvent   : ESL 事件
 *      esl_handle_t *pstHandle : 发送数据的handle
 *      SC_SCB_ST *pstSCB       : 业务控制块
 * 返回值: 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_incoming_call_proc(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB)
{
    U32   ulCustomerID = U32_BUTT;
    U32   ulBindType = U32_BUTT;
    U32   ulBindID = U32_BUTT;
    U32   ulErrCode = CC_ERR_NO_REASON;
    U32  ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        goto proc_finished;
    }

    ulCustomerID = pstSCB->ulCustomerID;
    if (U32_BUTT != ulCustomerID)
    {
        if (sc_did_bind_info_get(pstCallingLegCB->stCall.stNumInfo.szOriginalCallee, &ulBindType, &ulBindID) != DOS_SUCC
            || ulBindType >=SC_DID_BIND_TYPE_BUTT
            || U32_BUTT == ulBindID)
        {
            DOS_ASSERT(0);

            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Cannot get the bind info for the DID number %s, Reject Call.", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
            ulErrCode = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;
            goto proc_finished;
        }

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Process Incoming Call, DID Number %s. Bind Type: %u, Bind ID: %u"
                        , pstCallingLegCB->stCall.stNumInfo.szOriginalCallee
                        , ulBindType
                        , ulBindID);

        switch (ulBindType)
        {
            case SC_DID_BIND_TYPE_SIP:
                /* 绑定sip分机时，不判断坐席的状态, 不改变坐席的状态, 只是判断一下是否需要录音 */
                ulRet = sc_incoming_call_sip_proc(pstSCB, pstCallingLegCB, ulBindID, &ulErrCode);
                break;

            case SC_DID_BIND_TYPE_QUEUE:
                /*
                sc_ep_esl_execute("answer", NULL, pstSCB->szUUID);
                if (sc_ep_call_queue_add(pstSCB, ulBindID, DOS_TRUE) != DOS_SUCC)
                {
                    DOS_ASSERT(0);

                    sc_logr_info(pstSCB, SC_ESL, "Add Call to call waiting queue FAIL.Callee: %s. Reject Call.", pstSCB->szCalleeNum);
                    ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                    goto proc_fail;
                }
                */
                break;
            case SC_DID_BIND_TYPE_AGENT:
                /* 呼叫坐席 */
                ulRet = sc_agent_call_by_id(pstSCB, pstCallingLegCB, ulBindID);
                break;

            default:
                DOS_ASSERT(0);
                ulErrCode = CC_ERR_SC_CONFIG_ERR;
                //sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "DID number %s has bind an error number, Reject Call.", pstSCB->szCalleeNum);
                goto proc_finished;
        }
    }
    else
    {
        DOS_ASSERT(0);
        ulErrCode = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;
        //sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Destination is not a DID number, Reject Call. Destination: %s", pstSCB->szCalleeNum);
        ulRet = DOS_FAIL;

    }

proc_finished:
    //SC_SCB_SET_STATUS(pstSCB, SC_SCB_EXEC);
    if (ulRet != DOS_SUCC)
    {
        if (pstSCB)
        {
            //sc_ep_hangup_call_with_snd(pstSCB, ulErrCode);
        }

        //sc_log_digest_print("Start proc incoming call FAIL.");

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 根据 @a pstLCB 中的信息，发起一个到PSTN的呼叫
 *
 * @param pstSCB
 * @param pstLCB
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note 如果该函数中出现异常，@a pstSCB 和 @a pstLCB所指向的控制块不会被释放，调用的地方视情况而定
 */
U32 sc_make_call2pstn(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB)
{
    SC_MSG_CMD_CALL_ST          stCallMsg;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* @TODO 做号码变换，并将结果COPY到 pstLCB->stCall.stNumInfo.szRealCalling 和 pstLCB->stCall.stNumInfo.szRealCalling*/
    dos_snprintf(pstLCB->stCall.stNumInfo.szRealCalling, sizeof(pstLCB->stCall.stNumInfo.szRealCalling), pstLCB->stCall.stNumInfo.szOriginalCalling);
    dos_snprintf(pstLCB->stCall.stNumInfo.szRealCallee, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCallee);

    /* 出局呼叫需要找路由 */
    pstSCB->stCall.ulRouteID = sc_route_search(pstSCB, pstLCB->stCall.stNumInfo.szRealCalling, pstLCB->stCall.stNumInfo.szRealCallee);
    if (U32_BUTT == pstSCB->stCall.ulRouteID)
    {
        sc_trace_scb(pstSCB, "no route to pstn.");

        pstLCB->stCall.ulCause = CC_ERR_SC_NO_ROUTE;
        return DOS_FAIL;
    }

    /* 如果没有找到中继，需要拒绝护叫 */
    pstLCB->stCall.ulTrunkCnt = sc_route_get_trunks(pstSCB->stCall.ulRouteID, pstLCB->stCall.aulTrunkList, SC_MAX_TRUCK_NUM);
    if (0 == pstLCB->stCall.ulTrunkCnt)
    {
        sc_trace_scb(pstSCB, "no trunk to pstn. route id:%u", pstSCB->stCall.ulRouteID);

        pstLCB->stCall.ulCause = CC_ERR_SC_NO_TRUNK;
        return DOS_FAIL;
    }

    /* @TODO 做号码变换，并将结果COPY到 pstLCB->stCall.stNumInfo.szCalling 和 pstLCB->stCall.stNumInfo.szCallee*/
    dos_snprintf(pstLCB->stCall.stNumInfo.szCallee, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstLCB->stCall.stNumInfo.szCalling, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstLCB->stCall.stNumInfo.szCID, sizeof(pstLCB->stCall.stNumInfo.szCID), pstLCB->stCall.stNumInfo.szOriginalCallee);

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo   = pstSCB->ulSCBNo;
    stCallMsg.ulLCBNo = pstLCB->ulCBNo;
    stCallMsg.ulSCBNo = pstLCB->ulSCBNo;

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send new call request fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 根据 @a pstLCB 中的信息，发起一个到TT号的呼叫
 *
 * @param pstSCB
 * @param pstLCB
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note 如果该函数中出现异常，@a pstSCB 和 @a pstLCB所指向的控制块不会被释放，调用的地方视情况而定
 */
U32 sc_make_call2eix(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB)
{
    SC_MSG_CMD_CALL_ST          stCallMsg;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo   = pstSCB->ulSCBNo;
    stCallMsg.ulLCBNo = pstLCB->ulCBNo;
    stCallMsg.ulSCBNo = pstLCB->ulSCBNo;

    if (sc_eix_dev_get_by_tt(pstLCB->stCall.stNumInfo.szRealCallee, pstLCB->stCall.szEIXAddr, sizeof(pstLCB->stCall.szEIXAddr)) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Get EIX ip fail by the TT number. %s", pstLCB->stCall.stNumInfo.szRealCallee);
        return DOS_FAIL;
    }

    dos_snprintf(pstLCB->stCall.stNumInfo.szRealCallee, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstLCB->stCall.stNumInfo.szRealCalling, sizeof(pstLCB->stCall.stNumInfo.szRealCalling), pstLCB->stCall.stNumInfo.szOriginalCalling);
    dos_snprintf(pstLCB->stCall.stNumInfo.szCallee, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstLCB->stCall.stNumInfo.szCalling, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstLCB->stCall.stNumInfo.szCID, sizeof(pstLCB->stCall.stNumInfo.szCID), pstLCB->stCall.stNumInfo.szOriginalCallee);

    /* 呼叫EIX强制使用G723、G729 */
    pstLCB->stCall.aucCodecList[0] = PT_G723;
    pstLCB->stCall.aucCodecList[1] = PT_G729;
    pstLCB->stCall.ulCodecCnt = 2;

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send new call request fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 根据 @a pstLCB 中的信息，发起一个到SIP账户的呼叫
 *
 * @param pstSCB
 * @param pstLCB
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note 如果该函数中出现异常，@a pstSCB 和 @a pstLCB所指向的控制块不会被释放，调用的地方视情况而定
 */
U32 sc_make_call2sip(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB)
{
    SC_MSG_CMD_CALL_ST          stCallMsg;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo   = pstSCB->ulSCBNo;
    stCallMsg.ulLCBNo = pstLCB->ulCBNo;
    stCallMsg.ulSCBNo = pstLCB->ulSCBNo;

    dos_snprintf(pstLCB->stCall.stNumInfo.szRealCallee, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstLCB->stCall.stNumInfo.szRealCalling, sizeof(pstLCB->stCall.stNumInfo.szRealCalling), pstLCB->stCall.stNumInfo.szOriginalCalling);
    dos_snprintf(pstLCB->stCall.stNumInfo.szCallee, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstLCB->stCall.stNumInfo.szCalling, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstLCB->stCall.stNumInfo.szCID, sizeof(pstLCB->stCall.stNumInfo.szCID), pstLCB->stCall.stNumInfo.szOriginalCallee);

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send new call request fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_voice_verify_proc(U32 ulCustomer, S8 *pszNumber, S8 *pszPassword, U32 ulPlayCnt)
{
    SC_SRV_CB    *pstSCB = NULL;
    SC_LEG_CB    *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pszNumber) || '\0' == pszNumber[0])
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Empty number for voice verify");
        goto fail_proc;
    }

    if (DOS_ADDR_INVALID(pszPassword) || '\0' == pszPassword[0])
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Empty verify code for voice verify");
        goto fail_proc;
    }

    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc SCB fail");
        goto fail_proc;
    }

    pstLCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc LCB fail");
        goto fail_proc;
    }

    pstSCB->stVoiceVerify.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_INIT;
    pstSCB->stVoiceVerify.ulLegNo = pstLCB->ulCBNo;
    pstSCB->stVoiceVerify.ulTipsHitNo1 = SC_SND_YOUR_CODE_IS;
    pstSCB->stVoiceVerify.ulTipsHitNo2 = SC_SND_BUTT;
    dos_snprintf(pstSCB->stVoiceVerify.szVerifyCode, sizeof(pstSCB->stVoiceVerify.szVerifyCode), pszPassword);
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stVoiceVerify.stSCBTag;
    if (sc_scb_set_service(pstSCB, BS_SERV_VERIFY) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add voice verify service to scb fail.");
        goto fail_proc;
    }

    pstLCB->ulSCBNo = pstSCB->ulSCBNo;
    pstLCB->stCall.bValid = DOS_TRUE;

    if (sc_caller_setting_select_number(ulCustomer, 0, SC_SRC_CALLER_TYPE_ALL
                        , pstLCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is no caller for number verify.");
        goto fail_proc;
    }

    dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, SC_NUM_LENGTH, pszNumber);

    if (sc_send_usr_auth2bs(pstSCB, pstLCB) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth request fail.");
        goto fail_proc;
    }

    pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_AUTH;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Send auth request for number verify succ. Customer: %u, Code: %s, Callee: %s", ulCustomer, pszPassword, pszNumber);
    return DOS_SUCC;

fail_proc:
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

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Send auth request for number verify FAIL. Customer: %u, Code: %s, Callee: %s", ulCustomer, pszPassword, pszNumber);

    return DOS_FAIL;
}

U32 sc_call_ctrl_call_agent(U32 ulCurrentAgent, SC_AGENT_NODE_ST  *pstAgentNode)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_call_sip(U32 ulAgent, S8 *pszSipNumber)
{

    return DOS_SUCC;
}

U32 sc_call_ctrl_call_out(U32 ulAgent, U32 ulTaskID, S8 *pszNumber)
{
    SC_AGENT_NODE_ST *pstAgentNode = NULL;
    SC_SRV_CB        *pstSCB       = NULL;
    SC_LEG_CB        *pstLCB       = NULL;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Request call out. Agent: %u, Task: %u, Number: %u", ulAgent, ulTaskID, NULL == pszNumber ? "NULL" : pszNumber);

    if (DOS_ADDR_INVALID(pszNumber) || '\0' == pszNumber[0])
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Request call out. Number is empty");
        return DOS_FAIL;
    }

    pstAgentNode = sc_agent_get_by_id(ulAgent);
    if (DOS_ADDR_INVALID(pstAgentNode))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Cannot found the agent %u", ulAgent);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Alloc lcb fail");
        return DOS_FAIL;
    }

    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        sc_lcb_free(pstLCB);
        pstLCB = NULL;

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Alloc scb fail");
        return DOS_FAIL;
    }

    pstLCB->stCall.bValid = DOS_SUCC;
    pstLCB->stCall.ucStatus = SC_LEG_INIT;

    pstSCB->stPreviewCall.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_IDEL;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stPreviewCall.stSCBTag;
    if (sc_scb_set_service(pstSCB, BS_SERV_PREVIEW_DIALING) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Set service fail.");

        goto process_fail;
    }

    switch (pstAgentNode->pstAgentInfo->ucBindType)
    {
        case AGENT_BIND_SIP:
            dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szUserID);
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
            break;

        case AGENT_BIND_TELE:
            dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szTelePhone);
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");

                goto process_fail;
            }
            break;

        case AGENT_BIND_MOBILE:
            dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szMobile);
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");

                goto process_fail;
            }
            break;

        case AGENT_BIND_TT_NUMBER:
            dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szTTNumber);
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
            break;

        default:
            break;
    }
    dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstLCB->stCall.stNumInfo.szOriginalCalling), pszNumber);

    if (pstAgentNode->pstAgentInfo->bRecord)
    {
        if (sc_scb_set_service(pstSCB, BS_SERV_RECORDING))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add record service fail.");
        }
        else
        {
            pstLCB->stRecord.bValid = DOS_TRUE;
        }
    }

    pstSCB->stPreviewCall.ulCallingLegNo = pstLCB->ulCBNo;
    pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_AUTH;
    pstLCB->ulSCBNo = pstSCB->ulSCBNo;
    pstAgentNode->pstAgentInfo->ulLegNo = pstLCB->ulCBNo;

    if (sc_send_usr_auth2bs(pstSCB, pstLCB) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth fail.");

        goto process_fail;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Request call out. send auth succ.");

    return DOS_SUCC;

process_fail:
    if (DOS_ADDR_VALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    if (DOS_ADDR_VALID(pstLCB))
    {
        sc_lcb_free(pstLCB);
        pstLCB = DOS_SUCC;
    }

    return DOS_FAIL;
}

U32 sc_call_ctrl_transfer(U32 ulAgent, U32 ulAgentCalled, BOOL bIsAttend)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_hold(U32 ulAgent, BOOL isHold)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_unhold(U32 ulAgent)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_hangup(U32 ulAgent)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_hangup_all(U32 ulAgent)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_intercept(U32 ulTaskID, U32 ulAgent, U32 ulCustomerID, U32 ulType, S8 *pszCallee)
{
    SC_AGENT_NODE_ST *pstAgentNode = NULL;
    SC_LEG_CB        *pstLCB       = NULL;
    SC_SRV_CB        *pstSCB       = NULL;
    SC_LEG_CB        *pstLCBAgent  = NULL;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Request intercept. Agent: %u, Task: %u, Number: %s", ulAgent, ulTaskID, NULL == pszCallee ? "NULL" : pszCallee);

    if (DOS_ADDR_INVALID(pszCallee))
    {
        return DOS_FAIL;
    }

    pstAgentNode = sc_agent_get_by_id(ulAgent);
    if (DOS_ADDR_INVALID(pstAgentNode))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Cannot found the agent %u", ulAgent);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Agent CB Error %u", ulAgent);
        return DOS_FAIL;
    }

    if (pstAgentNode->pstAgentInfo->ulLegNo >= SC_LEG_CB_SIZE)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Agent not in calling %u", ulAgent);
        return DOS_FAIL;
    }

    pstLCBAgent = sc_lcb_get(pstAgentNode->pstAgentInfo->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCBAgent))
    {
        return DOS_FAIL;
    }

    pstSCB = sc_scb_get(pstLCBAgent->ulSCBNo);
    if (DOS_ADDR_INVALID(pstSCB))
    {
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Alloc lcb fail");
        return DOS_FAIL;
    }

    pstSCB->stInterception.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_IDEL;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stInterception.stSCBTag;

    pstLCB->stCall.bValid = DOS_SUCC;
    pstLCB->stCall.ucStatus = SC_LEG_INIT;

/*
    if (!sc_ep_black_list_check(ulCustomerID, pszCallee))
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Callee in blocak list. (%s)", pszCallee);

        goto process_fail;
    }
*/
    /* 主叫号码， 从主叫号码组中获取 */
    if (sc_caller_setting_select_number(ulCustomerID, 0, SC_SRC_CALLER_TYPE_ALL
                        , pstLCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is no caller for number verify.");

        goto process_fail;
    }

    /* 被叫号码 */
    dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pszCallee);
    switch (ulType)
    {
        case AGENT_BIND_SIP:
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
            break;

        case AGENT_BIND_TELE:
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");

                goto process_fail;
            }
            break;

        case AGENT_BIND_MOBILE:
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");

                goto process_fail;
            }
            break;

        case AGENT_BIND_TT_NUMBER:
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
            break;

        default:
            break;
    }

    pstSCB->stInterception.ulLegNo = pstLCB->ulCBNo;
    pstSCB->stInterception.ulAgentLegNo = pstLCBAgent->ulCBNo;
    pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_AUTH;
    pstLCB->ulSCBNo = pstSCB->ulSCBNo;

    if (sc_send_usr_auth2bs(pstSCB, pstLCB) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth fail.");

        goto process_fail;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Request call out. send auth succ.");

    return DOS_SUCC;

process_fail:

    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_lcb_free(pstLCB);
        pstLCB = NULL;
    }

    return DOS_FAIL;
}


U32 sc_call_ctrl_proc(U32 ulAction, U32 ulTaskID, U32 ulAgent, U32 ulCustomerID, U32 ulType, S8 *pszCallee, U32 ulFlag, U32 ulCalleeAgentID)
{
    U32 ulRet = DOS_FAIL;

    if (ulAction >= SC_API_CALLCTRL_BUTT)
    {
        DOS_ASSERT(0);

        goto proc_end;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_EVENT), "Start process call ctrl msg. Action: %u, Agent: %u, Customer: %u, Task: %u, Caller: %s"
                    , ulAction, ulAgent, ulCustomerID, ulTaskID, pszCallee ? pszCallee : "");

    switch (ulAction)
    {
        case SC_API_MAKE_CALL:
        case SC_API_TRANSFOR_ATTAND:
        case SC_API_TRANSFOR_BLIND:
        case SC_API_CONFERENCE:
        case SC_API_HOLD:
        case SC_API_UNHOLD:
            ulRet = DOS_SUCC;
            break;

        case SC_API_HANGUP_CALL:
            ulRet = sc_call_ctrl_hangup_all(ulAgent);
            break;

        case SC_API_RECORD:
#if 0
            /* 查找坐席 */
            if (sc_acd_get_agent_by_id(&stAgentInfo, ulAgent) != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Cannot hangup call for agent with id %u. Agent not found..", ulAgent);
                goto proc_fail;
            }

            if (stAgentInfo.usSCBNo >= SC_MAX_SCB_NUM)
            {
                DOS_ASSERT(0);

                sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent handle a invalid SCB No(%u).", ulAgent, stAgentInfo.usSCBNo);
                goto proc_fail;
            }

            pstSCB = sc_scb_get(stAgentInfo.usSCBNo);
            if (DOS_ADDR_INVALID(pstSCB) || !pstSCB->bValid)
            {
                DOS_ASSERT(0);

                sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent handle a SCB(%u) is invalid.", ulAgent, stAgentInfo.usSCBNo);
                goto proc_fail;
            }

            if ('\0' == pstSCB->szUUID[0])
            {
                DOS_ASSERT(0);

                sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent handle a SCB(%u) without UUID.", ulAgent, stAgentInfo.usSCBNo);
                goto proc_fail;
            }

            if (sc_ep_record(pstSCB) != DOS_SUCC)
            {
                goto proc_fail;
            }
#endif
            break;

        case SC_API_WHISPERS:
            ulRet = DOS_SUCC;
            break;
        case SC_API_INTERCEPT:
            ulRet = sc_call_ctrl_intercept(ulTaskID, ulAgent, ulCustomerID, ulType, pszCallee);
            break;
        default:
            ulRet = DOS_FAIL;
    }

proc_end:
    if (ulRet != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Process call ctrl msg FAIL. Action: %u, Agent: %u, Customer: %u, Task: %u, Caller: %s"
                    , ulAction, ulAgent, ulCustomerID, ulTaskID, pszCallee);
    }
    else
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_EVENT), "Finished to process call ctrl msg. Action: %u, Agent: %u, Customer: %u, Task: %u, Caller: %s"
                    , ulAction, ulAgent, ulCustomerID, ulTaskID, pszCallee);
    }

    return ulRet;
}

U32 sc_demo_task(U32 ulCustomerID, S8 *pszCallee, S8 *pszAgentNum, U32 ulAgentID)
{
    return DOS_SUCC;
}

U32 sc_demo_preview(U32 ulCustomerID, S8 *pszCallee, S8 *pszAgentNum, U32 ulAgentID)
{
    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */

