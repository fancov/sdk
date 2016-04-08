/**
 * @file : sc_lib.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 各种辅助函数集合
 *
 * @date: 2016年1月9日
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include <math.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_pub.h"
#include "bs_pub.h"
#include "sc_hint.h"
#include "sc_res.h"
#include "sc_publish.h"
#include "sc_http_api.h"

/** 管理background job的HASH表 */
extern HASH_TABLE_S          *g_pstBGJobHash;

/** 管理background job的HASH表锁 */
extern pthread_mutex_t        g_mutexBGJobHash;

/** LEG业务控制块列表 */
extern SC_LEG_CB             *g_pstLegCB;
/** LEG业务控制块锁 */
extern pthread_mutex_t       g_mutexLegCB;

/** 呼叫LEG控制块 */
extern HASH_TABLE_S         *g_pstLCBHash;
/** 保护呼叫控制块使用的互斥量 */
extern pthread_mutex_t      g_mutexLCBHash;

/** 业务控制块指针 */
extern SC_SRV_CB       *g_pstSCBList;

/** 业务控制块锁 */
extern pthread_mutex_t g_mutexSCBList;

/** 业务子层上报时间消息队列 */
DLL_S                 g_stCommandQueue;

/** 业务子层上报时间消息队列锁 */
extern pthread_mutex_t       g_mutexCommandQueue;

/** 业务子层上报时间消息队列条件变量 */
extern pthread_cond_t        g_condCommandQueue;

/** 业务子层上报时间消息队列 */
extern DLL_S           g_stEventQueue;

/** 业务子层上报时间消息队列锁 */
extern pthread_mutex_t g_mutexEventQueue;

/** 业务子层上报时间消息队列条件变量 */
extern pthread_cond_t  g_condEventQueue;

/** 任务列表 refer to struct tagTaskCB*/
extern SC_TASK_CB           *g_pstTaskList;
extern pthread_mutex_t      g_mutexTaskList;

extern U32          g_ulCPS;
extern U32          g_ulMaxConcurrency4Task;

U32 sc_send_sip_update_req(U32 ulID, U32 ulAction)
{
    S8 szURL[256]      = { 0, };
    S8 szData[512]     = { 0, };
    SC_PUB_FS_DATA_ST *pstData;
    S32 ulPAPIPort = -1;

    pstData = dos_dmem_alloc(sizeof(SC_PUB_FS_DATA_ST));
    if (DOS_ADDR_INVALID(pstData))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulPAPIPort = config_hb_get_papi_port();
    if (ulPAPIPort <= 0)
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost/index.php/papi");
    }
    else
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost:%d/index.php/papi", ulPAPIPort);
    }

    /* 格式中引号前面需要添加"\",提供给push stream做转义用 */
    dos_snprintf(szData, sizeof(szData), "data={\"type\":\"%u\", \"data\":{\"id\":\"%u\", \"action\":\"%s\"}}"
                    , SC_PUB_TYPE_SIP_XMl
                    , ulID
                    , SC_API_CMD_ACTION_SIP_DELETE == ulAction ? "delete" : "add");

    pstData->ulID = ulID;
    pstData->ulAction = ulAction;

    if (sc_pub_send_msg(szURL, szData, SC_PUB_TYPE_SIP_XMl, pstData) == DOS_SUCC)
    {
        return DOS_SUCC;
    }
    else
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstData);
        pstData = NULL;
        return DOS_FAIL;
    }
}

U32 sc_send_gateway_update_req(U32 ulID, U32 ulAction)
{
    S8 szURL[256]      = { 0, };
    S8 szData[512]     = { 0, };
    SC_PUB_FS_DATA_ST *pstData;
    S32 ulPAPIPort = -1;

    pstData = dos_dmem_alloc(sizeof(SC_PUB_FS_DATA_ST));
    if (DOS_ADDR_INVALID(pstData))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulPAPIPort = config_hb_get_papi_port();
    if (ulPAPIPort <= 0)
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost/index.php/papi");
    }
    else
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost:%d/index.php/papi", ulPAPIPort);
    }

    /* 格式中引号前面需要添加"\",提供给push stream做转义用 */
    dos_snprintf(szData, sizeof(szData), "data={\"type\":\"%u\", \"data\":{\"id\":\"%u\", \"action\":\"%s\"}}"
                    , SC_PUB_TYPE_GATEWAY
                    , ulID
                    , SC_API_CMD_ACTION_GATEWAY_DELETE == ulAction ? "delete" : "add");

    pstData->ulID = ulID;
    pstData->ulAction = ulAction;

    if (sc_pub_send_msg(szURL, szData, SC_PUB_TYPE_GATEWAY, pstData) == DOS_SUCC)
    {
        return DOS_SUCC;
    }
    else
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstData);
        pstData = NULL;
        return DOS_FAIL;
    }

}


U32 sc_get_record_file_path(S8 *pszBuff, U32 ulMaxLen, U32 ulCustomerID, S8 *pszJobNum, S8 *pszCaller, S8 *pszCallee)
{
    struct tm            *pstTime;
    struct tm            stTime;
    time_t               timep;

    timep = time(NULL);
    pstTime = dos_get_localtime_struct(timep, &stTime);

    if (DOS_ADDR_INVALID( pszBuff)
        || ulMaxLen <= 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_snprintf(pszBuff, ulMaxLen
            , "%s/%u/%04d%02d%02d"
            , SC_RECORD_FILE_PATH
            , ulCustomerID
            , pstTime->tm_year + 1900
            , pstTime->tm_mon + 1
            , pstTime->tm_mday);
    if (access(pszBuff, F_OK) < 0)
    {
        mkdir(pszBuff, S_IXOTH|S_IWOTH|S_IROTH|S_IRUSR|S_IWUSR|S_IXUSR);
        chown(pszBuff, SC_NOBODY_UID, SC_NOBODY_GID);
        chmod(pszBuff, S_IXOTH|S_IWOTH|S_IROTH|S_IRUSR|S_IWUSR|S_IXUSR);
    }

    dos_snprintf(pszBuff, ulMaxLen
            , "%u/%04d%02d%02d/VR-%s-%04d%02d%02d%02d%02d%02d-%s-%s"
            , ulCustomerID
            , pstTime->tm_year + 1900
            , pstTime->tm_mon + 1
            , pstTime->tm_mday
            , pszJobNum
            , pstTime->tm_year + 1900
            , pstTime->tm_mon + 1
            , pstTime->tm_mday
            , pstTime->tm_hour
            , pstTime->tm_min
            , pstTime->tm_sec
            , pszCallee
            , pszCaller);

    return DOS_SUCC;
}

/**
 * 根据 @a pszUUID 计算HASH值
 *
 * @param U32 ulLegNo LEG控制块编号
 * @param S8 *pszUUID BG-JOB的UUID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note 参与计算的UUID长度为18
 */
static U32 sc_uuid_hash_func(S8 *pszUUID, U32 ulHashSize)
{
    U32  ulIndex;
    U32  ulLoop;

    if (DOS_ADDR_INVALID(pszUUID) || '\0' == pszUUID[0] || ulHashSize == 0)
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    for (ulLoop=0, ulIndex=0; ulLoop<SC_UUID_HASH_LEN; ulLoop++)
    {
        ulIndex += pszUUID[ulLoop];
    }

    return (ulIndex % ulHashSize);

}

S32 sc_bgjob_hash_find_func(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_BG_JOB_HASH_NODE_ST *pstNode;

    if (DOS_ADDR_INVALID(pSymName) || DOS_ADDR_INVALID(pNode))
    {
        return DOS_FAIL;
    }

    pstNode = (SC_BG_JOB_HASH_NODE_ST *)pNode;

    if (dos_strncmp(pstNode->szUUID, (S8 *)pSymName, sizeof(pstNode->szUUID)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 记录BG-JOB的UUID
 *
 * @param U32 ulLegNo LEG控制块编号
 * @param S8 *pszUUID BG-JOB的UUID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_bgjob_hash_add(U32 ulLegNo, S8 *pszUUID)
{
    U32  ulHashIndex;
    SC_BG_JOB_HASH_NODE_ST *pstHashNode;

    if (ulLegNo >= SC_LEG_CB_SIZE)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pszUUID) || '\0' == pszUUID[0])
    {
        return DOS_FAIL;
    }

    ulHashIndex = sc_uuid_hash_func(pszUUID, SC_BG_JOB_HASH_SIZE);
    if (U32_BUTT == ulHashIndex)
    {
        return DOS_FAIL;
    }

    pstHashNode = dos_dmem_alloc(sizeof(SC_BG_JOB_HASH_NODE_ST));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        sc_log(LOG_LEVEL_ERROR, "Alloc memory fail.");
        return DOS_FAIL;
    }

    pstHashNode->ulLegNo = ulLegNo;
    dos_snprintf(pstHashNode->szUUID, sizeof(pstHashNode->szUUID), pszUUID);

    pthread_mutex_lock(&g_mutexBGJobHash);
    hash_add_node(g_pstBGJobHash, (HASH_NODE_S *)pstHashNode, ulHashIndex, NULL);
    pthread_mutex_unlock(&g_mutexBGJobHash);

    sc_log(LOG_LEVEL_DEBUG, "Add BG-JOB %s(%u) to hash table.", pszUUID, ulLegNo);

    return DOS_SUCC;
}

/**
 * 删除BG-JOB的UUID
 *
 * @param S8 *pszUUID BG-JOB的UUID
 *
 * @return 成功返回LEG控制块编号，否则返回U32_BUTT
 */
U32 sc_bgjob_hash_delete(S8 *pszUUID)
{
    U32  ulHashIndex;
    U32  ulLegNo = 0;
    SC_BG_JOB_HASH_NODE_ST *pstHashNode;

    if (DOS_ADDR_INVALID(pszUUID) || '\0' == pszUUID[0])
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_uuid_hash_func(pszUUID, SC_BG_JOB_HASH_SIZE);
    if (U32_BUTT == ulHashIndex)
    {
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexBGJobHash);
    pstHashNode = (SC_BG_JOB_HASH_NODE_ST *)hash_find_node(g_pstBGJobHash, ulHashIndex, pszUUID, sc_bgjob_hash_find_func);
    if (DOS_ADDR_VALID(pstHashNode))
    {
        ulLegNo = pstHashNode->ulLegNo;
        hash_delete_node(g_pstBGJobHash, (HASH_NODE_S *)pstHashNode, ulHashIndex);

        dos_dmem_free(pstHashNode);
    }
    pthread_mutex_unlock(&g_mutexBGJobHash);



    sc_log(LOG_LEVEL_DEBUG, "Del BG-JOB %s(%u) to hash table.", pszUUID, ulLegNo);

    return ulLegNo;
}

/**
 * 查找BG-JOB的UUID
 *
 * @param S8 *pszUUID BG-JOB的UUID
 *
 * @return 成功返回LEG控制块编号，否则返回U32_BUTT
 */
U32 sc_bgjob_hash_find(S8 *pszUUID)
{
    U32  ulHashIndex;
    U32  ulLegNo = U32_BUTT;
    SC_BG_JOB_HASH_NODE_ST *pstHashNode;

    if (DOS_ADDR_INVALID(pszUUID) || '\0' == pszUUID[0])
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    ulHashIndex = sc_uuid_hash_func(pszUUID, SC_BG_JOB_HASH_SIZE);
    if (U32_BUTT == ulHashIndex)
    {
        return U32_BUTT;
    }

    pthread_mutex_lock(&g_mutexBGJobHash);
    pstHashNode = (SC_BG_JOB_HASH_NODE_ST *)hash_find_node(g_pstBGJobHash, ulHashIndex, pszUUID, sc_bgjob_hash_find_func);
    if (DOS_ADDR_VALID(pstHashNode))
    {
        ulLegNo = pstHashNode->ulLegNo;
    }
    pthread_mutex_unlock(&g_mutexBGJobHash);

    return ulLegNo;

}

/**
 * 初始化 @a pstCall 所指向的基础呼叫业务单元控制块
 *
 * @param SC_SU_CALL_ST *pstCall 基础呼叫业务单元控制块
 *
 * @return VOID
 */
VOID sc_lcb_call_init(SC_SU_CALL_ST *pstCall)
{
    if (DOS_ADDR_INVALID(pstCall))
    {
        DOS_ASSERT(0);
        return;
    }
    pstCall->bValid = DOS_FALSE;
    pstCall->bTrace = DOS_FALSE;
    pstCall->bEarlyMedia =  DOS_FALSE;

    pstCall->ucStatus = SC_LEG_INIT;
    pstCall->ucHoldCnt = 0;
    pstCall->ucPeerType = SC_LEG_PEER_BUTT;
    pstCall->ucLocalMode = SC_LEG_LOCAL_BUTT;

    pstCall->ulHoldTotalTime = 0;
    pstCall->ulTrunkID = 0;
    pstCall->ulTrunkCount = 0;
    pstCall->ulCodecCnt = 0;
    pstCall->ulCause = 0;

    pstCall->stTimeInfo.ulStartTime = 0;
    pstCall->stTimeInfo.ulRingTime = 0;
    pstCall->stTimeInfo.ulAnswerTime = 0;
    pstCall->stTimeInfo.ulBridgeTime = 0;
    pstCall->stTimeInfo.ulByeTime = 0;
    pstCall->stTimeInfo.ulIVREndTime = 0;
    pstCall->stTimeInfo.ulDTMFStartTime = 0;
    pstCall->stTimeInfo.ulDTMFLastTime = 0;
    pstCall->stTimeInfo.ulRecordStartTime = 0;
    pstCall->stTimeInfo.ulRecordStopTime = 0;
    pstCall->stTimeInfo.ulTransferStartTime = 0;
    pstCall->stTimeInfo.ulTransferEndTime = 0;
    pstCall->stNumInfo.szOriginalCallee[0] = '\0';
    pstCall->stNumInfo.szOriginalCalling[0] = '\0';
    pstCall->stNumInfo.szRealCallee[0] = '\0';
    pstCall->stNumInfo.szRealCalling[0] = '\0';
    pstCall->stNumInfo.szCallee[0] = '\0';
    pstCall->stNumInfo.szCalling[0] = '\0';
    pstCall->stNumInfo.szANI[0] = '\0';
    pstCall->stNumInfo.szCID[0] = '\0';
    pstCall->stNumInfo.szDial[0] = '\0';
}

/**
 * 初始化 @a pstRecord 所指向的录音业务单元控制块
 *
 * @param SC_SU_RECORD_ST *pstRecord 录音业务单元控制块
 *
 * @return VOID
 */
VOID sc_lcb_record_init(SC_SU_RECORD_ST *pstRecord)
{
    if (DOS_ADDR_INVALID(pstRecord))
    {
        DOS_ASSERT(0);
        return;
    }

    pstRecord->bValid = DOS_FALSE;
    pstRecord->bTrace = DOS_FALSE;
    pstRecord->usStatus = SC_SU_RECORD_INIT;
    pstRecord->szRecordFilename[0] = '\0';
}

/**
 * 初始化 @a pstPlayback 所指向的放音业务单元控制块
 *
 * @param SC_SU_PLAYBACK_ST *pstPlayback 放音业务控制块
 *
 * @return VOID
 */
VOID sc_lcb_playback_init(SC_SU_PLAYBACK_ST *pstPlayback)
{
    if (DOS_ADDR_INVALID(pstPlayback))
    {
        DOS_ASSERT(0);
        return;
    }

    pstPlayback->bValid = DOS_FALSE;
    pstPlayback->bTrace = DOS_FALSE;
    pstPlayback->usStatus = SC_SU_PLAYBACK_INIT;
    pstPlayback->ulTotal = 0;
    pstPlayback->ulCurretnIndex = 0;
}

/**
 * 初始化 @a pstBridge 所指向的桥接业务单元控制块
 *
 * @param SC_SU_BRIDGE_ST *pstBridge 桥接业务控制块
 *
 * @return VOID
 */
VOID sc_lcb_bridge_init(SC_SU_BRIDGE_ST *pstBridge)
{
    if (DOS_ADDR_INVALID(pstBridge))
    {
        DOS_ASSERT(0);
        return;
    }

    pstBridge->bValid = DOS_FALSE;
    pstBridge->bTrace = DOS_FALSE;
    pstBridge->usStatus = SC_SU_BRIDGE_INIT;
    pstBridge->ulOtherLEGNo = U32_BUTT;
}

/**
 * 初始化 @a pstHold 所指向的呼叫保持业务单元控制块
 *
 * @param SC_SU_HOLD_ST *pstHold 呼叫保持业务控制块
 *
 * @return VOID
 */
VOID sc_lcb_hold_init(SC_SU_HOLD_ST *pstHold)
{
    if (DOS_ADDR_INVALID(pstHold))
    {
        DOS_ASSERT(0);
        return;
    }

    pstHold->bValid = DOS_FALSE;
    pstHold->bTrace = DOS_FALSE;
    pstHold->usStatus = SC_SU_HOLD_INIT;
    pstHold->usMode = 0;
}

/**
 * 初始化 @a pstMux 所指向的混音业务单元控制块
 *
 * @param SC_SU_MUX_ST *pstMux 混音业务控制块
 *
 * @return VOID
 */
VOID sc_lcb_mux_init(SC_SU_MUX_ST *pstMux)
{
    if (DOS_ADDR_INVALID(pstMux))
    {
        DOS_ASSERT(0);
        return;
    }

    pstMux->bValid = DOS_FALSE;
    pstMux->bTrace = DOS_FALSE;
    pstMux->usStatus = SC_SU_MUX_INIT;
    pstMux->usMode = 0;
    pstMux->ulOtherLegNo = U32_BUTT;
}

/**
 * 初始化 @a pstMCX 所指向的修改连接业务单元控制块
 *
 * @param SC_SU_MCX_ST *pstMCX 混音业务控制块
 *
 * @return VOID
 */
VOID sc_lcb_mcx_init(SC_SU_MCX_ST *pstMCX)
{
    if (DOS_ADDR_INVALID(pstMCX))
    {
        DOS_ASSERT(0);
        return;
    }

    pstMCX->bValid = DOS_FALSE;
    pstMCX->bTrace = DOS_FALSE;
    pstMCX->usStatus = SC_SU_MCX_INIT;
}

/**
 * 初始化 @a pstIVR 所指向的IVR业务单元控制块
 *
 * @param SC_SU_IVR_ST *pstIVR IVR业务控制块
 *
 * @return VOID
 */
VOID sc_lcb_ivr_init(SC_SU_IVR_ST *pstIVR)
{
    if (DOS_ADDR_INVALID(pstIVR))
    {
        DOS_ASSERT(0);
        return;
    }

    pstIVR->bValid = DOS_FALSE;
    pstIVR->bTrace = DOS_FALSE;
    pstIVR->usStatus = SC_SU_IVR_INIT;
}

/**
 * 初始化 @a pstLCB 指向的LEG控制块
 *
 * @param SC_LEG_CB *pstLCB LEG控制块指针
 *
 * @return NULL
 */
VOID sc_lcb_init(SC_LEG_CB *pstLCB)
{
    U32  ulIndex;
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return;
    }

    pstLCB->bValid = DOS_FALSE;
    pstLCB->bTrace = DOS_FALSE;
    pstLCB->szUUID[0] = '\0';
    pstLCB->ulAllocTime = 0;

    for (ulIndex=0; ulIndex<SC_MAX_CODEC_NUM; ulIndex++)
    {
        pstLCB->aucCodecList[ulIndex] = U8_BUTT;
    }
    pstLCB->ucCodec = U8_BUTT;
    pstLCB->ucPtime = 0;
    pstLCB->bValid = DOS_FALSE;
    pstLCB->bTrace = DOS_FALSE;
    pstLCB->ulSCBNo = U32_BUTT;
    pstLCB->ulIndSCBNo = U32_BUTT;
    pstLCB->ulOtherSCBNo = U32_BUTT;
    sem_init(&pstLCB->stEventSem, 0, 1);

    sc_lcb_call_init(&pstLCB->stCall);
    sc_lcb_record_init(&pstLCB->stRecord);
    sc_lcb_playback_init(&pstLCB->stPlayback);
    sc_lcb_bridge_init(&pstLCB->stBridge);
    sc_lcb_mcx_init(&pstLCB->stMcx);
    sc_lcb_mux_init(&pstLCB->stMux);
    sc_lcb_hold_init(&pstLCB->stHold);
    sc_lcb_ivr_init(&pstLCB->stIVR);
}

/**
 * 申请空闲的LEG控制块
 *
 * @return 成功返回LEG控制块指针，否则返回NULL
 */
SC_LEG_CB *sc_lcb_alloc()
{
    static U32 ulIndex = 0;
    U32 ulLastIndex = 0;
    SC_LEG_CB *pstLCB = NULL;

    pthread_mutex_lock(&g_mutexLegCB);
    ulLastIndex = ulIndex;
    for (; ulIndex<SC_LEG_CB_SIZE; ulIndex++)
    {
        if (g_pstLegCB[ulIndex].bValid)
        {
            continue;
        }

        pstLCB = &g_pstLegCB[ulIndex];
        break;
    }

    if (DOS_ADDR_INVALID(pstLCB))
    {
        for (ulIndex=0; ulIndex<ulLastIndex; ulIndex++)
        {
            if (g_pstLegCB[ulIndex].bValid)
            {
                continue;
            }

            pstLCB = &g_pstLegCB[ulIndex];
            break;
        }
    }

    if (DOS_ADDR_VALID(pstLCB))
    {
        sc_lcb_init(pstLCB);
        pstLCB->bValid = DOS_TRUE;
        pstLCB->ulAllocTime = time(NULL);
    }
    else
    {
        sc_log(LOG_LEVEL_ERROR, "Alloc leg CB fail.");
    }
    pthread_mutex_unlock(&g_mutexLegCB);

    if (pstLCB)
    {
        sc_log(LOG_LEVEL_DEBUG, "Alloc leg CB. No:%u", pstLCB->ulCBNo);
    }

    return pstLCB;
}

/**
 * 释放@a pstLCB所指定的LEG控制块
 *
 * @param SC_LEG_CB *pstLCB LEG控制块指针
 *
 * @return 成功返回LEG控制块指针，否则返回NULL
 */
VOID sc_lcb_free(SC_LEG_CB *pstLCB)
{
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return;
    }

    sc_log(LOG_LEVEL_DEBUG, "Free LCB. No:%u", pstLCB->ulCBNo);

    pthread_mutex_lock(&g_mutexLegCB);
    sc_lcb_init(pstLCB);
    pthread_mutex_unlock(&g_mutexLegCB);
}

/**
 * 通过 @a ulCBNo 获取LEG控制块
 *
 * @param U32 ulCBNo LEG控制块编号
 *
 * @return 成功返回LEG控制块指针，否则返回NULL
 *
 * @note 如果 @a ulCBNo 所指定的业务控制块为未分配状态，也会返回NULL
 */
SC_LEG_CB *sc_lcb_get(U32 ulCBNo)
{
    if (ulCBNo >= SC_LEG_CB_SIZE)
    {
        sc_log(LOG_LEVEL_INFO, "Invalid LCB No. %u", ulCBNo);
        return NULL;
    }

    if (!g_pstLegCB[ulCBNo].bValid)
    {
        sc_log(LOG_LEVEL_INFO, "LCB %u not alloced.", ulCBNo);
        return NULL;
    }

    return &g_pstLegCB[ulCBNo];
}

/**
 * LEG控制块hash表中查找一个业务控制块
 *
 * @param VOID *pSymName,
 * @param HASH_NODE_S *pNode
 *
 * @return 成功返回业务控制块指向，失败返回NULL
 */
static S32 sc_lcb_hash_find_func(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_LCB_HASH_NODE_ST *pstLCBHashNode = (SC_LCB_HASH_NODE_ST *)pNode;

    if (!pstLCBHashNode)
    {
        return DOS_FAIL;
    }

    if (dos_strncmp(pstLCBHashNode->szUUID, (S8 *)pSymName, sizeof(pstLCBHashNode->szUUID)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 向HASH表添加一个节点
 *
 * @param S8 *pszUUID 由FS参数的UUID
 * @param SC_SCB_ST *pstSCB
 *
 * @return 成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 sc_lcb_hash_add(S8 *pszUUID, SC_LEG_CB *pstLCB)
{
    U32  ulHashIndex;
    SC_LCB_HASH_NODE_ST *pstLCBHashNode;

    if (!pszUUID)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_uuid_hash_func(pszUUID, SC_UUID_HASH_SIZE);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexLCBHash);
    pstLCBHashNode = (SC_LCB_HASH_NODE_ST *)hash_find_node(g_pstLCBHash, ulHashIndex, pszUUID, sc_lcb_hash_find_func);
    if (pstLCBHashNode)
    {
        sc_trace_leg(pstLCB, "UUID %s has been added to hash list sometimes before.", pszUUID);
        if (DOS_ADDR_INVALID(pstLCBHashNode->pstLCB)
            && DOS_ADDR_VALID(pstLCB))
        {
            pstLCBHashNode->pstLCB = pstLCB;
        }
        pthread_mutex_unlock(&g_mutexLCBHash);

        return DOS_SUCC;
    }

    pstLCBHashNode = (SC_LCB_HASH_NODE_ST *)dos_dmem_alloc(sizeof(SC_LCB_HASH_NODE_ST));
    if (!pstLCBHashNode)
    {
        sc_trace_leg(pstLCB, "Alloc memory fail.");
        pthread_mutex_unlock(&g_mutexLCBHash);

        return DOS_FAIL;
    }

    HASH_Init_Node((HASH_NODE_S *)&pstLCBHashNode->stNode);
    pstLCBHashNode->pstLCB = NULL;
    pstLCBHashNode->szUUID[0] = '\0';

    if (DOS_ADDR_INVALID(pstLCBHashNode->pstLCB)
        && DOS_ADDR_VALID(pstLCB))
    {
        pstLCBHashNode->pstLCB = pstLCB;
    }

    dos_strncpy(pstLCBHashNode->szUUID, pszUUID, sizeof(pstLCBHashNode->szUUID));
    pstLCBHashNode->szUUID[sizeof(pstLCBHashNode->szUUID) - 1] = '\0';

    hash_add_node(g_pstLCBHash, (HASH_NODE_S *)pstLCBHashNode, ulHashIndex, NULL);

    sc_trace_leg(pstLCB, "Add SCB %d with the UUID %s to the hash table.", pstLCB->ulCBNo, pszUUID);

    pthread_mutex_unlock(&g_mutexLCBHash);
    return DOS_SUCC;
}

U32 sc_lcb_hash_delete(S8 *pszUUID)
{
    U32  ulHashIndex;
    SC_LCB_HASH_NODE_ST *pstLCBHashNode;

    if (!pszUUID)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_uuid_hash_func(pszUUID, SC_UUID_HASH_SIZE);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexLCBHash);
    pstLCBHashNode = (SC_LCB_HASH_NODE_ST *)hash_find_node(g_pstLCBHash, ulHashIndex, pszUUID, sc_lcb_hash_find_func);
    if (!pstLCBHashNode)
    {
        DOS_ASSERT(0);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Connot find the SCB with the UUID %s where delete the hash node.", pszUUID);
        pthread_mutex_unlock(&g_mutexLCBHash);
        return DOS_SUCC;
    }

    hash_delete_node(g_pstLCBHash, (HASH_NODE_S *)pstLCBHashNode, ulHashIndex);
    pstLCBHashNode->pstLCB = NULL;
    pstLCBHashNode->szUUID[sizeof(pstLCBHashNode->szUUID) - 1] = '\0';

    dos_dmem_free(pstLCBHashNode);
    pthread_mutex_unlock(&g_mutexLCBHash);

    return DOS_SUCC;
}

SC_LEG_CB *sc_lcb_hash_find(S8 *pszUUID)
{
    U32  ulHashIndex;
    SC_LCB_HASH_NODE_ST *pstLCBHashNode = NULL;
    SC_LEG_CB           *pstLCB = NULL;

    if (!pszUUID)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    ulHashIndex = sc_uuid_hash_func(pszUUID, SC_UUID_HASH_SIZE);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    pthread_mutex_lock(&g_mutexLCBHash);
    pstLCBHashNode = (SC_LCB_HASH_NODE_ST *)hash_find_node(g_pstLCBHash, ulHashIndex, pszUUID, sc_lcb_hash_find_func);
    if (!pstLCBHashNode)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Connot find the SCB with the UUID %s where delete the hash node.", pszUUID);
        pthread_mutex_unlock(&g_mutexLCBHash);
        return NULL;
    }
    pstLCB = pstLCBHashNode->pstLCB;
    pthread_mutex_unlock(&g_mutexLCBHash);

    return pstLCB;
}


VOID sc_scb_call_init(SC_SRV_CALL_ST *pstCall)
{
    if (DOS_ADDR_INVALID(pstCall))
    {
        DOS_ASSERT(0);
        return;
    }

    pstCall->stSCBTag.bTrace = DOS_FALSE;
    pstCall->stSCBTag.bValid = DOS_FALSE;
    pstCall->stSCBTag.bWaitingExit = DOS_FALSE;
    pstCall->stSCBTag.usSrvType = SC_SRV_CALL;
    pstCall->stSCBTag.usStatus = SC_CALL_IDEL;
    pstCall->ulCallingLegNo = U32_BUTT;
    pstCall->ulCalleeLegNo = U32_BUTT;
    pstCall->pstAgentCallee = NULL;
    pstCall->pstAgentCalling = NULL;
    pstCall->ulCallSrc = U32_BUTT;
    pstCall->ulCallDst = U32_BUTT;
    pstCall->bIsRingTimer = DOS_FALSE;
    pstCall->stTmrHandle = NULL;
    pstCall->ulAgentGrpID = 0;
}

VOID sc_scb_preview_call_init(SC_PREVIEW_CALL_ST *pstPreviewCall)
{
    if (DOS_ADDR_INVALID(pstPreviewCall))
    {
        DOS_ASSERT(0);
        return;
    }

    pstPreviewCall->stSCBTag.bTrace = DOS_FALSE;
    pstPreviewCall->stSCBTag.bValid = DOS_FALSE;
    pstPreviewCall->stSCBTag.usSrvType = SC_SRV_PREVIEW_CALL;
    pstPreviewCall->stSCBTag.usStatus = SC_PREVIEW_CALL_IDEL;
    pstPreviewCall->ulCallingLegNo = U32_BUTT;
    pstPreviewCall->ulCalleeLegNo = U32_BUTT;
    pstPreviewCall->ulAgentID = U32_BUTT;
}

VOID sc_scb_auto_call_init(SC_AUTO_CALL_ST *pstAutoCall)
{
    if (DOS_ADDR_INVALID(pstAutoCall))
    {
        DOS_ASSERT(0);
        return;
    }

    pstAutoCall->stSCBTag.bTrace = DOS_FALSE;
    pstAutoCall->stSCBTag.bValid = DOS_FALSE;
    pstAutoCall->stSCBTag.usSrvType = SC_SRV_AUTO_CALL;
    pstAutoCall->stSCBTag.usStatus = SC_AUTO_CALL_IDEL;
    pstAutoCall->ulCallingLegNo = U32_BUTT;
    pstAutoCall->ulCalleeLegNo = U32_BUTT;
    pstAutoCall->ulAudioID = 0;
    pstAutoCall->ulKeyMode = 0;
    pstAutoCall->ulAgentID = 0;
    pstAutoCall->ulTaskID = 0;
    pstAutoCall->ulTcbID = U32_BUTT;
    pstAutoCall->bIsRingTimer = DOS_FALSE;
    pstAutoCall->stTmrHandle = NULL;

}

VOID sc_scb_voice_verify_init(SC_VOICE_VERIFY_ST *pstVoiceVerify)
{
    if (DOS_ADDR_INVALID(pstVoiceVerify))
    {
        DOS_ASSERT(0);
        return;
    }

    pstVoiceVerify->stSCBTag.bValid = DOS_FALSE;
    pstVoiceVerify->stSCBTag.bTrace = DOS_FALSE;
    pstVoiceVerify->stSCBTag.usSrvType = SC_SRV_VOICE_VERIFY;
    pstVoiceVerify->stSCBTag.usStatus = SC_VOICE_VERIFY_INIT;
    pstVoiceVerify->ulLegNo = U32_BUTT;
    pstVoiceVerify->ulTipsHitNo1 = 0;
    pstVoiceVerify->ulTipsHitNo2 = 0;
    pstVoiceVerify->szVerifyCode[0] = '\0';
}

VOID sc_scb_access_code_init(SC_ACCESS_CODE_ST *pstAccessCode)
{
    if (DOS_ADDR_INVALID(pstAccessCode))
    {
        DOS_ASSERT(0);
        return;
    }

    pstAccessCode->stSCBTag.bTrace = DOS_FALSE;
    pstAccessCode->stSCBTag.bValid = DOS_FALSE;
    pstAccessCode->stSCBTag.usSrvType = SC_SRV_ACCESS_CODE;
    pstAccessCode->stSCBTag.usStatus = SC_ACCESS_CODE_IDEL;
    pstAccessCode->ulLegNo = U32_BUTT;
    pstAccessCode->bIsSecondDial = DOS_FALSE;
    pstAccessCode->ulSrvType = 0;
    pstAccessCode->szDialCache[0] = '\0';
    pstAccessCode->ulAgentID = 0;
}

VOID sc_scb_hold_init(SC_CALL_HOLD_ST *pstHold)
{
    if (DOS_ADDR_INVALID(pstHold))
    {
        DOS_ASSERT(0);
        return;
    }

    pstHold->stSCBTag.bTrace = DOS_FALSE;
    pstHold->stSCBTag.bValid = DOS_FALSE;
    pstHold->stSCBTag.usSrvType = SC_SRV_HOLD;
    pstHold->stSCBTag.usStatus = SC_HOLD_IDEL;
    pstHold->ulCallLegNo = U32_BUTT;
    pstHold->ulHoldCount = 0;
}

VOID sc_scb_transfer_init(SC_CALL_TRANSFER_ST *pstTransfer)
{
    if (DOS_ADDR_INVALID(pstTransfer))
    {
        DOS_ASSERT(0);
        return;
    }

    pstTransfer->stSCBTag.bTrace = DOS_FALSE;
    pstTransfer->stSCBTag.bValid = DOS_FALSE;
    pstTransfer->stSCBTag.usSrvType = SC_SRV_TRANSFER;
    pstTransfer->stSCBTag.usStatus = SC_TRANSFER_IDEL;
    pstTransfer->ulType = U32_BUTT;
    pstTransfer->ulPublishType = U32_BUTT;
    pstTransfer->ulSubLegNo = U32_BUTT;
    pstTransfer->ulPublishLegNo = U32_BUTT;
    pstTransfer->ulNotifyLegNo = U32_BUTT;
    pstTransfer->ulSubAgentID = 0;
    pstTransfer->ulPublishAgentID = 0;
    pstTransfer->ulNotifyAgentID = 0;
    pstTransfer->szSipNum[0] = '\0';
}

VOID sc_scb_incoming_queue_init(SC_INCOMING_QUEUE_ST *pstIncomingQueue)
{
    if (DOS_ADDR_INVALID(pstIncomingQueue))
    {
        DOS_ASSERT(0);
        return;
    }

    pstIncomingQueue->stSCBTag.bValid = DOS_FALSE;
    pstIncomingQueue->stSCBTag.bTrace = DOS_FALSE;
    pstIncomingQueue->stSCBTag.usSrvType = SC_SRV_INCOMING_QUEUE;
    pstIncomingQueue->stSCBTag.usStatus = SC_INQUEUE_IDEL;
    pstIncomingQueue->ulLegNo = U32_BUTT;
    pstIncomingQueue->ulQueueType = 0;
    pstIncomingQueue->ulEnqueuTime = 0;
    pstIncomingQueue->ulDequeuTime = 0;
}

VOID sc_scb_interception_init(SC_INTERCEPTION_ST *pstInterception)
{
    if (DOS_ADDR_INVALID(pstInterception))
    {
        DOS_ASSERT(0);
        return;
    }

    pstInterception->stSCBTag.bValid = DOS_FALSE;
    pstInterception->stSCBTag.bTrace = DOS_FALSE;
    pstInterception->stSCBTag.usSrvType = SC_SRV_INTERCEPTION;
    pstInterception->stSCBTag.usStatus = SC_INTERCEPTION_IDEL;
    pstInterception->ulLegNo = U32_BUTT;
    pstInterception->ulAgentLegNo = U32_BUTT;
    pstInterception->pstAgentInfo = NULL;

}

VOID sc_scb_whisper_init(SC_SRV_WHISPER_ST *pstWhisper)
{
    if (DOS_ADDR_INVALID(pstWhisper))
    {
        DOS_ASSERT(0);
        return;
    }

    pstWhisper->stSCBTag.bValid = DOS_FALSE;
    pstWhisper->stSCBTag.bTrace = DOS_FALSE;
    pstWhisper->stSCBTag.usSrvType = SC_SRV_WHISPER;
    pstWhisper->stSCBTag.usStatus = SC_WHISPER_IDEL;
    pstWhisper->ulLegNo = U32_BUTT;
    pstWhisper->ulAgentLegNo = U32_BUTT;
    pstWhisper->pstAgentInfo = NULL;
}

VOID sc_scb_mark_custom_init(SC_MARK_CUSTOM_ST *pstMarkCustom)
{
    if (DOS_ADDR_INVALID(pstMarkCustom))
    {
        DOS_ASSERT(0);
        return;
    }

    pstMarkCustom->stSCBTag.bValid = DOS_FALSE;
    pstMarkCustom->stSCBTag.bTrace = DOS_FALSE;
    pstMarkCustom->stSCBTag.usSrvType = SC_SRV_MARK_CUSTOM;
    pstMarkCustom->stSCBTag.usStatus = SC_MAKR_CUSTOM_IDEL;
    pstMarkCustom->ulLegNo = U32_BUTT;
    pstMarkCustom->pstAgentCall = NULL;
    pstMarkCustom->szDialCache[0] = '\0';
    pstMarkCustom->stTmrHandle = NULL;
}

VOID sc_scb_sigin_init(SC_SIGIN_ST *pstSigin)
{
    if (DOS_ADDR_INVALID(pstSigin))
    {
        DOS_ASSERT(0);
        return;
    }

    pstSigin->stSCBTag.bValid = DOS_FALSE;
    pstSigin->stSCBTag.bTrace = DOS_FALSE;
    pstSigin->stSCBTag.usSrvType = SC_SRV_AGENT_SIGIN;
    pstSigin->stSCBTag.usStatus = SC_SIGIN_IDEL;
    pstSigin->ulLegNo = U32_BUTT;
    pstSigin->pstAgentNode = NULL;
}

VOID sc_scb_demo_task_init(SC_AUTO_CALL_ST *pstAutoCall)
{
    if (DOS_ADDR_INVALID(pstAutoCall))
    {
        DOS_ASSERT(0);
        return;
    }

    pstAutoCall->stSCBTag.bTrace = DOS_FALSE;
    pstAutoCall->stSCBTag.bValid = DOS_FALSE;
    pstAutoCall->stSCBTag.usSrvType = SC_SRV_DEMO_TASK;
    pstAutoCall->stSCBTag.usStatus = SC_AUTO_CALL_IDEL;
    pstAutoCall->ulCallingLegNo = U32_BUTT;
    pstAutoCall->ulCalleeLegNo = U32_BUTT;
    pstAutoCall->ulAudioID = 0;
    pstAutoCall->ulKeyMode = 0;
    pstAutoCall->ulAgentID = 0;
    pstAutoCall->ulTaskID = 0;
    pstAutoCall->ulTcbID = U32_BUTT;
    pstAutoCall->bIsRingTimer = DOS_FALSE;
    pstAutoCall->stTmrHandle = NULL;
}

VOID sc_scb_call_agent_init(SC_SRV_CALL_AGENT_ST *pstCallAgent)
{
    if (DOS_ADDR_INVALID(pstCallAgent))
    {
        DOS_ASSERT(0);
        return;
    }

    pstCallAgent->stSCBTag.bTrace = DOS_FALSE;
    pstCallAgent->stSCBTag.bValid = DOS_FALSE;
    pstCallAgent->stSCBTag.usSrvType = SC_SRV_CALL_AGENT;
    pstCallAgent->stSCBTag.usStatus = SC_CALL_AGENT_IDEL;
    pstCallAgent->ulCallingLegNo = U32_BUTT;
    pstCallAgent->ulCalleeLegNo = U32_BUTT;
    pstCallAgent->ulCalleeType = U32_BUTT;
    pstCallAgent->pstAgentCalling = NULL;
    pstCallAgent->pstAgentCalling = NULL;
    pstCallAgent->szCalleeNum[0] = '\0';
}

VOID sc_scb_auto_preview_init(SC_AUTO_PREVIEW_ST *pstPreviewCall)
{
    if (DOS_ADDR_INVALID(pstPreviewCall))
    {
        DOS_ASSERT(0);
        return;
    }

    pstPreviewCall->stSCBTag.bTrace = DOS_FALSE;
    pstPreviewCall->stSCBTag.bValid = DOS_FALSE;
    pstPreviewCall->stSCBTag.usSrvType = SC_SRV_AUTO_PREVIEW;
    pstPreviewCall->stSCBTag.usStatus = SC_AUTO_PREVIEW_IDEL;
    pstPreviewCall->ulCallingLegNo = U32_BUTT;
    pstPreviewCall->ulCalleeLegNo = U32_BUTT;
    pstPreviewCall->ulAgentID = 0;
    pstPreviewCall->ulTaskID = 0;
    pstPreviewCall->ulTcbID = U32_BUTT;
}

/**
 * 初始化也控制块
 *
 * @param pstSCB 需要被初始化的业务控制块编号
 *
 * @return NULL
 */
VOID sc_scb_init(SC_SRV_CB *pstSCB)
{
    U32 ulIndex;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return;
    }


    pstSCB->bValid = DOS_FALSE;
    pstSCB->bTrace = DOS_FALSE;
    for (ulIndex=0; ulIndex<SC_SRV_BUTT; ulIndex++)
    {
        pstSCB->pstServiceList[ulIndex] = NULL;
    }

    for (ulIndex=0; ulIndex<SC_MAX_SERVICE_TYPE; ulIndex++)
    {
        pstSCB->aucServType[ulIndex] = 0;
    }

    pstSCB->ulCurrentSrv = 0;
    pstSCB->ulCustomerID = U32_BUTT;
    pstSCB->ulAgentID = 0;
    pstSCB->ulAllocTime = 0;
    pstSCB->ulClientID = 0;
    pstSCB->szClientNum[0] = '\0';

    sc_scb_call_init(&pstSCB->stCall);
    sc_scb_preview_call_init(&pstSCB->stPreviewCall);
    sc_scb_auto_call_init(&pstSCB->stAutoCall);
    sc_scb_voice_verify_init(&pstSCB->stVoiceVerify);
    sc_scb_access_code_init(&pstSCB->stAccessCode);
    sc_scb_hold_init(&pstSCB->stHold);
    sc_scb_transfer_init(&pstSCB->stTransfer);
    sc_scb_incoming_queue_init(&pstSCB->stIncomingQueue);
    sc_scb_interception_init(&pstSCB->stInterception);
    sc_scb_whisper_init(&pstSCB->stWhispered);
    sc_scb_mark_custom_init(&pstSCB->stMarkCustom);
    sc_scb_sigin_init(&pstSCB->stSigin);
    sc_scb_demo_task_init(&pstSCB->stDemoTask);
    sc_scb_call_agent_init(&pstSCB->stCallAgent);
    sc_scb_auto_preview_init(&pstSCB->stAutoPreview);
}

/**
 * 申请一个空闲的业务控制块
 *
 * @return 成功返回业务控制块指针，否则返回NULL
 */
SC_SRV_CB *sc_scb_alloc()
{
    static U32 ulIndex = 0;
    U32        ulLastIndex = 0;
    SC_SRV_CB  *pstSCB = NULL;

    pthread_mutex_lock(&g_mutexSCBList);
    ulLastIndex = ulIndex;
    for (; ulIndex<SC_SCB_SIZE; ulIndex++)
    {
        if (g_pstSCBList[ulIndex].bValid)
        {
            continue;
        }

        pstSCB = &g_pstSCBList[ulIndex];
        break;
    }

    if (DOS_ADDR_INVALID(pstSCB))
    {
        for (ulIndex=0; ulIndex<ulLastIndex; ulIndex++)
        {
            if (g_pstSCBList[ulIndex].bValid)
            {
                continue;
            }

            pstSCB = &g_pstSCBList[ulIndex];
            break;
        }
    }

    if (DOS_ADDR_VALID(pstSCB))
    {
        sc_scb_init(pstSCB);

        if (g_stSysStat.ulCurrentCalls < sc_get_call_limitation())
        {
            if (g_stSysStat.ulCurrentCalls != U32_BUTT)
            {
                g_stSysStat.ulCurrentCalls++;
            }
            else
            {
                DOS_ASSERT(0);
            }

            pstSCB->bValid = DOS_TRUE;
            pstSCB->ulAllocTime = time(NULL);
        }
        else
        {
            pstSCB = NULL;
        }
    }
    else
    {
        sc_log(LOG_LEVEL_ERROR, "Alloc Service CB fail.");
    }
    pthread_mutex_unlock(&g_mutexSCBList);

    if (pstSCB)
    {
        sc_log(LOG_LEVEL_DEBUG, "Alloc Service CB. No:%u", pstSCB->ulSCBNo);
    }

    return pstSCB;
}


/**
 * 释放 @a pstSCB 指定的业务控制块
 *
 * @param SC_SRV_CB *pstSCB 业务控制块指针
 *
 * @return VOID
 */
VOID sc_scb_free(SC_SRV_CB *pstSCB)
{
    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return;
    }

    sc_log(LOG_LEVEL_DEBUG, "Free SCB. No:%u", pstSCB->ulSCBNo);

    pthread_mutex_lock(&g_mutexSCBList);
    sc_scb_init(pstSCB);
    pthread_mutex_unlock(&g_mutexSCBList);

    if (g_stSysStat.ulCurrentCalls != 0)
    {
        g_stSysStat.ulCurrentCalls--;
    }
    else
    {
        DOS_ASSERT(0);
    }
}

/**
 * 通过 @a ulCBNo 获取业务控制块
 *
 * @param U32 ulCBNo 业务控制块编号
 *
 * @return 成功返回业务控制块指针，否则返回NULL
 *
 * @note 如果 @a ulCBNo 所指定的业务控制块为未分配状态，也会返回NULL
 */
SC_SRV_CB *sc_scb_get(U32 ulCBNo)
{
    if (ulCBNo >= SC_SCB_SIZE)
    {
        sc_log(LOG_LEVEL_INFO, "Invalid SCB No.");
        return NULL;
    }

    if (!g_pstSCBList[ulCBNo].bValid)
    {
        sc_log(LOG_LEVEL_INFO, "SCB %u not alloc.", ulCBNo);
        return NULL;
    }

    return &g_pstSCBList[ulCBNo];
}

U32 sc_scb_copy(SC_SRV_CB *pstDstSCB, SC_SRV_CB *pstSrcSCB)
{
    U32             ulScbNo         = U32_BUTT;
    S32             i               = 0;
    SC_SCB_TAG_ST   *pstSCBTag      = NULL;

    if (DOS_ADDR_INVALID(pstSrcSCB)
        || DOS_ADDR_INVALID(pstDstSCB))
    {
        return DOS_FAIL;
    }

    ulScbNo = pstDstSCB->ulSCBNo;
    dos_memcpy(pstDstSCB, pstSrcSCB, sizeof(SC_SRV_CB));
    pstDstSCB->ulSCBNo = ulScbNo;

    for (i=0; i<=pstDstSCB->ulCurrentSrv; i++)
    {
        switch (pstDstSCB->pstServiceList[i]->usSrvType)
        {
            case SC_SRV_CALL:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stCall;
                break;

            case SC_SRV_PREVIEW_CALL:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stPreviewCall;
                break;

            case SC_SRV_AUTO_CALL:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stAutoCall;
                break;

            case SC_SRV_VOICE_VERIFY:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stVoiceVerify;
                break;

            case SC_SRV_ACCESS_CODE:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stAccessCode;
                break;

            case SC_SRV_HOLD:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stHold;
                break;

            case SC_SRV_TRANSFER:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stTransfer;
                break;

            case SC_SRV_INCOMING_QUEUE:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stIncomingQueue;
                break;

            case SC_SRV_INTERCEPTION:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stInterception;
                break;

            case SC_SRV_WHISPER:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stWhispered;
                break;

            case SC_SRV_MARK_CUSTOM:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stMarkCustom;
                break;

            case SC_SRV_AGENT_SIGIN:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stSigin;
                break;

            case SC_SRV_DEMO_TASK:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stDemoTask;
                break;

            case SC_SRV_CALL_AGENT:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stCallAgent;
                break;

            case SC_SRV_AUTO_PREVIEW:
                pstSCBTag = (SC_SCB_TAG_ST *)&pstDstSCB->stAutoPreview;
                break;

            default:
                DOS_ASSERT(0);
                pstSCBTag = NULL;
                break;
        }

        pstDstSCB->pstServiceList[i] = pstSCBTag;
    }

    return DOS_SUCC;
}

/**
 * 检查业务控制块 @a pstSCB 中是否有 @a ulService这种业务
 *
 * @param SC_SRV_CB *pstSCB
 * @param U32 ulService
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_scb_check_service(SC_SRV_CB *pstSCB, U32 ulService)
{
    U32  ulIndex;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (0 == ulService || ulService >= BS_SERV_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    for (ulIndex=0; ulIndex<SC_MAX_SERVICE_TYPE; ulIndex++)
    {
        if (pstSCB->aucServType[ulIndex] != 0 && ulService == pstSCB->aucServType[ulIndex])
        {
            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}


/**
 * 给指定的SCB设置业务
 *
 * @param SC_SRV_CB *pstSCB
 * @param U32 ulService
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_scb_set_service(SC_SRV_CB *pstSCB, U32 ulService)
{
    U32  ulIndex;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (0 == ulService || ulService >= BS_SERV_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    for (ulIndex=0; ulIndex<SC_MAX_SERVICE_TYPE; ulIndex++)
    {
        if (pstSCB->aucServType[ulIndex] == 0)
        {
            pstSCB->aucServType[ulIndex] = ulService;
            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}

U32 sc_scb_remove_service(SC_SRV_CB *pstSCB, U32 ulService)
{
    U32  ulIndex;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (0 == ulService || ulService >= BS_SERV_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    for (ulIndex=0; ulIndex<SC_MAX_SERVICE_TYPE; ulIndex++)
    {
        if (pstSCB->aucServType[ulIndex] == ulService)
        {
            pstSCB->aucServType[ulIndex] = 0;
        }
    }

    return DOS_SUCC;
}

BOOL sc_scb_is_exit_service(SC_SRV_CB *pstSCB, U32 ulService)
{
    U32  ulIndex;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    if (0 == ulService || ulService >= BS_SERV_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    for (ulIndex=0; ulIndex<SC_MAX_SERVICE_TYPE; ulIndex++)
    {
        if (pstSCB->aucServType[ulIndex] == ulService)
        {
            return DOS_TRUE;
        }
    }

    return DOS_FALSE;
}

U32 sc_tcb_init(SC_TASK_CB *pstTCB)
{
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstTCB->ulAllocTime = 0;
    pstTCB->ucTaskStatus = SC_TASK_BUTT;
    pstTCB->ucMode = SC_TASK_MODE_BUTT;
    pstTCB->ucPriority = SC_TASK_PRI_NORMAL;
    pstTCB->bTraceON = 0;
    pstTCB->bTraceCallON = 0;
    pstTCB->ulLastCalleeIndex = 0;
    pstTCB->bThreadRunning = DOS_FALSE;

    pstTCB->ulTaskID = U32_BUTT;
    pstTCB->ulCustomID = U32_BUTT;
    pstTCB->ulCurrentConcurrency = 0;
    pstTCB->ulMaxConcurrency = SC_MAX_CALL;
    pstTCB->usSiteCount = 0;
    pstTCB->ulAgentQueueID = U32_BUTT;
    pstTCB->ucAudioPlayCnt = 0;
    pstTCB->ulCalleeCount = 0;
    pstTCB->usCallerCount = 0;
    pstTCB->ulCalledCount = 0;
    pstTCB->ulCalledCountLast = 0;
    pstTCB->ulCallerGrpID = 0;
    pstTCB->ulCallRate = 0;
    pstTCB->ulCalleeCountTotal = 0;

    dos_list_init(&pstTCB->stCalleeNumQuery);    /* TODO: 释放所有节点 */
    pstTCB->szAudioFileLen[0] = '\0';
    dos_memzero(pstTCB->astPeriod, sizeof(pstTCB->astPeriod));

    /* 统计相关 */
    pstTCB->ulTotalCall = 0;
    pstTCB->ulCallFailed = 0;
    pstTCB->ulCallConnected = 0;

    pstTCB->pstTmrHandle = NULL;

    return DOS_SUCC;
}

SC_TASK_CB *sc_tcb_alloc()
{
    static U32 ulIndex = 0;
    U32 ulLastIndex;
    SC_TASK_CB  *pstTCB = NULL;

    pthread_mutex_lock(&g_mutexTaskList);

    ulLastIndex = ulIndex;

    for (; ulIndex < SC_MAX_TASK_NUM; ulIndex++)
    {
        if (!g_pstTaskList[ulIndex].ucValid)
        {
            pstTCB = &g_pstTaskList[ulIndex];
            break;
        }
    }

    if (!pstTCB)
    {
        for (ulIndex = 0; ulIndex < ulLastIndex; ulIndex++)
        {
            if (!g_pstTaskList[ulIndex].ucValid)
            {
                pstTCB = &g_pstTaskList[ulIndex];
                break;
            }
        }
    }

    if (pstTCB)
    {
        pthread_mutex_init(&pstTCB->mutexTaskList, NULL);

        pthread_mutex_lock(&pstTCB->mutexTaskList);
        sc_tcb_init(pstTCB);
        pstTCB->ucValid = 1;
        pstTCB->ulAllocTime = time(0);
        pthread_mutex_unlock(&pstTCB->mutexTaskList);

        pthread_mutex_unlock(&g_mutexTaskList);

        sc_trace_task(pstTCB, "Alloc TCB %d.", pstTCB->usTCBNo);

        return pstTCB;
    }

    pthread_mutex_unlock(&g_mutexTaskList);
    return NULL;

}

VOID sc_tcb_free(SC_TASK_CB *pstTCB)
{
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return;
    }

    sc_trace_task(pstTCB, "Free TCB. %d", pstTCB->usTCBNo);

    pthread_mutex_lock(&g_mutexTaskList);

    sc_tcb_init(pstTCB);
    pstTCB->ucValid = 0;

    pthread_mutex_unlock(&g_mutexTaskList);
    return;
}


SC_TASK_CB *sc_tcb_get(U32 ulTCBNo)
{
    if (ulTCBNo >= SC_MAX_TASK_NUM)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    return &g_pstTaskList[ulTCBNo];
}

SC_TASK_CB *sc_tcb_find_by_taskid(U32 ulTaskID)
{
    U32 ulIndex;

    for (ulIndex = 0; ulIndex < SC_MAX_TASK_NUM; ulIndex++)
    {
        if (g_pstTaskList[ulIndex].ucValid
            && g_pstTaskList[ulIndex].ulTaskID == ulTaskID)
        {
            return &g_pstTaskList[ulIndex];
        }
    }

    return NULL;
}

U32 sc_task_check_can_call(SC_TASK_CB *pstTCB)
{
    U32 ulIdleAgent    = 0;
    U32 ulBusyAgent    = 0;
    U32 ulTotalAgent   = 0;
    S32 lTotalCalls    = 0;
    S32 lNeedCalls     = 0;

    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    if (SC_TASK_MODE_AUDIO_ONLY == pstTCB->ucMode)
    {
        if (pstTCB->ulCurrentConcurrency >= pstTCB->ulMaxConcurrency)
        {
            return DOS_FALSE;
        }
    }
    else
    {
        sc_agent_group_stat_by_id(pstTCB->ulAgentQueueID, &ulTotalAgent, NULL, &ulIdleAgent, &ulBusyAgent);
        pstTCB->usSiteCount = ulTotalAgent;
        pstTCB->ulMaxConcurrency = ceil(1.0 * (ulTotalAgent * pstTCB->ulCallRate) / 10);

        /*
          * 大意:
          *    ulIdleAgent * pstTCB->ulCallRate: 需要为空闲坐席发起的呼叫数
          *    pstTCB->ulCurrentConcurrency - ulBusyAgent: 当前系统已经为空闲坐席发起呼叫数
          */
        lTotalCalls = ceil(1.0 * (ulIdleAgent * pstTCB->ulCallRate) / 10);
        lNeedCalls  = pstTCB->ulCurrentConcurrency - ulBusyAgent;

        if ((S32)pstTCB->ulCurrentConcurrency >= (S32)pstTCB->ulMaxConcurrency)
        {
            return DOS_FALSE;
        }

        if (lNeedCalls >= lTotalCalls)
        {
            return DOS_FALSE;
        }

        if (0 == ulIdleAgent)
        {
            return DOS_FALSE;
        }
    }

    return DOS_TRUE;
}

U32 sc_task_check_can_call_by_time(SC_TASK_CB *pstTCB)
{
    time_t     now;
    struct tm  stimenow;
    struct tm  *pstimenow;
    U32 ulWeek, ulHour, ulMinute, ulSecond, ulIndex;
    U32 ulStartTime, ulEndTime, ulCurrentTime;


    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    time(&now);
    pstimenow = dos_get_localtime_struct(now, &stimenow);

    ulWeek = pstimenow->tm_wday;
    ulHour = pstimenow->tm_hour;
    ulMinute = pstimenow->tm_min;
    ulSecond = pstimenow->tm_sec;


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

        ulStartTime = pstTCB->astPeriod[ulIndex].ucHourBegin * 60 * 60 + pstTCB->astPeriod[ulIndex].ucMinuteBegin * 60 + pstTCB->astPeriod[ulIndex].ucSecondBegin;
        ulEndTime = pstTCB->astPeriod[ulIndex].ucHourEnd * 60 * 60 + pstTCB->astPeriod[ulIndex].ucMinuteEnd * 60 + pstTCB->astPeriod[ulIndex].ucSecondEnd;
        ulCurrentTime = ulHour * 60 * 60 + ulMinute * 60 + ulSecond;

        if (ulCurrentTime >= ulStartTime && ulCurrentTime < ulEndTime)
        {
            return DOS_TRUE;
        }
    }

    return DOS_FALSE;
}

U32 sc_task_check_can_call_by_status(SC_TASK_CB *pstTCB)
{
    U32 ulIdelCPUConfig, ulIdelCPU;

    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    ulIdelCPU = dos_get_cpu_idel_percentage();
    if (config_get_min_iedl_cpu(&ulIdelCPUConfig) != DOS_SUCC)
    {
        ulIdelCPUConfig = DOS_MIN_IDEL_CPU * 100;
    }
    else
    {
        ulIdelCPUConfig = ulIdelCPUConfig * 100;
    }

    /* 系统整体并发量控制 */
    if (pstTCB->ulCurrentConcurrency >= g_ulMaxConcurrency4Task)
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

#if 0
    if (g_pstTaskMngtInfo->stStat.ulCurrentSessions >= SC_MAX_CALL)
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }
#endif

    /* CPU占用控制 */
    if (ulIdelCPU < ulIdelCPUConfig)
    {
        dos_printf("Current Idel CPU: %u, Config Idel CPU: %u", ulIdelCPU , ulIdelCPUConfig);
        return DOS_FALSE;
    }

    return DOS_TRUE;
}

VOID sc_task_set_owner(SC_TASK_CB *pstTCB, U32 ulTaskID, U32 ulCustomID)
{
    if (!pstTCB)
    {
        DOS_ASSERT(0);

        return;
    }

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);

        return;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        return;
    }

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ulTaskID = ulTaskID;
    pstTCB->ulCustomID = ulCustomID;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);
}

S32 sc_task_and_callee_load(U32 ulIndex)
{
    SC_TASK_CB *pstTCB = NULL;
    S32 lRet = U32_BUTT;

    if (sc_task_load(ulIndex) != DOS_SUCC)
    {
        return DOS_FAIL;
    }

    pstTCB = sc_tcb_find_by_taskid(ulIndex);
    if (!pstTCB)
    {
        sc_log(LOG_LEVEL_ERROR, "SC Find task By TaskID FAIL.(TaskID:%u) ", ulIndex);
        return DOS_FAIL;
    }

    /* 维护被叫号码数据 */
    lRet = sc_task_load_callee(pstTCB);
    if (DOS_SUCC != lRet)
    {
        sc_log(LOG_LEVEL_ERROR, "SC Task Load Callee FAIL.(TaskID:%u, usNo:%u)", ulIndex, pstTCB->usTCBNo);
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_DEBUG, "SC Task Load callee SUCC.(TaskID:%u, usNo:%u)", ulIndex, pstTCB->usTCBNo);

    return DOS_SUCC;
}

/**
 * 向业务子层发送命令
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_command(SC_MSG_TAG_ST *pstMsg)
{
    DLL_NODE_S   *pstDLLNode = NULL;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstDLLNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstMsg);
        return DOS_FAIL;
    }

    DLL_Init_Node(pstDLLNode);
    pstDLLNode->pHandle = pstMsg;

    pthread_mutex_lock(&g_mutexCommandQueue);
    DLL_Add(&g_stCommandQueue, pstDLLNode);
    pthread_cond_signal(&g_condCommandQueue);
    pthread_mutex_unlock(&g_mutexCommandQueue);

    sc_log(LOG_LEVEL_DEBUG, "Send %s(%u). SCB: %u, Error: %u"
                , sc_command_str(pstMsg->ulMsgType), pstMsg->ulMsgType
                , pstMsg->ulSCBNo, pstMsg->usInterErr);

    return DOS_SUCC;
}

/**
 * 向业务子层发送发起呼叫命令
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_cmd_new_call(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_CMD_CALL_ST *pstCMDCall = NULL;
    U32                ulRet;

    pstCMDCall = (SC_MSG_CMD_CALL_ST *)dos_dmem_alloc(sizeof(SC_MSG_CMD_CALL_ST));
    if (DOS_ADDR_INVALID(pstCMDCall))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memcpy(pstCMDCall, pstMsg, sizeof(SC_MSG_CMD_CALL_ST));

    ulRet = sc_send_command(&pstCMDCall->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_ERROR, "Make call request send fail. SCB: %u", pstMsg->ulSCBNo);

        dos_dmem_free(pstCMDCall);
        pstCMDCall = NULL;
    }

    sc_log(SC_LOG_SET_FLAG(LOG_LEVEL_DEBUG, 0, SC_LOG_DISIST), "Send new call request. SCB: %u", pstMsg->ulSCBNo);

    return ulRet;
}

U32 sc_send_cmd_bridge(SC_MSG_TAG_ST *pstMsg)
{
    return DOS_SUCC;
}

U32 sc_send_cmd_release(SC_MSG_TAG_ST *pstMsg)
{
    return DOS_SUCC;
}

U32 sc_send_cmd_playback(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_CMD_PLAYBACK_ST *pstCMDPlayback = NULL;
    U32 ulRet;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCMDPlayback = (SC_MSG_CMD_PLAYBACK_ST *)dos_dmem_alloc(sizeof(SC_MSG_CMD_PLAYBACK_ST));
    if (DOS_ADDR_INVALID(pstCMDPlayback))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstCMDPlayback, pstMsg, sizeof(SC_MSG_CMD_PLAYBACK_ST));

    ulRet = sc_send_command(&pstCMDPlayback->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstCMDPlayback);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_send_cmd_playback_stop(SC_MSG_TAG_ST *pstMsg)
{
    return DOS_SUCC;
}

U32 sc_send_cmd_record(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_CMD_RECORD_ST    *pstRecordRsp   = NULL;
    SC_SRV_CB               *pstSCB         = NULL;
    SC_LEG_CB               *pstRecordLegCB = NULL;
    SC_AGENT_NODE_ST        *pstAgentNode   = NULL;
    S8         szEmpNo[SC_NUM_LENGTH] = {0};

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstRecordRsp = (SC_MSG_CMD_RECORD_ST *)pstMsg;
    pstSCB = sc_scb_get(pstRecordRsp->ulSCBNo);
    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstRecordLegCB = sc_lcb_get(pstRecordRsp->ulLegNo);
    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (sc_serv_ctrl_check(pstSCB->ulCustomerID
                                , BS_SERV_RECORDING
                                , SC_SRV_CTRL_ATTR_INVLID
                                , U32_BUTT
                                , SC_SRV_CTRL_ATTR_INVLID
                                , U32_BUTT))
    {
        sc_log(SC_LOG_SET_FLAG(LOG_LEVEL_INFO, SC_MOD_EVENT, SC_LOG_DISIST), "Recording service is under control, reject recoding request. Customer: %u", pstSCB->ulCustomerID);
        return DOS_SUCC;
    }

    if (sc_scb_set_service(pstSCB, BS_SERV_RECORDING))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add record service fail.");

        return DOS_SUCC;
    }

    if (pstSCB->stCall.stSCBTag.bValid)
    {
        if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
            && pstSCB->stCall.pstAgentCallee->pstAgentInfo->bRecord)
        {
            dos_strcpy(szEmpNo, pstSCB->stCall.pstAgentCallee->pstAgentInfo->szEmpNo);
        }
        else if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling))
        {
            dos_strcpy(szEmpNo, pstSCB->stCall.pstAgentCalling->pstAgentInfo->szEmpNo);
        }
        else if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee))
        {
            dos_strcpy(szEmpNo, pstSCB->stCall.pstAgentCallee->pstAgentInfo->szEmpNo);
        }
        else
        {
            /* 没有找到坐席 */
            szEmpNo[0] = '\0';
        }
    }
    else if (pstSCB->stPreviewCall.stSCBTag.bValid)
    {
        pstAgentNode = sc_agent_get_by_id(pstSCB->stPreviewCall.ulAgentID);
        if (DOS_ADDR_VALID(pstAgentNode)
            && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
        {
            dos_strcpy(szEmpNo, pstAgentNode->pstAgentInfo->szEmpNo);
        }
        else
        {
            szEmpNo[0] = '\0';
        }
    }
    else if (pstSCB->stAutoCall.stSCBTag.bValid)
    {
        pstAgentNode = sc_agent_get_by_id(pstSCB->stAutoCall.ulAgentID);
        if (DOS_ADDR_VALID(pstAgentNode)
            && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
        {
            dos_strcpy(szEmpNo, pstAgentNode->pstAgentInfo->szEmpNo);
        }
        else
        {
            szEmpNo[0] = '\0';
        }
    }
    else if (pstSCB->stDemoTask.stSCBTag.bValid)
    {
        pstAgentNode = sc_agent_get_by_id(pstSCB->stDemoTask.ulAgentID);
        if (DOS_ADDR_VALID(pstAgentNode)
            && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
        {
            dos_strcpy(szEmpNo, pstAgentNode->pstAgentInfo->szEmpNo);
        }
        else
        {
            szEmpNo[0] = '\0';
        }
    }
    else if (pstSCB->stTransfer.stSCBTag.bValid)
    {
        pstAgentNode = sc_agent_get_by_id(pstSCB->stTransfer.ulNotifyAgentID);
        if (DOS_ADDR_VALID(pstAgentNode)
            && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
        {
            dos_strcpy(szEmpNo, pstAgentNode->pstAgentInfo->szEmpNo);
        }
        else
        {
            szEmpNo[0] = '\0';
        }
    }
    else if (pstSCB->stAutoPreview.stSCBTag.bValid)
    {
        pstAgentNode = sc_agent_get_by_id(pstSCB->stAutoPreview.ulAgentID);
        if (DOS_ADDR_VALID(pstAgentNode)
            && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
        {
            dos_strcpy(szEmpNo, pstAgentNode->pstAgentInfo->szEmpNo);
        }
        else
        {
            szEmpNo[0] = '\0';
        }
    }

    /* 录音文件名 */
    sc_get_record_file_path(pstRecordRsp->szRecordFile, sizeof(pstRecordRsp->szRecordFile), pstSCB->ulCustomerID, szEmpNo, pstRecordLegCB->stCall.stNumInfo.szOriginalCalling, pstRecordLegCB->stCall.stNumInfo.szOriginalCallee);

    pstRecordRsp = NULL;
    pstRecordRsp = (SC_MSG_CMD_RECORD_ST *)dos_dmem_alloc(sizeof(SC_MSG_CMD_RECORD_ST));
    if (DOS_ADDR_INVALID(pstRecordRsp))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memcpy(pstRecordRsp, pstMsg, sizeof(SC_MSG_CMD_RECORD_ST));

    if (sc_send_command(&pstRecordRsp->stMsgTag) != DOS_SUCC)
    {
        dos_dmem_free(pstRecordRsp);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_send_cmd_record_stop(SC_MSG_TAG_ST *pstMsg)
{
    return DOS_SUCC;
}

U32 sc_send_cmd_hold(SC_MSG_TAG_ST *pstMsg)
{
    return DOS_SUCC;
}

U32 sc_send_cmd_unhold(SC_MSG_TAG_ST *pstMsg)
{
    return DOS_SUCC;
}

U32 sc_send_cmd_ivr(SC_MSG_TAG_ST *pstMsg)
{
    return DOS_SUCC;
}

U32 sc_send_cmd_mux(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_CMD_MUX_ST *pstCMDMux = NULL;
    U32 ulRet;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCMDMux = (SC_MSG_CMD_MUX_ST *)dos_dmem_alloc(sizeof(SC_MSG_CMD_MUX_ST));
    if (DOS_ADDR_INVALID(pstCMDMux))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstCMDMux, pstMsg, sizeof(SC_MSG_CMD_MUX_ST));

    ulRet = sc_send_command(&pstCMDMux->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstCMDMux);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_send_cmd_transfer(SC_MSG_TAG_ST *pstMsg)
{
    SC_MSG_CMD_TRANSFER_ST *pstCMDTransfer = NULL;
    U32 ulRet;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCMDTransfer = (SC_MSG_CMD_TRANSFER_ST *)dos_dmem_alloc(sizeof(SC_MSG_CMD_TRANSFER_ST));
    if (DOS_ADDR_INVALID(pstCMDTransfer))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstCMDTransfer, pstMsg, sizeof(SC_MSG_CMD_TRANSFER_ST));

    ulRet = sc_send_command(&pstCMDTransfer->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstCMDTransfer);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_send_cmd_manage(U32 ulType)
{
    SC_MSG_CMD_MANAGE_ST *pstCmd = NULL;
    U32 ulRet;

    if (ulType >= SC_CMD_TYPE_MANAGE_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCmd = (SC_MSG_CMD_MANAGE_ST *)dos_dmem_alloc(sizeof(SC_MSG_CMD_MANAGE_ST));
    if (DOS_ADDR_INVALID(pstCmd))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    pstCmd->stMsgTag.ulMsgType = SC_CMD_MANAGE;
    pstCmd->stMsgTag.usInterErr = 0;
    pstCmd->ulType = ulType;

    ulRet = sc_send_command(&pstCmd->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstCmd);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 向业务子层发送放音命令，正对单个声音文件
 *
 * @param U32 ulSCBNo
 * @param U32 ulLegNo
 * @param U32 ulSndInd
 * @param U32 ulLoop
 * @param U32 ulInterval
 * @param U32 ulSilence
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_req_play_sound(U32 ulSCBNo, U32 ulLegNo, U32 ulSndInd, U32 ulLoop, U32 ulInterval, U32 ulSilence)
{
    SC_MSG_CMD_PLAYBACK_ST  *pstCMDPlayback;
    U32                     ulRet;

    pstCMDPlayback = (SC_MSG_CMD_PLAYBACK_ST *)dos_dmem_alloc(sizeof(SC_MSG_CMD_PLAYBACK_ST));
    if (DOS_ADDR_INVALID(pstCMDPlayback))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstCMDPlayback->stMsgTag.ulMsgType = SC_CMD_PLAYBACK;
    pstCMDPlayback->stMsgTag.ulSCBNo = ulSCBNo;
    pstCMDPlayback->stMsgTag.usInterErr = 0;

    pstCMDPlayback->ulMode = 0;
    pstCMDPlayback->ulSCBNo = ulSCBNo;
    pstCMDPlayback->ulLegNo = ulLegNo;
    pstCMDPlayback->aulAudioList[0] = ulSndInd;
    pstCMDPlayback->ulLoopCnt = ulLoop;
    pstCMDPlayback->ulTotalAudioCnt = 1;
    pstCMDPlayback->ulInterval = ulInterval;
    pstCMDPlayback->ulSilence = ulSilence;
    pstCMDPlayback->enType = SC_CND_PLAYBACK_SYSTEM;
    pstCMDPlayback->blNeedDTMF = DOS_FALSE;

    ulRet = sc_send_command(&pstCMDPlayback->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstCMDPlayback);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 向业务子层发送放音命令，正对多个声音文件
 *
 * @param U32 ulSCBNo
 * @param U32 ulLegNo
 * @param U32 *pulSndInd
 * @param U32 ulSndCnt
 * @param U32 ulLoop
 * @param U32 ulInterval
 * @param U32 ulSilence
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_req_play_sounds(U32 ulSCBNo, U32 ulLegNo, U32 *pulSndInd, U32 ulSndCnt, U32 ulLoop, U32 ulInterval, U32 ulSilence)
{
    SC_MSG_CMD_PLAYBACK_ST  *pstCMDPlayback;
    U32                     ulRet;
    U32                     i;

    pstCMDPlayback = (SC_MSG_CMD_PLAYBACK_ST *)dos_dmem_alloc(sizeof(SC_MSG_CMD_PLAYBACK_ST));
    if (DOS_ADDR_INVALID(pstCMDPlayback))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstCMDPlayback->stMsgTag.ulMsgType = SC_CMD_PLAYBACK;
    pstCMDPlayback->stMsgTag.ulSCBNo = ulSCBNo;
    pstCMDPlayback->stMsgTag.usInterErr = 0;

    pstCMDPlayback->ulMode = 0;
    pstCMDPlayback->ulSCBNo = ulSCBNo;
    pstCMDPlayback->ulLegNo = ulLegNo;

    pstCMDPlayback->ulTotalAudioCnt = 0;
    for (i=0; i<ulSndCnt; i++)
    {
        pstCMDPlayback->aulAudioList[i] = pulSndInd[i];
        pstCMDPlayback->ulTotalAudioCnt++;
    }

    pstCMDPlayback->ulLoopCnt = ulLoop;
    pstCMDPlayback->ulInterval = ulInterval;
    pstCMDPlayback->ulSilence = ulSilence;

    ulRet = sc_send_command(&pstCMDPlayback->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstCMDPlayback);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 向业务子层发送放音结束命令
 *
 * @param U32 ulSCBNo
 * @param U32 ulLegNo
 * @param U32 ulSndInd
 * @param U32 ulLoop
 * @param U32 ulInterval
 * @param U32 ulSilence
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_req_playback_stop(U32 ulSCBNo, U32 ulLegNo)
{
    SC_MSG_CMD_PLAYBACK_ST  *pstCMDPlayback;
    U32                     ulRet;

    pstCMDPlayback = (SC_MSG_CMD_PLAYBACK_ST *)dos_dmem_alloc(sizeof(SC_MSG_CMD_PLAYBACK_ST));
    if (DOS_ADDR_INVALID(pstCMDPlayback))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstCMDPlayback->stMsgTag.ulMsgType = SC_CMD_PLAYBACK_STOP;
    pstCMDPlayback->stMsgTag.ulSCBNo = ulSCBNo;
    pstCMDPlayback->stMsgTag.usInterErr = 0;

    pstCMDPlayback->ulMode = 0;
    pstCMDPlayback->ulSCBNo = ulSCBNo;
    pstCMDPlayback->ulLegNo = ulLegNo;

    ulRet = sc_send_command(&pstCMDPlayback->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstCMDPlayback);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}


/**
 * 向业务子层发送挂断命令
 *
 * @param U32 ulSCBNo
 * @param U32 ulLegNo
 * @param U32 ulErrNo
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_req_hungup(U32 ulSCBNo, U32 ulLegNo, U32 ulErrNo)
{
    SC_MSG_CMD_HUNGUP_ST   *pstCMDHungup = NULL;
    U32                    ulRet;

    pstCMDHungup = (SC_MSG_CMD_HUNGUP_ST *)dos_dmem_alloc(sizeof(SC_MSG_CMD_HUNGUP_ST));
    if (DOS_ADDR_INVALID(pstCMDHungup))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstCMDHungup->stMsgTag.ulMsgType = SC_CMD_RELEASE_CALL;
    pstCMDHungup->stMsgTag.ulSCBNo = ulSCBNo;
    pstCMDHungup->stMsgTag.usInterErr = 0;
    pstCMDHungup->ulLegNo = ulLegNo;
    pstCMDHungup->ulSCBNo = ulSCBNo;
    pstCMDHungup->ulErrCode = ulErrNo;

    ulRet = sc_send_command(&pstCMDHungup->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstCMDHungup);

        sc_log(LOG_LEVEL_ERROR, "Send hungup request fail. SCB: %u, LCB: %u, Error: %u", ulSCBNo, ulLegNo, ulErrNo);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 向业务子层发送挂断命令，在这之前给当前LEG放音
 *
 * @param U32 ulSCBNo
 * @param U32 ulLegNo
 * @param U32 ulErrNo
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_req_hungup_with_sound(U32 ulSCBNo, U32 ulLegNo, U32 ulErrNo)
{
    U32 ulSoundIndex = U32_BUTT;

    switch (ulErrNo)
    {
        case CC_ERR_NO_REASON:
            ulSoundIndex = U32_BUTT;
            break;

        case CC_ERR_SC_IN_BLACKLIST:
        case CC_ERR_SC_CALLER_NUMBER_ILLEGAL:
        case CC_ERR_SC_CALLEE_NUMBER_ILLEGAL:
            ulSoundIndex = SC_SND_USER_NOT_FOUND;
            break;

        case CC_ERR_SC_USER_OFFLINE:
        case CC_ERR_SC_USER_HAS_BEEN_LEFT:
        case CC_ERR_SC_PERIOD_EXCEED:
        case CC_ERR_SC_RESOURCE_EXCEED:
            ulSoundIndex = SC_SND_TMP_UNAVAILABLE;
            break;

        case CC_ERR_SC_USER_BUSY:
            ulSoundIndex = SC_SND_USER_BUSY;
            break;

        case CC_ERR_SC_NO_ROUTE:
        case CC_ERR_SC_NO_TRUNK:
            ulSoundIndex = SC_SND_NETWORK_FAULT;
            break;

        case CC_ERR_BS_LACK_FEE:
            ulSoundIndex = SC_SND_LOW_BALANCE;
            break;

        case CC_ERR_SC_SYSTEM_BUSY:
            ulSoundIndex = SC_SND_TMP_UNAVAILABLE;
            break;

        default:
            ulSoundIndex = SC_SND_TMP_UNAVAILABLE;
            break;
    }

    if (U32_BUTT != ulSoundIndex)
    {
        /* 如果请求放音失败了，也要发送挂断请求 */
        sc_req_play_sound(ulSCBNo, ulLegNo, ulSoundIndex, SC_CALL_HIT_LOOP, SC_CALL_HIT_INTERVAL, SC_CALL_HIT_SILENCE);
    }

    return sc_req_hungup(ulSCBNo, ulLegNo, ulErrNo);
}

/**
 * 向业务子层发桥接LEG请求
 *
 * @param U32 ulSCBNo
 * @param U32 ulCallingLegNo
 * @param U32 ulCalleeLegNo
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_req_bridge_call(U32 ulSCBNo, U32 ulCallingLegNo, U32 ulCalleeLegNo)
{
    SC_MSG_CMD_BRIDGE_ST  *pstCMDBridge = NULL;
    U32                   ulRet = 0;

    pstCMDBridge = (SC_MSG_CMD_BRIDGE_ST *)dos_dmem_alloc(sizeof(SC_MSG_CMD_BRIDGE_ST));
    if (DOS_ADDR_INVALID(pstCMDBridge))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCMDBridge->stMsgTag.ulMsgType = SC_CMD_BRIDGE_CALL;
    pstCMDBridge->stMsgTag.ulSCBNo = ulSCBNo;
    pstCMDBridge->stMsgTag.usInterErr = 0;
    pstCMDBridge->ulSCBNo = ulSCBNo;
    pstCMDBridge->ulCallingLegNo = ulCallingLegNo;
    pstCMDBridge->ulCalleeLegNo = ulCalleeLegNo;

    ulRet = sc_send_command(&pstCMDBridge->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstCMDBridge);
        pstCMDBridge = NULL;
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 向业务子层发ANSWER命令
 *
 * @param U32 ulSCBNo
 * @param U32 ulLegNo
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_req_answer_call(U32 ulSCBNo, U32 ulLegNo)
{
    SC_MSG_CMD_ANSWER_ST *pstCMDAnswer = NULL;
    U32                   ulRet = 0;

    pstCMDAnswer = (SC_MSG_CMD_ANSWER_ST *)dos_dmem_alloc(sizeof(SC_MSG_CMD_ANSWER_ST));
    if (DOS_ADDR_INVALID(pstCMDAnswer))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCMDAnswer->stMsgTag.ulMsgType = SC_CMD_ANSWER_CALL;
    pstCMDAnswer->stMsgTag.ulSCBNo = ulSCBNo;
    pstCMDAnswer->stMsgTag.usInterErr = 0;
    pstCMDAnswer->ulSCBNo = ulSCBNo;
    pstCMDAnswer->ulLegNo = ulLegNo;

    ulRet = sc_send_command(&pstCMDAnswer->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstCMDAnswer);
        pstCMDAnswer = NULL;
        return DOS_FAIL;
    }

    return DOS_SUCC;
}


/**
 * 向业务子层发RINGBACK命令
 *
 * @param U32 ulSCBNo
 * @param U32 ulLegNo
 * @param BOOL blHasMedia
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_req_ringback(U32 ulSCBNo, U32 ulLegNo, BOOL bIsConnected, BOOL bIsEarlyMedia)
{
    SC_MSG_CMD_RINGBACK_ST *pstCMDRingback = NULL;
    U32                    ulRet = 0;

    pstCMDRingback = (SC_MSG_CMD_RINGBACK_ST*)dos_dmem_alloc(sizeof(SC_MSG_CMD_RINGBACK_ST));
    if (DOS_ADDR_INVALID(pstCMDRingback))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCMDRingback->stMsgTag.ulMsgType = SC_CMD_RINGBACK;
    pstCMDRingback->stMsgTag.ulSCBNo = ulSCBNo;
    pstCMDRingback->stMsgTag.usInterErr = 0;
    pstCMDRingback->ulSCBNo = ulSCBNo;
    pstCMDRingback->ulLegNo = ulLegNo;
    pstCMDRingback->ulCallConnected = bIsConnected;
    pstCMDRingback->ulEarlyMedia = bIsEarlyMedia;

    ulRet = sc_send_command(&pstCMDRingback->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstCMDRingback);
        pstCMDRingback = NULL;
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 向业务子层发RINGBACK命令
 *
 * @param U32 ulSCBNo
 * @param U32 ulLegNo
 * @param BOOL blHasMedia
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_req_hold(U32 ulSCBNo, U32 ulLegNo, U32 ulFlag)
{
    SC_MSG_CMD_HOLD_ST  *pstCMDHold = NULL;
    U32                    ulRet = 0;

    pstCMDHold = (SC_MSG_CMD_HOLD_ST*)dos_dmem_alloc(sizeof(SC_MSG_CMD_HOLD_ST));
    if (DOS_ADDR_INVALID(pstCMDHold))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (ulFlag == SC_HOLD_FLAG_HOLD)
    {
        pstCMDHold->stMsgTag.ulMsgType = SC_CMD_HOLD;
    }
    else
    {
        pstCMDHold->stMsgTag.ulMsgType = SC_CMD_UNHOLD;
    }
    pstCMDHold->stMsgTag.ulSCBNo = ulSCBNo;
    pstCMDHold->stMsgTag.usInterErr = 0;
    pstCMDHold->ulSCBNo = ulSCBNo;
    pstCMDHold->ulLegNo = ulLegNo;
    pstCMDHold->ulFlag = ulFlag;

    ulRet = sc_send_command(&pstCMDHold->stMsgTag);
    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstCMDHold);
        pstCMDHold = NULL;
        return DOS_FAIL;
    }

    return DOS_SUCC;
}


/**
 * 向业务层发送事件
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_event(SC_MSG_TAG_ST *pstMsg)
{
    DLL_NODE_S   *pstDLLNode = NULL;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstDLLNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstMsg);
        return DOS_FAIL;
    }

    DLL_Init_Node(pstDLLNode);
    pstDLLNode->pHandle = pstMsg;

    pthread_mutex_lock(&g_mutexEventQueue);
    DLL_Add(&g_stEventQueue, pstDLLNode);
    pthread_cond_signal(&g_condEventQueue);
    pthread_mutex_unlock(&g_mutexEventQueue);

    sc_log(LOG_LEVEL_DEBUG, "Send %s(%u). SCB: %u, Error: %u"
                , sc_event_str(pstMsg->ulMsgType), pstMsg->ulMsgType
                , pstMsg->ulSCBNo, pstMsg->usInterErr);

    return DOS_SUCC;
}

/**
 * 向业务层发送呼叫创建事件
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_event_call_create(SC_MSG_EVT_CALL_ST *pstEvent)
{
    SC_MSG_EVT_CALL_ST *pstEventCall = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEventCall = (SC_MSG_EVT_CALL_ST *)dos_dmem_alloc(sizeof(SC_MSG_EVT_CALL_ST));
    if (DOS_ADDR_INVALID(pstEventCall))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstEventCall, pstEvent, sizeof(SC_MSG_EVT_CALL_ST));

    return sc_send_event(&pstEventCall->stMsgTag);
}

/**
 * 向业务层发送应答事件
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_event_ringing(SC_MSG_EVT_RINGING_ST *pstEvent)
{
    SC_MSG_EVT_RINGING_ST *pstEventRinging = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEventRinging = (SC_MSG_EVT_RINGING_ST *)dos_dmem_alloc(sizeof(SC_MSG_EVT_RINGING_ST));
    if (DOS_ADDR_INVALID(pstEventRinging))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstEventRinging, pstEvent, sizeof(SC_MSG_EVT_RINGING_ST));

    return sc_send_event(&pstEventRinging->stMsgTag);
}

/**
 * 向业务层发送应答事件
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_event_answer(SC_MSG_EVT_ANSWER_ST *pstEvent)
{
    SC_MSG_EVT_ANSWER_ST *pstEventAnswer = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEventAnswer = (SC_MSG_EVT_ANSWER_ST *)dos_dmem_alloc(sizeof(SC_MSG_EVT_ANSWER_ST));
    if (DOS_ADDR_INVALID(pstEventAnswer))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstEventAnswer, pstEvent, sizeof(SC_MSG_EVT_ANSWER_ST));

    return sc_send_event(&pstEventAnswer->stMsgTag);
}


/**
 * 向业务层发送呼叫释放事件
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_event_release(SC_MSG_EVT_HUNGUP_ST *pstEvent)
{
    SC_MSG_EVT_HUNGUP_ST *pstEventHungup = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEventHungup = (SC_MSG_EVT_HUNGUP_ST *)dos_dmem_alloc(sizeof(SC_MSG_EVT_HUNGUP_ST));
    if (DOS_ADDR_INVALID(pstEventHungup))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstEventHungup, pstEvent, sizeof(SC_MSG_EVT_HUNGUP_ST));

    return sc_send_event(&pstEventHungup->stMsgTag);
}

/**
 * 向业务层发送呼叫变化事件
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_event_status()
{
    return DOS_SUCC;
}

/**
 * 向业务层发送DTMF事件
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_event_dtmf(SC_MSG_EVT_DTMF_ST *pstEvent)
{
    SC_MSG_EVT_DTMF_ST *pstEventDTMF = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEventDTMF = (SC_MSG_EVT_DTMF_ST *)dos_dmem_alloc(sizeof(SC_MSG_EVT_DTMF_ST));
    if (DOS_ADDR_INVALID(pstEventDTMF))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstEventDTMF, pstEvent, sizeof(SC_MSG_EVT_DTMF_ST));

    return sc_send_event(&pstEventDTMF->stMsgTag);
}

/**
 * 向业务层发送录音业务状态变化事件
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_event_record(SC_MSG_EVT_RECORD_ST *pstEvent)
{
    SC_MSG_EVT_RECORD_ST *pstEventRecord = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEventRecord = (SC_MSG_EVT_RECORD_ST *)dos_dmem_alloc(sizeof(SC_MSG_EVT_RECORD_ST));
    if (DOS_ADDR_INVALID(pstEventRecord))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstEventRecord, pstEvent, sizeof(SC_MSG_EVT_RECORD_ST));

    return sc_send_event(&pstEventRecord->stMsgTag);
}

/**
 * 向业务层发送PLAYBACK业务状态变化事件
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_event_playback(SC_MSG_EVT_PLAYBACK_ST *pstEvent)
{
    SC_MSG_EVT_PLAYBACK_ST *pstEventPlayback = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEventPlayback = (SC_MSG_EVT_PLAYBACK_ST *)dos_dmem_alloc(sizeof(SC_MSG_EVT_PLAYBACK_ST));
    if (DOS_ADDR_INVALID(pstEventPlayback))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstEventPlayback, pstEvent, sizeof(SC_MSG_EVT_PLAYBACK_ST));

    return sc_send_event(&pstEventPlayback->stMsgTag);
}

/**
 * 向业务层发送HOLD业务状态变化事件
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_event_hold(SC_MSG_EVT_HOLD_ST *pstEvent)
{
    SC_MSG_EVT_HOLD_ST *pstEventHold = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEventHold = (SC_MSG_EVT_HOLD_ST *)dos_dmem_alloc(sizeof(SC_MSG_EVT_HOLD_ST));
    if (DOS_ADDR_INVALID(pstEventHold))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstEventHold, pstEvent, sizeof(SC_MSG_EVT_HOLD_ST));

    return sc_send_event(&pstEventHold->stMsgTag);
}

/**
 * 向业务层发送错误上报事件
 *
 * @param pstMsg 消息头
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * @note pstMsg 所指向的内存将被别的线程使用，请动态分配
 */
U32 sc_send_event_err_report(SC_MSG_EVT_ERR_REPORT_ST *pstEvent)
{
    SC_MSG_EVT_ERR_REPORT_ST *pstErrInfo = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstErrInfo = (SC_MSG_EVT_ERR_REPORT_ST *)dos_dmem_alloc(sizeof(SC_MSG_EVT_ERR_REPORT_ST));
    if (DOS_ADDR_INVALID(pstErrInfo))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstErrInfo, pstEvent, sizeof(SC_MSG_EVT_ERR_REPORT_ST));

    return sc_send_event(&pstErrInfo->stMsgTag);
}

U32 sc_send_event_auth_rsp(SC_MSG_EVT_AUTH_RESULT_ST *pstEvent)
{
    SC_MSG_EVT_AUTH_RESULT_ST *pstAuthInfo = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstAuthInfo = (SC_MSG_EVT_AUTH_RESULT_ST *)dos_dmem_alloc(sizeof(SC_MSG_EVT_AUTH_RESULT_ST));
    if (DOS_ADDR_INVALID(pstAuthInfo))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstAuthInfo, pstEvent, sizeof(SC_MSG_EVT_AUTH_RESULT_ST));

    return sc_send_event(&pstAuthInfo->stMsgTag);

}

U32 sc_send_event_leave_call_queue_rsp(SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvent)
{
    SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvtLeaveCallque = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEvtLeaveCallque = (SC_MSG_EVT_LEAVE_CALLQUE_ST *)dos_dmem_alloc(sizeof(SC_MSG_EVT_LEAVE_CALLQUE_ST));
    if (DOS_ADDR_INVALID(pstEvtLeaveCallque))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstEvtLeaveCallque, pstEvent, sizeof(SC_MSG_EVT_LEAVE_CALLQUE_ST));

    return sc_send_event(&pstEvtLeaveCallque->stMsgTag);

}

U32 sc_send_event_ringing_timeout_rsp(SC_MSG_EVT_RINGING_TIMEOUT_ST *pstEvent)
{
    SC_MSG_EVT_RINGING_TIMEOUT_ST *pstEvtRingingTimeout = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEvtRingingTimeout = (SC_MSG_EVT_RINGING_TIMEOUT_ST *)dos_dmem_alloc(sizeof(SC_MSG_EVT_RINGING_TIMEOUT_ST));
    if (DOS_ADDR_INVALID(pstEvtRingingTimeout))
    {
        sc_log(LOG_LEVEL_ERROR, "Send event fail.");
        return DOS_FAIL;
    }

    dos_memcpy(pstEvtRingingTimeout, pstEvent, sizeof(SC_MSG_EVT_RINGING_TIMEOUT_ST));

    return sc_send_event(&pstEvtRingingTimeout->stMsgTag);

}

U32 sc_leg_get_source(SC_SRV_CB *pstSCB, SC_LEG_CB  *pstLegCB)
{
    SC_AGENT_NODE_ST *pstAgent = NULL;

    if (DOS_ADDR_INVALID(pstLegCB) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return SC_DIRECTION_INVALID;
    }

    if (SC_LEG_PEER_INBOUND_INTERNAL == pstLegCB->stCall.ucPeerType)
    {
        pstSCB->ulCustomerID = sc_sip_account_get_customer(pstLegCB->stCall.stNumInfo.szOriginalCalling);
        if (pstSCB->ulCustomerID != U32_BUTT)
        {
            pstAgent = sc_agent_get_by_sip_acc(pstLegCB->stCall.stNumInfo.szOriginalCalling);
            if (DOS_ADDR_VALID(pstAgent)
                && DOS_ADDR_VALID(pstAgent->pstAgentInfo)
                && pstAgent->pstAgentInfo->ucBindType == AGENT_BIND_SIP)
            {
                pstSCB->ulAgentID = pstAgent->pstAgentInfo->ulAgentID;
                pstSCB->stCall.pstAgentCalling = pstAgent;
                return SC_DIRECTION_SIP;
            }

            return SC_DIRECTION_SIP;
        }

        return SC_DIRECTION_INVALID;
    }
    else if (SC_LEG_PEER_INBOUND == pstLegCB->stCall.ucPeerType)
    {
        /* external可能是TT号呼入哦 */
        pstSCB->ulCustomerID = sc_did_get_custom(pstLegCB->stCall.stNumInfo.szOriginalCallee);
        if (U32_BUTT == pstSCB->ulCustomerID)
        {
            pstAgent = sc_agent_get_by_tt_num(pstLegCB->stCall.stNumInfo.szOriginalCalling);
            if (DOS_ADDR_VALID(pstAgent) && DOS_ADDR_VALID(pstAgent->pstAgentInfo))
            {
                pstSCB->ulCustomerID = pstAgent->pstAgentInfo->ulCustomerID;
                pstSCB->ulAgentID = pstAgent->pstAgentInfo->ulAgentID;
                pstSCB->stCall.pstAgentCalling = pstAgent;
                pstLegCB->stCall.bIsTTCall = DOS_TRUE;
                pstLegCB->stCall.ucPeerType = SC_LEG_PEER_INBOUND_TT;
                return SC_DIRECTION_SIP;
            }
        }

        return SC_DIRECTION_PSTN;
    }

    return SC_DIRECTION_PSTN;
}

U32 sc_leg_get_destination(SC_SRV_CB *pstSCB, SC_LEG_CB  *pstLegCB)
{
    U32 ulCustomID    = U32_BUTT;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        return SC_DIRECTION_INVALID;
    }

    if (!sc_black_list_check(pstSCB->ulCustomerID, pstLegCB->stCall.stNumInfo.szOriginalCallee))
    {
        sc_trace_scb(pstSCB, "The destination is in black list. %s", pstLegCB->stCall.stNumInfo.szOriginalCallee);

        return SC_DIRECTION_INVALID;
    }

    if (SC_LEG_PEER_INBOUND_INTERNAL == pstLegCB->stCall.ucPeerType)
    {
        /* 被叫号码是否是同一个客户下的SIP User ID */
        ulCustomID = sc_sip_account_get_customer(pstLegCB->stCall.stNumInfo.szOriginalCallee);
        if (ulCustomID == pstSCB->ulCustomerID)
        {
            return SC_DIRECTION_SIP;
        }

        /*  测试被叫是否是分机号 */
        if (sc_sip_account_extension_check(pstLegCB->stCall.stNumInfo.szOriginalCallee, pstSCB->ulCustomerID))
        {
            return SC_DIRECTION_SIP;
        }

        return SC_DIRECTION_PSTN;
    }
    else if (SC_LEG_PEER_INBOUND_TT == pstLegCB->stCall.ucPeerType)
    {
        /* 现在 TT号，只支持呼出 */
        return SC_DIRECTION_PSTN;
    }
    else
    {
        if (U32_BUTT == pstSCB->ulCustomerID)
        {
            DOS_ASSERT(0);

            sc_trace_scb(pstSCB, "The destination %s is not a DID number. Reject Call.", pstLegCB->stCall.stNumInfo.szOriginalCallee);
            return SC_DIRECTION_INVALID;
        }

        return SC_DIRECTION_SIP;
    }

    return SC_DIRECTION_PSTN;
}

/**
 * 获取语音文件文件名列表
 *
 * @param  *pulSndIndList
 * @param  ulCnt
 * @param *pszBuffer,
 * @param  ulBuffLen
 * @param  pszSep, 如果为NULL，将会使用默认值 "!"
 *
 * @return 成功返回录音文件个数， 失败返回0
 */
U32 sc_get_snd_list(U32 *pulSndIndList, U32 ulSndCnt, S8 *pszBuffer, U32 ulBuffLen, S8 *pszSep)
{
    U32   ulLen = 0;
    U32   ulLoop = 0;
    U32   ulCnt = 0;
    S8    *pszName = NULL;
    S8    *pszSepReal = NULL;

    if (DOS_ADDR_INVALID(pulSndIndList) || 0 == ulSndCnt)
    {
        DOS_ASSERT(0);
        return 0;
    }

    if (DOS_ADDR_INVALID(pszBuffer) || 0 == ulBuffLen)
    {
        DOS_ASSERT(0);
        return 0;
    }

    if (DOS_ADDR_INVALID(pszSep))
    {
        pszSepReal = "!";
    }

    for (ulLen=0, ulLoop=0, ulCnt=0; ulLoop<ulSndCnt; ulLoop++)
    {
        pszName = sc_hint_get_name(pulSndIndList[ulLoop]);
        if (DOS_ADDR_INVALID(pszName) || '\0' == pszName[0])
        {
            DOS_ASSERT(0);
            continue;
        }

        if (ulBuffLen - ulLen < dos_strlen(SC_HINT_FILE_PATH) + dos_strlen(pszName) + dos_strlen(pszSepReal))
        {
            ulCnt = 0;

            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Buffer not enought.");
            break;
        }

        ulLen += dos_snprintf(pszBuffer + ulLen, ulBuffLen - ulLen
                                , "%s/%s.wav%s"
                                , SC_HINT_FILE_PATH
                                , pszName
                                , pszSepReal);

        ulCnt++;
    }

    pszBuffer[ulLen - 1] = '\0';

    return ulCnt;
}

void sc_agent_mark_custom_callback(U64 arg)
{
    U32                 ulLcbNo    = U32_BUTT;
    SC_LEG_CB           *pstLeg    = NULL;
    SC_SRV_CB           *pstSCB    = NULL;

    ulLcbNo = (U32)arg;

    pstLeg = sc_lcb_get(ulLcbNo);
    if (DOS_ADDR_INVALID(pstLeg))
    {
        DOS_ASSERT(0);
        return;
    }

    pstSCB = sc_scb_get(pstLeg->ulSCBNo);
    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        sc_lcb_free(pstLeg);
        return;
    }

    /* 停止放音 */
    sc_req_playback_stop(pstSCB->ulSCBNo, pstLeg->ulCBNo);

    /* 判断坐席是否是长签，如果不是则挂断电话 */
    if (DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall))
    {
        sc_agent_serv_status_update(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo, SC_ACD_SERV_IDEL, SC_SRV_MARK_CUSTOM);
    }

    if (pstLeg->ulIndSCBNo != U32_BUTT)
    {
        /* 长签，继续放音 */
        pstLeg->ulSCBNo = U32_BUTT;
        sc_req_play_sound(pstLeg->ulIndSCBNo, pstLeg->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
        /* 释放掉 SCB */
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }
    else
    {
        sc_req_hungup(pstSCB->ulSCBNo, pstLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
        pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_ACTIVE;
    }

    return;
}

void sc_agent_ringing_timeout_callback(U64 arg)
{
    U32                 ulLCBNo    = U32_BUTT;
    SC_LEG_CB           *pstLeg    = NULL;
    SC_SRV_CB           *pstSCB    = NULL;
    SC_MSG_EVT_RINGING_TIMEOUT_ST stEvtRingingTimeOut;

    ulLCBNo = (U32)arg;

    pstLeg = sc_lcb_get(ulLCBNo);
    if (DOS_ADDR_INVALID(pstLeg))
    {
        return;
    }

    pstSCB = sc_scb_get(pstLeg->ulSCBNo);
    if (DOS_ADDR_INVALID(pstSCB))
    {
        return;
    }

    if (pstSCB->stCall.stSCBTag.usStatus == SC_CALL_ALERTING
        || pstSCB->stAutoCall.stSCBTag.usStatus == SC_AUTO_CALL_ALERTING2
        || pstSCB->stDemoTask.stSCBTag.usStatus == SC_AUTO_CALL_ALERTING2)
    {
        /* 发送超时提醒给fsm */
        stEvtRingingTimeOut.stMsgTag.ulMsgType = SC_EVT_RINGING_TIMEOUT;
        stEvtRingingTimeOut.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
        stEvtRingingTimeOut.ulSCBNo = pstSCB->ulSCBNo;
        stEvtRingingTimeOut.ulLCBNo = ulLCBNo;
        if (sc_send_event_ringing_timeout_rsp(&stEvtRingingTimeOut) != DOS_SUCC)
        {
            /* TODO 发送消息失败 */
        }
    }

    return;
}


U32 sc_stat_syn(U32 ulType, VOID *ptr)
{
    SC_SYS_STAT_ST       stSysStat;

    dos_memcpy((VOID *)&stSysStat, (VOID *)&g_stSysStat, sizeof(SC_SYS_STAT_ST));
    dos_memzero((VOID *)&stSysStat, sizeof(SC_SYS_STAT_ST));

    if (U32_BUTT - g_stSysStatLocal.ulCurrentCalls > stSysStat.ulCurrentCalls)
    {
        g_stSysStatLocal.ulCurrentCalls       += stSysStat.ulCurrentCalls;
    }
    if (U32_BUTT - g_stSysStatLocal.ulIncomingCalls > stSysStat.ulIncomingCalls)
    {
        g_stSysStatLocal.ulIncomingCalls      += stSysStat.ulIncomingCalls;
    }
    if (U32_BUTT - g_stSysStatLocal.ulOutgoingCalls > stSysStat.ulOutgoingCalls)
    {
        g_stSysStatLocal.ulOutgoingCalls      += stSysStat.ulOutgoingCalls;
    }
    if (U32_BUTT - g_stSysStatLocal.ulTotalTime > stSysStat.ulTotalTime)
    {
        g_stSysStatLocal.ulTotalTime          += stSysStat.ulTotalTime;
    }
    if (U32_BUTT - g_stSysStatLocal.ulOutgoingTime > stSysStat.ulOutgoingTime)
    {
        g_stSysStatLocal.ulOutgoingTime       += stSysStat.ulOutgoingTime;
    }
    if (U32_BUTT - g_stSysStatLocal.ulIncomingTime > stSysStat.ulIncomingTime)
    {
        g_stSysStatLocal.ulIncomingTime       += stSysStat.ulIncomingTime;
    }
    if (U32_BUTT - g_stSysStatLocal.ulAutoCallTime > stSysStat.ulAutoCallTime)
    {
        g_stSysStatLocal.ulAutoCallTime       += stSysStat.ulAutoCallTime;
    }
    if (U32_BUTT - g_stSysStatLocal.ulPreviewCallTime > stSysStat.ulPreviewCallTime)
    {
        g_stSysStatLocal.ulPreviewCallTime    += stSysStat.ulPreviewCallTime;
    }
    if (U32_BUTT - g_stSysStatLocal.ulPredictiveCallTime > stSysStat.ulPredictiveCallTime)
    {
        g_stSysStatLocal.ulPredictiveCallTime += stSysStat.ulPredictiveCallTime;
    }
    if (U32_BUTT - g_stSysStatLocal.ulInternalCallTime > stSysStat.ulInternalCallTime)
    {
        g_stSysStatLocal.ulInternalCallTime   += stSysStat.ulInternalCallTime;
    }

    licc_set_srv_stat(LIC_TOTAL_CALLTIME, stSysStat.ulTotalTime);
    licc_set_srv_stat(LIC_OUTBOUND_CALLTIME, stSysStat.ulOutgoingTime);
    licc_set_srv_stat(LIC_INBOUND_CALLTIME, stSysStat.ulIncomingTime);
    licc_set_srv_stat(LIC_AUTO_CALLTIME, stSysStat.ulAutoCallTime);

    sc_log(LOG_LEVEL_INFO, "%s", "Stat data syn");

    return DOS_SUCC;
}

U32 sc_stat_write(U32 ulType, VOID *ptr)
{
    return DOS_SUCC;
}

U32 sc_get_call_limitation()
{
    U32 ulLimitation;

    if (licc_get_limitation(LIC_TOTAL_CALLS, &ulLimitation) != DOS_SUCC)
    {
        if (dos_get_default_limitation(LIC_TOTAL_CALLS, &ulLimitation) != DOS_SUCC)
        {
            ulLimitation = 0;

            sc_log(LOG_LEVEL_INFO, "Get license limitation fail.. Use default: %u", ulLimitation);
            return DOS_FAIL;
        }

        sc_log(LOG_LEVEL_INFO, "Get license limitation fail. Use default: %u", ulLimitation);
    }

    return ulLimitation;
}

U32 sc_send_client_contect_req(U32 ulCustomerID, U32 ulClientID, S8 *pszNumber, BOOL blCallContected)
{
    S8 szURL[256]      = { 0, };
    S8 szData[512]     = { 0, };
    S32 ulPAPIPort     = -1;

    ulPAPIPort = config_hb_get_papi_port();
    if (ulPAPIPort <= 0)
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost/index.php/papi");
    }
    else
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost:%d/index.php/papi", ulPAPIPort);
    }

    /* 格式中引号前面需要添加"\",提供给push stream做转义用 */
    dos_snprintf(szData, sizeof(szData), "data={\"type\":\"%u\", \"data\":{\"customer_id\":\"%u\", \"client_id\":\"%u\", \"is_contact\":\"%s\", \"number\":\"%s\"}}"
                    , SC_PUB_TYPE_CLIENT
                    , ulCustomerID, ulClientID
                    , blCallContected ? "true" : "false"
                    , pszNumber);

    return sc_pub_send_msg(szURL, szData, SC_PUB_TYPE_MARKER, NULL);
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */


