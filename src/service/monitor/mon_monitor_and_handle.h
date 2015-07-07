/*
  *      (C)Copyright 2014,dipcc,CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *       monitor.h
  *       Created on:2014-12-06
  *       Author:bubble
  *       Todo:monitor and handle warning message
  *       History:
  */

#ifndef _MON_MONITOR_AND_HANDLE_H__
#define _MON_MONITOR_AND_HANDLE_H__

#if INCLUDE_RES_MONITOR

#include <dos/dos_types.h>

#define SQL_CMD_MAX_LENGTH 1024
#define MAX_BUFF_LENGTH    1024

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

VOID * mon_res_monitor(VOID *p);
VOID * mon_warning_handle(VOID *p);
U32    mon_res_alloc();
U32    mon_res_destroy();

#endif //#if INCLUDE_RES_MONITOR
#endif //end _MON_MONITOR_AND_HANDLE_H__