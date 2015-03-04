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
 * 1. 由g_pstAgentList hash表维护所有的坐席
 * 2. 由g_pstGroupList 维护所有的组，同时在组里面维护每个组成员的hash表
 * 3. 组成员的Hash表中有指向g_pstSiteList中坐席成员的指针
 *  g_pstAgentList
 *    | -- table --- buget1 --- agnet1 --- agnet2 --- agnet3 ...
 *           |   --- buget2 --- agnet4 --- agnet5 --- agnet6 ...
 *
 *  g_pstGroupList
 *    group table --- buget1 --- group1(agnet table1)  --- agent list(DLL)
 *            |                  |
 *            |                  group2(agnet table1)  --- agent list(DLL)
 *            |                  |
 *            |                  ...
 *            |
 *            |   --- buget2 --- group3(agnet table1)  --- agent list(DLL)
 *            |                  |
 *            |                  ...
 *            |
 *            |                  group4(agnet table1)  --- agent list(DLL)
 *            |                  |
 *            |                  ...
 *            ...
 *  其中g_pstGroupList table中所有的agnet使用指向g_pstAgentList中某一个坐席,
 *  同一个坐席可能属于多个坐席组，所以可能g_pstGroupList中有多个坐席指向g_pstAgentList中同一个节点，
 *  所以删除时不能直接delete
 */

#include <dos.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"
#include "sc_acd.h"

extern DB_HANDLE_ST         *g_pstSCDBHandle;


/* 坐席组的hash表 */
HASH_TABLE_S      *g_pstAgentList      = NULL;
pthread_mutex_t   g_mutexAgentList     = PTHREAD_MUTEX_INITIALIZER;

HASH_TABLE_S      *g_pstGroupList      = NULL;
pthread_mutex_t   g_mutexGroupList     = PTHREAD_MUTEX_INITIALIZER;

/* 坐席组个数 */
U32               g_ulGroupCount       = 0;


/*
 * 函  数: U32 sc_acd_agent_update_status(SC_ACD_AGENT_INFO_ST *pstAgent, U32 ulStatus)
 * 功  能: 更新坐席状态
 * 参  数:
 *      SC_ACD_AGENT_INFO_ST *pstAgent,
 *      U32 ulStatus
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_acd_agent_update_status(SC_ACD_AGENT_INFO_ST *pstAgent, U32 ulStatus)
{
    if (DOS_ADDR_INVALID(pstAgent)
        || ulStatus < SC_ACD_BUTT)
    {
        return DOS_FAIL;
    }

    pthread_mutex_lock(&pstAgent->mutexLock);
    pstAgent->usStatus = (U16)ulStatus;
    pthread_mutex_unlock(&pstAgent->mutexLock);


    return DOS_SUCC;
}

/*
 * 函  数: sc_acd_hash_func4agent
 * 功  能: 坐席的hash函数，通过分机号计算一个hash值
 * 参  数:
 *         S8 *pszExension  : 分机号
 *         U32 *pulHashIndex: 输出参数，计算之后的hash值
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
static U32 sc_acd_hash_func4agent(S8 *pszUserID, U32 *pulHashIndex)
{
    U32   ulHashVal = 0;
    U32   i         = 0;
    S8    *pszStr   = NULL;

    SC_TRACE_IN(pszUserID, pulHashIndex, 0, 0);

    if (DOS_ADDR_INVALID(pszUserID) || DOS_ADDR_INVALID(pulHashIndex))
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
    if (DOS_ADDR_INVALID(pulHashIndex))
    {
        return DOS_FAIL;
    }

    SC_TRACE_IN(ulGrpID, pulHashIndex, 0, 0);

    *pulHashIndex = ulGrpID % SC_ACD_HASH_SIZE;

    SC_TRACE_OUT();
    return DOS_SUCC;
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
    U32                        ulIndex         = 0;

    pstGrpHashNode = (SC_ACD_GRP_HASH_NODE_ST  *)pNode->pHandle;
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
 * 函  数: sc_acd_agent_hash_find
 * 功  能: 坐席的hash查找函数
 * 参  数:
 *         VOID *pSymName  : 坐席分机号
 *         HASH_NODE_S *pNode: HASH节点
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
static S32 sc_acd_agent_hash_find(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    S8                           *pszUserID         = NULL;

    if (DOS_ADDR_INVALID(pSymName)
        || DOS_ADDR_INVALID(pNode))
    {
        return DOS_FAIL;
    }

    pstAgentQueueNode = pNode->pHandle;
    pszUserID = (S8 *)pSymName;

    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pszUserID)
        || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
    {
        return DOS_FAIL;
    }

    if (0 != dos_strnicmp(pstAgentQueueNode->pstAgentInfo->szUserID, pszUserID, sizeof(pstAgentQueueNode->pstAgentInfo->szExtension)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/*
 * 函  数: S32 sc_acd_site_dll_find(VOID *pSymName, DLL_NODE_S *pNode)
 * 功  能: 坐席的hash查找函数
 * 参  数:
 *         VOID *pSymName  : 坐席分机号
 *         DLL_NODE_S *pNode: 链表节点
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
static S32 sc_acd_agent_dll_find(VOID *pSymName, DLL_NODE_S *pNode)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    S8                           *pszUserID         = NULL;

    if (DOS_ADDR_INVALID(pSymName)
        || DOS_ADDR_INVALID(pNode))
    {
        return DOS_FAIL;
    }

    pstAgentQueueNode = pNode->pHandle;
    pszUserID = (S8 *)pSymName;

    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pszUserID)
        || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
    {
        return DOS_FAIL;
    }

    if (0 != dos_strnicmp(pstAgentQueueNode->pstAgentInfo->szUserID, pszUserID, sizeof(pstAgentQueueNode->pstAgentInfo->szExtension)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}


/*
 * 函  数: sc_acd_add_agent
 * 功  能: 添加坐席
 * 参  数:
 *         SC_ACD_AGENT_INFO_ST *pstAgentInfo, 坐席信息描述
 *         U32 ulGrpID 所属组
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 * !!!!!该函数会检测所要添加的坐席所在组是否存在，但不会将坐席添加到组!!!!!!
 **/
U32 sc_acd_add_agent(SC_ACD_AGENT_INFO_ST *pstAgentInfo, U32 ulGrpID)
{
    SC_ACD_GRP_HASH_NODE_ST      *pstGroupListNode   = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode  = NULL;
    SC_ACD_AGENT_INFO_ST         *pstAgentData       = NULL;
    HASH_NODE_S                  *pstHashNode        = NULL;
    U32                          ulHashVal           = 0;

    SC_TRACE_IN(pstAgentInfo, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 检测所在队列是否存在 */
    sc_acd_hash_func4grp(ulGrpID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGrpID, sc_acd_grp_hash_find);
    pthread_mutex_unlock(&g_mutexGroupList);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot find the group \"%d\" for the site %s.", ulGrpID, pstAgentInfo->szExtension);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstAgentQueueNode = (SC_ACD_AGENT_QUEUE_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_AGENT_QUEUE_NODE_ST));
    if (DOS_ADDR_INVALID(pstAgentQueueNode))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstAgentData = (SC_ACD_AGENT_INFO_ST *)dos_dmem_alloc(sizeof(SC_ACD_AGENT_INFO_ST));
    if (DOS_ADDR_INVALID(pstAgentData))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 加入坐席管理hash表 */
    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = pstAgentQueueNode;
    sc_acd_hash_func4agent(pstAgentInfo->szUserID, &ulHashVal);
    pthread_mutex_lock(&g_mutexAgentList);
    hash_add_node(g_pstAgentList, pstHashNode, ulHashVal, NULL);
    pthread_mutex_unlock(&g_mutexAgentList);


    /* 添加到队列 */
    dos_memcpy(pstAgentData, pstAgentInfo, sizeof(SC_ACD_AGENT_INFO_ST));
    pthread_mutex_init(&pstAgentData->mutexLock, NULL);
    pstAgentQueueNode->pstAgentInfo = pstAgentData;
    pstAgentQueueNode->ulID = pstGroupListNode->usCount;
    pstGroupListNode->usCount++;

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
static VOID sc_acd_grp_wolk4delete_agent(HASH_NODE_S *pNode, VOID *pszUserID)
{
    SC_ACD_GRP_HASH_NODE_ST      *pstGroupListNode  = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    DLL_NODE_S                   *pstDLLNode        = NULL;
    U32                          ulHashVal          = 0;

    if (DOS_ADDR_INVALID(pNode) || DOS_ADDR_INVALID(pszUserID))
    {
        return;
    }

    pstGroupListNode = (SC_ACD_GRP_HASH_NODE_ST *)pNode->pHandle;
    sc_acd_hash_func4agent(pszUserID, &ulHashVal);
    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);
    pstDLLNode = dll_find(&pstGroupListNode->stAgentList, (VOID *)pszUserID, sc_acd_agent_dll_find);
    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);
        return;
    }

    /* 这个地方先不要free，有可能别的队列也有这个作息 */
    pstAgentQueueNode = pstDLLNode->pHandle;
    pstAgentQueueNode->pstAgentInfo = NULL;

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);
}

U32 sc_acd_delete_agent(S8 *pszUserID)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    HASH_NODE_S                  *pstHashNode       = NULL;
    U32                          ulHashVal          = 0;

    SC_TRACE_IN(pszUserID, 0, 0, 0);

    if (DOS_ADDR_INVALID(pszUserID))
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 遍历所有组，并删除相关坐席 */
    pthread_mutex_lock(&g_mutexGroupList);
    hash_walk_table(g_pstGroupList, pszUserID, sc_acd_grp_wolk4delete_agent);
    pthread_mutex_unlock(&g_mutexGroupList);

    /* 做到坐席，然后将其值为删除状态 */
    pthread_mutex_lock(&g_mutexAgentList);
    sc_acd_hash_func4agent(pszUserID, &ulHashVal);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashVal, pszUserID, sc_acd_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Connot find the Site \"%s\" while delete", pszUserID);
        pthread_mutex_unlock(&g_mutexAgentList);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstAgentQueueNode = pstHashNode->pHandle;
    if (DOS_ADDR_VALID(pstAgentQueueNode->pstAgentInfo))
    {
        pstAgentQueueNode->pstAgentInfo->bWaitingDelete = DOS_TRUE;
    }
    pthread_mutex_unlock(&g_mutexAgentList);

    return DOS_SUCC;
}


U32 sc_acd_update_agent_status(U32 ulAction, U32 ulAgentID, S8 *pszUserID)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    HASH_NODE_S                  *pstHashNode       = NULL;
    U32                          ulHashVal          = 0;

    SC_TRACE_IN(pszUserID, ulAgentID, ulAction, 0);

    if (DOS_ADDR_INVALID(pszUserID))
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

    /* 找到坐席 */
    sc_acd_hash_func4agent(pszUserID, &ulHashVal);
    pthread_mutex_lock(&g_mutexAgentList);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashVal , &pszUserID, sc_acd_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&g_mutexAgentList);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstAgentQueueNode = pstHashNode->pHandle;
    if (DOS_ADDR_VALID(pstAgentQueueNode)
        && DOS_ADDR_VALID(pstAgentQueueNode->pstAgentInfo))
    {
        pthread_mutex_lock(&pstAgentQueueNode->pstAgentInfo->mutexLock);
        switch (ulAction)
        {
            case SC_ACD_SITE_ACTION_DELETE:
                pstAgentQueueNode->pstAgentInfo->bWaitingDelete = DOS_TRUE;
                break;
            case SC_ACD_SITE_ACTION_ONLINE:
                pstAgentQueueNode->pstAgentInfo->bLogin = DOS_TRUE;
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                break;
            case SC_ACD_SITE_ACTION_OFFLINE:
                pstAgentQueueNode->pstAgentInfo->bLogin = DOS_TRUE;
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                break;
            case SC_ACD_SITE_ACTION_SIGNIN:
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_TRUE;
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                break;
            case SC_ACD_SITE_ACTION_SIGNOUT:
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                break;
            case SC_ACD_SITE_ACTION_EN_QUEUE:
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_TRUE;
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_TRUE;
                break;
            case SC_ACD_SITE_ACTION_DN_QUEUE:
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_TRUE;
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                break;
            default:
                DOS_ASSERT(0);
                break;
        }
        pthread_mutex_unlock(&pstAgentQueueNode->pstAgentInfo->mutexLock);
    }
    pthread_mutex_unlock(&g_mutexAgentList);

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_acd_http_req_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID)
 * 功能: 处理HTTP发过来的命令
 * 参数:
 *      U32 ulAction : 命令
 *      U32 ulAgentID : 坐席ID
 *      S8 *pszUserID : 坐席SIP User ID
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
U32 sc_acd_http_req_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID)
{
    switch (ulAction)
    {
        case SC_ACD_SITE_ACTION_DELETE:
        case SC_ACD_SITE_ACTION_SIGNIN:
        case SC_ACD_SITE_ACTION_SIGNOUT:
        case SC_ACD_SITE_ACTION_ONLINE:
        case SC_ACD_SITE_ACTION_OFFLINE:
        case SC_ACD_SITE_ACTION_EN_QUEUE:
        case SC_ACD_SITE_ACTION_DN_QUEUE:
            sc_acd_update_agent_status(ulAction, ulAgentID, pszUserID);
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
    HASH_NODE_S                *pstHashNode      = NULL;
    U32                        ulHashVal         = 0;

    SC_TRACE_IN(ulGroupID, ulCustomID, ulPolicy, pszGroupName);

    if (0 == ulGroupID
        || 0 == ulCustomID
        || ulPolicy >= SC_ACD_POLICY_BUTT)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_logr_debug(SC_ACD, "Load Group. ID:%d, Customer:%d, Policy: %d, Name: %s", ulGroupID, ulCustomID, ulPolicy, pszGroupName);

    /* 确定队列 */
    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_VALID(pstHashNode)
        && DOS_ADDR_VALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Group \"%d\" Already in the list.", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    pthread_mutex_unlock(&g_mutexGroupList);

    pstGroupListNode = (SC_ACD_GRP_HASH_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_GRP_HASH_NODE_ST));
    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "%s", "Add group fail. Alloc memory fail");

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;
        sc_logr_error(SC_ACD, "%s", "Add group fail. Alloc memory for hash node fail");

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    HASH_Init_Node(pstHashNode);
    DLL_Init(&pstGroupListNode->stAgentList);
    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "%s", "Add group fail. Alloc memory fail");
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
    pstHashNode->pHandle = pstGroupListNode;
    hash_add_node(g_pstGroupList, pstHashNode, ulHashVal, NULL);
    pthread_mutex_unlock(&g_mutexGroupList);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_acd_delete_queue(U32 ulGroupID)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode = NULL;
    HASH_NODE_S                *pstHashNode      = NULL;
    U32                        ulHashVal         = 0;

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
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Connot find the Group \"%d\".", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstGroupListNode = pstHashNode->pHandle;
    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);
    pstGroupListNode->ucWaitingDelete = DOS_TRUE;
    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);
    pthread_mutex_unlock(&g_mutexGroupList);

    SC_TRACE_OUT();
    return DOS_SUCC;
}


SC_ACD_AGENT_INFO_ST  *sc_acd_get_agent_by_grpid(U32 ulGroupID)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode = NULL;
    SC_ACD_FIND_SITE_PARAM_ST  stFindParam;
    HASH_NODE_S                *pstHashNode = NULL;
    DLL_NODE_S                 *pstDLLNode = NULL;
    U32                        ulHashVal = 0;

    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot fine the group with the ID \"%s\".", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        SC_TRACE_OUT();
        return NULL;
    }
    pthread_mutex_unlock(&g_mutexGroupList);

    pstGroupListNode = pstHashNode->pHandle;
    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);
    stFindParam.pstResult = NULL;
    stFindParam.ulResult = DOS_FAIL;
    stFindParam.ulPolocy = pstGroupListNode->ucACDPolicy;
    stFindParam.ulLastSieUsed = pstGroupListNode->usLastUsedSite;
    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        /* @TODO */
    }
    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    if (DOS_SUCC != stFindParam.ulResult
        || NULL == stFindParam.pstResult)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    return stFindParam.pstResult;
}

static S32 sc_acd_init_agent_queue_cb(VOID *PTR, S32 lCount, S8 **pszData, S8 **pszField)
{
    SC_ACD_AGENT_INFO_ST        stSiteInfo;
    U32                         ulSiteID   = 0, ulCustomID   = 0, ulGroupID  = 0;
    U32                         ulGroupID1 = 0, ulRecordFlag = 0, ulIsHeader = 0;
    S8                          *pszSiteID     = NULL, *pszCustomID = NULL;
    S8                          *pszGroupID1   = NULL, *pszGroupID2 = NULL;
    S8                          *pszExten      = NULL, *pszGroupID  = NULL;
    S8                          *pszJobNum     = NULL, *pszUserID   = NULL;
    S8                          *pszRecordFlag = NULL, *pszIsHeader = NULL;

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
        || dos_atoul(pszGroupID1, &ulGroupID) < 0
        || dos_atoul(pszGroupID2, &ulGroupID1) < 0
        || dos_atoul(pszRecordFlag, &ulRecordFlag) < 0
        || dos_atoul(pszIsHeader, &ulIsHeader) < 0)
    {
        return 0;
    }

    dos_memzero(&stSiteInfo, sizeof(stSiteInfo));
    stSiteInfo.ulSiteID = ulSiteID;
    stSiteInfo.ulCustomerID = ulCustomID;
    stSiteInfo.aulGroupID[0] = ulGroupID;
    stSiteInfo.aulGroupID[1] = ulGroupID1;
    stSiteInfo.bValid = DOS_TRUE;
    stSiteInfo.usStatus = SC_ACD_IDEL;
    stSiteInfo.bRecord = ulRecordFlag;
    stSiteInfo.bGroupHeader = ulIsHeader;

    sc_logr_debug(SC_ACD, "Load Agent. ID: %d, Customer: %d, Group1: %d, Group2: %d", ulSiteID, ulCustomID, ulGroupID, ulGroupID1);

    dos_strncpy(stSiteInfo.szUserID, pszUserID, sizeof(stSiteInfo.szUserID));
    stSiteInfo.szUserID[sizeof(stSiteInfo.szUserID) - 1] = '\0';
    dos_strncpy(stSiteInfo.szExtension, pszExten, sizeof(stSiteInfo.szExtension));
    stSiteInfo.szExtension[sizeof(stSiteInfo.szExtension) - 1] = '\0';
    dos_strncpy(stSiteInfo.szEmpNo, pszExten, sizeof(stSiteInfo.szEmpNo));
    stSiteInfo.szEmpNo[sizeof(stSiteInfo.szEmpNo) - 1] = '\0';
    pthread_mutex_init(&stSiteInfo.mutexLock, NULL);

    return sc_acd_add_agent(&stSiteInfo, ulGroupID);
}

static U32 sc_acd_init_agent_queue()
{
    S8 szSQL[1024] = { 0, };

    dos_snprintf(szSQL, sizeof(szSQL)
                    ,"SELECT " \
                     "    a.id, a.customer_id, a.job_number, a.username, a.extension, a.group1_id, a.group2_id, b.id, a.voice_record, a.class class " \
                     "FROM " \
                     "    (SELECT " \
                     "         tbl_agent.id id, tbl_agent.customer_id customer_id, tbl_agent.job_number job_number, " \
                     "         tbl_agent.group1_id group1_id, tbl_agent.group2_id group2_id, tbl_sip.extension extension, " \
                     "         tbl_sip.username username, tbl_agent.voice_record voice_record, tbl_agent.class class" \
                     "     FROM " \
                     "         tbl_agent, tbl_sip " \
                     "     WHERE tbl_agent.sip_id = tbl_sip.id) a " \
                     "LEFT JOIN " \
                     "    tbl_group b " \
                     "ON " \
                     "    a.group1_id = b.id OR a.group2_id = b.id;");

    if (db_query(g_pstSCDBHandle, szSQL, sc_acd_init_agent_queue_cb, NULL, NULL) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static S32 sc_acd_init_group_queue_cb(VOID *PTR, S32 lCount, S8 **pszData, S8 **pszField)
{
    U32     ulGroupID = 0, ulCustomID = 0, ulPolicy = 0;
    S8      *pszGroupID = NULL, *pszCustomID = NULL, *pszPolicy = NULL, *pszGroupName = NULL;

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

    if (db_query(g_pstSCDBHandle, szSql, sc_acd_init_group_queue_cb, NULL, NULL) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static U32 sc_acd_deinit_agent_queue()
{
    return DOS_SUCC;
}

static U32 sc_acd_deinit_group_queue()
{
    return DOS_SUCC;
}

static VOID sc_acd_agent_wolk4init(HASH_NODE_S *pNode, VOID *pParam)
{
    SC_ACD_AGENT_QUEUE_NODE_ST  *pstAgentQueueNode    = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST  *pstAgentQueueNodeNew = NULL;
    SC_ACD_GRP_HASH_NODE_ST     *pstGroupNode         = NULL;
    HASH_NODE_S                 *pstHashNode          = NULL;
    DLL_NODE_S                  *pstDLLNode           = NULL;
    U32                         ulHashVal             = 0;
    U32                         ulIndex               = 0;

    pstAgentQueueNode = (SC_ACD_AGENT_QUEUE_NODE_ST *)pNode;
    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
    {
        return;
    }

    for (ulIndex=0; ulIndex<MAX_GROUP_PER_SITE; ulIndex++)
    {
        if (0 != pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]
            && U32_BUTT != pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex])
        {
            sc_acd_hash_func4grp(pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex], &ulHashVal);
            pthread_mutex_lock(&g_mutexGroupList);
            pstHashNode = hash_find_node(g_pstGroupList
                            , ulHashVal
                            , (VOID *)&pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]
                            , sc_acd_grp_hash_find);
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                sc_logr_error(SC_ACD, "Find some agent not in any group. Agent ID: %d, Group ID:%d"
                                        , pstAgentQueueNode->pstAgentInfo->ulSiteID
                                        , pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]);

                pthread_mutex_unlock(&g_mutexGroupList);
                continue;
            }
            pstGroupNode = pstHashNode->pHandle;

            pstDLLNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
            if (DOS_ADDR_INVALID(pstDLLNode))
            {
                sc_logr_error(SC_ACD, "Add agent to group FAILED, Alloc memory for list Node fail. Agent ID: %d, Group ID:%d"
                        , pstAgentQueueNode->pstAgentInfo->ulSiteID
                        , pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]);
                pthread_mutex_unlock(&g_mutexGroupList);
                continue;
            }

            pstAgentQueueNodeNew = (SC_ACD_AGENT_QUEUE_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_AGENT_QUEUE_NODE_ST));
            if (DOS_ADDR_INVALID(pstAgentQueueNodeNew))
            {
                sc_logr_error(SC_ACD, "Add agent to group FAILED, Alloc memory fail. Agent ID: %d, Group ID:%d"
                        , pstAgentQueueNode->pstAgentInfo->ulSiteID
                        , pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]);

                dos_dmem_free(pstDLLNode);
                pstDLLNode = NULL;
                pthread_mutex_unlock(&g_mutexGroupList);
                continue;
            }

            DLL_Init_Node(pstDLLNode);
            pstDLLNode->pHandle = pstAgentQueueNodeNew;
            pstAgentQueueNodeNew->pstAgentInfo = pstAgentQueueNode->pstAgentInfo;

            pthread_mutex_lock(&pstGroupNode->mutexSiteQueue);
            pstAgentQueueNodeNew->ulID = pstGroupNode->usCount;
            pstGroupNode->usCount++;
            DLL_Add(&pstGroupNode->stAgentList, pstDLLNode);
            pthread_mutex_unlock(&pstGroupNode->mutexSiteQueue);

            pthread_mutex_unlock(&g_mutexGroupList);
        }
    }
}

static U32 sc_acd_init_relationship()
{
    HASH_NODE_S     *pstHashNode = NULL;
    U32             ulHashIndex  = 0;

    SC_TRACE_IN(0, 0, 0, 0);

    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            sc_acd_agent_wolk4init(pstHashNode, NULL);
        }
    }

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
        sc_logr_error(SC_ACD, "%s", "Init Group Hash Table Fail.");

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    g_pstGroupList->NodeNum = 0;

    g_pstAgentList = hash_create_table(SC_ACD_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstAgentList))
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_ACD, "%s", "Init Site Hash Table Fail.");

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    g_pstAgentList->NodeNum = 0;

    if (sc_acd_init_group_queue() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_ACD, "%s", "Init group list fail in ACD.");

        hash_delete_table(g_pstAgentList, NULL);
        g_pstAgentList = NULL;

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;


        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    sc_logr_info(SC_ACD, "Init group list finished. Load %d agent group(s).", g_pstGroupList->NodeNum);


    if (sc_acd_init_agent_queue() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_ACD, "%s", "Init sites list fail in ACD.");

        sc_acd_deinit_agent_queue();

        hash_delete_table(g_pstAgentList, NULL);
        g_pstAgentList = NULL;

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    sc_logr_info(SC_ACD, "Init agent list finished. Load %d agent(s).", g_pstAgentList->NodeNum);

    if (sc_acd_init_relationship() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_ACD, "%s", "Init ACD Data FAIL.");

        sc_acd_deinit_group_queue();
        sc_acd_deinit_agent_queue();

        hash_delete_table(g_pstAgentList, NULL);
        g_pstAgentList = NULL;

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

