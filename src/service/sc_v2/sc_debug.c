/**
 * @file : sc_debug.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 调试函数集合
 *
 * @date: 2016年1月9日
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_def.h"
#include "sc_res.h"
#include "sc_httpd.h"
#include "sc_http_api.h"
#include "sc_debug.h"
#include "bs_pub.h"

/* 坐席组的hash表 */
extern HASH_TABLE_S      *g_pstAgentList;
extern pthread_mutex_t   g_mutexAgentList;

extern HASH_TABLE_S      *g_pstGroupList;
extern pthread_mutex_t   g_mutexGroupList;

extern SC_HTTPD_CB_ST        *g_pstHTTPDList[SC_MAX_HTTPD_NUM];
extern SC_HTTP_CLIENT_CB_S   *g_pstHTTPClientList[SC_MAX_HTTP_CLIENT_NUM];
extern SC_BS_MSG_STAT_ST stBSMsgStat;
extern DLL_S               g_stCWQMngt;
extern U32          g_ulCPS;
extern SC_TASK_CB           *g_pstTaskList;
extern pthread_mutex_t      g_mutexTaskList;
extern BOOL                 g_blSCInitOK;

extern SC_MOD_LIST_ST       astSCModList[];

extern SC_SRV_CB       *g_pstSCBList;
extern SC_LEG_CB       *g_pstLegCB;

U32         g_ulSCLogLevel = LOG_LEVEL_DEBUG;

S8 *g_pszSCCommandStr[] = {
    "CMD_CALL_SETUP",
    "CMD_CALL_RINGING",
    "CMD_CALL_ANSWER",
    "CMD_CALL_BRIDGE",
    "CMD_CALL_RELEASE",
    "CMD_PLAYBACK",
    "CMD_PLAYBACK_STOP",
    "CMD_RECORD",
    "CMD_RECORD_STOP",
    "CMD_HOLD",
    "CMD_UNHOLD",
    "CMD_IVR_CTR",
    ""
};

S8 *g_pszSCEventStr[] = {
    "EVT_CALL_SETUP",
    "EVT_CALL_RINGING",
    "EVT_CALL_AMSWERED",
    "EVT_BRIDGE_START",
    "EVT_HOLD",
    "EVT_BRIDGE_STOP",
    "EVT_CALL_RERLEASE",
    "EVT_CALL_STATUS",
    "EVT_DTMF",
    "EVT_RECORD_START",
    "EVT_RECORD_END",
    "EVT_PLAYBACK_START",
    "EVT_PLAYBACK_END",
    "EVT_AUTH_RESULT",
    "EVT_ERROR_PORT",

    "",
};

S8 *sc_command_str(U32 ulCommand)
{
    if (ulCommand >= SC_CMD_BUTT)
    {
        return NULL;
    }

    return g_pszSCCommandStr[ulCommand];
}

S8 *sc_event_str(U32 ulEvent)
{
    if (ulEvent >= SC_EVT_BUTT)
    {
        return NULL;
    }

    return g_pszSCEventStr[ulEvent];
}


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

const S8* g_pszAgentStatus[] =
{
    "OFFLINE",
    "IDLE",
    "AWAY",
    "BUSY",
    "PROC"
};

const S8* g_pszAgentBindType[] =
{
    "SIP",
    "TELEPHONE",
    "MOBILE",
    "TT_NUMBER"
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

const S8* g_pszCallerPolicy[] =
{
    "IN-ORDER",
    "RANDOM"
};

const S8* g_pszDstCallerType[] =
{
    "CALLER",
    "DID",
    "CALLERGRP"
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

const S8* g_pszCallerType[] =
{
    "CALLER",
    "DID"
};

const S8* sc_translate_task_status(U32 ulStatus)
{
    if (ulStatus < sizeof(g_pszTaskStatus) / sizeof(S8 *))
    {
        return g_pszTaskStatus[ulStatus];
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

const S8* sc_translate_agent_bind_type(U32 ulBindType)
{
    if (ulBindType < sizeof(g_pszAgentBindType) / sizeof(S8*))
    {
        return g_pszAgentBindType[ulBindType];
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

const S8* sc_translate_caller_policy(U32 ulPolicy)
{
    if (ulPolicy < sizeof(g_pszCallerPolicy)/sizeof(S8*))
    {
        return g_pszCallerPolicy[ulPolicy];
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

const S8* sc_translate_caller_type(U32 ulType)
{
    if (ulType < sizeof(g_pszCallerType)/sizeof(S8*))
    {
        return g_pszCallerType[ulType];
    }
    return "UNKNOWN";
}

const S8* sc_translate_server_cb(SC_SRV_CB *pstSCB, SC_SCB_TAG_ST *pstServerAddr)
{
    if (DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstServerAddr))
    {
        return "Error";
    }

    if (pstServerAddr == (SC_SCB_TAG_ST *)&pstSCB->stCall)
    {
        return "Basic Call";
    }
    else if (pstServerAddr == (SC_SCB_TAG_ST *)&pstSCB->stPreviewCall)
    {
        return "Preview Call";
    }
    else if (pstServerAddr == (SC_SCB_TAG_ST *)&pstSCB->stAutoCall)
    {
        return "Auto Call";
    }
    else if (pstServerAddr == (SC_SCB_TAG_ST *)&pstSCB->stVoiceVerify)
    {
        return "Voice Verify";
    }
    else if (pstServerAddr == (SC_SCB_TAG_ST *)&pstSCB->stAccessCode)
    {
        return "Access Code";
    }
    else if (pstServerAddr == (SC_SCB_TAG_ST *)&pstSCB->stHold)
    {
        return "Hold";
    }
    else if (pstServerAddr == (SC_SCB_TAG_ST *)&pstSCB->stTransfer)
    {
        return "Transfer";
    }
    else if (pstServerAddr == (SC_SCB_TAG_ST *)&pstSCB->stIncomingQueue)
    {
        return "Incoming Queue";
    }
    else if (pstServerAddr == (SC_SCB_TAG_ST *)&pstSCB->stInterception)
    {
        return "Interception";
    }
    else if (pstServerAddr == (SC_SCB_TAG_ST *)&pstSCB->stWhispered)
    {
        return "Whispered";
    }
    else if (pstServerAddr == (SC_SCB_TAG_ST *)&pstSCB->stMarkCustom)
    {
        return "Mark Customer";
    }

    return "Error";
}

const S8* sc_translate_server_type(U32 ulSrvType)
{
    switch (ulSrvType)
    {
        case BS_SERV_OUTBAND_CALL:
            return "OUTBAND CALL";
        case BS_SERV_INBAND_CALL:
            return "INBAND CALL";
        case BS_SERV_INTER_CALL:
            return "INTER CALL";
        case BS_SERV_AUTO_DIALING:
            return "AUTO CALL";
        case BS_SERV_PREVIEW_DIALING:
            return "PREVIEW CALL";
        case BS_SERV_PREDICTIVE_DIALING:
            return "PREDICTIVE CALL";
        case BS_SERV_RECORDING:
            return "RECORD";
        case BS_SERV_CALL_FORWARD:
            return "CALL FORWARD";
        case BS_SERV_CALL_TRANSFER:
            return "CALL TRANSFER";
        case BS_SERV_PICK_UP:
            return "PICK UP";
        case BS_SERV_CONFERENCE:
            return "CONFERENCE";
        case BS_SERV_VOICE_MAIL:
            return "VOICE MAIL";
        case BS_SERV_SMS_SEND:
            return "SMS SEND";
        case BS_SERV_SMS_RECV:
            return "SMS RECV";
        case BS_SERV_MMS_SEND:
            return "MMS SEND";
        case BS_SERV_MMS_RECV:
            return "MMS RECV";
        case BS_SERV_RENT:
            return "RENT";
        case BS_SERV_SETTLE:
            return "SETTLE";
        case BS_SERV_VERIFY:
            return "VERIFY";
        default:
            return "ERROR";
    }
}

const S8 *sc_translate_module(U32 ulMod)
{
    switch (ulMod)
    {
        case SC_MOD_DB:
            return "DB";
        case SC_MOD_DB_WQ:
            return "DB_WQ";
        case SC_MOD_DIGIST:
            return "DIGIST";
        case SC_MOD_RES:
            return "RES";
        case SC_MOD_ACD:
            return "ACD";
        case SC_MOD_EVENT:
            return "EVENT";
        case SC_MOD_SU:
            return "SU";
        case SC_MOD_BS:
            return "BS";
        case SC_MOD_EXT_MNGT:
            return "EXT_MNGT";
        case SC_MOD_CWQ:
            return "CWQ";
        case SC_MOD_PUBLISH:
            return "PUBLISH";
        case SC_MOD_ESL:
            return "ESL";
        case SC_MOD_HTTP_API:
            return "HTTP_API";
        case SC_MOD_DATA_SYN:
            return "DATA_SYN";
        case SC_MOD_TASK:
            return "TASK";
        default:
            return "ERROR";
    }
}


S32 sc_cc_show_agent_stat(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulAgentID    = U32_BUTT;
    U32 ulGroupID    = U32_BUTT;
    U32 ulCustoemrID = U32_BUTT;
    U32 ulHashIndex  = U32_BUTT;
    S8                 szCmdBuff[300]     = {0, };
    HASH_NODE_S        *pstHashNode       = NULL;
    SC_AGENT_NODE_ST   *pstAgentQueueNode = NULL;

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
                          , "Connected", "Duration", "TimesSignin", "TimesOnline");
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
                                  , pstAgentQueueNode->pstAgentInfo->ulAgentID
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
                if (ulAgentID == pstAgentQueueNode->pstAgentInfo->ulAgentID)
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
    S8 szCmdBuff[1024] = {0, };
    U32 i = 0, ulCnt = 0;
    SC_SRV_CB *pstSCB = NULL;
    S8  szAllocTime[32]  = {0,};
    time_t  stTime;

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nShow the SCB List:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%7s%12s%12s%24s%10s%24s"
                    , "Index", "Cusomer", "Agent", "Alloc Time", "Trace", "CurrentSrv");
    cli_out_string(ulIndex, szCmdBuff);

    for (i=0; i<SC_SCB_SIZE; i++)
    {
        pstSCB = &g_pstSCBList[i];
        if (!pstSCB->bValid)
        {
            continue;
        }

        stTime = (time_t)pstSCB->ulAllocTime;
        strftime(szAllocTime, sizeof(szAllocTime), "%Y-%m-%d %H:%M:%S", localtime(&stTime));

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%7u%12u%12u%24s%10s%24s"
                        , pstSCB->ulSCBNo
                        , pstSCB->ulCustomerID == U32_BUTT ? 0 : pstSCB->ulCustomerID
                        , pstSCB->ulAgentID == U32_BUTT ? 0 : pstSCB->ulAgentID
                        , szAllocTime
                        , pstSCB->bTrace ? "true" : "false"
                        , sc_translate_server_cb(pstSCB, pstSCB->pstServiceList[pstSCB->ulCurrentSrv]));
        cli_out_string(ulIndex, szCmdBuff);
        ulCnt++;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------------\r\n\r\n");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal: %u\r\n\r\n", ulCnt);
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_scb_detail(U32 ulIndex, U32 ulSCBID)
{
    S8 szCmdBuff[1024] = {0, };
    U32 i = 0;
    SC_SRV_CB *pstSCB = NULL;
    S8  szAllocTime[32]  = {0,};
    time_t  stTime;

    pstSCB = sc_scb_get(ulSCBID);
    if (DOS_ADDR_INVALID(pstSCB))
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nThe SCBNo %u is invalid", ulSCBID);
        cli_out_string(ulIndex, szCmdBuff);
        return;
    }

    if (!pstSCB->bValid)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nThe SCBNo %u is not alloc", ulSCBID);
        cli_out_string(ulIndex, szCmdBuff);
        return;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nShow the SCB %u detail:", ulSCBID);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%7s%12s%12s%24s%10s%24s"
                    , "Index", "Cusomer", "Agent", "Alloc Time", "Trace", "CurrentSrv");
    cli_out_string(ulIndex, szCmdBuff);
    stTime = (time_t)pstSCB->ulAllocTime;
    strftime(szAllocTime, sizeof(szAllocTime), "%Y-%m-%d %H:%M:%S", localtime(&stTime));

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%7u%12u%12u%24s%10s%24s"
                    , pstSCB->ulSCBNo
                    , pstSCB->ulCustomerID == U32_BUTT ? 0 : pstSCB->ulCustomerID
                    , pstSCB->ulAgentID == U32_BUTT ? 0 : pstSCB->ulAgentID
                    , szAllocTime
                    , pstSCB->bTrace ? "true" : "false"
                    , sc_translate_server_cb(pstSCB, pstSCB->pstServiceList[pstSCB->ulCurrentSrv]));
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nShow server list:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    for (i=0; i<SC_SRV_BUTT; i++)
    {
        if (pstSCB->pstServiceList[i] != NULL)
        {
            dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%s", sc_translate_server_cb(pstSCB, pstSCB->pstServiceList[i]));
            cli_out_string(ulIndex, szCmdBuff);
        }
    }
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nShow server type:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    for (i=0; i<SC_MAX_SERVICE_TYPE; i++)
    {
        if (pstSCB->aucServType[i] >= BS_SERV_BUTT
            || pstSCB->aucServType[i] == 0)
        {
            continue;
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%s", sc_translate_server_type(pstSCB->aucServType[i]));
        cli_out_string(ulIndex, szCmdBuff);
    }
}

VOID sc_show_leg_all(U32 ulIndex)
{
    S8 szCmdBuff[1024] = {0, };
    U32 i = 0, ulCnt = 0;
    SC_LEG_CB *pstLeg = NULL;
    S8  szAllocTime[32]  = {0,};
    time_t  stTime;

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nShow the Leg List:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%7s%7s%7s%7s%24s%10s%45s"
                    , "Index", "SCBNo", "Codec", "Ptime", "Alloc Time", "Trace", "szUUID");
    cli_out_string(ulIndex, szCmdBuff);

    for (i=0; i<SC_LEG_CB_SIZE; i++)
    {
        pstLeg = &g_pstLegCB[i];
        if (!pstLeg->bValid)
        {
            continue;
        }

        stTime = (time_t)pstLeg->ulAllocTime;
        strftime(szAllocTime, sizeof(szAllocTime), "%Y-%m-%d %H:%M:%S", localtime(&stTime));

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%7u%7u%7u%7u%24s%10s%45s"
                        , pstLeg->ulCBNo
                        , pstLeg->ulSCBNo
                        , pstLeg->ucCodec
                        , pstLeg->ucPtime
                        , szAllocTime
                        , pstLeg->bTrace ? "true" : "false"
                        , pstLeg->szUUID);
        cli_out_string(ulIndex, szCmdBuff);
        ulCnt++;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------------\r\n\r\n");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal: %u\r\n\r\n", ulCnt);
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_leg_detail(U32 ulIndex, U32 ulLegID)
{
    S8 szCmdBuff[1024] = {0, };
    SC_LEG_CB *pstLeg = NULL;
    S8  szAllocTime[32]  = {0,};
    time_t  stTime;

    pstLeg = sc_lcb_get(ulLegID);
    if (DOS_ADDR_INVALID(pstLeg))
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nThe LegNo %u is invalid", ulLegID);
        cli_out_string(ulIndex, szCmdBuff);
        return;
    }

    if (!pstLeg->bValid)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nThe LegNo %u is not alloc", ulLegID);
        cli_out_string(ulIndex, szCmdBuff);
        return;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nShow the LegNo %u detail:", ulLegID);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%7s%7s%7s%7s%24s%10s%45s"
                    , "Index", "SCBNo", "Codec", "Ptime", "Alloc Time", "Trace", "szUUID");
    cli_out_string(ulIndex, szCmdBuff);
    stTime = (time_t)pstLeg->ulAllocTime;
    strftime(szAllocTime, sizeof(szAllocTime), "%Y-%m-%d %H:%M:%S", localtime(&stTime));

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%7u%7u%7u%7u%24s%10s%45s"
                        , pstLeg->ulCBNo
                        , pstLeg->ulSCBNo
                        , pstLeg->ucCodec
                        , pstLeg->ucPtime
                        , szAllocTime
                        , pstLeg->bTrace ? "true" : "false"
                        , pstLeg->szUUID);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nShow server list:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%20s%10s%10s"
                        , "Type", "Status", "Model");
    cli_out_string(ulIndex, szCmdBuff);

    if (pstLeg->stCall.bValid)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%20s%10u", "Basic Call", pstLeg->stCall.ucStatus);
        cli_out_string(ulIndex, szCmdBuff);
    }
    if (pstLeg->stRecord.bValid)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%20s%10u", "Record", pstLeg->stRecord.usStatus);
        cli_out_string(ulIndex, szCmdBuff);
    }
    if (pstLeg->stPlayback.bValid)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%20s%10u", "Playback", pstLeg->stPlayback.usStatus);
        cli_out_string(ulIndex, szCmdBuff);
    }
    if (pstLeg->stBridge.bValid)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%20s%10u", "Bridge", pstLeg->stBridge.usStatus);
        cli_out_string(ulIndex, szCmdBuff);
    }
    if (pstLeg->stHold.bValid)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%20s%10u", "Hold", pstLeg->stHold.usStatus);
        cli_out_string(ulIndex, szCmdBuff);
    }
    if (pstLeg->stMux.bValid)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%20s%10u%10u", "Mux", pstLeg->stMux.usStatus, pstLeg->stMux.usMode);
        cli_out_string(ulIndex, szCmdBuff);
    }
    if (pstLeg->stMcx.bValid)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%20s%10u", "Mcx", pstLeg->stMcx.usStatus);
        cli_out_string(ulIndex, szCmdBuff);
    }
    if (pstLeg->stIVR.bValid)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%20s%10u", "IVR", pstLeg->stIVR.usStatus);
        cli_out_string(ulIndex, szCmdBuff);
    }
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
    SC_TASK_CB      *pstTCB;

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
        pstTCB = &g_pstTaskList[ulTaskIndex];
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
    S8 szWeeks[4][64]  = {{0, },};
    SC_TASK_CB   *pstTCB = NULL;

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
                  "\r\n        Time Period 1 : %02u:%02u:%02u - %02u:%02u:%02u"
                  "\r\nTime Period 2 Weekday : %s"
                  "\r\n        Time Period 2 : %02u:%02u:%02u - %02u:%02u:%02u"
                  "\r\nTime Period 3 Weekday : %s"
                  "\r\n        Time Period 3 : %02u:%02u:%02u - %02u:%02u:%02u"
                  "\r\nTime Period 4 Weekday : %s"
                  "\r\n        Time Period 4 : %02u:%02u:%02u - %02u:%02u:%02u"
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
                , sc_debug_make_weeks(pstTCB->astPeriod[0].ucWeekMask, szWeeks[0], sizeof(szWeeks[0]))
                , pstTCB->astPeriod[0].ucHourBegin
                , pstTCB->astPeriod[0].ucMinuteBegin
                , pstTCB->astPeriod[0].ucSecondBegin
                , pstTCB->astPeriod[0].ucHourEnd
                , pstTCB->astPeriod[0].ucMinuteEnd
                , pstTCB->astPeriod[0].ucSecondEnd
                , sc_debug_make_weeks(pstTCB->astPeriod[1].ucWeekMask, szWeeks[1], sizeof(szWeeks[1]))
                , pstTCB->astPeriod[1].ucHourBegin
                , pstTCB->astPeriod[1].ucMinuteBegin
                , pstTCB->astPeriod[1].ucSecondBegin
                , pstTCB->astPeriod[1].ucHourEnd
                , pstTCB->astPeriod[1].ucMinuteEnd
                , pstTCB->astPeriod[1].ucSecondEnd
                , sc_debug_make_weeks(pstTCB->astPeriod[2].ucWeekMask, szWeeks[2], sizeof(szWeeks[2]))
                , pstTCB->astPeriod[2].ucHourBegin
                , pstTCB->astPeriod[2].ucMinuteBegin
                , pstTCB->astPeriod[2].ucSecondBegin
                , pstTCB->astPeriod[2].ucHourEnd
                , pstTCB->astPeriod[2].ucMinuteEnd
                , pstTCB->astPeriod[2].ucSecondEnd
                , sc_debug_make_weeks(pstTCB->astPeriod[3].ucWeekMask, szWeeks[3], sizeof(szWeeks[3]))
                , pstTCB->astPeriod[3].ucHourBegin
                , pstTCB->astPeriod[3].ucMinuteBegin
                , pstTCB->astPeriod[3].ucSecondBegin
                , pstTCB->astPeriod[3].ucHourEnd
                , pstTCB->astPeriod[3].ucMinuteEnd
                , pstTCB->astPeriod[3].ucSecondEnd);
    cli_out_string(ulIndex, szCmdBuff);

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
                  "\r\n------------------------------------------------\r\n\r\n");
    cli_out_string(ulIndex, szCmdBuff);

}

VOID sc_show_agent_group_detail(U32 ulIndex, U32 ulID)
{
    SC_AGENT_GRP_NODE_ST      *pstAgentGrouop = NULL;
    SC_AGENT_NODE_ST          *pstAgentQueueNode = NULL;
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

    sc_agent_hash_func4grp(ulID, &ulHashIndex);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashIndex, &ulID, sc_agent_group_hash_find);
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
                    , pstAgentQueueNode->pstAgentInfo->ulAgentID
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
    SC_AGENT_GRP_NODE_ST   *pstAgentGrouop = NULL;
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

            pstAgentGrouop = (SC_AGENT_GRP_NODE_ST *)pstHashNode->pHandle;
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
    SC_AGENT_NODE_ST *pstAgentQueueNode = NULL;
    HASH_NODE_S      *pstHashNode = NULL;

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

            pstAgentQueueNode = (SC_AGENT_NODE_ST *)pstHashNode->pHandle;
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
                if (ulID == pstAgentQueueNode->pstAgentInfo->ulAgentID)
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
                        , pstAgentQueueNode->pstAgentInfo->ulAgentID
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
                        , pstAgentQueueNode->pstAgentInfo->ulLegNo
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
    SC_TASK_CB *pstTCB = NULL;
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
    SC_TASK_CB  *pstTCB = NULL;
    S8  szCmdBuff[1024] = {0};
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

    cli_out_string(ulIndex, "\r\n---------------------------------------------------------------");
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal %d Caller(s).", lCount);
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

    ulHashIndex = sc_int_hash_func(ulGrpID, SC_CALLER_GRP_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulGrpID, sc_caller_group_hash_find);
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
    SC_SRV_CB      *pstSCB = NULL;
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
            pstSCB = (SC_SRV_CB *)pstListNode1->pHandle;
            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%12u%20s%6u"
                            , ulAgentGrpID
                            , szTime
                            , pstSCB->ulSCBNo);
            cli_out_string(ulIndex, szBuff);
        }
        cli_out_string(ulIndex, "\r\n--------------------------------------");
    }

    return DOS_SUCC;
}

S32 sc_show_taskmgnt(U32 ulIndex)
{
    return DOS_SUCC;
}

S32 sc_track_call_by_callee(U32 ulIndex ,S8 *pszCallee)
{
    return DOS_SUCC;
}

S32 sc_show_trace_mod(U32 ulIndex)
{
    S8  szBuff[1024] = {0};
    U32 i = 0;

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%30s%10s"
                    , "Modul", "Trace");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n--------------------------------------------------------------------------------------------------------------------------------------------------------------");

    for (i=0; i<SC_MOD_BUTT; i++)
    {
        dos_snprintf(szBuff, sizeof(szBuff), "\r\n%30s%10s"
            , sc_translate_module(i), astSCModList[i].bTrace ? "ON" : "OFF");
        cli_out_string(ulIndex, szBuff);
    }

    return DOS_SUCC;
}

#if 0
U32 sc_debug_call(U32 ulTraceFlag, S8 *pszCaller, S8 *pszCallee)
{
    U32 i       = 0;
    U32 lRet    = DOS_FAIL;

    if (DOS_ADDR_INVALID(pszCaller)
        && DOS_ADDR_INVALID(pszCallee))
    {
        return DOS_FAIL;
    }

    if (ulTraceFlag)
    {
        /* 打开跟踪 */
        if (DOS_ADDR_VALID(pszCaller))
        {
            for (i=0; i<MAX_TRACE_NUM_SIZE; i++)
            {
                if (g_aszCallerTrace[i][0] == '\0')
                {
                    dos_strncpy(g_aszCallerTrace[i], pszCaller, SC_TEL_NUMBER_LENGTH-1);
                    g_aszCallerTrace[i][SC_TEL_NUMBER_LENGTH-1] = '\0';

                    lRet = DOS_SUCC;
                    break;
                }
            }
        }

        if (DOS_ADDR_VALID(pszCallee))
        {
            for (i=0; i<MAX_TRACE_NUM_SIZE; i++)
            {
                if (g_aszCalleeTrace[i][0] == '\0')
                {
                    dos_strncpy(g_aszCalleeTrace[i], pszCallee, SC_TEL_NUMBER_LENGTH-1);
                    g_aszCalleeTrace[i][SC_TEL_NUMBER_LENGTH-1] = '\0';

                    lRet = DOS_SUCC;
                    break;
                }
            }
        }

    }
    else
    {
        /* 关闭跟踪 */
        if (DOS_ADDR_VALID(pszCaller))
        {
            for (i=0; i<MAX_TRACE_NUM_SIZE; i++)
            {
                if (dos_strcmp(g_aszCallerTrace[i], pszCaller) == 0)
                {
                    g_aszCallerTrace[i][0] = '\0';

                    lRet = DOS_SUCC;
                    break;
                }

            }
        }

        if (DOS_ADDR_VALID(pszCallee))
        {
            for (i=0; i<MAX_TRACE_NUM_SIZE; i++)
            {
                if (dos_strcmp(g_aszCalleeTrace[i], pszCallee) == 0)
                {
                    g_aszCalleeTrace[i][0] = '\0';

                    lRet = DOS_SUCC;
                    break;
                }
            }
        }
    }

    return lRet;
}

S8 *sc_show_server_name(U32 ulServerType)
{
    switch (ulServerType)
    {
        case SC_SERV_OUTBOUND_CALL:
            return "SC_SERV_OUTBOUND_CALL";
        case SC_SERV_INBOUND_CALL:
            return "SC_SERV_INBOUND_CALL";
        case SC_SERV_INTERNAL_CALL:
            return "SC_SERV_INTERNAL_CALL";
        case SC_SERV_EXTERNAL_CALL:
            return "SC_SERV_EXTERNAL_CALL";
        case SC_SERV_AUTO_DIALING:
            return "SC_SERV_AUTO_DIALING";
        case SC_SERV_PREVIEW_DIALING:
            return "SC_SERV_PREVIEW_DIALING";
        case SC_SERV_PREDICTIVE_DIALING:
            return "SC_SERV_PREDICTIVE_DIALING";
        case SC_SERV_RECORDING:
            return "SC_SERV_RECORDING";
        case SC_SERV_FORWORD_CFB:
            return "SC_SERV_FORWORD_CFB";
        case SC_SERV_FORWORD_CFU:
            return "SC_SERV_FORWORD_CFU";
        case SC_SERV_FORWORD_CFNR:
            return "SC_SERV_FORWORD_CFNR";
        case SC_SERV_BLIND_TRANSFER:
            return "SC_SERV_BLIND_TRANSFER";
        case SC_SERV_ATTEND_TRANSFER:
            return "SC_SERV_ATTEND_TRANSFER";
        case SC_SERV_PICK_UP:
            return "SC_SERV_PICK_UP";
        case SC_SERV_CONFERENCE:
            return "SC_SERV_CONFERENCE";
        case SC_SERV_VOICE_MAIL_RECORD:
            return "SC_SERV_VOICE_MAIL_RECORD";
        case SC_SERV_VOICE_MAIL_GET:
            return "SC_SERV_VOICE_MAIL_GET";
        case SC_SERV_SMS_RECV:
            return "SC_SERV_SMS_RECV";
        case SC_SERV_SMS_SEND:
            return "SC_SERV_SMS_SEND";
        case SC_SERV_MMS_RECV:
            return "SC_SERV_MMS_RECV";
        case SC_SERV_MMS_SNED:
            return "SC_SERV_MMS_SNED";
        case SC_SERV_FAX:
            return "SC_SERV_FAX";
        case SC_SERV_INTERNAL_SERVICE:
            return "SC_SERV_INTERNAL_SERVICE";
        case SC_SERV_AGENT_CALLBACK:
            return "SC_SERV_AGENT_CALLBACK";
        case SC_SERV_AGENT_SIGNIN:
            return "SC_SERV_AGENT_SIGNIN";
        case SC_SERV_AGENT_CLICK_CALL:
            return "SC_SERV_AGENT_CLICK_CALL";
        case SC_SERV_NUM_VERIFY:
            return "SC_SERV_NUM_VERIFY";
        case SC_SERV_CALL_INTERCEPT:
            return "SC_SERV_CALL_INTERCEPT";
        case SC_SERV_CALL_WHISPERS:
            return "SC_SERV_CALL_WHISPERS";
        case SC_SERV_DEMO_TASK:
            return "SC_SERV_DEMO_TASK";
        default:
            return "NULL";
    }
}

U32 sc_debug_trace_server(U32 ulTraceFlag, S8 *pszServer, U32 ulIndex)
{
    U32 ulServerType = 0;
    S8  szBuff[256] = {0};

    if (DOS_ADDR_INVALID(pszServer))
    {
        return DOS_FAIL;
    }

    if (dos_atoul(pszServer, &ulServerType) < 0)
    {
        return DOS_FAIL;
    }

    if (ulServerType >= SC_SERV_BUTT)
    {

        dos_snprintf(szBuff, sizeof(szBuff), "%u can not exceed %u\r\n"
                        , ulServerType, SC_SERV_BUTT);
    }
    else
    {
        if (ulTraceFlag)
        {
            g_aucServTraceFlag[ulServerType] = 1;
            dos_snprintf(szBuff, sizeof(szBuff), "Trace %s(%u) ON SUCC %u\r\n"
                            , sc_show_server_name(ulServerType), ulServerType);
        }
        else
        {
            g_aucServTraceFlag[ulServerType] = 0;
            dos_snprintf(szBuff, sizeof(szBuff), "Trace %s(%u) OFF SUCC %u\r\n"
                            , sc_show_server_name(ulServerType), ulServerType);
        }
    }

    cli_out_string(ulIndex, szBuff);

    return DOS_SUCC;
}

U32 sc_debug_trace_customer(U32 ulTraceFlag, S8 *pszCustomerID, U32 ulIndex)
{
    U32 ulCustomerID = 0;
    S8  szBuff[256] = {0};
    U32 i = 0;

    if (DOS_ADDR_INVALID(pszCustomerID))
    {
        return DOS_FAIL;
    }

    if (dos_atoul(pszCustomerID, &ulCustomerID) < 0)
    {
        return DOS_FAIL;
    }

    if (ulCustomerID == 0 || ulCustomerID == U32_BUTT)
    {
        dos_snprintf(szBuff, sizeof(szBuff), "Customer(%u) is not a company\r\n", ulCustomerID);
        cli_out_string(ulIndex, szBuff);

        return DOS_SUCC;
    }

    if (ulTraceFlag)
    {
        /* 判断SIP是否属于企业 */
        if (sc_ep_customer_list_find(ulCustomerID) != DOS_SUCC)
        {
            dos_snprintf(szBuff, sizeof(szBuff), "Customer(%u) is not a company\r\n", ulCustomerID);
        }
        else
        {
            for (i=0; i<MAX_TRACE_NUM_SIZE; i++)
            {
                if (g_aulCustomerTrace[i] == 0)
                {
                    g_aulCustomerTrace[i] = ulCustomerID;
                    dos_snprintf(szBuff, sizeof(szBuff), "Trace customer(%u) ON SUCC\r\n", ulCustomerID);
                    break;
                }
            }

            if (i == MAX_TRACE_NUM_SIZE)
            {
                dos_snprintf(szBuff, sizeof(szBuff), "Trace customer(%u) ON FAIL\r\n", ulCustomerID);
            }
        }
    }
    else
    {
        for (i=0; i<MAX_TRACE_NUM_SIZE; i++)
        {
            if (g_aulCustomerTrace[i] == ulCustomerID)
            {
                g_aulCustomerTrace[i] = 0;
                dos_snprintf(szBuff, sizeof(szBuff), "Trace customer(%u) OFF SUCC\r\n", ulCustomerID);
                break;
            }
        }

        if (i == MAX_TRACE_NUM_SIZE)
        {
            dos_snprintf(szBuff, sizeof(szBuff), "Trace customer(%u) OFF FAIL\r\n", ulCustomerID);
        }
    }


    cli_out_string(ulIndex, szBuff);

    return DOS_SUCC;
}


U32 sc_debug_trace_agent(U32 ulTraceFlag, S8 *pszAgentID, U32 ulIndex)
{
    U32 ulAgentID = 0;
    S8  szBuff[256] = {0};

    if (DOS_ADDR_INVALID(pszAgentID))
    {
        return DOS_FAIL;
    }

    if (dos_atoul(pszAgentID, &ulAgentID) < 0)
    {
        return DOS_FAIL;
    }

    if (sc_acd_update_agent_trace(ulTraceFlag, ulAgentID) != DOS_SUCC)
    {
        dos_snprintf(szBuff, sizeof(szBuff), "Not find agent(%u)\r\n", ulAgentID);
    }
    else
    {
        dos_snprintf(szBuff, sizeof(szBuff), "Trace Agent(%u) %s SUCC.\r\n"
                            , ulAgentID, ulTraceFlag ? "ON" : "OFF");
    }

    cli_out_string(ulIndex, szBuff);

    return DOS_SUCC;
}

U32 sc_show_trace_caller(U32 ulIndex)
{
    S8  szBuff[256] = {0};
    U32 i = 0;

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5s%24s"
                            , "ID", "Number");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n-------------------------------------------------------------------");

    for (i=0; i<MAX_TRACE_NUM_SIZE; i++)
    {
        if (g_aszCallerTrace[i][0] != '\0')
        {
            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5u%24s"
                            , i
                            , g_aszCallerTrace[i]);

            cli_out_string(ulIndex, szBuff);
        }
     }


    return DOS_SUCC;
}


U32 sc_show_trace_callee(U32 ulIndex)
{
    S8  szBuff[256] = {0};
    U32 i = 0;

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5s%24s"
                            , "ID", "Number");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n-------------------------------------------------------------------");

    for (i=0; i<MAX_TRACE_NUM_SIZE; i++)
    {
        if (g_aszCalleeTrace[i][0] != '\0')
        {
            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5u%24s"
                            , i
                            , g_aszCalleeTrace[i]);

            cli_out_string(ulIndex, szBuff);
        }
     }

    return DOS_SUCC;
}


U32 sc_show_trace_server(U32 ulIndex)
{
    S8  szBuff[256] = {0};
    U32 i = 0;

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5s%40s%10s"
                            , "ID", "Server", "Status");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n-------------------------------------------------------------------");

    for (i=0; i<SC_SERV_BUTT; i++)
    {
        dos_snprintf(szBuff, sizeof(szBuff), "\r\n%5u%40s%10s"
                        , i
                        , sc_show_server_name(i)
                        , g_aucServTraceFlag[i] ? "ON" : "OFF");

        cli_out_string(ulIndex, szBuff);
    }

    return DOS_SUCC;
}

U32 sc_show_trace_customer(U32 ulIndex)
{
    S8  szBuff[256] = {0};
    U32 i = 0;

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%10s%20s"
                            , "ID", "CustomerID");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n-------------------------------------------------------------------");

    for (i=0; i<MAX_TRACE_NUM_SIZE; i++)
    {
        if (g_aulCustomerTrace[i] != 0)
        {
            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%10u%20u"
                            , i, g_aulCustomerTrace[i]);

            cli_out_string(ulIndex, szBuff);
        }
     }

    return DOS_SUCC;
}

S8 *sc_trace_mod_name(U32 ulModID)
{
    switch (ulModID)
    {
        case SC_FUNC:
            return "SC_FUNC";
        case SC_HTTPD:
            return "SC_HTTPD";
        case SC_HTTP_API:
            return "SC_HTTP_API";
        case SC_ACD:
            return "SC_ACD";
        case SC_TASK_MNGT:
            return "SC_TASK_MNGT";
        case SC_TASK:
            return "SC_TASK";
        case SC_DIALER:
            return "SC_DIALER";
        case SC_ESL:
            return "SC_ESL";
        case SC_BS:
            return "SC_BS";
        case SC_SYN:
            return "SC_SYN";
        case SC_AUDIT:
            return "SC_AUDIT";
        case SC_DB:
            return "SC_DB";
        case SC_PUB:
            return "SC_PUB";
        case SC_DIGEST:
            return "SC_DIGEST";
        default:
            return "ERROR";
    }
}

U32 sc_show_trace_mod(U32 ulIndex)
{
    S8  szBuff[256] = {0};
    U32 i = 0;

    dos_snprintf(szBuff, sizeof(szBuff), "\r\n%10s%20s%10s"
                            , "ID", "CustomerID", "Status");
    cli_out_string(ulIndex, szBuff);
    cli_out_string(ulIndex, "\r\n-------------------------------------------------------------------");

    for (i=0; i<SC_SUB_MOD_BUTT; i++)
    {
        dos_snprintf(szBuff, sizeof(szBuff), "\r\n%10u%20s%10s"
                        , i, sc_trace_mod_name(i), (g_ulTraceFlags & (0x00000001 << i)) ? "ON" : "OFF");

        cli_out_string(ulIndex, szBuff);
     }

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
#endif

S32 cli_cc_trace_mod(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 i = 0, ulMod = 0;
    BOOL bIsTrace = DOS_FALSE;
    S8 szMsg[512] = {0};

    if (5 == argc)
    {
        if (dos_strnicmp(argv[4], "on", dos_strlen("on")) == 0)
        {
            bIsTrace = DOS_TRUE;
        }

        if (dos_strnicmp(argv[3], "all", dos_strlen("all")) == 0)
        {
            for (i=0; i<SC_MOD_BUTT; i++)
            {
                astSCModList[i].bTrace = bIsTrace;
            }

            dos_snprintf(szMsg, sizeof(szMsg), "Trave mod all %s SUCC\r\n", bIsTrace ? "ON" : "OFF");
            cli_out_string(ulIndex, szMsg);
        }
        else
        {
            if (dos_atoul(argv[3], &ulMod) != DOS_SUCC)
            {
                goto proc_fail;
            }

            if (ulMod < SC_MOD_BUTT)
            {
                astSCModList[ulMod].bTrace = bIsTrace;

                dos_snprintf(szMsg, sizeof(szMsg), "Trave mod %s %s SUCC\r\n", sc_translate_module(ulMod), bIsTrace ? "ON" : "OFF");
                cli_out_string(ulIndex, szMsg);
            }
            else
            {
                goto proc_fail;
            }
        }
    }
    else
    {
        goto proc_fail;
    }

    return DOS_SUCC;

proc_fail:
    /* 打印帮助信息 */

    return DOS_FAIL;
}

S32 cli_cc_trace(U32 ulIndex, S32 argc, S8 **argv)
{
    S32 lRet = DOS_FALSE;

    if (argc < 4)
    {
        return -1;
    }

    if (dos_strnicmp(argv[2], "mod", dos_strlen("mod")) == 0)
    {
        /* 模块跟踪 */
        lRet = cli_cc_trace_mod(ulIndex, argc, argv);
        if (lRet != DOS_SUCC)
        {
            /* 打印帮助信息 */
        }

    }

    return DOS_SUCC;
}


#if 0
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
    else if (dos_strnicmp(argv[2], "task", dos_strlen("task")) == 0)
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
                    if (sc_debug_call(DOS_FALSE, NULL, argv[4]) != DOS_SUCC)
                    {
                        cli_out_string(ulIndex, "Trace Callee off FAIL\r\n");
                    }
                    else
                    {
                        cli_out_string(ulIndex, "Trace Callee off SUCC\r\n");
                    }
                }
                else if (dos_strnicmp(argv[5], "on", dos_strlen("on")) == 0)
                {
                    if (sc_debug_call(DOS_TRUE, NULL, argv[4]) != DOS_SUCC)
                    {
                        cli_out_string(ulIndex, "Trace Callee on FAIL\r\n");
                    }
                    else
                    {
                        cli_out_string(ulIndex, "Trace Callee on SUCC\r\n");
                    }
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
                    if (sc_debug_call(DOS_FALSE, argv[4], NULL) != DOS_SUCC)
                    {
                        cli_out_string(ulIndex, "Trace Caller off FAIL\r\n");
                    }
                    else
                    {
                        cli_out_string(ulIndex, "Trace Caller off SUCC\r\n");
                    }
                }
                else if (dos_strnicmp(argv[5], "on", dos_strlen("on")) == 0)
                {
                    if (sc_debug_call(DOS_TRUE, argv[4], NULL) != DOS_SUCC)
                    {
                        cli_out_string(ulIndex, "Trace Caller on FAIL\r\n");
                    }
                    else
                    {
                        cli_out_string(ulIndex, "Trace Caller on SUCC\r\n");
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
                if (sc_debug_call(DOS_FALSE, pszCaller, pszCallee) != DOS_SUCC)
                {
                    cli_out_string(ulIndex, "Trace Caller or Callee off FAIL\r\n");
                }
                else
                {
                    cli_out_string(ulIndex, "Trace Caller and Callee off SUCC\r\n");
                }
            }
            else if (dos_strnicmp(argv[5], "off", dos_strlen("off")) == 0)
            {
                if (sc_debug_call(DOS_TRUE, pszCaller, pszCallee) != DOS_SUCC)
                {
                    cli_out_string(ulIndex, "Trace Caller or Callee on FAIL\r\n");
                }
                else
                {
                    cli_out_string(ulIndex, "Trace Caller and Callee on SUCC\r\n");
                }
            }
        }
        else
        {
            return -1;
        }

        return 0;
    }
    else if (dos_strnicmp(argv[2], "server", dos_strlen("server")) == 0)
    {
        if (5 == argc)
        {
            if (dos_strnicmp(argv[4], "off", dos_strlen("off")) == 0)
            {
                return sc_debug_trace_server(DOS_FALSE, argv[3], ulIndex);
            }
            else if (dos_strnicmp(argv[4], "on", dos_strlen("on")) == 0)
            {
                return sc_debug_trace_server(DOS_TRUE, argv[3], ulIndex);
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
    else if (dos_strnicmp(argv[2], "customer", dos_strlen("customer")) == 0)
    {
        if (5 == argc)
        {
            if (dos_strnicmp(argv[4], "off", dos_strlen("off")) == 0)
            {
                return sc_debug_trace_customer(DOS_FALSE, argv[3], ulIndex);
            }
            else if (dos_strnicmp(argv[4], "on", dos_strlen("on")) == 0)
            {
                return sc_debug_trace_customer(DOS_TRUE, argv[3], ulIndex);
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
    else if (dos_strnicmp(argv[2], "agent", dos_strlen("agent")) == 0)
    {
        if (5 == argc)
        {
            if (dos_strnicmp(argv[4], "off", dos_strlen("off")) == 0)
            {
                return sc_debug_trace_agent(DOS_FALSE, argv[3], ulIndex);
            }
            else if (dos_strnicmp(argv[4], "on", dos_strlen("on")) == 0)
            {
                return sc_debug_trace_agent(DOS_TRUE, argv[3], ulIndex);
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
#endif

VOID sc_show_cb(U32 ulIndex)
{
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
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_scb_detail(ulIndex, ulID);
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
    else if (dos_stricmp(argv[2], "leg") == 0)
    {
        if (3 == argc)
        {
            sc_show_leg_all(ulIndex);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_leg_detail(ulIndex, ulID);
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
    else if (0 == dos_stricmp(argv[2], "trace"))
    {
        if (4 == argc)
        {
            if (0 == dos_stricmp(argv[3], "caller"))
            {
                //sc_show_trace_caller(ulIndex);
            }
            else if (0 == dos_stricmp(argv[3], "callee"))
            {
                //sc_show_trace_callee(ulIndex);
            }
            else if (0 == dos_stricmp(argv[3], "server"))
            {
                //sc_show_trace_server(ulIndex);
            }
            else if (0 == dos_stricmp(argv[3], "customer"))
            {
                //sc_show_trace_customer(ulIndex);
            }
            else if (0 == dos_stricmp(argv[3], "mod"))
            {
                sc_show_trace_mod(ulIndex);
            }
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

#if 0
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
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
            sc_update_gateway_grp(ulID);
        }
        else
        {
            dos_snprintf(szBuff, sizeof(szBuff), "You inputed %d %s.", argc, argc == 1 ? "param" : "params");
            cli_out_string(ulIndex, szBuff);
            return DOS_FAIL;
        }
    }
    else if (0 == dos_strnicmp(argv[2], "did", dos_strlen("did")))
    {
        /* 强制同步 DID */
        if (3 == argc)
        {
            sc_update_did(U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
            sc_update_did(ulID);
        }
        else
        {
            dos_snprintf(szBuff, sizeof(szBuff), "You inputed %d %s.", argc, argc == 1 ? "param" : "params");
            cli_out_string(ulIndex, szBuff);
            return DOS_FAIL;
        }
    }
    else if (0 == dos_strnicmp(argv[2], "caller", dos_strlen("caller")))
    {
        /* 主叫号码 */
        if (3 == argc)
        {
            sc_update_caller(U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
            sc_update_caller(ulID);
        }
        else
        {
            dos_snprintf(szBuff, sizeof(szBuff), "You inputed %d %s.", argc, argc == 1 ? "param" : "params");
            cli_out_string(ulIndex, szBuff);
            return DOS_FAIL;
        }

    }
    else if (0 == dos_strnicmp(argv[2], "callergrp", dos_strlen("callergrp")))
    {
        /* 主叫号码 */
        if (3 == argc)
        {
            sc_update_callergrp(U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) < 0)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
            sc_update_callergrp(ulID);
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
#endif
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
#if 0
        /* 数据强制同步 */
        if (cli_cc_update(ulIndex, argc, argv) < 0)
        {
            goto cc_usage;
        }
#endif
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
 * SC模块打印函数
 *
 * @param const S8 *pszFormat 格式列表
 *
 * return NULL
 */
VOID sc_log(U32 ulLogFlags, const S8 *pszFormat, ...)
{
    va_list         Arg;
    U32             ulTraceTagLen = 0;
    S8              szTraceStr[1024] = {0, };
    BOOL            bIsOutput = DOS_FALSE;
    U32             ulLevel = 0;
    U32             ulFlags = 0;
    U32             ulMod   = 0;

    ulLevel = ulLogFlags & 0xF;
    ulMod   = (ulLogFlags & 0x3F0) >> 4;
    ulFlags = (ulLogFlags & 0xFFFFFC00);

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
        && ulLevel <= g_ulSCLogLevel)
    {
        bIsOutput = DOS_TRUE;
    }

    /* 模块跟踪，不判断 SC_MOD_DB 模块 */
    if (ulMod >= SC_MOD_BUTT || 0 == ulMod)
    {
        /* 不需要处理 */
    }
    else
    {
        if (!bIsOutput
            && astSCModList[ulMod].bTrace)
        {
            bIsOutput = DOS_TRUE;
        }
    }

    if(!bIsOutput)
    {
        return;
    }

    /* 这个地方有个BUG，0为合法值，但是如果不指定模块，也就是为0，冲突了，考虑到模块0只是初始化句柄，这么就把0过滤了 */
    if (ulMod >= SC_MOD_BUTT || 0 == ulMod)
    {
        ulTraceTagLen = dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC:");
    }
    else
    {
        ulTraceTagLen = dos_snprintf(szTraceStr, sizeof(szTraceStr), "%s:", astSCModList[ulMod].pszName);

        /* 各个模块的日志级别定义 */
        if (!bIsOutput && ulLevel > astSCModList[ulMod].ulDefaultLogLevel)
        {
            return;
        }
    }


    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(ulLevel, LOG_TYPE_RUNINFO, szTraceStr);

}

/**
 * SC模块打印函数
 *
 * @param const S8 *pszFormat 格式列表
 *
 * return NULL
 */
VOID sc_printf(const S8 *pszFormat, ...)
{
    va_list         Arg;
    S8              szTraceStr[1024] = {0, };
    U32             ulTraceTagLen = 0;

    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(LOG_LEVEL_DEBUG, LOG_TYPE_RUNINFO, szTraceStr);
}

/**
 * 跟踪打印业务控制块
 *
 * @parma SC_SRV_CB *pstSCB 业务控制块
 * @param const S8 *pszFormat 格式列表
 *
 * return NULL
 */
VOID sc_trace_scb(SC_SRV_CB *pstSCB, const S8 *pszFormat, ...)
{
    va_list         Arg;
    S8              szTraceStr[1024] = {0, };
    U32             ulTraceTagLen = 0;

    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(LOG_LEVEL_DEBUG, LOG_TYPE_RUNINFO, szTraceStr);
}

/**
 * 跟踪打印LEG控制块
 *
 * @parma SC_LEG_CB *pstLCB   LEG控制块
 * @param const S8 *pszFormat 格式列表
 *
 * return NULL
 */
VOID sc_trace_leg(SC_LEG_CB *pstLCB, const S8 *pszFormat, ...)
{
    va_list         Arg;
    S8              szTraceStr[1024] = {0, };
    U32             ulTraceTagLen = 0;

    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(LOG_LEVEL_DEBUG, LOG_TYPE_RUNINFO, szTraceStr);
}

VOID sc_trace_task(SC_TASK_CB *pstLCB, const S8 *pszFormat, ...)
{
    va_list         Arg;
    S8              szTraceStr[1024] = {0, };
    U32             ulTraceTagLen = 0;

    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(LOG_LEVEL_DEBUG, LOG_TYPE_RUNINFO, szTraceStr);
}


#ifdef __cplusplus
}
#endif /* End of __cplusplus */


