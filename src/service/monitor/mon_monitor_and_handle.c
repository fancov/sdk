#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include <pthread.h>
#include <mysql/mysql.h> //sql
#include <time.h>  //sql
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dos/dos_config.h>
#include "../util/config/config_api.h"
#include "mon_string.h"
#include "mon_notification.h"
#include "mon_get_mem_info.h"
#include "mon_get_cpu_info.h"
#include "mon_get_disk_info.h"
#include "mon_get_net_info.h"
#include "mon_get_proc_info.h"
#include "mon_monitor_and_handle.h"
#include "mon_warning_msg_queue.h"

pthread_mutex_t g_stMonMutex  = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  g_stMonCond   = PTHREAD_COND_INITIALIZER;

MON_SYS_MEM_DATA_S *   g_pstMem = NULL;
MON_CPU_RSLT_S*        g_pstCpuRslt = NULL;
MON_SYS_PART_DATA_S *  g_pastPartition[MAX_PARTITION_COUNT] = {0};
MON_NET_CARD_PARAM_S * g_pastNet[MAX_NETCARD_CNT] = {0};
MON_PROC_STATUS_S *   g_pastProc[MAX_PROC_CNT] = {0};

S32  g_lPartCnt = 0;
S32  g_lNetCnt  = 0;
S32  g_lPidCnt  = 0;

S8 g_szMonMemInfo[MAX_BUFF_LENGTH]     = {0};
S8 g_szMonCPUInfo[MAX_BUFF_LENGTH]     = {0};
S8 g_szMonDiskInfo[MAX_PARTITION_COUNT * MAX_BUFF_LENGTH]    = {0};
S8 g_szMonNetworkInfo[MAX_NETCARD_CNT * MAX_BUFF_LENGTH] = {0};
S8 g_szMonProcessInfo[MAX_PROC_CNT * MAX_BUFF_LENGTH] = {0};


static MON_MSG_QUEUE_S *      g_pstMsgQueue = NULL;//消息队列
static S8                     szIPv4Address[] = "localhost";
static MYSQL*                 g_pstConn = NULL;
static MON_THRESHOLD_S *      g_pstCond = NULL;

MYSQL  g_stMySQL;


static S32 mon_get_res_info();
static S32 mon_handle_excp();
static S32 mon_add_data_to_db();
static S32 mon_print_data_log();
static S32 mon_reset_res_data();

static S32 mon_add_warning_record(U32 ulResId);
static S32 mon_init_mysql_conn();
static S32 mon_init_warning_cond();
static S32 mon_close_mysql_conn();

VOID *mon_res_monitor(VOID *p)
{   
   while (1)
   { 
      S32 lRet = 0;
      sleep(5);

      pthread_mutex_lock(&g_stMonMutex); 
      /*  获取资源信息  */
      lRet = mon_get_res_info();
      while(DOS_SUCC != lRet)
      {
         logr_error("%s:Line %d:mon_res_monitor|get resource info failure,lRet is %d!"  
                    , dos_get_filename(__FILE__), __LINE__, lRet); 
         lRet = mon_get_res_info();
         continue;
      }
      
      /*  异常处理  */
      lRet = mon_handle_excp();
      while(DOS_SUCC != lRet)
      {
         logr_error("%s:Line %d:mon_res_monitor|handle exception failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         lRet = mon_handle_excp();
         continue;
      }
      
      /*  将数据记录至数据库  */
      lRet = mon_add_data_to_db();
      while(DOS_SUCC != lRet)
      {
         logr_error("%s:Line %d:mon_res_monitor|add record to database failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         lRet = mon_add_data_to_db();
         continue;
      }
 
      
      /*  打印数据日志  */
      lRet = mon_print_data_log();
      while(DOS_SUCC != lRet)
      {
         logr_error("%s:Line %d:mon_res_monitor|print data log failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         lRet = mon_print_data_log();
         continue;
      }
   
      pthread_cond_signal(&g_stMonCond);
      pthread_mutex_unlock(&g_stMonMutex);
   }
}

VOID* mon_warning_handle(VOID *p)
{
     S32 lRet = 0;
     
     g_pstMsgQueue =  mon_get_warning_msg_queue();
     if(!g_pstMsgQueue)
     {
        logr_cirt("%s:Line %d:mon_warning_handle|get warning msg failure,g_pstMsgQueue is %p!"
                    , dos_get_filename(__FILE__), __LINE__, g_pstMsgQueue);
        return NULL;
     }
     
     while (1)
     {
        pthread_mutex_lock(&g_stMonMutex);
        pthread_cond_wait(&g_stMonCond, &g_stMonMutex);
        while (1)
        {
          if (DOS_TRUE == mon_is_warning_msg_queue_empty())
          {
             break;
          }
          switch (g_pstMsgQueue->pstHead->ulWarningId & (U32)0xff000000)
          {
            case 0xf1000000: //CPU过大处理
               
               break;
            case 0xf2000000: //内存过大处理
               break;
            case 0xf3000000: //磁盘过大处理
               break;
            case 0xf4000000: //网络异常处理
               break;
            case 0xf5000000: //进程异常处理
               {
                  if(DOS_SUCC != lRet)
                  {
                     logr_error("%s:Line %d:mon_warning_handle|kill all monitor failure,lRet is %d!"
                                , dos_get_filename(__FILE__), __LINE__, lRet);
                  }
               }
               break;
            default:
               logr_error("%s:Line %d:g_pstMsgQueue->pstHead->ulWarningId is %s%x"
                            , dos_get_filename(__FILE__), __LINE__, "0x"
                            , g_pstMsgQueue->pstHead->ulWarningId);
               break;
          }
          
         lRet = mon_warning_msg_de_queue(g_pstMsgQueue);
         if(DOS_SUCC != lRet)
         {
            logr_error("%s:Line %d:mon_warning_handle|delete warning msg queue failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
            break;
         }
       }

             /*  数据重置  */
       lRet = mon_reset_res_data();
       if(DOS_SUCC != lRet)
       {
          logr_error("%s:Line %d:mon_res_monitor|reset resource data failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
       }
       pthread_mutex_unlock(&g_stMonMutex);
   }   
}

S32 mon_res_generate()
{
   /*初始化队列与数据库*/
   S32 lRet = 0;

   lRet = mon_init_cpu_queue();
   while(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_generate|init cpu queue failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      mon_destroy_warning_msg_queue();
      lRet = mon_init_cpu_queue();
      continue;
   }

   lRet = mon_init_warning_msg_queue();
   while(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_generate|init msg queue failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      mon_destroy_warning_msg_queue();
      lRet = mon_init_warning_msg_queue();
      continue;
   }

   lRet = mon_init_warning_cond();
   while(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_generate|init msg queue failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      lRet = mon_init_warning_cond();
      continue;
   }

   lRet = mon_init_mysql_conn();
   while(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_generate|init mysql connection failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      lRet = mon_init_mysql_conn();
      continue;
   }

   lRet = mon_init_str_array();
   while(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_generate|init string array failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      lRet = mon_init_str_array();
      continue;
   }

   lRet = mon_init_str_array();
   while(0 != lRet)
   {
      logr_error("%s:Line %d:mon_res_generate|init heartbeat config failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      lRet = mon_init_str_array();
      continue;
   }

    /*  分配资源 */
   lRet = mon_mem_malloc();
   while(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_generate|mem alloc memory failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      lRet = mon_mem_malloc();
      continue;
   }
   
   lRet = mon_cpu_rslt_malloc();
   while(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_generate|cpu result alloc memory failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      lRet = mon_cpu_rslt_malloc();
      continue;
   }

   lRet = mon_disk_malloc();
   while(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_generate|disk alloc memory failure,lRet is %d!"
                , dos_get_filename(__FILE__), __LINE__, lRet);
      lRet = mon_disk_malloc();
      continue;
   }

   lRet = mon_netcard_malloc();
   while(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_generate|net alloc memory failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      lRet = mon_netcard_malloc();
      continue;
   }

   lRet = mon_proc_malloc();
   while(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_generate|proc alloc memory failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      lRet = mon_proc_malloc();
      continue;
   }

   return DOS_SUCC;
}

static S32 mon_get_res_info()
{
    S32 lRet = 0;

    lRet = mon_read_mem_file();
    while(DOS_SUCC != lRet)
    {
       logr_error("%s:Line %d:mon_get_res_info|get memory data failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
       lRet = mon_read_mem_file();
       continue;
    }

    lRet = mon_get_cpu_rslt_data();
    while(DOS_SUCC != lRet)
    {
       logr_error("%s:Line %d:mon_get_res_info|get cpu result data success,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
       lRet = mon_get_cpu_rslt_data();
       continue;
    }

    lRet = mon_get_partition_data();
    while(DOS_SUCC != lRet)
    {
       logr_error("%s:Line %d:mon_get_res_info|get partition data failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
       lRet = mon_get_partition_data();
       continue;
    }

    lRet = mon_get_netcard_data();
    while(DOS_SUCC != lRet)
    {
       logr_error("%s:Line %d:mon_get_res_info|get netcard data failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
       lRet = mon_get_netcard_data();
       continue;
    }

    lRet = mon_get_process_data();
    while(DOS_SUCC != lRet)
    {
       logr_error("%s:Line %d:mon_get_res_info|get process data success,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
       lRet = mon_get_process_data();
       continue;
    }
    
    return DOS_SUCC;
}


static S32 mon_handle_excp()
{
    S32 lRet = 0;
    U32 ulRet = 0;
    S32 lRows = 0;
    S32 lTotalDiskRate = 0;
    
    /*  异常处理  */
    if (g_pstMem->lPhysicalMemUsageRate >= g_pstCond->lMemThreshold)
    {
       MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
       while(!pstMsg)
       {
          logr_error("%s:Line %d: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                        , dos_get_filename(__FILE__), __LINE__, pstMsg);
          pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
          continue;
       }

       ulRet = mon_generate_warning_id(MEM_RES, 0x00, RES_LACK);
       if((U32)0xff == ulRet)
       {
          logr_error("%s:Line %d:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                        , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
       }
       pstMsg->ulWarningId = ulRet;
       
       pstMsg->lMsgLen = sizeof(MON_SYS_MEM_DATA_S);
       pstMsg->msg = g_pstMem;
       
       lRet = mon_add_warning_record(pstMsg->ulWarningId);
       if(DOS_SUCC != lRet)
       {
          logr_error("%s:Line %d:mon_handle_excp|add warning record failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
       }
       
       lRet = mon_warning_msg_en_queue(pstMsg);
       if(DOS_SUCC != lRet)
       {
          logr_error("%s:Line %d:mon_handle_excp|warning msg enter queue failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
       }
    }

    for(lRows = 0; lRows < g_lPartCnt; lRows++)
    {
       if(g_pastPartition[lRows]->lPartitionUsageRate >= g_pstCond->lPartitionThreshold)
       {
          
          MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
          while(!pstMsg)
          {
             logr_error("%s:Line %d: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                          , dos_get_filename(__FILE__), __LINE__, pstMsg);
             pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
             continue;
          }
          

          ulRet = mon_generate_warning_id(DISK_RES, lRows, RES_LACK);
          if((U32)0xff == ulRet)
          {
             logr_error("%s:Line %d:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                        , dos_get_filename(__FILE__), __LINE__ , "0x", ulRet);
          }
          
          pstMsg->ulWarningId = ulRet;
          pstMsg->lMsgLen = sizeof(MON_SYS_MEM_DATA_S);
          pstMsg->msg = g_pastPartition[lRows];
          
          lRet = mon_add_warning_record(pstMsg->ulWarningId);
          if(DOS_SUCC != lRet)
          {
             logr_error("%s:Line %d:mon_handle_excp|add warning record failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
          }
          
          lRet = mon_warning_msg_en_queue(pstMsg);
          if(DOS_SUCC != lRet)
          {
             logr_error("%s:Line %d:mon_handle_excp|warning msg enter queue failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
          }
          
       }
    }

    lTotalDiskRate = mon_get_total_disk_usage_rate();
    if(-1 == lTotalDiskRate)
    {
       logr_error("%s:Line %d:mon_handle_excp|get total disk usage rate failure,lTotalDiskRate is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lTotalDiskRate);
    }
              
    if(lTotalDiskRate >= g_pstCond->lDiskThreshold)
    {
       
       MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
       while(!pstMsg)
       {
            logr_error("%s:Line %d: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                         , dos_get_filename(__FILE__), __LINE__, pstMsg);
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            continue;
       }
       

       ulRet = mon_generate_warning_id(DISK_RES, 0x00, RES_LACK);
       if((U32)0xff == ulRet)
       {
          logr_error("%s:Line %d:mon_handle_excp|generate warning id failure,lRet is %s%x!"
                        , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
       }
                 
       pstMsg->ulWarningId = ulRet;
       pstMsg->lMsgLen = sizeof(MON_SYS_PART_DATA_S);
       pstMsg->msg = g_pastPartition;
       
       lRet = mon_add_warning_record(pstMsg->ulWarningId);
       if(DOS_SUCC != lRet)
       {
          logr_error("%s:Line %d:mon_handle_excp|add warning record failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
       }
       
       lRet = mon_warning_msg_en_queue(pstMsg);
       if(DOS_SUCC != lRet)
       {
          logr_error("%s:Line %d:mon_handle_excp|warning msg enter queue failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
       }
    }
   
    if(g_pstCpuRslt->lCPUUsageRate >= g_pstCond->lCPUThreshold||
       g_pstCpuRslt->lCPU5sUsageRate >= g_pstCond->l5sCPUThreshold ||
       g_pstCpuRslt->lCPU1minUsageRate >= g_pstCond->l1minCPUThreshold ||
       g_pstCpuRslt->lCPU10minUsageRate >= g_pstCond->l10minCPUThreshold)
    {
       
       MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
       while(!pstMsg)
       {
           logr_error("%s:Line %d: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                        , dos_get_filename(__FILE__), __LINE__, pstMsg);
           pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
           continue;
       }
       
       ulRet = mon_generate_warning_id(CPU_RES, 0x00, RES_LACK);
       if((U32)0xff == ulRet)
       {
          logr_error("%s:Line %d:mon_handle_excp|generate warning id failure,ulRet is %s%x"
                        , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
       }
       pstMsg->ulWarningId = ulRet;
       
       pstMsg->lMsgLen = sizeof(MON_CPU_RSLT_S);
       pstMsg->msg = g_pstCpuRslt;
       
       lRet = mon_add_warning_record(pstMsg->ulWarningId);
       if(DOS_SUCC != lRet)
       {
          logr_error("%s:Line %d:mon_handle_excp|add warning record failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
       }
       
       lRet = mon_warning_msg_en_queue(pstMsg);
       if(DOS_SUCC != lRet)
       {
          logr_error("%s:Line %d:mon_handle_excp|warning msg enter queue failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
       }
    }

    for(lRows = 0; lRows < g_lNetCnt; lRows++)
    {
       if(DOS_FALSE == (mon_is_netcard_connected(g_pastNet[lRows]->szNetDevName)))
       {
          
          MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
          while(!pstMsg)
          {
              logr_error("%s:Line %d: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                           , dos_get_filename(__FILE__), __LINE__, pstMsg);
              pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
              continue;
          }          

          ulRet = mon_generate_warning_id(NET_RES, 0x00, 0x00);
          if((U32)0xff == ulRet)
          {
             logr_error("%s:Line %d:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                        , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
          }
          
          pstMsg->ulWarningId = ulRet;
          pstMsg->lMsgLen = sizeof(MON_NET_CARD_PARAM_S);
          pstMsg->msg = g_pastNet[lRows];
          
          lRet = mon_add_warning_record(pstMsg->ulWarningId);
          if(DOS_SUCC != lRet)
          {
             logr_error("%s:Line %d:mon_handle_excp|add warning record failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
          }
          
          lRet = mon_warning_msg_en_queue(pstMsg);
          if(DOS_SUCC != lRet)
          {
             logr_error("%s:Line %d:mon_handle_excp|warning msg enter queue failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
          }
       }
    }

    lRet = mon_get_proc_total_cpu_rate();
    if(-1 == lRet)
    {
       logr_error("%s:Line %d:mon_handle_excp|get all proc total cpu rate failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
    }
              
    if(lRet > g_pstCond->lProcCPUThreshold)
    {
        
        MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
        while(!pstMsg)
        {
           logr_error("%s:Line %d: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                          , dos_get_filename(__FILE__), __LINE__, pstMsg);
           pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
                  continue;
        }
        
        ulRet = mon_generate_warning_id(PROC_RES, 0x00, 0x01);
        if((U32)0xff == ulRet)
        {
          logr_error("%s:Line %d:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                        , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
        }
        
        pstMsg->ulWarningId = ulRet;
        pstMsg->lMsgLen = sizeof(MON_PROC_STATUS_S);
        pstMsg->msg = g_pastProc;
        
        lRet = mon_add_warning_record(pstMsg->ulWarningId);
        if(DOS_SUCC != lRet)
        {
           logr_error("%s:Line %d:mon_handle_excp|add warning record failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
        }
        
        lRet = mon_warning_msg_en_queue(pstMsg);
        if(DOS_SUCC != lRet)
        {
           logr_error("%s:Line %d:mon_handle_excp|warning msg enter queue failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
        }
        
    }

    lRet = mon_get_proc_total_mem_rate();
    if(-1 == lRet)
    {
       logr_error("%s:Line %d:mon_handle_excp|get all proc total memory usage rate failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
    }

    if(lRet >= g_pstCond->lProcMemThreshold)
    {
       
       MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
       while(!pstMsg)
       {
           logr_error("%s:Line %d: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                         , dos_get_filename(__FILE__), __LINE__, pstMsg);
           pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
           continue;
       }
       
       ulRet = mon_generate_warning_id(PROC_RES, 0x00, 0x02);
       if((U32)0xff == ulRet)
       {
         logr_error("%s:Line %d:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                    , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
       }
       
       pstMsg->ulWarningId = ulRet;
       
       pstMsg->lMsgLen = sizeof(MON_PROC_STATUS_S);
       pstMsg->msg = g_pastProc;
       
       lRet = mon_add_warning_record(pstMsg->ulWarningId);
       if(DOS_SUCC != lRet)
       {
         logr_error("%s:Line %d:mon_handle_excp|add warning record failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
       }
       
       lRet = mon_warning_msg_en_queue(pstMsg);
       if(DOS_SUCC != lRet)
       {
         logr_error("%s:Line %d:mon_handle_excp|warning msg enter queue failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
       }
    }

    return DOS_SUCC;
}

static S32 mon_add_data_to_db()
{
   time_t lCur;
   struct tm *pstCurTime;
   S8  szSQLCmd[SQL_CMD_MAX_LENGTH] = {0};
   S32 lRet = 0;
   S32 lTotalDiskKBytes = 0;
   S32 lTotalDiskRate = 0;
   S32 lProcTotalMemRate = 0;
   S32 lProcTotalCPURate = 0;

   lTotalDiskKBytes = mon_get_total_disk_kbytes();
   if(-1 == lTotalDiskKBytes)
   {
      logr_error("%s:Line %d:mon_add_data_to_db|get total disk kbytes failure,lTotalDiskKBytes is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lTotalDiskKBytes);
      return DOS_FAIL;
   }
   
   lTotalDiskRate = mon_get_total_disk_usage_rate();
   if(-1 == lTotalDiskRate)
   {
      logr_error("%s:Line %d:mon_add_data_to_db|get total disk uasge rate failure,lTotalDiskRate is %d!"
                    ,dos_get_filename(__FILE__), __LINE__ , lTotalDiskRate);
      return DOS_FAIL;
   }

   lProcTotalMemRate = mon_get_proc_total_mem_rate();
   if(-1 == lProcTotalMemRate)
   {
      logr_error("%s:Line %d:mon_add_data_to_db|get all proc total mem rate failure,lProcTotalMemRate is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lProcTotalMemRate);
       return DOS_FAIL;
   }

   lProcTotalCPURate = mon_get_proc_total_cpu_rate();
   if(-1 == lProcTotalCPURate)
   {
      logr_error("%s:Line %d:mon_add_data_to_db|get all proc total cpu rate failure,lProcTotalCPURate is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lProcTotalCPURate);
      return DOS_FAIL;
   }

   time(&lCur);
   pstCurTime = localtime(&lCur);

   dos_snprintf(szSQLCmd, MAX_BUFF_LENGTH, "insert into tbl_syssrc%04d%02d(datetime,phymem," \
     "phymem_pct,swap,swap_pct,hd,hd_pct,cpu_pct,5scpu_pct,1mcpu_pct,10mcpu_pct,trans_rate," \
     "procmem_pct,proccpu_pct) values(\'%04d-%02d-%02d %02d:%02d:%02d\',%d,%d," \
     "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)",
     pstCurTime->tm_year + 1900
     , pstCurTime->tm_mon + 1
     , pstCurTime->tm_year + 1900
     , pstCurTime->tm_mon + 1
     , pstCurTime->tm_mday
     , pstCurTime->tm_hour
     , pstCurTime->tm_min
     , pstCurTime->tm_sec
     , g_pstMem->lPhysicalMemTotalBytes
     , g_pstMem->lPhysicalMemUsageRate
     , g_pstMem->lSwapTotalBytes
     , g_pstMem->lSwapUsageRate
     , lTotalDiskKBytes
     , lTotalDiskRate
     , g_pstCpuRslt->lCPUUsageRate
     , g_pstCpuRslt->lCPU5sUsageRate
     , g_pstCpuRslt->lCPU1minUsageRate
     , g_pstCpuRslt->lCPU10minUsageRate
     , g_pastNet[0]->lRWSpeed
     , lProcTotalMemRate
     , lProcTotalCPURate
   );
   
   lRet = mysql_query(&g_stMySQL, szSQLCmd);
   if(0 != lRet)
   {
      logr_error("%s:Line %d:mon_add_data_to_db|sql:%s\nexecute failure, lRet is %d!"
                    ,dos_get_filename(__FILE__), __LINE__, szSQLCmd, lRet);
   }
             
   return DOS_SUCC;
}

static S32 mon_print_data_log()
{
   S32 lRet = 0;
   /*    打印数据日志  */
   
   lRet = mon_get_mem_formatted_info();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_print_data_log|get mem formatted information failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }
   logr_info("%s:Line %d:mon_print_data_log|g_szMonMemInfo is \n%s"
                , dos_get_filename(__FILE__), __LINE__
                , g_szMonMemInfo);

   lRet = mon_get_cpu_rslt_formatted_info();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_print_data_log|get cpu rslt formatted information failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }
   logr_info("%s:Line %d:mon_print_data_log|g_szMonCPUInfo is \n%s"
                , dos_get_filename(__FILE__), __LINE__
                , g_szMonCPUInfo);

   lRet = mon_get_partition_formatted_info();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_print_data_log|get partition formatted information failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }
   logr_info("%s:Line %d:mon_print_data_log|g_szMonDiskInfo is \n%s"
                , dos_get_filename(__FILE__), __LINE__
                , g_szMonDiskInfo);

   lRet = mon_netcard_formatted_info();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_print_data_log|get netcard formatted information failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }
   logr_info("%s:Line %d:mon_print_data_log|g_szMonNetworkInfo is \n%s"
                , dos_get_filename(__FILE__), __LINE__
                , g_szMonNetworkInfo);

   lRet = mon_get_process_formatted_info();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_print_data_log|get process formatted information failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }
   logr_info("%s:Line %d:mon_print_data_log|g_szMonProcessInfo is\n%s"
                , dos_get_filename(__FILE__), __LINE__
                , g_szMonProcessInfo);
   
   return DOS_SUCC;
}


static S32 mon_reset_res_data()
{
   MON_SYS_PART_DATA_S *  pastDisk = g_pastPartition[0];
   MON_NET_CARD_PARAM_S*  pastNet  = g_pastNet[0];
   MON_PROC_STATUS_S *    pastProc = g_pastProc[0];

   if(!pastDisk)
   {
      logr_error("%s:Line %d:mon_reset_res_data|pastDisk is %p!"
                    , dos_get_filename(__FILE__), __LINE__
                    , pastDisk);
      return DOS_FAIL;
   }
   if(!pastNet)
   {
      logr_error("%s:Line %d:mon_reset_res_data|pastNet is %p!"
                    , dos_get_filename(__FILE__), __LINE__
                    , pastNet);
      return DOS_FAIL;
   }
   if(!pastProc)
   {
      logr_error("%s:Line %d:mon_reset_res_data|pastProc is %p!"
                    , dos_get_filename(__FILE__), __LINE__
                    , pastProc);
      return DOS_FAIL;
   }
   
   memset(g_pstMem, 0, sizeof(MON_SYS_MEM_DATA_S));
   memset(g_pstCpuRslt, 0, sizeof(MON_CPU_RSLT_S));
   memset(pastDisk, 0, MAX_PARTITION_COUNT * sizeof(MON_SYS_PART_DATA_S));
   memset(pastNet, 0, MAX_NETCARD_CNT * sizeof(MON_NET_CARD_PARAM_S));
   memset(pastProc, 0, MAX_PROC_CNT * sizeof(MON_PROC_STATUS_S));

   return DOS_SUCC;
}


static S32 mon_add_warning_record(U32 ulResId)
{
   S32 lRet = 0;
   time_t lCur;
   struct tm *pstCurTime;
   
   S8 szSQLCmd[SQL_CMD_MAX_LENGTH] = {0};
   S8 szSmpDesc[MAX_DESC_LENGTH] = {0};
  
   switch (ulResId & (U32)0xff000000)
   {   
      case 0xf1000000:
         dos_strcpy(szSmpDesc, "CPU res lack");
         break;
      case 0xf2000000:
         dos_strcpy(szSmpDesc, "Memory res lack");
         break;
      case 0xf3000000:
         dos_strcpy(szSmpDesc, "Disk res lack");
         break;
      case 0xf4000000:
         dos_strcpy(szSmpDesc, "Network disconnected");
         break;
      case 0xf5000000:
         dos_strcpy(szSmpDesc, "Process cpu or memory failed");
         break;
      default:
         break;
   }

   time(&lCur);
   pstCurTime = localtime(&lCur);
   
   dos_snprintf(szSQLCmd, SQL_CMD_MAX_LENGTH, "insert into mon_warning_db(warningId," \
               "time,warning_cause,warning_type,warning_obj,warning_level,simple_desc," \
               "warning_cycle) values(concat(\'%s\', lower(hex(%u))),\'%04d-%02d-%02d %02d:%02d:%02d\',%d,%d," \
               "%d,%d,\'%s\',%d)"
               , "0x"
               , ulResId
               , pstCurTime->tm_year + 1900
               , pstCurTime->tm_mon + 1
               , pstCurTime->tm_mday
               , pstCurTime->tm_hour
               , pstCurTime->tm_min
               , pstCurTime->tm_sec
               , 0
               , 0
               , 0
               , 0
               , szSmpDesc
               , 5
              );
     
   lRet = mysql_query(&g_stMySQL, szSQLCmd);
   if(0 != lRet)
   {
      logr_error("%s:Line %d:mon_add_warning_record|sql:%s\n execute failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, szSQLCmd, lRet);
   }

   return DOS_SUCC;
}

static S32 mon_init_mysql_conn()
{
   g_pstConn = mysql_init(&g_stMySQL);
   if (!g_pstConn)
   {
     logr_error("%s:Line %d:mon_init_mysql_conn|mysql initialize failure, the cause:%s"
                , dos_get_filename(__FILE__), __LINE__, mysql_error(&g_stMySQL));
     return DOS_FAIL;
   }

   if (!mysql_real_connect(&g_stMySQL, szIPv4Address, "root", "admin", "syssrc", 0, NULL ,0))
   {
      logr_error("%s:Line %d:mon_init_mysql_conn|Connect database syssrc falied, the cause:%s"
                    , dos_get_filename(__FILE__), __LINE__, mysql_error(&g_stMySQL));
      return DOS_FAIL;
   }
             
   return DOS_SUCC;
}

static S32 mon_init_warning_cond()
{
   S32 lRet = 0;

   lRet = config_hb_init();
   if(0 > lRet)
   {
      logr_error("%s:Line %d:mon_init_warning_cond|threshold init failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
   }
   
   g_pstCond = (MON_THRESHOLD_S *)dos_dmem_alloc(sizeof(MON_THRESHOLD_S));
   if(!g_pstCond)
   {
      logr_error("%s:Line %d:mon_init_warning_cond|alloc memory failure,g_pstCond is %p!"
                    ,  dos_get_filename(__FILE__), __LINE__, g_pstCond);
      config_hb_deinit();
      return DOS_FAIL;
   }

   lRet = config_hb_threshold_mem(&(g_pstCond->lMemThreshold));
   if(0 > lRet)
   {
      logr_error("%s:Line %d:mon_init_warning_cond|get memory threshold failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      config_hb_deinit();
      dos_dmem_free(g_pstCond);
      return DOS_FAIL;
   }

   lRet = config_hb_threshold_cpu(&(g_pstCond->lCPUThreshold)
                                , &(g_pstCond->l5sCPUThreshold)
                                , &(g_pstCond->l1minCPUThreshold)
                                , &(g_pstCond->l10minCPUThreshold));
   if(0 > lRet)
   {
      logr_error("%s:Line %d:mon_init_warning_cond|get cpu threshold failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      config_hb_deinit();
      dos_dmem_free(g_pstCond);
      return DOS_FAIL;
   }

   lRet = config_hb_threshold_disk(&(g_pstCond->lPartitionThreshold)
                                 , &(g_pstCond->lDiskThreshold));
   if(0 > lRet)
   {
      logr_error("%s:Line %d:mon_init_warning_cond|get disk threshold failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      config_hb_deinit();
      dos_dmem_free(g_pstCond);
      return DOS_FAIL;
   }

   lRet = config_hb_threshold_proc(&(g_pstCond->lProcMemThreshold)
                                 , &(g_pstCond->lProcCPUThreshold));
   if(0 > lRet)
   {
      logr_error("%s:Line %d:mon_init_warning_cond|get proc threshold failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      config_hb_deinit();
      dos_dmem_free(g_pstCond);
      return DOS_FAIL;
   }

   return DOS_SUCC;
}

static S32 mon_close_mysql_conn()
{  
   if(!g_pstConn)
   {
      logr_error("%s:Line %d:mon_close_mysql_conn|close mysql failure,g_pConn is %p!"
                    , dos_get_filename(__FILE__), __LINE__, g_pstConn);
      return DOS_FAIL;
   }   
   mysql_close(&g_stMySQL);

   return DOS_SUCC;
}

S32 mon_res_destroy()
{
   S32 lRet = 0;

   lRet = mon_mem_free();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_destroy|mem resource destroy failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
   }
   
   lRet = mon_cpu_rslt_free();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_destroy|cpu rslt resource destroy success,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
   }

   lRet = mon_disk_free();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_destroy|disk resource destroy failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
   }

   lRet = mon_netcard_free();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_destroy|netcard resource destroy failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
   }

   lRet = mon_proc_free();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_destroy|process resource destroy failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
   }

   lRet = mon_destroy_warning_msg_queue();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_destroy|destroy warning msg queue failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
   }

   lRet = mon_cpu_queue_destroy();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_destroy|destroy cpu queue failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
   }
   
   lRet = mon_close_mysql_conn();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_destroy|close mysql connection failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
   }

   lRet = mon_deinit_str_array();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_res_destroy|deinit str array failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
   }

   lRet = config_hb_deinit();
   if(0 != lRet)
   {
      logr_error("%s:Line %d:mon_res_destroy|deinit heartbeat config failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
   }
   
   return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

