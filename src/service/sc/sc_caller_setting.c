#include <dos.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_ep.h"

/* 号码组表最多遍历次数 */
#define MAX_WALK_TIMES   20

extern HASH_TABLE_S *g_pstHashCallerSetting;
extern HASH_TABLE_S *g_pstHashCaller;
extern HASH_TABLE_S *g_pstHashCallerGrp;
extern HASH_TABLE_S *g_pstHashDIDNum;

/**
 *  函数:U32  sc_caller_setting_select_number(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, U32 ulPolicy, S8 *pszNumber)
 *  功能: 根据呼叫源和策略选择主叫号码
 *  参数:  U32 ulCustomerID   客户id，输入参数
 *         U32 ulSrcID        呼叫源id, 输入参数
 *         U32 ulSrcType      呼叫源类型, 输入参数
 *         U32 ulPolicy       呼叫选择策略,输入参数
 *         S8 *pszNumber      主叫号码缓存，输出参数
 *  返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 ***/
U32  sc_caller_setting_select_number(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, U32 ulPolicy, S8 *pszNumber)
{
    U32 ulDstID = 0, ulDstType = 0, ulRet = 0;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 根据呼叫源获取呼叫目标 */
    ulRet = sc_get_dst_by_src(ulCustomerID, ulSrcID, ulSrcType, &ulDstID, &ulDstType);
    if (DOS_SUCC != ulRet)
    {
        /* 未找到与呼叫源相匹配的呼叫目标，找当前呼叫源绑定的DID号码 */
        switch (ulSrcType)
        {
            /* 如果呼叫源是坐席，那么应该去查找坐席绑定的DID号码 */
            case SC_SRC_CALLER_TYPE_AGENT:
                break;
            /* 如果呼叫源是坐席组，则在坐席组中任选一个坐席绑定的DID号码 */
            case SC_SRC_CALLER_TYPE_AGENTGRP:
                break;
            /* 如果是所有的，在当前客户下的DID号码中任选一个号码 */
            case SC_SRC_CALLER_TYPE_ALL:
                {
                    /* 只是打了桩函数，未实现 */
                    ulRet = sc_select_did_random(ulCustomerID, pszNumber);
                    if (DOS_SUCC != ulRet)
                    {
                        DOS_ASSERT(0);
                        return DOS_FAIL;
                    }
                    else
                    {
                        return DOS_SUCC;
                    }
                }
            default:
                break;
        }
        return DOS_SUCC;
    }
    switch (ulDstType)
    {
        case SC_DST_CALLER_TYPE_CFG:
            {
                /* 直接查找主叫蛤蟆缓存 */
                ulRet = sc_get_number_by_callerid(ulCustomerID, ulDstID, pszNumber);
                if (DOS_SUCC != ulRet)
                {
                    DOS_ASSERT(0);
                    return DOS_FAIL;
                }
                else
                {
                    /* 查找当前坐席或者坐席组绑定的DID */
                    return DOS_SUCC;
                }
            }
        case SC_DST_CALLER_TYPE_DID:
            {
                /* 根据did号码id获取主叫号码缓存 */
                ulRet = sc_get_number_by_didid(ulDstID, pszNumber);
                if (DOS_SUCC != ulRet)
                {
                    DOS_ASSERT(0);
                    return DOS_FAIL;
                }
                else
                {
                    return DOS_SUCC;
                }
            }
        case SC_DST_CALLER_TYPE_CALLERGRP:
            {
                switch (ulPolicy)
                {
                    case SC_CALLER_POLICY_IN_ORDER:
                        {
                            ulRet = sc_select_number_in_order(ulCustomerID, ulDstID, pszNumber);
                            if (DOS_SUCC != ulRet)
                            {
                                DOS_ASSERT(0);
                                return DOS_FAIL;
                            }
                            else
                            {
                                return DOS_SUCC;
                            }
                        }
                    case SC_CALLER_POLICY_RANDOM:
                        {
                            ulRet = sc_select_number_random(ulCustomerID, ulDstID, pszNumber);
                            if (DOS_SUCC != ulRet)
                            {
                                DOS_ASSERT(0);
                                return DOS_FAIL;
                            }
                            else
                            {
                                return DOS_SUCC;
                            }
                        }
                    default:
                        break;
                }
                break;
            }
        default:
            break;
    }
    return DOS_SUCC;
}

/**
 * 函数: S32 sc_generate_random(S32 lUp, S32 lDown)
 * 功能: 产生一个随机数x. 其中lUp为上界，lDown为下界，x<=lUp && x>=lDown，不过传值的时候可随意传，不限于lUp一定大于lDown
 * 参数: S32 lUp 上界
 *       S32 lDown 下界
 * 返回值: 返回一个介于lUp和lDown之间的随机数，包括边界值lUp和lDown
 **/
S32 sc_generate_random(S32 lUp, S32 lDown)
{
    S32  lDiff = 0;

    srand(time(NULL));
    lDiff = lUp > lDown?(lUp-lDown):(lDown-lUp);

    return rand() % (lDiff + 1) + (lUp > lDown?lDown:lUp);
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
U32 sc_get_dst_by_src(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, U32* pulDstID, U32* pulDstType)
{
    HASH_NODE_S *pstHashNode = NULL;
    U32 ulHashIndex = U32_BUTT;
    SC_CALLER_SETTING_ST *pstSetting = NULL;

    if (DOS_ADDR_INVALID(pulDstID)
        || DOS_ADDR_INVALID(pulDstType))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_caller_setting_hash_func(ulCustomerID);
    pstHashNode = hash_find_node(g_pstHashCallerSetting, ulHashIndex, &ulCustomerID, sc_ep_caller_setting_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        ||DOS_ADDR_INVALID(pstHashNode->pHandle))
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
 * 函数: U32 sc_get_number_by_callerid(U32 ulCustomerID, U32 ulCallerID, S8 *pszNumber)
 * 功能: 根据caller id获取主叫号码缓存
 * 参数: U32 ulCustomerID    客户id，输入参数
 *       U32 ulCallerID      主叫号码id， 输入参数
 *       S8 *pszNumber       主叫号码缓存，输出参数
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_SUCC
 **/
U32 sc_get_number_by_callerid(U32 ulCustomerID, U32 ulCallerID, S8 *pszNumber)
{
    U32 ulHashIndex = U32_BUTT;
    HASH_NODE_S  *pstHashNode = NULL;
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_caller_hash_func(ulCustomerID);
    pstHashNode = hash_find_node(g_pstHashCaller, ulHashIndex, (VOID *)&ulCustomerID, sc_ep_caller_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstHashNode->pHandle;
    dos_memcpy(pszNumber, pstCaller->szNumber, sizeof(pstCaller->szNumber));

    return DOS_SUCC;
}

/**
 * 函数: U32 sc_get_number_by_didid(U32 ulDidID, S8* pszNumber)
 * 功能: 柑橘did号码id获取did号码缓存
 * 参数: U32 ulDidID     号码索引，输入参数
 *       S8* pszNumber   号码缓存，输出参数
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_SUCC
 **/
U32 sc_get_number_by_didid(U32 ulDidID, S8* pszNumber)
{
    SC_DID_NODE_ST *pstDid = NULL;
    U32  ulHashIndex = U32_BUTT;
    HASH_NODE_S *pstHashNode = NULL;
    BOOL bFound = DOS_FALSE;

    if (DOS_ADDR_INVALID(pszNumber))
    {
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
            if (pstDid->ulDIDID != ulDidID)
            {
                continue;
            }
            dos_memcpy(pszNumber, pstDid->szDIDNum, sizeof(pstDid->szDIDNum));
            bFound = DOS_TRUE;
        }
    }
    if (DOS_FALSE == bFound)
    {
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

/**
 * 函数: U32 sc_select_number_in_order(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber)
 * 功能: 按照循环顺序规则返回一个号码
 * 参数: U32 ulCustomerID  客户id
 *       U32 ulGrpID       号码组id
 *       S8 *pszNumber     号码缓存， 输出参数
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_SUCC
 **/
U32 sc_select_number_in_order(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber)
{
    U32  ulHashIndex = U32_BUTT, ulNewNo = U32_BUTT, ulCount = 0, ulTempNo = 0;
    HASH_NODE_S *pstHashNode = NULL;
    DLL_NODE_S *pstNode = NULL;
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;
    BOOL bFound = DOS_FALSE;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_caller_grp_hash_func(ulCustomerID);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulCustomerID, sc_ep_caller_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
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
                pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstNode->pHandle;
                /* 将数据复制到号码缓存 */
                dos_memcpy(pszNumber, pstCaller->szNumber, sizeof(pstCaller->szNumber));
                bFound = DOS_TRUE;
                /* 同时更新最新呼叫编号 */
                pstCallerGrp->ulLastNo = ulNewNo;
                break;
            }
        }
    }

    if (DOS_FALSE == bFound)
    {
        ulCount = 0;
        /* 如果还没有找到，那么继续从头开始给当前节点查找 */
        DLL_Scan(&pstCallerGrp->stCallerList, pstNode, DLL_NODE_S *)
        {
            ++ulCount;
            if (DOS_ADDR_INVALID(pstNode)
                || DOS_ADDR_INVALID(pstNode->pHandle))
            {
                /* 还没找到，则说明了内存中不存在该节点 */
                if (ulCount == ulTempNo)
                {
                    DOS_ASSERT(0);
                    return DOS_FAIL;
                }
            }
            else
            {
                pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstNode->pHandle;
                dos_memcpy(pszNumber, pstCaller->szNumber, sizeof(pstCaller->szNumber));
                pstCallerGrp->ulLastNo = ulCount;
                return DOS_SUCC;
            }
        }
    }
    else
    {
        return DOS_SUCC;
    }
    return DOS_FAIL;
}

/**
 * 函数: U32 sc_select_number_random(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber)
 * 功能: 按照随机规则返回一个号码
 * 参数: U32 ulCustomerID  客户id
 *       U32 ulGrpID       号码组id
 *       S8 *pszNumber     号码缓存， 输出参数
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_SUCC
 **/
U32 sc_select_number_random(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber)
{
    S32  lNum = U32_BUTT, lLoop = U32_BUTT;
    BOOL bFound = DOS_FALSE;
    HASH_NODE_S *pstHashNode = NULL;
    DLL_NODE_S  *pstNode = NULL;
    SC_CALLER_QUERY_NODE_ST *pstCaller= NULL;
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    U32  ulHashIndex = U32_BUTT, ulCount = 0;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_caller_grp_hash_func(ulCustomerID);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulCustomerID, sc_ep_caller_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;

    /* 最多查找MAX_WALK_TIMES次，如果还没找到合适的主叫号码，则认为查找失败，防止无限次造成死循环 */
    for (lLoop = 0; lLoop < MAX_WALK_TIMES; lLoop++)
    {
        lNum = sc_generate_random(1, pstCallerGrp->stCallerList.ulCount);

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
                    pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstNode->pHandle;
                    dos_memcpy(pszNumber, pstCaller->szNumber, sizeof(pstCaller->szNumber));
                    pstCallerGrp->ulLastNo = (U32)lNum;
                    bFound = DOS_TRUE;
                }
            }
        }
        if (DOS_FALSE == bFound)
        {
            /* 如果还未找到，则进行下一次循环查找 */
            continue;
        }
    }
    if (DOS_FALSE == bFound)
    {
        return DOS_FAIL;
    }
    else
    {
        return DOS_SUCC;
    }
}


/**
 * 函数: U32 sc_select_did_random(U32 ulCustomerID, S8 *pszNumber)
 * 功能: 按照随机规则返回一个DID号码
 * 参数: U32 ulCustomerID  客户id
 *       S8 *pszNumber     号码缓存， 输出参数
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_SUCC
 **/
U32 sc_select_did_random(U32 ulCustomerID, S8 *pszNumber)
{
    return DOS_SUCC;
}


