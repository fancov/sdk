/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: mod_dipcc_sc.c
 *
 *  创建时间: 2014年12月16日14:03:55
 *  作    者: Larry
 *  描    述: 业务模块主函数入口
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <bs_pub.h>
#include <dos/dos_py.h>
#include <esl.h>

/* include private header files */
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_httpd.h"
#include "sc_httpd_def.h"
#include "sc_debug.h"

/* 数据库句柄 */
DB_HANDLE_ST         *g_pstSCDBHandle = NULL;
BOOL                 g_blSCInitOK     = DOS_FALSE;
extern SC_TASK_MNGT_ST   *g_pstTaskMngtInfo;

U32 sc_httpd_init();
U32 sc_task_mngt_init();
U32 sc_dialer_init();
U32 sc_ep_init();
U32 sc_acd_init();
U32 sc_acd_start();
U32 sc_bs_fsm_init();
U32 sc_ep_ext_start();
U32 sc_httpd_start();
U32 sc_task_mngt_start();
U32 sc_dialer_start();
U32 sc_bs_fsm_start();
U32 sc_ep_start();
U32 sc_httpd_shutdown();
U32 sc_task_mngt_shutdown();
U32 sc_dialer_shutdown();
U32 sc_ep_init_agent_status();
U32 sc_data_syn_init();
U32 sc_data_syn_start();
U32 sc_ep_ext_init();
U32 sc_ep_ext_start();
U32 sc_audit_init();
U32 sc_audit_start();


/* define marcos */

/* define enums */

/* define structs */

/* global variables */


/* declare functions */

BOOL g_blSCIsRunning = DOS_FALSE;

U32 sc_init_db()
{
    U16             usDBPort;
    S8              szDBHost[DB_MAX_STR_LEN] = {0, };
    S8              szDBUsername[DB_MAX_STR_LEN] = {0, };
    S8              szDBPassword[DB_MAX_STR_LEN] = {0, };
    S8              szDBName[DB_MAX_STR_LEN] = {0, };
    S8              szDBSockPath[DB_MAX_STR_LEN] = {0, };

    SC_TRACE_IN(0, 0, 0, 0);

    if (config_get_db_host(szDBHost, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (config_get_db_user(szDBUsername, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (config_get_db_password(szDBPassword, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
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
        SC_TRACE_OUT();
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

        SC_TRACE_OUT();
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

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}


U32 mod_dipcc_sc_load()
{
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Start init SC.");

    if (sc_init_db() != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Init DB Handle FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Init DB Handle Successfully.");

#if 0 //INCLUDE_SERVICE_PYTHON
    /* 全局加载freeswitch配置文件xml */
    if (DOS_SUCC != py_exec_func("customer", "generate_all_customer", "()"))
    {
        DOS_ASSERT(0);
        logr_error("mod_dipcc_sc_load: Load sip xml FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Load sip XML SUCC.");

    if (DOS_SUCC != py_exec_func("router", "phone_route", "()"))
    {
        DOS_ASSERT(0);
        logr_error("mod_dipcc_sc_load: Load gateway XML FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Load gateway XML SUCC.");

#endif

    if (sc_httpd_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Init httpd server FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Init httpd server Successfully.");

    if (sc_task_mngt_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Init task mngt FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Init task mngt Successfully.");

    if (sc_dialer_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Init dialer FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Init dialer Successfully.");

    if (sc_ep_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Init event process module FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Init event process module Successfully.");

    if (DOS_SUCC != sc_acd_init())
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Init acd module FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Init acd module Successfully.");

#if 0
    if (DOS_SUCC != sc_ep_init_agent_status())
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Init Agent status FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Init Agent status Successfully.");
#endif

    if (DOS_SUCC != sc_bs_fsm_init())
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Init bs client FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Init bs client Successfully.");

    if (DOS_SUCC != sc_cwq_init())
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Init call waiting queue FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Init call waiting queue SUCC.");

    if (DOS_SUCC != sc_ep_ext_init())
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Init extension mngt task FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Init extension mngt task SUCC.");

    if (DOS_SUCC != sc_data_syn_init())
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Init Data Syn task FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Init Data Syn task SUCC.");

    if (DOS_SUCC != sc_audit_init())
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Init audit task FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Init audit task SUCC.");

    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Finished to init SC.");

    g_blSCInitOK = DOS_TRUE;

    return DOS_SUCC;
}

U32 mod_dipcc_sc_runtime()
{
    if (DOS_SUCC != sc_bs_fsm_start())
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Start bs client FAIL");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Start bs client Successfully.");


    if (sc_dialer_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Start dialer FAIL");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Start dialer Successfully.");

    if (sc_cwq_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Start call waiting queue task FAIL");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Start call waiting queue task SUCC.");

    if (sc_ep_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Start start event process task FAIL");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Start start event process task Successfully.");

    if (sc_task_mngt_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Start mngt service FAIL");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Start mngt service Successfully.");

    if (sc_httpd_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Start the httpd FAIL");
        return DOS_FAIL;;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Start the httpd task Successfully.");

    if (DOS_SUCC != sc_ep_ext_start())
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Start extension mngt task FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Start extension mngt task SUCC.");


    if (DOS_SUCC != sc_data_syn_start())
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Start data syn task FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Start data syn task SUCC.");

    if (DOS_SUCC != sc_audit_start())
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Start audit task FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Start audit task SUCC.");

    if (DOS_SUCC != sc_acd_start())
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_SUB_MOD_BUTT, "%s", "Start the agent status query task FAIL.");
        return DOS_FAIL;
    }
    sc_logr_info(SC_SUB_MOD_BUTT, "%s", "Start the agent status query task SUCC.");

    g_pstTaskMngtInfo->stStat.ulSystemUpTime = time(0);
    g_pstTaskMngtInfo->stStat.ulSystemIsWorking = DOS_TRUE;

    return DOS_SUCC;
}

U32 mod_dipcc_sc_shutdown()
{
    sc_httpd_shutdown();
    sc_task_mngt_shutdown();
    sc_dialer_shutdown();

#if INCLUDE_SERVICE_PYTHON
    py_deinit();
#endif

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


