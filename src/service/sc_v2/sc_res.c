/**
 * @file : sc_res.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 各种资源管理模块给提供的接口函数
 *
 * @date: 2016年1月14日
 * @arthur: Larry
 */
#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_def.h"
#include "sc_db.h"
#include "sc_res.h"
#include "sc_debug.h"
#include "sc_http_api.h"

extern DB_HANDLE_ST         *g_pstSCDBHandle;


/**
 * 计算一个字符串的HASH值
 *
 * @param U32 pszSrting     被计算的值
 * @param U32 ulHashSize hash表大小
 *
 * @note 最多计算这个字符串钱20个字符的hash值
 */
U32 sc_string_hash_func(S8 *pszSrting, U32 ulHashSize)
{
    U32 ulIndex = 0;
    U32 ulHashIndex = 0;
    U32 ulLoop = 0;

    if (DOS_ADDR_INVALID(pszSrting) || 0 == ulHashSize)
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    ulIndex = 0;
    for (ulLoop=0; ulLoop<SC_STRING_HASH_LIMIT; ulLoop++)
    {
        if ('\0' == pszSrting[ulIndex])
        {
            break;
        }

        ulHashIndex += (pszSrting[ulIndex] << 3);

        ulIndex++;
    }

    return ulHashIndex % ulHashSize;
}

/**
 * 计算一个整数的HASH值
 *
 * @param U32 ulVal     被计算的值
 * @param U32 ulHashSize hash表大小
 *
 * @return 成功返回hash值，否则返回U32_BUTT
 */
U32 sc_int_hash_func(U32 ulVal, U32 ulHashSize)
{
    if (0 == ulHashSize)
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    return ulVal % ulHashSize;
}

/**
 * 初始化pstUserID所指向的User ID描述结构
 *
 * @param SC_USER_ID_NODE_ST *pstUserID 需要别初始化的结构
 *
 * @return VOID
 */
VOID sc_sip_account_init(SC_USER_ID_NODE_ST *pstUserID)
{
    if (pstUserID)
    {
        dos_memzero(pstUserID, sizeof(SC_USER_ID_NODE_ST));

        pstUserID->ulCustomID = U32_BUTT;
        pstUserID->ulSIPID = U32_BUTT;
    }
}


/**
 * 查找SIP账户回调函数
 *
 * @param VOID *pObj
 * @param HASH_NODE_S *pstHashNode
 *
 * @return 如果找到了对应的节点则返回0，非零值都会被认为是不匹配
 */
S32 sc_sip_account_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    S8 *pszSIPAcc = NULL;
    SC_USER_ID_NODE_ST *pstSIPInfo = NULL;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pszSIPAcc = (S8 *)pObj;
    pstSIPInfo = pstHashNode->pHandle;

    if (dos_strnicmp(pstSIPInfo->szUserID, pszSIPAcc, sizeof(pstSIPInfo->szUserID)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}


/**
 * 获取pszNum所指定UserID所属的客户的ID
 *
 * @param S8 *pszNum    : User ID
 *
 * @return 成功返回客户ID值，否则返回U32_BUTT
 */
U32 sc_sip_account_get_customer(S8 *pszNum)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;
    U32                ulCustomerID   = 0;

    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    ulHashIndex = sc_string_hash_func(pszNum, SC_SIP_ACCOUNT_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashSIPUserID, ulHashIndex, (VOID *)pszNum, sc_sip_account_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get customer FAIL by sip(%s)", pszNum);
        pthread_mutex_unlock(&g_mutexHashSIPUserID);
        return U32_BUTT;
    }

    if (DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get customer FAIL by sip(%s), data error", pszNum);
        pthread_mutex_unlock(&g_mutexHashSIPUserID);
        return U32_BUTT;
    }

    pstUserIDNode = pstHashNode->pHandle;

    ulCustomerID = pstUserIDNode->ulCustomID;
    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return ulCustomerID;
}


/**
 * 获取客户ID为ulCustomID，分机号为pszExtension的User ID，并将User ID Copy到缓存pszUserID中
 *
 * @param U32 ulCustomID  : 客户ID
 * @param S8 *pszExtension: 分机号
 * @param S8 *pszUserID   : 账户ID缓存
 * @param U32 ulLength    : 缓存长度
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_sip_account_get_by_extension(U32 ulCustomID, S8 *pszExtension, S8 *pszUserID, U32 ulLength)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszExtension)
        || DOS_ADDR_INVALID(pszUserID)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    HASH_Scan_Table(g_pstHashSIPUserID, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashSIPUserID, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstUserIDNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstUserIDNode))
            {
                continue;
            }

            if (ulCustomID != pstUserIDNode->ulCustomID)
            {
                continue;
            }

            if (0 != dos_strnicmp(pstUserIDNode->szExtension, pszExtension, sizeof(pstUserIDNode->szExtension)))
            {
                continue;
            }

            dos_strncpy(pszUserID, pstUserIDNode->szUserID, ulLength);
            pszUserID[ulLength - 1] = '\0';
            pthread_mutex_unlock(&g_mutexHashSIPUserID);
            return DOS_SUCC;
        }
    }
    pthread_mutex_unlock(&g_mutexHashSIPUserID);
    return DOS_FAIL;
}

BOOL sc_sip_account_be_is_exit(U32 ulCustomID, S8 *szUserID)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(szUserID))
    {
        DOS_ASSERT(0);

        return DOS_FALSE;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    HASH_Scan_Table(g_pstHashSIPUserID, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashSIPUserID, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstUserIDNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstUserIDNode))
            {
                continue;
            }

            if (ulCustomID != pstUserIDNode->ulCustomID)
            {
                continue;
            }

            if (0 != dos_strcmp(pstUserIDNode->szUserID, szUserID))
            {
                continue;
            }

            pthread_mutex_unlock(&g_mutexHashSIPUserID);
            return DOS_TRUE;
        }
    }
    pthread_mutex_unlock(&g_mutexHashSIPUserID);
    return DOS_FALSE;
}

/**
 * 功能: 获取ID为ulSIPUserID SIP User ID，并将SIP USER ID Copy到缓存pszUserID中
 * 参数:
 *      U32 ulSIPUserID : SIP User ID的所以
 *      S8 *pszUserID   : 账户ID缓存
 *      U32 ulLength    : 缓存长度
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_sip_account_get_by_id(U32 ulSipID, S8 *pszUserID, U32 ulLength)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszUserID)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    HASH_Scan_Table(g_pstHashSIPUserID, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashSIPUserID, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstUserIDNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstUserIDNode))
            {
                continue;
            }

            if (ulSipID == pstUserIDNode->ulSIPID)
            {
                dos_strncpy(pszUserID, pstUserIDNode->szUserID, ulLength);
                pszUserID[ulLength - 1] = '\0';
                pthread_mutex_unlock(&g_mutexHashSIPUserID);
                return DOS_SUCC;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashSIPUserID);
    return DOS_FAIL;

}

/* 删除SIP账户 */
U32 sc_sip_account_delete(S8 * pszSipID)
{
    SC_USER_ID_NODE_ST *pstUserID   = NULL;
    HASH_NODE_S        *pstHashNode = NULL;
    U32                ulHashIndex  = U32_BUTT;

    ulHashIndex= sc_string_hash_func(pszSipID, SC_SIP_ACCOUNT_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashSIPUserID, ulHashIndex, (VOID *)pszSipID, sc_sip_account_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    hash_delete_node(g_pstHashSIPUserID, pstHashNode, ulHashIndex);
    pstUserID = pstHashNode->pHandle;
    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = NULL;

    dos_dmem_free(pstUserID);
    pstUserID = NULL;
    dos_dmem_free(pstHashNode);
    pstHashNode = NULL;

    return DOS_SUCC;
}


/**
 * 加载SIP账户时数据库查询的回调函数，将数据加入SIP账户的HASH表中
 *
 * @param VOID *pArg: 参数
 * @param S32 lCount: 列数量
 * @param S8 **aszValues: 值裂变
 * @param S8 **aszNames: 字段名列表
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_sip_account_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_USER_ID_NODE_ST *pstSIPUserIDNodeNew = NULL;
    SC_USER_ID_NODE_ST *pstSIPUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode      = NULL;
    BOOL               blProcessOK       = DOS_FALSE;
    U32                ulHashIndex       = 0;
    S32                lIndex            = 0;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSIPUserIDNodeNew = (SC_USER_ID_NODE_ST *)dos_dmem_alloc(sizeof(SC_USER_ID_NODE_ST));
    if (DOS_ADDR_INVALID(pstSIPUserIDNodeNew))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    sc_sip_account_init(pstSIPUserIDNodeNew);

    for (lIndex=0, blProcessOK = DOS_TRUE; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSIPUserIDNodeNew->ulSIPID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSIPUserIDNodeNew->ulCustomID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "userid", dos_strlen("userid")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstSIPUserIDNodeNew->szUserID, aszValues[lIndex], sizeof(pstSIPUserIDNodeNew->szUserID));
            pstSIPUserIDNodeNew->szUserID[sizeof(pstSIPUserIDNodeNew->szUserID) - 1] = '\0';
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "extension", dos_strlen("extension")))
        {
            if (DOS_ADDR_VALID(aszValues[lIndex])
                && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstSIPUserIDNodeNew->szExtension, aszValues[lIndex], sizeof(pstSIPUserIDNodeNew->szExtension));
                pstSIPUserIDNodeNew->szExtension[sizeof(pstSIPUserIDNodeNew->szExtension) - 1] = '\0';
            }
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstSIPUserIDNodeNew);
        pstSIPUserIDNodeNew = NULL;
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);

    ulHashIndex = sc_string_hash_func(pstSIPUserIDNodeNew->szUserID, SC_SIP_ACCOUNT_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashSIPUserID, ulHashIndex, (VOID *)&pstSIPUserIDNodeNew->szUserID, sc_sip_account_hash_find);

    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstSIPUserIDNodeNew);
            pstSIPUserIDNodeNew = NULL;
            pthread_mutex_unlock(&g_mutexHashSIPUserID);
            return DOS_FAIL;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstSIPUserIDNodeNew;

        hash_add_node(g_pstHashSIPUserID, (HASH_NODE_S *)pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        pstSIPUserIDNode = (SC_USER_ID_NODE_ST *)pstHashNode->pHandle;
        pstSIPUserIDNode->ulCustomID = pstSIPUserIDNodeNew->ulCustomID;

        dos_strncpy(pstSIPUserIDNode->szUserID, pstSIPUserIDNodeNew->szUserID, sizeof(pstSIPUserIDNode->szUserID));
        pstSIPUserIDNode->szUserID[sizeof(pstSIPUserIDNode->szUserID) - 1] = '\0';

        dos_strncpy(pstSIPUserIDNode->szExtension, pstSIPUserIDNodeNew->szExtension, sizeof(pstSIPUserIDNode->szExtension));
        pstSIPUserIDNode->szExtension[sizeof(pstSIPUserIDNode->szExtension) - 1] = '\0';

        dos_dmem_free(pstSIPUserIDNodeNew);
        pstSIPUserIDNodeNew = NULL;
    }

    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return DOS_SUCC;
}

/**
 * 加载SIP账户数据
 *
 * @param U32 ulIndex
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_sip_account_load(U32 ulIndex)
{
    S8 szSQL[1024] = {0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, extension,userid FROM tbl_sip where tbl_sip.status = 0;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, extension,userid FROM tbl_sip where tbl_sip.status = 0 AND id=%d;", ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_sip_account_load_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Load sip account fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_sip_account_update_proc(U32 ulAction, U32 ulSipID, U32 ulCustomerID)
{
    U32  ulRet = 0;
    S8   szUserID[32] = {0, };

    if (ulAction >= SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch(ulAction)
    {
        case SC_API_CMD_ACTION_SIP_ADD:
        case SC_API_CMD_ACTION_SIP_UPDATE:
        {
            ulRet = sc_sip_account_load(ulSipID);
            if (ulRet != DOS_SUCC)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
            break;
        }
        case SC_API_CMD_ACTION_SIP_DELETE:
        {
            ulRet = sc_sip_account_get_by_id(ulSipID, szUserID, sizeof(szUserID));
            if (DOS_SUCC != ulRet)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }

            ulRet = sc_sip_account_delete(szUserID);
            if (ulRet != DOS_SUCC)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }

            break;
        }
        default:
            break;
    }

    sc_send_sip_update_req(ulSipID, ulAction);

    return DOS_SUCC;
}



/**
 * 检查pszNum所执行的分机号，是否输入编号为ulCustomerID的客户
 *
 * @param S8 *pszNum: 分机号
 * @param U32 ulCustomerID: 客户ID
 *
 * @return 成功返回DOS_TRUE，否则返回DOS_FALSE
 */
BOOL sc_sip_account_extension_check(S8 *pszNum, U32 ulCustomerID)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);

        return DOS_FALSE;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    HASH_Scan_Table(g_pstHashSIPUserID, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashSIPUserID, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstUserIDNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstUserIDNode))
            {
                continue;
            }

            if (ulCustomerID == pstUserIDNode->ulCustomID
                && 0 == dos_strnicmp(pstUserIDNode->szExtension, pszNum, sizeof(pstUserIDNode->szExtension)))
            {
                pthread_mutex_unlock(&g_mutexHashSIPUserID);
                return DOS_TRUE;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return DOS_FALSE;
}

U32 sc_sip_account_update_info2db(U32 ulPublicIP, U32 ulPrivateIP, SC_SIP_STATUS_TYPE_EN enStatus, U32 ulSipID)
{
    S8                  szSQL[512]  = { 0 };
    SC_DB_MSG_TAG_ST    *pstMsg     = NULL;

    dos_snprintf(szSQL, sizeof(szSQL), "UPDATE tbl_sip SET public_net=%u, private_net=%u, register=%d WHERE id=%u", ulPublicIP, ulPrivateIP, enStatus, ulSipID);

    pstMsg = (SC_DB_MSG_TAG_ST *)dos_dmem_alloc(sizeof(SC_DB_MSG_TAG_ST));
    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    pstMsg->ulMsgType = SC_MSG_SAVE_SIP_IPADDR;
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

U32 sc_sip_account_update_status(S8 *szUserID, SC_SIP_STATUS_TYPE_EN enStatus, U32 *pulSipID)
{
    SC_USER_ID_NODE_ST *pstUserID   = NULL;
    HASH_NODE_S        *pstHashNode = NULL;
    U32                ulHashIndex  = U32_BUTT;

    ulHashIndex = sc_string_hash_func(szUserID, SC_SIP_ACCOUNT_HASH_SIZE);
    pthread_mutex_lock(&g_mutexHashSIPUserID);
    pstHashNode = hash_find_node(g_pstHashSIPUserID, ulHashIndex, (VOID *)szUserID, sc_sip_account_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashSIPUserID);

        return DOS_FAIL;
    }

    pstUserID = pstHashNode->pHandle;
    if (DOS_ADDR_INVALID(pstUserID))
    {
        pthread_mutex_unlock(&g_mutexHashSIPUserID);
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstUserID->enStatus = enStatus;
    switch (enStatus)
    {
        case SC_SIP_STATUS_TYPE_REGISTER:
            pstUserID->stStat.ulRegisterCnt++;
            break;
        case SC_SIP_STATUS_TYPE_UNREGISTER:
            pstUserID->stStat.ulUnregisterCnt++;
            break;
        default:
            break;
    }

    *pulSipID = pstUserID->ulSIPID;
    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return DOS_SUCC;
}


/**
 * 初始化pstDIDNum所指向的DID号码描述结构
 *
 * @param SC_DID_NODE_ST *pstDIDNum 需要别初始化的DID号码结构
 *
 * 返回值: VOID
 */
VOID sc_did_init(SC_DID_NODE_ST *pstDIDNum)
{
    if (pstDIDNum)
    {
        dos_memzero(pstDIDNum, sizeof(SC_DID_NODE_ST));
        pstDIDNum->ulBindID = U32_BUTT;
        pstDIDNum->ulBindType = U32_BUTT;
        pstDIDNum->ulCustomID = U32_BUTT;
        pstDIDNum->ulDIDID = U32_BUTT;
        pstDIDNum->ulTimes = 0;
    }
}


/* 查找DID号码 */
S32 sc_did_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    S8 *pszDIDNum = NULL;
    SC_DID_NODE_ST *pstDIDInfo = NULL;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pszDIDNum = (S8 *)pObj;
    pstDIDInfo = pstHashNode->pHandle;

    if (dos_strnicmp(pstDIDInfo->szDIDNum, pszDIDNum, sizeof(pstDIDInfo->szDIDNum)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}


U32 sc_did_delete(U32 ulDidID)
{
    HASH_NODE_S     *pstHashNode = NULL;
    SC_DID_NODE_ST  *pstDidNode  = NULL;
    BOOL blFound = DOS_FALSE;
    U32 ulIndex = U32_BUTT;

    pthread_mutex_lock(&g_mutexHashDIDNum);
    HASH_Scan_Table(g_pstHashDIDNum, ulIndex)
    {
        HASH_Scan_Bucket(g_pstHashDIDNum, ulIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                break;
            }

            pstDidNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstDidNode))
            {
                continue;
            }

            if (ulDidID == pstDidNode->ulDIDID)
            {
                blFound = DOS_TRUE;
                break;
            }
        }

        if (blFound)
        {
            break;
        }
    }

    if (blFound)
    {
        ulIndex = sc_string_hash_func(pstDidNode->szDIDNum, SC_DID_HASH_SIZE);

        /* 删除节点 */
        hash_delete_node(g_pstHashDIDNum, pstHashNode, ulIndex);

        if (pstHashNode)
        {
            dos_dmem_free(pstHashNode);
            pstHashNode = NULL;
        }

        if (pstDidNode)
        {
           dos_dmem_free(pstDidNode);
           pstDidNode = NULL;
        }
    }

    pthread_mutex_unlock(&g_mutexHashDIDNum);

    if (blFound)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FALSE;
    }
}


/**
 * 加载DID号码时数据库查询的回调函数，将数据加入DID号码的HASH表中
 *
 * @param VOID *pArg: 参数
 * @param S32 lCount: 列数量
 * @param S8 **aszValues: 值裂变
 * @param S8 **aszNames: 字段名列表
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_did_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_DID_NODE_ST     *pstDIDNumNode    = NULL;
    SC_DID_NODE_ST     *pstDIDNumTmp     = NULL;
    HASH_NODE_S        *pstHashNode      = NULL;
    BOOL               blProcessOK       = DOS_FALSE;
    U32                ulHashIndex       = 0;
    S32                lIndex            = 0;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstDIDNumNode = (SC_DID_NODE_ST *)dos_dmem_alloc(sizeof(SC_DID_NODE_ST));
    if (DOS_ADDR_INVALID(pstDIDNumNode))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_did_init(pstDIDNumNode);

    for (lIndex=0, blProcessOK=DOS_TRUE; lIndex<lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->ulDIDID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->ulCustomID) < 0)
            {
                blProcessOK = DOS_FALSE;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "did_number", dos_strlen("did_number")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstDIDNumNode->szDIDNum, aszValues[lIndex], sizeof(pstDIDNumNode->szDIDNum));
            pstDIDNumNode->szDIDNum[sizeof(pstDIDNumNode->szDIDNum) - 1] = '\0';
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "bind_type", dos_strlen("bind_type")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->ulBindType) < 0
                || pstDIDNumNode->ulBindType >= SC_DID_BIND_TYPE_BUTT)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "bind_id", dos_strlen("bind_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->ulBindID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "status", dos_strlen("status")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->bValid) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstDIDNumNode);
        pstDIDNumNode = NULL;
        return DOS_FALSE;
    }

    pthread_mutex_lock(&g_mutexHashDIDNum);
    ulHashIndex = sc_string_hash_func(pstDIDNumNode->szDIDNum, SC_DID_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashDIDNum, ulHashIndex, (VOID *)pstDIDNumNode->szDIDNum, sc_did_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstDIDNumNode);
            pstDIDNumNode = NULL;
            pthread_mutex_unlock(&g_mutexHashDIDNum);
            return DOS_FALSE;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstDIDNumNode;
        ulHashIndex = sc_string_hash_func(pstDIDNumNode->szDIDNum, SC_DID_HASH_SIZE);

        hash_add_node(g_pstHashDIDNum, (HASH_NODE_S *)pstHashNode, ulHashIndex, NULL);

    }
    else
    {
        pstDIDNumTmp = pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstDIDNumTmp))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstDIDNumNode);
            pstDIDNumNode = NULL;

            pthread_mutex_unlock(&g_mutexHashDIDNum);
            return DOS_FAIL;
        }

        pstDIDNumTmp->ulCustomID = pstDIDNumNode->ulCustomID;
        pstDIDNumTmp->ulDIDID = pstDIDNumNode->ulDIDID;
        pstDIDNumTmp->ulBindType = pstDIDNumNode->ulBindType;
        pstDIDNumTmp->ulBindID  = pstDIDNumNode->ulBindID;
        pstDIDNumTmp->bValid = pstDIDNumNode->bValid;
        dos_strncpy(pstDIDNumTmp->szDIDNum, pstDIDNumNode->szDIDNum, sizeof(pstDIDNumTmp->szDIDNum));
        pstDIDNumTmp->szDIDNum[sizeof(pstDIDNumTmp->szDIDNum) - 1] = '\0';

        dos_dmem_free(pstDIDNumNode);
        pstDIDNumNode = NULL;
    }
    pthread_mutex_unlock(&g_mutexHashDIDNum);

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_load_did_number()
 * 功能: 加载DID号码数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_did_load(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, did_number, bind_type, bind_id, status FROM tbl_sipassign;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, did_number, bind_type, bind_id, status FROM tbl_sipassign where id=%u;", ulIndex);
    }

    db_query(g_pstSCDBHandle, szSQL, sc_did_load_cb, NULL, NULL);

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_ep_get_custom_by_did(S8 *pszNum)
 * 功能: 通过pszNum所指定的DID号码，找到当前DID号码输入那个客户
 * 参数:
 *      S8 *pszNum : DID号码
 * 返回值: 成功返回客户ID，否则返回U32_BUTT
 */
U32 sc_did_get_custom(S8 *pszNum)
{
    SC_DID_NODE_ST     *pstDIDNumNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;
    U32                ulCustomerID   = 0;

    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    ulHashIndex = sc_string_hash_func(pszNum, SC_DID_HASH_SIZE);
    pthread_mutex_lock(&g_mutexHashDIDNum);
    pstHashNode = hash_find_node(g_pstHashDIDNum, ulHashIndex, (VOID *)pszNum, sc_did_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&g_mutexHashDIDNum);
        return U32_BUTT;
    }

    pstDIDNumNode = pstHashNode->pHandle;
    if (DOS_FALSE == pstDIDNumNode->bValid)
    {
        pthread_mutex_unlock(&g_mutexHashDIDNum);

        return U32_BUTT;
    }
    ulCustomerID = pstDIDNumNode->ulCustomID;

    pthread_mutex_unlock(&g_mutexHashDIDNum);

    return ulCustomerID;
}

U32 sc_did_update_proc(U32 ulAction, U32 ulDidID)
{
    U32 ulRet = U32_BUTT;

    if (ulAction > SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch(ulAction)
    {
        case SC_API_CMD_ACTION_DID_ADD:
        case SC_API_CMD_ACTION_DID_UPDATE:
            sc_did_load(ulDidID);
            break;
        case SC_API_CMD_ACTION_DID_DELETE:
            {
                ulRet = sc_did_delete(ulDidID);
                if (ulRet != DOS_SUCC)
                {
                   DOS_ASSERT(0);
                   return DOS_FAIL;
                }
            }
            break;
        default:
            break;
    }
    return DOS_SUCC;
}

U32 sc_did_bind_info_get(S8 *pszDidNum, U32 *pulBindType, U32 *pulBindID)
{
    SC_DID_NODE_ST     *pstDIDNumNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszDidNum)
        || DOS_ADDR_INVALID(pulBindType)
        || DOS_ADDR_INVALID(pulBindID))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulHashIndex = sc_string_hash_func(pszDidNum, SC_DID_HASH_SIZE);
    pthread_mutex_lock(&g_mutexHashDIDNum);
    pstHashNode = hash_find_node(g_pstHashDIDNum, ulHashIndex, (VOID *)pszDidNum, sc_did_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&g_mutexHashDIDNum);
        return DOS_FAIL;
    }

    pstDIDNumNode = pstHashNode->pHandle;

    *pulBindType = pstDIDNumNode->ulBindType;
    *pulBindID = pstDIDNumNode->ulBindID;

    pthread_mutex_unlock(&g_mutexHashDIDNum);

    return DOS_SUCC;
}


VOID sc_customer_init(SC_CUSTOMER_NODE_ST *pstCustomer)
{
    if (pstCustomer)
    {
        dos_memzero(pstCustomer, sizeof(SC_CUSTOMER_NODE_ST));
        pstCustomer->ulID = U32_BUTT;
        pstCustomer->bExist = DOS_FALSE;
    }
}


S32 sc_customer_find(VOID *pKey, DLL_NODE_S *pstDLLNode)
{
    SC_CUSTOMER_NODE_ST *pstCustomerCurrent;
    U32 ulIndex;

    if (DOS_ADDR_INVALID(pKey)
        || DOS_ADDR_INVALID(pstDLLNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulIndex = *(U32 *)pKey;
    pstCustomerCurrent = pstDLLNode->pHandle;

    if (ulIndex == pstCustomerCurrent->ulID && SC_CUSTOMER_TYPE_CONSUMER == pstCustomerCurrent->ucCustomerType)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

U32 sc_customer_delete(U32 ulCustomerID)
{
    DLL_NODE_S *pstListNode = NULL;

    pthread_mutex_lock(&g_mutexCustomerList);
    pstListNode = dll_find(&g_stCustomerList, &ulCustomerID, sc_customer_find);
    if (DOS_ADDR_INVALID(pstListNode)
        || DOS_ADDR_INVALID(pstListNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexCustomerList);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Num Transform Delete FAIL.ID %u does not exist.", ulCustomerID);
        return DOS_FAIL;
    }

    dll_delete(&g_stCustomerList, pstListNode);
    pthread_mutex_unlock(&g_mutexCustomerList);
    dos_dmem_free(pstListNode->pHandle);
    pstListNode->pHandle= NULL;
    dos_dmem_free(pstListNode);
    pstListNode = NULL;
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Delete Num Transform SUCC.(ID:%u)", ulCustomerID);

    return DOS_SUCC;
}


S32 sc_customer_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_CUSTOMER_NODE_ST  *pstCustomer       = NULL;
    SC_CUSTOMER_NODE_ST  *pstCustomerTmp    = NULL;
    DLL_NODE_S           *pstListNode       = NULL;
    S32                  lIndex;
    BOOL                 blProcessOK = DOS_FALSE;
    U32                  ulCallOutGroup;
    U32                  ulType;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstCustomer = dos_dmem_alloc(sizeof(SC_CUSTOMER_NODE_ST));
    if (DOS_ADDR_INVALID(pstCustomer))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_customer_init(pstCustomer);

    for (blProcessOK = DOS_TRUE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstCustomer->ulID) < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "name", dos_strlen("name")))
        {
            dos_strncpy(pstCustomer->szName, aszValues[lIndex], sizeof(pstCustomer->szName));
        }

        else if (0 == dos_strnicmp(aszNames[lIndex], "type", dos_strlen("type")))
        {
            if (dos_atoul(aszValues[lIndex], &ulType) < 0
                || ulType> U8_BUTT)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
            pstCustomer->ucCustomerType = (U8)ulType;
        }
        /* 分组编号可以为空，如果为空或者值超过最大值，都默认按0处理 */
        else if (0 == dos_strnicmp(aszNames[lIndex], "call_out_group", dos_strlen("call_out_group")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                pstCustomer->usCallOutGroup = 0;
            }
            else
            {
                if (dos_atoul(aszValues[lIndex], &ulCallOutGroup) < 0
                    || ulCallOutGroup > U16_BUTT)
                {
                    DOS_ASSERT(0);
                    blProcessOK = DOS_FALSE;
                    break;
                }
                pstCustomer->usCallOutGroup = (U16)ulCallOutGroup;
            }
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstCustomer);
        pstCustomer = NULL;
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexCustomerList);
    pstListNode = dll_find(&g_stCustomerList, &pstCustomer->ulID, sc_customer_find);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        pstListNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstListNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstCustomer);
            pstCustomer = NULL;

            pthread_mutex_unlock(&g_mutexCustomerList);
            return DOS_FAIL;
        }

        DLL_Init_Node(pstListNode);
        pstListNode->pHandle = pstCustomer;
        pstCustomer->bExist = DOS_TRUE;
        DLL_Add(&g_stCustomerList, pstListNode);
    }
    else
    {
        pstCustomerTmp = pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstCustomerTmp))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstCustomer);
            pstCustomer = NULL;

            pthread_mutex_unlock(&g_mutexCustomerList);
            return DOS_FAIL;
        }

        dos_memcpy(pstCustomerTmp, pstCustomer, sizeof(SC_CUSTOMER_NODE_ST));
        pstCustomerTmp->bExist = DOS_TRUE;

        dos_dmem_free(pstCustomer);
        pstCustomer = NULL;
    }
    pthread_mutex_unlock(&g_mutexCustomerList);

    return DOS_TRUE;
}

U32 sc_customer_load(U32 ulIndex)
{
    S8 szSQL[1024];

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, name,type, call_out_group FROM tbl_customer;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, name, type, call_out_group FROM tbl_customer where tbl_customer.id=%u;"
                    , ulIndex);
    }

    db_query(g_pstSCDBHandle, szSQL, sc_customer_load_cb, NULL, NULL);

    return DOS_SUCC;
}


U32 sc_customer_get_callout_group(U32 ulCustomerID, U32 *pusCallOutGroup)
{
    SC_CUSTOMER_NODE_ST  *pstCustomer       = NULL;
    DLL_NODE_S           *pstListNode       = NULL;

    if (DOS_ADDR_INVALID(pusCallOutGroup))
    {
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexCustomerList);
    pstListNode = dll_find(&g_stCustomerList, &ulCustomerID, sc_customer_find);
    if (DOS_ADDR_VALID(pstListNode)
        && DOS_ADDR_VALID(pstListNode->pHandle))
    {
        pstCustomer = (SC_CUSTOMER_NODE_ST *)pstListNode->pHandle;
        *pusCallOutGroup = pstCustomer->usCallOutGroup;
        pthread_mutex_unlock(&g_mutexCustomerList);

        return DOS_SUCC;
    }

    pthread_mutex_unlock(&g_mutexCustomerList);

    return DOS_FAIL;
}

U32 sc_customer_update_proc(U32 ulAction, U32 ulCustomerID)
{
    if (ulAction >= SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (ulAction)
    {
        case SC_API_CMD_ACTION_CALLER_ADD:
        case SC_API_CMD_ACTION_CALLER_UPDATE:
            sc_customer_load(ulCustomerID);
            break;
        case SC_API_CMD_ACTION_CALLER_DELETE:
            sc_customer_delete(ulCustomerID);
            break;
        default:
            break;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Update customer SUCC.(ulAction:%u, ulID:%d)", ulAction, ulCustomerID);

    return DOS_SUCC;
}

BOOL sc_customer_is_exit(U32 ulCustomerID)
{
    DLL_NODE_S           *pstListNode       = NULL;

    pthread_mutex_lock(&g_mutexCustomerList);
    pstListNode = dll_find(&g_stCustomerList, &ulCustomerID, sc_customer_find);
    if (DOS_ADDR_INVALID(pstListNode)
        || DOS_ADDR_INVALID(pstListNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexCustomerList);

        return DOS_FALSE;
    }
    pthread_mutex_unlock(&g_mutexCustomerList);

    return DOS_TRUE;
}

/**
 * 函数: VOID sc_ep_gw_init(SC_GW_NODE_ST *pstGW)
 * 功能: 初始化pstGW所指向的网关描述结构
 * 参数:
 *      SC_GW_NODE_ST *pstGW 需要别初始化的网关描述结构
 * 返回值: VOID
 */
VOID sc_gateway_init(SC_GW_NODE_ST *pstGW)
{
    if (pstGW)
    {
        dos_memzero(pstGW, sizeof(SC_GW_NODE_ST));
        pstGW->ulGWID = U32_BUTT;
        pstGW->bExist = DOS_FALSE;
        pstGW->bStatus = DOS_FALSE;
    }
}


/* 网关hash表查找函数 */
S32 sc_gateway_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_GW_NODE_ST *pstGWNode;

    U32 ulGWIndex = 0;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulGWIndex = *(U32 *)pObj;
    pstGWNode = pstHashNode->pHandle;

    if (ulGWIndex == pstGWNode->ulGWID)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }

}


/**
 * 加载网关列表数据时数据库查询的回调函数，将数据加入网关加入对于路由的列表中
 *
 * @param VOID *pArg: 参数
 * @param S32 lCount: 列数量
 * @param S8 **aszValues: 值裂变
 * @param S8 **aszNames: 字段名列表
 *
 * @return  成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_gateway_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_GW_NODE_ST        *pstGWNode     = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    S8 *pszGWID = NULL, *pszGWName = NULL, *pszDomain = NULL, *pszStatus = NULL, *pszRegister = NULL, *pszRegisterStatus = NULL, *pszPing = NULL;
    U32 ulID, ulStatus, ulRegisterStatus = 0;
    BOOL bIsRegister;
    BOOL bPing;
    U32 ulHashIndex = U32_BUTT;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pszGWID = aszValues[0];
    pszGWName = aszValues[1];
    pszDomain = aszValues[2];
    pszStatus = aszValues[3];
    pszRegister = aszValues[4];
    pszRegisterStatus = aszValues[5];
    pszPing = aszValues[6];
    if (DOS_ADDR_INVALID(pszGWID)
        || DOS_ADDR_INVALID(pszDomain)
        || DOS_ADDR_INVALID(pszStatus)
        || DOS_ADDR_INVALID(pszRegister)
        || dos_atoul(pszGWID, &ulID) < 0
        || dos_atoul(pszStatus, &ulStatus) < 0
        || dos_atoul(pszRegister, &bIsRegister) < 0
        || dos_atoul(pszPing, &bPing) < 0 )
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (bIsRegister)
    {
        /* 注册，获取注册的状态 */
        if (dos_atoul(pszRegisterStatus, &ulRegisterStatus) < 0)
        {
            /* 如果状态为空，则将状态置为 trying */
            ulRegisterStatus = SC_TRUNK_STATE_TYPE_TRYING;
        }
    }
    else
    {
        ulRegisterStatus = SC_TRUNK_STATE_TYPE_UNREGED;
    }

    pthread_mutex_lock(&g_mutexHashGW);
    ulHashIndex = sc_int_hash_func(ulID, SC_GW_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashGW, ulHashIndex, (VOID *)&ulID, sc_gateway_hash_find);
    /* 此过程为了将数据库里的数据全部同步到内存，bExist为true标明这些数据来自于数据库 */
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        /* 如果不存在则重新申请节点并加入内存 */
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);

            pthread_mutex_unlock(&g_mutexHashGW);
            return DOS_FAIL;
        }

        pstGWNode = dos_dmem_alloc(sizeof(SC_GW_NODE_ST));
        if (DOS_ADDR_INVALID(pstGWNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstHashNode);

            pthread_mutex_unlock(&g_mutexHashGW);
            return DOS_FAIL;
        }

        sc_gateway_init(pstGWNode);

        pstGWNode->ulGWID = ulID;
        if ('\0' != pszGWName[0])
        {
            dos_strncpy(pstGWNode->szGWName, pszGWName, sizeof(pstGWNode->szGWName));
            pstGWNode->szGWName[sizeof(pstGWNode->szGWName) - 1] = '\0';
        }
        else
        {
            pstGWNode->szGWName[0] = '\0';
        }


        if ('\0' != pszDomain[0])
        {
            dos_strncpy(pstGWNode->szGWDomain, pszDomain, sizeof(pstGWNode->szGWDomain));
            pstGWNode->szGWDomain[sizeof(pstGWNode->szGWDomain) - 1] = '\0';
        }
        else
        {
            pstGWNode->szGWDomain[0] = '\0';
        }

        pstGWNode->bStatus = ulStatus;
        pstGWNode->bRegister = bIsRegister;
        pstGWNode->bPing = bPing;
        pstGWNode->ulRegisterStatus = ulRegisterStatus;

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstGWNode;
        pstGWNode->bExist = DOS_TRUE;

        ulHashIndex = sc_int_hash_func(pstGWNode->ulGWID, SC_GW_GRP_HASH_SIZE);
        hash_add_node(g_pstHashGW, pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        pstGWNode = pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstGWNode))
        {
            DOS_ASSERT(0);

            pthread_mutex_unlock(&g_mutexHashGW);
            return DOS_FAIL;
        }

        dos_strncpy(pstGWNode->szGWDomain, pszDomain, sizeof(pstGWNode->szGWDomain));
        pstGWNode->szGWDomain[sizeof(pstGWNode->szGWDomain) - 1] = '\0';
        pstGWNode->bExist = DOS_TRUE;
        pstGWNode->bStatus = ulStatus;
        pstGWNode->bRegister = bIsRegister;
        pstGWNode->bPing = bPing;
        pstGWNode->ulRegisterStatus = ulRegisterStatus;
    }
    pthread_mutex_unlock(&g_mutexHashGW);
    return DOS_SUCC;
}


U32 sc_gateway_delete(U32 ulGatewayID)
{
    HASH_NODE_S   *pstHashNode = NULL;
    SC_GW_NODE_ST *pstGateway  = NULL;
    U32  ulIndex = U32_BUTT;

    ulIndex = sc_int_hash_func(ulGatewayID, SC_GW_HASH_SIZE);
    pthread_mutex_lock(&g_mutexHashGW);
    pstHashNode = hash_find_node(g_pstHashGW, ulIndex, (VOID *)&ulGatewayID, sc_gateway_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashGW);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstGateway = pstHashNode->pHandle;
    pstHashNode->pHandle = NULL;

    hash_delete_node(g_pstHashGW, pstHashNode, ulIndex);

    if (pstHashNode)
    {
        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;
    }

    if (pstGateway)
    {
        dos_dmem_free(pstGateway);
        pstHashNode = NULL;
    }

    pthread_mutex_unlock(&g_mutexHashGW);

    return DOS_SUCC;
}

U32 sc_gateway_update_status2db(U32 ulGateWayID, SC_TRUNK_STATE_TYPE_EN enTrunkState)
{
    S8                  szSQL[512]  = { 0 };
    SC_DB_MSG_TAG_ST    *pstMsg     = NULL;

    dos_snprintf(szSQL, sizeof(szSQL), "UPDATE tbl_gateway SET register_status=%d WHERE id=%u", enTrunkState, ulGateWayID);

    pstMsg = (SC_DB_MSG_TAG_ST *)dos_dmem_alloc(sizeof(SC_DB_MSG_TAG_ST));
    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    pstMsg->ulMsgType = SC_MSG_SAVE_TRUNK_STATUS;
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

U32 sc_gateway_update_status(U32 ulGWID, SC_TRUNK_STATE_TYPE_EN enRegisterStatus)
{
    SC_GW_NODE_ST  *pstGWNode     = NULL;
    HASH_NODE_S    *pstHashNode   = NULL;
    U32             ulHashIndex   = U32_BUTT;

    if (enRegisterStatus >= SC_TRUNK_STATE_TYPE_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_int_hash_func(ulGWID, SC_GW_HASH_SIZE);

    pthread_mutex_lock(&g_mutexHashGW);
    pstHashNode = hash_find_node(g_pstHashGW, ulHashIndex, (VOID *)&ulGWID, sc_gateway_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashGW);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstGWNode = (SC_GW_NODE_ST *)pstHashNode->pHandle;
    pstGWNode->ulRegisterStatus = enRegisterStatus;

    pthread_mutex_unlock(&g_mutexHashGW);

    return DOS_SUCC;
}

/**
 * 加载网关数据
 *
 * @param U32 ulIndex 需要加载的网关ID
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_gateway_load(U32 ulIndex)
{
    S8 szSQL[1024] = {0,};
    U32 ulRet;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id, sip_name, realm, status, register, register_status, ping FROM tbl_gateway;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id, sip_name, realm, status, register, register_status, ping FROM tbl_gateway WHERE id=%d;", ulIndex);
    }

    ulRet = db_query(g_pstSCDBHandle, szSQL, sc_gateway_load_cb, NULL, NULL);
    if (DB_ERR_SUCC != ulRet)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Load gateway FAIL.(ID:%u)", ulIndex);
        return DOS_FAIL;
    }
#if 0
    if (SC_INVALID_INDEX == ulIndex)
    {
        ulRet = sc_ep_esl_execute_cmd("bgapi sofia profile external rescan");
        if (ulRet != DOS_SUCC)
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
    }
#endif
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Load gateway SUCC.(ID:%u)", ulIndex);

    return DOS_SUCC;
}

/**
 * 加载路由网关组关系数据
 *
 * @param VOID *pArg: 参数
 * @param S32 lCount: 列数量
 * @param S8 **aszValues: 值裂变
 * @param S8 **aszNames: 字段名列表
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_gateway_relationship_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_GW_GRP_NODE_ST    *pstGWGrp      = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    U32                  ulGatewayID    = U32_BUTT;
    U32                  ulHashIndex    = U32_BUTT;
    S32                  lIndex         = 0;
    BOOL                 blProcessOK    = DOS_TRUE;

    if (DOS_ADDR_INVALID(pArg)
        || lCount <= 0
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    for (lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (DOS_ADDR_INVALID(aszNames[lIndex])
            || DOS_ADDR_INVALID(aszValues[lIndex]))
        {
            break;
        }

        if (0 == dos_strncmp(aszNames[lIndex], "gateway_id", dos_strlen("gateway_id")))
        {
            if (dos_atoul(aszValues[lIndex], &ulGatewayID) < 0)
            {
                blProcessOK  = DOS_FALSE;
                break;
            }
        }
    }

    if (!blProcessOK
        || U32_BUTT == ulGatewayID)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulHashIndex = sc_int_hash_func(ulGatewayID, SC_GW_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashGW, ulHashIndex, &ulGatewayID, sc_gateway_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstListNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    DLL_Init_Node(pstListNode);
    pstListNode->pHandle = pstHashNode->pHandle;

    pstGWGrp = (SC_GW_GRP_NODE_ST *)pArg;

    pthread_mutex_lock(&pstGWGrp->mutexGWList);
    DLL_Add(&pstGWGrp->stGWList, pstListNode);
    pthread_mutex_unlock(&pstGWGrp->mutexGWList);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Gateway %u will be added into group %u.", ulGatewayID, pstGWGrp->ulGWGrpID);

    return DOS_FAIL;
}

/**
 * 加载路由网关组关系数据
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_gateway_relationship_load()
{
    SC_GW_GRP_NODE_ST    *pstGWGrp      = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    U32                  ulHashIndex = 0;
    S8 szSQL[1024] = {0, };

    HASH_Scan_Table(g_pstHashGWGrp, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashGWGrp, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            pstGWGrp = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstGWGrp))
            {
                DOS_ASSERT(0);
                continue;
            }

            dos_snprintf(szSQL, sizeof(szSQL), "SELECT gateway_id FROM tbl_gateway_assign WHERE gateway_grp_id=%d;", pstGWGrp->ulGWGrpID);

            db_query(g_pstSCDBHandle, szSQL, sc_gateway_relationship_load_cb, (VOID *)pstGWGrp, NULL);
        }
    }

    return DOS_SUCC;
}

/* 网关组hash表查找函数 */
S32 sc_gateway_group_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_GW_GRP_NODE_ST *pstGWGrpNode;

    U32 ulGWGrpIndex = 0;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulGWGrpIndex = *(U32 *)pObj;
    pstGWGrpNode = pstHashNode->pHandle;

    if (ulGWGrpIndex == pstGWGrpNode->ulGWGrpID)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }
}



/**
 * 加载网关组列表数据时数据库查询的回调函数
 *
 * @param VOID *pArg: 参数
 * @param S32 lCount: 列数量
 * @param S8 **aszValues: 值裂变
 * @param S8 **aszNames: 字段名列表
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_gateway_group_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_GW_GRP_NODE_ST    *pstGWGrpNode  = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    S8 *pszGWGrpID = NULL;
    S8  szGWGrpName[SC_NAME_STR_LEN];
    U32 ulID = 0;
    U32 ulHashIndex = 0;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pszGWGrpID = aszValues[0];
    dos_strncpy(szGWGrpName, aszValues[1], sizeof(szGWGrpName));
    if (DOS_ADDR_INVALID(pszGWGrpID)
        || dos_atoul(pszGWGrpID, &ulID) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstGWGrpNode = dos_dmem_alloc(sizeof(SC_GW_GRP_NODE_ST));
    if (DOS_ADDR_INVALID(pstGWGrpNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstHashNode);
        return DOS_FAIL;
    }

    pstGWGrpNode->ulGWGrpID = ulID;
    pstGWGrpNode->bExist = DOS_TRUE;
    dos_strncpy(pstGWGrpNode->szGWGrpName, szGWGrpName, sizeof(pstGWGrpNode->szGWGrpName));

    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = pstGWGrpNode;
    DLL_Init(&pstGWGrpNode->stGWList);
    pthread_mutex_init(&pstGWGrpNode->mutexGWList, NULL);

    ulHashIndex = sc_int_hash_func(pstGWGrpNode->ulGWGrpID, SC_GW_GRP_HASH_SIZE);

    pthread_mutex_lock(&g_mutexHashGWGrp);
    hash_add_node(g_pstHashGWGrp, pstHashNode, ulHashIndex, NULL);
    pthread_mutex_unlock(&g_mutexHashGWGrp);

    return DOS_SUCC;
}

/**
 * 加载网关组数据
 *
 * @param U32 ulIndex
 *
 * @return 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_gateway_group_load(U32 ulIndex)
{
    S8 szSQL[1024];
    U32 ulRet;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id, name FROM tbl_gateway_grp WHERE tbl_gateway_grp.status = 0;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id, name FROM tbl_gateway_grp WHERE tbl_gateway_grp.status = 0 AND id = %d;"
                        , ulIndex);
    }

    ulRet = db_query(g_pstSCDBHandle, szSQL, sc_gateway_group_load_cb, NULL, NULL);
    if (ulRet != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Load gateway Group FAIL.(ID:%u)", ulIndex);
        return DOS_FAIL;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Load gateway Group SUCC.(ID:%u)", ulIndex);

    return ulRet;
}


U32 sc_gateway_group_delete(U32 ulGwGroupID)
{
    DLL_NODE_S         *pstDLLNode     = NULL;
    HASH_NODE_S        *pstHashNode    = NULL;
    SC_GW_GRP_NODE_ST  *pstGwGroupNode = NULL;
    U32 ulIndex = U32_BUTT;

    /* 获得网关组索引 */
    ulIndex = sc_int_hash_func(ulGwGroupID, SC_GW_GRP_HASH_SIZE);

    pthread_mutex_lock(&g_mutexHashGWGrp);

    /* 查找网关组节点首地址 */
    pstHashNode = hash_find_node(g_pstHashGWGrp, ulIndex, (VOID *)&ulGwGroupID, sc_gateway_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashGW);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstGwGroupNode = pstHashNode->pHandle;
    pstHashNode->pHandle = NULL;

    /* 删除节点 */
    hash_delete_node(g_pstHashGWGrp, pstHashNode, ulIndex);

    if (pstGwGroupNode)
    {
        pthread_mutex_lock(&pstGwGroupNode->mutexGWList);
        while (1)
        {
            if (DLL_Count(&pstGwGroupNode->stGWList) == 0)
            {
                break;
            }

            pstDLLNode = dll_fetch(&pstGwGroupNode->stGWList);
            if (DOS_ADDR_INVALID(pstDLLNode))
            {
                DOS_ASSERT(0);
                break;
            }

            /* dll节点的数据域勿要删除 */

            DLL_Init_Node(pstDLLNode);
            dos_dmem_free(pstDLLNode);
            pstDLLNode = NULL;
        }
        pthread_mutex_unlock(&pstGwGroupNode->mutexGWList);


        pthread_mutex_destroy(&pstGwGroupNode->mutexGWList);
        dos_dmem_free(pstGwGroupNode);
        pstGwGroupNode = NULL;
    }

    if (pstHashNode)
    {
       dos_dmem_free(pstHashNode);
       pstHashNode = NULL;
    }

    pthread_mutex_unlock(&g_mutexHashGWGrp);

    return DOS_SUCC;
}

U32 sc_gateway_group_refresh(U32 ulIndex)
{
    S8 szSQL[1024];
    U32 ulHashIndex;
    SC_GW_GRP_NODE_ST    *pstGWGrpNode  = NULL;
    SC_GW_NODE_ST        *pstGWNode = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    DLL_NODE_S           *pstDLLNode    = NULL;

    if (SC_INVALID_INDEX == ulIndex || U32_BUTT == ulIndex)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_int_hash_func(ulIndex, SC_CALLER_GRP_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashGWGrp, ulHashIndex, &ulIndex, sc_gateway_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstGWGrpNode = pstHashNode->pHandle;
    pthread_mutex_lock(&pstGWGrpNode->mutexGWList);
    while (1)
    {
        if (DLL_Count(&pstGWGrpNode->stGWList) == 0)
        {
            break;
        }

        pstDLLNode = dll_fetch(&pstGWGrpNode->stGWList);
        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            DOS_ASSERT(0);
            break;
        }
        pstGWNode = (SC_GW_NODE_ST *)pstDLLNode->pHandle;
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Gateway %u will be removed from Group %u", pstGWNode->ulGWID, ulIndex);

        /* 此处不能释放网关数据，只需释放链表结点即可，因为一个网关可以属于多个网关组，此处删除，容易产生double free信号 */
        if (DOS_ADDR_VALID(pstDLLNode->pHandle))
        {
            /*dos_dmem_free(pstDLLNode->pHandle);*/
            pstDLLNode->pHandle = NULL;
        }

        dos_dmem_free(pstDLLNode);
        pstDLLNode = NULL;
    }
    pthread_mutex_unlock(&pstGWGrpNode->mutexGWList);

    dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT tbl_gateway_assign.gateway_id FROM tbl_gateway_assign WHERE tbl_gateway_assign.gateway_grp_id = %u;"
                        , ulIndex);

    return db_query(g_pstSCDBHandle, szSQL, sc_gateway_relationship_load_cb, pstGWGrpNode, NULL);
}


U32 sc_gateway_group_update_proc(U32 ulAction, U32 ulGwGroupID)
{
    U32  ulRet = U32_BUTT;

    if (ulAction > SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch(ulAction)
    {
        case SC_API_CMD_ACTION_GW_GROUP_ADD:
            ulRet = sc_gateway_group_load(ulGwGroupID);
            if (DOS_SUCC != ulRet)
            {
                DOS_ASSERT(0);
                break;
            }
            /* 这里没有break */

        case SC_API_CMD_ACTION_GW_GROUP_UPDATE:
            ulRet = sc_gateway_group_refresh(ulGwGroupID);
            break;

        case SC_API_CMD_ACTION_GW_GROUP_DELETE:
            ulRet = sc_gateway_group_delete(ulGwGroupID);
            break;
        default:
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Edit gw group Finished. ID: %u Action:%u, Result: %u", ulGwGroupID, ulAction, ulRet);

    return ulRet;
}


/**
 * 函数名:S32 sc_update_gateway(U32 ulID)
 * 参数:  U32 ulID  网关ID
 * 功能:  强制更新网关，根据id或者表名
 * 返回:  更新成功则返回DOS_SUCC，否则返回DOS_FAIL;
 **/
S32 sc_gateway_update(U32 ulID)
{
    return DOS_SUCC;
}


U32 sc_gateway_update_proc(U32 ulAction, U32 ulGatewayID)
{
    U32   ulRet = 0;

    if (ulAction >= SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch(ulAction)
    {
        case SC_API_CMD_ACTION_GATEWAY_ADD:
        case SC_API_CMD_ACTION_GATEWAY_UPDATE:
        {
            ulRet = sc_gateway_load(ulGatewayID);
            if (ulRet != DOS_SUCC)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }

            break;
        }

        case SC_API_CMD_ACTION_GATEWAY_DELETE:
        {
            ulRet = sc_gateway_delete(ulGatewayID);
            if (ulRet != DOS_SUCC)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
            break;
        }
        case SC_API_CMD_ACTION_GATEWAY_SYNC:
        {
            ulRet = sc_gateway_update(U32_BUTT);
            if (DOS_SUCC != ulRet)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
            break;
        }
        default:
            break;
    }

    sc_send_gateway_update_req(ulGatewayID, ulAction);

    return DOS_SUCC;
}



/**
 * 初始化pstRoute所指向的路由描述结构
 *
 * @param SC_ROUTE_NODE_ST *pstRoute 需要别初始化的路由描述结构
 *
 * 返回值: VOID
 */
VOID sc_route_init(SC_ROUTE_NODE_ST *pstRoute)
{
    if (pstRoute)
    {
        dos_memzero(pstRoute, sizeof(SC_ROUTE_NODE_ST));
        pstRoute->ulID = U32_BUTT;

        pstRoute->ucHourBegin = 0;
        pstRoute->ucMinuteBegin = 0;
        pstRoute->ucHourEnd = 0;
        pstRoute->ucMinuteEnd = 0;
        pstRoute->bExist = DOS_FALSE;
        pstRoute->bStatus = DOS_FALSE;
    }
}


/* 查找路由 */
S32 sc_route_find(VOID *pKey, DLL_NODE_S *pstDLLNode)
{
    SC_ROUTE_NODE_ST *pstRouteCurrent;
    U32 ulIndex;

    if (DOS_ADDR_INVALID(pKey)
        || DOS_ADDR_INVALID(pstDLLNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulIndex = *(U32 *)pKey;
    pstRouteCurrent = pstDLLNode->pHandle;

    if (ulIndex == pstRouteCurrent->ulID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/**
 * 函数: S32 sc_load_route_group_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * 功能: 加载路由数据时数据库查询的回调函数，将数据加入路由列表中
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_route_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_ROUTE_NODE_ST     *pstRoute      = NULL;
    SC_ROUTE_NODE_ST     *pstRouteTmp   = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    S32                  lIndex;
    S32                  lSecond;
    S32                  lRet;
    S32                  lDestIDCount;
    BOOL                 blProcessOK = DOS_FALSE;
    U32                  ulCallOutGroup;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstRoute = dos_dmem_alloc(sizeof(SC_ROUTE_NODE_ST));
    if (DOS_ADDR_INVALID(pstRoute))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_route_init(pstRoute);


    for (blProcessOK = DOS_TRUE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstRoute->ulID) < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if ( 0 == dos_strnicmp(aszNames[lIndex], "name", dos_strlen("name")))
        {
            dos_strncpy(pstRoute->szRouteName, aszValues[lIndex], sizeof(pstRoute->szRouteName));
        }

        else if (0 == dos_strnicmp(aszNames[lIndex], "start_time", dos_strlen("start_time")))
        {
            lRet = dos_sscanf(aszValues[lIndex]
                        , "%d:%d:%d"
                        , &pstRoute->ucHourBegin
                        , &pstRoute->ucMinuteBegin
                        , &lSecond);
            if (3 != lRet)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "end_time", dos_strlen("end_time")))
        {
            lRet = dos_sscanf(aszValues[lIndex]
                    , "%d:%d:%d"
                    , &pstRoute->ucHourEnd
                    , &pstRoute->ucMinuteEnd
                    , &lSecond);
            if (3 != lRet)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "callee_prefix", dos_strlen("callee_prefix")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstRoute->szCalleePrefix, aszValues[lIndex], sizeof(pstRoute->szCalleePrefix));
                pstRoute->szCalleePrefix[sizeof(pstRoute->szCalleePrefix) - 1] = '\0';
            }
            else
            {
                pstRoute->szCalleePrefix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "caller_prefix", dos_strlen("caller_prefix")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstRoute->szCallerPrefix, aszValues[lIndex], sizeof(pstRoute->szCallerPrefix));
                pstRoute->szCallerPrefix[sizeof(pstRoute->szCallerPrefix) - 1] = '\0';
            }
            else
            {
                pstRoute->szCallerPrefix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "dest_type", dos_strlen("dest_type")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0]
                || dos_atoul(aszValues[lIndex], &pstRoute->ulDestType) < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "dest_id", dos_strlen("dest_id")))
        {
            if (SC_DEST_TYPE_GATEWAY == pstRoute->ulDestType)
            {
                if (DOS_ADDR_INVALID(aszValues[lIndex])
                    || '\0' == aszValues[lIndex][0]
                    || dos_atoul(aszValues[lIndex], &pstRoute->aulDestID[0]) < 0)
                {
                    DOS_ASSERT(0);

                    blProcessOK = DOS_FALSE;
                    break;
                }
            }
            else if (SC_DEST_TYPE_GW_GRP == pstRoute->ulDestType)
            {
                if (DOS_ADDR_INVALID(aszValues[lIndex])
                    || '\0' == aszValues[lIndex])
                {
                    DOS_ASSERT(0);

                    blProcessOK = DOS_FALSE;
                    break;
                }

                pstRoute->aulDestID[0] = U32_BUTT;
                pstRoute->aulDestID[1] = U32_BUTT;
                pstRoute->aulDestID[2] = U32_BUTT;
                pstRoute->aulDestID[3] = U32_BUTT;
                pstRoute->aulDestID[4] = U32_BUTT;

                lDestIDCount = dos_sscanf(aszValues[lIndex], "%d,%d,%d,%d,%d", &pstRoute->aulDestID[0], &pstRoute->aulDestID[1], &pstRoute->aulDestID[2], &pstRoute->aulDestID[3], &pstRoute->aulDestID[4]);
                if (lDestIDCount < 1)
                {
                    DOS_ASSERT(0);

                    blProcessOK = DOS_FALSE;
                    break;
                }
            }
            else
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "call_out_group", dos_strlen("call_out_group")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0]
                || dos_atoul(aszValues[lIndex], &ulCallOutGroup) < 0
                || ulCallOutGroup > U16_BUTT)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
            pstRoute->usCallOutGroup = (U16)ulCallOutGroup;
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "status", dos_strlen("status")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0]
                || dos_atoul(aszValues[lIndex], &pstRoute->bStatus) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "seq", dos_strlen("seq")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0]
                || dos_atoul(aszValues[lIndex], (U32 *)&pstRoute->ucPriority) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstRoute);
        pstRoute = NULL;
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexRouteList);
    pstListNode = dll_find(&g_stRouteList, &pstRoute->ulID, sc_route_find);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        pstListNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstListNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstRoute);
            pstRoute = NULL;

            pthread_mutex_unlock(&g_mutexRouteList);
            return DOS_FAIL;
        }

        DLL_Init_Node(pstListNode);
        pstListNode->pHandle = pstRoute;
        pstRoute->bExist = DOS_TRUE;
        DLL_Add(&g_stRouteList, pstListNode);
    }
    else
    {
        pstRouteTmp = pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstRouteTmp))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstRoute);
            pstRoute = NULL;

            pthread_mutex_unlock(&g_mutexRouteList);
            return DOS_FAIL;
        }

        dos_memcpy(pstRouteTmp, pstRoute, sizeof(SC_ROUTE_NODE_ST));
        pstRouteTmp->bExist = DOS_TRUE;

        dos_dmem_free(pstRoute);
        pstRoute = NULL;
    }
    pthread_mutex_unlock(&g_mutexRouteList);

    return DOS_TRUE;
}

/**
 * 函数: U32 sc_load_route()
 * 功能: 加载路由数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_route_load(U32 ulIndex)
{
    S8 szSQL[1024];
    S32 lRet;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, name, start_time, end_time, callee_prefix, caller_prefix, dest_type, dest_id, call_out_group, status, seq FROM tbl_route ORDER BY tbl_route.seq ASC;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, name, start_time, end_time, callee_prefix, caller_prefix, dest_type, dest_id, call_out_group, status, seq FROM tbl_route WHERE id=%d ORDER BY tbl_route.seq ASC;"
                    , ulIndex);
    }

    lRet = db_query(g_pstSCDBHandle, szSQL, sc_route_load_cb, NULL, NULL);
    if (DB_ERR_SUCC != lRet)
    {
        DOS_ASSERT(0);
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Load route FAIL(ID:%u).", ulIndex);
        return DOS_FAIL;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Load route SUCC.(ID:%u)", ulIndex);

    return DOS_SUCC;
}


U32 sc_route_delete(U32 ulRouteID)
{
    DLL_NODE_S       *pstDLLNode   = NULL;
    SC_ROUTE_NODE_ST *pstRouteNode = NULL;

    pthread_mutex_lock(&g_mutexRouteList);
    pstDLLNode = dll_find(&g_stRouteList, (VOID *)&ulRouteID, sc_route_find);
    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        pthread_mutex_unlock(&g_mutexRouteList);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pstRouteNode = pstDLLNode->pHandle;
    pstDLLNode->pHandle =  NULL;

    dll_delete(&g_stRouteList, pstDLLNode);

    if (pstRouteNode)
    {
        dos_dmem_free(pstRouteNode);
        pstRouteNode = NULL;
    }

    if (pstDLLNode)
    {
        dos_dmem_free(pstDLLNode);
        pstDLLNode = NULL;
    }

    pthread_mutex_unlock(&g_mutexRouteList);

    return DOS_SUCC;
}


/**
 * 函数: U32 sc_ep_search_route(SC_SCB_ST *pstSCB)
 * 功能: 获取路由组
 * 参数:
 *      SC_SCB_ST *pstSCB : 呼叫控制块，使用主被叫号码
 * 返回值: 成功返回路由组ID，否则返回U32_BUTT
 */
U32 sc_route_search(SC_SRV_CB *pstSCB, S8 *pszCalling, S8 *pszCallee)
{
    SC_ROUTE_NODE_ST     *pstRouetEntry = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    struct tm            *pstTime;
    time_t               timep;
    U32                  ulRouteGrpID;
    U32                  ulStartTime, ulEndTime, ulCurrentTime;
    U32                  ulCallOutGroup;

    if (DOS_ADDR_INVALID(pszCalling) || DOS_ADDR_INVALID(pszCallee))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    timep = time(NULL);
    pstTime = localtime(&timep);
    if (DOS_ADDR_INVALID(pstTime))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    ulRouteGrpID = U32_BUTT;

    /* 根据 ulCustomID 查到到 呼出组, 如果查找失败，则将呼出组置为0 */
    if (sc_customer_get_callout_group(pstSCB->ulCustomerID, &ulCallOutGroup) != DOS_SUCC)
    {
        ulCallOutGroup = 0;
    }

    sc_trace_scb(pstSCB, "usCallOutGroup : %u, custom : %u", ulCallOutGroup, pstSCB->ulCustomerID);
    pthread_mutex_lock(&g_mutexRouteList);

    while (1)
    {
        DLL_Scan(&g_stRouteList, pstListNode, DLL_NODE_S *)
        {
            pstRouetEntry = (SC_ROUTE_NODE_ST *)pstListNode->pHandle;
            if (DOS_ADDR_INVALID(pstRouetEntry))
            {
                continue;
            }

            if ((U16)ulCallOutGroup != pstRouetEntry->usCallOutGroup)
            {
                continue;
            }

            sc_trace_scb(pstSCB, "Search Route: %d:%d, %d:%d, CallerPrefix : %s, CalleePrefix : %s. " \
                                 "Caller:%s, Callee:%s, customer group : %u, route group : %u"
                            , pstRouetEntry->ucHourBegin, pstRouetEntry->ucMinuteBegin
                            , pstRouetEntry->ucHourEnd, pstRouetEntry->ucMinuteEnd
                            , pstRouetEntry->szCallerPrefix
                            , pstRouetEntry->szCalleePrefix
                            , pszCalling
                            , pszCallee
                            , ulCallOutGroup
                            , pstRouetEntry->usCallOutGroup);

            /* 如果开始和结束时间都为 00:00, 则表示全天有效，不用判断时间了 */
            if (pstRouetEntry->ucHourBegin
                || pstRouetEntry->ucMinuteBegin
                || pstRouetEntry->ucHourEnd
                || pstRouetEntry->ucMinuteEnd)
            {
                ulStartTime = pstRouetEntry->ucHourBegin * 60 + pstRouetEntry->ucMinuteBegin;
                ulEndTime = pstRouetEntry->ucHourEnd* 60 + pstRouetEntry->ucMinuteEnd;
                ulCurrentTime = pstTime->tm_hour *60 + pstTime->tm_min;

                if (ulCurrentTime < ulStartTime || ulCurrentTime > ulEndTime)
                {
                    sc_trace_scb(pstSCB, "Search Route(FAIL): Time not match: Peroid:%u-:%u, Current:%u"
                                    , ulStartTime, ulEndTime, ulCurrentTime);

                    continue;
                }
            }

            if ('\0' == pstRouetEntry->szCalleePrefix[0])
            {
                if ('\0' == pstRouetEntry->szCallerPrefix[0])
                {
                    ulRouteGrpID = pstRouetEntry->ulID;
                    break;
                }
                else
                {
                    if (0 == dos_strnicmp(pstRouetEntry->szCallerPrefix, pszCalling, dos_strlen(pstRouetEntry->szCallerPrefix)))
                    {
                        ulRouteGrpID = pstRouetEntry->ulID;
                        break;
                    }
                }
            }
            else
            {
                if ('\0' == pstRouetEntry->szCallerPrefix[0])
                {
                    if (0 == dos_strnicmp(pstRouetEntry->szCalleePrefix, pszCallee, dos_strlen(pstRouetEntry->szCalleePrefix)))
                    {
                        ulRouteGrpID = pstRouetEntry->ulID;
                        break;
                    }
                }
                else
                {
                    if (0 == dos_strnicmp(pstRouetEntry->szCalleePrefix, pszCallee, dos_strlen(pstRouetEntry->szCalleePrefix))
                        && 0 == dos_strnicmp(pstRouetEntry->szCallerPrefix, pszCalling, dos_strlen(pstRouetEntry->szCallerPrefix)))
                    {
                        ulRouteGrpID = pstRouetEntry->ulID;
                        break;
                    }
                }
            }
        }

        if (U32_BUTT == ulRouteGrpID && ulCallOutGroup != 0)
        {
            /* 没有查找到 呼出组一样的 路由， 再循环一遍，查找通配的路由 */
            ulCallOutGroup = 0;
            continue;
        }
        else
        {
            break;
        }
    }

    if (DOS_ADDR_VALID(pstRouetEntry))
    {
        sc_trace_scb(pstSCB, "Search Route Finished. Result: %s, Route ID: %d, Dest Type:%u, Dest ID: %u"
                        , U32_BUTT == ulRouteGrpID ? "FAIL" : "SUCC"
                        , ulRouteGrpID
                        , pstRouetEntry->ulDestType
                        , pstRouetEntry->aulDestID[0]);
    }
    else
    {
        sc_trace_scb(pstSCB, "Search Route Finished. Result: %s, Route ID: %d"
                , U32_BUTT == ulRouteGrpID ? "FAIL" : "SUCC"
                , ulRouteGrpID);
    }

    pthread_mutex_unlock(&g_mutexRouteList);

    return ulRouteGrpID;
}

/**
 * 通过路由组ID，和被叫号码获取出局呼叫的呼叫字符串，并将结果存储在szCalleeString中
 *
 * @param U32 ulRouteGroupID : 路由组ID
 * @param U32 *paulTrunkList : 被叫号码
 * @param U32 ulTrunkListSize : 呼叫字符串缓存
 *
 * 返回值: 返回 中继的个数，如果为0，则是失败
 */
U32 sc_route_get_trunks(U32 ulRouteID, U32 *paulTrunkList, U32 ulTrunkListSize)
{
    SC_ROUTE_NODE_ST     *pstRouetEntry = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    SC_GW_GRP_NODE_ST    *pstGWGrp      = NULL;
    SC_GW_NODE_ST        *pstGW         = NULL;
    U32                  ulHashIndex;
    S32                  lIndex;
    U32                  ulCurrentIndex = 0;

    if (DOS_ADDR_INVALID(paulTrunkList) || ulTrunkListSize < 0)
    {
        DOS_ASSERT(0);
        return 0;
    }

    pthread_mutex_lock(&g_mutexRouteList);
    pstListNode = dll_find(&g_stRouteList, (VOID*)&ulRouteID, sc_route_find);
    pthread_mutex_unlock(&g_mutexRouteList);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "cannot the route by the id %u", ulRouteID);
        return 0;
    }

    ulCurrentIndex = 0;
    pstRouetEntry = pstListNode->pHandle;
    switch (pstRouetEntry->ulDestType)
    {
        case SC_DEST_TYPE_GATEWAY:
            /* 获得中继，判断中继是否可用 */
            ulHashIndex = sc_int_hash_func(pstRouetEntry->aulDestID[0], SC_GW_HASH_SIZE);
            pthread_mutex_lock(&g_mutexHashGW);
            pstHashNode = hash_find_node(g_pstHashGW, ulHashIndex, (VOID *)&pstRouetEntry->aulDestID[0], sc_gateway_hash_find);
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                pthread_mutex_unlock(&g_mutexHashGW);
                break;
            }

            pstGW = pstHashNode->pHandle;
            if (DOS_FALSE == pstGW->bStatus
                || (pstGW->bRegister && pstGW->ulRegisterStatus != SC_TRUNK_STATE_TYPE_REGED))
            {
                pthread_mutex_unlock(&g_mutexHashGW);
                break;
            }

            paulTrunkList[ulCurrentIndex] = pstGW->ulGWID;
            ulCurrentIndex++;
            pthread_mutex_unlock(&g_mutexHashGW);
            break;

        case SC_DEST_TYPE_GW_GRP:
            /* 查找网关组 */
            for (lIndex=0; lIndex<SC_ROUT_GW_GRP_MAX_SIZE; lIndex++)
            {
                if (U32_BUTT == pstRouetEntry->aulDestID[lIndex])
                {
                    break;
                }

                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Search gateway Group, ID is %u", pstRouetEntry->aulDestID[lIndex]);

                ulHashIndex = sc_int_hash_func(pstRouetEntry->aulDestID[lIndex], SC_GW_GRP_HASH_SIZE);
                pstHashNode = hash_find_node(g_pstHashGWGrp, ulHashIndex, (VOID *)&pstRouetEntry->aulDestID[lIndex], sc_gateway_group_hash_find);
                if (DOS_ADDR_INVALID(pstHashNode)
                    || DOS_ADDR_INVALID(pstHashNode->pHandle))
                {
                    /* 没有找到对应的中继组，继续查找下一个，这种情况，理论上是不应该出现的 */
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_RES), "Not found gateway group %d", pstRouetEntry->aulDestID[lIndex]);
                    continue;
                }

                /* 查找网关 */
                /* 生成呼叫字符串 */
                pstGWGrp= pstHashNode->pHandle;
                pthread_mutex_lock(&pstGWGrp->mutexGWList);
                DLL_Scan(&pstGWGrp->stGWList, pstListNode, DLL_NODE_S *)
                {
                    if (DOS_ADDR_INVALID(pstListNode)
                        || DOS_ADDR_INVALID(pstListNode->pHandle))
                    {
                        continue;
                    }

                    pstGW = pstListNode->pHandle;
                    if (DOS_FALSE == pstGW->bStatus
                        || (pstGW->bRegister && pstGW->ulRegisterStatus != SC_TRUNK_STATE_TYPE_REGED))
                    {
                        continue;
                    }

                    paulTrunkList[ulCurrentIndex] = pstGW->ulGWID;
                    ulCurrentIndex++;
                }
                pthread_mutex_unlock(&pstGWGrp->mutexGWList);
            }
            break;

        default:
            DOS_ASSERT(0);
            break;
    }


    return ulCurrentIndex;
}


U32 sc_route_update_proc(U32 ulAction, U32 ulRouteID)
{
    U32 ulRet = U32_BUTT;

    if (ulAction > SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch(ulAction)
    {
        case SC_API_CMD_ACTION_ROUTE_ADD:
        case SC_API_CMD_ACTION_ROUTE_UPDATE:
            sc_route_load(ulRouteID);
            break;

        case SC_API_CMD_ACTION_ROUTE_DELETE:
            {
                ulRet = sc_route_delete(ulRouteID);
                if (ulRet != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                    return DOS_FAIL;
                }
            }
            break;
        case SC_API_CMD_ACTION_ROUTE_SYNC:
#if 0
            {
                ulRet = sc_update_route(U32_BUTT);
                if (ulRet != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                    return DOS_FAIL;
                }
            }
#endif
            break;
        default:
            break;
    }
    return DOS_SUCC;
}



VOID sc_transform_init(SC_NUM_TRANSFORM_NODE_ST *pstNumTransform)
{
    if (pstNumTransform)
    {
        dos_memzero(pstNumTransform, sizeof(SC_NUM_TRANSFORM_NODE_ST));
        pstNumTransform->ulID = U32_BUTT;
        pstNumTransform->bExist = DOS_FALSE;
    }
}


S32 sc_transform_find(VOID *pKey, DLL_NODE_S *pstDLLNode)
{
    SC_NUM_TRANSFORM_NODE_ST *pstTransformCurrent;
    U32 ulIndex;

    if (DOS_ADDR_INVALID(pKey)
        || DOS_ADDR_INVALID(pstDLLNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulIndex = *(U32 *)pKey;
    pstTransformCurrent = pstDLLNode->pHandle;

    if (ulIndex == pstTransformCurrent->ulID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}


U32 sc_transform_delete(U32 ulTransformID)
{
    DLL_NODE_S *pstListNode = NULL;

    pthread_mutex_lock(&g_mutexNumTransformList);
    pstListNode = dll_find(&g_stNumTransformList, &ulTransformID, sc_transform_find);
    if (DOS_ADDR_INVALID(pstListNode)
        || DOS_ADDR_INVALID(pstListNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexNumTransformList);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Num Transform Delete FAIL.ID %u does not exist.", ulTransformID);
        return DOS_FAIL;
    }

    dll_delete(&g_stNumTransformList, pstListNode);
    pthread_mutex_unlock(&g_mutexNumTransformList);
    dos_dmem_free(pstListNode->pHandle);
    pstListNode->pHandle= NULL;
    dos_dmem_free(pstListNode);
    pstListNode = NULL;
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Delete Num Transform SUCC.(ID:%u)", ulTransformID);

    return DOS_SUCC;
}



S32 sc_transform_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_NUM_TRANSFORM_NODE_ST    *pstNumTransform    = NULL;
    SC_NUM_TRANSFORM_NODE_ST    *pstNumTransformTmp = NULL;
    DLL_NODE_S                  *pstListNode        = NULL;
    S32                         lIndex              = 0;
    S32                         lRet                = 0;
    BOOL                        blProcessOK         = DOS_FALSE;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstNumTransform = (SC_NUM_TRANSFORM_NODE_ST *)dos_dmem_alloc(sizeof(SC_NUM_TRANSFORM_NODE_ST));
    if (DOS_ADDR_INVALID(pstNumTransform))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    sc_transform_init(pstNumTransform);

    for (blProcessOK = DOS_TRUE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstNumTransform->ulID) < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "object_id", dos_strlen("object_id")))
        {
            if ('\0' == aszValues[lIndex][0])
            {
                continue;
            }

            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->ulObjectID);
            if (lRet < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "object", dos_strlen("object")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->enObject);
            if (lRet < 0 || pstNumTransform->enObject >= SC_NUM_TRANSFORM_OBJECT_BUTT)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "direction", dos_strlen("direction")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->enDirection);

            if (lRet < 0 || pstNumTransform->enDirection >= SC_NUM_TRANSFORM_DIRECTION_BUTT)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "transform_timing", dos_strlen("transform_timing")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->enTiming);

            if (lRet < 0 || pstNumTransform->enTiming >= SC_NUM_TRANSFORM_TIMING_BUTT)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "num_selection", dos_strlen("num_selection")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->enNumSelect);

            if (lRet < 0 || pstNumTransform->enNumSelect >= SC_NUM_TRANSFORM_SELECT_BUTT)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "callee_prefixion", dos_strlen("callee_prefixion")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstNumTransform->szCalleePrefix, aszValues[lIndex], sizeof(pstNumTransform->szCalleePrefix));
                pstNumTransform->szCalleePrefix[sizeof(pstNumTransform->szCalleePrefix) - 1] = '\0';
            }
            else
            {
                pstNumTransform->szCalleePrefix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "caller_prefixion", dos_strlen("caller_prefixion")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstNumTransform->szCallerPrefix, aszValues[lIndex], sizeof(pstNumTransform->szCallerPrefix));
                pstNumTransform->szCallerPrefix[sizeof(pstNumTransform->szCallerPrefix) - 1] = '\0';
            }
            else
            {
                pstNumTransform->szCallerPrefix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "replace_all", dos_strlen("replace_all")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex]))
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }

            if (aszValues[lIndex][0] == '0')
            {
                pstNumTransform->bReplaceAll = DOS_FALSE;
            }
            else
            {
                pstNumTransform->bReplaceAll = DOS_TRUE;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "replace_num", dos_strlen("replace_num")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstNumTransform->szReplaceNum, aszValues[lIndex], sizeof(pstNumTransform->szReplaceNum));
                pstNumTransform->szReplaceNum[sizeof(pstNumTransform->szReplaceNum) - 1] = '\0';
            }
            else
            {
                pstNumTransform->szReplaceNum[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "del_left", dos_strlen("del_left")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->ulDelLeft);
            if (lRet < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "del_right", dos_strlen("del_right")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->ulDelRight);
            if (lRet < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "add_prefixion", dos_strlen("add_prefixion")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstNumTransform->szAddPrefix, aszValues[lIndex], sizeof(pstNumTransform->szAddPrefix));
                pstNumTransform->szAddPrefix[sizeof(pstNumTransform->szAddPrefix) - 1] = '\0';
            }
            else
            {
                pstNumTransform->szAddPrefix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "add_suffix", dos_strlen("add_suffix")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstNumTransform->szAddSuffix, aszValues[lIndex], sizeof(pstNumTransform->szAddSuffix));
                pstNumTransform->szAddSuffix[sizeof(pstNumTransform->szAddSuffix) - 1] = '\0';
            }
            else
            {
                pstNumTransform->szAddSuffix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "priority", dos_strlen("priority")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->enPriority);
            if (lRet < 0 || pstNumTransform->enPriority >= SC_NUM_TRANSFORM_PRIORITY_BUTT)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "expiry", dos_strlen("expiry")))
        {
            lRet = dos_atoul(aszValues[lIndex], &pstNumTransform->ulExpiry);
            if (lRet < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }

            if (0 == pstNumTransform->ulExpiry)
            {
                /* 无限制 */
                pstNumTransform->ulExpiry = U32_BUTT;
            }
        }
        else
        {
            DOS_ASSERT(0);

            blProcessOK = DOS_FALSE;
            break;
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstNumTransform);
        pstNumTransform = NULL;

        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexNumTransformList);
    pstListNode = dll_find(&g_stNumTransformList, &pstNumTransform->ulID, sc_transform_find);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        pstListNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstListNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstNumTransform);
            pstNumTransform = NULL;

            pthread_mutex_unlock(&g_mutexNumTransformList);

            return DOS_FAIL;
        }

        DLL_Init_Node(pstListNode);
        pstListNode->pHandle = pstNumTransform;
        pstNumTransform->bExist = DOS_TRUE;
        DLL_Add(&g_stNumTransformList, pstListNode);
    }
    else
    {
        pstNumTransformTmp = pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstNumTransformTmp))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstNumTransform);
            pstNumTransform = NULL;

            pthread_mutex_unlock(&g_mutexNumTransformList);
            return DOS_FAIL;
        }

        dos_memcpy(pstNumTransformTmp, pstNumTransform, sizeof(SC_NUM_TRANSFORM_NODE_ST));
        pstNumTransform->bExist = DOS_TRUE;

        dos_dmem_free(pstNumTransform);
        pstNumTransform = NULL;
    }
    pthread_mutex_unlock(&g_mutexNumTransformList);

    return DOS_TRUE;
}


U32 sc_transform_load(U32 ulIndex)
{
    S8 szSQL[1024];
    S32 lRet;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, object, object_id, direction, transform_timing, num_selection, caller_prefixion, callee_prefixion, replace_all, replace_num, del_left, del_right, add_prefixion, add_suffix, priority, expiry FROM tbl_num_transformation  ORDER BY tbl_num_transformation.priority ASC;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, object, object_id, direction, transform_timing, num_selection, caller_prefixion, callee_prefixion, replace_all, replace_num, del_left, del_right, add_prefixion, add_suffix, priority, expiry FROM tbl_num_transformation where tbl_num_transformation.id=%u;"
                    , ulIndex);
    }

    lRet = db_query(g_pstSCDBHandle, szSQL, sc_transform_load_cb, NULL, NULL);
    if (DB_ERR_SUCC != lRet)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Load Num Transform FAIL.(ID:%u)", ulIndex);
        return DOS_FAIL;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Load Num Transform SUCC.(ID:%u)", ulIndex);

    return DOS_SUCC;
}

U32 sc_transform_update_proc(U32 ulAction, U32 ulNumTransID)
{
    if (ulAction >= SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (ulAction)
    {
        case SC_API_CMD_ACTION_CALLER_ADD:
        case SC_API_CMD_ACTION_CALLER_UPDATE:
            sc_transform_load(ulNumTransID);
            break;
        case SC_API_CMD_ACTION_CALLER_DELETE:
            sc_transform_delete(ulNumTransID);
            break;
        default:
            break;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Update num transform SUCC.(ulAction:%u, ulID:%d)", ulAction, ulNumTransID);

    return DOS_SUCC;
}


U32 sc_transform_being(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB, U32 ulTrunkID, U32 ulTiming, U32 ulNumSelect, U32 ulDirection)
{
    SC_NUM_TRANSFORM_NODE_ST        *pstNumTransform        = NULL;
    SC_NUM_TRANSFORM_NODE_ST        *pstNumTransformEntry   = NULL;
    DLL_NODE_S                      *pstListNode            = NULL;
    S8  szNeedTransformNum[SC_NUM_LENGTH]                   = {0};
    U32 ulIndex         = 0;
    S32 lIndex          = 0;
    U32 ulNumLen        = 0;
    U32 ulOffsetLen     = 0;
    U32 ulCallerGrpID   = 0;
    time_t ulCurrTime   = time(NULL);

    if (DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstLCB)
        || ulTiming >= SC_NUM_TRANSFORM_TIMING_BUTT
        || ulNumSelect >= SC_NUM_TRANSFORM_SELECT_BUTT
        || ulDirection >= SC_NUM_TRANSFORM_DIRECTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Search number transfer rule, timing is : %d, number select : %d"
                                , ulTiming, ulNumSelect);

    /* 遍历号码变换规则的链表，查找没有过期的，优先级别高的，针对这个客户或者系统的变换规则。
        先按优先级，同优先级，客户优先于中继，中继优先于系统 */
    pthread_mutex_lock(&g_mutexNumTransformList);
    DLL_Scan(&g_stNumTransformList, pstListNode, DLL_NODE_S *)
    {
        pstNumTransformEntry = (SC_NUM_TRANSFORM_NODE_ST *)pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstNumTransformEntry))
        {
            continue;
        }

        /* 判断有效期 */
        if (pstNumTransformEntry->ulExpiry < ulCurrTime)
        {
            continue;
        }

        /* 判断 路由前/后 */
        if (pstNumTransformEntry->enTiming != ulTiming)
        {
            continue;
        }
        /* 判断 呼叫方向 */
        if (pstNumTransformEntry->enDirection != ulDirection)
        {
            continue;
        }
        /* 判断主被叫 */
        if (pstNumTransformEntry->enNumSelect != ulNumSelect)
        {
            continue;
        }

        /* 判断主叫号码前缀 */
        if ('\0' != pstNumTransformEntry->szCallerPrefix[0])
        {
            if (0 != dos_strnicmp(pstNumTransformEntry->szCallerPrefix, pstLCB->stCall.stNumInfo.szOriginalCalling, dos_strlen(pstNumTransformEntry->szCallerPrefix)))
            {
                continue;
            }
        }

        /* 判断被叫号码前缀 */
        if ('\0' != pstNumTransformEntry->szCalleePrefix[0])
        {
            if (0 != dos_strnicmp(pstNumTransformEntry->szCalleePrefix, pstLCB->stCall.stNumInfo.szOriginalCallee, dos_strlen(pstNumTransformEntry->szCalleePrefix)))
            {
                continue;
            }
        }

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Call Object : %d", pstNumTransformEntry->enObject);

        if (SC_NUM_TRANSFORM_OBJECT_CUSTOMER == pstNumTransformEntry->enObject)
        {
            /* 针对客户 */
            if (pstNumTransformEntry->ulObjectID == pstSCB->ulCustomerID)
            {
                if (DOS_ADDR_INVALID(pstNumTransform))
                {
                    pstNumTransform = pstNumTransformEntry;
                    continue;
                }

                if (pstNumTransformEntry->enPriority < pstNumTransform->enPriority)
                {
                    /* 选择优先级高的 */
                    pstNumTransform = pstNumTransformEntry;

                    continue;
                }

                if (pstNumTransformEntry->enPriority == pstNumTransform->enPriority && pstNumTransform->enObject != SC_NUM_TRANSFORM_OBJECT_CUSTOMER)
                {
                    /* 优先级相同，选择客户的 */
                    pstNumTransform = pstNumTransformEntry;

                    continue;
                }
            }
        }
        else if (SC_NUM_TRANSFORM_OBJECT_SYSTEM == pstNumTransformEntry->enObject)
        {
            /* 针对系统 */
            if (DOS_ADDR_INVALID(pstNumTransform))
            {
                pstNumTransform = pstNumTransformEntry;

                continue;
            }

            if (pstNumTransformEntry->enPriority < pstNumTransform->enPriority)
            {
                /* 选择优先级高的 */
                pstNumTransform = pstNumTransformEntry;

                continue;
            }
        }
        else if (SC_NUM_TRANSFORM_OBJECT_TRUNK == pstNumTransformEntry->enObject)
        {
            /* 针对中继，只有路由后，才需要判断这种情况 */
            if (ulTiming == SC_NUM_TRANSFORM_TIMING_AFTER)
            {
                if (pstNumTransformEntry->ulObjectID != ulTrunkID)
                {
                    continue;
                }

                if (DOS_ADDR_INVALID(pstNumTransform))
                {
                    pstNumTransform = pstNumTransformEntry;
                    continue;
                }

                if (pstNumTransformEntry->enPriority < pstNumTransform->enPriority)
                {
                    /* 选择优先级高的 */
                    pstNumTransform = pstNumTransformEntry;

                    continue;
                }

                if (pstNumTransformEntry->enPriority == pstNumTransform->enPriority && pstNumTransform->enObject == SC_NUM_TRANSFORM_OBJECT_SYSTEM)
                {
                    /* 优先级相同，如果是系统的，则换成中继的 */
                    pstNumTransform = pstNumTransformEntry;

                    continue;
                }
            }
        }
    }

    if (DOS_ADDR_INVALID(pstNumTransform))
    {
        /* 没有找到合适的变换规则 */
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Not find number transfer rule. timing is : %d, number select : %d"
                                , ulTiming, ulNumSelect);

        if (SC_NUM_TRANSFORM_SELECT_CALLER == ulNumSelect)
        {
            if (pstLCB->stCall.stNumInfo.szRealCalling[0] == '\0')
            {
                dos_strcpy(pstLCB->stCall.stNumInfo.szRealCalling, pstLCB->stCall.stNumInfo.szOriginalCalling);
            }
        }
        else
        {
            if (pstLCB->stCall.stNumInfo.szRealCallee[0] == '\0')
            {
                dos_strcpy(pstLCB->stCall.stNumInfo.szRealCallee, pstLCB->stCall.stNumInfo.szOriginalCallee);
            }
        }

        goto succ;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Find a number transfer rule(%d), timing is : %d, number select : %d"
                                , pstNumTransform->ulID, ulTiming, ulNumSelect);

    if (SC_NUM_TRANSFORM_SELECT_CALLER == ulNumSelect)
    {
        dos_strncpy(szNeedTransformNum, pstLCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
    }
    else
    {
        dos_strncpy(szNeedTransformNum, pstLCB->stCall.stNumInfo.szOriginalCallee, SC_NUM_LENGTH);
    }

    szNeedTransformNum[SC_NUM_LENGTH - 1] = '\0';

    /* 根据找到的规则变换号码 */
    if (pstNumTransform->bReplaceAll)
    {
        /* 完全替代 */
        if (pstNumTransform->szReplaceNum[0] == '*')
        {
            /* 使用号码组中的号码 */
            if (SC_NUM_TRANSFORM_OBJECT_CUSTOMER != pstNumTransform->enObject || ulNumSelect != SC_NUM_TRANSFORM_SELECT_CALLER)
            {
                /* 只有企业客户 和 变换主叫号码时，才可以选择号码组中的号码进行替换 */
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Number transfer rule(%d) fail : only a enterprise customers can, choose number in the group number"
                                , pstNumTransform->ulID, ulTiming, ulNumSelect);

                goto fail;
            }

            if (dos_atoul(&pstNumTransform->szReplaceNum[1], &ulCallerGrpID) < 0)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Number transfer rule(%d), get caller group id fail.", pstNumTransform->ulID);

                goto fail;
            }

            if (sc_select_number_in_order(pstSCB->ulCustomerID, ulCallerGrpID, szNeedTransformNum, SC_NUM_LENGTH) != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Number transfer rule(%d), get caller from group id(%u) fail.", pstNumTransform->ulID, ulCallerGrpID);

                goto fail;
            }

            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Number transfer rule(%d), get caller(%s) from group id(%u) succ.", pstNumTransform->ulID, szNeedTransformNum, ulCallerGrpID);

        }
        else if (pstNumTransform->szReplaceNum[0] == '\0')
        {
            /* 完全替换的号码不能为空 */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "The number transfer rule(%d) replace num is NULL!"
                                , pstNumTransform->ulID);

            goto fail;
        }
        else
        {
            dos_strcpy(szNeedTransformNum, pstNumTransform->szReplaceNum);
        }

        goto succ;
    }

    /* 删除左边几位 */
    ulOffsetLen = pstNumTransform->ulDelLeft;
    if (ulOffsetLen != 0)
    {
        ulNumLen = dos_strlen(szNeedTransformNum);

        if (ulOffsetLen >= ulNumLen)
        {
            /* 删除的位数大于号码的长度，整个号码置空 */
            szNeedTransformNum[0] = '\0';
        }
        else
        {
            for (ulIndex=ulOffsetLen; ulIndex<=ulNumLen; ulIndex++)
            {
                szNeedTransformNum[ulIndex-ulOffsetLen] = szNeedTransformNum[ulIndex];
            }
        }
    }

    /* 删除右边几位 */
    ulOffsetLen = pstNumTransform->ulDelRight;
    if (ulOffsetLen != 0)
    {
        ulNumLen = dos_strlen(szNeedTransformNum);

        if (ulOffsetLen >= ulNumLen)
        {
            /* 删除的位数大于号码的长度，整个号码置空 */
            szNeedTransformNum[0] = '\0';
        }
        else
        {
            szNeedTransformNum[ulNumLen-ulOffsetLen] = '\0';
        }
    }

    /* 增加前缀 */
    if (pstNumTransform->szAddPrefix[0] != '\0')
    {
        ulNumLen = dos_strlen(szNeedTransformNum);
        ulOffsetLen = dos_strlen(pstNumTransform->szAddPrefix);
        if (ulNumLen + ulOffsetLen >= SC_NUM_LENGTH)
        {
            /* 超过号码的长度，失败 */

            goto fail;
        }

        for (lIndex=ulNumLen; lIndex>=0; lIndex--)
        {
            szNeedTransformNum[lIndex+ulOffsetLen] = szNeedTransformNum[lIndex];
        }

        dos_strncpy(szNeedTransformNum, pstNumTransform->szAddPrefix, ulOffsetLen);
    }

    /* 增加后缀 */
    if (pstNumTransform->szAddSuffix[0] != '\0')
    {
        ulNumLen = dos_strlen(szNeedTransformNum);
        ulOffsetLen = dos_strlen(pstNumTransform->szAddSuffix);
        if (ulNumLen + ulOffsetLen >= SC_NUM_LENGTH)
        {
            /* 超过号码的长度，失败 */

            goto fail;
        }

        dos_strcat(szNeedTransformNum, pstNumTransform->szAddSuffix);
    }

    if (szNeedTransformNum[0] == '\0')
    {
        goto fail;
    }
    szNeedTransformNum[SC_NUM_LENGTH - 1] = '\0';

succ:

    if (DOS_ADDR_INVALID(pstNumTransform))
    {
        pthread_mutex_unlock(&g_mutexNumTransformList);

        return DOS_SUCC;
    }

    if (SC_NUM_TRANSFORM_SELECT_CALLER == ulNumSelect)
    {
        if (DOS_ADDR_VALID(pstNumTransform))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "The number transfer(%d) SUCC, befor : %s ,after : %s", pstNumTransform->ulID, pstLCB->stCall.stNumInfo.szOriginalCalling, szNeedTransformNum);
        }

        dos_strcpy(pstLCB->stCall.stNumInfo.szRealCalling, szNeedTransformNum);
    }
    else
    {
        if (DOS_ADDR_VALID(pstNumTransform))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "The number transfer(%d) SUCC, befor : %s ,after : %s", pstNumTransform->ulID, pstLCB->stCall.stNumInfo.szOriginalCallee, szNeedTransformNum);
        }
        dos_strcpy(pstLCB->stCall.stNumInfo.szRealCallee, szNeedTransformNum);
    }

    pthread_mutex_unlock(&g_mutexNumTransformList);

    return DOS_SUCC;

fail:
    if (DOS_ADDR_VALID(pstNumTransform))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "the number transfer(%d) FAIL", pstNumTransform->ulID);
    }

    if (SC_NUM_TRANSFORM_SELECT_CALLER == ulNumSelect)
    {
        pstLCB->stCall.stNumInfo.szRealCalling[0] = '\0';
    }
    else
    {
        pstLCB->stCall.stNumInfo.szRealCallee[0] = '\0';
    }

    pthread_mutex_unlock(&g_mutexNumTransformList);

    return DOS_FAIL;
}

/**
 * 函数: VOID sc_ep_gw_init(SC_GW_NODE_ST *pstGW)
 * 功能: 初始化pstGW所指向的网关描述结构
 * 参数:
 *      SC_GW_NODE_ST *pstGW 需要别初始化的网关描述结构
 * 返回值: VOID
 */
VOID sc_black_list_init(SC_BLACK_LIST_NODE *pstBlackListNode)
{
    if (pstBlackListNode)
    {
        dos_memzero(pstBlackListNode, sizeof(SC_BLACK_LIST_NODE));
        pstBlackListNode->ulID = U32_BUTT;
        pstBlackListNode->ulCustomerID = U32_BUTT;
        pstBlackListNode->enType = SC_NUM_BLACK_BUTT;
        pstBlackListNode->szNum[0] = '\0';
    }
}

/* 查找黑名单 */
S32 sc_black_list_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_BLACK_LIST_NODE *pstBlackList = NULL;
    U32  ulID;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulID = *(U32 *)pObj;
    pstBlackList = pstHashNode->pHandle;

    if (ulID == pstBlackList->ulID)
    {
        return DOS_SUCC;
    }


    return DOS_FAIL;
}

/**
 * 函数: S32 sc_load_black_list_cb()
 * 功能: 加载Black时数据库查询的回调函数，将数据加入黑名单hash表
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_black_list_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_BLACK_LIST_NODE *pstBlackListNode = NULL;
    SC_BLACK_LIST_NODE *pstBlackListTmp  = NULL;
    HASH_NODE_S        *pstHashNode      = NULL;
    BOOL               blProcessOK       = DOS_TRUE;
    S32                lIndex            = 0;
    U32                ulHashIndex       = 0;


    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstBlackListNode = dos_dmem_alloc(sizeof(SC_BLACK_LIST_NODE));
    if (DOS_ADDR_INVALID(pstBlackListNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_black_list_init(pstBlackListNode);

    for (blProcessOK = DOS_FALSE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || dos_atoul(aszValues[lIndex], &pstBlackListNode->ulID) < 0)
            {
                blProcessOK = DOS_FALSE;

                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || dos_atoul(aszValues[lIndex], &pstBlackListNode->ulCustomerID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "file_id", dos_strlen("file_id")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || dos_atoul(aszValues[lIndex], &pstBlackListNode->ulFileID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "number", dos_strlen("number")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstBlackListNode->szNum, aszValues[lIndex], sizeof(pstBlackListNode->szNum));
            pstBlackListNode->szNum[sizeof(pstBlackListNode->szNum) - 1] = '\0';
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "type", dos_strlen("type")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || dos_atoul(aszValues[lIndex], &pstBlackListNode->enType) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }

        blProcessOK = DOS_TRUE;
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstBlackListNode);
        return DOS_FAIL;
    }

    ulHashIndex = sc_int_hash_func(pstBlackListNode->ulID, SC_BLACK_LIST_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashBlackList, ulHashIndex, (VOID *)&pstBlackListNode->ulID, sc_black_list_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode ))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstBlackListNode);
            pstBlackListNode = NULL;
            return DOS_FAIL;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstBlackListNode;
        ulHashIndex = sc_int_hash_func(pstBlackListNode->ulID, SC_BLACK_LIST_HASH_SIZE);

        pthread_mutex_lock(&g_mutexHashBlackList);
        hash_add_node(g_pstHashBlackList, pstHashNode, ulHashIndex, NULL);
        pthread_mutex_unlock(&g_mutexHashBlackList);
    }
    else
    {
        pstBlackListTmp = pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstBlackListTmp))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstBlackListNode);
            pstBlackListNode = NULL;
            return DOS_FAIL;
        }

        pstBlackListTmp->enType = pstBlackListNode->enType;


        dos_strncpy(pstBlackListTmp->szNum, pstBlackListNode->szNum, sizeof(pstBlackListTmp->szNum));
        pstBlackListTmp->szNum[sizeof(pstBlackListTmp->szNum) - 1] = '\0';
        dos_dmem_free(pstBlackListNode);
        pstBlackListNode = NULL;
    }

    return DOS_SUCC;
}


/**
 * 函数: U32 sc_load_black_list(U32 ulIndex)
 * 功能: 加载黑名单数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_black_list_load(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, file_id, number, type FROM tbl_blacklist_pool;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, file_id, number, type FROM tbl_blacklist_pool WHERE file_id=%u;", ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_black_list_load_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "%s", "Load black list fail.");
        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Load black list succ.");

    return DOS_SUCC;
}

BOOL sc_black_regular_check(S8 *szRegularNum, S8 *szNum)
{
    S8 aszRegular[SC_NUM_LENGTH][SC_SINGLE_NUMBER_SRT_LEN] = {{0}};
    S8 szAllNum[SC_SINGLE_NUMBER_SRT_LEN] = "0123456789";
    S32 lIndex = 0;
    U32 ulLen  = 0;
    S8 *pszPos = szRegularNum;
    S8 ucChar;
    S32 i = 0;

    if (DOS_ADDR_INVALID(szRegularNum) || DOS_ADDR_INVALID(szNum))
    {
        /* 按照匹配不成功处理 */
        return DOS_TRUE;
    }

    while (*pszPos != '\0' && lIndex < SC_NUM_LENGTH)
    {
        ucChar = *pszPos;

        if (ucChar == '*')
        {
            dos_strcpy(aszRegular[lIndex], szAllNum);
        }
        else if (dos_strchr(szAllNum, ucChar) != NULL)
        {
            /* 0-9 */
            aszRegular[lIndex][0] = ucChar;
            aszRegular[lIndex][1] = '\0';
        }
        else if (ucChar == '[')
        {
            aszRegular[lIndex][0] = '\0';
            pszPos++;
            while (*pszPos != ']' && *pszPos != '\0')
            {
                if (dos_strchr(szAllNum, *pszPos) != NULL)
                {
                    /* 0-9, 先判断一下是否已经存在 */
                    if (dos_strchr(aszRegular[lIndex], *pszPos) == NULL)
                    {
                        ulLen = dos_strlen(aszRegular[lIndex]);
                        aszRegular[lIndex][ulLen] = *pszPos;
                        aszRegular[lIndex][ulLen+1] = '\0';
                    }
                }
                pszPos++;
            }

            if (*pszPos == '\0')
            {
                /* 正则表达式错误, 按照不匹配来处理 */
                return DOS_TRUE;
            }
        }
        else
        {
            /* 正则表达式错误, 按照不匹配来处理 */
            return DOS_TRUE;
        }

        pszPos++;
        lIndex++;
    }

    /* 正则表达式解析完成，比较号码是否满足正则表达式。首先比较长度，lIndex即为正则表达式的长度 */
    if (dos_strlen(szNum) != lIndex)
    {
        return DOS_TRUE;
    }

    for (i=0; i<lIndex; i++)
    {
        if (dos_strchr(aszRegular[i], *(szNum+i)) == NULL)
        {
            /* 不匹配 */
            return DOS_TRUE;
        }
    }

    return DOS_FALSE;
}

/**
 * 判断pszNum所指定的号码是否在黑名单中
 *
 * @param S8 *pszNum : 需要被处理的号码
 *
 * @retrun 成功返DOS_TRUE，否则返回DOS_FALSE
 */
BOOL sc_black_list_check(U32 ulCustomerID, S8 *pszNum)
{
    U32                ulHashIndex;
    HASH_NODE_S        *pstHashNode = NULL;
    SC_BLACK_LIST_NODE *pstBlackListNode = NULL;


    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Check num %s is in black list for customer %u", pszNum, ulCustomerID);

    pthread_mutex_lock(&g_mutexHashBlackList);
    HASH_Scan_Table(g_pstHashBlackList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashBlackList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                DOS_ASSERT(0);
                break;
            }

            pstBlackListNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstBlackListNode))
            {
                DOS_ASSERT(0);
                continue;
            }

            if (SC_TOP_USER_ID != pstBlackListNode->ulCustomerID
                && ulCustomerID != pstBlackListNode->ulCustomerID)
            {
                continue;
            }

            if (SC_NUM_BLACK_REGULAR == pstBlackListNode->enType)
            {
                /* 正则号码 */
                if (sc_black_regular_check(pstBlackListNode->szNum, pszNum) == DOS_FALSE)
                {
                    /* 匹配成功 */
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Num %s is matched black list item %s, id %u. (Customer:%u)"
                                , pszNum
                                , pstBlackListNode->szNum
                                , pstBlackListNode->ulID
                                , ulCustomerID);

                    pthread_mutex_unlock(&g_mutexHashBlackList);

                    return DOS_FALSE;
                }
            }
            else
            {
                if (0 == dos_strcmp(pstBlackListNode->szNum, pszNum))
                {
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Num %s is matched black list item %s, id %u. (Customer:%u)"
                                , pszNum
                                , pstBlackListNode->szNum
                                , pstBlackListNode->ulID
                                , ulCustomerID);

                    pthread_mutex_unlock(&g_mutexHashBlackList);
                    return DOS_FALSE;
                }
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashBlackList);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Num %s is not matched any black list. (Customer:%u)", pszNum, ulCustomerID);

    return DOS_TRUE;
}

/**
 * 函数: U32 sc_update_black_list(U32 ulIndex)
 * 功能: 更新黑名单数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_black_list_update(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, file_id, number, type FROM tbl_blacklist_pool WHERE id=%u;", ulIndex);
    if (db_query(g_pstSCDBHandle, szSQL, sc_black_list_load_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "%s", "Load blacklist fail.");
        return DOS_FAIL;
    }
    return DOS_SUCC;
}


U32 sc_black_list_delete(U32 ulID)
{
    HASH_NODE_S        *pstHashNode  = NULL;
    SC_BLACK_LIST_NODE *pstBlackList = NULL;
    U32  ulHashIndex = 0;

    pthread_mutex_lock(&g_mutexHashBlackList);

    HASH_Scan_Table(g_pstHashBlackList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashBlackList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstBlackList = (SC_BLACK_LIST_NODE *)pstHashNode->pHandle;
            /* 如果找到和该fileID相同，则从哈希表中删除*/
            if (pstBlackList->ulID == ulID)
            {
                hash_delete_node(g_pstHashBlackList, pstHashNode, ulHashIndex);
                dos_dmem_free(pstHashNode);
                pstHashNode = NULL;

                dos_dmem_free(pstBlackList);
                pstBlackList = NULL;
                break;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashBlackList);

    return DOS_SUCC;
}

U32 sc_black_list_update_proc(U32 ulAction, U32 ulBlackID)
{
    U32 ulRet = U32_BUTT;

    if (ulAction > SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch(ulAction)
    {
        case SC_API_CMD_ACTION_BLACK_ADD:
            sc_black_list_load(ulBlackID);
            break;
        case SC_API_CMD_ACTION_BLACK_UPDATE:
            /* 注明: 该ID为黑名单在数据库中的索引 */
            sc_black_list_update(ulBlackID);
            break;
        case SC_API_CMD_ACTION_BLACK_DELETE:
        {
            /*注明: 该参数代表的是黑名单文件ID*/
            ulRet = sc_black_list_delete(ulBlackID);
            if (ulRet != DOS_SUCC)
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
            break;
        }
        default:
            break;
    }

    return DOS_SUCC;
}


/**
 * 函数: VOID sc_ep_caller_init(SC_CALLER_QUERY_NODE_ST  *pstCaller)
 * 功能: 初始化pstCaller所指向的主叫号码描述结构
 * 参数:
 *      SC_CALLER_QUERY_NODE_ST  *pstCaller 需要别初始化的主叫号码描述结构
 * 返回值: VOID
 */
VOID sc_caller_init(SC_CALLER_QUERY_NODE_ST  *pstCaller)
{
    if (pstCaller)
    {
        dos_memzero(pstCaller, sizeof(SC_CALLER_QUERY_NODE_ST));
        /* 默认关系跟踪 */
        pstCaller->bTraceON = DOS_FALSE;
        /* 默认可用 */
        pstCaller->bValid = DOS_TRUE;
        pstCaller->ulCustomerID = 0;
        pstCaller->ulIndexInDB = 0;
        /* 号码被命中次数初始化为0 */
        pstCaller->ulTimes = 0;

        dos_memzero(pstCaller->szNumber, sizeof(pstCaller->szNumber));
    }
}


S32 sc_caller_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;
    U32 ulIndex = U32_BUTT;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulIndex = *(U32 *)pObj;
    pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstHashNode->pHandle;

    if (ulIndex == pstCaller->ulIndexInDB)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }
}


/**
 * 函数: S32 sc_load_caller_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * 功能: 加载主叫号码数据
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_caller_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_CALLER_QUERY_NODE_ST  *pstCaller   = NULL, *pstCallerTemp = NULL;
    HASH_NODE_S              *pstHashNode = NULL;
    U32  ulHashIndex = U32_BUTT;
    S32  lIndex = U32_BUTT;
    BOOL blProcessOK = DOS_FALSE;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCaller = (SC_CALLER_QUERY_NODE_ST *)dos_dmem_alloc(sizeof(SC_CALLER_QUERY_NODE_ST));
    if (DOS_ADDR_INVALID(pstCaller))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_caller_init(pstCaller);

    for (blProcessOK = DOS_TRUE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstCaller->ulIndexInDB) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstCaller->ulCustomerID) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "cid", dos_strlen("cid")))
        {
            if ('\0' == aszValues[lIndex][0])
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
            dos_snprintf(pstCaller->szNumber, sizeof(pstCaller->szNumber), "%s", aszValues[lIndex]);
        }
        else
        {
            DOS_ASSERT(0);
            blProcessOK = DOS_FALSE;
            break;
        }
        blProcessOK = DOS_TRUE;
    }
    if (!blProcessOK)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstCaller);
        pstCaller = NULL;
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashCaller);
    ulHashIndex = sc_int_hash_func(pstCaller->ulIndexInDB, SC_CALLER_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashCaller, ulHashIndex, (VOID *)&pstCaller->ulIndexInDB, sc_caller_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = (HASH_NODE_S*)dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstCaller);
            pstCaller = NULL;
            pthread_mutex_unlock(&g_mutexHashCaller);
            return DOS_FAIL;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = (VOID *)pstCaller;
        hash_add_node(g_pstHashCaller, pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        pstCallerTemp = (SC_CALLER_QUERY_NODE_ST *)pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstCallerTemp))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstCaller);
            pstCaller = NULL;
            pthread_mutex_unlock(&g_mutexHashCaller);
            return DOS_FAIL;
        }

        dos_memcpy(pstCallerTemp, pstCaller, sizeof(SC_CALLER_QUERY_NODE_ST));
        dos_dmem_free(pstCaller);
        pstCaller = NULL;
    }
    pthread_mutex_unlock(&g_mutexHashCaller);

    return DOS_SUCC;
}


/**
 * 函数: U32 sc_load_caller(U32 ulIndex)
 * 功能: 加载主叫号码数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_caller_load(U32 ulIndex)
{
    S8   szQuery[256] = {0};
    U32  ulRet = U32_BUTT;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szQuery, sizeof(szQuery), "SELECT id,customer_id,cid FROM tbl_caller WHERE verify_status=1;");
    }
    else
    {
        dos_snprintf(szQuery, sizeof(szQuery), "SELECT id,customer_id,cid FROM tbl_caller WHERE verify_status=1 AND id=%u", ulIndex);
    }
    ulRet = db_query(g_pstSCDBHandle, szQuery, sc_caller_load_cb, NULL, NULL);
    if (DB_ERR_SUCC != ulRet)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Load caller FAIL.(ID:%u)", ulIndex);
        return DOS_FAIL;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Load Caller SUCC.(ID:%u)", ulIndex);

    return DOS_SUCC;
}


U32 sc_caller_delete(U32 ulCallerID)
{
    HASH_NODE_S *pstHashNode = NULL;
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;
    U32  ulHashIndex = U32_BUTT;
    BOOL bFound = DOS_FALSE;

    HASH_Scan_Table(g_pstHashCaller, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashCaller, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstHashNode->pHandle;
            if (ulCallerID == pstCaller->ulIndexInDB)
            {
                bFound = DOS_TRUE;
                break;
            }
        }
        if (DOS_TRUE == bFound)
        {
            break;
        }
    }

    if (DOS_FALSE == bFound)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Delete Caller FAIL.(ulID:%u)", ulCallerID);
        return DOS_FAIL;
    }
    else
    {
        hash_delete_node(g_pstHashCaller, pstHashNode, ulHashIndex);
        dos_dmem_free(pstCaller);
        pstCaller = NULL;

        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Delete Caller SUCC.(ulID:%u)", ulCallerID);

        return DOS_SUCC;
    }
}

/**
 * 函数: VOID sc_ep_caller_grp_init(SC_CALLER_GRP_NODE_ST* pstCallerGrp)
 * 功能: 初始化pstCallerGrp所指向的主叫号码组描述结构
 * 参数:
 *      SC_CALLER_GRP_NODE_ST* pstCallerGrp 需要别初始化的主叫号码组描述结构
 * 返回值: VOID
 */
VOID sc_caller_group_init(SC_CALLER_GRP_NODE_ST* pstCallerGrp)
{
    if (pstCallerGrp)
    {
        dos_memzero(pstCallerGrp, sizeof(SC_CALLER_GRP_NODE_ST));

        /* 初始化为非默认组 */
        pstCallerGrp->bDefault = DOS_FALSE;

        pstCallerGrp->ulID = 0;
        pstCallerGrp->ulCustomerID = 0;
        /* 暂时保留现状 */
        pstCallerGrp->ulLastNo = 0;
        /* 默认使用顺序呼叫策略 */
        pstCallerGrp->ulPolicy = SC_CALLER_SELECT_POLICY_IN_ORDER;
        dos_memzero(pstCallerGrp->szGrpName, sizeof(pstCallerGrp->szGrpName));
        DLL_Init(&pstCallerGrp->stCallerList);

        pthread_mutex_init(&pstCallerGrp->mutexCallerList, NULL);
    }
}

/**
 * 函数: S32 sc_ep_caller_grp_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
 * 功能: 查找关键字,主叫号码组查找函数
 * 参数:
 *      VOID *pObj, : 查找关键字，这里是号码组客户id
 *      HASH_NODE_S *pstHashNode  当前哈希
 * 返回值: 查找成功返回DOS_SUCC，否则返回DOS_FAIL.
 */
S32 sc_caller_group_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    U32  ulID = U32_BUTT;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulID = *(U32 *)pObj;
    pstCallerGrp = pstHashNode->pHandle;

    if (ulID == pstCallerGrp->ulID)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }
}

S32 sc_caller_relationship_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    S32  lIndex = U32_BUTT;
    U32  ulCallerID = U32_BUTT, ulCallerType = U32_BUTT, ulHashIndex = U32_BUTT;
    SC_DID_NODE_ST *pstDid = NULL;
    DLL_NODE_S *pstNode = NULL;
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    SC_CALLER_CACHE_NODE_ST *pstCacheNode = NULL;
    BOOL blProcessOK = DOS_TRUE, bFound = DOS_FALSE;

    if (DOS_ADDR_INVALID(pArg)
        || lCount <= 0
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    for (lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (DOS_ADDR_INVALID(aszNames[lIndex])
            || DOS_ADDR_INVALID(aszValues[lIndex]))
        {
            break;
        }

        if (0 == dos_strnicmp(aszNames[lIndex], "caller_id", dos_strlen("caller_id")))
        {
            if (dos_atoul(aszValues[lIndex], &ulCallerID) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "caller_type", dos_strlen("caller_type")))
        {
            if (dos_atoul(aszValues[lIndex], &ulCallerType) < 0)
            {
                blProcessOK = DOS_FALSE;
            }
        }
    }

    if (!blProcessOK
        || U32_BUTT == ulCallerID)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCacheNode = (SC_CALLER_CACHE_NODE_ST *)dos_dmem_alloc(sizeof(SC_CALLER_CACHE_NODE_ST));
    if (DOS_ADDR_INVALID(pstCacheNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 如果是配置的主叫号码，先查找到对应的主叫号码节点 */
    if (SC_NUMBER_TYPE_CFG == ulCallerType)
    {
        pstCacheNode->ulType = SC_NUMBER_TYPE_CFG;
        ulHashIndex = sc_int_hash_func(ulCallerID, SC_CALLER_HASH_SIZE);
        pstHashNode = hash_find_node(g_pstHashCaller, ulHashIndex, (VOID *)&ulCallerID, sc_caller_hash_find);

        if (DOS_ADDR_INVALID(pstHashNode)
            || DOS_ADDR_INVALID(pstHashNode->pHandle))
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
        pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstHashNode->pHandle;
        pstCacheNode->stData.pstCaller = pstCaller;
    }
    else if (SC_NUMBER_TYPE_DID == ulCallerType)
    {
        pstCacheNode->ulType = SC_NUMBER_TYPE_DID;
        /* 目前由于索引未采用id，故只能遍历哈希表 */
        HASH_Scan_Table(g_pstHashDIDNum, ulHashIndex)
        {
            HASH_Scan_Bucket(g_pstHashDIDNum ,ulHashIndex ,pstHashNode, HASH_NODE_S *)
            {
                if (DOS_ADDR_INVALID(pstHashNode)
                    || DOS_ADDR_INVALID(pstHashNode->pHandle))
                {
                    continue;
                }

                pstDid = (SC_DID_NODE_ST *)pstHashNode->pHandle;
                if (ulCallerID == pstDid->ulDIDID)
                {
                    bFound = DOS_TRUE;
                    break;
                }
            }
            if (DOS_TRUE == bFound)
            {
                break;
            }
        }
        if (DOS_FALSE == bFound)
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
        pstCacheNode->stData.pstDid = pstDid;
    }

    pstNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    DLL_Init_Node(pstNode);
    pstNode->pHandle = (VOID *)pstCacheNode;

    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pArg;
    pthread_mutex_lock(&pstCallerGrp->mutexCallerList);
    DLL_Add(&pstCallerGrp->stCallerList, pstNode);
    pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);
    return DOS_SUCC;
}



U32 sc_caller_relationship_load()
{
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    U32  ulHashIndex = U32_BUTT;
    S32 lRet = U32_BUTT;
    S8   szQuery[256] = {0};
    HASH_NODE_S *pstHashNode = NULL;

    HASH_Scan_Table(g_pstHashCallerGrp, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashCallerGrp, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }
            pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;
            /* 先加载主叫号码，再加载DID号码 */
            dos_snprintf(szQuery, sizeof(szQuery), "SELECT caller_id,caller_type FROM tbl_caller_assign WHERE caller_grp_id=%u;", pstCallerGrp->ulID);
            lRet = db_query(g_pstSCDBHandle, szQuery, sc_caller_relationship_load_cb, (VOID *)pstCallerGrp, NULL);
            if (DB_ERR_SUCC != lRet)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Load Caller RelationShip FAIL.(CallerGrpID:%u)", pstCallerGrp->ulID);
                DOS_ASSERT(0);
                return DOS_FAIL;
            }
        }
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Load Caller relationship SUCC.");

    return DOS_SUCC;
}

/**
 * 函数: S32 sc_load_caller_grp_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * 功能: 加载主叫号码数据
 * 参数:
 *      VOID *pArg: 参数
 *      S32 lCount: 列数量
 *      S8 **aszValues: 值裂变
 *      S8 **aszNames: 字段名列表
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 sc_caller_group_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_CALLER_GRP_NODE_ST   *pstCallerGrp = NULL, *pstCallerGrpTemp = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    U32  ulHashIndex = U32_BUTT;
    S32  lIndex = U32_BUTT;
    BOOL blProcessOK = DOS_FALSE;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)dos_dmem_alloc(sizeof(SC_CALLER_GRP_NODE_ST));
    if (DOS_ADDR_INVALID(pstCallerGrp))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_caller_group_init(pstCallerGrp);

    for (blProcessOK = DOS_TRUE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstCallerGrp->ulID) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstCallerGrp->ulCustomerID) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "name", dos_strlen("name")))
        {
            if ('\0' == aszValues[lIndex][0])
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
            dos_snprintf(pstCallerGrp->szGrpName, sizeof(pstCallerGrp->szGrpName), "%s", aszValues[lIndex]);
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "policy", dos_strlen("policy")))
        {
            if (dos_atoul(aszValues[lIndex], &pstCallerGrp->ulPolicy) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else
        {
            DOS_ASSERT(0);
            blProcessOK = DOS_FALSE;
            break;
        }
    }
    if (!blProcessOK)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashCallerGrp);
    ulHashIndex = sc_int_hash_func(pstCallerGrp->ulID, SC_CALLER_GRP_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&pstCallerGrp->ulID, sc_caller_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstCallerGrp);
            pstCallerGrp = NULL;
            pthread_mutex_unlock(&g_mutexHashCallerGrp);
            return DOS_FAIL;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = (VOID *)pstCallerGrp;
        hash_add_node(g_pstHashCallerGrp, pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        pstCallerGrpTemp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstCallerGrpTemp))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstCallerGrp);
            pstCallerGrp = NULL;
            pthread_mutex_unlock(&g_mutexHashCallerGrp);
            return DOS_FAIL;
        }
        dos_memcpy(pstCallerGrpTemp, pstCallerGrp, sizeof(SC_CALLER_QUERY_NODE_ST));
        dos_dmem_free(pstCallerGrp);
        pstCallerGrp = NULL;
    }
    pthread_mutex_unlock(&g_mutexHashCallerGrp);

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_load_caller_grp(U32 ulIndex)
 * 功能: 加载主叫号码组数据
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 sc_caller_group_load(U32 ulIndex)
{
    S8 szSQL[1024] = {0};
    U32 ulRet = U32_BUTT;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id,customer_id,name,policy FROM tbl_caller_grp;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id,customer_id,name,policy FROM tbl_caller_grp WHERE id=%u;"
                        , ulIndex);
    }

    ulRet = db_query(g_pstSCDBHandle, szSQL, sc_caller_group_load_cb, NULL, NULL);
    if (ulRet != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Load caller group FAIL.(ID:%u)", ulIndex);
        return DOS_FAIL;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Load caller group SUCC.(ID:%u)", ulIndex);

    return ulRet;
}

U32 sc_caller_group_delete(U32 ulCallerGrpID)
{
    U32  ulHashIndex = U32_BUTT;
    HASH_NODE_S  *pstHashNode = NULL;
    DLL_NODE_S *pstListNode = NULL;
    SC_CALLER_GRP_NODE_ST  *pstCallerGrp = NULL;

    ulHashIndex = sc_int_hash_func(ulCallerGrpID, SC_CALLER_GRP_HASH_SIZE);
    pthread_mutex_lock(&g_mutexHashCallerGrp);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulCallerGrpID, sc_caller_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashCallerGrp);

        sc_log(LOG_LEVEL_EMERG, "Cannot Find Caller Grp %u.", ulCallerGrpID);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;
    pthread_mutex_lock(&pstCallerGrp->mutexCallerList);
    while (1)
    {
        if (DLL_Count(&pstCallerGrp->stCallerList) == 0)
        {
            break;
        }
        pstListNode = dll_fetch(&pstCallerGrp->stCallerList);
        if (DOS_ADDR_VALID(pstListNode->pHandle))
        {
            dos_dmem_free(pstListNode->pHandle);
            pstListNode->pHandle = NULL;
        }

        dll_delete(&pstCallerGrp->stCallerList, pstListNode);
        dos_dmem_free(pstListNode);
        pstListNode = NULL;
    }
    pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);

    hash_delete_node(g_pstHashCallerGrp, pstHashNode, ulHashIndex);
    pthread_mutex_unlock(&g_mutexHashCallerGrp);
    dos_dmem_free(pstHashNode->pHandle);
    pstHashNode->pHandle = NULL;
    dos_dmem_free(pstHashNode);
    pstHashNode = NULL;
    return DOS_SUCC;
}

U32 sc_caller_group_refresh(U32 ulIndex)
{
    U32  ulHashIndex = U32_BUTT;
    SC_CALLER_GRP_NODE_ST  *pstCallerGrp = NULL;
    DLL_NODE_S *pstListNode = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    SC_CALLER_CACHE_NODE_ST *pstCache = NULL;
    S8  szQuery[256] = {0,};
    U32 ulRet = 0;

    sc_caller_group_load(ulIndex);

    ulHashIndex = sc_int_hash_func(ulIndex, SC_CALLER_GRP_HASH_SIZE);
    pthread_mutex_lock(&g_mutexHashCallerGrp);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulIndex, sc_caller_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pthread_mutex_unlock(&g_mutexHashCallerGrp);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "SC Refresh Caller Grp FAIL.(CallerGrpID:%u)", ulIndex);
        return DOS_FAIL;
    }
    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;

    pthread_mutex_lock(&pstCallerGrp->mutexCallerList);
    while (1)
    {
        if (DLL_Count(&pstCallerGrp->stCallerList) == 0)
        {
            break;
        }

        pstListNode = dll_fetch(&pstCallerGrp->stCallerList);
        if (DOS_ADDR_INVALID(pstListNode))
        {
            continue;
        }

        pstCache = (SC_CALLER_CACHE_NODE_ST *)pstListNode->pHandle;
        if (DOS_ADDR_VALID(pstCache))
        {
            dos_dmem_free(pstCache);
            pstCache = NULL;
        }
        dll_delete(&pstCallerGrp->stCallerList, pstListNode);
        dos_dmem_free(pstListNode);
        pstListNode = NULL;
    }

    pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT caller_id,caller_type FROM tbl_caller_assign WHERE caller_grp_id=%u;", ulIndex);

    ulRet = db_query(g_pstSCDBHandle, szQuery, sc_caller_relationship_load_cb, (VOID *)pstCallerGrp, NULL);

    pthread_mutex_unlock(&g_mutexHashCallerGrp);
    return ulRet;
}


U32 sc_caller_group_update_proc(U32 ulAction, U32 ulCallerGrpID)
{
    if (ulAction >= SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (ulAction)
    {
        case SC_API_CMD_ACTION_CALLER_GRP_ADD:
        case SC_API_CMD_ACTION_CALLER_GRP_UPDATE:
            sc_caller_group_refresh(ulCallerGrpID);
            break;
        case SC_API_CMD_ACTION_CALLER_GRP_DELETE:
            sc_caller_group_delete(ulCallerGrpID);
            break;
        default:
            break;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Update Caller Group SUCC.(ulAction:%u, ulCallerGrpID:%u)", ulAction, ulCallerGrpID);

    return DOS_SUCC;
}

U32 sc_caller_update_proc(U32 ulAction, U32 ulCallerID)
{
    if (ulAction >= SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (ulAction)
    {
        case SC_API_CMD_ACTION_CALLER_ADD:
        case SC_API_CMD_ACTION_CALLER_UPDATE:
            sc_caller_load(ulCallerID);
            break;
        case SC_API_CMD_ACTION_CALLER_DELETE:
            sc_caller_delete(ulCallerID);
            break;
        default:
            break;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Update Caller SUCC.(ulAction:%u, ulID:%d)", ulAction, ulCallerID);

    return DOS_SUCC;
}



VOID sc_eix_dev_init(SC_TT_NODE_ST *pstTTNumber)
{
    if (pstTTNumber)
    {
        pstTTNumber->ulID = U32_BUTT;
        pstTTNumber->szAddr[0] = '\0';
        pstTTNumber->szPrefix[0] = '\0';
        pstTTNumber->ulPort = 0;
    }
}

S32 sc_eix_dev_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_TT_NODE_ST *pstTTNumber = NULL;
    U32  ulID;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulID = *(U32 *)pObj;
    pstTTNumber = pstHashNode->pHandle;

    if (ulID == pstTTNumber->ulID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}


S32 sc_eix_dev_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    HASH_NODE_S        *pstHashNode      = NULL;
    SC_TT_NODE_ST      *pstSCTTNumber    = NULL;
    SC_TT_NODE_ST      *pstSCTTNumberOld = NULL;
    BOOL               blProcessOK       = DOS_FALSE;
    U32                ulHashIndex       = 0;
    S32                lIndex            = 0;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstSCTTNumber = (SC_TT_NODE_ST *)dos_dmem_alloc(sizeof(SC_TT_NODE_ST));
    if (DOS_ADDR_INVALID(pstSCTTNumber))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_eix_dev_init(pstSCTTNumber);

    for (lIndex=0, blProcessOK=DOS_TRUE; lIndex<lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSCTTNumber->ulID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "ip", dos_strlen("ip")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }
            dos_snprintf(pstSCTTNumber->szAddr, sizeof(pstSCTTNumber->szAddr), "%s", aszValues[lIndex]);
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "port", dos_strlen("port")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0]
                || dos_atoul(aszValues[lIndex], &pstSCTTNumber->ulPort) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "prefix_number", dos_strlen("prefix_number")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstSCTTNumber->szPrefix, aszValues[lIndex], sizeof(pstSCTTNumber->szPrefix));
            pstSCTTNumber->szPrefix[sizeof(pstSCTTNumber->szPrefix) - 1] = '\0';
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstSCTTNumber);
        pstSCTTNumber = NULL;
        return DOS_FALSE;
    }


    pthread_mutex_lock(&g_mutexHashTTNumber);
    ulHashIndex = sc_int_hash_func(pstSCTTNumber->ulID, SC_EIX_DEV_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashTTNumber, ulHashIndex, &pstSCTTNumber->ulID, sc_eix_dev_find);
    if (DOS_ADDR_VALID(pstHashNode))
    {
        if (DOS_ADDR_VALID(pstHashNode->pHandle))
        {
            pstSCTTNumberOld = pstHashNode->pHandle;
            dos_snprintf(pstSCTTNumberOld->szAddr, sizeof(pstSCTTNumberOld->szAddr), pstSCTTNumber->szAddr);
            dos_snprintf(pstSCTTNumberOld->szPrefix, sizeof(pstSCTTNumberOld->szPrefix), pstSCTTNumber->szPrefix);
            pstSCTTNumberOld->ulPort = pstSCTTNumber->ulPort;
            dos_dmem_free(pstSCTTNumber);
            pstSCTTNumber = NULL;
        }
        else
        {
            pstHashNode->pHandle = pstSCTTNumber;
        }
    }
    else
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);

            pthread_mutex_unlock(&g_mutexHashTTNumber);
            dos_dmem_free(pstSCTTNumber);
            pstSCTTNumber = NULL;
            return DOS_FALSE;
        }

        pstHashNode->pHandle = pstSCTTNumber;

        hash_add_node(g_pstHashTTNumber, pstHashNode, ulHashIndex, NULL);
    }
    pthread_mutex_unlock(&g_mutexHashTTNumber);

    return DOS_SUCC;
}


U32 sc_eix_dev_load(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, ip, port, prefix_number FROM tbl_eix;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, ip, port, prefix_number FROM tbl_eix WHERE id=%u;", ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_eix_dev_load_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "%s", "Load TT number FAIL.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_eix_dev_delete(U32 ulIndex)
{
    HASH_NODE_S    *pstHashNode;
    U32            ulHashIndex;

    pthread_mutex_lock(&g_mutexHashTTNumber);
    ulHashIndex = sc_int_hash_func(ulIndex, SC_EIX_DEV_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashTTNumber, ulHashIndex, &ulIndex, sc_eix_dev_find);
    if (DOS_ADDR_VALID(pstHashNode))
    {
        hash_delete_node(g_pstHashTTNumber, pstHashNode, ulHashIndex);

        dos_dmem_free(pstHashNode->pHandle);
        pstHashNode->pHandle = NULL;
        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;
        pthread_mutex_unlock(&g_mutexHashTTNumber);
        return DOS_SUCC;
    }
    else
    {
        pthread_mutex_unlock(&g_mutexHashTTNumber);

        return DOS_FAIL;
    }
}

U32 sc_eix_dev_get_by_tt(S8 *pszTTNumber, S8 *pszEIX, U32 ulLength)
{
    U32            ulHashIndex   = 0;
    HASH_NODE_S    *pstHashNode  = NULL;
    SC_TT_NODE_ST  *pstTTNumber  = NULL;
    SC_TT_NODE_ST  *pstTTNumberLast  = NULL;

    if (DOS_ADDR_INVALID(pszTTNumber)
        || DOS_ADDR_INVALID(pszEIX)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashTTNumber);
    HASH_Scan_Table(g_pstHashTTNumber, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashTTNumber, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode ) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstTTNumber = pstHashNode->pHandle;
            /* 匹配前缀 */
            if (0 == dos_strncmp(pszTTNumber
                , pstTTNumber->szPrefix
                , dos_strlen(pstTTNumber->szPrefix)))
            {
                /* 要做一个最优匹配(匹配长度越大越优) */
                if (pstTTNumberLast)
                {
                    if (dos_strlen(pstTTNumberLast->szPrefix) < dos_strlen(pstTTNumber->szPrefix))
                    {
                        pstTTNumberLast = pstTTNumber;
                    }
                }
                else
                {
                    pstTTNumberLast = pstTTNumber;
                }
            }
        }
    }

    if (pstTTNumberLast)
    {
        dos_snprintf(pszEIX, ulLength, "%s:%u", pstTTNumberLast->szAddr, pstTTNumberLast->ulPort);
    }
    else
    {
        pthread_mutex_unlock(&g_mutexHashTTNumber);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Cannot find the EIA for the TT number %s", pszTTNumber);
        return DOS_FAIL;
    }

    pthread_mutex_unlock(&g_mutexHashTTNumber);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Found the EIA(%s) for the TT number(%s).", pszEIX, pszTTNumber);
    return DOS_SUCC;
}

U32 sc_eix_dev_update_proc(U32 ulAction, U32 ulEixID)
{
    if (ulAction >= SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (ulAction)
    {
        case SC_API_CMD_ACTION_CALLER_ADD:
        case SC_API_CMD_ACTION_CALLER_UPDATE:
            sc_eix_dev_load(ulEixID);
            break;
        case SC_API_CMD_ACTION_CALLER_DELETE:
            sc_eix_dev_delete(ulEixID);
            break;
        default:
            break;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Update eix SUCC.(ulAction:%u, ulID:%d)", ulAction, ulEixID);

    return DOS_SUCC;
}

U32 sc_number_lmt_update_proc(U32 ulAction, U32 ulNumlmtID)
{
    if (ulAction >= SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (ulAction)
    {
        case SC_API_CMD_ACTION_CALLER_ADD:
        case SC_API_CMD_ACTION_CALLER_UPDATE:
            sc_number_lmt_load(ulNumlmtID);
            break;

        case SC_API_CMD_ACTION_CALLER_DELETE:
            sc_number_lmt_delete(ulNumlmtID);
            break;

        default:
            break;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Update num limit SUCC.(ulAction:%u, ulID:%d)", ulAction, ulNumlmtID);

    return DOS_SUCC;
}

S32 sc_serv_ctrl_hash_find_cb(VOID *pKey, DLL_NODE_S *pstDLLNode)
{
    SC_SRV_CTRL_ST       *pstSrvCtrl;
    SC_SRV_CTRL_FIND_ST  *pstSrvCtrlFind;

    if (DOS_ADDR_INVALID(pKey))
    {
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstDLLNode) || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        return DOS_FAIL;
    }

    pstSrvCtrl     = (SC_SRV_CTRL_ST *)pstDLLNode->pHandle;
    pstSrvCtrlFind = (SC_SRV_CTRL_FIND_ST*)pKey;

    if (pstSrvCtrl->ulID != pstSrvCtrlFind->ulID
        || pstSrvCtrl->ulCustomerID != pstSrvCtrlFind->ulCustomerID)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

S32 sc_serv_ctrl_hash_find(VOID *pKey, DLL_NODE_S *pstDLLNode)
{
    SC_SRV_CTRL_ST       *pstSrvCtrl;
    SC_SRV_CTRL_FIND_ST  *pstSrvCtrlFind;

    if (DOS_ADDR_INVALID(pKey))
    {
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstDLLNode) || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        return DOS_FAIL;
    }

    pstSrvCtrl     = (SC_SRV_CTRL_ST *)pstDLLNode->pHandle;
    pstSrvCtrlFind = (SC_SRV_CTRL_FIND_ST*)pKey;

    if (pstSrvCtrl->ulID != pstSrvCtrlFind->ulID
        || pstSrvCtrl->ulCustomerID != pstSrvCtrlFind->ulCustomerID)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

S32 sc_serv_ctrl_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_SRV_CTRL_ST        *pstSrvCtrl;
    HASH_NODE_S           *pstHashNode;
    S8                    *pszTmpName;
    S8                    *pszTmpVal;
    U32                   ulLoop;
    U32                   ulProcessCnt;
    U32                   ulHashIndex;
    BOOL                  blProcessOK;
    SC_SRV_CTRL_FIND_ST   stFindParam;

    if (lCount < 9)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSrvCtrl = dos_dmem_alloc(sizeof(SC_SRV_CTRL_ST));
    if (DOS_ADDR_INVALID(pstSrvCtrl))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulProcessCnt = 0;
    blProcessOK=DOS_TRUE;
    for (ulLoop=0; ulLoop<9; ulLoop++)
    {
        pszTmpName = aszNames[ulLoop];
        pszTmpVal  = aszValues[ulLoop];

        if (DOS_ADDR_INVALID(pszTmpName))
        {
            continue;
        }

        if (dos_strnicmp(pszTmpName, "id", dos_strlen("id")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "customer", dos_strlen("customer")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulCustomerID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "serv_type", dos_strlen("serv_type")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulServType) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "effective", dos_strlen("effective")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulEffectTimestamp) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "expire_time", dos_strlen("expire_time")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulExpireTimestamp) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "attr1_value", dos_strlen("attr1_value")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulAttrValue1) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "attr2_value", dos_strlen("attr2_value")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulAttrValue2) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "attr1", dos_strlen("attr1")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulAttr1) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
        else if (dos_strnicmp(pszTmpName, "attr2", dos_strlen("attr2")) == 0)
        {
            if (DOS_ADDR_INVALID(pszTmpVal) || dos_atoul(pszTmpVal, &pstSrvCtrl->ulAttr2) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            ulProcessCnt++;
        }
    }

    if (!blProcessOK || 9 != ulProcessCnt)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_RES), "Load service control rule fail.(%u, %u)", ulProcessCnt, blProcessOK);

        dos_dmem_free(pstSrvCtrl);
        pstSrvCtrl = NULL;

        return DOS_FAIL;
    }

    /*
      * 定义属性类型如果为0，标示属性是无效的，这里将属性值设置为U32_BUTT, 方便程序处理
      * 目前在某些情况下属性值为0，也是一种合法值，因此将U32_BUTT视为无效值
      */
    if (0 == pstSrvCtrl->ulAttr1)
    {
        pstSrvCtrl->ulAttrValue1 = U32_BUTT;
    }

    if (0 == pstSrvCtrl->ulAttr2)
    {
        pstSrvCtrl->ulAttrValue2 = U32_BUTT;
    }

    ulHashIndex = sc_int_hash_func(pstSrvCtrl->ulCustomerID, SC_SERV_CTRL_HASH_SIZE);
    stFindParam.ulID = pstSrvCtrl->ulID;
    stFindParam.ulCustomerID = pstSrvCtrl->ulCustomerID;
    pthread_mutex_lock(&g_mutexHashServCtrl);
    pstHashNode = hash_find_node(g_pstHashServCtrl, ulHashIndex, &stFindParam, sc_serv_ctrl_hash_find_cb);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = dos_dmem_alloc(sizeof(SC_SRV_CTRL_ST));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            pthread_mutex_unlock(&g_mutexHashServCtrl);

            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_RES), "Alloc memory for service ctrl block fail. ID: %u, Customer: %u", stFindParam.ulID, stFindParam.ulCustomerID);

            dos_dmem_free(pstSrvCtrl);
            pstSrvCtrl = NULL;
            return DOS_FAIL;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstSrvCtrl;

        hash_add_node(g_pstHashServCtrl, pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        if (DOS_ADDR_INVALID(pstHashNode->pHandle))
        {
            DOS_ASSERT(0);

            pstHashNode->pHandle = pstSrvCtrl;
        }
        else
        {
            dos_memcpy(pstHashNode->pHandle, pstSrvCtrl, sizeof(SC_SRV_CTRL_ST));
        }
    }

    pthread_mutex_unlock(&g_mutexHashServCtrl);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Add service ctrl block. ID: %u, Customer: %u", stFindParam.ulID, stFindParam.ulCustomerID);
    return DOS_SUCC;
}


U32 sc_serv_ctrl_load(U32 ulIndex)
{
    S8 szSQL[1024];

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT "
                      "tbl_serv_ctrl.id AS id, "
                      "tbl_serv_ctrl.customer AS customer, "
                      "tbl_serv_ctrl.serv_type AS serv_type, "
                      "tbl_serv_ctrl.effective_time AS effective,"
                      "tbl_serv_ctrl.expire_time AS expire_time, "
                      "tbl_serv_ctrl.attr1 AS attr1, "
                      "tbl_serv_ctrl.attr2 AS attr2, "
                      "tbl_serv_ctrl.attr1_value AS attr1_value, "
                      "tbl_serv_ctrl.attr2_value AS attr2_value "
                      "FROM tbl_serv_ctrl");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT "
                      "tbl_serv_ctrl.id AS id, "
                      "tbl_serv_ctrl.customer AS customer, "
                      "tbl_serv_ctrl.serv_type AS serv_type, "
                      "tbl_serv_ctrl.effective_time AS effective,"
                      "tbl_serv_ctrl.expire_time AS expire_time, "
                      "tbl_serv_ctrl.attr1 AS attr1, "
                      "tbl_serv_ctrl.attr2 AS attr2, "
                      "tbl_serv_ctrl.attr1_value AS attr1_value, "
                      "tbl_serv_ctrl.attr2_value AS attr2_value "
                      "FROM tbl_serv_ctrl WHERE id=%u", ulIndex);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "szSQL : %s", szSQL);

    if (db_query(g_pstSCDBHandle, szSQL, sc_serv_ctrl_load_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "%s", "Load service control rule fail.");
        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Load service control rule succ.");

    return DOS_SUCC;
}

U32 sc_serv_ctrl_delete(U32 ulIndex)
{
    HASH_NODE_S           *pstHashNode;
    U32                   ulHashIndex;
    SC_SRV_CTRL_ST        *pstServCtrl;

    pthread_mutex_lock(&g_mutexHashServCtrl);
    HASH_Scan_Table(g_pstHashServCtrl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashServCtrl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstServCtrl = pstHashNode->pHandle;
            if (pstServCtrl->ulID == ulIndex)
            {
                hash_delete_node(g_pstHashServCtrl, pstHashNode, ulHashIndex);

                pthread_mutex_unlock(&g_mutexHashServCtrl);

                return DOS_SUCC;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashServCtrl);

    return DOS_SUCC;
}

U32 sc_serv_ctrl_update_proc(U32 ulAction, U32 ulID)
{
    U32 ulRet = DOS_SUCC;

    if (ulAction >= SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (ulAction)
    {
        case SC_API_CMD_ACTION_ADD:
        case SC_API_CMD_ACTION_UPDATE:
            ulRet = sc_serv_ctrl_load(ulID);
            break;

       case SC_API_CMD_ACTION_DELETE:
            ulRet = sc_serv_ctrl_delete(ulID);
            break;
        default:
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Update serv ctrl rule finished.(ulAction:%u, ulID:%d, Ret: %u)", ulAction, ulID, ulRet);

    return ulRet;
}



BOOL sc_serv_ctrl_check(U32 ulCustomerID, U32 ulServerType, U32 ulAttr1, U32 ulAttrVal1,U32 ulAttr2, U32 ulAttrVal2)
{
    BOOL             blResult;
    U32              ulHashIndex;
    HASH_NODE_S      *pstHashNode;
    SC_SRV_CTRL_ST   *pstSrvCtrl;
    time_t           stTime;

    ulHashIndex = sc_int_hash_func(ulCustomerID, SC_SERV_CTRL_HASH_SIZE);
    stTime = time(NULL);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "match service control rule. Customer: %u, Service: %u, Attr1: %u, Attr2: %u, Attr1Val: %u, Attr2Val: %u"
                , ulCustomerID, ulServerType, ulAttr1, ulAttrVal1, ulAttr2, ulAttrVal2);

    blResult = DOS_FALSE;
    pthread_mutex_lock(&g_mutexHashServCtrl);
    HASH_Scan_Bucket(g_pstHashServCtrl, ulHashIndex, pstHashNode, HASH_NODE_S *)
    {
        /* 合法性检查 */
        if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
        {
            continue;
        }

        /* 一个BUCKET中可能有很多企业的，需要过滤 */
        pstSrvCtrl = pstHashNode->pHandle;
        if (ulCustomerID != pstSrvCtrl->ulCustomerID)
        {
            continue;
        }

        if (stTime < pstSrvCtrl->ulEffectTimestamp
            || (pstSrvCtrl->ulExpireTimestamp && stTime > pstSrvCtrl->ulExpireTimestamp))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: FAIL, Effect: %u, Expire: %u, Current: %u"
                            , pstSrvCtrl->ulEffectTimestamp, pstSrvCtrl->ulExpireTimestamp, stTime);
            continue;
        }

        /* 业务类型要一致, 如果业务类型不为0，才检查 */
        if (0 != ulServerType
            && ulServerType != pstSrvCtrl->ulServType)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: FAIL, Request service type: %u, Current service type: %u"
                            , ulServerType, pstSrvCtrl->ulServType);
            continue;
        }

        /* 检查属性，不为0才检查 */
        if (0 != ulAttr1
            && ulAttr1 != pstSrvCtrl->ulAttr1)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: FAIL, Request Attr1: %u, Current Attr1: %u"
                            , ulAttr1, pstSrvCtrl->ulAttr1);
            continue;
        }

        /* 检查属性，不为0才检查 */
        if (0 != ulAttr2
            && ulAttr2 != pstSrvCtrl->ulAttr2)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: FAIL, Request Attr2: %u, Current Attr2: %u"
                            , ulAttr2, pstSrvCtrl->ulAttr2);
            continue;
        }

        /* 检查属性值，不为U32_BUTT才检查 */
        if (U32_BUTT != ulAttrVal1
            && ulAttrVal1 != pstSrvCtrl->ulAttrValue1)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: FAIL, Request Attr1 Value: %u, Attr1 Value: %u"
                            , ulAttrVal1, pstSrvCtrl->ulAttrValue1);
            continue;
        }

        /* 检查属性值，不为U32_BUTT才检查 */
        if (U32_BUTT != ulAttrVal2
            && ulAttrVal2 != pstSrvCtrl->ulAttrValue2)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: FAIL, Request Attr2 Value: %u, Attr2 Value: %u"
                            , ulAttrVal2, pstSrvCtrl->ulAttrValue2);
            continue;
        }

        /* 所有向均匹配 */
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: SUCC. ID: %u", pstSrvCtrl->ulID);

        blResult = DOS_TRUE;
        break;
    }

    if (!blResult)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "match service ctrl rule in all. server type: %u", ulServerType);

        /* BUCKET 0 中保存的是针对所有客户的规则 */
        HASH_Scan_Bucket(g_pstHashServCtrl, 0, pstHashNode, HASH_NODE_S *)
        {
            /* 合法性检查 */
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            /* 一个BUCKET中可能有很多企业的，需要过滤 */
            pstSrvCtrl = pstHashNode->pHandle;
            if (0 != pstSrvCtrl->ulCustomerID)
            {
                continue;
            }

            if (stTime < pstSrvCtrl->ulEffectTimestamp
                || (pstSrvCtrl->ulExpireTimestamp && stTime > pstSrvCtrl->ulExpireTimestamp))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: FAIL, Effect: %u, Expire: %u, Current: %u(BUCKET 0)"
                                , pstSrvCtrl->ulEffectTimestamp, pstSrvCtrl->ulExpireTimestamp, stTime);
                continue;
            }

            /* 业务类型要一致, 如果业务类型不为0，才检查 */
            if (0 != ulServerType
                && ulServerType != pstSrvCtrl->ulServType)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: FAIL, Request service type: %u, Current service type: %u(BUCKET 0)"
                                , ulServerType, pstSrvCtrl->ulServType);
                continue;
            }

            /* 检查属性，不为0才检查 */
            if (0 != ulAttr1
                && ulAttr1 != pstSrvCtrl->ulAttr1)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: FAIL, Request Attr1: %u, Current Attr1: %u(BUCKET 0)"
                                , ulAttr1, pstSrvCtrl->ulAttr1);
                continue;
            }

            /* 检查属性，不为0才检查 */
            if (0 != ulAttr2
                && ulAttr2 != pstSrvCtrl->ulAttr2)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: FAIL, Request Attr2: %u, Current Attr2: %u(BUCKET 0)"
                                , ulAttr2, pstSrvCtrl->ulAttr2);
                continue;
            }

            /* 检查属性值，不为U32_BUTT才检查 */
            if (U32_BUTT != ulAttrVal1
                && ulAttrVal1 != pstSrvCtrl->ulAttrValue1)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: FAIL, Request Attr1 Value: %u, Attr1 Value: %u(BUCKET 0)"
                                , ulAttrVal1, pstSrvCtrl->ulAttrValue1);
                continue;
            }

            /* 检查属性值，不为U32_BUTT才检查 */
            if (U32_BUTT != ulAttrVal2
                && ulAttrVal2 != pstSrvCtrl->ulAttrValue2)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: FAIL, Request Attr2 Value: %u, Attr2 Value: %u(BUCKET 0)"
                                , ulAttrVal2, pstSrvCtrl->ulAttrValue2);
                continue;
            }

            /* 所有向均匹配 */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Test service control rule: SUCC. ID: %u(BUCKET 0)", pstSrvCtrl->ulID);

            blResult = DOS_TRUE;
            break;
        }
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Match service control rule %s.", blResult ? "SUCC" : "FAIL");


    pthread_mutex_unlock(&g_mutexHashServCtrl);

    return blResult;
}

S32 sc_caller_setting_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_CALLER_SETTING_ST      *pstSetting = NULL;
    U32  ulSettingID = U32_BUTT;

    if (DOS_ADDR_INVALID(pObj)
            || DOS_ADDR_INVALID(pstHashNode)
            || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    ulSettingID = *(U32 *)pObj;
    pstSetting = (SC_CALLER_SETTING_ST *)pstHashNode->pHandle;

    if (ulSettingID == pstSetting->ulID)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }
}


VOID sc_caller_setting_init(SC_CALLER_SETTING_ST *pstCallerSetting)
{
    if (pstCallerSetting)
    {
        dos_memzero(pstCallerSetting, sizeof(SC_CALLER_SETTING_ST));
        pstCallerSetting->ulID = 0;
        pstCallerSetting->ulSrcID = 0;
        pstCallerSetting->ulSrcType = 0;
        pstCallerSetting->ulDstID = 0;
        pstCallerSetting->ulDstType = 0;
        pstCallerSetting->ulCustomerID = U32_BUTT;
    }
}


S32 sc_caller_setting_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    S32  lIndex = U32_BUTT;
    U32  ulHashIndex = U32_BUTT;
    HASH_NODE_S *pstHashNode = NULL;
    SC_CALLER_SETTING_ST *pstSetting = NULL, *pstSettingTemp = NULL;
    BOOL  blProcessOK = DOS_FALSE;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSetting = (SC_CALLER_SETTING_ST *)dos_dmem_alloc(sizeof(SC_CALLER_SETTING_ST));
    if (DOS_ADDR_INVALID(pstSetting))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_caller_setting_init(pstSetting);

    for (lIndex = 0; lIndex < lCount; ++lIndex)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSetting->ulID) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSetting->ulCustomerID) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "name", dos_strlen("name")))
        {
            if ('\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
            dos_snprintf(pstSetting->szSettingName, sizeof(pstSetting->szSettingName), "%s", aszValues[lIndex]);
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "src_id", dos_strlen("src_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSetting->ulSrcID) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "src_type", dos_strlen("src_type")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSetting->ulSrcType) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "dest_id", dos_strlen("dest_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSetting->ulDstID) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "dest_type", dos_strlen("dest_type")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSetting->ulDstType) < 0)
            {
                blProcessOK = DOS_FALSE;
                DOS_ASSERT(0);
                break;
            }
        }
        else
        {
            blProcessOK = DOS_FALSE;
            DOS_ASSERT(0);
            break;
        }
        blProcessOK = DOS_TRUE;
    }
    if (!blProcessOK)
    {
        dos_dmem_free(pstSetting);
        pstSetting = NULL;
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_int_hash_func(pstSetting->ulID, SC_CALLER_SETTING_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashCallerSetting, ulHashIndex, (VOID *)&pstSetting->ulID, sc_caller_setting_hash_find);
    pthread_mutex_lock(&g_mutexHashCallerSetting);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);
            pthread_mutex_unlock(&g_mutexHashCallerSetting);
            dos_dmem_free(pstSetting);
            pstSetting = NULL;
            return DOS_FAIL;
        }
        DLL_Init_Node(pstHashNode);
        pstHashNode->pHandle = (VOID *)pstSetting;
        hash_add_node(g_pstHashCallerSetting, pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        pstSettingTemp = (SC_CALLER_SETTING_ST *)pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstSettingTemp))
        {
            dos_dmem_free(pstSetting);
            pstSetting = NULL;
            DOS_ASSERT(0);
            pthread_mutex_unlock(&g_mutexHashCallerSetting);
        }
        dos_memcpy(pstSettingTemp, pstSetting, sizeof(SC_CALLER_SETTING_ST));
        dos_dmem_free(pstSetting);
        pstSetting = NULL;
    }
    pthread_mutex_unlock(&g_mutexHashCallerSetting);

    return DOS_SUCC;
}

U32  sc_caller_setting_load(U32 ulIndex)
{
    S8  szQuery[128] = {0};
    S32 lRet;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szQuery, sizeof(szQuery), "SELECT id,customer_id,name,src_id,src_type,dest_id,dest_type FROM tbl_caller_setting;");
    }
    else
    {
        dos_snprintf(szQuery, sizeof(szQuery), "SELECT id,customer_id,name,src_id,src_type,dest_id,dest_type FROM tbl_caller_setting WHERE id=%u;", ulIndex);
    }

    lRet = db_query(g_pstSCDBHandle, szQuery, sc_caller_setting_load_cb, NULL, NULL);
    if (DB_ERR_SUCC != lRet)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Load caller setting FAIL.(ID:%u)", ulIndex);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Load caller setting SUCC.(ID:%u)", ulIndex);

    return DOS_SUCC;
}

U32 sc_caller_setting_delete(U32 ulSettingID)
{
    HASH_NODE_S  *pstHashNode = NULL;
    U32 ulHashIndex = U32_BUTT;

    ulHashIndex = sc_int_hash_func(ulSettingID, SC_CALLER_SETTING_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashCallerSetting, ulHashIndex, (VOID *)&ulSettingID, sc_caller_setting_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    hash_delete_node(g_pstHashCallerSetting, pstHashNode, ulHashIndex);
    dos_dmem_free(pstHashNode->pHandle);
    pstHashNode->pHandle = NULL;
    dos_dmem_free(pstHashNode);
    pstHashNode = NULL;

    return DOS_SUCC;
}

U32 sc_caller_setting_update_proc(U32 ulAction, U32 ulSettingID)
{
    U32  ulRet = DOS_FAIL;

    if (ulAction >= SC_API_CMD_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    switch (ulAction)
    {
        case SC_API_CMD_ACTION_CALLER_SET_ADD:
        case SC_API_CMD_ACTION_CALLER_SET_UPDATE:
            ulRet = sc_caller_setting_load(ulSettingID);
            break;
        case SC_API_CMD_ACTION_CALLER_SET_DELETE:
            ulRet = sc_caller_setting_delete(ulSettingID);
            break;
        default:
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Update Caller Setting %s.(ulAction:%u, ulID:%u)"
                    , (DOS_SUCC == ulRet) ? "succ" : "FAIL", ulAction, ulSettingID);

    return DOS_SUCC;
}

static S32 sc_task_load_callee_callback(VOID *pArg, S32 lArgc, S8 **pszValues, S8 **pszNames)
{
    SC_TASK_CB               *pstTCB = NULL;
    SC_TEL_NUM_QUERY_NODE_ST *pstCallee = NULL;
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

    pstTCB = (SC_TASK_CB *)pArg;
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
    dos_strncpy(pstCallee->szNumber, pszNum, sizeof(pstCallee->szNumber));
    pstCallee->szNumber[sizeof(pstCallee->szNumber) - 1] = '\0';

    pstTCB->ulCalleeCount++;
    pstTCB->ulLastCalleeIndex++;
    dos_list_add_tail(&pstTCB->stCalleeNumQuery, &pstCallee->stLink);

    return DOS_SUCC;
}


U32 sc_task_load_callee(SC_TASK_CB *pstTCB)
{
    S8 szSQL[512] = { 0, };

    if (DOS_ADDR_INVALID(pstTCB))
    {
        return -1;
    }

    dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, number FROM tbl_callee_pool WHERE `status`=0 AND file_id = (SELECT tbl_calltask.callee_id FROM tbl_calltask WHERE id=%u) LIMIT %u, 1000;"
                    , pstTCB->ulTaskID
                    , pstTCB->ulLastCalleeIndex);

    if (DB_ERR_SUCC != db_query(g_pstSCDBHandle, szSQL, sc_task_load_callee_callback, (VOID *)pstTCB, NULL))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_RES), "Load task fail.");
        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_RES), "Load task SUCC.");
    return DOS_SUCC;
}


/*
 * 函数 : sc_task_save_status
 * 功能 : 保存任务状态
 * 参数 :
 *        U32 ulTaskID   : 任务ID
 *        U32 ulStatus   : 新状态
 *        S8 *pszStatus  : 如果呼叫名单是正则号码，(暂停时)需要把正则号码的分析状态记录下来
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 */
U32 sc_task_save_status(U32 ulTaskID, U32 ulStatus, S8 *pszStatus)
{
    /* 初始化坐席队列编号 */
    S8 szSQL[128] = { 0, };

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (ulStatus >= SC_TASK_STATUS_DB_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_snprintf(szSQL, sizeof(szSQL), "UPDATE tbl_calltask set status=%u WHERE id=%u", ulStatus, ulTaskID);

    if (db_query(g_pstSCDBHandle, szSQL, NULL, NULL, NULL) < 0)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Failed update the task(%u) status to %u.", ulTaskID, ulStatus);
        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Update the task(%u) status to %u.", ulTaskID, ulStatus);
    return DOS_SUCC;
}

S32 sc_task_load_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    U32 ulTaskID, ulCustomerID, ulMode, ulPlayCnt, ulAudioID, ulGroupID, ulStatus;
    U32 ulMoifyTime, ulCreateTime, ulCalleeCnt, ulCalledCnt, ulCallerGroupID, ulCallRate;
    U32 ulStartTime1, ulEndTime1, ulStartTime2, ulEndTime2;
    U32 ulStartTime3, ulEndTime3, ulStartTime4, ulEndTime4;
    BOOL blProcessOK = DOS_FALSE;
    S32 lIndex = U32_BUTT;
    S8  szTaskName[64] = {0};
    SC_TASK_CB *pstTCB = NULL;

    for (blProcessOK = DOS_TRUE, lIndex = 0; lIndex < lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &ulTaskID) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &ulCustomerID) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "task_name", dos_strlen("task_name")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
            dos_snprintf(szTaskName, sizeof(szTaskName), "%s", aszValues[lIndex]);
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "mtime", dos_strlen("mtime")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }

            if (dos_atoul(aszValues[lIndex], &ulMoifyTime) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "mode", dos_strlen("mode")))
        {
            if (dos_atoul(aszValues[lIndex], &ulMode) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "playcnt", dos_strlen("playcnt")))
        {
            if (dos_atoul(aszValues[lIndex], &ulPlayCnt) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "start_time1", dos_strlen("start_time1")))
        {
            if (dos_atoul(aszValues[lIndex], &ulStartTime1) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "end_time1", dos_strlen("end_time1")))
        {
            if (dos_atoul(aszValues[lIndex], &ulEndTime1) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "start_time2", dos_strlen("start_time2")))
        {
            if (dos_atoul(aszValues[lIndex], &ulStartTime2) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "end_time2", dos_strlen("end_time2")))
        {
            if (dos_atoul(aszValues[lIndex], &ulEndTime2) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "start_time3", dos_strlen("start_time3")))
        {
            if (dos_atoul(aszValues[lIndex], &ulStartTime3) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "end_time3", dos_strlen("end_time3")))
        {
            if (dos_atoul(aszValues[lIndex], &ulEndTime3) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "start_time4", dos_strlen("start_time4")))
        {
            if (dos_atoul(aszValues[lIndex], &ulStartTime4) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "end_time4", dos_strlen("end_time4")))
        {
            if (dos_atoul(aszValues[lIndex], &ulEndTime4) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "audio_id", dos_strlen("audio_id")))
        {
            if (dos_atoul(aszValues[lIndex], &ulAudioID) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "group_id", dos_strlen("group_id")))
        {
            if (dos_atoul(aszValues[lIndex], &ulGroupID) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "status", dos_strlen("status")))
        {
            if (dos_atoul(aszValues[lIndex], &ulStatus) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }

            /* 将数据库中的状态，转换成sc中的状态 SC_TASK_STATUS_DB_EN -> SC_TASK_STATUS_EN */
            switch (ulStatus)
            {
                case SC_TASK_STATUS_DB_INIT:
                case SC_TASK_STATUS_DB_START:
                case SC_TASK_STATUS_DB_PAUSED:
                case SC_TASK_STATUS_DB_STOP:
                    break;
                default:
                    blProcessOK = DOS_FALSE;
                    break;
            }

            if (blProcessOK == DOS_FALSE)
            {
                DOS_ASSERT(0);
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "ctime", dos_strlen("ctime")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }

            if (dos_atoul(aszValues[lIndex], &ulCreateTime) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "calleecnt", dos_strlen("calleecnt")))
        {
            if (dos_atoul(aszValues[lIndex], &ulCalleeCnt) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "calledcnt", dos_strlen("calledcnt")))
        {
            if (dos_atoul(aszValues[lIndex], &ulCalledCnt) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "callers", dos_strlen("callers")))
        {
            if (dos_atoul(aszValues[lIndex], &ulCallerGroupID) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "call_rate", dos_strlen("call_rate")))
        {
            if (dos_atoul(aszValues[lIndex], &ulCallRate) < 0)
            {
                DOS_ASSERT(0);
                blProcessOK = DOS_FALSE;
                break;
            }
        }
    }

    if (blProcessOK == DOS_FALSE)
    {
        /* 数据不正确，不用保存 */
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Load task info FAIL.(TaskID:%u) ", ulTaskID);

        return DOS_SUCC;
    }

    /* 如果只是加载一条数据，首先寻找有没有该控制块节点，如果有更新之
       如果没有，则重新寻找一个不可用节点，如果还是没有空闲节点，则返回失败 */
    pstTCB = sc_tcb_find_by_taskid(*(U32 *)pArg);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        pstTCB  = sc_tcb_alloc();
        if (DOS_ADDR_INVALID(pstTCB))
        {
            /* 数据不正确，不用保存 */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Alloc TCB fail.(TaskID:%u) ", ulTaskID);

            return DOS_FAIL;
        }
    }

    pstTCB->ucValid = DOS_TRUE;
    pstTCB->ulTaskID = ulTaskID;
    pstTCB->ulCustomID = ulCustomerID;
    dos_snprintf(pstTCB->szTaskName, sizeof(pstTCB->szTaskName), "%s", szTaskName);
    pstTCB->ulModifyTime = ulMoifyTime;
    pstTCB->ucMode = (U8)ulMode;
    pstTCB->ucAudioPlayCnt = (U8)ulPlayCnt;
    dos_snprintf(pstTCB->szAudioFileLen, sizeof(pstTCB->szAudioFileLen), "%s/%u/%u", SC_TASK_AUDIO_PATH, ulCustomerID, ulAudioID);
    pstTCB->ulAgentQueueID = ulGroupID;
    pstTCB->ucTaskStatus = ulStatus;
    pstTCB->ulAllocTime = ulCreateTime;
    pstTCB->ulCalleeCountTotal = ulCalleeCnt;
    pstTCB->ulCalledCount = ulCalledCnt;
    pstTCB->ulCallerGrpID = ulCallerGroupID;
    pstTCB->ulCallRate = ulCallRate;
    pstTCB->ulLastCalleeIndex = ulCalledCnt;

    pstTCB->astPeriod[0].ucValid = DOS_TRUE;
    pstTCB->astPeriod[0].ucWeekMask = (ulStartTime1 >> 24) & 0xFF;
    pstTCB->astPeriod[0].ucHourBegin = (ulStartTime1 >> 16) & 0xFF;
    pstTCB->astPeriod[0].ucMinuteBegin = (ulStartTime1 >> 8) & 0xFF;
    pstTCB->astPeriod[0].ucSecondBegin = (ulStartTime1) & 0xFF;
    pstTCB->astPeriod[0].ucHourEnd = (ulEndTime1>> 16) & 0xFF;
    pstTCB->astPeriod[0].ucMinuteEnd = (ulEndTime1>> 8) & 0xFF;
    pstTCB->astPeriod[0].ucSecondEnd = (ulEndTime1) & 0xFF;

    if (ulStartTime2 == 0)
    {
        dos_memzero(&pstTCB->astPeriod[1], sizeof(pstTCB->astPeriod[1]));
        pstTCB->astPeriod[1].ucValid = DOS_FALSE;
    }
    else
    {
        pstTCB->astPeriod[1].ucValid = DOS_TRUE;
        pstTCB->astPeriod[1].ucWeekMask = (ulStartTime2 >> 24) & 0xFF;
        pstTCB->astPeriod[1].ucHourBegin = (ulStartTime2 >> 16) & 0xFF;
        pstTCB->astPeriod[1].ucMinuteBegin = (ulStartTime2 >> 8) & 0xFF;
        pstTCB->astPeriod[1].ucSecondBegin = (ulStartTime2) & 0xFF;
        pstTCB->astPeriod[1].ucHourEnd = (ulEndTime2>> 16) & 0xFF;
        pstTCB->astPeriod[1].ucMinuteEnd = (ulEndTime2>> 8) & 0xFF;
        pstTCB->astPeriod[1].ucSecondEnd = (ulEndTime2) & 0xFF;
    }

    if (ulStartTime3 == 0)
    {
        dos_memzero(&pstTCB->astPeriod[2], sizeof(pstTCB->astPeriod[2]));
        pstTCB->astPeriod[2].ucValid = DOS_FALSE;
    }
    else
    {
        pstTCB->astPeriod[2].ucValid = DOS_TRUE;
        pstTCB->astPeriod[2].ucWeekMask = (ulStartTime3 >> 24) & 0xFF;
        pstTCB->astPeriod[2].ucHourBegin = (ulStartTime3 >> 16) & 0xFF;
        pstTCB->astPeriod[2].ucMinuteBegin = (ulStartTime3 >> 8) & 0xFF;
        pstTCB->astPeriod[2].ucSecondBegin = (ulStartTime3) & 0xFF;
        pstTCB->astPeriod[2].ucHourEnd = (ulEndTime3>> 16) & 0xFF;
        pstTCB->astPeriod[2].ucMinuteEnd = (ulEndTime3>> 8) & 0xFF;
        pstTCB->astPeriod[2].ucSecondEnd = (ulEndTime3) & 0xFF;
    }

    if (ulStartTime4 == 0)
    {
        dos_memzero(&pstTCB->astPeriod[3], sizeof(pstTCB->astPeriod[3]));
        pstTCB->astPeriod[3].ucValid = DOS_FALSE;
    }
    else
    {
        pstTCB->astPeriod[3].ucValid = DOS_TRUE;
        pstTCB->astPeriod[3].ucWeekMask = (ulStartTime4 >> 24) & 0xFF;
        pstTCB->astPeriod[3].ucHourBegin = (ulStartTime4 >> 16) & 0xFF;
        pstTCB->astPeriod[3].ucMinuteBegin = (ulStartTime4 >> 8) & 0xFF;
        pstTCB->astPeriod[3].ucSecondBegin = (ulStartTime4) & 0xFF;
        pstTCB->astPeriod[3].ucHourEnd = (ulEndTime4>> 16) & 0xFF;
        pstTCB->astPeriod[3].ucMinuteEnd = (ulEndTime4>> 8) & 0xFF;
        pstTCB->astPeriod[3].ucSecondEnd = (ulEndTime4) & 0xFF;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Load task info SUCC. Index(%d), (TaskID:%u) ", lIndex, ulTaskID);

    return DOS_SUCC;
}

S32 sc_task_load(U32 ulIndex)
{
    S8  szQuery[256] = {0};
    S32 lRet = U32_BUTT;

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szQuery, sizeof(szQuery),
                        "SELECT id,customer_id,task_name,mtime,mode,playcnt," \
                        "audio_id,group_id,status,ctime,calleecnt,calledcnt," \
                        "start_time1, end_time1, start_time2, end_time2," \
                        "start_time3, end_time3, start_time4, end_time4," \
                        "callers,call_rate FROM tbl_calltask;");
    }
    else
    {
        dos_snprintf(szQuery, sizeof(szQuery),
                        "SELECT id,customer_id,task_name,mtime,mode,playcnt," \
                        "audio_id,group_id,status,ctime,calleecnt,calledcnt," \
                        "start_time1, end_time1, start_time2, end_time2," \
                        "start_time3, end_time3, start_time4, end_time4," \
                        "callers,call_rate FROM tbl_calltask WHERE id=%u;", ulIndex);
    }

    /* 加载群呼任务的相关数据 */
    lRet = db_query(g_pstSCDBHandle, szQuery, sc_task_load_cb, &ulIndex, NULL);
    if (DB_ERR_SUCC != lRet)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), " SC Load task %u Data FAIL.", ulIndex);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Load task %u SUCC.", ulIndex);
    return DOS_SUCC;
}

/*
 * 函数: S32 sc_task_load_callback(VOID *pArg, S32 argc, S8 **argv, S8 **columnNames)
 * 功能: 任务控制模块加载任务回调函数
 * 参数:
 * 返回值
 *      成功返回DOS_SUCC,失败返回DOS_FAIL
 **/
static S32 sc_task_load_callback(VOID *pArg, S32 argc, S8 **argv, S8 **columnNames)
{
    SC_TASK_CB  *pstTCB;
    U32 ulIndex;
    U32 ulTaskID = U32_BUTT, ulCustomID = U32_BUTT;

    for (ulIndex = 0; ulIndex < argc; ulIndex++)
    {
        if (dos_strcmp(columnNames[ulIndex], "id") == 0)
        {
            if (dos_atoul(argv[ulIndex], &ulTaskID) < 0)
            {
                return DOS_FAIL;
            }
        }
        else if (dos_strcmp(columnNames[ulIndex], "customer_id") == 0)
        {
            if (dos_atoul(argv[ulIndex], &ulCustomID) < 0)
            {
                return DOS_FAIL;
            }
        }
    }

    pstTCB = sc_tcb_alloc();
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_task_set_owner(pstTCB, ulTaskID, ulCustomID);

    return DOS_SUCC;
}

/*
 * 函数: U32 sc_task_mngt_load_task()
 * 功能: 初始化时任务控制模块加载任务
 * 参数:
 * 返回值
 *      成功返回DOS_SUCC,失败返回DOS_FAIL
 **/
U32 sc_task_mngt_load_task()
{
    S8 szSqlQuery[128]= { 0 };
    U32 ulLength, ulResult;

    /* 不加载结束和暂停的任务 */
    ulLength = dos_snprintf(szSqlQuery
                , sizeof(szSqlQuery)
                , "SELECT id, customer_id from tbl_calltask where tbl_calltask.status = %d ;", SC_TASK_STATUS_DB_START);

    ulResult = db_query(g_pstSCDBHandle
                            , szSqlQuery
                            , sc_task_load_callback
                            , NULL
                            , NULL);

    if (ulResult != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Load call task fail.");
        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_RES), "Load call task fail.");

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */


