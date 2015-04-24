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
#define PTC_WEB_SOCKET_MAX_COUNT    16         /* 访问proxy的最大socket */
#define PTC_CMD_SOCKET_MAX_COUNT    8          /* 访问proxy的最大socket */
#define PTC_TIME_SIZE               24
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

}PTC_CLIENT_CB_ST;


extern PT_CC_CB_ST      *g_pstPtcSend;
extern PT_CC_CB_ST      *g_pstPtcRecv;
extern PTC_SERV_MSG_ST   g_stServMsg;
extern pthread_cond_t    g_cond_recv;
extern pthread_mutex_t   g_mutex_recv;
extern list_t  *g_pstPtcNendRecvNode;
extern sem_t g_SemPtcRecv;
extern PT_PTC_TYPE_EN g_enPtcType;

VOID  ptc_save_msg_into_cache(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen);
VOID *ptc_send_msg2pts(VOID *arg);
VOID *ptc_recv_msg_from_pts(VOID *arg);
VOID  ptc_send_heartbeat2pts();
VOID  ptc_send_login2pts();
VOID  ptc_send_logout2pts();
VOID  ptc_send_pthread_mutex_lock(S8 *szFileName, U32 ulLine);
VOID  ptc_send_pthread_mutex_unlock(S8 *szFileName, U32 ulLine);
VOID  ptc_recv_pthread_mutex_lock(S8 *szFileName, U32 ulLine);
VOID  ptc_recv_pthread_mutex_unlock(S8 *szFileName, U32 ulLine);
VOID  ptc_recv_pthread_cond_wait(S8 *szFileName, U32 ulLine);
VOID  ptc_send_exit_notify_to_pts(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S32 lSeq);
VOID  ptc_delete_send_stream_node(U32 ulStreamID, PT_DATA_TYPE_EN enDataType, BOOL bIsMutex);
VOID  ptc_delete_recv_stream_node(U32 ulStreamID, PT_DATA_TYPE_EN enDataType, BOOL bIsMutex);
S32   ptc_get_version(U8 *szVersion);
S32   ptc_create_udp_socket(U32 ulSocketCache);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif

