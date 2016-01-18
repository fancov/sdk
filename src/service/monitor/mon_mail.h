#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifndef __MON_MAIL_H__
#define __MON_MAIL_H__

#include <pthread.h>

#define MON_MAIL_SERV_COUNT     1
#define MON_MAIL_BUFF_64        64
#define MON_MAIL_IP_SIZE        16       /* IP地址 */
#define MON_MAIL_QUE_COUNT      50       /* 每个级别的邮箱队列的个数 */
#define MON_MAIL_WAIT_TIMEOUT   5        /* 邮件等待发送结果的超时时间 */
#define MON_MAIL_BSS_MAIL       "kobe@dipcc.com"

typedef enum tagMonWarningType
{
    MON_WARNING_TYPE_MAIL = 0,      /* 邮件告警 */
    MON_WARNING_TYPE_SMS,           /* 短信告警 */
    MON_WARNING_TYPE_VOICE,         /* 语音告警 */

    MON_WARNING_TYPE_BUTT
}MON_WARNING_TYPE_EN;

typedef enum tagMonWarningLevel
{
    MON_WARNING_LEVEL_CRITICAL = 0,     /* 严重告警 */
    MON_WARNING_LEVEL_MAJOR,            /* 重要告警 */
    MON_WARNING_LEVEL_MINOR,            /* 次要告警 */

    MON_WARNING_LEVEL_BUTT
}MON_WARNING_LEVEL_EN;

typedef enum tagMonWarnMailOperate
{
    MON_WARN_MAIL_HELO = 0,
    MON_WARN_MAIL_AUTH,
    MON_WARN_MAIL_USERNAME,
    MON_WARN_MAIL_PASSWORD,
    MON_WARN_MAIL_FROM,
    MON_WARN_MAIL_RCPT_TO,
    MON_WARN_MAIL_DATA,
    MON_WARN_MAIL_QUIT,
    MON_WARN_MAIL_BUTT

}MON_WARN_MAIL_OPERATE_EN;


typedef enum tagMonSMTPErrno
{
    MON_SMTP_ERRNO_211 = 211,   /* 系统状态或系统帮助响应  */
    MON_SMTP_ERRNO_214 = 214,   /* 帮助信息  */
    MON_SMTP_ERRNO_220 = 220,   /* 服务就绪  */
    MON_SMTP_ERRNO_221 = 221,   /* 服务关闭传输信道  */
    MON_SMTP_ERRNO_235 = 235,   /* 用户验证成功  */
    MON_SMTP_ERRNO_250 = 250,   /* 要求的邮件操作完成  */
    MON_SMTP_ERRNO_251 = 251,   /* 用户非本地，将转发向  */
    MON_SMTP_ERRNO_334 = 334,   /* 等待用户输入验证信息*/
    MON_SMTP_ERRNO_354 = 354,   /* 开始邮件输入，以.结束  */
    MON_SMTP_ERRNO_421 = 421,   /* 服务未就绪，关闭传输信道（当必须关闭时，此应答可以作为对任何命令的响应）  */
    MON_SMTP_ERRNO_450 = 450,   /* 要求的邮件操作未完成，邮箱不可用（例如，邮箱忙）  */
    MON_SMTP_ERRNO_451 = 451,   /* 放弃要求的操作；处理过程中出错  */
    MON_SMTP_ERRNO_452 = 452,   /* 系统存储不足，要求的操作未执行  */
    MON_SMTP_ERRNO_500 = 500,   /* 格式错误，命令不可识别(此错误也包括命令行过长)*/
    MON_SMTP_ERRNO_501 = 501,   /* 参数格式错误  */
    MON_SMTP_ERRNO_502 = 502,   /* 命令不可实现  */
    MON_SMTP_ERRNO_503 = 503,   /* 错误的命令序列  */
    MON_SMTP_ERRNO_504 = 504,   /* 命令参数不可实现  */
    MON_SMTP_ERRNO_535 = 535,   /* 用户验证失败  */
    MON_SMTP_ERRNO_550 = 550,   /* 要求的邮件操作未完成，邮箱不可用（例如，邮箱未找到，或不可访问）  */
    MON_SMTP_ERRNO_551 = 551,   /* 用户非本地，请尝试  */
    MON_SMTP_ERRNO_552 = 552,   /* 过量的存储分配，要求的操作未执行  */
    MON_SMTP_ERRNO_553 = 553,   /* 邮箱名不可用，要求的操作未执行（例如邮箱格式错误）  */
    MON_SMTP_ERRNO_554 = 554,   /* 操作失败  */

    MON_SMTP_ERRNO_BUTT

}MON_SMTP_ERRNO_EN;

/** 告警消息的结构体
*/
typedef struct tagMonWarningMsg
{
    U8  ucWarningType;          /* 告警的类型 MON_WARNING_TYPE_EN */
    U32 ulWarningLevel;         /* 告警级别 MON_WARNING_LEVEL_EN */
    S8  szEmail[32];            /* 邮件地址，该地址需开通SMTP服务，方可发邮件 */
    S8  szTelNo[32];            /* 推送消息的手机号码 */
    S8  szTitle[128];           /* 告警主题 */
    S8  szMessage[512];         /* 告警内容 */
}MON_WARNING_MSG_ST;

/** 存储告警消息的数组的结构体
*/
typedef struct tagMonWarning
{
    BOOL                    bIsValid;               /* 是否有效 */
    MON_WARNING_MSG_ST      *pstWarningMsg;         /* 告警消息结构体 */

}MON_WARNING_ST;

typedef struct tagMonWarningCB
{
    MON_WARNING_LEVEL_EN        enWarningLevel;            /* 告警级别 */
    U32                         ulWarningCount;            /* 链表中告警的数量 */
    MON_WARNING_ST              *pstWarningList;           /* 告警链表 */
    pthread_mutex_t             stMutex;

}MON_WARNING_CB_ST;

typedef struct tagMonMailGlobal
{
    BOOL                        bIsValid;
    S8                          szMailServ[MON_MAIL_BUFF_64];
    S8                          szUsername[MON_MAIL_BUFF_64];
    S8                          szUsernameBase64[MON_MAIL_BUFF_64];
    S8                          szPasswd[MON_MAIL_BUFF_64];
    S8                          szPasswdBase64[MON_MAIL_BUFF_64];
    S8                          szMailIP[MON_MAIL_IP_SIZE];
    U32                         ulMailPort;
    S32                         lMonMailSockfd;
    MON_WARN_MAIL_OPERATE_EN    enOperate;                              /* 操作 */
    U32                         ulErrno;                                /* 操作结果 */
    pthread_t                   pthreadMailSend;
    sem_t                       stSem;

}MON_MAIL_GLOBAL_ST;

U32 mon_mail_init();
U32 mon_mail_start();
U32 mon_mail_send_warning(MON_WARNING_TYPE_EN enWarnType, MON_WARNING_LEVEL_EN enWarnLevel, S8 *szAddr, S8 *szTitle, S8 *szMessage);

#endif /* end of __SC_LOG_DIGEST_H__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */



