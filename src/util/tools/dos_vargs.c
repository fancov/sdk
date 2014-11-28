/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_vargs.c
 *
 *  Created on: 2014-11-27
 *      Author: Larry
 *        Todo: 重写系统的可变参数函数提高安全性
 *     History:
 */
 
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include dos header file */
#include <dos.h>

/* include dos system file */

/* include private header file */

/* define local marco */

/* extern parameters and function */

/* local parameters and function */

/**
 * Name:S32 dos_sscanf(const S8 *pszSrc, const S8 * pszFormat, ...)
 * Func:rewrite the sscanf
 */
S32 dos_sscanf(const S8 *pszSrc, const S8 * pszFormat, ...)
{

    va_list argptr;
    S32 lRet = 0;

    if (DOS_ADDR_INVALID(pszSrc) || DOS_ADDR_INVALID(pszFormat))
    {
        DOS_ASSERT(0);
        return -1;
    }

    va_start (argptr, pszFormat);
    lRet = vsscanf(pszSrc, pszFormat, argptr);
    va_end (argptr);

    return lRet;
}

/**
 * Name:S32 dos_fscanf(FILE *pStream, const S8 * pszFormat, ...)
 * Function:rewite the fscanf function
 */
S32 dos_fscanf(FILE *pStream, const S8 * pszFormat, ...)
{
    va_list argptr;
    S32 lRet = 0;

    if (DOS_ADDR_INVALID(pStream) || DOS_ADDR_INVALID(pszFormat))
    {
        DOS_ASSERT(0);
        return -1;
    }

    va_start (argptr, pszFormat);
    lRet = vfscanf(pStream, pszFormat, argptr);
    va_end (argptr);

    return lRet;

}

/**
 * Name:S32 dos_snprintf(S8 *pszDstBuff, S32 lBuffLength, const S8 *pszFormat, ...)
 * Function:rewite the dos_snprintf function
 */
S32 dos_snprintf(S8 *pszDstBuff, S32 lBuffLength, const S8 *pszFormat, ...)
{
    va_list argptr;
    S32 lRet = 0;

    if (DOS_ADDR_INVALID(pszDstBuff) 
        || DOS_ADDR_INVALID(pszFormat)
        || lBuffLength <= 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    va_start (argptr, pszFormat);
    lRet = vsnprintf(pszDstBuff, lBuffLength, pszFormat, argptr);
    va_end (argptr);

    return lRet;

}

/**
 * Name:S32 dos_fprintf(FILE *pStream, const S8 *pszFormat, ...)
 * Function:rewite the dos_fprintf function
 */
S32 dos_fprintf(FILE *pStream, const S8 *pszFormat, ...)
{
    va_list argptr;
    S32 lRet = 0;

    if (DOS_ADDR_INVALID(pStream) 
        || DOS_ADDR_INVALID(pszFormat))
    {
        DOS_ASSERT(0);
        return -1;
    }

    va_start (argptr, pszFormat);
    lRet = vfprintf(pStream, pszFormat, argptr);
    va_end (argptr);

    return lRet;

}

#ifdef __cplusplus
}
#endif /* __cplusplus */


