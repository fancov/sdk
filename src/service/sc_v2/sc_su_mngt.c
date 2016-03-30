/**
 * @file : sc_su_mngt.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 业务单元管理
 *
 * @date: 2016年1月9日
 * @arthur: Larry
 */

#ifdef __cplusplus
 extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include <esl.h>

#include "sc_def.h"
#include "sc_debug.h"
#include "sc_pub.h"
#include "sc_su.h"
#include "sc_hint.h"


/** 内部请求处理线程 */
pthread_t              g_pthCommandThread;

/** 内部请求处理线程是否正常工作 */
BOOL                   g_blCommandThreadisRunning = DOS_FALSE;

/** 管理background job的HASH表 */
HASH_TABLE_S          *g_pstBGJobHash;

/** 管理background job的HASH表锁 */
pthread_mutex_t        g_mutexBGJobHash  = PTHREAD_MUTEX_INITIALIZER;

/** 业务子层上报时间消息队列 */
DLL_S                 g_stCommandQueue;

/** 业务子层上报时间消息队列锁 */
pthread_mutex_t       g_mutexCommandQueue = PTHREAD_MUTEX_INITIALIZER;

/** 业务子层上报时间消息队列条件变量 */
pthread_cond_t        g_condCommandQueue = PTHREAD_COND_INITIALIZER;

/** LEG业务控制块列表 */
SC_LEG_CB             *g_pstLegCB   = NULL;
/** LEG业务控制块锁 */
pthread_mutex_t       g_mutexLegCB = PTHREAD_MUTEX_INITIALIZER;

/** 呼叫LEG控制块 */
HASH_TABLE_S         *g_pstLCBHash;
/** 保护呼叫控制块使用的互斥量 */
pthread_mutex_t      g_mutexLCBHash = PTHREAD_MUTEX_INITIALIZER;

/* 呼出统计，如果超过一定阀值，就重启业务控制模块 */
U32                  g_ulOutgoingCallCnt = 0;


/**
 * 处理CHANNEL_CREATE事件
 *
 * @param esl_event_t *pstEvent ESL事件
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_create(esl_event_t *pstEvent)
{
    S8  *pszLegCBNo   = NULL;
    S8  *pszCalling   = NULL;
    S8  *pszCallee    = NULL;
    S8  *pszANI       = NULL;
    S8  *pszCallSrc   = NULL;
    S8  *pszTrunkIP   = NULL;
    S8  *pszGwName    = NULL;
    S8  *pszCallDirection = NULL;
    S8  *pszLegUUID       = NULL;
    S8  szCMD[128]        = { 0 };
    U32 ulLCBNo           = U32_BUTT;
    U32 ulThreshold       = -1;
    SC_LEG_CB *pstLCB     = NULL;
    SC_MSG_EVT_CALL_ST       stCallEvent;
    SC_MSG_EVT_ERR_REPORT_ST stErrReport;

    ulThreshold = config_get_exit_threshold();
    if (ulThreshold < 0)
    {
        ulThreshold = MAX_FAIL_CALL_CNT;
    }

    if (g_ulOutgoingCallCnt > ulThreshold)
    {
        dos_snprintf(szCMD, sizeof(szCMD), "Outgoing call failed count %u. The system will be shutdown with 2 seconds", ulThreshold);
        dos_log(LOG_LEVEL_EMERG, LOG_TYPE_RUNINFO, szCMD);
        dos_task_delay(2000);
        exit(0);
    }

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_SUCC;
    }

    /* 从ESL EVENT中回去相关呼叫信息 */
    /*
     * 1. PSTN呼入
     *   特征:
     *       Call Direction: Inbound;
     *       Profile Name:   external
     *   获取的信息:
     *       对端IP或者gateway name获取呼叫的网关ID
     *       主被叫信息, 来电信信息
     * 2. 呼出到PSTN
     *   特征:
     *       Call Direction: outbount;
     *       Profile Name:   external;
     *   获取的信息:
     *       对端IP或者gateway name获取呼叫的网关ID
     *       主被叫信息, 来电信信息
     *       获取用户信息标示
     * 3. 内部呼叫
     *   特征:
     *       Call Direction: Inbound;
     *       Profile Name:   internal;
     *   获取的信息:
     *       主被叫信息, 来电信信息
     *       获取用户信息标示
     */

    pszLegUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszLegUUID))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pszLegCBNo = esl_event_get_header(pstEvent, "variable_lcb_number");
    if (DOS_ADDR_VALID(pszLegCBNo))
    {
        if (dos_atoul(pszLegCBNo, &ulLCBNo) < 0)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_SU), "Channel vaiable is broken. %s", pszLegCBNo);

            return DOS_FAIL;
        }

        pstLCB = sc_lcb_get(ulLCBNo);
    }
    else
    {
        pstLCB = sc_lcb_alloc();
    }

    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_SU), "Create leg without a LEG CB. Leg No:%s(%u)"
                              , pszLegCBNo ? pszLegCBNo : "NULL"
                              , ulLCBNo);

        goto proc_fail;
    }

    /** 没有获取到profile说明有问题 */
    pszCallSrc = esl_event_get_header(pstEvent, "variable_sofia_profile_name");
    if (DOS_ADDR_INVALID(pszCallSrc))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_SU), "Create leg without profile.");

        goto proc_fail;
    }

    /** 没有获取到方向说明有问题 */
    pszCallDirection = esl_event_get_header(pstEvent, "Call-Direction");
    if (DOS_ADDR_INVALID(pszCallDirection))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_SU), "Create leg without call direction.");

        goto proc_fail;
    }

    pstLCB->stCall.ucLocalMode = SC_LEG_LOCAL_NORMAL;
    pstLCB->stCall.stTimeInfo.ulStartTime = time(NULL);
    if (pstLCB->ulSCBNo == U32_BUTT
        && pstLCB->ulIndSCBNo == U32_BUTT)
    {
        if (dos_strnicmp(pszCallSrc, "external", dos_strlen("external")) == 0)
        {
            if (dos_strnicmp(pszCallDirection, "Inbound", dos_strlen("Inbound")) == 0)
            {
                pstLCB->stCall.ucPeerType = SC_LEG_PEER_INBOUND;

                if (g_stSysStat.ulIncomingCalls != U32_BUTT)
                {
                    g_stSysStat.ulIncomingCalls++;
                }
                else
                {
                    DOS_ASSERT(0);
                }
            }
            else
            {
                pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;

                g_ulOutgoingCallCnt++;

                if (g_stSysStat.ulOutgoingCalls != U32_BUTT)
                {
                    g_stSysStat.ulOutgoingCalls ++;
                }
                else
                {
                    DOS_ASSERT(0);
                }
            }
        }
        else
        {
            if (dos_strnicmp(pszCallDirection, "Inbound", dos_strlen("Inbound")) == 0)
            {
                pstLCB->stCall.ucPeerType = SC_LEG_PEER_INBOUND_INTERNAL;
            }
            else
            {
                pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
            }
        }
    }

    dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_setvar %s exec_after_bridge_app park \r\n", pszLegUUID);
    sc_esl_execute_cmd(szCMD, NULL, 0);

    pszGwName  = esl_event_get_header(pstEvent, "variable_sip_gateway_name");
    pszTrunkIP = esl_event_get_header(pstEvent, "Caller-Network-Addr");
    pszCalling = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    pszCallee  = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    pszANI     = esl_event_get_header(pstEvent, "Caller-ANI");
    pszLegUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszCalling) || DOS_ADDR_INVALID(pszCallee)
        || DOS_ADDR_INVALID(pszANI) || DOS_ADDR_INVALID(pszLegUUID))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_SU)
                              , "Create leg without number info. Leg No:%u. Calling: %s, Callee: %s, ANI: %s"
                              , pstLCB->ulCBNo
                              , pszCalling ? pszCalling : "NULL"
                              , pszCallee ? pszCallee : "NULL"
                              , pszANI ? pszANI : "NULL");

        goto proc_fail;
    }

    sc_lcb_hash_add(pszLegUUID, pstLCB);

    /* 将相关数据写入SCB中 */
    if (pstLCB->ulSCBNo == U32_BUTT)
    {
        dos_strncpy(pstLCB->stCall.stNumInfo.szOriginalCallee, pszCallee, SC_NUM_LENGTH);
        pstLCB->stCall.stNumInfo.szOriginalCallee[SC_NUM_LENGTH -1] = '\0';

        dos_strncpy(pstLCB->stCall.stNumInfo.szOriginalCalling, pszCalling, SC_NUM_LENGTH);
        pstLCB->stCall.stNumInfo.szOriginalCalling[SC_NUM_LENGTH -1] = '\0';
    }

    dos_strncpy(pstLCB->stCall.stNumInfo.szANI, pszANI, SC_NUM_LENGTH);
    pstLCB->stCall.stNumInfo.szANI[SC_NUM_LENGTH -1] = '\0';

    dos_strncpy(pstLCB->szUUID, pszLegUUID, SC_UUID_LENGTH);
    pstLCB->szUUID[SC_UUID_LENGTH -1] = '\0';

    stCallEvent.stMsgTag.ulMsgType = SC_EVT_CALL_SETUP;
    if (pstLCB->ulIndSCBNo != U32_BUTT && pstLCB->ulSCBNo == U32_BUTT)
    {
        stCallEvent.stMsgTag.ulSCBNo = pstLCB->ulIndSCBNo;
    }
    else
    {
        stCallEvent.stMsgTag.ulSCBNo = pstLCB->ulSCBNo;
    }
    stCallEvent.ulLegNo = pstLCB->ulCBNo;
    sc_send_event_call_create(&stCallEvent);

    pstLCB->stCall.ucStatus = SC_LEG_PROC;

    return DOS_SUCC;

proc_fail:
    if (DOS_ADDR_VALID(pstLCB))
    {
        if (SC_LEG_INIT == pstLCB->stCall.ucStatus)
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
        else
        {
            stErrReport.stMsgTag.ulMsgType = SC_ERR_CALL_FAIL;
            stErrReport.stMsgTag.ulSCBNo = pstLCB->ulSCBNo;
            stErrReport.ulCMD = SC_CMD_BUTT;
            stErrReport.ulSCBNo = pstLCB->ulSCBNo;
            stErrReport.ulLegNo = pstLCB->ulCBNo;
            sc_send_event_err_report(&stErrReport);
        }
    }

    return DOS_FAIL;
}

/**
 * 处理CHANNEL_ANSWER事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_answer(esl_event_t *pstEvent, SC_LEG_CB *pstLegCB)
{
    SC_MSG_EVT_ANSWER_ST  stSCEvent;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_leg(pstLegCB, "Call has been answered. Leg: %u, SCB: %u", pstLegCB->ulCBNo, pstLegCB->ulSCBNo);

    if (!pstLegCB->stCall.bEarlyMedia)
    {
        sc_trace_leg(pstLegCB, "Call has been answered, waiting park. Leg: %u, SCB: %u", pstLegCB->ulCBNo, pstLegCB->ulSCBNo);

        return DOS_SUCC;
    }

    if (pstLegCB->stCall.stTimeInfo.ulAnswerTime == 0)
    {
        pstLegCB->stCall.stTimeInfo.ulAnswerTime = time(NULL);
    }

    pstLegCB->stCall.ucStatus = SC_LEG_ACTIVE;

    stSCEvent.stMsgTag.ulMsgType = SC_EVT_CALL_AMSWERED;
    //stSCEvent.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
    if (pstLegCB->ulIndSCBNo != U32_BUTT && pstLegCB->ulSCBNo == U32_BUTT)
    {
        stSCEvent.stMsgTag.ulSCBNo = pstLegCB->ulIndSCBNo;
    }
    else
    {
        stSCEvent.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
    }
    stSCEvent.ulSCBNo = pstLegCB->ulSCBNo;
    stSCEvent.ulLegNo = pstLegCB->ulCBNo;

    sc_send_event_answer(&stSCEvent);

    if (SC_LEG_PEER_OUTBOUND == pstLegCB->stCall.ucPeerType)
    {
        g_ulOutgoingCallCnt = 0;
    }

    return DOS_SUCC;
}

/**
 * 处理CHANNEL_HANGUP_COMPLETE事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_hangup(esl_event_t *pstEvent, SC_LEG_CB *pstLegCB)
{
    S8             *pszTermCause = NULL;
    SC_MSG_EVT_HUNGUP_ST stSCEvent;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLegCB->stCall.stTimeInfo.ulByeTime = time(NULL);

    pszTermCause = esl_event_get_header(pstEvent, "variable_sip_term_status");
    if (DOS_ADDR_INVALID(pszTermCause)
        || dos_atoul(pszTermCause, &stSCEvent.ulErrCode) < 0)
    {
        stSCEvent.ulErrCode = CC_ERR_NORMAL_CLEAR;
    }

    /* hangup命令，如果同时存在 ulSCBNo 和 ulIndSCBNo，两个都需要发送，且先给 ulSCBNo 发送  */
    stSCEvent.stMsgTag.ulMsgType = SC_EVT_CALL_RERLEASE;
    stSCEvent.ulSCBNo = pstLegCB->ulSCBNo;
    stSCEvent.ulLegNo = pstLegCB->ulCBNo;
    if (pstLegCB->ulSCBNo != U32_BUTT)
    {
        stSCEvent.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
        sc_send_event_release(&stSCEvent);
    }

    if (pstLegCB->ulIndSCBNo != U32_BUTT)
    {
        stSCEvent.stMsgTag.ulSCBNo = pstLegCB->ulIndSCBNo;
        sc_send_event_release(&stSCEvent);
    }

    if (SC_LEG_PEER_INBOUND == pstLegCB->stCall.ucPeerType)
    {
        if (g_stSysStat.ulIncomingCalls != 0)
        {
            g_stSysStat.ulIncomingCalls--;
        }
        else
        {
            DOS_ASSERT(0);
        }
    }
    else if (SC_LEG_PEER_OUTBOUND == pstLegCB->stCall.ucPeerType)
    {
        if (CC_ERR_SIP_REQUEST_TERMINATED == stSCEvent.ulErrCode)
        {
            g_ulOutgoingCallCnt = 0;
        }

        if (g_stSysStat.ulOutgoingCalls != 0)
        {
            g_stSysStat.ulOutgoingCalls--;
        }
        else
        {
            DOS_ASSERT(0);
        }
    }

    return DOS_SUCC;
}

/**
 * 处理CHANNEL_PROGRESS 或者 CHANNEL_PROGRESS_MEDIA事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_progress(esl_event_t *pstEvent, SC_LEG_CB *pstLegCB)
{
    SC_MSG_EVT_RINGING_ST stEventRinging;

    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLegCB->stCall.ucStatus = SC_LEG_ALERTING;

    pstLegCB->stCall.stTimeInfo.ulRingTime = time(NULL);
    /* 暂时不用往业务层发什么消息 */
    stEventRinging.stMsgTag.ulMsgType = SC_EVT_CALL_RINGING;
    //stEventRinging.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
    if (pstLegCB->ulIndSCBNo != U32_BUTT && pstLegCB->ulSCBNo == U32_BUTT)
    {
        stEventRinging.stMsgTag.ulSCBNo = pstLegCB->ulIndSCBNo;
    }
    else
    {
        stEventRinging.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
    }
    stEventRinging.stMsgTag.usInterErr = 0;

    stEventRinging.ulLegNo = pstLegCB->ulCBNo;
    stEventRinging.ulSCBNo = pstLegCB->ulSCBNo;

    if (pstEvent->event_id == ESL_EVENT_CHANNEL_PROGRESS_MEDIA)
    {
        pstLegCB->stCall.bEarlyMedia = DOS_TRUE;
        stEventRinging.ulWithMedia = DOS_TRUE;
    }
    else
    {
        pstLegCB->stCall.bEarlyMedia = DOS_FALSE;
        stEventRinging.ulWithMedia = DOS_FALSE;
    }

    if (SC_LEG_PEER_OUTBOUND == pstLegCB->stCall.ucPeerType)
    {
        g_ulOutgoingCallCnt = 0;
    }

    sc_send_event_ringing(&stEventRinging);

    return DOS_SUCC;
}

/**
 * 处理CHANNEL_PARK事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_park(esl_event_t *pstEvent, SC_LEG_CB *pstLegCB)
{
    S8  *pszDisposition = NULL;
    SC_MSG_EVT_ANSWER_ST  stSCEvent;

    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU)
                , "Start exchange media. LEG:%u, SCB:%u, EndPoint: %s"
                , pstLegCB->ulCBNo, pstLegCB->ulSCBNo
                , esl_event_get_header(pstEvent, "variable_endpoint_disposition"));

    pszDisposition = esl_event_get_header(pstEvent, "variable_endpoint_disposition");
    if (DOS_ADDR_VALID(pszDisposition))
    {
        if (dos_strnicmp(pszDisposition, "EARLY MEDIA", dos_strlen("EARLY MEDIA")) == 0)
        {
            pstLegCB->stCall.bEarlyMedia = DOS_TRUE;

            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_SU), "Start exchange media for early media. LEG:%u, SCB:%u", pstLegCB->ulCBNo, pstLegCB->ulSCBNo);

            return DOS_SUCC;

        }
        else if (dos_strnicmp(pszDisposition, "DELAYED NEGOTIATION", dos_strlen("DELAYED NEGOTIATION")) == 0)
        {
            /* 有些PARK消息是不需要处理的 */
            return DOS_SUCC;
        }
    }

    if (pstLegCB->stCall.stTimeInfo.ulAnswerTime == 0)
    {
        pstLegCB->stCall.stTimeInfo.ulAnswerTime = time(NULL);
    }

    pstLegCB->stCall.ucStatus = SC_LEG_ACTIVE;

    stSCEvent.stMsgTag.ulMsgType = SC_EVT_CALL_AMSWERED;
    if (pstLegCB->ulIndSCBNo != U32_BUTT && pstLegCB->ulSCBNo == U32_BUTT)
    {
        stSCEvent.stMsgTag.ulSCBNo = pstLegCB->ulIndSCBNo;
    }
    else
    {
        stSCEvent.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
    }
    stSCEvent.ulSCBNo = pstLegCB->ulSCBNo;
    stSCEvent.ulLegNo = pstLegCB->ulCBNo;

    sc_send_event_answer(&stSCEvent);


    return DOS_SUCC;
}

/**
 * 处理CHANNEL_HOLD事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_hold(esl_event_t *pstEvent, SC_LEG_CB *pstLegCB)
{
    SC_MSG_EVT_HOLD_ST stHold;

    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLegCB->stHold.usStatus = SC_SU_HOLD_ACTIVE;

    if (pstLegCB->ulIndSCBNo != U32_BUTT && pstLegCB->ulSCBNo == U32_BUTT)
    {
        stHold.stMsgTag.ulSCBNo = pstLegCB->ulIndSCBNo;
    }
    else
    {
        stHold.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
    }
    stHold.stMsgTag.ulMsgType = SC_EVT_HOLD;
    stHold.stMsgTag.usInterErr = 0;
    stHold.stMsgTag.usMsgLen = 0;
    stHold.ulLegNo = pstLegCB->ulCBNo;
    stHold.ulSCBNo = pstLegCB->ulSCBNo;
    stHold.bIsHold = DOS_TRUE;

    sc_send_event_hold(&stHold);

    return DOS_SUCC;
}

/**
 * 处理CHANNEL_UNHOLD事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_unhold(esl_event_t *pstEvent, SC_LEG_CB *pstLegCB)
{
    SC_MSG_EVT_HOLD_ST stHold;

    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLegCB->stHold.usStatus = SC_SU_HOLD_INIT;
    pstLegCB->stHold.bValid = DOS_FALSE;

    if (pstLegCB->ulIndSCBNo != U32_BUTT && pstLegCB->ulSCBNo == U32_BUTT)
    {
        stHold.stMsgTag.ulSCBNo = pstLegCB->ulIndSCBNo;
    }
    else
    {
        stHold.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
    }
    stHold.stMsgTag.ulMsgType = SC_EVT_HOLD;
    stHold.stMsgTag.usInterErr = 0;
    stHold.stMsgTag.usMsgLen = 0;
    stHold.ulLegNo = pstLegCB->ulCBNo;
    stHold.ulSCBNo = pstLegCB->ulSCBNo;
    stHold.bIsHold = DOS_FALSE;

    sc_send_event_hold(&stHold);

    return DOS_SUCC;
}

/**
 * 处理CHANNEL_DTMF事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_dtmf(esl_event_t *pstEvent, SC_LEG_CB *pstLegCB)
{
    S8                 *pszDTMF = NULL;
    SC_MSG_EVT_DTMF_ST stDTMF;

    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pszDTMF = esl_event_get_header(pstEvent, "DTMF-Digit");
    if (DOS_ADDR_INVALID(pszDTMF) || '\0' == pszDTMF[0])
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 如果不是第一次拨号，需要处理位间 */
    if (0 != pstLegCB->stCall.stTimeInfo.ulDTMFLastTime)
    {
        if (time(NULL) - pstLegCB->stCall.stTimeInfo.ulDTMFLastTime > SC_MAX_DTMF_INTERVAL)
        {
            pstLegCB->stCall.stNumInfo.szDial[0] = '\0';
        }

        pstLegCB->stCall.stTimeInfo.ulDTMFLastTime = time(NULL);
    }
    else
    {
        pstLegCB->stCall.stTimeInfo.ulDTMFStartTime = time(NULL);
        pstLegCB->stCall.stTimeInfo.ulDTMFLastTime = pstLegCB->stCall.stTimeInfo.ulDTMFStartTime;
    }

    dos_strcat(pstLegCB->stCall.stNumInfo.szDial, pszDTMF);
    pstLegCB->stCall.stNumInfo.szDial[SC_NUM_LENGTH - 1] = '\0';

    if (pstLegCB->ulIndSCBNo != U32_BUTT && pstLegCB->ulSCBNo == U32_BUTT)
    {
        stDTMF.stMsgTag.ulSCBNo = pstLegCB->ulIndSCBNo;
    }
    else
    {
        stDTMF.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
    }
    stDTMF.stMsgTag.ulMsgType = SC_EVT_DTMF;
    stDTMF.stMsgTag.usInterErr = 0;
    stDTMF.stMsgTag.usMsgLen = 0;
    stDTMF.ulLegNo = pstLegCB->ulCBNo;
    stDTMF.ulSCBNo = pstLegCB->ulSCBNo;
    stDTMF.cDTMFVal = pszDTMF[0];

    sc_send_event_dtmf(&stDTMF);

    return DOS_SUCC;
}

/**
 * 处理CHANNEL_BACKGROUND_JOB事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_background_job(esl_event_t *pstEvent)
{
    S8  *pszJobUUID = NULL;
    S8  *pszCommand = NULL;
    S8  *pszBody    = NULL;
    S8  *pszArgv    = NULL;
    U32  ulLegCBNo  = 0;
    SC_LEG_CB * pstLCB = NULL;
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    BOOL bIsSucc    = DOS_FALSE;

    pszJobUUID = esl_event_get_header(pstEvent, "Job-UUID");
    pszCommand = esl_event_get_header(pstEvent, "Job-Command");
    pszBody    = esl_event_get_body(pstEvent);
    pszArgv    = esl_event_get_header(pstEvent, "Job-Command-Arg");
    if (DOS_ADDR_INVALID(pszJobUUID))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "BJ-JOB exec. Command: %s, Argv: %s", pszCommand, pszArgv);

    ulLegCBNo = sc_bgjob_hash_find(pszJobUUID);
    if (ulLegCBNo > SC_LEG_CB_SIZE)
    {
        sc_bgjob_hash_delete(pszJobUUID);
        return DOS_SUCC;
    }

    sc_bgjob_hash_delete(pszJobUUID);

    pstLCB = sc_lcb_get(ulLegCBNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        return DOS_SUCC;
    }

    if (dos_strnicmp(pszBody, "+OK", dos_strlen("+OK")) == 0)
    {
        bIsSucc = DOS_TRUE;
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "BJ-JOB exec succ. Command: %s, Argv: %s", pszCommand, pszArgv);
        //return DOS_SUCC;
    }

    if (bIsSucc)
    {
        if (dos_strnicmp(pszCommand, "uuid_bridge", dos_strlen("uuid_bridge")) == 0)
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_BRIDGE_SUCC;
        }
        else
        {
            return DOS_SUCC;
        }
    }
    else
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_SU), "BJ-JOB exec fail. Command: %s, Argv: %s, Reply: %s", pszCommand, pszArgv, pszBody);

        if (dos_strnicmp(pszCommand, "originate", dos_strlen("originate")) == 0)
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_CALL_FAIL;
        }
        else if (dos_strnicmp(pszCommand, "uuid_bridge", dos_strlen("uuid_bridge")) == 0)
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_BRIDGE_FAIL;
        }
        else if (dos_strnicmp(pszCommand, "uuid_break", dos_strlen("uuid_break")) == 0)
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_BREAK_FAIL;
        }
        else if (dos_strnicmp(pszCommand, "uuid_record", dos_strlen("uuid_record")) == 0)
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_RECORD_FAIL;
        }
        else
        {
            /* 处理别的一些错误，一旦遇到应该是需要挂断呼叫的，但是暂时不这么处理 */
            return DOS_SUCC;
        }
    }

    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    if (pstLCB->ulIndSCBNo != U32_BUTT && pstLCB->ulSCBNo == U32_BUTT)
    {
        stErrReport.stMsgTag.ulSCBNo = pstLCB->ulIndSCBNo;
    }
    else
    {
        stErrReport.stMsgTag.ulSCBNo = pstLCB->ulSCBNo;
    }
    stErrReport.ulCMD = SC_CMD_BUTT;
    stErrReport.ulSCBNo = pstLCB->ulSCBNo;
    stErrReport.ulLegNo = pstLCB->ulCBNo;
    return sc_send_event_err_report(&stErrReport);
}

/**
 * 处理CHANNEL_SHUTDOWN事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_shutdown(esl_event_t *pstEvent)
{
    return DOS_SUCC;
}

/**
 * 处理RECORD_START事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_record_start(esl_event_t *pstEvent, SC_LEG_CB *pstLegCB)
{
    S8                     *pszRecordFile;
    SC_MSG_EVT_RECORD_ST   stRecord;

    pszRecordFile = esl_event_get_header(pstEvent, "Record-File-Path");
    if (DOS_ADDR_INVALID(pszRecordFile))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLegCB->stCall.stTimeInfo.ulRecordStartTime = time(NULL);

    pstLegCB->stRecord.usStatus = SC_SU_RECORD_ACTIVE;

    if (pstLegCB->ulIndSCBNo != U32_BUTT && pstLegCB->ulSCBNo == U32_BUTT)
    {
        stRecord.stMsgTag.ulSCBNo = pstLegCB->ulIndSCBNo;
    }
    else
    {
        stRecord.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
    }
    stRecord.stMsgTag.ulMsgType = SC_EVT_RECORD_START;
    stRecord.stMsgTag.usInterErr = 0;
    stRecord.stMsgTag.usMsgLen = 0;
    stRecord.ulLegNo = pstLegCB->ulCBNo;
    stRecord.ulSCBNo = pstLegCB->ulSCBNo;

    sc_send_event_record(&stRecord);

    return DOS_FALSE;

}

/**
 * 处理RECORD_STOP事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_record_stop(esl_event_t *pstEvent, SC_LEG_CB *pstLegCB)
{
    S8                     *pszRecordFile;
    S8                     szRecordFullName[SC_RECORD_FILENAME_LENGTH];
    SC_MSG_EVT_RECORD_ST   stRecord;

    pszRecordFile = esl_event_get_header(pstEvent, "Record-File-Path");
    if (DOS_ADDR_INVALID(pszRecordFile))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLegCB->stCall.stTimeInfo.ulRecordStopTime = time(NULL);

    dos_snprintf(szRecordFullName, sizeof(szRecordFullName)
                    , "%s-in.%s"
                    , pszRecordFile
                    , esl_event_get_header(pstEvent, "Channel-Read-Codec-Name"));
    chown(szRecordFullName, SC_NOBODY_UID, SC_NOBODY_GID);
    chmod(szRecordFullName, S_IXOTH|S_IWOTH|S_IROTH|S_IRUSR|S_IWUSR|S_IXUSR);

    dos_snprintf(szRecordFullName, sizeof(szRecordFullName)
                    , "%s-out.%s"
                    , pszRecordFile
                    , esl_event_get_header(pstEvent, "Channel-Read-Codec-Name"));
    chown(szRecordFullName, SC_NOBODY_UID, SC_NOBODY_GID);
    chmod(szRecordFullName, S_IXOTH|S_IWOTH|S_IROTH|S_IRUSR|S_IWUSR|S_IXUSR);

    dos_printf("Process recording file %s", szRecordFullName);


    pstLegCB->stRecord.usStatus = SC_SU_RECORD_RELEASE;
    if (pstLegCB->ulIndSCBNo != U32_BUTT && pstLegCB->ulSCBNo == U32_BUTT)
    {
        stRecord.stMsgTag.ulSCBNo = pstLegCB->ulIndSCBNo;
    }
    else
    {
        stRecord.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
    }
    stRecord.stMsgTag.ulMsgType = SC_EVT_RECORD_END;
    stRecord.stMsgTag.usInterErr = 0;
    stRecord.stMsgTag.usMsgLen = 0;
    stRecord.ulLegNo = pstLegCB->ulCBNo;
    stRecord.ulSCBNo = pstLegCB->ulSCBNo;

    sc_send_event_record(&stRecord);

    return DOS_FALSE;
}

/**
 * 处理PLAYBACK_START事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_playback_start(esl_event_t *pstEvent, SC_LEG_CB *pstLegCB)
{
    SC_MSG_EVT_PLAYBACK_ST stPlayback;

    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (!pstLegCB->stPlayback.bValid)
    {
        sc_trace_leg(pstLegCB, "Playback start event without playback service.");
        return DOS_FAIL;
    }

    sc_trace_leg(pstLegCB, "processing playback start event. status: ", pstLegCB->stPlayback.usStatus);

    switch (pstLegCB->stPlayback.usStatus)
    {
        case SC_SU_PLAYBACK_INIT:
            break;

        case SC_SU_PLAYBACK_PROC:
            pstLegCB->stPlayback.usStatus = SC_SU_PLAYBACK_ACTIVE;

            if (pstLegCB->ulIndSCBNo != U32_BUTT && pstLegCB->ulSCBNo == U32_BUTT)
            {
                stPlayback.stMsgTag.ulSCBNo = pstLegCB->ulIndSCBNo;
            }
            else
            {
                stPlayback.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
            }
            stPlayback.stMsgTag.ulMsgType = SC_EVT_PLAYBACK_START;
            stPlayback.stMsgTag.usInterErr = 0;
            stPlayback.stMsgTag.usMsgLen = 0;
            stPlayback.ulLegNo = pstLegCB->ulCBNo;
            stPlayback.ulSCBNo = pstLegCB->ulSCBNo;

            sc_send_event_playback(&stPlayback);
            break;

        case SC_SU_PLAYBACK_ACTIVE:

            break;

        case SC_SU_PLAYBACK_RELEASE:
            break;
    }

    sc_trace_leg(pstLegCB, "processed playback start event. status: ", pstLegCB->stPlayback.usStatus);

    return DOS_SUCC;

}

/**
 * 处理PLAYBACK_STOP事件
 *
 * @param esl_event_t *pstEvent ESL事件
 * @param U32 ulLegID LEG控制块ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_esl_event_playback_stop(esl_event_t *pstEvent, SC_LEG_CB *pstLegCB)
{
    SC_MSG_EVT_PLAYBACK_ST stPlayback;

    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLegCB->stPlayback.ulCurretnIndex++;

    sc_trace_leg(pstLegCB, "processing playback stop event. status: %u (%u:%u)"
                        , pstLegCB->stPlayback.usStatus
                        , pstLegCB->stPlayback.ulCurretnIndex
                        , pstLegCB->stPlayback.ulTotal);

    switch (pstLegCB->stPlayback.usStatus)
    {
        case SC_SU_PLAYBACK_INIT:
            break;

        case SC_SU_PLAYBACK_PROC:
            break;

        case SC_SU_PLAYBACK_ACTIVE:
            if (pstLegCB->stPlayback.ulCurretnIndex < pstLegCB->stPlayback.ulTotal)
            {
                break;
            }

            /*  没有break，让他继续执行 */

        case SC_SU_PLAYBACK_RELEASE:
            if (pstLegCB->ulIndSCBNo != U32_BUTT && pstLegCB->ulSCBNo == U32_BUTT)
            {
                stPlayback.stMsgTag.ulSCBNo = pstLegCB->ulIndSCBNo;
            }
            else
            {
                stPlayback.stMsgTag.ulSCBNo = pstLegCB->ulSCBNo;
            }
            stPlayback.stMsgTag.ulMsgType = SC_EVT_PLAYBACK_END;
            stPlayback.stMsgTag.usInterErr = 0;
            stPlayback.stMsgTag.usMsgLen = 0;
            stPlayback.ulLegNo = pstLegCB->ulCBNo;
            stPlayback.ulSCBNo = pstLegCB->ulSCBNo;
            pstLegCB->stCall.stTimeInfo.ulIVREndTime = time(NULL);
            sc_send_event_playback(&stPlayback);

            sc_lcb_playback_init(&pstLegCB->stPlayback);
            break;
    }

    sc_trace_leg(pstLegCB, "processed playback stop event. status: %u", pstLegCB->stPlayback.usStatus);

    return DOS_SUCC;
}

/**
 * 处理业务控制层发送过来的发起呼叫消息
 *
 * @param SC_MSG_HEAD_ST *pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_cmd_make_call(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_CMD_CALL_ST   *pstCMDMakeCall  = NULL;
    S8                   szBGJOBUUID[64] = { 0 };
    S8                   szCodecList[512] = { 0 };
    S8                   szGateway[512]   = { 0 };
    S8                   szCallString[1024] = { 0 };
    U32                  ulLoop, ulLength;
    SC_LEG_CB            *pstLegCB = NULL;
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;

    pstCMDMakeCall = (SC_MSG_CMD_CALL_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstCMDMakeCall))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "Processing call command. SCB: %u, LCB: %u", pstCMDMakeCall->ulSCBNo, pstCMDMakeCall->ulLCBNo);

    pstLegCB = sc_lcb_get(pstCMDMakeCall->ulLCBNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_SU), "Alloc lcb fail.");

        stErrReport.stMsgTag.usInterErr = SC_ERR_ALLOC_RES_FAIL;
        goto proc_fail;
    }

    if (pstLegCB->stCall.ulCodecCnt > 0)
    {
        ulLength = 0;

        ulLength += dos_snprintf(szCodecList + ulLength, sizeof(szCodecList) - ulLength, "absolute_codec_string=^^:");

        for (ulLoop=0; ulLoop<pstLegCB->stCall.ulCodecCnt; ulLoop++)
        {
            if (pstLegCB->stCall.aucCodecList[ulLoop] == U8_BUTT)
            {
                break;
            }

            switch (pstLegCB->stCall.aucCodecList[ulLoop])
            {
                case PT_PCMU:
                    ulLength += dos_snprintf(szCodecList + ulLength, sizeof(szCodecList) - ulLength, "PCMU@8000h@20i@8000b:");
                    break;
                case PT_G723:
                    ulLength += dos_snprintf(szCodecList + ulLength, sizeof(szCodecList) - ulLength, "G723@8000h@30i@6300b:");
                    break;
                case PT_PCMA:
                    ulLength += dos_snprintf(szCodecList + ulLength, sizeof(szCodecList) - ulLength, "PCMA@8000h@20i@8000b:");
                    break;
                case PT_G729:
                    ulLength += dos_snprintf(szCodecList + ulLength, sizeof(szCodecList) - ulLength, "G729@8000h@20i@8000b:");
                    break;
                default:
                    break;
            }

            /* 所有编解码描述都是4个字节 */
            if (sizeof(szCodecList) - ulLength < 4)
            {
                break;
            }
        }

        if (ulLength <= dos_strlen("absolute_codec_string=^^:"))
        {
            /* 没有编解码，直接将变量没了 */
            szCodecList[0] = '\0';
        }
        else
        {
            /* 将最后一个 ":" 替换为 "," */
            szCodecList[dos_strlen(szCodecList) - 1] = ',';
        }
    }

    if (SC_LEG_PEER_OUTBOUND_INTERNAL == pstLegCB->stCall.ucPeerType)
    {
        dos_snprintf(szCallString, sizeof(szCallString)
                        , "bgapi originate {%slcb_number=%u,origination_caller_id_number=%s," \
                          "origination_caller_id_name=%s,exec_after_bridge_app=park}user/%s &park \r\n"
                        , szCodecList
                        , pstLegCB->ulCBNo
                        , pstLegCB->stCall.stNumInfo.szCalling
                        , pstLegCB->stCall.stNumInfo.szCalling
                        , pstLegCB->stCall.stNumInfo.szCallee);
    }
    else if (SC_LEG_PEER_OUTBOUND_TT == pstLegCB->stCall.ucPeerType)
    {
        dos_snprintf(szCallString, sizeof(szCallString)
                    , "bgapi originate {%slcb_number=%u,origination_caller_id_number=%s," \
                      "exec_after_bridge_app=park,origination_caller_id_name=%s,sip_multipart" \
                      "=^^!application/x-allywll:m:=2!calli:=818!l:=01057063943!usert:=0!" \
                      "callt:=4!eig:=370!he:=5!w:=0!,sip_h_EixTTcall=TRUE,sip_h_Mime-version=1.0}" \
                      "sofia/external/%s@%s &park \r\n"
                    , szCodecList
                    , pstLegCB->ulCBNo
                    , pstLegCB->stCall.stNumInfo.szCalling
                    , pstLegCB->stCall.stNumInfo.szCalling
                    , pstLegCB->stCall.stNumInfo.szCallee
                    , pstLegCB->stCall.szEIXAddr);
    }
    else
    {
        if (pstLegCB->stCall.ulTrunkCnt <= 0)
        {
            /** 上报错误 */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_SU), "There is no trunk .");
            stErrReport.stMsgTag.usInterErr = SC_ERR_INVALID_MSG;
            goto proc_fail;
        }

        for (ulLoop=0,ulLength=0; ulLoop<pstLegCB->stCall.ulTrunkCnt; ulLoop++)
        {
            ulLength += dos_snprintf(szGateway + ulLength
                                        , sizeof(szGateway) - ulLength
                                        , "sofia/gateway/%u/%s|"
                                        , pstLegCB->stCall.aulTrunkList[ulLoop]
                                        , pstLegCB->stCall.stNumInfo.szCallee);
        }

        if (ulLength <= 0)
        {
            /** 上报错误 */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_SU), "There is no valid trunk.");
            stErrReport.stMsgTag.usInterErr = SC_ERR_INVALID_MSG;
            goto proc_fail;
        }

        szGateway[dos_strlen(szGateway) - 1] = '\0';

        dos_snprintf(szCallString, sizeof(szCallString)
                        , "bgapi originate {%sexec_after_bridge_app=park,lcb_number=%u," \
                          "origination_caller_id_number=%s,origination_caller_id_name=%s}%s &park \r\n"
                        , szCodecList
                        , pstLegCB->ulCBNo
                        , pstLegCB->stCall.stNumInfo.szCalling
                        , pstLegCB->stCall.stNumInfo.szCalling
                        , szGateway);
    }

    if (sc_esl_execute_cmd(szCallString, szBGJOBUUID, sizeof(szBGJOBUUID)) != DOS_SUCC)
    {
        /** 上报错误 */
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_SU), "Exec esl cmd fail");
        stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
        goto proc_fail;
    }

    sc_bgjob_hash_add(pstLegCB->ulCBNo, szBGJOBUUID);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "send call succ.");

    return DOS_SUCC;

proc_fail:
    if (pstLegCB)
    {
        sc_lcb_free(pstLegCB);
        pstLegCB = NULL;
    }

    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    stErrReport.stMsgTag.ulSCBNo = pstCMDMakeCall->ulSCBNo;
    stErrReport.ulSCBNo = pstCMDMakeCall->ulSCBNo;
    stErrReport.ulCMD = pstCMDMakeCall->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;
}

U32 sc_cmd_ringback(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    SC_MSG_CMD_RINGBACK_ST *pstAnswer = NULL;
    SC_LEG_CB              *pstLeg    = NULL;
    S8                     *pszArgv   = NULL;
    S8                     szUUID[SC_UUID_LENGTH];

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstAnswer = (SC_MSG_CMD_RINGBACK_ST *)pstMsg;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "request answer leg %u", pstAnswer->ulLegNo);

    pstLeg = sc_lcb_get(pstAnswer->ulLegNo);
    if (DOS_ADDR_INVALID(pstLeg))
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "Get scb fail. %u", pstAnswer->ulLegNo);
        goto proc_fail;
    }

    if (pstAnswer->ulMediaConnected)
    {
        pszArgv = sc_hine_get_tone(SC_TONE_RINGBACK);
        if (DOS_ADDR_INVALID(pszArgv))
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
            goto proc_fail;
        }

        if (sc_esl_execute("playback", pszArgv, pstLeg->szUUID) != DOS_SUCC)
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "Play ringback tone. %u.", pstAnswer->ulLegNo);
            goto proc_fail;
        }
    }
    else
    {
        if (sc_esl_execute("ring_ready", "", pstLeg->szUUID) != DOS_SUCC)
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "Play ringback tone. %u", pstAnswer->ulLegNo);
            goto proc_fail;
        }
    }

    sc_bgjob_hash_add(pstAnswer->ulLegNo, szUUID);

    return DOS_SUCC;

proc_fail:
    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    stErrReport.ulSCBNo = pstAnswer->ulSCBNo;
    stErrReport.ulCMD = pstAnswer->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;
}

U32 sc_cmd_answer_call(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    SC_MSG_CMD_ANSWER_ST *pstAnswer = NULL;
    SC_LEG_CB            *pstLeg    = NULL;
    S8                   szCMD[256];
    S8                   szUUID[SC_UUID_LENGTH];

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstAnswer = (SC_MSG_CMD_ANSWER_ST *)pstMsg;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "request answer leg %u", pstAnswer->ulLegNo);

    pstLeg = sc_lcb_get(pstAnswer->ulLegNo);
    if (DOS_ADDR_INVALID(pstLeg))
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "Get scb fail. %u", pstAnswer->ulLegNo);
        goto proc_fail;
    }

    dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_answer %s \r\n", pstLeg->szUUID);

    if (sc_esl_execute_cmd(szCMD, szUUID, sizeof(szUUID)) != DOS_SUCC)
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "Get scb fail. %u", pstAnswer->ulLegNo);
        goto proc_fail;
    }

    sc_bgjob_hash_add(pstAnswer->ulLegNo, szUUID);

    return DOS_SUCC;

proc_fail:
    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    stErrReport.ulSCBNo = pstAnswer->ulSCBNo;
    stErrReport.ulCMD = pstAnswer->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;
}


/**
 * 处理业务控制层发送过来的挂断呼叫的消息
 *
 * @param SC_MSG_TAG_ST *pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_cmd_hungup_call(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    SC_MSG_CMD_HUNGUP_ST   *pstHuangup = NULL;
    SC_LEG_CB              *pstLCB     = NULL;
    S8                     szCMD[256];

    pstHuangup = (SC_MSG_CMD_HUNGUP_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHuangup))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstHuangup->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
        goto proc_fail;
    }

    if (pstHuangup->ulErrCode != 0)
    {
        dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_setvar %u proto_specific_hangup_cause sip:%u", pstHuangup->ulErrCode);
        sc_esl_execute_cmd(szCMD, NULL, 0);
    }

    pstLCB->stCall.ucStatus = SC_LEG_RELEASE;

    sc_esl_execute("hangup", NULL, pstLCB->szUUID);

    return DOS_SUCC;

proc_fail:
    stErrReport.stMsgTag.ulSCBNo = SC_EVT_ERROR_PORT;
    stErrReport.ulSCBNo = pstHuangup->ulSCBNo;
    stErrReport.ulCMD = pstHuangup->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;

}

/**
 * 处理业务控制层发送过来的桥接呼叫的消息
 *
 * @param SC_MSG_TAG_ST *pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_cmd_bridge_call(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    SC_MSG_CMD_BRIDGE_ST *pstBridge = NULL;
    SC_LEG_CB            *pstCallingLeg = NULL;
    SC_LEG_CB            *pstCalleeLeg = NULL;
    S8                   szCMD[256];
    S8                   szBGJOBUUID[64] = { 0 };

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstBridge = (SC_MSG_CMD_BRIDGE_ST *)pstMsg;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "request bridge leg %u<-->%u", pstBridge->ulCallingLegNo, pstBridge->ulCalleeLegNo);

    pstCallingLeg = sc_lcb_get(pstBridge->ulCallingLegNo);
    pstCalleeLeg = sc_lcb_get(pstBridge->ulCalleeLegNo);
    if (DOS_ADDR_INVALID(pstCallingLeg) || DOS_ADDR_INVALID(pstCalleeLeg))
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_SU), "request bridge leg %u<-->%u. Leg not exist", pstBridge->ulCallingLegNo, pstBridge->ulCalleeLegNo);
        goto proc_fail;
    }

    dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_bridge %s %s \r\n", pstCallingLeg->szUUID, pstCalleeLeg->szUUID);
    if (sc_esl_execute_cmd(szCMD, szBGJOBUUID, sizeof(szBGJOBUUID)) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_SU), "request bridge leg %u<-->%u. exec esl cmd fail.", pstBridge->ulCallingLegNo, pstBridge->ulCalleeLegNo);

        stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
        goto proc_fail;
    }

    sc_bgjob_hash_add(pstCallingLeg->ulCBNo, szBGJOBUUID);

    pstCalleeLeg->stBridge.bValid = DOS_TRUE;
    pstCalleeLeg->stBridge.bValid = SC_SU_BRIDGE_INIT;
    pstCalleeLeg->stBridge.ulOtherLEGNo = pstCallingLeg->ulCBNo;
    pstCallingLeg->stBridge.bValid = DOS_TRUE;
    pstCallingLeg->stBridge.bValid = SC_SU_BRIDGE_INIT;
    pstCallingLeg->stBridge.ulOtherLEGNo = pstCalleeLeg->ulCBNo;


    return DOS_SUCC;

proc_fail:
    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    stErrReport.ulSCBNo = pstBridge->ulSCBNo;
    stErrReport.ulCMD = pstBridge->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;
}

/**
 * 处理业务控制层发送过来的发言请求
 *
 * @param SC_MSG_TAG_ST *pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_cmd_playback(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    SC_MSG_CMD_PLAYBACK_ST *pstPlayback = NULL;
    SC_LEG_CB              *pstLCB      = NULL;
    S8                     *pszPlayCMDArg  = NULL;
    S8                     szCMD[128] = { 0, };
    U32                    ulTotalCnt   = 0;
    U32                    ulLen;
    BOOL                   bIsAllocPlayArg = DOS_FALSE;         /* 是否申请了 pszPlayCMDArg */

    pstPlayback = (SC_MSG_CMD_PLAYBACK_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstPlayback))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstPlayback->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
        goto proc_fail;
    }

    if (SC_CND_PLAYBACK_FILE == pstPlayback->enType
        && pstPlayback->blNeedDTMF)
    {
        pszPlayCMDArg = (S8 *)dos_dmem_alloc(SC_MAX_FILELIST_LEN);
        if (DOS_ADDR_INVALID(pszPlayCMDArg))
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_ALLOC_RES_FAIL;
            goto proc_fail;
        }

        pstLCB->stPlayback.bValid = DOS_TRUE;
        pstLCB->stPlayback.usStatus = SC_SU_PLAYBACK_PROC;
        bIsAllocPlayArg = DOS_TRUE;
        ulLen = dos_snprintf(pszPlayCMDArg, SC_MAX_FILELIST_LEN, "1 1 %u 0 # %s pdtmf \\d+"
                                , pstPlayback->ulLoopCnt, pstPlayback->szAudioFile);
        if (sc_esl_execute("play_and_get_digits", pszPlayCMDArg, pstLCB->szUUID) != DOS_SUCC)
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
            goto proc_fail;
        }

        pstLCB->stPlayback.ulTotal += pstPlayback->ulLoopCnt;

        dos_dmem_free(pszPlayCMDArg);
        pszPlayCMDArg = NULL;

        return DOS_SUCC;
    }

    if (SC_CND_PLAYBACK_TONE == pstPlayback->enType)
    {
        pszPlayCMDArg = sc_hine_get_tone(pstPlayback->aulAudioList[0]);
        if (DOS_ADDR_INVALID(pszPlayCMDArg))
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_INVALID_MSG;
            goto proc_fail;
        }

        if (sc_esl_execute("playback", pszPlayCMDArg, pstLCB->szUUID) != DOS_SUCC)
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
            goto proc_fail;
        }

        pstLCB->stPlayback.ulTotal += pstPlayback->ulLoopCnt;

        return DOS_SUCC;
    }

    if (SC_CND_PLAYBACK_SYSTEM == pstPlayback->enType)
    {
        /* 获取文件列表再说 */
        pszPlayCMDArg = (S8 *)dos_dmem_alloc(SC_MAX_FILELIST_LEN);
        if (DOS_ADDR_INVALID(pszPlayCMDArg))
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_ALLOC_RES_FAIL;
            goto proc_fail;
        }

        bIsAllocPlayArg = DOS_TRUE;

        if (pstPlayback->ulLoopCnt == 1)
        {
            ulLen = dos_snprintf(pszPlayCMDArg, SC_MAX_FILELIST_LEN, "file_string://");
        }
        else
        {
            ulLen = dos_snprintf(pszPlayCMDArg, SC_MAX_FILELIST_LEN, "+%u file_string://", pstPlayback->ulLoopCnt);
        }

        ulTotalCnt = sc_get_snd_list(pstPlayback->aulAudioList, pstPlayback->ulTotalAudioCnt
                                        , pszPlayCMDArg + ulLen, SC_MAX_FILELIST_LEN - ulLen, NULL);
        if (0 == ulTotalCnt)
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_INVALID_MSG;
            goto proc_fail;
        }
    }
    else
    {
        pszPlayCMDArg = (S8 *)dos_dmem_alloc(SC_MAX_FILELIST_LEN);
        if (DOS_ADDR_INVALID(pszPlayCMDArg))
        {
            stErrReport.stMsgTag.usInterErr = SC_ERR_ALLOC_RES_FAIL;
            goto proc_fail;
        }

        bIsAllocPlayArg = DOS_TRUE;
        /* file_string://  添加这个之后，群呼任务，放语音文件，提示找不到文件 */
        if (pstPlayback->ulLoopCnt == 1)
        {
            ulLen = dos_snprintf(pszPlayCMDArg, SC_MAX_FILELIST_LEN, "file_string://%s", pstPlayback->szAudioFile);
        }
        else
        {
            ulLen = dos_snprintf(pszPlayCMDArg, SC_MAX_FILELIST_LEN, "+%u file_string://%s", pstPlayback->ulLoopCnt, pstPlayback->szAudioFile);
        }
    }

    dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_setvar %s playback_terminators none \r\n", pstLCB->szUUID);
    if (sc_esl_execute_cmd(szCMD, NULL, 0) == DOS_SUCC)
    {
        //pstLCB->stPlayback.ulTotal++;
    }

    /* 根据状态处理 */
    switch (pstLCB->stPlayback.usStatus)
    {
        case SC_SU_PLAYBACK_INIT:
            if (0 == pstPlayback->ulSilence)
            {
                pstPlayback->ulSilence = 200;
            }

            pstLCB->stPlayback.bValid = DOS_TRUE;
            pstLCB->stPlayback.usStatus = SC_SU_PLAYBACK_PROC;
            pstLCB->stPlayback.ulCurretnIndex = 0;
            pstLCB->stPlayback.ulTotal = 0;

            dos_snprintf(szCMD, sizeof(szCMD), "silence_stream://%u", pstPlayback->ulSilence);
            if (sc_esl_execute("playback", szCMD, pstLCB->szUUID) == DOS_SUCC)
            {
                pstLCB->stPlayback.ulTotal++;
            }

            if (pstPlayback->ulLoopCnt == 1)
            {
                if (sc_esl_execute("playback", pszPlayCMDArg, pstLCB->szUUID) == DOS_SUCC)
                {
                    pstLCB->stPlayback.ulTotal += pstPlayback->ulLoopCnt;
                }
            }
            else
            {
                if (sc_esl_execute("loop_playback", pszPlayCMDArg, pstLCB->szUUID) == DOS_SUCC)
                {
                    pstLCB->stPlayback.ulTotal += pstPlayback->ulLoopCnt;
                }
            }
            break;

        case SC_SU_PLAYBACK_PROC:
        case SC_SU_PLAYBACK_ACTIVE:
            if (sc_esl_execute("playback", pszPlayCMDArg, pstLCB->szUUID) == DOS_SUCC)
            {
                pstLCB->stPlayback.ulTotal += pstPlayback->ulLoopCnt;
            }
            break;

        case SC_SU_PLAYBACK_RELEASE:
            /* 被手动停止了，这个地方说明，正在等待上一次最后一个playback stop事件 */
            if (sc_esl_execute("playback", pszPlayCMDArg, pstLCB->szUUID) == DOS_SUCC)
            {
                pstLCB->stPlayback.ulTotal += pstPlayback->ulLoopCnt;

                /* 为了上次放音最后一个playback stop消息 */
                //pstLCB->stPlayback.ulTotal++;
            }

            pstLCB->stPlayback.usStatus = SC_SU_PLAYBACK_ACTIVE;
            break;
    }

    if (0 == pstLCB->stPlayback.ulTotal)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_SU), "there is no voice hint to play.");

        sc_lcb_playback_init(&pstLCB->stPlayback);

        goto proc_fail;
    }

    if (pszPlayCMDArg)
    {
        dos_dmem_free(pszPlayCMDArg);
        pszPlayCMDArg = NULL;
    }

    return DOS_SUCC;

proc_fail:
    if (pszPlayCMDArg && bIsAllocPlayArg)
    {
        dos_dmem_free(pszPlayCMDArg);
        pszPlayCMDArg = NULL;
    }

    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    stErrReport.stMsgTag.ulSCBNo = pstPlayback->ulSCBNo;
    stErrReport.ulSCBNo = pstPlayback->ulSCBNo;
    stErrReport.ulCMD = pstPlayback->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;
}

/**
 * 处理业务控制层发送过来的放音停止请求
 *
 * @param SC_MSG_HEAD_ST *pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_cmd_playback_stop(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    SC_MSG_CMD_PLAYBACK_ST *pstPlayback = NULL;
    SC_LEG_CB              *pstLCB      = NULL;
    S8                     szCMD[256];
    S8                     szUUID[SC_UUID_LENGTH];

    pstPlayback = (SC_MSG_CMD_PLAYBACK_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstPlayback))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstPlayback->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
        goto proc_fail;
    }

    dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_break %s all \r\n", pstLCB->szUUID);
    if (sc_esl_execute_cmd(szCMD, szUUID, sizeof(szUUID)) != DOS_SUCC)
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
        goto proc_fail;
    }

    sc_bgjob_hash_add(pstLCB->ulCBNo, szUUID);

    pstLCB->stPlayback.bValid = DOS_TRUE;
    pstLCB->stPlayback.usStatus = SC_SU_PLAYBACK_RELEASE;

    return DOS_SUCC;

proc_fail:
    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    stErrReport.stMsgTag.ulSCBNo = pstPlayback->ulSCBNo;
    stErrReport.ulSCBNo = pstPlayback->ulSCBNo;
    stErrReport.ulCMD = pstPlayback->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;
}

/**
 * 处理业务控制层发送过来的录音请求
 *
 * @param SC_MSG_HEAD_ST *pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_cmd_record(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    SC_MSG_CMD_RECORD_ST   *pstRecord = NULL;
    SC_LEG_CB              *pstLCB      = NULL;
    S8                     szCMD[256];
    S8                     szUUID[SC_UUID_LENGTH];

    pstRecord = (SC_MSG_CMD_RECORD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstRecord))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstRecord->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
        goto proc_fail;
    }

    if ('\0' == pstRecord->szRecordFile[0])
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_INVALID_MSG;
        goto proc_fail;
    }

    dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_record %s start %s/%s \r\n", pstLCB->szUUID, SC_RECORD_FILE_PATH, pstRecord->szRecordFile);
    if (sc_esl_execute_cmd(szCMD, szUUID, sizeof(szUUID)) != DOS_SUCC)
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
        goto proc_fail;
    }

    sc_lcb_hash_add(szUUID, pstLCB);

    pstLCB->stRecord.bValid = DOS_TRUE;
    pstLCB->stRecord.usStatus = SC_SU_RECORD_PROC;
    dos_snprintf(pstLCB->stRecord.szRecordFilename
                    , sizeof(pstLCB->stRecord.szRecordFilename)
                    , pstRecord->szRecordFile);

    return DOS_SUCC;

proc_fail:
    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    stErrReport.stMsgTag.ulSCBNo = pstRecord->ulSCBNo;
    stErrReport.ulSCBNo = pstRecord->ulSCBNo;
    stErrReport.ulCMD = pstRecord->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;
}

/**
 * 处理业务控制层发送过来的录音停止请求
 *
 * @param SC_MSG_HEAD_ST *pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_cmd_record_stop(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    SC_MSG_CMD_RECORD_ST   *pstRecord = NULL;
    SC_LEG_CB              *pstLCB      = NULL;
    S8                     szCMD[256];
    S8                     szUUID[SC_UUID_LENGTH];

    pstRecord = (SC_MSG_CMD_RECORD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstRecord))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstRecord->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
        goto proc_fail;
    }

    if ('\0' == pstRecord->szRecordFile[0])
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_INVALID_MSG;
        goto proc_fail;
    }

    dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_record %s stop %s \r\n", pstLCB->szUUID, pstRecord->szRecordFile);
    if (sc_esl_execute_cmd(szCMD, szUUID, sizeof(szUUID)) != DOS_SUCC)
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
        goto proc_fail;
    }

    sc_lcb_hash_add(szUUID, pstLCB);

    pstLCB->stRecord.bValid = DOS_TRUE;
    pstLCB->stRecord.usStatus = SC_SU_RECORD_RELEASE;
    dos_snprintf(pstLCB->stRecord.szRecordFilename
                    , sizeof(pstLCB->stRecord.szRecordFilename)
                    , pstRecord->szRecordFile);

    return DOS_SUCC;

proc_fail:
    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    stErrReport.stMsgTag.ulSCBNo = pstRecord->ulSCBNo;
    stErrReport.ulSCBNo = pstRecord->ulSCBNo;
    stErrReport.ulCMD = pstRecord->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;
}

/**
 * 处理业务控制层发送过来的呼叫保持请求
 *
 * @param SC_MSG_HEAD_ST *pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_cmd_hold(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    SC_MSG_CMD_HOLD_ST     *pstHold = NULL;
    SC_LEG_CB              *pstLCB      = NULL;
    S8                     szCMD[256];
    S8                     szUUID[SC_UUID_LENGTH];

    pstHold = (SC_MSG_CMD_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstHold->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
        goto proc_fail;
    }

    dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_hold %s \r\n", pstLCB->szUUID);
    if (sc_esl_execute_cmd(szCMD, szUUID, sizeof(szUUID)) != DOS_SUCC)
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
        goto proc_fail;
    }

    sc_lcb_hash_add(szUUID, pstLCB);

    pstLCB->stHold.bValid = DOS_TRUE;
    pstLCB->stHold.usStatus = SC_SU_HOLD_PROC;

    return DOS_SUCC;

proc_fail:
    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    stErrReport.stMsgTag.ulSCBNo = pstLCB->ulSCBNo;
    stErrReport.ulSCBNo = pstLCB->ulSCBNo;
    stErrReport.ulCMD = pstHold->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;
}

/**
 * 处理业务控制层发送过来的呼叫解除保持请求
 *
 * @param SC_MSG_HEAD_ST *pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_cmd_unhold(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    SC_MSG_CMD_HOLD_ST     *pstHold = NULL;
    SC_LEG_CB              *pstLCB      = NULL;
    S8                     szCMD[256];
    S8                     szUUID[SC_UUID_LENGTH];

    pstHold = (SC_MSG_CMD_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstHold->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
        goto proc_fail;
    }

    dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_hold off %s \r\n", pstLCB->szUUID);
    if (sc_esl_execute_cmd(szCMD, szUUID, sizeof(szUUID)) != DOS_SUCC)
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
        goto proc_fail;
    }

    sc_lcb_hash_add(szUUID, pstLCB);

    pstLCB->stHold.bValid = DOS_TRUE;
    pstLCB->stHold.usStatus = SC_SU_HOLD_RELEASE;

    return DOS_SUCC;

proc_fail:
    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    stErrReport.stMsgTag.ulSCBNo = pstLCB->ulSCBNo;
    stErrReport.ulSCBNo = pstLCB->ulSCBNo;
    stErrReport.ulCMD = pstHold->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;

}

/**
 * 处理业务控制层发送过来的呼叫IRV请求
 *
 * @param SC_MSG_HEAD_ST *pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_cmd_ivr(SC_MSG_TAG_ST *pstMsg)
{
    return DOS_SUCC;
}

U32 sc_cmd_mux(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    SC_MSG_CMD_MUX_ST          *pstMux = NULL;
    SC_LEG_CB                  *pstLCB          = NULL;
    SC_LEG_CB                  *pstAgentLCB     = NULL;

    pstMux = (SC_MSG_CMD_MUX_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstMux))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstMux->ulMode)
    {
        case SC_MUX_CMD_INTERCEPT:
            pstLCB = sc_lcb_get(pstMux->ulLegNo);
            if (DOS_ADDR_INVALID(pstLCB))
            {
                stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
                goto proc_fail;
            }

            pstAgentLCB = sc_lcb_get(pstMux->ulAgentLegNo);
            if (DOS_ADDR_INVALID(pstAgentLCB))
            {
                stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
                goto proc_fail;
            }

            if (sc_esl_execute("eavesdrop", pstAgentLCB->szUUID, pstLCB->szUUID) != DOS_SUCC)
            {
                stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
                goto proc_fail;
            }
            break;
        case SC_MUX_CMD_WHISPER:
            pstLCB = sc_lcb_get(pstMux->ulLegNo);
            if (DOS_ADDR_INVALID(pstLCB))
            {
                stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
                goto proc_fail;
            }

            pstAgentLCB = sc_lcb_get(pstMux->ulAgentLegNo);
            if (DOS_ADDR_INVALID(pstAgentLCB))
            {
                stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
                goto proc_fail;
            }

            if (sc_esl_execute("queue_dtmf", "w2@500", pstLCB->szUUID) != DOS_SUCC)
            {
                stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
                goto proc_fail;
            }

            if (sc_esl_execute("eavesdrop", pstAgentLCB->szUUID, pstLCB->szUUID) != DOS_SUCC)
            {
                stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
                goto proc_fail;
            }
            break;
        default:
            break;
    }

    return DOS_SUCC;

proc_fail:
    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    stErrReport.stMsgTag.ulSCBNo = pstMux->ulSCBNo;
    stErrReport.ulSCBNo = pstMux->ulSCBNo;
    stErrReport.ulCMD = pstMux->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;
}

U32 sc_cmd_transfer(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_ERR_REPORT_ST   stErrReport;
    SC_MSG_CMD_TRANSFER_ST     *pstTransfer     = NULL;
    SC_LEG_CB                  *pstLCB          = NULL;
    S8                          szCMD[256];
    S8                          szUUID[SC_UUID_LENGTH];

    pstTransfer = (SC_MSG_CMD_TRANSFER_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstTransfer))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstTransfer->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
        goto proc_fail;
    }

    if (pstLCB->szUUID[0] == '\0')
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_LEG_NOT_EXIST;
        goto proc_fail;
    }

    dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_transfer %s %s\r\n", pstLCB->szUUID, pstTransfer->szCalleeNum);
    if (sc_esl_execute_cmd(szCMD, szUUID, sizeof(szUUID)) != DOS_SUCC)
    {
        stErrReport.stMsgTag.usInterErr = SC_ERR_EXEC_FAIL;
        goto proc_fail;
    }

    sc_lcb_hash_add(szUUID, pstLCB);

    return DOS_SUCC;

proc_fail:
    stErrReport.stMsgTag.ulMsgType = SC_EVT_ERROR_PORT;
    stErrReport.stMsgTag.ulSCBNo = pstTransfer->ulSCBNo;
    stErrReport.ulSCBNo = pstTransfer->ulSCBNo;
    stErrReport.ulCMD = pstTransfer->stMsgTag.ulMsgType;

    sc_send_event_err_report(&stErrReport);

    return DOS_FAIL;
}

/**
 * 处理业务控制层发过来的请求命令，主要是分发
 *
 * @param SC_MSG_HEAD_ST *pstMsg 消息头
 *
 * @return VOID
 */
VOID sc_cmd_process(SC_MSG_TAG_ST *pstMsg)
{
    U32 ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        return;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "Processing cmd %s(%u), SCB: %u", sc_command_str(pstMsg->ulMsgType), pstMsg->ulMsgType, pstMsg->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_CMD_CALL:
            ulRet = sc_cmd_make_call(pstMsg);
            break;

        case SC_CMD_RINGBACK:
            ulRet = sc_cmd_ringback(pstMsg);
            break;

        case SC_CMD_ANSWER_CALL:
            ulRet = sc_cmd_answer_call(pstMsg);
            break;

        case SC_CMD_BRIDGE_CALL:
            ulRet = sc_cmd_bridge_call(pstMsg);
            break;

        case SC_CMD_RELEASE_CALL:
            ulRet = sc_cmd_hungup_call(pstMsg);
            break;

        case SC_CMD_PLAYBACK:
            ulRet = sc_cmd_playback(pstMsg);
            break;

        case SC_CMD_PLAYBACK_STOP:
            ulRet = sc_cmd_playback_stop(pstMsg);
            break;

        case SC_CMD_RECORD:
            ulRet = sc_cmd_record(pstMsg);
            break;

        case SC_CMD_RECORD_STOP:
            ulRet = sc_cmd_record_stop(pstMsg);
            break;

        case SC_CMD_HOLD:
            ulRet = sc_cmd_hold(pstMsg);
            break;

        case SC_CMD_UNHOLD:
            ulRet = sc_cmd_unhold(pstMsg);
            break;

        case SC_CMD_IVR_CTRL:
            ulRet = sc_cmd_ivr(pstMsg);
            break;

        case SC_CMD_MUX:
            ulRet = sc_cmd_mux(pstMsg);
            break;

        case SC_CMD_TRANSFER:
            ulRet = sc_cmd_transfer(pstMsg);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SU), "Invalid cmd type. %u", pstMsg->ulMsgType);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_SU), "Processed %s(%u), Ret:%s", sc_command_str(pstMsg->ulMsgType), pstMsg->ulMsgType, (DOS_SUCC == ulRet) ? "succ" : "FAIL");
}

/**
 * 维护业务控制层发过来的请求命令消息队列
 *
 * @return VOID
 */
VOID *sc_cmd_process_runtime(VOID *ptr)
{
    struct timespec     stTimeout;
    DLL_NODE_S    *pstDLLNode = NULL;

    while (1)
    {
        pthread_mutex_lock(&g_mutexCommandQueue);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condCommandQueue, &g_mutexCommandQueue, &stTimeout);
        pthread_mutex_unlock(&g_mutexCommandQueue);

        while(1)
        {
            if (DLL_Count(&g_stCommandQueue) == 0)
            {
                break;
            }

            pthread_mutex_lock(&g_mutexCommandQueue);
            pstDLLNode = dll_fetch(&g_stCommandQueue);
            pthread_mutex_unlock(&g_mutexCommandQueue);

            if (DOS_ADDR_INVALID(pstDLLNode))
            {
                break;
            }

            if (DOS_ADDR_INVALID(pstDLLNode->pHandle))
            {
                DOS_ASSERT(0);

                DLL_Init_Node(pstDLLNode);
                dos_dmem_free(pstDLLNode);
                pstDLLNode = NULL;

                continue;
            }

            sc_cmd_process((SC_MSG_TAG_ST *)pstDLLNode->pHandle);

            dos_dmem_free(pstDLLNode->pHandle);
            pstDLLNode->pHandle = NULL;
            DLL_Init_Node(pstDLLNode);
            dos_dmem_free(pstDLLNode);
            pstDLLNode = NULL;
        }
    }
}

/**
 * 业务子层初始化函数
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_su_mngt_init()
{
    U32  ulIndex;

    g_pstLegCB = (SC_LEG_CB *)dos_dmem_alloc(sizeof(SC_LEG_CB) * SC_LEG_CB_SIZE);
    if (DOS_ADDR_INVALID(g_pstLegCB))
    {
        sc_log(LOG_LEVEL_EMERG, "Alloc memory for leg cb fail.");
        return DOS_FAIL;
    }

    for (ulIndex=0; ulIndex<SC_LEG_CB_SIZE; ulIndex++)
    {
        g_pstLegCB[ulIndex].ulCBNo = ulIndex;
        sc_lcb_init(&g_pstLegCB[ulIndex]);
    }

    g_pstBGJobHash = hash_create_table(SC_BG_JOB_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstBGJobHash))
    {
        sc_log(LOG_LEVEL_EMERG, "Alloc memory for bgjob hash fail .");
        return DOS_FAIL;
    }

    g_pstLCBHash = hash_create_table(SC_UUID_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstLCBHash))
    {
        sc_log(LOG_LEVEL_EMERG, "Alloc memory for bgjob hash fail .");
        return DOS_FAIL;
    }

    DLL_Init(&g_stCommandQueue);

    return DOS_SUCC;
}

/**
 * 业务子层启动函数
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_su_mngt_start()
{
    if (pthread_create(&g_pthCommandThread, NULL, sc_cmd_process_runtime, NULL) < 0)
    {
        sc_log(LOG_LEVEL_EMERG, "Start event process thread fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 业务子层停止函数
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_su_mngt_stop()
{
    g_blCommandThreadisRunning = DOS_FALSE;

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */


