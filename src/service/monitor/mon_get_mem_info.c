#ifdef __cplusplus
extern "C"{
#endif


#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_MEM_MONITOR

#include "mon_get_mem_info.h"
#include "mon_lib.h"

static S8  m_szMemInfoFile[] = "/proc/meminfo";
extern S8  g_szMonMemInfo[MAX_BUFF_LENGTH];
extern MON_SYS_MEM_DATA_S * g_pstMem;

/**
 * 功能:为g_pstMem分配内存
 * 参数集：
 *     无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32  mon_mem_malloc()
{
   g_pstMem = (MON_SYS_MEM_DATA_S *)dos_dmem_alloc(sizeof(MON_SYS_MEM_DATA_S));
   if (DOS_ADDR_INVALID(g_pstMem))
   {
      logr_cirt("%s:Line %u: mon_mem_malloc|pstMem is %p!",
                dos_get_filename(__FILE__), __LINE__, g_pstMem);
      return DOS_FAIL;
   }
   
   memset(g_pstMem, 0, sizeof(MON_SYS_MEM_DATA_S));
   
   return DOS_SUCC;
}


/**
 * 功能:释放为g_pstMem分配内存
 * 参数集：
 *     无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_mem_free()
{
   if (DOS_ADDR_INVALID(g_pstMem))
   {
      logr_notice("%s:line %u: mon_mem_free|free memory failure,g_pstMem is %p!"
                    , dos_get_filename(__FILE__), __LINE__, g_pstMem);
      return DOS_FAIL;
   }
   
   dos_dmem_free(g_pstMem);
   g_pstMem = NULL;
   
   return DOS_SUCC;
}


/** "/proc/meminfo"内容
 * MemTotal:        1688544 kB
 * MemFree:          649060 kB
 * Buffers:          103812 kB
 * Cached:           203864 kB
 * SwapCached:            0 kB
 * Active:           640116 kB
 * ......
 * Mlocked:               0 kB
 * SwapTotal:       3125240 kB
 * SwapFree:        3125240 kB
 * Dirty:                 4 kB
 * ......
 * 功能:从proc文件系统获取内存占用信息的相关数据
 * 参数集：
 *     无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32  mon_read_mem_file()
{
   U32 ulCount = 0; 
   S8  szLine[MAX_BUFF_LENGTH] = {0};
   S8* pszAnalyseRslt[3] = {0};
   U32 ulRet = 0;
   
   FILE * fp = fopen(m_szMemInfoFile, "r");
   if (DOS_ADDR_INVALID(fp))
   {
      logr_cirt("%s:line %u: mon_read_mem_file |file \'%s\' fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, m_szMemInfoFile, fp);
      goto failure;
   }

   fseek(fp, 0, SEEK_SET);

   while (!feof(fp))
   {
      if ( NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
      {
         S32 lRet = 0;
         U32 ulData = 0;
         ulRet = mon_analyse_by_reg_expr(szLine, " \t\n", pszAnalyseRslt, sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]));
         if(DOS_SUCC != ulRet)
         {
            logr_error("%s:Line %u:mon_read_mem_file|analyse failure!"
                        , dos_get_filename(__FILE__), __LINE__);
            goto failure;
         }
      
         lRet = dos_atoul(pszAnalyseRslt[1], &ulData);
         if(0 > lRet)
         {
            logr_error("%s:Line %u:mon_read_mem_file|dos_atol failure!"
                        , dos_get_filename(__FILE__), __LINE__);
            goto failure;
         }
         /* 第一列是内容，第二列为其值，以此类推 */
         if (0 == dos_strnicmp(pszAnalyseRslt[0], "MemTotal", dos_strlen("MemTotal")))
         {
            g_pstMem->ulPhysicalMemTotalBytes = ulData;
            ++ulCount;
            continue;
         }
         else if(0 == dos_strnicmp(pszAnalyseRslt[0], "MemFree", dos_strlen("MemFree")))
         {
            g_pstMem->ulPhysicalMemFreeBytes = ulData;
            ++ulCount;
            continue;
         }

         else if(0 == dos_strnicmp(pszAnalyseRslt[0], "SwapTotal", dos_strlen("SwapTotal")))
         {
            g_pstMem->ulSwapTotalBytes = ulData;
            ++ulCount;
            continue;
         }
         else if(0 == dos_strnicmp(pszAnalyseRslt[0], "SwapFree", dos_strlen("SwapFree")))
         {
            g_pstMem->ulSwapFreeBytes = ulData;
            ++ulCount;
            continue;
         }
         else if(0 == dos_strnicmp(pszAnalyseRslt[0], "Buffers", dos_strlen("Buffers")))
         {
            g_pstMem->ulBuffers = ulData;
            ++ulCount;
            continue;
         }
         else if (0 == dos_strnicmp(pszAnalyseRslt[0], "Cached", dos_strlen("Cached")))
         {
            g_pstMem->ulCached = ulData;
            ++ulCount;
            continue;
         }

         if (MEMBER_COUNT == ulCount)
         {
            ulRet = mon_get_mem_data();
            if(DOS_SUCC != ulRet)
            {
               logr_error("%s:Line %u:mon_read_mem_file|get memory data failure"
                           , dos_get_filename(__FILE__), __LINE__);
            }
            goto success;
         }
      }
   }

success:
   fclose(fp);
   fp = NULL;
   return DOS_SUCC;
failure:
   fclose(fp);
   fp = NULL;
   return DOS_FAIL;
}

/**
 * 功能:获取并计算g_pstMem的相关数据
 * 参数集：
 *     无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32  mon_get_mem_data()
{
   U32 ulPhyMemTotal = g_pstMem->ulPhysicalMemTotalBytes;
   U32 ulPhyMemFree  = g_pstMem->ulPhysicalMemFreeBytes;

   U32 ulBusyMem = ulPhyMemTotal - ulPhyMemFree;
   
   U32 ulSwapTotal   = g_pstMem->ulSwapTotalBytes;
   U32 ulSwapFree    = g_pstMem->ulSwapFreeBytes;

   U32 ulBusySwap = ulSwapTotal - ulSwapFree;

   /**
    * 内存占用率算法:
    *     如果高速缓存的大小大于RAM大小，那么被占用的就是: busy = total -free
    *     否则被占用的为: busy = total - free - buffers - cached
    */
   if (0 == ulPhyMemTotal)
   {
       g_pstMem->ulPhysicalMemUsageRate = 0;
   }
   else
   {
       ulBusyMem *= 100;
       g_pstMem->ulPhysicalMemUsageRate = (ulBusyMem + ulBusyMem % ulPhyMemTotal) / ulPhyMemTotal;
   }

   if (0 == ulSwapTotal)
   {
       g_pstMem->ulSwapUsageRate = 0;
   }
   else
   {
       ulSwapFree *= 100;
       g_pstMem->ulSwapUsageRate = (ulBusySwap + ulBusySwap % ulSwapTotal) / ulSwapTotal;
   }

   return DOS_SUCC;
}


/**
 * 功能:获取内存信息的格式化输出信息字符串
 * 参数集：
 *     无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32  mon_get_mem_formatted_info()
{
   if(DOS_ADDR_INVALID(g_pstMem))
   {
      logr_cirt("%s:Line %u:mon_get_mem_formatted_info|get memory formatted information failure, g_pstMem is %p!"
                , dos_get_filename(__FILE__), __LINE__, g_pstMem);
      return DOS_FAIL;
   }

   memset(g_szMonMemInfo, 0, MAX_BUFF_LENGTH);
   dos_snprintf(g_szMonMemInfo, MAX_BUFF_LENGTH, "  Physical Mem KBytes:%u\n    Physical Usage Rate:%u%%\n" \
                "   Total Swap KBytes:%u\n  Swap Usage Rate:%u%%\n"
                , g_pstMem->ulPhysicalMemTotalBytes
                , g_pstMem->ulPhysicalMemUsageRate
                , g_pstMem->ulSwapTotalBytes
                , g_pstMem->ulSwapUsageRate
           );
   logr_info("%s:Line %u:g_szMonMemInfo is\n%s", dos_get_filename(__FILE__)
                , __LINE__, g_szMonMemInfo);
                
   return DOS_SUCC;
}

#endif //end #if INCLUDE_MEM_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_HB_SERVER)

#ifdef __cplusplus
}
#endif


