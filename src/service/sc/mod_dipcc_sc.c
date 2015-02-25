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
#include "sc_pub.h"
#include "sc_task_pub.h"
#include "sc_debug.h"

/* include private header files */
#include "bs_pub.h"
#include "sc_httpd.h"
#include "sc_httpd_pub.h"
#include "sc_pub.h"
#include "sc_bs_pub.h"
#include "sc_task_pub.h"
#include "sc_debug.h"
#include "sc_event_process.h"

U32 sc_httpd_init();
U32 sc_task_mngt_init();
U32 sc_dialer_init();
U32 sc_ep_init();
U32 sc_acd_init();
U32 sc_bs_fsm_init();
U32 sc_httpd_start();
U32 sc_task_mngt_start();
U32 sc_dialer_start();
U32 sc_bs_fsm_start();
U32 sc_ep_start();
U32 sc_httpd_shutdown();
U32 sc_task_mngt_shutdown();
U32 sc_dialer_shutdown();


extern U32 g_ulTaskTraceAll;

/* define marcos */

/* define enums */

/* define structs */

/* global variables */


/* declare functions */

BOOL g_blSCIsRunning = DOS_FALSE;


U32 mod_dipcc_sc_load()
{
#if 1
    if (sc_httpd_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#endif
    if (sc_task_mngt_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#if 1
    if (sc_dialer_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#endif
    if (sc_ep_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_SUCC != sc_acd_init())
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#if 1
    if (DOS_SUCC != sc_bs_fsm_init())
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#endif
    return DOS_SUCC;
}

U32 mod_dipcc_sc_runtime()
{
#if 1
    if (sc_httpd_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;;
    }
#endif
    if (sc_task_mngt_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#if 1
    if (sc_dialer_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_SUCC != sc_bs_fsm_start())
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#endif


    if (sc_ep_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 mod_dipcc_sc_shutdown()
{
    sc_httpd_shutdown();
    sc_task_mngt_shutdown();
    sc_dialer_shutdown();

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


