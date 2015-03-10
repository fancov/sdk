#ifndef __PTC_WEB_H__
#define __PTC_WEB_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <dos/dos_types.h>

VOID *ptc_recv_msg_from_web(VOID *arg);
VOID ptc_send_msg2web(PT_NEND_RECV_NODE_ST *pstNeedRecvNode);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */


#endif
