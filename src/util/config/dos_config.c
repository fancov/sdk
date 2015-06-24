/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_config.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 统一配置项管理，由使用者自己维护
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include provate header file */
#include "config_api.h"

#if (INCLUDE_XML_CONFIG)

/* provate macro */
#ifdef ARM_VERSION
    #define GLB_CONFIG_FILE_PATH1 "/flash/apps/ptc/etc"
#else
    #define GLB_CONFIG_FILE_PATH1 "/etc"
#endif

#define GLB_CONFIG_FILE_PATH2 "../etc"

#define GLB_CONFIG_FILE_NAME "global.xml"


/* global variable */
/* 全局配置句柄 */
static mxml_node_t *g_pstGlobalCfg;

/**
 * 函数：S8* config_get_service_root(S8 *pszBuff, U32 ulLen)
 * 功能：获取服务程序的根目录
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回buff的指针，失败时返回null
 */
S8* config_get_service_root(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return NULL;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/service/path", "service_root", pszBuff, ulLen);
    if (!pszValue || *pszValue == '\0')
    {
        pszBuff[0] = '\0';
        return NULL;
    }

    return pszBuff;
}

/**
 * 函数：U32 config_get_mysql_host(S8 *pszBuff, U32 ulLen)
 * 功能：获取数据库主机名
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_host(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "host", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}

/**
 * 函数：U32 config_get_mysql_host()
 * 功能：获取数据库端口号
 * 参数：
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_port()
{
    S8 *pszValue = NULL;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 == usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = 3306;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

/**
 * 函数：U32 config_get_mysql_user(S8 *pszBuff, U32 ulLen)
 * 功能：获取ccsys数据库用户名
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_user(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "username", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}


/**
 * 函数：U32 config_get_syssrc_db_user(S8 *pszBuff, U32 ulLen)
 * 功能：获取资源监控数据库用户名
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */

U32 config_get_syssrc_db_user(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "src_username", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}


/**
 * 函数：U32 config_get_db_password(S8 *pszBuff, U32 ulLen)
 * 功能：获取ccsys数据库密码
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_password(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "password", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}


/**
 * 函数：U32 config_get_syssrc_db_password(S8 *pszBuff, U32 ulLen)
 * 功能：获取资源监控数据库密码
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_syssrc_db_password(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "src_password", pszBuff, ulLen);

    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}

/**
 * 函数：U32 config_get_db_dbname(S8 *pszBuff, U32 ulLen)
 * 功能：获取ccsys数据库名
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_dbname(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "dbname", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}

/**
 * 函数：U32 config_get_syssrc_db_dbname(S8 *pszBuff, U32 ulLen)
 * 功能：获取系统资源数据库名
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_syssrc_db_dbname(S8 *pszBuff, U32 ulLen)
{
    S8  *pszValue = NULL;


    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "src_dbname", pszBuff, ulLen);


    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}

/**
 * 函数：U32 config_get_py_path(S8 *pszBuff, U32 ulLen)
 * 功能：获取Python脚本路径
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
#if INCLUDE_SERVICE_PYTHON
U32 config_get_py_path(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/service/path/freeswitch", "fs_scripts_path", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}
#endif

/**
 * 函数：U32 config_get_mysqlsock_path(S8 *pszBuff, U32 ulLen)
 * 功能：获取MySQL的sock文件路径
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_mysqlsock_path(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (DOS_ADDR_INVALID(pszBuff)
        || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "sockpath", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}

U32 config_get_shortcut_cmd(U32 ulNo, S8 *pszCtrlCmd, U32 ulLen)
{
    S8 *pszCmd = NULL;
    S8 szNum[4] = {0};

    /* 将数字转换为字符串 */
    if (dos_ltoa(ulNo, szNum, sizeof(szNum)) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    szNum[sizeof(szNum) - 1] = '\0';
    pszCmd = _config_get_param(g_pstGlobalCfg, "config/shortcut", szNum, pszCtrlCmd, ulLen);
    if (!pszCmd)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 函数：U32 config_hh_get_send_interval()
 * 功能：获取心跳发送间隔
 * 参数：
 * 返回值：成功返回心跳间隔.失败返回0
 */
U32 config_hh_get_send_interval()
{
    S8 *pszValue;
    S8 szBuff[32];
    S32 lInterval;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/heartbeat", "interval", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        return 0;
    }

    lInterval = atoi(pszValue);
    if (lInterval < 0)
    {
        return 0;
    }

    return (U32)lInterval;
}

/**
 * 函数：U32 config_hb_get_max_fail_cnt()
 * 功能：获取发送最大失败次数
 * 参数：
 * 返回值：成功返回最大失败次数.失败返回0
 */
U32 config_hb_get_max_fail_cnt()
{
    S8 *pszValue;
    S8 szBuff[32];
    S32 lInterval;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/heartbeat", "max_fail_cnt", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        return 0;
    }

    lInterval = atoi(pszValue);
    if (lInterval < 0)
    {
        return 0;
    }

    return (U32)lInterval;
}

/**
 * 函数：S32 config_hb_get_treatment()
 * 功能：获取心跳超时的处理方式
 * 参数：
 * 返回值：成功返回处理方式编号.失败返回－1
 */
S32 config_hb_get_treatment()
{
    S8 *pszValue;
    S8 szBuff[32];
    S32 lInterval;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/heartbeat", "max_fail_cnt", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        return -1;
    }

    lInterval = atoi(pszValue);
    if (lInterval < 0)
    {
        return -1;
    }

    return lInterval;
}

/**
 * 函数：U32 config_init()
 * 功能： 初始化配置模块
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数初始化配置模块。配置文件的搜索路径顺序为：
 *      1.系统配置文件目录
 *      2."../etc/" 把当前目录看作bin目录，上层目录为当前服务根目录，在服务根目录下面查找
 */
U32 config_init()
{
    S8 szBuffFilename[256];

    dos_printf("%s", "Start init config.");

    snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", GLB_CONFIG_FILE_PATH1, GLB_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the global config file in system etc. (%s)", szBuffFilename);

        snprintf(szBuffFilename, sizeof(szBuffFilename)
                        , "%s/%s", GLB_CONFIG_FILE_PATH2, GLB_CONFIG_FILE_NAME);
        if (access(szBuffFilename, R_OK|W_OK) < 0)
        {
            dos_printf("Cannon find the global config file in service etc. (%s)", szBuffFilename);
            dos_printf("%s", "Cannon find the global config. Config init failed.");
            DOS_ASSERT(0);
            return -1;
        }
    }

    dos_printf("Use file %s as global config file.", szBuffFilename);

    g_pstGlobalCfg = _config_init(szBuffFilename);
    if (!g_pstGlobalCfg)
    {
        dos_printf("%s", "Init global config fail.");
        DOS_ASSERT(0);
        return -1;
    }
    dos_printf("%s", "Config init successfully.");

    return 0;
}

/**
 * 函数：U32 config_deinit()
 * 功能： 销毁配置模块
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
U32 config_deinit()
{
    S32 lRet = 0;

    lRet = _config_deinit(g_pstGlobalCfg);
    g_pstGlobalCfg = NULL;

    return lRet;
}

S32 config_save()
{
     S8 szBuffFilename[256];

     snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", GLB_CONFIG_FILE_PATH1, GLB_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the global config file in system etc. (%s)", szBuffFilename);

        snprintf(szBuffFilename, sizeof(szBuffFilename)
                        , "%s/%s", GLB_CONFIG_FILE_PATH2, GLB_CONFIG_FILE_NAME);
        if (access(szBuffFilename, R_OK|W_OK) < 0)
        {
            dos_printf("Cannon find the global config file in service etc. (%s)", szBuffFilename);
            dos_printf("%s", "Cannon find the global config. Config init failed.");
            DOS_ASSERT(0);
            return -1;
        }
    }

    if (_config_save(g_pstGlobalCfg, szBuffFilename) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    return 0;
}

/* pts配置接口 */
#ifdef INCLUDE_PTS

/**
 * 函数：U32 config_get_pts_port()
 * 功能：获取pts的端口
 * 参数：
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_pts_port1()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/pts", "pts_port1", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_SERVER_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

U32 config_get_pts_port2()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/pts", "pts_port2", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_SERVER_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}


/**
 * 函数：U32 config_get_goahead_server_port()
 * 功能：获取pts的web server的端口
 * 参数：
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_web_server_port()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/pts", "web_server_port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_WEB_SERVER_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

/**
 * 函数：U32 config_get_pts_proxy_port()
 * 功能：获取pts的web server的端口
 * 参数：
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_pts_proxy_port()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/pts", "proxy_port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_PROXY_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

/**
 * 函数：U32 config_get_pts_telnet_server_port()
 * 功能：获取pts的telnet server的端口
 * 参数：
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_pts_telnet_server_port()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/pts", "telnet_server_port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_TELNETD_LINSTEN_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

/**
 * 函数：U32 config_get_pts_domain_dbname(S8 *pszBuff, U32 ulLen)
 * 功能：获取pts的域名
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
/*U32 config_get_pts_domain(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/pts", "pts_domain", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    return DOS_SUCC;
}*/

#endif

/* ptc配置接口 */
#ifdef INCLUDE_PTC

/**
 * 函数：U32 config_get_pts_domain_dbname(S8 *pszBuff, U32 ulLen)
 * 功能：获取pts的域名
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_pts_major_domain(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/ptc", "pts_major_domain", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 config_set_pts_major_domain(S8 *pszBuff)
{
    if (!pszBuff)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    S32 lResult = 0;

    lResult = _config_set_param(g_pstGlobalCfg, "config/ptc", "pts_major_domain", pszBuff);
    if (lResult < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 config_get_pts_minor_domain(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/ptc", "pts_minor_domain", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 config_set_pts_minor_domain(S8 *pszBuff)
{
    if (!pszBuff)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    S32 lResult = 0;

    lResult = _config_set_param(g_pstGlobalCfg, "config/ptc", "pts_minor_domain", pszBuff);
    if (lResult < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 函数：U32 config_get_pts_port()
 * 功能：获取pts的端口
 * 参数：
 * 返回值：
 */
U32 config_get_pts_major_port()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/ptc", "pts_major_port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_SERVER_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

U32 config_set_pts_major_port(S8 *pszBuff)
{
    if (!pszBuff)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    S32 lResult = 0;

    lResult = _config_set_param(g_pstGlobalCfg, "config/ptc", "pts_major_port", pszBuff);
    if (lResult < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 函数：U32 config_get_pts_port()
 * 功能：获取pts的端口
 * 参数：
 * 返回值：
 */
U32 config_get_pts_minor_port()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/ptc", "pts_minor_port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_SERVER_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

U32 config_set_pts_minor_port(S8 *pszBuff)
{
    if (!pszBuff)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    S32 lResult = 0;

    lResult = _config_set_param(g_pstGlobalCfg, "config/ptc", "pts_minor_port", pszBuff);
    if (lResult < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 函数：U32 config_get_ptc_name(S8 *pszBuff, U32 ulLen)
 * 功能：获取ptc的ID
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_ptc_name(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }
    pszValue = _config_get_param(g_pstGlobalCfg, "config/ptc", "ptc_name", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

#endif

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

