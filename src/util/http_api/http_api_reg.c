#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if INCLUDE_HTTP_API

#include <http_api.h>

#if DEBUG
U32 http_api_test_handle(HTTP_HANDLE_ST *pstHandle, list_t *pstArgvList)
{
    list_t *pstListNode = NULL;
    API_ARGV_ST *pstArgv;
    S32 lCnt = 0;
    S8 *pszVal;

    if (!pstArgvList)
    {
        http_api_write(pstHandle, "Just a test. There is no data!");
        return DOS_SUCC;
    }

    pszVal = http_api_get_var(pstArgvList, "id");
    http_api_write(pstHandle, "Fount an parameter. Name: id, value: %s!", pszVal);

    pstListNode = pstArgvList->next;
    while (pstListNode != pstArgvList)
    {
        pstArgv = dos_list_entry(pstListNode, API_ARGV_ST, stList);
        if (!pstArgv)
        {
            continue;
        }

        lCnt++;
        http_api_write(pstHandle, "Test. Parameter %d, Name: %s, Value: %s <br>", lCnt, pstArgv->pszName, pstArgv->pszVal);

        pstListNode = pstListNode->next;
        if (!pstListNode)
        {
            break;
        }
    }

    http_api_write(pstHandle, "Just a test. Parameters : %d!", lCnt);

    return DOS_SUCC;
}
#endif

U32 http_api_handle_init()
{
#if DEBUG
    http_api_handle_reg("/test", http_api_test_handle);
#endif
#if INCLUDE_LICENSE_SERVER
    http_api_handle_reg("/generate", http_lics_generate);
#endif

    return DOS_SUCC;
}



#endif /* end of INCLUDE_HTTP_API */

#ifdef __cplusplus
}
#endif /* __cplusplus */


