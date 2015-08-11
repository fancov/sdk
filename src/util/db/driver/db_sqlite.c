#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
#include <dos.h>
#include <dos/dos_config.h>
#include "db_sqlite.h"

#if DB_SQLITE

FUNCATTR S32 db_sqlite3_open(sqlite3 **pstHandle, S8 *pszFilePath, S8 **pszErrMsg)
{
    if (!pstHandle || !pszFilePath)
    {
        db_assert(0);

        return -1;
    }

    if (sqlite3_open(pszFilePath, pstHandle) != SQLITE_OK)
    {
        db_assert(0);

        return -1;
    }

    return 0;
}

FUNCATTR S32 db_sqlite3_close(sqlite3 *pstHandle, S8 **pszErrMsg)
{
    if (!pstHandle)
    {
        db_assert(0);

        return -1;
    }

    sqlite3_close(pstHandle);

    return 0;
}

FUNCATTR S32 db_sqlite3_query(sqlite3 *pstHandle, S8 *pszSQL, S32 (*callback)(VOID*, S32, S8**, S8**), VOID *pParamObj, S8 **pszErrMsg)
{
    if (sqlite3_exec(pstHandle, pszSQL, callback, pParamObj, pszErrMsg) != SQLITE_OK)
    {
        return DB_ERR_QUERY_FETCH_FAIL;
    }

    return DB_ERR_SUCC;
}

FUNCATTR S32 db_sqlite3_affect_row(sqlite3 *pstHandle, S8 **pszErrMsg)
{
    /* ²»Ö§³Ö */
    return -1;
}

FUNCATTR S32 db_sqlite3_transaction_begin(sqlite3 *pstHandle, S8 **pszErrMsg)
{
    if (!pstHandle)
    {
        db_assert(0);

        return -1;
    }

    return db_sqlite3_query(pstHandle, "BEGIN ", NULL, NULL, pszErrMsg);
}

FUNCATTR S32 db_sqlite3_transaction_commit(sqlite3 *pstHandle, S8 **pszErrMsg)
{
    if (!pstHandle)
    {
        db_assert(0);

        return -1;
    }

    return db_sqlite3_query(pstHandle, "COMMIT ", NULL, NULL, pszErrMsg);
}

FUNCATTR S32 db_sqlite3_transaction_rollback(sqlite3 *pstHandle, S8 **pszErrMsg)
{
    if (!pstHandle)
    {
        db_assert(0);

        return -1;
    }

    return db_sqlite3_query(pstHandle, "ROLLBACK ", NULL, NULL, pszErrMsg);
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


