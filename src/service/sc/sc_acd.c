/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_acd.c
 *
 *  创建时间: 2015年1月14日14:42:07
 *  作    者: Larry
 *  描    述: ACD模块相关功能函数实现
 *  修改历史:
 *
 * 坐席管理在内存中的结构
 * 1. 由g_pstSiteList hash表维护所有的坐席
 * 2. 由g_pstGroupList 维护所有的组，同时在组里面维护每个组成员的hash表
 * 3. 组成员的Hash表中有指向g_pstSiteList中坐席成员的指针
 *  g_pstSiteList
 *    | -- table --- buget1 --- site1 --- site2 --- site3 ...
 *           |   --- buget2 --- site4 --- site5 --- site6 ...
 *
 *  g_pstGroupList
 *    group table --- buget1 --- group1(site table1) --- buget1 --- site1 --- site2 ...
 *            |                            |         --- buget2 --- site3 --- site4 ...
 *            |                  group2(site table1) --- buget1 --- site1 --- site2 ...
 *            |                            |         --- buget2 --- site3 --- site4 ...
 *            |   --- buget2 --- group3(site table1) --- buget1 --- site1 --- site2 ...
 *            |                            |         --- buget2 --- site3 --- site4 ...
 *            |                  group4(site table1) --- buget1 --- site1 --- site2 ...
 *            |                            |         --- buget2 --- site3 --- site4 ...
 *  其中g_pstGroupList table中所有的site使用指向g_pstSiteList中某一个坐席,
 *  同一个坐席可能属于多个坐席组，所以可能g_pstGroupList中有多个坐席指向g_pstSiteList中同一个节点，
 *  所以删除时不能直接delete
 */

#include <dos.h>
#include "sc_pub.h"
#include "sc_task_pub.h"
#include "sc_debug.h"
#include "sc_acd_pub.h"
#include "sc_acd.h"

extern SC_TASK_MNGT_ST   *g_pstTaskMngtInfo;


/* 坐席组的hash表 */
HASH_TABLE_S      *g_pstSiteList      = NULL;
pthread_mutex_t   g_mutexSiteList     = PTHREAD_MUTEX_INITIALIZER;

HASH_TABLE_S      *g_pstGroupList     = NULL;
pthread_mutex_t   g_mutexGroupList    = PTHREAD_MUTEX_INITIALIZER;


extern U32       g_ulTaskTraceAll;

/* 坐席组个数 */
U32               g_ulGroupCount    = 0;

/*
 * 函  数: sc_acd_hash_func4site
 * 功  能: 坐席的hash函数，通过分机号计算一个hash值
 * 参  数:
 *         S8 *pszExension  : 分机号
 *         U32 *pulHashIndex: 输出参数，计算之后的hash值
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
static U32 sc_acd_hash_func4site(S8 *pszExension, U32 *pulHashIndex)
{
    U32 ulHashVal, i;
    S8  *pszStr = NULL;

    SC_TRACE_IN(pszExension, pulHashIndex, 0, 0);

    if (DOS_ADDR_INVALID(pszExension) || DOS_ADDR_INVALID(pulHashIndex))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }


    ulHashVal = 0;

    for (i = 0; i < dos_strlen(pszStr); i ++)
    {
        ulHashVal += (ulHashVal << 5) + (U8)pszStr[i];
    }

    *pulHashIndex = ulHashVal % SC_ACD_HASH_SIZE;
    SC_TRACE_OUT();
    return DOS_SUCC;
}

/*
 * 函  数: sc_acd_hash_func4grp
 * 功  能: 坐席组的hash函数，通过分机号计算一个hash值
 * 参  数:
 *         U32 ulGrpID  : 坐席组ID
 *         U32 *pulHashIndex: 输出参数，计算之后的hash值
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
static U32 sc_acd_hash_func4grp(U32 ulGrpID, U32 *pulHashIndex)
{

    SC_TRACE_IN(ulGrpID, pulHashIndex, 0, 0);

    SC_TRACE_OUT();

    return ulGrpID % SC_ACD_HASH_SIZE;
}

/*
 * 函  数: sc_acd_grp_hash_find
 * 功  能: 坐席组的hash查找函数
 * 参  数:
 *         VOID *pSymName  : 坐席组ID
 *         HASH_NODE_S *pNode: HASH节点
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
static S32 sc_acd_grp_hash_find(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGrpHashNode = NULL;
    U32                        ulIndex;

    pstGrpHashNode = (SC_ACD_GRP_HASH_NODE_ST  *)pNode;
    ulIndex = (U32)*((U32 *)pSymName);

    if (DOS_ADDR_VALID(pstGrpHashNode)
        && DOS_ADDR_VALID(pSymName)
        && pstGrpHashNode->ulGroupID == ulIndex)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/*
 * 函  数: sc_acd_site_hash_find
 * 功  能: 坐席的hash查找函数
 * 参  数:
 *         VOID *pSymName  : 坐席分机号
 *         HASH_NODE_S *pNode: HASH节点
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
static S32 sc_acd_site_hash_find(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_ACD_SITE_HASH_NODE_ST   *pstSiteHashNode = NULL;
    S8                         *pszExtensition = NULL;

    pstSiteHashNode = (SC_ACD_SITE_HASH_NODE_ST  *)pNode;
    pszExtensition = (S8 *)pSymName;

    if (DOS_ADDR_INVALID(pstSiteHashNode)
        || DOS_ADDR_INVALID(pszExtensition)
        || DOS_ADDR_INVALID(pstSiteHashNode->pstSiteInfo))
    {
        return DOS_FAIL;
    }

    if (0 != dos_strnicmp(pstSiteHashNode->pstSiteInfo->szExtension, pszExtensition, sizeof(pstSiteHashNode->pstSiteInfo->szExtension)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/*
 * 函  数: sc_acd_add_site
 * 功  能: 添加坐席
 * 参  数:
 *         SC_ACD_SITE_DESC_ST *pstSiteInfo, 坐席信息描述
 *         U32 ulGrpID 所属组
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_acd_add_site(SC_ACD_SITE_DESC_ST *pstSiteInfo, U32 ulGrpID)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode = NULL;
    SC_ACD_SITE_HASH_NODE_ST   *pstSiteListNode = NULL;
    SC_ACD_SITE_DESC_ST        *pstSiteData = NULL;
    U32                        ulHashVal = 0;

    SC_TRACE_IN(pstSiteInfo, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstSiteInfo))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 确定队列 */
    sc_acd_hash_func4grp(ulGrpID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstGroupListNode = (SC_ACD_GRP_HASH_NODE_ST *)hash_find_node(g_pstGroupList, ulHashVal , &ulGrpID, sc_acd_grp_hash_find);
    pthread_mutex_unlock(&g_mutexGroupList);
    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);

        sc_acd_trace_error("Cannot find the group \"%d\" for the site %s.", ulGrpID, pstSiteInfo->szExtension);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstSiteListNode = (SC_ACD_SITE_HASH_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_SITE_HASH_NODE_ST));
    if (DOS_ADDR_INVALID(pstSiteListNode))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstSiteData = (SC_ACD_SITE_DESC_ST *)dos_dmem_alloc(sizeof(SC_ACD_SITE_DESC_ST));
    if (DOS_ADDR_INVALID(pstSiteData))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 加入坐席管理hash表 */
    sc_acd_hash_func4site(pstSiteInfo->szExtension, &ulHashVal);
    pthread_mutex_lock(&g_mutexSiteList);
    hash_add_node(g_pstSiteList, (HASH_NODE_S *)pstSiteListNode, ulHashVal, NULL);
    pthread_mutex_unlock(&g_mutexSiteList);

    /* 添加到队列 */
    sc_acd_hash_func4grp(ulGrpID, &ulHashVal);
    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);
    dos_memcpy(pstSiteData, pstSiteInfo, sizeof(pstSiteInfo));
    pstSiteListNode->pstSiteInfo = pstSiteData;
    pstSiteListNode->ulID = pstGroupListNode->usCount;
    pstGroupListNode->usCount++;
    hash_add_node(pstGroupListNode->pstSiteQueue, (HASH_NODE_S *)pstSiteListNode, ulHashVal, NULL);
    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    SC_TRACE_OUT();
    return DOS_SUCC;
}


/*
 * 函  数: sc_acd_grp_wolk4delete_site
 * 功  能: 从某一个坐席组里面删除坐席
 * 参  数:
 *         HASH_NODE_S *pNode : 当前节点
 *         VOID *pszExtensition : 被删除坐席的分机号
 * 返回值: VOID
 **/
static VOID sc_acd_grp_wolk4delete_site(HASH_NODE_S *pNode, VOID *pszExtensition)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode = NULL;
    SC_ACD_SITE_HASH_NODE_ST   *pstSiteListNode = NULL;
    U32                        ulHashVal;

    if (DOS_ADDR_INVALID(pNode) || DOS_ADDR_INVALID(pszExtensition))
    {
        return;
    }

    pstGroupListNode = (SC_ACD_GRP_HASH_NODE_ST *)pNode;
    sc_acd_hash_func4site(pszExtensition, &ulHashVal);
    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);
    pstSiteListNode = (SC_ACD_SITE_HASH_NODE_ST *)hash_find_node(pstGroupListNode->pstSiteQueue, ulHashVal, pszExtensition, sc_acd_site_hash_find);
    if (DOS_ADDR_INVALID(pstSiteListNode))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);
        return;
    }

    /* 这个地方先不要free，有可能别的队列也有这个作息 */
    pstSiteListNode->pstSiteInfo = NULL;

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);
}

U32 sc_acd_delete_site(S8 *pszExenstion)
{
    SC_ACD_SITE_HASH_NODE_ST   *pstSiteListNode = NULL;
    U32                        ulHashVal = 0;

    SC_TRACE_IN(pszExenstion, 0, 0, 0);

    if (DOS_ADDR_INVALID(pszExenstion))
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 遍历所有组，并删除相关坐席 */
    pthread_mutex_lock(&g_mutexGroupList);
    hash_walk_table(g_pstGroupList, pszExenstion, sc_acd_grp_wolk4delete_site);
    pthread_mutex_unlock(&g_mutexGroupList);

    /* 做到坐席，然后将其值为删除状态 */
    pthread_mutex_lock(&g_mutexSiteList);
    sc_acd_hash_func4site(pszExenstion, &ulHashVal);
    pstSiteListNode = (SC_ACD_SITE_HASH_NODE_ST *)hash_find_node(g_pstSiteList, ulHashVal, pszExenstion, sc_acd_site_hash_find);
    if (DOS_ADDR_INVALID(pstSiteListNode))
    {
        DOS_ASSERT(0);

        sc_acd_trace_error("Connot find the Site \"%s\" while delete", pszExenstion);
        pthread_mutex_unlock(&g_mutexSiteList);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstSiteListNode->pstSiteInfo->bWaitingDelete = DOS_TRUE;
    pthread_mutex_unlock(&g_mutexSiteList);

    return DOS_SUCC;
}


U32 sc_acd_update_site_status(S8 *pszExenstion, U32 ulGrpID, U32 ulAction)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode = NULL;
    SC_ACD_SITE_HASH_NODE_ST   *pstSiteListNode = NULL;
    U32                        ulHashVal = 0;

    SC_TRACE_IN(pszExenstion, ulGrpID, ulAction, 0);

    if (DOS_ADDR_INVALID(pszExenstion))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (ulAction >= SC_ACD_SITE_ACTION_BUTT)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 确定队列 */
    sc_acd_hash_func4grp(ulGrpID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstGroupListNode = (SC_ACD_GRP_HASH_NODE_ST *)hash_find_node(g_pstGroupList, ulHashVal , &ulGrpID, sc_acd_grp_hash_find);
    pthread_mutex_unlock(&g_mutexGroupList);
    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);

        sc_acd_trace_error("Cannot find the group \"%d\" for the site %s.", ulGrpID, pszExenstion);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_acd_hash_func4site(pszExenstion, &ulHashVal);
    pthread_mutex_lock(&g_mutexSiteList);
    pstSiteListNode = (SC_ACD_SITE_HASH_NODE_ST *)hash_find_node(g_pstGroupList, ulHashVal , &pszExenstion, sc_acd_site_hash_find);
    if (DOS_ADDR_VALID(pstSiteListNode)
        && DOS_ADDR_VALID(pstSiteListNode->pstSiteInfo))
    {
        pthread_mutex_lock(&pstSiteListNode->pstSiteInfo->mutexLock);
        switch (ulAction)
        {
            case SC_ACD_SITE_ACTION_DELETE:
                pstSiteListNode->pstSiteInfo->bWaitingDelete = DOS_TRUE;
                break;
            case SC_ACD_SITE_ACTION_ONLINE:
                pstSiteListNode->pstSiteInfo->bLogin = DOS_TRUE;
                pstSiteListNode->pstSiteInfo->bConnected = DOS_FALSE;
                break;
            case SC_ACD_SITE_ACTION_OFFLINE:
                pstSiteListNode->pstSiteInfo->bLogin = DOS_TRUE;
                pstSiteListNode->pstSiteInfo->bConnected = DOS_FALSE;
                break;
            case SC_ACD_SITE_ACTION_SIGNIN:
                pstSiteListNode->pstSiteInfo->bConnected = DOS_TRUE;
                pstSiteListNode->pstSiteInfo->bConnected = DOS_FALSE;
                break;
            case SC_ACD_SITE_ACTION_SIGNOUT:
                pstSiteListNode->pstSiteInfo->bConnected = DOS_FAIL;
                pstSiteListNode->pstSiteInfo->bConnected = DOS_FALSE;
                break;
            case SC_ACD_SITE_ACTION_EN_QUEUE:
                pstSiteListNode->pstSiteInfo->bConnected = DOS_TRUE;
                pstSiteListNode->pstSiteInfo->bConnected = DOS_TRUE;
                break;
            case SC_ACD_SITE_ACTION_DN_QUEUE:
                pstSiteListNode->pstSiteInfo->bConnected = DOS_TRUE;
                pstSiteListNode->pstSiteInfo->bConnected = DOS_FALSE;
                break;
            default:
                DOS_ASSERT(0);
                break;
        }
        pthread_mutex_unlock(&pstSiteListNode->pstSiteInfo->mutexLock);
    }
    pthread_mutex_unlock(&g_mutexSiteList);

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_acd_http_req_proc(U32 ulAction, U32 ulGrpID, U32 ulSiteID, S8 *pszExtension)
 * 功能: 处理HTTP发过来的命令
 * 参数:
 *      U32 ulAction : 命令
 *      U32 ulGrpID : 坐席所在组ID
 *      U32 ulSiteID : 坐席ID
 *      S8 *pszExtension : 坐席分机号
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
U32 sc_acd_http_req_proc(U32 ulAction, U32 ulGrpID, U32 ulSiteID, S8 *pszExtension)
{
    if (DOS_ADDR_INVALID(pszExtension))
    {
        return DOS_FAIL;
    }

    switch (ulAction)
    {
        case SC_ACD_SITE_ACTION_DELETE:
        case SC_ACD_SITE_ACTION_SIGNIN:
        case SC_ACD_SITE_ACTION_SIGNOUT:
        case SC_ACD_SITE_ACTION_ONLINE:
        case SC_ACD_SITE_ACTION_OFFLINE:
        case SC_ACD_SITE_ACTION_EN_QUEUE:
        case SC_ACD_SITE_ACTION_DN_QUEUE:
            sc_acd_update_site_status(pszExtension, ulGrpID, ulAction);
            break;
        case SC_ACD_SITE_ACTION_ADD:
        case SC_ACD_SITE_ACTION_UPDATE:
            break;
        default:
            DOS_ASSERT(0);
            return DOS_FAIL;
            break;
    }

    return DOS_SUCC;
}


U32 sc_acd_add_queue(U32 ulGroupID, U32 ulCustomID, U32 ulPolicy, S8 *pszGroupName)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode = NULL;
    U32                        ulHashVal = 0;

    SC_TRACE_IN(ulGroupID, ulCustomID, ulPolicy, pszGroupName);

    if (0 == ulGroupID
        || 0 == ulCustomID
        || ulPolicy >= SC_ACD_POLICY_BUTT)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 确定队列 */
    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstGroupListNode = (SC_ACD_GRP_HASH_NODE_ST *)hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_VALID(pstGroupListNode))
    {
        DOS_ASSERT(0);

        sc_acd_trace_error("Group \"%d\" Already in the list.", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    pthread_mutex_unlock(&g_mutexGroupList);

    pstGroupListNode = (SC_ACD_GRP_HASH_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_GRP_HASH_NODE_ST));
    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);

        sc_acd_trace_error("%s", "Add group fail. Alloc memory fail");

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstGroupListNode->pstSiteQueue = hash_create_table(SC_ACD_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);

        sc_acd_trace_error("%s", "Add group fail. Alloc memory fail");
        dos_dmem_free(pstGroupListNode);
        pstGroupListNode = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_init(&pstGroupListNode->mutexSiteQueue, NULL);
    pstGroupListNode->ulCustomID = ulCustomID;
    pstGroupListNode->ulGroupID  = ulGroupID;
    pstGroupListNode->ucACDPolicy = (U8)ulPolicy;
    pstGroupListNode->usCount = 0;
    pstGroupListNode->usID = (U16)g_ulGroupCount;
    if (pszGroupName[0] != '\0')
    {
        dos_strncpy(pstGroupListNode->szGroupName, pszGroupName, sizeof(pstGroupListNode->szGroupName));
        pstGroupListNode->szGroupName[sizeof(pstGroupListNode->szGroupName) - 1] = '\0';
    }

    pthread_mutex_lock(&g_mutexGroupList);
    hash_add_node(g_pstGroupList, (HASH_NODE_S *)pstGroupListNode, ulHashVal, NULL);
    pthread_mutex_unlock(&g_mutexGroupList);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_acd_delete_queue(U32 ulGroupID)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode = NULL;
    U32                        ulHashVal = 0;

    SC_TRACE_IN(ulGroupID, 0, 0, 0);

    if (0 == ulGroupID)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 确定队列 */
    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstGroupListNode = (SC_ACD_GRP_HASH_NODE_ST *)hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);

        sc_acd_trace_error("Connot find the Group \"%d\".", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);
    pstGroupListNode->ucWaitingDelete = DOS_TRUE;
    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);
    pthread_mutex_unlock(&g_mutexGroupList);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

static VOID sc_acd_site_hash_wolk4acd(HASH_NODE_S * pNode, VOID * pSymName)
{
    SC_ACD_SITE_HASH_NODE_ST   *pstSiteHashNode = NULL;
    SC_ACD_FIND_SITE_PARAM_ST  *pstFindParam   = NULL;

    pstSiteHashNode = (SC_ACD_SITE_HASH_NODE_ST *)pNode;
    pstFindParam    = (SC_ACD_FIND_SITE_PARAM_ST  *)pSymName;
    if (DOS_ADDR_INVALID(pstSiteHashNode)
        || DOS_ADDR_INVALID(pstFindParam)
        || DOS_ADDR_INVALID(pstSiteHashNode->pstSiteInfo))
    {
        return;
    }

    if (DOS_SUCC == pstFindParam->pstResult)
    {
        return;
    }

    switch (pstFindParam->ulPolocy)
    {
        case SC_ACD_POLICY_INCREASE:
            break;
        case SC_ACD_POLICY_INCREASE_LOOP:
            break;
        case SC_ACD_POLICY_REDUCTION:
            break;
        case SC_ACD_POLICY_REDUCTION_LOOP:
            break;
        case SC_ACD_POLICY_MIN_CALL:
            break;
        default:
            break;
    }

    if (SC_ACD_SITE_IS_USEABLE(pstSiteHashNode->pstSiteInfo))
    {
        pstFindParam->pstResult = pstSiteHashNode->pstSiteInfo;
        pstFindParam->ulResult  = DOS_SUCC;
    }

    return;
}

SC_ACD_SITE_DESC_ST  *sc_acd_get_site_by_grpid(U32 ulGroupID)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode = NULL;
    SC_ACD_FIND_SITE_PARAM_ST  stFindParam;
    U32                        ulHashVal = 0;

    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstGroupListNode = (SC_ACD_GRP_HASH_NODE_ST *)hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);

        sc_acd_trace_error("Cannot fine the group with the ID \"%s\".", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        SC_TRACE_OUT();
        return NULL;
    }
    pthread_mutex_unlock(&g_mutexGroupList);

    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);
    stFindParam.pstResult = NULL;
    stFindParam.ulPolocy = pstGroupListNode->ucACDPolicy;
    stFindParam.ulLastSieUsed = pstGroupListNode->usLastUsedSite;
    hash_walk_table(pstGroupListNode->pstSiteQueue, (VOID *)&stFindParam, sc_acd_site_hash_wolk4acd);
    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    if (DOS_SUCC != stFindParam.ulResult
        || NULL == stFindParam.pstResult)
    {
        return NULL;
    }

    return stFindParam.pstResult;
}

static S32 sc_acd_init_site_queue_cb(VOID *PTR, S32 lCount, S8 **pszData, S8 **pszField)
{
    SC_ACD_SITE_DESC_ST        stSiteInfo;
    U32 ulSiteID, ulCustomID, ulGroupID, ulRecordFlag, ulIsHeader;
    S8  *pszSiteID, *pszCustomID, *pszGroupID1, *pszGroupID2
        , *pszExten, *pszGroupID, *pszJobNum, *pszUserID, *pszRecordFlag
        , *pszIsHeader;

    pszSiteID = pszData[0];
    pszCustomID = pszData[1];
    pszJobNum = pszData[2];
    pszUserID = pszData[3];
    pszExten = pszData[4];
    pszGroupID1 = pszData[5];
    pszGroupID2 = pszData[6];
    pszGroupID = pszData[7];
    pszRecordFlag = pszData[8];
    pszIsHeader = pszData[9];

    if (DOS_ADDR_INVALID(pszSiteID)
        || DOS_ADDR_INVALID(pszCustomID)
        || DOS_ADDR_INVALID(pszJobNum)
        || DOS_ADDR_INVALID(pszUserID)
        || DOS_ADDR_INVALID(pszExten)
        || DOS_ADDR_INVALID(pszGroupID)
        || DOS_ADDR_INVALID(pszRecordFlag)
        || DOS_ADDR_INVALID(pszIsHeader)
        || dos_atoul(pszSiteID, &ulSiteID) < 0
        || dos_atoul(pszCustomID, &ulCustomID) < 0
        || dos_atoul(pszGroupID, &ulGroupID) < 0
        || dos_atoul(pszRecordFlag, &ulRecordFlag) < 0
        || dos_atoul(pszIsHeader, &ulIsHeader) < 0)
    {
        return 0;
    }

    dos_memzero(&stSiteInfo, sizeof(stSiteInfo));
    stSiteInfo.ulSiteID = ulSiteID;
    stSiteInfo.ulCustomerID = ulCustomID;
    stSiteInfo.aulGroupID[0] = ulGroupID;
    stSiteInfo.bValid = DOS_TRUE;
    stSiteInfo.bRecord = ulRecordFlag;
    stSiteInfo.bGroupHeader = ulIsHeader;

    dos_strncpy(stSiteInfo.szUserID, pszUserID, sizeof(stSiteInfo.szUserID));
    stSiteInfo.szUserID[sizeof(stSiteInfo.szUserID) - 1] = '\0';
    dos_strncpy(stSiteInfo.szExtension, pszExten, sizeof(stSiteInfo.szExtension));
    stSiteInfo.szExtension[sizeof(stSiteInfo.szExtension) - 1] = '\0';
    dos_strncpy(stSiteInfo.szEmpNo, pszExten, sizeof(stSiteInfo.szEmpNo));
    stSiteInfo.szEmpNo[sizeof(stSiteInfo.szEmpNo) - 1] = '\0';
    pthread_mutex_init(&stSiteInfo.mutexLock, NULL);

    return sc_acd_add_site(&stSiteInfo, ulGroupID);
}

static U32 sc_acd_init_site_queue()
{
    S8 szSQL[1024] = { 0, };

    dos_snprintf(szSQL, sizeof(szSQL)
                    ,"SELECT "
                     "    a.id, a.customer_id, a.job_number, a.username, a.extension, a.group1_id, a.group2_id, b.id, a.voice_record, a.class class"
                     "from "
                     "    (SELECT "
                     "         tbl_agent.id id, tbl_agent.customer_id customer_id, tbl_agent.job_number job_number, "
                     "         tbl_agent.group1_id group1_id, tbl_agent.group2_id group2_id, tbl_sip.extension extension, "
                     "         tbl_sip.username username, tbl_agent.voice_record voice_record, tbl_agent.class class"
                     "     FROM "
                     "         tbl_agent, tbl_sip "
                     "     WHERE tbl_agent.sip_id = tbl_sip.id) a "
                     "LEFT JOIN "
                     "    tbl_group b "
                     "ON "
                     "    a.group1_id = b.id OR a.group2_id = b.id;");

    if (db_query(g_pstTaskMngtInfo->pstDBHandle, szSQL, sc_acd_init_site_queue_cb, NULL, NULL) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static S32 sc_acd_init_group_queue_cb(VOID *PTR, S32 lCount, S8 **pszData, S8 **pszField)
{
    U32 ulGroupID = 0, ulCustomID = 0, ulPolicy = 0;
    S8  *pszGroupID = NULL, *pszCustomID = NULL, *pszPolicy = NULL, *pszGroupName = NULL;

    if (DOS_ADDR_INVALID(pszField) || DOS_ADDR_INVALID(pszData))
    {
        return 0;
    }

    pszGroupID = pszData[0];
    pszCustomID = pszData[1];
    pszPolicy = pszData[2];
    pszGroupName = pszData[3];

    if (dos_atoul(pszGroupID, &ulGroupID) < 0
        || dos_atoul(pszCustomID, &ulCustomID) < 0
        || dos_atoul(pszPolicy, &ulPolicy) < 0
        || DOS_ADDR_INVALID(pszGroupName))
    {
        return 0;
    }

    return sc_acd_add_queue(ulGroupID, ulCustomID, ulPolicy,pszGroupName);
}

static U32 sc_acd_init_group_queue()
{
    S8 szSql[1024] = { 0, };

    dos_snprintf(szSql, sizeof(szSql), "SELECT id,customer_id,acd_policy,`name` from tbl_group;");

    if (db_query(g_pstTaskMngtInfo->pstDBHandle, szSql, sc_acd_init_group_queue_cb, NULL, NULL) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static U32 sc_acd_deinit_site_queue()
{
    return DOS_SUCC;
}

static U32 sc_acd_deinit_group_queue()
{
    return DOS_SUCC;
}

static VOID sc_ack_site_wolk4init(HASH_NODE_S *pNode, VOID *pParam)
{
    SC_ACD_GRP_HASH_NODE_ST  *pstGroupNode = NULL;
    SC_ACD_SITE_HASH_NODE_ST *pstSiteNode = NULL, *pstSiteNodeNew = NULL;
    U32 ulHashVal = 0, ulIndex = 0;

    pstSiteNode = (SC_ACD_SITE_HASH_NODE_ST *)pNode;
    if (DOS_ADDR_INVALID(pstSiteNode)
        || DOS_ADDR_INVALID(pstSiteNode->pstSiteInfo))
    {
        return;
    }

    for (ulIndex=0; ulIndex<MAX_GROUP_PER_SITE; ulIndex++)
    {
        if (0 != pstSiteNode->pstSiteInfo->aulGroupID[ulIndex]
            && U32_BUTT != pstSiteNode->pstSiteInfo->aulGroupID[ulIndex])
        {
            sc_acd_hash_func4grp(pstSiteNode->pstSiteInfo->aulGroupID[ulIndex], &ulHashVal);
            pthread_mutex_lock(&g_mutexGroupList);
            pstGroupNode = (SC_ACD_GRP_HASH_NODE_ST *)hash_find_node(g_pstGroupList
                            , ulHashVal
                            , (VOID *)&pstSiteNode->pstSiteInfo->aulGroupID[ulIndex]
                            , sc_acd_grp_hash_find);
            if (DOS_ADDR_INVALID(pstGroupNode))
            {
                pthread_mutex_unlock(&g_mutexGroupList);
                continue;
            }

            pstSiteNodeNew = (SC_ACD_SITE_HASH_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_SITE_HASH_NODE_ST));
            if (DOS_ADDR_INVALID(pstSiteNodeNew))
            {
                pthread_mutex_unlock(&g_mutexGroupList);
                continue;
            }

            pstSiteNodeNew->pstSiteInfo = pstSiteNode->pstSiteInfo;

            pthread_mutex_lock(&pstGroupNode->mutexSiteQueue);
            pstSiteNodeNew->ulID = pstGroupNode->usCount;
            pstGroupNode->usCount++;
            hash_add_node(pstGroupNode->pstSiteQueue, (HASH_NODE_S *)pstSiteNodeNew, ulHashVal, NULL);
            pthread_mutex_unlock(&pstGroupNode->mutexSiteQueue);

            pthread_mutex_unlock(&g_mutexGroupList);
        }
    }
}

static U32 sc_acd_init_relationship()
{
    SC_TRACE_IN(0, 0, 0, 0);

    pthread_mutex_lock(&g_mutexSiteList);
    hash_walk_table(g_pstSiteList, NULL, sc_ack_site_wolk4init);
    pthread_mutex_unlock(&g_mutexSiteList);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_acd_init()
{
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstGroupList = hash_create_table(SC_ACD_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstGroupList))
    {
        DOS_ASSERT(0);
        sc_acd_trace_error("%s", "Init Group Hash Table Fail.");

        SC_TRACE_OUT();
        return DOS_FAIL;
    }


    g_pstSiteList = hash_create_table(SC_ACD_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstSiteList))
    {
        DOS_ASSERT(0);
        sc_acd_trace_error("%s", "Init Site Hash Table Fail.");

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (sc_acd_init_site_queue() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_acd_trace_error("%s", "Init sites list fail in ACD.");


        hash_delete_table(g_pstSiteList, NULL);
        g_pstSiteList = NULL;

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (sc_acd_init_group_queue() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_acd_trace_error("%s", "Init group list fail in ACD.");

        sc_acd_deinit_site_queue();

        hash_delete_table(g_pstSiteList, NULL);
        g_pstSiteList = NULL;

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;


        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (sc_acd_init_relationship() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_acd_trace_error("%s", "Init ACD Data FAIL.");

        sc_acd_deinit_site_queue();
        sc_acd_deinit_group_queue();

        hash_delete_table(g_pstSiteList, NULL);
        g_pstSiteList = NULL;

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

