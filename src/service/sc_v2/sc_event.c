/**
 * @file : sc_event.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 业务控制层主要入口函数集合
 *
 * @date: 2016年1月9日
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_pub.h"
#include "sc_def.h"
#include "sc_debug.h"

/** 事件处理线程句柄 */
pthread_t       g_pthEventQueue;

/** 业务子层上报时间消息队列 */
DLL_S           g_stEventQueue;

/** 业务子层上报时间消息队列锁 */
pthread_mutex_t g_mutexEventQueue = PTHREAD_MUTEX_INITIALIZER;

/** 业务子层上报时间消息队列条件变量 */
pthread_cond_t  g_condEventQueue = PTHREAD_COND_INITIALIZER;

/** 系统是否处于维护状态, 必须有业务子层给上次发送startup消息，该标记将被置为FALSE，方可开展业务 */
BOOL            g_blInMaintain        = DOS_TRUE;

/** 业务控制块指针 */
SC_SRV_CB       *g_pstSCBList         = NULL;

/** 业务控制块锁 */
pthread_mutex_t g_mutexSCBList = PTHREAD_MUTEX_INITIALIZER;

/**
 * 基本呼叫业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_call_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in Call service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_AUTH_RESULT:
            ulRet = sc_call_auth_rsp(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_SETUP:
            ulRet = sc_call_setup(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_AMSWERED:
            ulRet = sc_call_answer(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RINGING:
            ulRet = sc_call_ringing(pstMsg, pstSCB);
            break;

        case SC_EVT_BRIDGE_START:
            ulRet = sc_call_bridge(pstMsg, pstSCB);
            break;

        case SC_EVT_BRIDGE_STOP:
            ulRet = sc_call_unbridge(pstMsg, pstSCB);
            break;

        case SC_EVT_HOLD:
            ulRet = sc_call_hold(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_call_release(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_STATUS:
            /* 暂时不处理 */
            break;

        case SC_EVT_DTMF:
            ulRet = sc_call_dtmf(pstMsg, pstSCB);
            break;

        case SC_EVT_RECORD_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_RECORD_END:
            ulRet = sc_call_record_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_PLAYBACK_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_END:
            ulRet = sc_call_playback_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_LEACE_CALL_QUEUE:
            ulRet = sc_call_queue_leave(pstMsg, pstSCB);
            break;

        case SC_EVT_RINGING_TIMEOUT:
            ulRet = sc_call_ringing_timeout(pstMsg, pstSCB);
            break;

        case SC_EVT_ERROR_PORT:
            ulRet = sc_call_error(pstMsg, pstSCB);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Invalid event type. %u", pstMsg->ulMsgType);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in Call service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

/**
 * 基本呼叫业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_preview_dial_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in Preview Call Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_AUTH_RESULT:
            ulRet = sc_preview_auth_rsp(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_SETUP:
            ulRet = sc_preview_setup(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_AMSWERED:
            ulRet = sc_preview_answer(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RINGING:
            ulRet = sc_preview_ringing(pstMsg, pstSCB);
            break;

        case SC_EVT_BRIDGE_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_BRIDGE_STOP:
            /* 暂时不处理 */
            break;

        case SC_EVT_HOLD:
            ulRet = sc_preview_hold(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_preview_release(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_STATUS:
            /* 暂时不处理 */
            break;

        case SC_EVT_DTMF:
            ulRet = sc_preview_dtmf(pstMsg, pstSCB);
            break;

        case SC_EVT_RECORD_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_RECORD_END:
            ulRet = sc_preview_record_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_PLAYBACK_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_END:
            ulRet = sc_preview_playback_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_ERROR_PORT:
            ulRet = sc_preview_error(pstMsg, pstSCB);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Invalid event type. %u", pstMsg->ulMsgType);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in Preview Call Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

/**
 * 基本呼叫业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_auto_dial_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in auto call Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_AUTH_RESULT:
            ulRet = sc_auto_call_auth_rsp(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_SETUP:
            ulRet = sc_auto_call_setup(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_AMSWERED:
            ulRet = sc_auto_call_answer(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RINGING:
            ulRet = sc_auto_call_ringing(pstMsg, pstSCB);
            break;

        case SC_EVT_BRIDGE_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_BRIDGE_STOP:
            /* 暂时不处理 */
            break;

        case SC_EVT_HOLD:
            ulRet = sc_auto_call_hold(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_auto_call_release(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_STATUS:
            /* 暂时不处理 */
            break;

        case SC_EVT_DTMF:
            ulRet = sc_auto_call_dtmf(pstMsg, pstSCB);
            break;

        case SC_EVT_RECORD_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_RECORD_END:
            /* 暂时不处理 */
            ulRet = sc_auto_call_record_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_PLAYBACK_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_END:
            ulRet = sc_auto_call_palayback_end(pstMsg, pstSCB);
            break;

        case SC_EVT_LEACE_CALL_QUEUE:
            ulRet = sc_auto_call_queue_leave(pstMsg, pstSCB);
            break;

        case SC_EVT_RINGING_TIMEOUT:
            ulRet = sc_auto_call_ringing_timeout(pstMsg, pstSCB);
            break;

        case SC_EVT_ERROR_PORT:
            ulRet = sc_auto_call_error(pstMsg, pstSCB);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Invalid event type. %u", pstMsg->ulMsgType);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in auto call Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

/**
 * 基本呼叫业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_voice_verify_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in Voice Verify Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_AUTH_RESULT:
            ulRet = sc_voice_verify_auth_rsp(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_SETUP:
            ulRet = sc_voice_verify_setup(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_AMSWERED:
            ulRet = sc_voice_verify_answer(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RINGING:
            ulRet = sc_voice_verify_ringing(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_voice_verify_release(pstMsg, pstSCB);
            break;

        case SC_EVT_PLAYBACK_END:
            ulRet = sc_voice_verify_playback_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_STATUS:
        case SC_EVT_DTMF:
        case SC_EVT_RECORD_START:
        case SC_EVT_RECORD_END:
        case SC_EVT_PLAYBACK_START:
        case SC_EVT_BRIDGE_START:
        case SC_EVT_BRIDGE_STOP:
        case SC_EVT_HOLD:
            ulRet = DOS_SUCC;
            break;
        case SC_EVT_ERROR_PORT:
            ulRet = sc_voice_verify_error(pstMsg, pstSCB);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Invalid event type. %u", pstMsg->ulMsgType);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in Voice Verify Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;

}

/**
 * 基本呼叫业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_access_code_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in Access Code Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);
    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_AUTH_RESULT:
            ulRet = sc_access_code_auth_rsp(pstMsg, pstSCB);
            break;

        case SC_EVT_DTMF:
            ulRet = sc_access_code_dtmf(pstMsg, pstSCB);
            break;

        case SC_EVT_PLAYBACK_END:
            ulRet = sc_access_code_playback_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_access_code_release(pstMsg, pstSCB);
            break;

        case SC_EVT_ERROR_PORT:
            ulRet = sc_access_code_error(pstMsg, pstSCB);
            break;

        default:
            break;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in Access Code Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

/**
 * 基本呼叫业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_hold_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in Hold Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

     switch (pstMsg->ulMsgType)
    {
        case SC_EVT_HOLD:
            ulRet = sc_hold_hold(pstMsg, pstSCB);
            break;
        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_hold_release(pstMsg, pstSCB);
            break;
        case SC_EVT_ERROR_PORT:
            ulRet = sc_hold_error(pstMsg, pstSCB);
            break;
        default:
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in Hold Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

/**
 * 基本呼叫业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_transfer_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in Transfer Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_AUTH_RESULT:
            ulRet = sc_transfer_auth_rsp(pstMsg, pstSCB);
            break;
        case SC_EVT_CALL_SETUP:
            ulRet = sc_transfer_setup(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_AMSWERED:
            ulRet = sc_transfer_answer(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RINGING:
            ulRet = sc_transfer_ringing(pstMsg, pstSCB);
            break;

        case SC_EVT_HOLD:
            ulRet = sc_transfer_hold(pstMsg, pstSCB);
            break;

        case SC_EVT_DTMF:
            ulRet = sc_transfer_dtmf(pstMsg, pstSCB);
            break;

        case SC_EVT_PLAYBACK_END:
            ulRet = sc_transfer_playback_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_RECORD_END:
            ulRet = sc_transfer_record_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_transfer_release(pstMsg, pstSCB);
            break;

        case SC_EVT_ERROR_PORT:
            ulRet = sc_transfer_error(pstMsg, pstSCB);
            break;

        default:
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in Transfer Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

/**
 * 基本呼叫业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_incoming_queue_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in Incoming Queue Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_LEACE_CALL_QUEUE:
            ulRet = sc_incoming_queue_leave(pstMsg, pstSCB);
            break;

        case SC_EVT_PLAYBACK_END:
            ulRet = sc_incoming_playback_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_incoming_queue_release(pstMsg, pstSCB);
            break;

        default:
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in Incoming Queue Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}


/**
 * 基本呼叫业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_interception_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in Interception Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_AUTH_RESULT:
            ulRet = sc_interception_auth_rsp(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_SETUP:
            ulRet = sc_interception_setup(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_AMSWERED:
            ulRet = sc_interception_answer(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RINGING:
            ulRet = sc_interception_ringing(pstMsg, pstSCB);
            break;

        case SC_EVT_BRIDGE_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_BRIDGE_STOP:
            /* 暂时不处理 */
            break;

        case SC_EVT_HOLD:
            /* 暂时不处理 */
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_interception_release(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_STATUS:
            /* 暂时不处理 */
            break;

        case SC_EVT_DTMF:
            /* 暂时不处理 */
            break;

        case SC_EVT_RECORD_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_RECORD_END:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_END:
            /* 暂时不处理 */
            break;

        case SC_EVT_ERROR_PORT:
            ulRet = sc_interception_error(pstMsg, pstSCB);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Invalid event type. %u", pstMsg->ulMsgType);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in Interception Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

U32 sc_srv_whisper_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in whisper Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_AUTH_RESULT:
            ulRet = sc_whisper_auth_rsp(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_SETUP:
            ulRet = sc_whisper_setup(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_AMSWERED:
            ulRet = sc_whisper_answer(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RINGING:
            ulRet = sc_whisper_ringing(pstMsg, pstSCB);
            break;

        case SC_EVT_BRIDGE_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_BRIDGE_STOP:
            /* 暂时不处理 */
            break;

        case SC_EVT_HOLD:
            /* 暂时不处理 */
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_whisper_release(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_STATUS:
            /* 暂时不处理 */
            break;

        case SC_EVT_DTMF:
            /* 暂时不处理 */
            break;

        case SC_EVT_RECORD_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_RECORD_END:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_END:
            /* 暂时不处理 */
            break;

        case SC_EVT_ERROR_PORT:
            ulRet = sc_whisper_error(pstMsg, pstSCB);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Invalid event type. %u", pstMsg->ulMsgType);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in Interception Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

/**
 * 基本呼叫业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_whisoered_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in Whisoered Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in Whisoered Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

/**
 * 基本呼叫业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_mark_custom_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in Customer Mark Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_CALL_SETUP:
        case SC_EVT_CALL_RINGING:
        case SC_EVT_CALL_AMSWERED:
        case SC_EVT_BRIDGE_START:
        case SC_EVT_HOLD:
        case SC_EVT_BRIDGE_STOP:
        case SC_EVT_CALL_STATUS:
        case SC_EVT_RECORD_START:
        case SC_EVT_RECORD_END:
            ulRet = DOS_SUCC;
            break;
        case SC_EVT_PLAYBACK_START:
            ulRet = sc_mark_custom_playback_start(pstMsg, pstSCB);
            break;
        case SC_EVT_PLAYBACK_END:
            ulRet = sc_mark_custom_playback_end(pstMsg, pstSCB);
            break;
        case SC_EVT_AUTH_RESULT:
        case SC_EVT_LEACE_CALL_QUEUE:
            ulRet = DOS_SUCC;
            break;
        case SC_EVT_DTMF:
            ulRet = sc_mark_custom_dtmf(pstMsg, pstSCB);
            break;
        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_mark_custom_release(pstMsg, pstSCB);
            break;
        case SC_EVT_ERROR_PORT:
            ulRet = sc_mark_custom_error(pstMsg, pstSCB);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Invalid event type. %u", pstMsg->ulMsgType);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in Customer Mark Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

/**
 * 坐席长签业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_sigin_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in sigin service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_AUTH_RESULT:
            ulRet = sc_sigin_auth_rsp(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_SETUP:
            ulRet = sc_sigin_setup(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_AMSWERED:
            ulRet = sc_sigin_answer(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RINGING:
            ulRet = sc_sigin_ringing(pstMsg, pstSCB);
            break;

        case SC_EVT_BRIDGE_START:
           /* 暂时不处理 */
            break;

        case SC_EVT_BRIDGE_STOP:
            /* 暂时不处理 */
            break;

        case SC_EVT_HOLD:
            /* 暂时不处理 */
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_sigin_release(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_STATUS:
            /* 暂时不处理 */
            break;

        case SC_EVT_DTMF:
            ulRet = sc_sigin_dtmf(pstMsg, pstSCB);
            break;

        case SC_EVT_RECORD_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_RECORD_END:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_END:
            ulRet = sc_sigin_playback_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_ERROR_PORT:
            ulRet = sc_sigin_error(pstMsg, pstSCB);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Invalid event type. %u", pstMsg->ulMsgType);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in sigin service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}

/**
 * 群呼任务demo业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_demo_task_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in auto call Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_AUTH_RESULT:
            ulRet = sc_demo_task_auth_rsp(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_SETUP:
            ulRet = sc_demo_task_setup(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_AMSWERED:
            ulRet = sc_demo_task_answer(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RINGING:
            ulRet = sc_demo_task_ringing(pstMsg, pstSCB);
            break;

        case SC_EVT_BRIDGE_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_BRIDGE_STOP:
            /* 暂时不处理 */
            break;

        case SC_EVT_HOLD:
            ulRet = sc_demo_task_hold(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_demo_task_release(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_STATUS:
            /* 暂时不处理 */
            break;

        case SC_EVT_DTMF:
            ulRet = sc_demo_task_dtmf(pstMsg, pstSCB);
            break;

        case SC_EVT_RECORD_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_RECORD_END:
            /* 暂时不处理 */
            ulRet = sc_demo_task_record_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_PLAYBACK_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_END:
            ulRet = sc_demo_task_palayback_end(pstMsg, pstSCB);
            break;

        case SC_EVT_LEACE_CALL_QUEUE:
            /* 暂时不处理 */
            break;

        case SC_EVT_ERROR_PORT:
            ulRet = sc_demo_task_error(pstMsg, pstSCB);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Invalid event type. %u", pstMsg->ulMsgType);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in auto call Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

U32 sc_srv_call_agent_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in call agent Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_AUTH_RESULT:
            ulRet = sc_call_agent_auth_rsp(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_SETUP:
            ulRet = sc_call_agent_setup(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_AMSWERED:
            ulRet = sc_call_agent_answer(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RINGING:
            ulRet = sc_call_agent_ringing(pstMsg, pstSCB);
            break;

        case SC_EVT_BRIDGE_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_BRIDGE_STOP:
            /* 暂时不处理 */
            break;

        case SC_EVT_HOLD:
            ulRet = sc_call_agent_hold(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_call_agent_release(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_STATUS:
            /* 暂时不处理 */
            break;

        case SC_EVT_DTMF:
            ulRet = sc_call_agent_dtmf(pstMsg, pstSCB);
            break;

        case SC_EVT_RECORD_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_RECORD_END:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_END:
            ulRet = sc_call_agent_playback_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_ERROR_PORT:
            ulRet = sc_call_agent_error(pstMsg, pstSCB);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Invalid event type. %u", pstMsg->ulMsgType);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in call agent Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}


/**
 * 基本呼叫业务状态机处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 当前事件
 * @param SC_SRV_CB *pstSCB      当前业务控制块
 * @param SC_SRV_INFO_ST *pstSubServ 基本呼叫业务控制块
 *
 * return 成功返回DOS_SUCC，否则返回DOS_FALSE
 */
U32 sc_srv_auto_preview_proc(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstSubServ)
{
    U32 ulRet = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstSubServ))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processing %s in auto Preview Service, SCB:%u"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo);

    switch (pstMsg->ulMsgType)
    {
        case SC_EVT_AUTH_RESULT:
            ulRet = sc_auto_preview_auth_rsp(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_SETUP:
            ulRet = sc_auto_preview_setup(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_AMSWERED:
            ulRet = sc_auto_preview_answer(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RINGING:
            ulRet = sc_auto_preview_ringing(pstMsg, pstSCB);
            break;

        case SC_EVT_BRIDGE_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_BRIDGE_STOP:
            /* 暂时不处理 */
            break;

        case SC_EVT_HOLD:
            ulRet = sc_auto_preview_hold(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_RERLEASE:
            ulRet = sc_auto_preview_release(pstMsg, pstSCB);
            break;

        case SC_EVT_CALL_STATUS:
            /* 暂时不处理 */
            break;

        case SC_EVT_DTMF:
            ulRet = sc_auto_preview_dtmf(pstMsg, pstSCB);
            break;

        case SC_EVT_RECORD_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_RECORD_END:
            ulRet = sc_auto_preview_record_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_PLAYBACK_START:
            /* 暂时不处理 */
            break;

        case SC_EVT_PLAYBACK_END:
            ulRet = sc_auto_preview_playback_stop(pstMsg, pstSCB);
            break;

        case SC_EVT_LEACE_CALL_QUEUE:
            ulRet = sc_auto_preview_queue_leave(pstMsg, pstSCB);
            break;

        case SC_EVT_ERROR_PORT:
            ulRet = sc_auto_preview_error(pstMsg, pstSCB);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Invalid event type. %u", pstMsg->ulMsgType);
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Processed %s in Auto Preview Service, SCB:%u, Ret: %s"
                , sc_event_str(pstMsg->ulMsgType), pstSCB->ulSCBNo
                , (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}


/**
 * 处理业务子层发过来的事件，根据事件中指示的业务类型执行不同业务类型的状态机, 如果有多个业务，需要执行业务栈来处理
 *
 * @param SC_MSG_HEAD_ST *pstMsg 时间消息头
 *
 * @return NULL
 */
VOID sc_evt_process(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_EVT_CALL_ST *pstEventCallSetup = NULL;
    SC_SRV_CB          *pstSCB = NULL;
    SC_LEG_CB          *pstLCB = NULL;
    U32                ulSCBNo = U32_BUTT;
    U32                ulRet = DOS_FAIL;
    U32                ulCurrentSrv;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return;
    }

    /* 如果是呼入，就需要创建业务控制块 */
    if (SC_EVT_CALL_SETUP == pstMsg->ulMsgType)
    {
        if (pstMsg->ulSCBNo >= SC_SCB_SIZE)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Call setup event. alloc new scb");

            pstEventCallSetup = (SC_MSG_EVT_CALL_ST *)pstMsg;
            pstLCB = sc_lcb_get(pstEventCallSetup->ulLegNo);
            if (DOS_ADDR_INVALID(pstLCB))
            {
                DOS_ASSERT(0);
                return;
            }

            pstSCB = sc_scb_alloc();
            if (DOS_ADDR_INVALID(pstSCB))
            {
                sc_req_hungup_with_sound(U32_BUTT, pstEventCallSetup->ulLegNo, CC_ERR_SC_SYSTEM_BUSY);
                return;
            }

            pstSCB->stCall.stSCBTag.bValid = DOS_TRUE;
            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_PORC;
            pstSCB->stCall.ulCallingLegNo = pstEventCallSetup->ulLegNo;
            pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stCall.stSCBTag;
            pstLCB->ulSCBNo = pstSCB->ulSCBNo;
            pstMsg->ulSCBNo = pstSCB->ulSCBNo;
        }
        else
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Call setup event with scb %u, Event type:%u", pstMsg->ulSCBNo, pstMsg->ulMsgType);

            pstSCB = sc_scb_get(pstMsg->ulSCBNo);
            if (DOS_ADDR_INVALID(pstSCB))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Get SCB fail. %u", pstMsg->ulSCBNo);
                return;
            }
        }
    }
    else
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Recv event with scb %u, Event type:%u", pstMsg->ulSCBNo, pstMsg->ulMsgType);

        pstSCB = sc_scb_get(pstMsg->ulSCBNo);
        if (DOS_ADDR_INVALID(pstSCB))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Get SCB fail. %u", pstMsg->ulSCBNo);
            return;
        }
    }

    ulSCBNo = pstMsg->ulSCBNo;

    sc_trace_scb(pstSCB, "Processing event %s(%u), SCB:%u, errno: %u"
                    , sc_event_str(pstMsg->ulMsgType), pstMsg->ulMsgType, pstMsg->ulSCBNo, pstMsg->usInterErr);

    /* 使用循环处理业务栈 */
    while (1)
    {
        /* 如果业务栈中只有一个业务，且这个业务不合法，需要释放 */
        ulCurrentSrv = pstSCB->ulCurrentSrv;
        if (0 == pstSCB->ulCurrentSrv &&
            (DOS_ADDR_INVALID(pstSCB->pstServiceList[ulCurrentSrv]) || !pstSCB->pstServiceList[ulCurrentSrv]->bValid))
        {
            sc_scb_free(pstSCB);
            break;
        }

        ulRet = DOS_FAIL;

        switch (pstSCB->pstServiceList[ulCurrentSrv]->usSrvType)
        {
            case SC_SRV_CALL:
                ulRet = sc_srv_call_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_PREVIEW_CALL:
                ulRet = sc_srv_preview_dial_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_AUTO_CALL:
                ulRet = sc_srv_auto_dial_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_VOICE_VERIFY:
                ulRet = sc_srv_voice_verify_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_ACCESS_CODE:
                ulRet = sc_srv_access_code_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_HOLD:
                ulRet = sc_srv_hold_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_TRANSFER:
                ulRet = sc_srv_transfer_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_INCOMING_QUEUE:
                ulRet = sc_srv_incoming_queue_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_INTERCEPTION:
                ulRet = sc_srv_interception_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_WHISPER:
                ulRet = sc_srv_whisper_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_MARK_CUSTOM:
                ulRet = sc_srv_mark_custom_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_AGENT_SIGIN:
                ulRet = sc_srv_sigin_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_DEMO_TASK:
                ulRet = sc_srv_demo_task_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_CALL_AGENT:
                ulRet = sc_srv_call_agent_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            case SC_SRV_AUTO_PREVIEW:
                ulRet = sc_srv_auto_preview_proc(pstMsg, pstSCB, pstSCB->pstServiceList[ulCurrentSrv]);
                break;

            default:
                sc_trace_scb(pstSCB, "Invalid service type : %u", pstSCB->pstServiceList[ulCurrentSrv]->usSrvType);
                break;
        }

        /* 如果返回错误，需要在返回之前将SCB释放掉 */
        if (ulRet != DOS_SUCC)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Process event fail.");
            break;
        }

        if (!SC_SCB_IS_VALID(pstSCB))
        {
            break;
        }


        /* 如果有新业务入栈，继续处理 */
        if (pstSCB->ulCurrentSrv > ulCurrentSrv)
        {
            continue;
        }

        /* 如果当前业务即将退出，就需要退栈 */
        if (pstSCB->pstServiceList[ulCurrentSrv]->bWaitingExit)
        {
            pstSCB->pstServiceList[ulCurrentSrv]->bWaitingExit = DOS_FALSE;

            if (pstSCB->ulCurrentSrv > 0)
            {
                pstSCB->ulCurrentSrv--;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    sc_trace_scb(pstSCB, "Processed %s. ", sc_event_str(pstMsg->ulMsgType));
}

/**
 * 维护业务子层发过来的事件的消息队列
 *
 * @return VOID
 */
VOID *sc_evt_process_runtime(VOID *ptr)
{
    struct timespec     stTimeout;
    DLL_NODE_S    *pstDLLNode = NULL;

    while (1)
    {
        pthread_mutex_lock(&g_mutexEventQueue);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condEventQueue, &g_mutexEventQueue, &stTimeout);
        pthread_mutex_unlock(&g_mutexEventQueue);

        while(1)
        {
            if (DLL_Count(&g_stEventQueue) == 0)
            {
                break;
            }

            pthread_mutex_lock(&g_mutexEventQueue);
            pstDLLNode = dll_fetch(&g_stEventQueue);
            pthread_mutex_unlock(&g_mutexEventQueue);

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

            sc_evt_process((SC_MSG_TAG_ST *)pstDLLNode->pHandle);

            dos_dmem_free(pstDLLNode->pHandle);
            pstDLLNode->pHandle = NULL;
            DLL_Init_Node(pstDLLNode);
            dos_dmem_free(pstDLLNode);
            pstDLLNode = NULL;
        }
    }

}

/**
 * 业务控制层初始化函数
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_event_init()
{
    U32 ulIndex;

    DLL_Init(&g_stEventQueue);

    g_pstSCBList = (SC_SRV_CB *)dos_dmem_alloc(sizeof(SC_SRV_CB) * SC_SCB_SIZE);
    if (DOS_ADDR_INVALID(g_pstSCBList))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    for (ulIndex=0; ulIndex<SC_SCB_SIZE; ulIndex++)
    {
        g_pstSCBList[ulIndex].ulSCBNo = ulIndex;
        sc_scb_init(&g_pstSCBList[ulIndex]);
    }

    return DOS_SUCC;
}

/**
 * 业务控制层启动函数
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_event_start()
{
    if (pthread_create(&g_pthEventQueue, NULL, sc_evt_process_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 业务控制层停止函数
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_event_stop()
{
    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* End of __cplusplus */

