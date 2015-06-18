#ifndef __PTC_WEB_H__
#define __PTC_WEB_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <dos/dos_types.h>

#define PTC_LOCK_MIN_COUT   3

typedef struct tagRecvFromWebMsgHandle
{
    list_t                  stList;
    S8 *                    paRecvBuff;
    S32                     lRecvLen;
    U32                     ulStreamID;
    S32                     lSocket;

}PTC_WEB_REV_MSG_HANDLE_ST;

extern U32 g_ulWebSocketMax;
extern fd_set g_stWebFdSet;
extern S32 g_lPipeWRFd;

VOID *ptc_recv_msg_from_web(VOID *arg);
VOID ptc_send_msg2web(PT_NEND_RECV_NODE_ST *pstNeedRecvNode);
void *ptc_handle_recvfrom_web_msg(void *arg);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */


#endif

