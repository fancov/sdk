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

#ifndef __BS_CDR_H__
#define __BS_CDR_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* 话单类型 */
enum BS_CDR_TYPE_E
{
    BS_CDR_ORIGINAL                 = 0,        /* 原始话单 */
    BS_CDR_VOICE                    = 1,        /* 语音话单 */
    BS_CDR_RECORDING                = 2,        /* 录音话单 */
    BS_CDR_MESSAGE                  = 3,        /* 信息话单 */
    BS_CDR_SETTLE                   = 4,        /* 结算话单 */
    BS_CDR_RENT                     = 5,        /* 租金话单 */
    BS_CDR_ACCOUNT                  = 6,        /* 账务话单 */


    BS_CDR_TYPE_BUTT                = 255
};


/* 话单TAG */
typedef struct
{
    U32             ulCDRMark;                  /* 话单标记,同一原始话单拆分的基础话单标记相同 */
    U32             ulAccountMark;              /* 正对统一客户来说，同一次出账关联的CDR中Account Mark是相同的 */
    U8              ucCDRType;                  /* 话单类型 */
    U8              aucReserv[3];
}BS_CDR_TAG;

/*
语音话单
使用说明:
1.呼入/呼出/内部呼叫直接使用;
2.呼叫转移/前转类,对转移目的方生成话单,注意填写配套业务类型;
3.会议类,每个LEG生成话单;
4.因业务的不同,可能同一个呼叫,会生成多张基础话单;
5.对于外呼类业务:
  被叫号码填写外呼号码;
  外呼类业务,可能生成2张话单,一张对应外呼号码(业务类型为外呼),一张对应坐席的PSTN号码(出局呼叫);
*/
typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulUserID;                   /* 用户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAgentID;                  /* 坐席ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulCustomerID;               /* 客户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAccountID;                /* 账户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulTaskID;                   /* 任务ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulRuleID;                   /* 使用的计费规则 */

    U32             aulFee[BS_MAX_AGENT_LEVEL];                     /* 每个层级的费用,单位:1/100分,毫 */
    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* 主叫号码 */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* 被叫号码 */
    S8              szCID[BS_MAX_CALL_NUM_LEN];                     /* 来显号码 */
    S8              szAgentNum[BS_MAX_AGENT_NUM_LEN];               /* 坐席号码(工号) */
    S8              szRecordFile[BS_MAX_RECORD_FILE_NAME_LEN];      /* 为空代表未录音 */
    U32             ulPDDLen;                   /* 接续时长:从发起呼叫到收到振铃 */
    U32             ulRingTime;                 /* 振铃时长,单位:秒 */
    U32             ulAnswerTimeStamp;          /* 应答时间戳 */
    U32             ulDTMFTime;                 /* 第一个二次拨号时间,单位:秒 */
    U32             ulIVRFinishTime;            /* IVR放音完成时间,单位:秒 */
    U32             ulWaitAgentTime;            /* 等待坐席接听时间,单位:秒 */
    U32             ulTimeLen;                  /* 呼叫时长,单位:秒 */
    U32             ulHoldCnt;                  /* 保持次数 */
    U32             ulHoldTimeLen;              /* 保持总时长,单位:秒 */
    U32             aulPeerIP[4];               /* 对端IP地址,填写为发送侧IP;兼容IPv6 */
    U16             usPeerTrunkID;              /* 对端中继ID */
    U16             usTerminateCause;           /* 终止原因 */

    U8              ucServType;                 /* 业务类型 */
    U8              ucAgentLevel;               /* 所属层级,[0,BS_MAX_AGENT_LEVEL) */
    U8              ucRecordFlag;               /* 是否录音标记 */
    U8              ucReleasePart;              /* 会话释放方 */

    U8              ucPayloadType;              /* 媒体类型 */
    U8              ucPacketLossRate;           /* 收包丢包率,0-100 */
    U8              ucNeedCharge;               /* 是否需要计费 */
    U8              ucRes;                      /* 保留 */
}BS_CDR_VOICE_ST;

typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulUserID;                   /* 用户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAgentID;                  /* 坐席ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulCustomerID;               /* 客户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAccountID;                /* 账户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulTaskID;                   /* 任务ID,要求全数字,不超过10位,最高位小于4 */

    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* 主叫号码 */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* 被叫号码 */
    S8              szCID[BS_MAX_CALL_NUM_LEN];                     /* 来显号码 */
    S8              szAgentNum[BS_MAX_AGENT_NUM_LEN];               /* 坐席号码(工号) */
    S8              szRecordFile[BS_MAX_RECORD_FILE_NAME_LEN];      /* 为空代表未录音 */
    U32             ulPDDLen;                   /* 接续时长:从发起呼叫到收到振铃 */
    U32             ulRingTime;                 /* 振铃时长,单位:秒 */
    U32             ulAnswerTimeStamp;          /* 应答时间戳 */
    U32             ulDTMFTime;                 /* 第一个二次拨号时间,单位:秒 */
    U32             ulIVRFinishTime;            /* IVR放音完成时间,单位:秒 */
    U32             ulWaitAgentTime;            /* 等待坐席接听时间,单位:秒 */
    U32             ulTimeLen;                  /* 呼叫时长,单位:秒 */
    U32             ulHoldCnt;                  /* 保持次数 */
    U32             ulHoldTimeLen;              /* 保持总时长,单位:秒 */
    U16             usTerminateCause;           /* 终止原因 */
    U8              ucServType;                 /* 业务类型 */
    U8              ucReleasePart;              /* 会话释放方 */

    U32             ulResult;                   /* 呼叫结果 */
}BS_CDR_CALLTASK_RESULT_ST;


/* 录音话单 */
typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulUserID;                   /* 用户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAgentID;                  /* 坐席ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulCustomerID;               /* 客户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAccountID;                /* 账户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulTaskID;                   /* 任务ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulRuleID;                   /* 使用的计费规则 */

    U32             aulFee[BS_MAX_AGENT_LEVEL];                     /* 每个层级的费用,单位:1/100分,毫 */
    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* 主叫号码 */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* 被叫号码 */
    S8              szCID[BS_MAX_CALL_NUM_LEN];                     /* 来显号码 */
    S8              szAgentNum[BS_MAX_AGENT_NUM_LEN];               /* 坐席号码(工号) */
    S8              szRecordFile[BS_MAX_RECORD_FILE_NAME_LEN];      /* 为空代表未录音 */
    U32             ulRecordTimeStamp;          /* 录音开始时间戳 */
    U32             ulTimeLen;                  /* 录音时长,单位:秒 */
    U8              ucAgentLevel;               /* 所属层级,[0,BS_MAX_AGENT_LEVEL) */

}BS_CDR_RECORDING_ST;

/* 短信/彩信话单 */
typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulUserID;                   /* 用户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAgentID;                  /* 坐席ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulCustomerID;               /* 客户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAccountID;                /* 账户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulTaskID;                   /* 任务ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulRuleID;                   /* 使用的计费规则 */

    U32             aulFee[BS_MAX_AGENT_LEVEL];                     /* 每个层级的费用,单位:1/100分,毫 */
    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* 主叫号码 */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* 被叫号码 */
    S8              szAgentNum[BS_MAX_AGENT_NUM_LEN];               /* 坐席号码 */
    U32             ulTimeStamp;                /* 发送/接收时间戳 */
    U32             ulArrivedTimeStamp;         /* 到达时间戳 */
    U32             ulLen;                      /* 信息长度,单位:字节 */
    U32             aulPeerIP[4];               /* 对端IP地址,填写为发送侧IP;兼容IPv6 */
    U16             usPeerTrunkID;              /* 对端中继ID */
    U16             usTerminateCause;           /* 终止原因 */
    U8              ucServType;                 /* 业务类型 */
    U8              ucAgentLevel;               /* 所属层级,[0,BS_MAX_AGENT_LEVEL) */

}BS_CDR_MS_ST;

/* 租金话单 */
typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulCustomerID;               /* 客户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAccountID;                /* 账户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulRuleID;                   /* 使用的计费规则 */

    U32             aulFee[BS_MAX_AGENT_LEVEL];                     /* 每个层级的费用,单位:1/100分,毫 */
    U32             ulTimeStamp;                /* 时间戳 */
    U8              ucAttrType;                 /* 租金属性类型,参考BS_BILLING_ATTRIBUTE_E */
    U8              ucAgentLevel;               /* 所属层级,[0,BS_MAX_AGENT_LEVEL) */

}BS_CDR_RENT_ST;

/* 结算话单 */
typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulSPID;                     /* 服务提供商ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulRuleID;                   /* 使用的计费规则 */

    U32             ulFee;                      /* 费用,单位:1/100分,毫 */
    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* 主叫号码 */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* 被叫号码 */
    U32             ulTimeStamp;                /* 时间戳 */
    U32             ulLen;                      /* 呼叫时长或信息长度,单位:秒/字节 */
    U32             aulPeerIP[4];               /* 对端IP地址,填写为发送侧IP;兼容IPv6 */
    U16             usPeerTrunkID;              /* 对端中继ID */
    U16             usTerminateCause;           /* 终止原因 */
    U8              ucServType;                 /* 业务类型 */

}BS_CDR_SETTLE_ST;

/* 账户记录 */
typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulCustomerID;               /* 客户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAccountID;                /* 账户ID,要求全数字,不超过10位,最高位小于4 */
    U8              ucOperateType;              /* 操作类型,参考:enBS_ACCOUNT_OPERATE_TYPE_E */
    S32             lMoney;                     /* 金额,单位:1/100分,毫 */
    S64             LBalance;                   /* 余额,单位:1/100分,毫 */
    U32             ulPeeAccount;               /* 对端账户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulTimeStamp;                /* 时间戳 */
    U32             ulOperatorID;               /* 操作员 */
    U32             ulOperateDir;               /* 方向 */
    S8              szRemark[BS_MAX_REMARK_LEN];                  /* 备注 */

}BS_CDR_ACCOUNT_ST;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif


