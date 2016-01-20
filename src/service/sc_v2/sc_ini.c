/**
 * @file : sc_ini.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * ҵ�����ģ������
 *
 * @date: 2016��1��15��
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


/* ���ݿ��� */
DB_HANDLE_ST         *g_pstSCDBHandle = NULL;

/* �Ƿ��Ѿ���ʼ��OK */
BOOL                 g_blSCInitOK     = DOS_FALSE;

U32          g_ulCPS                  = SC_MAX_CALL_PRE_SEC;
U32          g_ulMaxConcurrency4Task  = SC_MAX_CALL / 3;

/* define marcos */


/* define structs */
typedef struct tagSCModList{
    U32   ulIndex;
    S8    *pszName;
    U32   ulDefaultLogLevel;
    U32   (*fn_init)();
    U32   (*fn_start)();
    U32   (*fn_stop)();
}SC_MOD_LIST_ST;

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
    {SC_MOD_DB,         "db handle",          LOG_LEVEL_NOTIC, sc_init_db,           NULL,                   NULL},
    {SC_MOD_DB_WQ,      "db write queue",     LOG_LEVEL_NOTIC, sc_db_init,           sc_db_start,            sc_db_stop},
    {SC_MOD_DIGIST,     "digist log",         LOG_LEVEL_NOTIC, sc_log_digest_init,   sc_log_digest_start,    sc_log_digest_stop},
    {SC_MOD_RES,        "resource mngt",      LOG_LEVEL_NOTIC, sc_res_init,          sc_res_start,           sc_res_stop},
    {SC_MOD_ACD,        "agent mngt",         LOG_LEVEL_NOTIC, sc_agent_mngt_init,   NULL,                   NULL},
    {SC_MOD_EVENT,      "core event proc",    LOG_LEVEL_NOTIC, sc_event_init,        sc_event_start,         sc_event_stop},
    {SC_MOD_SU,         "esl event proc",     LOG_LEVEL_NOTIC, sc_su_mngt_init,      sc_su_mngt_start,       sc_su_mngt_stop},
    {SC_MOD_BS,         "bs msg proc",        LOG_LEVEL_NOTIC, sc_bs_fsm_init,       sc_bs_fsm_start,        sc_su_mngt_stop},
    {SC_MOD_EXT_MNGT,   "extension mngt",     LOG_LEVEL_NOTIC, sc_ext_mngt_init,     sc_ext_mngt_start,      sc_ext_mngt_stop},
    {SC_MOD_CWQ,        "call waiting queue", LOG_LEVEL_NOTIC, sc_cwq_init,          sc_cwq_start,           sc_cwq_stop},
    {SC_MOD_PUBLISH,    "http publish",       LOG_LEVEL_NOTIC, sc_pub_init,          sc_pub_start,           sc_pub_stop},
    {SC_MOD_ESL,        "esl send&recv",      LOG_LEVEL_NOTIC, sc_esl_init,          sc_esl_start,           sc_esl_stop},
    {SC_MOD_HTTP_API,   "http api",           LOG_LEVEL_NOTIC, sc_httpd_init,        sc_httpd_start,         sc_httpd_stop},
    {SC_MOD_DATA_SYN,   "data syn",           LOG_LEVEL_NOTIC, sc_data_syn_init,     sc_data_syn_start,      sc_data_syn_stop},

    {SC_MOD_TASK,       "task mngt",          LOG_LEVEL_NOTIC, sc_task_mngt_init,    sc_task_mngt_start,     sc_task_mngt_stop},
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

    /* �������ݿ� */
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


