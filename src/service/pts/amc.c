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
#include <ptc_pts/dos_sqlite3.h>
#include "amc_msg.h"

#define INDEX_HTML  "GET   /index.html   HTTP/1.1\r\nAccept:   */*\r\nAccept-Language:   zh-cn\r\n \
                    User-Agent:   Mozilla/4.0   (compatible;   MSIE   5.01;   Windows   NT   5.0)\r\n \
                    Host:   192.168.124.130:80\r\nConnection:   Close\r\n\r\n "
#define HTTP_SUCCESS  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
#define HTTP_ERROR  "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n     \
                        <HTML><BODY>File not found</BODY></HTML>"
#define IPCC_LIST_HTML_HEAD "<!DOCTYPE html><html lang=\"en\"><head><title>Home Page</title></head> \
                                <script type=\"text/javascript\" src=\"ipcc_list.js\"></script><body>"


fd_set         m_ReadFds;                                      /*客户端套接字集合*/
DOS_SQLITE_ST  m_stMySqlite;                                   /*数据库sqlite3结构体*/
list_t*  	   m_stClinetCBList = NULL;                        /*客户端请求链表*/

U32 pts_create_stream_id()
{
    static U32 ulStreamId = 0;
    ulStreamId++;

    return ulStreamId;
}


list_t *pts_clinetCB_insert(list_t *psthead, S32 lSockfd, struct sockaddr_in stClientAddr)
{
    PTS_CLIENT_CB_ST *stNewNode = NULL;

    stNewNode = (PTS_CLIENT_CB_ST *)malloc(sizeof(PTS_CLIENT_CB_ST));
    if (NULL == stNewNode)
    {
        perror("malloc");
        return NULL;
    }

    stNewNode->lSocket = lSockfd;
    stNewNode->ulStreamID = pts_create_stream_id();
    stNewNode->stClientAddr = stClientAddr;
    stNewNode->eSaveHeadFlag = DOS_FALSE;

    if (NULL == psthead)
    {
        psthead = &(stNewNode->stList);
        list_init(psthead);
    }
    else
    {
        list_add_tail(psthead, &(stNewNode->stList));
    }

    return psthead;
}


PTS_CLIENT_CB_ST* pts_clinetCB_search_by_sockfd(list_t *pstHead, S32 lSockfd)
{
    list_t    *pstNode = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    if (NULL == pstHead)
    {
        alert("empty list!");
        return NULL;
    }

    pstNode = pstHead;
    pstData = list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
    while (pstData->lSocket != lSockfd && pstNode->next != pstHead)
    {
        pstNode = pstNode->next;
        pstData = (PTS_CLIENT_CB_ST *)list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
    }
    if (pstData->lSocket == lSockfd)
    {
        return pstData;
    }
    else
    {
        alert("not found!");
        return NULL;
    }
}


PTS_CLIENT_CB_ST* pts_clinetCB_search_by_stream(list_t* pstHead, U32 ulStreamID)
{
    list_t *pstNode = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    if (NULL == pstHead)
    {
        alert("PTS_CLIENT_CB_ST search : empty list");
        return NULL;
    }

    pstNode = pstHead;
    pstData = list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
    while (pstData->ulStreamID!=ulStreamID && pstNode->next!=pstHead)
    {
        pstNode = pstNode->next;
        pstData = list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
    }
    if (pstData->ulStreamID == ulStreamID)
    {
        return pstData;
    }
    else
    {
        alert("PTS_CLIENT_CB_ST search by stream : not found");
        return NULL;
    }
}


list_t *pts_clinetCB_delete_by_sockfd(list_t* pstHead, S32 lSockfd)
{
    list_t    *pstNode = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    if (NULL == pstHead)
    {
        alert("PTS_CLIENT_CB_ST delete : empty list");
        return pstHead;
    }

    pstNode = pstHead;
    pstData = list_entry(pstNode, PTS_CLIENT_CB_ST, stList);

    while (pstData->lSocket != lSockfd && pstNode->next != pstHead)
    {
        pstNode = pstNode->next;
        pstData = list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
    }
    if (pstData->lSocket == lSockfd)
    {
        if (pstNode == pstHead)
        {
            if (pstNode->next == pstHead)
            {
                pstHead = NULL;
            }
            else
            {
                pstHead = pstNode->next;
            }
        }
        list_del(pstNode);
        free(pstData);
    }
    else
    {
        alert("PTS_CLIENT_CB_ST delete : not found!");
    }

    return pstHead;
}


list_t *pts_clinetCB_delete_by_stream(list_t* pstHead, U32 ulStreamID)
{
    list_t    *pstNode = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    if (NULL == pstHead)
    {
        alert("empty list!");
        return pstHead;
    }

    pstNode = pstHead;
    pstData = list_entry(pstNode, PTS_CLIENT_CB_ST, stList);

    while (pstData->ulStreamID != ulStreamID && pstNode->next != pstHead)
    {
        pstNode = pstNode->next;
        pstData = list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
    }
    if (pstData->ulStreamID == ulStreamID)
    {
        if (pstNode == pstHead)
        {
            if (pstNode->next == pstHead)
            {
                pstHead = NULL;
            }
            else
            {
                pstHead = pstNode->next;
            }
        }
        list_del(pstNode);
        close(pstData->lSocket);
        FD_CLR(pstData->lSocket, &m_ReadFds);
        free(pstData);
    }
    else
    {
        alert("not found!");
    }

    return pstHead;
}


U32 pts_create_udp_socket(U16 usUdpPort)
{
    S32 lSockfd = 0;
    S32 lError  = 0;
    struct sockaddr_in stMyAddr;

    lSockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (lSockfd < 0)
    {
        perror("udp create socket");
        exit(DOS_FAIL);
    }

    memset(&stMyAddr, 0, sizeof(stMyAddr));
    stMyAddr.sin_family = AF_INET;
    stMyAddr.sin_port   = htons(usUdpPort);
    stMyAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    alert("udp channel server port: %d", usUdpPort);
    lError = bind(lSockfd, (struct sockaddr*)&stMyAddr, sizeof(stMyAddr));
    if (lError != 0)
    {
        perror("udp server bind");
        close(lSockfd);
        exit(DOS_FAIL);
    }

    return lSockfd;
}


U32 pts_create_tcp_socket(U16 usTcpPort)
{
    S32 lSockfd = 0;
    S32 lError  = 0;
    struct sockaddr_in stMyAddr;

    memset(&stMyAddr, 0, sizeof(stMyAddr));
    stMyAddr.sin_family = AF_INET;
    stMyAddr.sin_port   = htons(usTcpPort);
    stMyAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	alert("web service port is : %d", usTcpPort);
    lSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lSockfd < 0)
    {
        perror("web service create socket");
        exit(DOS_FAIL);
    }
    lError = bind(lSockfd, (struct sockaddr*)&stMyAddr, sizeof(stMyAddr));
    if (lError != 0)
    {
        perror("web service bind");
        close(lSockfd);
        exit(DOS_FAIL);
    }

    lError = listen(lSockfd, PTS_LISTEN_COUNT);
    if (lError != 0)
    {
        perror("web service listen");
        close(lSockfd);
        exit(DOS_FAIL);
    }

    return lSockfd;
}


BOOL pts_get_cookie(S8 *pcRequest, U8 *pucCookie)
{
    S8 *pcStr1 = NULL;
    S8 *pcStr2 = NULL;

    pcStr1 = strstr(pcRequest, "Cookie");
    if (NULL == pcStr1)
    {
        return DOS_FALSE;
    }

    pcStr2 = strchr(pcStr1,'=');
    if (NULL == pcStr2)
    {
        return DOS_FALSE;
    }

    memcpy(pucCookie, pcStr2+1, PT_IP_SIZE);

    return DOS_TRUE;
}


BOOL pts_is_ipcc_id(S8* pcUrl)
{
    /*ipcc id规格未定，暂时这样写*/
    if(6 == strlen(pcUrl))
    {
        return DOS_TRUE;
    }
    return DOS_FALSE;
}


BOOL pts_is_http_head(S8 *pcRequest)
{
    if (NULL == pcRequest)
    {
        return DOS_FALSE;
    }

    if (NULL != strstr(pcRequest,"HTTP/1.1"))
    {
        return DOS_TRUE;
    }
    return DOS_FALSE;
}

BOOL pts_is_http_end(S8 *pcRequest)
{
    if (NULL == pcRequest)
    {
        return DOS_FALSE;
    }

    if (NULL != strstr(pcRequest,"\r\n\r\n"))
    {
        return DOS_TRUE;
    }
    return DOS_FALSE;
}

S32 pts_create_cclist_html_callback(void *para, S32 n_column, S8 **column_value, S8 **column_name)
{
   S8 *pcHtml = (S8 *)para;

   if (NULL == column_value[1])
   {
        sprintf(pcHtml+strlen(pcHtml),
            "<input type=\"button\" id=\"button_%s\" value=\"设置别名\"  \
            onclick=\"set_alias(%s)\"/><a id=\"a_%s\" href=\"%s\">%s<a/> ?\\
            <input type=\"input\" value=\"\" id=\"alise_%s\"  style=\"display:none;\" /> \
            <input type=\"button\" id=\"sure_%s\" value=\"确定\" style=\"display:none;\" onclick=\"make_sure(%s)\"/><br/>"
            ,column_value[0],column_value[0],column_value[0],column_value[0], column_value[0], column_value[0], column_value[0], column_value[0]);
   }
   else
   {
       sprintf(pcHtml+strlen(pcHtml),
            "<input type=\"button\" id=\"button_%s\" value=\"更改别名\"  \
            onclick=\"set_alias(%s)\"/><a id=\"a_%s\" href=\"%s\">%s<a/>  \
            <input type=\"input\" value=\"\" id=\"alise_%s\"  style=\"display:none;\" /><br/>"
            ,column_value[0],column_value[0],column_value[0],column_value[0], column_value[1], column_value[0]);
   }
    return DOS_SUCC;
}


BOOL pts_deal_with_http_head(S8 *pcRequest, U32 ulConnfd, U32 ulStreamID, U8* pcIpccId, S32 lReqLen)
{
    if (NULL == pcRequest || NULL == pcIpccId)
    {
        alert("pts_deal_with_http_head : arg null");
        return DOS_FALSE;
    }
	
    S8  szUrl[PTS_URL_SIZE] = {0};
    S8  *pcStr1   = NULL;
    S8  *pcStr2   = NULL;
    U8  *pcCookie = NULL;
    S8  *pacCClist_html = NULL;
    S8  achSql[PTS_SQL_STR_SIZE] = {0};
	BOOL bIsGetID = DOS_FALSE;
	S8 pcListBuf[PTS_DATA_BUFF_512] = {0};
	S32 lFd = 0;
	S32 lReadLen = 0;

    sscanf(pcRequest, "%*s /%[^ ]", szUrl);
    alert("url=%s", szUrl);

    if (szUrl[0] == '\0')
    {
        /*请求IPCC列表*/
        memset(pcListBuf, 0, PTS_DATA_BUFF_512);
		/*应该使用数据库内表信息，组成，现在正在调试，用的固定页面*/
        lFd = open("/mnt/hgfs/share/sdk/src/service/pts/index.html", O_RDONLY);
        if (lFd < 0 )	
        {
            perror("open");
            exit(DOS_FAIL);  
        }
		send(ulConnfd, HTTP_SUCCESS, strlen(HTTP_SUCCESS), 0);
		while (1)
		{
			lReadLen = read(lFd, pcListBuf, PTS_DATA_BUFF_512);
	        if (lReadLen < 0)
	        {
	            alert("%s %d read fail\n", __FILE__, __LINE__);
	            exit(DOS_FAIL);
	        }
			else if (lReadLen == 0)
			{
				break;
			}
			send(ulConnfd, pcListBuf, lReadLen, 0);
		}
       
        close(ulConnfd);
        close(lFd);
        FD_CLR(ulConnfd, &m_ReadFds);
        m_stClinetCBList = pts_clinetCB_delete_by_sockfd(m_stClinetCBList, ulConnfd);
    }
	else if (!strcmp(szUrl, "pt.js"))
    {
        /*IPCC列表页面的js文件*/
        memset(pcListBuf, 0, PTS_DATA_BUFF_512);

        lFd = open("/mnt/hgfs/share/sdk/src/service/pts/pt.js", O_RDONLY);
        if (lFd < 0 )	
        {
            alert("%s %d open fail\n", __FILE__, __LINE__);
            exit(DOS_FAIL);  
        }
		send(ulConnfd, HTTP_SUCCESS, strlen(HTTP_SUCCESS), 0);
		while (1)
		{
			lReadLen = read(lFd, pcListBuf, PTS_DATA_BUFF_512);
	        if (lReadLen < 0)
	        {
	            alert("%s %d read fail\n", __FILE__, __LINE__);
	            exit(DOS_FAIL);
	        }
			else if (lReadLen == 0)
			{
				break;
			}
			send(ulConnfd, pcListBuf, lReadLen, 0);
		}
       
        close(ulConnfd);
        close(lFd);
        FD_CLR(ulConnfd, &m_ReadFds);
        m_stClinetCBList = pts_clinetCB_delete_by_sockfd(m_stClinetCBList, ulConnfd);
    }
	else if (!strncmp(szUrl, "cgi-bin/pt.cgi",13))
    {
        /*更改备注名*/

		/*更新数据库*/
		 sprintf(achSql, "update ipcc_alias set register=1 where id='%s';", pstMsgDes->aucID);
         dos_sqlite3_exec(m_stMySqlite, achSql);
		
		/*返回结果*/
   		strcpy(pcListBuf,"1");
        send(ulConnfd, HTTP_SUCCESS, strlen(HTTP_SUCCESS), 0);
		send(ulConnfd, pcListBuf, strlen(pcListBuf), 0);
        close(ulConnfd);
        FD_CLR(ulConnfd, &m_ReadFds);
        m_stClinetCBList = pts_clinetCB_delete_by_sockfd(m_stClinetCBList, ulConnfd);
    }
    else if (pts_is_ipcc_id(szUrl))
    {
        /*第一次请求*/
		memcpy(pcIpccId, szUrl, IPCC_ID_LEN);
        pts_amc_send_msg(pcIpccId, PT_DATA_WEB, ulStreamID, INDEX_HTML, strlen(INDEX_HTML), DOS_FALSE);
		bIsGetID = DOS_TRUE;
    }
    else
    {
        pcCookie = (U8 *)malloc(PT_IP_SIZE);
        if (NULL == pcCookie)
        {
            perror("malloc");
            close(ulConnfd);
            FD_CLR(ulConnfd, &m_ReadFds);
            m_stClinetCBList = pts_clinetCB_delete_by_sockfd(m_stClinetCBList, ulConnfd);
            return bIsGetID;
        }

        if (pts_get_cookie(pcRequest, pcCookie))  /*应该传入pcCookie的大小*/
        {
            alert("cookie=%s", pcCookie);
            /* 去掉请求中的cookie */
            pcStr1 = strstr(pcRequest, "Cookie");
            /* pts_get_cookie已经进行了判断 */
            pcStr2 = strchr(pcStr1, '\n') + 1;
            pcStr1[0] = '\0';
            strcat(pcRequest, pcStr2);
            pts_amc_send_msg(pcCookie, PT_DATA_WEB, ulStreamID, pcRequest, lReqLen, DOS_FALSE);

			memcpy(pcIpccId, pcCookie, IPCC_ID_LEN);
			bIsGetID = DOS_TRUE;
        }
        else
        {
            alert("not get cookie");
            close(ulConnfd);
            FD_CLR(ulConnfd, &m_ReadFds);
            m_stClinetCBList = pts_clinetCB_delete_by_sockfd(m_stClinetCBList, ulConnfd);
        }
        free(pcCookie);
    }

    return bIsGetID;
}

void pts_deal_with_web_request(S8 *pcRequest, U32 ulConnfd, S32 lReqLen)
{
    PTS_CLIENT_CB_ST* pstHttpHead   = NULL;
    S8 szHeadBuf[PT_SEND_DATA_SIZE] = "";
	BOOL bIsGetID = DOS_FALSE;
    U8 *pcIpccId = (U8 *)malloc(IPCC_ID_LEN);
    if (NULL == pcIpccId)
    {
        perror("malloc");
        /*?*/
        return;
    }

    memset(pcIpccId, 0, IPCC_ID_LEN);
    pstHttpHead = pts_clinetCB_search_by_sockfd(m_stClinetCBList, ulConnfd);
    if (NULL == pstHttpHead)
    {
        return;
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
                    memcpy(pstHttpHead->aucId, pcIpccId, IPCC_ID_LEN);
                }
            }
            else
            {
                pstHttpHead->pcRequestHead = (S8 *)malloc(PTS_DATA_BUFF_512);
                if (NULL == pstHttpHead->pcRequestHead)
                {
                    perror("malloc");
                    /*怎么处理??*/
                }
                else
                {
                    strcpy(pstHttpHead->pcRequestHead, pcRequest);
                    pstHttpHead->lReqLen = lReqLen;
                }
            }
        }
        else
        {
            pts_amc_send_msg(pstHttpHead->aucId, PT_DATA_WEB, pstHttpHead->ulStreamID, pcRequest, lReqLen, DOS_FALSE);
        }
    }
    else
    {
        if (pts_is_http_end(pcRequest))
        {
            strcpy(szHeadBuf, pstHttpHead->pcRequestHead);
            strcat(szHeadBuf, pcRequest);
            bIsGetID = pts_deal_with_http_head(szHeadBuf, ulConnfd, pstHttpHead->ulStreamID, pcIpccId, pstHttpHead->lReqLen+lReqLen);
            if (bIsGetID)
            {
				memcpy(pstHttpHead->aucId, pcIpccId, IPCC_ID_LEN);
            }
            free(pstHttpHead->pcRequestHead);
            pstHttpHead->eSaveHeadFlag = DOS_FALSE;
        }
        else
        {
            strcat(pstHttpHead->pcRequestHead,pcRequest);
            pstHttpHead->lReqLen += lReqLen;
        }
    }

    free(pcIpccId);

    return;
}


void *pts_recv_msg_from_client(void *arg)
{
    S32 lSocket = 0;
    S32 lConnFd = 0;
    S32 MaxFdp  = 0;
    S32 i       = 0;
    S32 lNbyte  = 0;
    fd_set ReadFdsCpio;
    S8  szRecvBuf[PT_RECV_DATA_SIZE] = {0};
    struct sockaddr_in stClientAddr;
    socklen_t ulCliaddrLen = sizeof(stClientAddr);

    lSocket = pts_create_tcp_socket(g_stServMsg.usWebServPort);
    FD_ZERO(&m_ReadFds);
    FD_SET(lSocket, &m_ReadFds);
    MaxFdp = lSocket;

    while (1)
    {
        ReadFdsCpio = m_ReadFds;

        if (select(MaxFdp + 1, &ReadFdsCpio, NULL, NULL, NULL) < 0)
        {
            perror("pts_recv_msg_from_client : fail to select");
            exit(DOS_FAIL);
        }

        for (i=0; i<=MaxFdp; i++)
        {
            if (FD_ISSET(i, &ReadFdsCpio))
            {
                if (i == lSocket)
                {
                    alert("new connect from explorer to web service");
                    if ((lConnFd = accept(i, (struct sockaddr*)&stClientAddr, &ulCliaddrLen)) < 0)
                    {
                        perror("pts_recv_msg_from_client : accept");
                        exit(DOS_FAIL);
                    }
                    FD_SET(lConnFd, &m_ReadFds);
                    MaxFdp = MaxFdp > lConnFd ? MaxFdp : lConnFd;
                    m_stClinetCBList = pts_clinetCB_insert(m_stClinetCBList, lConnFd, stClientAddr);
                }
                else
                {
                    memset(szRecvBuf, 0, PT_RECV_DATA_SIZE);
                    lNbyte = recv(i, szRecvBuf, PT_RECV_DATA_SIZE, 0);
                    if (lNbyte == 0)
                    {
                          alert("pts_recv_msg_from_client : socket closed");
                          FD_CLR(i, &m_ReadFds);
                    }
                    else if (lNbyte < 0)
                    {
                        alert("pts_recv_msg_from_client : fail to recv");
                        exit(DOS_FAIL);
                    }
                    else
                    {
                        pts_deal_with_web_request(szRecvBuf, i, lNbyte);
                    }
                }
            }
        }
    }
    return NULL;
}


void *pts_send_msg2client(void *arg)
{
	U8 				 szID[IPCC_ID_LEN+1]  				= {0};
	S8 				 *pcSendMsg 						= NULL;
    S8 				 *pcStrchr  						= NULL;
    U32 			 ulArraySub  						= 0;
    list_t           *pstStreamList  					= NULL;
    PT_MSG_ST        *pstMsgDes      					= NULL;
    PT_CC_CB_ST      *pstCCNode      					= NULL;
    PT_STREAM_CB_ST  *pstStreamNode  					= NULL;
    PTS_CLIENT_CB_ST *pstClinetCB    					= NULL;
    PT_DATA_TCP_ST   stRecvDataTcp;
	S8  			 cookie[PTS_COOKIE_SIZE] 			 = {0};
    S8  			 szResponse[PT_DATA_SEND_CACHE_SIZE] = {0};
    S8  			 achSql[PTS_SQL_STR_SIZE]      		 = {0};
    S8  			 acMsgHead[PTS_DATA_BUFF_512]  		 = {0};

    while (1)
    {
        pthread_mutex_lock(&g_mutex_recv);
        pthread_cond_wait(&g_cond_recv, &g_mutex_recv);
        pstCCNode = pts_CClist_search(g_stCCCBRecv, g_uIpccIDRecv);
        if(NULL == pstCCNode)
        {
            alert("pts_send_msg2client : not found IPCC id is %s", g_uIpccIDRecv);
        }
        else
        {
            pstStreamList = pstCCNode->astDataTypes[g_enDataTypeRecv].pstStreamQueHead;
            if (NULL == pstStreamList)
            {
                alert("pts_send_msg2client : not found stream list of type is %d", g_enDataTypeRecv);
            }
            else
            {
                pstStreamNode = pt_stream_queue_serach(pstStreamList, g_ulStreamIDRecv);
                if (NULL == pstStreamNode)
                {
                    alert("pts_send_msg2client : not found stream node of id is %d", g_ulStreamIDRecv);
                }
                else
                {
                    if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
                    {
                        alert("pts_send_msg2client : not found data queue");
                    }
                    else
                    {
                        ulArraySub = (pstStreamNode->ulCurrSeq + 1) & (PT_DATA_RECV_CACHE_SIZE - 1);
                        stRecvDataTcp = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
                        /*发送连续的包*/
                        while (stRecvDataTcp.bIsSaveMsg && stRecvDataTcp.lSeq > pstStreamNode->ulCurrSeq)
                        {
                            memcpy(acMsgHead, stRecvDataTcp.szBuff, sizeof(PT_MSG_ST));
                            pstMsgDes = (PT_MSG_ST *)acMsgHead;
                            pcSendMsg = stRecvDataTcp.szBuff + sizeof(PT_MSG_ST);

                            if (pstMsgDes->enDataType == PT_DATA_CTRL)
                            {
                                /*控制消息*/
                                switch (pstMsgDes->ulStreamID)
                                {
                                    case 2:
                                        /*登陆成功*/
                                        sprintf(achSql,"select count(*) from ipcc_alias where id='%s'", pstMsgDes->aucID);
                                        if (dos_sqlite3_record_is_exist(m_stMySqlite, achSql))  /*判断是否存在*/
                                        {
                                            /*存在，更新IPCC的注册状态*/
                                            alert("pts_send_msg2client : db existed");
                                            sprintf(achSql, "update ipcc_alias set register=1 where id='%s';", pstMsgDes->aucID);
                                            dos_sqlite3_exec(m_stMySqlite, achSql);
                                        }
                                        else
                                        {
                                            /*不存在，添加IPCC到DB*/
                                            alert("pts_send_msg2client : db insert");
											memcpy(szID, pstMsgDes->aucID, IPCC_ID_LEN);
											szID[IPCC_ID_LEN] = '\0';
                                            sprintf(achSql, "insert into ipcc_alias values('%s', NULL, %d, %d, %d);", szID, DOS_TRUE, DOS_TRUE, DOS_TRUE);
                                            dos_sqlite3_exec(m_stMySqlite, achSql);
                                        }
                                        break;
                                    case 3:
                                        /*取消注册*/
                                        break;
                                    default:
                                        break;
                                }

                                break;
                            }
                            else
                            {
                                if (pstMsgDes->ExitNotifyFlag)
                                {
                                    /*响应结束*/
                                    /*释放缓存的资源*/
                                    //pt_delete_stream_resource(pstCCNode, pstStreamNode, g_enDataTypeRecv);
                                    //m_stClinetCBList = pts_clinetCB_delete_by_stream(m_stClinetCBList, pstMsgDes->ulStreamID);
                                    break;
                                }

                                pstClinetCB = pts_clinetCB_search_by_stream(m_stClinetCBList, pstMsgDes->ulStreamID);
                                if(NULL == pstClinetCB)
                                {
                                    /*没有找到stream对应的套接字*/
                                    alert("error not found stockfd");
                                    break;
                                }
                                else
                                {
                                    /*将IPCC ID设置为cookie*/
                                    pcStrchr = strchr(pcSendMsg,'\n');
                                    strncpy(szResponse, pcSendMsg, pcStrchr-pcSendMsg+1);
                                    sprintf(cookie,"Set-Cookie:id=%s;\r\n",pstMsgDes->aucID);
                                    strcat(szResponse,cookie);
                                    strcat(szResponse,pcStrchr+1);
                                    alert("response:%s", szResponse);
                                    send(pstClinetCB->lSocket, szResponse, strlen(szResponse), 0);
                                }

                            }
						    pstStreamNode->ulCurrSeq++;
                            ulArraySub = (pstStreamNode->ulCurrSeq + 1) & (PT_DATA_RECV_CACHE_SIZE - 1);
                            stRecvDataTcp = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
                        }

                    }

                }

			}

        }
        pthread_mutex_unlock(&g_mutex_recv);
    }
}

void pts_init_serv_msg()
{
    struct in_addr stInAddr;

    inet_pton(AF_INET, "192.168.1.251", (void *)&stInAddr);
    g_stServMsg.achChannelServIP[0] = stInAddr.s_addr;
    inet_pton(AF_INET, "192.168.1.251", (void *)&stInAddr);
    g_stServMsg.achWebServIP[0] = stInAddr.s_addr;
    g_stServMsg.usChannelServPort = 8200;
    g_stServMsg.usWebServPort = 8000;

    return;
}


S32 main(S32 argc, S8 *argv[])
{
    S32 lSocket, lRet;
    pthread_t tid1, tid2, tid3, tid4;
    S8 achSql[PTS_SQL_STR_SIZE] = {0};

    pts_init_serv_msg();

    lSocket = pts_create_udp_socket(g_stServMsg.usChannelServPort);

    lRet = dos_sqlite3_create_db(&m_stMySqlite);
    if (lRet)
    {
        return DOS_FAIL;
    }
    sprintf(achSql, "create table ipcc_alias(id varchar(20) not null primary key , name varchar(10), web integer, telnet integer, register integer)");
    lRet = dos_sqlite3_creat_table(m_stMySqlite, achSql);
    if (lRet)
    {
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid1, NULL, pts_send_msg2ipcc, (void *)&lSocket);
    if (lRet)
    {
        alert("Create pthread error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid2, NULL, pts_recv_msg_from_ipcc, (void *)&lSocket);
    if (lRet)
    {
        alert("Create pthread error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid3, NULL, pts_recv_msg_from_client, NULL);
    if (lRet)
    {
        alert("Create pthread error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid4, NULL, pts_send_msg2client, NULL);
    if (lRet)
    {
        alert("Create pthread error!");
        return DOS_FAIL;
    }

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    pthread_join(tid4, NULL);

    lRet = dos_sqlite3_close(m_stMySqlite);
    if (lRet)
    {
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */
