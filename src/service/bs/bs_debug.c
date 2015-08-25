/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名
 *
 *  创建时间:
 *  作    者:
 *  描    述:
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <bs_pub.h>
#include "bs_cdr.h"
#include "bs_stat.h"
#include "bs_def.h"
#include "bsd_db.h"
#include "../../util/heartbeat/heartbeat.h"

#if (INCLUDE_SYSLOG_ENABLE)

extern DLL_S g_stWebCMDTbl;
extern pthread_mutex_t g_mutexWebCMDTbl;
extern U32 g_ulLastCMDTimestamp ;
extern DLL_S g_stBSS2DMsgList;

#if INCLUDE_BH_CLIENT
extern PROCESS_INFO_ST *g_pstDebugProcInfo;
#endif

extern VOID bss_update_agent(U32 ulOpteration,JSON_OBJ_ST * pstJSONObj);
extern VOID bss_update_billing_package(U32 ulOpteration,JSON_OBJ_ST * pstJSONObj);
extern VOID bss_update_billing_rate(U32 ulOpteration,JSON_OBJ_ST * pstJSONObj);
extern VOID bss_update_customer(U32 ulOpteration,JSON_OBJ_ST * pstJSONObj);
extern VOID bss_update_call_task(U32 ulOpteration,JSON_OBJ_ST * pstJSONObj);

const S8 *g_aszBsTraceTarget[] =
{
    "FS",                   /* BS_TRACE_FS */
    "WEB",                  /* BS_TRACE_WEB */
    "DB",                   /* BS_TRACE_DB */
    "CDR",                  /* BS_TRACE_CDR */
    "BILLING",              /* BS_TRACE_BILLING */
    "ACCOUNT",              /* BS_TRACE_ACCOUNT */
    "STAT",                 /* BS_TRACE_STAT */
    "AUDIT",                /* BS_TRACE_AUDIT */
    "MAINTAIN",             /* BS_TRACE_MAINTAIN */
    "RUN"                   /* BS_TRACE_RUN */
};

const S8 *bs_trace_target_str(U32 ulTraceTarget)
{
    U32     ulTarget;

    switch(ulTraceTarget)
    {
        case BS_TRACE_FS:
            ulTarget = 0;
            break;
        case BS_TRACE_WEB:
            ulTarget = 1;
            break;
        case BS_TRACE_DB:
            ulTarget = 2;
            break;
        case BS_TRACE_CDR:
            ulTarget = 3;
            break;
        case BS_TRACE_BILLING:
            ulTarget = 4;
            break;
        case BS_TRACE_ACCOUNT:
            ulTarget = 5;
            break;
        case BS_TRACE_STAT:
            ulTarget = 6;
            break;
        case BS_TRACE_AUDIT:
            ulTarget = 7;
            break;
        case BS_TRACE_MAINTAIN:
            ulTarget = 8;
            break;
        case BS_TRACE_RUN:
            ulTarget = 9;
            break;
        default:
            return "";
    }

    return g_aszBsTraceTarget[ulTarget];
}

const S8 *g_aszBsTraceLevel[] =
{
    "EMERG",
    "ALERT",
    "CRIT",
    "ERR",
    "WARNING",
    "NOTICE",
    "INFO",
    "DEBUG"
};

const S8 *bs_trace_level_str(U32 ulTraceLevel)
{
    if (ulTraceLevel >= sizeof(g_aszBsTraceLevel)/sizeof(S8 *))
    {
        return "";
    }
    return g_aszBsTraceLevel[ulTraceLevel];
}

const S8* g_pszCustomerType[] =
{
    "CONSUMER",
    "AGENT",
    "TOP",
    "SP"
};

const S8* sc_translate_customer_type(U32 ulCustomerType)
{
    if (ulCustomerType < sizeof(g_pszCustomerType) / sizeof(S8*))
    {
        return g_pszCustomerType[ulCustomerType];
    }
    return "UNKNOWN";
}

const S8* g_pszCustomerState[] =
{
    "ACTIVE",
    "FROZEN",
    "INACTIVE"
};

const S8* bs_translate_customer_state(U32 ulCustomerState)
{
    if (ulCustomerState < sizeof(g_pszCustomerState) / sizeof(S8*))
    {
        return g_pszCustomerState[ulCustomerState];
    }
    return "UNKNOWN";
}

const S8* g_pszServType[] =
{
    "",
    "OUTBAND CALL",
    "INBAND CALL",
    "INTER CALL",
    "AUTO DIALING",
    "PREVIEW DIALING",
    "PREDICTIVE DIALING",
    "RECORDING",
    "CALL FORWARD",
    "CALL TRANSFER",
    "PICK UP",
    "CONFERENCE",
    "VOICE MAIL",
    "SMS SEND",
    "SMS RECV",
    "MMS SEND",
    "MMS RECV",
    "RENT",
    "SETTLE",
    "VERIFY"
};

const S8* bs_translate_serv_type(U32 ulServType)
{
    if (ulServType < sizeof(g_pszServType) / sizeof(S8*))
    {
        return g_pszServType[ulServType];
    }
    return "UNKNOWN";
}

const S8* g_pszBillingAttr[] =
{
    "",
    "REGION",
    "TEL OPERATOR",
    "NUMBER TYPE",
    "TIME",
    "ACCUMULATED TIMELEN",
    "ACCUMULATED COUNT",
    "ACCUMULATED TRAFFIC",
    "CONCURRENT",
    "SET",
    "AGENT RESOURCE",
    "NUMBER RESOURCE",
    "LINE RESOURCE",
    "LAST"
};

const S8* bs_translate_billing_attr(U32 ulBillingAttr)
{
    if (ulBillingAttr < sizeof(g_pszBillingAttr) / sizeof(S8*))
    {
        return g_pszBillingAttr[ulBillingAttr];
    }
    return "UNKNOWN";
}

const S8* g_pszBillingType [] =
{
    "BY TIMELEN",
    "BY COUNT",
    "BY TRAFFIC",
    "BY CYCLE"
};

const S8* bs_translate_billing_type(U32 ulBillingType)
{
    if (ulBillingType < sizeof(g_pszBillingType) / sizeof(S8*))
    {
        return g_pszBillingType[ulBillingType];
    }
    return "UNKNOWN";
}

const S8* g_pszBsErrNo[] =
{
    "SUCCESS",
    "NOT EXIST",
    "EXPIRE",
    "FROZEN",
    "FEE LACK",
    "PASSWORD ERR",
    "RESTRICT",
    "OVER LIMIT",
    "TIME OUT",
    "LINK DOWN",
    "SYSTEM ERR",
    "MAINTAIN",
    "DATA ABNORMAL",
    "PARAM ERR",
    "NOT MATCH"
};

const S8* bs_translate_bs_errno(U32 ulErrNo)
{
    if (ulErrNo < sizeof(g_pszBsErrNo) / sizeof(S8*))
    {
        return g_pszBsErrNo[ulErrNo];
    }
    return "UNKNOWN";
}

const S8* g_pszInterMsgType[] =
{
    "",
    "WALK REQ",
    "WALK RSP",
    "ORIGINAL CDR",
    "VOICE CDR",
    "RECORDING CDR",
    "MESSAGE CDR",
    "SETTLE CDR",
    "RENT CDR",
    "ACCOUNT CDR",
    "OUTBAND STAT",
    "INBAND STAT",
    "OUTDIALING STAT",
    "MESSAGE STAT",
    "ACCOUNT STAT"
};

const S8* bs_translate_inter_msg_type(U32 ulMsgType)
{
    if (ulMsgType < sizeof(g_pszInterMsgType) / sizeof(S8*))
    {
        return g_pszInterMsgType[ulMsgType];
    }
    return "UNKNOWN";
}

const S8* g_pszObject[] =
{
    "SYSTEM",
    "CUSTOMER",
    "ACCOUNT",
    "AGENT",
    "NUMBER",
    "LINE",
    "TASK"
    "TRUNK"
};

const S8* bs_translate_object(U32 ulObject)
{
    if (ulObject < sizeof(g_pszObject) / sizeof(S8*))
    {
        return g_pszObject[ulObject];
    }
    return "UNKNOWN";
}

S32 bs_show_trace_info(U32 ulIndex)
{
    S8      szBuf[BS_TRACE_BUFF_LEN] = {0, };

    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "FS", "WEB", "DB", "CDR",
                 "BILLING", "ACCOUNT", "STAT", "AUDIT",
                 "MAINTAIN", "RUN", "LEVEL");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10d%10d%10d%10d%10d%10d%10d%10d%10d%10d%10s",
                 (g_stBssCB.ulTraceFlag & BS_TRACE_FS) ? 1:0,
                 (g_stBssCB.ulTraceFlag & BS_TRACE_WEB) ? 1:0,
                 (g_stBssCB.ulTraceFlag & BS_TRACE_DB) ? 1:0,
                 (g_stBssCB.ulTraceFlag & BS_TRACE_CDR)? 1:0,
                 (g_stBssCB.ulTraceFlag & BS_TRACE_BILLING) ? 1:0,
                 (g_stBssCB.ulTraceFlag & BS_TRACE_ACCOUNT) ? 1:0,
                 (g_stBssCB.ulTraceFlag & BS_TRACE_STAT) ? 1:0,
                 (g_stBssCB.ulTraceFlag & BS_TRACE_AUDIT) ? 1:0,
                 (g_stBssCB.ulTraceFlag & BS_TRACE_MAINTAIN) ? 1:0,
                 (g_stBssCB.ulTraceFlag & BS_TRACE_RUN) ? 1:0,
                 bs_trace_level_str(g_stBssCB.ulTraceLevel));
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n");

    return DOS_SUCC;

}


/* 显示BSS控制块*/
S32 bs_show_cb(U32 ulIndex)
{
    time_t  stTime;
    U32     i;
    S8      szIP[64] = {0,}, szDayBase[32] = {0,}, szHourBase[32]= {0,};
    S8      szBuf[BS_TRACE_BUFF_LEN] = {0,};

    stTime = (time_t)g_stBssCB.ulStatDayBase;
    strftime(szDayBase, sizeof(szDayBase), "%Y-%m-%d %H:%M:%S", localtime(&stTime));
    stTime = (time_t)g_stBssCB.ulStatHourBase;
    strftime(szHourBase, sizeof(szHourBase), "%Y-%m-%d %H:%M:%S", localtime(&stTime));

    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%12s%10s%10s%10s%20s%20s%10s%10s",
                 "Maintain", "BillDay", "Hour", "CDRMark",
                 "MaxVoice", "MaxMsg", "StatDay", "StatHour",
                 "TCPPort", "UDPPort");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-------------------------------------------------"
                   "-------------------------------------------------------------------------");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%12u%10u%10u%10u%20s%20s%10u%10u",
                 g_stBssCB.bIsMaintain, g_stBssCB.bIsBillDay,
                 g_stBssCB.ulHour, g_stBssCB.ulCDRMark,
                 g_stBssCB.ulMaxVocTime, g_stBssCB.ulMaxMsNum,
                 szDayBase, szHourBase,
                 dos_ntohs(g_stBssCB.usTCPListenPort),
                 dos_ntohs(g_stBssCB.usUDPListenPort));
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);

    cli_out_string(ulIndex, "\r\n\r\n---------- TRACE INFO ----------\r\n");
    bs_show_trace_info(ulIndex);

    cli_out_string(ulIndex, "\r\n\r\n---------- APP CONN ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%20s%10s",
                 "Connected", "Type", "Seq", "IP", "Port");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n------------------------------------------------------------");
    for (i = 0; i < BS_MAX_APP_CONN_NUM; i++)
    {
        if (!g_stBssCB.astAPPConn[i].bIsValid)
        {
            continue;
        }

        if (g_stBssCB.astAPPConn[i].ucCommType != BSCOMM_PROTO_UNIX)
        {
            dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%20s%10u",
                         g_stBssCB.astAPPConn[i].bIsConn,
                         g_stBssCB.astAPPConn[i].ucCommType,
                         g_stBssCB.astAPPConn[i].ulMsgSeq,
                         dos_ipaddrtostr(g_stBssCB.astAPPConn[i].aulIPAddr[0], szIP, sizeof(szIP)),
                         g_stBssCB.astAPPConn[i].stAddr.stInAddr.sin_port);
            szBuf[sizeof(szBuf) - 1] = '\0';
        }
        else
        {
            dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%20s",
                         g_stBssCB.astAPPConn[i].bIsConn,
                         g_stBssCB.astAPPConn[i].ucCommType,
                         g_stBssCB.astAPPConn[i].ulMsgSeq,
                         g_stBssCB.astAPPConn[i].stAddr.stUnAddr.sun_path);
            szBuf[sizeof(szBuf) - 1] = '\0';

        }
        cli_out_string(ulIndex, szBuf);
    }

    if (g_astCustomerTbl != NULL && g_astBillingPackageTbl != NULL
        && g_astSettleTbl != NULL && g_astSettleTbl != NULL
        && g_astTaskTbl != NULL)
    {
        cli_out_string(ulIndex, "\r\n\r\n---------- HASH TBL ----------\r\n");
        dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s",
                     "Customer", "BillPack", "Settle", "Agent", "Task");
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
        cli_out_string(ulIndex, "\r\n--------------------------------------------------");
        dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u",
                     g_astCustomerTbl->NodeNum,
                     g_astBillingPackageTbl->NodeNum,
                     g_astSettleTbl->NodeNum,
                     g_astAgentTbl->NodeNum,
                     g_astTaskTbl->NodeNum);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- LIST INFO ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s",
                 "S2D", "D2S", "AppSend", "AAA", "CDR", "Billing");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n------------------------------------------------------------");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u",
                 g_stBSS2DMsgList.ulCount,
                 g_stBSD2SMsgList.ulCount,
                 g_stBSAppMsgSendList.ulCount,
                 g_stBSAAAMsgList.ulCount,
                 g_stBSCDRList.ulCount,
                 g_stBSBillingList.ulCount);
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);

    cli_out_string(ulIndex, "\r\n\r\n");

    return DOS_SUCC;
}

/* 显示坐席信息 */
S32 bs_show_agent(U32 ulIndex, U32 ulObjectID)
{
    S8          szBuf[BS_TRACE_BUFF_LEN] = {0, };
    BS_AGENT_ST *pstAgent = NULL;

    pstAgent = bs_get_agent_st(ulObjectID);
    if (NULL == pstAgent)
    {
        cli_out_string(ulIndex, "\r\n Err: Agent is not exist!\r\n");
        return DOS_FAIL;
    }

    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s %10s%10s%10s",
                 "AgentID", "CustomerID", "Group1", "Group2");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-------------------------------------------");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%11u%10u%10u",
                 pstAgent->ulAgentID, pstAgent->ulCustomerID,
                 pstAgent->ulGroup1, pstAgent->ulGroup2);
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);

    cli_out_string(ulIndex, "\r\n\r\n---------- OUTBAND STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Call", "Ring", "Busy",
                 "NotExist", "NoAnswer", "Reject", "EarlyRel",
                 "Answer", "Pdd", "Long");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------");
    if (pstAgent->stStat.astOutBand[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstAgent->stStat.astOutBand[0].ulTimeStamp,
                    pstAgent->stStat.astOutBand[0].ulCallCnt,
                    pstAgent->stStat.astOutBand[0].ulRingCnt,
                    pstAgent->stStat.astOutBand[0].ulBusyCnt,
                    pstAgent->stStat.astOutBand[0].ulNotExistCnt,
                    pstAgent->stStat.astOutBand[0].ulNoAnswerCnt,
                    pstAgent->stStat.astOutBand[0].ulRejectCnt,
                    pstAgent->stStat.astOutBand[0].ulEarlyReleaseCnt,
                    pstAgent->stStat.astOutBand[0].ulAnswerCnt,
                    pstAgent->stStat.astOutBand[0].ulPDD,
                    pstAgent->stStat.astOutBand[0].ulAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstAgent->stStat.astOutBand[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstAgent->stStat.astOutBand[1].ulTimeStamp,
                    pstAgent->stStat.astOutBand[1].ulCallCnt,
                    pstAgent->stStat.astOutBand[1].ulRingCnt,
                    pstAgent->stStat.astOutBand[1].ulBusyCnt,
                    pstAgent->stStat.astOutBand[1].ulNotExistCnt,
                    pstAgent->stStat.astOutBand[1].ulNoAnswerCnt,
                    pstAgent->stStat.astOutBand[1].ulRejectCnt,
                    pstAgent->stStat.astOutBand[1].ulEarlyReleaseCnt,
                    pstAgent->stStat.astOutBand[1].ulAnswerCnt,
                    pstAgent->stStat.astOutBand[1].ulPDD,
                    pstAgent->stStat.astOutBand[1].ulAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- INBAND STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Call", "Ring", "Busy",
                 "NoAnswer", "EarlyRel", "Answer", "ConnAgent",
                 "AgentAswr", "Hold", "Long", "WaitAgTm",
                 "AgAswrTm", "HoldTm");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------"
                   "------------------------------");
    if (pstAgent->stStat.astInBand[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstAgent->stStat.astInBand[0].ulTimeStamp,
                    pstAgent->stStat.astInBand[0].ulCallCnt,
                    pstAgent->stStat.astInBand[0].ulRingCnt,
                    pstAgent->stStat.astInBand[0].ulBusyCnt,
                    pstAgent->stStat.astInBand[0].ulNoAnswerCnt,
                    pstAgent->stStat.astInBand[0].ulEarlyReleaseCnt,
                    pstAgent->stStat.astInBand[0].ulAnswerCnt,
                    pstAgent->stStat.astInBand[0].ulConnAgentCnt,
                    pstAgent->stStat.astInBand[0].ulAgentAnswerCnt,
                    pstAgent->stStat.astInBand[0].ulHoldCnt,
                    pstAgent->stStat.astInBand[0].ulAnswerTime,
                    pstAgent->stStat.astInBand[0].ulWaitAgentTime,
                    pstAgent->stStat.astInBand[0].ulAgentAnswerTime,
                    pstAgent->stStat.astInBand[0].ulHoldTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstAgent->stStat.astInBand[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstAgent->stStat.astInBand[1].ulTimeStamp,
                    pstAgent->stStat.astInBand[1].ulCallCnt,
                    pstAgent->stStat.astInBand[1].ulRingCnt,
                    pstAgent->stStat.astInBand[1].ulBusyCnt,
                    pstAgent->stStat.astInBand[1].ulNoAnswerCnt,
                    pstAgent->stStat.astInBand[1].ulEarlyReleaseCnt,
                    pstAgent->stStat.astInBand[1].ulAnswerCnt,
                    pstAgent->stStat.astInBand[1].ulConnAgentCnt,
                    pstAgent->stStat.astInBand[1].ulAgentAnswerCnt,
                    pstAgent->stStat.astInBand[1].ulHoldCnt,
                    pstAgent->stStat.astInBand[1].ulAnswerTime,
                    pstAgent->stStat.astInBand[1].ulWaitAgentTime,
                    pstAgent->stStat.astInBand[1].ulAgentAnswerTime,
                    pstAgent->stStat.astInBand[1].ulHoldTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- OUTDIALING STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Call", "Ring", "Busy",
                 "NotExist", "NoAnswer", "Reject", "EarlyRel",
                 "Answer", "ConnAgent", "AgentAswr", "Pdd"
                 "Long", "WaitAgTm", "AgAswrTm");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------"
                   "----------------------------------------");
    if (pstAgent->stStat.astOutDialing[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstAgent->stStat.astOutDialing[0].ulTimeStamp,
                    pstAgent->stStat.astOutDialing[0].ulCallCnt,
                    pstAgent->stStat.astOutDialing[0].ulRingCnt,
                    pstAgent->stStat.astOutDialing[0].ulBusyCnt,
                    pstAgent->stStat.astOutDialing[0].ulNotExistCnt,
                    pstAgent->stStat.astOutDialing[0].ulNoAnswerCnt,
                    pstAgent->stStat.astOutDialing[0].ulRejectCnt,
                    pstAgent->stStat.astOutDialing[0].ulEarlyReleaseCnt,
                    pstAgent->stStat.astOutDialing[0].ulAnswerCnt,
                    pstAgent->stStat.astOutDialing[0].ulConnAgentCnt,
                    pstAgent->stStat.astOutDialing[0].ulAgentAnswerCnt,
                    pstAgent->stStat.astOutDialing[0].ulPDD,
                    pstAgent->stStat.astOutDialing[0].ulAnswerTime,
                    pstAgent->stStat.astOutDialing[0].ulWaitAgentTime,
                    pstAgent->stStat.astOutDialing[0].ulAgentAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstAgent->stStat.astOutDialing[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstAgent->stStat.astOutDialing[1].ulTimeStamp,
                    pstAgent->stStat.astOutDialing[1].ulCallCnt,
                    pstAgent->stStat.astOutDialing[1].ulRingCnt,
                    pstAgent->stStat.astOutDialing[1].ulBusyCnt,
                    pstAgent->stStat.astOutDialing[1].ulNotExistCnt,
                    pstAgent->stStat.astOutDialing[1].ulNoAnswerCnt,
                    pstAgent->stStat.astOutDialing[1].ulRejectCnt,
                    pstAgent->stStat.astOutDialing[1].ulEarlyReleaseCnt,
                    pstAgent->stStat.astOutDialing[1].ulAnswerCnt,
                    pstAgent->stStat.astOutDialing[1].ulConnAgentCnt,
                    pstAgent->stStat.astOutDialing[1].ulAgentAnswerCnt,
                    pstAgent->stStat.astOutDialing[1].ulPDD,
                    pstAgent->stStat.astOutDialing[1].ulAnswerTime,
                    pstAgent->stStat.astOutDialing[1].ulWaitAgentTime,
                    pstAgent->stStat.astOutDialing[1].ulAgentAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- MESSAGE STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Send", "Recv", "SndSucc",
                 "SndFail", "SndLen", "RcvLen");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n--------------------------------------------------"
                   "--------------------");
    if (pstAgent->stStat.astMS[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u",
                    pstAgent->stStat.astMS[0].ulTimeStamp,
                    pstAgent->stStat.astMS[0].ulSendCnt,
                    pstAgent->stStat.astMS[0].ulRecvCnt,
                    pstAgent->stStat.astMS[0].ulSendSucc,
                    pstAgent->stStat.astMS[0].ulSendFail,
                    pstAgent->stStat.astMS[0].ulSendLen,
                    pstAgent->stStat.astMS[0].ulRecvLen);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstAgent->stStat.astMS[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u",
                    pstAgent->stStat.astMS[1].ulTimeStamp,
                    pstAgent->stStat.astMS[1].ulSendCnt,
                    pstAgent->stStat.astMS[1].ulRecvCnt,
                    pstAgent->stStat.astMS[1].ulSendSucc,
                    pstAgent->stStat.astMS[1].ulSendFail,
                    pstAgent->stStat.astMS[1].ulSendLen,
                    pstAgent->stStat.astMS[1].ulRecvLen);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n");

    return DOS_SUCC;
}

/* 显示客户信息 */
S32 bs_show_customer(U32 ulIndex, U32 ulObjectID)
{
    S8              szBuf[BS_TRACE_BUFF_LEN] = {0, };
    BS_CUSTOMER_ST  *pstCustomer = NULL;
    BS_ACCOUNT_ST   *pstAccount = NULL;

    pstCustomer = bs_get_customer_st(ulObjectID);
    if (NULL == pstCustomer)
    {
        cli_out_string(ulIndex, "\r\n Err: Customer is not exist!\r\n");
        return DOS_FAIL;
    }
    pstAccount = &pstCustomer->stAccount;

    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%20s%10s%10s%10s%10s%10s%10s%10s",
                 "CustmID", "Name", "Type", "State",
                 "ParentID", "Child", "Agent", "Number", "UserLine");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n--------------------------------------------------"
                   "--------------------------------------------------");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%20s%10s%10s%10u%10u%10u%10u%10u",
                 pstCustomer->ulCustomerID, pstCustomer->szCustomerName,
                 sc_translate_customer_type(pstCustomer->ucCustomerType), bs_translate_customer_state(pstCustomer->ucCustomerState),
                 pstCustomer->ulParentID, pstCustomer->stChildrenList.ulCount,
                 pstCustomer->ulAgentNum, pstCustomer->ulNumberNum,
                 pstCustomer->ulUserLineNum);
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);

    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%14s%14s%16s%16s%14s%14s",
                 "AccID", "CustmID", "PackageID", "CreditLn",
                 "WarnLn", "Balance", "ActBalnc", "Rebate", "AccTime");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n--------------------------------------------------------"
                   "--------------------------------------------------------------");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%14ld%14ld%16ld%16ld%14d%14u",
                 pstAccount->ulAccountID, pstAccount->ulCustomerID,
                 pstAccount->ulBillingPackageID, pstAccount->lCreditLine,
                 pstAccount->lBalanceWarning, pstAccount->LBalance,
                 pstAccount->LBalanceActive, pstAccount->lRebate,
                 pstAccount->ulAccountingTime);
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);

    cli_out_string(ulIndex, "\r\n\r\n---------- OUTBAND STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Call", "Ring", "Busy",
                 "NotExist", "NoAnswer", "Reject", "EarlyRel",
                 "Answer", "Pdd", "Long");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------");
    if (pstCustomer->stStat.astOutBand[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstCustomer->stStat.astOutBand[0].ulTimeStamp,
                    pstCustomer->stStat.astOutBand[0].ulCallCnt,
                    pstCustomer->stStat.astOutBand[0].ulRingCnt,
                    pstCustomer->stStat.astOutBand[0].ulBusyCnt,
                    pstCustomer->stStat.astOutBand[0].ulNotExistCnt,
                    pstCustomer->stStat.astOutBand[0].ulNoAnswerCnt,
                    pstCustomer->stStat.astOutBand[0].ulRejectCnt,
                    pstCustomer->stStat.astOutBand[0].ulEarlyReleaseCnt,
                    pstCustomer->stStat.astOutBand[0].ulAnswerCnt,
                    pstCustomer->stStat.astOutBand[0].ulPDD,
                    pstCustomer->stStat.astOutBand[0].ulAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstCustomer->stStat.astOutBand[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstCustomer->stStat.astOutBand[1].ulTimeStamp,
                    pstCustomer->stStat.astOutBand[1].ulCallCnt,
                    pstCustomer->stStat.astOutBand[1].ulRingCnt,
                    pstCustomer->stStat.astOutBand[1].ulBusyCnt,
                    pstCustomer->stStat.astOutBand[1].ulNotExistCnt,
                    pstCustomer->stStat.astOutBand[1].ulNoAnswerCnt,
                    pstCustomer->stStat.astOutBand[1].ulRejectCnt,
                    pstCustomer->stStat.astOutBand[1].ulEarlyReleaseCnt,
                    pstCustomer->stStat.astOutBand[1].ulAnswerCnt,
                    pstCustomer->stStat.astOutBand[1].ulPDD,
                    pstCustomer->stStat.astOutBand[1].ulAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- INBAND STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Call", "Ring", "Busy",
                 "NoAnswer", "EarlyRel", "Answer", "ConnAgent",
                 "AgentAswr", "Hold", "Long", "WaitAgTm",
                 "AgAswrTm", "HoldTm");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------"
                   "------------------------------");
    if (pstCustomer->stStat.astInBand[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstCustomer->stStat.astInBand[0].ulTimeStamp,
                    pstCustomer->stStat.astInBand[0].ulCallCnt,
                    pstCustomer->stStat.astInBand[0].ulRingCnt,
                    pstCustomer->stStat.astInBand[0].ulBusyCnt,
                    pstCustomer->stStat.astInBand[0].ulNoAnswerCnt,
                    pstCustomer->stStat.astInBand[0].ulEarlyReleaseCnt,
                    pstCustomer->stStat.astInBand[0].ulAnswerCnt,
                    pstCustomer->stStat.astInBand[0].ulConnAgentCnt,
                    pstCustomer->stStat.astInBand[0].ulAgentAnswerCnt,
                    pstCustomer->stStat.astInBand[0].ulHoldCnt,
                    pstCustomer->stStat.astInBand[0].ulAnswerTime,
                    pstCustomer->stStat.astInBand[0].ulWaitAgentTime,
                    pstCustomer->stStat.astInBand[0].ulAgentAnswerTime,
                    pstCustomer->stStat.astInBand[0].ulHoldTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstCustomer->stStat.astInBand[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstCustomer->stStat.astInBand[1].ulTimeStamp,
                    pstCustomer->stStat.astInBand[1].ulCallCnt,
                    pstCustomer->stStat.astInBand[1].ulRingCnt,
                    pstCustomer->stStat.astInBand[1].ulBusyCnt,
                    pstCustomer->stStat.astInBand[1].ulNoAnswerCnt,
                    pstCustomer->stStat.astInBand[1].ulEarlyReleaseCnt,
                    pstCustomer->stStat.astInBand[1].ulAnswerCnt,
                    pstCustomer->stStat.astInBand[1].ulConnAgentCnt,
                    pstCustomer->stStat.astInBand[1].ulAgentAnswerCnt,
                    pstCustomer->stStat.astInBand[1].ulHoldCnt,
                    pstCustomer->stStat.astInBand[1].ulAnswerTime,
                    pstCustomer->stStat.astInBand[1].ulWaitAgentTime,
                    pstCustomer->stStat.astInBand[1].ulAgentAnswerTime,
                    pstCustomer->stStat.astInBand[1].ulHoldTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- OUTDIALING STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Call", "Ring", "Busy",
                 "NotExist", "NoAnswer", "Reject", "EarlyRel",
                 "Answer", "ConnAgent", "AgentAswr", "Pdd",
                 "Long", "WaitAgTm", "AgAswrTm");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------"
                   "----------------------------------------");
    if (pstCustomer->stStat.astOutDialing[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstCustomer->stStat.astOutDialing[0].ulTimeStamp,
                    pstCustomer->stStat.astOutDialing[0].ulCallCnt,
                    pstCustomer->stStat.astOutDialing[0].ulRingCnt,
                    pstCustomer->stStat.astOutDialing[0].ulBusyCnt,
                    pstCustomer->stStat.astOutDialing[0].ulNotExistCnt,
                    pstCustomer->stStat.astOutDialing[0].ulNoAnswerCnt,
                    pstCustomer->stStat.astOutDialing[0].ulRejectCnt,
                    pstCustomer->stStat.astOutDialing[0].ulEarlyReleaseCnt,
                    pstCustomer->stStat.astOutDialing[0].ulAnswerCnt,
                    pstCustomer->stStat.astOutDialing[0].ulConnAgentCnt,
                    pstCustomer->stStat.astOutDialing[0].ulAgentAnswerCnt,
                    pstCustomer->stStat.astOutDialing[0].ulPDD,
                    pstCustomer->stStat.astOutDialing[0].ulAnswerTime,
                    pstCustomer->stStat.astOutDialing[0].ulWaitAgentTime,
                    pstCustomer->stStat.astOutDialing[0].ulAgentAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstCustomer->stStat.astOutDialing[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstCustomer->stStat.astOutDialing[1].ulTimeStamp,
                    pstCustomer->stStat.astOutDialing[1].ulCallCnt,
                    pstCustomer->stStat.astOutDialing[1].ulRingCnt,
                    pstCustomer->stStat.astOutDialing[1].ulBusyCnt,
                    pstCustomer->stStat.astOutDialing[1].ulNotExistCnt,
                    pstCustomer->stStat.astOutDialing[1].ulNoAnswerCnt,
                    pstCustomer->stStat.astOutDialing[1].ulRejectCnt,
                    pstCustomer->stStat.astOutDialing[1].ulEarlyReleaseCnt,
                    pstCustomer->stStat.astOutDialing[1].ulAnswerCnt,
                    pstCustomer->stStat.astOutDialing[1].ulConnAgentCnt,
                    pstCustomer->stStat.astOutDialing[1].ulAgentAnswerCnt,
                    pstCustomer->stStat.astOutDialing[1].ulPDD,
                    pstCustomer->stStat.astOutDialing[1].ulAnswerTime,
                    pstCustomer->stStat.astOutDialing[1].ulWaitAgentTime,
                    pstCustomer->stStat.astOutDialing[1].ulAgentAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- MESSAGE STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Send", "Recv", "SndSucc",
                 "SndFail", "SndLen", "RcvLen");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n--------------------------------------------------"
                   "--------------------");
    if (pstCustomer->stStat.astMS[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u",
                    pstCustomer->stStat.astMS[0].ulTimeStamp,
                    pstCustomer->stStat.astMS[0].ulSendCnt,
                    pstCustomer->stStat.astMS[0].ulRecvCnt,
                    pstCustomer->stStat.astMS[0].ulSendSucc,
                    pstCustomer->stStat.astMS[0].ulSendFail,
                    pstCustomer->stStat.astMS[0].ulSendLen,
                    pstCustomer->stStat.astMS[0].ulRecvLen);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstCustomer->stStat.astMS[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u",
                    pstCustomer->stStat.astMS[1].ulTimeStamp,
                    pstCustomer->stStat.astMS[1].ulSendCnt,
                    pstCustomer->stStat.astMS[1].ulRecvCnt,
                    pstCustomer->stStat.astMS[1].ulSendSucc,
                    pstCustomer->stStat.astMS[1].ulSendFail,
                    pstCustomer->stStat.astMS[1].ulSendLen,
                    pstCustomer->stStat.astMS[1].ulRecvLen);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- ACCOUNT STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Profit", "OutBndFee", "InBndFee",
                 "AtDialFee", "PrevwFee", "PredtFee", "RecordFee",
                 "ConferFee", "SmsFee", "MmsFee", "RentFee");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n--------------------------------------------------"
                   "----------------------------------------------------------------------");
    if (pstAccount->stStat.ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10d%10d%10d%10d%10d%10d%10d%10d%10d%10d%10d",
                    pstAccount->stStat.ulTimeStamp,
                    pstAccount->stStat.lProfit,
                    pstAccount->stStat.lOutBandCallFee,
                    pstAccount->stStat.lInBandCallFee,
                    pstAccount->stStat.lAutoDialingFee,
                    pstAccount->stStat.lPreviewDailingFee,
                    pstAccount->stStat.lPredictiveDailingFee,
                    pstAccount->stStat.lRecordFee,
                    pstAccount->stStat.lConferenceFee,
                    pstAccount->stStat.lSmsFee,
                    pstAccount->stStat.lMmsFee,
                    pstAccount->stStat.lRentFee);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n");

    return DOS_SUCC;

}

/* 显示任务信息 */
S32 bs_show_task(U32 ulIndex, U32 ulObjectID)
{
    S8          szBuf[BS_TRACE_BUFF_LEN] = {0, };
    BS_TASK_ST  *pstTask = NULL;

    pstTask = bs_get_task_st(ulObjectID);
    if (NULL == pstTask)
    {
        cli_out_string(ulIndex, "\r\nErr: Task is not exist!\r\n");
        return DOS_FAIL;
    }

    cli_out_string(ulIndex, "\r\n---------- OUTBAND STAT ----------");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Call", "Ring", "Busy",
                 "NotExist", "NoAnswer", "Reject", "EarlyRel",
                 "Answer", "Pdd", "Long");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------");
    if (pstTask->stStat.astOutBand[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstTask->stStat.astOutBand[0].ulTimeStamp,
                    pstTask->stStat.astOutBand[0].ulCallCnt,
                    pstTask->stStat.astOutBand[0].ulRingCnt,
                    pstTask->stStat.astOutBand[0].ulBusyCnt,
                    pstTask->stStat.astOutBand[0].ulNotExistCnt,
                    pstTask->stStat.astOutBand[0].ulNoAnswerCnt,
                    pstTask->stStat.astOutBand[0].ulRejectCnt,
                    pstTask->stStat.astOutBand[0].ulEarlyReleaseCnt,
                    pstTask->stStat.astOutBand[0].ulAnswerCnt,
                    pstTask->stStat.astOutBand[0].ulPDD,
                    pstTask->stStat.astOutBand[0].ulAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstTask->stStat.astOutBand[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstTask->stStat.astOutBand[1].ulTimeStamp,
                    pstTask->stStat.astOutBand[1].ulCallCnt,
                    pstTask->stStat.astOutBand[1].ulRingCnt,
                    pstTask->stStat.astOutBand[1].ulBusyCnt,
                    pstTask->stStat.astOutBand[1].ulNotExistCnt,
                    pstTask->stStat.astOutBand[1].ulNoAnswerCnt,
                    pstTask->stStat.astOutBand[1].ulRejectCnt,
                    pstTask->stStat.astOutBand[1].ulEarlyReleaseCnt,
                    pstTask->stStat.astOutBand[1].ulAnswerCnt,
                    pstTask->stStat.astOutBand[1].ulPDD,
                    pstTask->stStat.astOutBand[1].ulAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- INBAND STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Call", "Ring", "Busy",
                 "NoAnswer", "EarlyRel", "Answer", "ConnAgent",
                 "AgentAswr", "Hold", "Long", "WaitAgTm",
                 "AgAswrTm", "HoldTm");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------"
                   "------------------------------");
    if (pstTask->stStat.astInBand[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstTask->stStat.astInBand[0].ulTimeStamp,
                    pstTask->stStat.astInBand[0].ulCallCnt,
                    pstTask->stStat.astInBand[0].ulRingCnt,
                    pstTask->stStat.astInBand[0].ulBusyCnt,
                    pstTask->stStat.astInBand[0].ulNoAnswerCnt,
                    pstTask->stStat.astInBand[0].ulEarlyReleaseCnt,
                    pstTask->stStat.astInBand[0].ulAnswerCnt,
                    pstTask->stStat.astInBand[0].ulConnAgentCnt,
                    pstTask->stStat.astInBand[0].ulAgentAnswerCnt,
                    pstTask->stStat.astInBand[0].ulHoldCnt,
                    pstTask->stStat.astInBand[0].ulAnswerTime,
                    pstTask->stStat.astInBand[0].ulWaitAgentTime,
                    pstTask->stStat.astInBand[0].ulAgentAnswerTime,
                    pstTask->stStat.astInBand[0].ulHoldTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstTask->stStat.astInBand[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstTask->stStat.astInBand[1].ulTimeStamp,
                    pstTask->stStat.astInBand[1].ulCallCnt,
                    pstTask->stStat.astInBand[1].ulRingCnt,
                    pstTask->stStat.astInBand[1].ulBusyCnt,
                    pstTask->stStat.astInBand[1].ulNoAnswerCnt,
                    pstTask->stStat.astInBand[1].ulEarlyReleaseCnt,
                    pstTask->stStat.astInBand[1].ulAnswerCnt,
                    pstTask->stStat.astInBand[1].ulConnAgentCnt,
                    pstTask->stStat.astInBand[1].ulAgentAnswerCnt,
                    pstTask->stStat.astInBand[1].ulHoldCnt,
                    pstTask->stStat.astInBand[1].ulAnswerTime,
                    pstTask->stStat.astInBand[1].ulWaitAgentTime,
                    pstTask->stStat.astInBand[1].ulAgentAnswerTime,
                    pstTask->stStat.astInBand[1].ulHoldTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- OUTDIALING STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Call", "Ring", "Busy",
                 "NotExist", "NoAnswer", "Reject", "EarlyRel",
                 "Answer", "ConnAgent", "AgentAswr", "Pdd"
                 "Long", "WaitAgTm", "AgAswrTm");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------"
                   "----------------------------------------");
    if (pstTask->stStat.astOutDialing[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstTask->stStat.astOutDialing[0].ulTimeStamp,
                    pstTask->stStat.astOutDialing[0].ulCallCnt,
                    pstTask->stStat.astOutDialing[0].ulRingCnt,
                    pstTask->stStat.astOutDialing[0].ulBusyCnt,
                    pstTask->stStat.astOutDialing[0].ulNotExistCnt,
                    pstTask->stStat.astOutDialing[0].ulNoAnswerCnt,
                    pstTask->stStat.astOutDialing[0].ulRejectCnt,
                    pstTask->stStat.astOutDialing[0].ulEarlyReleaseCnt,
                    pstTask->stStat.astOutDialing[0].ulAnswerCnt,
                    pstTask->stStat.astOutDialing[0].ulConnAgentCnt,
                    pstTask->stStat.astOutDialing[0].ulAgentAnswerCnt,
                    pstTask->stStat.astOutDialing[0].ulPDD,
                    pstTask->stStat.astOutDialing[0].ulAnswerTime,
                    pstTask->stStat.astOutDialing[0].ulWaitAgentTime,
                    pstTask->stStat.astOutDialing[0].ulAgentAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstTask->stStat.astOutDialing[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstTask->stStat.astOutDialing[1].ulTimeStamp,
                    pstTask->stStat.astOutDialing[1].ulCallCnt,
                    pstTask->stStat.astOutDialing[1].ulRingCnt,
                    pstTask->stStat.astOutDialing[1].ulBusyCnt,
                    pstTask->stStat.astOutDialing[1].ulNotExistCnt,
                    pstTask->stStat.astOutDialing[1].ulNoAnswerCnt,
                    pstTask->stStat.astOutDialing[1].ulRejectCnt,
                    pstTask->stStat.astOutDialing[1].ulEarlyReleaseCnt,
                    pstTask->stStat.astOutDialing[1].ulAnswerCnt,
                    pstTask->stStat.astOutDialing[1].ulConnAgentCnt,
                    pstTask->stStat.astOutDialing[1].ulAgentAnswerCnt,
                    pstTask->stStat.astOutDialing[1].ulPDD,
                    pstTask->stStat.astOutDialing[1].ulAnswerTime,
                    pstTask->stStat.astOutDialing[1].ulWaitAgentTime,
                    pstTask->stStat.astOutDialing[1].ulAgentAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- MESSAGE STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Send", "Recv", "SndSucc",
                 "SndFail", "SndLen", "RcvLen");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n--------------------------------------------------"
                   "--------------------");
    if (pstTask->stStat.astMS[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u",
                    pstTask->stStat.astMS[0].ulTimeStamp,
                    pstTask->stStat.astMS[0].ulSendCnt,
                    pstTask->stStat.astMS[0].ulRecvCnt,
                    pstTask->stStat.astMS[0].ulSendSucc,
                    pstTask->stStat.astMS[0].ulSendFail,
                    pstTask->stStat.astMS[0].ulSendLen,
                    pstTask->stStat.astMS[0].ulRecvLen);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstTask->stStat.astMS[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u",
                    pstTask->stStat.astMS[1].ulTimeStamp,
                    pstTask->stStat.astMS[1].ulSendCnt,
                    pstTask->stStat.astMS[1].ulRecvCnt,
                    pstTask->stStat.astMS[1].ulSendSucc,
                    pstTask->stStat.astMS[1].ulSendFail,
                    pstTask->stStat.astMS[1].ulSendLen,
                    pstTask->stStat.astMS[1].ulRecvLen);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n");

    return DOS_SUCC;
}

S32 bs_show_billing_package(U32 ulIndex, U32 ulObjectID)
{
    S8 szBuff[1024] = {0};
    HASH_NODE_S *pstHashNode = NULL;
    U32 ulHashIndex = 0;

    BS_BILLING_PACKAGE_ST  *pstPkg = NULL;
    U32 ulLoop = 0;

    HASH_Scan_Table(g_astBillingPackageTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astBillingPackageTbl,ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstPkg = (BS_BILLING_PACKAGE_ST *)pstHashNode->pHandle;
            if (pstPkg->ulPackageID != ulObjectID)
            {
                continue;
            }

            /* 打印表头 */
            dos_snprintf(szBuff, sizeof(szBuff), "\r\nList Package ID %d.", ulObjectID);
            cli_out_string(ulIndex, szBuff);
            cli_out_string(ulIndex, "\r\n---------------------------------------");
            cli_out_string(ulIndex, "\r\n  Package ID        Service Type");

            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%12u%20s\r\n"
                            , ulObjectID
                            , bs_translate_serv_type(pstPkg->ucServType));
            cli_out_string(ulIndex, szBuff);

            for (ulLoop = 0; ulLoop < BS_MAX_BILLING_RULE_IN_PACKAGE; ++ulLoop)
            {
                if (0 == pstPkg->astRule[ulLoop].ulRuleID)
                {
                    continue;
                }
                /* 打印资费包属性和值 */
                cli_out_string(ulIndex, "\r\n--------------------------------------------------------------------------------------------------------------------------------------------------");
                cli_out_string(ulIndex, "\r\nRule ID        SrcAttrType1 SrcAttrValue1        SrcAttrType2 SrcAttrValue2       DestAttrType1 DestAttrValue1       DestAttrType2 DestAttrValue2");

                dos_snprintf(szBuff, sizeof(szBuff), "\r\n%7u%20s%14u%20s%14u%20s%15u%20s%15u\r\n"
                                , pstPkg->astRule[ulLoop].ulRuleID
                                , bs_translate_billing_attr(pstPkg->astRule[ulLoop].ucSrcAttrType1)
                                , pstPkg->astRule[ulLoop].ulSrcAttrValue1
                                , bs_translate_billing_attr(pstPkg->astRule[ulLoop].ucSrcAttrType2)
                                , pstPkg->astRule[ulLoop].ulSrcAttrValue2
                                , bs_translate_billing_attr(pstPkg->astRule[ulLoop].ucDstAttrType1)
                                , pstPkg->astRule[ulLoop].ulDstAttrValue1
                                , bs_translate_billing_attr(pstPkg->astRule[ulLoop].ucDstAttrType2)
                                , pstPkg->astRule[ulLoop].ulDstAttrValue2);
                cli_out_string(ulIndex, szBuff);
            }

            dos_snprintf(szBuff, sizeof(szBuff), "\r\nList the Billing Rule");
            cli_out_string(ulIndex, szBuff);

            dos_snprintf(szBuff, sizeof(szBuff), "\r\n-----------------------------------------------------------------------------------------------------------------------------------------------------------");
            cli_out_string(ulIndex, szBuff);

            dos_snprintf(szBuff, sizeof(szBuff), "\r\n  %-20s|%12u %12u %12u %12u %12u %12u %12u %12u %12u %12u", "Billing Rule Record", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
            cli_out_string(ulIndex, szBuff);

            dos_snprintf(szBuff, sizeof(szBuff), "\r\n-----------------------------------------------------------------------------------------------------------------------------------------------------------");
            cli_out_string(ulIndex, szBuff);

            cli_out_string(ulIndex, "\r\nFirstBillingUnit FirstBillingCnt NextBillingUnit NextBillingCnt            ServType    BillingType BillingRate       Effect       Expire Priority Valid");

            dos_snprintf(szBuff, sizeof(szBuff), "\r\n%16u%16u%16u%15u%20s%15s%12u%13u%13u%9u%6s\r\n\r\n\r\n"
                            , pstPkg->astRule[ulLoop].ulFirstBillingUnit
                            , pstPkg->astRule[ulLoop].ucFirstBillingCnt
                            , pstPkg->astRule[ulLoop].ulNextBillingUnit
                            , pstPkg->astRule[ulLoop].ucNextBillingCnt
                            , bs_translate_serv_type(pstPkg->astRule[ulLoop].ucServType)
                            , bs_translate_billing_type(pstPkg->astRule[ulLoop].ucBillingType)
                            , pstPkg->astRule[ulLoop].ulBillingRate
                            , pstPkg->astRule[ulLoop].ulEffectTimestamp
                            , pstPkg->astRule[ulLoop].ulExpireTimestamp
                            , pstPkg->astRule[ulLoop].ucPriority
                            , pstPkg->astRule[ulLoop].ucValid == 1 ? "Y" : "N");
            cli_out_string(ulIndex, szBuff);
        }
    }
    return DOS_SUCC;
}

/* 显示中继信息 */
S32 bs_show_trunk(U32 ulIndex, U32 ulObjectID)
{
    S8              szBuf[BS_TRACE_BUFF_LEN] = {0,};
    BS_SETTLE_ST    *pstSettle = NULL;

    pstSettle = bs_get_settle_st(ulObjectID);
    if (NULL == pstSettle)
    {
        cli_out_string(ulIndex, "\r\n Err: Trunk is not exist!\r\n");
        return DOS_FAIL;
    }

    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s",
                 "TrunkID", "SPID", "PackageID", "Settle");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n----------------------------------------");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u",
                 pstSettle->usTrunkID, pstSettle->ulSPID,
                 pstSettle->ulPackageID, pstSettle->ucSettleFlag);
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);

    cli_out_string(ulIndex, "\r\n\r\n---------- OUTBAND STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Call", "Ring", "Busy",
                 "NotExist", "NoAnswer", "Reject", "EarlyRel",
                 "Answer", "Pdd", "Long");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------");
    if (pstSettle->stStat.astOutBand[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstSettle->stStat.astOutBand[0].ulTimeStamp,
                    pstSettle->stStat.astOutBand[0].ulCallCnt,
                    pstSettle->stStat.astOutBand[0].ulRingCnt,
                    pstSettle->stStat.astOutBand[0].ulBusyCnt,
                    pstSettle->stStat.astOutBand[0].ulNotExistCnt,
                    pstSettle->stStat.astOutBand[0].ulNoAnswerCnt,
                    pstSettle->stStat.astOutBand[0].ulRejectCnt,
                    pstSettle->stStat.astOutBand[0].ulEarlyReleaseCnt,
                    pstSettle->stStat.astOutBand[0].ulAnswerCnt,
                    pstSettle->stStat.astOutBand[0].ulPDD,
                    pstSettle->stStat.astOutBand[0].ulAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstSettle->stStat.astOutBand[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstSettle->stStat.astOutBand[1].ulTimeStamp,
                    pstSettle->stStat.astOutBand[1].ulCallCnt,
                    pstSettle->stStat.astOutBand[1].ulRingCnt,
                    pstSettle->stStat.astOutBand[1].ulBusyCnt,
                    pstSettle->stStat.astOutBand[1].ulNotExistCnt,
                    pstSettle->stStat.astOutBand[1].ulNoAnswerCnt,
                    pstSettle->stStat.astOutBand[1].ulRejectCnt,
                    pstSettle->stStat.astOutBand[1].ulEarlyReleaseCnt,
                    pstSettle->stStat.astOutBand[1].ulAnswerCnt,
                    pstSettle->stStat.astOutBand[1].ulPDD,
                    pstSettle->stStat.astOutBand[1].ulAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- INBAND STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Call", "Ring", "Busy",
                 "NoAnswer", "EarlyRel", "Answer", "ConnAgent",
                 "AgentAswr", "Hold", "Long", "WaitAgTm",
                 "AgAswrTm", "HoldTm");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------"
                   "------------------------------");
    if (pstSettle->stStat.astInBand[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstSettle->stStat.astInBand[0].ulTimeStamp,
                    pstSettle->stStat.astInBand[0].ulCallCnt,
                    pstSettle->stStat.astInBand[0].ulRingCnt,
                    pstSettle->stStat.astInBand[0].ulBusyCnt,
                    pstSettle->stStat.astInBand[0].ulNoAnswerCnt,
                    pstSettle->stStat.astInBand[0].ulEarlyReleaseCnt,
                    pstSettle->stStat.astInBand[0].ulAnswerCnt,
                    pstSettle->stStat.astInBand[0].ulConnAgentCnt,
                    pstSettle->stStat.astInBand[0].ulAgentAnswerCnt,
                    pstSettle->stStat.astInBand[0].ulHoldCnt,
                    pstSettle->stStat.astInBand[0].ulAnswerTime,
                    pstSettle->stStat.astInBand[0].ulWaitAgentTime,
                    pstSettle->stStat.astInBand[0].ulAgentAnswerTime,
                    pstSettle->stStat.astInBand[0].ulHoldTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstSettle->stStat.astInBand[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstSettle->stStat.astInBand[1].ulTimeStamp,
                    pstSettle->stStat.astInBand[1].ulCallCnt,
                    pstSettle->stStat.astInBand[1].ulRingCnt,
                    pstSettle->stStat.astInBand[1].ulBusyCnt,
                    pstSettle->stStat.astInBand[1].ulNoAnswerCnt,
                    pstSettle->stStat.astInBand[1].ulEarlyReleaseCnt,
                    pstSettle->stStat.astInBand[1].ulAnswerCnt,
                    pstSettle->stStat.astInBand[1].ulConnAgentCnt,
                    pstSettle->stStat.astInBand[1].ulAgentAnswerCnt,
                    pstSettle->stStat.astInBand[1].ulHoldCnt,
                    pstSettle->stStat.astInBand[1].ulAnswerTime,
                    pstSettle->stStat.astInBand[1].ulWaitAgentTime,
                    pstSettle->stStat.astInBand[1].ulAgentAnswerTime,
                    pstSettle->stStat.astInBand[1].ulHoldTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- OUTDIALING STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Call", "Ring", "Busy",
                 "NotExist", "NoAnswer", "Reject", "EarlyRel",
                 "Answer", "ConnAgent", "AgentAswr", "Pdd"
                 "Long", "WaitAgTm", "AgAswrTm");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n-----------------------------------------------"
                   "---------------------------------------------------------------"
                   "----------------------------------------");
    if (pstSettle->stStat.astOutDialing[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstSettle->stStat.astOutDialing[0].ulTimeStamp,
                    pstSettle->stStat.astOutDialing[0].ulCallCnt,
                    pstSettle->stStat.astOutDialing[0].ulRingCnt,
                    pstSettle->stStat.astOutDialing[0].ulBusyCnt,
                    pstSettle->stStat.astOutDialing[0].ulNotExistCnt,
                    pstSettle->stStat.astOutDialing[0].ulNoAnswerCnt,
                    pstSettle->stStat.astOutDialing[0].ulRejectCnt,
                    pstSettle->stStat.astOutDialing[0].ulEarlyReleaseCnt,
                    pstSettle->stStat.astOutDialing[0].ulAnswerCnt,
                    pstSettle->stStat.astOutDialing[0].ulConnAgentCnt,
                    pstSettle->stStat.astOutDialing[0].ulAgentAnswerCnt,
                    pstSettle->stStat.astOutDialing[0].ulPDD,
                    pstSettle->stStat.astOutDialing[0].ulAnswerTime,
                    pstSettle->stStat.astOutDialing[0].ulWaitAgentTime,
                    pstSettle->stStat.astOutDialing[0].ulAgentAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstSettle->stStat.astOutDialing[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u%10u",
                    pstSettle->stStat.astOutDialing[1].ulTimeStamp,
                    pstSettle->stStat.astOutDialing[1].ulCallCnt,
                    pstSettle->stStat.astOutDialing[1].ulRingCnt,
                    pstSettle->stStat.astOutDialing[1].ulBusyCnt,
                    pstSettle->stStat.astOutDialing[1].ulNotExistCnt,
                    pstSettle->stStat.astOutDialing[1].ulNoAnswerCnt,
                    pstSettle->stStat.astOutDialing[1].ulRejectCnt,
                    pstSettle->stStat.astOutDialing[1].ulEarlyReleaseCnt,
                    pstSettle->stStat.astOutDialing[1].ulAnswerCnt,
                    pstSettle->stStat.astOutDialing[1].ulConnAgentCnt,
                    pstSettle->stStat.astOutDialing[1].ulAgentAnswerCnt,
                    pstSettle->stStat.astOutDialing[1].ulPDD,
                    pstSettle->stStat.astOutDialing[1].ulAnswerTime,
                    pstSettle->stStat.astOutDialing[1].ulWaitAgentTime,
                    pstSettle->stStat.astOutDialing[1].ulAgentAnswerTime);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n---------- MESSAGE STAT ----------\r\n");
    dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10s%10s%10s%10s%10s%10s%10s",
                 "TS", "Send", "Recv", "SndSucc",
                 "SndFail", "SndLen", "RcvLen");
    szBuf[sizeof(szBuf) - 1] = '\0';
    cli_out_string(ulIndex, szBuf);
    cli_out_string(ulIndex, "\r\n--------------------------------------------------"
                   "--------------------");
    if (pstSettle->stStat.astMS[0].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u",
                    pstSettle->stStat.astMS[0].ulTimeStamp,
                    pstSettle->stStat.astMS[0].ulSendCnt,
                    pstSettle->stStat.astMS[0].ulRecvCnt,
                    pstSettle->stStat.astMS[0].ulSendSucc,
                    pstSettle->stStat.astMS[0].ulSendFail,
                    pstSettle->stStat.astMS[0].ulSendLen,
                    pstSettle->stStat.astMS[0].ulRecvLen);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    if (pstSettle->stStat.astMS[1].ulTimeStamp != 0)
    {
       dos_snprintf(szBuf, sizeof(szBuf), "\r\n%10u%10u%10u%10u%10u%10u%10u",
                    pstSettle->stStat.astMS[1].ulTimeStamp,
                    pstSettle->stStat.astMS[1].ulSendCnt,
                    pstSettle->stStat.astMS[1].ulRecvCnt,
                    pstSettle->stStat.astMS[1].ulSendSucc,
                    pstSettle->stStat.astMS[1].ulSendFail,
                    pstSettle->stStat.astMS[1].ulSendLen,
                    pstSettle->stStat.astMS[1].ulRecvLen);
        szBuf[sizeof(szBuf) - 1] = '\0';
        cli_out_string(ulIndex, szBuf);
    }

    cli_out_string(ulIndex, "\r\n\r\n");

    return DOS_SUCC;

}


S32 bs_show_outband_stat(U32 ulIndex)
{
    DLL_NODE_S * pNode = NULL;
    S8  szBuff[1024] = {0};
    BS_INTER_MSG_STAT   *pstMsg = NULL;
    BS_STAT_OUTBAND_ST  *pstStat = NULL;

    DLL_Scan(&g_stBSS2DMsgList, pNode, DLL_NODE_S *)
    {
        if (DOS_ADDR_INVALID(pNode->pHandle))
        {
            continue;
        }

        pstMsg = (BS_INTER_MSG_STAT *)pNode->pHandle;
        if (BS_INTER_MSG_OUTBAND_STAT != pstMsg->stMsgTag.ucMsgType)
        {
            continue;
        }

        pstStat = (BS_STAT_OUTBAND_ST  *)pstMsg->pStat;
        if (DOS_ADDR_INVALID(pstStat))
        {
            continue;
        }

        dos_snprintf(szBuff, sizeof(szBuff), "\r\nList outband stat...\r\n");
        cli_out_string(ulIndex, szBuff);

        dos_snprintf(szBuff, sizeof(szBuff), "\r\nObject ID:%u\r\nObject Type:%s\r\nAnswer Cnt:%u\r\n"
                        "Answer Time:%u\r\nBusy Cnt:%u\r\nCall Cnt:%u\r\nEarly Release Cnt:%u\r\n"
                        "No Answer Cnt:%u\r\nNo Exist Cnt:%u\r\nPDD:%u\r\nReject Cnt:%u\r\n"
                        "Ring Cnt:%u\r\nTime Stamp:%u\r\n"
                        , pstStat->stStatTag.ulObjectID
                        , bs_translate_object(pstStat->stStatTag.ucObjectType)
                        , pstStat->stOutBand.ulAnswerCnt
                        , pstStat->stOutBand.ulAnswerTime
                        , pstStat->stOutBand.ulBusyCnt
                        , pstStat->stOutBand.ulCallCnt
                        , pstStat->stOutBand.ulEarlyReleaseCnt
                        , pstStat->stOutBand.ulNoAnswerCnt
                        , pstStat->stOutBand.ulNotExistCnt
                        , pstStat->stOutBand.ulPDD
                        , pstStat->stOutBand.ulRejectCnt
                        , pstStat->stOutBand.ulRingCnt
                        , pstStat->stOutBand.ulTimeStamp
                    );
         cli_out_string(ulIndex, szBuff);
    }

    return DOS_SUCC;
}


/* 命令行处理函数:以后SDK会将debug和show统一管理,介时须作响应调整 */
S32 bs_command_proc(U32 ulIndex, S32 argc, S8 **argv)
{
    S32 i;
    U32 ulModule, ulLevel, ulObjectID;
    /* 测试代码 */
#if INCLUDE_BH_CLIENT
    MON_NOTIFY_MSG_ST stMsg;
#endif
    /* 测试代码 */

    for (i = 1; i < argc && i < 3; i++)
    {
        dos_lowercase(argv[i]);
    }

    if (0 == dos_strncmp(argv[1], "show", dos_strlen(argv[1])))
    {
        if (0 == dos_strncmp(argv[2], "cb", dos_strlen(argv[2])))
        {
            return bs_show_cb(ulIndex);
        }
        else if (0 == dos_strncmp(argv[2], "outband", dos_strlen(argv[2])))
        {
            return bs_show_outband_stat(ulIndex);
        }

        if (argc != 4 || dos_atoul(argv[3], &ulObjectID) != 0)
        {
            goto help;
        }

        if (0 == dos_strncmp(argv[2], "agent", dos_strlen(argv[2])))
        {
            return bs_show_agent(ulIndex, ulObjectID);
        }
        else if (0 == dos_strncmp(argv[2], "customer", dos_strlen(argv[2])))
        {
            return bs_show_customer(ulIndex, ulObjectID);
        }
        else if (0 == dos_strncmp(argv[2], "task", dos_strlen(argv[2])))
        {
            return bs_show_task(ulIndex, ulObjectID);
        }
        else if (0 == dos_strncmp(argv[2], "trunk", dos_strlen(argv[2])))
        {
            return bs_show_trunk(ulIndex, ulObjectID);
        }
        else if (0 == dos_strncmp(argv[2], "billing", dos_strlen(argv[2])))
        {
            return bs_show_billing_package(ulIndex, ulObjectID);
        }
        else
        {
            goto help;
        }
    }

#if INCLUDE_BH_CLIENT
    /* 本段代码用来测试发送告警消息，可删除 */
    else if (dos_strncmp(argv[1], "email", dos_strlen(argv[1])) == 0)
    {
        stMsg.ulCurTime = time(0);
        stMsg.ulCustomerID = 1;
        stMsg.ulRoleID= 3;
        stMsg.ulLevel = MON_NOTIFY_LEVEL_CRITI;
        stMsg.ulWarningID = MON_NOTIFY_TYPE_LACK_FEE | 0x07000000;
        dos_snprintf(stMsg.stContact.szEmail, sizeof(stMsg.stContact.szEmail), "%s", "2381823710@qq.com");
        dos_snprintf(stMsg.stContact.szTelNo, sizeof(stMsg.stContact.szTelNo), "%s", "18700944432");
        dos_snprintf(stMsg.szContent, sizeof(stMsg.szContent), "%s", "700000");
        dos_snprintf(stMsg.szNotes, sizeof(stMsg.szNotes), "%s", "余额已不足");
        hb_send_external_warning(g_pstDebugProcInfo, &stMsg);
    }
#endif

    else if (dos_strncmp(argv[1], "debug", dos_strlen(argv[1])) != 0)
    {
        goto help;
    }

    ulLevel = 0;
    ulModule = 0;

    if (0 == dos_strcmp(argv[2], "off"))
    {
        ulModule = 0;
    }
    else if (argc != 4
             || dos_atoul(argv[3], &ulLevel) != 0
             || ulLevel >= LOG_LEVEL_INVAILD)
    {
        goto help;
    }

    if (0 == dos_strcmp(argv[2], "all"))
    {
        ulModule = BS_TRACE_ALL;
    }
    else if (0 == dos_strncmp(argv[2], "account", dos_strlen(argv[2])))
    {
        ulModule = BS_TRACE_ACCOUNT;
    }
    else if (0 == dos_strncmp(argv[2], "audit", dos_strlen(argv[2])))
    {
        ulModule = BS_TRACE_AUDIT;
    }
    else if (0 == dos_strncmp(argv[2], "bill", dos_strlen(argv[2])))
    {
        ulModule = BS_TRACE_BILLING;
    }
    else if (0 == dos_strncmp(argv[2], "cdr", dos_strlen(argv[2])))
    {
        ulModule = BS_TRACE_CDR;
    }
    else if (0 == dos_strncmp(argv[2], "db", dos_strlen(argv[2])))
    {
        ulModule = BS_TRACE_DB;
    }
    else if (0 == dos_strncmp(argv[2], "fs", dos_strlen(argv[2])))
    {
        ulModule = BS_TRACE_FS;
    }
    else if (0 == dos_strncmp(argv[2], "maintain", dos_strlen(argv[2])))
    {
        ulModule = BS_TRACE_MAINTAIN;
    }
    else if (0 == dos_strncmp(argv[2], "run", dos_strlen(argv[2])))
    {
        ulModule = BS_TRACE_RUN;
    }
    else if (0 == dos_strncmp(argv[2], "stat", dos_strlen(argv[2])))
    {
        ulModule = BS_TRACE_STAT;
    }
    else if (0 == dos_strncmp(argv[2], "web", dos_strlen(argv[2])))
    {
        ulModule = BS_TRACE_WEB;
    }
    else
    {
        goto help;
    }

    g_stBssCB.ulTraceLevel = ulLevel;
    if (0 == ulModule)
    {
        g_stBssCB.ulTraceFlag = 0;
    }
    else
    {
        g_stBssCB.ulTraceFlag = g_stBssCB.ulTraceFlag|ulModule;
    }

    return bs_show_trace_info(ulIndex);

help:
    cli_out_string(ulIndex, "\r\nInvalid parameters");
    cli_out_string(ulIndex, "\r\nUsage:");
    cli_out_string(ulIndex, "\r\n1. bs show <object:agent|cb|customer|task|trunk|billing|outband> <object id>");
    cli_out_string(ulIndex, "\r\n2. bs debug <module:all|off|account|audit|bill|cdr|db|fs|maintain|run|stat|web> <level:0|1|2|3|4|5|6|7>");

    cli_out_string(ulIndex, "\r\n\r\n");

    return DOS_FAIL;
}


/* 跟踪函数 */
VOID bs_trace(U32 ulTraceTarget, U8 ucTraceLevel, const S8 * szFormat, ...)
{
    va_list         Arg;
    U32             ulTraceTagLen;
    S8              szTraceStr[BS_TRACE_BUFF_LEN];
    BOOL            bIsOutput = DOS_FALSE;

    if (ucTraceLevel >= LOG_LEVEL_INVAILD)
    {
        return;
    }

    /* warning级别以上强制输出 */
    if (ucTraceLevel <= LOG_LEVEL_WARNING)
    {
        bIsOutput = DOS_TRUE;
    }

    if (!bIsOutput
        && ulTraceTarget&g_stBssCB.ulTraceFlag
        && g_stBssCB.ulTraceLevel <= ucTraceLevel)
    {
        bIsOutput = DOS_TRUE;
    }

    if(!bIsOutput)
    {
        return;
    }

    switch(ulTraceTarget)
    {
        case BS_TRACE_FS:
            dos_strcpy(szTraceStr, "BS-FS:");
            break;
        case BS_TRACE_WEB:
            dos_strcpy(szTraceStr, "BS-WEB:");
            break;
        case BS_TRACE_DB:
            dos_strcpy(szTraceStr, "BS-DB:");
            break;
        case BS_TRACE_CDR:
            dos_strcpy(szTraceStr, "BS-CDR:");
            break;
        case BS_TRACE_BILLING:
            dos_strcpy(szTraceStr, "BS-BILL:");
            break;
        case BS_TRACE_ACCOUNT:
            dos_strcpy(szTraceStr, "BS-ACC:");
            break;
        case BS_TRACE_STAT:
            dos_strcpy(szTraceStr, "BS-STAT:");
            break;
        case BS_TRACE_AUDIT:
            dos_strcpy(szTraceStr, "BS-AUDT:");
            break;
        case BS_TRACE_MAINTAIN:
            dos_strcpy(szTraceStr, "BS-MANT:");
            break;
        case BS_TRACE_RUN:
            dos_strcpy(szTraceStr, "BS-RUN:");
            break;
        default:
            dos_strcpy(szTraceStr, "BS:");
            break;
    }

    ulTraceTagLen = dos_strlen(szTraceStr);

    va_start(Arg, szFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, szFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(ucTraceLevel, LOG_TYPE_RUNINFO, szTraceStr);
}

S32 bs_update_test(U32 ulIndex, S32 argc, S8 **argv)
{
    U32  ulOperation = BS_CMD_BUTT;
    S32 lRet = -1;
    BS_WEB_CMD_INFO_ST *pszTblRow   = NULL;
    const S8  *pszOperation  = NULL, *pszTableName = NULL;
    S8  szCmdBuff[1024] = {0, };
    DLL_NODE_S *pstNode = NULL, *pstListNode = NULL;
    JSON_OBJ_ST         *pstJsonNode = NULL;

    lRet = bsd_walk_web_cmd_tbl(NULL);
    if (lRet < 0)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nlRet is %d\r\n", lRet);
        cli_out_string(ulIndex, szCmdBuff);
        goto help;/*goto help;*/
    }

    pthread_mutex_lock(&g_mutexWebCMDTbl);
    pstListNode = dll_fetch(&g_stWebCMDTbl);
    pthread_mutex_unlock(&g_mutexWebCMDTbl);

    if (DOS_ADDR_INVALID(pstListNode))
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\npstListNode is %p.\r\n", pstListNode);
        cli_out_string(ulIndex, szCmdBuff);
        goto help;
    }
    pszTblRow = (BS_WEB_CMD_INFO_ST *)pstListNode->pHandle;
    if (DOS_ADDR_INVALID(pszTblRow))
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\npszTblRow is %p.\r\n", pszTblRow);
        cli_out_string(ulIndex, szCmdBuff);
        goto help;
    }

      /* 获取json格式数据 */
    pstJsonNode = pszTblRow->pstData;
    if (DOS_ADDR_INVALID(pstJsonNode))
    {
        DOS_ASSERT(0);/*json格式数据*/
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\npstJsonNode is %p.\r\n", pstJsonNode);
        cli_out_string(ulIndex, szCmdBuff);

        goto help;
    }

    pszOperation = json_get_param(pstJsonNode, "dboperate");
    pszTableName = json_get_param(pstJsonNode, "table");
    if (DOS_ADDR_INVALID(pszOperation))
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\npszOperation is %p.\r\n", pszOperation);/* goto help; */
        cli_out_string(ulIndex, szCmdBuff);
    }

    if (0 == dos_stricmp(pszOperation, "insert"))
    {
        ulOperation = BS_CMD_INSERT;
    }
    else if (0 == dos_stricmp(pszOperation, "update"))
    {
        ulOperation = BS_CMD_UPDATE;
    }
    else if (0 == dos_stricmp(pszOperation, "delete"))
    {
        ulOperation = BS_CMD_DELETE;
    }
    else
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nUnknown operation \"%s\". ╰_╯\r\n", pszOperation);
        cli_out_string(ulIndex, szCmdBuff);
        goto help;
    }

    if (2 != argc)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nOnly need 2 params,but you input %d param(s). ╰_╯\r\n", argc);
        cli_out_string(ulIndex, szCmdBuff);
        goto help;
    }

    if (0 != dos_stricmp(argv[1], "update"))
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nThe param 1 must be \"%s\",but your input is \"%s\". ╰_╯\r\n", "update", argv[1]);
        cli_out_string(ulIndex, szCmdBuff);
        goto help;
    }

    if (0 == dos_stricmp(pszTableName, "tbl_customer"))
    {
        bss_update_customer(ulOperation, pstJsonNode);
    }
    else if (0 == dos_stricmp(pszTableName, "tbl_agent"))
    {
        bss_update_agent(ulOperation, pstJsonNode);
    }
    else if (0 == dos_stricmp(pszTableName, "tbl_billing_rule"))
    {
        bss_update_billing_package(ulOperation, pstJsonNode);
    }
    else if (0 == dos_stricmp(pszTableName, "tbl_billing_rate"))
    {

    }
    else if (0 == dos_stricmp(pszTableName, "tbl_calltask"))
    {
        bss_update_call_task(ulOperation, pstJsonNode);
    }
    else
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nOh,My God! The table name is %s, What do you want to do? ╰_╯\r\n", pszTableName);
        cli_out_string(ulIndex, szCmdBuff);
        goto help;
    }

    return DOS_SUCC;

help:
    if (DOS_ADDR_VALID(pstNode))
    {
        dos_dmem_free(pstNode);
        pstNode = NULL;
    }
    if (DOS_ADDR_VALID(pstListNode))
    {
        dos_dmem_free(pstListNode);
        pstListNode = NULL;
    }
    if (DOS_ADDR_VALID(pszTblRow))
    {
        dos_dmem_free(pszTblRow);
        pszTblRow = NULL;
    }
    if (DOS_ADDR_VALID(pstJsonNode))
    {
        json_deinit(&pstJsonNode);
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nHelp:\r\n    bsd bst update agent|customer|billing|task\r\n");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n\r\nUpdate test fail. It\'s a pity! =_=\" .\r\n");
    cli_out_string(ulIndex, szCmdBuff);

    return DOS_FAIL;
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

