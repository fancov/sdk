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
#include <esl.h>
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

extern U32 sc_ep_agent_signin(SC_ACD_AGENT_INFO_ST *pstAgentInfo);
extern U32 sc_ep_agent_signout(SC_ACD_AGENT_INFO_ST *pstAgentInfo);

/*
 * 函  数: sc_acd_hash_func4agent
 * 功  能: 坐席的hash函数，通过分机号计算一个hash值
 * 参  数:
 *         S8 *pszExension  : 分机号
 *         U32 *pulHashIndex: 输出参数，计算之后的hash值
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_acd_hash_func4agent(U32 ulSiteID, U32 *pulHashIndex)
{
    SC_TRACE_IN(ulSiteID, pulHashIndex, 0, 0);

    if (DOS_ADDR_INVALID(pulHashIndex))
    {
        DOS_ASSERT(0);

        *pulHashIndex = 0;
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    *pulHashIndex = ulSiteID % SC_ACD_HASH_SIZE;

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
U32 sc_acd_hash_func4grp(U32 ulGrpID, U32 *pulHashIndex)
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
 * 函  数: sc_acd_hash_func4calller_relation
 * 功  能: 主叫号码和坐席对应关系的hash函数，通过主叫号码计算一个hash值
 * 参  数:
 *         U32 ulGrpID  : 坐席组ID
 *         U32 *pulHashIndex: 输出参数，计算之后的hash值
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_acd_hash_func4calller_relation(S8 *szCallerNum, U32 *pulHashIndex)
{
    U64 ulCallerNum = 0;

    if (DOS_ADDR_INVALID(pulHashIndex) || DOS_ADDR_INVALID(szCallerNum))
    {
        return DOS_FAIL;
    }

    SC_TRACE_IN(szCallerNum, pulHashIndex, 0, 0);

    dos_sscanf(szCallerNum, "%9lu", &ulCallerNum);

    *pulHashIndex =  ulCallerNum % SC_ACD_CALLER_NUM_RELATION_HASH_SIZE;

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
S32 sc_acd_grp_hash_find(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGrpHashNode = NULL;
    U32                        ulIndex         = 0;

    if (DOS_ADDR_INVALID(pNode)
        || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

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
S32 sc_acd_agent_hash_find(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    U32                          ulSiteID;

    if (DOS_ADDR_INVALID(pSymName)
        || DOS_ADDR_INVALID(pNode))
    {
        return DOS_FAIL;
    }

    pstAgentQueueNode = pNode->pHandle;
    ulSiteID = *(U32 *)pSymName;

    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
    {
        return DOS_FAIL;
    }

    if (ulSiteID != pstAgentQueueNode->pstAgentInfo->ulSiteID)
    {
        return DOS_FAIL;
    }

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
S32 sc_acd_caller_relation_hash_find(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_ACD_MEMORY_RELATION_QUEUE_NODE_ST *pstHashNode = NULL;
    S8  *szCallerNum = NULL;

    if (DOS_ADDR_INVALID(pSymName)
        || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHashNode = (SC_ACD_MEMORY_RELATION_QUEUE_NODE_ST *)pNode->pHandle;
    szCallerNum = (S8 *)pSymName;
    if (DOS_ADDR_VALID(pstHashNode)
        && !dos_strcmp(szCallerNum, pstHashNode->szCallerNum))
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
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
    U32                          ulSiteID;

    if (DOS_ADDR_INVALID(pSymName)
        || DOS_ADDR_INVALID(pNode))
    {
        return DOS_FAIL;
    }

    pstAgentQueueNode = pNode->pHandle;
    ulSiteID = *(U32 *)pSymName;

    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
    {
        return DOS_FAIL;
    }

    if (ulSiteID != pstAgentQueueNode->pstAgentInfo->ulSiteID)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_acd_agent_update_status_db(U32 ulSiteID, U32 ulStatus)
{
    S8  szQuery[512] = { 0, };
    U32 ulStatusDB = 0;

    ulStatusDB = ulStatus > 1 ? 2 : ulStatus;

    dos_snprintf(szQuery, sizeof(szQuery), "UPDATE tbl_agent SET status=%d WHERE id = %u;", ulStatusDB, ulSiteID);

    if (db_query(g_pstSCDBHandle, szQuery, NULL, NULL, NULL) < 0 )
    {

        sc_logr_info(SC_ACD, "Update agent(%d) status(%d) FAIL!", ulSiteID, ulStatus);
        return DOS_FAIL;
    }

    sc_logr_debug(SC_ACD, "Update agent(%d) status(%d) SUCC", ulSiteID, ulStatus);

    return DOS_SUCC;
}

/*
 * 函  数: U32 sc_acd_agent_update_status(S8 *pszUserID, U32 ulStatus)
 * 功  能: 更新坐席状态
 * 参  数:
 *      S8 *pszUserID : 坐席的SIP USER ID
 *      U32 ulStatus  : 新状态
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_acd_agent_update_status(U32 ulSiteID, U32 ulStatus, U32 ulSCBNo)
{
    SC_ACD_AGENT_QUEUE_NODE_ST  *pstAgentQueueInfo = NULL;
    HASH_NODE_S                 *pstHashNode       = NULL;
    U32                         ulHashIndex        = 0;

    if (ulStatus >= SC_ACD_BUTT)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_acd_hash_func4agent(ulSiteID, &ulHashIndex);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashIndex, (VOID *)&ulSiteID, sc_acd_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstAgentQueueInfo = pstHashNode->pHandle;
    if (DOS_ADDR_INVALID(pstAgentQueueInfo->pstAgentInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&pstAgentQueueInfo->pstAgentInfo->mutexLock);
    pstAgentQueueInfo->pstAgentInfo->ucStatus = (U8)ulStatus;
    pstAgentQueueInfo->pstAgentInfo->usSCBNo = (U16)ulSCBNo;
    pthread_mutex_unlock(&pstAgentQueueInfo->pstAgentInfo->mutexLock);

    /* 更新数据库中，坐席的状态 */
    sc_acd_agent_update_status_db(ulSiteID, ulStatus);

    return DOS_SUCC;
}


/*
 * 函  数: U32 sc_acd_agent_grp_del_call(U32 ulGrpID)
 * 功  能: 坐席组中有等待队列
 * 参  数:
 *       U32 ulGrpID :
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_acd_agent_grp_add_call(U32 ulGrpID)
{
    return DOS_SUCC;
}

/*
 * 函  数: U32 sc_acd_agent_grp_del_call(U32 ulGrpID)
 * 功  能: 统计坐席坐席组
 * 参  数:
 *       U32 ulGrpID : 坐席组中某个等待的呼叫得到调度，通知坐席组
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_acd_agent_grp_del_call(U32 ulGrpID)
{
    return DOS_SUCC;
}

/*
 * 函  数: U32 sc_acd_agent_grp_stat(U32 ulGrpID, U32 ulWaitingTime)
 * 功  能: 统计坐席坐席组
 * 参  数:
 *      U32 ulGrpID,       : 坐席组ID
 *      U32 ulWaitingTime  : 坐席组中呼叫等待时间
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_acd_agent_grp_stat(U32 ulGrpID, U32 ulWaitingTime)
{
    return DOS_SUCC;
}

/*
 * 函  数: U32 sc_acd_agent_stat(U32 ulAgentID, U32 ulCallType, U32 ulStatus)
 * 功  能: 统计坐席呼叫信息
 * 参  数:
 *       U32 ulAgentID   : 坐席ID
 *       U32 ulCallType  : 呼叫类型
 *       U32 ulStatus    : 状态
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_acd_agent_stat(U32 ulAgentID, U32 ulCallType, U32 ulStatus)
{
    return DOS_SUCC;
}

/* 根据SIP，查找到绑定的坐席，更新usSCBNo字段 */
U32 sc_acd_update_agent_scbno_by_userid(S8 *szUserID, U16 usSCBNo)
{
    U32                         ulHashIndex         = 0;
    HASH_NODE_S                 *pstHashNode        = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST  *pstAgentQueueNode  = NULL;
    SC_ACD_AGENT_INFO_ST        *pstAgentData       = NULL;

    if (DOS_ADDR_INVALID(szUserID) || usSCBNo >= SC_MAX_SCB_NUM)
    {
        return DOS_FAIL;
    }

    /* 查找SIP绑定的坐席 */
    pthread_mutex_lock(&g_mutexAgentList);

    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }
            pstAgentQueueNode = (SC_ACD_AGENT_QUEUE_NODE_ST *)pstHashNode->pHandle;
            pstAgentData = pstAgentQueueNode->pstAgentInfo;

            if (DOS_ADDR_INVALID(pstAgentData))
            {
                continue;
            }

            if (pstAgentData->ucBindType != AGENT_BIND_SIP)
            {
                continue;
            }

            if (dos_strcmp(pstAgentData->szUserID, szUserID) == 0)
            {
                pthread_mutex_lock(&pstAgentData->mutexLock);
                pstAgentData->usSCBNo = usSCBNo;
                pthread_mutex_unlock(&pstAgentData->mutexLock);

                pthread_mutex_unlock(&g_mutexAgentList);

                return DOS_SUCC;
            }
        }
    }

    pthread_mutex_unlock(&g_mutexAgentList);

    return DOS_FAIL;
}

U32 sc_acd_update_agent_scbno_by_Siteid(U32 ulAgentID, U16 usSCBNo)
{
    HASH_NODE_S                *pstHashNode = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentNode = NULL;
    U32                        ulHashIndex = 0;

    sc_acd_hash_func4agent(ulAgentID, &ulHashIndex);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashIndex, &ulAgentID, sc_acd_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        sc_logr_warning(SC_ACD, "Cannot find the agent with this id %u.", ulAgentID);
        return DOS_FAIL;
    }

    pstAgentNode = pstHashNode->pHandle;
    if (DOS_ADDR_INVALID(pstAgentNode) || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        DOS_ASSERT(0);
        sc_logr_warning(SC_ACD, "Cannot find the agent with this id %u..", ulAgentID);
        return DOS_FAIL;
    }

    pstAgentNode->pstAgentInfo->usSCBNo = usSCBNo;

    return DOS_SUCC;
}


/*
 * 函  数: U32 sc_acd_group_remove_agent(U32 ulGroupID, S8 *pszUserID)
 * 功  能: 从坐席队列中移除坐席
 * 参  数:
 *       U32 ulGroupID: 坐席组ID
 *       S8 *pszUserID: 坐席唯一标示 sip账户
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_acd_group_remove_agent(U32 ulGroupID, U32 ulSiteID)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode  = NULL;
    SC_ACD_GRP_HASH_NODE_ST      *pstGroupNode       = NULL;
    HASH_NODE_S                  *pstHashNode        = NULL;
    DLL_NODE_S                   *pstDLLNode         = NULL;
    U32                          ulHashVal           = 0;

    /* 检测所在队列是否存在 */
    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot find the group \"%u\" for the site %u.", ulGroupID, ulSiteID);

        pthread_mutex_unlock(&g_mutexGroupList);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstGroupNode = pstHashNode->pHandle;

    pstDLLNode = dll_find(&pstGroupNode->stAgentList, (VOID *)&ulSiteID, sc_acd_agent_dll_find);
    if (DOS_ADDR_INVALID(pstDLLNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot find the agent %u in the group %u.", ulSiteID, ulGroupID);

        pthread_mutex_unlock(&g_mutexGroupList);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dll_delete(&pstGroupNode->stAgentList, pstDLLNode);

    pstAgentQueueNode = pstDLLNode->pHandle;

    pstDLLNode->pHandle = NULL;
    dos_dmem_free(pstDLLNode);
    pstDLLNode = NULL;
    pstAgentQueueNode->pstAgentInfo = NULL;
    dos_dmem_free(pstAgentQueueNode);
    pstAgentQueueNode = NULL;

    pstGroupNode->usCount--;

    pthread_mutex_unlock(&g_mutexGroupList);
    sc_logr_debug(SC_ACD, "Remove agent %u from group %u SUCC.", ulSiteID, ulGroupID);

    return DOS_SUCC;
}

/*
 * 函  数: U32 sc_acd_group_add_agent(U32 ulGroupID, SC_ACD_AGENT_INFO_ST *pstAgentInfo)
 * 功  能: 将坐席添加到坐席队列
 * 参  数:
 *       U32 ulGroupID: 坐席组ID
 *       SC_ACD_AGENT_INFO_ST *pstAgentInfo : 坐席组信息
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_acd_group_add_agent(U32 ulGroupID, SC_ACD_AGENT_INFO_ST *pstAgentInfo)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode  = NULL;
    SC_ACD_GRP_HASH_NODE_ST      *pstGroupNode       = NULL;
    HASH_NODE_S                  *pstHashNode        = NULL;
    DLL_NODE_S                   *pstDLLNode         = NULL;
    U32                          ulHashVal           = 0;

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* 检测所在队列是否存在 */
    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    pthread_mutex_unlock(&g_mutexGroupList);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot find the group \"%u\" for the site %s.", ulGroupID, pstAgentInfo->szUserID);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    pstGroupNode = pstHashNode->pHandle;

    pstDLLNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        sc_logr_error(SC_ACD, "Add agent to group FAILED, Alloc memory for list Node fail. Agent ID: %u, Group ID:%u"
                , pstAgentInfo->ulSiteID
                , ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);
        return DOS_FAIL;
    }

    pstAgentQueueNode = (SC_ACD_AGENT_QUEUE_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_AGENT_QUEUE_NODE_ST));
    if (DOS_ADDR_INVALID(pstAgentQueueNode))
    {
        sc_logr_error(SC_ACD, "Add agent to group FAILED, Alloc memory fail. Agent ID: %u, Group ID:%u"
                , pstAgentInfo->ulSiteID
                , ulGroupID);

        dos_dmem_free(pstDLLNode);
        pstDLLNode = NULL;
        pthread_mutex_unlock(&g_mutexGroupList);
        return DOS_FAIL;
    }

    DLL_Init_Node(pstDLLNode);
    pstDLLNode->pHandle = pstAgentQueueNode;
    pstAgentQueueNode->pstAgentInfo = pstAgentInfo;

    pthread_mutex_lock(&pstGroupNode->mutexSiteQueue);
    pstAgentQueueNode->ulID = pstGroupNode->usCount;
    pstGroupNode->usCount++;

    DLL_Add(&pstGroupNode->stAgentList, pstDLLNode);
    pthread_mutex_unlock(&pstGroupNode->mutexSiteQueue);

    pthread_mutex_unlock(&g_mutexGroupList);

    sc_logr_debug(SC_ACD, "Add agent to group SUCC. Agent ID: %u, Group ID:%u, Bind Type: %u"
            , pstAgentInfo->ulSiteID
            , pstGroupNode->ulGroupID
            , pstAgentInfo->ucBindType);

    return DOS_SUCC;
}

/*
 * 函  数: sc_acd_add_agent
 * 功  能: 添加坐席
 * 参  数:
 *         SC_ACD_AGENT_INFO_ST *pstAgentInfo, 坐席信息描述
 *         U32 ulGrpID 所属组
 * 返回值: SC_ACD_AGENT_QUEUE_NODE_ST *返回坐席队列里面的节点。编译调用函数将坐席添加到组。如果失败则返回NULL
 * !!! 该函数只是将坐席加入管理队列，不会将坐席加入坐席对列 !!!
 **/
SC_ACD_AGENT_INFO_ST *sc_acd_add_agent(SC_ACD_AGENT_INFO_ST *pstAgentInfo)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode  = NULL;
    SC_ACD_AGENT_INFO_ST         *pstAgentData       = NULL;
    HASH_NODE_S                  *pstHashNode        = NULL;
    U32                          ulHashVal           = 0;

    SC_TRACE_IN(pstAgentInfo, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return NULL;
    }

    pstAgentQueueNode = (SC_ACD_AGENT_QUEUE_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_AGENT_QUEUE_NODE_ST));
    if (DOS_ADDR_INVALID(pstAgentQueueNode))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return NULL;
    }

    pstAgentData = (SC_ACD_AGENT_INFO_ST *)dos_dmem_alloc(sizeof(SC_ACD_AGENT_INFO_ST));
    if (DOS_ADDR_INVALID(pstAgentData))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return NULL;
    }

    pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return NULL;
    }

    /* 加入坐席管理hash表 */
    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = pstAgentQueueNode;
    sc_acd_hash_func4agent(pstAgentInfo->ulSiteID, &ulHashVal);
    pthread_mutex_lock(&g_mutexAgentList);
    hash_add_node(g_pstAgentList, pstHashNode, ulHashVal, NULL);
    pthread_mutex_unlock(&g_mutexAgentList);

    sc_logr_debug(SC_ACD, "Load Agent. ID: %u, Customer: %u, Group1: %u, Group2: %u"
                    , pstAgentInfo->ulSiteID, pstAgentInfo->ulCustomerID
                    , pstAgentInfo->aulGroupID[0], pstAgentInfo->aulGroupID[1]);

    /* 添加到队列 */
    dos_memcpy(pstAgentData, pstAgentInfo, sizeof(SC_ACD_AGENT_INFO_ST));
    pthread_mutex_init(&pstAgentData->mutexLock, NULL);
    pstAgentQueueNode->pstAgentInfo = pstAgentData;

    SC_TRACE_OUT();
    return pstAgentData;
}


/*
 * 函  数: sc_acd_grp_wolk4delete_site
 * 功  能: 从某一个坐席组里面删除坐席
 * 参  数:
 *         HASH_NODE_S *pNode : 当前节点
 *         VOID *pszExtensition : 被删除坐席的分机号
 * 返回值: VOID
 **/
static VOID sc_acd_grp_wolk4delete_agent(HASH_NODE_S *pNode, VOID *pulSiteID)
{
    SC_ACD_GRP_HASH_NODE_ST      *pstGroupListNode  = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    DLL_NODE_S                   *pstDLLNode        = NULL;

    if (DOS_ADDR_INVALID(pNode) || DOS_ADDR_INVALID(pulSiteID))
    {
        return;
    }

    if (DOS_ADDR_INVALID(pNode)
        || DOS_ADDR_INVALID(pNode->pHandle))
    {
        DOS_ASSERT(0);
        return;
    }

    pstGroupListNode = (SC_ACD_GRP_HASH_NODE_ST *)pNode->pHandle;
    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);
    pstDLLNode = dll_find(&pstGroupListNode->stAgentList, (VOID *)pulSiteID, sc_acd_agent_dll_find);
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

U32 sc_acd_delete_agent(U32  ulSiteID)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    HASH_NODE_S                  *pstHashNode       = NULL;
    U32                          ulHashVal          = 0;

    SC_TRACE_IN(ulSiteID, 0, 0, 0);

    /* 遍历所有组，并删除相关坐席 */
    pthread_mutex_lock(&g_mutexGroupList);
    hash_walk_table(g_pstGroupList, &ulSiteID, sc_acd_grp_wolk4delete_agent);
    pthread_mutex_unlock(&g_mutexGroupList);

    /* 做到坐席，然后将其值为删除状态 */
    pthread_mutex_lock(&g_mutexAgentList);
    sc_acd_hash_func4agent(ulSiteID, &ulHashVal);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashVal, &ulSiteID, sc_acd_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Connot find the Site %u while delete", ulSiteID);
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


U32 sc_acd_update_agent_status(U32 ulAction, U32 ulAgentID)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    HASH_NODE_S                  *pstHashNode       = NULL;
    U32                          ulHashVal          = 0;
    U32                          ulAgentStatusOld   = 0;

    SC_TRACE_IN(ulAgentID, ulAction, 0, 0);

    if (ulAction >= SC_ACD_SITE_ACTION_BUTT)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 找到坐席 */
    pthread_mutex_lock(&g_mutexAgentList);

    HASH_Scan_Table(g_pstAgentList, ulHashVal)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashVal, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                break;
            }

            pstAgentQueueNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentQueueNode))
            {
                continue;
            }

            if (DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
            {
                continue;
            }

            if (pstAgentQueueNode->pstAgentInfo->ulSiteID == ulAgentID)
            {
                ulAgentStatusOld = pstAgentQueueNode->pstAgentInfo->ucStatus;

                switch (ulAction)
                {
                    case SC_ACD_SITE_ACTION_DELETE:
                        pstAgentQueueNode->pstAgentInfo->bWaitingDelete = DOS_TRUE;
                        break;

                    case SC_ACD_SITE_ACTION_ONLINE:
                        pstAgentQueueNode->pstAgentInfo->bLogin = DOS_TRUE;
                        pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bNeedConnected = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bWaitingDelete = DOS_FALSE;

                        pstAgentQueueNode->pstAgentInfo->ucStatus = SC_ACD_IDEL;
                        pstAgentQueueNode->pstAgentInfo->ulLastOnlineTime = time(0);
                        break;

                    case SC_ACD_SITE_ACTION_OFFLINE:
                        pstAgentQueueNode->pstAgentInfo->bLogin = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bNeedConnected = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bWaitingDelete = DOS_FALSE;

                        pstAgentQueueNode->pstAgentInfo->ucStatus = SC_ACD_OFFLINE;

                        pstAgentQueueNode->pstAgentInfo->stStat.ulTimeOnthePhone += (time(0) - pstAgentQueueNode->pstAgentInfo->ulLastOnlineTime);
                        pstAgentQueueNode->pstAgentInfo->ulLastOnlineTime = 0;
                        break;

                    case SC_ACD_SITE_ACTION_SIGNIN:
                        pstAgentQueueNode->pstAgentInfo->bLogin = DOS_TRUE;
                        pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bNeedConnected = DOS_TRUE;
                        pstAgentQueueNode->pstAgentInfo->bWaitingDelete = DOS_FALSE;

                        //pstAgentQueueNode->pstAgentInfo->ucStatus = SC_ACD_IDEL;

                        pstAgentQueueNode->pstAgentInfo->ulLastSignInTime = time(0);
                        /* 呼叫坐席 */
                        sc_ep_agent_signin(pstAgentQueueNode->pstAgentInfo);

                        break;

                    case SC_ACD_SITE_ACTION_SIGNOUT:
                        pstAgentQueueNode->pstAgentInfo->bLogin = DOS_TRUE;
                        pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bNeedConnected = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bWaitingDelete = DOS_FALSE;

                        //pstAgentQueueNode->pstAgentInfo->ucStatus = SC_ACD_IDEL;

                        pstAgentQueueNode->pstAgentInfo->stStat.ulTimeOnSignin += (time(0) - pstAgentQueueNode->pstAgentInfo->ulLastSignInTime);
                        pstAgentQueueNode->pstAgentInfo->ulLastSignInTime = 0;

                        /* 挂断坐席的电话 */
                        sc_ep_agent_signout(pstAgentQueueNode->pstAgentInfo);

                        break;

                    case SC_ACD_SITE_ACTION_EN_QUEUE:
                        pstAgentQueueNode->pstAgentInfo->bLogin = DOS_TRUE;
                        pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bNeedConnected = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bWaitingDelete = DOS_FALSE;

                        pstAgentQueueNode->pstAgentInfo->ucStatus = SC_ACD_IDEL;

                        break;

                    case SC_ACD_SITE_ACTION_DN_QUEUE:
                        pstAgentQueueNode->pstAgentInfo->bLogin = DOS_TRUE;
                        pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bNeedConnected = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bWaitingDelete = DOS_FALSE;

                        pstAgentQueueNode->pstAgentInfo->ucStatus = SC_ACD_BUSY;

                        break;

                    case SC_ACD_SITE_ACTION_CONNECTED:
                        pstAgentQueueNode->pstAgentInfo->bConnected = DOS_TRUE;
                        break;

                    case SC_ACD_SITE_ACTION_DISCONNECT:
                        pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                        break;

                    case SC_ACD_SITE_ACTION_CONNECT_FAIL:
                        pstAgentQueueNode->pstAgentInfo->bNeedConnected = DOS_FALSE;
                        pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                        break;

                    default:
                        DOS_ASSERT(0);
                        break;
                }

                /* 状态发生改变，更新数据库 */
                if (ulAgentStatusOld != pstAgentQueueNode->pstAgentInfo->ucStatus
                     && (ulAgentStatusOld < 2 || pstAgentQueueNode->pstAgentInfo->ucStatus < 2))
                {
                    sc_acd_agent_update_status_db(ulAgentID, pstAgentQueueNode->pstAgentInfo->ucStatus);
                }

                pthread_mutex_unlock(&g_mutexAgentList);
                return DOS_SUCC;
            }
        }
    }

    pthread_mutex_unlock(&g_mutexAgentList);
    return DOS_FAIL;
}

U32 sc_acd_add_queue(U32 ulGroupID, U32 ulCustomID, U32 ulPolicy, S8 *pszGroupName)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode = NULL;
    HASH_NODE_S                *pstHashNode      = NULL;
    U32                        ulHashVal         = 0;

    SC_TRACE_IN(ulGroupID, ulCustomID, ulPolicy, pszGroupName);

    if (DOS_ADDR_INVALID(pszGroupName))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (0 == ulGroupID
        || 0 == ulCustomID
        || ulPolicy >= SC_ACD_POLICY_BUTT)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_logr_debug(SC_ACD, "Load Group. ID:%u, Customer:%u, Policy: %u, Name: %s", ulGroupID, ulCustomID, ulPolicy, pszGroupName);

    /* 确定队列 */
    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_VALID(pstHashNode)
        && DOS_ADDR_VALID(pstHashNode->pHandle))
    {
        pstGroupListNode = pstHashNode->pHandle;

        pstGroupListNode->ucACDPolicy = (U8)ulPolicy;
        pstGroupListNode->usCount = 0;
        pstGroupListNode->usLastUsedAgent = 0;
        pstGroupListNode->szLastEmpNo[0] = '\0';

        sc_logr_error(SC_ACD, "Group \"%u\" Already in the list. Update", ulGroupID);
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
    dos_memzero(pstGroupListNode, sizeof(SC_ACD_GRP_HASH_NODE_ST));

    pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstGroupListNode);
        pstGroupListNode = NULL;
        sc_logr_error(SC_ACD, "%s", "Add group fail. Alloc memory for hash node fail");

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    HASH_Init_Node(pstHashNode);
    DLL_Init(&pstGroupListNode->stAgentList);
    pstGroupListNode->pstRelationList = hash_create_table(SC_ACD_CALLER_NUM_RELATION_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(pstGroupListNode->pstRelationList))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstGroupListNode);
        pstGroupListNode = NULL;
        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;

        sc_logr_error(SC_ACD, "%s", "Init Group caller num relation Hash Table Fail.");

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    pstGroupListNode->pstRelationList->NodeNum = 0;

    pthread_mutex_init(&pstGroupListNode->mutexSiteQueue, NULL);
    pstGroupListNode->ulCustomID = ulCustomID;
    pstGroupListNode->ulGroupID  = ulGroupID;
    pstGroupListNode->ucACDPolicy = (U8)ulPolicy;
    pstGroupListNode->usCount = 0;
    pstGroupListNode->usLastUsedAgent = 0;
    pstGroupListNode->szLastEmpNo[0] = '\0';
    pstGroupListNode->usID = (U16)g_ulGroupCount;
    pstGroupListNode->ucWaitingDelete = DOS_FALSE;
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

        sc_logr_error(SC_ACD, "Connot find the Group \"%u\".", ulGroupID);
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


SC_ACD_AGENT_QUEUE_NODE_ST * sc_acd_get_agent_by_random(SC_ACD_GRP_HASH_NODE_ST *pstGroupListNode)
{
    U32     ulRandomAgent      = 0;
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentQueueNode = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentNodeRet   = NULL;
    SC_ACD_AGENT_INFO_ST       *pstAgentInfo      = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;

    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /*
     * 随机一个编号，然后从这个编号开始查找一个可用的坐席。如果到队尾了还没有找到就再从头来
     */

    ulRandomAgent = dos_random(pstGroupListNode->usCount);

    sc_logr_debug(SC_ACD, "Select agent in random. Start find agent %u in group %u, count: %u."
                    , ulRandomAgent
                    , pstGroupListNode->ulGroupID
                    , pstGroupListNode->stAgentList.ulCount);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Group: %u."
                                        , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentQueueNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentQueueNode)
            || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (pstAgentQueueNode->ulID <= ulRandomAgent)
        {
            sc_logr_debug(SC_ACD, "Found an agent. But the agent's order(%u) is less then last agent order(%u). coutinue.(Agent %d in Group %u)"
                            , pstAgentQueueNode->ulID
                            , ulRandomAgent
                            , pstAgentQueueNode->pstAgentInfo->ulSiteID
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "There found an agent. But the agent is not useable. coutinue.(Agent %u in Group %u)"
                            , pstAgentQueueNode->pstAgentInfo->ulSiteID
                            , pstGroupListNode->ulGroupID);

            continue;
        }

        pstAgentNodeRet = pstAgentQueueNode;
        pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
        sc_logr_notice(SC_ACD, "Found an uaeable agent.(Agent %u in Group %u)"
                        , pstAgentInfo->ulSiteID
                        , pstGroupListNode->ulGroupID);
         break;
    }

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        sc_logr_debug(SC_ACD, "Select agent in random form header. Start find agent %u in group %u, count: %u."
                        , ulRandomAgent
                        , pstGroupListNode->ulGroupID
                        , pstGroupListNode->stAgentList.ulCount);

        DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstDLLNode)
                || DOS_ADDR_INVALID(pstDLLNode->pHandle))
            {
                sc_logr_debug(SC_ACD, "Group List node has no data. Group: %u."
                                            , pstGroupListNode->ulGroupID);
                continue;
            }

            pstAgentQueueNode = pstDLLNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentQueueNode)
                || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
            {
                sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %u."
                                , pstGroupListNode->ulGroupID);
                continue;
            }

            /* 邋到这里已经查找过所有的的坐席了 */
            if (pstAgentQueueNode->ulID > ulRandomAgent)
            {
                sc_logr_debug(SC_ACD, "The end of the select loop.(Group %u)"
                                , pstGroupListNode->ulGroupID);
                break;
            }

            if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
            {
                sc_logr_debug(SC_ACD, "There found an agent. But the agent is not useable. coutinue.(Agent %u in Group %u)"
                                , pstAgentQueueNode->pstAgentInfo->ulSiteID
                                , pstGroupListNode->ulGroupID);
                continue;
            }

            pstAgentNodeRet = pstAgentQueueNode;
            pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
            sc_logr_notice(SC_ACD, "Found an uaeable agent.(Agent %u in Group %u)"
                            , pstAgentInfo->ulSiteID
                            , pstGroupListNode->ulGroupID);

            break;
        }
    }

    return pstAgentNodeRet;
}

SC_ACD_AGENT_QUEUE_NODE_ST * sc_acd_get_agent_by_inorder(SC_ACD_GRP_HASH_NODE_ST *pstGroupListNode)
{
    S8      szLastEmpNo[SC_EMP_NUMBER_LENGTH]     = {0};
    S8      szEligibleEmpNo[SC_EMP_NUMBER_LENGTH] = {0};
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentQueueNode = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentNodeRet   = NULL;
    SC_ACD_AGENT_INFO_ST       *pstAgentInfo      = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;

    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /*
     * 从上次使用的编号开始查找一个可用的坐席。如果到队尾了还没有找到就再从头来
     */

    dos_strncpy(szLastEmpNo, pstGroupListNode->szLastEmpNo, SC_EMP_NUMBER_LENGTH);
    szLastEmpNo[SC_EMP_NUMBER_LENGTH-1] = '\0';
    szEligibleEmpNo[0] = '\0';

start_find:
    sc_logr_debug(SC_ACD, "Select agent in order. Start find agent %s in group %u, Count : %u"
                    , szLastEmpNo
                    , pstGroupListNode->ulGroupID
                    , pstGroupListNode->stAgentList.ulCount);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentQueueNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentQueueNode)
            || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        /* 找到一个比最后一个大且最小工号的坐席 */
        if (dos_strncmp(szLastEmpNo, pstAgentQueueNode->pstAgentInfo->szEmpNo, SC_EMP_NUMBER_LENGTH) >= 0)
        {
            sc_logr_debug(SC_ACD, "Found an agent. But the agent's order(%s) is less then last agent order(%s). coutinue.(Agent %u in Group %u)"
                            , pstAgentQueueNode->pstAgentInfo->szEmpNo
                            , szLastEmpNo
                            , pstAgentQueueNode->pstAgentInfo->ulSiteID
                            , pstGroupListNode->ulGroupID);

            continue;
        }

        if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
        {

            sc_logr_debug(SC_ACD, "There found an agent. But the agent is not useable. coutinue.(Agent %u in Group %u)"
                            , pstAgentQueueNode->pstAgentInfo->ulSiteID
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if ('\0' != szEligibleEmpNo[0])
        {
            if (dos_strncmp(szEligibleEmpNo, pstAgentQueueNode->pstAgentInfo->szEmpNo, SC_EMP_NUMBER_LENGTH) <= 0)
            {
                continue;
            }
        }
        dos_strncpy(szEligibleEmpNo, pstAgentQueueNode->pstAgentInfo->szEmpNo, SC_EMP_NUMBER_LENGTH);
        szEligibleEmpNo[SC_EMP_NUMBER_LENGTH - 1] = '\0';

        pstAgentNodeRet = pstAgentQueueNode;
        pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
        sc_logr_notice(SC_ACD, "Found an useable agent.(Agent %u in Group %u)"
                        , pstAgentInfo->ulSiteID
                        , pstGroupListNode->ulGroupID);
    }

    if (DOS_ADDR_INVALID(pstAgentInfo) && szLastEmpNo[0] != '\0')
    {
        /* 没有找到到，找最小的工号编号的坐席来接电话 */
        szLastEmpNo[0] = '\0';

        goto start_find;
    }

    return pstAgentNodeRet;
}

SC_ACD_AGENT_QUEUE_NODE_ST * sc_acd_get_agent_by_call_count(SC_ACD_GRP_HASH_NODE_ST *pstGroupListNode)
{
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentQueueNode = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentNode      = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;

    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    sc_logr_debug(SC_ACD, "Select agent by the min call count. Start find agent in group %u."
                    , pstGroupListNode->ulGroupID);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentQueueNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentQueueNode)
            || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "There found an agent. But the agent is not useable. coutinue.(Agent %u in Group %u)"
                        , pstAgentQueueNode->pstAgentInfo->ulSiteID
                        , pstGroupListNode->ulGroupID);
            continue;
        }

        if (DOS_ADDR_INVALID(pstAgentNode))
        {
            pstAgentNode = pstAgentQueueNode;
        }

        sc_logr_notice(SC_ACD, "Found an uaeable agent. Call Count: %d. (Agent %d in Group %d)"
                        , pstAgentQueueNode->pstAgentInfo->ulCallCnt
                        , pstAgentQueueNode->pstAgentInfo->ulSiteID
                        , pstGroupListNode->ulGroupID);
        if (pstAgentNode->pstAgentInfo->ulCallCnt > pstAgentQueueNode->pstAgentInfo->ulCallCnt)
        {
            pstAgentNode = pstAgentQueueNode;
        }
    }

    if (DOS_ADDR_VALID(pstAgentNode) && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
    {
        pstAgentNode->pstAgentInfo->ulCallCnt++;
    }

    return pstAgentNode;
}


SC_ACD_AGENT_QUEUE_NODE_ST * sc_acd_get_agent_by_caller(SC_ACD_GRP_HASH_NODE_ST *pstGroupListNode, S8 *szCallerNum)
{
    SC_ACD_MEMORY_RELATION_QUEUE_NODE_ST *pstRelationQueueNode = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentNode      = NULL;
    HASH_NODE_S                *pstHashNode       = NULL;
    HASH_NODE_S                *pstAgentHashNode  = NULL;
    U32                         ulHashVal         = 0;
    U32                         ulAgentHashVal    = 0;
    U32                         ulSiteID          = 0;

    if (DOS_ADDR_INVALID(pstGroupListNode) || DOS_ADDR_INVALID(szCallerNum))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    sc_logr_debug(SC_ACD, "Select agent by the calllerNum(%s). Start find agent in group %u. %p"
                    , szCallerNum, pstGroupListNode->ulGroupID, pstGroupListNode->pstRelationList);
    /* 根据主叫号码查找对应的坐席 */
    sc_acd_hash_func4calller_relation(szCallerNum, &ulHashVal);
    pstHashNode = hash_find_node(pstGroupListNode->pstRelationList, ulHashVal, szCallerNum, sc_acd_caller_relation_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_logr_debug(SC_ACD, "Not found the agent ulSiteID by CallerNum(%s), Group(%u)"
                        , szCallerNum
                        , pstGroupListNode->ulGroupID);

        goto process_call;
    }

    pstRelationQueueNode = pstHashNode->pHandle;
    ulSiteID = pstRelationQueueNode->ulSiteID;
    /* 根据坐席编号, 在坐席hash中查找到对应的坐席 */
    sc_acd_hash_func4agent(ulSiteID, &ulAgentHashVal);
    pstAgentHashNode = hash_find_node(g_pstAgentList, ulAgentHashVal, (VOID *)&ulSiteID, sc_acd_agent_hash_find);
    if (DOS_ADDR_INVALID(pstAgentHashNode)
        || DOS_ADDR_INVALID(pstAgentHashNode->pHandle))
    {
        sc_logr_debug(SC_ACD, "Not found the agent.(Agent %u in Group %u)"
                    , ulSiteID
                    , pstGroupListNode->ulGroupID);

        goto process_call;
    }

    pstAgentNode = pstAgentHashNode->pHandle;
    if (!SC_ACD_SITE_IS_USEABLE(pstAgentNode->pstAgentInfo))
    {
        sc_logr_debug(SC_ACD, "There found an agent. But the agent is not useable.(Agent %u in Group %u)"
                , ulSiteID
                , pstGroupListNode->ulGroupID);

        goto process_call;
    }

    sc_logr_debug(SC_ACD, "There found an agent.(Agent %u in Group %u)"
                , ulSiteID
                , pstGroupListNode->ulGroupID);

    return pstAgentNode;

process_call:
    pstAgentNode = sc_acd_get_agent_by_call_count(pstGroupListNode);
    if (DOS_ADDR_VALID(pstAgentNode))
    {
        /* 添加或者更新 主叫号码和坐席对应关系的hash */
        sc_logr_notice(SC_ACD, "Found an uaeable agent. Call Count: %d. (Agent %d in Group %d)"
                        , pstAgentNode->pstAgentInfo->ulCallCnt
                        , pstAgentNode->pstAgentInfo->ulSiteID
                        , pstGroupListNode->ulGroupID);

        if (DOS_ADDR_VALID(pstHashNode) && DOS_ADDR_VALID(pstHashNode->pHandle))
        {
            /* 更新 */
            pstRelationQueueNode->ulSiteID = pstAgentNode->pstAgentInfo->ulSiteID;

            goto end;
        }

        pstRelationQueueNode = (SC_ACD_MEMORY_RELATION_QUEUE_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_MEMORY_RELATION_QUEUE_NODE_ST));
        if (DOS_ADDR_INVALID(pstRelationQueueNode))
        {
            DOS_ASSERT(0);
            sc_logr_error(SC_ACD, "%s", "Add CallerNum relationship fail. Alloc memory fail");

            goto end;
        }

        if (DOS_ADDR_INVALID(pstHashNode))
        {
            pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                DOS_ASSERT(0);

                sc_logr_error(SC_ACD, "%s", "Add CallerNum relationship fail. Alloc memory fail");
                dos_dmem_free(pstRelationQueueNode);
                pstRelationQueueNode = NULL;

                goto end;
            }

            HASH_Init_Node(pstHashNode);
            pstHashNode->pHandle = NULL;
            hash_add_node(pstGroupListNode->pstRelationList, pstHashNode, ulHashVal, NULL);
            sc_logr_debug(SC_ACD, "add into hash, ulHashVal : %d, count : %d", ulHashVal, pstGroupListNode->pstRelationList[ulHashVal].NodeNum);
        }

        pstRelationQueueNode->ulSiteID = pstAgentNode->pstAgentInfo->ulSiteID;
        dos_strncpy(pstRelationQueueNode->szCallerNum, szCallerNum, SC_TEL_NUMBER_LENGTH);
        pstRelationQueueNode->szCallerNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';
        pstHashNode->pHandle = pstRelationQueueNode;
    }

end:
    return pstAgentNode;
}

U32 sc_acd_get_agent_by_id(SC_ACD_AGENT_INFO_ST *pstAgentInfo, U32 ulAgentID)
{
    HASH_NODE_S                *pstHashNode = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentNode = NULL;
    U32                        ulHashIndex = 0;

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_acd_hash_func4agent(ulAgentID, &ulHashIndex);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashIndex, &ulAgentID, sc_acd_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        sc_logr_warning(SC_ACD, "Cannot find the agent with this id %u.", ulAgentID);
        return DOS_FAIL;
    }

    pstAgentNode = pstHashNode->pHandle;
    if (DOS_ADDR_INVALID(pstAgentNode) || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        DOS_ASSERT(0);
        sc_logr_warning(SC_ACD, "Cannot find the agent with this id %u..", ulAgentID);
        return DOS_FAIL;
    }

    dos_memcpy(pstAgentInfo, pstAgentNode->pstAgentInfo, sizeof(SC_ACD_AGENT_INFO_ST));

    return DOS_SUCC;
 }


U32 sc_acd_get_agent_by_grpid(SC_ACD_AGENT_INFO_ST *pstAgentBuff, U32 ulGroupID, S8 *szCallerNum)
{
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentNode      = NULL;
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode  = NULL;
    HASH_NODE_S                *pstHashNode       = NULL;
    U32                        ulHashVal          = 0;
    U32                        ulResult           = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstAgentBuff)
        || 0 == ulGroupID
        || U32_BUTT == ulGroupID)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

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
        return DOS_FAIL;
    }

    pstGroupListNode = pstHashNode->pHandle;

    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);

    switch (pstGroupListNode->ucACDPolicy)
    {
        case SC_ACD_POLICY_IN_ORDER:
            pstAgentNode = sc_acd_get_agent_by_inorder(pstGroupListNode);
            break;

        case SC_ACD_POLICY_MIN_CALL:
            pstAgentNode = sc_acd_get_agent_by_call_count(pstGroupListNode);
            break;

        case SC_ACD_POLICY_RANDOM:
            pstAgentNode = sc_acd_get_agent_by_random(pstGroupListNode);
            break;

        case SC_ACD_POLICY_RECENT:
        case SC_ACD_POLICY_GROUP:
            sc_logr_notice(SC_ACD, "Template not support policy %d", pstGroupListNode->ucACDPolicy);
            break;
        case SC_ACD_POLICY_MEMORY:
            pstAgentNode = sc_acd_get_agent_by_caller(pstGroupListNode, szCallerNum);
            break;
        default:
            break;
    }

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    if (DOS_ADDR_VALID(pstAgentNode)
        && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
    {
        pstGroupListNode->usLastUsedAgent = pstAgentNode->ulID;
        dos_strncmp(pstGroupListNode->szLastEmpNo, pstAgentNode->pstAgentInfo->szEmpNo, SC_EMP_NUMBER_LENGTH);
        pstGroupListNode->szLastEmpNo[SC_EMP_NUMBER_LENGTH-1] = '\0';
        pstAgentNode->pstAgentInfo->stStat.ulCallCnt++;
        pstAgentNode->pstAgentInfo->stStat.ulSelectCnt++;

        dos_memcpy(pstAgentBuff, pstAgentNode->pstAgentInfo, sizeof(SC_ACD_AGENT_INFO_ST));
        ulResult = DOS_SUCC;
    }
    else
    {
        dos_memzero(pstAgentBuff, sizeof(SC_ACD_AGENT_INFO_ST));
        ulResult = DOS_FAIL;
    }
    pthread_mutex_unlock(&g_mutexGroupList);

    return ulResult;
}

U32 sc_acd_get_agent_by_userid(SC_ACD_AGENT_INFO_ST *pstAgentInfo, S8 *szUserID)
{
    U32                         ulHashIndex         = 0;
    HASH_NODE_S                 *pstHashNode        = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST  *pstAgentQueueNode  = NULL;
    SC_ACD_AGENT_INFO_ST        *pstAgentData       = NULL;

    if (DOS_ADDR_INVALID(pstAgentInfo) || DOS_ADDR_INVALID(szUserID))
    {
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexAgentList);

    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }
            pstAgentQueueNode = (SC_ACD_AGENT_QUEUE_NODE_ST *)pstHashNode->pHandle;
            pstAgentData = pstAgentQueueNode->pstAgentInfo;

            if (DOS_ADDR_INVALID(pstAgentData))
            {
                continue;
            }

            if (pstAgentData->ucBindType != AGENT_BIND_SIP)
            {
                continue;
            }

            if (dos_strcmp(pstAgentData->szUserID, szUserID) == 0)
            {
                pthread_mutex_lock(&pstAgentData->mutexLock);
                dos_memcpy(pstAgentInfo, pstAgentData, sizeof(SC_ACD_AGENT_INFO_ST));
                pthread_mutex_unlock(&pstAgentData->mutexLock);

                pthread_mutex_unlock(&g_mutexAgentList);

                return DOS_SUCC;
            }
        }
    }

    pthread_mutex_unlock(&g_mutexAgentList);

    return DOS_FAIL;
}


U32 sc_acd_query_idel_agent(U32 ulAgentGrpID, BOOL *pblResult)
{
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentNode      = NULL;
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode  = NULL;
    HASH_NODE_S                *pstHashNode       = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;
    U32                        ulHashVal          = 0;

    if (DOS_ADDR_INVALID(pblResult))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    *pblResult = DOS_FALSE;


    sc_acd_hash_func4grp(ulAgentGrpID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulAgentGrpID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot fine the group with the ID \"%s\" .", ulAgentGrpID);
        pthread_mutex_unlock(&g_mutexGroupList);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstGroupListNode = pstHashNode->pHandle;

    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentNode)
            || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (SC_ACD_SITE_IS_USEABLE(pstAgentNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "Found an useable agent. (Agent %u in Group %u)"
                        , pstAgentNode->pstAgentInfo->ulSiteID
                        , pstGroupListNode->ulGroupID);

            *pblResult = DOS_TRUE;
            break;
        }
    }

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    pthread_mutex_unlock(&g_mutexGroupList);

    return DOS_SUCC;

}

U32 sc_acd_get_total_agent(U32 ulGroupID)
{
    U32 ulCnt = 0;

    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode  = NULL;
    HASH_NODE_S                *pstHashNode       = NULL;
    U32                        ulHashVal          = 0;


    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot fine the group with the ID \"%u\" .", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        SC_TRACE_OUT();
        return 0;
    }

    pstGroupListNode = pstHashNode->pHandle;

    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);

    ulCnt = pstGroupListNode->stAgentList.ulCount;

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    pthread_mutex_unlock(&g_mutexGroupList);

    return ulCnt;

}


U32 sc_acd_get_idel_agent(U32 ulGroupID)
{
    U32 ulCnt = 0;

    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentNode      = NULL;
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode  = NULL;
    HASH_NODE_S                *pstHashNode       = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;
    U32                        ulHashVal          = 0;


    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot fine the group with the ID \"%u\" .", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        SC_TRACE_OUT();
        return 0;
    }

    pstGroupListNode = pstHashNode->pHandle;

    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentNode)
            || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (SC_ACD_SITE_IS_USEABLE(pstAgentNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "There found an agent. But the agent is not useable. coutinue.(Agent %u in Group %u)"
                        , pstAgentNode->pstAgentInfo->ulSiteID
                        , pstGroupListNode->ulGroupID);

            ulCnt++;
            continue;
        }
    }

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    pthread_mutex_unlock(&g_mutexGroupList);

    return ulCnt;

}

static S32 sc_acd_init_agent_queue_cb(VOID *PTR, S32 lCount, S8 **pszData, S8 **pszField)
{
    SC_ACD_AGENT_QUEUE_NODE_ST  *pstAgentQueueNode  = NULL;
    HASH_NODE_S                 *pstHashNode = NULL;
    S8                          *pszSiteID     = NULL, *pszCustomID = NULL;
    S8                          *pszGroupID0   = NULL, *pszGroupID1 = NULL;
    S8                          *pszExten      = NULL, *pszStatus   = NULL;
    S8                          *pszJobNum     = NULL, *pszUserID   = NULL;
    S8                          *pszRecordFlag = NULL, *pszIsHeader = NULL;
    S8                          *pszTelePhone  = NULL, *pszMobile   = NULL;
    S8                          *pszSelectType = NULL;
    S8                          *pszTTNumber = NULL;
    S8                          *pszSIPID = NULL;
    SC_ACD_AGENT_INFO_ST        *pstSiteInfo = NULL;
    SC_ACD_AGENT_INFO_ST        stSiteInfo;
    S32                         lLoop = 0;
    BOOL                        bFound = DOS_FALSE;
    U32                         ulSiteID   = 0, ulCustomID   = 0, ulGroupID0  = 0;
    U32                         ulGroupID1 = 0, ulRecordFlag = 0, ulIsHeader = 0;
    U32                         ulHashIndex = 0, ulIndex = 0, ulRest = 0, ulSelectType = 0;
    U32                         ulAgentIndex = 0, ulSIPID = 0, ulStatus = 0;

    if (DOS_ADDR_INVALID(PTR)
        || DOS_ADDR_INVALID(pszData)
        || DOS_ADDR_INVALID(pszField))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulAgentIndex = *(U32 *)PTR;

    pszSiteID = pszData[0];
    pszStatus= pszData[1];
    pszCustomID = pszData[2];
    pszJobNum = pszData[3];
    pszUserID = pszData[4];
    pszExten = pszData[5];
    pszGroupID0 = pszData[6];
    pszGroupID1 = pszData[7];
    pszRecordFlag = pszData[8];
    pszIsHeader = pszData[9];
    pszSelectType = pszData[10];
    pszTelePhone = pszData[11];
    pszMobile = pszData[12];
    pszTTNumber = pszData[13];
    pszSIPID = pszData[14];

    if (DOS_ADDR_INVALID(pszSiteID)
        || DOS_ADDR_INVALID(pszStatus)
        || DOS_ADDR_INVALID(pszCustomID)
        || DOS_ADDR_INVALID(pszJobNum)
        || DOS_ADDR_INVALID(pszRecordFlag)
        || DOS_ADDR_INVALID(pszIsHeader)
        || DOS_ADDR_INVALID(pszSelectType)
        || dos_atoul(pszSiteID, &ulSiteID) < 0
        || dos_atoul(pszStatus, &ulStatus) < 0
        || dos_atoul(pszCustomID, &ulCustomID) < 0
        || dos_atoul(pszRecordFlag, &ulRecordFlag) < 0
        || dos_atoul(pszIsHeader, &ulIsHeader) < 0
        || dos_atoul(pszSelectType, &ulSelectType) < 0)
    {
        return 0;
    }

    if (DOS_ADDR_VALID(pszGroupID0))
    {
        if (dos_atoul(pszGroupID0, &ulGroupID0) < 0)
        {
            ulGroupID0 = 0;
        }
    }
    else
    {
        ulGroupID0 = 0;
    }
    if (DOS_ADDR_VALID(pszGroupID1))
    {
        if (dos_atoul(pszGroupID1, &ulGroupID1) < 0)
        {
            ulGroupID1 = 0;
        }
    }
    else
    {
        ulGroupID1 = 0;
    }

    if (AGENT_BIND_SIP == ulSelectType)
    {
        if (DOS_ADDR_INVALID(pszSIPID)
            || '\0' == pszSIPID
            || dos_atoul(pszSIPID, &ulSIPID) < 0)
        {
            DOS_ASSERT(0);

            return 0;
        }

        if (DOS_ADDR_INVALID(pszUserID)
            || '\0' == pszUserID[0])
        {
            DOS_ASSERT(0);

            return 0;
        }
    }
    else if (AGENT_BIND_TELE == ulSelectType)
    {
        if (DOS_ADDR_INVALID(pszTelePhone)
            || '\0' == pszTelePhone)
        {
            DOS_ASSERT(0);

            return 0;
        }
    }
    else if (AGENT_BIND_MOBILE == ulSelectType)
    {
        if (DOS_ADDR_INVALID(pszMobile)
            || '\0' == pszMobile[0])
        {
            DOS_ASSERT(0);

            return 0;
        }
    }
    else if (AGENT_BIND_TT_NUMBER == ulSelectType)
    {
        if (DOS_ADDR_INVALID(pszTTNumber)
            || '\0' == pszTTNumber[0])
        {
            DOS_ASSERT(0);

            return 0;
        }
    }

    dos_memzero(&stSiteInfo, sizeof(stSiteInfo));
    stSiteInfo.ulSiteID = ulSiteID;
    stSiteInfo.ucStatus = (U8)ulStatus;
    stSiteInfo.ulCustomerID = ulCustomID;
    stSiteInfo.aulGroupID[0] = ulGroupID0;
    stSiteInfo.aulGroupID[1] = ulGroupID1;
    stSiteInfo.bValid = DOS_TRUE;
    stSiteInfo.bRecord = ulRecordFlag;
    stSiteInfo.bGroupHeader = ulIsHeader;
    stSiteInfo.ucBindType = (U8)ulSelectType;
    stSiteInfo.bLogin = DOS_FALSE;
    stSiteInfo.bWaitingDelete = DOS_FALSE;
    stSiteInfo.bConnected = DOS_FALSE;
    stSiteInfo.ucProcesingTime = 0;
    stSiteInfo.ulSIPUserID = ulSIPID;
    pthread_mutex_init(&stSiteInfo.mutexLock, NULL);

    if (pszUserID && '\0' != pszUserID[0])
    {
        dos_strncpy(stSiteInfo.szUserID, pszUserID, sizeof(stSiteInfo.szUserID));
        stSiteInfo.szUserID[sizeof(stSiteInfo.szUserID) - 1] = '\0';
    }

    if (pszExten && '\0' != pszExten[0])
    {
        dos_strncpy(stSiteInfo.szExtension, pszExten, sizeof(stSiteInfo.szExtension));
        stSiteInfo.szExtension[sizeof(stSiteInfo.szExtension) - 1] = '\0';
    }

    if ('\0' != pszJobNum[0])
    {
        dos_strncpy(stSiteInfo.szEmpNo, pszJobNum, sizeof(stSiteInfo.szEmpNo));
        stSiteInfo.szEmpNo[sizeof(stSiteInfo.szEmpNo) - 1] = '\0';
    }

    if (pszTelePhone && '\0' != pszTelePhone[0])
    {
        dos_strncpy(stSiteInfo.szTelePhone, pszTelePhone, sizeof(stSiteInfo.szTelePhone));
        stSiteInfo.szTelePhone[sizeof(stSiteInfo.szTelePhone) - 1] = '\0';
    }

    if (pszMobile && '\0' != pszMobile[0])
    {
        dos_strncpy(stSiteInfo.szMobile, pszMobile, sizeof(stSiteInfo.szMobile));
        stSiteInfo.szMobile[sizeof(stSiteInfo.szMobile) - 1] = '\0';
    }

    if (pszTTNumber && '\0' != pszTTNumber[0])
    {
        dos_strncpy(stSiteInfo.szTTNumber, pszTTNumber, sizeof(stSiteInfo.szTTNumber));
        stSiteInfo.szTTNumber[sizeof(stSiteInfo.szTTNumber) - 1] = '\0';
    }

    /* 查看当前要添加的坐席是否已经存在，如果存在，就准备更新就好 */
    sc_acd_hash_func4agent(stSiteInfo.ulSiteID, &ulHashIndex);
    pthread_mutex_lock(&g_mutexAgentList);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashIndex , &stSiteInfo.ulSiteID, sc_acd_agent_hash_find);
    if (DOS_ADDR_VALID(pstHashNode)
        && DOS_ADDR_VALID(pstHashNode->pHandle))
    {
        sc_logr_info(SC_ACD, "Agent \"%d\" exist. Update", stSiteInfo.ulSiteID);

        pstAgentQueueNode = pstHashNode->pHandle;
        if (pstAgentQueueNode->pstAgentInfo)
        {
            /* 看看坐席有没有去了别的组，如果是，就需要将坐席换到别的组 */
            for (ulIndex = 0; ulIndex < MAX_GROUP_PER_SITE; ulIndex++)
            {
                if (pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] != U32_BUTT
                   && pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] != 0)
                {
                    /* 修改之前组ID合法，修改之后组ID合法，就需要看看前后两个ID是否相同，相同就不做什么了 */
                    if (stSiteInfo.aulGroupID[ulIndex] != U32_BUTT
                       && stSiteInfo.aulGroupID[ulIndex] != 0 )
                    {
                        if (pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] != stSiteInfo.aulGroupID[ulIndex])
                        {
                            /* 从别的组删除 */
                            sc_logr_debug(SC_ACD, "Agent %u will be removed from Group %u."
                                             , pstAgentQueueNode->pstAgentInfo->ulSiteID
                                             , pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]);

                            ulRest = sc_acd_group_remove_agent(pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]
                                                                , pstAgentQueueNode->pstAgentInfo->ulSiteID);
                            pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] = 0;
                            if (DOS_SUCC == ulRest)
                            {
                                /* 添加到新的组 */
                                sc_logr_debug(SC_ACD, "Agent %u will be added into Group %u."
                                                , stSiteInfo.aulGroupID[ulIndex]
                                                , pstAgentQueueNode->pstAgentInfo->ulSiteID);
                                bFound = DOS_FALSE;
                                for (lLoop = 0; lLoop < MAX_GROUP_PER_SITE; lLoop++)
                                {
                                    if (pstAgentQueueNode->pstAgentInfo->aulGroupID[lLoop] == stSiteInfo.aulGroupID[ulIndex])
                                    {
                                        /* 查找到了坐席中有该坐席组id，标记 */
                                        bFound = DOS_TRUE;
                                        break;
                                    }
                                }
                                /* 没有找到，则找一空闲的节点存放 */
                                if (!bFound)
                                {
                                    for (lLoop = 0; lLoop < MAX_GROUP_PER_SITE; lLoop++)
                                    {
                                        if (0 == pstAgentQueueNode->pstAgentInfo->aulGroupID[lLoop]
                                            || U32_BUTT == pstAgentQueueNode->pstAgentInfo->aulGroupID[lLoop])
                                        {
                                            pstAgentQueueNode->pstAgentInfo->aulGroupID[lLoop] = stSiteInfo.aulGroupID[ulIndex];
                                            break;
                                        }
                                    }
                                }
                                sc_acd_group_add_agent(stSiteInfo.aulGroupID[ulIndex], pstAgentQueueNode->pstAgentInfo);
                                pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] = stSiteInfo.aulGroupID[ulIndex];
                            }
                        }
                    }
                    /* 修改之前组ID合法，修改之后组ID不合法，就需要吧agent从之前的组里面删除掉 */
                    else
                    {
                        sc_logr_debug(SC_ACD, "Agent %u will be removed from group %u."
                                        , pstAgentQueueNode->pstAgentInfo->ulSiteID
                                        , pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]);
                        sc_acd_group_remove_agent(pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]
                                                    , pstAgentQueueNode->pstAgentInfo->ulSiteID);
                        pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] = 0;
                    }
                }
                else
                {
                    /* 修改之前组ID不合法，修改之后组ID合法，就需要吧agent添加到所设置的组 */
                    if (stSiteInfo.aulGroupID[ulIndex] != U32_BUTT
                        && stSiteInfo.aulGroupID[ulIndex] != 0)
                    {
                        /* 添加到新的组 */
                        sc_logr_debug(SC_ACD, "Agent %u will be add into group %u."
                                        , pstAgentQueueNode->pstAgentInfo->ulSiteID
                                        , stSiteInfo.aulGroupID[ulIndex]);
                        bFound = DOS_FALSE;
                        for (lLoop = 0; lLoop < MAX_GROUP_PER_SITE; ++lLoop)
                        {
                            if (pstAgentQueueNode->pstAgentInfo->aulGroupID[lLoop] == stSiteInfo.aulGroupID[ulIndex])
                            {
                                /* 查找到了坐席中有该坐席组id，标记 */
                                bFound = DOS_TRUE;
                                break;
                            }
                        }
                        if (!bFound)
                        {
                            for (lLoop = 0; lLoop < MAX_GROUP_PER_SITE; lLoop++)
                            {
                                if (0 == pstAgentQueueNode->pstAgentInfo->aulGroupID[lLoop]
                                    || U32_BUTT == pstAgentQueueNode->pstAgentInfo->aulGroupID[lLoop])
                                {
                                    pstAgentQueueNode->pstAgentInfo->aulGroupID[lLoop] = stSiteInfo.aulGroupID[ulIndex];
                                    break;
                                }
                            }
                        }
                        sc_acd_group_add_agent(stSiteInfo.aulGroupID[ulIndex], pstAgentQueueNode->pstAgentInfo);
                    }
                    else
                    {
                        /* 修改之前组ID不合法，修改之后组ID不合法，就啥也不做哩 */
                    }
                }
            }

            pstAgentQueueNode->pstAgentInfo->bRecord = stSiteInfo.bRecord;
            pstAgentQueueNode->pstAgentInfo->bAllowAccompanying = stSiteInfo.bAllowAccompanying;
            pstAgentQueueNode->pstAgentInfo->bGroupHeader = stSiteInfo.bGroupHeader;
            pstAgentQueueNode->pstAgentInfo->ucBindType = stSiteInfo.ucBindType;
            pstAgentQueueNode->pstAgentInfo->ucStatus = stSiteInfo.ucStatus;
            pstAgentQueueNode->pstAgentInfo->ulCustomerID = stSiteInfo.ulCustomerID;
            pstAgentQueueNode->pstAgentInfo->bValid = stSiteInfo.bValid;
            pstAgentQueueNode->pstAgentInfo->ulSIPUserID = stSiteInfo.ulSIPUserID;

            dos_strncpy(pstAgentQueueNode->pstAgentInfo->szEmpNo, stSiteInfo.szEmpNo,SC_EMP_NUMBER_LENGTH);
            pstAgentQueueNode->pstAgentInfo->szEmpNo[SC_EMP_NUMBER_LENGTH - 1] = '\0';

            if (pszExten &&  '\0' != pszExten[0])
            {
                dos_strncpy(pstAgentQueueNode->pstAgentInfo->szExtension, stSiteInfo.szExtension,SC_TEL_NUMBER_LENGTH);
                pstAgentQueueNode->pstAgentInfo->szExtension[SC_TEL_NUMBER_LENGTH - 1] = '\0';
            }

            if (pszUserID && '\0' != pszUserID[0])
            {
                dos_strncpy(pstAgentQueueNode->pstAgentInfo->szUserID, stSiteInfo.szUserID,SC_TEL_NUMBER_LENGTH);
                pstAgentQueueNode->pstAgentInfo->szUserID[SC_TEL_NUMBER_LENGTH - 1] = '\0';
            }

            if (pszTelePhone && '\0' != pszTelePhone[0])
            {
                dos_strncpy(pstAgentQueueNode->pstAgentInfo->szTelePhone, stSiteInfo.szTelePhone,SC_TEL_NUMBER_LENGTH);
                pstAgentQueueNode->pstAgentInfo->szTelePhone[SC_TEL_NUMBER_LENGTH - 1] = '\0';
            }

            if (pszMobile && '\0' != pszMobile[0])
            {
                dos_strncpy(pstAgentQueueNode->pstAgentInfo->szMobile, stSiteInfo.szMobile,SC_TEL_NUMBER_LENGTH);
                pstAgentQueueNode->pstAgentInfo->szMobile[SC_TEL_NUMBER_LENGTH - 1] = '\0';
            }

            if (pszTTNumber && '\0' != pszTTNumber[0])
            {
                dos_strncpy(pstAgentQueueNode->pstAgentInfo->szTTNumber, stSiteInfo.szTTNumber, SC_TEL_NUMBER_LENGTH);
                pstAgentQueueNode->pstAgentInfo->szTTNumber[SC_TEL_NUMBER_LENGTH - 1] = '\0';
            }
        }

        SC_TRACE_OUT();
        pthread_mutex_unlock(&g_mutexAgentList);
        return 0;
    }
    pthread_mutex_unlock(&g_mutexAgentList);

    pstSiteInfo = sc_acd_add_agent(&stSiteInfo);
    if (DOS_ADDR_INVALID(pstSiteInfo))
    {
        DOS_ASSERT(0);
        return 0;
    }

    /* 将坐席加入到组 */
    if (ulAgentIndex != SC_INVALID_INDEX)
    {
        for (ulIndex = 0; ulIndex < MAX_GROUP_PER_SITE; ulIndex++)
        {
            if (0 == stSiteInfo.aulGroupID[ulIndex] || U32_BUTT == stSiteInfo.aulGroupID[ulIndex])
            {
                continue;
            }
            bFound = DOS_FALSE;
            for (lLoop = 0; lLoop < MAX_GROUP_PER_SITE; ++lLoop)
            {
                if (pstSiteInfo->aulGroupID[lLoop] == stSiteInfo.aulGroupID[ulIndex])
                {
                    bFound = DOS_TRUE;
                    break;
                }
            }
            if (!bFound)
            {
                for (lLoop = 0; lLoop < MAX_GROUP_PER_SITE; lLoop++)
                {
                    if (0 == pstSiteInfo->aulGroupID[lLoop]
                        || U32_BUTT == pstSiteInfo->aulGroupID[lLoop])
                    {
                        pstSiteInfo->aulGroupID[lLoop] = stSiteInfo.aulGroupID[ulIndex];
                        break;
                    }
                }
            }

            if (sc_acd_group_add_agent(stSiteInfo.aulGroupID[ulIndex], pstSiteInfo) != DOS_SUCC)
            {
                DOS_ASSERT(0);
            }
        }
    }
    return 0;
}

static U32 sc_acd_init_agent_queue(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT " \
                      "    a.id, a.status, a.customer_id, a.job_number,b.userid, b.extension, a.group1_id, a.group2_id, " \
                      "    a.voice_record, a.class, a.select_type, a.fixed_telephone, a.mobile_number, " \
                      "    a.tt_number, a.sip_id " \
                      "FROM " \
                      "    tbl_agent a " \
                      "LEFT JOIN" \
                      "    tbl_sip b "\
                      "ON "\
                      "    b.id = a.sip_id;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT " \
                      "    a.id, a.status, a.customer_id, a.job_number,b.userid, b.extension, a.group1_id, a.group2_id, " \
                      "    a.voice_record, a.class, a.select_type, a.fixed_telephone, a.mobile_number, " \
                      "    a.tt_number, a.sip_id " \
                      "FROM " \
                      "    tbl_agent a " \
                      "LEFT JOIN" \
                      "    tbl_sip b " \
                      "ON "\
                      "    b.id = a.sip_id " \
                      "WHERE " \
                      "    a.id = %u;",
                      ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_acd_init_agent_queue_cb, &ulIndex, NULL) < 0)
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

U32 sc_acd_init_group_queue(U32 ulIndex)
{
    S8 szSql[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSql, sizeof(szSql), "SELECT id,customer_id,acd_policy,`name` from tbl_group;");
    }
    else
    {
        dos_snprintf(szSql, sizeof(szSql), "SELECT id,customer_id,acd_policy,`name` from tbl_group WHERE id=%d;", ulIndex);
    }

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
    U32                         ulIndex               = 0;

    if (DOS_ADDR_INVALID(pNode)
        || DOS_ADDR_INVALID(pNode->pHandle))
    {
        DOS_ASSERT(0);
        return ;
    }

    pstAgentQueueNode = pNode->pHandle;
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
            sc_acd_group_add_agent(pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex], pstAgentQueueNode->pstAgentInfo);
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

    if (sc_acd_init_group_queue(SC_INVALID_INDEX) != DOS_SUCC)
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


    if (sc_acd_init_agent_queue(SC_INVALID_INDEX) != DOS_SUCC)
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

/**
 * 函数: U32 sc_acd_http_agent_update_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID)
 * 功能: 处理HTTP发过来的命令
 * 参数:
 *      U32 ulAction : 命令
 *      U32 ulAgentID : 坐席ID
 *      S8 *pszUserID : 坐席SIP User ID
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
U32 sc_acd_http_agent_update_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID)
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
            sc_acd_update_agent_status(ulAction, ulAgentID);
            break;
        case SC_ACD_SITE_ACTION_ADD:
        case SC_ACD_SITE_ACTION_UPDATE:
            sc_acd_init_agent_queue(ulAgentID);
            break;
        default:
            DOS_ASSERT(0);
            return DOS_FAIL;
            break;
    }

    return DOS_SUCC;
}

U32 sc_acd_http_agentgrp_update_proc(U32 ulAction, U32 ulGrpID)
{
    if (ulAction > SC_ACD_SITE_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch(ulAction)
    {
        case SC_API_CMD_ACTION_AGENTGREP_ADD:
        case SC_API_CMD_ACTION_AGENTGREP_UPDATE:
            sc_acd_init_group_queue(ulGrpID);
            break;

        case SC_API_CMD_ACTION_AGENTGREP_DELETE:
            sc_acd_delete_queue(ulGrpID);
            break;

        default:
            break;
    }
    return DOS_SUCC;
}

/* 记录坐席统计信息 */
U32 sc_acd_save_agent_stat(SC_ACD_AGENT_INFO_ST *pstAgentInfo)
{
    S8 szSQL[512] = { 0, };

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 没有上线过，就直接过了 */
    if (pstAgentInfo->stStat.ulTimeOnSignin)
    {
        return DOS_SUCC;
    }

    dos_snprintf(szSQL, sizeof(szSQL),
                    "INSERT INTO tbl_stat_agents(ctime, bid, \"job_number\", calls, group_id"
                    "calls_connected, total_duration, avg_call_duration, online_time) VALUES("
                    "%u, %u, %s, %u, %u, %u, %u, %u, %u)"
                , time(NULL), pstAgentInfo->ulSiteID, pstAgentInfo->szEmpNo
                , pstAgentInfo->stStat.ulCallCnt, pstAgentInfo->aulGroupID[0]
                , pstAgentInfo->stStat.ulCallCnt, pstAgentInfo->stStat.ulTimeOnSignin, 0, 0);

    if (db_query(g_pstSCDBHandle, szSQL, NULL, NULL, NULL) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/* 审计坐席，其实就 写入统计信息 */
U32 sc_acd_agent_audit(U32 ulCycle, VOID *ptr)
{
    HASH_NODE_S                 *pstHashNode       = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST  *pstAgentQueueNode = NULL;
    SC_ACD_AGENT_INFO_ST        *pstAgentInfo      = NULL;
    U32             ulHashIndex  = 0;

    SC_TRACE_IN(0, 0, 0, 0);

    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                DOS_ASSERT(0);
                continue;
            }

            pstAgentQueueNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentQueueNode)
                || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
            {
                /* 有可能被删除了，不需要断言 */
                continue;
            }

            pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
            if (pstAgentInfo->bWaitingDelete)
            {
                /* 被删除了*/
                continue;
            }

            sc_acd_save_agent_stat(pstAgentInfo);
        }
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}



