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


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <version.h>

#define MAX_PROCESS_NAME_LENGTH   48

#define MAX_PATH_LENGTH           256

typedef struct tagModDesc{
    U32 ulIndex;           /* 模块ID */
    S8  *pszName;          /* 模块名称 */
    S8  *pzsMask;          /* 模块掩码 */
    U32 ulDefaultLimit;    /* 默认限制 */
    S8  *pszExtraData;     /* 其他数据 */
}MOD_DESC_ST;

/* 定时模块相关信息 */
/* 进程名 */
static S8 g_pszProcessName[MAX_PROCESS_NAME_LENGTH] = { 0 };

/* 定义默认系统根目录 */
static S8 *g_pszDefaultSysRootPath = "/ipcc";

/* 存放配置文件中系统根目录 */
static S8 g_szSysRootPath[MAX_PATH_LENGTH] = {0, };

/* 定义各个模块的掩码 */
static MOD_DESC_ST g_stModList[] = {
#ifdef DIPCC_PTS
    {PTS_SUBMOD_PTCS,   "PTC",        "pts-ptc",  2,   ""},
#endif
#ifdef DIPCC_FREESWITCH
    {LIC_EIA_AGENT,            "EIA AGENT",       "eia-agent",     1,        ""},
    {LIC_RECORD,               "RECORD",          "record",        1,        ""},
    {LIC_VOICE_VERIFY,         "VOICE VERIFY",    "voice-verify",  1,        ""},
    {LIC_SMS,                  "SMS",             "sms send/recv", 1,        ""},
    {LIC_GRAPHIC_REPORT,       "GRAPH REPORT",    "graph report",  1,        ""},

    {LIC_TOTAL_CALLS,          "CALLS",           "total calls",   1500,     ""},
    {LIC_AGENTS,               "AGENTS",          "total agents",  1500,     ""},
    {LIC_PTCS,                 "PTCS",            "ptc numbers",   1500,     ""},
    {LIC_G723,                 "G723",            "G723 Calls",    5000,     ""},
    {LIC_G729,                 "G729",            "G729 Calls",    5000,     ""},

    {LIC_TOTAL_CALLTIME,       "TOTAL CALL TIME", "total call time",     0,        ""},
    {LIC_OUTBOUND_CALLTIME,    "OUT CALL TIME",   "outgoing call time",  0,        ""},
    {LIC_INBOUND_CALLTIME,     "IN CALL TIME",    "incoming call time",  0,        ""},
    {LIC_AUTO_CALLTIME,        "AUTO CALL TIME",  "auto call time",      0,        ""}
#endif

};

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
    return DOS_PROCESS_VERSION;
}

/*
 *  函数：U32 dos_get_build_datetime(U32 *pulBuildData, U32 *pulBuildTime)
 *  功能：获取编译时间
 *  参数：
 *      U32 *pulBuildData: 存储编译日期
 *      U32 pulBuildTime: 存储编译时间
 *  返回值:
 *      如果获取便是时间OK，返回DOS_SUCC，否则返回DOS_FAIL
 */
U32 dos_get_build_datetime(U32 *pulBuildData, U32 *pulBuildTime)
{
    S32 lYear, lMonth, lDay, lHour, lMinute, lSecond;
    S32 lRet;

    if (DOS_ADDR_INVALID(pulBuildData)
        && DOS_ADDR_INVALID(pulBuildTime))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    lRet = dos_sscanf(BUILD_TIME, "%d-%d-%d %d:%d:%d"
                , &lYear, &lMonth, &lDay
                , &lHour, &lMinute, &lSecond);
    if (lRet != 6)
    {
        DOS_ASSERT(0);

        *pulBuildTime = U32_BUTT;
        return DOS_FAIL;
    }


    *pulBuildData = 0;
    *pulBuildData = (lYear << 16) | (lMonth << 8) | lDay;
    *pulBuildTime = 0;
    *pulBuildTime = (lYear << 16) | (lMonth << 8) | lDay;

    return DOS_SUCC;
}

/*
 *  函数：S8* dos_get_build_datetime_str()
 *  功能：获取字符串格式编译时间,
 *  参数：
 *  返回值:
 *      返回编译时间
 */
S8* dos_get_build_datetime_str()
{
    return BUILD_TIME;
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

/**
 * 函数：S8 *dos_get_filename(S8* path)
 * 功能：含有路径的文件名整理成不含路径的文件名
 * 参数：
 *      S8 *pszName : 启动时的进程名（可能会带上全路径）
 * 返回值
 *      成功返文件名?，失败返回NULL
 */
DLLEXPORT S8 *dos_get_filename(const S8* path)
{
    const S8 *pSprit = NULL;
    const S8 *pNext = NULL;

    if (!path || '\0' == path)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /* 可能带了路径，我们只需要最后一段 */
    pSprit = path;
    pNext = dos_strchr(pSprit, '/');
    if (!pNext)
    {
        return (S8 *)path;
    }

    while(pNext)
    {
        pSprit = pNext + 1;
        if ('\0' == pSprit[0])
        {
            break;
        }

        pNext = dos_strchr(pSprit, '/');
    }

    if ('\0' == pSprit[0])
    {
        return NULL;
    }
    else
    {
        return (char *)pSprit;
    }
}

U32 dos_get_check_mod(U32 ulIndex)
{
    U32 i;

    for (i=0; i<sizeof(g_stModList)/sizeof(MOD_DESC_ST); i++)
    {
        if (g_stModList[i].ulIndex == ulIndex)
        {
            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}

U32 dos_get_max_mod_id()
{
    U32 ulMax = 0, i;

    for (i=0; i<sizeof(g_stModList)/sizeof(MOD_DESC_ST); i++)
    {
        if (g_stModList[i].ulIndex > ulMax)
        {
            ulMax = g_stModList[i].ulIndex;
        }
    }

    return ulMax;
}

U32 dos_get_mod_mask(U32 ulIndex, S8 *pszModuleName, S32 *plLength)
{
    U32 i;

    if (DOS_ADDR_INVALID(pszModuleName) || *plLength <= 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    for (i=0; i<sizeof(g_stModList)/sizeof(MOD_DESC_ST); i++)
    {
        if (g_stModList[i].ulIndex == ulIndex)
        {
            dos_strncpy(pszModuleName, g_stModList[i].pzsMask, *plLength);
            pszModuleName[*plLength - 1] = '\0';
            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}

U32 dos_get_product(S8 *pszProduct, S32 *plLength)
{
    if (DOS_ADDR_INVALID(pszProduct) || *plLength <= 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_strncpy(pszProduct, DOS_PROCESS_NAME, *plLength);
    pszProduct[*plLength - 1] = '\0';

    return DOS_SUCC;
}

U32 dos_get_default_limitation(U32 ulModIndex, U32 * pulLimitation)
{
    U32 i;

    if (DOS_ADDR_INVALID(pulLimitation))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    for (i=0; i<sizeof(g_stModList)/sizeof(MOD_DESC_ST); i++)
    {
        if (g_stModList[i].ulIndex == ulModIndex)
        {
            *pulLimitation = g_stModList[i].ulDefaultLimit;
            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */
