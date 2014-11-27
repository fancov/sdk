/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  sys_def.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 配置Dos相关参数
 *     History:
 */

#ifndef __DOSDEF_H__
#define __DOSDEF_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* 默认最大循环次数 10485759次 */
#define DOS_DEFAULT_MAX_LOOP       0x9FFFFFL

/* 初始化一个循环变量 */
#define DOS_LOOP_DETECT_INIT(__var, __limit)  \
    long __##__var = __limit;

/* 检测循环 */
#define DOS_LOOP_DETECT(__var)        \
(__##__var)--;                        \
if ((__##__var) <= 0)                 \
{                                     \
    logr_error("Endless loop detected. File:%s, Line:%d.", dos_get_filename(__FILE__), __LINE__); \
    break;                            \
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DOSDEF_H__ */

