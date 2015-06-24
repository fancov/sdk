/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_data_syn.c
 *
 *  创建时间: 2015年6月18日15:25:18
 *  作    者: Larry
 *  描    述: 处理FS核心发过来的各种事件
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <esl.h>
#include <sys/time.h>
#include <pthread.h>
#include <bs_pub.h>
#include <libcurl/curl.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"
#include "sc_ep.h"
#include "sc_acd_def.h"
#include "sc_http_api.h"


#define SC_DATA_SYN_SOCK_PATH "/var/www/html/temp/sc-syn.sock"
#define SC_DATA_SYN_BUFF_LEN  512

pthread_mutex_t  g_mutexDataSyn = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t   g_condDataSyn  = PTHREAD_COND_INITIALIZER;
pthread_t        g_pthDataSyn;
pthread_t        g_pthDataSynProc;
BOOL             g_blSCDataSybFlag = DOS_FALSE;
BOOL             g_blConnected  = DOS_FALSE;
DLL_S            g_stSCTMPTblRecord;

extern DB_HANDLE_ST         *g_pstSCDBHandle;
extern SC_HTTP_REQ_REG_TABLE_SC g_pstHttpReqRegTable[];


static S32 sc_walk_tmp_tbl_cb(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    DLL_NODE_S *pstDLLNode;
    S8         *pstData;

    if (DOS_ADDR_INVALID(aszData) || DOS_ADDR_INVALID(aszData[2]) || aszData[2][0] == '\0')
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstDLLNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    DLL_Init_Node(pstDLLNode);

    pstData = dos_dmem_alloc(SC_DATA_SYN_BUFF_LEN);
    if (DOS_ADDR_INVALID(pstData))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstDLLNode);
        pstDLLNode = NULL;
        return DOS_FAIL;
    }

    dos_strncpy(pstData, aszData[2], SC_DATA_SYN_BUFF_LEN);
    pstData[SC_DATA_SYN_BUFF_LEN - 1] = '\0';

    pstDLLNode->pHandle = pstData;
    DLL_Add(&g_stSCTMPTblRecord, pstDLLNode);

    return DOS_SUCC;
}

U32 sc_walk_tmp_tbl()
{
    S8 szQuery[256] = {0, };

    if (!g_pstSCDBHandle || g_pstSCDBHandle->ulDBStatus != DB_STATE_CONNECTED)
    {
        DOS_ASSERT(0);
        sc_logr_emerg(SC_SYN, "%s", "DB Has been down or not init.");
        return DOS_FAIL;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT id,ctime,json_fields FROM tmp_fs_modify;");

    if (db_query(g_pstSCDBHandle, szQuery, sc_walk_tmp_tbl_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        sc_logr_emerg(SC_SYN, "%s", "DB query failed.(%s)", szQuery);
        return DOS_FAIL;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "delete from tmp_tbl_modify;");
    db_transaction_begin(g_pstSCDBHandle);
    if (db_query(g_pstSCDBHandle, szQuery, NULL, NULL, NULL) != DB_ERR_SUCC)
    {
        sc_logr_emerg(SC_SYN, "%s", "DB query failed. (%s)", szQuery);
        db_transaction_rollback(g_pstSCDBHandle);
        return DOS_FAIL;
    }
    db_transaction_commit(g_pstSCDBHandle);

    return DOS_SUCC;
}

U32 sc_data_syn_proc(DLL_NODE_S *pstNode)
{
    S8        *pRequest = NULL;
    S8        *pStart = NULL, *pEnd = NULL;
    S8        *pszKeyWord[SC_API_PARAMS_MAX_NUM] = { 0 };
    S8        *pWord = NULL, *pValue = NULL;
    S8        szReqBuffer[SC_HTTP_REQ_MAX_LEN] = { 0 };
    S8        szReqLine[1024] = { 0 };
    S32       lKeyCnt = 0, lParamIndex = 0;
    U32       ulRet;
    list_t    stParamList;
    list_t    *pstParamListNode;
    SC_API_PARAMS_ST *pstParamsList;

    http_req_handle cb;

    if (DOS_ADDR_INVALID(pstNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pRequest = pstNode->pHandle;

    sc_logr_debug(SC_HTTP_API, "HTTP Request: %s", pRequest);

    /* 获取请求的文件 */
    pStart = pRequest;
    if (!pStart)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }
    pEnd = dos_strstr(pStart, "?");
    if (!pStart || !pEnd
        || pEnd <= pStart)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }
    dos_strncpy(szReqBuffer, pStart, pEnd-pStart);
    szReqBuffer[pEnd - pStart] = '\0';
    if ('\0' == szReqBuffer[0])
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    /* 获取请求行参数 */
    pStart = dos_strstr(pRequest, "?");
    if (!pStart)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    /* 获取请求参数 */
    pStart += dos_strlen("?");
    if ('\0' == *pStart)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }
    dos_strncpy(szReqLine, pStart, sizeof(szReqLine));
    szReqLine[sizeof(szReqLine) - 1] = '\0';

    sc_logr_debug(SC_HTTP_API, "HTTP Request Line: %s?%s", szReqBuffer, szReqLine);

    /* 获取 key=value 字符串 */
    lKeyCnt = 0;
    pWord = strtok(szReqLine, "&");
    while (pWord)
    {
        pszKeyWord[lKeyCnt] = dos_dmem_alloc(dos_strlen(pWord) + 1);
        if (!pszKeyWord[lKeyCnt])
        {
            logr_warning("%s", "Alloc fail.");
            break;
        }

        dos_strcpy(pszKeyWord[lKeyCnt], pWord);
        lKeyCnt++;
        pWord = strtok(NULL, "&");
        if (NULL == pWord)
        {
            break;
        }
    }

    if (lKeyCnt<= 0)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    sc_logr_debug(SC_HTTP_API, "%s", "Start prase the http request.");

    /* 解析key=value，并将结果存入链表 */
    dos_list_init(&stParamList);
    for (lParamIndex=0; lParamIndex<lKeyCnt; lParamIndex++)
    {
        if(!pszKeyWord[lParamIndex])
        {
            continue;
        }

        sc_logr_debug(SC_HTTP_API, "Process Token: %s", pszKeyWord[lParamIndex]);

        pWord = dos_strstr(pszKeyWord[lParamIndex], "=");
        pValue = pWord;
        if (!pValue)
        {
            continue;
        }
        pValue++;
        if (!pValue)
        {
            continue;
        }
        *pWord = '\0';

        pstParamsList = (SC_API_PARAMS_ST *)dos_dmem_alloc(sizeof(SC_API_PARAMS_ST));
        if (!pstParamsList)
        {
            DOS_ASSERT(0);
            continue;
        }

        /*
         * pValue 指向的是 pszKeyWord[lParamIndex]所在地址段的某个地址，
         * 而pszKeyWord[lParamIndex]是动态申请的内存，所以这里就不用重新申请内存了
         */
        pstParamsList->pszStringName = pszKeyWord[lParamIndex];
        pstParamsList->pszStringVal = pValue;

        dos_list_add_tail(&(stParamList), &pstParamsList->stList);
    }

    sc_logr_debug(SC_HTTP_API, "%s", "Prase the http request finished.");

    ulRet = SC_HTTP_ERRNO_INVALID_REQUEST;
    cb = sc_http_api_find(szReqBuffer);
    if (cb)
    {
        ulRet = cb(&(stParamList));
    }


    sc_logr_notice(SC_HTTP_API, "HTTP Request process finished. Return code: %d", ulRet);

    while (1)
    {
        if (dos_list_is_empty(&stParamList))
        {
            break;
        }

        pstParamListNode = dos_list_fetch(&stParamList);
        if (!pstParamListNode)
        {
            break;
        }

        pstParamsList = dos_list_entry(pstParamListNode, SC_API_PARAMS_ST, stList);
        if (!pstParamsList)
        {
            continue;
        }

        pstParamsList->pszStringName = NULL;
        pstParamsList->pszStringVal = NULL;
        dos_dmem_free(pstParamsList);
        pstParamsList = NULL;
    }

    for (lParamIndex=0; lParamIndex<lKeyCnt; lParamIndex++)
    {
        dos_dmem_free(pszKeyWord[lParamIndex]);
        pszKeyWord[lParamIndex] = NULL;
    }

    if (DOS_SUCC == ulRet)
    {
        return DOS_SUCC;
    }

cmd_prase_fail1:
    SC_TRACE_OUT();
    return DOS_FAIL;

}

VOID* sc_data_syn_proc_runtime(VOID *ptr)
{
    struct timespec stTimeout;
    DLL_NODE_S *pstNode;

    while (1)
    {
        /* 读取消息队列第一个数据 */
        pthread_mutex_lock(&g_mutexDataSyn);
        stTimeout.tv_sec = time(0) + 5;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condDataSyn, &g_mutexDataSyn, &stTimeout);
        pthread_mutex_unlock(&g_mutexDataSyn);

        if (!g_blSCDataSybFlag)
        {
            continue;
        }

        g_blSCDataSybFlag = DOS_FALSE;

        do{
            if (sc_walk_tmp_tbl() != DOS_SUCC)
            {
                DOS_ASSERT(0);
                break;
            }

            while (1)
            {
                if (DLL_Count(&g_stSCTMPTblRecord) == 0)
                {
                    break;
                }

                pstNode = dll_fetch(&g_stSCTMPTblRecord);
                if (DOS_ADDR_INVALID(pstNode))
                {
                    DOS_ASSERT(0);

                    continue;
                }

                sc_data_syn_proc(pstNode);

                if (pstNode->pHandle)
                {
                    dos_dmem_free(pstNode->pHandle);
                    pstNode->pHandle = NULL;
                }

                DLL_Init_Node(pstNode);
                dos_dmem_free(pstNode);
                pstNode = NULL;
            }
        }while (g_blSCDataSybFlag);
    }
}

/**
 * 函数: VOID* sc_ep_runtime(VOID *ptr)
 * 功能: ESL事件接收线程主函数
 * 参数:
 * 返回值:
 */
VOID* sc_data_syn_runtime(VOID *ptr)
{
    S32                 lSocket = -1, lRet, lAddrLen;
    U8                  aucBuff[SC_DATA_SYN_BUFF_LEN];
    BS_MSG_TAG          *pstMsgTag;
    struct sockaddr_un  stAddr, stAddrIn;
    struct timeval      stTimeout={2, 0};
    fd_set              stFDSet;

    for (;;)
    {
        while (!g_blConnected)
        {
            /* 初始化socket(UNIX STREAM方式) */
            lSocket = socket(AF_UNIX, SOCK_DGRAM, 0);
            if (lSocket < 0)
            {
                sc_logr_error(SC_SYN, "%s", "ERR: create socket fail!");
                goto connect_fail;
            }

            dos_memzero(&stAddrIn, sizeof(stAddrIn));
            dos_memzero(&stAddr, sizeof(stAddr));
            lAddrLen = sizeof(struct sockaddr_un);
            stAddr.sun_family = AF_UNIX;
            strncpy(stAddr.sun_path, SC_DATA_SYN_SOCK_PATH, sizeof(stAddr.sun_path) - 1);
            unlink(SC_DATA_SYN_SOCK_PATH);
            lRet = bind(lSocket, (struct sockaddr *)&stAddr, lAddrLen);
            if (lRet < 0)
            {
                sc_logr_error(SC_SYN, "%s", "ERR: bind socket fail!");
                close(lSocket);
                lSocket= -1;
                goto connect_fail;
            }

            g_blConnected = DOS_TRUE;
            break;
connect_fail:
            sleep(2000);
            continue;
        }

        FD_ZERO(&stFDSet);
        FD_SET(lSocket, &stFDSet);
        stTimeout.tv_sec = 1;
        stTimeout.tv_usec = 0;
        lRet = select(lSocket + 1, &stFDSet, NULL, NULL, &stTimeout);
        if (lRet < 0)
        {
            DOS_ASSERT(0);

            sc_logr_error(SC_SYN, "%s", "Select FAIL.");
            g_blConnected = DOS_FALSE;
            continue;
        }
        else if (0 == lRet)
        {
            continue;
        }

        if (!FD_ISSET(lSocket, &stFDSet))
        {
            continue;
        }

        lAddrLen = sizeof(stAddrIn);
        lRet = recvfrom(lSocket
                        , aucBuff
                        , SC_DATA_SYN_BUFF_LEN
                        , 0
                        ,  (struct sockaddr *)&stAddrIn
                        , (socklen_t *)&lAddrLen);
        if (lRet < 0)
        {
            sc_logr_error(SC_SYN, "%s", "Select FAIL.");
            close(lSocket);
            g_blConnected = DOS_FALSE;
            continue;
        }

        if (0 == lRet || EINTR == errno || EAGAIN == errno)
        {
            continue;
        }

        /* TODO: 判断长度之后再转换 */
        pstMsgTag = (BS_MSG_TAG *)aucBuff;

        /* TODO: 转换之后需要处理消息 */


        g_blSCDataSybFlag = DOS_TRUE;

        pthread_mutex_lock(&g_mutexDataSyn);
        pthread_cond_signal(&g_condDataSyn);
        pthread_mutex_unlock(&g_mutexDataSyn);
    }

    /* @TODO 释放资源 */
    return NULL;
}

/* 初始化事件处理模块 */
U32 sc_data_syn_init()
{
    DLL_Init(&g_stSCTMPTblRecord);
    return DOS_SUCC;
}

/* 启动事件处理模块 */
U32 sc_data_syn_start()
{
    if (pthread_create(&g_pthDataSynProc, NULL, sc_data_syn_proc_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (pthread_create(&g_pthDataSyn, NULL, sc_data_syn_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }


    return DOS_SUCC;
}

/* 停止事件处理模块 */
U32 sc_data_syn_shutdown()
{
    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


