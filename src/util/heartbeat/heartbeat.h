/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  heartbeat.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 本文件定义心跳模块相关的申明
 *     History:
 */

#ifndef __HEARTBEAT_H__
#define __HEARTBEAT_H__

/* define local macros */
#define MAX_BUFF_LENGTH     1024

/* define heartbeat interval */
#define DEFAULT_HB_INTERVAL_MIN 3
#define DEFAULT_HB_INTERVAL_MAX 30

/* define mac fail count */
#define DEFAULT_HB_FAIL_CNT_MIN 10
#define DEFAULT_HB_FAIL_CNT_MAX 15

/* define heartbeat timeout length */
#define HB_TIMEOUT          3


/* 消息类型 */
enum HEARTBEAT_DATA_TYPE{
    HEARTBEAT_DATA_REG  = 0,
    HEARTBEAT_DATA_REG_RESPONSE,

    HEARTBEAT_DATA_UNREG,
    HEARTBEAT_DATA_UNREG_RESPONSE,

    HEARTBEAT_WARNING_SEND,
    HEARTBEAT_WARNING_SEND_RESPONSE,

    HEARTBEAT_DATA_HB,
};

/* 告警级别 */
typedef enum tagMonNotifyLevel
{
    MON_NOTIFY_LEVEL_EMERG = 0,  /* 紧急告警 */
    MON_NOTIFY_LEVEL_CRITI,      /* 重要告警 */
    MON_NOTIFY_LEVEL_MINOR,      /* 次要告警 */
    MON_NOTIFY_LEVEL_HINT,       /* 提示告警 */

    MON_NOTIFY_LEVEL_BUTT = 31   /* 无效告警 */
}MON_NOTIFY_LEVEL_EN;

/* 特别注意: 该枚举的定义顺序必须和全局变量
 * m_pstMsgMapCN以及m_pstMsgMapEN
 * 的顺序完全对应，否则会出错
 */
typedef enum tagMonNotifyType
{
    MON_NOTIFY_TYPE_LACK_FEE = 0,  /* 账户余额不足 */
    MON_MOTIFY_TYPE_LACK_GW,       /* 中继不够用 */
    MON_NOTIFY_TYPE_LACK_ROUTE,    /* 缺少路由 */

    MON_NOTIFY_TYPE_BUTT = 31     /* 无效值 */
}MON_NOTIFY_TYPE_EN;

/* 告警手段 */
typedef enum tagMonNofityMeans
{
    MON_NOTIFY_MEANS_EMAIL = 0,   /* 邮件 */
    MON_NOTIFY_MEANS_SMS,         /* 短信 */
    MON_NOTIFY_MEANS_AUDIO        /* 语音 */
}MON_NOTIFY_MEANS_EN;

/* 当前IPCC系统的语言环境 */
typedef enum tagMonNotifyLang
{
    MON_NOTIFY_LANG_CN = 0,          /* 简体中文 */
    MON_NOTIFY_LANG_EN               /* 美式英语 */
}MON_NOTIFY_LANG_ENUM;

/* 联系人相关结构，相关需要的信息可继续添加成员 */
typedef struct tagMonContact
{
    S8  szEmail[32];   /* 邮件地址，该地址需开通SMTP服务，方可发邮件 */
    S8  szTelNo[32];   /* 推送消息的手机号码 */
}MON_CONTACT_ST;


/* 该结构用来定义的消息
 * 特别注意:*
 *     为了将外部的告警消息与系统监控告警消息区别开来，
 *     这里的ulWarningID必须是0x07000000与MON_NOTIFY_TYPE_EN
 *     中的某一个值做位或运算的结果.
 *     消息的序列号可不填充数值，由心跳server端统一来维护
 */
typedef struct tagMonNotifyMsg
{
    U32     ulWarningID;         /* 告警ID */
    U32     ulSeq;               /* 消息的序列号 */
    U32     ulLevel;             /* 告警级别 */
    U32     ulCustomerID;        /* 客户ID，该客户的该消息需要报告给运营商相关人 */
    U32     ulSPID;              /* 运营商ID */
    U32     ulRoleID;            /* 企业相关负责人的角色ID */
    time_t  ulCurTime;           /* 消息发送时间 */
    MON_CONTACT_ST stContact;    /* 联系人相关信息，发消息模块可填写，如果不填写，监控模块负责去查找 */
    S8      szContent[128];      /* 消息的内容，比如余额，或者为当前并发数量等 */
    S8      szNotes[128];        /* 备注消息，对content的进一步说明，可有可无，如果无，则发送过来请初始化为空 */
}MON_NOTIFY_MSG_ST;


typedef struct tagMonMsgMap
{
    S8  *szName;     /* 配置项名称 */
    S8  *szTitle;    /* 告警标题 */
    S8  *szDesc;     /* 告警描述 */
}MON_MSG_MAP_ST;


typedef struct tagMonNotifyMeansCB
{
    MON_NOTIFY_MEANS_EN  ulMeans;         /* 告警方式 */
    U32 (*callback)(S8*, S8*, S8*);       /* 回调函数 */
}MON_NOTIFY_MEANS_CB_ST;


/* 模块状态 */
enum PROCESS_HB_STATUS{
    PROCESS_HB_INIT,
    PROCESS_HB_WORKING,
    PROCESS_HB_DEINIT,

    PROCESS_HB_BUTT
};

#define HB_MAX_PROCESS_NAME_LEN DOS_MAX_PROCESS_NAME_LEN
#define HB_MAX_PROCESS_VER_LEN  DOS_MAX_PROCESS_VER_LEN

/* 心跳 */
typedef struct tagHeartbeatData{
    U32 ulCommand;                                  /* 控制命令字 */
    U32 ulLength;                                   /* 消息长度 */
    S8  szProcessName[HB_MAX_PROCESS_NAME_LEN];     /* 进程名称 */
    S8  szProcessVersion[HB_MAX_PROCESS_VER_LEN];   /* 版本号 */
    S8  szResv[8];                                  /* 保留 */
}HEARTBEAT_DATA_ST;

/* 控制块 */
typedef struct tagProcessInfo{
    U32                ulStatus;                                  /* 当前进程状态 */

    socklen_t          ulPeerAddrLen;                             /* 对端地址长度 */
    struct sockaddr_un stPeerAddr;                                /* 对端地址 */
    S32                lSocket;                                   /* 对端socket */

    S8                 szProcessName[HB_MAX_PROCESS_NAME_LEN];    /* 进程名称 */
    S8                 szProcessVersion[HB_MAX_PROCESS_VER_LEN];  /* 版本号 */

    U32                ulHBCnt;
    U32                ulHBFailCnt;                               /* 失败统计 */
#if INCLUDE_BH_CLIENT
    DOS_TMR_ST         hTmrRegInterval;                           /* 注册间隔 */
    DOS_TMR_ST         hTmrSendInterval;                          /* 发送间隔 */
    DOS_TMR_ST         hTmrRecvTimeout;                           /* 发送超时定时器 */
#endif
#if INCLUDE_BH_SERVER
    U32                ulProcessCBNo;                             /* 控制块编号 */
    U32                ulVilad;                                   /* 该控制块是否有效 */
    U32                ulActive;                                  /* 该进程是否处于激活状态 */
    DOS_TMR_ST         hTmrHBTimeout;                             /* 心跳超市定时器，每次收到心跳重启 */
#endif
}PROCESS_INFO_ST;

S32 hb_heartbeat_proc(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_send_heartbeat(PROCESS_INFO_ST *pstProcessInfo);

#if INCLUDE_BH_SERVER
S32 hb_recv_external_warning(VOID *pMsg);
S32 hb_unreg_proc(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_reg_proc(PROCESS_INFO_ST *pstProcessInfo);
VOID* hb_external_warning_proc(VOID* ptr);
#endif

#if INCLUDE_BH_CLIENT
S32 hb_send_reg(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_unreg_response_proc(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_reg_response_proc(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_send_external_warning(PROCESS_INFO_ST *pstProcessInfo, MON_NOTIFY_MSG_ST *pstMsg);
#endif

#endif

