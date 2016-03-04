/**
 * @file : sc_def.h
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 定义各种数据结构
 *
 *
 * @date: 2016年1月9日
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#ifndef __SC_DEF_V2_H__
#define __SC_DEF_V2_H__

/**
 * 常用术语
 * SU -- 业务单元
 * SC -- 业务控制
 * SC_LEG -- 业务子层相关等译
 */

/* 最大呼叫数量 */
#define SC_MAX_CALL                  3000

/** 最大号码位数 */
#define SC_NUM_LENGTH                32

/** UUID长度 */
#define SC_UUID_LENGTH               44

/** 录音文件长度 */
#define SC_RECORD_FILENAME_LENGTH    128

/** 播放语音文件队列长度 */
#define SC_MAX_PLAY_QUEUE            32

/** 语音验证码最大长度 */
#define SC_VOICE_VERIFY_CODE_LENGTH  8

/** 接入码最大拨号长度 */
#define SC_MAX_ACCESS_CODE_LENGTH    40

/** LEG控制块数量 */
#define SC_LEG_CB_SIZE               6000

/** SCB控制块数量 */
#define SC_SCB_SIZE                  3000

/** 最大编解码个数 */
#define SC_MAX_CODEC_NUM             8

/** 最大中继个数 */
#define SC_MAX_TRUCK_NUM             20

/** 单次放音请求最大放音个数 */
#define SC_MAX_AUDIO_NUM             48

/** IP地址最大长度 */
#define SC_MAX_IP_LEN                24

/** 最大业务个数 */
#define SC_MAX_SERVICE_TYPE          4

#define SC_MAX_DTMF_INTERVAL         3

/* 呼叫进程提示音播放次数 */
#define SC_CALL_HIT_LOOP             3

/* 呼叫进程提示音播放间隔 MS */
#define SC_CALL_HIT_INTERVAL         500

/* 呼叫进程提示音播放间隔 MS */
#define SC_CALL_HIT_SILENCE          300

/* 工号长度最大值 */
#define SC_EMP_NUMBER_LENGTH         12

#define SC_ACD_GROUP_NAME_LEN        24

#define SC_ACD_HASH_SIZE             128

#define SC_ACD_CALLER_NUM_RELATION_HASH_SIZE    512         /* 主叫号码和坐席对应关系hash的大小 */


/* 单个坐席最大可以属于组的个数 */
#define MAX_GROUP_PER_SITE           2


#define SC_NOBODY_UID                99
#define SC_NOBODY_GID                99

#define SC_UUID_HASH_LEN             18

#define SC_BG_JOB_HASH_SIZE          1024
#define SC_UUID_HASH_SIZE            1024

/* 单个坐席最大可以属于组的个数 */
#define SC_MAX_FILELIST_LEN          4096

/* 定义最多有16个客户端连接HTTP服务器 */
#define SC_MAX_HTTP_CLIENT_NUM         16

/* 定义HTTP API中最大参数个数 */
#define SC_API_PARAMS_MAX_NUM          24

/* 定有HTTP请求行中，文件名的最大长度 */
#define SC_HTTP_REQ_MAX_LEN            64

/* 定义HTTP服务器最大个数 */
#define SC_MAX_HTTPD_NUM               1

#define SC_NUM_VERIFY_TIME             3          /* 语音验证码播放次数 */
#define SC_NUM_VERIFY_TIME_MAX         10         /* 语音验证码播放次数 */
#define SC_NUM_VERIFY_TIME_MIN         2          /* 语音验证码播放次数 */

#define SC_MAX_CALL_PRE_SEC            120

#define SC_TASK_AUDIO_PATH             "/home/ipcc/data/audio"

#define SC_RECORD_FILE_PATH            "/home/ipcc/data/voicerecord"

/* 每个任务最多时间段描述节点 */
#define SC_MAX_PERIOD_NUM              4

/* 语言文件名长度 */
#define SC_MAX_AUDIO_FILENAME_LEN      128

/* 呼叫进度同步时间 */
#define SC_TASK_UPDATE_DB_TIMER        2

/* 最大任务数 */
#define SC_MAX_TASK_NUM                3000

/* 单个群呼任务最大并发数 */
#define SC_MAX_TASK_MAX_CONCURRENCY    80

#define SC_ACCESS_CODE_LEN             8

#define SC_DEMOE_TASK_COUNT            3
#define SC_DEMOE_TASK_FILE             "/usr/local/freeswitch/sounds/okcc/CC_demo.wav"

/**
 * 1. 没有被删除
 * 2. 已经登陆了     && (pstSiteDesc)->bLogin 这个状态不判断了
 * 3. 需要连接，并且处于连接状态
 * 4. 状态为EDL
 */
#define SC_ACD_SITE_IS_USEABLE(pstSiteDesc)                             \
            (DOS_ADDR_VALID(pstSiteDesc)                                \
            && !(pstSiteDesc)->bWaitingDelete                           \
            && SC_ACD_WORK_IDEL == (pstSiteDesc)->ucWorkStatus          \
            && SC_ACD_SERV_IDEL == (pstSiteDesc)->ucServStatus          \
            && !(pstSiteDesc)->bSelected)

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
    SC_ACD_WORK_OFFLINE     = 0,             /* 离线 */
    SC_ACD_WORK_IDEL,                        /* 空闲 */
    SC_ACD_WORK_BUSY,                        /* 忙 */
    SC_ACD_WORK_AWAY,                        /* 离开 */

    SC_ACD_WORK_BUTT
};

enum {
    SC_ACD_SERV_IDEL        = 0,              /* 空闲 */
    SC_ACD_SERV_CALL_OUT,                     /* 呼出 */
    SC_ACD_SERV_CALL_IN,                      /* 呼入 */
    SC_ACD_SERV_RINGING,                      /* 振铃 */
    SC_ACD_SERV_RINGBACK,                     /* 回铃 */
    SC_ACD_SERV_PROC,                         /* 整理 */

    SC_ACD_SERV_BUTT
};

/*
* 坐席呼叫状态
* 没有呼叫时就是NONE, 到底是CALLIN还是CALLOUT由当前坐席呼叫的对端决定，
* 如果对端是被动接听电话这是呼出，否则即为呼入
* 预览外呼，群呼任务，呼叫坐席则是呼出
* 客户呼入，接听坐席电话则是呼入
*/
enum {
    SC_ACD_CALL_NONE = 0,  /* 没有呼叫 */
    SC_ACD_CALL_IN,        /* 呼入 */
    SC_ACD_CALL_OUT,       /* 呼出 */

    SC_ACD_CALL_BUTT
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

enum tagAgentStat{
    SC_AGENT_STAT_SELECT,
    SC_AGENT_STAT_CALL,
    SC_AGENT_STAT_CALL_OK,
    SC_AGENT_STAT_CALL_FINISHED,
    SC_AGENT_STAT_CALL_IN,
    SC_AGENT_STAT_CALL_OUT,
    SC_AGENT_STAT_ONLINE,
    SC_AGENT_STAT_OFFLINE,
    SC_AGENT_STAT_SIGNIN,
    SC_AGENT_STAT_SIGNOUT,

    SC_AGENT_STAT_BUTT
};

enum tagSCLegDirection{
    SC_DIRECTION_PSTN,                      /* 呼叫方向来自于PSTN */
    SC_DIRECTION_SIP,                       /* 呼叫方向来自于SIP UA */

    SC_DIRECTION_INVALID                    /* 非法值 */
}SC_LEG_DIRACTION_EN;

typedef enum tagTaskStatus{
    SC_TASK_INIT                       = 0,       /* 任务状态，初始化 */
    SC_TASK_WORKING,                              /* 任务状态，工作 */
    SC_TASK_PAUSED,                               /* 任务状态，暂停 */
    SC_TASK_STOP,                                 /* 任务状态，停止，不再发起呼叫，如果所有呼叫结束即将释放资源 */

    SC_TASK_SYS_BUSY,                             /* 任务状态，系统忙，暂停呼叫量在80%以上的任务发起呼叫 */
    SC_TASK_SYS_ALERT,                            /* 任务状态，系统忙，只允许高优先级客户，并且呼叫量在80%以下的客户发起呼叫 */
    SC_TASK_SYS_EMERG,                            /* 任务状态，系统忙，暂停所有任务 */

    SC_TASK_BUTT                       = 255      /* 任务状态，非法值 */
}SC_TASK_STATUS_EN;

/* 数据库中任务状态 */
typedef enum tagTaskStatusInDB{
    SC_TASK_STATUS_DB_INIT             = 0,/* 新增(未开始) */
    SC_TASK_STATUS_DB_START,               /* 开始 */
    SC_TASK_STATUS_DB_PAUSED,              /* 暂停 */
    SC_TASK_STATUS_DB_STOP,                /* 结束 */

    SC_TASK_STATUS_DB_BUTT
}SC_TASK_STATUS_DB_EN;

typedef enum tagTaskMode{
    SC_TASK_MODE_KEY4AGENT           = 0,         /* 呼叫任务模式，放音，按任意键之后转坐席 */
    SC_TASK_MODE_KEY4AGENT1          = 1,         /* 呼叫任务模式，放音，按特定键0之后转坐席 */
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


typedef enum tagSCRTPPayloadType{
    PT_PCMU            = 0,
    PT_G723            = 4,
    PT_PCMA            = 8,
    PT_G729            = 18,
}SC_RTP_PAYLOAD_TYPE_EN;

/**
 * 业务控制层发送给业务子层的命令集合
 */
typedef enum tagSCSUCommand{
    SC_CMD_CALL,               /**< 请求发起呼叫 */
    SC_CMD_RINGBACK,           /**< 请求播放回铃 */
    SC_CMD_ANSWER_CALL,        /**< 请求接听呼叫 */
    SC_CMD_BRIDGE_CALL,        /**< 请求桥接呼叫 */
    SC_CMD_RELEASE_CALL,       /**< 请求释放呼叫 */
    SC_CMD_PLAYBACK,           /**< 请求放音 */
    SC_CMD_PLAYBACK_STOP,      /**< 请求放音停止 */
    SC_CMD_RECORD,             /**< 请求录音 */
    SC_CMD_RECORD_STOP,        /**< 请求录音结束 */
    SC_CMD_HOLD,               /**< 请求呼叫保持 */
    SC_CMD_UNHOLD,             /**< 请求呼叫解除保持 */
    SC_CMD_IVR_CTRL,           /**< IVR控制命令 */
    SC_CMD_MUX,                /**< 混音命令 */

    SC_CMD_BUTT,
}SC_SU_COMMANGEN;

/**
 * 业务子层发送给业务控制层的命令
 */
typedef enum tagSCSUEvent{
    SC_EVT_CALL_SETUP,          /**< 呼叫被创建 */
    SC_EVT_CALL_RINGING,        /**< 呼叫被应答 */
    SC_EVT_CALL_AMSWERED,       /**< 呼叫被应答 */
    SC_EVT_BRIDGE_START,        /**< 桥接开始 */
    SC_EVT_HOLD,                /**< 呼叫保持 */
    SC_EVT_BRIDGE_STOP,         /**< 桥接结束 */
    SC_EVT_CALL_RERLEASE,       /**< 呼叫被释放 */
    SC_EVT_CALL_STATUS,         /**< 呼叫状态上报 */
    SC_EVT_DTMF,                /**< 二次拨号 */
    SC_EVT_RECORD_START,        /**< 录音开始 */
    SC_EVT_RECORD_END,          /**< 录音结束 */
    SC_EVT_PLAYBACK_START,      /**< 放音开始 */
    SC_EVT_PLAYBACK_END,        /**< 放音结束 */
    SC_EVT_AUTH_RESULT,         /**< 认证结果 */
    SC_EVT_LEACE_CALL_QUEUE,    /**< 出呼叫队列 */

    SC_EVT_ERROR_PORT,          /**< 错误上报事件 */

    SC_EVT_BUTT,
}SC_SU_EVENT_EN;

typedef enum tagSCInterErr{
    SC_ERR_INVALID_MSG,
    SC_ERR_ALLOC_RES_FAIL,
    SC_ERR_EXEC_FAIL,
    SC_ERR_LEG_NOT_EXIST,
    SC_ERR_CALL_FAIL,
    SC_ERR_BRIDGE_FAIL,
    SC_ERR_RECORD_FAIL,
    SC_ERR_BREAK_FAIL,

    SC_ERR_BUTT
}SC_INTER_ERR_EN;


/** 对端类型 */
typedef enum tagSCLegPeerType{
    SC_LEG_PEER_INBOUND,              /**< 呼入 */
    SC_LEG_PEER_OUTBOUND,             /**< 呼出 */
    SC_LEG_PEER_INBOUND_TT,           /**< 对端是TT号 */
    SC_LEG_PEER_OUTBOUND_TT,          /**< 对端是TT号 */
    SC_LEG_PEER_INBOUND_INTERNAL,     /**< 内部呼叫呼入 */
    SC_LEG_PEER_OUTBOUND_INTERNAL,    /**< 内部呼叫呼出 */

    SC_LEG_PEER_BUTT
}SC_LEG_PEER_TYPE_EN;

/** 本端模式 */
typedef enum tagSCLegLocalType{
    SC_LEG_LOCAL_NORMAL,      /**< 正常类型 */
    SC_LEG_LOCAL_SIGNIN,      /**< 长签 */

    SC_LEG_LOCAL_BUTT
}SC_LEG_LOCAL_TYPE_EN;

typedef enum tagCallNumType{
    SC_CALL_NUM_TYPE_NORMOAL              = 0,       /* 号码类型，正常的号码 */
    SC_CALL_NUM_TYPE_EXPR,                           /* 号码类型，正则表达式 */

    SC_CALL_NUM_TYPE_BUTT                 = 255      /* 号码类型，非法值 */
}SC_CALL_NUM_TYPE_EN;

/**
 * 业务类型枚举
 *
 * @warning 枚举值必须连续，其不能重复
 */
typedef enum tagSCSrvType{
    SC_SRV_CALL                 = 0,   /**< 基本呼叫业务 */
    SC_SRV_PREVIEW_CALL         = 1,   /**< 预览外呼业务 */
    SC_SRV_AUTO_CALL            = 2,   /**< 群呼任务业务 */
    SC_SRV_VOICE_VERIFY         = 3,   /**< 语音验证码业务 */
    SC_SRV_ACCESS_CODE          = 4,   /**< 接入码业务 */
    SC_SRV_HOLD                 = 5,   /**< HOLD业务 */
    SC_SRV_TRANSFER             = 6,   /**< 转接业务 */
    SC_SRV_INCOMING_QUEUE       = 7,   /**< 呼入队列业务 */
    SC_SRV_INTERCEPTION         = 8,   /**< 监听业务 */
    SC_SRV_WHISPER              = 9,   /**< 耳语业务 */
    SC_SRV_MARK_CUSTOM          = 10,  /**< 客户标记业务 */
    SC_SRV_AGENT_SIGIN          = 11,  /**< 坐席长签业务 这是个独立业务 */
    SC_SRV_DEMO_TASK            = 12,  /**< 群呼任务demo */

    SC_SRV_BUTT,
}SC_SRV_TYPE_EN;

typedef struct tagSCAgentStat
{
    U32  ulSelectCnt;
    U32  ulCallCnt;      /* 暂时和 ulSelectCnt 保持一致 */
    U32  ulCallConnected;/* 接通的呼叫 */
    U32  ulTotalDuration;/* 接通的呼叫 */

    U32  ulIncomingCall; /* 暂时没有实现 */
    U32  ulOutgoingCall; /* 暂时没有实现 */
    U32  ulTimesSignin;  /* 长签总时间 */
    U32  ulTimesOnline;  /* 在线总时间 */
}SC_AGENT_STAT_ST;

typedef struct tagSCAgentGrpStat
{
    U32  ulCallCnt;
    U32  ulCallinQueue;
    U32  ulTotalWaitingTime;      /* 暂时没有实现 */
    U32  ulTotalWaitingCall;
}SC_AGENT_GRP_STAT_ST;


typedef struct tagACDSiteDesc{
    /* 附加信息，主要存储SCB，主要用于和SCB的交互，需要在SCB释放时检查，并清除 */
    U32        ulLegNo;

    U8         ucWorkStatus;                      /* 坐席工作状态 refer to L200 */
    U8         ucServStatus;                      /* 坐席业务状态 refer to L209 */
    U8         ucBindType;                        /* 坐席绑定类型 refer to SC_AGENT_BIND_TYPE_EN */
    U8         usResl;

    U32        ulAgentID;                          /* 坐席数据库编号 */
    U32        ulCallCnt;                         /* 呼叫总数 */
    U32        ulCustomerID;                      /* 客户id */
    U32        ulSIPUserID;                       /* SIP账户ID */

    U32        aulGroupID[MAX_GROUP_PER_SITE];    /* 组ID */

    U32        ulLastOnlineTime;
    U32        ulLastSignInTime;
    U32        ulLastIdelTime;
    U32        ulLastProcTime;
    U32        ulLastCallTime;

    U32        bValid:1;                          /* 是否可用 */
    U32        bRecord:1;                         /* 是否录音 */
    U32        bTraceON:1;                        /* 是否调试跟踪 */
    U32        bAllowAccompanying:1;              /* 是否允许分机随行 refer to SC_SITE_ACCOM_STATUS_EN */

    U32        bGroupHeader:1;                    /* 是否是组长 */
    U32        bLogin:1;                          /* 是否已经登录 */
    U32        bConnected:1;                      /* 是否已经长连 */
    U32        bNeedConnected:1;                  /* 是否已经长连 */

    U32        bWaitingDelete:1;                  /* 是否已经被删除 */
    U32        bSelected:1;                       /* 是否被呼叫队列选中 */
    U32        bMarkCustomer:1;                   /* 是否标记了客户 */
    U32        bIsInterception:1;                 /* 是否监听 */

    U32        bIsWhisper:1;                      /* 是否耳语 */
    U32        ucRes1:11;

    U8         ucProcesingTime;                   /* 坐席处理呼叫结果时间 */
    U8         ucCallStatus;                      /* 呼叫状态 */
    U32        usRes;

    S8         szUserID[SC_NUM_LENGTH];    /* SIP User ID */
    S8         szExtension[SC_NUM_LENGTH]; /* 分机号 */
    S8         szEmpNo[SC_NUM_LENGTH];     /* 工号 */
    S8         szTelePhone[SC_NUM_LENGTH]; /* 固化号码 */
    S8         szMobile[SC_NUM_LENGTH];    /* 移动电话 */
    S8         szTTNumber[SC_NUM_LENGTH];    /* TT号码 */

    S8         szLastCustomerNum[SC_NUM_LENGTH];    /* 最后一个通话的客户的号码 */

    DOS_TMR_ST htmrLogout;

    pthread_mutex_t  mutexLock;

    SC_AGENT_STAT_ST stStat;
}SC_AGENT_INFO_ST;


typedef struct tagACDQueueNode{
    U32                     ulID;             /* 当前节点在当前队列里面的编号 */

    SC_AGENT_INFO_ST   *pstAgentInfo;     /* 坐席信息 */
}SC_AGENT_NODE_ST;

typedef struct tagACDMemoryNode{
    U32        ulAgentID;                            /* 坐席数据库编号 */

    S8        szCallerNum[SC_NUM_LENGTH];    /* 主叫号码 */
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
    S8        szLastEmpNo[SC_NUM_LENGTH];   /* 最后一个接电话的坐席的工号 */

    DLL_S     stAgentList;                         /* 坐席hash表 */
    pthread_mutex_t  mutexSiteQueue;

    HASH_TABLE_S     *pstRelationList;             /* 主叫号码和坐席的对应关系列表 */

    SC_AGENT_GRP_STAT_ST stStat;
}SC_AGENT_GRP_NODE_ST;

typedef struct tagACDFindSiteParam
{
    U32                  ulPolocy;
    U32                  ulLastSieUsed;
    U32                  ulResult;

    SC_AGENT_INFO_ST *pstResult;
}SC_ACD_FIND_SITE_PARAM_ST;


/**
 * 单个LEG中时间信息
 */
typedef struct tagSCSUTimeInfo{
    U32          ulStartTime;          /**< 开始时间，就是创建时间 */
    U32          ulRingTime;           /**< 开始振铃时间(呼出的LEG是对端开始振铃事件，呼入的LEG是给会180的时间) */
    U32          ulAnswerTime;         /**< 应答时间(呼出的LEG是对端接通的时间，呼入的LEG是发送200OK的时间) */
    U32          ulBridgeTime;         /**< LEG被桥接桥接的时间 */
    U32          ulByeTime;            /**< LEG被挂断时间 */
    U32          ulIVREndTime;         /**< 播放IVR结束时间 */
    U32          ulDTMFStartTime;      /**< 第一个DTMF的时间 */
    U32          ulDTMFLastTime;       /**< 最后一次DTMF的时间 */
    U32          ulRecordStartTime;    /**< 录音开始的时间 */
    U32          ulRecordStopTime;     /**< 录音结束的时间 */

}SC_SU_TIME_INFO_ST;

/**
 * 单个LEG中时间信息
 */
typedef struct tagSCSUNumInfo{
    S8      szOriginalCallee[SC_NUM_LENGTH]; /**< 原始被叫号码(业务发起时的) */
    S8      szOriginalCalling[SC_NUM_LENGTH];/**< 原始主叫号码(业务发起时的) */

    S8      szRealCallee[SC_NUM_LENGTH];     /**< 号码变换之前的主叫(业务应该使用的) */
    S8      szRealCalling[SC_NUM_LENGTH];    /**< 号码变换之前的主叫(业务应该使用的) */

    S8      szCallee[SC_NUM_LENGTH];         /**< 号码变换之后的主叫(经过号码变换之后的) */
    S8      szCalling[SC_NUM_LENGTH];        /**< 号码变换之后的主叫(经过号码变换之后的) */

    S8      szANI[SC_NUM_LENGTH];            /**< 主叫号码 */
    S8      szCID[SC_NUM_LENGTH];            /**< 来电显示 */

    S8      szDial[SC_NUM_LENGTH];           /**< 二次拨号等 */

}SC_SU_NUM_INFO_ST;

/**< 单条LEG的状态  */
typedef enum tagSCSULegStatus{
    SC_LEG_INIT,          /**< 初始化状态 */
    SC_LEG_CALLING,       /**< 正在呼叫状态，对于系统发起的呼叫，专用 */
    SC_LEG_PROC,          /**< 处理状态 */
    SC_LEG_ALERTING,      /**< 整理状态 */
    SC_LEG_ACTIVE,        /**< 激活状态 */
    SC_LEG_RELEASE,       /**< 释放状态 */
}SC_SU_LEG_SATUE_EN;

/**
 * 业务单元,  基本呼叫
 */
typedef struct tagSCSUCall{
    U32                  bValid:1;       /**< 是否已经分配 */
    U32                  bTrace:1;       /**< 是否调试跟踪 */
    U32                  bEarlyMedia:1;  /**< 是否有早期媒体 */
    U32                  bIsTTCall:1;    /**< 是否是TT号呼叫 */
    U32                  bRes:28;

    U8                   ucStatus;       /**< 当前LEG的状态 */
    U8                   ucHoldCnt;      /**< HOLD的次数 */
    U8                   ucPeerType;     /**< 对端类型，标示是呼入，还是呼出 */
    U8                   ucLocalMode;    /**< 本段模式，标示是否长签 */

    U32                  ulHoldTotalTime;/**< 呼叫保持总时长 */
    U32                  ulTrunkID;      /**< 中继ID */
    U32                  ulTrunkCount;   /**< 中继的个数 */
    U32                  ulCause;        /**< 挂断原因 */
    SC_SU_TIME_INFO_ST   stTimeInfo;     /**< 号码信息 */
    SC_SU_NUM_INFO_ST    stNumInfo;      /**< 时间信息 */


    U8      aucCodecList[SC_MAX_CODEC_NUM];  /**< 编解码列表 */
    U32     ulCodecCnt;
    U32     aulTrunkList[SC_MAX_TRUCK_NUM];  /**< 中继列表 */
    U32     ulTrunkCnt;

    S8      szEIXAddr[SC_MAX_IP_LEN];        /**< TT号呼叫时需要EIXIP地址 */

    U32     ulRes;
}SC_SU_CALL_ST;


/**< 单条LEG录音状态  */
typedef enum tagSCSURecordStatus{
    SC_SU_RECORD_INIT,          /**< 初始化状态 */
    SC_SU_RECORD_PROC,          /**< 初始化状态 */
    SC_SU_RECORD_ACTIVE,        /**< 开始录音状态 */
    SC_SU_RECORD_RELEASE,       /**< 录音结束状态 */
}SC_SU_RECORD_SATUE_EN;

/**
 * 业务单元,  录音业务
 */
typedef struct tagSCSURecord{

    U32      bValid:1;   /**< 是否已经分配 */
    U32      bTrace:1;   /**< 是否调试跟踪 */
    U32      bRes:30;
    U16      usStatus;   /**< 当前LEG的状态 */
    U16      usRes;

    /** 录音文件名 */
    S8      szRecordFilename[SC_RECORD_FILENAME_LENGTH];
}SC_SU_RECORD_ST;

/**< 单条LEG放音状态状态  */
typedef enum tagSCSUPlaybackStatus{
    SC_SU_PLAYBACK_INIT,          /**< 初始化状态 */
    SC_SU_PLAYBACK_PROC,          /**< 初始化状态 */
    SC_SU_PLAYBACK_ACTIVE,        /**< 激活状态 */
    SC_SU_PLAYBACK_RELEASE,       /**< 释放状态 */
}SC_SU_PLAYBACK_SATUE_EN;

/**
 * 业务单元,  放音业务
 */
typedef struct tagSCSUPlayback{
    U32      bValid:1;   /**< 是否已经分配 */
    U32      bTrace:1;   /**< 是否调试跟踪 */
    U32      bRes:30;
    U16      usStatus;   /**< 当前LEG的状态 */
    U16      usRes;

    /** 录音文件队列 */
    U32      ulTotal;

    /** 当前录音文件播放序列 */
    U32      ulCurretnIndex;
}SC_SU_PLAYBACK_ST;


/**< 单条LEG桥接状态状态  */
typedef enum tagSCSUBridgeStatus{
    SC_SU_BRIDGE_INIT,          /**< 初始化状态 */
    SC_SU_BRIDGE_PROC,          /**< 处理中 */
    SC_SU_BRIDGE_ACTIVE,        /**< 激活状态 */
    SC_SU_BRIDGE_RELEASE,       /**< 释放状态 */
}SC_SU_BRIDGE_SATUE_EN;

/**
 * 业务单元,  桥接业务
 */
typedef struct tagSCSUBridge{
    U32      bValid:1;      /**< 是否已经分配 */
    U32      bTrace:1;      /**< 是否调试跟踪 */
    U32      bRes:30;

    U16      usStatus;      /**< 当前LEG的状态 */
    U16      usRes;
    U32      ulOtherLEGNo;  /**< 当前录音文件播放序列 */
    U32      ulRes;
}SC_SU_BRIDGE_ST;


/**< 单条LEG HOLD状态状态  */
typedef enum tagSCSUHoldStatus{
    SC_SU_HOLD_INIT,          /**< 初始化状态 */
    SC_SU_HOLD_PROC,          /**< 处理中 */
    SC_SU_HOLD_ACTIVE,        /**< 激活状态 */
    SC_SU_HOLD_RELEASE,       /**< 释放状态 */
}SC_SU_HOLD_SATUE_EN;

/**
 * 业务单元,  呼叫保持业务
 */
typedef struct tagSCSUHold{
    U32      bValid:1;      /**< 是否已经分配 */
    U32      bTrace:1;      /**< 是否调试跟踪 */
    U32      bRes:30;
    U16      usStatus;      /**< 当前LEG的状态 */
    U16      usMode;        /**< HOLD模式，被本端HOLD，还是被对端HOLD */
    U32      ulRes;
}SC_SU_HOLD_ST;


/**< 单条LEG 混音状态状态  */
typedef enum tagSCSUMUXStatus{
    SC_SU_MUX_INIT,          /**< 初始化状态 */
    SC_SU_MUX_ACTIVE,        /**< 激活状态 */
    SC_SU_MUX_RELEASE,       /**< 释放状态 */
}SC_SU_MUX_SATUE_EN;

/**
 * 业务单元,  混音业务
 */
typedef struct tagSCSUMux{
    /** 是否已经分配 */
    U32                   bValid:1;
    /** 是否调试跟踪 */
    U32                   bTrace:1;
    U32                   bRes:30;

    U16      usStatus;                   /**< 当前LEG的状态 */
    U16      usMode;                     /**< 混音模式 */

    U32     ulOtherLegNo;
    U32     ulRes;
}SC_SU_MUX_ST;

/**< 单条LEG 修改连接状态状态  */
typedef enum tagSCSUMCXStatus{
    SC_SU_MCX_INIT,          /**< 初始化状态 */
    SC_SU_MCX_ACTIVE,        /**< 激活状态 */
    SC_SU_MCX_RELEASE,       /**< 释放状态 */
}SC_SU_MCX_SATUE_EN;


/**
 * 业务单元,  修改连接业务
 */
typedef struct tagSCSUMCX{
    U32      bValid:1;  /**< 是否已经分配 */
    U32      bTrace:1;  /**< 是否调试跟踪 */
    U32      bRes:30;
    U16      usStatus;  /**< 当前LEG的状态 */
    U16      usRes;
}SC_SU_MCX_ST;


/**< 单条LEG 修改连接状态状态  */
typedef enum tagSCSUIVRStatus{
    SC_SU_IVR_INIT,          /**< 初始化状态 */
    SC_SU_IVR_ACTIVE,        /**< 激活状态 */
    SC_SU_IVR_RELEASE,       /**< 释放状态 */
}SC_SU_IVR_SATUE_EN;

/**
 * 业务单元,  IVR业务
 */
typedef struct tagSCSUIVR{
    U32      bValid:1;       /**< 是否已经分配 */
    U32      bTrace:1;       /**< 是否调试跟踪 */
    U32      bRes:30;

    U16      usStatus;       /**< 当前LEG的状态 */
    U16      usRes;
    U32      ulIVRIndex;     /**< IVR编号 */
    U32      ulRes;
}SC_SU_IVR_ST;

/**
 * 呼叫LEG控制块
 */
typedef struct tagSCLegCB{
    /** LEG控制块编号 */
    U32                   ulCBNo;
    /** 记录当前LEG管理的业务控制块 */
    U32                   ulSCBNo;
    /** 记录独立业务控制块 */
    U32                   ulIndSCBNo;
    /** 是否已经分配 */
    U32                   bValid:1;
    /** 是否调试跟踪 */
    U32                   bTrace:1;
    U32                   bRes:30;

    /** UUID */
    S8                    szUUID[SC_UUID_LENGTH];

    /** 分配时间 */
    U32                   ulAllocTime;

    /** 原始编解码列表 */
    U8                    aucCodecList[SC_MAX_CODEC_NUM];
    /** 使用的编解码 */
    U8                    ucCodec;
    /** 打包时长 */
    U8                    ucPtime;
    U16                   usRes;

    /** 保证事件被业务控制块处理完后，再被独立业务控制块处理，暂时不需要，现在是一个线程在处理 */
    sem_t                 stEventSem;

    /** 基本呼叫业务单元控制块 */
    SC_SU_CALL_ST         stCall;
    /** 录音业务单元控制块 */
    SC_SU_RECORD_ST       stRecord;
    /** 放音业务单元控制块 */
    SC_SU_PLAYBACK_ST     stPlayback;
    /** 桥接业务单元控制块 */
    SC_SU_BRIDGE_ST       stBridge;
    /** 呼叫保持业务单元控制块 */
    SC_SU_HOLD_ST         stHold;
    /** 混音业务单元控制块 */
    SC_SU_MUX_ST          stMux;
    /** 修改连接业务单元控制块 */
    SC_SU_MCX_ST          stMcx;
    /** IVR业务单元控制块 */
    SC_SU_IVR_ST          stIVR;

}SC_LEG_CB;

/**
 * 业务控制块头
 */
typedef struct tagSCBTag{
    /** 是否被分配 */
    U32                      bValid:1;

    /** 是否需要调试跟踪 */
    U32                      bTrace:1;

    /** 是否已经结束 */
    U32                      bWaitingExit:1;

    U32                      bRes:29;

    /** 业务类型 */
    U16                      usSrvType;
    /** 状态 */
    U16                      usStatus;
}SC_SCB_TAG_ST;


/** 基本呼叫业务状态 */
typedef enum tagSCCallStatus{
    SC_CALL_IDEL,       /**< 状态初始化 */
    SC_CALL_PORC,       /**< 呼叫预处理，确定客户等信息 */
    SC_CALL_AUTH,       /**< 认证 */
    SC_CALL_AUTH2,      /**< 认证被叫 */
    SC_CALL_EXEC,       /**< 开始呼叫被叫 */
    SC_CALL_ALERTING,   /**< 被叫开始振铃 */
    SC_CALL_TONE,       /**< 坐席长签时，给长签的坐席放提示音 */
    SC_CALL_ACTIVE,     /**< 呼叫接通 */
    SC_CALL_PROCESS,    /**< 呼叫之后的处理 */
    SC_CALL_RELEASE,    /**< 结束 */
}SC_CALL_STATE_EN;

/**
 * 业务控制块, 基本呼叫
 */
typedef struct tagSCSrvCall{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /* 第一次创建的LEG编号 */
    U32               ulCallingLegNo;

    /* 关联LEG编号 */
    U32               ulCalleeLegNo;

    /* 呼叫来源 SC_LEG_DIRACTION_EN */
    U32               ulCallSrc;

    /* 呼叫目的 SC_LEG_DIRACTION_EN */
    U32               ulCallDst;

    /* 路由ID */
    U32               ulRouteID;

    /** 主叫坐席指针 */
    SC_AGENT_NODE_ST *pstAgentCalling;

    /** 被叫坐席指针 */
    SC_AGENT_NODE_ST *pstAgentCallee;
}SC_SRV_CALL_ST;


/** 外呼业务状态机定义
 *   AGENT Leg                     SC                         Custom Leg          BS
 *   |                             |     Auth Request         |                    |
 *   |                      (AUTH) | --------------------------------------------> |
 *   |                             |      Auth Rsp            |                    |
 *   |                      (EXEC) | <-------------------------------------------- |
 *   |        Call request         |                          |                    |
 *   | <-------------------------- |                          |
 *   |      Call Setup Event       |                          |
 *   | --------------------------> | (PROC)                   |
 *   |     Call Ringing Event      |                          |
 *   | --------------------------> | (ALERTING)               |
 *   |      Call Answer Event      |                          |
 *   | --------------------------> | (ACTIVE)                 |
 *   |                             |     Call request         |
 *   |                             | -----------------------> |
 *   |                             |     Call Setup Event     |
 *   |                (CONNECTING) | <----------------------- |
 *   |                             |    Call Ringing Event    |
 *   |                 (ALERTING2) | <----------------------- |
 *   |                             |     Call Answer Event    |
 *   |                 (CONNECTED) | <----------------------- |
 *   |                          Talking                       |
 *   | <----------------------------------------------------> |
 *   |                             |       Hungup Event       |
 *   |                   (PROCESS) | <----------------------- |
 *   |      Hungup Event           |                          |
 *   | --------------------------> | (IDEL)                   |
 *   |                             |                          |
 *
 *   如果发起客户那一侧的呼叫失败，可能会有早起媒体过来，需要将两个LEG桥接
 */
typedef enum tagSCPreviewCallStatus{
    SC_PREVIEW_CALL_IDEL,       /**< 状态初始化 */
    SC_PREVIEW_CALL_AUTH,       /**< 状态初始化 */
    SC_PREVIEW_CALL_EXEC,       /**< 状态初始化 */
    SC_PREVIEW_CALL_PORC,       /**< 发起到坐席的呼叫 */
    SC_PREVIEW_CALL_ALERTING,   /**< 坐席在振铃了 */
    SC_PREVIEW_CALL_ACTIVE,     /**< 坐席接通 */
    SC_PREVIEW_CALL_CONNECTING, /**< 呼叫客户 */
    SC_PREVIEW_CALL_ALERTING2,  /**< 客户开始振铃了 */
    SC_PREVIEW_CALL_CONNECTED,  /**< 呼叫接通了 */
    SC_PREVIEW_CALL_PROCESS,    /**< 通话结束之后，如果有客户标记，就开始标记，没有直接到释放 */
    SC_PREVIEW_CALL_RELEASE
}SC_PREVIEW_CALL_STATE_EN;

/**
 * 业务控制块, 预览外呼
 */
typedef struct tagSCPreviewCall{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /** 呼叫坐席的LEG */
    U32               ulCallingLegNo;

    /** 呼叫客户的LEG */
    U32               ulCalleeLegNo;

    /** 坐席ID */
    U32               ulAgentID;
}SC_PREVIEW_CALL_ST;

/** 自动外呼业务 */
typedef enum tagSCAutoCallStatus{
    SC_AUTO_CALL_IDEL,          /**< 状态初始化 */
    SC_AUTO_CALL_AUTH,
    SC_AUTO_CALL_EXEC,
    SC_AUTO_CALL_PORC,          /**< 发起呼叫 */
    SC_AUTO_CALL_ALERTING,
    SC_AUTO_CALL_ACTIVE,        /**< 播放语音 */
    SC_AUTO_CALL_AFTER_KEY,     /**< 按键之后，加入到呼叫队列 */
    SC_AUTO_CALL_AUTH2,         /**< 坐席绑定手机、电话时，呼叫坐席，需要认证 */
    SC_AUTO_CALL_EXEC2,         /**< 呼叫坐席 */
    SC_AUTO_CALL_PORC2,         /**< 坐席的channel创建 */
    SC_AUTO_CALL_ALERTING2,     /**< 坐席振铃 */
    SC_AUTO_CALL_TONE,          /**< 坐席长签时，给坐席放提示音 */
    SC_AUTO_CALL_CONNECTED,     /**< 和坐席开始通话 */
    SC_AUTO_CALL_PROCESS,       /**< 通话结束之后，如果有满意度调查，就需要这个状态，没有直接到释放 */
    SC_AUTO_CALL_RELEASE,       /**< 结束 */

}SC_AUTO_CALL_STATE_EN;

/**
 * 业务控制块, 自动外呼
 */
typedef struct tagSCAUTOCall{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /** 呼叫客户的LEG */
    U32               ulCallingLegNo;

    /** 呼叫坐席的LEG */
    U32               ulCalleeLegNo;

    /** 语音文件ID */
    U32               ulAudioID;

    /** 接通坐席模式 */
    U32               ulKeyMode;

    /** 坐席ID */
    U32               ulAgentID;

    /** 群呼任务ID */
    U32               ulTaskID;
    /** 群呼任务控制块ID */
    U32               ulTcbID;
}SC_AUTO_CALL_ST;


/** 语音验证码业务状态机
* Leg                      SC                       BS
* |                        |         Auth           |
* |                 (AUTH) | ---------------------> |
* |                        |        Auth Rsp        |
* |        Call Req        | <--------------------- |
* | <--------------------- | (EXEC)                 |
* |    Call setup event    |                        |
* | ---------------------> | (PROC)                 |
* |   Call Ringing event   |                        |
* | ---------------------> | (ALERTING)             |
* |   Call Answer event    |                        |
* | ---------------------> | (ACTIVE)               |
* |    Playback Req        |                        |
* | <--------------------- |                        |
* |     Playback stop      |                        |
* | ---------------------> | (RELEASE->INIT)        |
* |                        |                        |
*/
typedef enum tagSCVoiceVerifyStatus{
    SC_VOICE_VERIFY_INIT,       /**< 状态初始化 */
    SC_VOICE_VERIFY_AUTH,       /**< 发起呼叫 */
    SC_VOICE_VERIFY_EXEC,       /**< 播放语音 */
    SC_VOICE_VERIFY_PROC,       /**< 结束 */
    SC_VOICE_VERIFY_ALERTING,   /**< 结束 */
    SC_VOICE_VERIFY_ACTIVE,     /**< 结束 */
    SC_VOICE_VERIFY_RELEASE,    /**< 结束 */
}SC_VOICE_VERIFY_STATE_EN;

/**
 * 业务控制块, 语音验证码
 */
typedef struct tagSCVoiceVerify{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /* 呼叫客户的LEG */
    U32               ulLegNo;
    /* 语音验证码前段提示音ID */
    U32               ulTipsHitNo1;
    /* 语音验证码后段提示音ID */
    U32               ulTipsHitNo2;
    /* 语音验证码 */
    S8                szVerifyCode[SC_VOICE_VERIFY_CODE_LENGTH];
}SC_VOICE_VERIFY_ST;

/** 接入码业务 */
typedef enum tagSCAccessCodeStatus{
    SC_ACCESS_CODE_IDEL,       /**< 状态初始化 */
    SC_ACCESS_CODE_OVERLAP,    /**< 收号 */
    SC_ACCESS_CODE_ACTIVE,     /**< 执行业务 */
    SC_ACCESS_CODE_RELEASE,    /**< 结束 */
}SC_ACCESS_CODE_STATE_EN;


/**
 * 业务控制块, 接入码业务
 */
typedef struct tagSCAccessCode{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /* 呼叫客户的LEG */
    U32               ulLegNo;

    /** 是否是二次拨号 */
    BOOL              bIsSecondDial;

    /** 接入码种类 */
    U32               ulSrvType;

    /** 接入码缓存 */
    S8                szDialCache[SC_MAX_ACCESS_CODE_LENGTH];

    /** 坐席ID */
    U32               ulAgentID;

}SC_ACCESS_CODE_ST;

/** 呼入队列 */
typedef enum tagSCInQueueStatus{
    SC_INQUEUE_IDEL,       /**< 状态初始化 */
    SC_INQUEUE_ACTIVE,     /**< 进入队列成功 */
    SC_INQUEUE_RELEASE,    /**< 结束 */
}SC_IN_QUEUE_STATE_EN;

/**
 * 业务控制块, 呼入队列业务
 */
typedef struct tagSCIncomingQueue{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /* 呼叫客户的LEG */
    U32               ulLegNo;

    /** 队列类型，班组，还是总机业务 */
    U32               ulQueueType;

    /** 进入队列的时间 */
    U32               ulEnqueuTime;

    /** 出队列的时间 */
    U32               ulDequeuTime;
}SC_INCOMING_QUEUE_ST;

/** 监听业务状态 */
typedef enum tagSCInterStatus{
    SC_INTERCEPTION_IDEL,       /**< 状态初始化 */
    SC_INTERCEPTION_AUTH,       /**< 发起呼叫 */
    SC_INTERCEPTION_EXEC,       /**< 呼叫流程 */
    SC_INTERCEPTION_PROC,       /**< 创建leg */
    SC_INTERCEPTION_ALERTING,   /**< 振铃 */
    SC_INTERCEPTION_ACTIVE,     /**< 开始监听 */
    SC_INTERCEPTION_RELEASE,    /**< 结束 */
}SC_INTERCEPTION_STATE_EN;

/**
 * 业务控制块, 监听业务
 */
typedef struct tagSCInterception{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /* 呼叫监听话机的LEG */
    U32               ulLegNo;

    /* 被监听坐席的LEG */
    U32               ulAgentLegNo;

    /* 坐席 */
    SC_AGENT_NODE_ST  *pstAgentInfo;

}SC_INTERCEPTION_ST;

/** 耳语业务状态 */
typedef enum tagSCWhisperStatus{
    SC_WHISPER_IDEL,       /**< 状态初始化 */
    SC_WHISPER_AUTH,       /**< 发起呼叫 */
    SC_WHISPER_EXEC,       /**< 呼叫流程 */
    SC_WHISPER_PROC,       /**< 创建leg */
    SC_WHISPER_ALERTING,   /**< 振铃 */
    SC_WHISPER_ACTIVE,     /**< 开始监听 */
    SC_WHISPER_RELEASE,    /**< 结束 */
}SC_WHISPER_STATE_EN;

/**
 * 业务控制块, 耳语业务
 */
typedef struct tagSCWhisper{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /* 呼叫客户的LEG */
    U32               ulLegNo;

    /* 被监听坐席的LEG */
    U32               ulAgentLegNo;

    /* 坐席 */
    SC_AGENT_NODE_ST  *pstAgentInfo;

}SC_SRV_WHISPER_ST;

/** 客户标记 */
typedef enum tagSCMarkCustomStatus{
    SC_MAKR_CUSTOM_IDEL,       /**< 状态初始化 */
    SC_MAKR_CUSTOM_PROC,       /**< 放音过程 */
    SC_MAKR_CUSTOM_ACTIVE,     /**< 按键标记OK */
    SC_MAKR_CUSTOM_RELEASE,    /**< 释放 */
}SC_MAKR_CUSTOM_STATE_EN;

/**
 * 业务控制块, 客户标记业务
 */
typedef struct tagSCMarkCustom{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /* 呼叫坐席的LEG */
    U32               ulLegNo;

    /* 坐席的信息 */
    SC_AGENT_NODE_ST *pstAgentCall;


    /** 接入码缓存 */
    S8                szDialCache[SC_MAX_ACCESS_CODE_LENGTH];

    /* 定时器，超时之后结束呼叫 */
    DOS_TMR_ST        stTmrHandle;
}SC_MARK_CUSTOM_ST;

/** 呼叫转接业务状态 */
typedef enum tagSCTransStatus{
    SC_TRANSFER_IDEL,       /**< 状态初始化 */
    SC_TRANSFER_AUTH,       /**< 验证转接业务，如果第三方为PSTN，也要一起认证 */
    SC_TRANSFER_EXEC,
    SC_TRANSFER_PROC,       /**< 呼叫转接处理(请求呼叫第三方的流程) */
    SC_TRANSFER_ALERTING,
    SC_TRANSFER_ACTIVE,     /**< 第三方接通 */
    SC_TRANSFER_TRANSFER,   /**< 协商转时，转接呼叫 */
    SC_TRANSFER_FINISHED,   /**< 转接完成 */
    SC_TRANSFER_RELEASE,    /**< 转接业务结束 */
}SC_TRANS_STATE_EN;

typedef enum tagSCTransType{
    SC_TRANSFER_BLIND,      /**< 盲转 */
    SC_TRANSFER_ATTENDED,   /**< 协商转 */
}SC_TRANS_TYPE_EN;

/**
 * 业务控制块, 转接业务
 */
typedef struct tagSCCallTransfer{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /** 转接类型 */
    U32               ulType;

    /** 订阅放所在的SCB */
    U32               ulSubLegNo;

    /** 发布方LEG，即转接的目的地LEG */
    U32               ulPublishLegNo;
    /** 通知方LEG，即发起转接的LEG */
    U32               ulNotifyLegNo;

    /** 要转接的坐席，要修改坐席的状态 */
    U32               ulSubAgentID;
    U32               ulPublishAgentID;
    U32               ulNotifyAgentID;
}SC_CALL_TRANSFER_ST;

/** 呼叫保持业务状态 */
typedef enum tagSCHoldStatus{
    SC_HOLD_IDEL,       /**< 呼叫保持状态初始化 */
    SC_HOLD_PROC,       /**< 呼叫保持处理中(发送了呼叫保持命令之后) */
    SC_HOLD_ACTIVE,     /**< 收到HOLD事件之后 */
    SC_HOLD_RELEASE,    /**< 收到UNHOLD事件之后 */
}SC_HOLD_STATE_EN;

/**
 * 业务控制块, 呼叫保持业务
 */
typedef struct tagSCCallHold{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /* 呼叫客户的LEG */
    U32               ulCallLegNo;
}SC_CALL_HOLD_ST;

/**< 坐席长签业务状态  */
typedef enum tagSCSiginStatus{
    SC_SIGIN_IDEL,          /**< 初始化状态 */
    SC_SIGIN_AUTH,          /**< 初始化状态 */
    SC_SIGIN_EXEC,          /**< 开始呼叫 */
    SC_SIGIN_PORC,          /**<  */
    SC_SIGIN_ALERTING,      /**< 开始振铃 */
    SC_SIGIN_ACTIVE,        /**< 激活状态 */
    SC_SIGIN_RELEASE,       /**< 释放状态 */
}SC_SIGIN_SATUE_EN;

/**
 * 业务单元,  呼叫长签业务
 */
typedef struct tagSCSigin{
     /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /* 呼叫客户的LEG */
    U32               ulLegNo;

    /* 坐席的信息 */
    SC_AGENT_NODE_ST *pstAgentNode;

}SC_SIGIN_ST;

typedef struct tagSCBalanceWarning{
    SC_SCB_TAG_ST     stSCBTag;
}SC_BALANCE_WARNING_ST;

#define SC_SCB_IS_VALID(pstSCB) \
            (DOS_ADDR_VALID((pstSCB)) && (pstSCB)->bValid)

/**
 * 业务控制块
 */
typedef struct tagSCSrvCB{
    /** 记录当前LEG管理的业务控制块 */
    U32                      ulSCBNo;

    /** 是否已经分配 */
    U32                      bValid:1;

    /** 是否调试跟踪 */
    U32                      bTrace:1;
    U32                      bRes:30;

    /** 业务控制块队列 */
    SC_SCB_TAG_ST            *pstServiceList[SC_SRV_BUTT];
    /** 当队列指针 */
    U32                      ulCurrentSrv;

    /** 企业客户ID */
    U32                      ulCustomerID;

    /** 坐席ID */
    U32                      ulAgentID;

    /** 分配时间 */
    U32                      ulAllocTime;

    /** 业务类型 */
    U8                       aucServType[SC_MAX_SERVICE_TYPE];

    /** 基本呼叫业务控制块 */
    SC_SRV_CALL_ST       stCall;
    /** 预览外呼业务控制块 */
    SC_PREVIEW_CALL_ST   stPreviewCall;
    /** 自动外呼业务控制块 */
    SC_AUTO_CALL_ST      stAutoCall;
    /** 语音验证码业务控制块 */
    SC_VOICE_VERIFY_ST   stVoiceVerify;
    /** 接入码业务控制块 */
    SC_ACCESS_CODE_ST    stAccessCode;
    /** 呼叫保持业务控制块 */
    SC_CALL_HOLD_ST      stHold;
    /** 呼叫转接业务控制块 */
    SC_CALL_TRANSFER_ST  stTransfer;
    /** 呼入队列业务控制块 */
    SC_INCOMING_QUEUE_ST stIncomingQueue;
    /** 监听业务控制块 */
    SC_INTERCEPTION_ST   stInterception;
    /** 耳语业务控制块 */
    SC_SRV_WHISPER_ST    stWhispered;
    /** 客户标记业务控制块 */
    SC_MARK_CUSTOM_ST    stMarkCustom;
    /** 坐席长签业务控制块 */
    SC_SIGIN_ST          stSigin;
    /** 群呼任务demo控制块 */
    SC_AUTO_CALL_ST      stDemoTask;
    /** 余额告警业务是否启用 */
    SC_BALANCE_WARNING_ST stBalanceWarning;

}SC_SRV_CB;


/** SC子模块之间通讯的消息头 */
typedef struct tagSCMsgTAG{
    U32   ulMsgType;   /**< 目的 */

    U16   usMsgLen;    /**< 消息总长度 */
    U16   usInterErr;  /**< 内部错误码 */

    U32   ulSCBNo;     /**< 业务控制模块编号 */
}SC_MSG_TAG_ST;

/** 发起呼叫请求 */
typedef struct tagSCMsgCmdCall{
    SC_MSG_TAG_ST      stMsgTag;             /**< 消息头 */

    SC_SU_NUM_INFO_ST  stNumInfo;            /**< 号码信息 */
    U32     ulSCBNo;
    U32     ulLCBNo;                         /**< 新LEG编号 */
}SC_MSG_CMD_CALL_ST;

/** 桥接请求 */
typedef struct tagSCMsgCmdBridge{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    U32     ulSCBNo;                         /**< SCB编号 */
    U32     ulCallingLegNo;                  /**< 主叫LEG */
    U32     ulCalleeLegNo;                   /**< 被叫LEG */
}SC_MSG_CMD_BRIDGE_ST;

/** 应答请求 */
typedef struct tagSCMsgCmdRingback{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    U32     ulSCBNo;                        /**< SCB编号 */
    U32     ulLegNo;                        /**< 主叫LEG */
    U32     ulMediaConnected;               /**< 主叫LEG */
}SC_MSG_CMD_RINGBACK_ST;

/** 应答请求 */
typedef struct tagSCMsgCmdAnswer{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    U32     ulSCBNo;                        /**< SCB编号 */
    U32     ulLegNo;                        /**< 主叫LEG */
}SC_MSG_CMD_ANSWER_ST;

typedef enum tagSCCmdPlaybackType{
    SC_CND_PLAYBACK_TONE,                   /**< 回铃音等 */
    SC_CND_PLAYBACK_SYSTEM,                 /**< 系统语音 */
    SC_CND_PLAYBACK_FILE,                   /**< 指定语音文件名 */

    SC_CND_PLAYBACK_BUTT

}SC_CND_PLAYBACK_TYPE_EN;

/** 放音请求 */
typedef struct tagSCMsgCmdPlayback{
    SC_MSG_TAG_ST    stMsgTag;               /**< 消息头 */
    U32     ulMode;                          /**< 标示是放音。停止放音。停止当前放音 */
    U32     ulSCBNo;                         /**< SCB编号 */
    U32     ulLegNo;                         /**< LEG编号 */
    U32     aulAudioList[SC_MAX_AUDIO_NUM];  /**< 语音文件列表 */
    U32     ulTotalAudioCnt;                 /**< 语音文件个数 */
    U32     ulLoopCnt;                       /**< 是否循环播放，如果是指定循环播放的次数 */
    U32     ulInterval;                      /**< 每次循环时间间隔 ms */
    U32     ulSilence;                       /**< 播放之前Silence的时长 ms */

    U32     blNeedDTMF;                      /**< 是否需要DTMF */

    S8      szAudioFile[SC_MAX_AUDIO_FILENAME_LEN];  /**< 语言文件文件名 */

    U32     enType;                          /**< SC_CND_PLAYBACK_TYPE_EN */
}SC_MSG_CMD_PLAYBACK_ST;

/** 录音请求 */
typedef struct tagSCMsgCmdRecord{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 标示路音开始/结束 */
    U32     ulFlag;

    /** LEG编号 */
    U32     ulLegNo;

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** 录音文件名 */
    S8      szRecordFile[SC_RECORD_FILENAME_LENGTH];
}SC_MSG_CMD_RECORD_ST;

/** 挂断请求 */
typedef struct tagSCMsgCmdHungup{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** LEG编号 */
    U32     ulLegNo;

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** 挂断原因 */
    U32     ulErrCode;
}SC_MSG_CMD_HUNGUP_ST;

typedef enum tagSCHoldFlag{
    SC_HOLD_FLAG_HOLD      = 0,
    SC_HOLD_FLAG_UNHOLD
}SC_CMD_HOLD_FLAG_EN;

/** 呼叫保持请求 */
typedef struct tagSCMsgCmdHold{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 标示 HOLD/UNHOLD */
    U32     ulFlag;

    /** LEG编号 */
    U32     ulLegNo;

    /** 业务控制块编号 */
    U32     ulSCBNo;
}SC_MSG_CMD_HOLD_ST;

/** IRV请求 */
typedef struct tagSCMsgCmdIVR{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 标示 IVR操作类型 */
    U32     ulOpter;

    /** LEG编号 */
    U32     ulLegNo;

    /** 业务控制块编号 */
    U32     ulSCBNo;
}SC_MSG_CMD_IVR_ST;

typedef enum tagScMuxCmdMode{
    SC_MUX_CMD_INTERCEPT,               /**< 监听 */
    SC_MUX_CMD_WHISPER,                 /**< 耳语 */

    SC_MUX_CMD_BUTT
}SC_MUX_CMD_MODE_EN;


/** 混音 请求 */
typedef struct tagSCMsgCmdMux{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 类型。SC_MUX_CMD_MODE_EN */
    U32     ulMode;

    /** LEG编号 */
    U32     ulLegNo;

    /** 监听的坐席的LEG编号 */
    U32     ulAgentLegNo;

    /** 业务控制块编号 */
    U32     ulSCBNo;
}SC_MSG_CMD_MUX_ST;

/** 呼叫建立事件 */
typedef struct tagSCMsgEvtCall{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** LEG编号 */
    U32     ulLegNo;
}SC_MSG_EVT_CALL_ST;

/** 振铃事件事件 */
typedef struct tagSCMsgEvtRinging{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** LEG编号 */
    U32     ulLegNo;

    /** 是否带有媒体 */
    U32     ulWithMedia;
}SC_MSG_EVT_RINGING_ST;

/** 呼叫接听事件 */
typedef struct tagSCMsgEvtAnswer{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** LEG编号 */
    U32     ulLegNo;
}SC_MSG_EVT_ANSWER_ST;

/** 呼叫挂断事件 */
typedef struct tagSCMsgEvtHungup{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** LEG编号 */
    U32     ulLegNo;

    /** 挂断原因 */
    U32     ulErrCode;
}SC_MSG_EVT_HUNGUP_ST;

/** DTMF事件 */
typedef struct tagSCMsgEvtDTMF{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** LEG编号 */
    U32     ulLegNo;

    /** 挂断原因 */
    U32     ulErrCode;

    /** 当前DTMF值(多于1个的存储在LEG信息里面，当前这个也会在LEG中保存) */
    S8      cDTMFVal;
}SC_MSG_EVT_DTMF_ST;

typedef enum tagSCMsgEvtPlayMode{
    SC_MSG_EVT_PLAY_START,
    SC_MSG_EVT_PLAY_STOP,

    SC_MSG_EVT_PLAY_BUTT
}SC_MSG_EVT_PLAY_MODE_EN;

/** 播放语音事件 */
typedef struct tagSCMsgEvtPlayback{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** LEG编号 */
    U32     ulLegNo;

    /** 标示是否是开始、停止 */
    U32     ulFlag;
}SC_MSG_EVT_PLAYBACK_ST;

typedef enum tagSCMsgEvtRecordMode{
    SC_MSG_EVT_RECORD_START,
    SC_MSG_EVT_RECORD_STOP,

    SC_MSG_EVT_RECORD_BUTT
}SC_MSG_EVT_RECORD_MODE_EN;

/** 录音事件 */
typedef struct tagSCMsgEvtRecord{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** LEG编号 */
    U32     ulLegNo;

    /** 标示是否是开始、停止 */
    U32     ulFlag;
}SC_MSG_EVT_RECORD_ST;

/** HOLD事件 */
typedef struct tagSCMsgEvtHold{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** LEG编号 */
    U32     ulLegNo;

    /** 标示是否是HOLD/UNHOLD */
    U32     blIsHold;
}SC_MSG_EVT_HOLD_ST;

/** HOLD事件 */
typedef struct tagSCMsgEvtErrReport{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** 业务控制块编号 */
    U32     ulCMD;

    /** LEG编号 */
    U32     ulLegNo;
}SC_MSG_EVT_ERR_REPORT_ST;

/** 认证结果 */
typedef struct tagSCMsgEvtAuthResult{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** LEG控制块编号 */
    U32     ulLCBNo;

    /** 余额,单位:分 */
    S64             LBalance;
    /** 会话次数,可用于SMS类业务 */
    U32             ulSessionNum;
    /** 可通话时长/最大数量,单位:秒/条 */
    U32             ulMaxSession;
    /** 是否余额告警 */
    U32              ucBalanceWarning;
}SC_MSG_EVT_AUTH_RESULT_ST;

typedef enum tagSCLeaveCallQueReason{
    SC_LEAVE_CALL_QUE_SUCC,
    SC_LEAVE_CALL_QUE_TIMEOUT,

    SC_LEAVE_CALL_QUE_BUTT
}SC_LEAVE_CALL_QUE_REASON_EN;

/** 呼叫队列结果 */
typedef struct tagSCMsgEvtLeaveCallQue{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** 选中的坐席 */
    SC_AGENT_NODE_ST        *pstAgentNode;

}SC_MSG_EVT_LEAVE_CALLQUE_ST;

/**
 * backgroup job管理的hash表节点
 */
typedef struct tagSCBGJobHashNode{
    HASH_NODE_S  stHashNode;    /**< HASH节点 */

    U32 ulLegNo;                /**< LEG编号 */
    S8  szUUID[SC_UUID_LENGTH]; /**< JOB UUID */
}SC_BG_JOB_HASH_NODE_ST;

/* 定义SCBhash表 */
typedef struct tagLCBHashNode
{
    HASH_NODE_S     stNode;                       /* hash链表节点 */

    S8              szUUID[SC_UUID_LENGTH];   /* UUID */
    SC_LEG_CB       *pstLCB;                      /* LCB指针 */
}SC_LCB_HASH_NODE_ST;

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

typedef struct tagCallWaitQueueNode{
    U32                 ulAgentGrpID;                     /* 坐席组ID */
    U32                 ulStartWaitingTime;               /* 开始等待的时间 */

    pthread_mutex_t     mutexCWQMngt;
    DLL_S               stCallWaitingQueue;               /* 呼叫等待队列 refer to SC_SCB_ST */
}SC_CWQ_NODE_ST;

/* 检测一个TCB是有正常的Task和CustomID */
#define SC_TCB_HAS_VALID_OWNER(pstTCB)                        \
    ((pstTCB)                                                 \
    && (pstTCB)->ulTaskID != 0                                \
    && (pstTCB)->ulTaskID != U32_BUTT                         \
    && (pstTCB)->ulCustomID != 0                              \
    && (pstTCB)->ulCustomID != U32_BUTT)

#define SC_TCB_VALID(pstTCB)                                  \
    ((pstTCB)                                                 \
    && (pstTCB)->ulTaskID != 0)

typedef struct tagTelNumQueryNode
{
    list_t     stLink;                            /* 队列链表节点 */

    U32        ulIndex;                           /* 数据库中的ID */

    U8         ucTraceON;                         /* 是否跟踪 */
    U8         ucCalleeType;                      /* 被叫号码类型， refer to enum SC_CALL_NUM_TYPE_EN */
    U8         aucRes[2];

    S8         szNumber[SC_NUM_LENGTH];    /* 号码缓存 */
}SC_TEL_NUM_QUERY_NODE_ST;

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

typedef struct tagTaskCB
{
    U16        usTCBNo;                           /* 编号 */
    U8         ucValid;                           /* 是否被使用 */
    U8         ucTaskStatus;                      /* 任务状态 refer to SC_TASK_STATUS_EN */

    U32        ulAllocTime;
    U32        ulModifyTime;
    U8         ucPriority;                        /* 任务优先级 */
    U8         ucAudioPlayCnt;                    /* 语言播放次数 */
    U8         bTraceON;                          /* 是否跟踪 */
    U8         bTraceCallON;                      /* 是否跟踪呼叫 */
    S8         szTaskName[64];                    /* 任务名称 */

    U8         ucMode;                            /* 任务模式 refer to SC_TASK_MODE_EN*/
    U8         bThreadRunning;                    /* 线程是否在运行 */
    U8         aucRess[2];

    U32        ulTaskID;                          /* 呼叫任务ID */
    U32        ulCustomID;                        /* 呼叫任务所属 */
    U32        ulCurrentConcurrency;              /* 当前并发数 */
    U32        ulMaxConcurrency;                  /* 当前并发数 */
    U32        ulAgentQueueID;                    /* 坐席队列编号 */

    U16        usSiteCount;                       /* 坐席数量 */
    U16        usCallerCount;                     /* 当前主叫号码数量 */
    U32        ulCalleeCount;                     /* 当前被叫号码数量 */
    U32        ulLastCalleeIndex;                 /* 用于数据分页 */
    U32        ulCalleeCountTotal;                /* 呼叫任务总的被叫号码数量 */
    U32        ulCalledCount;                     /* 已经呼叫过的号码数量 */
    U32        ulCalledCountLast;                 /* 上一次同步时的数量已经呼叫过的号码数量 */
    U32        ulCallerGrpID;                     /* 主叫号码组的ID */
    U32        ulCallRate;                        /* 呼叫倍率 */
    list_t     stCalleeNumQuery;                  /* 被叫号码缓存 refer to struct tagTelNumQueryNode */
    S8         szAudioFileLen[SC_MAX_AUDIO_FILENAME_LEN];  /* 语言文件文件名 */
    SC_TASK_ALLOW_PERIOD_ST astPeriod[SC_MAX_PERIOD_NUM];  /* 任务执行时间段 */

    /* 统计相关 */
    U32        ulTotalCall;                       /* 总呼叫数 */
    U32        ulCallFailed;                      /* 呼叫失败数 */
    U32        ulCallConnected;                   /* 呼叫接通数 */

    DOS_TMR_ST pstTmrHandle;                      /* 定时器，更新数据库中，已经呼叫过的号码数量 */

    pthread_t  pthID;                             /* 线程ID */
    pthread_mutex_t  mutexTaskList;               /* 保护任务队列使用的互斥量 */
}SC_TASK_CB;

typedef struct tagSCInconmingCallNode
{
    SC_SRV_CB   *pstSCB;
    S8          szCaller[SC_NUM_LENGTH];
}SC_INCOMING_CALL_NODE_ST;

typedef enum tagAccessCodeType{
    SC_ACCESS_BALANCE_ENQUIRY   = 0,          /**< 余额查询 */
    SC_ACCESS_AGENT_ONLINE      = 1,          /**< 坐席登陆 */
    SC_ACCESS_AGENT_OFFLINE     = 2,          /**< 坐席登出 */
    SC_ACCESS_AGENT_EN_QUEUE    = 3,          /**< 坐席置闲 */
    SC_ACCESS_AGENT_DN_QUEUE    = 4,          /**< 坐席置忙 */
    SC_ACCESS_AGENT_SIGNIN      = 5,          /**< 坐席长签 */
    SC_ACCESS_AGENT_SIGNOUT     = 6,          /**< 坐席退出长签 */
    SC_ACCESS_MARK_CUSTOMER     = 7,          /**< 标记客户 */
    SC_ACCESS_BLIND_TRANSFER    = 8,          /**< 盲转 */
    SC_ACCESS_ATTENDED_TRANSFER = 9,          /**< 协商转 */
    SC_ACCESS_HANGUP_CUSTOMER1  = 10,         /**< 挂断客户的电话1 */
    SC_ACCESS_HANGUP_CUSTOMER2  = 11,         /**< 挂断客户的电话2 */

    SC_ACCESS_BUTT

}SC_ACCESS_CODE_TYPE_EN;

typedef struct tagAccessCodeList
{
    U32  ulType;                                /**< 接入码类型 */
    S8   szCodeFormat[SC_ACCESS_CODE_LEN];      /**< 接入码格式 */
    BOOL bSupportDirectDial;                    /**< 是否支持直接拨号 */
    BOOL bSupportSecondDial;                    /**< 是否支持二次拨号 */
    BOOL bExactMatch;                           /**< 是否是完全匹配 */
    BOOL bValid;                                /**< 是否有效 */
    U32   (*fn_init)(SC_SRV_CB *, SC_LEG_CB *); /**< 接入号处理函数 */
}SC_ACCESS_CODE_LIST_ST;

/** 呼叫统计相关 */
typedef struct tagSysStat{
    U32      ulCRC;

    U32      ulCurrentCalls;       /**< 当前并发量，业务控制块分配的个数 */
    U32      ulIncomingCalls;      /**< 当前呼入量，创建/释放LEG时处理 */
    U32      ulOutgoingCalls;      /**< 当前呼出量，创建/释放LEG时处理 */

    U32      ulTotalTime;          /**< 总时间 */
    U32      ulOutgoingTime;       /**< 呼出 */
    U32      ulIncomingTime;       /**< 呼入 */
    U32      ulAutoCallTime;       /**< 自动呼叫时长 */
    U32      ulPreviewCallTime;    /**< 预览外呼时长 */
    U32      ulPredictiveCallTime; /**< 预测外呼时长 */
    U32      ulInternalCallTime;   /**< 内部呼叫时长 */
}SC_SYS_STAT_ST;


extern SC_SYS_STAT_ST       g_stSysStat;
extern SC_SYS_STAT_ST       g_stSysStatLocal;


VOID sc_scb_init(SC_SRV_CB *pstSCB);
SC_SRV_CB *sc_scb_alloc();
VOID sc_scb_free(SC_SRV_CB *pstSCB);
SC_SRV_CB *sc_scb_get(U32 ulCBNo);
U32 sc_scb_set_service(SC_SRV_CB *pstSCB, U32 ulService);
U32 sc_scb_check_service(SC_SRV_CB *pstSCB, U32 ulService);
U32 sc_scb_remove_service(SC_SRV_CB *pstSCB, U32 ulService);
BOOL sc_scb_is_exit_service(SC_SRV_CB *pstSCB, U32 ulService);

VOID sc_lcb_init(SC_LEG_CB *pstLCB);
SC_LEG_CB *sc_lcb_alloc();
VOID sc_lcb_free(SC_LEG_CB *pstLCB);
SC_LEG_CB *sc_lcb_get(U32 ulCBNo);
SC_LEG_CB *sc_lcb_hash_find(S8 *pszUUID);
U32 sc_lcb_hash_add(S8 *pszUUID, SC_LEG_CB *pstLCB);
U32 sc_lcb_hash_delete(S8 *pszUUID);
U32 sc_leg_get_destination(SC_SRV_CB *pstSCB, SC_LEG_CB  *pstLegCB);
U32 sc_leg_get_source(SC_SRV_CB *pstSCB, SC_LEG_CB  *pstLegCB);

U32 sc_bgjob_hash_add(U32 ulLegNo, S8 *pszUUID);

U32 sc_get_call_limitation();

VOID sc_lcb_playback_init(SC_SU_PLAYBACK_ST *pstPlayback);


U32 sc_send_event_call_create(SC_MSG_EVT_CALL_ST *pstEvent);
U32 sc_send_event_err_report(SC_MSG_EVT_ERR_REPORT_ST *pstEvent);
U32 sc_send_event_auth_rsp(SC_MSG_EVT_AUTH_RESULT_ST *pstEvent);
U32 sc_send_event_answer(SC_MSG_EVT_ANSWER_ST *pstEvent);
U32 sc_send_event_release(SC_MSG_EVT_HUNGUP_ST *pstEvent);
U32 sc_send_event_ringing(SC_MSG_EVT_RINGING_ST *pstEvent);
U32 sc_send_event_hold(SC_MSG_EVT_HOLD_ST *pstEvent);
U32 sc_send_event_dtmf(SC_MSG_EVT_DTMF_ST *pstEvent);
U32 sc_send_event_record(SC_MSG_EVT_RECORD_ST *pstEvent);
U32 sc_send_event_playback(SC_MSG_EVT_PLAYBACK_ST *pstEvent);
U32 sc_send_event_leave_call_queue_rsp(SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvent);
U32 sc_send_usr_auth2bs(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
U32 sc_send_balance_query2bs(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);

U32 sc_req_hungup(U32 ulSCBNo, U32 ulLegNo, U32 ulErrNo);
U32 sc_req_bridge_call(U32 ulSCBNo, U32 ulCallingLegNo, U32 ulCalleeLegNo);
U32 sc_req_ringback(U32 ulSCBNo, U32 ulLegNo, BOOL blHasMedia);
U32 sc_req_answer_call(U32 ulSCBNo, U32 ulLegNo);
U32 sc_req_play_sound(U32 ulSCBNo, U32 ulLegNo, U32 ulSndInd, U32 ulLoop, U32 ulInterval, U32 ulSilence);
U32 sc_req_play_sounds(U32 ulSCBNo, U32 ulLegNo, U32 *pulSndInd, U32 ulSndCnt, U32 ulLoop, U32 ulInterval, U32 ulSilence);
U32 sc_req_playback_stop(U32 ulSCBNo, U32 ulLegNo);
U32 sc_req_hungup_with_sound(U32 ulSCBNo, U32 ulLegNo, U32 ulErrNo);
U32 sc_req_hold(U32 ulSCBNo, U32 ulLegNo, U32 ulFlag);
U32 sc_send_cmd_new_call(SC_MSG_TAG_ST *pstMsg);
U32 sc_send_cmd_playback(SC_MSG_TAG_ST *pstMsg);
U32 sc_send_cmd_mux(SC_MSG_TAG_ST *pstMsg);
U32 sc_send_cmd_record(SC_MSG_TAG_ST *pstMsg);

U32 sc_agent_group_agent_count(U32 ulGroupID);
U32 sc_agent_stat(U32 ulType, SC_AGENT_INFO_ST *pstAgentInfo, U32 ulAgentID, U32 ulParam);
SC_AGENT_NODE_ST *sc_agent_get_by_id(U32 ulAgentID);
SC_AGENT_NODE_ST *sc_agent_get_by_emp_num(U32 ulCustomID, S8 *pszEmpNum);
SC_AGENT_NODE_ST *sc_agent_get_by_sip_acc(S8 *szUserID);
SC_AGENT_NODE_ST *sc_agent_get_by_tt_num(S8 *szTTNumber);
SC_AGENT_NODE_ST *sc_agent_select_by_grpid(U32 ulGroupID, S8 *szCallerNum);
U32 sc_agent_hash_func4grp(U32 ulGrpID, U32 *pulHashIndex);
S32 sc_agent_group_hash_find(VOID *pSymName, HASH_NODE_S *pNode);
U32 sc_agent_group_stat_by_id(U32 ulGroupID, U32 *pulTotal, U32 *pulWorking, U32 *pulIdel, U32 *pulBusy);
U32 sc_agent_init(U32 ulIndex);
U32 sc_agent_group_init(U32 ulIndex);
U32 sc_agent_status_update(U32 ulAction, U32 ulAgentID, U32 ulOperatingType);
U32 sc_agent_http_update_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID);
U32 sc_agent_query_idel(U32 ulAgentGrpID, BOOL *pblResult);
U32 sc_agent_auto_callback(SC_SRV_CB *pstSCB, SC_AGENT_NODE_ST *pstAgentNode);
U32 sc_demo_task_callback(SC_SRV_CB *pstSCB, SC_AGENT_NODE_ST *pstAgentNode);
U32 sc_agent_call_by_id(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB, U32 ulAgentID, U32 *pulErrCode);
U32 sc_agent_call_notify(SC_AGENT_INFO_ST *pstAgentInfo, S8 *szCaller);
U32 sc_agent_serv_status_update(SC_AGENT_INFO_ST *pstAgentQueueInfo, U32 ulServStatus);
U32 sc_agent_update_status_db(SC_AGENT_INFO_ST *pstAgentInfo);

U32 sc_agent_access_set_sigin(SC_AGENT_NODE_ST *pstAgent, SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
void sc_agent_mark_custom_callback(U64 arg);

U32 sc_call_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_bridge(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_unbridge(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_sigin_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_preview_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_voice_verify_proc(U32 ulCustomer, S8 *pszNumber, S8 *pszPassword, U32 ulPlayCnt);
U32 sc_voice_verify_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_voice_verify_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_voice_verify_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_voice_verify_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_voice_verify_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_voice_verify_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_voice_verify_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_interception_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_interception_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_interception_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_interception_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_interception_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_interception_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_whisper_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_whisper_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_whisper_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_whisper_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_whisper_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_whisper_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_auto_call_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_palayback_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_transfer_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_transfer_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_transfer_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_transfer_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_transfer_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_transfer_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_incoming_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_incoming_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_mark_custom_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_mark_custom_playback_start(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_mark_custom_playback_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_mark_custom_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_mark_custom_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_access_code_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_access_code_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_access_code_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_access_code_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_access_code_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_demo_task_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_palayback_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_agent_marker_update_req(U32 ulCustomID, U32 ulAgentID, S32 lKey, S8 *szCallerNum);

SC_TASK_CB *sc_tcb_alloc();
SC_TASK_CB *sc_tcb_find_by_taskid(U32 ulTaskID);
VOID sc_tcb_free(SC_TASK_CB *pstTCB);
VOID sc_task_set_owner(SC_TASK_CB *pstTCB, U32 ulTaskID, U32 ulCustomID);
U32 sc_task_check_can_call(SC_TASK_CB *pstTCB);
U32 sc_task_check_can_call_by_time(SC_TASK_CB *pstTCB);
U32 sc_task_check_can_call_by_status(SC_TASK_CB *pstTCB);
S32 sc_task_and_callee_load(U32 ulIndex);
U32 sc_task_get_mode(U32 ulTCBNo);
U32 sc_task_get_playcnt(U32 ulTCBNo);
S8 *sc_task_get_audio_file(U32 ulTCBNo);
U32 sc_task_get_agent_queue(U32 ulTCBNo);

U32 sc_internal_call_process(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
U32 sc_outgoing_call_process(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB);
U32 sc_incoming_call_proc(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB);
U32 sc_make_call2pstn(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB);
U32 sc_make_call2eix(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB);
U32 sc_make_call2sip(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB);

U32 sc_call_ctrl_call_agent(U32 ulCurrentAgent, SC_AGENT_NODE_ST  *pstAgentNode);
U32 sc_call_ctrl_call_sip(U32 ulAgent, S8 *pszSipNumber);
U32 sc_call_ctrl_call_out(U32 ulAgent, U32 ulTaskID, S8 *pszNumber);
U32 sc_call_ctrl_transfer(U32 ulAgent, U32 ulAgentCalled, BOOL bIsAttend);
U32 sc_call_ctrl_hold(U32 ulAgent, BOOL isHold);
U32 sc_call_ctrl_unhold(U32 ulAgent);
U32 sc_call_ctrl_hangup(U32 ulAgent);
U32 sc_call_ctrl_proc(U32 ulAction, U32 ulTaskID, U32 ulAgent, U32 ulCustomerID, U32 ulType, S8 *pszCallee, U32 ulFlag, U32 ulCalleeAgentID);
U32 sc_demo_task(U32 ulCustomerID, S8 *pszCallee, S8 *pszAgentNum, U32 ulAgentID);
U32 sc_demo_preview(U32 ulCustomerID, S8 *pszCallee, S8 *pszAgentNum, U32 ulAgentID);

U32 sc_did_bind_info_get(S8 *pszDidNum, U32 *pulBindType, U32 *pulBindID);
U32 sc_sip_account_get_by_id(U32 ulSipID, S8 *pszUserID, U32 ulLength);
BOOL sc_customer_is_exit(U32 ulCustomerID);
U32 sc_log_digest_print(const S8 *szTraceStr);
U32 sc_cwq_add_call(SC_SRV_CB *pstSCB, U32 ulAgentGrpID, S8 *szCaller);

U32 sc_access_balance_enquiry(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
U32 sc_access_agent_proc(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
U32 sc_access_transfer(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
U32 sc_access_hungup(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);

U32 sc_send_sip_update_req(U32 ulID, U32 ulAction);
U32 sc_send_gateway_update_req(U32 ulID, U32 ulAction);

#endif  /* end of __SC_DEF_V2_H__ */

#ifdef __cplusplus
}
#endif /* End of __cplusplus */


