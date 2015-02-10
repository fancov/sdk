/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: mod_dipcc_sc.c
 *
 *  创建时间: 2014年12月16日14:03:55
 *  作    者: Larry
 *  描    述: 业务模块主函数入口
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <switch.h>
#include "sc_pub.h"
#include "sc_task_pub.h"
#include "sc_debug.h"

/* include private header files */
#include "bs_pub.h"
#include "sc_httpd.h"
#include "sc_httpd_pub.h"
#include "sc_pub.h"
#include "sc_bs_pub.h"
#include "sc_task_pub.h"
#include "sc_debug.h"
#include "sc_event_process.h"

extern U32 g_ulTaskTraceAll;

/* define marcos */

/* define enums */

/* define structs */

/* global variables */


/* declare functions */

BOOL g_blSCIsRunning = DOS_FALSE;

static SC_CCB_ST * sc_get_ccb_from_session(switch_core_session_t *session)
{
    S8  *pszUUID = NULL;
    SC_CCB_ST *pstCCB = NULL;
    switch_channel_t *pszChannel = NULL;

    SC_TRACE_IN(session, 0, 0, 0);

    pszChannel = switch_core_session_get_channel(session);
    if (!pszChannel)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return NULL;
    }

    pszUUID = switch_channel_get_uuid(pszChannel);
    if (!pszUUID)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return NULL;
    }

    pstCCB = sc_ccb_hash_tables_find(pszUUID);

    SC_TRACE_OUT();
    return pstCCB;
}

static U32 sc_channel_param_parse(switch_core_session_t *session, SC_CCB_ST *pstCCB)
{
    return DOS_SUCC;
}

static switch_status_t sc_on_init(switch_core_session_t *session)
{
    S8  *pszCCBNo    = NULL;
    S8  *pszUUID     = NULL;
    S8  *pszSource   = NULL;
    S8  *pszAutoDial = NULL;
    S8  szCCBNo[12];
    U32 ulCallDriection, ulCCBNo;
    SC_CCB_ST *pstCCB = NULL;
	switch_channel_t *pszChannel = NULL;
    switch_caller_profile_t *pstCallerPrefile = NULL;

    SC_TRACE_IN(session, 0, 0, 0);

	pszChannel = switch_core_session_get_channel(session);
    pszUUID = switch_channel_get_uuid(pszChannel);
    if (DOS_ADDR_INVALID(pszChannel) || DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return SWITCH_STATUS_TERM;
    }

    /* 不需要loopback的leg */
    pszSource = switch_channel_get_variable(pszChannel, "Channel-Name");
    if (DOS_ADDR_VALID(pszSource)
        && 0 == dos_strnicmp(pszSource, "loopback", dos_strlen("loopback")))
    {
        sc_logr_debug("%s", "Loopback leg, give up process.");

        SC_TRACE_OUT();
        return SWITCH_STATUS_SUCCESS;
    }

    /* 如果没有携带CCB_NO就需要创建CCB */
    pszCCBNo = switch_channel_get_variable(pszChannel, "ccb_no");
    if (DOS_ADDR_INVALID(pszCCBNo))
    {
        pstCCB = sc_ccb_alloc();
        if (!pstCCB)
        {
            DOS_ASSERT(0);
            sc_logr_error("%s", "Alloc CCB FAIL.");

            SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);

            SC_TRACE_OUT();
            return SWITCH_STATUS_TERM;
        }

        /* 设置变量 */
        dos_snprintf(szCCBNo, sizeof(szCCBNo), "%d", pstCCB->usCCBNo);
        switch_channel_set_variable(pszChannel, "ccb_no", szCCBNo);

        pstCCB->aucServiceType[pstCCB->ulCurrentSrvInd++] = SC_SERV_INBOUND_CALL;
    }
    else
    {
        if (dos_atoul(pszCCBNo, &ulCCBNo) < 0)
        {
            DOS_ASSERT(0);
            sc_logr_error("Invalid CCB NO. %s", pszCCBNo);

            SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);

            SC_TRACE_OUT();
            return SWITCH_STATUS_TERM;
        }

        pstCCB = sc_ccb_get(ulCCBNo);
        if (DOS_ADDR_INVALID(pstCCB))
        {
            DOS_ASSERT(0);
            sc_logr_error("Cannot find the ccb with CCBNO %d", ulCCBNo);

            SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);

            SC_TRACE_OUT();
            return SWITCH_STATUS_TERM;
        }

        pstCCB->aucServiceType[pstCCB->ulCurrentSrvInd++] = SC_SERV_OUTBOUND_CALL;
    }

    /* 如果是自动外呼，置上业务就行，如果不是，就需要解析相关参数填充CCB */
    pszAutoDial = switch_channel_get_variable(pszChannel, "auto_flag");
    if (pszAutoDial)
    {
        pstCCB->aucServiceType[pstCCB->ulCurrentSrvInd++] = SC_SERV_AUTO_DIALING;
    }
    else
    {
        /* 获取相关参数 */
        pstCallerPrefile = switch_channel_get_caller_profile(pszChannel);
        if (DOS_ADDR_VALID(pstCallerPrefile))
        {
            pstCCB->usTCBNo = U16_BUTT;
            pstCCB->usSiteNo = U16_BUTT;
            pstCCB->usCallerNo = U16_BUTT;
            pstCCB->ulCustomID = U32_BUTT;
            pstCCB->ulAgentID = U32_BUTT;
        }
    }

    sc_call_trace(pstCCB, "Add CCB into Hash Table.");

    /* 加入HASH表 */
    if (sc_ccb_hash_tables_add(pszUUID, pstCCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return SWITCH_STATUS_TERM;
    }

    SC_CCB_SET_STATUS(pstCCB, SC_CCB_INIT);

    SC_TRACE_OUT();
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t sc_on_routing(switch_core_session_t *session)
{
    /* 没有什么要处理的 */
    return SWITCH_STATUS_SUCCESS;
}

static switch_status_t sc_on_execute(switch_core_session_t *session)
{
    U32 ulStatus;
    S8  *pszUUID = NULL;
    S8  *pszSource   = NULL;
    SC_CCB_ST *pstCCB = NULL;
	switch_channel_t *pszChannel = NULL;

    SC_TRACE_IN(session, 0, 0, 0);

	pszChannel = switch_core_session_get_channel(session);
    pszUUID = switch_channel_get_uuid(pszChannel);
    if (DOS_ADDR_INVALID(pszChannel) || DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return SWITCH_STATUS_TERM;
    }

    /* 不需要loopback的leg */
    pszSource = switch_channel_get_variable(pszChannel, "Channel-Name");
    if (DOS_ADDR_VALID(pszSource)
        && 0 == dos_strnicmp(pszSource, "loopback", dos_strlen("loopback")))
    {
        sc_logr_debug("%s", "Loopback leg, give up process.");

        return SWITCH_STATUS_SUCCESS;
    }


    pstCCB = sc_get_ccb_from_session(session);
    if (DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return SWITCH_STATUS_TERM;
    }

    sc_call_trace(pstCCB, "Process the exec msg.");

    ulStatus = pstCCB->usCCBNo;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
            /* 没有BREAK */
        case SC_CCB_INIT:
            SC_CCB_SET_STATUS(pstCCB, SC_CCB_AUTH);
            /* 没有BREAK */
        case SC_CCB_AUTH:
            sc_logr_info("Start verify the call with UUID : %s.", pszUUID);

            if (sc_send_usr_auth2bs(pstCCB) != DOS_SUCC)
            {
                /* 拒绝呼叫 */
            }

            if (pstCCB->ucTerminationFlag)
            {
                /* 拒绝呼叫 */
            }

            SC_CCB_SET_STATUS(pstCCB, SC_CCB_EXEC);
            sc_logr_info("Start verify the call successfully. UUID : %s.", pszUUID);
        case SC_CCB_EXEC:
            /* 如果已经认证过了，就是这个状态 */
            SC_CCB_SET_STATUS(pstCCB, SC_CCB_EXEC);
            break;
        case SC_CCB_REPORT:
            /* @TODO report the call status */
            SC_CCB_SET_STATUS(pstCCB, SC_CCB_RELEASE);
            break;
        case SC_CCB_RELEASE:
            sc_ccb_hash_tables_delete(pszUUID);

            sc_ccb_free(pstCCB);
        default:
            DOS_ASSERT(0);
            break;
    }

    sc_call_trace(pstCCB, "Exec msg processed finished.");

    SC_TRACE_OUT();
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t sc_on_exch_media(switch_core_session_t *session)
{
    U32 ulStatus;
    S8  *pszUUID = NULL;
    S8  *pszSource   = NULL;
    SC_CCB_ST *pstCCB = NULL;
	switch_channel_t *pszChannel = NULL;

    SC_TRACE_IN(session, 0, 0, 0);

	pszChannel = switch_core_session_get_channel(session);
    pszUUID = switch_channel_get_uuid(pszChannel);
    if (DOS_ADDR_INVALID(pszChannel) || DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }

    /* 不需要loopback的leg */
    pszSource = switch_channel_get_variable(pszChannel, "Channel-Name");
    if (DOS_ADDR_VALID(pszSource)
        && 0 == dos_strnicmp(pszSource, "loopback", dos_strlen("loopback")))
    {
        sc_logr_debug("%s", "Loopback leg, give up process.");

        SC_TRACE_OUT();
        return SWITCH_STATUS_SUCCESS;
    }

    pstCCB = sc_get_ccb_from_session(session);
    if (DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return SWITCH_STATUS_TERM;
    }

    sc_call_trace(pstCCB, "Porcess CS_CUSTOM_MEDIA msg.");

    ulStatus = pstCCB->usCCBNo;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
            /* 没有BREAK */
        case SC_CCB_INIT:
            /* 没有BREAK */
        case SC_CCB_AUTH:
            sc_logr_info("Start verify the call with UUID : %s.", pszUUID);
            /* @TODO AUTH */
            sc_logr_info("Start verify the call successfully. UUID : %s.", pszUUID);
        case SC_CCB_EXEC:
            sc_logr_info("Start billing call with UUID : %s.", pszUUID);
            /* @TODO : start billing */
            sc_logr_info("Start billing call successfully. UUID : %s.", pszUUID);
            SC_CCB_SET_STATUS(pstCCB, SC_CCB_EXEC);
            break;
        case SC_CCB_REPORT:
            /* @TODO report the call status */
            SC_CCB_SET_STATUS(pstCCB, SC_CCB_RELEASE);
            break;
        case SC_CCB_RELEASE:
            sc_ccb_hash_tables_delete(pszUUID);

            sc_ccb_free(pstCCB);
        default:
            DOS_ASSERT(0);
            break;
    }

    sc_call_trace(pstCCB, "Porcess CS_CUSTOM_MEDIA msg finished.");

    SC_TRACE_OUT();
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t sc_on_hungup(switch_core_session_t *session)
{
    U32 ulStatus;
    S8  *pszUUID = NULL;
    S8  *pszSource   = NULL;
    SC_CCB_ST *pstCCB = NULL;
	switch_channel_t *pszChannel = NULL;

    SC_TRACE_IN(session, 0, 0, 0);

	pszChannel = switch_core_session_get_channel(session);
    pszUUID = switch_channel_get_uuid(pszChannel);
    if (DOS_ADDR_INVALID(pszChannel) || DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return SWITCH_STATUS_TERM;
    }

    /* 不需要loopback的leg */
    pszSource = switch_channel_get_variable(pszChannel, "Channel-Name");
    if (DOS_ADDR_VALID(pszSource)
        && 0 == dos_strnicmp(pszSource, "loopback", dos_strlen("loopback")))
    {
        sc_logr_debug("%s", "Loopback leg, give up process.");

        SC_TRACE_OUT();
        return SWITCH_STATUS_SUCCESS;
    }

    pstCCB = sc_get_ccb_from_session(session);
    if (DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return SWITCH_STATUS_TERM;
    }

    sc_call_trace(pstCCB, "Porcess CS_HUNGUP msg.");

    ulStatus = pstCCB->usCCBNo;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
            /* 没有BREAK */
        case SC_CCB_INIT:
            /* 没有BREAK */
        case SC_CCB_AUTH:
            sc_logr_info("Start verify the call with UUID : %s.", pszUUID);
            /* @TODO AUTH */
            sc_logr_info("Start verify the call successfully. UUID : %s.", pszUUID);
        case SC_CCB_EXEC:
            sc_logr_info("Stop billing call with UUID : %s.", pszUUID);
            /* @TODO : stop billing */
            sc_logr_info("Stop billing call successfully. UUID : %s.", pszUUID);

            SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);
            break;
        case SC_CCB_REPORT:
            /* @TODO report the call status */
            SC_CCB_SET_STATUS(pstCCB, SC_CCB_RELEASE);
            break;
        case SC_CCB_RELEASE:
            sc_ccb_hash_tables_delete(pszUUID);

            sc_ccb_free(pstCCB);
        default:
            DOS_ASSERT(0);
            break;
    }

    sc_call_trace(pstCCB, "Porcess CS_HUNGUP msg finished.");

    SC_TRACE_OUT();
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t sc_on_report(switch_core_session_t *session)
{
    /* DO Nothing */
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t sc_on_destroy(switch_core_session_t *session)
{
    U32 ulStatus;
    S8  *pszUUID = NULL;
    S8  *pszSource   = NULL;
    SC_CCB_ST *pstCCB = NULL;
	switch_channel_t *pszChannel = NULL;

    SC_TRACE_IN(session, 0, 0, 0);

	pszChannel = switch_core_session_get_channel(session);
    pszUUID = switch_channel_get_uuid(pszChannel);
    if (DOS_ADDR_INVALID(pszChannel) || DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }

    /* 不需要loopback的leg */
    pszSource = switch_channel_get_variable(pszChannel, "Channel-Name");
    if (DOS_ADDR_VALID(pszSource)
        && 0 == dos_strnicmp(pszSource, "loopback", dos_strlen("loopback")))
    {
        sc_logr_debug("%s", "Loopback leg, give up process.");

        SC_TRACE_OUT();
        return SWITCH_STATUS_SUCCESS;
    }

    pstCCB = sc_get_ccb_from_session(session);
    if (DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return SWITCH_STATUS_TERM;
    }

    sc_call_trace(pstCCB, "Porcess CS_DESTROY msg.");

    ulStatus = pstCCB->usCCBNo;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
            /* 没有BREAK */
        case SC_CCB_INIT:
            /* 没有BREAK */
        case SC_CCB_AUTH:
            /* 没有BREAK */
        case SC_CCB_EXEC:

            sc_send_billing_stop2bs(pstCCB);

            SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);

            /* 没有BREAK */
        case SC_CCB_REPORT:
            SC_CCB_SET_STATUS(pstCCB, SC_CCB_RELEASE);
        case SC_CCB_RELEASE:
            sc_ccb_hash_tables_delete(pszUUID);

            sc_ccb_free(pstCCB);
        default:
            DOS_ASSERT(0);
            break;
    }

    sc_call_trace(pstCCB, "Porcess CS_DESTROY msg finished.");

    SC_TRACE_OUT();
	return SWITCH_STATUS_SUCCESS;
}


switch_state_handler_table_t sc_state_handler = {
	/* on_init */ sc_on_init,
	/* on_routing */ sc_on_routing,
	/* on_execute */ sc_on_execute,
	/* on_hangup */ sc_on_hungup,
	/* on_exch_media */ sc_on_exch_media,
	/* on_soft_exec */ NULL,
	/* on_consume_med */ NULL,
	/* on_hibernate */ NULL,
	/* on_reset */ NULL,
	/* on_park */ NULL,
	/* on_reporting */ sc_on_report,
	/* on_destroy */ sc_on_destroy
};


SWITCH_MODULE_LOAD_FUNCTION(mod_dipcc_sc_load);
SWITCH_MODULE_RUNTIME_FUNCTION(mod_dipcc_sc_runtime);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_dipcc_sc_shutdown);

SWITCH_MODULE_DEFINITION(mod_dipcc_sc, mod_dipcc_sc_load, mod_dipcc_sc_shutdown, mod_dipcc_sc_runtime);


SWITCH_MODULE_LOAD_FUNCTION(mod_dipcc_sc_load)
{
    /* 加载 */
    if (sc_httpd_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }

    if (sc_task_mngt_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }

    if (sc_dialer_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }
#if 1
    if (sc_ep_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }
#endif

    if (DOS_SUCC != sc_acd_init())
    {
        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }

    if (DOS_SUCC != sc_bs_fsm_init())
    {
        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }

    *module_interface = switch_loadable_module_create_module_interface(pool, modname);

    /* register state handlers for billing */
    switch_core_add_state_handler(&sc_state_handler);

    return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_RUNTIME_FUNCTION(mod_dipcc_sc_runtime)
{
    if (sc_httpd_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }

    if (sc_task_mngt_start() != DOS_SUCC)
    {
        sc_httpd_shutdown();

        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }

    if (sc_dialer_start() != DOS_SUCC)
    {
        sc_httpd_shutdown();
        sc_task_mngt_shutdown();

        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }

    if (DOS_SUCC != sc_bs_fsm_start())
    {
        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }

#if 0
    if (sc_ep_start() != DOS_SUCC)
    {
        /* @TODO, 释放资源 */
        DOS_ASSERT(0);
        return SWITCH_STATUS_TERM;
    }
#endif
    g_blSCIsRunning = DOS_TRUE;
    while (g_blSCIsRunning)
    {
        dos_task_delay(10000);

        continue;
    }

    sc_httpd_shutdown();
    sc_task_mngt_shutdown();
    sc_dialer_shutdown();

    return SWITCH_STATUS_TERM;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_dipcc_sc_shutdown)
{
    g_blSCIsRunning = DOS_FALSE;
    switch_core_remove_state_handler(&sc_state_handler);

    return SWITCH_STATUS_UNLOAD;

}

#ifdef __cplusplus
}
#endif /* __cplusplus */


