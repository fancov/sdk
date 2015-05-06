/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        disk_info.h
  *        Created on:2014-12-01
  *            Author:bubble
  *              Todo:Monitoring disk resources
  *           History:
  */


#ifndef _MON_GET_DISK_INFO_H__
#define _MON_GET_DISK_INFO_H__

#if INCLUDE_RES_MONITOR  

#include <dos/dos_types.h>

#define MAX_PARTITION_COUNT   16 //磁盘最大分区数
#define MAX_PARTITION_LENGTH  64
#define MAX_RAID_SERIAL_LEN   255
#define MAX_CMD_LENGTH        64
#define MAX_BUFF_LENGTH       1024


typedef struct tagMonSysPartData
{
    S8   szPartitionName[MAX_PARTITION_LENGTH];  //分区名
    U32  ulPartitionTotalBytes;  //硬盘总1K 块数
    U32  ulPartitionUsedBytes;   //已使用的块数
    U32  ulPartitionAvailBytes;  //未使用块数
    U32  ulPartitionUsageRate;   //分区使用率
    S8   szDiskSerialNo[MAX_PARTITION_LENGTH]; //所在磁盘序列号
}MON_SYS_PART_DATA_S;

U32  mon_disk_malloc();
U32  mon_disk_free();
U32  mon_get_total_disk_usage_rate();
U32  mon_get_partition_data();
U32  mon_get_partition_formatted_info();
U32  mon_get_total_disk_kbytes();

#endif //#if INCLUDE_RES_MONITOR  
#endif //end of _MON_GET_DISK_INFO_H__

