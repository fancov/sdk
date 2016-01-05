/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_event_process.c
 *
 *  创建时间: 2015年1月5日16:18:41
 *  作    者: Larry
 *  描    述: 处理FS核心发过来的各种事件
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#include <dos.h>
#include <esl.h>
#include <sys/time.h>
#include <pthread.h>
#include <bs_pub.h>
#include <libcurl/curl.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"
#include "sc_ep.h"
#include "sc_acd_def.h"
#include "sc_http_api.h"
#include "sc_pub.h"
#include "sc_db.h"
#include "sc_publish.h"


/* 应用外部变量 */
extern DB_HANDLE_ST         *g_pstSCDBHandle;

extern SC_TASK_MNGT_ST      *g_pstTaskMngtInfo;

/* ESL 句柄维护 */
SC_EP_HANDLE_ST          *g_pstHandle = NULL;
pthread_mutex_t          g_mutexEventList = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t           g_condEventList = PTHREAD_COND_INITIALIZER;

/* 事件队列 REFER TO SC_EP_EVENT_NODE_ST */
DLL_S                    g_stEventList;

/* 号码变换数据链表 REFER TO SC_NUM_TRANSFORM_NODE_ST */
DLL_S                    g_stNumTransformList;
pthread_mutex_t          g_mutexNumTransformList = PTHREAD_MUTEX_INITIALIZER;

/* 路由数据链表 REFER TO SC_ROUTE_NODE_ST */
DLL_S                    g_stRouteList;
pthread_mutex_t          g_mutexRouteList = PTHREAD_MUTEX_INITIALIZER;

/* 网关列表 refer to SC_GW_NODE_ST (使用HASH) */
HASH_TABLE_S             *g_pstHashGW = NULL;
pthread_mutex_t          g_mutexHashGW = PTHREAD_MUTEX_INITIALIZER;

/* 网关组列表， refer to SC_GW_GRP_NODE_ST (使用HASH) */
HASH_TABLE_S             *g_pstHashGWGrp = NULL;
pthread_mutex_t          g_mutexHashGWGrp = PTHREAD_MUTEX_INITIALIZER;

/* TT号和EIX对应关系表 */
HASH_TABLE_S             *g_pstHashTTNumber = NULL;
pthread_mutex_t          g_mutexHashTTNumber = PTHREAD_MUTEX_INITIALIZER;/* 主叫号码列表 ，参照 SC_CALLER_QUERY_NODE_ST */
HASH_TABLE_S             *g_pstHashCaller = NULL;
pthread_mutex_t          g_mutexHashCaller = PTHREAD_MUTEX_INITIALIZER;

/* 主叫号码组列表，参照SC_CALLER_GRP_NODE_ST */
HASH_TABLE_S             *g_pstHashCallerGrp = NULL;
pthread_mutex_t          g_mutexHashCallerGrp = PTHREAD_MUTEX_INITIALIZER;


/* 主叫号码设定列表，参照SC_CALLER_SETTING_ST */
HASH_TABLE_S             *g_pstHashCallerSetting = NULL;
pthread_mutex_t          g_mutexHashCallerSetting = PTHREAD_MUTEX_INITIALIZER;

/*
 * 网关组和网关内存中的结构:
 * 使用两个hash表存放网关和中网关组
 * 每个网关组节点中使用双向链表将属于当前网关组的网关存储起来，
 * 而每一双向链表节点的数据域是存储网关hash表hash节点的数据域
 */

/* SIP账户HASH表 REFER TO SC_USER_ID_NODE_ST */
HASH_TABLE_S             *g_pstHashSIPUserID  = NULL;
pthread_mutex_t          g_mutexHashSIPUserID = PTHREAD_MUTEX_INITIALIZER;

/* DID号码hash表 REFER TO SC_DID_NODE_ST */
HASH_TABLE_S             *g_pstHashDIDNum  = NULL;
pthread_mutex_t          g_mutexHashDIDNum = PTHREAD_MUTEX_INITIALIZER;

/* 黑名单HASH表 */
HASH_TABLE_S             *g_pstHashBlackList  = NULL;
pthread_mutex_t          g_mutexHashBlackList = PTHREAD_MUTEX_INITIALIZER;

/* 企业链表 */
DLL_S                    g_stCustomerList;
pthread_mutex_t          g_mutexCustomerList = PTHREAD_MUTEX_INITIALIZER;

/* 主叫号码限制hash表 refer to SC_NUMBER_LMT_NODE_ST */
HASH_TABLE_S             *g_pstHashNumberlmt = NULL;
pthread_mutex_t          g_mutexHashNumberlmt = PTHREAD_MUTEX_INITIALIZER;

/* 业务控制 */
HASH_TABLE_S             *g_pstHashServCtrl = NULL;
pthread_mutex_t          g_mutexHashServCtrl = PTHREAD_MUTEX_INITIALIZER;

SC_EP_TASK_CB            g_astEPTaskList[SC_EP_TASK_NUM];


U32                      g_ulCPS                  = SC_MAX_CALL_PRE_SEC;
U32                      g_ulMaxConcurrency4Task  = SC_MAX_CALL / 3;

extern U32 py_exec_func(const char * pszModule,const char * pszFunc,const char * pszPyFormat,...);
SC_EP_MSG_STAT_ST        g_astEPMsgStat[2];


U32 sc_ep_calltask_result(SC_SCB_ST *pstSCB, U32 ulSIPRspCode)
{
    SC_DB_MSG_CALL_RESULT_ST *pstCallResult = NULL;
    SC_SCB_ST                *pstSCB1       = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (0 == pstSCB->ulTaskID || U32_BUTT == pstSCB->ulTaskID)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstCallResult = dos_dmem_alloc(sizeof(SC_DB_MSG_CALL_RESULT_ST));
    if (DOS_ADDR_INVALID(pstCallResult))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_logr_debug(pstSCB, SC_ESL, "Analysis call result for task: %u, SIP Code:%u(%u)", pstSCB->ulTaskID, ulSIPRspCode, pstSCB->usTerminationCause);

    if (0 == ulSIPRspCode)
    {
        ulSIPRspCode = sc_ep_transform_errcode_from_sc2sip(pstSCB->usTerminationCause);
    }

    pstSCB1 = sc_scb_get(pstSCB->usOtherSCBNo);

    dos_memzero(pstCallResult, sizeof(SC_DB_MSG_CALL_RESULT_ST));
    pstCallResult->ulCustomerID = pstSCB->ulCustomID; /* 客户ID,要求全数字,不超过10位,最高位小于4 */

    /* 坐席ID,要求全数字,不超过10位,最高位小于4 */
    if (U32_BUTT != pstSCB->ulAgentID)
    {
        pstCallResult->ulAgentID = pstSCB->ulAgentID;
    }
    else
    {
        pstCallResult->ulAgentID = 0;
    }
    pstCallResult->ulTaskID = pstSCB->ulTaskID;       /* 任务ID,要求全数字,不超过10位,最高位小于4 */

    /* 坐席号码(工号) */
    if ('\0' != pstSCB->szSiteNum[0])
    {
        dos_snprintf(pstCallResult->szAgentNum, sizeof(pstCallResult->szAgentNum), "%s", pstSCB->szSiteNum);
    }

    /* 主叫号码 */
    if ('\0' != pstSCB->szCallerNum[0])
    {
        dos_snprintf(pstCallResult->szCaller, sizeof(pstCallResult->szCaller), "%s", pstSCB->szCallerNum);
    }

    /* 被叫号码 */
    if ('\0' != pstSCB->szCalleeNum[0])
    {
        dos_snprintf(pstCallResult->szCallee, sizeof(pstCallResult->szCallee), "%s", pstSCB->szCalleeNum);
    }

    if (pstSCB->pstExtraData)
    {
        /* 接续时长:从发起呼叫到收到振铃 */
        if (0 == pstSCB->pstExtraData->ulRingTimeStamp || 0 == pstSCB->pstExtraData->ulStartTimeStamp)
        {
            pstCallResult->ulPDDLen = 0;
        }
        else
        {
            pstCallResult->ulPDDLen = pstSCB->pstExtraData->ulRingTimeStamp - pstSCB->pstExtraData->ulStartTimeStamp;
        }
        pstCallResult->ulRingTime = pstSCB->pstExtraData->ulRingTimeStamp;                 /* 振铃时长,单位:秒 */
        pstCallResult->ulAnswerTimeStamp = pstSCB->pstExtraData->ulStartTimeStamp;          /* 应答时间戳 */
        pstCallResult->ulFirstDTMFTime = pstSCB->ulFirstDTMFTime;            /* 第一个二次拨号时间,单位:秒 */
        pstCallResult->ulIVRFinishTime = pstSCB->ulIVRFinishTime;            /* IVR放音完成时间,单位:秒 */

        /* 呼叫时长,单位:秒 */
        if (0 == pstSCB->pstExtraData->ulByeTimeStamp || 0 == pstSCB->pstExtraData->ulAnswerTimeStamp)
        {
            pstCallResult->ulTimeLen = 0;
        }
        else
        {
            pstCallResult->ulTimeLen = pstSCB->pstExtraData->ulByeTimeStamp - pstSCB->pstExtraData->ulAnswerTimeStamp;
        }
    }

    pstCallResult->ulWaitAgentTime = pstSCB->bIsInQueue ? time(NULL) - pstSCB->ulInQueueTime : pstSCB->ulInQueueTime;            /* 等待坐席接听时间,单位:秒 */
    pstCallResult->ulHoldCnt = pstSCB->usHoldCnt;                  /* 保持次数 */
    pstCallResult->ulHoldTimeLen = pstSCB->usHoldTotalTime;              /* 保持总时长,单位:秒 */
    pstCallResult->usTerminateCause = pstSCB->usTerminationCause;           /* 终止原因 */
    pstCallResult->ucReleasePart = pstSCB->ucReleasePart;

    sc_logr_debug(pstSCB, SC_ESL, "Start get call result. Release: %d, Terminate Cause: %d(%d), "
                          "In queue: %d, DTMF Time: %u, IVR Finish Time: %u, Other leg: %d, Status: %d"
                        , pstSCB->bIsInQueue, pstCallResult->ulFirstDTMFTime
                        , pstCallResult->ulIVRFinishTime
                        , pstSCB1 ? pstSCB1->usSCBNo : -1
                        , pstSCB1 ? pstSCB1->ucStatus : SC_SCB_RELEASE);

    pstCallResult->ulResult = CC_RST_BUTT;

    if (CC_ERR_SIP_SUCC == ulSIPRspCode
        || CC_ERR_NORMAL_CLEAR == ulSIPRspCode)
    {
        /* 坐席全忙 */
        if (pstSCB->bIsInQueue)
        {
            pstCallResult->ulResult = CC_RST_AGNET_BUSY;
            goto proc_finished;
        }

        /*有可能放音确实没有结束，客户就按键了,所有应该优先处理 */
        if (pstCallResult->ulFirstDTMFTime
            && DOS_ADDR_INVALID(pstSCB1))
        {
            pstCallResult->ulResult = CC_RST_HANGUP_AFTER_KEY;
            goto proc_finished;
        }

        /* 播放语音时挂断 */
        if (0 == pstCallResult->ulIVRFinishTime)
        {
            pstCallResult->ulResult = CC_RST_HANGUP_WHILE_IVR;
            goto proc_finished;
        }

        /* 放音已经结束了，并且呼叫没有在队列，说明呼叫已经被转到坐席了 */
        if (pstCallResult->ulIVRFinishTime && !pstSCB->bIsInQueue)
        {
            /* ANSWER为0，说明坐席没有接通等待坐席时 挂断的 */
            if (DOS_ADDR_VALID(pstSCB1)
                && pstSCB1->pstExtraData
                && !pstSCB1->pstExtraData->ulAnswerTimeStamp)
            {
                if (SC_CALLEE == pstCallResult->ucReleasePart)
                {
                    pstCallResult->ulResult = CC_RST_AGENT_NO_ANSER;
                }
                else
                {
                    pstCallResult->ulResult = CC_RST_HANGUP_NO_ANSER;
                }

                goto proc_finished;
            }
        }

        if (SC_CALLEE == pstCallResult->ucReleasePart)
        {
            pstCallResult->ulResult = CC_RST_AGENT_HANGUP;
        }
        else
        {
            pstCallResult->ulResult = CC_RST_CUSTOMER_HANGUP;
        }
    }
    else
    {
        switch (ulSIPRspCode)
        {

            case CC_ERR_SIP_NOT_FOUND:
                pstCallResult->ulResult = CC_RST_NOT_FOUND;
                break;

            case CC_ERR_SIP_TEMPORARILY_UNAVAILABLE:
                pstCallResult->ulResult = CC_RST_REJECTED;
                break;

            case CC_ERR_SIP_BUSY_HERE:
                pstCallResult->ulResult = CC_RST_BUSY;
                break;

            case CC_ERR_SIP_REQUEST_TIMEOUT:
            case CC_ERR_SIP_REQUEST_TERMINATED:
                pstCallResult->ulResult = CC_RST_NO_ANSWER;
                break;

            default:
                pstCallResult->ulResult = CC_RST_CONNECT_FAIL;
                break;
        }
    }

proc_finished:

    if (CC_RST_BUTT == pstCallResult->ulResult)
    {
        pstCallResult->ulResult = CC_RST_CONNECT_FAIL;
    }

    sc_logr_debug(pstSCB, SC_ESL, "Call result: %u. Release: %d, Terminate Cause: %d(%d), "
                          "In queue: %d, DTMF Time: %u, IVR Finish Time: %u, Other leg: %d, Status: %d"
                        , pstCallResult->ulResult, pstSCB->bIsInQueue, pstCallResult->ulFirstDTMFTime
                        , pstCallResult->ulIVRFinishTime
                        , pstSCB1 ? pstSCB1->usSCBNo : -1
                        , pstSCB1 ? pstSCB1->ucStatus : SC_SCB_RELEASE);


    pstCallResult->stMsgTag.ulMsgType = SC_MSG_SAVE_CALL_RESULT;
    return sc_send_msg2db((SC_DB_MSG_TAG_ST *)pstCallResult);
}

U32 sc_ep_get_channel_id(SC_ACD_AGENT_INFO_ST *pstAgentInfo, S8 *pszChannel, U32 ulLen)
{
    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pszChannel) || 0 == ulLen)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_snprintf(pszChannel, ulLen, "%u_%s"
                , pstAgentInfo->ulSiteID
                , pstAgentInfo->szEmpNo);

    return DOS_SUCC;
}


U32 sc_ep_agent_status_get(SC_ACD_AGENT_INFO_ST *pstAgentInfo)
{
    S8 szJson[256]        = { 0, };
    S8 szURL[256]         = { 0, };
    S8 szChannel[128]     = { 0, };
    S32 ulPAPIPort = -1;

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (sc_ep_get_channel_id(pstAgentInfo, szChannel, sizeof(szChannel)) != DOS_SUCC)
    {
        sc_logr_info(NULL, SC_ESL, "Get channel ID fail for agent: %u", pstAgentInfo->ulSiteID);
        return DOS_FAIL;
    }

    ulPAPIPort = config_hb_get_papi_port();
    if (ulPAPIPort <= 0)
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost/pub?id=%s", szChannel);
    }
    else
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost:%d/pub?id=%s", ulPAPIPort, szChannel);
    }

    dos_snprintf(szJson, sizeof(szJson)
                    , "{\\\"type\\\":\\\"%u\\\",\\\"body\\\":{\\\"type\\\":\\\"%d\\\",\\\"work_status\\\":\\\"%u\\\",\\\"is_signin\\\":\\\"%u\\\"}}"
                    , ACD_MSG_TYPE_QUERY
                    , ACD_MSG_TYPE_QUERY_STATUS
                    , pstAgentInfo->ucStatus
                    , pstAgentInfo->bConnected ? 1 : 0);


    return sc_pub_send_msg(szURL, szJson, SC_PUB_TYPE_STATUS, NULL);
}

U32 sc_ep_agent_status_notify(SC_ACD_AGENT_INFO_ST *pstAgentInfo, U32 ulStatus)
{
    S8 szURL[256]         = { 0, };
    S8 szData[512]        = { 0, };
    S8 szChannel[128]     = { 0, };
    S32 ulPAPIPort        = -1;

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (sc_ep_get_channel_id(pstAgentInfo, szChannel, sizeof(szChannel)) != DOS_SUCC)
    {
        sc_logr_info(NULL, SC_ESL, "Get channel ID fail for agent: %u", pstAgentInfo->ulSiteID);
        return DOS_FAIL;
    }

    ulPAPIPort = config_hb_get_papi_port();
    if (ulPAPIPort <= 0)
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost/pub?id=%s", szChannel);
    }
    else
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost:%d/pub?id=%s", ulPAPIPort, szChannel);
    }


    /* 格式中引号前面需要添加"\",提供给push stream做转义用 */
    dos_snprintf(szData, sizeof(szData), "{\\\"type\\\":\\\"1\\\",\\\"body\\\":{\\\"type\\\":\\\"%u\\\",\\\"result\\\":\\\"0\\\"}}", ulStatus);

    return sc_pub_send_msg(szURL, szData, SC_PUB_TYPE_STATUS, NULL);
}

/**
 * 函数: sc_ep_call_notify
 * 功能: 通知坐席弹屏
 * 参数:
 *    SC_ACD_AGENT_INFO_ST *pstAgentInfo, S8 *szCaller
 * 返回值:
 *    成功返回DOS_SUCC， 否则返回DOS_FAIL
 */
U32 sc_ep_call_notify(SC_ACD_AGENT_INFO_ST *pstAgentInfo, S8 *szCaller)
{
    S8 szURL[256]      = { 0, };
    S8 szData[512]     = { 0, };
    S8 szChannel[128]  = { 0, };
    S32 ulPAPIPort     = -1;

    if (DOS_ADDR_INVALID(pstAgentInfo)
        || DOS_ADDR_INVALID(szCaller))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (sc_ep_get_channel_id(pstAgentInfo, szChannel, sizeof(szChannel)) != DOS_SUCC)
    {
        sc_logr_info(NULL, SC_ESL, "Get channel ID fail for agent: %u", pstAgentInfo->ulSiteID);
        return DOS_FAIL;
    }

    ulPAPIPort = config_hb_get_papi_port();
    if (ulPAPIPort <= 0)
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost/pub?id=%s", szChannel);
    }
    else
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost:%d/pub?id=%s", ulPAPIPort, szChannel);
    }


    /* 格式中引号前面需要添加"\",提供给push stream做转义用 */
    dos_snprintf(szData, sizeof(szData), "{\\\"type\\\":\\\"0\\\",\\\"body\\\":{\\\"number\\\":\\\"%s\\\"}}", szCaller);

    return sc_pub_send_msg(szURL, szData, SC_PUB_TYPE_STATUS, NULL);
}

U32 sc_ep_record(SC_SCB_ST *pstSCBRecord)
{
    S8 szFilePath[512] = { 0 };
    S8 szAPPParam[512] = { 0 };

    if (DOS_ADDR_INVALID(pstSCBRecord))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }


    if (sc_check_server_ctrl(pstSCBRecord->ulCustomID
                            , SC_SERV_RECORDING
                            , SC_SRV_CTRL_ATTR_INVLID
                            , U32_BUTT
                            , SC_SRV_CTRL_ATTR_INVLID
                            , U32_BUTT))
    {
        sc_logr_debug(pstSCBRecord, SC_ESL, "Recording service is under control, reject recoding request. Customer: %u", pstSCBRecord->ulCustomID);
        return DOS_SUCC;
    }

    SC_SCB_SET_SERVICE(pstSCBRecord, SC_SERV_RECORDING);
    sc_get_record_file_path(szFilePath, sizeof(szFilePath), pstSCBRecord->ulCustomID, pstSCBRecord->szSiteNum, pstSCBRecord->szCallerNum, pstSCBRecord->szCalleeNum);
    pthread_mutex_lock(&pstSCBRecord->mutexSCBLock);
    pstSCBRecord->pszRecordFile = dos_dmem_alloc(dos_strlen(szFilePath) + 1);
    if (DOS_ADDR_INVALID(pstSCBRecord->pszRecordFile))
    {
        DOS_ASSERT(0);
        pthread_mutex_unlock(&pstSCBRecord->mutexSCBLock);

        return DOS_FAIL;
    }

    dos_strncpy(pstSCBRecord->pszRecordFile, szFilePath, dos_strlen(szFilePath) + 1);
    pstSCBRecord->pszRecordFile[dos_strlen(szFilePath)] = '\0';

    dos_snprintf(szAPPParam, sizeof(szAPPParam)
                    , "bgapi uuid_record %s start %s/%s\r\n"
                    , pstSCBRecord->bRecord ? pstSCBRecord->szUUID : pstSCBRecord->szUUID
                    , SC_RECORD_FILE_PATH
                    , szFilePath);
    sc_ep_esl_execute_cmd(szAPPParam);
    pstSCBRecord->bIsSendRecordCdr = DOS_TRUE;

    pthread_mutex_unlock(&pstSCBRecord->mutexSCBLock);

    return DOS_SUCC;
}


/**
 * 函数: sc_ep_call_notify
 * 功能: BS的错误码，转换为SC中的错误码
 * 参数:
 *    void *pszBffer, S32 lSize, S32 lBlock, void *pArg
 * 返回值:
 *    成功返回参数lBlock， 否则返回0
 */

U16 sc_ep_transform_errcode_from_bs2sc(U8 ucErrcode)
{
    U16 usErrcodeSC = 0;

    switch (ucErrcode)
    {
        case BS_ERR_SUCC:
            usErrcodeSC = CC_ERR_NORMAL_CLEAR;
            break;
        case BS_ERR_NOT_EXIST:
            usErrcodeSC = CC_ERR_BS_NOT_EXIST;
            break;
        case BS_ERR_EXPIRE:
            usErrcodeSC = CC_ERR_BS_EXPIRE;
            break;
        case BS_ERR_FROZEN:
            usErrcodeSC = CC_ERR_BS_FROZEN;
            break;
        case BS_ERR_LACK_FEE:
            usErrcodeSC = CC_ERR_BS_LACK_FEE;
            break;
        case BS_ERR_PASSWORD:
            usErrcodeSC = CC_ERR_BS_PASSWORD;
            break;
        case BS_ERR_RESTRICT:
            usErrcodeSC = CC_ERR_BS_RESTRICT;
            break;
        case BS_ERR_OVER_LIMIT:
            usErrcodeSC = CC_ERR_BS_OVER_LIMIT;
            break;
        case BS_ERR_TIMEOUT:
            usErrcodeSC = CC_ERR_BS_TIMEOUT;
            break;
        case BS_ERR_LINK_DOWN:
            usErrcodeSC = CC_ERR_BS_LINK_DOWN;
            break;
        case BS_ERR_SYSTEM:
            usErrcodeSC = CC_ERR_BS_SYSTEM;
            break;
        case BS_ERR_MAINTAIN:
            usErrcodeSC = CC_ERR_BS_MAINTAIN;
            break;
        case BS_ERR_DATA_ABNORMAL:
            usErrcodeSC = CC_ERR_BS_DATA_ABNORMAL;
            break;
        case BS_ERR_PARAM_ERR:
            usErrcodeSC = CC_ERR_BS_PARAM_ERR;
            break;
        case BS_ERR_NOT_MATCH:
            usErrcodeSC = CC_ERR_BS_NOT_MATCH;
            break;
        default:
            DOS_ASSERT(0);
            usErrcodeSC = CC_ERR_NO_REASON;
            break;
    }

    return usErrcodeSC;
}

U16 sc_ep_transform_errcode_from_sc2sip(U32 ulErrcode)
{
    U16 usErrcodeSC = CC_ERR_SIP_UNDECIPHERABLE;

    if (ulErrcode >= CC_ERR_BUTT)
    {
        DOS_ASSERT(0);
        return usErrcodeSC;
    }

    if (ulErrcode < 1000 && ulErrcode > 99)
    {
        /* 1000以下为sip错误码，不需要转换 */
        return ulErrcode;
    }

    switch (ulErrcode)
    {
        case CC_ERR_NORMAL_CLEAR:
            usErrcodeSC = CC_ERR_SIP_SUCC;
            break;
        case CC_ERR_NO_REASON:
            usErrcodeSC = CC_ERR_SIP_BUSY_EVERYWHERE;
            break;
        case CC_ERR_SC_SERV_NOT_EXIST:
        case CC_ERR_SC_NO_SERV_RIGHTS:
        case CC_ERR_SC_USER_DOES_NOT_EXIST:
        case CC_ERR_SC_CUSTOMERS_NOT_EXIST:
            usErrcodeSC = CC_ERR_SIP_FORBIDDEN;
            break;
        case CC_ERR_SC_USER_OFFLINE:
        case CC_ERR_SC_USER_HAS_BEEN_LEFT:
        case CC_ERR_SC_PERIOD_EXCEED:
        case CC_ERR_SC_RESOURCE_EXCEED:
            usErrcodeSC = CC_ERR_SIP_TEMPORARILY_UNAVAILABLE;
            break;
        case CC_ERR_SC_USER_BUSY:
            usErrcodeSC = CC_ERR_SIP_BUSY_HERE;
            break;
        case CC_ERR_SC_CB_ALLOC_FAIL:
        case CC_ERR_SC_MEMORY_ALLOC_FAIL:
            usErrcodeSC = CC_ERR_SIP_INTERNAL_SERVER_ERROR;
            break;
        case CC_ERR_SC_IN_BLACKLIST:
        case CC_ERR_SC_CALLER_NUMBER_ILLEGAL:
        case CC_ERR_SC_CALLEE_NUMBER_ILLEGAL:
            usErrcodeSC = CC_ERR_SIP_NOT_FOUND;
            break;
        case CC_ERR_SC_NO_ROUTE:
        case CC_ERR_SC_NO_TRUNK:
            break;
        case CC_ERR_SC_MESSAGE_TIMEOUT:
        case CC_ERR_SC_AUTH_TIMEOUT:
        case CC_ERR_SC_QUERY_TIMEOUT:
            usErrcodeSC = CC_ERR_SIP_REQUEST_TIMEOUT;
            break;
        case CC_ERR_SC_CONFIG_ERR:
        case CC_ERR_SC_MESSAGE_PARAM_ERR:
        case CC_ERR_SC_MESSAGE_SENT_ERR:
        case CC_ERR_SC_MESSAGE_RECV_ERR:
        case CC_ERR_SC_CLEAR_FORCE:
        case CC_ERR_SC_SYSTEM_ABNORMAL:
        case CC_ERR_SC_SYSTEM_BUSY:
        case CC_ERR_SC_SYSTEM_MAINTAINING:
            usErrcodeSC = CC_ERR_SIP_SERVICE_UNAVAILABLE;
            break;
        case CC_ERR_BS_NOT_EXIST:
        case CC_ERR_BS_EXPIRE:
        case CC_ERR_BS_FROZEN:
        case CC_ERR_BS_LACK_FEE:
        case CC_ERR_BS_PASSWORD:
        case CC_ERR_BS_RESTRICT:
        case CC_ERR_BS_OVER_LIMIT:
        case CC_ERR_BS_TIMEOUT:
        case CC_ERR_BS_LINK_DOWN:
        case CC_ERR_BS_SYSTEM:
        case CC_ERR_BS_MAINTAIN:
        case CC_ERR_BS_DATA_ABNORMAL:
        case CC_ERR_BS_PARAM_ERR:
        case CC_ERR_BS_NOT_MATCH:
            usErrcodeSC = CC_ERR_SIP_PAYMENT_REQUIRED;
            break;
        default:
            DOS_ASSERT(0);
            break;
    }

    return usErrcodeSC;
}

/**
 * 函数: sc_ep_call_notify
 * 功能: CURL回调函数，保存数据，以便后续处理
 * 参数:
 *    void *pszBffer, S32 lSize, S32 lBlock, void *pArg
 * 返回值:
 *    成功返回参数lBlock， 否则返回0
 */
static S32 sc_ep_agent_update_recv(void *pszBffer, S32 lSize, S32 lBlock, void *pArg)
{
    IO_BUF_CB *pstIOBuffer = NULL;

    if (DOS_ADDR_INVALID(pArg))
    {
        DOS_ASSERT(0);
        return 0;
    }

    if (lSize * lBlock == 0)
    {
        return 0;
    }

    pstIOBuffer = (IO_BUF_CB *)pArg;

    if (dos_iobuf_append(pstIOBuffer, pszBffer, (U32)(lSize * lBlock)) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return 0;
    }

    return lSize * lBlock;
}


/* 解析一下格式的字符串
   : {"channel": "5_10000001", "published_messages": "0", "stored_messages": "0", "subscribers": "1"}
   同时更新坐席状态 */
U32 sc_ep_update_agent_status(S8 *pszJSONString)
{
    JSON_OBJ_ST *pstJsonArrayItem     = NULL;
    const S8    *pszAgentID           = NULL;
    S8          szJobNum[16]          = { 0 };
    U32         ulID;

    if (DOS_ADDR_INVALID(pszJSONString))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstJsonArrayItem = json_init(pszJSONString);
    if (DOS_ADDR_INVALID(pstJsonArrayItem))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 更新坐席状态 */
    pszAgentID = json_get_param(pstJsonArrayItem, "channel");
    if (DOS_ADDR_INVALID(pszAgentID))
    {
        DOS_ASSERT(0);

        json_deinit(&pstJsonArrayItem);
        return DOS_FAIL;
    }

    if (dos_sscanf(pszAgentID, "%u_%[^_]", &ulID, szJobNum) != 3)
    {
        DOS_ASSERT(0);

        json_deinit(&pstJsonArrayItem);
        return DOS_FAIL;
    }

    json_deinit(&pstJsonArrayItem);

    return sc_acd_update_agent_status(SC_ACD_SITE_ACTION_DN_QUEUE, ulID, OPERATING_TYPE_CHECK);
}

/* 解析这样一个json字符串
{"hostname": "localhost.localdomain"
    , "time": "2015-06-10T12:23:18"
    , "channels": "1"
    , "wildcard_channels": "0"
    , "uptime": "120366"
    , "infos": [{"channel": "5_10000001", "published_messages": "0", "stored_messages": "0", "subscribers": "1"}
                , {"channel": "6_10000002", "published_messages": "0", "stored_messages": "0", "subscribers": "1"}]}*/
U32 sc_ep_update_agent_req_proc(S8 *pszJSONString)
{
    S32 lIndex;
    S8 *pszAgentInfos = NULL;
    const S8 *pszAgentItem = NULL;
    JSON_OBJ_ST *pstJSONObj = NULL;
    JSON_OBJ_ST *pstJSONAgentInfos = NULL;

    pstJSONObj = json_init(pszJSONString);
    if (DOS_ADDR_INVALID(pstJSONObj))
    {
        DOS_ASSERT(0);

        sc_logr_notice(NULL, SC_ESL, "%s", "Update the agent FAIL while init the json string.");

        goto process_fail;
    }

    pszAgentInfos = (S8 *)json_get_param(pstJSONObj, "infos");
    if (DOS_ADDR_INVALID(pstJSONObj))
    {
        DOS_ASSERT(0);

        sc_logr_notice(NULL, SC_ESL, "%s", "Update the agent FAIL while get agent infos from the json string.");

        goto process_fail;
    }

    pstJSONAgentInfos = json_init(pszAgentInfos);
    if (DOS_ADDR_INVALID(pstJSONObj))
    {
        DOS_ASSERT(0);

        sc_logr_notice(NULL, SC_ESL, "%s", "Update the agent FAIL while get agent infos json array from the json string.");

        goto process_fail;
    }

    JSON_ARRAY_SCAN(lIndex, pstJSONAgentInfos, pszAgentItem)
    {
        sc_ep_update_agent_status((S8 *)pszAgentItem);
    }

    if (DOS_ADDR_INVALID(pstJSONObj))
    {
        json_deinit(&pstJSONObj);
    }

    if (DOS_ADDR_INVALID(pstJSONAgentInfos))
    {
        json_deinit(&pstJSONAgentInfos);
    }

    return DOS_SUCC;

process_fail:

    if (DOS_ADDR_INVALID(pstJSONObj))
    {
        json_deinit(&pstJSONObj);
    }

    if (DOS_ADDR_INVALID(pstJSONAgentInfos))
    {
        json_deinit(&pstJSONAgentInfos);
    }

    return DOS_FAIL;

}

U32 sc_ep_query_agent_status(CURL *curl, SC_ACD_AGENT_INFO_ST *pstAgentInfo)
{
    S8 szURL[256] = { 0, };
    U32 ulRet = 0;
    IO_BUF_CB stIOBuffer = IO_BUF_INIT;
    //U32 ulTimeout = 2;

    if (DOS_ADDR_INVALID(curl))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    dos_snprintf(szURL, sizeof(szURL)
                , "http://localhost/channels-stats?id=%u_%s_%s"
                , pstAgentInfo->ulSiteID
                , pstAgentInfo->szEmpNo
                , pstAgentInfo->szExtension);

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, szURL);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sc_ep_agent_update_recv);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (VOID *)&stIOBuffer);
    //curl_easy_setopt(curl, CURLOPT_TIMEOUT, ulTimeout);
    //curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2);
    //curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 2 * 1000);
    ulRet = curl_easy_perform(curl);
    if(CURLE_OK != ulRet)
    {
        sc_logr_notice(NULL, SC_ESL, "%s, (%s)", "CURL get agent status FAIL.", curl_easy_strerror(ulRet));

        //curl_easy_cleanup(g_pstCurlHandle);
        dos_iobuf_free(&stIOBuffer);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(stIOBuffer.pszBuffer)
        || '\0' == stIOBuffer.pszBuffer)
    {
        ulRet = DOS_FAIL;
    }
    else
    {
        stIOBuffer.pszBuffer[stIOBuffer.ulLength] = '\0';
        ulRet = DOS_SUCC;
    }

    //curl_easy_cleanup(g_pstCurlHandle);
    dos_iobuf_free(&stIOBuffer);
    return ulRet;

}

/*
U32 sc_ep_init_agent_status()
{
    S8 szURL[256] = { 0, };
    U32 ulRet = 0;
    IO_BUF_CB stIOBuffer = IO_BUF_INIT;

    if (DOS_ADDR_INVALID(g_pstCurlHandle))
    {
        g_pstCurlHandle = curl_easy_init();
        if (DOS_ADDR_INVALID(g_pstCurlHandle))
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
    }

    dos_snprintf(szURL, sizeof(szURL), "http://localhost/channels-stats?id=*");

    curl_easy_reset(g_pstCurlHandle);
    curl_easy_setopt(g_pstCurlHandle, CURLOPT_URL, szURL);
    curl_easy_setopt(g_pstCurlHandle, CURLOPT_WRITEFUNCTION, sc_ep_agent_update_recv);
    curl_easy_setopt(g_pstCurlHandle, CURLOPT_WRITEDATA, (VOID *)&stIOBuffer);
    ulRet = curl_easy_perform(g_pstCurlHandle);
    if(CURLE_OK != ulRet)
    {
        sc_logr_notice(pstSCB, SC_ESL, "%s, (%s)", "CURL get agent status FAIL.", curl_easy_strerror(ulRet));
        curl_easy_cleanup(g_pstCurlHandle);
        dos_iobuf_free(&stIOBuffer);
        return DOS_FAIL;
    }

    stIOBuffer.pszBuffer[stIOBuffer.ulLength] = '\0';

    sc_logr_notice(pstSCB, SC_ESL, "%s", "CURL get agent status SUCC.Result");
    dos_printf("%s", stIOBuffer.pszBuffer);

    ulRet = sc_ep_update_agent_req_proc((S8 *)stIOBuffer.pszBuffer);

    curl_easy_cleanup(g_pstCurlHandle);
    dos_iobuf_free(&stIOBuffer);
    return ulRet;
}

*/

BOOL sc_ep_black_regular_check(S8 *szRegularNum, S8 *szNum)
{
    S8 aszRegular[SC_TEL_NUMBER_LENGTH][SC_SINGLE_NUMBER_SRT_LEN] = {{0}};
    S8 szAllNum[SC_SINGLE_NUMBER_SRT_LEN] = "0123456789";
    S32 lIndex = 0;
    U32 ulLen  = 0;
    S8 *pszPos = szRegularNum;
    S8 ucChar;
    S32 i = 0;

    if (DOS_ADDR_INVALID(szRegularNum) || DOS_ADDR_INVALID(szNum))
    {
        /* 按照匹配不成功处理 */
        return DOS_TRUE;
    }

    while (*pszPos != '\0' && lIndex < SC_TEL_NUMBER_LENGTH)
    {
        ucChar = *pszPos;

        if (ucChar == '*')
        {
            dos_strcpy(aszRegular[lIndex], szAllNum);
        }
        else if (dos_strchr(szAllNum, ucChar) != NULL)
        {
            /* 0-9 */
            aszRegular[lIndex][0] = ucChar;
            aszRegular[lIndex][1] = '\0';
        }
        else if (ucChar == '[')
        {
            aszRegular[lIndex][0] = '\0';
            pszPos++;
            while (*pszPos != ']' && *pszPos != '\0')
            {
                if (dos_strchr(szAllNum, *pszPos) != NULL)
                {
                    /* 0-9, 先判断一下是否已经存在 */
                    if (dos_strchr(aszRegular[lIndex], *pszPos) == NULL)
                    {
                        ulLen = dos_strlen(aszRegular[lIndex]);
                        aszRegular[lIndex][ulLen] = *pszPos;
                        aszRegular[lIndex][ulLen+1] = '\0';
                    }
                }
                pszPos++;
            }

            if (*pszPos == '\0')
            {
                /* 正则表达式错误, 按照不匹配来处理 */
                return DOS_TRUE;
            }
        }
        else
        {
            /* 正则表达式错误, 按照不匹配来处理 */
            return DOS_TRUE;
        }

        pszPos++;
        lIndex++;
    }

    /* 正则表达式解析完成，比较号码是否满足正则表达式。首先比较长度，lIndex即为正则表达式的长度 */
    if (dos_strlen(szNum) != lIndex)
    {
        return DOS_TRUE;
    }

    for (i=0; i<lIndex; i++)
    {
        if (dos_strchr(aszRegular[i], *(szNum+i)) == NULL)
        {
            /* 不匹配 */
            return DOS_TRUE;
        }
    }

    return DOS_FALSE;
}

/**
 * 函数: BOOL sc_ep_black_list_check(U32 ulCustomerID, S8 *pszNum)
 * 功能: 检查pszNum是否被黑名单过滤
 * 参数:
 *       U32 ulCustomerID : 客户编号，如果是非法值，将会只对全局黑名单过滤
 *       S8 *pszNum       : 需要检查的号码
 * 返回值: 如果pszNum被黑名单过滤，将返回DOS_FALSE，否则返回TRUE
 */
BOOL sc_ep_black_list_check(U32 ulCustomerID, S8 *pszNum)
{
    U32                ulHashIndex;
    HASH_NODE_S        *pstHashNode = NULL;
    SC_BLACK_LIST_NODE *pstBlackListNode = NULL;


    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    sc_logr_debug(NULL, SC_ESL, "Check num %s is in black list for customer %u", pszNum, ulCustomerID);

    pthread_mutex_lock(&g_mutexHashBlackList);
    HASH_Scan_Table(g_pstHashBlackList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashBlackList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                DOS_ASSERT(0);
                break;
            }

            pstBlackListNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstBlackListNode))
            {
                DOS_ASSERT(0);
                continue;
            }

            if (SC_TOP_USER_ID != pstBlackListNode->ulCustomerID
                && ulCustomerID != pstBlackListNode->ulCustomerID)
            {
                continue;
            }

            if (SC_NUM_BLACK_REGULAR == pstBlackListNode->enType)
            {
                /* 正则号码 */
                if (sc_ep_black_regular_check(pstBlackListNode->szNum, pszNum) == DOS_FALSE)
                {
                    /* 匹配成功 */
                    sc_logr_debug(NULL, SC_ESL, "Num %s is matched black list item %s, id %u. (Customer:%u)"
                                , pszNum
                                , pstBlackListNode->szNum
                                , pstBlackListNode->ulID
                                , ulCustomerID);

                    pthread_mutex_unlock(&g_mutexHashBlackList);

                    return DOS_FALSE;
                }
            }
            else
            {
                if (0 == dos_strcmp(pstBlackListNode->szNum, pszNum))
                {
                    sc_logr_debug(NULL, SC_ESL, "Num %s is matched black list item %s, id %u. (Customer:%u)"
                                , pszNum
                                , pstBlackListNode->szNum
                                , pstBlackListNode->ulID
                                , ulCustomerID);

                    pthread_mutex_unlock(&g_mutexHashBlackList);
                    return DOS_FALSE;
                }
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashBlackList);

    sc_logr_debug(NULL, SC_ESL, "Num %s is not matched any black list. (Customer:%u)", pszNum, ulCustomerID);
    return DOS_TRUE;
}

/**
 * 函数: VOID sc_ep_sip_userid_init(SC_USER_ID_NODE_ST *pstUserID)
 * 功能: 初始化pstUserID所指向的User ID描述结构
 * 参数:
 *      SC_USER_ID_NODE_ST *pstUserID 需要别初始化的结构
 * 返回值: VOID
 */
VOID sc_ep_sip_userid_init(SC_USER_ID_NODE_ST *pstUserID)
{
    if (pstUserID)
    {
        dos_memzero(pstUserID, sizeof(SC_USER_ID_NODE_ST));

        pstUserID->ulCustomID = U32_BUTT;
        pstUserID->ulSIPID = U32_BUTT;
    }
}

/**
 * 函数: VOID sc_ep_did_init(SC_DID_NODE_ST *pstDIDNum)
 * 功能: 初始化pstDIDNum所指向的DID号码描述结构
 * 参数:
 *      SC_DID_NODE_ST *pstDIDNum 需要别初始化的DID号码结构
 * 返回值: VOID
 */
VOID sc_ep_did_init(SC_DID_NODE_ST *pstDIDNum)
{
    if (pstDIDNum)
    {
        dos_memzero(pstDIDNum, sizeof(SC_DID_NODE_ST));
        pstDIDNum->ulBindID = U32_BUTT;
        pstDIDNum->ulBindType = U32_BUTT;
        pstDIDNum->ulCustomID = U32_BUTT;
        pstDIDNum->ulDIDID = U32_BUTT;
        pstDIDNum->ulTimes = 0;
    }
}

VOID sc_ep_tt_init(SC_TT_NODE_ST *pstTTNumber)
{
    if (pstTTNumber)
    {
        pstTTNumber->ulID = U32_BUTT;
        pstTTNumber->szAddr[0] = '\0';
        pstTTNumber->szPrefix[0] = '\0';
        pstTTNumber->ulPort = 0;
    }
}

/**
 * 函数: VOID sc_ep_route_init(SC_ROUTE_NODE_ST *pstRoute)
 * 功能: 初始化pstRoute所指向的路由描述结构
 * 参数:
 *      SC_ROUTE_NODE_ST *pstRoute 需要别初始化的路由描述结构
 * 返回值: VOID
 */
VOID sc_ep_route_init(SC_ROUTE_NODE_ST *pstRoute)
{
    if (pstRoute)
    {
        dos_memzero(pstRoute, sizeof(SC_ROUTE_NODE_ST));
        pstRoute->ulID = U32_BUTT;

        pstRoute->ucHourBegin = 0;
        pstRoute->ucMinuteBegin = 0;
        pstRoute->ucHourEnd = 0;
        pstRoute->ucMinuteEnd = 0;
        pstRoute->bExist = DOS_FALSE;
        pstRoute->bStatus = DOS_FALSE;
    }
}

VOID sc_ep_num_transform_init(SC_NUM_TRANSFORM_NODE_ST *pstNumTransform)
{
    if (pstNumTransform)
    {
        dos_memzero(pstNumTransform, sizeof(SC_NUM_TRANSFORM_NODE_ST));
        pstNumTransform->ulID = U32_BUTT;
        pstNumTransform->bExist = DOS_FALSE;
    }
}

VOID sc_ep_customer_init(SC_CUSTOMER_NODE_ST *pstCustomer)
{
    if (pstCustomer)
    {
        dos_memzero(pstCustomer, sizeof(SC_CUSTOMER_NODE_ST));
        pstCustomer->ulID = U32_BUTT;
        pstCustomer->bExist = DOS_FALSE;
    }
}


/**
 * 函数: VOID sc_ep_caller_init(SC_CALLER_QUERY_NODE_ST  *pstCaller)
 * 功能: 初始化pstCaller所指向的主叫号码描述结构
 * 参数:
 *      SC_CALLER_QUERY_NODE_ST  *pstCaller 需要别初始化的主叫号码描述结构
 * 返回值: VOID
 */
VOID sc_ep_caller_init(SC_CALLER_QUERY_NODE_ST  *pstCaller)
{
    if (pstCaller)
    {
        dos_memzero(pstCaller, sizeof(SC_CALLER_QUERY_NODE_ST));
        /* 默认关系跟踪 */
        pstCaller->bTraceON = DOS_FALSE;
        /* 默认可用 */
        pstCaller->bValid = DOS_TRUE;
        pstCaller->ulCustomerID = 0;
        pstCaller->ulIndexInDB = 0;
        /* 号码被命中次数初始化为0 */
        pstCaller->ulTimes = 0;

        dos_memzero(pstCaller->szNumber, sizeof(pstCaller->szNumber));
    }
}

/**
 * 函数: VOID sc_ep_caller_grp_init(SC_CALLER_GRP_NODE_ST* pstCallerGrp)
 * 功能: 初始化pstCallerGrp所指向的主叫号码组描述结构
 * 参数:
 *      SC_CALLER_GRP_NODE_ST* pstCallerGrp 需要别初始化的主叫号码组描述结构
 * 返回值: VOID
 */
VOID sc_ep_caller_grp_init(SC_CALLER_GRP_NODE_ST* pstCallerGrp)
{
    if (pstCallerGrp)
    {
        dos_memzero(pstCallerGrp, sizeof(SC_CALLER_GRP_NODE_ST));

        /* 初始化为非默认组 */
        pstCallerGrp->bDefault = DOS_FALSE;

        pstCallerGrp->ulID = 0;
        pstCallerGrp->ulCustomerID = 0;
        /* 暂时保留现状 */
        pstCallerGrp->ulLastNo = 0;
        /* 默认使用顺序呼叫策略 */
        pstCallerGrp->ulPolicy = SC_CALLER_SELECT_POLICY_IN_ORDER;
        dos_memzero(pstCallerGrp->szGrpName, sizeof(pstCallerGrp->szGrpName));
        DLL_Init(&pstCallerGrp->stCallerList);

        pthread_mutex_init(&pstCallerGrp->mutexCallerList, NULL);
    }
}

VOID sc_ep_caller_setting_init(SC_CALLER_SETTING_ST *pstCallerSetting)
{
    if (pstCallerSetting)
    {
        dos_memzero(pstCallerSetting, sizeof(SC_CALLER_SETTING_ST));
        pstCallerSetting->ulID = 0;
        pstCallerSetting->ulSrcID = 0;
        pstCallerSetting->ulSrcType = 0;
        pstCallerSetting->ulDstID = 0;
        pstCallerSetting->ulDstType = 0;
        pstCallerSetting->ulCustomerID = U32_BUTT;
    }
}

/**
 * 函数: VOID sc_ep_gw_init(SC_GW_NODE_ST *pstGW)
 * 功能: 初始化pstGW所指向的网关描述结构
 * 参数:
 *      SC_GW_NODE_ST *pstGW 需要别初始化的网关描述结构
 * 返回值: VOID
 */
VOID sc_ep_gw_init(SC_GW_NODE_ST *pstGW)
{
    if (pstGW)
    {
        dos_memzero(pstGW, sizeof(SC_GW_NODE_ST));
        pstGW->ulGWID = U32_BUTT;
        pstGW->bExist = DOS_FALSE;
        pstGW->bStatus = DOS_FALSE;
    }
}

/**
 * 函数: VOID sc_ep_gw_init(SC_GW_NODE_ST *pstGW)
 * 功能: 初始化pstGW所指向的网关描述结构
 * 参数:
 *      SC_GW_NODE_ST *pstGW 需要别初始化的网关描述结构
 * 返回值: VOID
 */
VOID sc_ep_black_init(SC_BLACK_LIST_NODE *pstBlackListNode)
{
    if (pstBlackListNode)
    {
        dos_memzero(pstBlackListNode, sizeof(SC_BLACK_LIST_NODE));
        pstBlackListNode->ulID = U32_BUTT;
        pstBlackListNode->ulCustomerID = U32_BUTT;
        pstBlackListNode->enType = SC_NUM_BLACK_BUTT;
        pstBlackListNode->szNum[0] = '\0';
    }
}


/* 网关组的hash函数 */
U32 sc_ep_gw_grp_hash_func(U32 ulGWGrpID)
{
    return ulGWGrpID % SC_GW_GRP_HASH_SIZE;
}

/* 网关的hash函数 */
U32 sc_ep_gw_hash_func(U32 ulGWID)
{
    return ulGWID % SC_GW_GRP_HASH_SIZE;
}

U32 sc_ep_tt_hash_func(U32 ulID)
{
    return ulID % SC_TT_NUMBER_HASH_SIZE;
}

/**
 * 函数: U32 sc_ep_caller_hash_func(U32 ulCallerID)
 * 功能: 通过ulCallerID计算主叫号码id的哈希值
 * 参数:
 *      U32 ulCallerID 主叫号码id
 * 返回值: U32 返回hash值
 */
U32 sc_ep_caller_hash_func(U32 ulCallerID)
{
    return ulCallerID % SC_CALLER_HASH_SIZE;
}

/**
 * 函数: U32 sc_ep_caller_grp_hash_func(U32 ulGrpID)
 * 功能: 通过ulGrpID计算主叫号码组id的哈希值
 * 参数:
 *      U32 ulGrpID 主叫号码组id
 * 返回值: U32 返回hash值
 */
U32 sc_ep_caller_grp_hash_func(U32 ulGrpID)
{
    return ulGrpID % SC_CALLER_GRP_HASH_SIZE;
}

U32 sc_ep_caller_setting_hash_func(U32 ulSettingID)
{
    return ulSettingID % SC_CALLER_SETTING_HASH_SIZE;
}

/**
 * 函数: U32 sc_ep_number_lmt_hash_func(U32 ulCustomerID)
 * 功能: 根据号码pszNumber计算号码限制hash节点的hash值
 * 参数:
 *      U32 pszNumber 号码
 * 返回值: U32 返回hash值
 */
U32 sc_ep_number_lmt_hash_func(S8 *pszNumber)
{
    U32 ulIndex = 0;
    U32 ulHashIndex = 0;

    ulIndex = 0;
    for (;;)
    {
        if ('\0' == pszNumber[ulIndex])
        {
            break;
        }

        ulHashIndex += (pszNumber[ulIndex] << 3);

        ulIndex++;
    }

    return ulHashIndex % SC_NUMBER_LMT_HASH_SIZE;

}


/**
 * 函数: static U32 sc_sip_userid_hash_func(S8 *pszUserID)
 * 功能: 通过pszUserID计算SIP User IDHash表的HASH值
 * 参数:
 *      S8 *pszUserID 当前HASH节点的User ID
 * 返回值: U32 返回hash值
 */
static U32 sc_sip_userid_hash_func(S8 *pszUserID)
{
    U32 ulIndex = 0;
    U32 ulHashIndex = 0;

    ulIndex = 0;
    for (;;)
    {
        if ('\0' == pszUserID[ulIndex])
        {
            break;
        }

        ulHashIndex += (pszUserID[ulIndex] << 3);

        ulIndex++;
    }

    return ulHashIndex % SC_IP_USERID_HASH_SIZE;
}

/**
 * 函数: static U32 sc_sip_did_hash_func(S8 *pszDIDNum)
 * 功能: 通过pszDIDNum计算DID号码Hash表的HASH值
 * 参数:
 *      S8 *pszDIDNum 当前HASH节点的DID号码
 * 返回值: U32 返回hash值
 */
static U32 sc_sip_did_hash_func(S8 *pszDIDNum)
{
    U32 ulIndex = 0;
    U32 ulHashIndex = 0;

    ulIndex = 0;
    for
(;;)
    {
        if ('\0' == pszDIDNum[ulIndex])
        {
            break;
        }

        ulHashIndex += (pszDIDNum[ulIndex] << 3);

        ulIndex++;
    }

    return ulHashIndex % SC_IP_DID_HASH_SIZE;
}



/**
 * 函数: static U32 sc_black_list_hash_func(S8 *pszNum)
 * 功能: 计算黑名单hash节点的hash值
 * 参数:
 *      S8 *pszNum : 当前黑名单号码
 * 返回值: U32 返回hash值
 */
static U32 sc_ep_black_list_hash_func(U32 ulID)
{
    return ulID % SC_IP_USERID_HASH_SIZE;
}

S32 sc_ep_caller_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;
    U32 ulIndex = U32_BUTT;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulIndex = *(U32 *)pObj;
    pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstHashNode->pHandle;

    if (ulIndex == pstCaller->ulIndexInDB)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }
}

/**
 * 函数: S32 sc_ep_caller_grp_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
 * 功能: 查找关键字,主叫号码组查找函数
 * 参数:
 *      VOID *pObj, : 查找关键字，这里是号码组客户id
 *      HASH_NODE_S *pstHashNode  当前哈希
 * 返回值: 查找成功返回DOS_SUCC，否则返回DOS_FAIL.
 */
S32 sc_ep_caller_grp_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    U32  ulID = U32_BUTT;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_logr_error(NULL, SC_FUNC, "pObj:%p, pstHashNode:%p, pHandle:%p", pObj, pstHashNode, pstHashNode->pHandle);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulID = *(U32 *)pObj;
    pstCallerGrp = pstHashNode->pHandle;

    if (ulID == pstCallerGrp->ulID)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }
}

S32 sc_ep_caller_setting_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_CALLER_SETTING_ST      *pstSetting = NULL;
    U32  ulSettingID = U32_BUTT;

    if (DOS_ADDR_INVALID(pObj)
            || DOS_ADDR_INVALID(pstHashNode)
            || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    ulSettingID = *(U32 *)pObj;
    pstSetting = (SC_CALLER_SETTING_ST *)pstHashNode->pHandle;

    if (ulSettingID == pstSetting->ulID)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }
}

/* 网关组hash表查找函数 */
S32 sc_ep_gw_grp_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_GW_GRP_NODE_ST *pstGWGrpNode;

    U32 ulGWGrpIndex = 0;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulGWGrpIndex = *(U32 *)pObj;
    pstGWGrpNode = pstHashNode->pHandle;

    if (ulGWGrpIndex == pstGWGrpNode->ulGWGrpID)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }
}

/* 网关hash表查找函数 */
S32 sc_ep_gw_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_GW_NODE_ST *pstGWNode;

    U32 ulGWIndex = 0;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulGWIndex = *(U32 *)pObj;
    pstGWNode = pstHashNode->pHandle;

    if (ulGWIndex == pstGWNode->ulGWID)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }

}


/* 查找DID号码 */
S32 sc_ep_did_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    S8 *pszDIDNum = NULL;
    SC_DID_NODE_ST *pstDIDInfo = NULL;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pszDIDNum = (S8 *)pObj;
    pstDIDInfo = pstHashNode->pHandle;

    if (dos_strnicmp(pstDIDInfo->szDIDNum, pszDIDNum, sizeof(pstDIDInfo->szDIDNum)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/* 查找SIP User ID */
S32 sc_ep_sip_userid_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    S8 *pszDIDNum = NULL;
    SC_USER_ID_NODE_ST *pstSIPInfo = NULL;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pszDIDNum = (S8 *)pObj;
    pstSIPInfo = pstHashNode->pHandle;

    if (dos_strnicmp(pstSIPInfo->szUserID, pszDIDNum, sizeof(pstSIPInfo->szUserID)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/* 查找路由 */
S32 sc_ep_route_find(VOID *pKey, DLL_NODE_S *pstDLLNode)
{
    SC_ROUTE_NODE_ST *pstRouteCurrent;
    U32 ulIndex;

    if (DOS_ADDR_INVALID(pKey)
        || DOS_ADDR_INVALID(pstDLLNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulIndex = *(U32 *)pKey;
    pstRouteCurrent = pstDLLNode->pHandle;

    if (ulIndex == pstRouteCurrent->ulID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

S32 sc_ep_num_transform_find(VOID *pKey, DLL_NODE_S *pstDLLNode)
{
    SC_NUM_TRANSFORM_NODE_ST *pstTransformCurrent;
    U32 ulIndex;

    if (DOS_ADDR_INVALID(pKey)
        || DOS_ADDR_INVALID(pstDLLNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulIndex = *(U32 *)pKey;
    pstTransformCurrent = pstDLLNode->pHandle;

    if (ulIndex == pstTransformCurrent->ulID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

S32 sc_ep_customer_find(VOID *pKey, DLL_NODE_S *pstDLLNode)
{
    SC_CUSTOMER_NODE_ST *pstCustomerCurrent;
    U32 ulIndex;

    if (DOS_ADDR_INVALID(pKey)
        || DOS_ADDR_INVALID(pstDLLNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulIndex = *(U32 *)pKey;
    pstCustomerCurrent = pstDLLNode->pHandle;

    if (ulIndex == pstCustomerCurrent->ulID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/* 查找黑名单 */
S32 sc_ep_black_list_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_BLACK_LIST_NODE *pstBlackList = NULL;
    U32  ulID;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulID = *(U32 *)pObj;
    pstBlackList = pstHashNode->pHandle;

    if (ulID == pstBlackList->ulID)
    {
        return DOS_SUCC;
    }


    return DOS_FAIL;
}

S32 sc_ep_tt_list_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_TT_NODE_ST *pstTTNumber = NULL;
    U32  ulID;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulID = *(U32 *)pObj;
    pstTTNumber = pstHashNode->pHandle;

    if (ulID == pstTTNumber->ulID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

S32 sc_ep_number_lmt_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_NUMBER_LMT_NODE_ST *pstNumber = NULL;
    S8 *pszNumber = (S8 *)pObj;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        return DOS_FAIL;
    }

    pstNumber = pstHashNode->pHandle;

    if (0 == dos_strcmp(pstNumber->szPrefix, pszNumber))
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;

}

/* 根据userid 更新状态 */
U32 sc_ep_update_sip_status(S8 *szUserID, SC_SIP_STATUS_TYPE_EN enStatus, U32 *pulSipID)
{
    SC_USER_ID_NODE_ST *pstUserID   = NULL;
    HASH_NODE_S        *pstHashNode = NULL;
    U32                ulHashIndex  = U32_BUTT;

    ulHashIndex= sc_sip_userid_hash_func(szUserID);
    pthread_mutex_lock(&g_mutexHashSIPUserID);
    pstHashNode = hash_find_node(g_pstHashSIPUserID, ulHashIndex, (VOID *)szUserID, sc_ep_sip_userid_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashSIPUserID);

        return DOS_FAIL;
    }

    pstUserID = pstHashNode->pHandle;
    if (DOS_ADDR_INVALID(pstUserID))
    {
        pthread_mutex_unlock(&g_mutexHashSIPUserID);
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstUserID->enStatus = enStatus;
    switch (enStatus)
    {
        case SC_SIP_STATUS_TYPE_REGISTER:
            pstUserID->stStat.ulRegisterCnt++;
            break;
        case SC_SIP_STATUS_TYPE_UNREGISTER:
            pstUserID->stStat.ulUnregisterCnt++;
            break;
        default:
            break;
    }

    *pulSipID = pstUserID->ulSIPID;
    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return DOS_SUCC;
}

U32 sc_ep_update_trunk_status(U32 ulGateWayID, SC_TRUNK_STATE_TYPE_EN enStatus)
{
    SC_GW_NODE_ST        *pstGWNode     = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    U32 ulHashIndex = U32_BUTT;

    pthread_mutex_lock(&g_mutexHashGW);
    ulHashIndex = sc_ep_gw_hash_func(ulGateWayID);
    pstHashNode = hash_find_node(g_pstHashGW, ulHashIndex, (VOID *)&ulGateWayID, sc_ep_gw_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pthread_mutex_unlock(&g_mutexHashGW);
        return DOS_FAIL;
    }

    pstGWNode = pstHashNode->pHandle;
    if (DOS_ADDR_INVALID(pstGWNode))
    {
        pthread_mutex_unlock(&g_mutexHashGW);
        return DOS_FAIL;
    }

    pstGWNode->bStatus = enStatus;
    pthread_mutex_unlock(&g_mutexHashGW);

    return DOS_SUCC;

}

/* 删除SIP账户 */
U32 sc_ep_sip_userid_delete(S8 * pszSipID)
{
    SC_USER_ID_NODE_ST *pstUserID   = NULL;
    HASH_NODE_S        *pstHashNode = NULL;
    U32                ulHashIndex  = U32_BUTT;

    ulHashIndex= sc_sip_userid_hash_func(pszSipID);
    pstHashNode = hash_find_node(g_pstHashSIPUserID, ulHashIndex, (VOID *)pszSipID, sc_ep_sip_userid_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    hash_delete_node(g_pstHashSIPUserID, pstHashNode, ulHashIndex);
    pstUserID = pstHashNode->pHandle;
    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = NULL;

    dos_dmem_free(pstUserID);
    pstUserID = NULL;
    dos_dmem_free(pstHashNode);
    pstHashNode = NULL;

    return DOS_SUCC;
}

#if 1
U32 sc_caller_delete(U32 ulCallerID)
{
    HASH_NODE_S *pstHashNode = NULL;
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;
    U32  ulHashIndex = U32_BUTT;
    BOOL bFound = DOS_FALSE;
#if 0
    S32 lIndex;
    SC_TASK_CB_ST *pstTaskCB = NULL;
#endif

    HASH_Scan_Table(g_pstHashCaller, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashCaller, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstHashNode->pHandle;
            if (ulCallerID == pstCaller->ulIndexInDB)
            {
                bFound = DOS_TRUE;
                break;
            }
        }
        if (DOS_TRUE == bFound)
        {
            break;
        }
    }
#if 0
    /* 现在号码组的主叫号码与群呼任务的主叫号码是两份数据，同步起来比较恼火，将来解决了两份数据问题肯定要删掉
       目前先暂时将其状态置为invalid即可 */
    for (lIndex = 0; lIndex < SC_MAX_TASK_NUM; lIndex++)
    {
        pstTaskCB = &g_pstTaskMngtInfo->pstTaskList[lIndex];
        if (g_pstTaskMngtInfo->pstTaskList[lIndex].ulCustomID == pstCaller->ulCustomerID)
        {
            break;
        }
    }
    for (lIndex = 0; lIndex < SC_MAX_CALLER_NUM; lIndex++)
    {
        if (pstTaskCB->pstCallerNumQuery[lIndex].bValid
            && pstTaskCB->pstCallerNumQuery[lIndex].ulIndexInDB == pstCaller->ulIndexInDB
            && 0 == dos_strcmp(pstTaskCB->pstCallerNumQuery[lIndex].szNumber, pstCaller->szNumber))
        {
            pstTaskCB->pstCallerNumQuery[lIndex].bValid = DOS_FALSE;
            break;
        }
    }
#endif
    if (DOS_FALSE == bFound)
    {
        DOS_ASSERT(0);
        sc_logr_error(NULL, SC_FUNC, "Delete Caller FAIL.(ulID:%u)", ulCallerID);
        return DOS_FAIL;
    }
    else
    {
        hash_delete_node(g_pstHashCaller, pstHashNode, ulHashIndex);
        dos_dmem_free(pstCaller);
        pstCaller = NULL;

        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;

        sc_logr_info(NULL, SC_FUNC, "Delete Caller SUCC.(ulID:%u)", ulCallerID);

        return DOS_SUCC;
    }
}
#endif

U32 sc_caller_grp_delete(U32 ulCallerGrpID)
{
    U32  ulHashIndex = U32_BUTT;
    HASH_NODE_S  *pstHashNode = NULL;
    DLL_NODE_S *pstListNode = NULL;
    SC_CALLER_GRP_NODE_ST  *pstCallerGrp = NULL;

    ulHashIndex = sc_ep_caller_grp_hash_func(ulCallerGrpID);
    pthread_mutex_lock(&g_mutexHashCallerGrp);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulCallerGrpID, sc_ep_caller_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashCallerGrp);

        sc_logr_error(NULL, SC_FUNC, "Cannot Find Caller Grp %u.", ulCallerGrpID);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;
    pthread_mutex_lock(&pstCallerGrp->mutexCallerList);
    while (1)
    {
        if (DLL_Count(&pstCallerGrp->stCallerList) == 0)
        {
            break;
        }
        pstListNode = dll_fetch(&pstCallerGrp->stCallerList);
        if (DOS_ADDR_VALID(pstListNode->pHandle))
        {
            dos_dmem_free(pstListNode->pHandle);
            pstListNode->pHandle = NULL;
        }

        dll_delete(&pstCallerGrp->stCallerList, pstListNode);
        dos_dmem_free(pstListNode);
        pstListNode = NULL;
    }
    pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);

    hash_delete_node(g_pstHashCallerGrp, pstHashNode, ulHashIndex);
    pthread_mutex_unlock(&g_mutexHashCallerGrp);
    dos_dmem_free(pstHashNode->pHandle);
    pstHashNode->pHandle = NULL;
    dos_dmem_free(pstHashNode);
    pstHashNode = NULL;
    return DOS_SUCC;
}

U32 sc_caller_setting_delete(U32 ulSettingID)
{
    HASH_NODE_S  *pstHashNode = NULL;
    U32 ulHashIndex = U32_BUTT;

    ulHashIndex = sc_ep_caller_setting_hash_func(ulSettingID);
    pstHashNode = hash_find_node(g_pstHashCallerSetting, ulHashIndex, (VOID *)&ulSettingID, sc_ep_caller_setting_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    hash_delete_node(g_pstHashCallerSetting, pstHashNode, ulHashIndex);
    dos_dmem_free(pstHashNode->pHandle);
    pstHashNode->pHandle = NULL;
    dos_dmem_free(pstHashNode);
    pstHashNode = NULL;

    return DOS_SUCC;
}

U32 sc_transform_delete(U32 ulTransformID)
{
    DLL_NODE_S *pstListNode = NULL;

    pthread_mutex_lock(&g_mutexNumTransformList);
    pstListNode = dll_find(&g_stNumTransformList, &ulTransformID, sc_ep_num_transform_find);
    if (DOS_ADDR_INVALID(pstListNode)
        || DOS_ADDR_INVALID(pstListNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexNumTransformList);
        DOS_ASSERT(0);
        sc_logr_error(NULL, SC_FUNC, "Num Transform Delete FAIL.ID %u does not exist.", ulTransformID);
        return DOS_FAIL;
    }

    dll_delete(&g_stNumTransformList, pstListNode);
    pthread_mutex_unlock(&g_mutexNumTransformList);
    dos_dmem_free(pstListNode->pHandle);
    pstListNode->pHandle= NULL;
    dos_dmem_free(pstListNode);
    pstListNode = NULL;
    sc_logr_debug(NULL, SC_FUNC, "Delete Num Transform SUCC.(ID:%u)", ulTransformID);

    return DOS_SUCC;
}

U32 sc_customer_delete(U32 ulCustomerID)
{
    DLL_NODE_S *pstListNode = NULL;

    pthread_mutex_lock(&g_mutexCustomerList);
    pstListNode = dll_find(&g_stCustomerList, &ulCustomerID, sc_ep_customer_find);
    if (DOS_ADDR_INVALID(pstListNode)
        || DOS_ADDR_INVALID(pstListNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexCustomerList);
        DOS_ASSERT(0);
        sc_logr_error(NULL, SC_FUNC, "Num Transform Delete FAIL.ID %u does not exist.", ulCustomerID);
        return DOS_FAIL;
    }

    dll_delete(&g_stCustomerList, pstListNode);
    pthread_mutex_unlock(&g_mutexCustomerList);
    dos_dmem_free(pstListNode->pHandle);
    pstListNode->pHandle= NULL;
    dos_dmem_free(pstListNode);
    pstListNode = NULL;
    sc_logr_debug(NULL, SC_FUNC, "Delete Num Transform SUCC.(ID:%u)", ulCustomerID);

    return DOS_SUCC;
}

/* 增加一个号码与号码组的关系 */
U32 sc_caller_assign_add(U32 ulGrpID, U32 ulCallerID, U32 ulCallerType)
{
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;
    SC_DID_NODE_ST          *pstDid = NULL;
    SC_CALLER_CACHE_NODE_ST *pstCache = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    U32 ulHashIndex = U32_BUTT;

    pstCache = (SC_CALLER_CACHE_NODE_ST *)dos_dmem_alloc(sizeof(SC_CALLER_CACHE_NODE_ST));
    if (DOS_ADDR_INVALID(pstCache))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pstCache->ulType= ulCallerType;

    if (SC_NUMBER_TYPE_CFG == ulCallerType)
    {
        pstCaller = (SC_CALLER_QUERY_NODE_ST *)dos_dmem_alloc(sizeof(SC_CALLER_QUERY_NODE_ST));
        if (!pstCaller)
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstCache);
            pstCache = NULL;
            sc_logr_error(NULL, SC_FUNC, "Add Caller Assign FAIL.(ulGrpID:%u, ulCallerID:%u).", ulGrpID, ulCallerID);
            return DOS_FAIL;
        }
        pstCache->stData.pstCaller = pstCaller;
        pstCaller->ulIndexInDB = ulCallerID;
    }
    else if (SC_NUMBER_TYPE_DID == ulCallerType)
    {
        pstDid = (SC_DID_NODE_ST *)dos_dmem_alloc(sizeof(SC_DID_NODE_ST));
        if (!pstDid)
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstCache);
            pstCache = NULL;
            sc_logr_error(NULL, SC_FUNC, "Add Caller Assign FAIL.(ulGrpID:%u, ulCallerID:%u).", ulGrpID, ulCallerID);
            return DOS_FAIL;
        }
        pstCache->stData.pstDid = pstDid;
        pstDid->ulDIDID = ulCallerID;
    }

    pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (!pstHashNode)
    {
        dos_dmem_free(pstCache);
        pstCache = NULL;
        if (DOS_ADDR_VALID(pstCaller))
        {
            dos_dmem_free(pstCaller);
            pstCaller = NULL;
        }
        if (DOS_ADDR_VALID(pstDid))
        {
            dos_dmem_free(pstDid);
            pstDid = NULL;
        }
        sc_logr_error(NULL, SC_FUNC, "Add Caller Assign FAIL.(ulGrpID:%u, ulCallerID:%u).", ulGrpID, ulCallerID);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = (VOID *)pstCache;

    ulHashIndex = sc_ep_caller_grp_hash_func(ulGrpID);
    hash_add_node(g_pstHashCallerGrp, pstHashNode, ulHashIndex, NULL);

    sc_logr_info(NULL, SC_FUNC, "Add Caller Assign SUCC.(ulGrpID:%u, ulCallerID:%u).", ulGrpID, ulCallerID);

    return DOS_SUCC;
}

U32 sc_gateway_delete(U32 ulGatewayID)
{
    HASH_NODE_S   *pstHashNode = NULL;
    SC_GW_NODE_ST *pstGateway  = NULL;
    U32  ulIndex = U32_BUTT;

    ulIndex = sc_ep_gw_hash_func(ulGatewayID);
    pthread_mutex_lock(&g_mutexHashGW);
    pstHashNode = hash_find_node(g_pstHashGW, ulIndex, (VOID *)&ulGatewayID, sc_ep_gw_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashGW);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstGateway = pstHashNode->pHandle;
    pstHashNode->pHandle = NULL;

    hash_delete_node(g_pstHashGW, pstHashNode, ulIndex);

    if (pstHashNode)
    {
        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;
    }

    if (pstGateway)
    {
        dos_dmem_free(pstGateway);
        pstHashNode = NULL;
    }

    pthread_mutex_unlock(&g_mutexHashGW);

    return DOS_SUCC;
}

U32 sc_route_delete(U32 ulRouteID)
{
    DLL_NODE_S       *pstDLLNode   = NULL;
    SC_ROUTE_NODE_ST *pstRouteNode = NULL;

    pthread_mutex_lock(&g_mutexRouteList);
    pstDLLNode = dll_find(&g_stRouteList, (VOID *)&ulRouteID, sc_ep_route_find);
    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        pthread_mutex_unlock(&g_mutexRouteList);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pstRouteNode = pstDLLNode->pHandle;
    pstDLLNode->pHandle =  NULL;

    dll_delete(&g_stRouteList, pstDLLNode);

    if (pstRouteNode)
    {
        dos_dmem_free(pstRouteNode);
        pstRouteNode = NULL;
    }

    if (pstDLLNode)
    {
        dos_dmem_free(pstDLLNode);
        pstDLLNode = NULL;
    }

    pthread_mutex_unlock(&g_mutexRouteList);

    return DOS_SUCC;
}

U32 sc_gateway_grp_delete(U32 ulGwGroupID)
{
    DLL_NODE_S         *pstDLLNode     = NULL;
    HASH_NODE_S        *pstHashNode    = NULL;
    SC_GW_GRP_NODE_ST  *pstGwGroupNode = NULL;
    U32 ulIndex = U32_BUTT;

    /* 获得网关组索引 */
    ulIndex = sc_ep_gw_grp_hash_func(ulGwGroupID);

    pthread_mutex_lock(&g_mutexHashGWGrp);

    /* 查找网关组节点首地址 */
    pstHashNode = hash_find_node(g_pstHashGWGrp, ulIndex, (VOID *)&ulGwGroupID, sc_ep_gw_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashGW);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstGwGroupNode = pstHashNode->pHandle;
    pstHashNode->pHandle = NULL;

    /* 删除节点 */
    hash_delete_node(g_pstHashGWGrp, pstHashNode, ulIndex);

    if (pstGwGroupNode)
    {
        pthread_mutex_lock(&pstGwGroupNode->mutexGWList);
        while (1)
        {
            if (DLL_Count(&pstGwGroupNode->stGWList) == 0)
            {
                break;
            }

            pstDLLNode = dll_fetch(&pstGwGroupNode->stGWList);
            if (DOS_ADDR_INVALID(pstDLLNode))
            {
                DOS_ASSERT(0);
                break;
            }

            /* dll节点的数据域勿要删除 */

            DLL_Init_Node(pstDLLNode);
            dos_dmem_free(pstDLLNode);
            pstDLLNode = NULL;
        }
        pthread_mutex_unlock(&pstGwGroupNode->mutexGWList);


        pthread_mutex_destroy(&pstGwGroupNode->mutexGWList);
        dos_dmem_free(pstGwGroupNode);
        pstGwGroupNode = NULL;
    }

    if (pstHashNode)
    {
       dos_dmem_free(pstHashNode);
       pstHashNode = NULL;
    }

    pthread_mutex_unlock(&g_mutexHashGWGrp);

    return DOS_SUCC;
}

U32 sc_did_delete(U32 ulDidID)
{
    HASH_NODE_S     *pstHashNode = NULL;
    SC_DID_NODE_ST  *pstDidNode  = NULL;
    BOOL blFound = DOS_FALSE;
    U32 ulIndex = U32_BUTT;

    pthread_mutex_lock(&g_mutexHashDIDNum);
    HASH_Scan_Table(g_pstHashDIDNum, ulIndex)
    {
        HASH_Scan_Bucket(g_pstHashDIDNum, ulIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                break;
            }

            pstDidNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstDidNode))
            {
                continue;
            }

            if (ulDidID == pstDidNode->ulDIDID)
            {
                blFound = DOS_TRUE;
                break;
            }
        }

        if (blFound)
        {
            break;
        }
    }

    if (blFound)
    {
        ulIndex = sc_sip_did_hash_func(pstDidNode->szDIDNum);

        /* 删除节点 */
        hash_delete_node(g_pstHashDIDNum, pstHashNode, ulIndex);

        if (pstHashNode)
        {
            dos_dmem_free(pstHashNode);
            pstHashNode = NULL;
        }

        if (pstDidNode)
        {
           dos_dmem_free(pstDidNode);
           pstDidNode = NULL;
        }
    }

    pthread_mutex_unlock(&g_mutexHashDIDNum);

    if (blFound)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FALSE;
    }
}

U32 sc_black_list_delete(U32 ulID)
{
    HASH_NODE_S        *pstHashNode  = NULL;
    SC_BLACK_LIST_NODE *pstBlackList = NULL;
    U32  ulHashIndex = 0;

    pthread_mutex_lock(&g_mutexHashBlackList);

    HASH_Scan_Table(g_pstHashBlackList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashBlackList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstBlackList = (SC_BLACK_LIST_NODE *)pstHashNode->pHandle;
            /* 如果找到和该fileID相同，则从哈希表中删除*/
            if (pstBlackList->ulID == ulID)
            {
                hash_delete_node(g_pstHashBlackList, pstHashNode, ulHashIndex);
                dos_dmem_free(pstHashNode);
                pstHashNode = NULL;

                dos_dmem_free(pstBlackList);
                pstBlackList = NULL;
                break;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashBlackList);

    return DOS_SUCC;
}


/**
 * 函数: S32 sc_load_sip_userid_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * 功能: 加载SIP账户时数据库查询的回调函数，将数据加入SIP账户的HASH表中
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_load_sip_userid_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_USER_ID_NODE_ST *pstSIPUserIDNodeNew = NULL;
    SC_USER_ID_NODE_ST *pstSIPUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode      = NULL;
    BOOL               blProcessOK       = DOS_FALSE;
    U32                ulHashIndex       = 0;
    S32                lIndex            = 0;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSIPUserIDNodeNew = (SC_USER_ID_NODE_ST *)dos_dmem_alloc(sizeof(SC_USER_ID_NODE_ST));
    if (DOS_ADDR_INVALID(pstSIPUserIDNodeNew))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    sc_ep_sip_userid_init(pstSIPUserIDNodeNew);

    for (lIndex=0, blProcessOK = DOS_TRUE; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSIPUserIDNodeNew->ulSIPID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSIPUserIDNodeNew->ulCustomID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "userid", dos_strlen("userid")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstSIPUserIDNodeNew->szUserID, aszValues[lIndex], sizeof(pstSIPUserIDNodeNew->szUserID));
            pstSIPUserIDNodeNew->szUserID[sizeof(pstSIPUserIDNodeNew->szUserID) - 1] = '\0';
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "extension", dos_strlen("extension")))
        {
            if (DOS_ADDR_VALID(aszValues[lIndex])
                && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstSIPUserIDNodeNew->szExtension, aszValues[lIndex], sizeof(pstSIPUserIDNodeNew->szExtension));
                pstSIPUserIDNodeNew->szExtension[sizeof(pstSIPUserIDNodeNew->szExtension) - 1] = '\0';
            }
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstSIPUserIDNodeNew);
        pstSIPUserIDNodeNew = NULL;
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);

    ulHashIndex = sc_sip_userid_hash_func(pstSIPUserIDNodeNew->szUserID);
    pstHashNode = hash_find_node(g_pstHashSIPUserID, ulHashIndex, (VOID *)&pstSIPUserIDNodeNew->szUserID, sc_ep_sip_userid_hash_find);

    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstSIPUserIDNodeNew);
            pstSIPUserIDNodeNew = NULL;
            pthread_mutex_unlock(&g_mutexHashSIPUserID);
            return DOS_FAIL;
        }
/*
        sc_logr_info(pstSCB, SC_ESL, "Load SIP User. ID: %d, Customer: %d, UserID: %s, Extension: %s"
                    , pstSIPUserIDNodeNew->ulSIPID
                    , pstSIPUserIDNodeNew->ulCustomID
                    , pstSIPUserIDNodeNew->szUserID
                    , pstSIPUserIDNodeNew->szExtension);
*/

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstSIPUserIDNodeNew;

        hash_add_node(g_pstHashSIPUserID, (HASH_NODE_S *)pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        pstSIPUserIDNode = (SC_USER_ID_NODE_ST *)pstHashNode->pHandle;
        pstSIPUserIDNode->ulCustomID = pstSIPUserIDNodeNew->ulCustomID;

        dos_strncpy(pstSIPUserIDNode->szUserID, pstSIPUserIDNodeNew->szUserID, sizeof(pstSIPUserIDNode->szUserID));
        pstSIPUserIDNode->szUserID[sizeof(pstSIPUserIDNode->szUserID) - 1] = '\0';

        dos_strncpy(pstSIPUserIDNode->szExtension, pstSIPUserIDNodeNew->szExtension, sizeof(pstSIPUserIDNode->szExtension));
        pstSIPUserIDNode->szExtension[sizeof(pstSIPUserIDNode->szExtension) - 1] = '\0';

        dos_dmem_free(pstSIPUserIDNodeNew);
        pstSIPUserIDNodeNew = NULL;
    }
    //

    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_load_sip_userid()
 * 功能: 加载SIP账户数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_load_sip_userid(U32 ulIndex)
{
    S8 szSQL[1024] = {0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, extension,userid FROM tbl_sip where tbl_sip.status = 0;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, extension,userid FROM tbl_sip where tbl_sip.status = 0 AND id=%d;", ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_load_sip_userid_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(NULL, SC_ESL, "%s", "Load sip account fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 函数: S32 sc_load_black_list_cb()
 * 功能: 加载Black时数据库查询的回调函数，将数据加入黑名单hash表
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_load_black_list_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_BLACK_LIST_NODE *pstBlackListNode = NULL;
    SC_BLACK_LIST_NODE *pstBlackListTmp  = NULL;
    HASH_NODE_S        *pstHashNode      = NULL;
    BOOL               blProcessOK       = DOS_TRUE;
    S32                lIndex            = 0;
    U32                ulHashIndex       = 0;


    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstBlackListNode = dos_dmem_alloc(sizeof(SC_BLACK_LIST_NODE));
    if (DOS_ADDR_INVALID(pstBlackListNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_ep_black_init(pstBlackListNode);

    for (blProcessOK = DOS_FALSE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || dos_atoul(aszValues[lIndex], &pstBlackListNode->ulID) < 0)
            {
                blProcessOK = DOS_FALSE;

                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || dos_atoul(aszValues[lIndex], &pstBlackListNode->ulCustomerID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "file_id", dos_strlen("file_id")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || dos_atoul(aszValues[lIndex], &pstBlackListNode->ulFileID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "number", dos_strlen("number")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstBlackListNode->szNum, aszValues[lIndex], sizeof(pstBlackListNode->szNum));
            pstBlackListNode->szNum[sizeof(pstBlackListNode->szNum) - 1] = '\0';
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "type", dos_strlen("type")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || dos_atoul(aszValues[lIndex], &pstBlackListNode->enType) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }

        blProcessOK = DOS_TRUE;
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstBlackListNode);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_black_list_hash_func(pstBlackListNode->ulID);
    pstHashNode = hash_find_node(g_pstHashBlackList, ulHashIndex, (VOID *)&pstBlackListNode->ulID, sc_ep_black_list_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode ))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstBlackListNode);
            pstBlackListNode = NULL;
            return DOS_FAIL;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstBlackListNode;
        ulHashIndex = sc_ep_black_list_hash_func(pstBlackListNode->ulID);

        pthread_mutex_lock(&g_mutexHashBlackList);
        hash_add_node(g_pstHashBlackList, pstHashNode, ulHashIndex, NULL);
        pthread_mutex_unlock(&g_mutexHashBlackList);
    }
    else
    {
        pstBlackListTmp = pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstBlackListTmp))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstBlackListNode);
            pstBlackListNode = NULL;
            return DOS_FAIL;
        }

        pstBlackListTmp->enType = pstBlackListNode->enType;


        dos_strncpy(pstBlackListTmp->szNum, pstBlackListNode->szNum, sizeof(pstBlackListTmp->szNum));
        pstBlackListTmp->szNum[sizeof(pstBlackListTmp->szNum) - 1] = '\0';
        dos_dmem_free(pstBlackListNode);
        pstBlackListNode = NULL;
    }

    return DOS_SUCC;
}


/**
 * 函数: U32 sc_load_black_list(U32 ulIndex)
 * 功能: 加载黑名单数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_load_black_list(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, file_id, number, type FROM tbl_blacklist_pool;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, file_id, number, type FROM tbl_blacklist_pool WHERE file_id=%u;", ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_load_black_list_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(NULL, SC_ESL, "%s", "Load black list fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_update_black_list(U32 ulIndex)
 * 功能: 更新黑名单数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_update_black_list(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, file_id, number, type FROM tbl_blacklist_pool WHERE id=%u;", ulIndex);
    if (db_query(g_pstSCDBHandle, szSQL, sc_load_black_list_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(NULL, SC_ESL, "%s", "Load blacklist fail.");
        return DOS_FAIL;
    }
    return DOS_SUCC;
}


static S32 sc_load_tt_number_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    HASH_NODE_S        *pstHashNode      = NULL;
    SC_TT_NODE_ST      *pstSCTTNumber    = NULL;
    SC_TT_NODE_ST      *pstSCTTNumberOld = NULL;
    BOOL               blProcessOK       = DOS_FALSE;
    U32                ulHashIndex       = 0;
    S32                lIndex            = 0;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstSCTTNumber = (SC_TT_NODE_ST *)dos_dmem_alloc(sizeof(SC_TT_NODE_ST));
    if (DOS_ADDR_INVALID(pstSCTTNumber))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_ep_tt_init(pstSCTTNumber);

    for (lIndex=0, blProcessOK=DOS_TRUE; lIndex<lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSCTTNumber->ulID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "ip", dos_strlen("ip")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }
            dos_snprintf(pstSCTTNumber->szAddr, sizeof(pstSCTTNumber->szAddr), "%s", aszValues[lIndex]);
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "port", dos_strlen("port")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0]
                || dos_atoul(aszValues[lIndex], &pstSCTTNumber->ulPort) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "prefix_number", dos_strlen("prefix_number")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstSCTTNumber->szPrefix, aszValues[lIndex], sizeof(pstSCTTNumber->szPrefix));
            pstSCTTNumber->szPrefix[sizeof(pstSCTTNumber->szPrefix) - 1] = '\0';
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstSCTTNumber);
        pstSCTTNumber = NULL;
        return DOS_FALSE;
    }


    pthread_mutex_lock(&g_mutexHashTTNumber);
    ulHashIndex = sc_ep_tt_hash_func(pstSCTTNumber->ulID);
    pstHashNode = hash_find_node(g_pstHashTTNumber, ulHashIndex, &pstSCTTNumber->ulID, sc_ep_tt_list_find);
    if (DOS_ADDR_VALID(pstHashNode))
    {
        if (DOS_ADDR_VALID(pstHashNode->pHandle))
        {
            pstSCTTNumberOld = pstHashNode->pHandle;
            dos_snprintf(pstSCTTNumberOld->szAddr, sizeof(pstSCTTNumberOld->szAddr), pstSCTTNumber->szAddr);
            dos_snprintf(pstSCTTNumberOld->szPrefix, sizeof(pstSCTTNumberOld->szPrefix), pstSCTTNumber->szPrefix);
            pstSCTTNumberOld->ulPort = pstSCTTNumber->ulPort;
            dos_dmem_free(pstSCTTNumber);
            pstSCTTNumber = NULL;
        }
        else
        {
            pstHashNode->pHandle = pstSCTTNumber;
        }
    }
    else
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);

            pthread_mutex_unlock(&g_mutexHashTTNumber);
            dos_dmem_free(pstSCTTNumber);
            pstSCTTNumber = NULL;
            return DOS_FALSE;
        }

        pstHashNode->pHandle = pstSCTTNumber;

        hash_add_node(g_pstHashTTNumber, pstHashNode, ulHashIndex, NULL);
    }
    pthread_mutex_unlock(&g_mutexHashTTNumber);

    return DOS_SUCC;
}


U32 sc_load_tt_number(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, ip, port, prefix_number FROM tbl_eix;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, ip, port, prefix_number FROM tbl_eix WHERE id=%u;", ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_load_tt_number_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(NULL, SC_ESL, "%s", "Load TT number FAIL.");
        return DOS_FAIL;
    }

    return DOS_SUCC;

}

/**
 * 函数: S32 sc_load_did_number_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * 功能: 加载DID号码时数据库查询的回调函数，将数据加入DID号码的HASH表中
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_load_did_number_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_DID_NODE_ST     *pstDIDNumNode    = NULL;
    SC_DID_NODE_ST     *pstDIDNumTmp     = NULL;
    HASH_NODE_S        *pstHashNode      = NULL;
    BOOL               blProcessOK       = DOS_FALSE;
    U32                ulHashIndex       = 0;
    S32                lIndex            = 0;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstDIDNumNode = (SC_DID_NODE_ST *)dos_dmem_alloc(sizeof(SC_DID_NODE_ST));
    if (DOS_ADDR_INVALID(pstDIDNumNode))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_ep_did_init(pstDIDNumNode);

    for (lIndex=0, blProcessOK=DOS_TRUE; lIndex<lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->ulDIDID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->ulCustomID) < 0)
            {
                blProcessOK = DOS_FALSE;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "did_number", dos_strlen("did_number")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstDIDNumNode->szDIDNum, aszValues[lIndex], sizeof(pstDIDNumNode->szDIDNum));
            pstDIDNumNode->szDIDNum[sizeof(pstDIDNumNode->szDIDNum) - 1] = '\0';
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "bind_type", dos_strlen("bind_type")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->ulBindType) < 0
                || pstDIDNumNode->ulBindType >= SC_DID_BIND_TYPE_BUTT)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "bind_id", dos_strlen("bind_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->ulBindID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "status", dos_strlen("status")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->bValid) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstDIDNumNode);
        pstDIDNumNode = NULL;
        return DOS_FALSE;
    }

    pthread_mutex_lock(&g_mutexHashDIDNum);
    ulHashIndex = sc_sip_did_hash_func(pstDIDNumNode->szDIDNum);
    pstHashNode = hash_find_node(g_pstHashDIDNum, ulHashIndex, (VOID *)pstDIDNumNode->szDIDNum, sc_ep_did_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstDIDNumNode);
            pstDIDNumNode = NULL;
            pthread_mutex_unlock(&g_mutexHashDIDNum);
            return DOS_FALSE;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstDIDNumNode;
        ulHashIndex = sc_sip_did_hash_func(pstDIDNumNode->szDIDNum);

        hash_add_node(g_pstHashDIDNum, (HASH_NODE_S *)pstHashNode, ulHashIndex, NULL);

    }
    else
    {
        pstDIDNumTmp = pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstDIDNumTmp))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstDIDNumNode);
            pstDIDNumNode = NULL;

            pthread_mutex_unlock(&g_mutexHashDIDNum);
            return DOS_FAIL;
        }

        pstDIDNumTmp->ulCustomID = pstDIDNumNode->ulCustomID;
        pstDIDNumTmp->ulDIDID = pstDIDNumNode->ulDIDID;
        pstDIDNumTmp->ulBindType = pstDIDNumNode->ulBindType;
        pstDIDNumTmp->ulBindID  = pstDIDNumNode->ulBindID;
        pstDIDNumTmp->bValid = pstDIDNumNode->bValid;
        dos_strncpy(pstDIDNumTmp->szDIDNum, pstDIDNumNode->szDIDNum, sizeof(pstDIDNumTmp->szDIDNum));
        pstDIDNumTmp->szDIDNum[sizeof(pstDIDNumTmp->szDIDNum) - 1] = '\0';

        dos_dmem_free(pstDIDNumNode);
        pstDIDNumNode = NULL;
    }
    pthread_mutex_unlock(&g_mutexHashDIDNum);

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_load_did_number()
 * 功能: 加载DID号码数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_load_did_number(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, did_number, bind_type, bind_id, status FROM tbl_sipassign;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, did_number, bind_type, bind_id, status FROM tbl_sipassign where id=%u;", ulIndex);
    }

    db_query(g_pstSCDBHandle, szSQL, sc_load_did_number_cb, NULL, NULL);

    return DOS_SUCC;
}

/**
 * 函数: S32 sc_load_gateway_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * 功能: 加载网关列表数据时数据库查询的回调函数，将数据加入网关加入对于路由的列表中
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_load_gateway_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_GW_NODE_ST        *pstGWNode     = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    S8 *pszGWID = NULL, *pszDomain = NULL, *pszStatus = NULL, *pszRegister = NULL, *pszRegisterStatus = NULL;
    U32 ulID, ulStatus, ulRegisterStatus = 0;
    BOOL bIsRegister;
    U32 ulHashIndex = U32_BUTT;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pszGWID = aszValues[0];
    pszDomain = aszValues[1];
    pszStatus = aszValues[2];
    pszRegister = aszValues[3];
    pszRegisterStatus = aszValues[4];
    if (DOS_ADDR_INVALID(pszGWID)
        || DOS_ADDR_INVALID(pszDomain)
        || DOS_ADDR_INVALID(pszStatus)
        || DOS_ADDR_INVALID(pszRegister)
        || dos_atoul(pszGWID, &ulID) < 0
        || dos_atoul(pszStatus, &ulStatus) < 0
        || dos_atoul(pszRegister, &bIsRegister) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (bIsRegister)
    {
        /* 注册，获取注册的状态 */
        if (dos_atoul(pszRegisterStatus, &ulRegisterStatus) < 0)
        {
            /* 如果状态为空，则将状态置为 trying */
            ulRegisterStatus = SC_TRUNK_STATE_TYPE_TRYING;
        }
    }
    else
    {
        ulRegisterStatus = SC_TRUNK_STATE_TYPE_UNREGED;
    }

    pthread_mutex_lock(&g_mutexHashGW);
    ulHashIndex = sc_ep_gw_hash_func(ulID);
    pstHashNode = hash_find_node(g_pstHashGW, ulHashIndex, (VOID *)&ulID, sc_ep_gw_hash_find);
    /* 此过程为了将数据库里的数据全部同步到内存，bExist为true标明这些数据来自于数据库 */
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        /* 如果不存在则重新申请节点并加入内存 */
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);

            pthread_mutex_unlock(&g_mutexHashGW);
            return DOS_FAIL;
        }

        pstGWNode = dos_dmem_alloc(sizeof(SC_GW_NODE_ST));
        if (DOS_ADDR_INVALID(pstGWNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstHashNode);

            pthread_mutex_unlock(&g_mutexHashGW);
            return DOS_FAIL;
        }

        sc_ep_gw_init(pstGWNode);

        pstGWNode->ulGWID = ulID;
        if ('\0' != pszDomain[0])
        {
            dos_strncpy(pstGWNode->szGWDomain, pszDomain, sizeof(pstGWNode->szGWDomain));
            pstGWNode->szGWDomain[sizeof(pstGWNode->szGWDomain) - 1] = '\0';
        }
        else
        {
            pstGWNode->szGWDomain[0] = '\0';
        }
        pstGWNode->bStatus = ulStatus;
        pstGWNode->bRegister = bIsRegister;
        pstGWNode->ulRegisterStatus = ulRegisterStatus;

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstGWNode;
        pstGWNode->bExist = DOS_TRUE;

        ulHashIndex = sc_ep_gw_hash_func(pstGWNode->ulGWID);
        hash_add_node(g_pstHashGW, pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        pstGWNode = pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstGWNode))
        {
            DOS_ASSERT(0);

            pthread_mutex_unlock(&g_mutexHashGW);
            return DOS_FAIL;
        }

        dos_strncpy(pstGWNode->szGWDomain, pszDomain, sizeof(pstGWNode->szGWDomain));
        pstGWNode->szGWDomain[sizeof(pstGWNode->szGWDomain) - 1] = '\0';
        pstGWNode->bExist = DOS_TRUE;
        pstGWNode->bStatus = ulStatus;
        pstGWNode->bRegister = bIsRegister;
        pstGWNode->ulRegisterStatus = ulRegisterStatus;
    }
    pthread_mutex_unlock(&g_mutexHashGW);
#if INCLUDE_SERVICE_PYTHON
    /* 在这里强制生成网关和删除网关 */
    if (DOS_FALSE == pstGWNode->bStatus)
    {
        ulRet = py_exec_func("router", "del_route", "(k)", (U64)pstGWNode->ulGWID);
        if (DOS_SUCC != ulRet)
        {
            sc_logr_debug(NULL, SC_FUNC, "Delete FreeSWITCH XML file of gateway %u FAIL by status.", pstGWNode->ulGWID);
        }
        else
        {
            sc_logr_debug(NULL, SC_FUNC, "Delete FreeSWITCH XML file of gateway %u SUCC by status.", pstGWNode->ulGWID);
        }
    }
    else
    {
        ulRet = py_exec_func("router", "make_route", "(i)", pstGWNode->ulGWID);
        if (DOS_SUCC != ulRet)
        {
            sc_logr_error(NULL, SC_FUNC, "Generate FreeSWITCH XML file of gateway %u FAIL by status.", pstGWNode->ulGWID);
        }
        else
        {
            sc_logr_debug(NULL, SC_FUNC, "Generate FreeSWITCH XML file of gateway %u SUCC by status.", pstGWNode->ulGWID);
        }
    }
#endif
    return DOS_SUCC;
}

/**
 * 函数: U32 sc_load_gateway(U32 ulIndex)
 * 功能: 加载网关数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_load_gateway(U32 ulIndex)
{
    S8 szSQL[1024] = {0,};
    U32 ulRet;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id, realm, status, register, register_status FROM tbl_gateway;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id, realm, status, register, register_status FROM tbl_gateway WHERE id=%d;", ulIndex);
    }

    ulRet = db_query(g_pstSCDBHandle, szSQL, sc_load_gateway_cb, NULL, NULL);
    if (DB_ERR_SUCC != ulRet)
    {
        DOS_ASSERT(0);
        sc_logr_error(NULL, SC_FUNC, "Load gateway FAIL.(ID:%u)", ulIndex);
        return DOS_FAIL;
    }

    if (SC_INVALID_INDEX == ulIndex)
    {
        ulRet = sc_ep_esl_execute_cmd("bgapi sofia profile external rescan");
        if (ulRet != DOS_SUCC)
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
    }

    sc_logr_debug(NULL, SC_FUNC, "Load gateway SUCC.(ID:%u)", ulIndex);

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_ep_gateway_register_status_update(U32 ulGWID, SC_TRUNK_STATE_TYPE_EN enRegisterStatus)
 * 功能: 更新中继的状态
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_ep_gateway_register_status_update(U32 ulGWID, SC_TRUNK_STATE_TYPE_EN enRegisterStatus)
{
    SC_GW_NODE_ST  *pstGWNode     = NULL;
    HASH_NODE_S    *pstHashNode   = NULL;
    U32             ulHashIndex   = U32_BUTT;

    if (enRegisterStatus >= SC_TRUNK_STATE_TYPE_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_gw_hash_func(ulGWID);

    pthread_mutex_lock(&g_mutexHashGW);
    pstHashNode = hash_find_node(g_pstHashGW, ulHashIndex, (VOID *)&ulGWID, sc_ep_gw_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashGW);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstGWNode = (SC_GW_NODE_ST *)pstHashNode->pHandle;
    pstGWNode->ulRegisterStatus = enRegisterStatus;

    pthread_mutex_unlock(&g_mutexHashGW);

    return DOS_SUCC;
}

/**
 * 函数: S32 sc_load_gateway_grp_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * 功能: 加载网关组列表数据时数据库查询的回调函数
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
S32 sc_load_gateway_grp_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_GW_GRP_NODE_ST    *pstGWGrpNode  = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    S8 *pszGWGrpID = NULL;
    U32 ulID = 0;
    U32 ulHashIndex = 0;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pszGWGrpID = aszValues[0];
    if (DOS_ADDR_INVALID(pszGWGrpID)
        || dos_atoul(pszGWGrpID, &ulID) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstGWGrpNode = dos_dmem_alloc(sizeof(SC_GW_GRP_NODE_ST));
    if (DOS_ADDR_INVALID(pstGWGrpNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstHashNode);
        return DOS_FAIL;
    }

    pstGWGrpNode->ulGWGrpID = ulID;
    pstGWGrpNode->bExist = DOS_TRUE;

    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = pstGWGrpNode;
    DLL_Init(&pstGWGrpNode->stGWList);
    pthread_mutex_init(&pstGWGrpNode->mutexGWList, NULL);

    ulHashIndex = sc_ep_gw_grp_hash_func(pstGWGrpNode->ulGWGrpID);

    pthread_mutex_lock(&g_mutexHashGWGrp);
    hash_add_node(g_pstHashGWGrp, pstHashNode, ulHashIndex, NULL);
    pthread_mutex_unlock(&g_mutexHashGWGrp);

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_load_did_number()
 * 功能: 加载网关组数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_load_gateway_grp(U32 ulIndex)
{
    S8 szSQL[1024];
    U32 ulRet;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id FROM tbl_gateway_grp WHERE tbl_gateway_grp.status = 0;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id FROM tbl_gateway_grp WHERE tbl_gateway_grp.status = 0 AND id = %d;"
                        , ulIndex);
    }

    ulRet = db_query(g_pstSCDBHandle, szSQL, sc_load_gateway_grp_cb, NULL, NULL);
    if (ulRet != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(NULL, SC_FUNC, "Load gateway Group FAIL.(ID:%u)", ulIndex);
        return DOS_FAIL;
    }
    sc_logr_info(NULL, SC_FUNC, "Load gateway Group SUCC.(ID:%u)", ulIndex);

    return ulRet;
}

/**
 * 函数: S32 sc_load_relationship_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * 功能: 加载路由网关组关系数据
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_load_gw_relationship_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_GW_GRP_NODE_ST    *pstGWGrp      = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    U32                  ulGatewayID    = U32_BUTT;
    U32                  ulHashIndex    = U32_BUTT;
    S32                  lIndex         = 0;
    BOOL                 blProcessOK    = DOS_TRUE;

    if (DOS_ADDR_INVALID(pArg)
        || lCount <= 0
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    for (lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (DOS_ADDR_INVALID(aszNames[lIndex])
            || DOS_ADDR_INVALID(aszValues[lIndex]))
        {
            break;
        }

        if (0 == dos_strncmp(aszNames[lIndex], "gateway_id", dos_strlen("gateway_id")))
        {
            if (dos_atoul(aszValues[lIndex], &ulGatewayID) < 0)
            {
                blProcessOK  = DOS_FALSE;
                break;
            }
        }
    }

    if (!blProcessOK
        || U32_BUTT == ulGatewayID)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_gw_hash_func(ulGatewayID);
    pstHashNode = hash_find_node(g_pstHashGW, ulHashIndex, &ulGatewayID, sc_ep_gw_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstListNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    DLL_Init_Node(pstListNode);
    pstListNode->pHandle = pstHashNode->pHandle;

    pstGWGrp = (SC_GW_GRP_NODE_ST *)pArg;

    pthread_mutex_lock(&pstGWGrp->mutexGWList);
    DLL_Add(&pstGWGrp->stGWList, pstListNode);
    pthread_mutex_unlock(&pstGWGrp->mutexGWList);

    sc_logr_debug(NULL, SC_FUNC, "Gateway %u will be added into group %u.", ulGatewayID, pstGWGrp->ulGWGrpID);

    return DOS_FAIL;
}

/**
 * 函数: U32 sc_load_gw_relationship()
 * 功能: 加载路由网关组关系数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_load_gw_relationship()
{
    SC_GW_GRP_NODE_ST    *pstGWGrp      = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    U32                  ulHashIndex = 0;
    S8 szSQL[1024] = {0, };

    HASH_Scan_Table(g_pstHashGWGrp, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashGWGrp, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            pstGWGrp = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstGWGrp))
            {
                DOS_ASSERT(0);
                continue;
            }

            dos_snprintf(szSQL, sizeof(szSQL), "SELECT gateway_id FROM tbl_gateway_assign WHERE gateway_grp_id=%d;", pstGWGrp->ulGWGrpID);

            db_query(g_pstSCDBHandle, szSQL, sc_load_gw_relationship_cb, (VOID *)pstGWGrp, NULL);
        }
    }

    return DOS_SUCC;
}



/**
 * 函数: S32 sc_load_caller_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * 功能: 加载主叫号码数据
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_load_caller_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_CALLER_QUERY_NODE_ST  *pstCaller   = NULL, *pstCallerTemp = NULL;
    HASH_NODE_S              *pstHashNode = NULL;
    U32  ulHashIndex = U32_BUTT;
    S32  lIndex = U32_BUTT;
    BOOL blProcessOK = DOS_FALSE;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCaller = (SC_CALLER_QUERY_NODE_ST *)dos_dmem_alloc(sizeof(SC_CALLER_QUERY_NODE_ST));
    if (DOS_ADDR_INVALID(pstCaller))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_ep_caller_init(pstCaller);

    for (blProcessOK = DOS_TRUE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstCaller->ulIndexInDB) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstCaller->ulCustomerID) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "cid", dos_strlen("cid")))
        {
            if ('\0' == aszValues[lIndex][0])
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
            dos_snprintf(pstCaller->szNumber, sizeof(pstCaller->szNumber), "%s", aszValues[lIndex]);
        }
        else
        {
            DOS_ASSERT(0);
            blProcessOK = DOS_FALSE;
            break;
        }
        blProcessOK = DOS_TRUE;
    }
    if (!blProcessOK)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstCaller);
        pstCaller = NULL;
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashCaller);
    ulHashIndex = sc_ep_caller_hash_func(pstCaller->ulIndexInDB);
    pstHashNode = hash_find_node(g_pstHashCaller, ulHashIndex, (VOID *)&pstCaller->ulIndexInDB, sc_ep_caller_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = (HASH_NODE_S*)dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstCaller);
            pstCaller = NULL;
            pthread_mutex_unlock(&g_mutexHashCaller);
            return DOS_FAIL;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = (VOID *)pstCaller;
        hash_add_node(g_pstHashCaller, pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        pstCallerTemp = (SC_CALLER_QUERY_NODE_ST *)pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstCallerTemp))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstCaller);
            pstCaller = NULL;
            pthread_mutex_unlock(&g_mutexHashCaller);
            return DOS_FAIL;
        }

        dos_memcpy(pstCallerTemp, pstCaller, sizeof(SC_CALLER_QUERY_NODE_ST));
        dos_dmem_free(pstCaller);
        pstCaller = NULL;
    }
    pthread_mutex_unlock(&g_mutexHashCaller);

    return DOS_SUCC;
}


/**
 * 函数: U32 sc_load_caller(U32 ulIndex)
 * 功能: 加载主叫号码数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_load_caller(U32 ulIndex)
{
    S8   szQuery[256] = {0};
    U32  ulRet = U32_BUTT;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szQuery, sizeof(szQuery), "SELECT id,customer_id,cid FROM tbl_caller WHERE verify_status=1;");
    }
    else
    {
        dos_snprintf(szQuery, sizeof(szQuery), "SELECT id,customer_id,cid FROM tbl_caller WHERE verify_status=1 AND id=%u", ulIndex);
    }
    ulRet = db_query(g_pstSCDBHandle, szQuery, sc_load_caller_cb, NULL, NULL);
    if (DB_ERR_SUCC != ulRet)
    {
        sc_logr_error(NULL, SC_FUNC, "Load caller FAIL.(ID:%u)", ulIndex);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    sc_logr_debug(NULL, SC_FUNC, "Load Caller SUCC.(ID:%u)", ulIndex);

    return DOS_SUCC;
}


/**
 * 函数: S32 sc_load_caller_grp_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * 功能: 加载主叫号码数据
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_load_caller_grp_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_CALLER_GRP_NODE_ST   *pstCallerGrp = NULL, *pstCallerGrpTemp = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    U32  ulHashIndex = U32_BUTT;
    S32  lIndex = U32_BUTT;
    BOOL blProcessOK = DOS_FALSE;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)dos_dmem_alloc(sizeof(SC_CALLER_GRP_NODE_ST));
    if (DOS_ADDR_INVALID(pstCallerGrp))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_ep_caller_grp_init(pstCallerGrp);

    for (blProcessOK = DOS_TRUE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstCallerGrp->ulID) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstCallerGrp->ulCustomerID) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "name", dos_strlen("name")))
        {
            if ('\0' == aszValues[lIndex][0])
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
            dos_snprintf(pstCallerGrp->szGrpName, sizeof(pstCallerGrp->szGrpName), "%s", aszValues[lIndex]);
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "policy", dos_strlen("policy")))
        {
            if (dos_atoul(aszValues[lIndex], &pstCallerGrp->ulPolicy) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else
        {
            DOS_ASSERT(0);
            blProcessOK = DOS_FALSE;
            break;
        }
    }
    if (!blProcessOK)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashCallerGrp);
    ulHashIndex = sc_ep_caller_grp_hash_func(pstCallerGrp->ulID);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&pstCallerGrp->ulID, sc_ep_caller_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstCallerGrp);
            pstCallerGrp = NULL;
            pthread_mutex_unlock(&g_mutexHashCallerGrp);
            return DOS_FAIL;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = (VOID *)pstCallerGrp;
        hash_add_node(g_pstHashCallerGrp, pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        pstCallerGrpTemp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstCallerGrpTemp))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstCallerGrp);
            pstCallerGrp = NULL;
            pthread_mutex_unlock(&g_mutexHashCallerGrp);
            return DOS_FAIL;
        }
        dos_memcpy(pstCallerGrpTemp, pstCallerGrp, sizeof(SC_CALLER_QUERY_NODE_ST));
        dos_dmem_free(pstCallerGrp);
        pstCallerGrp = NULL;
    }
    pthread_mutex_unlock(&g_mutexHashCallerGrp);

    return DOS_SUCC;
}


S32 sc_load_caller_relationship_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    S32  lIndex = U32_BUTT;
    U32  ulCallerID = U32_BUTT, ulCallerType = U32_BUTT, ulHashIndex = U32_BUTT;
    SC_DID_NODE_ST *pstDid = NULL;
    DLL_NODE_S *pstNode = NULL;
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    SC_CALLER_CACHE_NODE_ST *pstCacheNode = NULL;
    BOOL blProcessOK = DOS_TRUE, bFound = DOS_FALSE;

    if (DOS_ADDR_INVALID(pArg)
        || lCount <= 0
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    for (lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (DOS_ADDR_INVALID(aszNames[lIndex])
            || DOS_ADDR_INVALID(aszValues[lIndex]))
        {
            break;
        }

        if (0 == dos_strnicmp(aszNames[lIndex], "caller_id", dos_strlen("caller_id")))
        {
            if (dos_atoul(aszValues[lIndex], &ulCallerID) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "caller_type", dos_strlen("caller_type")))
        {
            if (dos_atoul(aszValues[lIndex], &ulCallerType) < 0)
            {
                blProcessOK = DOS_FALSE;
            }
        }
    }

    if (!blProcessOK
        || U32_BUTT == ulCallerID)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCacheNode = (SC_CALLER_CACHE_NODE_ST *)dos_dmem_alloc(sizeof(SC_CALLER_CACHE_NODE_ST));
    if (DOS_ADDR_INVALID(pstCacheNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 如果是配置的主叫号码，先查找到对应的主叫号码节点 */
    if (SC_NUMBER_TYPE_CFG == ulCallerType)
    {
        pstCacheNode->ulType = SC_NUMBER_TYPE_CFG;
        ulHashIndex = sc_ep_caller_hash_func(ulCallerID);
        pstHashNode = hash_find_node(g_pstHashCaller, ulHashIndex, (VOID *)&ulCallerID, sc_ep_caller_hash_find);

        if (DOS_ADDR_INVALID(pstHashNode)
            || DOS_ADDR_INVALID(pstHashNode->pHandle))
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
        pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstHashNode->pHandle;
        pstCacheNode->stData.pstCaller = pstCaller;
    }
    else if (SC_NUMBER_TYPE_DID == ulCallerType)
    {
        pstCacheNode->ulType = SC_NUMBER_TYPE_DID;
        /* 目前由于索引未采用id，故只能遍历哈希表 */
        HASH_Scan_Table(g_pstHashDIDNum, ulHashIndex)
        {
            HASH_Scan_Bucket(g_pstHashDIDNum ,ulHashIndex ,pstHashNode, HASH_NODE_S *)
            {
                if (DOS_ADDR_INVALID(pstHashNode)
                    || DOS_ADDR_INVALID(pstHashNode->pHandle))
                {
                    continue;
                }

                pstDid = (SC_DID_NODE_ST *)pstHashNode->pHandle;
                if (ulCallerID == pstDid->ulDIDID)
                {
                    bFound = DOS_TRUE;
                    break;
                }
            }
            if (DOS_TRUE == bFound)
            {
                break;
            }
        }
        if (DOS_FALSE == bFound)
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
        pstCacheNode->stData.pstDid = pstDid;
    }

    pstNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    DLL_Init_Node(pstNode);
    pstNode->pHandle = (VOID *)pstCacheNode;

    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pArg;
    pthread_mutex_lock(&pstCallerGrp->mutexCallerList);
    DLL_Add(&pstCallerGrp->stCallerList, pstNode);
    pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);
    return DOS_SUCC;
}


/**
 * 函数: U32 sc_load_caller_grp(U32 ulIndex)
 * 功能: 加载主叫号码组数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_load_caller_grp(U32 ulIndex)
{
    S8 szSQL[1024] = {0};
    U32 ulRet = U32_BUTT;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id,customer_id,name,policy FROM tbl_caller_grp;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id,customer_id,name,policy FROM tbl_caller_grp WHERE id=%u;"
                        , ulIndex);
    }

    ulRet = db_query(g_pstSCDBHandle, szSQL, sc_load_caller_grp_cb, NULL, NULL);
    if (ulRet != DOS_SUCC)
    {
        sc_logr_error(NULL, SC_FUNC, "Load caller group FAIL.(ID:%u)", ulIndex);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    sc_logr_debug(NULL, SC_FUNC, "Load caller group SUCC.(ID:%u)", ulIndex);

    return ulRet;
}


U32 sc_load_caller_relationship()
{
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    U32  ulHashIndex = U32_BUTT;
    S32 lRet = U32_BUTT;
    S8   szQuery[256] = {0};
    HASH_NODE_S *pstHashNode = NULL;

    HASH_Scan_Table(g_pstHashCallerGrp, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashCallerGrp, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }
            pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;
            /* 先加载主叫号码，再加载DID号码 */
            dos_snprintf(szQuery, sizeof(szQuery), "SELECT caller_id,caller_type FROM tbl_caller_assign WHERE caller_grp_id=%u;", pstCallerGrp->ulID);
            lRet = db_query(g_pstSCDBHandle, szQuery, sc_load_caller_relationship_cb, (VOID *)pstCallerGrp, NULL);
            if (DB_ERR_SUCC != lRet)
            {
                sc_logr_error(NULL, SC_FUNC, "Load Caller RelationShip FAIL.(CallerGrpID:%u)", pstCallerGrp->ulID);
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
        }
    }
    sc_logr_debug(NULL, SC_FUNC, "%s", "Load Caller relationship SUCC.");

    return DOS_SUCC;
}

S32 sc_load_caller_setting_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    S32  lIndex = U32_BUTT;
    U32  ulHashIndex = U32_BUTT;
    HASH_NODE_S *pstHashNode = NULL;
    SC_CALLER_SETTING_ST *pstSetting = NULL, *pstSettingTemp = NULL;
    BOOL  blProcessOK = DOS_FALSE;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSetting = (SC_CALLER_SETTING_ST *)dos_dmem_alloc(sizeof(SC_CALLER_SETTING_ST));
    if (DOS_ADDR_INVALID(pstSetting))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_ep_caller_setting_init(pstSetting);

    for (lIndex = 0; lIndex < lCount; ++lIndex)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSetting->ulID) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSetting->ulCustomerID) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "name", dos_strlen("name")))
        {
            if ('\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
            dos_snprintf(pstSetting->szSettingName, sizeof(pstSetting->szSettingName), "%s", aszValues[lIndex]);
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "src_id", dos_strlen("src_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSetting->ulSrcID) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "src_type", dos_strlen("src_type")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSetting->ulSrcType) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "dest_id", dos_strlen("dest_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSetting->ulDstID) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "dest_type", dos_strlen("dest_type")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSetting->ulDstType) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else
        {
            blProcessOK = DOS_FALSE;
            DOS_ASSERT(0);
            break;
        }
        blProcessOK = DOS_TRUE;
    }
    if (!blProcessOK)
    {
        dos_dmem_free(pstSetting);
        pstSetting = NULL;
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_caller_setting_hash_func(pstSetting->ulID);
    pstHashNode = hash_find_node(g_pstHashCallerSetting, ulHashIndex, (VOID *)&pstSetting->ulID, sc_ep_caller_setting_hash_find);
    pthread_mutex_lock(&g_mutexHashCallerSetting);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);
            pthread_mutex_unlock(&g_mutexHashCallerSetting);
            dos_dmem_free(pstSetting);
            pstSetting = NULL;
            return DOS_FAIL;
        }
        DLL_Init_Node(pstHashNode);
        pstHashNode->pHandle = (VOID *)pstSetting;
        hash_add_node(g_pstHashCallerSetting, pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        pstSettingTemp = (SC_CALLER_SETTING_ST *)pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstSettingTemp))
        {
            dos_dmem_free(pstSetting);
            pstSetting = NULL;
            DOS_ASSERT(0);
            pthread_mutex_unlock(&g_mutexHashCallerSetting);
        }
        dos_memcpy(pstSettingTemp, pstSetting, sizeof(SC_CALLER_SETTING_ST));
        dos_dmem_free(pstSetting);
        pstSetting = NULL;
    }
    pthread_mutex_unlock(&g_mutexHashCallerSetting);

    return DOS_SUCC;
}

U32  sc_load_caller_setting(U32 ulIndex)
{
    S8  szQuery[128] = {0};
    S32 lRet;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szQuery, sizeof(szQuery), "SELECT id,customer_id,name,src_id,src_type,dest_id,dest_type FROM tbl_caller_setting;");
    }
    else
    {
        dos_snprintf(szQuery, sizeof(szQuery), "SELECT id,customer_id,name,src_id,src_type,dest_id,dest_type FROM tbl_caller_setting WHERE id=%u;", ulIndex);
    }

    lRet = db_query(g_pstSCDBHandle, szQuery, sc_load_caller_setting_cb, NULL, NULL);
    if (DB_ERR_SUCC != lRet)
    {
        sc_logr_error(NULL, SC_FUNC, "Load caller setting FAIL.(ID:%u)", ulIndex);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    sc_logr_debug(NULL, SC_FUNC, "Load caller setting SUCC.(ID:%u)", ulIndex);

    return DOS_SUCC;
}

U32 sc_refresh_gateway_grp(U32 ulIndex)
{
    S8 szSQL[1024];
    U32 ulHashIndex;
    SC_GW_GRP_NODE_ST    *pstGWGrpNode  = NULL;
    SC_GW_NODE_ST        *pstGWNode = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    DLL_NODE_S           *pstDLLNode    = NULL;

    if (SC_INVALID_INDEX == ulIndex || U32_BUTT == ulIndex)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_gw_grp_hash_func(ulIndex);
    pstHashNode = hash_find_node(g_pstHashGWGrp, ulHashIndex, &ulIndex, sc_ep_gw_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstGWGrpNode = pstHashNode->pHandle;
    pthread_mutex_lock(&pstGWGrpNode->mutexGWList);
    while (1)
    {
        if (DLL_Count(&pstGWGrpNode->stGWList) == 0)
        {
            break;
        }

        pstDLLNode = dll_fetch(&pstGWGrpNode->stGWList);
        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            DOS_ASSERT(0);
            break;
        }
        pstGWNode = (SC_GW_NODE_ST *)pstDLLNode->pHandle;
        sc_logr_debug(NULL, SC_FUNC, "Gateway %u will be removed from Group %u", pstGWNode->ulGWID, ulIndex);

        /* 此处不能释放网关数据，只需释放链表结点即可，因为一个网关可以属于多个网关组，此处删除，容易产生double free信号 */
        if (DOS_ADDR_VALID(pstDLLNode->pHandle))
        {
            /*dos_dmem_free(pstDLLNode->pHandle);*/
            pstDLLNode->pHandle = NULL;
        }

        dos_dmem_free(pstDLLNode);
        pstDLLNode = NULL;
    }
    pthread_mutex_unlock(&pstGWGrpNode->mutexGWList);

    dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT tbl_gateway_assign.gateway_id FROM tbl_gateway_assign WHERE tbl_gateway_assign.gateway_grp_id = %u;"
                        , ulIndex);

    return db_query(g_pstSCDBHandle, szSQL, sc_load_gw_relationship_cb, pstGWGrpNode, NULL);
}

U32 sc_refresh_caller_grp(U32 ulIndex)
{
    U32  ulHashIndex = U32_BUTT;
    SC_CALLER_GRP_NODE_ST  *pstCallerGrp = NULL;
    DLL_NODE_S *pstListNode = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    SC_CALLER_CACHE_NODE_ST *pstCache = NULL;
    S8  szQuery[256] = {0,};
    U32 ulRet = 0;

    sc_load_caller_grp(ulIndex);

    ulHashIndex = sc_ep_caller_grp_hash_func(ulIndex);
    pthread_mutex_lock(&g_mutexHashCallerGrp);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulIndex, sc_ep_caller_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pthread_mutex_unlock(&g_mutexHashCallerGrp);

        DOS_ASSERT(0);
        sc_logr_error(NULL, SC_FUNC, "SC Refresh Caller Grp FAIL.(CallerGrpID:%u)", ulIndex);
        return DOS_FAIL;
    }
    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;

    pthread_mutex_lock(&pstCallerGrp->mutexCallerList);
    while (1)
    {
        if (DLL_Count(&pstCallerGrp->stCallerList) == 0)
        {
            break;
        }

        pstListNode = dll_fetch(&pstCallerGrp->stCallerList);
        if (DOS_ADDR_INVALID(pstListNode))
        {
            continue;
        }

        pstCache = (SC_CALLER_CACHE_NODE_ST *)pstListNode->pHandle;
        if (DOS_ADDR_VALID(pstCache))
        {
            dos_dmem_free(pstCache);
            pstCache = NULL;
        }
        dll_delete(&pstCallerGrp->stCallerList, pstListNode);
        dos_dmem_free(pstListNode);
        pstListNode = NULL;
    }

    pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT caller_id,caller_type FROM tbl_caller_assign WHERE caller_grp_id=%u;", ulIndex);

    ulRet = db_query(g_pstSCDBHandle, szQuery, sc_load_caller_relationship_cb, (VOID *)pstCallerGrp, NULL);

    pthread_mutex_unlock(&g_mutexHashCallerGrp);
    return ulRet;
}

/**
 * 函数: S32 sc_load_route_group_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * 功能: 加载路由数据时数据库查询的回调函数，将数据加入路由列表中
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_load_route_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_ROUTE_NODE_ST     *pstRoute      = NULL;
    SC_ROUTE_NODE_ST     *pstRouteTmp   = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    S32                  lIndex;
    S32                  lSecond;
    S32                  lRet;
    S32                  lDestIDCount;
    BOOL                 blProcessOK = DOS_FALSE;
    U32                  ulCallOutGroup;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstRoute = dos_dmem_alloc(sizeof(SC_ROUTE_NODE_ST));
    if (DOS_ADDR_INVALID(pstRoute))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_ep_route_init(pstRoute);


    for (blProcessOK = DOS_TRUE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstRoute->ulID) < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "start_time", dos_strlen("start_time")))
        {
            lRet = dos_sscanf(aszValues[lIndex]
                        , "%d:%d:%d"
                        , &pstRoute->ucHourBegin
                        , &pstRoute->ucMinuteBegin
                        , &lSecond);
            if (3 != lRet)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "end_time", dos_strlen("end_time")))
        {
            lRet = dos_sscanf(aszValues[lIndex]
                    , "%d:%d:%d"
                    , &pstRoute->ucHourEnd
                    , &pstRoute->ucMinuteEnd
                    , &lSecond);
            if (3 != lRet)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "callee_prefix", dos_strlen("callee_prefix")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstRoute->szCalleePrefix, aszValues[lIndex], sizeof(pstRoute->szCalleePrefix));
                pstRoute->szCalleePrefix[sizeof(pstRoute->szCalleePrefix) - 1] = '\0';
            }
            else
            {
                pstRoute->szCalleePrefix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "caller_prefix", dos_strlen("caller_prefix")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstRoute->szCallerPrefix, aszValues[lIndex], sizeof(pstRoute->szCallerPrefix));
                pstRoute->szCallerPrefix[sizeof(pstRoute->szCallerPrefix) - 1] = '\0';
            }
            else
            {
                pstRoute->szCallerPrefix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "dest_type", dos_strlen("dest_type")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0]
                || dos_atoul(aszValues[lIndex], &pstRoute->ulDestType) < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "dest_id", dos_strlen("dest_id")))
        {
            if (SC_DEST_TYPE_GATEWAY == pstRoute->ulDestType)
            {
                if (DOS_ADDR_INVALID(aszValues[lIndex])
                    || '\0' == aszValues[lIndex][0]
                    || dos_atoul(aszValues[lIndex], &pstRoute->aulDestID[0]) < 0)
                {
                    DOS_ASSERT(0);

                    blProcessOK = DOS_FALSE;
                    break;
                }
            }
            else if (SC_DEST_TYPE_GW_GRP == pstRoute->ulDestType)
            {
                if (DOS_ADDR_INVALID(aszValues[lIndex])
                    || '\0' == aszValues[lIndex])
                {
                    DOS_ASSERT(0);

                    blProcessOK = DOS_FALSE;
                    break;
                }

                pstRoute->aulDestID[0] = U32_BUTT;
                pstRoute->aulDestID[1] = U32_BUTT;
                pstRoute->aulDestID[2] = U32_BUTT;
                pstRoute->aulDestID[3] = U32_BUTT;
                pstRoute->aulDestID[4] = U32_BUTT;

                lDestIDCount = dos_sscanf(aszValues[lIndex], "%d,%d,%d,%d,%d", &pstRoute->aulDestID[0], &pstRoute->aulDestID[1], &pstRoute->aulDestID[2], &pstRoute->aulDestID[3], &pstRoute->aulDestID[4]);
                if (lDestIDCount < 1)
                {
                    DOS_ASSERT(0);

                    blProcessOK = DOS_FALSE;
                    break;
                }
            }
            else
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "call_out_group", dos_strlen("call_out_group")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0]
                || dos_atoul(aszValues[lIndex], &ulCallOutGroup) < 0
                || ulCallOutGroup > U16_BUTT)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
            pstRoute->usCallOutGroup = (U16)ulCallOutGroup;
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "status", dos_strlen("status")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0]
                || dos_atoul(aszValues[lIndex], &pstRoute->bStatus) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "seq", dos_strlen("seq")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0]
                || dos_atoul(aszValues[lIndex], (U32 *)&pstRoute->ucPriority) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstRoute);
        pstRoute = NULL;
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexRouteList);
    pstListNode = dll_find(&g_stRouteList, &pstRoute->ulID, sc_ep_route_find);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        pstListNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstListNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstRoute);
            pstRoute = NULL;

            pthread_mutex_unlock(&g_mutexRouteList);
            return DOS_FAIL;
        }

        DLL_Init_Node(pstListNode);
        pstListNode->pHandle = pstRoute;
        pstRoute->bExist = DOS_TRUE;
        DLL_Add(&g_stRouteList, pstListNode);
    }
    else
    {
        pstRouteTmp = pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstRouteTmp))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstRoute);
            pstRoute = NULL;

            pthread_mutex_unlock(&g_mutexRouteList);
            return DOS_FAIL;
        }

        dos_memcpy(pstRouteTmp, pstRoute, sizeof(SC_ROUTE_NODE_ST));
        pstRouteTmp->bExist = DOS_TRUE;

        dos_dmem_free(pstRoute);
        pstRoute = NULL;
    }
    pthread_mutex_unlock(&g_mutexRouteList);

    return DOS_TRUE;
}

/**
 * 函数: U32 sc_load_route()
 * 功能: 加载路由数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_load_route(U32 ulIndex)
{
    S8 szSQL[1024];
    S32 lRet;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, start_time, end_time, callee_prefix, caller_prefix, dest_type, dest_id, call_out_group, status, seq FROM tbl_route ORDER BY tbl_route.seq ASC;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, start_time, end_time, callee_prefix, caller_prefix, dest_type, dest_id, call_out_group, status, seq FROM tbl_route WHERE id=%d ORDER BY tbl_route.seq ASC;"
                    , ulIndex);
    }

    lRet = db_query(g_pstSCDBHandle, szSQL, sc_load_route_cb, NULL, NULL);
    if (DB_ERR_SUCC != lRet)
    {
        DOS_ASSERT(0);
        sc_logr_error(NULL, SC_FUNC, "Load route FAIL(ID:%u).", ulIndex);
        return DOS_FAIL;
    }
    sc_logr_debug(NULL, SC_FUNC, "Load route SUCC.(ID:%u)", ulIndex);

    return DOS_SUCC;
}

S32 sc_load_num_transform_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_NUM_TRANSFORM_NODE_ST    *pstNumTransform    = NULL;
    SC_NUM_TRANSFORM_NODE_ST    *pstNumTransformTmp = NULL;
    DLL_NODE_S                  *pstListNode        = NULL;
    S32                         lIndex              = 0;
    S32                         lRet                = 0;
    BOOL                        blProcessOK         = DOS_FALSE;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstNumTransform = (SC_NUM_TRANSFORM_NODE_ST *)dos_dmem_alloc(sizeof(SC_NUM_TRANSFORM_NODE_ST));
    if (DOS_ADDR_INVALID(pstNumTransform))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    sc_ep_num_transform_init(pstNumTransform);

    for (blProcessOK = DOS_TRUE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstNumTransform->ulID) < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "object_id", dos_strlen("object_id")))
        {
            if ('\0' == aszValues[lIndex][0])
            {
                continue;
            }

            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->ulObjectID);
            if (lRet < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "object", dos_strlen("object")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->enObject);
            if (lRet < 0 || pstNumTransform->enObject >= SC_NUM_TRANSFORM_OBJECT_BUTT)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "direction", dos_strlen("direction")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->enDirection);

            if (lRet < 0 || pstNumTransform->enDirection >= SC_NUM_TRANSFORM_DIRECTION_BUTT)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "transform_timing", dos_strlen("transform_timing")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->enTiming);

            if (lRet < 0 || pstNumTransform->enTiming >= SC_NUM_TRANSFORM_TIMING_BUTT)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "num_selection", dos_strlen("num_selection")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->enNumSelect);

            if (lRet < 0 || pstNumTransform->enNumSelect >= SC_NUM_TRANSFORM_SELECT_BUTT)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "callee_prefixion", dos_strlen("callee_prefixion")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstNumTransform->szCalleePrefix, aszValues[lIndex], sizeof(pstNumTransform->szCalleePrefix));
                pstNumTransform->szCalleePrefix[sizeof(pstNumTransform->szCalleePrefix) - 1] = '\0';
            }
            else
            {
                pstNumTransform->szCalleePrefix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "caller_prefixion", dos_strlen("caller_prefixion")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstNumTransform->szCallerPrefix, aszValues[lIndex], sizeof(pstNumTransform->szCallerPrefix));
                pstNumTransform->szCallerPrefix[sizeof(pstNumTransform->szCallerPrefix) - 1] = '\0';
            }
            else
            {
                pstNumTransform->szCallerPrefix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "replace_all", dos_strlen("replace_all")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex]))
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }

            if (aszValues[lIndex][0] == '0')
            {
                pstNumTransform->bReplaceAll = DOS_FALSE;
            }
            else
            {
                pstNumTransform->bReplaceAll = DOS_TRUE;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "replace_num", dos_strlen("replace_num")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstNumTransform->szReplaceNum, aszValues[lIndex], sizeof(pstNumTransform->szReplaceNum));
                pstNumTransform->szReplaceNum[sizeof(pstNumTransform->szReplaceNum) - 1] = '\0';
            }
            else
            {
                pstNumTransform->szReplaceNum[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "del_left", dos_strlen("del_left")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->ulDelLeft);
            if (lRet < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "del_right", dos_strlen("del_right")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->ulDelRight);
            if (lRet < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "add_prefixion", dos_strlen("add_prefixion")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstNumTransform->szAddPrefix, aszValues[lIndex], sizeof(pstNumTransform->szAddPrefix));
                pstNumTransform->szAddPrefix[sizeof(pstNumTransform->szAddPrefix) - 1] = '\0';
            }
            else
            {
                pstNumTransform->szAddPrefix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "add_suffix", dos_strlen("add_suffix")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstNumTransform->szAddSuffix, aszValues[lIndex], sizeof(pstNumTransform->szAddSuffix));
                pstNumTransform->szAddSuffix[sizeof(pstNumTransform->szAddSuffix) - 1] = '\0';
            }
            else
            {
                pstNumTransform->szAddSuffix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "priority", dos_strlen("priority")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->enPriority);
            if (lRet < 0 || pstNumTransform->enPriority >= SC_NUM_TRANSFORM_PRIORITY_BUTT)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "expiry", dos_strlen("expiry")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->ulExpiry);
            if (lRet < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }

            if (0 == pstNumTransform->ulExpiry)
            {
                /* 无限制 */
                pstNumTransform->ulExpiry = U32_BUTT;
            }
        }
        else
        {
            DOS_ASSERT(0);

            blProcessOK = DOS_FALSE;
            break;
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstNumTransform);
        pstNumTransform = NULL;

        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexNumTransformList);
    pstListNode = dll_find(&g_stNumTransformList, &pstNumTransform->ulID, sc_ep_num_transform_find);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        pstListNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstListNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstNumTransform);
            pstNumTransform = NULL;

            pthread_mutex_unlock(&g_mutexNumTransformList);

            return DOS_FAIL;
        }

        DLL_Init_Node(pstListNode);
        pstListNode->pHandle = pstNumTransform;
        pstNumTransform->bExist = DOS_TRUE;
        DLL_Add(&g_stNumTransformList, pstListNode);
    }
    else
    {
        pstNumTransformTmp = pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstNumTransformTmp))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstNumTransform);
            pstNumTransform = NULL;

            pthread_mutex_unlock(&g_mutexNumTransformList);
            return DOS_FAIL;
        }

        dos_memcpy(pstNumTransformTmp, pstNumTransform, sizeof(SC_NUM_TRANSFORM_NODE_ST));
        pstNumTransform->bExist = DOS_TRUE;

        dos_dmem_free(pstNumTransform);
        pstNumTransform = NULL;
    }
    pthread_mutex_unlock(&g_mutexNumTransformList);

    return DOS_TRUE;
}


U32 sc_load_num_transform(U32 ulIndex)
{
    S8 szSQL[1024];
    S32 lRet;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, object, object_id, direction, transform_timing, num_selection, caller_prefixion, callee_prefixion, replace_all, replace_num, del_left, del_right, add_prefixion, add_suffix, priority, expiry FROM tbl_num_transformation  ORDER BY tbl_num_transformation.priority ASC;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, object, object_id, direction, transform_timing, num_selection, caller_prefixion, callee_prefixion, replace_all, replace_num, del_left, del_right, add_prefixion, add_suffix, priority, expiry FROM tbl_num_transformation where tbl_num_transformation.id=%u;"
                    , ulIndex);
    }

    lRet = db_query(g_pstSCDBHandle, szSQL, sc_load_num_transform_cb, NULL, NULL);
    if (DB_ERR_SUCC != lRet)
    {
        DOS_ASSERT(0);
        sc_logr_error(NULL, SC_FUNC, "Load Num Transform FAIL.(ID:%u)", ulIndex);
        return DOS_FAIL;
    }
    sc_logr_debug(NULL, SC_FUNC, "Load Num Transform SUCC.(ID:%u)", ulIndex);

    return DOS_SUCC;
}

S32 sc_load_customer_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_CUSTOMER_NODE_ST  *pstCustomer       = NULL;
    SC_CUSTOMER_NODE_ST  *pstCustomerTmp    = NULL;
    DLL_NODE_S           *pstListNode       = NULL;
    S32                  lIndex;
    BOOL                 blProcessOK = DOS_FALSE;
    U32                  ulCallOutGroup;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstCustomer = dos_dmem_alloc(sizeof(SC_CUSTOMER_NODE_ST));
    if (DOS_ADDR_INVALID(pstCustomer))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_ep_customer_init(pstCustomer);

    for (blProcessOK = DOS_TRUE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstCustomer->ulID) < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        /* 分组编号可以为空，如果为空或者值超过最大值，都默认按0处理 */
        else if (0 == dos_strnicmp(aszNames[lIndex], "call_out_group", dos_strlen("call_out_group")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                pstCustomer->usCallOutGroup = 0;
            }
            else
            {
                if (dos_atoul(aszValues[lIndex], &ulCallOutGroup) < 0
                    || ulCallOutGroup > U16_BUTT)
                {
                    DOS_ASSERT(0);
                    blProcessOK = DOS_FALSE;
                    break;
                }
                pstCustomer->usCallOutGroup = (U16)ulCallOutGroup;
            }
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstCustomer);
        pstCustomer = NULL;
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexCustomerList);
    pstListNode = dll_find(&g_stCustomerList, &pstCustomer->ulID, sc_ep_customer_find);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        pstListNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstListNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstCustomer);
            pstCustomer = NULL;

            pthread_mutex_unlock(&g_mutexCustomerList);
            return DOS_FAIL;
        }

        DLL_Init_Node(pstListNode);
        pstListNode->pHandle = pstCustomer;
        pstCustomer->bExist = DOS_TRUE;
        DLL_Add(&g_stCustomerList, pstListNode);
    }
    else
    {
        pstCustomerTmp = pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstCustomerTmp))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstCustomer);
            pstCustomer = NULL;

            pthread_mutex_unlock(&g_mutexCustomerList);
            return DOS_FAIL;
        }

        dos_memcpy(pstCustomerTmp, pstCustomer, sizeof(SC_CUSTOMER_NODE_ST));
        pstCustomerTmp->bExist = DOS_TRUE;

        dos_dmem_free(pstCustomer);
        pstCustomer = NULL;
    }
    pthread_mutex_unlock(&g_mutexCustomerList);

    return DOS_TRUE;
}

U32 sc_load_customer(U32 ulIndex)
{
    S8 szSQL[1024];

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, call_out_group FROM tbl_customer where tbl_customer.type=0;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, call_out_group FROM tbl_customer where tbl_customer.type=0 and tbl_customer.id=%u;"
                    , ulIndex);
    }

    db_query(g_pstSCDBHandle, szSQL, sc_load_customer_cb, NULL, NULL);

    return DOS_SUCC;
}

static S32 sc_load_number_lmt_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    HASH_NODE_S           *pstHashNode      = NULL;
    SC_NUMBER_LMT_NODE_ST *pstNumLmtNode    = NULL;
    S8                    *pszID            = NULL;
    S8                    *pszLimitationID  = NULL;
    S8                    *pszType          = NULL;
    S8                    *pszBID           = NULL;
    S8                    *pszHandle        = NULL;
    S8                    *pszCycle         = NULL;
    S8                    *pszTimes         = NULL;
    S8                    *pszCID           = NULL;
    S8                    *pszDID           = NULL;
    S8                    *pszNumber        = NULL;
    U32                   ulID              = U32_BUTT;
    U32                   ulLimitationID    = U32_BUTT;
    U32                   ulType            = U32_BUTT;
    U32                   ulBID             = U32_BUTT;
    U32                   ulHandle          = U32_BUTT;
    U32                   ulCycle           = U32_BUTT;
    U32                   ulTimes           = U32_BUTT;
    U32                   ulHashIndex       = U32_BUTT;

    if (lCount < 9)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pszID            = aszValues[0];
    pszLimitationID  = aszValues[1];
    pszType          = aszValues[2];
    pszBID           = aszValues[3];
    pszHandle        = aszValues[4];
    pszCycle         = aszValues[5];
    pszTimes         = aszValues[6];
    pszCID           = aszValues[7];
    pszDID           = aszValues[8];

    if (DOS_ADDR_INVALID(pszID)
        || DOS_ADDR_INVALID(pszLimitationID)
        || DOS_ADDR_INVALID(pszType)
        || DOS_ADDR_INVALID(pszBID)
        || DOS_ADDR_INVALID(pszHandle)
        || DOS_ADDR_INVALID(pszCycle)
        || DOS_ADDR_INVALID(pszTimes)
        || dos_atoul(pszID, &ulID) < 0
        || dos_atoul(pszLimitationID, &ulLimitationID) < 0
        || dos_atoul(pszType, &ulType) < 0
        || dos_atoul(pszBID, &ulBID) < 0
        || dos_atoul(pszHandle, &ulHandle) < 0
        || dos_atoul(pszCycle, &ulCycle) < 0
        || dos_atoul(pszTimes, &ulTimes) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (ulType)
    {
        case SC_NUM_LMT_TYPE_DID:
            pszNumber = pszDID;
            break;
        case SC_NUM_LMT_TYPE_CALLER:
            pszNumber = pszCID;
            break;
        default:
            DOS_ASSERT(0);
            return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pszNumber)
        || '\0' == pszNumber[0])
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (ulHandle >= SC_NUM_LMT_HANDLE_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_number_lmt_hash_func(pszNumber);
    pthread_mutex_lock(&g_mutexHashNumberlmt);
    pstHashNode = hash_find_node(g_pstHashNumberlmt, ulHashIndex, pszNumber, sc_ep_number_lmt_find);
    if (DOS_ADDR_VALID(pstHashNode))
    {
        pstNumLmtNode= pstHashNode->pHandle;
        if (DOS_ADDR_VALID(pstNumLmtNode))
        {
            pstNumLmtNode->ulID      = ulID;
            pstNumLmtNode->ulGrpID   = ulLimitationID;
            pstNumLmtNode->ulHandle  = ulHandle;
            pstNumLmtNode->ulLimit   = ulTimes;
            pstNumLmtNode->ulCycle   = ulCycle;
            pstNumLmtNode->ulType    = ulType;
            pstNumLmtNode->ulNumberID= ulBID;
            dos_strncpy(pstNumLmtNode->szPrefix, pszNumber, sizeof(pstNumLmtNode->szPrefix));
            pstNumLmtNode->szPrefix[sizeof(pstNumLmtNode->szPrefix) - 1] = '\0';
        }
        else
        {
            pstNumLmtNode = dos_dmem_alloc(sizeof(SC_NUMBER_LMT_NODE_ST));
            if (DOS_ADDR_INVALID(pstNumLmtNode))
            {
                DOS_ASSERT(0);
                pthread_mutex_unlock(&g_mutexHashNumberlmt);

                return DOS_FAIL;
            }

            pstNumLmtNode->ulID      = ulID;
            pstNumLmtNode->ulGrpID   = ulLimitationID;
            pstNumLmtNode->ulHandle  = ulHandle;
            pstNumLmtNode->ulLimit   = ulTimes;
            pstNumLmtNode->ulCycle   = ulCycle;
            pstNumLmtNode->ulType    = ulType;
            pstNumLmtNode->ulNumberID= ulBID;
            pstNumLmtNode->ulStatUsed = 0;
            dos_strncpy(pstNumLmtNode->szPrefix, pszNumber, sizeof(pstNumLmtNode->szPrefix));
            pstNumLmtNode->szPrefix[sizeof(pstNumLmtNode->szPrefix) - 1] = '\0';

            pstHashNode->pHandle = pstNumLmtNode;
        }
    }
    else
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        pstNumLmtNode = dos_dmem_alloc(sizeof(SC_NUMBER_LMT_NODE_ST));
        if (DOS_ADDR_INVALID(pstHashNode)
            || DOS_ADDR_INVALID(pstNumLmtNode))
        {
            DOS_ASSERT(0);
            pthread_mutex_unlock(&g_mutexHashNumberlmt);

            if (DOS_ADDR_VALID(pstHashNode))
            {
                dos_dmem_free(pstHashNode);
                pstHashNode = NULL;
            }

            if (DOS_ADDR_VALID(pstNumLmtNode))
            {
                dos_dmem_free(pstNumLmtNode);
                pstNumLmtNode = NULL;
            }

            return DOS_FAIL;
        }

        pstNumLmtNode->ulID      = ulID;
        pstNumLmtNode->ulGrpID   = ulLimitationID;
        pstNumLmtNode->ulHandle  = ulHandle;
        pstNumLmtNode->ulLimit   = ulTimes;
        pstNumLmtNode->ulCycle   = ulCycle;
        pstNumLmtNode->ulType    = ulType;
        pstNumLmtNode->ulNumberID= ulBID;
        pstNumLmtNode->ulStatUsed = 0;
        dos_strncpy(pstNumLmtNode->szPrefix, pszNumber, sizeof(pstNumLmtNode->szPrefix));
        pstNumLmtNode->szPrefix[sizeof(pstNumLmtNode->szPrefix) - 1] = '\0';

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstNumLmtNode;

        hash_add_node(g_pstHashNumberlmt, pstHashNode, ulHashIndex, NULL);
    }
    pthread_mutex_unlock(&g_mutexHashNumberlmt);

    return DOS_SUCC;
}

U32 sc_load_number_lmt(U32 ulIndex)
{
    S8 szSQL[1024];

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT a.id, a.limitation_id, a.btype, a.bid, b.handle, " \
                      "b.cycle, b.times, c.cid, d.did_number " \
                      "FROM tbl_caller_limitation_assign a " \
                      "LEFT JOIN tbl_caller_limitation b ON a.limitation_id = b.id " \
                      "LEFT JOIN tbl_caller c ON a.btype = %u AND a.bid = c.id " \
                      "LEFT JOIN tbl_sipassign d ON a.btype = %u AND a.bid = d.id "
                    , SC_NUM_LMT_TYPE_CALLER, SC_NUM_LMT_TYPE_DID);
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT a.id, a.limitation_id, a.btype, a.bid, b.handle, " \
                      "b.cycle, b.times, c.cid, d.did_number " \
                      "FROM tbl_caller_limitation_assign a " \
                      "LEFT JOIN tbl_caller_limitation b ON a.limitation_id = b.id " \
                      "LEFT JOIN tbl_caller c ON a.btype = %u AND a.bid = c.id " \
                      "LEFT JOIN tbl_sipassign d ON a.btype = %u AND a.bid = d.id " \
                      "WHERE a.limitation_id = %u"
                    , SC_NUM_LMT_TYPE_CALLER, SC_NUM_LMT_TYPE_DID, ulIndex);

    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_load_number_lmt_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(NULL, SC_ESL, "%s", "Load number limitation fail.");
        return DOS_FAIL;
    }


    sc_num_lmt_update(0, NULL);

    return DOS_SUCC;
}

U32 sc_del_number_lmt(U32 ulIndex)
{
    U32                   ulHashIndex       = U32_BUTT;
    HASH_NODE_S           *pstHashNode      = NULL;
    SC_NUMBER_LMT_NODE_ST *pstNumLmtNode    = NULL;

    pthread_mutex_lock(&g_mutexHashNumberlmt);
    HASH_Scan_Table(g_pstHashNumberlmt, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashNumberlmt, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstNumLmtNode = pstHashNode->pHandle;
            if (pstNumLmtNode->ulGrpID == ulIndex)
            {
                hash_delete_node(g_pstHashNumberlmt, pstHashNode, ulHashIndex);
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashNumberlmt);

    return DOS_FAIL;
}

S32 sc_serv_ctrl_hash(U32 ulCustomerID)
{
    return ulCustomerID % SC_SERV_CTRL_HASH_SIZE;
}

S32 sc_serv_ctrl_hash_find_cb(VOID *pKey, DLL_NODE_S *pstDLLNode)
{
    SC_SRV_CTRL_ST       *pstSrvCtrl;
    SC_SRV_CTRL_FIND_ST  *pstSrvCtrlFind;

    if (DOS_ADDR_INVALID(pKey))
    {
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstDLLNode) || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        return DOS_FAIL;
    }

    pstSrvCtrl     = (SC_SRV_CTRL_ST *)pstDLLNode->pHandle;
    pstSrvCtrlFind = (SC_SRV_CTRL_FIND_ST*)pKey;

    if (pstSrvCtrl->ulID != pstSrvCtrlFind->ulID
        || pstSrvCtrl->ulCustomerID != pstSrvCtrlFind->ulCustomerID)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

S32 sc_load_serv_ctrl_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_SRV_CTRL_ST        *pstSrvCtrl;
    HASH_NODE_S           *pstHashNode;
    S8                    *pszTmpName;
    S8                    *pszTmpVal;
    U32                   ulLoop;
    U32                   ulProcessCnt;
    U32                   ulHashIndex;
    BOOL                  blProcessOK;
    SC_SRV_CTRL_FIND_ST   stFindParam;

    if (lCount < 9)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSrvCtrl = dos_dmem_alloc(sizeof(SC_SRV_CTRL_ST));
    if (DOS_ADDR_INVALID(pstSrvCtrl))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulProcessCnt = 0;
    blProcessOK=DOS_TRUE;
    for (ulLoop=0; ulLoop<9; ulLoop++)
    {
        pszTmpName = aszNames[ulLoop];
        pszTmpVal  = aszValues[ulLoop];

        if (DOS_ADDR_INVALID(pszTmpName))
        {
            continue;
        }

        if (dos_strnicmp(pszTmpName, "id", dos_strlen("id")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "customer", dos_strlen("customer")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulCustomerID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "serv_type", dos_strlen("serv_type")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulServType) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "effective", dos_strlen("effective")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulEffectTimestamp) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "expire_time", dos_strlen("expire_time")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulExpireTimestamp) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "attr1_value", dos_strlen("attr1_value")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulAttrValue1) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "attr2_value", dos_strlen("attr2_value")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulAttrValue2) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "attr1", dos_strlen("attr1")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulAttr1) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "attr2", dos_strlen("attr2")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulAttr2) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
    }

    if (!blProcessOK || 9 != ulProcessCnt)
    {
        sc_logr_notice(NULL, SC_ESL, "Load service control rule fail.(%u, %u)", ulProcessCnt, blProcessOK);

        dos_dmem_free(pstSrvCtrl);
        pstSrvCtrl = NULL;

        return DOS_FAIL;
    }

    /*
      * 定义属性类型如果为0，标示属性是无效的，这里将属性值设置为U32_BUTT, 方便程序处理
      * 目前在某些情况下属性值为0，也是一种合法值，因此将U32_BUTT视为无效值
      */
    if (0 == pstSrvCtrl->ulAttr1)
    {
        pstSrvCtrl->ulAttrValue1 = U32_BUTT;
    }

    if (0 == pstSrvCtrl->ulAttr2)
    {
        pstSrvCtrl->ulAttrValue2 = U32_BUTT;
    }

    ulHashIndex = sc_serv_ctrl_hash(pstSrvCtrl->ulCustomerID);
    stFindParam.ulID = pstSrvCtrl->ulID;
    stFindParam.ulCustomerID = pstSrvCtrl->ulCustomerID;
    pthread_mutex_lock(&g_mutexHashServCtrl);
    pstHashNode = hash_find_node(g_pstHashServCtrl, ulHashIndex, &stFindParam, sc_serv_ctrl_hash_find_cb);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = dos_dmem_alloc(sizeof(SC_SRV_CTRL_ST));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            pthread_mutex_unlock(&g_mutexHashServCtrl);

            sc_logr_warning(NULL, SC_ESL, "Alloc memory for service ctrl block fail. ID: %u, Customer: %u", stFindParam.ulID, stFindParam.ulCustomerID);

            dos_dmem_free(pstSrvCtrl);
            pstSrvCtrl = NULL;
            return DOS_FAIL;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstSrvCtrl;

        hash_add_node(g_pstHashServCtrl, pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        if (DOS_ADDR_INVALID(pstHashNode->pHandle))
        {
            DOS_ASSERT(0);

            pstHashNode->pHandle = pstSrvCtrl;
        }
        else
        {
            dos_memcpy(pstHashNode->pHandle, pstSrvCtrl, sizeof(SC_SRV_CTRL_ST));
        }
    }

    pthread_mutex_unlock(&g_mutexHashServCtrl);

    sc_logr_debug(NULL, SC_ESL, "Add service ctrl block. ID: %u, Customer: %u", stFindParam.ulID, stFindParam.ulCustomerID);
    return DOS_SUCC;
}


U32 sc_serv_ctrl_delete(U32 ulIndex)
{
    HASH_NODE_S           *pstHashNode;
    U32                   ulHashIndex;
    SC_SRV_CTRL_ST        *pstServCtrl;

    pthread_mutex_lock(&g_mutexHashServCtrl);
    HASH_Scan_Table(g_pstHashServCtrl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashServCtrl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstServCtrl = pstHashNode->pHandle;
            if (pstServCtrl->ulID == ulIndex)
            {
                hash_delete_node(g_pstHashServCtrl, pstHashNode, ulHashIndex);

                pthread_mutex_unlock(&g_mutexHashServCtrl);

                return DOS_SUCC;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashServCtrl);

    return DOS_SUCC;
}

U32 sc_load_serv_ctrl(U32 ulIndex)
{
    S8 szSQL[1024];

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT "
                      "tbl_serv_ctrl.id AS id, "
                      "tbl_serv_ctrl.customer AS customer, "
                      "tbl_serv_ctrl.serv_type AS serv_type, "
                      "tbl_serv_ctrl.effective_time AS effective,"
                      "tbl_serv_ctrl.expire_time AS expire_time, "
                      "tbl_serv_ctrl.attr1 AS attr1, "
                      "tbl_serv_ctrl.attr2 AS attr2, "
                      "tbl_serv_ctrl.attr1_value AS attr1_value, "
                      "tbl_serv_ctrl.attr2_value AS attr2_value "
                      "FROM tbl_serv_ctrl");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT "
                      "tbl_serv_ctrl.id AS id, "
                      "tbl_serv_ctrl.customer AS customer, "
                      "tbl_serv_ctrl.serv_type AS serv_type, "
                      "tbl_serv_ctrl.effective_time AS effective,"
                      "tbl_serv_ctrl.expire_time AS expire_time, "
                      "tbl_serv_ctrl.attr1 AS attr1, "
                      "tbl_serv_ctrl.attr2 AS attr2, "
                      "tbl_serv_ctrl.attr1_value AS attr1_value, "
                      "tbl_serv_ctrl.attr2_value AS attr2_value "
                      "FROM tbl_serv_ctrl WHERE id=%u", ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_load_serv_ctrl_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(NULL, SC_ESL, "%s", "Load service control rule fail.");
        return DOS_FAIL;
    }

    sc_logr_error(NULL, SC_ESL, "%s", "Load service control rule succ.");

    return DOS_SUCC;
}

BOOL sc_check_server_ctrl(U32 ulCustomerID, U32 ulServerType, U32 ulAttr1, U32 ulAttrVal1,U32 ulAttr2, U32 ulAttrVal2)
{
    BOOL             blResult;
    U32              ulHashIndex;
    HASH_NODE_S      *pstHashNode;
    SC_SRV_CTRL_ST   *pstSrvCtrl;
    time_t           stTime;

    ulHashIndex = sc_serv_ctrl_hash(ulCustomerID);
    stTime = time(NULL);

    sc_logr_debug(NULL, SC_ESL, "match service control rule. Customer: %u, Service: %u, Attr1: %u, Attr2: %u, Attr1Val: %u, Attr2Val: %u"
                , ulCustomerID, ulServerType, ulAttr1, ulAttrVal1, ulAttr2, ulAttrVal2);

    blResult = DOS_FALSE;
    pthread_mutex_lock(&g_mutexHashServCtrl);
    HASH_Scan_Bucket(g_pstHashServCtrl, ulHashIndex, pstHashNode, HASH_NODE_S *)
    {
        /* 合法性检查 */
        if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
        {
            continue;
        }

        /* 一个BUCKET中可能有很多企业的，需要过滤 */
        pstSrvCtrl = pstHashNode->pHandle;
        if (ulCustomerID != pstSrvCtrl->ulCustomerID)
        {
            continue;
        }

        if (stTime < pstSrvCtrl->ulEffectTimestamp
            || (pstSrvCtrl->ulExpireTimestamp && stTime > pstSrvCtrl->ulExpireTimestamp))
        {
            sc_logr_debug(NULL, SC_ESL, "Test service control rule: FAIL, Effect: %u, Expire: %u, Current: %u"
                            , pstSrvCtrl->ulEffectTimestamp, pstSrvCtrl->ulExpireTimestamp, stTime);
            continue;
        }

        /* 业务类型要一致, 如果业务类型不为0，才检查 */
        if (0 != ulServerType
            && ulServerType != pstSrvCtrl->ulServType)
        {
            sc_logr_debug(NULL, SC_ESL, "Test service control rule: FAIL, Request service type: %u, Current service type: %u"
                            , ulServerType, pstSrvCtrl->ulServType);
            continue;
        }

        /* 检查属性，不为0才检查 */
        if (0 != ulAttr1
            && ulAttr1 != pstSrvCtrl->ulAttr1)
        {
            sc_logr_debug(NULL, SC_ESL, "Test service control rule: FAIL, Request Attr1: %u, Current Attr1: %u"
                            , ulAttr1, pstSrvCtrl->ulAttr1);
            continue;
        }

        /* 检查属性，不为0才检查 */
        if (0 != ulAttr2
            && ulAttr2 != pstSrvCtrl->ulAttr2)
        {
            sc_logr_debug(NULL, SC_ESL, "Test service control rule: FAIL, Request Attr2: %u, Current Attr2: %u"
                            , ulAttr2, pstSrvCtrl->ulAttr2);
            continue;
        }

        /* 检查属性值，不为U32_BUTT才检查 */
        if (U32_BUTT != ulAttrVal1
            && ulAttrVal1 != pstSrvCtrl->ulAttrValue1)
        {
            sc_logr_debug(NULL, SC_ESL, "Test service control rule: FAIL, Request Attr1 Value: %u, Attr1 Value: %u"
                            , ulAttrVal1, pstSrvCtrl->ulAttrValue1);
            continue;
        }

        /* 检查属性值，不为U32_BUTT才检查 */
        if (U32_BUTT != ulAttrVal2
            && ulAttrVal2 != pstSrvCtrl->ulAttrValue2)
        {
            sc_logr_debug(NULL, SC_ESL, "Test service control rule: FAIL, Request Attr2 Value: %u, Attr2 Value: %u"
                            , ulAttrVal2, pstSrvCtrl->ulAttrValue2);
            continue;
        }

        /* 所有向均匹配 */
        sc_logr_debug(NULL, SC_ESL, "Test service control rule: SUCC. ID: %u", pstSrvCtrl->ulID);

        blResult = DOS_TRUE;
        break;
    }

    if (!blResult)
    {
        sc_logr_debug(NULL, SC_ESL, "match service ctrl rule in all. CustomerID", ulServerType);

        /* BUCKET 0 中保存的是针对所有客户的规则 */
        HASH_Scan_Bucket(g_pstHashServCtrl, 0, pstHashNode, HASH_NODE_S *)
        {
            /* 合法性检查 */
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            /* 一个BUCKET中可能有很多企业的，需要过滤 */
            pstSrvCtrl = pstHashNode->pHandle;
            if (0 != pstSrvCtrl->ulCustomerID)
            {
                continue;
            }

            if (stTime < pstSrvCtrl->ulEffectTimestamp
                || (pstSrvCtrl->ulExpireTimestamp && stTime > pstSrvCtrl->ulExpireTimestamp))
            {
                sc_logr_debug(NULL, SC_ESL, "Test service control rule: FAIL, Effect: %u, Expire: %u, Current: %u(BUCKET 0)"
                                , pstSrvCtrl->ulEffectTimestamp, pstSrvCtrl->ulExpireTimestamp, stTime);
                continue;
            }

            /* 业务类型要一致, 如果业务类型不为0，才检查 */
            if (0 != ulServerType
                && ulServerType != pstSrvCtrl->ulServType)
            {
                sc_logr_debug(NULL, SC_ESL, "Test service control rule: FAIL, Request service type: %u, Current service type: %u(BUCKET 0)"
                                , ulServerType, pstSrvCtrl->ulServType);
                continue;
            }

            /* 检查属性，不为0才检查 */
            if (0 != ulAttr1
                && ulAttr1 != pstSrvCtrl->ulAttr1)
            {
                sc_logr_debug(NULL, SC_ESL, "Test service control rule: FAIL, Request Attr1: %u, Current Attr1: %u(BUCKET 0)"
                                , ulAttr1, pstSrvCtrl->ulAttr1);
                continue;
            }

            /* 检查属性，不为0才检查 */
            if (0 != ulAttr2
                && ulAttr2 != pstSrvCtrl->ulAttr2)
            {
                sc_logr_debug(NULL, SC_ESL, "Test service control rule: FAIL, Request Attr2: %u, Current Attr2: %u(BUCKET 0)"
                                , ulAttr2, pstSrvCtrl->ulAttr2);
                continue;
            }

            /* 检查属性值，不为U32_BUTT才检查 */
            if (U32_BUTT != ulAttrVal1
                && ulAttrVal1 != pstSrvCtrl->ulAttrValue1)
            {
                sc_logr_debug(NULL, SC_ESL, "Test service control rule: FAIL, Request Attr1 Value: %u, Attr1 Value: %u(BUCKET 0)"
                                , ulAttrVal1, pstSrvCtrl->ulAttrValue1);
                continue;
            }

            /* 检查属性值，不为U32_BUTT才检查 */
            if (U32_BUTT != ulAttrVal2
                && ulAttrVal2 != pstSrvCtrl->ulAttrValue2)
            {
                sc_logr_debug(NULL, SC_ESL, "Test service control rule: FAIL, Request Attr2 Value: %u, Attr2 Value: %u(BUCKET 0)"
                                , ulAttrVal2, pstSrvCtrl->ulAttrValue2);
                continue;
            }

            /* 所有向均匹配 */
            sc_logr_debug(NULL, SC_ESL, "Test service control rule: SUCC. ID: %u(BUCKET 0)", pstSrvCtrl->ulID);

            blResult = DOS_TRUE;
            break;
        }
    }

    sc_logr_info(NULL, SC_ESL, "Match service control rule %s.", blResult ? "SUCC" : "FAIL");


    pthread_mutex_unlock(&g_mutexHashServCtrl);

    return blResult;
}


U32 sc_ep_reloadxml()
{
    sc_ep_esl_execute_cmd("api reloadxml\r\n");


    return DOS_SUCC;
}

U32 sc_ep_update_gateway(VOID *pData)
{
    SC_PUB_FS_DATA_ST *pstData;
    S8 szBuff[100] = { 0, };

    pstData = pData;
    if (DOS_ADDR_INVALID(pstData))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    switch (pstData->ulAction)
    {
        case SC_API_CMD_ACTION_GATEWAY_ADD:
            sc_ep_esl_execute_cmd("bgapi sofia profile external rescan");
            break;

        case SC_API_CMD_ACTION_GATEWAY_DELETE:
            dos_snprintf(szBuff, sizeof(szBuff), "bgapi sofia profile external killgw %u", pstData->ulID);
            sc_ep_esl_execute_cmd(szBuff);
            break;

        case SC_API_CMD_ACTION_GATEWAY_UPDATE:
        case SC_API_CMD_ACTION_GATEWAY_SYNC:
            dos_snprintf(szBuff, sizeof(szBuff), "bgapi sofia profile external killgw %u", pstData->ulID);
            sc_ep_esl_execute_cmd(szBuff);
            sc_ep_esl_execute_cmd("bgapi sofia profile external rescan");
            break;
    }

    return DOS_SUCC;
}

U32 sc_find_gateway_by_addr(const S8 *pszAddr)
{
    HASH_NODE_S   *pstHashNode = NULL;
    SC_GW_NODE_ST *pstGWNode   = NULL;
    U32   ulHashIndex = U32_BUTT;
    U32   ulLen = 0;

    if (DOS_ADDR_INVALID(pszAddr))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    HASH_Scan_Table(g_pstHashGW,ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashGW, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstGWNode = (SC_GW_NODE_ST *)pstHashNode->pHandle;
            if (DOS_FALSE == pstGWNode->bExist)
            {
                continue;
            }

            ulLen = dos_strlen(pszAddr);
            if (ulLen < SC_GW_DOMAIN_LEG
                && pstGWNode->szGWDomain[ulLen] == ':'
                && dos_strncmp(pstGWNode->szGWDomain, pszAddr, ulLen) == 0)
            {
                return DOS_SUCC;
            }
        }
    }

    return DOS_FAIL;
}

/**
  * 函数名: U32 sc_del_invalid_gateway()
  * 参数:
  * 功能: 删除掉内存中残留但数据库没有的数据
  * 返回: 成功返回DOS_SUCC，失败返回DOS_FAIL
  **/
U32 sc_del_invalid_gateway()
{
    HASH_NODE_S   *pstHashNode = NULL;
    SC_GW_NODE_ST *pstGWNode   = NULL;
    U32   ulHashIndex = U32_BUTT;
    U32   ulGWID = U32_BUTT, ulRet = U32_BUTT;
    S8    szBuff[64] = {0};

    HASH_Scan_Table(g_pstHashGW,ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashGW, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }
            pstGWNode = (SC_GW_NODE_ST *)pstHashNode->pHandle;
            /* 如果说内存里有该条记录，但数据库没有该数据，删之 */
            if (DOS_FALSE == pstGWNode->bExist)
            {
                /* 记录网关id */
                ulGWID = pstGWNode->ulGWID;

#if INCLUDE_SERVICE_PYTHON
                /* FreeSWITCH删除该配置数据 */
                ulRet = py_exec_func("router", "del_route", "(k)", (U64)ulGWID);
                if (DOS_SUCC != ulRet)
                {
                    DOS_ASSERT(0);
                    return DOS_FAIL;
                }
#endif

                /* FreeSWITCH删除内存中该数据 */
                dos_snprintf(szBuff, sizeof(szBuff), "bgapi sofia profile external killgw %u", ulGWID);
                ulRet = sc_ep_esl_execute_cmd(szBuff);
                if (DOS_SUCC != ulRet)
                {
                    DOS_ASSERT(0);
                }

                /* 从节点中删除数据 */
                hash_delete_node(g_pstHashGW, pstHashNode, ulHashIndex);
                if (DOS_ADDR_VALID(pstGWNode))
                {
                    dos_dmem_free(pstGWNode);
                    pstGWNode = NULL;
                }
                if (DOS_ADDR_VALID(pstHashNode))
                {
                    dos_dmem_free(pstHashNode);
                    pstHashNode = NULL;
                }
            }
        }
    }
    return DOS_SUCC;
}

/**
  * 函数名: U32 sc_del_invalid_route()
  * 参数:
  * 功能: 删除掉内存中残留但数据库没有的数据
  * 返回: 成功返回DOS_SUCC，失败返回DOS_FAIL
  **/
U32 sc_del_invalid_route()
{
    SC_ROUTE_NODE_ST  *pstRoute = NULL;
    DLL_NODE_S        *pstNode  = NULL;

    DLL_Scan(&g_stRouteList, pstNode, DLL_NODE_S *)
    {
        if (DOS_ADDR_INVALID(pstNode)
            || DOS_ADDR_INVALID(pstNode->pHandle))
        {
            continue;
        }

        pstRoute = (SC_ROUTE_NODE_ST *)pstNode->pHandle;
        /* 如果说该数据并非来自数据库，删之 */
        if (DOS_FALSE == pstRoute->bExist)
        {
            dll_delete(&g_stRouteList, pstNode);
            if (DOS_ADDR_VALID(pstRoute))
            {
                dos_dmem_free(pstRoute);
                pstRoute = NULL;
            }
            if (DOS_ADDR_VALID(pstNode))
            {
                dos_dmem_free(pstNode);
                pstNode = NULL;
            }
        }
        else
        {
            /* 并将所有的节点该标志置为false */
            pstRoute->bExist = DOS_FALSE;
        }
    }

    return DOS_SUCC;
}

U32 sc_del_invalid_gateway_grp()
{
    SC_GW_GRP_NODE_ST * pstGWGrp = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    U32  ulHashIndex = 0;

    HASH_Scan_Table(g_pstHashGWGrp, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashGWGrp, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstGWGrp = (SC_GW_GRP_NODE_ST *)pstHashNode->pHandle;
            if (DOS_FALSE == pstGWGrp->bExist)
            {
                /* 删除之 */
                sc_gateway_grp_delete(pstGWGrp->bExist);
            }
            else
            {
                pstGWGrp->bExist = DOS_FALSE;
            }
        }
    }
    return DOS_SUCC;
}

U32 sc_del_tt_number(U32 ulIndex)
{
    HASH_NODE_S    *pstHashNode;
    U32            ulHashIndex;

    pthread_mutex_lock(&g_mutexHashTTNumber);
    ulHashIndex = sc_ep_tt_hash_func(ulIndex);
    pstHashNode = hash_find_node(g_pstHashTTNumber, ulHashIndex, &ulIndex, sc_ep_tt_list_find);
    if (DOS_ADDR_VALID(pstHashNode))
    {
        hash_delete_node(g_pstHashTTNumber, pstHashNode, ulHashIndex);

        dos_dmem_free(pstHashNode->pHandle);
        pstHashNode->pHandle = NULL;
        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;
        pthread_mutex_unlock(&g_mutexHashTTNumber);
        return DOS_SUCC;
    }
    else
    {
        pthread_mutex_unlock(&g_mutexHashTTNumber);

        return DOS_FAIL;
    }
}

U32 sc_save_number_stat(U32 ulCustomerID, U32 ulType, S8 *pszNumber, U32 ulTimes)
{
    S8        szSQL[512];

    if (ulType >= SC_NUM_LMT_TYPE_BUTT
        || DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_snprintf(szSQL, sizeof(szSQL)
                    , "INSERT INTO tbl_stat_caller(customer_id, type, ctime, caller, times) VALUE(%u, %u, %u, \"%s\", %u)"
                    , ulCustomerID, ulType, time(NULL), pszNumber, ulTimes);

    if (db_query(g_pstSCDBHandle, szSQL, NULL, NULL, NULL) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

S32 sc_update_number_stat_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    U32 ulTimes;

    if (DOS_ADDR_INVALID(pArg)
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(aszValues[0])
        || DOS_ADDR_INVALID(aszNames[0]))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if ('\0' == aszValues[0][0]
        || '\0' == aszNames[0][0]
        || dos_strcmp(aszNames[0], "times")
        || dos_atoul(aszValues[0], &ulTimes) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    *(U32 *)pArg = ulTimes;

    return DOS_SUCC;
}

U32 sc_update_number_stat(U32 ulType, U32 ulCycle, S8 *pszNumber)
{
    U32 ulStartTimestamp;
    U32 ulTimes;
    time_t    ulTime;
    struct tm *pstTime;
    struct tm stStartTime;
    S8        szSQL[512];

    if (ulType >= SC_NUM_LMT_TYPE_BUTT
        || ulCycle >= SC_NUMBER_LMT_CYCLE_BUTT
        || DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulTime = time(NULL);
    pstTime = localtime(&ulTime);
    stStartTime.tm_sec   = 59;
    stStartTime.tm_min   = 59;
    stStartTime.tm_hour  = 23;
    stStartTime.tm_wday  = 0;
    stStartTime.tm_yday  = 0;
    stStartTime.tm_isdst = 0;

    switch (ulCycle)
    {
        case SC_NUMBER_LMT_CYCLE_DAY:
            stStartTime.tm_mday  = pstTime->tm_mday - 1;
            stStartTime.tm_mon   = pstTime->tm_mon;
            stStartTime.tm_year  = pstTime->tm_year;
            break;

        case SC_NUMBER_LMT_CYCLE_WEEK:
            stStartTime.tm_mday  = pstTime->tm_mday - pstTime->tm_wday;
            stStartTime.tm_mon   = pstTime->tm_mon;
            stStartTime.tm_year  = pstTime->tm_year;
            break;

        case SC_NUMBER_LMT_CYCLE_MONTH:
            stStartTime.tm_mday  = 0;
            stStartTime.tm_mon   = pstTime->tm_mon;
            stStartTime.tm_year  = pstTime->tm_year;
            break;

        case SC_NUMBER_LMT_CYCLE_YEAR:
            stStartTime.tm_mday  = 0;
            stStartTime.tm_mon   = 0;
            stStartTime.tm_year  = pstTime->tm_year;
            break;

        default:
            DOS_ASSERT(0);
            return 0;
    }

    ulStartTimestamp = mktime(&stStartTime);

    dos_snprintf(szSQL, sizeof(szSQL)
        , "SELECT SUM(times) AS times FROM tbl_stat_caller WHERE ctime > %u AND type=%u AND caller=%s"
        , ulStartTimestamp, ulType, pszNumber);

    if (db_query(g_pstSCDBHandle, szSQL, sc_update_number_stat_cb, &ulTimes, NULL) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return 0;
    }

    return ulTimes;
}

U32 sc_num_lmt_stat(U32 ulType, VOID *ptr)
{
    U32 ulHashIndex;
    U32 ulHashIndexMunLmt;
    U32 ulTimes = 0;
    HASH_NODE_S             *pstHashNodeNumber = NULL;
    HASH_NODE_S             *pstHashNodeNumLmt = NULL;
    SC_CALLER_QUERY_NODE_ST *ptCaller          = NULL;
    SC_DID_NODE_ST          *ptDIDNumber       = NULL;
    SC_NUMBER_LMT_NODE_ST   *pstNumLmt         = NULL;

    pthread_mutex_lock(&g_mutexHashCaller);
    HASH_Scan_Table(g_pstHashCaller, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashCaller, ulHashIndex, pstHashNodeNumber, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNodeNumber)
                || DOS_ADDR_INVALID(pstHashNodeNumber->pHandle))
            {
                continue;
            }

            ptCaller = pstHashNodeNumber->pHandle;
            ulTimes = ptCaller->ulTimes;
            ptCaller->ulTimes = 0;

            sc_save_number_stat(ptCaller->ulCustomerID, SC_NUM_LMT_TYPE_CALLER, ptCaller->szNumber, ulTimes);

            ulHashIndexMunLmt = sc_ep_number_lmt_hash_func(ptCaller->szNumber);
            pthread_mutex_lock(&g_mutexHashNumberlmt);
            pstHashNodeNumLmt= hash_find_node(g_pstHashNumberlmt, ulHashIndexMunLmt, ptCaller->szNumber, sc_ep_number_lmt_find);
            if (DOS_ADDR_INVALID(pstHashNodeNumLmt)
                || DOS_ADDR_INVALID(pstHashNodeNumLmt->pHandle))
            {
                pthread_mutex_unlock(&g_mutexHashNumberlmt);
                continue;
            }

            pstNumLmt = pstHashNodeNumLmt->pHandle;
            pstNumLmt->ulStatUsed += ulTimes;

            pthread_mutex_unlock(&g_mutexHashNumberlmt);
        }
    }
    pthread_mutex_unlock(&g_mutexHashCaller);

    pthread_mutex_lock(&g_mutexHashDIDNum);
    HASH_Scan_Table(g_pstHashDIDNum, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashDIDNum, ulHashIndex, pstHashNodeNumber, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNodeNumber)
                || DOS_ADDR_INVALID(pstHashNodeNumber->pHandle))
            {
                continue;
            }

            ptDIDNumber = pstHashNodeNumber->pHandle;
            if (DOS_FALSE == ptDIDNumber->bValid)
            {
                continue;
            }
            ulTimes = ptDIDNumber->ulTimes;
            ptDIDNumber->ulTimes = 0;

            sc_save_number_stat(ptDIDNumber->ulCustomID, SC_NUM_LMT_TYPE_DID, ptDIDNumber->szDIDNum, ulTimes);

            ulHashIndexMunLmt = sc_ep_number_lmt_hash_func(ptDIDNumber->szDIDNum);
            pthread_mutex_lock(&g_mutexHashNumberlmt);
            pstHashNodeNumLmt= hash_find_node(g_pstHashNumberlmt, ulHashIndexMunLmt, ptDIDNumber->szDIDNum, sc_ep_number_lmt_find);
            if (DOS_ADDR_INVALID(pstHashNodeNumLmt)
                || DOS_ADDR_INVALID(pstHashNodeNumLmt->pHandle))
            {
                pthread_mutex_unlock(&g_mutexHashNumberlmt);
                continue;
            }

            pstNumLmt = pstHashNodeNumLmt->pHandle;
            pstNumLmt->ulStatUsed += ulTimes;
            pthread_mutex_unlock(&g_mutexHashNumberlmt);
        }
    }
    pthread_mutex_unlock(&g_mutexHashDIDNum);

    return DOS_SUCC;
}

U32 sc_num_lmt_update(U32 ulType, VOID *ptr)
{
    U32 ulHashIndex;
    U32 ulTimes = 0;
    HASH_NODE_S             *pstHashNode       = NULL;
    SC_NUMBER_LMT_NODE_ST   *pstNumLmt         = NULL;

    pthread_mutex_lock(&g_mutexHashNumberlmt);

    HASH_Scan_Table(g_pstHashNumberlmt, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashNumberlmt, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstNumLmt = pstHashNode->pHandle;

            ulTimes = sc_update_number_stat(pstNumLmt->ulType, pstNumLmt->ulCycle, pstNumLmt->szPrefix);
            pstNumLmt->ulStatUsed = ulTimes;
        }
    }

    pthread_mutex_unlock(&g_mutexHashNumberlmt);

    return DOS_SUCC;
}

BOOL sc_num_lmt_check(U32 ulType, U32 ulCurrentTime, S8 *pszNumber)
{
    U32  ulHashIndexMunLmt  = 0;
    U32  ulHandleType       = 0;
    HASH_NODE_S             *pstHashNodeNumLmt = NULL;
    SC_NUMBER_LMT_NODE_ST   *pstNumLmt         = NULL;

    if (DOS_ADDR_INVALID(pszNumber)
        || '\0' == pszNumber[0]
        || ulType >= SC_NUM_LMT_TYPE_BUTT)
    {
        DOS_ASSERT(0);

        return DOS_TRUE;
    }

    ulHashIndexMunLmt = sc_ep_number_lmt_hash_func(pszNumber);
    pthread_mutex_lock(&g_mutexHashNumberlmt);
    pstHashNodeNumLmt= hash_find_node(g_pstHashNumberlmt, ulHashIndexMunLmt, pszNumber, sc_ep_number_lmt_find);
    if (DOS_ADDR_INVALID(pstHashNodeNumLmt)
        || DOS_ADDR_INVALID(pstHashNodeNumLmt->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashNumberlmt);

        sc_logr_debug(NULL, SC_ESL
                , "Number limit check for \"%s\", There is no limitation for this number."
                , pszNumber);
        return DOS_TRUE;
    }

    pstNumLmt = (SC_NUMBER_LMT_NODE_ST *)pstHashNodeNumLmt->pHandle;
    if (pstNumLmt->ulStatUsed + ulCurrentTime < pstNumLmt->ulLimit)
    {
        sc_logr_debug(NULL, SC_ESL
                , "Number limit check for \"%s\". This number did not reached the limitation. Cycle: %u, Limitation: %u, Used: %u"
                , pszNumber, pstNumLmt->ulCycle, pstNumLmt->ulLimit, pstNumLmt->ulStatUsed + ulCurrentTime);

        pthread_mutex_unlock(&g_mutexHashNumberlmt);
        return DOS_TRUE;
    }

    ulHandleType = pstNumLmt->ulHandle;

    pthread_mutex_unlock(&g_mutexHashNumberlmt);

    sc_logr_notice(NULL, SC_ESL
            , "Number limit check for \"%s\", This number has reached the limitation. Process as handle: %u"
            , pszNumber, pstNumLmt->ulHandle);

    switch (ulHandleType)
    {
        case SC_NUM_LMT_HANDLE_REJECT:
            return DOS_FALSE;
            break;

        default:
            DOS_ASSERT(0);
            return DOS_TRUE;
    }

    return DOS_TRUE;
}


U32 sc_ep_get_callout_group_by_customerID(U32 ulCustomerID, U16 *pusCallOutGroup)
{
    SC_CUSTOMER_NODE_ST  *pstCustomer       = NULL;
    DLL_NODE_S           *pstListNode       = NULL;

    if (DOS_ADDR_INVALID(pusCallOutGroup))
    {
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexCustomerList);
    pstListNode = dll_find(&g_stCustomerList, &ulCustomerID, sc_ep_customer_find);
    if (DOS_ADDR_VALID(pstListNode)
        && DOS_ADDR_VALID(pstListNode->pHandle))
    {
        pstCustomer = (SC_CUSTOMER_NODE_ST *)pstListNode->pHandle;
        *pusCallOutGroup = pstCustomer->usCallOutGroup;
        pthread_mutex_unlock(&g_mutexCustomerList);

        return DOS_SUCC;
    }

    pthread_mutex_unlock(&g_mutexCustomerList);

    return DOS_FAIL;
}

U32 sc_ep_customer_list_find(U32 ulCustomerID)
{
    SC_CUSTOMER_NODE_ST  *pstCustomer       = NULL;
    DLL_NODE_S           *pstListNode       = NULL;

    pthread_mutex_lock(&g_mutexCustomerList);
    DLL_Scan(&g_stCustomerList, pstListNode, DLL_NODE_S *)
    {
        pstCustomer = (SC_CUSTOMER_NODE_ST *)pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstCustomer))
        {
            continue;
        }

        if (pstCustomer->ulID == ulCustomerID)
        {
            pthread_mutex_unlock(&g_mutexCustomerList);
            return DOS_SUCC;
        }
    }

    pthread_mutex_unlock(&g_mutexCustomerList);

    return DOS_FAIL;
}

/*
 * 函  数: U32 sc_ep_num_transform(SC_SCB_ST *pstSCB, U32 ulTrunkID, SC_NUM_TRANSFORM_TIMING_EN enTiming, SC_NUM_TRANSFORM_DIRECTION_EN enDirection, SC_NUM_TRANSFORM_SELECT_EN enNumSelect)
 * 功  能: 号码变换
 * 参  数:
 *      SC_SCB_ST *pstSCB                           : 呼叫控制块
 *      U32 ulTrunkID                               : 中继ID,(只有路由后才会匹配中继)
 *      SC_NUM_TRANSFORM_TIMING_EN      enTiming    : 路由前/路由后
 *      SC_NUM_TRANSFORM_DIRECTION_EN   enDirection : 呼入/呼出
 *      SC_NUM_TRANSFORM_SELECT_EN      enNumSelect : 主被叫选择
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_ep_num_transform(SC_SCB_ST *pstSCB, U32 ulTrunkID, SC_NUM_TRANSFORM_TIMING_EN enTiming, SC_NUM_TRANSFORM_SELECT_EN enNumSelect)
{
    SC_NUM_TRANSFORM_NODE_ST        *pstNumTransform        = NULL;
    SC_NUM_TRANSFORM_NODE_ST        *pstNumTransformEntry   = NULL;
    DLL_NODE_S                      *pstListNode            = NULL;
    SC_NUM_TRANSFORM_DIRECTION_EN   enDirection;
    S8  szNeedTransformNum[SC_TEL_NUMBER_LENGTH]        = {0};
    U32 ulIndex         = 0;
    S32 lIndex          = 0;
    U32 ulNumLen        = 0;
    U32 ulOffsetLen     = 0;
    U32 ulCallerGrpID   = 0;
    time_t ulCurrTime   = time(NULL);

    if (DOS_ADDR_INVALID(pstSCB)
        || '\0' == pstSCB->szCalleeNum[0]
        || '\0' == pstSCB->szCallerNum[0]
        || enTiming >= SC_NUM_TRANSFORM_TIMING_BUTT
        || enNumSelect >= SC_NUM_TRANSFORM_SELECT_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_logr_debug(pstSCB, SC_DIALER, "Search number transfer rule for the task %d, timing is : %d, number select : %d"
                                , pstSCB->ulTaskID, enTiming, enNumSelect);

    /* 判断一下呼叫方向 呼入/呼出 */
    if (pstSCB->ucLegRole == SC_CALLEE && pstSCB->bIsAgentCall)
    {
        /* 被叫是坐席则为呼入, 否则为呼出 */
        enDirection = SC_NUM_TRANSFORM_DIRECTION_IN;
    }
    else
    {
        enDirection = SC_NUM_TRANSFORM_DIRECTION_OUT;
    }

    sc_logr_debug(pstSCB, SC_DIALER, "call firection : %d, pstSCB->ucLegRole : %d, pstSCB->bIsAgentCall : %d", enDirection, pstSCB->ucLegRole, pstSCB->bIsAgentCall);

    /* 遍历号码变换规则的链表，查找没有过期的，优先级别高的，针对这个客户或者系统的变换规则。
        先按优先级，同优先级，客户优先于中继，中继优先于系统 */
    pthread_mutex_lock(&g_mutexNumTransformList);
    DLL_Scan(&g_stNumTransformList, pstListNode, DLL_NODE_S *)
    {
        pstNumTransformEntry = (SC_NUM_TRANSFORM_NODE_ST *)pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstNumTransformEntry))
        {
            continue;
        }

        /* 判断有效期 */
        if (pstNumTransformEntry->ulExpiry < ulCurrTime)
        {
            continue;
        }

        /* 判断 路由前/后 */
        if (pstNumTransformEntry->enTiming != enTiming)
        {
            continue;
        }
        /* 判断 呼叫方向 */
        if (pstNumTransformEntry->enDirection != enDirection)
        {
            continue;
        }
        /* 判断主被叫 */
        if (pstNumTransformEntry->enNumSelect != enNumSelect)
        {
            continue;
        }

        /* 判断主叫号码前缀 */
        if ('\0' != pstNumTransformEntry->szCallerPrefix[0])
        {
            if (0 != dos_strnicmp(pstNumTransformEntry->szCallerPrefix, pstSCB->szCallerNum, dos_strlen(pstNumTransformEntry->szCallerPrefix)))
            {
                continue;
            }
        }

        /* 判断被叫号码前缀 */
        if ('\0' != pstNumTransformEntry->szCalleePrefix[0])
        {
            if (0 != dos_strnicmp(pstNumTransformEntry->szCalleePrefix, pstSCB->szCalleeNum, dos_strlen(pstNumTransformEntry->szCalleePrefix)))
            {
                continue;
            }
        }

        sc_logr_debug(pstSCB, SC_DIALER, "Call Object : %d", pstNumTransformEntry->enObject);

        if (SC_NUM_TRANSFORM_OBJECT_CUSTOMER == pstNumTransformEntry->enObject)
        {
            /* 针对客户 */
            if (pstNumTransformEntry->ulObjectID == pstSCB->ulCustomID)
            {
                if (DOS_ADDR_INVALID(pstNumTransform))
                {
                    pstNumTransform = pstNumTransformEntry;
                    continue;
                }

                if (pstNumTransformEntry->enPriority < pstNumTransform->enPriority)
                {
                    /* 选择优先级高的 */
                    pstNumTransform = pstNumTransformEntry;

                    continue;
                }

                if (pstNumTransformEntry->enPriority == pstNumTransform->enPriority && pstNumTransform->enObject != SC_NUM_TRANSFORM_OBJECT_CUSTOMER)
                {
                    /* 优先级相同，选择客户的 */
                    pstNumTransform = pstNumTransformEntry;

                    continue;
                }
            }
        }
        else if (SC_NUM_TRANSFORM_OBJECT_SYSTEM == pstNumTransformEntry->enObject)
        {
            /* 针对系统 */
            if (DOS_ADDR_INVALID(pstNumTransform))
            {
                pstNumTransform = pstNumTransformEntry;

                continue;
            }

            if (pstNumTransformEntry->enPriority < pstNumTransform->enPriority)
            {
                /* 选择优先级高的 */
                pstNumTransform = pstNumTransformEntry;

                continue;
            }
        }
        else if (SC_NUM_TRANSFORM_OBJECT_TRUNK == pstNumTransformEntry->enObject)
        {
            /* 针对中继，只有路由后，才需要判断这种情况 */
            if (enTiming == SC_NUM_TRANSFORM_TIMING_AFTER)
            {
                if (pstNumTransformEntry->ulObjectID != ulTrunkID)
                {
                    continue;
                }

                if (DOS_ADDR_INVALID(pstNumTransform))
                {
                    pstNumTransform = pstNumTransformEntry;
                    continue;
                }

                if (pstNumTransformEntry->enPriority < pstNumTransform->enPriority)
                {
                    /* 选择优先级高的 */
                    pstNumTransform = pstNumTransformEntry;

                    continue;
                }

                if (pstNumTransformEntry->enPriority == pstNumTransform->enPriority && pstNumTransform->enObject == SC_NUM_TRANSFORM_OBJECT_SYSTEM)
                {
                    /* 优先级相同，如果是系统的，则换成中继的 */
                    pstNumTransform = pstNumTransformEntry;

                    continue;
                }
            }
        }
    }

    if (DOS_ADDR_INVALID(pstNumTransform))
    {
        /* 没有找到合适的变换规则 */
        sc_logr_debug(pstSCB, SC_DIALER, "Not find number transfer rule for the task %d, timing is : %d, number select : %d"
                                , pstSCB->ulTaskID, enTiming, enNumSelect);

        goto succ;
    }

    sc_logr_debug(pstSCB, SC_DIALER, "Find a number transfer rule(%d) for the task %d, timing is : %d, number select : %d"
                                , pstNumTransform->ulID, pstSCB->ulTaskID, enTiming, enNumSelect);

    if (SC_NUM_TRANSFORM_SELECT_CALLER == pstNumTransform->enNumSelect)
    {
        dos_strncpy(szNeedTransformNum, pstSCB->szCallerNum, SC_TEL_NUMBER_LENGTH);
    }
    else
    {
        dos_strncpy(szNeedTransformNum, pstSCB->szCalleeNum, SC_TEL_NUMBER_LENGTH);
    }

    szNeedTransformNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';

    /* 根据找到的规则变换号码 */
    if (pstNumTransform->bReplaceAll)
    {
        /* 完全替代 */
        if (pstNumTransform->szReplaceNum[0] == '*')
        {
            /* 使用号码组中的号码 */
            if (SC_NUM_TRANSFORM_OBJECT_CUSTOMER != pstNumTransform->enObject || enNumSelect != SC_NUM_TRANSFORM_SELECT_CALLER)
            {
                /* 只有企业客户 和 变换主叫号码时，才可以选择号码组中的号码进行替换 */
                sc_logr_info(pstSCB, SC_DIALER, "Number transfer rule(%d) for the task %d fail : only a enterprise customers can, choose number in the group number"
                                , pstNumTransform->ulID, pstSCB->ulTaskID, enTiming, enNumSelect);

                goto fail;
            }

            if (dos_atoul(&pstNumTransform->szReplaceNum[1], &ulCallerGrpID) < 0)
            {
                sc_logr_info(pstSCB, SC_DIALER, "Number transfer rule(%d), get caller group id fail.", pstNumTransform->ulID);

                goto fail;
            }

            if (sc_select_number_in_order(pstSCB->ulCustomID, ulCallerGrpID, szNeedTransformNum, SC_TEL_NUMBER_LENGTH) != DOS_SUCC)
            {
                sc_logr_info(pstSCB, SC_DIALER, "Number transfer rule(%d), get caller from group id(%u) fail.", pstNumTransform->ulID, ulCallerGrpID);

                goto fail;
            }

            sc_logr_debug(pstSCB, SC_DIALER, "Number transfer rule(%d), get caller(%s) from group id(%u) succ.", pstNumTransform->ulID, szNeedTransformNum, ulCallerGrpID);

        }
        else if (pstNumTransform->szReplaceNum[0] == '\0')
        {
            /* 完全替换的号码不能为空 */
            sc_logr_info(pstSCB, SC_DIALER, "The number transfer rule(%d) replace num is NULL!"
                                , pstNumTransform->ulID, pstSCB->ulTaskID, enTiming, enNumSelect);

            goto fail;
        }
        else
        {
            dos_strcpy(szNeedTransformNum, pstNumTransform->szReplaceNum);
        }

        goto succ;
    }

    /* 删除左边几位 */
    ulOffsetLen = pstNumTransform->ulDelLeft;
    if (ulOffsetLen != 0)
    {
        ulNumLen = dos_strlen(szNeedTransformNum);

        if (ulOffsetLen >= ulNumLen)
        {
            /* 删除的位数大于号码的长度，整个号码置空 */
            szNeedTransformNum[0] = '\0';
        }
        else
        {
            for (ulIndex=ulOffsetLen; ulIndex<=ulNumLen; ulIndex++)
            {
                szNeedTransformNum[ulIndex-ulOffsetLen] = szNeedTransformNum[ulIndex];
            }
        }
    }

    /* 删除右边几位 */
    ulOffsetLen = pstNumTransform->ulDelRight;
    if (ulOffsetLen != 0)
    {
        ulNumLen = dos_strlen(szNeedTransformNum);

        if (ulOffsetLen >= ulNumLen)
        {
            /* 删除的位数大于号码的长度，整个号码置空 */
            szNeedTransformNum[0] = '\0';
        }
        else
        {
            szNeedTransformNum[ulNumLen-ulOffsetLen] = '\0';
        }
    }

    /* 增加前缀 */
    if (pstNumTransform->szAddPrefix[0] != '\0')
    {
        ulNumLen = dos_strlen(szNeedTransformNum);
        ulOffsetLen = dos_strlen(pstNumTransform->szAddPrefix);
        if (ulNumLen + ulOffsetLen >= SC_TEL_NUMBER_LENGTH)
        {
            /* 超过号码的长度，失败 */

            goto fail;
        }

        for (lIndex=ulNumLen; lIndex>=0; lIndex--)
        {
            szNeedTransformNum[lIndex+ulOffsetLen] = szNeedTransformNum[lIndex];
        }

        dos_strncpy(szNeedTransformNum, pstNumTransform->szAddPrefix, ulOffsetLen);
    }

    /* 增加后缀 */
    if (pstNumTransform->szAddSuffix[0] != '\0')
    {
        ulNumLen = dos_strlen(szNeedTransformNum);
        ulOffsetLen = dos_strlen(pstNumTransform->szAddSuffix);
        if (ulNumLen + ulOffsetLen >= SC_TEL_NUMBER_LENGTH)
        {
            /* 超过号码的长度，失败 */

            goto fail;
        }

        dos_strcat(szNeedTransformNum, pstNumTransform->szAddSuffix);
    }

    if (szNeedTransformNum[0] == '\0')
    {
        goto fail;
    }
    szNeedTransformNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';

succ:

    if (DOS_ADDR_INVALID(pstNumTransform))
    {
        pthread_mutex_unlock(&g_mutexNumTransformList);

        return DOS_SUCC;
    }

    if (SC_NUM_TRANSFORM_SELECT_CALLER == pstNumTransform->enNumSelect)
    {
        sc_logr_debug(pstSCB, SC_DIALER, "The number transfer(%d) SUCC, task : %d, befor : %s ,after : %s", pstNumTransform->ulID, pstSCB->ulTaskID, pstSCB->szCallerNum, szNeedTransformNum);
        dos_strcpy(pstSCB->szCallerNum, szNeedTransformNum);
    }
    else
    {
        sc_logr_debug(pstSCB, SC_DIALER, "The number transfer(%d) SUCC, task : %d, befor : %s ,after : %s", pstNumTransform->ulID, pstSCB->ulTaskID, pstSCB->szCalleeNum, szNeedTransformNum);
        dos_strcpy(pstSCB->szCalleeNum, szNeedTransformNum);
    }

    pthread_mutex_unlock(&g_mutexNumTransformList);

    return DOS_SUCC;

fail:

    sc_logr_info(pstSCB, SC_DIALER, "the number transfer(%d) FAIL, task : %d", pstNumTransform->ulID, pstSCB->ulTaskID);
    if (SC_NUM_TRANSFORM_SELECT_CALLER == pstNumTransform->enNumSelect)
    {
        pstSCB->szCallerNum[0] = '\0';
    }
    else
    {
        pstSCB->szCalleeNum[0] = '\0';
    }

    pthread_mutex_unlock(&g_mutexNumTransformList);

    return DOS_FAIL;
}

U32 sc_ep_get_eix_by_tt(S8 *pszTTNumber, S8 *pszEIX, U32 ulLength)
{
    U32            ulHashIndex   = 0;
    HASH_NODE_S    *pstHashNode  = NULL;
    SC_TT_NODE_ST  *pstTTNumber  = NULL;
    SC_TT_NODE_ST  *pstTTNumberLast  = NULL;

    if (DOS_ADDR_INVALID(pszTTNumber)
        || DOS_ADDR_INVALID(pszEIX)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashTTNumber);
    HASH_Scan_Table(g_pstHashTTNumber, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashTTNumber, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode ) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstTTNumber = pstHashNode->pHandle;
            /* 匹配前缀 */
            if (0 == dos_strncmp(pszTTNumber
                , pstTTNumber->szPrefix
                , dos_strlen(pstTTNumber->szPrefix)))
            {
                /* 要做一个最优匹配(匹配长度越大越优) */
                if (pstTTNumberLast)
                {
                    if (dos_strlen(pstTTNumberLast->szPrefix) < dos_strlen(pstTTNumber->szPrefix))
                    {
                        pstTTNumberLast = pstTTNumber;
                    }
                }
                else
                {
                    pstTTNumberLast = pstTTNumber;
                }
            }
        }
    }

    if (pstTTNumberLast)
    {
        dos_snprintf(pszEIX, ulLength, "%s:%u", pstTTNumberLast->szAddr, pstTTNumberLast->ulPort);
    }
    else
    {
        pthread_mutex_unlock(&g_mutexHashTTNumber);

        sc_logr_info(NULL, SC_ESL, "Cannot find the EIA for the TT number %s", pszTTNumber);
        return DOS_FAIL;
    }

    pthread_mutex_unlock(&g_mutexHashTTNumber);

    sc_logr_info(NULL, SC_ESL, "Found the EIA(%s) for the TT number(%s).", pszEIX, pszTTNumber);
    return DOS_SUCC;
}

/**
 * 函数: U32 sc_ep_esl_execute(const S8 *pszApp, const S8 *pszArg, const S8 *pszUUID)
 * 功能: 使用pstHandle所指向的ESL句柄指向命令pszApp，参数为pszArg，对象为pszUUID
 * 参数:
 *      esl_handle_t *pstHandle: ESL句柄
 *      const S8 *pszApp: 执行的命令
 *      const S8 *pszArg: 命令参数
 *      const S8 *pszUUID: channel的UUID
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * 注意: 当该函数在执行命令时，如果发现当前句柄已经失去连接，将会重新连接ESL服务器
 */
U32 sc_ep_esl_execute(const S8 *pszApp, const S8 *pszArg, const S8 *pszUUID)
{
    U32 ulRet;

    if (DOS_ADDR_INVALID(pszApp)
        || DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (!g_pstHandle->stSendHandle.connected)
    {
        ulRet = esl_connect(&g_pstHandle->stSendHandle, "127.0.0.1", 8021, NULL, "ClueCon");
        if (ESL_SUCCESS != ulRet)
        {
            esl_disconnect(&g_pstHandle->stSendHandle);
            sc_logr_notice(NULL, SC_ESL, "ELS for send event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_pstHandle->stSendHandle.err);

            DOS_ASSERT(0);

            return DOS_FAIL;
        }

        g_pstHandle->stSendHandle.event_lock = 1;
    }

    if (ESL_SUCCESS != esl_execute(&g_pstHandle->stSendHandle, pszApp, pszArg, pszUUID))
    {
        DOS_ASSERT(0);
        sc_logr_notice(NULL, SC_ESL, "ESL execute command fail. Result:%d, APP: %s, ARG : %s, UUID: %s"
                        , ulRet
                        , pszApp
                        , DOS_ADDR_VALID(pszArg) ? pszArg : "NULL"
                        , DOS_ADDR_VALID(pszUUID) ? pszUUID : "NULL");

        return DOS_FAIL;
    }

    sc_logr_debug(NULL, SC_ESL, "ESL execute command SUCC. APP: %s, Param: %s, UUID: %s"
                    , pszApp
                    , DOS_ADDR_VALID(pszArg) ? pszArg : "NULL"
                    , pszUUID);

    return DOS_SUCC;
}

U32 sc_ep_play_sound(U32 ulSoundType, S8 *pszUUID, S32 lTime)
{
    S8 *pszFileName = NULL;
    S8 szFileName[256] = { 0, };
    S8 szParams[256] = { 0, };

    if (ulSoundType >= SC_SND_BUTT || DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    switch (ulSoundType)
    {
        case SC_SND_CALL_OVER:
            pszFileName = "call_over";
            break;
        case SC_SND_INCOMING_CALL_TIP:
            pszFileName = "incoming_tip";
            break;
        case SC_SND_LOW_BALANCE:
            pszFileName = "low_balance";
            break;
        case SC_SND_MUSIC_HOLD:
            pszFileName = "music_hold";
            break;
        case SC_SND_MUSIC_SIGNIN:
            pszFileName = "music_signin";
            break;
        case SC_SND_NETWORK_FAULT:
            pszFileName = "network_fault";
            break;
        case SC_SND_NO_PERMISSION:
            pszFileName = "no_permission_for_service";
            break;
        case SC_SND_NO_OUT_BALANCE:
            pszFileName = "out_balance";
            break;
        case SC_SND_SET_SUCC:
            pszFileName = "set_succ";
            break;
        case SC_SND_SET_FAIL:
            pszFileName = "set_fail";
            break;
        case SC_SND_OPT_SUCC:
            pszFileName = "opt_succ";
            break;
        case SC_SND_OPT_FAIL:
            pszFileName = "opt_fail";
            break;
        case SC_SND_SYS_MAINTAIN:
            pszFileName = "sys_in_maintain";
            break;
        case SC_SND_TMP_UNAVAILABLE:
            pszFileName = "temporarily_unavailable";
            break;
        case SC_SND_USER_BUSY:
            pszFileName = "user_busy";
            break;
        case SC_SND_USER_LINE_FAULT:
            pszFileName = "user_line_fault";
            break;
        case SC_SND_USER_NOT_FOUND:
            pszFileName = "user_not_found";
            break;
        case SC_SND_CONNECTING:
            pszFileName = "connecting";
            break;
    }

    if (!pszFileName)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    dos_snprintf(szFileName, sizeof(szFileName), "%s/%s.wav", SC_PROMPT_TONE_PATH, pszFileName);

    /* Nat下开始那段媒体可能会被丢掉，线延迟一下再放音 */
    sc_ep_esl_execute("playback", "silence_stream://300", pszUUID);

    if (lTime <= 0)
    {
        sc_ep_esl_execute("endless_playback", szFileName, pszUUID);
    }
    else if (1 == lTime)
    {
        sc_ep_esl_execute("playback", szFileName, pszUUID);
    }
    else
    {
        dos_snprintf(szParams, sizeof(szParams), "+%d %s", lTime, szFileName);
        sc_ep_esl_execute("loop_playback", szParams, pszUUID);
    }

    return DOS_SUCC;
}


/**
 * 函数: U32 sc_ep_esl_execute_cmd(const S8 *pszCmd)
 * 功能: 使用pstHandle所指向的ESL句柄执行命令
 * 参数:
 *      const S8 *pszCmd:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * 注意: 当该函数在执行命令时，如果发现当前句柄已经失去连接，将会重新连接ESL服务器
 */
U32 sc_ep_esl_execute_cmd(const S8 *pszCmd)
{
    U32 ulRet;

    if (DOS_ADDR_INVALID(pszCmd))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (!g_pstHandle->stSendHandle.connected)
    {
        ulRet = esl_connect(&g_pstHandle->stSendHandle, "127.0.0.1", 8021, NULL, "ClueCon");
        if (ESL_SUCCESS != ulRet)
        {
            esl_disconnect(&g_pstHandle->stSendHandle);
            sc_logr_notice(NULL, SC_ESL, "ELS for send event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_pstHandle->stSendHandle.err);

            DOS_ASSERT(0);

            return DOS_FAIL;
        }

        g_pstHandle->stSendHandle.event_lock = 1;
    }

    if (ESL_SUCCESS != esl_send(&g_pstHandle->stSendHandle, pszCmd))
    {
        DOS_ASSERT(0);
        sc_logr_notice(NULL, SC_ESL, "ESL execute command fail. Result:%d, CMD: %s"
                        , ulRet
                        , pszCmd);

        return DOS_FAIL;
    }

    sc_logr_notice(NULL, SC_ESL, "ESL execute command SUCC. CMD: %s", pszCmd);

    return DOS_SUCC;
}

U32 sc_ep_per_hangup_call(SC_SCB_ST * pstSCB)
{
    S8 szParamString[128] = {0};

    if (DOS_ADDR_INVALID(pstSCB) || '\0' == pstSCB->szUUID)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    dos_snprintf(szParamString, sizeof(szParamString), "bgapi uuid_park %s \r\n", pstSCB->szUUID);
    sc_ep_esl_execute_cmd(szParamString);

    return DOS_SUCC;
}

U32 sc_ep_hangup_call_with_snd(SC_SCB_ST * pstSCB, U32 ulTernmiteCase)
{
    U16 usSipErrCode = 0;
    S8 szParamString[128] = {0};

    if (DOS_ADDR_INVALID(pstSCB) || '\0' == pstSCB->szUUID)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    usSipErrCode = sc_ep_transform_errcode_from_sc2sip(ulTernmiteCase);

    sc_logr_debug(pstSCB, SC_ESL, "Hangup call with error code %u", ulTernmiteCase);
    sc_log_digest_print("Hangup call with error code %u. customer : %u", ulTernmiteCase, pstSCB->ulCustomID);

    switch (ulTernmiteCase)
    {
        case CC_ERR_NO_REASON:
            sc_ep_play_sound(SC_SND_USER_BUSY, pstSCB->szUUID, 3);
            break;
        case CC_ERR_SC_IN_BLACKLIST:
        case CC_ERR_SC_CALLER_NUMBER_ILLEGAL:
        case CC_ERR_SC_CALLEE_NUMBER_ILLEGAL:
            sc_ep_play_sound(SC_SND_USER_NOT_FOUND, pstSCB->szUUID, 3);
            break;
        case CC_ERR_SC_USER_OFFLINE:
        case CC_ERR_SC_USER_HAS_BEEN_LEFT:
        case CC_ERR_SC_PERIOD_EXCEED:
        case CC_ERR_SC_RESOURCE_EXCEED:
            sc_ep_play_sound(SC_SND_TMP_UNAVAILABLE, pstSCB->szUUID, 3);
            break;
        case CC_ERR_SC_USER_BUSY:
            sc_ep_play_sound(SC_SND_USER_BUSY, pstSCB->szUUID, 3);
            break;
        case CC_ERR_SC_NO_ROUTE:
        case CC_ERR_SC_NO_TRUNK:
            sc_ep_play_sound(SC_SND_NETWORK_FAULT, pstSCB->szUUID, 1);
            break;
        case CC_ERR_BS_LACK_FEE:
            sc_ep_play_sound(SC_SND_NO_OUT_BALANCE, pstSCB->szUUID, 1);
            break;
        default:
            sc_ep_play_sound(SC_SND_TMP_UNAVAILABLE, pstSCB->szUUID, 3);
            break;
    }

    dos_snprintf(szParamString, sizeof(szParamString), "proto_specific_hangup_cause=sip:%u", usSipErrCode);
    sc_ep_esl_execute("set", szParamString, pstSCB->szUUID);

    sc_ep_esl_execute("hangup", "", pstSCB->szUUID);

    return DOS_SUCC;
}

U32 sc_ep_hangup_call(SC_SCB_ST *pstSCB, U32 ulTernmiteCase)
{
    U16 usSipErrCode = 0;
    S8 szParamString[128] = {0};

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (ulTernmiteCase >= CC_ERR_BUTT)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if ('\0' == pstSCB->szUUID[0])
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    sc_logr_debug(pstSCB, SC_ESL, "Hangup call with error code %d, pstscb : %d, other : %d", ulTernmiteCase, pstSCB->usSCBNo, pstSCB->usOtherSCBNo);
    sc_log_digest_print("Hangup call with error code %d, pstscb : %d, other : %d", ulTernmiteCase, pstSCB->usSCBNo, pstSCB->usOtherSCBNo);
    /* 将 SC 的错误码转换为 SIP 的 */
    usSipErrCode = sc_ep_transform_errcode_from_sc2sip(ulTernmiteCase);
    /* 设置错误码 */
    dos_snprintf(szParamString, sizeof(szParamString), "proto_specific_hangup_cause=sip:%u", usSipErrCode);
    sc_ep_esl_execute("set", szParamString, pstSCB->szUUID);
    /* 保证一定可以挂断 */
    dos_snprintf(szParamString, sizeof(szParamString), "bgapi uuid_setvar %s will_hangup true \r\n", pstSCB->szUUID);
    sc_ep_esl_execute_cmd(szParamString);
    dos_snprintf(szParamString, sizeof(szParamString), "bgapi uuid_park %s \r\n", pstSCB->szUUID);
    sc_ep_esl_execute_cmd(szParamString);
    /* 挂断 */
    sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
    pstSCB->usTerminationCause = ulTernmiteCase;
    pstSCB->bTerminationFlag = DOS_TRUE;

    return DOS_SUCC;
}



/**
 * 函数: U32 sc_ep_parse_event(esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 从ESL事件pstEvent中获取参数，并存储在pstSCB
 * 参数:
 *          esl_event_t *pstEvent : 数据源 ESL事件
 *          SC_SCB_ST *pstSCB     : SCB，存储数据
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_parse_event(esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8         *pszCaller    = NULL;
    S8         *pszCallee    = NULL;
    S8         *pszANI       = NULL;
    S8         *pszCallSrc   = NULL;
    S8         *pszTrunkIP   = NULL;
    S8         *pszGwName    = NULL;
    S8         *pszCallDirection = NULL;
    S8         *pszOtherLegUUID  = NULL;
    SC_SCB_ST  *pstSCB2 = NULL;

    SC_TRACE_IN(pstEvent, pstSCB, 0, 0);

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
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

    pszCallSrc = esl_event_get_header(pstEvent, "variable_sofia_profile_name");
    if (DOS_ADDR_INVALID(pszCallSrc))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pszCallDirection = esl_event_get_header(pstEvent, "Call-Direction");
    if (DOS_ADDR_INVALID(pszCallDirection))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pszGwName = esl_event_get_header(pstEvent, "variable_sip_gateway_name");
    pszTrunkIP = esl_event_get_header(pstEvent, "Caller-Network-Addr");
    pszCaller = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    pszCallee = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    pszANI    = esl_event_get_header(pstEvent, "Caller-ANI");
    pszOtherLegUUID = esl_event_get_header(pstEvent, "Other-Leg-Unique-ID");
    if (DOS_ADDR_INVALID(pszCaller) || DOS_ADDR_INVALID(pszCallee) || DOS_ADDR_INVALID(pszANI))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (DOS_ADDR_VALID(pszOtherLegUUID))
    {
        pstSCB2 = sc_scb_hash_tables_find(pszOtherLegUUID);
    }

    /* 将相关数据写入SCB中 */
    pthread_mutex_lock(&pstSCB->mutexSCBLock);
    if (DOS_ADDR_VALID(pstSCB2))
    {
        pstSCB->usOtherSCBNo = pstSCB2->usSCBNo;
        pstSCB2->usOtherSCBNo = pstSCB->usSCBNo;
    }
    dos_strncpy(pstSCB->szCalleeNum, pszCallee, sizeof(pstSCB->szCalleeNum));
    pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) -1] = '\0';
    dos_strncpy(pstSCB->szCallerNum, pszCaller, sizeof(pstSCB->szCallerNum));
    pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) -1] = '\0';
    dos_strncpy(pstSCB->szANINum, pszANI, sizeof(pstSCB->szANINum));
    pstSCB->szANINum[sizeof(pstSCB->szANINum) -1] = '\0';
    pthread_mutex_unlock(&pstSCB->mutexSCBLock);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_ep_parse_extra_data(esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8 *pszTmp = NULL;
    U64 uLTmp  = 0;

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstSCB->pstExtraData))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstSCB->ulMarkStartTimeStamp != 0)
    {
        /* 客户标记，ulStartTimeStamp和ulAnswerTimeStamp、 ulRingTimeStamp 就不用获取了，要修改成成这个时间 */
        pstSCB->pstExtraData->ulStartTimeStamp = pstSCB->ulMarkStartTimeStamp;
        sc_logr_debug(pstSCB, SC_ESL, "Get extra data: Caller-Channel-Created-Time=%u(%u)", pstSCB->ulMarkStartTimeStamp, pstSCB->pstExtraData->ulStartTimeStamp);
        pstSCB->pstExtraData->ulAnswerTimeStamp = pstSCB->ulMarkStartTimeStamp;
        sc_logr_debug(pstSCB, SC_ESL, "Get extra data: Caller-Channel-Answered-Time=%u(%u)", pstSCB->ulMarkStartTimeStamp, pstSCB->pstExtraData->ulAnswerTimeStamp);
        pstSCB->pstExtraData->ulRingTimeStamp = pstSCB->ulMarkStartTimeStamp;
        sc_logr_debug(pstSCB, SC_ESL, "Get extra data: Caller-Channel-Progress-Time=%u(%u)", pstSCB->ulMarkStartTimeStamp, pstSCB->pstExtraData->ulRingTimeStamp);
    }
    else
    {
        pszTmp = esl_event_get_header(pstEvent, "Caller-Channel-Created-Time");
        if (DOS_ADDR_VALID(pszTmp)
            && '\0' != pszTmp[0]
            && dos_atoull(pszTmp, &uLTmp) == 0)
        {
            pstSCB->pstExtraData->ulStartTimeStamp = uLTmp / 1000000;
            sc_logr_debug(pstSCB, SC_ESL, "Get extra data: Caller-Channel-Created-Time=%s(%u)", pszTmp, pstSCB->pstExtraData->ulStartTimeStamp);
        }

        pszTmp = esl_event_get_header(pstEvent, "Caller-Channel-Answered-Time");
        if (DOS_ADDR_VALID(pszTmp)
            && '\0' != pszTmp[0]
            && dos_atoull(pszTmp, &uLTmp) == 0)
        {
            pstSCB->pstExtraData->ulAnswerTimeStamp = uLTmp / 1000000;
            sc_logr_debug(pstSCB, SC_ESL, "Get extra data: Caller-Channel-Answered-Time=%s(%u)", pszTmp, pstSCB->pstExtraData->ulAnswerTimeStamp);
        }

        pszTmp = esl_event_get_header(pstEvent, "Caller-Channel-Progress-Time");
        if (DOS_ADDR_VALID(pszTmp)
            && '\0' != pszTmp[0]
            && dos_atoull(pszTmp, &uLTmp) == 0)
        {
            pstSCB->pstExtraData->ulRingTimeStamp = uLTmp / 1000000;
            sc_logr_debug(pstSCB, SC_ESL, "Get extra data: Caller-Channel-Progress-Time=%s(%u)", pszTmp, pstSCB->pstExtraData->ulRingTimeStamp);
        }
    }

    pszTmp = esl_event_get_header(pstEvent, "Caller-Channel-Bridged-Time");
    if (DOS_ADDR_VALID(pszTmp)
        && '\0' != pszTmp[0]
        && dos_atoull(pszTmp, &uLTmp) == 0)
    {
        pstSCB->pstExtraData->ulBridgeTimeStamp = uLTmp / 1000000;
        sc_logr_debug(pstSCB, SC_ESL, "Get extra data: Caller-Channel-Bridged-Time=%s(%u)", pszTmp, pstSCB->pstExtraData->ulBridgeTimeStamp);
    }

    pszTmp = esl_event_get_header(pstEvent, "Caller-Channel-Hangup-Time");
    if (DOS_ADDR_VALID(pszTmp)
        && '\0' != pszTmp[0]
        && dos_atoull(pszTmp, &uLTmp) == 0)
    {
        pstSCB->pstExtraData->ulByeTimeStamp = uLTmp / 1000000;
        sc_logr_debug(pstSCB, SC_ESL, "Get extra data: Caller-Channel-Hangup-Time=%s(%u)", pszTmp, pstSCB->pstExtraData->ulByeTimeStamp);
    }

    if (pstSCB->ucTranforRole == SC_TRANS_ROLE_NOTIFY)
    {
        /* 转接时，B挂断时 */
        pszTmp = esl_event_get_header(pstEvent, "variable_bridge_epoch");
        if (DOS_ADDR_VALID(pszTmp)
            && '\0' != pszTmp[0]
            && dos_atoull(pszTmp, &uLTmp) == 0)
        {
            pstSCB->pstExtraData->ulTransferTimeStamp = uLTmp;
            sc_logr_debug(pstSCB, SC_ESL, "Get extra data: variable_bridge_epoch=%s(%u)", pszTmp, pstSCB->pstExtraData->ulTransferTimeStamp);
        }
    }

    return DOS_SUCC;
}

U32 sc_ep_terminate_call(SC_SCB_ST *pstSCB)
{
    SC_SCB_ST *pstSCBOther = NULL;
    BOOL    bIsOtherHangup = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
    if (DOS_ADDR_VALID(pstSCBOther))
    {
        if ('\0' != pstSCBOther->szUUID[0])
        {
            if (sc_ep_hangup_call_with_snd(pstSCBOther, pstSCBOther->usTerminationCause) == DOS_SUCC)
            {
                bIsOtherHangup = DOS_TRUE;
            }

            sc_logr_notice(pstSCB, SC_ESL, "Hangup Call for Auth FAIL. SCB No : %d, UUID: %s.", pstSCBOther->usSCBNo, pstSCBOther->szUUID);
        }
        else
        {
            SC_SCB_SET_STATUS(pstSCBOther, SC_SCB_RELEASE);
            sc_call_trace(pstSCBOther, "Terminate call.");
            sc_logr_notice(pstSCB, SC_ESL, "Call terminate call. SCB No : %d.", pstSCBOther->usSCBNo);
            sc_scb_free(pstSCBOther);
        }
    }

    if ('\0' != pstSCB->szUUID[0])
    {
        /* 有FS通讯的话，就直接挂断呼叫就好 */
        sc_ep_hangup_call_with_snd(pstSCB, pstSCB->usTerminationCause);
        sc_logr_notice(pstSCB, SC_ESL, "Hangup Call for Auth FAIL. SCB No : %d, UUID: %d. *", pstSCB->usSCBNo, pstSCB->szUUID);
    }
    else
    {
        /* 如果另一个leg成功调用了hangup，让一条leg释放这条leg就可以了，这里就不用释放了 */
        if (!bIsOtherHangup)
        {
            SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);
            sc_call_trace(pstSCB, "Terminate call.");
            sc_logr_notice(pstSCB, SC_ESL, "Call terminate call. SCB No : %d. *", pstSCB->usSCBNo);
            sc_scb_free(pstSCB);
        }
    }

    return DOS_SUCC;
}


/**
 * 函数: U32 sc_ep_internal_service_check(esl_event_t *pstEvent)
 * 功能: 检查当前事件是否是在执行内部业务
 * 参数:
 *          esl_event_t *pstEvent : 数据源 ESL事件
 * 返回值: 成功返回内部业务枚举值，否则返回无效业务枚举
 */
U32 sc_ep_internal_service_check(esl_event_t *pstEvent)
{
    return SC_INTER_SRV_BUTT;
}

/**
 * 函数: BOOL sc_ep_check_extension(S8 *pszNum, U32 ulCustomerID)
 * 功能: 检查pszNum所执行的分机号，是否输入编号为ulCustomerID的客户
 * 参数:
 *      S8 *pszNum: 分机号
 *      U32 ulCustomerID: 客户ID
 * 返回值: 成功返回DOS_TRUE，否则返回DOS_FALSE
 */
BOOL sc_ep_check_extension(S8 *pszNum, U32 ulCustomerID)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);

        return DOS_FALSE;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    HASH_Scan_Table(g_pstHashSIPUserID, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashSIPUserID, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstUserIDNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstUserIDNode))
            {
                continue;
            }

            if (ulCustomerID == pstUserIDNode->ulCustomID
                && 0 == dos_strnicmp(pstUserIDNode->szExtension, pszNum, sizeof(pstUserIDNode->szExtension)))
            {
                pthread_mutex_unlock(&g_mutexHashSIPUserID);
                return DOS_TRUE;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return DOS_FALSE;
}

/**
 * 函数: U32 sc_ep_get_extension_by_userid(S8 *pszUserID, S8 *pszExtension, U32 ulLength)
 * 功能: 获取UserID pszUserID对应的分机号，并copy到缓存pszExtension中，使用ulLength指定缓存的长度
 * 参数:
 *      S8 *pszUserID    : User ID
 *      S8 *pszExtension : 存储分机号的缓存
 *      U32 ulLength     : 缓存长度
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_get_extension_by_userid(S8 *pszUserID, S8 *pszExtension, U32 ulLength)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszUserID)
        || DOS_ADDR_INVALID(pszExtension)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    ulHashIndex = sc_sip_userid_hash_func(pszUserID);
    pstHashNode = hash_find_node(g_pstHashSIPUserID, ulHashIndex, (VOID *)pszUserID, sc_ep_sip_userid_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&g_mutexHashSIPUserID);
        return DOS_FAIL;
    }

    pstUserIDNode = pstHashNode->pHandle;

    dos_strncpy(pszExtension, pstUserIDNode->szExtension, ulLength);
    pszExtension[ulLength - 1] = '\0';

    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_ep_get_custom_by_sip_userid(S8 *pszNum)
 * 功能: 获取pszNum所指定UserID所属的客户的ID
 * 参数:
 *      S8 *pszNum    : User ID
 * 返回值: 成功返回客户ID值，否则返回U32_BUTT
 */
U32 sc_ep_get_custom_by_sip_userid(S8 *pszNum)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;
    U32                ulCustomerID   = 0;

    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    ulHashIndex = sc_sip_userid_hash_func(pszNum);
    pstHashNode = hash_find_node(g_pstHashSIPUserID, ulHashIndex, (VOID *)pszNum, sc_ep_sip_userid_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        sc_logr_info(NULL, SC_FUNC, "Get customer FAIL by sip(%s)", pszNum);
        pthread_mutex_unlock(&g_mutexHashSIPUserID);
        return U32_BUTT;
    }

    if (DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_info(NULL, SC_FUNC, "Get customer FAIL by sip(%s), data error", pszNum);
        pthread_mutex_unlock(&g_mutexHashSIPUserID);
        return U32_BUTT;
    }

    pstUserIDNode = pstHashNode->pHandle;

    ulCustomerID = pstUserIDNode->ulCustomID;
    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return ulCustomerID;
}

/**
 * 函数: U32 sc_ep_get_custom_by_did(S8 *pszNum)
 * 功能: 通过pszNum所指定的DID号码，找到当前DID号码输入那个客户
 * 参数:
 *      S8 *pszNum : DID号码
 * 返回值: 成功返回客户ID，否则返回U32_BUTT
 */
U32 sc_ep_get_custom_by_did(S8 *pszNum)
{
    SC_DID_NODE_ST     *pstDIDNumNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;
    U32                ulCustomerID   = 0;

    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    ulHashIndex = sc_sip_did_hash_func(pszNum);
    pthread_mutex_lock(&g_mutexHashDIDNum);
    pstHashNode = hash_find_node(g_pstHashDIDNum, ulHashIndex, (VOID *)pszNum, sc_ep_did_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&g_mutexHashDIDNum);
        return U32_BUTT;
    }

    pstDIDNumNode = pstHashNode->pHandle;
    if (DOS_FALSE == pstDIDNumNode->bValid)
    {
        pthread_mutex_unlock(&g_mutexHashDIDNum);

        return U32_BUTT;
    }
    ulCustomerID = pstDIDNumNode->ulCustomID;

    pthread_mutex_unlock(&g_mutexHashDIDNum);

    return ulCustomerID;
}


/**
 * 函数: U32 sc_ep_get_bind_info4did(S8 *pszDidNum, U32 *pulBindType, U32 *pulBindID)
 * 功能: 获取pszDidNum所执行的DID号码的绑定信息
 * 参数:
 *      S8 *pszDidNum    : DID号码
 *      U32 *pulBindType : 当前DID号码绑定的类型
 *      U32 *pulBindID   : 当前DID号码绑定的ID
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_get_bind_info4did(S8 *pszDidNum, U32 *pulBindType, U32 *pulBindID)
{
    SC_DID_NODE_ST     *pstDIDNumNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszDidNum)
        || DOS_ADDR_INVALID(pulBindType)
        || DOS_ADDR_INVALID(pulBindID))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulHashIndex = sc_sip_did_hash_func(pszDidNum);
    pthread_mutex_lock(&g_mutexHashDIDNum);
    pstHashNode = hash_find_node(g_pstHashDIDNum, ulHashIndex, (VOID *)pszDidNum, sc_ep_did_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&g_mutexHashDIDNum);
        return DOS_FAIL;
    }

    pstDIDNumNode = pstHashNode->pHandle;

    *pulBindType = pstDIDNumNode->ulBindType;
    *pulBindID = pstDIDNumNode->ulBindID;

    pthread_mutex_unlock(&g_mutexHashDIDNum);

    return DOS_SUCC;
}


/**
 * 函数: U32 sc_ep_get_userid_by_id(U32 ulSIPUserID, S8 *pszUserID, U32 ulLength)
 * 功能: 获取ID为ulSIPUserID SIP User ID，并将SIP USER ID Copy到缓存pszUserID中
 * 参数:
 *      U32 ulSIPUserID : SIP User ID的所以
 *      S8 *pszUserID   : 账户ID缓存
 *      U32 ulLength    : 缓存长度
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_get_userid_by_id(U32 ulSipID, S8 *pszUserID, U32 ulLength)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszUserID)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    HASH_Scan_Table(g_pstHashSIPUserID, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashSIPUserID, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstUserIDNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstUserIDNode))
            {
                continue;
            }

            if (ulSipID == pstUserIDNode->ulSIPID)
            {
                dos_strncpy(pszUserID, pstUserIDNode->szUserID, ulLength);
                pszUserID[ulLength - 1] = '\0';
                pthread_mutex_unlock(&g_mutexHashSIPUserID);
                return DOS_SUCC;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashSIPUserID);
    return DOS_FAIL;

}

/**
 * 函数: U32 sc_ep_get_userid_by_extension(U32 ulCustomID, S8 *pszExtension, S8 *pszUserID, U32 ulLength)
 * 功能: 获取客户ID为ulCustomID，分机号为pszExtension的User ID，并将User ID Copy到缓存pszUserID中
 * 参数:
 *      U32 ulCustomID  : 客户ID
 *      S8 *pszExtension: 分机号
 *      S8 *pszUserID   : 账户ID缓存
 *      U32 ulLength    : 缓存长度
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_get_userid_by_extension(U32 ulCustomID, S8 *pszExtension, S8 *pszUserID, U32 ulLength)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszExtension)
        || DOS_ADDR_INVALID(pszUserID)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    HASH_Scan_Table(g_pstHashSIPUserID, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashSIPUserID, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstUserIDNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstUserIDNode))
            {
                continue;
            }

            if (ulCustomID != pstUserIDNode->ulCustomID)
            {
                continue;
            }

            if (0 != dos_strnicmp(pstUserIDNode->szExtension, pszExtension, sizeof(pstUserIDNode->szExtension)))
            {
                continue;
            }

            dos_strncpy(pszUserID, pstUserIDNode->szUserID, ulLength);
            pszUserID[ulLength - 1] = '\0';
            pthread_mutex_unlock(&g_mutexHashSIPUserID);
            return DOS_SUCC;
        }
    }
    pthread_mutex_unlock(&g_mutexHashSIPUserID);
    return DOS_FAIL;
}

/**
 * 函数: U32 sc_ep_search_route(SC_SCB_ST *pstSCB)
 * 功能: 获取路由组
 * 参数:
 *      SC_SCB_ST *pstSCB : 呼叫控制块，使用主被叫号码
 * 返回值: 成功返回路由组ID，否则返回U32_BUTT
 */
U32 sc_ep_search_route(SC_SCB_ST *pstSCB)
{
    SC_ROUTE_NODE_ST     *pstRouetEntry = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    struct tm            *pstTime;
    time_t               timep;
    U32                  ulRouteGrpID;
    U32                  ulStartTime, ulEndTime, ulCurrentTime;
    U16                  usCallOutGroup;

    timep = time(NULL);
    pstTime = localtime(&timep);
    if (DOS_ADDR_INVALID(pstTime))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    ulRouteGrpID = U32_BUTT;

    /* 根据 ulCustomID 查到到 呼出组, 如果查找失败，则将呼出组置为0 */
    if (sc_ep_get_callout_group_by_customerID(pstSCB->ulCustomID, &usCallOutGroup) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        usCallOutGroup = 0;
    }

    sc_logr_debug(pstSCB, SC_DIALER, "usCallOutGroup : %u, custom : %u", usCallOutGroup, pstSCB->ulCustomID);
    pthread_mutex_lock(&g_mutexRouteList);

loop_search:
    DLL_Scan(&g_stRouteList, pstListNode, DLL_NODE_S *)
    {
        pstRouetEntry = (SC_ROUTE_NODE_ST *)pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstRouetEntry))
        {
            continue;
        }

        if (usCallOutGroup != pstRouetEntry->usCallOutGroup)
        {
            continue;
        }

        sc_logr_info(pstSCB, SC_ESL, "Search Route: %d:%d, %d:%d, CallerPrefix : %s, CalleePrefix : %s. Caller:%s, Callee:%s"
            ", customer group : %u, route group : %u"
                , pstRouetEntry->ucHourBegin, pstRouetEntry->ucMinuteBegin
                , pstRouetEntry->ucHourEnd, pstRouetEntry->ucMinuteEnd
                , pstRouetEntry->szCallerPrefix
                , pstRouetEntry->szCalleePrefix
                , pstSCB->szCallerNum
                , pstSCB->szCalleeNum
                , usCallOutGroup
                , pstRouetEntry->usCallOutGroup);

        /* 如果开始和结束时间都为 00:00, 则表示全天有效，不用判断时间了 */
        if (pstRouetEntry->ucHourBegin
            || pstRouetEntry->ucMinuteBegin
            || pstRouetEntry->ucHourEnd
            || pstRouetEntry->ucMinuteEnd)
        {
            ulStartTime = pstRouetEntry->ucHourBegin * 60 + pstRouetEntry->ucMinuteBegin;
            ulEndTime = pstRouetEntry->ucHourEnd* 60 + pstRouetEntry->ucMinuteEnd;
            ulCurrentTime = pstTime->tm_hour *60 + pstTime->tm_min;

            if (ulCurrentTime < ulStartTime || ulCurrentTime > ulEndTime)
            {
                sc_logr_info(pstSCB, SC_ESL, "Search Route(FAIL): Time not match: Peroid:%u-:%u, Current:%u"
                        , ulStartTime, ulEndTime, ulCurrentTime);

                continue;
            }
        }

        if ('\0' == pstRouetEntry->szCalleePrefix[0])
        {
            if ('\0' == pstRouetEntry->szCallerPrefix[0])
            {
                ulRouteGrpID = pstRouetEntry->ulID;
                break;
            }
            else
            {
                if (0 == dos_strnicmp(pstRouetEntry->szCallerPrefix, pstSCB->szCallerNum, dos_strlen(pstRouetEntry->szCallerPrefix)))
                {
                    ulRouteGrpID = pstRouetEntry->ulID;
                    break;
                }
            }
        }
        else
        {
            if ('\0' == pstRouetEntry->szCallerPrefix[0])
            {
                if (0 == dos_strnicmp(pstRouetEntry->szCalleePrefix, pstSCB->szCalleeNum, dos_strlen(pstRouetEntry->szCalleePrefix)))
                {
                    ulRouteGrpID = pstRouetEntry->ulID;
                    break;
                }
            }
            else
            {
                if (0 == dos_strnicmp(pstRouetEntry->szCalleePrefix, pstSCB->szCalleeNum, dos_strlen(pstRouetEntry->szCalleePrefix))
                    && 0 == dos_strnicmp(pstRouetEntry->szCallerPrefix, pstSCB->szCallerNum, dos_strlen(pstRouetEntry->szCallerPrefix)))
                {
                    ulRouteGrpID = pstRouetEntry->ulID;
                    break;
                }
            }
        }
    }

    if (U32_BUTT == ulRouteGrpID && usCallOutGroup != 0)
    {
        /* 没有查找到 呼出组一样的 路由， 再循环一遍，查找通配的路由 */
        usCallOutGroup = 0;
        goto loop_search;
    }

    if (DOS_ADDR_VALID(pstRouetEntry))
    {
        sc_logr_debug(pstSCB, SC_ESL, "Search Route Finished. Result: %s, Route ID: %d, Dest Type:%u, Dest ID: %u"
                , U32_BUTT == ulRouteGrpID ? "FAIL" : "SUCC"
                , ulRouteGrpID
                , pstRouetEntry->ulDestType
                , pstRouetEntry->aulDestID[0]);
    }
    else
    {
        sc_logr_debug(pstSCB, SC_ESL, "Search Route Finished. Result: %s, Route ID: %d"
                , U32_BUTT == ulRouteGrpID ? "FAIL" : "SUCC"
                , ulRouteGrpID);
    }

    pthread_mutex_unlock(&g_mutexRouteList);

    return ulRouteGrpID;
}

/**
 * 函数: U32 sc_ep_get_callee_string(U32 ulRouteGroupID, S8 *pszNum, S8 *szCalleeString, U32 ulLength)
 * 功能: 通过路由组ID，和被叫号码获取出局呼叫的呼叫字符串，并将结果存储在szCalleeString中
 * 参数:
 *      U32 ulRouteGroupID : 路由组ID
 *      S8 *pszNum         : 被叫号码
 *      S8 *szCalleeString : 呼叫字符串缓存
 *      U32 ulLength       : 缓存长度
 * 返回值: 返回 中继的个数，如果为0，则是失败
 */
U32 sc_ep_get_callee_string(U32 ulRouteID, SC_SCB_ST *pstSCB, S8 *szCalleeString, U32 ulLength)
{
    SC_ROUTE_NODE_ST     *pstRouetEntry = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    DLL_NODE_S           *pstListNode1  = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    SC_GW_GRP_NODE_ST    *pstGWGrp      = NULL;
    SC_GW_NODE_ST        *pstGW         = NULL;
    U32                  ulCurrentLen   = 0;
    U32                  ulGWCount      = 0;
    U32                  ulHashIndex;
    S32                  lIndex;
    BOOL                 blIsFound = DOS_FALSE;
    S8                  *pszNum         = NULL;

    if (DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstSCB->szCalleeNum)
        || DOS_ADDR_INVALID(szCalleeString)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);
        return 0;
    }

    pszNum = pstSCB->szCalleeNum;

    ulCurrentLen = 0;
    pthread_mutex_lock(&g_mutexRouteList);
    DLL_Scan(&g_stRouteList, pstListNode, DLL_NODE_S *)
    {
        pstRouetEntry = (SC_ROUTE_NODE_ST *)pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstRouetEntry))
        {
            continue;
        }

        if (!pstRouetEntry->bStatus)
        {
            continue;
        }

        if (pstRouetEntry->ulID == ulRouteID)
        {
            switch (pstRouetEntry->ulDestType)
            {
                case SC_DEST_TYPE_GATEWAY:
                    /* TODO 路由后号码变换。现在只支持中继的，不支持中继组 */
                    /* 获得中继，判断中继是否可用 */
                    ulHashIndex = sc_ep_gw_hash_func(pstRouetEntry->aulDestID[0]);
                    pthread_mutex_lock(&g_mutexHashGW);
                    pstHashNode = hash_find_node(g_pstHashGW, ulHashIndex, (VOID *)&pstRouetEntry->aulDestID[0], sc_ep_gw_hash_find);
                    if (DOS_ADDR_INVALID(pstHashNode)
                        || DOS_ADDR_INVALID(pstHashNode->pHandle))
                    {
                        pthread_mutex_unlock(&g_mutexHashGW);
                        break;
                    }

                    pstGW = pstHashNode->pHandle;
                    if (DOS_FALSE == pstGW->bStatus
                        || (pstGW->bRegister && pstGW->ulRegisterStatus != SC_TRUNK_STATE_TYPE_NOREG))
                    {
                        pthread_mutex_unlock(&g_mutexHashGW);
                        break;
                    }
                    pthread_mutex_unlock(&g_mutexHashGW);

                    if (sc_ep_num_transform(pstSCB, pstRouetEntry->aulDestID[0], SC_NUM_TRANSFORM_TIMING_AFTER, SC_NUM_TRANSFORM_SELECT_CALLER) != DOS_SUCC)
                    {
                        DOS_ASSERT(0);
                        blIsFound = DOS_FALSE;

                        break;
                    }

                    if (sc_ep_num_transform(pstSCB, pstRouetEntry->aulDestID[0], SC_NUM_TRANSFORM_TIMING_AFTER, SC_NUM_TRANSFORM_SELECT_CALLEE) != DOS_SUCC)
                    {
                        DOS_ASSERT(0);
                        blIsFound = DOS_FALSE;

                        break;
                    }

                    pszNum = pstSCB->szCalleeNum;

                    ulCurrentLen = dos_snprintf(szCalleeString + ulCurrentLen
                                    , ulLength - ulCurrentLen
                                    , "sofia/gateway/%d/%s|"
                                    , pstRouetEntry->aulDestID[0]
                                    , pszNum);

                    blIsFound = DOS_TRUE;
                    ulGWCount = 1;

                    break;
                case SC_DEST_TYPE_GW_GRP:
                    /* 查找网关组 */
                    for (lIndex=0; lIndex<SC_ROUT_GW_GRP_MAX_SIZE; lIndex++)
                    {
                        if (U32_BUTT == pstRouetEntry->aulDestID[lIndex])
                        {
                            break;
                        }

                        sc_logr_debug(pstSCB, SC_ESL, "Search gateway froup, ID is %d", pstRouetEntry->aulDestID[lIndex]);
                        ulHashIndex = sc_ep_gw_grp_hash_func(pstRouetEntry->aulDestID[lIndex]);
                        pstHashNode = hash_find_node(g_pstHashGWGrp, ulHashIndex, (VOID *)&pstRouetEntry->aulDestID[lIndex], sc_ep_gw_grp_hash_find);
                        if (DOS_ADDR_INVALID(pstHashNode)
                            || DOS_ADDR_INVALID(pstHashNode->pHandle))
                        {
                            /* 没有找到对应的中继组，继续查找下一个，这种情况，理论上是不应该出现的 */
                            sc_logr_info(pstSCB, SC_ESL, "Not found gateway froup %d", pstRouetEntry->aulDestID[lIndex]);

                            continue;
                        }

                        /* 查找网关 */
                        /* 生成呼叫字符串 */
                        pstGWGrp= pstHashNode->pHandle;
                        ulGWCount = 0;
                        pthread_mutex_lock(&pstGWGrp->mutexGWList);
                        DLL_Scan(&pstGWGrp->stGWList, pstListNode1, DLL_NODE_S *)
                        {
                            if (DOS_ADDR_VALID(pstListNode1)
                                && DOS_ADDR_VALID(pstListNode1->pHandle))
                            {
                                pstGW = pstListNode1->pHandle;
                                if (DOS_FALSE == pstGW->bStatus
                                    || (pstGW->bRegister && pstGW->ulRegisterStatus != SC_TRUNK_STATE_TYPE_NOREG))
                                {
                                    continue;
                                }
                                ulCurrentLen += dos_snprintf(szCalleeString + ulCurrentLen
                                                , ulLength - ulCurrentLen
                                                , "sofia/gateway/%d/%s|"
                                                , pstGW->ulGWID
                                                , pszNum);

                                ulGWCount++;
                            }
                        }
                        pthread_mutex_unlock(&pstGWGrp->mutexGWList);
                    }

                    if (ulGWCount > 0)
                    {
                        blIsFound = DOS_TRUE;
                    }
                    else
                    {
                        DOS_ASSERT(0);
                        blIsFound = DOS_FALSE;
                    }
                    break;
                default:
                    DOS_ASSERT(0);
                    blIsFound = DOS_FALSE;
                    break;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexRouteList);

    if (blIsFound)
    {
        /* 最后多了一个  | */
        szCalleeString[dos_strlen(szCalleeString) - 1] = '\0';
        sc_logr_debug(pstSCB, SC_ESL, "callee string is %s", szCalleeString);

        return ulGWCount;
    }
    else
    {
        szCalleeString[0] = '\0';
        return 0;
    }
}

/**
 * 函数: BOOL sc_ep_dst_is_black(S8 *pszNum)
 * 功能: 判断pszNum所指定的号码是否在黑名单中
 * 参数:
 *      S8 *pszNum : 需要被处理的号码
 * 返回值: 成功返DOS_TRUE，否则返回DOS_FALSE
 */
BOOL sc_ep_dst_is_black(S8 *pszNum)
{
    return DOS_FALSE;
}

/**
 * 函数: U32 sc_ep_get_source(esl_event_t *pstEvent)
 * 功能: 通过esl事件pstEvent判断当前呼叫的来源
 * 参数:
 *      esl_event_t *pstEvent : 需要被处理的时间
 * 返回值: 枚举值 enum tagCallDirection
 */
U32 sc_ep_get_source(esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    const S8 *pszCallSource;
    SC_ACD_AGENT_INFO_ST stAgentInfo;

    pszCallSource = esl_event_get_header(pstEvent, "variable_sofia_profile_name");
    if (DOS_ADDR_INVALID(pszCallSource))
    {
        DOS_ASSERT(0);
        return SC_DIRECTION_INVALID;
    }

    if (0 == dos_strcmp(pszCallSource, "internal"))
    {
        return SC_DIRECTION_SIP;
    }

    /* 先判断 来源，是不是配置的中继 */
    pszCallSource = esl_event_get_header(pstEvent, "Caller-Network-Addr");
    if (DOS_ADDR_INVALID(pszCallSource))
    {
        DOS_ASSERT(0);
        return SC_DIRECTION_PSTN;
    }

    if (sc_find_gateway_by_addr(pszCallSource) != DOS_SUCC)
    {
        return SC_DIRECTION_PSTN;
    }

    /* 判断 主叫号码 是否是 TT 号，现在 TT号 只支持外呼 */
    if (sc_acd_get_agent_by_tt_num(&stAgentInfo, pstSCB->szCallerNum, pstSCB) == DOS_SUCC)
    {
        pstSCB->bIsCallerInTT = DOS_TRUE;

        return SC_DIRECTION_SIP;
    }

    return SC_DIRECTION_PSTN;
}

/**
 * 函数: U32 sc_ep_get_source(esl_event_t *pstEvent)
 * 功能: 通过esl事件pstEvent判断当前呼叫的目的地
 * 参数:
 *      esl_event_t *pstEvent : 需要被处理的时间
 * 返回值: 枚举值 enum tagCallDirection
 */
U32 sc_ep_get_destination(esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8 *pszDstNum     = NULL;
    S8 *pszSrcNum     = NULL;
    S8 *pszCallSource = NULL;
    U32 ulCustomID    = U32_BUTT;
    U32 ulCustomID1   = U32_BUTT;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);

        return SC_DIRECTION_INVALID;
    }

    pszDstNum = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    pszSrcNum = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    pszCallSource = esl_event_get_header(pstEvent, "variable_sofia_profile_name");
    if (DOS_ADDR_INVALID(pszDstNum)
        || DOS_ADDR_INVALID(pszSrcNum)
        || DOS_ADDR_INVALID(pszCallSource))
    {
        DOS_ASSERT(0);

        return SC_DIRECTION_INVALID;
    }

    if (sc_ep_dst_is_black(pszDstNum))
    {
        sc_logr_notice(pstSCB, SC_ESL, "The destination is in black list. %s", pszDstNum);

        return SC_DIRECTION_INVALID;
    }

    if (0 == dos_strcmp(pszCallSource, "internal"))
    {
        /* IP测发起的呼叫，主叫一定为某SIP账户 */
        ulCustomID = sc_ep_get_custom_by_sip_userid(pszSrcNum);
        if (U32_BUTT == ulCustomID)
        {
            DOS_ASSERT(0);

            sc_logr_info(pstSCB, SC_ESL, "Source number %s is not invalid sip user id. Reject Call", pszSrcNum);
            return SC_DIRECTION_INVALID;
        }

        /*  测试被叫是否是分机号 */
        if (sc_ep_check_extension(pszDstNum, ulCustomID))
        {
            return SC_DIRECTION_SIP;
        }

        /* 被叫号码是否是同一个客户下的SIP User ID */
        ulCustomID1 = sc_ep_get_custom_by_sip_userid(pszDstNum);
        if (ulCustomID == ulCustomID1)
        {
            return SC_DIRECTION_SIP;
        }

        return SC_DIRECTION_PSTN;
    }
    else
    {
        if (pstSCB->bIsCallerInTT)
        {
            /* TT 号 发起的呼叫, 现在只支持外呼 */
            return SC_DIRECTION_PSTN;
        }

        ulCustomID = sc_ep_get_custom_by_did(pszDstNum);
        if (U32_BUTT == ulCustomID)
        {
            /* 判断一下主叫是否 TT 号 */
            DOS_ASSERT(0);

            sc_logr_notice(pstSCB, SC_ESL, "The destination %s is not a DID number. Reject Call.", pszDstNum);
            return SC_DIRECTION_INVALID;
        }

        return SC_DIRECTION_SIP;
    }
}

/**
 * 函数: sc_ep_agent_signin
 * 功能: 坐席长签时，向坐席发起呼叫
 * 参数:
 *      SC_ACD_AGENT_INFO_ST *pstAgentInfo : 坐席信息
 * 返回值: 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_ep_agent_signin(SC_ACD_AGENT_INFO_ST *pstAgentInfo)
{
    SC_SCB_ST *pstSCB           = NULL;
    U32       ulRet;
    S8        szNumber[SC_TEL_NUMBER_LENGTH] = {0};
    S8        szCMD[200] = { 0 };


    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCB = sc_scb_get(pstAgentInfo->usSCBNo);
    if (DOS_ADDR_VALID(pstSCB) && SC_SCB_ACTIVE == pstSCB->ucStatus)
    {
        if (pstAgentInfo->bConnected && pstAgentInfo->bNeedConnected)
        {
            sc_logr_warning(pstSCB, SC_ESL, "Agent %u request signin. But it seems already signin. Exit.", pstAgentInfo->ulSiteID);
            return DOS_SUCC;
        }
        else
        {
            sc_logr_warning(pstSCB, SC_ESL, "Agent %u request signin. But it seems in a call(SCB: %u). Exit.", pstAgentInfo->ulSiteID, pstAgentInfo->usSCBNo);

            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_AGENT_SIGNIN);
            sc_ep_agent_status_notify(pstAgentInfo, ACD_MSG_SUBTYPE_SIGNIN);

            dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_setval %s exec_after_bridge_app park \r\n", pstSCB->szUUID);
            sc_ep_esl_execute_cmd(szCMD);

            pstAgentInfo->bConnected = DOS_TRUE;
            pstAgentInfo->bNeedConnected = DOS_TRUE;
            return DOS_SUCC;
        }
    }

    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        sc_logr_error(pstSCB, SC_ESL, "%s", "Allc SCB FAIL.");

        goto error;
    }

    pstAgentInfo->usSCBNo = pstSCB->usSCBNo;
    pstSCB->ulCustomID = pstAgentInfo->ulCustomerID;
    pstSCB->ulAgentID = pstAgentInfo->ulSiteID;
    pstSCB->ucLegRole = SC_CALLEE;
    pstSCB->bRecord = pstAgentInfo->bRecord;
    pstSCB->bTraceNo = pstAgentInfo->bTraceON;
    pstSCB->bIsAgentCall = DOS_TRUE;

    switch (pstAgentInfo->ucBindType)
    {
        case AGENT_BIND_SIP:
            dos_strncpy(pstSCB->szCalleeNum, pstAgentInfo->szUserID, sizeof(pstSCB->szCalleeNum));
            pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
            break;
        case AGENT_BIND_TELE:
            dos_strncpy(pstSCB->szCalleeNum, pstAgentInfo->szTelePhone, sizeof(pstSCB->szCalleeNum));
            pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
            break;
        case AGENT_BIND_MOBILE:
            dos_strncpy(pstSCB->szCalleeNum, pstAgentInfo->szMobile, sizeof(pstSCB->szCalleeNum));
            pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
            break;
        case AGENT_BIND_TT_NUMBER:
            dos_strncpy(pstSCB->szCalleeNum, pstAgentInfo->szTTNumber, sizeof(pstSCB->szCalleeNum));
            pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
            break;
        default:
            goto error;
    }

    dos_strncpy(pstSCB->szCallerNum, pstAgentInfo->szUserID, sizeof(pstSCB->szCallerNum));
    pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) - 1] = '\0';

    dos_strncpy(pstSCB->szSiteNum, pstAgentInfo->szEmpNo, sizeof(pstSCB->szSiteNum));
    pstSCB->szSiteNum[sizeof(pstSCB->szSiteNum) - 1] = '\0';

    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_AGENT_SIGNIN);
    SC_SCB_SET_STATUS(pstSCB, SC_SCB_EXEC);

    if (AGENT_BIND_SIP != pstAgentInfo->ucBindType
        && AGENT_BIND_TT_NUMBER != pstAgentInfo->ucBindType)
    {
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL);

        /* 设置主叫号码 */
        if (U32_BUTT == pstSCB->ulAgentID)
        {
            sc_logr_info(pstSCB, SC_DIALER, "Agent signin not get agent ID by scd(%u)", pstSCB->usSCBNo);

            goto go_on;
        }

        /* 查找呼叫源和号码的对应关系，如果匹配上某一呼叫源，就选择特定号码 */
        ulRet = sc_caller_setting_select_number(pstSCB->ulCustomID, pstSCB->ulAgentID, SC_SRC_CALLER_TYPE_AGENT, szNumber, SC_TEL_NUMBER_LENGTH);
        if (ulRet != DOS_SUCC)
        {
            sc_logr_info(pstSCB, SC_DIALER, "Agent signin customID(%u) get caller number FAIL by agent(%u)", pstSCB->ulCustomID, pstSCB->ulAgentID);

            goto go_on;
        }

        sc_logr_info(pstSCB, SC_DIALER, "Agent signin customID(%u) get caller number(%s) SUCC by agent(%u)", pstSCB->ulCustomID, szNumber, pstSCB->ulAgentID);
        dos_strncpy(pstSCB->szCallerNum, szNumber, SC_TEL_NUMBER_LENGTH);
        pstSCB->szCallerNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';

go_on:
        if (!sc_ep_black_list_check(pstSCB->ulCustomID, pstSCB->szCalleeNum))
        {
            DOS_ASSERT(0);
            sc_logr_info(pstSCB, SC_ESL, "Cannot make call for number %s which is in black list.", pstSCB->szCalleeNum);
            goto error;
        }

        if (sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
        {
            sc_logr_notice(pstSCB, SC_ESL, "Send auth msg FAIL. SCB No: %d", pstSCB->usSCBNo);
            goto error;
        }

        return DOS_SUCC;
    }

    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_CALL);

    if (AGENT_BIND_SIP == pstAgentInfo->ucBindType)
    {
        if (sc_dial_make_call2ip(pstSCB, SC_SERV_AGENT_SIGNIN, DOS_TRUE) != DOS_SUCC)
        {
            goto error;
        }
    }
    else if (AGENT_BIND_TT_NUMBER == pstAgentInfo->ucBindType)
    {
        if (sc_dial_make_call2eix(pstSCB, SC_SERV_AGENT_SIGNIN) != DOS_SUCC)
        {
            goto error;
        }
    }

    return DOS_SUCC;

error:

    sc_logr_notice(pstSCB, SC_ESL, "Agent signin fail agent: %u", pstAgentInfo->ulSiteID);

    if (SC_SCB_IS_VALID(pstSCB))
    {
        sc_scb_free(pstSCB);
    }

    return DOS_FAIL;
}

U32 sc_ep_agent_signout(SC_ACD_AGENT_INFO_ST *pstAgentInfo)
{
    SC_SCB_ST     *pstSCB           = NULL;

    if (DOS_ADDR_INVALID(pstAgentInfo) || pstAgentInfo->usSCBNo >= SC_MAX_SCB_NUM)
    {
        return DOS_FAIL;
    }

    pstSCB = sc_scb_get(pstAgentInfo->usSCBNo);

    /* 挂断 */
    pstAgentInfo->usSCBNo = U16_BUTT;
    sc_ep_per_hangup_call(pstSCB);
    sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_NORMAL_CLEAR);
    sc_scb_free(pstSCB);

    return DOS_SUCC;
}

U32 sc_ep_transfer_notify_release(SC_SCB_ST * pstSCBNotify)
{
    SC_SCB_ST* pstSCBSubscription = NULL;
    SC_SCB_ST* pstSCBPublish = NULL;
    S8         szBuffCMD[512] = { 0 };
#if 0
    U8         ucSubscriptionSrv    = U8_BUTT;
    U8         ucPublishSrv         = U8_BUTT;
    S32        i;
#endif

    if (!SC_SCB_IS_VALID(pstSCBNotify))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_call_trace(pstSCBNotify, "notify has hangup");

    pstSCBPublish = sc_scb_get(pstSCBNotify->usPublishSCB);
    if (!SC_SCB_IS_VALID(pstSCBPublish))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCBSubscription = sc_scb_get(pstSCBNotify->usOtherSCBNo);
    if (!SC_SCB_IS_VALID(pstSCBSubscription))
    {
        sc_logr_info(pstSCBNotify, SC_ESL, "%s", "notify hangup with the subscript already hangup. hangup call.");

        sc_ep_hangup_call(pstSCBPublish, CC_ERR_NORMAL_CLEAR);

        return DOS_SUCC;
    }

    if (pstSCBPublish->ucStatus < SC_SCB_ACTIVE)
    {
        if (SC_SCB_ACTIVE == pstSCBSubscription->ucStatus)
        {
            if (sc_call_check_service(pstSCBPublish, SC_SERV_ATTEND_TRANSFER))
            {
                /* 直接挂断 */
                sc_ep_hangup_call(pstSCBSubscription, CC_ERR_NORMAL_CLEAR);
                sc_ep_hangup_call(pstSCBPublish, CC_ERR_NORMAL_CLEAR);
            }
            else if (sc_call_check_service(pstSCBPublish, SC_SERV_ATTEND_TRANSFER))
            {
#if 0
                /* bridge 其它两条leg */
                dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "hangup_after_bridge=false");
                sc_ep_esl_execute("set", szBuffCMD, pstSCBSubscription->szUUID);

                dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_bridge %s %s \r\n", pstSCBSubscription->szUUID, pstSCBPublish->szUUID);
                sc_ep_esl_execute_cmd(szBuffCMD);
#endif

                /* 维护各种状态，使后续呼叫与transfer流程无关 */
                pstSCBPublish->usOtherSCBNo = pstSCBSubscription->usSCBNo;
                pstSCBSubscription->usOtherSCBNo = pstSCBPublish->usSCBNo;
                pstSCBPublish->ucTranforRole = SC_TRANS_ROLE_BUTT;
                pstSCBSubscription->ucTranforRole = SC_TRANS_ROLE_BUTT;
                pstSCBNotify->ucTranforRole = SC_TRANS_ROLE_BUTT;

                /* 发送话单 */
                pstSCBNotify->usOtherSCBNo = U16_BUTT;
                sc_acd_agent_update_status2(SC_ACTION_AGENT_IDLE, pstSCBNotify->ulAgentID, OPERATING_TYPE_PHONE);
                sc_send_billing_stop2bs(pstSCBNotify);

                /* 清理资源 */
                if (!sc_call_check_service(pstSCBNotify, SC_SERV_AGENT_SIGNIN))
                {
                    sc_scb_free(pstSCBNotify);
                }
            }
            else
            {
                DOS_ASSERT(0);
            }
        }
        else /* Release */
        {
            /* 要求发布方挂断(这个地方没办法发送，发布方还没有接通，那就等发布方接通的时候在处理) */

            /* 维护当前业务控制块的状态为release */
            SC_SCB_SET_SERVICE(pstSCBNotify, SC_SCB_RELEASE);
        }
    }
    else if (SC_SCB_ACTIVE == pstSCBPublish->ucStatus)
    {
        if (SC_SCB_ACTIVE == pstSCBSubscription->ucStatus)
        {
            if (sc_call_check_service(pstSCBPublish, SC_SERV_ATTEND_TRANSFER))
            {
                /* 接通订阅方和发布方 */
                dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_break %s all \r\n", pstSCBSubscription->szUUID);
                sc_ep_esl_execute_cmd(szBuffCMD);

                dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_bridge %s %s \r\n", pstSCBSubscription->szUUID, pstSCBPublish->szUUID);
                sc_ep_esl_execute_cmd(szBuffCMD);
            }

            /* 维护各种状态，保留A和C中的role,方便后面的特殊处理 */
            pstSCBPublish->usOtherSCBNo = pstSCBSubscription->usSCBNo;
            pstSCBSubscription->usOtherSCBNo = pstSCBPublish->usSCBNo;
            pstSCBNotify->ucTranforRole = SC_TRANS_ROLE_BUTT;
            pstSCBSubscription->ucTranforRole = SC_TRANS_ROLE_BUTT;

            /* 将A中的被叫号码改为C中的 */
            dos_strcpy(pstSCBSubscription->szCalleeNum, pstSCBPublish->szCalleeNum);

            /*
                1、B挂断的时间，记录在C的通道变量中
                2、将B的server状态传递给C
                3、此时的B应该生成三个话单
                    a.A和B的话单
                    b.转接的话单
                    c.B和C的话单
                4、可以将B,分两次发送话单。
                    A-B的话单一次，如果有录音业务，则可以包含录音业务的话单
                    B-C一次，包含B-C的通话、转接。
            */
            pstSCBNotify->usOtherSCBNo = U16_BUTT;
#if 0
            for (i=0; i<SC_MAX_SRV_TYPE_PRE_LEG; i++)
            {
                pstSCBNotify->aucServiceType[i] = U8_BUTT;
            }
            pstSCBNotify->bIsNotSrvAdapter = DOS_TRUE;
            /* 分析出A的业务 */
            sc_bs_srv_type_adapter_for_transfer(pstSCBSubscription->aucServiceType, sizeof(pstSCBSubscription->aucServiceType), &ucSubscriptionSrv);
            pstSCBNotify->aucServiceType[0] = ucSubscriptionSrv;
            /* 判断是否有录音 */
            if (sc_call_check_service(pstSCBNotify, SC_SERV_RECORDING))
            {
                pstSCBNotify->aucServiceType[1] = SC_SERV_RECORDING;
            }

            sc_logr_debug(pstSCBNotify, SC_ESL, "Send CDR to bs. SCB1 No:%d, SCB2 No:%d", pstSCBNotify->usSCBNo, pstSCBNotify->usOtherSCBNo);
            /* 发送话单 */
            if (sc_send_billing_stop2bs(pstSCBNotify) != DOS_SUCC)
            {
                sc_logr_debug(pstSCBNotify, SC_ESL, "Send CDR to bs FAIL. SCB1 No:%d, SCB2 No:%d", pstSCBNotify->usSCBNo, pstSCBNotify->usOtherSCBNo);
            }
            else
            {
                sc_logr_debug(pstSCBNotify, SC_ESL, "Send CDR to bs SUCC. SCB1 No:%d, SCB2 No:%d", pstSCBNotify->usSCBNo, pstSCBNotify->usOtherSCBNo);
            }

            for (i=0; i<SC_MAX_SRV_TYPE_PRE_LEG; i++)
            {
                pstSCBNotify->aucServiceType[i] = U8_BUTT;
            }

            /* 分析出C的业务 */
            sc_bs_srv_type_adapter_for_transfer(pstSCBPublish->aucServiceType, sizeof(pstSCBPublish->aucServiceType), &ucPublishSrv);

            /* 将A中的被叫号码改为C中的 */
            dos_strcpy(pstSCBSubscription->szCalleeNum, pstSCBPublish->szCalleeNum);

            /* 将 C 中的转接业务去掉。 如果C为内部呼叫，应该吧内部呼叫也去掉 */
            if (BS_SERV_INTER_CALL == ucPublishSrv)
            {
                for (i=0; i<SC_MAX_SRV_TYPE_PRE_LEG; i++)
                {
                    pstSCBPublish->aucServiceType[i] = U8_BUTT;
                }
            }
            else
            {
                for (i=0; i<SC_MAX_SRV_TYPE_PRE_LEG; i++)
                {
                    if (pstSCBPublish->aucServiceType[i] == SC_SERV_BLIND_TRANSFER
                        || pstSCBPublish->aucServiceType[i] == SC_SERV_ATTEND_TRANSFER)
                    {
                        pstSCBPublish->aucServiceType[i] = U8_BUTT;
                    }
                }
            }

            /* 将B bridge的时间，设置为开始时间 */
            if (pstSCBNotify->pstExtraData->ulTransferTimeStamp != 0)
            {
                /* 修改一下 主被叫号码 */
                dos_strcpy(pstSCBNotify->szCallerNum, pstSCBNotify->szCalleeNum);
                dos_strcpy(pstSCBNotify->szCalleeNum, pstSCBPublish->szCalleeNum);
                pstSCBNotify->pstExtraData->ulAnswerTimeStamp = pstSCBNotify->pstExtraData->ulTransferTimeStamp;
                pstSCBNotify->aucServiceType[0] = ucPublishSrv;
                pstSCBNotify->aucServiceType[1] = BS_SERV_CALL_TRANSFER;

                sc_logr_debug(pstSCBNotify, SC_ESL, "Send CDR to bs. SCB1 No:%d, SCB2 No:%d", pstSCBNotify->usSCBNo, pstSCBNotify->usOtherSCBNo);
                /* 发送话单 */
                if (sc_send_billing_stop2bs(pstSCBNotify) != DOS_SUCC)
                {
                    sc_logr_debug(pstSCBNotify, SC_ESL, "Send CDR to bs FAIL. SCB1 No:%d, SCB2 No:%d", pstSCBNotify->usSCBNo, pstSCBNotify->usOtherSCBNo);
                }
                else
                {
                    sc_logr_debug(pstSCBNotify, SC_ESL, "Send CDR to bs SUCC. SCB1 No:%d, SCB2 No:%d", pstSCBNotify->usSCBNo, pstSCBNotify->usOtherSCBNo);
                }
            }
            else
            {
                /* TODO 时间不对 */
            }
#endif

            sc_acd_agent_update_status2(SC_ACTION_AGENT_IDLE, pstSCBNotify->ulAgentID, OPERATING_TYPE_PHONE);

            /* 清理资源 */
            pstSCBNotify->bCallSip = DOS_FALSE;

            if (sc_call_check_service(pstSCBNotify, SC_SERV_AGENT_SIGNIN))
            {
                /* 长签，如果坐席是通过直接挂断的方式来结束转接这需要特殊操作 */
                if (SC_SCB_ACTIVE == pstSCBNotify->ucStatus)
                {
                    pstSCBNotify->usOtherSCBNo = U16_BUTT;
                    sc_acd_update_agent_info(pstSCBNotify, SC_ACD_IDEL, pstSCBNotify->usSCBNo, NULL);
                }
                else
                {
                    pstSCBNotify->usOtherSCBNo = U16_BUTT;
                    sc_acd_update_agent_info(pstSCBNotify, SC_ACD_IDEL, U32_BUTT, NULL);

                    sc_scb_hash_tables_delete(pstSCBNotify->szUUID);
                    sc_bg_job_hash_delete(pstSCBNotify->usSCBNo);

                    sc_scb_free(pstSCBNotify);
                }
            }
            else
            {
                pstSCBNotify->usOtherSCBNo = U16_BUTT;
                sc_acd_update_agent_info(pstSCBNotify, SC_ACD_IDEL, U32_BUTT, NULL);

                if (SC_SCB_ACTIVE == pstSCBNotify->ucStatus)
                {
                    sc_ep_esl_execute("hangup", "", pstSCBNotify->szUUID);
                }

                sc_scb_hash_tables_delete(pstSCBNotify->szUUID);
                sc_bg_job_hash_delete(pstSCBNotify->usSCBNo);

                sc_scb_free(pstSCBNotify);
            }
        }
        else /* Release */
        {
            /* 要求发布方挂断 */
            sc_ep_esl_execute("hangup", "", pstSCBPublish->szUUID);
        }
    }
    else /* Release */
    {
        /* 这个流程为异常流程 */
        /* 应该在发布方挂断时，让后续呼叫不在于transfer有关 */
        DOS_ASSERT(0);
    }

    return DOS_SUCC;
}

U32 sc_ep_transfer_publish_release(SC_SCB_ST * pstSCBPublish)
{
    SC_SCB_ST* pstSCBSubscription = NULL;
    SC_SCB_ST* pstSCBNotify = NULL;
    S8         szBuffCMD[512] = { 0 };

    if (!SC_SCB_IS_VALID(pstSCBPublish))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_call_trace(pstSCBPublish, "publish has hangup");

    pstSCBNotify = sc_scb_get(pstSCBPublish->usOtherSCBNo);
    if (!SC_SCB_IS_VALID(pstSCBNotify))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCBSubscription = sc_scb_get(pstSCBNotify->usOtherSCBNo);
    if (!SC_SCB_IS_VALID(pstSCBSubscription))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (SC_SCB_ACTIVE == pstSCBNotify->ucStatus)
    {
        if (SC_SCB_ACTIVE == pstSCBSubscription->ucStatus)
        {
            /* 接通订阅放和发起方 */
            dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_break %s all \r\n", pstSCBSubscription->szUUID);
            sc_ep_esl_execute_cmd(szBuffCMD);

            dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_break %s all \r\n", pstSCBNotify->szUUID);
            sc_ep_esl_execute_cmd(szBuffCMD);

            dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_setvar %s mark_customer false \r\n", pstSCBPublish->szUUID);
            sc_ep_esl_execute_cmd(szBuffCMD);

            dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_bridge %s %s \r\n", pstSCBSubscription->szUUID, pstSCBNotify->szUUID);
            sc_ep_esl_execute_cmd(szBuffCMD);

            pstSCBNotify->bTransWaitingBridge = DOS_TRUE;
        }
        else
        {
            /* 要求挂断发起方 */
            sc_ep_esl_execute("hangup", "", pstSCBNotify->szUUID);
        }
    }
    else /* Release */
    {
        if (SC_SCB_ACTIVE == pstSCBSubscription->ucStatus)
        {
            /* 要求挂断订阅方 */
            sc_ep_esl_execute("hangup", "", pstSCBSubscription->szUUID);
        }
        else
        {
            /* 发送订阅方和发起方的话单 */
            /* 发送发布方的话单 */
            sc_send_billing_stop2bs(pstSCBSubscription);

            /* 清理资源 */
            sc_scb_free(pstSCBSubscription);
            /* 释放资源 */
        }
    }

    /* 发送transfer话单，使后续呼叫与transfer无关 */
    pstSCBPublish->usOtherSCBNo = U16_BUTT;
    pstSCBPublish->ucTranforRole = SC_TRANS_ROLE_BUTT;

    pstSCBNotify->usPublishSCB = U16_BUTT;
    pstSCBNotify->ucTranforRole = SC_TRANS_ROLE_BUTT;

    pstSCBSubscription->ucTranforRole = SC_TRANS_ROLE_BUTT;

    /* 发送发布方的话单 */
    sc_send_billing_stop2bs(pstSCBPublish);

    if (sc_call_check_service(pstSCBPublish, SC_SERV_AGENT_SIGNIN))
    {
        /* 清理资源 */
        sc_call_clear_service(pstSCBPublish, SC_SERV_ATTEND_TRANSFER);

        pstSCBPublish->usOtherSCBNo = U16_BUTT;
        sc_acd_update_agent_info(pstSCBPublish, SC_ACD_IDEL, pstSCBPublish->usSCBNo, NULL);

        dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_setvar %s mark_customer false \r\n", pstSCBPublish->szUUID);
        sc_ep_esl_execute_cmd(szBuffCMD);
    }
    else
    {
        pstSCBPublish->usOtherSCBNo = U16_BUTT;
        sc_acd_update_agent_info(pstSCBPublish, SC_ACD_IDEL, U32_BUTT, NULL);

        sc_ep_hangup_call(pstSCBPublish, CC_ERR_NORMAL_CLEAR);

        sc_scb_free(pstSCBPublish);
        pstSCBPublish = NULL;
    }

    return DOS_SUCC;
}


U32 sc_ep_transfer_publish_active(SC_SCB_ST * pstSCBPublish, U32 ulMainService)
{
    SC_SCB_ST* pstSCBSubscription = NULL;
    SC_SCB_ST* pstSCBNotify = NULL;
    S8         szBuffCMD[512] = { 0 };

    if (!SC_SCB_IS_VALID(pstSCBPublish))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstSCBNotify = sc_scb_get(pstSCBPublish->usOtherSCBNo);
    if (!SC_SCB_IS_VALID(pstSCBNotify))
    {
        /* 盲转，第三方已经接通时，第二方已经挂断，所有直接接通就好 */
        if (SC_SERV_BLIND_TRANSFER == ulMainService)
        {
            dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_break %s \r\n", pstSCBNotify->szUUID);
            sc_ep_esl_execute_cmd(szBuffCMD);
            dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_bridge %s %s \r\n", pstSCBNotify->szUUID, pstSCBPublish->szUUID);
            sc_ep_esl_execute_cmd(szBuffCMD);


            return DOS_SUCC;
        }
        else
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
    }

    /* 盲转，第三方已经接通时，第二方已经挂断，所有直接接通就好 */
    if (SC_SERV_BLIND_TRANSFER == ulMainService)
    {
        dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_break %s \r\n", pstSCBNotify->szUUID);
        sc_ep_esl_execute_cmd(szBuffCMD);
        dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_bridge %s %s \r\n", pstSCBNotify->szUUID, pstSCBPublish->szUUID);
        sc_ep_esl_execute_cmd(szBuffCMD);
    }
    else
    {
        pstSCBSubscription = sc_scb_get(pstSCBNotify->usOtherSCBNo);
        if (!SC_SCB_IS_VALID(pstSCBSubscription))
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }

        if (SC_SCB_ACTIVE == pstSCBNotify->ucStatus)
        {
            if (SC_SCB_ACTIVE == pstSCBSubscription->ucStatus)
            {
                /* 接通订阅方和发起方 */
                dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_setvar %s hangup_after_bridge false \r\n", pstSCBNotify->szUUID);
                sc_ep_esl_execute_cmd(szBuffCMD);

                dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "bgapi uuid_bridge %s %s \r\n", pstSCBNotify->szUUID, pstSCBPublish->szUUID);
                sc_ep_esl_execute_cmd(szBuffCMD);
            }
            else
            {
                /* 接通发起方和发布方 */
                dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "uuid_bridge %s %s \r\n", pstSCBNotify->szUUID, pstSCBPublish->szUUID);
                sc_ep_esl_execute_cmd(szBuffCMD);

                /* 维护各种状态，使后续呼叫与transfer流程无关 */
                pstSCBPublish->usOtherSCBNo = pstSCBNotify->usSCBNo;
                pstSCBNotify->usOtherSCBNo = pstSCBPublish->usSCBNo;
                pstSCBPublish->ucTranforRole = SC_TRANS_ROLE_BUTT;
                pstSCBSubscription->ucTranforRole = SC_TRANS_ROLE_BUTT;
                pstSCBNotify->ucTranforRole = SC_TRANS_ROLE_BUTT;

                /* 发送话单 */
                pstSCBSubscription->usOtherSCBNo = U16_BUTT;
                sc_send_billing_stop2bs(pstSCBSubscription);

                /* 清理资源 */
                sc_scb_free(pstSCBSubscription);
            }
        }
        else /* Release */
        {
            if (SC_SCB_ACTIVE == pstSCBSubscription->ucStatus)
            {
                if (sc_call_check_service(pstSCBPublish, SC_SERV_BLIND_TRANSFER))
                {
                    /* 接通订阅方和发布方 */
                    dos_snprintf(szBuffCMD, sizeof(szBuffCMD), "uuid_bridge %s %s \r\n", pstSCBSubscription->szUUID, pstSCBPublish->szUUID);
                    sc_ep_esl_execute_cmd(szBuffCMD);

                    /* 维护各种状态，使后续呼叫与transfer流程无关 */
                    pstSCBPublish->usOtherSCBNo = pstSCBSubscription->usSCBNo;
                    pstSCBSubscription->usOtherSCBNo = pstSCBPublish->usSCBNo;
                    pstSCBPublish->ucTranforRole = SC_TRANS_ROLE_BUTT;
                    pstSCBSubscription->ucTranforRole = SC_TRANS_ROLE_BUTT;
                    pstSCBNotify->ucTranforRole = SC_TRANS_ROLE_BUTT;

                    /* 发送话单 */
                    pstSCBNotify->usOtherSCBNo = U16_BUTT;
                    sc_send_billing_stop2bs(pstSCBNotify);

                    /* 清理资源 */
                    sc_scb_free(pstSCBNotify);

                }
                else if (sc_call_check_service(pstSCBPublish, SC_SERV_ATTEND_TRANSFER))
                {
                    /* 要求发布方挂断，要求订阅方挂断 */
                    sc_ep_esl_execute("hangup", "", pstSCBPublish->szUUID);
                    sc_ep_esl_execute("hangup", "", pstSCBSubscription->szUUID);
                }
                else
                {
                    DOS_ASSERT(0);
                }
            }
            else
            {
                /* 要求释发布方 */
                sc_ep_esl_execute("hangup", "", pstSCBPublish->szUUID);
            }
        }
    }

    return DOS_SUCC;
}

void sc_ep_transfer_publish_server_type(SC_SCB_ST * pstSCB, U32 ulTransferFinishTime)
{
    SC_SCB_ST   *pstSCBOther        = NULL;
    SC_SCB_ST   *pstSCBFrist        = NULL;
    SC_SCB_ST   *pstSCBSecond       = NULL;
    U8          ucPublishSrv         = U8_BUTT;
    U8          ucSubscriptionSrv    = U8_BUTT;
    BOOL        bIsRecord            = DOS_FALSE;
    S32         i;

    pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
    if (DOS_ADDR_INVALID(pstSCBOther))
    {
        /* TODO */
        DOS_ASSERT(0);
        return;
    }

    if (SC_TRANS_ROLE_PUBLISH == pstSCB->ucTranforRole)
    {
        /* 修改一下C的应答时间，换成B挂断通话的时间 */
        pstSCB->pstExtraData->ulAnswerTimeStamp = ulTransferFinishTime;
    }

    if (!pstSCBOther->bWaitingOtherRelase)
    {
        /* 需要等待另一通电话挂断 */
        //pstSCB->bWaitingOtherRelase = DOS_TRUE;
        //break;
        pstSCB->ucTranforRole = SC_TRANS_ROLE_BUTT;
        return;
    }

    if (SC_TRANS_ROLE_SUBSCRIPTION == pstSCB->ucTranforRole)
    {
        /* 需要以pstSCBOther为主leg */
        pstSCBFrist = pstSCBOther;
        pstSCBSecond = pstSCB;
    }
    else
    {
        pstSCBFrist = pstSCB;
        pstSCBSecond = pstSCBOther;
    }

    /* 分析业务,
        1、获取A的业务
        2、获取C的业务。如果C为内部呼叫，则不需要
        3、判断C是否有录音
    */
    pstSCBFrist->bIsFristSCB = DOS_TRUE;
    pstSCBFrist->bIsNotSrvAdapter = DOS_TRUE;
    sc_bs_srv_type_adapter_for_transfer(pstSCBSecond->aucServiceType, sizeof(pstSCBSecond->aucServiceType), &ucSubscriptionSrv);
    sc_bs_srv_type_adapter_for_transfer(pstSCBFrist->aucServiceType, sizeof(pstSCBFrist->aucServiceType), &ucPublishSrv);

    /* 判断是否有录音 */
    if (sc_call_check_service(pstSCBFrist, SC_SERV_RECORDING))
    {
        bIsRecord = DOS_TRUE;
    }

    for (i=0; i<SC_MAX_SRV_TYPE_PRE_LEG; i++)
    {
        pstSCBFrist->aucServiceType[i] = U8_BUTT;
    }

    pstSCBFrist->aucServiceType[0] = ucSubscriptionSrv;
    if (ucPublishSrv == BS_SERV_INTER_CALL && bIsRecord)
    {
        pstSCBFrist->aucServiceType[1] = BS_SERV_RECORDING;
    }
    else if (bIsRecord)
    {
        pstSCBFrist->aucServiceType[1] = ucPublishSrv;
        pstSCBFrist->aucServiceType[2] = BS_SERV_RECORDING;
    }
    else if (ucPublishSrv != BS_SERV_INTER_CALL)
    {
        pstSCBFrist->aucServiceType[1] = ucPublishSrv;
    }
    else
    {
        /* 没有录音的内部呼叫 */
    }

    sc_logr_debug(pstSCB, SC_ESL, "Transfer scb(%u) server type : %u, %u, %u"
        , pstSCBFrist->usSCBNo, pstSCBFrist->aucServiceType[0], pstSCBFrist->aucServiceType[1], pstSCBFrist->aucServiceType[2]);

    return;
}

U32 sc_ep_transfer_subscription_release(SC_SCB_ST * pstSCBSubscription)
{
    SC_SCB_ST* pstSCBNotify = NULL;
    SC_SCB_ST* pstSCBPublish = NULL;

    if (!SC_SCB_IS_VALID(pstSCBSubscription))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_call_trace(pstSCBSubscription, "subscription has hangup");

    pstSCBNotify = sc_scb_get(pstSCBSubscription->usOtherSCBNo);
    if (!SC_SCB_IS_VALID(pstSCBNotify))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCBPublish = sc_scb_get(pstSCBNotify->usPublishSCB);
    if (!SC_SCB_IS_VALID(pstSCBPublish))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstSCBPublish->ucStatus < SC_SCB_ACTIVE)
    {
        if (SC_SCB_ACTIVE == pstSCBNotify->ucStatus)
        {
            /* 置当前为release就好，等待发布方接通做处理 */
            SC_SCB_SET_SERVICE(pstSCBSubscription, SC_SCB_RELEASE);
        }
        else /* Release */
        {
            /* 要求发布方挂断(无法做到哦，这个时候发布方还没有接通，那就只有等到发布方接通的时候处理) */
        }
    }
    else if (SC_SCB_ACTIVE == pstSCBPublish->ucStatus)
    {
        if (SC_SCB_ACTIVE == pstSCBNotify->ucStatus)
        {
            /* 维护各种状态，使后续呼叫与transfer流程无关 */
            pstSCBPublish->usOtherSCBNo = pstSCBNotify->usSCBNo;
            pstSCBNotify->usOtherSCBNo = pstSCBPublish->usSCBNo;
            pstSCBPublish->ucTranforRole = SC_TRANS_ROLE_BUTT;
            pstSCBSubscription->ucTranforRole = SC_TRANS_ROLE_BUTT;
            pstSCBNotify->ucTranforRole = SC_TRANS_ROLE_BUTT;

            /* 发送话单 */
            pstSCBSubscription->usOtherSCBNo = U16_BUTT;
            sc_send_billing_stop2bs(pstSCBSubscription);

            /* 清理资源 */
            sc_scb_free(pstSCBSubscription);
        }
        else /* Release */
        {
            /* 要求发布方挂断 */
            sc_ep_esl_execute("hangup", "", pstSCBPublish->szUUID);
        }
    }
    else /* Release */
    {
        /* 这个流程有问题，应该在发布方挂断的时候进行处理 */
    }

    return DOS_SUCC;
}

/**
 * 第三方transfer的处理
 */
U32 sc_ep_transfer_exec(SC_SCB_ST * pstSCBTmp, U32 ulMainService)
{
    SC_SCB_ST * pstSCBSubscription = NULL;
    SC_SCB_ST* pstSCBPublish = NULL;
    SC_SCB_ST* pstSCBNotify = NULL;
    SC_SCB_ST* pstSCBAgent = NULL;
    SC_ACD_AGENT_INFO_ST stAgentInfo;
    S8         szBuffer[512] = { 0, };

    sc_call_trace(pstSCBSubscription, "Exec transfer.");

    if (DOS_ADDR_INVALID(pstSCBTmp))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (SC_TRANS_ROLE_SUBSCRIPTION == pstSCBTmp->ucTranforRole)
    {
        pstSCBSubscription = pstSCBTmp;
        /* 获取通知方，如果通知方不存在，业务就没办法执行了，且有可能资源泄露 */
        pstSCBNotify = sc_scb_get(pstSCBSubscription->usOtherSCBNo);
        if (DOS_ADDR_INVALID(pstSCBNotify))
        {
            DOS_ASSERT(0);

            /* 释放呼叫 */

            return DOS_FAIL;
        }
    }
    else
    {
        pstSCBNotify = pstSCBTmp;
        /* 获取通知方，如果通知方不存在，业务就没办法执行了，且有可能资源泄露 */
        pstSCBSubscription = sc_scb_get(pstSCBNotify->usOtherSCBNo);
        if (DOS_ADDR_INVALID(pstSCBNotify))
        {
            DOS_ASSERT(0);

            /* 释放呼叫 */

            return DOS_FAIL;
        }
    }

    /* 发布方需要有 */
    pstSCBPublish = sc_scb_get(pstSCBSubscription->usPublishSCB);
    if (DOS_ADDR_INVALID(pstSCBPublish))
    {
        DOS_ASSERT(0);

        /* 释放呼叫 */

        return DOS_FAIL;
    }

    if (sc_acd_get_agent_by_id(&stAgentInfo, pstSCBPublish->ulAgentID) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        /* 释放呼叫 */

        return DOS_FAIL;
    }

    if (!SC_ACD_SITE_IS_USEABLE(&stAgentInfo))
    {
        DOS_ASSERT(0);

        /* 释放呼叫 */

        return DOS_FAIL;
    }

    pstSCBAgent = sc_scb_get(stAgentInfo.usSCBNo);
    if (!SC_SCB_IS_VALID(pstSCBAgent))
    {
        pstSCBAgent = NULL;
    }

    /*
      * 呼叫发布方
      * 盲转 --> 挂断通知方，订阅放放回铃
      * 寻转 --> 给订阅放方等待音, 给通知方发送回铃
      */
    sc_logr_info(pstSCBPublish, SC_ESL, "%s", "Start tansfer call.");
    pstSCBPublish->ucServStatus = SC_SERVICE_ACTIVE;

    /* 如果是盲转，第二方可能已经挂断了, 维护一下相关标准 */
    if (SC_SERV_BLIND_TRANSFER == ulMainService)
    {
        /* 如果长签这个地方就好办了 */
        if (sc_call_check_service(pstSCBAgent, SC_SERV_AGENT_SIGNIN))
        {
            /*
                * 注意:
                * 之前在准备转接时, 产生了新的SCB(pstSCBPublish), 在后续一系列的过程中都是用了这个新的SCB
                * 然而这个地方发现第三方坐席, 已经长签, 对应SCB为(pstSCBAgent), 因此需要将pstSCBPublish
                * 中各种标志copy到pstSCBAgent, 然后释放, 并且维护其他两方的各种标志
                */
            sc_scb_free(pstSCBPublish);
            pstSCBPublish = pstSCBAgent;

            sc_acd_agent_update_status2(SC_ACTION_AGENT_BUSY, pstSCBAgent->ulAgentID, OPERATING_TYPE_PHONE);

            /* 直接置忙 */
            dos_snprintf(szBuffer, sizeof(szBuffer), "bgapi uuid_break %s all\r\n", pstSCBAgent->szUUID);
            sc_ep_esl_execute_cmd(szBuffer);

            dos_snprintf(szBuffer, sizeof(szBuffer), "bgapi uuid_bridge %s %s \r\n", pstSCBAgent->szUUID, pstSCBSubscription->szUUID);
            sc_ep_esl_execute_cmd(szBuffer);
        }
        else
        {
            if (sc_call_check_service(pstSCBPublish, SC_SERV_EXTERNAL_CALL))
            {
                sc_dialer_add_call(pstSCBPublish);
            }
            else
            {
                if (pstSCBPublish->bIsTTCall)
                {
                    sc_dial_make_call2eix(pstSCBPublish, ulMainService);
                }
                else
                {
                    sc_dial_make_call2ip(pstSCBPublish, ulMainService, DOS_FALSE);
                }
            }

            /* 放音 */
            sc_ep_esl_execute("playback", "tone_stream://%(1000,4000,450);loops=-1", pstSCBSubscription->szUUID);
        }

        pstSCBPublish->usOtherSCBNo = pstSCBSubscription->usSCBNo;
        pstSCBSubscription->usOtherSCBNo = pstSCBPublish->usSCBNo;
        pstSCBSubscription->ucTranforRole = SC_TRANS_ROLE_BUTT;
        pstSCBNotify->ucTranforRole = SC_TRANS_ROLE_BUTT;
        pstSCBPublish->ucTranforRole = SC_TRANS_ROLE_BUTT;
        pstSCBSubscription->usPublishSCB = U16_BUTT;
        pstSCBNotify->usPublishSCB = U16_BUTT;


        /* 挂断 */
        pstSCBNotify->usOtherSCBNo = U16_BUTT;
        pstSCBNotify->bCallSip = DOS_FALSE;
        sc_acd_agent_update_status2(SC_ACTION_AGENT_IDLE, pstSCBNotify->ulAgentID, OPERATING_TYPE_PHONE);

        if (!sc_call_check_service(pstSCBNotify, SC_SERV_AGENT_SIGNIN))
        {
            sc_ep_hangup_call(pstSCBNotify, CC_ERR_NORMAL_CLEAR);
        }
        else
        {
            dos_snprintf(szBuffer, sizeof(szBuffer), "bgapi uuid_break %s all\r\n", pstSCBNotify->szUUID);
            sc_ep_esl_execute_cmd(szBuffer);

            sc_ep_esl_execute("set", "playback_terminators=none", pstSCBNotify->szUUID);
            if (sc_ep_play_sound(SC_SND_MUSIC_SIGNIN, pstSCBNotify->szUUID, 1) != DOS_SUCC)
            {
                sc_logr_info(pstSCBNotify, SC_ESL, "%s", "Cannot find the music on hold. just park the call.");
            }
            sc_ep_esl_execute("set", "playback_terminators=*", pstSCBNotify->szUUID);

            pstSCBNotify->bIsInMarkState= DOS_FALSE;
        }
    }
    else if (SC_SERV_ATTEND_TRANSFER == ulMainService)
    {
        /* @todo 这个地方需要将呼叫HOLD然后在放音 */
        sc_ep_play_sound(SC_SND_MUSIC_HOLD, pstSCBSubscription->szUUID, 1);

        /* 如果长签这个地方就好办了 */
        if (sc_call_check_service(pstSCBAgent, SC_SERV_AGENT_SIGNIN))
        {
            /*
                * 注意:
                * 之前在准备转接时, 产生了新的SCB(pstSCBPublish), 在后续一系列的过程中都是用了这个新的SCB
                * 然而这个地方发现第三方坐席, 已经长签, 对应SCB为(pstSCBAgent), 因此需要将pstSCBPublish
                * 中各种标志copy到pstSCBAgent, 然后释放, 并且维护其他两方的各种标志
                */
            SC_SCB_SET_SERVICE(pstSCBAgent, SC_SERV_ATTEND_TRANSFER);
            pstSCBAgent->ucTranforRole = pstSCBPublish->ucTranforRole;
            pstSCBAgent->ucServStatus = pstSCBPublish->ucServStatus;
            pstSCBNotify->usPublishSCB = pstSCBAgent->usSCBNo;
            pstSCBAgent->usOtherSCBNo = pstSCBNotify->usSCBNo;
            sc_scb_free(pstSCBPublish);
            pstSCBPublish = NULL;

            /* 直接置忙 */
            sc_acd_agent_update_status2(SC_ACTION_AGENT_BUSY, pstSCBAgent->ulAgentID, OPERATING_TYPE_PHONE);

            dos_snprintf(szBuffer, sizeof(szBuffer), "bgapi uuid_break %s all \r\n", pstSCBAgent->szUUID);
            sc_ep_esl_execute_cmd(szBuffer);

            dos_snprintf(szBuffer, sizeof(szBuffer), "bgapi uuid_bridge %s %s \r\n", pstSCBAgent->szUUID, pstSCBNotify->szUUID);
            sc_ep_esl_execute_cmd(szBuffer);

        }
        else
        {
            /* 回铃 */
            sc_ep_esl_execute("playback", "tone_stream://%(1000,4000,450);loops=-1", pstSCBNotify->szUUID);

            /* 发起呼叫 */
            if (sc_call_check_service(pstSCBPublish, SC_SERV_EXTERNAL_CALL))
            {
                sc_dialer_add_call(pstSCBPublish);
            }
            else
            {
                if (pstSCBPublish->bIsTTCall)
                {
                    sc_dial_make_call2eix(pstSCBPublish, ulMainService);
                }
                else
                {
                    sc_dial_make_call2ip(pstSCBPublish, ulMainService, DOS_FALSE);
                }
            }
        }
    }

    return DOS_SUCC;
}


U32 sc_ep_call_transfer(SC_SCB_ST * pstSCB, U32 ulAgentCalled, U32 ulNumType, S8 *pszCallee, BOOL bIsAttend)
{
    SC_SCB_ST * pstSCBNew = NULL;
    SC_SCB_ST * pstSCB1 = NULL;
    U32         ulServType;

    if (!SC_SCB_IS_VALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCB1 = sc_scb_get(pstSCB->usOtherSCBNo);
    if (!SC_SCB_IS_VALID(pstSCB1))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pszCallee) || '\0' == pszCallee)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulServType = bIsAttend ? SC_SERV_ATTEND_TRANSFER : SC_SERV_BLIND_TRANSFER;

    /* 先呼叫callee, 如果有任务ID就使用任务所指定的主叫号码，如果没有就用客户的主叫号码 */
    pstSCBNew = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCBNew))
    {
        sc_logr_warning(pstSCB, SC_ESL, "%s", "Cannot make call for the API CMD. Alloc SCB FAIL.");
    }

    pstSCB->usPublishSCB = pstSCBNew->usSCBNo;
    pstSCB->ucTranforRole = SC_TRANS_ROLE_NOTIFY;
    sc_call_trace(pstSCB, "Call trace prepare. Nofify Leg. %u", pstSCB->ucTranforRole);

    pstSCB1->usPublishSCB = pstSCBNew->usSCBNo;
    pstSCB1->ucTranforRole = SC_TRANS_ROLE_SUBSCRIPTION;
    sc_call_trace(pstSCB1, "Call trace prepare. Subscription Leg. %u", pstSCB1->ucTranforRole);

    pstSCBNew->ulCustomID = pstSCB->ulCustomID;
    pstSCBNew->ulAgentID = ulAgentCalled;
    pstSCBNew->ulTaskID = pstSCB->ulTaskID;
    pstSCBNew->usOtherSCBNo = pstSCB->usSCBNo;
    pstSCBNew->ucTranforRole = SC_TRANS_ROLE_PUBLISH;
    pstSCBNew->ucMainService = ulServType;
    pstSCBNew->bIsAgentCall = DOS_TRUE;
    sc_call_trace(pstSCBNew, "Call trace prepare. Publish Leg. %u", pstSCBNew->ucTranforRole);

    /* 需要指定主叫号码 */
    dos_strncpy(pstSCBNew->szCalleeNum, pszCallee, sizeof(pstSCBNew->szCalleeNum));
    pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szCallerNum, pstSCB->szCallerNum, sizeof(pstSCBNew->szCallerNum));
    pstSCBNew->szCallerNum[sizeof(pstSCBNew->szCallerNum) - 1] = '\0';

    SC_SCB_SET_SERVICE(pstSCBNew, ulServType);

    if (!sc_ep_black_list_check(pstSCBNew->ulCustomID, pszCallee))
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Callee in blocak list. (%s)", pszCallee);

        goto proc_fail;
    }

    switch (ulNumType)
    {
        case AGENT_BIND_SIP:
            SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
            SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_INTERNAL_CALL);
            break;

        case AGENT_BIND_TELE:
        case AGENT_BIND_MOBILE:
            SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
            SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_EXTERNAL_CALL);
            break;

        case AGENT_BIND_TT_NUMBER:
            SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
            SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_INTERNAL_CALL);
            pstSCBNew->bIsTTCall = DOS_TRUE;
            break;

        default:
            DOS_ASSERT(0);
            goto proc_fail;
    }


    pstSCBNew->ucServStatus = SC_SERVICE_AUTH;
    if (sc_send_usr_auth2bs(pstSCBNew) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Send auth fail.", pszCallee);

        goto proc_fail;
    }

    return DOS_SUCC;

proc_fail:
    if (DOS_ADDR_INVALID(pstSCBNew))
    {
        sc_scb_free(pstSCBNew);
    }

    return DOS_FAIL;
}


BOOL sc_ep_chack_has_call4agent(U32 ulSCBNo)
{
    SC_SCB_ST *pstSCBAgent = NULL;
    SC_SCB_ST *pstSCBOther = NULL;

    pstSCBAgent = sc_scb_get(ulSCBNo);
    if (DOS_ADDR_INVALID(pstSCBAgent))
    {
        return DOS_FALSE;
    }

    if (sc_call_check_service(pstSCBAgent, SC_SERV_AGENT_SIGNIN))
    {
        pstSCBOther = sc_scb_get(pstSCBAgent->usOtherSCBNo);
        if (DOS_ADDR_INVALID(pstSCBOther))
        {
            return DOS_FALSE;
        }
    }

    return DOS_TRUE;
}

U32 sc_ep_call_intercept(SC_SCB_ST * pstSCB)
{
    SC_SCB_ST *pstSCBAgent = NULL;
    SC_ACD_AGENT_INFO_ST stAgentInfo;

    /* 查找坐席 */
    if (sc_acd_get_agent_by_id(&stAgentInfo, pstSCB->ulAgentID) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot intercept call for agent with id %u. Agent not found..", pstSCB->ulAgentID);
        goto proc_fail;
    }

    if (stAgentInfo.usSCBNo >= SC_MAX_SCB_NUM)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot intercept call for agent with id %u. Agent handle a invalid SCB No(%u).", pstSCB->ulAgentID, stAgentInfo.usSCBNo);
        goto proc_fail;
    }

    pstSCBAgent = sc_scb_get(stAgentInfo.usSCBNo);
    if (DOS_ADDR_INVALID(pstSCBAgent) || !pstSCBAgent->bValid)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot intercept call for agent with id %u. Agent handle a SCB(%u) is invalid.", pstSCB->ulAgentID, stAgentInfo.usSCBNo);
        goto proc_fail;
    }

    if ('\0' == pstSCBAgent->szUUID[0])
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent handle a SCB(%u) without UUID.", pstSCB->ulAgentID, stAgentInfo.usSCBNo);
        goto proc_fail;
    }


    if (sc_call_check_service(pstSCB, SC_SERV_CALL_INTERCEPT))
    {
        sc_ep_esl_execute("eavesdrop", pstSCBAgent->szUUID, pstSCB->szUUID);
    }
    else if (sc_call_check_service(pstSCB, SC_SERV_CALL_WHISPERS))
    {
        sc_ep_esl_execute("queue_dtmf", "w2@500", pstSCB->szUUID);
        sc_ep_esl_execute("eavesdrop", pstSCBAgent->szUUID, pstSCB->szUUID);
    }
    else
    {
        DOS_ASSERT(0);
        goto proc_fail;
    }

    return DOS_SUCC;

proc_fail:

    sc_ep_terminate_call(pstSCB);

    return DOS_FAIL;
}

U32 sc_ep_call_agent(SC_SCB_ST *pstSCB, SC_ACD_AGENT_INFO_ST *pstAgentInfo, BOOL bIsUpdateCaller, U32 *pulErrCode)
{
    S8            szAPPParam[512] = { 0 };
    U32           ulErrCode = CC_ERR_NO_REASON;
    S8            szNumber[SC_TEL_NUMBER_LENGTH] = {0};
    U32           ulRet;
    SC_SCB_ST     *pstSCBNew = NULL;
    SC_SCB_ST     *pstSCBRecord = NULL;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstAgentInfo) || DOS_ADDR_INVALID(pulErrCode))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* 判断坐席的状态 */
    if (pstAgentInfo->ucStatus != SC_ACD_IDEL)
    {
        /* 不允许呼叫 */
        if (pstAgentInfo->ucStatus == SC_ACD_OFFLINE)
        {
            ulErrCode = CC_ERR_SC_USER_HAS_BEEN_LEFT;
        }
        else
        {
            ulErrCode = CC_ERR_SC_USER_BUSY;
        }

        sc_logr_info(pstSCB, SC_ESL, "Call agnet FAIL. Agent (%u) status is (%d)", pstAgentInfo->ulSiteID, pstAgentInfo->ucStatus);
        goto proc_error;
    }

    sc_acd_agent_stat(SC_AGENT_STAT_CALL, pstAgentInfo->ulSiteID, NULL, 0);

    /* 应该把坐席的工号存储到 pstSCB 中，结果分析时，使用 */
    if (!pstSCB->bIsAgentCall)
    {
        dos_strncpy(pstSCB->szSiteNum, pstAgentInfo->szEmpNo, sizeof(pstSCB->szSiteNum));
        pstSCB->szSiteNum[sizeof(pstSCB->szSiteNum) - 1] = '\0';
    }

    pstAgentInfo->stStat.ulCallCnt++;
    pstAgentInfo->stStat.ulCallCnt++;

    /* 如果坐席长连了，就需要特殊处理 */
    if (pstAgentInfo->bConnected && pstAgentInfo->usSCBNo < SC_MAX_SCB_NUM)
    {
        pstSCBNew = sc_scb_get(pstAgentInfo->usSCBNo);
        if (DOS_ADDR_INVALID(pstSCBNew))
        {
            ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
            DOS_ASSERT(0);
            goto proc_error;
        }

        if ('\0' == pstSCBNew->szUUID[0])
        {
            ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
            DOS_ASSERT(0);
            goto proc_error;
        }

        pstSCBNew->bRecord = pstAgentInfo->bRecord;
        pstSCBNew->bTraceNo = pstAgentInfo->bTraceON;
        /* 更新 pstSCBNew 中的主被叫号码 */
        if (pstSCB->bIsPassThrough)
        {
            dos_strncpy(pstSCBNew->szCallerNum, pstSCB->szCallerNum, sizeof(pstSCBNew->szCallerNum));
        }
        else
        {
            dos_strncpy(pstSCBNew->szCallerNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCallerNum));
        }
        pstSCBNew->szCallerNum[sizeof(pstSCBNew->szCallerNum) - 1] = '\0';
        dos_strncpy(pstSCBNew->szANINum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szANINum));
        pstSCBNew->szANINum[sizeof(pstSCBNew->szANINum) - 1] = '\0';
        dos_strncpy(pstSCBNew->szCalleeNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCalleeNum));
        pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';

        /* 如果为群呼任务，则customer num 为被叫号码，如果是呼入，则是主叫号码 */
        if (sc_call_check_service(pstSCB, SC_SERV_AUTO_DIALING))
        {
            dos_strncpy(pstSCBNew->szCustomerNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCustomerNum));
            pstSCBNew->szCustomerNum[sizeof(pstSCBNew->szCustomerNum) - 1] = '\0';
        }
        else
        {
            dos_strncpy(pstSCBNew->szCustomerNum, pstSCB->szCallerNum, sizeof(pstSCBNew->szCustomerNum));
            pstSCBNew->szCustomerNum[sizeof(pstSCBNew->szCustomerNum) - 1] = '\0';
        }

        if (!pstSCB->bIsAgentCall)
        {
            /* 不是坐席呼叫坐席时，才弹屏 */
            if (sc_ep_call_notify(pstAgentInfo, pstSCBNew->szCustomerNum) != DOS_SUCC)
            {
                DOS_ASSERT(0);
            }
        }

        /* 更新坐席的状态 */
        sc_acd_update_agent_info(pstSCBNew, SC_ACD_BUSY, pstSCBNew->usSCBNo, pstSCB->szCallerNum);

        pstSCBNew->usOtherSCBNo = pstSCB->usSCBNo;
        pstSCB->usOtherSCBNo = pstSCBNew->usSCBNo;

        pstSCBNew->bCallSip = DOS_TRUE;

        /* 判断是否需要录音 */
        if (pstSCBNew->bRecord
            || (pstSCB->bIsAgentCall && pstSCB->bRecord))
        {
            if (pstSCBNew->bRecord)
            {
                pstSCBRecord = pstSCBNew;
            }
            else
            {
                pstSCBRecord = pstSCB;
            }

            if (sc_ep_record(pstSCBRecord) != DOS_SUCC)
            {
                DOS_ASSERT(0);
            }
        }

        /* 给通道设置变量 */
        sc_ep_esl_execute("set", "exec_after_bridge_app=park", pstSCBNew->szUUID);
        dos_snprintf(szAPPParam, sizeof(szAPPParam), "bgapi uuid_break %s all \r\n", pstSCBNew->szUUID);
        sc_ep_esl_execute_cmd(szAPPParam);
        /* 不answer，没有语音 */
        sc_ep_esl_execute("answer", NULL, pstSCB->szUUID);

        dos_snprintf(szAPPParam, sizeof(szAPPParam), "bgapi uuid_bridge %s %s \r\n", pstSCBNew->szUUID, pstSCB->szUUID);
        if (sc_ep_esl_execute_cmd(szAPPParam) != DOS_SUCC)
        {
            ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
            DOS_ASSERT(0);
            goto proc_error;
        }

        return DOS_SUCC;
    }

    pstSCBNew = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCBNew))
    {
        DOS_ASSERT(0);

        sc_logr_error(pstSCB, SC_ESL, "%s", "Allc SCB FAIL.");
        ulErrCode = CC_ERR_SC_CB_ALLOC_FAIL;
        goto proc_error;
    }

    //pthread_mutex_lock(&pstSCBNew->mutexSCBLock);

    /* 根据ulSiteID, 找到坐席，给坐席的 usSCBNo 赋值 */
    if (sc_acd_update_agent_scbno_by_siteid(pstAgentInfo->ulSiteID, pstSCBNew, NULL, NULL) != DOS_SUCC)
    {
        sc_logr_info(pstSCB, SC_ESL, "Update agent(%u) scbno(%d) fail", pstAgentInfo->ulSiteID, pstSCBNew->usSCBNo);
    }

    pstSCBNew->bIsAgentCall = DOS_TRUE;
    pstSCB->usOtherSCBNo = pstSCBNew->usSCBNo;
    pstSCBNew->ulCustomID = pstSCB->ulCustomID;
    pstSCBNew->ulAgentID = pstAgentInfo->ulSiteID;
    pstSCBNew->usOtherSCBNo = pstSCB->usSCBNo;
    pstSCBNew->ucLegRole = SC_CALLEE;
    pstSCBNew->bRecord = pstAgentInfo->bRecord;
    pstSCBNew->bTraceNo = pstAgentInfo->bTraceON;
    pstSCBNew->bIsAgentCall = DOS_TRUE;

    /* 如果为群呼任务，则customer num 为被叫号码，如果是呼入，则是主叫号码 */
    if (sc_call_check_service(pstSCB, SC_SERV_AUTO_DIALING))
    {
        dos_strncpy(pstSCBNew->szCustomerNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCustomerNum));
        pstSCBNew->szCustomerNum[sizeof(pstSCBNew->szCustomerNum) - 1] = '\0';
    }
    else
    {
        dos_strncpy(pstSCBNew->szCustomerNum, pstSCB->szCallerNum, sizeof(pstSCBNew->szCustomerNum));
        pstSCBNew->szCustomerNum[sizeof(pstSCBNew->szCustomerNum) - 1] = '\0';
    }

    if (pstSCB->bIsPassThrough)
    {
        dos_strncpy(pstSCBNew->szCallerNum, pstSCB->szCallerNum, sizeof(pstSCBNew->szCallerNum));
    }
    else
    {
        dos_strncpy(pstSCBNew->szCallerNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCallerNum));
    }
    pstSCBNew->szCallerNum[sizeof(pstSCBNew->szCallerNum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szANINum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szANINum));
    pstSCBNew->szANINum[sizeof(pstSCBNew->szANINum) - 1] = '\0';
    switch (pstAgentInfo->ucBindType)
    {
        case AGENT_BIND_SIP:
            dos_strncpy(pstSCBNew->szCalleeNum, pstAgentInfo->szUserID, sizeof(pstSCBNew->szCalleeNum));
            pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';
            break;
        case AGENT_BIND_TELE:
            dos_strncpy(pstSCBNew->szCalleeNum, pstAgentInfo->szTelePhone, sizeof(pstSCBNew->szCalleeNum));
            pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';
            break;
        case AGENT_BIND_MOBILE:
            dos_strncpy(pstSCBNew->szCalleeNum, pstAgentInfo->szMobile, sizeof(pstSCBNew->szCalleeNum));
            pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';
            break;
        case AGENT_BIND_TT_NUMBER:
            dos_strncpy(pstSCBNew->szCalleeNum, pstAgentInfo->szTTNumber, sizeof(pstSCBNew->szCalleeNum));
            pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';
            break;
    }

    dos_strncpy(pstSCBNew->szSiteNum, pstAgentInfo->szEmpNo, sizeof(pstSCBNew->szSiteNum));
    pstSCBNew->szSiteNum[sizeof(pstSCBNew->szSiteNum) - 1] = '\0';
    //pthread_mutex_unlock(&pstSCB->mutexSCBLock);
    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_AGENT_CALLBACK);

    SC_SCB_SET_STATUS(pstSCBNew, SC_SCB_EXEC);

    if (AGENT_BIND_SIP != pstAgentInfo->ucBindType
        && AGENT_BIND_TT_NUMBER != pstAgentInfo->ucBindType)
    {
        SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
        SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_EXTERNAL_CALL);

        if (bIsUpdateCaller)
        {
            /* 设置主叫号码 */
            if (U32_BUTT == pstSCB->ulAgentID)
            {
                sc_logr_info(pstSCB, SC_DIALER, "Agent signin not get agent ID by scd(%u)", pstSCB->usSCBNo);

                goto go_on;
            }

            /* 查找呼叫源和号码的对应关系，如果匹配上某一呼叫源，就选择特定号码 */
            ulRet = sc_caller_setting_select_number(pstSCB->ulCustomID, pstSCB->ulAgentID, SC_SRC_CALLER_TYPE_AGENT, szNumber, SC_TEL_NUMBER_LENGTH);
            if (ulRet != DOS_SUCC)
            {
                sc_logr_info(pstSCB, SC_DIALER, "Agent signin customID(%u) get caller number FAIL by agent(%u)", pstSCB->ulCustomID, pstSCB->ulAgentID);

                goto go_on;
            }

            sc_logr_info(pstSCB, SC_DIALER, "Agent signin customID(%u) get caller number(%s) SUCC by agent(%u)", pstSCB->ulCustomID, szNumber, pstSCB->ulAgentID);
            dos_strncpy(pstSCB->szCallerNum, szNumber, SC_TEL_NUMBER_LENGTH);
            pstSCB->szCallerNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';
        }
go_on:

        if (!sc_ep_black_list_check(pstSCBNew->ulCustomID, pstSCBNew->szCalleeNum))
        {
            DOS_ASSERT(0);
            sc_logr_info(pstSCB, SC_ESL, "Cannot make call for number %s which is in black list.", pstSCBNew->szCalleeNum);
            ulErrCode = CC_ERR_SC_IN_BLACKLIST;
            goto proc_error;
        }

        if (sc_send_usr_auth2bs(pstSCBNew) != DOS_SUCC)
        {
            sc_logr_notice(pstSCB, SC_ESL, "Send auth msg FAIL. SCB No: %d", pstSCBNew->usSCBNo);
            ulErrCode = CC_ERR_SC_MESSAGE_SENT_ERR;
            goto proc_error;
        }

        return DOS_SUCC;
    }

    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_INTERNAL_CALL);

    if (AGENT_BIND_SIP == pstAgentInfo->ucBindType)
    {
        if (sc_dial_make_call2ip(pstSCBNew, SC_SERV_AGENT_CALLBACK, bIsUpdateCaller) != DOS_SUCC)
        {
            ulErrCode = CC_ERR_SIP_BAD_GATEWAY;
            goto proc_error;
        }
    }
    else if (AGENT_BIND_TT_NUMBER == pstAgentInfo->ucBindType)
    {
        if (sc_dial_make_call2eix(pstSCBNew, SC_SERV_AGENT_CALLBACK) != DOS_SUCC)
        {
            ulErrCode = CC_ERR_SIP_BAD_GATEWAY;
            goto proc_error;
        }
    }

    if (!pstSCB->bIsAgentCall)
    {
        /* 不是坐席呼叫坐席时，才弹屏 */
        if (sc_ep_call_notify(pstAgentInfo, pstSCBNew->szCustomerNum) != DOS_SUCC)
        {
            DOS_ASSERT(0);
        }
    }

    return DOS_SUCC;

proc_error:
    *pulErrCode = ulErrCode;
    if (pstSCBNew)
    {
        DOS_ASSERT(0);
        sc_scb_free(pstSCBNew);
        pstSCBNew = NULL;
    }
    return DOS_FAIL;
}

/**
 * 函数: sc_ep_call_agent_by_id
 * 功能: 呼叫某一个特定的坐席
 * 参数:
 *      SC_SCB_ST *pstSCB       : 业务控制块
 *      U32 ulAgentID           : 坐席ID
 *      BOOL bIsUpdateCaller    : 是否使用主叫号码组中的号码
 *      BOOL bIsPassThrough     : 是否透传主叫号码
 * 返回值: 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_ep_call_agent_by_id(SC_SCB_ST * pstSCB, U32 ulAgentID, BOOL bIsUpdateCaller, BOOL bIsPassThrough)
{
    SC_ACD_AGENT_INFO_ST stAgentInfo;
    U32 ulErrCode = CC_ERR_NO_REASON;

    /* 查找坐席 */
    if (sc_acd_get_agent_by_id(&stAgentInfo, ulAgentID) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        ulErrCode = CC_ERR_SC_USER_DOES_NOT_EXIST;
        sc_logr_info(pstSCB, SC_ESL, "Cannot call agent with id %u. Agent not found, Errcode : %u", ulAgentID, ulErrCode);
        goto proc_fail;
    }

    pstSCB->bIsPassThrough = bIsPassThrough;
    /* 呼叫坐席 */
    if (sc_ep_call_agent(pstSCB, &stAgentInfo, bIsUpdateCaller, &ulErrCode) != DOS_SUCC)
    {
        goto proc_fail;
    }

    return DOS_SUCC;

proc_fail:
    if (DOS_ADDR_VALID(pstSCB))
    {
        sc_ep_hangup_call_with_snd(pstSCB, ulErrCode);
    }
    return DOS_FAIL;
}

/**
 * 函数: U32 sc_ep_call_agent_by_grpid(SC_SCB_ST *pstSCB, U32 ulAgentQueue)
 * 功能: 群呼任务之后接通坐席
 * 参数:
 *      SC_SCB_ST *pstSCB       : 业务控制块
 * 返回值: 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_ep_call_agent_by_grpid(SC_SCB_ST *pstSCB, U32 ulTaskAgentQueueID)
{
    U32           ulErrCode = CC_ERR_NO_REASON;
    SC_ACD_AGENT_INFO_ST stAgentInfo;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstSCB->bIsInQueue = DOS_FALSE;

    sc_log_digest_print("Start call agent by groupid(%u)", ulTaskAgentQueueID);
    /* 1.获取坐席队列，2.查找坐席。3.接通坐席 */
    if (U32_BUTT == ulTaskAgentQueueID)
    {
        /* 前面已经判断过，如果还出错，暂定为 系统错误 */
        DOS_ASSERT(0);
        sc_logr_info(pstSCB, SC_ESL, "Cannot get the agent queue for the task %d", pstSCB->ulTaskID);
        ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
        goto proc_fail;
    }

    if (sc_acd_get_agent_by_grpid(&stAgentInfo, ulTaskAgentQueueID, pstSCB->szCalleeNum) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_notice(pstSCB, SC_ESL, "There is no useable agent for the task %d. Queue: %d. ", pstSCB->ulTaskID, ulTaskAgentQueueID);
        ulErrCode = CC_ERR_SC_USER_BUSY;
        goto proc_fail;
    }

    sc_logr_info(pstSCB, SC_ESL, "Select agent for call OK. Agent ID: %d, User ID: %s, Externsion: %s, Job-Num: %s"
                    , stAgentInfo.ulSiteID
                    , stAgentInfo.szUserID
                    , stAgentInfo.szExtension
                    , stAgentInfo.szEmpNo);


    /* 呼叫坐席 */
    if (sc_ep_call_agent(pstSCB, &stAgentInfo, DOS_FALSE, &ulErrCode) != DOS_SUCC)
    {
        goto proc_fail;
    }

    return DOS_SUCC;

proc_fail:
    if (DOS_ADDR_VALID(pstSCB))
    {
        sc_ep_hangup_call_with_snd(pstSCB, ulErrCode);
    }
    sc_log_digest_print("Call agent by groupid(%u) FAIL", ulTaskAgentQueueID);

    return DOS_FAIL;
}

/**
 * 函数: sc_ep_call_queue_add
 * 功能: 接通坐席时，将呼叫加入队列
 * 参数:
 *      SC_SCB_ST *pstSCB       : 业务控制块
 * 返回值: 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_ep_call_queue_add(SC_SCB_ST *pstSCB, U32 ulTaskAgentQueueID, BOOL bIsPassThrough)
{
    U32 ulResult;
    S8  szAPPParam[512]    = { 0, };

    if (DOS_ADDR_INVALID(pstSCB))
    {
        return DOS_FAIL;
    }

    pstSCB->bIsPassThrough = bIsPassThrough;
    ulResult = sc_cwq_add_call(pstSCB, ulTaskAgentQueueID);
    if (ulResult == DOS_SUCC)
    {
        pstSCB->bIsInQueue = DOS_TRUE;

        sc_ep_esl_execute("set", "hungup_after_play=false", pstSCB->szUUID);
        sc_ep_play_sound(SC_SND_CONNECTING, pstSCB->szUUID, 1);
        dos_snprintf(szAPPParam, sizeof(szAPPParam)
                        , "{timeout=180000}file_string://%s/music_hold.wav"
                        , SC_PROMPT_TONE_PATH);
        sc_ep_esl_execute("playback", szAPPParam, pstSCB->szUUID);
    }

    return ulResult;
}

/* 拆除坐席呼叫的接口 */
U32 sc_ep_call_ctrl_hangup(U32 ulAgent)
{
    SC_SCB_ST *pstSCB       = NULL;
    SC_SCB_ST *pstSCBOther  = NULL;
    SC_ACD_AGENT_INFO_ST stAgentInfo;

    sc_log_digest_print("Hangup Agent(%u) call.", ulAgent);
    /* 查找坐席 */
    if (sc_acd_get_agent_by_id(&stAgentInfo, ulAgent) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent not found..", ulAgent);
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
    }
    else
    {
        if (!stAgentInfo.bNeedConnected)
        {
            sc_ep_hangup_call(pstSCB, CC_ERR_SC_CLEAR_FORCE);
        }
    }

    pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
    if (DOS_ADDR_INVALID(pstSCBOther) || !pstSCBOther->bValid)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent handle a SCB(%u) is invalid.", ulAgent, stAgentInfo.usSCBNo);
        goto proc_fail;
    }

    if ('\0' == pstSCBOther->szUUID[0])
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent handle a SCB(%u) without UUID.", ulAgent, stAgentInfo.usSCBNo);
        goto proc_fail;
    }

    sc_ep_hangup_call(pstSCBOther, CC_ERR_SC_CLEAR_FORCE);

    sc_logr_notice(pstSCB, SC_ESL, "Request hangup call FAIL. Agent: %u", ulAgent);
    return DOS_SUCC;

proc_fail:
    sc_logr_notice(pstSCB, SC_ESL, "Request hangup call FAIL. Agent: %u", ulAgent);
    return DOS_FAIL;
}

/* 因为某些操作需要强制拆除坐席相关的呼叫的接口函数 */
U32 sc_ep_call_ctrl_hangup_all(U32 ulAgent)
{
    return sc_ep_call_ctrl_hangup(ulAgent);
}

U32 sc_ep_call_ctrl_hangup_agent(SC_ACD_AGENT_INFO_ST *pstAgentQueueInfo)
{
    SC_SCB_ST *pstSCB       = NULL;
    SC_SCB_ST *pstSCBOther  = NULL;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (pstAgentQueueInfo->usSCBNo >= SC_MAX_SCB_NUM)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent handle a invalid SCB No(%u)."
                        , pstAgentQueueInfo->ulSiteID
                        , pstAgentQueueInfo->usSCBNo);

        return DOS_FAIL;
    }

    pstSCB = sc_scb_get(pstAgentQueueInfo->usSCBNo);
    if (DOS_ADDR_INVALID(pstSCB) || !pstSCB->bValid)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent handle a SCB(%u) is invalid."
                        , pstAgentQueueInfo->ulSiteID
                        , pstAgentQueueInfo->usSCBNo);

        return DOS_FAIL;
    }

    sc_logr_notice(pstSCB, SC_ESL, "Hangup call for force. Agent: %u, SCB: %u"
                    , pstAgentQueueInfo->ulSiteID
                    , pstAgentQueueInfo->usSCBNo);
    pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);

    sc_ep_hangup_call(pstSCB, CC_ERR_SC_CLEAR_FORCE);
    if (pstSCBOther)
    {
        sc_ep_hangup_call(pstSCBOther, CC_ERR_SC_CLEAR_FORCE);
    }

    return DOS_SUCC;
}

/* hold坐席相关的呼叫 */
U32 sc_ep_call_ctrl_hold(U32 ulAgent, BOOL isHold)
{
    SC_SCB_ST *pstSCB       = NULL;
    SC_SCB_ST *pstSCBOther  = NULL;
    S8        szCMD[256] = { 0, };
    SC_ACD_AGENT_INFO_ST stAgentInfo;

    sc_logr_notice(pstSCB, SC_ESL, "Request %s call. Agent: %u", isHold ? "hold" : "unhold", ulAgent);

    /* 查找坐席 */
    if (sc_acd_get_agent_by_id(&stAgentInfo, ulAgent) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hold call for agent with id %u. Agent not found..", ulAgent);
        goto proc_fail;
    }

    if (stAgentInfo.usSCBNo >= SC_MAX_SCB_NUM)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hold call for agent with id %u. Agent handle a invalid SCB No(%u).", ulAgent, stAgentInfo.usSCBNo);
        goto proc_fail;
    }

    pstSCB = sc_scb_get(stAgentInfo.usSCBNo);
    if (DOS_ADDR_INVALID(pstSCB) || !pstSCB->bValid)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hold call for agent with id %u. Agent handle a SCB(%u) is invalid.", ulAgent, stAgentInfo.usSCBNo);
        goto proc_fail;
    }

    pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
    if (DOS_ADDR_INVALID(pstSCBOther) || !pstSCBOther->bValid)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hold call for agent with id %u. Agent handle a SCB(%u) is invalid.", ulAgent, stAgentInfo.usSCBNo);
        goto proc_fail;
    }

    if ('\0' == pstSCBOther->szUUID[0])
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hold call for agent with id %u. Agent handle a SCB(%u) without UUID.", ulAgent, stAgentInfo.usSCBNo);
        goto proc_fail;
    }

    if (isHold)
    {
        dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_hold %s", pstSCB->szUUID);
        if (sc_ep_esl_execute_cmd(szCMD) != DOS_SUCC)
        {
            DOS_ASSERT(0);
            sc_logr_info(pstSCB, SC_ESL, "Hold FAIL. %s", pstSCB->szUUID);

            goto proc_fail;
        }
    }
    else
    {
        dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_hold off %s", pstSCB->szUUID);
        if (sc_ep_esl_execute_cmd(szCMD) != DOS_SUCC)
        {
            DOS_ASSERT(0);
            sc_logr_info(pstSCB, SC_ESL, "Unhold FAIL. %s", pstSCB->szUUID);

            goto proc_fail;
        }
    }

    sc_logr_notice(pstSCB, SC_ESL, "Request %s call SUCC. Agent: %u", isHold ? "hold" : "unhold", ulAgent);
    return DOS_SUCC;

proc_fail:
    sc_logr_notice(pstSCB, SC_ESL, "Request %s call FAIL. Agent: %u", isHold ? "hold" : "unhold", ulAgent);
    return DOS_FAIL;
}

/* 解除HOLD相关的呼叫坐席相关的呼叫 */
U32 sc_ep_call_ctrl_unhold(U32 ulAgent)
{
    return sc_ep_call_ctrl_hold(ulAgent, DOS_FALSE);
}

/* 呼叫一个坐席 */
U32 sc_ep_call_ctrl_call_agent(U32 ulCurrentAgent, U32 ulAgentCalled)
{
    SC_SCB_ST *pstSCB       = NULL;
    SC_SCB_ST *pstSCBOther  = NULL;
    SC_ACD_AGENT_INFO_ST stAgentInfo, stCalleeAgentInfo;
    S8 szParams[512]        = {0};

    sc_logr_info(pstSCB, SC_ESL, "Request call agent %u(%u)", ulAgentCalled, ulCurrentAgent);
    sc_log_digest_print("Agent(%u) call other agent(%u) start", ulCurrentAgent, ulAgentCalled);

    /* 查找坐席 */
    if (sc_acd_get_agent_by_id(&stAgentInfo, ulCurrentAgent) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_info(pstSCB, SC_ESL, "Cannot call agent with id %u. Agent not found.", ulCurrentAgent);
        goto make_all_fail;
    }

    /* 规格为: 坐席点击呼叫，如果坐席正在进行呼叫业务，则不允许呼叫，否则可以呼叫 */
#if 0
    if (stAgentInfo.ucStatus != SC_ACD_IDEL && )
    {
        /* 不允许呼叫 */
        sc_logr_info(pstSCB, SC_ESL, "Call agnet FAIL. Agent (%u) status is (%d)", ulCurrentAgent, stAgentInfo.ucStatus);
        goto make_all_fail;
    }
#endif

    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        sc_logr_warning(pstSCB, SC_ESL, "%s", "Cannot make call for the API CMD. Alloc SCB FAIL.");

        goto make_all_fail;
    }

    dos_strncpy(pstSCB->szSiteNum, stAgentInfo.szEmpNo, sizeof(pstSCB->szSiteNum));
    pstSCB->szSiteNum[sizeof(pstSCB->szSiteNum) - 1] = '\0';

    if (stAgentInfo.bConnected && stAgentInfo.usSCBNo < SC_MAX_SCB_NUM)
    {
        /* 坐席长连 */
        pstSCBOther = sc_scb_get(stAgentInfo.usSCBNo);
        if (DOS_ADDR_INVALID(pstSCBOther))
        {
            sc_logr_warning(pstSCB, SC_ESL, "%s", "Get SCB fail for signin agent.");
            goto make_all_fail;
        }

        sc_log_digest_print("Agent(%u) Connected, now call other agent(%u)", ulCurrentAgent, ulAgentCalled);

        /* 更新坐席的状态 */
        sc_acd_update_agent_info(pstSCBOther, SC_ACD_BUSY, pstSCBOther->usSCBNo, NULL);

        sc_scb_free(pstSCB);
        dos_snprintf(szParams, sizeof(szParams), "bgapi uuid_break %s all \r\n", pstSCBOther->szUUID);
        sc_ep_esl_execute_cmd(szParams);
        /* 放回铃音 */
        sc_ep_esl_execute("set", "instant_ringback=true", pstSCBOther->szUUID);
        sc_ep_esl_execute("set", "transfer_ringback=tone_stream://%(1000,4000,450);loops=-1", pstSCBOther->szUUID);
        sc_ep_call_agent_by_id(pstSCBOther, ulAgentCalled, DOS_FALSE, DOS_FALSE);
    }
    else
    {
        sc_log_digest_print("Agent(%u) is not Connected, now call agent", ulCurrentAgent);

        pstSCB->enCallCtrlType = SC_CALL_CTRL_TYPE_AGENT;
        pstSCB->ulOtherAgentID = ulAgentCalled;

        pstSCB->ulCustomID = stAgentInfo.ulCustomerID;
        pstSCB->ulAgentID  = ulCurrentAgent;
        pstSCB->ulTaskID   = U32_BUTT;
        pstSCB->bRecord    = stAgentInfo.bRecord;
        pstSCB->bTraceNo   = stAgentInfo.bTraceON;
        pstSCB->bIsAgentCall = DOS_TRUE;

        /* 如果呼叫的坐席 */
        if (sc_acd_get_agent_by_id(&stCalleeAgentInfo, ulAgentCalled) != DOS_SUCC)
        {
            DOS_ASSERT(0);

            sc_logr_info(pstSCB, SC_ESL, "Call agent(%u) FAIL. Agent not found.", ulAgentCalled);
            goto make_all_fail;
        }

        switch (stCalleeAgentInfo.ucBindType)
        {
            case AGENT_BIND_SIP:
                dos_strncpy(pstSCB->szCallerNum, stCalleeAgentInfo.szUserID, sizeof(pstSCB->szCallerNum));
                pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) - 1] = '\0';
                break;
            case AGENT_BIND_TELE:
                dos_strncpy(pstSCB->szCallerNum, stCalleeAgentInfo.szTelePhone, sizeof(pstSCB->szCallerNum));
                pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) - 1] = '\0';
                break;
            case AGENT_BIND_MOBILE:
                dos_strncpy(pstSCB->szCallerNum, stCalleeAgentInfo.szMobile, sizeof(pstSCB->szCallerNum));
                pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) - 1] = '\0';
                break;
            case AGENT_BIND_TT_NUMBER:
                dos_strncpy(pstSCB->szCallerNum, stCalleeAgentInfo.szTTNumber, sizeof(pstSCB->szCallerNum));
                pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) - 1] = '\0';
                break;
        }

        /* 指定被叫号码 */
        switch (stAgentInfo.ucBindType)
        {
            case AGENT_BIND_SIP:
                dos_strncpy(pstSCB->szCalleeNum, stAgentInfo.szUserID, sizeof(pstSCB->szCalleeNum));
                pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
                break;
            case AGENT_BIND_TELE:
                dos_strncpy(pstSCB->szCalleeNum, stAgentInfo.szTelePhone, sizeof(pstSCB->szCalleeNum));
                pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
                break;
            case AGENT_BIND_MOBILE:
                dos_strncpy(pstSCB->szCalleeNum, stAgentInfo.szMobile, sizeof(pstSCB->szCalleeNum));
                pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
                break;
            case AGENT_BIND_TT_NUMBER:
                dos_strncpy(pstSCB->szCalleeNum, stAgentInfo.szTTNumber, sizeof(pstSCB->szCalleeNum));
                pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
                break;
        }

        sc_log_digest_print("Call agent(%u). caller : %s, callee : %s", ulCurrentAgent, pstSCB->szCallerNum, pstSCB->szCalleeNum);

        /* 呼叫坐席 */
        SC_SCB_SET_STATUS(pstSCB, SC_SCB_INIT);

        if (AGENT_BIND_SIP != stAgentInfo.ucBindType
            && AGENT_BIND_TT_NUMBER != stAgentInfo.ucBindType)
        {
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL);

            if (!sc_ep_black_list_check(pstSCB->ulCustomID, pstSCB->szCalleeNum))
            {
                DOS_ASSERT(0);
                sc_logr_info(pstSCB, SC_ESL, "Cannot make call for number %s which is in black list.", pstSCB->szCalleeNum);
                goto make_all_fail;
            }

            if (sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
            {
                sc_logr_notice(pstSCB, SC_ESL, "Send auth msg FAIL. SCB No: %d", pstSCB->usSCBNo);
                goto make_all_fail;
            }

            goto make_call_succ;
        }

        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_CALL);

        if (AGENT_BIND_SIP == stAgentInfo.ucBindType)
        {
            if (sc_dial_make_call2ip(pstSCB, SC_SERV_AGENT_CLICK_CALL, DOS_FALSE) != DOS_SUCC)
            {
                goto make_all_fail;
            }
        }
        else if (AGENT_BIND_TT_NUMBER == stAgentInfo.ucBindType)
        {
            if (sc_dial_make_call2eix(pstSCB, SC_SERV_AGENT_CLICK_CALL) != DOS_SUCC)
            {
                goto make_all_fail;
            }
        }
    }

make_call_succ:

    sc_logr_info(pstSCB, SC_ESL, "Request call agent %u(%u) SUCC", ulAgentCalled, ulCurrentAgent);

    return DOS_SUCC;

make_all_fail:
    if (DOS_ADDR_INVALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    sc_log_digest_print("Agent(%u) call other agent(%u) FAIL", ulCurrentAgent, ulAgentCalled);
    sc_logr_info(pstSCB, SC_ESL, "Request call agent %u(%u) FAIL", ulAgentCalled, ulCurrentAgent);

    return DOS_FAIL;
}

/* 呼叫特定号码 */
U32 sc_ep_call_ctrl_call_out(U32 ulAgent, U32 ulTaskID, S8 *pszNumber)
{
    SC_SCB_ST *pstSCB       = NULL;
    SC_SCB_ST *pstSCBAgent  = NULL;
    SC_SCB_ST *pstSCBOther  = NULL;
    S8        szParams[256] = { 0, };
    SC_ACD_AGENT_INFO_ST stAgentInfo;

    sc_logr_info(pstSCB, SC_ESL, "Call out request. Agent: %u, Task: %u, Number: %s"
            , ulAgent, ulTaskID, pszNumber ? pszNumber : "");
    sc_log_digest_print("Call out request. Agent: %u, Task: %u, Number: %s"
            , ulAgent, ulTaskID, pszNumber ? pszNumber : "");
    /* 查找坐席 */
    if (sc_acd_get_agent_by_id(&stAgentInfo, ulAgent) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot call agent with id %u. Agent not found.", ulAgent);
        goto make_all_fail;
    }

    /* 不需要判断坐席的状态 */
#if 0
    if (stAgentInfo.ucStatus  >=  SC_ACD_BUSY)
    {
        /* 不允许呼叫 */
        sc_logr_info(pstSCB, SC_ESL, "Call agnet FAIL. Agent (%u) status is (%d)", ulAgent, stAgentInfo.ucStatus);
        goto make_all_fail;
    }
#endif

    /* 如果坐席相关的业务控制块有管理到另外一个业务控制块，就不要再呼叫了 */
    pstSCBAgent = sc_scb_get(stAgentInfo.usSCBNo);
    if (DOS_ADDR_VALID(pstSCBAgent)
        && (pstSCBOther = sc_scb_get(pstSCBAgent->usOtherSCBNo)))
    {
        /* 不允许呼叫 */
        sc_logr_info(pstSCB, SC_ESL, "Call agnet FAIL. Agent %u (status:%d) has a call already", ulAgent, stAgentInfo.ucStatus);
        goto make_all_fail;
    }


    sc_acd_agent_stat(SC_AGENT_STAT_CALL, stAgentInfo.ulSiteID, NULL, 0);

    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        sc_logr_warning(pstSCB, SC_ESL, "%s", "Cannot make call for the API CMD. Alloc SCB FAIL.");

        goto make_all_fail;
    }

    dos_strncpy(pstSCB->szSiteNum, stAgentInfo.szEmpNo, sizeof(pstSCB->szSiteNum));
    pstSCB->szSiteNum[sizeof(pstSCB->szSiteNum) - 1] = '\0';

    if (stAgentInfo.bConnected && stAgentInfo.usSCBNo < SC_MAX_SCB_NUM)
    {
        /* 坐席长连 */
        pstSCBOther = sc_scb_get(stAgentInfo.usSCBNo);
        if (DOS_ADDR_INVALID(pstSCBOther))
        {
            sc_logr_warning(pstSCB, SC_ESL, "%s", "Cannot make find the scb for a signin agent.");

            goto make_all_fail;
        }

        /* 更新坐席的状态 */
        sc_acd_update_agent_info(pstSCBOther, SC_ACD_BUSY, pstSCBOther->usSCBNo, pszNumber);

        pstSCB->ulCustomID = stAgentInfo.ulCustomerID;
        pstSCB->ulAgentID = ulAgent;
        pstSCB->ulTaskID = ulTaskID;
        pstSCB->bRecord = stAgentInfo.bRecord;
        pstSCB->bTraceNo = stAgentInfo.bTraceON;
        pstSCB->usOtherSCBNo = pstSCBOther->usSCBNo;
        pstSCBOther->ulTaskID = ulTaskID;
        pstSCBOther->usOtherSCBNo = pstSCB->usSCBNo;

        dos_strncpy(pstSCBOther->szCustomerNum, pszNumber, sizeof(pstSCB->szCalleeNum));
        pstSCBOther->szCustomerNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';

        /* 指定被叫号码 */
        dos_strncpy(pstSCB->szCalleeNum, pszNumber, sizeof(pstSCB->szCalleeNum));
        pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';

        /* 后面会获取主叫号码，这里就不需要获取了，随便填个就行了 */
        dos_strncpy(pstSCB->szCallerNum, stAgentInfo.szUserID, sizeof(pstSCB->szCallerNum));
        pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) - 1] = '\0';

#if 0
        /* 使用号码组， 获得主叫号码 */
        if (sc_caller_setting_select_number(pstSCB->ulCustomID, ulAgent, SC_SRC_CALLER_TYPE_AGENT, pstSCB->szCallerNum, sizeof(pstSCB->szCallerNum)) != DOS_SUCC)
        {
            DOS_ASSERT(0);
            sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Get caller fail by agent(%u). ", ulAgent);

            goto make_all_fail;
        }
#endif

        if (!sc_ep_black_list_check(pstSCB->ulCustomID, pszNumber))
        {
            DOS_ASSERT(0);

            sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Callee in blocak list. (%s)", pszNumber);

            goto make_all_fail;
        }

        dos_snprintf(szParams, sizeof(szParams), "bgapi uuid_break %s", pstSCBOther->szUUID);
        sc_ep_esl_execute_cmd(szParams);

        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_PREVIEW_DIALING);

        if (sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
        {
            DOS_ASSERT(0);

            sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Send auth fail.", pszNumber);

            goto make_all_fail;
        }
    }
    else
    {
        pstSCB->enCallCtrlType = SC_CALL_CTRL_TYPE_OUT;
        pstSCB->ulCustomID = stAgentInfo.ulCustomerID;
        pstSCB->ulAgentID = ulAgent;
        pstSCB->ulTaskID = ulTaskID;
        pstSCB->bRecord = stAgentInfo.bRecord;
        pstSCB->bTraceNo = stAgentInfo.bTraceON;
        pstSCB->bIsAgentCall = DOS_TRUE;

        dos_strncpy(pstSCB->szCallerNum, pszNumber, sizeof(pstSCB->szCallerNum));
        pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) - 1] = '\0';

        dos_strncpy(pstSCB->szCustomerNum, pszNumber, sizeof(pstSCB->szCustomerNum));
        pstSCB->szCustomerNum[sizeof(pstSCB->szCustomerNum) - 1] = '\0';
        /* 指定被叫号码 */
        switch (stAgentInfo.ucBindType)
        {
            case AGENT_BIND_SIP:
                dos_strncpy(pstSCB->szCalleeNum, stAgentInfo.szUserID, sizeof(pstSCB->szCalleeNum));
                pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
                break;
            case AGENT_BIND_TELE:
                dos_strncpy(pstSCB->szCalleeNum, stAgentInfo.szTelePhone, sizeof(pstSCB->szCalleeNum));
                pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
                break;
            case AGENT_BIND_MOBILE:
                dos_strncpy(pstSCB->szCalleeNum, stAgentInfo.szMobile, sizeof(pstSCB->szCalleeNum));
                pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
                break;
            case AGENT_BIND_TT_NUMBER:
                dos_strncpy(pstSCB->szCalleeNum, stAgentInfo.szTTNumber, sizeof(pstSCB->szCalleeNum));
                pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
                break;

        }

        sc_logr_info(pstSCB, SC_ESL, "Call agent %u, bind type: %u, bind number: %s", stAgentInfo.ulSiteID, stAgentInfo.ucBindType, pstSCB->szCalleeNum);

        /* 呼叫坐席 */
        SC_SCB_SET_STATUS(pstSCB, SC_SCB_INIT);

        if (AGENT_BIND_SIP != stAgentInfo.ucBindType
            && AGENT_BIND_TT_NUMBER != stAgentInfo.ucBindType)
        {
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL);
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_AGENT_CLICK_CALL);

            if (!sc_ep_black_list_check(pstSCB->ulCustomID, pstSCB->szCalleeNum))
            {
                DOS_ASSERT(0);
                sc_logr_info(pstSCB, SC_ESL, "Cannot make call for number %s which is in black list.", pstSCB->szCalleeNum);
                goto make_all_fail;
            }

            if (sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
            {
                sc_logr_notice(pstSCB, SC_ESL, "Send auth msg FAIL. SCB No: %d", pstSCB->usSCBNo);
                goto make_all_fail;
            }

            goto make_all_succ;
        }

        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_AGENT_CLICK_CALL);

        if (AGENT_BIND_SIP == stAgentInfo.ucBindType)
        {
            if (sc_dial_make_call2ip(pstSCB, SC_SERV_AGENT_CLICK_CALL, DOS_FALSE) != DOS_SUCC)
            {
                goto make_all_fail;
            }
        }
        else if (AGENT_BIND_TT_NUMBER == stAgentInfo.ucBindType)
        {
            if (sc_dial_make_call2eix(pstSCB, SC_SERV_AGENT_CLICK_CALL) != DOS_SUCC)
            {
                goto make_all_fail;
            }
        }
    }

make_all_succ:
    /* 坐席弹屏 */
    sc_ep_call_notify(&stAgentInfo, pstSCB->szCallerNum);

    sc_logr_info(pstSCB, SC_ESL, "Call out request SUCC. Agent: %u, Task: %u, Number: %s"
            , ulAgent, ulTaskID, pszNumber ? pszNumber : "");

    return DOS_SUCC;

make_all_fail:
    if (SC_SCB_IS_VALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    sc_logr_info(pstSCB, SC_ESL, "Call out request FAIL. Agent: %u, Task: %u, Number: %s"
            , ulAgent, ulTaskID, pszNumber ? pszNumber : "");
    sc_log_digest_print("Call out request FAIL. Agent: %u, Task: %u, Number: %s"
            , ulAgent, ulTaskID, pszNumber ? pszNumber : "");

    return DOS_FAIL;
}

/* 坐席在web上呼叫一个分机
        已经验证过了，sip分机和agent属于同一个客户 */
U32 sc_ep_call_ctrl_call_sip(U32 ulAgent, S8 *pszSipNumber)
{
    SC_SCB_ST *pstSCB       = NULL;
    SC_SCB_ST *pstSCBOther  = NULL;
    SC_ACD_AGENT_INFO_ST stAgentInfo;
    S8        szParams[256] = { 0, };

    if (DOS_ADDR_INVALID(pszSipNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_logr_info(pstSCB, SC_ESL, "Request call sip %s(%u)", pszSipNumber, ulAgent);
    sc_log_digest_print("Request call sip %s(%u)", pszSipNumber, ulAgent);

    /* 查找坐席 */
    if (sc_acd_get_agent_by_id(&stAgentInfo, ulAgent) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot call agent with id %u. Agent not found.", ulAgent);
        goto make_all_fail;
    }

    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        sc_logr_warning(pstSCB, SC_ESL, "%s", "Cannot make call for the API CMD. Alloc SCB FAIL.");

        goto make_all_fail;
    }

    dos_strncpy(pstSCB->szSiteNum, stAgentInfo.szEmpNo, sizeof(pstSCB->szSiteNum));
    pstSCB->szSiteNum[sizeof(pstSCB->szSiteNum) - 1] = '\0';

    if (stAgentInfo.bConnected && stAgentInfo.usSCBNo < SC_MAX_SCB_NUM)
    {
        /* 坐席长连 */
        pstSCBOther = sc_scb_get(stAgentInfo.usSCBNo);
        if (DOS_ADDR_INVALID(pstSCBOther))
        {
            sc_logr_warning(pstSCB, SC_ESL, "%s", "Get SCB fail for signin agent.");
            goto make_all_fail;
        }

        /* 更新坐席的状态 */
        sc_acd_update_agent_info(pstSCBOther, SC_ACD_BUSY, pstSCBOther->usSCBNo, NULL);

        pstSCB->ulCustomID = stAgentInfo.ulCustomerID;
        pstSCB->ulAgentID = ulAgent;
        pstSCB->bRecord = stAgentInfo.bRecord;
        pstSCB->bTraceNo = stAgentInfo.bTraceON;
        pstSCB->usOtherSCBNo = pstSCBOther->usSCBNo;
        pstSCBOther->usOtherSCBNo = pstSCB->usSCBNo;

        /* 指定被叫号码 */
        dos_strncpy(pstSCB->szCalleeNum, pszSipNumber, sizeof(pstSCB->szCalleeNum));
        pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';

        dos_strncpy(pstSCB->szCallerNum, pstSCBOther->szSiteNum, sizeof(pstSCB->szCallerNum));
        pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) - 1] = '\0';

        dos_snprintf(szParams, sizeof(szParams), "bgapi uuid_break %s", pstSCBOther->szUUID);
        sc_ep_esl_execute_cmd(szParams);

        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_CALL);

        if (sc_dial_make_call2ip(pstSCB, SC_SERV_BUTT, DOS_FALSE) != DOS_SUCC)
        {
            goto make_all_fail;
        }
    }
    else
    {
        pstSCB->enCallCtrlType = SC_CALL_CTRL_TYPE_SIP;

        pstSCB->ulCustomID = stAgentInfo.ulCustomerID;
        pstSCB->ulAgentID  = ulAgent;
        pstSCB->ulTaskID   = U32_BUTT;
        pstSCB->bRecord    = stAgentInfo.bRecord;
        pstSCB->bTraceNo   = stAgentInfo.bTraceON;
        pstSCB->bIsAgentCall = DOS_TRUE;

        dos_strncpy(pstSCB->szCallerNum, pszSipNumber, sizeof(pstSCB->szCallerNum));
        pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) - 1] = '\0';

        /* 指定被叫号码 */
        switch (stAgentInfo.ucBindType)
        {
            case AGENT_BIND_SIP:
                dos_strncpy(pstSCB->szCalleeNum, stAgentInfo.szUserID, sizeof(pstSCB->szCalleeNum));
                pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
                break;
            case AGENT_BIND_TELE:
                dos_strncpy(pstSCB->szCalleeNum, stAgentInfo.szTelePhone, sizeof(pstSCB->szCalleeNum));
                pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
                break;
            case AGENT_BIND_MOBILE:
                dos_strncpy(pstSCB->szCalleeNum, stAgentInfo.szMobile, sizeof(pstSCB->szCalleeNum));
                pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
                break;
            case AGENT_BIND_TT_NUMBER:
                dos_strncpy(pstSCB->szCalleeNum, stAgentInfo.szTTNumber, sizeof(pstSCB->szCalleeNum));
                pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';
                break;
        }

        /* 呼叫坐席 */
        SC_SCB_SET_STATUS(pstSCB, SC_SCB_INIT);

        if (AGENT_BIND_SIP != stAgentInfo.ucBindType
            && AGENT_BIND_TT_NUMBER != stAgentInfo.ucBindType)
        {
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL);
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_AGENT_CLICK_CALL);

            if (!sc_ep_black_list_check(pstSCB->ulCustomID, pstSCB->szCalleeNum))
            {
                DOS_ASSERT(0);
                sc_logr_info(pstSCB, SC_ESL, "Cannot make call for number %s which is in black list.", pstSCB->szCalleeNum);
                goto make_all_fail;
            }

            if (sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
            {
                sc_logr_notice(pstSCB, SC_ESL, "Send auth msg FAIL. SCB No: %d", pstSCB->usSCBNo);
                goto make_all_fail;
            }

            goto make_call_succ;
        }

        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_AGENT_CLICK_CALL);

        if (AGENT_BIND_SIP == stAgentInfo.ucBindType)
        {
            if (sc_dial_make_call2ip(pstSCB, SC_SERV_AGENT_CLICK_CALL, DOS_FALSE) != DOS_SUCC)
            {
                goto make_all_fail;
            }
        }
        else if (AGENT_BIND_TT_NUMBER == stAgentInfo.ucBindType)
        {
            if (sc_dial_make_call2eix(pstSCB, SC_SERV_AGENT_CLICK_CALL) != DOS_SUCC)
            {
                goto make_all_fail;
            }
        }
    }

make_call_succ:

    sc_logr_info(pstSCB, SC_ESL, "Request call sip %s(%u) SUCC", pszSipNumber, ulAgent);

    return DOS_SUCC;

make_all_fail:
    if (DOS_ADDR_INVALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    sc_logr_info(pstSCB, SC_ESL, "Request call sip %s(%u) FAIL", pszSipNumber, ulAgent);
    sc_log_digest_print("Request call sip %s(%u) FAIL", pszSipNumber, ulAgent);

    return DOS_FAIL;
}

/* 给坐席录音 */
U32 sc_ep_call_ctrl_record(U32 ulAgent)
{
    SC_SCB_ST *pstSCB       = NULL;
    SC_ACD_AGENT_INFO_ST stAgentInfo;

    /* 查找坐席 */
    if (sc_acd_get_agent_by_id(&stAgentInfo, ulAgent) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent not found..", ulAgent);
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

    sc_logr_notice(pstSCB, SC_ESL, "Request record agent %u SUCC", ulAgent);
    return DOS_FAIL;

proc_fail:
    sc_logr_notice(pstSCB, SC_ESL, "Request record agent %u FAIL", ulAgent);
    return DOS_FAIL;
}

/* 给坐席方耳语 */
U32 sc_ep_call_ctrl_whispers(U32 ulAgent, S8 *pszCallee)
{
    return DOS_FAIL;
}

/* 监听坐席 */
U32 sc_ep_call_ctrl_intercept(U32 ulAgent, S8 *pszCallee)
{
    return DOS_FAIL;
}

/* 坐席发起呼叫转接 */
U32 sc_ep_call_ctrl_transfer(U32 ulAgent, U32 ulAgentCalled, BOOL bIsAttend)
{
    SC_SCB_ST *pstSCB       = NULL;
    SC_ACD_AGENT_INFO_ST stAgentInfo, stCalleeAgentInfo;
    S8  szCallerNumber[SC_TEL_NUMBER_LENGTH] = { 0 };

    sc_logr_notice(pstSCB, SC_ESL, "Request transfer to %u (%u)", ulAgentCalled, ulAgent);
    sc_log_digest_print("Request transfer to %u (%u)", ulAgentCalled, ulAgent);

    /* 查找坐席 */
    if (sc_acd_get_agent_by_id(&stAgentInfo, ulAgent) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent not found..", ulAgent);
        goto proc_fail;
    }

    /* 如果呼叫的坐席 */
    if (sc_acd_get_agent_by_id(&stCalleeAgentInfo, ulAgentCalled) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Call agent(%u) FAIL. Agent called not found.", ulAgentCalled);
        goto proc_fail;
    }

    /* 判断坐席的状态 */
    if (!SC_ACD_SITE_IS_USEABLE(&stCalleeAgentInfo))
    {
        sc_logr_debug(pstSCB, SC_ACD, "The agent is not useable.(Agent %u)", stCalleeAgentInfo.ulSiteID);

        goto proc_fail;
    }
#if 0
    /* 对端在长签状态，不要打扰 */
    if (stCalleeAgentInfo.bConnected)
    {
        sc_logr_debug(pstSCB, SC_ACD, "The agent is not useable.(Agent %u)", stCalleeAgentInfo.ulSiteID);

        goto proc_fail;
    }
#endif

    /* 当前坐席需要有自己的业务控制块 */
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

    /* 获取被叫号码 */
    switch (stCalleeAgentInfo.ucBindType)
    {
        case AGENT_BIND_SIP:
            dos_strncpy(szCallerNumber, stCalleeAgentInfo.szUserID, SC_TEL_NUMBER_LENGTH);
            szCallerNumber[SC_TEL_NUMBER_LENGTH - 1] = '\0';
            break;

        case AGENT_BIND_TELE:
            dos_strncpy(szCallerNumber, stCalleeAgentInfo.szTelePhone, SC_TEL_NUMBER_LENGTH);
            szCallerNumber[SC_TEL_NUMBER_LENGTH - 1] = '\0';
            break;

        case AGENT_BIND_MOBILE:
            dos_strncpy(szCallerNumber, stCalleeAgentInfo.szMobile, SC_TEL_NUMBER_LENGTH);
            szCallerNumber[SC_TEL_NUMBER_LENGTH - 1] = '\0';
            break;

        case AGENT_BIND_TT_NUMBER:
            dos_strncpy(szCallerNumber, stCalleeAgentInfo.szTTNumber, SC_TEL_NUMBER_LENGTH);
            szCallerNumber[SC_TEL_NUMBER_LENGTH - 1] = '\0';
            break;

        default:
            goto proc_fail;
    }

    if (sc_ep_call_transfer(pstSCB, stCalleeAgentInfo.ulSiteID, stCalleeAgentInfo.ucBindType, szCallerNumber, bIsAttend) != DOS_SUCC)
    {
        goto proc_fail;
    }

    sc_logr_notice(pstSCB, SC_ESL, "Request transfer to %u SUCC. %u", ulAgentCalled, ulAgent);
    return DOS_SUCC;

proc_fail:

    if (DOS_ADDR_VALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    sc_logr_notice(pstSCB, SC_ESL, "Request transfer to %u FAIL. %u", ulAgentCalled, ulAgent);
    sc_log_digest_print("Request transfer to %u FAIL. %u", ulAgentCalled, ulAgent);

    return DOS_FAIL;
}

/* 坐席发起电话会议 */
U32 sc_ep_call_ctrl_conference(U32 ulAgent, S8 *pszCallee, BOOL blBlind)
{
    return DOS_FAIL;
}


U32 sc_ep_call_ctrl_proc(U32 ulAction, U32 ulTaskID, U32 ulAgent, U32 ulCustomerID, U32 ulType, S8 *pszCallee, U32 ulFlag, U32 ulCalleeAgentID)
{
    SC_SCB_ST *pstSCB       = NULL;
    SC_SCB_ST *pstSCBNew    = NULL;
    SC_ACD_AGENT_INFO_ST stAgentInfo;
    U32       ulMainServie;

    if (ulAction >= SC_API_CALLCTRL_BUTT)
    {
        DOS_ASSERT(0);

        goto proc_fail;
    }

    sc_logr_info(pstSCB, SC_ESL, "Start process call ctrl msg. Action: %u, Agent: %u, Customer: %u, Task: %u, Caller: %s"
                    , ulAction, ulAgent, ulCustomerID, ulTaskID, pszCallee ? pszCallee : "");

    switch (ulAction)
    {
        case SC_API_MAKE_CALL:
        case SC_API_TRANSFOR_ATTAND:
        case SC_API_TRANSFOR_BLIND:
        case SC_API_CONFERENCE:
        case SC_API_HOLD:
        case SC_API_UNHOLD:
            break;

        case SC_API_HANGUP_CALL:
            return sc_ep_call_ctrl_hangup_all(ulAgent);
            break;

        case SC_API_RECORD:
            /* 查找坐席 */
            if (sc_acd_get_agent_by_id(&stAgentInfo, ulAgent) != DOS_SUCC)
            {
                DOS_ASSERT(0);

                sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent not found..", ulAgent);
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
            break;

        case SC_API_WHISPERS:
        case SC_API_INTERCEPT:
            /* 查找坐席 */
            if (sc_acd_get_agent_by_id(&stAgentInfo, ulAgent) != DOS_SUCC)
            {
                DOS_ASSERT(0);

                sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent not found..", ulAgent);
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

            pstSCBNew = sc_scb_alloc();
            if (DOS_ADDR_INVALID(pstSCBNew))
            {
                sc_logr_warning(pstSCB, SC_ESL, "%s", "Cannot make call for the API CMD. Alloc SCB FAIL..");
            }

            pstSCBNew->ulCustomID = ulCustomerID;
            pstSCBNew->ulAgentID = ulAgent;
            pstSCBNew->ulTaskID = ulTaskID;

            /* 需要指定主叫号码 */
            dos_strncpy(pstSCBNew->szCalleeNum, pszCallee, sizeof(pstSCBNew->szCalleeNum));
            pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';
            dos_strncpy(pstSCBNew->szCallerNum, pszCallee, sizeof(pstSCBNew->szCallerNum));
            pstSCBNew->szCallerNum[sizeof(pstSCBNew->szCallerNum) - 1] = '\0';

            if (SC_API_WHISPERS == ulAction)
            {
                ulMainServie = SC_SERV_CALL_WHISPERS;
            }
            else
            {
                ulMainServie = SC_SERV_CALL_INTERCEPT;
            }

            SC_SCB_SET_SERVICE(pstSCBNew, ulMainServie);

            if (!sc_ep_black_list_check(ulCustomerID, pszCallee))
            {
                DOS_ASSERT(0);

                sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Callee in blocak list. (%s)", pszCallee);

                goto make_all_fail1;
            }

            //if (ulCustomerID == sc_ep_get_custom_by_sip_userid(pszCallee)
            //    || sc_ep_check_extension(pszCallee, ulCustomerID))
            if (AGENT_BIND_SIP == ulType)
            {
                /* 呼叫的是sip */
                SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
                SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_INTERNAL_CALL);

                if (sc_dial_make_call2ip(pstSCBNew, ulMainServie, DOS_TRUE) != DOS_SUCC)
                {
                    DOS_ASSERT(0);

                    sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Make call to other endpoint fail. callee : %s, type :%u", pszCallee, ulType);
                    goto make_all_fail1;
                }
            }
            else if (AGENT_BIND_TT_NUMBER == ulType)
            {
                if (sc_dial_make_call2eix(pstSCBNew, SC_SERV_AGENT_CALLBACK) != DOS_SUCC)
                {
                    sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Make call to other endpoint fail. callee : %s, type :%u", pszCallee, ulType);
                    goto make_all_fail1;
                }
            }
            else
            {
                SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
                SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_EXTERNAL_CALL);
                if (sc_send_usr_auth2bs(pstSCBNew) != DOS_SUCC)
                {
                    DOS_ASSERT(0);

                    sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Send auth fail.", pszCallee);

                    goto make_all_fail1;
                }
            }

            break;
make_all_fail1:
            if (DOS_ADDR_INVALID(pstSCBNew))
            {
                sc_scb_free(pstSCBNew);
                pstSCBNew = NULL;
            }

            goto proc_fail;
            break;
        default:
            goto proc_fail;
    }

    sc_logr_info(pstSCB, SC_ESL, "Finished to process call ctrl msg. Action: %u, Agent: %u, Customer: %u, Task: %u, Caller: %s"
                    , ulAction, ulAgent, ulCustomerID, ulTaskID, pszCallee);

    return DOS_SUCC;

proc_fail:
    sc_logr_info(pstSCB, SC_ESL, "Process call ctrl msg FAIL. Action: %u, Agent: %u, Customer: %u, Task: %u, Caller: %s"
                    , ulAction, ulAgent, ulCustomerID, ulTaskID, pszCallee);

    return DOS_FAIL;
}

/* 群呼任务的 demo */
U32 sc_demo_task(U32 ulCustomerID, S8 *pszCallee, S8 *pszAgentNum, U32 ulAgentID)
{
    SC_SCB_ST   *pstSCB    = NULL;

    if (DOS_ADDR_INVALID(pszCallee)
        || DOS_ADDR_INVALID(pszAgentNum))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_log_digest_print("Start task demo. customer : %u", ulCustomerID);

    /* 呼叫客户 */
    pstSCB = sc_scb_alloc();
    if (!pstSCB)
    {
        sc_logr_notice(pstSCB, SC_TASK, "Make call for demo task fail. Alloc SCB fail. customer : %u", ulCustomerID);
        goto FAIL;
    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_INIT);

    pstSCB->ulCustomID = ulCustomerID;
    pstSCB->usSiteNo = U16_BUTT;
    pstSCB->ulTrunkID = U32_BUTT;
    pstSCB->ulCallDuration = 0;
    pstSCB->ucLegRole = SC_CALLER;
    pstSCB->ulAgentID = ulAgentID;

    dos_strncpy(pstSCB->szCallerNum, pszAgentNum, SC_TEL_NUMBER_LENGTH);
    pstSCB->szCallerNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';
    dos_strncpy(pstSCB->szCalleeNum, pszCallee, SC_TEL_NUMBER_LENGTH);
    pstSCB->szCalleeNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';
    pstSCB->szSiteNum[0] = '\0';
    pstSCB->szUUID[0] = '\0';

    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_DEMO_TASK);
    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL);

    if (!sc_ep_black_list_check(pstSCB->ulCustomID, pstSCB->szCalleeNum))
    {
        DOS_ASSERT(0);
        sc_logr_info(pstSCB, SC_ESL, "Cannot make call for number %s which is in black list.", pstSCB->szCalleeNum);
        goto FAIL;
    }

    if (sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
    {
        sc_call_trace(pstSCB, "Make call failed.");

        //sc_task_callee_set_recall(pstTCB, pstCallee->ulIndex);
        goto FAIL;
    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_AUTH);

    sc_call_trace(pstSCB, "Make call for demo task successfully. customer : %u", ulCustomerID);

    return DOS_SUCC;

FAIL:

    sc_log_digest_print("Start task demo FAIL.");
    if (DOS_ADDR_VALID(pstSCB))
    {
        DOS_ASSERT(0);
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    return DOS_FAIL;
}

/* 预览外呼的 demo */
U32 sc_demo_preview(U32 ulCustomerID, S8 *pszCallee, S8 *pszAgentNum, U32 ulAgentID)
{
    SC_SCB_ST *pstSCB       = NULL;
    SC_ACD_AGENT_INFO_ST stAgentInfo;

    sc_logr_info(pstSCB, SC_ESL, "Call out request. Agent: %u, Number: %s"
            , ulAgentID, pszCallee ? pszCallee : "");

    sc_log_digest_print("Start preview demo. customer : %u", ulCustomerID);
    /* 查找坐席 */
    if (sc_acd_get_agent_by_id(&stAgentInfo, ulAgentID) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot call agent with id %u. Agent not found.", ulAgentID);
        goto PROC_FAIL;
    }

    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        sc_logr_warning(pstSCB, SC_ESL, "%s", "Cannot make call for the API CMD. Alloc SCB FAIL.");

        goto PROC_FAIL;
    }

    dos_strncpy(pstSCB->szSiteNum, stAgentInfo.szEmpNo, sizeof(pstSCB->szSiteNum));
    pstSCB->szSiteNum[sizeof(pstSCB->szSiteNum) - 1] = '\0';

    pstSCB->enCallCtrlType = SC_CALL_CTRL_TYPE_OUT;
    pstSCB->ulCustomID = stAgentInfo.ulCustomerID;
    pstSCB->ulAgentID = ulAgentID;
    pstSCB->ulTaskID = 0;
    pstSCB->bRecord = stAgentInfo.bRecord;
    pstSCB->bTraceNo = stAgentInfo.bTraceON;
    pstSCB->bIsAgentCall = DOS_TRUE;

    dos_strncpy(pstSCB->szCallerNum, pszCallee, sizeof(pstSCB->szCallerNum));
    pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) - 1] = '\0';

    dos_strncpy(pstSCB->szCustomerNum, pszCallee, sizeof(pstSCB->szCustomerNum));
    pstSCB->szCustomerNum[sizeof(pstSCB->szCustomerNum) - 1] = '\0';

    /* 被叫号码 */
    dos_strncpy(pstSCB->szCalleeNum, pszAgentNum, sizeof(pstSCB->szCalleeNum));
    pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) - 1] = '\0';

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_INIT);

    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL);
    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_AGENT_CLICK_CALL);

    if (!sc_ep_black_list_check(pstSCB->ulCustomID, pstSCB->szCalleeNum))
    {
        DOS_ASSERT(0);
        sc_logr_info(pstSCB, SC_ESL, "Cannot make call for number %s which is in black list.", pstSCB->szCalleeNum);
        goto PROC_FAIL;
    }

    if (sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
    {
        sc_logr_notice(pstSCB, SC_ESL, "Send auth msg FAIL. SCB No: %d", pstSCB->usSCBNo);
        goto PROC_FAIL;
    }

    /* 坐席弹屏 */
    sc_ep_call_notify(&stAgentInfo, pstSCB->szCallerNum);

    sc_logr_info(pstSCB, SC_ESL, "Call out request SUCC. Agent: %u, Number: %s"
            , ulAgentID, pszCallee ? pszCallee : "");

    return DOS_SUCC;

PROC_FAIL:
    if (SC_SCB_IS_VALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    sc_log_digest_print("Start preview demo FAIL. customer : %u", ulCustomerID);

    sc_logr_info(pstSCB, SC_ESL, "Call out request FAIL. Agent: %u, Number: %s"
            , ulAgentID, pszCallee ? pszCallee : "");

    return DOS_FAIL;
}

U32 sc_ep_demo_task_call_agent(SC_SCB_ST *pstSCB)
{
    U32           ulErrCode  = CC_ERR_NO_REASON;
    SC_SCB_ST     *pstSCBNew = NULL;
    SC_ACD_AGENT_INFO_ST stAgentInfo;

    pstSCBNew = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCBNew))
    {
        DOS_ASSERT(0);

        sc_logr_error(pstSCB, SC_ESL, "%s", "Allc SCB FAIL.");
        ulErrCode = CC_ERR_SC_CB_ALLOC_FAIL;
        goto proc_error;
    }

    /* 查找坐席 */
    if (sc_acd_get_agent_by_id(&stAgentInfo, pstSCB->ulAgentID) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        ulErrCode = CC_ERR_SC_USER_DOES_NOT_EXIST;
        sc_logr_info(pstSCB, SC_ESL, "Cannot call agent with id %u. Agent not found, Errcode : %u", pstSCB->ulAgentID, ulErrCode);
        goto proc_error;
    }

    pstSCBNew->bIsAgentCall = DOS_TRUE;
    pstSCB->usOtherSCBNo = pstSCBNew->usSCBNo;
    pstSCBNew->ulCustomID = pstSCB->ulCustomID;
    pstSCBNew->ulAgentID = stAgentInfo.ulSiteID;
    pstSCBNew->usOtherSCBNo = pstSCB->usSCBNo;
    pstSCBNew->ucLegRole = SC_CALLEE;
    pstSCBNew->bRecord = stAgentInfo.bRecord;
    pstSCBNew->bTraceNo = stAgentInfo.bTraceON;

    /* 如果为群呼任务，则customer num 为被叫号码，如果是呼入，则是主叫号码 */
    dos_strncpy(pstSCBNew->szCallerNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCallerNum));
    pstSCBNew->szCallerNum[sizeof(pstSCBNew->szCallerNum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szANINum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szANINum));
    pstSCBNew->szANINum[sizeof(pstSCBNew->szANINum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szCalleeNum, pstSCB->szCallerNum, sizeof(pstSCBNew->szCalleeNum));
    pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';

    dos_strncpy(pstSCBNew->szCustomerNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCustomerNum));
    pstSCBNew->szCustomerNum[sizeof(pstSCBNew->szCustomerNum) - 1] = '\0';

    dos_strncpy(pstSCBNew->szSiteNum, stAgentInfo.szEmpNo, sizeof(pstSCBNew->szSiteNum));
    pstSCBNew->szSiteNum[sizeof(pstSCBNew->szSiteNum) - 1] = '\0';

    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_AGENT_CALLBACK);
    SC_SCB_SET_STATUS(pstSCBNew, SC_SCB_IDEL);

    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_EXTERNAL_CALL);

    if (!sc_ep_black_list_check(pstSCBNew->ulCustomID, pstSCBNew->szCalleeNum))
    {
        DOS_ASSERT(0);
        sc_logr_info(pstSCB, SC_ESL, "Cannot make call for number %s which is in black list.", pstSCBNew->szCalleeNum);
        ulErrCode = CC_ERR_SC_IN_BLACKLIST;
        goto proc_error;
    }

    if (sc_send_usr_auth2bs(pstSCBNew) != DOS_SUCC)
    {
        sc_logr_notice(pstSCB, SC_ESL, "Send auth msg FAIL. SCB No: %d", pstSCBNew->usSCBNo);
        ulErrCode = CC_ERR_SC_MESSAGE_SENT_ERR;
        goto proc_error;
    }

    /* 不是坐席呼叫坐席时，才弹屏 */
    if (sc_ep_call_notify(&stAgentInfo, pstSCBNew->szCustomerNum) != DOS_SUCC)
    {
        DOS_ASSERT(0);
    }

    return DOS_SUCC;

proc_error:
    if (pstSCBNew)
    {
        DOS_ASSERT(0);
        sc_scb_free(pstSCBNew);
        pstSCBNew = NULL;
    }
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
U32 sc_ep_incoming_call_proc(SC_SCB_ST *pstSCB)
{
    U32   ulCustomerID = U32_BUTT;
    U32   ulBindType = U32_BUTT;
    U32   ulBindID = U32_BUTT;
    S8    szCallString[512] = { 0, };
    S8    szAPPParam[512] = { 0, };
    S8    szCallee[32] = { 0, };
    U32   ulErrCode = CC_ERR_NO_REASON;
    SC_ACD_AGENT_INFO_ST stAgentInfo;
    SC_SCB_ST *pstSCBNew = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        goto proc_fail;
    }
    /* 之前已经获得了 */
    //ulCustomerID = sc_ep_get_custom_by_did(pstSCB->szCalleeNum);
    sc_log_digest_print("Start proc incoming call. caller : %s, callee : %s", pstSCB->szCallerNum, pstSCB->szCalleeNum);
    ulCustomerID = pstSCB->ulCustomID;
    if (U32_BUTT != ulCustomerID)
    {
        if (sc_ep_get_bind_info4did(pstSCB->szCalleeNum, &ulBindType, &ulBindID) != DOS_SUCC
            || ulBindType >=SC_DID_BIND_TYPE_BUTT
            || U32_BUTT == ulBindID)
        {
            DOS_ASSERT(0);

            sc_logr_info(pstSCB, SC_ESL, "Cannot get the bind info for the DID number %s, Reject Call.", pstSCB->szCalleeNum);
            ulErrCode = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;
            goto proc_fail;
        }

        sc_logr_info(pstSCB, SC_ESL, "Process Incoming Call, DID Number %s. Bind Type: %u, Bind ID: %u"
                        , pstSCB->szCalleeNum
                        , ulBindType
                        , ulBindID);

        switch (ulBindType)
        {
            case SC_DID_BIND_TYPE_SIP:
                /* 绑定sip分机时，不判断坐席的状态, 不改变坐席的状态, 只是判断一下是否需要录音 */
                if (DOS_SUCC != sc_ep_get_userid_by_id(ulBindID, szCallee, sizeof(szCallee)))
                {
                    DOS_ASSERT(0);

                    sc_logr_info(pstSCB, SC_ESL, "DID number %s seems donot bind a SIP User ID, Reject Call.", pstSCB->szCalleeNum);
                    ulErrCode = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;
                    goto proc_fail;
                }

                sc_logr_info(pstSCB, SC_ESL, "Start find agent by userid(%s)", szCallee);
                if (sc_acd_get_agent_by_userid(&stAgentInfo, szCallee) != DOS_SUCC)
                {
                    /* 没有找到绑定的坐席 */
                    dos_snprintf(szCallString, sizeof(szCallString), "{other_leg_scb=%d,exec_after_bridge_app=park,mark_customer=true}user/%s", pstSCB->usSCBNo, szCallee);

                    sc_ep_esl_execute("bridge", szCallString, pstSCB->szUUID);
                    break;
                }
                sc_logr_info(pstSCB, SC_ESL, "Find agent(%d) succ. bConnected : %d", stAgentInfo.ulSiteID, stAgentInfo.bConnected);

                pstSCB->bRecord = stAgentInfo.bRecord;
                pstSCB->bTraceNo = stAgentInfo.bTraceON;
                pstSCB->ulAgentID = stAgentInfo.ulSiteID;
                dos_strncpy(pstSCB->szSiteNum, stAgentInfo.szEmpNo, sizeof(pstSCB->szSiteNum));
                pstSCB->szSiteNum[sizeof(pstSCB->szSiteNum) - 1] = '\0';
                if (pstSCB->bRecord)
                {
                    /* 录音 */
                    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_RECORDING);
                }

                /* 坐席弹屏 */
                sc_ep_call_notify(&stAgentInfo, pstSCB->szCallerNum);

                /* 坐席长签，且坐席绑定的是分机，可以直接 uuid_bridge */
                if (stAgentInfo.bConnected
                    && stAgentInfo.usSCBNo < SC_MAX_SCB_NUM
                    && stAgentInfo.ucBindType == AGENT_BIND_SIP)
                {
                    /* 坐席长签 */
                    pstSCBNew = sc_scb_get(stAgentInfo.usSCBNo);
                    if (DOS_ADDR_INVALID(pstSCBNew))
                    {
                        DOS_ASSERT(0);
                        goto proc_fail;
                    }

                    if ('\0' == pstSCBNew->szUUID[0])
                    {
                        DOS_ASSERT(0);
                        goto proc_fail;
                    }
                    pstSCBNew->bCallSip = DOS_TRUE;

                    dos_strncpy(pstSCBNew->szCustomerNum, pstSCB->szCallerNum, SC_TEL_NUMBER_LENGTH);
                    pstSCBNew->szCustomerNum[SC_TEL_NUMBER_LENGTH-1] = '\0';

                    pstSCBNew->usOtherSCBNo = pstSCB->usSCBNo;
                    pstSCB->usOtherSCBNo = pstSCBNew->usSCBNo;

                    sc_ep_esl_execute("answer", NULL, pstSCB->szUUID);

                    /* 给通道设置变量 */
                    dos_snprintf(szAPPParam, sizeof(szAPPParam), "bgapi uuid_break %s all\r\n", pstSCBNew->szUUID);
                    sc_ep_esl_execute_cmd(szAPPParam);
                    sc_ep_esl_execute("break", NULL, pstSCBNew->szUUID);
                    sc_ep_esl_execute("set", "hangup_after_bridge=false", pstSCBNew->szUUID);

                    dos_snprintf(szCallString, sizeof(szCallString), "bgapi uuid_bridge %s %s \r\n", pstSCB->szUUID, pstSCBNew->szUUID);
                    if (sc_ep_esl_execute_cmd(szCallString) != DOS_SUCC)
                    {
                        DOS_ASSERT(0);
                        goto proc_fail;
                    }
                }
                else
                {
                    dos_snprintf(szCallString, sizeof(szCallString), "{other_leg_scb=%d,update_agent_id=0,origination_caller_id_number=%s,origination_caller_id_name=%s,exec_after_bridge_app=park,mark_customer=true,customer_num=%s}user/%s"
                        , pstSCB->usSCBNo
                        , pstSCB->szCallerNum
                        , pstSCB->szCallerNum
                        , pstSCB->szCallerNum
                        , szCallee);

                    sc_ep_esl_execute("bridge", szCallString, pstSCB->szUUID);
                }

                break;

            case SC_DID_BIND_TYPE_QUEUE:
                sc_ep_esl_execute("answer", NULL, pstSCB->szUUID);
                if (sc_ep_call_queue_add(pstSCB, ulBindID, DOS_TRUE) != DOS_SUCC)
                {
                    DOS_ASSERT(0);

                    sc_logr_info(pstSCB, SC_ESL, "Add Call to call waiting queue FAIL.Callee: %s. Reject Call.", pstSCB->szCalleeNum);
                    ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                    goto proc_fail;
                }
                break;
            case SC_DID_BIND_TYPE_AGENT:
                /* 呼叫坐席 */
                if (sc_ep_call_agent_by_id(pstSCB, ulBindID, DOS_FALSE, DOS_TRUE) != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                    sc_logr_info(pstSCB, SC_ESL, "Call to agent(%u) FAIL.Callee: %s. Reject Call.", ulBindID, pstSCB->szCalleeNum);
                    /* 失败后，已经挂断，不需要再挂断 */
                    return DOS_FAIL;
                }
                break;

            default:
                DOS_ASSERT(0);
                ulErrCode = CC_ERR_SC_CONFIG_ERR;
                sc_logr_info(pstSCB, SC_ESL, "DID number %s has bind an error number, Reject Call.", pstSCB->szCalleeNum);
                goto proc_fail;
        }
    }
    else
    {
        DOS_ASSERT(0);
        ulErrCode = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;
        sc_logr_info(pstSCB, SC_ESL, "Destination is not a DID number, Reject Call. Destination: %s", pstSCB->szCalleeNum);
        goto proc_fail;

    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_EXEC);

    return DOS_SUCC;

proc_fail:
    if (pstSCB)
    {
        sc_ep_hangup_call_with_snd(pstSCB, ulErrCode);
    }

    sc_log_digest_print("Start proc incoming call FAIL.");

    return DOS_FAIL;
}

/**
 * 函数: U32 sc_ep_outgoing_call_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理由SIP呼入到PSTN侧的呼叫
 * 参数:
 *      esl_handle_t *pstHandle : 发送数据的handle
 *      esl_event_t *pstEvent   : ESL 事件
 *      SC_SCB_ST *pstSCB       : 业务控制块
 * 返回值: 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_ep_outgoing_call_proc(SC_SCB_ST *pstSCB)
{
    SC_SCB_ST *pstSCBNew  = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        goto proc_fail;
    }

    sc_log_digest_print("Start proc outgoing call. caller : %s, callee : %s", pstSCB->szCallerNum, pstSCB->szCalleeNum);
    pstSCBNew = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCBNew))
    {
        DOS_ASSERT(0);

        sc_logr_error(pstSCB, SC_ESL, "Alloc SCB FAIL, %s:%d", __FUNCTION__, __LINE__);

        goto proc_fail;
    }

    pthread_mutex_lock(&pstSCBNew->mutexSCBLock);
    pstSCB->usOtherSCBNo = pstSCBNew->usSCBNo;
    pstSCBNew->ulCustomID = pstSCB->ulCustomID;
    pstSCBNew->usOtherSCBNo = pstSCB->usSCBNo;
    pstSCBNew->ucLegRole = SC_CALLEE;

    /* @todo 主叫号码应该使用客户的DID号码 */
    dos_strncpy(pstSCBNew->szCallerNum, pstSCB->szCallerNum, sizeof(pstSCBNew->szCallerNum));
    pstSCBNew->szCallerNum[sizeof(pstSCBNew->szCallerNum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szCalleeNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCalleeNum));
    pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szANINum, pstSCB->szCallerNum, sizeof(pstSCBNew->szANINum));
    pstSCBNew->szANINum[sizeof(pstSCBNew->szANINum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szSiteNum, pstSCB->szSiteNum, sizeof(pstSCBNew->szSiteNum));
    pstSCBNew->szSiteNum[sizeof(pstSCBNew->szSiteNum) - 1] = '\0';

    pthread_mutex_unlock(&pstSCBNew->mutexSCBLock);
    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_EXTERNAL_CALL);
    SC_SCB_SET_STATUS(pstSCBNew, SC_SCB_INIT);

    if (!sc_ep_black_list_check(pstSCBNew->ulCustomID, pstSCBNew->szCalleeNum))
    {
        DOS_ASSERT(0);
        sc_logr_info(pstSCB, SC_ESL, "Cannot make call for number %s which is in black list.", pstSCBNew->szCalleeNum);
        goto proc_fail;
    }

    if (sc_send_usr_auth2bs(pstSCBNew) != DOS_SUCC)
    {
        sc_logr_notice(pstSCB, SC_ESL, "Send auth msg FAIL. SCB No: %d", pstSCBNew->usSCBNo);

        goto proc_fail;
    }

    return DOS_SUCC;

proc_fail:
    sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_MESSAGE_SENT_ERR);
    sc_log_digest_print("Proc outgoing call FAIL. ");

    if (DOS_ADDR_VALID(pstSCBNew))
    {
        DOS_ASSERT(0);
        sc_scb_free(pstSCBNew);
    }

    return DOS_FAIL;
}

/**
 * 函数: U32 sc_ep_auto_dial_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理由系统自动发起的呼叫
 * 参数:
 *      esl_handle_t *pstHandle : 发送数据的handle
 *      esl_event_t *pstEvent   : ESL 事件
 *      SC_SCB_ST *pstSCB       : 业务控制块
 * 返回值: 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_ep_auto_dial_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8      szAPPParam[512]    = { 0, };
    U32     ulTaskMode         = U32_BUTT;
    U32     ulErrCode          = CC_ERR_NO_REASON;

    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        goto auto_call_proc_error;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    if (!sc_call_check_service(pstSCB, SC_SERV_AUTO_DIALING))
    {
        DOS_ASSERT(0);

        sc_logr_debug(pstSCB, SC_ESL, "Process event %s finished. SCB do not include service auto call."
                            , esl_event_get_header(pstEvent, "Event-Name"));

        ulErrCode = CC_ERR_SC_NO_SERV_RIGHTS;
        goto auto_call_proc_error;
    }

    ulTaskMode = sc_task_get_mode(pstSCB->usTCBNo);
    if (ulTaskMode >= SC_TASK_MODE_BUTT)
    {
        DOS_ASSERT(0);

        sc_logr_debug(pstSCB, SC_ESL, "Process event %s finished. Cannot get the task mode for task %d."
                            , esl_event_get_header(pstEvent, "Event-Name")
                            , pstSCB->usTCBNo);

        ulErrCode = CC_ERR_SC_CONFIG_ERR;
        goto auto_call_proc_error;
    }

    /* 自动外呼需要处理 */
    /* 1.AOTO CALL走到这里客户那段已经接通了。这里根据所属任务的类型，做相应动作就好 */
    switch (ulTaskMode)
    {
        /* 需要放音的，统一先放音。在放音结束后请处理后续流程 */
        case SC_TASK_MODE_KEY4AGENT:
        case SC_TASK_MODE_KEY4AGENT1:
            sc_ep_esl_execute("set", "ignore_early_media=true", pstSCB->szUUID);
            sc_ep_esl_execute("set", "timer_name=soft", pstSCB->szUUID);
            sc_ep_esl_execute("sleep", "500", pstSCB->szUUID);

            dos_snprintf(szAPPParam, sizeof(szAPPParam)
                            , "1 1 %u 0 # %s pdtmf \\d+"
                            , sc_task_audio_playcnt(pstSCB->usTCBNo)
                            , sc_task_get_audio_file(pstSCB->usTCBNo));

            sc_ep_esl_execute("play_and_get_digits", szAPPParam, pstSCB->szUUID);
            pstSCB->ucCurrentPlyCnt = sc_task_audio_playcnt(pstSCB->usTCBNo);
            break;

        case SC_TASK_MODE_AUDIO_ONLY:
        case SC_TASK_MODE_AGENT_AFTER_AUDIO:
            sc_ep_esl_execute("set", "ignore_early_media=true", pstSCB->szUUID);
            sc_ep_esl_execute("set", "timer_name=soft", pstSCB->szUUID);
            sc_ep_esl_execute("sleep", "500", pstSCB->szUUID);

            dos_snprintf(szAPPParam, sizeof(szAPPParam)
                            , "+%d %s"
                            , sc_task_audio_playcnt(pstSCB->usTCBNo)
                            , sc_task_get_audio_file(pstSCB->usTCBNo));

            sc_ep_esl_execute("loop_playback", szAPPParam, pstSCB->szUUID);
            pstSCB->ucCurrentPlyCnt = sc_task_audio_playcnt(pstSCB->usTCBNo);

            break;

        /* 直接接通坐席 */
        case SC_TASK_MODE_DIRECT4AGETN:
            sc_ep_call_queue_add(pstSCB, sc_task_get_agent_queue(pstSCB->usTCBNo), DOS_FALSE);

            break;

        default:
            DOS_ASSERT(0);
            ulErrCode = CC_ERR_SC_CONFIG_ERR;
            goto auto_call_proc_error;
    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_ACTIVE);

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

auto_call_proc_error:
    sc_call_trace(pstSCB, "FAILED to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    if (DOS_ADDR_VALID(pstSCB))
    {
        SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);
        sc_ep_hangup_call_with_snd(pstSCB, ulErrCode);
    }

    return DOS_FAIL;
}

U32 sc_ep_demo_task_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8      szAPPParam[512]    = { 0, };
    U32     ulErrCode          = CC_ERR_NO_REASON;

    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        goto proc_error;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    if (!sc_call_check_service(pstSCB, SC_SERV_DEMO_TASK))
    {
        DOS_ASSERT(0);

        sc_logr_debug(pstSCB, SC_ESL, "Process event %s finished. SCB do not include service auto call."
                            , esl_event_get_header(pstEvent, "Event-Name"));

        ulErrCode = CC_ERR_SC_NO_SERV_RIGHTS;
        goto proc_error;
    }

    sc_ep_esl_execute("set", "ignore_early_media=true", pstSCB->szUUID);
    sc_ep_esl_execute("set", "timer_name=soft", pstSCB->szUUID);
    sc_ep_esl_execute("sleep", "500", pstSCB->szUUID);

    dos_snprintf(szAPPParam, sizeof(szAPPParam)
                    , "1 1 %u 0 # %s pdtmf \\d+"
                    , SC_DEMOE_TASK_COUNT
                    , SC_DEMOE_TASK_FILE);

    sc_ep_esl_execute("play_and_get_digits", szAPPParam, pstSCB->szUUID);
    pstSCB->ucCurrentPlyCnt = SC_DEMOE_TASK_COUNT;

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_ACTIVE);

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

proc_error:
    sc_call_trace(pstSCB, "FAILED to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    if (DOS_ADDR_VALID(pstSCB))
    {
        SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);
        sc_ep_hangup_call_with_snd(pstSCB, ulErrCode);
    }

    return DOS_FAIL;
}

/**
 * 函数: U32 sc_ep_num_verify(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 执行语音验证码业务
 * 参数:
 *      esl_handle_t *pstHandle : 发送数据的handle
 *      esl_event_t *pstEvent   : ESL 事件
 *      SC_SCB_ST *pstSCB       : 业务控制块
 * 返回值: 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_ep_num_verify(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8 szCmdParam[128] = { 0 };
    S8 szPlayParam[128] = {0};
    U32 ulPlayCnt = 0;

    ulPlayCnt = pstSCB->ucCurrentPlyCnt;
    if (ulPlayCnt < SC_NUM_VERIFY_TIME_MIN
        || ulPlayCnt > SC_NUM_VERIFY_TIME_MAX)
    {
        ulPlayCnt = SC_NUM_VERIFY_TIME;
    }

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);

        sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_MESSAGE_PARAM_ERR);

        return DOS_FAIL;
    }

    dos_snprintf(szPlayParam, sizeof(szPlayParam), "file_string://%s/nindyzm.wav", SC_PROMPT_TONE_PATH);
    dos_snprintf(szCmdParam, sizeof(szCmdParam), "en name_spelled iterated %s", pstSCB->szDialNum);
    sc_ep_esl_execute("answer", NULL, pstSCB->szUUID);
    sc_ep_esl_execute("sleep", "1000", pstSCB->szUUID);

    while (ulPlayCnt-- > 0)
    {
        sc_ep_esl_execute("playback", szPlayParam, pstSCB->szUUID);
        sc_ep_esl_execute("say", szCmdParam, pstSCB->szUUID);
        sc_ep_esl_execute("sleep", "1000", pstSCB->szUUID);
    }

    sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_ep_internal_call_process(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理内部呼叫
 * 参数:
 *      esl_handle_t *pstHandle : 发送数据的handle
 *      esl_event_t *pstEvent   : ESL 事件
 *      SC_SCB_ST *pstSCB       : 业务控制块
 * 返回值: 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_ep_internal_call_process(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8    *pszSrcNum = NULL;
    S8    *pszDstNum = NULL;
    S8    *pszUUID = NULL;
    S8    szSIPUserID[32] = { 0, };
    S8    szCallString[512] = { 0, };
    U32   ulCustomerID = U32_BUTT;
    U32   ulCustomerID1 = U32_BUTT;
    U32   ulErrCode = CC_ERR_NO_REASON;
    SC_ACD_AGENT_INFO_ST stAgentInfo;

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstHandle) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* 获取事件的UUID */
    pszUUID = esl_event_get_header(pstEvent, "Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        ulErrCode = CC_ERR_SC_MESSAGE_PARAM_ERR;
        goto process_fail;
    }

    pszDstNum = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    if (DOS_ADDR_INVALID(pszDstNum))
    {
        DOS_ASSERT(0);

        ulErrCode = CC_ERR_SC_MESSAGE_PARAM_ERR;
        goto process_fail;
    }

    pszSrcNum = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    if (DOS_ADDR_INVALID(pszSrcNum))
    {
        DOS_ASSERT(0);

        ulErrCode = CC_ERR_SC_MESSAGE_PARAM_ERR;
        goto process_fail;
    }

    ulCustomerID = pstSCB->ulCustomID;
    if (U32_BUTT == ulCustomerID)
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "The source number %s seem not beyound to any customer, Reject Call", pszSrcNum);
        ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
        goto process_fail;
    }

    /* 判断被叫号码是否是分机号，如果是分机号，就要找到对应的SIP账户，再呼叫，同时呼叫之前还需要获取主叫的分机号，修改ANI为主叫的分机号 */
    ulCustomerID1 = sc_ep_get_custom_by_sip_userid(pszDstNum);
    if (U32_BUTT != ulCustomerID1)
    {
        if (ulCustomerID == ulCustomerID1)
        {
            if (sc_acd_get_agent_by_userid(&stAgentInfo, pszDstNum) == DOS_SUCC)
            {
                /* 不需要再判断分机的状态 */
#if 0
                /* 根据分机找到分机绑定的坐席，判断坐席的状态 */
                if (stAgentInfo.ucBindType == AGENT_BIND_SIP
                    && stAgentInfo.ucStatus != SC_ACD_IDEL)
                {
                    /* 加入到队列 */

                    goto process_fail;
                }
#endif
                dos_snprintf(szCallString, sizeof(szCallString), "{other_leg_scb=%d,update_agent_id=%d}user/%s", pstSCB->usSCBNo, stAgentInfo.ulSiteID, pszDstNum);
                sc_ep_esl_execute("bridge", szCallString, pszUUID);
            }
            else
            {
                dos_snprintf(szCallString, sizeof(szCallString), "user/%s", pszDstNum);
                sc_ep_esl_execute("bridge", szCallString, pszUUID);
                //sc_ep_esl_execute("hangup", NULL, pszUUID);
            }
        }
        else
        {
            DOS_ASSERT(0);

            sc_logr_info(pstSCB, SC_ESL, "Cannot call other customer direct, Reject Call. Src %s is owned by customer %d, Dst %s is owned by customer %d"
                            , pszSrcNum, ulCustomerID, pszDstNum, ulCustomerID1);
            ulErrCode = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;
            goto process_fail;
        }
    }
    else
    {
        if (sc_ep_get_userid_by_extension(ulCustomerID, pszDstNum, szSIPUserID, sizeof(szSIPUserID)) != DOS_SUCC)
        {
            DOS_ASSERT(0);

            sc_logr_info(pstSCB, SC_ESL, "Destination number %s is not seems a SIP User ID or Extension. Reject Call", pszDstNum);
            ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
            goto process_fail;
        }

        dos_snprintf(szCallString, sizeof(szCallString), "user/%s", szSIPUserID);
        sc_ep_esl_execute("bridge", szCallString, pszUUID);
    }

    return DOS_SUCC;

process_fail:
    if (pstSCB)
    {
        sc_ep_hangup_call_with_snd(pstSCB, ulErrCode);
    }

    return DOS_FAIL;
}

/**
 * 函数: U32 sc_ep_internal_call_process(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理内部呼叫
 * 参数:
 *      esl_handle_t *pstHandle : 发送数据的handle
 *      esl_event_t *pstEvent   : ESL 事件
 *      SC_SCB_ST *pstSCB       : 业务控制块
 * 返回值: 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 sc_ep_internal_service_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8    *pszUUID = NULL;

    sc_logr_info(pstSCB, SC_ESL, "%s", "Start exec internal service.");

    pszUUID = esl_event_get_header(pstEvent, "Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_ep_esl_execute("answer", NULL, pszUUID);
    sc_ep_esl_execute("sleep", "1000", pszUUID);
    sc_ep_esl_execute("speak", "flite|kal|Temporary not support.", pszUUID);
    sc_ep_esl_execute("hangup", NULL, pszUUID);
    sc_ep_hangup_call(pstSCB, CC_ERR_SC_SERV_NOT_EXIST);

    return DOS_SUCC;
}


/**
 * 函数: U32 sc_ep_system_stat(esl_event_t *pstEvent)
 * 功能: 统计信息
 * 参数:
 *      esl_event_t *pstEvent   : 事件
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_system_stat(esl_event_t *pstEvent)
{
    S8 *pszProfileName         = NULL;
    S8 *pszGatewayName         = NULL;
    S8 *pszOtherLeg            = NULL;
    S8 *pszSIPHangupCause      = NULL;
    U32 ulGatewayID            = 0;
    HASH_NODE_S   *pstHashNode = NULL;
    SC_GW_NODE_ST *pstGateway  = NULL;
    U32  ulIndex = U32_BUTT;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }


    pszGatewayName = esl_event_get_header(pstEvent, "variable_sip_profile_name");
    if (DOS_ADDR_VALID(pszGatewayName)
        && dos_strncmp(pszGatewayName, "gateway", dos_strlen("gateway")) == 0)
    {
        pszGatewayName = esl_event_get_header(pstEvent, "variable_sip_gateway_name");
        if (DOS_ADDR_VALID(pszGatewayName)
            && dos_atoul(pszGatewayName, &ulGatewayID) >= 0)
        {
            ulIndex = sc_ep_gw_hash_func(ulGatewayID);
            pthread_mutex_lock(&g_mutexHashGW);
            pstHashNode = hash_find_node(g_pstHashGW, ulIndex, (VOID *)&ulGatewayID, sc_ep_gw_hash_find);
            if (DOS_ADDR_VALID(pstHashNode)
                && DOS_ADDR_VALID(pstHashNode->pHandle))
            {
                pstGateway = pstHashNode->pHandle;
            }
            else
            {
                pstGateway = NULL;
                DOS_ASSERT(0);
            }
            pthread_mutex_unlock(&g_mutexHashGW);
        }
    }

    if (DOS_ADDR_INVALID(pstGateway))
    {
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashGW);
    if (ESL_EVENT_CHANNEL_CREATE == pstEvent->event_id)
    {
        g_pstTaskMngtInfo->stStat.ulTotalSessions++;
        g_pstTaskMngtInfo->stStat.ulCurrentSessions++;
        if (g_pstTaskMngtInfo->stStat.ulCurrentSessions > g_pstTaskMngtInfo->stStat.ulMaxSession)
        {
            g_pstTaskMngtInfo->stStat.ulMaxSession = g_pstTaskMngtInfo->stStat.ulCurrentSessions;
        }

        if (pstGateway && DOS_FALSE != pstGateway->bStatus)
        {
            pstGateway->stStat.ulTotalSessions++;
            pstGateway->stStat.ulCurrentSessions++;
            if (pstGateway->stStat.ulCurrentSessions > pstGateway->stStat.ulMaxSession)
            {
                pstGateway->stStat.ulMaxSession = pstGateway->stStat.ulCurrentSessions;
            }
        }

        pszOtherLeg = esl_event_get_header(pstEvent, "variable_other_leg_scb");
        if (DOS_ADDR_INVALID(pszOtherLeg))
        {
            g_pstTaskMngtInfo->stStat.ulTotalCalls++;
            g_pstTaskMngtInfo->stStat.ulCurrentCalls++;
            if (g_pstTaskMngtInfo->stStat.ulCurrentCalls > g_pstTaskMngtInfo->stStat.ulMaxCalls)
            {
                g_pstTaskMngtInfo->stStat.ulMaxCalls = g_pstTaskMngtInfo->stStat.ulCurrentCalls;
            }

            if (pstGateway && DOS_FALSE != pstGateway->bStatus)
            {
                pstGateway->stStat.ulTotalCalls++;
                pstGateway->stStat.ulCurrentCalls++;
                if (pstGateway->stStat.ulCurrentCalls > pstGateway->stStat.ulMaxCalls)
                {
                    pstGateway->stStat.ulMaxCalls = pstGateway->stStat.ulCurrentCalls;
                }
            }
        }

        pszProfileName = esl_event_get_header(pstEvent, "variable_is_outbound");
        if (DOS_ADDR_VALID(pszProfileName)
            && dos_strncmp(pszProfileName, "true", dos_strlen("true")) == 0)
        {
            g_pstTaskMngtInfo->stStat.ulOutgoingSessions++;

            if (pstGateway && DOS_FALSE != pstGateway->bStatus)
            {
                pstGateway->stStat.ulOutgoingSessions++;
            }
        }
        else
        {
            g_pstTaskMngtInfo->stStat.ulIncomingSessions++;

            if (pstGateway && DOS_FALSE != pstGateway->bStatus)
            {
                pstGateway->stStat.ulIncomingSessions++;
            }
        }
    }
    else if (ESL_EVENT_CHANNEL_HANGUP_COMPLETE == pstEvent->event_id)
    {
        if (g_pstTaskMngtInfo->stStat.ulCurrentSessions > 0)
        {
            g_pstTaskMngtInfo->stStat.ulCurrentSessions--;
        }

        if (pstGateway && DOS_FALSE != pstGateway->bStatus)
        {
            if (pstGateway->stStat.ulCurrentSessions > 0)
            {
                pstGateway->stStat.ulCurrentSessions--;
            }
        }


        pszOtherLeg = esl_event_get_header(pstEvent, "variable_other_leg_scb");
        if (DOS_ADDR_INVALID(pszOtherLeg))
        {
            if (g_pstTaskMngtInfo->stStat.ulCurrentCalls > 0)
            {
                g_pstTaskMngtInfo->stStat.ulCurrentCalls--;
            }

            if (pstGateway && DOS_FALSE != pstGateway->bStatus)
            {
                if (pstGateway->stStat.ulCurrentCalls > 0)
                {
                    pstGateway->stStat.ulCurrentCalls--;
                }
            }
        }

        /* 如果挂断时SIP的响应码不是2xx就认为失败的session,通道变量
           variable_proto_specific_hangup_cause格式为 sip:200 后面数字为sip错误码 */
        pszSIPHangupCause = esl_event_get_header(pstEvent, "variable_proto_specific_hangup_cause");
        if (DOS_ADDR_VALID(pszSIPHangupCause))
        {
            if (dos_strncmp(pszSIPHangupCause, "sip:2", dos_strlen("sip:2")) != 0)
            {
                g_pstTaskMngtInfo->stStat.ulFailSessions++;
                if (pstGateway && DOS_FALSE != pstGateway->bStatus)
                {
                    pstGateway->stStat.ulFailSessions++;
                }
            }
        }
    }

    pthread_mutex_unlock(&g_mutexHashGW);


    return DOS_SUCC;
}

U32 sc_ep_get_agent_by_caller(SC_SCB_ST *pstSCB, SC_ACD_AGENT_INFO_ST *pstAgentInfo, S8 *szCallerNum, esl_event_t *pstEvent)
{
    const S8 *pszCallSource;

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(szCallerNum)
        || DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 将caller作为sip分机查找坐席，如果找不到，再作为 TT 号，继续查找 */
    if (sc_acd_get_agent_by_userid(pstAgentInfo, szCallerNum) == DOS_SUCC)
    {
        sc_logr_debug(pstSCB, SC_ESL, "POTS, Find agent(%u) by userid(%s) SUCC", pstAgentInfo->ulSiteID, szCallerNum);

        return DOS_SUCC;
    }

    sc_logr_debug(pstSCB, SC_ESL, "POTS, Can not find agent by userid(%s)", szCallerNum);

    /* 判断 来源，是不是配置的中继 */
    pszCallSource = esl_event_get_header(pstEvent, "Caller-Network-Addr");
    if (DOS_ADDR_INVALID(pszCallSource))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (sc_find_gateway_by_addr(pszCallSource) != DOS_SUCC)
    {
        sc_logr_info(pstSCB, SC_ESL, "POTS, Find gateway by addr(%s) FAIL", pszCallSource);

        return DOS_FAIL;
    }

    /* 判断 主叫号码 是否是 TT 号，现在 TT号 只支持外呼 */
    if (sc_acd_get_agent_by_tt_num(pstAgentInfo, szCallerNum, NULL) == DOS_SUCC)
    {
        sc_logr_debug(pstSCB, SC_ESL, "POTS, Find agent(%u) by TT num(%s) SUCC", pstAgentInfo->ulSiteID, szCallerNum);

        return DOS_SUCC;
    }

    return DOS_FAIL;
}
/**
 * 函数: U32 sc_ep_pots_pro(SC_SCB_ST *pstSCB, BOOL bIsSecondaryDialing)
 * 功能: 新业务处理
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_pots_pro(SC_SCB_ST *pstSCB, esl_event_t *pstEvent, BOOL bIsSecondaryDialing)
{
    U32         ulRet       = DOS_FAIL;
    BOOL        bIsHangUp   = DOS_TRUE;
    SC_ACD_AGENT_INFO_ST    stAgentInfo;
    S8          pszCallee[SC_TEL_NUMBER_LENGTH] = {0};
    S8          pszDealNum[SC_TEL_NUMBER_LENGTH] = {0};
    S8          pszEmpNum[SC_TEL_NUMBER_LENGTH] = {0};
    U32         ulKey       = U32_BUTT;
    U32         ulNeedTip   = DOS_TRUE;
    SC_SCB_ST   *pstSCBOther = NULL;
    S8          *pszValue    = NULL;
    U64         uLTmp        = 0;
    S8          szCMD[128]   = {0};

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstEvent))
    {
        return DOS_FAIL;
    }

    if (bIsSecondaryDialing)
    {
        dos_strcpy(pszDealNum, pstSCB->szDialNum);
    }
    else
    {
        dos_strcpy(pszDealNum, pstSCB->szCalleeNum);
    }

    sc_logr_debug(pstSCB, SC_FUNC, "POTS scb(%u) num(%s)", pstSCB->usSCBNo, pszDealNum);

    if (!bIsSecondaryDialing)
    {
        pstSCB->ulCustomID = sc_ep_get_custom_by_sip_userid(pstSCB->szCallerNum);
        if (U32_BUTT == pstSCB->ulCustomID)
        {
            if (sc_acd_get_agent_by_tt_num(&stAgentInfo, pstSCB->szCallerNum, NULL) == DOS_SUCC)
            {
                pstSCB->ulCustomID = stAgentInfo.ulCustomerID;
            }
        }
    }

    if (pszDealNum[0] != '*'
        && pszDealNum[0] != '#')
    {
        /* 不是以 '*'或者'#'开头不处理 */
        return DOS_FAIL;
    }

    if ((dos_strcmp(pszDealNum, SC_POTS_HANGUP_CUSTOMER1) == 0 || dos_strcmp(pszDealNum, SC_POTS_HANGUP_CUSTOMER2) == 0)
        && bIsSecondaryDialing)
    {
        SC_SCB_ST *pstSCBPublishd;
        /* 二次拨号时，挂断客户的电话 */
        pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
        pstSCBPublishd = sc_scb_get(pstSCB->usPublishSCB);
        if (DOS_ADDR_VALID(pstSCBOther)
            && !pstSCBOther->bWaitingOtherRelase)
        {
            if (sc_call_check_service(pstSCB, SC_SERV_ATTEND_TRANSFER))
            {
                sc_ep_transfer_publish_release(pstSCB);
            }
            else if (sc_call_check_service(pstSCBPublishd, SC_SERV_ATTEND_TRANSFER))
            {
                sc_ep_transfer_notify_release(pstSCB);
            }
            else
            {
                sc_ep_hangup_call(pstSCBOther, CC_ERR_NORMAL_CLEAR);
            }

            ulNeedTip = DOS_FALSE;
        }

        ulRet = DOS_SUCC;
        bIsHangUp = DOS_FALSE;
    }
    else if (dos_strncmp(pszDealNum, SC_POTS_MARK_CUSTOMER, dos_strlen(SC_POTS_MARK_CUSTOMER)) == 0
        && !bIsSecondaryDialing)
    {
        /* 标记上一个客户，只支持话机操作 */
        if (sc_ep_get_agent_by_caller(pstSCB, &stAgentInfo, pstSCB->szCallerNum, pstEvent) != DOS_SUCC)
        {
            sc_logr_info(pstSCB, SC_ACD, "POTS, Can not find agent by caller(%s)", pstSCB->szCallerNum);
            bIsHangUp = DOS_FALSE;

            goto end;
        }

        if (stAgentInfo.szLastCustomerNum[0] == '\0')
        {
            /* 不存在上一个客户，放音提示 */
            goto end;
        }

        /* 将主叫号码修改为被修改的客户的号码 */
        dos_strcpy(pstSCB->szCallerNum, stAgentInfo.szLastCustomerNum);
        dos_strcpy(pstSCB->szSiteNum, stAgentInfo.szEmpNo);
        //pstSCB->ulCustomID = stAgentInfo.ulCustomerID;

        if (1 == dos_sscanf(pszDealNum+dos_strlen(SC_POTS_MARK_CUSTOMER), "%d", &ulKey) && ulKey <= 9)
        {
            sc_send_marker_update_req(stAgentInfo.ulCustomerID, stAgentInfo.ulSiteID, ulKey, stAgentInfo.szLastCustomerNum);
        }
        else
        {
            /* 格式错误，放音提示 */
            goto end;
        }

        sc_logr_debug(pstSCB, SC_ACD, "POTS, find agent(%u) by userid(%s), mark customer", stAgentInfo.ulSiteID, pstSCB->szCallerNum);

        ulRet = DOS_SUCC;
    }
    else if (dos_strcmp(pszDealNum, SC_POTS_BALANCE) == 0
        && !bIsSecondaryDialing)
    {
        /* 查询余额 只支持话机操作 */
        if (pstSCB->ulCustomID != U32_BUTT)
        {
            sc_send_balance_query2bs(pstSCB);

            ulRet = DOS_SUCC;
        }

        ulNeedTip = DOS_FALSE;
        bIsHangUp = DOS_FALSE;
    }
    else if (dos_strcmp(pszDealNum, SC_POTS_AGENT_ONLINE) == 0
        && !bIsSecondaryDialing)
    {
        /* 坐席登陆web页面 只支持话机操作 */
        if (sc_ep_get_agent_by_caller(pstSCB, &stAgentInfo, pstSCB->szCallerNum, pstEvent) != DOS_SUCC)
        {
            sc_logr_info(pstSCB, SC_ACD, "POTS, Can not find agent by caller(%s)", pstSCB->szCallerNum);
            bIsHangUp = DOS_FALSE;

            goto end;
        }

        sc_logr_debug(pstSCB, SC_ACD, "POTS, find agent(%u) by userid(%s), online", stAgentInfo.ulSiteID, pstSCB->szCallerNum);
        sc_acd_update_agent_status(SC_ACD_SITE_ACTION_ONLINE, stAgentInfo.ulSiteID, OPERATING_TYPE_PHONE);

        ulRet = DOS_SUCC;
    }
    else if (dos_strcmp(pszDealNum, SC_POTS_AGENT_OFFLINE) == 0
        && !bIsSecondaryDialing)
    {
        /* 坐席从web页面下线 */
        if (sc_ep_get_agent_by_caller(pstSCB, &stAgentInfo, pstSCB->szCallerNum, pstEvent) != DOS_SUCC)
        {
            sc_logr_info(pstSCB, SC_ACD, "POTS, Can not find agent by caller(%s)", pstSCB->szCallerNum);

            goto end;
        }

        sc_logr_debug(pstSCB, SC_ACD, "POTS, find agent(%u) by userid(%s), offline", stAgentInfo.ulSiteID, pstSCB->szCallerNum);
        sc_acd_update_agent_status(SC_ACD_SITE_ACTION_OFFLINE, stAgentInfo.ulSiteID, OPERATING_TYPE_PHONE);

        ulRet = DOS_SUCC;
    }
    else if (dos_strcmp(pszDealNum, SC_POTS_AGENT_EN_QUEUE) == 0)
    {
        /* 坐席置闲 */
        if (bIsSecondaryDialing)
        {
            if (sc_acd_get_agent_by_id(&stAgentInfo, pstSCB->ulAgentID) != DOS_SUCC)
            {
                sc_logr_info(pstSCB, SC_ACD, "POTS, Can not find agent by userid(%s)", pstSCB->szCallerNum);
                bIsHangUp = DOS_FALSE;

                goto end;
            }
        }
        else
        {
            if (sc_ep_get_agent_by_caller(pstSCB, &stAgentInfo, pstSCB->szCallerNum, pstEvent) != DOS_SUCC)
            {
                sc_logr_info(pstSCB, SC_ACD, "POTS, Can not find agent by caller(%s)", pstSCB->szCallerNum);

                goto end;
            }
        }

        sc_logr_debug(pstSCB, SC_ACD, "POTS, find agent(%u) by userid(%s), en queue", stAgentInfo.ulSiteID, pstSCB->szCallerNum);
        sc_acd_update_agent_status(SC_ACD_SITE_ACTION_EN_QUEUE, stAgentInfo.ulSiteID, OPERATING_TYPE_PHONE);
        ulRet = DOS_SUCC;
        if (bIsSecondaryDialing)
        {
            /* 二次拨号，不用挂机 */
            bIsHangUp = DOS_FALSE;
        }
    }
    else if (dos_strcmp(pszDealNum, SC_POTS_AGENT_DN_QUEUE) == 0)
    {
        /* 坐席置忙 */
        if (bIsSecondaryDialing)
        {
            if (sc_acd_get_agent_by_id(&stAgentInfo, pstSCB->ulAgentID) != DOS_SUCC)
            {
                sc_logr_info(pstSCB, SC_ACD, "POTS, Can not find agent by userid(%s)", pstSCB->szCallerNum);
                bIsHangUp = DOS_FALSE;

                goto end;
            }
        }
        else
        {
            if (sc_ep_get_agent_by_caller(pstSCB, &stAgentInfo, pstSCB->szCallerNum, pstEvent) != DOS_SUCC)
            {
                sc_logr_info(pstSCB, SC_ACD, "POTS, Can not find agent by caller(%s)", pstSCB->szCallerNum);

                goto end;
            }
        }

        sc_logr_debug(pstSCB, SC_ACD, "POTS, find agent(%u) by userid(%s), dn queue", stAgentInfo.ulSiteID, pstSCB->szCallerNum);
        sc_acd_update_agent_status(SC_ACD_SITE_ACTION_DN_QUEUE, stAgentInfo.ulSiteID, OPERATING_TYPE_PHONE);
        ulRet = DOS_SUCC;
        if (bIsSecondaryDialing)
        {
            /* 二次拨号，不用挂机 */
            bIsHangUp = DOS_FALSE;
        }
    }
    else if (dos_strcmp(pszDealNum, SC_POTS_AGENT_SIGNIN) == 0
        && !bIsSecondaryDialing)
    {
        /* 坐席长签 只支持话机操作 */
        if ((ulRet = sc_acd_singin_by_phone(pstSCB->szCallerNum, pstSCB)) != DOS_SUCC)
        {
            sc_logr_info(pstSCB, SC_ACD, "POTS, Can not find agent by caller(%s)", pstSCB->szCallerNum);

            goto end;
        }

        sc_logr_debug(pstSCB, SC_ACD, "POTS, agent(%u) by userid(%s), signin SUCC", stAgentInfo.ulSiteID, pstSCB->szCallerNum);
        sc_ep_esl_execute("answer", NULL, pstSCB->szUUID);
        sc_ep_esl_execute("park", NULL, pstSCB->szUUID);

        /*
            更新一下坐席长签的开始时间，
            到这里时，还没有Caller-Channel-Answered-Time，只能用Caller-Channel-Created-Time
        */
        pszValue = esl_event_get_header(pstEvent, "Caller-Channel-Created-Time");
        if (DOS_ADDR_VALID(pszValue)
            && '\0' != pszValue[0]
            && dos_atoull(pszValue, &uLTmp) == 0)
        {
            pstSCB->ulSiginTimeStamp = uLTmp / 1000000;
            sc_logr_debug(pstSCB, SC_ESL, "Get extra data: Caller-Channel-Answered-Time=%s(%u)", pszValue, pstSCB->ulSiginTimeStamp);
        }

        bIsHangUp = DOS_FALSE;
    }
    else if (dos_strcmp(pszDealNum, SC_POTS_AGENT_SIGNOUT) == 0)
    {
        /* 坐席退出长签 */
        if (bIsSecondaryDialing)
        {
            if (sc_acd_get_agent_by_id(&stAgentInfo, pstSCB->ulAgentID) != DOS_SUCC)
            {
                sc_logr_info(pstSCB, SC_ACD, "POTS, Can not find agent by userid(%s)", pstSCB->szCallerNum);

                goto end;
            }
        }
        else
        {
            if (sc_ep_get_agent_by_caller(pstSCB, &stAgentInfo, pstSCB->szCallerNum, pstEvent) != DOS_SUCC)
            {
                sc_logr_info(pstSCB, SC_ACD, "POTS, Can not find agent by userid(%s)", pstSCB->szCallerNum);

                goto end;
            }
        }

        sc_logr_debug(pstSCB, SC_ACD, "POTS, find agent(%u) by userid(%s), signout", stAgentInfo.ulSiteID, pstSCB->szCallerNum);
        sc_acd_update_agent_status(SC_ACD_SITE_ACTION_SIGNOUT, stAgentInfo.ulSiteID, OPERATING_TYPE_PHONE);

        ulRet = DOS_SUCC;
    }
    else if (dos_strncmp(pszDealNum, SC_POTS_BLIND_TRANSFER, dos_strlen(SC_POTS_BLIND_TRANSFER)) == 0
        && bIsSecondaryDialing)
    {
        /* 盲转  只支持二次拨号 */
        /* 先根据被叫号码，获得被转坐席的id，根据坐席的工号找个坐席 */
        bIsHangUp = DOS_FALSE;
        if (dos_sscanf(pszDealNum, "*%*[^*]*%[^#]s", pszEmpNum) != 1)
        {
            sc_logr_info(pstSCB, SC_ACD, "POTS, format error : %s", pszDealNum);

            goto end;
        }

        if (sc_acd_get_agent_by_emp_num(&stAgentInfo, pstSCB->ulCustomID, pszEmpNum) != DOS_SUCC)
        {
            sc_logr_info(pstSCB, SC_ACD, "POTS, Can not find agent. customer id(%u), empNum(%s)", pstSCB->ulCustomID, pszEmpNum);

            goto end;
        }

        /* 判断坐席的状态 */
        if (!SC_ACD_SITE_IS_USEABLE(&stAgentInfo))
        {
            sc_logr_debug(pstSCB, SC_ACD, "The agent is not useable.(Agent %u)", stAgentInfo.ulSiteID);

            goto end;
        }
#if 0
        /* 对端在长签状态，不要打扰 */
        if (stAgentInfo.bConnected)
        {
            sc_logr_debug(pstSCB, SC_ACD, "The agent is not useable.(Agent %u)", stAgentInfo.ulSiteID);

            goto end;
        }
#endif

        /* 获取被叫号码 */
        switch (stAgentInfo.ucBindType)
        {
            case AGENT_BIND_SIP:
                dos_strncpy(pszCallee, stAgentInfo.szUserID, SC_TEL_NUMBER_LENGTH);
                pszCallee[SC_TEL_NUMBER_LENGTH - 1] = '\0';
                break;
            case AGENT_BIND_TELE:
                dos_strncpy(pszCallee, stAgentInfo.szTelePhone, SC_TEL_NUMBER_LENGTH);
                pszCallee[SC_TEL_NUMBER_LENGTH - 1] = '\0';
                break;
            case AGENT_BIND_MOBILE:
                dos_strncpy(pszCallee, stAgentInfo.szMobile, SC_TEL_NUMBER_LENGTH);
                pszCallee[SC_TEL_NUMBER_LENGTH - 1] = '\0';
                break;
            case AGENT_BIND_TT_NUMBER:
                dos_strncpy(pszCallee, stAgentInfo.szTTNumber, SC_TEL_NUMBER_LENGTH);
                pszCallee[SC_TEL_NUMBER_LENGTH - 1] = '\0';
                break;
            default:
                goto end;
        }

        if (sc_ep_call_transfer(pstSCB, stAgentInfo.ulSiteID, stAgentInfo.ucBindType, pszCallee, DOS_FALSE) != DOS_SUCC)
        {
            goto end;
        }

    }
    else if (dos_strncmp(pszDealNum, SC_POTS_ATTENDED_TRANSFER, dos_strlen(SC_POTS_ATTENDED_TRANSFER)) == 0
        && bIsSecondaryDialing)
    {
        /* 协商转 只支持二次拨号 */
        bIsHangUp = DOS_FALSE;

        if (dos_sscanf(pszDealNum, "*%*[^*]*%[^#]s", pszEmpNum) != 1)
        {
            sc_logr_info(pstSCB, SC_ACD, "POTS, format error : %s", pszDealNum);

            goto end;
        }

        if (sc_acd_get_agent_by_emp_num(&stAgentInfo, pstSCB->ulCustomID, pszEmpNum) != DOS_SUCC)
        {
            sc_logr_info(pstSCB, SC_ACD, "POTS, Can not find agent. customer id(%u), empNum(%s)", pstSCB->ulCustomID, pszEmpNum);

            goto end;
        }

        /* 判断坐席的状态 */
        if (!SC_ACD_SITE_IS_USEABLE(&stAgentInfo))
        {
            sc_logr_debug(pstSCB, SC_ACD, "The agent is not useable.(Agent %u)", stAgentInfo.ulSiteID);

            goto end;
        }
#if 0
        /* 对端在长签状态，不要打扰 */
        if (stAgentInfo.bConnected)
        {
            sc_logr_debug(pstSCB, SC_ACD, "The agent is not useable.(Agent %u)", stAgentInfo.ulSiteID);

            goto end;
        }
#endif
        /* 获取被叫号码 */
        switch (stAgentInfo.ucBindType)
        {
            case AGENT_BIND_SIP:
                dos_strncpy(pszCallee, stAgentInfo.szUserID, SC_TEL_NUMBER_LENGTH);
                pszCallee[SC_TEL_NUMBER_LENGTH - 1] = '\0';
                break;

            case AGENT_BIND_TELE:
                dos_strncpy(pszCallee, stAgentInfo.szTelePhone, SC_TEL_NUMBER_LENGTH);
                pszCallee[SC_TEL_NUMBER_LENGTH - 1] = '\0';
                break;

            case AGENT_BIND_MOBILE:
                dos_strncpy(pszCallee, stAgentInfo.szMobile, SC_TEL_NUMBER_LENGTH);
                pszCallee[SC_TEL_NUMBER_LENGTH - 1] = '\0';
                break;

            case AGENT_BIND_TT_NUMBER:
                dos_strncpy(pszCallee, stAgentInfo.szTTNumber, SC_TEL_NUMBER_LENGTH);
                pszCallee[SC_TEL_NUMBER_LENGTH - 1] = '\0';
                break;

            default:
                goto end;
        }

        if (sc_ep_call_transfer(pstSCB, stAgentInfo.ulSiteID, stAgentInfo.ucBindType, pszCallee, DOS_TRUE) != DOS_SUCC)
        {
            goto end;
        }
    }
    else if (bIsSecondaryDialing
            && pszDealNum[0] == '*'
            && pszDealNum[2] == '\0'
            && 1 == dos_sscanf(pszDealNum+1, "%u", &ulKey)
            && ulKey <= 9)
    {
        /* 判断是否是客户标记 *D# / *D* */
        pstSCB->szCustomerMark[0] = '\0';
        dos_strcpy(pstSCB->szCustomerMark, pszDealNum);
        /* 最后加上#，与之前的版本保持一致 */
        pstSCB->szCustomerMark[2] = '#';
        pstSCB->szCustomerMark[SC_CUSTOMER_MARK_LENGTH-1] = '\0';

        if (pstSCB->szCustomerNum[0] != '\0')
        {
            sc_send_marker_update_req(pstSCB->ulCustomID, pstSCB->ulAgentID, ulKey, pstSCB->szCustomerNum);
        }
        else
        {
            if (sc_call_check_service(pstSCB, SC_SERV_OUTBOUND_CALL))
            {
                sc_send_marker_update_req(pstSCB->ulCustomID, pstSCB->ulAgentID, ulKey, pstSCB->szCallerNum);
            }
            else
            {
                sc_send_marker_update_req(pstSCB->ulCustomID, pstSCB->ulAgentID, ulKey, pstSCB->szCalleeNum);
            }
        }

        sc_logr_debug(pstSCB, SC_ESL, "dtmf proc, callee : %s, caller : %s, UUID : %s"
            , pstSCB->szCalleeNum, pstSCB->szCallerNum, pstSCB->szUUID);

        /* 操作成功，放音提示 */
        sc_ep_play_sound(SC_SND_OPT_SUCC, pstSCB->szUUID, 1);
        sc_acd_update_agent_info(pstSCB, SC_ACD_IDEL, pstSCB->usSCBNo, NULL);

        pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
        /* 呼叫过程中挂断 */
        if (SC_SCB_IS_VALID(pstSCBOther) && pstSCBOther->ucStatus != SC_SCB_RELEASE)
        {
            sc_ep_hangup_call(pstSCBOther, CC_ERR_NORMAL_CLEAR);

            pstSCB->bIsMarkCustomer = DOS_TRUE;
        }
        else
        {
            /* 挂断后标记,恢复为初始化值 */
            pstSCB->bIsMarkCustomer = DOS_FALSE;

            /* 中断正在播放的标记背景音 */
            dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_break %s \r\n", pstSCB->szUUID);
            sc_ep_esl_execute_cmd(szCMD);
        }

        /* 判断是否是长签 */
        if (!sc_call_check_service(pstSCB, SC_SERV_AGENT_SIGNIN))
        {
            sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
        }
        else
        {
#if 0
            if (pstSCB->bIsInMarkState)
            {
                /* 放音 */
                sc_ep_esl_execute("sleep", "500", pstSCB->szUUID);
                sc_ep_play_sound(SC_SND_MUSIC_SIGNIN, pstSCB->szUUID, 1);
            }
#endif
        }

        pstSCB->bIsInMarkState = DOS_FALSE;

        return DOS_SUCC;
    }
    else
    {
        sc_logr_info(pstSCB, SC_ACD, "POTS, Not matched any action : %s", pszDealNum);

        if (bIsSecondaryDialing)
        {
            bIsHangUp = DOS_FALSE;
        }
    }

end:
    sc_logr_debug(pstSCB, SC_ACD, "POTS, result : %d, callee : %s", ulRet, pszDealNum);

    if (ulRet != DOS_SUCC)
    {
        pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
        if (SC_SCB_IS_VALID(pstSCBOther) && pstSCBOther->ucStatus != SC_SCB_RELEASE)
        {
            /* 在通话过程中按键，如果输入错误，就不要干预呼叫了 */
        }
        else
        {
            sc_ep_play_sound(SC_SND_OPT_FAIL, pstSCB->szUUID, 1);
        }
    }
    else
    {
        if (ulNeedTip)
        {
            sc_ep_play_sound(SC_SND_OPT_SUCC, pstSCB->szUUID, 1);
        }
    }

    if (bIsHangUp)
    {
        sc_ep_esl_execute("hangup", "", pstSCB->szUUID);
    }

    return ulRet;
}

U32 sc_ep_agent_signin_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    U32       ulRet = 0;
    S8        *pszPlaybackMS = 0;
    S8        *pszPlaybackTimeout = 0;
    U32       ulPlaybackMS, ulPlaybackTimeout;
    SC_ACD_AGENT_INFO_ST stAgentInfo;

    if (DOS_ADDR_INVALID(pstHandle) || DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (U32_BUTT == pstSCB->ulAgentID)
    {
        sc_logr_notice(pstSCB, SC_ESL, "%s", "Proc the signin msg. But there is no agent.");

        return DOS_FAIL;
    }


    if (sc_acd_get_agent_by_id(&stAgentInfo, pstSCB->ulAgentID) != DOS_SUCC)
    {
        sc_logr_notice(pstSCB, SC_ESL, "Cannot find the agent with the id %u.", pstSCB->ulAgentID);

        return DOS_FAIL;
    }

    sc_logr_info(pstSCB, SC_ESL, "Start process signin msg. Agent: %u, Need Connect: %s, Connected: %s Status: %u"
                    , stAgentInfo.ulSiteID
                    , stAgentInfo.bNeedConnected ? "Yes" : "No"
                    , stAgentInfo.bConnected ? "Yes" : "No"
                    , stAgentInfo.ucStatus);

    if (stAgentInfo.bNeedConnected && !stAgentInfo.bConnected)
    {
        sc_logr_debug(pstSCB, SC_ESL, "Agent signin. Agent: %u, Need Connect: %s, Connected: %s Status: %u"
                        , stAgentInfo.ulSiteID
                        , stAgentInfo.bNeedConnected ? "Yes" : "No"
                        , stAgentInfo.bConnected ? "Yes" : "No"
                        , stAgentInfo.ucStatus);

        /* 坐席需要长签，但还没有长签成功, 这个地方就是他长签成功的流程 */
        ulRet = sc_acd_update_agent_status(SC_ACD_SITE_ACTION_CONNECTED, pstSCB->ulAgentID, OPERATING_TYPE_PHONE);
        if (ulRet == DOS_SUCC)
        {
            sc_acd_update_agent_info(pstSCB, SC_ACD_BUTT, pstSCB->usSCBNo, NULL);
        }

        sc_ep_esl_execute("set", "mark_customer=true", pstSCB->szUUID);
        sc_ep_esl_execute("set", "exec_after_bridge_app=park", pstSCB->szUUID);

        /* 播放等待音 */
        sc_ep_esl_execute("set", "playback_terminators=none", pstSCB->szUUID);
        if (sc_ep_play_sound(SC_SND_MUSIC_SIGNIN, pstSCB->szUUID, 1) != DOS_SUCC)
        {
            sc_logr_info(pstSCB, SC_ESL, "%s", "Cannot find the music on hold. just park the call.");
        }
    }
    else
    {
        /* 这个地方则是长签成功之后的业务处理流程 */

        /* 坐席还处于IDEL就直接继续放音就好 */
        if (SC_ACD_IDEL == stAgentInfo.ucStatus)
        {
            sc_logr_debug(pstSCB, SC_ESL, "Agent signin and playback timeout. Agent: %u, Need Connect: %s, Connected: %s Status: %u, sip call: %u, marker:%u"
                            , stAgentInfo.ulSiteID
                            , stAgentInfo.bNeedConnected ? "Yes" : "No"
                            , stAgentInfo.bConnected ? "Yes" : "No"
                            , stAgentInfo.ucStatus
                            , pstSCB->bCallSip
                            , pstSCB->bIsInMarkState);

            if (pstSCB->bCallSip)
            {
                /* 外部电话正在呼叫坐席绑定的分机 */
                pstSCB->bCallSip = DOS_FALSE;
            }
            else
            {
                /* 播放等待音 */
                if (!pstSCB->bIsInMarkState)
                {
                    sc_ep_esl_execute("set", "playback_terminators=none", pstSCB->szUUID);
                    if (sc_ep_play_sound(SC_SND_MUSIC_SIGNIN, pstSCB->szUUID, 1) != DOS_SUCC)
                    {
                        sc_logr_info(pstSCB, SC_ESL, "%s", "Cannot find the music on hold. just park the call.");
                    }
                    sc_ep_esl_execute("set", "playback_terminators=*", pstSCB->szUUID);

                    pstSCB->bIsInMarkState= DOS_FALSE;
                }
            }
        }
        else if (SC_ACD_BUSY == stAgentInfo.ucStatus)
        {
            /* 这个地方，坐席已经在处理业务了 */
            /* DO Nothing */
        }
        else if (SC_ACD_PROC == stAgentInfo.ucStatus)
        {
            /* 不处理PARK消息 */
            if (ESL_EVENT_CHANNEL_PARK == pstEvent->event_id)
            {
                goto proc_succ;
            }

            /* 处理呼叫结果过程 */
            /* 放音，然后接受案件 */
            sc_logr_debug(pstSCB, SC_ESL, "Agent signin and a call just huangup. Agent: %u, Need Connect: %s, Connected: %s Status: %u"
                            , stAgentInfo.ulSiteID
                            , stAgentInfo.bNeedConnected ? "Yes" : "No"
                            , stAgentInfo.bConnected ? "Yes" : "No"
                            , stAgentInfo.ucStatus);

            /* 标记完成，修改坐席的状态 */
            if (pstSCB->bIsInMarkState)
            {
                /* 判断是否因为超时进入该流程 */
                pszPlaybackMS = esl_event_get_header(pstEvent, "variable_playback_ms");
                pszPlaybackTimeout = esl_event_get_header(pstEvent, "timeout");
                if (DOS_ADDR_VALID(pszPlaybackMS) && DOS_ADDR_VALID(pszPlaybackTimeout)
                    && dos_atoul(pszPlaybackMS, &ulPlaybackMS) >= 0
                    && dos_atoul(pszPlaybackTimeout, &ulPlaybackTimeout) >= 0
                    && ulPlaybackMS >= ulPlaybackTimeout)
                {
                    /* 标记超时，挂断电话, 将客户的标记值更改为 '*#'， 表示没有进行客户标记 */
                    pstSCB->szCustomerMark[0] = '\0';
                    dos_strcpy(pstSCB->szCustomerMark, "*#");
                    pstSCB->usOtherSCBNo = U16_BUTT;

                    /* 修改坐席的状态 */
                    sc_acd_update_agent_info(pstSCB, SC_ACD_IDEL, pstSCB->usSCBNo, NULL);
                    sc_ep_esl_execute("set", "playback_terminators=none", pstSCB->szUUID);
                    if (sc_ep_play_sound(SC_SND_MUSIC_SIGNIN, pstSCB->szUUID, 1) != DOS_SUCC)
                    {
                        sc_logr_info(pstSCB, SC_ESL, "%s", "Cannot find the music on hold. just park the call.");
                    }
                    sc_ep_esl_execute("set", "playback_terminators=*", pstSCB->szUUID);
                }

                pstSCB->bIsInMarkState= DOS_FALSE;
            }
            else
            {
                DOS_ASSERT(0);
            }
        }
    }

proc_succ:

    return DOS_SUCC;
}

S8 *sc_ep_call_type(U32 ulCallType)
{
    if (ulCallType == SC_DIRECTION_PSTN)
    {
        return "PSTN";
    }
    else if (ulCallType == SC_DIRECTION_SIP)
    {
        return "SIP";
    }
    else
    {
        return "INVALID";
    }
}

/**
 * 函数: U32 sc_ep_channel_park_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理ESL的CHANNEL PARK事件
 * 参数:
 *      esl_handle_t *pstHandle : 发送句柄
 *      esl_event_t *pstEvent   : 时间
 *      SC_SCB_ST *pstSCB       : SCB
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_channel_park_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8        *pszIsAutoCall = NULL;
    S8        *pszCaller     = NULL;
    S8        *pszCallee     = NULL;
    S8        *pszUUID       = NULL;
    S8        *pszMainService = NULL;
    S8        *pszValue       = NULL;
    S8        *pszDisposition = NULL;
    U32       ulCallSrc, ulCallDst;
    U32       ulRet = DOS_SUCC;
    U32       ulMainService = U32_BUTT;
    U64       uLTmp = DOS_SUCC;
    SC_SCB_ST *pstSCBOther  = NULL, *pstSCBNew = NULL;
    SC_ACD_AGENT_INFO_ST stAgentData;

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_logr_debug(pstSCB, SC_ESL, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    /*  1.申请控制块
     *  2.判断是否是自动外呼
     *    如果是自动外呼: 就使用originate命令发起呼叫
     *  3.星业务
     *    执行相应业务
     *  4.普通呼叫
     *    查找路由，找中级组，或者对应的SIP分机呼叫
     */

    /* 获取事件的UUID */
    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (pstSCB->bParkHack)
    {
        pstSCB->bParkHack = DOS_FALSE;
        ulRet = DOS_SUCC;
        goto proc_finished;
    }

    /* 业务控制 */
    pszIsAutoCall = esl_event_get_header(pstEvent, "variable_auto_call");
    pszCaller     = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    pszCallee     = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    pszMainService= esl_event_get_header(pstEvent, "variable_main_service");
    if (DOS_ADDR_INVALID(pszMainService)
        || dos_atoul(pszMainService, &ulMainService) < 0)
    {
        ulMainService = U32_BUTT;
    }

    sc_logr_info(pstSCB, SC_ESL, "Route Call Start: Auto Call Flag: %s, Caller: %s, Callee: %s, pstSCB : %u, Main Service: %s"
                , NULL == pszIsAutoCall ? "NULL" : pszIsAutoCall
                , NULL == pszCaller ? "NULL" : pszCaller
                , NULL == pszCallee ? "NULL" : pszCallee, pstSCB->usSCBNo
                , NULL == pszMainService ? "NULL" : pszMainService);

    pszValue = esl_event_get_header(pstEvent, "variable_will_hangup");
    if (DOS_ADDR_VALID(pszValue))
    {
        /* 将要挂断, 不用操作 */
        sc_call_trace(pstSCB, "Call will be hangup. no need process");

        goto proc_finished;
    }

    /* 判断是否是 新业务 */
    if (pstSCB->szCalleeNum[0] == '*')
    {
        sc_ep_esl_execute("answer", "", pszUUID);

        sc_call_trace(pstSCB, "process * service. number: %s", pstSCB->szCalleeNum);

        ulRet = sc_ep_pots_pro(pstSCB, pstEvent, DOS_FALSE);

        goto proc_finished;
    }


    pszValue = esl_event_get_header(pstEvent, "variable_begin_to_transfer");
#if 0
    if (DOS_ADDR_VALID(pszValue)
        && dos_strcmp(pszValue, "true") == 0)
    {
        /* 转接时，第二方挂断电话后, 不要挂断第三方, bridge 第一方和第三方 */

        ulRet = DOS_SUCC;

        sc_call_trace(pstSCB, "transfer notify is hangup.");

        goto proc_finished;
    }
#endif


    if (pstSCB->bTransWaitingBridge)
    {
        pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
        if (DOS_ADDR_VALID(pstSCBOther))
        {
            S8 szCMDBuffer[128] = { 0, };
            dos_snprintf(szCMDBuffer, sizeof(szCMDBuffer), "bgapi uuid_break %s all \r\n", pstSCB->szUUID);
            sc_ep_esl_execute_cmd(szCMDBuffer);

            dos_snprintf(szCMDBuffer, sizeof(szCMDBuffer), "bgapi uuid_break %s all \r\n", pstSCBOther->szUUID);
            sc_ep_esl_execute_cmd(szCMDBuffer);

            dos_snprintf(szCMDBuffer, sizeof(szCMDBuffer), "bgapi uuid_bridge %s %s \r\n", pstSCB->szUUID, pstSCBOther->szUUID);
            sc_ep_esl_execute_cmd(szCMDBuffer);
        }

        pstSCB->bTransWaitingBridge = DOS_FALSE;

        goto proc_finished;
    }

    if (pstSCB->bIsAgentCall == DOS_TRUE
        && pstSCB->bIsMarkCustomer == DOS_FALSE)
    {
        pszValue = esl_event_get_header(pstEvent, "variable_mark_customer");
        if (DOS_ADDR_VALID(pszValue)
            && dos_strcmp(pszValue, "true") == 0)
        {
            /* 客户标记, 客户一侧挂断电话后，坐席一侧保持，进入客户标记状态 */
            pstSCB->usOtherSCBNo = U16_BUTT;
            ulRet = DOS_SUCC;

            pszValue = esl_event_get_header(pstEvent, "variable_customer_num");
            if (DOS_ADDR_VALID(pszValue))
            {
                dos_strncpy(pstSCB->szCustomerNum, pszValue, SC_TEL_NUMBER_LENGTH);
                pstSCB->szCustomerNum[SC_TEL_NUMBER_LENGTH-1] = '\0';
            }

            sc_call_trace(pstSCB, "Agent is in marker statue.");

            goto proc_finished;
        }
    }

    pszDisposition = esl_event_get_header(pstEvent, "variable_endpoint_disposition");
    if (DOS_ADDR_VALID(pszDisposition))
    {
        if (dos_strnicmp(pszDisposition, "EARLY MEDIA", dos_strlen("EARLY MEDIA")) == 0)
        {
            pstSCB->bHasEarlyMedia = DOS_TRUE;
        }
    }
    else
    {
        ulRet = DOS_SUCC;

        sc_logr_info(pstSCB, SC_ESL, "Recv park without disposition give up to process. UUID: %s", pstSCB->szUUID);

        goto proc_finished;
    }

    if (pstSCB->bIsTTCall)
    {
       sc_ep_esl_execute("unset", "sip_h_EixTTcall", pstSCB->szUUID);
       sc_ep_esl_execute("unset", "sip_h_Mime-version", pstSCB->szUUID);
    }

    if (SC_SERV_CALL_INTERCEPT == ulMainService
            || SC_SERV_CALL_WHISPERS == ulMainService)
    {
        if (!pstSCB->bHasEarlyMedia)
        {
            ulRet = sc_ep_call_intercept(pstSCB);
        }
        else
        {
            ulRet = DOS_SUCC;
        }
    }
    else if (SC_SERV_ATTEND_TRANSFER == ulMainService
        || SC_SERV_BLIND_TRANSFER == ulMainService)
    {
        sc_logr_info(pstSCB, SC_ESL, "Park for trans. role: %u, service: %u", pstSCB->ucTranforRole, ulMainService);

        if (!pstSCB->bHasEarlyMedia)
        {
            if (SC_TRANS_ROLE_SUBSCRIPTION == pstSCB->ucTranforRole)
            {
                ulRet = sc_ep_transfer_exec(pstSCB, ulMainService);
            }
            else if (SC_TRANS_ROLE_PUBLISH == pstSCB->ucTranforRole)
            {
                pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
                if (DOS_ADDR_VALID(pstSCBOther) && pstSCBOther->ucStatus <= SC_SCB_ACTIVE)
                {
                    ulRet = sc_ep_transfer_publish_active(pstSCB, ulMainService);
                }
                else
                {
                    sc_logr_info(pstSCB, SC_ESL, "%s", "Park for trans role publish with the scb status release.");
                    ulRet = DOS_SUCC;
                }
            }
            else
            {
                if (SC_SERV_BLIND_TRANSFER == ulMainService)
                {
                    ulRet = sc_ep_transfer_publish_active(pstSCB, ulMainService);
                }
                else
                {
                    sc_logr_info(pstSCB, SC_ESL, "%s", "Park for trans role notify");
                    ulRet = DOS_SUCC;
                }
            }
        }
        else
        {
            ulRet = DOS_SUCC;
        }
    }
    else if (SC_SERV_AGENT_SIGNIN == ulMainService)
    {
        if (!pstSCB->bHasEarlyMedia)
        {
            //sc_ep_esl_execute("answer", "", pstSCB->szUUID);
            if (pstSCB->ulSiginTimeStamp == 0)
            {
                pszValue = esl_event_get_header(pstEvent, "Caller-Channel-Answered-Time");
                if (DOS_ADDR_VALID(pszValue)
                    && '\0' != pszValue[0]
                    && dos_atoull(pszValue, &uLTmp) == 0)
                {
                    pstSCB->ulSiginTimeStamp = uLTmp / 1000000;
                    sc_logr_debug(pstSCB, SC_ESL, "Get extra data: Caller-Channel-Answered-Time=%s(%u)", pszValue, pstSCB->ulSiginTimeStamp);
                }
            }
            /* 坐席长签成功 */
            ulRet = sc_ep_agent_signin_proc(pstHandle, pstEvent, pstSCB);
        }
        else
        {
            ulRet = DOS_SUCC;
        }
    }
    else if (SC_SERV_PREVIEW_DIALING == ulMainService)
    {
        if (!pstSCB->bHasEarlyMedia)
        {
            ulRet = sc_ep_call_agent_by_id(pstSCB, pstSCB->ulAgentID, DOS_TRUE, DOS_FALSE);
        }
        else
        {
            ulRet = DOS_SUCC;
        }
    }
    else if (SC_SERV_AGENT_CLICK_CALL == ulMainService)
    {
        if (!pstSCB->bHasEarlyMedia)
        {
            if (pstSCB->bIsAgentCall)
            {
                if (pstSCB->enCallCtrlType == SC_CALL_CTRL_TYPE_AGENT)
                {
                    /* 呼叫另一个坐席 */
                    sc_ep_call_agent_by_id(pstSCB, pstSCB->ulOtherAgentID, DOS_FALSE, DOS_FALSE);
                }
                else if (pstSCB->enCallCtrlType == SC_CALL_CTRL_TYPE_SIP)
                {
                    /* 呼叫一个分机 */
                    pstSCBNew = sc_scb_alloc();
                    if (DOS_ADDR_INVALID(pstSCB))
                    {
                        sc_logr_warning(pstSCB, SC_ESL, "%s", "Cannot make call for the API CMD. Alloc SCB FAIL.");
                        ulRet = DOS_FAIL;
                        DOS_ASSERT(0);
                        goto proc_finished;
                    }

                    pstSCBNew->ulCustomID = pstSCB->ulCustomID;
                    pstSCBNew->ulAgentID = pstSCB->ulAgentID;
                    pstSCBNew->ulTaskID = pstSCB->ulTaskID;
                    pstSCBNew->usOtherSCBNo = pstSCB->usSCBNo;
                    pstSCB->usOtherSCBNo = pstSCBNew->usSCBNo;

                    dos_strncpy(pstSCBNew->szSiteNum, pstSCB->szSiteNum, sizeof(pstSCBNew->szSiteNum));
                    pstSCBNew->szSiteNum[sizeof(pstSCBNew->szSiteNum) - 1] = '\0';

                    /* 指定主被叫号码 */
                    dos_strncpy(pstSCBNew->szCalleeNum, pstSCB->szCallerNum, sizeof(pstSCBNew->szCalleeNum));
                    pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';
                    dos_strncpy(pstSCBNew->szCallerNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCallerNum));
                    pstSCBNew->szCallerNum[sizeof(pstSCBNew->szCallerNum) - 1] = '\0';

                    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
                    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_INTERNAL_CALL);

                    if (sc_dial_make_call2ip(pstSCBNew, SC_SERV_BUTT, DOS_FALSE) != DOS_SUCC)
                    {
                        goto proc_finished;
                    }
                }
                else
                {
                    /* 呼叫客户 */
                    pstSCBNew = sc_scb_alloc();
                    if (DOS_ADDR_INVALID(pstSCB))
                    {
                        sc_logr_warning(pstSCB, SC_ESL, "%s", "Cannot make call for the API CMD. Alloc SCB FAIL.");
                        ulRet = DOS_FAIL;
                        DOS_ASSERT(0);
                        goto proc_finished;
                    }

                    pstSCBNew->ulCustomID = pstSCB->ulCustomID;
                    pstSCBNew->ulAgentID = pstSCB->ulAgentID;
                    pstSCBNew->ulTaskID = pstSCB->ulTaskID;
                    pstSCBNew->usOtherSCBNo = pstSCB->usSCBNo;
                    pstSCB->usOtherSCBNo = pstSCBNew->usSCBNo;

                    dos_strncpy(pstSCBNew->szSiteNum, pstSCB->szSiteNum, sizeof(pstSCBNew->szSiteNum));
                    pstSCBNew->szSiteNum[sizeof(pstSCBNew->szSiteNum) - 1] = '\0';

                    /* 指定被叫号码 */
                    dos_strncpy(pstSCBNew->szCalleeNum, pstSCB->szCallerNum, sizeof(pstSCBNew->szCalleeNum));
                    pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';

                    /* 呼叫 PSTN 时，会从主叫号码组中获取一个主叫号码，这里就不需要获取了 */
                    dos_strncpy(pstSCBNew->szCallerNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCallerNum));
                    pstSCBNew->szCallerNum[sizeof(pstSCBNew->szCallerNum) - 1] = '\0';
#if 0
                    /* 指定主叫号码 号码组 */
                    if (sc_caller_setting_select_number(pstSCBNew->ulCustomID, pstSCBNew->ulAgentID, SC_SRC_CALLER_TYPE_AGENT, pstSCBNew->szCallerNum, sizeof(pstSCBNew->szCallerNum)) != DOS_SUCC)
                    {
                        sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Get caller fail by agent(%u). ", pstSCBNew->ulAgentID);
                        sc_scb_free(pstSCBNew);
                        pstSCB->usOtherSCBNo = U16_BUTT;
                        sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_CALLER_NUMBER_ILLEGAL);

                        goto proc_finished;
                    }
#endif
                    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_PREVIEW_DIALING);

                    if (!sc_ep_black_list_check(pstSCBNew->ulCustomID, pstSCBNew->szCalleeNum))
                    {
                        DOS_ASSERT(0);

                        sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Callee in blocak list. (%s)", pszCallee);
                        ulRet = DOS_FAIL;
                        goto proc_finished;
                    }

                    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
                    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_EXTERNAL_CALL);
                    if (sc_send_usr_auth2bs(pstSCBNew) != DOS_SUCC)
                    {
                        DOS_ASSERT(0);

                        sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Send auth fail.", pszCallee);
                        ulRet = DOS_FAIL;

                        sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_MESSAGE_SENT_ERR);
                        goto proc_finished;
                    }
                }
            }
        }
        else
        {
            ulRet = DOS_SUCC;
        }
    }
    /* 如果是AUTO Call就不需要创建SCB，将SCB同步到HASH表中就好 */
    else if (SC_SERV_AUTO_DIALING == ulMainService)
    {
        /* 自动外呼处理 */
        if (!pstSCB->bHasEarlyMedia)
        {
            ulRet = sc_ep_auto_dial_proc(pstHandle, pstEvent, pstSCB);
        }
        else
        {
            ulRet = DOS_SUCC;
        }
    }
    else if (SC_SERV_DEMO_TASK == ulMainService)
    {
        /* 群呼任务演示 */
        if (!pstSCB->bHasEarlyMedia)
        {
            ulRet = sc_ep_demo_task_proc(pstHandle, pstEvent, pstSCB);
        }
        else
        {
            ulRet = DOS_SUCC;
        }
    }
    else if (SC_SERV_NUM_VERIFY == ulMainService)
    {
        if (!pstSCB->bHasEarlyMedia)
        {
            ulRet = sc_ep_num_verify(pstHandle, pstEvent, pstSCB);
        }
        else
        {
            ulRet = DOS_SUCC;
        }
    }
    /* 如果是回呼到坐席的呼叫。就需要连接客户和坐席 */
    else if (SC_SERV_AGENT_CALLBACK == ulMainService
        || SC_SERV_OUTBOUND_CALL == ulMainService)
    {
        S8 szCMDBuff[512] = { 0, };

        /* 如果在标记状态，就不处理了 */
        if (pstSCB->bIsInMarkState)
        {
            ulRet = DOS_SUCC;
            goto proc_finished;
        }

        pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
        if (DOS_ADDR_INVALID(pstSCBOther))
        {
            DOS_ASSERT(0);

            sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_SYSTEM_ABNORMAL);
            ulRet = DOS_FAIL;

            goto proc_finished;
        }

        /* 不增加answer，听不到声音 */
        sc_ep_esl_execute("answer", NULL, pstSCBOther->szUUID);
        /* 如果命令执行失败，就需要挂断另外一通呼叫 */
        dos_snprintf(szCMDBuff, sizeof(szCMDBuff), "bgapi uuid_bridge %s %s \r\n", pstSCB->szUUID, pstSCBOther->szUUID);
        pstSCBOther->usOtherSCBNo= pstSCB->usSCBNo;
        pstSCB->usOtherSCBNo = pstSCBOther->usSCBNo;

        if (sc_ep_esl_execute_cmd(szCMDBuff) != DOS_SUCC)
        {
            sc_ep_hangup_call_with_snd(pstSCBOther, CC_ERR_SC_SYSTEM_ABNORMAL);
            sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_SYSTEM_ABNORMAL);
            ulRet = DOS_FAIL;

            goto proc_finished;

        }

        SC_SCB_SET_STATUS(pstSCB, SC_SCB_ACTIVE);

        sc_logr_info(pstSCB, SC_ESL, "Agent has been connected. UUID: %s <> %s. SCBNo: %d <> %d."
                     , pstSCB->szUUID, pstSCBOther->szUUID
                     , pstSCB->usSCBNo, pstSCBOther->usSCBNo);
    }
    else if (sc_ep_internal_service_check(pstEvent) != SC_INTER_SRV_BUTT)
    {
        /* 接听主叫方呼叫 */
        sc_ep_esl_execute("answer", NULL, pszUUID);

        /* 内部业务处理 */
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INBOUND_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_SERVICE);

        ulRet = sc_ep_internal_service_proc(pstHandle, pstEvent, pstSCB);
    }
    else
    {
        /* 正常呼叫处理 */
        pstSCB->ucLegRole = SC_CALLER;
        ulCallSrc = sc_ep_get_source(pstEvent, pstSCB);
        ulCallDst = sc_ep_get_destination(pstEvent, pstSCB);
        sc_log_digest_print("source type is %s, destination type is %s"
            , sc_ep_call_type(ulCallSrc), sc_ep_call_type(ulCallDst));

        /* 获得ulCustomID */
        if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_PSTN == ulCallDst)
        {
            /* TT 号发起呼叫时，ulCustomID已经获得，这里不需要再获得了 */
            if (!pstSCB->bIsCallerInTT)
            {
                pstSCB->ulCustomID = sc_ep_get_custom_by_sip_userid(pstSCB->szCallerNum);
            }
        }
        else if (SC_DIRECTION_PSTN == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
        {
            pstSCB->ulCustomID = sc_ep_get_custom_by_did(pstSCB->szCalleeNum);
        }
        else if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
        {
            pstSCB->ulCustomID = sc_ep_get_custom_by_sip_userid(pszCaller);
        }
        else
        {
            DOS_ASSERT(0);
            sc_logr_info(pstSCB, SC_ESL, "Invalid call source or destension. Source: %d, Dest: %d", ulCallSrc, ulCallDst);

            sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_CUSTOMERS_NOT_EXIST);

            ulRet = DOS_FAIL;

            goto proc_finished;
        }

        /* 判断SIP是否属于企业 */
        if (sc_ep_customer_list_find(pstSCB->ulCustomID) != DOS_SUCC)
        {
            sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_NO_SERV_RIGHTS);

            SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);
            ulRet = DOS_FAIL;
            sc_logr_info(pstSCB, SC_ESL, "Invalid call source or destension. Source: %d, Dest: %d, CustomID(%u) is not vest in enterprise", ulCallSrc, ulCallDst, pstSCB->ulCustomID);

            goto proc_finished;
        }

        sc_logr_info(pstSCB, SC_ESL, "Get call source and dest. Source: %d, Dest: %d", ulCallSrc, ulCallDst);

        if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_PSTN == ulCallDst)
        {
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INBOUND_CALL);
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_CALL);

            /* 更改不同的主叫，获取当前呼叫时哪一个客户 */
            if (U32_BUTT != pstSCB->ulCustomID)
            {
                if (!pstSCB->bIsCallerInTT)
                {
                    /* 根据SIP，找到坐席，将SCB的usSCBNo, 绑定到坐席上 */
                    if (sc_acd_update_agent_scbno_by_userid(pstSCB->szCallerNum, pstSCB, &stAgentData, pstSCB->szCalleeNum) != DOS_SUCC)
                    {
                        sc_logr_info(pstSCB, SC_ESL, "Not found agent by sip(%s). SCBNO : %d", pstSCB->szCallerNum, pstSCB->usSCBNo);
                    }
                    else
                    {
                        /* 绑定了坐席，要进行客户标记 */
                        sc_logr_debug(pstSCB, SC_ESL, "update agent SCBNO SUCC. sip : %s, SCBNO : %d", pstSCB->szCallerNum, pstSCB->usSCBNo);
                        sc_ep_esl_execute("set", "exec_after_bridge_app=park", pstSCB->szUUID);
                        sc_ep_esl_execute("set", "mark_customer=true", pstSCB->szUUID);
                        pstSCB->bIsAgentCall = DOS_TRUE;
                        sc_acd_agent_stat(SC_AGENT_STAT_CALL, stAgentData.ulSiteID, NULL, 0);
                        sc_ep_call_notify(&stAgentData, pstSCB->szCalleeNum);
                    }
                }
                else
                {
                    /* 根据SIP，找到坐席，将SCB的usSCBNo, 绑定到坐席上 */
                    if (sc_acd_update_agent_scbno_by_siteid(pstSCB->ulAgentID, pstSCB, &stAgentData, pstSCB->szCalleeNum) != DOS_SUCC)
                    {
                        sc_logr_info(pstSCB, SC_ESL, "Not found agent by sip(%s). SCBNO : %d", pstSCB->szCallerNum, pstSCB->usSCBNo);
                    }
                    else
                    {
                        /* 绑定了坐席，要进行客户标记 */
                        sc_logr_debug(pstSCB, SC_ESL, "update agent SCBNO SUCC. sip : %s, SCBNO : %d", pstSCB->szCallerNum, pstSCB->usSCBNo);
                        sc_ep_esl_execute("set", "exec_after_bridge_app=park", pstSCB->szUUID);
                        sc_ep_esl_execute("set", "mark_customer=true", pstSCB->szUUID);
                        pstSCB->bIsAgentCall = DOS_TRUE;
                        sc_ep_call_notify(&stAgentData, pstSCB->szCalleeNum);
                    }
                }

                if (sc_ep_outgoing_call_proc(pstSCB) != DOS_SUCC)
                {
                    SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);
                    ulRet = DOS_FAIL;
                }
                else
                {
                    SC_SCB_SET_STATUS(pstSCB, SC_SCB_EXEC);
                }
            }
            else
            {
                SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);

                sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_CUSTOMERS_NOT_EXIST);
                ulRet = DOS_FAIL;
            }
        }
        else if (SC_DIRECTION_PSTN == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
        {
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INBOUND_CALL);
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL);

            if (pstSCB->ulCustomID != U32_BUTT)
            {
                if (sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
                {
                    sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_MESSAGE_SENT_ERR);

                    SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);
                    ulRet = DOS_FAIL;
                }
                else
                {
                    SC_SCB_SET_STATUS(pstSCB, SC_SCB_AUTH);
                }
            }
            else
            {
                DOS_ASSERT(0);

                sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_CUSTOMERS_NOT_EXIST);

                SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);
                ulRet = DOS_FAIL;
            }
        }
        else if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
        {
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_CALL);
            /* 根据SIP，找到坐席，将SCB的usSCBNo, 绑定到坐席上 */
            if (sc_acd_update_agent_scbno_by_userid(pszCaller, pstSCB, &stAgentData, NULL) != DOS_SUCC)
            {
                sc_logr_debug(pstSCB, SC_ESL, "Not found agent by sip(%s). SCBNO : %d", pszCaller, pstSCB->usSCBNo);
            }
            else
            {
                sc_logr_debug(pstSCB, SC_ESL, "update agent SCBNO SUCC. sip : %s, SCBNO : %d", pszCaller, pstSCB->usSCBNo);
            }

            ulRet = sc_ep_internal_call_process(pstHandle, pstEvent, pstSCB);
        }
        else
        {
            DOS_ASSERT(0);
            sc_logr_info(pstSCB, SC_ESL, "Invalid call source or destension. Source: %d, Dest: %d", ulCallSrc, ulCallDst);

            sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_CUSTOMERS_NOT_EXIST);
            ulRet = DOS_FAIL;
        }
    }

proc_finished:
    sc_call_trace(pstSCB, "Finished to process %s event. Result : %s"
                    , esl_event_get_header(pstEvent, "Event-Name")
                    , (DOS_SUCC == ulRet) ? "OK" : "FAILED");

    return ulRet;
}

U32 sc_ep_backgroud_job_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent)
{
    S8    *pszEventBody   = NULL;
    S8    *pszAppName     = NULL;
    S8    *pszAppArg      = NULL;
    S8    *pszStart       = NULL;
    S8    *pszEnd         = NULL;
    S8    szSCBNo[16]      = { 0 };
    U32   ulProcessResult = DOS_SUCC;
    U32   ulSCBNo    = 0;
    SC_SCB_ST   *pstSCB = NULL;
    SC_SCB_ST   *pstOtherSCB = NULL;

    if (DOS_ADDR_INVALID(pstHandle)

        || DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pszEventBody = esl_event_get_body(pstEvent);
    pszAppName   = esl_event_get_header(pstEvent, "Job-Command");
    pszAppArg    = esl_event_get_header(pstEvent, "Job-Command-Arg");
    if (DOS_ADDR_VALID(pszEventBody)
        && DOS_ADDR_VALID(pszAppName)
        && DOS_ADDR_VALID(pszAppArg))
    {
        if (0 == dos_strnicmp(pszEventBody, "+OK", dos_strlen("+OK")))
        {
            ulProcessResult = DOS_SUCC;
        }
        else
        {
            ulProcessResult = DOS_FAIL;
        }

        sc_logr_info(pstSCB, SC_ESL, "Execute command %s %s %s, Info: %s."
                        , pszAppName
                        , pszAppArg
                        , DOS_SUCC == ulProcessResult ? "SUCC" : "FAIL"
                        , pszEventBody);

        if (DOS_SUCC == ulProcessResult)
        {
            goto process_finished;
        }

        pszStart = dos_strstr(pszAppArg, "scb_number=");
        if (DOS_ADDR_INVALID(pszStart))
        {
            DOS_ASSERT(0);
            goto process_fail;
        }

        pszStart += dos_strlen("scb_number=");
        if (DOS_ADDR_INVALID(pszStart))
        {
            DOS_ASSERT(0);
            goto process_fail;
        }

        pszEnd = dos_strstr(pszStart, ",");
        if (DOS_ADDR_VALID(pszEnd))
        {
            dos_strncpy(szSCBNo, pszStart, pszEnd-pszStart);
            szSCBNo[pszEnd-pszStart] = '\0';
        }
        else
        {
            dos_strncpy(szSCBNo, pszStart, sizeof(szSCBNo));
            szSCBNo[sizeof(szSCBNo)-1] = '\0';
        }

        if (dos_atoul(szSCBNo, &ulSCBNo) < 0)
        {
            DOS_ASSERT(0);
            goto process_fail;
        }

        pstSCB = sc_scb_get(ulSCBNo);
        if (DOS_ADDR_VALID(pstSCB))
        {
            pstOtherSCB = sc_scb_get(pstSCB->usOtherSCBNo);
        }

        if (dos_stricmp(pszAppName, "originate") == 0)
        {
            if (DOS_ADDR_VALID(pstOtherSCB))
            {
                /* TODO !!!! 这个地方需要视呼叫业务进行处理 如果是转接，还是要判断一下的 */
                if (sc_call_check_service(pstSCB, SC_SERV_ATTEND_TRANSFER)
                    || sc_call_check_service(pstSCB, SC_SERV_BLIND_TRANSFER))
                {
                    //if (pstSCB->ucTranforRole == SC_TRANS_ROLE_PUBLISH)
                    {
                        goto process_finished;
                    }
                }

                sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
            }
            else
            {
                /* 如果还没有创建通道就释放控制块 */
                if ('\0' == pstSCB->szUUID[0])
                {
                    /* 呼叫失败了 */
                    DOS_ASSERT(0);

                    /* 记录错误码 */
                    pstSCB->usTerminationCause = sc_ep_transform_errcode_from_sc2sip(CC_ERR_SIP_BAD_GATEWAY);

                    /* 如果是群呼任务，就需要分析呼叫结果 */
                    if (pstSCB->ulTaskID != 0 && pstSCB->ulTaskID != U32_BUTT)
                    {
                        sc_ep_calltask_result(pstSCB, CC_ERR_SIP_BAD_GATEWAY);
                    }

                    /* 发送话单 */
                    if (sc_send_billing_stop2bs(pstSCB) != DOS_SUCC)
                    {
                        sc_logr_notice(pstSCB, SC_DIALER, "Send billing stop FAIL where make call fail. (SCB: %u)", pstSCB->usSCBNo);
                    }

                    sc_bg_job_hash_delete(pstSCB->usSCBNo);
                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                }
            }

            sc_logr_error(pstSCB, SC_ESL, "ERROR: BGJOB Fail.Argv: %s, SCB-NO: %s(%u)", pszAppArg, szSCBNo, ulSCBNo);
        }
    }

process_finished:
    return DOS_SUCC;

process_fail:
    return DOS_FAIL;
}


/**
 * 函数: U32 sc_ep_channel_create_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent)
 * 功能: 处理ESL的CHANNEL CREATE事件
 * 参数:
 *      esl_event_t *pstEvent   : 时间
 *      esl_handle_t *pstHandle : 发送句柄
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_channel_create_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent)
{
    S8          *pszUUID = NULL;
    S8          *pszMainService = NULL;
    S8          *pszSCBNum = NULL;
    S8          *pszOtherSCBNo = NULL;
    S8          *pszAgentID = NULL;
    SC_SCB_ST   *pstSCB = NULL;
    SC_SCB_ST   *pstSCB1 = NULL;
    S8          szBuffCmd[128] = { 0 };
    U32         ulSCBNo = 0;
    U32         ulOtherSCBNo = 0;
    U32         ulRet = DOS_SUCC;
    U32         ulMainService = U32_BUTT;
    U32         ulAgentID = 0;

    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_logr_debug(pstSCB, SC_ESL, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

#if 0
    dos_snprintf(szBuffCmd, sizeof(szBuffCmd), "bgapi uuid_park %s \r\n", pszUUID);
    sc_ep_esl_execute_cmd(szBuffCmd);
#endif

    pszMainService = esl_event_get_header(pstEvent, "variable_main_service");
    if (DOS_ADDR_INVALID(pszMainService)
        || dos_atoul(pszMainService, &ulMainService) < 0)
    {
        ulMainService = U32_BUTT;
    }

    /* 如果是AUTO Call就不需要创建SCB，将SCB同步到HASH表中就好 */
    pszSCBNum = esl_event_get_header(pstEvent, "variable_scb_number");
    if (DOS_ADDR_VALID(pszSCBNum))
    {
        if (dos_atoul(pszSCBNum, &ulSCBNo) < 0)
        {
            DOS_ASSERT(0);

            goto process_fail;
        }

        pstSCB = sc_scb_get(ulSCBNo);
        if (DOS_ADDR_INVALID(pstSCB))
        {
            DOS_ASSERT(0);

            goto process_fail;
        }

        if (pstSCB->bIsTTCall
            && pstSCB->usOtherSCBNo < SC_MAX_SCB_NUM)
        {
            /* 如果坐席绑定的是 EIX，需要给对端放回铃音 */
            pstSCB1 = sc_scb_get(pstSCB->usOtherSCBNo);
            if (DOS_ADDR_VALID(pstSCB1))
            {
                sc_ep_esl_execute("ring_ready", NULL, pstSCB1->szUUID);
            }
        }

        /* 判断是否是呼叫坐席，更新坐席的状态 */
        if (pstSCB->bIsAgentCall && !sc_call_check_service(pstSCB, SC_SERV_AGENT_SIGNIN))
        {
            sc_acd_update_agent_info(pstSCB, SC_ACD_BUSY, ulSCBNo, pstSCB->szCustomerNum);
        }

        /* 更新UUID */
        dos_strncpy(pstSCB->szUUID, pszUUID, sizeof(pstSCB->szUUID));
        pstSCB->szUUID[sizeof(pstSCB->szUUID) - 1] = '\0';

        sc_scb_hash_tables_add(pszUUID, pstSCB);

        if (sc_call_check_service(pstSCB, SC_SERV_AUTO_DIALING)
            && pstSCB->usTCBNo < SC_MAX_TASK_NUM)
        {
            sc_task_concurrency_add(pstSCB->usTCBNo);
        }
        else if (sc_call_check_service(pstSCB, SC_SERV_BLIND_TRANSFER)
            || sc_call_check_service(pstSCB, SC_SERV_ATTEND_TRANSFER))
        {
            /* 盲转，协商转 */
            dos_snprintf(szBuffCmd, sizeof(szBuffCmd), "begin_to_transfer=true");
            sc_ep_esl_execute("set", szBuffCmd, pstSCB->szUUID);
        }

        goto process_finished;

process_fail:
       ulRet = DOS_FAIL;
    }
    else
    {
        pstSCB = sc_scb_alloc();
        if (DOS_ADDR_INVALID(pstSCB))
        {
            DOS_ASSERT(0);
            sc_logr_error(pstSCB, SC_ESL, "%s", "Alloc SCB FAIL.");

            SC_TRACE_OUT();
            return DOS_FAIL;
        }

        sc_scb_hash_tables_add(pszUUID, pstSCB);
        sc_ep_parse_event(pstEvent, pstSCB);

        dos_strncpy(pstSCB->szUUID, pszUUID, sizeof(pstSCB->szUUID));
        pstSCB->szUUID[sizeof(pstSCB->szUUID) - 1] = '\0';

        /* 给通道设置变量 */
        dos_snprintf(szBuffCmd, sizeof(szBuffCmd), "scb_number=%u", pstSCB->usSCBNo);
        sc_ep_esl_execute("set", szBuffCmd, pszUUID);

        SC_SCB_SET_STATUS(pstSCB, SC_SCB_INIT);
    }

    /* 根据参数  交换SCB No */
    pszOtherSCBNo = esl_event_get_header(pstEvent, "variable_other_leg_scb");
    if (DOS_ADDR_INVALID(pszOtherSCBNo)
        || dos_atoul(pszOtherSCBNo, &ulOtherSCBNo) < 0)
    {
        goto process_finished;
    }

    pstSCB1 = sc_scb_get(ulOtherSCBNo);
    if (DOS_ADDR_VALID(pstSCB)
        && DOS_ADDR_VALID(pstSCB1))
    {
        pstSCB->usOtherSCBNo = pstSCB1->usSCBNo;
        pstSCB1->usOtherSCBNo = pstSCB->usSCBNo;
    }

    /* 根据参数update_agent，判断是否需要更新坐席中的 usSCBNo  */
    pszAgentID = esl_event_get_header(pstEvent, "variable_update_agent_id");
    if (DOS_ADDR_VALID(pszAgentID) && dos_atoul(pszAgentID, &ulAgentID) == 0)
    {
        /* 更改坐席的状态 坐席时登陆状态，坐席绑定的是sip分机 */
        pstSCB->bIsAgentCall = DOS_TRUE;
        if (ulAgentID == 0)
        {
            /* 不更新坐席的状态 */
            if (DOS_ADDR_VALID(pstSCB1))
            {
                pstSCB->ulAgentID = pstSCB1->ulAgentID;
            }
            pstSCB->bIsNotChangeAgentState = DOS_TRUE;
            goto process_finished;
        }

        if (sc_acd_update_agent_scbno_by_siteid(ulAgentID, pstSCB, NULL, NULL) != DOS_SUCC)
        {
            sc_logr_info(pstSCB, SC_ESL, "update(%u) agent SCBNO FAIL. SCBNO : %d!", ulAgentID, pstSCB->usSCBNo);
        }
        else
        {
            sc_logr_debug(pstSCB, SC_ESL, "update(%u) agent SCBNO SUCC. SCBNO : %d!", ulAgentID, pstSCB->usSCBNo);
        }
    }

process_finished:

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return ulRet;
}


/**
 * 函数: U32 sc_ep_channel_answer(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理ESL的CHANNEL EXECUTE COMPLETE事件
 * 参数:
 *      esl_handle_t *pstHandle : 发送句柄
 *      esl_event_t *pstEvent   : 时间
 *      SC_SCB_ST *pstSCB       : SCB
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_channel_answer(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8          *pszWaitingPark     = NULL;
    S8          *pszMainService     = NULL;
    S8          *pszCaller          = NULL;
    S8          *pszCallee          = NULL;
    S8          *pszValue           = NULL;
    SC_SCB_ST   *pstSCBRecord       = NULL;
    SC_SCB_ST   *pstSCBOther        = NULL;
    SC_SCB_ST   *pstSCBNew          = NULL;
    U32 ulMainService               = U32_BUTT;
    U32 ulRet  = DOS_SUCC;
    U64 uLTmp  = 0;


    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    pszCaller     = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    pszCallee     = esl_event_get_header(pstEvent, "Caller-Destination-Number");

    /* 如果没有置上waiting park标志，就直接切换状态到active */
    pszWaitingPark = esl_event_get_header(pstEvent, "variable_waiting_park");
    if (DOS_ADDR_INVALID(pszWaitingPark)
        || 0 != dos_strncmp(pszWaitingPark, "true", dos_strlen("true")))
    {
        SC_SCB_SET_STATUS(pstSCB, SC_SCB_ACTIVE);
    }

    pszMainService = esl_event_get_header(pstEvent, "variable_main_service");
    if (DOS_ADDR_INVALID(pszMainService)
        || dos_atoul(pszMainService, &ulMainService) < 0)
    {
        ulMainService = U32_BUTT;
    }

    if (U32_BUTT == pstSCB->ulCustomID)
    {
        pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
        if (DOS_ADDR_INVALID(pstSCBOther))
        {
            /* @TODO 要不要给挂断呼叫 */
        }
        else
        {
            pstSCB->ulCustomID = pstSCBOther->ulCustomID;
        }
    }

    /* 判断是否需要录音，就开始录音 */
    pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
    if (DOS_ADDR_VALID(pstSCB) && DOS_ADDR_VALID(pstSCBOther)
        && pstSCB->ucStatus >= SC_SCB_EXEC && pstSCBOther->ucStatus >= SC_SCB_EXEC
        && (pstSCB->bRecord || pstSCBOther->bRecord))
    {
        if (pstSCB->bRecord)
        {
            pstSCBRecord = pstSCB;
        }
        else
        {
            pstSCBRecord = pstSCBOther;
        }

        if (sc_ep_record(pstSCBRecord) != DOS_SUCC)
        {
            DOS_ASSERT(0);
            sc_logr_error(pstSCB, SC_ESL, "Record FAIL. SCB(%u)", pstSCBRecord->usSCBNo);
        }
    }

    /* 如果是坐席呼叫，就需要更新坐席状态 */
    if (DOS_ADDR_VALID(pstSCB) && DOS_ADDR_VALID(pstSCBOther)
        && pstSCB->ucStatus >= SC_SCB_ACTIVE && pstSCBOther->ucStatus >= SC_SCB_ACTIVE)
    {
        if (pstSCB->bIsAgentCall)
        {
            if (pstSCB->ulAgentID != 0 && pstSCB->ulAgentID != U32_BUTT)
            {
                sc_acd_agent_stat(SC_AGENT_STAT_CALL_OK, pstSCB->ulAgentID, NULL, 0);
            }
        }
        else if (pstSCBOther->bIsAgentCall)
        {
            if (pstSCBOther->ulAgentID != 0 && pstSCBOther->ulAgentID != U32_BUTT)
            {
                sc_acd_agent_stat(SC_AGENT_STAT_CALL_OK, pstSCBOther->ulAgentID, NULL, 0);
            }
        }
    }

    if (pstSCB->bHasEarlyMedia)
    {
        if (SC_SERV_CALL_INTERCEPT == ulMainService
            || SC_SERV_CALL_WHISPERS == ulMainService)
        {
            ulRet = sc_ep_call_intercept(pstSCB);
        }
        else if (SC_SERV_AGENT_SIGNIN == ulMainService)
        {
            if (pstSCB->bIsTTCall)
            {
                sc_ep_esl_execute("unset", "sip_h_EixTTcall", pstSCB->szUUID);
                sc_ep_esl_execute("unset", "sip_h_Mime-version", pstSCB->szUUID);
            }

            //sc_ep_esl_execute("answer", "", pstSCB->szUUID);
            if (pstSCB->ulSiginTimeStamp == 0)
            {
                pszValue = esl_event_get_header(pstEvent, "Caller-Channel-Answered-Time");
                if (DOS_ADDR_VALID(pszValue)
                    && '\0' != pszValue[0]
                    && dos_atoull(pszValue, &uLTmp) == 0)
                {
                    pstSCB->ulSiginTimeStamp = uLTmp / 1000000;
                    sc_logr_debug(pstSCB, SC_ESL, "Get extra data: Caller-Channel-Answered-Time=%s(%u)", pszValue, pstSCB->ulSiginTimeStamp);
                }
            }
            /* 坐席长签成功 */
            ulRet = sc_ep_agent_signin_proc(pstHandle, pstEvent, pstSCB);
        }
        else if (SC_SERV_AUTO_DIALING == ulMainService)
        {
            /* 自动外呼处理 */
            ulRet = sc_ep_auto_dial_proc(pstHandle, pstEvent, pstSCB);
        }
        else if (SC_SERV_DEMO_TASK == ulMainService)
        {
            /* 群呼任务演示 */
            ulRet = sc_ep_demo_task_proc(pstHandle, pstEvent, pstSCB);
        }
        else if (SC_SERV_NUM_VERIFY == ulMainService)
        {
            ulRet = sc_ep_num_verify(pstHandle, pstEvent, pstSCB);
        }
        else if (SC_SERV_AGENT_CLICK_CALL == ulMainService)
        {
            if (pstSCB->bIsAgentCall)
            {
                if (pstSCB->enCallCtrlType == SC_CALL_CTRL_TYPE_AGENT)
                {
                    /* 呼叫另一个坐席 */
                    sc_ep_call_agent_by_id(pstSCB, pstSCB->ulOtherAgentID, DOS_FALSE, DOS_FALSE);
                }
                else if (pstSCB->enCallCtrlType == SC_CALL_CTRL_TYPE_SIP)
                {
                    /* 呼叫一个分机 */
                    pstSCBNew = sc_scb_alloc();
                    if (DOS_ADDR_INVALID(pstSCB))
                    {
                        sc_logr_warning(pstSCB, SC_ESL, "%s", "Cannot make call for the API CMD. Alloc SCB FAIL.");
                        ulRet = DOS_FAIL;
                        DOS_ASSERT(0);
                        goto proc_finished;
                    }

                    pstSCBNew->ulCustomID = pstSCB->ulCustomID;
                    pstSCBNew->ulAgentID = pstSCB->ulAgentID;
                    pstSCBNew->ulTaskID = pstSCB->ulTaskID;
                    pstSCBNew->usOtherSCBNo = pstSCB->usSCBNo;
                    pstSCB->usOtherSCBNo = pstSCBNew->usSCBNo;

                    dos_strncpy(pstSCBNew->szSiteNum, pstSCB->szSiteNum, sizeof(pstSCBNew->szSiteNum));
                    pstSCBNew->szSiteNum[sizeof(pstSCBNew->szSiteNum) - 1] = '\0';

                    /* 指定主被叫号码 */
                    dos_strncpy(pstSCBNew->szCalleeNum, pstSCB->szCallerNum, sizeof(pstSCBNew->szCalleeNum));
                    pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';
                    dos_strncpy(pstSCBNew->szCallerNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCallerNum));
                    pstSCBNew->szCallerNum[sizeof(pstSCBNew->szCallerNum) - 1] = '\0';

                    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
                    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_INTERNAL_CALL);

                    if (sc_dial_make_call2ip(pstSCBNew, SC_SERV_BUTT, DOS_FALSE) != DOS_SUCC)
                    {
                        goto proc_finished;
                    }
                }
                else
                {
                    /* 呼叫客户 */
                    pstSCBNew = sc_scb_alloc();
                    if (DOS_ADDR_INVALID(pstSCB))
                    {
                        sc_logr_warning(pstSCB, SC_ESL, "%s", "Cannot make call for the API CMD. Alloc SCB FAIL.");
                        ulRet = DOS_FAIL;
                        DOS_ASSERT(0);
                        goto proc_finished;
                    }

                    pstSCBNew->ulCustomID = pstSCB->ulCustomID;
                    pstSCBNew->ulAgentID = pstSCB->ulAgentID;
                    pstSCBNew->ulTaskID = pstSCB->ulTaskID;
                    pstSCBNew->usOtherSCBNo = pstSCB->usSCBNo;
                    pstSCB->usOtherSCBNo = pstSCBNew->usSCBNo;

                    dos_strncpy(pstSCBNew->szSiteNum, pstSCB->szSiteNum, sizeof(pstSCBNew->szSiteNum));
                    pstSCBNew->szSiteNum[sizeof(pstSCBNew->szSiteNum) - 1] = '\0';

                    /* 指定被叫号码 */
                    dos_strncpy(pstSCBNew->szCalleeNum, pstSCB->szCallerNum, sizeof(pstSCBNew->szCalleeNum));
                    pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';

                    /* 指定主叫号码 号码组 */
                    if (sc_caller_setting_select_number(pstSCBNew->ulCustomID, pstSCBNew->ulAgentID, SC_SRC_CALLER_TYPE_AGENT, pstSCBNew->szCallerNum, sizeof(pstSCBNew->szCallerNum)) != DOS_SUCC)
                    {
                        sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Get caller fail by agent(%u). ", pstSCBNew->ulAgentID);
                        sc_scb_free(pstSCBNew);
                        pstSCB->usOtherSCBNo = U16_BUTT;
                        sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_CALLER_NUMBER_ILLEGAL);

                        goto proc_finished;
                    }

                    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_PREVIEW_DIALING);

                    if (!sc_ep_black_list_check(pstSCBNew->ulCustomID, pszCaller))
                    {
                        DOS_ASSERT(0);

                        sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Callee in blocak list. (%s)", pszCallee);
                        ulRet = DOS_FAIL;
                        goto proc_finished;
                    }

                    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
                    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_EXTERNAL_CALL);
                    if (sc_send_usr_auth2bs(pstSCBNew) != DOS_SUCC)
                    {
                        DOS_ASSERT(0);

                        sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Send auth fail.", pszCallee);
                        ulRet = DOS_FAIL;

                        sc_ep_hangup_call_with_snd(pstSCB, CC_ERR_SC_MESSAGE_SENT_ERR);
                        goto proc_finished;
                    }
                }

                ulRet = DOS_SUCC;
            }
        }
        else
        {
            ulRet = DOS_SUCC;
        }
    }

proc_finished:

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return ulRet;
}

/**
 * 函数: U32 sc_ep_channel_hungup_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理ESL的CHANNEL HANGUP事件
 * 参数:
 *      esl_handle_t *pstHandle : 发送句柄
 *      esl_event_t *pstEvent   : 时间
 *      SC_SCB_ST *pstSCB       : SCB
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_channel_hungup_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8 *szHangupCause = NULL;
    U32 ulHangupCause = CC_ERR_NO_REASON;
    SC_SCB_ST *pstSCBOther = NULL;

    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
    if (DOS_ADDR_VALID(pstSCBOther))
    {
        if (pstSCBOther->ucStatus <= SC_SCB_ACTIVE)
        {
            pstSCB->ucReleasePart = pstSCB->ucLegRole;
        }
        else
        {
            pstSCB->ucReleasePart = pstSCBOther->ucLegRole;
        }
    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);

    /* 统计坐席通话时长 */
    if (DOS_ADDR_VALID(pstSCB) && DOS_ADDR_VALID(pstSCBOther))
    {
        if (pstSCB->bIsAgentCall)
        {
            if (pstSCBOther->ulAgentID != 0 && pstSCBOther->ulAgentID != U32_BUTT)
            {
                sc_acd_agent_stat(SC_AGENT_STAT_CALL_FINISHED, pstSCBOther->ulAgentID, NULL, 0);
            }
        }
        else if (pstSCBOther->bIsAgentCall)
        {
            if (pstSCBOther->ulAgentID != 0 && pstSCBOther->ulAgentID != U32_BUTT)
            {
                sc_acd_agent_stat(SC_AGENT_STAT_CALL_FINISHED, pstSCBOther->ulAgentID, NULL, 0);
            }
        }
    }

    /* 获取 variable_proto_specific_hangup_cause 挂断原因 */
    szHangupCause = esl_event_get_header(pstEvent, "variable_proto_specific_hangup_cause");
    if (DOS_ADDR_VALID(szHangupCause))
    {
        if (1 == dos_sscanf(szHangupCause, "sip:%u", &ulHangupCause))
        {
            if (pstSCB->usTerminationCause == 0)
            {
                /* 将错误原因记录到pstSCB中 */
                pstSCB->usTerminationCause = ulHangupCause;
            }
        }
    }

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

/**
 * 函数: U32 sc_ep_channel_hungup_complete_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理ESL的CHANNEL HANGUP COMPLETE事件
 * 参数:
 *      esl_handle_t *pstHandle : 发送句柄
 *      esl_event_t *pstEvent   : 时间
 *      SC_SCB_ST *pstSCB       : SCB
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_channel_hungup_complete_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    U32         ulStatus, ulRet = DOS_SUCC;
    //U32         ulAgentStatus;
    U32         ulSCBNo             = U32_BUTT;
    U32         ulHuangupCause      = 0;
    S32         i;
    S8          szCMD[512]          = { 0, };
    S8          *pszTransforType    = NULL;
    S8          *pszGatewayID       = NULL;
    S8          *pszHitDiaplan      = NULL;
    S8          *pszHungupCause     = NULL;
    S8          *pszValue           = NULL;
    SC_SCB_ST   *pstSCBOther        = NULL;
    SC_ACD_AGENT_INFO_ST stAgentInfo;
    U32         ulTransferFinishTime = 0;
    U32         ulProcesingTime      = 0;
    BOOL        bIsInitExtraData     = DOS_FALSE;

    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pszGatewayID = esl_event_get_header(pstEvent, "variable_sip_gateway_name");
    if (DOS_ADDR_INVALID(pszGatewayID)
        || dos_atoul(pszGatewayID, &pstSCB->ulTrunkID) < 0)
    {
        pstSCB->ulTrunkID = U32_BUTT;
    }

    pszTransforType = esl_event_get_header(pstEvent, "variable_transfer_to");
    if (DOS_ADDR_VALID(pszTransforType) && '\0' != pszTransforType[0])
    {
        if (dos_strnicmp(pszTransforType, "blind", dos_strlen("blind")) == 0
            || dos_strnicmp(pszTransforType, "attend", dos_strlen("attend")) == 0)
        {
            sc_logr_info(pstSCB, SC_ESL, "Got into transfor. transfor type: %s", pszTransforType);

            pstSCB->bWaitingOtherRelase = DOS_TRUE;
            goto process_finished;
        }
    }

    pszHungupCause = esl_event_get_header(pstEvent, "variable_sip_term_status");
    if (DOS_ADDR_INVALID(pszHungupCause)
        || dos_atoul(pszHungupCause, &ulHuangupCause) < 0)
    {
        ulHuangupCause = pstSCB->usTerminationCause;
        DOS_ASSERT(0);
    }

    ulStatus = pstSCB->ucStatus;
    switch (ulStatus)
    {
        case SC_SCB_IDEL:
            /* 这个地方初始化一下就好 */
            DOS_ASSERT(0);

            sc_scb_hash_tables_delete(pstSCB->szUUID);
            sc_bg_job_hash_delete(pstSCB->usSCBNo);

            sc_scb_free(pstSCB);
            break;

        case SC_SCB_INIT:
        case SC_SCB_AUTH:
        case SC_SCB_EXEC:
        case SC_SCB_ACTIVE:
        case SC_SCB_RELEASE:
            /* 统一将资源置为release状态 */
            SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);

            /* 将当前leg的信息dump下来 */
            if (DOS_ADDR_INVALID(pstSCB->pstExtraData))
            {
                pstSCB->pstExtraData = dos_dmem_alloc(sizeof(SC_SCB_EXTRA_DATA_ST));
            }
            pthread_mutex_lock(&pstSCB->mutexSCBLock);
            if (DOS_ADDR_VALID(pstSCB->pstExtraData))
            {
                dos_memzero(pstSCB->pstExtraData, sizeof(SC_SCB_EXTRA_DATA_ST));
                sc_ep_parse_extra_data(pstEvent, pstSCB);
            }

            pthread_mutex_unlock(&pstSCB->mutexSCBLock);

            /* 自动外呼，需要维护任务的并发量 */
            if (sc_call_check_service(pstSCB, SC_SERV_AUTO_DIALING)
                && pstSCB->usTCBNo < SC_MAX_TASK_NUM)
            {
                sc_task_concurrency_minus(pstSCB->usTCBNo);

                sc_update_callee_status(pstSCB->usTCBNo, pstSCB->szCalleeNum, SC_CALLEE_NORMAL);

                sc_ep_calltask_result(pstSCB, ulHuangupCause);
            }

            if (pstSCB->ulTrunkCount > 0)
            {
                pstSCB->ulTrunkCount--;
            }

            /* 获取 Channel-HIT-Dialplan 的值，判断是否是二次选路 */
            pszHitDiaplan = esl_event_get_header(pstEvent, "Channel-HIT-Dialplan");
            if (DOS_ADDR_VALID(pszHitDiaplan))
            {
                sc_logr_debug(pstSCB, SC_ESL, "Channel-HIT-Dialplan : %s, trunk cout : %u", pszHitDiaplan, pstSCB->ulTrunkCount);
                if (dos_strcmp(pszHitDiaplan, "false") == 0 && pstSCB->ulTrunkCount)
                {
                    /* 判断一下振铃时间 */
                    if (pstSCB->pstExtraData->ulRingTimeStamp == 0)
                    {
                        goto process_finished;
                    }
                }
            }
            else
            {
                sc_logr_debug(pstSCB, SC_ESL, "Not foung Channel-HIT-Dialplan, trunk cout : %u", pstSCB->ulTrunkCount);
            }

            if (pstSCB->ucTranforRole == SC_TRANS_ROLE_PUBLISH
                || pstSCB->ucTranforRole == SC_TRANS_ROLE_SUBSCRIPTION)
            {
                pszValue = esl_event_get_header(pstEvent, "variable_transfer_finish_time");
                if (DOS_ADDR_VALID(pszValue)
                    && dos_atoul(pszValue, &ulTransferFinishTime) == 0
                    && ulTransferFinishTime > 0)
                {
                    /* 转接的结束时间 */
                    sc_ep_transfer_publish_server_type(pstSCB, ulTransferFinishTime);
                }
            }

            sc_call_trace(pstSCB, "process transfer. role: %u", pstSCB->ucTranforRole);

            if (SC_TRANS_ROLE_NOTIFY == pstSCB->ucTranforRole)
            {
                if (sc_ep_transfer_notify_release(pstSCB) == DOS_SUCC)
                {
                    goto process_finished;
                }
            }
            else if (SC_TRANS_ROLE_PUBLISH == pstSCB->ucTranforRole)
            {
                if (sc_ep_transfer_publish_release(pstSCB) == DOS_SUCC)
                {
                    goto process_finished;
                }
            }
            else if (SC_TRANS_ROLE_SUBSCRIPTION == pstSCB->ucTranforRole)
            {
                if (sc_ep_transfer_subscription_release(pstSCB) == DOS_SUCC)
                {
                    goto process_finished;
                }
            }

            /* 如果呼叫已经进入队列了，需要删除 */
            if (pstSCB->bIsInQueue)
            {
                sc_cwq_del_call(pstSCB);
                pstSCB->bIsInQueue = DOS_FALSE;
            }

            /*
                 * 1.如果有另外一条腿，有必要等待另外一条腿释放
                 * 2.需要另外一条腿没有处于等待释放状态，那就等待吧
                 * 3.如果另一条腿的DID不存在，就直接挂断
                 */
            pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
            if (DOS_ADDR_VALID(pstSCBOther) && pstSCBOther->ucStatus < SC_SCB_ACTIVE)
            {
                sc_ep_hangup_call(pstSCBOther, CC_ERR_NORMAL_CLEAR);
            }

            if (DOS_ADDR_VALID(pstSCBOther)
                && !pstSCBOther->bWaitingOtherRelase)
            {
                /* pstSCB 是先挂断的一方 */
                /* A-leg不是坐席，B-leg是坐席，判断一下B是否已经标记客户，
                    如果没有，获得B对应坐席的整理时长，如果不为0，则B-leg不用挂断电话，进行客户标记 */
                sc_logr_debug(pstSCB, SC_ESL, "Hungup : SCB(%u) bIsAgentCall : %u, SCBOther(%u) bIsAgentCall : %u, SCBOther bIsMarkCustomer : %u", pstSCB->usSCBNo, pstSCB->bIsAgentCall, pstSCBOther->usSCBNo, pstSCBOther->bIsAgentCall, pstSCBOther->bIsMarkCustomer);
                if (!pstSCB->bIsAgentCall
                    && pstSCBOther->bIsAgentCall)
                {
                    /* 如果通话过程中标记了，就不要走下面这一段 */
                    if (!pstSCBOther->bIsMarkCustomer)
                    {
                        /* 关于录音的问题，
                                    1、坐席一端，有录音业务
                                    2、客户端先挂断的，坐席一端后挂，
                                    3、坐席还没有标记客户
                                    需要将录音业务结束，并且将业务传递给客户，生成话单
                              */
                        /* 获得坐席 */
                        if (sc_acd_get_agent_by_id(&stAgentInfo, pstSCBOther->ulAgentID) != DOS_SUCC)
                        {
                            /* 获取坐席失败，直接释放 */
                            sc_logr_notice(pstSCB, SC_ESL, "Get agent FAIL by agentID(%u)", pstSCBOther->ulAgentID);
                        }
                        else
                        {
                            /* 判断是否有录音业务 */
                            if (pstSCBOther->bRecord && pstSCBOther->pszRecordFile)
                            {
                                dos_snprintf(szCMD, sizeof(szCMD)
                                                , "bgapi uuid_record %s stop %s/%s\r\n"
                                                , pstSCBOther->szUUID
                                                , SC_RECORD_FILE_PATH
                                                , pstSCBOther->pszRecordFile);
                                sc_ep_esl_execute_cmd(szCMD);
                            }

                            /* 判断坐席是否是长签 */
                            if (sc_call_check_service(pstSCBOther, SC_SERV_AGENT_SIGNIN))
                            {
                                ulSCBNo = pstSCBOther->usSCBNo;
                            }
                            else
                            {
                                ulSCBNo = U32_BUTT;
                            }

                            if (stAgentInfo.ucProcesingTime != 0)
                            {
                                /* 将 客户端挂断的时间戳记录下来，作为客户标记的开始时间 */
                                if (DOS_ADDR_VALID(pstSCB->pstExtraData))
                                {
                                    pstSCBOther->ulMarkStartTimeStamp = pstSCB->pstExtraData->ulByeTimeStamp;
                                }

                                dos_snprintf(szCMD, sizeof(szCMD), "bgapi uuid_break %s \r\n", pstSCBOther->szUUID);
                                sc_ep_esl_execute_cmd(szCMD);
                                /* 不要挂断坐席，把坐席置为 整理状态，放音提示坐席，可标记客户 */
                                pstSCBOther->usOtherSCBNo = U16_BUTT;
                                if (sc_acd_update_agent_info(pstSCBOther, SC_ACD_PROC, ulSCBNo, NULL) == DOS_SUCC)
                                {
                                    pstSCBOther->bIsInMarkState = DOS_TRUE;
                                    pstSCBOther->bIsMarkCustomer = DOS_FALSE;

                                    if (pstSCBOther->bIsInHoldStatus)
                                    {
                                        /* 如果是在hold状态，需要先解除hold */
                                        sc_ep_esl_execute("unhold", NULL, pstSCBOther->szUUID);
                                    }

                                    sc_logr_debug(pstSCB, SC_ACD, "Start timer change agent(%u) from SC_ACD_PROC to SC_ACD_IDEL, time : %d", pstSCBOther->ulAgentID, ulProcesingTime);
                                    sc_ep_esl_execute("set", "playback_terminators=none", pstSCBOther->szUUID);
                                    sc_ep_esl_execute("sleep", "500", pstSCBOther->szUUID);
                                    dos_snprintf(szCMD, sizeof(szCMD)
                                                    , "{timeout=%u}file_string://%s/call_over.wav"
                                                    , stAgentInfo.ucProcesingTime * 1000, SC_PROMPT_TONE_PATH);
                                    sc_ep_esl_execute("playback", szCMD, pstSCBOther->szUUID);
                                }
                            }
                            else
                            {
                                /* 整理时长为0，直接置为 空闲状态 */

                                if (ulSCBNo == U32_BUTT)
                                {
                                    /* 非长签，释放掉SC */
                                    sc_ep_hangup_call(pstSCBOther, CC_ERR_NORMAL_CLEAR);
                                }

                                sc_acd_update_agent_info(pstSCBOther, SC_ACD_IDEL, ulSCBNo, NULL);
                            }


                        }
                    }
                    else
                    {
                        /* 通话过程中进行了客户标记，等待坐席挂断 */
                        pstSCB->bWaitingOtherRelase = DOS_TRUE;
                        break;
                    }

                    pstSCBOther->bIsMarkCustomer = DOS_FALSE;
                }
                else
                {
                    if (pstSCBOther->bIsAgentCall
                        && sc_call_check_service(pstSCBOther, SC_SERV_AGENT_SIGNIN))
                    {
                        /* 另一条腿是坐席长签，不用挂断 */
                        /* 判断是否有录音业务，如果有的话，需要把 pstSCB 的时间信息拷贝过来，发送完话单后，要清空  */
                        /* 判断是否有录音业务 */
                        if (pstSCBOther->bRecord && pstSCBOther->pszRecordFile)
                        {
                            dos_snprintf(szCMD, sizeof(szCMD)
                                            , "bgapi uuid_record %s stop %s/%s\r\n"
                                            , pstSCBOther->szUUID
                                            , SC_RECORD_FILE_PATH
                                            , pstSCBOther->pszRecordFile);
                            sc_ep_esl_execute_cmd(szCMD);

                         }
                    }
                    else
                    {
                        /* 等待另一条腿的挂断 */
                        if (sc_ep_hangup_call(pstSCBOther, CC_ERR_NORMAL_CLEAR) == DOS_SUCC)
                        {
                            pstSCB->bWaitingOtherRelase = DOS_TRUE;
                            sc_logr_info(pstSCB, SC_ESL, "Waiting other leg hangup.Curretn Leg UUID: %s, Other Leg UUID: %s"
                                            , pstSCB->szUUID ? pstSCB->szUUID : "NULL"
                                            , pstSCBOther->szUUID ? pstSCBOther->szUUID : "NULL");

                            /* 如果为坐席，则应该修改坐席的状态 */
                            if (pstSCB->bIsAgentCall)
                            {
                                /* 坐席对应的leg正在等待退出，修改坐席的状态变为空闲状态 */
                                if (sc_acd_get_agent_by_id(&stAgentInfo, pstSCB->ulAgentID) != DOS_SUCC)
                                {
                                    /* 获取坐席失败 */
                                    sc_logr_notice(pstSCB, SC_ESL, "Get agent FAIL by agentID(%u)", pstSCB->ulAgentID);
                                }
                                else
                                {
                                    /* 判断是否是长签 */
                                    if (sc_call_check_service(pstSCB, SC_SERV_AGENT_SIGNIN))
                                    {
                                        sc_acd_update_agent_info(pstSCB, SC_ACD_IDEL, pstSCB->usSCBNo, NULL);
                                    }
                                    else
                                    {
                                        sc_acd_update_agent_info(pstSCB, SC_ACD_IDEL, U32_BUTT, NULL);
                                    }
                                }
                            }

                            break;
                        }
                    }
                }
            }

            if (pstSCB->bIsAgentCall)
            {
                if (sc_acd_get_agent_by_id(&stAgentInfo, pstSCB->ulAgentID) != DOS_SUCC)
                {
                    /* 获取坐席失败 */
                    sc_logr_notice(pstSCB, SC_ESL, "Get agent FAIL by agentID(%u)", pstSCB->ulAgentID);
                }
                else
                {
                    /* 判断是否是长签, 如果是长签，需要重新呼叫坐席 */
                    sc_acd_update_agent_info(pstSCB, SC_ACD_IDEL, U32_BUTT, NULL);
                }

                if (sc_call_check_service(pstSCB, SC_SERV_AGENT_SIGNIN))
                {
                    /* 长签的电话挂断时，不能有录音业务 */
                    for (i=0; i<SC_MAX_SRV_TYPE_PRE_LEG; i++)
                    {
                        if (pstSCB->aucServiceType[i] == SC_SERV_RECORDING)
                        {
                            pstSCB->aucServiceType[i] = U8_BUTT;
                        }
                    }

                    /* 长签的电话被挂断，修改坐席的answer时间，修改为 ulSiginTimeStamp */
                    if (pstSCB->ulSiginTimeStamp != 0)
                    {
                        pstSCB->pstExtraData->ulAnswerTimeStamp = pstSCB->ulSiginTimeStamp;
                    }
                }
                else
                {
                    if (DOS_ADDR_INVALID(pstSCBOther) && pstSCB->ulMarkStartTimeStamp != 0)
                    {
                        /* 修改主叫号码，修改为客户的号码 */
                        if (pstSCB->szCustomerNum[0] != '\0')
                        {
                            dos_strncpy(pstSCB->szCallerNum, pstSCB->szCustomerNum, SC_TEL_NUMBER_LENGTH);

                        }
                        else
                        {
                            if (sc_call_check_service(pstSCB, SC_SERV_OUTBOUND_CALL))
                            {
                                /* 不用修改 */
                            }
                            else
                            {
                                dos_strncpy(pstSCB->szCallerNum, pstSCB->szCalleeNum, SC_TEL_NUMBER_LENGTH);
                            }
                        }
                        pstSCB->szCallerNum[SC_TEL_NUMBER_LENGTH-1] = '\0';

                        /* 修改 pstSCB 的被叫号码，修改为 客户标记值 */
                        if (pstSCB->szCustomerMark[0] != '\0')
                        {
                            dos_strncpy(pstSCB->szCalleeNum, pstSCB->szCustomerMark, SC_TEL_NUMBER_LENGTH);
                        }
                        else
                        {
                            /* 没有标记、且标记没有超时，坐席一侧直接挂断了 */
                            dos_strncpy(pstSCB->szCalleeNum, "*#", SC_TEL_NUMBER_LENGTH);
                        }
                    }
                }
            }

            if (DOS_ADDR_VALID(pstSCBOther) && pstSCBOther->bIsAgentCall)
            {
                if (pstSCBOther->bWaitingOtherRelase)
                {
                    /* 坐席对应的leg正在等待退出，修改坐席的状态变为空闲状态 */
                    if (sc_acd_get_agent_by_id(&stAgentInfo, pstSCBOther->ulAgentID) != DOS_SUCC)
                    {
                        /* 获取坐席失败 */
                        sc_logr_notice(pstSCB, SC_ESL, "Get agent FAIL by agentID(%u)", pstSCBOther->ulAgentID);
                    }
                    else
                    {
                        /* 判断是否是长签 */
                        if (sc_call_check_service(pstSCBOther, SC_SERV_AGENT_SIGNIN))
                        {
                            sc_acd_update_agent_info(pstSCBOther, SC_ACD_IDEL, pstSCBOther->usSCBNo, NULL);
                        }
                        else
                        {
                            sc_acd_update_agent_info(pstSCBOther, SC_ACD_IDEL, U32_BUTT, NULL);
                        }
                    }
                }
                else
                {
                    if (pstSCB->bIsAgentCall && sc_call_check_service(pstSCBOther, SC_SERV_AGENT_SIGNIN))
                    {
                        /* 另一条腿为坐席长签，此处不是和客户通话，直接修改为idel状态 */
                        sc_acd_update_agent_info(pstSCBOther, SC_ACD_IDEL, pstSCBOther->usSCBNo, NULL);
                        pstSCBOther->usOtherSCBNo = U16_BUTT;
                        sc_ep_play_sound(SC_SND_MUSIC_SIGNIN, pstSCBOther->szUUID, 1);

                        /* 需要给长签的那条leg的时间戳赋值，否则显示1970-01-01，发送完账单后，清空 */
                        bIsInitExtraData = DOS_TRUE;
                    }

                    /* 坐席处在整理状态，应该另一条腿的时间信息，拷贝到坐席这条leg上 */
                    if (DOS_ADDR_INVALID(pstSCBOther->pstExtraData))
                    {
                        pstSCBOther->pstExtraData = dos_dmem_alloc(sizeof(SC_SCB_EXTRA_DATA_ST));
                    }
                    if (DOS_ADDR_VALID(pstSCBOther->pstExtraData) && DOS_ADDR_VALID(pstSCB->pstExtraData))
                    {
                        dos_memcpy(pstSCBOther->pstExtraData, pstSCB->pstExtraData, sizeof(SC_SCB_EXTRA_DATA_ST));
                    }
                }
            }

            if (pstSCB->ulMarkStartTimeStamp == 0
                || pstSCB->pstExtraData->ulByeTimeStamp != pstSCB->pstExtraData->ulStartTimeStamp)
            {
                /* 判断一下是否有录音话单，如果有，则删除 */
                for (i=0; i<SC_MAX_SRV_TYPE_PRE_LEG; i++)
                {
                    if (pstSCB->aucServiceType[i] == SC_SERV_RECORDING)
                    {
                        pstSCB->aucServiceType[i] = U8_BUTT;
                    }
                }

                if (DOS_ADDR_VALID(pstSCBOther))
                {
                    for (i=0; i<SC_MAX_SRV_TYPE_PRE_LEG; i++)
                    {
                        if (pstSCBOther->aucServiceType[i] == SC_SERV_RECORDING)
                        {
                            pstSCBOther->aucServiceType[i] = U8_BUTT;
                        }
                    }
                }

                sc_logr_debug(pstSCB, SC_ESL, "Send CDR to bs. SCB1 No:%d, SCB2 No:%d", pstSCB->usSCBNo, pstSCB->usOtherSCBNo);
                /* 发送话单 */
                if (sc_send_billing_stop2bs(pstSCB) != DOS_SUCC)
                {
                    sc_logr_debug(pstSCB, SC_ESL, "Send CDR to bs FAIL. SCB1 No:%d, SCB2 No:%d", pstSCB->usSCBNo, pstSCB->usOtherSCBNo);
                }
                else
                {
                    sc_logr_debug(pstSCB, SC_ESL, "Send CDR to bs SUCC. SCB1 No:%d, SCB2 No:%d", pstSCB->usSCBNo, pstSCB->usOtherSCBNo);
                }

                if (bIsInitExtraData
                    && DOS_ADDR_VALID(pstSCBOther)
                    && DOS_ADDR_VALID(pstSCBOther->pstExtraData))
                {
                    dos_memzero(pstSCBOther->pstExtraData, sizeof(SC_SCB_EXTRA_DATA_ST));
                }
            }

            sc_logr_debug(pstSCB, SC_ESL, "Start release the SCB. SCB1 No:%d, SCB2 No:%d", pstSCB->usSCBNo, pstSCB->usOtherSCBNo);
            /* 维护资源 */
            sc_scb_hash_tables_delete(pstSCB->szUUID);
            if (DOS_ADDR_VALID(pstSCBOther) && pstSCBOther->bWaitingOtherRelase)
            {
                sc_scb_hash_tables_delete(pstSCBOther->szUUID);
            }

            sc_bg_job_hash_delete(pstSCB->usSCBNo);
            sc_scb_free(pstSCB);

            if (pstSCBOther && pstSCBOther->bWaitingOtherRelase)
            {
                sc_bg_job_hash_delete(pstSCBOther->usSCBNo);
                sc_scb_free(pstSCBOther);
            }
            else if (DOS_ADDR_VALID(pstSCBOther))
            {
                pstSCBOther->usOtherSCBNo = U16_BUTT;
            }

            break;
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;
            break;
    }

process_finished:

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

/**
 * 函数: U32 sc_ep_channel_hold(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理ESL的CHANNEL HEARTBEAT事件
 * 参数:
 *      esl_handle_t *pstHandle : 发送句柄
 *      esl_event_t *pstEvent   : 事件
 *      SC_SCB_ST *pstSCB       : SCB
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_channel_hold(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8        *pszChannelStat = NULL;

    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        /* Hold 事件不能拆呼叫的 */
        return DOS_SUCC;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    pszChannelStat = esl_event_get_header(pstEvent, "Channel-Call-State");
    if (DOS_ADDR_VALID(pszChannelStat))
    {
        if (dos_strnicmp(pszChannelStat, "HELD", dos_strlen("HELD")) == 0)
        {
            if (0 == pstSCB->ulLastHoldTimetamp)
            {
                pstSCB->ulLastHoldTimetamp = time(0);
                pstSCB->usHoldCnt++;
                pstSCB->bIsInHoldStatus = DOS_TRUE;
            }
        }
        else if (dos_strnicmp(pszChannelStat, "UNHELD", dos_strlen("UNHELD")) == 0)
        {
            if (pstSCB->ulLastHoldTimetamp)
            {
                pstSCB->usHoldTotalTime += (time(0) - pstSCB->ulLastHoldTimetamp);
                pstSCB->ulLastHoldTimetamp = 0;
                pstSCB->bIsInHoldStatus = DOS_FALSE;
            }
        }
    }

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/**
 * 函数: U32 sc_ep_dtmf_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理ESL的CHANNEL DTMF事件
 * 参数:
 *      esl_handle_t *pstHandle : 发送句柄
 *      esl_event_t *pstEvent   : 时间
 *      SC_SCB_ST *pstSCB       : SCB
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_dtmf_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8 *pszDTMFDigit = NULL;
    U32 ulTaskMode   = U32_BUTT;
    U32 ulLen        = 0;
    U32 ulCurrTime   = 0;
    SC_SCB_ST *pstSCBOther = NULL;

    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        /* dtmf 事件不能拆呼叫 */
        return DOS_SUCC;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    if (0 == pstSCB->ulFirstDTMFTime)
    {
        pstSCB->ulFirstDTMFTime = time(NULL);
    }

    pszDTMFDigit = esl_event_get_header(pstEvent, "DTMF-Digit");
    if (DOS_ADDR_INVALID(pszDTMFDigit))
    {
        DOS_ASSERT(0);
        goto process_fail;
    }

    sc_logr_info(pstSCB, SC_ESL, "Recv DTMF: %s, %d, Numbers Dialed: %s", pszDTMFDigit, pszDTMFDigit[0], pstSCB->szDialNum);

    /* 自动外呼，拨0接通坐席 */
    if (sc_call_check_service(pstSCB, SC_SERV_AUTO_DIALING))
    {
        /* 群呼任务的流程是，放音-按键-进队列-转坐席-坐席振铃-坐席接通, 走到这里有可能已经在按键之后了，需要排除一下 */
        pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
        if (pstSCB->bIsInQueue || DOS_ADDR_VALID(pstSCBOther))
        {
            sc_logr_debug(pstSCB, SC_ESL, "Auto dialing recv a dtmf. The call is in queue or has connected to an agent, disgard."
                            , pszDTMFDigit, pszDTMFDigit[0], pstSCB->szDialNum);
            goto process_succ;
        }

        ulTaskMode = sc_task_get_mode(pstSCB->usTCBNo);
        if (ulTaskMode >= SC_TASK_MODE_BUTT)
        {
            DOS_ASSERT(0);
            /* 要不要挂断 */
            goto process_fail;
        }

        /* 给客户放音 */
        if (SC_TASK_MODE_KEY4AGENT == ulTaskMode)
        {
            pstSCB->bIsHasKeyCallTask = DOS_TRUE;
            sc_ep_call_queue_add(pstSCB, sc_task_get_agent_queue(pstSCB->usTCBNo), DOS_FALSE);
        }
        else if(SC_TASK_MODE_KEY4AGENT1 == ulTaskMode
                && '0' == pszDTMFDigit[0])
        {
            pstSCB->bIsHasKeyCallTask = DOS_TRUE;
            sc_ep_call_queue_add(pstSCB, sc_task_get_agent_queue(pstSCB->usTCBNo), DOS_FALSE);
        }
    }
    else if (sc_call_check_service(pstSCB, SC_SERV_DEMO_TASK))
    {
        pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
        if (pstSCB->bIsInQueue || DOS_ADDR_VALID(pstSCBOther))
        {
            sc_logr_debug(pstSCB, SC_ESL, "Auto dialing recv a dtmf. The call is in queue or has connected to an agent, disgard."
                            , pszDTMFDigit, pszDTMFDigit[0], pstSCB->szDialNum);
            goto process_succ;
        }

        pstSCB->bIsHasKeyCallTask = DOS_TRUE;
        /* 接通坐席 */
        sc_ep_demo_task_call_agent(pstSCB);

    }
    else if (sc_call_check_service(pstSCB, SC_SERV_AGENT_CALLBACK)
        || sc_call_check_service(pstSCB, SC_SERV_AGENT_SIGNIN)
        || !sc_call_check_service(pstSCB, SC_SERV_EXTERNAL_CALL)
        || pstSCB->bIsAgentCall)
    {
        /* 坐席长签，二次拨号 */
        /* 只有 * 或者 # 开头时才被缓存，获取时间间隔为3s，超过3s则去掉缓存中的数据。
           一般 # 作为结束符，特别 "##", 挂断对端的电话 */
        ulCurrTime = time(NULL);
        if ((pstSCB->szDialNum[0] != '\0'
            && ulCurrTime - pstSCB->ulLastDTMFTime >= 3))
        {
            /* 超时 */
            pstSCB->szDialNum[0] = '\0';
            sc_logr_debug(pstSCB, SC_ESL, "Scb(%u) DTMF timeout.", pstSCB->usSCBNo);
        }

        if (pstSCB->szDialNum[0] == '\0'
            && (pszDTMFDigit[0] != '#' && pszDTMFDigit[0] != '*'))
        {
            /* 第一个符号不是 '#' 或者 '*'，不保存 */
            sc_logr_debug(pstSCB, SC_ESL, "Scb(%u) DTMF is not * or #.", pstSCB->usSCBNo);

            goto process_succ;
        }

        pstSCB->ulLastDTMFTime = ulCurrTime;
        /* 保存到缓存中 */
        ulLen = dos_strlen(pstSCB->szDialNum);
        if (ulLen < SC_TEL_NUMBER_LENGTH - 1)
        {
            dos_snprintf(pstSCB->szDialNum+ulLen, SC_TEL_NUMBER_LENGTH-ulLen, "%s", pszDTMFDigit);
        }

        if (dos_strcmp(pstSCB->szDialNum, SC_POTS_HANGUP_CUSTOMER1) == 0
            || dos_strcmp(pstSCB->szDialNum, SC_POTS_HANGUP_CUSTOMER2) == 0)
        {
            /* ## / ** */
            sc_ep_pots_pro(pstSCB, pstEvent, DOS_TRUE);
            /* 清空缓存 */
            pstSCB->szDialNum[0] = '\0';
        }
        else if (pszDTMFDigit[0] == '#' && dos_strlen(pstSCB->szDialNum) > 1)
        {
            /* # 为结束符，收到后，就应该去解析, 特别的，如果第一个字符为#,不需要去解析 */
            sc_logr_debug(pstSCB, SC_ESL, "Secondary dialing. caller : %s, DialNum : %s", pstSCB->szCallerNum, pstSCB->szDialNum);
            /* 不保存最后一个 # */
            pstSCB->szDialNum[dos_strlen(pstSCB->szDialNum) - 1] = '\0';
            sc_ep_pots_pro(pstSCB, pstEvent, DOS_TRUE);
            /* 清空缓存 */
            pstSCB->szDialNum[0] = '\0';
        }
        else if (pszDTMFDigit[0] == '*' && dos_strlen(pstSCB->szDialNum) > 1)
        {
            /* 判断接收到 * 号，是否需要解析 */
            if (dos_strcmp(pstSCB->szDialNum, SC_POTS_BLIND_TRANSFER)
                && dos_strcmp(pstSCB->szDialNum, SC_POTS_ATTENDED_TRANSFER)
                && dos_strcmp(pstSCB->szDialNum, SC_POTS_MARK_CUSTOMER))
            {
                /* 解析 */
                sc_logr_debug(pstSCB, SC_ESL, "Secondary dialing. caller : %s, DialNum : %s", pstSCB->szCallerNum, pstSCB->szDialNum);
                /* 不保存最后一个 * */
                pstSCB->szDialNum[dos_strlen(pstSCB->szDialNum) - 1] = '\0';
                sc_ep_pots_pro(pstSCB, pstEvent, DOS_TRUE);
                /* 清空缓存 */
                pstSCB->szDialNum[0] = '\0';
            }
        }
    }

process_succ:
    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

process_fail:
    sc_call_trace(pstSCB, "Finished to process %s event. FAIL.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_FAIL;

}

/**
 * 函数: U32 sc_ep_playback_stop(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理放音结束事件
 * 参数:
 *      esl_handle_t *pstHandle : 发送句柄
 *      esl_event_t *pstEvent   : 时间
 *      SC_SCB_ST *pstSCB       : SCB
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_playback_stop(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    U32           ulTaskMode = 0;
    U32           ulMainService = U32_BUTT;
    U32           ulErrCode = CC_ERR_NO_REASON;
    U32           ulPlaybackMS, ulPlaybackTimeout;
    S8            *pszPlaybackMS = 0;
    S8            *pszPlaybackTimeout = 0;
    S8            *pszMainService = NULL;
    S8            *pszValue = NULL;
    S8            *pszSIPTermCause = NULL;
    SC_SCB_ST     *pstOtherSCB    = NULL;

    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));
    pszMainService = esl_event_get_header(pstEvent, "variable_main_service");
    pszSIPTermCause = esl_event_get_header(pstEvent, "variable_sip_term_cause");

    if (sc_call_check_service(pstSCB, SC_SERV_AGENT_SIGNIN)
        && SC_SCB_ACTIVE == pstSCB->ucStatus
        && '\0' != pstSCB->szUUID[0])
    {
        sc_ep_agent_signin_proc(pstHandle, pstEvent, pstSCB);

        goto proc_succ;
    }

    if (DOS_ADDR_INVALID(pszMainService)
        || dos_atoul(pszMainService, &ulMainService) < 0)
    {
        ulMainService = U32_BUTT;
    }

    /* 如果服务类型OK，就根据服务类型来处理。如果服务类型不OK，就直接挂机吧 */
    if (U32_BUTT != ulMainService)
    {
        pstOtherSCB = sc_scb_get(pstSCB->usOtherSCBNo);

        switch (ulMainService)
        {
            case SC_SERV_AUTO_DIALING:

                /* 先减少播放次数，再判断播放次数，如果播放次数已经使用完就需要后续处理 */
                pstSCB->ucCurrentPlyCnt--;
                if (pstSCB->ucCurrentPlyCnt <= 0)
                {
                    /* 如果playback-stop消息中有term cause，说明该呼叫即将结束，这个地方不在记录时间 */
                    if (DOS_ADDR_INVALID(pszSIPTermCause))
                    {
                        pstSCB->ulIVRFinishTime = time(NULL);
                    }
                    ulTaskMode = sc_task_get_mode(pstSCB->usTCBNo);
                    if (ulTaskMode >= SC_TASK_MODE_BUTT)
                    {
                        DOS_ASSERT(0);
                        ulErrCode = CC_ERR_SC_CONFIG_ERR;
                        goto proc_error;
                    }

                    switch (ulTaskMode)
                    {
                        /* 以两种放音结束后需要挂断 */
                        case SC_TASK_MODE_KEY4AGENT:
                        case SC_TASK_MODE_KEY4AGENT1:
                            if (!pstSCB->bIsHasKeyCallTask)
                            {
                                sc_logr_notice(pstSCB, SC_ESL, "Hangup call for there is no input.(%s)", pstSCB->szUUID);
                                sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
                            }
                            else
                            {
                                sc_logr_notice(pstSCB, SC_ESL, "Playback stop with there has a key input.(%s)", pstSCB->szUUID);
                                /* 已经在按键的地方处理了，这里不用做什么 */
                            }

                            break;
                        case SC_TASK_MODE_AUDIO_ONLY:
                            /* 如果不在呼叫队列里面，就有可能要挂断 */
                            sc_logr_notice(pstSCB, SC_ESL, "Hangup call for audio play finished.(%s)", pstSCB->szUUID);
                            sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
                            break;

                        /* 放音后接通坐席 */
                        case SC_TASK_MODE_AGENT_AFTER_AUDIO:
                            /* 1.获取坐席队列，2.查找坐席。3.接通坐席 */
                            /* 判断一下scd是否是active状态。如果不是，则直接挂断，不用接通坐席了 */
                            if (pstSCB->ucStatus != SC_SCB_ACTIVE)
                            {
                                sc_logr_info(pstSCB, SC_ESL, "Hangup call. SCB(%u) status is %u.(%s)", pstSCB->usSCBNo, pstSCB->ucStatus, pstSCB->szUUID);
                                sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
                            }

                            sc_ep_call_queue_add(pstSCB, sc_task_get_agent_queue(pstSCB->usTCBNo), DOS_FALSE);
                            break;

                        /* 这个地方出故障了 */
                        case SC_TASK_MODE_DIRECT4AGETN:
                        default:
                            DOS_ASSERT(0);
                            ulErrCode = CC_ERR_SC_CONFIG_ERR;
                            goto proc_error;
                    }
                }

                //break;
                goto proc_succ;
            case SC_SERV_DEMO_TASK:
                pstSCB->ucCurrentPlyCnt--;
                if (pstSCB->ucCurrentPlyCnt <= 0)
                {
                    /* 如果playback-stop消息中有term cause，说明该呼叫即将结束，这个地方不在记录时间 */
                    if (DOS_ADDR_INVALID(pszSIPTermCause))
                    {
                        pstSCB->ulIVRFinishTime = time(NULL);
                    }

                    if (!pstSCB->bIsHasKeyCallTask)
                    {
                        sc_logr_notice(pstSCB, SC_ESL, "Hangup call for there is no input.(%s)", pstSCB->szUUID);
                        sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
                    }
                    else
                    {
                        sc_logr_notice(pstSCB, SC_ESL, "Playback stop with there has a key input.(%s)", pstSCB->szUUID);
                        /* 已经在按键的地方处理了，这里不用做什么 */
                    }
                }

                goto proc_succ;
            case SC_SERV_NUM_VERIFY:
                //break;
                goto proc_succ;

            default:
                break;
        }
    }

    pszValue = esl_event_get_header(pstEvent, "variable_hungup_after_play");
    if (DOS_ADDR_VALID(pszValue))
    {
        if (dos_strcmp(pszValue, "false") == 0)
        {
            /* 播放后，不需要挂断 */
            pszPlaybackMS = esl_event_get_header(pstEvent, "variable_playback_ms");
            pszPlaybackTimeout = esl_event_get_header(pstEvent, "timeout");
            if (DOS_ADDR_VALID(pszPlaybackMS) && DOS_ADDR_VALID(pszPlaybackTimeout)
                && dos_atoul(pszPlaybackMS, &ulPlaybackMS) >= 0
                && dos_atoul(pszPlaybackTimeout, &ulPlaybackTimeout) >= 0
                && ulPlaybackMS >= ulPlaybackTimeout)
            {
                /* 超时 */
                sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);

                goto proc_succ;
            }

            sc_logr_debug(pstSCB, SC_ESL, "SCB %d playback stop not need hangup, %s", pstSCB->usSCBNo, pszValue);
        }
        else
        {
            sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
        }

        goto proc_succ;
    }

    if (pstSCB->bIsInMarkState)
    {
        /* 这个地方条件比较多，说明一下1. 前面五个都是在判断是否因超时而发送的该消息，最后一个判断是否处于标记状态 */
        pszPlaybackMS = esl_event_get_header(pstEvent, "variable_playback_ms");
        pszPlaybackTimeout = esl_event_get_header(pstEvent, "timeout");
        if (DOS_ADDR_VALID(pszPlaybackMS) && DOS_ADDR_VALID(pszPlaybackTimeout)
            && dos_atoul(pszPlaybackMS, &ulPlaybackMS) >= 0
            && dos_atoul(pszPlaybackTimeout, &ulPlaybackTimeout) >= 0
            && ulPlaybackMS >= ulPlaybackTimeout)
        {
            /* 标记超时，挂断电话, 将客户的标记值更改为 '*#'， 表示没有进行客户标记 */
            pstSCB->szCustomerMark[0] = '\0';
            dos_strcpy(pstSCB->szCustomerMark, "*#");
            pstSCB->usOtherSCBNo = U16_BUTT;

            /* 修改坐席的状态， 这里可以不用判断是否长签，长签的走的是 sc_ep_agent_signin_proc， 走到这里的一定不是长签的 */
            sc_acd_update_agent_info(pstSCB, SC_ACD_IDEL, U32_BUTT, NULL);
            sc_ep_hangup_call(pstSCB, CC_ERR_NORMAL_CLEAR);

            pstSCB->bIsInMarkState = DOS_FALSE;

            goto proc_succ;
        }
    }

    sc_logr_notice(pstSCB, SC_ESL, "SCB %d donot needs handle any playback application.", pstSCB->usSCBNo);

proc_succ:

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

proc_error:

    sc_call_trace(pstSCB,"FAILED to process %s event. Call will be hangup.", esl_event_get_header(pstEvent, "Event-Name"));

    sc_ep_per_hangup_call(pstSCB);
    sc_ep_hangup_call_with_snd(pstSCB, ulErrCode);

    return DOS_FAIL;
}

/**
 * 函数: sc_ep_destroy_proc
 * 功能: 处理FS通道结束的消息,主要清理资源，保护作用
 * 参数:
 *      esl_handle_t *pstHandle : 发送句柄
 *      esl_event_t *pstEvent   : 事件
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL, 不应该影响业务流程
 */
U32 sc_ep_destroy_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent)
{
    S8          *pszUUID      = NULL;
    SC_SCB_ST   *pstSCB       = NULL;
    SC_SCB_ST   *pstSCBOther  = NULL;

    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        return DOS_SUCC;
    }

    pstSCB = sc_scb_hash_tables_find(pszUUID);
    if (DOS_ADDR_INVALID(pstSCB))
    {
        return DOS_SUCC;
    }

    pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
    if (DOS_ADDR_VALID(pstSCBOther)
        && !pstSCBOther->bWaitingOtherRelase)
    {
        /* 另一条leg还没有释放，这里不要释放这一条leg了，否则话单会有问题 */
        return DOS_SUCC;
    }


    sc_logr_warning(pstSCB, SC_ESL, "Free scb while channel destroy, it's not in the designed.SCBNO: %u, UUID: %s", pstSCB->usSCBNo, pszUUID);

    if (pstSCB->ulTaskID != 0 && pstSCB->ulTaskID != U32_BUTT)
    {
        sc_task_concurrency_minus(pstSCB->usTCBNo);
    }

    sc_scb_free(pstSCB);

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_ep_playback_stop(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理放音结束事件
 * 参数:
 *      esl_handle_t *pstHandle : 发送句柄
 *      esl_event_t *pstEvent   : 时间
 *      SC_SCB_ST *pstSCB       : SCB
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_record_stop(esl_handle_t *pstHandle, esl_event_t *pstEvent)
{
    S8          *pszUUID            = NULL;
    S8          *pszRecordFile      = NULL;
    SC_SCB_ST   *pstSCB             = NULL;
    SC_SCB_ST   *pstOtherSCB        = NULL;
    U16         usOtherSCBNo        = U16_BUTT;
    S8          szCMD[512]          = { 0, };
    S8          *pszTmp             = NULL;
    U64         uLTmp               = 0;
    U8          aucServiceType[SC_MAX_SRV_TYPE_PRE_LEG];        /* 业务类型 列表 */
    S32         i                   = 0;
    U32         ulMarkStartTimeStamp = 0;

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_logr_debug(pstSCB, SC_ESL, "ESL event process START. %s(%d), SCB No:%s, Channel Name: %s"
                            , esl_event_get_header(pstEvent, "Event-Name")
                            , pstEvent->event_id
                            , esl_event_get_header(pstEvent, "variable_scb_number")
                            , esl_event_get_header(pstEvent, "Channel-Name"));

    sc_logr_debug(pstSCB, SC_ESL, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCB = sc_scb_hash_tables_find(pszUUID);
    if (DOS_ADDR_INVALID(pstSCB)
        || !pstSCB->bValid)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (!pstSCB->bIsSendRecordCdr)
    {
        /* 不需要发送录音话单 */
        goto finished;
    }

    pstSCB->bIsSendRecordCdr = DOS_FALSE;

    ulMarkStartTimeStamp = pstSCB->ulMarkStartTimeStamp;
    /* 将当前leg的信息dump下来 */
    if (DOS_ADDR_INVALID(pstSCB->pstExtraData))
    {
        pstSCB->pstExtraData = dos_dmem_alloc(sizeof(SC_SCB_EXTRA_DATA_ST));
    }
    pthread_mutex_lock(&pstSCB->mutexSCBLock);
    if (DOS_ADDR_VALID(pstSCB->pstExtraData))
    {
        pstSCB->ulMarkStartTimeStamp = 0;
        dos_memzero(pstSCB->pstExtraData, sizeof(SC_SCB_EXTRA_DATA_ST));
        sc_ep_parse_extra_data(pstEvent, pstSCB);

        /* 用这个变量作为录音结束时间 */
        pszTmp = esl_event_get_header(pstEvent, "Event-Date-Timestamp");
        if (DOS_ADDR_VALID(pszTmp)
            && '\0' != pszTmp[0]
            && dos_atoull(pszTmp, &uLTmp) == 0)
        {
            pstSCB->pstExtraData->ulByeTimeStamp = uLTmp / 1000000;
            sc_logr_debug(pstSCB, SC_ESL, "Get extra data: Caller-Channel-Hangup-Time=%s(%u)", pszTmp, pstSCB->pstExtraData->ulByeTimeStamp);
        }
        else
        {
            DOS_ASSERT(0);
            pstSCB->pstExtraData->ulByeTimeStamp = time(NULL);
        }

        if (sc_call_check_service(pstSCB, SC_SERV_AGENT_SIGNIN))
        {
            /* 坐席长签，将bridge的时间戳作为开始时间 */
            pstSCB->pstExtraData->ulStartTimeStamp = pstSCB->pstExtraData->ulBridgeTimeStamp;
            pstSCB->pstExtraData->ulAnswerTimeStamp = pstSCB->pstExtraData->ulBridgeTimeStamp;
        }
    }
    pthread_mutex_unlock(&pstSCB->mutexSCBLock);

    pszRecordFile = esl_event_get_header(pstEvent, "Record-File-Path");
    if (DOS_ADDR_INVALID(pszRecordFile))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    for (i=0; i<SC_MAX_SRV_TYPE_PRE_LEG; i++)
    {
        aucServiceType[i] = U8_BUTT;
        if (pstSCB->aucServiceType[i] != SC_SERV_RECORDING)
        {
            aucServiceType[i] = pstSCB->aucServiceType[i];
            pstSCB->aucServiceType[i] = U8_BUTT;
        }
    }
    pstSCB->aucServiceType[0] = BS_SERV_RECORDING;
    pstSCB->bIsNotSrvAdapter = DOS_TRUE;
    usOtherSCBNo = pstSCB->usOtherSCBNo;
    pstSCB->usOtherSCBNo = U16_BUTT;

    dos_snprintf(szCMD, sizeof(szCMD)
                    , "%s-in.%s"
                    , pszRecordFile
                    , esl_event_get_header(pstEvent, "Channel-Read-Codec-Name"));
    chown(szCMD, SC_NOBODY_UID, SC_NOBODY_GID);
    chmod(szCMD, S_IXOTH|S_IWOTH|S_IROTH|S_IRUSR|S_IWUSR|S_IXUSR);

    dos_snprintf(szCMD, sizeof(szCMD)
                    , "%s-out.%s"
                    , pszRecordFile
                    , esl_event_get_header(pstEvent, "Channel-Read-Codec-Name"));
    chown(szCMD, SC_NOBODY_UID, SC_NOBODY_GID);
    chmod(szCMD, S_IXOTH|S_IWOTH|S_IROTH|S_IRUSR|S_IWUSR|S_IXUSR);

    dos_printf("Process recording file %s", szCMD);

    pstOtherSCB = sc_scb_get(usOtherSCBNo);

    if (pstSCB->ulTaskID == U32_BUTT || pstSCB->ulTaskID == 0)
    {
        if (DOS_ADDR_VALID(pstOtherSCB))
        {
            pstSCB->ulTaskID = pstOtherSCB->ulTaskID;
        }
    }

    /* 发送录音话单 */
    sc_logr_debug(pstSCB, SC_ESL, "Send CDR Record to bs. SCB1 No:%d", pstSCB->usSCBNo);
    /* 发送话单 */
    if (sc_send_billing_stop2bs(pstSCB) != DOS_SUCC)
    {
        sc_logr_debug(pstSCB, SC_ESL, "Send CDR Record to bs FAIL. SCB1 No:%d", pstSCB->usSCBNo);
    }
    else
    {
        sc_logr_debug(pstSCB, SC_ESL, "Send CDR Record to bs SUCC. SCB1 No:%d", pstSCB->usSCBNo);
    }

    /* 如果是和坐席相关的leg，则把录音文件名称删除 */
    if (pstSCB->bIsAgentCall
        && DOS_ADDR_VALID(pstSCB->pszRecordFile))
    {
        dos_dmem_free(pstSCB->pszRecordFile);
        pstSCB->pszRecordFile = NULL;
    }

    if (DOS_ADDR_VALID(pstOtherSCB)
        && pstOtherSCB->bIsAgentCall
        && DOS_ADDR_VALID(pstOtherSCB->pszRecordFile))
    {
        dos_dmem_free(pstOtherSCB->pszRecordFile);
        pstOtherSCB->pszRecordFile = NULL;
    }

    /* 将 pstSCB 中的数据还原 */
    if (DOS_ADDR_VALID(pstSCB->pstExtraData))
    {
        dos_memzero(pstSCB->pstExtraData, sizeof(SC_SCB_EXTRA_DATA_ST));
    }

    for (i=0; i<SC_MAX_SRV_TYPE_PRE_LEG; i++)
    {
        pstSCB->aucServiceType[i] = aucServiceType[i];
    }

    pstSCB->bIsNotSrvAdapter = DOS_FALSE;
    pstSCB->usOtherSCBNo = usOtherSCBNo;
    pstSCB->ulMarkStartTimeStamp = ulMarkStartTimeStamp;

finished:
    sc_logr_debug(pstSCB, SC_ESL, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_ep_session_heartbeat(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * 功能: 处理ESL的CHANNEL HEARTBEAT事件
 * 参数:
 *      esl_handle_t *pstHandle : 发送句柄
 *      esl_event_t *pstEvent   : 时间
 *      SC_SCB_ST *pstSCB       : SCB
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_session_heartbeat(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

/**
 * 函数: U32 sc_ep_process(esl_handle_t *pstHandle, esl_event_t *pstEvent)
 * 功能: 分发各种ESL事件
 * 参数:
 *      esl_handle_t *pstHandle : 发送句柄
 *      esl_event_t *pstEvent   : 时间
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_ep_process(esl_handle_t *pstHandle, esl_event_t *pstEvent)
{
    S8                     *pszUUID = NULL;
    SC_SCB_ST              *pstSCB = NULL;
    U32                    ulRet = DOS_FAIL;

    SC_TRACE_IN(pstEvent, pstHandle, 0, 0);

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstHandle))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 获取事件的UUID */
    if (ESL_EVENT_BACKGROUND_JOB == pstEvent->event_id)
    {
        g_astEPMsgStat[SC_EP_STAT_PROC].ulBGJob++;
        return sc_ep_backgroud_job_proc(pstHandle, pstEvent);
    }

    if (ESL_EVENT_RECORD_STOP == pstEvent->event_id)
    {
        g_astEPMsgStat[SC_EP_STAT_PROC].ulRecordStop++;
        return sc_ep_record_stop(pstHandle, pstEvent);
    }

    if (ESL_EVENT_CHANNEL_DESTROY == pstEvent->event_id)
    {
        g_astEPMsgStat[SC_EP_STAT_PROC].ulDestroy++;
        return sc_ep_destroy_proc(pstHandle, pstEvent);
    }

    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 系统统计 */
    sc_ep_system_stat(pstEvent);

    /* 如果不是CREATE消息，就需要获取SCB */
    if (ESL_EVENT_CHANNEL_CREATE != pstEvent->event_id)
    {
        pstSCB = sc_scb_hash_tables_find(pszUUID);
        if (DOS_ADDR_INVALID(pstSCB)
            || !pstSCB->bValid)
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
    }

    sc_logr_debug(pstSCB, SC_ESL, "Start process event: %s(%d), SCB No:%s"
                    , esl_event_get_header(pstEvent, "Event-Name")
                    , pstEvent->event_id
                    , esl_event_get_header(pstEvent, "variable_scb_number"));

    switch (pstEvent->event_id)
    {
        /* 获取呼叫状态 */
        case ESL_EVENT_CHANNEL_PARK:
            g_astEPMsgStat[SC_EP_STAT_PROC].ulPark++;
            ulRet = sc_ep_channel_park_proc(pstHandle, pstEvent, pstSCB);
            if (ulRet != DOS_SUCC)
            {
                sc_logr_info(pstSCB, SC_ESL, "Hangup for process event %s fail. UUID: %s", esl_event_get_header(pstEvent, "Event-Name"), pszUUID);
            }
            break;

        case ESL_EVENT_CHANNEL_CREATE:
            g_astEPMsgStat[SC_EP_STAT_PROC].ulCreate++;
            ulRet = sc_ep_channel_create_proc(pstHandle, pstEvent);
            if (ulRet != DOS_SUCC)
            {
                sc_ep_esl_execute("hangup", NULL, pszUUID);
                sc_logr_info(pstSCB, SC_ESL, "Hangup for process event %s fail. UUID: %s", esl_event_get_header(pstEvent, "Event-Name"), pszUUID);
            }
            break;

        case ESL_EVENT_CHANNEL_ANSWER:
            g_astEPMsgStat[SC_EP_STAT_PROC].ulAnswer++;
            ulRet = sc_ep_channel_answer(pstHandle, pstEvent, pstSCB);
            break;

        case ESL_EVENT_CHANNEL_HANGUP:
            g_astEPMsgStat[SC_EP_STAT_PROC].ulHungup++;
            ulRet = sc_ep_channel_hungup_proc(pstHandle, pstEvent, pstSCB);
            break;

        case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
            g_astEPMsgStat[SC_EP_STAT_PROC].ulHungupCom++;
            ulRet = sc_ep_channel_hungup_complete_proc(pstHandle, pstEvent, pstSCB);
            break;

        case ESL_EVENT_DTMF:
            g_astEPMsgStat[SC_EP_STAT_PROC].ulDTMF++;
            ulRet = sc_ep_dtmf_proc(pstHandle, pstEvent, pstSCB);
            break;

        case ESL_EVENT_PLAYBACK_STOP:
            ulRet = sc_ep_playback_stop(pstHandle, pstEvent, pstSCB);
            break;

        case ESL_EVENT_SESSION_HEARTBEAT:
            ulRet = sc_ep_session_heartbeat(pstHandle, pstEvent, pstSCB);
            break;

        case ESL_EVENT_CHANNEL_HOLD:
            g_astEPMsgStat[SC_EP_STAT_PROC].ulHold++;
            ulRet = sc_ep_channel_hold(pstHandle, pstEvent, pstSCB);
            break;

        case ESL_EVENT_CHANNEL_UNHOLD:
            g_astEPMsgStat[SC_EP_STAT_PROC].ulUnhold++;
            ulRet = sc_ep_channel_hold(pstHandle, pstEvent, pstSCB);
            break;

        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;

            sc_logr_info(pstSCB, SC_ESL, "Recv unhandled event: %s(%d)", esl_event_get_header(pstEvent, "Event-Name"), pstEvent->event_id);
            break;
    }

    sc_logr_debug(pstSCB, SC_ESL, "Process finished event: %s(%d). Result:%s"
                    , esl_event_get_header(pstEvent, "Event-Name")
                    , pstEvent->event_id
                    , (DOS_SUCC == ulRet) ? "Successfully" : "FAIL");

    SC_TRACE_OUT();
    return ulRet;
}

/**
 * 函数: VOID*sc_ep_process_runtime(VOID *ptr)
 * 功能: ESL事件处理主线程函数
 * 参数:
 * 返回值:
 */
VOID*sc_ep_process_runtime(VOID *ptr)
{
    DLL_NODE_S          *pstListNode = NULL;
    esl_event_t         *pstEvent = NULL;
    U32                 ulRet;
    struct timespec     stTimeout;
    SC_EP_TASK_CB       *pstEPTaskList = (SC_EP_TASK_CB*)ptr;

    if (DOS_ADDR_INVALID(pstEPTaskList))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    for (;;)
    {
        pthread_mutex_lock(&pstEPTaskList->mutexMsgList);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&pstEPTaskList->contMsgList, &pstEPTaskList->mutexMsgList, &stTimeout);
        pthread_mutex_unlock(&pstEPTaskList->mutexMsgList);

        while (1)
        {
            if (DLL_Count(&pstEPTaskList->stMsgList) <= 0)
            {
                break;
            }

            pthread_mutex_lock(&pstEPTaskList->mutexMsgList);
            pstListNode = dll_fetch(&pstEPTaskList->stMsgList);
            if (DOS_ADDR_INVALID(pstListNode))
            {
                DOS_ASSERT(0);

                pthread_mutex_unlock(&pstEPTaskList->mutexMsgList);
                continue;
            }

            pthread_mutex_unlock(&pstEPTaskList->mutexMsgList);

            if (DOS_ADDR_INVALID(pstListNode->pHandle))
            {
                DOS_ASSERT(0);
                continue;
            }

            pstEvent = (esl_event_t*)pstListNode->pHandle;

            pstListNode->pHandle = NULL;
            DLL_Init_Node(pstListNode)
            dos_dmem_free(pstListNode);

            sc_logr_debug(NULL, SC_ESL, "ESL event process START. %s(%d), SCB No:%s, Channel Name: %s"
                            , esl_event_get_header(pstEvent, "Event-Name")
                            , pstEvent->event_id
                            , esl_event_get_header(pstEvent, "variable_scb_number")
                            , esl_event_get_header(pstEvent, "Channel-Name"));

            ulRet = sc_ep_process(&g_pstHandle->stSendHandle, pstEvent);
            if (ulRet != DOS_SUCC)
            {
                DOS_ASSERT(0);
            }

            sc_logr_debug(NULL, SC_ESL, "ESL event process FINISHED. %s(%d), SCB No:%s Processed, Result: %d"
                            , esl_event_get_header(pstEvent, "Event-Name")
                            , pstEvent->event_id
                            , esl_event_get_header(pstEvent, "variable_scb_number")
                            , ulRet);

            esl_event_destroy(&pstEvent);
        }
    }

    return NULL;
}

VOID*sc_ep_process_master(VOID *ptr)
{
    DLL_NODE_S          *pstListNode = NULL;
    esl_event_t         *pstEvent = NULL;
    struct timespec     stTimeout;
    S8                  *pszUUID;
    U32                 ulSrvInd;
    S32                 i;
    static U32          ulSrvIndex = 0;
    SC_EP_TASK_CB       *pstEPTaskList = (SC_EP_TASK_CB*)ptr;
    if (DOS_ADDR_INVALID(pstEPTaskList))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    for (;;)
    {
        pthread_mutex_lock(&pstEPTaskList->mutexMsgList);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&pstEPTaskList->contMsgList, &pstEPTaskList->mutexMsgList, &stTimeout);
        pthread_mutex_unlock(&pstEPTaskList->mutexMsgList);

        while (1)
        {
            if (DLL_Count(&pstEPTaskList->stMsgList) <= 0)
            {
                break;
            }

            pthread_mutex_lock(&pstEPTaskList->mutexMsgList);

            pstListNode = dll_fetch(&pstEPTaskList->stMsgList);
            if (DOS_ADDR_INVALID(pstListNode))
            {
                DOS_ASSERT(0);

                pthread_mutex_unlock(&pstEPTaskList->mutexMsgList);
                continue;
            }

            pthread_mutex_unlock(&pstEPTaskList->mutexMsgList);

            if (DOS_ADDR_INVALID(pstListNode->pHandle))
            {
                DOS_ASSERT(0);
                dos_dmem_free(pstListNode);
                pstListNode = NULL;
                continue;
            }

            pstEvent = (esl_event_t*)pstListNode->pHandle;

            switch (pstEvent->event_id)
            {
                case ESL_EVENT_BACKGROUND_JOB:
                    g_astEPMsgStat[SC_EP_STAT_RECV].ulBGJob++;
                    break;
                /* 获取呼叫状态 */
                case ESL_EVENT_CHANNEL_PARK:
                    g_astEPMsgStat[SC_EP_STAT_RECV].ulPark++;
                    break;

                case ESL_EVENT_CHANNEL_CREATE:
                    g_astEPMsgStat[SC_EP_STAT_RECV].ulCreate++;
                    break;

                case ESL_EVENT_CHANNEL_ANSWER:
                    g_astEPMsgStat[SC_EP_STAT_RECV].ulAnswer++;
                    break;

                case ESL_EVENT_CHANNEL_HANGUP:
                    g_astEPMsgStat[SC_EP_STAT_RECV].ulHungup++;
                    break;

                case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
                    g_astEPMsgStat[SC_EP_STAT_RECV].ulHungupCom++;
                    break;

                case ESL_EVENT_DTMF:
                    g_astEPMsgStat[SC_EP_STAT_RECV].ulDTMF++;
                    break;

                case ESL_EVENT_CHANNEL_HOLD:
                    g_astEPMsgStat[SC_EP_STAT_RECV].ulHold++;
                    break;

                case ESL_EVENT_CHANNEL_UNHOLD:
                    g_astEPMsgStat[SC_EP_STAT_RECV].ulUnhold++;
                    break;

                case ESL_EVENT_RECORD_STOP:
                    g_astEPMsgStat[SC_EP_STAT_RECV].ulRecordStop++;
                    break;

                case ESL_EVENT_CHANNEL_DESTROY:
                    g_astEPMsgStat[SC_EP_STAT_RECV].ulDestroy++;
                    break;

                default:
                    break;
            }


            /* 一些消息特殊处理 */
            if (ESL_EVENT_BACKGROUND_JOB == pstEvent->event_id
                || ESL_EVENT_RECORD_STOP == pstEvent->event_id)
            {
                sc_ep_process(&g_pstHandle->stSendHandle, pstEvent);

                pstListNode->pHandle = NULL;
                DLL_Init_Node(pstListNode)
                dos_dmem_free(pstListNode);

                esl_event_destroy(&pstEvent);

                continue;
            }

#if 1
            pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
            if (DOS_ADDR_INVALID(pszUUID))
            {
                DOS_ASSERT(0);
                goto process_fail;
            }

            for (i=0; i< dos_strlen(pszUUID); i++)
            {
                ulSrvIndex += pszUUID[i];
            }

            /* 第0个位master任务，不能分配数据 */
            ulSrvInd = (ulSrvIndex % (SC_EP_TASK_NUM - 1) + 1);
#else
            if (ESL_EVENT_CHANNEL_CREATE == pstEvent->event_id)
            {
                pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
                if (DOS_ADDR_INVALID(pszUUID))
                {
                    DOS_ASSERT(0);
                    goto process_fail;
                }

                ulSrvInd = (ulSrvIndex % 2 + 1);
                ulSrvIndex++;
                dos_snprintf(szBuffCmd, sizeof(szBuffCmd), "srv_index=%d", ulSrvInd);
                sc_ep_esl_execute("set", szBuffCmd, pszUUID);
            }
            else
            {
                pszSrvIndex = esl_event_get_header(pstEvent, "variable_srv_index");
                if (DOS_ADDR_INVALID(pszUUID)
                    || dos_atoul(pszSrvIndex, &ulSrvInd) < 0)
                {
                    DOS_ASSERT(0);
                    goto process_fail;
                }
            }
#endif

            if (ulSrvInd >= SC_EP_TASK_NUM
                || SC_MASTER_TASK_INDEX == ulSrvInd)
            {
                DOS_ASSERT(0);
                goto process_fail;
            }

            pthread_mutex_lock(&g_astEPTaskList[ulSrvInd].mutexMsgList);
            DLL_Add(&g_astEPTaskList[ulSrvInd].stMsgList, pstListNode);
            pthread_cond_signal(&g_astEPTaskList[ulSrvInd].contMsgList);
            pthread_mutex_unlock(&g_astEPTaskList[ulSrvInd].mutexMsgList);

            continue;
process_fail:
            pstListNode->pHandle = NULL;
            DLL_Init_Node(pstListNode)
            dos_dmem_free(pstListNode);

            esl_event_destroy(&pstEvent);
        }
    }

    return NULL;
}


/**
 * 函数: VOID* sc_ep_runtime(VOID *ptr)
 * 功能: ESL事件接收线程主函数
 * 参数:
 * 返回值:
 */
VOID* sc_ep_runtime(VOID *ptr)
{
    U32                  ulRet = ESL_FAIL;
    DLL_NODE_S           *pstDLLNode = NULL;
    // 判断第一次连接是否成功
    static BOOL bFirstConnSucc = DOS_FALSE;

    for (;;)
    {
        /* 如果退出标志被置上，就准备退出了 */
        if (g_pstHandle->blIsWaitingExit)
        {
            sc_logr_notice(NULL, SC_ESL, "%s", "Event process exit flag has been set. the task will be exit.");
            break;
        }

        /*
         * 检查连接是否正常
         * 如果连接不正常，就准备重连
         **/
        if (!g_pstHandle->blIsESLRunning)
        {
            sc_logr_notice(NULL, SC_ESL, "%s", "ELS for event connection has been down, re-connect.");
            g_pstHandle->stRecvHandle.event_lock = 1;
            ulRet = esl_connect(&g_pstHandle->stRecvHandle, "127.0.0.1", 8021, NULL, "ClueCon");
            if (ESL_SUCCESS != ulRet)
            {
                esl_disconnect(&g_pstHandle->stRecvHandle);
                sc_logr_debug(NULL, SC_ESL, "ELS for event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_pstHandle->stRecvHandle.err);

                sleep(1);
                continue;
            }

            g_pstHandle->blIsESLRunning = DOS_TRUE;
            g_pstHandle->ulESLDebugLevel = ESL_LOG_LEVEL_WARNING;
            esl_global_set_default_logger(g_pstHandle->ulESLDebugLevel);
            esl_events(&g_pstHandle->stRecvHandle, ESL_EVENT_TYPE_PLAIN, SC_EP_EVENT_LIST);

            sc_logr_notice(NULL, SC_ESL, "%s", "ELS for event connect Back to Normal.");
        }

        if (!bFirstConnSucc)
        {
            bFirstConnSucc = DOS_TRUE;
            sc_ep_esl_execute_cmd("api reloadxml\r\n");
        }

        ulRet = esl_recv_event(&g_pstHandle->stRecvHandle, 1, NULL);
        if (ESL_FAIL == ulRet)
        {
            DOS_ASSERT(0);

            sc_logr_info(NULL, SC_ESL, "%s", "ESL Recv event fail, continue.");
            g_pstHandle->blIsESLRunning = DOS_FALSE;

            esl_disconnect(&g_pstHandle->stRecvHandle);
            esl_disconnect(&g_pstHandle->stSendHandle);
            sc_dialer_disconnect();
            continue;
        }

        esl_event_t *pstEvent = g_pstHandle->stRecvHandle.last_ievent;
        if (DOS_ADDR_INVALID(pstEvent))
        {
            DOS_ASSERT(0);

            sc_logr_info(NULL, SC_ESL, "%s", "ESL get event fail, continue.");
            g_pstHandle->blIsESLRunning = DOS_FALSE;
            continue;
        }

        sc_logr_info(NULL, SC_ESL, "ESL recv thread recv event %s(%d)."
                        , esl_event_get_header(pstEvent, "Event-Name")
                        , pstEvent->event_id);

        pstDLLNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            DOS_ASSERT(0);

            sc_logr_warning(NULL, SC_ESL, "ESL recv thread recv event %s(%d). Alloc memory fail. Drop"
                            , esl_event_get_header(pstEvent, "Event-Name")
                            , pstEvent->event_id);

            continue;
        }

        pthread_mutex_lock(&g_astEPTaskList[SC_MASTER_TASK_INDEX].mutexMsgList);
        DLL_Init_Node(pstDLLNode);
        pstDLLNode->pHandle = NULL;
        esl_event_dup((esl_event_t **)(&pstDLLNode->pHandle), pstEvent);
        DLL_Add(&g_astEPTaskList[SC_MASTER_TASK_INDEX].stMsgList, pstDLLNode);

        pthread_cond_signal(&g_astEPTaskList[SC_MASTER_TASK_INDEX].contMsgList);
        pthread_mutex_unlock(&g_astEPTaskList[SC_MASTER_TASK_INDEX].mutexMsgList);
    }

    /* @TODO 释放资源 */
    return NULL;
}

/* 初始化事件处理模块 */
U32 sc_ep_init()
{
    S32 i;
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstHandle = dos_dmem_alloc(sizeof(SC_EP_HANDLE_ST));
    g_pstHashGWGrp = hash_create_table(SC_GW_GRP_HASH_SIZE, NULL);
    g_pstHashGW = hash_create_table(SC_GW_HASH_SIZE, NULL);
    g_pstHashDIDNum = hash_create_table(SC_IP_DID_HASH_SIZE, NULL);
    g_pstHashSIPUserID = hash_create_table(SC_IP_USERID_HASH_SIZE, NULL);
    g_pstHashBlackList = hash_create_table(SC_BLACK_LIST_HASH_SIZE, NULL);
    g_pstHashTTNumber = hash_create_table(SC_TT_NUMBER_HASH_SIZE, NULL);
    g_pstHashCaller = hash_create_table(SC_CALLER_HASH_SIZE, NULL);
    g_pstHashCallerGrp = hash_create_table(SC_CALLER_GRP_HASH_SIZE, NULL);
    g_pstHashNumberlmt = hash_create_table(SC_NUMBER_LMT_HASH_SIZE, NULL);
    g_pstHashCallerSetting = hash_create_table(SC_CALLER_SETTING_HASH_SIZE, NULL);
    g_pstHashServCtrl = hash_create_table(SC_SERV_CTRL_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstHandle)
        || DOS_ADDR_INVALID(g_pstHashGW)
        || DOS_ADDR_INVALID(g_pstHashGWGrp)
        || DOS_ADDR_INVALID(g_pstHashDIDNum)
        || DOS_ADDR_INVALID(g_pstHashSIPUserID)
        || DOS_ADDR_INVALID(g_pstHashBlackList)
        || DOS_ADDR_INVALID(g_pstHashCaller)
        || DOS_ADDR_INVALID(g_pstHashCallerGrp)
        || DOS_ADDR_INVALID(g_pstHashTTNumber)
        || DOS_ADDR_INVALID(g_pstHashNumberlmt)
        || DOS_ADDR_INVALID(g_pstHashServCtrl))
    {
        DOS_ASSERT(0);

        goto init_fail;
    }


    for (i = 0; i < SC_EP_TASK_NUM; i++)
    {
        pthread_mutex_init(&g_astEPTaskList[i].mutexMsgList, NULL);
        pthread_cond_init(&g_astEPTaskList[i].contMsgList, NULL);
        DLL_Init(&g_astEPTaskList[i].stMsgList);
    }

    dos_memzero(g_pstHandle, sizeof(SC_EP_HANDLE_ST));
    g_pstHandle->blIsESLRunning = DOS_FALSE;
    g_pstHandle->blIsWaitingExit = DOS_FALSE;

    dos_memzero(g_astEPMsgStat, sizeof(g_astEPMsgStat));

    DLL_Init(&g_stEventList)
    DLL_Init(&g_stRouteList);
    DLL_Init(&g_stNumTransformList);
    DLL_Init(&g_stCustomerList);

    /* 以下三项加载顺序不能更改 */
    sc_load_gateway(SC_INVALID_INDEX);
    sc_load_gateway_grp(SC_INVALID_INDEX);
    sc_load_gw_relationship();

    sc_load_route(SC_INVALID_INDEX);
    sc_load_num_transform(SC_INVALID_INDEX);
    sc_load_customer(SC_INVALID_INDEX);
    sc_load_did_number(SC_INVALID_INDEX);
    sc_load_sip_userid(SC_INVALID_INDEX);
    sc_load_black_list(SC_INVALID_INDEX);
    sc_load_tt_number(SC_INVALID_INDEX);

    /* 以下三项的加载顺序同样不可乱,同时必须保证之前已经加载DID号码(号码组业务逻辑) */
    sc_load_caller(SC_INVALID_INDEX);
    sc_load_caller_grp(SC_INVALID_INDEX);
    sc_load_number_lmt(SC_INVALID_INDEX);
    sc_load_serv_ctrl(SC_INVALID_INDEX);

    sc_load_caller_relationship();

    sc_load_caller_setting(SC_INVALID_INDEX);
    SC_TRACE_OUT();
    return DOS_SUCC;
init_fail:

    /* TODO: 释放资源 */
    return DOS_FAIL;
}

/* 启动事件处理模块 */
U32 sc_ep_start()
{
    S32 i;

    SC_TRACE_IN(0, 0, 0, 0);

    for (i=0; i < SC_EP_TASK_NUM; i++)
    {
        if (SC_MASTER_TASK_INDEX == i)
        {
            if (pthread_create(&g_astEPTaskList[i].pthTaskID, NULL, sc_ep_process_master, &g_astEPTaskList[i]) < 0)
            {
                SC_TRACE_OUT();
                return DOS_FAIL;
            }
        }
        else
        {
            if (pthread_create(&g_astEPTaskList[i].pthTaskID, NULL, sc_ep_process_runtime, &g_astEPTaskList[i]) < 0)
            {
                SC_TRACE_OUT();
                return DOS_FAIL;
            }
        }
    }

    if (pthread_create(&g_pstHandle->pthID, NULL, sc_ep_runtime, NULL) < 0)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/* 停止事件处理模块 */
U32 sc_ep_shutdown()
{
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstHandle->blIsWaitingExit = DOS_TRUE;

    SC_TRACE_OUT();
    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */



