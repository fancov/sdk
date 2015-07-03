#ifndef __PTC_LIB_H__
#define __PTC_LIB_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include <semaphore.h>
#include <pt/pt.h>
#include <time.h>

#define PTC_SQL_STR_SIZE                512                     /* 存放sql语句数组的大小 */
#define PTC_SEND_HB_TIME                5000                    /* 向pts发送心跳的间隔 */
#define PTC_WAIT_HB_ACK_TIME            3000                    /* ptc接收pts心跳响应的超时时间 */
#define PTC_HB_TIMEOUT_COUNT_MAX        4                       /* 心跳响应最大连续超时次数，超过则认为掉线，重新发送登陆请求 */
//#define PTC_DNS_IP_SIZE               2                       /* 保存DNS解析出IP的数组的大小 */
#define PTC_DEBUG                       0                       /* 调试宏 */
#define PTC_WAIT_CONFIRM_TIMEOUT        5                       /* 等待确认接收消息的超时时间 s */
#define PTC_RECV_FROM_PTS_CMD_SIZE      8
#define PTC_HISTORY_PATH                "var/pts_history"       /* history 的文件目录 */
#define PTC_GET_LOCAL_IP_SERVER_DOMAIN  "www.baidu.com"
#define PTC_GET_LOCAL_IP_SERVER_IP      "180.97.33.107"
#define PTC_GET_LOCAL_IP_SERVER_PORT    80
#define PTC_CONNECT_TIME_OUT            10 * 60
#define PTC_DISPOSE_INTERVAL_TIME       5 * 60
#define PTC_LOCK_MIN_COUNT              3

typedef struct tagServMsg
{
    S8                    szHistoryPath[PT_DATA_BUFF_128];             /* history 的路径 */
    S8                    achPtsMajorDomain[PT_DATA_BUFF_64];         /* pts主域名地址 */
    S8                    achPtsMinorDomain[PT_DATA_BUFF_64];         /* pts备域名地址 */
    U8                    achPtsMajorIP[IPV6_SIZE];          /* PTS 主 IP */
    U8                    achPtsMinorIP[IPV6_SIZE];          /* PTS 副 IP */
    U8                    achBaiduIP[IPV6_SIZE];             /* baidu IP */
    S8                    szPtsLasterIP1[PT_IP_ADDR_SIZE];   /* pts历史记录 */
    S8                    szPtsLasterIP2[PT_IP_ADDR_SIZE];   /* pts历史记录 */
    S8                    szPtsLasterIP3[PT_IP_ADDR_SIZE];   /* pts历史记录 */
    U8                    achLocalIP[IPV6_SIZE];             /* 本机IP */
    S8                    szMac[PT_DATA_BUFF_64];            /* mac地址 */

    U16                   usPtsMajorPort;                    /* pts主域名端口 */
    U16                   usPtsMinorPort;                    /* pts备域名端口 */
    U16                   usBaiduPort;                       /* baidu端口 */
    U16                   usPtsLasterPort1;                  /* PTS PORT */
    U16                   usPtsLasterPort2;                  /* PTS PORT */
    U16                   usPtsLasterPort3;                  /* PTS PORT */
    U16                   usLocalPort;                       /* PTS PORT */

} PTC_SERV_MSG_ST;

typedef enum
{
    PTC_CLIENT_LEISURE,         /* 空闲中 */
    PTC_CLIENT_USING,           /* 使用中 */
    PTC_CLIENT_FREEING,         /* 释放中 */

    PT_CLIENT_BUTT

}PTC_CLIENT_STATE;

typedef struct tagPtcClientCB
{
    U32                     ulStreamID;                         /* stream ID */
    S32                     lSocket;                            /* client sockfd */
    PTC_CLIENT_STATE        enState;
    PT_DATA_TYPE_EN         enDataType;
    list_t                  stRecvBuffList;
    U32                     ulBuffNodeCount;
    pthread_mutex_t         pMutexPthread;
    time_t                  lLastUseTime;
    PT_STREAM_CB_ST         *pstSendStreamNode;

}PTC_CLIENT_CB_ST;


extern PT_CC_CB_ST      *g_pstPtcSend;
extern PT_CC_CB_ST      *g_pstPtcRecv;
extern PTC_SERV_MSG_ST   g_stServMsg;
extern pthread_cond_t    g_condPtcRecv;
extern pthread_mutex_t   g_mutexPtcRecvPthread;
extern list_t  g_stPtcNendRecvNode;
extern sem_t g_SemPtcRecv;
extern PT_PTC_TYPE_EN g_enPtcType;
extern S32 g_ulUdpSocket;
extern PTC_CLIENT_CB_ST g_astPtcConnects[PTC_STREAMID_MAX_COUNT];
extern U32 g_ulPtcNendSendNodeCount;

VOID *ptc_send_msg2pts(VOID *arg);
VOID *ptc_recv_msg_from_pts(VOID *arg);
VOID *ptc_send_msg2proxy(VOID *arg);
VOID  ptc_send_heartbeat2pts();
VOID  ptc_send_login2pts();
VOID  ptc_send_logout2pts();
VOID  ptc_send_exit_notify_to_pts(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S32 lSeq);
S32   ptc_get_version(U8 *szVersion);
S32   ptc_create_udp_socket(U32 ulSocketCache);
void ptc_free_stream_resoure(PT_DATA_TYPE_EN enDataType, PTC_CLIENT_CB_ST *pstPtcClientCB, U32 ulStreamID, S32 lExitType);
void ptc_init_ptc_client_cb(PTC_CLIENT_CB_ST *pstPtcClientCB);
void ptc_init_all_ptc_client_cb();
void ptc_delete_all_client_cb();
S32 ptc_get_socket_by_streamID(U32 ulStreamID);
S32 ptc_get_streamID_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType);
S32 ptc_get_client_node_array_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType);
S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType, PT_STREAM_CB_ST *pstRecvStreamNode);
VOID ptc_delete_client_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType);
VOID ptc_delete_client_by_streamID(U32 ulStreamID);
void ptc_set_cache_full_true(S32 lSocket, PT_DATA_TYPE_EN enDataType);
void ptc_set_cache_full_false(U32 ulStreamID, PT_DATA_TYPE_EN enDataType);
VOID ptc_delete_client_by_index(PTC_CLIENT_CB_ST *pstPtcClientCB);
void ptc_delete_stream_node_by_recv(PT_STREAM_CB_ST *pstRecvStreamNode);
VOID ptc_delete_send_stream_node_by_streamID(U32 ulStreamID);
void *ptc_terminal_dispose(void *arg);
BOOL ptc_cache_is_or_not_have_space(PT_STREAM_CB_ST *pstSendStreamNode);
S32 ptc_save_ctrl_msg_into_cache(U32 ulStreamID, S8 *acSendBuf, S32 lDataLen);
S32 ptc_save_msg_into_cache(PT_STREAM_CB_ST *pstStreamNode, PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *acSendBuf, S32 lDataLen);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif

