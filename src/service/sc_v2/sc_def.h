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

#define SC_ACD_GROUP_NAME_LEN         24

#define SC_ACD_HASH_SIZE              128

#define SC_ACD_CALLER_NUM_RELATION_HASH_SIZE    512         /* 主叫号码和坐席对应关系hash的大小 */


/* 单个坐席最大可以属于组的个数 */
#define MAX_GROUP_PER_SITE      2


#define SC_NOBODY_UID                  99
#define SC_NOBODY_GID                  99

#define SC_UUID_HASH_LEN    18

#define SC_BG_JOB_HASH_SIZE 1024
#define SC_UUID_HASH_SIZE   1024

/* 单个坐席最大可以属于组的个数 */
#define SC_MAX_FILELIST_LEN    4096


/**
 * 1. 没有被删除
 * 2. 已经登陆了     && (pstSiteDesc)->bLogin 这个状态不判断了
 * 3. 需要连接，并且处于连接状态
 * 4. 状态为EDL
 */
#define SC_ACD_SITE_IS_USEABLE(pstSiteDesc)                             \
            (DOS_ADDR_VALID(pstSiteDesc)                                \
            && !(pstSiteDesc)->bWaitingDelete                           \
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

    SC_CMD_BUTT,
}SC_SU_COMMANGEN;

/**
 * 业务子层发送给业务控制层的命令
 */
typedef enum tagSCSUEvent{
    SC_EVT_CALL_SETUP,          /**< 呼叫被创建 */
    SC_EVT_EXCHANGE_MEDIA,      /**< 呼叫开始交换媒体 */
    SC_EVT_CALL_RINGING,       /**< 呼叫被应答 */
    SC_EVT_CALL_AMSWERED,       /**< 呼叫被应答 */
    SC_EVT_BRIDGE_START,        /**< 开始桥接开始 */
    SC_EVT_HOLD,                /**< 呼叫保持 */
    SC_EVT_BRIDGE_STOP,         /**< 桥接结束开始 */
    SC_EVT_CALL_RERLEASE,       /**< 呼叫被释放 */
    SC_EVT_CALL_STATUS,         /**< 呼叫状态上报 */
    SC_EVT_DTMF,                /**< 二次拨号 */
    SC_EVT_RECORD_START,        /**< 录音开始 */
    SC_EVT_RECORD_END,          /**< 录音结束 */
    SC_EVT_PLAYBACK_START,      /**< 放音开始 */
    SC_EVT_PLAYBACK_END,        /**< 放音结束 */
    SC_EVT_AUTH_RESULT,         /**< 认证结果 */

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
    SC_LEG_PEER_INTERNAL_INBOUND,     /**< 内部呼叫呼入 */
    SC_LEG_PEER_INTERNAL_OUTBOUND,    /**< 内部呼叫呼出 */

    SC_LEG_PEER_BUTT
}SC_LEG_PEER_TYPE_EN;

/** 本端模式 */
typedef enum tagSCLegLocalType{
    SC_LEG_LOCAL_NORMAL,      /**< 正常类型 */
    SC_LEG_LOCAL_SIGNIN,      /**< 长签 */

    SC_LEG_LOCAL_BUTT
}SC_LEG_LOCAL_TYPE_EN;


/**
 * 业务类型枚举
 *
 * @warning 枚举值必须连续，其不能重复
 */
typedef enum tagSCSrvType{
    SC_SRV_CALL           = 0,   /**< 基本呼叫业务 */
    SC_SRV_PREVIEW_CALL   = 1,   /**< 预览外呼业务 */
    SC_SRV_AUTO_CALL      = 2,   /**< 基本呼叫业务 */
    SC_SRV_VOICE_VERIFY   = 3,   /**< 语音验证码业务 */
    SC_SRV_ACCESS_CODE    = 4,   /**< 接入码业务 */
    SC_SRV_HOLD           = 5,   /**< HOLD业务 */
    SC_SRV_TRANSFER       = 6,   /**< 转接业务 */
    SC_SRV_INCOMING_QUEUE = 7,   /**< 呼入队列业务 */
    SC_SRV_INTERCEPTION   = 8,   /**< 监听业务 */
    SC_SRV_WHISPER        = 9,   /**< 耳语业务 */
    SC_SRV_MARK_CUSTOM    = 10,  /**< 客户标记业务 */

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
    U32        ulSCBNo;

    U8         ucStatus;                          /* 坐席状态 refer to SC_SITE_STATUS_EN */
    U8         ucBindType;                        /* 坐席绑定类型 refer to SC_AGENT_BIND_TYPE_EN */
    U16        usResl;

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
    U32        ucRes1:15;

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
}SC_ACD_AGENT_INFO_ST;


typedef struct tagACDQueueNode{
    U32                     ulID;             /* 当前节点在当前队列里面的编号 */

    SC_ACD_AGENT_INFO_ST   *pstAgentInfo;     /* 坐席信息 */
}SC_ACD_AGENT_QUEUE_NODE_ST;

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
}SC_ACD_GRP_HASH_NODE_ST;

typedef struct tagACDFindSiteParam
{
    U32                  ulPolocy;
    U32                  ulLastSieUsed;
    U32                  ulResult;

    SC_ACD_AGENT_INFO_ST *pstResult;
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

    S8      szRealCallee[SC_NUM_LENGTH];     /**< 号码变换之后的主叫(业务应该使用的) */
    S8      szRealCalling[SC_NUM_LENGTH];    /**< 号码变换之后的主叫(业务应该使用的) */

    S8      szCallee[SC_NUM_LENGTH];         /**< 号码变换之后的主叫(经过号码变换之后的) */
    S8      szCalling[SC_NUM_LENGTH];        /**< 号码变换之后的主叫(经过号码变换之后的) */

    S8      szANI[SC_NUM_LENGTH];            /**< 主叫号码 */
    S8      szCID[SC_NUM_LENGTH];            /**< 来电显示 */

    S8      szDial[SC_NUM_LENGTH];           /**< 来电显示 */

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
    SC_SU_TIME_INFO_ST   stTimeInfo;      /**< 号码信息 */
    SC_SU_NUM_INFO_ST    stNumInfo;     /**< 时间信息 */

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
    SC_CALL_EXEC,       /**< 开始呼叫被叫 */
    SC_CALL_ALERTING,   /**< 被叫开始振铃 */
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

    /* 路由ID */
    U32               ulRouteID;

    /** 主叫坐席指针 */
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentCalling;

    /** 被叫坐席指针 */
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentCallee;
}SC_SRV_CALL_ST;


/** 愉快外呼业务 */
typedef enum tagSCPreviewCallStatus{
    SC_PREVIEW_CALL_IDEL,       /**< 状态初始化 */
    SC_PREVIEW_CALL_PORC,       /**< 发起到坐席的呼叫 */
    SC_PREVIEW_CALL_CALL_SECOND,/**< 呼叫客户 */
    SC_PREVIEW_CALL_CONNECTED,  /**< 客户接通了 */
    SC_PREVIEW_CALL_PROCESS,    /**< 通话结束之后，如果有客户标记，就开始标记，没有直接到释放 */
    SC_PREVIEW_CALL_RELEASE,    /**< 结束 */
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
    SC_AUTO_CALL_IDEL,       /**< 状态初始化 */
    SC_AUTO_CALL_PORC,       /**< 发起呼叫 */
    SC_AUTO_CALL_ACTIVE,     /**< 播放语音 */
    SC_AUTO_CALL_AFTER_KEY,  /**< 按键之后，开始呼叫坐席班组 */
    SC_AUTO_CALL_CONNECTED,  /**< 和坐席开始通话 */
    SC_AUTO_CALL_PROCESS,    /**< 通话结束之后，如果有满意度调查，就需要这个状态，没有直接到释放 */
    SC_AUTO_CALL_RELEASE,    /**< 结束 */
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


/** 语音验证码业务 */
typedef enum tagSCVoiceVerifyStatus{
    SC_VOICE_VERIFY_IDEL,       /**< 状态初始化 */
    SC_VOICE_VERIFY_PORC,       /**< 发起呼叫 */
    SC_VOICE_VERIFY_ACTIVE,     /**< 播放语音 */
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
    SC_INTERCEPTION_PROC,       /**< 呼叫流程 */
    SC_INTERCEPTION_ACTIVE,     /**< 开始监听 */
    SC_INTERCEPTION_RELEASE,    /**< 结束 */
}SC_INTERCEPTION_STATE_EN;

/**
 * 业务控制块, 监听业务
 */
typedef struct tagSCInterception{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /* 呼叫客户的LEG */
    U32               ulLegNo;
}SC_INTERCEPTION_ST;

/** 耳语业务状态 */
typedef enum tagSCWhisperStatus{
    SC_WHISPER_IDEL,       /**< 状态初始化 */
    SC_WHISPER_PROC,       /**< 呼叫流程 */
    SC_WHISPER_ACTIVE,     /**< 呼叫接通开始耳语 */
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

    /* 呼叫客户的LEG */
    U32               ulLegNo;

    /** 接入码缓存 */
    S8                szDialCache[SC_MAX_ACCESS_CODE_LENGTH];

    /* 定时器，超时之后结束呼叫 */
}SC_MARK_CUSTOM_ST;

/** 呼叫转接业务状态 */
typedef enum tagSCTransStatus{
    SC_TRANSFER_IDEL,       /**< 状态初始化 */
    SC_TRANSFER_PROC,       /**< 呼叫转接处理(请求呼叫第三方的流程) */
    SC_TRANSFER_ACTIVE,     /**< 第三方接通 */
    SC_TRANSFER_RELEASE,    /**< 转接业务结束 */
}SC_TRANS_STATE_EN;

/**
 * 业务控制块, 转接业务
 */
typedef struct tagSCCallTransfer{
    /** 基本信息 */
    SC_SCB_TAG_ST     stSCBTag;

    /** 订阅放所在的SCB */
    U32               ulOtherSCBNo;

    /** 发布方LEG，即转接的目的地LEG */
    U32               ulPublishLegNo;
    /** 通知方LEG，即发起转接的LEG */
    U32               ulNotifyLegNo;
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
    SC_SRV_WHISPER_ST    stWhisoered;
    /** 客户标记业务控制块 */
    SC_MARK_CUSTOM_ST    stMarkCustom;
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

    U32     ulPeerType;                      /**< 新LEG的业务类型 */
    U32     ulSCBNo;                         /**< SCB编号 */
    U8      aucCodecList[SC_MAX_CODEC_NUM];  /**< 编解码列表 */
    U32     ulCodecCnt;
    U32     aulTrunkList[SC_MAX_TRUCK_NUM];  /**< 中继列表 */
    U32     ulTrunkCnt;

    S8      szEIXAddr[SC_MAX_IP_LEN];        /**< TT号呼叫时需要EIXIP地址 */
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

    U32     blTone;                          /**< 是否是tone */
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

/** 媒体交换事件 */
typedef struct tagSCMsgEvtMedia{
    SC_MSG_TAG_ST    stMsgTag;              /**< 消息头 */

    /** 业务控制块编号 */
    U32     ulSCBNo;

    /** LEG编号 */
    U32     ulLegNo;
}SC_MSG_EVT_MEDIA_ST;

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
    S32             lBalance;
    /** 会话次数,可用于SMS类业务 */
    U32             ulSessionNum;
    /** 可通话时长/最大数量,单位:秒/条 */
    U32             ulMaxSession;
    /** 是否余额告警 */
    U32              ucBalanceWarning;
}SC_MSG_EVT_AUTH_RESULT_ST;


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

VOID sc_scb_init(SC_SRV_CB *pstSCB);
VOID sc_lcb_playback_init(SC_SU_PLAYBACK_ST *pstPlayback);
SC_SRV_CB *sc_scb_alloc();
VOID sc_scb_free(SC_SRV_CB *pstSCB);
VOID sc_lcb_init(SC_LEG_CB *pstLCB);
SC_LEG_CB *sc_lcb_alloc();
VOID sc_lcb_free(SC_LEG_CB *pstLCB);
SC_SRV_CB *sc_scb_get(U32 ulCBNo);
SC_LEG_CB *sc_lcb_get(U32 ulCBNo);
SC_LEG_CB *sc_lcb_hash_find(S8 *pszUUID);
U32 sc_lcb_hash_add(S8 *pszUUID, SC_LEG_CB *pstLCB);
U32 sc_lcb_hash_delete(S8 *pszUUID);
U32 sc_send_event_call_create(SC_MSG_EVT_CALL_ST *pstEvent);
U32 sc_send_event_err_report(SC_MSG_EVT_ERR_REPORT_ST *pstEvent);
U32 sc_send_event_auth_rsp(SC_MSG_EVT_AUTH_RESULT_ST *pstEvent);
U32 sc_send_event_answer(SC_MSG_EVT_ANSWER_ST *pstEvent);
U32 sc_send_event_release(SC_MSG_EVT_HUNGUP_ST *pstEvent);
U32 sc_send_event_ringing(SC_MSG_EVT_RINGING_ST *pstEvent);
U32 sc_send_event_exchange_media(SC_MSG_EVT_MEDIA_ST *pstEvent);
U32 sc_send_event_hold(SC_MSG_EVT_HOLD_ST *pstEvent);
U32 sc_send_event_dtmf(SC_MSG_EVT_DTMF_ST *pstEvent);
U32 sc_send_event_record(SC_MSG_EVT_RECORD_ST *pstEvent);
U32 sc_send_event_playback(SC_MSG_EVT_PLAYBACK_ST *pstEvent);
U32 sc_send_usr_auth2bs(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
U32 sc_scb_set_service(SC_SRV_CB *pstSCB, U32 ulService);
U32 sc_acd_agent_stat(U32 ulType, SC_ACD_AGENT_INFO_ST *pstAgentInfo, U32 ulAgentID, U32 ulParam);
U32 sc_leg_get_destination(SC_SRV_CB *pstSCB, SC_LEG_CB  *pstLegCB);
U32 sc_leg_get_source(SC_SRV_CB *pstSCB, SC_LEG_CB  *pstLegCB);

BOOL sc_black_list_check(S8 *pszNum);
BOOL sc_sip_extension_check(S8 *pszNum, U32 ulCustomerID);
U32 sc_did_get_custom(S8 *pszNum);
U32 sc_sip_account_get_customer(S8 *pszNum);
SC_ACD_AGENT_QUEUE_NODE_ST *sc_get_agent_by_sip_acc(S8 *szUserID);
SC_ACD_AGENT_QUEUE_NODE_ST *sc_get_agent_by_tt_num(S8 *szTTNumber);
U32 sc_req_hungup(U32 ulSCBNo, U32 ulLegNo, U32 ulErrNo);
U32 sc_req_bridge_call(U32 ulSCBNo, U32 ulCallingLegNo, U32 ulCalleeLegNo);
U32 sc_req_ringback(U32 ulSCBNo, U32 ulLegNo, BOOL blHasMedia);
U32 sc_req_answer_call(U32 ulSCBNo, U32 ulLegNo);
U32 sc_req_play_sound(U32 ulSCBNo, U32 ulLegNo, U32 ulSndInd, U32 ulLoop, U32 ulInterval, U32 ulSilence);
U32 sc_req_play_sounds(U32 ulSCBNo, U32 ulLegNo, U32 *pulSndInd, U32 ulSndCnt, U32 ulLoop, U32 ulInterval, U32 ulSilence);

U32 sc_call_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_exchange_media(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_bridge(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_unbridge(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_req_hungup_with_sound(U32 ulSCBNo, U32 ulLegNo, U32 ulErrNo);
U32 sc_send_cmd_new_call(SC_MSG_TAG_ST *pstMsg);
U32 sc_bgjob_hash_add(U32 ulLegNo, S8 *pszUUID);


#endif  /* end of __SC_DEF_V2_H__ */

#ifdef __cplusplus
}
#endif /* End of __cplusplus */


