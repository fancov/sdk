/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名
 *
 *  创建时间:
 *  作    者:
 *  描    述:
 *  修改历史:
 */

#ifndef __BS_PUB_H__
#define __BS_PUB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "json-c/json.h"


#define BS_MSG_INTERFACE_VERSION        0x101   /* 消息版本号 */
#define BS_MAX_PASSWORD_LEN             16      /* 最大密码长度 */
#define BS_MAX_SESSION_ID_LEN           40      /* 会话ID最大长度 */
#define BS_MAX_CALL_NUM_LEN             24      /* 号码最大长度 */
#define BS_MAX_AGENT_NUM_LEN            8       /* 坐席工号最大长度 */
#define BS_MAX_SERVICE_TYPE_IN_SESSION  3       /* 单个会话中业务类型种类最大个数 */
#define BS_MAX_SESSION_LEG_IN_BILL      2       /* 单个话单中会话leg的最大个数,不得小于2 */
#define BS_MAX_RECORD_FILE_NAME_LEN     64      /* 录音文件名最大长度 */
#define BS_MAX_AGENT_LEVEL              5       /* 代理商最大层级 */
#define BS_MAX_REMARK_LEN               32      /* 备注最大长度 */

#define BS_MAX_RULE_ITEM                20      /* 资费规则的字段数 */



/* 定义错误码 */
enum BS_ERRCODE_E
{
    BS_ERR_SUCC                 = 0,        /* 成功 */
    BS_ERR_NOT_EXIST            = 1,        /* 不存在 */
    BS_ERR_EXPIRE               = 2,        /* 过期/失效 */
    BS_ERR_FROZEN               = 3,        /* 被冻结 */
    BS_ERR_LACK_FEE             = 4,        /* 余额不足 */
    BS_ERR_PASSWORD             = 5,        /* 密码错误 */
    BS_ERR_RESTRICT             = 6,        /* 业务受限 */
    BS_ERR_OVER_LIMIT           = 7,        /* 超过限制 */
    BS_ERR_TIMEOUT              = 8,        /* 超时 */
    BS_ERR_LINK_DOWN            = 9,        /* 连接中断 */
    BS_ERR_SYSTEM               = 10,       /* 系统错误 */
    BS_ERR_MAINTAIN             = 11,       /* 系统维护中 */
    BS_ERR_DATA_ABNORMAL        = 12,       /* 数据异常*/
    BS_ERR_PARAM_ERR            = 13,       /* 参数错误 */
    BS_ERR_NOT_MATCH            = 14,       /* 不匹配 */

    BS_ERR_BUTT                 = 255
};

enum BS_CALL_TERMINATE_CAUSE_E
{
    BS_TERM_NONE                = 0,
    BS_TERM_SCB_LEEK,
    BS_TERM_AUTH_FAIL,
    BS_TERM_INIT_FAIL,
    BS_TERM_SERV_FAIL,
    BS_TERM_COMM_FAIL,
    BS_TERM_SERV_FORBID,
    BS_TERM_INTERNAL_ERR,
    BS_TERM_TASK_PARAM_ERR,
    BS_TERM_NUM_INVALID,
    BS_TERM_SERV_INVALID,
    BS_TERM_CUSTOM_INVALID,
    BS_TERM_QUEUE_INVALID,
    BS_TERM_HANGUP,
    BS_TERM_UNKNOW,

    BS_TERM_BUTT
};

/* 消息类型定义,0为无效值 */
enum BS_MSG_TYPE_E
{
    BS_MSG_HELLO_REQ            = 1,        /* 握手请求 */
    BS_MSG_HELLO_RSP            = 2,        /* 握手响应 */
    BS_MSG_START_UP_NOTIFY      = 3,        /* 启动通知 */

    BS_MSG_BALANCE_QUERY_REQ    = 11,       /* 查询余额 */
    BS_MSG_BALANCE_QUERY_RSP    = 12,       /* 查询余额 */
    BS_MSG_USER_AUTH_REQ        = 13,       /* 用户鉴权 */
    BS_MSG_USER_AUTH_RSP        = 14,       /* 用户鉴权 */
    BS_MSG_ACCOUNT_AUTH_REQ     = 15,       /* 帐号鉴权 */
    BS_MSG_ACCOUNT_AUTH_RSP     = 16,       /* 帐号鉴权 */
    BS_MSG_BILLING_START_REQ    = 17,       /* 开始计费 */
    BS_MSG_BILLING_START_RSP    = 18,       /* 开始计费 */
    BS_MSG_BILLING_UPDATE_REQ   = 19,       /* 中间计费 */
    BS_MSG_BILLING_UPDATE_RSP   = 10,       /* 中间计费 */
    BS_MSG_BILLING_STOP_REQ     = 21,       /* 结束计费 */
    BS_MSG_BILLING_STOP_RSP     = 22,       /* 结束计费 */
    BS_MSG_BILLING_RELEASE_IND  = 23,       /* 释放指示 */
    BS_MSG_BILLING_RELEASE_ACK  = 24,       /* 释放确认 */

    BS_MSG_BUTT                 = 255
};


/* 业务类型定义,0为无效值;最大不超过31个业务类型 */
enum BS_SERV_TYPE_E
{
    BS_SERV_OUTBAND_CALL        = 1,        /* 出局呼叫 */
    BS_SERV_INBAND_CALL         = 2,        /* 入局呼叫 */
    BS_SERV_INTER_CALL          = 3,        /* 内部呼叫 */
    BS_SERV_AUTO_DIALING        = 4,        /* 自动外呼(比例外呼) */
    BS_SERV_PREVIEW_DIALING     = 5,        /* 预览式外呼 */
    BS_SERV_PREDICTIVE_DIALING  = 6,        /* 预测式外呼 */
    BS_SERV_RECORDING           = 7,        /* 录音业务 */
    BS_SERV_CALL_FORWARD        = 8,        /* 呼叫前转 */
    BS_SERV_CALL_TRANSFER       = 9,        /* 呼叫转接 */
    BS_SERV_PICK_UP             = 10,       /* 呼叫代答 */
    BS_SERV_CONFERENCE          = 11,       /* 会议电话 */
    BS_SERV_VOICE_MAIL          = 12,       /* voice mail */
    BS_SERV_SMS_SEND            = 13,       /* 短信发送业务 */
    BS_SERV_SMS_RECV            = 14,       /* 短信接收业务 */
    BS_SERV_MMS_SEND            = 15,       /* 彩信发送业务 */
    BS_SERV_MMS_RECV            = 16,       /* 彩信接收业务 */
    BS_SERV_RENT                = 17,       /* 租金业务 */
    BS_SERV_SETTLE              = 18,       /* 结算业务 */

    BS_SERV_VERIFY              = 19,       /* 语音验证码业务 */

    BS_SERV_BUTT                = 31
};

/* 呼叫释放方定义,0为无效值 */
enum BS_SESSION_RELEASE_PART_E
{
    BS_SESS_RELEASE_BY_CALLER   = 1,        /* 主叫释放 */
    BS_SESS_RELEASE_BY_CALLEE   = 2,        /* 被叫释放 */
    BS_SESS_RELEASE_BY_SYS      = 3,        /* 系统释放 */

    BS_SESS_RELEASE_PART_BUTT   = 255
};

/*
定义消息报文头,多字节字段统一使用网络序
握手消息/握手&计费响应消息/启动通知消息共用本结构
*/
typedef struct
{
    U16     usVersion;          /* 接口协议版本号 */
    U16     usPort;             /* 通讯端口,填写为发送侧监听端口 */
    U32     aulIPAddr[4];       /* IP地址,填写为发送侧IP;兼容IPv6 */
    U32     ulMsgSeq;           /* 消息序列号 */
    U32     ulCRNo;             /* 会话ID */
    U8      ucMsgType;          /* 消息类型 */
    U8      ucErrcode;          /* 错误码 */
    U16     usMsgLen;           /* 消息总长度 */
    U8      aucReserv[40];      /* 保留,将来可能需要加密,可以存放MD5加密摘要 */
}BS_MSG_TAG;


/*
查询消息结构体,请求与响应共用
特别注意:涉及到ID类的整形字段,0为无效值
*/
typedef struct
{
    BS_MSG_TAG      stMsgTag;

    U32             ulUserID;                   /* 用户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulCustomerID;               /* 客户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAccountID;                /* 账户ID,要求全数字,不超过10位,最高位小于4 */
    S32             lBalance;                   /* 余额 */
    U8              ucBalanceWarning;           /* 是否余额告警 */
    U8              ucServType;                 /* 业务类型 */
    U8              aucReserv[32];              /* 保留 */

} BS_MSG_BALANCE_QUERY;


/*
认证消息结构体,用户认证/账号认证共用,请求与响应共用
特别注意:涉及到ID类的整形字段,0为无效值
资费套餐ID:由BS回传给APP,session中只要客户及业务类型未变,APP后续发送的计费消息要携带此字段
*/
typedef struct
{
    BS_MSG_TAG      stMsgTag;

    U32             ulUserID;                   /* 用户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAgentID;                  /* 坐席ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulCustomerID;               /* 客户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAccountID;                /* 账户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulTaskID;                   /* 任务ID,要求全数字,不超过10位,最高位小于4 */
    S32             lBalance;                   /* 余额,单位:分 */
    S8              szPwd[BS_MAX_PASSWORD_LEN]; /* 密码 */
    U32             ulTimeStamp;                /* 时间戳 */
    U32             ulSessionNum;               /* 会话次数,可用于SMS类业务 */
    U32             ulMaxSession;               /* 可通话时长/最大数量,单位:秒/条 */
    S8              szSessionID[BS_MAX_SESSION_ID_LEN];             /* 会话ID */
    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* 主叫号码 */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* 被叫号码 */
    S8              szCID[BS_MAX_CALL_NUM_LEN];                     /* 来显号码 */
    S8              szAgentNum[BS_MAX_AGENT_NUM_LEN];               /* 坐席号码 */

    U8              ucBalanceWarning;           /* 是否余额告警 */
    U8              aucServType[BS_MAX_SERVICE_TYPE_IN_SESSION];    /* 业务类型 */
    U8              aucReserv[32];              /* 保留 */

} BS_MSG_AUTH;

/*
话单SESSION LEG结构体
要求:
1.话单中的号码,使用真实号码,非变换过的号码;
2.租金类业务,在周期类计费中使用,消息中一般不会出现;
*/
typedef struct
{
    U32             ulCDRMark;                  /* 话单标记,BS使用 */
    U32             ulUserID;                   /* 用户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAgentID;                  /* 坐席ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulCustomerID;               /* 客户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAccountID;                /* 账户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulTaskID;                   /* 任务ID,要求全数字,不超过10位,最高位小于4 */
    S8              szRecordFile[BS_MAX_RECORD_FILE_NAME_LEN];      /* 为空代表未录音 */
    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* 主叫号码 */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* 被叫号码 */
    S8              szCID[BS_MAX_CALL_NUM_LEN];                     /* 来显号码 */
    S8              szAgentNum[BS_MAX_AGENT_NUM_LEN];               /* 坐席号码 */
    S8              szSessionID[BS_MAX_SESSION_ID_LEN];             /* 会话ID */
    U32             ulStartTimeStamp;           /* 起始时间戳 */
    U32             ulRingTimeStamp;            /* 振铃时间戳 */
    U32             ulAnswerTimeStamp;          /* 应答时间戳 */
    U32             ulIVRFinishTimeStamp;       /* IVR播放完成时间戳 */
    U32             ulDTMFTimeStamp;            /* (第一个)二次拨号时间戳 */
    U32             ulBridgeTimeStamp;          /* LEG桥接时间戳 */
    U32             ulByeTimeStamp;             /* 释放时间戳 */
    U32             ulHoldCnt;                  /* 保持次数 */
    U32             ulHoldTimeLen;              /* 保持总时长,单位:秒 */
    U32             aulPeerIP[4];               /* 对端IP地址,填写为发送侧IP;兼容IPv6 */
    U16             usPeerTrunkID;              /* 对端中继ID */
    U16             usTerminateCause;           /* 终止原因 */
    U8              ucReleasePart;              /* 会话释放方 */
    U8              ucPayloadType;              /* 媒体类型 */
    U8              ucPacketLossRate;           /* 收包丢包率,0-100 */
    U8              aucServType[BS_MAX_SERVICE_TYPE_IN_SESSION];    /* 业务类型 */
    U8              aucReserv[2];               /* 保留 */

}BS_BILL_SESSION_LEG;


/*
话单消息结构体
特别注意:涉及到ID类的整形字段,0为无效值
要求:
1.SESSION LEG数组,有效LEG在前,无效LEG在后,也即,一旦LEG是无效的,后面均无效;
2.呼出类业务以目的所在LEG为主LEG,呼入类业务以源所在LEG为主LEG;内部呼叫以被叫所在LEG为主LEG;
3.呼叫过程中可能因为主被叫转接呼叫导致业务不明晰,此时,最开始呼叫业务主LEG未转移的则保持,否则转移到新LEG上;
4.主LEG存放在话单消息中的第一个LEG中,是最主要的计费参考依据;
5.业务类型数组,如果存在多个,则依据业务发生时间先后依次填写;如果同时发生,则辅助类业务存放在后面(例如录音,转接等);
6.出局呼叫/内部呼叫,话单记录以被叫所在leg为主;e.g.
  a.A呼叫B,话单采用LEG B信息(主LEG);
  b.A呼叫B,A将呼叫转接到C,并且过程中A挂机,那么A挂机时产生A-C的内部呼叫话单(A与B的呼叫过程包括在LEG B中);
    B/C挂机时产生2个LEG, B LEG出局, C LEG转接,分别生成2张话单,B LEG话单主叫依然为A;
    如果C为PSTN号码,那么C LEG还包含一个出局呼叫业务,对应生成一张出局话单;
    如果A转接到C,但C先挂机,A与B恢复通话,先生成一个B与C的呼叫话单,业务类型为内部呼叫,A/B与情况a类似;
7.入局呼叫,话单记录以主叫所在leg为主;e.g.
  a.A呼叫B,话单采用leg A信息(主LEG);
  b.A呼叫B,B将呼叫前转C,那么产生LEG A的呼入话单及B-C的前转呼叫话单;
  b.A呼叫B,B将呼叫转接到C,并且过程中B挂机,那么B挂机时产生B-C的内部呼叫话单(B与A的呼叫过程包括在LEG A中);;
    A/C挂机时产生2个LEG, A LEG入局, C LEG转接,分别生成2张话单,A LEG话单被叫依然为B;
    如果C为PSTN号码,那么C LEG还包含一个出局呼叫业务,对应生成一张出局话单;
    如果B转接到C但C先挂机,B与A恢复通话,先生成一个B与C的呼叫话单,业务类型为内部呼叫,A/B与情况a类似;
8.外呼,话单记录结合第一个和第二个session leg;
  比例外呼/预测外呼,第一个leg对应PSTN用户,第二个leg对应坐席;
  预览外呼,第一个leg对应坐席(业务类型:内部呼叫/出局呼叫),第二个leg对应PSTN被叫;
  特别的,外呼业务,转接呼叫时,对于转接方,需要生成外呼方与转接方的内部呼叫话单;e.g.
  外呼A,分配坐席B,B转接到C,B转接后挂机,除生成B到C的内部呼叫话单外,
  还要生成A到B的内部呼叫话单(主叫使用特定号码,CID为A号码,呼叫类型为内部呼叫),用于记录转接过程;
9.录音业务作为辅助业务,一般与其它业务类型结合出现;
  同一个录音,要求在所有LEG中只能出现一次;
  呼叫未结束,录音已结束的情况下,可先发送录音话单;后续的话单中不得再包含原录音业务,防止重复计费;
10.呼叫前传/呼叫转接,只有目的号码所在的leg中才有此业务,其它leg不应出现该业务;主叫为转接方,CID为转接后对方号码;
11.呼叫代答,只有代答方所在leg有此业务,其它leg不应出现该业务;注意:不要使其出现在主LEG中;
12.会议电话,每一方电话均单独生成一个话单;
13.voice mail,呼入留言,在第二个leg中出现;用户获取留言,在第一个leg中出现;
14.信息类业务,只需要一个LEG即可;
*/
typedef struct
{
    BS_MSG_TAG      stMsgTag;

    /* 会话LEG,要求业务方面,任何一个会话LEG释放,均要生成一个话单 */
    BS_BILL_SESSION_LEG astSessionLeg[BS_MAX_SESSION_LEG_IN_BILL];
    U8              ucLegNum;                   /* LEG数量 */
    U8              aucReserv[31];              /* 保留 */

} BS_MSG_CDR;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif


