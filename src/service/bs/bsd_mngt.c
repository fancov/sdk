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
#include "bs_pub.h"
#include "bs_cdr.h"
#include "bs_stat.h"
#include "bs_def.h"
#include "bsd_db.h"

/* 表遍历请求处理 */
S32 bsd_walk_tbl_req(DLL_NODE_S *pMsgNode)
{
    S32                 lRet = BS_INTER_ERR_FAIL;
    BS_INTER_MSG_WALK   *pstMsg;

    /* 前面已经判断过地址合法性,此处直接使用即可 */
    pstMsg = (BS_INTER_MSG_WALK *)pMsgNode->pHandle;
    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Start walk tbl:%u!", pstMsg->ulTblType);

    switch (pstMsg->ulTblType)
    {
        case BS_TBL_TYPE_CUSTOMER:
            lRet = bsd_walk_customer_tbl(pstMsg);
            bsd_send_walk_rsp2sl(pMsgNode, lRet);
            break;

        case BS_TBL_TYPE_AGENT:
            lRet = bsd_walk_agent_tbl(pstMsg);
            bsd_send_walk_rsp2sl(pMsgNode, lRet);
            break;

        case BS_TBL_TYPE_BILLING_PACKAGE:
            lRet = bsd_walk_billing_package_tbl(pstMsg);
            bsd_send_walk_rsp2sl(pMsgNode, lRet);
            break;
        case BS_TBL_TYPE_BILLING_RULE:
            bsd_inherit_rule_req2sl(pMsgNode);
            break;
        case BS_TBL_TYPE_SETTLE:
            lRet = bsd_walk_settle_tbl(pstMsg);
            bsd_send_walk_rsp2sl(pMsgNode, lRet);
            break;
        case BS_TBL_TYPE_TMP_CMD:
            lRet = bsd_walk_web_cmd_tbl(pstMsg);
            bsd_send_walk_rsp2sl(pMsgNode, lRet);
            break;
        case BS_TBL_TYPE_TMP_CMD_DEL:
            bsd_delete_web_cmd_tbl(pstMsg);
            break;
        default:
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_WARNING, "Warning: It's a unknown table");
            /* 未知消息,不做处理,释放内存 */
            dos_dmem_free(pMsgNode->pHandle);
            pMsgNode->pHandle = NULL;
            dos_dmem_free(pMsgNode);
            break;
    }
    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Walk result:%d!", lRet);

    return lRet;

}

/* 保存原始话单 */
VOID bsd_cdr(DLL_NODE_S *pMsgNode)
{
    BS_INTER_MSG_CDR    *pstMsg;

    /* 前面已经判断过地址合法性,此处直接使用即可 */
    pstMsg = (BS_INTER_MSG_CDR *)pMsgNode->pHandle;

    switch (pstMsg->stMsgTag.ucMsgType)
    {
        case BS_INTER_MSG_ORIGINAL_CDR:
            bsd_save_original_cdr(pstMsg);
            break;

        case BS_INTER_MSG_VOICE_CDR:
            bsd_save_voice_cdr(pstMsg);
            break;

        case BS_INTER_MSG_RECORDING_CDR:
            bsd_save_recording_cdr(pstMsg);
            break;

        case BS_INTER_MSG_MESSAGE_CDR:
            bsd_save_message_cdr(pstMsg);
            break;

        case BS_INTER_MSG_SETTLE_CDR:
            bsd_save_settle_cdr(pstMsg);
            break;

        case BS_INTER_MSG_RENT_CDR:
            bsd_save_rent_cdr(pstMsg);
            break;

        case BS_INTER_MSG_ACCOUNT_CDR:
            bsd_save_account_cdr(pstMsg);
            break;

        default:
            DOS_ASSERT(0);
            break;
    }



    /* 将原始话单释放掉 */
    dos_dmem_free(pstMsg->pCDR);
    pstMsg->pCDR = NULL;
    dos_dmem_free(pMsgNode->pHandle);
    pMsgNode->pHandle = NULL;
    dos_dmem_free(pMsgNode);

}

/* 统计信息处理 */
VOID bsd_stat(DLL_NODE_S *pMsgNode)
{
    BS_INTER_MSG_STAT   *pstMsg;

    /* 前面已经判断过地址合法性,此处直接使用即可 */
    pstMsg = (BS_INTER_MSG_STAT *)pMsgNode->pHandle;
    switch (pstMsg->stMsgTag.ucMsgType)
    {
        case BS_INTER_MSG_OUTBAND_STAT:
            bsd_save_outband_stat(pstMsg);
            break;

        case BS_INTER_MSG_INBAND_STAT:
            bsd_save_inband_stat(pstMsg);
            break;

        case BS_INTER_MSG_OUTDIALING_STAT:
            bsd_save_outdialing_stat(pstMsg);
            break;

        case BS_INTER_MSG_MESSAGE_STAT:
            bsd_save_message_stat(pstMsg);
            break;

        case BS_INTER_MSG_ACCOUNT_STAT:
            bsd_save_account_stat(pstMsg);
            break;

        default:
            DOS_ASSERT(0);
            break;
    }

    /* 将原始话单释放掉 */
    dos_dmem_free(pstMsg->pStat);
    pstMsg->pStat = NULL;
    dos_dmem_free(pMsgNode->pHandle);
    pMsgNode->pHandle = NULL;
    dos_dmem_free(pMsgNode);

}


/* 业务层消息处理函数 */
VOID bsd_sl_msg_proc(DLL_NODE_S *pMsgNode)
{
    BS_INTER_MSG_TAG    *pstMsgTag;

    if (DOS_ADDR_INVALID(pMsgNode))
    {
        DOS_ASSERT(0);
        return;
    }

    pstMsgTag = (BS_INTER_MSG_TAG *)pMsgNode->pHandle;
    if (DOS_ADDR_INVALID(pstMsgTag))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pMsgNode);
        return;
    }

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Recv msg from sl, type:%u, len:%u, errcode:%d",
             pstMsgTag->ucMsgType, pstMsgTag->usMsgLen, pstMsgTag->lInterErr);

    switch (pstMsgTag->ucMsgType)
    {
        case BS_INTER_MSG_WALK_REQ:
            bsd_walk_tbl_req(pMsgNode);
            break;

        case BS_INTER_MSG_ORIGINAL_CDR:
        case BS_INTER_MSG_VOICE_CDR:
        case BS_INTER_MSG_RECORDING_CDR:
        case BS_INTER_MSG_MESSAGE_CDR:
        case BS_INTER_MSG_SETTLE_CDR:
        case BS_INTER_MSG_RENT_CDR:
        case BS_INTER_MSG_ACCOUNT_CDR:
            bsd_cdr(pMsgNode);
            break;


        case BS_INTER_MSG_OUTBAND_STAT:
        case BS_INTER_MSG_INBAND_STAT:
        case BS_INTER_MSG_OUTDIALING_STAT:
        case BS_INTER_MSG_MESSAGE_STAT:
        case BS_INTER_MSG_ACCOUNT_STAT:
            bsd_stat(pMsgNode);
            break;

        default:
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_WARNING, "Warning: Unknown msg type:%u", pstMsgTag->ucMsgType);
            dos_dmem_free(pMsgNode->pHandle);
            pMsgNode->pHandle = NULL;
            dos_dmem_free(pMsgNode);

            break;
    }

}

/* 数据维护线程处理 */
VOID *bsd_recv_bss_msg(VOID *arg)
{
    DLL_NODE_S *pNode;
    struct timespec         stTimeout;

    while (1)
    {
        pNode = NULL;

        /* 读取消息队列第一个数据 */
        pthread_mutex_lock(&g_mutexBSS2DMsg);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condBSS2DList, &g_mutexBSS2DMsg, &stTimeout);

        while (1)
        {
            if (DLL_Count(&g_stBSS2DMsgList) <= 0)
            {
                break;
            }

            pNode = dll_fetch(&g_stBSS2DMsgList);
            if (NULL == pNode)
            {
                continue;
            }

            /* 队列消息处理 */
            bsd_sl_msg_proc(pNode);
        }

        pthread_mutex_unlock(&g_mutexBSS2DMsg);
    }
}

/* 数据备份线程处理 */
VOID *bsd_backup(VOID *arg)
{
    while (1)
    {
        //TODO
        dos_task_delay(BS_BACKUP_INTERVAL * 1000);
    }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


