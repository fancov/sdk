/**
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_db.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现数据库操作相关函数
 *     History:
 */

#include <dos.h>

#if DB_MYSQL
#include <mysql/mysql.h>
#endif

#if DB_SQLITE
#include <sqlite3.h>
#endif

#include "driver/db_mysql.h"
#include "driver/db_sqlite.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/**
 * 函数：db_create
 * 功能：根据参数创建数据库句柄
 * 参数：DB_TYPE_EN enType将要创建的数据库类型
 * 返回值：如果成功返回数据库句柄的指针，如果失败返回NULL
 */
FUNCATTR DB_HANDLE_ST *db_create(DB_TYPE_EN enType)
{
    DB_HANDLE_ST *pstHandle = NULL;
    U32          ulResult   = DOS_SUCC;

    if (enType >= DB_TYPE_BUTT)
    {
        db_assert(0);

        return NULL;
    }

    pstHandle = (DB_HANDLE_ST *)db_mem_alloc(sizeof(DB_HANDLE_ST));
    if (NULL == pstHandle)
    {
        db_assert(0);

        return NULL;
    }
    db_memset((VOID *)pstHandle, 0, sizeof(DB_HANDLE_ST));
    pstHandle->ulDBType = enType;
    pstHandle->ulDBStatus = DB_STATE_INIT;

    switch (enType)
    {
#if DB_MYSQL
        case DB_TYPE_MYSQL:
            pstHandle->pstMYSql = (MYSQL *)db_mem_alloc(sizeof(MYSQL));
            if (!pstHandle->pstMYSql)
            {
                ulResult = DOS_FAIL;
            }
            else
            {
                db_memset(pstHandle->pstMYSql, 0, sizeof(MYSQL));
            }

            break;
#endif
#if DB_SQLITE
        case DB_TYPE_SQLITE:
            pstHandle->pstSQLite = NULL;
            pstHandle->pszFilepath = NULL;
            break;
#endif
        default:
            ulResult = DOS_FAIL;
            break;
    }

    if (ulResult != DOS_SUCC)
    {
        db_mem_free(pstHandle);

        return NULL;
    }

    return pstHandle;
}

/**
 * 函数：db_destroy
 * 功能：销毁数据库句柄
 * 参数：DB_HANDLE_ST **ppstDBHandle，数据库句柄的指针
 * 返回值：成功返回0，失败返回-1
 */
FUNCATTR S32 db_destroy(DB_HANDLE_ST **ppstDBHandle)
{
    DB_HANDLE_ST *pstDBHandle = NULL;

    if (NULL == ppstDBHandle
        || NULL == *ppstDBHandle)
    {
        db_assert(0);

        return -1;
    }

    pstDBHandle = *ppstDBHandle;
    switch (pstDBHandle->ulDBType)
    {
#if DB_MYSQL
        case DB_TYPE_MYSQL:
            if (pstDBHandle->pstMYSql)
            {
                db_mysql_close(pstDBHandle->pstMYSql, NULL);

                db_mem_free(pstDBHandle->pstMYSql);
                pstDBHandle->pstMYSql = NULL;
            }
            pstDBHandle->pstMYSql = NULL;

            break;
#endif

#if DB_SQLITE
        case DB_TYPE_SQLITE:
            if (pstDBHandle->pstSQLite)
            {
                db_sqlite3_close(pstDBHandle->pstSQLite, NULL);
                pstDBHandle->pstSQLite = NULL;
            }
            if (pstDBHandle->pszFilepath)
            {
                db_mem_free(pstDBHandle->pszFilepath);
                pstDBHandle->pszFilepath = NULL;
            }
            break;
#endif
        default:
            db_assert(0);
            return -1;
    }

    db_mem_free(pstDBHandle);
    *ppstDBHandle = NULL;

    return 0;
}

/**
 * 函数：db_open
 * 功能：打开数据库
 * 参数：DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 * 返回值：成功返回0，失败返回-1
 */
FUNCATTR S32 db_open(DB_HANDLE_ST *pstDBHandle)
{
    S32 lRet = 0;

    if (!pstDBHandle)
    {
        db_assert(0);

        return -1;
    }

    if (pstDBHandle->ulDBType >= DB_TYPE_BUTT)
    {
        db_assert(0);

        return -1;
    }

    switch (pstDBHandle->ulDBType)
    {
#if DB_MYSQL
        case DB_TYPE_MYSQL:

            lRet = db_mysql_open(pstDBHandle->pstMYSql
                            , pstDBHandle->szHost
                            , pstDBHandle->usPort
                            , pstDBHandle->szUsername
                            , pstDBHandle->szPassword
                            , pstDBHandle->szDBName
                            , pstDBHandle->szSockPath
                            , NULL);
            if (lRet < 0)
            {
                db_assert(0);

                return -1;
            }

            pstDBHandle->ulDBStatus = DB_STATE_CONNECTED;
            break;
#endif
#if DB_SQLITE
        case DB_TYPE_SQLITE:
            if (!pstDBHandle->pszFilepath)
            {
                db_assert(0);

                return -1;
            }

            lRet = db_sqlite3_open(&pstDBHandle->pstSQLite, pstDBHandle->pszFilepath, NULL);
            if (lRet < 0)
            {
                db_assert(0);

                return -1;
            }
            break;
#endif
        default:
            db_assert(0);
            return -1;
    }

    return 0;
}


/**
 * 函数：db_close
 * 功能：关闭数据库
 * 参数：DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 * 返回值：成功返回0，失败返回-1
 * 注意：该函数不会释放内存，需要调用destroy函数销毁句柄
 */
FUNCATTR S32 db_close(DB_HANDLE_ST *pstDBHandle)
{
    if (!pstDBHandle)
    {
        db_assert(0);

        return -1;
    }

    if (pstDBHandle->ulDBType >= DB_TYPE_BUTT)
    {
        db_assert(0);

        return -1;
    }

    switch (pstDBHandle->ulDBType)
    {
#if DB_MYSQL
        case DB_TYPE_MYSQL:
            /* @TODO: Read configure */
            if (db_mysql_close(pstDBHandle->pstMYSql, NULL) < 0)
            {
                db_assert(0);
                return -1;
            }

            pstDBHandle->ulDBStatus = DB_STATE_DOWN;
            break;
#endif
#if DB_SQLITE
        case DB_TYPE_SQLITE:
            if (pstDBHandle->pstSQLite)
            {
                db_sqlite3_close(pstDBHandle->pstSQLite, NULL);
                pstDBHandle->pstSQLite = NULL;
            }
            break;
#endif
        default:
            db_assert(0);
            return -1;
    }

    return 0;
}

/**
 * 函数：db_query
 * 功能：执行sql语句
 * 参数：
 *		DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 *		S8 *pszSQL, SQL语句
 *		S32 (*callback)(VOID*, S32, S8**, S8**), 行处理函数，没查询到一行会调用该函数一次
 *		VOID *pParamObj, 回调函数的额参数
 *		S8 **pszErrMsg，错误信息
 * 返回值：返回响应的错误码refer DB_ERR_NO_EN
 */
FUNCATTR S32 db_query(DB_HANDLE_ST *pstDBHandle, S8 *pszSQL, S32 (*callback)(VOID*, S32, S8**, S8**), VOID *pParamObj, S8 **pszErrMsg)
{
    U32 ulRet = 0;
    if (!pstDBHandle)
    {
        db_assert(0);
        return DB_ERR_INVALID_PARAMS;
    }

    if (!pszSQL)
    {
        db_assert(0);

        return DB_ERR_INVALID_PARAMS;
    }

    if (pstDBHandle->ulDBStatus != DB_STATE_CONNECTED)
    {
        db_assert(0);

        return DB_ERR_NOT_CONNECTED;
    }

    if (pstDBHandle->ulDBType >= DB_TYPE_BUTT)
    {
        db_assert(0);

        return DB_ERR_NOT_INIT;
    }

    switch (pstDBHandle->ulDBType)
    {
#if DB_MYSQL
        case DB_TYPE_MYSQL:
            /* @TODO: Read configure */
            ulRet = db_mysql_query(pstDBHandle->pstMYSql, pszSQL, callback, pParamObj, pszErrMsg);
            if (ulRet != DB_ERR_SUCC)
            {
                db_assert(0);
                return ulRet;
            }
            break;
#endif
#if DB_SQLITE
        case DB_TYPE_SQLITE:
            ulRet = db_sqlite3_query(pstDBHandle->pstSQLite, pszSQL, callback, pParamObj, pszErrMsg);
            if (ulRet != DB_ERR_SUCC)
            {
                db_assert(0);
                return ulRet;
            }
            break;
#endif
        default:
            db_assert(0);
            return DB_ERR_INVALID_DB_TYPE;
    }

    return DB_ERR_SUCC;
}

/**
 * 函数：db_affect_raw
 * 功能：获取查询结果集中的函数
 * 参数：
 *		DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 * 返回值：成功返回函数，失败返回-1
 */
FUNCATTR S32 db_affect_raw(DB_HANDLE_ST *pstDBHandle)
{
    if (!pstDBHandle)
    {
        db_assert(0);

        return -1;
    }

    if (pstDBHandle->ulDBStatus != DB_STATE_CONNECTED)
    {
        db_assert(0);

        return -1;
    }

    switch (pstDBHandle->ulDBType)
    {
#if DB_MYSQL
        case DB_TYPE_MYSQL:
            /* @TODO: Read configure */
            if (db_mysql_affect_row(pstDBHandle->pstMYSql, NULL) < 0)
            {
                db_assert(0);
                return -1;
            }
            break;
#endif
#if DB_SQLITE
        case DB_TYPE_SQLITE:
            if (db_sqlite3_affect_row(pstDBHandle->pstSQLite, NULL) < 0)
            {
                db_assert(0);
                return -1;
            }
            break;
#endif
        default:
            db_assert(0);
            return -1;
    }

    db_assert(0);
    return -1;
}

/**
 * 函数：db_transaction_begin
 * 功能：开始执行事务
 * 参数：
 *		DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 * 返回值：成功返回函数，失败返回-1
 */
FUNCATTR S32 db_transaction_begin(DB_HANDLE_ST *pstDBHandle)
{
    if (!pstDBHandle)
    {
        db_assert(0);

        return -1;
    }

    if (pstDBHandle->ulDBType >= DB_TYPE_BUTT)
    {
        db_assert(0);

        return -1;
    }

    if (pstDBHandle->ulDBStatus != DB_STATE_CONNECTED)
    {
        db_assert(0);

        return -1;
    }

    switch (pstDBHandle->ulDBType)
    {
#if DB_MYSQL
        case DB_TYPE_MYSQL:
            /* @TODO: Read configure */
            if (db_mysql_transaction_begin(pstDBHandle->pstMYSql, NULL) < 0)
            {
                db_assert(0);
                return -1;
            }
            break;
#endif
#if DB_SQLITE
        case DB_TYPE_SQLITE:
            if (db_sqlite3_transaction_begin(pstDBHandle->pstSQLite, NULL) < 0)
            {
                db_assert(0);
                return -1;
            }
            break;
#endif
        default:
            db_assert(0);
            return -1;
    }

    return 0;
}

/**
 * 函数：db_transaction_commit
 * 功能：提交事务
 * 参数：
 *		DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 * 返回值：成功返回函数，失败返回-1
 */
FUNCATTR S32 db_transaction_commit(DB_HANDLE_ST *pstDBHandle)
{
    if (!pstDBHandle)
    {
        db_assert(0);

        return -1;
    }

    if (pstDBHandle->ulDBType >= DB_TYPE_BUTT)
    {
        db_assert(0);

        return -1;
    }

    if (pstDBHandle->ulDBStatus != DB_STATE_CONNECTED)
    {
        db_assert(0);

        return -1;
    }


    switch (pstDBHandle->ulDBType)
    {
#if DB_MYSQL
        case DB_TYPE_MYSQL:
            /* @TODO: Read configure */
            if (db_mysql_transaction_commit(pstDBHandle->pstMYSql, NULL) < 0)
            {
                db_assert(0);
                return -1;
            }
            break;
#endif
#if DB_SQLITE
        case DB_TYPE_SQLITE:
            if (db_sqlite3_transaction_commit(pstDBHandle->pstSQLite, NULL) < 0)
            {
                db_assert(0);
                return -1;
            }
            break;
#endif

        default:
            db_assert(0);
            return -1;
    }

    return 0;
}


/**
 * 函数：db_transaction_rollback
 * 功能：回滚事务
 * 参数：
 *		DB_HANDLE_ST *pstDBHandle，数据库句柄的指针
 * 返回值：成功返回函数，失败返回-1
 */
FUNCATTR S32 db_transaction_rollback(DB_HANDLE_ST *pstDBHandle)
{
    if (!pstDBHandle)
    {
        db_assert(0);

        return -1;
    }

    if (pstDBHandle->ulDBType >= DB_TYPE_BUTT)
    {
        db_assert(0);

        return -1;
    }

    if (pstDBHandle->ulDBStatus != DB_STATE_CONNECTED)
    {
        db_assert(0);

        return -1;
    }

    switch (pstDBHandle->ulDBType)
    {
#if DB_MYSQL
        case DB_TYPE_MYSQL:
            /* @TODO: Read configure */
            if (db_mysql_transaction_rollback(pstDBHandle->pstMYSql, NULL) < 0)
            {
                db_assert(0);
                return -1;
            }
            break;
#endif
#if DB_SQLITE
        case DB_TYPE_SQLITE:
            if (db_sqlite3_transaction_rollback(pstDBHandle->pstSQLite, NULL) < 0)
            {
                db_assert(0);
                return -1;
            }
            break;
#endif

        default:
            db_assert(0);
            return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


