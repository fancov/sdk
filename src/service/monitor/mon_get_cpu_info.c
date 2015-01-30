#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_CPU_MONITOR

#include "mon_string.h"
#include "mon_get_cpu_info.h"


struct tagMonCPUQueue //cpu队列
{
   MON_SYS_CPU_TIME_S *pstHead; //第一个节点
   MON_SYS_CPU_TIME_S *pstPenultimate; //倒数第2个节点
   MON_SYS_CPU_TIME_S *pstCountdownXII;//第12个节点
   MON_SYS_CPU_TIME_S *pstRear;//队列的尾节点
   S32                 lQueueLength; //队列长度
};

typedef struct tagMonCPUQueue MON_CPU_QUEUE_S;

static S8 m_szMonCPUInfoFile[]            = "/proc/stat";
static MON_CPU_QUEUE_S * m_pstCPUQueue; //CPU队列

extern S8 g_szMonCPUInfo[MAX_BUFF_LENGTH];
extern MON_CPU_RSLT_S * g_pstCpuRslt;

static S32  mon_cpu_malloc(MON_SYS_CPU_TIME_S ** ppstCpu);
static S32  mon_get_cpu_data(MON_SYS_CPU_TIME_S * pstCpu);
static S32  mon_cpu_en_queue(MON_SYS_CPU_TIME_S * pstCpu);
static S32  mon_cpu_de_queue();
static S32  mon_refresh_cpu_queue();


static S32  mon_cpu_malloc(MON_SYS_CPU_TIME_S ** ppstCpu)
{
   *ppstCpu = (MON_SYS_CPU_TIME_S *)dos_dmem_alloc(sizeof(MON_SYS_CPU_TIME_S));
   if(!*ppstCpu)
   {
      logr_error("%s:Line %d:mon_cpu_malloc| alloc memory failure!"
                    , dos_get_filename(__FILE__), __LINE__);
      return DOS_FAIL;
   }
   memset(*ppstCpu, 0, sizeof(MON_SYS_CPU_TIME_S));
   return DOS_SUCC;
}

S32 mon_cpu_rslt_malloc()
{
   g_pstCpuRslt = (MON_CPU_RSLT_S *)dos_dmem_alloc(sizeof(MON_CPU_RSLT_S));
   if(!g_pstCpuRslt)
   {
      logr_cirt("%s:Line %d:mon_cpu_rslt_struct_obj_alloc_memory |alloc memory failure,pstCpuRslt is %p!"
                , dos_get_filename(__FILE__), __LINE__, g_pstCpuRslt);
      return DOS_FAIL;
   }
   
   memset(g_pstCpuRslt, 0, sizeof(MON_CPU_RSLT_S));

   return DOS_SUCC;
}

S32 mon_cpu_rslt_free()
{
   if(!g_pstCpuRslt)
   {
      logr_warning("%s:Line %d:mon_cpu_rslt_struct_obj_free_memory|free memory failure,pstCpuRslt is %p!"
                    , dos_get_filename(__FILE__), __LINE__, g_pstCpuRslt);
      return DOS_FAIL;
   }
   
   dos_dmem_free(g_pstCpuRslt);
   g_pstCpuRslt = NULL;

   return DOS_SUCC;
}


static S32  mon_get_cpu_data(MON_SYS_CPU_TIME_S * pstCpu)
{
   FILE * fp;
   S8     szBuff[MAX_BUFF_LENGTH] = {0};
   S32    lRet = 0;
   S8*    pszAnalyseRslt[10] = {0};

   if (!pstCpu)
   {
      logr_cirt("%s:line %d:mon_get_cpu_struct_obj_data|get cpu data failure,pstCpu is %p!"
                , dos_get_filename(__FILE__), __LINE__, pstCpu);
      return DOS_FAIL;
   }

   fp = fopen(m_szMonCPUInfoFile, "r");
   if(!fp)
   {
      logr_cirt("%s:Line %d:mon_get_cpu_struct_obj_data|get cpu data failure,file \'%s\' fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, m_szMonCPUInfoFile, fp);
      return DOS_FAIL;
   }

   fseek(fp, 0, SEEK_SET);
   if (NULL != (fgets(szBuff, MAX_BUFF_LENGTH, fp)))
   {
      U32 ulRet = 0;
      U32 ulRows = 0;
      S32 lData = 0;
      
      ulRet = mon_analyse_by_reg_expr(szBuff, " \t\n", pszAnalyseRslt, sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]));
      if(DOS_SUCC != ulRet)
      {
         logr_error("%s:Line %d:mon_get_cpu_data|analyse sentence failure!"
                    , dos_get_filename(__FILE__), __LINE__);
         goto failure;
      }

      for(ulRows = 1; ulRows < sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]); ulRows++)
      {
         lRet = dos_atol(pszAnalyseRslt[ulRows], &lData);
         if(0 != lRet)
         {
            logr_error("%s:Line %d:mon_get_cpu_data|dos_atol failure!"
                        , dos_get_filename(__FILE__), __LINE__);
            goto failure;
         }
         
         switch(ulRows)
         {
           case 1:
              pstCpu->lUser        = lData;
              break;
           case 2:
              pstCpu->lNice        = lData;
              break;
           case 3:
              pstCpu->lSystem      = lData;
              break;
           case 4:
              pstCpu->lIdle        = lData;
              break;
           case 5:
              pstCpu->lIowait      = lData;
              break;
           case 6:
              pstCpu->lHardirq     = lData;
              break;
           case 7:
              pstCpu->lSoftirq     = lData;
              break;
           case 8:
              pstCpu->lStealstolen = lData;
              break;
           case 9:
              pstCpu->lGuest       = lData;
              break;
           default:
              break;
         }
      }
   }
   goto success;

success:
   fclose(fp);
   fp = NULL;
   return DOS_SUCC;
failure:
   fclose(fp);
   fp = NULL;
   return DOS_FAIL;
}

S32 mon_init_cpu_queue()
{
   S32 lRet = 0;
   
   m_pstCPUQueue = (MON_CPU_QUEUE_S *)dos_dmem_alloc(sizeof(MON_CPU_QUEUE_S));
   if (!m_pstCPUQueue)
   {
       logr_cirt("%s:line %d:mon_init_cpu_struct_obj_queue| init CPU queue failed,pstQueue is %p!"
                    , dos_get_filename(__FILE__),  __LINE__, m_pstCPUQueue);
       return DOS_FAIL;
   }
   
   memset(m_pstCPUQueue, 0, sizeof(MON_CPU_QUEUE_S));
   
   if(!(m_pstCPUQueue->pstHead))
   {
      MON_SYS_CPU_TIME_S * pstCpu = NULL;
      lRet = mon_cpu_malloc(&pstCpu);
      if(DOS_SUCC != lRet)
      {
         logr_cirt("%s:Line %d:mon_init_cpu_struct_obj_queue|alloc memory failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         return DOS_FAIL;
      }   
      m_pstCPUQueue->pstHead = pstCpu;
   }
   m_pstCPUQueue->lQueueLength = 1;

   m_pstCPUQueue->pstHead->next     = NULL;
   m_pstCPUQueue->pstHead->prior    = NULL;
   m_pstCPUQueue->pstPenultimate    = NULL;
   m_pstCPUQueue->pstCountdownXII   = NULL;
   m_pstCPUQueue->pstRear = m_pstCPUQueue->pstHead;

   while ((m_pstCPUQueue->lQueueLength) < TEN_MINUTE_SAMPLE_ARRAY)
   {    

      MON_SYS_CPU_TIME_S * pstCpu = NULL;
      lRet = mon_cpu_malloc(&pstCpu);
      if(DOS_SUCC != lRet)
      {
         logr_cirt("%s:Line %d:mon_init_cpu_struct_obj_queue|alloc memory failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         return DOS_FAIL;
      }
      
      lRet = mon_cpu_en_queue(pstCpu);
      if(DOS_SUCC != lRet)
      {
         logr_error("%s:Line %d:mon_init_cpu_struct_obj_queue|enter queue failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         return DOS_FAIL;
      }

      if (PENULTIMATE == m_pstCPUQueue->lQueueLength)
      {
         m_pstCPUQueue->pstPenultimate = m_pstCPUQueue->pstRear;
      }
      else if (COUNTDOWN_XII == m_pstCPUQueue->lQueueLength)
      {
         m_pstCPUQueue->pstCountdownXII = m_pstCPUQueue->pstRear;
      }

      logr_debug("%s:Line %d:mon_init_cpu_struct_obj_queue|m_pstCPUQueue->lQueueLength = %d!"
                    , dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue->lQueueLength);
   }

   return DOS_SUCC;
}

static S32  mon_cpu_en_queue(MON_SYS_CPU_TIME_S * pstCpu)
{//从队尾入队

      if (!m_pstCPUQueue)
      {
         logr_cirt("%s:Line %d: mon_cpu_obj_struct_en_queue|enter queue failure,m_pstCPUQueue is %p!"
                    , dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue);
         return DOS_FAIL;
      }
      if (!pstCpu)
      {
         logr_cirt("%s:Line %d: mon_cpu_obj_struct_en_queue|enter queue failure,pstCpu is %p!"
                    , dos_get_filename(__FILE__), __LINE__, pstCpu);
         return DOS_FAIL;
      }

      pstCpu->next = NULL;
      pstCpu->prior = NULL;

      m_pstCPUQueue->pstRear->next = pstCpu;
      pstCpu->prior = m_pstCPUQueue->pstRear;
      m_pstCPUQueue->pstRear = pstCpu;

      if (m_pstCPUQueue->pstPenultimate && m_pstCPUQueue->lQueueLength>= TEN_MINUTE_SAMPLE_ARRAY)
      {
         m_pstCPUQueue->pstPenultimate = m_pstCPUQueue->pstPenultimate->next;//第5指针往后移1
      }
      if (m_pstCPUQueue->pstCountdownXII && m_pstCPUQueue->lQueueLength >= TEN_MINUTE_SAMPLE_ARRAY)  
      {
         m_pstCPUQueue->pstCountdownXII = m_pstCPUQueue->pstCountdownXII->next;//第60指针往后移1
      }
 
      ++(m_pstCPUQueue->lQueueLength);

      return DOS_SUCC;
}


static S32  mon_cpu_de_queue()
{//从队头出队
   if(!m_pstCPUQueue)
   {
      logr_cirt("%s:line %d:mon_cpu_struct_obj_de_queue|delete queue failure,pstCPUQueue is %p!"
                , dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue);
      return DOS_FAIL;
   }
   
   if (!(m_pstCPUQueue->pstHead))
   {
      logr_cirt("%s:line %d:mon_cpu_struct_obj_de_queue|delete queue failure,pstCPUQueue->pstHead is %p!"
                ,dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue->pstHead);
      return DOS_FAIL;
   }
   
   MON_SYS_CPU_TIME_S * pTemp = m_pstCPUQueue->pstHead;
   m_pstCPUQueue->pstHead = m_pstCPUQueue->pstHead->next;
   m_pstCPUQueue->pstHead->prior = NULL;
   pTemp->next = NULL;
   dos_dmem_free(pTemp);
   
   --(m_pstCPUQueue->lQueueLength);

   return DOS_SUCC;
}

S32  mon_get_cpu_rslt_data()
{
   S32 lFirstTemp = 0, lSecondTemp = 0, lTwelfthTemp = 0, lLastTemp = 0;
   S32 lRet = 0;
   
   lFirstTemp = m_pstCPUQueue->pstHead->lUser
              + m_pstCPUQueue->pstHead->lNice
              + m_pstCPUQueue->pstHead->lSystem
              + m_pstCPUQueue->pstHead->lIdle
              + m_pstCPUQueue->pstHead->lIowait
              + m_pstCPUQueue->pstHead->lSoftirq
              + m_pstCPUQueue->pstHead->lHardirq
              + m_pstCPUQueue->pstHead->lStealstolen
              + m_pstCPUQueue->pstHead->lGuest;
                 
   lSecondTemp = m_pstCPUQueue->pstPenultimate->lUser
               + m_pstCPUQueue->pstPenultimate->lNice
               + m_pstCPUQueue->pstPenultimate->lSystem 
               + m_pstCPUQueue->pstPenultimate->lIdle
               + m_pstCPUQueue->pstPenultimate->lIowait
               + m_pstCPUQueue->pstPenultimate->lSoftirq
               + m_pstCPUQueue->pstPenultimate->lHardirq
               + m_pstCPUQueue->pstPenultimate->lStealstolen
               + m_pstCPUQueue->pstPenultimate->lGuest;
                 
   lTwelfthTemp = m_pstCPUQueue->pstCountdownXII->lUser
                + m_pstCPUQueue->pstCountdownXII->lNice
                + m_pstCPUQueue->pstCountdownXII->lSystem
                + m_pstCPUQueue->pstCountdownXII->lIdle
                + m_pstCPUQueue->pstCountdownXII->lIowait
                + m_pstCPUQueue->pstCountdownXII->lSoftirq
                + m_pstCPUQueue->pstCountdownXII->lHardirq
                + m_pstCPUQueue->pstCountdownXII->lStealstolen
                + m_pstCPUQueue->pstCountdownXII->lGuest;
                    
   lLastTemp = m_pstCPUQueue->pstRear->lUser 
             + m_pstCPUQueue->pstRear->lNice
             + m_pstCPUQueue->pstRear->lSystem
             + m_pstCPUQueue->pstRear->lIdle
             + m_pstCPUQueue->pstRear->lIowait 
             + m_pstCPUQueue->pstRear->lSoftirq
             + m_pstCPUQueue->pstRear->lHardirq
             + m_pstCPUQueue->pstRear->lStealstolen
             + m_pstCPUQueue->pstRear->lGuest;
             
   if(0 == m_pstCPUQueue->pstRear->lUser)
   {
      g_pstCpuRslt->lCPUUsageRate = 0;
   }
   else
   {
      S32 lIdle = m_pstCPUQueue->pstRear->lIdle;
      lIdle *= 100;
      g_pstCpuRslt->lCPUUsageRate = 100 - (lIdle + lIdle % lLastTemp) / lLastTemp;
   }

   if(lFirstTemp == lLastTemp)
   {
      g_pstCpuRslt->lCPUUsageRate = 0;
      g_pstCpuRslt->lCPU1minUsageRate = 0;
      g_pstCpuRslt->lCPU10minUsageRate = 0;
      
      lRet = mon_refresh_cpu_queue();
      if(DOS_SUCC != lRet)
      {
         logr_error("%s:Line %d:mon_get_cpu_rslt_struct_obj_data|refresh cpu queue failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         return DOS_FAIL;
      }
   
      return DOS_SUCC;
   }
   else
   {
      S32 lIdle = m_pstCPUQueue->pstRear->lIdle - m_pstCPUQueue->pstHead->lIdle;
      S32 lTemp = lLastTemp - lFirstTemp;
      lIdle *= 100; 
      
      g_pstCpuRslt->lCPU10minUsageRate = 100 - (lIdle + lIdle % lTemp) / lTemp;
   }

   if(lLastTemp == lTwelfthTemp)
   {
      g_pstCpuRslt->lCPU1minUsageRate = 0;
      g_pstCpuRslt->lCPU5sUsageRate = 0;
      
      lRet = mon_refresh_cpu_queue();
      if(DOS_SUCC != lRet)
      {
         logr_error("%s:Line %d:mon_get_cpu_rslt_struct_obj_data|refresh cpu queue failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         return DOS_FAIL;
      }
      
      return DOS_SUCC;
   }
   else
   {
      S32 lIdle = m_pstCPUQueue->pstRear->lIdle - m_pstCPUQueue->pstCountdownXII->lIdle;
      S32 lTemp = lLastTemp - lTwelfthTemp;
      lIdle *= 100;
      g_pstCpuRslt->lCPU1minUsageRate = 100 - (lIdle + lIdle % lTemp) / lTemp;
   }

   if(lSecondTemp == lLastTemp)
   {
      g_pstCpuRslt->lCPU5sUsageRate = 0;
      
      lRet = mon_refresh_cpu_queue();
      if(DOS_SUCC != lRet)
      {
         logr_error("%s:Line %d:mon_get_cpu_rslt_struct_obj_data|refresh cpu queue failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         return DOS_FAIL;
      }
      
      return DOS_SUCC;
   }
   else
   {
      S32 lIdle = m_pstCPUQueue->pstRear->lIdle - m_pstCPUQueue->pstPenultimate->lIdle;
      S32 lTemp = lLastTemp - lSecondTemp;
      lIdle *= 100;
      g_pstCpuRslt->lCPU5sUsageRate = (lIdle + lIdle % lTemp) / lTemp;
   }

   lRet = mon_refresh_cpu_queue();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_get_cpu_rslt_struct_obj_data|refresh cpu queue failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }
   
   return DOS_SUCC;
}


static S32  mon_refresh_cpu_queue()
{
   S32 lRet = 0;
   
   MON_SYS_CPU_TIME_S * pstCpu = NULL;
   lRet = mon_cpu_malloc(&pstCpu);
   if(DOS_SUCC != lRet)
   {
      logr_cirt("%s:Line %d:mon_refresh_cpu_struct_obj_queue|resfresh queue failure,lRet is %d!"
                , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }

   if(!m_pstCPUQueue)
   {
      logr_cirt("%s:Line %d:mon_refresh_cpu_struct_obj_queue|refresh queue failure,m_pstCPUQueue is %p!"
                , dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue);
      return DOS_FAIL;
   }

   lRet = mon_get_cpu_data(pstCpu);
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_refresh_cpu_struct_obj_queue|get cpu data failure,lRet == %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }
   
   lRet = mon_cpu_en_queue(pstCpu);
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_refresh_cpu_struct_obj_queue|lRet == %d!", __FILE__, __LINE__, lRet);
      return DOS_FAIL;
   }
   
   lRet = mon_cpu_de_queue();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_refresh_cpu_struct_obj_queue|delete queue failure,lRet == %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }

   return DOS_SUCC;
}

S32  mon_get_cpu_rslt_formatted_info()
{
   
   memset(g_szMonCPUInfo, 0, MAX_BUFF_LENGTH);
   
   dos_snprintf(g_szMonCPUInfo, MAX_BUFF_LENGTH, 
         "Total CPU Average Usage Rate:%d%%.\n" \
         "5 seconds CPU Average Usage Rate:%d%%.\n" \
         "1min CPU Average Usage Rate:%d%%.\n" \
         "10min CPU Average Usage Rate:%d%%.\n",
         g_pstCpuRslt->lCPUUsageRate,
         g_pstCpuRslt->lCPU5sUsageRate,
         g_pstCpuRslt->lCPU1minUsageRate,
         g_pstCpuRslt->lCPU10minUsageRate);
   
   return DOS_SUCC;
}

S32  mon_cpu_queue_destroy()
{
   MON_SYS_CPU_TIME_S * pstCpu = NULL;
   if(!m_pstCPUQueue)
   {
      logr_cirt("%s:Line %d:mon_cpu_struct_obj_queue_destroy|destory queue failure,m_pstCPUQueue is %p!"
                , dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue);
      return DOS_FAIL;
   }

   if(!(m_pstCPUQueue->pstHead))
   {
      logr_cirt("%s:Line %d: mon_cpu_struct_obj_queue_destroy|destroy queue failure,m_pstCPUQueue->pstHead is %p!"
                , dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue->pstHead);
                  
      dos_dmem_free(m_pstCPUQueue);
      m_pstCPUQueue = NULL;
      
      return DOS_FAIL;
   }

   pstCpu = m_pstCPUQueue->pstHead;
   while(pstCpu)
   {
      MON_SYS_CPU_TIME_S * pstCpuTemp = pstCpu;
      pstCpu = pstCpu->next;
      pstCpuTemp->next = NULL;
      pstCpu->prior = NULL;

      dos_dmem_free(pstCpuTemp);
      pstCpuTemp = NULL;
   }

   dos_dmem_free(m_pstCPUQueue);
   m_pstCPUQueue = NULL;

   return DOS_SUCC;
}

#endif //end #if INCLUDE_CPU_MONITOR
#endif //#if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

