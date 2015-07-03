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
#include <semaphore.h>
#include <sys/time.h>
/* include provate header file */
#include <pt/ptc.h>
#include <netinet/tcp.h>
#include "ptc_msg.h"
#include "ptc_telnet.h"

extern S32 cli_cmd_get_mem_info(S8 *szInfoBuff, U32 ulBuffSize);
PT_PTC_RECV_CMD_ST g_austPtcRecvCmds[PTC_RECV_FROM_PTS_CMD_SIZE];
sem_t g_SemCmdDispose;

U32 g_ulCmdSocketMax = 0;
fd_set g_stCmdFdSet;

S32 ptc_save_into_cmd_array(PT_PTC_COMMAND_ST *pstCommand)
{
    if (NULL == pstCommand)
    {
        return DOS_FALSE;
    }

    S32 i = 0;

    for (i=0; i<PTC_RECV_FROM_PTS_CMD_SIZE; i++)
    {
        if (g_austPtcRecvCmds[i].bIsValid == DOS_FALSE)
        {
            dos_strcpy(g_austPtcRecvCmds[i].szCommand, pstCommand->szCommand);
            g_austPtcRecvCmds[i].ulIndex = pstCommand->ulIndex;
            g_austPtcRecvCmds[i].bIsValid = DOS_TRUE;

            return DOS_SUCC;
        }
    }

    return DOS_FALSE;
}

void *ptc_deal_with_pts_command(void *arg)
{
    S32 i = 0 ;
    struct timespec stSemTime;
    struct timeval now;
    PT_PTC_COMMAND_ST stPtcCommand;
    S8 *szInfoBuff = NULL;
    S8 *szInfoBuffCpy = NULL;
    U32 ulInfoBuffLen = 0;

    S8 szCmdAck[PT_RECV_DATA_SIZE] = {0};

    for (i=0; i<PTC_RECV_FROM_PTS_CMD_SIZE; i++)
    {
        g_austPtcRecvCmds[i].bIsValid = DOS_FALSE;
    }

    while (1)
    {
        gettimeofday(&now, NULL);
        stSemTime.tv_sec = now.tv_sec + 5;
        stSemTime.tv_nsec = now.tv_usec * 1000;
        sem_timedwait(&g_SemCmdDispose, &stSemTime);

        for (i = 0; i<PTC_RECV_FROM_PTS_CMD_SIZE; i++)
        {
            if (g_austPtcRecvCmds[i].bIsValid == DOS_TRUE)
            {
                /* 处理命令 */
                szInfoBuff = (S8 *)dos_dmem_alloc(PT_DATA_BUFF_2048);
                if (NULL == szInfoBuff)
                {
                    DOS_ASSERT(0);
                    continue;
                }
#ifndef ARM_VERSION
                cli_cmd_get_mem_info(szInfoBuff, PT_DATA_BUFF_2048);
#endif
                ulInfoBuffLen = dos_strlen(szInfoBuff);
                szInfoBuffCpy = szInfoBuff;


                dos_strcpy(stPtcCommand.szCommand, g_austPtcRecvCmds[i].szCommand);
                stPtcCommand.ulIndex = g_austPtcRecvCmds[i].ulIndex;
                dos_memzero(szCmdAck, PT_RECV_DATA_SIZE);
                dos_memcpy(szCmdAck, &stPtcCommand, sizeof(PT_PTC_COMMAND_ST));

                while (ulInfoBuffLen > 0)
                {
                    if (ulInfoBuffLen > PT_RECV_DATA_SIZE - sizeof(PT_PTC_COMMAND_ST))
                    {
                        dos_memcpy(szCmdAck+sizeof(PT_PTC_COMMAND_ST), szInfoBuffCpy, PT_RECV_DATA_SIZE - sizeof(PT_PTC_COMMAND_ST));
                        ptc_save_msg_into_cache(NULL, PT_DATA_CMD, PT_CTRL_COMMAND, szCmdAck, PT_RECV_DATA_SIZE);
                        szInfoBuffCpy += (PT_RECV_DATA_SIZE - sizeof(PT_PTC_COMMAND_ST));
                        ulInfoBuffLen -= (PT_RECV_DATA_SIZE - sizeof(PT_PTC_COMMAND_ST));
                    }
                    else
                    {
                        dos_memcpy(szCmdAck+sizeof(PT_PTC_COMMAND_ST), szInfoBuffCpy, ulInfoBuffLen);
                        ptc_save_msg_into_cache(NULL, PT_DATA_CMD, PT_CTRL_COMMAND, szCmdAck, sizeof(PT_PTC_COMMAND_ST)+ ulInfoBuffLen);
                        ulInfoBuffLen = 0;
                    }
                }
                g_austPtcRecvCmds[i].bIsValid = DOS_FALSE;
                dos_dmem_free(szInfoBuff);
            }
        }
    }

    return NULL;
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
    S32 lSocket = 0;
    FD_ZERO(&g_stCmdFdSet);
    g_ulCmdSocketMax = 0;

    while (1)
    {
        stTimeValCpy = stTimeVal;
        FD_ZERO(&ReadFds);
        ulMaxFd = g_ulCmdSocketMax;
        ReadFds = g_stCmdFdSet;

        lResult = select(ulMaxFd + 1, &ReadFds, NULL, NULL, &stTimeValCpy);
        if (lResult < 0)
        {
            perror("fail to select");
            FD_ZERO(&g_stCmdFdSet);
            g_ulCmdSocketMax = 0;
            for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
            {
                if (g_astPtcConnects[i].enState == PTC_CLIENT_USING && g_astPtcConnects[i].enDataType == PT_DATA_CMD)
                {
                    FD_SET(g_astPtcConnects[i].lSocket, &g_stCmdFdSet);
                    g_ulCmdSocketMax = g_ulCmdSocketMax > g_astPtcConnects[i].lSocket ? g_ulCmdSocketMax : g_astPtcConnects[i].lSocket;
                }
            }
            continue;
        }
        else if (0 == lResult)
        {
            continue;
        }

        for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
        {
            if (g_astPtcConnects[i].enState != PTC_CLIENT_USING
                || g_astPtcConnects[i].enDataType != PT_DATA_CMD
                || g_astPtcConnects[i].lSocket <= 0)
            {
                continue;
            }

            lSocket = g_astPtcConnects[i].lSocket;
            lStreamID = g_astPtcConnects[i].ulStreamID;

            if (FD_ISSET(lSocket, &ReadFds))
            {
                dos_memzero(cRecvBuf, PT_RECV_DATA_SIZE);
                lRecvLen = recv(lSocket, cRecvBuf, PT_RECV_DATA_SIZE, 0);
                pt_logr_info("ptc recv from telnet server len : %d", lRecvLen);
                if (lRecvLen <= 0)
                {
                    ptc_delete_send_stream_node_by_streamID(lStreamID);
                    FD_CLR(lSocket, &g_stCmdFdSet);
                    close(lSocket);
                    ptc_free_stream_resoure(PT_DATA_CMD, &g_astPtcConnects[i], lStreamID, 0);
                }
                else
                {
                    ptc_save_msg_into_cache(g_astPtcConnects[i].pstSendStreamNode, PT_DATA_CMD, lStreamID, cRecvBuf, lRecvLen);

                }
            } /* end of  if (FD_ISSET(i, &ReadFds)) */
        } /* end of for */
    } /* end of while(1) */

    return NULL;
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
    PT_STREAM_CB_ST *pstStreamList    = NULL;
    PT_STREAM_CB_ST *pstStreamNode    = NULL;
    PT_DATA_TCP_ST  *pstRecvDataTcp   = NULL;
    S32              lResult          = 0;
    //socklen_t optlen = sizeof(S32);
    //S32 tcpinfo;
    PT_PTC_COMMAND_ST *pstCommand = NULL;
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

    while (1)
    {
        lCurrSeq = pstStreamNode->lCurrSeq + 1;
        ulArraySub = (lCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1);
        pstRecvDataTcp = &pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
        if (pstRecvDataTcp->lSeq == lCurrSeq)
        {
            if (PT_CTRL_COMMAND == pstStreamNode->ulStreamID)
            {
                /* pts 发送的命令 */
                pstCommand = (PT_PTC_COMMAND_ST *)pstRecvDataTcp->szBuff;
                lResult = ptc_save_into_cmd_array(pstCommand);
                if (lResult != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                }
                else
                {
                    sem_post(&g_SemCmdDispose);
                }

                continue;
            }


            lSockfd = ptc_get_socket_by_streamID(pstStreamNode->ulStreamID);
            if (DOS_FAIL == lSockfd)
            {
                lSockfd = ptc_create_socket_proxy(pstStreamNode->aulServIp, pstStreamNode->usServPort);
                if (lSockfd <= 0)
                {
                    /* create sockfd fail */
                    pt_logr_info("create socket fail, stream : %d", pstStreamNode->ulStreamID);
                    ptc_free_stream_resoure(PT_DATA_CMD, NULL, pstStreamNode->ulStreamID, 1);

                    return;
                }
                lResult = ptc_add_client(pstStreamNode->ulStreamID, lSockfd, PT_DATA_CMD, pstStreamNode);
                if (lResult != DOS_SUCC)
                {
                    /* socket 数量已满 */
                    close(lSockfd);
                    ptc_free_stream_resoure(PT_DATA_CMD, NULL, pstStreamNode->ulStreamID, 2);

                    return;
                }
            }
#if 0
            if (getsockopt(lSockfd, IPPROTO_TCP, TCP_INFO, &tcpinfo, &optlen) < 0)
            {
                DOS_ASSERT(0);
                ptc_delete_client_by_socket(lSockfd, PT_DATA_CMD);
                ptc_send_exit_notify_to_pts(PT_DATA_CMD, pstStreamNode->ulStreamID, 3);
                ptc_delete_stream_node_by_recv(pstStreamNode);
                break;
            }

            if (TCP_CLOSE == tcpinfo || TCP_CLOSE_WAIT == tcpinfo || TCP_CLOSING == tcpinfo)
            {
                DOS_ASSERT(0);
                ptc_delete_client_by_socket(lSockfd, PT_DATA_CMD);
                ptc_send_exit_notify_to_pts(PT_DATA_CMD, pstStreamNode->ulStreamID, 3);
                ptc_delete_stream_node_by_recv(pstStreamNode);
                break;
            }
#endif
            lSendCount = send(lSockfd, pstRecvDataTcp->szBuff, pstRecvDataTcp->ulLen, MSG_NOSIGNAL);
            if (lSendCount != pstRecvDataTcp->ulLen || lSendCount < 0)
            {
                /* 创建socket失败，通知pts结束 */
                DOS_ASSERT(0);
                lIndex = ptc_get_client_node_array_by_socket(lSockfd, PT_DATA_CMD);
                if (lIndex >= 0 && lIndex < PTC_STREAMID_MAX_COUNT)
                {
                    ptc_free_stream_resoure(PT_DATA_CMD, &g_astPtcConnects[lIndex], pstStreamNode->ulStreamID, 3);
                }
                else
                {
                    ptc_free_stream_resoure(PT_DATA_CMD, NULL, pstStreamNode->ulStreamID, 3);
                }

                break;
            }
            pstStreamNode->lCurrSeq = lCurrSeq;
            pt_logr_debug("ptc send to telnet server, stream : %d, len : %d, result = %d", pstStreamNode->ulStreamID, pstRecvDataTcp->ulLen, lSendCount);

        }
        else
        {
            break;
        }
    }

    return;
}

S32 ptc_printf_telnet_msg(U32 ulIndex, S32 argc, S8 **argv)
{
    S32 i = 0;
    U32 ulLen = 0;
    S8 szBuff[PT_DATA_BUFF_512] = {0};

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%10s%10s%10s\r\n", "StreamID", "Socket", "state");
    cli_out_string(ulIndex, szBuff);

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        snprintf(szBuff, sizeof(szBuff), "%10d%10d%10d\r\n", g_astPtcConnects[i].ulStreamID, g_astPtcConnects[i].lSocket, g_astPtcConnects[i].enState);
        cli_out_string(ulIndex, szBuff);
    }

    return 0;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

