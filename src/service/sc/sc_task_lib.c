/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名：sc_task_lib.c
 *
 *  创建时间: 2014年12月16日10:23:53
 *  作    者: Larry
 *  描    述: 每一个群呼任务公共库函数
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <time.h>

/* include private header files */
#include "sc_pub.h"
#include "sc_task_pub.h"
#include "sc_debug.h"
#include "sc_httpd.h"
#include "sc_event_process.h"


#define SC_USE_BUILDIN_DATA 0
#if SC_USE_BUILDIN_DATA
const S8 *g_pszCallerList[] = {
    "075526361510", "075526361511", "075526361512"
};
const S8 *g_pszCalledList[] = {
    "18719065681", "18719065682", "18719065683",
    "18719065611", "18719065612", "18719065613",
    "18719065621", "18719065622", "18719065623",
    "18719065631", "18719065632", "18719065633"
};

const S8 *g_pszSiteList[] = {
    "2000", "2001", "2002"
};

#endif


/* define marcos */

/* define enums */

/* define structs */

extern U32       g_ulTaskTraceAll;
extern SC_TASK_MNGT_ST   *g_pstTaskMngtInfo;

static S8 *g_pszCCBStatus[] =
{
    "IDEL",
    "INIT",
    "AUTH",
    "EXEC",
    "ACTIVE",
    "RELEASE"
    ""
};

S8 *sc_ccb_get_status(U32 ulStatus)
{
    if (ulStatus >= SC_CCB_BUTT)
    {
        DOS_ASSERT(0);

        return "";
    }

    return g_pszCCBStatus[ulStatus];
}

/* declare functions */
SC_CCB_ST *sc_ccb_alloc()
{
    static U32 ulIndex = 0;
    U32 ulLastIndex;
    SC_CCB_ST   *pstCCB = NULL;

    SC_TRACE_IN(ulIndex, 0, 0, 0);

    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCallList);

    ulLastIndex = ulIndex;

    for (; ulIndex<SC_MAX_CCB_NUM; ulIndex++)
    {
        if (!sc_ccb_is_valid(&g_pstTaskMngtInfo->pstCallCCBList[ulIndex]))
        {
            pstCCB = &g_pstTaskMngtInfo->pstCallCCBList[ulIndex];
            break;
        }
    }

    if (!pstCCB)
    {
        for (ulIndex = 0; ulIndex < ulLastIndex; ulIndex++)
        {
            if (!sc_ccb_is_valid(&g_pstTaskMngtInfo->pstCallCCBList[ulIndex]))
            {
                pstCCB = &g_pstTaskMngtInfo->pstCallCCBList[ulIndex];
                break;
            }
        }
    }

    if (pstCCB)
    {
        ulIndex++;
        pthread_mutex_lock(&pstCCB->mutexCCBLock);
        sc_ccb_init(pstCCB);
        pstCCB->bValid = 1;
        sc_call_trace(pstCCB, "Alloc CCB.");
        pthread_mutex_unlock(&pstCCB->mutexCCBLock);
        pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallList);
        SC_TRACE_OUT();
        return pstCCB;
    }


    DOS_ASSERT(0);

    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallList);
    SC_TRACE_OUT();
    return NULL;
}

VOID sc_ccb_free(SC_CCB_ST *pstCCB)
{
    SC_TRACE_IN((U64)pstCCB, 0, 0, 0);

    if (!pstCCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return;
    }

    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCallList);
    pthread_mutex_lock(&pstCCB->mutexCCBLock);
    sc_call_trace(pstCCB, "Free CCB.");
    sc_ccb_init(pstCCB);
    pthread_mutex_unlock(&pstCCB->mutexCCBLock);
    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallList);

    SC_TRACE_OUT();
    return;
}

BOOL sc_ccb_is_valid(SC_CCB_ST *pstCCB)
{
    BOOL ulValid = DOS_FALSE;

    SC_TRACE_IN(pstCCB, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstCCB))
    {
        return DOS_FALSE;
    }

    pthread_mutex_lock(&pstCCB->mutexCCBLock);
    ulValid = pstCCB->bValid;
    pthread_mutex_unlock(&pstCCB->mutexCCBLock);

    SC_TRACE_OUT();

    return ulValid;
}


U32 sc_ccb_init(SC_CCB_ST *pstCCB)
{
    U32 i;

    SC_TRACE_IN((U64)pstCCB, 0, 0, 0);

    if (!pstCCB)
    {
        SC_TRACE_OUT();
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sem_destroy(&pstCCB->semCCBSyn);
    sem_init(&pstCCB->semCCBSyn, 0, 0);

    pstCCB->usOtherCCBNo = U16_BUTT;           /* 另外一个leg的CCB编号 */

    pstCCB->usTCBNo = U16_BUTT;                /* 任务控制块编号ID */
    pstCCB->usSiteNo = U16_BUTT;               /* 坐席编号 */

    pstCCB->ulCustomID = U32_BUTT;             /* 当前呼叫属于哪个客户 */
    pstCCB->ulAgentID = U32_BUTT;              /* 当前呼叫属于哪个客户 */
    pstCCB->ulTaskID = U32_BUTT;               /* 当前任务ID */
    pstCCB->ulTrunkID = U32_BUTT;              /* 中继ID */

    pstCCB->ucStatus = SC_CCB_IDEL;            /* 呼叫控制块编号，refer to SC_CCB_STATUS_EN */
    pstCCB->ucTerminationFlag = 0;             /* 业务终止标志 */
    pstCCB->ucTerminationCause = 0;            /* 业务终止原因 */
    pstCCB->ucPayloadType = U8_BUTT;           /* 编解码 */

    pstCCB->usHoldCnt = 0;                     /* 被hold的次数 */
    pstCCB->usHoldTotalTime = 0;               /* 被hold的总时长 */
    pstCCB->ulLastHoldTimetamp = 0;            /* 上次hold是的时间戳，解除hold的时候值零 */

    pstCCB->bValid = DOS_FALSE;                /* 是否合法 */
    pstCCB->bTraceNo = DOS_FALSE;              /* 是否跟踪 */
    pstCCB->bBanlanceWarning = DOS_FALSE;      /* 是否余额告警 */
    pstCCB->bNeedConnSite = DOS_FALSE;         /* 接通后是否需要接通坐席 */
    pstCCB->bWaitingOtherRelase = DOS_FALSE;   /* 是否在等待另外一跳退释放 */

    pstCCB->ulCallDuration = 0;                /* 呼叫时长，防止吊死用，每次心跳时更新 */

    pstCCB->ulStartTimeStamp = 0;              /* 起始时间戳 */
    pstCCB->ulRingTimeStamp = 0;               /* 振铃时间戳 */
    pstCCB->ulAnswerTimeStamp = 0;             /* 应答时间戳 */
    pstCCB->ulIVRFinishTimeStamp = 0;          /* IVR播放完成时间戳 */
    pstCCB->ulDTMFTimeStamp = 0;               /* (第一个)二次拨号时间戳 */
    pstCCB->ulBridgeTimeStamp = 0;             /* LEG桥接时间戳 */
    pstCCB->ulByeTimeStamp = 0;                /* 释放时间戳 */

    pstCCB->szCallerNum[0] = '\0';             /* 主叫号码 */
    pstCCB->szCalleeNum[0] = '\0';             /* 被叫号码 */
    pstCCB->szANINum[0] = '\0';                /* 被叫号码 */
    pstCCB->szDialNum[0] = '\0';               /* 用户拨号 */
    pstCCB->szSiteNum[0] = '\0';               /* 坐席号码 */
    pstCCB->szUUID[0] = '\0';                  /* Leg-A UUID */

    /* 业务类型 列表*/
    pstCCB->ucCurrentSrvInd = 0;               /* 当前空闲的业务类型索引 */
    for (i=0; i<SC_MAX_SRV_TYPE_PRE_LEG; i++)
    {
        pstCCB->aucServiceType[i] = U8_BUTT;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}

SC_CCB_ST *sc_ccb_get(U32 ulIndex)
{
    if (ulIndex >= SC_MAX_CCB_NUM)
    {
        return NULL;
    }

    return &g_pstTaskMngtInfo->pstCallCCBList[ulIndex];
}

static S32 sc_ccb_hash_find(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_CCB_HASH_NODE_ST *pstCCBHashNode = (SC_CCB_HASH_NODE_ST *)pNode;

    if (!pstCCBHashNode)
    {
        return DOS_FAIL;
    }

    if (dos_strncmp(pstCCBHashNode->szUUID, (S8 *)pSymName, sizeof(pstCCBHashNode->szUUID)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static U32 sc_ccb_hash_index_get(S8 *pszUUID)
{
    S8   szUUIDHead[16] = { 0 };
    U32  ulIndex;

    SC_TRACE_IN(pszUUID, 0, 0, 0);

    if (!pszUUID)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return U32_BUTT;
    }

    dos_strncpy(szUUIDHead, pszUUID, 8);
    szUUIDHead[8] = '\0';

    if (dos_atoulx(szUUIDHead, &ulIndex) < 0)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return U32_BUTT;
    }

    SC_TRACE_OUT();
    return (ulIndex % SC_MAX_CCB_HASH_NUM);
}

U32 sc_ccb_hash_tables_add(S8 *pszUUID, SC_CCB_ST *pstCCB)
{
    U32  ulHashIndex;
    SC_CCB_HASH_NODE_ST *pstCCBHashNode;

    SC_TRACE_IN(pszUUID, pstCCB, 0, 0);

    if (!pszUUID)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    ulHashIndex = sc_ccb_hash_index_get(pszUUID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }


    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCallHash);
    pstCCBHashNode = (SC_CCB_HASH_NODE_ST *)hash_find_node(g_pstTaskMngtInfo->pstCallCCBHash, ulHashIndex, pszUUID, sc_ccb_hash_find);
    if (pstCCBHashNode)
    {
        sc_logr_info("UUID %s has been added to hash list sometimes before.", pszUUID);
        if (DOS_ADDR_INVALID(pstCCBHashNode->pstCCB)
            && DOS_ADDR_VALID(pstCCB))
        {
            pstCCBHashNode->pstCCB = pstCCB;
        }
        pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallHash);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    pstCCBHashNode = (SC_CCB_HASH_NODE_ST *)dos_dmem_alloc(sizeof(SC_CCB_HASH_NODE_ST));
    if (!pstCCBHashNode)
    {
        DOS_ASSERT(0);

        sc_logr_warning("%s", "Alloc memory fail");
        pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallHash);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    HASH_Init_Node((HASH_NODE_S *)&pstCCBHashNode->stNode);
    pstCCBHashNode->pstCCB = NULL;
    pstCCBHashNode->szUUID[0] = '\0';
    sem_init(&pstCCBHashNode->semCCBSyn, 0, 0);

    if (DOS_ADDR_INVALID(pstCCBHashNode->pstCCB)
        && DOS_ADDR_VALID(pstCCB))
    {
        pstCCBHashNode->pstCCB = pstCCB;
    }

    dos_strncpy(pstCCBHashNode->szUUID, pszUUID, sizeof(pstCCBHashNode->szUUID));
    pstCCBHashNode->szUUID[sizeof(pstCCBHashNode->szUUID) - 1] = '\0';

    hash_add_node(g_pstTaskMngtInfo->pstCallCCBHash, (HASH_NODE_S *)pstCCBHashNode, ulHashIndex, NULL);

    sc_logr_warning("Add CCB with the UUID %s to the hash table.", pszUUID);

    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallHash);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_ccb_hash_tables_delete(S8 *pszUUID)
{
    U32  ulHashIndex;
    SC_CCB_HASH_NODE_ST *pstCCBHashNode;

    SC_TRACE_IN(pszUUID, 0, 0, 0);

    if (!pszUUID)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    ulHashIndex = sc_ccb_hash_index_get(pszUUID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCallHash);
    pstCCBHashNode = (SC_CCB_HASH_NODE_ST *)hash_find_node(g_pstTaskMngtInfo->pstCallCCBHash, ulHashIndex, pszUUID, sc_ccb_hash_find);
    if (!pstCCBHashNode)
    {
        DOS_ASSERT(0);

        sc_logr_info("Connot find the CCB with the UUID %s where delete the hash node.", pszUUID);
        pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallHash);
        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    hash_delete_node(g_pstTaskMngtInfo->pstCallCCBHash, (HASH_NODE_S *)pstCCBHashNode, ulHashIndex);
    pstCCBHashNode->pstCCB = NULL;
    pstCCBHashNode->szUUID[sizeof(pstCCBHashNode->szUUID) - 1] = '\0';

    dos_dmem_free(pstCCBHashNode);
    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallHash);

    return DOS_SUCC;
}

SC_CCB_ST *sc_ccb_hash_tables_find(S8 *pszUUID)
{
    U32  ulHashIndex;
    SC_CCB_HASH_NODE_ST *pstCCBHashNode = NULL;
    SC_CCB_ST           *pstCCB = NULL;

    SC_TRACE_IN(pszUUID, 0, 0, 0);

    if (!pszUUID)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return NULL;
    }

    ulHashIndex = sc_ccb_hash_index_get(pszUUID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return NULL;
    }

    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCallHash);
    pstCCBHashNode = (SC_CCB_HASH_NODE_ST *)hash_find_node(g_pstTaskMngtInfo->pstCallCCBHash, ulHashIndex, pszUUID, sc_ccb_hash_find);
    if (!pstCCBHashNode)
    {
        sc_logr_info("Connot find the CCB with the UUID %s.", pszUUID);
        pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallHash);

        SC_TRACE_OUT();
        return NULL;
    }
    pstCCB = pstCCBHashNode->pstCCB;
    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallHash);

    SC_TRACE_OUT();
    return pstCCBHashNode->pstCCB;
}

U32 sc_ccb_syn_wait(S8 *pszUUID)
{
    U32  ulHashIndex;
    SC_CCB_HASH_NODE_ST *pstCCBHashNode = NULL;
    SC_CCB_ST           *pstCCB = NULL;

    SC_TRACE_IN(pszUUID, 0, 0, 0);

    if (!pszUUID)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    ulHashIndex = sc_ccb_hash_index_get(pszUUID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCallHash);
    pstCCBHashNode = (SC_CCB_HASH_NODE_ST *)hash_find_node(g_pstTaskMngtInfo->pstCallCCBHash, ulHashIndex, pszUUID, sc_ccb_hash_find);
    if (!pstCCBHashNode)
    {
        sc_logr_info("Connot find the CCB with the UUID %s.", pszUUID);
        pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallHash);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    pstCCB = pstCCBHashNode->pstCCB;
    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallHash);

    sem_wait(&pstCCBHashNode->semCCBSyn);

    return DOS_SUCC;

}

U32 sc_ccb_syn_post(S8 *pszUUID)
{
    U32  ulHashIndex;
    SC_CCB_HASH_NODE_ST *pstCCBHashNode = NULL;
    SC_CCB_ST           *pstCCB = NULL;

    SC_TRACE_IN(pszUUID, 0, 0, 0);

    if (!pszUUID)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    ulHashIndex = sc_ccb_hash_index_get(pszUUID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexCallHash);
    pstCCBHashNode = (SC_CCB_HASH_NODE_ST *)hash_find_node(g_pstTaskMngtInfo->pstCallCCBHash, ulHashIndex, pszUUID, sc_ccb_hash_find);
    if (!pstCCBHashNode)
    {
        sc_logr_info("Connot find the CCB with the UUID %s.", pszUUID);
        pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallHash);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    pstCCB = pstCCBHashNode->pstCCB;
    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexCallHash);

    sem_post(&pstCCBHashNode->semCCBSyn);

    SC_TRACE_OUT();
    return DOS_SUCC;

}


BOOL sc_call_check_service(SC_CCB_ST *pstCCB, U32 ulService)
{
    U32 ulIndex;

    if (DOS_ADDR_INVALID(pstCCB) || ulService >= SC_SERV_BUTT)
    {
        return DOS_FALSE;
    }

    for (ulIndex=0; ulIndex<SC_MAX_SRV_TYPE_PRE_LEG; ulIndex++)
    {
        if (ulService == pstCCB->aucServiceType[ulIndex])
        {
            return DOS_TRUE;
        }
    }

    return DOS_FALSE;
}

U32 sc_call_owner(SC_CCB_ST *pstCCB, U16  usTaskID, U32 ulCustomID)
{
    SC_TRACE_IN((U64)pstCCB, usTaskID, ulCustomID, 0);

    if (!pstCCB)
    {
        SC_TRACE_OUT();
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pthread_mutex_lock(&pstCCB->mutexCCBLock);
    pstCCB->usTCBNo = usTaskID;
    pstCCB->ulCustomID = ulCustomID;
    pthread_mutex_unlock(&pstCCB->mutexCCBLock);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_call_set_trunk(SC_CCB_ST *pstCCB, U32 ulTrunkID)
{
    SC_TRACE_IN((U64)pstCCB, ulTrunkID, 0, 0);

    if (!pstCCB)
    {
        SC_TRACE_OUT();
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pthread_mutex_lock(&pstCCB->mutexCCBLock);
    pstCCB->ulTrunkID = ulTrunkID;
    pthread_mutex_unlock(&pstCCB->mutexCCBLock);

    SC_TRACE_OUT();
    return DOS_SUCC;

}


BOOL sc_tcb_get_valid(SC_TASK_CB_ST *pstTCB)
{
    BOOL bValid = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        return DOS_FALSE;
    }

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    bValid = pstTCB->ucValid;
    pthread_mutex_lock(&pstTCB->mutexTaskList);

    return bValid;
}

SC_TASK_CB_ST *sc_tcb_alloc()
{
    static U32 ulIndex = 0;
    U32 ulLastIndex;
    SC_TASK_CB_ST  *pstTCB = NULL;

    SC_TRACE_IN(ulIndex, 0, 0, 0);

    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexTCBList);

    ulLastIndex = ulIndex;

    for (; ulIndex<SC_MAX_TASK_NUM; ulIndex++)
    {
        if (sc_tcb_get_valid(&g_pstTaskMngtInfo->pstTaskList[ulIndex]))
        {
            pstTCB = &g_pstTaskMngtInfo->pstTaskList[ulIndex];
            break;
        }
    }

    if (!pstTCB)
    {
        for (ulIndex = 0; ulIndex < ulLastIndex; ulIndex++)
        {
            if (sc_tcb_get_valid(&g_pstTaskMngtInfo->pstTaskList[ulIndex]))
            {
                pstTCB = &g_pstTaskMngtInfo->pstTaskList[ulIndex];
                break;
            }
        }
    }

    if (pstTCB)
    {
        pthread_mutex_lock(&pstTCB->mutexTaskList);
        sc_tcb_init(pstTCB);
        pstTCB->ucValid = 1;
        pthread_mutex_unlock(&pstTCB->mutexTaskList);

        sc_task_trace(pstTCB, "Alloc TCB %d.", pstTCB->usTCBNo);

        pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexTCBList);
        SC_TRACE_OUT();
        return pstTCB;
    }

    DOS_ASSERT(0);

    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexTCBList);
    SC_TRACE_OUT();
    return NULL;

}

VOID sc_tcb_free(SC_TASK_CB_ST *pstTCB)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (!pstTCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return;
    }

    sc_task_trace(pstTCB, "Free TCB. %d", pstTCB->usTCBNo);

    pthread_mutex_lock(&g_pstTaskMngtInfo->mutexTCBList);

    sc_tcb_init(pstTCB);
    pstTCB->ucValid = 0;

    pthread_mutex_unlock(&g_pstTaskMngtInfo->mutexTCBList);
    SC_TRACE_OUT();
    return;
}

U32 sc_tcb_init(SC_TASK_CB_ST *pstTCB)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (!pstTCB)
    {
        SC_TRACE_OUT();
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstTCB->ucTaskStatus = SC_TASK_BUTT;
    pstTCB->ucPriority = SC_TASK_PRI_NORMAL;
    pstTCB->bTraceON = 0;
    pstTCB->bTraceCallON = 0;

    pstTCB->ulTaskID = U32_BUTT;
    pstTCB->ulCustomID = U32_BUTT;
    pstTCB->ulConcurrency = 0;
    pstTCB->usSiteCount = 0;
    pstTCB->ucAudioPlayCnt = 0;

    dos_list_init(&pstTCB->stCalleeNumQuery);    /* TODO: 释放所有节点 */
    pstTCB->pstCallerNumQuery = NULL;   /* TODO: 初始化所有节点 */
    pstTCB->pstSiteQuery = NULL;        /* TODO: 初始化所有节点*/
    pstTCB->szAudioFileLen[0] = '\0';
    dos_memzero(pstTCB->astPeriod, sizeof(pstTCB->astPeriod));

    /* 统计相关 */
    pstTCB->ulTotalCall = 0;
    pstTCB->ulCallFailed = 0;
    pstTCB->ulCallConnected = 0;

    SC_TRACE_OUT();
    return DOS_SUCC;
}

SC_TASK_CB_ST *sc_tcb_get_by_id(U32 ulTCBNo)
{
    SC_TRACE_IN((U32)ulTCBNo, 0, 0, 0);

    if (ulTCBNo >= SC_MAX_TASK_NUM)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return NULL;
    }

    SC_TRACE_OUT();
    return &g_pstTaskMngtInfo->pstTaskList[ulTCBNo];
}

SC_TASK_CB_ST *sc_tcb_find_by_taskid(U32 ulTaskID)
{
    U32 ulIndex;
    SC_TRACE_IN((U32)ulTaskID, 0, 0, 0);

    for (ulIndex=0; ulIndex<SC_MAX_TASK_NUM; ulIndex++)
    {
        if (g_pstTaskMngtInfo->pstTaskList[ulIndex].ucValid
            && g_pstTaskMngtInfo->pstTaskList[ulIndex].ulTaskID == ulTaskID)
        {
            SC_TRACE_OUT();
            return &g_pstTaskMngtInfo->pstTaskList[ulIndex];
        }
    }

    SC_TRACE_OUT();
    return NULL;
}


VOID sc_task_set_current_call_cnt(SC_TASK_CB_ST *pstTCB, U32 ulCurrentCall)
{
    SC_TRACE_IN((U64)pstTCB, ulCurrentCall, 0, 0);

    if (!pstTCB)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return;
    }

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ulConcurrency = ulCurrentCall;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);
}

U32 sc_task_get_current_call_cnt(SC_TASK_CB_ST *pstTCB)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (!pstTCB)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return U32_BUTT;
    }

    return pstTCB->ulConcurrency;
}

VOID sc_task_set_owner(SC_TASK_CB_ST *pstTCB, U32 ulTaskID, U32 ulCustomID)
{
    SC_TRACE_IN((U64)pstTCB, ulTaskID, ulCustomID, 0);

    if (!pstTCB)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return;
    }

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return;
    }

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ulTaskID = ulTaskID;
    pstTCB->ulCustomID = ulCustomID;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);
}

S32 sc_task_load_site_callback(VOID *pArg, S32 lArgc, S8 **pszValues, S8 **pszNames)
{
    return -1;
}

S32 sc_task_load_site(SC_TASK_CB_ST *pstTCB)
{
#if SC_USE_BUILDIN_DATA
    SC_SITE_QUERY_NODE_ST *pszSiteNode;
    U32 ulIndex;
    S32 lCNT;

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return -1;
    }

    if (!g_pstTaskMngtInfo)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return -1;
    }

    lCNT = 0;

    for (ulIndex=0; ulIndex<sizeof(g_pszSiteList)/sizeof(S8 *); ulIndex++)
    {
        pstTCB->pstSiteQuery[ulIndex].bValid = 1;
        pstTCB->pstSiteQuery[ulIndex].bTraceON = 1;
        pstTCB->pstSiteQuery[ulIndex].ulSiteID = ulIndex;
        pstTCB->pstSiteQuery[ulIndex].ulStatus = SC_SITE_ACCOM_ENABLE;
        dos_strncpy(pstTCB->pstSiteQuery[ulIndex].szUserID, g_pszSiteList[ulIndex], SC_TEL_NUMBER_LENGTH);
        pstTCB->pstSiteQuery[ulIndex].szUserID[SC_TEL_NUMBER_LENGTH - 1] = '\0';
        dos_strncpy(pstTCB->pstSiteQuery[ulIndex].szExtension, g_pszSiteList[ulIndex], SC_TEL_NUMBER_LENGTH);
        pstTCB->pstSiteQuery[ulIndex].szExtension[SC_TEL_NUMBER_LENGTH - 1] = '\0';
        dos_strncpy(pstTCB->pstSiteQuery[ulIndex].szEmpNo, g_pszSiteList[ulIndex], SC_EMP_NUMBER_LENGTH);

        lCNT++;
    }

    SC_TRACE_OUT();
    return lCNT;
#else
    return 0;
#endif
}

static S32 sc_task_load_caller_callback(VOID *pArg, S32 lArgc, S8 **pszValues, S8 **pszNames)
{
    SC_TASK_CB_ST            *pstTCB;
    S8                       *pszCustmerID = NULL, *pszCaller = NULL, *pszID = NULL;
    U32                      ulFirstInvalidNode = U32_BUTT, ulIndex = 0;
    U32                      ulCustomerID = 0, ulID = 0;
    BOOL                     blNeedAdd = DOS_TRUE;

    pstTCB = (SC_TASK_CB_ST *)pArg;
    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pszValues) || DOS_ADDR_INVALID(pszNames))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstTCB->pstCallerNumQuery))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (lArgc < 3)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    for (ulIndex=0; ulIndex<lArgc; ulIndex++)
    {
        if (dos_strcmp(pszNames[ulIndex], "id") == 0)
        {
            pszID = pszNames[ulIndex];
        }
        else if (dos_strcmp(pszNames[ulIndex], "customer") == 0)
        {
            pszCustmerID = pszNames[ulIndex];
        }
        else if (dos_strcmp(pszNames[ulIndex], "caller") == 0)
        {
            pszCaller = pszNames[ulIndex];
        }
    }

    if (DOS_ADDR_INVALID(pszID) || DOS_ADDR_INVALID(pszCustmerID) || DOS_ADDR_INVALID(pszCaller))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if ('\0' == pszID[0] || '\0' == pszCustmerID[0] || '\0' == pszCaller[0])
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (dos_atoul(pszID, &ulID) < 0)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (dos_atoul(pszCustmerID, &ulCustomerID) < 0)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (dos_strlen(pszCaller) >= SC_TEL_NUMBER_LENGTH)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 检测号码是否重复了，并且找到一个空闲的控制块 */
    for (ulIndex=0; ulIndex<SC_MAX_CALLER_NUM; ulIndex++)
    {
        if (!pstTCB->pstCallerNumQuery[ulIndex].bValid
            && U32_BUTT == ulFirstInvalidNode)
        {
            ulFirstInvalidNode = ulIndex;
        }

        if (pstTCB->pstCallerNumQuery[ulIndex].bValid
            && dos_strcmp(pstTCB->pstCallerNumQuery[ulIndex].szNumber, pszCaller) == 0)
        {
            blNeedAdd = DOS_FALSE;
        }
    }

    if (ulFirstInvalidNode < SC_MAX_CALLER_NUM)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstTCB->pstCallerNumQuery[ulIndex].bValid = 1;
    pstTCB->pstCallerNumQuery[ulIndex].ulIndexInDB = ulID;
    pstTCB->pstCallerNumQuery[ulIndex].bTraceON = pstTCB->bTraceCallON;
    dos_strncpy(pstTCB->pstCallerNumQuery[ulIndex].szNumber, pszCaller, SC_MAX_CALLER_NUM);
    pstTCB->pstCallerNumQuery[ulIndex].szNumber[SC_MAX_CALLER_NUM - 1] = '\0';

    SC_TRACE_OUT();
    return DOS_TRUE;
}

S32 sc_task_load_caller(SC_TASK_CB_ST *pstTCB)
{
#if SC_USE_BUILDIN_DATA
    U32 ulIndex;
    S32 lCnt;

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return -1;
    }

    if (!g_pstTaskMngtInfo)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return -1;
    }

    lCnt = 0;

    /* 检测号码是否重复了，并且找到一个空闲的控制块 */
    for (ulIndex=0; ulIndex<sizeof(g_pszCallerList)/sizeof(S8 *); ulIndex++)
    {
        pstTCB->pstCallerNumQuery[ulIndex].bValid   = 1;
        pstTCB->pstCallerNumQuery[ulIndex].bTraceON = 1;
        pstTCB->pstCallerNumQuery[ulIndex].ulIndexInDB = ulIndex;
        dos_strncpy(pstTCB->pstCallerNumQuery[ulIndex].szNumber, g_pszCallerList[ulIndex], SC_TEL_NUMBER_LENGTH);
        pstTCB->pstCallerNumQuery[ulIndex].szNumber[SC_TEL_NUMBER_LENGTH - 1] = '\0';

        lCnt++;
    }

    SC_TRACE_OUT();
    return lCnt;
#else
    S8 szSqlQuery[1024]= { 0 };
    U32 ulLength, ulResult;


    ulLength = dos_snprintf(szSqlQuery
                , sizeof(szSqlQuery)
                , "SELECT tbl_caller.id as id, tbl_caller.customer_id as customer, tbl_caller.cid as caller from tbl_caller WHERE tbl_caller.customer_id=%d"
                , pstTCB->ulCustomID);

    ulResult = db_query(g_pstTaskMngtInfo->pstDBHandle
                            , szSqlQuery
                            , sc_task_load_caller_callback
                            , pstTCB
                            , NULL);

    if (ulResult != DOS_SUCC)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
#endif
}

static S32 sc_task_load_callee_callback(VOID *pArg, S32 lArgc, S8 **pszValues, S8 **pszNames)
{
    SC_TASK_CB_ST *pstTCB = NULL;
    SC_TEL_NUM_QUERY_NODE_ST *pstCallee;
    U32 ulIndex;
    S8 *pszIndex = NULL;
    S8 *pszNum = NULL;

    if (DOS_ADDR_INVALID(pArg)
        || DOS_ADDR_INVALID(pszValues)
        || DOS_ADDR_INVALID(pszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstTCB = (SC_TASK_CB_ST *)pArg;
    pszIndex = pszValues[0];
    pszNum = pszValues[1];

    if (DOS_ADDR_INVALID(pszIndex)
        || DOS_ADDR_INVALID(pszNum)
        || dos_atoul(pszIndex, &ulIndex) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstCallee = (SC_TEL_NUM_QUERY_NODE_ST *)dos_dmem_alloc(sizeof(SC_TEL_NUM_QUERY_NODE_ST));
    if (DOS_ADDR_INVALID(pstCallee))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    dos_list_node_init(&pstCallee->stLink);
    pstCallee->ulIndex = ulIndex;
    pstCallee->ucTraceON = 0;
    pstCallee->ucCalleeType = SC_CALL_NUM_TYPE_NORMOAL;
    dos_strncpy(pstCallee->szNumber, pstCallee->szNumber, sizeof(pstCallee->szNumber));
    pstCallee->szNumber[sizeof(pstCallee->szNumber) - 1] = '\0';

    pstTCB->ulLastCalleeIndex++;
    dos_list_add_tail(&pstTCB->stCalleeNumQuery, (struct list_s *)pstCallee);

    return 0;
}

S32 sc_task_load_callee(SC_TASK_CB_ST *pstTCB)
{
#if SC_USE_BUILDIN_DATA
    SC_TEL_NUM_QUERY_NODE_ST *pstCalledNode;
    U32 ulIndex;
    S32 lCnt;

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return -1;
    }

    lCnt = 0;

    for (ulIndex=0; ulIndex<sizeof(g_pszCalledList)/sizeof(S8 *); ulIndex++)
    {
        pstCalledNode = (SC_TEL_NUM_QUERY_NODE_ST *)dos_dmem_alloc(sizeof(SC_TEL_NUM_QUERY_NODE_ST));
        if (!pstCalledNode)
        {
            break;
        }

        pstCalledNode->ucTraceON = 1;
        pstCalledNode->ucCalleeType = SC_CALL_NUM_TYPE_NORMOAL;
        dos_strncpy(pstCalledNode->szNumber, g_pszCalledList[ulIndex], SC_TEL_NUMBER_LENGTH);
        pstCalledNode->szNumber[SC_TEL_NUMBER_LENGTH - 1] = '\0';

        dos_list_add_head(&pstTCB->stCalleeNumQuery, &pstCalledNode->stLink);

        lCnt++;
    }

    SC_TRACE_OUT();
    return lCnt;
#else
    S8 szSQL[512] = { 0, };

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return -1;
    }


    dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT "
                      "  a.id, a.regex_number "
                      "FROM "
                      "  tbl_callee a "
                      "LEFT JOIN "
                      "    (SELECT "
                      "      c.id as id "
                      "    FROM "
                      "      tbl_calleefile c "
                      "    LEFT JOIN "
                      "      tbl_calltask d "
                      "    ON "
                      "      c.id = d.callee_id AND d.id = 0) b "
                      "ON "
                      "  a.calleefile_id = b.id AND a.`status` <> \"done\"; "
                      "LIMIT %d, 1000;"
                    , pstTCB->usTCBNo
                    , pstTCB->ulLastCalleeIndex);

    if (DB_ERR_SUCC != db_query(g_pstTaskMngtInfo->pstDBHandle, szSQL, sc_task_load_callee_callback, (VOID *)pstTCB, NULL))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
#endif
}

static S32 sc_task_load_period_cb(VOID *pArg, S32 lColumnCount, S8 **aszValues, S8 **aszNames)
{
    S8 *pszStarTime = NULL, *pszEndTime = NULL;
    S32 lStartHour, lStartMin, lStartSecond;
    S32 lEndHour, lEndMin, lEndSecond;
    SC_TASK_CB_ST *pstTCB = NULL;
    U32 ulIndex;

    if (DOS_ADDR_INVALID(pArg)
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstTCB = (SC_TASK_CB_ST *)pArg;
    pszStarTime = aszValues[0];
    pszEndTime = aszValues[1];

    if (DOS_ADDR_INVALID(pszStarTime)
        || DOS_ADDR_INVALID(pszEndTime))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (dos_sscanf(pszStarTime, "%d:%d:%d", &lStartHour, &lStartMin, &lStartSecond) != 3
        || dos_sscanf(pszEndTime, "%d:%d:%d", &lEndHour, &lEndMin, &lEndSecond) != 3)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (lStartHour > 23 || lEndHour > 23
        || lStartMin > 59 || lEndMin > 59
        || lStartSecond > 59 || lEndSecond > 59)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    for (ulIndex=0; ulIndex<SC_MAX_PERIOD_NUM; ulIndex++)
    {
        if (!pstTCB->astPeriod[0].ucValid)
        {
            break;
        }
    }

    if (SC_MAX_PERIOD_NUM <= ulIndex)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstTCB->astPeriod[ulIndex].ucValid = DOS_TRUE;
    pstTCB->astPeriod[ulIndex].ucWeekMask = 0xFF;

    pstTCB->astPeriod[ulIndex].ucHourBegin = (U8)lStartHour;
    pstTCB->astPeriod[ulIndex].ucMinuteBegin = (U8)lStartMin;
    pstTCB->astPeriod[ulIndex].ucSecondBegin = (U8)lStartSecond;

    pstTCB->astPeriod[ulIndex].ucHourEnd = (U8)lEndHour;
    pstTCB->astPeriod[ulIndex].ucMinuteEnd = (U8)lEndMin;
    pstTCB->astPeriod[ulIndex].ucSecondEnd= (U8)lEndSecond;

    return DOS_SUCC;
}

U32 sc_task_load_period(SC_TASK_CB_ST *pstTCB)
{
#if SC_USE_BUILDIN_DATA

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return -1;
    }

    pstTCB->astPeriod[0].ucValid = 1;

    pstTCB->astPeriod[0].ucWeekMask = 0xFF;

    pstTCB->astPeriod[0].ucHourBegin = 0;
    pstTCB->astPeriod[0].ucMinuteBegin = 0;

    pstTCB->astPeriod[0].ucHourEnd = 23;
    pstTCB->astPeriod[0].ucMinuteEnd = 59;

    SC_TRACE_OUT();
    return 1;

#else
    S8 szSQL[128] = { 0, };

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_snprintf(szSQL, sizeof(szSQL), "SELECT start_time, end_time from tbl_calltask WHERE id=%d;", pstTCB->usTCBNo);

    if (db_query(g_pstTaskMngtInfo->pstDBHandle, szSQL, sc_task_load_period_cb, (VOID *)pstTCB, NULL) != DB_ERR_SUCC)
    {
        sc_logr_debug("Load task time period for task %d fail;", pstTCB->usTCBNo);

        return DOS_FAIL;
    }

    return DOS_SUCC;
#endif
}

U32 sc_task_update_stat(SC_TASK_CB_ST *pstTCB)
{
    return 0;
}

U32 sc_task_load_audio(SC_TASK_CB_ST *pstTCB)
{
#if SC_USE_BUILDIN_DATA
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return -1;
    }

    pstTCB->ucAudioPlayCnt = 3;
    dos_strncpy(pstTCB->szAudioFileLen, "/usr/local/freeswitch/sounds/mmt1_speex_8k.wav", SC_MAX_AUDIO_FILENAME_LEN);
    pstTCB->szAudioFileLen[SC_MAX_AUDIO_FILENAME_LEN - 1] = '\0';

    SC_TRACE_OUT();
    return 1;
#else
    return 0;
#endif
}

U32 sc_task_check_can_call_by_time(SC_TASK_CB_ST *pstTCB)
{
    time_t     now;
    struct tm  *timenow;
    U32 ulWeek, ulHour, ulMinute, ulIndex;

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (!pstTCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FALSE;
    }

    time(&now);
    timenow = localtime(&now);

    ulWeek = timenow->tm_wday;
    ulHour = timenow->tm_hour;
    ulMinute = timenow->tm_min;

    for (ulIndex=0; ulIndex<SC_MAX_PERIOD_NUM; ulIndex++)
    {
        if (!pstTCB->astPeriod[ulIndex].ucValid)
        {
            continue;
        }

        if (!((pstTCB->astPeriod[ulIndex].ucWeekMask >> ulWeek) & 0x01))
        {
            continue;
        }

        if ((ulHour < pstTCB->astPeriod[ulIndex].ucHourBegin
            || ulHour > pstTCB->astPeriod[ulIndex].ucHourEnd))
        {
            continue;
        }

        if (ulMinute < pstTCB->astPeriod[ulIndex].ucMinuteBegin
            || ulMinute > pstTCB->astPeriod[ulIndex].ucMinuteEnd)
        {
            continue;
        }

        SC_TRACE_OUT();
        return DOS_TRUE;
    }

    DOS_ASSERT(0);
    SC_TRACE_OUT();
    return DOS_FALSE;
}

U32 sc_task_check_can_call_by_status(SC_TASK_CB_ST *pstTCB)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (!pstTCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FALSE;
    }

    if (pstTCB->ulConcurrency >= (pstTCB->usSiteCount * SC_MAX_CALL_MULTIPLE))
    {
        SC_TRACE_OUT();
        return DOS_FALSE;
    }

    SC_TRACE_OUT();
    return DOS_TRUE;
}

U32 sc_task_get_call_interval(SC_TASK_CB_ST *pstTCB)
{
    U32 ulPercentage;
    U32 ulInterval;

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (!pstTCB)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return 1000;
    }

    if (pstTCB->ulConcurrency)
    {
        ulPercentage = ((pstTCB->usSiteCount * SC_MAX_CALL_MULTIPLE) * 100) / pstTCB->ulConcurrency;
    }
    else
    {
        ulPercentage = 0;
    }

    if (ulPercentage < 40)
    {
        ulInterval = 300;
    }
    else if (ulPercentage < 80)
    {
        ulInterval = 500;
    }
    else
    {
        ulInterval = 1000;
    }

    SC_TRACE_OUT();
    return ulInterval;
}

S8 *sc_task_get_audio_file(U32 ulTCBNo)
{
    SC_TRACE_IN(ulTCBNo, 0, 0, 0);
    if (ulTCBNo > SC_MAX_TASK_NUM)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return NULL;
    }

    if (!g_pstTaskMngtInfo->pstTaskList[ulTCBNo].ucValid)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return NULL;
    }

    return g_pstTaskMngtInfo->pstTaskList[ulTCBNo].szAudioFileLen;
}


U32 sc_task_audio_playcnt(U32 ulTCBNo)
{
    SC_TRACE_IN(ulTCBNo, 0, 0, 0);
    if (ulTCBNo > SC_MAX_TASK_NUM)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return 0;
    }

    if (!g_pstTaskMngtInfo->pstTaskList[ulTCBNo].ucValid)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return 0;
    }

    return g_pstTaskMngtInfo->pstTaskList[ulTCBNo].ucAudioPlayCnt;
}

U32 sc_task_get_timeout_for_noanswer(U32 ulTCBNo)
{
    return 10;
}


U32 sc_task_callee_set_recall(SC_TASK_CB_ST *pstTCB, U32 ulIndex)
{
    S8 szSQL[128];

    dos_snprintf(szSQL, sizeof(szSQL), "UPDATE tbl_callee SET tbl_callee.`status`=\"waiting\" WHERE tbl_callee.id = 0;");

    if (DB_ERR_SUCC != db_query(g_pstTaskMngtInfo->pstDBHandle, szSQL, NULL, NULL, NULL))
    {
        DOS_ASSERT(0);

        sc_logr_notice("Set callee to waiting status FAIL. index:%d", ulIndex);

        return DOS_FALSE;
    }

    return DOS_TRUE;
}

SC_SYS_STATUS_EN sc_check_sys_stat()
{
    return SC_SYS_NORMAL;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


