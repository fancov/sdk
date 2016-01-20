
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_res.h"
#include "sc_debug.h"

/* 号码组表最多遍历次数 */
#define SC_MAX_WALK_TIMES   20

extern HASH_TABLE_S *g_pstHashCallerSetting;
extern HASH_TABLE_S *g_pstHashCaller;
extern HASH_TABLE_S *g_pstHashCallerGrp;
extern HASH_TABLE_S *g_pstHashDIDNum;
extern DB_HANDLE_ST *g_pstSCDBHandle;
extern HASH_TABLE_S *g_pstAgentList;
extern U32 sc_acd_hash_func4agent(U32 ulSiteID, U32 *pulHashIndex);
extern S32 sc_acd_agent_hash_find(VOID *pSymName, HASH_NODE_S *pNode);
static S32 sc_generate_random(S32 lUp, S32 lDown);
static U32 sc_get_dst_by_src(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, U32* pulDstID, U32* pulDstType);
static U32 sc_get_number_by_callerid(U32 ulCallerID, S8 *pszNumber, U32 ulLen);
static U32 sc_get_number_by_didid(U32 ulDidID, S8* pszNumber, U32 ulLen);
static U32 sc_get_policy_by_grpid(U32 ulGroupID);
static U32 sc_get_numbers_of_did(U32 ulCustomerID);
static U32 sc_select_did_random(U32 ulCustomerID, S8 *pszNumber, U32 ulLen);
static U32 sc_get_numbers_of_caller(U32 ulCustomerID);
static U32 sc_select_caller_random(U32 ulCustomerID, S8 *pszNumber, U32 ulLen);
static U32 sc_get_did_by_agent(U32 ulAgentID, S8 *pszNumber, U32 ulLen);
static U32 sc_get_did_by_agentgrp(U32 ulAgentGrpID, S8 *pszNumber, U32 ulLen);
static U32 sc_get_agentgrp_by_agentid(U32 ulAgentID, U32 *paulGroupID, U32 ulLen);
static U32 sc_get_number_from_dst(U32 ulCustomerID, U32 ulDstID, U32 ulDstType, S8 *pszNumber, U32 ulLen);
static U32 sc_select_number_random(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber, U32 ulLen);
static U32 sc_select_number_in_order(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber, U32 ulLen);

/**
 *  函数: U32  sc_caller_setting_select_number(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, S8 *pszNumber, U32 ulLen)
 *  功能: 根据呼叫源和策略选择主叫号码
 *  参数:  U32 ulCustomerID   客户id，输入参数
 *         U32 ulSrcID        呼叫源id, 输入参数
 *         U32 ulSrcType      呼叫源类型, 输入参数
 *         S8 *pszNumber      主叫号码缓存，输出参数
 *         U32 ulLen          主叫号码缓存长度，输入参数
 *  返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32  sc_caller_setting_select_number(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, S8 *pszNumber, U32 ulLen)
{
    U32 ulDstID = 0, ulDstType = 0, ulRet = 0;
    U32 aulAgentGrpID[MAX_GROUP_PER_SITE] = {0}, ulLoop = 0;
    U32 ulAgentGrpID = 0, ulHashIndex = U32_BUTT;
    SC_AGENT_INFO_ST stAgent;
    SC_CALLER_SETTING_ST *pstSetting = NULL;
    HASH_NODE_S  *pstHashNode = NULL;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Select Number FAIL.(CustomerID:%u,SrcID:%u,SrcType:%u,pszNumber:%p,len:%u)."
                        , ulCustomerID, ulSrcID, ulSrcType, pszNumber, ulLen);
        return DOS_FAIL;
    }

    /* 根据呼叫源获取呼叫目标 */
    ulRet = sc_get_dst_by_src(ulCustomerID, ulSrcID, ulSrcType, &ulDstID, &ulDstType);
    if (DOS_SUCC == ulRet)
    {
        /* 根据呼叫源查找 */
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get dest by src SUCC.(CustomerID:%u,SrcID:%u,SrcType:%u,DstID:%u,DstType:%u)."
                        , ulCustomerID
                        , pstSetting?pstSetting->ulSrcID:ulSrcID
                        , pstSetting?pstSetting->ulSrcType:ulSrcType
                        , ulDstID
                        , ulDstType);

        ulRet = sc_get_number_from_dst(ulCustomerID, ulDstID, ulDstType, pszNumber, ulLen);
        if (DOS_SUCC == ulRet)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Select Caller SUCC.(CustomerID:%u,SrcID:%u,SrcType:%u,pszNumber:%s)."
                    , ulCustomerID, ulSrcID, ulSrcType, pszNumber);
            return DOS_SUCC;
        }
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get dest by src FAIL.(CustomerID:%u,SrcID:%u,SrcType:%u,DstID:%u,DstType:%u). Then find another Setting..."
                    , ulCustomerID, ulSrcID, ulSrcType, ulDstID, ulDstType);

    /* 如果说呼叫源为坐席，但未匹配到，则找坐席组的设定 */
    if (SC_SRC_CALLER_TYPE_AGENT == ulSrcType)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Find Setting From Agent Group.");
        /* 根据坐席获取坐席组id */
        ulRet = sc_acd_get_agent_by_id(&stAgent, ulSrcID);
        if (DOS_SUCC == ulRet)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get Agent By ID SUCC.(AgentID:%u)", ulSrcID);
            /* 查找有效groupid */
            for (ulLoop = 0; ulLoop < MAX_GROUP_PER_SITE; ulLoop++)
            {
                if (0 == stAgent.aulGroupID[ulLoop] || U32_BUTT == stAgent.aulGroupID[ulLoop])
                {
                    continue;
                }
                ulAgentGrpID = stAgent.aulGroupID[ulLoop];
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Find a Proper Agent Group.(AgentID:%u,AgentGrpID:%u)", ulSrcID, ulAgentGrpID);
                /* 通过坐席组查找设定 */
                HASH_Scan_Table(g_pstHashCallerSetting, ulHashIndex)
                {
                    HASH_Scan_Bucket(g_pstHashCallerSetting, ulHashIndex, pstHashNode, HASH_NODE_S *)
                    {
                        if (DOS_ADDR_INVALID(pstHashNode)
                            || DOS_ADDR_INVALID(pstHashNode->pHandle))
                        {
                            continue;
                        }
                        pstSetting = (SC_CALLER_SETTING_ST *)pstHashNode->pHandle;
                        if (pstSetting->ulSrcID == ulAgentGrpID
                            && pstSetting->ulSrcType == SC_SRC_CALLER_TYPE_AGENTGRP
                            && pstSetting->ulCustomerID == ulCustomerID)
                        {
                            /* 查找设定匹配的目标 */
                            ulDstID = pstSetting->ulDstID;
                            ulDstType = pstSetting->ulDstType;
                            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Find a Proper Setting of Agent Group.(AgentGrpID:%u, DstID:%u, DstType:%u)"
                                            , ulAgentGrpID, ulDstID, ulDstType);

                            ulRet = sc_get_number_from_dst(ulCustomerID, ulDstID, ulDstType, pszNumber, ulLen);
                            if (DOS_SUCC == ulRet)
                            {
                                return DOS_SUCC;
                            }
                        }
                    }
                }
            }
        }
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Find Setting of AgentGrp FAIL.(AgentID:%u), and then find setting from ALL.", ulSrcID);
    }

    if (SC_SRC_CALLER_TYPE_AGENT == ulSrcType
        || SC_SRC_CALLER_TYPE_AGENT == ulSrcType)
    {
        /* 系统设定中查找 */
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Find Setting From ALL.");
        HASH_Scan_Table(g_pstHashCallerSetting, ulHashIndex)
        {
            HASH_Scan_Bucket(g_pstHashCallerSetting, ulHashIndex, pstHashNode, HASH_NODE_S *)
            {
                if (DOS_ADDR_INVALID(pstHashNode)
                    || DOS_ADDR_INVALID(pstHashNode->pHandle))
                {
                    continue;
                }
                pstSetting = (SC_CALLER_SETTING_ST *)pstHashNode->pHandle;
                if (0 == pstSetting->ulSrcID
                    && pstSetting->ulCustomerID == ulCustomerID
                    && pstSetting->ulSrcType == SC_SRC_CALLER_TYPE_ALL)
                {
                    ulDstID = pstSetting->ulDstID;
                    ulDstType = pstSetting->ulDstType;
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Find a Proper setting of ALL.(DstID:%u, DstType:%u)", ulDstID, ulDstType);
                    ulRet = sc_get_number_from_dst(ulCustomerID, ulDstID, ulDstType, pszNumber, ulLen);
                    if (DOS_SUCC == ulRet)
                    {
                        return DOS_SUCC;
                    }
                }
            }
        }
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "No Setting Matched the Call Source.");
    }

    /* 未找到与呼叫源相匹配的呼叫目标，找当前呼叫源绑定的DID号码 */
    switch (ulSrcType)
    {
        /* 如果呼叫源是坐席，那么应该去查找坐席绑定的DID号码 */
        case SC_SRC_CALLER_TYPE_AGENT:
        {
            ulRet = sc_get_did_by_agent(ulSrcID, pszNumber, ulLen);
            if (DOS_SUCC != ulRet)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get Did By Agent FAIL(AgentID:%u). Then Find a Did From AgentGroup.", ulSrcID);
                /* 根据坐席id去获取坐席组id */
                ulRet = sc_get_agentgrp_by_agentid(ulSrcID, aulAgentGrpID, MAX_GROUP_PER_SITE);
                if (DOS_SUCC != ulRet)
                {
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Get AgentGroupID By Agent ID FAIL, And Then find a Random Did.");
                    ulRet = sc_select_did_random(ulCustomerID, pszNumber, ulLen);
                    if (DOS_SUCC != ulRet)
                    {
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random Did FAIL,Select number FAIL.");
                    }
                    else
                    {
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random Did SUCC.");
                        return DOS_SUCC;
                    }

                    ulRet = sc_select_caller_random(ulCustomerID, pszNumber, ulLen);
                    if (DOS_SUCC != ulRet)
                    {
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random caller FAIL,Select number FAIL.");
                        return DOS_FAIL;
                    }
                    else
                    {
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random caller SUCC.");
                        return DOS_SUCC;
                    }
                }
                if (0 != aulAgentGrpID[0] && U32_BUTT != aulAgentGrpID[0])
                {
                    /* 首先查找第1个 */
                    ulRet = sc_get_did_by_agentgrp(aulAgentGrpID[0], pszNumber, ulLen);
                    if (DOS_SUCC != ulRet)
                    {
                        if (0 != aulAgentGrpID[1] && U32_BUTT != aulAgentGrpID[1] && aulAgentGrpID[1] != aulAgentGrpID[0])
                        {
                            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get Did By Agent Group %u FAIL,And Then Did Agent Group %u.", aulAgentGrpID[0], aulAgentGrpID[1]);
                        }
                        else
                        {
                            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get Did By Agent Group %u FAIL,And then Select a Random Did.", aulAgentGrpID[0]);
                            /* 随机选择一个DID号码 */
                            ulRet = sc_select_did_random(ulCustomerID, pszNumber, ulLen);
                            if (DOS_SUCC != ulRet)
                            {
                                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select Did Random FAIL,Select number FAIL.");
                            }
                            else
                            {
                                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random Did SUCC.");
                                return DOS_SUCC;
                            }

                            ulRet = sc_select_caller_random(ulCustomerID, pszNumber, ulLen);
                            if (DOS_SUCC != ulRet)
                            {
                                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random caller FAIL,Select number FAIL.");
                                return DOS_FAIL;
                            }
                            else
                            {
                                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random caller SUCC.");
                                return DOS_SUCC;
                            }
                        }
                    }
                    else
                    {
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get Did By Agent Group %u SUCC.", aulAgentGrpID[0]);
                        return DOS_SUCC;
                    }
                }
                if (0 != aulAgentGrpID[1] && U32_BUTT != aulAgentGrpID[1] && aulAgentGrpID[1] != aulAgentGrpID[0])
                {
                    /* 查找第2个主叫坐席组 */
                    ulRet = sc_get_did_by_agentgrp(aulAgentGrpID[1], pszNumber, ulLen);
                    if (DOS_SUCC != ulRet)
                    {
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get Did By Agent Group %u FAIL,And then Select a Did Random", aulAgentGrpID[1]);
                        /* 预留 */
                        ulRet = sc_select_did_random(ulCustomerID, pszNumber, ulLen);
                        if (DOS_SUCC != ulRet)
                        {
                            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select Did Random FAIL,Select number FAIL.");
                        }
                        else
                        {
                            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random Did SUCC.");
                            return DOS_SUCC;
                        }

                        ulRet = sc_select_caller_random(ulCustomerID, pszNumber, ulLen);
                        if (DOS_SUCC != ulRet)
                        {
                            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random caller FAIL,Select number FAIL.");
                            return DOS_FAIL;
                        }
                        else
                        {
                            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random caller SUCC.");
                            return DOS_SUCC;
                            }
                    }
                    else
                    {
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get Did By Agent Group %u SUCC.", aulAgentGrpID[1]);
                        return DOS_SUCC;
                    }
                }
            }
            else
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get Did By Agent %u SUCC.", ulSrcID);
                return DOS_SUCC;
            }
        }
        /* 如果呼叫源是坐席组，则在坐席组中任选一个坐席绑定的DID号码 */
        case SC_SRC_CALLER_TYPE_AGENTGRP:
        {
            ulRet = sc_get_did_by_agentgrp(ulSrcID, pszNumber, ulLen);
            if (DOS_SUCC != ulRet)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get Did By AgentgGrp %u FAIL,And Then Select Random Did.", ulSrcID);
                /* 随机选择一个主叫号码呼叫 */
                ulRet = sc_select_did_random(ulCustomerID, pszNumber, ulLen);
                if (DOS_SUCC != ulRet)
                {
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select Did Random FAIL,Select number FAIL.");
                }
                else
                {
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random Did SUCC.");
                    return DOS_SUCC;
                }

                ulRet = sc_select_caller_random(ulCustomerID, pszNumber, ulLen);
                if (DOS_SUCC != ulRet)
                {
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random caller FAIL,Select number FAIL.");
                    return DOS_FAIL;
                }
                else
                {
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random caller SUCC.");
                    return DOS_SUCC;
                }
            }
            else
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get Did By AgentGrp %u SUCC.", ulSrcID);
                return DOS_SUCC;
            }
        }
        /* 如果是所有的，在当前客户下的DID号码中任选一个号码 */
        case SC_SRC_CALLER_TYPE_ALL:
        {
            ulRet = sc_select_did_random(ulCustomerID, pszNumber, ulLen);
            if (DOS_SUCC != ulRet)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select Did Random FAIL,Select number FAIL.");
            }
            else
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random Did SUCC.");
                return DOS_SUCC;
            }

            ulRet = sc_select_caller_random(ulCustomerID, pszNumber, ulLen);
            if (DOS_SUCC != ulRet)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random caller FAIL,Select number FAIL.");
                return DOS_FAIL;
            }
            else
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "%s", "Select a Random caller SUCC.");
                return DOS_SUCC;
            }

        }
        default:
            break;
    }
    return DOS_FAIL;
}

static U32 sc_get_number_from_dst(U32 ulCustomerID, U32 ulDstID, U32 ulDstType, S8 *pszNumber, U32 ulLen)
{
    U32 ulRet = DOS_FAIL;
    U32 ulPolicy = 0;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (ulDstType)
    {
        case SC_DST_CALLER_TYPE_CFG:
        {
            /* 直接查找主叫号码缓存 */
            ulRet = sc_get_number_by_callerid(ulDstID, pszNumber, ulLen);
            break;
        }
        case SC_DST_CALLER_TYPE_DID:
        {
            /* 根据did号码id获取主叫号码缓存 */
            ulRet = sc_get_number_by_didid(ulDstID, pszNumber, ulLen);
            break;
        }
        case SC_DST_CALLER_TYPE_CALLERGRP:
        {
            /* 获取呼叫策略 */
            ulPolicy = sc_get_policy_by_grpid(ulDstID);
            if (U32_BUTT == ulPolicy)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "SC Get Policy By Caller GrpID FAIL.(Caller Group ID:%u)", ulDstID);
                return DOS_FAIL;
            }
            switch (ulPolicy)
            {
                case SC_CALLER_POLICY_IN_ORDER:
                {
                    ulRet = sc_select_number_in_order(ulCustomerID, ulDstID, pszNumber, ulLen);
                    break;
                }
                case SC_CALLER_POLICY_RANDOM:
                {
                    ulRet = sc_select_number_random(ulCustomerID, ulDstID, pszNumber, ulLen);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

    if (ulRet != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Select number Random FAIL.(ulCustomerID : %u, CallerGrpID:%u, ulDstType : %u)"
            , ulCustomerID, ulDstID, ulDstType);
    }
    else
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Select number(%s) Random SUCC.(ulCustomerID : %u, CallerGrpID:%u, ulDstType : %u)"
            , pszNumber, ulCustomerID, ulDstID, ulDstType);
    }

    return ulRet;
}

/**
 * 函数: S32 sc_generate_random(S32 lUp, S32 lDown)
 * 功能: 产生一个随机数x. 其中lUp为上界，lDown为下界，x<=lUp && x>=lDown，不过传值的时候可随意传，不限于lUp一定大于lDown
 * 参数: S32 lUp 上界
 *       S32 lDown 下界
 * 返回值: 返回一个介于lUp和lDown之间的随机数，包括边界值lUp和lDown
 **/
static S32 sc_generate_random(S32 lUp, S32 lDown)
{
    S32  lDiff = 0;

    srand(time(NULL));
    lDiff = lUp > lDown?(lUp-lDown):(lDown-lUp);

    return rand() % (lDiff + 1) + (lUp > lDown?lDown:lUp);
}


S32 sc_get_setting_id_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    U32 ulID = U32_BUTT;

    if (DOS_ADDR_INVALID(pArg)
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (dos_atoul(aszValues[0], &ulID) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memcpy(pArg, &ulID, sizeof(U32));
    return DOS_SUCC;
}

/**
 * 函数: U32 sc_get_dst_by_src(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, U32 ulDstID, U32 ulDstType)
 * 功能: 根据呼叫源获得呼叫目的
 * 参数: U32 ulCustomerID  客户id
 *       U32 ulSrcID       呼叫源id， 输入参数
 *       U32 ulSrcType     呼叫源类型, 输入参数
 *       U32* pulDstID     呼叫目的id，输出参数
 *       U32* pulDstType   呼叫目的类型，输出参数
 * 返回值: 返回一个介于lUp和lDown之间的随机数
 **/
static U32 sc_get_dst_by_src(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, U32* pulDstID, U32* pulDstType)
{
    HASH_NODE_S *pstHashNode = NULL;
    U32 ulHashIndex = U32_BUTT;
    U32 ulSettingID = U32_BUTT;
    SC_CALLER_SETTING_ST *pstSetting = NULL;
    S8  szSQL[256] = {0};
    S32  lRet = 0;

    if (DOS_ADDR_INVALID(pulDstID)
        || DOS_ADDR_INVALID(pulDstType))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 寻找caller setting */
    dos_snprintf(szSQL, sizeof(szSQL), "SELECT id FROM tbl_caller_setting WHERE customer_id=%u AND src_id=%u AND src_type=%u;"
                    , ulCustomerID, ulSrcID, ulSrcType);
    lRet = db_query(g_pstSCDBHandle, szSQL, sc_get_setting_id_cb, &ulSettingID, NULL);
    if (DB_ERR_SUCC != lRet)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 寻找setting节点 */
    ulHashIndex = sc_int_hash_func(ulSettingID, SC_CALLER_SETTING_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashCallerSetting, ulHashIndex, &ulSettingID, sc_caller_setting_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        return DOS_FAIL;
    }
    else
    {
        pstSetting = (SC_CALLER_SETTING_ST *)pstHashNode->pHandle;
        *pulDstID = pstSetting->ulDstID;
        *pulDstType = pstSetting->ulDstType;
        return DOS_SUCC;
    }
}

/**
 * 函数: U32 sc_get_number_by_callerid(U32 ulCustomerID, U32 ulCallerID, S8 *pszNumber, U32 ulLen)
 * 功能: 根据caller id获取主叫号码缓存
 * 参数: U32 ulCustomerID    客户id，输入参数
 *       U32 ulCallerID      主叫号码id， 输入参数
 *       S8 *pszNumber       主叫号码缓存，输出参数
 *       U32 ulLen           主叫号码缓存长度
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
static U32 sc_get_number_by_callerid(U32 ulCallerID, S8 *pszNumber, U32 ulLen)
{
    U32 ulHashIndex = U32_BUTT;
    HASH_NODE_S  *pstHashNode = NULL;
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_int_hash_func(ulCallerID, SC_CALLER_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashCaller, ulHashIndex, (VOID *)&ulCallerID, sc_caller_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Get number by caller id FAIL.(CallerID:%u,pszNummber:%p,len:%u)"
                        , ulCallerID, pszNumber, ulLen);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstHashNode->pHandle;

    if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_DID, pstCaller->ulTimes, pstCaller->szNumber))
    {
        return DOS_FAIL;
    }
    dos_snprintf(pszNumber, ulLen, "%s", pstCaller->szNumber);
    pstCaller->ulTimes++;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Get number by caller id SUCC.(CallerID:%u,Number:%s,len:%u)"
                        , ulCallerID, pszNumber, ulLen);
    return DOS_SUCC;
}

/**
 * 函数: U32 sc_get_number_by_didid(U32 ulDidID, S8* pszNumber, U32 ulLen)
 * 功能: 柑橘did号码id获取did号码缓存
 * 参数: U32 ulDidID     号码索引，输入参数
 *       S8* pszNumber   号码缓存，输出参数
 *       U32 ulLen       号码缓存长度，输入参数
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
static U32 sc_get_number_by_didid(U32 ulDidID, S8* pszNumber, U32 ulLen)
{
    SC_DID_NODE_ST *pstDid = NULL;
    U32  ulHashIndex = U32_BUTT;
    HASH_NODE_S *pstHashNode = NULL;
    BOOL bFound = DOS_FALSE;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Get number by did id FAIL.(DidID:%u,pszNumber:%p,len:%u)."
                        , ulDidID, pszNumber, ulLen);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    HASH_Scan_Table(g_pstHashDIDNum, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashDIDNum, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstDid = (SC_DID_NODE_ST *)pstHashNode->pHandle;
            if (DOS_FALSE == pstDid->bValid || pstDid->ulDIDID != ulDidID)
            {
                continue;
            }

            if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_DID, pstDid->ulTimes, pstDid->szDIDNum))
            {
                return DOS_FAIL;
            }

            pstDid->ulTimes++;
            dos_snprintf(pszNumber, ulLen, "%s", pstDid->szDIDNum);
            bFound = DOS_TRUE;
        }
    }
    if (DOS_FALSE == bFound)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Get number by did id FAIL.(DidID:%u,pszNumber:%p,Len:%u)."
                        , ulDidID, pszNumber, ulLen);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static U32 sc_get_policy_by_grpid(U32 ulGroupID)
{
    U32  ulHashIndex = U32_BUTT;
    SC_CALLER_GRP_NODE_ST  *pstGrp = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    BOOL bFound = DOS_FALSE;

    HASH_Scan_Table(g_pstHashCallerGrp, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashCallerGrp, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }
            pstGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;
            if (pstGrp->ulID == ulGroupID)
            {
                bFound = DOS_TRUE;
                return pstGrp->ulPolicy;
            }
        }
    }
    if (DOS_FALSE == bFound)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

/**
 * 功能: 按照循环顺序规则返回一个号码
 * 参数: U32 ulCustomerID  客户id
 *       U32 ulGrpID       号码组id
 *       S8 *pszNumber     号码缓存， 输出参数
 *       U32 ulLen         号码缓存长度，输入参数
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
U32 sc_select_number_in_order(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber, U32 ulLen)
{
    U32  ulHashIndex = U32_BUTT, ulNewNo = U32_BUTT, ulCount = 0, ulTempNo = 0;
    HASH_NODE_S *pstHashNode = NULL;
    DLL_NODE_S *pstNode = NULL;
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    SC_CALLER_CACHE_NODE_ST *pstCache = NULL;
    BOOL bFound = DOS_FALSE;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_int_hash_func(ulGrpID, SC_CALLER_GRP_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulGrpID, sc_caller_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Find Node FAIL,select number in order FAIL.(CustomerID:%u,HashIndex:%u,GrpID:%u)."
                        , ulCustomerID, ulHashIndex, ulGrpID);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;
    /* 如果之前还未用过(初始化状态是0)，或者之前最后一次是链表的最后一个节点，则从头开始找 */
    if (0 == pstCallerGrp->ulLastNo || pstCallerGrp->stCallerList.ulCount == pstCallerGrp->ulLastNo)
    {
        ulNewNo = 1;
    }
    else
    {
        /* 找最新号码的下一个号码 */
        ulNewNo = pstCallerGrp->ulLastNo + 1;
    }

    ulTempNo = ulNewNo;

    pthread_mutex_lock(&pstCallerGrp->mutexCallerList);
    DLL_Scan(&pstCallerGrp->stCallerList, pstNode, DLL_NODE_S *)
    {
        ++ulCount;
        if (ulCount == ulNewNo)
        {
            /* 如果找到了该节点，但为空，则继续往后找一个可用的即可 */
            if (DOS_ADDR_INVALID(pstNode)
                || DOS_ADDR_INVALID(pstHashNode))
            {
                /* 找完了，还未找到可用的，暂时退出，以便后续从头往当前节点继续寻找 */
                if (ulNewNo == pstCallerGrp->stCallerList.ulCount)
                {
                    break;
                }
                else
                {
                    ulNewNo++;
                }
                continue;
            }
            else
            {
                pstCache = (SC_CALLER_CACHE_NODE_ST *)pstNode->pHandle;
                if (SC_NUMBER_TYPE_CFG == pstCache->ulType)
                {
                    if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_CFG, pstCache->stData.pstCaller->ulTimes, pstCache->stData.pstCaller->szNumber))
                    {
                        continue;
                    }
                    dos_snprintf(pszNumber, ulLen, "%s", pstCache->stData.pstCaller->szNumber);
                    pstCache->stData.pstCaller->ulTimes++;
                    pstCallerGrp->ulLastNo = ulNewNo;
                    bFound = DOS_TRUE;
                }
                else if (SC_NUMBER_TYPE_DID == pstCache->ulType)
                {
                    if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_DID, pstCache->stData.pstDid->ulTimes, pstCache->stData.pstDid->szDIDNum))
                    {
                        continue;
                    }
                    dos_snprintf(pszNumber, ulLen, "%s", pstCache->stData.pstDid->szDIDNum);
                    pstCache->stData.pstDid->ulTimes++;
                    pstCallerGrp->ulLastNo = ulNewNo;
                    bFound = DOS_TRUE;
                }
                break;
            }
        }
    }
    pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);

    if (DOS_FALSE == bFound)
    {
        ulCount = 0;
        /* 如果还没有找到，那么继续从头开始给当前节点查找 */
        pthread_mutex_lock(&pstCallerGrp->mutexCallerList);
        DLL_Scan(&pstCallerGrp->stCallerList, pstNode, DLL_NODE_S *)
        {
            ++ulCount;
            if (DOS_ADDR_INVALID(pstNode)
                || DOS_ADDR_INVALID(pstNode->pHandle))
            {
                /* 还没找到，则说明了内存中不存在该节点 */
                if (ulCount == ulTempNo)
                {
                    pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);

                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Select number in order FAIL.(CustomerID:%u, GrpID:%u).", ulCustomerID, ulGrpID);
                    DOS_ASSERT(0);
                    return DOS_FAIL;
                }
            }
            else
            {
                pstCache = (SC_CALLER_CACHE_NODE_ST *)pstNode;
                if (SC_NUMBER_TYPE_CFG == pstCache->ulType)
                {
                    if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_CFG, pstCache->stData.pstCaller->ulTimes, pstCache->stData.pstCaller->szNumber))
                    {
                        continue;
                    }
                    dos_snprintf(pszNumber, ulLen, "%s", pstCache->stData.pstCaller->szNumber);
                    pstCache->stData.pstCaller->ulTimes++;
                    pstCallerGrp->ulLastNo = ulCount;
                }
                else if (SC_NUMBER_TYPE_DID == pstCache->ulType)
                {
                    if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_DID, pstCache->stData.pstDid->ulTimes, pstCache->stData.pstDid->szDIDNum))
                    {
                        continue;
                    }
                    dos_snprintf(pszNumber, ulLen, "%s", pstCache->stData.pstDid->szDIDNum);
                    pstCache->stData.pstDid->ulTimes++;
                    pstCallerGrp->ulLastNo = ulCount;
                }
                else
                {
                    pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);

                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Select number in order FAIL.(CustomerID:%u, GrpID:%u).", ulCustomerID, ulGrpID);
                    DOS_ASSERT(0);
                    return DOS_FAIL;
                }

                pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);
                return DOS_SUCC;
            }
        }

        pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);
    }
    else
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Select number SUCC.(CustomerID:%u, GrpID:%u, pszNumber:%s).", ulCustomerID, ulGrpID, pszNumber);
        return DOS_SUCC;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Select number in order FAIL.(CustomerID:%u, GrpID:%u).", ulCustomerID, ulGrpID);

    return DOS_FAIL;
}

/**
 * 函数: U32 sc_select_number_random(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber, U32 ulLen)
 * 功能: 按照随机规则从号码组中返回一个主叫号码
 * 参数: U32 ulCustomerID  客户id
 *       U32 ulGrpID       号码组id
 *       S8 *pszNumber     号码缓存， 输出参数
 *       U32 ulLen         号码缓存长度
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
U32 sc_select_number_random(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber, U32 ulLen)
{
    S32  lNum = U32_BUTT, lLoop = U32_BUTT;
    BOOL bFound = DOS_FALSE;
    HASH_NODE_S *pstHashNode = NULL;
    DLL_NODE_S  *pstNode = NULL;
    SC_CALLER_CACHE_NODE_ST *pstCache = NULL;
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    U32  ulHashIndex = U32_BUTT, ulCount = 0;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_int_hash_func(ulGrpID, SC_CALLER_GRP_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulGrpID, sc_caller_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Hash find node FAIL,select random number FAIL.(CustomerID:%u,GrpID:%u,HashIndex:%u)."
                        , ulCustomerID, ulGrpID, ulHashIndex);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;

    /* 最多查找MAX_WALK_TIMES次，如果还没找到合适的主叫号码，则认为查找失败，防止无限次造成死循环 */
    for (lLoop = 0; lLoop < SC_MAX_WALK_TIMES; lLoop++)
    {
        lNum = sc_generate_random(1, pstCallerGrp->stCallerList.ulCount);

        pthread_mutex_lock(&pstCallerGrp->mutexCallerList);

        DLL_Scan(&pstCallerGrp->stCallerList, pstNode, DLL_NODE_S *)
        {
            ++ulCount;
            if (ulCount == lNum)
            {
                /* 如果节点为空，则重新生成随机数 */
                if (DOS_ADDR_INVALID(pstNode)
                    || DOS_ADDR_INVALID(pstNode->pHandle))
                {
                    break;
                }
                else
                {
                    pstCache = (SC_CALLER_CACHE_NODE_ST *)pstNode->pHandle;
                    if (SC_NUMBER_TYPE_CFG == pstCache->ulType)
                    {
                        if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_CFG, pstCache->stData.pstCaller->ulTimes, pstCache->stData.pstCaller->szNumber))
                        {
                            continue;
                        }
                        dos_snprintf(pszNumber, ulLen, "%s", pstCache->stData.pstCaller->szNumber);
                        pstCache->stData.pstCaller->ulTimes++;
                    }
                    else if (SC_NUMBER_TYPE_DID == pstCache->ulType)
                    {
                        if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_DID, pstCache->stData.pstCaller->ulTimes, pstCache->stData.pstDid->szDIDNum))
                        {
                            continue;
                        }
                        dos_snprintf(pszNumber, ulLen, "%s", pstCache->stData.pstDid->szDIDNum);
                        pstCache->stData.pstDid->ulTimes++;
                    }
                    else
                    {
                        pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);

                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "select random number FAIL.(CustomerID:%u,GrpID:%u,HashIndex:%u)"
                                        , ulCustomerID, ulGrpID, ulHashIndex);
                        DOS_ASSERT(0);
                        return DOS_FAIL;
                    }
                    pstCallerGrp->ulLastNo = (U32)lNum;
                    bFound = DOS_TRUE;
                }
            }
        }

        pthread_mutex_unlock(&pstCallerGrp->mutexCallerList);
        if (DOS_FALSE == bFound)
        {
            /* 如果还未找到，则进行下一次循环查找 */
            continue;
        }
    }
    if (DOS_FALSE == bFound)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Select random number FAIL.(CustomerID:%u, GrpID:%u).", ulCustomerID, ulGrpID);
        return DOS_FAIL;
    }
    else
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Select random number SUCC.(CustomerID:%u, GrpID:%u, pszNumber:%s).", ulCustomerID, ulGrpID, pszNumber);
        return DOS_SUCC;
    }
}

/**
 * 函数: U32 sc_select_did_random(U32 ulCustomerID, S8 *pszNumber, U32 ulLen)
 * 功能: 按照随机规从号码组中则返回一个DID号码
 * 参数: U32 ulCustomerID  客户id
 *       S8 *pszNumber     号码缓存， 输出参数
 *       U32 ulLen         DID号码缓存
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
static U32 sc_select_did_random(U32 ulCustomerID, S8 *pszNumber, U32 ulLen)
{
    U32  ulCount = 0, ulTick = 0, ulHashIndex = U32_BUTT;
    S32  lRandomNum = U32_BUTT;
    SC_DID_NODE_ST *pstDid = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    BOOL  bIsCheckBegin = DOS_FALSE;

    /* 先获取该客户拥有的did个数 */
    ulCount = sc_get_numbers_of_did(ulCustomerID);
    if (DOS_FAIL == ulCount)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    /* 生成随机数 */
    lRandomNum = sc_generate_random(1, ulCount);
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "randomnum : %d", lRandomNum);

    HASH_Scan_Table(g_pstHashDIDNum, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashDIDNum, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }
            pstDid = (SC_DID_NODE_ST *)pstHashNode->pHandle;
            if (DOS_FALSE != pstDid->bValid && pstDid->ulCustomID == ulCustomerID)
            {
                ulTick++;
                if (ulTick == (U32)lRandomNum)
                {
                    bIsCheckBegin = DOS_TRUE;
                }

                if (bIsCheckBegin)
                {
                    /* 判断主叫超频 */
                    if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_DID, pstDid->ulTimes, pstDid->szDIDNum))
                    {
                        continue;
                    }

                    dos_snprintf(pszNumber, ulLen, "%s", pstDid->szDIDNum);
                    pstDid->ulTimes++;
                    return DOS_SUCC;
                }
            }
        }
    }

    if (bIsCheckBegin)
    {
        ulTick = 0;
        /* 重头开始查找 */
        HASH_Scan_Table(g_pstHashDIDNum, ulHashIndex)
        {
            HASH_Scan_Bucket(g_pstHashDIDNum, ulHashIndex, pstHashNode, HASH_NODE_S *)
            {
                if (DOS_ADDR_INVALID(pstHashNode)
                    || DOS_ADDR_INVALID(pstHashNode->pHandle))
                {
                    continue;
                }
                pstDid = (SC_DID_NODE_ST *)pstHashNode->pHandle;
                if (DOS_FALSE != pstDid->bValid && pstDid->ulCustomID == ulCustomerID)
                {
                    ulTick++;
                    if (ulTick == (U32)lRandomNum)
                    {
                        /* 没有找到 */
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Select random did FAIL.(CustomerID:%u)"
                            , ulCustomerID);
                        return DOS_FAIL;
                    }

                    if (bIsCheckBegin)
                    {
                        /* 判断主叫超频 */
                        if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_DID, pstDid->ulTimes, pstDid->szDIDNum))
                        {
                            continue;
                        }

                        dos_snprintf(pszNumber, ulLen, "%s", pstDid->szDIDNum);
                        pstDid->ulTimes++;
                        return DOS_SUCC;
                    }
                }
            }
        }
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Select random did FAIL.(CustomerID:%u)"
                    , ulCustomerID);
    return DOS_FAIL;
}

static U32 sc_select_caller_random(U32 ulCustomerID, S8 *pszNumber, U32 ulLen)
{
    U32  ulCount = 0, ulTick = 0, ulHashIndex = U32_BUTT;
    S32  lRandomNum = U32_BUTT;
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    BOOL  bIsCheckBegin = DOS_FALSE;

    /* 先获取该客户拥有的caller个数 */
    ulCount = sc_get_numbers_of_caller(ulCustomerID);
    if (DOS_FAIL == ulCount)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    /* 生成随机数 */
    lRandomNum = sc_generate_random(1, ulCount);
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "randomnum : %d", lRandomNum);
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
            if (DOS_FALSE != pstCaller->bValid && pstCaller->ulCustomerID == ulCustomerID)
            {
                ulTick++;
                if (ulTick == (U32)lRandomNum)
                {
                    bIsCheckBegin = DOS_TRUE;
                }

                if (bIsCheckBegin)
                {
                    /* 判断主叫超频 */
                    if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_DID, pstCaller->ulTimes, pstCaller->szNumber))
                    {
                        continue;
                    }

                    dos_snprintf(pszNumber, ulLen, "%s", pstCaller->szNumber);
                    pstCaller->ulTimes++;
                    return DOS_SUCC;
                }
            }
        }
    }

    if (bIsCheckBegin)
    {
        ulTick = 0;
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
                if (DOS_FALSE != pstCaller->bValid && pstCaller->ulCustomerID == ulCustomerID)
                {
                    ulTick++;
                    if (ulTick == (U32)lRandomNum)
                    {
                        /* 没有找到 */
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Select random did FAIL.(CustomerID:%u)"
                            , ulCustomerID);
                        return DOS_FAIL;
                    }

                    if (bIsCheckBegin)
                    {
                        if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_DID, pstCaller->ulTimes, pstCaller->szNumber))
                        {
                            continue;
                        }

                        dos_snprintf(pszNumber, ulLen, "%s", pstCaller->szNumber);
                        pstCaller->ulTimes++;
                        return DOS_SUCC;
                    }
                }
            }
        }
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Select random caller FAIL.(CustomerID:%u)"
                    , ulCustomerID);
    return DOS_FAIL;
}

U32  sc_get_number_by_callergrp(U32 ulGrpID, S8 *pszNumber, U32 ulLen)
{
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    U32  ulHashIndex = 0, ulRet = U32_BUTT;

    ulHashIndex = sc_int_hash_func(ulGrpID,SC_CALLER_GRP_HASH_SIZE);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulGrpID, sc_caller_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;
    if (SC_CALLER_POLICY_IN_ORDER == pstCallerGrp->ulPolicy)
    {
        ulRet = sc_select_number_in_order(pstCallerGrp->ulCustomerID, ulGrpID, pszNumber, ulLen);
        if (DOS_SUCC != ulRet)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Select number in order FAIL.(CallerGrpID:%u)", ulGrpID);
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
    }
    else
    {
        ulRet = sc_select_number_random(pstCallerGrp->ulCustomerID, ulGrpID, pszNumber, ulLen);
        if (DOS_SUCC != ulRet)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Select number random FAIL.(CallerGrpID:%u)", ulGrpID);
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
    }
    return DOS_SUCC;
}

/**
 * 函数: S32 sc_get_numbers_of_did_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * 功能: 数据库回调函数
 * 参数:
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL.
 **/
S32 sc_get_numbers_of_did_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    U32 *pulCount = NULL;

    if (DOS_ADDR_INVALID(pArg)
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pulCount = (U32 *)pArg;
    if (dos_atoul(aszValues[0], pulCount) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

S32 sc_get_numbers_of_caller_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    U32 *pulCount = NULL;

    if (DOS_ADDR_INVALID(pArg)
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pulCount = (U32 *)pArg;
    if (dos_atoul(aszValues[0], pulCount) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

/**
 * 函数: U32 sc_get_numbers_of_did(U32 ulCustomerID)
 * 功能: 返回某一个客户的did号码个数
 * 参数: U32 ulCustomerID  客户id
 * 返回值: 成功返回个数,否则返回DOS_FAIL
 **/
static U32 sc_get_numbers_of_did(U32 ulCustomerID)
{
    S8   szQuery[256] = {0};
    S32  lRet = U32_BUTT;
    U32  ulCount;

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT COUNT(id) FROM tbl_sipassign WHERE customer_id=%u and status=1;", ulCustomerID);
    lRet = db_query(g_pstSCDBHandle, szQuery, sc_get_numbers_of_did_cb, &ulCount, NULL);
    if (DB_ERR_SUCC != lRet)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Get numbers of did FAIL.(ulCustomerID:%u)", ulCustomerID);
        return DOS_FAIL;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get numbers of did SUCC.(ulCustomerID:%u, ulCount:%u)", ulCustomerID, ulCount);
    return ulCount;
}

static U32 sc_get_numbers_of_caller(U32 ulCustomerID)
{
    S8   szQuery[256] = {0};
    S32  lRet = U32_BUTT;
    U32  ulCount;

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT COUNT(id) FROM tbl_caller WHERE customer_id=%u and verify_status=1;", ulCustomerID);
    lRet = db_query(g_pstSCDBHandle, szQuery, sc_get_numbers_of_caller_cb, &ulCount, NULL);
    if (DB_ERR_SUCC != lRet)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Get numbers of caller FAIL.(ulCustomerID:%u)", ulCustomerID);
        return DOS_FAIL;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Get numbers of caller SUCC.(ulCustomerID:%u, ulCount:%u)", ulCustomerID, ulCount);
    return ulCount;
}

/**
 * 函数: U32  sc_get_did_by_agent(U32 ulAgentID, S8 *pszNumber, U32 ulLen)
 * 功能: 根据坐席获取绑定的DID号码
 * 参数: U32 ulAgentID  坐席id
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
static U32 sc_get_did_by_agent(U32 ulAgentID, S8 *pszNumber, U32 ulLen)
{
    U32 ulHashIndex = U32_BUTT;
    HASH_NODE_S  *pstHashNode = NULL;
    SC_DID_NODE_ST *pstDid = NULL;
    SC_AGENT_INFO_ST stAgent;
    BOOL bFound = DOS_FALSE;

    /* 先根据坐席id查找sip_id */
    if (sc_acd_get_agent_by_id(&stAgent, ulAgentID) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get agent SUCC.(ulAgentID : %u, sipID : %u).", ulAgentID, stAgent.ulSIPUserID);
    /* 然后通过sip_id去查找 */
    HASH_Scan_Table(g_pstHashDIDNum, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashDIDNum, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstDid = (SC_DID_NODE_ST *)pstHashNode->pHandle;
            if (DOS_FALSE != pstDid->bValid)
            {
                /* DID绑定 SIP 分机或者坐席 */
                if ((SC_DID_BIND_TYPE_SIP == pstDid->ulBindType && pstDid->ulBindID == stAgent.ulSIPUserID)
                    || (SC_DID_BIND_TYPE_AGENT == pstDid->ulBindType && pstDid->ulBindID == stAgent.ulAgentID))
                {
                    /* 判断主叫超频 */
                    if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_DID, pstDid->ulTimes, pstDid->szDIDNum))
                    {
                        continue;
                    }

                    bFound = DOS_TRUE;
                    pstDid->ulTimes++;
                    dos_snprintf(pszNumber, ulLen, "%s", pstDid->szDIDNum);
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_RES), "Get did by agent SUCC.(ulAgentID:%u, sipID : %u).", ulAgentID, stAgent.ulSIPUserID);
                    break;
                }
            }
        }

        if (DOS_TRUE == bFound)
        {
            break;
        }
    }
    if (DOS_FALSE == bFound)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Get did by agent FAIL.(ulAgentID:%u).", ulAgentID);
        return DOS_FAIL;
    }
    else
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get did by agent SUCC.(ulAgentID:%u).", ulAgentID);
        return DOS_SUCC;
    }
}

/**
 * 函数: U32 sc_get_did_by_agentgrp(U32 ulAgentGrpID, S8 *pszNumber, U32 ulLen)
 * 功能: 根据坐席组获取绑定的DID号码
 * 参数: U32 ulAgentGrpID  坐席组id
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
static U32 sc_get_did_by_agentgrp(U32 ulAgentGrpID, S8 *pszNumber, U32 ulLen)
{
    U32 ulHashIndex = U32_BUTT;
    HASH_NODE_S *pstHashNode = NULL;
    SC_DID_NODE_ST *pstDid = NULL;
    BOOL bFound = DOS_FALSE;

    HASH_Scan_Table(g_pstHashDIDNum, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashDIDNum, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }
            pstDid = (SC_DID_NODE_ST *)pstHashNode->pHandle;
            if (DOS_FALSE != pstDid->bValid && pstDid->ulBindID == ulAgentGrpID && SC_DID_BIND_TYPE_QUEUE == pstDid->ulBindType)
            {
                if (DOS_TRUE != sc_number_lmt_check(SC_NUMBER_TYPE_DID, pstDid->ulTimes, pstDid->szDIDNum))
                {
                    continue;
                }

                bFound = DOS_TRUE;
                pstDid->ulTimes++;
                dos_snprintf(pszNumber, ulLen, "%s", pstDid->szDIDNum);
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
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Get DID by agent group FAIL.(AgentGrpID:%u)", ulAgentGrpID);
        return DOS_FAIL;
    }
    else
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Get DID by agent group SUCC.(AgentGrpID:%u)", ulAgentGrpID);
        return DOS_SUCC;
    }
}

/**
 * 函数: U32 sc_get_agentgrp_by_agentid(U32 ulAgentID, U32 *paulGroupID, U32 ulLen)
 * 功能: 根据坐席组获取绑定的DID号码
 * 参数: U32 ulAgentGrpID  坐席组id
 *       U32 *paulGroupID  坐席组的ID数组首地址，输出参数
 *       U32 ulLen         数组长度
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
static U32 sc_get_agentgrp_by_agentid(U32 ulAgentID, U32 *paulGroupID, U32 ulLen)
{
    HASH_NODE_S *pstHashNode = NULL;
    U32 ulHashIndex = U32_BUTT, ulRet = U32_BUTT;
    SC_AGENT_INFO_ST *pstAgent = NULL;
    S32  lIndex = U32_BUTT;

    ulRet = sc_acd_hash_func4agent(ulAgentID, &ulHashIndex);
    if (DOS_SUCC != ulRet)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "Cannot find agent %u!", ulAgentID);
        return DOS_FAIL;
    }
    pstHashNode = hash_find_node(g_pstAgentList, ulHashIndex , &ulAgentID, sc_acd_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_RES), "No agent %u in AgentList!", ulAgentID);
        return DOS_FAIL;
    }
    pstAgent = (SC_AGENT_INFO_ST *)pstHashNode->pHandle;

    if (ulLen != MAX_GROUP_PER_SITE)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    for (lIndex = 0; lIndex < MAX_GROUP_PER_SITE; lIndex++)
    {
        *(paulGroupID + lIndex) = pstAgent->aulGroupID[lIndex];
    }

    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


