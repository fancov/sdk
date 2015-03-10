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
/* include provate header file */
#include <pt/dos_sqlite3.h>
#include <pt/ptc.h>
#include "ptc_msg.h"

sem_t g_SemPtc;                             /* 控制缓存信号量 */
sem_t g_SemPtcRecv;
U32  g_ulHDTimeoutCount = 0;                /* 心跳超时次数 */
BOOL g_bIsOnLine = DOS_FALSE;               /* 设备是否在线 */
DOS_TMR_ST g_stHBTmrHandle    = NULL;       /* 心跳定时器 */
DOS_TMR_ST g_stACKTmrHandle   = NULL;       /* 登陆、心跳接收响应的定时器 */
PT_CC_CB_ST *g_pstPtcSend     = NULL;       /* 发送缓存中ptc节点 */
PT_CC_CB_ST *g_pstPtcRecv     = NULL;       /* 接收缓存中ptc节点 */
list_t  *g_pstPtcNendRecvNode = NULL;       /* 需要接收的包的消息队列 */
list_t  *g_pstPtcNendSendNode = NULL;       /* 需要发送的包的消息队列 */
PTC_SERV_MSG_ST g_stServMsg;                /* 存放ptc信息的全局变量 */
pthread_mutex_t g_mutex_send  = PTHREAD_MUTEX_INITIALIZER;       /* 发送线程锁 */
pthread_cond_t  g_cond_send   = PTHREAD_COND_INITIALIZER;        /* 发送条件变量 */
pthread_mutex_t g_mutex_recv  = PTHREAD_MUTEX_INITIALIZER;       /* 接收线程锁 */
pthread_cond_t  g_cond_recv   = PTHREAD_COND_INITIALIZER;        /* 接收条件变量 */
U32 g_ulSendTimeSleep = 20;
static S32 g_lUdpSocket = 0;
U32 g_ulConnectPtsCount = 0;
PT_PTC_TYPE_EN g_enPtcType = PT_PTC_TYPE_IPCC;

S8 *ptc_get_current_time()
{
    time_t ulCurrTime = 0;

    ulCurrTime = time(NULL);
    return ctime(&ulCurrTime);
}

VOID ptc_send_pthread_mutex_lock(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : send\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
    pthread_mutex_lock(&g_mutex_send);
    printf("%s %d %.*s : send lock\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
}

VOID ptc_send_pthread_mutex_unlock(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : send unlock\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
    pthread_mutex_unlock(&g_mutex_send);
}

VOID ptc_send_pthread_cond_timedwait(struct timespec *timeout, S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : send wait\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
    pthread_cond_timedwait(&g_cond_send, &g_mutex_send, timeout); /* 必须使用这个的函数 */
    printf("%s %d %.*s : send lock\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
}

VOID ptc_recv_pthread_mutex_lock(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : recv\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
    pthread_mutex_lock(&g_mutex_recv);
    printf("%s %d %.*s : recv lock\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
}

VOID ptc_recv_pthread_mutex_unlock(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : recv unlock\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
    pthread_mutex_unlock(&g_mutex_recv);
}

VOID ptc_recv_pthread_cond_wait(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : recv wait\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
    pthread_cond_wait(&g_cond_recv, &g_mutex_recv);
    printf("%s %d %.*s : recv wait in\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
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
VOID ptc_delete_send_stream_node(U32 ulStreamID, PT_DATA_TYPE_EN enDataType, BOOL bIsMutex)
{
    list_t *pstStreamListHead = NULL;
    PT_STREAM_CB_ST *pstStreamNode = NULL;

    if (bIsMutex)
    {
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
        #else
        pthread_mutex_lock(&g_mutex_send);
        #endif
    }

    if (g_pstPtcSend != NULL)
    {
        pstStreamListHead = g_pstPtcSend->astDataTypes[enDataType].pstStreamQueHead;
        if (pstStreamListHead != NULL)
        {
            pstStreamNode = pt_stream_queue_search(pstStreamListHead, ulStreamID);
            if (pstStreamNode != NULL)
            {
                pstStreamListHead = pt_delete_stream_node(pstStreamListHead, &pstStreamNode->stStreamListNode, enDataType);
                g_pstPtcSend->astDataTypes[enDataType].pstStreamQueHead = pstStreamListHead;
             }
        }
    }

    if (bIsMutex)
    {
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_send);
        #endif
    }
}

/**
 * 函数：VOID ptc_delete_recv_stream_node(U32 ulStreamID, PT_DATA_TYPE_EN enDataType)
 * 功能：
 *      1.删除接收缓存中， 一个stream节点
 * 参数
 *      U32 ulStreamID ：stream id
 *      U32 enDataType ：消息类型
 * 返回值：无
 */
VOID ptc_delete_recv_stream_node(U32 ulStreamID, PT_DATA_TYPE_EN enDataType, BOOL bIsMutex)
{
    list_t *pstStreamListHead = NULL;
    PT_STREAM_CB_ST *pstStreamNode = NULL;

    if (bIsMutex)
    {
        #if PT_MUTEX_DEBUG
        ptc_recv_pthread_mutex_lock(__FILE__, __LINE__);
        #else
        pthread_mutex_lock(&g_mutex_recv);
        #endif
    }

    if (g_pstPtcRecv != NULL)
    {
        pstStreamListHead = g_pstPtcRecv->astDataTypes[enDataType].pstStreamQueHead;
        if (pstStreamListHead != NULL)
        {
            pstStreamNode = pt_stream_queue_search(pstStreamListHead, ulStreamID);
            if (pstStreamNode != NULL)
            {
                pstStreamListHead = pt_delete_stream_node(pstStreamListHead, &pstStreamNode->stStreamListNode, enDataType);
                g_pstPtcRecv->astDataTypes[enDataType].pstStreamQueHead = pstStreamListHead;
            }
        }
    }

    if (bIsMutex)
    {
        #if PT_MUTEX_DEBUG
        ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_recv);
        #endif
    }
}

VOID ptc_get_udp_use_ip()
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
        exit(DOS_FAIL);
    }

ptc_connect: lError = connect(lSockfd,(struct sockaddr*)&stServAddr,sizeof(stServAddr));
     if (lError < 0)
     {
        perror("connect pts");
        sleep(2);
        /* 重连 */
        goto ptc_connect;
        exit(DOS_FAIL);
     }

    lError = getsockname(lSockfd,(struct sockaddr*)&stAddrCli, &lAddrLen);
    if (lError != 0)
    {
        pt_logr_info("Get sockname Error!");
        exit(DOS_FAIL);
    }
    g_stServMsg.usLocalPort = dos_ntohs(stAddrCli.sin_port);
    dos_strcpy(szLocalIp, inet_ntoa(stAddrCli.sin_addr));
    inet_pton(AF_INET, szLocalIp, (VOID *)(g_stServMsg.achLocalIP));
    lError = ptc_get_ifr_name_by_ip(szLocalIp, lSockfd, szIfrName, PT_DATA_BUFF_64);
    if (lError != DOS_SUCC)
    {
        pt_logr_info("Get ifr name Error!");
        exit(DOS_FAIL);
    }
    lError = ptc_get_mac(szIfrName, lSockfd, szMac, PT_DATA_BUFF_64);
    if (lError != DOS_SUCC)
    {
        pt_logr_info("Get mac Error!");
        exit(DOS_FAIL);
    }
    dos_strncpy(g_stServMsg.szMac, szMac, PT_DATA_BUFF_64);
    logr_debug("local ip : %s, irf name : %s, mac : %s", szLocalIp, szIfrName, szMac);

    close(lSockfd);
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

    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
    #else
    pthread_mutex_lock(&g_mutex_send);
    #endif
    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_RESEND;
    g_pstPtcNendSendNode = pt_need_send_node_list_insert(g_pstPtcNendSendNode, g_pstPtcSend->aucID, pstMsgDes, enCmdValue, bIsResend);

    pthread_cond_signal(&g_cond_send);
    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
    #else
    pthread_mutex_unlock(&g_mutex_send);
    #endif

    return;
}

VOID ptc_send_exit_notify_to_pts(PT_DATA_TYPE_EN enDataType, U32 ulStreamID)
{
    PT_MSG_TAG stMsgDes;
    S8 acBuff[PT_DATA_BUFF_512] = {0};

    stMsgDes.enDataType = enDataType;
    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(ulStreamID);
    stMsgDes.ExitNotifyFlag = DOS_TRUE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));

    sendto(g_lUdpSocket, acBuff, sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
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
    list_t *pstStreamListHead = NULL;

    if (pstStreamNode->ulCountResend >= PT_RESEND_REQ_MAX_COUNT)
    {
        /* 丢包重发PTC_RESEND_MAX_COUNT次，仍未收到 */
        //ptc_save_msg_into_cache(pstMsg->enDataType, pstMsg->ulStreamID, "", 0, DOS_TRUE); /* 通知pts关闭sockfd */
        ptc_send_exit_notify_to_pts(pstMsg->enDataType, pstMsg->ulStreamID);
        /* 清空ptc中，stream的资源 */
        if (NULL != g_pstPtcRecv)
        {
            pstStreamListHead = g_pstPtcRecv->astDataTypes[pstMsg->enDataType].pstStreamQueHead;
            pt_delete_stream_node(pstStreamListHead, &pstStreamNode->stStreamListNode, pstMsg->enDataType);
            g_pstPtcRecv->astDataTypes[pstMsg->enDataType].pstStreamQueHead = pstStreamListHead;
        }
        ptc_delete_send_stream_node(pstMsg->ulStreamID, pstMsg->enDataType, DOS_FALSE);
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

    if (dos_strncmp(ptc_get_version(), "1.1", dos_strlen("1.1")) == 0)
    {
        /* 1.1版本验证方法 */
        for (i=0; i<PT_LOGIN_VERIFY_SIZE-1; i++)
        {
            szDest[i] = szKey[i]&0xA9;
        }
        szDest[PT_LOGIN_VERIFY_SIZE] = '\0';
    }
    else
    {
        return DOS_FAIL;
    }

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
VOID ptc_hd_ack_timeout_callback(U64 lSockfd)
{
    g_ulHDTimeoutCount++;
    pt_logr_debug("Heartbeat response timeout : %d", g_ulHDTimeoutCount);
    if (g_ulHDTimeoutCount >= PTC_HB_TIMEOUT_COUNT_MAX)
    {
        /* 连续多次无法收到心跳响应，重新发送登陆请求 */
        ptc_send_login2pts(lSockfd);
        g_bIsOnLine = DOS_FALSE;
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
VOID ptc_send_hb_req(U64 lSockfd)
{
    PT_MSG_TAG stMsgDes;
    PT_CTRL_DATA_ST stVerRet;
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
    stVerRet.enCtrlType = PT_CTRL_HB_REQ;
    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

    sendto(lSockfd, acBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
    /* 启动定时器，判断心跳响应是否超时 */
    lResult = dos_tmr_start(&g_stACKTmrHandle, PTC_WAIT_HB_ACK_TIME, ptc_hd_ack_timeout_callback, lSockfd, TIMER_NORMAL_NO_LOOP);
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
    S32 lSockfd = (S32)arg;
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
        ptc_send_heartbeat2pts(lSockfd);

        return;
    }
    g_ulConnectPtsCount++;
    printf("connect pts count : %d\n", g_ulConnectPtsCount);
    if (g_ulConnectPtsCount > 4)
    {
        g_ulConnectPtsCount = 0;
        if (g_pstPtcSend->stDestAddr.sin_addr.s_addr == *(U32 *)(g_stServMsg.achPtsMajorIP))
        {
            /* 主域名注册不上，切换pts到副域名 */
            g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMinorIP);
            g_pstPtcSend->stDestAddr.sin_port = g_stServMsg.usPtsMinorPort;
        }
        else if (g_pstPtcSend->stDestAddr.sin_addr.s_addr == *(U32 *)(g_stServMsg.achPtsMinorIP))
        {
            /* 副域名注册不上，切换到主域名 */
            g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMajorIP);
            g_pstPtcSend->stDestAddr.sin_port = g_stServMsg.usPtsMajorPort;
        }
        else
        {
            /* 其它域名注册不上，切换到主域名 */
            g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMajorIP);
            g_pstPtcSend->stDestAddr.sin_port = g_stServMsg.usPtsMajorPort;
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
    dos_strncpy(stVerRet.szVersion, ptc_get_version(), PT_VERSION_LEN);

    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

    inet_ntop(AF_INET, (void *)(&g_pstPtcSend->stDestAddr.sin_addr.s_addr), szDestIp, PT_IP_ADDR_SIZE);
    sendto(lSockfd, acBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
    logr_debug("ptc send login request to pts : ip : %s", szDestIp);

    /* 获取ptc一侧udp的私网端口号 -- sendto后，才能获取端口号 */
    if (g_stServMsg.usLocalPort == 0)
    {
        lResult = getsockname(lSockfd,(struct sockaddr*)&stAddrCli, &lAddrLen);
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
VOID ptc_send_logout_req(U64 arg)
{
    S32 lSockfd = (S32)arg;
    PT_MSG_TAG stMsgDes;
    PT_CTRL_DATA_ST stVerRet;
    S8 acBuff[PT_DATA_BUFF_512] = {0};

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

    sendto(lSockfd, acBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
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

    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
    #else
    pthread_mutex_lock(&g_mutex_send);
    #endif
    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_CONFIRM;
    pstMsgDes->lSeq = lConfirmSeq;

    pt_logr_debug("send confirm data to pts: type = %d, stream = %d", pstMsgDes->enDataType,pstMsgDes->ulStreamID);
    for (i=0; i<PT_SEND_CONFIRM_COUNT; i++)
    {
        g_pstPtcNendSendNode = pt_need_send_node_list_insert(g_pstPtcNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);
    }
    pthread_cond_signal(&g_cond_send);
    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
    #else
    pthread_mutex_unlock(&g_mutex_send);
    #endif
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
    if (NULL == g_pstPtcSend || NULL == pstMsgDes || NULL == acSendBuf)
    {
        pt_logr_debug("ptc_save_into_send_cache : param error");
        return PT_SAVE_DATA_FAIL;
    }

    list_t          *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST *pstStreamNode      = NULL;
    PT_DATA_TCP_ST  *pstDataQueue       = NULL;
    S32 lResult = 0;

    /* 判断stream是否在接收队列中存在，若不存在，说明这个stream已经结束 */
    if (pstMsgDes->enDataType != PT_DATA_CTRL)
    {
        #if PT_MUTEX_DEBUG
        ptc_recv_pthread_mutex_lock(__FILE__, __LINE__);
        #else
        pthread_mutex_lock(&g_mutex_recv);
        #endif
        pstStreamListHead = g_pstPtcRecv->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
        if (NULL == pstStreamListHead)
        {
            #if PT_MUTEX_DEBUG
            ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
            #else
            pthread_mutex_unlock(&g_mutex_recv);
            #endif
            return PT_SAVE_DATA_FAIL;
        }
        else
        {
            pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
            if (NULL == pstStreamNode)
            {
                #if PT_MUTEX_DEBUG
                ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_recv);
                #endif
                return PT_SAVE_DATA_FAIL;
            }
        }
        #if PT_MUTEX_DEBUG
        ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_recv);
        #endif
    }

    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
    #else
    pthread_mutex_lock(&g_mutex_send);
    #endif
    pstStreamListHead = g_pstPtcSend->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
    if (NULL == pstStreamListHead)
    {
        /* 创建stream list head */
        pstStreamNode = pt_stream_node_create(pstMsgDes->ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* stream node创建失败 */
            #if PT_MUTEX_DEBUG
            ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
            #else
            pthread_mutex_unlock(&g_mutex_send);
            #endif
            return PT_SAVE_DATA_FAIL;
        }

        pstStreamListHead = &(pstStreamNode->stStreamListNode);
        g_pstPtcSend->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead = pstStreamListHead;
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
                #if PT_MUTEX_DEBUG
                ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_send);
                #endif
                return PT_SAVE_DATA_FAIL;
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
            /* create data queue失败,释放掉stream */
            pt_logr_info("ptc_save_into_send_cache : create data queue fail");
            pstStreamListHead = pt_delete_stream_node(pstStreamListHead, &pstStreamNode->stStreamListNode, pstMsgDes->enDataType);
            #if PT_MUTEX_DEBUG
            ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
            #else
            pthread_mutex_unlock(&g_mutex_send);
            #endif
            return PT_SAVE_DATA_FAIL;
        }

        pstStreamNode->unDataQueHead.pstDataTcp = pstDataQueue;
    }

    /* 将数据插入到data queue中 */
    lResult = pt_send_data_tcp_queue_insert(pstStreamNode, acSendBuf, lDataLen, PT_DATA_SEND_CACHE_SIZE);
    if (lResult < 0)
    {
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_send);
        #endif
        return PT_SAVE_DATA_FAIL;
    }

    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
    #else
    pthread_mutex_unlock(&g_mutex_send);
    #endif
    return lResult;
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
S32 ptc_save_into_recv_cache(PT_CC_CB_ST *pstPtcNode, S8 *acRecvBuf, S32 lDataLen, S32 lSockfd)
{
    if (NULL == pstPtcNode || NULL == acRecvBuf)
    {
        return PT_SAVE_DATA_FAIL;
    }

    S32 i = 0;
    U32 ulArraySub = 0;
    S32 lResult = 0;
    U32 ulNextSendArraySub = 0;
    list_t             *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST    *pstStreamNode      = NULL;
    PT_LOSE_BAG_MSG_ST *pstLoseMsg         = NULL;
    PT_DATA_TCP_ST     *pstDataQueue       = NULL;
    PT_MSG_TAG         *pstMsgDes = (PT_MSG_TAG *)acRecvBuf;

    pstStreamListHead = pstPtcNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
    if (NULL == pstStreamListHead)
    {
        /* 创建stream list head */
        pstStreamNode = pt_stream_node_create(pstMsgDes->ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* stream node创建失败 */
            pt_logr_info("ptc_save_into_recv_cache : create stream node fail");
            return PT_SAVE_DATA_FAIL;
        }
        dos_memcpy(pstStreamNode->aulServIp, pstMsgDes->aulServIp, IPV6_SIZE);
        pstStreamNode->usServPort = pstMsgDes->usServPort;

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
                pt_logr_info("ptc_save_into_recv_cache : create stream node fail");
                return PT_SAVE_DATA_FAIL;
            }
            dos_memcpy(pstStreamNode->aulServIp, pstMsgDes->aulServIp, IPV6_SIZE);
            pstStreamNode->usServPort = pstMsgDes->usServPort;
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
            /* create data queue失败 */
            pt_logr_info("ptc_save_into_recv_cache : create tcp data queue fail");
            return PT_SAVE_DATA_FAIL;
        }

        pstStreamNode->unDataQueHead.pstDataTcp = pstDataQueue;
    }

    /* 将数据插入到data queue中 */
    lResult = pt_recv_data_tcp_queue_insert(pstStreamNode, pstMsgDes, acRecvBuf+sizeof(PT_MSG_TAG), lDataLen-sizeof(PT_MSG_TAG), PT_DATA_RECV_CACHE_SIZE);
    if (lResult < 0)
    {
        pt_logr_debug("ptc_save_into_recv_cache : add data into recv cache fail");
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
                pt_logr_debug("lose data, start timer, seq : %d", pstStreamNode->lCurrSeq + 1);
                if (pstStreamNode->pstLostParam == NULL)
                {
                    pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)dos_dmem_alloc(sizeof(PT_LOSE_BAG_MSG_ST));
                    if (NULL == pstLoseMsg)
                    {
                        perror("ptc_save_into_recv_cache : malloc");
                        return PT_SAVE_DATA_FAIL;
                    }

                    pstLoseMsg->stMsg = *pstMsgDes;
                    pstLoseMsg->pstStreamNode = pstStreamNode;
                    pstLoseMsg->lSocket = lSockfd;

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
            return PT_SAVE_DATA_FAIL;
        }
    }
}

VOID ptc_update_pts_history_file(U8 aulServIp[IPV6_SIZE], U16 usServPort)
{
    S8 szPtsIp[PT_IP_ADDR_SIZE] = {0};
    FILE *pFileHandle = NULL;
    S32 lPtsHistoryConut = 0;
    S8 szPtsHistory[3][PT_DATA_BUFF_64];

    inet_ntop(AF_INET, (void *)(aulServIp), szPtsIp, PT_IP_ADDR_SIZE);
    pFileHandle = fopen("pts_history", "r");
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
    pFileHandle = fopen("pts_history", "w");
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
 * 函数：VOID ptc_ctrl_msg_handle(U32 ulStreamID, S8 *pData)
 * 功能：
 *      1.处理控制消息
 * 参数
 *
 * 返回值：无
 */
VOID ptc_ctrl_msg_handle(PT_MSG_TAG *pstMsgDes, S8 *pData)
{
    if (NULL == pData)
    {
        return;
    }
    PT_CTRL_DATA_ST *pstCtrlData = NULL;
    S8 szDestKey[PT_LOGIN_VERIFY_SIZE] = {0};
    S32 lResult = 0;
    PT_CTRL_DATA_ST stVerRet;
    S8 acBuff[sizeof(PT_CTRL_DATA_ST)] = {0};
    S8 szPtcName[PT_PTC_NAME_LEN] = {0};
    S8 szPtsIp[PT_IP_ADDR_SIZE] = {0};
    S8 szPtsPort[PT_DATA_BUFF_16] = {0};
    U8 paucIPAddr[IPV6_SIZE] = {0};
    PT_CTRL_DATA_ST stCtrlData;

    pstCtrlData = (PT_CTRL_DATA_ST *)(pData + sizeof(PT_MSG_TAG));
    switch (pstCtrlData->enCtrlType)
    {
        case PT_CTRL_LOGIN_RSP:
            /* 登陆验证 */
            ptc_get_udp_use_ip();  /* 获取本机ip */
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
            dos_strncpy(stVerRet.szVersion, ptc_get_version(), PT_VERSION_LEN);
            dos_strcpy(stVerRet.szPtsHistoryIp1, g_stServMsg.szPtsLasterIP1);
            dos_strcpy(stVerRet.szPtsHistoryIp2, g_stServMsg.szPtsLasterIP2);
            dos_strcpy(stVerRet.szPtsHistoryIp3, g_stServMsg.szPtsLasterIP3);
            stVerRet.usPtsHistoryPort1 = g_stServMsg.usPtsLasterPort1;
            stVerRet.usPtsHistoryPort2 = g_stServMsg.usPtsLasterPort2;
            stVerRet.usPtsHistoryPort3 = g_stServMsg.usPtsLasterPort3;
            dos_strncpy(stVerRet.szMac, g_stServMsg.szMac, PT_DATA_BUFF_64);
            dos_memcpy(acBuff, (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

            ptc_save_msg_into_cache(PT_DATA_CTRL, pstMsgDes->ulStreamID, acBuff, sizeof(PT_CTRL_DATA_ST));
            break;
        case PT_CTRL_LOGIN_ACK:
            /* 登陆结果 */
            if (pstCtrlData->ucLoginRes == DOS_TRUE)
            {
                /* 成功 */
                pt_logr_info("ptc login succ");
                g_bIsOnLine = DOS_TRUE;
                g_ulHDTimeoutCount = 0;
                g_ulConnectPtsCount = 0;
            }
            else
            {
                /* 失败 */
                pt_logr_info("login fail");
            }
            /* 通知pts，清除登录的缓存 */
            ptc_delete_send_stream_node(PT_CTRL_LOGIN_RSP, PT_DATA_CTRL, DOS_TRUE);
            ptc_send_exit_notify_to_pts(PT_DATA_CTRL, PT_CTRL_LOGIN_RSP);
            break;
        case PT_CTRL_HB_RSP:
            /* 心跳响应，关闭定时器 */
            pt_logr_debug("recv from pts hb rsp");
            if (g_stACKTmrHandle != NULL)
            {
                dos_tmr_stop(&g_stACKTmrHandle);
            }
            g_stACKTmrHandle = NULL;
            g_ulHDTimeoutCount = 0;
            break;
        case PT_CTRL_SWITCH_PTS:
            /* 切换pts */
            ptc_send_logout2pts(g_lUdpSocket);
            g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(pstMsgDes->aulServIp);
            g_pstPtcSend->stDestAddr.sin_port = pstMsgDes->usServPort;

            /* 写入文件,记录注册的pts的历史记录 */
            //if (*(U32 *)pstMsgDes->aulServIp != *(U32 *)g_stServMsg.achPtsMajorIP && *(U32 *)pstMsgDes->aulServIp != *(U32 *)g_stServMsg.achPtsMinorIP)
            ptc_update_pts_history_file(pstMsgDes->aulServIp, pstMsgDes->usServPort);
            ptc_send_login2pts(g_lUdpSocket);
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
                    lResult = pt_DNS_resolution(pstCtrlData->achPtsMajorDomain, paucIPAddr);
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
                    lResult = pt_DNS_resolution(pstCtrlData->achPtsMinorDomain, paucIPAddr);
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
    list_t             *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST    *pstStreamNode      = NULL;

    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
    #else
    pthread_mutex_lock(&g_mutex_send);
    #endif
    pstStreamListHead = g_pstPtcSend->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
    if (NULL == pstStreamListHead)
    {
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_send);
        #endif
        return DOS_FALSE;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
    if (NULL == pstStreamNode)
    {
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_send);
        #endif
        return DOS_FALSE;
    }

    if (pstStreamNode->lConfirmSeq < pstMsgDes->lSeq)
    {
        pstStreamNode->lConfirmSeq = pstMsgDes->lSeq;
        //sem_getvalue(&g_SemPtc, &i);
        //pt_logr_info("sem post : %ld", i);
        sem_post(&g_SemPtc);
    }
    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
    #else
    pthread_mutex_unlock(&g_mutex_send);
    #endif
    return DOS_TRUE;
}

/**
 * 函数：S8 *ptc_get_version()
 * 功能：
 *      1.获取版本号
 * 参数
 * 返回值：
 */
S8 *ptc_get_version()
{
    return PTC_VERSION;
}

/**
 * 函数：VOID ptc_send_heartbeat2pts(S32 lSockfd)
 * 功能：
 *      1.开启定时器，发送心跳请求
 * 参数
 * 返回值：
 */
VOID ptc_send_heartbeat2pts(S32 lSockfd)
{
    S32 lResult = 0;

    lResult = dos_tmr_start(&g_stHBTmrHandle, PTC_SEND_HB_TIME, ptc_send_hb_req, (U64)lSockfd, TIMER_NORMAL_LOOP);
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
VOID ptc_send_login2pts(S32 lSockfd)
{
    S32 lResult = 0;
    ptc_send_login_req((U64)lSockfd);
    lResult = dos_tmr_start(&g_stHBTmrHandle, PTC_SEND_HB_TIME, ptc_send_login_req, (U64)lSockfd, TIMER_NORMAL_LOOP);
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
VOID ptc_send_logout2pts(S32 lSockfd)
{
    g_bIsOnLine = DOS_FALSE;
    dos_tmr_stop(&g_stHBTmrHandle);
    ptc_send_logout_req((U64)lSockfd);
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
VOID ptc_save_msg_into_cache(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen)
{
    S32 lResult = 0;
    PT_MSG_TAG stMsgDes;
    struct timespec stSemTime;
    struct timeval now;
    if (enDataType < 0 || enDataType >= PT_DATA_BUTT)
    {
        pt_logr_debug("ptc_save_msg_into_cache : enDataType should in 0-%d: %d", PT_DATA_BUTT, enDataType);
        return;
    }
    else if (NULL == pcData)
    {
        pt_logr_debug("ptc_save_msg_into_cache : send data is NULL");
        return;
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
        return;
    }
    else
    {
        /* 添加到发送消息队列 */
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
        #else
        pthread_mutex_lock(&g_mutex_send);
        #endif
        if (NULL == pt_need_send_node_list_search(g_pstPtcNendSendNode, ulStreamID))
        {
            g_pstPtcNendSendNode = pt_need_send_node_list_insert(g_pstPtcNendSendNode, g_pstPtcSend->aucID, &stMsgDes, enCmdValue, bIsResend);
        }
        pthread_cond_signal(&g_cond_send);
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_send);
        #endif
    }

    if (PT_NEED_CUT_PTHREAD == lResult)
    {
        gettimeofday(&now, NULL);
        stSemTime.tv_sec = now.tv_sec + PTC_WAIT_CONFIRM_TIMEOUT;
        stSemTime.tv_nsec = now.tv_usec * 1000;
        pt_logr_debug("wait for confirm msg");
        // sem_getvalue(&g_SemPtc, &i);
        sem_timedwait(&g_SemPtc, &stSemTime);
    }

    usleep(10); /* 保证切换线程 */
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
    S32              lSockfd          = *(S32 *)arg;
    U32              ulArraySub       = 0;
    U32              ulSendCount      = 0;
    list_t           *pstStreamHead   = NULL;
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
    sem_init (&g_SemPtc, 0, 0);  /* 初始化信号量 */

    while (1)
    {
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = (now.tv_usec) * 1000;

        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
        #else
        pthread_mutex_lock(&g_mutex_send);
        #endif
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_cond_timedwait(&timeout, __FILE__, __LINE__); /* 必须使用这个的函数 */
        #else
        pthread_cond_timedwait(&g_cond_send, &g_mutex_send, &timeout);
        #endif

        if (NULL == g_pstPtcSend)
        {
            pt_logr_info("ptc send cache not init");
            continue;
        }

        pstNendSendList = g_pstPtcNendSendNode;
        DOS_LOOP_DETECT_INIT(lLoopMaxCount, DOS_DEFAULT_MAX_LOOP);

        while(1)
        {
            /* 防止死循环 */
            DOS_LOOP_DETECT(lLoopMaxCount)

            if (NULL == pstNendSendList)
            {
                break;
            }
            pstNeedSendNode = list_entry(pstNendSendList, PT_NEND_SEND_NODE_ST, stListNode);
            if (pstNendSendList == pstNendSendList->next)
            {
                pstNendSendList = NULL;
            }
            else
            {
                pstNendSendList = pstNendSendList->next;
                list_del(&pstNeedSendNode->stListNode);
            }

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
                sendto(lSockfd, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG), 0,  (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));

                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }
            /* 发送数据包 */
            pstStreamHead = g_pstPtcSend->astDataTypes[pstNeedSendNode->enDataType].pstStreamQueHead;
            if (NULL == pstStreamHead)
            {
                pt_logr_info("ptc_send_msg2pts : stream list head is NULL, cann't get data package");
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
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
                        lResult = sendto(lSockfd, acBuff, stSendDataNode.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
                        ulSendCount--;
                        pt_logr_info("ptc_send_msg2pts : send resend data seq : %d, stream, result = %d", pstNeedSendNode->lSeqResend, pstNeedSendNode->ulStreamID, lResult);

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

                    //usleep(g_ulSendTimeSleep);
                    lResult = sendto(lSockfd, acBuff, stSendDataNode.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
                    pt_logr_info("ptc_send_msg2pts : send data  length:%d, seq:%d, stream:%d, result:%d", stRecvDataTcp.ulLen, pstStreamNode->lCurrSeq, pstNeedSendNode->ulStreamID, lResult);
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

        g_pstPtcNendSendNode = pstNendSendList;
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_send);
        #endif
    }
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
    S32         lSockfd    = *(S32*)arg;
    S32         lRecvLen   = 0;
    S32         lResult    = 0;
    U32         MaxFdp     = lSockfd;
    PT_MSG_TAG *pstMsgDes  = NULL;
    struct sockaddr_in stClientAddr = g_pstPtcSend->stDestAddr;
    socklen_t          ulCliaddrLen = sizeof(stClientAddr);
    fd_set             ReadFdsCpio;
    fd_set             ReadFds;
    struct timeval stTimeVal = {1, 0};
    struct timeval stTimeValCpy;
    FD_ZERO(&ReadFds);
    FD_SET(lSockfd, &ReadFds);
    S8 acRecvBuf[PT_SEND_DATA_SIZE]  = {0};
    struct timeval now;
    sem_init(&g_SemPtcRecv, 0, 1);  /* 初始化信号量 */
    g_lUdpSocket = lSockfd;

    while (1)
    {
        stTimeValCpy = stTimeVal;
        ReadFdsCpio = ReadFds;
        lResult = select(MaxFdp + 1, &ReadFdsCpio, NULL, NULL, &stTimeValCpy);
        if (lResult < 0)
        {
            perror("ptc_recv_msg_from_pts : fail to select");
            exit(DOS_FAIL);
        }
        else if (0 == lResult)
        {
            continue;
        }

        if (FD_ISSET(lSockfd, &ReadFdsCpio))
        {
            lRecvLen = recvfrom(lSockfd, acRecvBuf, PT_SEND_DATA_SIZE, 0, (struct sockaddr*)&stClientAddr, &ulCliaddrLen);
            if (lRecvLen < 0)
            {
                perror("recvfrom");
                continue;
            }
            pt_logr_debug("ptc recv msg from pts : data size:%d", lRecvLen);
            sem_wait(&g_SemPtcRecv);
            #if PT_MUTEX_DEBUG
            ptc_recv_pthread_mutex_lock(__FILE__, __LINE__);
            #else
            pthread_mutex_lock(&g_mutex_recv);
            #endif
            /* 取出消息头部数据 */
            pstMsgDes = (PT_MSG_TAG *)acRecvBuf;
            /* 字节序转换 */
            pstMsgDes->ulStreamID = dos_ntohl(pstMsgDes->ulStreamID);
            pstMsgDes->lSeq = dos_ntohl(pstMsgDes->lSeq);
            if (pstMsgDes->enCmdValue == PT_CMD_RESEND)
            {
                /* 重传请求 */
                #if PT_MUTEX_DEBUG
                ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
                #else
                pthread_mutex_lock(&g_mutex_send);
                #endif
                pt_logr_info("ptc recv resend req, seq : %d, stream : %d", pstMsgDes->lSeq, pstMsgDes->ulStreamID);

                BOOL bIsResend = DOS_TRUE;
                PT_CMD_EN enCmdValue = PT_CMD_NORMAL;

                g_pstPtcNendSendNode = pt_need_send_node_list_insert(g_pstPtcNendSendNode, g_pstPtcSend->aucID, pstMsgDes, enCmdValue, bIsResend);
                pthread_cond_signal(&g_cond_send);
                #if PT_MUTEX_DEBUG
                ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_send);
                #endif
                #if PT_MUTEX_DEBUG
                ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_recv);
                #endif
                sem_post(&g_SemPtcRecv);
                continue;
            }
            else if (pstMsgDes->enCmdValue == PT_CMD_CONFIRM)
            {
                /* 确认接收 */
                gettimeofday(&now, NULL);
                pt_logr_info("make sure recv seq : %d", pstMsgDes->lSeq);
                ptc_deal_with_confirm_msg(pstMsgDes);
                #if PT_MUTEX_DEBUG
                ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_recv);
                #endif
                sem_post(&g_SemPtcRecv);
                continue;
            }
            else if (pstMsgDes->ExitNotifyFlag == DOS_TRUE)
            {
                /* 响应结束，清除streamID节点 */
                ptc_delete_send_stream_node(pstMsgDes->ulStreamID, pstMsgDes->enDataType, DOS_TRUE);
                ptc_delete_recv_stream_node(pstMsgDes->ulStreamID, pstMsgDes->enDataType, DOS_FALSE);
#if PT_MUTEX_DEBUG
                ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
#else
                pthread_mutex_unlock(&g_mutex_recv);
#endif
                sem_post(&g_SemPtcRecv);
                continue;
            }

            if (pstMsgDes->enDataType == PT_DATA_CTRL)
            {
                /* 控制消息 */
                ptc_ctrl_msg_handle(pstMsgDes, acRecvBuf);
                #if PT_MUTEX_DEBUG
                ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_recv);
                #endif
                sem_post(&g_SemPtcRecv);
                continue;
            }

            lResult = ptc_save_into_recv_cache(g_pstPtcRecv, acRecvBuf, lRecvLen, lSockfd);
            if (lResult < 0)
            {
                pt_logr_error("ptc add data into recv cache fail, result : %d", lResult);
                #if PT_MUTEX_DEBUG
                ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_recv);
                #endif
                sem_post(&g_SemPtcRecv);
                continue;
            }

            pt_logr_debug("ptc recv from pts length:%d, seq:%d, stream:%d", lRecvLen, pstMsgDes->lSeq, pstMsgDes->ulStreamID);
            if (NULL == pt_need_recv_node_list_search(g_pstPtcNendRecvNode, pstMsgDes->ulStreamID))
            {
                g_pstPtcNendRecvNode = pt_need_recv_node_list_insert(g_pstPtcNendRecvNode, pstMsgDes);
            }
            pthread_cond_signal(&g_cond_recv);

#if PT_MUTEX_DEBUG
            ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
#else
            pthread_mutex_unlock(&g_mutex_recv);
#endif
            if (lResult == PT_NEED_CUT_PTHREAD)
            {
                usleep(10); /* 挂起线程，执行接收函数 */
            }
        }
    }
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */
