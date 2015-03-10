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
#include <limits.h>
#include <netdb.h>
#include <ctype.h>
/* include provate header file */
#include <pt/ptc.h>
#include <netinet/tcp.h>
#include "ptc_msg.h"

PTC_CLIENT_CB_ST g_austCmdClients[PTC_CMD_SOCKET_MAX_COUNT];

S32 ptc_cmd_get_socket_by_streamID(U32 ulStreamID)
{
    S32 i = 0;

    for (i=0; i<PTC_CMD_SOCKET_MAX_COUNT; i++)
    {
        if (g_austCmdClients[i].bIsValid && g_austCmdClients[i].ulStreamID == ulStreamID)
        {
            return g_austCmdClients[i].lSocket;
        }
    }

    return DOS_FAIL;
}

S32 ptc_cmd_get_streamID_by_socket(S32 lSocket)
{
    S32 i = 0;

    for (i=0; i<PTC_CMD_SOCKET_MAX_COUNT; i++)
    {
        if (g_austCmdClients[i].bIsValid && g_austCmdClients[i].lSocket == lSocket)
        {
            return g_austCmdClients[i].ulStreamID;
        }
    }

    return DOS_FAIL;
}

S32 ptc_cmd_add_client(U32 ulStreamID, S32 lSocket)
{
    S32 i = 0;

    for (i=0; i<PTC_CMD_SOCKET_MAX_COUNT; i++)
    {
        if (!g_austCmdClients[i].bIsValid)
        {
            g_austCmdClients[i].bIsValid = DOS_TRUE;
            g_austCmdClients[i].ulStreamID = ulStreamID;
            g_austCmdClients[i].lSocket = lSocket;
            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}

VOID ptc_cmd_delete_client(S32 lSocket)
{
    S32 i = 0;

    for (i=0; i<PTC_CMD_SOCKET_MAX_COUNT; i++)
    {
        if (g_austCmdClients[i].bIsValid && g_austCmdClients[i].lSocket == lSocket)
        {
            g_austCmdClients[i].bIsValid = DOS_FALSE;
            g_austCmdClients[i].lSocket = -1;

            /* 发送退出通知给pts */
            ptc_send_exit_notify_to_pts(PT_DATA_CMD, g_austCmdClients[i].ulStreamID);
            return;
        }
    }
}

/**
 * 函数：void *ptc_recv_msg_from_cmd(void *arg)
 * 功能：
 *      1.线程  从cmd服务器接收数据
 * 参数
 * 返回值：
 */
void *ptc_recv_msg_from_cmd(void *arg)
{
    S8     cRecvBuf[PT_RECV_DATA_SIZE] = {0};
    S32    lRecvLen   = 0;
    U32    i          = 0;
    S32    lResult    = 0;
    struct timeval stTimeVal = {1, 0};
    struct timeval stTimeValCpy;
    fd_set ReadFds;
    U32 ulMaxFd = 0;
    S32 lStreamID = 0;

    while (1)
    {
        stTimeValCpy = stTimeVal;
        FD_ZERO(&ReadFds);
        ulMaxFd = 0;

        for (i=0; i<PTC_CMD_SOCKET_MAX_COUNT; i++)
        {
            if (g_austCmdClients[i].bIsValid)
            {
                FD_SET(g_austCmdClients[i].lSocket, &ReadFds);
                ulMaxFd = ulMaxFd > g_austCmdClients[i].lSocket ? ulMaxFd : g_austCmdClients[i].lSocket;
            }
        }

        lResult = select(ulMaxFd + 1, &ReadFds, NULL, NULL, &stTimeValCpy);
        if (lResult < 0)
        {
            perror("fail to select");
            exit(DOS_FAIL);
        }
        else if (0 == lResult)
        {
            continue;
        }

        for (i=0; i<=ulMaxFd; i++)
        {
            if (FD_ISSET(i, &ReadFds))
            {
                dos_memzero(cRecvBuf, PT_RECV_DATA_SIZE);
                lRecvLen = recv(i, cRecvBuf, PT_RECV_DATA_SIZE, 0);

                pt_logr_debug("ptc recv from telnet server len : %d",lRecvLen);

                lStreamID = ptc_cmd_get_streamID_by_socket(i);
                if (DOS_FAIL == lStreamID)
                {
                    pt_logr_debug("not found socket, streamID is %d", i);
                    continue;
                }

                if (lRecvLen == 0)
                {
                    /* sockfd结束，通知pts */
                    pt_logr_debug("sockfd end, streamID is %d", lStreamID);
                    /* 清除收发缓存 */
                    ptc_delete_send_stream_node(lStreamID, PT_DATA_CMD, DOS_TRUE);
                    ptc_delete_recv_stream_node(lStreamID, PT_DATA_CMD, DOS_TRUE);
                    ptc_cmd_delete_client(i);
                    close(i);

                    FD_CLR(i, &ReadFds);
                }
                else if (lRecvLen < 0)
                {
                    pt_logr_info("fail to recv, streamID is %d", i);
                    ptc_delete_send_stream_node(lStreamID, PT_DATA_CMD, DOS_TRUE);
                    ptc_delete_recv_stream_node(lStreamID, PT_DATA_CMD, DOS_TRUE);
                    ptc_cmd_delete_client(i);
                    close(i);
                }
                else
                {
                    #if PTC_DEBUG
                    S32 j = 0;
                    for (j=0; j<lRecvLen; j++)
                    {
                        if (j%16 == 0)
                        {
                            printf("\n");
                        }
                        printf("%02x ", (U8)(cRecvBuf[j]));
                    }
                    printf("\n-------------------------------------\n");
                    #endif

                    ptc_save_msg_into_cache(PT_DATA_CMD, lStreamID, cRecvBuf, lRecvLen);

                }
            } /* end of  if (FD_ISSET(i, &ReadFds)) */
        } /* end of for */
    } /* end of while(1) */
}

/**
 * 函数：void ptc_send_msg2cmd(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
 * 功能：
 *      1.发送数据到cmd服务器
 * 参数
 * 返回值：
 */
void ptc_send_msg2cmd(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
{
    S32              lSockfd          = 0;
    U32              ulArraySub       = 0;
    S32              lSendCount       = 0;
    list_t          *pstStreamList    = NULL;
    PT_STREAM_CB_ST *pstStreamNode    = NULL;
    PT_DATA_TCP_ST   stRecvDataTcp;
    S32              lResult          = 0;
    socklen_t optlen = sizeof(S32);
    S32 tcpinfo;

    pstStreamList = g_pstPtcRecv->astDataTypes[pstNeedRecvNode->enDataType].pstStreamQueHead;
    if (NULL == pstStreamList)
    {
        pt_logr_info("send data to proxy : stream list head is NULL, cann't get data package\n");
        return;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamList, pstNeedRecvNode->ulStreamID);
    if (NULL == pstStreamNode)
    {
        pt_logr_info("send data to proxy : stream node cann't found : %d\n", pstNeedRecvNode->ulStreamID);
        return;
    }

    if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
    {
        pt_logr_info("send data to proxy : data queue is NULL\n");
        return;
    }
    while (1)
    {
        pstStreamNode->lCurrSeq++;
        ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1);
        stRecvDataTcp = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];

        if (stRecvDataTcp.lSeq == pstStreamNode->lCurrSeq)
        {
            lSockfd = ptc_cmd_get_socket_by_streamID(pstStreamNode->ulStreamID);
            if (DOS_FAIL == lSockfd)
            {
                lSockfd = ptc_create_socket_proxy(pstStreamNode->aulServIp, pstStreamNode->usServPort);
                if (lSockfd <= 0)
                {
                    /* create sockfd fail */
                    ptc_send_exit_notify_to_pts(PT_DATA_CMD, pstStreamNode->ulStreamID);
                    return;
                }
                lResult = ptc_cmd_add_client(pstStreamNode->ulStreamID, lSockfd);
                if (lResult != DOS_SUCC)
                {
                    /* socket 数量已满 */
                    return;
                }
            }

            if (getsockopt(lSockfd, IPPROTO_TCP, TCP_INFO, &tcpinfo, &optlen) < 0)
            {
                pt_logr_info("get info fail");
                exit(1);
            }

            if (TCP_CLOSE == tcpinfo || TCP_CLOSE_WAIT == tcpinfo || TCP_CLOSING == tcpinfo)
            {
                break;
            }

            lSendCount = send(lSockfd, stRecvDataTcp.szBuff, stRecvDataTcp.ulLen, 0);
            pt_logr_debug("ptc send to telnet server, stream : %d, len : %d, lSendCount = %d, buff : %s", pstStreamNode->ulStreamID, stRecvDataTcp.ulLen, lSendCount, stRecvDataTcp.szBuff);
        }
        else
        {
            pstStreamNode->lCurrSeq--; /* 这个seq没有发送，减一 */
            break;
        }
    }
    return;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

