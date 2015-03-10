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
#define MAX_DESC_LENGTH    256
#define MAX_BUFF_LENGTH    1024
#define PORT_NO            11111

typedef struct tagThreshold
{
   S32 lMemThreshold;
   S32 lCPUThreshold;
   S32 l5sCPUThreshold;
   S32 l1minCPUThreshold;
   S32 l10minCPUThreshold;
   S32 lPartitionThreshold;
   S32 lDiskThreshold;
   S32 lProcMemThreshold;
   S32 lProcCPUThreshold;
}MON_THRESHOLD_S;

#if 0
VOID read_xml(const S8 * xml_file, MON_WARNING_COND_S * pCond);
#endif // end #if 0



VOID * mon_res_monitor(VOID *p);
VOID * mon_warning_handle(VOID *p);
S32    mon_res_alloc();
S32    mon_res_destroy();

#endif //#if INCLUDE_RES_MONITOR  
#endif //end _MON_MONITOR_AND_HANDLE_H__