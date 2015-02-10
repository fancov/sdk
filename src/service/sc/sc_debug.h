/*
*            (C) Copyright 2014, DIPCC . Co., Ltd.
*                    ALL RIGHTS RESERVED
*
*  文件名: sc_debug.h
*
*  创建时间: 2014年12月16日15:20:47
*  作    者: Larry
*  描    述: 业务控制模块调试相关定义
*  修改历史:
*/

#ifndef __SC_DEBUG_H__
#define __SC_DEBUG_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */

/* include private header files */

/* define marcos */
#define sc_logr_debug(_pszFormat, ...) \
        sc_logr_write(LOG_LEVEL_DEBUG, (_pszFormat), __VA_ARGS__)

#define sc_logr_info(_pszFormat, ...) \
        sc_logr_write(LOG_LEVEL_INFO, (_pszFormat), __VA_ARGS__)

#define sc_logr_notice(_pszFormat, ...) \
        sc_logr_write(LOG_LEVEL_NOTIC, (_pszFormat), __VA_ARGS__)

#define sc_logr_warning(_pszFormat, ...) \
        sc_logr_write(LOG_LEVEL_WARNING, (_pszFormat), __VA_ARGS__)

#define sc_logr_error(_pszFormat, ...) \
        sc_logr_write(LOG_LEVEL_ERROR, (_pszFormat), __VA_ARGS__)

#define sc_logr_cirt(_pszFormat, ...) \
        sc_logr_write(LOG_LEVEL_CIRT, (_pszFormat), __VA_ARGS__)

#define sc_logr_alert(_pszFormat, ...) \
        sc_logr_write(LOG_LEVEL_ALERT, (_pszFormat), __VA_ARGS__)

#define sc_logr_emerg(_pszFormat, ...) \
        sc_logr_write(LOG_LEVEL_EMERG, (_pszFormat), __VA_ARGS__)

#define SC_TRACE_IN(param0, param1, param2, param3) \
    do{ \
        if (g_ulTaskTraceAll)\
        {\
            sc_logr_debug("SC Trace in %s (0x%X, 0x%X, 0x%X, 0x%X)" \
                            , __FUNCTION__, (U64)(param0), (U64)(param1), (U64)(param2), (U64)(param3));\
        }\
    }while(0)

#define SC_TRACE_OUT() \
    do{ \
        if (g_ulTaskTraceAll)\
        {\
            sc_logr_debug("SC Trace out %s", __FUNCTION__);\
        }\
    }while(0)

#define SC_HTTPD_TRACE(param0, param1, param2, param3) \
    do{ \
        if (g_ulTaskTraceAll)\
        {\
            sc_logr_debug("SC Trace Func:%s (%s:%d) Srv: %lu, Client: %lu, RspCode:%lu, Errno:0x%X" \
                            , __FUNCTION__, dos_get_filename(__FILE__), __LINE__, (param0), (param1), (param2), (param3));\
        }\
    }while(0)

#define SC_TASK_TRACE(pstTCB, format, ...) \
    do{ \
        if (pstTCB && g_ulTaskTraceAll)\
        {\
            sc_logr_debug("SC Trace Func:%s (%s:%d) CustomID: %lu, TaskID: %lu" \
                            , __FUNCTION__, dos_get_filename(__FILE__), __LINE__, (pstTCB)->ulCustomID, (pstTCB)->ulTaskID);\
        }\
    }while(0)

/* define enums */

/* define structs */

/* declare functions */
VOID sc_logr_write(U32 ulLevel, S8 *pszFormat, ...);
VOID sc_call_trace(SC_CCB_ST *pstCCB, const S8 *szFormat, ...);
VOID sc_task_trace(SC_TASK_CB_ST *pstTCB, const S8* szFormat, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SC_DEBUG_H__ */


