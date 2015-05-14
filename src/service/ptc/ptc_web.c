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
/* include provate header file */
#include <pt/ptc.h>
#include "ptc_msg.h"

PTC_CLIENT_CB_ST g_austClients[PTC_WEB_SOCKET_MAX_COUNT];
const S8 *g_szFifoName = "/tmp/ptc_fifo";
S32 g_lPipeWRFd = -1;
S8 *g_pPackageBuff = NULL;
U32 g_ulPackageLen = 0;
U32 g_ulReceivedLen = 0;
U32 g_usCrc = 0;

S32 ptc_get_socket_by_streamID(U32 ulStreamID)
{
    S32 i = 0;

    for (i=0; i<PTC_WEB_SOCKET_MAX_COUNT; i++)
    {
        if (g_austClients[i].bIsValid && g_austClients[i].ulStreamID == ulStreamID)
        {
            return g_austClients[i].lSocket;
        }
    }

    return DOS_FAIL;
}

S32 ptc_get_streamID_by_socket(S32 lSocket)
{
    S32 i = 0;

    for (i=0; i<PTC_WEB_SOCKET_MAX_COUNT; i++)
    {
        if (g_austClients[i].bIsValid && g_austClients[i].lSocket == lSocket)
        {
            return g_austClients[i].ulStreamID;
        }
    }

    return DOS_FAIL;
}

S32 ptc_add_client(U32 ulStreamID, S32 lSocket)
{
    S32 i = 0;
    //static S32 lCreateSocket = 0;
    //lCreateSocket++;
    //printf("add %d\n", lCreateSocket);
    //printf("add stream %d, socket %d\n", pstStreamNode->ulStreamID, lSockfd);
    for (i=0; i<PTC_WEB_SOCKET_MAX_COUNT; i++)
    {
        if (!g_austClients[i].bIsValid)
        {
            g_austClients[i].bIsValid = DOS_TRUE;
            g_austClients[i].ulStreamID = ulStreamID;
            g_austClients[i].lSocket = lSocket;
            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}

VOID ptc_delete_client(S32 lSocket)
{
    S32 i = 0;
    //static S32 lDelSocket = 0;
    //lDelSocket++;
    //printf("del %d\n", lDelSocket);
    for (i=0; i<PTC_WEB_SOCKET_MAX_COUNT; i++)
    {
        if (g_austClients[i].bIsValid && g_austClients[i].lSocket == lSocket)
        {
            //printf("del stream %d, socket %d\n", g_austClients[i].ulStreamID, lSocket);
            g_austClients[i].bIsValid = DOS_FALSE;
            g_austClients[i].lSocket = -1;
            g_austClients[i].ulStreamID = U32_BUTT;

            return;
        }
    }
}

VOID ptc_delete_client_by_streamID(U32 ulStreamID)
{
    S32 i = 0;
    //static S32 lDelSocket = 0;
    //lDelSocket++;
    //printf("del %d\n", lDelSocket);
    for (i=0; i<PTC_WEB_SOCKET_MAX_COUNT; i++)
    {
        if (g_austClients[i].bIsValid && g_austClients[i].ulStreamID == ulStreamID)
        {
            //printf("del stream %d, socket %d\n", g_austClients[i].ulStreamID, lSocket);
            g_austClients[i].bIsValid = DOS_FALSE;
            if (g_austClients[i].lSocket > 0)
            {
                close(g_austClients[i].lSocket);
            }
            g_austClients[i].lSocket = -1;
            g_austClients[i].ulStreamID = U32_BUTT;

            return;
        }
    }
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
    S8     cRecvBuf[PT_RECV_DATA_SIZE] = {0};
    S32    lRecvLen   = 0;
    U32    i          = 0;
    S32    lResult    = 0;
    struct timeval stTimeVal = {5, 0};
    struct timeval stTimeValCpy;
    fd_set ReadFds;
    U32 ulMaxFd = 0;
    S32 lPipeFd = -1;

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
    while (1)
    {
        stTimeValCpy = stTimeVal;
        FD_ZERO(&ReadFds);
        ulMaxFd = lPipeFd;
        FD_SET(lPipeFd, &ReadFds);
        for (i=0; i<PTC_WEB_SOCKET_MAX_COUNT; i++)
        {
            if (g_austClients[i].bIsValid)
            {
                FD_SET(g_austClients[i].lSocket, &ReadFds);
                ulMaxFd = ulMaxFd > g_austClients[i].lSocket ? ulMaxFd : g_austClients[i].lSocket;
            }
        }

        lResult = select(ulMaxFd + 1, &ReadFds, NULL, NULL, &stTimeValCpy);
        if (lResult < 0)
        {
            perror("fail to select");
            DOS_ASSERT(0);
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
                if (i == lPipeFd)
                {
                    lRecvLen = read(lPipeFd, cRecvBuf, PT_RECV_DATA_SIZE);
                    continue;
                }
                dos_memzero(cRecvBuf, PT_RECV_DATA_SIZE);
                lRecvLen = recv(i, cRecvBuf, PT_RECV_DATA_SIZE, 0);
                pt_logr_debug("ptc recv from web server len : %d, socket : %d", lRecvLen, i);
#if PTC_DEBUG
                U32 k = 0;
                for (k=0; k<lRecvLen; k++)
                {
                    if (k%16 == 0)
                    {
                      pt_logr_debug("\n");
                    }
                    printf("%02x ", (U8)(cRecvBuf[k]));
                }
#endif
                lResult = ptc_get_streamID_by_socket(i);
                if (DOS_FAIL == lResult)
                {
                    pt_logr_debug("not found socket, streamID is %d", i);
                    FD_CLR(i, &ReadFds);
                    close(i);
                    continue;
                }
                printf("recv from web server, stream : %d, socket : %d, len : %d\n", lResult, i, lRecvLen);
                if (lRecvLen <= 0)
                {
                    /* sockfd结束，通知pts */
                    FD_CLR(i, &ReadFds);
                    pt_logr_debug("sockfd end, streamID is %d", lResult);
                    ptc_save_msg_into_cache(PT_DATA_WEB, lResult, "", 0);
                    ptc_delete_client(i);
                    close(i);
                }
                else
                {
                    lResult = ptc_get_streamID_by_socket(i);
                    if(DOS_FAIL == lResult)
                    {
                        pt_logr_info("not found socket, streamID is %d", i);
                    }
                    else
                    {
                        ptc_save_msg_into_cache(PT_DATA_WEB, lResult, cRecvBuf, lRecvLen);
                    }
                }
            } /* end of if */
        } /* end of for */
    } /* end of while(1) */
}

/**
 * 函数：BOOL ptc_upgrade(PT_DATA_TCP_ST *pstRecvDataTcp, PT_STREAM_CB_ST *pstStreamNode)
 * 功能：
 *      1.接收升级包，升级ptc
 * 参数
 * 返回值： DOS_TRUE  : break;
 *          DOS_FALSE : continue;
 */
BOOL ptc_upgrade(PT_DATA_TCP_ST *pstRecvDataTcp, PT_STREAM_CB_ST *pstStreamNode)
{
    LOAD_FILE_TAG *pstUpgrade = NULL;
    FILE *pFileFd = NULL;
    S32 lFd = 0;
    U32 usCrc = 0;

    if (NULL == g_pPackageBuff)
    {
        g_ulReceivedLen = 0;
        pstUpgrade = (LOAD_FILE_TAG *)pstRecvDataTcp->szBuff;
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

        dos_memcpy(g_pPackageBuff+g_ulReceivedLen, pstRecvDataTcp->szBuff, pstRecvDataTcp->ulLen);
        g_ulReceivedLen += pstRecvDataTcp->ulLen;

        return DOS_TRUE;
    }


   dos_memcpy(g_pPackageBuff+g_ulReceivedLen, pstRecvDataTcp->szBuff, pstRecvDataTcp->ulLen);
   g_ulReceivedLen += pstRecvDataTcp->ulLen;

    if (pstRecvDataTcp->ulLen < PT_RECV_DATA_SIZE)
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
                /* ptc 退出程序, 等待升级 */
                exit(0);

            }
            dos_dmem_free(g_pPackageBuff);
            g_pPackageBuff = NULL;
        }
        else
        {
            /* 验证不通过 */
            logr_info("upgrade package error : md5 error");
        }

        ptc_delete_recv_stream_node(pstStreamNode->ulStreamID, PT_DATA_WEB, DOS_FALSE);
        ptc_send_exit_notify_to_pts(PT_DATA_WEB, pstStreamNode->ulStreamID, 0);

        return DOS_FALSE;
    }

    return DOS_TRUE;
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
    list_t            *pstStreamList   = NULL;
    PT_STREAM_CB_ST   *pstStreamNode   = NULL;
    PT_DATA_TCP_ST    stRecvDataTcp;

    if (pstNeedRecvNode->ExitNotifyFlag)
    {
        ptc_delete_client_by_streamID(pstNeedRecvNode->ulStreamID);
        ptc_send_exit_notify_to_pts(PT_DATA_WEB, pstNeedRecvNode->ulStreamID, 2);
        ptc_delete_recv_stream_node(pstNeedRecvNode->ulStreamID, PT_DATA_WEB, DOS_FALSE);
        ptc_delete_send_stream_node(pstNeedRecvNode->ulStreamID, PT_DATA_CMD, DOS_TRUE);

        return;
    }

    pstStreamList = g_pstPtcRecv->astDataTypes[pstNeedRecvNode->enDataType].pstStreamQueHead;
    if (NULL == pstStreamList)
    {
        pt_logr_info("send data to proxy : stream list head is NULL, cann't get data package");
        return;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamList, pstNeedRecvNode->ulStreamID);
    if (NULL == pstStreamNode)
    {
        pt_logr_info("send data to proxy : stream node cann't found : %d", pstNeedRecvNode->ulStreamID);
        return;
    }

    if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
    {
        pt_logr_info("send data to proxy : data queue is NULL");
        return;
    }
    while(1)
    {
        pstStreamNode->lCurrSeq++;
        ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1);
        stRecvDataTcp = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
        if (stRecvDataTcp.lSeq == pstStreamNode->lCurrSeq)
        {
            if (PT_CTRL_PTC_PACKAGE == pstStreamNode->ulStreamID)
            {
                /* PTC 升级 */
                lResult = ptc_upgrade(&stRecvDataTcp, pstStreamNode);
                if (!lResult)
                {
                    break;
                }
                else
                {
                    continue;
                }
            }

            lSockfd = ptc_get_socket_by_streamID(pstStreamNode->ulStreamID);
            if (DOS_FAIL == lSockfd)
            {
                lSockfd = ptc_create_socket_proxy(pstStreamNode->aulServIp, pstStreamNode->usServPort);
                if (lSockfd <= 0)
                {
                    /* 创建socket失败，通知pts结束 */
                    ptc_send_exit_notify_to_pts(PT_DATA_WEB, pstStreamNode->ulStreamID, 1);
                    ptc_delete_recv_stream_node(pstStreamNode->ulStreamID, PT_DATA_WEB, DOS_FALSE);

                    break;
                }
                printf("create socket : %d, stream : %d\n", lSockfd, pstStreamNode->ulStreamID);
                lResult = ptc_add_client(pstStreamNode->ulStreamID, lSockfd);
                if (lResult != DOS_SUCC)
                {
                    /* socket 数量已满 */
                    printf("aocket is full\n");
                    close(lSockfd);
                    ptc_send_exit_notify_to_pts(PT_DATA_WEB, pstStreamNode->ulStreamID, 2);
                    ptc_delete_recv_stream_node(pstStreamNode->ulStreamID, PT_DATA_WEB, DOS_FALSE);

                    break;
                }
                write(g_lPipeWRFd, "s", 1);
            }

            pt_logr_debug("send msg to web server, stream : %d, seq : %d, len : %d", pstStreamNode->ulStreamID, pstStreamNode->lCurrSeq, stRecvDataTcp.ulLen);
            lResult = send(lSockfd, stRecvDataTcp.szBuff, stRecvDataTcp.ulLen, 0);
            //pt_logr_debug("send to web server, len : %d", stRecvDataTcp.ulLen);
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

S32 ptc_printf_web_msg(U32 ulIndex, S32 argc, S8 **argv)
{
    S32 i = 0;
    U32 ulLen = 0;
    S8 szBuff[PT_DATA_BUFF_512] = {0};

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%10s%10s%10s\r\n", "ulStreamID", "lSocket", "bIsValid");
    cli_out_string(ulIndex, szBuff);

    for (i=0; i<PTC_CMD_SOCKET_MAX_COUNT; i++)
    {
        snprintf(szBuff, sizeof(szBuff), "%10d%10d%10d\r\n", g_austClients[i].ulStreamID, g_austClients[i].lSocket, g_austClients[i].bIsValid);
        cli_out_string(ulIndex, szBuff);
    }

    return 0;

}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

