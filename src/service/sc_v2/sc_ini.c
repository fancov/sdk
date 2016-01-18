/**
 * @file : sc_ini.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 业务控制模块启动
 *
 * @date: 2016年1月15日
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
/* include private header files */
#include "sc_def.h"
#include "sc_debug.h"


/* 数据库句柄 */
DB_HANDLE_ST         *g_pstSCDBHandle = NULL;

/* 是否已经初始化OK */
BOOL                 g_blSCInitOK     = DOS_FALSE;


/* define marcos */

/* define enums */

/* define structs */

/* global variables */


/* declare functions */
U32 sc_res_init();
U32 sc_res_start();
U32 sc_res_stop();
U32 sc_esl_init();
U32 sc_esl_start();
U32 sc_esl_stop();
U32 sc_event_init();
U32 sc_event_start();
U32 sc_event_stop();
U32 sc_su_mngt_init();
U32 sc_su_mngt_start();
U32 sc_su_mngt_stop();
U32 sc_bs_fsm_init();
U32 sc_bs_fsm_start();
U32 sc_acd_init();

U32 sc_init_db()
{
    U16             usDBPort;
    S8              szDBHost[DB_MAX_STR_LEN] = {0, };
    S8              szDBUsername[DB_MAX_STR_LEN] = {0, };
    S8              szDBPassword[DB_MAX_STR_LEN] = {0, };
    S8              szDBName[DB_MAX_STR_LEN] = {0, };
    S8              szDBSockPath[DB_MAX_STR_LEN] = {0, };

    if (config_get_db_host(szDBHost, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (config_get_db_user(szDBUsername, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (config_get_db_password(szDBPassword, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    usDBPort = config_get_db_port();
    if (0 == usDBPort || U16_BUTT == usDBPort)
    {
        usDBPort = 3306;
    }

    if (config_get_db_dbname(szDBName, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (config_get_mysqlsock_path(szDBSockPath, DB_MAX_STR_LEN) <0)
    {
        DOS_ASSERT(0);
        szDBSockPath[0] = '\0';
    }

    g_pstSCDBHandle = db_create(DB_TYPE_MYSQL);
    if (!g_pstSCDBHandle)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_strncpy(g_pstSCDBHandle->szHost, szDBHost, sizeof(g_pstSCDBHandle->szHost));
    g_pstSCDBHandle->szHost[sizeof(g_pstSCDBHandle->szHost) - 1] = '\0';

    dos_strncpy(g_pstSCDBHandle->szUsername, szDBUsername, sizeof(g_pstSCDBHandle->szUsername));
    g_pstSCDBHandle->szUsername[sizeof(g_pstSCDBHandle->szUsername) - 1] = '\0';

    dos_strncpy(g_pstSCDBHandle->szPassword, szDBPassword, sizeof(g_pstSCDBHandle->szPassword));
    g_pstSCDBHandle->szPassword[sizeof(g_pstSCDBHandle->szPassword) - 1] = '\0';

    dos_strncpy(g_pstSCDBHandle->szDBName, szDBName, sizeof(g_pstSCDBHandle->szDBName));
    g_pstSCDBHandle->szDBName[sizeof(g_pstSCDBHandle->szDBName) - 1] = '\0';

    if ('\0' != szDBSockPath[0])
    {
        dos_strncpy(g_pstSCDBHandle->szSockPath, szDBSockPath, sizeof(g_pstSCDBHandle->szSockPath));
        g_pstSCDBHandle->szSockPath[sizeof(g_pstSCDBHandle->szSockPath) - 1] = '\0';
    }
    else
    {
        g_pstSCDBHandle->szSockPath[0] = '\0';
    }


    g_pstSCDBHandle->usPort = usDBPort;

    /* 连接数据库 */
    if (db_open(g_pstSCDBHandle) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        db_destroy(&g_pstSCDBHandle);

        return DOS_FAIL;
    }

    return DOS_SUCC;
}


U32 sc_init()
{
    sc_log(LOG_LEVEL_INFO, "%s", "Start init SC.");

    if (sc_init_db() != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_log(LOG_LEVEL_ERROR, "%s", "Init DB Handle FAIL.");
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_INFO, "%s", "Init DB Handle Successfully.");

    if (DOS_SUCC != sc_bs_fsm_init())
    {
        DOS_ASSERT(0);

        sc_log(LOG_LEVEL_ERROR, "%s", "Init bs client FAIL.");
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_INFO, "%s", "Init bs client Successfully.");


    if (sc_acd_init() != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "Init ACD FAIL.");
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_INFO, "%s", "Init ACD succ.");

    if (sc_res_init() != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "Init resource FAIL.");
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_INFO, "%s", "Init resource succ.");

    if (sc_event_init() != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "Init sc core fail.");
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_INFO, "%s", "Init sc core succ.");

    if (sc_su_mngt_init() != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "Init service unit FAIL.");
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_INFO, "%s", "Init service unit succ.");


    if (sc_esl_init() != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "Init esl client FAIL.");
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_INFO, "%s", "Init esl client suc.");


    sc_log(LOG_LEVEL_INFO, "%s", "Start init SC finished.");

    g_blSCInitOK = DOS_TRUE;

    return DOS_SUCC;
}

U32 sc_start()
{
    sc_log(LOG_LEVEL_INFO, "%s", "Start SC.");



    if (DOS_SUCC != sc_bs_fsm_start())
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "Start bs client FAIL");
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_INFO, "%s", "Start bs client Successfully.");

    if (sc_res_start() != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "Start resource management FAIL.");
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_INFO, "%s", "Start resource management succ.");

    if (sc_event_start() != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "start sc core fail.");
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_INFO, "%s", "start sc core succ.");

    if (sc_su_mngt_start() != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "start service unit FAIL.");
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_INFO, "%s", "start service unit succ.");


    if (sc_esl_start() != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_ERROR, "%s", "start esl client FAIL.");
        return DOS_FAIL;
    }
    sc_log(LOG_LEVEL_INFO, "%s", "start esl client suc.");


    sc_log(LOG_LEVEL_INFO, "%s", "Start SC finished.");

    g_blSCInitOK = DOS_TRUE;

    return DOS_SUCC;

}

U32 mod_dipcc_sc_shutdown()
{
    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */



