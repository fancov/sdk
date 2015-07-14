#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_warning_msg_queue.h"
#include "mon_def.h"

static MON_MSG_QUEUE_S * g_pstMsgQueue;

/**
 * 功能:初始化告警消息队列
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_init_warning_msg_queue()
{
    g_pstMsgQueue = (MON_MSG_QUEUE_S *)dos_dmem_alloc(sizeof(MON_MSG_QUEUE_S));
    if (!g_pstMsgQueue)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    g_pstMsgQueue->ulQueueLength = 0;
    g_pstMsgQueue->pstHead = NULL;
    g_pstMsgQueue->pstRear = NULL;

    return DOS_SUCC;
}

/**
 * 功能:消息入队
 * 参数集：
 *   参数1: MON_MSG_S * pstMsg 入队的消息对象地址
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_warning_msg_en_queue(MON_MSG_S * pstMsg)
{
    if (!pstMsg)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (0 == g_pstMsgQueue->ulQueueLength)//如果队列为空，则此节点为头结点
    {
        pstMsg->next = NULL;
        pstMsg->prior = NULL;
        g_pstMsgQueue->pstHead = pstMsg;
        g_pstMsgQueue->pstRear = pstMsg;
    }
    else
    {//如果队列非空，则在队列尾部添加元素并同时维护队尾指针和队列长度
        pstMsg->next = NULL;
        pstMsg->prior = g_pstMsgQueue->pstRear;
        g_pstMsgQueue->pstRear->next = pstMsg;
        g_pstMsgQueue->pstRear = pstMsg;
    }
    ++(g_pstMsgQueue->ulQueueLength);

    return DOS_SUCC;
}


/**
 * 功能:消息出队
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_warning_msg_de_queue()
{
    if (0 == g_pstMsgQueue->ulQueueLength)
    {//队列为空
        mon_trace(MON_TRACE_WARNING_MSG, LOG_LEVEL_INFO, "The Msg Queue is Empty.");
        return DOS_FAIL;
    }
    else
    {//队列非空，从队头删除数据，并维护队列的头结点指针和队列长度
        MON_MSG_S * p = g_pstMsgQueue->pstHead;
        g_pstMsgQueue->pstHead = g_pstMsgQueue->pstHead->next;
        if (g_pstMsgQueue->pstHead)
        {
            g_pstMsgQueue->pstHead->prior = NULL;
        }
        p->next = NULL;
        dos_dmem_free(p);
        p = NULL;
    }
    --g_pstMsgQueue->ulQueueLength;

    return DOS_SUCC;
}

BOOL mon_is_warning_msg_queue_empty()
{
    if (g_pstMsgQueue->ulQueueLength == 0)
    {
        return DOS_TRUE;
    }
    else
    {
        return DOS_FALSE;
    }
}

/**
 * 功能:获取告警消息队列的队头地址
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回告警消息队列队头指针，失败返回NULL
 */
MON_MSG_QUEUE_S * mon_get_warning_msg_queue()
{
    return g_pstMsgQueue;
}

/**
 * 功能:销毁告警消息队列
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_destroy_warning_msg_queue()
{
    U32 ulRet = 0;

    if(!g_pstMsgQueue)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    if(!(g_pstMsgQueue->pstHead))
    {
        mon_trace(MON_TRACE_WARNING_MSG, LOG_LEVEL_ERROR, "Msg Queue\'s head is empty.");
        dos_dmem_free(g_pstMsgQueue);
        return DOS_FAIL;
    }

    while(g_pstMsgQueue->pstHead)
    {
        ulRet = mon_warning_msg_de_queue();
        if(DOS_SUCC != ulRet)
        {
            mon_trace(MON_TRACE_WARNING_MSG, LOG_LEVEL_ERROR, "Warning Msg DeQueue FAIL.");
            return DOS_FAIL;
        }
    }
    dos_dmem_free(g_pstMsgQueue);
    g_pstMsgQueue = NULL;

    return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif /* __cplusplus */


