
#ifndef __DB_SQLITE_H__
#define __DB_SQLITE_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#if DB_SQLITE

FUNCATTR S32 db_sqlite3_open(sqlite3 **pstHandle, S8 *pszFilePath, S8 **pszErrMsg);
FUNCATTR S32 db_sqlite3_close(sqlite3 *pstHandle, S8 **pszErrMsg);
FUNCATTR S32 db_sqlite3_query(sqlite3 *pstHandle, S8 *pszSQL, S32 (*callback)(VOID*, S32, S8**, S8**), VOID *pParamObj, S8 **pszErrMsg);
FUNCATTR S32 db_sqlite3_affect_row(sqlite3 *pstHandle, S8 **pszErrMsg);
FUNCATTR S32 db_sqlite3_transaction_begin(sqlite3 *pstHandle, S8 **pszErrMsg);
FUNCATTR S32 db_sqlite3_transaction_commit(sqlite3 *pstHandle, S8 **pszErrMsg);
FUNCATTR S32 db_sqlite3_transaction_rollback(sqlite3 *pstHandle, S8 **pszErrMsg);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*__DB_SQLITE_H__*/


