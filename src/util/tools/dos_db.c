/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: dos_db.c
 *
 *  创建时间: 2014年12月17日20:52:58
 *  作    者: Larry
 *  描    述: 数据操作相关API
 *  修改历史:
 */

#ifndef __DOS_DB_H__
#define __DOS_DB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <sql.h>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201)
#include <sqlext.h>
#pragma warning(pop)
#else
#include <sqlext.h>
#endif
#include <sqltypes.h>

#if (ODBCVER < 0x0300)
#define SQL_NO_DATA SQL_SUCCESS
#endif

/* include private header files */

/* define marcos */

/* define enums */

/* define structs */

/* declare functions */

DLLEXPORT DOS_ODBC_HANDLE_ST *dos_odbc_handle_new(const S8 *pszDsn, const S8 *pszUsername, const S8 *pszPassword)
{
    DOS_ODBC_HANDLE_ST *pstNewHandle = NULL;

    if (!(pstNewHandle = malloc(sizeof(*pstNewHandle))))
    {
        goto err;
    }

    memset(pstNewHandle, 0, sizeof(*pstNewHandle));

    if (!pszDsn)
    {
        goto err;
    }

    if (!(pstNewHandle->pszDNS = dos_strndup(pszDsn, dos_strlen(pszDsn) + 1)))
    {
        goto err;
    }

    if (pszUsername)
    {
        if (!(pstNewHandle->pszUsername = dos_strndup(pszUsername, dos_strlen(pszUsername) + 1)))
        {
            goto err;
        }
    }

    if (pszPassword)
    {
        if (!(pstNewHandle->pszPassword = dos_strndup(pszPassword, dos_strlen(pszPassword) + 1)))
        {
            goto err;
        }
    }

    pstNewHandle->hEnv = SQL_NULL_HANDLE;
    pstNewHandle->enState = SC_ODBC_STATE_INIT;
    pstNewHandle->ulAffectedRows = 0;
    pstNewHandle->ulNumRetries = SC_DEFAULT_ODBC_RETRIES;

    return pstNewHandle;

  err:
    if (pstNewHandle)
    {
        if (pstNewHandle->pszDNS)
        {
            dos_dmem_free(pstNewHandle->pszDNS);
        }

        if (pstNewHandle->pszUsername)
        {
            dos_dmem_free(pstNewHandle->pszUsername);
        }

        if (pstNewHandle->pszPassword)
        {
            dos_dmem_free(pstNewHandle->pszPassword);
        }

        if (pstNewHandle)
        {
            dos_dmem_free(pstNewHandle);
        }
    }

    return NULL;
}

DLLEXPORT VOID dos_odbc_set_num_retries(DOS_ODBC_HANDLE_ST *pstHandle, S32 lNumRetries)
{
    if (pstHandle)
    {
        pstHandle->ulNumRetries = lNumRetries;
    }
}

DLLEXPORT U32 dos_odbc_handle_disconnect(DOS_ODBC_HANDLE_ST *pstHandle)
{

    S32 lResult;

    if (!pstHandle)
    {
        return DOS_FAIL;
    }

    if (SC_ODBC_STATE_CONNECTED == pstHandle->enState)
    {
        lResult = SQLDisconnect(pstHandle->hCon);
        if (lResult == DOS_SUCC)
        {
            logr_info("Disconnected %d from [%s]", lResult, pstHandle->pszDNS);
        }
        else
        {
            logr_error("Error Disconnecting [%s]", pstHandle->pszDNS);
        }
    }

    pstHandle->enState = SC_ODBC_STATE_DOWN;

    return DOS_SUCC;
}



static U32 dos_init_odbc_handles(DOS_ODBC_HANDLE_ST *pstHandle, BOOL bReinit)
{
    S32 lResult;

    if (!pstHandle)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* if handle is already initialized, and we're supposed to reinit - free old handle first */
    if (bReinit == DOS_TRUE && pstHandle->hEnv != SQL_NULL_HANDLE)
    {
        SQLFreeHandle(SQL_HANDLE_DBC, pstHandle->hCon);
        SQLFreeHandle(SQL_HANDLE_ENV, pstHandle->hEnv);
        pstHandle->hEnv = SQL_NULL_HANDLE;
    }

    if (pstHandle->hEnv == SQL_NULL_HANDLE)
    {
        lResult = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &pstHandle->hEnv);

        if ((lResult != SQL_SUCCESS) && (lResult != SQL_SUCCESS_WITH_INFO))
        {
            logr_error("Error AllocHandle");
            pstHandle->hEnv = SQL_NULL_HANDLE; /* Reset handle value, just in case */
            return DOS_FAIL;
        }

        lResult = SQLSetEnvAttr(pstHandle->hEnv, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);

        if ((lResult != SQL_SUCCESS) && (lResult != SQL_SUCCESS_WITH_INFO))
        {
            logr_error("Error SetEnv\n");
            SQLFreeHandle(SQL_HANDLE_ENV, pstHandle->hEnv);
            pstHandle->hEnv = SQL_NULL_HANDLE; /* Reset handle value after it's freed */
            return DOS_FAIL;
        }

        lResult = SQLAllocHandle(SQL_HANDLE_DBC, pstHandle->hEnv, &pstHandle->hCon);

        if ((lResult != SQL_SUCCESS) && (lResult != SQL_SUCCESS_WITH_INFO))
        {
            logr_error("Error AllocHDB %d\n", lResult);
            SQLFreeHandle(SQL_HANDLE_ENV, pstHandle->hEnv);
            pstHandle->hEnv = SQL_NULL_HANDLE; /* Reset handle value after it's freed */
            return DOS_FAIL;
        }
        SQLSetConnectAttr(pstHandle->hCon, SQL_LOGIN_TIMEOUT, (SQLPOINTER *) 10, 0);
    }

    return DOS_SUCC;
}

DLLEXPORT U32 dos_db_is_up(DOS_ODBC_HANDLE_ST *pstHandle)
{
    S32 lRet = 0;
    SQLHSTMT stmt = NULL;
    SQLLEN m = 0;
    S32 lResult;
    U32 ulReconn = 0;
    S8 *pszErrStr = NULL;
    SQLCHAR sql[255] = "";
    S32 lMaxTries = SC_DEFAULT_ODBC_RETRIES;
    S32 lCode = 0;
    SQLRETURN rc;
    SQLSMALLINT nresultcols;


    if (pstHandle)
    {
        lMaxTries = pstHandle->ulNumRetries;
        if (lMaxTries < 1)
        {
            lMaxTries = SC_DEFAULT_ODBC_RETRIES;
        }
    }

  top:

    if (!pstHandle)
    {
        logr_info("No DB Handle");
        goto done;
    }

    if (pstHandle->blIsOracle)
    {
        dos_strcpy((S8 *) sql, "select 1 from dual");
    }
    else if (pstHandle->blIsFirebird)
    {
        dos_strcpy((S8 *) sql, "select first 1 * from RDB$RELATIONS");
    }
    else
    {
        dos_strcpy((S8 *) sql, "select 1");
    }

    if (SQLAllocHandle(SQL_HANDLE_STMT, pstHandle->hCon, &stmt) != SQL_SUCCESS)
    {
        lCode = __LINE__;
        goto error;
    }

    SQLSetStmtAttr(stmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)30, 0);

    if (SQLPrepare(stmt, sql, SQL_NTS) != SQL_SUCCESS)
    {
        lCode = __LINE__;
        goto error;
    }

    lResult = SQLExecute(stmt);

    if (lResult != SQL_SUCCESS && lResult != SQL_SUCCESS_WITH_INFO)
    {
        lCode = __LINE__;
        goto error;
    }

    SQLRowCount(stmt, &m);
    rc = SQLNumResultCols(stmt, &nresultcols);
    if (rc != SQL_SUCCESS)
    {
        lCode = __LINE__;
        goto error;
    }
    lRet = (int) nresultcols;
    /* determine statement type */
    if (nresultcols <= 0)
    {
        /* statement is not a select statement */
        lCode = __LINE__;
        goto error;
    }

    goto done;

  error:
    pszErrStr = dos_odbc_handle_get_error(pstHandle, stmt);

    /* Make sure to free the handle before we try to reconnect */
    if (stmt)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        stmt = NULL;
    }

    ulReconn = dos_odbc_handle_connect(pstHandle);

    lMaxTries--;
    if (!lMaxTries)
    {
        logr_notice("DB Connection fail, give up retry.");
        goto done;
    }

    logr_notice("DB Connection fail, will bi retry after 10s. Error Msg:%s. (line:%d)", pszErrStr, lCode);

    if (pszErrStr)
    {
        dos_dmem_free(pszErrStr);
        pszErrStr = NULL;
    }
    dos_task_delay(10 * 1000);
    goto top;

  done:

    if (pszErrStr)
    {
        dos_dmem_free(pszErrStr);
        pszErrStr = NULL;
    }

    if (stmt)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }

    return lRet;
}

DLLEXPORT U32 dos_odbc_statement_handle_free(SC_ODBC_STAT_HANDLE_ST *stmt)
{
    if (!stmt || !*stmt)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, *stmt);
    *stmt = NULL;
    return DOS_SUCC;
}


DLLEXPORT U32 dos_odbc_handle_connect(DOS_ODBC_HANDLE_ST *pstHandle)
{
    S32 lResult;
    SQLINTEGER err;
    S16 mlen;
    unsigned S8 msg[200] = "", stat[10] = "";
    SQLSMALLINT valueLength = 0;
    int i = 0;

    dos_init_odbc_handles(pstHandle, DOS_FALSE); /* Init ODBC handles, if they are already initialized, don't do it again */

    if (pstHandle->enState == SC_ODBC_STATE_CONNECTED)
    {
        dos_odbc_handle_disconnect(pstHandle);
        logr_debug("Re-connecting %s\n", pstHandle->pszDNS);
    }

    logr_debug("Connecting %s\n", pstHandle->pszDNS);

    if (!strstr(pstHandle->pszDNS, "DRIVER"))
    {
        lResult = SQLConnect(pstHandle->hCon
                            , (SQLCHAR *) pstHandle->pszDNS
                            , SQL_NTS
                            , (SQLCHAR *) pstHandle->pszUsername
                            , SQL_NTS
                            , (SQLCHAR *) pstHandle->pszPassword
                            , SQL_NTS);
    }
    else
    {
        SQLCHAR outstr[1024] = { 0 };
        SQLSMALLINT outstrlen = 0;
        lResult = SQLDriverConnect(pstHandle->hCon
                                    , NULL
                                    , (SQLCHAR *) pstHandle->pszDNS
                                    , (SQLSMALLINT) strlen(pstHandle->pszDNS)
                                    , outstr, sizeof(outstr)
                                    , &outstrlen
                                    , SQL_DRIVER_NOPROMPT);
    }

    if ((lResult != SQL_SUCCESS) && (lResult != SQL_SUCCESS_WITH_INFO))
    {
        S8 *pszErrString;
        if ((pszErrString = dos_odbc_handle_get_error(pstHandle, NULL)))
        {
            logr_error(pszErrString);
            dos_dmem_free(pszErrString);
        }
        else
        {
            SQLGetDiagRec(SQL_HANDLE_DBC, pstHandle->hCon, 1, stat, &err, msg, sizeof(msg), &mlen);
            logr_error("Error SQLConnect=%d errno=%d [%s]\n", lResult, (int) err, msg);
        }

        /* Deallocate handles again, more chanses to succeed when reconnecting */
        dos_init_odbc_handles(pstHandle, DOS_TRUE); /* Reinit ODBC handles */
        return DOS_FAIL;
    }

    lResult = SQLGetInfo(pstHandle->hCon, SQL_DRIVER_NAME, (SQLCHAR *) pstHandle->szODBCDriver, 255, &valueLength);
    if (lResult == SQL_SUCCESS || lResult == SQL_SUCCESS_WITH_INFO)
    {
        for (i = 0; i < valueLength; ++i)
        {
            pstHandle->szODBCDriver[i] = (S8) toupper(pstHandle->szODBCDriver[i]);
        }
    }

    if (strstr(pstHandle->szODBCDriver, "SQORA32.DLL") != 0
        || strstr(pstHandle->szODBCDriver, "SQORA64.DLL") != 0)
    {
        pstHandle->blIsFirebird = DOS_FALSE;
        pstHandle->blIsOracle = DOS_TRUE;
    }
    else if (strstr(pstHandle->szODBCDriver, "FIREBIRD") != 0
        || strstr(pstHandle->szODBCDriver, "FB32") != 0
        || strstr(pstHandle->szODBCDriver, "FB64") != 0)
    {
        pstHandle->blIsFirebird = DOS_TRUE;
        pstHandle->blIsOracle = DOS_FALSE;
    }
    else
    {
        pstHandle->blIsFirebird = DOS_FALSE;
        pstHandle->blIsOracle = DOS_FALSE;
    }

    logr_debug("Connected to [%s]\n", pstHandle->pszDNS);
    pstHandle->enState = SC_ODBC_STATE_CONNECTED;
    return DOS_SUCC;
}

DLLEXPORT U32 dos_odbc_handle_exec_string(DOS_ODBC_HANDLE_ST *pstHandle, const S8 *sql, S8 *resbuf, S32 len, S8 **err)
{
    U32 sstatus = DOS_FAIL;
    SC_ODBC_STAT_HANDLE_ST stmt = NULL;
    SQLCHAR name[1024];
    SQLLEN m = 0;

    pstHandle->ulAffectedRows = 0;

    if (dos_odbc_handle_exec(pstHandle, sql, &stmt, err) == DOS_SUCC)
    {
        SQLSMALLINT NameLength, DataType, DecimalDigits, Nullable;
        SQLULEN ColumnSize;
        S32 lResult;

        SQLRowCount(stmt, &m);
        pstHandle->ulAffectedRows = (int) m;

        if (m == 0)
        {
            goto done;
        }

        lResult = SQLFetch(stmt);

        if (lResult != SQL_SUCCESS && lResult != SQL_SUCCESS_WITH_INFO && lResult != SQL_NO_DATA)
        {
            goto done;
        }

        SQLDescribeCol(stmt, 1, name, sizeof(name), &NameLength, &DataType, &ColumnSize, &DecimalDigits, &Nullable);
        SQLGetData(stmt, 1, SQL_C_CHAR, (SQLCHAR *) resbuf, (SQLLEN) len, NULL);

        sstatus = DOS_SUCC;
    }

done:
    dos_odbc_statement_handle_free(&stmt);

    return sstatus;
}

DLLEXPORT U32 dos_odbc_handle_exec(DOS_ODBC_HANDLE_ST *pstHandle
                            , const S8 *sql
                            , SC_ODBC_STAT_HANDLE_ST *rstmt
                            , S8 **ppszErrMsg)
{
    SQLHSTMT stmt = NULL;
    S32 lResult;
    S8 *pszErrStr = NULL, *pszXErr = NULL;
    SQLLEN m = 0;

    pstHandle->ulAffectedRows = 0;

    if (!dos_db_is_up(pstHandle))
    {
        goto error;
    }

    if (SQLAllocHandle(SQL_HANDLE_STMT, pstHandle->hCon, &stmt) != SQL_SUCCESS)
    {
        pszXErr = "SQLAllocHandle failed.";
        goto error;
    }

    if (SQLPrepare(stmt, (unsigned S8 *) sql, SQL_NTS) != SQL_SUCCESS)
    {
        pszXErr = "SQLPrepare failed.";
        goto error;
    }

    lResult = SQLExecute(stmt);

    switch (lResult)
    {
        case SQL_SUCCESS:
        case SQL_SUCCESS_WITH_INFO:
        case SQL_NO_DATA:
            break;
        case SQL_ERROR:
            pszXErr = "SQLExecute returned SQL_ERROR.";
            goto error;
            break;
        case SQL_NEED_DATA:
            pszXErr = "SQLExecute returned SQL_NEED_DATA.";
            goto error;
            break;
        default:
            pszXErr = "SQLExecute returned unknown result code.";
            goto error;
    }

    SQLRowCount(stmt, &m);
    pstHandle->ulAffectedRows = (int) m;

    if (rstmt)
    {
        *rstmt = stmt;
    }
    else
    {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }

    return DOS_SUCC;

  error:


    if (stmt)
    {
        pszErrStr = dos_odbc_handle_get_error(pstHandle, stmt);
    }

    if (!pszErrStr)
    {
        if (pszXErr)
        {
            pszErrStr = dos_strndup(pszXErr, dos_strlen(pszXErr) + 1);
        }
        else
        {
            pszErrStr = dos_strndup("SQL ERROR!", dos_strlen("SQL ERROR!") + 1);
        }
    }

    if (pszErrStr)
    {
        logr_error("ERR: [%s]\n[%s]\n", sql, pszErrStr);

        if (ppszErrMsg)
        {
            *ppszErrMsg = pszErrStr;
        }
        else
        {
            dos_dmem_free(pszErrStr);
        }
    }

    if (rstmt)
    {
        *rstmt = stmt;
    }
    else if (stmt)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }

    return DOS_FAIL;
}

DLLEXPORT U32 dos_odbc_handle_callback_exec(S8 *file
                                        , const S8 *func
                                        , S32 line
                                        , DOS_ODBC_HANDLE_ST *pstHandle
                                        , S8 *sql
                                        , sc_db_callback_func_t callback
                                        , VOID *pdata
                                        , S8 **ppszErrMsg)
{
    SQLHSTMT stmt = NULL;
    SQLSMALLINT c = 0, x = 0;
    SQLLEN m = 0;
    S8 *pszXErr = NULL, *pszErrStr = NULL;
    S32 lResult;
    S32 err_cnt = 0;
    S32 done = 0;

    pstHandle->ulAffectedRows = 0;

    if (!callback)
    {
        DOS_ASSERT(0);
    }

    if (!dos_db_is_up(pstHandle))
    {
        pszXErr = "DB is not up!";
        goto error;
    }

    if (SQLAllocHandle(SQL_HANDLE_STMT, pstHandle->hCon, &stmt) != SQL_SUCCESS)
    {
        pszXErr = "Unable to SQL allocate handle!";
        goto error;
    }

    if (SQLPrepare(stmt, (unsigned S8 *) sql, SQL_NTS) != SQL_SUCCESS)
    {
        pszXErr = "Unable to prepare SQL statement!";
        goto error;
    }

    lResult = SQLExecute(stmt);

    if (lResult != SQL_SUCCESS && lResult != SQL_SUCCESS_WITH_INFO && lResult != SQL_NO_DATA)
    {
        pszXErr = "execute error!";
        goto error;
    }

    SQLNumResultCols(stmt, &c);
    SQLRowCount(stmt, &m);
    pstHandle->ulAffectedRows = (int) m;


    while (!done)
    {
        int name_len = 256;
        S8 **names;
        S8 **vals;
        int y = 0;

        lResult = SQLFetch(stmt);

        if (lResult != SQL_SUCCESS)
        {
            if (lResult != SQL_NO_DATA)
            {
                err_cnt++;
            }
            break;
        }

        names = calloc(c, sizeof(*names));
        vals = calloc(c, sizeof(*vals));

        DOS_ASSERT(names && vals);

        for (x = 1; x <= c; x++)
        {
            SQLSMALLINT NameLength = 0, DataType = 0, DecimalDigits = 0, Nullable = 0;
            SQLULEN ColumnSize = 0;
            names[y] = malloc(name_len);
            memset(names[y], 0, name_len);

            SQLDescribeCol(stmt, x, (SQLCHAR *) names[y], (SQLSMALLINT) name_len, &NameLength, &DataType, &ColumnSize, &DecimalDigits, &Nullable);

            if (!ColumnSize)
            {
                ColumnSize = 255;
            }
            ColumnSize++;

            vals[y] = malloc(ColumnSize);
            memset(vals[y], 0, ColumnSize);
            SQLGetData(stmt, x, SQL_C_CHAR, (SQLCHAR *) vals[y], ColumnSize, NULL);
            y++;
        }

        if (callback(pdata, y, vals, names) != DOS_SUCC)
        {
            done = 1;
        }

        for (x = 0; x < y; x++)
        {
            free(names[x]);
            free(vals[x]);
        }
        free(names);
        free(vals);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    stmt = NULL; /* Make sure we don't try to free this handle again */

    if (!err_cnt)
    {
        return DOS_SUCC;
    }

  error:

    if (stmt)
    {
        pszErrStr = dos_odbc_handle_get_error(pstHandle, stmt);
    }

    if (!pszErrStr && pszXErr)
    {
        pszErrStr = dos_strndup(pszXErr, dos_strlen(pszXErr) + 1);
    }

    if (pszErrStr)
    {
        logr_error("ERR: [%s]\n[%s]\n", sql, pszErrStr ? pszErrStr : 0);
        if (ppszErrMsg)
        {
            *ppszErrMsg = pszErrStr;
        }
        else
        {
            dos_dmem_free(pszErrStr);
        }
    }

    if (stmt)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }

    return DOS_FAIL;
}

DLLEXPORT VOID dos_odbc_handle_destroy(DOS_ODBC_HANDLE_ST **ppstHandle)
{
    DOS_ODBC_HANDLE_ST *pstHandle = NULL;

    if (!ppstHandle)
    {
        return;
    }
    pstHandle = *ppstHandle;

    if (pstHandle)
    {
        dos_odbc_handle_disconnect(pstHandle);

        if (pstHandle->hEnv!= SQL_NULL_HANDLE)
        {
            SQLFreeHandle(SQL_HANDLE_DBC, pstHandle->hCon);
            SQLFreeHandle(SQL_HANDLE_ENV, pstHandle->hEnv);
        }

        if (pstHandle->pszDNS)
        {
            dos_dmem_free(pstHandle->pszDNS);
        }

        if (pstHandle->pszUsername)
        {
            dos_dmem_free(pstHandle->pszUsername);
        }

        if (pstHandle->pszPassword)
        {
            dos_dmem_free(pstHandle->pszPassword);
        }

        free(pstHandle);
    }

    *ppstHandle = NULL;
}

DLLEXPORT SC_ODBC_STATUS_EN dos_odbc_handle_get_state(DOS_ODBC_HANDLE_ST *pstHandle)
{
    return pstHandle ? pstHandle->enState : SC_ODBC_STATE_INIT;
}

DLLEXPORT S8* dos_odbc_handle_get_error(DOS_ODBC_HANDLE_ST *pstHandle, SC_ODBC_STAT_HANDLE_ST stmt)
{
    S8 szBuffer[SQL_MAX_MESSAGE_LENGTH + 1] = "";
    S8 szSqlstate[SQL_SQLSTATE_SIZE + 1] = "";
    SQLINTEGER sqlcode;
    SQLSMALLINT length;
    S8 *pszRet = NULL;

    pszRet = dos_dmem_alloc(SC_MAX_ERR_MSG_BUF);
    if (!pszRet)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    if (SQLError(pstHandle->hEnv, pstHandle->hCon, stmt, (SQLCHAR *) szSqlstate, &sqlcode, (SQLCHAR *) szBuffer, sizeof(szBuffer), &length) == SQL_SUCCESS)
    {
        dos_snprintf(pszRet, SC_MAX_ERR_MSG_BUF, "STATE: %s CODE %ld ERROR: %s\n", szSqlstate, sqlcode, szBuffer);
    }
    else
    {
        pszRet[0] = '\0';
    }

    return pszRet;
}

DLLEXPORT S32 dos_odbc_handle_affected_rows(DOS_ODBC_HANDLE_ST *pstHandle)
{
    return pstHandle->ulAffectedRows;
}

#if 0
SWITCH_DECLARE(switch_bool_t) sc_odbc_available(void)
{
#ifdef SWITCH_HAVE_ODBC
    return SWITCH_TRUE;
#else
    return SWITCH_FALSE;
#endif
}
#endif

DLLEXPORT U32 dos_odbc_SQLSetAutoCommitAttr(DOS_ODBC_HANDLE_ST *pstHandle, BOOL blSwitchON)
{
    if (blSwitchON)
    {
        return SQLSetConnectAttr(pstHandle->hCon, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER *) SQL_AUTOCOMMIT_ON, 0 );
    }
    else
    {
        return SQLSetConnectAttr(pstHandle->hCon, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER *) SQL_AUTOCOMMIT_OFF, 0 );
    }
}

DLLEXPORT U32 dos_odbc_SQLEndTran(DOS_ODBC_HANDLE_ST *pstHandle, BOOL blCommit)
{
    if (blCommit)
    {
        return SQLEndTran(SQL_HANDLE_DBC, pstHandle->hCon, SQL_COMMIT);
    }
    else
    {
        return SQLEndTran(SQL_HANDLE_DBC, pstHandle->hCon, SQL_ROLLBACK);
    }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SC_HTTPD_PUB_H__ */

