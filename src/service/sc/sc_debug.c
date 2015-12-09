/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名：sc_debug.c
 *
 *  创建时间: 2014年12月16日15:16:28
 *  作    者: Larry
 *  描    述: 业务控制模块调试函数
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <esl.h>
#include <hash/hash.h>

/* include private header files */
#include "sc_httpd.h"
#include "sc_http_api.h"
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"
#include "sc_acd.h"
#include "sc_ep.h"

/* define marcos */

/* define enums */

/* define structs */

/* declare global params */
U32       g_ulSCLogLevel        = LOG_LEVEL_DEBUG;       /* 日志级别 */

U32       g_ulTaskTraceAll      = 0;       /* 是否跟踪所有任务 */

U32       g_ulCallTraceAll      = 1;     /* 跟踪所有的呼叫 */



#ifdef DEBUG_VERSION
U32       g_ulTraceFlags        = 0xFFFFFFFF;
#else
U32       g_ulTraceFlags        = 0;
#endif



extern SC_HTTPD_CB_ST        *g_pstHTTPDList[SC_MAX_HTTPD_NUM];
extern SC_HTTP_CLIENT_CB_S   *g_pstHTTPClientList[SC_MAX_HTTP_CLIENT_NUM];
extern HASH_TABLE_S          *g_pstAgentList;
extern SC_TASK_MNGT_ST       *g_pstTaskMngtInfo;
extern HASH_TABLE_S          *g_pstGroupList;
extern HASH_TABLE_S          *g_pstAgentList;
extern pthread_mutex_t       g_mutexAgentList;
extern HASH_TABLE_S          *g_pstHashGW;
extern HASH_TABLE_S          *g_pstHashSIPUserID;
extern HASH_TABLE_S          *g_pstHashGWGrp;
extern HASH_TABLE_S          *g_pstHashDIDNum;
extern HASH_TABLE_S          *g_pstHashBlackList;
extern HASH_TABLE_S          *g_pstHashCaller;
extern HASH_TABLE_S          *g_pstHashCallerGrp;
extern HASH_TABLE_S          *g_pstHashCallerSetting;
extern HASH_TABLE_S          *g_pstHashTTNumber;
extern HASH_TABLE_S          *g_pstHashNumberlmt;
extern HASH_TABLE_S           *g_pstHashServCtrl;
extern pthread_mutex_t        g_mutexHashServCtrl;
extern pthread_mutex_t        g_mutexHashDIDNum;
extern pthread_mutex_t        g_mutexHashBlackList;
extern pthread_mutex_t        g_mutexRouteList;
extern DLL_S                  g_stRouteList;
extern DLL_S                  g_stEventList;
extern DLL_S                  g_stCustomerList;
extern DLL_S                  g_stNumTransformList;
extern DLL_S                  g_stCWQMngt;
extern SC_DIALER_HANDLE_ST   *g_pstDialerHandle;
extern DLL_S                  g_stBSMsgList;
extern SC_EP_TASK_CB          g_astEPTaskList[SC_EP_TASK_NUM];
extern U32                    g_ulCPS;
extern SC_EP_MSG_STAT_ST      g_astEPMsgStat[2];
extern SC_BS_MSG_STAT_ST      stBSMsgStat;



/* declare functions */
extern SC_TASK_CB_ST *sc_tcb_get_by_id(U32 ulTCBNo);


const S8* g_pszAgentBindType[] =
{
    "SIP",
    "TELEPHONE",
    "MOBILE",
    "TT_NUMBER"
};

const S8* g_pszCallServiceType[] =
{
    "OUTBOUND CALL",
    "INBOUND CALL",
    "INTERNAL CALL",
    "EXTERNAL CALL",
    "AUTO DIALING",
    "PREVIEW DIALING",
    "PREDICTIVE DIALING",
    "RECORDING",
    "FORWARD CFB",
    "FORWARD CFU",
    "FORWARD CFNR",
    "BLIND TRANSFER",
    "ATTEND_TRANSFER",
    "PICK UP",
    "CONFERENCE",
    "VOICE MAIL RECORD",
    "VOICE MAIL GET",
    "SMS RECV",
    "SMS SEND",
    "MMS RECV",
    "MMS SEND",
    "FAX",
    "INTERNAL SERVICE",
    "AGENT CALLBACK",
    "AGENT SIGNIN",
    "NUM VERIFY"
};

const S8* g_pszRouteDestType[] =
{
    "",
    "TRUNK",
    "GWGRP"
};

const S8* g_pszDidBindType[] =
{
    "",
    "SIP",
    "AGENT-QUEUE",
    "AGENT",

};

const S8* g_pszTaskStatus[] =
{
    "INIT",
    "WORKING",
    "STOP",
    "PAUSED",
    "BUSY",
    "ALERT",
    "EMERG"
};

const S8* g_pszTaskMode[] =
{
    "KEY4AGENT",
    "KEY4AGENT1",
    "DIRECT4AGENT",
    "AUDIO-ONLY",
    "AGENT-AFTER-AUDIO"
};

const S8* g_pszAgentStatus[] =
{
    "OFFLINE",
    "IDLE",
    "AWAY",
    "BUSY",
    "PROC"
};

const S8* g_pszCallerType[] =
{
    "CALLER",
    "DID"
};

const S8* g_pszTransformObject[] =
{
    "SYSTEM",
    "TRUNK",
    "CUSTOMER"
};

const S8* g_pszTransformDirection[] =
{
    "IN",
    "OUT"
};

const S8* g_szTransformTime[] =
{
    "BEFORE",
    "AFTER"
};

const S8* g_pszTransformNumSelect[] =
{
    "CALLER",
    "CALLEE"
};

const S8* g_pszDstCallerType[] =
{
    "CALLER",
    "DID",
    "CALLERGRP"
};

const S8* g_pszCallerPolicy[] =
{
    "IN-ORDER",
    "RANDOM"
};

const S8* sc_translate_agent_bind_type(U32 ulBindType)
{
    if (ulBindType < sizeof(g_pszAgentBindType) / sizeof(S8*))
    {
        return g_pszAgentBindType[ulBindType];
    }
    return "UNKNOWN";
}

const S8* sc_translate_call_service_type(U32 ulServiceType)
{
    if (ulServiceType < sizeof(g_pszCallServiceType) / sizeof(S8*))
    {
        return g_pszCallServiceType[ulServiceType];
    }
    return "UNKNOWN";
}

const S8* sc_translate_route_dest_type(U32 ulDestType)
{
    if (ulDestType < sizeof(g_pszRouteDestType) / sizeof(S8*))
    {
        return g_pszRouteDestType[ulDestType];
    }
    return "UNKNOWN";
}

const S8* sc_translate_did_bind_type(U32 ulBindType)
{
    if (ulBindType < sizeof(g_pszDidBindType) / sizeof(S8*))
    {
        return g_pszDidBindType[ulBindType];
    }
    return "UNKNOWN";
}

const S8* sc_translate_task_status(U32 ulStatus)
{
    if (ulStatus < sizeof(g_pszTaskStatus) / sizeof(S8 *))
    {
        return g_pszTaskStatus[ulStatus];
    }
    return "UNKNOWN";
}

const S8* sc_translate_task_mode(U32 ulMode)
{
    if (ulMode < sizeof(g_pszTaskMode) / sizeof(S8*))
    {
        return g_pszTaskMode[ulMode];
    }
    return "UNKNOWN";
}

const S8* sc_translate_agent_status(U32 ulStatus)
{
    if (ulStatus < sizeof(g_pszAgentStatus)/sizeof(S8*))
    {
        return g_pszAgentStatus[ulStatus];
    }
    return "UNKNOWN";
}

const S8* sc_translate_caller_type(U32 ulType)
{
    if (ulType < sizeof(g_pszCallerType)/sizeof(S8*))
    {
        return g_pszCallerType[ulType];
    }
    return "UNKNOWN";
}

const S8* sc_translate_transform_object(U32 ulObject)
{
    if (ulObject < sizeof(g_pszTransformObject)/sizeof(S8*))
    {
        return g_pszTransformObject[ulObject];
    }
    return "UNKNOWN";
}

const S8* sc_translate_transform_direction(U32 ulDirection)
{
    if (ulDirection < sizeof(g_pszTransformDirection)/sizeof(S8*))
    {
        return g_pszTransformDirection[ulDirection];
    }
    return "UNKNOWN";
}

const S8* sc_translate_transform_time(U32 ulTime)
{
    if (ulTime < sizeof(g_szTransformTime)/sizeof(S8*))
    {
        return g_szTransformTime[ulTime];
    }
    return "UNKNOWN";
}

const S8* sc_translate_transfrom_numselect(U32 ulNumSelect)
{
    if (ulNumSelect < sizeof(g_pszTransformNumSelect)/sizeof(S8*))
    {
        return g_pszTransformNumSelect[ulNumSelect];
    }
    return "UNKNOWN";
}

const S8* sc_translate_caller_srctype(U32 ulSrcType)
{
    switch (ulSrcType)
    {
        case SC_SRC_CALLER_TYPE_AGENT:
            return "AGENT";
        case SC_SRC_CALLER_TYPE_AGENTGRP:
            return "AGENTGRP";
        case SC_SRC_CALLER_TYPE_ALL:
            return "ALL";
        default:
            break;
    }
    return "UNKNOWN";
}

const S8* sc_translate_caller_dsttype(U32 ulDstType)
{
    if (ulDstType < sizeof(g_pszDstCallerType)/sizeof(S8*))
    {
        return g_pszDstCallerType[ulDstType];
    }
    return "UNKNOWN";
}

const S8* sc_translate_caller_policy(U32 ulPolicy)
{
    if (ulPolicy < sizeof(g_pszCallerPolicy)/sizeof(S8*))
    {
        return g_pszCallerPolicy[ulPolicy];
    }
    return "UNKNOWN";
}

S32 sc_cc_show_agent_stat(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulAgentID    = U32_BUTT;
    U32 ulGroupID    = U32_BUTT;
    U32 ulCustoemrID = U32_BUTT;
    U32 ulHashIndex  = U32_BUTT;
    S8                           szCmdBuff[300]     = {0, };
    HASH_NODE_S                  *pstHashNode       = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;

    if (argc == 2)
    {
        /* scd stat agent */
    }
    else if (argc == 3)
    {
        /* scd stat agent id */
        if (dos_atoul(argv[2], &ulAgentID) < 0)
        {
            goto process_fail;
        }
    }
    else if (argc == 4)
    {
        /* scd stat agent customer|group id */
        if (dos_strnicmp(argv[2], "customer", dos_strlen("customer")) == 0)
        {
            /* scd stat agent id */
            if (dos_atoul(argv[3], &ulCustoemrID) < 0)
            {
                goto process_fail;
            }
        }
        else if (dos_strnicmp(argv[2], "group", dos_strlen("group")) == 0)
        {
            /* scd stat agent id */
            if (dos_atoul(argv[3], &ulGroupID) < 0)
            {
                goto process_fail;
            }
        }
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nShow agent stat:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%11s%9s%11s%11s%11s%11s%11s%11s%11s"
                          , "ID", "Num", "Group1", "Group2", "Calls"
                          , "Connected", "Duration", "TimeLogin", "TimeOnPhone");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n-------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);

    pthread_mutex_lock(&g_mutexAgentList);
    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstAgentQueueNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentQueueNode)
                || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
            {
                continue;
            }


            dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                                  , "\r\n%11u%9s%11u%11u%11u%11u%11u%11u%11u"
                                  , pstAgentQueueNode->pstAgentInfo->ulSiteID
                                  , pstAgentQueueNode->pstAgentInfo->szEmpNo
                                  , pstAgentQueueNode->pstAgentInfo->aulGroupID[0]
                                  , pstAgentQueueNode->pstAgentInfo->aulGroupID[1]
                                  , pstAgentQueueNode->pstAgentInfo->stStat.ulCallCnt
                                  , pstAgentQueueNode->pstAgentInfo->stStat.ulCallConnected
                                  , pstAgentQueueNode->pstAgentInfo->stStat.ulTotalDuration
                                  , pstAgentQueueNode->pstAgentInfo->stStat.ulTimesSignin
                                  , pstAgentQueueNode->pstAgentInfo->stStat.ulTimesOnline);

            if (ulAgentID != U32_BUTT)
            {
                if (ulAgentID == pstAgentQueueNode->pstAgentInfo->ulSiteID)
                {
                    /* 单个坐席的情况，打印就退出其 */
                    cli_out_string(ulIndex, szCmdBuff);
                    break;
                }

                continue;
            }

            if (ulCustoemrID != U32_BUTT)
            {
                if (ulCustoemrID == pstAgentQueueNode->pstAgentInfo->ulCustomerID)
                {
                    /* 单个坐席的 */
                    cli_out_string(ulIndex, szCmdBuff);
                }

                continue;
            }

            if (ulGroupID != U32_BUTT)
            {
                if (ulGroupID == pstAgentQueueNode->pstAgentInfo->aulGroupID[0]
                   || ulGroupID == pstAgentQueueNode->pstAgentInfo->aulGroupID[1])
                {
                    cli_out_string(ulIndex, szCmdBuff);
                }

                continue;
            }

            /* 显示所有的哦 */
            cli_out_string(ulIndex, szCmdBuff);
        }
    }
    pthread_mutex_unlock(&g_mutexAgentList);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n-------------------------------------------------------------------------------------------------\r\n\r\n");
    cli_out_string(ulIndex, szCmdBuff);

    return 0;

process_fail:
    cli_out_string(ulIndex, "\r\nUsage:");
    cli_out_string(ulIndex, "\r\n    scd stat agent [customer|group] id\r\n\r\n");

    return 0;
}

VOID sc_show_serv_ctrl(U32 ulIndex, U32 ulCustomer)
{
    HASH_NODE_S         *pstHashNode = NULL;
    SC_SRV_CTRL_ST      *pstSrvCtrl  = NULL;
    U32                 ulHashIndex;
    S8                  szCmdBuff[300] = {0, };

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nService Control Rules:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%11s%11s%11s%11s%11s%11s%11s%11s%11s", "ID", "Customer", "Service", "Effect", "Expire", "Attr1", "Attr2", "AttrVal1", "AttrVal2");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n----------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);

    pthread_mutex_lock(&g_mutexHashServCtrl);

    HASH_Scan_Table(g_pstHashServCtrl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashServCtrl, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstSrvCtrl = pstHashNode->pHandle;

            if (0 != ulCustomer && ulCustomer != pstSrvCtrl->ulCustomerID)
            {
                continue;
            }

            dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%11u%11u%11u%11u%11u%11u%11u%11u%11u"
                        , pstSrvCtrl->ulID
                        , pstSrvCtrl->ulCustomerID
                        , pstSrvCtrl->ulServType
                        , pstSrvCtrl->ulEffectTimestamp
                        , pstSrvCtrl->ulExpireTimestamp
                        , pstSrvCtrl->ulAttr1
                        , pstSrvCtrl->ulAttr2
                        , pstSrvCtrl->ulAttrValue1
                        , pstSrvCtrl->ulAttrValue2);
            cli_out_string(ulIndex, szCmdBuff);
        }
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n----------------------------------------------------------------------------------------------------\r\n\r\n");
    cli_out_string(ulIndex, szCmdBuff);

    pthread_mutex_unlock(&g_mutexHashServCtrl);
}

/**
 * 函数: VOID sc_debug_show_httpd(U32 ulIndex)
 * 功能: 打印http server的信息
 * 参数:
 *  U32 ulIndex  telnet客户端索引
 */
VOID sc_show_httpd(U32 ulIndex, U32 ulID)
{
    U32 ulHttpdIndex, ulActive;
    S8 szCmdBuff[1024] = {0, };
    S8 szIPAddr[32] = {0, };

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nHTTP Server List for WEB CMD:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%10s%32s%16s%10s%10s", "Index", "IP Address", "Port", "Req Cnt", "Status");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n--------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);

    for (ulActive=0, ulHttpdIndex=0; ulHttpdIndex<SC_MAX_HTTPD_NUM; ulHttpdIndex++)
    {
        if (!g_pstHTTPDList[ulHttpdIndex])
        {
            continue;
        }

        if (U32_BUTT != ulID && ulID != ulHttpdIndex)
        {
            continue;
        }

        if (!dos_ipaddrtostr(g_pstHTTPDList[ulHttpdIndex]->aulIPAddr[0], szIPAddr, sizeof(szIPAddr)))
        {
            szIPAddr[0] = '\0';
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%10u%32s%16u%10u%10s"
                        , g_pstHTTPDList[ulHttpdIndex]->ulIndex
                        , '\0' == szIPAddr[0] ? "NULL" : szIPAddr
                        , g_pstHTTPDList[ulHttpdIndex]->usPort
                        , g_pstHTTPDList[ulHttpdIndex]->ulReqCnt
                        , g_pstHTTPDList[ulHttpdIndex]->ulStatus ? "Active" : "Inactive");
        cli_out_string(ulIndex, szCmdBuff);
    }
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n--------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal: %d, Active: %d.\r\n", ulHttpdIndex, ulActive);
    cli_out_string(ulIndex, szCmdBuff);
}

/**
 * 函数: VOID sc_debug_show_http(U32 ulIndex)
 * 功能: 打印http 客户端的信息
 * 参数:
 *  U32 ulIndex  telnet客户端索引
 */
VOID sc_show_http(U32 ulIndex, U32 ulID)
{
    S8 szCmdBuff[1024] = {0, };
    U32 ulHttpClientIndex = 0;

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nHTTP Client List for WEB CMD:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%16s%16s%16s", "Index", "Valid", "Server Index");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);

    for (ulHttpClientIndex=0; ulHttpClientIndex<SC_MAX_HTTP_CLIENT_NUM; ulHttpClientIndex++)
    {
        if (U32_BUTT != ulID && ulID != ulHttpClientIndex)
        {
            continue;
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%16u%16u%16u"
                        , g_pstHTTPClientList[ulHttpClientIndex]->ulIndex
                        , g_pstHTTPClientList[ulHttpClientIndex]->ulValid
                        , g_pstHTTPClientList[ulHttpClientIndex]->ulCurrentSrv);
        cli_out_string(ulIndex, szCmdBuff);
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal: %d.\r\n", ulHttpClientIndex);
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_scb_all(U32 ulIndex)
{
    SC_SCB_ST   *pstSCB = NULL;
    U32 i, ulCnt = 0;
    S8  szCmdBuff[1024] = {0, };

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nShow the SCB List:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%7s%10s%12s%12s%7s%12s%24s%24s"
                    , "Index", "Ref Index", "Cusomer", "Agent", "Status", "Trunk ID", "Caller", "Callee");
    cli_out_string(ulIndex, szCmdBuff);

    for (i=0, ulCnt=0; i<SC_MAX_SCB_NUM; i++)
    {
        pstSCB = &g_pstTaskMngtInfo->pstCallSCBList[i];
        if (!pstSCB->bValid)
        {
            continue;
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%7u%10u%12u%12u%7s%12u%24s%24s"
                        , pstSCB->usSCBNo
                        , pstSCB->usOtherSCBNo
                        , pstSCB->ulCustomID == U32_BUTT ? 0 : pstSCB->ulCustomID
                        , pstSCB->ulAgentID == U32_BUTT ? 0 : pstSCB->ulAgentID
                        , sc_scb_get_status(pstSCB->ucStatus)
                        , pstSCB->ulTrunkID == U32_BUTT ? 0 : pstSCB->ulTrunkID
                        , pstSCB->szCallerNum
                        , pstSCB->szCalleeNum);
        cli_out_string(ulIndex, szCmdBuff);
        ulCnt++;
    }
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------------\r\n\r\n");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal: %u\r\n\r\n", ulCnt);
    cli_out_string(ulIndex, szCmdBuff);
}

static S8* sc_debug_make_weeks(U32 ulWeekMask, S8 *pszWeeks, U32 ulLength)
{
    U32 ulCurrLen = 0;

    if (DOS_ADDR_INVALID(pszWeeks) || 0 == ulLength)
    {
        return "\0";
    }

    if (ulWeekMask & 0x00000001)
    {
        ulCurrLen = dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Sun. ");
    }

    if (ulWeekMask & 0x00000002)
    {
        ulCurrLen += dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Mon. ");
    }

    if (ulWeekMask & 0x00000004)
    {
        ulCurrLen += dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Tus. ");
    }

    if (ulWeekMask & 0x00000008)
    {
        ulCurrLen += dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Wen. ");
    }

    if (ulWeekMask & 0x00000010)
    {
        ulCurrLen += dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Thur. ");
    }

    if (ulWeekMask & 0x00000020)
    {
        ulCurrLen += dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Fri. ");
    }

    if (ulWeekMask & 0x00000040)
    {
        ulCurrLen += dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Sat. ");
    }

    return pszWeeks;
}

VOID sc_show_task_list(U32 ulIndex, U32 ulCustomID)
{
    S8 szCmdBuff[1024] = { 0, };
    U32 ulTaskIndex, ulTotal;
    SC_TASK_CB_ST *pstTCB;

    if (U32_BUTT == ulCustomID)
    {
        cli_out_string(ulIndex, "\r\nCli Show Task List: ");
    }
    else
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nCli Show Task List for Customer %u: ", ulCustomID);

        cli_out_string(ulIndex, szCmdBuff);
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n--------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%6s%7s%9s%6s%10s%10s%10s%12s", "No.", "Status", "Priority", "Trace", "ID", "Custom-ID", "Agent-Cnt", "Caller-Cnt");
    cli_out_string(ulIndex, szCmdBuff);

    for (ulTaskIndex=0,ulTotal=0; ulTaskIndex < SC_MAX_TASK_NUM; ulTaskIndex++)
    {
        pstTCB = &g_pstTaskMngtInfo->pstTaskList[ulTaskIndex];
        if (DOS_ADDR_INVALID(pstTCB))
        {
            continue;
        }

        if (!pstTCB->ucValid)
        {
            continue;
        }

        if (U32_BUTT != ulCustomID && ulCustomID != pstTCB->ulCustomID)
        {
            continue;
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%6u%7u%9u%6s%10u%10u%10u%12u"
                        , pstTCB->usTCBNo
                        , pstTCB->ucTaskStatus
                        , pstTCB->ucPriority
                        , pstTCB->bTraceON ? "Y" : "N"
                        , pstTCB->ulTaskID
                        , pstTCB->ulCustomID
                        , pstTCB->usSiteCount
                        , pstTCB->usCallerCount);
        cli_out_string(ulIndex, szCmdBuff);
        ulTotal++;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n--------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal : %u\r\n", ulTotal);
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_task(U32 ulIndex, U32 ulTaskID, U32 ulCustomID)
{
    S8 szCmdBuff[1024] = {0, };
    S8 szWeeks[64] = {0,};
    SC_TASK_CB_ST *pstTCB = NULL;
    //U32 i = 0;

    /* 如果没有指定task id，或者指定了customer id，就需要使用列表的形式显示任务概要 */
    if (U32_BUTT == ulTaskID || U32_BUTT != ulCustomID)
    {
        sc_show_task_list(ulIndex, ulCustomID);
        return ;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        cli_out_string(ulIndex, "\r\nError:"
                                "\r\n    Invalid task ID"
                                "\r\n    Task with the ID is not valid. "
                                "\r\n    Please use \"sc show task custom id \" to ckeck a valid task\r\n");
        return;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                , "\r\nTask Detail: "
                  "\r\n---------------Detail Information---------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                , "\r\n                   ID : %u"
                  "\r\n               Status : %s"
                  "\r\n             Priority : %u"
                  "\r\n  Voice File Play Cnt : %u"
                  "\r\n      Voice File Path : %s"
                  "\r\n                Trace : %s"
                  "\r\n           Trace Call : %s"
                  "\r\n             ID in DB : %u"
                  "\r\n          Customer ID : %u"
                  "\r\n                 Mode : %u"
                  "\r\n  Current Concurrency : %u"
                  "\r\n      Max Concurrency : %u"
                  "\r\n          Agent Count : %u"
                  "\r\n         Caller Count : %u"
                  "\r\n         Callee Count : %u"
                  "\r\n    Last Callee Index : %u"
                  "\r\n       Agent Queue ID : %u"
                  "\r\n      Caller Group ID : %u"
                  "\r\n            Call Rate : %u"
                  "\r\nTime Period 1 Weekday : %s"
                  "\r\n        Time Period 1 : %u:%u - %u:%u"
                  "\r\nTime Period 2 Weekday : %s"
                  "\r\n        Time Period 2 : %u:%u - %u:%u"
                  "\r\nTime Period 3 Weekday : %s"
                  "\r\n        Time Period 3 : %u:%u - %u:%u"
                  "\r\nTime Period 4 Weekday : %s"
                  "\r\n        Time Period 4 : %u:%u - %u:%u"
                , pstTCB->ulTaskID
                , sc_translate_task_status(pstTCB->ucTaskStatus)
                , pstTCB->ucPriority
                , pstTCB->ucAudioPlayCnt
                , pstTCB->szAudioFileLen
                , pstTCB->bTraceON ? "Y" : "N"
                , pstTCB->bTraceCallON ? "Y" : "N"
                , pstTCB->ulTaskID
                , pstTCB->ulCustomID
                , pstTCB->ucMode
                , pstTCB->ulCurrentConcurrency
                , pstTCB->ulMaxConcurrency
                , pstTCB->usSiteCount
                , pstTCB->usCallerCount
                , pstTCB->ulCalleeCount
                , pstTCB->ulLastCalleeIndex
                , pstTCB->ulAgentQueueID
                , pstTCB->ulCallerGrpID
                , pstTCB->ulCallRate
                , sc_debug_make_weeks(pstTCB->astPeriod[0].ucWeekMask, szWeeks, sizeof(szWeeks))
                , pstTCB->astPeriod[0].ucHourBegin
                , pstTCB->astPeriod[0].ucMinuteBegin
                , pstTCB->astPeriod[0].ucHourEnd
                , pstTCB->astPeriod[0].ucMinuteEnd
                , sc_debug_make_weeks(pstTCB->astPeriod[1].ucWeekMask, szWeeks, sizeof(szWeeks))
                , pstTCB->astPeriod[1].ucHourBegin
                , pstTCB->astPeriod[1].ucMinuteBegin
                , pstTCB->astPeriod[1].ucHourEnd
                , pstTCB->astPeriod[1].ucMinuteEnd
                , sc_debug_make_weeks(pstTCB->astPeriod[2].ucWeekMask, szWeeks, sizeof(szWeeks))
                , pstTCB->astPeriod[2].ucHourBegin
                , pstTCB->astPeriod[2].ucMinuteBegin
                , pstTCB->astPeriod[2].ucHourEnd
                , pstTCB->astPeriod[2].ucMinuteEnd
                , sc_debug_make_weeks(pstTCB->astPeriod[3].ucWeekMask, szWeeks, sizeof(szWeeks))
                , pstTCB->astPeriod[3].ucHourBegin
                , pstTCB->astPeriod[3].ucMinuteBegin
                , pstTCB->astPeriod[3].ucHourEnd
                , pstTCB->astPeriod[3].ucMinuteEnd);
    cli_out_string(ulIndex, szCmdBuff);

#if 0
    cli_out_string(ulIndex, "\r\nList Caller.");
    for (i = 0; i < SC_MAX_CALLER_NUM; i++)
    {
        if (DOS_ADDR_INVALID(pstTCB->pstCallerNumQuery)
            || !pstTCB->pstCallerNumQuery[i].bValid)
        {
            continue;
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%6u%6s%10u%16s"
                        , pstTCB->pstCallerNumQuery[i].usNo
                        , pstTCB->pstCallerNumQuery[i].bTraceON ? "Y" : "N"
                        , pstTCB->pstCallerNumQuery[i].ulIndexInDB
                        , pstTCB->pstCallerNumQuery[i].szNumber);
        cli_out_string(ulIndex, szCmdBuff);
    }
#endif

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff),
                  "\r\n----------------Stat Information----------------");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                , "\r\n       Total Call(s) : %u"
                  "\r\n         Call Failed : %u"
                  "\r\n      Call Connected : %u"
                , pstTCB->ulTotalCall
                , pstTCB->ulCallFailed
                , pstTCB->ulCallConnected);
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff),
                  "\r\n------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);

}

VOID sc_show_agent_group_detail(U32 ulIndex, U32 ulID)
{
    SC_ACD_GRP_HASH_NODE_ST      *pstAgentGrouop = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    HASH_NODE_S  *pstHashNode = NULL;
    DLL_NODE_S   *pstDLLNode  = NULL;
    U32 ulHashIndex = 0;
    S8  szCmdBuff[1024] = {0, };

    if (U32_BUTT == ulID)
    {
        return;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nThe detail info for the Agent Group %u:", ulID);
    cli_out_string(ulIndex, szCmdBuff);

    sc_acd_hash_func4grp(ulID, &ulHashIndex);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashIndex, &ulID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n\r\n\tERROR: Cannot find the agent group with id %u!", ulID);
        cli_out_string(ulIndex, szCmdBuff);
        return;
    }

    pstAgentGrouop = pstHashNode->pHandle;

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n----------Group Parameters----------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n           Index:%u", pstAgentGrouop->ulGroupID);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n            Name:%u", pstAgentGrouop->ulGroupID);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n     Customer ID:%u", pstAgentGrouop->ulCustomID);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n     Agent Count:%u", pstAgentGrouop->usCount);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n      ACD Policy:%u", pstAgentGrouop->ucACDPolicy);
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n      ACD Policy:%u", pstAgentGrouop->ucACDPolicy);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n\r\nGroup Members");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n---------------------------------------------------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%10s%10s%10s%10s%10s%8s%7s%8s%12s%12s%12s%9s%10s%12s%12s"
                    , "ID", "Status", "Custom", "Group1", "Group2"
                    , "Record", "Trace", "Leader", "SIP Acc", "Extension", "Emp NO.", "CallCnt", "Bind", "Telephone", "Mobile");
    cli_out_string(ulIndex, szCmdBuff);

    DLL_Scan(&pstAgentGrouop->stAgentList, pstDLLNode, DLL_NODE_S *)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            continue;
        }

        pstAgentQueueNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
        {
            continue;
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%10u%10s%10u%10u%10u%8s%7s%8s%12s%12s%12s%9u%10s%12s%12s"
                    , pstAgentQueueNode->pstAgentInfo->ulSiteID
                    , sc_translate_agent_status(pstAgentQueueNode->pstAgentInfo->ucStatus)
                    , pstAgentQueueNode->pstAgentInfo->ulCustomerID
                    , pstAgentQueueNode->pstAgentInfo->aulGroupID[0]
                    , pstAgentQueueNode->pstAgentInfo->aulGroupID[1]
                    , pstAgentQueueNode->pstAgentInfo->bRecord ? "Y" : "N"
                    , pstAgentQueueNode->pstAgentInfo->bTraceON ? "Y" : "N"
                    , pstAgentQueueNode->pstAgentInfo->bGroupHeader ? "Y" : "N"
                    , pstAgentQueueNode->pstAgentInfo->szUserID
                    , pstAgentQueueNode->pstAgentInfo->szExtension
                    , pstAgentQueueNode->pstAgentInfo->szEmpNo
                    , pstAgentQueueNode->pstAgentInfo->ulCallCnt
                    , sc_translate_agent_bind_type(pstAgentQueueNode->pstAgentInfo->ucBindType)
                    , pstAgentQueueNode->pstAgentInfo->szTelePhone
                    , pstAgentQueueNode->pstAgentInfo->szMobile);

        cli_out_string(ulIndex, szCmdBuff);
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n---------------------------------------------------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n** Bind : 0 -- SIP User ID, 1 -- Telephone, 2 -- Mobile\r\n");
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_agent_group(U32 ulIndex, U32 ulCustomID, U32 ulGroupID)
{
    SC_ACD_GRP_HASH_NODE_ST   *pstAgentGrouop = NULL;
    HASH_NODE_S  *pstHashNode = NULL;
    U32 ulHashIndex, ulTotal = 0;
    S8  szCmdBuff[1024] = {0, };

    if (U32_BUTT != ulGroupID)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents group with the id %d:", ulGroupID);
    }
    else if (U32_BUTT != ulCustomID)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents group own by Customer %d: ", ulCustomID);
    }
    else
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents List: ");
    }

    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%6s%10s%10s%10s%12s%16s%16s"
                    , "#", "Index", "Customer", "Agent Cnt", "ACD Policy", "Name", "LastEmpNo");
    cli_out_string(ulIndex, szCmdBuff);

    HASH_Scan_Table(g_pstGroupList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstGroupList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstAgentGrouop = (SC_ACD_GRP_HASH_NODE_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentGrouop))
            {
                continue;
            }

            if (U32_BUTT != ulCustomID && pstAgentGrouop->ulCustomID != ulCustomID)
            {
                continue;
            }

            if (U32_BUTT != ulGroupID && pstAgentGrouop->ulGroupID != ulGroupID)
            {
                continue;
            }

            dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                            , "\r\n%6u%10u%10u%10u%12u%16s%16s"
                            , pstAgentGrouop->usID
                            , pstAgentGrouop->ulGroupID
                            , pstAgentGrouop->ulCustomID
                            , pstAgentGrouop->usCount
                            , pstAgentGrouop->ucACDPolicy
                            , pstAgentGrouop->szGroupName
                            , pstAgentGrouop->szLastEmpNo);
            cli_out_string(ulIndex, szCmdBuff);

            ulTotal++;
        }
    }
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal : %d\r\n", ulTotal);
    cli_out_string(ulIndex, szCmdBuff);

}

VOID sc_show_agent(U32 ulIndex, U32 ulID, U32 ulCustomID, U32 ulGroupID)
{
    U32 ulHashIndex, i, blNeedPrint, ulTotal = 0;
    S8  szCmdBuff[1024] = {0, };
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    HASH_NODE_S  *pstHashNode = NULL;

    if (U32_BUTT != ulGroupID)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents in the Group %u:", ulGroupID);
    }
    else if (U32_BUTT != ulCustomID)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents own by Customer %u: ", ulCustomID);
    }
    else if (U32_BUTT != ulID)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents %u: ", ulID);
    }
    else
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents: ");
    }

    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%5s%8s%8s%10s%8s%8s%8s%8s%7s%8s%12s%12s%12s%9s%10s%14s%12s%12s%9s%9s%5s%8s%13s%12s"
                    , "ID", "Status", "NeedCon", "Connected", "Custom", "Group1", "Group2"
                    , "Record", "Trace", "Leader", "SIP Acc", "Extension", "Emp NO.", "CallCnt", "Bind", "Telephone", "Mobile", "TT_number", "Sip ID", "ScbNO", "bDel", "bLogin", "LastCustomer", "ProcessTime");
    cli_out_string(ulIndex, szCmdBuff);

    pthread_mutex_lock(&g_mutexAgentList);
    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstAgentQueueNode = (SC_ACD_AGENT_QUEUE_NODE_ST   *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
            {
                continue;
            }

            blNeedPrint = DOS_FALSE;

            if (U32_BUTT != ulGroupID)
            {
                blNeedPrint = DOS_FALSE;
                for (i=0; i<MAX_GROUP_PER_SITE; i++)
                {
                    if (pstAgentQueueNode->pstAgentInfo->aulGroupID[i] == ulGroupID)
                    {
                        blNeedPrint = DOS_TRUE;
                        break;
                    }
                }
            }
            else if (U32_BUTT != ulCustomID)
            {
                if (ulCustomID == pstAgentQueueNode->pstAgentInfo->ulCustomerID)
                {
                    blNeedPrint = DOS_TRUE;
                }
            }
            else if (U32_BUTT != ulID)
            {
                if (ulID == pstAgentQueueNode->pstAgentInfo->ulSiteID)
                {
                    blNeedPrint = DOS_TRUE;
                }
            }
            else
            {
                blNeedPrint = DOS_TRUE;
            }

            if (!blNeedPrint)
            {
                continue;
            }

            dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%5u%8s%8s%10s%8u%8u%8u%8s%7s%8s%12s%12s%12s%9u%10s%14s%12s%12s%9u%9u%5s%8s%13s%12u"
                        , pstAgentQueueNode->pstAgentInfo->ulSiteID
                        , sc_translate_agent_status(pstAgentQueueNode->pstAgentInfo->ucStatus)
                        , pstAgentQueueNode->pstAgentInfo->bNeedConnected ? "Y" : "N"
                        , pstAgentQueueNode->pstAgentInfo->bConnected ? "Y" : "N"
                        , pstAgentQueueNode->pstAgentInfo->ulCustomerID
                        , pstAgentQueueNode->pstAgentInfo->aulGroupID[0]
                        , pstAgentQueueNode->pstAgentInfo->aulGroupID[1]
                        , pstAgentQueueNode->pstAgentInfo->bRecord ? "Y" : "N"
                        , pstAgentQueueNode->pstAgentInfo->bTraceON ? "Y" : "N"
                        , pstAgentQueueNode->pstAgentInfo->bGroupHeader ? "Y" : "N"
                        , pstAgentQueueNode->pstAgentInfo->szUserID
                        , pstAgentQueueNode->pstAgentInfo->szExtension
                        , pstAgentQueueNode->pstAgentInfo->szEmpNo
                        , pstAgentQueueNode->pstAgentInfo->ulCallCnt
                        , sc_translate_agent_bind_type(pstAgentQueueNode->pstAgentInfo->ucBindType)
                        , pstAgentQueueNode->pstAgentInfo->szTelePhone
                        , pstAgentQueueNode->pstAgentInfo->szMobile
                        , pstAgentQueueNode->pstAgentInfo->szTTNumber
                        , pstAgentQueueNode->pstAgentInfo->ulSIPUserID
                        , pstAgentQueueNode->pstAgentInfo->usSCBNo
                        , pstAgentQueueNode->pstAgentInfo->bWaitingDelete ? "Y" : "N"
                        , pstAgentQueueNode->pstAgentInfo->bLogin ? "Y" : "N"
                        , pstAgentQueueNode->pstAgentInfo->szLastCustomerNum
                        , pstAgentQueueNode->pstAgentInfo->ucProcesingTime);
            cli_out_string(ulIndex, szCmdBuff);
            ulTotal++;
        }
    }
    pthread_mutex_unlock(&g_mutexAgentList);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n** Bind : 0 -- SIP User ID, 1 -- Telephone, 2 -- Mobile");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal : %d\r\n", ulTotal);
    cli_out_string(ulIndex, szCmdBuff);

    return;
}

VOID sc_show_callee_for_task(U32 ulIndex, U32 ulTaskID)
{
    S8 szCmdBuff[1024] = {0, };
    U32 ulTotal = 0;
    SC_TASK_CB_ST *pstTCB = NULL;
    list_t *pNode = NULL;
    SC_TEL_NUM_QUERY_NODE_ST *pstCallee = NULL;

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "Cannot find the TCB.(TaskID:%u)", ulTaskID);
        cli_out_string(ulIndex, szCmdBuff);
        return ;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%12s%7s%9s%12s"
                    , "Number", "Index", "TraceOn", "CalleeType");
    cli_out_string(ulIndex, szCmdBuff);
    cli_out_string(ulIndex, "\r\n----------------------------------------");

    dos_list_scan(&pstTCB->stCalleeNumQuery, pNode)
    {
        pstCallee = dos_list_entry(pNode, SC_TEL_NUM_QUERY_NODE_ST, stLink);
        if (NULL == pstCallee)
        {
            break;
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%12s%7u%9s%12u"
                        , pstCallee->szNumber
                        , pstCallee->ulIndex
                        , DOS_FALSE == pstCallee->ucTraceON?"No":"Yes"
                        , pstCallee->ucCalleeType);
        cli_out_string(ulIndex, szCmdBuff);
        ulTotal++;
    }
    cli_out_string(ulIndex, "\r\n----------------------------------------");
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal %u Callee(s).", ulTotal);
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_caller_for_task(U32 ulIndex, U32 ulTaskID)
{
    SC_TASK_CB_ST  *pstTCB = NULL;
    S8  szCmdBuff[1024] = {0};
    //S32 lIndex = U32_BUTT;
    S32 lCount = 0;

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "Cannot find the TCB.(TaskID:%u)", ulTaskID);
        cli_out_string(ulIndex, szCmdBuff);
        return ;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%16s%7s%6s%8s%8s%12s%6s"
                    , "Number", "TCBNo", "Valid", "TraceOn", "Index", "CustomerID", "Times");
    cli_out_string(ulIndex, szCmdBuff);
    cli_out_string(ulIndex, "\r\n---------------------------------------------------------------");
#if 0
    for (lIndex = 0; lIndex < SC_MAX_CALLER_NUM; lIndex++)
    {
        if (DOS_FALSE != pstTCB->pstCallerNumQuery[lIndex].bValid && '\0' != pstTCB->pstCallerNumQuery[lIndex].szNumber[0])
        {
            dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%16s%7u%6s%8s%8u%12u%6u"
                            , pstTCB->pstCallerNumQuery[lIndex].szNumber
                            , pstTCB->usTCBNo
                            , DOS_FALSE == pstTCB->pstCallerNumQuery[lIndex].bValid?"No":"Yes"
                            , DOS_FALSE == pstTCB->pstCallerNumQuery[lIndex].bTraceON?"No":"Yes"
                            , pstTCB->pstCallerNumQuery[lIndex].ulIndexInDB
                            , pstTCB->pstCallerNumQuery[lIndex].ulCustomerID
                            , pstTCB->pstCallerNumQuery[lIndex].ulTimes);
            cli_out_string(ulIndex, szCmdBuff);
            lCount++;
        }
    }
#endif

    cli_out_string(ulIndex, "\r\n---------------------------------------------------------------");
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal %d Caller(s).", lCount);
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_scb_detail(U32 ulIndex, U32 ulSCBID)
{
    SC_SCB_ST *pstSCB = NULL;
    S8 szCmdBuff[1024] = {0, };

    if (ulSCBID >= SC_MAX_SCB_NUM
        || !g_pstTaskMngtInfo->pstCallSCBList[ulSCBID].bValid)
    {
        cli_out_string(ulIndex, "\r\nError:"
                        "\r\n    Invalid SCB ID\r\n");
        return;
    }

    pstSCB = &g_pstTaskMngtInfo->pstCallSCBList[ulSCBID];
    cli_out_string(ulIndex, "\r\nList the SCB Information:");

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                   , "\r\n             ID : %u"
                   , "\r\n          Trace : %s"
                   , "\r\n         TCB ID : %u"
                   , "\r\n       Agent ID : %u"
                   , "\r\n         Status : %s"
                   , "\r\n   Service Type : %s, %s, %s, %s"
                   , "\r\n      Custom ID : %u"
                   , "\r\n        Task ID : %u"
                   , "\r\n       Trunk ID : %u"
                   , "\r\n     Hold Count : %u"
                   , "\r\nHold Time Count : %u"
                   , "\r\n  Caller Number : %s"
                   , "\r\n  Callee Number : %s"
                   , "\r\n            ANI : %s"
                   , "\r\n    Site Number : %s"
                   , "\r\n           UUID : %s\r\n"
                   , pstSCB->usSCBNo
                   , pstSCB->bTraceNo ? "Yes" : "No"
                   , pstSCB->usTCBNo
                   , pstSCB->usSiteNo
                   , sc_translate_task_status(pstSCB->ucStatus)
                   , sc_translate_call_service_type(pstSCB->aucServiceType[0])
                   , sc_translate_call_service_type(pstSCB->aucServiceType[1])
                   , sc_translate_call_service_type(pstSCB->aucServiceType[2])
                   , sc_translate_call_service_type(pstSCB->aucServiceType[3])
                   , pstSCB->ulCustomID
                   , pstSCB->ulTaskID
                   , pstSCB->ulTrunkID
                   , pstSCB->usHoldCnt
                   , pstSCB->usHoldTotalTime
                   , pstSCB->szCallerNum
                   , pstSCB->szCalleeNum
                   , pstSCB->szANINum
                   , pstSCB->szSiteNum
                   , pstSCB->szUUID);
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_gateway(U32 ulIndex, U32 ulID)
{
    SC_GW_NODE_ST        *pstGWNode     = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    S8 szCmdBuff[1024] = {0, };
    U32 ulHashIndex;

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList all the gateway:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n--------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%12s%8s%36s", "Index", "Status", "Domain");
    cli_out_string(ulIndex, szCmdBuff);

    HASH_Scan_Table(g_pstHashGW, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashGW, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstGWNode = pstHashNode->pHandle;

            if (U32_BUTT != ulID && ulID != pstGWNode->ulGWID)
            {
                continue;
            }

            dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%12u%8u%36s", pstGWNode->ulGWID, pstGWNode->bStatus, pstGWNode->szGWDomain);
            cli_out_string(ulIndex, szCmdBuff);
        }
    }
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n--------------------------------------------------------");
}

VOID sc_show_gateway_grp(U32 ulIndex, U32 ulID)
{
    SC_GW_GRP_NODE_ST    *pstGWGrpNode  = NULL;
    SC_GW_NODE_ST        *pstGWNode     = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    DLL_NODE_S           *pstDLLNode    = NULL;
    S8 szCmdBuff[1024] = {0, };
    U32 ulHashIndex;
    BOOL bFound = DOS_FALSE;

    HASH_Scan_Table(g_pstHashGWGrp, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashGWGrp, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstGWGrpNode = (SC_GW_GRP_NODE_ST *)pstHashNode->pHandle;
            if (U32_BUTT != ulID && ulID != pstGWGrpNode->ulGWGrpID)
            {
                continue;
            }

            dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the gateway in the gateway group %u:", ulID);
            cli_out_string(ulIndex, szCmdBuff);
            dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%12s%12s%36s", "GWID","GrpID", "Domain");
            cli_out_string(ulIndex, szCmdBuff);
            dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------");
            cli_out_string(ulIndex, szCmdBuff);

            DLL_Scan(&pstGWGrpNode->stGWList, pstDLLNode, DLL_NODE_S *)
            {
                if (DOS_ADDR_INVALID(pstDLLNode)
                    || DOS_ADDR_INVALID(pstDLLNode->pHandle))
                {
                    continue;
                }

                pstGWNode = pstDLLNode->pHandle;

                dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%12u%12u%36s", pstGWNode->ulGWID, pstGWGrpNode->ulGWGrpID, pstGWNode->szGWDomain);
                cli_out_string(ulIndex, szCmdBuff);
            }
            bFound = DOS_TRUE;
        }
    }

    if (DOS_FALSE == bFound)
    {
        cli_out_string(ulIndex, "\r\nNot found.");
    }
}

VOID sc_show_stat(U32 ulIndex, S32 argc, S8 **argv)
{
    S8 szBuff[1024] = {0, };

    cli_out_string(ulIndex, "\r\n\r\nESL Msg Stat:(recv/proc)");
    cli_out_string(ulIndex, "\r\n--------------------------------------------------------");
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n          Channel Create : %u/%u", g_astEPMsgStat[SC_EP_STAT_RECV].ulCreate, g_astEPMsgStat[SC_EP_STAT_PROC].ulCreate);
    cli_out_string(ulIndex, szBuff);
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n            Channel Park : %u/%u", g_astEPMsgStat[SC_EP_STAT_RECV].ulPark, g_astEPMsgStat[SC_EP_STAT_PROC].ulPark);
    cli_out_string(ulIndex, szBuff);
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n          Channel Answer : %u/%u", g_astEPMsgStat[SC_EP_STAT_RECV].ulAnswer, g_astEPMsgStat[SC_EP_STAT_PROC].ulAnswer);
    cli_out_string(ulIndex, szBuff);
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n          Channel Hungup : %u/%u", g_astEPMsgStat[SC_EP_STAT_RECV].ulHungup, g_astEPMsgStat[SC_EP_STAT_PROC].ulHungup);
    cli_out_string(ulIndex, szBuff);
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n Channel Hungup Complete : %u/%u", g_astEPMsgStat[SC_EP_STAT_RECV].ulHungupCom, g_astEPMsgStat[SC_EP_STAT_PROC].ulHungupCom);
    cli_out_string(ulIndex, szBuff);
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n                    DTMF : %u/%u", g_astEPMsgStat[SC_EP_STAT_RECV].ulDTMF, g_astEPMsgStat[SC_EP_STAT_PROC].ulDTMF);
    cli_out_string(ulIndex, szBuff);
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n  Channel Background Job : %u/%u", g_astEPMsgStat[SC_EP_STAT_RECV].ulBGJob, g_astEPMsgStat[SC_EP_STAT_PROC].ulBGJob);
    cli_out_string(ulIndex, szBuff);
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n            Channel Hold : %u/%u", g_astEPMsgStat[SC_EP_STAT_RECV].ulHold, g_astEPMsgStat[SC_EP_STAT_PROC].ulHold);
    cli_out_string(ulIndex, szBuff);
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n          Channel Unhold : %u/%u", g_astEPMsgStat[SC_EP_STAT_RECV].ulUnhold, g_astEPMsgStat[SC_EP_STAT_PROC].ulUnhold);
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n--------------------------------------------------------");

    cli_out_string(ulIndex, "\r\n\r\nBS Msg Stat:");
    cli_out_string(ulIndex, "\r\n--------------------------------------------------------");
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n      Auth Request(Send) : %u(%u)", stBSMsgStat.ulAuthReq, stBSMsgStat.ulAuthReqSend);
    cli_out_string(ulIndex, szBuff);
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n   Billing Request(Send) : %u(%u)", stBSMsgStat.ulBillingReq, stBSMsgStat.ulBillingReqSend);
    cli_out_string(ulIndex, szBuff);
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n           Auth Response : %u", stBSMsgStat.ulAuthRsp);
    cli_out_string(ulIndex, szBuff);
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n        Billing Response : %u", stBSMsgStat.ulBillingRsp);
    cli_out_string(ulIndex, szBuff);
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n    Heartbeat(Send/Recv) : %u/%u", stBSMsgStat.ulHBReq, stBSMsgStat.ulHBRsp);
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n--------------------------------------------------------");


    cli_out_string(ulIndex, "\r\n\r\n");
}


U32 sc_show_status(U32 ulIndex, S32 argc, S8 **argv)
{
    S8 szBuff[1024] = {0, };
    U32 ulIdelCPU, ulIdelCPUConfig;

    if (config_get_min_iedl_cpu(&ulIdelCPUConfig) != DOS_SUCC)
    {
        ulIdelCPUConfig = DOS_MIN_IDEL_CPU;
    }

    cli_out_string(ulIndex, "\r\n\r\nThe CC Model for IPCC System Status:");

    dos_snprintf(szBuff, sizeof(szBuff), "\r\nVersion : %s Build on %s", dos_get_process_version(), dos_get_build_datetime_str());
    cli_out_string(ulIndex, szBuff);

    ulIdelCPU = dos_get_cpu_idel_percentage();
    dos_snprintf(szBuff, sizeof(szBuff), "\r\nMin Idel CPU(default/current) : %d%% / %d.%d%%"
                , ulIdelCPUConfig
                , (ulIdelCPU / 100)
                , (ulIdelCPU % 100));
    cli_out_string(ulIndex, szBuff);

    cli_out_string(ulIndex, "\r\n\r\n");

    return DOS_SUCC;
}


VOID sc_show_sip_acc(U32 ulIndex, S32 argc, S8 **argv)
{
    HASH_NODE_S *pstHashNode = NULL;
    SC_USER_ID_NODE_ST *pstUserID = NULL;
    U32 ulHashIndex = 0;
    S8 szBuff[1024] = {0, };

    cli_out_string(ulIndex, "\r\nList All the SIP User ID:");

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%12s%12s%32s%32s", "Index", "Customer", "User ID", "Extension");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n----------------------------------------------------------------------------------------");

    HASH_Scan_Table(g_pstHashSIPUserID, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashSIPUserID, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                break;
            }

            pstUserID = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstUserID))
            {
                continue;
            }

            dos_snprintf(szBuff, sizeof(szBuff)
                        , "\r\n%12d%12d%32s%32s"
                        , pstUserID->ulSIPID
                        , pstUserID->ulCustomID
                        , '\0' == pstUserID->szUserID[0] ? "NULL" : pstUserID->szUserID
                        , '\0' == pstUserID->szExtension[0] ? "NULL" : pstUserID->szExtension);
            cli_out_string(ulIndex, szBuff);
        }
    }

    cli_out_string(ulIndex, "\r\n----------------------------------------------------------------------------------------\r\n\r\n");
}

VOID sc_show_route(U32 ulIndex, U32 ulRouteID)
{
    SC_ROUTE_NODE_ST *pstRoute = NULL;
    S8  szCmdBuff[1024] = {0,};
    S8  szDataList[32] = {0};
    S8  szData[8] = {0};
    DLL_NODE_S * pstDLLNode = NULL;
    U32  ulRouteCnt = 0, i = 0;

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the route list.");
    cli_out_string(ulIndex, szCmdBuff);

    /*制作表头*/
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n+----------------------------------------------------------------------------------------------------------------------------------------+");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n|                                                             Route List                                                                 |");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n+--------------+---------------+---------------------------------+---------------+------------------+----------+----------+--------------+");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n|              |     Time      |             Prefix              |               |                  |          |          |              |");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n|      ID      +-------+-------+----------------+----------------+   Dest Type   |      Dest ID     |  Status  | Priority | CallOutGroup |");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n|              | Start |  End  |     Callee     |     Caller     |               |                  |          |          |              |");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n+--------------+-------+-------+----------------+----------------+---------------+------------------+----------+----------+--------------+");
    cli_out_string(ulIndex, szCmdBuff);

    pthread_mutex_lock(&g_mutexRouteList);

    DLL_Scan(&g_stRouteList, pstDLLNode, DLL_NODE_S *)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode))
        {
            continue;
        }
        pstRoute = (SC_ROUTE_NODE_ST *)pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstRoute->szCalleePrefix)
            || DOS_ADDR_INVALID(pstRoute->szCallerPrefix))
        {
            continue;
        }

        /*打印数据*/
        if (U32_BUTT != ulRouteID && ulRouteID != pstRoute->ulID)
        {
            continue;
        }

        for (i = 0; i < SC_ROUT_GW_GRP_MAX_SIZE; ++i)
        {
            if (0 == pstRoute->aulDestID[i] || U32_BUTT == pstRoute->aulDestID[i])
            {
                continue;
            }
            dos_ltoa(pstRoute->aulDestID[i], szData, sizeof(szData));
            dos_strcat(szDataList, szData);
            dos_strcat(szDataList, " ");
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n|%14u| %02u:%02u | %02u:%02u |%-16s|%-16s|%-15s|%-18s|%10s|%10u|%14u|"
                    , pstRoute->ulID
                    , pstRoute->ucHourBegin
                    , pstRoute->ucMinuteBegin
                    , pstRoute->ucHourEnd
                    , pstRoute->ucMinuteEnd
                    , pstRoute->szCalleePrefix[0] == '\0' ? "NULL" : pstRoute->szCalleePrefix
                    , pstRoute->szCallerPrefix[0] == '\0' ? "NULL" : pstRoute->szCallerPrefix
                    , sc_translate_route_dest_type(pstRoute->ulDestType)
                    , szDataList
                    , DOS_FALSE == pstRoute->bStatus?"No":"Yes"
                    , pstRoute->ucPriority
                    , pstRoute->usCallOutGroup);
        cli_out_string(ulIndex, szCmdBuff);
        ++ulRouteCnt;
        dos_memzero(szDataList, sizeof(szDataList));
    }

    pthread_mutex_unlock(&g_mutexRouteList);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n+--------------+-------+-------+----------------+----------------+---------------+------------------+----------+----------+--------------+");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal:%d routes.\r\n\r\n", ulRouteCnt);
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_did(U32 ulIndex, S8 *pszDidNum)
{
    SC_DID_NODE_ST *pstDid      = NULL;
    HASH_NODE_S    *pstHashNode = NULL;
    U32   ulDidCnt = 0;
    U32   ulHashIndex = 0;
    S8    szCmdBuff[1024] = {0, };

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the did list.");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n+--------------------------------------------------------------------------------------------------+----------+");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n|                                                  Did List                                                   |");
    cli_out_string(ulIndex,szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n+--------------+---------------+------------------------+---------------+--------------+-----------+----------|");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n|    Did ID    |  Customer ID  |        Did Num         |   Bind Type   |    Bind ID   |   Times   |  Status  |");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n+--------------+---------------+------------------------+---------------+--------------+-----------+----------|");
    cli_out_string(ulIndex, szCmdBuff);

    pthread_mutex_lock(&g_mutexHashDIDNum);

    HASH_Scan_Table(g_pstHashDIDNum,ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashDIDNum, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                    || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstDid = (SC_DID_NODE_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstDid->szDIDNum))
            {
                continue;
            }

            if (NULL != pszDidNum && 0 != dos_strcmp(pszDidNum, pstDid->szDIDNum))
            {
                continue;
            }

            dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                            , "\r\n|%14u|%15u|%-24s|%-15s|%14u|%11u|%10s|"
                            , pstDid->ulDIDID
                            , pstDid->ulCustomID
                            , pstDid->szDIDNum[0] == '\0' ? "NULL": pstDid->szDIDNum
                            , sc_translate_did_bind_type(pstDid->ulBindType)
                            , pstDid->ulBindID
                            , pstDid->ulTimes
                            , DOS_FALSE == pstDid->bValid ? "No":"Yes"
                            );
            cli_out_string(ulIndex, szCmdBuff);
            ++ulDidCnt;
        }
    }

    pthread_mutex_unlock(&g_mutexHashDIDNum);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n+--------------+---------------+------------------------+---------------+--------------+-----------+----------+");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal: %d did numbers.\r\n\r\n", ulDidCnt);
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_black_list(U32 ulIndex, U32 ulBlackListID)
{
    SC_BLACK_LIST_NODE *pstBlackList = NULL;
    HASH_NODE_S * pstHashNode = NULL;
    U32 ulHashIndex = 0;
    U32 ulBlackListCnt = 0;
    S8    szCmdBuff[1024] = {0, };

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "List the black list.");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n+-----------------------------------------------------------+");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n|                        Black List                         |");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n+--------------+---------------+----------------------------+");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n|      ID      |  Customer ID  |            Num             |");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n+--------------+---------------+----------------------------+");
    cli_out_string(ulIndex, szCmdBuff);

    pthread_mutex_lock(&g_mutexHashBlackList);

    HASH_Scan_Table(g_pstHashBlackList,ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashBlackList,ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstBlackList = (SC_BLACK_LIST_NODE *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstBlackList->szNum))
            {
                continue;
            }

            if (U32_BUTT != ulBlackListID && ulBlackListID != pstBlackList->ulID)
            {
                continue;
            }
            dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                            , "\r\n|%14u|%15u|%28s|"
                            , pstBlackList->ulID
                            , pstBlackList->ulCustomerID
                            , pstBlackList->szNum[0] == '\0'? "NULL" : pstBlackList->szNum);
            cli_out_string(ulIndex, szCmdBuff);
            ++ulBlackListCnt;
        }
    }

    pthread_mutex_unlock(&g_mutexHashBlackList);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n+--------------+---------------+----------------------------+");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal: %d Black List(s).\r\n\r\n", ulBlackListCnt);
    cli_out_string(ulIndex, szCmdBuff);
}

U32 sc_show_caller(U32 ulIndex, U32 ulCallerID)
{
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    S8  szBuff[128] = {0};
    U32 ulHashIndex = U32_BUTT;

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5s%8s%8s%8s%12s%8s%25s"
                    , "No.", "Valid", "TraceOn", "Index", "CustomerID", "Times", "Number");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n--------------------------------------------------------------------------");

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

            if (U32_BUTT != ulCallerID && ulCallerID != pstCaller->ulIndexInDB)
            {
                continue;
            }
            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5u%8s%8s%8u%12u%8u%25s"
                            , pstCaller->usNo
                            , pstCaller->bValid == DOS_FALSE ? "No":"Yes"
                            , pstCaller->bTraceON == DOS_FALSE ? "No":"Yes"
                            , pstCaller->ulIndexInDB
                            , pstCaller->ulCustomerID
                            , pstCaller->ulTimes
                            , pstCaller->szNumber);
             cli_out_string(ulIndex, szBuff);
        }
    }

    return DOS_SUCC;
}

U32 sc_show_caller_grp(U32 ulIndex, U32 ulGrpID)
{
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    U32 ulHashIndex = U32_BUTT;
    S8  szBuff[128] = {0};

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%6s%12s%8s%10s%10s%-64s"
                    , "Index", "CustomerID", "LastNo", "Policy", "Default", " GroupName");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n----------------------------------------------------------------------------------------------------");

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
            if (U32_BUTT != ulGrpID && ulGrpID != pstCallerGrp->ulID)
            {
                continue;
            }

            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%6u%12u%8u%10s%10s %-63s"
                            , pstCallerGrp->ulID
                            , pstCallerGrp->ulCustomerID
                            , pstCallerGrp->ulLastNo
                            , sc_translate_caller_policy(pstCallerGrp->ulPolicy)
                            , pstCallerGrp->bDefault == DOS_FALSE?"No":"Yes"
                            , pstCallerGrp->szGrpName);
            cli_out_string(ulIndex, szBuff);
        }
    }

    return DOS_SUCC;
}

U32 sc_show_caller_by_callergrp(U32 ulIndex, U32 ulGrpID)
{
    U32 ulHashIndex = U32_BUTT;
    HASH_NODE_S *pstHashNode = NULL;
    S8 szBuff[256] = {0};
    DLL_NODE_S *pstListNode = NULL;
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    SC_CALLER_CACHE_NODE_ST *pstCallerCache = NULL;

    ulHashIndex = sc_ep_caller_grp_hash_func(ulGrpID);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulGrpID, sc_ep_caller_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        dos_snprintf(szBuff, sizeof(szBuff), "\r\nCannot find Caller Group %u.", ulGrpID);
        cli_out_string(ulIndex, szBuff);
        return DOS_FAIL;
    }
    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%7s%12s%24s", "Index", "CallerType", "Number");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n-------------------------------------------");

    pthread_mutex_lock(&pstCallerGrp->mutexCallerList);
    DLL_Scan(&pstCallerGrp->stCallerList, pstListNode, DLL_NODE_S *)
    {
        if (DOS_ADDR_INVALID(pstListNode)
            || DOS_ADDR_INVALID(pstListNode->pHandle))
        {
            continue;
        }
        pstCallerCache = (SC_CALLER_CACHE_NODE_ST *)pstListNode->pHandle;
        if (SC_NUMBER_TYPE_CFG == pstCallerCache->ulType)
        {
            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%7u%12s%24s"
                            , pstCallerCache->stData.pstCaller->ulIndexInDB
                            , "CALLER"
                            , pstCallerCache->stData.pstCaller->szNumber);
        }
        else
        {
            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%7u%12s%24s"
                            , pstCallerCache->stData.pstDid->ulDIDID
                            , "DID"
                            , pstCallerCache->stData.pstDid->szDIDNum);
        }
        cli_out_string(ulIndex, szBuff);
    }
    pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);
    return DOS_SUCC;
}

U32  sc_show_caller_setting(U32 ulIndex, U32 ulSettingID)
{
    SC_CALLER_SETTING_ST *pstSetting = NULL;
    S8  szBuff[128] = {0};
    U32 ulHashIndex = U32_BUTT;
    HASH_NODE_S *pstHashNode  = NULL;

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%6s%12s%14s%9s%7s%11s%-64s"
                    , "Index", "CustomerID", "SrcID", "SrcType", "DstID", "DstType", " SettingName");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------------------------------------------------------------------------");

    HASH_Scan_Table(g_pstHashCallerSetting, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashCallerSetting, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            if (U32_BUTT != ulSettingID && ulSettingID != pstSetting->ulID)
            {
                continue;
            }
            pstSetting = (SC_CALLER_SETTING_ST *)pstHashNode->pHandle;
            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%6u%12u%14u%9s%7u%11s %-64s"
                            , pstSetting->ulID
                            , pstSetting->ulCustomerID
                            , pstSetting->ulSrcID
                            , sc_translate_caller_srctype(pstSetting->ulSrcType)
                            , pstSetting->ulDstID
                            , sc_translate_caller_dsttype(pstSetting->ulDstType)
                            , pstSetting->szSettingName);
            cli_out_string(ulIndex, szBuff);
        }
    }
    return DOS_SUCC;
}

U32 sc_show_tt(U32 ulIndex, U32 ulTTID)
{
    U32  ulHashIndex = U32_BUTT;
    HASH_NODE_S *pstHashNode = NULL;
    SC_TT_NODE_ST *pstTTNumber = NULL;
    S8  szBuff[256] = {0};

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5s%6s%24s %-31s"
                            , "ID", "Port", "Prefix", "Addr");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n-------------------------------------------------------------------");

    HASH_Scan_Table(g_pstHashTTNumber,ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashTTNumber, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstTTNumber = (SC_TT_NODE_ST *)pstHashNode->pHandle;

            if (U32_BUTT != ulTTID && ulTTID != pstTTNumber->ulID)
            {
                continue;
            }

            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5u%6u%24s %-31s"
                            , pstTTNumber->ulID
                            , pstTTNumber->ulPort
                            , pstTTNumber->szPrefix
                            , pstTTNumber->szAddr);
            cli_out_string(ulIndex, szBuff);
        }
    }

    return DOS_SUCC;
}

U32 sc_show_customer(U32 ulIndex, U32 ulCustomerID)
{
    SC_CUSTOMER_NODE_ST *pstCustomer = NULL;
    DLL_NODE_S *pstNode = NULL;
    S8  szBuff[256] = {0};

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5s%8s%15s"
                    , "ID", "bExist", "CallOutGroup");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n----------------------------");

    DLL_Scan(&g_stCustomerList, pstNode, DLL_NODE_S *)
    {
        if (DOS_ADDR_INVALID(pstNode)
            || DOS_ADDR_INVALID(pstNode->pHandle))
        {
            continue;
        }
        pstCustomer = (SC_CUSTOMER_NODE_ST *)pstNode->pHandle;
        if (U32_BUTT != ulCustomerID && ulCustomerID != pstCustomer->ulID)
        {
            continue;
        }
        dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5u%8s%15u"
                        , pstCustomer->ulID
                        , DOS_FALSE == pstCustomer->bExist?"No":"Yes"
                        , pstCustomer->usCallOutGroup);
        cli_out_string(ulIndex, szBuff);
    }
    return DOS_SUCC;
}

U32 sc_show_transform(U32 ulIndex, U32 ulID)
{
    SC_NUM_TRANSFORM_NODE_ST *pstTransform = NULL;
    DLL_NODE_S *pstNode = NULL;
    S8  szBuff[1024] = {0};

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%3s%10s%6s%10s%14s%10s%13s%13s%11s%11s%8s%9s%10s%10s%9s%11s"
                    , "ID", "Obj", "ObjID", "Direction", "TransformTime", "NumSelect", "CalleePrefix", "CallerPrefix", "ReplaceAll", "ReplaceNum", "DelLeft", "DelRight", "AddPrefix", "AddSuffix", "Priority", "Expiry");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n--------------------------------------------------------------------------------------------------------------------------------------------------------------");

    DLL_Scan(&g_stNumTransformList, pstNode, DLL_NODE_S *)
    {
        if (DOS_ADDR_INVALID(pstNode)
            || DOS_ADDR_INVALID(pstNode->pHandle))
        {
            continue;
        }
        pstTransform = (SC_NUM_TRANSFORM_NODE_ST *)pstNode->pHandle;
        if (U32_BUTT != ulID && ulID != pstTransform->ulID)
        {
            continue;
        }
        dos_snprintf(szBuff, sizeof(szBuff), "\r\n%3u%10s%6u%10s%14s%10s%13s%13s%11s%11s%8u%9u%10s%10s%9u%11u"
                        , pstTransform->ulID
                        , sc_translate_transform_object(pstTransform->enObject)
                        , pstTransform->ulObjectID
                        , sc_translate_transform_direction(pstTransform->enDirection)
                        , sc_translate_transform_time(pstTransform->enTiming)
                        , sc_translate_transfrom_numselect(pstTransform->enNumSelect)
                        , pstTransform->szCalleePrefix
                        , pstTransform->szCallerPrefix
                        , pstTransform->bReplaceAll == DOS_FALSE?"No":"Yes"
                        , pstTransform->szReplaceNum
                        , pstTransform->ulDelLeft
                        , pstTransform->ulDelRight
                        , pstTransform->szAddPrefix
                        , pstTransform->szAddSuffix
                        , pstTransform->enPriority
                        , pstTransform->ulExpiry);
        cli_out_string(ulIndex, szBuff);
    }
    return DOS_SUCC;
}

U32 sc_show_numlmt(U32 ulIndex, U32 ulID)
{
    SC_NUMBER_LMT_NODE_ST *pstNumlmt = NULL;
    U32 ulHashIndex = U32_BUTT;
    HASH_NODE_S *pstHashNode = NULL;
    S8  szBuff[256] = {0};

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5s%6s%7s%10s%6s%10s%15s%10s%24s"
                    , "ID", "GrpID", "Handle", "Limit", "Cycle", "Type", "NumberID", "StatUsed", "Prefix");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n---------------------------------------------------------------------------------------------");

    HASH_Scan_Table(g_pstHashNumberlmt, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashNumberlmt, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstNumlmt = (SC_NUMBER_LMT_NODE_ST *)pstHashNode->pHandle;
            if (ulID != U32_BUTT && ulID != pstNumlmt->ulID)
            {
                continue;
            }

            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5u%6u%7u%10u%6u%10s%15u%10u%24s"
                            , pstNumlmt->ulID
                            , pstNumlmt->ulGrpID
                            , pstNumlmt->ulHandle
                            , pstNumlmt->ulLimit
                            , pstNumlmt->ulCycle
                            , sc_translate_caller_type(pstNumlmt->ulType)
                            , pstNumlmt->ulNumberID
                            , pstNumlmt->ulStatUsed
                            , pstNumlmt->szPrefix);
             cli_out_string(ulIndex, szBuff);
        }
    }
    return DOS_SUCC;
}

U32  sc_show_cwq(U32 ulIndex, U32 ulAgentGrpID)
{
    DLL_NODE_S *pstListNode = NULL, *pstListNode1 = NULL;
    SC_CWQ_NODE_ST *pstCWQNode = NULL;
    SC_SCB_ST      *pstSCB = NULL;
    struct tm *pstSWTime = NULL;
    S8  szBuff[256] = {0}, szTime[32] = {0};

    DLL_Scan(&g_stCWQMngt, pstListNode, DLL_NODE_S *)
    {
        if (DOS_ADDR_INVALID(pstListNode)
            || DOS_ADDR_INVALID(pstListNode->pHandle))
        {
            continue;
        }
        pstCWQNode = (SC_CWQ_NODE_ST *)pstListNode->pHandle;
        if (ulAgentGrpID != U32_BUTT && ulAgentGrpID != pstCWQNode->ulAgentGrpID)
        {
            continue;
        }
        pstSWTime = localtime((time_t *)&pstCWQNode->ulStartWaitingTime);
        dos_snprintf(szTime, sizeof(szTime), "%04u/%02u/%02u %02u:%02u:%02u"
                        , pstSWTime->tm_year + 1900
                        , pstSWTime->tm_mon + 1
                        , pstSWTime->tm_mday
                        , pstSWTime->tm_hour
                        , pstSWTime->tm_min
                        , pstSWTime->tm_sec);

        dos_snprintf(szBuff, sizeof(szBuff), "\r\nList the cwq of agentgrp %u.", ulAgentGrpID);
        cli_out_string(ulIndex, szBuff);
        dos_snprintf(szBuff, sizeof(szBuff), "\r\n%12s%20s%6s"
                        , "AgentGrpID", "StartWaitingTime", "SCBNo");
        cli_out_string(ulIndex, szBuff);
        cli_out_string(ulIndex, "\r\n--------------------------------------");
        DLL_Scan(&pstCWQNode->stCallWaitingQueue, pstListNode1, DLL_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstListNode1)
                || DOS_ADDR_INVALID(pstListNode1->pHandle))
            {
                continue;
            }
            pstSCB = (SC_SCB_ST *)pstListNode1->pHandle;
            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%12u%20s%6u"
                            , ulAgentGrpID
                            , szTime
                            , pstSCB->usSCBNo);
            cli_out_string(ulIndex, szBuff);
        }
        cli_out_string(ulIndex, "\r\n--------------------------------------");
    }

    return DOS_SUCC;
}

S32 sc_show_taskmgnt(U32 ulIndex)
{
    S8  szBuff[1024] = {0};

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%10s%17s", "TaskCount", "MaxCallRate4Task");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n---------------------------");
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%10u%17u\r\n", g_pstTaskMngtInfo->ulTaskCount, g_pstTaskMngtInfo->ulMaxCallRate4Task);
    cli_out_string(ulIndex, szBuff);

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%9s%12s%16s%13s%14s%11s%17s%17s%13s%14s%13s", "MaxCalls", "MaxSessions", "CurrentSessions", "CurrentCalls", "TotalSessions"
                    , "TotalCalls", "OutgoingSessions", "IncomingSessions", "FailSessions", "SysUptime", "SysIsWorking");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------------------------------------------------------------------------------------------------------------");
    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%9u%12u%16u%13u%14u%11u%17u%17u%13u%14u%13u"
                    , g_pstTaskMngtInfo->stStat.ulMaxCalls
                    , g_pstTaskMngtInfo->stStat.ulMaxSession
                    , g_pstTaskMngtInfo->stStat.ulCurrentSessions
                    , g_pstTaskMngtInfo->stStat.ulCurrentCalls
                    , g_pstTaskMngtInfo->stStat.ulTotalSessions
                    , g_pstTaskMngtInfo->stStat.ulTotalCalls
                    , g_pstTaskMngtInfo->stStat.ulOutgoingSessions
                    , g_pstTaskMngtInfo->stStat.ulIncomingSessions
                    , g_pstTaskMngtInfo->stStat.ulFailSessions
                    , g_pstTaskMngtInfo->stStat.ulSystemUpTime
                    , g_pstTaskMngtInfo->stStat.ulSystemIsWorking);
    cli_out_string(ulIndex, szBuff);

    return DOS_SUCC;
}

S32 sc_debug_call(U32 ulTraceFlag, S8 *pszCaller, S8 *pszCallee)
{
    return 0;
}

#if 0
S32 sc_track_call_by_caller(U32 ulIndex ,S8 *pszCaller)
{
    S8 szCmdBuff[1024] = {0, };
    U32 ulTaskIndex = U32_BUTT;
    SC_TASK_CB_ST *pstTCB = NULL;

    for (ulTaskIndex = 0; ulTaskIndex < SC_MAX_TASK_NUM; ulTaskIndex++)
    {
        pstTCB = &g_pstTaskMngtInfo->pstTaskList[ulTaskIndex];
        if (DOS_ADDR_INVALID(pstTCB))
        {
            continue;
        }

        if (!pstTCB->ucValid)
        {
            continue;
        }

        if (0 != dos_strcmp(pstTCB->pstCallerNumQuery->szNumber, pszCaller))
        {
            continue;
        }

        cli_out_string(ulIndex, "\r\nList Trace Info By Caller");
        cli_out_string(ulIndex, "\r\n---------------------------");
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%8s%12s%8s","Task ID", "Customer ID", "TCB NO");
        cli_out_string(ulIndex, szCmdBuff);
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%8u%12u%8u\r\n"
                        , pstTCB->ulTaskID
                        , pstTCB->ulCustomID
                        , pstTCB->usTCBNo);
        cli_out_string(ulIndex, szCmdBuff);
    }

    return DOS_SUCC;
}
#endif

S32 sc_track_call_by_callee(U32 ulIndex ,S8 *pszCallee)
{
    /*
    S8 szCmdBuff[1024] = {0, };
    U32 ulTaskIndex = U32_BUTT;
    SC_TASK_CB_ST *pstTCB = NULL;

    for (ulTaskIndex = 0; ulTaskIndex < SC_MAX_TASK_NUM; ulTaskIndex++)
    {
        pstTCB = &g_pstTaskMngtInfo->pstTaskList[ulTaskIndex];
        if (DOS_ADDR_INVALID(pstTCB))
        {
            continue;
        }

        if (!pstTCB->ucValid)
        {
            continue;
        }

        pstTCB->stCalleeNumQuery
    }*/
    return DOS_SUCC;
}

S32 sc_track_call_by_task(U32 ulIndex ,U32 ulTaskID)
{
    S8 szCmdBuff[1024] = {0, };
    U32 ulTaskIndex = U32_BUTT;
    SC_TASK_CB_ST *pstTCB = NULL;

    for (ulTaskIndex = 0; ulTaskIndex < SC_MAX_TASK_NUM; ulTaskIndex++)
    {
        pstTCB = &g_pstTaskMngtInfo->pstTaskList[ulTaskIndex];
        if (DOS_ADDR_INVALID(pstTCB))
        {
            continue;
        }

        if (!pstTCB->ucValid)
        {
            continue;
        }

        if (ulTaskID != pstTCB->ulTaskID)
        {
            continue;
        }

        cli_out_string(ulIndex, "\r\nList Trace Info By Caller");
        cli_out_string(ulIndex, "\r\n---------------------------");
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%8s%12s%8s","Task ID", "Customer ID", "TCB NO");
        cli_out_string(ulIndex, szCmdBuff);
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%8u%12u%8u\r\n"
                        , pstTCB->ulTaskID
                        , pstTCB->ulCustomID
                        , pstTCB->usTCBNo);
        cli_out_string(ulIndex, szCmdBuff);
    }

    return DOS_SUCC;
}


S32 cli_cc_trace(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulSubMod = SC_SUB_MOD_BUTT;
    U32 ulSubMod1 = SC_SUB_MOD_BUTT;
    U32 ulTraceAll = DOS_FALSE;
    U32 ulID = 0;

    if (argc < 4)
    {
        return -1;
    }

    if (dos_strnicmp(argv[2], "scb", dos_strlen("scb")) == 0)
    {
        if (5 == argc)
        {
            if (dos_strnicmp(argv[3], "all", dos_strlen("all")) == 0
                || dos_atoul(argv[3], &ulID) == 0)
            {
                if (dos_strnicmp(argv[4], "off", dos_strlen("off")) == 0)
                {}
                else if (dos_strnicmp(argv[4], "on", dos_strlen("on")) == 0)
                {}
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }

        return 0;
    }
    else if (dos_strnicmp(argv[2], "task", dos_strlen("debug")) == 0)
    {
        if (5 == argc)
        {
            if (dos_strnicmp(argv[3], "all", dos_strlen("all")) == 0)
            {
                if (dos_strnicmp(argv[4], "off", dos_strlen("off")) == 0)
                {
                    g_ulTaskTraceAll = 0;
                }
                else if (dos_strnicmp(argv[4], "on", dos_strlen("on")) == 0)
                {
                    g_ulTaskTraceAll = 1;
                }
                else
                {
                    return -1;
                }
            }
            else if (dos_atoul(argv[3], &ulID) == 0)
            {
                SC_TASK_CB_ST *pstTCB = NULL;

                pstTCB = sc_tcb_find_by_taskid(ulID);
                if (DOS_ADDR_INVALID(pstTCB))
                {
                    return -1;
                }

                if (dos_strnicmp(argv[4], "off", dos_strlen("off")) == 0)
                {
                    pstTCB->bTraceON = DOS_FALSE;
                }
                else if (dos_strnicmp(argv[4], "on", dos_strlen("on")) == 0)
                {
                    pstTCB->bTraceON = DOS_TRUE;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }

        return 0;
    }
    else if (dos_strnicmp(argv[2], "call", dos_strlen("call")) == 0)
    {
        if (6 == argc)
        {
            if (dos_strnicmp(argv[3], "callee", dos_strlen("callee")) == 0)
            {
                if (dos_strnicmp(argv[5], "off", dos_strlen("off")) == 0)
                {
                    sc_debug_call(DOS_FALSE, NULL, argv[4]);
                }
                else if (dos_strnicmp(argv[5], "on", dos_strlen("on")) == 0)
                {
                    sc_debug_call(DOS_TRUE, NULL, argv[4]);
                }
                else
                {
                    return -1;
                }
            }
            else if (dos_strnicmp(argv[3], "caller", dos_strlen("caller")) == 0)
            {
                if (dos_strnicmp(argv[5], "off", dos_strlen("off")) == 0)
                {
                    sc_debug_call(DOS_FALSE, argv[4], NULL);
                }
                else if (dos_strnicmp(argv[5], "on", dos_strlen("on")) == 0)
                {
                    sc_debug_call(DOS_TRUE, argv[4], NULL);
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
        }
        else if (8 == argc)
        {
            S8 *pszCaller = NULL, *pszCallee = NULL;

            if (dos_strnicmp(argv[3], "caller", dos_strlen("caller")) == 0)
            {
                pszCaller = argv[4];
            }
            else if (dos_strnicmp(argv[3], "callee", dos_strlen("callee")) == 0)
            {
                pszCallee = argv[4];
            }
            else
            {
                return -1;
            }

            if (dos_strnicmp(argv[5], "caller", dos_strlen("caller")) == 0)
            {
                pszCaller = argv[6];
            }
            else if (dos_strnicmp(argv[5], "callee", dos_strlen("callee")) == 0)
            {
                pszCallee = argv[6];
            }
            else
            {
                return -1;
            }

            if (dos_strnicmp(argv[5], "off", dos_strlen("off")) == 0)
            {
                sc_debug_call(DOS_FALSE, pszCaller, pszCallee);
            }
            else if (dos_strnicmp(argv[5], "off", dos_strlen("off")) == 0)
            {
                sc_debug_call(DOS_TRUE, pszCaller, pszCallee);
            }
        }
        else
        {
            return -1;
        }

        return 0;
    }
    else
    {
        if (dos_strnicmp(argv[2], "func", dos_strlen("func")) == 0)
        {
            ulSubMod = SC_FUNC;
        }
        else if (dos_strnicmp(argv[2], "http", dos_strlen("http")) == 0)
        {
            ulSubMod = SC_HTTPD;
        }
        else if (dos_strnicmp(argv[2], "api", dos_strlen("api")) == 0)
        {
            ulSubMod = SC_HTTP_API;
        }
        else if (dos_strnicmp(argv[2], "acd", dos_strlen("acd")) == 0)
        {
            ulSubMod = SC_ACD;
        }
        else if (dos_strnicmp(argv[2], "task", dos_strlen("task")) == 0)
        {
            ulSubMod = SC_TASK;
            ulSubMod1 = SC_TASK_MNGT;
        }
        else if (dos_strnicmp(argv[2], "dialer", dos_strlen("dialer")) == 0)
        {
            ulSubMod = SC_DIALER;
        }
        else if (dos_strnicmp(argv[2], "esl", dos_strlen("esl")) == 0)
        {
            ulSubMod = SC_ESL;
        }
        else if (dos_strnicmp(argv[2], "bss", dos_strlen("bss")) == 0)
        {
            ulSubMod = SC_BS;
        }
        else if (dos_strnicmp(argv[2], "all", dos_strlen("bss")) == 0)
        {
            ulTraceAll = DOS_TRUE;
        }

        if (!ulTraceAll && SC_SUB_MOD_BUTT == ulSubMod)
        {
            return -1;
        }

        if (dos_strnicmp(argv[3], "off", dos_strlen("off")) == 0)
        {
            if (ulTraceAll)
            {
                g_ulTraceFlags = 0;
            }
            else
            {
                if (SC_SUB_MOD_BUTT != ulSubMod)
                {
                    SC_NODEBUG_SUBMOD(g_ulTraceFlags, ulSubMod);
                }

                if (SC_SUB_MOD_BUTT != ulSubMod1)
                {
                    SC_NODEBUG_SUBMOD(g_ulTraceFlags, ulSubMod1);
                }
            }
        }
        else if (dos_strnicmp(argv[3], "on", dos_strlen("on")) == 0)
        {
            if (ulTraceAll)
            {
                g_ulTraceFlags = 0xFFFFFFFF;
            }
            else
            {
                if (SC_SUB_MOD_BUTT != ulSubMod)
                {
                    SC_DEBUG_SUBMOD(g_ulTraceFlags, ulSubMod);
                }

                if (SC_SUB_MOD_BUTT != ulSubMod1)
                {
                    SC_DEBUG_SUBMOD(g_ulTraceFlags, ulSubMod1);
                }
            }
        }
        else
        {
            return -1;
        }
    }

    return 0;
}

VOID sc_show_cb(U32 ulIndex)
{
    S8    szCmdBuff[1024] = {0, };
    S32   i;

    cli_out_string(ulIndex, "\r\n\r\nList the CB Usage:");
    cli_out_string(ulIndex, "\r\n--------------------------------------------------");
    for (i=0; i<SC_EP_TASK_NUM; i++)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n      ESL Process Task : %u (%s)"
                    , g_astEPTaskList[i].stMsgList.ulCount
                    , (SC_MASTER_TASK_INDEX==i) ? "Master" : "Slave ");
        cli_out_string(ulIndex, szCmdBuff);
    }
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n\r\n          Dialer Queue : %u", g_pstDialerHandle->ulCallCnt);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n      BS Message Queue : %u", g_stBSMsgList.ulCount);
    cli_out_string(ulIndex, szCmdBuff);

    cli_out_string(ulIndex, "\r\n--------------------------------------------------\r\n\r\n");
}


S32 cli_cc_show(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulID;

    if (argc < 3)
    {
        return -1;
    }
    if (dos_stricmp(argv[2], "status") == 0)
    {
        sc_show_status(ulIndex, argc, argv);
    }
    else if (dos_stricmp(argv[2], "sip") == 0)
    {
        sc_show_sip_acc(ulIndex, argc, argv);
    }
    else if (dos_stricmp(argv[2], "cb") == 0)
    {
        sc_show_cb(ulIndex);
    }
    else if (dos_stricmp(argv[2], "stat") == 0)
    {
        sc_show_stat(ulIndex, argc, argv);
    }
    else if (dos_stricmp(argv[2], "httpd") == 0)
    {
        if (3 == argc)
        {
            sc_show_httpd(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_httpd(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid HTTPD ID while show the HTTPDS.\r\n");
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_stricmp(argv[2], "http") == 0)
    {
        if (3 == argc)
        {
            sc_show_http(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_http(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid HTTP Clinet ID while show the HTTPS.\r\n");
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_stricmp(argv[2], "gateway") == 0)
    {
        if (3 == argc)
        {
            sc_show_gateway(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_gateway(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERRNO: Invalid gateway ID while show the gateway(s).\r\n");
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_stricmp(argv[2], "gwgrp") == 0)
    {
        if (3 == argc)
        {
            sc_show_gateway_grp(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_gateway_grp(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR:Invalid gateway group ID while show the gateway group(s).\r\n");
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_stricmp(argv[2], "scb") == 0)
    {
        if (3 == argc)
        {
            sc_show_scb_all(ulIndex);
        }
        else
        {
            return -1;
        }
    }
    else if (dos_stricmp(argv[2], "task") == 0)
    {
        if (3 == argc)
        {
            sc_show_task(ulIndex, U32_BUTT, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_task(ulIndex, ulID, U32_BUTT);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Task ID while show the Task(s).\r\n");
                return -1;
            }
        }
        else if (5 == argc)
        {
            if (dos_atoul(argv[4], &ulID) < 0)
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Customer ID while show the Task(s).\r\n");
                return -1;
            }

            if (dos_strnicmp(argv[3], "custom", dos_strlen("custom")) == 0)
            {
                sc_show_task(ulIndex, U32_BUTT, ulID);
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_stricmp(argv[2], "caller") == 0)
    {
        if (3 == argc)
        {
            sc_show_caller_for_task(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_caller_for_task(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Task ID while show the Caller(s).\r\n");
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_stricmp(argv[2], "callee") == 0)
    {
        if (3 == argc)
        {
            sc_show_callee_for_task(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_callee_for_task(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Task ID while show the Caller(s).\r\n");
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_stricmp(argv[2], "agentgrp") == 0)
    {
        if (3 == argc)
        {
            sc_show_agent_group(ulIndex, U32_BUTT, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_agent_group_detail(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Agent Group ID while show the Agent Group(s).\r\n");
                return -1;
            }
        }
        else if (5 == argc)
        {
            if (dos_atoul(argv[4], &ulID) < 0)
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Agent Group ID while show the Agent Group(s).\r\n");
                return -1;
            }

            if (dos_strnicmp(argv[3], "custom", dos_strlen("custom")) == 0)
            {
                sc_show_agent_group(ulIndex, ulID, U32_BUTT);
            }
            else if (dos_strnicmp(argv[3], "group", dos_strlen("group")) == 0)
            {
                sc_show_agent_group(ulIndex, U32_BUTT, ulID);
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_stricmp(argv[2], "agent") == 0)
    {
        if (3 == argc)
        {
            sc_show_agent(ulIndex, U32_BUTT, U32_BUTT, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_agent(ulIndex, ulID, U32_BUTT, U32_BUTT);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Agent Group ID while show the Agent Group(s).\r\n");
                return -1;
            }
        }
        else if (5 == argc)
        {
            if (dos_atoul(argv[4], &ulID) < 0)
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Agent Group ID while show the Agent Group(s).\r\n");
                return -1;
            }

            if (dos_strnicmp(argv[3], "custom", dos_strlen("custom")) == 0)
            {
                sc_show_agent(ulIndex, U32_BUTT, ulID, U32_BUTT);
            }
            else if (dos_strnicmp(argv[3], "group", dos_strlen("group")) == 0)
            {
                sc_show_agent(ulIndex, U32_BUTT, U32_BUTT, ulID);
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (0 == dos_stricmp(argv[2], "route"))
    {
        if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return -1;
            }
        }
        else if (3 == argc)
        {
            ulID = U32_BUTT;
        }
        sc_show_route(ulIndex, ulID);
    }
    else if (0 == dos_stricmp(argv[2], "did"))
    {
        if (3 == argc)
        {
            sc_show_did(ulIndex, NULL);
        }
        else if (4 == argc)
        {
            sc_show_did(ulIndex, argv[3]);
        }
    }
    else if (0 == dos_stricmp(argv[2], "blacklist"))
    {
        if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return -1;
            }
        }
        else if (3 == argc)
        {
            ulID = U32_BUTT;
        }
        sc_show_black_list(ulIndex, ulID);
    }
    else if (0 == dos_stricmp(argv[2], "_caller"))
    {
        if (3 == argc)
        {
            sc_show_caller(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return -1;
            }
            sc_show_caller(ulIndex, ulID);
        }
    }
    else if (0 == dos_stricmp(argv[2], "callergrp"))
    {
        if (3 == argc)
        {
            sc_show_caller_grp(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return -1;
            }
            sc_show_caller_grp(ulIndex, ulID);
        }
        else if (5 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return -1;
            }
            if (0 != dos_stricmp(argv[4], "caller"))
            {
                DOS_ASSERT(0);
                return -1;
            }
            /* 显示号码组下的所有号码 */
            sc_show_caller_by_callergrp(ulIndex, ulID);
        }
    }
    else if (0 == dos_stricmp(argv[2], "callerset"))
    {
        if (3 == argc)
        {
            sc_show_caller_setting(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return -1;
            }
            sc_show_caller_setting(ulIndex, ulID);
        }
    }
    else if (0 == dos_stricmp(argv[2], "tt"))
    {
        if (3 == argc)
        {
            sc_show_tt(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return -1;
            }
            sc_show_tt(ulIndex, ulID);
        }
    }
    else if (0 == dos_stricmp(argv[2], "customer"))
    {
        if (3 == argc)
        {
            sc_show_customer(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return -1;
            }
            sc_show_customer(ulIndex, ulID);
        }
    }
    else if (0 == dos_stricmp(argv[2], "transform"))
    {
        if (3 == argc)
        {
            sc_show_transform(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return -1;
            }
            sc_show_transform(ulIndex, ulID);
        }
    }
    else if (0 == dos_stricmp(argv[2], "numlmt"))
    {
        if (3 == argc)
        {
            sc_show_numlmt(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return -1;
            }
            sc_show_numlmt(ulIndex, ulID);
        }
    }
    else if (0 == dos_stricmp(argv[2], "cwq"))
    {
        if (3 == argc)
        {
            sc_show_cwq(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return -1;
            }
            sc_show_cwq(ulIndex, ulID);
        }
    }
    else if (0 == dos_stricmp(argv[2], "taskmgnt"))
    {
        if (3 == argc)
        {
            sc_show_taskmgnt(ulIndex);
        }
        else
        {
            return -1;
        }
    }
    else if (0 == dos_stricmp(argv[2], "servctrl"))
    {
        if (3 == argc)
        {
            sc_show_serv_ctrl(ulIndex, 0);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                return -1;
            }

            sc_show_serv_ctrl(ulIndex, ulID);
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

S32 cli_cc_call(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulCPS;
    S8 szBuff[512] = { 0 };

    if (argc < 3)
    {
        return -1;
    }

    if (dos_strnicmp(argv[2], "cps", dos_strlen("cps")) == 0)
    {
        if (3 == argc)
        {
            dos_snprintf(szBuff, sizeof(szBuff), "\r\n\r\nCurrent CSP is : %u\r\n\r\n", g_ulCPS);
            cli_out_string(ulIndex, szBuff);
            return 0;
        }
        else
        {
            if (DOS_ADDR_INVALID(argv[3])
                || dos_atoul(argv[3], &ulCPS) < 0)
            {
                return -1;
            }

            g_ulCPS = ulCPS;
            dos_snprintf(szBuff, sizeof(szBuff), "\r\n\r\nSet CSP to %u\r\n\r\n", g_ulCPS);
            cli_out_string(ulIndex, szBuff);
            return 0;
        }
    }
    else
    {
        return -1;
    }
}


S32 cli_cc_debug(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulLogLevel = LOG_LEVEL_INVAILD;

    if (argc < 3)
    {
        return -1;
    }

    if (dos_strnicmp(argv[2], "debug", dos_strlen("debug")) == 0)
    {
        ulLogLevel = LOG_LEVEL_DEBUG;
    }
    else if (dos_strnicmp(argv[2], "info", dos_strlen("info")) == 0)
    {
        ulLogLevel = LOG_LEVEL_INFO;
    }
    else if (dos_strnicmp(argv[2], "notice", dos_strlen("notice")) == 0)
    {
        ulLogLevel = LOG_LEVEL_NOTIC;
    }
    else if (dos_strnicmp(argv[2], "warning", dos_strlen("warning")) == 0)
    {
        ulLogLevel = LOG_LEVEL_WARNING;
    }
    else if (dos_strnicmp(argv[2], "error", dos_strlen("error")) == 0)
    {
        ulLogLevel = LOG_LEVEL_ERROR;
    }
    else if (dos_strnicmp(argv[2], "cirt", dos_strlen("cirt")) == 0)
    {
        ulLogLevel = LOG_LEVEL_CIRT;
    }
    else if (dos_strnicmp(argv[2], "alert", dos_strlen("alert")) == 0)
    {
        ulLogLevel = LOG_LEVEL_ALERT;
    }
    else if (dos_strnicmp(argv[2], "emerg", dos_strlen("emerg")) == 0)
    {
        ulLogLevel = LOG_LEVEL_EMERG;
    }

    if (LOG_LEVEL_INVAILD == ulLogLevel)
    {
        return -1;
    }

    g_ulSCLogLevel = ulLogLevel;

    return 0;
}

S32 cli_cc_process(U32 ulIndex, S32 argc, S8 **argv)
{
    if (DOS_ADDR_INVALID(argv))
    {
        goto cc_usage;
    }

    if (!g_blSCInitOK)
    {
        cli_out_string(ulIndex, "SC not init finished Plese try later.\r\n");
        return 0;
    }

    if (argc < 3)
    {
        goto cc_usage;
    }

    if (dos_strnicmp(argv[1], "call", dos_strlen("call")) == 0)
    {
        if (cli_cc_call(ulIndex, argc, argv) < 0)
        {
            goto cc_usage;
        }

        return 0;
    }
    if (dos_strnicmp(argv[1], "debug", dos_strlen("debug")) == 0)
    {
        if (cli_cc_debug(ulIndex, argc, argv) < 0)
        {
            goto cc_usage;
        }
        else
        {
            cli_out_string(ulIndex, "Set debug level successfully.\r\n");
        }
    }
    else if (dos_strnicmp(argv[1], "show", dos_strlen("show")) == 0)
    {
        if (cli_cc_show(ulIndex, argc, argv) < 0)
        {
            goto cc_usage;
        }
    }
    else if (dos_strnicmp(argv[1], "trace", dos_strlen("trace")) == 0)
    {
        if (cli_cc_trace(ulIndex, argc, argv) < 0)
        {
            goto cc_usage;
        }
    }
    else if (dos_strnicmp(argv[1], "update", dos_strlen("update")) == 0)
    {
        /* 数据强制同步 */
        if (cli_cc_update(ulIndex, argc, argv) < 0)
        {
            goto cc_usage;
        }
    }
    else if (dos_strnicmp(argv[1], "test", dos_strlen("test")) == 0)
    {
        /* 专门用来进行函数功能测试 */
        if (cli_cc_test(ulIndex, argc, argv) < 0)
        {
            return 0;
        }
    }
    else
    {
        goto cc_usage;
    }

    return 0;

cc_usage:

    cli_out_string(ulIndex, "\r\n");
    cli_out_string(ulIndex, "cc show httpd|http|gateway|gwgrp|scb|route|blacklist|tt|_caller|callergrp|callerset|customer|transform|numlmt|cwq|taskmgnt [id]\r\n");
    cli_out_string(ulIndex, "cc show did [did_number]\r\n");
    cli_out_string(ulIndex, "cc show task [custom] id\r\n");
    cli_out_string(ulIndex, "cc show caller|callee taskid\r\n");
    cli_out_string(ulIndex, "cc show agent|agentgrp [custom|group] id\r\n");
    cli_out_string(ulIndex, "cc debug debug|info|notice|warning|error|cirt|alert|emerg\r\n");
    cli_out_string(ulIndex, "cc trace func|http|api|acd|task|dialer|esl|bss|all on|off\r\n");
    cli_out_string(ulIndex, "cc trace scb scbid|all on|off\r\n");
    cli_out_string(ulIndex, "cc trace task taskid|all on|off\r\n");
    cli_out_string(ulIndex, "cc trace call <callee num> <caller num> on|off\r\n");
    cli_out_string(ulIndex, "cc update route|gateway|gwgrp [id]\r\n");

    return 0;
}

/**
  * 函数名: S32 cli_cc_update(U32 ulIndex, S32 argc, S8 **argv)
  * 参数:
  * 功能: 数据强制同步的统一处理接口
  * 返回: 成功返回DOS_SUCC,否则返回DOS_FAIL
  **/
S32 cli_cc_update(U32 ulIndex, S32 argc, S8 **argv)
{
    S8  szBuff[1024] = {0};
    U32 ulID = U32_BUTT;

    if (0 == dos_strnicmp(argv[2], "route", dos_strlen("route")))
    {
        /* 处理与路由相关的 */
        if (3 == argc)
        {
            /* 全部处理 */
            sc_update_route(U32_BUTT);
        }
        else if (4 == argc)
        {
            /* 根据id更新 */
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
            sc_update_route(ulID);
        }
        else
        {
            dos_snprintf(szBuff, sizeof(szBuff), "You inputed %d %s.", argc, argc == 1?"param":"params");
            cli_out_string(ulIndex, szBuff);
            return DOS_FAIL;
        }
    }
    else if (0 == dos_strnicmp(argv[2], "gateway", dos_strlen("gateway")))
    {
        /* 处理与网关相关的 */
        if (3 == argc)
        {
            /* 全部处理 */
            sc_update_gateway(U32_BUTT);
        }
        else if (4 == argc)
        {
            /* 根据id更新 */
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
            sc_update_gateway(ulID);
        }
        else
        {
            dos_snprintf(szBuff, sizeof(szBuff), "You inputed %d %s.", argc, argc == 1?"param":"params");
            cli_out_string(ulIndex, szBuff);
            return DOS_FAIL;
        }
    }
    else if (0 == dos_strnicmp(argv[2], "gwgrp", dos_strlen("gwgrp")))
    {
        /* 处理与网关组相关的 */
        if (3 == argc)
        {
            /* 全部处理 */
            sc_update_gateway_grp(U32_BUTT);
        }
        else if(4 == argc)
        {
            /* 根据id去处理 */
            sc_update_gateway_grp(ulID);
        }
        else
        {
            dos_snprintf(szBuff, sizeof(szBuff), "You inputed %d %s.", argc, argc == 1 ? "param" : "params");
            cli_out_string(ulIndex, szBuff);
            return DOS_FAIL;
        }
    }
    else
    {
        /* 直接goto help */
        goto help;
    }
    return DOS_SUCC;

help:
    dos_snprintf(szBuff, sizeof(szBuff), "\r\nParam \'%s\' is not supported.", argv[2]);
    cli_out_string(ulIndex, szBuff);
    return DOS_FAIL;
}

/**
  * 函数名: S32 cli_cc_test(U32 ulIndex, S32 argc, S8 **argv)
  * 参数:
  * 功能: CC模块函数功能测试函数
  * 返回: 成功返回DOS_SUCC,否则返回DOS_FAIL
  **/
S32 cli_cc_test(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulRet = 0;
    S8  szBuff[256] = {0};

    /* 此分支专门用来测试主叫号码组 */
    if (0 == dos_stricmp(argv[2], "callerset"))
    {
        U32 ulCustomerID = U32_BUTT, ulSrcID = U32_BUTT, ulSrcType = U32_BUTT;
        S8  szNumber[32] = {0};

        if (6 != argc)
        {
            cli_out_string(ulIndex, "\r\nInput error param.");
            return DOS_FAIL;
        }
        if (dos_atoul(argv[3], &ulCustomerID) < 0
            || dos_atoul(argv[4], &ulSrcID) < 0
            || dos_atoul(argv[5], &ulSrcType) < 0)
        {
            cli_out_string(ulIndex, "\r\nParam 4,5,6 must be integers.");
            return DOS_FAIL;
        }

        ulRet = sc_caller_setting_select_number(ulCustomerID, ulSrcID, ulSrcType, szNumber, sizeof(szNumber));
        if (DOS_SUCC != ulRet)
        {
            cli_out_string(ulIndex, "\r\nSorry, Select Number FAIL.");
            return DOS_FAIL;
        }
        dos_snprintf(szBuff, sizeof(szBuff), "\r\nNumber    \r\n----------\r\n%s\r\n", szNumber);
        cli_out_string(ulIndex, szBuff);
    }
    return DOS_SUCC;
}

/**
 * 函数名:S32 sc_update_gateway(U32 ulID)
 * 参数:  U32 ulID  网关ID
 * 功能:  强制更新网关，根据id或者表名
 * 返回:  更新成功则返回DOS_SUCC，否则返回DOS_FAIL;
 **/
S32 sc_update_gateway(U32 ulID)
{
    U32 ulRet = U32_BUTT;

    if (U32_BUTT == ulID)
    {
        /* 所有数据全部强制更新 */
        sc_load_gateway(SC_INVALID_INDEX);
        /* 删除多余数据 */
        sc_del_invalid_gateway();
    }
    else
    {
        /* 强制更新该id数据 */
        sc_load_gateway(ulID);
    }

    /* 使网关生效 */
    ulRet = sc_ep_esl_execute_cmd("bgapi sofia profile external rescan");
    if (DOS_SUCC != ulRet)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
  * 函数名:S32 sc_update_gateway_grp(U32 ulID)
  * 参数: U32 ulID  网关ID
  * 功能: 强制更新网关组，根据id或者表名
  * 返回: 更新成功则返回DOS_SUCC，否则返回DOS_FAIL;
  **/
S32 sc_update_gateway_grp(U32 ulID)
{
    if (U32_BUTT == ulID)
    {
        /* 加载数据 */
        sc_load_gateway_grp(SC_INVALID_INDEX);
        /* 删除没有来自数据库的数据 */
        sc_del_invalid_gateway_grp();
    }
    else
    {
        sc_load_gateway_grp(ulID);
    }
    return DOS_SUCC;
}

/**
  * 函数名:S32 sc_update_route(U32 ulID)
  * 参数: U32 ulID  路由
  * 功能: 强制更新路由,根据id或者表名
  * 返回: 更新成功则返回DOS_SUCC，否则返回DOS_FAIL;
  **/
S32 sc_update_route(U32 ulID)
{
    if (U32_BUTT == ulID)
    {
        /* 全部更新 */
        sc_load_route(SC_INVALID_INDEX);
        /* 删除不在数据库中的成员 */
        sc_del_invalid_route();
    }
    else
    {
        sc_load_route(ulID);
    }
    return DOS_SUCC;
}

VOID sc_debug(U32 ulSubMod, U32 ulLevel, const S8* szFormat, ...)
{
    va_list         Arg;
    U32             ulTraceTagLen;
    S8              szTraceStr[1024] = {0, };
    BOOL            bIsOutput = DOS_FALSE;

    if (ulLevel >= LOG_LEVEL_INVAILD)
    {
        return;
    }

    /* warning级别以上强制输出 */
    if (ulLevel <= LOG_LEVEL_WARNING)
    {
        bIsOutput = DOS_TRUE;
    }

    if (!bIsOutput
        && ulSubMod >= SC_SUB_MOD_BUTT)
    {
        bIsOutput = DOS_TRUE;
    }

    if (!bIsOutput
        && SC_CHECK_SUBMOD(g_ulTraceFlags, ulSubMod))
    {
        bIsOutput = DOS_TRUE;
    }

    if (!bIsOutput
        && ulLevel <= g_ulSCLogLevel)
    {
        bIsOutput = DOS_TRUE;
    }

    if(!bIsOutput)
    {
        return;
    }

    switch(ulSubMod)
    {
        case SC_FUNC:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_FNC:");
            break;
        case SC_HTTPD:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_HTD:");
            break;
        case SC_HTTP_API:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_API:");
            break;
        case SC_ACD:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_ACD:");
            break;
        case SC_TASK_MNGT:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_TSK:");
            break;
        case SC_TASK:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_TSK:");
            break;
        case SC_DIALER:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_DLR:");
            break;
        case SC_ESL:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_ESL:");
            break;
        case SC_BS:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_BSS:");
            break;
        case SC_SYN:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_SYN:");
            break;
        case SC_PUB:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_PUB:");
            break;
        default:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC:");
            break;
    }

    ulTraceTagLen = dos_strlen(szTraceStr);

    va_start(Arg, szFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, szFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    //printf("%s\r\n", szTraceStr);
    dos_log(ulLevel, LOG_TYPE_RUNINFO, szTraceStr);
}


VOID sc_call_trace(SC_SCB_ST *pstSCB, const S8 *szFormat, ...)
{
    SC_TASK_CB_ST                 *pstTCB      = NULL;
    SC_CALLER_QUERY_NODE_ST       *pstCaller   = NULL;
    va_list argptr;
    char szBuf[1024] = {0, };

    SC_TRACE_IN((U64)pstSCB, 0, 0, 0);

    if (!pstSCB)
    {
        SC_TRACE_OUT();
        return;
    }

    if (pstSCB->usTCBNo <= SC_MAX_TASK_NUM)
    {
        pstTCB = sc_tcb_get_by_id(pstSCB->usTCBNo);
    }

    if (g_ulCallTraceAll)
    {
        goto trace;
    }

    if (pstTCB && pstTCB->bTraceCallON)
    {
        goto trace;
    }

    if (pstCaller && pstCaller->bTraceON)
    {
        goto trace;
    }

    return;

trace:
    va_start(argptr, szFormat);
    vsnprintf(szBuf, sizeof(szBuf), szFormat, argptr);
    va_end(argptr);
    /*
     *格式: <SCB:No, status, token, caller, callee, uuid, trunk><TCB:No, status, ID, Custom,><CALLER:No, num,><SITE:No, status, SIP, id, externsion>
     */
    sc_logr_debug(SC_ESL, "Call Trace:%s\r\n"
                  "\t[SCB Info]: No:%05d, status: %d, caller: %s, callee: %s, uuid:%s\r\n"
                  "\t[TCB Info]: No:%05d, status: %d, Task ID: %d, Custom ID: %d, AgentQueue: %u\r\n"
                  , szBuf
                  , pstSCB->usSCBNo
                  , pstSCB->bValid
                  , ('\0' == pstSCB->szCallerNum[0]) ? "NULL" : pstSCB->szCallerNum
                  , ('\0' == pstSCB->szCalleeNum[0]) ? "NULL" : pstSCB->szCalleeNum
                  , ('\0' == pstSCB->szUUID[0]) ? "NULL" : pstSCB->szUUID
                  , pstTCB ? pstTCB->usTCBNo : -1
                  , pstTCB ? pstTCB->ucValid : -1
                  , pstTCB ? pstTCB->ulTaskID : -1
                  , pstTCB ? pstTCB->ulCustomID : -1
                  , pstTCB ? pstTCB->ulAgentQueueID : -1);
}

VOID sc_task_trace(SC_TASK_CB_ST *pstTCB, const S8* szFormat, ...)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (!pstTCB)
    {
        SC_TRACE_OUT();
        return;
    }

    if (!pstTCB->bTraceON && !g_ulTaskTraceAll)
    {
        SC_TRACE_OUT();
        return;
    }

    /*
     * 格式: <任务控制块号，任务ID, 用户ID, 状态>
     **/
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

