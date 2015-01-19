#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_DISK_MONITOR

#include <fcntl.h>
#include "mon_get_disk_info.h"
#include "mon_string.h"

extern  S8 g_szMonDiskInfo[MAX_PARTITION_COUNT * MAX_BUFF_LENGTH];
extern  MON_SYS_PART_DATA_S * g_pastPartition[MAX_PARTITION_COUNT];
extern  S32 g_lPartCnt;

static S32  mon_get_disk_temperature();
static S8 * mon_get_disk_serial_num(S8 * pszPartitionName);


S32 mon_disk_malloc()
{
   S32 lRows = 0;
   MON_SYS_PART_DATA_S * pstPartition;
   
   pstPartition = (MON_SYS_PART_DATA_S *)dos_dmem_alloc(MAX_PARTITION_COUNT * sizeof(MON_SYS_PART_DATA_S));
   if(!pstPartition)
   {
      logr_cirt("%s:Line %d: mon_disk_free|alloc memory failure,pstPartition is %p!"
                , dos_get_filename(__FILE__), __LINE__, pstPartition);
      return DOS_FAIL;
   }

   memset(pstPartition, 0, MAX_PARTITION_COUNT * sizeof(MON_SYS_PART_DATA_S));

   for(lRows = 0; lRows < MAX_PARTITION_COUNT; lRows++)
   {
      g_pastPartition[lRows] = &(pstPartition[lRows]);
   }

   return DOS_SUCC;
}

S32 mon_disk_free()
{
   S32 lRows = 0;
   MON_SYS_PART_DATA_S * pstPartition = g_pastPartition[0];
   if(!pstPartition)
   {
      logr_cirt("%s:Line %d:mon_disk_free|free memory failure,pstPartition is %p!"
                , dos_get_filename(__FILE__) , __LINE__, pstPartition);
      return DOS_FAIL;  
   }
   dos_dmem_free(pstPartition);
   
   for(lRows = 0 ; lRows < MAX_PARTITION_COUNT; lRows++)
   {
      g_pastPartition[lRows] = NULL;
   }
   
   return DOS_SUCC;
}


static S32  mon_get_disk_temperature()
{//保留，未完成
   
   return DOS_SUCC;
}

static S8 *  mon_get_disk_serial_num(S8 * pszPartitionName)
{//保留，未完成
    if(!pszPartitionName)
    {
       logr_cirt("%s:Line %d:mon_get_disk_serial_num|pszPartitionName is %p!",
                  dos_get_filename(__FILE__), __LINE__, pszPartitionName);
       return NULL;
    }
    
    return "IC35L180AVV207-1";
}


S32 mon_get_partition_data()
{
   S8  szDfCmd[MAX_CMD_LENGTH] = {0};
   S8  szLine[MAX_BUFF_LENGTH] = {0};
   S8  szDiskFileName[] = "disk.info";
   S8* pszAnalyseRslt[6] = {0};
   U32 ulRet = 0;
   FILE * fp = NULL;
   g_lPartCnt = 0;

   dos_snprintf(szDfCmd, MAX_CMD_LENGTH, "df | grep /> %s", szDiskFileName);
   system(szDfCmd);

   fp = fopen(szDiskFileName, "r");
   if (!fp)
   {
      logr_cirt("%s: Line %d:mon_get_partition_data|file \'%s\' fp is %p!"
                , dos_get_filename(__FILE__) , __LINE__, szDiskFileName, fp);
      return DOS_FAIL;
   }
   
   fseek(fp, 0, SEEK_SET);
   while(!feof(fp))
   {
      S32 lData = 0;
      S32 lRet = 0;
      S8* pszTemp = NULL;
      if (NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
      {
         ulRet = mon_analyse_by_reg_expr(szLine, " \t\n", pszAnalyseRslt, sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]));
         if(DOS_SUCC != ulRet)
         {
            logr_error("%s:Line %d:mon_get_partition_data|analyse sentence failure!"
                         , dos_get_filename(__FILE__), __LINE__);
            goto failure;
         }
            
         dos_strcpy(g_pastPartition[g_lPartCnt]->szPartitionName, pszAnalyseRslt[0]);
         g_pastPartition[g_lPartCnt]->szPartitionName[dos_strlen(pszAnalyseRslt[0])] = '\0';

         lRet = dos_atol(pszAnalyseRslt[3], &lData);
         if(0 != lRet)
         {
            logr_error("%s:Line %d:mon_get_partition_data|dos_atol failure!"
                         , dos_get_filename(__FILE__), __LINE__);
            goto failure;
         }
         g_pastPartition[g_lPartCnt]->lPartitionAvailBytes = lData;

         pszTemp = pszAnalyseRslt[4];
         while('%' != *pszTemp)
         {
            ++pszTemp;
         }
         *pszTemp = '\0';

         lRet = dos_atol(pszAnalyseRslt[4], &lData);
         if(0 != lRet)
         {
            logr_error("%s:Line %d:mon_get_partition_data|dos_atol failure!"
                         , dos_get_filename(__FILE__), __LINE__);
            goto failure;
         }
         g_pastPartition[g_lPartCnt]->lPartitionUsageRate = lData;

         dos_strcpy(g_pastPartition[g_lPartCnt]->szDiskSerialNo, mon_get_disk_serial_num(pszAnalyseRslt[0]));
         g_pastPartition[g_lPartCnt]->szDiskSerialNo[dos_strlen(mon_get_disk_serial_num(pszAnalyseRslt[0]))] = '\0';
         ++g_lPartCnt;
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

S32  mon_get_total_disk_usage_rate()
{
   U32  ulRows = 0;
   S32  lTotal = 0;
   S32  lUsed  = 0;
   
   for (ulRows = 0; ulRows < g_lPartCnt; ulRows++)
   {
      S32 lOneAvail = 0;
      S32 lOneTotal = 0;
      S32 lOneUsed  = 0;
      S32 lOneRate  = 0;
      
      if(!g_pastPartition[ulRows])
      {
         logr_error("%s:Line %d:mon_get_total_disk_usage_rate|g_pastPartition[%u] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, ulRows
                    , g_pastPartition[g_lPartCnt]);
         return DOS_FAIL;
      }
      
      lOneAvail = g_pastPartition[ulRows]->lPartitionAvailBytes;
      lOneRate  = g_pastPartition[ulRows]->lPartitionUsageRate;

      lOneAvail *= 100;
      lOneTotal = (lOneAvail + lOneAvail % (100 - lOneRate))/(100 - lOneRate);
      lOneUsed  = lOneTotal * lOneRate / 100;

      lTotal += lOneTotal;
      lUsed  += lOneUsed;
      
   }

   lTotal /= 100;
   
   return (lUsed + lUsed % lTotal) / lTotal;
}


S32 mon_get_partition_formatted_info()
{
   S32 lRows = 0;
   S8  szTempBuff[MAX_BUFF_LENGTH] = {0};
   S32 lDiskKBytes = 0;
   S32 lDiskUsageRate = 0;
   S32 lDiskTemperature = 0;

   lDiskKBytes = mon_get_total_disk_kbytes();
   if(DOS_SUCC == lDiskKBytes)
   {
      logr_error("%s:Line %d:mon_get_partition_struct_obj_array_formatted_info|get total disk bytes failure,lDiskKBytes is %d!", 
                    dos_get_filename(__FILE__), __LINE__, lDiskKBytes);
      return DOS_FAIL;
   }

   lDiskUsageRate = mon_get_total_disk_usage_rate();
   if(-1 == lDiskUsageRate)
   {
      logr_error("%s:Line %d:mon_get_partition_struct_obj_array_formatted_info|get total disk usage rate failure,lDiskUsageRate is %d!",
                 dos_get_filename(__FILE__), __LINE__, lDiskUsageRate);
      return DOS_FAIL;
   }

   lDiskTemperature = mon_get_disk_temperature();
   if(-1 == lDiskTemperature)
   {
      logr_error("%s:Line %d:mon_get_partition_struct_obj_array_formatted_info|get disk temperature failure,lDiskTemperature is %d", 
                 dos_get_filename(__FILE__) ,__LINE__, lDiskTemperature);
      return DOS_FAIL;
   }

   memset(g_szMonDiskInfo, 0, MAX_PARTITION_COUNT * MAX_BUFF_LENGTH);
   
   for (lRows = 0; lRows < g_lPartCnt; lRows++)
   {
      S32 lOneAvail = 0;
      S32 lOneRate = 0;
      S8  szTempInfo[MAX_BUFF_LENGTH] = {0};

      lOneAvail = g_pastPartition[lRows]->lPartitionAvailBytes;
      lOneRate  = g_pastPartition[lRows]->lPartitionUsageRate;
      lOneAvail *= 100;

      if(!g_pastPartition[lRows])
      {
         logr_cirt("%s:Line %d:mon_get_partition_struct_obj_array_formatted_info|get partition formatted information failure,m_pastPartition[%d] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, lRows, g_pastPartition[lRows]);
         return DOS_FAIL;
      }
      
      dos_snprintf(szTempInfo, MAX_BUFF_LENGTH, "partition %d:\nName:%s\nTotal KBytes:%d\nUsage Rate:%d%%\nSerialNo:%s\n",
                    lRows, g_pastPartition[lRows]->szPartitionName,
                    (lOneAvail + lOneAvail % (100 - lOneRate)) / (100 - lOneRate),
                    g_pastPartition[lRows]->lPartitionUsageRate,
                    g_pastPartition[lRows]->szDiskSerialNo);
      dos_strcat(g_szMonDiskInfo, szTempInfo);
      dos_strcat(g_szMonDiskInfo, "\n");
   }
   
   dos_snprintf(szTempBuff, MAX_BUFF_LENGTH, "Total Disk KBytes:%d\nTotal Usage Rate:%d%%\nDisk Temperature:%d\n"
                , lDiskKBytes, lDiskUsageRate, lDiskTemperature);
   dos_strcat(g_szMonDiskInfo, szTempBuff);

   return DOS_SUCC;
}

S32 mon_get_total_disk_kbytes()
{
   S32 lTotal = 0;
   U32 ulRows = 0;

   for(ulRows = 0; ulRows < g_lPartCnt; ulRows++)
   {
      S32 lOneTotal = 0;
      S32 lOneAvail = 0;
      S32 lOneRate  = 0;

      lOneAvail = g_pastPartition[ulRows]->lPartitionAvailBytes;
      lOneRate  = g_pastPartition[ulRows]->lPartitionUsageRate;

      lOneAvail *= 100;
      lOneTotal = (lOneAvail + lOneAvail % (100 - lOneRate)) / (100 - lOneRate);
      lTotal += lOneTotal;
   }

   return lTotal / 100;
}

#endif //end #if INCLUDE_DISK_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif
