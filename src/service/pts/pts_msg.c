#ifdef  __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <limits.h>
#include <semaphore.h>
#include <sys/time.h>

#include <pt/dos_sqlite3.h>
#include <pt/pts.h>
#include "pts_msg.h"

sem_t     g_SemPts;                         /* 发送信号量 */
sem_t     g_SemPtsRecv;                     /* 接收信号量 */
S32       g_lSeqSend           = 0;         /* 发送的包编号 */
S32       g_lResendSeq         = 0;         /* 需要重新接收的包编号 */
list_t   *g_pstPtcListSend     = NULL;      /* 发送缓存 */
list_t   *g_pstPtcListRecv     = NULL;      /* 接收缓存 */
list_t   *g_pstPtsNendRecvNode = NULL;      /* pts需要接收的数据 */
list_t   *g_pstPtsNendSendNode = NULL;      /* pts需要发送的数据 */
static S32 g_lPtsUdpSocket     = 0;         /* UDP通道的套接字 */
PTS_SERV_MSG_ST g_stPtsMsg;                 /* 存放从配置文件中读取到的pts的信息 */
pthread_mutex_t g_pts_mutex_send  = PTHREAD_MUTEX_INITIALIZER;      /* 发送线程锁 */
pthread_cond_t  g_pts_cond_send   = PTHREAD_COND_INITIALIZER;       /* 发送条件变量 */
pthread_mutex_t g_pts_mutex_recv  = PTHREAD_MUTEX_INITIALIZER;      /* 接收线程锁 */
pthread_cond_t  g_pts_cond_recv   = PTHREAD_COND_INITIALIZER;       /* 接收条件变量 */
PTS_SERV_SOCKET_ST g_lPtsServSocket[PTS_WEB_SERVER_MAX_SIZE];

S8 *pts_get_current_time()
{
    time_t ulCurrTime = 0;

    ulCurrTime = time(NULL);
    return ctime(&ulCurrTime);
}

VOID pts_send_pthread_mutex_lock(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : send\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
    pthread_mutex_lock(&g_pts_mutex_send);
    printf("%s %d %.*s : send lock\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
}

VOID pts_send_pthread_mutex_unlock(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : send unlock\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
    pthread_mutex_unlock(&g_pts_mutex_send);
}

VOID pts_send_pthread_cond_wait(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : send wait\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
    pthread_cond_wait(&g_pts_cond_send, &g_pts_mutex_send);
    printf("%s %d %.*s : send wait in\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
}

VOID pts_recv_pthread_mutex_lock(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : recv\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
    pthread_mutex_lock(&g_pts_mutex_recv);
    printf("%s %d %.*s : recv lock\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
}

VOID pts_recv_pthread_mutex_unlock(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : recv unlock\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
    pthread_mutex_unlock(&g_pts_mutex_recv);
}

VOID pts_recv_pthread_cond_timedwait(struct timespec *timeout, S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : recv wait\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
    pthread_cond_timedwait(&g_pts_cond_recv, &g_pts_mutex_recv, timeout);
    printf("%s %d %.*s : recv wait lock\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
}

/**
 * 函数：S32 pts_find_ptc_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
 * 功能：
 *      1.数据库查询回调函数
 * 参数
 * 返回值：
 */
S32 pts_find_ptc_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
{
    S8 *pcDestID = (S8 *)para;
    dos_strncpy(pcDestID, column_value[0], PTC_ID_LEN);
    pcDestID[PTC_ID_LEN] = '\0';

    return 0;
}

/**
 * 函数：S32 pts_find_ptc_by_dest_addr(S8 *pDestInternetIp, S8 *pDestIntranetIp, S8 *pcDestSN)
 * 功能：
 *      1.根据公、私网ip地址，查找对应的ptc
 * 参数
 *      S8 *pDestInternetIp : 公网IP地址
 *      S8 *pDestIntranetIp : 私网ip地址
 *      S8 *pcDestSN  : 用来存放查找到的ptc的sn
 * 返回值：
 */
S32 pts_find_ptc_by_dest_addr(S8 *pDestInternetIp, S8 *pDestIntranetIp, S8 *pcDestSN)
{
    S32 lRet = 0;
    S8 szSql[PT_DATA_BUFF_128] = {0};

    if (NULL == pDestInternetIp || NULL == pDestIntranetIp)
    {
        return DOS_FAIL;
    }

    dos_snprintf(szSql, PT_DATA_BUFF_128, "select sn from ipcc_alias where register = 1 and internetIP='%s' and intranetIP='%s'", pDestInternetIp, pDestIntranetIp);
    lRet = dos_sqlite3_exec_callback(g_pstMySqlite, szSql, pts_find_ptc_callback, (VOID *)pcDestSN);
    if (lRet != DOS_SUCC)
    {
        return DOS_FAIL;
    }

    if (dos_strlen(pcDestSN) > 0)
    {
        return DOS_SUCC;
    }
    else
    {
        dos_snprintf(szSql, PT_DATA_BUFF_128, "select sn from ipcc_alias where register = 1 and internetIP='%s'", pDestInternetIp);
        dos_sqlite3_exec_callback(g_pstMySqlite, szSql, pts_find_ptc_callback, (VOID *)pcDestSN);
        if (dos_strlen(pcDestSN) > 0)
        {
            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}

/**
 * 函数：VOID pts_data_lose(PT_MSG_TAG *pstMsgDes, S32 lShouldSeq)
 * 功能：
 *      1.发送丢包请求
 * 参数
 *      PT_MSG_TAG *pstMsgDes ：包含丢失包的描述信息
 * 返回值：无
 */
VOID pts_data_lose(PT_MSG_TAG *pstMsgDes, S32 lLoseSeq)
{
    if (NULL == pstMsgDes)
    {
        return;
    }
#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_lock(__FILE__, __LINE__);
#else
    pthread_mutex_lock(&g_pts_mutex_send);
#endif
    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_RESEND;
    pstMsgDes->lSeq = lLoseSeq;

    pt_logr_info("send lose data : stream = %d, seq = %d", pstMsgDes->ulStreamID, lLoseSeq);
    g_pstPtsNendSendNode = pt_need_send_node_list_insert(g_pstPtsNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);

    pthread_cond_signal(&g_pts_cond_send);
#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
    pthread_mutex_unlock(&g_pts_mutex_send);
#endif
}

/**
 * 函数：VOID pts_send_lost_data_req(U64 ulLoseMsg)
 * 功能：
 *      1.重传丢失的包
 * 参数
 *      U64 ulLoseMsg ：
 * 返回值：无
 */
VOID pts_send_lost_data_req(U64 ulLoseMsg)
{
    if (0 == ulLoseMsg)
    {
        return;
    }

    PT_LOSE_BAG_MSG_ST *pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)ulLoseMsg;
    PT_STREAM_CB_ST *pstStreamNode = pstLoseMsg->pstStreamNode;

    S32 i = 0;
    U32 ulCount = 0;
    U32 ulArraySub = 0;
    if (pstStreamNode == NULL)
    {
        return;
    }

    if (pstStreamNode->ulCountResend >= 3)
    {
        /* 3秒后，未收到包。关闭定时器、sockfd */
        pt_logr_error("stream resend fail, close。stream is %d", pstStreamNode->ulStreamID);
        dos_tmr_stop(&pstStreamNode->hTmrHandle);
        pstStreamNode->hTmrHandle= NULL;
        pts_delete_recv_stream_node(&pstLoseMsg->stMsg, NULL, DOS_TRUE);
        pts_delete_send_stream_node(&pstLoseMsg->stMsg, NULL, DOS_TRUE);
        return;
    }
    else
    {
        pstStreamNode->ulCountResend++;
        /* 遍历，找出丢失的包，发送重传信息 */
        for (i=pstStreamNode->lCurrSeq+1; i<pstStreamNode->lMaxSeq; i++)
        {
            ulArraySub = i & (PT_DATA_SEND_CACHE_SIZE - 1);
            if (0 == i)
            {
                /* 判断0号包是否丢失 */
                if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].ulLen == 0)
                {
                    ulCount++;
                    pts_data_lose(&pstLoseMsg->stMsg, i);
                }
            }
            else if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].lSeq != i)
            {
                ulCount++;
                /* 发送丢包重发请求 */
                pts_data_lose(&pstLoseMsg->stMsg, i);
            }
        }
    }
    pt_logr_debug("send lose data count : %d", ulCount);

    if (0 == ulCount)
    {
        #if 0
        if (pstLoseMsg->stMsg.enDataType != PT_DATA_CMD)
        {
            ulArraySub = pstStreamNode->lMaxSeq & (PT_DATA_SEND_CACHE_SIZE - 1);
            if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].ulLen == 0)
            {
                dos_tmr_stop(&pstStreamNode->hTmrHandle);
            }
            else
            {
                pstStreamNode->ulCountResend = 0;
                pts_data_lose(&pstLoseMsg->stMsg, pstStreamNode->lMaxSeq+1);
            }
        }
        else
        {
            dos_tmr_stop(&pstStreamNode->hTmrHandle);
        }
        #endif
        if (pstStreamNode->hTmrHandle != NULL)
        {
            dos_tmr_stop(&pstStreamNode->hTmrHandle);
            pstStreamNode->hTmrHandle = NULL;
        }
    }
}

/**
 * 函数：VOID pts_send_confirm_msg(PT_MSG_TAG *pstMsgDes, U32 lConfirmSeq)
 * 功能：
 *      1.发送确认接收消息
 * 参数
 *
 * 返回值：无
 */
VOID pts_send_confirm_msg(PT_MSG_TAG *pstMsgDes, U32 lConfirmSeq)
{
    if (NULL == pstMsgDes)
    {
        pt_logr_debug("arg error");
        return;
    }

    S32 i = 0;
#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_lock(__FILE__, __LINE__);
#else
    pthread_mutex_lock(&g_pts_mutex_send);
#endif
    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_CONFIRM;
    pstMsgDes->lSeq = lConfirmSeq;

    pt_logr_info("send confirm data, type = %d, stream = %d", pstMsgDes->enDataType,pstMsgDes->ulStreamID);
    for (i=0; i<PTS_SEND_CONFIRM_MSG_COUNT; i++)
    {
        g_pstPtsNendSendNode = pt_need_send_node_list_insert(g_pstPtsNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);
    }
    pthread_cond_signal(&g_pts_cond_send);
#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
    pthread_mutex_unlock(&g_pts_mutex_send);
#endif
}

/**
 * 函数：VOID pts_send_login_verify(PT_MSG_TAG *pstMsgDes)
 * 功能：
 *      1.发送登陆验证消息
 * 参数
 * 返回值：无
 */
VOID pts_send_login_verify(PT_MSG_TAG *pstMsgDes)
{
    if (NULL == pstMsgDes)
    {
        pt_logr_debug("arg error");
        return;
    }

#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_lock(__FILE__, __LINE__);
#else
    pthread_mutex_lock(&g_pts_mutex_send);
#endif
    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_NORMAL;

    if (NULL == pt_need_send_node_list_search(g_pstPtsNendSendNode, pstMsgDes->ulStreamID))
    {
        pstMsgDes->ulStreamID = PT_CTRL_LOGIN_RSP;
        g_pstPtsNendSendNode = pt_need_send_node_list_insert(g_pstPtsNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);
    }

    pthread_cond_signal(&g_pts_cond_send);
#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
    pthread_mutex_unlock(&g_pts_mutex_send);
#endif
}

/**
 * 函数：VOID pts_create_rand_str(S8 *szVerifyStr)
 * 功能：
 *      1.产生随机字符串
 * 参数
 * 返回值：无
 */
VOID pts_create_rand_str(S8 *szVerifyStr)
{
    U8 i = 0;
    if (NULL == szVerifyStr)
    {
        return;
    }

    srand((int)time(0));
    for(i=0; i<PT_LOGIN_VERIFY_SIZE-1; i++)
    {
        szVerifyStr[i] = 1 + (rand()&CHAR_MAX);
    }
    szVerifyStr[PT_LOGIN_VERIFY_SIZE-1] = '\0';
}

/**
 * 函数：S32 pts_key_convert(S8 *szKey, S8 *szDest, S8 *szVersion)
 * 功能：
 *      1.登陆验证
 * 参数
 *      S8 *szKey     ：原始值
 *      S8 *szDest    ：转换后的值
 *      S8 *szVersion ：ptc版本号
 * 返回值：无
 */
S32 pts_key_convert(S8 *szKey, S8 *szDest, S8 *szPtcVersion)
{
    if (NULL == szKey || NULL == szDest)
    {
        return DOS_FAIL;
    }

    S32 i = 0;

    //if (dos_strncmp(szPtcVersion, "1.1", dos_strlen("1.1")) == 0)
    //{
    /* TODO 1.1版本验证方法 */
    for (i=0; i<PT_LOGIN_VERIFY_SIZE-1; i++)
    {
        szDest[i] = szKey[i]&0xA9;
    }
    szDest[PT_LOGIN_VERIFY_SIZE] = '\0';
    //}

    return DOS_SUCC;
}

/**
 * 函数：S32 pts_get_key(PT_MSG_TAG *pstMsgDes, S8 *szKey, U32 ulLoginVerSeq)
 * 功能：
 *      1.
 * 参数
 * 返回值：无
 */
S32 pts_get_key(PT_MSG_TAG *pstMsgDes, S8 *szKey, U32 ulLoginVerSeq)
{
    if (NULL == pstMsgDes || NULL == szKey)
    {
        pt_logr_debug("pts_get_key : arg error");
        return DOS_FAIL;
    }

    list_t  *pstStreamHead          = NULL;
    PT_STREAM_CB_ST *pstStreamNode  = NULL;
    PT_CC_CB_ST     *pstCCNode      = NULL;
    PT_CTRL_DATA_ST *pstCtrlData    = NULL;
    PT_DATA_QUE_HEAD_UN  punDataList;
    U32 ulArrarySub = 0;

#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_lock(__FILE__, __LINE__);
#else
    pthread_mutex_lock(&g_pts_mutex_send);
#endif
    pstCCNode = pt_ptc_list_search(g_pstPtcListSend, pstMsgDes->aucID);
    if(NULL == pstCCNode)
    {
        pt_logr_debug("pts_get_key : not found ptc");
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
        return DOS_FAIL;
    }

    pstStreamHead = pstCCNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
    if (NULL == pstStreamHead)
    {
        pt_logr_debug("pts_get_key : not found stream list");
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
        return DOS_FAIL;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamHead, PT_CTRL_LOGIN_RSP);
    if(NULL == pstStreamNode)
    {
        pt_logr_debug("pts_get_key : not found stream node");
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
        return DOS_FAIL;
    }

    punDataList = pstStreamNode->unDataQueHead;
    if (NULL == punDataList.pstDataTcp)
    {
        pt_logr_debug("pts_get_key : data queue is NULL");
    }
    else
    {
        ulArrarySub = (ulLoginVerSeq) & (PT_DATA_SEND_CACHE_SIZE - 1);
        if (punDataList.pstDataTcp[ulArrarySub].lSeq == ulLoginVerSeq)
        {
            pstCtrlData = (PT_CTRL_DATA_ST *)punDataList.pstDataTcp[ulArrarySub].szBuff;
            dos_memcpy(szKey, pstCtrlData->szLoginVerify, PT_LOGIN_VERIFY_SIZE-1);
            szKey[PT_LOGIN_VERIFY_SIZE-1] = '\0';
#if PT_MUTEX_DEBUG
            pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
            pthread_mutex_unlock(&g_pts_mutex_send);
#endif
            return DOS_SUCC;
        }
    }
#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
    pthread_mutex_unlock(&g_pts_mutex_send);
#endif

    return DOS_FAIL;
}

/**
 * 函数：S32 pts_save_into_send_cache(PT_CC_CB_ST *pstPtcNode, PT_MSG_TAG *pstMsgDes, S8 *acSendBuf, S32 lDataLen)
 *      1.添加到发送缓存
 * 参数
 *      PT_CC_CB_ST *pstPtcNode : ptc缓存地址
 *      PT_MSG_TAG *pstMsgDes   ：头部信息
 *      S8 *acSendBuf           ：包内容
 *      S32 lDataLen            ：包内容长度
 * 返回值：DOS_SUCC 通知proxy接收消息
 *         DOS_FAIL 失败，或者丢包
 */
S32 pts_save_into_send_cache(PT_CC_CB_ST *pstPtcNode, U32 ulStreamID, PT_DATA_TYPE_EN enDataType, S8 *acSendBuf, S32 lDataLen, S8 *szDestIp, U16 usDestPort)
{
    if (NULL == pstPtcNode || NULL == acSendBuf)
    {
        return DOS_FAIL;
    }

    S32                lResult             = 0;
    list_t             *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST    *pstStreamNode      = NULL;
    PT_DATA_TCP_ST     *pstDataQueue       = NULL;

    pstStreamListHead = pstPtcNode->astDataTypes[enDataType].pstStreamQueHead;
    if (NULL == pstStreamListHead)
    {
        /* 创建stream list head */
        pstStreamNode = pt_stream_node_create(ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* stream node创建失败 */
            pt_logr_info("pts_save_into_send_cache : create stream node fail");
            return DOS_FAIL;
        }
        if (szDestIp != NULL)
        {
            inet_pton(AF_INET, szDestIp, (VOID *)(pstStreamNode->aulServIp));
        }
        if (usDestPort)
        {
            pstStreamNode->usServPort = usDestPort;
        }

        pstStreamListHead = &(pstStreamNode->stStreamListNode);
        pstPtcNode->astDataTypes[enDataType].pstStreamQueHead = pstStreamListHead;
    }
    else
    {
        pstStreamNode = pt_stream_queue_search(pstStreamListHead, ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* 创建stream node */
            pstStreamNode = pt_stream_node_create(ulStreamID);
            if (NULL == pstStreamNode)
            {
                /* stream node创建失败 */
                pt_logr_info("pts_save_into_send_cache : create stream node fail");
                return DOS_FAIL;
            }
            if (szDestIp != NULL)
            {
                pstStreamNode->usServPort = usDestPort;
                inet_pton(AF_INET, szDestIp, (VOID *)(pstStreamNode->aulServIp));
            }
            /* 插入 stream list中 */
            pstStreamListHead = pt_stream_queue_insert(pstStreamListHead, &(pstStreamNode->stStreamListNode));
        }
    }

    pstDataQueue = pstStreamNode->unDataQueHead.pstDataTcp;
    if (NULL == pstDataQueue)
    {
        /* 创建tcp data queue */
        pstDataQueue = pt_data_tcp_queue_create(PT_DATA_RECV_CACHE_SIZE);
        if (NULL == pstDataQueue)
        {
            /* 创建data queue失败*/
            pt_logr_info("pts_save_into_send_cache : create data queue fail");
            return DOS_FAIL;
        }

        pstStreamNode->unDataQueHead.pstDataTcp = pstDataQueue;
    }

    /*将数据插入到data queue中*/
    lResult = pt_send_data_tcp_queue_insert(pstStreamNode, acSendBuf, lDataLen, PT_DATA_SEND_CACHE_SIZE);
    if (lResult < 0)
    {
        pt_logr_info("pts_save_into_send_cache : add data into send cache fail");
        return DOS_FAIL;
    }

    return lResult;
}

/**
 * 函数：S32 pts_save_into_recv_cache(PT_CC_CB_ST *pstPtcNode, PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen)
 *      1.添加到发送缓存
 * 参数
 *      PT_CC_CB_ST *pstPtcNode : ptc缓存地址
 *      PT_MSG_TAG *pstMsgDes   ：头部信息
 *      S8 *acRecvBuf           ：包内容
 *      S32 lDataLen            ：包内容长度
 * 返回值：DOS_SUCC 通知proxy接收消息
 *         DOS_FAIL 失败，或者丢包
 */
S32 pts_save_into_recv_cache(PT_CC_CB_ST *pstPtcNode, PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen)
{
    if (NULL == pstPtcNode || NULL == pstMsgDes || NULL == acRecvBuf)
    {
        return PT_SAVE_DATA_FAIL;
    }

    S32 i = 0;
    S32 lResult = 0;
    U32 ulNextSendArraySub = 0;
    U32 ulArraySub = 0;
    list_t             *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST    *pstStreamNode      = NULL;
    PT_LOSE_BAG_MSG_ST *pstLoseMsg         = NULL;
    PT_DATA_TCP_ST     *pstDataQueue       = NULL;
    PT_CC_CB_ST        *pstSendPtcNode     = NULL;

    /* 判断stream是否在发送队列中存在，若不存在，说明这个stream已经结束 */
#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_lock(__FILE__, __LINE__);
#else
    pthread_mutex_lock(&g_pts_mutex_send);
#endif
    pstSendPtcNode = pt_ptc_list_search(g_pstPtcListSend, pstMsgDes->aucID);
    if(NULL == pstSendPtcNode)
    {
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
        return PT_SAVE_DATA_FAIL;
    }
    pstStreamListHead = pstSendPtcNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
    if (NULL == pstStreamListHead)
    {
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
        return PT_SAVE_DATA_FAIL;
    }
    else
    {
        pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
        if (NULL == pstStreamNode)
        {
#if PT_MUTEX_DEBUG
            pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
            pthread_mutex_unlock(&g_pts_mutex_send);
#endif
            return PT_SAVE_DATA_FAIL;
        }
    }
#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
    pthread_mutex_unlock(&g_pts_mutex_send);
#endif
    pstStreamListHead = pstPtcNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
    if (NULL == pstStreamListHead)
    {
        /* 创建stream list head */
        pstStreamNode = pt_stream_node_create(pstMsgDes->ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* stream node创建失败 */
            pt_logr_info("pts_save_into_recv_cache : create stream node fail");
            return PT_SAVE_DATA_FAIL;
        }

        pstStreamListHead = &(pstStreamNode->stStreamListNode);
        pstPtcNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead = pstStreamListHead;
        #if 0
        if (pstMsgDes->enDataType != PT_DATA_CMD)
        {
            /* 创建定时器 */
            pt_logr_info("create timer streamID : %d",  pstMsgDes->ulStreamID);
            pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)dos_dmem_alloc(sizeof(PT_LOSE_BAG_MSG_ST));
            if (NULL == pstLoseMsg)
            {
                perror("malloc");
                return PT_SAVE_DATA_FAIL;
            }
            pstStreamNode->pstLostParam = pstLoseMsg;
            pstLoseMsg->stMsg = *pstMsgDes;
            pstLoseMsg->pstStreamNode = pstStreamNode;
            pstStreamNode->ulCountResend = 0;
            lResult = dos_tmr_start(&pstStreamNode->hTmrHandle, PT_SEND_LOSE_DATA_TIMER, pts_send_lost_data_req, (U64)pstLoseMsg, TIMER_NORMAL_LOOP);
            if (DOS_SUCC != lResult)
            {
                pt_logr_debug("pts_save_into_recv_cache : start timer fail");
                return PT_SAVE_DATA_FAIL;
            }
        }
        #endif
    }
    else
    {
        pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* 创建stream node */
            pstStreamNode = pt_stream_node_create(pstMsgDes->ulStreamID);
            if (NULL == pstStreamNode)
            {
                /* stream node创建失败 */
                pt_logr_info("pts_save_into_recv_cache : create stream node fail");
                return PT_SAVE_DATA_FAIL;
            }
            /* 插入 stream list中 */
            pstStreamListHead = pt_stream_queue_insert(pstStreamListHead, &(pstStreamNode->stStreamListNode));
            #if 0
            if (pstMsgDes->enDataType != PT_DATA_CMD)
            {
                pt_logr_info("create timer, %d streamID : %d", __LINE__, pstMsgDes->ulStreamID);
                /* 创建定时器 */
                pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)dos_dmem_alloc(sizeof(PT_LOSE_BAG_MSG_ST));
                if (NULL == pstLoseMsg)
                {
                    perror("malloc");
                    return PT_SAVE_DATA_FAIL;
                }
                pstStreamNode->pstLostParam = pstLoseMsg;
                pstLoseMsg->stMsg = *pstMsgDes;
                pstLoseMsg->pstStreamNode = pstStreamNode;
                pstStreamNode->ulCountResend = 0;
                lResult = dos_tmr_start(&pstStreamNode->hTmrHandle, PT_SEND_LOSE_DATA_TIMER, pts_send_lost_data_req, (U64)pstLoseMsg, TIMER_NORMAL_LOOP);
                if (DOS_SUCC != lResult)
                {
                    pt_logr_debug("pts_save_into_recv_cache : start timer fail");
                    return PT_SAVE_DATA_FAIL;
                }
            }
            #endif
        }
    }
    pstDataQueue = pstStreamNode->unDataQueHead.pstDataTcp;
    if (NULL == pstDataQueue)
    {
        /* 创建tcp data queue */
        pstDataQueue = pt_data_tcp_queue_create(PT_DATA_RECV_CACHE_SIZE);
        if (NULL == pstDataQueue)
        {
            /* data queue失败 */
            pt_logr_info("pts_save_into_recv_cache : create tcp data queue fail");
            return PT_SAVE_DATA_FAIL;
        }

        pstStreamNode->unDataQueHead.pstDataTcp = pstDataQueue;
    }

    /* 统计包的数量 */
    if (pstMsgDes->lSeq > pstStreamNode->lMaxSeq)
    {
        pstPtcNode->ulUdpRecvDataCount++;
        pstPtcNode->ulUdpLostDataCount += (pstMsgDes->lSeq - pstStreamNode->lMaxSeq -1);
        //printf("loss rate : %d\n", pstPtcNode->ulUdpLostDataCount*100/(pstPtcNode->ulUdpRecvDataCount+pstPtcNode->ulUdpLostDataCount));
    }

    /* 将数据插入到data queue中 */
    lResult = pt_recv_data_tcp_queue_insert(pstStreamNode, pstMsgDes, acRecvBuf, lDataLen, PT_DATA_RECV_CACHE_SIZE);
    if (lResult < 0)
    {
        pt_logr_info("pts_save_into_recv_cache : add data into recv cache fail");
        return PT_SAVE_DATA_FAIL;
    }

    /* 每64个，检查是否接收完成 */
    if (pstMsgDes->lSeq - pstStreamNode->lConfirmSeq >= PT_CONFIRM_RECV_MSG_SIZE)
    {
        i = pstStreamNode->lConfirmSeq + 1;
        for (i=0; i<PT_CONFIRM_RECV_MSG_SIZE; i++)
        {
            ulArraySub = (pstStreamNode->lConfirmSeq + 1 + i) & (PT_DATA_RECV_CACHE_SIZE - 1);  /*求余数,包存放在数组中的位置*/
            if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].lSeq != pstStreamNode->lConfirmSeq + 1 + i)
            {
                break;
            }
        }

        if (i == PT_CONFIRM_RECV_MSG_SIZE)
        {
            /* 发送确认接收消息 */
            pt_logr_info("send make sure msg : %d", pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE);
            pts_send_confirm_msg(pstMsgDes, pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE);
            pstStreamNode->lConfirmSeq = pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE;
        }
    }

    if (PT_NEED_CUT_PTHREAD == lResult)
    {
        return PT_NEED_CUT_PTHREAD;
    }
    else
    {
        /* 判断应该发送的包是否丢包 */
        if (pstStreamNode->lCurrSeq + 1 == pstMsgDes->lSeq)
        {
            if (NULL != pstStreamNode->hTmrHandle)
            {
                /* 如果存在定时器，则这个包为丢失的最小包 */
                pstStreamNode->ulCountResend = 0;
            }
            return PT_SAVE_DATA_SUCC;
        }
        else if (pstMsgDes->lSeq <= pstStreamNode->lCurrSeq)
        {
            /* 已发送过的包 */
            return PT_SAVE_DATA_FAIL;
        }
        else
        {
            if (pstStreamNode->lCurrSeq != -1)
            {
                ulNextSendArraySub = (pstStreamNode->lCurrSeq + 1) & (PT_DATA_RECV_CACHE_SIZE - 1);
                if (pstDataQueue[ulNextSendArraySub].lSeq == pstStreamNode->lCurrSeq + 1)
                {
                    return PT_SAVE_DATA_SUCC;
                }
            }
            else
            {
                if (pstMsgDes->lSeq == 0)
                {
                    return PT_SAVE_DATA_SUCC;
                }
            }

            /* 丢包，如果没有定时器的，创建定时器 */
            if (NULL == pstStreamNode->hTmrHandle)
            {
                if (pstStreamNode->pstLostParam == NULL)
                {
                    pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)dos_dmem_alloc(sizeof(PT_LOSE_BAG_MSG_ST));
                    if (NULL == pstLoseMsg)
                    {
                        perror("malloc");
                        return PT_SAVE_DATA_FAIL;
                    }
                    pstLoseMsg->stMsg = *pstMsgDes;
                    pstLoseMsg->pstStreamNode = pstStreamNode;

                    pstStreamNode->pstLostParam = pstLoseMsg;
                }
                pstStreamNode->ulCountResend = 0;
                pt_logr_info("create timer, %d streamID : %d", __LINE__, pstMsgDes->ulStreamID);
                pts_send_lost_data_req((U64)pstStreamNode->pstLostParam);
                lResult = dos_tmr_start(&pstStreamNode->hTmrHandle, PT_SEND_LOSE_DATA_TIMER, pts_send_lost_data_req, (U64)pstStreamNode->pstLostParam, TIMER_NORMAL_LOOP);
                if (PT_SAVE_DATA_FAIL == lResult)
                {
                    pt_logr_debug("pts_save_into_recv_cache : start timer fail");
                    return PT_SAVE_DATA_FAIL;
                }

            }
            return PT_SAVE_DATA_FAIL;
        }
    }
}

/**
 * 函数：S32 pts_save_login_ack_into_recv_cache(PT_CC_CB_ST *pstPtcNode, PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen)
 * 功能：
 *      1.添加登陆结果到接收缓存，通知pts修改数据库
 * 参数
 *
 * 返回值：
 */
S32 pts_save_login_ack_into_recv_cache(PT_CC_CB_ST *pstPtcNode, PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen)
{
    if (NULL == pstPtcNode || NULL == pstMsgDes || NULL == acRecvBuf)
    {
        return DOS_FAIL;
    }

    S32                lResult             = 0;
    list_t             *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST    *pstStreamNode      = NULL;
    PT_DATA_TCP_ST     *pstDataQueue       = NULL;

    pstStreamListHead = pstPtcNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
    if (NULL == pstStreamListHead)
    {
        /* 创建stream list head */
        pstStreamNode = pt_stream_node_create(pstMsgDes->ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* stream node创建失败 */
            pt_logr_info("pts_save_login_ack_into_recv_cache : create stream node fail");
            return DOS_FAIL;
        }

        pstStreamListHead = &(pstStreamNode->stStreamListNode);
        pstPtcNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead = pstStreamListHead;
    }
    else
    {
        pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* 创建stream node */
            pstStreamNode = pt_stream_node_create(pstMsgDes->ulStreamID);
            if (NULL == pstStreamNode)
            {
                /* stream node创建失败 */
                pt_logr_info("pts_save_login_ack_into_recv_cache : create stream node fail");
                return DOS_FAIL;
            }
            /* 插入 stream list中 */
            pstStreamListHead = pt_stream_queue_insert(pstStreamListHead, &(pstStreamNode->stStreamListNode));

        }
    }

    pstDataQueue = pstStreamNode->unDataQueHead.pstDataTcp;
    if (NULL == pstDataQueue)
    {
        /* 创建tcp data queue */
        pstDataQueue = pt_data_tcp_queue_create(PT_DATA_RECV_CACHE_SIZE);
        if (NULL == pstDataQueue)
        {
            /* 创建data queue失败 */
            pt_logr_info("pts_save_login_ack_into_recv_cache : create data queue fail");
            return DOS_FAIL;
        }

        pstStreamNode->unDataQueHead.pstDataTcp = pstDataQueue;
    }

    /* 将数据插入到data queue中 */
    lResult = pt_send_data_tcp_queue_insert(pstStreamNode, acRecvBuf, lDataLen, PT_DATA_RECV_CACHE_SIZE);
    if (lResult < 0)
    {
        pt_logr_info("pts_save_login_ack_into_recv_cache : add data into send cache fail");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 函数：VOID pts_handle_logout_req(PT_MSG_TAG *pstMsgDes)
 * 功能：
 *      1.处理退出登陆请求
 * 参数
 *
 * 返回值：
 */
VOID pts_handle_logout_req(PT_MSG_TAG *pstMsgDes)
{
    if (NULL == pstMsgDes)
    {
        return;
    }

    S32 lRet = 0;
    PT_CC_CB_ST *pstPtcRecvNode = NULL;
    PT_CC_CB_ST *pstPtcSendNode = NULL;
    S8 szSql[PT_DATA_BUFF_128] = {0};

    /* 退出登录，删除ptc node，通知数据修改ptc状态 */
    pstPtcRecvNode = pt_ptc_list_search(g_pstPtcListRecv, pstMsgDes->aucID);
    if (pstPtcRecvNode != NULL)
    {
        dos_tmr_stop(&pstPtcRecvNode->stHBTmrHandle);
        g_pstPtcListRecv = pt_delete_ptc_node(g_pstPtcListRecv, pstPtcRecvNode);
        pstPtcSendNode = pt_ptc_list_search(g_pstPtcListSend, pstMsgDes->aucID);
        g_pstPtcListSend = pt_delete_ptc_node(g_pstPtcListSend, pstPtcSendNode);
        /* 通知pts，修改数据库 */
        dos_snprintf(szSql, PT_DATA_BUFF_128, "update ipcc_alias set register=0 where sn='%.*s';", PTC_ID_LEN, pstMsgDes->aucID);
        lRet = dos_sqlite3_exec(g_pstMySqlite, szSql);
        if (lRet!= DOS_SUCC)
        {
            pt_logr_info("logout update db fail");
        }
    }

    return;
}

/**
 * 函数：VOID pts_handle_login_req(S32 lSockfd, PT_MSG_TAG *pstMsgDes, struct sockaddr_in stClientAddr, S8 *szPtcVersion)
 * 功能：
 *      1.处理登陆请求
 * 参数
 *
 * 返回值：
 */
VOID pts_handle_login_req(S32 lSockfd, PT_MSG_TAG *pstMsgDes, struct sockaddr_in stClientAddr, S8 *szPtcVersion)
{
    if (NULL == pstMsgDes)
    {
        return;
    }

    S8 szKey[PT_LOGIN_VERIFY_SIZE] = {0};
    S8 szBuff[sizeof(PT_CTRL_DATA_ST)] = {0};
    PT_CTRL_DATA_ST stCtrlData;
    PT_CC_CB_ST *pstPtcNode = NULL;
    S32 lResult = 0;

    /* 创建随机字符串 */
    pts_create_rand_str(szKey);
    dos_memcpy(stCtrlData.szLoginVerify, szKey, PT_LOGIN_VERIFY_SIZE);
    stCtrlData.enCtrlType = PT_CTRL_LOGIN_RSP;

    /* 添加登陆验证的随机字符串到发送队列 */
    if (NULL == g_pstPtcListSend)
    {
        pstPtcNode = pt_ptc_node_create(pstMsgDes->aucID, szPtcVersion, stClientAddr, lSockfd);
        if (NULL == pstPtcNode)
        {
            pt_logr_debug("login req : create ptc node fail");
            return;
        }
        g_pstPtcListSend = pt_ptc_list_insert(g_pstPtcListSend, pstPtcNode);
    }
    else
    {
        pstPtcNode = pt_ptc_list_search(g_pstPtcListSend, pstMsgDes->aucID);
        if(NULL == pstPtcNode)
        {
            pstPtcNode = pt_ptc_node_create(pstMsgDes->aucID, szPtcVersion, stClientAddr, lSockfd);
            if (NULL == pstPtcNode)
            {
                pt_logr_debug("create ptc node fail");
                return;
            }
            g_pstPtcListSend = pt_ptc_list_insert(g_pstPtcListSend, pstPtcNode);

        }
        else
        {
            /* 清空ptc资源 */
            pt_delete_ptc_resource(pstPtcNode);
            pstPtcNode->stDestAddr = stClientAddr;
        }
    }
    dos_memcpy(szBuff, (VOID *)&stCtrlData, sizeof(PT_CTRL_DATA_ST));

    lResult = pts_save_into_send_cache(pstPtcNode, PT_CTRL_LOGIN_RSP, pstMsgDes->enDataType, szBuff, sizeof(PT_CTRL_DATA_ST), NULL, 0);
    if (lResult == DOS_SUCC)
    {
        pts_send_login_verify(pstMsgDes);
    }

    return;
}

/**
 * 函数：VOID pts_send_hb_rsp(PT_MSG_TAG *pstMsgDes)
 * 功能：
 *      1.发送心跳响应
 * 参数
 *
 * 返回值：
 */
VOID pts_send_hb_rsp(S32 lSockfd, PT_MSG_TAG *pstMsgDes, struct sockaddr_in stClientAddr)
{
    if (NULL == pstMsgDes)
    {
        return;
    }

    PT_CTRL_DATA_ST stLoginRes;
    S8 szBuff[PT_SEND_DATA_SIZE] = {0};
    PT_MSG_TAG stMsgDes;

    stLoginRes.enCtrlType = PT_CTRL_HB_RSP;
    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, pstMsgDes->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_HB_RSP);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(szBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
    dos_memcpy(szBuff+sizeof(PT_MSG_TAG), (VOID *)&stLoginRes, sizeof(PT_CTRL_DATA_ST));

    sendto(lSockfd, szBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&stClientAddr, sizeof(stClientAddr));
    pt_logr_debug("send hb response to ptc : %.16s", pstMsgDes->aucID);

    return;
}

VOID pts_send_exit_notify_to_ptc(PT_MSG_TAG *pstMsgDes, PT_CC_CB_ST *pstPtcSendNode)
{
    if (NULL == pstPtcSendNode)
    {
        return;
    }

    PT_MSG_TAG stMsgDes;
    S8 szBuff[PT_DATA_BUFF_512] = {0};

    stMsgDes.enDataType = pstMsgDes->enDataType;
    dos_memcpy(stMsgDes.aucID, pstPtcSendNode->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(pstMsgDes->ulStreamID);
    stMsgDes.ExitNotifyFlag = DOS_TRUE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(szBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));

    sendto(pstPtcSendNode->lSocket, szBuff, sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&pstPtcSendNode->stDestAddr, sizeof(pstPtcSendNode->stDestAddr));

    return;
}

VOID pts_send_login_fail2ptc(S32 lSockfd, PT_MSG_TAG *pstMsgDes, struct sockaddr_in stClientAddr)
{
    if (NULL == pstMsgDes)
    {
        return;
    }

    PT_CTRL_DATA_ST stLoginRes;
    S8 szBuff[PT_SEND_DATA_SIZE] = {0};
    PT_MSG_TAG stMsgDes;

    stLoginRes.enCtrlType = PT_CTRL_LOGIN_ACK;
    stLoginRes.ucLoginRes = DOS_FALSE;

    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, pstMsgDes->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_LOGIN_ACK);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(szBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
    dos_memcpy(szBuff+sizeof(PT_MSG_TAG), (VOID *)&stLoginRes, sizeof(PT_CTRL_DATA_ST));

    sendto(lSockfd, szBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&stClientAddr, sizeof(stClientAddr));
    pt_logr_debug("send login response to ptc : %.16s", pstMsgDes->aucID);

    return;
}

/**
 * 函数：VOID pts_hd_timeout_callback(U64 param)
 * 功能：
 *      1.心跳响应超时函数
 * 参数
 *
 * 返回值：
 */
VOID pts_hd_timeout_callback(U64 param)
{
    S32 lRet = 0;
    PT_CC_CB_ST *pstPtcRecvNode = (PT_CC_CB_ST *)param;
    PT_CC_CB_ST *pstPtcSendNode = NULL;
    PT_MSG_TAG  stMsgDes;
    S8 szSql[PT_DATA_BUFF_128] = {0};
    U8 aucID[PTC_ID_LEN] = {0};
    pstPtcRecvNode->usHBOutTimeCount++;
    pt_logr_debug("%.16s hd rsp timeout : %d", pstPtcRecvNode->aucID, pstPtcRecvNode->usHBOutTimeCount);
    if (pstPtcRecvNode->usHBOutTimeCount > PTS_HB_TIMEOUT_COUNT_MAX)
    {
        /* 连续多次无法收到心跳，ptc掉线 */
        pt_logr_info("ptc lost connect : %.*s", PTC_ID_LEN, pstPtcRecvNode->aucID);
        dos_tmr_stop(&pstPtcRecvNode->stHBTmrHandle);
        pstPtcRecvNode->stHBTmrHandle = NULL;
        pstPtcRecvNode->usHBOutTimeCount = 0;

        stMsgDes.ulStreamID = PT_CTRL_LOGOUT;
        stMsgDes.enDataType = PT_DATA_CTRL;
        dos_memcpy(stMsgDes.aucID, pstPtcRecvNode->aucID, PTC_ID_LEN);
        /* 释放ptc接收缓存 */
#if PT_MUTEX_DEBUG
        pts_recv_pthread_mutex_lock(__FILE__, __LINE__);
#else
        pthread_mutex_lock(&g_pts_mutex_recv);
#endif
        dos_memcpy(aucID, pstPtcRecvNode->aucID, PTC_ID_LEN);
        g_pstPtcListRecv = pt_delete_ptc_node(g_pstPtcListRecv, pstPtcRecvNode);
#if PT_MUTEX_DEBUG
        pts_recv_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_recv);
#endif
        /* 释放ptc发送缓存 */
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_lock(__FILE__, __LINE__);
#else
        pthread_mutex_lock(&g_pts_mutex_send);
#endif
        pstPtcSendNode = pt_ptc_list_search(g_pstPtcListSend, aucID);
        if (pstPtcSendNode != NULL)
        {
            g_pstPtcListSend = pt_delete_ptc_node(g_pstPtcListSend, pstPtcSendNode);
        }
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif

        /* 修改数据库 */
        dos_snprintf(szSql, PT_DATA_BUFF_128, "update ipcc_alias set register=0 where sn='%.*s';", PTC_ID_LEN, aucID);
        lRet = dos_sqlite3_exec(g_pstMySqlite, szSql);
        if (lRet != DOS_SUCC)
        {
            pt_logr_info("ptc lost connect, update db fail");
        }
    }
}

S32 pts_create_recv_cache(S32 lSockfd, PT_MSG_TAG *pstMsgDes, S8 *szPtcVersion, struct sockaddr_in stClientAddr)
{
    if (NULL == pstMsgDes)
    {
        return DOS_FAIL;
    }

    PT_CC_CB_ST *pstPtcNode = NULL;
    PT_MSG_TAG stMsgDes;
    S32 lResult = 0;

    if (NULL == g_pstPtcListRecv)
    {
        pstPtcNode = pt_ptc_node_create(pstMsgDes->aucID, szPtcVersion, stClientAddr, lSockfd);
        if (NULL == pstPtcNode)
        {
            pt_logr_info("pts_login_verify : create ptc node fail");
            return DOS_FAIL;
        }
        g_pstPtcListRecv = pt_ptc_list_insert(g_pstPtcListRecv, pstPtcNode);
    }
    else
    {
        pstPtcNode = pt_ptc_list_search(g_pstPtcListRecv, pstMsgDes->aucID);
        if(NULL == pstPtcNode)
        {
            pstPtcNode = pt_ptc_node_create(pstMsgDes->aucID, szPtcVersion, stClientAddr, lSockfd);
            if (NULL == pstPtcNode)
            {
                pt_logr_info("pts_login_verify : create ptc node fail");
                return DOS_FAIL;
            }
            g_pstPtcListRecv = pt_ptc_list_insert(g_pstPtcListRecv, pstPtcNode);

        }
        else
        {
            pstPtcNode->usHBOutTimeCount = 0;
            pstPtcNode->stDestAddr = stClientAddr;
        }
    }

    stMsgDes.ExitNotifyFlag = pstMsgDes->ExitNotifyFlag;
    stMsgDes.ulStreamID = PT_CTRL_LOGIN_ACK;
    stMsgDes.enDataType = pstMsgDes->enDataType;

    //lResult = pts_save_login_ack_into_recv_cache(pstPtcNode, &stMsgDes, acBuff, sizeof(PT_CTRL_DATA_ST));
    //if (lResult < 0)
    //{
    //    return DOS_FAIL;
    //}

    /* 登陆成功，开启心跳接收的定时器 */
    if (pstPtcNode->stHBTmrHandle != NULL)
    {
        dos_tmr_stop(&pstPtcNode->stHBTmrHandle);
        pstPtcNode->stHBTmrHandle= NULL;
    }

    lResult = dos_tmr_start(&pstPtcNode->stHBTmrHandle, PTS_WAIT_HB_TIME, pts_hd_timeout_callback, (U64)pstPtcNode, TIMER_NORMAL_LOOP);
    if (lResult < 0)
    {
        pt_logr_info("pts start hb timer : start timer fail");
        return DOS_FAIL;
    }

    return lResult;
}

/**
 * 函数：S32 pts_login_verify(S32 lSockfd, PT_MSG_TAG *pstMsgDes, S8 *pData, struct sockaddr_in stClientAddr, S8 *szPtcVersion)
 * 功能：
 *      1.登陆验证
 * 参数
 *
 * 返回值：
 */
S32 pts_login_verify(S32 lSockfd, PT_MSG_TAG *pstMsgDes, S8 *pData, struct sockaddr_in stClientAddr, S8 *szPtcVersion)
{
    if (NULL == pstMsgDes || NULL == pData || NULL == szPtcVersion)
    {
        return DOS_FAIL;
    }

    S32 lResult = 0;
    S32 lRet = DOS_FAIL;
    S8 szKey[PT_LOGIN_VERIFY_SIZE] = {0};
    S8 szDestKey[PT_LOGIN_VERIFY_SIZE] = {0};
    PT_CTRL_DATA_ST *pstCtrlData = NULL;
    PT_CTRL_DATA_ST stLoginRes;
    S8 szBuff[sizeof(PT_CTRL_DATA_ST)] = {0};
    S8 acSendToPtcBuff[sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG)] = {0};
    PT_CC_CB_ST *pstPtcNode = NULL;
    PT_MSG_TAG stMsgDes;

    pstCtrlData = (PT_CTRL_DATA_ST *)(pData+sizeof(PT_MSG_TAG));
    pstCtrlData->ulLginVerSeq = dos_ntohl(pstCtrlData->ulLginVerSeq);
    /* 获取发送的随机字符串 */
    lResult = pts_get_key(pstMsgDes, szKey, pstCtrlData->ulLginVerSeq);
    if (lResult < 0)
    {
        /* TODO 验证失败，清除发送缓存 */
        pt_logr_debug("pts_login_verify : not find key");
        pstPtcNode = pt_ptc_list_search(g_pstPtcListSend, pstMsgDes->aucID);
        if (pstPtcNode != NULL)
        {
            g_pstPtcListSend = pt_delete_ptc_node(g_pstPtcListSend, pstPtcNode);
        }
        lRet = DOS_FAIL;
    }
    else
    {
        lResult = pts_key_convert(szKey, szDestKey, szPtcVersion);
        if (lResult < 0)
        {
            pt_logr_debug("key convert error");
            pstPtcNode = pt_ptc_list_search(g_pstPtcListSend, pstMsgDes->aucID);
            if (pstPtcNode != NULL)
            {
                g_pstPtcListSend = pt_delete_ptc_node(g_pstPtcListSend, pstPtcNode);
            }
            lRet = DOS_FAIL;
        }
        else
        {
            if (dos_memcmp(szDestKey, pstCtrlData->szLoginVerify, PT_LOGIN_VERIFY_SIZE))
            {
                pt_logr_info("login : verify fail");
                /* 验证失败 */
                pstPtcNode = pt_ptc_list_search(g_pstPtcListSend, pstMsgDes->aucID);
                if (pstPtcNode != NULL)
                {
                    g_pstPtcListSend = pt_delete_ptc_node(g_pstPtcListSend, pstPtcNode);
                }
                lRet = DOS_FAIL;
            }
            else
            {
                pt_logr_info("login : verify succ");
                lRet = DOS_SUCC;
            }
        }
    }

    /* 发送验证结果到ptc */
    stLoginRes.enCtrlType = PT_CTRL_LOGIN_ACK;
    if (lRet != DOS_SUCC)
    {
        stLoginRes.ucLoginRes = DOS_FALSE;
    }
    else
    {
        stLoginRes.ucLoginRes = DOS_TRUE;
    }

    dos_memcpy(stLoginRes.szPtcName, pstCtrlData->szPtcName, PT_PTC_NAME_LEN);

    dos_memcpy(szBuff, (VOID *)&stLoginRes, sizeof(PT_CTRL_DATA_ST));
    //pts_save_msg_into_cache(pstMsgDes->aucID, PT_DATA_CTRL, PT_CTRL_LOGIN_ACK, acBuff, sizeof(PT_CTRL_DATA_ST), DOS_FALSE);
    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, pstMsgDes->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_LOGIN_ACK);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(acSendToPtcBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
    dos_memcpy(acSendToPtcBuff+sizeof(PT_MSG_TAG), (VOID *)&stLoginRes, sizeof(PT_CTRL_DATA_ST));

    sendto(lSockfd, acSendToPtcBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&stClientAddr, sizeof(stClientAddr));

    /* 如果登录成功，创建接收缓存 */
    if (DOS_SUCC == lRet)
    {
        lRet = pts_create_recv_cache(lSockfd, pstMsgDes, szPtcVersion, stClientAddr);
    }

    return lRet;
}

/**
 * 函数：VOID pts_send_exit_notify2ptc(PT_CC_CB_ST *pstPtcNode, PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
 * 功能：
 *      1.发送退出通知给ptc
 * 参数
 *
 * 返回值：
 */
VOID pts_send_exit_notify2ptc(PT_CC_CB_ST *pstPtcNode, PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
{
    if (NULL == pstPtcNode || NULL == pstNeedRecvNode)
    {
        return;
    }

    PT_MSG_TAG stMsgDes;

    stMsgDes.enDataType = pstNeedRecvNode->enDataType;
    dos_memcpy(stMsgDes.aucID, pstNeedRecvNode->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(pstNeedRecvNode->ulStreamID);
    stMsgDes.ExitNotifyFlag = DOS_TRUE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    sendto(pstPtcNode->lSocket, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG), 0,  (struct sockaddr*)&pstPtcNode->stDestAddr, sizeof(pstPtcNode->stDestAddr));
}

/**
 * 函数：S32 pts_get_curr_position_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
 * 功能：
 *      1.ping 获得存放最新时间的位置
 * 参数
 *
 * 返回值：
 */
S32 pts_get_curr_position_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
{
    *(S32 *)para = atoi(column_value[0]);

    return 0;
}


/**
 * 函数：VOID pts_ctrl_msg_handle(S32 lSockfd, U32 ulStreamID, S8 *pData, struct sockaddr_in stClientAddr)
 * 功能：
 *      1.控制消息处理
 * 参数
 *
 * 返回值：
 */
VOID pts_ctrl_msg_handle(S32 lSockfd, S8 *pData, struct sockaddr_in stClientAddr)
{
    if (NULL == pData)
    {
        return;
    }

    PT_CTRL_DATA_ST *pstCtrlData = NULL;
    S32 lResult = 0;
    PT_MSG_TAG * pstMsgDes = NULL;
    PT_CC_CB_ST *pstPtcNode = NULL;
    S8 szSql[PTS_SQL_STR_SIZE] = {0};
    S8 szID[PTC_ID_LEN+1] = {0};
    S8 szPtcIntranetIP[IPV6_SIZE] = {0};   /* 内网IP */
    S8 szPtcInternetIP[IPV6_SIZE] = {0};   /* 外网IP */
    U16 usPtcIntranetPort = 0;
    U16 usPtcInternetPort = 0;
    S8 szPtcType[PT_DATA_BUFF_10] = {0};
    PT_PING_PACKET_ST *pstPingPacket = NULL;
    struct timeval stEndTime;
    U32 ulPingTime;
    S32 lCurPosition = -1;
    U32 ulDataField = 0;
    double dHBTimeInterval = 0.0;
    S8 szVersion[PT_IP_ADDR_SIZE] = {0};
    //S8 szNameDecode[PT_DATA_BUFF_64] = {0};

    pstMsgDes = (PT_MSG_TAG *)pData;
    pstCtrlData = (PT_CTRL_DATA_ST *)(pData + sizeof(PT_MSG_TAG));

    if (pstMsgDes->ExitNotifyFlag == DOS_TRUE)
    {
        pts_delete_send_stream_node(pstMsgDes, NULL, DOS_TRUE);
        return;
    }

    dos_memcpy(szID, pstMsgDes->aucID, PTC_ID_LEN);
    szID[PTC_ID_LEN] = '\0';
    switch (pstMsgDes->ulStreamID) //pstCtrlData->enCtrlType
    {
    case PT_CTRL_LOGIN_REQ:
        /* 登陆请求 */
        if (pts_is_ptc_sn(szID))
        {
            pt_logr_debug("request login ipcc id is %s", szID);
        }
        else
        {
            /* ptc id错误 */
            pt_logr_error("request login ipcc id(%s) format error", szID);
            pts_send_login_fail2ptc(lSockfd, pstMsgDes, stClientAddr);
            break;
        }
        pts_handle_login_req(lSockfd, pstMsgDes, stClientAddr, pstCtrlData->szLoginVerify);

        break;
    case PT_CTRL_LOGIN_RSP:
        /* 登陆验证, 添加结果到接收缓存，若验证成功，开启心跳定时器 */
        logr_debug("login rsp : %.16s", pstMsgDes->aucID);
        /* 判断 version */
        if (pstCtrlData->szVersion[1] != '.')
        {
            inet_ntop(AF_INET, (void *)(pstCtrlData->szVersion), szVersion, PT_IP_ADDR_SIZE);
        }
        else
        {
            dos_strcpy(szVersion, (S8 *)pstCtrlData->szVersion);
        }

        lResult = pts_login_verify(lSockfd, pstMsgDes, pData, stClientAddr, szVersion);
        if (DOS_SUCC == lResult)
        {
            inet_ntop(AF_INET, (void *)(pstMsgDes->aulServIp), szPtcIntranetIP, IPV6_SIZE);
            usPtcIntranetPort = dos_ntohs(pstMsgDes->usServPort);
            inet_ntop(AF_INET, &stClientAddr.sin_addr, szPtcInternetIP, IPV6_SIZE);
            usPtcInternetPort = dos_ntohs(stClientAddr.sin_port);

            switch (pstCtrlData->enPtcType)
            {
            case PT_PTC_TYPE_OEM:
                strcpy(szPtcType, "Embedded");
                break;
            case PT_PTC_TYPE_WINDOWS:
                strcpy(szPtcType, "Windows");
                break;
            case PT_PTC_TYPE_IPCC:
                strcpy(szPtcType, "Linux");
                break;
            default:
                break;
            }

            dos_snprintf(szSql, PTS_SQL_STR_SIZE, "select * from ipcc_alias where sn='%.*s'", PTC_ID_LEN, pstMsgDes->aucID);
            lResult = dos_sqlite3_record_is_exist(g_pstMySqlite, szSql);
            if (lResult < 0)
            {
                DOS_ASSERT(0);
            }
            else if (DOS_TRUE == lResult)  /* 判断是否存在 */
            {
                /* 存在，更新IPCC的注册状态 */
                pt_logr_debug("pts_send_msg2client : db existed");
                dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ipcc_alias set register=1, name='%s', version='%s', lastLoginTime=datetime('now','localtime'), intranetIP='%s'\
, intranetPort=%d, internetIP='%s', internetPort=%d, ptcType='%s', achPtsMajorDomain='%s', achPtsMinorDomain='%s'\
, usPtsMajorPort=%d, usPtsMinorPort=%d, szPtsHistoryIp1='%s', szPtsHistoryIp2='%s', szPtsHistoryIp3='%s'\
, usPtsHistoryPort1=%d, usPtsHistoryPort2=%d, usPtsHistoryPort3=%d, szMac='%s' where sn='%.*s';"
                             , pstCtrlData->szPtcName, szVersion, szPtcIntranetIP, usPtcIntranetPort, szPtcInternetIP, usPtcInternetPort
                             , szPtcType, pstCtrlData->achPtsMajorDomain, pstCtrlData->achPtsMinorDomain, dos_ntohs(pstCtrlData->usPtsMajorPort)
                             , dos_ntohs(pstCtrlData->usPtsMinorPort), pstCtrlData->szPtsHistoryIp1, pstCtrlData->szPtsHistoryIp2, pstCtrlData->szPtsHistoryIp3
                             , dos_ntohs(pstCtrlData->usPtsHistoryPort1), dos_ntohs(pstCtrlData->usPtsHistoryPort2), dos_ntohs(pstCtrlData->usPtsHistoryPort3)
                             , pstCtrlData->szMac, PTC_ID_LEN, pstMsgDes->aucID);

                lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
                if (lResult != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                    pt_logr_info("update db state fail : %.*s", PTC_ID_LEN, pstMsgDes->aucID);
                }
            }
            else
            {
                /* 不存在，添加IPCC到DB */
                pt_logr_debug("pts_send_msg2client : db insert");
                dos_snprintf(szSql, PTS_SQL_STR_SIZE, "INSERT INTO ipcc_alias (\"id\", \"sn\", \"name\", \"remark\", \"version\", \"register\", \"domain\", \"intranetIP\", \"internetIP\", \"intranetPort\"\
, \"internetPort\", \"ptcType\", \"achPtsMajorDomain\", \"achPtsMinorDomain\", \"usPtsMajorPort\", \"usPtsMinorPort\", \"szPtsHistoryIp1\", \"szPtsHistoryIp2\", \"szPtsHistoryIp3\"\
, \"usPtsHistoryPort1\", \"usPtsHistoryPort2\", \"usPtsHistoryPort3\", \"szMac\") VALUES (NULL, '%s', '%s', NULL, '%s', %d, NULL, '%s', '%s', %d, %d, '%s', '%s', '%s', %d, %d, '%s', '%s', '%s', %d, %d, %d, '%s');"
                             , szID, pstCtrlData->szPtcName, szVersion, DOS_TRUE, szPtcIntranetIP, szPtcInternetIP, usPtcIntranetPort, usPtcInternetPort, szPtcType
                             , pstCtrlData->achPtsMajorDomain, pstCtrlData->achPtsMinorDomain, dos_ntohs(pstCtrlData->usPtsMajorPort), dos_ntohs(pstCtrlData->usPtsMinorPort)
                             , pstCtrlData->szPtsHistoryIp1, pstCtrlData->szPtsHistoryIp2, pstCtrlData->szPtsHistoryIp3, dos_ntohs(pstCtrlData->usPtsHistoryPort1)
                             , dos_ntohs(pstCtrlData->usPtsHistoryPort2), dos_ntohs(pstCtrlData->usPtsHistoryPort3), pstCtrlData->szMac);

                lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
                if (lResult != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                    pt_logr_info("insert db state fail : %.*s", PTC_ID_LEN, pstMsgDes->aucID);
                }
            }
        }

        break;
    case PT_CTRL_LOGOUT:
        /* 退出登陆 */
        pts_handle_logout_req(pstMsgDes);

        break;
    case PT_CTRL_HB_REQ:
        /* 心跳, 修改ptc中的usHBOutTimeCount */
        pt_logr_debug("recv hb from ptc : %.16s", pstMsgDes->aucID);
        dHBTimeInterval = (double)pstCtrlData->lHBTimeInterval/1000;
        /* 将心跳和响应之间的时间差，更新到数据库 */
        dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ipcc_alias set heartbeatTime=%.2f where sn='%.*s';", dHBTimeInterval, PTC_ID_LEN, pstMsgDes->aucID);
        lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
        if (lResult != DOS_SUCC)
        {
            pt_logr_info("hb time, update db fail");
        }

        pstPtcNode = pt_ptc_list_search(g_pstPtcListRecv, pstMsgDes->aucID);
        if(NULL == pstPtcNode)
        {
            pt_logr_info("pts_ctrl_msg_handle : can not found ptc id = %.16s", pstMsgDes->aucID);
            break;
        }
        else
        {
            pstPtcNode->usHBOutTimeCount = 0;
        }

        pts_send_hb_rsp(lSockfd, pstMsgDes, stClientAddr);
        break;
    case PT_CTRL_PTS_MAJOR_DOMAIN:
        if (pstCtrlData->achPtsMajorDomain[0] != '\0')
        {
            dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ipcc_alias set achPtsMajorDomain='%s' where sn='%.*s';", pstCtrlData->achPtsMajorDomain, PTC_ID_LEN, pstMsgDes->aucID);
            lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
            if (lResult != DOS_SUCC)
            {
                DOS_ASSERT(0);
                pt_logr_info("change major domain, update db fail");
            }
        }
        if (pstCtrlData->usPtsMajorPort != 0)
        {
            dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ipcc_alias set usPtsMajorPort=%d where sn='%.*s';", dos_ntohs(pstCtrlData->usPtsMajorPort), PTC_ID_LEN, pstMsgDes->aucID);
            lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
            if (lResult != DOS_SUCC)
            {
                DOS_ASSERT(0);
                pt_logr_info("change major port, update db fail");
            }
        }
        break;
    case PT_CTRL_PTS_MINOR_DOMAIN:
        if (pstCtrlData->achPtsMinorDomain[0] != '\0')
        {
            dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ipcc_alias set achPtsMinorDomain='%s' where sn='%.*s';", pstCtrlData->achPtsMinorDomain, PTC_ID_LEN, pstMsgDes->aucID);
            lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
            if (lResult != DOS_SUCC)
            {
                DOS_ASSERT(0);
                pt_logr_info("change minor domain, update db fail");
            }
        }
        if (pstCtrlData->usPtsMinorPort != 0)
        {
            dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ipcc_alias set usPtsMinorPort=%d where sn='%.*s';", dos_ntohs(pstCtrlData->usPtsMinorPort), PTC_ID_LEN, pstMsgDes->aucID);
            lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
            if (lResult != DOS_SUCC)
            {
                DOS_ASSERT(0);
                pt_logr_info("change minor port, update db fail");
            }
        }
        break;
    case PT_CTRL_PING_ACK:
        pstPingPacket = (PT_PING_PACKET_ST *)(pData + sizeof(PT_MSG_TAG));
        gettimeofday(&stEndTime, NULL);
        /* 计算时间差 */
        ulPingTime = (stEndTime.tv_sec - pstPingPacket->stStartTime.tv_sec) * 1000 * 1000 + (stEndTime.tv_usec - pstPingPacket->stStartTime.tv_usec);
        /* 不超过超时时间 */
        if (ulPingTime < (PTS_PING_TIMEOUT)*1000)
        {
            dos_tmr_stop(&pstPingPacket->hTmrHandle);
            pstPingPacket->hTmrHandle = NULL;
            dos_snprintf(szSql, PTS_SQL_STR_SIZE, "select curr_position from ping_result where sn='%.*s' limit 1 ", PTC_ID_LEN, pstMsgDes->aucID);
            lResult = dos_sqlite3_exec_callback(g_pstMySqlite, szSql, pts_get_curr_position_callback, (VOID *)&lCurPosition);
            if (lResult != DOS_SUCC)
            {
                DOS_ASSERT(0);
                return;
            }

            if (lCurPosition < 0)
            {
                dos_snprintf(szSql, PTS_SQL_STR_SIZE, "INSERT INTO ping_result (\"id\", \"sn\", \"curr_position\", \"timer0\") VALUES (NULL, '%.*s', 0, %d)", PTC_ID_LEN, pstMsgDes->aucID, ulPingTime);
            }
            else
            {
                ulDataField = lCurPosition + 1;
                if (ulDataField > 7)
                {
                    ulDataField = 0;
                }

                dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ping_result set curr_position=%d, timer%d=%d where sn='%.*s'", ulDataField, ulDataField, ulPingTime, PTC_ID_LEN, pstMsgDes->aucID);
            }

            lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
            if (lResult != DOS_SUCC)
            {
                pt_logr_info("update hb time fail : %.*s", PTC_ID_LEN, pstMsgDes->aucID);
            }
        }
        break;
    default:
        break;
    }

}

/**
 * 函数：VOID pts_delete_stream_node(PT_MSG_TAG *pstMsgDes, PT_CC_CB_ST *pstPtcRecvNode)
 * 功能：
 *      1.删除stream 节点
 * 参数
 *
 * 返回值：
 */

VOID pts_delete_recv_stream_node(PT_MSG_TAG *pstMsgDes, PT_CC_CB_ST *pstPtcRecvNode, BOOL bIsMutex)
{
    if (NULL == pstMsgDes)
    {
        return;
    }

    list_t *pstStreamListHead = NULL;
    PT_STREAM_CB_ST *pstStreamNode = NULL;

    if (bIsMutex)
    {
#if PT_MUTEX_DEBUG
        pts_recv_pthread_mutex_lock(__FILE__, __LINE__);
#else
        pthread_mutex_lock(&g_pts_mutex_recv);
#endif
    }

    if (NULL == pstPtcRecvNode)
    {
        pstPtcRecvNode = pt_ptc_list_search(g_pstPtcListRecv, pstMsgDes->aucID);
    }
    if (pstPtcRecvNode != NULL)
    {
        pstStreamListHead = pstPtcRecvNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
        if (pstStreamListHead != NULL)
        {
            pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
            if (pstStreamNode != NULL)
            {
                if (pstStreamNode->hTmrHandle != NULL)
                {
                    dos_tmr_stop(&pstStreamNode->hTmrHandle);
                    pstStreamNode->hTmrHandle = NULL;
                }
                pstStreamListHead = pt_delete_stream_node(pstStreamListHead, &pstStreamNode->stStreamListNode, pstMsgDes->enDataType);
                pstPtcRecvNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead = pstStreamListHead;
            }
        }
    }

    if (bIsMutex)
    {
#if PT_MUTEX_DEBUG
        pts_recv_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_recv);
#endif
    }
}

VOID pts_delete_send_stream_node(PT_MSG_TAG *pstMsgDes, PT_CC_CB_ST *pstPtcSendNode, BOOL bIsMutex)
{
    if (NULL == pstMsgDes)
    {
        return;
    }

    list_t *pstStreamListHead = NULL;
    PT_STREAM_CB_ST *pstStreamNode = NULL;

    if (bIsMutex)
    {
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_lock(__FILE__, __LINE__);
#else
        pthread_mutex_lock(&g_pts_mutex_send);
#endif
    }

    if (NULL == pstPtcSendNode)
    {
        pstPtcSendNode = pt_ptc_list_search(g_pstPtcListSend, pstMsgDes->aucID);
    }
    if (pstPtcSendNode != NULL)
    {
        pstStreamListHead = pstPtcSendNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
        if (pstStreamListHead != NULL)
        {
            pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
            if (pstStreamNode != NULL)
            {
                pstStreamListHead = pt_delete_stream_node(pstStreamListHead, &pstStreamNode->stStreamListNode, pstMsgDes->enDataType);
                pstPtcSendNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead = pstStreamListHead;
            }
        }
    }

    if (bIsMutex)
    {
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
    }
}

/**
 * 函数：S32 pts_deal_with_confirm_msg(PT_CC_CB_ST *pstPtcNode, PT_MSG_TAG *pstMsgDes)
 * 功能：
 *      1.处理消息确认消息
 * 参数
 *
 * 返回值：无
 */
S32 pts_deal_with_confirm_msg(PT_MSG_TAG *pstMsgDes)
{
    PT_CC_CB_ST        *pstPtcNode         = NULL;
    list_t             *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST    *pstStreamNode      = NULL;

    if (NULL == pstMsgDes)
    {
        return DOS_FALSE;
    }

#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_lock(__FILE__, __LINE__);
#else
    pthread_mutex_lock(&g_pts_mutex_send);
#endif
    if (NULL == g_pstPtcListSend)
    {
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
        return DOS_FALSE;
    }

    pstPtcNode = pt_ptc_list_search(g_pstPtcListSend, pstMsgDes->aucID);
    if(NULL == pstPtcNode)
    {
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
        return DOS_FALSE;
    }

    pstStreamListHead = pstPtcNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
    if (NULL == pstStreamListHead)
    {
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
        return DOS_FALSE;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
    if (NULL == pstStreamNode)
    {
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
        return DOS_FALSE;
    }

    if (pstStreamNode->lConfirmSeq < pstMsgDes->lSeq)
    {
        pstStreamNode->lConfirmSeq = pstMsgDes->lSeq;
        sem_post(&g_SemPts);
    }

#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
    pthread_mutex_unlock(&g_pts_mutex_send);
#endif
    return DOS_TRUE;
}

/**
 * 函数：void pts_save_msg_into_cache(S8 *pcIpccId, PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen, U8 ExitNotifyFlag)
 * 功能：
 *      1.添加数据到发送缓存区
 *      2.如果添加成功，则通知发送线程，发送数据
 * 参数
 *      S8 *pcIpccId                  : IPCC ID
 *      PT_DATA_TYPE_EN enDataType    ：data的类型
 *      U32 ulStreamID                ：stream ID
 *      S8 *pcData                    ：包内容
 *      S32 lDataLen                  ：包内容长度
 *      U8 ExitNotifyFlag             ：通知对方响应是否结束
 * 返回值：无
 */
VOID pts_save_msg_into_cache(U8 *pcIpccId, PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen, S8 *szDestIp, U16 usDestPort)
{
    if (enDataType < 0 || enDataType >= PT_DATA_BUTT)
    {
        pt_logr_debug("Data Type should in 0-%d: %d", PT_DATA_BUTT - 1, enDataType);
        return;
    }
    else if (NULL == pcIpccId || NULL == pcData)
    {
        pt_logr_debug("data pointer is NULL");
        return;
    }

    PT_CC_CB_ST *pstPtcNode = NULL;
    S32 lResult = 0;
    struct timespec stSemTime;
    struct timeval now;

#if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_lock(__FILE__, __LINE__);
#else
    pthread_mutex_lock(&g_pts_mutex_send);
#endif
    pstPtcNode = pt_ptc_list_search(g_pstPtcListSend, pcIpccId);
    if(NULL == pstPtcNode)
    {
        pt_logr_debug("pts_send_msg : not found IPCC");
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
        return;
    }
    else
    {
        BOOL bIsResend = DOS_FALSE;
        PT_CMD_EN enCmdValue = PT_CMD_NORMAL;
        PT_MSG_TAG stMsgDes;

        stMsgDes.ExitNotifyFlag = DOS_FALSE;
        stMsgDes.ulStreamID = ulStreamID;
        stMsgDes.enDataType = enDataType;

        lResult = pts_save_into_send_cache(pstPtcNode, ulStreamID, enDataType, pcData, lDataLen, szDestIp, usDestPort);
        if (lResult < 0)
        {
            /* 添加发送消息失败 */
            return;
        }
        else
        {
            if (NULL == pt_need_send_node_list_search(g_pstPtsNendSendNode, ulStreamID))
            {
                g_pstPtsNendSendNode = pt_need_send_node_list_insert(g_pstPtsNendSendNode, pcIpccId, &stMsgDes, enCmdValue, bIsResend);
            }
            pthread_cond_signal(&g_pts_cond_send);
        }
    }

    if (PT_NEED_CUT_PTHREAD == lResult)
    {
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
        gettimeofday(&now, NULL);
        stSemTime.tv_sec = now.tv_sec + 5;
        stSemTime.tv_nsec = now.tv_usec * 1000;
        pt_logr_debug("wair make sure msg");
        sem_timedwait(&g_SemPts, &stSemTime);
    }
    else
    {
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif
        //usleep(10);
    }
}

/**
 * 函数：void *pts_send_msg2ipcc(VOID *arg)
 * 功能：
 *      1.发送数据线程
 * 参数
 *      VOID *arg :通道通信的sockfd
 * 返回值：void *
 */
VOID *pts_send_msg2ptc(VOID *arg)
{
    S32              lSockfd         = *(S32 *)arg;
    PT_CC_CB_ST      *pstPtcNode     = NULL;
    U32              ulArraySub      = 0;
    U32              ulSendCount     = 0;
    PT_MSG_TAG       stMsgDes;
    list_t           *pstStreamHead   = NULL;
    list_t           *pstNendSendList = NULL;
    PT_STREAM_CB_ST  *pstStreamNode   = NULL;
    PT_DATA_TCP_ST   *pstSendDataHead = NULL;
    PT_DATA_TCP_ST   stSendDataNode;
    S8 szBuff[PT_SEND_DATA_SIZE] = {0};
    PT_NEND_SEND_NODE_ST *pstNeedSendNode = NULL;
    PT_DATA_TCP_ST    stRecvDataTcp;
    S32 lResult = 0;

    while(1)
    {
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_lock(__FILE__, __LINE__);
#else
        pthread_mutex_lock(&g_pts_mutex_send);
#endif
#if PT_MUTEX_DEBUG
        pts_send_pthread_cond_wait(__FILE__, __LINE__);
#else
        pthread_cond_wait(&g_pts_cond_send, &g_pts_mutex_send);
#endif
        /* 循环发送g_pstPtsNendRecvNode中的stream */
        pstNendSendList = g_pstPtsNendSendNode;

        while(1)
        {
            if (NULL == pstNendSendList)
            {
                break;
            }

            pstNeedSendNode = dos_list_entry(pstNendSendList, PT_NEND_SEND_NODE_ST, stListNode);
            if (pstNendSendList == pstNendSendList->next)
            {
                /* 最后一个 */
                pstNendSendList = NULL;
            }
            else
            {
                pstNendSendList = pstNendSendList->next;
                dos_list_del(&pstNeedSendNode->stListNode);
            }

            dos_memzero(&stMsgDes, sizeof(PT_MSG_TAG));

            pstPtcNode = pt_ptc_list_search(g_pstPtcListSend, pstNeedSendNode->aucID);
            if(NULL == pstPtcNode)
            {
                pt_logr_debug("pts_send_msg2ptc : not found ptc");
            }
            else if (pstNeedSendNode->enCmdValue != PT_CMD_NORMAL || pstNeedSendNode->ExitNotifyFlag == DOS_TRUE)
            {
                /* 发送丢包、重发请求 / 消息确认接收 / 发送结束消息 */
                stMsgDes.enDataType = pstNeedSendNode->enDataType;
                dos_memcpy(stMsgDes.aucID, pstNeedSendNode->aucID, PTC_ID_LEN);
                stMsgDes.ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                stMsgDes.ExitNotifyFlag = pstNeedSendNode->ExitNotifyFlag;
                stMsgDes.lSeq = dos_htonl(pstNeedSendNode->lSeqResend);
                stMsgDes.enCmdValue = pstNeedSendNode->enCmdValue;
                stMsgDes.bIsEncrypt = DOS_FALSE;
                stMsgDes.bIsCompress = DOS_FALSE;

                lResult = sendto(lSockfd, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG), 0,  (struct sockaddr*)&pstPtcNode->stDestAddr, sizeof(pstPtcNode->stDestAddr));
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }

            pstStreamHead = pstPtcNode->astDataTypes[pstNeedSendNode->enDataType].pstStreamQueHead;
            if (NULL == pstStreamHead)
            {
                pt_logr_debug("pts_send_msg2ptc : stream list is NULL");
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }

            pstStreamNode = pt_stream_queue_search(pstStreamHead, pstNeedSendNode->ulStreamID);
            if(NULL == pstStreamNode)
            {
                pt_logr_debug("pts_send_msg2ptc : cann't found stream node");
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }

            pstSendDataHead = pstStreamNode->unDataQueHead.pstDataTcp;
            if (NULL == pstSendDataHead)
            {
                pt_logr_debug("send data to ptc : data queue is NULL");
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }

            if (pstNeedSendNode->bIsResend)
            {
                /* 要求重传的包 */
                ulArraySub = pstNeedSendNode->lSeqResend & (PT_DATA_SEND_CACHE_SIZE - 1);  /* 要发送的包在data数组中的下标 */

                if (pstSendDataHead[ulArraySub].lSeq != pstNeedSendNode->lSeqResend)
                {
                    /* 要发送的包不存在 */
                    pt_logr_debug("send data to pts : data package is not exit");
                }
                else
                {
                    stSendDataNode = pstSendDataHead[ulArraySub];
                    stMsgDes.enDataType = pstNeedSendNode->enDataType;
                    dos_memcpy(stMsgDes.aucID, pstNeedSendNode->aucID, PTC_ID_LEN);
                    stMsgDes.ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                    stMsgDes.ExitNotifyFlag = stSendDataNode.ExitNotifyFlag;
                    stMsgDes.lSeq = dos_htonl(pstNeedSendNode->lSeqResend);
                    stMsgDes.enCmdValue = pstNeedSendNode->enCmdValue;
                    stMsgDes.bIsEncrypt = DOS_FALSE;
                    stMsgDes.bIsCompress = DOS_FALSE;
                    dos_memcpy(stMsgDes.aulServIp, pstStreamNode->aulServIp, IPV6_SIZE);
                    stMsgDes.usServPort = dos_htons(pstStreamNode->usServPort);

                    dos_memcpy(szBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
                    dos_memcpy(szBuff+sizeof(PT_MSG_TAG), stSendDataNode.szBuff, stSendDataNode.ulLen);

                    ulSendCount = PT_RESEND_RSP_COUNT;    /*重传的，发送三遍*/
                    while (ulSendCount)
                    {
                        pt_logr_info("send data to ptc, stream : %d, seq : %d, size : %d", pstNeedSendNode->ulStreamID, pstNeedSendNode->lSeqResend, stSendDataNode.ulLen);
                        sendto(lSockfd, szBuff, stSendDataNode.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&pstPtcNode->stDestAddr, sizeof(pstPtcNode->stDestAddr));
                        ulSendCount--;
                    }

                }
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }

            while(1)
            {
                /* 发送data，直到不连续 */
                pstStreamNode->lCurrSeq++;
                ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_SEND_CACHE_SIZE - 1);
                stRecvDataTcp = pstSendDataHead[ulArraySub];
                if (stRecvDataTcp.lSeq == pstStreamNode->lCurrSeq)
                {
                    //stSendDataNode = pstSendDataHead[ulArraySub];
                    stMsgDes.enDataType = pstNeedSendNode->enDataType;
                    dos_memcpy(stMsgDes.aucID, pstNeedSendNode->aucID, PTC_ID_LEN);
                    stMsgDes.ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                    stMsgDes.ExitNotifyFlag = stRecvDataTcp.ExitNotifyFlag;
                    stMsgDes.lSeq = dos_htonl(pstStreamNode->lCurrSeq);
                    stMsgDes.enCmdValue = pstNeedSendNode->enCmdValue;
                    stMsgDes.bIsEncrypt = DOS_FALSE;
                    stMsgDes.bIsCompress = DOS_FALSE;
                    dos_memcpy(stMsgDes.aulServIp, pstStreamNode->aulServIp, IPV6_SIZE);
                    stMsgDes.usServPort = dos_htons(pstStreamNode->usServPort);

                    dos_memcpy(szBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
                    dos_memcpy(szBuff+sizeof(PT_MSG_TAG), stRecvDataTcp.szBuff, stRecvDataTcp.ulLen);
                    pt_logr_debug("pts send data to ptc, streamID is %d, seq is %d ", pstNeedSendNode->ulStreamID, pstStreamNode->lCurrSeq);
                    //printf("send msg to ptc, stream : %d, seq : %d, len : %d\n", pstNeedSendNode->ulStreamID, pstStreamNode->lCurrSeq, stRecvDataTcp.ulLen);
                    sendto(lSockfd, szBuff, stRecvDataTcp.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&pstPtcNode->stDestAddr, sizeof(pstPtcNode->stDestAddr));
                }
                else
                {
                    /* 这个seq没有发送，减一 */
                    pstStreamNode->lCurrSeq--;
                    break;
                }
            }

            dos_dmem_free(pstNeedSendNode);
            pstNeedSendNode = NULL;

        }/* end of while(1) */
        g_pstPtsNendSendNode = pstNendSendList;
#if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_send);
#endif

    } /* end of while(1) */

    pthread_mutex_destroy(&g_pts_mutex_send);
    pthread_cond_destroy(&g_pts_cond_send);
}

/**
 * 函数：void *pts_recv_msg_from_ipcc(VOID *arg)
 * 功能：
 *      1.接收数据线程
 * 参数
 *      VOID *arg :通道通信的sockfd
 * 返回值：
 */
VOID *pts_recv_msg_from_ptc(VOID *arg)
{
    S32  lSockfd   = *(S32 *)arg;
    S32  lRecvLen  = 0;
    S32  lResult   = 0;
    struct sockaddr_in  stClientAddr;
    socklen_t           lCliaddrLen    = sizeof(stClientAddr);
    PT_MSG_TAG         *pstMsgDes      = NULL;
    PT_CC_CB_ST        *pstPtcNode     = NULL;
    S8                  acRecvBuf[PT_SEND_DATA_SIZE] = {0};
    U32                 MaxFdp = lSockfd;
    fd_set              ReadFdsCpio;
    fd_set              ReadFds;
    struct timeval stTimeVal = {1, 0};
    struct timeval stTimeValCpy;
    FD_ZERO(&ReadFds);
    FD_SET(lSockfd, &ReadFds);
    /* 初始化信号量 */
    sem_init(&g_SemPtsRecv, 0, 1);
    g_lPtsUdpSocket = lSockfd;

    while(1)
    {
        stTimeValCpy = stTimeVal;
        ReadFdsCpio = ReadFds;
        lResult = select(MaxFdp + 1, &ReadFdsCpio, NULL, NULL, &stTimeValCpy);
        if (lResult < 0)
        {
            perror("pts_recv_msg_from_ptc select");
            DOS_ASSERT(0);
            sleep(1);
            continue;
        }
        else if (0 == lResult)
        {
            continue;
        }
        if (FD_ISSET(lSockfd, &ReadFdsCpio))
        {
            lRecvLen = recvfrom(lSockfd, acRecvBuf, PT_SEND_DATA_SIZE, 0, (struct sockaddr*)&stClientAddr, &lCliaddrLen);
            sem_wait(&g_SemPtsRecv);
#if PT_MUTEX_DEBUG
            pts_recv_pthread_mutex_lock(__FILE__, __LINE__);
#else
            pthread_mutex_lock(&g_pts_mutex_recv);
#endif
            /* 取出头部信息 */
            pstMsgDes = (PT_MSG_TAG *)acRecvBuf;
            /* 字节序转换 */
            pstMsgDes->ulStreamID = dos_ntohl(pstMsgDes->ulStreamID);
            pstMsgDes->lSeq = dos_ntohl(pstMsgDes->lSeq);

            if (pstMsgDes->enDataType == PT_DATA_CTRL)
            {
                /* 控制消息 */
                pts_ctrl_msg_handle(lSockfd, acRecvBuf, stClientAddr);
                sem_post(&g_SemPtsRecv);
            }
            else
            {
                pstPtcNode = pt_ptc_list_search(g_pstPtcListRecv, pstMsgDes->aucID);
                if(NULL == pstPtcNode)
                {
                    pt_logr_info("pts_recv_msg_from_ptc : not found ipcc");
#if PT_MUTEX_DEBUG
                    pts_recv_pthread_mutex_unlock(__FILE__, __LINE__);
#else
                    pthread_mutex_unlock(&g_pts_mutex_recv);
#endif
                    sem_post(&g_SemPtsRecv);
                    continue;
                }

                if (pstMsgDes->enCmdValue == PT_CMD_RESEND)
                {
                    /* pts发送的重传请求 */
#if PT_MUTEX_DEBUG
                    pts_send_pthread_mutex_lock(__FILE__, __LINE__);
#else
                    pthread_mutex_lock(&g_pts_mutex_send);
#endif
                    BOOL bIsResend = DOS_TRUE;
                    PT_CMD_EN enCmdValue = PT_CMD_NORMAL;

                    g_pstPtsNendSendNode = pt_need_send_node_list_insert(g_pstPtsNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);

                    pthread_cond_signal(&g_pts_cond_send);
#if PT_MUTEX_DEBUG
                    pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
                    pthread_mutex_unlock(&g_pts_mutex_send);
#endif
                    sem_post(&g_SemPtsRecv);
                }
                else if (pstMsgDes->enCmdValue == PT_CMD_CONFIRM)
                {
                    /* 确认接收消息 */
                    pt_logr_debug("pts recv make sure, seq : %d", pstMsgDes->lSeq);
                    pts_deal_with_confirm_msg(pstMsgDes);
                    sem_post(&g_SemPtsRecv);
                }
                else if (pstMsgDes->ExitNotifyFlag == DOS_TRUE)
                {
                    /* stream 退出 */
                    pt_logr_debug("pts recv exit msg, streamID = %d", pstMsgDes->ulStreamID);
                    pts_delete_recv_stream_node(pstMsgDes, pstPtcNode, DOS_FALSE);
                    pts_delete_send_stream_node(pstMsgDes, NULL, DOS_TRUE);
                    if (pstMsgDes->ulStreamID != PT_CTRL_PTC_PACKAGE)
                    {
                        g_pstPtsNendRecvNode = pt_need_recv_node_list_insert(g_pstPtsNendRecvNode, pstMsgDes);
                        pthread_cond_signal(&g_pts_cond_recv);
                    }
                    sem_post(&g_SemPtsRecv);
                }
                else
                {
                    pt_logr_debug("pts recv data from ptc, streamID = %d, seq : %d", pstMsgDes->ulStreamID, pstMsgDes->lSeq);
                    lResult = pts_save_into_recv_cache(pstPtcNode, pstMsgDes, acRecvBuf+sizeof(PT_MSG_TAG), lRecvLen-sizeof(PT_MSG_TAG));
                    if (lResult < 0)
                    {
                        sem_post(&g_SemPtsRecv);
                    }
                    else
                    {
                        if (NULL == pt_need_recv_node_list_search(g_pstPtsNendRecvNode, pstMsgDes->ulStreamID))
                        {
                            g_pstPtsNendRecvNode = pt_need_recv_node_list_insert(g_pstPtsNendRecvNode, pstMsgDes);
                        }
                        pthread_cond_signal(&g_pts_cond_recv);
                    }
                }
            }
#if PT_MUTEX_DEBUG
            pts_recv_pthread_mutex_unlock(__FILE__, __LINE__);
#else
            pthread_mutex_unlock(&g_pts_mutex_recv);
#endif

            if (lResult == PT_NEED_CUT_PTHREAD)
            {
                /* 挂起线程，执行接收函数 */
                usleep(10);
            }
        }

    } /* end of while(1) */
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

