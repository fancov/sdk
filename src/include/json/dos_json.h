
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


#define JSON_ARRAY_SCAN(index, array, array_item) \
	for ((index)=0, (array_item) = json_array_at((array), (index)) \
			; (index)<json_array_length((array)) && ((array_item)) \
			; (index)++, (array_item) = json_array_at((array), (index)))



JSON_OBJ_ST *json_init(S8 *pszJsonString);
VOID json_deinit(JSON_OBJ_ST **ppstJsonObj);

const S8 *json_get_param(JSON_OBJ_ST *pstJsonObj, S8 *szKey);

S32 json_array_length(JSON_OBJ_ST *pstJsonObj);
const S8 *json_array_at(JSON_OBJ_ST *pstJsonArray, S32 ulIndex);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* end of __DOS_JSON_H__ */

