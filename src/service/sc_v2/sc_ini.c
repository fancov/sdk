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

U32          g_ulCPS                  = SC_MAX_CALL_PRE_SEC;
U32          g_ulMaxConcurrency4Task  = SC_MAX_CALL / 3;

/* define marcos */


/* define structs */

U32 sc_init_db();


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
U32 sc_bs_fsm_stop();

U32 sc_agent_mngt_init();

U32 sc_ext_mngt_init();
U32 sc_ext_mngt_start();
U32 sc_ext_mngt_stop();

U32 sc_cwq_init();
U32 sc_cwq_start();
U32 sc_cwq_stop();

U32 sc_log_digest_init();
U32 sc_log_digest_start();
U32 sc_log_digest_stop();

U32 sc_data_syn_init();
U32 sc_data_syn_start();
U32 sc_data_syn_stop();

U32 sc_httpd_init();
U32 sc_httpd_start();
U32 sc_httpd_stop();

U32 sc_pub_init();
U32 sc_pub_start();
U32 sc_pub_stop();

U32 sc_db_init();
U32 sc_db_start();
U32 sc_db_stop();

U32 sc_task_mngt_init();
U32 sc_task_mngt_start();
U32 sc_task_mngt_stop();


/* global variables */
SC_MOD_LIST_ST  astSCModList[] = {
    {SC_MOD_DB,         "SC_DB",     LOG_LEVEL_NOTIC, DOS_FALSE, sc_init_db,           NULL,                   NULL},
    {SC_MOD_DB_WQ,      "SC_DB_WQ",  LOG_LEVEL_NOTIC, DOS_FALSE, sc_db_init,           sc_db_start,            sc_db_stop},
    {SC_MOD_DIGIST,     "SC_DLOG",   LOG_LEVEL_NOTIC, DOS_FALSE, sc_log_digest_init,   sc_log_digest_start,    sc_log_digest_stop},
    {SC_MOD_RES,        "SC_RES",    LOG_LEVEL_NOTIC, DOS_FALSE, sc_res_init,          sc_res_start,           sc_res_stop},
    {SC_MOD_ACD,        "SC_AGENT",  LOG_LEVEL_NOTIC, DOS_FALSE, sc_agent_mngt_init,   NULL,                   NULL},
    {SC_MOD_EVENT,      "SC_EVT",    LOG_LEVEL_NOTIC, DOS_FALSE, sc_event_init,        sc_event_start,         sc_event_stop},
    {SC_MOD_SU,         "SC_ESL",    LOG_LEVEL_NOTIC, DOS_FALSE, sc_su_mngt_init,      sc_su_mngt_start,       sc_su_mngt_stop},
    {SC_MOD_BS,         "SC_BS",     LOG_LEVEL_NOTIC, DOS_FALSE, sc_bs_fsm_init,       sc_bs_fsm_start,        sc_su_mngt_stop},
    {SC_MOD_EXT_MNGT,   "SC_EXT",    LOG_LEVEL_NOTIC, DOS_FALSE, sc_ext_mngt_init,     sc_ext_mngt_start,      sc_ext_mngt_stop},
    {SC_MOD_CWQ,        "SC_CWQ",    LOG_LEVEL_NOTIC, DOS_FALSE, sc_cwq_init,          sc_cwq_start,           sc_cwq_stop},
    {SC_MOD_PUBLISH,    "SC_PUB",    LOG_LEVEL_NOTIC, DOS_FALSE, sc_pub_init,          sc_pub_start,           sc_pub_stop},
    {SC_MOD_ESL,        "SC_ESL_WR", LOG_LEVEL_NOTIC, DOS_FALSE, sc_esl_init,          sc_esl_start,           sc_esl_stop},
    {SC_MOD_HTTP_API,   "SC_HTTP",   LOG_LEVEL_NOTIC, DOS_FALSE, sc_httpd_init,        sc_httpd_start,         sc_httpd_stop},
    {SC_MOD_DATA_SYN,   "SC_SYN",    LOG_LEVEL_NOTIC, DOS_FALSE, sc_data_syn_init,     sc_data_syn_start,      sc_data_syn_stop},

    {SC_MOD_TASK,       "SC_TASK",   LOG_LEVEL_NOTIC, DOS_FALSE, sc_task_mngt_init,    sc_task_mngt_start,     sc_task_mngt_stop},
};


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
    U32   ulIndex = 0;
    U32   ulRet = DOS_SUCC;

    sc_log(LOG_LEVEL_NOTIC, "Start init SC. %u", time(NULL));

    for (ulIndex=0; ulIndex<sizeof(astSCModList)/sizeof(SC_MOD_LIST_ST); ulIndex++)
    {
        if (astSCModList[ulIndex].ulIndex >= SC_MOD_BUTT)
        {
            continue;
        }

        if (DOS_ADDR_INVALID(astSCModList[ulIndex].pszName))
        {
            continue;
        }

        sc_log(LOG_LEVEL_INFO, "Init %s mod.", astSCModList[ulIndex].pszName);

        if (DOS_ADDR_VALID(astSCModList[ulIndex].fn_init))
        {
            ulRet = astSCModList[ulIndex].fn_init();
        }

        sc_log(LOG_LEVEL_INFO, "Init %s mod %s.", astSCModList[ulIndex].pszName, DOS_SUCC == ulRet ? "succ" : "FAIL");

        if (ulRet != DOS_SUCC)
        {
            break;
        }
    }

    if (ulRet != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_NOTIC, "Init SC FAIL. %u", time(NULL));
        return DOS_SUCC;
    }

    sc_log(LOG_LEVEL_NOTIC, "Init SC finished. %u", time(NULL));

    g_blSCInitOK = DOS_TRUE;

    return DOS_SUCC;
}

U32 sc_start()
{
    U32   ulIndex = 0;
    U32   ulRet = DOS_SUCC;

    sc_log(LOG_LEVEL_NOTIC, "Start SC. %d", time(NULL));

    for (ulIndex=0; ulIndex<sizeof(astSCModList)/sizeof(SC_MOD_LIST_ST); ulIndex++)
    {
        if (astSCModList[ulIndex].ulIndex >= SC_MOD_BUTT)
        {
            continue;
        }

        if (DOS_ADDR_INVALID(astSCModList[ulIndex].pszName))
        {
            continue;
        }

        sc_log(LOG_LEVEL_INFO, "satrt %s mod.", astSCModList[ulIndex].pszName);

        if (DOS_ADDR_VALID(astSCModList[ulIndex].fn_start))
        {
            ulRet = astSCModList[ulIndex].fn_start();
        }

        sc_log(LOG_LEVEL_INFO, "start %s mod %s.", astSCModList[ulIndex].pszName, DOS_SUCC == ulRet ? "succ" : "FAIL");

        if (ulRet != DOS_SUCC)
        {
            break;
        }
    }

    if (ulRet != DOS_SUCC)
    {
        sc_log(LOG_LEVEL_NOTIC, "start SC FAIL. %u", time(NULL));
        return DOS_SUCC;
    }

    sc_log(LOG_LEVEL_NOTIC, "start SC finished. %u", time(NULL));

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



