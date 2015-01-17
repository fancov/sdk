/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_db_conn.c
 *
 *  创建时间: 2014年12月17日20:52:58
 *  作    者: Larry
 *  描    述: 数据操作相关API
 *  修改历史:
 */

#ifndef __DOS_DB_PUB_H__
#define __DOS_DB_PUB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>

#undef VOID

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
#define SC_ODBC_LENGTH               256

#define SC_DEFAULT_ODBC_RETRIES      128

#define SC_MAX_ERR_MSG_BUF           1024

#define SC_ODBC_SQL_LENGTH           1024

typedef VOID *SC_ODBC_STAT_HANDLE_ST;
typedef S32 (*sc_db_callback_func_t)(VOID *pArg, S32 argc, S8 **argv, S8 **columnNames);
typedef S32 (*sc_db_err_callback_func_t)(void *pArg, const S8 *errmsg);

/* define enums */
typedef enum {
	SC_ODBC_STATE_INIT,
	SC_ODBC_STATE_DOWN,
	SC_ODBC_STATE_CONNECTED,
	SC_ODBC_STATE_ERROR
}SC_ODBC_STATUS_EN;

/* define structs */
typedef struct tagODBCHandle{
    S8                  *pszDNS;
	S8                  *pszUsername;
	S8                  *pszPassword;
	SQLHENV             hEnv;
	SQLHDBC             hCon;
	SC_ODBC_STATUS_EN   enState;
	S8                  szODBCDriver[SC_ODBC_LENGTH];
	BOOL                blIsFirebird;
	BOOL                blIsOracle;
	U32                 ulAffectedRows;
	U32                 ulNumRetries;
}DOS_ODBC_HANDLE_ST;




/* declare functions */

DLLEXPORT DOS_ODBC_HANDLE_ST *dos_odbc_handle_new(const S8 *pszDsn, const S8 *pszUsername, const S8 *pszPassword);
DLLEXPORT VOID dos_odbc_set_num_retries(DOS_ODBC_HANDLE_ST *pstHandle, S32 lNumRetries);
DLLEXPORT U32 dos_odbc_handle_disconnect(DOS_ODBC_HANDLE_ST *pstHandle);
DLLEXPORT U32 dos_odbc_statement_handle_free(SC_ODBC_STAT_HANDLE_ST *pStmt);
DLLEXPORT U32 dos_odbc_handle_connect(DOS_ODBC_HANDLE_ST *pstHandle);
DLLEXPORT U32 dos_odbc_handle_exec_string(DOS_ODBC_HANDLE_ST *pstHandle, const S8 *sql, S8 *resbuf, S32 len, S8 **err);
DLLEXPORT U32 dos_odbc_handle_exec(DOS_ODBC_HANDLE_ST *pstHandle
                            , const S8 *sql
                            , SC_ODBC_STAT_HANDLE_ST *pRStmt
                            , S8 **ppszErrMsg);
DLLEXPORT U32 dos_odbc_handle_callback_exec(S8 *file
                            , const S8 *func
                            , S32 line
                            , DOS_ODBC_HANDLE_ST *pstHandle
                            , S8 *sql
                            , sc_db_callback_func_t callback
                            , VOID *pdata
                            , S8 **ppszErrMsg);
DLLEXPORT VOID dos_odbc_handle_destroy(DOS_ODBC_HANDLE_ST **ppstHandle);
DLLEXPORT SC_ODBC_STATUS_EN dos_odbc_handle_get_state(DOS_ODBC_HANDLE_ST *pstHandle);
DLLEXPORT S8* dos_odbc_handle_get_error(DOS_ODBC_HANDLE_ST *pstHandle, SC_ODBC_STAT_HANDLE_ST pStmt);
DLLEXPORT S32 dos_odbc_handle_affected_rows(DOS_ODBC_HANDLE_ST *pstHandle);
DLLEXPORT U32 dos_odbc_SQLSetAutoCommitAttr(DOS_ODBC_HANDLE_ST *pstHandle, BOOL blSwitchON);
DLLEXPORT U32 dos_odbc_SQLEndTran(DOS_ODBC_HANDLE_ST *pstHandle, BOOL blCommit);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SC_HTTPD_PUB_H__ */


