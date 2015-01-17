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

#ifndef __BS_STAT_H__
#define __BS_STAT_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */




/* 呼出统计 */
typedef struct
{
    U32             ulTimeStamp;            /* 统计基点时间戳 */
    U32             ulCallCnt;              /* 呼叫总次数 */
    U32             ulRingCnt;              /* 振铃次数 */
    U32             ulBusyCnt;              /* 用户忙次数 */
    U32             ulNotExistCnt;          /* 号码不存在次数 */
    U32             ulNoAnswerCnt;          /* 无应答次数 */
    U32             ulRejectCnt;            /* 拒绝次数 */
    U32             ulEarlyReleaseCnt;      /* 早释次数 */
    U32             ulAnswerCnt;            /* 接听次数 */
    U32             ulPDD;                  /* 接续总时长 */
    U32             ulAnswerTime;           /* 接听总时长 */

}BS_STAT_OUTBAND;


/* 呼入统计 */
typedef struct
{
    U32             ulTimeStamp;            /* 统计基点时间戳 */
    U32             ulCallCnt;              /* 呼叫总次数 */
    U32             ulRingCnt;              /* 振铃次数 */
    U32             ulBusyCnt;              /* 用户忙次数 */
    U32             ulNoAnswerCnt;          /* 无应答次数 */
    U32             ulEarlyReleaseCnt;      /* 早释次数 */
    U32             ulAnswerCnt;            /* 接通次数 */
    U32             ulConnAgentCnt;         /* 呼叫坐席次数 */
    U32             ulAgentAnswerCnt;       /* 坐席接听次数 */
    U32             ulHoldCnt;              /* 保持次数 */
    U32             ulAnswerTime;           /* 接通总时长 */
    U32             ulWaitAgentTime;        /* 等待坐席接听总时长 */
    U32             ulAgentAnswerTime;      /* 坐席接听总时长 */
    U32             ulHoldTime;             /* 保持总时长,单位:毫秒 */

}BS_STAT_INBAND;


/* 外呼统计,几类外呼类型共用 */
typedef struct
{
    U32             ulTimeStamp;            /* 统计基点时间戳 */
    U32             ulCallCnt;              /* 呼叫总次数 */
    U32             ulRingCnt;              /* 振铃次数 */
    U32             ulBusyCnt;              /* 用户忙次数 */
    U32             ulNotExistCnt;          /* 号码不存在次数 */
    U32             ulNoAnswerCnt;          /* 无应答次数 */
    U32             ulRejectCnt;            /* 拒绝次数 */
    U32             ulEarlyReleaseCnt;      /* 早释次数 */
    U32             ulAnswerCnt;            /* 接听次数 */
    U32             ulConnAgentCnt;         /* 呼叫坐席次数 */
    U32             ulAgentAnswerCnt;       /* 坐席接听次数 */
    U32             ulPDD;                  /* 接续总时长 */
    U32             ulAnswerTime;           /* 接通总时长 */
    U32             ulWaitAgentTime;        /* 等待坐席接听总时长 */
    U32             ulAgentAnswerTime;      /* 坐席接听总时长 */

}BS_STAT_OUTDIALING;


/* 信息统计,短信/彩信共用 */
typedef struct
{
    U32             ulTimeStamp;            /* 统计基点时间戳 */
    U32             ulSendCnt;              /* 发送次数 */
    U32             ulRecvCnt;              /* 接收次数 */
    U32             ulSendSucc;             /* 发送成功次数 */
    U32             ulSendFail;             /* 发送失败次数 */
    U32             ulSendLen;              /* 发送总字节数 */
    U32             ulRecvLen;              /* 接收总字节数 */

}BS_STAT_MESSAGE;

typedef struct
{
    U32             ulObjectID;             /* 统计对象ID,要求全数字,不超过10位,最高位小于4 */
    U8              ucObjectType;           /* 统计对象类型,参考:BS_OBJECT_E */
    U8              aucReserv[3];

}BS_STAT_TAG;

/*
业务统计信息
使用说明:
1.每个话单生成后,更新一下统计数据;
2.统计区间固定为1个小时,每个小时将上一个小时统计的数据存入到数据库;
3.偶数小时的统计存放到数组0号下标中,奇数小时的统计数据存放到1号下标中;
4.异常情况:
  当天程序重启,统计信息只能统计到从程序启动以来的数据;
  前一个小时内的话单,但前一小时的统计数据已经入库,也不再更新统计,做丢弃处理;
*/
typedef struct
{
    BS_STAT_TAG             stStatTag;

    BS_STAT_OUTBAND         astOutBand[2];          /* 呼出统计 */
    BS_STAT_INBAND          astInBand[2];           /* 呼入统计 */
    BS_STAT_OUTDIALING      astOutDialing[2];       /* 外呼统计 */
    BS_STAT_MESSAGE         astMS[2];               /* 消息统计 */

}BS_STAT_SERV_ST;

typedef struct
{
    BS_STAT_TAG             stStatTag;

    BS_STAT_OUTBAND         stOutBand;              /* 呼出统计 */

}BS_STAT_OUTBAND_ST;

typedef struct
{
    BS_STAT_TAG             stStatTag;

    BS_STAT_INBAND          stInBand;               /* 呼入统计 */

}BS_STAT_INBAND_ST;

typedef struct
{
    BS_STAT_TAG             stStatTag;

    BS_STAT_OUTDIALING      stOutDialing;           /* 外呼统计 */

}BS_STAT_OUTDIALING_ST;

typedef struct
{
    BS_STAT_TAG             stStatTag;

    BS_STAT_MESSAGE         stMS;                   /* 消息统计 */

}BS_STAT_MESSAGE_ST;



/*
账户账户统计
使用说明:
1.每个话单生成后,更新一下统计数据;
2.在账户结构信息体中存放统计值;
3.统计区间为每天,每天结束后将统计信息存入到数据库;
5.异常情况:
  当天程序重启,统计信息只能统计到从程序启动以来的数据;
*/
typedef struct
{
    BS_STAT_TAG             stStatTag;

    U32     ulTimeStamp;                    /* 统计基点时间戳 */
    S32     lProfit;                        /* 利润,单位:1/100分,毫 */
    S32     lOutBandCallFee;                /* 自身/下级呼出费用,单位:1/100分,毫 */
    S32     lInBandCallFee;                 /* 自身/下级呼入费用,单位:1/100分,毫 */
    S32     lAutoDialingFee;                /* 自身/下级自动外呼费用,单位:1/100分,毫 */
    S32     lPreviewDailingFee;             /* 自身/下级预览外呼费用,单位:1/100分,毫 */
    S32     lPredictiveDailingFee;          /* 自身/下级预测外呼费用,单位:1/100分,毫 */
    S32     lRecordFee;                     /* 自身/下级录音费用,单位:1/100分,毫 */
    S32     lConferenceFee;                 /* 自身/下级会议费用,单位:1/100分,毫 */
    S32     lSmsFee;                        /* 自身/下级短信费用,单位:1/100分,毫 */
    S32     lMmsFee;                        /* 自身/下级彩信费用,单位:1/100分,毫 */
    S32     lRentFee;                       /* 自身/下级租金费用,单位:1/100分,毫 */

}BS_STAT_ACCOUNT_ST;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif



