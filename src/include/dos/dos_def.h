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

/* 定义最大定时器个数 */
#define TIMER_MAX_NUMBER 1024

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* 定义进程相关信息 */
S8 *dos_get_process_name();
S8 *dos_get_process_version();
S8 *dos_get_pid_file_path(S8 *pszBuff, S32 lMaxLen);
S8 *dos_get_sys_root_path();
S32 dos_set_process_name(S8 *pszName);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
