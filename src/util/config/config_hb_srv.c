/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  config_hb_srv.c
 *
 *  Created on: 2014-11-14
 *      Author: Larry
 *        Todo: 统一配置项管理，由使用者自己维护
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include dos public header files */
#include <dos.h>

#if INCLUDE_BH_SERVER
/* include provate header file */
#include "config_api.h"

#if (INCLUDE_XML_CONFIG)

/* provate macro */
#define HB_CONFIG_FILE_PATH1 "/etc"
#define HB_CONFIG_FILE_PATH2 "../etc"

#define HB_CONFIG_FILE_NAME "hb-srv.xml"


/* global variable */
/* 全局配置句柄 */
static mxml_node_t *g_pstHBSrvCfg = NULL;


/**
 * 函数：config_hb_get_process_list
 * 功能：获取指定index的路径下的进程信息
 * 参数：
 *      U32 ulIndex：xml中process的路径编号
 *      S8 *pszName：输出参数，存放进程名的缓存
 *      U32 ulNameLen：存放进程名缓存的长度
 *      S8 *pszVersion：输出参数，存放进程版本号
 *      U32 ulVerLen：存放进程版本号缓存的长度
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 config_hb_get_process_list(U32 ulIndex, S8 *pszName, U32 ulNameLen, S8 *pszVersion, U32 ulVerLen)
{
    S8 szXmlPath[256] = { 0 };
    S8 szProcessName[DOS_MAX_PROCESS_NAME_LEN] = { 0 };
    S8 szProcessVersion[DOS_MAX_PROCESS_VER_LEN] = { 0 };
    S8 *pszValue = NULL;

    if (!pszName || 0 == ulNameLen)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (!pszVersion || 0 == ulVerLen)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (!g_pstHBSrvCfg)
    {
        DOS_ASSERT(0);
        return -1;
    }

    snprintf(szXmlPath, sizeof(szXmlPath), "config/process/%d", ulIndex);
    pszValue = _config_get_param(g_pstHBSrvCfg, szXmlPath, "name", szProcessName, sizeof(szProcessName));
    if (!pszValue || '\0' == pszValue[0])
    {
        return -1;
    }
    pszValue = _config_get_param(g_pstHBSrvCfg, szXmlPath, "version", szProcessVersion, sizeof(szProcessVersion));

    dos_strncpy(pszName, szProcessName, ulNameLen);
    pszName[ulNameLen - 1] = '\0';

    if (!pszValue || '\0' == pszValue[0])
    {
        dos_strncpy(pszVersion, szProcessVersion, ulVerLen);
        pszVersion[ulVerLen - 1] = '\0';
    }
    else
    {
        pszVersion[0] = '\0';
    }

    return 0;
}


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
S32 config_hb_init()
{
    S8 szBuffFilename[256];

    dos_printf("%s", "Start init hb-srv config.");

    snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", HB_CONFIG_FILE_PATH1, HB_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the hb-srv.xml file in system etc. (%s)", szBuffFilename);

        snprintf(szBuffFilename, sizeof(szBuffFilename)
                        , "%s/%s", HB_CONFIG_FILE_PATH2, HB_CONFIG_FILE_NAME);
        if (access(szBuffFilename, R_OK|W_OK) < 0)
        {
            dos_printf("Cannon find the hb-srv.xml file in service etc. (%s)", szBuffFilename);
            dos_printf("%s", "Cannon find the hb-srv.xml. Config init failed.");
            DOS_ASSERT(0);
            return -1;
        }
    }

    dos_printf("Use file %s as hb server config file.", szBuffFilename);

    g_pstHBSrvCfg = _config_init(szBuffFilename);
    if (!g_pstHBSrvCfg)
    {
        dos_printf("%s", "Init hb-srv.xml config fail.");
        DOS_ASSERT(0);
        return -1;
    }
    dos_printf("%s", "hb-srv config init successfully.");

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
S32 config_hb_deinit()
{
    S32 lRet = 0;

    lRet = _config_deinit(g_pstHBSrvCfg);
    g_pstHBSrvCfg = NULL;

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
S32 config_hb_save()
{
    S8 szBuffFilename[256];

    dos_printf("%s", "Start save hb server config.");

    snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", HB_CONFIG_FILE_PATH1, HB_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the hb-srv.xml file in system etc. (%s)", szBuffFilename);

        snprintf(szBuffFilename, sizeof(szBuffFilename)
                        , "%s/%s", HB_CONFIG_FILE_PATH2, HB_CONFIG_FILE_NAME);
        if (access(szBuffFilename, R_OK|W_OK) < 0)
        {
            dos_printf("Cannon find the hb-srv.xml file in service etc. (%s)", szBuffFilename);
            dos_printf("%s", "Cannon find the hb-srv.xml. save failed.");
            DOS_ASSERT(0);
            return -1;
        }
    }

    dos_printf("Save process list to file %s.", szBuffFilename);

    if (_config_save(g_pstHBSrvCfg, szBuffFilename) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    return 0;
}

#endif

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

