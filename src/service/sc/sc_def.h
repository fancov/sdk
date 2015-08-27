/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_task_pub.h
 *
 *  创建时间: 2014年12月16日10:15:56
 *  作    者: Larry
 *  描    述: 业务控制模块，群呼任务相关定义
 *  修改历史:
 */

#ifndef __SC_TASK_PUB_H__
#define __SC_TASK_PUB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
extern BOOL                 g_blSCInitOK;


/* include private header files */

/* define marcos */
/* 定义HTTP服务器最大个数 */
#define SC_MAX_HTTPD_NUM               1


/* 定义最多有16个客户端连接HTTP服务器 */
#define SC_MAX_HTTP_CLIENT_NUM         16

/* 定义HTTP API中最大参数个数 */
#define SC_API_PARAMS_MAX_NUM          24

/* 定有HTTP请求行中，文件名的最大长度 */
#define SC_HTTP_REQ_MAX_LEN            64

/* 定义ESL模块命令的最大长度 */
#define SC_ESL_CMD_BUFF_LEN            1024

/* 定义呼叫模块，最大放音次数 */
#define SC_DEFAULT_PLAYCNT             3

/* 定义呼叫模块，最大无人接听超时时间 */
#define SC_MAX_TIMEOUT4NOANSWER        10

/* 电话号码长度 */
#define SC_TEL_NUMBER_LENGTH           24

/* IP地址长度 */
#define SC_IP_ADDR_LEN                 32

/* 工号长度最大值 */
#define SC_EMP_NUMBER_LENGTH           12

/* 每个任务最多时间段描述节点 */
#define SC_MAX_PERIOD_NUM              4

/* UUID 最大长度 */
#define SC_MAX_UUID_LENGTH             40

/* 语言文件名长度 */
#define SC_MAX_AUDIO_FILENAME_LEN      128

/* 比例呼叫的比例 */
#define SC_MAX_CALL_MULTIPLE           3

#define SC_MAX_CALL_PRE_SEC            200


#define SC_MAX_SRV_TYPE_PRE_LEG        4

/* 最大呼叫数量 */
#define SC_MAX_CALL                    3000

/* 最大任务数 */
#define SC_MAX_TASK_NUM                1024

/* 最大呼叫数 */
#define SC_MAX_SCB_NUM                 SC_MAX_CALL*2

/* SCB hash表最大数 */
#define SC_MAX_SCB_HASH_NUM            4096

/* 每个任务的坐席最大数 */
#define SC_MAX_SITE_NUM                1024

/* 每个任务主叫号码最大数量 */
#define SC_MAX_CALLER_NUM              1024

/* 每个任务被叫号码最小数量(一次LOAD的最大数量，实际会根据实际情况计算) */
#define SC_MIN_CALLEE_NUM              65535

/* 定义一个呼叫最大可以持续时长。12小时(43200s) */
#define SC_MAX_CALL_DURATION           43200

/* 允许同一个号码被重复呼叫的最大时间间隔，4小时 */
#define SC_MAX_CALL_INTERCAL           60 * 4

#define SC_CALL_THRESHOLD_VAL0         40         /* 阀值0，百分比 ，需要调整呼叫发起间隔*/
#define SC_CALL_THRESHOLD_VAL1         80         /* 阀值1，百分比 ，需要调整呼叫发起间隔*/

#define SC_CALL_INTERVAL_VAL0          300        /* 当前呼叫量/最大呼叫量*100 < 阀值0，300毫秒一个 */
#define SC_CALL_INTERVAL_VAL1          500        /* 当前呼叫量/最大呼叫量*100 < 阀值1，500毫秒一个 */
#define SC_CALL_INTERVAL_VAL2          1000       /* 当前呼叫量/最大呼叫量*100 > 阀值1，1000毫秒一个 */

#define SC_MEM_THRESHOLD_VAL0          90         /* 系统状态阀值，内存占用率阀值0 */
#define SC_MEM_THRESHOLD_VAL1          95         /* 系统状态阀值，内存占用率阀值0 */
#define SC_CPU_THRESHOLD_VAL0          90         /* 系统状态阀值，CPU占用率阀值0 */
#define SC_CPU_THRESHOLD_VAL1          95         /* 系统状态阀值，CPU占用率阀值0 */

#define SC_INVALID_INDEX               0

#define SC_NUM_VERIFY_TIME             3          /* 语音验证码播放次数 */
#define SC_NUM_VERIFY_TIME_MAX         10         /* 语音验证码播放次数 */
#define SC_NUM_VERIFY_TIME_MIN         2          /* 语音验证码播放次数 */

#define SC_MASTER_TASK_INDEX           0
#define SC_EP_TASK_NUM                 2


#define SC_BGJOB_HASH_SIZE             128


/* 定义运营商的ID */
#define SC_TOP_USER_ID                 1

#define SC_LIST_MIN_CNT                3

#define SC_TASK_AUDIO_PATH             "/var/www/html/data/audio"

#define SC_RECORD_FILE_PATH            "/var/record"

#define SC_NOBODY_UID                  99
#define SC_NOBODY_GID                  99

#define SC_NUM_TRANSFORM_PREFIX_LEN    16

/* 检测一个TCB是有正常的Task和CustomID */
#define SC_TCB_HAS_VALID_OWNER(pstTCB)                        \
    ((pstTCB)                                                 \
    && (pstTCB)->ulTaskID != 0                                \
    && (pstTCB)->ulTaskID != U32_BUTT                         \
    && (pstTCB)->ulCustomID != 0                              \
    && (pstTCB)->ulCustomID != U32_BUTT)

#define SC_SCB_HAS_VALID_OWNER(pstSCB)                        \
    ((pstSCB)                                                 \
    && (pstSCB)->ulTaskID != 0                                \
    && (pstSCB)->ulTaskID != U32_BUTT                         \
    && (pstSCB)->ulCustomID != 0                              \
    && (pstSCB)->ulCustomID != U32_BUTT)


#define SC_TCB_VALID(pstTCB)                                  \
    ((pstTCB)                                                 \
    && (pstTCB)->ulTaskID != 0)

#define SC_SCB_IS_VALID(pstSCB)                               \
    ((pstSCB) && (pstSCB)->bValid)

#define SC_SCB_SET_STATUS(pstSCB, ulStatus)                   \
do                                                            \
{                                                             \
    if (DOS_ADDR_INVALID(pstSCB)                              \
        || ulStatus >= SC_SCB_BUTT)                           \
    {                                                         \
        break;                                                \
    }                                                         \
    pthread_mutex_lock(&(pstSCB)->mutexSCBLock);              \
    sc_call_trace((pstSCB), "SCB Status Change %s -> %s"      \
                , sc_scb_get_status((pstSCB)->ucStatus)       \
                , sc_scb_get_status(ulStatus));               \
    (pstSCB)->ucStatus = (U8)ulStatus;                        \
    pthread_mutex_unlock(&(pstSCB)->mutexSCBLock);            \
}while(0)

#define SC_SCB_SET_SERVICE(pstSCB, ulService)                 \
do                                                            \
{                                                             \
    if (DOS_ADDR_INVALID(pstSCB)                              \
      || (pstSCB)->ucCurrentSrvInd >= SC_MAX_SRV_TYPE_PRE_LEG \
      || ulService >= SC_SERV_BUTT)                           \
    {                                                         \
        break;                                                \
    }                                                         \
    pthread_mutex_lock(&(pstSCB)->mutexSCBLock);              \
    sc_call_trace((pstSCB), "SCB Add service.");              \
    (pstSCB)->aucServiceType[(pstSCB)->ucCurrentSrvInd++]     \
                = (U8)ulService;                              \
    pthread_mutex_unlock(&(pstSCB)->mutexSCBLock);            \
}while(0)


#define SC_TRACE_HTTPD                  (1<<1)
#define SC_TRACE_HTTP                   (1<<2)
#define SC_TRACE_TASK                   (1<<3)
#define SC_TRACE_SC                     (1<<4)
#define SC_TRACE_ACD                    (1<<5)
#define SC_TRACE_DIAL                   (1<<6)
#define SC_TRACE_FUNC                   (1<<7)
#define SC_TRACE_ALL                    0xFFFFFFFF


#define SC_EP_EVENT_LIST \
            "BACKGROUND_JOB " \
            "CHANNEL_PARK " \
            "CHANNEL_CREATE " \
            "CHANNEL_ANSWER " \
            "PLAYBACK_STOP " \
            "CHANNEL_HANGUP " \
            "CHANNEL_HANGUP_COMPLETE " \
            "CHANNEL_HOLD " \
            "CHANNEL_UNHOLD " \
            "DTMF " \
            "SESSION_HEARTBEAT "


enum tagCallServiceType{
    SC_SERV_OUTBOUND_CALL           = 0,   /* 由FS向外部(包括SIP、PSTN)发起的呼叫 */
    SC_SERV_INBOUND_CALL            = 1,   /* 由外部(包括SIP、PSTN)向FS发起的呼叫 */
    SC_SERV_INTERNAL_CALL           = 2,   /* FS和SIP终端之间的呼叫 */
    SC_SERV_EXTERNAL_CALL           = 3,   /* FS和PSTN之间的呼叫 */

    SC_SERV_AUTO_DIALING            = 4,   /* 自动拨号外乎，比例呼叫 */
    SC_SERV_PREVIEW_DIALING         = 5,   /* 预览外乎 */
    SC_SERV_PREDICTIVE_DIALING      = 6,   /* 预测外乎 */

    SC_SERV_RECORDING               = 7,   /* 录音业务 */
    SC_SERV_FORWORD_CFB             = 8,   /* 忙转业务 */
    SC_SERV_FORWORD_CFU             = 9,   /* 无条件转业务 */
    SC_SERV_FORWORD_CFNR            = 10,   /* 无应答转业务 */
    SC_SERV_BLIND_TRANSFER          = 11,  /* 忙转业务 */
    SC_SERV_ATTEND_TRANSFER         = 12,  /* 协商转业务 */

    SC_SERV_PICK_UP                 = 13,  /* 代答业务 */        /* ** */
    SC_SERV_CONFERENCE              = 14,  /* 会议 */
    SC_SERV_VOICE_MAIL_RECORD       = 15,  /* 语言邮箱 */
    SC_SERV_VOICE_MAIL_GET          = 16,  /* 语言邮箱 */

    SC_SERV_SMS_RECV                = 17,  /* 接收短信 */
    SC_SERV_SMS_SEND                = 18,  /* 发送短信 */
    SC_SERV_MMS_RECV                = 19,  /* 接收彩信 */
    SC_SERV_MMS_SNED                = 20,  /* 发送彩信 */

    SC_SERV_FAX                     = 21,  /* 传真业务 */
    SC_SERV_INTERNAL_SERVICE        = 22,  /* 内部业务 */

    SC_SERV_AGENT_CALLBACK          = 23,  /* 回呼坐席 */
    SC_SERV_AGENT_SIGNIN            = 24,  /* 坐席签入 */
    SC_SERV_AGENT_CLICK_CALL        = 25,  /* 坐席签入 */

    SC_SERV_NUM_VERIFY              = 26,  /* 号码验证 */

    SC_SERV_CALL_INTERCEPT          = 27,  /* 号码验证 */
    SC_SERV_CALL_WHISPERS           = 28,  /* 号码验证 */

    SC_SERV_BUTT                    = 255
}SC_CALL_SERVICE_TYPE_EN;

enum {
    SC_NUM_TYPE_USERID              = 0,   /* 号码为SIP User ID */
    SC_NUM_TYPE_EXTENSION,                 /* 号码为分机号 */
    SC_NUM_TYPE_OUTLINE,                   /* 号码为长号 */

    SC_NUM_TYPE_BUTT
};

enum tagCallDirection{
    SC_DIRECTION_PSTN,                     /* 呼叫方向来自于PSTN */
    SC_DIRECTION_SIP,                       /* 呼叫方向来自于SIP UA */

    SC_DIRECTION_INVALID                    /* 非法值 */
};

enum tagCalleeStatus
{
    SC_CALLEE_UNCALLED  =  0,           /* 被叫号码没有被呼叫过 */
    SC_CALLEE_NORMAL,                   /* 被叫号码正常呼叫了，客户也接通了 */
    SC_CALLEE_NOT_CONN,                 /* 被叫号码呼叫了，客户没有接听 */
    SC_CALLEE_NOT_EXIST,                /* 被叫号码为空号 */
    SC_CALLEE_REJECT,                   /* 被叫号码被客户拒绝了 */
};

typedef enum tagSiteAccompanyingStatus{
    SC_SITE_ACCOM_DISABLE              = 0,       /* 分机随行，禁止 */
    SC_SITE_ACCOM_ENABLE,                         /* 分机随行，允许 */

    SC_SITE_ACCOM_BUTT                 = 255      /* 分机随行，非法值 */
}SC_SITE_ACCOM_STATUS_EN;

typedef enum tagIntervalService
{
    SC_INTER_SRV_NUMBER_VIRFY            = 0,  /* 号码审核业务 */
    SC_INTER_SRV_INGROUP_CALL,                 /* 组内呼叫 */        /* * */
    SC_INTER_SRV_OUTGROUP_CALL,                /* 组外呼叫 */        /* *9 */
    SC_INTER_SRV_HOTLINE_CALL,                 /* 热线呼叫 */
    SC_INTER_SRV_SITE_SIGNIN,                  /* 坐席签入呼叫 */    /* *2 */

    SC_INTER_SRV_WARNING,                      /* 告警信息 */
    SC_INTER_SRV_QUERY_IP,                     /* 查IP信息 */        /* *158 */
    SC_INTER_SRV_QUERY_EXTENTION,              /* 查分机信息 */      /* *114 */
    SC_INTER_SRV_QUERY_AMOUNT,                 /* 查余额业务 */      /* *199 */

    SC_INTER_SRV_BUTT
}SC_INTERVAL_SERVICE_EN;

typedef enum tagSysStatus{
    SC_SYS_NORMAL                      = 3,
    SC_SYS_BUSY                        = 4,       /* 任务状态，系统忙，暂停呼叫量在80%以上的任务发起呼叫 */
    SC_SYS_ALERT,                                 /* 任务状态，系统忙，只允许高优先级客户，并且呼叫量在80%以下的客户发起呼叫 */
    SC_SYS_EMERG,                                 /* 任务状态，系统忙，暂停所有任务 */

    SC_SYS_BUTT                        = 255      /* 任务状态，非法值 */
}SC_SYS_STATUS_EN;

typedef enum tagTaskStatusInDB{
    SC_TASK_STATUS_DB_STOP             = 0,/* 数据库中任务状态 */
    SC_TASK_STATUS_DB_START,               /* 数据库中任务状态 */
    SC_TASK_STATUS_DB_PAUSED,              /* 数据库中任务状态 */
    SC_TASK_STATUS_DB_CONTINUE,            /* 数据库中任务状态 */

    SC_TASK_STATUS_DB_BUTT
}SC_TASK_STATUS_DB_EN;

typedef enum tagTaskStatus{
    SC_TASK_INIT                       = 0,       /* 任务状态，初始化 */
    SC_TASK_WORKING,                              /* 任务状态，工作 */
    SC_TASK_STOP,                                 /* 任务状态，停止，不再发起呼叫，如果所有呼叫结束即将释放资源 */
    SC_TASK_PAUSED,                               /* 任务状态，暂停 */
    SC_TASK_SYS_BUSY,                             /* 任务状态，系统忙，暂停呼叫量在80%以上的任务发起呼叫 */
    SC_TASK_SYS_ALERT,                            /* 任务状态，系统忙，只允许高优先级客户，并且呼叫量在80%以下的客户发起呼叫 */
    SC_TASK_SYS_EMERG,                            /* 任务状态，系统忙，暂停所有任务 */

    SC_TASK_BUTT                       = 255      /* 任务状态，非法值 */
}SC_TASK_STATUS_EN;

typedef enum tagTaskMode{
    SC_TASK_MODE_KEY4AGENT           = 0,         /* 呼叫任务模式，放音，按键之后转坐席 */
    SC_TASK_MODE_KEY4AGENT1          = 1,         /* 呼叫任务模式，放音，按键之后转坐席 */
    SC_TASK_MODE_DIRECT4AGETN,                    /* 呼叫任务模式，接通后直接转坐席 */
    SC_TASK_MODE_AUDIO_ONLY,                      /* 呼叫任务模式，放音后结束 */
    SC_TASK_MODE_AGENT_AFTER_AUDIO,               /* 呼叫任务模式，放音后转坐席 */

    SC_TASK_MODE_BUTT
}SC_TASK_MODE_EN;

typedef enum tagTaskPriority{
    SC_TASK_PRI_LOW                       = 0,    /* 任务优先级，低优先级 */
    SC_TASK_PRI_NORMAL,                           /* 任务优先级，正常优先级 */
    SC_TASK_PRI_HIGHT,                            /* 任务优先级，高优先级 */
}SC_TASK_PRI_EN;

typedef enum tagCallNumType{
    SC_CALL_NUM_TYPE_NORMOAL              = 0,       /* 号码类型，正常的号码 */
    SC_CALL_NUM_TYPE_EXPR,                           /* 号码类型，正则表达式 */

    SC_CALL_NUM_TYPE_BUTT                 = 255      /* 号码类型，非法值 */
}SC_CALL_NUM_TYPE_EN;


typedef enum tagSCBStatus
{
    SC_SCB_IDEL                           = 0,     /* SCB状态，空闲状态 */
    SC_SCB_INIT,                                   /* SCB状态，业务初始化状态 */
    SC_SCB_AUTH,                                   /* SCB状态，业务认证状态 */
    SC_SCB_EXEC,                                   /* SCB状态，业务执行状态 */
    SC_SCB_ACTIVE,                                 /* SCB状态，业务激活状态 */
    SC_SCB_RELEASE,                                /* SCB状态，业务释放状态 */

    SC_SCB_BUTT
}SC_SCB_STATUS_EN;

typedef enum tagSCStatusType
{
    SC_STATUS_TYPE_UNREGISTER,
    SC_STATUS_TYPE_REGISTER,

    SC_STATUS_TYPE_BUTT
}SC_STATUS_TYPE_EN;

typedef enum tagSCCallRole
{
    SC_CALLEE,
    SC_CALLER,

    SC_CALL_ROLE_BUTT
}SC_CALL_ROLE_EN;

typedef enum tagTransferRole
{
    SC_TRANS_ROLE_NOTIFY        = 0,
    SC_TRANS_ROLE_PUBLISH       = 1,
    SC_TRANS_ROLE_SUBSCRIPTION  = 2,

    SC_TRANS_ROLE_BUTT,
}SC_TRANSFER_ROLE_EN;


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

/* 定义加入号码组的号码类型 */
typedef enum tagSCNumberType
{
    SC_NUMBER_TYPE_CFG = 0,         /* 系统手动配置的主叫号码 */
    SC_NUMBER_TYPE_DID,             /* 系统的did号码 */
}SC_NUMBER_TYPE_EN;

#define SC_EP_STAT_RECV 0
#define SC_EP_STAT_PROC 1

typedef struct tagEPMsgStat
{
    U32   ulCreate;
    U32   ulPark;
    U32   ulAnswer;
    U32   ulHungup;
    U32   ulHungupCom;
    U32   ulDTMF;
    U32   ulBGJob;
    U32   ulHold;
    U32   ulUnhold;
}SC_EP_MSG_STAT_ST;

typedef struct tagBSMsgStat
{
    U32  ulAuthReq;
    U32  ulAuthReqSend;
    U32  ulAuthRsp;
    U32  ulBillingReq;
    U32  ulBillingReqSend;
    U32  ulBillingRsp;
    U32  ulHBReq;
    U32  ulHBRsp;
}SC_BS_MSG_STAT_ST;

typedef struct tagSystemStat
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
    U32  ulSystemUpTime;
    U32  ulSystemIsWorking;
}SC_SYSTEM_STAT_ST;

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

typedef struct tagSiteStat
{
    U32  ulSelectCnt;
    U32  ulCallCnt;      /* 暂时和 ulSelectCnt 保持一致 */
    U32  ulIncomingCall; /* 暂时没有实现 */
    U32  ulOutgoingCall; /* 暂时没有实现 */
    U32  ulTimeOnSignin;             /* 长签总时间 */
    U32  ulTimeOnthePhone;           /* 在线总时间 */
}SC_SITE_STAT_ST;

typedef struct tagSiteGrpStat
{
    U32  ulCallCnt;
    U32  ulCallinQueue;
    U32  ulTotalWaitingTime;      /* 暂时没有实现 */
    U32  ulTotalWaitingCall;
}SC_SITE_GRP_STAT_ST;

typedef struct tagSIPAcctStat
{
    U32   ulRegisterCnt;
    U32   ulRegisterFailCnt;
    U32   ulUnregisterCnt;
}SC_SIP_ACCT_ST;


typedef struct tagCallerQueryNode{
    U16        usNo;                              /* 编号 */
    U8         bValid;
    U8         bTraceON;                          /* 是否跟踪 */

    U32        ulIndexInDB;                       /* 数据库中的ID */
    U32        ulCustomerID;                      /* 所属客户id */
    U32        ulTimes;                           /* 号码被呼叫选中的次数，用做统计 */

    S8         szNumber[SC_TEL_NUMBER_LENGTH];    /* 号码缓存 */
}SC_CALLER_QUERY_NODE_ST;

/* define structs */
typedef struct tagTelNumQueryNode
{
    list_t     stLink;                            /* 队列链表节点 */

    U32        ulIndex;                           /* 数据库中的ID */

    U8         ucTraceON;                         /* 是否跟踪 */
    U8         ucCalleeType;                      /* 被叫号码类型， refer to enum SC_CALL_NUM_TYPE_EN */
    U8         aucRes[2];

    S8         szNumber[SC_TEL_NUMBER_LENGTH];    /* 号码缓存 */
}SC_TEL_NUM_QUERY_NODE_ST;

typedef struct tagSiteQueryNode
{
    U16        usSCBNo;
    U16        usRec;

    U32        bValid:1;
    U32        bRecord:1;
    U32        bTraceON:1;
    U32        bAllowAccompanying:1;              /* 是否允许分机随行 refer to SC_SITE_ACCOM_STATUS_EN */
    U32        ulRes1:28;

    U32        ulStatus;                          /* 坐席状态 refer to SC_SITE_STATUS_EN */
    U32        ulSiteID;                          /* 坐席数据库编号 */

    S8         szUserID[SC_TEL_NUMBER_LENGTH];    /* SIP User ID */
    S8         szExtension[SC_TEL_NUMBER_LENGTH]; /* 分机号 */
    S8         szEmpNo[SC_EMP_NUMBER_LENGTH];     /* 号码缓存 */
}SC_SITE_QUERY_NODE_ST;

typedef struct tagTaskAllowPeriod{
    U8         ucValid;
    U8         ucWeekMask;                        /* 周控制，使用位操作，第0位为星期天 */
    U8         ucHourBegin;                       /* 开始时间，小时 */
    U8         ucMinuteBegin;                     /* 开始时间，分钟 */

    U8         ucSecondBegin;                     /* 开始时间，秒 */
    U8         ucHourEnd;                         /* 结束时间，小时 */
    U8         ucMinuteEnd;                       /* 结束时间，分钟 */
    U8         ucSecondEnd;                       /* 结束时间，秒 */

    U32        ulRes;
}SC_TASK_ALLOW_PERIOD_ST;

typedef struct tagSCBExtraData
{
    U32             ulStartTimeStamp;           /* 起始时间戳 */
    U32             ulRingTimeStamp;            /* 振铃时间戳 */
    U32             ulAnswerTimeStamp;          /* 应答时间戳 */
    U32             ulIVRFinishTimeStamp;       /* IVR播放完成时间戳 */
    U32             ulDTMFTimeStamp;            /* (第一个)二次拨号时间戳 */
    U32             ulBridgeTimeStamp;          /* LEG桥接时间戳 */
    U32             ulByeTimeStamp;             /* 释放时间戳 */
    U32             ulPeerTrunkID;              /* 对端中继ID */

    U8              ucPayloadType;              /* 媒体类型 */
    U8              ucPacketLossRate;           /* 收包丢包率,0-100 */
}SC_SCB_EXTRA_DATA_ST;


/* 呼叫控制块 */
typedef struct tagSCSCB{
    U16       usSCBNo;                            /* 编号 */
    U16       usOtherSCBNo;                       /* 另外一个leg的SCB编号 */

    U16       usTCBNo;                            /* 任务控制块编号ID */
    U16       usSiteNo;                           /* 坐席编号 */

    U32       ulAllocTime;
    U32       ulCustomID;                         /* 当前呼叫属于哪个客户 */
    U32       ulAgentID;                          /* 当前呼叫属于哪个客户 */
    U32       ulTaskID;                           /* 当前任务ID */
    U32       ulTrunkID;                          /* 中继ID */

    U8        ucStatus;                           /* 呼叫控制块编号，refer to SC_SCB_STATUS_EN */
    U8        ucServStatus;                       /* 业务状态 */
    U8        ucTerminationFlag;                  /* 业务终止标志 */
    U8        ucTerminationCause;                 /* 业务终止原因 */

    U8        aucServiceType[SC_MAX_SRV_TYPE_PRE_LEG];        /* 业务类型 列表*/

    U8        ucMainService;
    U8        ucCurrentSrvInd;                    /* 当前空闲的业务类型索引 */
    U8        ucLegRole;                          /* 主被叫标示 */
    U8        ucCurrentPlyCnt;                    /* 当前放音次数 */

    U8        ucTranforRole;                      /* transfer角色 */
    U8        ucRes;
    U16       usPublishSCB;                       /* 发起放SCBNo */

    U16       usHoldCnt;                          /* 被hold的次数 */
    U16       usHoldTotalTime;                    /* 被hold的总时长 */
    U32       ulLastHoldTimetamp;                 /* 上次hold是的时间戳，解除hold的时候值零 */

    U32       bValid:1;                           /* 是否合法 */
    U32       bTraceNo:1;                         /* 是否跟踪 */
    U32       bBanlanceWarning:1;                 /* 是否余额告警 */
    U32       bNeedConnSite:1;                    /* 接通后是否需要接通坐席 */
    U32       bWaitingOtherRelase:1;              /* 是否在等待另外一跳退释放 */
    U32       bRecord:1;                          /* 是否录音 */
    U32       bIsAgentCall:1;                     /* 是否在呼叫坐席 */
    U32       bIsInQueue:1;                       /* 是否已经入队列了 */
    U32       bChannelCreated:1;                  /* FREESWITCH 是否为该同呼叫创建了通道 */
    U32       ulRes:25;

    U32       ulCallDuration;                     /* 呼叫时长，防止吊死用，每次心跳时更新 */

    U32       ulRes1;

    S32       lBalance;                           /* 余额,单位:分 */

    S8        szCallerNum[SC_TEL_NUMBER_LENGTH];  /* 主叫号码 */
    S8        szCalleeNum[SC_TEL_NUMBER_LENGTH];  /* 被叫号码 */
    S8        szANINum[SC_TEL_NUMBER_LENGTH];     /* 被叫号码 */
    S8        szDialNum[SC_TEL_NUMBER_LENGTH];    /* 用户拨号 */
    S8        szSiteNum[SC_TEL_NUMBER_LENGTH];    /* 坐席号码 */
    S8        szUUID[SC_MAX_UUID_LENGTH];         /* Leg-A UUID */

    S8        *pszRecordFile;

    SC_SCB_EXTRA_DATA_ST *pstExtraData;           /* 结算话单是需要的额外数据 */

    pthread_mutex_t mutexSCBLock;                 /* 保护SCB的锁 */
}SC_SCB_ST;

/* 定义SCBhash表 */
typedef struct tagSCBHashNode
{
    HASH_NODE_S     stNode;                       /* hash链表节点 */

    S8              szUUID[SC_MAX_UUID_LENGTH];   /* UUID */
    SC_SCB_ST       *pstSCB;                      /* SCB指针 */

    sem_t           semSCBSyn;
}SC_SCB_HASH_NODE_ST;


typedef struct tagTaskCB
{
    U16        usTCBNo;                           /* 编号 */
    U8         ucValid;                           /* 是否被使用 */
    U8         ucTaskStatus;                      /* 任务状态 refer to SC_TASK_STATUS_EN */

    U32        ulAllocTime;
    U8         ucPriority;                        /* 任务优先级 */
    U8         ucAudioPlayCnt;                    /* 语言播放次数 */
    U8         bTraceON;                          /* 是否跟踪 */
    U8         bTraceCallON;                      /* 是否跟踪呼叫 */

    U8         ucMode;                            /* 任务模式 refer to SC_TASK_MODE_EN*/
    U8         aucRess[3];

    U32        ulTaskID;                          /* 呼叫任务ID */
    U32        ulCustomID;                        /* 呼叫任务所属 */
    U32        ulCurrentConcurrency;              /* 当前并发数 */
    U32        ulMaxConcurrency;                  /* 当前并发数 */
    U32        ulAgentQueueID;                    /* 坐席队列编号 */

    U16        usSiteCount;                       /* 坐席数量 */
    U16        usCallerCount;                     /* 当前主叫号码数量 */
    U32        ulCalleeCount;
    U32        ulLastCalleeIndex;                 /* 用于数据分页 */

    list_t     stCalleeNumQuery;                  /* 被叫号码缓存 refer to struct tagTelNumQueryNode */
    S8         szAudioFileLen[SC_MAX_AUDIO_FILENAME_LEN];  /* 语言文件文件名 */
    SC_CALLER_QUERY_NODE_ST *pstCallerNumQuery;            /* 主叫号码缓存 refer to struct tagTelNumQueryNode */
    SC_TASK_ALLOW_PERIOD_ST astPeriod[SC_MAX_PERIOD_NUM];  /* 任务执行时间段 */

    /* 统计相关 */
    U32        ulTotalCall;                       /* 总呼叫数 */
    U32        ulCallFailed;                      /* 呼叫失败数 */
    U32        ulCallConnected;                   /* 呼叫接通数 */

    pthread_t  pthID;                             /* 线程ID */
    pthread_mutex_t  mutexTaskList;               /* 保护任务队列使用的互斥量 */
}SC_TASK_CB_ST;

typedef struct tagBGJobHash{
    S8       szJobUUID[SC_MAX_UUID_LENGTH];

    U32      ulRCNo;                 /* 对应资源编号 */
}SC_BG_JOB_HASH_NODE_ST;

typedef struct tagTaskCtrlCMD{
    list_t      stLink;
    U32         ulTaskID;                         /* 任务ID */
    U32         ulCMD;                            /* 命令字 */
    U32         ulCustomID;                       /* 客户ID */
    U32         ulAction;                         /* action */
    U32         ulCMDSeq;                         /* 请求序号 */
    U32         ulCMDErrCode;                     /* 错误码 */
    S8          *pszErrMSG;                       /* 错误信息 */
    sem_t       semCMDExecNotify;                 /* 命令执行完毕通知使用的信号量 */
}SC_TASK_CTRL_CMD_ST;

typedef struct tagTaskMngtInfo{
    pthread_t            pthID;                   /* 线程ID */
    pthread_mutex_t      mutexCMDList;            /* 保护命令队列使用的互斥量 */
    pthread_mutex_t      mutexTCBList;            /* 保护任务控制块使用的互斥量 */
    pthread_mutex_t      mutexCallList;           /* 保护呼叫控制块使用的互斥量 */
    pthread_mutex_t      mutexCallHash;           /* 保护呼叫控制块使用的互斥量 */
    pthread_mutex_t      mutexHashBGJobHash;
    pthread_cond_t       condCMDList;             /* 命令队列数据到达通知条件量 */
    U32                  blWaitingExitFlag;       /* 等待退出标示 */

    list_t               stCMDList;               /* 命令队列(节点由HTTP Server创建，有HTTP Server释放) */
    SC_SCB_ST            *pstCallSCBList;         /* 呼叫控制块列表 (需要hash表存储) */
    HASH_TABLE_S         *pstCallSCBHash;         /* 呼叫控制块的hash索引 */
    SC_TASK_CB_ST        *pstTaskList;            /* 任务列表 refer to struct tagTaskCB*/
    U32                  ulTaskCount;             /* 当前正在执行的任务数 */
    HASH_TABLE_S         *pstHashBGJobHash;       /* background-job hash表 */

    SC_SYSTEM_STAT_ST    stStat;
}SC_TASK_MNGT_ST;


/*****************呼叫对待队列相关********************/
typedef struct tagCallWaitQueueNode{
    U32                 ulAgentGrpID;                     /* 坐席组ID */
    U32                 ulStartWaitingTime;               /* 开始等待的时间 */

    pthread_mutex_t     mutexCWQMngt;
    DLL_S               stCallWaitingQueue;               /* 呼叫等待队列 refer to SC_SCB_ST */
}SC_CWQ_NODE_ST;
/***************呼叫对待队列相关结束********************/

/* dialer模块控制块 */
typedef struct tagSCDialerHandle
{
    esl_handle_t        stHandle;                /*  esl 句柄 */
    pthread_t           pthID;
    U32                 ulCallCnt;
    pthread_mutex_t     mutexCallQueue;          /* 互斥锁 */
    pthread_cond_t      condCallQueue;           /* 条件变量 */
    list_t              stCallList;              /* 呼叫队列 */

    BOOL                blIsESLRunning;          /* ESL是否连接正常 */
    BOOL                blIsWaitingExit;         /* 任务是否正在等待退出 */
    S8                  *pszCMDBuff;
}SC_DIALER_HANDLE_ST;

typedef struct tagEPTaskCB
{
    DLL_S            stMsgList;
    pthread_t        pthTaskID;
    pthread_mutex_t  mutexMsgList;
    pthread_cond_t   contMsgList;
}SC_EP_TASK_CB;

/***************号码变换开始********************/

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

    S8                              szReplaceNum[SC_TEL_NUMBER_LENGTH]; /* 如果以*号开头，则后面跟号码组的id */
    S8                              szCallerPrefix[SC_NUM_TRANSFORM_PREFIX_LEN];  /* 主叫前缀 */
    S8                              szCalleePrefix[SC_NUM_TRANSFORM_PREFIX_LEN];  /* 被叫前缀 */
    S8                              szAddPrefix[SC_NUM_TRANSFORM_PREFIX_LEN];     /* 增加前缀 */
    S8                              szAddSuffix[SC_NUM_TRANSFORM_PREFIX_LEN];     /* 增加后缀 */

    U32                             ulExpiry;                           /* 有效期 */

}SC_NUM_TRANSFORM_NODE_ST;

/***************号码变换结束********************/
/* declare functions */
SC_SCB_ST *sc_scb_alloc();
VOID sc_scb_free(SC_SCB_ST *pstSCB);
U32 sc_scb_init(SC_SCB_ST *pstSCB);
U32 sc_call_set_owner(SC_SCB_ST *pstSCB, U32  ulTaskID, U32 ulCustomID);
U32 sc_call_set_trunk(SC_SCB_ST *pstSCB, U32 ulTrunkID);
SC_TASK_CB_ST *sc_tcb_find_by_taskid(U32 ulTaskID);
SC_SCB_ST *sc_scb_get(U32 ulIndex);
U32 sc_ep_terminate_call(SC_SCB_ST *pstSCB);
U32 sc_ep_outgoing_call_proc(SC_SCB_ST *pstSCB);
U32 sc_ep_incoming_call_proc(SC_SCB_ST *pstSCB);
U32 sc_dialer_make_call2pstn(SC_SCB_ST *pstSCB, U32 ulMainService);
SC_TASK_CB_ST *sc_tcb_alloc();
VOID sc_tcb_free(SC_TASK_CB_ST *pstTCB);
U32 sc_tcb_init(SC_TASK_CB_ST *pstTCB);
VOID sc_task_set_owner(SC_TASK_CB_ST *pstTCB, U32 ulTaskID, U32 ulCustomID);
U32 sc_task_get_current_call_cnt(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_caller(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_callee(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_period(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_agent_info(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_other_info(SC_TASK_CB_ST *pstTCB);
U32 sc_task_update_stat(SC_TASK_CB_ST *pstTCB);
U32 sc_task_save_status(U32 ulTaskID, U32 ulStatus, S8 *pszStatus);
U32 sc_task_check_can_call_by_time(SC_TASK_CB_ST *pstTCB);
U32 sc_task_check_can_call_by_status(SC_TASK_CB_ST *pstTCB);
U32 sc_task_get_call_interval(SC_TASK_CB_ST *pstTCB);
U32 sc_task_set_recall(SC_TASK_CB_ST *pstTCB);
U32 sc_task_cmd_queue_add(SC_TASK_CTRL_CMD_ST *pstCMD);
U32 sc_task_cmd_queue_del(SC_TASK_CTRL_CMD_ST *pstCMD);
S8 *sc_task_get_audio_file(U32 ulTCBNo);
U32 sc_task_audio_playcnt(U32 ulTCBNo);
U32 sc_task_get_mode(U32 ulTCBNo);
U32 sc_task_get_timeout_for_noanswer(U32 ulTCBNo);
U32 sc_task_get_agent_queue(U32 ulTCBNo);
U32 sc_dialer_add_call(SC_SCB_ST *pstSCB);
U32 sc_task_concurrency_minus (U32 ulTCBNo);
U32 sc_task_concurrency_add(U32 ulTCBNo);
VOID sc_call_trace(SC_SCB_ST *pstSCB, const S8 *szFormat, ...);
U32 sc_task_callee_set_recall(SC_TASK_CB_ST *pstTCB, U32 ulIndex);
U32 sc_task_load_audio(SC_TASK_CB_ST *pstTCB);
BOOL sc_scb_is_valid(SC_SCB_ST *pstSCB);
U32 sc_task_init(SC_TASK_CB_ST *pstTCB);
BOOL sc_call_check_service(SC_SCB_ST *pstSCB, U32 ulService);
U32 sc_task_continue(SC_TASK_CB_ST *pstTCB);
U32 sc_task_pause(SC_TASK_CB_ST *pstTCB);
U32 sc_task_start(SC_TASK_CB_ST *pstTCB);
U32 sc_task_stop(SC_TASK_CB_ST *pstTCB);
S8 *sc_scb_get_status(U32 ulStatus);
SC_SYS_STATUS_EN sc_check_sys_stat();
U32 sc_ep_search_route(SC_SCB_ST *pstSCB);
U32 sc_ep_get_callee_string(U32 ulRouteID, SC_SCB_ST *pstSCB, S8 *szCalleeString, U32 ulLength);
U32 sc_get_record_file_path(S8 *pszBuff, U32 ulMaxLen, U32 ulCustomerID, S8 *pszCaller, S8 *pszCallee);
U32 sc_dial_make_call_for_verify(U32 ulCustomer, S8 *pszCaller, S8 *pszNumber, S8 *pszPassword, U32 ulPlayCnt);
U32 sc_send_usr_auth2bs(SC_SCB_ST *pstSCB);
U32 sc_send_billing_stop2bs(SC_SCB_ST *pstSCB);
U32 sc_http_gateway_update_proc(U32 ulAction, U32 ulGatewayID);
U32 sc_http_caller_update_proc(U32 ulAction, U32 ulCallerID);
U32 sc_http_eix_update_proc(U32 ulAction, U32 ulEixID);
U32 sc_http_num_lmt_update_proc(U32 ulAction, U32 ulNumlmtID);
U32 sc_http_num_transform_update_proc(U32 ulAction, U32 ulNumTransID);
U32 sc_http_customer_update_proc(U32 ulAction, U32 ulCustomerID);
U32 sc_http_sip_update_proc(U32 ulAction, U32 ulSipID, U32 ulCustomerID);
U32 sc_http_route_update_proc(U32 ulAction, U32 ulRouteID);
U32 sc_http_gw_group_update_proc(U32 ulAction, U32 ulGwGroupID);
U32 sc_http_did_update_proc(U32 ulAction, U32 ulDidID);
U32 sc_ep_update_sip_status(S8 *szUserID, SC_STATUS_TYPE_EN enStatus, U32 *pulSipID);
U32 sc_gateway_delete(U32 ulGatewayID);
U32 sc_load_sip_userid(U32 ulIndex);
U32 sc_load_gateway(U32 ulIndex);
U32 sc_load_route(U32 ulIndex);
U32 sc_route_delete(U32 ulRouteID);
U32 sc_load_gateway_grp(U32 ulIndex);
U32 sc_refresh_gateway_grp(U32 ulIndex);
U32 sc_load_did_number(U32 ulIndex);
U32 sc_load_black_list(U32 ulIndex);
U32 sc_load_num_transform(U32 ulIndex);
U32 sc_load_tt_number(U32 ulIndex);
U32 sc_del_tt_number(U32 ulIndex);
U32 sc_load_number_lmt(U32 ulIndex);
U32 sc_del_number_lmt(U32 ulIndex);
U32 sc_load_customer(U32 ulIndex);
U32 sc_load_caller_setting(U32 ulIndex);
U32 sc_load_caller_grp(U32 ulIndex);
U32 sc_load_caller(U32 ulIndex);
U32 sc_gateway_grp_delete(U32 ulGwGroupID);
U32 sc_black_list_delete(U32 ulBlackListID);
U32 sc_caller_grp_delete(U32 ulCallerGrpID);
U32 sc_did_delete(U32 ulDidID);
U32 sc_caller_delete(U32 ulCallerID);
U32 sc_caller_setting_delete(U32 ulSettingID);
U32 sc_ep_sip_userid_delete(S8 * pszSipID);
U32 sc_caller_delete(U32 ulCallerID);
U32 sc_transform_delete(U32 ulTransformID);
U32 sc_customer_delete(U32 ulCustomerID);
U32 sc_http_black_update_proc(U32 ulAction, U32 ulBlackID);
U32 sc_del_invalid_gateway();
U32 sc_del_invalid_gateway_grp();
U32 sc_del_invalid_route();
U32 sc_ep_esl_execute(const S8 *pszApp, const S8 *pszArg, const S8 *pszUUID);
U32 sc_ep_esl_execute_cmd(const S8* pszCmd);
U32 sc_ep_get_userid_by_id(U32 ulSipID, S8 *pszUserID, U32 ulLength);
S32 sc_ep_gw_grp_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
S32 sc_ep_caller_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
S32 sc_ep_caller_setting_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
S32 sc_ep_caller_grp_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
U32 sc_ep_gw_grp_hash_func(U32 ulGWGrpID);
U32 sc_ep_caller_hash_func(U32 ulCustomerID);
U32 sc_ep_caller_grp_hash_func(U32 ulCustomerID);
U32 sc_ep_caller_setting_hash_func(U32 ulCustomerID);
U32 sc_ep_esl_execute(const S8 *pszApp, const S8 *pszArg, const S8 *pszUUID);
U32 sc_ep_hangup_call(SC_SCB_ST *pstSCB, U32 ulTernmiteCase);
BOOL sc_ep_black_list_check(U32 ulCustomerID, S8 *pszNum);
U32 sc_ep_call_agent_by_grpid(SC_SCB_ST *pstSCB, U32 ulTaskAgentQueueID);
U32 sc_update_callee_status(U32 ulTaskID, S8 *pszCallee, U32 ulStatsu);
U32 sc_update_task_status(U32 ulTaskID,  U32 ulStatsu);
U32 sc_ep_ext_init();
U32 sc_cwq_init();
U32 sc_cwq_start();
U32 sc_cwq_stop();
U32 sc_cwq_add_call(SC_SCB_ST *pstSCB, U32 ulAgentGrpID);
U32 sc_cwq_del_call(SC_SCB_ST *pstSCB);
U32 sc_bg_job_hash_add(S8 *pszUUID, U32 ulUUIDLen, U32 ulRCNo);
U32 sc_bg_job_hash_delete(U32 ulRCNo);
BOOL sc_bg_job_find(U32 ulRCNo);
U32 sc_scb_hash_tables_add(S8 *pszUUID, SC_SCB_ST *pstSCB);
U32 sc_scb_hash_tables_delete(S8 *pszUUID);
SC_SCB_ST *sc_scb_hash_tables_find(S8 *pszUUID);
U32 sc_ep_call_ctrl_proc(U32 ulAction, U32 ulTaskID, U32 ulAgent, U32 ulCustomerID, S8 *pszCallee);
U32 sc_ep_get_custom_by_sip_userid(S8 *pszNum);
BOOL sc_ep_check_extension(S8 *pszNum, U32 ulCustomerID);
U32 sc_dial_make_call2ip(SC_SCB_ST *pstSCB, U32 ulMainService);
U32 sc_ep_num_transform(SC_SCB_ST *pstSCB, U32 ulTrunkID, SC_NUM_TRANSFORM_TIMING_EN enTiming, SC_NUM_TRANSFORM_SELECT_EN enNumSelect);
U32 sc_ep_get_eix_by_tt(S8 *pszTTNumber, S8 *pszEIX, U32 ulLength);
U32 sc_dial_make_call2eix(SC_SCB_ST *pstSCB, U32 ulMainService);
U32 sc_ep_transfer_publish_release(SC_SCB_ST * pstSCBPublish);

/* 以下是和号码组设定相关的API */
U32  sc_caller_setting_select_number(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, U32 ulPolicy, S8 *pszNumber, U32 ulLen);

/* 周期任务 */
U32 sc_num_lmt_stat(U32 ulType, VOID *ptr);
U32 sc_num_lmt_update(U32 ulType, VOID *ptr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SC_TASK_PUB_H__ */

