/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        process_info.h
  *        Created on:2014-12-03
  *            Author:bubble
  *              Todo:Monitoring process resources
  *           History:
  */


#ifndef _MON_GET_PROC_INFO_H__
#define _MON_GET_PROC_INFO_H__

#if INCLUDE_RES_MONITOR  


#include <dos/dos_types.h>

#define STRING_MAX_LEN   8
#define MAX_PROC_CNT     16
#define MAX_CMD_LENGTH   64
#define MAX_BUFF_LENGTH  1024
#define MAX_PID_VALUE    65535
#define MIN_PID_VALUE    0


typedef struct tagMonProcStatus
{
   S32 lProcId;           //进程id
   S8  szProcName[64];    //进程名
   F64 fMemoryRate;       //内存占用率
   F64 fCPURate;          //CPU平均占用率
   S8  szProcCPUTime[32]; //CPU持续时间
   S32 lOpenFileCnt;      //打开的文件个数
   S32 lDBConnCnt;        //数据库连接数
   S32 lThreadsCnt;       //线程数
}MON_PROC_STATUS_S;


S32  mon_proc_malloc();
S32  mon_proc_free();
S32  mon_get_process_data();
S32  mon_kill_all_monitor_process();
S32  mon_restart_computer();
S8*  mon_get_proc_name_by_id(S32 lPid, S8 * pszPidName);
BOOL mon_is_proc_dead(S32 lPid);
S32  mon_get_proc_total_cpu_rate();
S32  mon_get_proc_total_mem_rate();
S32  mon_get_process_formatted_info();

#endif //#if INCLUDE_RES_MONITOR  
#endif //end _MON_GET_PROC_INFO_H__

