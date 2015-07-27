/*
  *      (C)Copyright 2014,dipcc,CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *       notification.h
  *       Created on:2014-12-07
  *           Author:bubble
  *             Todo:define notification method
  *          History:
  */
#include <dos/dos_types.h>
#include "mon_pub.h"

#ifndef _NOTIFICATION_H__
#define _NOTIFICATION_H__

/* 该结构用来定义的消息
 * 特别注意:*
 *     为了将外部的告警消息与系统监控告警消息区别开来，
 *     这里的ulWarningID必须是0x07000000与MON_NOTIFY_TYPE_EN
 *     中的某一个值做位或运算的结果.
 *     消息的序列号可不填充数值，由心跳server端统一来维护
 */
typedef struct tagMonNotifyMsg
{
    U32     ulWarningID;         /* 告警ID */
    U32     ulSeq;               /* 消息的序列号 */
    U32     ulLevel;             /* 告警级别 */
    U32     ulDestCustomerID;    /* 目的客户ID */
    U32     ulDestRoleID;        /* 目的角色ID */
    time_t  ulCurTime;           /* 消息发送时间 */
    S8      szContact[32];       /* 联系方式 */
    S8      szContent[256];      /* 消息内容 */
}MON_NOTIFY_MSG_ST;


typedef struct tagMonMsgMap
{
    S8   *pszName;   /* 配置项名称 */
    S8   *pszTitle;  /* 告警标题 */
    S8   *pszDesc;   /* 告警描述 */
}MON_MSG_MAP_ST;


typedef struct tagMonNotifyMeansCB
{
    MON_NOTIFY_MEANS_EN  ulMeans;         /* 告警方式 */
    U32 (*callback)(S8*, S8*, S8*);       /* 回调函数 */
}MON_NOTIFY_MEANS_CB_ST;


#if INCLUDE_RES_MONITOR

U32 mon_send_sms(S8 * pszMsg, S8* pszTitle,S8 * pszTelNo);
U32 mon_send_audio(S8 * pszMsg, S8* pszTitle, S8 * pszTelNo);
U32 mon_send_email(S8* pszMsg, S8* pszTitle, S8* pszEmailAddress);

#endif //#if INCLUDE_RES_MONITOR
#endif // end _NOTIFICATION_H__
