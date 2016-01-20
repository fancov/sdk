
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

}SC_MSG_TYPE_EN;

typedef struct tagSCDBMsgTag{
    U32     ulMsgType;
    U32     ulSrc;
    U32     ulDst;
    U32     ulRes;
    void    *szData;            /* ��� sql ��䣬����Ϊ NULL */
}SC_DB_MSG_TAG_ST;

typedef struct tagSCDBMsgCallResult{
    SC_DB_MSG_TAG_ST stMsgTag;

    U32             ulCustomerID;               /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAgentID;                  /* ��ϯID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulTaskID;                   /* ����ID,Ҫ��ȫ����,������10λ,���λС��4 */

    S8              szAgentNum[SC_NUM_LENGTH];               /* ��ϯ����(����) */

    S8              szCaller[SC_NUM_LENGTH];                  /* ���к��� */
    S8              szCallee[SC_NUM_LENGTH];                  /* ���к��� */

    U32             ulPDDLen;                   /* ����ʱ��:�ӷ�����е��յ����� */
    U32             ulRingTime;                 /* ����ʱ��,��λ:�� */
    U32             ulAnswerTimeStamp;          /* Ӧ��ʱ��� */
    U32             ulFirstDTMFTime;            /* ��һ�����β���ʱ��,��λ:�� */
    U32             ulIVRFinishTime;            /* IVR�������ʱ��,��λ:�� */
    U32             ulWaitAgentTime;            /* �ȴ���ϯ����ʱ��,��λ:�� */
    U32             ulTimeLen;                  /* ����ʱ��,��λ:�� */
    U32             ulHoldCnt;                  /* ���ִ��� */
    U32             ulHoldTimeLen;              /* ������ʱ��,��λ:�� */

    U16             usTerminateCause;           /* ��ֹԭ�� */
    U8              ucReleasePart;              /* �Ự�ͷŷ� */
    U8              ucRsc;

    U32             ulResult;                   /* ���н�� */
}SC_DB_MSG_CALL_RESULT_ST;




U32 sc_send_msg2db(SC_DB_MSG_TAG_ST *pstMsg);
U32 sc_db_init();
U32 sc_db_start();
U32 sc_db_stop();


#endif /* end of __SC_DB_H__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */
