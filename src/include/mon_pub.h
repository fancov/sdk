/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        mon_pub.h
  *        Created on: 2014-12-29
  *        Author: bubble
  *        Todo: Monitoring CPU resources
  *        History:
  */

#ifndef _MON_PUB_H__
#define _MON_PUB_H__


typedef enum tagMonNotifyLevel
{
    MON_NOTIFY_LEVEL_EMERG = 0,  /* 紧急告警 */
    MON_NOTIFY_LEVEL_CRITI,      /* 重要告警 */
    MON_NOTIFY_LEVEL_MINOR,      /* 次要告警 */
    MON_NOTIFY_LEVEL_HINT,       /* 提示告警 */

    MON_NOTIFY_LEVEL_BUTT = 31   /* 无效告警 */
}MON_NOTIFY_LEVEL_EN;

/* 特别注意: 该枚举的定义顺序必须和全局变量
 * m_pstMsgMapCN以及m_pstMsgMapEN
 * 的顺序完全对应，否则会出错
 */
typedef enum tagMonNotifyType
{
    MON_NOTIFY_TYPE_LACK_FEE = 0,  /* 账户余额不足 */
    MON_MOTIFY_TYPE_LACK_GW,       /* 中继不够用 */
    MON_NOTIFY_TYPE_LACK_ROUTE,    /* 缺少路由 */

    MON_NOTIFY_TYPE_BUTT = 31     /* 无效值 */
}MON_NOTIFY_TYPE_EN;

typedef enum tagMonNofityMeans
{
    MON_NOTIFY_MEANS_EMAIL = 0,   /* 邮件 */
    MON_NOTIFY_MEANS_SMS,         /* 短信 */
    MON_NOTIFY_MEANS_AUDIO        /* 语音 */
}MON_NOTIFY_MEANS_EN;


#endif //_MON_PUB_H__
