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

VOID * mon_res_monitor(VOID *p);
VOID * mon_warning_handle(VOID *p);
U32    mon_res_alloc();
U32    mon_res_destroy();

#endif //#if INCLUDE_RES_MONITOR
#endif //end _MON_MONITOR_AND_HANDLE_H__