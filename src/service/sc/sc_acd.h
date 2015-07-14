/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_acd.h
 *
 *  创建时间: 2014年12月16日10:15:56
 *  作    者: Larry
 *  描    述: ACD模块私有头文件
 *  修改历史:
 */


#ifndef __SC_ACD_H__
#define __SC_ACD_H__

#define SC_ACD_GROUP_NAME_LEN         24

#define SC_ACD_HASH_SIZE              128

/* 1. 没有被删除
   2. 已经登陆了
   3. 需要连接，并且处于连接状态
   4. 状态为EDL*/
#define SC_ACD_SITE_IS_USEABLE(pstSiteDesc)                             \
    (DOS_ADDR_VALID(pstSiteDesc)                                       \
    && !(pstSiteDesc)->bWaitingDelete                                  \
    && (pstSiteDesc)->bLogin                                           \
    && SC_ACD_IDEL == (pstSiteDesc)->ucStatus                          \
    && !((pstSiteDesc)->bNeedConnected && !(pstSiteDesc)->bConnected))

typedef struct tagACDQueueNode{
    U32                     ulID;             /* 当前节点在当前队列里面的编号 */

    SC_ACD_AGENT_INFO_ST   *pstAgentInfo;     /* 坐席信息 */
}SC_ACD_AGENT_QUEUE_NODE_ST;

typedef struct tagACDMemoryNode{
    U32        ulSiteID;                            /* 坐席数据库编号 */

    S8        szCallerNum[SC_TEL_NUMBER_LENGTH];    /* 主叫号码 */
}SC_ACD_MEMORY_RELATION_QUEUE_NODE_ST;

typedef struct tagACDQueryMngtNode
{
    U16       usID;                                /* 当前坐席组编号 */
    U16       usCount;                             /* 当前坐席组坐席数量 */
    U16       usLastUsedAgent;                     /* 上一次接电话的坐席编号 */
    U8        ucACDPolicy;                         /* 组呼叫分配策略 */
    U8        ucWaitingDelete;                     /* 是否等待删除 */

    U32       ulCustomID;                          /* 当前坐席组所属 */
    U32       ulGroupID;                           /* 坐席组在数据库中的编号 */
    S8        szGroupName[SC_ACD_GROUP_NAME_LEN];  /* 坐席组名 */

    DLL_S     stAgentList;                         /* 坐席hash表 */

    HASH_TABLE_S     *pstRelationList;             /* 主叫号码和坐席的对应关系列表 */
    pthread_mutex_t  mutexSiteQueue;

    SC_SITE_GRP_STAT_ST stStat;
}SC_ACD_GRP_HASH_NODE_ST;

typedef struct tagACDFindSiteParam
{
    U32                  ulPolocy;
    U32                  ulLastSieUsed;
    U32                  ulResult;

    SC_ACD_AGENT_INFO_ST *pstResult;
}SC_ACD_FIND_SITE_PARAM_ST;


U32 sc_acd_agent_grp_add_call(U32 ulGrpID);
U32 sc_acd_agent_grp_del_call(U32 ulGrpID);
U32 sc_acd_agent_grp_stat(U32 ulGrpID, U32 ulWaitingTime);

#endif

