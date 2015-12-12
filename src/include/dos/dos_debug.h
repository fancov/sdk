/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_debug.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: DOS调试模块提供的公共函数申明
 *     History:
 */

#ifndef __DOS_DEBUG_H__
#define __DOS_DEBUG_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#define dos_printf(format, args...)  dos_vprintf(dos_get_filename(__FILE__), __LINE__, (format), ##args)

VOID dos_vprintf(const S8 *pszFile, S32 lLine, const S8 *pszFormat, ...);
VOID dos_syslog(S32 lLevel, const S8 *pszLog);

#if INCLUDE_EXCEPTION_CATCH
extern VOID dos_signal_handle_reg();
#endif

S32 dos_assert_init();
VOID dos_assert(const S8 *pszFileName, const U32 ulLine, const U32 param);
S32 dos_assert_print(U32 ulIndex, S32 argc, S8 **argv);
S32 dos_assert_record();


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

