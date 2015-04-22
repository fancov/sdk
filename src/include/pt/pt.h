#ifndef __PT_H__
#define __PT_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <dos.h>
#include <pt/pt_msg.h>
#include <netinet/in.h>
#include <sys/syscall.h>

#define PT_DATA_BUFF_10             10
#define PT_DATA_BUFF_16             16
#define PT_DATA_BUFF_32             32
#define PT_DATA_BUFF_64             64
#define PT_DATA_BUFF_128            128
#define PT_DATA_BUFF_256            256
#define PT_DATA_BUFF_512            512
#define PT_DATA_BUFF_1024           1024
#define PT_DATA_BUFF_2048           2048
#define MAX_FILENAME_LEN            24
#define PT_DATA_RECV_CACHE_SIZE     1024    /* 发送缓存的个数 */
#define PT_DATA_SEND_CACHE_SIZE     512     /* 接收缓存的个数 */
#define PT_SEND_DATA_SIZE           700     /* 接收数据包时，使用的数组的大小 */
#define PT_RECV_DATA_SIZE           512     /* socket 每数据的大小 */
#define PT_SEND_LOSE_DATA_TIMER     1000    /* 发送丢包请求的间隔 */
#define PT_RESEND_RSP_COUNT         3       /* 收到丢包请求时，发送丢失包的个数 */
#define PT_RESEND_REQ_MAX_COUNT     3       /* 丢失的包的最大请求次数 */
#define PT_SEND_CONFIRM_COUNT       3       /* 发送确认消息的次数 */
#define PT_CONFIRM_RECV_MSG_SIZE    64      /* 收到多少个包时，发送确认信息 */
#define PT_LOGIN_VERIFY_SIZE        12      /* ptc登陆时，验证字符串的大小 */
#define PT_SAVE_DATA_FAIL           -1      /* 保存数据到缓存失败 */
#define PT_SAVE_DATA_SUCC           0       /* 保存数据到缓存成功 */
#define PT_NEED_CUT_PTHREAD         1       /* 缓存存满，需要切换线程 */
#define PT_VERSION_LEN              4       /* ptc版本号的长度 */
#define PT_PTC_NAME_LEN             16      /* ptc名称 */
#define PT_IP_ADDR_SIZE             16      /* 点分十进制，ip的长度 */
#define PT_MD5_LEN                  33      /* md5加密的长度 32+1 */
#define PT_MD5_ARRAY_SIZE           16
#define pt_logr_emerg(_pszFormat, args...)   pt_logr_write(LOG_LEVEL_ALERT, (_pszFormat), ##args)
#define pt_logr_cirt(_pszFormat, args...)    pt_logr_write(LOG_LEVEL_CIRT, (_pszFormat), ##args)
#define pt_logr_error(_pszFormat, args...)   pt_logr_write(LOG_LEVEL_ERROR, (_pszFormat), ##args)
#define pt_logr_warning(_pszFormat, args...) pt_logr_write(LOG_LEVEL_WARNING, (_pszFormat), ##args)
#define pt_logr_notic(_pszFormat, args...)   pt_logr_write(LOG_LEVEL_NOTIC, (_pszFormat), ##args)
#define pt_logr_info(_pszFormat, args...)    pt_logr_write(LOG_LEVEL_INFO, (_pszFormat), ##args)
#define pt_logr_debug(_pszFormat, args...)   pt_logr_write(LOG_LEVEL_DEBUG, (_pszFormat), ##args)
#define CRC16_POLY                  0x1021
#define PT_MUTEX_DEBUG              0
#define PT_PTC_MUTEX_DEBUG          0
#define PT_WEB_MUTEX_DEBUG          0

#define gettid() syscall(__NR_gettid)       /* 获得线程号 */

extern S32 g_ulUdpSocket;
extern U8  gucIsTableInit;
extern U16 g_crc_tab[256];

/* 传输的数据类型 */
typedef enum
{
    PT_DATA_CTRL = 0,          /* 控制 */
    PT_DATA_WEB,               /* web */
    PT_DATA_CMD,               /* 命令行 */
    PT_DATA_BUTT

}PT_DATA_TYPE_EN;

/* 通道传输方式 */
typedef enum
{
    PT_TCP = 0,
    PT_UDP,
    PT_BUTT

}PT_PROT_TYPE_EN;


/* 数据包 */
typedef struct tagDataUdp
{
    S32                     lSeq;                             /* 包编号 */
    U32                     ulLen;                            /* 包大小 */
    U8                      ExitNotifyFlag;                   /* 是否响应结束 */
    S8                      Reserver[3];
    S8*                     pcBuff;                           /* 包数据 */

}PT_DATA_UDP_ST;


typedef struct tagDataTcp
{
    S32                     lSeq;                             /* 包编号 */
    U32                     ulLen;                            /* 包大小 */
    U8                      ExitNotifyFlag;                   /* 是否响应结束 */
    S8                      Reserver[3];

    S8                      szBuff[PT_RECV_DATA_SIZE];        /* 包数据 */

}PT_DATA_TCP_ST;


typedef union tagDataQue
{
    PT_DATA_UDP_ST* pstDataUdp;
    PT_DATA_TCP_ST* pstDataTcp;
}PT_DATA_QUE_HEAD_UN;


typedef struct tagStreamCB
{
    list_t                  stStreamListNode;
    PT_DATA_QUE_HEAD_UN     unDataQueHead;                    /* 数据地址 */
    DOS_TMR_ST              hTmrHandle;                       /* 定时器 */
    struct tagLoseBagMsg    *pstLostParam;                    /* 定时器参数地址，用于释放资源 */
    U32                     ulStreamID;                       /* stream ID */
    S32                     lMaxSeq;                          /* 最大包编号 */
    S32                     lCurrSeq;                         /* 已发送包的编号 */
    S32                     lConfirmSeq;                      /* 确认接收的最大编号 */
    U32                     ulCountResend;
    U8                      aulServIp[IPV6_SIZE];
    U16                     usServPort;
    S8                      Reserver[2];

}PT_STREAM_CB_ST;

typedef struct tagChannelCB
{
    PT_DATA_TYPE_EN         enDataType;                        /*类型*/

    list_t*                 pstStreamQueHead;                  /*stream list*/
    U32                     ulStreamNumInQue;

    BOOL                    bValid;                            /*有效性*/
    S8                      Reserver[3];

}PT_CHAN_CB_ST;

typedef struct tagCCCB
{
    list_t                  stCCListNode;
    U8                      aucID[PTC_ID_LEN];                /* IPCC ID */
    S32                     lSocket;                          /* UDP socket */
    struct sockaddr_in      stDestAddr;                       /* ipcc addr */
    PT_CHAN_CB_ST           astDataTypes[PT_DATA_BUTT];       /* 传输类型 */
    U8                      aucPtcIp[IPV6_SIZE];
    DOS_TMR_ST              stHBTmrHandle;                    /* 心跳定时器 */
    U16                     usHBOutTimeCount;                 /* 心跳连续超时次数 */
    U16                     usPtcPort;
    U32                     ulUdpLostDataCount;
    U32                     ulUdpRecvDataCount;
    //S8                    Reserver[8];
}PT_CC_CB_ST;


/* 传输的数据类型 */
typedef enum
{
    PT_CTRL_LOGIN_REQ = 0,
    PT_CTRL_LOGIN_RSP,
    PT_CTRL_LOGIN_ACK,
    PT_CTRL_LOGOUT,
    PT_CTRL_HB_REQ,
    PT_CTRL_HB_RSP,
    PT_CTRL_SWITCH_PTS,
    PT_CTRL_PTS_MAJOR_DOMAIN,
    PT_CTRL_PTS_MINOR_DOMAIN,
    PT_CTRL_PTC_PACKAGE,
    PT_CTRL_PING,
    PT_CTRL_PING_ACK,
    PT_CTRL_COMMAND,                /* pts发送命令到ptc，获取ptc的信息 */

    PT_CTRL_BUTT

}PT_CTRL_TYPE_EN;

/* 传输的数据类型 */
typedef enum
{
    PT_PTC_TYPE_EMBEDDED = 0,        /* EMBEDDED */
    PT_PTC_TYPE_WINDOWS,        /* WINDOWS */
    PT_PTC_TYPE_LINUX,           /* LINUX */

    PT_PTC_TYPE_BUTT

}PT_PTC_TYPE_EN;

/* 产品类型 */
typedef enum
{
    PT_TYPE_PTS_LINUX = 1,
    PT_TYPE_PTS_WINDOWS,
    PT_TYPE_PTC_LINUX,
    PT_TYPE_PTC_WINDOWS,
    PT_TYPE_PTC_EMBEDDED,

    PT_TYPE_BUTT

}PT_PRODUCT_TYPE_EN;


/* 控制消息内容 */
typedef struct tagCtrlData
{
    S8      szLoginVerify[PT_LOGIN_VERIFY_SIZE];    /* 用于登陆验证的随机字符串 */
    U8      szVersion[PT_VERSION_LEN];              /* 版本号 */
    S8      szPtcName[PT_PTC_NAME_LEN];             /* ptc名称 */
    U32     ulLginVerSeq;                           /* 登陆验证随机字符串在发送缓存中存储的包包编号 */
    S8      achPtsMajorDomain[PT_DATA_BUFF_64];     /* pts主域名地址 */
    S8      achPtsMinorDomain[PT_DATA_BUFF_64];     /* pts备域名地址 */
    S8      szPtsHistoryIp1[IPV6_SIZE];             /* ptc注册的pts的历史记录 */
    S8      szPtsHistoryIp2[IPV6_SIZE];             /* ptc注册的pts的历史记录 */
    S8      szPtsHistoryIp3[IPV6_SIZE];             /* ptc注册的pts的历史记录 */
    S8      szMac[PT_DATA_BUFF_64];
    U16     usPtsHistoryPort1;
    U16     usPtsHistoryPort2;
    U16     usPtsHistoryPort3;
    U16     usPtsMajorPort;                         /* pts主域名端口 */
    U16     usPtsMinorPort;                         /* pts备域名端口 */

    S32     lHBTimeInterval;                        /* 心跳和心跳响应的间隔，查看网络情况 */

    U8      enCtrlType;                             /* 控制消息类型 PT_CTRL_TYPE_EN */
    U8      ucLoginRes;                             /* 登陆结果 */
    U8      enPtcType;                              /* ptc类型 PT_PTC_TYPE_EN */
    S8      Reserver[1];

}PT_CTRL_DATA_ST;

/* 定时器回调函数的参数 */
typedef struct tagLoseBagMsg
{
    PT_MSG_TAG          stMsg;
    PT_STREAM_CB_ST     *pstStreamNode;
    S32                 lSocket;

}PT_LOSE_BAG_MSG_ST;

/* 记录接收缓存中可以接收的数据包的ptc和stream节点 */
typedef struct tagNendRecvNode
{
    list_t              stListNode;
    U8                  aucID[PTC_ID_LEN];          /* PTC ID */
    PT_DATA_TYPE_EN     enDataType;                 /* 类型 */
    U32                 ulStreamID;                 /* stream ID */
    S32                 lSeq;
    U8                  ExitNotifyFlag;
    S8                  Reserver[3];

}PT_NEND_RECV_NODE_ST;

/* 记录发送缓存中需要发送的数据包的ptc和stream节点 */
typedef struct tagNendSendNode
{
    list_t              stListNode;
    U8                  aucID[PTC_ID_LEN];          /* PTC ID */
    PT_DATA_TYPE_EN     enDataType;                 /* 类型 */
    U32                 ulStreamID;                 /* stream ID */
    S32                 lSeqResend;                 /* 要求重发或者重发的包编号 */
    PT_CMD_EN           enCmdValue;                 /* 重传/确认/正常 */
    BOOL                bIsResend;                  /* 是否是重发请求 */
    U8                  ExitNotifyFlag;
    S8                  Reserver[3];

}PT_NEND_SEND_NODE_ST;


/* ping */
typedef struct tagPingPacket
{
   U32              ulSeq;
   U32              ulPacketSize;
   U32              ulPingNum;
   struct timeval   stStartTime;
   void             *pWpHandle;
   DOS_TMR_ST       hTmrHandle;     /* 定时器 */

}PT_PING_PACKET_ST;

typedef struct tagPtcCommand
{
    U32 ulIndex;
    S8 szCommand[PT_DATA_BUFF_16];

}PT_PTC_COMMAND_ST;


/* ptc升级包，包头的结构体 */
typedef  struct tagFileHeadStruct
{
    U32 usCRC;                           /* CRC校验码 */
    S8  szFileName[MAX_FILENAME_LEN];    /* 版本文件名 */
    U32 ulVision;                        /* 版本号 */
    U32 ulFileLength;                    /* 文件长度,不含tag */
    U32 ulDate;                          /* 日期 */
    U32 ulOSType;                        /* 操作系统类型 */

}LOAD_FILE_TAG;

PT_CC_CB_ST *pt_ptc_node_create(U8 *pcIpccId, S8 *szPtcVersion, struct sockaddr_in stDestAddr);
list_t *pt_delete_ptc_node(list_t *stPtcListHead, PT_CC_CB_ST *pstPtcNode);
list_t *pt_ptc_list_insert(list_t *pstPtcListHead, PT_CC_CB_ST *pstPtcNode);
PT_CC_CB_ST *pt_ptc_list_search(list_t* pstHead, U8 *pucID);
void pt_delete_ptc_resource(PT_CC_CB_ST *pstPtcNode);

PT_STREAM_CB_ST *pt_stream_node_create(U32 ulStreamID);
list_t *pt_stream_queue_insert(list_t *pstHead, list_t *pstStreamNode);
PT_STREAM_CB_ST *pt_stream_queue_search(list_t *pstStreamListHead, U32 ulStreamID);
list_t *pt_delete_stream_node(list_t *pstStreamListHead, list_t *pstStreamNode, PT_DATA_TYPE_EN enDataType);

PT_DATA_TCP_ST *pt_data_tcp_queue_create(U32 ulCacheSize);
S32 pt_send_data_tcp_queue_insert(PT_STREAM_CB_ST *pstStreamNode, S8 *acSendBuf, S32 lDataLen, U32 lCacheSize);
S32 pt_recv_data_tcp_queue_insert(PT_STREAM_CB_ST *pstStreamNode, PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen, U32 lCacheSize);

list_t *pt_need_recv_node_list_insert(list_t *pstHead, PT_MSG_TAG *pstMsgDes);
list_t *pt_need_recv_node_list_search(list_t *pstHead, U32 ulStreamID);
list_t *pt_need_send_node_list_insert(list_t *pstHead, U8 *aucID, PT_MSG_TAG *pstMsgDes, PT_CMD_EN enCmdValue, BOOL bIsResend);
list_t *pt_need_send_node_list_search(list_t *pstHead, U32 ulStreamID);

VOID pt_log_set_level(U32 ulLevel);
U32 pt_log_current_level();
VOID pt_logr_write(U32 ulLevel, S8 *pszFormat, ...);
S8 *pt_get_gmt_time(U32 ulExpiresTime);
BOOL pt_is_or_not_ip(S8 *szIp);
BOOL pts_is_ptc_sn(S8* pcUrl);
BOOL pts_is_int(S8* pcUrl);
S32 pt_DNS_analyze(S8 *szDomainName, U8 paucIPAddr[IPV6_SIZE]);
S32 pt_md5_encrypt(S8 *szEncrypt, S8 *szDecrypt);
U16 load_calc_crc(U8* pBuffer, U32 ulCount);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif
