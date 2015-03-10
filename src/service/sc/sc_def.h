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

/* 工号长度最大值 */
#define SC_EMP_NUMBER_LENGTH           8

/* 每个任务最多时间段描述节点 */
#define SC_MAX_PERIOD_NUM              4

/* UUID 最大长度 */
#define SC_MAX_UUID_LENGTH             40

/* 语言文件名长度 */
#define SC_MAX_AUDIO_FILENAME_LEN      128

/* 比例呼叫的比例 */
#define SC_MAX_CALL_MULTIPLE           3

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

typedef enum tagSiteAccompanyingStatus
{
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

typedef enum tagSysStatus
{
    SC_SYS_NORMAL                      = 3,
    SC_SYS_BUSY                        = 4,       /* 任务状态，系统忙，暂停呼叫量在80%以上的任务发起呼叫 */
    SC_SYS_ALERT,                                 /* 任务状态，系统忙，只允许高优先级客户，并且呼叫量在80%以下的客户发起呼叫 */
    SC_SYS_EMERG,                                 /* 任务状态，系统忙，暂停所有任务 */

    SC_SYS_BUTT                        = 255      /* 任务状态，非法值 */
}SC_SYS_STATUS_EN;

typedef enum tagTaskStatusInDB
{
    SC_TASK_STATUS_DB_START            = 0,       /* 数据库中任务状态 */
    SC_TASK_STATUS_DB_STOP,                       /* 数据库中任务状态 */

    SC_TASK_STATUS_DB_BUTT
}SC_TASK_STATUS_DB_EN;

typedef enum tagTaskStatus
{
    SC_TASK_INIT                       = 0,       /* 任务状态，初始化 */
    SC_TASK_WORKING,                              /* 任务状态，工作 */
    SC_TASK_STOP,                                 /* 任务状态，停止，不再发起呼叫，如果所有呼叫结束即将释放资源 */
    SC_TASK_PAUSED,                               /* 任务状态，暂停 */
    SC_TASK_SYS_BUSY,                             /* 任务状态，系统忙，暂停呼叫量在80%以上的任务发起呼叫 */
    SC_TASK_SYS_ALERT,                            /* 任务状态，系统忙，只允许高优先级客户，并且呼叫量在80%以下的客户发起呼叫 */
    SC_TASK_SYS_EMERG,                            /* 任务状态，系统忙，暂停所有任务 */

    SC_TASK_BUTT                       = 255      /* 任务状态，非法值 */
}SC_TASK_STATUS_EN;

typedef enum tagTaskPriority
{
    SC_TASK_PRI_LOW                       = 0,    /* 任务优先级，低优先级 */
    SC_TASK_PRI_NORMAL,                           /* 任务优先级，正常优先级 */
    SC_TASK_PRI_HIGHT,                            /* 任务优先级，高优先级 */
}SC_TASK_PRI_EN;

typedef enum tagCallNumType
{
    SC_CALL_NUM_TYPE_NORMOAL              = 0,       /* 号码类型，正常的号码 */
    SC_CALL_NUM_TYPE_EXPR,                           /* 号码类型，正则表达式 */

    SC_CALL_NUM_TYPE_BUTT                 = 255      /* 号码类型，非法值 */
}SC_CALL_NUM_TYPE_EN;


typedef enum tagSCBStatus
{
    SC_SCB_IDEL                           = 0,     /* SCB状态，空闲状态 */
    SC_SCB_INIT,                                   /* SCB状态，呼叫初始化状态 */
    SC_SCB_AUTH,                                   /* SCB状态，呼叫认证状态 */
    SC_SCB_EXEC,                                   /* SCB状态，呼叫执行状态 */
    SC_SCB_ACTIVE,                                 /* SCB状态，呼叫接通状态状态 */
    SC_SCB_RELEASE,                                /* SCB状态，呼叫释放状态 */

    SC_SCB_BUTT
}SC_SCB_STATUS_EN;

typedef struct tagCallerQueryNode
{
    U16        usNo;                              /* 编号 */
    U8         bValid;
    U8         bTraceON;                          /* 是否跟踪 */

    U32        ulIndexInDB;                       /* 数据库中的ID */

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

typedef struct tagTaskAllowPeriod
{
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


/* 呼叫控制块 */
typedef struct tagSCSCB
{
    U16       usSCBNo;                            /* 编号 */
    U16       usOtherSCBNo;                       /* 另外一个leg的SCB编号 */

    U16       usTCBNo;                            /* 任务控制块编号ID */
    U16       usSiteNo;                           /* 坐席编号 */

    U32       ulCustomID;                         /* 当前呼叫属于哪个客户 */
    U32       ulAgentID;                          /* 当前呼叫属于哪个客户 */
    U32       ulTaskID;                           /* 当前任务ID */
    U32       ulTrunkID;                          /* 中继ID */

    U8        ucStatus;                           /* 呼叫控制块编号，refer to SC_SCB_STATUS_EN */
    U8        ucServStatus;                       /* 业务状态 */
    U8        ucTerminationFlag;                  /* 业务终止标志 */
    U8        ucTerminationCause;                 /* 业务终止原因 */

    U8        aucServiceType[SC_MAX_SRV_TYPE_PRE_LEG];        /* 业务类型 列表*/
    U8        ucCurrentSrvInd;                    /* 当前空闲的业务类型索引 */
    U8        ucLegRole;                          /* 主被叫标示 */
    U8        ucCurrentPlyCnt;                    /* 当前放音次数 */
    U8        aucRes[1];

    U16       usHoldCnt;                          /* 被hold的次数 */
    U16       usHoldTotalTime;                    /* 被hold的总时长 */
    U32       ulLastHoldTimetamp;                 /* 上次hold是的时间戳，解除hold的时候值零 */

    U32       bValid:1;                           /* 是否合法 */
    U32       bTraceNo:1;                         /* 是否跟踪 */
    U32       bBanlanceWarning:1;                 /* 是否余额告警 */
    U32       bNeedConnSite:1;                    /* 接通后是否需要接通坐席 */
    U32       bWaitingOtherRelase:1;              /* 是否在等待另外一跳退释放 */
    U32       ulRes:27;

    U32       ulCallDuration;                     /* 呼叫时长，防止吊死用，每次心跳时更新 */

    U32       ulStartTimeStamp;                   /* 起始时间戳 */
    U32       ulRingTimeStamp;                    /* 振铃时间戳 */
    U32       ulAnswerTimeStamp;                  /* 应答时间戳 */
    U32       ulIVRFinishTimeStamp;               /* IVR播放完成时间戳 */
    U32       ulDTMFTimeStamp;                    /* (第一个)二次拨号时间戳 */
    U32       ulBridgeTimeStamp;                  /* LEG桥接时间戳 */
    U32       ulByeTimeStamp;                     /* 释放时间戳 */

    U32       ulRes1;

    S8        szCallerNum[SC_TEL_NUMBER_LENGTH];  /* 主叫号码 */
    S8        szCalleeNum[SC_TEL_NUMBER_LENGTH];  /* 被叫号码 */
    S8        szANINum[SC_TEL_NUMBER_LENGTH];     /* 被叫号码 */
    S8        szDialNum[SC_TEL_NUMBER_LENGTH];    /* 用户拨号 */
    S8        szSiteNum[SC_TEL_NUMBER_LENGTH];    /* 坐席号码 */
    S8        szUUID[SC_MAX_UUID_LENGTH];         /* Leg-A UUID */

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
    U8         ucPriority;                        /* 任务优先级 */
    U8         ucAudioPlayCnt;                    /* 语言播放次数 */
    U8         bTraceON;                          /* 是否跟踪 */
    U8         bTraceCallON;                      /* 是否跟踪呼叫 */

    pthread_t  pthID;                             /* 线程ID */
    pthread_mutex_t  mutexTaskList;               /* 保护任务队列使用的互斥量 */

    U32        ulTaskID;                          /* 呼叫任务ID */
    U32        ulCustomID;                        /* 呼叫任务所属 */
    U32        ulConcurrency;                     /* 当前并发数 */
    U32        ulAgentQueueID;                    /* 坐席队列编号 */

    U16        usSiteCount;                       /* 坐席数量 */
    U16        usCallerCount;                     /* 当前主叫号码数量 */
    U32        ulLastCalleeIndex;                 /* 用于数据分页 */

    list_t     stCalleeNumQuery;                  /* 被叫号码缓存 refer to struct tagTelNumQueryNode */
    S8         szAudioFileLen[SC_MAX_AUDIO_FILENAME_LEN];  /* 语言文件文件名 */
    SC_CALLER_QUERY_NODE_ST *pstCallerNumQuery;            /* 主叫号码缓存 refer to struct tagTelNumQueryNode */
    SC_TASK_ALLOW_PERIOD_ST astPeriod[SC_MAX_PERIOD_NUM];  /* 任务执行时间段 */

    /* 统计相关 */
    U32        ulTotalCall;                       /* 总呼叫数 */
    U32        ulCallFailed;                      /* 呼叫失败数 */
    U32        ulCallConnected;                   /* 呼叫接通数 */
}SC_TASK_CB_ST;


typedef struct tagTaskCtrlCMD
{
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


typedef struct tagTaskMngtInfo
{
    pthread_t            pthID;                   /* 线程ID */
    pthread_mutex_t      mutexCMDList;            /* 保护命令队列使用的互斥量 */
    pthread_mutex_t      mutexTCBList;            /* 保护任务控制块使用的互斥量 */
    pthread_mutex_t      mutexCallList;           /* 保护呼叫控制块使用的互斥量 */
    pthread_mutex_t      mutexCallHash;           /* 保护呼叫控制块使用的互斥量 */
    pthread_cond_t       condCMDList;             /* 命令队列数据到达通知条件量 */
    U32                  blWaitingExitFlag;       /* 等待退出标示 */

    list_t               stCMDList;               /* 命令队列(节点由HTTP Server创建，有HTTP Server释放) */
    SC_SCB_ST            *pstCallSCBList;         /* 呼叫控制块列表 (需要hash表存储) */
    HASH_TABLE_S         *pstCallSCBHash;         /* 呼叫控制块的hash索引 */
    SC_TASK_CB_ST        *pstTaskList;            /* 任务列表 refer to struct tagTaskCB*/
    U32                  ulTaskCount;             /* 当前正在执行的任务数 */

    U32                  ulMaxCall;               /* 历史最大呼叫并发数 */

    SC_SYS_STATUS_EN     enSystemStatus;          /* 系统状态 */
}SC_TASK_MNGT_ST;


/* declare functions */
SC_SCB_ST *sc_scb_alloc();
VOID sc_scb_free(SC_SCB_ST *pstSCB);
U32 sc_scb_init(SC_SCB_ST *pstSCB);
U32 sc_call_set_owner(SC_SCB_ST *pstSCB, U32  ulTaskID, U32 ulCustomID);
U32 sc_call_set_trunk(SC_SCB_ST *pstSCB, U32 ulTrunkID);
SC_TASK_CB_ST *sc_tcb_find_by_taskid(U32 ulTaskID);
SC_SCB_ST *sc_scb_get(U32 ulIndex);

SC_TASK_CB_ST *sc_tcb_alloc();
VOID sc_tcb_free(SC_TASK_CB_ST *pstTCB);
U32 sc_tcb_init(SC_TASK_CB_ST *pstTCB);
VOID sc_task_set_owner(SC_TASK_CB_ST *pstTCB, U32 ulTaskID, U32 ulCustomID);
VOID sc_task_set_current_call_cnt(SC_TASK_CB_ST *pstTCB, U32 ulCurrentCall);
U32 sc_task_get_current_call_cnt(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_caller(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_callee(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_period(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_agent_info(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_other_info(SC_TASK_CB_ST *pstTCB);
U32 sc_task_update_stat(SC_TASK_CB_ST *pstTCB);
U32 sc_task_check_can_call_by_time(SC_TASK_CB_ST *pstTCB);
U32 sc_task_check_can_call_by_status(SC_TASK_CB_ST *pstTCB);
U32 sc_task_get_call_interval(SC_TASK_CB_ST *pstTCB);
U32 sc_task_set_recall(SC_TASK_CB_ST *pstTCB);
U32 sc_task_cmd_queue_add(SC_TASK_CTRL_CMD_ST *pstCMD);
U32 sc_task_cmd_queue_del(SC_TASK_CTRL_CMD_ST *pstCMD);
U32 sc_task_audio_playcnt(U32 ulTCBNo);
U32 sc_task_get_timeout_for_noanswer(U32 ulTCBNo);
U32 sc_dialer_add_call(SC_SCB_ST *pstSCB);
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
U32 sc_scb_hash_tables_add(S8 *pszUUID, SC_SCB_ST *pstSCB);
U32 sc_scb_hash_tables_delete(S8 *pszUUID);
SC_SCB_ST *sc_scb_hash_tables_find(S8 *pszUUID);
U32 sc_scb_syn_post(S8 *pszUUID);
U32 sc_scb_syn_wait(S8 *pszUUID);

SC_SYS_STATUS_EN sc_check_sys_stat();

SC_SCB_ST *sc_scb_hash_tables_find(S8 *pszUUID);
U32 sc_scb_hash_tables_delete(S8 *pszUUID);
U32 sc_scb_hash_tables_add(S8 *pszUUID, SC_SCB_ST *pstSCB);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SC_TASK_PUB_H__ */

