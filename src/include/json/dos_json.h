
#ifndef __DOS_JSON_H__
#define __DOS_JSON_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

typedef struct tagJSONObj
{
    pthread_mutex_t  mutexObj;
    json_object      *jsonObj;
}JSON_OBJ_ST;

const S8 *json_get_param(JSON_OBJ_ST *pstJsonObj, S8 *szKey);
JSON_OBJ_ST *json_init(S8 *pszJsonString);
VOID json_deinit(JSON_OBJ_ST **ppstJsonObj);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* end of __DOS_JSON_H__ */

