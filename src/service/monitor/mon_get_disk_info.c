#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>
#include <dos/dos_mem.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_DISK_MONITOR

#include <fcntl.h>
#include "mon_get_disk_info.h"
#include "mon_lib.h"
#include "mon_def.h"

extern  S8 g_szMonDiskInfo[MAX_PARTITION_COUNT * MAX_BUFF_LENGTH];
extern  MON_SYS_PART_DATA_S * g_pastPartition[MAX_PARTITION_COUNT];
extern  U32 g_ulPartCnt;

#if 0
static U32  mon_get_disk_temperature();
#endif

static S8 * mon_get_disk_serial_num(S8 * pszPartitionName);
static U32  mon_partition_reset_data();

/**
 * 功能:为分区信息数组分配内存
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_disk_malloc()
{
    U32 ulRows = 0;
    MON_SYS_PART_DATA_S * pstPartition = NULL;

    pstPartition = (MON_SYS_PART_DATA_S *)dos_dmem_alloc(MAX_PARTITION_COUNT * sizeof(MON_SYS_PART_DATA_S));
    if(!pstPartition)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero(pstPartition, MAX_PARTITION_COUNT * sizeof(MON_SYS_PART_DATA_S));

    for(ulRows = 0; ulRows < MAX_PARTITION_COUNT; ulRows++)
    {
        g_pastPartition[ulRows] = &(pstPartition[ulRows]);
    }

    return DOS_SUCC;
}

/**
 * 功能:释放为分区信息数组分配的内存
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_disk_free()
{
    U32 ulRows = 0;
    MON_SYS_PART_DATA_S * pstPartition = g_pastPartition[0];
    if(DOS_ADDR_INVALID(pstPartition))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    dos_dmem_free(pstPartition);

    for(ulRows = 0 ; ulRows < MAX_PARTITION_COUNT; ulRows++)
    {
        g_pastPartition[ulRows] = NULL;
    }

    pstPartition = NULL;

    return DOS_SUCC;
}

static U32  mon_partition_reset_data()
{
    MON_SYS_PART_DATA_S * pstPartition = g_pastPartition[0];

    if(DOS_ADDR_INVALID(pstPartition))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero(pstPartition, MAX_PARTITION_COUNT * sizeof(MON_SYS_PART_DATA_S));

    return DOS_SUCC;
}


/**
 * 功能:获取磁盘的温度
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
#if 0
static U32  mon_get_disk_temperature()
{
   /* 由于软件包hdparm安装问题，故无法采用 */
   return DOS_SUCC;
}
#endif

/**
 * 功能:获取磁盘的序列号信息字符串
 * 参数集：
 *   参数1: S8 * pszPartitionName 分区名
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
static S8 *  mon_get_disk_serial_num(S8 * pszPartitionName)
{   /* 目前没有有效的方法获得其硬件序列 */
    if(DOS_ADDR_INVALID(pszPartitionName))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    return "IC35L180AVV207-1";
}

/** df命令输出
 * Filesystem                   1K-blocks     Used Available Use% Mounted on
 * /dev/mapper/VolGroup-lv_root   8780808  6175596   2159160  75% /
 * tmpfs                           699708       72    699636   1% /dev/shm
 * /dev/sda1                       495844    34870    435374   8% /boot
 * .host:/                      209715196 19219176 190496020  10% /mnt/hgfs
 * 功能:获取磁盘的序列号信息字符串
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
 U32 mon_get_partition_data()
 {
     S8 szDfCmd[] = "df -l | grep -v \'tmpfs\'| grep /dev/";
     S8 szBuff[MAX_BUFF_LENGTH] = {0};
     FILE *fp = NULL;
     S8 *paszInfo[6] = {0};
     S8 *pszTemp = NULL;
     U32 ulRet = U32_BUTT;
     U32 ulTotalVolume = 0, ulUsedVolume = 0, ulAvailVolume = 0, ulTotalRate = 0;

     g_ulPartCnt = 0;

     fp = popen(szDfCmd, "r");
     if (DOS_ADDR_INVALID(fp))
     {
         DOS_ASSERT(0);
         return DOS_FAIL;
     }

     ulRet = mon_partition_reset_data();
     if (DOS_SUCC != ulRet)
     {
         mon_trace(MON_TRACE_DISK, LOG_LEVEL_ERROR, "Reset Partition Data FAIL.");
         return DOS_FAIL;
     }

     while( !feof(fp))
     {
         if (NULL != fgets(szBuff, MAX_BUFF_LENGTH, fp))
         {
             ulRet = mon_analyse_by_reg_expr(szBuff, " \t\n", paszInfo, sizeof(paszInfo) / sizeof(paszInfo[0]));
             if (DOS_SUCC != ulRet)
             {
                 mon_trace(MON_TRACE_DISK, LOG_LEVEL_ERROR, "Analyse Buff by Regular expression FAIL.");
                 goto fail;
             }

            dos_snprintf(g_pastPartition[g_ulPartCnt]->szPartitionName, MAX_PARTITION_LENGTH, "%s", paszInfo[0]);

            if (dos_atoul(paszInfo[1], &ulTotalVolume) < 0)
            {
                DOS_ASSERT(0);
                goto fail;
            }

            if (dos_atoul(paszInfo[2], &ulUsedVolume) < 0)
            {
                DOS_ASSERT(0);
                goto fail;
            }

            if (dos_atoul(paszInfo[3], &ulAvailVolume) < 0)
            {
                DOS_ASSERT(0);
                goto fail;
            }

            pszTemp = paszInfo[4];
            while('%' != *pszTemp)
            {
                ++pszTemp;
            }

            *pszTemp = '\0';

            if (dos_atoul(paszInfo[4], &ulTotalRate) < 0)
            {
                DOS_ASSERT(0);
                goto fail;
            }

            g_pastPartition[g_ulPartCnt]->ulPartitionTotalBytes = ulTotalVolume;
            g_pastPartition[g_ulPartCnt]->ulPartitionUsedBytes = ulUsedVolume;
            g_pastPartition[g_ulPartCnt]->ulPartitionAvailBytes = ulAvailVolume;
            g_pastPartition[g_ulPartCnt]->ulPartitionUsageRate = ulTotalRate;

            pszTemp = mon_get_disk_serial_num(g_pastPartition[g_ulPartCnt]->szPartitionName);
            dos_snprintf(g_pastPartition[g_ulPartCnt]->szDiskSerialNo, MAX_PARTITION_LENGTH, "%s", pszTemp);

            ++g_ulPartCnt;
        }
    }

    pclose(fp);
    fp = NULL;
    return DOS_SUCC;

fail:
    pclose(fp);
    fp = NULL;
    return DOS_FAIL;
 }

/**
 * 功能:获取磁盘的总占用率
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回占用率大小，失败返回DOS_FAIL
 */
U32  mon_get_total_disk_usage_rate()
{
    U32 ulRows = 0;
    U32 ulTotal = 0, ulUsed = 0;

    for (ulRows = 0; ulRows < g_ulPartCnt; ++ulRows)
    {
        ulTotal += g_pastPartition[ulRows]->ulPartitionAvailBytes + g_pastPartition[ulRows]->ulPartitionUsedBytes ;
        ulUsed += g_pastPartition[ulRows]->ulPartitionUsedBytes;
    }

    ulUsed *= 100;

    return ulUsed / ulTotal + 1;
}

/**
 * 功能:获取磁盘的总占用率
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回磁盘总字节数(KBytes)，失败返回DOS_FAIL
 */
U32 mon_get_total_disk_kbytes()
{
    U32  ulTotal = 0, ulRows = 0;

    for (ulRows = 0; ulRows < g_ulPartCnt; ++ulRows)
    {
        ulTotal += g_pastPartition[ulRows]->ulPartitionTotalBytes;
    }

    return ulTotal;
}

#endif //end #if INCLUDE_DISK_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

