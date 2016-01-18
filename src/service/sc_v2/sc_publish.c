#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <esl.h>
#include <sys/time.h>
#include <bs_pub.h>
#include <libcurl/curl.h>
#include "sc_def.h"
#include "sc_res.h"
#include "sc_pub.h"
#include "sc_db.h"
#include "sc_su.h"
#include "sc_debug.h"
#include "sc_publish.h"
#include "sc_http_api.h"



#define SC_PUB_TASK_NUMBER       3

#define SC_PUB_MASTER_INDEX      0
#define SC_PUB_STATUS_INDEX      1
#define SC_PUB_DATA_INDEX        2

#define SC_PUB_MAX_URL           128
#define SC_PUB_MAX_DATA          512


typedef struct tagPublishMsg{
    S8      *pszURL;
    S8      *pszData;

    U32     ulType;
    VOID    *pstDesc;
}SC_PUB_MSG_ST;

typedef struct tagPublishTask{
    U32              ulValid;
    CURL            *pstPublishCurlHandle;
    DLL_S            stPublishQueue;

    pthread_t        pthPublishCurl;
    pthread_mutex_t  mutexPublishQueue;
    pthread_mutex_t  mutexPublishCurl;
    pthread_cond_t   condPublishCurl;
}SC_PUB_TASK_ST;


static U32               g_ulPublishExit;
static SC_PUB_TASK_ST    g_stPubTaskList[SC_PUB_TASK_NUMBER];


static U32 sc_pub_on_succ(U32 ulType, VOID *pDesc)
{
    SC_PUB_FS_DATA_ST *pstData;

    switch (ulType)
    {
        case SC_PUB_TYPE_STATUS:
            /* Do nothing */
            break;

        case SC_PUB_TYPE_MARKER:
            /* Do nothing */
            break;

        case SC_PUB_TYPE_SIP_XMl:
            sc_esl_reloadxml();
            break;

        case SC_PUB_TYPE_GATEWAY:
            pstData = (SC_PUB_FS_DATA_ST *)pDesc;
            sc_esl_update_gateway(pstData->ulAction, pstData->ulID);
            break;
    }

    return DOS_SUCC;
}

static U32 sc_pub_publicsh(CURL **pstCurlHandle, S8 *pszURL, S8 *pszData)
{
    U32 ulRet;
    U32 ulTimeout = 2;

    if (DOS_ADDR_INVALID(pszURL))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstCurlHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(*pstCurlHandle))
    {
        *pstCurlHandle = curl_easy_init();
        if (DOS_ADDR_INVALID(*pstCurlHandle))
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
    }

    curl_easy_reset(*pstCurlHandle);
    curl_easy_setopt(*pstCurlHandle, CURLOPT_URL, pszURL);
    curl_easy_setopt(*pstCurlHandle, CURLOPT_NOSIGNAL, 1L);
    if (DOS_ADDR_VALID(pszData))
    {
        curl_easy_setopt(*pstCurlHandle, CURLOPT_POSTFIELDS, pszData);
    }
    curl_easy_setopt(*pstCurlHandle, CURLOPT_TIMEOUT, ulTimeout);
    ulRet = curl_easy_perform(*pstCurlHandle);
    if(CURLE_OK != ulRet)
    {
        DOS_ASSERT(0);

        sc_log(LOG_LEVEL_ERROR, "CURL post FAIL. Data:%s.", pszData);
        return DOS_FAIL;
    }
    else
    {
        sc_log(LOG_LEVEL_INFO, "CURL post SUCC.URL:%s, Data:%s.", pszURL, pszData);

        return DOS_SUCC;
    }
}


static VOID *sc_pub_runtime(VOID *ptr)
{
    DLL_NODE_S     *pstDllNode = NULL;
    SC_PUB_MSG_ST  *pstPubData = NULL;
    SC_PUB_TASK_ST *pstTask    = NULL;

    pstTask = (SC_PUB_TASK_ST *)ptr;
    if (DOS_ADDR_INVALID(pstTask))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    pstTask->ulValid = DOS_TRUE;

    while (1)
    {
        if (g_ulPublishExit)
        {
            break;
        }

        pthread_mutex_lock(&pstTask->mutexPublishCurl);
        pthread_cond_wait(&pstTask->condPublishCurl, &pstTask->mutexPublishCurl);
        pthread_mutex_unlock(&pstTask->mutexPublishCurl);

        if (0 == DLL_Count(&pstTask->stPublishQueue))
        {
            continue;
        }

        while (1)
        {
            if (0 == DLL_Count(&pstTask->stPublishQueue))
            {
                break;
            }

            pthread_mutex_lock(&pstTask->mutexPublishQueue);
            pstDllNode = dll_fetch(&pstTask->stPublishQueue);
            pthread_mutex_unlock(&pstTask->mutexPublishQueue);

            if (DOS_ADDR_INVALID(pstDllNode))
            {
                sc_log(LOG_LEVEL_INFO, "%s", "DLL Node is null, may be the queue is breaken");
                break;
            }

            pstPubData = pstDllNode->pHandle;
            if (DOS_ADDR_INVALID(pstPubData))
            {
                sc_log(LOG_LEVEL_INFO, "%s", "DLL Node with empty data.");
                continue;
            }

            if (DOS_ADDR_INVALID(pstPubData->pszURL))
            {
                sc_log(LOG_LEVEL_INFO, "%s", "Publish request is invalid.");

                goto proc_fail;
            }

            if (sc_pub_publicsh(&pstTask->pstPublishCurlHandle, pstPubData->pszURL, pstPubData->pszData) == DOS_SUCC)
            {
                sc_pub_on_succ(pstPubData->ulType, pstPubData->pstDesc);
            }
proc_fail:
            if (DOS_ADDR_VALID(pstPubData->pstDesc))
            {
                dos_dmem_free(pstPubData->pstDesc);
                pstPubData->pstDesc = NULL;
            }

            if (DOS_ADDR_VALID(pstPubData->pszURL))
            {
                dos_dmem_free(pstPubData->pszURL);
                pstPubData->pszURL = NULL;
            }

            if (DOS_ADDR_VALID(pstPubData->pszData))
            {
                dos_dmem_free(pstPubData->pszData);
                pstPubData->pszData = NULL;
            }

            if (DOS_ADDR_VALID(pstPubData))
            {
                dos_dmem_free(pstPubData);
                pstPubData = NULL;
            }

            if (DOS_ADDR_VALID(pstDllNode))
            {
                DLL_Init_Node(pstDllNode);
                dos_dmem_free(pstDllNode);
                pstDllNode = NULL;
            }
        }
    }


    pstTask->ulValid = DOS_FALSE;

    return NULL;
}

static VOID *sc_pub_runtime_master(VOID *ptr)
{
    DLL_NODE_S     *pstDllNode = NULL;
    SC_PUB_TASK_ST *pstTask    = NULL;
    SC_PUB_MSG_ST  *pstPubData = NULL;

    pstTask = (SC_PUB_TASK_ST *)ptr;
    if (DOS_ADDR_INVALID(pstTask))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    while (1)
    {
        if (g_ulPublishExit)
        {
            break;
        }

        pthread_mutex_lock(&pstTask->mutexPublishCurl);
        pthread_cond_wait(&pstTask->condPublishCurl, &pstTask->mutexPublishCurl);
        pthread_mutex_unlock(&pstTask->mutexPublishCurl);

        if (0 == DLL_Count(&pstTask->stPublishQueue))
        {
            continue;
        }

        while (1)
        {
            if (0 == DLL_Count(&pstTask->stPublishQueue))
            {
                break;
            }

            pthread_mutex_lock(&pstTask->mutexPublishQueue);
            pstDllNode = dll_fetch(&pstTask->stPublishQueue);
            pthread_mutex_unlock(&pstTask->mutexPublishQueue);

            if (DOS_ADDR_INVALID(pstDllNode))
            {
                sc_log(LOG_LEVEL_WARNING, "%s", "DLL Node is null, may be the queue is breaken");
                break;
            }

            pstPubData = pstDllNode->pHandle;
            if (DOS_ADDR_INVALID(pstPubData))
            {
                sc_log(LOG_LEVEL_WARNING, "%s", "DLL Node with empty data, may be the queue is breaken");

                dos_dmem_free(pstPubData);
                pstPubData = NULL;
                break;
            }

            if (SC_PUB_TYPE_STATUS == pstPubData->ulType)
            {
                pthread_mutex_lock(&g_stPubTaskList[SC_PUB_STATUS_INDEX].mutexPublishQueue);
                DLL_Add(&g_stPubTaskList[SC_PUB_STATUS_INDEX].stPublishQueue, pstDllNode);
                pthread_mutex_unlock(&g_stPubTaskList[SC_PUB_STATUS_INDEX].mutexPublishQueue);

                pthread_mutex_lock(&g_stPubTaskList[SC_PUB_STATUS_INDEX].mutexPublishCurl);
                pthread_cond_signal(&g_stPubTaskList[SC_PUB_STATUS_INDEX].condPublishCurl);
                pthread_mutex_unlock(&g_stPubTaskList[SC_PUB_STATUS_INDEX].mutexPublishCurl);
            }
            else
            {
                pthread_mutex_lock(&g_stPubTaskList[SC_PUB_DATA_INDEX].mutexPublishQueue);
                DLL_Add(&g_stPubTaskList[SC_PUB_DATA_INDEX].stPublishQueue, pstDllNode);
                pthread_mutex_unlock(&g_stPubTaskList[SC_PUB_DATA_INDEX].mutexPublishQueue);

                pthread_mutex_lock(&g_stPubTaskList[SC_PUB_DATA_INDEX].mutexPublishCurl);
                pthread_cond_signal(&g_stPubTaskList[SC_PUB_DATA_INDEX].condPublishCurl);
                pthread_mutex_unlock(&g_stPubTaskList[SC_PUB_DATA_INDEX].mutexPublishCurl);
            }
        }
    }

    return NULL;
}

U32 sc_pub_send_msg(S8 *pszURL, S8 *pszData, U32 ulType, VOID *pstData)
{
    DLL_NODE_S     *pstDllNode;
    SC_PUB_MSG_ST  *pstPubData = NULL;

    if (DOS_ADDR_INVALID(pszURL))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (ulType >= SC_PUB_TYPE_BUUT)
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "Invalid msg type.(%u)", ulType);
        return DOS_FAIL;
    }

    pstDllNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    pstPubData = dos_dmem_alloc(sizeof(SC_PUB_MSG_ST));
    if (DOS_ADDR_INVALID(pstDllNode) || DOS_ADDR_INVALID(pstPubData))
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "Alloc memory fail while send publish request.");

        goto proc_fail;
    }

    DLL_Init_Node(pstDllNode);
    pstPubData->pszURL = NULL;
    pstPubData->pszData = NULL;
    pstPubData->pstDesc = pstData;
    pstPubData->ulType = ulType;
    pstPubData->pszURL = dos_strndup(pszURL, SC_PUB_MAX_URL);
    if (DOS_ADDR_INVALID(pstPubData->pszURL))
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "Dump the url fail.");
        goto proc_fail;
    }

    if (DOS_ADDR_VALID(pszData))
    {
        pstPubData->pszData = dos_strndup(pszData, SC_PUB_MAX_DATA);
        if (DOS_ADDR_INVALID(pstPubData->pszData))
        {
            sc_log(LOG_LEVEL_ERROR, "%s", "Dump the data fail.");
            goto proc_fail;
        }
    }

    pstDllNode->pHandle = pstPubData;

    pthread_mutex_lock(&g_stPubTaskList[0].mutexPublishQueue);
    DLL_Add(&g_stPubTaskList[0].stPublishQueue, pstDllNode);
    pthread_mutex_unlock(&g_stPubTaskList[0].mutexPublishQueue);

    pthread_mutex_lock(&g_stPubTaskList[0].mutexPublishCurl);
    pthread_cond_signal(&g_stPubTaskList[0].condPublishCurl);
    pthread_mutex_unlock(&g_stPubTaskList[0].mutexPublishCurl);

    sc_log(LOG_LEVEL_DEBUG, "Publish request send succ. URL: %s, Data: %s", pszURL, DOS_ADDR_VALID(pszData) ? pszData : "");

    return DOS_SUCC;
proc_fail:
    if (DOS_ADDR_VALID(pstDllNode))
    {
        dos_dmem_free(pstDllNode);
        pstDllNode = NULL;
    }

    if (DOS_ADDR_VALID(pstPubData))
    {
        if (DOS_ADDR_VALID(pstPubData->pszURL))
        {
            dos_dmem_free(pstPubData->pszURL);
            pstPubData->pszURL = NULL;
        }

        if (DOS_ADDR_VALID(pszData))
        {
            if (DOS_ADDR_VALID(pstPubData->pszData))
            {
                dos_dmem_free(pstPubData->pszData);
                pstPubData->pszData = NULL;
            }
        }

        dos_dmem_free(pstPubData);
        pstDllNode = NULL;
    }

    return DOS_FAIL;
}

U32 sc_pub_init()
{
    U32 i;

    dos_memzero(g_stPubTaskList, sizeof(g_stPubTaskList));

    for (i=0; i<SC_PUB_TASK_NUMBER; i++)
    {
        g_stPubTaskList[i].ulValid = DOS_FALSE;
        pthread_mutex_init(&g_stPubTaskList[i].mutexPublishCurl, NULL);
        pthread_mutex_init(&g_stPubTaskList[i].mutexPublishQueue, NULL);
        pthread_cond_init(&g_stPubTaskList[i].condPublishCurl, NULL);
        g_stPubTaskList[i].pstPublishCurlHandle = NULL;
        DLL_Init(&g_stPubTaskList[i].stPublishQueue);
    }

    return DOS_SUCC;
}

U32 sc_pub_start()
{
    U32 i;

    if (pthread_create(&g_stPubTaskList[SC_PUB_MASTER_INDEX].pthPublishCurl, NULL, sc_pub_runtime_master, &g_stPubTaskList[SC_PUB_MASTER_INDEX]) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    for (i=1; i<SC_PUB_TASK_NUMBER; i++)
    {
        if (pthread_create(&g_stPubTaskList[i].pthPublishCurl, NULL, sc_pub_runtime, &g_stPubTaskList[i]) < 0)
        {
            g_stPubTaskList[i].ulValid = DOS_FALSE;
        }
    }

    return DOS_SUCC;
}

U32 sc_pub_stop()
{
    g_ulPublishExit = DOS_TRUE;

    return DOS_SUCC;
}



#ifdef __cplusplus
}
#endif /* __cplusplus */

