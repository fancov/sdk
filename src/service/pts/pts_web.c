#ifdef  __cplusplus
extern "C" {
#endif

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
#include <sys/time.h>
#include <semaphore.h>
#include <netinet/tcp.h>

#include <dos.h>
#include <pt/dos_sqlite3.h>
#include <pt/pts.h>
#include "pts_msg.h"
#include "pts_web.h"

#define PTS_VISIT_WEB_ERROE_1 "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE html><HTML><HEAD><TITLE>Error</TITLE><META http-equiv=Content-Type content=\"text/html; charset=gb2312\"><STYLE type=text/css>BODY {BACKGROUND: #fff; MARGIN: 80px auto; FONT: 14px/150% Verdana, Georgia, Sans-Serif; COLOR: #000; TEXT-ALIGN: center}H1 {PADDING-RIGHT: 4px; PADDING-LEFT: 4px; FONT-SIZE: 14px; BACKGROUND: #eee; PADDING-BOTTOM: 4px; MARGIN: 0px; PADDING-TOP: 4px; BORDER-BOTTOM: #84b0c7 1px solid} DIV{BORDER-RIGHT: #84b0c7 1px solid; BORDER-TOP: #84b0c7 1px solid; BACKGROUND: #e5eef5; MARGIN: 0px auto; BORDER-LEFT: #84b0c7 1px solid; WIDTH: 500px; BORDER-BOTTOM: #84b0c7 1px solid}</STYLE></HEAD><BODY><DIV><H1>提示：您访问的地址无法建立连接</H1></DIV></BODY></HTML>"
#define PTS_VISIT_WEB_ERROE_2 "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE html><HTML><HEAD><TITLE>Error</TITLE><META http-equiv=Content-Type content=\"text/html; charset=gb2312\"><STYLE type=text/css>BODY {BACKGROUND: #fff; MARGIN: 80px auto; FONT: 14px/150% Verdana, Georgia, Sans-Serif; COLOR: #000; TEXT-ALIGN: center}H1 {PADDING-RIGHT: 4px; PADDING-LEFT: 4px; FONT-SIZE: 14px; BACKGROUND: #eee; PADDING-BOTTOM: 4px; MARGIN: 0px; PADDING-TOP: 4px; BORDER-BOTTOM: #84b0c7 1px solid} DIV{BORDER-RIGHT: #84b0c7 1px solid; BORDER-TOP: #84b0c7 1px solid; BACKGROUND: #e5eef5; MARGIN: 0px auto; BORDER-LEFT: #84b0c7 1px solid; WIDTH: 500px; BORDER-BOTTOM: #84b0c7 1px solid}</STYLE></HEAD><BODY><DIV><H1>提示：用该PTC的连接过多，请稍后再试</H1></DIV></BODY></HTML>"
#define PTS_VISIT_WEB_ERROE_3 "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE html><HTML><HEAD><TITLE>Error</TITLE><META http-equiv=Content-Type content=\"text/html; charset=gb2312\"><STYLE type=text/css>BODY {BACKGROUND: #fff; MARGIN: 80px auto; FONT: 14px/150% Verdana, Georgia, Sans-Serif; COLOR: #000; TEXT-ALIGN: center}H1 {PADDING-RIGHT: 4px; PADDING-LEFT: 4px; FONT-SIZE: 14px; BACKGROUND: #eee; PADDING-BOTTOM: 4px; MARGIN: 0px; PADDING-TOP: 4px; BORDER-BOTTOM: #84b0c7 1px solid} DIV{BORDER-RIGHT: #84b0c7 1px solid; BORDER-TOP: #84b0c7 1px solid; BACKGROUND: #e5eef5; MARGIN: 0px auto; BORDER-LEFT: #84b0c7 1px solid; WIDTH: 500px; BORDER-BOTTOM: #84b0c7 1px solid}</STYLE></HEAD><BODY><DIV><H1>提示：超时</H1></DIV></BODY></HTML>"

list_t  m_stClinetCBList; /* 客户端请求链表 */
pthread_mutex_t g_mutexWebClient  = PTHREAD_MUTEX_INITIALIZER;   /* 保护 m_stClinetCBList 的信号量 */
S32 m_lPtcSeq = 0;
S32 g_lWebServSocket = 0;
const S8 *g_szPtsFifoName = "/tmp/pts_fifo";
S32 g_lPtsPipeWRFd = -1;

VOID pts_web_free_stream(PT_NEND_RECV_NODE_ST *pstNeedRecvNode, PT_STREAM_CB_ST *pstStreamNode, PT_CC_CB_ST *pstCCNode)
{
    PT_MSG_TAG stMsgDes;

    if (DOS_ADDR_INVALID(pstNeedRecvNode) || DOS_ADDR_INVALID(pstStreamNode) || DOS_ADDR_INVALID(pstCCNode))
    {
        return;
    }

    dos_memcpy(stMsgDes.aucID, pstNeedRecvNode->aucID, PTC_ID_LEN);
    stMsgDes.enDataType = pstNeedRecvNode->enDataType;
    stMsgDes.ulStreamID = pstNeedRecvNode->ulStreamID;
    /* 向ptc发送结束通知 */
    pts_send_exit_notify2ptc(pstCCNode, pstNeedRecvNode);
    /* 释放stream节点 */
    pts_free_stream_resource(&stMsgDes);
}


VOID pts_web_close_socket(S32 lSocket)
{
    pthread_mutex_lock(&g_mutexWebClient);
    pts_clinetCB_delete_by_sockfd(&m_stClinetCBList, lSocket);
    pthread_mutex_unlock(&g_mutexWebClient);

    close(lSocket);
}

VOID pts_web_close_socket_without_sem(S32 lSocket)
{
    pts_clinetCB_delete_by_sockfd(&m_stClinetCBList, lSocket);
    close(lSocket);
}

VOID pts_web_free_resource(S32 lSocket)
{
    PT_CC_CB_ST             *pstCCNode          = NULL;
    DLL_NODE_S              *pstListNode        = NULL;
    STREAM_CACHE_ADDR_CB_ST *pstStreamCacheAddr = NULL;
    PT_NEND_RECV_NODE_ST stNeedRecvNode;
    PT_MSG_TAG stMsgDes;
    PTS_CLIENT_CB_ST stClientMsg;
    PTS_CLIENT_CB_ST *pstClient = NULL;

    pthread_mutex_lock(&g_mutexWebClient);
    pstClient = pts_clinetCB_search_by_sockfd(&m_stClinetCBList, lSocket);
    if (DOS_ADDR_INVALID(pstClient))
    {
        pthread_mutex_unlock(&g_mutexWebClient);

        return;
    }
    stClientMsg = *pstClient;
    pts_clinetCB_delete_node(pstClient);
    pthread_mutex_unlock(&g_mutexWebClient);

    dos_memcpy(stNeedRecvNode.aucID, stClientMsg.aucID, PTC_ID_LEN);
    stNeedRecvNode.enDataType = PT_DATA_WEB;
    stNeedRecvNode.ulStreamID = stClientMsg.ulStreamID;
    dos_memcpy(stMsgDes.aucID, stClientMsg.aucID, PTC_ID_LEN);
    stMsgDes.enDataType = PT_DATA_WEB;
    stMsgDes.ulStreamID = stClientMsg.ulStreamID;
    /* 向ptc发送结束通知 */
    pthread_mutex_lock(&g_mutexStreamAddrList);
    pstListNode = dll_find(&g_stStreamAddrList, (VOID *)&stClientMsg.ulStreamID, pts_find_stream_addr_by_streamID);
    if (DOS_ADDR_VALID(pstListNode))
    {
        pstStreamCacheAddr = pstListNode->pHandle;
        if (DOS_ADDR_VALID(pstStreamCacheAddr))
        {
            pstCCNode = pstStreamCacheAddr->pstPtcSendNode;
            if (DOS_ADDR_VALID(pstCCNode))
            {
                pts_send_exit_notify2ptc(pstCCNode, &stNeedRecvNode);
            }
            dos_dmem_free(pstStreamCacheAddr);
            pstStreamCacheAddr = NULL;
        }

        dll_delete(&g_stStreamAddrList, pstListNode);
        dos_dmem_free(pstListNode);
        pstListNode= NULL;
    }
    pthread_mutex_unlock(&g_mutexStreamAddrList);

    /* 释放收发缓存 */
    pts_free_stream_resource(&stMsgDes);
}

void pts_set_cache_full_true(S32 lSocket)
{
    PTS_CLIENT_CB_ST *pstClient = NULL;

    pthread_mutex_lock(&g_mutexWebClient);
    pstClient = pts_clinetCB_search_by_sockfd(&m_stClinetCBList, lSocket);
    if (pstClient == NULL)
    {
        pthread_mutex_unlock(&g_mutexWebClient);

        return;
    }

    pstClient->bIsCacheFull = DOS_TRUE;
    pthread_mutex_unlock(&g_mutexWebClient);
}

void pts_set_cache_full_false(U32 ulStreamID)
{
    PTS_CLIENT_CB_ST *pstClient = NULL;

    pthread_mutex_lock(&g_mutexWebClient);
    pstClient = pts_clinetCB_search_by_stream(&m_stClinetCBList, ulStreamID);
    if (pstClient == NULL)
    {
        pthread_mutex_unlock(&g_mutexWebClient);

        return;
    }

    pstClient->bIsCacheFull = DOS_FALSE;

    pthread_mutex_unlock(&g_mutexWebClient);
}

/**
 * 函数：list_t *pts_clinetCB_insert(list_t *psthead, S32 lSockfd, struct sockaddr_in stClientAddr)
 * 功能：插入新的请求道请求链表
 * 参数
 * 返回值:
 */
S32 pts_clinetCB_insert(list_t *psthead, S32 lSockfd, struct sockaddr_in stClientAddr)
{
    PTS_CLIENT_CB_ST *stNewNode = NULL;

    stNewNode = (PTS_CLIENT_CB_ST *)dos_dmem_alloc(sizeof(PTS_CLIENT_CB_ST));
    if (DOS_ADDR_INVALID(stNewNode))
    {
        perror("dos_dmem_malloc");
        return DOS_FAIL;
    }

    stNewNode->lSocket = lSockfd;
    stNewNode->ulStreamID = pts_create_stream_id();
    stNewNode->stClientAddr = stClientAddr;
    stNewNode->eSaveHeadFlag = DOS_FALSE;
    stNewNode->bIsCacheFull = DOS_FALSE;
    pt_logr_debug("sockfd : %d, stream : %d", lSockfd, stNewNode->ulStreamID);

    dos_list_node_init(&(stNewNode->stList));
    dos_list_add_tail(psthead, &(stNewNode->stList));

    return DOS_SUCC;
}

/**
 * 函数：PTS_CLIENT_CB_ST* pts_clinetCB_search_by_sockfd(list_t *pstHead, S32 lSockfd)
 * 功能：查找请求链表中的节点，通过lSockfd
 * 参数
 * 返回值:
 */
PTS_CLIENT_CB_ST* pts_clinetCB_search_by_sockfd(list_t *pstHead, S32 lSockfd)
{
    list_t *pstNode = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    if (DOS_ADDR_INVALID(pstHead))
    {
        pt_logr_debug("PTS_CLIENT_CB_ST search : empty list");
        return NULL;
    }

    if (dos_list_is_empty(pstHead))
    {
        return NULL;
    }

    pstNode = pstHead;

    while (1)
    {
        pstNode = dos_list_work(pstHead, pstNode);
        if (NULL == pstNode)
        {
            return NULL;
        }

        pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
        if (pstData->lSocket == lSockfd)
        {
            return pstData;
        }
    }
}

/**
 * 函数：PTS_CLIENT_CB_ST* pts_clinetCB_search_by_stream(list_t* pstHead, U32 ulStreamID)
 * 功能：查找请求链表中的节点，通过ulStreamID
 * 参数
 * 返回值:
 */
PTS_CLIENT_CB_ST* pts_clinetCB_search_by_stream(list_t* pstHead, U32 ulStreamID)
{
    list_t *pstNode = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    if (DOS_ADDR_INVALID(pstHead))
    {
        pt_logr_debug("PTS_CLIENT_CB_ST search : empty list");
        return NULL;
    }

    if (dos_list_is_empty(pstHead))
    {
        return NULL;
    }

    pstNode = pstHead;

    while (1)
    {
        pstNode = dos_list_work(pstHead, pstNode);
        if (NULL == pstNode)
        {
            return NULL;
        }

        pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
        if (pstData->ulStreamID == ulStreamID)
        {
            return pstData;
        }
    }
}

S32 pts_clinetCB_delete_node(PTS_CLIENT_CB_ST* pstNode)
{
    if (DOS_ADDR_INVALID(pstNode))
    {
        return DOS_FAIL;
    }

    dos_list_del(&pstNode->stList);
    pstNode->lSocket = -1;
    dos_dmem_free(pstNode);

    return DOS_SUCC;
}

/**
 * 函数：list_t *pts_clinetCB_delete_by_sockfd(list_t* pstHead, S32 lSockfd)
 * 功能：删除请求链表中的节点，通过lSockfd
 * 参数
 * 返回值:
 */
S32 pts_clinetCB_delete_by_sockfd(list_t* pstHead, S32 lSockfd)
{
    list_t *pstNode = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    if (DOS_ADDR_INVALID(pstHead))
    {
        pt_logr_debug("PTS_CLIENT_CB_ST delete : empty list");
        return DOS_FAIL;
    }

    if (dos_list_is_empty(pstHead))
    {
        return DOS_FAIL;
    }

    pstNode = pstHead;

    while (1)
    {
        pstNode = dos_list_work(pstHead, pstNode);
        if (NULL == pstNode)
        {
            return DOS_FAIL;
        }

        pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
        if (pstData->lSocket == lSockfd)
        {
            dos_list_del(pstNode);
            pstData->lSocket = -1;
            dos_dmem_free(pstData);
            pstData = NULL;

            return DOS_SUCC;
        }
    }
}

/**
 * 函数：list_t *pts_clinetCB_delete_by_stream(list_t* pstHead, U32 ulStreamID)
 * 功能：删除请求链表中的节点，通过ulStreamID
 * 参数
 * 返回值:
 */
S32 pts_clinetCB_delete_by_stream(list_t* pstHead, U32 ulStreamID)
{
    list_t *pstNode = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    if (DOS_ADDR_INVALID(pstHead))
    {
        pt_logr_debug("PTS_CLIENT_CB_ST delete : empty list");
        return DOS_FAIL;
    }

    if (dos_list_is_empty(pstHead))
    {
        return DOS_FAIL;
    }

    pstNode = pstHead;

    while (1)
    {
        pstNode = dos_list_work(pstHead, pstNode);
        if (NULL == pstNode)
        {
            return DOS_FAIL;
        }

        pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
        if (pstData->ulStreamID == ulStreamID)
        {
            dos_list_del(pstNode);
            pstData->lSocket = -1;
            dos_dmem_free(pstData);
            pstData = NULL;

            return DOS_SUCC;
        }
    }
}

/**
 * 函数：BOOL pts_is_http_head(S8 *pcRequest)
 * 功能：判断是否是http请求头
 * 参数
 * 返回值:
 */
BOOL pts_is_http_head(S8 *pcRequest)
{
    if (NULL == pcRequest)
    {
        return DOS_FALSE;
    }

    if (NULL != dos_strstr(pcRequest,"HTTP/1.1"))
    {
        return DOS_TRUE;
    }
    return DOS_FALSE;
}

/**
 * 函数：BOOL pts_is_http_end(S8 *pcRequest)
 * 功能：判断http请求头是否结束
 * 参数
 * 返回值:
 */
BOOL pts_is_http_end(S8 *pcRequest)
{
    if (NULL == pcRequest)
    {
        return DOS_FALSE;
    }

    if (NULL != dos_strstr(pcRequest,"\r\n\r\n"))
    {
        return DOS_TRUE;
    }
    return DOS_FALSE;
}

/**
 * 函数：BOOL pts_request_ptc_proxy(S8 *pcRequest, U32 ulConnfd, U32 ulStreamID, U8* pcIpccId, S32 lReqLen, S8 *szUrl)
 * 功能：发送请求道 ptc proxy
 * 参数
 * 返回值:
 */
BOOL pts_request_ptc_proxy(S8 *pcRequest, U32 ulConnfd, U32 ulStreamID, U8* pcIpccId, S32 lReqLen, S8 *szUrl)
{
    BOOL bIsGetID = DOS_FALSE;
    U16 usDestPort = 0;
    U32 ulSendBufSize = 0;
    S8 *pStr1 = NULL;
    S8 *pcCookie = NULL;
    S8 szDestIP[PT_IP_ADDR_SIZE] = {0};
    U8 aucDestID[PTC_ID_LEN+1] = {0};
    S8 szDestPortStr[PT_DATA_BUFF_16] = {0};
    S8 szCookieId[PT_DATA_BUFF_16] = {0};
    S8 szServPort[PT_DATA_BUFF_16] = {0};
    /* 获取请求的端口 */
    pStr1 = dos_strstr(pcRequest, "Host:");
    if (NULL == pStr1)
    {
        pts_web_close_socket(ulConnfd);
        return bIsGetID;
    }
    sscanf(pStr1, "%*[^:]:%*[^:]:%[0-9]", szServPort);

    snprintf(szCookieId, PT_DATA_BUFF_16, "ptsId_%s", szServPort);

    pcCookie = dos_strstr(pcRequest, szCookieId);
    if (pcCookie != NULL)
    {
        sscanf(pcCookie, "%*[^=]=%[^!]!%[^!]!%[0-9]", aucDestID, szDestIP, szDestPortStr);
        usDestPort = atoi(szDestPortStr);
        pt_logr_debug("aucDestID = %s, szDestIP = %s, szDestPortStr = %s", aucDestID, szDestIP, szDestPortStr);
        pStr1 = pcRequest;
        ulSendBufSize = lReqLen;
        pt_logr_debug("pts send web server filename : %s, sockfd : %d, stream : %d", szUrl, ulConnfd, ulStreamID);

        while (1)
        {
            if (ulSendBufSize <= PT_RECV_DATA_SIZE)
            {
                pts_save_msg_into_cache(aucDestID, PT_DATA_WEB, ulStreamID, pStr1, ulSendBufSize, szDestIP, usDestPort);

                break;
            }
            else
            {
                pts_save_msg_into_cache(aucDestID, PT_DATA_WEB, ulStreamID, pStr1, PT_RECV_DATA_SIZE, szDestIP, usDestPort);
                ulSendBufSize -= PT_RECV_DATA_SIZE;
                pStr1 += PT_RECV_DATA_SIZE;
            }
        }
        dos_memcpy(pcIpccId, aucDestID, PTC_ID_LEN);
        bIsGetID = DOS_TRUE;
    }
    else
    {
        pt_logr_info("not get cookie");
        pts_web_close_socket(ulConnfd);
    }

    return bIsGetID;
}

/**
 * 函数：BOOL pts_deal_with_http_head(S8 *pcRequest, U32 ulConnfd, U32 ulStreamID, U8* pcIpccId, S32 lReqLen)
 * 功能：
 *      1.处理web浏览器请求
 * 参数
 *      S8 *pcRequest :  请求报文
 *      U32 ulConnfd:    套接字
 *      U32 ulStreamID:  stream ID
 *      U8* pcIpccId:    获取设备的ID
 *      S32 lReqLen:     报文的大小
 * 返回值：BOOL:   是否获得报文的大小
 */
BOOL pts_deal_with_http_head(S8 *pcRequest, U32 ulConnfd, U32 ulStreamID, U8* pcIpccId, S32 lReqLen)
{
    if (NULL == pcRequest || NULL == pcIpccId)
    {
        pt_logr_debug("pts_deal_with_http_head : param null");
        return DOS_FALSE;
    }

    S8 szUrl[PT_DATA_BUFF_128] = {0};
    BOOL bIsGetID = DOS_FALSE;

    pt_logr_debug("send req socket : %d, ulStreamID : %d", ulConnfd, ulStreamID);

    bIsGetID = pts_request_ptc_proxy(pcRequest, ulConnfd, ulStreamID, pcIpccId, lReqLen, szUrl);

    return bIsGetID;
}

/**
 * 函数：VOID pts_deal_with_web_request(S8 *pcRequest, U32 ulConnfd, S32 lReqLen)
 * 功能：处理浏览器请求
 * 参数
 * 返回值:
 */
BOOL pts_deal_with_web_request(S8 *pcRequest, U32 ulConnfd, S32 lReqLen)
{
    S32 lResult = 0;
    BOOL bIsCacheFull = DOS_FALSE;
    PTS_CLIENT_CB_ST* pstHttpHead   = NULL;
    S8 *pcHeadBuf = NULL;                //PT_SEND_DATA_SIZE
    BOOL bIsGetID = DOS_FALSE;
    U8 *pcIpccId = (U8 *)dos_dmem_alloc(PTC_ID_LEN);
    if (NULL == pcIpccId)
    {
        perror("dos_dmem_malloc");
        /* TODO */
        return DOS_FALSE;
    }

    dos_memzero(pcIpccId, PTC_ID_LEN);
    pstHttpHead = pts_clinetCB_search_by_sockfd(&m_stClinetCBList, ulConnfd);
    if (NULL == pstHttpHead)
    {
        return DOS_FALSE;
    }

    if (!pstHttpHead->eSaveHeadFlag)
    {
        if (pts_is_http_head(pcRequest))
        {
            if (pts_is_http_end(pcRequest))
            {
                bIsGetID = pts_deal_with_http_head(pcRequest, ulConnfd, pstHttpHead->ulStreamID, pcIpccId, lReqLen);
                if (bIsGetID)
                {
                    dos_memcpy(pstHttpHead->aucID, pcIpccId, PTC_ID_LEN);
                }
            }
            else
            {
                pstHttpHead->pcRequestHead = (S8 *)dos_dmem_alloc(lReqLen);
                if (NULL == pstHttpHead->pcRequestHead)
                {
                    perror("dos_dmem_malloc");
                    return DOS_FALSE;
                }
                else
                {
                    dos_memcpy(pstHttpHead->pcRequestHead, pcRequest, lReqLen);
                    pstHttpHead->lReqLen = lReqLen;
                    pstHttpHead->eSaveHeadFlag = DOS_TRUE;
                }
            }
        }
        else
        {
            lResult = pts_save_msg_into_cache(pstHttpHead->aucID, PT_DATA_WEB, pstHttpHead->ulStreamID, pcRequest, lReqLen, NULL, 0);
            if (PT_NEED_CUT_PTHREAD == lResult)
            {
                bIsCacheFull = DOS_TRUE;
            }
        }
    }
    else
    {
        pcHeadBuf = (S8 *)dos_dmem_alloc(pstHttpHead->lReqLen + lReqLen);
        if (NULL == pcHeadBuf)
        {
            perror("malloc");
            return DOS_FALSE;
        }

        dos_memcpy(pcHeadBuf, pstHttpHead->pcRequestHead, pstHttpHead->lReqLen);
        dos_memcpy(pcHeadBuf+pstHttpHead->lReqLen, pcRequest, lReqLen);

        if (pts_is_http_end(pcHeadBuf))
        {

            bIsGetID = pts_deal_with_http_head(pcHeadBuf, ulConnfd, pstHttpHead->ulStreamID, pcIpccId, pstHttpHead->lReqLen+lReqLen);
            if (bIsGetID)
            {
                dos_memcpy(pstHttpHead->aucID, pcIpccId, PTC_ID_LEN);
            }
            dos_dmem_free(pstHttpHead->pcRequestHead);
            pstHttpHead->pcRequestHead = NULL;
            pstHttpHead->eSaveHeadFlag = DOS_FALSE;
            dos_dmem_free(pcHeadBuf);
            pcHeadBuf = NULL;
        }
        else
        {
            dos_dmem_free(pstHttpHead->pcRequestHead);
            pstHttpHead->pcRequestHead = pcHeadBuf;
            pstHttpHead->lReqLen += lReqLen;
        }

    }
    dos_dmem_free(pcIpccId);
    pcIpccId = NULL;

    return bIsCacheFull;
}


VOID pts_web_timeout_callback(U64 arg)
{
    U32 ulArraySub = (U32)arg;

    g_lPtsServSocket[ulArraySub].hTmrHandle = NULL;
    close(g_lPtsServSocket[ulArraySub].lSocket);
    g_lPtsServSocket[ulArraySub].lSocket = -1;
    pt_logr_debug("timer timeout port : %d", g_lPtsServSocket[ulArraySub].usServPort);
}

VOID pts_create_web_serv_socket(U32 ulArraySeq)
{
    U16 usPort = g_lWebServSocket;
    S32 lSocket = 0;
    U32 lResult = 0;

    while(1)
    {
        lSocket = pts_create_tcp_socket(usPort);
        if (lSocket <= 0)
        {
            usPort++;
            if (usPort > g_stPtsMsg.usWebServPort + 1000)
            {
                usPort = g_stPtsMsg.usWebServPort;
            }
            continue;
        }
        break;
    }

    if (usPort == g_stPtsMsg.usWebServPort + 1000)
    {
        g_lWebServSocket = g_stPtsMsg.usWebServPort;
    }
    else
    {
        g_lWebServSocket = usPort + 1;
    }

    lResult = write(g_lPtsPipeWRFd, "s", 1);
    g_lPtsServSocket[ulArraySeq].lSocket = lSocket;
    g_lPtsServSocket[ulArraySeq].usServPort = usPort;
    /* 开启定时器 */
    lResult = dos_tmr_start(&g_lPtsServSocket[ulArraySeq].hTmrHandle, PTS_WEB_TIMEOUT, pts_web_timeout_callback, ulArraySeq, TIMER_NORMAL_NO_LOOP);
    if (lResult < 0)
    {
        pt_logr_info("ptc_send_hb_req : start timer fail");
    }

    return;
}


S32 pts_is_serv_socket(S32 lSocket)
{
    S32 i = 0;
    S32 lResult = 0;

    for (i=0; i<PTS_WEB_SERVER_MAX_SIZE; i++)
    {
        if (g_lPtsServSocket[i].lSocket == lSocket)
        {
            if (g_lPtsServSocket[i].hTmrHandle != NULL)
            {
                dos_tmr_stop(&g_lPtsServSocket[i].hTmrHandle);
                g_lPtsServSocket[i].hTmrHandle = NULL;
            }
            lResult = dos_tmr_start(&g_lPtsServSocket[i].hTmrHandle, PTS_WEB_TIMEOUT, pts_web_timeout_callback, i, TIMER_NORMAL_NO_LOOP);
            if (lResult < 0)
            {
                pt_logr_info("ptc_send_hb_req : start timer fail");
            }
            return DOS_TRUE;
        }
    }

    return DOS_FALSE;
}

/**
 * 函数：VOID *pts_recv_msg_from_web(VOID *arg)
 * 功能：线程 获得浏览器请求
 * 参数
 * 返回值:
 */
VOID *pts_recv_msg_from_browser(VOID *arg)
{
    S32 lConnFd = 0;
    S32 lMaxFdp = 0;
    S32 i       = 0;
    S32 lRecvLen  = 0;
    S32 lResult = 0;
    BOOL bIsCacheFull = DOS_FALSE;
    fd_set ReadFds;
    S8  szRecvBuf[PT_RECV_DATA_SIZE] = {0};
    struct sockaddr_in stClientAddr;
    socklen_t ulCliaddrLen = sizeof(struct sockaddr_in);
    struct timeval stTimeval = {1, 0};
    struct timeval stTvCopy;
    S32 lPipeFd = -1;
    S32 lError  = 0;
    U32 ulSocketCache = PTS_SOCKET_CACHE;
    list_t *pstNode = NULL;
    list_t *pstHead = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    dos_list_init(&m_stClinetCBList);

    g_lWebServSocket = g_stPtsMsg.usWebServPort;

    /* 创建管道 */
    if(access(g_szPtsFifoName, F_OK) == -1)
    {
        /* 管道文件不存在,创建命名管道 */
        lResult = mkfifo(g_szPtsFifoName, 0777);
        if(lResult != 0)
        {
            logr_error("Could not create fifo %s\n", g_szPtsFifoName);
            perror("mkfifo");
            DOS_ASSERT(0);

            return NULL;
        }
    }

    lPipeFd = open(g_szPtsFifoName, O_RDONLY | O_NONBLOCK);
    g_lPtsPipeWRFd = open(g_szPtsFifoName, O_WRONLY);

    while (1)
    {
        FD_ZERO(&ReadFds);
        FD_SET(lPipeFd, &ReadFds);
        lMaxFdp = lPipeFd;

        pthread_mutex_lock(&g_mutexWebClient);
        pstHead = &m_stClinetCBList;
        pstNode = pstHead;

        while (1)
        {
            pstNode = dos_list_work(pstHead, pstNode);
            if (DOS_ADDR_INVALID(pstNode))
            {
                break;
            }

            pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);

            if (pstData->lSocket > 0 && !pstData->bIsCacheFull)
            {
                FD_SET(pstData->lSocket, &ReadFds);
                lMaxFdp = lMaxFdp > pstData->lSocket ? lMaxFdp : pstData->lSocket;
            }
        }

        pthread_mutex_unlock(&g_mutexWebClient);

        for (i=0; i<PTS_WEB_SERVER_MAX_SIZE; i++)
        {
            if (g_lPtsServSocket[i].lSocket > 0)
            {
                FD_SET(g_lPtsServSocket[i].lSocket, &ReadFds);
                lMaxFdp = lMaxFdp > g_lPtsServSocket[i].lSocket ? lMaxFdp : g_lPtsServSocket[i].lSocket;
            }
        }

        stTvCopy = stTimeval;

        lResult = select(lMaxFdp + 1, &ReadFds, NULL, NULL, &stTvCopy);
        if (lResult < 0)
        {
            perror("pts recv msg from proxy  : fail to select");
            DOS_ASSERT(0);
            usleep(20);
            continue;
        }
        else if (0 == lResult)
        {
            continue;
        }

        for (i=0; i<=lMaxFdp; i++)
        {
            if (FD_ISSET(i, &ReadFds))
            {
                if (i == lPipeFd)
                {
                    lRecvLen = read(lPipeFd, szRecvBuf, PT_RECV_DATA_SIZE);
                    continue;
                }
                if (pts_is_serv_socket(i))
                {
                    pt_logr_debug("%s", "new connect from client to  web service of pts");
                    if ((lConnFd = accept(i, (struct sockaddr*)&stClientAddr, &ulCliaddrLen)) < 0)
                    {
                        perror("accept");
                        DOS_ASSERT(0);
                        //exit(DOS_FAIL);
                        sleep(1);

                        continue;
                    }
                    else
                    {
                        lError = setsockopt(lConnFd, SOL_SOCKET, SO_SNDBUF, (char*)&ulSocketCache, sizeof(ulSocketCache));
                        if (lError != 0)
                        {
                            logr_error("setsockopt error : %d", lError);
                            DOS_ASSERT(0);
                        }

                        lError = setsockopt(lConnFd, SOL_SOCKET, SO_RCVBUF, (char*)&ulSocketCache, sizeof(ulSocketCache));
                        if (lError != 0)
                        {
                            logr_error("setsockopt error : %d", lError);
                            DOS_ASSERT(0);
                        }
                    }

                    pthread_mutex_lock(&g_mutexWebClient);
                    pts_clinetCB_insert(&m_stClinetCBList, lConnFd, stClientAddr);
                    pthread_mutex_unlock(&g_mutexWebClient);

                    continue;
                }
                else
                {
                    dos_memzero(szRecvBuf, PT_RECV_DATA_SIZE);
                    lRecvLen = recv(i, szRecvBuf, PT_RECV_DATA_SIZE, 0);
                    if (lRecvLen < 0)
                    {
                        if (errno == EAGAIN)
                        {
                            continue;
                        }
                        pts_web_free_resource(i);
                        close(i);
                    }
                    else if (lRecvLen == 0)
                    {
                        pts_web_free_resource(i);
                        close(i);
                    }
                    else
                    {
                        bIsCacheFull = pts_deal_with_web_request(szRecvBuf, i, lRecvLen);
                        if (bIsCacheFull)
                        {
                            /* 缓存已满 */
                            pts_set_cache_full_true(i);
                        }
                    }
                }
            }
        }/*end of for*/
    }/*end of while(1)*/
}

void *pts_send_msg2browser_pthread(void *arg)
{
    PT_SEND_MSG_PTHREAD_PARAM   *pstParam           = (PT_SEND_MSG_PTHREAD_PARAM *)arg;
    PT_CC_CB_ST                 *pstCCNode          = NULL;
    PT_STREAM_CB_ST             *pstStreamNode      = NULL;
    PT_DATA_TCP_ST              *pstDataTcp         = NULL;
    S8                          *pcSendMsg          = NULL;
    STREAM_CACHE_ADDR_CB_ST     *pstStreamCacheAddr = NULL;
    DLL_NODE_S                  *pstListNode        = NULL;
    socklen_t                   optlen              = sizeof(S32);
    S32                         tcpinfo             = 0;
    U32                         ulArraySub          = 0;
    S32                         lResult             = 0;
    PT_NEND_RECV_NODE_ST        stNeedRecvNode;
    S8                          szBuff[PT_RECV_DATA_SIZE] = {0};        /* 包数据 */
    U32                         ulBuffLen           = 0;
    U32                         ulSeq               = 0;
    U32                         ulSteamID           = 0;
    struct timeval              now;
    struct timespec             timeout;

    pthread_mutex_lock(&g_mutexStreamAddrList);
    pstListNode = dll_find(&g_stStreamAddrList, &pstParam->ulStreamID, pts_find_stream_addr_by_streamID);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        pthread_mutex_unlock(&g_mutexStreamAddrList);

        goto end;
    }

    pstStreamCacheAddr = pstListNode->pHandle;
    if (DOS_ADDR_INVALID(pstStreamCacheAddr))
    {
        pthread_mutex_unlock(&g_mutexStreamAddrList);

        goto end;
    }

    pstCCNode = pstStreamCacheAddr->pstPtcRecvNode;
    if (DOS_ADDR_INVALID(pstCCNode))
    {
        pthread_mutex_unlock(&g_mutexStreamAddrList);

        goto end;
    }

    pstStreamNode = pstStreamCacheAddr->pstStreamRecvNode;
    if (DOS_ADDR_INVALID(pstStreamNode) || pstStreamNode->ulStreamID != pstParam->ulStreamID)
    {
        pstStreamCacheAddr->pstStreamRecvNode = NULL;
        pthread_mutex_unlock(&g_mutexStreamAddrList);

        goto end;
    }

    pthread_mutex_unlock(&g_mutexStreamAddrList);
    pthread_mutex_lock(&pstCCNode->pthreadMutex);

    if (DOS_ADDR_INVALID(pstStreamNode->unDataQueHead.pstDataTcp))
    {
        pthread_mutex_unlock(&pstCCNode->pthreadMutex);

        goto end;
    }

    dos_memcpy(stNeedRecvNode.aucID, pstParam->aucID, PTC_ID_LEN);
    stNeedRecvNode.ulStreamID = pstParam->ulStreamID;
    stNeedRecvNode.enDataType = pstParam->enDataType;

    pstStreamNode->bIsUsing = DOS_TRUE;

    while (!pstParam->bIsNeedExit)
    {
        /* 发送包，直到不连续 */
        pstStreamNode->lCurrSeq++;
        ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1);
        pstDataTcp = pstStreamNode->unDataQueHead.pstDataTcp;
        if (DOS_ADDR_INVALID(pstDataTcp))
        {
            pstStreamNode->bIsUsing = DOS_FALSE;
            pts_web_close_socket(pstParam->lSocket);
            pthread_mutex_unlock(&pstCCNode->pthreadMutex);
            //pts_web_free_stream(&stNeedRecvNode, pstStreamNode, pstCCNode);

            goto end;
        }

        if (pstDataTcp[ulArraySub].lSeq == pstStreamNode->lCurrSeq)
        {
            if (pstDataTcp[ulArraySub].ulLen == 0)
            {
                pstStreamNode->bIsUsing = DOS_FALSE;
                pt_logr_info("recv 0 from ptc, close socket %d, stream : %d", pstParam->lSocket, pstStreamNode->ulStreamID);
                pts_web_close_socket(pstParam->lSocket);
                write(g_lPtsPipeWRFd, "s", 1);
                pthread_mutex_unlock(&pstCCNode->pthreadMutex);
                pts_web_free_stream(&stNeedRecvNode, pstStreamNode, pstCCNode);

                goto end;
            }
            /* 在http响应头中ptc ID设置为cookie */
            pcSendMsg = pstDataTcp[ulArraySub].szBuff;
            {
                if (pstParam->lSocket < 0)
                {
                    pstStreamNode->bIsUsing = DOS_FALSE;
                    pts_web_close_socket(pstParam->lSocket);
                    pthread_mutex_unlock(&pstCCNode->pthreadMutex);
                    pts_web_free_stream(&stNeedRecvNode, pstStreamNode, pstCCNode);

                    goto end;
                }
                if (getsockopt(pstParam->lSocket, IPPROTO_TCP, TCP_INFO, &tcpinfo, &optlen) < 0)
                {
                    pstStreamNode->bIsUsing = DOS_FALSE;
                    pt_logr_info("get info fail");
                    DOS_ASSERT(0);
                    pts_web_close_socket(pstParam->lSocket);
                    pthread_mutex_unlock(&pstCCNode->pthreadMutex);
                    pts_web_free_stream(&stNeedRecvNode, pstStreamNode, pstCCNode);

                    goto end;
                }

                if (TCP_CLOSE == tcpinfo || TCP_CLOSE_WAIT == tcpinfo || TCP_CLOSING == tcpinfo)
                {
                    pstStreamNode->bIsUsing = DOS_FALSE;
                    pts_web_close_socket(pstParam->lSocket);
                    pthread_mutex_unlock(&pstCCNode->pthreadMutex);
                    pts_web_free_stream(&stNeedRecvNode, pstStreamNode, pstCCNode);

                    goto end;
                }

                ulBuffLen = pstDataTcp[ulArraySub].ulLen;
                dos_memcpy(szBuff, pcSendMsg, ulBuffLen);
                pcSendMsg = szBuff;
                ulSeq = pstDataTcp[ulArraySub].lSeq;
                ulSteamID = pstStreamNode->ulStreamID;
                pts_trace(pstCCNode->bIsTrace, LOG_LEVEL_DEBUG, "will send data to browser, stream : %d, seq : %d, len : %d", ulSteamID, ulSeq, ulBuffLen);
                pthread_mutex_unlock(&pstCCNode->pthreadMutex);
                for (;;)
                {
                    lResult = send(pstParam->lSocket, pcSendMsg, ulBuffLen, MSG_NOSIGNAL);//MSG_NOSIGNAL
                    pts_trace(pstCCNode->bIsTrace, LOG_LEVEL_DEBUG, "send data to browser, stream : %d, seq : %d, len : %d, result : %d", ulSteamID, ulSeq, ulBuffLen, lResult);
                    if (lResult != ulBuffLen)
                    {
                        pt_logr_info("send data to browser, stream : %d, seq : %d, len : %d, result : %d", ulSteamID, ulSeq, ulBuffLen, lResult);
                    }

                    if (lResult <= 0)
                    {
                        perror("send to browser");
                        printf("EINTR : %d, error : %d\n", EINTR, errno);
                        if (errno == EINTR)
                        {
                            continue;
                        }
                        pthread_mutex_lock(&pstCCNode->pthreadMutex);
                        //pstStreamNode->bIsUsing = DOS_FALSE;
                        pts_web_close_socket(pstParam->lSocket);
                        pthread_mutex_unlock(&pstCCNode->pthreadMutex);
                        pts_web_free_stream(&stNeedRecvNode, pstStreamNode, pstCCNode);

                        goto end;
                    }
                    else if (lResult < ulBuffLen)
                    {
                        pcSendMsg = pcSendMsg + lResult;
                        ulBuffLen -= lResult;

                        continue;
                    }
                    else
                    {
                        break;
                    }
                }

                pthread_mutex_lock(&pstCCNode->pthreadMutex);
            }
        }
        else
        {
            pstStreamNode->lCurrSeq--;
            gettimeofday(&now, NULL);
            timeout.tv_sec = now.tv_sec;
            timeout.tv_nsec = now.tv_usec * 1000 + 50;

            pthread_mutex_unlock(&pstCCNode->pthreadMutex);
            sem_wait(&pstParam->stSemSendMsg);
            pthread_mutex_lock(&pstCCNode->pthreadMutex);
        }
    } /* end of while(1) */

    pthread_mutex_unlock(&pstCCNode->pthreadMutex);
end:
    pthread_mutex_lock(&g_mutexSendMsgPthreadList);
    pt_send_msg_pthread_delete(&g_stSendMsgPthreadList, pstParam->ulStreamID);
    pthread_mutex_unlock(&g_mutexSendMsgPthreadList);

    return NULL;
}

/**
 * 函数：VOID pts_send_msg2web(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
 * 功能：发送消息到web浏览器
 * 参数
 * 返回值:
 */
VOID pts_send_msg2browser(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
{
    S32                         lResult             = 0;
    PTS_CLIENT_CB_ST            *pstClinetCB        = NULL;
    PT_SEND_MSG_PTHREAD         *pstSendPthread     = NULL;
    S8  szExitReason[PT_DATA_BUFF_1024]     = {0};
    pthread_t sendPthreadId;

    pthread_mutex_lock(&g_mutexWebClient);
    pstClinetCB = pts_clinetCB_search_by_stream(&m_stClinetCBList, pstNeedRecvNode->ulStreamID);
    if(DOS_ADDR_INVALID(pstClinetCB))
    {
        /* 没有找到stream对应的套接字 */
        pt_logr_debug("pts send msg to proxy : error not found stockfd");
        pthread_mutex_unlock(&g_mutexWebClient);

        return;
    }

    if (pstNeedRecvNode->ExitNotifyFlag)
    {
        /* 响应结束 */
        switch (pstNeedRecvNode->lSeq)
        {
            case 1:
                dos_snprintf(szExitReason, PT_DATA_BUFF_1024, PTS_VISIT_WEB_ERROE_1);
                break;
            case 2:
                dos_snprintf(szExitReason, PT_DATA_BUFF_1024, PTS_VISIT_WEB_ERROE_2);
                break;
            case 3:
                dos_snprintf(szExitReason, PT_DATA_BUFF_1024, PTS_VISIT_WEB_ERROE_3);
                break;
            default:
                szExitReason[0] = '\0';
                break;
        }

        /* 关闭socket */
        send(pstClinetCB->lSocket, szExitReason, dos_strlen(szExitReason), 0);
        pts_web_close_socket_without_sem(pstClinetCB->lSocket);
        pthread_mutex_unlock(&g_mutexWebClient);

        return;
    }

    pthread_mutex_lock(&g_mutexSendMsgPthreadList);
    pstSendPthread = pt_send_msg_pthread_search(&g_stSendMsgPthreadList, pstNeedRecvNode->ulStreamID);
    if (DOS_ADDR_INVALID(pstSendPthread))
    {
        pthread_mutex_unlock(&g_mutexSendMsgPthreadList);
        pstSendPthread = pt_send_msg_pthread_create(pstNeedRecvNode, pstClinetCB->lSocket);
        if (DOS_ADDR_INVALID(pstSendPthread))
        {
            pthread_mutex_unlock(&g_mutexWebClient);

            return;
        }

        /* 创建线程 */
        lResult = pthread_create(&sendPthreadId, NULL, pts_send_msg2browser_pthread, (void *)pstSendPthread->pstPthreadParam);
        if (lResult < 0)
        {
            dos_dmem_free(pstSendPthread->pstPthreadParam);
            pstSendPthread->pstPthreadParam = NULL;
            dos_dmem_free(pstSendPthread);
            pstSendPthread = NULL;

            return;
        }
        pthread_detach(sendPthreadId);

        pthread_mutex_unlock(&g_mutexWebClient);

        pthread_mutex_lock(&g_mutexSendMsgPthreadList);
        dos_list_add_tail(&g_stSendMsgPthreadList, &pstSendPthread->stList);
        pthread_mutex_unlock(&g_mutexSendMsgPthreadList);
    }
    else
    {
        sem_post(&pstSendPthread->pstPthreadParam->stSemSendMsg);

        pthread_mutex_unlock(&g_mutexSendMsgPthreadList);
        pthread_mutex_unlock(&g_mutexWebClient);
    }

    return;
}

S32 pts_printf_web_msg(U32 ulIndex, S32 argc, S8 **argv)
{
    list_t              *pstNode = NULL;
    list_t              *pstHead = NULL;
    PTS_CLIENT_CB_ST    *pstData = NULL;
    U32                 ulLen    = 0;
    S32                 i        = 0;
    S8                  szBuff[PT_DATA_BUFF_512] = {0};

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%-20s%-15s%-10s%-10s\r\n", "SN", "StreamID", "Socket", "CacheFull");
    cli_out_string(ulIndex, szBuff);

    pthread_mutex_lock(&g_mutexWebClient);
    pstHead = &m_stClinetCBList;
    pstNode = pstHead;

    while (1)
    {
        pstNode = dos_list_work(pstHead, pstNode);
        if (DOS_ADDR_INVALID(pstNode))
        {
            break;
        }

        pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);

        snprintf(szBuff, sizeof(szBuff), "%.16s%11d%10d%10d\r\n", pstData->aucID, pstData->ulStreamID, pstData->lSocket, pstData->bIsCacheFull);
        cli_out_string(ulIndex, szBuff);
    }

    pthread_mutex_unlock(&g_mutexWebClient);

    cli_out_string(ulIndex, "------------------------------------------------\r\n");
    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%-15s%-10s\r\n", "Socket", "Port");
    cli_out_string(ulIndex, szBuff);

    for (i=0; i<PTS_WEB_SERVER_MAX_SIZE; i++)
    {
        if (g_lPtsServSocket[i].lSocket > 0)
        {
            snprintf(szBuff, sizeof(szBuff), "%-15d%-10d\r\n", g_lPtsServSocket[i].lSocket, g_lPtsServSocket[i].usServPort);
            cli_out_string(ulIndex, szBuff);
        }
    }

    return 0;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

