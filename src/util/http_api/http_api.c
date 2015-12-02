
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if INCLUDE_HTTP_API

#include "mongoose.h"
#include <http_api.h>

#if INCLUDE_RES_MONITOR
#define DOS_HTTP_API_PORT  2804
#endif

struct mg_server    *g_pstMonServer           = NULL;
pthread_t           g_pthHttpApi;
BOOL                g_blRunning               = DOS_FALSE;
U16                 g_usHttpApiLintenPort     = DOS_HTTP_API_PORT;
S8                  g_szHttpApiLintenAddr[32] = { 0 };
HASH_TABLE_S        *g_pstAPIHandleTable      = NULL;
pthread_mutex_t     g_mutexAPIHandleTable     = PTHREAD_MUTEX_INITIALIZER;

S8 *http_api_get_var(list_t *pstArgvList, S8 *pcName)
{
    list_t *pstListNode = NULL;
    API_ARGV_ST *pstArgv;
    S8 *pszVal = NULL;

    if (!pstArgvList || !pcName)
    {
        return NULL;
    }

    pstListNode = pstArgvList->next;
    while (pstListNode != pstArgvList)
    {
        pstArgv = dos_list_entry(pstListNode, API_ARGV_ST, stList);
        if (!pstArgv)
        {
            continue;
        }

        if (dos_strcmp(pcName, pstArgv->pszName) == 0)
        {
            pszVal = pstArgv->pszVal;
            break;
        }

        pstListNode = pstListNode->next;
        if (!pstListNode)
        {
            break;
        }
    }

    return pszVal;
}

S32 http_api_write(HTTP_HANDLE_ST * pstWebHandle, S8 *fmt, ...)
{
    va_list argptr;
    S32 lRet = 0;
    S8  *pcBuffer = NULL;

    if (DOS_ADDR_INVALID(pstWebHandle)
        || DOS_ADDR_INVALID(fmt))
    {
        DOS_ASSERT(0);
        return -1;
    }

    pcBuffer = dos_dmem_alloc(HTTP_BUFFER_BASE_SIZE * 4);
    if (DOS_ADDR_INVALID(pcBuffer))
    {
        DOS_ASSERT(0);
        return 0;
    }

    va_start (argptr, fmt);
    lRet = vsnprintf(pcBuffer, HTTP_BUFFER_BASE_SIZE, fmt, argptr);
    va_end (argptr);

    lRet = mg_send_data(pstWebHandle, pcBuffer, lRet);

    dos_dmem_free(pcBuffer);
    pcBuffer = NULL;

    return lRet;
}


static U32 http_api_hash_func(const S8 *pszName)
{
    U32 ulHashIndex = 0;
    U32 i = 0;

    if (DOS_ADDR_INVALID(pszName))
    {
        return 0;
    }

    for (ulHashIndex=0, i=0; i<dos_strlen(pszName); i++)
    {
        ulHashIndex += (pszName[0] << 2);
    }

    if (0 == ulHashIndex)
    {
        return 0;
    }

    return ulHashIndex % HANDLE_NAME_LEN;
}

static S32 http_api_handle_find_cb(VOID *pKey, HASH_NODE_S *pNode)
{
    HTTP_API_HANDLE_ST *pstHandle;
    S8 *pszName = NULL;

    if (DOS_ADDR_INVALID(pKey)
        || DOS_ADDR_INVALID(pNode)
        || DOS_ADDR_INVALID(pNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHandle = pNode->pHandle;
    pszName = (S8 *)pKey;

    if (0 == dos_strnicmp(pszName, pstHandle->szHandleName, sizeof(pstHandle->szHandleName)))
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

U32 http_api_handle_proc(HTTP_HANDLE_ST *pstHandle, U32 *pulErrNo)
{
    HASH_NODE_S *pNode = NULL;
    HTTP_API_HANDLE_ST *pstApiHandle = NULL;
    U32 ulHashIndex;
    S32 lKeyCnt = 0, i;
    S8  *pWord = NULL;
    S8  *pszKeyWord[HTTP_QUERY_PARAM_NUM] = { NULL };
    S8  *pszName = NULL, *pszVal = NULL;
    list_t stArgvList;
    list_t *pstList = NULL;
    API_ARGV_ST *pstArgv = NULL;

    if (DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstHandle->uri))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    logr_debug("Recv http apit request. Prefix: %s, Query: %s, Conten Len:%d"
                , pstHandle->uri
                , pstHandle->query_string
                , pstHandle->content_len);

    ulHashIndex = http_api_hash_func(pstHandle->uri);
    pNode = hash_find_node(g_pstAPIHandleTable, ulHashIndex, (VOID *)pstHandle->uri, http_api_handle_find_cb);
    if (DOS_ADDR_INVALID(pNode)
        || DOS_ADDR_INVALID(pNode->pHandle))
    {
        *pulErrNo = 0;

        return DOS_FAIL;
    }

    pstApiHandle = pNode->pHandle;
    if (DOS_ADDR_INVALID(pstApiHandle->handle))
    {
        *pulErrNo = 0;

        return DOS_FAIL;
    }

    /* 获取 key=value 字符串 */
    lKeyCnt = 0;
    pWord = strtok((S8 *)pstHandle->query_string, "&");
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

    dos_list_init(&stArgvList);
    if (lKeyCnt > 0)
    {
        for (i=0; i<HTTP_QUERY_PARAM_NUM; i++)
        {
            if (!pszKeyWord[i])
            {
                continue;
            }

            pszName = pszKeyWord[i];

            /* 如果没有等号就有问题 */
            pWord = dos_strstr(pszKeyWord[i], "=");
            if (!pWord)
            {
                goto free_res;
            }
            *pWord = '\0';

            /* 没有值也不处理了 */
            pszVal = ++pWord;
            if (!pszVal)
            {
                goto free_res;
            }

            pstArgv = dos_dmem_alloc(sizeof(API_ARGV_ST));
            if (!pstArgv)
            {
                goto free_res;
            }

            pstArgv->pszName = pszName;
            pstArgv->pszVal = pszVal;

            dos_list_add_tail(&stArgvList, &pstArgv->stList);

            continue;
free_res:
            dos_dmem_free(pszKeyWord[i]);
        }
    }

    pstApiHandle->handle(pstHandle, &stArgvList);

    /* 释放资源 */
    while (1)
    {
        if (dos_list_is_empty(&stArgvList))
        {
            break;
        }

        pstList = dos_list_fetch(&stArgvList);
        if (!pstList)
        {
            break;
        }

        pstArgv = dos_list_entry(pstList, API_ARGV_ST, stList);
        if (!pstArgv)
        {
            continue;
        }

        dos_dmem_free(pstArgv->pszName);
        pstArgv->pszName = NULL;
        pstArgv->pszVal = NULL;
        dos_dmem_free(pstArgv);
        pstArgv = NULL;
    }

    return DOS_SUCC;

}

U32 http_api_handle_reg(S8 *pcPrefix, U32 (*handle)(HTTP_HANDLE_ST *, list_t *))
{
    U32 ulHashIndex;
    HASH_NODE_S *pNode = NULL;
    HTTP_API_HANDLE_ST *pstHandleNode = NULL;

    if (DOS_ADDR_INVALID(pcPrefix)
        || DOS_ADDR_INVALID(handle))
    {
        return DOS_FALSE;
    }

    ulHashIndex = http_api_hash_func(pcPrefix);

    pNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHandleNode = (HTTP_API_HANDLE_ST *)dos_dmem_alloc(sizeof(HTTP_API_HANDLE_ST));
    if (DOS_ADDR_INVALID(pstHandleNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    HASH_Init_Node(pNode);
    pNode->pHandle = pstHandleNode;

    dos_memcpy(pstHandleNode->szHandleName, pcPrefix, sizeof(pstHandleNode->szHandleName));
    pstHandleNode->szHandleName[sizeof(pstHandleNode->szHandleName) - 1] = '\0';
    pstHandleNode->handle = handle;

    pthread_mutex_lock(&g_mutexAPIHandleTable);
    hash_add_node(g_pstAPIHandleTable, pNode, ulHashIndex, NULL);
    pthread_mutex_unlock(&g_mutexAPIHandleTable);

    return DOS_SUCC;
}


static S32 http_event_handler(HTTP_HANDLE_ST *pstHandle, enum mg_event enEvent)
{
    S8  acRsp[128];
    U32 ulErrNo;

    if (DOS_ADDR_INVALID(pstHandle))
    {
        DOS_ASSERT(0);

        return MG_FALSE;
    }

    if (MG_REQUEST == enEvent)
    {
        if (http_api_handle_proc(pstHandle, &ulErrNo) != DOS_SUCC)
        {
            dos_snprintf(acRsp, sizeof(acRsp), "{\"errno\":\"%d\", \"msg\":\"Process FAIL.\"}", ulErrNo);
            mg_send_data(pstHandle, acRsp, dos_strlen(acRsp));
        }

        return MG_TRUE;
    }
    else if (MG_AUTH == enEvent)
    {
        return MG_TRUE;
    }
    else
    {
        return MG_FALSE;
    }
}


VOID* http_api_runtime(void *ptr)
{
    logr_debug("Http API Server start on port %s\n", mg_get_option(g_pstMonServer, "listening_port"));

    g_blRunning = DOS_TRUE;
    for (;;)
    {
        if (!g_blRunning)
        {
            logr_debug("%s", "Detect request stop Http API Server. Exit!");

            break;
        }

        mg_poll_server(g_pstMonServer, 1000);
    }

    mg_destroy_server(&g_pstMonServer);

    g_blRunning = DOS_FALSE;

    logr_debug("%s", "API Server exited!");

    return NULL;
}

U32 http_api_init()
{
    S8 szBuffer[128] = { 0 };

    g_pstAPIHandleTable = hash_create_table(HTTP_API_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstAPIHandleTable))
    {
        DOS_ASSERT(0);

        logr_error("%s", "Create the HTTP API Handle hash table FAIL.");
        return DOS_FAIL;
    }

    g_pstMonServer = mg_create_server(NULL, http_event_handler);
    if (DOS_ADDR_INVALID(g_pstMonServer))
    {
        DOS_ASSERT(0);

        logr_error("%s", "Create the HTTP API Server FAIL.");

        hash_delete_table(g_pstAPIHandleTable, NULL);
        return DOS_FAIL;
    }

    dos_snprintf(szBuffer, sizeof(szBuffer), "%u", g_usHttpApiLintenPort);
    mg_set_option(g_pstMonServer, "listening_port", szBuffer);
    dos_snprintf(szBuffer, sizeof(szBuffer), "-0.0.0.0, +%s", g_szHttpApiLintenAddr[0] == '\0' ? "127.0.0.1" : g_szHttpApiLintenAddr);
    mg_set_option(g_pstMonServer, "access_control_list", szBuffer);

    logr_info("Init the API Server on port %u.ACL:%s", g_usHttpApiLintenPort, szBuffer);

    http_api_handle_init();

    return DOS_TRUE;
}

U32 http_api_start()
{
    if (pthread_create(&g_pthHttpApi, NULL, http_api_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 http_api_stop()
{
    g_blRunning = DOS_FAIL;

    return DOS_SUCC;
}


#endif /* end of INCLUDE_HTTP_API */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

