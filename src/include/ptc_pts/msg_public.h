#ifndef __MSG_PUBLIC_H__
#define __MSG_PUBLIC_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <dos.h>

#define IPCC_ID_LEN                         16
#define PT_IP_SIZE                          4
#define PT_DATA_RECV_CACHE_SIZE             1024
#define PT_DATA_SEND_CACHE_SIZE             512
#define PT_SEND_DATA_SIZE                   1024
#define PT_RECV_DATA_SIZE                   512
#define PT_SEND_LOSE_DATA_TIMER             60
#define PT_RESEND_COUNT                     3
#define alert(format,...)                    printf(__FILE__"  %d: "format"\n", __LINE__, ##__VA_ARGS__)

/*传输的数据类型*/
typedef enum
{
    PT_DATA_CTRL = 0,                                        /*控制*/
    PT_DATA_WEB,                                             /*web*/
    PT_DATA_CMD,                                             /*命令行*/
    PT_DATA_BUTT

}PT_DATA_TYPE_EN;

typedef enum
{
    PT_TCP = 0,
    PT_UDP,
    PT_BUTT

}PT_PROT_TYPE_EN;


/*数据包*/
typedef struct tagDataUdp
{
	U8						bIsSaveMsg;	   					  /*是否存储数据*/
	U8                      ExitNotifyFlag;                   /*是否响应结束*/
    S8                      Reserver[1];

    S32                     lSeq;                             /*包编号*/
    U32                     ulLen;                            /*包大小*/
	S8*                     pcBuff;                           /*包数据*/

}PT_DATA_UDP_ST;


typedef struct tagDataTcp
{
	U8						bIsSaveMsg;	   					  /*是否存储数据*/
	U8                      ExitNotifyFlag;                   /*是否响应结束*/
    S8                      Reserver[1];

    S32                     lSeq;                             /*包编号*/
    U32                     ulLen;                            /*包大小*/
	S8                      szBuff[PT_RECV_DATA_SIZE];        /*包数据*/

}PT_DATA_TCP_ST;


typedef union tagDataQue
{
    PT_DATA_UDP_ST* pstDataUdp;
	PT_DATA_TCP_ST* pstDataTcp;
}PT_DATA_QUE_HEAD_UN;


typedef struct tagStreamCB
{
    list_t                  stStreamListNode;
    PT_DATA_QUE_HEAD_UN     unDataQueHead;                    /*数据地址*/
	DOS_TMR_ST              hTmrHandle;                      /*定时器*/
    U32                     ulStreamID;                       /*stream ID*/
    S32                     ulMinSeq;                         /*最小包编号*/
	S32                     ulMaxSeq;                         /*最大包编号*/
    S32                     ulCurrSeq;                        /*已发送包的编号*/
	S32                     lSockFd;                          /*与proxy或者client通信的套接字*/
	U32                     ulCountResend;
}PT_STREAM_CB_ST;

typedef struct tagChannelCB
{
    PT_DATA_TYPE_EN         enDataType;                        /*类型*/
    BOOL                    bValid;                            /*有效性*/
    list_t*                 pstStreamQueHead;                  /*stream list*/
    U32                     ulStreamNumInQue;
}PT_CHAN_CB_ST;

typedef struct tagCCCB
{
    list_t                  stCCListNode;
    U8                      aucID[IPCC_ID_LEN];               /*IPCC ID*/
    S32                     lSocket;                          /*UDP socket*/
    struct sockaddr_in      stDestAddr;                       /*ipcc addr*/
    PT_CHAN_CB_ST           astDataTypes[PT_DATA_BUTT];       /*传输类型*/
	S8                      Reserver[8];
}PT_CC_CB_ST;

typedef struct tagMsg
{
    U32                     ulStreamID;
	S32                     lSeq;                            /*包编号*/
    U8                      aucID[IPCC_ID_LEN];              /*ipcc ID*/
	U32 					aulServIp[PT_IP_SIZE];

    U16	                    usServPort;
	U8                      enDataType;                      /*mag类型*/
    U8                      ExitNotifyFlag;                  /**/

    U8                      bIsResendReq;                    /*是否是重传请求*/
    U8                      bIsEncrypt;                      /*是否加密*/
    U8                      bIsCompress;                     /*是否压缩*/
    S8                      Reserver[1];
}PT_MSG_ST;

/*定时器回调函数的参数*/
typedef struct tagLoseBagMsg
{
    PT_MSG_ST           *stMsg;
	PT_STREAM_CB_ST     *stStreamNode;
}PT_LOSE_BAG_MSG_ST;

PT_DATA_TCP_ST *pt_data_tcp_queue_create(U32 ulCacheSize);
BOOL pt_send_data_tcp_queue_add(PT_STREAM_CB_ST* pstStreamNode, U8 ExitNotifyFlag, S8 *pcData, S32 lDataLen, S32 lSeq, U32 ulCacheSize);
PT_STREAM_CB_ST *pt_stream_node_create(U32 ulStreamID, PT_DATA_QUE_HEAD_UN unDataQueHead, S32 lSeq);
list_t *pt_stream_queue_insert(list_t *pstHead, list_t *pstStreamNode);
PT_STREAM_CB_ST *pt_stream_queue_serach(list_t *pstStreamListHead, U32 ulStreamID);
PT_STREAM_CB_ST *pt_stream_queue_serach_by_sockfd(PT_CC_CB_ST *pstCCCB, S32 lSockFd);
list_t *pt_create_stream_and_data_que(U32 ulStreamID, S8 *pcData, S32 lDataLen, U8 ExitNotifyFlag, S32 lSeq, U32 ulCacheSize);
list_t *pt_delete_stream_resource(list_t *pstCCNode, list_t *pstStreamNode, PT_DATA_TYPE_EN enDataType);
#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif
