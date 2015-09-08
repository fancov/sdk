#ifndef _MON_DEF_H_
#define _MON_DEF_H_

#include "mon_lib.h"


enum tagMonSubMod
{
    MON_TRACE_MEM = 0,      /* 内存资源模块 */
    MON_TRACE_CPU,          /* CPU资源模块 */
    MON_TRACE_DISK,         /* 硬盘资源模块 */
    MON_TRACE_NET,          /* 网络资源模块 */
    MON_TRACE_PROCESS,      /* 进程资源模块 */
    MON_TRACE_MH,           /* 监控与处理模块 */
    MON_TRACE_PUB,          /* 资源监控公共模块 */
    MON_TRACE_LIB,          /* 资源监控库模块 */
    MON_TRACE_NOTIFY,       /* 告警通知模块 */
    MON_TRACE_WARNING_MSG,  /* 告警消息模块 */

    MON_TRACE_BUTT = 32 /* 无效模块 */
};

enum tagSysRestartType
{
    MON_SYS_RESTART_IMMEDIATELY = 0,  /* 系统马上重启 */
    MON_SYS_RESTART_FIXED,            /* 系统定时重启 */
    MON_SYS_RESTART_LATER,            /* 稍后重启(没有业务时) */

    MON_SYS_RESTART_BUTT = 16
}MON_SYS_RESTART_TYPE;

U32 mon_restart_system(U32 ulStyle, U32 ulTimeStamp);
U32 mon_system(S8 *pszCmd);
VOID mon_show_cpu(U32 ulIndex);
VOID mon_show_mem(U32 ulIndex);
VOID mon_show_disk(U32 ulIndex);
VOID mon_show_netcard(U32 ulIndex);
VOID mon_show_process(U32 ulIndex);

VOID mon_trace(U32 ulTraceTarget, U8 ucTraceLevel, const S8 * szFormat, ...);

U32 mon_init_notify_list();

#endif
