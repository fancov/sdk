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
#include "sc_pub.h"
#include "sc_task_pub.h"
#include "sc_debug.h"
#include "sc_event_process.h"

#define SC_NUM_PREFIX_LEN     16
#define SC_GW_DOMAIN_LEG      32

typedef struct tagSCGWNode
{
    U32 ulGWID;
    S8  szGWDomain[SC_GW_DOMAIN_LEG];
}SC_GW_NODE_ST;

typedef struct tagSCRouteGroupNode
{
    U32        ulGroupID;

    U8         ucHourBegin;                       /* 开始时间，小时 */
    U8         ucMinuteBegin;                     /* 开始时间，分钟 */
    U8         ucHourEnd;                         /* 结束时间，小时 */
    U8         ucMinuteEnd;                       /* 结束时间，分钟 */

    S8         szCallerPrefix[SC_NUM_PREFIX_LEN]; /* 前缀长度 */
    S8         szCalleePrefix[SC_NUM_PREFIX_LEN]; /* 前缀长度 */

    DLL_S      stGWList;                          /* */

    pthread_mutex_t  mutexGWList;
}SC_ROUTE_GRP_NODE_ST;

typedef struct tagSCEventNode
{
    list_t stLink;

    esl_event_t *pstEvent;
}SC_EP_EVENT_NODE_ST;

typedef struct tagSCEventProcessHandle
{
    esl_handle_t        stRecvHandle;                /*  esl 句柄 */
    esl_handle_t        stSendHandle;                /*  esl 句柄 */
    pthread_t           pthID;

    BOOL                blIsESLRunning;          /* ESL是否连接正常 */
    BOOL                blIsWaitingExit;         /* 任务是否正在等待退出 */
}SC_EP_HANDLE_ST;

extern U32   g_ulTaskTraceAll;


SC_EP_HANDLE_ST        *g_pstHandle = NULL;
list_t                 g_stEventList;    /* REFER TO SC_EP_EVENT_NODE_ST */
pthread_mutex_t        g_mutexEventList = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t         g_condEventList = PTHREAD_COND_INITIALIZER;

DLL_S                  g_stRouteGrpList;        /* 中继组链表 REFER TO SC_ROUTE_GRP_NODE_ST */
pthread_mutex_t        g_mutexRouteGrpList = PTHREAD_MUTEX_INITIALIZER;



U32 g_ulESLDebugLevel = ESL_LOG_LEVEL_INFO;


U32 sc_load_gateway_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_ROUTE_GRP_NODE_ST *pstRouetGroup = NULL;
    SC_GW_NODE_ST        *pstGWNode     = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    S8 *pszGWID    = NULL;
    S8 *pszDomain  = NULL;
    U32 ulID;

    if (DOS_ADDR_INVALID(pArg)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pszGWID = aszValues[0];
    pszDomain = aszValues[1];
    pstRouetGroup = pArg;
    if (DOS_ADDR_INVALID(pszGWID)
        || DOS_ADDR_INVALID(pszDomain)
        || dos_atoul(pszGWID, &ulID) < 0)
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

    pstGWNode = dos_dmem_alloc(sizeof(SC_GW_NODE_ST));
    if (DOS_ADDR_INVALID(pstGWNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstListNode);
        return DOS_FAIL;
    }

    pstGWNode->ulGWID = ulID;
    dos_strncpy(pstGWNode->szGWDomain, pszDomain, sizeof(pstGWNode->szGWDomain));
    pstGWNode->szGWDomain[sizeof(pstGWNode->szGWDomain) - 1] = '\0';

    DLL_Init_Node(pstListNode);
    pstListNode->pHandle = pstGWNode;

    pthread_mutex_lock(&pstRouetGroup->mutexGWList);
    DLL_Add(&pstRouetGroup->stGWList, pstListNode);
    pthread_mutex_unlock(&pstRouetGroup->mutexGWList);

    return DOS_SUCC;
}

U32 sc_load_gateway(U32 ulGroupID)
{
    S8 szSQL[1024];

    dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, domain FROM tbl_gateway WHERE group_id = %d;"
                    , ulGroupID);

    return DOS_SUCC;
}

U32 sc_load_route_group_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_ROUTE_GRP_NODE_ST *pstRouetGroup = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    S32                  lIndex;
    S32                  lSecond;
    S32                  lRet;
    S32                  lItemCnt = 0;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstRouetGroup = dos_dmem_alloc(sizeof(SC_ROUTE_GRP_NODE_ST));
    if (DOS_ADDR_INVALID(pstRouetGroup))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    pthread_mutex_init(&pstRouetGroup->mutexGWList, NULL);

    for (lItemCnt=0, lIndex=0; lIndex<lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszNames[lIndex], &pstRouetGroup->ulGroupID) < 0)
            {
                DOS_ASSERT(0);
                break;
            }

            lItemCnt++;
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "start_time", dos_strlen("start_time")))
        {
            lRet = dos_sscanf(aszNames[lIndex]
                        , "%d:%d:%d"
                        , &pstRouetGroup->ucHourBegin
                        , &pstRouetGroup->ucMinuteBegin
                        , &lSecond);
            if (3 != lRet)
            {
                DOS_ASSERT(0);
                break;
            }

            lItemCnt++;
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "end_time", dos_strlen("end_time")))
        {
            lRet = dos_sscanf(aszNames[lIndex]
                    , "%d:%d:%d"
                    , &pstRouetGroup->ucHourEnd
                    , &pstRouetGroup->ucMinuteEnd
                    , &lSecond);
            if (3 != lRet)
            {
                DOS_ASSERT(0);
                break;
            }

            lItemCnt++;
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "callee_prefix", dos_strlen("callee_prefix")))
        {
            dos_strncpy(pstRouetGroup->szCalleePrefix, aszNames[lIndex], sizeof(pstRouetGroup->szCalleePrefix));
            pstRouetGroup->szCalleePrefix[sizeof(pstRouetGroup->szCalleePrefix) - 1] = '\0';

            lItemCnt++;
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "callee_prefix", dos_strlen("caller_prefix")))
        {
            dos_strncpy(pstRouetGroup->szCalleePrefix, aszNames[lIndex], sizeof(pstRouetGroup->szCalleePrefix));
            pstRouetGroup->szCalleePrefix[sizeof(pstRouetGroup->szCalleePrefix) - 1] = '\0';

            lItemCnt++;
        }
    }

    if (5 != lItemCnt)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstRouetGroup);
        return DOS_FAIL;
    }

    pstListNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstRouetGroup);
        return DOS_FAIL;
    }

    DLL_Init_Node(pstListNode);
    pstListNode->pHandle = pstRouetGroup;
    pthread_mutex_lock(&g_mutexRouteGrpList);
    DLL_Add(&g_stRouteGrpList, pstListNode);
    pthread_mutex_unlock(&g_mutexRouteGrpList);

    return DOS_TRUE;
}


U32 sc_load_route_group()
{
    S8 szSQL[1024];
    SC_ROUTE_GRP_NODE_ST *pstRouetGroup = NULL;
    DLL_NODE_S           *pstListNode   = NULL;

    dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, start_time, end_time, callee_prefix, caller_prefix FROM tbl_route_grp;");

    pthread_mutex_lock(&g_mutexRouteGrpList);
    DLL_Scan(&g_stRouteGrpList, pstListNode, DLL_NODE_S *)
    {
        pstRouetGroup = (SC_ROUTE_GRP_NODE_ST *)pstListNode->pHandle;
        sc_load_gateway(pstRouetGroup->ulGroupID);
    }
    pthread_mutex_unlock(&g_mutexRouteGrpList);

    return DOS_SUCC;
}

U32 sc_ep_esl_execute(esl_handle_t *pstHandle, const S8 *pszApp, const S8 *pszArg, const S8 *pszUUID)
{
    U32 ulRet;

    if (DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pszApp))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (!pstHandle->connected)
    {
        ulRet = esl_connect(pstHandle, "127.0.0.1", 8021, NULL, "ClueCon");
        if (ESL_SUCCESS != ulRet)
        {
            esl_disconnect(pstHandle);
            sc_logr_notice("ELS for send event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, pstHandle->err);

            DOS_ASSERT(0);

            return DOS_FAIL;
        }
    }

    if (ESL_SUCCESS != esl_execute(pstHandle, pszApp, pszArg, pszUUID))
    {
        DOS_ASSERT(0);
        sc_logr_notice("ESL execute command fail. Result:%d, APP: %s, ARG : %s, UUID: %s"
                        , ulRet
                        , pszApp
                        , DOS_ADDR_VALID(pszArg) ? pszArg : "NULL"
                        , DOS_ADDR_VALID(pszUUID) ? pszUUID : "NULL");

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_ep_parse_event(esl_event_t *pstEvent, SC_CCB_ST *pstCCB)
{
    S8         *pszCaller    = NULL;
    S8         *pszCallee    = NULL;
    S8         *pszANI       = NULL;
    S8         *pszCallSrc   = NULL;
    S8         *pszTrunkIP   = NULL;
    S8         *pszGwName    = NULL;
    S8         *pszCallDirection = NULL;

    SC_TRACE_IN(pstEvent, pstCCB, 0, 0);

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstCCB))
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
    if (DOS_ADDR_INVALID(pszCaller) || DOS_ADDR_INVALID(pszCallee) || DOS_ADDR_INVALID(pszANI))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 将相关数据写入CCB中 */
    pthread_mutex_lock(&pstCCB->mutexCCBLock);
    dos_strncpy(pstCCB->szCalleeNum, pszCallee, sizeof(pstCCB->szCalleeNum));
    pstCCB->szCalleeNum[sizeof(pstCCB->szCalleeNum) -1] = '\0';
    dos_strncpy(pstCCB->szCallerNum, pszCaller, sizeof(pstCCB->szCallerNum));
    pstCCB->szCallerNum[sizeof(pstCCB->szCallerNum) -1] = '\0';
    dos_strncpy(pstCCB->szANINum, pszANI, sizeof(pstCCB->szANINum));
    pstCCB->szANINum[sizeof(pstCCB->szANINum) -1] = '\0';
    pthread_mutex_unlock(&pstCCB->mutexCCBLock);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_ep_internal_service_check(esl_event_t *pstEvent)
{
    return SC_INTER_SRV_BUTT;
}

BOOL sc_ep_check_extension(S8 *pszNum, U32 ulCustomerID)
{
    return DOS_TRUE;
}

U32 sc_ep_get_custom_by_sip_userid(S8 *pszNum)
{
    return U32_BUTT;
}

U32 sc_ep_get_custom_by_did(S8 *pszNum)
{
    return U32_BUTT;
}

U32 sc_ep_get_userid_by_extension(S8 *pszExtension, S8 *pszUserID, U32 ulLength)
{
    return DOS_SUCC;
}


U32 sc_ep_search_gw_group(SC_CCB_ST *pstCCB)
{
    SC_ROUTE_GRP_NODE_ST *pstRouetGroup = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    struct tm            *pstTime;
    time_t               timep;
    U32                  ulRouteGrpID;

    timep = time(NULL);
    pstTime = localtime(&timep);
    if (DOS_ADDR_INVALID(pstTime))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    ulRouteGrpID = U32_BUTT;
    pthread_mutex_lock(&g_mutexRouteGrpList);
    DLL_Scan(&g_stRouteGrpList, pstListNode, DLL_NODE_S *)
    {
        pstRouetGroup = (SC_ROUTE_GRP_NODE_ST *)pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstRouetGroup))
        {
            continue;
        }

        /* 先看看小时是否匹配 */
        if (pstTime->tm_hour < pstRouetGroup->ucHourBegin
            || pstTime->tm_hour > pstRouetGroup->ucHourEnd)
        {
            continue;
        }

        /* 判断分钟对不对 */
        if (pstTime->tm_min < pstRouetGroup->ucMinuteBegin
            || pstTime->tm_min > pstRouetGroup->ucMinuteEnd)
        {
            continue;
        }

        if (0 != dos_strnicmp(pstRouetGroup->szCalleePrefix, pstCCB->szCalleeNum, dos_strlen(pstRouetGroup->szCalleePrefix))
            || 0 != dos_strnicmp(pstRouetGroup->szCallerPrefix, pstCCB->szCallerNum, dos_strlen(pstRouetGroup->szCallerPrefix)))
        {
            continue;
        }

        ulRouteGrpID = pstRouetGroup->ulGroupID;
        break;
    }

    pthread_mutex_unlock(&g_mutexRouteGrpList);

    return ulRouteGrpID;
}

U32 sc_ep_get_callee_string(S8 *pszNum, S8 *szCalleeString, U32 ulLength, U32 ulRouteGroupID)
{
    SC_ROUTE_GRP_NODE_ST *pstRouetGroup = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    DLL_NODE_S           *pstListNode2  = NULL;
    SC_GW_NODE_ST        *pstGW         = NULL;
    U32                  ulCurrentLen;
    U32                  ulGWCount;
    BOOL                 blIsFound = DOS_FALSE;

    if (DOS_ADDR_INVALID(pszNum)
        || DOS_ADDR_INVALID(szCalleeString)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulCurrentLen = 0;
    pthread_mutex_lock(&g_mutexRouteGrpList);
    DLL_Scan(&g_stRouteGrpList, pstListNode, DLL_NODE_S *)
    {
        pstRouetGroup = (SC_ROUTE_GRP_NODE_ST *)pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstRouetGroup))
        {
            continue;
        }

        if (pstRouetGroup->ulGroupID == ulRouteGroupID)
        {
            ulGWCount = 0;
            pthread_mutex_lock(&pstRouetGroup->mutexGWList);
            DLL_Scan(&pstRouetGroup->stGWList, pstListNode2, DLL_NODE_S *)
            {
                pstGW = pstListNode2->pHandle;
                if (DOS_ADDR_INVALID(pstGW))
                {
                    continue;
                }

                ulGWCount++;

                ulCurrentLen = dos_snprintf(szCalleeString + ulCurrentLen
                                , ulLength - ulCurrentLen
                                , "sofia/gateway/%d/%s|"
                                , pstGW->ulGWID
                                , pszNum);
                if (ulLength >= ulCurrentLen)
                {
                    break;
                }
            }

            if (ulGWCount)
            {
                blIsFound = DOS_TRUE;
            }
            pthread_mutex_unlock(&pstRouetGroup->mutexGWList);
            break;
        }
    }

    pthread_mutex_unlock(&g_mutexRouteGrpList);

    if (blIsFound)
    {
        /* 最后多了一个  | */
        szCalleeString[dos_strlen(szCalleeString) - 1] = '\0';
        return DOS_SUCC;
    }
    else
    {
        szCalleeString[0] = '\0';
        return DOS_FAIL;
    }
}

BOOL sc_ep_dst_is_externsion(S8 *pszDstNum)
{
    return DOS_FALSE;
}

BOOL sc_ep_dst_is_sip_account(S8 *pszDstNum)
{
    return DOS_TRUE;
}

BOOL sc_ep_dst_is_did(S8 *pszDstNum)
{
    return DOS_FALSE;
}

BOOL sc_ep_dst_is_black(S8 *pszDstNum)
{
    return DOS_FALSE;
}

U32 sc_ep_get_source(esl_event_t *pstEvent)
{
    const S8 *pszCallSource;

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

    return SC_DIRECTION_PSTN;
}

U32 sc_ep_get_destination(esl_event_t *pstEvent)
{
    S8 *pszDstNum = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);

        return SC_DIRECTION_INVALID;
    }

    pszDstNum = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    if (DOS_ADDR_INVALID(pszDstNum))
    {
        DOS_ASSERT(0);

        return SC_DIRECTION_INVALID;
    }

    if (sc_ep_dst_is_black(pszDstNum))
    {
        sc_logr_notice("%s", "The destination is in black list.");

        return SC_DIRECTION_INVALID;
    }

    if (sc_ep_dst_is_externsion(pszDstNum))
    {
        return SC_DIRECTION_SIP;
    }

    if (sc_ep_dst_is_sip_account(pszDstNum))
    {
        return SC_DIRECTION_SIP;
    }

    if (sc_ep_dst_is_did(pszDstNum))
    {
        return SC_DIRECTION_SIP;
    }

    return SC_DIRECTION_PSTN;
}

U32 sc_ep_incoming_call_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    S8    *pszDstNum = NULL;
    S8    *pszUUID = NULL;
    U32   ulCustomerID;
    S8    szCallString[512] = { 0, };

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstHandle) || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* 获取事件的UUID */
    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pszDstNum = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    if (DOS_ADDR_INVALID(pszDstNum))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
/*
    if (sc_ep_dst_is_black(pstEvent))
    {
        DOS_ASSERT(0);

        sc_logr_info("Called number \"%s\" is in black list.", pszDstNum);

        return DOS_FAIL;
    }
    */

    if (sc_ep_dst_is_did(pszDstNum))
    {
        ulCustomerID = sc_ep_get_custom_by_did(pszDstNum);
        if (U32_BUTT != ulCustomerID)
        {
            /* @TODO 查找出来DID对应的SIP账户，或者坐席队列 */
        }
    }

    if (sc_ep_dst_is_sip_account(pszDstNum))
    {
        dos_snprintf(szCallString, sizeof(szCallString), "user/%s", pszDstNum);
        sc_ep_esl_execute(pstHandle, "bridge", szCallString, pszUUID);

        return DOS_TRUE;
    }

    return DOS_SUCC;
}

U32 sc_ep_outgoing_call_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    U32 ulTrunkGrpID = U32_BUTT;
    S8  szCallString[1024] = { 0, };

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstHandle) || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (sc_ep_dst_is_black(pstCCB->szCalleeNum))
    {
        DOS_ASSERT(0);

        sc_call_trace(pstCCB,"The callee is in BLACK LIST. The call will be hungup later. UUID: %s", pstCCB->szUUID);
        return DOS_FAIL;
    }

    ulTrunkGrpID = sc_ep_search_gw_group(pstCCB);
    if (U32_BUTT == ulTrunkGrpID)
    {
        DOS_ASSERT(0);

        sc_call_trace(pstCCB,"Find trunk gruop FAIL. The call will be hungup later. UUID: %s", pstCCB->szUUID);
        return DOS_FAIL;
    }

    pstCCB->ulTrunkID = ulTrunkGrpID;
    if (DOS_SUCC != sc_ep_get_callee_string(pstCCB->szCalleeNum, szCallString, sizeof(szCallString), ulTrunkGrpID))
    {
        DOS_ASSERT(0);

        sc_call_trace(pstCCB,"Make call string FAIL. The call will be hungup later. UUID: %s", pstCCB->szUUID);
        return DOS_FAIL;
    }
#if 0
    if (sc_send_usr_auth2bs(pstCCB))
    {
        DOS_ASSERT(0);

        sc_call_trace(pstCCB, "Auth fail. The call will be hungup later. UUID: %s", pstCCB->szUUID);
        return DOS_FAIL;
    }
#endif

    sc_ep_esl_execute(pstHandle, "bridge", szCallString, pstCCB->szUUID);

    SC_CCB_SET_STATUS(pstCCB, SC_CCB_ACTIVE);

    return DOS_SUCC;

}

U32 sc_ep_auto_dial(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    return sc_ep_outgoing_call_proc(pstEvent, pstHandle, pstCCB);
}


U32 sc_ep_internal_call_process(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    S8    *pszDstNum = NULL;
    S8    *pszUUID = NULL;
    S8    szCallString[512] = { 0, };

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstHandle) || DOS_ADDR_INVALID(pstCCB))
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
        return DOS_FAIL;
    }

    pszDstNum = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    if (DOS_ADDR_INVALID(pszDstNum))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    dos_snprintf(szCallString, sizeof(szCallString), "user/%s", pszDstNum);
    sc_ep_esl_execute(pstHandle, "bridge", szCallString, pszUUID);

    return DOS_SUCC;

}


U32 sc_ep_internal_service_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    return DOS_SUCC;
}


U32 sc_ep_channel_park_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    S8        *pszIsAutoCall = NULL;
    S8        *pszCaller     = NULL;
    S8        *pszCallee     = NULL;
    S8        *pszUUID;
    U32       ulCallSrc, ulCallDst;

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

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


    /* 接听主叫方呼叫 */
    sc_ep_esl_execute(pstHandle, "answer", NULL, pszUUID);

    /* 业务控制 */
    pszIsAutoCall = esl_event_get_header(pstEvent, "auto_flag");
    pszCaller     = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    pszCallee     = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    if (DOS_ADDR_VALID(pszIsAutoCall)
        && 0 == dos_strnicmp(pszIsAutoCall, "true", dos_strlen("true")))
    {
        /* 自动外呼处理 */
        SC_CCB_SET_SERVICE(pstCCB, SC_SERV_OUTBOUND_CALL);
        SC_CCB_SET_SERVICE(pstCCB, SC_SERV_EXTERNAL_CALL);
        SC_CCB_SET_SERVICE(pstCCB, SC_SERV_AUTO_DIALING);

        return sc_ep_auto_dial(pstEvent, pstHandle, pstCCB);
    }
    else if (sc_ep_internal_service_check(pstEvent) != SC_INTER_SRV_BUTT)
    {
        /* 内部业务处理 */
        SC_CCB_SET_SERVICE(pstCCB, SC_SERV_INBOUND_CALL);
        SC_CCB_SET_SERVICE(pstCCB, SC_SERV_INTERNAL_CALL);
        SC_CCB_SET_SERVICE(pstCCB, SC_SERV_INTERNAL_SERVICE);

        return sc_ep_internal_service_proc(pstEvent, pstHandle, pstCCB);
    }
    else
    {
        /* 正常呼叫处理 */
        ulCallSrc = sc_ep_get_source(pstEvent);
        ulCallDst = sc_ep_get_destination(pstEvent);

        sc_logr_info("Get call source and dest. Source: %d, Dest: %d", ulCallSrc, ulCallDst);

        if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_PSTN == ulCallDst)
        {
            SC_CCB_SET_SERVICE(pstCCB, SC_SERV_INBOUND_CALL);
            SC_CCB_SET_SERVICE(pstCCB, SC_SERV_EXTERNAL_CALL);

            return sc_ep_outgoing_call_proc(pstEvent, pstHandle, pstCCB);
        }
        else if (SC_DIRECTION_PSTN == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
        {
            SC_CCB_SET_SERVICE(pstCCB, SC_SERV_INBOUND_CALL);
            SC_CCB_SET_SERVICE(pstCCB, SC_SERV_EXTERNAL_CALL);

            return sc_ep_incoming_call_proc(pstEvent, pstHandle, pstCCB);
        }
        else if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
        {
            SC_CCB_SET_SERVICE(pstCCB, SC_SERV_INBOUND_CALL);
            SC_CCB_SET_SERVICE(pstCCB, SC_SERV_INTERNAL_CALL);

            return sc_ep_internal_call_process(pstEvent, pstHandle, pstCCB);
        }
        else
        {
            sc_logr_info("Invalid call source or destension. Source: %d, Dest: %d", ulCallSrc, ulCallDst);
            goto hungup_proc;
        }
    }

    return DOS_SUCC;

hungup_proc:
    return DOS_FAIL;
}


U32 sc_ep_channel_create_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle)
{
    S8          *pszUUID = NULL;
    S8          *pszOtherUUID = NULL;
    SC_CCB_ST   *pstCCB = NULL;
    S8          szBuffCmd[128] = { 0 };
    U32         ulRet = DOS_SUCC;


    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pszOtherUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        sc_ep_esl_execute(pstHandle, "set", "is_lega=true", pszUUID);
    }
    else
    {
        sc_ep_esl_execute(pstHandle, "set", "is_legb=true", pszUUID);
    }

    pstCCB = sc_ccb_alloc();
    if (!pstCCB)
    {
        DOS_ASSERT(0);
        sc_logr_error("%s", "Alloc CCB FAIL.");

        SC_CCB_SET_STATUS(pstCCB, SC_CCB_RELEASE);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_ccb_hash_tables_add(pszUUID, pstCCB);
    sc_ep_parse_event(pstEvent, pstCCB);

    dos_strncpy(pstCCB->szUUID, pszUUID, sizeof(pstCCB->szUUID));
    pstCCB->szUUID[sizeof(pstCCB->szUUID) - 1] = '\0';

    /* 给通道设置变量 */
    dos_snprintf(szBuffCmd, sizeof(szBuffCmd), "ccb_number=%d", pstCCB->usCCBNo);
    sc_ep_esl_execute(pstHandle, "set", szBuffCmd, pszUUID);

    SC_CCB_SET_STATUS(pstCCB, SC_CCB_INIT);

    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return ulRet;
}


U32 sc_ep_channel_exec_complete_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    S8 *pszAppication = NULL;
    S8 *pszDesc = NULL;
    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    pszAppication = esl_event_get_header(pstEvent, "Application");
    pszDesc = esl_event_get_header(pstEvent, "variable_originate_disposition");

    sc_logr_debug("Exec application %s, result %s", pszAppication, pszDesc);

    if (DOS_ADDR_VALID(pszAppication)
        && 0 == dos_stricmp(pszAppication, "bridge"))
    {
        if (0 == dos_stricmp(pszDesc, "SUCCESS"))
        {
            SC_CCB_SET_STATUS(pstCCB, SC_CCB_ACTIVE);
        }
        else
        {
            /* @TODO 是否需要手动挂断电话 ? */
        }
    }

    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

U32 sc_ep_channel_hungup_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_CCB_SET_STATUS(pstCCB, SC_CCB_RELEASE);

    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

U32 sc_ep_channel_hungup_complete_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    U32 ulStatus, ulRet = DOS_SUCC;
    S8  *pszUUID1       = NULL;
    S8  *pszUUID2       = NULL;
    SC_CCB_ST *pstCCBOther = NULL;

    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));
    pszUUID1 = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    pszUUID2 = esl_event_get_header(pstEvent, "Other-Leg-Unique-ID");
    if (pszUUID2)
    {
        pstCCBOther = sc_ccb_hash_tables_find(pszUUID2);
    }

    ulStatus = pstCCB->usStatus;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
        case SC_CCB_INIT:
        case SC_CCB_AUTH:
        case SC_CCB_EXEC:
        case SC_CCB_ACTIVE:
        case SC_CCB_RELEASE:
            /* 如果有另外一条腿，且林外一条腿没有挂断，就只通知对端挂断，然后不再处理了 */
            SC_CCB_SET_STATUS(pstCCB, SC_CCB_RELEASE);
            if (DOS_ADDR_VALID(pszUUID2)
                && DOS_ADDR_VALID(pstCCBOther)
                && DOS_TRUE != pstCCBOther->bWaitingOtherRelase)
            {
                pstCCB->bWaitingOtherRelase = DOS_TRUE;

                sc_ep_esl_execute(pstHandle, "hangup", NULL, pszUUID2);

                sc_logr_info("Waiting other leg hangup.Curretn Leg UUID: %s, Other Leg UUID: %s"
                                , pszUUID1 ? pszUUID1 : "NULL"
                                , pszUUID2 ? pszUUID2 : "NULL");
                break;
            }

            /* 发送话单 */
            //sc_send_billing_stop2bs(pstCCB);

            /* 维护资源 */
            sc_ccb_hash_tables_delete(pszUUID1);
            if (DOS_ADDR_VALID(pszUUID2))
            {
                sc_ccb_hash_tables_delete(pszUUID2);
            }

            sc_ccb_free(pstCCB);
            if (pstCCBOther)
            {
                sc_ccb_free(pstCCBOther);
            }
            break;
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;
            break;
    }


    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}


U32 sc_ep_dtmf_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));



    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

U32 sc_ep_session_heartbeat(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

U32 sc_ep_process(esl_event_t *pstEvent, esl_handle_t *pstHandle)
{
    S8                     *pszUUID = NULL;
    S8                     *pszCallerSource = NULL;
    SC_CCB_ST              *pstCCB = NULL;
    U32                    ulRet = DOS_FAIL;

    SC_TRACE_IN(pstEvent, pstHandle, 0, 0);

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstHandle))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 获取事件的UUID */
    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 如果某个事件确实没有Caller-Source ?*/
    pszCallerSource = esl_event_get_header(pstEvent, "Caller-Source");
    if (DOS_ADDR_INVALID(pszCallerSource))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (0 == dos_stricmp(pszCallerSource, "mod_loopback"))
    {
        sc_logr_info("%s", "Discard event send by loopback leg.");

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    if (ESL_EVENT_CHANNEL_CREATE != pstEvent->event_id)
    {
        pstCCB = sc_ccb_hash_tables_find(pszUUID);
        if (DOS_ADDR_INVALID(pstCCB))
        {
            DOS_ASSERT(0);
        }
    }

    sc_logr_info("Start process event: %s(%d), CCB No:%d"
                    , esl_event_get_header(pstEvent, "Event-Name")
                    , pstEvent->event_id
                    , esl_event_get_header(pstEvent, "ccb_number"));

    switch (pstEvent->event_id)
    {
        /* 获取呼叫状态 */
        case ESL_EVENT_CHANNEL_PARK:
            ulRet = sc_ep_channel_park_proc(pstEvent, pstHandle, pstCCB);
            if (ulRet != DOS_SUCC)
            {
                sc_ep_esl_execute(pstHandle, "hangup", NULL, pszUUID);
                sc_logr_info("Hangup for process event %s fail. UUID: %s", esl_event_get_header(pstEvent, "Event-Name"), pszUUID);
            }
            break;

        case ESL_EVENT_CHANNEL_CREATE:
            ulRet = sc_ep_channel_create_proc(pstEvent, pstHandle);
            if (ulRet != DOS_SUCC)
            {
                sc_ep_esl_execute(pstHandle, "hangup", NULL, pszUUID);
                sc_logr_info("Hangup for process event %s fail. UUID: %s", esl_event_get_header(pstEvent, "Event-Name"), pszUUID);
            }
            break;

        case ESL_EVENT_CHANNEL_EXECUTE_COMPLETE:
            ulRet = sc_ep_channel_exec_complete_proc(pstEvent, pstHandle, pstCCB);
            break;

        case ESL_EVENT_CHANNEL_HANGUP:
            ulRet = sc_ep_channel_hungup_proc(pstEvent, pstHandle, pstCCB);
            break;

        case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
            ulRet = sc_ep_channel_hungup_complete_proc(pstEvent, pstHandle, pstCCB);
            break;

        case ESL_EVENT_DTMF:
            ulRet = sc_ep_dtmf_proc(pstEvent, pstHandle, pstCCB);
            break;

        case ESL_EVENT_SESSION_HEARTBEAT:
            ulRet = sc_ep_session_heartbeat(pstEvent, pstHandle, pstCCB);
            break;
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;

            sc_logr_info("Recv unhandled event: %s(%d)", esl_event_get_header(pstEvent, "Event-Name"), pstEvent->event_id);
            break;
    }

    sc_logr_info("Process finished event: %s(%d). Result:%s"
                    , esl_event_get_header(pstEvent, "Event-Name")
                    , pstEvent->event_id
                    , (DOS_SUCC == ulRet) ? "Successfully" : "FAIL");

    SC_TRACE_OUT();
    return ulRet;
}

VOID*sc_ep_process_runtime(VOID *ptr)
{
    SC_EP_EVENT_NODE_ST *pstListNode = NULL;
    esl_event_t         *pstEvent = NULL;
    list_t              *pstListLink = NULL;
    U32                 ulRet;
    struct timespec     stTimeout;

    while (1)
    {
        pthread_mutex_lock(&g_mutexEventList);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condEventList, &g_mutexEventList, &stTimeout);

        if (dos_list_is_empty(&g_stEventList))
        {
            pthread_mutex_unlock(&g_mutexEventList);
            continue;
        }

        pstListLink = dos_list_fetch(&g_stEventList);
        if (DOS_ADDR_INVALID(pstListLink))
        {
            pthread_mutex_unlock(&g_mutexEventList);
            continue;
        }
        pthread_mutex_unlock(&g_mutexEventList);

        pstListNode = dos_list_entry(pstListLink, SC_EP_EVENT_NODE_ST, stLink);
        if (DOS_ADDR_INVALID(pstListNode))
        {
            continue;
        }

        if (DOS_ADDR_INVALID(pstListNode->pstEvent))
        {
            dos_dmem_free(pstListNode);
            pstListNode = NULL;
            continue;
        }

        pstEvent = pstListNode->pstEvent;

        pstListNode->pstEvent = NULL;
        dos_dmem_free(pstListNode);
        pstListNode = NULL;


        sc_logr_info("ESL event process START. %s(%d), CCB No:%s"
                        , esl_event_get_header(pstEvent, "Event-Name")
                        , pstEvent->event_id
                        , esl_event_get_header(pstEvent, "variable_ccb_no"));

        ulRet = sc_ep_process(pstEvent, &g_pstHandle->stSendHandle);
        if (ulRet != DOS_SUCC)
        {
            DOS_ASSERT(0);
        }

        sc_logr_info("ESL event process FINISHED. %s(%d), CCB No:%s Processed, Result: %d"
                        , esl_event_get_header(pstEvent, "Event-Name")
                        , pstEvent->event_id
                        , esl_event_get_header(pstEvent, "variable_ccb_no")
                        , ulRet);

        esl_event_destroy(&pstEvent);
    }

    return NULL;
}


VOID* sc_ep_runtime(VOID *ptr)
{
    U32                  ulRet = ESL_FAIL;
    S8                   *pszIsLoopback = NULL;
    SC_EP_EVENT_NODE_ST  *pstListNode = NULL;

    while(1)
    {
        /* 如果退出标志被置上，就准备退出了 */
        if (g_pstHandle->blIsWaitingExit)
        {
            sc_logr_notice("%s", "Dialer exit flag has been set. the task will be exit.");
            break;
        }

        /*
         * 检查连接是否正常
         * 如果连接不正常，就准备重连
         **/
        if (!g_pstHandle->blIsESLRunning)
        {
            sc_logr_notice("%s", "ELS for event connection has been down, re-connect.");
            g_pstHandle->stRecvHandle.event_lock = 1;
            ulRet = esl_connect(&g_pstHandle->stRecvHandle, "127.0.0.1", 8021, NULL, "ClueCon");
            if (ESL_SUCCESS != ulRet)
            {
                esl_disconnect(&g_pstHandle->stRecvHandle);
                sc_logr_notice("ELS for event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_pstHandle->stRecvHandle.err);

                sleep(1);
                continue;
            }

            g_pstHandle->blIsESLRunning = DOS_TRUE;
            esl_global_set_default_logger(g_ulESLDebugLevel);
            esl_events(&g_pstHandle->stRecvHandle, ESL_EVENT_TYPE_PLAIN, SC_EP_EVENT_LIST);

            sc_logr_notice("%s", "ELS for event connect Back to Normal.");
        }

        ulRet = esl_recv_event(&g_pstHandle->stRecvHandle, 1, NULL);
        if (ESL_FAIL == ulRet)
        {
            continue;
        }

        esl_event_t *pstEvent = g_pstHandle->stRecvHandle.last_ievent;
        if (DOS_ADDR_INVALID(pstEvent))
        {
            continue;
        }

        /* 不处理loopback的呼叫 */
        /* 这里本应该使用is_loopback这个变量，但是这个变量在一些时间中没有被设置，导致过滤不全 */
        pszIsLoopback = esl_event_get_header(pstEvent, "Channel-Name");
        if (pszIsLoopback
            && 0 == dos_strnicmp(pszIsLoopback, "loopback", dos_strlen("loopback")))
        {
            continue;
        }

        sc_logr_info("ESL recv event %s(%d)."
                        , esl_event_get_header(pstEvent, "Event-Name")
                        , pstEvent->event_id);

        pstListNode = (SC_EP_EVENT_NODE_ST *)dos_dmem_alloc(sizeof(SC_EP_EVENT_NODE_ST));
        if (DOS_ADDR_INVALID(pstListNode))
        {
            DOS_ASSERT(0);

            sc_logr_info("ESL recv event %s(%d). Alloc memory fail. Drop"
                            , esl_event_get_header(pstEvent, "Event-Name")
                            , pstEvent->event_id);

            continue;
        }

        pthread_mutex_lock(&g_mutexEventList);

        dos_list_node_init(&pstListNode->stLink);
        esl_event_dup(&pstListNode->pstEvent, pstEvent);
        dos_list_add_tail(&g_stEventList, (list_t *)pstListNode);

        pthread_cond_signal(&g_condEventList);
        pthread_mutex_unlock(&g_mutexEventList);
    }

    /* @TODO 释放资源 */
    return NULL;
}

U32 sc_ep_init()
{
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstHandle = dos_dmem_alloc(sizeof(SC_EP_HANDLE_ST));
    if (!g_pstHandle)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_memzero(g_pstHandle, sizeof(SC_EP_HANDLE_ST));
    g_pstHandle->blIsESLRunning = DOS_FALSE;
    g_pstHandle->blIsWaitingExit = DOS_FALSE;


    dos_list_init(&g_stEventList);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_ep_start()
{
    SC_TRACE_IN(0, 0, 0, 0);

    if (pthread_create(&g_pstHandle->pthID, NULL, sc_ep_runtime, NULL) < 0)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (pthread_create(&g_pstHandle->pthID, NULL, sc_ep_process_runtime, NULL) < 0)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }


    SC_TRACE_OUT();
    return DOS_SUCC;
}

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


