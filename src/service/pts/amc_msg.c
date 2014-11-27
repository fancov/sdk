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
#include <limits.h>
#include "amc_msg.h"



U8        g_uIpccIDRecv[IPCC_ID_LEN];                           /*接收msg的IPCC ID*/
U8        g_uIpccIDSend[IPCC_ID_LEN];                           /*发送msg的IPCC ID*/
U32       g_ulStreamIDRecv = 0;                                 /*接收msg的stream ID*/
U32       g_ulStreamIDSend = 0;                                 /**/
S32       g_lSeqSend       = 0;                                 /*发送的包编号*/
S32       g_lResendSeq     = 0;                                 /*需要重新接收的包编号*/
BOOL      g_bIsResend      = DOS_FALSE;                         /*重新发送包flag*/
BOOL      g_bIsResendReq   = DOS_FALSE;                         /*重发请求flag*/
list_t   *g_stCCCBSend     = NULL;                              /*发送缓存*/
list_t   *g_stCCCBRecv     = NULL;                              /*接收缓存*/
pthread_mutex_t g_mutex_send  = PTHREAD_MUTEX_INITIALIZER;      /*发送线程锁*/
pthread_cond_t  g_cond_send   = PTHREAD_COND_INITIALIZER;       /*发送条件变量*/
pthread_mutex_t g_mutex_recv  = PTHREAD_MUTEX_INITIALIZER;      /*接收线程锁*/
pthread_cond_t  g_cond_recv   = PTHREAD_COND_INITIALIZER;       /*接收条件变量*/
PTS_SERV_MSG_ST g_stServMsg;
PT_DATA_TYPE_EN g_enDataTypeRecv;                               /*接收msg的type*/
PT_DATA_TYPE_EN g_enDataTypeSend;                               /*发送msg的type*/


/*--------------------------private  function-------------------*/


PT_CC_CB_ST *pts_CC_node_create(U8 *pcIpccId, PT_DATA_TYPE_EN enType, struct sockaddr_in stDestAddr, S32 lSocket, list_t *pstStreamNode)
{
    PT_CC_CB_ST *pstNewNode = NULL;

    if (NULL == pcIpccId)
    {
        alert("insert fail:have pointer age is NULL");
        return NULL;
    }

    pstNewNode = (PT_CC_CB_ST *)malloc(sizeof(PT_CC_CB_ST));
    if (NULL == pstNewNode)
    {
        perror("malloc");
        return NULL;
    }

	memcpy(pstNewNode->aucID, pcIpccId, IPCC_ID_LEN);
    pstNewNode->lSocket = lSocket;
    pstNewNode->stDestAddr = stDestAddr;

    pstNewNode->astDataTypes[PT_DATA_CTRL].enDataType = PT_DATA_CTRL;
    pstNewNode->astDataTypes[PT_DATA_CTRL].bValid = DOS_TRUE;
    pstNewNode->astDataTypes[PT_DATA_CTRL].pstStreamQueHead = NULL;
    pstNewNode->astDataTypes[PT_DATA_CTRL].ulStreamNumInQue = 0;
    pstNewNode->astDataTypes[PT_DATA_WEB].enDataType = PT_DATA_WEB;
    pstNewNode->astDataTypes[PT_DATA_WEB].bValid = DOS_FALSE;
    pstNewNode->astDataTypes[PT_DATA_WEB].pstStreamQueHead = NULL;
    pstNewNode->astDataTypes[PT_DATA_WEB].ulStreamNumInQue = 0;
    pstNewNode->astDataTypes[PT_DATA_CMD].enDataType = PT_DATA_CMD;
    pstNewNode->astDataTypes[PT_DATA_CMD].bValid = DOS_FALSE;
    pstNewNode->astDataTypes[PT_DATA_CMD].pstStreamQueHead = NULL;
    pstNewNode->astDataTypes[PT_DATA_CMD].ulStreamNumInQue = 0;

    pstNewNode->astDataTypes[enType].bValid = DOS_TRUE;
    if (NULL != pstStreamNode)
    {
        pstNewNode->astDataTypes[PT_DATA_CTRL].pstStreamQueHead = pstStreamNode;
    }

    return pstNewNode;
}


list_t *pts_recv_CClist_insert(list_t *pstQueHead, U8 *pcIpccId, PT_DATA_TYPE_EN enType, struct sockaddr_in stDestAddr, S32 lSocket, list_t *pstStreamNode)
{
    PT_CC_CB_ST *pstNewNode = NULL;

    if (NULL == pcIpccId || NULL == pstStreamNode)
    {
       alert("insert fail:have pointer age is NULL");
       return NULL;
    }

    pstNewNode = pts_CC_node_create(pcIpccId, enType, stDestAddr, lSocket, pstStreamNode);
    if (NULL == pstNewNode)
    {
       alert("create cccb node fail");
       return NULL;
    }

    if (NULL == pstQueHead)
    {
        pstQueHead = &(pstNewNode->stCCListNode);
        list_init(pstQueHead);
    }
    else
    {
        list_add_tail(pstQueHead, &(pstNewNode->stCCListNode));
    }

    return pstQueHead;
}

list_t *pts_send_CClist_insert(list_t *pstQueHead, U8 *pcIpccId, PT_DATA_TYPE_EN enType, struct sockaddr_in stDestAddr, S32 lSocket, list_t *pstStreamListHead)
{
    PT_CC_CB_ST *pstNewNode = NULL;

    if (NULL == pcIpccId)
    {
       alert("insert fail:have pointer age is NULL");
       return NULL;
    }

    pstNewNode = pts_CC_node_create(pcIpccId, enType, stDestAddr, lSocket, pstStreamListHead);
    if (NULL == pstNewNode)
    {
       alert("create cccb node fail");
       return NULL;
    }

    if (NULL == pstQueHead)
    {
        pstQueHead = &(pstNewNode->stCCListNode);
        list_init(pstQueHead);
    }
    else
    {
        list_add_tail(pstQueHead, &(pstNewNode->stCCListNode));
    }

    return pstQueHead;
}

PT_CC_CB_ST *pts_CClist_search(list_t* pstHead, U8 *pucID)
{
    list_t *pstNode = NULL;
    PT_CC_CB_ST   *pstData = NULL;

    if (NULL == pstHead)
    {
        alert("empty list!");
        return NULL;
    }
    else if (NULL == pucID)
    {
        alert("IPCC ID is NULL!");
        return NULL;
    }

    pstNode = pstHead;
    pstData = list_entry(pstNode, PT_CC_CB_ST, stCCListNode);
    while (memcmp(pstData->aucID, pucID, IPCC_ID_LEN) && pstNode->next!=pstHead)
    {
        pstNode = pstNode->next;
        pstData = list_entry(pstNode, PT_CC_CB_ST, stCCListNode);
    }
    if (!memcmp(pstData->aucID, pucID, IPCC_ID_LEN))
    {
        return pstData;
    }
    else
    {
        alert("not found!");
        return NULL;
    }
}


void pts_data_lose(PT_MSG_ST *pstMsgDes, U32 ulLoseSeq)
{
    if (NULL == pstMsgDes)
    {
        alert("arg error");
        return;
    }

    pthread_mutex_lock(&g_mutex_send);
    g_bIsResend = DOS_FALSE;
    g_bIsResendReq = DOS_TRUE;
    memcpy(g_uIpccIDSend, pstMsgDes->aucID, IPCC_ID_LEN);
    g_enDataTypeSend = pstMsgDes->enDataType;
    g_ulStreamIDSend = pstMsgDes->ulStreamID;
    g_lResendSeq = ulLoseSeq;

    pthread_cond_signal(&g_cond_send);
    pthread_mutex_unlock(&g_mutex_send);
}

void pts_send_lose_data_msg_callback(U64 ulLoseMsg)
{
    if (0 == ulLoseMsg)
    {
        return;
    }
    PT_LOSE_BAG_MSG_ST *pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)ulLoseMsg;
    PT_MSG_ST *pstMsg = pstLoseMsg->stMsg;
    PT_STREAM_CB_ST *pstStreamNode = pstLoseMsg->stStreamNode;

    S32 i = 0;
    U32 ulCount = 0;
    U32 ulArraySub = 0;

    if (pstStreamNode->ulCountResend == 3)
    {
        /*关闭sockfd*/
    }
    else
    {
        pstStreamNode->ulCountResend++;
        for (i=pstStreamNode->ulCurrSeq+1; i<pstStreamNode->ulMaxSeq; i++)
        {
            ulArraySub = i & (PT_DATA_SEND_CACHE_SIZE - 1);
            if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].bIsSaveMsg != DOS_TRUE ||
                (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].bIsSaveMsg == DOS_TRUE &&
                pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].lSeq <= pstStreamNode->ulCurrSeq))
            {
                ulCount++;
                pstStreamNode->ulCountResend++;
                /*发送丢包重发请求*/
                pts_data_lose(pstMsg, ulArraySub);
            }
        }
    }

    if (0 == ulCount)
    {
        /*没有需要发送的丢包重发请求，关闭定时器*/
        dos_tmr_stop(&pstStreamNode->hTmrHandle);
        pstStreamNode->ulCountResend = 0;
    }
}

void pts_send_login_verify(PT_MSG_ST *pstMsgDes)
{
    if (NULL == pstMsgDes)
    {
        alert("arg error");
        return;
    }

    pthread_mutex_lock(&g_mutex_send);
    g_bIsResend = DOS_FALSE;
    g_bIsResendReq = DOS_FALSE;
    memcpy(g_uIpccIDSend, pstMsgDes->aucID, IPCC_ID_LEN);
    g_enDataTypeSend = pstMsgDes->enDataType;
    g_ulStreamIDSend = pstMsgDes->ulStreamID;

    pthread_cond_signal(&g_cond_send);
    pthread_mutex_unlock(&g_mutex_send);
}

void pts_create_rand_str(S8 *szVerifyStr)
{
    U8 i = 0;

    srand((int)time(0));
    for(i=0; i<PTS_LOGIN_VERIFY_SIZE-1; i++)
    {
        szVerifyStr[i] = 1 + (rand()&CHAR_MAX);
    }
    szVerifyStr[PTS_LOGIN_VERIFY_SIZE-1] = '\0';
}

S32 pts_key_convert(S8 *szKey, S8 *szDest)
{
    if (NULL == szKey || NULL == szDest)
    {

        return DOS_FAIL;
    }

    S32 i = 0;

    for (i=0; i<PTS_LOGIN_VERIFY_SIZE-1; i++)
    {
        szDest[i] = szKey[i]&0xA9;
    }
    szDest[PTS_LOGIN_VERIFY_SIZE] = '\0';

    return DOS_SUCC;
}

S32 pts_get_key(PT_MSG_ST *pstMsgDes, S8 *szKey, U32 ulKeySize)
{
    if (NULL == pstMsgDes || NULL == szKey)
    {
        alert("arg error");
        return DOS_FAIL;
    }

    list_t  *pstStreamHead          = NULL;
    PT_STREAM_CB_ST *pstStreamNode  = NULL;
    PT_CC_CB_ST     *pstCCNode      = NULL;
    PT_DATA_QUE_HEAD_UN  punDataList;
    U32 ulArrarySub = 0;

    pthread_mutex_lock(&g_mutex_send);

    pstCCNode = pts_CClist_search(g_stCCCBSend, pstMsgDes->aucID);
    if(NULL == pstCCNode)
    {
         alert("not found IPCC");
    }
    pstStreamHead = pstCCNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
    if (NULL == pstStreamHead)
    {
         alert("not found Stream list");
    }
    else
    {
        pstStreamNode = pt_stream_queue_serach(pstStreamHead, 1);
        if(NULL == pstStreamNode)
        {
            alert("not found Stream node");
        }
        else
        {
            punDataList = pstStreamNode->unDataQueHead;
            if (NULL == punDataList.pstDataTcp)
            {
                alert("not found data");
            }
			else
			{
	            ulArrarySub = pstMsgDes->lSeq & (PT_DATA_SEND_CACHE_SIZE - 1);
				alert("seq = %d, %d\n", punDataList.pstDataTcp[ulArrarySub].lSeq, pstMsgDes->lSeq);
	            if (punDataList.pstDataTcp[ulArrarySub].lSeq == pstMsgDes->lSeq)
	            {
	                memcpy(szKey, punDataList.pstDataTcp[ulArrarySub].szBuff, ulKeySize-1);
	                szKey[ulKeySize-1] = '\0';

	                pthread_mutex_unlock(&g_mutex_send);
	                return DOS_SUCC;
	            }
			
			}

        }
		
    }

    pthread_mutex_unlock(&g_mutex_send);

    return DOS_FAIL;
}

/*--------------------------------public  function-------------------------------------*/

/**
 * 函数：void pts_amc_send_msg(S8 *pcIpccId, PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen, U8 ExitNotifyFlag)
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
void pts_amc_send_msg(U8 *pcIpccId, PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen, U8 ExitNotifyFlag)
{
    if (enDataType < 0 || enDataType > 2)
    {
        alert("Data Type should in 0-2: %d", enDataType);
        return;
    }
    else if (NULL == pcIpccId || NULL == pcData)
    {
        alert("data pointer is NULL");
        return;
    }

    PT_CC_CB_ST      *pstCCNode      = NULL;
    PT_STREAM_CB_ST  *pstStreamNode  = NULL;
    list_t    *pstStreamListHead     = NULL;
    list_t    *pstStreamListNode     = NULL;
    S32 lDataSeq = 0;

    pthread_mutex_lock(&g_mutex_send);

    pstCCNode = pts_CClist_search(g_stCCCBSend, pcIpccId);
    if(NULL == pstCCNode)
    {
        alert("not found IPCC");
        goto pts_send_lock;
    }
    else
    {
        pstStreamListHead = pstCCNode->astDataTypes[enDataType].pstStreamQueHead;
        if (NULL == pstStreamListHead)
        {
            alert(" Stream list is NULL");
            pstStreamListHead = pt_create_stream_and_data_que(ulStreamID, pcData, lDataLen, ExitNotifyFlag, 0, PT_DATA_SEND_CACHE_SIZE);
            if (NULL == pstStreamListHead)
            {
                /*创建失败*/
                goto pts_send_lock;
            }
            else
            {
                pstCCNode->astDataTypes[enDataType].pstStreamQueHead = pstStreamListHead;
            }
        }
        else
        {
            pstStreamNode = pt_stream_queue_serach(pstStreamListHead, ulStreamID);
            if (NULL == pstStreamNode)
            {
                pstStreamListNode = pt_create_stream_and_data_que(ulStreamID, pcData, lDataLen, ExitNotifyFlag, 0, PT_DATA_SEND_CACHE_SIZE);
                if (NULL == pstStreamListNode)
                {
                    /*创建失败*/
                    goto pts_send_lock;
                }
                else
                {
                    pstStreamListHead = pt_stream_queue_insert(pstStreamListHead, pstStreamListNode);
                    if (NULL == pstStreamListHead)
                    {
                        alert("insert stream queue fail");
                        pstStreamNode = list_entry(pstStreamListHead, PT_STREAM_CB_ST, stStreamListNode);
                        free(pstStreamNode->unDataQueHead.pstDataTcp);
                        free(pstStreamNode);
                        goto pts_send_lock;
                    }
                }
            }
            else
            {
                pstStreamNode->ulMaxSeq = pstStreamNode->ulMaxSeq + 1;
                lDataSeq = pstStreamNode->ulMaxSeq;

                if (pt_send_data_tcp_queue_add(pstStreamNode, ExitNotifyFlag, pcData, lDataLen, lDataSeq, PT_DATA_SEND_CACHE_SIZE))
                {
                    alert("add data queue fail");
                    /*释放资源!!!!!!*/
                    goto pts_send_lock;
                }
            }
        }
    }

	memcpy(g_uIpccIDSend, pcIpccId, IPCC_ID_LEN);
    g_enDataTypeSend = enDataType;
    g_ulStreamIDSend = ulStreamID;
    pthread_cond_signal(&g_cond_send);
pts_send_lock:    pthread_mutex_unlock(&g_mutex_send);

}

/**
 * 函数：void *pts_send_msg2ipcc(void *arg)
 * 功能：
 *      1.发送数据线程
 * 参数
 *      void *arg :通道通信的sockfd
 * 返回值：void *
 */
void *pts_send_msg2ipcc(void *arg)
{
    S32              lSockfd         = *(S32 *)arg;
    PT_CC_CB_ST      *pstCCNode      = NULL;
    S32              lSeq            = 0;
    U32              ulArraySub      = 0;
    U32              ulSendCount     = 0;
    PT_MSG_ST        stMsgDes;
    list_t    *pstStreamHead  = NULL;
    PT_STREAM_CB_ST  *pstStreamNode  = NULL;
    PT_DATA_TCP_ST   stSendDataNode;
    S8 acBuff[PT_SEND_DATA_SIZE]     = {0};

    while(1)
    {
        pthread_mutex_lock(&g_mutex_send);
        pthread_cond_wait(&g_cond_send, &g_mutex_send);

        memset(&stMsgDes, 0, sizeof(PT_MSG_ST));
        pstCCNode = pts_CClist_search(g_stCCCBSend, g_uIpccIDSend);
        if(NULL == pstCCNode)
        {
            alert("not found IPCC");
        }
        else if (g_bIsResendReq)
        {
            /*发送丢包、重发请求*/
            stMsgDes.enDataType = g_enDataTypeSend;
            memcpy(stMsgDes.aucID, g_uIpccIDSend, IPCC_ID_LEN);
            stMsgDes.ulStreamID = g_ulStreamIDSend;
            stMsgDes.ExitNotifyFlag = DOS_FALSE;
            stMsgDes.lSeq =  g_lSeqSend;
            stMsgDes.bIsResendReq = DOS_TRUE;
            stMsgDes.bIsEncrypt = DOS_FALSE;
            stMsgDes.bIsCompress = DOS_FALSE;

            alert("lose data bag request");
            sendto(lSockfd, (void *)&stMsgDes, sizeof(PT_MSG_ST), 0,  (struct sockaddr*)&pstCCNode->stDestAddr, sizeof(pstCCNode->stDestAddr));
        }
        else
        {
            pstStreamHead = pstCCNode->astDataTypes[g_enDataTypeSend].pstStreamQueHead;
            if (NULL == pstStreamHead)
            {
                alert("not found data");
            }
            else
            {
                pstStreamNode = pt_stream_queue_serach(pstStreamHead, g_ulStreamIDSend);
                if(NULL == pstStreamNode)
                {
                    alert("not found data");
                }
                else
                {
                    if (g_bIsResend)
                    {
                        /*要求重传的包*/
                        lSeq = g_lSeqSend;
                    }
                    else
                    {
                        lSeq = pstStreamNode->ulCurrSeq + 1;
                        pstStreamNode->ulCurrSeq = lSeq;
                    }
                    if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
                    {
                        alert("not found data");
                    }
                    else if (lSeq < pstStreamNode->ulMinSeq || lSeq > pstStreamNode->ulMaxSeq)
                    {
                        alert("not found data");
                    }
                    else
                    {
                        ulArraySub = lSeq & (PT_DATA_SEND_CACHE_SIZE - 1);
                        stSendDataNode = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];

                        stMsgDes.enDataType = g_enDataTypeSend;
                        memcpy(stMsgDes.aucID, pstCCNode->aucID, IPCC_ID_LEN);
                        stMsgDes.ulStreamID = g_ulStreamIDSend;
                        stMsgDes.ExitNotifyFlag = stSendDataNode.ExitNotifyFlag;
                        stMsgDes.lSeq = lSeq;
                        stMsgDes.bIsResendReq = DOS_FALSE;
                        stMsgDes.bIsEncrypt = DOS_FALSE;
                        stMsgDes.bIsCompress = DOS_FALSE;
                        /*stMsgDes.aulServIp[0] = g_stServMsg.achProxyIP[0];
                        stMsgDes.usServPort = g_stServMsg.us;*/

                        memcpy(acBuff, (void *)&stMsgDes, sizeof(PT_MSG_ST));
                        memcpy(acBuff+sizeof(PT_MSG_ST), stSendDataNode.szBuff, stSendDataNode.ulLen);
                        if (g_bIsResend)
                        {
                            ulSendCount = PT_RESEND_COUNT;    /*重传的，发送三遍*/
                        }
                        else
                        {
                            ulSendCount = 1;
                        }
                        while (ulSendCount)
                        {
							alert("send data size:%d",stSendDataNode.ulLen);
                            sendto(lSockfd, acBuff, stSendDataNode.ulLen + sizeof(PT_MSG_ST), 0, (struct sockaddr*)&pstCCNode->stDestAddr, sizeof(pstCCNode->stDestAddr));
                            ulSendCount--;
                        }

                    }
                }
            }
        }
        pthread_mutex_unlock(&g_mutex_send);
    }

    pthread_mutex_destroy(&g_mutex_send);
    pthread_cond_destroy(&g_cond_send);
}


/**
 * 函数：void *pts_recv_msg_from_ipcc(void *arg)
 * 功能：
 *      1.接收数据线程
 * 参数
 *      void *arg :通道通信的sockfd
 * 返回值：void *
 */
void *pts_recv_msg_from_ipcc(void *arg)
{
    S32                 lSockfd             = *(S32 *)arg;
    S32                 lRecvLen            = 0;
    S32                 lShouldSeq          = 0;
    S32                 lResult             = 0;
    struct sockaddr_in  stClientAddr;
    socklen_t           lCliaddrLen         = sizeof(stClientAddr);
    PT_MSG_ST           *pstMsgDes          = NULL;
    list_t              *pstCCList          = NULL;
    list_t              *pstStreamListHead  = NULL;
    list_t              *pstStreamListNode  = NULL;
    PT_CC_CB_ST         *pstCCNode          = NULL;
    PT_DATA_TCP_ST      *pstTcpDataQueHead  = NULL;
    PT_STREAM_CB_ST     *pstStreamNode      = NULL;
    S8                  acRecvBuf[PT_SEND_DATA_SIZE]    = {0};
    S8                  acMsgDes[PTS_DATA_BUFF_512]     = {0};
    PT_LOSE_BAG_MSG_ST  *pstLoseMsg          = NULL;
    S8                  szKey[PTS_LOGIN_VERIFY_SIZE] = {0};
    S8                  szDestKey[PTS_LOGIN_VERIFY_SIZE] = {0};
    BOOL                bIsLoginSuc;
    S8                  szLoginRes[PTS_DATA_BUFF_512] = {0};

    while(1)
    {
        memset(&stClientAddr, 0, sizeof(stClientAddr));
        memset(acMsgDes, 0, PTS_DATA_BUFF_512);
        lRecvLen = recvfrom(lSockfd, acRecvBuf, PT_SEND_DATA_SIZE, 0, (struct sockaddr*)&stClientAddr, &lCliaddrLen);
        alert("recv data len: %d", lRecvLen);

        /*取出头部信息*/
        memcpy(acMsgDes, acRecvBuf, sizeof(PT_MSG_ST));
        pstMsgDes = (PT_MSG_ST *)acMsgDes;

        pthread_mutex_lock(&g_mutex_recv);

        memcpy(g_uIpccIDRecv, pstMsgDes->aucID, IPCC_ID_LEN);
        g_enDataTypeRecv = pstMsgDes->enDataType;
        g_ulStreamIDRecv = pstMsgDes->ulStreamID;

        if (pstMsgDes->enDataType == PT_DATA_CTRL)
        {
            /*控制信息*/
            switch (pstMsgDes->ulStreamID)
            {
                case 1:
                    /*登陆请求*/
                    alert("ipcc id : %16s", pstMsgDes->aucID);

                    /*添加IPCC到接收缓存链表*/
                    pstStreamListHead = pt_create_stream_and_data_que(pstMsgDes->ulStreamID, acRecvBuf, lRecvLen, pstMsgDes->ExitNotifyFlag, 0, PT_DATA_RECV_CACHE_SIZE);
                    if (NULL == pstStreamListHead)
                    {
                        /*创建失败*/
                        goto pts_recv_lock;
                    }

                    pstCCList = pts_recv_CClist_insert(g_stCCCBRecv, pstMsgDes->aucID, PT_DATA_WEB, stClientAddr, lSockfd, pstStreamListHead);
                    if (NULL == pstCCList)
                    {
                        alert("insert CClist fail");
                    }
                    else
                    {
                        g_stCCCBRecv = pstCCList;
                    }

                    /*添加IPCC到发送缓存链表*/
                    pts_create_rand_str(szKey);  /*创建随机字符串*/
                    pstStreamListHead = pt_create_stream_and_data_que(pstMsgDes->ulStreamID, szKey, PTS_LOGIN_VERIFY_SIZE, pstMsgDes->ExitNotifyFlag, 0, PT_DATA_SEND_CACHE_SIZE);
                    if (NULL == pstStreamListHead)
                    {
                        /*创建失败*/
                        goto pts_recv_lock;
                    }

                    pstCCList = pts_send_CClist_insert(g_stCCCBSend, pstMsgDes->aucID, PT_DATA_WEB, stClientAddr, lSockfd, pstStreamListHead);
                    if (NULL == pstCCList)
                    {
                        alert("insert CClist fail");
                    }
                    else
                    {
                        g_stCCCBSend = pstCCList;
                    }

                    pts_send_login_verify(pstMsgDes);
					
					goto pts_recv_lock; 
                    break;
                case 2:
                    /*登陆验证*/
                    lResult = pts_get_key(pstMsgDes, szKey, PTS_LOGIN_VERIFY_SIZE);
                    if (lResult < 0)
                    {
                        alert("not find key");
                        /*验证失败，清除收发缓存*/
                        bIsLoginSuc = DOS_FALSE;
                    }
					else
					{
	                    lResult = pts_key_convert(szKey, szDestKey);
	                    if (lResult < 0)
	                    {
	                        alert("key convert error");
	                        /*验证失败，清除收发缓存*/
	                        bIsLoginSuc = DOS_FALSE;
	                    }
						else
						{
		                    if (0 == strcmp(szDestKey, acRecvBuf+sizeof(PT_MSG_ST)))
		                    {
		                        alert("verify succeed");
		                        bIsLoginSuc = DOS_TRUE;
		                    }
		                    else
		                    {
		                        alert("verify error");
		                        /*验证失败，清除收发缓存*/
		                        bIsLoginSuc = DOS_FALSE;
		                    }
						}
                    }
                    sprintf(szLoginRes, "result=%d", bIsLoginSuc);
					alert("login result: %s", szLoginRes);
                    pts_amc_send_msg(pstMsgDes->aucID, PT_DATA_CTRL, pstMsgDes->ulStreamID, szLoginRes, strlen(szLoginRes), DOS_FALSE);
					if (bIsLoginSuc == DOS_FALSE)
					{
						
						goto pts_recv_lock;
					}
					else
					{
						/*验证成功，存入接收缓存*/
						pstCCNode = pts_CClist_search(g_stCCCBRecv, pstMsgDes->aucID);
			            if(NULL == pstCCNode)
			            {
			                alert("not found ipcc");
			            }
						else
						{
							pstStreamListHead = pstCCNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
			                if (NULL == pstStreamListHead)
			                {
								alert("Stream List is null");
			                }
							else
							{
								pstStreamListNode = pt_create_stream_and_data_que(pstMsgDes->ulStreamID, acRecvBuf, lRecvLen, pstMsgDes->ExitNotifyFlag, 0, PT_DATA_RECV_CACHE_SIZE);
			                    if (NULL == pstStreamListHead)
			                    {
			                        /*创建失败*/
			                        goto pts_recv_lock;
			                    }
								else
		                        {
		                            pstStreamListHead = pt_stream_queue_insert(pstStreamListHead, pstStreamListNode);
		                            if (NULL == pstStreamListHead)
		                            {
		                                alert("insert stream queue fail");
		                                /*资源释放 !!!!!!*/
		                                goto pts_recv_lock;
		                            }
									alert("存入数据成功");
		                        }
							}
						}
					}
                    break;
                default:
                    break;
            }

        }
        else
        {
            pstCCNode = pts_CClist_search(g_stCCCBRecv, pstMsgDes->aucID);
            if(NULL == pstCCNode)
            {
                alert("not found ipcc");
            }
            else if (pstMsgDes->bIsResendReq)
            {
                /*pts发送的重传请求*/
                pthread_mutex_lock(&g_mutex_send);
                g_bIsResend = DOS_TRUE;
                g_bIsResendReq = DOS_FALSE;
                memcpy(g_uIpccIDSend, pstMsgDes->aucID, IPCC_ID_LEN);
                g_enDataTypeSend = pstMsgDes->enDataType;
                g_ulStreamIDSend = pstMsgDes->ulStreamID;
                g_lSeqSend = pstMsgDes->lSeq;
                pthread_cond_signal(&g_cond_send);
                pthread_mutex_unlock(&g_mutex_send);
            }
            else
            {
                pstStreamListHead = pstCCNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
                if (NULL == pstStreamListHead)
                {
                    pstStreamListHead = pt_create_stream_and_data_que(pstMsgDes->ulStreamID, acRecvBuf, lRecvLen, pstMsgDes->ExitNotifyFlag, pstMsgDes->lSeq, PT_DATA_RECV_CACHE_SIZE);
                    if (NULL == pstStreamListHead)
                    {
                        /*创建失败*/
                        goto pts_recv_lock;
                    }
                    else
                    {
                        pstCCNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead = pstStreamListHead;
                    }

                    if (pstMsgDes->lSeq != 0)
                    {
                        /*丢包*/
                        pstStreamNode = list_entry(pstStreamListHead, PT_STREAM_CB_ST, stStreamListNode);

                        pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)malloc(sizeof(PT_LOSE_BAG_MSG_ST));
                        if (NULL == pstLoseMsg)
                        {
                            perror("malloc");
                            return NULL;
                        }

                        pstLoseMsg->stMsg = pstMsgDes;
                        pstLoseMsg->stStreamNode = pstStreamNode;

                        pts_send_lose_data_msg_callback((U64)pstLoseMsg);

                        lResult = dos_tmr_start(&pstStreamNode->hTmrHandle, PT_SEND_LOSE_DATA_TIMER, pts_send_lose_data_msg_callback, (U64)pstLoseMsg, TIMER_NORMAL_LOOP);
                        if (DOS_FAIL == lResult)
                        {
                            alert("start timer fail");
                            /*释放资源!!!!!*/
                            return NULL;
                        }
                        goto pts_recv_lock;
                    }
                }
                else
                {
                    pstStreamNode = pt_stream_queue_serach(pstStreamListHead, pstMsgDes->ulStreamID);
                    if (NULL == pstStreamNode)
                    {
                        pstStreamListNode = pt_create_stream_and_data_que(pstMsgDes->ulStreamID, acRecvBuf, lRecvLen, pstMsgDes->ExitNotifyFlag, pstMsgDes->lSeq, PT_DATA_RECV_CACHE_SIZE);
                        if (NULL == pstStreamListNode)
                        {
                            /*创建失败*/
                            goto pts_recv_lock;
                        }
                        else
                        {
                            pstStreamListHead = pt_stream_queue_insert(pstStreamListHead, pstStreamListNode);
                            if (NULL == pstStreamListHead)
                            {
                                alert("insert stream queue fail");
                                /*资源释放 !!!!!!*/
                                goto pts_recv_lock;
                            }
                        }

                        if (pstMsgDes->lSeq != 0)
                        {
                            /*丢包*/
                            pstStreamNode = list_entry(pstStreamListHead, PT_STREAM_CB_ST, stStreamListNode);
                            pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)malloc(sizeof(PT_LOSE_BAG_MSG_ST));
                            if (NULL == pstLoseMsg)
                            {
                                perror("malloc");
                                return NULL;
                            }

                            pstLoseMsg->stMsg = pstMsgDes;
                            pstLoseMsg->stStreamNode = pstStreamNode;
                            pts_send_lose_data_msg_callback((U64)pstLoseMsg);
                            lResult = dos_tmr_start(&pstStreamNode->hTmrHandle, PT_SEND_LOSE_DATA_TIMER, pts_send_lose_data_msg_callback, (U64)pstLoseMsg, TIMER_NORMAL_LOOP);
                            if (DOS_FAIL == lResult)
                            {
                                alert("start timer fail");
                                /*释放资源!!!!!*/
                                return NULL;
                            }
                            goto pts_recv_lock;
                        }
                    }
                    else
                    {
                        pstStreamNode->ulMaxSeq = pstStreamNode->ulMaxSeq + 1;
                        lShouldSeq = pstStreamNode->ulMaxSeq;

                        if (((pstMsgDes->lSeq - pstStreamNode->ulCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1)) == 0)
                        {
                            /*存过来一圈!!!!!!!!!!!!!!!*/
                            /*关闭sockfd*/
                        }

                        if (pt_send_data_tcp_queue_add(pstStreamNode, pstMsgDes->ExitNotifyFlag, acRecvBuf, lRecvLen, lShouldSeq, PT_DATA_SEND_CACHE_SIZE))
                        {
                            alert("add data queue fail");
                            free(pstTcpDataQueHead);        /*添加数据失败，处理方式????*/
                            goto pts_recv_lock;
                        }

                        if (pstMsgDes->lSeq != lShouldSeq)
                        {
                            /*丢包了 !!!!*/
                            if (NULL == pstStreamNode->hTmrHandle)
                            {
                                /*没有定时器的，创建定时器*/
                                pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)malloc(sizeof(PT_LOSE_BAG_MSG_ST));
                                if (NULL == pstLoseMsg)
                                {
                                    perror("malloc");
                                    return NULL;
                                }

                                pstLoseMsg->stMsg = pstMsgDes;
                                pstLoseMsg->stStreamNode = pstStreamNode;

                                lResult = dos_tmr_start(&pstStreamNode->hTmrHandle, PT_SEND_LOSE_DATA_TIMER, pts_send_lose_data_msg_callback, (U64)pstLoseMsg, TIMER_NORMAL_LOOP);
                                if (DOS_FAIL == lResult)
                                {
                                    alert("start timer fail");
                                    /*释放资源!!!!!*/
                                    return NULL;
                                }
                            }
                            goto pts_recv_lock;
                        }
                        else
                        {
                            /*最小的包丢失的包已接收到，重置个数*/
                            pstStreamNode->ulCountResend = 0;
                        }
                    }

                }

            }

        }
        pthread_cond_signal(&g_cond_recv);
pts_recv_lock: pthread_mutex_unlock(&g_mutex_recv);
    }
}


#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */