#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_object_private.h>
#include <json-c/linkhash.h>
#include <json/dos_json.h>

const S8 *json_get_param(JSON_OBJ_ST *pstJsonObj, S8 *szKey)
{
    const S8 *pszVal;
    if (DOS_ADDR_INVALID(pstJsonObj)
        || DOS_ADDR_INVALID(pstJsonObj->jsonObj)
        || DOS_ADDR_INVALID(szKey))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    pthread_mutex_lock(&pstJsonObj->mutexObj);
    struct lh_table *pstTable = pstJsonObj->jsonObj->o.c_object;
    if (DOS_ADDR_INVALID(pstTable))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&pstJsonObj->mutexObj);
        return NULL;
    }

    struct lh_entry *pstEntry = lh_table_lookup_entry(pstTable, szKey);
    if (DOS_ADDR_INVALID(pstEntry))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&pstJsonObj->mutexObj);
        return NULL;
    }

    json_object *jsonVal = (json_object *)pstEntry->v;
    if (DOS_ADDR_INVALID(jsonVal ))
    {   
        DOS_ASSERT(0);

        pthread_mutex_unlock(&pstJsonObj->mutexObj);
        return NULL;
    }

    pszVal = json_object_get_string(jsonVal);
    pthread_mutex_unlock(&pstJsonObj->mutexObj);

    return pszVal;
}

JSON_OBJ_ST *json_init(S8 *pszJsonString)
{
    JSON_OBJ_ST *pstJSONObj = NULL;

    if (DOS_ADDR_INVALID(pszJsonString))
    {
        DOS_ASSERT(0);

        return NULL;
    }

    pstJSONObj = (JSON_OBJ_ST *)dos_dmem_alloc(sizeof(JSON_OBJ_ST));
    if (DOS_ADDR_INVALID(pstJSONObj))
    {
        DOS_ASSERT(0);

        return NULL;
    }

    pthread_mutex_init(&pstJSONObj->mutexObj, NULL);
    pthread_mutex_lock(&pstJSONObj->mutexObj);
    pstJSONObj->jsonObj = json_tokener_parse(pszJsonString);
    if (DOS_ADDR_INVALID(pstJSONObj->jsonObj))
    {
        dos_dmem_free(pstJSONObj);
        pstJSONObj = NULL;
        pthread_mutex_unlock(&pstJSONObj->mutexObj);
        pthread_mutex_destroy(&pstJSONObj->mutexObj);

        return NULL;
    }
    pthread_mutex_unlock(&pstJSONObj->mutexObj);

    return pstJSONObj;
}

VOID json_deinit(JSON_OBJ_ST **ppstJsonObj)
{
    JSON_OBJ_ST *pstJsonObj = NULL;

    if (DOS_ADDR_INVALID(ppstJsonObj) || DOS_ADDR_INVALID(*ppstJsonObj))
    {
        DOS_ASSERT(0);
        return;
    }

    pstJsonObj = *ppstJsonObj;

    pthread_mutex_lock(&pstJsonObj->mutexObj);
    json_object_put(pstJsonObj->jsonObj);
    pstJsonObj->jsonObj = NULL;
    pthread_mutex_unlock(&pstJsonObj->mutexObj);

    pthread_mutex_destroy(&pstJsonObj->mutexObj);
    dos_dmem_free(pstJsonObj);
    pstJsonObj = NULL;
    *ppstJsonObj = NULL;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

