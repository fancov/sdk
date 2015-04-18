#ifndef __PTS_GOAHEAD_H__
#define __PTS_GOAHEAD_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <webs/webs.h>

#define PTS_VISIT_FILELDS_COUNT     10
#define PTS_SWITCH_FILELDS_COUNT    12
#define PTS_CONFIG_FILELDS_COUNT    11
#define PTS_UPGRADES_FILELDS_COUNT  10
#define PTS_USER_COUNT              7
#define PTS_PTC_UPGRADE_TIMER       1000 * 60 * 30
#define PTS_UPGRADE_PTC_COUNT       5
#define PTS_SELECT_PTC_MAX_COUNT    100
#define PTS_DELECT_USER_MAX_COUNT   10
#define PTS_LANG_ARRAY_SIZE         79

typedef struct tagSqliteParam
{
   webs_t wp;
   S8 *pszBuff;
   U32 ulResCount;             /* 结果的个数 */

}PTS_SQLITE_PARAM_ST;

typedef struct tagPingTimeoutParam
{
   U32     ulSeq;
   S8      szPtcId[PTC_ID_LEN+1];

}PTS_PING_TIMEOUT_PARAM_ST;

typedef struct tagUpgradePtc
{
   list_t           stNextNode;

   S8               szPtcId[PTC_ID_LEN + 1];

}PTS_UPGRADE_PTC_ST;

typedef struct tagPtcUpgradeParam
{
    PT_PTC_UPGRADE_ST *pstUpgrade;
    S8 *szUrl;

}PTS_PTC_UPGRADE_PARAM_ST;

VOID *pts_goahead_service(VOID *arg);
S8 *pts_md5_encrypt(S8 *szUserName, S8 *szPassWord);
VOID pts_goAhead_free(S8 *pPoint);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif

