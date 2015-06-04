#ifndef __PTC_LIB_H__
#define __PTC_LIB_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include <semaphore.h>
#include <pt/pt.h>

#define PTC_SQL_STR_SIZE            512        /* 存放sql语句数组的大小 */
#define PTC_SEND_HB_TIME            5000       /* 向pts发送心跳的间隔 */
#define PTC_WAIT_HB_ACK_TIME        3000       /* ptc接收pts心跳响应的超时时间 */
#define PTC_HB_TIMEOUT_COUNT_MAX    4          /* 心跳响应最大连续超时次数，超过则认为掉线，重新发送登陆请求 */
//#define PTC_DNS_IP_SIZE           2          /* 保存DNS解析出IP的数组的大小 */
#define PTC_DEBUG                   0          /* 调试宏 */
#define PTC_WAIT_CONFIRM_TIMEOUT    5          /* 等待确认接收消息的超时时间 s */
#define PTC_RECV_FROM_PTS_CMD_SIZE  8
#define PTC_HISTORY_PATH            "var/pts_history"      /* history 的文件目录 */

typedef struct tagServMsg
{
    S8                    szHistoryPath[PT_DATA_BUFF_128];             /* history 的路径 */
    S8                    achPtsMajorDomain[PT_DATA_BUFF_64];         /* pts主域名地址 */
    S8                    achPtsMinorDomain[PT_DATA_BUFF_64];         /* pts备域名地址 */
    U8                    achPtsMajorIP[IPV6_SIZE];          /* PTS 主 IP */
    U8                    achPtsMinorIP[IPV6_SIZE];          /* PTS 副 IP */
    S8                    szPtsLasterIP1[PT_IP_ADDR_SIZE];   /* pts历史记录 */
    S8                    szPtsLasterIP2[PT_IP_ADDR_SIZE];   /* pts历史记录 */
    S8                    szPtsLasterIP3[PT_IP_ADDR_SIZE];   /* pts历史记录 */
    U8                    achLocalIP[IPV6_SIZE];             /* 本机IP */
    S8                    szMac[PT_DATA_BUFF_64];            /* mac地址 */

    U16                   usPtsMajorPort;                    /* pts主域名端口 */
    U16                   usPtsMinorPort;                    /* pts备域名端口 */
    U16                   usPtsLasterPort1;                  /* PTS PORT */
    U16                   usPtsLasterPort2;                  /* PTS PORT */
    U16                   usPtsLasterPort3;                  /* PTS PORT */
    U16                   usLocalPort;                       /* PTS PORT */

} PTC_SERV_MSG_ST;

typedef struct tagPtcClientCB
{
    U32                     ulStreamID;                         /* stream ID */
    S32                     lSocket;                            /* client sockfd */
    BOOL                    bIsValid;
    PT_DATA_TYPE_EN         enDataType;
    list_t                  stRecvBuffList;
    U32                     ulBuffNodeCount;

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

S32  ptc_save_msg_into_cache(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen);
VOID *ptc_send_msg2pts(VOID *arg);
VOID *ptc_recv_msg_from_pts(VOID *arg);
VOID *ptc_send_msg2proxy(VOID *arg);
VOID  ptc_send_heartbeat2pts();
VOID  ptc_send_login2pts();
VOID  ptc_send_logout2pts();
VOID  ptc_send_exit_notify_to_pts(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S32 lSeq);
S32   ptc_get_version(U8 *szVersion);
S32   ptc_create_udp_socket(U32 ulSocketCache);

void ptc_init_ptc_client_cb();
S32 ptc_get_socket_by_streamID(U32 ulStreamID);
S32 ptc_get_streamID_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType);
S32 ptc_get_client_node_array_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType);
S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType);
VOID ptc_delete_client_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType);
VOID ptc_delete_client_by_streamID(U32 ulStreamID);
void ptc_set_cache_full_true(S32 lSocket, PT_DATA_TYPE_EN enDataType);
void ptc_set_cache_full_false(U32 ulStreamID, PT_DATA_TYPE_EN enDataType);
void ptc_delete_stream_node_by_recv(PT_STREAM_CB_ST *pstRecvStreamNode);
VOID ptc_delete_send_stream_node_by_streamID(U32 ulStreamID);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif

