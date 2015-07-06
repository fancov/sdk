#ifdef  __cplusplus
extern "C" {
#endif

/* include sys header files */
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <ctype.h>
#include <netinet/tcp.h>
#include <sys/time.h>

/* include provate header file */
#include <pt/ptc.h>
#include "ptc_msg.h"
#include "ptc_web.h"

const S8 *g_szFifoName = "/tmp/ptc_fifo";
S32 g_lPipeWRFd = 0;

pthread_mutex_t g_mutexPtcRecvMsgHandle     = PTHREAD_MUTEX_INITIALIZER;        /* 处理接收消息锁 */
pthread_cond_t  g_condPtcRecvMsgHandle      = PTHREAD_COND_INITIALIZER;         /* 处理接收消息变量 */
U32 g_ulWebSocketMax = 0;
fd_set g_stWebFdSet;

S32 ptc_recvfrom_web_buff_list_insert(PTC_CLIENT_CB_ST *pstPtcClientCB, S8 *szBUff, S32 lBuffLen)
{
    PTC_WEB_REV_MSG_HANDLE_ST *pstNewNode = NULL;
    U32 ulCount = 0;

    if (DOS_ADDR_INVALID(szBUff) || DOS_ADDR_INVALID(pstPtcClientCB))
    {
        return DOS_FAIL;
    }

    pstNewNode = (PTC_WEB_REV_MSG_HANDLE_ST *)dos_dmem_alloc(sizeof(PTC_WEB_REV_MSG_HANDLE_ST));
    if (DOS_ADDR_INVALID(pstNewNode))
    {
        pt_logr_debug("malloc PTC_WEB_REV_MSG_HANDLE_ST fail");

        return DOS_FAIL;
    }

    pstNewNode->paRecvBuff = szBUff;
    pstNewNode->lRecvLen = lBuffLen;
    pstNewNode->ulStreamID = pstPtcClientCB->ulStreamID;
    pstNewNode->lSocket = pstPtcClientCB->lSocket;
    dos_list_node_init(&pstNewNode->stList);

    ulCount = pstPtcClientCB->ulBuffNodeCount;
    if (ulCount < PTC_LOCK_MIN_COUNT)
    {
        pthread_mutex_lock(&pstPtcClientCB->pMutexPthread);
    }

    if (pstPtcClientCB->enState != PTC_CLIENT_USING)
    {
        dos_dmem_free(pstNewNode);
        pstNewNode = NULL;

        if (ulCount < PTC_LOCK_MIN_COUNT)
        {
            pthread_mutex_unlock(&pstPtcClientCB->pMutexPthread);
        }

        pt_logr_debug("CB state error , enState : %d", pstPtcClientCB->enState);

        return DOS_FAIL;
    }
    else
    {
        dos_list_add_tail(&pstPtcClientCB->stRecvBuffList, &(pstNewNode->stList));
        pstPtcClientCB->ulBuffNodeCount++;
        pt_logr_debug("add node into list succ, count : %d", pstPtcClientCB->ulBuffNodeCount);
    }
    if (ulCount < PTC_LOCK_MIN_COUNT)
    {
        pthread_mutex_unlock(&pstPtcClientCB->pMutexPthread);
    }

    return DOS_SUCC;
}

/**
 * 函数：void *ptc_recv_msg_from_web(void *arg)
 * 功能：
 *      1.线程  从web服务器接收数据
 * 参数
 * 返回值：
 */
void *ptc_recv_msg_from_web(void *arg)
{
    S8     cRecvBuf[PT_DATA_BUFF_16] = {0};
    S32    lRecvLen   = 0;
    U32    i          = 0;
    S32    lResult    = 0;
    struct timeval stTimeVal = {1, 0};
    struct timeval stTimeValCpy;
    fd_set ReadFds;
    U32 ulMaxFd     = 0;
    S32 lPipeFd     = -1;
    S8  *pcRecvBuf  = NULL;
    S32 lSocket     = 0;
    U32 ulMaxLoopCount = 10;

    /* 创建管道 */
    if(access(g_szFifoName, F_OK) == -1)
    {
        /* 管道文件不存在,创建命名管道 */
        lResult = mkfifo(g_szFifoName, 0777);
        if(lResult != 0)
        {
            logr_error("Could not create fifo %s\n", g_szFifoName);
            perror("mkfifo");
            DOS_ASSERT(0);
            return NULL;
        }
    }

    lPipeFd = open(g_szFifoName, O_RDONLY | O_NONBLOCK);
    g_lPipeWRFd = open(g_szFifoName, O_WRONLY);

    FD_ZERO(&g_stWebFdSet);
    FD_SET(lPipeFd, &g_stWebFdSet);
    g_ulWebSocketMax = lPipeFd;

    while (1)
    {
        stTimeValCpy = stTimeVal;
        FD_ZERO(&ReadFds);
        ReadFds = g_stWebFdSet;
        ulMaxFd = g_ulWebSocketMax;

        lResult = select(ulMaxFd + 1, &ReadFds, NULL, NULL, &stTimeValCpy);
        if (lResult < 0)
        {
            perror("fail to select");
            FD_ZERO(&g_stWebFdSet);
            FD_SET(lPipeFd, &g_stWebFdSet);
            g_ulWebSocketMax = lPipeFd;
            for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
            {
                if (g_astPtcConnects[i].enState == PTC_CLIENT_USING && g_astPtcConnects[i].enDataType == PT_DATA_WEB)
                {
                    FD_SET(g_astPtcConnects[i].lSocket, &g_stWebFdSet);
                    g_ulWebSocketMax = g_ulWebSocketMax > g_astPtcConnects[i].lSocket ? g_ulWebSocketMax : g_astPtcConnects[i].lSocket;
                }
            }
            continue;
        }
        else if (0 == lResult)
        {
            continue;
        }

        for (i=0; i<=PTC_STREAMID_MAX_COUNT; i++)
        {
            if (PTC_STREAMID_MAX_COUNT == i)
            {
                lSocket = lPipeFd;
            }
            else
            {
                if (g_astPtcConnects[i].enState != PTC_CLIENT_USING
                        || g_astPtcConnects[i].enDataType != PT_DATA_WEB
                        || g_astPtcConnects[i].lSocket <= 0)
                {
                    continue;
                }

                lSocket = g_astPtcConnects[i].lSocket;
            }

            if (!FD_ISSET(lSocket, &ReadFds))
            {
                continue;
            }
            if (lSocket == lPipeFd)
            {
                lRecvLen = read(lPipeFd, cRecvBuf, sizeof(cRecvBuf));
                continue;
            }

            ulMaxLoopCount = PTC_RECV_FROM_WEB_MAX_COUNT;
            while (ulMaxLoopCount--)
            {
                pcRecvBuf = (S8 *)dos_dmem_alloc(PT_RECV_DATA_SIZE);
                if (DOS_ADDR_INVALID(pcRecvBuf))
                {
                    /* 关闭该strream */
                    ptc_free_stream_resoure(PT_DATA_WEB, &g_astPtcConnects[i], g_astPtcConnects[i].ulStreamID, 1);

                    break;
                }

                lRecvLen = recv(lSocket, pcRecvBuf, PT_RECV_DATA_SIZE, 0);
                if (lRecvLen < 0)
                {
                    /* sockfd结束，通知pts */
                    pt_logr_info("recv socket fail, socket : %d, index : %d, result : %d", lSocket, i, lRecvLen);
                    FD_CLR(lSocket, &g_stWebFdSet);
                    ptc_free_stream_resoure(PT_DATA_WEB, &g_astPtcConnects[i], g_astPtcConnects[i].ulStreamID, 1);

                    break;
                }
                else if (lRecvLen == 0)
                {
                    /* sockfd结束，通知pts */
                    FD_CLR(lSocket, &g_stWebFdSet);
                }
                pt_logr_debug("ptc recv from web server len : %d, socket : %d", lRecvLen, lSocket);
                pthread_mutex_lock(&g_mutexPtcRecvMsgHandle);
                lResult = ptc_recvfrom_web_buff_list_insert(&g_astPtcConnects[i], pcRecvBuf, lRecvLen);
                if (lResult != DOS_SUCC)
                {
                    dos_dmem_free(pcRecvBuf);
                    pcRecvBuf = NULL;
                    pthread_mutex_unlock(&g_mutexPtcRecvMsgHandle);

                    ptc_free_stream_resoure(PT_DATA_WEB, &g_astPtcConnects[i], g_astPtcConnects[i].ulStreamID, 1);
                }
                else
                {
                    pthread_cond_signal(&g_condPtcRecvMsgHandle);
                    pthread_mutex_unlock(&g_mutexPtcRecvMsgHandle);
                }

                if (lRecvLen < PT_RECV_DATA_SIZE)
                {
                    break;
                }
            }

        } /* end of for */

    } /* end of while(1) */
}

void *ptc_handle_recvfrom_web_msg(void *arg)
{
    list_t *pstRecvBuffList = NULL;
    PTC_WEB_REV_MSG_HANDLE_ST *pstRecvBuffNode = NULL;
    S32 lResult = 0;
    struct timeval now;
    struct timespec timeout;
    S32     i       = 0;
    U32     ulCount = 0;
    BOOL    bIsFail = DOS_FALSE;

    while (1)
    {
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = now.tv_usec * 1000;

        pthread_mutex_lock(&g_mutexPtcRecvMsgHandle);
        pthread_cond_timedwait(&g_condPtcRecvMsgHandle, &g_mutexPtcRecvMsgHandle, &timeout);
        pthread_mutex_unlock(&g_mutexPtcRecvMsgHandle);

        for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
        {
            if (g_astPtcConnects[i].enState != PTC_CLIENT_USING
                || g_astPtcConnects[i].enDataType != PT_DATA_WEB
                || 0 == g_astPtcConnects[i].ulBuffNodeCount)
            {
                /* 不符合条件，跳过 */
                continue;
            }

            while (1)
            {
                if (dos_list_is_empty(&g_astPtcConnects[i].stRecvBuffList))
                {
                    break;
                }

                ulCount = g_astPtcConnects[i].ulBuffNodeCount;
                if (ulCount < PTC_LOCK_MIN_COUNT)
                {
                    pthread_mutex_lock(&g_astPtcConnects[i].pMutexPthread);
                }

                //pstRecvBuffList = dos_list_fetch(&g_astPtcConnects[i].stRecvBuffList);
                pstRecvBuffList = g_astPtcConnects[i].stRecvBuffList.next;
                if (pstRecvBuffList == &g_astPtcConnects[i].stRecvBuffList || DOS_ADDR_INVALID(pstRecvBuffList))
                {
                    if (ulCount < PTC_LOCK_MIN_COUNT)
                    {
                        pthread_mutex_unlock(&g_astPtcConnects[i].pMutexPthread);
                    }
                    break;
                }

                if (DOS_ADDR_INVALID(g_astPtcConnects[i].pstSendStreamNode))
                {
                    /* 找不到发送缓存 */
                    if (ulCount < PTC_LOCK_MIN_COUNT)
                    {
                        pthread_mutex_unlock(&g_astPtcConnects[i].pMutexPthread);
                    }
                    pt_logr_debug("can not found send stream cache, index: %d", i);
                    ptc_delete_client_by_index(&g_astPtcConnects[i]);

                    break;
                }

                if (!ptc_cache_is_or_not_have_space(g_astPtcConnects[i].pstSendStreamNode))
                {
                    if (ulCount < PTC_LOCK_MIN_COUNT)
                    {
                        pthread_mutex_unlock(&g_astPtcConnects[i].pMutexPthread);
                    }
                    pt_logr_debug("ptc cache not have space! streamID : %d", g_astPtcConnects[i].ulStreamID);

                    break;
                }
                else
                {
                    if (g_astPtcConnects[i].ulBuffNodeCount > 0)
                    {
                        g_astPtcConnects[i].ulBuffNodeCount--;
                    }
                    dos_list_del(pstRecvBuffList);

                    pt_logr_debug("index : %d, count : %d", i, g_astPtcConnects[i].ulBuffNodeCount);
                }

                if (ulCount < PTC_LOCK_MIN_COUNT)
                {
                    pthread_mutex_unlock(&g_astPtcConnects[i].pMutexPthread);
                }

                pstRecvBuffNode = dos_list_entry(pstRecvBuffList, PTC_WEB_REV_MSG_HANDLE_ST, stList);
                if (DOS_ADDR_INVALID(pstRecvBuffNode))
                {
                    break;
                }
                bIsFail = DOS_FALSE;

                lResult = ptc_save_msg_into_cache(g_astPtcConnects[i].pstSendStreamNode, PT_DATA_WEB, pstRecvBuffNode->ulStreamID, pstRecvBuffNode->paRecvBuff, pstRecvBuffNode->lRecvLen);
                if (PT_NEED_CUT_PTHREAD == lResult)
                {
                    ptc_set_cache_full_true(pstRecvBuffNode->lSocket, PT_DATA_WEB);
                    /* 添加回头部 */
                    pthread_mutex_lock(&g_astPtcConnects[i].pMutexPthread);
                    dos_list_add_head(&g_astPtcConnects[i].stRecvBuffList, pstRecvBuffList);
                    g_astPtcConnects[i].ulBuffNodeCount++;
                    pthread_mutex_unlock(&g_astPtcConnects[i].pMutexPthread);

                    break;
                }
                else if (PT_SAVE_DATA_FAIL == lResult)
                {
                    /* 添加失败 */
                    pt_logr_debug("save into send cache fail, stream : %d, index : %d", pstRecvBuffNode->ulStreamID, i);
                    bIsFail = DOS_TRUE;
                }

                if (0 == pstRecvBuffNode->lRecvLen)
                {
                    pt_logr_debug("save len is 0 into cache succ, stream : %d, index: %d", pstRecvBuffNode->ulStreamID, i);
                    ptc_delete_client_by_index(&g_astPtcConnects[i]);
                }

                if (DOS_ADDR_VALID(pstRecvBuffNode->paRecvBuff))
                {
                    dos_dmem_free(pstRecvBuffNode->paRecvBuff);
                    pstRecvBuffNode->paRecvBuff = NULL;
                }
                if (DOS_ADDR_VALID(pstRecvBuffNode))
                {
                    dos_dmem_free(pstRecvBuffNode);
                    pstRecvBuffNode = NULL;
                }

                if (bIsFail)
                {
                    /* 操作失败 */
                    ptc_free_stream_resoure(PT_DATA_WEB, &g_astPtcConnects[i], g_astPtcConnects[i].ulStreamID, 1);

                    break;
                }
            }
        }

    }

    return NULL;
}


/**
 * 函数：void ptc_send_msg2web(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
 * 功能：
 *      1.发送请求到web服务器
 * 参数
 * 返回值：
 */
void ptc_send_msg2web(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
{
    S32               lResult          = 0;
    S32               lSockfd          = 0;
    U32               ulArraySub       = 0;
    //socklen_t         optlen           = sizeof(S32);
    //S32               tcpinfo          = 0;
    PT_STREAM_CB_ST   *pstStreamList   = NULL;
    PT_STREAM_CB_ST   *pstStreamNode   = NULL;
    PT_DATA_TCP_ST    stRecvDataTcp;
    S32               lCurrSeq         = 0;
    S32               lIndex           = 0;

    pstStreamList = g_pstPtcRecv->pstStreamHead;
    if (DOS_ADDR_INVALID(pstStreamList))
    {
        return;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamList, pstNeedRecvNode->ulStreamID);
    if (DOS_ADDR_INVALID(pstStreamNode))
    {
        return;
    }

    if (DOS_ADDR_INVALID(pstStreamNode->unDataQueHead.pstDataTcp))
    {
        return;
    }

    while(1)
    {
        lCurrSeq = pstStreamNode->lCurrSeq + 1;
        ulArraySub = (lCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1);
        stRecvDataTcp = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
        if (stRecvDataTcp.lSeq == lCurrSeq)
        {
            lSockfd = ptc_get_socket_by_streamID(pstStreamNode->ulStreamID);
            if (DOS_FAIL == lSockfd)
            {
                lSockfd = ptc_create_socket_proxy(pstStreamNode->aulServIp, pstStreamNode->usServPort);
                if (lSockfd <= 0)
                {
                    /* 创建socket失败，通知pts结束 */
                    ptc_free_stream_resoure(PT_DATA_WEB, NULL, pstStreamNode->ulStreamID, 1);

                    break;
                }

                lResult = ptc_add_client(pstStreamNode->ulStreamID, lSockfd, PT_DATA_WEB, pstStreamNode);
                if (lResult != DOS_SUCC)
                {
                    /* socket 数量已满 */
                    close(lSockfd);
                    ptc_free_stream_resoure(PT_DATA_WEB, NULL, pstStreamNode->ulStreamID, 2);

                    break;
                }
                write(g_lPipeWRFd, "s", 1);
            }

#if 0
            if (getsockopt(lSockfd, IPPROTO_TCP, TCP_INFO, &tcpinfo, &optlen) < 0)
            {
                DOS_ASSERT(0);
                ptc_send_exit_notify_to_pts(PT_DATA_WEB, pstStreamNode->ulStreamID, 1);
                ptc_delete_stream_node_by_recv(pstStreamNode);
                ptc_delete_client_by_socket(lSockfd, PT_DATA_WEB);

                break;

            }

            if (TCP_CLOSE == tcpinfo || TCP_CLOSE_WAIT == tcpinfo || TCP_CLOSING == tcpinfo)
            {
                DOS_ASSERT(0);
                ptc_send_exit_notify_to_pts(PT_DATA_WEB, pstStreamNode->ulStreamID, 1);
                ptc_delete_stream_node_by_recv(pstStreamNode);
                ptc_delete_client_by_socket(lSockfd, PT_DATA_WEB);

                break;
            }
#endif
            pt_logr_debug("send msg to web server, stream : %d, seq : %d, len : %d", pstStreamNode->ulStreamID, pstStreamNode->lCurrSeq, stRecvDataTcp.ulLen);
            lResult = send(lSockfd, stRecvDataTcp.szBuff, stRecvDataTcp.ulLen, MSG_NOSIGNAL);
            if (lResult < stRecvDataTcp.ulLen)
            {
                /* 创建socket失败，通知pts结束 */
                DOS_ASSERT(0);
                lIndex = ptc_get_client_node_array_by_socket(lSockfd, PT_DATA_WEB);
                if (lIndex >= 0 && lIndex < PTC_STREAMID_MAX_COUNT)
                {
                    ptc_free_stream_resoure(PT_DATA_WEB, &g_astPtcConnects[lIndex], pstStreamNode->ulStreamID, 1);
                }
                else
                {
                    ptc_free_stream_resoure(PT_DATA_WEB, NULL, pstStreamNode->ulStreamID, 1);
                }

                break;
            }

            pstStreamNode->lCurrSeq = lCurrSeq;
        }
        else
        {
            break;
        }
    }

    return;
}

S32 ptc_printf_web_msg(U32 ulIndex, S32 argc, S8 **argv)
{
    S32 i = 0;
    U32 ulLen = 0;
    S8 szBuff[PT_DATA_BUFF_512] = {0};

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%10s%10s%10s%10s%10s\r\n", "index", "streamID", "socket", "state", "count");
    cli_out_string(ulIndex, szBuff);

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        snprintf(szBuff, sizeof(szBuff), "%10d%10d%10d%10d%10d\r\n", i, g_astPtcConnects[i].ulStreamID, g_astPtcConnects[i].lSocket, g_astPtcConnects[i].enState, g_astPtcConnects[i].ulBuffNodeCount);
        cli_out_string(ulIndex, szBuff);
    }

    return 0;

}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

