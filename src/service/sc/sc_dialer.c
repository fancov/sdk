/*
*            (C) Copyright 2014, DIPCC . Co., Ltd.
*                    ALL RIGHTS RESERVED
*
*  文件名: sc_dialer.h
*
*  创建时间: 2014年12月16日10:19:49
*  作    者: Larry
*  描    述: 群呼任务拨号器
*  修改历史:
*/

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <sys/time.h>

/* include private header files */
#include <esl.h>
#include <bs_pub.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"

/* define marcos */

/* define enums */

/* define structs */
/* 呼叫队列 */
typedef struct tagSCCallQueueNode
{
    list_t      stList;                         /* 呼叫队列链表 */
    SC_SCB_ST   *pstSCB;                        /* 呼叫控制块 */
}SC_CALL_QUEUE_NODE_ST;

/* dialer模块控制块示例 */
SC_DIALER_HANDLE_ST  *g_pstDialerHandle = NULL;

U32 sc_dial_make_call_for_verify(U32 ulCustomer, S8 *pszNumber, S8 *pszPassword, U32 ulPlayCnt)
{
    SC_SCB_ST *pstSCB = NULL;
    U32   ulRouteID;
    S8    szCaller[SC_TEL_NUMBER_LENGTH] = {0};

    if (0 == ulCustomer || U32_BUTT == ulCustomer)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pszNumber)
        || DOS_ADDR_INVALID(pszPassword))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstSCB->ulCustomID = ulCustomer;
    dos_strncpy(pstSCB->szCalleeNum, pszNumber, sizeof(pstSCB->szCalleeNum));
    pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';

    /* 主叫号码，根据客户从主叫号码组中获得一个主叫号码 */
    if (sc_caller_setting_select_number(ulCustomer, 0, SC_SRC_CALLER_TYPE_ALL, szCaller, SC_TEL_NUMBER_LENGTH) != DOS_SUCC)
    {
        sc_logr_info(SC_HTTPD, "Get caller FAIL by customer(%u)", ulCustomer);
        DOS_ASSERT(0);
        goto proc_fail;
    }

    sc_logr_debug(SC_HTTPD, "Get caller(%s) SUCC by customer(%u)", szCaller, ulCustomer);
    dos_strncpy(pstSCB->szCallerNum, szCaller, sizeof(pstSCB->szCallerNum));
    pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) - 1] = '\0';
    dos_strncpy(pstSCB->szDialNum, pszPassword, sizeof(pstSCB->szDialNum));
    pstSCB->szDialNum[sizeof(pstSCB->szDialNum) - 1] = '\0';
    pstSCB->ucCurrentPlyCnt = ulPlayCnt;

    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL);
    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_NUM_VERIFY);

    ulRouteID = sc_ep_search_route(pstSCB);
    if (U32_BUTT == ulRouteID)
    {
        DOS_ASSERT(0);
        sc_logr_info(SC_ESL, "%s", "Search route fail while make call to patn.");
        goto proc_fail;
    }

    if (!sc_ep_black_list_check(ulCustomer, pszNumber))
    {
        DOS_ASSERT(0);
        sc_logr_info(SC_ESL, "Cannot make call for number %s which is in black list.", pszNumber);
        goto proc_fail;
    }

    /* 认证 */
    if (sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_info(SC_ESL, "%s", "Send auth msg FAIL.");
        goto proc_fail;
    }
    else
    {
        SC_SCB_SET_STATUS(pstSCB, SC_SCB_AUTH);
    }

    return DOS_SUCC;

proc_fail:
    pstSCB->bTerminationFlag = DOS_TRUE;
    pstSCB->usTerminationCause = BS_ERR_SYSTEM;

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);

    DOS_ASSERT(0);
    sc_scb_free(pstSCB);
    pstSCB = NULL;

    return DOS_FAIL;
}

U32 sc_dial_make_call2eix(SC_SCB_ST *pstSCB, U32 ulMainService)
{
    S8 szEIXAddr[SC_IP_ADDR_LEN] = { 0, };
    S8 szCMDBuff[SC_ESL_CMD_BUFF_LEN] = { 0, };
    S8   *pszEventHeader = NULL, *pszEventBody = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (sc_ep_get_eix_by_tt(pstSCB->szCalleeNum, szEIXAddr, sizeof(szEIXAddr)) != DOS_SUCC
        || '\0' == szEIXAddr[0])
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    dos_snprintf(szCMDBuff, sizeof(szCMDBuff)
                    , "bgapi originate{main_service=%u,scb_number=%u,origination_caller_id_number=%s," \
                      "origination_caller_id_name=%s,sip_multipart=^^!application/x-allywll:m:=2!" \
                      "calli:=818!l:=01057063943!usert:=0!callt:=4!eig:=370!he:=5!w:=0!,sip_h_EixTTcall" \
                      "=TRUE,sip_h_Mime-version=1.0}sofia/external/%s@%s &park \r\n"
                    , ulMainService
                    , pstSCB->usSCBNo
                    , pstSCB->szCallerNum
                    , pstSCB->szCallerNum
                    , pstSCB->szCalleeNum
                    , szEIXAddr);

    sc_logr_debug(SC_DIALER, "ESL CMD: %s", szCMDBuff);

    if (esl_send_recv(&g_pstDialerHandle->stHandle, szCMDBuff) != ESL_SUCCESS)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call FAIL.Msg:%s(%d)", g_pstDialerHandle->stHandle.err, g_pstDialerHandle->stHandle.errnum);
        g_pstDialerHandle->blIsESLRunning = DOS_FALSE;
        goto esl_exec_fail;
    }

    if (!g_pstDialerHandle->stHandle.last_sr_event)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "%s", "ESL request call successfully. But the reply event is NULL.");

        goto esl_exec_fail;
    }

    pszEventHeader = esl_event_get_header(g_pstDialerHandle->stHandle.last_sr_event, "Content-Type");
    if (!pszEventHeader || '\0' == pszEventHeader[0]
        || dos_strcmp(pszEventHeader, "command/reply") != 0)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call successfully. But the reply event an invalid content-type.(Type:%s)", pszEventHeader);

        goto esl_exec_fail;

    }

    pszEventBody = esl_event_get_header(g_pstDialerHandle->stHandle.last_sr_event, "reply-text");
    if (!pszEventBody || '\0' == pszEventBody[0])
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call successfully. But the reply event an invalid reply-text.(Type:%s)", pszEventBody);

        goto esl_exec_fail;
    }

    if (dos_strnicmp(pszEventBody, "+OK", dos_strlen("+OK")) != 0)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL exec fail. (Reply-Text:%s)", pszEventBody);

        goto esl_exec_fail;
    }

    sc_logr_info(SC_DIALER, "Make call successfully. Caller:%s, Callee:%s", pstSCB->szCallerNum, pstSCB->szCalleeNum);

    SC_TRACE_OUT();
    return DOS_SUCC;

esl_exec_fail:

    sc_logr_info(SC_DIALER, "%s", "ESL Exec fail, the call will be FREE.");

    SC_TRACE_OUT();
    return DOS_FAIL;

}

/* 这个地方有个问题。 g_pstDialerHandle->stHandle 被多个线程使用，会不会出现，一个线程刚刚发送了呼叫命令。名外一个线程收到了响应?*/
U32 sc_dial_make_call2ip(SC_SCB_ST *pstSCB, U32 ulMainService, BOOL bIsUpdateCaller)
{
    S8    szCMDBuff[SC_ESL_CMD_BUFF_LEN] = { 0 };
    S8    *pszEventHeader = NULL, *pszEventBody = NULL;
    S8    szNumber[SC_TEL_NUMBER_LENGTH] = {0};
    U32   ulRet;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (bIsUpdateCaller)
    {
        /* 调用主叫号码组接口，将主叫号码变更为对应的did */
        if (U32_BUTT == pstSCB->ulAgentID)
        {
            sc_logr_info(SC_DIALER, "Call to IP not get agent ID by scd(%u)", pstSCB->usSCBNo);

            goto go_on;
        }

        /* 查找呼叫源和号码的对应关系，如果匹配上某一呼叫源，就选择特定号码 */
        ulRet = sc_caller_setting_select_number(pstSCB->ulCustomID, pstSCB->ulAgentID, SC_SRC_CALLER_TYPE_AGENT, szNumber, SC_TEL_NUMBER_LENGTH);
        if (ulRet != DOS_SUCC)
        {
            DOS_ASSERT(0);
            sc_logr_info(SC_DIALER, "Call to IP customID(%u) get caller number FAIL by agnet(%u)", pstSCB->ulCustomID, pstSCB->ulAgentID);

            goto go_on;
        }

        sc_logr_info(SC_DIALER, "Call to IP customID(%u) get caller number(%s) SUCC by agnet(%u)", pstSCB->ulCustomID, szNumber, pstSCB->ulAgentID);
        dos_strncpy(pstSCB->szCallerNum, szNumber, SC_TEL_NUMBER_LENGTH);
        pstSCB->szCallerNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';
    }
go_on:
    dos_snprintf(szCMDBuff, sizeof(szCMDBuff)
                    , "bgapi originate {main_service=%u,scb_number=%u,origination_caller_id_number=%s,origination_caller_id_name=%s}user/%s &park \r\n"
                    , ulMainService
                    , pstSCB->usSCBNo
                    , pstSCB->szCallerNum
                    , pstSCB->szCallerNum
                    , pstSCB->szCalleeNum);

    sc_logr_debug(SC_DIALER, "ESL CMD: %s", szCMDBuff);

    if (esl_send_recv(&g_pstDialerHandle->stHandle, szCMDBuff) != ESL_SUCCESS)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call FAIL.Msg:%s(%d)", g_pstDialerHandle->stHandle.err, g_pstDialerHandle->stHandle.errnum);
        g_pstDialerHandle->blIsESLRunning = DOS_FALSE;
        goto esl_exec_fail;
    }

    if (!g_pstDialerHandle->stHandle.last_sr_event)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "%s", "ESL request call successfully. But the reply event is NULL.");

        goto esl_exec_fail;
    }

    pszEventHeader = esl_event_get_header(g_pstDialerHandle->stHandle.last_sr_event, "Content-Type");
    if (!pszEventHeader || '\0' == pszEventHeader[0]
        || dos_strcmp(pszEventHeader, "command/reply") != 0)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call successfully. But the reply event an invalid content-type.(Type:%s)", pszEventHeader);

        goto esl_exec_fail;

    }

    pszEventBody = esl_event_get_header(g_pstDialerHandle->stHandle.last_sr_event, "reply-text");
    if (!pszEventBody || '\0' == pszEventBody[0])
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call successfully. But the reply event an invalid reply-text.(Type:%s)", pszEventBody);

        goto esl_exec_fail;
    }

    if (dos_strnicmp(pszEventBody, "+OK", dos_strlen("+OK")) != 0)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL exec fail. (Reply-Text:%s)", pszEventBody);

        goto esl_exec_fail;
    }

    sc_logr_info(SC_DIALER, "Make call successfully. Caller:%s, Callee:%s", pstSCB->szCallerNum, pstSCB->szCalleeNum);

    SC_TRACE_OUT();
    return DOS_SUCC;

esl_exec_fail:

    sc_logr_info(SC_DIALER, "%s", "ESL Exec fail, the call will be FREE.");

    SC_TRACE_OUT();
    return DOS_FAIL;
}

U32 sc_dialer_make_call2pstn(SC_SCB_ST *pstSCB, U32 ulMainService)
{
    S8    *pszEventHeader   = NULL;
    S8    *pszEventBody     = NULL;
    S8    *pszUUID          = NULL;
    S8    szCMDBuff[SC_ESL_CMD_BUFF_LEN] = { 0 };
    S8    szCallString[SC_ESL_CMD_BUFF_LEN] = { 0 };
    U32   ulRouteID         = U32_BUTT;
    SC_SCB_ST *pstSCBOther  = NULL;
    U32   ulTrunkCount    = 0;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if ('\0' == pstSCB->szCalleeNum[0]
        || '\0' == pstSCB->szCallerNum[0])
    {
        DOS_ASSERT(0);

        goto esl_exec_fail;
    }

    ulRouteID = sc_ep_search_route(pstSCB);
    if (U32_BUTT == ulRouteID)
    {
        DOS_ASSERT(0);
        sc_logr_info(SC_ESL, "%s", "Search route fail while make call to patn.");
        goto esl_exec_fail;;
    }

    ulTrunkCount = sc_ep_get_callee_string(ulRouteID, pstSCB, szCallString, sizeof(szCallString));
    if (ulTrunkCount == 0)
    {
        DOS_ASSERT(0);

        goto esl_exec_fail;
    }

    pstSCB->ulTrunkCount = ulTrunkCount;

    /* 如果当前控制块有另外一通呼叫，直接bridge就好，否则需要发起新呼叫 */
    pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
    if (DOS_ADDR_VALID(pstSCBOther))
    {
        dos_snprintf(szCMDBuff, sizeof(szCMDBuff)
                        , "{instant_ringback=true,scb_number=%u,other_leg_scb=%u,main_service=%u,origination_caller_id_number=%s,origination_caller_id_name=%s}%s"
                        , pstSCB->usSCBNo
                        , pstSCB->usOtherSCBNo
                        , ulMainService
                        , pstSCB->szCallerNum
                        , pstSCB->szCallerNum
                        , szCallString);

        sc_logr_debug(SC_DIALER, "ESL CMD: %s", szCMDBuff);

        if (sc_ep_esl_execute("bridge", szCMDBuff, pstSCBOther->szUUID) != ESL_SUCCESS)
        {
            DOS_ASSERT(0);

            esl_execute(&g_pstDialerHandle->stHandle, "hangup", NULL, pstSCBOther->szUUID);
            esl_execute(&g_pstDialerHandle->stHandle, "hangup", NULL, pstSCB->szUUID);
            return DOS_FAIL;
        }

        if (esl_execute(&g_pstDialerHandle->stHandle, "hangup", NULL, pstSCB->szUUID) != ESL_SUCCESS)
        {
            DOS_ASSERT(0);

            esl_execute(&g_pstDialerHandle->stHandle, "hangup", NULL, pstSCB->szUUID);
            return DOS_FAIL;
        }

        SC_SCB_SET_STATUS(pstSCB, SC_SCB_EXEC);

        return DOS_SUCC;
    }

    dos_snprintf(szCMDBuff, sizeof(szCMDBuff)
                        , "bgapi originate {scb_number=%u,main_service=%u,origination_caller_id_number=%s,origination_caller_id_name=%s,waiting_park=true}%s &park \r\n"
                        , pstSCB->usSCBNo
                        , ulMainService
                        , pstSCB->szCallerNum
                        , pstSCB->szCallerNum
                        , szCallString);

    sc_logr_debug(SC_DIALER, "ESL CMD: %s", szCMDBuff);

    if (esl_send_recv(&g_pstDialerHandle->stHandle, szCMDBuff) != ESL_SUCCESS)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call FAIL.Msg:%s(%d)", g_pstDialerHandle->stHandle.err, g_pstDialerHandle->stHandle.errnum);

        g_pstDialerHandle->blIsESLRunning = DOS_FALSE;

        goto esl_exec_fail;
    }

    if (!g_pstDialerHandle->stHandle.last_sr_event)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "%s", "ESL request call successfully. But the reply event is NULL.");

        goto esl_exec_fail;
    }

    pszEventHeader = esl_event_get_header(g_pstDialerHandle->stHandle.last_sr_event, "Content-Type");
    if (!pszEventHeader || '\0' == pszEventHeader[0]
        || dos_strcmp(pszEventHeader, "command/reply") != 0)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call successfully. But the reply event an invalid content-type.(Type:%s)", pszEventHeader);

        goto esl_exec_fail;

    }

    pszEventBody = esl_event_get_header(g_pstDialerHandle->stHandle.last_sr_event, "reply-text");
    if (!pszEventBody || '\0' == pszEventBody[0])
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL request call successfully. But the reply event an invalid reply-text.(Type:%s)", pszEventBody);

        goto esl_exec_fail;
    }

    if (dos_strnicmp(pszEventBody, "+OK", dos_strlen("+OK")) != 0)
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_DIALER, "ESL exec fail. (Reply-Text:%s)", pszEventBody);

        goto esl_exec_fail;
    }

    pszUUID = dos_strstr(pszEventBody, "+OK ");
    if (DOS_ADDR_INVALID(pszUUID) || pszUUID[0] == '\0')
    {
        DOS_ASSERT(0);
    }
    else
    {
        pszUUID += dos_strlen("+OK ");
        if (DOS_ADDR_INVALID(pszUUID) || pszUUID[0] == '\0')
        {
            DOS_ASSERT(0);
        }
        else
        {
            sc_bg_job_hash_add(pszUUID, dos_strlen(pszUUID), pstSCB->usSCBNo);
        }
    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_EXEC);

    sc_logr_info(SC_DIALER, "Make call successfully. Caller:%s, Callee:%s", pstSCB->szCallerNum, pstSCB->szCalleeNum);
    sc_call_trace(pstSCB, "Make call successfully.");

    SC_TRACE_OUT();
    return DOS_SUCC;

esl_exec_fail:

    sc_logr_info(SC_DIALER, "%s", "ESL Exec fail, the call will be FREE.");

    if (sc_call_check_service(pstSCB, SC_SERV_AGENT_SIGNIN))
    {
        sc_acd_update_agent_status(SC_ACD_SITE_ACTION_CONNECT_FAIL, pstSCB->ulAgentID);
    }

    pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
    if (DOS_ADDR_VALID(pstSCBOther))
    {
        esl_execute(&g_pstDialerHandle->stHandle, "hangup", NULL, pstSCBOther->szUUID);
    }

    DOS_ASSERT(0);
    sc_scb_free(pstSCB);

    SC_TRACE_OUT();
    return DOS_FAIL;
}

/*
 * 函数: VOID *sc_dialer_runtime(VOID * ptr)
 * 功能: 拨号模块主函数
 * 参数:
 */
VOID *sc_dialer_runtime(VOID * ptr)
{
    list_t                  *pstList;
    SC_CALL_QUEUE_NODE_ST   *pstListNode;
    SC_SCB_ST               *pstSCB;
    SC_SCB_ST               *pstOtherSCB;
    struct timespec         stTimeout;
    U32                     ulRet = ESL_FAIL;
    U32                     ulIdelCPU = 0;
    U32                     ulIdelCPUConfig = 0;
    U32                     ulAgentID = U32_BUTT;
    S8                      szNumber[SC_TEL_NUMBER_LENGTH] = {0};

    if (config_get_min_iedl_cpu(&ulIdelCPUConfig) != DOS_SUCC)
    {
        ulIdelCPUConfig = DOS_MIN_IDEL_CPU * 100;
    }
    else
    {
        ulIdelCPUConfig = ulIdelCPUConfig * 100;
    }

    while(1)
    {
        /* 如果退出标志被置上，就准备退出了 */
        if (g_pstDialerHandle->blIsWaitingExit)
        {
            sc_logr_notice(SC_DIALER, "%s", "Dialer exit flag has been set. the task will be exit.");
            break;
        }

        /*
         * 检查连接是否正常
         * 如果连接不正常，就准备重连
         **/
        if (!g_pstDialerHandle->blIsESLRunning)
        {
            sc_logr_notice(SC_DIALER, "%s", "ELS connection has been down, re-connect.");
            ulRet = esl_connect(&g_pstDialerHandle->stHandle, "127.0.0.1", 8021, NULL, "ClueCon");
            if (ESL_SUCCESS != ulRet)
            {
                esl_disconnect(&g_pstDialerHandle->stHandle);
                sc_logr_notice(SC_DIALER, "ELS re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_pstDialerHandle->stHandle.err);

                sleep(1);
                continue;
            }

            g_pstDialerHandle->blIsESLRunning = DOS_TRUE;

            sc_logr_notice(SC_DIALER, "%s", "ELS connect Back to Normal.");
        }

        pthread_mutex_lock(&g_pstDialerHandle->mutexCallQueue);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_pstDialerHandle->condCallQueue, &g_pstDialerHandle->mutexCallQueue, &stTimeout);
        pthread_mutex_unlock(&g_pstDialerHandle->mutexCallQueue);

        if (dos_list_is_empty(&g_pstDialerHandle->stCallList))
        {
            continue;
        }

        while (1)
        {
            /* 控制CPS */
            dos_task_delay(1000 / SC_MAX_CALL_PRE_SEC);

            /* 检查CPU占用 */
            ulIdelCPU = dos_get_cpu_idel_percentage();
            if (ulIdelCPU < ulIdelCPUConfig)
            {
                sc_logr_debug(SC_DIALER, "cpu too high. ulIdelCPU : %u, ulIdelCPUConfig : %u", ulIdelCPU, ulIdelCPUConfig);
                continue;
            }

            pthread_mutex_lock(&g_pstDialerHandle->mutexCallQueue);
            if (dos_list_is_empty(&g_pstDialerHandle->stCallList))
            {
                pthread_mutex_unlock(&g_pstDialerHandle->mutexCallQueue);
                break;
            }

            pstList = dos_list_fetch(&g_pstDialerHandle->stCallList);
            if (!pstList)
            {
                pthread_mutex_unlock(&g_pstDialerHandle->mutexCallQueue);
                continue;
            }

            if (0 == g_pstDialerHandle->ulCallCnt)
            {
                g_pstDialerHandle->ulCallCnt = 0;
                DOS_ASSERT(0);
            }
            else
            {
                g_pstDialerHandle->ulCallCnt--;
            }
            pthread_mutex_unlock(&g_pstDialerHandle->mutexCallQueue);

            pstListNode = dos_list_entry(pstList, SC_CALL_QUEUE_NODE_ST, stList);
            if (!pstList)
            {
                DOS_ASSERT(0);
                continue;
            }

            pstSCB = pstListNode->pstSCB;
            if (!pstSCB)
            {
                goto free_res;
                continue;
            }

            /* 主叫号码组 */
            if (sc_call_check_service(pstSCB, SC_SERV_OUTBOUND_CALL)
                && sc_call_check_service(pstSCB, SC_SERV_EXTERNAL_CALL))
            {
                /* 出局呼叫 */
                /* 查找呼叫的分机绑定的坐席 */
                pstOtherSCB = sc_scb_get(pstSCB->usOtherSCBNo);
                if (DOS_ADDR_INVALID(pstOtherSCB))
                {
                    sc_logr_info(SC_DIALER, "Not get other scb. scbNo : %u, other : %u", pstSCB->usSCBNo, pstSCB->usOtherSCBNo);

                    goto go_on;
                }

                ulAgentID = pstOtherSCB->ulAgentID;
                if (U32_BUTT == ulAgentID)
                {
                    sc_logr_info(SC_DIALER, "Not get agent ID by scd(%u)", pstSCB->usOtherSCBNo);

                    goto go_on;
                }

                /* 查找呼叫源和号码的对应关系，如果匹配上某一呼叫源，就选择特定号码 */
                ulRet = sc_caller_setting_select_number(pstSCB->ulCustomID, ulAgentID, SC_SRC_CALLER_TYPE_AGENT, szNumber, SC_TEL_NUMBER_LENGTH);
                if (ulRet != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                    sc_logr_info(SC_DIALER, "CustomID(%u) get caller number FAIL by agnet(%u)", pstSCB->ulCustomID, ulAgentID);

                    goto go_on;
                }

                sc_logr_info(SC_DIALER, "CustomID(%u) get caller number(%s) SUCC by agnet(%u)", pstSCB->ulCustomID, szNumber, ulAgentID);
                dos_strncpy(pstSCB->szCallerNum, szNumber, SC_TEL_NUMBER_LENGTH);
                pstSCB->szCallerNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';
            }

go_on:
            /* 路由前号码变换 主叫 */
            if (sc_ep_num_transform(pstSCB, 0, SC_NUM_TRANSFORM_TIMING_BEFORE, SC_NUM_TRANSFORM_SELECT_CALLER) != DOS_SUCC)
            {
                 DOS_ASSERT(0);

                 goto free_res;
            }

            /* 被叫号码 */
            if (sc_ep_num_transform(pstSCB, 0, SC_NUM_TRANSFORM_TIMING_BEFORE, SC_NUM_TRANSFORM_SELECT_CALLEE) != DOS_SUCC)
            {
                 DOS_ASSERT(0);

                 goto free_res;
            }

            if (sc_dialer_make_call2pstn(pstSCB, pstSCB->ucMainService) != DOS_SUCC)
            {
                DOS_ASSERT(0);
            }

free_res:
            /* SCB是预分配的，所以这里只需要把队列节点释放一下就好 */
            pstListNode->pstSCB = NULL;
            pstListNode->stList.next = NULL;
            pstListNode->stList.prev = NULL;
            dos_dmem_free(pstListNode);
            pstListNode = NULL;
        }
    }

    return NULL;
}

/*
 * 函数: U32 sc_dialer_add_call(SC_SCB_ST *pstSCB)
 * 功能: 向拨号模块添加一个呼叫
 * 参数:
 *      SC_SCB_ST *pstSCB: 呼叫控制块
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_dialer_add_call(SC_SCB_ST *pstSCB)
{
    SC_CALL_QUEUE_NODE_ST *pstNode;
    SC_TRACE_IN((U64)pstSCB, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_call_trace(pstSCB, "Dialer recv call request.");

    pstNode = (SC_CALL_QUEUE_NODE_ST *)dos_dmem_alloc(sizeof(SC_CALL_QUEUE_NODE_ST));
    if (!pstNode)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstNode->pstSCB = pstSCB;
    pthread_mutex_lock(&g_pstDialerHandle->mutexCallQueue);
    dos_list_add_tail(&g_pstDialerHandle->stCallList, &(pstNode->stList));

    sc_call_trace(pstSCB, "Call request has been accepted by the dialer.");

    g_pstDialerHandle->ulCallCnt++;
    pthread_mutex_unlock(&g_pstDialerHandle->mutexCallQueue);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/*
 * 函数: U32 sc_dialer_init()
 * 功能: 初始化拨号模块
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_dialer_init()
{
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstDialerHandle = (SC_DIALER_HANDLE_ST *)dos_dmem_alloc(sizeof(SC_DIALER_HANDLE_ST));
    if (!g_pstDialerHandle)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    g_pstDialerHandle->pszCMDBuff = (S8 *)dos_dmem_alloc(SC_ESL_CMD_BUFF_LEN);
    if (!g_pstDialerHandle->pszCMDBuff)
    {
        DOS_ASSERT(0);

        dos_dmem_free(g_pstDialerHandle);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_memzero(g_pstDialerHandle, sizeof(SC_DIALER_HANDLE_ST));

    pthread_mutex_init(&g_pstDialerHandle->mutexCallQueue, NULL);
    pthread_cond_init(&g_pstDialerHandle->condCallQueue, NULL);
    dos_list_init(&g_pstDialerHandle->stCallList);
    g_pstDialerHandle->blIsESLRunning = DOS_FALSE;
    g_pstDialerHandle->blIsWaitingExit = DOS_FALSE;
    g_pstDialerHandle->ulCallCnt = 0;

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/*
 * 函数: U32 sc_dialer_start()
 * 功能: 启动拨号模块
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_dialer_start()
{
    SC_TRACE_IN(0, 0, 0, 0);
    if (pthread_create(&g_pstDialerHandle->pthID, NULL, sc_dialer_runtime, NULL) < 0)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/*
 * 函数: U32 sc_dialer_shutdown()
 * 功能: 停止拨号模块
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_dialer_shutdown()
{
    SC_TRACE_IN(0, 0, 0, 0);

    pthread_mutex_lock(&g_pstDialerHandle->mutexCallQueue);
    g_pstDialerHandle->blIsWaitingExit = DOS_TRUE;
    pthread_mutex_unlock(&g_pstDialerHandle->mutexCallQueue);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


