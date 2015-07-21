
#ifdef  __cplusplus
extern "C" {
#endif

#ifndef __HTTP_API_H__
#define __HTTP_API_H__

#if (INCLUDE_HTTP_API)

typedef            struct mg_connection       HTTP_HANDLE_ST;

#define            HTTP_API_HASH_SIZE         128
#define            HANDLE_NAME_LEN            128
#define            HTTP_BUFFER_BASE_SIZE      2048
#define            HTTP_QUERY_PARAM_NUM       32

typedef struct tagApiHandle{
    S8  szHandleName[HANDLE_NAME_LEN];
    U32 (*handle)(HTTP_HANDLE_ST *, list_t *);
}HTTP_API_HANDLE_ST;

typedef struct tagAPIArgv{
    S8  *pszName;
    S8  *pszVal;

    list_t stList;
}API_ARGV_ST;

U32 http_api_handle_init();
U32 http_api_handle_reg(S8 *pcPrefix, U32 (*handle)(HTTP_HANDLE_ST *, list_t *));
S32 http_api_write(HTTP_HANDLE_ST * pstWebHandle, S8 *fmt, ...);
S8 *http_api_get_var(list_t *pstArgvList, S8 *pcName);

U32 http_api_init();
U32 http_api_start();
U32 http_api_stop();

#if INCLUDE_LICENSE_SERVER
U32 http_lics_generate(HTTP_HANDLE_ST *pstHandle, list_t *pstArgvList);
#endif

#if INCLUDE_RES_MONITOR
U32 http_api_alarm_clear(HTTP_HANDLE_ST *pstHandle, list_t *pstArgvList);
#endif

#endif /* end of  INCLUDE_HTTP_API*/

#endif /* end of __HTTP_API_H__ */

#ifdef  __cplusplus
}
#endif


