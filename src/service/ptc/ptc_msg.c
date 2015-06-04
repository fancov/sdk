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
#include <sys/time.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
/* include provate header file */
#include <pt/dos_sqlite3.h>
#include <pt/ptc.h>
#include "ptc_msg.h"
#include "ptc_web.h"
#include "ptc_telnet.h"

sem_t g_SemPtcRecv;
U32  g_ulHDTimeoutCount = 0;                /* 心跳超时次数 */
BOOL g_bIsOnLine = DOS_FALSE;               /* 设备是否在线 */
DOS_TMR_ST g_stHBTmrHandle    = NULL;       /* 心跳定时器 */
DOS_TMR_ST g_stACKTmrHandle   = NULL;       /* 登陆、心跳接收响应的定时器 */
PT_CC_CB_ST *g_pstPtcSend     = NULL;       /* 发送缓存中ptc节点 */
PT_CC_CB_ST *g_pstPtcRecv     = NULL;       /* 接收缓存中ptc节点 */
list_t   g_stPtcNendRecvNode;       /* 需要接收的包的消息队列 */
list_t   g_stPtcNendSendNode;       /* 需要发送的包的消息队列 */
PTC_SERV_MSG_ST g_stServMsg;                /* 存放ptc信息的全局变量 */
pthread_mutex_t g_mutexPtcSendPthread   = PTHREAD_MUTEX_INITIALIZER;       /* 发送线程锁 */
pthread_cond_t  g_condPtcSend           = PTHREAD_COND_INITIALIZER;        /* 发送条件变量 */
pthread_mutex_t g_mutexPtcRecvPthread   = PTHREAD_MUTEX_INITIALIZER;       /* 接收线程锁 */
pthread_cond_t  g_condPtcRecv           = PTHREAD_COND_INITIALIZER;        /* 接收条件变量 */

S32 g_ulUdpSocket       = 0;
U32 g_ulSendTimeSleep   = 20;
U32 g_ulConnectPtsCount = 0;
S32 g_lHBTimeInterval   = 0;         /* ns */
PT_PTC_TYPE_EN g_enPtcType = PT_PTC_TYPE_LINUX;
struct timeval g_stHBStartTime;
S8 *g_pPackageBuff = NULL;
U32 g_ulPackageLen = 0;
U32 g_ulReceivedLen = 0;
U32 g_usCrc = 0;
PTC_CLIENT_CB_ST g_astPtcConnects[PTC_STREAMID_MAX_COUNT];

S8 *ptc_get_current_time()
{
    time_t ulCurrTime = 0;

    ulCurrTime = time(NULL);
    return ctime(&ulCurrTime);
}

void ptc_init_ptc_client_cb()
{
    S32 i = 0;

    for(i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        g_astPtcConnects[i].ulStreamID = 0;
        g_astPtcConnects[i].lSocket = -1;
        g_astPtcConnects[i].bIsValid = DOS_FALSE;
        g_astPtcConnects[i].enDataType = PT_DATA_BUTT;
        dos_list_init(&g_astPtcConnects[i].stRecvBuffList);
        g_astPtcConnects[i].ulBuffNodeCount = 0;
    }
}

/**
 * 函数：S32 ptc_get_socket_by_streamID(U32 ulStreamID)
 * 功能：
 *      1.通过streamID获得对应的socket
 * 参数
 *      U64 ulStreamID ：
 * 返回值：无
 */
S32 ptc_get_socket_by_streamID(U32 ulStreamID)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].bIsValid && g_astPtcConnects[i].ulStreamID == ulStreamID)
        {
            return g_astPtcConnects[i].lSocket;
        }
    }

    return DOS_FAIL;
}

/**
 * 函数：S32 ptc_get_streamID_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType)
 * 功能：
 *      1.通过lSocket 和 enDataType 获得对应的streamID
 * 参数
 * 返回值：无
 */
S32 ptc_get_streamID_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enDataType == enDataType && g_astPtcConnects[i].bIsValid && g_astPtcConnects[i].lSocket == lSocket)
        {
            return g_astPtcConnects[i].ulStreamID;
        }
    }

    return DOS_FAIL;
}

S32 ptc_get_client_node_array_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enDataType == enDataType && g_astPtcConnects[i].bIsValid && g_astPtcConnects[i].lSocket == lSocket)
        {
            return i;
        }
    }

    return DOS_FAIL;
}

/**
 * 函数：S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType)
 * 功能：
 *      1.
 * 参数
 * 返回值：无
 */
S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (!g_astPtcConnects[i].bIsValid)
        {
            g_astPtcConnects[i].bIsValid = DOS_TRUE;
            g_astPtcConnects[i].ulStreamID = ulStreamID;
            g_astPtcConnects[i].lSocket = lSocket;
            g_astPtcConnects[i].enDataType = enDataType;

            switch (enDataType)
            {
                case PT_DATA_WEB:
                    FD_SET(lSocket, &g_stWebFdSet);
                    g_ulWebSocketMax = g_ulWebSocketMax > lSocket ? g_ulWebSocketMax : lSocket;
                    break;
                case PT_DATA_CMD:
                    FD_SET(lSocket, &g_stCmdFdSet);
                    g_ulCmdSocketMax = g_ulCmdSocketMax > lSocket ? g_ulCmdSocketMax : lSocket;
                    break;
                default:
                    break;
            }

            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}

/**
 * 函数：S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType)
 * 功能：
 *      1.
 * 参数
 * 返回值：无
 */
VOID ptc_delete_client_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType)
{
    S32 i = 0;
    list_t *pstRecvBuffList = NULL;
    PTC_WEB_REV_MSG_HANDLE_ST *pstRecvBuffNode = NULL;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enDataType == enDataType && g_astPtcConnects[i].bIsValid && g_astPtcConnects[i].lSocket == lSocket)
        {
            g_astPtcConnects[i].bIsValid = DOS_FALSE;
            g_astPtcConnects[i].lSocket = -1;
            g_astPtcConnects[i].ulStreamID = U32_BUTT;

            if (g_astPtcConnects[i].ulBuffNodeCount > 0)
            {
                g_astPtcConnects[i].ulBuffNodeCount = 0;

                while (1)
                {
                    pstRecvBuffList = dos_list_fetch(&g_astPtcConnects[i].stRecvBuffList);
                    if (DOS_ADDR_INVALID(pstRecvBuffList))
                    {
                        break;
                    }

                    pstRecvBuffNode = dos_list_entry(pstRecvBuffList, PTC_WEB_REV_MSG_HANDLE_ST, stList);
                    if (DOS_ADDR_INVALID(pstRecvBuffList))
                    {
                        continue;
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
                }
            }

            return;
        }
    }
}

/**
 * 函数：S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType)
 * 功能：
 *      1.
 * 参数
 * 返回值：无
 */
VOID ptc_delete_client_by_streamID(U32 ulStreamID)
{
    S32 i = 0;
    list_t *pstRecvBuffList = NULL;
    PTC_WEB_REV_MSG_HANDLE_ST *pstRecvBuffNode = NULL;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].bIsValid && g_astPtcConnects[i].ulStreamID == ulStreamID)
        {
            g_astPtcConnects[i].bIsValid = DOS_FALSE;
            if (g_astPtcConnects[i].lSocket > 0)
            {
                switch (g_astPtcConnects[i].enDataType)
                {
                    case PT_DATA_WEB:
                        FD_CLR(g_astPtcConnects[i].lSocket, &g_stWebFdSet);
                        break;
                    case PT_DATA_CMD:
                        FD_CLR(g_astPtcConnects[i].lSocket, &g_stCmdFdSet);
                        break;
                    default:
                        break;
                }
                close(g_astPtcConnects[i].lSocket);
            }
            g_astPtcConnects[i].lSocket = -1;
            g_astPtcConnects[i].ulStreamID = U32_BUTT;
            g_astPtcConnects[i].enDataType = PT_DATA_BUTT;

            if (g_astPtcConnects[i].ulBuffNodeCount > 0)
            {
                g_astPtcConnects[i].ulBuffNodeCount = 0;
                while (1)
                {
                    pstRecvBuffList = dos_list_fetch(&g_astPtcConnects[i].stRecvBuffList);
                    if (DOS_ADDR_INVALID(pstRecvBuffList))
                    {
                        break;
                    }

                    pstRecvBuffNode = dos_list_entry(pstRecvBuffList, PTC_WEB_REV_MSG_HANDLE_ST, stList);
                    if (DOS_ADDR_INVALID(pstRecvBuffList))
                    {
                        continue;
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
                }
            }

            return;
        }
    }
}

/**
 * 函数：S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType)
 * 功能：
 *      1.
 * 参数
 * 返回值：无
 */
void ptc_set_cache_full_true(S32 lSocket, PT_DATA_TYPE_EN enDataType)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enDataType == enDataType && g_astPtcConnects[i].bIsValid && g_astPtcConnects[i].lSocket == lSocket)
        {
            if (PT_DATA_WEB == enDataType)
            {
                FD_CLR(lSocket, &g_stWebFdSet);
            }
        }
    }
}

/**
 * 函数：S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType)
 * 功能：
 *      1.
 * 参数
 * 返回值：无
 */
void ptc_set_cache_full_false(U32 ulStreamID, PT_DATA_TYPE_EN enDataType)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enDataType == enDataType && g_astPtcConnects[i].bIsValid && g_astPtcConnects[i].ulStreamID == ulStreamID)
        {
            if (PT_DATA_WEB == enDataType)
            {
                FD_SET(g_astPtcConnects[i].lSocket, &g_stWebFdSet);
            }
        }
    }
}

S32 ptc_printf_recv_cache_msg(U32 ulIndex, S32 argc, S8 **argv)
{
    S32             i           = 0;
    U32             ulLen       = 0;
    S8              szBuff[PT_DATA_BUFF_512]     = {0};
    PT_STREAM_CB_ST *pstStreamHead  = NULL;
    PT_STREAM_CB_ST *pstStreamCB    = NULL;

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%-20s%-15s%-10s%-10s%-10s\r\n", "SN", "StreamID", "CurrSeq", "MaxSeq", "ConfirmSeq");
    cli_out_string(ulIndex, szBuff);

    pstStreamHead = g_pstPtcRecv->pstStreamHead;
    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        pstStreamCB = &pstStreamHead[i];

        snprintf(szBuff, sizeof(szBuff), "%.16s%11d%10d%10d%10d\r\n", g_pstPtcRecv->aucID, pstStreamCB->ulStreamID, pstStreamCB->lCurrSeq, pstStreamCB->lMaxSeq, pstStreamCB->lConfirmSeq);
        cli_out_string(ulIndex, szBuff);
    }

    return 0;
}

S32 ptc_printf_send_cache_msg(U32 ulIndex, S32 argc, S8 **argv)
{
    S32             i           = 0;
    U32             ulLen       = 0;
    S8              szBuff[PT_DATA_BUFF_512]     = {0};
    PT_STREAM_CB_ST *pstStreamHead  = NULL;
    PT_STREAM_CB_ST *pstStreamCB    = NULL;

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%-20s%-15s%-10s%-10s%-10s\r\n", "SN", "StreamID", "CurrSeq", "MaxSeq", "ConfirmSeq");
    cli_out_string(ulIndex, szBuff);

    pstStreamHead = g_pstPtcSend->pstStreamHead;
    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        pstStreamCB = &pstStreamHead[i];

        snprintf(szBuff, sizeof(szBuff), "%.16s%11d%10d%10d%10d\r\n", g_pstPtcSend->aucID, pstStreamCB->ulStreamID, pstStreamCB->lCurrSeq, pstStreamCB->lMaxSeq, pstStreamCB->lConfirmSeq);
        cli_out_string(ulIndex, szBuff);
    }

    return 0;
}


S32 ptc_create_udp_socket(U32 ulSocketCache)
{
    S32 lSocket = 0;
    S32 lRet = 0;

    while (1)
    {
        lSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (lSocket < 0)
        {
            perror("create socket error!");
            sleep(1);

            continue;
        }
        lRet = setsockopt(lSocket, SOL_SOCKET, SO_SNDBUF, (char*)&ulSocketCache, sizeof(ulSocketCache));
        if (lRet != 0)
        {
            logr_error("setsockopt error : %d", lRet);
            close(lSocket);
            sleep(1);

            continue;
        }

        lRet = setsockopt(lSocket, SOL_SOCKET, SO_RCVBUF, (char*)&ulSocketCache, sizeof(ulSocketCache));
        if (lRet != 0)
        {
            logr_error("setsockopt error : %d", lRet);
            close(lSocket);
            sleep(1);

            continue;
        }

        break;
    }

    return lSocket;
}

/**
 * 函数：VOID ptc_delete_send_stream_node(U32 ulStreamID, PT_DATA_TYPE_EN enDataType)
 * 功能：
 *      1.删除发送缓存中,一个stream节点
 * 参数
 *      U32 ulStreamID ：stream id
 *      U32 enDataType ：消息类型
 * 返回值：无
 */
VOID ptc_delete_send_stream_node_by_streamID(U32 ulStreamID)
{
    PT_STREAM_CB_ST *pstRecvStreamHead = g_pstPtcRecv->pstStreamHead;
    PT_STREAM_CB_ST *pstSendStreamHead = g_pstPtcSend->pstStreamHead;
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (pstRecvStreamHead[i].bIsValid && pstRecvStreamHead[i].ulStreamID == ulStreamID)
        {
            pt_delete_stream_node(&pstRecvStreamHead[i], 0);
            pt_delete_stream_node(&pstSendStreamHead[i], 1);
        }
    }
}

void ptc_delete_stream_node_by_recv(PT_STREAM_CB_ST *pstRecvStreamNode)
{
    PT_STREAM_CB_ST *pstRecvStreamHead = g_pstPtcRecv->pstStreamHead;
    PT_STREAM_CB_ST *pstSendStreamHead = g_pstPtcSend->pstStreamHead;
    S32 i = 0;

    if (DOS_ADDR_INVALID(pstRecvStreamNode))
    {
        return;
    }

    pt_delete_stream_node(pstRecvStreamNode, 0);

    i = (pstRecvStreamHead - pstRecvStreamNode)/sizeof(PT_STREAM_CB_ST);
    if (i < PTC_STREAMID_MAX_COUNT)
    {
        pt_delete_stream_node(&pstSendStreamHead[i], 1);
    }
}

void ptc_init_all_stream()
{
    S32 i = 0;
    PT_STREAM_CB_ST *pstStreamRecvHead = g_pstPtcRecv->pstStreamHead;
    PT_STREAM_CB_ST *pstStreamSendHead = g_pstPtcSend->pstStreamHead;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        pt_delete_stream_node(&pstStreamRecvHead[i], 0);
        pt_delete_stream_node(&pstStreamSendHead[i], 1);
    }

}

S32 ptc_get_udp_user_ip()
{
    S32 lSockfd = 0;
    S32 lError  = 0;
    struct sockaddr_in stServAddr;
    struct sockaddr_in stAddrCli;
    socklen_t lAddrLen = sizeof(struct sockaddr_in);
    S8 szLocalIp[PT_IP_ADDR_SIZE] = {0}; /* 点分十进制 */
    S8 szIfrName[PT_DATA_BUFF_64] = {0};
    S8 szMac[PT_DATA_BUFF_64] = {0};

    stServAddr = g_pstPtcSend->stDestAddr;

    lSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lSockfd < 0)
    {
        perror("create tcp socket");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

ptc_connect:
    lError = connect(lSockfd,(struct sockaddr*)&stServAddr, sizeof(stServAddr));
    if (lError < 0)
    {
        perror("connect pts");
        sleep(2);
        /* 重连 */
        goto ptc_connect;
    }

    lError = getsockname(lSockfd,(struct sockaddr*)&stAddrCli, &lAddrLen);
    if (lError != 0)
    {
        pt_logr_info("Get sockname Error!");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    g_stServMsg.usLocalPort = dos_ntohs(stAddrCli.sin_port);
    dos_strcpy(szLocalIp, inet_ntoa(stAddrCli.sin_addr));
    inet_pton(AF_INET, szLocalIp, (VOID *)(g_stServMsg.achLocalIP));
    lError = ptc_get_ifr_name_by_ip(szLocalIp, lSockfd, szIfrName, PT_DATA_BUFF_64);
    if (lError != DOS_SUCC)
    {
        pt_logr_info("Get ifr name Error!");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    lError = ptc_get_mac(szIfrName, lSockfd, szMac, PT_DATA_BUFF_64);
    if (lError != DOS_SUCC)
    {
        pt_logr_info("Get mac Error!");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    dos_strncpy(g_stServMsg.szMac, szMac, PT_DATA_BUFF_64);
    logr_debug("local ip : %s, irf name : %s, mac : %s", szLocalIp, szIfrName, szMac);

    close(lSockfd);

    return DOS_SUCC;
}

/**
 * 函数：VOID ptc_data_lose(PT_MSG_TAG *pstMsgDes, S32 lShouldSeq)
 * 功能：
 *      1.发送丢包请求
 * 参数
 *      PT_MSG_TAG *pstMsgDes ：包含丢失包的描述信息
 * 返回值：无
 */
VOID ptc_data_lose(PT_MSG_TAG *pstMsgDes)
{
    if (NULL == pstMsgDes)
    {
        pt_logr_debug("ptc data lose param is NULL");
        return;
    }

    pthread_mutex_lock(&g_mutexPtcSendPthread);

    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_RESEND;
    pt_need_send_node_list_insert(&g_stPtcNendSendNode, g_pstPtcSend->aucID, pstMsgDes, enCmdValue, bIsResend);

    pthread_cond_signal(&g_condPtcSend);
    pthread_mutex_unlock(&g_mutexPtcSendPthread);

    return;
}

VOID ptc_send_exit_notify_to_pts(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S32 lSeq)
{
    PT_MSG_TAG stMsgDes;
    S8 acBuff[PT_DATA_BUFF_512] = {0};
    S32 lResult = 0;

    stMsgDes.enDataType = enDataType;
    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(ulStreamID);
    stMsgDes.ExitNotifyFlag = DOS_TRUE;
    stMsgDes.lSeq = dos_htonl(lSeq);
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
    if (g_ulUdpSocket > 0)
    {
        lResult = sendto(g_ulUdpSocket, acBuff, sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
        if (lResult < 0)
        {
            close(g_ulUdpSocket);
            g_ulUdpSocket = -1;
            g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);
        }
    }
}

/**
 * 函数：VOID ptc_send_lost_data_req(U64 ulLoseMsg)
 * 功能：
 *      1.重传丢失的包
 * 参数
 *      U64 ulLoseMsg ：
 * 返回值：无
 */
VOID ptc_send_lost_data_req(U64 ulLoseMsg)
{
    if (0 == ulLoseMsg)
    {
        return;
    }

    S32 i = 0;
    U32 ulCount = 0;
    U32 ulArraySub = 0;
    PT_LOSE_BAG_MSG_ST *pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)ulLoseMsg;
    PT_MSG_TAG *pstMsg = &pstLoseMsg->stMsg;
    PT_STREAM_CB_ST *pstStreamNode = pstLoseMsg->pstStreamNode;

    if (pstStreamNode->ulCountResend >= PT_RESEND_REQ_MAX_COUNT)
    {
        /* 丢包重发PTC_RESEND_MAX_COUNT次，仍未收到 */
        ptc_send_exit_notify_to_pts(pstMsg->enDataType, pstMsg->ulStreamID, 0);    /* 通知pts关闭sockfd */
        /* 清空ptc中，stream的资源 */
        ptc_delete_send_stream_node_by_streamID(pstMsg->ulStreamID);
        dos_tmr_stop(&pstStreamNode->hTmrHandle);
        return;
    }
    else
    {
        pstStreamNode->ulCountResend++;
        for (i=pstStreamNode->lCurrSeq+1; i<pstStreamNode->lMaxSeq; i++)
        {
            ulArraySub = i & (PT_DATA_SEND_CACHE_SIZE - 1);
            if (0 == i)
            {
                /* 判断0号包是否丢失 */
                if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].ulLen == 0)
                {
                    ulCount++;
                    pstMsg->lSeq = i;
                    ptc_data_lose(pstMsg);
                }
            }
            else if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].lSeq != i)
            {
                ulCount++;
                pstMsg->lSeq = i;
                ptc_data_lose(pstMsg);  /*发送丢包重发请求*/
            }
        }
    }

    if (0 == ulCount)
    {
        if (NULL != pstStreamNode->hTmrHandle)
        {
            dos_tmr_stop(&pstStreamNode->hTmrHandle);
            pstStreamNode->hTmrHandle = NULL;
        }
        pstStreamNode->ulCountResend = 0;
    }
}

/**
 * 函数：S32 ptc_key_convert(S8 *szKey, S8 *szDest, S8 *szVersion)
 * 功能：
 *      1.登陆验证
 * 参数
 *      S8 *szKey     ：原始值
 *      S8 *szDest    ：转换后的值
 *      S8 *szVersion ：ptc版本号
 * 返回值：无
 */
S32 ptc_key_convert(S8 *szKey, S8 *szDest)
{
    if (NULL == szKey || NULL == szDest)
    {

        return DOS_FAIL;
    }

    S32 i = 0;

    //if (dos_strncmp(ptc_get_version(), "1.1", dos_strlen("1.1")) == 0)
    //{
    /* 1.1版本验证方法 */
    for (i=0; i<PT_LOGIN_VERIFY_SIZE-1; i++)
    {
        szDest[i] = szKey[i]&0xA9;
    }
    szDest[PT_LOGIN_VERIFY_SIZE] = '\0';
    //}

    return DOS_SUCC;
}

/**
 * 函数：VOID ptc_hd_ack_timeout_callback(U64 lSockfd)
 * 功能：
 *      1.心跳响应的超时函数
 * 参数
 *      U64 lSockfd ：udp通信套接字
 * 返回值：无
 */
VOID ptc_hd_ack_timeout_callback()
{
    g_ulHDTimeoutCount++;
    pt_logr_debug("Heartbeat response timeout : %d", g_ulHDTimeoutCount);
    g_lHBTimeInterval = -1;
    if (g_ulHDTimeoutCount >= PTC_HB_TIMEOUT_COUNT_MAX)
    {
        /* 连续多次无法收到心跳响应，重新发送登陆请求 */
        logr_debug("can not connect PTS!!!");
        dos_tmr_stop(&g_stACKTmrHandle);
        g_stACKTmrHandle = NULL;
        ptc_init_all_stream();

        g_bIsOnLine = DOS_FALSE;

        ptc_send_login2pts();

    }
}

/**
 * 函数：VOID ptc_send_hb_req(U64 lSockfd)
 * 功能：
 *      1.发送心跳请求
 * 参数
 *      U64 lSockfd ：udp通信套接字
 * 返回值：无
 */
VOID ptc_send_hb_req()
{
    PT_MSG_TAG stMsgDes;
    HEART_BEAT_RTT_TSG stVerRet;
    S8 acBuff[PT_DATA_BUFF_512] = {0};
    S32 lResult = 0;
    /* 心跳消息描述赋值 */
    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_HB_REQ);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;
    dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
    /* 心跳内容赋值 */
    stMsgDes.usServPort = g_stServMsg.usLocalPort;
    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));

    stVerRet.lHBTimeInterval = g_lHBTimeInterval;
    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

    gettimeofday(&g_stHBStartTime, NULL);

    if (g_ulUdpSocket > 0)
    {
        lResult = sendto(g_ulUdpSocket, acBuff, sizeof(HEART_BEAT_RTT_TSG) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
        if (lResult < 0)
        {
            close(g_ulUdpSocket);
            g_ulUdpSocket = -1;
            g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);
        }
    }

    /* 启动定时器，判断心跳响应是否超时 */
    pt_logr_debug("send hd to pts");
    lResult = dos_tmr_start(&g_stACKTmrHandle, PTC_WAIT_HB_ACK_TIME, ptc_hd_ack_timeout_callback, 0, TIMER_NORMAL_NO_LOOP);
    if (lResult < 0)
    {
        pt_logr_info("ptc_send_hb_req : start timer fail");
    }
}

/**
 * 函数：VOID ptc_send_login_req(U64 arg)
 * 功能：
 *      1.发送登陆请求
 * 参数
 *      U64 arg ：udp通信套接字
 * 返回值：无
 */
VOID ptc_send_login_req(U64 arg)
{
    S32 lResult = 0;
    PT_MSG_TAG stMsgDes;
    PT_CTRL_DATA_ST stVerRet;
    S8 acBuff[PT_DATA_BUFF_512] = {0};
    struct sockaddr_in stAddrCli;
    socklen_t lAddrLen = sizeof(struct sockaddr_in);
    S8 szDestIp[PT_IP_ADDR_SIZE] = {0};

    if (g_bIsOnLine)
    {
        /* 已登陆,关闭定时器。发送心跳 */
        dos_tmr_stop(&g_stHBTmrHandle);
        ptc_send_heartbeat2pts();

        return;
    }
    g_ulConnectPtsCount++;
    if (g_ulConnectPtsCount > 4)
    {
        g_ulConnectPtsCount = 0;
        if (g_pstPtcSend->stDestAddr.sin_addr.s_addr == *(U32 *)(g_stServMsg.achPtsMajorIP))
        {
            /* 主域名注册不上，切换pts到副域名 */
            if (g_stServMsg.usPtsMinorPort != 0)
            {
                g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMinorIP);
                g_pstPtcSend->stDestAddr.sin_port = g_stServMsg.usPtsMinorPort;
            }
        }
        else if (g_pstPtcSend->stDestAddr.sin_addr.s_addr == *(U32 *)(g_stServMsg.achPtsMinorIP))
        {
            /* 副域名注册不上，切换到主域名 */
            if (g_stServMsg.usPtsMajorPort != 0)
            {
                g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMajorIP);
                g_pstPtcSend->stDestAddr.sin_port = g_stServMsg.usPtsMajorPort;
            }
        }
        else
        {
            /* 其它域名注册不上，切换到主域名 */
            if (g_stServMsg.usPtsMajorPort != 0)
            {
                g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMajorIP);
                g_pstPtcSend->stDestAddr.sin_port = g_stServMsg.usPtsMajorPort;
            }
        }
    }
    /* 登陆消息描述赋值 */
    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_LOGIN_REQ);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;
    //dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
    //stMsgDes.usServPort = g_stServMsg.usLocalPort;
    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
    /* 登陆内容赋值 */
    stVerRet.enCtrlType = PT_CTRL_LOGIN_REQ;

    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

    if (dos_ntohs(g_pstPtcSend->stDestAddr.sin_port) != 0)
    {
        if (g_ulUdpSocket > 0)
        {
            lResult = sendto(g_ulUdpSocket, acBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
            if (lResult  < 0)
            {
                close(g_ulUdpSocket);
                g_ulUdpSocket = -1;
                g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);
            }
        }
        inet_ntop(AF_INET, (void *)(&g_pstPtcSend->stDestAddr.sin_addr.s_addr), szDestIp, PT_IP_ADDR_SIZE);
        logr_debug("ptc send login request to pts : ip : %s:%u", szDestIp, dos_ntohs(g_pstPtcSend->stDestAddr.sin_port));
    }

    /* 获取ptc一侧udp的私网端口号 -- sendto后，才能获取端口号 */
    if (g_stServMsg.usLocalPort == 0)
    {
        lResult = getsockname(g_ulUdpSocket,(struct sockaddr*)&stAddrCli, &lAddrLen);
        if (lResult != 0)
        {
            pt_logr_info("Get sockname Error!");
            return;
        }
        g_stServMsg.usLocalPort = dos_ntohs(stAddrCli.sin_port);
    }
}

/**
 * 函数：VOID ptc_send_logout_req(U64 arg)
 * 功能：
 *      1.退出登陆请求
 * 参数
 *      U64 arg ：udp通信套接字
 * 返回值：无
 */
VOID ptc_send_logout_req()
{
    PT_MSG_TAG stMsgDes;
    PT_CTRL_DATA_ST stVerRet;
    S8 acBuff[PT_DATA_BUFF_512] = {0};
    S32 lResult = 0;

    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_LOGOUT);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;
    dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
    stMsgDes.usServPort = g_stServMsg.usLocalPort;

    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));

    stVerRet.enCtrlType = PT_CTRL_LOGOUT;
    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));
    if (g_ulUdpSocket > 0)
    {
        lResult = sendto(g_ulUdpSocket, acBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
        if (lResult  < 0)
        {
            close(g_ulUdpSocket);
            g_ulUdpSocket = -1;
            g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);
        }
    }
    pt_logr_debug("ptc send logout request to pts");
}

/**
 * 函数：VOID ptc_send_confirm_msg(PT_MSG_TAG *pstMsgDes, U32 lConfirmSeq)
 * 功能：
 *      1.发送确认接收消息
 * 参数
 *
 * 返回值：无
 */
VOID ptc_send_confirm_msg(PT_MSG_TAG *pstMsgDes, U32 lConfirmSeq)
{
    if (NULL == pstMsgDes)
    {
        pt_logr_debug("ptc_send_confirm_msg : param error");
        return;
    }

    S32 i = 0;

    pthread_mutex_lock(&g_mutexPtcSendPthread);

    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_CONFIRM;
    pstMsgDes->lSeq = lConfirmSeq;

    pt_logr_debug("send confirm data to pts: type = %d, stream = %d", pstMsgDes->enDataType,pstMsgDes->ulStreamID);
    for (i=0; i<PT_SEND_CONFIRM_COUNT; i++)
    {
        pt_need_send_node_list_insert(&g_stPtcNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);
    }
    pthread_cond_signal(&g_condPtcSend);
    pthread_mutex_unlock(&g_mutexPtcSendPthread);

}

/**
 * 函数：S32 ptc_save_into_send_cache(PT_CC_CB_ST *pstPtcNode, PT_MSG_TAG *pstMsgDes, S8 *acSendBuf, S32 lDataLen)
 *      1.添加到发送缓存
 * 参数
 *      PT_CC_CB_ST *pstPtcNode : ptc缓存地址
 *      PT_MSG_TAG *pstMsgDes   ：头部信息
 *      S8 *acSendBuf           ：包内容
 *      S32 lDataLen            ：包内容长度
 * 返回值：
 *      PT_SAVE_DATA_SUCC 添加成功
 *      PT_SAVE_DATA_FAIL 失败
 *      PT_NEED_CUT_PTHREAD 满64个包，等待确认接收信息
 */
S32 ptc_save_into_send_cache(PT_MSG_TAG *pstMsgDes, S8 *acSendBuf, S32 lDataLen)
{
    if (DOS_ADDR_INVALID(g_pstPtcSend) || DOS_ADDR_INVALID(pstMsgDes) || DOS_ADDR_INVALID(acSendBuf))
    {
        return PT_SAVE_DATA_FAIL;
    }

    PT_STREAM_CB_ST *pstStreamListHead      = NULL;
    PT_STREAM_CB_ST *pstRecvStreamListHead  = NULL;
    PT_STREAM_CB_ST *pstStreamNode          = NULL;
    S32             lResult                 = 0;
    S32             i                       = 0;

    pstStreamListHead     = g_pstPtcSend->pstStreamHead;
    pstRecvStreamListHead = g_pstPtcRecv->pstStreamHead;
    pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
    if (DOS_ADDR_INVALID(pstStreamNode))
    {
        if (pstMsgDes->enDataType == PT_DATA_CTRL)
        {
            /* 不存在 */
            i =  pt_stream_queue_get_node(pstRecvStreamListHead);
            if (i < 0 || i >= PTC_STREAMID_MAX_COUNT)
            {
                return PT_SAVE_DATA_FAIL;
            }
            pstStreamNode = &pstStreamListHead[i];
            /* 初始化对应的发送缓存 */
            pt_stream_node_init(pstStreamNode);
            pstStreamNode->bIsValid = DOS_TRUE;
            pstStreamNode->ulStreamID = pstMsgDes->ulStreamID;
            pstStreamNode->enDataType = (PT_DATA_TYPE_EN)pstMsgDes->enDataType;

            dos_memcpy(pstRecvStreamListHead->aulServIp, pstMsgDes->aulServIp, IPV6_SIZE);
            pstRecvStreamListHead->usServPort = pstMsgDes->usServPort;
            pstRecvStreamListHead->ulStreamID = pstMsgDes->ulStreamID;
            pstRecvStreamListHead->enDataType = (PT_DATA_TYPE_EN)pstMsgDes->enDataType;

        }
        else
        {
            return PT_SAVE_DATA_FAIL;
        }
    }

    /* 将数据插入到data queue中 */
    lResult = pt_send_data_tcp_queue_insert(pstStreamNode, acSendBuf, lDataLen, PT_DATA_SEND_CACHE_SIZE, 0);
    if (lResult < 0)
    {
        return PT_SAVE_DATA_FAIL;
    }

    return lResult;
}

void ptc_send_login_rsp_to_pts(PT_CTRL_DATA_ST *pstVerRet, PT_MSG_TAG *pstMsgDes)
{
    PT_MSG_TAG stMsgDes;
    S8 acBuff[PT_DATA_BUFF_512] = {0};
    S32 lResult = 0;

    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_LOGIN_RSP);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
    stMsgDes.usServPort = g_stServMsg.usLocalPort;

    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), pstVerRet, sizeof(PT_CTRL_DATA_ST));

    if (g_ulUdpSocket >= 0)
    {
        lResult = sendto(g_ulUdpSocket, acBuff,  sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
        if (lResult  < 0)
        {
            close(g_ulUdpSocket);
            g_ulUdpSocket = -1;
            g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);
        }
    }
}

/**
 * 函数：S32 ptc_save_into_cache(PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen)
 *      1.添加到接收缓存
 * 参数
 *      PT_CC_CB_ST *pstPtcNode : ptc缓存地址
 *      PT_MSG_TAG *pstMsgDes   ：头部信息
 *      S8 *acRecvBuf           ：包内容
 *      S32 lDataLen            ：包内容长度
 *      PT_SAVE_DATA_SUCC 添加成功
 *      PT_SAVE_DATA_FAIL 失败
 *      PT_NEED_CUT_PTHREAD 缓存已存满
 */
S32 ptc_save_into_recv_cache(S8 *acRecvBuf, S32 lDataLen)
{
   if (DOS_ADDR_INVALID(acRecvBuf))
    {
        return PT_SAVE_DATA_FAIL;
    }

    S32 i = 0;
    U32 ulArraySub = 0;
    S32 lResult = 0;
    U32 ulNextSendArraySub = 0;
    PT_STREAM_CB_ST    *pstRecvStreamHead	= NULL;
    PT_STREAM_CB_ST    *pstSendStreamHead	= NULL;
    PT_STREAM_CB_ST    *pstStreamNode       = NULL;
    PT_LOSE_BAG_MSG_ST *pstLoseMsg          = NULL;
    PT_DATA_TCP_ST     *pstDataQueue        = NULL;
    PT_MSG_TAG         *pstMsgDes           = (PT_MSG_TAG *)acRecvBuf;

    /* 判断发送缓存中是否存在 streamID */
    pstRecvStreamHead = g_pstPtcRecv->pstStreamHead;
    pstSendStreamHead = g_pstPtcSend->pstStreamHead;
    pstStreamNode = pt_stream_queue_search(pstRecvStreamHead, pstMsgDes->ulStreamID);
    if (DOS_ADDR_INVALID(pstStreamNode))
    {
        /* 不存在 */
        i =  pt_stream_queue_get_node(pstRecvStreamHead);
        if (i < 0 || i >= PTC_STREAMID_MAX_COUNT)
        {
            return PT_SAVE_DATA_FAIL;
        }
        pstStreamNode = &pstRecvStreamHead[i];
        /* 初始化对应的发送缓存 */
        pt_stream_node_init(&pstSendStreamHead[i]);
        pstSendStreamHead[i].bIsValid = DOS_TRUE;
        pstSendStreamHead[i].ulStreamID = pstMsgDes->ulStreamID;
        pstSendStreamHead[i].enDataType = (PT_DATA_TYPE_EN)pstMsgDes->enDataType;

        dos_memcpy(pstStreamNode->aulServIp, pstMsgDes->aulServIp, IPV6_SIZE);
        pstStreamNode->usServPort = pstMsgDes->usServPort;
        pstStreamNode->ulStreamID = pstMsgDes->ulStreamID;
        pstStreamNode->enDataType = (PT_DATA_TYPE_EN)pstMsgDes->enDataType;
    }

    /* 将数据插入到data queue中 */
    lResult = pt_recv_data_tcp_queue_insert(pstStreamNode, pstMsgDes, acRecvBuf+sizeof(PT_MSG_TAG), lDataLen-sizeof(PT_MSG_TAG), PT_DATA_RECV_CACHE_SIZE);
    if (lResult < 0)
    {
        pt_logr_info("ptc_save_into_recv_cache : add data into recv cache fail");
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
            pt_logr_debug("send make sure msg : %d", pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE);
            ptc_send_confirm_msg(pstMsgDes, pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE);
            pstStreamNode->lConfirmSeq = pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE;
        }
    }

    if (PT_NEED_CUT_PTHREAD == lResult)
    {
        /* 缓存已存满 */
        return PT_NEED_CUT_PTHREAD;
    }
    else
    {
        /* 判断是否丢包，存储的是否是可以发送的包 */
        if (pstStreamNode->lCurrSeq + 1 == pstMsgDes->lSeq)
        {
            if (NULL != pstStreamNode->hTmrHandle)
            {
                /* 如果存在定时器，则这个包为丢失的最小包 */
                pstStreamNode->ulCountResend = 0;
            }
            return PT_SAVE_DATA_SUCC;
        }
        else if (pstMsgDes->lSeq < pstStreamNode->lCurrSeq + 1)
        {
            /* 已发送过的包 */
            pt_logr_info("stream id %d, seq is %d, currseq is %d", pstMsgDes->ulStreamID, pstMsgDes->lSeq, pstStreamNode->lCurrSeq);
            printf("stream id %d, seq is %d, currseq is %d\n", pstMsgDes->ulStreamID, pstMsgDes->lSeq, pstStreamNode->lCurrSeq);
            return PT_SAVE_DATA_FAIL;
        }
        else
        {
            ulNextSendArraySub = (pstStreamNode->lCurrSeq + 1) & (PT_DATA_RECV_CACHE_SIZE - 1);
            pstDataQueue = pstStreamNode->unDataQueHead.pstDataTcp;
            if (pstDataQueue[ulNextSendArraySub].lSeq == pstStreamNode->lCurrSeq + 1)
            {
                return PT_SAVE_DATA_SUCC;
            }

            if (pstMsgDes->lSeq == 0 && pstStreamNode->lCurrSeq == -1)
            {
                return PT_SAVE_DATA_SUCC;
            }

            /* 丢包，如果没有定时器的，创建定时器 */
            if (NULL == pstStreamNode->hTmrHandle)
            {
                pt_logr_debug("lose data, start timer, seq : %d", pstStreamNode->lCurrSeq + 1);
                if (pstStreamNode->pstLostParam == NULL)
                {
                    pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)dos_dmem_alloc(sizeof(PT_LOSE_BAG_MSG_ST));
                    if (NULL == pstLoseMsg)
                    {
                        pt_logr_error("ptc_save_into_recv_cache : malloc");

                        return PT_SAVE_DATA_FAIL;
                    }

                    pstLoseMsg->stMsg = *pstMsgDes;
                    pstLoseMsg->pstStreamNode = pstStreamNode;

                    pstStreamNode->pstLostParam = pstLoseMsg;
                }
                pstStreamNode->ulCountResend = 0;
                ptc_send_lost_data_req((U64)pstStreamNode->pstLostParam);
                lResult = dos_tmr_start(&pstStreamNode->hTmrHandle, PT_SEND_LOSE_DATA_TIMER, ptc_send_lost_data_req, (U64)pstStreamNode->pstLostParam, TIMER_NORMAL_LOOP);
                if (PT_SAVE_DATA_FAIL == lResult)
                {
                    pt_logr_info("ptc_save_into_recv_cache : start timer fail");

                    return PT_SAVE_DATA_FAIL;
                }

            }

            pt_logr_info("ptc_save_into_recv_cache : time exited");

            return PT_SAVE_DATA_FAIL;
        }
    }
}

/**
 * 函数：BOOL ptc_upgrade(PT_DATA_TCP_ST *pstRecvDataTcp, PT_STREAM_CB_ST *pstStreamNode)
 * 功能：
 *      1.接收升级包，升级ptc
 * 参数
 * 返回值： DOS_FALSE   : break;
 *          DOS_TRUE    : continue;
 */
BOOL ptc_save_upgrade_package(S8 *szBuff, U32 ulLen)
{
    LOAD_FILE_TAG *pstUpgrade = NULL;
    FILE *pFileFd = NULL;
    S32 lFd = 0;
    U32 usCrc = 0;

    if (DOS_ADDR_INVALID(g_pPackageBuff))
    {
        g_ulReceivedLen = 0;
        pstUpgrade = (LOAD_FILE_TAG *)szBuff;
        g_ulPackageLen = dos_ntohl(pstUpgrade->ulFileLength);
        g_usCrc = dos_ntohl(pstUpgrade->usCRC);
        if (g_ulPackageLen <= 0)
        {
            /* 失败 */
            logr_info("recv upgrade package fail");
        }
        else
        {
            logr_debug("start recv upgrade package");
            g_pPackageBuff = (S8 *)dos_dmem_alloc(g_ulPackageLen + sizeof(LOAD_FILE_TAG));
            if (NULL == g_pPackageBuff)
            {
                DOS_ASSERT(0);
                logr_debug("malloc fail\n");
                return DOS_FALSE;
            }
            else
            {
                dos_memzero(g_pPackageBuff, g_ulPackageLen + sizeof(LOAD_FILE_TAG));
            }
        }

        dos_memcpy(g_pPackageBuff+g_ulReceivedLen, szBuff, ulLen);
        g_ulReceivedLen += ulLen;

        return DOS_TRUE;
    }

    dos_memcpy(g_pPackageBuff+g_ulReceivedLen, szBuff, ulLen);
    g_ulReceivedLen += ulLen;

    if (ulLen < PT_RECV_DATA_SIZE)
    {
        /* 接收完成，CRC验证 */
        usCrc = load_calc_crc((U8 *)g_pPackageBuff + sizeof(LOAD_FILE_TAG), g_ulReceivedLen-sizeof(LOAD_FILE_TAG));
        printf("g_usCrc : %u, usCrc = %u, file len : %u, recv len : %u\n\n", g_usCrc, usCrc, g_ulPackageLen, g_ulReceivedLen);
        if (usCrc == g_usCrc)
        {
            logr_debug("recv upgrade package succ");
            /* 验证通过，保存到文件中 */
            system("mv ptcd ptcd_old");

            pFileFd = fopen("./ptcd", "w");
            if (NULL == pFileFd)
            {
                pt_logr_info("create file file\n");
                system("mv ptcd_old ptcd");
            }
            else
            {
                lFd = fileno(pFileFd);
                fchmod(lFd, 0777);
                fwrite(g_pPackageBuff+sizeof(LOAD_FILE_TAG), g_ulPackageLen-sizeof(LOAD_FILE_TAG), 1, pFileFd);
                fclose(pFileFd);
                pFileFd = NULL;
                ptc_send_exit_notify_to_pts(PT_DATA_CTRL, PT_CTRL_PTC_PACKAGE, 0);
                ptc_send_logout2pts();
                /* ptc 退出程序, 等待升级 */
                //exit(0);
                kill(getpid(), SIGUSR1);
            }
            dos_dmem_free(g_pPackageBuff);
            g_pPackageBuff = NULL;
        }
        else
        {
            /* 验证不通过 */
            logr_info("upgrade package error : md5 error");
        }

        ptc_delete_send_stream_node_by_streamID(PT_CTRL_PTC_PACKAGE);
        ptc_send_exit_notify_to_pts(PT_DATA_CTRL, PT_CTRL_PTC_PACKAGE, 0);

        return DOS_FALSE;
    }

    return DOS_TRUE;
}

VOID ptc_recv_upgrade_package(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
{
    U32               ulArraySub       = 0;
    PT_STREAM_CB_ST   *pstStreamList   = NULL;
    PT_STREAM_CB_ST   *pstStreamNode   = NULL;
    PT_DATA_TCP_ST    stRecvDataTcp;
    S32               lResult          = 0;

     pstStreamList = g_pstPtcRecv->pstStreamHead;
    if (DOS_ADDR_INVALID(pstStreamList))
    {
        return;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamList, pstNeedRecvNode->ulStreamID);
    if (NULL == pstStreamNode)
    {
        return;
    }

    if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
    {
        return;
    }
    while(1)
    {
        pstStreamNode->lCurrSeq++;
        ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1);
        stRecvDataTcp = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
        if (stRecvDataTcp.lSeq == pstStreamNode->lCurrSeq)
        {
            lResult = ptc_save_upgrade_package(stRecvDataTcp.szBuff, stRecvDataTcp.ulLen);
            if (!lResult)
            {
                break;
            }
            continue;
        }
        else
        {
            /* 这个seq没有发送，减一 */
            pstStreamNode->lCurrSeq--;
            break;
        }
    }

    return;
}

VOID ptc_update_pts_history_file(U8 aulServIp[IPV6_SIZE], U16 usServPort)
{
    S8 szPtsIp[PT_IP_ADDR_SIZE] = {0};
    FILE *pFileHandle = NULL;
    S32 lPtsHistoryConut = 0;
    S8 szPtsHistory[3][PT_DATA_BUFF_64];

    inet_ntop(AF_INET, (void *)(aulServIp), szPtsIp, PT_IP_ADDR_SIZE);
    pFileHandle = fopen(g_stServMsg.szHistoryPath, "r");
    if (NULL == pFileHandle)
    {
        return;
    }

    while (!feof(pFileHandle) && lPtsHistoryConut < 3)
    {
        fgets(szPtsHistory[lPtsHistoryConut], PT_DATA_BUFF_64, pFileHandle);
        printf("lPtsHistoryConut : %d, %s\n", lPtsHistoryConut, szPtsHistory[lPtsHistoryConut]);
        lPtsHistoryConut++;
    }
    fclose(pFileHandle);
    lPtsHistoryConut--;
    printf("switch pts count : %d\n", lPtsHistoryConut);
    pFileHandle = fopen(g_stServMsg.szHistoryPath, "w");
    if (NULL == pFileHandle)
    {
        return;
    }

    if (0 == lPtsHistoryConut)
    {
        fprintf(pFileHandle, "%s&%d\n", szPtsIp, dos_ntohs(usServPort));
    }
    else if (1 == lPtsHistoryConut)
    {
        fprintf(pFileHandle, "%s&%d\n%s", szPtsIp, dos_ntohs(usServPort), szPtsHistory[0]);
    }
    else
    {
        fprintf(pFileHandle, "%s&%d\n%s%s", szPtsIp, dos_ntohs(usServPort), szPtsHistory[0], szPtsHistory[1]);
    }
    fclose(pFileHandle);

    ptc_get_pts_history();
    return;
}

/**
 * 函数：void ptc_get_hb_time_interval(struct timeval stHBEndTime)
 * 功能：
 *      1.计算心跳和心跳响应之间的时间间隔
 * 参数
 *
 * 返回值：无
 */
void ptc_get_hb_time_interval(struct timeval stHBEndTime)
{
    S32 lSec = stHBEndTime.tv_sec - g_stHBStartTime.tv_sec;
    S32 lUsec = stHBEndTime.tv_usec - g_stHBStartTime.tv_usec;

    g_lHBTimeInterval = lSec * 1000 * 1000 + lUsec;
}

/**
 * 函数：VOID ptc_ctrl_msg_handle(U32 ulStreamID, S8 *pData)
 * 功能：
 *      1.处理控制消息
 * 参数
 *
 * 返回值：无
 */
VOID ptc_ctrl_msg_handle(S8 *pData, U32 lRecvLen)
{
    if (DOS_ADDR_INVALID(pData))
    {
        return;
    }
    PT_CTRL_DATA_ST *pstCtrlData = NULL;
    PT_CTRL_DATA_ST stVerRet;
    PT_CTRL_DATA_ST stCtrlData;
    PT_MSG_TAG      *pstMsgDes = (PT_MSG_TAG *)pData;
    struct timeval  stHBEndTime;
    S8  szDestKey[PT_LOGIN_VERIFY_SIZE]  = {0};
    S8  acBuff[sizeof(PT_CTRL_DATA_ST)]  = {0};
    S8  szPtcName[PT_PTC_NAME_LEN]       = {0};
    S8  szPtsIp[PT_IP_ADDR_SIZE]         = {0};
    S8  szPtsPort[PT_DATA_BUFF_16]       = {0};
    U8  paucIPAddr[IPV6_SIZE]            = {0};
    S32 lResult = 0;

    pstCtrlData = (PT_CTRL_DATA_ST *)(pData + sizeof(PT_MSG_TAG));
    switch (pstMsgDes->ulStreamID)
    {
    case PT_CTRL_LOGIN_RSP:
        /* 登陆验证 */
        pt_logr_debug("recv login response");
        lResult = ptc_get_udp_user_ip();  /* 获取本机ip */
        if (lResult != DOS_SUCC)
        {
            return;
        }

        lResult = ptc_key_convert(pstCtrlData->szLoginVerify, szDestKey);
        if (lResult < 0)
        {
            break;
        }

        /* 获取ptc名称 */
        lResult = config_get_ptc_name(szPtcName, PT_PTC_NAME_LEN);
        if (lResult != DOS_SUCC)
        {
            pt_logr_info("get ptc name fail\n");
            dos_memzero(stVerRet.szPtcName, PT_PTC_NAME_LEN);
        }
        else
        {
            dos_memcpy(stVerRet.szPtcName, szPtcName, PT_PTC_NAME_LEN);
        }

        /* 发送key到pts */
        dos_memcpy(stVerRet.szLoginVerify, szDestKey, PT_LOGIN_VERIFY_SIZE);
        stVerRet.enCtrlType = PT_CTRL_LOGIN_RSP;
        stVerRet.ulLginVerSeq = dos_htonl(pstMsgDes->lSeq);
        stVerRet.enPtcType = g_enPtcType;
        dos_strcpy(stVerRet.achPtsMajorDomain, g_stServMsg.achPtsMajorDomain);
        dos_strcpy(stVerRet.achPtsMinorDomain, g_stServMsg.achPtsMinorDomain);
        stVerRet.usPtsMajorPort = g_stServMsg.usPtsMajorPort;
        stVerRet.usPtsMinorPort = g_stServMsg.usPtsMinorPort;
        ptc_get_version(stVerRet.szVersion);
        dos_strcpy(stVerRet.szPtsHistoryIp1, g_stServMsg.szPtsLasterIP1);
        dos_strcpy(stVerRet.szPtsHistoryIp2, g_stServMsg.szPtsLasterIP2);
        dos_strcpy(stVerRet.szPtsHistoryIp3, g_stServMsg.szPtsLasterIP3);
        stVerRet.usPtsHistoryPort1 = g_stServMsg.usPtsLasterPort1;
        stVerRet.usPtsHistoryPort2 = g_stServMsg.usPtsLasterPort2;
        stVerRet.usPtsHistoryPort3 = g_stServMsg.usPtsLasterPort3;
        dos_strncpy(stVerRet.szMac, g_stServMsg.szMac, PT_DATA_BUFF_64);
        dos_memcpy(acBuff, (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

        ptc_send_login_rsp_to_pts(&stVerRet, pstMsgDes);

        break;
    case PT_CTRL_LOGIN_ACK:
        /* 登陆结果 */
        switch (pstCtrlData->ucLoginRes)
        {
            case PT_PTC_LOGIN_SUCC:
                /* 成功 */
                pt_logr_notic("ptc login succ");
                g_bIsOnLine = DOS_TRUE;
                g_ulHDTimeoutCount = 0;
                g_ulConnectPtsCount = 0;
                break;
            case PT_PTC_LOGIN_FAIL_LICENSE:
                pt_logr_notic("ptc login fail, the license do not allow");
                g_bIsOnLine = DOS_FALSE;
                break;
            case PT_PTC_LOGIN_FAIL_SN:
                pt_logr_notic("ptc login fail, request login ipcc SN format error");
                g_bIsOnLine = DOS_FALSE;
                break;
            case PT_PTC_LOGIN_FAIL_VERIFY:
                pt_logr_notic("ptc login fail, verify fail");
                g_bIsOnLine = DOS_FALSE;
                break;
            default:
                pt_logr_notic("ptc login fail, default");
                g_bIsOnLine = DOS_FALSE;
                break;
        }

        /* 通知pts，清除登录的缓存 */
        ptc_send_exit_notify_to_pts(PT_DATA_CTRL, PT_CTRL_LOGIN_RSP, 0);
        break;
    case PT_CTRL_HB_RSP:
        /* 心跳响应，关闭定时器 */
        gettimeofday(&stHBEndTime, NULL);
        ptc_get_hb_time_interval(stHBEndTime);
        pt_logr_debug("recv from pts hb rsp");
        if (DOS_ADDR_VALID(g_stACKTmrHandle))
        {
            dos_tmr_stop(&g_stACKTmrHandle);
        }
        g_stACKTmrHandle = NULL;
        g_ulHDTimeoutCount = 0;
        break;
    case PT_CTRL_SWITCH_PTS:
        /* 切换pts */
        ptc_send_logout2pts();
        g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(pstMsgDes->aulServIp);
        g_pstPtcSend->stDestAddr.sin_port = pstMsgDes->usServPort;

        /* 写入文件,记录注册的pts的历史记录 */
        //if (*(U32 *)pstMsgDes->aulServIp != *(U32 *)g_stServMsg.achPtsMajorIP && *(U32 *)pstMsgDes->aulServIp != *(U32 *)g_stServMsg.achPtsMinorIP)
        ptc_update_pts_history_file(pstMsgDes->aulServIp, pstMsgDes->usServPort);
        ptc_send_login2pts();
        break;
    case PT_CTRL_PTS_MAJOR_DOMAIN:
        /* 修改主域名 */
        dos_memzero(&stCtrlData, sizeof(PT_CTRL_DATA_ST));
        stCtrlData.enCtrlType = PT_CTRL_PTS_MAJOR_DOMAIN;
        if (pstCtrlData->achPtsMajorDomain[0] != '\0')
        {
            /* 解析域名 */
            if (pt_is_or_not_ip(pstCtrlData->achPtsMajorDomain))
            {
                if (DOS_SUCC == config_set_pts_major_domain(pstCtrlData->achPtsMajorDomain))
                {
                    if (!config_save())
                    {
                        /* 保存成功 */
                        dos_strncpy(szPtsIp, pstCtrlData->achPtsMajorDomain, PT_IP_ADDR_SIZE);
                        inet_pton(AF_INET, szPtsIp, (VOID *)(g_stServMsg.achPtsMajorIP));
                        dos_strncpy(g_stServMsg.achPtsMajorDomain, pstCtrlData->achPtsMajorDomain, PT_DATA_BUFF_64-1);
                        dos_strncpy(stCtrlData.achPtsMajorDomain, pstCtrlData->achPtsMajorDomain, PT_DATA_BUFF_64-1);
                    }
                }
            }
            else
            {
                lResult = pt_DNS_analyze(pstCtrlData->achPtsMajorDomain, paucIPAddr);
                if (lResult <= 0)
                {
                    logr_info("1DNS fail");
                }
                else
                {
                    inet_ntop(AF_INET, (void *)(paucIPAddr), szPtsIp, PT_IP_ADDR_SIZE);
                    if (DOS_SUCC == config_set_pts_major_domain(pstCtrlData->achPtsMajorDomain))
                    {
                        if (!config_save())
                        {
                            /* 保存成功 */
                            inet_pton(AF_INET, szPtsIp, (VOID *)(g_stServMsg.achPtsMajorIP));
                            dos_strncpy(g_stServMsg.achPtsMajorDomain, pstCtrlData->achPtsMajorDomain, PT_DATA_BUFF_64-1);
                            dos_strncpy(stCtrlData.achPtsMajorDomain, pstCtrlData->achPtsMajorDomain, PT_DATA_BUFF_64-1);
                        }
                    }
                }
            }
        }

        if (pstCtrlData->usPtsMajorPort != 0)
        {
            sprintf(szPtsPort, "%d", dos_ntohs(pstCtrlData->usPtsMajorPort));
            if (DOS_SUCC == config_set_pts_major_port(szPtsPort))
            {
                if (!config_save())
                {
                    g_stServMsg.usPtsMajorPort = pstCtrlData->usPtsMajorPort;
                    stCtrlData.usPtsMajorPort = pstCtrlData->usPtsMajorPort;
                }
            }
        }

        ptc_save_msg_into_cache(PT_DATA_CTRL, PT_CTRL_PTS_MAJOR_DOMAIN, (S8 *)&stCtrlData, sizeof(PT_CTRL_DATA_ST));
        break;
    case PT_CTRL_PTS_MINOR_DOMAIN:
        /* 修改备域名 */
        dos_memzero(&stCtrlData, sizeof(PT_CTRL_DATA_ST));
        stCtrlData.enCtrlType = PT_CTRL_PTS_MINOR_DOMAIN;
        if (pstCtrlData->achPtsMinorDomain[0] != '\0')
        {
            /* 解析域名 */
            if (pt_is_or_not_ip(pstCtrlData->achPtsMinorDomain))
            {
                if (DOS_SUCC == config_set_pts_minor_domain(pstCtrlData->achPtsMinorDomain))
                {
                    if (!config_save())
                    {
                        /* 保存成功 */
                        dos_strncpy(szPtsIp, pstCtrlData->achPtsMinorDomain, PT_IP_ADDR_SIZE);
                        inet_pton(AF_INET, szPtsIp, (VOID *)(g_stServMsg.achPtsMajorIP));
                        dos_strncpy(g_stServMsg.achPtsMinorDomain, pstCtrlData->achPtsMinorDomain, PT_DATA_BUFF_64-1);
                        dos_strncpy(stCtrlData.achPtsMinorDomain, pstCtrlData->achPtsMinorDomain, PT_DATA_BUFF_64-1);
                    }
                }
            }
            else
            {
                lResult = pt_DNS_analyze(pstCtrlData->achPtsMinorDomain, paucIPAddr);
                if (lResult <= 0)
                {
                    logr_info("1DNS fail");
                }
                else
                {
                    inet_ntop(AF_INET, (void *)(paucIPAddr), szPtsIp, PT_IP_ADDR_SIZE);
                    if (DOS_SUCC == config_set_pts_minor_domain(pstCtrlData->achPtsMinorDomain))
                    {
                        if (!config_save())
                        {
                            /* 保存成功 */
                            inet_pton(AF_INET, szPtsIp, (VOID *)(g_stServMsg.achPtsMajorIP));
                            dos_strncpy(g_stServMsg.achPtsMinorDomain, pstCtrlData->achPtsMinorDomain, PT_DATA_BUFF_64-1);
                            dos_strncpy(stCtrlData.achPtsMinorDomain, pstCtrlData->achPtsMinorDomain, PT_DATA_BUFF_64-1);
                        }
                    }
                }
            }
        }

        if (pstCtrlData->usPtsMinorPort != 0)
        {
            sprintf(szPtsPort, "%d", dos_ntohs(pstCtrlData->usPtsMinorPort));
            if (DOS_SUCC == config_set_pts_minor_port(szPtsPort))
            {
                if (!config_save())
                {
                    g_stServMsg.usPtsMinorPort = pstCtrlData->usPtsMinorPort;
                    stCtrlData.usPtsMinorPort = pstCtrlData->usPtsMinorPort;
                }
            }
        }

        ptc_save_msg_into_cache(PT_DATA_CTRL, PT_CTRL_PTS_MINOR_DOMAIN, (S8 *)&stCtrlData, sizeof(PT_CTRL_DATA_ST));
        break;
    case PT_CTRL_PING:
        ptc_save_msg_into_cache(PT_DATA_CTRL, PT_CTRL_PING_ACK, pData + sizeof(PT_MSG_TAG), lRecvLen - sizeof(PT_MSG_TAG));
        break;
    default:
        break;
    }

    return;
}

/**
 * 函数：S32 ptc_deal_with_confirm_msg(PT_MSG_TAG *pstMsgDes)
 * 功能：
 *      1.处理消息确认消息
 * 参数
 *
 * 返回值：无
 */
S32 ptc_deal_with_confirm_msg(PT_MSG_TAG *pstMsgDes)
{
    PT_STREAM_CB_ST    *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST    *pstStreamNode      = NULL;
    S32 lResult = DOS_FALSE;

    pstStreamListHead = g_pstPtcSend->pstStreamHead;
    pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
    if (NULL == pstStreamNode)
    {
        return lResult;
    }

    if (pstStreamNode->lConfirmSeq < pstMsgDes->lSeq)
    {
        pstStreamNode->lConfirmSeq = pstMsgDes->lSeq;
        lResult = DOS_TRUE;
    }

    return lResult;
}

/**
 * 函数：S8 *ptc_get_version()
 * 功能：
 *      1.获取版本号
 * 参数
 * 返回值：
 */
S32 ptc_get_version(U8 *szVersion)
{
    if (NULL == szVersion)
    {
        return DOS_FALSE;
    }

    inet_pton(AF_INET, DOS_PROCESS_VERSION, (VOID *)szVersion);

    return DOS_SUCC;
}

/**
 * 函数：VOID ptc_send_heartbeat2pts(S32 lSockfd)
 * 功能：
 *      1.开启定时器，发送心跳请求
 * 参数
 * 返回值：
 */
VOID ptc_send_heartbeat2pts()
{
    S32 lResult = 0;

    lResult = dos_tmr_start(&g_stHBTmrHandle, PTC_SEND_HB_TIME, ptc_send_hb_req, (U64)g_ulUdpSocket, TIMER_NORMAL_LOOP);
    if (lResult < 0)
    {
        pt_logr_info("ptc_send_heartbeat2pts : start timer fail");
    }
}

/**
 * 函数：VOID ptc_send_login2pts(S32 lSockfd)
 * 功能：
 *      1.开启定时器，发送登陆请求
 * 参数
 * 返回值：
 */
VOID ptc_send_login2pts()
{
    S32 lResult = 0;
    ptc_send_login_req(0);
    lResult = dos_tmr_start(&g_stHBTmrHandle, PTC_SEND_HB_TIME, ptc_send_login_req, 0, TIMER_NORMAL_LOOP);
    if (lResult < 0)
    {
        pt_logr_info("ptc_send_login2pts : start timer fail");
    }
}

/**
 * 函数：VOID ptc_send_logout2pts(S32 lSockfd)
 * 功能：
 *      1.开启定时器，发送退出登陆请求
 * 参数
 * 返回值：
 */
VOID ptc_send_logout2pts()
{
    g_bIsOnLine = DOS_FALSE;
    dos_tmr_stop(&g_stHBTmrHandle);
    ptc_send_logout_req();
}


/**
 * 函数：void ptc_ipcc_send_msg(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen, U8 ExitNotifyFlag)
 * 功能：
 *      1.添加数据到发送缓存区
 *      2.如果添加成功，则通知发送线程，发送数据
 * 参数
 *      PT_DATA_TYPE_EN enDataType    ：data的类型
 *      U32 ulStreamID                ：stream ID
 *      S8 *pcData                    ：包内容
 *      S32 lDataLen                  ：包内容长度
 *      U8 ExitNotifyFlag             ：通知对方响应是否结束
 * 返回值：无
 */
S32 ptc_save_msg_into_cache(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen)
{
    S32 lResult = 0;
    PT_MSG_TAG stMsgDes;
    //struct timespec stSemTime;
    //struct timeval now;
    if (enDataType < 0 || enDataType >= PT_DATA_BUTT)
    {
        pt_logr_debug("ptc_save_msg_into_cache : enDataType should in 0-%d: %d", PT_DATA_BUTT, enDataType);
        return PT_SAVE_DATA_FAIL;
    }
    else if (NULL == pcData)
    {
        pt_logr_debug("ptc_save_msg_into_cache : send data is NULL");
        return PT_SAVE_DATA_FAIL;
    }

    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_NORMAL;

    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.ulStreamID     = ulStreamID;
    stMsgDes.enDataType     = enDataType;

    lResult = ptc_save_into_send_cache(&stMsgDes, pcData, lDataLen);
    if (lResult < 0)
    {
        /* 添加到发送缓存失败 */
        return PT_SAVE_DATA_FAIL;
    }
    else
    {
        /* 添加到发送消息队列 */

        pthread_mutex_lock(&g_mutexPtcSendPthread);
        if (NULL == pt_need_send_node_list_search(&g_stPtcNendSendNode, ulStreamID))
        {
            pt_need_send_node_list_insert(&g_stPtcNendSendNode, g_pstPtcSend->aucID, &stMsgDes, enCmdValue, bIsResend);
        }
        pthread_cond_signal(&g_condPtcSend);
        pthread_mutex_unlock(&g_mutexPtcSendPthread);
    }

    usleep(10); /* 保证切换线程 */

    return lResult;
}

/**
 * 函数：void *ptc_send_msg2pts(VOID *arg)
 * 功能：
 *      1.发送数据线程
 * 参数
 *      VOID *arg :通道通信的sockfd
 * 返回值：void *
 */
VOID *ptc_send_msg2pts(VOID *arg)
{
    U32              ulArraySub       = 0;
    U32              ulSendCount      = 0;
    PT_STREAM_CB_ST  *pstStreamHead   = NULL;
    list_t           *pstNendSendList = NULL;
    PT_STREAM_CB_ST  *pstStreamNode   = NULL;
    PT_DATA_TCP_ST   *pstSendDataHead = NULL;
    PT_DATA_TCP_ST   stSendDataNode;
    PT_MSG_TAG       stMsgDes;
    S8 acBuff[PT_SEND_DATA_SIZE]      = {0};
    PT_NEND_SEND_NODE_ST *pstNeedSendNode = NULL;
    PT_DATA_TCP_ST    stRecvDataTcp;
    struct timeval now;
    struct timespec timeout;
    S32 lResult = 0;

    dos_list_init(&g_stPtcNendRecvNode);
    dos_list_init(&g_stPtcNendSendNode);

    while (1)
    {
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = (now.tv_usec) * 1000;

        pthread_mutex_lock(&g_mutexPtcSendPthread);
        pthread_cond_timedwait(&g_condPtcSend, &g_mutexPtcSendPthread, &timeout);
        pthread_mutex_unlock(&g_mutexPtcSendPthread);

        DOS_LOOP_DETECT_INIT(lLoopMaxCount, DOS_DEFAULT_MAX_LOOP);

        while(1)
        {
            /* 防止死循环 */
            DOS_LOOP_DETECT(lLoopMaxCount)
            pthread_mutex_lock(&g_mutexPtcSendPthread);
            if (dos_list_is_empty(&g_stPtcNendSendNode))
            {
                pthread_mutex_unlock(&g_mutexPtcSendPthread);
                break;
            }

            pstNendSendList = dos_list_fetch(&g_stPtcNendSendNode);
            if (DOS_ADDR_INVALID(pstNendSendList))
            {
                pthread_mutex_unlock(&g_mutexPtcSendPthread);
                DOS_ASSERT(0);
                continue;
            }
            pthread_mutex_unlock(&g_mutexPtcSendPthread);
            pstNeedSendNode = dos_list_entry(pstNendSendList, PT_NEND_SEND_NODE_ST, stListNode);

            dos_memzero(&stMsgDes, sizeof(PT_MSG_TAG));

            if (pstNeedSendNode->enCmdValue != PT_CMD_NORMAL || pstNeedSendNode->ExitNotifyFlag == DOS_TRUE)
            {
                /* 发送 lost/resend/confirm/exitNotify消息 */
                stMsgDes.enDataType = pstNeedSendNode->enDataType;
                dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
                stMsgDes.ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                stMsgDes.ExitNotifyFlag = pstNeedSendNode->ExitNotifyFlag;
                stMsgDes.lSeq = dos_htonl(pstNeedSendNode->lSeqResend);
                stMsgDes.enCmdValue = pstNeedSendNode->enCmdValue;
                stMsgDes.bIsEncrypt = DOS_FALSE;
                stMsgDes.bIsCompress = DOS_FALSE;
                dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
                stMsgDes.usServPort = g_stServMsg.usLocalPort;
                pt_logr_debug("ptc_send_msg2pts : lost/resend/confirm/exitNotify request, seq : %d", pstNeedSendNode->lSeqResend);
                printf("ptc_send_msg2pts : lost/resend/confirm/exitNotify request, seq : %d\n", pstNeedSendNode->lSeqResend);
                if (g_ulUdpSocket > 0)
                {
                    lResult = sendto(g_ulUdpSocket, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG), 0,  (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
                    if (lResult < 0)
                    {
                        close(g_ulUdpSocket);
                        g_ulUdpSocket = -1;
                        g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);
                    }
                }
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }
            /* 发送数据包 */
            pstStreamHead = g_pstPtcSend->pstStreamHead;
            if (NULL == pstStreamHead)
            {
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                //pthread_mutex_unlock(&g_mutexSendCache);

                continue;
            }

            pstStreamNode = pt_stream_queue_search(pstStreamHead, pstNeedSendNode->ulStreamID);
            if(NULL == pstStreamNode)
            {
                pt_logr_info("ptc_send_msg2pts : stream node cann't found : %d", pstNeedSendNode->ulStreamID);
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;

                continue;
            }

            pstSendDataHead = pstStreamNode->unDataQueHead.pstDataTcp;
            if (NULL == pstSendDataHead)
            {
                pt_logr_info("ptc_send_msg2pts : data queue is NULL");
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
                    pt_logr_info("ptc_send_msg2pts : need resend data is not exit : seq = %d ", pstNeedSendNode->lSeqResend);
                }
                else
                {
                    stSendDataNode = pstSendDataHead[ulArraySub];
                    stMsgDes.enDataType = pstNeedSendNode->enDataType;
                    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
                    stMsgDes.ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                    stMsgDes.ExitNotifyFlag = stSendDataNode.ExitNotifyFlag;
                    stMsgDes.lSeq = dos_htonl(pstNeedSendNode->lSeqResend);
                    stMsgDes.enCmdValue = pstNeedSendNode->enCmdValue;
                    stMsgDes.bIsEncrypt = DOS_FALSE;
                    stMsgDes.bIsCompress = DOS_FALSE;
                    dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
                    stMsgDes.usServPort = g_stServMsg.usLocalPort;

                    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
                    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), stSendDataNode.szBuff, stSendDataNode.ulLen);

                    /* 重传的，发送三遍 */
                    ulSendCount = PT_RESEND_RSP_COUNT;
                    while (ulSendCount)
                    {
                        //usleep(g_ulSendTimeSleep);
                        if (g_ulUdpSocket > 0)
                        {
                            lResult = sendto(g_ulUdpSocket, acBuff, stSendDataNode.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
                            if (lResult  < 0)
                            {
                                close(g_ulUdpSocket);
                                g_ulUdpSocket = -1;
                                g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);
                            }
                        }
                        ulSendCount--;
                        pt_logr_debug("send resend data seq : %d, stream, result = %d", pstNeedSendNode->lSeqResend, pstNeedSendNode->ulStreamID, lResult);

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
                if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
                {
                    break;
                }
                stRecvDataTcp = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
                if (stRecvDataTcp.lSeq == pstStreamNode->lCurrSeq)
                {
                    stSendDataNode = pstSendDataHead[ulArraySub];
                    stMsgDes.enDataType = pstNeedSendNode->enDataType;
                    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
                    stMsgDes.ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                    stMsgDes.ExitNotifyFlag = stSendDataNode.ExitNotifyFlag;
                    stMsgDes.lSeq = dos_htonl(pstStreamNode->lCurrSeq);
                    stMsgDes.enCmdValue = pstNeedSendNode->enCmdValue;
                    stMsgDes.bIsEncrypt = DOS_FALSE;
                    stMsgDes.bIsCompress = DOS_FALSE;
                    dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
                    stMsgDes.usServPort = g_stServMsg.usLocalPort;

                    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
                    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), stSendDataNode.szBuff, stSendDataNode.ulLen);
#if 0
                    if (0 == stSendDataNode.ulLen)
                    {
                        ulSendCount = PT_RESEND_RSP_COUNT;
                        while (ulSendCount)
                        {
                            if (g_ulUdpSocket > 0)
                            {
                                lResult = sendto(g_ulUdpSocket, acBuff, stSendDataNode.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
                                if (lResult  < 0)
                                {
                                    close(g_ulUdpSocket);
                                    g_ulUdpSocket = -1;
                                    g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);
                                }
                            }
                            ulSendCount--;
                        }
                    }
                    else
#endif
                    {
                        if (g_ulUdpSocket > 0)
                        {
                            usleep(20);
                            lResult = sendto(g_ulUdpSocket, acBuff, stSendDataNode.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
                            if (lResult  < 0)
                            {
                                close(g_ulUdpSocket);
                                g_ulUdpSocket = -1;
                                g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);
                            }
                        }
                    }
                    pt_logr_debug("send data to pts : length:%d, stream:%d, seq:%d", stRecvDataTcp.ulLen, pstNeedSendNode->ulStreamID, pstStreamNode->lCurrSeq);
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

        }
    }
    return NULL;
}

/**
 * 函数：void *ptc_recv_msg_from_amc(VOID *arg)
 * 功能：
 *      1.接收数据线程
 * 参数
 *      VOID *arg :通道通信的sockfd
 * 返回值：void *
 */
VOID *ptc_recv_msg_from_pts(VOID *arg)
{
    S32         lRecvLen   = 0;
    S32         lResult    = 0;
    S32         lSaveIntoCacheRes = 0;
    U32         MaxFdp     = 0;
    PT_MSG_TAG *pstMsgDes  = NULL;
    struct sockaddr_in stClientAddr = g_pstPtcSend->stDestAddr;
    socklen_t          ulCliaddrLen = sizeof(stClientAddr);
    fd_set             ReadFds;
    struct timeval stTimeVal = {1, 0};
    struct timeval stTimeValCpy;

    S8 acRecvBuf[PT_SEND_DATA_SIZE]  = {0};
    struct timeval now;
    sem_init(&g_SemPtcRecv, 0, 1);              /* 初始化信号量 */

    while (1)
    {
        if (g_ulUdpSocket < 0)
        {
            sleep(1);
            continue;
        }
        stTimeValCpy = stTimeVal;
        MaxFdp = g_ulUdpSocket;
        FD_ZERO(&ReadFds);
        FD_SET(g_ulUdpSocket, &ReadFds);

        lResult = select(MaxFdp + 1, &ReadFds, NULL, NULL, &stTimeValCpy);
        if (lResult < 0)
        {
            perror("ptc_recv_msg_from_pts : fail to select");
            DOS_ASSERT(0);
            sleep(1);
            continue;
        }
        else if (0 == lResult)
        {
            continue;
        }

        if (FD_ISSET(g_ulUdpSocket, &ReadFds))
        {
            lRecvLen = recvfrom(g_ulUdpSocket, acRecvBuf, PT_SEND_DATA_SIZE, 0, (struct sockaddr*)&stClientAddr, &ulCliaddrLen);
            if (lRecvLen < 0)
            {
                pt_logr_info("recvfrom fail ,create socket again");
                close(g_ulUdpSocket);
                g_ulUdpSocket = -1;
                g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);

                continue;
            }

            sem_wait(&g_SemPtcRecv);

            /* 取出消息头部数据 */
            pstMsgDes = (PT_MSG_TAG *)acRecvBuf;
            /* 字节序转换 */
            pstMsgDes->ulStreamID = dos_ntohl(pstMsgDes->ulStreamID);
            pstMsgDes->lSeq = dos_ntohl(pstMsgDes->lSeq);

            if (pstMsgDes->enDataType == PT_DATA_CTRL && pstMsgDes->ulStreamID != PT_CTRL_PTC_PACKAGE)
            {
                /* 控制消息 */
                ptc_ctrl_msg_handle(acRecvBuf, lRecvLen);

                sem_post(&g_SemPtcRecv);
                continue;
            }

            if (pstMsgDes->enCmdValue == PT_CMD_RESEND)
            {
                /* 重传请求 */
                pt_logr_info("ptc recv resend req, seq : %d, stream : %d", pstMsgDes->lSeq, pstMsgDes->ulStreamID);
                printf("recv resend req, seq : %d, stream : %d\n", pstMsgDes->lSeq, pstMsgDes->ulStreamID);
                BOOL bIsResend = DOS_TRUE;
                PT_CMD_EN enCmdValue = PT_CMD_NORMAL;

                pthread_mutex_lock(&g_mutexPtcSendPthread);
                pt_need_send_node_list_insert(&g_stPtcNendSendNode, g_pstPtcSend->aucID, pstMsgDes, enCmdValue, bIsResend);
                pthread_cond_signal(&g_condPtcSend);
                pthread_mutex_unlock(&g_mutexPtcSendPthread);

                sem_post(&g_SemPtcRecv);
                continue;
            }
            else if (pstMsgDes->enCmdValue == PT_CMD_CONFIRM)
            {
                /* 确认接收 */
                gettimeofday(&now, NULL);
                pt_logr_info("make sure recv seq : %d", pstMsgDes->lSeq);
                lResult = ptc_deal_with_confirm_msg(pstMsgDes);
                if (lResult)
                {
                    if (PT_DATA_WEB == pstMsgDes->enDataType)
                    {
                        ptc_set_cache_full_false(pstMsgDes->ulStreamID, PT_DATA_WEB);
                        send(g_lPipeWRFd, "s", 1, 0);
                    }
                }

                sem_post(&g_SemPtcRecv);
                continue;
            }
            else if (pstMsgDes->ExitNotifyFlag == DOS_TRUE)
            {
                /* 响应结束，清除streamID节点 */
                ptc_delete_send_stream_node_by_streamID(pstMsgDes->ulStreamID);
                ptc_delete_client_by_streamID(pstMsgDes->ulStreamID);

                sem_post(&g_SemPtcRecv);
                continue;
            }
            else
            {
                pt_logr_debug("ptc recv from pts stream : %d, seq : %d", pstMsgDes->ulStreamID, pstMsgDes->lSeq);
                lSaveIntoCacheRes = ptc_save_into_recv_cache(acRecvBuf, lRecvLen);
                if (lSaveIntoCacheRes < 0)
                {
                    sem_post(&g_SemPtcRecv);
                    continue;
                }

                pthread_mutex_lock(&g_mutexPtcRecvPthread);
                if (NULL == pt_need_recv_node_list_search(&g_stPtcNendRecvNode, pstMsgDes->ulStreamID))
                {
                    pt_need_recv_node_list_insert(&g_stPtcNendRecvNode, pstMsgDes);
                }
                pthread_cond_signal(&g_condPtcRecv);
                pthread_mutex_unlock(&g_mutexPtcRecvPthread);
            }

            if (lSaveIntoCacheRes == PT_NEED_CUT_PTHREAD)
            {
                usleep(10); /* 挂起线程，执行接收函数 */
            }
        }
    }

    return NULL;
}

/**
 * 函数：VOID *ptc_send_msg2proxy(VOID *arg)
 * 功能：
 *      1.线程 发送消息到proxy
 * 参数
 * 返回值：
 */
VOID *ptc_send_msg2proxy(VOID *arg)
{
    list_t *pstNendRecvList = NULL;
    PT_NEND_RECV_NODE_ST *pstNeedRecvNode = NULL;
    struct timeval now;
    struct timespec timeout;

    while (1)
    {
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = now.tv_usec * 1000;

        pthread_mutex_lock(&g_mutexPtcRecvPthread);
        sem_post(&g_SemPtcRecv);
        pthread_cond_timedwait(&g_condPtcRecv, &g_mutexPtcRecvPthread, &timeout);
        pthread_mutex_unlock(&g_mutexPtcRecvPthread);

        /* 循环发送g_pstPtsNendRecvNode中的stream */

        DOS_LOOP_DETECT_INIT(lLoopMaxCount, DOS_DEFAULT_MAX_LOOP);

        while(1)
        {
            /* 防止死循环 */
            DOS_LOOP_DETECT(lLoopMaxCount)
            pthread_mutex_lock(&g_mutexPtcRecvPthread);
            if (dos_list_is_empty(&g_stPtcNendRecvNode))
            {
                pthread_mutex_unlock(&g_mutexPtcRecvPthread);
                break;
            }

            pstNendRecvList = dos_list_fetch(&g_stPtcNendRecvNode);
            if (DOS_ADDR_INVALID(pstNendRecvList))
            {
                pthread_mutex_unlock(&g_mutexPtcRecvPthread);
                DOS_ASSERT(0);
                continue;
            }
            pthread_mutex_unlock(&g_mutexPtcRecvPthread);

            pstNeedRecvNode = dos_list_entry(pstNendRecvList, PT_NEND_RECV_NODE_ST, stListNode);

            if (PT_DATA_CTRL == pstNeedRecvNode->enDataType)
            {
                if (PT_CTRL_PTC_PACKAGE == pstNeedRecvNode->ulStreamID)
                {
                    ptc_recv_upgrade_package(pstNeedRecvNode);
                }
                dos_dmem_free(pstNeedRecvNode);
                pstNeedRecvNode = NULL;
                continue;
            }
            else if (PT_DATA_WEB == pstNeedRecvNode->enDataType)
            {
                ptc_send_msg2web(pstNeedRecvNode);
                dos_dmem_free(pstNeedRecvNode);
                pstNeedRecvNode = NULL;
                continue;
            }
            else if (PT_DATA_CMD == pstNeedRecvNode->enDataType)
            {
                ptc_send_msg2cmd(pstNeedRecvNode);
                dos_dmem_free(pstNeedRecvNode);
                pstNeedRecvNode = NULL;
                continue;
            }
            else
            {
                dos_dmem_free(pstNeedRecvNode);
                pstNeedRecvNode = NULL;
            }
        }
    }

    return NULL;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

