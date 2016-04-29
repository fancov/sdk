/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名
 *
 *  创建时间:
 *  作    者:
 *  描    述:
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <sc_pub.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_log_digest.h"

static BOOL             g_blExitFlag = DOS_TRUE;
DLL_S                   g_stLogDigestQueue;
static pthread_t        g_pthreadLogDigest;
static pthread_mutex_t  g_mutexLogDigestQueue = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   g_condLogDigestQueue  = PTHREAD_COND_INITIALIZER;

static VOID *sc_log_digest_mainloop(VOID *ptr)
{
    DLL_NODE_S          *pstDLLNode    = NULL;
    S8                  *pstMsg        = NULL;
    FILE                *pstLogFile    = NULL;
    FILE                *fp            = NULL;
    U32                 ulFileSize     = 0;
    S8                  szPsCmd[128]   = {0};
    S8                  szCurTime[32]  = {0,};
    time_t              stTime;
    SC_PTHREAD_MSG_ST   *pstPthreadMsg = NULL;
    struct timespec     stTimeout;

    g_blExitFlag = DOS_FALSE;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    pstPthreadMsg = sc_pthread_cb_alloc();
    if (DOS_ADDR_VALID(pstPthreadMsg))
    {
        pstPthreadMsg->ulPthID = pthread_self();
        pstPthreadMsg->func = sc_log_digest_mainloop;
        pstPthreadMsg->pParam = ptr;
        dos_strcpy(pstPthreadMsg->szName, "sc_log_digest_mainloop");
    }

    while (1)
    {
        if (g_blExitFlag)
        {
            break;
        }

        if (DOS_ADDR_VALID(pstPthreadMsg))
        {
            pstPthreadMsg->ulLastTime = time(NULL);
        }

        pthread_mutex_lock(&g_mutexLogDigestQueue);
        stTimeout.tv_sec = time(0) + 5;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condLogDigestQueue, &g_mutexLogDigestQueue, &stTimeout);
        pthread_mutex_unlock(&g_mutexLogDigestQueue);

        if (0 == DLL_Count(&g_stLogDigestQueue))
        {
            continue;
        }

        while (1)
        {
            if (0 == DLL_Count(&g_stLogDigestQueue))
            {
                break;
            }

            if (DOS_ADDR_INVALID(pstLogFile))
            {
                pstLogFile = fopen(SC_LOG_DIGEST_PATH, "a");
                if (DOS_ADDR_INVALID(pstLogFile))
                {
                    sleep(2);
                    break;
                }

                fseek(pstLogFile, 0L, SEEK_END);
                ulFileSize = ftell(pstLogFile);
            }

            pthread_mutex_lock(&g_mutexLogDigestQueue);
            pstDLLNode = dll_fetch(&g_stLogDigestQueue);
            pthread_mutex_unlock(&g_mutexLogDigestQueue);

            if (DOS_ADDR_INVALID(pstDLLNode))
            {
                break;
            }

            if (DOS_ADDR_INVALID(pstDLLNode->pHandle))
            {
                break;
            }

            pstMsg = (S8 *)pstDLLNode->pHandle;

            DLL_Init_Node(pstDLLNode);
            dos_dmem_free(pstDLLNode);
            pstDLLNode = NULL;

            if (fwrite(pstMsg, dos_strlen(pstMsg), 1, pstLogFile) != 1)
            {
            }

            fflush(pstLogFile);
            ulFileSize += dos_strlen(pstMsg);
            dos_dmem_free(pstMsg);
            pstMsg = NULL;

            if (ulFileSize > SC_LOG_DIGEST_FILE_MAX_SIZE)
            {
                fclose(pstLogFile);
                pstLogFile = NULL;

                stTime = time(NULL);
                strftime(szCurTime, sizeof(szCurTime), "%Y%m%d%H%M%S", localtime(&stTime));

                dos_snprintf(szPsCmd, sizeof(szPsCmd), "mv %s %s%s", SC_LOG_DIGEST_PATH, SC_LOG_DIGEST_PATH, szCurTime);
                fp = popen(szPsCmd, "r");
                if (DOS_ADDR_INVALID(fp))
                {
                }
            }
        }
    }

    return NULL;
}

U32 sc_log_digest_print(const S8 *szTraceStr)
{
    DLL_NODE_S  *pstNode        = NULL;
    S8          *pszBuf         = NULL;
    S8          szCurTime[32]   = {0,};

    if (DOS_ADDR_INVALID(szTraceStr))
    {
        return DOS_FAIL;
    }

    dos_get_localtime(time(NULL), szCurTime, sizeof(szCurTime));

    pszBuf = (S8 *)dos_dmem_alloc(SC_LOG_DIGEST_LEN);
    if (DOS_ADDR_INVALID(pszBuf))
    {
        return DOS_FAIL;
    }
    dos_snprintf(pszBuf, SC_LOG_DIGEST_LEN, "%s  %s\r\n", szCurTime, szTraceStr);

    pstNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstNode))
    {
        dos_dmem_free(pszBuf);
        pszBuf = NULL;
        return DOS_FAIL;
    }

    DLL_Init_Node(pstNode);
    pstNode->pHandle = pszBuf;

    pthread_mutex_lock(&g_mutexLogDigestQueue);
    DLL_Add(&g_stLogDigestQueue, pstNode);
    pthread_cond_signal(&g_condLogDigestQueue);
    pthread_mutex_unlock(&g_mutexLogDigestQueue);

    return DOS_SUCC;
}

U32 sc_log_digest_init()
{
    DLL_Init(&g_stLogDigestQueue);

    return DOS_SUCC;
}

U32 sc_log_digest_start()
{
    if (pthread_create(&g_pthreadLogDigest, NULL, sc_log_digest_mainloop, NULL) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_log_digest_stop()
{
    g_blExitFlag = DOS_TRUE;

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


