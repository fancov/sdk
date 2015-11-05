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
#include "sc_pub.h"
#include "sc_db.h"

#define SC_PUB_TASK_NUMBER       5
#define SC_PUB_MAX_URL           128
#define SC_PUB_MAX_DATA          512


typedef struct tagPublishMsg{
    S8      *pszURL;
    S8      *pszData;
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
    if (DOS_ADDR_VALID(pszData))
    {
        curl_easy_setopt(*pstCurlHandle, CURLOPT_POSTFIELDS, pszData);
    }
    curl_easy_setopt(*pstCurlHandle, CURLOPT_TIMEOUT, ulTimeout);
    ulRet = curl_easy_perform(*pstCurlHandle);
    if(CURLE_OK != ulRet)
    {
        DOS_ASSERT(0);

        sc_logr_notice(SC_PUB, "CURL post FAIL.Caller:%s.", pszData);
        return DOS_FAIL;
    }
    else
    {
        sc_logr_notice(SC_PUB, "CURL post SUCC.URL:%s, Caller:%s.", pszURL, pszData);

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
                sc_logr_warning(SC_PUB, "%s", "DLL Node is null, may be the queue is breaken");
                break;
            }

            pstPubData = pstDllNode->pHandle;
            if (DOS_ADDR_INVALID(pstPubData))
            {
                sc_logr_info(SC_PUB, "%s", "DLL Node with empty data.");
                continue;
            }

            if (DOS_ADDR_INVALID(pstPubData->pszURL))
            {
                sc_logr_info(SC_PUB, "%s", "Publish request is invalid.");

                goto proc_fail;
            }

            sc_pub_publicsh(&pstTask->pstPublishCurlHandle, pstPubData->pszURL, pstPubData->pszData);

proc_fail:
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
    U32            ulLastTask  = 0;
    BOOL           blSend      = DOS_FALSE;
    U32 i;

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
                sc_logr_warning(SC_PUB, "%s", "DLL Node is null, may be the queue is breaken");
                break;
            }

            for (blSend = DOS_FALSE,i=ulLastTask+1; i<SC_PUB_TASK_NUMBER; i++)
            {
                if (!g_stPubTaskList[i].ulValid)
                {
                    continue;
                }

                pthread_mutex_lock(&g_stPubTaskList[i].mutexPublishQueue);
                DLL_Add(&g_stPubTaskList[i].stPublishQueue, pstDllNode);
                pthread_mutex_unlock(&g_stPubTaskList[i].mutexPublishQueue);

                pthread_mutex_lock(&g_stPubTaskList[i].mutexPublishCurl);
                pthread_cond_signal(&g_stPubTaskList[i].condPublishCurl);
                pthread_mutex_unlock(&g_stPubTaskList[i].mutexPublishCurl);

                ulLastTask = i;
                blSend = DOS_TRUE;
                break;
            }

            if (blSend)
            {
                continue;
            }

            for (i=1; i<=ulLastTask; i++)
            {
                if (!g_stPubTaskList[i].ulValid)
                {
                    continue;
                }

                pthread_mutex_lock(&g_stPubTaskList[i].mutexPublishQueue);
                DLL_Add(&g_stPubTaskList[i].stPublishQueue, pstDllNode);
                pthread_mutex_unlock(&g_stPubTaskList[i].mutexPublishQueue);

                pthread_mutex_lock(&g_stPubTaskList[i].mutexPublishCurl);
                pthread_cond_signal(&g_stPubTaskList[i].condPublishCurl);
                pthread_mutex_unlock(&g_stPubTaskList[i].mutexPublishCurl);

                ulLastTask = i;
                blSend = DOS_TRUE;
                break;
            }

            if (blSend)
            {
                continue;
            }

            sc_logr_notice(SC_PUB, "%s", "There is no slave task to process the request. Add to queue");

            /* 没有找到合适的线程处理，就让他回到队尾吧 */
            pthread_mutex_lock(&pstTask->mutexPublishQueue);
            DLL_Add(&pstTask->stPublishQueue, pstDllNode);
            pthread_mutex_unlock(&pstTask->mutexPublishQueue);
        }
    }

    return NULL;
}

U32 sc_pub_send_msg(S8 *pszURL, S8 *pszData)
{
    DLL_NODE_S     *pstDllNode;
    SC_PUB_MSG_ST  *pstPubData = NULL;

    if (DOS_ADDR_INVALID(pszURL))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstDllNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    pstPubData = dos_dmem_alloc(sizeof(SC_PUB_MSG_ST));
    if (DOS_ADDR_INVALID(pstDllNode) || DOS_ADDR_INVALID(pstPubData))
    {
        sc_logr_error(SC_PUB, "%s", "Alloc memory fail while send publish request.");

        goto proc_fail;
    }

    DLL_Init_Node(pstDllNode);
    pstPubData->pszURL = NULL;
    pstPubData->pszData = NULL;
    pstPubData->pszURL = dos_strndup(pszURL, SC_PUB_MAX_URL);
    if (DOS_ADDR_INVALID(pstPubData->pszURL))
    {
        sc_logr_error(SC_PUB, "%s", "Dump the url fail.");
        goto proc_fail;
    }

    if (DOS_ADDR_VALID(pszData))
    {
        pstPubData->pszData = dos_strndup(pszData, SC_PUB_MAX_DATA);
        if (DOS_ADDR_INVALID(pstPubData->pszData))
        {
            sc_logr_error(SC_PUB, "%s", "Dump the data fail.");
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

    sc_logr_notice(SC_PUB, "Publish request send succ. URL: %s, Data: %s", pszURL, DOS_ADDR_VALID(pszData) ? pszData : "");

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

    if (pthread_create(&g_stPubTaskList[0].pthPublishCurl, NULL, sc_pub_runtime_master, &g_stPubTaskList[0]) < 0)
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

S32 sc_pub_stop()
{
    g_ulPublishExit = DOS_TRUE;

    return DOS_SUCC;
}



#ifdef __cplusplus
}
#endif /* __cplusplus */

