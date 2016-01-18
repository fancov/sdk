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
#include <http_api.h>

#if INCLUDE_SERVICE_PYTHON
#include <dos/dos_py.h>
#endif

#if INCLUDE_OPENSSL_LIB
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#endif

#if INCLUDE_SERVICE_MC
#include <mc_pub.h>
#endif


#if (INCLUDE_LICENSE_SERVER || INCLUDE_LICENSE_CLIENT)
#include <license.h>
#endif

#if INCLUDE_PTC
    #include <pt/ptc.h>
#elif INCLUDE_PTS
    #include <pt/pts.h>
#endif

#if INCLUDE_SERVICE_BS
S32 bs_start();
#endif

#if INCLUDE_RES_MONITOR
U32 mon_init();
U32 mon_start();
U32 mon_stop();
extern U32 mon_init_notify_list();
#endif


#if INCLUDE_CC_SC
U32 sc_init();
U32 sc_start();
#endif

S32 root(S32 _argc, S8 ** _argv)
{
#if INCLUDE_SERVICE_MC
    if (mc_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);

        dos_printf("Media convert task init fail");
        return DOS_FAIL;
    }
    dos_printf("Media convert task init success");

    if (mc_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);

        dos_printf("Media convert task start fail");
        return DOS_FAIL;
    }

    dos_printf("Media convert task start success");
#endif

#if INCLUDE_DEBUG_CLI_SERVER
    telnetd_start();
#elif INCLUDE_BH_SERVER
    S32 lRet = 0;

    if (DOS_SUCC != mon_init_notify_list())
    {
        DOS_ASSERT(0);
        exit(0);
    }
    dos_printf("%s", "Notify List Init SUCC.");

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
        DOS_ASSERT(0);
        return -1;
    }
    lRet = mon_start();
    if(DOS_SUCC != lRet)
    {
        DOS_ASSERT(0);
        lRet = mon_stop();
        if(DOS_SUCC != lRet)
        {
            DOS_ASSERT(0);
        }
        return -1;
    }
#endif //INCLUDE_RES_MONITOR

#endif //INCLUDE_DEBUG_CLI_SERVER

#if INCLUDE_PTC
    if (DOS_SUCC != ptc_main())
    {
        DOS_ASSERT(0);
        return -1;
    }
#elif INCLUDE_PTS
    if (DOS_SUCC != pts_main())
    {
        DOS_ASSERT(0);
        return -1;
    }
#endif

#if INCLUDE_SERVICE_BS
    if (bs_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return -1;
    }

    logr_info("%s", "BS Start.");
#endif

#ifdef INCLUDE_CC_SC

    if (DOS_SUCC != sc_init())
    {
        DOS_ASSERT(0);

        logr_info("%s", "SC INIT FAIL.");
        return -1;
    }

    if (DOS_SUCC != sc_start())
    {
        DOS_ASSERT(0);

        logr_info("%s", "SC start FAIL.");
        return -1;
    }

    logr_info("%s", "SC start.");
#endif

#if INCLUDE_OPENSSL_LIB
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    OpenSSL_add_all_digests();
    OpenSSL_add_all_ciphers();
    ERR_load_crypto_strings();
    ERR_load_SSL_strings();
#endif


#if INCLUDE_HTTP_API
    http_api_init();
    http_api_start();
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

