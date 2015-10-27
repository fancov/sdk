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
typedef enum tagSCSubmodDef
{
    SC_FUNC         = 0,
    SC_HTTPD,
    SC_HTTP_API,
    SC_ACD,
    SC_TASK_MNGT,
    SC_TASK,
    SC_DIALER,
    SC_ESL,
    SC_BS,
    SC_SYN,
    SC_AUDIT,
    SC_DB,

    SC_SUB_MOD_BUTT,
}SC_SUB_MOD_DEF_EN;

#define SC_DEBUG_SUBMOD(ulCtrlFrame, ulSubmod)                         \
do{                                                                    \
    if ((ulSubmod) < SC_SUB_MOD_BUTT)                                  \
    {                                                                  \
        (ulCtrlFrame) = (ulCtrlFrame) | (0x00000001 << (ulSubmod));    \
    }                                                                  \
}while(0)

#define SC_NODEBUG_SUBMOD(ulCtrlFrame, ulSubmod)                       \
do{                                                                    \
    if ((ulSubmod) < SC_SUB_MOD_BUTT)                                  \
    {                                                                  \
        (ulCtrlFrame) = (ulCtrlFrame) & (~(0x00000001 << (ulSubmod))); \
    }                                                                  \
}while(0)

#define SC_CHECK_SUBMOD(ulCtrlFrame, ulSubmod)                         \
    ((ulSubmod) < SC_SUB_MOD_BUTT) && (ulCtrlFrame & (0x00000001 << (ulSubmod)))


#define sc_logr_debug(_ulSubmod, _pszFormat, ...) \
        sc_debug(_ulSubmod, LOG_LEVEL_DEBUG, (_pszFormat), __VA_ARGS__)

#define sc_logr_info(_ulSubmod, _pszFormat, ...) \
        sc_debug(_ulSubmod, LOG_LEVEL_INFO, (_pszFormat), __VA_ARGS__)

#define sc_logr_notice(_ulSubmod, _pszFormat, ...) \
        sc_debug(_ulSubmod, LOG_LEVEL_NOTIC, (_pszFormat), __VA_ARGS__)

#define sc_logr_warning(_ulSubmod, _pszFormat, ...) \
        sc_debug(_ulSubmod, LOG_LEVEL_WARNING, (_pszFormat), __VA_ARGS__)

#define sc_logr_error(_ulSubmod, _pszFormat, ...) \
        sc_debug(_ulSubmod, LOG_LEVEL_ERROR, (_pszFormat), __VA_ARGS__)

#define sc_logr_cirt(_ulSubmod, _pszFormat, ...) \
        sc_debug(_ulSubmod, LOG_LEVEL_CIRT, (_pszFormat), __VA_ARGS__)

#define sc_logr_alert(_ulSubmod, _pszFormat, ...) \
        sc_debug(_ulSubmod, LOG_LEVEL_ALERT, (_pszFormat), __VA_ARGS__)

#define sc_logr_emerg(_ulSubmod, _pszFormat, ...) \
        sc_debug(_ulSubmod, LOG_LEVEL_EMERG, (_pszFormat), __VA_ARGS__)

#if 0
#define SC_TRACE_IN(param0, param1, param2, param3) \
        sc_logr_debug(SC_FUNC, "SC Trace in %s (0x%X, 0x%X, 0x%X, 0x%X)" \
                        , __FUNCTION__, (U64)(param0), (U64)(param1), (U64)(param2), (U64)(param3));

#define SC_TRACE_OUT() \
        sc_logr_debug(SC_FUNC, "SC Trace out %s", __FUNCTION__);
#else
#define SC_TRACE_IN(param0, param1, param2, param3) do{}while(0)

#define SC_TRACE_OUT() do{}while(0)

#endif

#define SC_HTTPD_TRACE(param0, param1, param2, param3) \
        sc_logr_debug(SC_HTTPD, "SC Trace Func:%s (%s:%d) Srv: %lu, Client: %lu, RspCode:%lu, Errno:0x%X" \
                        , __FUNCTION__, dos_get_filename(__FILE__), __LINE__, (param0), (param1), (param2), (param3));

#define SC_TASK_TRACE(pstTCB, format, ...) \
        sc_logr_debug(SC_TASK, "SC Trace Func:%s (%s:%d) CustomID: %lu, TaskID: %lu" \
                        , __FUNCTION__, dos_get_filename(__FILE__), __LINE__, (pstTCB)->ulCustomID, (pstTCB)->ulTaskID);

/* define enums */

/* define structs */

/* declare functions */
VOID sc_debug(U32 ulSubMod, U32 ulLevel, const S8* szFormat, ...);
VOID sc_call_trace(SC_SCB_ST *pstSCB, const S8 *szFormat, ...);
VOID sc_task_trace(SC_TASK_CB_ST *pstTCB, const S8* szFormat, ...);

/* 强制同步 */
S32 cli_cc_update(U32 ulIndex, S32 argc, S8 **argv);
S32 sc_update_route(U32 ulID);
S32 sc_update_gateway_grp(U32 ulID);
S32 sc_update_gateway(U32 ulID);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SC_DEBUG_H__ */


