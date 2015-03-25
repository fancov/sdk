#ifndef __PTS_LIB_H__
#define __PTS_LIB_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <semaphore.h>
#include <pt/pt.h>
#include <pt/goahead/webs.h>

#define PTS_SQL_STR_SIZE            1024           /* 存放sql语句数组的大小 */
#define PTS_LISTEN_COUNT            10             /* listen 个数 */
#define PTS_WAIT_HB_TIME            5000           /* 等待心跳超时的时间 */
#define PTS_HB_TIMEOUT_COUNT_MAX    3              /* 心跳超时的次数，超过，则认为掉线 */
#define PTS_DEBUG                   0
#define PTS_TIME_SIZE               24
#define PTS_DOMAIN_SIZE             64
#define PTS_WEB_COOKIE_EXPIRES      600
#define PTS_WEB_SERVER_MAX_SIZE     64
#define PTS_WEB_TIMEOUT             640000       /* 浏览器不操作超时时间 5分钟 */
#define PTS_PING_TIMEOUT            2000

typedef struct tagClientCB
{
    list_t                  stList;
    U32                     ulStreamID;                         /* stream ID */
    S32                     lSocket;                            /* client sockfd */
    U8                      aucID[PTC_ID_LEN];                  /* PTC ID */
    struct sockaddr_in      stClientAddr;                       /* IPCC addr */
    BOOL                    eSaveHeadFlag;                      /* 是否保存有http头 */
    S8 *                    pcRequestHead;                      /* 不完整的http头 */
    S32                     lReqLen;
    S8                      Reserver[8];
}PTS_CLIENT_CB_ST;

typedef struct tagPtsServMsg
{
   //S8                    szDoMain[PTS_DOMAIN_SIZE];
   U16                     usPtsPort;
   U16                     usWebServPort;
   U16                     usCmdServPort;
   S8                      Reserver[2];
}PTS_SERV_MSG_ST;

typedef struct tagServListenSocket
{
    S32             lSocket;        /* 套接字 */
    U16             usServPort;     /* 端口 */
    DOS_TMR_ST      hTmrHandle;     /* 定时器 */
}PTS_SERV_SOCKET_ST;

typedef struct tagPingStatsNode
{
    webs_t  wp;
    S8      szPtcID[PTC_ID_LEN+1];
    double  TotalTime;
    double  MaxTime;
    double  MinTime;
    U32     ulReceivedNum;

}PTS_PING_STATS_NODE_ST;

extern list_t *g_pstPtcListRecv;
extern list_t *g_pstPtcListSend;
extern PTS_SERV_MSG_ST  g_stPtsMsg;
extern pthread_cond_t   g_pts_cond_recv;
extern pthread_mutex_t  g_pts_mutex_send;
extern pthread_mutex_t  g_pts_mutex_recv;
extern list_t  *g_pstPtsNendRecvNode;
extern sem_t g_SemPtsRecv;
extern PTS_SERV_SOCKET_ST g_lPtsServSocket[PTS_WEB_SERVER_MAX_SIZE];

VOID *pts_recv_msg_from_ptc(VOID *arg);
VOID *pts_send_msg2ptc(VOID *arg);
VOID pts_save_msg_into_cache(U8 *pcIpccId, PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen, S8 *szDestIp, U16 usDestPort);
VOID pts_send_exit_notify2ptc(PT_CC_CB_ST *pstPtcNode, PT_NEND_RECV_NODE_ST *pstNeedRecvNode);
VOID pts_delete_recv_stream_node(PT_MSG_TAG *pstMsgDes, PT_CC_CB_ST *pstPtcRecvNode, BOOL bIsMutex);
VOID pts_delete_send_stream_node(PT_MSG_TAG *pstMsgDes, PT_CC_CB_ST *pstPtcSendNode, BOOL bIsMutex);
VOID pts_send_pthread_mutex_lock(S8 *szFileName, U32 ulLine);
VOID pts_send_pthread_mutex_unlock(S8 *szFileName, U32 ulLine);
VOID pts_send_pthread_cond_wait(S8 *szFileName, U32 ulLine);
VOID pts_recv_pthread_mutex_lock(S8 *szFileName, U32 ulLine);
VOID pts_recv_pthread_mutex_unlock(S8 *szFileName, U32 ulLine);
VOID pts_recv_pthread_cond_timedwait(struct timespec *timeout, S8 *szFileName, U32 ulLine);
S8 *pts_get_current_time();
VOID pts_send_exit_notify_to_ptc(PT_MSG_TAG *pstMsgDes, PT_CC_CB_ST *pstPtcSendNode);
S32 pts_find_ptc_by_dest_addr(S8 *pDestInternetIp, S8 *pDestIntranetIp, S8 *aucDestID);
S32 pts_get_curr_position_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif
