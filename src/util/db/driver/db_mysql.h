
#ifndef __DB_MYSQL_H__
#define __DB_MYSQL_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#if DB_MYSQL

FUNCATTR S32 db_mysql_open(MYSQL *pstMysql, S8 *pszHost, U16 usPort, S8 *pszUsername, S8 *pszPassword, S8 *pszDBName, S8 **pszErrMsg);
FUNCATTR S32 db_mysql_close(MYSQL *pstMysql, S8 **pszErrMsg);
FUNCATTR S32 db_mysql_query(MYSQL *pstMysql, S8 *pszSQL, S32 (*callback)(VOID*, S32, S8**, S8**), VOID *pParamObj, S8 **pszErrMsg);
FUNCATTR S32 db_mysql_affect_row(MYSQL *pstMysql, S8 **pszErrMsg);
FUNCATTR S32 db_mysql_transaction_begin(MYSQL *pstMysql, S8 **pszErrMsg);
FUNCATTR S32 db_mysql_transaction_commit(MYSQL *pstMysql, S8 **pszErrMsg);
FUNCATTR S32 db_mysql_transaction_rollback(MYSQL *pstMysql, S8 **pszErrMsg);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*__DB_MYSQL_H__*/

