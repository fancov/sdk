#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_warning_msg_queue.h"

static MON_MSG_QUEUE_S * pstMsgQueue;


S32 mon_init_warning_msg_queue()
{
   pstMsgQueue = (MON_MSG_QUEUE_S *)dos_dmem_alloc(sizeof(MON_MSG_QUEUE_S));
   if (!pstMsgQueue)
   {
      logr_cirt("%s:Line %d:mon_init_warning_msg_queue|initialize msg queue failure,pstMsgQueue is %p!"
                , __FILE__, __LINE__, pstMsgQueue);
      return DOS_FAIL;
   }

   pstMsgQueue->lQueueLength = 0;
   pstMsgQueue->pstHead = NULL;
   pstMsgQueue->pstRear = NULL;

   return DOS_SUCC;  
}

S32 mon_warning_msg_en_queue(MON_MSG_S * pstMsg)
{
   
   if (!pstMsg)
   {
      logr_cirt("%s:Line %d:mon_warning_msg_en_queue|msg enter queue failure,pstMsg is %p!"
                , __FILE__, __LINE__, pstMsg);
      return DOS_FAIL;
   }

   if (0 == pstMsgQueue->lQueueLength)//如果队列为空，则此节点为头结点
   {
      pstMsg->next = NULL;
      pstMsg->prior = NULL;
      pstMsgQueue->pstHead = pstMsg;
      pstMsgQueue->pstRear = pstMsg;
   }
   else
   {//如果队列非空，则在队列尾部添加元素并同时维护队尾指针和队列长度
      pstMsg->next = NULL;
      pstMsg->prior = pstMsgQueue->pstRear;
      pstMsgQueue->pstRear->next = pstMsg;
      pstMsgQueue->pstRear = pstMsg;
   }
   ++(pstMsgQueue->lQueueLength);
 
   return DOS_SUCC;
}

S32 mon_warning_msg_de_queue()
{
   if (0 == pstMsgQueue->lQueueLength)
   {//队列为空
      logr_notice("%s:Line %d:mon_warning_msg_de_queue|msg delete queue failure msg queue is null"
                    , __FILE__, __LINE__);
      return DOS_FAIL;
   }
   else
   {//队列非空，从队头删除数据，并维护队列的头结点指针和队列长度
      MON_MSG_S * p = pstMsgQueue->pstHead;
      pstMsgQueue->pstHead = pstMsgQueue->pstHead->next;
      if (pstMsgQueue->pstHead)
      {
         pstMsgQueue->pstHead->prior = NULL;
      }
      p->next = NULL;
      dos_dmem_free(p);
      p = NULL;
   }
   --(pstMsgQueue->lQueueLength);

   return DOS_SUCC;
}

BOOL mon_is_warning_msg_queue_empty()
{
   if (pstMsgQueue->lQueueLength == 0)
   {
      return DOS_TRUE;
   }
   else
   {
      return DOS_FALSE;
   } 
}

MON_MSG_QUEUE_S * mon_get_warning_msg_queue()
{
   return pstMsgQueue;
}

S32 mon_destroy_warning_msg_queue()
{
   S32 lRet = 0;

   if(!pstMsgQueue)
   {
      logr_cirt("%s:Line %d:mon_destroy_warning_msg_queue|destroy warning msg queue failure,pstMsgQueue is %p!"
                , __FILE__, __LINE__, pstMsgQueue);
      return DOS_FAIL;
   }
   if(!(pstMsgQueue->pstHead))
   {
      logr_cirt("%s:Line %d:mon_destroy_warning_msg_queue|destroy warning msg queue failure,pstMsgQueue->pstHead is %p!"
                , __FILE__, __LINE__, pstMsgQueue->pstHead);
      dos_dmem_free(pstMsgQueue);
      return DOS_FAIL;
   }

   while(pstMsgQueue->pstHead)
   {
      lRet = mon_warning_msg_de_queue();
      if(DOS_SUCC != lRet)
      {
         logr_error("%s:Line %d:mon_destroy_warning_msg_queue|destroy warning msg queue failure,lRet is %d!"
                    , __FILE__, __LINE__, lRet);
         return DOS_FAIL;
      }
   }
   dos_dmem_free(pstMsgQueue);
   pstMsgQueue = NULL;
   
   return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif /* __cplusplus */


