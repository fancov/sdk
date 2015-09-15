/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_db.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 定义数据操作相关宏、数据结构，函数
 *     History:
 */

#ifndef __DOS_DB_H__
#define __DOS_DB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#if DB_MYSQL
#include <mysql/mysql.h>
#endif

#if DB_SQLITE
#include <sqlite3.h>
#endif

#define DB_DIGIST_TYPE_LENGTH 32

#define DB_MAX_STR_LEN        48

/* 重定义一些常用函数 */
#define db_mem_alloc   dos_dmem_alloc
#define db_mem_free    dos_dmem_free
#define db_memset      memset
#define db_assert      do{;}while
#define db_strlen      dos_strlen
#define db_strncpy     dos_strncpy
#define db_snprintf    dos_snprintf
#define db_log_debug   do{;}while
#define db_log_info    do{;}while
#define db_log_notice  do{;}while
#define db_log_warning do{;}while

/* 函数的修饰，默认为导出 */
#define FUNCATTR DLLEXPORT

/* 单次查询最大的字段数，多于的字段将会被忽略 */
#define MAX_DB_FIELDS 32

/* 错误码 */
typedef enum tagDBERRNo
{
    DB_ERR_SUCC = 0,
    DB_ERR_NOT_INIT,
    DB_ERR_NOT_CONNECTED,
    DB_ERR_INVALID_DB_TYPE,
    DB_ERR_INVALID_PARAMS,
    DB_ERR_QUERY_FAIL,
    DB_ERR_QUERY_MEM_LEAK,
    DB_ERR_QUERY_FETCH_FAIL,
    DB_ERR_UNKNOW,
}DB_ERR_NO_EN;

/* 数据库类型列表 */
typedef enum tagDBType
{
    DB_TYPE_MYSQL,
    DB_TYPE_SQLITE,

    DB_TYPE_BUTT
}DB_TYPE_EN;

/* 数据库状态 */
typedef enum {
    DB_STATE_INIT,
    DB_STATE_DOWN,
    DB_STATE_CONNECTED,
    DB_STATE_ERROR
}SC_ODBC_STATUS_EN;

/* 数据库连接handle */
typedef struct tagDBHandle
{
#if DB_MYSQL
    MYSQL   *pstMYSql;     /* MYSQL数据库句柄 */
    S8      szHost[DB_MAX_STR_LEN];
    S8      szUsername[DB_MAX_STR_LEN];
    S8      szPassword[DB_MAX_STR_LEN];
    S8      szDBName[DB_MAX_STR_LEN];
    S8      szSockPath[DB_MAX_STR_LEN];
    U16     usPort;
#endif
#if DB_SQLITE
    sqlite3 *pstSQLite;    /* SQLite数据库句柄 */
    S8      *pszFilepath;
#endif
    U32     ulDBType;      /* Refer to tagDBType */
    U32     ulDBStatus;    /* Refer to SC_ODBC_STATUS_EN */
    S8      *pzsErrMsg;    /* 错误信息BUFFER */
    pthread_mutex_t  mutexHandle;
}DB_HANDLE_ST;

/* 数据库API */

/**
 * 函数：db_create
 * 功能：根据参数创建数据库句柄
 * 参数：
 *      DB_TYPE_EN enType将要创建的数据库类型
 * 返回值：
 *      如果成功返回数据库句柄的指针，如果失败返回NULL
 */
FUNCATTR DB_HANDLE_ST *db_create(DB_TYPE_EN enType);

/**
 * 函数：db_destroy
 * 功能：销毁数据库句柄
 * 参数：
 *      DB_HANDLE_ST **ppstDBHandle，数据库句柄的指针
 * 返回值：
 *      成功返回0，失败返回-1
 */
FUNCATTR S32 db_destroy(DB_HANDLE_ST **pstDBHandle);

/**
 * 函数：db_open
 * 功能：打开数据库
 * 参数：
 *      DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 * 返回值：
 *      成功返回0，失败返回-1
 */
FUNCATTR S32 db_open(DB_HANDLE_ST *pstDBHandle);

/**
 * 函数：db_close
 * 功能：关闭数据库
 * 参数：DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 * 返回值：
 *      成功返回0，失败返回-1
 * 注意：
 *      该函数不会释放内存，需要调用destroy函数销毁句柄
 */
FUNCATTR S32 db_close(DB_HANDLE_ST *pstDBHandle);

/**
 * 函数：db_query
 * 功能：执行sql语句
 * 参数：
 *      DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 *      S8 *pszSQL, SQL语句
 *      S32 (*callback)(VOID*, S32, S8**, S8**), 行处理函数，没查询到一行会调用该函数一次
 *      VOID *pParamObj, 回调函数的额参数
 *      S8 **pszErrMsg，错误信息
 * 返回值：
 *      返回响应的错误码refer DB_ERR_NO_EN
 */
FUNCATTR S32 db_query(DB_HANDLE_ST *pstDBHandle, S8 *pszSQL, S32 (*callback)(VOID*, S32, S8**, S8**), VOID *pParamObj, S8 **pszErrMsg);

/**
 * 函数：db_affect_raw
 * 功能：获取查询结果集中的函数
 * 参数：
 *      DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 * 返回值：
 *      成功返回函数，失败返回-1
 */
FUNCATTR S32 db_affect_raw(DB_HANDLE_ST *pstDBHandle);

/**
 * 函数：db_transaction_begin
 * 功能：开始执行事务
 * 参数：
 *      DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 * 返回值：成功返回函数，失败返回-1
 */
FUNCATTR S32 db_transaction_begin(DB_HANDLE_ST *pstDBHandle);

/**
 * 函数：db_transaction_commit
 * 功能：提交事务
 * 参数：
 *      DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 * 返回值：
 *      成功返回函数，失败返回-1
 */
FUNCATTR S32 db_transaction_commit(DB_HANDLE_ST *pstDBHandle);

/**
 * 函数：db_transaction_rollback
 * 功能：回滚事务
 * 参数：
 *      DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 * 返回值：
 *      成功返回函数，失败返回-1
 */
FUNCATTR S32 db_transaction_rollback(DB_HANDLE_ST *pstDBHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DOS_DB_H__ */


