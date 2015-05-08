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
#include "mon_lib.h"
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
MON_PROC_STATUS_S *    g_pastProc[MAX_PROC_CNT] = {0};

U32  g_ulPartCnt = 0;
U32  g_ulNetCnt  = 0;
U32  g_ulPidCnt  = 0;

S8 g_szMonMemInfo[MAX_BUFF_LENGTH]     = {0};
S8 g_szMonCPUInfo[MAX_BUFF_LENGTH]     = {0};
S8 g_szMonDiskInfo[MAX_PARTITION_COUNT * MAX_BUFF_LENGTH]    = {0};
S8 g_szMonNetworkInfo[MAX_NETCARD_CNT * MAX_BUFF_LENGTH] = {0};
S8 g_szMonProcessInfo[MAX_PROC_CNT * MAX_BUFF_LENGTH] = {0};

static DB_HANDLE_ST *         g_pstDBHandle = NULL;
static MON_MSG_QUEUE_S *      g_pstMsgQueue = NULL;//消息队列
static MON_THRESHOLD_S *      g_pstCond = NULL;

S8 * g_pszAnalyseList = NULL;

static U32 mon_get_res_info();
static U32 mon_handle_excp();
static U32 mon_add_data_to_db();
static U32 mon_print_data_log();

#if 0
static S32 mon_reset_res_data();
#endif

static U32 mon_add_warning_record(U32 ulResId);
static U32 mon_init_db_conn();
static U32 mon_init_warning_cond();
static U32 mon_close_db_conn();

/**
 * 功能:资源监控
 * 参数集：
 *   参数1:VOID *p  创建线程的回调参数
 * 返回值：
 *   无返回值
 */
VOID *mon_res_monitor(VOID *p)
{   
   while (1)
   { 
      U32 ulRet = 0;
      pthread_mutex_lock(&g_stMonMutex); 
      /*  获取资源信息  */
      ulRet = mon_get_res_info();
      if (DOS_SUCC != ulRet)
      {
         logr_error("%s:Line %u:mon_res_monitor|get resource info failure,ulRet is %u!"  
                    , dos_get_filename(__FILE__), __LINE__, ulRet); 
      }
      
      /*  异常处理  */
      ulRet = mon_handle_excp();
      if (DOS_SUCC != ulRet)
      {
         logr_error("%s:Line %u:mon_res_monitor|handle exception failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
      }
      
      /*  将数据记录至数据库  */
      ulRet = mon_add_data_to_db();
      if (DOS_SUCC != ulRet)
      {
         logr_error("%s:Line %u:mon_res_monitor|add record to database failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
      }
 
      /*  打印数据日志  */
      ulRet = mon_print_data_log();
      if (DOS_SUCC != ulRet)
      {
         logr_error("%s:Line %u:mon_res_monitor|print data log failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
      }
   
      pthread_cond_signal(&g_stMonCond);
      pthread_mutex_unlock(&g_stMonMutex);
      sleep(5);
   }
}

/**
 * 功能:告警处理
 * 参数集：
 *   参数1:VOID *p  创建线程的回调参数
 * 返回值：
 *   无返回值
 */
VOID* mon_warning_handle(VOID *p)
{
     U32 ulRet = 0;
     
     g_pstMsgQueue =  mon_get_warning_msg_queue();
     if(DOS_ADDR_INVALID(g_pstMsgQueue))
     {
        logr_cirt("%s:Line %u:mon_warning_handle|get warning msg failure,g_pstMsgQueue is %p!"
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
                  if(DOS_SUCC != ulRet)
                  {
                     logr_error("%s:Line %u:mon_warning_handle|kill all monitor failure,lRet is %u!"
                                , dos_get_filename(__FILE__), __LINE__, ulRet);
                  }
               }
               break;
            default:
               logr_error("%s:Line %u:g_pstMsgQueue->pstHead->ulWarningId is %s%x"
                            , dos_get_filename(__FILE__), __LINE__, "0x"
                            , g_pstMsgQueue->pstHead->ulWarningId);
               break;
          }
          
         ulRet = mon_warning_msg_de_queue(g_pstMsgQueue);
         if(DOS_SUCC != ulRet)
         {
            logr_error("%s:Line %u:mon_warning_handle|delete warning msg queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            break;
         }
       }

       /*  数据重置  */
#if 0
       lRet = mon_reset_res_data();
       if(DOS_SUCC != lRet)
       {
          logr_error("%s:Line %d:mon_res_monitor|reset resource data failure,lRet is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lRet);
       }
#endif
       pthread_mutex_unlock(&g_stMonMutex);
   }   
}

/**
 * 功能:资源生成
 * 参数集：
 *   无参数
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_res_alloc()
{
   U32 ulRet = 0;

   ulRet = mon_init_cpu_queue();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_generate|init cpu queue failure,lRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_init_warning_msg_queue();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_generate|init msg queue failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_init_warning_cond();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_generate|init msg queue failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_init_db_conn();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_generate|init mysql connection failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_init_str_array();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_generate|init string array failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

    /*  分配资源 */
   ulRet = mon_mem_malloc();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_generate|mem alloc memory failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }
   
   ulRet = mon_cpu_rslt_malloc();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_generate|cpu result alloc memory failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_disk_malloc();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_generate|disk alloc memory failure,ulRet is %u!"
                , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_netcard_malloc();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_generate|net alloc memory failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_proc_malloc();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_generate|proc alloc memory failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   return DOS_SUCC;
}

/**
 * 功能:初始化资源
 * 参数集：
 *   无参数
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
static U32 mon_get_res_info()
{
    U32 ulRet = 0;
    
    ulRet = mon_read_mem_file();
    if (DOS_SUCC != ulRet)
    {
       logr_error("%s:Line %u:mon_get_res_info|get memory data failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
    }

    ulRet = mon_get_cpu_rslt_data();
    if (DOS_SUCC != ulRet)
    {
       logr_error("%s:Line %u:mon_get_res_info|get cpu result data success,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
    }

    ulRet = mon_get_partition_data();
    if (DOS_SUCC != ulRet)
    {
       logr_error("%s:Line %u:mon_get_res_info|get partition data failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
    }

    ulRet = mon_get_netcard_data();
    if (DOS_SUCC != ulRet)
    {
       logr_error("%s:Line %u:mon_get_res_info|get netcard data failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
    }
    
    ulRet = mon_get_process_data();
    if (DOS_SUCC != ulRet)
    {
       logr_error("%s:Line %u:mon_get_res_info|get process data success,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
    }
    
    return DOS_SUCC;
}

/**
 * 功能:处理告警异常
 * 参数集：
 *   无参数
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
static U32 mon_handle_excp()
{
    S32 lRet = 0;
    U32 ulRet = 0;
    U32 ulRows = 0;
    U32 ulTotalDiskRate = 0;
    
    /*  异常处理  */
    if (g_pstMem->ulPhysicalMemUsageRate >= g_pstCond->ulMemThreshold)
    {
       MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
       if (DOS_ADDR_INVALID(pstMsg))
       {
          logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                        , dos_get_filename(__FILE__), __LINE__, pstMsg);
       }

       ulRet = mon_generate_warning_id(MEM_RES, 0x00, RES_LACK);
       if((U32)0xff == ulRet)
       {
          logr_error("%s:Line %u:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                        , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
       }
       pstMsg->ulWarningId = ulRet;
       
       pstMsg->ulMsgLen = sizeof(MON_SYS_MEM_DATA_S);
       pstMsg->msg = g_pstMem;
       
       ulRet = mon_add_warning_record(pstMsg->ulWarningId);
       if(DOS_SUCC != lRet)
       {
          logr_error("%s:Line %u:mon_handle_excp|add warning record failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
       }
       
       ulRet = mon_warning_msg_en_queue(pstMsg);
       if(DOS_SUCC != ulRet)
       {
          logr_error("%s:Line %u:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
       }
       logr_info("%s:Line %u: Lack of Memory", dos_get_filename(__FILE__), __LINE__);
    }

    for(ulRows = 0; ulRows < g_ulPartCnt; ulRows++)
    {
       if(g_pastPartition[ulRows]->ulPartitionUsageRate >= g_pstCond->ulPartitionThreshold)
       {
          
          MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
          if (DOS_ADDR_INVALID(pstMsg))
          {
             logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                          , dos_get_filename(__FILE__), __LINE__, pstMsg);
          }
          

          ulRet = mon_generate_warning_id(DISK_RES, ulRows, RES_LACK);
          if((U32)0xff == ulRet)
          {
             logr_error("%s:Line %u:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                        , dos_get_filename(__FILE__), __LINE__ , "0x", ulRet);
          }
          
          pstMsg->ulWarningId = ulRet;
          pstMsg->ulMsgLen = sizeof(MON_SYS_MEM_DATA_S);
          pstMsg->msg = g_pastPartition[ulRows];
          
          ulRet = mon_add_warning_record(pstMsg->ulWarningId);
          if(DOS_SUCC != ulRet)
          {
             logr_error("%s:Line %d:mon_handle_excp|add warning record failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
          }
          
          ulRet = mon_warning_msg_en_queue(pstMsg);
          if(DOS_SUCC != ulRet)
          {
             logr_error("%s:Line %u:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
          }
          logr_info("%s:Line %u: Partition %s:Not enough partition volume."
                    , dos_get_filename(__FILE__), __LINE__, g_pastPartition[ulRows]->szPartitionName);
          
       }
    }

    ulTotalDiskRate = mon_get_total_disk_usage_rate();
    if(DOS_FAIL == ulTotalDiskRate)
    {
       logr_error("%s:Line %u:mon_handle_excp|get total disk usage rate failure,ulTotalDiskRate is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulTotalDiskRate);
    }
              
    if(ulTotalDiskRate >= g_pstCond->ulDiskThreshold)
    {
       
       MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
       if (DOS_ADDR_INVALID(pstMsg))
       {
            logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                         , dos_get_filename(__FILE__), __LINE__, pstMsg);
       }
       

       ulRet = mon_generate_warning_id(DISK_RES, 0x00, RES_LACK);
       if((U32)0xff == ulRet)
       {
          logr_error("%s:Line %u:mon_handle_excp|generate warning id failure,lRet is %s%x!"
                        , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
       }
                 
       pstMsg->ulWarningId = ulRet;
       pstMsg->ulMsgLen = sizeof(MON_SYS_PART_DATA_S);
       pstMsg->msg = g_pastPartition;
       
       ulRet = mon_add_warning_record(pstMsg->ulWarningId);
       if(DOS_SUCC != ulRet)
       {
          logr_error("%s:Line %u:mon_handle_excp|add warning record failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
       }
       
       ulRet = mon_warning_msg_en_queue(pstMsg);
       if(DOS_SUCC != ulRet)
       {
          logr_error("%s:Line %u:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
       }
       logr_info("%s:Line %u:Not enough disk volumn."
                    , dos_get_filename(__FILE__), __LINE__);
    }
   
    if(g_pstCpuRslt->ulCPUUsageRate >= g_pstCond->ulCPUThreshold||
       g_pstCpuRslt->ulCPU5sUsageRate >= g_pstCond->ul5sCPUThreshold ||
       g_pstCpuRslt->ulCPU1minUsageRate >= g_pstCond->ul1minCPUThreshold ||
       g_pstCpuRslt->ulCPU10minUsageRate >= g_pstCond->ul10minCPUThreshold)
    {
       
       MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
       if (DOS_ADDR_INVALID(pstMsg))
       {
           logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                        , dos_get_filename(__FILE__), __LINE__, pstMsg);
       }
       
       ulRet = mon_generate_warning_id(CPU_RES, 0x00, RES_LACK);
       if((U32)0xff == ulRet)
       {
          logr_error("%s:Line %u:mon_handle_excp|generate warning id failure,ulRet is %s%x"
                        , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
       }
       pstMsg->ulWarningId = ulRet;
       
       pstMsg->ulMsgLen = sizeof(MON_CPU_RSLT_S);
       pstMsg->msg = g_pstCpuRslt;
       
       ulRet = mon_add_warning_record(pstMsg->ulWarningId);
       if(DOS_SUCC != ulRet)
       {
          logr_error("%s:Line %u:mon_handle_excp|add warning record failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
       }
       
       ulRet = mon_warning_msg_en_queue(pstMsg);
       if(DOS_SUCC != ulRet)
       {
          logr_error("%s:Line %u:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
       }

       logr_info("%s:Line %u: Such high CPU rate.", dos_get_filename(__FILE__), __LINE__);
    }

    for(ulRows = 0; ulRows < g_ulNetCnt; ulRows++)
    {
       if(DOS_FALSE == (mon_is_netcard_connected(g_pastNet[ulRows]->szNetDevName)))
       {
          
          MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
          if (DOS_ADDR_INVALID(pstMsg))
          {
              logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                           , dos_get_filename(__FILE__), __LINE__, pstMsg);
          }          

          ulRet = mon_generate_warning_id(NET_RES, 0x00, 0x00);
          if((U32)0xff == ulRet)
          {
             logr_error("%s:Line %u:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                        , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
          }
          
          pstMsg->ulWarningId = ulRet;
          pstMsg->ulMsgLen = sizeof(MON_NET_CARD_PARAM_S);
          pstMsg->msg = g_pastNet[ulRows];
          
          ulRet = mon_add_warning_record(pstMsg->ulWarningId);
          if(DOS_SUCC != ulRet)
          {
             logr_error("%s:Line %u:mon_handle_excp|add warning record failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
          }
          
          ulRet = mon_warning_msg_en_queue(pstMsg);
          if(DOS_SUCC != lRet)
          {
             logr_error("%s:Line %u:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
          }
          logr_info("%s:Line %u: Netcard %s disconnected.", dos_get_filename(__FILE__), __LINE__, g_pastNet[ulRows]->szNetDevName);
       }
    }

    ulRet = mon_get_proc_total_cpu_rate();
    if(DOS_FAIL == ulRet)
    {
       logr_error("%s:Line %u:mon_handle_excp|get all proc total cpu rate failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
    }
              
    if(ulRet > g_pstCond->ulProcCPUThreshold)
    { 
        MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
        if (DOS_ADDR_INVALID(pstMsg))
        {
           logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                          , dos_get_filename(__FILE__), __LINE__, pstMsg);
        }
        
        ulRet = mon_generate_warning_id(PROC_RES, 0x00, 0x01);
        if((U32)0xff == ulRet)
        {
          logr_error("%s:Line %u:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                        , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
        }
        
        pstMsg->ulWarningId = ulRet;
        pstMsg->ulMsgLen = sizeof(MON_PROC_STATUS_S);
        pstMsg->msg = g_pastProc;
        
        ulRet = mon_add_warning_record(pstMsg->ulWarningId);
        if(DOS_SUCC != ulRet)
        {
           logr_error("%s:Line %u:mon_handle_excp|add warning record failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
        }
        
        ulRet = mon_warning_msg_en_queue(pstMsg);
        if(DOS_SUCC != ulRet)
        {
           logr_error("%s:Line %u:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
        }
        logr_info("%s:Line %u:Processes possess such high CPU rate.", dos_get_filename(__FILE__), __LINE__);     
    }

    ulRet = mon_get_proc_total_mem_rate();
    if(DOS_FAIL == ulRet)
    {
       logr_error("%s:Line %u:mon_handle_excp|get all proc total memory usage rate failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
    }

    if(ulRet >= g_pstCond->ulProcMemThreshold)
    {   
       MON_MSG_S * pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
       if (DOS_ADDR_INVALID(pstMsg))
       {
           logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                         , dos_get_filename(__FILE__), __LINE__, pstMsg);
       }
       
       ulRet = mon_generate_warning_id(PROC_RES, 0x00, 0x02);
       if((U32)0xff == ulRet)
       {
         logr_error("%s:Line %u:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                    , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
       }
       
       pstMsg->ulWarningId = ulRet;
       
       pstMsg->ulMsgLen = sizeof(MON_PROC_STATUS_S);
       pstMsg->msg = g_pastProc;
       
       ulRet = mon_add_warning_record(pstMsg->ulWarningId);
       if(DOS_SUCC != ulRet)
       {
         logr_error("%s:Line %u:mon_handle_excp|add warning record failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
       }
       
       ulRet = mon_warning_msg_en_queue(pstMsg);
       if(DOS_SUCC != ulRet)
       {
         logr_error("%s:Line %d:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
       }
       logr_info("%s:Line %u:Processes possess such high Memory rate."
                    , dos_get_filename(__FILE__), __LINE__);
    }

    return DOS_SUCC;
}

/**
 * 功能:将监控数据添加至数据库
 * 参数集：
 *   无参数
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
static U32 mon_add_data_to_db()
{
   time_t ulCur;
   struct tm *pstCurTime;
   S8  szSQLCmd[SQL_CMD_MAX_LENGTH] = {0};
   S32 lRet = 0;
   U32 ulTotalDiskKBytes = 0;
   U32 ulTotalDiskRate = 0;
   U32 ulProcTotalMemRate = 0;
   U32 ulProcTotalCPURate = 0;

   ulTotalDiskKBytes = mon_get_total_disk_kbytes();
   if(DOS_FAIL == ulTotalDiskKBytes)
   {
      logr_error("%s:Line %u:mon_add_data_to_db|get total disk kbytes failure,ulTotalDiskKBytes is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulTotalDiskKBytes);
      return DOS_FAIL;
   }
   
   ulTotalDiskRate = mon_get_total_disk_usage_rate();
   if(DOS_FAIL == ulTotalDiskRate)
   {
      logr_error("%s:Line %u:mon_add_data_to_db|get total disk uasge rate failure,ulTotalDiskRate is %u!"
                    ,dos_get_filename(__FILE__), __LINE__ , ulTotalDiskRate);
      return DOS_FAIL;
   }

   ulProcTotalMemRate = mon_get_proc_total_mem_rate();
   if(DOS_FAIL == ulProcTotalMemRate)
   {
      logr_error("%s:Line %u:mon_add_data_to_db|get all proc total mem rate failure,ulProcTotalMemRate is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulProcTotalMemRate);
       return DOS_FAIL;
   }

   ulProcTotalCPURate = mon_get_proc_total_cpu_rate();
   if(DOS_FAIL == ulProcTotalCPURate)
   {
      logr_error("%s:Line %du:mon_add_data_to_db|get all proc total cpu rate failure,ulProcTotalCPURate is %d!"
                    , dos_get_filename(__FILE__), __LINE__, ulProcTotalCPURate);
      return DOS_FAIL;
   }

   time(&ulCur);
   pstCurTime = localtime(&ulCur);
   dos_snprintf(szSQLCmd, MAX_BUFF_LENGTH, "INSERT INTO tbl_syssrc%04u%02u(ctime,phymem," \
     "phymem_pct,swap,swap_pct,hd,hd_pct,cpu_pct,5scpu_pct,1mcpu_pct,10mcpu_pct,trans_rate,procmem_pct,proccpu_pct)" \
     " VALUES(\'%04u-%02u-%02u %02u:%02u:%02u\',%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u);"
     , pstCurTime->tm_year + 1900
     , pstCurTime->tm_mon + 1
     , pstCurTime->tm_year + 1900
     , pstCurTime->tm_mon + 1
     , pstCurTime->tm_mday
     , pstCurTime->tm_hour
     , pstCurTime->tm_min
     , pstCurTime->tm_sec
     , g_pstMem->ulPhysicalMemTotalBytes
     , g_pstMem->ulPhysicalMemUsageRate
     , g_pstMem->ulSwapTotalBytes
     , g_pstMem->ulSwapUsageRate
     , ulTotalDiskKBytes
     , ulTotalDiskRate
     , g_pstCpuRslt->ulCPUUsageRate 
     , g_pstCpuRslt->ulCPU5sUsageRate
     , g_pstCpuRslt->ulCPU1minUsageRate
     , g_pstCpuRslt->ulCPU10minUsageRate
     , g_pastNet[0]->ulRWSpeed
     , ulProcTotalMemRate
     , ulProcTotalCPURate
   );

   lRet = db_query(g_pstDBHandle, szSQLCmd, NULL, NULL, NULL);
   if(DB_ERR_SUCC != lRet)
   {
      logr_error("%s:Line %u:mon_add_warning_record|db_query failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }
             
   return DOS_SUCC;
}

/**
 * 功能:打印数据日志
 * 参数集：
 *   无参数
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
static U32 mon_print_data_log()
{
   U32 ulRet = 0;
   /*    打印数据日志  */
   
   ulRet = mon_get_mem_formatted_info();
   if(DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_print_data_log|get mem formatted information failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
      return DOS_FAIL;
   }
   logr_info("%s:Line %u:mon_print_data_log|print mem log SUCC.", dos_get_filename(__FILE__), __LINE__);

   ulRet = mon_get_cpu_rslt_formatted_info();
   if(DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_print_data_log|get cpu rslt formatted information failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
      return DOS_FAIL;
   }
   logr_info("%s:Line %u:mon_print_data_log|print cpu log SUCC.", dos_get_filename(__FILE__), __LINE__);

   ulRet = mon_get_partition_formatted_info();
   if(DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_print_data_log|get partition formatted information failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
      return DOS_FAIL;
   }
   logr_info("%s:Line %u:mon_print_data_log|print partition log SUCC.", dos_get_filename(__FILE__), __LINE__);


   ulRet = mon_netcard_formatted_info();
   if(DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_print_data_log|get netcard formatted information failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
      return DOS_FAIL;
   }
   logr_info("%s:Line %u:mon_print_data_log|print netcard log SUCC.", dos_get_filename(__FILE__), __LINE__);
   
   ulRet = mon_get_process_formatted_info();
   if(DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_print_data_log|get process formatted information failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
      return DOS_FAIL;
   }
   logr_info("%s:Line %u:mon_print_data_log|print process log SUCC.", dos_get_filename(__FILE__), __LINE__);

   logr_info("%s:Line %u:mon_print_data_log|print all data log SUCC.", dos_get_filename(__FILE__), __LINE__);
   
   return DOS_SUCC;
}

/**
 * 功能:将数据初始化为0
 * 参数集：
 *   无参数
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
#if 0
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
#endif

/**
 * 功能:给告警数据库添加告警记录
 * 参数集：
 *   参数1: U32 ulResId  告警id
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
static U32 mon_add_warning_record(U32 ulResId)
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
   
   dos_snprintf(szSQLCmd, SQL_CMD_MAX_LENGTH, "INSERT INTO tbl_alarmlog(" \
               "ctime,warning,cause,type,object,content,cycle,status)" \
               " VALUES(\'%04u-%02u-%02u %02u:%02u:%02u\',concat(\'%s\', lower(hex(%u))),%u,%u," \
               "%u,\'%s\',%u,%u)"
               , pstCurTime->tm_year + 1900
               , pstCurTime->tm_mon + 1
               , pstCurTime->tm_mday
               , pstCurTime->tm_hour
               , pstCurTime->tm_min
               , pstCurTime->tm_sec
               , "0x"
               , ulResId
               , ((ulResId & 0x0fffffff) >> 24) - 1
               , 0
               , 0
               , szSmpDesc
               , 5
               , 1
              );

   lRet = db_query(g_pstDBHandle, szSQLCmd, NULL, NULL, NULL);
   if(DB_ERR_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_add_warning_record|db_query failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }

   return DOS_SUCC;
}


/**
 * 功能:初始化数据库连接
 * 参数集：
 *   无参数
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
static U32 mon_init_db_conn()
{
   S8  szDBHost[48] = {0};
   S8  szDBUsername[48] = {0};
   S8  szDBPassword[48] = {0};
   S8  szDBName[48] = {0};
   S8  szDBSockPath[48] = {0,};
   U16 usDBPort = 0;

   if(config_get_db_host(szDBHost, sizeof(szDBHost)) < 0 )
   {
        DOS_ASSERT(0);
        return DOS_FAIL;
   }
   
   if(config_get_syssrc_db_user(szDBUsername, sizeof(szDBUsername)) < 0 )
   {
        DOS_ASSERT(0);
        return DOS_FAIL;
   }
      
   if(config_get_syssrc_db_password(szDBPassword, sizeof(szDBPassword)) < 0 )
   {
        DOS_ASSERT(0);
        return DOS_FAIL;
   }

   if(config_get_syssrc_db_dbname(szDBName, sizeof(szDBName)) < 0 )
   {
        DOS_ASSERT(0);
      
        return DOS_FAIL;
   }

   if (config_get_mysqlsock_path(szDBSockPath, sizeof(szDBSockPath)) < 0)
   {
        szDBSockPath[0] = '\0';
   }

   usDBPort = config_get_db_port();
   if (0 == usDBPort || U16_BUTT == usDBPort)
   {
       usDBPort = 3306;
   }

   g_pstDBHandle = db_create(DB_TYPE_MYSQL);
   if (DOS_ADDR_INVALID(g_pstDBHandle))
   {
       DOS_ASSERT(0);
       return DOS_FAIL;
   }

   dos_strncpy(g_pstDBHandle->szHost, szDBHost, sizeof(g_pstDBHandle->szHost));
   g_pstDBHandle->szHost[sizeof(g_pstDBHandle->szHost) - 1] = '\0';

   dos_strncpy(g_pstDBHandle->szUsername, szDBUsername, sizeof(g_pstDBHandle->szUsername));
   g_pstDBHandle->szUsername[sizeof(g_pstDBHandle->szUsername) - 1] = '\0';

   dos_strncpy(g_pstDBHandle->szPassword, szDBPassword, sizeof(g_pstDBHandle->szPassword));
   g_pstDBHandle->szPassword[sizeof(g_pstDBHandle->szPassword) - 1] = '\0';

   dos_strncpy(g_pstDBHandle->szDBName, szDBName, sizeof(g_pstDBHandle->szDBName));
   g_pstDBHandle->szDBName[sizeof(g_pstDBHandle->szDBName) - 1] = '\0';

   if ('\0' != szDBSockPath[0])
   {
        dos_strncpy(g_pstDBHandle->szSockPath, szDBSockPath, sizeof(szDBSockPath));
        g_pstDBHandle->szSockPath[sizeof(g_pstDBHandle->szSockPath) - 1] = '\0';
   }
   else
   {
        g_pstDBHandle->szSockPath[0] = '\0';
   }

   g_pstDBHandle->usPort = usDBPort;

   if (db_open(g_pstDBHandle) < 0 )
   {
      DOS_ASSERT(0);
      db_destroy(&g_pstDBHandle);
      g_pstDBHandle = NULL;
      return DOS_FAIL;
   }

   return DOS_SUCC;
}

/**
 * 功能:初始化告警发生的条件，即阀值
 * 参数集：
 *   无参数
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
static U32 mon_init_warning_cond()
{
   U32 ulRet = 0;

   ulRet = config_hb_init();
   if(0 > ulRet)
   {
      logr_error("%s:Line %u:mon_init_warning_cond|threshold init failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }
   
   g_pstCond = (MON_THRESHOLD_S *)dos_dmem_alloc(sizeof(MON_THRESHOLD_S));
   if(DOS_ADDR_INVALID(g_pstCond))
   {
      logr_error("%s:Line %u:mon_init_warning_cond|alloc memory failure,g_pstCond is %p!"
                    ,  dos_get_filename(__FILE__), __LINE__, g_pstCond);
      config_hb_deinit();
      return DOS_FAIL;
   }

   ulRet = config_hb_threshold_mem(&(g_pstCond->ulMemThreshold));
   if(0 > ulRet)
   {
      logr_error("%s:Line %u:mon_init_warning_cond|get memory threshold failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);

      /* 如果数据读取失败，给设置默认值 */
      g_pstCond->ulMemThreshold = 90;
   }

   ulRet = config_hb_threshold_cpu(&(g_pstCond->ulCPUThreshold)
                                , &(g_pstCond->ul5sCPUThreshold)
                                , &(g_pstCond->ul1minCPUThreshold)
                                , &(g_pstCond->ul10minCPUThreshold));
   if(0 > ulRet)
   {
      logr_error("%s:Line %u:mon_init_warning_cond|get cpu threshold failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);

      /* 如果读取失败，设置默认值 */
      g_pstCond->ulCPUThreshold = 95;
      g_pstCond->ul5sCPUThreshold = 95;
      g_pstCond->ul1minCPUThreshold= 95;
      g_pstCond->ul10minCPUThreshold = 95;
   }

   ulRet = config_hb_threshold_disk(&(g_pstCond->ulPartitionThreshold)
                                 , &(g_pstCond->ulDiskThreshold));
   if(0 > ulRet)
   {
      logr_error("%s:Line %u:mon_init_warning_cond|get disk threshold failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
                    
      /* 读取失败则设置默认值 */
      g_pstCond->ulPartitionThreshold = 95;
      g_pstCond->ulDiskThreshold = 90;
   }

   ulRet = config_hb_threshold_proc(&(g_pstCond->ulProcMemThreshold)
                                 , &(g_pstCond->ulProcCPUThreshold));
   if(0 > ulRet)
   {
      logr_error("%s:Line %u:mon_init_warning_cond|get proc threshold failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);

      /* 如果读取失败则设置默认值 */
      g_pstCond->ulProcMemThreshold = 20;
      g_pstCond->ulProcCPUThreshold = 80;
   }

   return DOS_SUCC;
}

/**
 * 功能:关闭数据库连接
 * 参数集：
 *   无参数
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
static U32 mon_close_db_conn()
{  
   db_destroy(&g_pstDBHandle);
   dos_dmem_free(g_pstDBHandle);
   g_pstDBHandle = NULL;
   return DOS_SUCC;
}

/**
 * 功能:资源销毁
 * 参数集：
 *   无参数
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_res_destroy()
{
   U32 ulRet = 0;

   ulRet = mon_mem_free();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_destroy|mem resource destroy failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }
   
   ulRet = mon_cpu_rslt_free();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_destroy|cpu rslt resource destroy success,lRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_disk_free();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_destroy|disk resource destroy failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_netcard_free();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_destroy|netcard resource destroy failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_proc_free();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_destroy|process resource destroy failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_destroy_warning_msg_queue();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_destroy|destroy warning msg queue failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_cpu_queue_destroy();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_destroy|destroy cpu queue failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }
   
   ulRet = mon_close_db_conn();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_destroy|close mysql connection failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = mon_deinit_str_array();
   if (DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_res_destroy|deinit str array failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }

   ulRet = config_hb_deinit();
   if (0 != ulRet)
   {
      logr_error("%s:Line %u:mon_res_destroy|deinit heartbeat config failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
   }
   
   return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

