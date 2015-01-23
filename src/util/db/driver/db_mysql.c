

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
#include <dos.h>
#include "db_mysql.h"

#if DB_MYSQL

FUNCATTR S32 db_mysql_open(MYSQL *pstMysql, S8 *pszHost, U16 usPort, S8 *pszUsername, S8 *pszPassword, S8 **pszErrMsg)
{
    S8 value = 1;

    if (!pstMysql || !pszHost)
    {
        db_assert(0);

        return -1;
    }

    if (!mysql_init(pstMysql))
    {
        db_assert(0);

        return -1;
    }

    if (!mysql_real_connect(pstMysql, pszHost, pszUsername, pszPassword, NULL, usPort,  NULL, 0))
    {
        db_assert(0);

        return -1;
    }

    mysql_options(pstMysql, MYSQL_OPT_RECONNECT, &value);

    return 0;
}

FUNCATTR S32 db_mysql_close(MYSQL *pstMysql, S8 **pszErrMsg)
{
    if (!pstMysql)
    {
        db_assert(0);

        return -1;
    }

    mysql_close(pstMysql);

    return 0;
}

FUNCATTR S32 db_mysql_query(MYSQL *pstMysql, S8 *pszSQL, S32 (*callback)(VOID*, S32, S8**, S8**), VOID *pParamObj, S8 **pszErrMsg)
{
    MYSQL_RES   *pstResource;
    MYSQL_FIELD *pstField;
    S8          *pszFields[MAX_DB_FIELDS] = { NULL };
    S8          *pszDatas[MAX_DB_FIELDS] = { NULL };
    MYSQL_ROW   stRow;
    S32         i, ulNameLen, ulValLen, ulFieldCnt, ulResult;

    if (!pstMysql || !pszSQL)
    {
        db_assert(0);

        return DB_ERR_INVALID_PARAMS;
    }

    if (mysql_query(pstMysql, pszSQL))
    {
        db_assert(0);

        return DB_ERR_QUERY_FAIL;
    }

    pstResource = mysql_store_result(pstMysql);
    if (!pstResource)
    {
        ulFieldCnt = mysql_field_count(pstMysql);
        /* 如果是0，说明是非select查询 */
        if (0 == ulFieldCnt)
        {
            //return mysql_affected_rows(pstMysql);
            return DB_ERR_SUCC;
        }
        else
        {
            db_assert(0);

            return DB_ERR_QUERY_MEM_LEAK;
        }
    }

    i = 0;
    ulFieldCnt = mysql_field_count(pstMysql);
    while ((pstField = mysql_fetch_field(pstResource)) && i<MAX_DB_FIELDS)
    {
        if (pstField->name)
        {
            continue;
        }

        if (pstField->name)
        {
            ulNameLen = db_strlen(pstField->name) + 1;
            pszFields[i] = db_mem_alloc(ulNameLen);
            db_strncpy(pszFields[i], pstField->name, ulNameLen);
            pszFields[i][ulNameLen - 1] = '\0';
        }
        else
        {
            pszFields[i] = NULL;
        }

        i++;
    }

    while ((stRow = mysql_fetch_row(pstResource)))
    {
        ulResult   = 0;
        for (i=0; i<MAX_DB_FIELDS && i<ulFieldCnt; i++)
        {
            if (stRow[i] && stRow[i][0] != '\0')
            {
                ulValLen = db_strlen(stRow[i]) + 1;
                pszDatas[i] = db_mem_alloc(ulValLen);
                if (!pszDatas[i])
                {
                    ulResult = (U32)-1;
                    break;
                }

                db_snprintf(pszDatas[i], ulValLen, "%s", stRow[i]);
            }
            else
            {
                pszDatas[i] = NULL;
            }
        }

        if (!ulResult && callback)
        {
            callback(pParamObj, (S32)ulFieldCnt, pszFields, pszDatas);
        }

        for (i=0; i<MAX_DB_FIELDS; i++)
        {
            if (pszFields[i])
            {
                db_mem_free(pszFields[i]);
                pszFields[i] = NULL;
            }

            if (pszDatas[i])
            {
                db_mem_free(pszDatas[i]);
                pszDatas[i] = NULL;
            }
        }

        if (ulResult)
        {
            mysql_free_result(pstResource);
            return DB_ERR_QUERY_FETCH_FAIL;
        }
    }

    mysql_free_result(pstResource);
    return DB_ERR_SUCC;
}

FUNCATTR S32 db_mysql_affect_row(MYSQL *pstMysql, S8 **pszErrMsg)
{
    if (!pstMysql)
    {
        db_assert(0);

        return -1;
    }

    return mysql_affected_rows(pstMysql);
}

FUNCATTR S32 db_mysql_transaction_begin(MYSQL *pstMysql, S8 **pszErrMsg)
{
    if (!pstMysql)
    {
        db_assert(0);

        return -1;
    }

    return db_mysql_query(pstMysql, "BEGIN ", NULL, NULL, pszErrMsg);
}

FUNCATTR S32 db_mysql_transaction_commit(MYSQL *pstMysql, S8 **pszErrMsg)
{
    if (!pstMysql)
    {
        db_assert(0);

        return -1;
    }

    return db_mysql_query(pstMysql, "COMMIT ", NULL, NULL, pszErrMsg);
}

FUNCATTR S32 db_mysql_transaction_rollback(MYSQL *pstMysql, S8 **pszErrMsg)
{
    if (!pstMysql)
    {
        db_assert(0);

        return -1;
    }

    return db_mysql_query(pstMysql, "ROLLBACK ", NULL, NULL, pszErrMsg);
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

