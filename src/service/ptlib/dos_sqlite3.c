#ifdef  __cplusplus
extern "C" {
#endif

#include <pt/dos_sqlite3.h>
#include <dos.h>
#include <pt/pt.h>
#include <pthread.h>

#if INCLUDE_PTS

pthread_mutex_t g_mutex_sqlite = PTHREAD_MUTEX_INITIALIZER;

S32 dos_sqlite3_create_db(DOS_SQLITE_ST *pstMySqlite)
{
    S32 lRc;

    pthread_mutex_lock(&g_mutex_sqlite);
    lRc = sqlite3_open(pstMySqlite->pchDbPath, &pstMySqlite->pHandle);
    if (lRc != SQLITE_OK)
    {
        pt_logr_info("sqlite3 create error : %s", sqlite3_errmsg(pstMySqlite->pHandle));
        dos_sqlite3_close(pstMySqlite);
        pthread_mutex_unlock(&g_mutex_sqlite);
        return DOS_FAIL;
    }
    pt_logr_info("sqlite3 create succeed:%s", pstMySqlite->pchDbPath);
    pthread_mutex_unlock(&g_mutex_sqlite);

    return DOS_SUCC;
}

S32 dos_sqlite3_creat_table(DOS_SQLITE_ST *pstMySqlite, S8 *pchSql)
{
    S32 lRc;
    S8 *pErrMsg;
    pthread_mutex_lock(&g_mutex_sqlite);
    lRc = sqlite3_exec(pstMySqlite->pHandle, pchSql , NULL , NULL ,&pErrMsg);
    if(lRc != SQLITE_OK)
    {
        if (1 == sqlite3_errcode(pstMySqlite->pHandle))
        {
            pt_logr_debug("sqlite3 table exited");
            pthread_mutex_unlock(&g_mutex_sqlite);

            return PT_SLITE3_EXISTED;
        }
        else
        {
            pt_logr_info("can't open database: %s", sqlite3_errmsg(pstMySqlite->pHandle));
            dos_sqlite3_close(pstMySqlite);
            pthread_mutex_unlock(&g_mutex_sqlite);

            return PT_SLITE3_FAIL;
        }
    }

    pt_logr_info("sqlite3 create table succeed");
    pthread_mutex_unlock(&g_mutex_sqlite);

    return PT_SLITE3_SUCC;
}

S32 dos_sqlite3_exec(DOS_SQLITE_ST *pstMySqlite, S8 *pchSql)
{
    S32 lRc;
    S8 *pErrMsg;
    pthread_mutex_lock(&g_mutex_sqlite);
    lRc = sqlite3_exec(pstMySqlite->pHandle, pchSql, NULL, NULL, &pErrMsg);
    if(lRc != SQLITE_OK)
    {
        pt_logr_info("operation database fail: %s", sqlite3_errmsg(pstMySqlite->pHandle));
        dos_sqlite3_close(pstMySqlite);
        pthread_mutex_unlock(&g_mutex_sqlite);
        dos_sqlite3_create_db(pstMySqlite);

        return DOS_FAIL;
    }
    pthread_mutex_unlock(&g_mutex_sqlite);
    return DOS_SUCC;
}

S32 dos_sqlite3_exec_callback(DOS_SQLITE_ST *pstMySqlite, S8 *pchSql, my_sqlite_callback callback, void *pFristArg)
{
    S8 *pErrMsg;
    S32 lRc;
    pthread_mutex_lock(&g_mutex_sqlite);
    lRc = sqlite3_exec(pstMySqlite->pHandle, pchSql, callback, pFristArg, &pErrMsg);
    if(lRc != SQLITE_OK)
    {
        logr_debug("operation database  fail : %d, %s\n", lRc, sqlite3_errstr(lRc));
        dos_sqlite3_close(pstMySqlite);
        pthread_mutex_unlock(&g_mutex_sqlite);
        dos_sqlite3_create_db(pstMySqlite);

        return DOS_FAIL;
    }
    pthread_mutex_unlock(&g_mutex_sqlite);
    return DOS_SUCC;
}

S32 dos_sqlite3_close(DOS_SQLITE_ST *pstMySqlite)
{
    S32 lRc;

    lRc = sqlite3_close(pstMySqlite->pHandle);
    if (lRc != SQLITE_OK)
    {
        pt_logr_info("sqlite3 close error: %s", sqlite3_errmsg(pstMySqlite->pHandle));
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/*
S32 dos_sqlite3_record_is_exist(DOS_SQLITE_ST *pstMySqlite, S8 *pchSql)
{
    sqlite3_stmt *pstmt;
    S32 lCount;
    S32 lRes = 0;

    pthread_mutex_lock(&g_mutex_sqlite);
    lRes = sqlite3_prepare(pstMySqlite->pHandle, pchSql, dos_strlen(pchSql), &pstmt, NULL);
    if (lRes != SQLITE_OK)
    {
        dos_sqlite3_close(pstMySqlite);
        pthread_mutex_unlock(&g_mutex_sqlite);
        dos_sqlite3_create_db(pstMySqlite);
        return DOS_FALSE;
    }

    lRes = sqlite3_step(pstmt);

    lCount = sqlite3_column_int(pstmt,0);
    sqlite3_finalize(pstmt);

    if(lCount > 0)
    {
        pthread_mutex_unlock(&g_mutex_sqlite);
        return DOS_TRUE;
    }
    pthread_mutex_unlock(&g_mutex_sqlite);
    return DOS_FALSE;
}
*/

S32 dos_get_count_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
{
    U32 *pulCount = (U32 *)para;
    (*pulCount)++;

    return 0;
}

S32 dos_sqlite3_record_is_exist(DOS_SQLITE_ST *pstMySqlite, S8 *pchSql)
{
    U32 count = 0;
    S32 lRes = 0;

    lRes = dos_sqlite3_exec_callback(pstMySqlite, pchSql, dos_get_count_callback, &count);
    if (lRes != DOS_SUCC)
    {
        return -1;
    }

    if (count > 0)
    {
        return DOS_TRUE;
    }

    return DOS_FALSE;
}

#endif

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */
