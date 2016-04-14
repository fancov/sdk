
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_db.h"
#include "sc_res.h"

extern DB_HANDLE_ST         *g_pstSCDBHandle;

/* 主叫号码限制hash表 refer to SC_NUMBER_LMT_NODE_ST */
//HASH_TABLE_S             *g_pstHashNumberlmt = NULL;
//pthread_mutex_t          g_mutexHashNumberlmt = PTHREAD_MUTEX_INITIALIZER;

/* 使用次数统计，数据链表 */
DLL_S    g_stLimitStatQueue;

static pthread_t        g_pthreadLimit;
static pthread_mutex_t  g_mutexLimit = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   g_condLimit  = PTHREAD_COND_INITIALIZER;


U32 sc_save_number_stat(U32 ulCustomerID, U32 ulType, S8 *pszNumber, U32 ulTimes)
{
    S8                  szSQL[512]  = { 0 };
    SC_DB_MSG_TAG_ST    *pstMsg     = NULL;

    dos_snprintf(szSQL, sizeof(szSQL)
                    , "INSERT INTO tbl_stat_caller(customer_id, type, ctime, caller, times) VALUE(%u, %u, %u, \"%s\", %u)"
                    , ulCustomerID, ulType, time(NULL), pszNumber, ulTimes);

    pstMsg = (SC_DB_MSG_TAG_ST *)dos_dmem_alloc(sizeof(SC_DB_MSG_TAG_ST));
    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    pstMsg->ulMsgType = SC_MSG_SAVE_STAT_LIMIT_CALLER;
    pstMsg->szData = dos_dmem_alloc(dos_strlen(szSQL) + 1);
    if (DOS_ADDR_INVALID(pstMsg->szData))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstMsg->szData);

        return DOS_FAIL;
    }

    dos_strcpy(pstMsg->szData, szSQL);

    return sc_send_msg2db(pstMsg);
}

/**
 * 函数: U32 sc_ep_number_lmt_hash_func(U32 ulCustomerID)
 * 功能: 根据号码pszNumber计算号码限制hash节点的hash值
 * 参数:
 *      U32 pszNumber 号码
 * 返回值: U32 返回hash值
 */
U32 sc_ep_number_lmt_hash_func(S8 *pszNumber)
{
    U32 ulIndex = 0;
    U32 ulHashIndex = 0;

    ulIndex = 0;
    for (;;)
    {
        if ('\0' == pszNumber[ulIndex])
        {
            break;
        }

        ulHashIndex += (pszNumber[ulIndex] << 3);

        ulIndex++;
    }

    return ulHashIndex % SC_NUMBER_LMT_HASH_SIZE;

}

S32 sc_ep_number_lmt_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_NUMBER_LMT_NODE_ST *pstNumber = NULL;
    SC_LIMIT_CALLER_STAT_ST *pstLimitCaller = NULL;

    pstLimitCaller = (SC_LIMIT_CALLER_STAT_ST *)pObj;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        return DOS_FAIL;
    }

    pstNumber = pstHashNode->pHandle;

    if (pstLimitCaller->enNumType == pstNumber->ulType
        && 0 == dos_strcmp(pstNumber->szPrefix, pstLimitCaller->szNumber))
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/* 每天执行一次 */
U32 sc_num_lmt_stat(U32 ulType, VOID *ptr)
{
    U32 ulHashIndex;
    HASH_NODE_S             *pstHashNode       = NULL;
    SC_NUMBER_LMT_NODE_ST   *pstNumLmt         = NULL;
    time_t ulCurrentTime = 0;
    struct tm stTime;
    BOOL   bClearWeek  = DOS_FALSE;
    BOOL   bClearMonth = DOS_FALSE;
    BOOL   bClearYear  = DOS_FALSE;

    /* 判断是否是周一/一日/一月一日 */
    ulCurrentTime = time(NULL);
    dos_memzero(&stTime, sizeof(stTime));
    localtime_r((time_t *)&ulCurrentTime, &stTime);

    if (stTime.tm_wday == 1)
    {
        bClearWeek = DOS_TRUE;
    }

    if (stTime.tm_mday == 1)
    {
        bClearMonth = DOS_TRUE;
    }

    if (stTime.tm_mon == 1 && stTime.tm_mday == 1)
    {
        bClearYear = DOS_TRUE;
    }

    pthread_mutex_lock(&g_mutexHashNumberlmt);

    HASH_Scan_Table(g_pstHashNumberlmt, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashNumberlmt, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstNumLmt = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstNumLmt))
            {
                continue;
            }

            switch (pstNumLmt->ulCycle)
            {
                case SC_NUMBER_LMT_CYCLE_DAY:
                    pstNumLmt->ulStatUsed = 0;
                    break;

                case SC_NUMBER_LMT_CYCLE_WEEK:
                    if (bClearWeek)
                    {
                        pstNumLmt->ulStatUsed = 0;
                    }
                    break;

                case SC_NUMBER_LMT_CYCLE_MONTH:
                    if (bClearMonth)
                    {
                        pstNumLmt->ulStatUsed = 0;
                    }
                    break;

                case SC_NUMBER_LMT_CYCLE_YEAR:
                    if (bClearYear)
                    {
                        pstNumLmt->ulStatUsed = 0;
                    }
                    break;

                default:
                    DOS_ASSERT(0);
                    break;
            }
        }
    }

    pthread_mutex_unlock(&g_mutexHashNumberlmt);

    return DOS_SUCC;
}

S32 sc_update_number_stat_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    U32 ulTimes;

    if (DOS_ADDR_INVALID(pArg)
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(aszValues[0])
        || DOS_ADDR_INVALID(aszNames[0]))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if ('\0' == aszValues[0][0]
        || '\0' == aszNames[0][0]
        || dos_strcmp(aszNames[0], "times")
        || dos_atoul(aszValues[0], &ulTimes) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    *(U32 *)pArg = ulTimes;

    return DOS_SUCC;
}

U32 sc_update_number_stat(U32 ulType, U32 ulCycle, S8 *pszNumber)
{
    U32 ulStartTimestamp;
    U32 ulTimes;
    time_t    ulTime;
    struct tm *pstTime;
    struct tm stTime;
    struct tm stStartTime;
    S8        szSQL[512];

    if (ulType >= SC_NUM_LMT_TYPE_BUTT
        || ulCycle >= SC_NUMBER_LMT_CYCLE_BUTT
        || DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulTime = time(NULL);
    pstTime = dos_get_localtime_struct(ulTime, &stTime);

    stStartTime.tm_sec = 0;
    stStartTime.tm_min = 0;
    stStartTime.tm_hour = 0;

    switch (ulCycle)
    {
        case SC_NUMBER_LMT_CYCLE_DAY:
            stStartTime.tm_mday = pstTime->tm_mday;
            stStartTime.tm_mon = pstTime->tm_mon;
            stStartTime.tm_year = pstTime->tm_year;
            break;

        case SC_NUMBER_LMT_CYCLE_WEEK:
            if (pstTime->tm_wday == 0)
            {
                stStartTime.tm_mday  = pstTime->tm_mday - 6;
            }
            else
            {
                stStartTime.tm_mday  = pstTime->tm_mday - pstTime->tm_wday + 1;
            }

            stStartTime.tm_mon   = pstTime->tm_mon;
            stStartTime.tm_year  = pstTime->tm_year;
            break;

        case SC_NUMBER_LMT_CYCLE_MONTH:
            stStartTime.tm_mday  = 0;
            stStartTime.tm_mon   = pstTime->tm_mon;
            stStartTime.tm_year  = pstTime->tm_year;
            break;

        case SC_NUMBER_LMT_CYCLE_YEAR:
            stStartTime.tm_mday  = 0;
            stStartTime.tm_mon   = 0;
            stStartTime.tm_year  = pstTime->tm_year;
            break;

        default:
            DOS_ASSERT(0);
            return 0;
    }

    ulStartTimestamp = mktime(&stStartTime);

    dos_snprintf(szSQL, sizeof(szSQL)
        , "SELECT COUNT(times) AS times FROM tbl_stat_caller WHERE ctime > %u AND type=%u AND caller=%s"
        , ulStartTimestamp, ulType, pszNumber);

    if (db_query(g_pstSCDBHandle, szSQL, sc_update_number_stat_cb, &ulTimes, NULL) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return 0;
    }

    return ulTimes;
}

U32 sc_num_lmt_update(U32 ulType, VOID *ptr)
{
    U32 ulHashIndex;
    U32 ulTimes = 0;
    HASH_NODE_S             *pstHashNode       = NULL;
    SC_NUMBER_LMT_NODE_ST   *pstNumLmt         = NULL;

    pthread_mutex_lock(&g_mutexHashNumberlmt);

    HASH_Scan_Table(g_pstHashNumberlmt, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashNumberlmt, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstNumLmt = pstHashNode->pHandle;

            ulTimes = sc_update_number_stat(pstNumLmt->ulType, pstNumLmt->ulCycle, pstNumLmt->szPrefix);
            pstNumLmt->ulStatUsed = ulTimes;
        }
    }

    pthread_mutex_unlock(&g_mutexHashNumberlmt);

    return DOS_SUCC;
}

BOOL sc_number_lmt_check(U32 ulType, U32 ulCustomerID, S8 *pszNumber)
{
    U32  ulHashIndexMunLmt  = 0;
    U32  ulHandleType       = 0;
    HASH_NODE_S             *pstHashNodeNumLmt = NULL;
    SC_NUMBER_LMT_NODE_ST   *pstNumLmt         = NULL;
    SC_LIMIT_CALLER_STAT_ST stLimitCaller;

    if (DOS_ADDR_INVALID(pszNumber)
        || '\0' == pszNumber[0]
        || ulType >= SC_NUM_LMT_TYPE_BUTT)
    {
        DOS_ASSERT(0);

        return DOS_TRUE;
    }

    stLimitCaller.enNumType = ulType;
    dos_strncpy(stLimitCaller.szNumber, pszNumber, SC_NUM_LENGTH-1);
    stLimitCaller.szNumber[SC_NUM_LENGTH-1] = '\0';

    ulHashIndexMunLmt = sc_ep_number_lmt_hash_func(pszNumber);
    pthread_mutex_lock(&g_mutexHashNumberlmt);
    pstHashNodeNumLmt= hash_find_node(g_pstHashNumberlmt, ulHashIndexMunLmt, &stLimitCaller, sc_ep_number_lmt_find);
    if (DOS_ADDR_INVALID(pstHashNodeNumLmt)
        || DOS_ADDR_INVALID(pstHashNodeNumLmt->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashNumberlmt);

        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_LIMIT)
                , "Number limit check for \"%s\", There is no limitation for this number."
                , pszNumber);
        return DOS_TRUE;
    }

    pstNumLmt = (SC_NUMBER_LMT_NODE_ST *)pstHashNodeNumLmt->pHandle;
    if (pstNumLmt->ulStatUsed < pstNumLmt->ulLimit)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_LIMIT)
                , "Number limit check for \"%s\". This number did not reached the limitation. Cycle: %u, Limitation: %u, Used: %u"
                , pszNumber, pstNumLmt->ulCycle, pstNumLmt->ulLimit, pstNumLmt->ulStatUsed);

        pthread_mutex_unlock(&g_mutexHashNumberlmt);
        /* 写入统计表 */
        sc_save_number_stat(ulCustomerID, ulType, pszNumber, 1);

        return DOS_TRUE;
    }

    ulHandleType = pstNumLmt->ulHandle;

    pthread_mutex_unlock(&g_mutexHashNumberlmt);

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_LIMIT)
            , "Number limit check for \"%s\", This number has reached the limitation. Process as handle: %u"
            , pszNumber, pstNumLmt->ulHandle);

    switch (ulHandleType)
    {
        case SC_NUM_LMT_HANDLE_REJECT:
            return DOS_FALSE;
            break;

        default:
            DOS_ASSERT(0);
            return DOS_TRUE;
    }

    return DOS_TRUE;
}

static S32 sc_load_number_lmt_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    HASH_NODE_S           *pstHashNode      = NULL;
    SC_NUMBER_LMT_NODE_ST *pstNumLmtNode    = NULL;
    S8                    *pszID            = NULL;
    S8                    *pszLimitationID  = NULL;
    S8                    *pszType          = NULL;
    S8                    *pszBID           = NULL;
    S8                    *pszHandle        = NULL;
    S8                    *pszCycle         = NULL;
    S8                    *pszTimes         = NULL;
    S8                    *pszCID           = NULL;
    S8                    *pszDID           = NULL;
    S8                    *pszNumber        = NULL;
    U32                   ulID              = U32_BUTT;
    U32                   ulLimitationID    = U32_BUTT;
    U32                   ulType            = U32_BUTT;
    U32                   ulBID             = U32_BUTT;
    U32                   ulHandle          = U32_BUTT;
    U32                   ulCycle           = U32_BUTT;
    U32                   ulTimes           = U32_BUTT;
    U32                   ulHashIndex       = U32_BUTT;
    SC_LIMIT_CALLER_STAT_ST stLimitCaller;

    if (lCount < 9)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pszID            = aszValues[0];
    pszLimitationID  = aszValues[1];
    pszType          = aszValues[2];
    pszBID           = aszValues[3];
    pszHandle        = aszValues[4];
    pszCycle         = aszValues[5];
    pszTimes         = aszValues[6];
    pszCID           = aszValues[7];
    pszDID           = aszValues[8];

    if (DOS_ADDR_INVALID(pszID)
        || DOS_ADDR_INVALID(pszLimitationID)
        || DOS_ADDR_INVALID(pszType)
        || DOS_ADDR_INVALID(pszBID)
        || DOS_ADDR_INVALID(pszHandle)
        || DOS_ADDR_INVALID(pszCycle)
        || DOS_ADDR_INVALID(pszTimes)
        || dos_atoul(pszID, &ulID) < 0
        || dos_atoul(pszLimitationID, &ulLimitationID) < 0
        || dos_atoul(pszType, &ulType) < 0
        || dos_atoul(pszBID, &ulBID) < 0
        || dos_atoul(pszHandle, &ulHandle) < 0
        || dos_atoul(pszCycle, &ulCycle) < 0
        || dos_atoul(pszTimes, &ulTimes) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (ulType)
    {
        case SC_NUM_LMT_TYPE_DID:
            pszNumber = pszDID;
            break;
        case SC_NUM_LMT_TYPE_CALLER:
            pszNumber = pszCID;
            break;
        default:
            DOS_ASSERT(0);
            return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pszNumber)
        || '\0' == pszNumber[0])
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (ulHandle >= SC_NUM_LMT_HANDLE_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    stLimitCaller.enNumType = ulType;
    dos_strncpy(stLimitCaller.szNumber, pszNumber, SC_NUM_LENGTH-1);
    stLimitCaller.szNumber[SC_NUM_LENGTH-1] = '\0';

    ulHashIndex = sc_ep_number_lmt_hash_func(pszNumber);
    pthread_mutex_lock(&g_mutexHashNumberlmt);
    pstHashNode = hash_find_node(g_pstHashNumberlmt, ulHashIndex, &stLimitCaller, sc_ep_number_lmt_find);
    if (DOS_ADDR_VALID(pstHashNode))
    {
        pstNumLmtNode= pstHashNode->pHandle;
        if (DOS_ADDR_VALID(pstNumLmtNode))
        {
            pstNumLmtNode->ulID      = ulID;
            pstNumLmtNode->ulGrpID   = ulLimitationID;
            pstNumLmtNode->ulHandle  = ulHandle;
            pstNumLmtNode->ulLimit   = ulTimes;
            pstNumLmtNode->ulCycle   = ulCycle;
            pstNumLmtNode->ulType    = ulType;
            pstNumLmtNode->ulNumberID= ulBID;
            dos_strncpy(pstNumLmtNode->szPrefix, pszNumber, sizeof(pstNumLmtNode->szPrefix));
            pstNumLmtNode->szPrefix[sizeof(pstNumLmtNode->szPrefix) - 1] = '\0';
        }
        else
        {
            pstNumLmtNode = dos_dmem_alloc(sizeof(SC_NUMBER_LMT_NODE_ST));
            if (DOS_ADDR_INVALID(pstNumLmtNode))
            {
                DOS_ASSERT(0);
                pthread_mutex_unlock(&g_mutexHashNumberlmt);

                return DOS_FAIL;
            }

            pstNumLmtNode->ulID      = ulID;
            pstNumLmtNode->ulGrpID   = ulLimitationID;
            pstNumLmtNode->ulHandle  = ulHandle;
            pstNumLmtNode->ulLimit   = ulTimes;
            pstNumLmtNode->ulCycle   = ulCycle;
            pstNumLmtNode->ulType    = ulType;
            pstNumLmtNode->ulNumberID= ulBID;
            pstNumLmtNode->ulStatUsed = 0;
            dos_strncpy(pstNumLmtNode->szPrefix, pszNumber, sizeof(pstNumLmtNode->szPrefix));
            pstNumLmtNode->szPrefix[sizeof(pstNumLmtNode->szPrefix) - 1] = '\0';

            pstHashNode->pHandle = pstNumLmtNode;
        }
    }
    else
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        pstNumLmtNode = dos_dmem_alloc(sizeof(SC_NUMBER_LMT_NODE_ST));
        if (DOS_ADDR_INVALID(pstHashNode)
            || DOS_ADDR_INVALID(pstNumLmtNode))
        {
            DOS_ASSERT(0);
            pthread_mutex_unlock(&g_mutexHashNumberlmt);

            if (DOS_ADDR_VALID(pstHashNode))
            {
                dos_dmem_free(pstHashNode);
                pstHashNode = NULL;
            }

            if (DOS_ADDR_VALID(pstNumLmtNode))
            {
                dos_dmem_free(pstNumLmtNode);
                pstNumLmtNode = NULL;
            }

            return DOS_FAIL;
        }

        pstNumLmtNode->ulID      = ulID;
        pstNumLmtNode->ulGrpID   = ulLimitationID;
        pstNumLmtNode->ulHandle  = ulHandle;
        pstNumLmtNode->ulLimit   = ulTimes;
        pstNumLmtNode->ulCycle   = ulCycle;
        pstNumLmtNode->ulType    = ulType;
        pstNumLmtNode->ulNumberID= ulBID;
        pstNumLmtNode->ulStatUsed = 0;
        dos_strncpy(pstNumLmtNode->szPrefix, pszNumber, sizeof(pstNumLmtNode->szPrefix));
        pstNumLmtNode->szPrefix[sizeof(pstNumLmtNode->szPrefix) - 1] = '\0';

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstNumLmtNode;

        hash_add_node(g_pstHashNumberlmt, pstHashNode, ulHashIndex, NULL);
    }
    pthread_mutex_unlock(&g_mutexHashNumberlmt);

    return DOS_SUCC;
}

U32 sc_number_lmt_load(U32 ulIndex)
{
    S8 szSQL[1024];

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT a.id, a.limitation_id, a.btype, a.bid, b.handle, " \
                      "b.cycle, b.times, c.cid, d.did_number " \
                      "FROM tbl_caller_limitation_assign a " \
                      "LEFT JOIN tbl_caller_limitation b ON a.limitation_id = b.id " \
                      "LEFT JOIN tbl_caller c ON a.btype = %u AND a.bid = c.id " \
                      "LEFT JOIN tbl_sipassign d ON a.btype = %u AND a.bid = d.id "
                    , SC_NUM_LMT_TYPE_CALLER, SC_NUM_LMT_TYPE_DID);
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT a.id, a.limitation_id, a.btype, a.bid, b.handle, " \
                      "b.cycle, b.times, c.cid, d.did_number " \
                      "FROM tbl_caller_limitation_assign a " \
                      "LEFT JOIN tbl_caller_limitation b ON a.limitation_id = b.id " \
                      "LEFT JOIN tbl_caller c ON a.btype = %u AND a.bid = c.id " \
                      "LEFT JOIN tbl_sipassign d ON a.btype = %u AND a.bid = d.id " \
                      "WHERE a.limitation_id = %u"
                    , SC_NUM_LMT_TYPE_CALLER, SC_NUM_LMT_TYPE_DID, ulIndex);

    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_load_number_lmt_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_LIMIT), "%s", "Load number limitation fail.");
        return DOS_FAIL;
    }


    sc_num_lmt_update(0, NULL);

    return DOS_SUCC;
}

U32 sc_number_lmt_delete(U32 ulIndex)
{
    U32                   ulHashIndex       = U32_BUTT;
    HASH_NODE_S           *pstHashNode      = NULL;
    SC_NUMBER_LMT_NODE_ST *pstNumLmtNode    = NULL;

    pthread_mutex_lock(&g_mutexHashNumberlmt);
    HASH_Scan_Table(g_pstHashNumberlmt, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashNumberlmt, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstNumLmtNode = pstHashNode->pHandle;
            if (pstNumLmtNode->ulGrpID == ulIndex)
            {
                hash_delete_node(g_pstHashNumberlmt, pstHashNode, ulHashIndex);
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashNumberlmt);

    return DOS_FAIL;
}

void *sc_limit_stat_mainloop(void *arg)
{
    DLL_NODE_S              *pstDLLNode     = NULL;
    SC_LIMIT_CALLER_STAT_ST *pstLimit       = NULL;
    U32                     ulHashIndex     = U32_BUTT;
    HASH_NODE_S             *pstHashNode    = NULL;
    SC_NUMBER_LMT_NODE_ST   *pstNumLmtNode  = NULL;

    while (1)
    {
        pthread_mutex_lock(&g_mutexLimit);
        pthread_cond_wait(&g_condLimit, &g_mutexLimit);
        pthread_mutex_unlock(&g_mutexLimit);

        if (0 == DLL_Count(&g_stLimitStatQueue))
        {
            break;
        }

        pthread_mutex_lock(&g_mutexLimit);
        pstDLLNode = dll_fetch(&g_stLimitStatQueue);
        pthread_mutex_unlock(&g_mutexLimit);

        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_LIMIT), "%s", "Log digest error.");
            break;
        }

        if (DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_LIMIT), "%s", "Log digest is empty.");
            break;
        }

        pstLimit = (SC_LIMIT_CALLER_STAT_ST *)pstDLLNode->pHandle;

        DLL_Init_Node(pstDLLNode);
        dos_dmem_free(pstDLLNode);
        pstDLLNode = NULL;

        if (DOS_ADDR_INVALID(pstLimit))
        {
            DOS_ASSERT(0);
            continue;
        }

        /* 根据号码查找到对应的控制块，统计次数 +1 */
        ulHashIndex = sc_ep_number_lmt_hash_func(pstLimit->szNumber);
        pthread_mutex_lock(&g_mutexHashNumberlmt);
        pstHashNode = hash_find_node(g_pstHashNumberlmt, ulHashIndex, pstLimit, sc_ep_number_lmt_find);
        if (DOS_ADDR_VALID(pstHashNode))
        {
            pstNumLmtNode = pstHashNode->pHandle;
            if (DOS_ADDR_VALID(pstNumLmtNode))
            {
                pstNumLmtNode->ulStatUsed++;
            }
        }
        pthread_mutex_unlock(&g_mutexHashNumberlmt);

        dos_dmem_free(pstLimit);
        pstLimit = NULL;
    }

    return NULL;
}

U32 sc_number_lmt_stat_add(U32 ulNumType, S8 *szNumber)
{
    DLL_NODE_S              *pstNode    = NULL;
    SC_LIMIT_CALLER_STAT_ST *pstLimit   = NULL;

    if (ulNumType >= SC_NUMBER_TYPE_BUTT
        || DOS_ADDR_INVALID(szNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLimit = (SC_LIMIT_CALLER_STAT_ST *)dos_dmem_alloc(sizeof(SC_LIMIT_CALLER_STAT_ST));
    if (DOS_ADDR_INVALID(pstLimit))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLimit->enNumType = ulNumType;
    dos_strncpy(pstLimit->szNumber, szNumber, SC_NUM_LENGTH-1);
    pstLimit->szNumber[SC_NUM_LENGTH-1] = '\0';

    pstNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstNode))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstLimit);
        pstLimit = NULL;

        return DOS_FAIL;
    }
    DLL_Init_Node(pstNode);
    pstNode->pHandle = (VOID *)pstLimit;

    pthread_mutex_lock(&g_mutexLimit);
    DLL_Add(&g_stLimitStatQueue, pstNode);
    pthread_cond_signal(&g_condLimit);
    pthread_mutex_unlock(&g_mutexLimit);

    return DOS_SUCC;
}


U32 sc_limit_start()
{
    if (pthread_create(&g_pthreadLimit, NULL, sc_limit_stat_mainloop, NULL) < 0)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_LIMIT), "%s", "Create limit stat process pthread fail");

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_limit_init()
{
    DLL_Init(&g_stLimitStatQueue);

    g_pstHashNumberlmt = hash_create_table(SC_NUMBER_LMT_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstHashNumberlmt))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_number_lmt_load(SC_INVALID_INDEX);

    return DOS_SUCC;
}

U32 sc_limit_stop()
{
    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


