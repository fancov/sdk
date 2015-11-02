/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        mem_info.h
  *        Created on:2014-11-29
  *            Author:bubble
  *              Todo:Monitoring memory resources
  *           History:
  */

#ifndef _MON_GET_MEM_INFO_H__
#define _MON_GET_MEM_INFO_H__

#if INCLUDE_RES_MONITOR

#include <dos/dos_types.h>

#define MEMBER_COUNT    6     //需要统计的成员个数
#define MAX_BUFF_LENGTH 1024  //缓冲区大小

typedef struct tagMonSysMemData
{
    U32  ulPhysicalMemTotalBytes; //物理内存总大小KBytes
    U32  ulPhysicalMemFreeBytes;  //空闲物理内存大小KBytes
    U32  ulCached;
    U32  ulBuffers;
    U32  ulSwapTotalBytes;        //Swap交换分区大小
    U32  ulSwapFreeBytes;         //Swap空闲分区大小
    U32  ulSwapUsageRate;         //Swap交换分区占用率
    U32  ulPhysicalMemUsageRate;  //物理内存使用率
}MON_SYS_MEM_DATA_S;

U32  mon_mem_malloc();
U32  mon_mem_free();
U32  mon_read_mem_file();
U32  mon_get_mem_data();
U32  mon_handle_mem_warning();

#endif // #if INCLUDE_RES_MONITOR
#endif //end _MON_GET_MEM_INFO_H__

