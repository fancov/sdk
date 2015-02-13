/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  root.c
 *
 *  Created on: 2014-11-3
 *      Author: Larry
 *        Todo: 启动该进程的主服务程序
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if INCLUDE_RES_MONITOR
#include <mon_pub.h>
#endif

#if INCLUDE_PTC
    #include <ptc_pts/ipcc.h>
#endif

#if INCLUDE_SERVICE_BS
S32 bs_start();
#endif

#if INCLUDE_CC_SC
U32 mod_dipcc_sc_load();
U32 mod_dipcc_sc_runtime();
#endif
S32 root(S32 _argc, S8 ** _argv)
{
#if INCLUDE_DEBUG_CLI_SERVER
    telnetd_start();
#elif INCLUDE_BH_SERVER
    S32 lRet = 0;

    if (heartbeat_init())
    {
        DOS_ASSERT(0);
        exit(0);
    }
    dos_printf("%s", "Heartbeat init successfully.");

    heartbeat_start();
#if INCLUDE_RES_MONITOR
    lRet = mon_init();
    if(DOS_SUCC != lRet)
    {
       logr_error("%s:Line %d:root|init resource failure!"
                    , dos_get_filename(__FILE__), __LINE__);
       return -1;
    }
    lRet = mon_start();
    if(DOS_SUCC != lRet)
    {
       logr_error("%s:Line %d:root|start monitor failure!"
                    , dos_get_filename(__FILE__), __LINE__);
       lRet = mon_stop();
       if(DOS_SUCC != lRet)
       {
          logr_error("%s:Line %d:root|stop monitor failure!"
                    , dos_get_filename(__FILE__), __LINE__);
       }
       return -1;
    }
#endif //INCLUDE_RES_MONITOR

#endif //INCLUDE_DEBUG_CLI_SERVER

#if INCLUDE_PTC
    ptc_main();
#endif //INCLUDE_PTC

#if INCLUDE_SERVICE_BS
    if (bs_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return -1;
    }

    logr_info("%s", "BS Start.");
#endif



#ifdef INCLUDE_CC_SC

    if (DOS_SUCC != mod_dipcc_sc_load())
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (DOS_SUCC != mod_dipcc_sc_runtime())
    {
        DOS_ASSERT(0);
        return -1;
    }
#endif

    while(1)
    {
        sleep(1);
    }

    return 0;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

