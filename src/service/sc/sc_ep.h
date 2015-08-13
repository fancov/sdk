/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_ep_pub.h
 *
 *  创建时间: 2015年1月9日11:09:15
 *  作    者: Larry
 *  描    述: 处理FS核心模块发送过来的消息模块的公共头文件
 *  修改历史:
 */

#ifndef __SC_EP_PUB_H__
#define __SC_EP_PUB_H__

/* 定义路由信息中号码前、后缀的长度 */
#define SC_NUM_PREFIX_LEN       16

/* 定义网关域的长度 */
#define SC_GW_DOMAIN_LEG        32

/* 定义SIP User ID的hash表大小 */
#define SC_IP_USERID_HASH_SIZE  1024

/* 定义DID号码的hash表大小 */
#define SC_IP_DID_HASH_SIZE     128

/* 定义黑名单号码的hash表大小 */
#define SC_BLACK_LIST_HASH_SIZE 1024

/* TT号对应关系 */
#define SC_TT_NUMBER_HASH_SIZE  128

/* 定义网关组的hash表大小 */
#define SC_GW_GRP_HASH_SIZE     128

/* 定义网关的hash表大小 */
#define SC_GW_HASH_SIZE         128

/* 定义主叫号码的hash表大小 */
#define SC_CALLER_HASH_SIZE     128

/* 定义主叫号码组的hash表大小 */
#define SC_CALLER_GRP_HASH_SIZE 128

/* 定义主叫号码设定的hash表大小 */
#define SC_CALLER_SETTING_HASH_SIZE   128

/* 路由 中继组的最大数量 */
#define SC_ROUT_GW_GRP_MAX_SIZE   5


/*  DID号码绑定类型 */
typedef enum tagSCDIDBindType{
    SC_DID_BIND_TYPE_SIP     = 1,               /* DID号码被绑定到SIP账户 */
    SC_DID_BIND_TYPE_QUEUE   = 2,               /* DID号码被绑定到坐席队列 */
    SC_DID_BIND_TYPE_AGENT   = 3,               /* DID号码绑定到坐席 */
    SC_DID_BIND_TYPE_BUTT
}SC_DID_BIND_TYPE_EN;

/*  路由目的地类型 */
typedef enum tagSCDestType{
    SC_DEST_TYPE_GATEWAY     = 1,               /* 呼叫目的为网关 */
    SC_DEST_TYPE_GW_GRP      = 2,               /* 呼叫目的为网关组 */
    SC_DEST_TYPE_BUTT
}SC_DEST_TYPE_EN;

/* 主叫号码选择策略 */
typedef enum tagCallerSelectPolicy
{
    SC_CALLER_SELECT_POLICY_IN_ORDER = 0,   /* 顺序选择号码 */
    SC_CALLER_SELECT_POLICY_RANDOM          /* 随机选择 */
}SC_CALLER_SELECT_POLICY_EN;


/* User ID 描述节点 */
typedef struct tagSCUserIDNode{
    U32  ulCustomID;                             /* 用户 ID */
    U32  ulSIPID;                                /* 账户 ID */
    S8   szUserID[SC_TEL_NUMBER_LENGTH];         /* SIP账户 */
    S8   szExtension[SC_TEL_NUMBER_LENGTH];      /* 分机号 */

    SC_STATUS_TYPE_EN  enStatus;                 /* 状态 */

    SC_SIP_ACCT_ST stStat;
}SC_USER_ID_NODE_ST;

/* 黑名单HASH表节点 */
typedef struct tagSCBlackListNode{
    U32  ulID;                                   /* 索引 */
    U32  ulFileID;                                   /* 索引 */
    U32  ulCustomerID;                           /* 用户ID */
    S8   szNum[SC_TEL_NUMBER_LENGTH];            /* 表达式 */
    U32  ulType;                                 /* 类型，号码或者正则表达式 */
}SC_BLACK_LIST_NODE;

/* DIDI号码们描述节点 */
typedef struct tagSCDIDNode{
    U32   ulCustomID;                             /* 用户ID */
    U32   ulDIDID;                                /* DID 号码ID */
    S8    szDIDNum[SC_TEL_NUMBER_LENGTH];         /* DID 号码 */
    U32   ulBindType;                             /* 绑定类型 refer to SC_DID_BIND_TYPE_EN */
    U32   ulBindID;                               /* 绑定结果 */
}SC_DID_NODE_ST;

/* TT号信息 */
typedef struct tagSCTTNumNode{
    U32 ulID;
    S8  szPrefix[SC_TEL_NUMBER_LENGTH];
    S8  szAddr[SC_IP_ADDR_LEN];
}SC_TT_NODE_ST;

/* 网关描述节点 */
typedef struct tagSCGWNode
{
    U32 ulGWID;                                    /* 网关ID */
    S8  szGWDomain[SC_GW_DOMAIN_LEG];              /* 网关的域，暂时没有用的 */
    BOOL bExist;                                   /* 该标识用来判断数据库是否有该数据 */

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

/* 路由描述节点 */
typedef struct tagSCRouteNode
{
    U32        ulID;
    BOOL       bExist;                            /* 该标记用来检查是否来自于数据库 */

    U8         ucHourBegin;                       /* 开始时间，小时 */
    U8         ucMinuteBegin;                     /* 开始时间，分钟 */
    U8         ucHourEnd;                         /* 结束时间，小时 */
    U8         ucMinuteEnd;                       /* 结束时间，分钟 */

    S8         szCallerPrefix[SC_NUM_PREFIX_LEN]; /* 前缀长度 */
    S8         szCalleePrefix[SC_NUM_PREFIX_LEN]; /* 前缀长度 */

    U32        ulDestType;                        /* 目的类型 */
    U32        aulDestID[SC_ROUT_GW_GRP_MAX_SIZE];/* 目的ID */

    U16        usCallOutGroup;
    U8         aucReserv[2];

}SC_ROUTE_NODE_ST;

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
    pthread_mutex_t  mutexCallerList;   /* 锁 */
}SC_CALLER_GRP_NODE_ST;

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

/* 事件队列 */
typedef struct tagSCEventNode
{
    list_t stLink;
    esl_event_t *pstEvent;
}SC_EP_EVENT_NODE_ST;

/* ESL客户端控制块 */
typedef struct tagSCEventProcessHandle
{
    pthread_t           pthID;
    esl_handle_t        stRecvHandle;                /*  esl 接收 句柄 */
    esl_handle_t        stSendHandle;                /*  esl 发送 句柄 */

    BOOL                blIsESLRunning;              /* ESL是否连接正常 */
    BOOL                blIsWaitingExit;             /* 任务是否正在等待退出 */
    U32                 ulESLDebugLevel;             /* ESL调试级别 */
}SC_EP_HANDLE_ST;


#endif

