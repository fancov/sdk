#ifndef __IPCC_MSG_H__
#define __IPCC_MSG_H__

#ifdef  __cplusplus
extern "C"{
#endif


#include <ptc_pts/msg_public.h>

#define PTC_DATA_BUFF_512        512
#define PTC_DATA_BUFF_1024       1024
#define PTC_LOGIN_VERIFY_SIZE    10
#define PTC_LOGIN_VERIFY_SIZE    10

typedef struct tagServMsg
{
   U32                      achServIP[PT_IP_SIZE];
   U32                      achProxyIP[PT_IP_SIZE];
   U16                      usServPort;
   U16                      usProxyPort;
}PTC_SERV_MSG_ST;

extern PT_CC_CB_ST      *g_stCCCBSend;
extern PT_CC_CB_ST      *g_stCCCBRecv;
extern PTC_SERV_MSG_ST  g_stServMsg;
extern pthread_cond_t   g_cond_recv;
extern pthread_mutex_t  g_mutex_recv;
extern PT_DATA_TYPE_EN  g_enDataTypeRecv;
extern U32              g_ulStreamIDRecv;

void ptc_ipcc_send_msg(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen, U8 ExitNotifyFlag);
void *ptc_send_msg2amc(void *arg);
void *ptc_recv_msg_from_amc(void *arg);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif