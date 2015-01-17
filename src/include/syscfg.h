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


#ifdef DIPCC_CTRL_PANEL

#   include <cfg/cfg_mod_ctrl_panel.h>
#   define DOS_PROCESS_VERSION "1.0"

#elif defined(DIPCC_MONITER)

#   include <cfg/cfg_mod_moniter.h>
#   define DOS_PROCESS_VERSION "1.0"

#elif defined(DIPCC_PTS)

#   include "cfg/cfg_mod_pts.h"
#   define DOS_PROCESS_VERSION "1.0"

#elif defined(DIPCC_PTC)

#   include "cfg/cfg_mod_ptc.h"
#   define DOS_PROCESS_VERSION "1.0"
#elif defined(DIPCC_BS)

#   include "cfg/cfg_mod_bs.h"
#   define DOS_PROCESS_VERSION "1.0"

#else
#   error "Please special the mod's macro!"
#endif

/* 定义进程相关信息获取函数 */
S8 *dos_get_process_name();
S8 *dos_get_process_version();
S8 *dos_get_pid_file_path(S8 *pszBuff, S32 lMaxLen);
S8 *dos_get_sys_root_path();
S32 dos_set_process_name(S8 *pszName);
const S8 *dos_get_filename(const S8* path);


#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* END __CONFIG_H__ */

