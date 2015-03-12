#ifndef __PTS_TELNET_H__
#define __PTS_TELNET_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <dos/dos_types.h>
#include <pt/pt.h>
#include "pt/pts.h"

#define MAX_KEY_WORDS               16
#define MAX_CMD_LENGTH              512     /* 定义命令最大长度 */
#define PTS_MAX_CLIENT_NUMBER       8
#define PTS_GET_PASSW_SUCC          0       /* 获取密码成功 */
#define PTS_GET_PASSW_FAIL          -1      /* 获取密码失败 */
#define PTS_USER_NO_EXITED          -2      /* 用户不存 */
#define PTS_PASSWORD_SIZE           128
#define PTS_MAX_PORT                65535
#define PTS_TELNET_DISCONNECT_TIME  2000
#define PTS_TELNET_CR_TIME          50

typedef enum
{
    PT_TELNET_FAIL = -1,          /* 控制 */
    PT_TELNET_LIST = 0,           /* web */
    PT_TELNET_CONNECT,            /* 命令行 */
    PT_TELNET_BUTT

}PT_TELNET_CMD_ANALYSE;

typedef struct tagCmdClientCB
{
    U8     aucID[PTC_ID_LEN];   /*IPCC ID*/
    U32    ulStreamID;          /*stream ID*/
    S32    lSocket;             /*client sockfd*/
    BOOL   bIsValid;
    S8     Reserver[3];
}PTS_CMD_CLIENT_CB_ST;

S32 pts_server_cmd_analyse(U32 ulClientIndex, U32 ulMode, S8 *szBuffer, U32 ulLength);
VOID pts_telnet_send_msg2ptc(U32 ulClientIndex, S8 *szBuff, U32 ulLen);
VOID pts_send_msg2cmd(PT_NEND_RECV_NODE_ST *pstNeedRecvNode);
S32 pts_get_password(S8 *szUserName, S8 *pcPassWord, S32 lPasswLen);
VOID pts_cmd_client_delete(U32 ulClientIndex);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */


#endif

