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
	S32  lPhysicalMemTotalBytes; //物理内存总大小KBytes 
    S32  lPhysicalMemFreeBytes;  //空闲物理内存大小KBytes
    S32  lBuffers;               //Buffers大小
    S32  lCached;                //Cached大小
	S32  lSwapTotalBytes;        //Swap交换分区大小
    S32  lSwapFreeBytes;         //Swap空闲分区大小
    S32  lPhysicalMemUsageRate;  //物理内存使用率
    S32  lSwapUsageRate;         //Swap交换分区占用率
}MON_SYS_MEM_DATA_S;


S32  mon_mem_malloc();
S32  mon_mem_free();
S32  mon_read_mem_file();
S32  mon_get_mem_data();
S32  mon_get_mem_formatted_info();

#endif // #if INCLUDE_RES_MONITOR  
#endif //end _MON_GET_MEM_INFO_H__

