#ifndef __AMC_MSG_H__
#define __AMC_MSG_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <ptc_pts/msg_public.h>

#define PTS_DATA_BUFF_512        512
#define PTS_DATA_BUFF_1024       1024
#define PTS_SQL_STR_SIZE         512
#define PTS_URL_SIZE             128
#define PTS_COOKIE_SIZE          512
#define PTS_LISTEN_COUNT         10
#define PTS_LOGIN_VERIFY_SIZE    10

typedef struct tagClientCB
{
    list_t                  stList;
    U32                     ulStreamID;                         /*stream ID*/
    S32                     lSocket;                            /*client sockfd*/
    U8                      aucId[IPCC_ID_LEN];                 /*IPCC ID*/
    struct sockaddr_in      stClientAddr;                       /*IPCC addr*/
    BOOL                    eSaveHeadFlag;                      /*是否保存有http头*/
    S8 *                    pcRequestHead;                      /*不完整的http头*/
    S32                     lReqLen;
    S8                      Reserver[8];
}PTS_CLIENT_CB_ST;

typedef struct tagServMsg
{
   U32                      achChannelServIP[PT_IP_SIZE];
   U32                      achWebServIP[PT_IP_SIZE];
   U16                      usChannelServPort;
   U16                      usWebServPort;
}PTS_SERV_MSG_ST;

extern list_t   *g_stCCCBSend;
extern list_t   *g_stCCCBRecv;
extern PTS_SERV_MSG_ST  g_stServMsg;
extern pthread_cond_t   g_cond_recv;
extern pthread_mutex_t  g_mutex_recv;
extern PT_DATA_TYPE_EN  g_enDataTypeRecv;
extern U32              g_ulStreamIDRecv;
extern U8               g_uIpccIDRecv[IPCC_ID_LEN];

void *pts_recv_msg_from_ipcc(void *arg);
void *pts_send_msg2ipcc(void *arg);
void pts_amc_send_msg(U8 *pcIpccId, PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen, U8 ExitNotifyFlag);
PT_CC_CB_ST *pts_CClist_search(list_t* pstHead, U8 *pucID);



#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif