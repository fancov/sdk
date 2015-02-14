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


#define SC_NUM_PREFIX_LEN       16
#define SC_GW_DOMAIN_LEG        32
#define SC_IP_USERID_HASH_SIZE  1024
#define SC_IP_DID_HASH_SIZE     1024


/*  DID号码绑定类型 */
typedef enum tagSCDIDBindType{
    SC_DID_BIND_TYPE_SIP     = 1,               /* DID号码被绑定到SIP账户 */
    SC_DID_BIND_TYPE_QUEUE   = 2,               /* DID号码被绑定到坐席队列 */
    SC_DID_BIND_TYPE_BUTT
}SC_DID_BIND_TYPE_EN;

/* User ID 描述节点 */
typedef struct tagSCUserIDNode{
    U32  ulCustomID;                             /* 用户 ID */
    U32  ulSIPID;                                /* 账户 ID */
    S8   szUserID[SC_TEL_NUMBER_LENGTH];         /* SIP账户 */
    S8   szExtension[SC_TEL_NUMBER_LENGTH];      /* 分机号 */
}SC_USER_ID_NODE_ST;


/* DIDI号码们描述节点 */
typedef struct tagSCDIDNode{
    U32   ulCustomID;                             /* 用户ID */
    U32   ulDIDID;                                /* DID 号码ID */
    S8    szDIDNum[SC_TEL_NUMBER_LENGTH];         /* DID 号码 */
    U32   ulBindType;                             /* 绑定类型 refer to SC_DID_BIND_TYPE_EN */
    U32   ulBindID;                               /* 绑定结果 */
}SC_DID_NODE_ST;

/* 网关描述节点 */
typedef struct tagSCGWNode
{
    U32 ulGWID;                                    /* 网关ID */
    S8  szGWDomain[SC_GW_DOMAIN_LEG];              /* 网关的域，暂时没有用的 */
}SC_GW_NODE_ST;

/* 路由描述节点 */
typedef struct tagSCRouteGroupNode
{
    U32        ulGroupID;

    U8         ucHourBegin;                       /* 开始时间，小时 */
    U8         ucMinuteBegin;                     /* 开始时间，分钟 */
    U8         ucHourEnd;                         /* 结束时间，小时 */
    U8         ucMinuteEnd;                       /* 结束时间，分钟 */

    S8         szCallerPrefix[SC_NUM_PREFIX_LEN]; /* 前缀长度 */
    S8         szCalleePrefix[SC_NUM_PREFIX_LEN]; /* 前缀长度 */

    DLL_S      stGWList;                          /* 网关列表 refer to SC_GW_NODE_ST */

    pthread_mutex_t  mutexGWList;                 /* 路由组的锁 */
}SC_ROUTE_GRP_NODE_ST;

/* 时间队列 */
typedef struct tagSCEventNode
{
    list_t stLink;

    esl_event_t *pstEvent;
}SC_EP_EVENT_NODE_ST;

/* ESL客户端控制块 */
typedef struct tagSCEventProcessHandle
{
    pthread_t           pthID;
    esl_handle_t        stRecvHandle;                /*  esl 接收 句柄 */
    esl_handle_t        stSendHandle;                /*  esl 发送 句柄 */

    BOOL                blIsESLRunning;              /* ESL是否连接正常 */
    BOOL                blIsWaitingExit;             /* 任务是否正在等待退出 */
    U32                 ulESLDebugLevel;             /* ESL调试级别 */
}SC_EP_HANDLE_ST;

/* 应用外部变量 */
extern U32               g_ulTaskTraceAll;
extern SC_TASK_MNGT_ST   *g_pstTaskMngtInfo;

/* ESL 句柄维护 */
SC_EP_HANDLE_ST          *g_pstHandle = NULL;
pthread_mutex_t          g_mutexEventList = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t           g_condEventList = PTHREAD_COND_INITIALIZER;

/* 事件队列 REFER TO SC_EP_EVENT_NODE_ST */
list_t                   g_stEventList;

/* 中继组链表 REFER TO SC_ROUTE_GRP_NODE_ST */
DLL_S                    g_stRouteGrpList;
pthread_mutex_t          g_mutexRouteGrpList = PTHREAD_MUTEX_INITIALIZER;

/* SIP账户HASH表 REFER TO SC_USER_ID_NODE_ST */
HASH_TABLE_S             *g_pstHashSIPUserID  = NULL;
pthread_mutex_t          g_mutexHashSIPUserID = PTHREAD_MUTEX_INITIALIZER;

/* DID号码hash表 REFER TO SC_DID_NODE_ST */
HASH_TABLE_S             *g_pstHashDIDNum  = NULL;
pthread_mutex_t          g_mutexHashDIDNum = PTHREAD_MUTEX_INITIALIZER;

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
    }
}

/**
 * 函数: VOID sc_ep_route_init(SC_ROUTE_GRP_NODE_ST *pstRoute)
 * 功能: 初始化pstRoute所指向的路由描述结构
 * 参数:
 *      SC_ROUTE_GRP_NODE_ST *pstRoute 需要别初始化的路由描述结构
 * 返回值: VOID
 */
VOID sc_ep_route_init(SC_ROUTE_GRP_NODE_ST *pstRoute)
{
    if (pstRoute)
    {
        dos_memzero(pstRoute, sizeof(SC_ROUTE_GRP_NODE_ST));
        pstRoute->ulGroupID = U32_BUTT;

        pstRoute->ucHourBegin = 0;
        pstRoute->ucMinuteBegin = 0;
        pstRoute->ucHourEnd = 0;
        pstRoute->ucMinuteEnd = 0;

        DLL_Init(&pstRoute->stGWList);

        pthread_mutex_init(&pstRoute->mutexGWList, NULL);
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
    }
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
    U32 ulIndex;
    U32 ulHashIndex;

    ulIndex = 0;
    while(1)
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
    U32 ulIndex;
    U32 ulHashIndex;

    ulIndex = 0;
    while(1)
    {
        if ('\0' == pszDIDNum[ulIndex])
        {
            break;
        }

        ulHashIndex += (pszDIDNum[ulIndex] << 3);

        ulIndex++;
    }

    return ulHashIndex % SC_IP_USERID_HASH_SIZE;
}


S32 sc_load_sip_userid_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
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

    pstSIPUserIDNode = (SC_USER_ID_NODE_ST *)dos_dmem_alloc(sizeof(SC_USER_ID_NODE_ST));
    if (DOS_ADDR_INVALID(pstSIPUserIDNode))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    sc_ep_sip_userid_init(pstSIPUserIDNode);

    for (lIndex=0, blProcessOK=DOS_TRUE; lIndex<lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSIPUserIDNode->ulSIPID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSIPUserIDNode->ulCustomID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "username", dos_strlen("username")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstSIPUserIDNode->szUserID, aszValues[lIndex], sizeof(pstSIPUserIDNode->szUserID));
            pstSIPUserIDNode->szUserID[sizeof(pstSIPUserIDNode->szUserID) - 1] = '\0';
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "extension", dos_strlen("extension")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstSIPUserIDNode->szExtension, aszValues[lIndex], sizeof(pstSIPUserIDNode->szExtension));
            pstSIPUserIDNode->szExtension[sizeof(pstSIPUserIDNode->szExtension) - 1] = '\0';
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstSIPUserIDNode);
        pstSIPUserIDNode = NULL;
        return DOS_FALSE;
    }

    pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstSIPUserIDNode);
        pstSIPUserIDNode = NULL;
        return DOS_FALSE;
    }

    printf("Load SIP User. ID: %d, Customer: %d, UserID: %s, Extension: %s"
                , pstSIPUserIDNode->ulSIPID
                , pstSIPUserIDNode->ulCustomID
                , pstSIPUserIDNode->szUserID
                , pstSIPUserIDNode->szExtension);

    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = pstSIPUserIDNode;
    ulHashIndex = sc_sip_userid_hash_func(pstSIPUserIDNode->szUserID);

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    hash_add_node(g_pstHashSIPUserID, (HASH_NODE_S *)pstHashNode, ulHashIndex, NULL);
    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return DOS_SUCC;
}

U32 sc_load_sip_userid()
{
    S8 szSQL[1024] = { 0, };

    dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, extension,username FROM tbl_sip;");

    if (db_query(g_pstTaskMngtInfo->pstDBHandle, szSQL, sc_load_sip_userid_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error("%s", "Load sip account fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

S32 sc_load_did_number_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_DID_NODE_ST     *pstDIDNumNode = NULL;
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

    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstDIDNumNode);
        pstDIDNumNode = NULL;
        return DOS_FALSE;
    }

    pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstDIDNumNode);
        pstDIDNumNode = NULL;
        return DOS_FALSE;
    }

    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = pstDIDNumNode;
    ulHashIndex = sc_sip_did_hash_func(pstDIDNumNode->szDIDNum);

    pthread_mutex_lock(&g_mutexHashDIDNum);
    hash_add_node(g_pstHashDIDNum, (HASH_NODE_S *)pstHashNode, ulHashIndex, NULL);
    pthread_mutex_unlock(&g_mutexHashDIDNum);

    return DOS_SUCC;
}

U32 sc_load_did_number()
{
    S8 szSQL[1024] = { 0, };

    dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, did_number, bind_type, bind_id FROM tbl_sipassign;");

    db_query(g_pstTaskMngtInfo->pstDBHandle, szSQL, sc_load_did_number_cb, NULL, NULL);

    return DOS_SUCC;
}


S32 sc_load_gateway_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
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

    sc_ep_gw_init(pstGWNode);

    pstGWNode->ulGWID = ulID;
    if ('\0' == pszDomain[0])
    {
        dos_strncpy(pstGWNode->szGWDomain, pszDomain, sizeof(pstGWNode->szGWDomain));
        pstGWNode->szGWDomain[sizeof(pstGWNode->szGWDomain) - 1] = '\0';
    }
    else
    {
        pstGWNode->szGWDomain[0] = '\0';
    }

    DLL_Init_Node(pstListNode);
    pstListNode->pHandle = pstGWNode;

    pthread_mutex_lock(&pstRouetGroup->mutexGWList);
    DLL_Add(&pstRouetGroup->stGWList, pstListNode);
    pthread_mutex_unlock(&pstRouetGroup->mutexGWList);

    return DOS_SUCC;
}

U32 sc_load_gateway(SC_ROUTE_GRP_NODE_ST *pszGroupNode)
{
    S8 szSQL[1024];

    if (DOS_ADDR_INVALID(pszGroupNode))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, realm FROM tbl_relaygrp WHERE group_id = %d;"
                    , pszGroupNode->ulGroupID);

    db_query(g_pstTaskMngtInfo->pstDBHandle, szSQL, sc_load_gateway_cb, pszGroupNode, NULL);

    return DOS_SUCC;
}

S32 sc_load_route_group_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_ROUTE_GRP_NODE_ST *pstRoute      = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    S32                  lIndex;
    S32                  lSecond;
    S32                  lRet;
    BOOL                 blProcessOK = 0;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstRoute = dos_dmem_alloc(sizeof(SC_ROUTE_GRP_NODE_ST));
    if (DOS_ADDR_INVALID(pstRoute))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_ep_route_init(pstRoute);


    for (blProcessOK=DOS_TRUE, lIndex=0; lIndex<lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstRoute->ulGroupID) < 0)
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
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstRoute);
        return DOS_FAIL;
    }

    pstListNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstRoute);
        return DOS_FAIL;
    }

    DLL_Init_Node(pstListNode);
    pstListNode->pHandle = pstRoute;
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

    db_query(g_pstTaskMngtInfo->pstDBHandle, szSQL, sc_load_route_group_cb, NULL, NULL);

    pthread_mutex_lock(&g_mutexRouteGrpList);
    DLL_Scan(&g_stRouteGrpList, pstListNode, DLL_NODE_S *)
    {
        pstRouetGroup = (SC_ROUTE_GRP_NODE_ST *)pstListNode->pHandle;
        sc_load_gateway(pstRouetGroup);
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

        pstHandle->event_lock = 1;
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

            if (0 == dos_strnicmp(pstUserIDNode->szUserID, pszUserID, sizeof(pstUserIDNode->szUserID)))
            {
                dos_strncpy(pszExtension, pstUserIDNode->szExtension, ulLength);
                pszExtension[ulLength - 1] = '\0';
                pthread_mutex_unlock(&g_mutexHashSIPUserID);
                return DOS_SUCC;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashSIPUserID);
    return DOS_FAIL;
}

U32 sc_ep_get_custom_by_sip_userid(S8 *pszNum)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
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

            if (0 == dos_strnicmp(pstUserIDNode->szUserID, pszNum, sizeof(pstUserIDNode->szUserID)))
            {
                pthread_mutex_unlock(&g_mutexHashSIPUserID);
                return pstUserIDNode->ulCustomID;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashSIPUserID);
    return U32_BUTT;
}


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

    pthread_mutex_lock(&g_mutexHashDIDNum);
    HASH_Scan_Table(g_pstHashDIDNum, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashDIDNum, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstDIDNumNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstDIDNumNode))
            {
                continue;
            }

            if (0 == dos_strnicmp(pstDIDNumNode->szDIDNum, pszDidNum, sizeof(pstDIDNumNode->szDIDNum)))
            {
                *pulBindType = pstDIDNumNode->ulBindType;
                *pulBindID   = pstDIDNumNode->ulBindID;

                pthread_mutex_unlock(&g_mutexHashDIDNum);
                return DOS_SUCC;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashDIDNum);
    return DOS_FAIL;

}


U32 sc_ep_get_custom_by_did(S8 *pszNum)
{
    SC_DID_NODE_ST     *pstDIDNumNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    pthread_mutex_lock(&g_mutexHashDIDNum);
    HASH_Scan_Table(g_pstHashDIDNum, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashDIDNum, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstDIDNumNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstDIDNumNode))
            {
                continue;
            }

            if (0 == dos_strnicmp(pstDIDNumNode->szDIDNum, pszNum, sizeof(pstDIDNumNode->szDIDNum)))
            {
                pthread_mutex_unlock(&g_mutexHashDIDNum);
                return pstDIDNumNode->ulCustomID;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashDIDNum);
    return U32_BUTT;
}

U32 sc_ep_get_userid_by_id(U32 ulSIPUserID, S8 *pszUserID, U32 ulLength)
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

            if (ulSIPUserID == pstUserIDNode->ulSIPID)
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

        sc_logr_debug("Search Route: %d:%d, %d:%d, %s, %s\r\n"
                , pstRouetGroup->ucHourBegin, pstRouetGroup->ucMinuteBegin
                , pstRouetGroup->ucHourEnd, pstRouetGroup->ucMinuteEnd
                , pstRouetGroup->szCalleePrefix
                , pstRouetGroup->szCallerPrefix);

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

        if ('\0' == pstRouetGroup->szCalleePrefix[0])
        {
            if ('\0' == pstRouetGroup->szCallerPrefix[0])
            {
                ulRouteGrpID = pstRouetGroup->ulGroupID;
                break;
            }
            else
            {
                if (0 == dos_strnicmp(pstRouetGroup->szCalleePrefix, pstCCB->szCalleeNum, dos_strlen(pstRouetGroup->szCalleePrefix)))
                {
                    ulRouteGrpID = pstRouetGroup->ulGroupID;
                    break;
                }
            }
        }
        else
        {
            if ('\0' == pstRouetGroup->szCallerPrefix[0])
            {
                if (0 == dos_strnicmp(pstRouetGroup->szCallerPrefix, pstCCB->szCallerNum, dos_strlen(pstRouetGroup->szCallerPrefix)))
                {
                    ulRouteGrpID = pstRouetGroup->ulGroupID;
                    break;
                }
            }
            else
            {
                if (0 == dos_strnicmp(pstRouetGroup->szCalleePrefix, pstCCB->szCalleeNum, dos_strlen(pstRouetGroup->szCalleePrefix))
                    && 0 == dos_strnicmp(pstRouetGroup->szCallerPrefix, pstCCB->szCallerNum, dos_strlen(pstRouetGroup->szCallerPrefix)))
                {
                    ulRouteGrpID = pstRouetGroup->ulGroupID;
                    break;
                }
            }
        }
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
        sc_logr_notice("The destination is in black list. %s", pszDstNum);

        return SC_DIRECTION_INVALID;
    }

    if (0 == dos_strcmp(pszCallSource, "internal"))
    {
        /* IP测发起的呼叫，主叫一定为某SIP账户 */
        ulCustomID = sc_ep_get_custom_by_sip_userid(pszSrcNum);
        if (U32_BUTT == ulCustomID)
        {
            DOS_ASSERT(0);

            sc_logr_info("Source number %s is not invalid sip user id. Reject Call", pszSrcNum);
            return DOS_FAIL;
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
        ulCustomID = sc_ep_get_custom_by_did(pszDstNum);
        if (U32_BUTT == ulCustomID)
        {
            DOS_ASSERT(0);

            sc_logr_notice("The destination %s is not a DID number. Reject Call.", pszDstNum);
            return SC_DIRECTION_INVALID;
        }

        return SC_DIRECTION_SIP;
    }
}

U32 sc_ep_incoming_call_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    S8    *pszDstNum = NULL;
    S8    *pszUUID = NULL;
    U32   ulCustomerID = U32_BUTT;
    U32   ulBindType = U32_BUTT;
    U32   ulBindID = U32_BUTT;
    S8    szCallString[512] = { 0, };
    S8    szCallee[32] = { 0, };

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

    ulCustomerID = sc_ep_get_custom_by_did(pszDstNum);
    if (U32_BUTT != ulCustomerID)
    {
        if (sc_ep_get_bind_info4did(pszDstNum, &ulBindType, &ulBindID) != DOS_SUCC
            || ulBindType >=SC_DID_BIND_TYPE_BUTT
            || U32_BUTT == ulBindID)
        {
            DOS_ASSERT(0);

            sc_logr_info("Cannot get the bind info for the DID number %s, Reject Call.", pszDstNum);
            return DOS_FAIL;
        }

        switch (ulBindType)
        {
            case SC_DID_BIND_TYPE_SIP:
                if (DOS_SUCC != sc_ep_get_userid_by_id(ulBindID, szCallee, sizeof(szCallee)))
                {
                    DOS_ASSERT(0);

                    sc_logr_info("DID number %s seems donot bind a SIP User ID, Reject Call.", pszDstNum);
                    return DOS_FAIL;
                }

                dos_snprintf(szCallString, sizeof(szCallString), "user/%s", szCallee);

                sc_ep_esl_execute(pstHandle, "answer", "", pszUUID);
                sc_ep_esl_execute(pstHandle, "bridge", szCallString, pszUUID);
                sc_ep_esl_execute(pstHandle, "hangup", szCallString, pszUUID);
                break;
            case SC_DID_BIND_TYPE_QUEUE:
                /* @TODO 呼叫坐席 */
                break;
            default:
                DOS_ASSERT(0);

                sc_logr_info("DID number %s has bind an error number, Reject Call.", pszDstNum);
                return DOS_FAIL;
                break;
        }

        return DOS_SUCC;
    }
    else
    {
        DOS_ASSERT(0);

        sc_logr_info("Destination is not a DID number, Reject Call. Destination: %s", pszDstNum);
        return DOS_FAIL;

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
    sc_ep_esl_execute(pstHandle, "hangup", "", pstCCB->szUUID);

    SC_CCB_SET_STATUS(pstCCB, SC_CCB_ACTIVE);

    return DOS_SUCC;

}

U32 sc_ep_auto_dial(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    return sc_ep_outgoing_call_proc(pstEvent, pstHandle, pstCCB);
}


U32 sc_ep_internal_call_process(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    S8    *pszSrcNum = NULL;
    S8    *pszDstNum = NULL;
    S8    *pszUUID = NULL;
    S8    szSIPUserID[32] = { 0, };
    S8    szCallString[512] = { 0, };
    U32   ulCustomerID = U32_BUTT;
    U32   ulCustomerID1 = U32_BUTT;

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

    pszDstNum = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    pszSrcNum = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    if (DOS_ADDR_INVALID(pszDstNum)
        || DOS_ADDR_INVALID(pszSrcNum))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* 判断被叫号码是否是分机号，如果是分机号，就要找到对应的SIP账户，再呼叫，同时呼叫之前还需要获取主叫的分机号，修改ANI为主叫的分机号 */
    ulCustomerID = sc_ep_get_custom_by_sip_userid(pszSrcNum);
    if (U32_BUTT == ulCustomerID)
    {
        DOS_ASSERT(0);

        sc_logr_info("The source number %s seem not beyound to any customer, Reject Call", pszSrcNum);
        return DOS_FAIL;
    }

    ulCustomerID1 = sc_ep_get_custom_by_sip_userid(pszDstNum);
    if (U32_BUTT != ulCustomerID1)
    {
        if (ulCustomerID == ulCustomerID1)
        {
            dos_snprintf(szCallString, sizeof(szCallString), "user/%s", pszSrcNum);
            sc_ep_esl_execute(pstHandle, "bridge", szCallString, pszUUID);
            sc_ep_esl_execute(pstHandle, "hangup", NULL, pszUUID);
        }
        else
        {
            DOS_ASSERT(0);

            sc_logr_info("Cannot call other customer direct, Reject Call. Src %s is owned by customer %d, Dst %s is owned by customer %d"
                            , pszSrcNum, ulCustomerID, pszDstNum, ulCustomerID1);
            return DOS_FAIL;
        }
    }
    else
    {
        if (sc_ep_get_userid_by_extension(ulCustomerID, pszDstNum, szSIPUserID, sizeof(szSIPUserID)) != DOS_SUCC)
        {
            DOS_ASSERT(0);

            sc_logr_info("Destination number %s is not seems a SIP User ID or Extension. Reject Call", pszDstNum);
            return DOS_FAIL;
        }

        dos_snprintf(szCallString, sizeof(szCallString), "user/%s", szSIPUserID);
        sc_ep_esl_execute(pstHandle, "bridge", szCallString, pszUUID);
        sc_ep_esl_execute(pstHandle, "hangup", NULL, pszUUID);
    }

    return DOS_SUCC;

}


U32 sc_ep_internal_service_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    S8    *pszUUID = NULL;

    sc_logr_info("%s", "Start exec internal service.");

    pszUUID = esl_event_get_header(pstEvent, "Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_ep_esl_execute(pstHandle, "answer", NULL, pszUUID);
    sc_ep_esl_execute(pstHandle, "sleep", "1000", pszUUID);
    sc_ep_esl_execute(pstHandle, "speak", "flite|kal|Temporary not support.", pszUUID);
    sc_ep_esl_execute(pstHandle, "hangup", NULL, pszUUID);
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
    pszIsAutoCall = esl_event_get_header(pstEvent, "auto_call");
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
    S8          *pszAutoCall = NULL;
    S8          *pszCCBNum = NULL;
    SC_CCB_ST   *pstCCB = NULL;
    S8          szBuffCmd[128] = { 0 };
    U32         ulCCBNo;
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

    pszAutoCall = esl_event_get_header(pstEvent, "variable_auto_call");
    if (DOS_ADDR_VALID(pszAutoCall))
    {
        if (dos_strnicmp(pszAutoCall, "true", dos_strlen("true")))
        {
            DOS_ASSERT(0);

            goto process_fail;
        }


        pszCCBNum = esl_event_get_header(pstEvent, "variable_ccb_number");
        if (DOS_ADDR_INVALID(pszCCBNum)
            && dos_atoul(pszCCBNum, &ulCCBNo) < 0)
        {
            DOS_ASSERT(0);

            goto process_fail;
        }

        pstCCB = sc_ccb_get(ulCCBNo);
        if (DOS_ADDR_INVALID(pstCCB))
        {
            DOS_ASSERT(0);

            goto process_fail;
        }

        /* 更新UUID */
        dos_strncpy(pstCCB->szUUID, pszUUID, sizeof(pstCCB->szUUID));
        pstCCB->szUUID[sizeof(pstCCB->szUUID) - 1] = '\0';

process_fail:
       ulRet = DOS_FAIL;
    }
    else
    {
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
    }

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
    S8                   *pszIsLoopbackLeg = NULL;
    S8                   *pszIsAutoCall = NULL;
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
            g_pstHandle->ulESLDebugLevel = ESL_LOG_LEVEL_INFO;
            esl_global_set_default_logger(g_pstHandle->ulESLDebugLevel);
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

        /* 如果是AUTO Call, 需要吧loopback call的leg a丢掉 */
        pszIsLoopbackLeg = esl_event_get_header(pstEvent, "variable_loopback_leg");
        pszIsAutoCall = esl_event_get_header(pstEvent, "variable_auto_call");
        if (pszIsLoopbackLeg && pszIsAutoCall
            && 0 == dos_strnicmp(pszIsLoopbackLeg, "A", dos_strlen("A"))
            && 0 == dos_strnicmp(pszIsAutoCall, "true", dos_strlen("true")))
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
    DLL_Init(&g_stRouteGrpList);

    g_pstHashDIDNum = hash_create_table(SC_IP_DID_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstHashDIDNum))
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    g_pstHashSIPUserID = hash_create_table(SC_IP_USERID_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstHashSIPUserID))
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_load_route_group();
    sc_load_did_number();
    sc_load_sip_userid();

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


