
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifndef __SC_DB_H__
#define __SC_DB_H__


typedef enum  tagSCMsgType{
    SC_MSG_SAVE_CALL_RESULT,
    SC_MSG_SAVE_AGENT_STATUS,
    SC_MSG_SAVE_TASK_CALLED_COUNT,
    SC_MSG_SAVE_SIP_IPADDR,
    SC_MSG_SAVE_TRUNK_STATUS,
    SC_MSG_SAVE_STAT_LIMIT_CALLER,
    SC_MSG_SACE_TASK_STATUS

}SC_MSG_TYPE_EN;

typedef struct tagSCDBMsgTag{
    U32     ulMsgType;
    U32     ulSrc;
    U32     ulDst;
    U32     ulRes;
    void    *szData;            /* 存放 sql 语句，可以为 NULL */
}SC_DB_MSG_TAG_ST;

typedef struct tagSCDBMsgCallResult{
    SC_DB_MSG_TAG_ST stMsgTag;

    U32             ulCustomerID;               /* 客户ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulAgentID;                  /* 坐席ID,要求全数字,不超过10位,最高位小于4 */
    U32             ulTaskID;                   /* 任务ID,要求全数字,不超过10位,最高位小于4 */

    S8              szAgentNum[SC_NUM_LENGTH];               /* 坐席号码(工号) */

    S8              szCaller[SC_NUM_LENGTH];                  /* 主叫号码 */
    S8              szCallee[SC_NUM_LENGTH];                  /* 被叫号码 */

    U32             ulPDDLen;                   /* 接续时长:从发起呼叫到收到振铃 */
    U32             ulStartTime;                /* 开始时间,单位:秒 */
    U32             ulRingTime;                 /* 振铃时长,单位:秒 */
    U32             ulAnswerTimeStamp;          /* 应答时间戳 */
    U32             ulFirstDTMFTime;            /* 第一个二次拨号时间,单位:秒 */
    U32             ulIVRFinishTime;            /* IVR放音完成时间,单位:秒 */
    U32             ulWaitAgentTime;            /* 等待坐席接听时间,单位:秒 */
    U32             ulTimeLen;                  /* 呼叫时长,单位:秒 */
    U32             ulHoldCnt;                  /* 保持次数 */
    U32             ulHoldTimeLen;              /* 保持总时长,单位:秒 */

    U16             usTerminateCause;           /* 终止原因 */
    U8              ucReleasePart;              /* 会话释放方 */
    U8              ucRsc;

    U32             ulResult;                   /* 呼叫结果 */
}SC_DB_MSG_CALL_RESULT_ST;

typedef struct tagSCDBMsgTaskStatus{
    SC_DB_MSG_TAG_ST stMsgTag;

    U32             ulTaskID;

    U32             ulTotalAgent;
    U32             ulIdleAgent;

    U32             ulCurrentConcurrency;              /* 当前并发数 */
    U32             ulMaxConcurrency;                  /* 当前并发数 */

    U32             ulCalledCount;
}SC_DB_MSG_TASK_STATUS_ST;

typedef struct tagSCDBMsgAgentStatus{
    SC_DB_MSG_TAG_ST stMsgTag;

    U32             ulAgentID;

    U32             ulWorkStatus;
    U32             ulServStatus;

    BOOL            bIsSigin;
    BOOL            bIsInterception;
    BOOL            bIsWhisper;

    S8              szEmpNo[SC_NUM_LENGTH];     /* 工号 */

}SC_DB_MSG_AGENT_STATUS_ST;


U32 sc_send_msg2db(SC_DB_MSG_TAG_ST *pstMsg);
U32 sc_db_init();
U32 sc_db_start();
U32 sc_db_stop();


#endif /* end of __SC_DB_H__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

