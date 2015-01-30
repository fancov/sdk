/*
 *      (C)Copyright 2014,dipcc,CO.,LTD.
 *            ALL RIGHTS RESERVED
 *
 *       monitor.h
 *       Created on:2014-11-29
 *           Author:bubble
 *             Todo:define common simple operations
 *          History:
 */

#ifndef _MONITOR_H__
#define _MONITOR_H__

#if INCLUDE_RES_MONITOR  

#define CPU_RES                   0x000000f1 //CPU资源
#define MEM_RES                   0x000000f2 //内存资源
#define DISK_RES                  0x000000f3 //磁盘资源
#define NET_RES                   0x000000f4 //网络资源
#define PROC_RES                  0x000000f5 //进程资源

#define RES_LACK                  0x00000001 //资源缺乏
#define TEMP_HIGH                 0x00000002 //温度过高

#define MAX_PID_VALUE             65535
#define MIN_PID_VALUE             0

#define MAX_TOKEN_LEN             256 //最大长度
#define MAX_TOKEN_CNT             16

#include <dos/dos_types.h>

#if 0
#define mon_logr_debug(format, args... ) \
do {\
    if (g_ulMonLogLevel >=　LOG_LEVEL_DEBUG) \
    { \
        dos_vlog(_iLevel,_iType,format,args...)(LOG_LEVEL_DEBUG, LOG_TYPE_WARNING, \
       (format), \
        ##args)\
    }\
}\
while(0)
#endif

S32  mon_init_str_array();
S32  mon_deinit_str_array();
BOOL mon_is_substr(S8 * pszSrc, S8 * pszDest);
BOOL mon_is_ended_with(S8 * pszSentence, const S8 * pszStr);
BOOL mon_is_suffix_true(S8 * pszFile, const S8 * pszSuffix);
S32  mon_first_int_from_str(S8 * pszStr);
U32  mon_analyse_by_reg_expr(S8* pszStr, S8* pszRegExpr, S8* pszRsltList[], U32 ulLen);
U32  mon_generate_warning_id(U32 ulResType, U32 ulNo, U32 ulErrType);
S8 * mon_get_proc_name_by_id(S32 lPid, S8 * pszPidName);
S8 * mon_str_get_name(S8 * pszCmd); 


#endif //#if INCLUDE_RES_MONITOR  
#endif // end of _MONITOR_H__

