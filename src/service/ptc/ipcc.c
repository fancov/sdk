#ifdef  __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <ctype.h>

#include "ipcc_msg.h"

fd_set   m_ReadFds;                       /*请求ipcc proxy的套接字集合*/
S32      g_MaxFdp = 0;                    /*m_ReadFds套接字集合的最大值*/
BOOL     g_bIsOnLine;                     /*设备是否在线*/

void ptc_init_serv_msg()
{
    struct in_addr stInAddr;
    inet_pton(AF_INET, "192.168.1.251", (void *)&stInAddr);
    g_stServMsg.achProxyIP[0] = stInAddr.s_addr;
    inet_pton(AF_INET, "192.168.1.251", (void *)&stInAddr);
    g_stServMsg.achServIP[0] = stInAddr.s_addr;
    g_stServMsg.usProxyPort = 80;
    g_stServMsg.usServPort = 8200;

    return;
}

void ptc_init_send_cache_queue(S32 lSocket, U8 *pcID)
{
    struct sockaddr_in stDestAddr;
    g_stCCCBSend = (PT_CC_CB_ST *)malloc(sizeof(PT_CC_CB_ST));
    if (NULL == g_stCCCBSend)
    {
        perror("malloc");
        exit(DOS_FAIL);
    }

    if (NULL == pcID)
    {
        memcpy(g_stCCCBSend->aucID, "123456789abcdefg", IPCC_ID_LEN);
    }
    else
    {
        memcpy(g_stCCCBSend->aucID, pcID, IPCC_ID_LEN);
    }

    bzero(&stDestAddr, sizeof(stDestAddr));
    stDestAddr.sin_family = AF_INET;
    stDestAddr.sin_port   = htons(g_stServMsg.usServPort);
    stDestAddr.sin_addr.s_addr = g_stServMsg.achServIP[0];

    g_stCCCBSend->lSocket = lSocket;
    g_stCCCBSend->stDestAddr = stDestAddr;

    g_stCCCBSend->astDataTypes[PT_DATA_CTRL].enDataType = PT_DATA_CTRL;
    g_stCCCBSend->astDataTypes[PT_DATA_CTRL].bValid = DOS_TRUE;
    g_stCCCBSend->astDataTypes[PT_DATA_CTRL].pstStreamQueHead = NULL;
    g_stCCCBSend->astDataTypes[PT_DATA_CTRL].ulStreamNumInQue = 0;
    g_stCCCBSend->astDataTypes[PT_DATA_WEB].enDataType = PT_DATA_WEB;
    g_stCCCBSend->astDataTypes[PT_DATA_WEB].bValid = DOS_TRUE;
    g_stCCCBSend->astDataTypes[PT_DATA_WEB].pstStreamQueHead = NULL;
    g_stCCCBSend->astDataTypes[PT_DATA_WEB].ulStreamNumInQue = 0;
    g_stCCCBSend->astDataTypes[PT_DATA_CMD].enDataType = PT_DATA_CMD;
    g_stCCCBSend->astDataTypes[PT_DATA_CMD].bValid = DOS_TRUE;
    g_stCCCBSend->astDataTypes[PT_DATA_CMD].pstStreamQueHead = NULL;
    g_stCCCBSend->astDataTypes[PT_DATA_CMD].ulStreamNumInQue = 0;

    return;
}


void ptc_init_recv_cache_queue(S32 lSocket, U8 *pcID)
{
    struct sockaddr_in stDestAddr;
    g_stCCCBRecv = (PT_CC_CB_ST *)malloc(sizeof(PT_CC_CB_ST));
    if (NULL == g_stCCCBSend)
    {
        perror("malloc");
        exit(DOS_FAIL);
    }

    if (NULL == pcID)
    {
        memcpy(g_stCCCBSend->aucID, "123456789abcdefg", IPCC_ID_LEN);
    }
    else
    {
        memcpy(g_stCCCBSend->aucID, pcID, IPCC_ID_LEN);
    }

    bzero(&stDestAddr, sizeof(stDestAddr));
    stDestAddr.sin_family = AF_INET;
    stDestAddr.sin_port   = htons(g_stServMsg.usServPort);
    stDestAddr.sin_addr.s_addr = g_stServMsg.achServIP[0];

    g_stCCCBRecv->lSocket = lSocket;
    g_stCCCBRecv->stDestAddr = stDestAddr;

    g_stCCCBRecv->astDataTypes[PT_DATA_CTRL].enDataType = PT_DATA_CTRL;
    g_stCCCBRecv->astDataTypes[PT_DATA_CTRL].bValid = DOS_TRUE;
    g_stCCCBRecv->astDataTypes[PT_DATA_CTRL].pstStreamQueHead = NULL;
    g_stCCCBRecv->astDataTypes[PT_DATA_CTRL].ulStreamNumInQue = 0;
    g_stCCCBRecv->astDataTypes[PT_DATA_WEB].enDataType = PT_DATA_WEB;
    g_stCCCBRecv->astDataTypes[PT_DATA_WEB].bValid = DOS_TRUE;
    g_stCCCBRecv->astDataTypes[PT_DATA_WEB].pstStreamQueHead = NULL;
    g_stCCCBRecv->astDataTypes[PT_DATA_WEB].ulStreamNumInQue = 0;
    g_stCCCBRecv->astDataTypes[PT_DATA_CMD].enDataType = PT_DATA_CMD;
    g_stCCCBRecv->astDataTypes[PT_DATA_CMD].bValid = DOS_TRUE;
    g_stCCCBRecv->astDataTypes[PT_DATA_CMD].pstStreamQueHead = NULL;
    g_stCCCBRecv->astDataTypes[PT_DATA_CMD].ulStreamNumInQue = 0;

    return;
}


S32 ptc_create_socket_proxy(U32 *alServIp, U16 usServPort)
{
    S32 lSockfd = 0;
    struct sockaddr_in stServerAddr;

    if((lSockfd = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
    {
        alert("create socket error");
        return DOS_FAIL;
    }

    bzero(&stServerAddr, sizeof(stServerAddr));
    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_port = htons(usServPort);
    stServerAddr.sin_addr.s_addr = alServIp[0];

    if(connect(lSockfd, (struct sockaddr*)(&stServerAddr), sizeof(stServerAddr)) < 0)
    {
        alert("create socket");
        return DOS_FAIL;
    }

    return lSockfd;
}


void *ptc_send_msg2proxy(void *arg)
{
    S32               lSockfd         = 0;
    U32               ulArraySub      = 0;
    list_t            *pstStreamList  = NULL;
    PT_STREAM_CB_ST   *pstStreamNode  = NULL;
    PT_DATA_TCP_ST    stRecvDataTcp;
    S8                acMsgHead[PTC_DATA_BUFF_512]     = {0};
    PT_MSG_ST         *pstMsgDes                      = NULL;

    FD_ZERO(&m_ReadFds);

    while (1)
    {
        pthread_mutex_lock(&g_mutex_recv);
        pthread_cond_wait(&g_cond_recv, &g_mutex_recv);

        pstStreamList = g_stCCCBRecv->astDataTypes[g_enDataTypeRecv].pstStreamQueHead;
        if (NULL == pstStreamList)
        {
            alert("not found stream");
        }
        else
        {
            pstStreamNode = pt_stream_queue_serach(pstStreamList, g_ulStreamIDRecv);
            if (NULL == pstStreamNode)
            {
                alert("not found stream");
            }
            else
            {
                if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
                {
                    alert("not found stream");
                }
                else
                {
                    ulArraySub = (pstStreamNode->ulCurrSeq + 1) & (PT_DATA_RECV_CACHE_SIZE - 1);
                    stRecvDataTcp = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
                    while (stRecvDataTcp.bIsSaveMsg && stRecvDataTcp.lSeq > pstStreamNode->ulCurrSeq)
                    {
                        memcpy(acMsgHead, stRecvDataTcp.szBuff, sizeof(PT_MSG_ST));
                        pstMsgDes = (PT_MSG_ST *)acMsgHead;

                        if (0 == pstStreamNode->lSockFd)
                        {
                            lSockfd = ptc_create_socket_proxy(pstMsgDes->aulServIp, pstMsgDes->usServPort);
                            if (DOS_FAIL == lSockfd)
                            {
                                alert("create sockfd fail");
                                continue;
                            }

                            FD_SET(lSockfd, &m_ReadFds);
                            g_MaxFdp = g_MaxFdp > lSockfd ? g_MaxFdp : lSockfd;
                            pstStreamNode->lSockFd = lSockfd;
                        }

                        alert("request size: %d", stRecvDataTcp.ulLen);
                        send(pstStreamNode->lSockFd, stRecvDataTcp.szBuff + sizeof(PT_MSG_ST), stRecvDataTcp.ulLen - sizeof(PT_MSG_ST), 0);

                        pstStreamNode->ulCurrSeq++;
                        ulArraySub = (pstStreamNode->ulCurrSeq + 1) & (PT_DATA_RECV_CACHE_SIZE - 1);
                        stRecvDataTcp = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
                    }

                }

            }

        }

        pthread_mutex_unlock(&g_mutex_recv);

   }

}


void *ptc_recv_msg_from_proxy(void *arg)
{
    fd_set ReadFdsCpio;
    S8     cRecvBuf[PT_RECV_DATA_SIZE] = {0};
    S32    lRecvLen   = 0;
    U32    i          = 0;
    struct timeval tv = {1, 0};
    PT_STREAM_CB_ST *pstNode = NULL;

    while (1)
    {
        ReadFdsCpio = m_ReadFds;
        if (select(g_MaxFdp + 1, &ReadFdsCpio, NULL, NULL, &tv) < 0)
        {
            perror("fail to select");
            exit(DOS_FAIL);
        }
        for (i=0; i<=g_MaxFdp; i++)
        {
            if (FD_ISSET(i, &ReadFdsCpio))
            {
                 memset(cRecvBuf, 0, PT_RECV_DATA_SIZE);
                 lRecvLen = recv(i, cRecvBuf, PT_RECV_DATA_SIZE, 0);
                 if (lRecvLen == 0)
                 {
                    alert("peer closed");
                    FD_CLR(i, &m_ReadFds);
                    pstNode = pt_stream_queue_serach_by_sockfd(g_stCCCBRecv, i);
                    if(NULL == pstNode)
                    {
                        alert("not found");
                    }
                    else
                    {
                        /*sockfd结束，通知amc*/
                        ptc_ipcc_send_msg(PT_DATA_WEB, pstNode->ulStreamID, "", lRecvLen, DOS_TRUE);
                    }
                }
                else if (lRecvLen < 0)
                {
                    alert("fail to recv");
                    exit(DOS_FAIL);
                }
                else
                {
                    pstNode = pt_stream_queue_serach_by_sockfd(g_stCCCBRecv, i);
                    if(NULL == pstNode)
                    {
                        alert("not found");
                    }
                    else
                    {
                        alert("response:%s", cRecvBuf);
                        ptc_ipcc_send_msg(PT_DATA_WEB, pstNode->ulStreamID, cRecvBuf, lRecvLen, DOS_FALSE);
                    }
                }
            }
        }
    }
}


/*void *ptc_get_from_stdin(void *arg)
{
    S8 acInputStr[PTC_DATA_BUFF_512];
    S32 lOperateType = 0;

    while (1)
    {
        acInputStr[0] = '\0';

        if (NULL == fgets(acInputStr, PTC_DATA_BUFF_512, stdin))
        {
            perror("fgets");
            continue;
        }

        acInputStr[strlen(acInputStr)-1] = '\0';
        lOperateType = atoi(acInputStr);
        if (lOperateType < 0 || lOperateType > 4)
        {
            alert("输入操作无效");
            continue;
        }

        ptc_ipcc_send_msg(PT_DATA_CTRL, lOperateType, acInputStr, strlen(acInputStr), DOS_FALSE);
    }
}
*/

S32 ptc_main()
{
    S32 lSocket, lRet;
    U8 achID[IPCC_ID_LEN] = {0};
    pthread_t tid1, tid2, tid3, tid4/*, tid5*/;

    lSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (lSocket < 0)
    {
        alert("create socket error!");
        return DOS_FAIL;
    }

    memcpy(achID, "123456789abcdefg", IPCC_ID_LEN);
   
    ptc_init_serv_msg();
    ptc_init_send_cache_queue(lSocket, achID);
    ptc_init_recv_cache_queue(lSocket, achID);

    lRet = pthread_create(&tid1, NULL, ptc_send_msg2amc, (void *)&lSocket);
    if (lRet < 0)
    {
        alert("Create pthread error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid2, NULL, ptc_recv_msg_from_amc, (void *)&lSocket);
    if (lRet < 0)
    {
        alert("Create pthread error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid3, NULL, ptc_recv_msg_from_proxy, NULL);
    if (lRet < 0)
    {
        alert("Create pthread error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid4, NULL, ptc_send_msg2proxy, NULL);
    if (lRet < 0)
    {
        alert("Create pthread error!");
        return DOS_FAIL;
    }

   /* lRet = pthread_create(&tid5, NULL, ptc_get_from_stdin, NULL);
    if (lRet < 0)
    {
        alert("Create pthread error!");
        return DOS_FAIL;
    }*/

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    pthread_join(tid4, NULL);
  /*  pthread_join(tid5, NULL);*/

    return DOS_SUCC;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */