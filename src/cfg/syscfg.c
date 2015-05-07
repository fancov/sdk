/*
 *            (C) Copyright 2014, ÌìÌìÑ¶Í¨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  syscfg.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ½ø³ÌĞÅÏ¢Ïà¹Øº¯Êı
 *     History:
 */


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
    
#include <dos.h>

#define MAX_PROCESS_NAME_LENGTH   48

#define MAX_PATH_LENGTH           256

/* ¶¨Ê±Ä£¿éÏà¹ØĞÅÏ¢ */
/* ½ø³ÌÃû */
static S8 g_pszProcessName[MAX_PROCESS_NAME_LENGTH] = { 0 };

/* ½ø³Ì°æ±¾ */
static S8 *g_pszProcessVersion = "1.1";

/* ¶¨ÒåÄ¬ÈÏÏµÍ³¸ùÄ¿Â¼ */
static S8 *g_pszDefaultSysRootPath = "/ipcc";

/* ´æ·ÅÅäÖÃÎÄ¼şÖĞÏµÍ³¸ùÄ¿Â¼ */
static S8 g_szSysRootPath[MAX_PATH_LENGTH] = {0, };

/**
 * Function: S8 *dos_get_process_name()
 *     Todo: »ñÈ¡¸Ã½ø³ÌÃû³Æ
 *   Return: ·µ»Ø¸Ã½ø³ÌÃû³Æ
 */
S8 *dos_get_process_name()
{
    return g_pszProcessName;
}

/**
 * Function: S8 *dos_get_process_version()
 *     Todo: »ñÈ¡¸Ã½ø³Ì°æ±¾ºÅ
 *   Return: ·µ»Ø¸Ã½ø³Ì°æ±¾ºÅ
 */
S8 *dos_get_process_version()
{
    return g_pszProcessVersion;
}

/*
 *  º¯Êı£ºS8 *dos_get_pid_file_pah(S8 *pszBuff, S32 lMaxLen)
 *  ¹¦ÄÜ£º»ñÈ¡pidÎÄ¼şµÄÂ·¾¶
 *  ²ÎÊı£º
 *      S8 *pszBuff  Êı¾İ»º´æ
 *      S32 lMaxLen  »º´æµÄ³¤¶È
 *  ·µ»ØÖµ:
 *      Èç¹ûÒ»ÇĞÕı³££¬½«»á°ÑpidÎÄ¼şÂ·¾¶copyµ½»º´æ£¬²¢ÇÒ·µ»Ø»º´æµÃÊ×µØÖ·£»
 *      Èç¹ûÒì³££¬½«»Ø·µNULL, »º´æÖĞµÄÊı¾İ½«»áÊÇËæ»úÊı¾İ
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
 * º¯Êı£ºS8 *dos_get_sys_root_path()
 * ¹¦ÄÜ£º»ñÈ¡ÏµÍ³¸ùÄ¿Â¼
 * ²ÎÊı£º
 * ·µ»ØÖµ£º·µ»ØÅäÖÃÎÄ¼şÖĞµÄ¸ùÄ¿Â¼£¬»òÕßÏµÍ³Ä£¿é¸ùÄ¿Â¼
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
 * º¯Êı£ºU32 dos_set_process_name(S8 *pszName)
 * ¹¦ÄÜ£ºÉèÖÃµ±Ç°½ø³ÌµÄÃû×Ö
 * ²ÎÊı£º
 *      S8 *pszName : Æô¶¯Ê±µÄ½ø³ÌÃû£¨¿ÉÄÜ»á´øÉÏÈ«Â·¾¶£©
 * ·µ»ØÖµ
 *      ³É¹¦·µ»Ø0£¬Ê§°Ü·µ»Ø£­1
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

    /* ¿ÉÄÜ´øÁËÂ·¾¶£¬ÎÒÃÇÖ»ĞèÒª×îºóÒ»¶Î */
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

    /* ´æ´¢Ò»ÏÂ */
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
 * º¯Êı£ºS8 *dos_get_filename(S8* path)
 * ¹¦ÄÜ£ºº¬ÓĞÂ·¾¶µÄÎÄ¼şÃûÕûÀí³É²»º¬Â·¾¶µÄÎÄ¼şÃû
 * ²ÎÊı£º
 *      S8 *pszName : Æô¶¯Ê±µÄ½ø³ÌÃû£¨¿ÉÄÜ»á´øÉÏÈ«Â·¾¶£©
 * ·µ»ØÖµ
 *      ³É¹¦·µÎÄ¼şÃûØ0£¬Ê§°Ü·µ»ØNULL
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

    /* ¿ÉÄÜ´øÁËÂ·¾¶£¬ÎÒÃÇÖ»ĞèÒª×îºóÒ»¶Î */
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


#ifdef __cplusplus
}
#endif /* __cplusplus */
