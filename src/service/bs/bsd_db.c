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



/* 遍历客户数据表 */
S32 bsd_walk_customer_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    U32             i, ulCnt = 0, ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;
    BS_CUSTOMER_ST  *pstCustomer = NULL;

    //TODO:将数据从数据库中读取出来



    //TODO:依次读取每一行数据,相关信息赋值到客户信息结构体中
    for(i = 0; i < 0; i++)
    {
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

        pstHashNode->pHandle = (VOID *)pstCustomer;
        ulHashIndex = bs_hash_get_index(BS_HASH_TBL_CUSTOMER_SIZE, pstCustomer->ulCustomerID);

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
    }

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read total %u customers from DB !", ulCnt);
    return BS_INTER_ERR_SUCC;

}

/* 遍历坐席表 */
S32 bsd_walk_agent_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    U32                     i, ulCnt = 0, ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    HASH_NODE_S             *pstMatchNode = NULL;
    BS_AGENT_ST             *pstAgent = NULL;

    //TODO:将数据从数据库中读取出来



    //TODO:依次读取每一行数据,相关信息赋值到客户信息结构体中
    for(i = 0; i < 0; i++)
    {
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

        //TODO:拷贝数据库信息到结构体中


        pstHashNode->pHandle = (VOID *)pstAgent;
        ulHashIndex = bs_hash_get_index(BS_HASH_TBL_AGENT_SIZE, pstAgent->ulAgentID);

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
    }

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read total %u agents info from DB !", ulCnt);
    return BS_INTER_ERR_SUCC;

}

/* 遍历资费数据表 */
S32 bsd_walk_billing_package_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    U32                     i, ulCnt = 0, ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    HASH_NODE_S             *pstMatchNode = NULL;
    BS_BILLING_PACKAGE_ST   *pstBillingPackage = NULL;
    BS_BILLING_PACKAGE_ST   *pstMatchPackage = NULL;

    //TODO:将数据从数据库中读取出来



    //TODO:依次读取每一行数据,相关信息赋值到结构体中
    for(i = 0; i < 0; i++)
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
            return BS_INTER_ERR_MEM_ALLOC_FAIL;
        }
        HASH_Init_Node(pstHashNode);

        pstBillingPackage = dos_dmem_alloc(sizeof(BS_BILLING_PACKAGE_ST));
        if (DOS_ADDR_INVALID(pstBillingPackage))
        {
            dos_dmem_free(pstHashNode);
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
            return BS_INTER_ERR_MEM_ALLOC_FAIL;
        }
        bs_init_billing_package_st(pstBillingPackage);

        //TODO:拷贝数据库信息到结构体中
        /* 资费规则先存到数组0号下标中 */

        pstHashNode->pHandle = (VOID *)pstBillingPackage;
        ulHashIndex = bs_hash_get_index(BS_HASH_TBL_BILLING_PACKAGE_SIZE, pstBillingPackage->ulPackageID);

        pthread_mutex_lock(&g_mutexBillingPackageTbl);
        /* 存放到哈希表之前先查下,确定是否有重复 */
        pstMatchNode = hash_find_node(g_astBillingPackageTbl,
                                      ulHashIndex,
                                      (VOID *)&pstBillingPackage,
                                      bs_billing_package_hash_node_match);
        if(DOS_ADDR_INVALID(pstMatchNode))
        {
            ulCnt++;
            hash_add_node(g_astBillingPackageTbl, pstHashNode, ulHashIndex, NULL);
            g_astBillingPackageTbl->NodeNum++;
        }
        else
        {
            /* 每个业务有多条计费规则,如果冲突,直接覆盖 */

            U8  ucPriority;

            ucPriority = pstBillingPackage->astRule[0].ucPriority;
            if (ucPriority >= BS_MAX_BILLING_RULE_IN_PACKAGE)
            {
                bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR,
                         "ERR: priority(%u) of billing rule(package:%u, service:%u) is wrong in DB !",
                         ucPriority, pstBillingPackage->ulPackageID, pstBillingPackage->ucServType);
            }
            else
            {
                pstMatchPackage = (BS_BILLING_PACKAGE_ST *)pstMatchNode->pHandle;
                pstMatchPackage->astRule[ucPriority] = pstBillingPackage->astRule[0];
            }

            /* 不用创建hash表节点,释放内存 */
            dos_dmem_free(pstHashNode);
            dos_dmem_free(pstBillingPackage);
        }
        pthread_mutex_unlock(&g_mutexBillingPackageTbl);
    }

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read total %u billing package from DB !", ulCnt);
    return BS_INTER_ERR_SUCC;

}

/* 遍历结算数据表 */
S32 bsd_walk_settle_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    U32                     i, ulCnt = 0, ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    BS_SETTLE_ST            *pstSettle = NULL;

    //TODO:将数据从数据库中读取出来



    //TODO:依次读取每一行数据,相关信息赋值到结构体中
    for(i = 0; i < 0; i++)
    {
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

        //TODO:拷贝数据库信息到结构体中

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
    }

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read total %u settle info from DB !", ulCnt);
    return BS_INTER_ERR_SUCC;

}

/* 存储原始话单 */
VOID bsd_save_original_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    U32             i;
    BS_MSG_CDR      *pstCDR;

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

        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");

    }

}

/* 存储语音话单 */
VOID bsd_save_voice_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_VOICE_ST *pstCDR;

    pstCDR = (BS_CDR_VOICE_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:存储话单到数据库中

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save voice CDR in DB !");

}

/* 存储语音话单 */
VOID bsd_save_recording_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_RECORDING_ST *pstCDR;

    pstCDR = (BS_CDR_RECORDING_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:存储话单到数据库中

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save recording CDR in DB !");

}

/* 存储消息话单 */
VOID bsd_save_message_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_MS_ST    *pstCDR;

    pstCDR = (BS_CDR_MS_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:存储话单到数据库中

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save message CDR in DB !");

}

/* 存储结算话单 */
VOID bsd_save_settle_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_SETTLE_ST    *pstCDR;

    pstCDR = (BS_CDR_SETTLE_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:存储话单到数据库中

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save settle CDR in DB !");

}

/* 存储租金话单 */
VOID bsd_save_rent_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_RENT_ST    *pstCDR;

    pstCDR = (BS_CDR_RENT_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:存储话单到数据库中

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save rent CDR in DB !");

}

/* 存储账户类话单 */
VOID bsd_save_account_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_ACCOUNT_ST   *pstCDR;

    pstCDR = (BS_CDR_ACCOUNT_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:存储话单到数据库中

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save account CDR in DB !");

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





#ifdef __cplusplus
}
#endif /* __cplusplus */

