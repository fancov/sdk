/*
  *      (C)Copyright 2014,dipcc,CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *       monitor.h
  *       Created on:2014-12-09
  *           Author:bubble
  *             Todo:define message queue operations
  *          History:
  */


#ifndef _MON_WARNING_MSG_QUEUE_H__
#define _MON_WARNING_MSG_QUEUE_H__

#if INCLUDE_RES_MONITOR  


#include <dos/dos_types.h>

typedef struct tagMessage
{
   U32     ulWarningId; //告警id
   S32     lMsgLen;    //消息长度
   VOID *  msg;        //消息内容
   struct  tagMessage * next; //后继结点
   struct  tagMessage * prior;//前驱节点
}MON_MSG_S;

typedef struct tagMsgQueue
{
   MON_MSG_S * pstHead;//队列的队头指针
   MON_MSG_S * pstRear;//队列的队尾指针
   S32 lQueueLength;   //队列的长度
}MON_MSG_QUEUE_S;


S32  mon_init_warning_msg_queue();
S32  mon_warning_msg_en_queue(MON_MSG_S * pstMsg);
S32  mon_warning_msg_de_queue();
BOOL mon_is_warning_msg_queue_empty();
MON_MSG_QUEUE_S * mon_get_warning_msg_queue();
S32  mon_destroy_warning_msg_queue();

#endif //#if INCLUDE_RES_MONITOR  
#endif //end _MON_WARNING_MSG_QUEUE_H__

