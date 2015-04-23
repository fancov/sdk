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
PTC_CLIENT_CB_ST g_austCmdClients[PTC_CMD_SOCKET_MAX_COUNT];
PT_PTC_RECV_CMD_ST g_austPtcRecvCmds[PTC_RECV_FROM_PTS_CMD_SIZE];
sem_t g_SemCmdDispose;

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
            ptc_send_exit_notify_to_pts(PT_DATA_CMD, g_austCmdClients[i].ulStreamID, 0);
            g_austCmdClients[i].ulStreamID = U32_BUTT;
            return;
        }
    }
}

VOID ptc_cmd_delete_client_by_stream(U32 ulStreamID)
{
    S32 i = 0;

    for (i=0; i<PTC_CMD_SOCKET_MAX_COUNT; i++)
    {
        if (g_austCmdClients[i].bIsValid && g_austCmdClients[i].ulStreamID == ulStreamID)
        {
            g_austCmdClients[i].bIsValid = DOS_FALSE;
            g_austCmdClients[i].lSocket = -1;
            g_austCmdClients[i].ulStreamID = U32_BUTT;

            return;
        }
    }
}

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
                        ptc_save_msg_into_cache(PT_DATA_CMD, PT_CTRL_COMMAND, szCmdAck, PT_RECV_DATA_SIZE);
                        szInfoBuffCpy += (PT_RECV_DATA_SIZE - sizeof(PT_PTC_COMMAND_ST));
                        ulInfoBuffLen -= (PT_RECV_DATA_SIZE - sizeof(PT_PTC_COMMAND_ST));
                    }
                    else
                    {
                        dos_memcpy(szCmdAck+sizeof(PT_PTC_COMMAND_ST), szInfoBuffCpy, ulInfoBuffLen);
                        ptc_save_msg_into_cache(PT_DATA_CMD, PT_CTRL_COMMAND, szCmdAck, sizeof(PT_PTC_COMMAND_ST)+ ulInfoBuffLen);
                        ulInfoBuffLen = 0;
                    }
                }
                g_austPtcRecvCmds[i].bIsValid = DOS_FALSE;
                dos_dmem_free(szInfoBuff);
            }
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
            DOS_ASSERT(0);
            perror("fail to select");
            //exit(DOS_FAIL);
            sleep(1);
            continue;
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

                pt_logr_info("ptc recv from telnet server len : %d", lRecvLen);

                lStreamID = ptc_cmd_get_streamID_by_socket(i);
                if (DOS_FAIL == lStreamID)
                {
                    pt_logr_debug("not found socket, streamID is %d", i);
                    continue;
                }

                if (lRecvLen <= 0)
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
                else
                {
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
    PT_DATA_TCP_ST  *pstRecvDataTcp   = NULL;
    S32              lResult          = 0;
    socklen_t optlen = sizeof(S32);
    S32 tcpinfo;
    PT_PTC_COMMAND_ST *pstCommand = NULL;

    if (pstNeedRecvNode->ExitNotifyFlag)
    {
        ptc_cmd_delete_client_by_stream(pstNeedRecvNode->ulStreamID);

        return;
    }

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
        pstRecvDataTcp = &pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
        if (pstRecvDataTcp->lSeq == pstStreamNode->lCurrSeq)
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


            lSockfd = ptc_cmd_get_socket_by_streamID(pstStreamNode->ulStreamID);
            if (DOS_FAIL == lSockfd)
            {
                lSockfd = ptc_create_socket_proxy(pstStreamNode->aulServIp, pstStreamNode->usServPort);
                if (lSockfd <= 0)
                {
                    /* create sockfd fail */
                    printf("create socket fail, stream : %d\n", pstStreamNode->ulStreamID);
                    ptc_send_exit_notify_to_pts(PT_DATA_CMD, pstStreamNode->ulStreamID, 1);
                    ptc_delete_recv_stream_node(pstStreamNode->ulStreamID, PT_DATA_CMD, DOS_FALSE);
                    return;
                }
                lResult = ptc_cmd_add_client(pstStreamNode->ulStreamID, lSockfd);
                if (lResult != DOS_SUCC)
                {
                    /* socket 数量已满 */
                    close(lSockfd);
                    ptc_send_exit_notify_to_pts(PT_DATA_CMD, pstStreamNode->ulStreamID, 2);
                    ptc_delete_recv_stream_node(pstStreamNode->ulStreamID, PT_DATA_CMD, DOS_FALSE);

                    return;
                }
            }

            if (getsockopt(lSockfd, IPPROTO_TCP, TCP_INFO, &tcpinfo, &optlen) < 0)
            {
                DOS_ASSERT(0);
                close(lSockfd);
                ptc_cmd_delete_client(lSockfd);
                ptc_send_exit_notify_to_pts(PT_DATA_CMD, pstStreamNode->ulStreamID, 3);
                ptc_delete_recv_stream_node(pstStreamNode->ulStreamID, PT_DATA_CMD, DOS_FALSE);
                break;
            }

            if (TCP_CLOSE == tcpinfo || TCP_CLOSE_WAIT == tcpinfo || TCP_CLOSING == tcpinfo)
            {
                DOS_ASSERT(0);
                close(lSockfd);
                ptc_cmd_delete_client(lSockfd);
                ptc_send_exit_notify_to_pts(PT_DATA_CMD, pstStreamNode->ulStreamID, 3);
                ptc_delete_recv_stream_node(pstStreamNode->ulStreamID, PT_DATA_CMD, DOS_FALSE);
                break;
            }

            lSendCount = send(lSockfd, pstRecvDataTcp->szBuff, pstRecvDataTcp->ulLen, 0);
            printf("ptc send to telnet server, stream : %d, len : %d, result = %d\n\n", pstStreamNode->ulStreamID, pstRecvDataTcp->ulLen, lSendCount);

        }
        else
        {
            pstStreamNode->lCurrSeq--; /* 这个seq没有发送，减一 */
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

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%10s%10s%10s\r\n", "ulStreamID", "lSocket", "bIsValid");
    cli_out_string(ulIndex, szBuff);

    for (i=0; i<PTC_CMD_SOCKET_MAX_COUNT; i++)
    {
        snprintf(szBuff, sizeof(szBuff), "%10d%10d%10d\r\n", g_austCmdClients[i].ulStreamID, g_austCmdClients[i].lSocket, g_austCmdClients[i].bIsValid);
        cli_out_string(ulIndex, szBuff);
    }

    return 0;

}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

