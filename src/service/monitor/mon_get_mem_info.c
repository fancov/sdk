#ifdef __cplusplus
extern "C"{
#endif


#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_MEM_MONITOR

#include "mon_get_mem_info.h"
#include "mon_string.h"

static S8  m_szMemInfoFile[] = "/proc/meminfo";
extern S8  g_szMonMemInfo[MAX_BUFF_LENGTH];
extern MON_SYS_MEM_DATA_S * g_pstMem;


S32  mon_mem_malloc()
{
   g_pstMem = (MON_SYS_MEM_DATA_S *)dos_dmem_alloc(sizeof(MON_SYS_MEM_DATA_S));
   if (!g_pstMem)
   {
      logr_cirt("%s:Line %d: mon_mem_malloc|pstMem is %p!",
                __FILE__, __LINE__, g_pstMem);
      return DOS_FAIL;
   }
   
   memset(g_pstMem, 0, sizeof(MON_SYS_MEM_DATA_S));
   
   return DOS_SUCC;
}

S32 mon_mem_free()
{
   if (!g_pstMem)
   {
      logr_notice("%s:line %d: mon_mem_free|free memory failure,g_pstMem is %p!"
                    , __FILE__, __LINE__, g_pstMem);
      return DOS_FAIL;
   }
   
   dos_dmem_free(g_pstMem);
   g_pstMem = NULL;
   
   return DOS_SUCC;
}


S32  mon_read_mem_file()
{
   S32 lCount = 0; 
   S8  szLine[MAX_BUFF_LENGTH] = {0};
   S8* pszAnalyseRslt[3] = {0};
   U32 ulRet = 0;
   
   FILE * fp = fopen(m_szMemInfoFile, "r");
   if (!fp)
   {
      logr_cirt("%s:line %d: mon_get_memory_data |file \'%s\' fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, m_szMemInfoFile, fp);
      goto failure;
   }

   fseek(fp, 0, SEEK_SET);

   while (!feof(fp))
   {
      if ( NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
      {
         S32 lRet = 0;
         S32 lData = 0;
         ulRet = mon_analyse_by_reg_expr(szLine, " \t\n", pszAnalyseRslt, sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]));
         if(0 > ulRet)
         {
            logr_error("%s:Line %d:mon_get_memory_data|analyse failure!"
                        , dos_get_filename(__FILE__), __LINE__);
            goto failure;
         }

          lRet = dos_atol(pszAnalyseRslt[1], &lData);
          if(0 > lRet)
          {
             logr_error("%s:Line %d:mon_get_memory_data|dos_atol failure!"
                          , dos_get_filename(__FILE__), __LINE__);
             goto failure;
          }

          if (0 == dos_strnicmp(pszAnalyseRslt[0], "MemTotal", dos_strlen("MemTotal")))
          {
             g_pstMem->lPhysicalMemTotalBytes = lData;
             ++lCount;
             continue;
          }
          else if(0 == dos_strnicmp(pszAnalyseRslt[0], "MemFree", dos_strlen("MemFree")))
          {
             g_pstMem->lPhysicalMemFreeBytes = lData;
             ++lCount;
             continue;
          }
          else if(0 == dos_strnicmp(pszAnalyseRslt[0], "Buffers", dos_strlen("Buffers")))
          {
             g_pstMem->lBuffers = lData;
             ++lCount;
             continue;
          }
          else if(0 == dos_strnicmp(pszAnalyseRslt[0], "Cached", dos_strlen("Cached")))
          {
             g_pstMem->lCached = lData;
             ++lCount;
             continue;
          }
          else if(0 == dos_strnicmp(pszAnalyseRslt[0], "SwapTotal", dos_strlen("SwapTotal")))
          {
             g_pstMem->lSwapTotalBytes = lData;
             ++lCount;
             continue;
          }
          else if(0 == dos_strnicmp(pszAnalyseRslt[0], "SwapFree", dos_strlen("SwapFree")))
          {
             g_pstMem->lSwapFreeBytes = lData;
             ++lCount;
             continue;
          }

          if (MEMBER_COUNT == lCount)
          {
             lRet = mon_get_mem_data();
             if(DOS_SUCC != lRet)
             {
                logr_error("%s:Line %d:mon_get_memory_data|get memory data failure"
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

S32  mon_get_mem_data()
{
   S32 lCached      = g_pstMem->lCached;
   S32 lPhyMemTotal = g_pstMem->lPhysicalMemTotalBytes;
   S32 lPhyMemFree  = g_pstMem->lPhysicalMemFreeBytes;
   S32 lBuffers     = g_pstMem->lBuffers ;
   S32 lSwapTotal   = g_pstMem->lSwapTotalBytes;
   S32 lSwapFree    = g_pstMem->lSwapFreeBytes;

   S32 lBusyMem  = 0;
   S32 lSwapBusy = 0;

   if (lCached > lPhyMemTotal)
   {
      lBusyMem = 100 * (lPhyMemTotal - lPhyMemFree);
   }
   else
   {
      lBusyMem = 100 * (lPhyMemTotal - lPhyMemFree - lBuffers - lCached);
   }

   lSwapBusy = 100 * (lSwapTotal - lSwapFree);

   g_pstMem->lPhysicalMemUsageRate = (lBusyMem + lBusyMem % lPhyMemTotal) / lPhyMemTotal;
   g_pstMem->lSwapUsageRate = (lSwapBusy + lSwapBusy % lSwapTotal) / lSwapTotal;

   return DOS_SUCC;
}

S32  mon_get_mem_formatted_info()
{
   if(!g_pstMem)
   {
      logr_cirt("%s:Line %d:mon_get_mem_formatted_info|get memory formatted information failure, g_pstMem is %p!"
                , dos_get_filename(__FILE__), __LINE__, g_pstMem);
      return DOS_FAIL;
   }

   memset(g_szMonMemInfo, 0, MAX_BUFF_LENGTH);
   dos_snprintf(g_szMonMemInfo, MAX_BUFF_LENGTH, "Physical Mem KBytes:%d\nPhysical Usage Rate:%d%%\n" \
                "Swap partition KBytes:%d\nSwap partition Usage Rate:%d%%\n"
                , g_pstMem->lPhysicalMemTotalBytes
                , g_pstMem->lPhysicalMemUsageRate
                , g_pstMem->lSwapTotalBytes
                , g_pstMem->lSwapUsageRate);
   
   return DOS_SUCC;
}
#endif //end #if INCLUDE_MEM_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_HB_SERVER)

#ifdef __cplusplus
}
#endif


