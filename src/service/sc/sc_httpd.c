/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_httpd.c
 *
 *  创建时间: 2014年12月16日10:20:33
 *  作    者: Larry
 *  描    述: 业务控制模块外部接口接收相关处理
 *  修改历史:
 */
#if 0
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
#endif
/* include public header files */
#include <dos.h>
#include <esl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>

/* include private header files */
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_httpd.h"
#include "sc_httpd_def.h"
#include "sc_http_api.h"
#include <dos/dos_config.h>
#include <dos/dos_py.h>

/* define marcos */
#define SC_HTTP_RSP_HEADER_FMT "HTTP/1.1 %s\r\n" \
                                "Server: IPCC Service Control Server/1.1\r\n" \
                                "Content-Type: text/json\r\n" \
                                "Content-Length: %u\r\n\r\n%s"

/* define enums */

/* define structs */

/* global variables */

SC_HTTP_CLIENT_CB_S   *g_pstHTTPClientList[SC_MAX_HTTP_CLIENT_NUM] = { NULL };

SC_HTTPD_CB_ST        *g_pstHTTPDList[SC_MAX_HTTPD_NUM] = { NULL };

pthread_t             g_pthHTTPDThread;

SC_HTTP_RSP_CODE_ST   g_astRspCodeDesc[] =
{
    {SC_HTTP_1XX,                        ""},
    {SC_HTTP_OK,                         "200 OK"},
    {SC_HTTP_300,                        "300 Redirect"},
    {SC_HTTP_302,                        "302 Not Modifyed"},
    {SC_HTTP_FORBIDDEN,                  "403 Forbidden"},
    {SC_HTTP_NOTFOUND,                   "404 Not Found"},
    {SC_HTTP_500,                        "500 Server Error"},
};

const SC_RESULT_NAME_NODE_ST g_astHttpResultNameList[] =
{
    {SC_HTTP_DATA_ERRNO,                 "errno"},           /* 数据项：原因值 */
    {SC_HTTP_DATA_MSG,                   "msg"},             /* 数据项：原因描述 */

    {SC_HTTP_DATA_BUTT,                  ""}
};

/* !!!!!!!!!!!此数组不能使用第一个字段索引!!!!!!!!!!! */
const SC_HTTP_ERRNO_DESC_ST g_astHttpErrNOList[] = {
    {SC_HTTP_ERRNO_SUCC,                 "Successfully."},
    {SC_HTTP_ERRNO_INVALID_USR,          "Invalid User."},
    {SC_HTTP_ERRNO_INVALID_DATA,         "Invalid Data."},
    {SC_HTTP_ERRNO_INVALID_REQUEST,      "Invalid Request."},
    {SC_HTTP_ERRNO_INVALID_CMD,          "Invalid Command."},
    {SC_HTTP_ERRNO_INVALID_PARAM,        "Invalid Parameter."},
    {SC_HTTP_ERRNO_CMD_EXEC_FAIL,        "Command exec failed."},
    {SC_HTTP_ERRNO_SERVER_ERROR,         "Internal Error."},
    {SC_HTTP_ERRNO_SERVER_NOT_READY,     "Server Not ready, please try again."},

    {SC_HTTP_ERRNO_BUTT,                 ""},
};

/**
 * 函数: sc_http_client_get_err_desc
 * 功能: 获取错误码描述信息
 * 参数:
 *     U32 ulErrCode: 错误码
 * 返回值:
 *     成功返回描述信息，失败返回空
 * 示例:
 * 特殊说明:
 */
S8 *sc_http_client_get_err_desc(U32 ulErrCode)
{
    U32 ulIndex;

    SC_TRACE_IN(ulErrCode, 0, 0, 0);

    for (ulIndex=0; ulIndex < sizeof(g_astHttpErrNOList)/sizeof(SC_HTTP_ERRNO_DESC_ST); ulIndex++)
    {
        if (ulErrCode == g_astHttpErrNOList[ulIndex].ulHttpErrNO)
        {
            SC_TRACE_OUT();
            return g_astHttpErrNOList[ulIndex].pszVal;
        }
    }

    SC_TRACE_OUT();
    return NULL;
}


/**
 * 函数: sc_http_client_repaid_rsp
 * 功能: 在没有HTTP客户端控制块时，向HTTP客户端发送响应
 * 参数:
 *     S32 lSocket: 与HTTP客户端连接的socket值
 *     U32 ulRspCode: HTTP的响应码
 *     U32 ulErrCode: 错误码
 * 返回值:
 *     成功返回DOS_SUCC，失败返回DOS_FAIL
 * 示例:
 * 特殊说明:
 */
U32 sc_http_client_repaid_rsp(S32 lSocket, U32 ulRspCode, U32 ulErrCode)
{
    S8 szSendBuff[SC_HTTP_MAX_SEND_BUFF_LEN] = { 0, };
    S8 szDataBuff[SC_HTTP_MAX_SEND_BUFF_LEN] = { 0, };
    U32 ulDataLength, ulRspLen;
    S8 *pszErrDesc = NULL;

    SC_TRACE_IN(ulRspCode, ulErrCode, 0, 0);

    if (lSocket < 0)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (ulRspCode >= SC_HTTP_BUTT)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pszErrDesc = sc_http_client_get_err_desc(ulErrCode);
    ulDataLength = dos_snprintf(szDataBuff, sizeof(szDataBuff)
                    , "{\"%s\":\"0x%08X\", \"%s\":\"%s\"}"
                    , g_astHttpResultNameList[SC_HTTP_DATA_ERRNO].pszName
                    , ulErrCode
                    , g_astHttpResultNameList[SC_HTTP_DATA_MSG].pszName
                    , (NULL == pszErrDesc) ? " " : pszErrDesc);

    ulRspLen = dos_snprintf(szSendBuff, sizeof(szSendBuff)
                    , SC_HTTP_RSP_HEADER_FMT
                    , g_astRspCodeDesc[ulRspCode].pszDesc
                    , ulDataLength
                    , szDataBuff);

    if (send(lSocket, szSendBuff, ulRspLen, 0) <= 0)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/**
 * 函数: sc_http_client_req_check
 * 功能: 检测当前客户端的请求释放可以开始处理
 * 参数:
 *     SC_HTTP_CLIENT_CB_S *pstClient: 客户端控制块
 * 返回值:
 *     成功返回DOS_SUCC，失败返回DOS_FAIL
 * 示例:
 * 特殊说明:
 */
U32 sc_http_client_req_check(SC_HTTP_CLIENT_CB_S *pstClient)
{
    SC_TRACE_IN((U64)pstClient, 0, 0, 0);

    if (!pstClient)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_HTTPD_TRACE(pstClient->ulCurrentSrv, pstClient->ulIndex, pstClient->ulResponseCode, pstClient->ulIndex);

    if (pstClient->lSock <= 0
        || !pstClient->ulValid)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (pstClient->stDataBuff.pszBuff)
    {
        if (dos_strstr(pstClient->stDataBuff.pszBuff, "\r\n"))
        {
            SC_TRACE_OUT();
            return DOS_SUCC;
        }
    }

    SC_TRACE_OUT();
    return DOS_FAIL;
}

/**
 * 函数: sc_http_client_alloc
 * 功能: 申请HTTP客户端
 * 参数:
 *     U32 ulSrvNo : 新客户端属于哪个server
 *     S32 lSoc: 新客户端的socket描述符
 * 返回值:
 *     成功返回DOS_SUCC，失败返回DOS_FAIL
 * 示例:
 * 特殊说明:
 */
U32 sc_http_client_alloc(U32 ulSrvNo, S32 lSock)
{
    U32 ulIndex;

    SC_TRACE_IN(ulSrvNo, lSock, 0, 0);

    if (lSock <= 0)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (ulSrvNo >= SC_MAX_HTTPD_NUM)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    for (ulIndex = 0; ulIndex < SC_MAX_HTTP_CLIENT_NUM; ulIndex++)
    {
        if (DOS_TRUE != g_pstHTTPClientList[ulIndex]->ulValid)
        {
            break;
        }
    }
    if (ulIndex >= SC_MAX_HTTP_CLIENT_NUM)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    g_pstHTTPClientList[ulIndex]->ulValid = DOS_TRUE;
    g_pstHTTPClientList[ulIndex]->ulCurrentSrv = ulSrvNo;
    g_pstHTTPClientList[ulIndex]->lSock = lSock;
    g_pstHTTPClientList[ulIndex]->ulResponseCode = 0;
    g_pstHTTPClientList[ulIndex]->ulErrCode = 0;
    g_pstHTTPClientList[ulIndex]->stDataBuff.pszBuff = NULL;
    g_pstHTTPClientList[ulIndex]->stDataBuff.ulLength = 0;
    dos_list_init(&g_pstHTTPClientList[ulIndex]->stResponseDataList);

    SC_TRACE_OUT();
    return DOS_SUCC;
}


/**
 * 函数: sc_http_client_free
 * 功能: 释放HTTP客户端
 * 参数:
 *     SC_HTTP_CLIENT_CB_S *pstClient: 客户端控制块
 * 返回值:
 *     成功返回DOS_SUCC，失败返回DOS_FAIL
 * 示例:
 * 特殊说明:
 */
U32 sc_http_client_free(SC_HTTP_CLIENT_CB_S *pstClient)
{
    SC_TRACE_IN((U64)pstClient, 0, 0, 0);

    if (!pstClient)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_HTTPD_TRACE(pstClient->ulCurrentSrv, pstClient->ulIndex, pstClient->ulResponseCode, pstClient->ulIndex);

    if (!pstClient->ulValid)
    {
        DOS_ASSERT(0);
    }

    if (pstClient->stDataBuff.pszBuff)
    {
        dos_dmem_free(pstClient->stDataBuff.pszBuff);
        pstClient->stDataBuff.pszBuff = NULL;
    }

    if (pstClient->lSock > 0)
    {
        close(pstClient->lSock);
        pstClient->lSock = -1;
    }

    pstClient->lSock = -1;
    pstClient->ulValid = DOS_FALSE;
    pstClient->ulCurrentSrv = U32_BUTT;
    pstClient->ulResponseCode = 0;
    pstClient->ulErrCode =  0;
    pstClient->stDataBuff.pszBuff = NULL;
    pstClient->stDataBuff.ulLength = 0;

    //@TODO 需要释放链表

    SC_TRACE_OUT();
    return DOS_SUCC;
}


/**
 * 函数: sc_http_client_recv_data
 * 功能: HTTP客户端接收请求数据处理
 * 参数:
 *     SC_HTTP_CLIENT_CB_S *pstClient: 客户端控制块
 *     S8 *pszBuff: 收到数据的缓存
 *     U32 ulLength: 收到数据的长度
 * 返回值:
 *     成功返回DOS_SUCC，失败返回DOS_FAIL
 * 示例:
 * 特殊说明:
 *    当发现所有的WEB Server都不可用是会进入重新初始化状态
 */
U32 sc_http_client_recv_data(SC_HTTP_CLIENT_CB_S *pstClient, S8 *pszBuff, U32 ulLength)
{
    S8 *pszNewBuff = NULL;

    SC_TRACE_IN((U64)pstClient, (U64)pszBuff, ulLength, 0);

    if (!pstClient)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_HTTPD_TRACE(pstClient->ulCurrentSrv, pstClient->ulIndex, pstClient->ulResponseCode, pstClient->ulIndex);

    if (!pszBuff || 0 == ulLength)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* 空间不够了，重新申请内存 */
    pszNewBuff = (S8 *)dos_dmem_alloc(pstClient->stDataBuff.ulLength + ulLength + 1);
    if(!pszNewBuff)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_memcpy(pszNewBuff, pstClient->stDataBuff.pszBuff, pstClient->stDataBuff.ulLength);

    if (pstClient->stDataBuff.pszBuff)
    {
        dos_dmem_free(pstClient->stDataBuff.pszBuff);
    }
    pstClient->stDataBuff.pszBuff = pszNewBuff;

    dos_memcpy(pstClient->stDataBuff.pszBuff + pstClient->stDataBuff.ulLength, pszBuff, ulLength);
    pstClient->stDataBuff.ulLength += ulLength;
    pstClient->stDataBuff.pszBuff[pstClient->stDataBuff.ulLength] = 0;

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/**
 * 函数: sc_httpd_init
 * 功能: 初始化话业务控制模块中的httpd模块
 * 参数:
 * 返回值:
 *      成功返回SWITCH_STATUS_SUCCESS，失败返回响应错误码
 * 示例:
 * 特殊说明:
 */
U32 sc_httpd_init()
{
    U8 *aucCBMem = NULL;
    U32 ulIndex;

    SC_TRACE_IN(0, 0, 0, 0);

    aucCBMem = dos_dmem_alloc(sizeof(SC_HTTP_CLIENT_CB_S)* SC_MAX_HTTP_CLIENT_NUM);
    if (NULL == aucCBMem)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_memzero(aucCBMem, sizeof(SC_HTTP_CLIENT_CB_S)* SC_MAX_HTTP_CLIENT_NUM);

    for (ulIndex = 0; ulIndex < SC_MAX_HTTP_CLIENT_NUM; ulIndex++)
    {
        g_pstHTTPClientList[ulIndex] = (SC_HTTP_CLIENT_CB_S *)(aucCBMem + sizeof(SC_HTTP_CLIENT_CB_S) * ulIndex);
        g_pstHTTPClientList[ulIndex]->ulIndex = ulIndex;
        g_pstHTTPClientList[ulIndex]->ulCurrentSrv = U32_BUTT;
        g_pstHTTPClientList[ulIndex]->lSock = -1;
        g_pstHTTPClientList[ulIndex]->ulResponseCode = 0;
        dos_list_init(&g_pstHTTPClientList[ulIndex]->stParamList);
        dos_list_init(&g_pstHTTPClientList[ulIndex]->stResponseDataList);
    }

    aucCBMem = dos_dmem_alloc(sizeof(SC_HTTPD_CB_ST) * SC_MAX_HTTPD_NUM);
    if (NULL == aucCBMem)
    {
        dos_dmem_free(g_pstHTTPClientList[0]);
        g_pstHTTPClientList[0] = NULL;

        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_memzero(aucCBMem, sizeof(SC_HTTPD_CB_ST) * SC_MAX_HTTPD_NUM);
    for (ulIndex = 0; ulIndex < SC_MAX_HTTPD_NUM; ulIndex++)
    {
        g_pstHTTPDList[ulIndex] = (SC_HTTPD_CB_ST *)(aucCBMem + sizeof(SC_HTTPD_CB_ST) * ulIndex);
        g_pstHTTPDList[ulIndex]->ulIndex = ulIndex;
        g_pstHTTPDList[ulIndex]->lListenSocket = -1;
        g_pstHTTPDList[ulIndex]->ulReqCnt = 0;
        g_pstHTTPDList[ulIndex]->ulStatus = 0;
    }

    g_pstHTTPDList[0]->ulValid = DOS_TRUE;
    dos_strtoipaddr("127.0.0.1", &(g_pstHTTPDList[0]->aulIPAddr[0]));
    g_pstHTTPDList[0]->usPort = 18250;

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/**
 * 函数: sc_httpd_srv_init
 * 功能: 初始化话业务控制模块中的各个服务器
 * 参数:
 * 返回值:
 *      成功返回SWITCH_STATUS_SUCCESS，失败返回响应错误码
 * 示例:
 * 特殊说明:
 */
U32 sc_httpd_srv_init()
{
    U32 ulIndex, ulClientCnt;
    struct sockaddr_in stListenAddr;

    SC_TRACE_IN(0, 0, 0, 0);

    /* 初始化服务器 */
    for (ulIndex = 0; ulIndex < SC_MAX_HTTPD_NUM; ulIndex++)
    {
        if (DOS_TRUE != g_pstHTTPDList[ulIndex]->ulValid)
        {
            continue;
        }

        if (g_pstHTTPDList[ulIndex]->lListenSocket > 0)
        {
            continue;
        }

        g_pstHTTPDList[ulIndex]->lListenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (g_pstHTTPDList[ulIndex]->lListenSocket < 0)
        {
            sc_logr_error(SC_HTTPD, "%s", "Create http server socket fail.");
            g_pstHTTPDList[ulIndex]->lListenSocket = -1;
            continue;
        }

        memset(&stListenAddr, 0, sizeof(stListenAddr));
        stListenAddr.sin_family = AF_INET;
        stListenAddr.sin_addr.s_addr = dos_htonl(g_pstHTTPDList[ulIndex]->aulIPAddr[0]);
        stListenAddr.sin_port = dos_htons(g_pstHTTPDList[ulIndex]->usPort);
        if (bind(g_pstHTTPDList[ulIndex]->lListenSocket, (struct sockaddr *)&stListenAddr, sizeof(stListenAddr)) < 0)
        {
            sc_logr_error(SC_HTTPD, "%s", "HTTP server bind IP/Address fail.");
            close(g_pstHTTPDList[ulIndex]->lListenSocket);
            g_pstHTTPDList[ulIndex]->lListenSocket = -1;
            continue;
        }

        if (listen(g_pstHTTPDList[ulIndex]->lListenSocket, 5) < 0)
        {
            sc_logr_error(SC_HTTPD, "Linsten on port %d fail", g_pstHTTPDList[ulIndex]->usPort);
            close(g_pstHTTPDList[ulIndex]->lListenSocket);
            g_pstHTTPDList[ulIndex]->lListenSocket = -1;
            continue;
        }

        sc_logr_info(SC_HTTPD, "HTTP Server create OK. On port %u.", g_pstHTTPDList[ulIndex]->usPort);
    }

    /* 如果所有服务器都不好使，就认为错误了 */
    for (ulIndex = 0, ulClientCnt = 0; ulIndex < SC_MAX_HTTPD_NUM; ulIndex++)
    {
        if (DOS_TRUE == g_pstHTTPDList[ulIndex]->ulValid
            && g_pstHTTPDList[ulIndex]->lListenSocket > 0)
        {
            ulClientCnt++;
        }
    }
    if (!ulClientCnt)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/**
 * 函数: sc_httpd_active_srv_cnt
 * 功能: 获取当前处于激活状态的服务器个数
 * 参数:
 * 返回值:
 * 示例:
 * 特殊说明:
 *    当发现所有的WEB Server都不可用是会进入重新初始化状态
 *    线程函数里面被调用，不要trace
 */
U32  sc_httpd_active_srv_cnt()
{
    U32 ulIndex, ulSrvCnt;

    //SC_TRACE_IN(g_pstHTTPDList[0], 0, 0, 0);

    for (ulIndex = 0, ulSrvCnt = 0; ulIndex < SC_MAX_HTTPD_NUM; ulIndex++)
    {
        if (DOS_TRUE == g_pstHTTPDList[ulIndex]->ulValid
            && g_pstHTTPDList[ulIndex]->lListenSocket > 0)
        {
            ulSrvCnt++;
        }
    }

    //SC_TRACE_OUT();
    return ulSrvCnt;
}


/**
 * 函数: sc_httpd_runtime
 * 功能: HTTP Server的线程启动函数
 * 参数:
 * 返回值:
 * 示例:
 * 特殊说明:
 *    当发现所有的WEB Server都不可用是会进入重新初始化状态
 */
VOID* sc_httpd_runtime(VOID *ptr)
{
    fd_set               stFDSet;
    struct timeval       stTimeVal;
    struct sockaddr_in   stClientAddr;
    S32                  lMacSocket, lClientSock, lRet;
    U32                  ulIndex, ulClientAddrLen;
    S8                   szRecvBuff[SC_HTTP_MAX_RECV_BUFF_LEN] = {0, };

    SC_TRACE_IN(0, 0, 0, 0);

    while (1)
    {
        if (sc_httpd_active_srv_cnt() <= 0)
        {
            /* 如果所有服务器都宕机了，就进入重连阶段, 每隔2秒钟重连一次 */
            if (sc_httpd_srv_init() != DOS_SUCC)
            {
                sc_logr_notice(SC_HTTPD, "%s", "All the http server lost, will be retry after 2 seconds.");

                sleep(2);

                continue;
            }
        }

        lMacSocket = 0;
        FD_ZERO(&stFDSet);
        for (ulIndex = 0; ulIndex < SC_MAX_HTTPD_NUM; ulIndex++)
        {
            if (DOS_TRUE == g_pstHTTPDList[ulIndex]->ulValid
                && g_pstHTTPDList[ulIndex]->lListenSocket > 0)
            {
                FD_SET(g_pstHTTPDList[ulIndex]->lListenSocket, &stFDSet);
                if (g_pstHTTPDList[ulIndex]->lListenSocket > lMacSocket)
                {
                    lMacSocket = g_pstHTTPDList[ulIndex]->lListenSocket;
                }
#if 0
                SC_HTTPD_TRACE(g_pstHTTPDList[ulIndex]->ulIndex, 0, 0, 0);
#endif
            }
        }

        for (ulIndex = 0; ulIndex < SC_MAX_HTTP_CLIENT_NUM; ulIndex++)
        {
            if (DOS_TRUE == g_pstHTTPClientList[ulIndex]->ulValid
                && g_pstHTTPClientList[ulIndex]->lSock > 0)
            {
                FD_SET(g_pstHTTPClientList[ulIndex]->lSock, &stFDSet);
                if (g_pstHTTPClientList[ulIndex]->lSock > lMacSocket)
                {
                    lMacSocket = g_pstHTTPClientList[ulIndex]->lSock;
                }
#if 0
                SC_HTTPD_TRACE(g_pstHTTPClientList[ulIndex]->ulCurrentSrv
                                , g_pstHTTPClientList[ulIndex]->ulIndex
                                , g_pstHTTPClientList[ulIndex]->ulResponseCode
                                , g_pstHTTPClientList[ulIndex]->ulErrCode);
#endif
            }
        }

        stTimeVal.tv_sec = 1;
        stTimeVal.tv_usec = 0;
        lRet = select(lMacSocket + 1, &stFDSet, NULL, NULL, &stTimeVal);
        if (lRet < 0)
        {
            DOS_ASSERT(0);
            continue;
        }
        else if (0 == lRet)
        {
            continue;
        }

        /* 看看是不是服务器有新客户端连入 */
        for (ulIndex = 0; ulIndex < SC_MAX_HTTPD_NUM; ulIndex++)
        {
            if (DOS_TRUE == g_pstHTTPDList[ulIndex]->ulValid
                && g_pstHTTPDList[ulIndex]->lListenSocket > 0
                && FD_ISSET(g_pstHTTPDList[ulIndex]->lListenSocket, &stFDSet))
            {
                lClientSock = accept(g_pstHTTPDList[ulIndex]->lListenSocket, (struct sockaddr *)&stClientAddr, &ulClientAddrLen);
                if (lClientSock < 0)
                {
                    close(g_pstHTTPDList[ulIndex]->lListenSocket);
                    g_pstHTTPDList[ulIndex]->lListenSocket = -1;
                    logr_error("%s:Line %d: errno is %d, err msg:%s"
                                , dos_get_filename(__FILE__), __LINE__, errno, strerror(errno));
                    continue;
                }

                if (!g_blSCInitOK)
                {
                    sc_http_client_repaid_rsp(lClientSock, SC_HTTP_500, SC_HTTP_ERRNO_SERVER_NOT_READY);
                    close(lClientSock);
                    continue;
                }

                g_pstHTTPDList[ulIndex]->ulReqCnt++;

                if (sc_http_client_alloc(ulIndex, lClientSock) != DOS_SUCC)
                {
                    sc_http_client_repaid_rsp(lClientSock, SC_HTTP_500, SC_HTTP_ERRNO_SERVER_ERROR);
                    close(lClientSock);
                }
            }
        }

        /* 也有可能是客户端的读事件 */
        for (ulIndex = 0; ulIndex < SC_MAX_HTTP_CLIENT_NUM; ulIndex++)
        {
            if (DOS_TRUE == g_pstHTTPClientList[ulIndex]->ulValid
                && g_pstHTTPClientList[ulIndex]->lSock > 0
                && FD_ISSET(g_pstHTTPClientList[ulIndex]->lSock, &stFDSet))
            {
                lRet = recv(g_pstHTTPClientList[ulIndex]->lSock
                            , szRecvBuff
                            , sizeof(szRecvBuff)
                            , 0);
                if (lRet <= 0)
                {
                    sc_http_client_free(g_pstHTTPClientList[ulIndex]);
                    continue;
                }

                /* 处理缓存 */
                if (sc_http_client_recv_data(g_pstHTTPClientList[ulIndex], szRecvBuff, lRet) != DOS_SUCC)
                {
                    sc_http_client_free(g_pstHTTPClientList[ulIndex]);
                    continue;
                }

                /* 检测是否可以处理该请求 */
                if (sc_http_client_req_check(g_pstHTTPClientList[ulIndex]) == DOS_SUCC)
                {

                    sc_http_api_process(g_pstHTTPClientList[ulIndex]);

                    sc_http_client_repaid_rsp(g_pstHTTPClientList[ulIndex]->lSock
                                        , g_pstHTTPClientList[ulIndex]->ulResponseCode
                                        , g_pstHTTPClientList[ulIndex]->ulErrCode);

                    close(g_pstHTTPClientList[ulIndex]->lSock);
                    g_pstHTTPClientList[ulIndex]->lSock= -1;
                    sc_http_client_free(g_pstHTTPClientList[ulIndex]);
                }
            }
        }
    }


    SC_TRACE_OUT();
}

/**
 * 函数: sc_httpd_runtime
 * 功能: HTTP Server的线程函数
 * 参数:
 * 返回值:
 * 示例:
 * 特殊说明:
 */
U32 sc_httpd_start()
{
    if (pthread_create(&g_pthHTTPDThread, NULL, sc_httpd_runtime, NULL) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 函数: sc_httpd_shutdown
 * 功能: HTTP Server对出函数
 * 参数:
 * 返回值:
 * 示例:
 * 特殊说明:
 *    调用该函数之后，HTTP Server不会立即退出，需要等待所有任务
 */
U32 sc_httpd_shutdown()
{
    U32 ulIndex;

    if (g_pstHTTPClientList[0])
    {
        dos_dmem_free(g_pstHTTPClientList[0]);
    }

    if (g_pstHTTPDList[0])
    {
        for (ulIndex = 0; ulIndex < SC_MAX_HTTPD_NUM; ulIndex++)
        {
            if (g_pstHTTPDList[ulIndex]->lListenSocket > 0)
            {
                close(g_pstHTTPDList[ulIndex]->lListenSocket);
                g_pstHTTPDList[ulIndex]->lListenSocket = -1;
            }
        }

        dos_dmem_free(g_pstHTTPDList[0]);
    }

    return DOS_SUCC;
}


#if 0
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
