/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  syscfg.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 进程信息相关函数
 *     History:
 */

/*
#ifdef __cplusplus
extern "C"{
#endif*/ /* __cplusplus */
    
#include <dos.h>

#define MAX_PROCESS_NAME_LENGTH   48

#define MAX_PATH_LENGTH           256

/* 定时模块相关信息 */
/* 进程名 */
static S8 g_pszProcessName[MAX_PROCESS_NAME_LENGTH] = { 0 };

/* 进程版本 */
static S8 *g_pszProcessVersion = "1.1";

/* 定义默认系统根目录 */
static S8 *g_pszDefaultSysRootPath = "/ttcom";

/* 存放配置文件中系统根目录 */
static S8 g_szSysRootPath[MAX_PATH_LENGTH] = {0, };

/**
 * Function: S8 *dos_get_process_name()
 *     Todo: 获取该进程名称
 *   Return: 返回该进程名称
 */
S8 *dos_get_process_name()
{
    return g_pszProcessName;
}

/**
 * Function: S8 *dos_get_process_version()
 *     Todo: 获取该进程版本号
 *   Return: 返回该进程版本号
 */
S8 *dos_get_process_version()
{
    return g_pszProcessVersion;
}

/*
 *  函数：S8 *dos_get_pid_file_pah(S8 *pszBuff, S32 lMaxLen)
 *  功能：获取pid文件的路径
 *  参数：
 *      S8 *pszBuff  数据缓存
 *      S32 lMaxLen  缓存的长度
 *  返回值:
 *      如果一切正常，将会把pid文件路径copy到缓存，并且返回缓存得首地址；
 *      如果异常，将回返NULL, 缓存中的数据将会是随机数据
 */
S8 *dos_get_pid_file_path(S8 *pszBuff, S32 lMaxLen)
{
    S32 lRet = 0;
    S8 szSysRootPath[256];

    if (!pszBuff || lMaxLen <= 0)
    {
        DOS_ASSERT(0);
        return 0;
    }

    if (config_get_service_root(szSysRootPath, sizeof(szSysRootPath)))
    {
        lRet = snprintf(pszBuff, lMaxLen, "%s/var/run/pid", szSysRootPath);
    }
    else
    {
        lRet = snprintf(pszBuff, lMaxLen, "%s/var/run/pid", g_pszDefaultSysRootPath);
    }

    if (lRet >= lMaxLen)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    return pszBuff;
}

/**
 * 函数：S8 *dos_get_sys_root_path()
 * 功能：获取系统根目录
 * 参数：
 * 返回值：返回配置文件中的根目录，或者系统模块根目录
 */
S8 *dos_get_sys_root_path()
{
    S8 szSysRootPath[256];
    S8 *pszPath;

    if ('\0' != g_szSysRootPath[0])
    {
        return g_szSysRootPath;
    }
    else
    {
        pszPath = config_get_service_root(szSysRootPath, sizeof(szSysRootPath));
        if (pszPath)
        {
            dos_strncpy(g_szSysRootPath, szSysRootPath, sizeof(g_szSysRootPath));
            g_szSysRootPath[MAX_PATH_LENGTH - 1] = '\0';

            return g_szSysRootPath;
        }
        else
        {
            return g_pszDefaultSysRootPath;
        }
    }
}

/**
 * 函数：U32 dos_set_process_name(S8 *pszName)
 * 功能：设置当前进程的名字
 * 参数：
 *      S8 *pszName : 启动时的进程名（可能会带上全路径）
 * 返回值
 *      成功返回0，失败返回－1
 */
S32 dos_set_process_name(S8 *pszName)
{
    S8 *pSprit = NULL;
    S8 *pNext = NULL;

    if (!pszName || '\0' == pszName)
    {
        DOS_ASSERT(0);
        return -1;
    }

    /* 可能带了路径，我们只需要最后一段 */
    pSprit = pszName;
    pNext = dos_strchr(pSprit, '/');
    while(pNext)
    {
        pSprit = pNext + 1;
        if ('\0' == pSprit[0])
        {
            break;
        }

        pNext = dos_strchr(pSprit, '/');
    }

    /* 存储一下 */
    if ('\0' == pSprit[0])
    {
        DOS_ASSERT(0);
        g_pszProcessName[0] = '\0';
    }
    else
    {
        dos_strncpy(g_pszProcessName, pSprit, sizeof(g_pszProcessName));
        g_pszProcessName[sizeof(g_pszProcessName) - 1] = '\0';
    }

    return 0;
}

/*
#ifdef __cplusplus
}
#endif*/ /* __cplusplus */
