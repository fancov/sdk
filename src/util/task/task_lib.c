/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  task_lib.c
 *
 *  Created on: 2014-11-6
 *      Author: Larry
 *        Todo: 封装系统相关的函数，提高可一致性性
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

S8 *dos_get_localtime(U32 ulTimestamp, S8 *pszBuffer, S32 lLength)
{
    S32 MON1[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};    /* 平年 */
    S32 MON2[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};    /* 闰年 */
    static const S32 FOURYEARS = (366 + 365 +365 +365);                 /* 每个四年的总天数 */
    static const S32 DAYMS = 24*3600;                                   /* 每天的秒数 */
    S32 lDaysTotal = 0;                                                 /* 总天数 */
    S32 lYear4 = 0;                                                     /* 得到从1970年以来的周期（4年）的次数 */
    S32 lDayRemain = 0;                                                 /* 得到不足一个周期的天数 */
    S32 lSecondRemain = 0;                                              /* 不足一天剩余的秒数 */
    S32 lYear = 0, lMonth = 0, lDays = 0, lHour = 0, lMin = 0, lSec = 0;
    BOOL bLeapYear = DOS_FALSE;
    S32 *pMonths = NULL;
    S32 i = 0;
    S32 lTemp = 0;

    if (DOS_ADDR_INVALID(pszBuffer) || lLength < TIME_STR_LEN)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /* 获取北京时间 */
    ulTimestamp     += 8 * 3600;
    lDaysTotal      = ulTimestamp / DAYMS;
    lYear4          = lDaysTotal / FOURYEARS;
    lDayRemain      = lDaysTotal % FOURYEARS;
    lYear           = 1970 + lYear4 * 4;
    lSecondRemain   = ulTimestamp % DAYMS;

    if (lDayRemain < 365)                               /* 一个周期内，第一年 */
    {
        /* 平年 */
    }
    else if (lDayRemain < (365+365))                    /* 一个周期内，第二年 */
    {
        /* 平年 */
        lYear += 1;
        lDayRemain -= 365;
    }
    else if (lDayRemain < (365+365+365))                /* 一个周期内，第三年 */
    {
        /* 平年 */
        lYear += 2;
        lDayRemain -= (365+365);
    }
    else                                                /* 一个周期内，第四年，这一年是闰年 */
    {
        /* 润年 */
        lYear += 3;
        lDayRemain -= (365+365+365);
        bLeapYear = DOS_TRUE;
    }

    pMonths = bLeapYear ? MON2 : MON1;

    for (i=0; i<12; ++i)
    {
        lTemp = lDayRemain - pMonths[i];
        if (lTemp <= 0)
        {
            lMonth = i + 1;
            if (lTemp == 0) /* 表示刚好是这个月的最后一天，那么天数就是这个月的总天数了 */
            {
                lDays = pMonths[i];
            }
            else
            {
                lDays = lDayRemain;
            }
            break;
        }

        lDayRemain = lTemp;
    }

    lHour = lSecondRemain / 3600;
    lMin = (lSecondRemain % 3600) / 60;
    lSec = (lSecondRemain % 3600) % 60;

    dos_snprintf(pszBuffer, lLength, "%04d-%02d-%02d %02d:%02d:%02d", lYear, lMonth, lDays, lHour, lMin, lSec);

    return pszBuffer;
}

struct tm *dos_get_localtime_struct(U32 ulTimestamp, struct tm *pstTime)
{
    S32 MON1[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};    /* 平年 */
    S32 MON2[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};    /* 闰年 */
    static const S32 FOURYEARS = (366 + 365 +365 +365);                 /* 每个四年的总天数 */
    static const S32 DAYMS = 24*3600;                                   /* 每天的秒数 */
    S32 lDaysTotal = 0;                                                 /* 总天数 */
    S32 lYear4 = 0;                                                     /* 得到从1970年以来的周期（4年）的次数 */
    S32 lDayRemain = 0;                                                 /* 得到不足一个周期的天数 */
    S32 lSecondRemain = 0;                                              /* 不足一天剩余的秒数 */
    S32 lYear = 0, lMonth = 0, lDays = 0, lHour = 0, lMin = 0, lSec = 0;
    BOOL bLeapYear = DOS_FALSE;
    S32 *pMonths = NULL;
    S32 i = 0;
    S32 lTemp = 0;

    if (DOS_ADDR_INVALID(pstTime))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /* 获取北京时间 */
    ulTimestamp     += 8 * 3600;
    lDaysTotal      = ulTimestamp / DAYMS;
    lYear4          = lDaysTotal / FOURYEARS;
    lDayRemain      = lDaysTotal % FOURYEARS;
    lYear           = 1970 + lYear4 * 4;
    lSecondRemain   = ulTimestamp % DAYMS;

    if (lDayRemain < 365)                               /* 一个周期内，第一年 */
    {
        /* 平年 */
    }
    else if (lDayRemain < (365+365))                    /* 一个周期内，第二年 */
    {
        /* 平年 */
        lYear += 1;
        lDayRemain -= 365;
    }
    else if (lDayRemain < (365+365+365))                /* 一个周期内，第三年 */
    {
        /* 平年 */
        lYear += 2;
        lDayRemain -= (365+365);
    }
    else                                                /* 一个周期内，第四年，这一年是闰年 */
    {
        /* 润年 */
        lYear += 3;
        lDayRemain -= (365+365+365);
        bLeapYear = DOS_TRUE;
    }

    pMonths = bLeapYear ? MON2 : MON1;

    for (i=0; i<12; ++i)
    {
        lTemp = lDayRemain - pMonths[i];
        if (lTemp <= 0)
        {
            lMonth = i + 1;
            if (lTemp == 0) /* 表示刚好是这个月的最后一天，那么天数就是这个月的总天数了 */
            {
                lDays = pMonths[i];
            }
            else
            {
                lDays = lDayRemain;
            }
            break;
        }

        lDayRemain = lTemp;
    }

    if (lSecondRemain)
    {
        /* 加一天 */
        lDaysTotal++;
    }
    pstTime->tm_wday = (lDaysTotal + 3) % 7;

    lHour = lSecondRemain / 3600;
    lMin = (lSecondRemain % 3600) / 60;
    lSec = (lSecondRemain % 3600) % 60;

    pstTime->tm_year = lYear - 1900;
    pstTime->tm_mon = lMonth - 1;
    pstTime->tm_mday = lDays;
    pstTime->tm_hour = lHour;
    pstTime->tm_min = lMin;
    pstTime->tm_sec = lSec;

    return pstTime;
}

/**
 * 函数: void dos_task_delay(U32 ulMsSec)
 * 功能: 使某一个线程睡眠 ulMsSec毫秒之后再执行
 * 参数: U32 ulMSEC睡眠的毫秒数
 * 返回值: void
 */
DLLEXPORT VOID dos_task_delay(U32 ulMSec)
{
    if (0 == ulMSec)
    {
        return;
    }

    usleep(ulMSec * 1000);
}

/**
 * 函数: void dos_clean_watchdog()
 * 功能: 清除看门狗
 * 参数: NULL
 * 返回值: void
 */
DLLEXPORT void dos_clean_watchdog()
{

}

DLLEXPORT VOID dos_random_init()
{
    srand((unsigned)time( NULL ));
}

DLLEXPORT U32 dos_random(U32 ulMax)
{
    return rand() % ulMax;
}

DLLEXPORT U32 dos_random_range(U32 ulMin, U32 ulMax)
{
    U32 ulRand = 0;
    S32 i;

    if (ulMin >= ulMax)
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    i = 1000;
    while (i-- > 0)
    {
        ulRand = rand() % ulMax;

        if (ulRand >= ulMin && ulRand < ulMax)
        {
            break;
        }
    }

    if (ulRand < ulMin || ulRand >= ulMax)
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    return ulRand;
}



#ifdef __cplusplus
}
#endif /* __cplusplus */


