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


/* 释放链表结点 */
VOID bs_free_node(VOID *pNode)
{
    DLL_NODE_S *pstNode;

    if(DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return;
    }

    pstNode = (DLL_NODE_S *)pNode;
    if(DOS_ADDR_INVALID(pstNode->pHandle))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pNode);
        return;
    }

    dos_dmem_free(pstNode->pHandle);
    dos_dmem_free(pNode);

}

/* 判断客户哈希表节点是否匹配 */
S32 bs_customer_hash_node_match(VOID *pKey, HASH_NODE_S *pNode)
{
    BS_CUSTOMER_ST  *pstCustomer = NULL;

    if (DOS_ADDR_INVALID(pKey) || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCustomer = (BS_CUSTOMER_ST *)pNode->pHandle;
    if (DOS_ADDR_INVALID(pstCustomer))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstCustomer->ulCustomerID == *(U32 *)pKey)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/* 判断客户哈希表节点是否匹配 */
S32 bs_agent_hash_node_match(VOID *pKey, HASH_NODE_S *pNode)
{
    BS_AGENT_ST     *pstAgent = NULL;

    if (DOS_ADDR_INVALID(pKey) || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstAgent = (BS_AGENT_ST *)pNode->pHandle;
    if (DOS_ADDR_INVALID(pstAgent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstAgent->ulAgentID == *(U32 *)pKey)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}


/* 判断资费哈希表节点是否匹配 */
S32 bs_billing_package_hash_node_match(VOID *pKey, HASH_NODE_S *pNode)
{
    BS_BILLING_PACKAGE_ST   *pstMatch = NULL;
    BS_BILLING_PACKAGE_ST   *pstBillingPackage = NULL;

    if (DOS_ADDR_INVALID(pKey) || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstBillingPackage = (BS_BILLING_PACKAGE_ST *)pNode->pHandle;
    if (DOS_ADDR_INVALID(pstBillingPackage))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstMatch = (BS_BILLING_PACKAGE_ST *)pKey;
    if (pstBillingPackage->ulPackageID == pstMatch->ulPackageID
        && pstBillingPackage->ucServType == pstMatch->ucServType)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/* 判断结算哈希表节点是否匹配 */
S32 bs_settle_hash_node_match(VOID *pKey, HASH_NODE_S *pNode)
{
    BS_SETTLE_ST    *pstMatch = NULL;
    BS_SETTLE_ST    *pstSettle = NULL;

    if (DOS_ADDR_INVALID(pKey) || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSettle = (BS_SETTLE_ST *)pNode->pHandle;
    if (DOS_ADDR_INVALID(pstSettle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstMatch = (BS_SETTLE_ST *)pKey;
    if (pstSettle->usTrunkID == pstMatch->usTrunkID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/* 判断任务哈希表节点是否匹配 */
S32 bs_task_hash_node_match(VOID *pKey, HASH_NODE_S *pNode)
{
    BS_TASK_ST     *pstTask = NULL;

    if (DOS_ADDR_INVALID(pKey) || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstTask = (BS_TASK_ST *)pNode->pHandle;
    if (DOS_ADDR_INVALID(pstTask))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstTask->ulTaskID == *(U32 *)pKey)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/*
说明:根据ID和哈希表大小获取哈希索引,U32_BUTT为无效值
提示:为提高效率,要求哈希表大小为2的整数幂
*/
U32 bs_hash_get_index(U32 ulHashTblSize, U32 ulID)
{
    if (U32_BUTT == ulID)
    {
        return U32_BUTT;
    }

    return ulID & (ulHashTblSize - 1);
}

/* 获取客户信息在哈希表中的节点 */
HASH_NODE_S *bs_get_customer_node(U32 ulCustomerID)
{
    U32             ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;

    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_CUSTOMER_SIZE, ulCustomerID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    pthread_mutex_lock(&g_mutexCustomerTbl);
    pstHashNode = hash_find_node(g_astCustomerTbl, ulHashIndex,
                                  (VOID *)&ulCustomerID,
                                  bs_customer_hash_node_match);
    pthread_mutex_unlock(&g_mutexCustomerTbl);

    return pstHashNode;

}

/* 获取客户信息结构体 */
BS_CUSTOMER_ST *bs_get_customer_st(U32 ulCustomerID)
{
    HASH_NODE_S     *pstHashNode = NULL;

    pstHashNode = bs_get_customer_node(ulCustomerID);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        return NULL;
    }

    return (BS_CUSTOMER_ST *)pstHashNode->pHandle;

}

/* 获取坐席信息结构体 */
BS_AGENT_ST *bs_get_agent_st(U32 ulAgentID)
{
    U32             ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;

    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_AGENT_SIZE, ulAgentID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    pthread_mutex_lock(&g_mutexAgentTbl);
    pstHashNode = hash_find_node(g_astAgentTbl, ulHashIndex,
                                  (VOID *)&ulAgentID,
                                  bs_agent_hash_node_match);
    pthread_mutex_unlock(&g_mutexAgentTbl);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        return NULL;
    }

    return (BS_AGENT_ST *)pstHashNode->pHandle;

}

/* 获取结算信息结构体 */
BS_SETTLE_ST *bs_get_settle_st(U16 usTrunkID)
{
    U32             ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;
    BS_SETTLE_ST    stSettle;

    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_SETTLE_SIZE, usTrunkID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    stSettle.usTrunkID = usTrunkID;
    pthread_mutex_lock(&g_mutexSettleTbl);
    pstHashNode = hash_find_node(g_astSettleTbl, ulHashIndex,
                                  (VOID *)&stSettle,
                                  bs_settle_hash_node_match);
    pthread_mutex_unlock(&g_mutexSettleTbl);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        return NULL;
    }

    return (BS_SETTLE_ST *)pstHashNode->pHandle;

}

/* 获取任务信息结构体 */
BS_TASK_ST *bs_get_task_st(U32 ulTaskID)
{
    U32             ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;

    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_TASK_SIZE, ulTaskID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    pthread_mutex_lock(&g_mutexTaskTbl);
    pstHashNode = hash_find_node(g_astTaskTbl, ulHashIndex,
                                  (VOID *)&ulTaskID,
                                  bs_task_hash_node_match);
    pthread_mutex_unlock(&g_mutexTaskTbl);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        return NULL;
    }

    return (BS_TASK_ST *)pstHashNode->pHandle;

}


/* 客户树添加子节点函数 */
VOID bs_customer_add_child(BS_CUSTOMER_ST *pstCustomer, BS_CUSTOMER_ST *pstChildCustomer)
{
    DLL_NODE_S      *pstNode = NULL;

    if (DOS_ADDR_INVALID(pstCustomer) || DOS_ADDR_INVALID(pstChildCustomer))
    {
        DOS_ASSERT(0);
        return;
    }

    /* 查找是否已经添加过该子节点,注意match函数中应该使用HASH_NODE_S,因与DLL_NODE_S结构相同,故能调用 */
    pstNode = dll_find(&pstCustomer->stChildrenList, (VOID *)&pstChildCustomer->ulCustomerID, bs_customer_hash_node_match);
    if (DOS_ADDR_VALID(pstNode))
    {
        /* 节点已经存在 */
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_WARNING, "Warning: the child(%u:%s) of customer(%u:%s) is already exist when build tree!",
                 pstChildCustomer->ulCustomerID, pstChildCustomer->szCustomerName,
                 pstCustomer->ulCustomerID, pstCustomer->szCustomerName);
        return;
    }

    DLL_Add(&pstCustomer->stChildrenList, &pstChildCustomer->stNode);
}

/* 获取应用层socket相关信息 */
BSS_APP_CONN *bs_get_app_conn(BS_MSG_TAG *pstMsgTag)
{
    S32     i;

    if (DOS_ADDR_INVALID(pstMsgTag))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    for (i = 0; i < BS_MAX_APP_CONN_NUM; i++)
    {
        if (g_stBssCB.astAPPConn[i].bIsConn
            && pstMsgTag->aulIPAddr[0] == g_stBssCB.astAPPConn[i].aulIPAddr[0]
            && pstMsgTag->aulIPAddr[1] == g_stBssCB.astAPPConn[i].aulIPAddr[1]
            && pstMsgTag->aulIPAddr[2] == g_stBssCB.astAPPConn[i].aulIPAddr[2]
            && pstMsgTag->aulIPAddr[3] == g_stBssCB.astAPPConn[i].aulIPAddr[3])
        {
            return &g_stBssCB.astAPPConn[i];
        }
    }

    return NULL;
}

/* 保存应用层socket相关信息,目前只实现IPv4 */
BSS_APP_CONN *bs_save_app_conn(S32 lSockfd, U8 *pstAddrIn, U32 ulAddrinHeader, S32 lAddrLen)
{
    S32                 i, lIdlePos = -1;
    BSS_APP_CONN        *pstAppConn = NULL;
    struct sockaddr_in  *pstAddr;
    struct sockaddr_un  *pstUnAddr;

    if (DOS_ADDR_INVALID(pstAddrIn))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /* 查找地址是否已经保存过,如果保存过则忽略 */
    for (i = 0; i < BS_MAX_APP_CONN_NUM; i++)
    {
        if (g_stBssCB.astAPPConn[i].bIsValid)
        {
            if (0 == dos_memcmp(pstAddrIn, &g_stBssCB.astAPPConn[i].stAddr, sizeof(struct sockaddr_in)))
            {
                pstAppConn = &g_stBssCB.astAPPConn[i];

                /* 如果是unix socket，需要更新一下标示符 */
                if (BSCOMM_PROTO_UNIX == g_stBssCB.ulCommProto
                    && ulAddrinHeader != g_stBssCB.astAPPConn[i].aulIPAddr[0])
                {
                    pstAppConn->aulIPAddr[0] = ulAddrinHeader;
                }

                break;
            }
        }
        else if (-1 == lIdlePos)
        {
            lIdlePos = i;
        }
    }

    if (NULL == pstAppConn && lIdlePos != -1)
    {
        /* 未保存过,则在空闲位置保存信息 */
        pstAppConn = &g_stBssCB.astAPPConn[lIdlePos];
        pstAppConn->bIsValid = DOS_TRUE;
        pstAppConn->bIsConn = DOS_TRUE;
        pstAppConn->ucCommType = (U8)g_stBssCB.ulCommProto;
        pstAppConn->ulReserv = 0;
        pstAppConn->ulMsgSeq = 0;
        pstAppConn->lSockfd = lSockfd;
        pstAppConn->lAddrLen = lAddrLen;

        if (g_stBssCB.ulCommProto != BSCOMM_PROTO_UNIX)
        {
            pstAddr = (struct sockaddr_in *)pstAddrIn;

            pstAppConn->aulIPAddr[0] = pstAddr->sin_addr.s_addr;
            pstAppConn->aulIPAddr[1] = 0;
            pstAppConn->aulIPAddr[2] = 0;
            pstAppConn->aulIPAddr[3] = 0;
            pstAppConn->stAddr.stInAddr.sin_family = AF_INET;
            pstAppConn->stAddr.stInAddr.sin_port = pstAddr->sin_port;
            pstAppConn->stAddr.stInAddr.sin_addr.s_addr = pstAddr->sin_addr.s_addr;
        }
        else
        {
            pstAppConn->aulIPAddr[0] = ulAddrinHeader;
            pstAppConn->aulIPAddr[1] = 0;
            pstAppConn->aulIPAddr[2] = 0;
            pstAppConn->aulIPAddr[3] = 0;

            pstUnAddr = (struct sockaddr_un *)pstAddrIn;
            pstAppConn->stAddr.stUnAddr.sun_family = AF_UNIX;
            dos_strncpy(pstAppConn->stAddr.stUnAddr.sun_path, pstUnAddr->sun_path, lAddrLen);
            pstAppConn->stAddr.stUnAddr.sun_path[lAddrLen] = '\0';

            bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG, "New unix socket %s, len: %d", pstAppConn->stAddr.stUnAddr.sun_path, lAddrLen);
        }
    }

    return pstAppConn;
}

/* 统计坐席数量 */
VOID bs_stat_agent_num(VOID)
{
    U32             ulCnt = 0, ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;
    BS_AGENT_ST     *pstAgent = NULL;
    BS_CUSTOMER_ST  *pstCustomer = NULL;

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Stat agent number!");

    pthread_mutex_lock(&g_mutexAgentTbl);
    HASH_Scan_Table(g_astAgentTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astAgentTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            ulCnt++;
            if (ulCnt > g_astAgentTbl->NodeNum)
            {
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR,
                         "Err: agent number is exceed in hash table(%u)!",
                         g_astAgentTbl->NodeNum);
                pthread_mutex_unlock(&g_mutexAgentTbl);
                return;
            }

            pstAgent = (BS_AGENT_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgent))
            {
                /* 节点未保存客户信息,异常,按道理应该释放该节点,但考虑到总体节点数量相对固定,内存不存在泄露风险,保留之 */
                DOS_ASSERT(0);
                ulCnt--;
                continue;
            }

            /* 获取客户信息结构体 */
            pstCustomer = bs_get_customer_st(pstAgent->ulCustomerID);
            if (NULL == pstCustomer)
            {
                /* 找不到 */
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR,
                         "Err: unknown agent! agent:%u, customer:%u",
                         pstAgent->ulAgentID, pstAgent->ulCustomerID);
                continue;
            }

            /* 更新客户控制块信息 */
            pstCustomer->ulAgentNum++;
        }
    }
    pthread_mutex_unlock(&g_mutexAgentTbl);

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Total %d agent!", ulCnt);

}

/* 根据中继编号查找资费包ID */
U32 bs_get_settle_packageid(U16 usTrunkID)
{
    U32             ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;
    BS_SETTLE_ST    *pstSettle = NULL;
    BS_SETTLE_ST    stSettle;

    stSettle.usTrunkID = usTrunkID;
    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_SETTLE_SIZE, usTrunkID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    pthread_mutex_lock(&g_mutexSettleTbl);
    pstHashNode = hash_find_node(g_astSettleTbl,
                                 ulHashIndex,
                                 (VOID *)&stSettle,
                                 bs_settle_hash_node_match);
    pthread_mutex_unlock(&g_mutexSettleTbl);

    pstSettle = (BS_SETTLE_ST *)pstHashNode->pHandle;
    if (DOS_ADDR_INVALID(pstSettle))
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    return pstSettle->ulPackageID;
}

/* 判断是否属于信息类业务 */
BOOL bs_is_message_service(U8 ucServType)
{
    if (BS_SERV_MMS_RECV == ucServType
        || BS_SERV_MMS_SEND == ucServType
        || BS_SERV_SMS_RECV == ucServType
        || BS_SERV_SMS_SEND == ucServType)
    {
        return DOS_TRUE;
    }

    return DOS_FALSE;
}

/* 查找特定业务类型的计费包 */
BS_BILLING_PACKAGE_ST *bs_get_billing_package(U32 ulPackageID, U8 ucServType)
{
    U32                     ulHashIndex;
    BS_BILLING_PACKAGE_ST   stMatch;
    HASH_NODE_S             *pstHashNode = NULL;

    stMatch.ulPackageID = ulPackageID;
    stMatch.ucServType = ucServType;

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_INFO, "Get billing package %d for service %d", ulPackageID, ucServType);

    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_BILLING_PACKAGE_SIZE, ulPackageID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    pthread_mutex_lock(&g_mutexBillingPackageTbl);
    pstHashNode = hash_find_node(g_astBillingPackageTbl, ulHashIndex,
                                  (VOID *)&stMatch,
                                  bs_billing_package_hash_node_match);
    pthread_mutex_unlock(&g_mutexBillingPackageTbl);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        return NULL;
    }

    return (BS_BILLING_PACKAGE_ST *)pstHashNode->pHandle;
}

/* 判断计费规则是否恰当 */
BOOL bs_billing_rule_is_properly(BS_BILLING_RULE_ST  *pstRule)
{
    BOOL    bIsProperly = DOS_FALSE;
    U32     ulMatch;

    /* 调用前指针合法性已确认,不再判断 */

    if (0 == pstRule->ulPackageID
        || U32_BUTT == pstRule->ulPackageID
        || 0 == pstRule->ulRuleID
        || U32_BUTT == pstRule->ulRuleID)
    {
        /* 计费规则未配置 */
        return bIsProperly;
    }

    if (0 == pstRule->ucServType)
    {
        /* 本条规则未配置 */
        return bIsProperly;
    }

    /* 为提高比较效率,使用位移与或的计算方式;潜在要求:属性值不得超过31 */
    switch (pstRule->ucBillingType)
    {
        case BS_BILLING_BY_TIMELEN:
            ulMatch = (1<<BS_BILLING_ATTR_REGION)|(1<<BS_BILLING_ATTR_TEL_OPERATOR)
                      |(1<<BS_BILLING_ATTR_NUMBER_TYPE)|(1<<BS_BILLING_ATTR_TIME)
                      |(1<<BS_BILLING_ATTR_ACCUMULATE_TIMELEN);
            if (((0 == pstRule->ucSrcAttrType1) || ((1<<pstRule->ucSrcAttrType1)&ulMatch))
                && ((0 == pstRule->ucSrcAttrType2) || ((1<<pstRule->ucSrcAttrType2)&ulMatch))
                && ((0 == pstRule->ucDstAttrType1) || ((1<<pstRule->ucDstAttrType1)&ulMatch))
                && ((0 == pstRule->ucDstAttrType2) || ((1<<pstRule->ucDstAttrType2)&ulMatch)))
            {
                bIsProperly = DOS_TRUE;
            }
            break;

        case BS_BILLING_BY_COUNT:
            ulMatch = (1<<BS_BILLING_ATTR_REGION)|(1<<BS_BILLING_ATTR_TEL_OPERATOR)
                      |(1<<BS_BILLING_ATTR_NUMBER_TYPE)|(1<<BS_BILLING_ATTR_TIME)
                      |(1<<BS_BILLING_ATTR_ACCUMULATE_COUNT);
            if (((0 == pstRule->ucSrcAttrType1) || ((1<<pstRule->ucSrcAttrType1)&ulMatch))
                && ((0 == pstRule->ucSrcAttrType2) || ((1<<pstRule->ucSrcAttrType2)&ulMatch))
                && ((0 == pstRule->ucDstAttrType1) || ((1<<pstRule->ucDstAttrType1)&ulMatch))
                && ((0 == pstRule->ucDstAttrType2) || ((1<<pstRule->ucDstAttrType2)&ulMatch)))
            {
                bIsProperly = DOS_TRUE;
            }
            break;

        case BS_BILLING_BY_TRAFFIC:
            /* 暂不支持 */
            break;

        case BS_BILLING_BY_CYCLE:
            ulMatch = (1<<BS_BILLING_ATTR_CONCURRENT)
                      |(1<<BS_BILLING_ATTR_SET)
                      |(1<<BS_BILLING_ATTR_RESOURCE_AGENT)
                      |(1<<BS_BILLING_ATTR_RESOURCE_NUMBER)
                      |(1<<BS_BILLING_ATTR_RESOURCE_LINE);

            /* pstRule->ucSrcAttrType1 为计费对象(坐席，线路，等)， pstRule->ucSrcAttrType2为周期(日，周，月)*/
            /* 因此pstRule->ucSrcAttrType2不能参与下面的比较 */
            if (BS_SERV_RENT == pstRule->ucServType
                && ((0 == pstRule->ucSrcAttrType1) || ((1<<pstRule->ucSrcAttrType1)&ulMatch))
                && ((0 == pstRule->ucDstAttrType1) || ((1<<pstRule->ucDstAttrType1)&ulMatch))
                && ((0 == pstRule->ucDstAttrType2) || ((1<<pstRule->ucDstAttrType2)&ulMatch))
                && pstRule->ulBillingRate != 0)
            {
                bIsProperly = DOS_TRUE;
            }
            break;

        default:
            break;
    }

    return bIsProperly;

}

/* 获取计费属性参数输入 */
VOID *bs_get_attr_input(BS_BILLING_MATCH_ST *pstBillingMatch, U8 ucAttrType, BOOL bIsSrc)
{
    VOID    *pAddr = NULL;

    if (DOS_ADDR_INVALID(pstBillingMatch))
    {
        DOS_ASSERT(0);
        return pAddr;
    }

    if (U8_BUTT == ucAttrType && 0 == ucAttrType)
    {
        return NULL;
    }

    switch (ucAttrType)
    {
        case BS_BILLING_ATTR_REGION:
        case BS_BILLING_ATTR_TEL_OPERATOR:
        case BS_BILLING_ATTR_NUMBER_TYPE:
            if (bIsSrc)
            {
                pAddr = (VOID *)pstBillingMatch->szCaller;
            }
            else
            {
                pAddr = (VOID *)pstBillingMatch->szCallee;
            }
            break;

        case BS_BILLING_ATTR_TIME:
            pAddr = (VOID *)&pstBillingMatch->ulTimeStamp;
            break;

        case BS_BILLING_ATTR_ACCUMULATE_TIMELEN:
        case BS_BILLING_ATTR_ACCUMULATE_COUNT:
        case BS_BILLING_ATTR_ACCUMULATE_TRAFFIC:
            /* 累计类 */
            pAddr = (VOID *)pstBillingMatch;
            break;

        case BS_BILLING_ATTR_CONCURRENT:
        case BS_BILLING_ATTR_SET:
        case BS_BILLING_ATTR_RESOURCE_AGENT:
        case BS_BILLING_ATTR_RESOURCE_NUMBER:
        case BS_BILLING_ATTR_RESOURCE_LINE:
            /* 资源都是分配到客户的 */
            pAddr = (VOID *)&pstBillingMatch->ulComsumerID;
            break;

        default:
            break;

    }

    return pAddr;

}

/* 获取号码归属地 */
U32 bs_get_number_region(S8 *szNum, U32 ulStrLen)
{
    if (DOS_ADDR_INVALID(szNum))
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    //TODO查询数据库,获取号码归属地信息;特别的,号码归属地信息最好一次性读到内存再使用

    /*
    返回值32bit定义:国家代码(32-17bit),省编号(16-11bit),区号(10-1bit)
    #define BS_MASK_REGION_COUNTRY          0xFFFF0000
    #define BS_MASK_REGION_PROVINCE         0xFC00
    #define BS_MASK_REGION_CITY             0x3FF
    */

    return U32_BUTT;
}

/* 获取号码所属电信运营商 */
U32 bs_get_number_teloperator(S8 *szNum, U32 ulStrLen)
{
    if (DOS_ADDR_INVALID(szNum))
    {
        DOS_ASSERT(0);
        return BS_OPERATOR_UNKNOWN;
    }

    //TODO查询数据库,获取号码所属运营商信息


    return BS_OPERATOR_UNKNOWN;
}

/* 获取号码类型 */
U32 bs_get_number_type(S8 *szNum, U32 ulStrLen)
{
    if (DOS_ADDR_INVALID(szNum))
    {
        DOS_ASSERT(0);
        return BS_NUMBER_TYPE_BUTT;
    }

    //TODO查询数据库,获取号码所属运营商信息


    return BS_NUMBER_TYPE_BUTT;
}

/* 判断时间是否在特定的时段内 */
BOOL bs_time_is_in_period(U32 *pulTimeStamp, U32 ulPeriodID)
{
    if (DOS_ADDR_INVALID(pulTimeStamp))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    //TODO根据时段ID查询数据库,获取时段定义

    /* 注意:时段定义不一定是连续的,例如每周固定时间,每天固定时间段等 */

    return BS_NUMBER_TYPE_BUTT;
}

/* 获取累计规则 */
BS_ACCUMULATE_RULE *bs_get_accumulate_rule(U32 ulRuleID)
{
    //TODO根据ID查询数据库,获取累计规则定义(须读取到内存)


    return NULL;
}

/* 判断通话中止原因是否为被叫忙 */
BOOL bs_cause_is_busy(U16 usCause)
{
    //TODO
    return DOS_FALSE;
}

/* 判断通话中止原因是否为被叫不存在 */
BOOL bs_cause_is_not_exist(U16 usCause)
{
    //TODO
    return DOS_FALSE;
}

/* 判断通话中止原因是否为被叫无应答 */
BOOL bs_cause_is_no_answer(U16 usCause)
{
    //TODO
    return DOS_FALSE;
}

/* 判断通话中止原因是否为被叫拒绝 */
BOOL bs_cause_is_reject(U16 usCause)
{
    //TODO
    return DOS_FALSE;
}

/* 出局统计刷新 */
VOID bs_outband_stat_refresh(BS_STAT_OUTBAND *pstDst, BS_STAT_OUTBAND *pstSrc)
{
    pstDst->ulTimeStamp = pstSrc->ulTimeStamp;
    pstDst->ulCallCnt += pstSrc->ulCallCnt;
    pstDst->ulRingCnt += pstSrc->ulRingCnt;
    pstDst->ulBusyCnt += pstSrc->ulBusyCnt;
    pstDst->ulNotExistCnt += pstSrc->ulNotExistCnt;
    pstDst->ulNoAnswerCnt += pstSrc->ulNoAnswerCnt;
    pstDst->ulRejectCnt += pstSrc->ulRejectCnt;
    pstDst->ulEarlyReleaseCnt += pstSrc->ulEarlyReleaseCnt;
    pstDst->ulAnswerCnt += pstSrc->ulAnswerCnt;
    pstDst->ulPDD += pstSrc->ulPDD;
    pstDst->ulAnswerTime += pstSrc->ulAnswerTime;

}

/* 入局统计刷新 */
VOID bs_inband_stat_refresh(BS_STAT_INBAND *pstDst, BS_STAT_INBAND *pstSrc)
{
    pstDst->ulTimeStamp = pstSrc->ulTimeStamp;
    pstDst->ulCallCnt += pstSrc->ulCallCnt;
    pstDst->ulRingCnt += pstSrc->ulRingCnt;
    pstDst->ulBusyCnt += pstSrc->ulBusyCnt;
    pstDst->ulNoAnswerCnt += pstSrc->ulNoAnswerCnt;
    pstDst->ulEarlyReleaseCnt += pstSrc->ulEarlyReleaseCnt;
    pstDst->ulAnswerCnt += pstSrc->ulAnswerCnt;
    pstDst->ulConnAgentCnt += pstSrc->ulConnAgentCnt;
    pstDst->ulAgentAnswerCnt += pstSrc->ulAgentAnswerCnt;
    pstDst->ulHoldCnt += pstSrc->ulHoldCnt;
    pstDst->ulAnswerTime += pstSrc->ulAnswerTime;
    pstDst->ulWaitAgentTime += pstSrc->ulWaitAgentTime;
    pstDst->ulAgentAnswerTime += pstSrc->ulAgentAnswerTime;
    pstDst->ulHoldTime += pstSrc->ulHoldTime;

}

/* 外呼统计刷新 */
VOID bs_outdialing_stat_refresh(BS_STAT_OUTDIALING *pstDst, BS_STAT_OUTDIALING *pstSrc)
{
    pstDst->ulTimeStamp = pstSrc->ulTimeStamp;
    pstDst->ulCallCnt += pstSrc->ulCallCnt;
    pstDst->ulRingCnt += pstSrc->ulRingCnt;
    pstDst->ulBusyCnt += pstSrc->ulBusyCnt;
    pstDst->ulNotExistCnt += pstSrc->ulNotExistCnt;
    pstDst->ulNoAnswerCnt += pstSrc->ulNoAnswerCnt;
    pstDst->ulRejectCnt += pstSrc->ulRejectCnt;
    pstDst->ulEarlyReleaseCnt += pstSrc->ulEarlyReleaseCnt;
    pstDst->ulAnswerCnt += pstSrc->ulAnswerCnt;
    pstDst->ulPDD += pstSrc->ulPDD;
    pstDst->ulAnswerTime += pstSrc->ulAnswerTime;
    pstDst->ulWaitAgentTime += pstSrc->ulWaitAgentTime;
    pstDst->ulAgentAnswerTime += pstSrc->ulAgentAnswerTime;

}

/* 出局统计刷新 */
VOID bs_message_stat_refresh(BS_STAT_MESSAGE *pstDst, BS_STAT_MESSAGE *pstSrc)
{
    pstDst->ulTimeStamp = pstSrc->ulTimeStamp;
    pstDst->ulSendCnt += pstSrc->ulSendCnt;
    pstDst->ulRecvCnt += pstSrc->ulRecvCnt;
    pstDst->ulSendSucc += pstSrc->ulSendSucc;
    pstDst->ulSendFail += pstSrc->ulSendFail;
    pstDst->ulSendLen += pstSrc->ulSendLen;
    pstDst->ulRecvLen += pstSrc->ulRecvLen;

}

/* 账户统计刷新 */
VOID bs_account_stat_refresh(BS_ACCOUNT_ST *pstAccount,
                                  S32 lFee,
                                  S32 lPorfit,
                                  U8 ucServType)
{
    BS_STAT_ACCOUNT_ST  *pstStat;

    if (DOS_ADDR_INVALID(pstAccount))
    {
        DOS_ASSERT(0);
        return;
    }

    pstStat = &pstAccount->stStat;
    pstStat->ulTimeStamp = g_stBssCB.ulStatDayBase;
    pstStat->lProfit += lPorfit;
    switch (ucServType)
    {
        case BS_SERV_OUTBAND_CALL:
            pstStat->lOutBandCallFee += lFee;
            break;

        case BS_SERV_INBAND_CALL:
            pstStat->lInBandCallFee += lFee;
            break;

        case BS_SERV_AUTO_DIALING:
            pstStat->lAutoDialingFee += lFee;
            break;

        case BS_SERV_PREVIEW_DIALING:
            pstStat->lPreviewDailingFee += lFee;
            break;

        case BS_SERV_PREDICTIVE_DIALING:
            pstStat->lPredictiveDailingFee += lFee;
            break;

        case BS_SERV_RECORDING:
            pstStat->lRecordFee += lFee;
            break;

        case BS_SERV_CONFERENCE:
            pstStat->lConferenceFee += lFee;
            break;

        case BS_SERV_SMS_SEND:
        case BS_SERV_SMS_RECV:
            pstStat->lSmsFee += lFee;
            break;

        case BS_SERV_MMS_SEND:
        case BS_SERV_MMS_RECV:
            pstStat->lMmsFee += lFee;
            break;

        case BS_SERV_RENT:
            pstStat->lRentFee += lFee;
            break;

        default:
            break;
    }

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
             "Refresh account stat, "
             "account id:%u, time stamp:%u, profit:%d, "
             "outband fee:%d, inband fee:%d, auto dialing fee:%d, preview dialing fee:%d, "
             "predictive fee:%d, record fee:%d, sms fee:%d, mms fee:%d, rent fee:%d",
             pstAccount->ulAccountID,
             pstStat->ulTimeStamp, pstStat->lProfit,
             pstStat->lOutBandCallFee, pstStat->lInBandCallFee,
             pstStat->lAutoDialingFee, pstStat->lPreviewDailingFee,
             pstStat->lPredictiveDailingFee, pstStat->lRecordFee,
             pstStat->lSmsFee, pstStat->lMmsFee,
             pstStat->lRentFee);

}

/* 出局呼叫统计 */
VOID bs_stat_outband(BS_CDR_VOICE_ST *pstCDR)
{
    U8                  ucPos;
    BS_CUSTOMER_ST      *pstCustomer = NULL;
    BS_AGENT_ST         *pstAgent = NULL;
    BS_TASK_ST          *pstTask = NULL;
    BS_SETTLE_ST        *pstSettle = NULL;
    BS_STAT_OUTBAND     *pstStat = NULL;
    BS_STAT_OUTBAND     stStat;

    /* 确定统计的数组下标 */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    dos_memzero(&stStat, sizeof(stStat));
    stStat.ulTimeStamp = g_stBssCB.ulStatHourBase;
    stStat.ulCallCnt = 1;
    if (pstCDR->ulPDDLen != 0)
    {
        stStat.ulRingCnt = 1;
        stStat.ulPDD = pstCDR->ulPDDLen;
    }

    if (bs_cause_is_busy(pstCDR->usTerminateCause))
    {
        stStat.ulBusyCnt = 1;
    }
    else if (bs_cause_is_not_exist(pstCDR->usTerminateCause))
    {
        stStat.ulNotExistCnt = 1;
    }
    else if (bs_cause_is_no_answer(pstCDR->usTerminateCause))
    {
        stStat.ulNoAnswerCnt = 1;
    }
    else if (bs_cause_is_reject(pstCDR->usTerminateCause))
    {
        stStat.ulRejectCnt = 1;
    }

    if (pstCDR->ulTimeLen > 0)
    {
        stStat.ulAnswerCnt = 1;
        stStat.ulAnswerTime = pstCDR->ulTimeLen;
    }
    else if (BS_SESS_RELEASE_BY_CALLER == pstCDR->ucReleasePart)
    {
        stStat.ulEarlyReleaseCnt = 1;
    }

    if (pstCDR->ulCustomerID != 0 && pstCDR->ulCustomerID != U32_BUTT)
    {
        pstCustomer = bs_get_customer_st(pstCDR->ulCustomerID);
        if (pstCustomer != NULL)
        {
            pstStat = &pstCustomer->stStat.astOutBand[ucPos];
            bs_outband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh customer stat, "
                     "serv type:%u, customer id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, total pdd:%u, "
                     "total answer time:%u",
                     pstCDR->ucServType, pstCDR->ulCustomerID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime);
        }
    }

    if (pstCDR->ulAgentID != 0 && pstCDR->ulAgentID != U32_BUTT)
    {
        pstAgent = bs_get_agent_st(pstCDR->ulAgentID);
        if (pstAgent != NULL)
        {
            pstStat = &pstAgent->stStat.astOutBand[ucPos];
            bs_outband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh agent stat, "
                     "serv type:%u, agent id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, total pdd:%u, "
                     "total answer time:%u",
                     pstCDR->ucServType, pstCDR->ulAgentID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime);
        }
    }

    if (pstCDR->ulTaskID != 0 && pstCDR->ulTaskID != U32_BUTT)
    {
        pstTask = bs_get_task_st(pstCDR->ulTaskID);
        if (pstTask != NULL)
        {
            pstStat = &pstTask->stStat.astOutBand[ucPos];
            bs_outband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh task stat, "
                     "serv type:%u, task id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, total pdd:%u, "
                     "total answer time:%u",
                     pstCDR->ucServType, pstCDR->ulTaskID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime);
        }
    }

    if (pstCDR->usPeerTrunkID != 0 && pstCDR->usPeerTrunkID != U16_BUTT)
    {
        pstSettle = bs_get_settle_st(pstCDR->usPeerTrunkID);
        if (pstSettle != NULL)
        {
            pstStat = &pstSettle->stStat.astOutBand[ucPos];
            bs_outband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh trunk stat, "
                     "serv type:%u, trunk id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, total pdd:%u, "
                     "total answer time:%u",
                     pstCDR->ucServType, pstCDR->usPeerTrunkID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime);
        }
    }

}

/* 入局呼叫统计 */
VOID bs_stat_inband(BS_CDR_VOICE_ST *pstCDR)
{
    U8                  ucPos;
    BS_CUSTOMER_ST      *pstCustomer = NULL;
    BS_AGENT_ST         *pstAgent = NULL;
    BS_TASK_ST          *pstTask = NULL;
    BS_SETTLE_ST        *pstSettle = NULL;
    BS_STAT_INBAND      *pstStat = NULL;
    BS_STAT_INBAND      stStat;

    /* 确定统计的数组下标 */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    dos_memzero(&stStat, sizeof(stStat));
    stStat.ulTimeStamp = g_stBssCB.ulStatHourBase;
    stStat.ulCallCnt = 1;
    if (pstCDR->ulPDDLen != 0)
    {
        stStat.ulRingCnt = 1;
    }

    if (bs_cause_is_busy(pstCDR->usTerminateCause))
    {
        stStat.ulBusyCnt = 1;
    }
    else if (bs_cause_is_no_answer(pstCDR->usTerminateCause))
    {
        stStat.ulNoAnswerCnt = 1;
    }

    if (pstCDR->ulTimeLen > 0)
    {
        stStat.ulAnswerCnt = 1;
        stStat.ulAgentAnswerCnt = 1;
        stStat.ulAnswerTime = pstCDR->ulTimeLen;
        stStat.ulAgentAnswerTime = pstCDR->ulTimeLen;
    }
    else if (BS_SESS_RELEASE_BY_CALLER == pstCDR->ucReleasePart)
    {
        stStat.ulEarlyReleaseCnt = 1;
    }

    if (pstCDR->ulAgentID != 0 && pstCDR->ulAgentID != U32_BUTT)
    {
        stStat.ulConnAgentCnt = 1;
    }

    stStat.ulHoldCnt = pstCDR->ulHoldCnt;
    stStat.ulWaitAgentTime = pstCDR->ulWaitAgentTime;
    stStat.ulHoldTime = pstCDR->ulHoldTimeLen;

    if (pstCDR->ulCustomerID != 0 && pstCDR->ulCustomerID != U32_BUTT)
    {
        pstCustomer = bs_get_customer_st(pstCDR->ulCustomerID);
        if (pstCustomer != NULL)
        {
            pstStat = &pstCustomer->stStat.astInBand[ucPos];
            bs_inband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh customer stat, "
                     "serv type:%u, customer id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, no answer cnt:%u, early release cnt:%u, "
                     "answer cnt:%u, conn agent cnt:%u, agent answer cnt:%u, hold cnt:%u, "
                     "total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u, total hold time:%u",
                     pstCDR->ucServType, pstCDR->ulCustomerID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNoAnswerCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulHoldCnt,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime, pstStat->ulHoldTime);
        }
    }

    if (pstCDR->ulAgentID != 0 && pstCDR->ulAgentID != U32_BUTT)
    {
        pstAgent = bs_get_agent_st(pstCDR->ulAgentID);
        if (pstAgent != NULL)
        {
            pstStat = &pstAgent->stStat.astInBand[ucPos];
            bs_inband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh agent stat, "
                     "serv type:%u, agent id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, no answer cnt:%u, early release cnt:%u, "
                     "answer cnt:%u, conn agent cnt:%u, agent answer cnt:%u, hold cnt:%u, "
                     "total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u, total hold time:%u",
                     pstCDR->ucServType, pstCDR->ulAgentID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNoAnswerCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulHoldCnt,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime, pstStat->ulHoldTime);
        }
    }

    if (pstCDR->ulTaskID != 0 && pstCDR->ulTaskID != U32_BUTT)
    {
        pstTask = bs_get_task_st(pstCDR->ulTaskID);
        if (pstTask != NULL)
        {
            pstStat = &pstTask->stStat.astInBand[ucPos];
            bs_inband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh task stat, "
                     "serv type:%u, task id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, no answer cnt:%u, early release cnt:%u, "
                     "answer cnt:%u, conn agent cnt:%u, agent answer cnt:%u, hold cnt:%u, "
                     "total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u, total hold time:%u",
                     pstCDR->ucServType, pstCDR->ulTaskID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNoAnswerCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulHoldCnt,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime, pstStat->ulHoldTime);
        }
    }

    if (pstCDR->usPeerTrunkID != 0 && pstCDR->usPeerTrunkID != U16_BUTT)
    {
        pstSettle = bs_get_settle_st(pstCDR->usPeerTrunkID);
        if (pstSettle != NULL)
        {
            pstStat = &pstSettle->stStat.astInBand[ucPos];
            bs_inband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh trunk stat, "
                     "serv type:%u, trunk id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, no answer cnt:%u, early release cnt:%u, "
                     "answer cnt:%u, conn agent cnt:%u, agent answer cnt:%u, hold cnt:%u, "
                     "total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u, total hold time:%u",
                     pstCDR->ucServType, pstCDR->usPeerTrunkID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNoAnswerCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulHoldCnt,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime, pstStat->ulHoldTime);
        }
    }

}

/* 外呼统计 */
VOID bs_stat_outdialing(BS_CDR_VOICE_ST *pstCDR)
{
    U8                  ucPos;
    BS_CUSTOMER_ST      *pstCustomer = NULL;
    BS_AGENT_ST         *pstAgent = NULL;
    BS_TASK_ST          *pstTask = NULL;
    BS_SETTLE_ST        *pstSettle = NULL;
    BS_STAT_OUTDIALING  *pstStat = NULL;
    BS_STAT_OUTDIALING  stStat;

    /* 确定统计的数组下标 */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    dos_memzero(&stStat, sizeof(stStat));
    stStat.ulTimeStamp = g_stBssCB.ulStatHourBase;
    stStat.ulCallCnt = 1;
    if (pstCDR->ulPDDLen != 0)
    {
        stStat.ulRingCnt = 1;
        stStat.ulPDD = pstCDR->ulPDDLen;
    }

    if (bs_cause_is_busy(pstCDR->usTerminateCause))
    {
        stStat.ulBusyCnt = 1;
    }
    else if (bs_cause_is_not_exist(pstCDR->usTerminateCause))
    {
        stStat.ulNotExistCnt = 1;
    }
    else if (bs_cause_is_no_answer(pstCDR->usTerminateCause))
    {
        stStat.ulNoAnswerCnt = 1;
    }
    else if (bs_cause_is_reject(pstCDR->usTerminateCause))
    {
        stStat.ulRejectCnt = 1;
    }

    if (pstCDR->ulTimeLen > 0)
    {
        stStat.ulAnswerCnt = 1;
        stStat.ulAnswerTime = pstCDR->ulTimeLen;
        stStat.ulAgentAnswerTime = pstCDR->ulTimeLen;
    }
    else if (BS_SESS_RELEASE_BY_CALLER == pstCDR->ucReleasePart)
    {
        stStat.ulEarlyReleaseCnt = 1;
    }

    stStat.ulWaitAgentTime = pstCDR->ulWaitAgentTime;

    if (pstCDR->ulCustomerID != 0 && pstCDR->ulCustomerID != U32_BUTT)
    {
        pstCustomer = bs_get_customer_st(pstCDR->ulCustomerID);
        if (pstCustomer != NULL)
        {
            pstStat = &pstCustomer->stStat.astOutDialing[ucPos];
            bs_outdialing_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh customer stat, "
                     "serv type:%u, customer id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, conn agent cnt:%u, "
                     "agent answer cnt:%u, total pdd:%u, total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u",
                     pstCDR->ucServType, pstCDR->ulCustomerID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime);
        }
    }

    if (pstCDR->ulAgentID != 0 && pstCDR->ulAgentID != U32_BUTT)
    {
        pstAgent = bs_get_agent_st(pstCDR->ulAgentID);
        if (pstAgent != NULL)
        {
            pstStat = &pstAgent->stStat.astOutDialing[ucPos];
            bs_outdialing_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh agent stat, "
                     "serv type:%u, agent id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, conn agent cnt:%u, "
                     "agent answer cnt:%u, total pdd:%u, total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u",
                     pstCDR->ucServType, pstCDR->ulAgentID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime);
        }
    }

    if (pstCDR->ulTaskID != 0 && pstCDR->ulTaskID != U32_BUTT)
    {
        pstTask = bs_get_task_st(pstCDR->ulTaskID);
        if (pstTask != NULL)
        {
            pstStat = &pstTask->stStat.astOutDialing[ucPos];
            bs_outdialing_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh task stat, "
                     "serv type:%u, task id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, conn agent cnt:%u, "
                     "agent answer cnt:%u, total pdd:%u, total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u",
                     pstCDR->ucServType, pstCDR->ulTaskID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime);
        }
    }

    if (pstCDR->usPeerTrunkID != 0 && pstCDR->usPeerTrunkID != U16_BUTT)
    {
        pstSettle = bs_get_settle_st(pstCDR->usPeerTrunkID);
        if (pstSettle != NULL)
        {
            pstStat = &pstSettle->stStat.astOutDialing[ucPos];
            bs_outdialing_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh trunk stat, "
                     "serv type:%u, trunk id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, conn agent cnt:%u, "
                     "agent answer cnt:%u, total pdd:%u, total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u",
                     pstCDR->ucServType, pstCDR->usPeerTrunkID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime);
        }
    }

}

/* 语音业务统计 */
VOID bs_stat_voice(BS_CDR_VOICE_ST *pstCDR)
{
    switch (pstCDR->ucServType)
    {
        case BS_SERV_OUTBAND_CALL:
            bs_stat_outband(pstCDR);
            break;

        case BS_SERV_INBAND_CALL:
            bs_stat_inband(pstCDR);
            break;

        case BS_SERV_AUTO_DIALING:
            bs_stat_outdialing(pstCDR);
            break;

        case BS_SERV_PREVIEW_DIALING:
            bs_stat_outdialing(pstCDR);
            break;

        case BS_SERV_PREDICTIVE_DIALING:
            bs_stat_outdialing(pstCDR);
            break;

        default:
            break;
    }
}

/* 信息业务统计 */
VOID bs_stat_message(BS_CDR_MS_ST *pstCDR)
{
    U8                  ucPos;
    BS_CUSTOMER_ST      *pstCustomer = NULL;
    BS_AGENT_ST         *pstAgent = NULL;
    BS_TASK_ST          *pstTask = NULL;
    BS_SETTLE_ST        *pstSettle = NULL;
    BS_STAT_MESSAGE     *pstStat = NULL;
    BS_STAT_MESSAGE     stStat;

    /* 确定统计的数组下标 */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    dos_memzero(&stStat, sizeof(stStat));
    stStat.ulTimeStamp = g_stBssCB.ulStatHourBase;
    switch (pstCDR->ucServType)
    {
        case BS_SERV_SMS_SEND:
        case BS_SERV_MMS_SEND:
            stStat.ulSendCnt = 1;
            if (pstCDR->ulArrivedTimeStamp != 0)
            {
                stStat.ulSendSucc = 1;
            }
            else
            {
                stStat.ulSendFail= 1;
            }
            stStat.ulSendLen = pstCDR->ulLen;
            break;

        case BS_SERV_SMS_RECV:
        case BS_SERV_MMS_RECV:
            stStat.ulRecvCnt = 1;
            stStat.ulRecvLen = pstCDR->ulLen;
            break;

        default:
            break;
    }

    if (pstCDR->ulCustomerID != 0 && pstCDR->ulCustomerID != U32_BUTT)
    {
        pstCustomer = bs_get_customer_st(pstCDR->ulCustomerID);
        if (pstCustomer != NULL)
        {
            pstStat = &pstCustomer->stStat.astMS[ucPos];
            bs_message_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh customer stat, "
                     "serv type:%u, customer id:%u, time stamp:%u, send cnt:%u, "
                     "recv cnt:%u, succ cnt:%u, fail cnt:%u, send len:%u, recv len:%u, ",
                     pstCDR->ucServType, pstCDR->ulCustomerID,
                     pstStat->ulTimeStamp, pstStat->ulSendCnt,
                     pstStat->ulRecvCnt, pstStat->ulSendSucc,
                     pstStat->ulSendFail, pstStat->ulSendLen,
                     pstStat->ulRecvLen);
        }
    }

    if (pstCDR->ulAgentID != 0 && pstCDR->ulAgentID != U32_BUTT)
    {
        pstAgent = bs_get_agent_st(pstCDR->ulAgentID);
        if (pstAgent != NULL)
        {
            pstStat = &pstAgent->stStat.astMS[ucPos];
            bs_message_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh agent stat, "
                     "serv type:%u, agent id:%u, time stamp:%u, send cnt:%u, "
                     "recv cnt:%u, succ cnt:%u, fail cnt:%u, send len:%u, recv len:%u, ",
                     pstCDR->ucServType, pstCDR->ulAgentID,
                     pstStat->ulTimeStamp, pstStat->ulSendCnt,
                     pstStat->ulRecvCnt, pstStat->ulSendSucc,
                     pstStat->ulSendFail, pstStat->ulSendLen,
                     pstStat->ulRecvLen);
        }
    }

    if (pstCDR->ulTaskID != 0 && pstCDR->ulTaskID != U32_BUTT)
    {
        pstTask = bs_get_task_st(pstCDR->ulTaskID);
        if (pstTask != NULL)
        {
            pstStat = &pstTask->stStat.astMS[ucPos];
            bs_message_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh task stat, "
                     "serv type:%u, task id:%u, time stamp:%u, send cnt:%u, "
                     "recv cnt:%u, succ cnt:%u, fail cnt:%u, send len:%u, recv len:%u, ",
                     pstCDR->ucServType, pstCDR->ulTaskID,
                     pstStat->ulTimeStamp, pstStat->ulSendCnt,
                     pstStat->ulRecvCnt, pstStat->ulSendSucc,
                     pstStat->ulSendFail, pstStat->ulSendLen,
                     pstStat->ulRecvLen);
        }
    }

    if (pstCDR->usPeerTrunkID != 0 && pstCDR->usPeerTrunkID != U16_BUTT)
    {
        pstSettle = bs_get_settle_st(pstCDR->usPeerTrunkID);
        if (pstSettle != NULL)
        {
            pstStat = &pstSettle->stStat.astMS[ucPos];
            bs_message_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh trunk stat, "
                     "serv type:%u, trunk id:%u, time stamp:%u, send cnt:%u, "
                     "recv cnt:%u, succ cnt:%u, fail cnt:%u, send len:%u, recv len:%u, ",
                     pstCDR->ucServType, pstCDR->usPeerTrunkID,
                     pstStat->ulTimeStamp, pstStat->ulSendCnt,
                     pstStat->ulRecvCnt, pstStat->ulSendSucc,
                     pstStat->ulSendFail, pstStat->ulSendLen,
                     pstStat->ulRecvLen);
        }
    }

}


/* 根据累计规则获取统计值 */
U32 bs_get_timelen_stat(BS_BILLING_MATCH_ST *pstBillingMatch)
{
    if (DOS_ADDR_INVALID(pstBillingMatch))
    {
        DOS_ASSERT(0);
        return 0;
    }

    //TODO查询数据库,获取统计值


    /* 务必注意:由数据层完成操作,需要考虑数据库操作效率,不能因此导致线程长时间阻塞 */


    return 0;
}

/* 根据累计规则获取统计值 */
U32 bs_get_count_stat(BS_BILLING_MATCH_ST *pstBillingMatch)
{
    if (DOS_ADDR_INVALID(pstBillingMatch))
    {
        DOS_ASSERT(0);
        return 0;
    }

    //TODO查询数据库,获取统计值

    /* 务必注意:由数据层完成操作,需要考虑数据库操作效率,不能因此导致线程长时间阻塞 */

    return 0;
}

/* 根据累计规则获取统计值 */
U32 bs_get_traffic_stat(BS_BILLING_MATCH_ST *pstBillingMatch)
{
    if (DOS_ADDR_INVALID(pstBillingMatch))
    {
        DOS_ASSERT(0);
        return 0;
    }

    //TODO查询数据库,获取统计值

    /* 务必注意:由数据层完成操作,需要考虑数据库操作效率,不能因此导致线程长时间阻塞 */


    return 0;
}

/*
计费属性值匹配
说明:
1.先获取属性值,保存后再进行匹配;
2.根据不同的属性类别,pInput指向不同的参数地址;
3.pulValue指向参数对应的属性值地址,为避免反复查询,可以传入有效值;
*/
S32 bs_match_billing_attr(U8 ucAttrType, U32 ulAttrValue, VOID *pInput, U32 *pulValue)
{
    S8                      *szNum;
    U32                     ulValue;
    S32                     lMatchResult = DOS_FAIL;
    U32                     ulStat;
    BS_ACCUMULATE_RULE      *pstAccuRule = NULL;
    BS_BILLING_MATCH_ST     *pstInput = NULL;

    if (DOS_ADDR_INVALID(pulValue))
    {
        DOS_ASSERT(0);
        return lMatchResult;
    }

    if (U8_BUTT == ucAttrType || 0 == ucAttrType)
    {
        /* 不限定属性 */
        return DOS_SUCC;
    }

    if (*pulValue != U32_BUTT)
    {
        /* 曾经查询过,后面不再进行查询 */
        ulValue = *pulValue;
    }
    else
    {
        ulValue = U32_BUTT;
        *pulValue = U32_BUTT;
    }

    switch (ucAttrType)
    {
        case BS_BILLING_ATTR_REGION:
            /*
            1.归属地属性,32bit定义:3个字段,国家代码(32-17bit),省编号(16-11bit),区号(10-1bit)
            2.归属地属性,匹配规则:
              a.0-通配所有;
              b.字段值:0-通配字段代表的所有地区;
              c.完全匹配:所有字段均相同;
            */
            if (0 == ulAttrValue)
            {
                /* 通配所有,无需查询 */
                lMatchResult = DOS_SUCC;
            }
            else
            {
                szNum = (S8 *)pInput;
                if (DOS_ADDR_VALID(szNum)
                    && U32_BUTT == ulValue)
                {
                    ulValue = bs_get_number_region(szNum, BS_MAX_CALL_NUM_LEN);
                    *pulValue = ulValue;
                }

                if (U32_BUTT == ulValue)
                {
                    /* 查询失败 */
                    lMatchResult = DOS_FAIL;
                }
                else if (ulAttrValue == (ulAttrValue&ulValue))
                {
                    lMatchResult = DOS_SUCC;
                }
            }
            break;

        case BS_BILLING_ATTR_TEL_OPERATOR:
            /*
            3.运营商属性,匹配规则:
              a.0-通配所有;
              b.要么通配,要么匹配单个运营商;
            */
            if (0 == ulAttrValue)
            {
                /* 通配所有,无需查询 */
                lMatchResult = DOS_SUCC;
            }
            else
            {
                szNum = (S8 *)pInput;
                if (DOS_ADDR_VALID(szNum)
                    && U32_BUTT == ulValue)
                {
                    ulValue = bs_get_number_teloperator(szNum, BS_MAX_CALL_NUM_LEN);
                    *pulValue = ulValue;
                }

                if (U32_BUTT == ulValue)
                {
                    /* 查询失败 */
                    lMatchResult = DOS_FAIL;
                }
                else if (ulAttrValue == ulValue)
                {
                    lMatchResult = DOS_SUCC;
                }
            }
            break;

        case BS_BILLING_ATTR_NUMBER_TYPE:
            /*
            4.号码类型属性,匹配规则:
              a.0-通配所有;
              b.要么通配,要么匹配单个号码类型;
            */
            if (0 == ulAttrValue)
            {
                /* 通配所有,无需查询 */
                lMatchResult = DOS_SUCC;
            }
            else
            {
                szNum = (S8 *)pInput;
                if (DOS_ADDR_VALID(szNum)
                    && U32_BUTT == ulValue)
                {
                    ulValue = bs_get_number_type(szNum, BS_MAX_CALL_NUM_LEN);
                    *pulValue = ulValue;
                }

                if (U32_BUTT == ulValue)
                {
                    /* 查询失败 */
                    lMatchResult = DOS_FAIL;
                }
                else if (ulAttrValue == ulValue)
                {
                    lMatchResult = DOS_SUCC;
                }
            }
            break;

        case BS_BILLING_ATTR_TIME:
            /*
            5.时间属性,匹配规则:
              a.0-不限定时间;
              b.根据属性值,查询数据库,判断时间是否在数据库的时间定义中;
            */
            if (0 == ulAttrValue)
            {
                /* 不限定时间,无需查询 */
                lMatchResult = DOS_SUCC;
            }
            else
            {
                if (ulAttrValue == ulValue)
                {
                    /* 曾经匹配过 */
                    lMatchResult = DOS_SUCC;
                }
                else if (bs_time_is_in_period((U32 *)pInput, ulAttrValue))
                {
                    lMatchResult = DOS_SUCC;
                    *pulValue = ulAttrValue;
                }
            }
            break;

        case BS_BILLING_ATTR_ACCUMULATE_TIMELEN:
        case BS_BILLING_ATTR_ACCUMULATE_COUNT:
        case BS_BILLING_ATTR_ACCUMULATE_TRAFFIC:
            /*
            6.累计类属性,匹配规则:
              a.根据属性值查询数据库,获取累计规则;
              b.累计规则包括累计对象/累计周期/业务集/最大值;
              c.统计数据;
              d.累计对象/累计周期/业务集相同的情况下,统计数据小于最大值即匹配成功;
              e.最大值为0,表示不限定最大值;
            */

            if (ulValue != U32_BUTT && ulAttrValue == ulValue)
            {
                /* 曾经匹配过 */
                lMatchResult = DOS_SUCC;
                break;
            }

            pstInput = (BS_BILLING_MATCH_ST *)pInput;
            pstAccuRule = bs_get_accumulate_rule(ulAttrValue);
            if (DOS_ADDR_INVALID(pstAccuRule) || DOS_ADDR_INVALID(pstInput))
            {
                lMatchResult = DOS_FAIL;
                break;
            }

            if (0 == pstAccuRule->ulMaxValue)
            {
                /* 不限定,无需查询统计 */
                lMatchResult = DOS_SUCC;
            }
            else
            {
                /* 使用数据库中的配置进行统计 */
                pstInput->stAccuRule = *pstAccuRule;
                if (BS_BILLING_ATTR_ACCUMULATE_TIMELEN == ucAttrType)
                {
                    ulStat = bs_get_timelen_stat(pstInput);
                }
                else if (BS_BILLING_ATTR_ACCUMULATE_COUNT == ucAttrType)
                {
                    ulStat = bs_get_count_stat(pstInput);
                }
                else if (BS_BILLING_ATTR_ACCUMULATE_TRAFFIC == ucAttrType)
                {
                    ulStat = bs_get_traffic_stat(pstInput);
                }

                if (ulStat < pstAccuRule->ulMaxValue)
                {
                    lMatchResult = DOS_SUCC;
                    *pulValue = ulAttrValue;
                }
            }

            break;

        case BS_BILLING_ATTR_CONCURRENT:
        case BS_BILLING_ATTR_SET:
        case BS_BILLING_ATTR_RESOURCE_AGENT:
        case BS_BILLING_ATTR_RESOURCE_NUMBER:
        case BS_BILLING_ATTR_RESOURCE_LINE:
            /*
            6.资源类属性,只有在租金类业务下使用,由系统自行计算处理,无需匹配.
              a.属性值为资源数量,费率为资源单价;其它字段可以不填;;
              b.属性值为0表示由系统统计数量;
            */
            lMatchResult = DOS_SUCC;
            break;

        default:
            break;

    }

    return lMatchResult;

}

/* 匹配计费规则 */
BS_BILLING_RULE_ST *bs_match_billing_rule(BS_BILLING_PACKAGE_ST *pstPackage,
                                              BS_BILLING_MATCH_ST *pstBillingMatch)
{
    S32                 i, lMatch;
    U32                 aulSrcValue[BS_BILLING_ATTR_LAST] = {U32_BUTT};
    U32                 aulDstValue[BS_BILLING_ATTR_LAST] = {U32_BUTT};
    VOID                *pInput;
    BS_BILLING_RULE_ST  *pstRule    = NULL;
    BS_BILLING_RULE_ST  *pstRuleRet = NULL;

    /* 按优先级从高到低依次匹配,匹配成功则退出匹配 */
    for (i = 0; i < BS_MAX_BILLING_RULE_IN_PACKAGE; i++)
    {
        pstRule = &pstPackage->astRule[i];

        if (BS_BILLING_BY_CYCLE == pstRule->ucBillingType)
        {
            /* 此处不适用周期计费方式 */
            continue;
        }

        if(!bs_billing_rule_is_properly(pstRule))
        {
            /* 计费规则不妥当 */
            continue;
        }

        if ((pstRule->ulExpireTimestamp != 0
             && pstBillingMatch->ulTimeStamp >= pstRule->ulExpireTimestamp)
            || pstBillingMatch->ulTimeStamp < pstRule->ulEffectTimestamp)
        {
            /* 计费规则尚未生效;0表示永远有效 */
            continue;
        }

        /* 依次判断每个计费属性是否匹配 */

        /* 获取属性值参数 */
        pInput = bs_get_attr_input(pstBillingMatch, pstRule->ucSrcAttrType1, DOS_TRUE);
        /* 计费属性匹配(属性值一旦计算获取即记录下来,不会重复计算) */
        lMatch = bs_match_billing_attr(pstRule->ucSrcAttrType1, pstRule->ulSrcAttrValue1,
                                       pInput, &aulSrcValue[pstRule->ucSrcAttrType1]);
        if (lMatch != DOS_SUCC)
        {
            continue;
        }

        /* 获取属性值参数 */
        pInput = bs_get_attr_input(pstBillingMatch, pstRule->ucSrcAttrType2, DOS_TRUE);
        /* 计费属性匹配(属性值一旦计算获取即记录下来,不会重复计算) */
        lMatch = bs_match_billing_attr(pstRule->ucSrcAttrType2, pstRule->ulSrcAttrValue2,
                                       pInput, &aulSrcValue[pstRule->ucSrcAttrType2]);
        if (lMatch != DOS_SUCC)
        {
            continue;
        }

        /* 获取属性值参数 */
        pInput = bs_get_attr_input(pstBillingMatch, pstRule->ucDstAttrType1, DOS_TRUE);
        /* 计费属性匹配(属性值一旦计算获取即记录下来,不会重复计算) */
        lMatch = bs_match_billing_attr(pstRule->ucDstAttrType1, pstRule->ulDstAttrValue1,
                                       pInput, &aulDstValue[pstRule->ucDstAttrType1]);
        if (lMatch != DOS_SUCC)
        {
            continue;
        }

        /* 获取属性值参数 */
        pInput = bs_get_attr_input(pstBillingMatch, pstRule->ucDstAttrType2, DOS_TRUE);
        /* 计费属性匹配(属性值一旦计算获取即记录下来,不会重复计算) */
        lMatch = bs_match_billing_attr(pstRule->ucDstAttrType2, pstRule->ulDstAttrValue2,
                                       pInput, &aulDstValue[pstRule->ucDstAttrType2]);
        if (lMatch != DOS_SUCC)
        {
            continue;
        }

        /* 到此匹配成功 */
        if (DOS_ADDR_INVALID(pstRuleRet))
        {
            pstRuleRet = pstRule;
        }
        else if (pstRuleRet->ucPriority > pstRule->ucPriority)
        {
            /* 选择优先级高的 */
            pstRuleRet = pstRule;
        }
    }

    if (DOS_ADDR_INVALID(pstRuleRet))
    {
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
                 "Match billing rule fail! package:%u, service:%u, customer:%u, agent:%u"
                 "userid:%u, timestamp:%u, caller:%s, callee:%s",
                 pstPackage->ulPackageID, pstPackage->ucServType,
                 pstBillingMatch->ulCustomerID, pstBillingMatch->ulAgentID,
                 pstBillingMatch->ulUserID, pstBillingMatch->ulTimeStamp,
                 pstBillingMatch->szCaller, pstBillingMatch->szCallee);

    }
    else
    {
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
                 "Match billing rule succ! "
                 "package:%u, rule:%u, service:%u, priority:%u, type:%u"
                 "src attr1:%u-%u, attr2:%u-%u, dst attr1:%u-%u, attr1:%u-%u, "
                 "first unit:%u, cnt:%u, next unit:%u, cnt:%u, rate:%u",
                 pstRuleRet->ulPackageID, pstRuleRet->ulRuleID,
                 pstRuleRet->ucServType, pstRuleRet->ucPriority,
                 pstRuleRet->ucBillingType, pstRuleRet->ucSrcAttrType1,
                 pstRuleRet->ulSrcAttrValue1, pstRuleRet->ucSrcAttrType2,
                 pstRuleRet->ulSrcAttrValue2, pstRuleRet->ucDstAttrType1,
                 pstRuleRet->ulDstAttrValue1, pstRuleRet->ucDstAttrType2,
                 pstRuleRet->ulDstAttrValue2, pstRuleRet->ulFirstBillingUnit,
                 pstRuleRet->ucFirstBillingCnt, pstRuleRet->ulNextBillingUnit,
                 pstRuleRet->ucNextBillingCnt, pstRuleRet->ulBillingRate);
    }

    return pstRuleRet;
}

/* 预批价处理,一般用于认证请求 */
U32 bs_pre_billing(BS_CUSTOMER_ST *pstCustomer, BS_MSG_AUTH *pstMsg, BS_BILLING_PACKAGE_ST *pstPackage)
{
    U32                 ulMaxSession;
    BS_BILLING_RULE_ST  *pstRule;
    BS_BILLING_MATCH_ST stBillingMatch;

    if (DOS_ADDR_INVALID(pstCustomer) || DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstPackage))
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    ulMaxSession = U32_BUTT;
    dos_memzero(&stBillingMatch, sizeof(stBillingMatch));
    stBillingMatch.ulCustomerID = pstMsg->ulCustomerID;
    stBillingMatch.ulComsumerID = pstMsg->ulCustomerID;
    stBillingMatch.ulAgentID = pstMsg->ulAgentID;
    stBillingMatch.ulUserID = pstMsg->ulUserID;
    stBillingMatch.ulTimeStamp = pstMsg->ulTimeStamp;
    dos_strncpy(stBillingMatch.szCaller, pstMsg->szCaller, sizeof(stBillingMatch.szCaller));
    dos_strncpy(stBillingMatch.szCallee, pstMsg->szCallee, sizeof(stBillingMatch.szCallee));

    pstRule = bs_match_billing_rule(pstPackage, &stBillingMatch);
    if (NULL == pstRule)
    {
        /* 匹配失败 */
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
                 "Prebilling fail! package:%u, service:%u",
                 pstPackage->ulPackageID, pstPackage->ucServType);
        return ulMaxSession;
    }

    /* 开始试算,按第二段计费费率进行预估计算 */
    if (pstRule->ulBillingRate != 0 && pstRule->ucNextBillingCnt != 0)
    {
        ulMaxSession = (U32)((S64)pstRule->ulNextBillingUnit
                     * (pstCustomer->stAccount.LBalanceActive + pstCustomer->stAccount.lCreditLine)
                     / ((S64)pstRule->ulBillingRate * (S64)pstRule->ucNextBillingCnt));
    }

    if (bs_is_message_service(pstRule->ucServType)
        && ulMaxSession > g_stBssCB.ulMaxMsNum)
    {
        ulMaxSession = g_stBssCB.ulMaxMsNum;
    }
    else if (BS_BILLING_BY_TIMELEN == pstRule->ucBillingType
             && ulMaxSession > g_stBssCB.ulMaxVocTime)
    {
        ulMaxSession = g_stBssCB.ulMaxVocTime;
    }

    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
             "Prebilling succ! package:%u, service:%u, customer:%u, agent:%u"
             "userid:%u, timestamp:%u, caller:%s, callee:%s, max session:%u",
             pstPackage->ulPackageID, pstPackage->ucServType,
             stBillingMatch.ulCustomerID, stBillingMatch.ulAgentID,
             stBillingMatch.ulUserID, stBillingMatch.ulTimeStamp,
             stBillingMatch.szCaller, stBillingMatch.szCallee,
             ulMaxSession);

    return ulMaxSession;
}



#ifdef __cplusplus
}
#endif /* __cplusplus */


