
/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_log.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: log模块公共头文件
 *     History:
 */

#ifndef __LOG_PUB__
#define __LOG_PUB__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* log级别 */
enum LOG_LEVEL{
    LOG_LEVEL_EMERG = 0,
    LOG_LEVEL_ALERT,
    LOG_LEVEL_CIRT,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_NOTIC,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,

    LOG_LEVEL_INVAILD
};

/* log类型 */
enum LOG_TYPE{
    LOG_TYPE_RUNINFO = 0,
    LOG_TYPE_WARNING,
    LOG_TYPE_SERVICE,
    LOG_TYPE_OPTERATION,

    LOG_TYPE_INVAILD
};

/* log模块公共函数 */
#if INCLUDE_SYSLOG_ENABLE
S32 dos_log_init();
S32 dos_log_start();
VOID dos_log_stop();

DLLEXPORT VOID dos_log(S32 _iLevel, S32 _iType, const S8 *_pszMsg);
DLLEXPORT VOID dos_vlog(S32 _iLevel, S32 _iType, const S8 *format, ...);
DLLEXPORT VOID dos_olog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *_pszMsg);
DLLEXPORT VOID dos_volog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *format, ...);
DLLEXPORT S32 dos_log_set_cli_level(U32 ulLeval);
#else

#define dos_log_init() \
    do{ \
        printf("Init the log."); \
    }while(0)

#define dos_log_start() \
    do{ \
        printf("Start the log."); \
    }while(0)

#define dos_log_stop() \
    do{ \
        printf("Stop the log."); \
    }while(0)

#define dos_log(_iLevel, _iType, _pszMsg)  do {printf(_pszMsg); printf("\r\n");}while(0)
#define dos_vlog(_iLevel, _iType, format, args...) do {printf(format, ##args); printf("\r\n");}while(0)
#define dos_olog(_lLevel, pszOpterator, pszOpterand, ulResult, _pszMsg) do {printf(_pszMsg); printf("\r\n");}while(0)
#define dos_volog(_lLevel, pszOpterator, pszOpterand, ulResult, format, args...) do {printf(format, ##args); printf("\r\n");}while(0)

DLLEXPORT S32 dos_log_set_cli_level(U32 ulLeval);

#endif


#define logw_debug(format, args... ) dos_vlog(LOG_LEVEL_DEBUG, LOG_TYPE_WARNING, (format), ##args)
#define logw_info(format, args... ) dos_vlog(LOG_LEVEL_INFO, LOG_TYPE_WARNING, (format), ##args)
#define logw_notice(format, args... ) dos_vlog(LOG_LEVEL_NOTIC, LOG_TYPE_WARNING, (format), ##args)
#define logw_warning(format, args... ) dos_vlog(LOG_LEVEL_WARNING, LOG_TYPE_WARNING, (format), ##args)
#define logw_error(format, args... ) dos_vlog(LOG_LEVEL_ERROR, LOG_TYPE_WARNING, (format), ##args)
#define logw_cirt(format, args... ) dos_vlog(LOG_LEVEL_CIRT, LOG_TYPE_WARNING, (format), ##args)
#define logw_alert(format, args... ) dos_vlog(LOG_LEVEL_ALERT, LOG_TYPE_WARNING, (format), ##args)
#define logw_emerg(format, args... ) dos_vlog(LOG_LEVEL_EMERG, LOG_TYPE_WARNING, (format), ##args)

/* 运行调试日志 */
#define logr_debug(format, args... ) dos_vlog(LOG_LEVEL_DEBUG, LOG_TYPE_RUNINFO, (format), ##args)
#define logr_info(format, args... ) dos_vlog(LOG_LEVEL_INFO, LOG_TYPE_RUNINFO, (format), ##args)
#define logr_notice(format, args... ) dos_vlog(LOG_LEVEL_NOTIC, LOG_TYPE_RUNINFO, (format), ##args)
#define logr_warning(format, args... ) dos_vlog(LOG_LEVEL_WARNING, LOG_TYPE_RUNINFO, (format), ##args)
#define logr_error(format, args... ) dos_vlog(LOG_LEVEL_ERROR, LOG_TYPE_RUNINFO, (format), ##args)
#define logr_cirt(format, args... ) dos_vlog(LOG_LEVEL_CIRT, LOG_TYPE_RUNINFO, (format), ##args)
#define logr_alert(format, args... ) dos_vlog(LOG_LEVEL_ALERT, LOG_TYPE_RUNINFO, (format), ##args)
#define logr_emerg(format, args... ) dos_vlog(LOG_LEVEL_EMERG, LOG_TYPE_RUNINFO, (format), ##args)

/* 业务日志 */
#define logs_debug(format, args... ) dos_vlog(LOG_LEVEL_DEBUG, LOG_TYPE_SERVICE, (format), ##args)
#define logs_info(format, args... ) dos_vlog(LOG_LEVEL_INFO, LOG_TYPE_SERVICE, (format), ##args)
#define logs_notice(format, args... ) dos_vlog(LOG_LEVEL_NOTIC, LOG_TYPE_SERVICE, (format), ##args)
#define logs_warning(format, args... ) dos_vlog(LOG_LEVEL_WARNING, LOG_TYPE_SERVICE, (format), ##args)
#define logs_error(format, args... ) dos_vlog(LOG_LEVEL_ERROR, LOG_TYPE_SERVICE, (format), ##args)
#define logs_cirt(format, args... ) dos_vlog(LOG_LEVEL_CIRT, LOG_TYPE_SERVICE, (format), ##args)
#define logs_alert(format, args... ) dos_vlog(LOG_LEVEL_ALERT, LOG_TYPE_SERVICE, (format), ##args)
#define logs_emerg(format, args... ) dos_vlog(LOG_LEVEL_EMERG, LOG_TYPE_SERVICE, (format), ##args)

/* 操作日志 */
#define logo_debug(_pszOpterator, _pszOpterand, _ulResult, _format, args... ) \
        dos_volog(LOG_LEVEL_DEBUG, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), ##args)

#define logo_info(_pszOpterator, _pszOpterand, _ulResult, _format, args... ) \
        dos_volog(LOG_LEVEL_INFO, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), ##args)

#define logo_notice(_pszOpterator, _pszOpterand, _ulResult, _format, args... ) \
        dos_volog(LOG_LEVEL_NOTIC, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), ##args)

#define logo_warning(_pszOpterator, _pszOpterand, _ulResult, _format, args... ) \
        dos_volog(LOG_LEVEL_WARNING, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), ##args)

#define logo_error(_pszOpterator, _pszOpterand, _ulResult, _format, args... ) \
        dos_volog(LOG_LEVEL_ERROR, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), ##args)

#define logo_cirt(_pszOpterator, _pszOpterand, _ulResult, _format, args... ) \
        dos_volog(LOG_LEVEL_CIRT, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), ##args)

#define logo_alert(_pszOpterator, _pszOpterand, _ulResult, _format, args... ) \
        dos_volog(LOG_LEVEL_ALERT, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), ##args)

#define olog_emerg(_pszOpterator, _pszOpterand, _ulResult, _format, args... ) \
        dos_volog(LOG_LEVEL_EMERG, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), ##args)

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
