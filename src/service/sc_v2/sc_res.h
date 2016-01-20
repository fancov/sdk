/**
 * @file : sc_res.h
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 各种资源数据定义
 *
 * @date: 2016年1月14日
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#ifndef __SC_RES_H__
#define __SC_RES_H__

/** 申明外部变量 */
extern HASH_TABLE_S             *g_pstHashSIPUserID;
extern pthread_mutex_t          g_mutexHashSIPUserID;
extern HASH_TABLE_S             *g_pstHashDIDNum;
extern pthread_mutex_t          g_mutexHashDIDNum;
extern DLL_S                    g_stRouteList;
extern pthread_mutex_t          g_mutexRouteList;
extern DLL_S                    g_stCustomerList;
extern pthread_mutex_t          g_mutexCustomerList;
extern HASH_TABLE_S             *g_pstHashGW;
extern pthread_mutex_t          g_mutexHashGW;
extern HASH_TABLE_S             *g_pstHashGWGrp;
extern pthread_mutex_t          g_mutexHashGWGrp;
extern DLL_S                    g_stNumTransformList;
extern pthread_mutex_t          g_mutexNumTransformList;
extern HASH_TABLE_S             *g_pstHashBlackList;
extern pthread_mutex_t          g_mutexHashBlackList;
extern HASH_TABLE_S             *g_pstHashCaller;
extern pthread_mutex_t          g_mutexHashCaller;
extern HASH_TABLE_S             *g_pstHashCallerGrp;
extern pthread_mutex_t          g_mutexHashCallerGrp;
extern HASH_TABLE_S             *g_pstHashTTNumber;
extern pthread_mutex_t          g_mutexHashTTNumber;
extern HASH_TABLE_S             *g_pstHashNumberlmt;
extern pthread_mutex_t          g_mutexHashNumberlmt;
extern HASH_TABLE_S             *g_pstHashServCtrl;
extern pthread_mutex_t          g_mutexHashServCtrl;
extern HASH_TABLE_S             *g_pstHashCallerSetting;
extern pthread_mutex_t          g_mutexHashCallerSetting;


#define SC_INVALID_INDEX       U32_BUTT

#define SC_STRING_HASH_LIMIT     20

#define SC_SIP_ACCOUNT_HASH_SIZE 512
#define SC_DID_HASH_SIZE         512

/* 路由 中继组的最大数量 */
#define SC_ROUT_GW_GRP_MAX_SIZE  5

#define SC_GW_DOMAIN_LEG        32

#define SC_GW_GRP_HASH_SIZE     128

#define SC_GW_HASH_SIZE         128
#define SC_BLACK_LIST_HASH_SIZE 1024

/** 定义主叫号码的hash表大小 */
#define SC_CALLER_HASH_SIZE     128

/* 定义主叫号码组的hash表大小 */
#define SC_CALLER_GRP_HASH_SIZE 128

/* TT号对应关系 */
#define SC_EIX_DEV_HASH_SIZE   128

/* SCB hash表最大数 */
#define SC_MAX_SCB_HASH_NUM            4096

/* 业务控制HASH表 */
#define SC_SERV_CTRL_HASH_SIZE        512

/* 号码超频限制hash表大小 */
#define SC_NUMBER_LMT_HASH_SIZE 128

/* 定义主叫号码设定的hash表大小 */
#define SC_CALLER_SETTING_HASH_SIZE   128



/* sip 分机的状态 */
typedef enum tagSCSipStatusType
{
    SC_SIP_STATUS_TYPE_UNREGISTER,
    SC_SIP_STATUS_TYPE_REGISTER,

    SC_SIP_STATUS_TYPE_BUTT
}SC_SIP_STATUS_TYPE_EN;

/* 中继的状态 */
typedef enum tagSCTrunkStateType
{
    SC_TRUNK_STATE_TYPE_UNREGED     = 0,
    SC_TRUNK_STATE_TYPE_TRYING,
    SC_TRUNK_STATE_TYPE_REGISTER,
    SC_TRUNK_STATE_TYPE_REGED,
    SC_TRUNK_STATE_TYPE_FAILED,
    SC_TRUNK_STATE_TYPE_FAIL_WAIT,
    SC_TRUNK_STATE_TYPE_NOREG,

    SC_TRUNK_STATE_TYPE_BUTT
}SC_TRUNK_STATE_TYPE_EN;


/*  路由目的地类型 */
typedef enum tagSCDestType{
    SC_DEST_TYPE_GATEWAY     = 1,               /* 呼叫目的为网关 */
    SC_DEST_TYPE_GW_GRP      = 2,               /* 呼叫目的为网关组 */
    SC_DEST_TYPE_BUTT
}SC_DEST_TYPE_EN;


/*  DID号码绑定类型 */
typedef enum tagSCDIDBindType{
    SC_DID_BIND_TYPE_SIP     = 1,               /* DID号码被绑定到SIP账户 */
    SC_DID_BIND_TYPE_QUEUE   = 2,               /* DID号码被绑定到坐席队列 */
    SC_DID_BIND_TYPE_AGENT   = 3,               /* DID号码绑定到坐席 */
    SC_DID_BIND_TYPE_BUTT
}SC_DID_BIND_TYPE_EN;

/* 号码变换的作用对象 */
typedef enum tagSCNumTransformObject{
    SC_NUM_TRANSFORM_OBJECT_SYSTEM = 0,        /* 系统 */
    SC_NUM_TRANSFORM_OBJECT_TRUNK,             /* 中继 */
    SC_NUM_TRANSFORM_OBJECT_CUSTOMER,          /* 客户 */

    SC_NUM_TRANSFORM_OBJECT_BUTT

}SC_NUM_TRANSFORM_OBJECT_EN;

typedef enum tagSCNumTransformDirection{
    SC_NUM_TRANSFORM_DIRECTION_IN = 0,        /* 呼入 */
    SC_NUM_TRANSFORM_DIRECTION_OUT,           /* 呼出 */

    SC_NUM_TRANSFORM_DIRECTION_BUTT

}SC_NUM_TRANSFORM_DIRECTION_EN;

typedef enum tagSCNumTransformTiming{
    SC_NUM_TRANSFORM_TIMING_BEFORE = 0,       /* 路由前 */
    SC_NUM_TRANSFORM_TIMING_AFTER,            /* 路由后 */

    SC_NUM_TRANSFORM_TIMING_BUTT

}SC_NUM_TRANSFORM_TIMING_EN;

typedef enum tagSCNumTransformSelect{
    SC_NUM_TRANSFORM_SELECT_CALLER = 0,        /* 主叫 */
    SC_NUM_TRANSFORM_SELECT_CALLEE,            /* 被叫 */

    SC_NUM_TRANSFORM_SELECT_BUTT

}SC_NUM_TRANSFORM_SELECT_EN;

typedef enum tagSCNumTransformPriority{        /* 号码变换的优先级 */
    SC_NUM_TRANSFORM_PRIORITY_0 = 0,
    SC_NUM_TRANSFORM_PRIORITY_1,
    SC_NUM_TRANSFORM_PRIORITY_2,
    SC_NUM_TRANSFORM_PRIORITY_3,
    SC_NUM_TRANSFORM_PRIORITY_4,
    SC_NUM_TRANSFORM_PRIORITY_5,
    SC_NUM_TRANSFORM_PRIORITY_6,
    SC_NUM_TRANSFORM_PRIORITY_7,
    SC_NUM_TRANSFORM_PRIORITY_8,
    SC_NUM_TRANSFORM_PRIORITY_9,

    SC_NUM_TRANSFORM_PRIORITY_BUTT

}SC_NUM_TRANSFORM_PRIORITY_EN;

typedef enum tagSCBlackNumType
{
    SC_NUM_BLACK_FILE       = 0,            /* 黑名文件中号码 */
    SC_NUM_BLACK_REGULAR,                   /* 正则号码 */
    SC_NUM_BLACK_NUM,                       /* 单个号码 */

    SC_NUM_BLACK_BUTT

}SC_NUM_BLACK_TYPE_EN;

/* 主叫号码选择策略 */
typedef enum tagCallerSelectPolicy
{
    SC_CALLER_SELECT_POLICY_IN_ORDER = 0,   /* 顺序选择号码 */
    SC_CALLER_SELECT_POLICY_RANDOM          /* 随机选择 */
}SC_CALLER_SELECT_POLICY_EN;

typedef enum tagNumberLmtType
{
    SC_NUM_LMT_TYPE_DID     = 0,
    SC_NUM_LMT_TYPE_CALLER  = 1,

    SC_NUM_LMT_TYPE_BUTT
}SC_NUMBER_LMT_TYPE_EN;

typedef enum tagNumberLmtHandle
{
    SC_NUM_LMT_HANDLE_REJECT     = 0,

    SC_NUM_LMT_HANDLE_BUTT
}SC_NUMBER_LMT_HANDLE_EN;

typedef enum tagNumberLmtCycle
{
    SC_NUMBER_LMT_CYCLE_DAY   = 0,
    SC_NUMBER_LMT_CYCLE_WEEK  = 1,
    SC_NUMBER_LMT_CYCLE_MONTH = 2,
    SC_NUMBER_LMT_CYCLE_YEAR  = 3,

    SC_NUMBER_LMT_CYCLE_BUTT
}SC_NUMBER_LMT_CYCLE_EN;


typedef struct tagSIPAcctStat
{
    U32   ulRegisterCnt;
    U32   ulRegisterFailCnt;
    U32   ulUnregisterCnt;
}SC_SIP_ACCT_ST;

/* User ID 描述节点 */
typedef struct tagSCUserIDNode{
    U32  ulCustomID;                             /* 用户 ID */
    U32  ulSIPID;                                /* 账户 ID */
    S8   szUserID[SC_NUM_LENGTH];         /* SIP账户 */
    S8   szExtension[SC_NUM_LENGTH];      /* 分机号 */

    SC_SIP_STATUS_TYPE_EN  enStatus;             /* 状态 */

    SC_SIP_ACCT_ST stStat;
}SC_USER_ID_NODE_ST;

/* DIDI号码们描述节点 */
typedef struct tagSCDIDNode{
    U32   ulCustomID;                             /* 用户ID */
    U32   ulDIDID;                                /* DID 号码ID */
    BOOL  bValid;                                 /* 是否可用标识 */
    S8    szDIDNum[SC_NUM_LENGTH];         /* DID 号码 */
    U32   ulBindType;                             /* 绑定类型 refer to SC_DID_BIND_TYPE_EN */
    U32   ulBindID;                               /* 绑定结果 */
    U32   ulTimes;                                /* 号码被命中次数,用做统计数据 */
}SC_DID_NODE_ST;

/* 路由描述节点 */
typedef struct tagSCRouteNode
{
    U32        ulID;
    BOOL       bExist;                            /* 该标记用来检查是否来自于数据库 */
    BOOL       bStatus;                           /* 标记路由是否可用 */

    U8         ucHourBegin;                       /* 开始时间，小时 */
    U8         ucMinuteBegin;                     /* 开始时间，分钟 */
    U8         ucHourEnd;                         /* 结束时间，小时 */
    U8         ucMinuteEnd;                       /* 结束时间，分钟 */

    S8         szCallerPrefix[SC_NUM_LENGTH];     /* 前缀长度 */
    S8         szCalleePrefix[SC_NUM_LENGTH];     /* 前缀长度 */

    U32        ulDestType;                        /* 目的类型 */
    U32        aulDestID[SC_ROUT_GW_GRP_MAX_SIZE];/* 目的ID */

    U16        usCallOutGroup;
    U8         ucPriority;                        /* 优先级 */
    U8         aucReserv[2];

}SC_ROUTE_NODE_ST;

/* 客户描述节点 */
typedef struct tagSCCustomerNode
{
    U32        ulID;
    BOOL       bExist;                            /* 该标记用来检查是否来自于数据库 */

    U16        usCallOutGroup;
    U8         bTraceCall;
    U8         aucReserv[1];

}SC_CUSTOMER_NODE_ST;


typedef struct tagTrunkStat
{
    U32  ulMaxCalls;
    U32  ulMaxSession;
    U32  ulCurrentSessions;
    U32  ulCurrentCalls;
    U32  ulTotalSessions;
    U32  ulTotalCalls;
    U32  ulOutgoingSessions;
    U32  ulIncomingSessions;
    U32  ulFailSessions;

    U32  ulRegisterCnt;
    U32  ulUnregisterCnt;
    U32  ulRegisterFailCnt;
    U32  ulKeepAliveCnt;
    U32  ulKeepAliveFailCnt;
}SC_TRUNK_STAT_ST;

/* 网关描述节点 */
typedef struct tagSCGWNode
{
    U32 ulGWID;                                    /* 网关ID */
    S8  szGWDomain[SC_GW_DOMAIN_LEG];              /* 网关的域，暂时没有用的 */
    BOOL bExist;                                   /* 该标识用来判断数据库是否有该数据 */
    BOOL bStatus;                                  /* 网关状态，启用或者是禁用 */
    BOOL bRegister;                                /* 是否注册 */
    U32 ulRegisterStatus;                          /* 注册状态 */

    SC_TRUNK_STAT_ST stStat;
}SC_GW_NODE_ST;

/* 中继组 */
typedef struct tagGatewayGrpNode
{
    U32        ulGWGrpID;                         /* 网关组ID */
    BOOL       bExist;                            /* 该标记用来检验该数据是否来自与数据库 */
    DLL_S      stGWList;                          /* 网关列表 refer to SC_GW_NODE_ST */
    pthread_mutex_t  mutexGWList;                 /* 路由组的锁 */
}SC_GW_GRP_NODE_ST;

typedef struct tagSCNumTransformNode
{
    U32                             ulID;
    BOOL                            bExist;

    SC_NUM_TRANSFORM_OBJECT_EN      enObject;
    U32                             ulObjectID;
    SC_NUM_TRANSFORM_DIRECTION_EN   enDirection;                        /* 呼入/呼出 */
    SC_NUM_TRANSFORM_TIMING_EN      enTiming;                           /* 路由前/路由后 */
    SC_NUM_TRANSFORM_SELECT_EN      enNumSelect;                        /* 主叫号码/被叫号码 */

    U32                             ulDelLeft;                          /* 左边删除位数 */
    U32                             ulDelRight;                         /* 右边删除位数 */
    SC_NUM_TRANSFORM_PRIORITY_EN    enPriority;                         /* 优先级 */
    BOOL                            bReplaceAll;                        /* 是否完全替代 */

    S8                              szReplaceNum[SC_NUM_LENGTH];    /* 如果以*号开头，则后面跟号码组的id */
    S8                              szCallerPrefix[SC_NUM_LENGTH];  /* 主叫前缀 */
    S8                              szCalleePrefix[SC_NUM_LENGTH];  /* 被叫前缀 */
    S8                              szAddPrefix[SC_NUM_LENGTH];     /* 增加前缀 */
    S8                              szAddSuffix[SC_NUM_LENGTH];     /* 增加后缀 */

    U32                             ulExpiry;                       /* 有效期 */
}SC_NUM_TRANSFORM_NODE_ST;

/* 黑名单HASH表节点 */
typedef struct tagSCBlackListNode{
    U32                     ulID;                                   /* 索引 */
    U32                     ulFileID;                               /* 文件ID */
    U32                     ulCustomerID;                           /* 用户ID */
    S8                      szNum[SC_NUM_LENGTH];            /* 表达式 */
    SC_NUM_BLACK_TYPE_EN    enType;                                 /* 类型，号码或者正则表达式 */
}SC_BLACK_LIST_NODE;

typedef struct tagCallerQueryNode{
    U16        usNo;                              /* 编号 */
    U8         bValid;
    U8         bTraceON;                          /* 是否跟踪 */

    U32        ulIndexInDB;                       /* 数据库中的ID */
    U32        ulCustomerID;                      /* 所属客户id */
    U32        ulTimes;                           /* 号码被呼叫选中的次数，用做统计 */

    S8         szNumber[SC_NUM_LENGTH];    /* 号码缓存 */
}SC_CALLER_QUERY_NODE_ST;

/* 主叫号码组描述节点 */
typedef struct tagCallerGrpNode
{
    U32   ulID;          /* 主叫号码组id */
    U32   ulCustomerID;  /* 客户id */
    U32   ulLastNo;      /* 上一次参与呼叫的主叫号码序列号 */
    U32   ulPolicy;      /* 呼叫策略 */
    BOOL  bDefault;      /* 是否为默认组，DOS_FALSE表示非默认组，DOS_TRUE表示默认组 */
    S8    szGrpName[64]; /* 主叫号码组名称 */
    DLL_S stCallerList;  /* 主叫号码列表 */

    pthread_mutex_t mutexCallerList;
}SC_CALLER_GRP_NODE_ST;

/* TT号信息 */
typedef struct tagSCTTNumNode{
    U32 ulID;
    U32 ulPort;
    S8  szPrefix[SC_NUM_LENGTH];
    S8  szAddr[SC_MAX_IP_LEN];

}SC_TT_NODE_ST;

/* 号码限制hash节点 */
typedef struct tagSCNumberLmtNode
{
    U32 ulID;           /* ID */
    U32 ulGrpID;        /* 组ID */
    U32 ulHandle;       /* 处理手段 */
    U32 ulLimit;        /* 限制数量 */
    U32 ulCycle;        /* 周期 */
    U32 ulType;         /* 主叫号码还是DID号码 */
    U32 ulNumberID;     /* 号码ID */

    /* 号码 */
    S8  szPrefix[SC_NUM_LENGTH];

    U32 ulStatUsed;
}SC_NUMBER_LMT_NODE_ST;

typedef struct tagSCSrvCtrlFind{
    U32          ulID;               /* 规则ID */
    U32          ulCustomerID;       /* CUSTOMER ID */
}SC_SRV_CTRL_FIND_ST;

typedef enum tagSCSrvCtrlAttr
{
    SC_SRV_CTRL_ATTR_INVLID      = 0,

    SC_SRV_CTRL_ATTR_TASK_MODE   = 1,

    SC_SRV_CTRL_ATTR_BUTT
}SC_SRV_CTRL_ATTR_EN;

typedef struct tagSCSrvCtrl
{
    U32          ulID;               /* 规则ID */
    U32          ulCustomerID;       /* CUSTOMER ID */
    U32          ulServType;         /* 业务类型 */
    U32          ulEffectTimestamp;  /* 生效日期 */
    U32          ulExpireTimestamp;  /* 失效日期 */
    U32          ulAttr1;            /* 属性 */
    U32          ulAttr2;            /* 属性 */
    U32          ulAttrValue1;       /* 属性值1 */
    U32          ulAttrValue2;       /* 属性值2 */

    BOOL         bExist;             /* 该标识用来判断数据库是否有该数据 */
}SC_SRV_CTRL_ST;

/* 主叫号码设定描述节点 */
typedef struct tagCallerSetting
{
    U32   ulID;               /* 号码绑定关系id */
    U32   ulCustomerID;       /* 客户id */
    S8    szSettingName[64];  /* 关系名称 */
    U32   ulSrcID;            /* 呼叫源id */
    U32   ulSrcType;          /* 呼叫源类型 */
    U32   ulDstID;            /* 目的ID */
    U32   ulDstType;          /* 目标类型 */
}SC_CALLER_SETTING_ST;

/* 定义加入号码组的号码类型 */
typedef enum tagSCNumberType
{
    SC_NUMBER_TYPE_CFG = 0,         /* 系统手动配置的主叫号码 */
    SC_NUMBER_TYPE_DID,             /* 系统的did号码 */
}SC_NUMBER_TYPE_EN;

/* 定义主叫号码选择策略 */
typedef enum tagCallerPolicy
{
    SC_CALLER_POLICY_IN_ORDER = 0,   /* 循环顺序选择策略 */
    SC_CALLER_POLICY_RANDOM = 1,     /* 随机选择策略 */

    SC_CALLER_POLICY_BUTT
}SC_CALLER_POLICY_EN;

/* 定义呼叫源类型 */
typedef enum tagSrcCallerType
{
    SC_SRC_CALLER_TYPE_AGENT = 1,      /* 坐席 */
    SC_SRC_CALLER_TYPE_AGENTGRP = 2,   /* 坐席组 */
    SC_SRC_CALLER_TYPE_ALL = 10        /* 所有呼叫 */
}SC_SRC_CALLER_TYPE_EN;

/* 定义呼叫目标类型 */
typedef enum tagDstCallerType
{
    SC_DST_CALLER_TYPE_CFG = 0,     /* 系统配置号码 */
    SC_DST_CALLER_TYPE_DID,         /* did号码 */
    SC_DST_CALLER_TYPE_CALLERGRP    /* 号码组 */
}SC_DST_CALLER_TYPE_EN;


typedef struct tagCallerCacheNode
{
    U32   ulType;   /* 号码类型 */
    union stCallerData
    {
        SC_CALLER_QUERY_NODE_ST  *pstCaller;  /* 主叫号码指针 */
        SC_DID_NODE_ST           *pstDid;     /* did号码指针 */
    }stData;
}SC_CALLER_CACHE_NODE_ST;

U32 sc_int_hash_func(U32 ulVal, U32 ulHashSize);

U32 sc_serv_ctrl_load(U32 ulIndex);
U32 sc_number_lmt_load(U32 ulIndex);
U32 sc_caller_setting_load(U32 ulIndex);
U32 sc_caller_relationship_load();
U32 sc_caller_group_load(U32 ulIndex);
U32 sc_caller_load(U32 ulIndex);
U32 sc_eix_dev_load(U32 ulIndex);
U32 sc_black_list_load(U32 ulIndex);
U32 sc_sip_account_load(U32 ulIndex);
U32 sc_did_load(U32 ulIndex);
U32 sc_customer_load(U32 ulIndex);
U32 sc_transform_load(U32 ulIndex);
U32 sc_route_load(U32 ulIndex);
U32 sc_gateway_relationship_load();
U32 sc_gateway_group_load(U32 ulIndex);
U32 sc_gateway_load(U32 ulIndex);
U32 sc_route_search(SC_SRV_CB *pstSCB, S8 *pszCalling, S8 *pszCallee);
U32 sc_route_get_trunks(U32 ulRouteID, U32 *paulTrunkList, U32 ulTrunkListSize);
U32 sc_sip_account_get_by_extension(U32 ulCustomID, S8 *pszExtension, S8 *pszUserID, U32 ulLength);
S32 sc_caller_setting_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
S32 sc_caller_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
S32 sc_caller_group_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
U32 sc_caller_setting_update_proc(U32 ulAction, U32 ulSettingID);
U32 sc_caller_group_update_proc(U32 ulAction, U32 ulCallerGrpID);
U32 sc_gateway_update_proc(U32 ulAction, U32 ulGatewayID);
U32 sc_sip_account_update_proc(U32 ulAction, U32 ulSipID, U32 ulCustomerID);
U32 sc_route_update_proc(U32 ulAction, U32 ulRouteID);
U32 sc_gateway_group_update_proc(U32 ulAction, U32 ulGwGroupID);
U32 sc_did_update_proc(U32 ulAction, U32 ulDidID);
U32 sc_black_list_update_proc(U32 ulAction, U32 ulBlackID);
U32 sc_caller_update_proc(U32 ulAction, U32 ulCallerID);
U32 sc_eix_dev_update_proc(U32 ulAction, U32 ulEixID);
U32 sc_serv_ctrl_update_proc(U32 ulAction, U32 ulID);
U32 sc_transform_update_proc(U32 ulAction, U32 ulNumTransID);
U32 sc_customer_update_proc(U32 ulAction, U32 ulCustomerID);
BOOL sc_number_lmt_check(U32 ulType, U32 ulCurrentTime, S8 *pszNumber);
U32 sc_number_lmt_update_proc(U32 ulAction, U32 ulNumlmtID);


S32 sc_task_load(U32 ulIndex);
U32 sc_task_load_callee(SC_TASK_CB *pstTCB);
U32 sc_task_mngt_load_task();
U32 sc_task_save_status(U32 ulTaskID, U32 ulStatus, S8 *pszStatus);

U32  sc_get_number_by_callergrp(U32 ulGrpID, S8 *pszNumber, U32 ulLen);
BOOL sc_serv_ctrl_check(U32 ulCustomerID, U32 ulServerType, U32 ulAttr1, U32 ulAttrVal1,U32 ulAttr2, U32 ulAttrVal2);
U32 sc_eix_dev_get_by_tt(S8 *pszTTNumber, S8 *pszEIX, U32 ulLength);
U32  sc_caller_setting_select_number(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, S8 *pszNumber, U32 ulLen);

#endif /* __SC_RES_H__ */

#ifdef __cplusplus
}
#endif /* End of __cplusplus */

