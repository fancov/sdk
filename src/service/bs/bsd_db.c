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
#include <json/dos_json.h>
#include "bs_cdr.h"
#include "bs_stat.h"
#include "bs_def.h"
#include "bsd_db.h"


/* 数据库句柄 */
DB_HANDLE_ST    *g_pstDBHandle = NULL;

static S32 bsd_record_cnt_cb(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    U32 ulCnt = 0;
    S32 *pulCnt = NULL;

    if (DOS_ADDR_INVALID(pParam)|| DOS_ADDR_INVALID(aszData) || DOS_ADDR_INVALID(*aszData))
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (dos_atoul(aszData[0], &ulCnt) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    pulCnt = (S32 *)pParam;
    *pulCnt = ulCnt;

    return 0;
}

static S32 bsd_walk_customer_tbl_cb(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    U32             ulCnt = 0, ulHashIndex, ulCustomerType, ulCustomerState;
    U32             ulBallingPageage, ulBanlance;
    HASH_NODE_S     *pstHashNode = NULL;
    BS_CUSTOMER_ST  *pstCustomer = NULL;

    pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    HASH_Init_Node(pstHashNode);

    pstCustomer = dos_dmem_alloc(sizeof(BS_CUSTOMER_ST));
    if (DOS_ADDR_INVALID(pstCustomer))
    {
        dos_dmem_free(pstHashNode);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    bs_init_customer_st(pstCustomer);

    //TODO:拷贝数据库信息到结构体中(包含账户数据)
    if (dos_atoul(aszData[0], &pstCustomer->ulCustomerID) < 0
        || dos_atoul(aszData[2], &pstCustomer->ulParentID) < 0
        || dos_atoul(aszData[3], &ulCustomerType)
        || dos_atoul(aszData[4], &ulCustomerState) < 0
        || dos_atoul(aszData[5], &ulBallingPageage) < 0
        || dos_atoul(aszData[6], &ulBanlance) < 0)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstHashNode);
        dos_dmem_free(pstCustomer);
        pstHashNode = NULL;
        pstCustomer = NULL;
        return -1;
    }

    pstCustomer->ucCustomerType = (U8)ulCustomerType;
    pstCustomer->ucCustomerState = (U8)ulCustomerState;
    pstCustomer->stAccount.ulAccountID = pstCustomer->ulCustomerID;
    pstCustomer->stAccount.ulBillingPackageID = ulBallingPageage;
    pstCustomer->stAccount.LBalanceActive = ulBanlance;
    dos_strncpy(pstCustomer->szCustomerName, aszData[1], sizeof(pstCustomer->szCustomerName));
    pstCustomer->szCustomerName[sizeof(pstCustomer->szCustomerName) - 1] = '\0';

    pstHashNode->pHandle = (VOID *)pstCustomer;
    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_CUSTOMER_SIZE, pstCustomer->ulCustomerID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return -1;
    }


    pthread_mutex_lock(&g_mutexCustomerTbl);
    /* 存放到哈希表之前先查下,确定是否有重复 */
    if(NULL == hash_find_node(g_astCustomerTbl,
                              ulHashIndex,
                              (VOID *)&pstCustomer->ulCustomerID,
                              bs_customer_hash_node_match))
    {
        ulCnt++;
        hash_add_node(g_astCustomerTbl, pstHashNode, ulHashIndex, NULL);
        g_astCustomerTbl->NodeNum++;
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "ERR: customer(%u:%s) is duplicated in DB !",
                 pstCustomer->ulCustomerID, pstCustomer->szCustomerName);
        dos_dmem_free(pstHashNode);
        dos_dmem_free(pstCustomer);
    }
    pthread_mutex_unlock(&g_mutexCustomerTbl);

    return 0;
}

/* 遍历客户数据表 */
S32 bsd_walk_customer_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    U32             ulCnt = 0, ulHashIndex;
    S8              szQuery[512] = { 0 };
    HASH_NODE_S     *pstHashNode = NULL;
    BS_CUSTOMER_ST  *pstCustomer = NULL;

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT `id`,`name`,`parent_id`,`type`,`status`, `billing_package_id`, `balance` from tbl_customer;");
    if (db_query(g_pstDBHandle, szQuery, bsd_walk_customer_tbl_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read customers from DB FAIL!");
        return BS_INTER_ERR_FAIL;
    }

    HASH_Scan_Table(g_astCustomerTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astCustomerTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstCustomer = pstHashNode->pHandle;
            dos_snprintf(szQuery, sizeof(szQuery), "SELECT count(*) FROM tbl_agent WHERE tbl_agent.customer_id=%d;", pstCustomer->ulCustomerID);
            if (db_query(g_pstDBHandle, szQuery, bsd_record_cnt_cb, (VOID *)&pstCustomer->ulAgentNum, NULL) != DB_ERR_SUCC)
            {
                bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read Agent list fail for custom %d!", pstCustomer->ulCustomerID);
                return BS_INTER_ERR_FAIL;
            }

            dos_snprintf(szQuery, sizeof(szQuery), "SELECT count(*) FROM tbl_sipassign WHERE tbl_sipassign.customer_id=%d;", pstCustomer->ulCustomerID);
            if (db_query(g_pstDBHandle, szQuery, bsd_record_cnt_cb, (VOID *)&pstCustomer->ulUserLineNum, NULL) != DB_ERR_SUCC)
            {
                bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read Agent user line fail for custom %d!", pstCustomer->ulCustomerID);
                return BS_INTER_ERR_FAIL;
            }

            dos_snprintf(szQuery, sizeof(szQuery), "SELECT count(*) FROM tbl_caller WHERE tbl_caller.customer_id=%d;", pstCustomer->ulCustomerID);
            if (db_query(g_pstDBHandle, szQuery, bsd_record_cnt_cb, (VOID *)&pstCustomer->ulNumberNum, NULL) != DB_ERR_SUCC)
            {
                bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read Agent number list fail for custom %d!", pstCustomer->ulCustomerID);
                return BS_INTER_ERR_FAIL;
            }
        }
    }

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read customers from DB SUCC!", ulCnt);
    return BS_INTER_ERR_SUCC;

}


static S32 bsd_walk_agent_tbl_cb(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    U32                     ulCnt = 0, ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    HASH_NODE_S             *pstMatchNode = NULL;
    BS_AGENT_ST             *pstAgent = NULL;

    /* 值查询了4列记录 */
    if (lCnt != 4)
    {
        DOS_ASSERT(0);

        return -1;
    }

    if (DOS_ADDR_INVALID(aszData) || DOS_ADDR_INVALID(*aszData))
    {
        DOS_ASSERT(0);

        return -1;
    }

    pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    HASH_Init_Node(pstHashNode);

    pstAgent = dos_dmem_alloc(sizeof(BS_AGENT_ST));
    if (DOS_ADDR_INVALID(pstAgent))
    {
        dos_dmem_free(pstHashNode);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    bs_init_agent_st(pstAgent);

    if (dos_atoul(aszData[0], &pstAgent->ulAgentID) < 0
        || dos_atoul(aszData[1], &pstAgent->ulCustomerID) < 0
        || dos_atoul(aszData[2], &pstAgent->ulGroup1) < 0
        || dos_atoul(aszData[3], &pstAgent->ulGroup2) < 0)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstHashNode);
        dos_dmem_free(pstAgent);
        pstHashNode = NULL;
        pstAgent = NULL;

        return -1;
    }

    pstHashNode->pHandle = (VOID *)pstAgent;
    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_AGENT_SIZE, pstAgent->ulAgentID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return -1;
    }

    pthread_mutex_lock(&g_mutexAgentTbl);
    /* 存放到哈希表之前先查下,确定是否有重复 */
    pstMatchNode = hash_find_node(g_astAgentTbl,
                                  ulHashIndex,
                                  (VOID *)&pstAgent->ulAgentID,
                                  bs_agent_hash_node_match);
    if(DOS_ADDR_INVALID(pstMatchNode))
    {
        ulCnt++;
        hash_add_node(g_astAgentTbl, pstHashNode, ulHashIndex, NULL);
        g_astAgentTbl->NodeNum++;
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR,
                 "ERR: agent(%u) is duplicated in DB !",
                 pstAgent->ulAgentID);
        dos_dmem_free(pstHashNode);
        dos_dmem_free(pstAgent);
    }
    pthread_mutex_unlock(&g_mutexAgentTbl);

    return 0;
}


/* 遍历坐席表 */
S32 bsd_walk_agent_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    S8 szQuery[1024] = { 0, };

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT id, customer_id, group1_id, group2_id FROM tbl_agent;");

    if (db_query(g_pstDBHandle, szQuery, bsd_walk_agent_tbl_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read agents info from DB fail!");

        return BS_INTER_ERR_FAIL;
    }
    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read agents info from DB succ!");

    return BS_INTER_ERR_SUCC;
}


static S32 bsd_walk_billing_rule_tbl_cb(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    BS_BILLING_RULE_ST      stBillingRule;
    BS_BILLING_PACKAGE_ST   *pstBillingPackage = NULL;
    U32                     ulSrcAttrType1, ulSrcAttrType2, ulDstAttrType1, ulDstAttrType2;
    U32                     ulFirstBillingCnt, ulNextBillingCnt, ulServType1;
    U32                     ulBillingType, ulIndex;

    if (DOS_ADDR_INVALID(pParam)
        || DOS_ADDR_INVALID(aszData)
        || DOS_ADDR_INVALID(aszFields))
    {
        DOS_ASSERT(0);

        return -1;
    }

    pstBillingPackage = pParam;

    if (dos_atoul(aszData[0], &stBillingRule.ulRuleID) < 0
        || dos_atoul(aszData[3], &ulSrcAttrType1) < 0
        || dos_atoul(aszData[4], &ulSrcAttrType2) < 0
        || dos_atoul(aszData[5], &ulDstAttrType1) < 0
        || dos_atoul(aszData[6], &ulDstAttrType2) < 0
        || dos_atoul(aszData[7], &stBillingRule.ulSrcAttrValue1) < 0
        || dos_atoul(aszData[8], &stBillingRule.ulSrcAttrValue2) < 0
        || dos_atoul(aszData[9], &stBillingRule.ulDstAttrValue1) < 0
        || dos_atoul(aszData[10], &stBillingRule.ulDstAttrValue2) < 0
        || dos_atoul(aszData[11], &stBillingRule.ulFirstBillingUnit) < 0
        || dos_atoul(aszData[12], &stBillingRule.ulNextBillingUnit) < 0
        || dos_atoul(aszData[13], &ulFirstBillingCnt) < 0
        || dos_atoul(aszData[14], &ulNextBillingCnt) < 0
        || dos_atoul(aszData[15], &ulServType1) < 0
        || dos_atoul(aszData[16], &ulBillingType) < 0
        || dos_atoul(aszData[17], &stBillingRule.ulBillingRate) < 0
        || dos_atoul(aszData[18], &stBillingRule.ulEffectTimestamp) < 0
        || dos_atoul(aszData[19], &stBillingRule.ulExpireTimestamp) < 0
        || dos_atoul(aszData[20], &stBillingRule.ulPackageID) < 0)
    {
        DOS_ASSERT(0);

        return -1;
    }

    stBillingRule.ulPackageID = pstBillingPackage->ulPackageID;
    stBillingRule.ucSrcAttrType1 = (U8)ulSrcAttrType1;
    stBillingRule.ucSrcAttrType2 = (U8)ulSrcAttrType2;
    stBillingRule.ucDstAttrType1 = (U8)ulDstAttrType1;
    stBillingRule.ucDstAttrType2 = (U8)ulDstAttrType2;
    stBillingRule.ucFirstBillingCnt = (U8)ulFirstBillingCnt;
    stBillingRule.ucNextBillingCnt = (U8)ulNextBillingCnt;
    stBillingRule.ucServType = (U8)ulServType1;
    stBillingRule.ucBillingType = (U8)ulBillingType;

    /* 遍历找到一个没有使用的插进去 */
    for (ulIndex=0; ulIndex<BS_MAX_BILLING_RULE_IN_PACKAGE; ulIndex++)
    {
        if (!pstBillingPackage->astRule[ulIndex].ucValid)
        {
            dos_memcpy(&pstBillingPackage->astRule[ulIndex], &stBillingRule, sizeof(stBillingRule));

            pstBillingPackage->astRule[ulIndex].ucValid = DOS_TRUE;

            break;
        }
    }

    return 0;
}

S32 bsd_walk_billing_package_tbl_cb(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    U32 ulServType   = U32_BUTT;
    U32 ulPkgID      = U32_BUTT;
    U32 ulCount      = 0;
    S32 lIndex       = 0;
    U32 ulHashIndex  = 0;
    HASH_NODE_S  *pstHashNode = NULL;
    BS_BILLING_PACKAGE_ST *pstBillingPkg = NULL;

    if (DOS_ADDR_INVALID(aszData)
        || DOS_ADDR_INVALID(aszFields))
    {
        DOS_ASSERT(0);

        return -1;
    }

    for (ulCount=0, lIndex=0; lIndex<lCnt; lIndex++)
    {
        if (0 == dos_strnicmp(aszFields[lIndex], "id", dos_strlen("id")))
        {
            if (DOS_ADDR_INVALID(aszData[lIndex])
                || dos_atoul(aszData[lIndex], &ulPkgID) < 0)
            {
                /* 这个地方时有可能的，如果一个计费业务中没有任何计费规则时，就会有么个业务不存在任何计费规则 */
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszFields[lIndex], "type", dos_strlen("type")))
        {
            if (DOS_ADDR_INVALID(aszData[lIndex])
                || dos_atoul(aszData[lIndex], &ulServType) < 0)
            {
                DOS_ASSERT(0);
                break;
            }
        }

        ulCount++;
    }

    if (ulCount != 2)
    {
        DOS_ASSERT(0);

        return -1;
    }

    pstBillingPkg = (BS_BILLING_PACKAGE_ST *)dos_dmem_alloc(sizeof(BS_BILLING_PACKAGE_ST));
    if (DOS_ADDR_INVALID(pstBillingPkg))
    {
        DOS_ASSERT(0);

        return -1;
    }
    dos_memzero(pstBillingPkg, sizeof(BS_BILLING_PACKAGE_ST));

    pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(BS_BILLING_PACKAGE_ST));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstBillingPkg);
        pstBillingPkg = NULL;

        return -1;
    }
    HASH_Init_Node(pstHashNode);

    pstBillingPkg->ucServType = (U8)ulServType;
    pstBillingPkg->ulPackageID = ulPkgID;
    pstHashNode->pHandle = pstBillingPkg;

    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_BILLING_PACKAGE_SIZE, ulPkgID);
    if (U32_BUTT == ulHashIndex)
    {
        dos_dmem_free(pstBillingPkg);
        pstBillingPkg = NULL;

        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;
    }

    pthread_mutex_lock(&g_mutexBillingPackageTbl);
    hash_add_node(g_astBillingPackageTbl, pstHashNode, ulHashIndex, NULL);
    pthread_mutex_unlock(&g_mutexBillingPackageTbl);

    return 0;
}

S32 bsd_walk_billing_package_tbl_cb1(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    U32                     ulBillPkgID, ulServType, ulHashIndex, ulIndex;
    U32                     ulSrcAttrType1, ulSrcAttrType2, ulDstAttrType1, ulDstAttrType2;
    U32                     ulFirstBillingCnt, ulNextBillingCnt, ulBillingType;
    BOOL                    blFound;
    HASH_NODE_S             *pstHashNode = NULL;
    BS_BILLING_RULE_ST      stBillingRule;
    BS_BILLING_PACKAGE_ST   *pstBillingPkg = NULL;

    if (DOS_ADDR_INVALID(aszData)
        || DOS_ADDR_INVALID(aszFields))
    {
        DOS_ASSERT(0);

        return -1;
    }

    if (dos_atoul(aszData[0], &ulBillPkgID) < 0
        || dos_atoul(aszData[1], &ulServType) < 0)
    {
        return -1;
    }

    if (dos_atoul(aszData[2], &stBillingRule.ulRuleID) < 0
        || dos_atoul(aszData[5], &ulSrcAttrType1) < 0
        || dos_atoul(aszData[6], &ulSrcAttrType2) < 0
        || dos_atoul(aszData[7], &ulDstAttrType1) < 0
        || dos_atoul(aszData[8], &ulDstAttrType2) < 0
        || dos_atoul(aszData[9], &stBillingRule.ulSrcAttrValue1) < 0
        || dos_atoul(aszData[10], &stBillingRule.ulSrcAttrValue2) < 0
        || dos_atoul(aszData[11], &stBillingRule.ulDstAttrValue1) < 0
        || dos_atoul(aszData[12], &stBillingRule.ulDstAttrValue2) < 0
        || dos_atoul(aszData[13], &stBillingRule.ulFirstBillingUnit) < 0
        || dos_atoul(aszData[14], &stBillingRule.ulNextBillingUnit) < 0
        || dos_atoul(aszData[15], &ulFirstBillingCnt) < 0
        || dos_atoul(aszData[16], &ulNextBillingCnt) < 0
        || dos_atoul(aszData[18], &ulBillingType) < 0
        || dos_atoul(aszData[19], &stBillingRule.ulBillingRate) < 0
        || dos_atoul(aszData[20], &stBillingRule.ulEffectTimestamp) < 0
        || dos_atoul(aszData[21], &stBillingRule.ulExpireTimestamp) < 0)
    {
        DOS_ASSERT(0);

        return -1;
    }

    stBillingRule.ulPackageID = ulBillPkgID;
    stBillingRule.ucSrcAttrType1 = (U8)ulSrcAttrType1;
    stBillingRule.ucSrcAttrType2 = (U8)ulSrcAttrType2;
    stBillingRule.ucDstAttrType1 = (U8)ulDstAttrType1;
    stBillingRule.ucDstAttrType2 = (U8)ulDstAttrType2;
    stBillingRule.ucFirstBillingCnt = (U8)ulFirstBillingCnt;
    stBillingRule.ucNextBillingCnt = (U8)ulNextBillingCnt;
    stBillingRule.ucServType = (U8)ulServType;
    stBillingRule.ucBillingType = (U8)ulBillingType;

    pthread_mutex_lock(&g_mutexBillingPackageTbl);
    blFound = DOS_FALSE;
    HASH_Scan_Table(g_astBillingPackageTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astBillingPackageTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                break;
            }

            pstBillingPkg = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstBillingPkg))
            {
                continue;
            }

            if (pstBillingPkg->ulPackageID == ulBillPkgID
                && pstBillingPkg->ucServType == ulServType)
            {
                /* 遍历找到一个没有使用的插进去 */
                for (ulIndex=0; ulIndex<BS_MAX_BILLING_RULE_IN_PACKAGE; ulIndex++)
                {
                    if (!pstBillingPkg->astRule[ulIndex].ucValid)
                    {
                        dos_memcpy(&pstBillingPkg->astRule[ulIndex], &stBillingRule, sizeof(stBillingRule));

                        pstBillingPkg->astRule[ulIndex].ucValid = DOS_TRUE;

                        break;
                    }
                }

                blFound = DOS_TRUE;
                break;
            }
        }
    }

    if (!blFound)
    {
        ulHashIndex = bs_hash_get_index(BS_HASH_TBL_BILLING_PACKAGE_SIZE, ulBillPkgID);
        if (U32_BUTT == ulHashIndex)
        {
            pthread_mutex_unlock(&g_mutexBillingPackageTbl);

            return DOS_FAIL;
        }

        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);
            pthread_mutex_unlock(&g_mutexBillingPackageTbl);

            return DOS_FAIL;
        }

        pstBillingPkg = dos_dmem_alloc(sizeof(BS_BILLING_PACKAGE_ST));
        if (DOS_ADDR_INVALID(pstBillingPkg))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstHashNode);
            pstHashNode = NULL;
            pthread_mutex_unlock(&g_mutexBillingPackageTbl);

            return DOS_FAIL;
        }

        dos_memzero(pstBillingPkg, sizeof(BS_BILLING_PACKAGE_ST));

        pstBillingPkg->ulPackageID = ulBillPkgID;
        pstBillingPkg->ucServType = (S8)ulServType;

        dos_memcpy(&pstBillingPkg->astRule[0], &stBillingRule, sizeof(stBillingRule));
        pstBillingPkg->astRule[0].ucValid = DOS_TRUE;

        HASH_INIT_NODE(pstHashNode);
        pstHashNode->pHandle = pstBillingPkg;

        hash_add_node(g_astBillingPackageTbl, pstHashNode, ulHashIndex, NULL);
    }

    pthread_mutex_unlock(&g_mutexBillingPackageTbl);

    return DOS_SUCC;
}



/* 遍历资费数据表 */
S32 bsd_walk_billing_package_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    S8 szQuery[1024] = {0, };
    U32 ulHashIndex = 0;
    HASH_NODE_S *pstHashNode = NULL;
    BS_BILLING_PACKAGE_ST *pstBillingPkg = NULL;

    dos_snprintf(szQuery, sizeof(szQuery)
                   , "SELECT "
                     "    t1.id, t2.type, t3.* "
                     "FROM "
                     "   tbl_billing_package t1, "
                     "   tbl_billing_business t2, "
                     "   tbl_billing_rule t3 "
                     "WHERE "
                     "   t2.id IN ( "
                     "       t1.outgoing_calls, "
                     "       t1.incoming_calls, "
                     "       t1.call_forward, "
                     "       t1.call_pickup, "
                     "       t1.call_transfer, "
                     "       t1.conference_call, "
                     "       t1.internal_calls, "
                     "       t1.mms_receive, "
                     "       t1.mms_send, "
                     "       t1.predictive_outbound, "
                     "       t1.recording_business, "
                     "       t1.rent_business, "
                     "       t1.preview_outbound, "
                     "       t1.voice_mail, "
                     "       t1.settle_business, "
                     "       t1.automatic_outbound, "
                     "       t1.sms_receive, "
                     "       t1.sms_send "
                     "   ) "
                     "AND t3.id IN ( "
                     "   t2.billing_rule_1, "
                     "   t2.billing_rule_2, "
                     "   t2.billing_rule_3, "
                     "   t2.billing_rule_4, "
                     "   t2.billing_rule_6, "
                     "   t2.billing_rule_6, "
                     "   t2.billing_rule_7, "
                     "   t2.billing_rule_8, "
                     "   t2.billing_rule_9, "
                     "   t2.billing_rule_10 "
                     "); ");

    if (db_query(g_pstDBHandle, szQuery, bsd_walk_billing_package_tbl_cb1, NULL, NULL) != DB_ERR_SUCC)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read billing package from DB FAIL!");
        return BS_INTER_ERR_FAIL;
    }

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read billing package from DB OK!(%u)", g_astBillingPackageTbl->NodeNum);


    return DOS_SUCC;

    dos_snprintf(szQuery
                , sizeof(szQuery)
                , "SELECT "
                  "  t2.id as id, t1.type "
                  "FROM "
                  "  tbl_billing_business t1 "
                  "LEFT JOIN "
                  "  tbl_billing_package t2 "
                  "ON t1.id IN (t2.outgoing_calls, t2.incoming_calls, t2.call_forward, t2.call_pickup,"
                  "          t2.call_transfer, t2.conference_call, t2.internal_calls, t2.mms_receive,"
                  "          t2.mms_send, t2.predictive_outbound, t2.recording_business, t2.rent_business,"
                  "          t2.preview_outbound, t2.voice_mail, t2.settle_business, t2.automatic_outbound,"
                  "          t2.sms_receive, t2.sms_send);");

    if (db_query(g_pstDBHandle, szQuery, bsd_walk_billing_package_tbl_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read billing package from DB FAIL!");
        return BS_INTER_ERR_FAIL;
    }

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read billing package from DB OK!");

    pthread_mutex_lock(&g_mutexBillingPackageTbl);
    HASH_Scan_Table(g_astBillingPackageTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astBillingPackageTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstBillingPkg = pstHashNode->pHandle;

            dos_snprintf(szQuery
                          , sizeof(szQuery)
                          , "SELECT " \
                            "  *, t2.id " \
                            "FROM " \
                            "  tbl_billing_rule t1 " \
                            "LEFT JOIN  " \
                            "  tbl_billing_business t2 " \
                            "ON " \
                            "  t1.id IN (t2.billing_rule_1,t2.billing_rule_2,t2.billing_rule_3, " \
                            "    t2.billing_rule_4,t2.billing_rule_6,t2.billing_rule_6, " \
                            "    t2.billing_rule_7,t2.billing_rule_8,t2.billing_rule_9, " \
                            "    t2.billing_rule_10) " \
                            "  AND t2.id = %u " \
                            "WHERE t1.serv_type = %u;"
                          , pstBillingPkg->ulPackageID
                          , pstBillingPkg->ucServType);

            if (db_query(g_pstDBHandle, szQuery, bsd_walk_billing_rule_tbl_cb, pstBillingPkg, NULL) != DB_ERR_SUCC)
            {
                bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read billing package from DB FAIL!");

                pthread_mutex_unlock(&g_mutexBillingPackageTbl);
                return BS_INTER_ERR_FAIL;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexBillingPackageTbl);

    return BS_INTER_ERR_SUCC;

}
#if 0
static S32 bsd_walk_settle_tbl_cb(BS_INTER_MSG_WALK *pstMsg)
{
    U32                     ulCnt = 0, ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    BS_SETTLE_ST            *pstSettle = NULL;

    pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    HASH_Init_Node(pstHashNode);

    pstSettle = dos_dmem_alloc(sizeof(BS_SETTLE_ST));
    if (DOS_ADDR_INVALID(pstSettle))
    {
        dos_dmem_free(pstHashNode);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    bs_init_settle_st(pstSettle);

    //TODO : Copy数据到控制块

    pstHashNode->pHandle = (VOID *)pstSettle;
    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_SETTLE_SIZE, pstSettle->usTrunkID);

    pthread_mutex_lock(&g_mutexSettleTbl);
    /* 存放到哈希表之前先查下,确定是否有重复 */
    if(NULL == hash_find_node(g_astSettleTbl,
                              ulHashIndex,
                              (VOID *)pstSettle,
                              bs_settle_hash_node_match))
    {
        ulCnt++;
        hash_add_node(g_astSettleTbl, pstHashNode, ulHashIndex, NULL);
        g_astSettleTbl->NodeNum++;
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "ERR: settle info(trunk:%u, sp:%u) is duplicated in DB !",
                 pstSettle->usTrunkID, pstSettle->ulSPID);
        dos_dmem_free(pstHashNode);
        dos_dmem_free(pstSettle);
    }
    pthread_mutex_unlock(&g_mutexSettleTbl);

    return 0;
}
#endif
/* 遍历结算数据表 */
S32 bsd_walk_settle_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    //TODO : 构造查询数据库语句查询资费表
    //TODO:依次读取每一行数据,相关信息赋值到结构体中

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read settle info from DB !");
    return BS_INTER_ERR_SUCC;
}

static S32 bsd_walk_web_cmd_cb(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    BS_WEB_CMD_INFO_ST      *pszTblRow   = NULL;
    DLL_NODE_S              *pstListNode = NULL;
    JSON_OBJ_ST             *pstJSONObj  = NULL;
    U32                     ulTimestamp = 0;

    /* 查询三个字段 */
    if (lCnt != 3)
    {
        DOS_ASSERT(0);

        return -1;
    }

    if (DOS_ADDR_INVALID(aszData) || DOS_ADDR_INVALID(*aszData))
    {
        DOS_ASSERT(0);

        return -1;
    }

    /* 第一列是时间戳，第2列是json数据 */
    if ( DOS_ADDR_INVALID(aszData[1])
        || DOS_ADDR_INVALID(aszData[2]))
    {
        DOS_ASSERT(0);

        return -1;
    }

    pstListNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        goto error_proc;
    }
    DLL_Init_Node(pstListNode);

    pszTblRow = (BS_WEB_CMD_INFO_ST *)dos_dmem_alloc(sizeof(BS_WEB_CMD_INFO_ST));
    if (DOS_ADDR_INVALID(pszTblRow))
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        goto error_proc;
    }

    pstJSONObj = json_init(aszData[2]);
    if (DOS_ADDR_INVALID(pstJSONObj))
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Warning: Parse the json fail!");
        goto error_proc;
    }

    if (dos_atoul(aszData[1], &ulTimestamp) < 0)
    {
        DOS_ASSERT(0);
        goto error_proc;
    }

    pszTblRow->pstData = pstJSONObj;
    pstListNode->pHandle = pszTblRow;

    pthread_mutex_lock(&g_mutexWebCMDTbl);
    DLL_Add(&g_stWebCMDTbl, pstListNode);
    pthread_mutex_unlock(&g_mutexWebCMDTbl);

    return 0;

error_proc:
    if (pstListNode)
    {
        dos_dmem_free(pstListNode);
    }

    if (pszTblRow)
    {
        dos_dmem_free(pszTblRow);
    }

    if (pstJSONObj)
    {
        json_deinit(&pstJSONObj);
    }

    return -1;
}

/* 遍历WEB命令的临时表 */
S32 bsd_walk_web_cmd_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    S8 szQuery[256] = {0, };

    if (!g_pstDBHandle || g_pstDBHandle->ulDBStatus != DB_STATE_CONNECTED)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_DB, LOG_LEVEL_EMERG, "DB Has been down or not init.");
        return -1;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT id,ctime,json_fields FROM tmp_tbl_modify;");

    if (db_query(g_pstDBHandle, szQuery, bsd_walk_web_cmd_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_NOTIC, "DB query failed.(%s)", szQuery);
        return -1;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "delete from tmp_tbl_modify;");
    db_transaction_begin(g_pstDBHandle);
    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) != DB_ERR_SUCC)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_NOTIC, "DB query failed. (%s)", szQuery);
        db_transaction_rollback(g_pstDBHandle);
        return -1;
    }
    db_transaction_commit(g_pstDBHandle);

    return 0;
}


/* 存储原始话单 */
VOID bsd_save_original_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    U32             i;
    BS_MSG_CDR      *pstCDR;
    S8              szQuery[1024] = { 0, };

    pstCDR = (BS_MSG_CDR *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    if (pstCDR->ucLegNum > BS_MAX_SESSION_LEG_IN_BILL)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_CDR, LOG_LEVEL_ERROR, "ERR: leg num(%u) in CDR is too many!", pstCDR->ucLegNum);
        return;
    }

    /* 依次读取每个LEG的信息,转化为话单存储到数据库中 */
    for(i = 0; i < pstCDR->ucLegNum; i++)
    {

        //TODO:存储话单到数据库中
        dos_snprintf(szQuery, sizeof(szQuery), "INSERT IGNORE INTO "
                    	"tbl_cdr (id, customer_id, account_id, user_id, task_id, type1, type2, type3"
                    	", record_file, caller, callee, CID, agent_num, start_time, ring_time"
                    	", answer_time, ivr_end_time, dtmf_time, hold_cnt, hold_times, peer_trunk_id"
                    	", terminate_cause, release_part, payload_type, package_loss_rate, cdr_mark"
                    	", sessionID, bridge_time, bye_time, peer_ip1, peer_ip2, tbl_cdr.peer_ip3, peer_ip4)"
                    "VALUES(NULL, %u, %u, %u, %u, %u, %u, %u, \"%s\", \"%s\", \"%s\", \"%s\""
                        ", \"%s\", FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), %u, %u, %u, %u, %u, %u, %u, %u, \"%s\""
                        ", FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), %u, %u, %u, %u);"
                    , pstCDR->astSessionLeg[i].ulCustomerID
                    , pstCDR->astSessionLeg[i].ulAccountID
                    , pstCDR->astSessionLeg[i].ulUserID
                    , pstCDR->astSessionLeg[i].ulTaskID
                    , pstCDR->astSessionLeg[i].aucServType[0]
                    , pstCDR->astSessionLeg[i].aucServType[1]
                    , pstCDR->astSessionLeg[i].aucServType[2]
                    , pstCDR->astSessionLeg[i].szRecordFile
                    , pstCDR->astSessionLeg[i].szCaller
                    , pstCDR->astSessionLeg[i].szCallee
                    , pstCDR->astSessionLeg[i].szCID
                    , pstCDR->astSessionLeg[i].szAgentNum
                    , pstCDR->astSessionLeg[i].ulStartTimeStamp
                    , pstCDR->astSessionLeg[i].ulRingTimeStamp
                    , pstCDR->astSessionLeg[i].ulAnswerTimeStamp
                    , pstCDR->astSessionLeg[i].ulIVRFinishTimeStamp
                    , pstCDR->astSessionLeg[i].ulDTMFTimeStamp
                    , pstCDR->astSessionLeg[i].ulHoldCnt
                    , pstCDR->astSessionLeg[i].ulHoldTimeLen
                    , pstCDR->astSessionLeg[i].usPeerTrunkID
                    , pstCDR->astSessionLeg[i].usTerminateCause
                    , pstCDR->astSessionLeg[i].ucReleasePart
                    , pstCDR->astSessionLeg[i].ucPayloadType
                    , pstCDR->astSessionLeg[i].ucPacketLossRate
                    , pstCDR->astSessionLeg[i].ulCDRMark
                    , pstCDR->astSessionLeg[i].szSessionID
                    , pstCDR->astSessionLeg[i].ulBridgeTimeStamp
                    , pstCDR->astSessionLeg[i].ulByeTimeStamp
                    , pstCDR->astSessionLeg[i].aulPeerIP[0]
                    , pstCDR->astSessionLeg[i].aulPeerIP[1]
                    , pstCDR->astSessionLeg[i].aulPeerIP[2]
                    , pstCDR->astSessionLeg[i].aulPeerIP[3]);

        if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
        {
            bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
            continue;
        }

        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }

}

/* 存储语音话单 */
VOID bsd_save_voice_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_VOICE_ST *pstCDR;
    S8              szQuery[1024] = { 0, };

    pstCDR = (BS_CDR_VOICE_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "INSERT IGNORE INTO tbl_cdr_voice("
                	"`id`,`customer_id`,`account_id`,`user_id`,`task_id`,`billing_rule_id`,`type`,`fee_l1`,"
                	"`fee_l2`,`fee_l3`,`fee_l4`,`fee_l5`,`record_file`,`caller`,`callee`,`CID`,`agent_num`,"
                	"`pdd_len`,`ring_times`,`answer_time`,`ivr_end_times`,`dtmf_times`,`wait_agent_times`,"
                	"`time_len`,`hold_cnt`,`hold_times`,`peer_trunk_id`,`terminate_cause`,`release_part`,"
                	"`payload_type`,`package_loss_rate`,`record_flag`,`agent_level`,`cdr_mark`,`cdr_type`,"
                	"`peer_ip1`,`peer_ip2`,`peer_ip3`,`peer_ip4`)"
                "VALUES(NULL, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, \"%s\", \"%s\", \"%s\""
                	", \"%s\", \"%s\", %u, %u, FROM_UNIXTIME(%u), %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u"
                	", %u, %u, %u, %u, %u, %u, %u, %u);"
                	, pstCDR->ulCustomerID, pstCDR->ulAccountID, pstCDR->ulUserID
                	, pstCDR->ulTaskID, pstCDR->ulRuleID, pstCDR->ucServType
                	, pstCDR->aulFee[0], pstCDR->aulFee[1], pstCDR->aulFee[2]
                	, pstCDR->aulFee[3], pstCDR->aulFee[4], pstCDR->szRecordFile
                	, pstCDR->szCaller, pstCDR->szCallee, pstCDR->szCID, pstCDR->szAgentNum
                	, pstCDR->ulPDDLen, pstCDR->ulRingTime, pstCDR->ulAnswerTimeStamp
                	, pstCDR->ulIVRFinishTime, pstCDR->ulDTMFTime, pstCDR->ulWaitAgentTime
                	, pstCDR->ulTimeLen, pstCDR->ulHoldCnt, pstCDR->ulHoldTimeLen
                	, pstCDR->usPeerTrunkID, pstCDR->usTerminateCause, pstCDR->ucReleasePart
                	, pstCDR->ucPayloadType, pstCDR->ucPacketLossRate, pstCDR->ucRecordFlag
                	, pstCDR->ucAgentLevel, pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType
                	, pstCDR->aulPeerIP[0], pstCDR->aulPeerIP[1], pstCDR->aulPeerIP[2], pstCDR->aulPeerIP[3]);

    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }
}

/* 存储语音话单 */
VOID bsd_save_recording_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_RECORDING_ST *pstCDR;
    S8                  szQuery[1024] = { 0, };

    pstCDR = (BS_CDR_RECORDING_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "INSERT INTO `tbl_cdr_record` ("
                      "`id`,`customer_id`,`account_id`,`user_id`,`task_id`,`billing_rule_id`,"
                      "`fee_l1`,`fee_l2`,`fee_l3`,`fee_l4`,`fee_l5`,"
                      "`record_file`,`caller`,`callee`,`CID`,`agent_num`,"
                      "`start_time`,`time_len`,`agent_level`,`cdr_mark`,`cdr_type`)"
                    "VALUES(NULL, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, \"%s\""
                    		, "\%s\", \"%s\", \"%s\", \"%s\", %u, %u, %u, %u, %u);"
                	, pstCDR->ulCustomerID, pstCDR->ulAccountID, pstCDR->ulUserID
                	, pstCDR->ulTaskID, pstCDR->ulRuleID, pstCDR->aulFee[0]
                	, pstCDR->aulFee[1], pstCDR->aulFee[2], pstCDR->aulFee[3]
                	, pstCDR->aulFee[4], pstCDR->szRecordFile, pstCDR->szCaller
                	, pstCDR->szCallee, pstCDR->szCID, pstCDR->szAgentNum
                	, pstCDR->ulRecordTimeStamp, pstCDR->ulTimeLen, pstCDR->ucAgentLevel
                	, pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType);

    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }
}

/* 存储消息话单 */
VOID bsd_save_message_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_MS_ST    *pstCDR;
    S8              szQuery[1024] = { 0, };

    pstCDR = (BS_CDR_MS_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "INSERT INTO `tbl_cdr_ms` ("
                      "`id`,`customer_id`,`account_id`,`user_id`,`sms_id`,`billing_rule_id`,"
                      "`type`,`fee_l1`,`fee_l2`,`fee_l3`,`fee_l4`,`fee_l5`,`caller`,`callee`,"
                      "`agent_num`,`deal_time`,`arrived_time`,`msg_len`,`peer_trunk_id`,"
                      "`terminate_cause`,`agent_level`,`cdr_mark`,`cdr_type`,`peer_ip1`,"
                      "`peer_ip2`,`peer_ip3`,`peer_ip4`)"
                    "VALUES (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u"
                    	", \"%s\", \"%s\", \"%s\", %u, %u, %u, %u, %u, %u"
                    	", %u, %u, %u, %u, %u, %u);"
                	, pstCDR->ulCustomerID, pstCDR->ulAccountID, pstCDR->ulUserID, 0
                	, pstCDR->ulRuleID, pstCDR->ucServType, pstCDR->aulFee[0]
                	, pstCDR->aulFee[1], pstCDR->aulFee[2], pstCDR->aulFee[3]
                	, pstCDR->aulFee[4], pstCDR->szCaller, pstCDR->szCallee
                	, pstCDR->szAgentNum, pstCDR->ulTimeStamp, pstCDR->ulArrivedTimeStamp
                	, pstCDR->ulLen, pstCDR->usPeerTrunkID, pstCDR->usTerminateCause, pstCDR->ucAgentLevel
                	, pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType, pstCDR->aulPeerIP[0]
                	, pstCDR->aulPeerIP[1], pstCDR->aulPeerIP[2], pstCDR->aulPeerIP[3]);

    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }


}

/* 存储结算话单 */
VOID bsd_save_settle_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_SETTLE_ST    *pstCDR;
    S8                  szQuery[1024] = { 0, };

    pstCDR = (BS_CDR_SETTLE_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }
    dos_snprintf(szQuery, sizeof(szQuery), "INSERT INTO `tbl_cdr_settle` ("
                      "`id`, `sp_customer_ id`,`billing_rule_id`,`ctime`,"
                      "`type`,`fee`,`caller`,`callee`,`deal_times`,`peer_trunk_id`,"
                      "`terminate_cause`,`cdr_mark`,`cdr_type`,`peer_ip1`,"
                      "`peer_ip2`,`peer_ip3`,`peer_ip4`)"
                    "VALUES(NULL, %u, %u, %u, %u, %u, \"%s\", \"%s\", %u"
                    	", %u, %u, %u, %u, %u, %u, %u, %u);"
                	, pstCDR->ulSPID, pstCDR->ulRuleID, pstCDR->ulTimeStamp
                	, pstCDR->ucServType, pstCDR->ulFee, pstCDR->szCaller
                	, pstCDR->szCallee, pstCDR->ulTimeStamp, pstCDR->usPeerTrunkID
                	, pstCDR->usTerminateCause, pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType
                	, pstCDR->aulPeerIP[0], pstCDR->aulPeerIP[1], pstCDR->aulPeerIP[2], pstCDR->aulPeerIP[3]);

    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }

}

/* 存储租金话单 */
VOID bsd_save_rent_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_RENT_ST    *pstCDR;
    S8                szQuery[1024] = { 0, };

    pstCDR = (BS_CDR_RENT_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "INSERT INTO `tbl_cdr_rent` ("
                      "`id`,`customer_id`,`account_id`,`billing_rule_id`,`ctime`,"
                      "`type`,`fee_l1`,`fee_l2`,`fee_l3`,`fee_l4`,`fee_l5`,"
                      "`agent_level`,`cdr_mark`,`cdr_type`)"
                    "VALUES(NULL, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u);"
                	, pstCDR->ulCustomerID, pstCDR->ulAccountID, pstCDR->ulRuleID, pstCDR->ulTimeStamp
                	, pstCDR->ucAttrType, pstCDR->aulFee[0], pstCDR->aulFee[1], pstCDR->aulFee[2]
                	, pstCDR->aulFee[3], pstCDR->aulFee[4], pstCDR->ucAgentLevel, pstCDR->stCDRTag.ulCDRMark
                	, pstCDR->stCDRTag.ucCDRType);

    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }
}

/* 存储账户类话单 */
VOID bsd_save_account_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_ACCOUNT_ST   *pstCDR;
    S8                  szQuery[1024] = { 0, };

    pstCDR = (BS_CDR_ACCOUNT_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "INSERT IGNORE INTO `tbl_cdr_account` ("
                      "`id`,`customer_id`,`account_id`,`ctime`,"
                      "`type`,`money`,`balance`,`peer_account_id`,"
                      "`operator_id`,`note`)"
                    "VALUES(NULL, %u, %u, %u, %u, %u, %u, %u, %u, \"%s\");"
                	, pstCDR->ulCustomerID, pstCDR->ulAccountID, pstCDR->ulTimeStamp
                	, pstCDR->ucOperateType, pstCDR->lMoney, pstCDR->LBalance
                	, pstCDR->ulPeeAccount, pstCDR->ulOperatorID, pstCDR->szRemark);

    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }


}

/* 存储出局呼叫统计 */
VOID bsd_save_outband_stat(BS_INTER_MSG_STAT *pstMsg)
{
    BS_STAT_OUTBAND_ST  *pstStat;

    pstStat = (BS_STAT_OUTBAND_ST *)pstMsg->pStat;
    if (DOS_ADDR_INVALID(pstStat))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:存储统计信息到数据库中

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save outband stat in DB !");

}

/* 存储出局呼叫统计 */
VOID bsd_save_inband_stat(BS_INTER_MSG_STAT *pstMsg)
{
    BS_STAT_INBAND_ST   *pstStat;

    pstStat = (BS_STAT_INBAND_ST *)pstMsg->pStat;
    if (DOS_ADDR_INVALID(pstStat))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:存储统计信息到数据库中

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save inband stat in DB !");

}

/* 存储出局呼叫统计 */
VOID bsd_save_outdialing_stat(BS_INTER_MSG_STAT *pstMsg)
{
    BS_STAT_OUTDIALING_ST   *pstStat;

    pstStat = (BS_STAT_OUTDIALING_ST *)pstMsg->pStat;
    if (DOS_ADDR_INVALID(pstStat))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:存储统计信息到数据库中

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save out dialing stat in DB !");

}

/* 存储出局呼叫统计 */
VOID bsd_save_message_stat(BS_INTER_MSG_STAT *pstMsg)
{
    BS_STAT_MESSAGE_ST  *pstStat;

    pstStat = (BS_STAT_MESSAGE_ST *)pstMsg->pStat;
    if (DOS_ADDR_INVALID(pstStat))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:存储统计信息到数据库中

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save message stat in DB !");

}

/* 存储出局呼叫统计 */
VOID bsd_save_account_stat(BS_INTER_MSG_STAT *pstMsg)
{
    BS_STAT_ACCOUNT_ST  *pstStat;

    pstStat = (BS_STAT_ACCOUNT_ST *)pstMsg->pStat;
    if (DOS_ADDR_INVALID(pstStat))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:存储统计信息到数据库中

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save account stat in DB !");

}


S32 bs_init_db()
{
    g_pstDBHandle = db_create(DB_TYPE_MYSQL);
    if (!g_pstDBHandle)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create db handle fail!");
        return -1;
    }

    if (config_get_db_host(g_pstDBHandle->szHost, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get DB host fail !");
        goto errno_proc;
    }

    if (config_get_db_user(g_pstDBHandle->szUsername, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get DB username fail !");
        goto errno_proc;
    }

    if (config_get_db_password(g_pstDBHandle->szPassword, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get DB username password fail !");
        goto errno_proc;
    }

    g_pstDBHandle->usPort = config_get_db_port();
    if (0 == g_pstDBHandle->usPort || g_pstDBHandle->usPort)
    {
        g_pstDBHandle->usPort = 3306;
    }

    if (config_get_db_dbname(g_pstDBHandle->szDBName, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get DB Name fail !");
        goto errno_proc;
    }

    if (db_open(g_pstDBHandle) < 0)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Open DB fail !");
        goto errno_proc;
    }

    return 0;

errno_proc:
    db_destroy(&g_pstDBHandle);
    return -1;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

