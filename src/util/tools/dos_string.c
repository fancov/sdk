/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_string.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 定义常用字符串操作函数
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>


/**
 * 函数：S8* dos_strcpy(S8 *dst, const S8 *src)
 * 功能：将src字符串copy到dts所指向的buf中
 * 参数：
 */
DLLEXPORT S8* dos_strcpy(S8 *dst, const S8 *src)
{
    if (DOS_ADDR_INVALID(dst) || DOS_ADDR_INVALID(src))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    return strcpy(dst, src);
}


/**
 * 函数：S8* dos_strcpy(S8 *dst, const S8 *src)
 * 功能：将src字符串copy到dts所指向的buf中，最多copylmaxlen个字符
 * 参数：
 */
DLLEXPORT S8* dos_strncpy(S8 *dst, const S8 *src, S32 lMaxLen)
{
    if (DOS_ADDR_INVALID(dst) || DOS_ADDR_INVALID(src))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    return strncpy(dst, src, lMaxLen);
}


/**
 * 函数：U32 dos_strlen(const S8 *str)
 * 功能：计算字符串长度
 * 参数：
 *  失败返回0，成功返回字符串长度
 */
DLLEXPORT U32 dos_strlen(const S8 *str)
{
    U32 ulCnt;

    if (DOS_ADDR_VALID(str))
    {
        ulCnt = 0;
        while (*str)
        {
            ulCnt++;
            str++;
        }

        return ulCnt;

    }

    return 0;
}


/**
 * 函数：S8 *dos_strcat(S8 *dest, const S8 *src)
 * 功能：字符串拼接功能
 * 参数：
 *      S8 *dest： 目的字符串
 *      const S8 *src：源字符串串
 *  失败返回null，成功返回目的字符串的手地址
 */
DLLEXPORT S8 *dos_strcat(S8 *dest, const S8 *src)
{
    if (DOS_ADDR_VALID(dest) && DOS_ADDR_VALID(src))
    {
        return strcat(dest, src);
    }

    return NULL;
}


/**
 * 函数：S8 *dos_strchr(const S8 *str, S32 i)
 * 功能：在str中查找字符查找字符i
 * 参数：
 *      const S8 *str：源字符串
 *      S32 i：字符
 * 返回值：
 *      成功返回字符i相对str首地址偏移地址，失败返回null
 */
DLLEXPORT S8 *dos_strchr(const S8 *str, S32 i)
{
    if (i >= U8_BUTT)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    if (DOS_ADDR_VALID(str))
    {
        return strchr(str, i);
    }

    return NULL;
}


/**
 * 函数：S8* dos_strstr(S8 * source,  S8 *key)
 * 功能：在字符串srouce中查找字符串key
 * 参数：
 *      S8 * source：源字符串
 *      S8 *key：关键字
 * 返回值：如果没有找到，返回null，如果找到返回key相对source首地址的偏移
 */
DLLEXPORT S8* dos_strstr(S8 * source, S8 *key)
{
       char *pCh = key;
       char *pSch = NULL, *pSource = NULL;

       if (DOS_ADDR_INVALID(source) || DOS_ADDR_INVALID(key))
       {
           DOS_ASSERT(0);
           return NULL;
       }

       pSource = source;
       do
       {
           pSch = dos_strchr(pSource, *pCh);
           if (pSch)
           {
                if (dos_strncmp(key, pSch, dos_strlen(key)) == 0)
                {
                    break;
                }
                else
                {
                    pSource = pSch+1;
                }
           }
       }while (pSch);

       return pSch;
}


/**
 * 函数：S32 dos_strcmp (const S8 *s1, const S8 *s2)
 * 功能：字符串比较
 * 参数：
 * 返回值：
 */
DLLEXPORT S32 dos_strcmp(const S8 *s1, const S8 *s2)
{

    if (DOS_ADDR_VALID(s1) && DOS_ADDR_VALID(s2))
    {
        return strcmp(s1, s2);
    }
    else
    {
        DOS_ASSERT(0);
    }

    return -1;/*not equal*/
}


/**
 * 函数：S32 dos_stricmp (const S8 *s1, const S8 *s2)
 * 功能：字符串比较,忽略字符的大小写
 * 参数：
 * 返回值：
 */
DLLEXPORT S32 dos_stricmp(const S8 *s1, const S8 *s2)
{
    S8 cCh1, cCh2;

    if (DOS_ADDR_VALID(s1) && DOS_ADDR_VALID(s2))
    {
        if (s1 == s2)
        {
            return 0;
        }

        for (;;)
        {
            cCh1 = *s1;
            cCh2 = *s2;

            if (cCh1 >= 'A' && cCh1 <= 'Z')
            {
                 cCh1 += 'a' - 'A';
            }
            if (cCh2 >= 'A' && cCh2 <= 'Z')
            {
                 cCh2 += 'a' - 'A';
            }

            if (cCh1 != cCh2)
            {
                 return (S32)(cCh1 - cCh2);
            }
            if (!cCh1)
            {
                 return 0;
            }

            s1++;
            s2++;
        }
    }
    else
    {
        DOS_ASSERT(0);
    }

    return -1;/*not equal*/
}


/**
 * 函数：S32 dos_strncmp (const S8 *s1, const S8 *s2, U32 n)
 * 功能：比较s1和s2中前面n个字符
 * 参数：
 * 返回值：
 */
DLLEXPORT S32 dos_strncmp(const S8 *s1, const S8 *s2, U32 n)
{
    if (DOS_ADDR_VALID(s1) && DOS_ADDR_VALID(s2))
    {
        return strncmp(s1, s2, n);
    }

    return -1;/*not equal*/
}

DLLEXPORT S32 dos_strlastindexof(const S8 *s, const S8 ch, U32 n)
{
    U32 i, len;

    if (DOS_ADDR_INVALID(s))
    {
        return -1;
    }

    i = n;
    len = dos_strlen(s);
    if (len < n)
    {
        i = len;
    }

    for (; i>=0; i--)
    {
        if (ch == s[i])
        {
            return i;
        }
    }

    return -1;
}


/**
 * 函数：S32 dos_strnicmp(const S8 *s1, const S8 *s2, U32 n)
 * 功能：比较s1和s2中前面n个字符，忽略大小写
 * 参数：
 * 返回值：
 */
DLLEXPORT S32 dos_strnicmp(const S8 *s1, const S8 *s2, U32 n)
{
    S8  cCh1, cCh2;

    if (DOS_ADDR_VALID(s1) && DOS_ADDR_VALID(s2))
    {
        if ((s1 == s2) || (n == 0))
        {
            return 0;
        }

        while (n)
        {
            cCh1 = *s1;
            cCh2 = *s2;

            if (cCh1 >= 'A' && cCh1 <= 'Z')
            {
                cCh1 += 'a' - 'A';
            }

            if (cCh2 >= 'A' && cCh2 <= 'Z')
            {
                cCh2 += 'a' - 'A';
            }
            if (cCh1 != cCh2)
            {
                return (S32)(cCh1 - cCh2);
            }
            if (!cCh1)
            {
                return 0;
            }

            s1++;
            s2++;
            n--;
        }

        return 0;
    }

    return -1;
}


/**
 * 函数：S32 dos_strnlen(const S8 * str, S32 count)
 * 功能：计算字符串长度，最多计算count个字节
 * 参数：
 * 返回值：
 */
DLLEXPORT S32 dos_strnlen(const S8 *s, S32 count)
{
    const S8 *sc;

    if(DOS_ADDR_VALID(s))
    {
        for (sc = s; count-- && *sc != '\0'; ++sc)
        /* nothing */;

        return (S32)(sc - s);
    }

   return 0;
}


/**
 * 函数：S32 dos_strndup(const S8 * str, S32 count)
 * 功能：dump s所指向的字符串，最多count个字节
 * 参数：
 * 返回值：
 */
DLLEXPORT S8 *dos_strndup(const S8 *s, S32 count)
{
    S8 *pStr;

    pStr = (S8 *)dos_dmem_alloc(count);
    if (!pStr)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    if (DOS_ADDR_VALID(s))
    {
        dos_strncpy(pStr, s, count);
        pStr[count - 1] ='\0';
    }

    return pStr;
}

/**
 * 函数：S8 dos_toupper(S8 ch)
 * 功能：将字符转换成大写字母
 * 参数：
 * 返回值：
 */
DLLEXPORT S8 dos_toupper(S8 ch)
{
    if (ch >= 'a' && ch <= 'z')
    {
        return (S8)(ch + 'A' - 'a');
    }

    return ch;
}


/**
 * 函数：S8 dos_tolower(S8 ch)
 * 功能：将字符转换成小写字母
 * 参数：
 * 返回值：
 */
DLLEXPORT S8 dos_tolower(S8 ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return (S8)(ch + 'a' - 'A');
    }

    return ch;
}


/**
 * 函数：VOID dos_uppercase (S8 *str)
 * 功能：将字符串中所有的小写字符转换成大写
 * 参数：
 * 返回值：
 */
DLLEXPORT VOID dos_uppercase(S8 *str)
{
    if (DOS_ADDR_INVALID(str))
    {
        DOS_ASSERT(0);
        return;
    }

    while (*str)
    {
        if (*str >= 'a' && *str <= 'z')
        {
            *str = (S8)((*str) + 'A' - 'a');
        }
        str++;
    }
}


/**
 * 函数：VOID dos_lowercase (S8 *str)
 * 功能：将字符串中所有的大写字符转换成小写
 * 参数：
 * 返回值：
 */
DLLEXPORT VOID dos_lowercase(S8 *str)
{
    if (DOS_ADDR_INVALID(str))
    {
        DOS_ASSERT(0);
        return;
    }

    while (*str)
    {
        if (*str >= 'A' && *str <= 'Z')
        {
            *str = (S8)((*str) + 'a' - 'A');
        }
        str++;
    }
}


/**
 *函数: S32 dos_atouc(const S8 *szStr, U8 *pnVal)
 *功能:将字符串转换成8位无符号数字
 *参数:
 *返回值:成功返回0，失败返回-1
 */
DLLEXPORT S32 dos_atouc(const S8 *szStr, U8 *pucVal)
{
    S32  lTmp;
    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pucVal))
    {
        if (dos_sscanf(szStr, "%d", &lTmp) < 1)
        {
            *pucVal = 0;
            return -1;
        }

        *pucVal = lTmp;
        return 0;
    }
    return -1;
}



/**
 * 函数：S32 dos_atol(const S8 *szStr, S32 *pnVal)
 * 功能：将字符串转换成32位有符号数字
 * 参数：
 * 返回值：成功返回0，失败返回－1
 */
DLLEXPORT S32 dos_atol(const S8 *szStr, S32 *pnVal)
{
    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pnVal))
    {
        if (dos_sscanf(szStr, "%d", pnVal) < 1)
        {
            *pnVal = 0;
            return -1;
        }
        return 0;
    }
    return -1;
}


/**
 * 函数：S32 dos_atoul (const S8 *szStr, U32 *pulVal)
 * 功能：将字符串转换成32位无符号数字
 * 参数：
 * 返回值：成功返回－0，失败返回－1
 */
DLLEXPORT S32 dos_atoul(const S8 *szStr, U32 *pulVal)
{
    U32 nVal;

    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pulVal))
    {
        if (dos_sscanf(szStr, "%u", &nVal) < 1)
        {
            *pulVal = 0;
            return -1;
        }

        *pulVal = nVal;
        return 0;
    }

    return -1;
}

/**
 * 函数：S32 dos_atoull(const S8 *szStr, U64 *pulVal)
 * 功能：将字符串转换成32位无符号数字
 * 参数：
 * 返回值：成功返回－0，失败返回－1
 */
DLLEXPORT S32 dos_atoull(const S8 *szStr, U64 *pulVal)
{
    U64 nVal;

    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pulVal))
    {
        if (dos_sscanf(szStr, "%lu", &nVal) < 1)
        {
            *pulVal = 0;
            return -1;
        }

        *pulVal = nVal;
        return 0;
    }

    return -1;
}

/**
 * 函数：S32 dos_atoll(const S8 *szStr, S64 *pulVal)
 * 功能：将字符串转换成32位无符号数字
 * 参数：
 * 返回值：成功返回－0，失败返回－1
 */
DLLEXPORT S32 dos_atoll(const S8 *szStr, S64 *pulVal)
{
    S64 nVal;

    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pulVal))
    {
        if (dos_sscanf(szStr, "%ld", &nVal) < 1)
        {
            *pulVal = 0;
            return -1;
        }

        *pulVal = nVal;
        return 0;
    }

    return -1;
}


/**
 * 函数：S32 dos_atolx (const S8 *szStr, S32 *pnVal)
 * 功能：将16进制格式的数字字符串转换成32位无符号数字
 * 参数：
 * 返回值：成功返回－0，失败返回－1
 */
DLLEXPORT S32 dos_atolx (const S8 *szStr, S32 *pnVal)
{
    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pnVal))
    {
        if (dos_sscanf(szStr, "%x", pnVal) < 1 &&
            dos_sscanf(szStr, "%X", pnVal) < 1)
        {
            *pnVal = 0;
            return -1;
        }

        return 0;
    }

    return -1;
}


/**
 * 函数：S32 dos_atoulx(const S8 *szStr, U32 *pulVal)
 * 功能：将16进制格式的数字字符串转换成32位无符号数字
 * 参数：
 * 返回值：成功返回－0，失败返回－1
 */
DLLEXPORT S32 dos_atoulx(const S8 *szStr, U32 *pulVal)
{
    U32 nVal;

    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pulVal))
    {
        if (dos_sscanf(szStr, "%x", &nVal) < 1 &&
            dos_sscanf(szStr, "%X", &nVal) < 1)
        {
            *pulVal = 0;
            return -1;
        }

        *pulVal = nVal;
        return 0;
    }

    return -1;
}


/**
 * 函数：S32 dos_ltoa(S32 nVal, S8 *szStr)
 * 功能：将数字转换成字符串
 * 参数：
 * 返回值：成功返回－0，失败返回－1
 */
DLLEXPORT S32 dos_ltoa(S32 nVal, S8 *szStr, U32 ulStrLen)
{
    if (DOS_ADDR_VALID(szStr) && ulStrLen > 0)
    {
        if (dos_snprintf(szStr, ulStrLen, "%d", nVal) < 1)
        {
            return -1;
        }
        return 0;
    }
    return -1;
}


/**
 * 函数：S32 dos_ltoax(S32 nVal, S8 *szStr)
 * 功能：将32位有符号整数转换成16进制的格式的字符串
 * 参数：
 * 返回值：成功返回－0，失败返回－1
 */
DLLEXPORT S32 dos_ltoax(S32 nVal, S8 *szStr, U32 ulStrLen)
{
    if (DOS_ADDR_VALID(szStr))
    {
        if (dos_snprintf(szStr, ulStrLen, "%x", nVal) < 1)
        {
            return -1;
        }
        return 0;
    }

    return -1;
}


/**
 * 函数：S32 dos_ultoax (U32 ulVal, S8 *szStr)
 * 功能：将32位无符号整数转换成16进制的格式的字符串
 * 参数：
 * 返回值：成功返回－0，失败返回－1
 */
DLLEXPORT S32 dos_ultoax (U32 ulVal, S8 *szStr, U32 ulStrLen)
{
    if (DOS_ADDR_VALID(szStr))
    {
        if (dos_snprintf(szStr, ulStrLen, "%x", ulVal) < 1)
        {
            return -1;
        }

        return 0;
    }

    return -1;
}


/**
 * 函数：S8  *dos_ipaddrtostr(U32 ulAddr, S8 *str)
 * 功能：将ulAddr所表示的ip地址转化成点分格式的IP地址
 * 参数：
 * 返回值：成功返回STR首地址，失败返回null
 */
DLLEXPORT S8  *dos_ipaddrtostr(U32 ulAddr, S8 *str, U32 ulStrLen)
{
    if (DOS_ADDR_VALID(str))
    {
        dos_snprintf(str, ulStrLen, "%u.%u.%u.%u"
                        , ulAddr & 0xff
                        , (ulAddr >> 8) & 0xff
                        , (ulAddr >> 16) & 0xff
                        , ulAddr >> 24);

        return str;
    }

    return NULL;
}


/**
 * 函数：S32 dos_is_digit_str(S8 *str)
 * 功能：判断字符串是否是数字字符串
 * 参数：
 * 返回值：成功返回0，失败返回-1
 */
DLLEXPORT S32 dos_is_digit_str(S8 *str)
{
    S32 i = 0;

    if (DOS_ADDR_VALID(str))
    {
        while ('\0' != str[i])
        {
            if (str[i] < '0' || str[i] > '9')
            {
                return -1;
            }

            i++;
        }

        return 0;
    }

    return -1;
}

/**
 * 函数：S32 dos_is_printable_str(S8* str)
 * 功能：判断字符串是否为可打印字符串
 *       如果字符串里面有一个可打印字符(空格除外)，则认为是可打印字符串
 * 参数：
 * 返回值：成功返回0，失败返回-1
 */
DLLEXPORT S32 dos_is_printable_str(S8* str)
{
    S8 *pszPtr = str;
    S32 lFlag = -1;

    while ('\0' != *pszPtr)
    {
        if (*pszPtr >= '!')
        {
            lFlag = 0;
            break;
        }
        ++pszPtr;
    }

    return lFlag;
}

/**
 * 函数：S32 dos_strtoipaddr(S8 *szStr, U32 *pulIpAddr)
 * 功能：将点分格式的字符串IP地址转化为字节整数格式的IP地址
 * 参数：
 * 返回值：成功返回STR首地址，失败返回null
 */
DLLEXPORT S32 dos_strtoipaddr(S8 *szStr, U32 *pulIpAddr)
{
    U32 a, b, c, d;

    if (DOS_ADDR_VALID(pulIpAddr))
    {
        if (dos_sscanf(szStr, "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
        {
            if (a<256 && b<256 && c<256 && d<256)
            {
                *pulIpAddr = (a<<24) | (b<<16) | (c<<8) | d;
                return 0;
            }
        }
    }

    return -1;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
