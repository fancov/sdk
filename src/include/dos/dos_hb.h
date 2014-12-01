/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_hb.h
 *
 *  Created on: 2014-11-11
 *      Author: Larry
 *        Todo: 心跳模块提供的公共头文件
 *     History:
 */

#ifndef __DOS_HB_H__
#define __DOS_HB_H__


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#if INCLUDE_BH_SERVER
S32 heartbeat_init();
S32 heartbeat_start();
S32 heartbeat_stop();
#endif
#if INCLUDE_BH_CLIENT
S32 hb_client_init();
S32 hb_client_start();
S32 hb_client_stop();
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
