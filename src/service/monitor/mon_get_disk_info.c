#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_DISK_MONITOR

#include <fcntl.h>
#include "mon_get_disk_info.h"
#include "mon_lib.h"

extern  S8 g_szMonDiskInfo[MAX_PARTITION_COUNT * MAX_BUFF_LENGTH];
extern  MON_SYS_PART_DATA_S * g_pastPartition[MAX_PARTITION_COUNT];
extern  U32 g_ulPartCnt;

static U32  mon_get_disk_temperature();
static S8 * mon_get_disk_serial_num(S8 * pszPartitionName);

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
      logr_cirt("%s:Line %d: mon_disk_free|alloc memory failure,pstPartition is %p!"
                , dos_get_filename(__FILE__), __LINE__, pstPartition);
      return DOS_FAIL;
   }

   memset(pstPartition, 0, MAX_PARTITION_COUNT * sizeof(MON_SYS_PART_DATA_S));

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
   if(!pstPartition)
   {
      logr_cirt("%s:Line %u:mon_disk_free|free memory failure,pstPartition is %p!"
                , dos_get_filename(__FILE__) , __LINE__, pstPartition);
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

/**
 * 功能:获取磁盘的温度
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
static U32  mon_get_disk_temperature()
{
   /* 由于软件包hdparm安装问题，故无法采用 */
   return DOS_SUCC;
}

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
       logr_cirt("%s:Line %u:mon_get_disk_serial_num|pszPartitionName is %p!",
                  dos_get_filename(__FILE__), __LINE__, pszPartitionName);
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
   S8  szDfCmd[MAX_CMD_LENGTH] = {0};
   S8  szLine[MAX_BUFF_LENGTH] = {0};
   S8  szDiskFileName[] = "disk.info";
   S8* pszAnalyseRslt[6] = {0};
   U32 ulRet = 0;
   FILE * fp = NULL;
   g_ulPartCnt = 0;

   dos_snprintf(szDfCmd, MAX_CMD_LENGTH, "df | grep /dev/> %s", szDiskFileName);
   system(szDfCmd);

   fp = fopen(szDiskFileName, "r");
   if (DOS_ADDR_INVALID(fp))
   {
      logr_cirt("%s: Line %u:mon_get_partition_data|file \'%s\' fp is %p!"
                , dos_get_filename(__FILE__) , __LINE__, szDiskFileName, fp);
      return DOS_FAIL;
   }
   
   fseek(fp, 0, SEEK_SET);
   while(!feof(fp))
   {
      U32 ulData = 0;
      S32 lRet = 0;
      S8* pszTemp = NULL;
      if (NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
      {
         ulRet = mon_analyse_by_reg_expr(szLine, " \t\n", pszAnalyseRslt, sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]));
         if(DOS_SUCC != ulRet)
         {
            logr_error("%s:Line %u:mon_get_partition_data|analyse sentence failure!"
                         , dos_get_filename(__FILE__), __LINE__);
            goto failure;
         }

         if (0 == dos_strncmp("tmpfs", pszAnalyseRslt[0], dos_strlen("tmpfs")))
         {
            continue;
         }
            
         dos_strcpy(g_pastPartition[g_ulPartCnt]->szPartitionName, pszAnalyseRslt[0]);
         g_pastPartition[g_ulPartCnt]->szPartitionName[dos_strlen(pszAnalyseRslt[0])] = '\0';

         lRet = dos_atoul(pszAnalyseRslt[3], &ulData);
         if(0 != lRet)
         {
            logr_error("%s:Line %u:mon_get_partition_data|dos_atol failure!"
                         , dos_get_filename(__FILE__), __LINE__);
            goto failure;
         }
         g_pastPartition[g_ulPartCnt]->ulPartitionAvailBytes = ulData;

         pszTemp = pszAnalyseRslt[4];
         while('%' != *pszTemp)
         {
            ++pszTemp;
         }
         *pszTemp = '\0';

         lRet = dos_atoul(pszAnalyseRslt[4], &ulData);
         if(0 > lRet)
         {
            logr_error("%s:Line %u:mon_get_partition_data|dos_atol failure!"
                         , dos_get_filename(__FILE__), __LINE__);
            goto failure;
         }
         g_pastPartition[g_ulPartCnt]->ulPartitionUsageRate = ulData;

         dos_strcpy(g_pastPartition[g_ulPartCnt]->szDiskSerialNo, mon_get_disk_serial_num(pszAnalyseRslt[0]));
         g_pastPartition[g_ulPartCnt]->szDiskSerialNo[dos_strlen(mon_get_disk_serial_num(pszAnalyseRslt[0]))] = '\0';
         ++g_ulPartCnt;
      } 
   }
   goto success;

success:   
   fclose(fp);
   fp = NULL;
   unlink(szDiskFileName);
   return DOS_SUCC;
failure:
   fclose(fp);
   fp = NULL;
   unlink(szDiskFileName);
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
   U32  ulRows = 0;
   U32  ulTotal = 0;
   U32  ulUsed  = 0;

   /**
    *  使用率的算法:
    *     rate = busy/total
    */
   for (ulRows = 0; ulRows < g_ulPartCnt; ulRows++)
   {
      U32 ulOneAvail = 0;
      U32 ulOneTotal = 0;
      U32 ulOneUsed  = 0;
      U32 ulOneRate  = 0;
      
      if(DOS_ADDR_INVALID(g_pastPartition[ulRows]))
      {
         logr_error("%s:Line %u:mon_get_total_disk_usage_rate|g_pastPartition[%u] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, ulRows
                    , g_pastPartition[ulRows]);
         return DOS_FAIL;
      }
      
      ulOneAvail = g_pastPartition[ulRows]->ulPartitionAvailBytes;
      ulOneRate  = g_pastPartition[ulRows]->ulPartitionUsageRate;

      ulOneAvail *= 100;
      ulOneTotal = (ulOneAvail + ulOneAvail % (100 - ulOneRate))/(100 - ulOneRate);
      ulOneUsed  = ulOneTotal * ulOneRate / 100;

      ulTotal += ulOneTotal;
      ulUsed  += ulOneUsed;
      
   }

   ulTotal /= 100;
   
   return (ulUsed + ulUsed % ulTotal) / ulTotal;
}

/**
 * 功能:获取磁盘分区数组的格式化信息字符串
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_get_partition_formatted_info()
{
   U32 ulRows = 0;
   S8  szTempBuff[MAX_BUFF_LENGTH] = {0};
   U32 ulDiskKBytes = 0;
   U32 ulDiskUsageRate = 0;
   U32 ulDiskTemperature = 0;

   ulDiskUsageRate = mon_get_total_disk_usage_rate();
   if(DOS_FAIL == ulDiskUsageRate)
   {
      logr_error("%s:Line %u:mon_get_partition_formatted_info|get total disk usage rate failure,ulDiskUsageRate is %u!",
                 dos_get_filename(__FILE__), __LINE__, ulDiskUsageRate);
      return DOS_FAIL;
   }

   ulDiskKBytes = mon_get_total_disk_kbytes();

   ulDiskTemperature = mon_get_disk_temperature();
   if(DOS_FAIL == ulDiskTemperature)
   {
      logr_error("%s:Line %u:mon_get_partition_formatted_info|get disk temperature failure,ulDiskTemperature is %u", 
                 dos_get_filename(__FILE__) ,__LINE__, ulDiskTemperature);
      return DOS_FAIL;
   }

   memset(g_szMonDiskInfo, 0, MAX_PARTITION_COUNT * MAX_BUFF_LENGTH);
   
   for (ulRows = 0; ulRows < g_ulPartCnt; ulRows++)
   {
      U32 ulOneAvail = 0;
      U32 ulOneRate = 0;
      S8  szTempInfo[MAX_BUFF_LENGTH] = {0};

      ulOneAvail = g_pastPartition[ulRows]->ulPartitionAvailBytes;
      ulOneRate  = g_pastPartition[ulRows]->ulPartitionUsageRate;
      ulOneAvail *= 100;

      if(DOS_ADDR_INVALID(g_pastPartition[ulRows]))
      {
         logr_cirt("%s:Line %u:mon_get_partition_formatted_info|get partition formatted information failure,m_pastPartition[%d] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, ulRows, g_pastPartition[ulRows]);
         return DOS_FAIL;
      }
      
      dos_snprintf(szTempInfo, MAX_BUFF_LENGTH, "partition %u:\nName:%s\nTotal KBytes:%u\nUsage Rate:%u%%\nSerialNo:%s\n",
                    ulRows, g_pastPartition[ulRows]->szPartitionName,
                    (ulOneAvail + ulOneAvail % (100 - ulOneRate)) / (100 - ulOneRate),
                    g_pastPartition[ulRows]->ulPartitionUsageRate,
                    g_pastPartition[ulRows]->szDiskSerialNo);
      dos_strcat(g_szMonDiskInfo, szTempInfo);
      dos_strcat(g_szMonDiskInfo, "\n");
   }
   
   dos_snprintf(szTempBuff, MAX_BUFF_LENGTH, "Total Disk KBytes:%u\nTotal Usage Rate:%u%%\nDisk Temperature:%u\n"
                , ulDiskKBytes, ulDiskUsageRate, ulDiskTemperature);
   dos_strcat(g_szMonDiskInfo, szTempBuff);

   return DOS_SUCC;
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
   U32 ulTotal = 0;
   U32 ulRows = 0;

   for(ulRows = 0; ulRows < g_ulPartCnt; ulRows++)
   {
      U32 ulOneTotal = 0;
      U32 ulOneAvail = 0;
      U32 ulOneRate  = 0;

      ulOneAvail = g_pastPartition[ulRows]->ulPartitionAvailBytes;
      ulOneRate  = g_pastPartition[ulRows]->ulPartitionUsageRate;

      ulOneAvail *= 100;
      ulOneTotal = (ulOneAvail + ulOneAvail % (100 - ulOneRate)) / (100 - ulOneRate);
      ulTotal += ulOneTotal;
   }

   return ulTotal;
}

#endif //end #if INCLUDE_DISK_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

