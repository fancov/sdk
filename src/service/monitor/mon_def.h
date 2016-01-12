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
    MON_TRACE_MAIL,         /* 邮件模块 */
    MON_TRACE_CONFIG,       /* 配置文件模块 */

    MON_TRACE_BUTT = 32 /* 无效模块 */
};

typedef struct tagThreshold
{
    U32 ulMemThreshold;       //系统内存最大占用率
    U32 ulCPUThreshold;       //系统CPU平均最大占用率
    U32 ul5sCPUThreshold;     //5s CPU平均最大占用率
    U32 ul1minCPUThreshold;   //1min CPU平均最大占用率
    U32 ul10minCPUThreshold;  //10min CPU平均最大占用率
    U32 ulPartitionThreshold; //分区最大平均占用率
    U32 ulDiskThreshold;      //磁盘最大平均占用率
    U32 ulMaxBandWidth;       //最大带宽限制 90Mbps
    U32 ulProcMemThreshold;   //进程占用内存最大百分比
}MON_THRESHOLD_S;

typedef struct tagWarningMsg
{
    U32   ulNo;              //告警编号
    BOOL  bExcep;            //是否正常状态
    U32   ulWarningLevel;    //告警级别
    S8    szWarningDesc[128]; //告警描述
    S8    szNormalDesc[32];  //正常描述
}MON_WARNING_MSG_S;

U32 mon_system(S8 *pszCmd);
VOID mon_show_cpu(U32 ulIndex);
VOID mon_show_mem(U32 ulIndex);
VOID mon_show_disk(U32 ulIndex);
VOID mon_show_netcard(U32 ulIndex);
VOID mon_show_process(U32 ulIndex);
VOID mon_trace(U32 ulTraceTarget, U8 ucTraceLevel, const S8 * szFormat, ...);

U32 mon_init_notify_list();

#endif
