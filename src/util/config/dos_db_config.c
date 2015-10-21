#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if INCLUDE_DB_CONFIG
#include <dos/dos_db_config.h>

#define MAX_PARAM_LENGTH  512

static DB_HANDLE_ST * g_pstGllobalConfigDBHandle = NULL;

static S32 dos_db_config_get_param_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    IO_BUF_CB *pstIOBuffer = NULL;

    if (DOS_ADDR_INVALID(pArg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (lCount != 1 || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (aszValues[0][0] == '\0')
    {
        if (pstIOBuffer->ulSize > 0)
        {
            pstIOBuffer->pszBuffer[0] = '\0';
        }
    }
    else
    {
        pstIOBuffer = (IO_BUF_CB *)pArg;
        dos_iobuf_append(pstIOBuffer, aszValues[0], dos_strnlen(aszValues[0], MAX_PARAM_LENGTH));
        pstIOBuffer->pszBuffer[pstIOBuffer->ulLength] = '\0';
    }

    return DOS_SUCC;
}

U32 dos_db_config_get_param(U32 ulIndex, S8 *pszBuffer, U32 ulBufferLen)
{
    S8 szSQL[1024];
    IO_BUF_CB stBuffer = IO_BUF_INIT;

    if (DOS_ADDR_INVALID(g_pstGllobalConfigDBHandle))
    {
        dos_printf("Database Handle for the db config not init.");
        return DOS_FAIL;
    }

    if (ulIndex >= PARAM_BUTT)
    {
        DOS_ASSERT(0);

        dos_printf("Invalid parameter index.");
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pszBuffer) || 0 == ulBufferLen)
    {
        DOS_ASSERT(0);

        dos_printf("Empty buffer.");
        return DOS_FAIL;
    }

    /* 最大长度为512 */
    dos_iobuf_resize(&stBuffer, MAX_PARAM_LENGTH);
    dos_snprintf(szSQL, sizeof(szSQL), "SELECT parameter_value FROM ccsys.tbl_parameters WHERE id=%u", ulIndex);
    if (db_query(g_pstGllobalConfigDBHandle, szSQL, dos_db_config_get_param_cb, &stBuffer, NULL) != DB_ERR_SUCC)
    {
        dos_iobuf_free(&stBuffer);

        dos_printf("Query database fail.");
        return DOS_FAIL;
    }

    if (stBuffer.ulLength >= ulBufferLen)
    {
        dos_iobuf_free(&stBuffer);

        dos_printf("Buffer not enought.");
        return DOS_FAIL;
    }

    dos_strncpy(pszBuffer, (S8 *)stBuffer.pszBuffer, ulBufferLen);
    pszBuffer[ulBufferLen - 1] = '\0';

    dos_iobuf_free(&stBuffer);

    return DOS_SUCC;
}


U32 dos_db_config_init()
{
    U16 usDBPort;
    S8  szDBHost[DB_MAX_STR_LEN] = {0, };
    S8  szDBUsername[DB_MAX_STR_LEN] = {0, };
    S8  szDBPassword[DB_MAX_STR_LEN] = {0, };
    S8  szDBName[DB_MAX_STR_LEN] = {0, };
    S8  szDBSockPath[DB_MAX_STR_LEN] = {0, };

    if (config_get_db_host(szDBHost, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    if (config_get_db_user(szDBUsername, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    if (config_get_db_password(szDBPassword, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    usDBPort = config_get_db_port();
    if (0 == usDBPort || U16_BUTT == usDBPort)
    {
        usDBPort = 3306;
    }

    if (config_get_db_dbname(szDBName, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    if (config_get_mysqlsock_path(szDBSockPath, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }


    g_pstGllobalConfigDBHandle = db_create(DB_TYPE_MYSQL);
    if (!g_pstGllobalConfigDBHandle)
    {
        DOS_ASSERT(0);
        return -1;
    }

    dos_strncpy(g_pstGllobalConfigDBHandle->szHost, szDBHost, sizeof(g_pstGllobalConfigDBHandle->szHost));
    g_pstGllobalConfigDBHandle->szHost[sizeof(g_pstGllobalConfigDBHandle->szHost) - 1] = '\0';

    dos_strncpy(g_pstGllobalConfigDBHandle->szUsername, szDBUsername, sizeof(g_pstGllobalConfigDBHandle->szUsername));
    g_pstGllobalConfigDBHandle->szUsername[sizeof(g_pstGllobalConfigDBHandle->szUsername) - 1] = '\0';

    dos_strncpy(g_pstGllobalConfigDBHandle->szPassword, szDBPassword, sizeof(g_pstGllobalConfigDBHandle->szPassword));
    g_pstGllobalConfigDBHandle->szPassword[sizeof(g_pstGllobalConfigDBHandle->szPassword) - 1] = '\0';

    dos_strncpy(g_pstGllobalConfigDBHandle->szDBName, szDBName, sizeof(g_pstGllobalConfigDBHandle->szDBName));
    g_pstGllobalConfigDBHandle->szDBName[sizeof(g_pstGllobalConfigDBHandle->szDBName) - 1] = '\0';

    dos_strncpy(g_pstGllobalConfigDBHandle->szSockPath, szDBSockPath, sizeof(g_pstGllobalConfigDBHandle->szSockPath));
    g_pstGllobalConfigDBHandle->szSockPath[sizeof(g_pstGllobalConfigDBHandle->szSockPath) - 1] = '\0';

    g_pstGllobalConfigDBHandle->usPort = usDBPort;

    if (db_open(g_pstGllobalConfigDBHandle) < 0)
    {
        DOS_ASSERT(0);

        db_destroy(&g_pstGllobalConfigDBHandle);
        g_pstGllobalConfigDBHandle = NULL;
        return -1;
    }

    return DOS_SUCC;

errno_proc:

    return DOS_FAIL;

}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


