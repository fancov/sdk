#ifndef __PT_MSG_H__
#define __PT_MSG_H__

#ifdef  __cplusplus
extern "C"{
#endif

#define PTC_ID_LEN          16       /* PTC ID 长度 */
#define IPV6_SIZE           16       /* IPV6 字节数 */

typedef enum
{
    PT_CMD_NORMAL = 0,          /* 正常数据 */
    PT_CMD_RESEND,              /* 重传 */
    PT_CMD_CONFIRM,             /* 确认接收 */
    PT_CMD_BUTT

}PT_CMD_EN;

typedef struct tagMsg
{
    U32                     ulStreamID;
    S32                     lSeq;                            /* 数据包编号 */
    U8                      aucID[PTC_ID_LEN];               /* PTC ID */
    U8                      aulServIp[IPV6_SIZE];

    U16                     usServPort;
    U8                      enDataType;                      /* 消息的类型 PT_DATA_TYPE_EN */
    U8                      ExitNotifyFlag;                  /* 通知 stream 结束 */

    U8                      enCmdValue;                      /* PT_CMD_EN */
    U8                      bIsEncrypt;                      /* 是否加密 */
    U8                      bIsCompress;                     /* 是否压缩 */
    S8                      Reserver[1];
}PT_MSG_TAG;

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif