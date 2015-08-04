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

#if (INCLUDE_BH_SERVER || INCLUDE_BH_CLIENT)





#define hb_logr_debug(_pszFormat, args...) \
        hb_logr_write(LOG_LEVEL_DEBUG, (_pszFormat), ##args)

#define hb_logr_info(_pszFormat, args...) \
        hb_logr_write(LOG_LEVEL_INFO, (_pszFormat), ##args)

#define hb_logr_notice(_pszFormat, args...) \
        hb_logr_write(LOG_LEVEL_NOTIC, (_pszFormat), ##args)

#define hb_logr_warning(_pszFormat, args...) \
        hb_logr_write(LOG_LEVEL_WARNING, (_pszFormat), ##args)

#define hb_logr_error(_pszFormat, args...) \
        hb_logr_write(LOG_LEVEL_ERROR, (_pszFormat), ##args)

#define hb_logr_cirt(_pszFormat, args...) \
        hb_logr_write(LOG_LEVEL_CIRT, (_pszFormat), ##args)

#define hb_logr_alert(_pszFormat, args...) \
        hb_logr_write(LOG_LEVEL_ALERT, (_pszFormat), ##args)

#define hb_logr_emerg(_pszFormat, args...) \
        hb_logr_write(LOG_LEVEL_EMERG, (_pszFormat), ##args)

VOID hb_logr_write(U32 ulLevel, S8 *pszFormat, ...);
VOID hb_log_set_level(U32 ulLevel);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
