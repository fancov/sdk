/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  env.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 配置sdk中模块开关宏
 *     History:
 */

#ifndef __ENV_H__
#define __ENV_H__

#if (__unix__ || __linux || __linux__ || __gnu_linux__ || unix || linux)

#define OS_LINUX    1
#define OS_WIN      0

#else

#define OS_LINUX    0
#define OS_WIN      1

#endif

#if OS_WIN

#if (_M_IX86)
#   define  OS_32                        1
#   define  OS_64                        0
#   define  POINTER_WIDTH_IN_BIT         32
#   define  POINTER_WIDTH_IN_BYTE        4
#elif (_WIN64 || _M_X64 || _M_IA64 || _M_AMD64)
#   define  OS_32                        0
#   define  OS_64                        1
#   define  POINTER_WIDTH_IN_BIT         64
#   define  POINTER_WIDTH_IN_BYTE        8
#else
#   error "Unknow OS type. A x86-64 host is recommended."
#endif

#elif OS_LINUX

#if (bswap_32)
#   define  OS_32                        1
#   define  OS_64                        0
#   define  POINTER_WIDTH_IN_BIT         32
#   define  POINTER_WIDTH_IN_BYTE        4
#elif (__amd64__ || __amd64 || __x86_64__ || __x86_64 || _LP64)
#   define  OS_32                        0
#   define  OS_64                        1
#   define  POINTER_WIDTH_IN_BIT         64
#   define  POINTER_WIDTH_IN_BYTE        8
#else
//#   error "Unknow OS type. A x86-64 host is recommended."
#   define  OS_32                        1
#   define  OS_64                        0
#   define  POINTER_WIDTH_IN_BIT         32
#   define  POINTER_WIDTH_IN_BYTE        4

#endif

#else

#endif

#endif /* END __ENV_H__ */

