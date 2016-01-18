/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  config_hb_srv.c
 *
 *  Created on: 2015-12-24
 *      Author: Lion
 *        Todo: 统一配置项管理，由使用者自己维护
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include dos public header files */
#include <dos.h>

/* include provate header file */
#include "config_api.h"

#if (INCLUDE_XML_CONFIG)

#if INCLUDE_MONITOR

/* provate macro */
#define WARNING_CONFIG_FILE_PATH "/ipcc/etc/pub"

#define WARNING_CONFIG_FILE_NAME "warn-config.xml"


/* global variable */
/* 全局配置句柄 */
static mxml_node_t *g_pstWaringCfg = NULL;


/**
 * 函数：S32 config_hb_init()
 * 功能： 初始化心跳模块的配置
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数初始化配置模块。配置文件的搜索路径顺序为：
 *      1.系统配置文件目录
 *      2."../etc/" 把当前目录看作bin目录，上层目录为当前服务根目录，在服务根目录下面查找
 */
S32 config_warn_init()
{
    S8 szBuffFilename[256];

    dos_printf("%s", "Start init hb-srv config.");

    snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", WARNING_CONFIG_FILE_PATH, WARNING_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the warn-config.xml file. (%s)", szBuffFilename);
        dos_printf("%s", "Cannon find the warn-config.xml. Config init failed.");
        DOS_ASSERT(0);
        return -1;
    }

    dos_printf("Use file %s as hb server config file.", szBuffFilename);

    g_pstWaringCfg = _config_init(szBuffFilename);
    if (!g_pstWaringCfg)
    {
        dos_printf("%s", "Init warn-config.xml config fail.");
        DOS_ASSERT(0);
        return -1;
    }
    dos_printf("%s", "warn-config config init successfully.");

    return 0;
}

/**
 * 函数：S32 config_hb_deinit()
 * 功能： 销毁心跳模块配置
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_warn_deinit()
{
    S32 lRet = 0;

    lRet = _config_deinit(g_pstWaringCfg);
    g_pstWaringCfg = NULL;

    return lRet;
}

/**
 * 函数：S32 config_hb_save()
 * 功能：保存心跳模块配置文件
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_warn_save()
{
    S8 szBuffFilename[256];

    dos_printf("%s", "Start save hb server config.");

    snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", WARNING_CONFIG_FILE_PATH, WARNING_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the warn-config.xml file in service etc. (%s)", szBuffFilename);
        dos_printf("%s", "Cannon find the warn-config.xml. save failed.");
        DOS_ASSERT(0);
        return -1;
    }

    dos_printf("Save process list to file %s.", szBuffFilename);

    if (_config_save(g_pstWaringCfg, szBuffFilename) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    return 0;
}

/**
 *  函数: S32 config_hb_get_mail_server(S8 *pszServer, U32 ulLen)
 *  参数:
 *      S8 *pszServer  SMTP服务器名称
 *      U32 ulLen      长度
 *  功能: 获取SMTP服务器名称
 *  返回: 成功返回0，否则返回-1
 */
S32 config_warn_get_mail_server(S8 *pszServer, U32 ulLen)
{
    S8  szNodePath[] = "config/mail";
    S8  *pszSMTP = NULL;

    if (DOS_ADDR_INVALID(g_pstWaringCfg)
        || DOS_ADDR_INVALID(pszServer))
    {
        dos_printf("%s", ".Init warn-config.xml config fail!");
        DOS_ASSERT(0);
        return -1;
    }
    pszSMTP = _config_get_param(g_pstWaringCfg, szNodePath, "server", pszServer, ulLen);
    if (!pszSMTP)
    {
        dos_printf("%s", "Get SMTP Server FAIL.");
        DOS_ASSERT(0);
        return -1;
    }
    return 0;
}

/**
 *  函数: S32 config_hb_get_auth_username(S8 *pszServer, U32 ulLen)
 *  参数:
 *      S8 *pszServer  认证用户名
 *      U32 ulLen      用户名长度
 *  功能: 获取SMTP默认账户的认证名
 *  返回: 成功返回0，否则返回-1
 */
S32 config_warn_get_auth_username(S8 *pszUsername, U32 ulLen)
{
    S8  szNodePath[] = "config/mail";
    S8  *szServer = NULL;

    if(!g_pstWaringCfg)
    {
        dos_printf("%s", ".Init warn-config.xml config fail!");
        DOS_ASSERT(0);
        return -1;
    }
    szServer = _config_get_param(g_pstWaringCfg, szNodePath, "username", pszUsername, ulLen);
    if (!szServer)
    {
        dos_printf("%s", "Get Auth Username FAIL.");
        DOS_ASSERT(0);
        return -1;
    }
    return 0;
}

/**
 *  函数: S32 config_hb_get_auth_passwd(S8 *pszPasswd, U32 ulLen)
 *  参数:
 *      S8 *pszPasswd  认证密码
 *      U32 ulLen      密码长度
 *  功能: 获取SMTP默认账户的认证密码
 *  返回: 成功返回0，否则返回-1
 */
S32 config_warn_get_auth_passwd(S8 *pszPasswd, U32 ulLen)
{
    S8  szNodePath[] = "config/mail";
    S8  *szServer = NULL;

    if(!g_pstWaringCfg)
    {
        dos_printf("%s", ".Init warn-config.xml config fail!");
        DOS_ASSERT(0);
        return -1;
    }

    szServer = _config_get_param(g_pstWaringCfg, szNodePath, "password", pszPasswd, ulLen);
    if (!szServer)
    {
        dos_printf("%s", "Get Auth Username FAIL.");
        DOS_ASSERT(0);
        return -1;
    }
    return 0;
}

U32 config_warn_get_mail_port()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usPort = 0;
    S32 lPort = 0;

    if(!g_pstWaringCfg)
    {
        dos_printf("%s", ".Init warn-config.xml config fail!");
        DOS_ASSERT(0);
        return -1;
    }

    pszValue = _config_get_param(g_pstWaringCfg, "config/mail", "port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usPort || usPort >= U16_BUTT)
    {
        usPort = DOS_MON_MAIL_PORT;
    }
    else
    {
        usPort = (U16)lPort;
    }

    return usPort;
}

#endif //end  #if (INCLUDE_MONITOR)

#endif //end  #if (INCLUDE_XML_CONFIG)

#ifdef __cplusplus
}
#endif /* __cplusplus */

