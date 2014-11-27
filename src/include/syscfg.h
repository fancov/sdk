/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  config.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 配置sdk中模块开关宏
 *     History:
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef DIPCC_CTRL_PANEL

#	include <cfg/cfg_mod_ctrl_panel.h>
#	define DOS_PROCESS_VERSION "1.0"

#elif defined(DIPCC_MONITER)

#	include <cfg/cfg_mod_moniter.h>
#	define DOS_PROCESS_VERSION "1.0"

#elif defined(DIPCC_PTS)

#   include "cfg/cfg_mod_pts.h"
#   define DOS_PROCESS_VERSION "1.0"

#elif defined(DIPCC_PTC)

#   include "cfg/cfg_mod_ptc.h"
#   define DOS_PROCESS_VERSION "1.0"

#else
#	error "Please special the mod's macro!"
#endif


#endif /* END __CONFIG_H__ */

