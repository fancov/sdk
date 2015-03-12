#ifndef __PTS_GOAHEAD_H__
#define __PTS_GOAHEAD_H__

#ifdef  __cplusplus
extern "C"{
#endif

#define PTS_VISIT_FILELDS_COUNT 9
#define PTS_SWITCH_FILELDS_COUNT 12
#define PTS_CONFIG_FILELDS_COUNT 11
#define PTS_UPGRADES_FILELDS_COUNT 10
#define PTS_USER_COUNT 7

typedef struct tagSqliteParam
{
   S8 *pszBuff;
   U32 ulResCount;             /* 结果的个数 */
   U32 ulBuffLen;              /* 已经使用的大小 */
   U32 ulMallocSize;           /* 空间的大小 */

}PTS_SQLITE_PARAM_ST;

VOID *pts_goahead_service(VOID *arg);
S8 *pts_md5_encrypt(S8 *szUserName, S8 *szPassWord);
VOID pts_goAhead_free(S8 *pPoint);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif
