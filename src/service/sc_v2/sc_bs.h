#ifndef __SC_BS_PUB_H__
#define __SC_BS_PUB_H__

/* BS客户端个数 */
#define SC_MAX_BS_CLIENT     1

/* 心跳定时器相关处理 */
#define SC_BS_HB_TIMEOUT     3000
#define SC_BS_HB_INTERVAL    4000
#define SC_BS_HB_FAIL_CNT    5

/* 业务定时器相关定义 */
#define SC_BS_SEND_INTERVAL  1500
#define SC_BS_SEND_TIMEOUT   1000
#define SC_BS_SEND_FAIL_CNT  3

#define SC_BS_MSG_HASH_SIZE  1024

#define SC_BS_MEG_BUFF_LEN   1024

#define SC_BS_NEED_RESEND    0

/* 客户端状态 */
enum{
    SC_BS_STATUS_INVALID         = 0, /* 没有初始化 */
    SC_BS_STATUS_INIT,                /* 已经初始化了 */
    SC_BS_STATUS_CONNECT,             /* 已经连接 */
    SC_BS_STATUS_LOST,                /* 掉线了 */
    SC_BS_STATUS_REALSE,              /* 掉线了 */
};

/* 客户端控制块 */
typedef struct tagBSClient
{
    U32 ulIndex;
    U32 blValid;
    U32 ulStatus;
    S32 lSocket;

    U16 usPort;
    U16 usCommProto;

    union tagAddr{
        struct sockaddr_in  stInAddr;
        struct sockaddr_un  stUnAddr;
    }stAddr;

    U32 aulIPAddr[4];
    U32 ulHBFailCnt;
    DOS_TMR_ST hTmrHBInterval;
    DOS_TMR_ST hTmrHBTimeout;
    U32 ulCtrlFailCnt;
    DOS_TMR_ST hTmrCtrlInterval;
    DOS_TMR_ST hTmrCtrlTimeout;
}SC_BS_CLIENT_ST;

typedef struct tagBSMsgNode
{
    HASH_NODE_S   stLink;          /*  链表节点 */

    U32           ulFailCnt;       /* 发送失败次数 */
    U32           ulRCNo;          /* 资源编号，SCB的编号 */
    U32           ulSeq;           /* 资源编号，SCB的编号 */

    VOID          *pData;          /* 数据 */
    U32           ulLength;

    U32           blNeedSyn;
    DOS_TMR_ST    hTmrSendInterval;
}SC_BS_MSG_NODE;

U32 sc_send_release_ind2bs(BS_MSG_TAG *pstMsg);
U32 sc_send_hello2bs(U32 ulClientIndex);
U32 sc_send_billing_stop2bs(SC_SRV_CB *pstSCB, SC_LEG_CB *pstFristLeg, SC_LEG_CB *pstSecondLeg);
U32 sc_send_billing_stop2bs_record(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLeg);
#if SC_BS_NEED_RESEND
U32 sc_bs_msg_free(U32 ulSeq);
#endif
#endif

