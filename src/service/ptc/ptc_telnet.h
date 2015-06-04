#ifndef __PTC_TELNET_H__
#define __PTC_TELNET_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <dos/dos_types.h>

typedef struct tagPtcRecvCmd
{
    U32 ulIndex;
    S8 szCommand[PT_DATA_BUFF_16];
    BOOL bIsValid;

}PT_PTC_RECV_CMD_ST;

extern U32 g_ulCmdSocketMax;
extern fd_set g_stCmdFdSet;
//VOID *ptc_recv_msg_from_web(VOID *arg);
VOID ptc_send_msg2cmd(PT_NEND_RECV_NODE_ST *pstNeedRecvNode);
VOID *ptc_recv_msg_from_cmd(VOID *arg);
void *ptc_deal_with_pts_command(void *arg);
VOID ptc_cmd_delete_client_by_stream(U32 ulStreamID);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */


#endif
