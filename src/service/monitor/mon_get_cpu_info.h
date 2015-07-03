/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        cpu_info.h
  *        Created on:2014-12-01
  *            Author:bubble
  *              Todo:Monitoring CPU resources
  *           History:
  */


#ifndef _MON_GET_CPU_INFO_H__
#define _MON_GET_CPU_INFO_H__

#if INCLUDE_RES_MONITOR

#include <dos/dos_types.h>

#define TEN_MINUTE_SAMPLE_ARRAY  120 //10min最大节点个数
#define MAX_BUFF_LENGTH          1024
#define PENULTIMATE              119 //倒数第二
#define COUNTDOWN_XII            108 //倒数第十二


typedef struct tagSysMonCPUTime
{
   U32  ulUser;         //用户态CPU时间
   U32  ulNice;         //nice为负的进程占用CPU时间
   U32  ulSystem;       //核心时间
   U32  ulIdle;         //CPU空闲等待时间
   U32  ulIowait;       //硬盘IO等待时间
   U32  ulHardirq;      //硬中断时间
   U32  ulSoftirq;      //软中断时间
   U32  ulStealstolen;  //其他操作系统虚机环境时间
   U32  ulGuest;        //内核态下系统虚拟cpu占用时间
   struct tagSysMonCPUTime * next;  //指向下一节点的指针
   struct tagSysMonCPUTime * prior; //指向前一个节点指针
}MON_SYS_CPU_TIME_S;

typedef struct tagMonCPURslt
{
  U32 ulCPUUsageRate;     //CPU总体平均占用率
  U32 ulCPU5sUsageRate;   //5s钟CPU平均占用率
  U32 ulCPU1minUsageRate; //1min钟CPU平均占用率
  U32 ulCPU10minUsageRate;//10min钟CPU平均占用率
}MON_CPU_RSLT_S;



U32  mon_cpu_rslt_malloc();
U32  mon_cpu_rslt_free();
U32  mon_init_cpu_queue();
U32  mon_get_cpu_rslt_data();
U32  mon_cpu_queue_destroy();

#endif // end #if INCLUDE_RES_MONITOR
#endif // end of _MON_GET_CPU_INFO_H__

