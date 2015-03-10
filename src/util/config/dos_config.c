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
#define GLB_CONFIG_FILE_PATH1 "/etc"
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
    S8 *pszValue;

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
    S8 *pszValue;

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
 * 功能：获取数据库主机名
 * 参数：
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_port()
{
    S8 *pszValue;
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
 * 功能：获取数据库主机名
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_user(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue;

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
 * 函数：U32 config_get_mysql_password(S8 *pszBuff, U32 ulLen)
 * 功能：获取数据库主机名
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_password(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue;

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
 * 函数：U32 config_get_mysql_dbname(S8 *pszBuff, U32 ulLen)
 * 功能：获取数据库主机名
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_dbname(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue;

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

U32 config_get_py_path(S8 *pszBuff, U32 ulLen)
{
   S8 *pszValue;

   if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/py_path", "path", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
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

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

