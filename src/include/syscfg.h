/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  syscfg.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 配置sdk中模块开关宏
 *     History:
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifdef DIPCC_PTC
#define DB_MYSQL 0
#else
#define DB_MYSQL 1
#endif

#include <env.h>

#ifdef DIPCC_CTRL_PANEL

#   include <cfg/cfg_mod_ctrl_panel.h>
#   define DOS_PROCESS_NAME    "CTRL-PANEL"
#   define DOS_PROCESS_VERSION "1.100.0.0"

#elif defined(DIPCC_MONITER)

#   include <cfg/cfg_mod_moniter.h>
#   define DOS_PROCESS_NAME    "MONITORD"
#   define DOS_PROCESS_VERSION "1.101.0.0"

#elif defined(DIPCC_PTS)

#   include "cfg/cfg_mod_pts.h"
#   define DOS_PROCESS_NAME    "PTS"
#   define DOS_PROCESS_VERSION "1.1.0.7"

#elif defined(DIPCC_PTC)

#   include "cfg/cfg_mod_ptc.h"
#   define DOS_PROCESS_NAME    "PTC"
#   define DOS_PROCESS_VERSION "1.3.0.7"
#elif defined(DIPCC_BS)

#   include "cfg/cfg_mod_bs.h"
#   define DOS_PROCESS_NAME    "BS"
#   define DOS_PROCESS_VERSION "1.6.0.0"

#elif defined(DIPCC_FREESWITCH)

#   include <cfg/cfg_mod_fs_core.h>
#   define DOS_PROCESS_NAME    "SC"
#   define DOS_PROCESS_VERSION "1.7.0.0"

#elif defined(DIPCC_LICENSE_SERVER)
#   include <cfg/cfg_mod_lics.h>
#   define DOS_PROCESS_NAME    "LICS"
#   define DOS_PROCESS_VERSION "1.102.0.0"
#else
#   error "Please special the mod's macro!"
#endif



/* 定义进程相关信息获取函数 */
S32 dos_set_process_name(S8 *pszName);

S8 *dos_get_pid_file_path(S8 *pszBuff, S32 lMaxLen);
S8 *dos_get_sys_root_path();
S8 *dos_get_process_name();
S8 *dos_get_process_version();
U32 dos_get_build_datetime(U32 *pulBuildData, U32 *pulBuildTime);
S8* dos_get_build_datetime_str();
DLLEXPORT S8 *dos_get_filename(const S8* path);

U32 dos_get_product(S8 *pszProduct, S32 *plLength);
U32 dos_get_mod_mask(U32 ulIndex, S8 *pszModuleName, S32 *plLength);
U32 dos_get_check_mod(U32 ulIndex);
U32 dos_get_max_mod_id();

#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* END __CONFIG_H__ */

