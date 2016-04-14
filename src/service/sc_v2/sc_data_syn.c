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
#include "sc_res.h"
#include "sc_http_api.h"


#define SC_DATA_SYN_SOCK_PATH "/var/www/html/temp/sc-syn.sock"
#define SC_DATA_SYN_BUFF_LEN  512


typedef struct tagRequestData{
    DLL_NODE_S stDLLNode;

    U32 ulTime;
    U32 ulID;
    S8 *pszData;
}SC_SYN_REQUEST_DATA_ST;

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
    SC_SYN_REQUEST_DATA_ST *pstData;

    if (DOS_ADDR_INVALID(aszData) || DOS_ADDR_INVALID(aszData[2]) || aszData[2][0] == '\0')
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstData = dos_dmem_alloc(sizeof(SC_SYN_REQUEST_DATA_ST));
    if (DOS_ADDR_INVALID(pstData))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    DLL_Init_Node(&(pstData->stDLLNode));

    pstData->pszData = dos_dmem_alloc(SC_DATA_SYN_BUFF_LEN);
    if (DOS_ADDR_INVALID(pstData))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstData);
        pstData = NULL;
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(aszData[0]) || '\0' == aszData[0][0]
        || dos_atoul(aszData[0], &pstData->ulID) < 0)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstData);
        pstData = NULL;
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(aszData[1]) || '\0' == aszData[1][0]
        || dos_atoul(aszData[1], &pstData->ulTime) < 0)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstData);
        pstData = NULL;
        return DOS_FAIL;
    }

    dos_strncpy(pstData->pszData, aszData[2], SC_DATA_SYN_BUFF_LEN);
    pstData->pszData[SC_DATA_SYN_BUFF_LEN - 1] = '\0';

    DLL_Add(&g_stSCTMPTblRecord, (DLL_NODE_S *)pstData);

    return DOS_SUCC;
}

VOID sc_tmp_tbl_clean()
{
    S8 szQuery[256] = {0, };

    dos_snprintf(szQuery, sizeof(szQuery), "delete from tmp_tbl_fsmodify where fs_related<>1;");

    db_transaction_begin(g_pstSCDBHandle);
    if (db_query(g_pstSCDBHandle, szQuery, NULL, NULL, NULL) != DB_ERR_SUCC)
    {
        sc_log(DOS_FALSE, LOG_LEVEL_EMERG, "DB query failed. (%s)", szQuery);
        db_transaction_rollback(g_pstSCDBHandle);
        return;
    }

    db_transaction_commit(g_pstSCDBHandle);
}

U32 sc_walk_tmp_tbl()
{
    S8 szQuery[256] = {0, };
    SC_SYN_REQUEST_DATA_ST *pstData;

    if (!g_pstSCDBHandle || g_pstSCDBHandle->ulDBStatus != DB_STATE_CONNECTED)
    {
        DOS_ASSERT(0);
        sc_log(DOS_FALSE, LOG_LEVEL_EMERG, "%s", "DB Has been down or not init.");
        return DOS_FAIL;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT id,ctime,json_fields FROM tmp_tbl_fsmodify;");

    if (db_query(g_pstSCDBHandle, szQuery, sc_walk_tmp_tbl_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        sc_log(DOS_FALSE, LOG_LEVEL_EMERG, "DB query failed.(%s)", szQuery);
        return DOS_FAIL;
    }

    DLL_Scan(&g_stSCTMPTblRecord, pstData, SC_SYN_REQUEST_DATA_ST *)
    {
        dos_snprintf(szQuery, sizeof(szQuery), "delete FROM tmp_tbl_fsmodify where id=%u;", pstData->ulID);
        db_query(g_pstSCDBHandle, szQuery, NULL, NULL, NULL);
    }

    return DOS_SUCC;
}


U32 sc_data_syn_proc(SC_SYN_REQUEST_DATA_ST *pstNode)
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

    pRequest = pstNode->pszData;

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_DATA_SYN), "HTTP Request: %s", pRequest);

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

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_DATA_SYN), "HTTP Request Line: %s?%s", szReqBuffer, szReqLine);

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

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_DATA_SYN), "%s", "Start parse the http request.");

    /* 解析key=value，并将结果存入链表 */
    dos_list_init(&stParamList);
    for (lParamIndex=0; lParamIndex<lKeyCnt; lParamIndex++)
    {
        if(!pszKeyWord[lParamIndex])
        {
            continue;
        }

        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_DATA_SYN), "Process Token: %s", pszKeyWord[lParamIndex]);

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

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_DATA_SYN), "Parse the http request finished. Request: %s", szReqBuffer);

    ulRet = SC_HTTP_ERRNO_INVALID_REQUEST;
    cb = sc_http_api_find(szReqBuffer);
    if (cb)
    {
        ulRet = cb(&(stParamList));
    }
    else
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_DATA_SYN), "Cannot find the callback function for the request %s", szReqBuffer);
    }

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_DATA_SYN), "HTTP Request process finished. Return code: 0x%X", ulRet);

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
    return DOS_FAIL;

}

VOID* sc_data_syn_proc_runtime(VOID *ptr)
{
    struct timespec stTimeout;
    SC_SYN_REQUEST_DATA_ST *pstNode;
    static BOOL blOnStartUP = DOS_TRUE;

    while (1)
    {
        /* 读取消息队列第一个数据 */
        pthread_mutex_lock(&g_mutexDataSyn);
        stTimeout.tv_sec = time(0) + 5;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condDataSyn, &g_mutexDataSyn, &stTimeout);
        pthread_mutex_unlock(&g_mutexDataSyn);

        /* 如果是第一次运行需要检查以下哦 */
        if (!blOnStartUP)
        {
            if (!g_blSCDataSybFlag)
            {
                continue;
            }

            g_blSCDataSybFlag = DOS_FALSE;
        }
        blOnStartUP = DOS_FALSE;

        if (sc_walk_tmp_tbl() != DOS_SUCC)
        {
            DOS_ASSERT(0);
            continue;
        }

        while (1)
        {
            if (DLL_Count(&g_stSCTMPTblRecord) == 0)
            {
                break;
            }

            pstNode = (SC_SYN_REQUEST_DATA_ST *)dll_fetch(&g_stSCTMPTblRecord);
            if (DOS_ADDR_INVALID(pstNode))
            {
                DOS_ASSERT(0);

                continue;
            }

            sc_data_syn_proc(pstNode);

            if (pstNode->pszData)
            {
                dos_dmem_free(pstNode->pszData);
                pstNode->pszData = NULL;
            }

            DLL_Init_Node((DLL_NODE_S *)pstNode);
            dos_dmem_free(pstNode);
            pstNode = NULL;
        }
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
    S32                 lSocket = -1, lClientSocket, lRet, lAddrLen;
    U8                  aucBuff[SC_DATA_SYN_BUFF_LEN];
    BS_MSG_TAG          *pstMsgTag;
    struct sockaddr_un  stAddr, stAddrIn;
    struct timeval      stTimeout={2, 0};
    fd_set              stFDSet;

    for (;;)
    {
        /* Socket 文件被删除了 */
        if (g_blConnected && access(SC_DATA_SYN_SOCK_PATH, F_OK) < 0)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DATA_SYN), "%s", "Socket file has been lost.");

            close(lSocket);
            lSocket = -1;
            g_blConnected = DOS_FALSE;
        }

        while (!g_blConnected)
        {
            /* 初始化socket(UNIX STREAM方式) */
            lSocket = socket(AF_UNIX, SOCK_STREAM, 0);
            if (lSocket < 0)
            {
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DATA_SYN), "%s", "ERR: create socket fail!");
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
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DATA_SYN), "%s", "ERR: bind socket fail!");
                close(lSocket);
                lSocket= -1;
                goto connect_fail;
            }

            chmod(SC_DATA_SYN_SOCK_PATH, S_IREAD|S_IWRITE|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

            /* 目前监听数量不确定,暂时以SOMAXCONN为准 */
            lRet = listen(lSocket, SOMAXCONN);
            if (lRet < 0)
            {
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DATA_SYN), "%s", "ERR: listen socket fail!");
                close(lSocket);
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
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DATA_SYN), "%s", "Select FAIL.");
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

        lClientSocket = accept(lSocket, (struct sockaddr *)&stAddrIn, (socklen_t *)&lAddrLen);
        if (lClientSocket < 0)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DATA_SYN), "%s", "Accept FAIL.");
            continue;
        }

        lAddrLen = sizeof(stAddrIn);
        lRet = recv(lClientSocket, aucBuff, SC_DATA_SYN_BUFF_LEN, 0);
        if (lRet < 0)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_DATA_SYN), "%s", "Recv FAIL.");
            close(lClientSocket);
            lClientSocket = -1;
            g_blConnected = DOS_FALSE;
            continue;
        }

        close(lClientSocket);
        lClientSocket = 0;

        /* TODO: 判断长度之后再转换 */
        pstMsgTag = (BS_MSG_TAG *)aucBuff;

        /* TODO: 转换之后需要处理消息 */

        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_DATA_SYN), "%s", "Recv data syn command!");

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
U32 sc_data_syn_stop()
{
    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


