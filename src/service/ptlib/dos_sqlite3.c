#ifdef  __cplusplus
extern "C"{
#endif

#include <ptc_pts/dos_sqlite3.h>
#include <dos.h>

BOOL dos_sqlite3_create_db(DOS_SQLITE_ST *pstMySqlite)
{
    S32 lRc;

    lRc = sqlite3_open(pstMySqlite->pchDbPath, &pstMySqlite->pHandle);
    if (lRc != SQLITE_OK){
		alert("sqlite3 create error:%s", sqlite3_errmsg(pstMySqlite->pHandle));
        dos_sqlite3_close(*pstMySqlite);
        return DOS_FAIL;
    }
	alert("sqlite3 create succeed:%s", pstMySqlite->pchDbPath);

    return DOS_SUCC;
}

BOOL dos_sqlite3_creat_table(DOS_SQLITE_ST stMySqlite, S8 *pchSql)
{
    S32 lRc;
    S8 *pErrMsg;
    lRc = sqlite3_exec(stMySqlite.pHandle, pchSql , NULL , NULL ,&pErrMsg);
    if(lRc != SQLITE_OK)
    {
		alert("can't open database: %s", sqlite3_errmsg(stMySqlite.pHandle));
        dos_sqlite3_close(stMySqlite);
        return DOS_FAIL;
    }
	
	alert("sqlite3 create table succeed");

    return DOS_SUCC;
}

BOOL dos_sqlite3_exec(DOS_SQLITE_ST stMySqlite, S8 *pchSql)
{
    S32 lRc;
    S8 *pErrMsg;

    lRc = sqlite3_exec(stMySqlite.pHandle, pchSql, NULL, NULL, &pErrMsg);
    if(lRc != SQLITE_OK)
    {
        alert("operation database  fail: %s", sqlite3_errmsg(stMySqlite.pHandle));
        dos_sqlite3_close(stMySqlite);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

BOOL dos_sqlite3_exec_callback(DOS_SQLITE_ST stMySqlite, S8 *pchSql, my_sqlite_callback callback, void *pFristArg)
{
    S8 *pErrMsg;
    S32 lRc;

    lRc = sqlite3_exec(stMySqlite.pHandle, pchSql, callback, pFristArg, &pErrMsg);

    if(lRc != SQLITE_OK)
    {
		alert("operation database  fail: %s", sqlite3_errmsg(stMySqlite.pHandle));
        dos_sqlite3_close(stMySqlite);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

BOOL dos_sqlite3_close(DOS_SQLITE_ST stMySqlite)
{
    S32 lRc;

    lRc = sqlite3_close(stMySqlite.pHandle);
    if (lRc != SQLITE_OK){
		alert("sqlite3 close error: %s", sqlite3_errmsg(stMySqlite.pHandle));
        return DOS_FAIL;
    }

	alert("sqlite3 close db succeed");

    return DOS_SUCC;
}

BOOL dos_sqlite3_record_is_exist(DOS_SQLITE_ST stMySqlite, S8 *pchSql)
{
    sqlite3_stmt *pstmt;
    S32 lCount;

    sqlite3_prepare(stMySqlite.pHandle, pchSql, strlen(pchSql), &pstmt, NULL);
    sqlite3_step(pstmt);
    lCount = sqlite3_column_int(pstmt,0);
    sqlite3_finalize(pstmt);

    if(lCount > 0)
    {
        return DOS_TRUE;
    }
    return DOS_FALSE;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */