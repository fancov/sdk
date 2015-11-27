/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_acd_pub.h
 *
 *  创建时间: 2015年1月14日14:42:12
 *  作    者: Larry
 *  描    述: ACD模块公共头文件
 *  修改历史:
 */

#ifndef __SC_ACD_PUB_H__
#define __SC_ACD_PUB_H__

/* 单个坐席最大可以属于组的个数 */
#define MAX_GROUP_PER_SITE      2

/* 1. 没有被删除
   2. 已经登陆了     && (pstSiteDesc)->bLogin 这个状态不判断了
   3. 需要连接，并且处于连接状态
   4. 状态为EDL*/
#define SC_ACD_SITE_IS_USEABLE(pstSiteDesc)                             \
    (DOS_ADDR_VALID(pstSiteDesc)                                        \
    && !(pstSiteDesc)->bWaitingDelete                                   \
    && SC_ACD_IDEL == (pstSiteDesc)->ucStatus)

enum {
    ACD_MSG_TYPE_CALL_NOTIFY   = 0,
    ACD_MSG_TYPE_STATUS        = 1,
    ACD_MSG_TYPE_QUERY         = 2,

    ACD_MSG_TYPE_BUTT
};

enum {
    ACD_MSG_SUBTYPE_LOGIN      = 1,
    ACD_MSG_SUBTYPE_LOGINOUT   = 2,
    ACD_MSG_SUBTYPE_IDLE       = 3,
    ACD_MSG_SUBTYPE_AWAY       = 4,
    ACD_MSG_SUBTYPE_BUSY       = 5,
    ACD_MSG_SUBTYPE_SIGNIN     = 6,
    ACD_MSG_SUBTYPE_SIGNOUT    = 7,
    ACD_MSG_SUBTYPE_CALLIN     = 8,
    ACD_MSG_SUBTYPE_CALLOUT    = 9,
    ACD_MSG_SUBTYPE_PROC       = 10,
    ACD_MSG_SUBTYPE_RINGING    = 11,

    ACD_MSG_TYPE_QUERY_STATUS  = 12,

    ACD_MSG_SUBTYPE_BUTT
};

enum {
    MSG_CALL_STATE_CONNECTING  = 0,
    MSG_CALL_STATE_CONNECTED   = 1,
    MSG_CALL_STATE_HOLDING     = 2,
    MSG_CALL_STATE_TRANSFER    = 3,
    MSG_CALL_STATE_HUNGUP      = 4,
};

/* IP坐席:
 * 1. 初始化为OFFLINE状态
 * 2. 登陆之后就处于AWAY状态/坐席置忙也处于AWAY状态
 * 3. 坐席置闲就处于IDEL状态
 * 4. 接通呼叫就处于BUSY状态
 */
enum {
    SC_ACD_OFFLINE = 0,                 /* 离线状态，不可用 */
    SC_ACD_IDEL,                        /* 坐席已可用 */
    SC_ACD_AWAY,                        /* 坐席暂时离开，不可用 */
    SC_ACD_BUSY,                        /* 坐席忙，不可用 */
    SC_ACD_PROC,                        /* 坐席正在处理通话结果 */

    SC_ACD_BUTT
};

enum {
    SC_ACD_SITE_ACTION_DELETE = 0,       /* 坐席操作动作，删除 */
    SC_ACD_SITE_ACTION_ADD,              /* 坐席操作动作，删除 */
    SC_ACD_SITE_ACTION_UPDATE,           /* 坐席操作动作，删除 */

    SC_ACD_SITE_ACTION_SIGNIN,           /* 长签 */
    SC_ACD_SITE_ACTION_SIGNOUT,          /* 退出长签 */
    SC_ACD_SITE_ACTION_ONLINE,           /* 坐席登陆到WEB页面 */
    SC_ACD_SITE_ACTION_OFFLINE,          /* 坐席从WEB页面退出 */
    SC_ACD_SITE_ACTION_EN_QUEUE,         /* 置闲 */
    SC_ACD_SITE_ACTION_DN_QUEUE,         /* 置忙 */

    SC_ACD_SITE_ACTION_CONNECTED,         /* 坐席长连成功 */
    SC_ACD_SITE_ACTION_DISCONNECT,        /* 坐席长连失败 */
    SC_ACD_SITE_ACTION_CONNECT_FAIL,      /* 异常长连失败了，无法再次尝试在进行尝试了 */

    SC_API_CMD_ACTION_AGENTGREP_ADD,
    SC_API_CMD_ACTION_AGENTGREP_DELETE,
    SC_API_CMD_ACTION_AGENTGREP_UPDATE,

    SC_API_CMD_ACTION_QUERY,

    SC_ACD_SITE_ACTION_BUTT              /* 坐席签出(长连) */
};

enum {
    SC_ACTION_AGENT_BUSY,
    SC_ACTION_AGENT_IDLE,
    SC_ACTION_AGENT_REST,
    SC_ACTION_AGENT_SIGNIN,
    SC_ACTION_AGENT_SIGNOUT,
    SC_ACTION_AGENT_CALL,
    SC_ACTION_AGENT_HANGUP,
    SC_ACTION_AGENT_HOLD,
    SC_ACTION_AGENT_UNHOLD,
    SC_ACTION_AGENT_TRANSFER,
    SC_ACTION_AGENT_ATRANSFER,
    SC_ACTION_AGENT_LOGIN,
    SC_ACTION_AGENT_LOGOUT,
    SC_ACTION_AGENT_FORCE_OFFLINE,

    SC_ACTION_AGENT_QUERY,

    SC_ACTION_AGENT_BUTT
};

enum {
    SC_ACTION_CALL_TYPE_CLICK,
    SC_ACTION_CALL_TYPE_CALL_NUM,
    SC_ACTION_CALL_TYPE_CALL_AGENT,

    SC_ACTION_CALL_TYPE_BUTT
};


typedef enum tagSCACDPolicy{
    SC_ACD_POLICY_IN_ORDER,               /* 顺序选择 */
    SC_ACD_POLICY_MIN_CALL,               /* 最少呼叫 */
    SC_ACD_POLICY_RANDOM,                 /* 随机 */
    SC_ACD_POLICY_RECENT,                 /* 最近呼叫 */
    SC_ACD_POLICY_GROUP,                  /* 全部 */
    SC_ACD_POLICY_MEMORY,                 /* 记忆呼叫 */

    SC_ACD_POLICY_BUTT
}SC_ACD_POLICY_EN;

typedef enum tagAgentBindType{
    AGENT_BIND_SIP        = 0,
    AGENT_BIND_TELE,
    AGENT_BIND_MOBILE,
    AGENT_BIND_TT_NUMBER,

    AGENT_BIND_BUTT
}SC_AGENT_BIND_TYPE_EN;

enum tagOperatingType{
    OPERATING_TYPE_WEB,
    OPERATING_TYPE_PHONE,
    OPERATING_TYPE_CHECK
};

typedef struct tagACDSiteDesc{
    U16        usSCBNo;
    U8         ucStatus;                          /* 坐席状态 refer to SC_SITE_STATUS_EN */
    U8         ucBindType;                        /* 坐席绑定类型 refer to SC_AGENT_BIND_TYPE_EN */
    U32        ulSiteID;                          /* 坐席数据库编号 */

    U32        ulCallCnt;                         /* 呼叫总数 */
    U32        ulCustomerID;                      /* 客户id */
    U32        ulSIPUserID;                       /* SIP账户ID */
    U32        aulGroupID[MAX_GROUP_PER_SITE];    /* 组ID */

    U32        ulLastOnlineTime;
    U32        ulLastSignInTime;

    U32        bValid:1;                          /* 是否可用 */
    U32        bRecord:1;                         /* 是否录音 */
    U32        bTraceON:1;                        /* 是否调试跟踪 */
    U32        bAllowAccompanying:1;              /* 是否允许分机随行 refer to SC_SITE_ACCOM_STATUS_EN */
    U32        bGroupHeader:1;                    /* 是否是组长 */

    U32        bLogin:1;                          /* 是否已经登录 */
    U32        bConnected:1;                      /* 是否已经长连 */
    U32        bNeedConnected:1;                  /* 是否已经长连 */
    U32        bWaitingDelete:1;                  /* 是否已经被删除 */

    U32        ucProcesingTime:8;                 /* 坐席处理呼叫结果时间 */
    U32        ucRes1:14;

    S8         szUserID[SC_TEL_NUMBER_LENGTH];    /* SIP User ID */
    S8         szExtension[SC_TEL_NUMBER_LENGTH]; /* 分机号 */
    S8         szEmpNo[SC_EMP_NUMBER_LENGTH];     /* 工号 */
    S8         szTelePhone[SC_TEL_NUMBER_LENGTH]; /* 固化号码 */
    S8         szMobile[SC_TEL_NUMBER_LENGTH];    /* 移动电话 */
    S8         szTTNumber[SC_TEL_NUMBER_LENGTH];    /* TT号码 */

    S8         szLastCustomerNum[SC_TEL_NUMBER_LENGTH];    /* 最后一个通话的客户的号码 */

    DOS_TMR_ST htmrLogout;

    pthread_mutex_t  mutexLock;

    SC_SITE_STAT_ST stStat;
}SC_ACD_AGENT_INFO_ST;

U32 sc_acd_get_agent_by_grpid(SC_ACD_AGENT_INFO_ST *pstAgentInfo, U32 ulGroupID, S8 *szCallerNum);
U32 sc_acd_agent_update_status(SC_SCB_ST *pstSCB, U32 ulStatus, U32 ulSCBNo, S8 *szCustomerNum);
S32 sc_acd_grp_hash_find(VOID *pSymName, HASH_NODE_S *pNode);
U32 sc_acd_hash_func4grp(U32 ulGrpID, U32 *pulHashIndex);
U32 sc_acd_query_idel_agent(U32 ulAgentGrpID, BOOL *pblResult);
U32 sc_acd_update_agent_status(U32 ulAction, U32 ulAgentID, U32 ulOperatingType);
U32 sc_acd_agent_stat_by_grpid(U32 ulGroupID, U32 *pulTotal, U32 *pulWorking, U32 *pulIdel, U32 *pulBusy);
U32 sc_acd_get_total_agent(U32 ulGroupID);
U32 sc_acd_get_agent_by_id(SC_ACD_AGENT_INFO_ST *pstAgentInfo, U32 ulAgentID);
U32 sc_acd_get_agent_by_userid(SC_ACD_AGENT_INFO_ST *pstAgentInfo, S8 *szUserID);
U32 sc_acd_get_agent_by_emp_num(SC_ACD_AGENT_INFO_ST *pstAgentInfo, U32 ulCustomID, S8 *pszEmpNum);
U32 sc_acd_singin_by_phone(S8 *szUserID, SC_SCB_ST *pstSCB);
U32 sc_acd_update_agent_scbno_by_userid(S8 *szUserID, SC_SCB_ST *pstSCB, SC_ACD_AGENT_INFO_ST *pstAgentInfo, S8 *szCustomerNum);
U32 sc_acd_update_agent_scbno_by_siteid(U32 ulAgentID, SC_SCB_ST *pstSCB, U32 ulType);
U32 sc_acd_agent_audit(U32 ulCycle, VOID *ptr);
U32 sc_ep_query_agent_status(CURL *curl, SC_ACD_AGENT_INFO_ST *pstAgentInfo);
U32 sc_acd_singin_by_phone(S8 *szUserID, SC_SCB_ST *pstSCB);
U32 sc_ep_agent_status_get(SC_ACD_AGENT_INFO_ST *pstAgentInfo);
U32 sc_ep_agent_status_update(SC_ACD_AGENT_INFO_ST *pstAgentInfo, U32 ulStatus);
U32 sc_ep_call_ctrl_hangup_agent(SC_ACD_AGENT_INFO_ST *pstAgentQueueInfo);

#endif

