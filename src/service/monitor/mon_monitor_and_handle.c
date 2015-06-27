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

#define MAX_WARNING_TYPE_CNT   10 //最大告警类型个数

#define GENERATE_WARNING_MSG(pstMsg, ulIndex, ulNo) \
    pstMsg->ulWarningId = ulNo; \
    pstMsg->ulMsgLen = dos_strlen(g_pstWarningMsg[ulIndex].szWarningDesc); \
    pstMsg->msg = (VOID *)g_pstWarningMsg[ulIndex].szWarningDesc ; \
    g_pstWarningMsg[ulIndex].bExcep = DOS_TRUE

#define GENERATE_NORMAL_MSG(pstMsg, ulIndex, ulNo) \
    pstMsg->ulWarningId = ulNo; \
    pstMsg->ulMsgLen = dos_strlen(g_pstWarningMsg[ulIndex].szNormalDesc); \
    pstMsg->msg = (VOID *)g_pstWarningMsg[ulIndex].szNormalDesc; \
    g_pstWarningMsg[ulIndex].bExcep = DOS_FALSE

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
MON_WARNING_MSG_S*     g_pstWarningMsg = NULL;

S8 * g_pszAnalyseList = NULL;

static U32 mon_get_res_info();
static U32 mon_handle_excp();
static U32 mon_add_data_to_db();

#if 0
static S32 mon_reset_res_data();
#endif

static U32 mon_init_db_conn();
static U32 mon_init_warning_cond();
static U32 mon_close_db_conn();
static U32 mon_init_warning_msg();
static U32 mon_deinit_warning_msg();
U32 mon_get_msg_index(U32 ulNo);
U32 mon_add_warning_record(U32 ulResId, S8* szInfoDesc);


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

          ulRet = mon_warning_msg_de_queue(g_pstMsgQueue);
          if(DOS_SUCC != ulRet)
          {
              logr_error("%s:Line %u:mon_warning_handle|delete warning msg queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
              break;
          }
       }

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

   ulRet = mon_init_warning_msg();
   if (DOS_SUCC != ulRet)
   {
       logr_error("%s:Line %u:init warning msg FAIL.", dos_get_filename(__FILE__), __LINE__);
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
    U32  ulRet = 0, ulProcMem = 0;
    U32  ulRows = 0;
    U32  ulIndex = 0;
    U32  ulTotalDiskRate = 0;
    BOOL bDiskExcept = DOS_FALSE, bAddToDB = DOS_FALSE, bNetExcept = DOS_FALSE, bNetBWExcept = DOS_FALSE;
    MON_MSG_S * pstMsg = NULL;

    /*******************************处理内存告警开始**********************************/
    /* 先生成内存异常告警编号 */
    ulRet = mon_generate_warning_id(MEM_RES, 0x00, RES_LACK);
    if((U32)0xff == ulRet)
    {
        logr_error("%s:Line %u:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                    , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
        return DOS_FAIL;
    }

    /* 利用告警编号找到消息的索引 */
    ulIndex = mon_get_msg_index(ulRet);
    if (U32_BUTT == ulIndex)
    {
        return DOS_FAIL;
    }

    /*  处理内存异常  */
    if (g_pstMem->ulPhysicalMemUsageRate >= g_pstCond->ulMemThreshold)
    {
        /* 如果第一次产生告警，须将其记录下来 */
        if (DOS_FALSE == g_pstWarningMsg[ulIndex].bExcep)
        {
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                            , dos_get_filename(__FILE__), __LINE__, pstMsg);
            }

            /* 构造告警消息并表明已产生告警 */
            GENERATE_WARNING_MSG(pstMsg,ulIndex,ulRet);

            /* 表明该记录需要添加至数据库 */
            bAddToDB = DOS_TRUE;
        }
    }
    else
    {
        /* 若处于正常水平，但是还没有回复告警，则恢复告警 */
        if (DOS_TRUE == g_pstWarningMsg[ulIndex].bExcep)
        {
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                            , dos_get_filename(__FILE__), __LINE__, pstMsg);
            }

            /* 构造恢复告警并标明告警已恢复 */
            GENERATE_NORMAL_MSG(pstMsg,ulIndex,ulRet);

            bAddToDB = DOS_TRUE;
        }
    }

    if (DOS_TRUE == bAddToDB)
    {
        /* 将记录插入数据库 */
        ulRet = mon_add_warning_record(pstMsg->ulWarningId, (S8 *)pstMsg->msg);
        if(DOS_SUCC != lRet)
        {
            logr_error("%s:Line %u:mon_handle_excp|add warning record failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            return DOS_FAIL;
        }

        /* 将消息加入消息队列 */
        ulRet = mon_warning_msg_en_queue(pstMsg);
        if(DOS_SUCC != ulRet)
        {
            logr_error("%s:Line %u:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            return DOS_FAIL;
        }
    }

    /************************内存告警处理完毕*************************/

    /************************磁盘告警处理开始*************************/
    bAddToDB = DOS_FALSE;
    ulRet = mon_generate_warning_id(DISK_RES, 0x00, RES_LACK);
    if ((U32)0xff == ulRet)
    {
        logr_error("%s:Line %u:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                        , dos_get_filename(__FILE__), __LINE__ , "0x", ulRet);
        return DOS_FAIL;
    }

    ulIndex = mon_get_msg_index(ulRet);
    if (U32_BUTT == ulIndex)
    {
        return DOS_FAIL;
    }

    for (ulRows = 0; ulRows < g_ulPartCnt; ++ulRows)
    {
        if(g_pastPartition[ulRows]->ulPartitionUsageRate >= g_pstCond->ulPartitionThreshold)
        {
            bDiskExcept = DOS_TRUE;
        }
    }
    ulTotalDiskRate = mon_get_total_disk_usage_rate();
    if(ulTotalDiskRate >= g_pstCond->ulDiskThreshold)
    {
        bDiskExcept = DOS_TRUE;
    }

    /* 如果产生告警 */
    if (DOS_TRUE == bDiskExcept)
    {
        /* 如果第一次产生告警，则将告警加入告警队列 */
        if (DOS_FALSE == g_pstWarningMsg[ulIndex].bExcep)
        {
            /* 构造告警消息 */
            pstMsg = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                return DOS_FAIL;
            }

            GENERATE_WARNING_MSG(pstMsg,ulIndex,ulRet);
            bAddToDB = DOS_TRUE;
        }
    }
    else
    {
        /* 如果告警不产生但未恢复，则恢复之 */
        if (g_pstWarningMsg[ulIndex].bExcep == DOS_TRUE)
        {
             /* 构造告警消息 */
            pstMsg = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                return DOS_FAIL;
            }

            GENERATE_NORMAL_MSG(pstMsg,ulIndex,ulRet);
            bAddToDB = DOS_TRUE;
        }
    }

    if (DOS_TRUE == bAddToDB)
    {
        ulRet = mon_add_warning_record(pstMsg->ulWarningId, (S8*)pstMsg->msg);
        if(DOS_SUCC != lRet)
        {
            logr_error("%s:Line %u:mon_handle_excp|add warning record failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            return DOS_FAIL;
        }

        /* 将消息加入消息队列 */
        ulRet = mon_warning_msg_en_queue(pstMsg);
        if(DOS_SUCC != ulRet)
        {
            logr_error("%s:Line %u:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            return DOS_FAIL;
        }
    }
    /*************************硬盘信息处理完毕**************************/

    /*************************CPU信息开始处理***************************/
    bAddToDB = DOS_FALSE;

    ulRet = mon_generate_warning_id(CPU_RES, 0x00, RES_LACK);
    if((U32)0xff == ulRet)
    {
        logr_error("%s:Line %u:mon_handle_excp|generate warning id failure,ulRet is %s%x"
                    , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
        return DOS_FAIL;
    }

    ulIndex = mon_get_msg_index(ulRet);
    if (U32_BUTT == ulIndex)
    {
        return DOS_FAIL;
    }

    if(g_pstCpuRslt->ulCPUUsageRate >= g_pstCond->ulCPUThreshold
        || g_pstCpuRslt->ulCPU5sUsageRate >= g_pstCond->ul5sCPUThreshold
        || g_pstCpuRslt->ulCPU1minUsageRate >= g_pstCond->ul1minCPUThreshold
        || g_pstCpuRslt->ulCPU10minUsageRate >= g_pstCond->ul10minCPUThreshold)
    {
        if (DOS_FALSE == g_pstWarningMsg[ulIndex].bExcep)
        {
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                            , dos_get_filename(__FILE__), __LINE__, pstMsg);
            }

            GENERATE_WARNING_MSG(pstMsg,ulIndex,ulRet);

            bAddToDB = DOS_TRUE;
        }
    }
    else
    {
        if (DOS_TRUE == g_pstWarningMsg[ulIndex].bExcep)
        {
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                            , dos_get_filename(__FILE__), __LINE__, pstMsg);
            }

            GENERATE_NORMAL_MSG(pstMsg,ulIndex,ulRet);

            bAddToDB = DOS_TRUE;
        }
    }

    if (DOS_TRUE == bAddToDB)
    {
        ulRet = mon_add_warning_record(pstMsg->ulWarningId, (S8*)pstMsg->msg);
        if(DOS_SUCC != lRet)
        {
            logr_error("%s:Line %u:mon_handle_excp|add warning record failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            return DOS_FAIL;
        }

        /* 将消息加入消息队列 */
        ulRet = mon_warning_msg_en_queue(pstMsg);
        if(DOS_SUCC != ulRet)
        {
            logr_error("%s:Line %u:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            return DOS_FAIL;
        }
    }
    /***********************CPU异常信息处理完毕***************************/

    /***********************网卡异常信息处理开始**************************/
    bAddToDB = DOS_FALSE;

    ulRet = mon_generate_warning_id(NET_RES, 0x00, 0x00);
    if((U32)0xff == ulRet)
    {
        logr_error("%s:Line %u:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                    , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
        return DOS_FAIL;
    }
    ulIndex = mon_get_msg_index(ulRet);
    if (U32_BUTT == ulIndex)
    {
        return DOS_FAIL;
    }

    for (ulRows = 0; ulRows < g_ulNetCnt; ++ulRows)
    {
        if(DOS_FALSE == mon_is_netcard_connected(g_pastNet[ulRows]->szNetDevName))
        {
            bNetExcept = DOS_TRUE;
        }
    }

    if (DOS_TRUE == bNetExcept)
    {
        if (DOS_FALSE == g_pstWarningMsg[ulIndex].bExcep)
        {
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                            , dos_get_filename(__FILE__), __LINE__, pstMsg);
            }

            GENERATE_WARNING_MSG(pstMsg,ulIndex,ulRet);

            bAddToDB = DOS_TRUE;
        }
    }
    else
    {
        if (DOS_TRUE == g_pstWarningMsg[ulIndex].bExcep)
        {
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                            , dos_get_filename(__FILE__), __LINE__, pstMsg);
            }

            GENERATE_NORMAL_MSG(pstMsg,ulIndex,ulRet);

            bAddToDB = DOS_TRUE;
        }
    }

    if (DOS_TRUE == bAddToDB)
    {
        ulRet = mon_add_warning_record(pstMsg->ulWarningId, (S8*)pstMsg->msg);
        if(DOS_SUCC != lRet)
        {
            logr_error("%s:Line %u:mon_handle_excp|add warning record failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            return DOS_FAIL;
        }

        /* 将消息加入消息队列 */
        ulRet = mon_warning_msg_en_queue(pstMsg);
        if(DOS_SUCC != ulRet)
        {
            logr_error("%s:Line %u:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            return DOS_FAIL;
        }
    }
    /************************网卡异常信息处理结束*******************************/

    /************************网络带宽占用资源过大处理开始******************************/
    bAddToDB = DOS_FALSE;

    ulRet = mon_generate_warning_id(NET_RES, 0x00, 0x01);
    if ((U32)0xff == ulRet)
    {
        logr_error("%s:Line %d: Generate warning ID FAIL.", dos_get_filename(__FILE__), __LINE__);
        return DOS_FAIL;
    }

    ulIndex = mon_get_msg_index(ulRet);
    if (U32_BUTT == ulIndex)
    {
        return DOS_FAIL;
    }

    for (ulRows = 0; ulRows < g_ulNetCnt; ++ulRows)
    {
        if (0 != dos_stricmp(g_pastNet[ulRows]->szNetDevName, "lo")
            && g_pastNet[ulRows]->ulRWSpeed >= g_pstCond->ulMaxBandWidth * 1024)
        {
            bNetBWExcept = DOS_TRUE;
        }
    }

    if (DOS_TRUE == bNetBWExcept)
    {
        if (DOS_FALSE == g_pstWarningMsg[ulIndex].bExcep)
        {
             pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
             if (DOS_ADDR_INVALID(pstMsg))
             {
                 logr_error("%s:Line %u: Alloc Memory FAIL."
                                  , dos_get_filename(__FILE__), __LINE__);
                 return DOS_FAIL;
             }

             GENERATE_WARNING_MSG(pstMsg,ulIndex, ulRet);

             bAddToDB = DOS_TRUE;
        }
    }
    else
    {
        if (DOS_TRUE == g_pstWarningMsg[ulIndex].bExcep)
        {
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                logr_error("%s:Line %u: Alloc Memory FAIL."
                                  , dos_get_filename(__FILE__), __LINE__);
                return DOS_FAIL;
            }

            GENERATE_NORMAL_MSG(pstMsg,ulIndex,ulRet);

            bAddToDB = DOS_TRUE;
        }
    }

    if (DOS_TRUE == bAddToDB)
    {
        ulRet = mon_add_warning_record(pstMsg->ulWarningId, (S8*)pstMsg->msg);
        if(DOS_SUCC != lRet)
        {
            logr_error("%s:Line %u:mon_handle_excp|add warning record failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            return DOS_FAIL;
        }

        /* 将消息加入消息队列 */
        ulRet = mon_warning_msg_en_queue(pstMsg);
        if(DOS_SUCC != ulRet)
        {
            logr_error("%s:Line %u:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            return DOS_FAIL;
        }
    }
    /************************网络带宽占用资源过大处理结束******************************/

    /*************************进程占用内存过大处理开始*****************************/
    bAddToDB = DOS_FALSE;

    ulRet = mon_generate_warning_id(PROC_RES, 0x00, 0x02);
    if((U32)0xff == ulRet)
    {
        logr_error("%s:Line %u:mon_handle_excp|generate warning id failure,ulRet is %s%x!"
                    , dos_get_filename(__FILE__), __LINE__, "0x", ulRet);
        return DOS_FAIL;
    }

    ulIndex = mon_get_msg_index(ulRet);
    if (U32_BUTT == ulIndex)
    {
        return DOS_FAIL;
    }

    ulProcMem = mon_get_proc_total_mem_rate();
    if (DOS_FAIL == ulProcMem)
    {
        return DOS_FAIL;
    }

    if (ulProcMem >= g_pstCond->ulProcMemThreshold)
    {
        if (DOS_FALSE == g_pstWarningMsg[ulIndex].bExcep)
        {
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
               logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                              , dos_get_filename(__FILE__), __LINE__, pstMsg);
               return DOS_FAIL;
            }

            GENERATE_WARNING_MSG(pstMsg,ulIndex, ulRet);

            bAddToDB = DOS_TRUE;
        }
    }
    else
    {
        if (DOS_TRUE == g_pstWarningMsg[ulIndex].bExcep)
        {

            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
               logr_error("%s:Line %u: mon_handle_excp|warning msg alloc memory failure,pstMsg is %p!"
                              , dos_get_filename(__FILE__), __LINE__, pstMsg);
               return DOS_FAIL;
            }

            GENERATE_NORMAL_MSG(pstMsg,ulIndex,ulRet);
            bAddToDB = DOS_TRUE;
        }
    }

    if (DOS_TRUE == bAddToDB)
    {
        ulRet = mon_add_warning_record(pstMsg->ulWarningId, (S8*)pstMsg->msg);
        if(DOS_SUCC != lRet)
        {
            logr_error("%s:Line %u:mon_handle_excp|add warning record failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            return DOS_FAIL;
        }

        /* 将消息加入消息队列 */
        ulRet = mon_warning_msg_en_queue(pstMsg);
        if(DOS_SUCC != ulRet)
        {
            logr_error("%s:Line %u:mon_handle_excp|warning msg enter queue failure,ulRet is %u!"
                        , dos_get_filename(__FILE__), __LINE__, ulRet);
            return DOS_FAIL;
        }
    }
    /*************************进程占用内存过大处理结束*****************************/

    ulRet = mon_check_all_process();
    if (DOS_SUCC != ulRet)
    {
        return DOS_FAIL;
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
 * 功能:给告警数据库添加告警记录
 * 参数集：
 *   参数1: U32 ulResId  告警id
 *   参数2: S8* szInfoDesc 告警信息描述
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_add_warning_record(U32 ulResId, S8* szInfoDesc)
{
   S32 lRet = 0;
   U32 ulIndex = 0;
   time_t lCur;
   struct tm *pstCurTime;

   S8 szSQLCmd[SQL_CMD_MAX_LENGTH] = {0};

   time(&lCur);
   pstCurTime = localtime(&lCur);

   ulIndex = mon_get_msg_index(ulResId);
   if (U32_BUTT == ulIndex)
   {
       return DOS_FAIL;
   }

   dos_snprintf(szSQLCmd, SQL_CMD_MAX_LENGTH, "INSERT INTO tbl_alarmlog(" \
               "ctime,warning,cause,type,object,content,cycle,status)" \
               " VALUES(\'%04u-%02u-%02u %02u:%02u:%02u\',concat(\'%s\', lower(hex(%u))),%u,%u," \
               "%u,\'%s\',%u,%u);"
               , pstCurTime->tm_year + 1900
               , pstCurTime->tm_mon + 1
               , pstCurTime->tm_mday
               , pstCurTime->tm_hour
               , pstCurTime->tm_min
               , pstCurTime->tm_sec
               , "0x"
               , ulResId
               , ((ulResId & 0x0fffffff) >> 24) - 1
               , g_pstWarningMsg[ulIndex].ulWarningLevel
               , 0
               , szInfoDesc
               , 5
               , g_pstWarningMsg[ulIndex].bExcep == DOS_FALSE ? 0:1
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

   dos_snprintf(g_pstDBHandle->szHost, sizeof(g_pstDBHandle->szHost)
                , "%s", szDBHost);
   dos_snprintf(g_pstDBHandle->szUsername, sizeof(g_pstDBHandle->szUsername)
                , "%s", szDBUsername);
   dos_snprintf(g_pstDBHandle->szPassword, sizeof(g_pstDBHandle->szPassword)
                , "%s", szDBPassword);
   dos_snprintf(g_pstDBHandle->szDBName, sizeof(g_pstDBHandle->szDBName)
                , "%s", szDBName);

   if ('\0' != szDBSockPath[0])
   {
        dos_snprintf(g_pstDBHandle->szSockPath, sizeof(g_pstDBHandle->szSockPath)
                        , "%s", szDBSockPath);
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
      g_pstCond->ulCPUThreshold      = 95;
      g_pstCond->ul5sCPUThreshold    = 95;
      g_pstCond->ul1minCPUThreshold  = 95;
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

   ulRet = config_hb_threshold_bandwidth(&(g_pstCond->ulMaxBandWidth));
   if (0 > ulRet)
   {
        logr_error("%s:Line %u:Get Max BandWidth FAIL.", dos_get_filename(__FILE__), __LINE__);
        g_pstCond->ulMaxBandWidth = 90;
   }

   ulRet = config_hb_threshold_proc(&(g_pstCond->ulProcMemThreshold));
   if(0 > ulRet)
   {
      logr_error("%s:Line %u:mon_init_warning_cond|get proc threshold failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);

      /* 如果读取失败则设置默认值 */
      g_pstCond->ulProcMemThreshold = 40;
   }

   return DOS_SUCC;
}


/**
 * 功能:初始化告警消息
 * 参数集：
 *   无参数
 * 返回值：
 *   成功则返回DOS_SUCC，失败返回DOS_FAIL
 */
static U32 mon_init_warning_msg()
{
    U32 ulLoop = 0;

    g_pstWarningMsg = (MON_WARNING_MSG_S *)dos_dmem_alloc(MAX_WARNING_TYPE_CNT * sizeof(MON_WARNING_MSG_S));
    if (DOS_ADDR_INVALID(g_pstWarningMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero(g_pstWarningMsg, MAX_WARNING_TYPE_CNT * sizeof(MON_WARNING_MSG_S));

    for (ulLoop = 0; ulLoop < MAX_WARNING_TYPE_CNT; ++ulLoop)
    {
        /* 将所有的初始状态置为正常状态(即未超过相应资源所配置的阀值) */
        g_pstWarningMsg[ulLoop].bExcep = DOS_FALSE;
    }

    /* 第0个节点存储内存资源缺乏的相关信息 */
    g_pstWarningMsg[0].ulNo = mon_generate_warning_id(MEM_RES, 0x00, RES_LACK);
    g_pstWarningMsg[0].ulWarningLevel = MON_WARNING_IMPORTANT;
    dos_snprintf(g_pstWarningMsg[0].szWarningDesc, sizeof(g_pstWarningMsg[0].szWarningDesc)
                    , "%s", "No enough memory");
    dos_snprintf(g_pstWarningMsg[0].szNormalDesc, sizeof(g_pstWarningMsg[0].szNormalDesc)
                    , "%s", "Memory is OK");

    /* 第1个节点存储CPU资源占用过高的信息 */
    g_pstWarningMsg[1].ulNo = mon_generate_warning_id(CPU_RES, 0x00, RES_LACK);
    g_pstWarningMsg[1].ulWarningLevel = MON_WARNING_IMPORTANT;
    dos_snprintf(g_pstWarningMsg[1].szWarningDesc, sizeof(g_pstWarningMsg[1].szWarningDesc)
                    , "%s", "CPU is too busy");
    dos_snprintf(g_pstWarningMsg[1].szNormalDesc, sizeof(g_pstWarningMsg[1].szNormalDesc)
                    , "%s", "CPU is OK");

    /* 第2个节点存储硬盘资源占用过高的相关信息 */
    g_pstWarningMsg[2].ulNo = mon_generate_warning_id(DISK_RES, 0x00, RES_LACK);
    g_pstWarningMsg[2].ulWarningLevel = MON_WARNING_IMPORTANT;
    dos_snprintf(g_pstWarningMsg[2].szWarningDesc, sizeof(g_pstWarningMsg[2].szWarningDesc)
                    , "%s", "Lack of HardDisk Volume");
    dos_snprintf(g_pstWarningMsg[2].szNormalDesc, sizeof(g_pstWarningMsg[2].szNormalDesc)
                    , "%s", "HardDisk is OK");

    /* 第3个节点存储网络连接中断的相关信息 */
    g_pstWarningMsg[3].ulNo = mon_generate_warning_id(NET_RES, 0x00, 0x00);
    g_pstWarningMsg[3].ulWarningLevel = MON_WARNING_EMERG;
    dos_snprintf(g_pstWarningMsg[3].szWarningDesc, sizeof(g_pstWarningMsg[3].szWarningDesc)
                    , "%s", "Network disconnected");
    dos_snprintf(g_pstWarningMsg[3].szNormalDesc, sizeof(g_pstWarningMsg[3].szNormalDesc)
                    , "%s", "Network connection is OK");

    /* 第4个节点存储被监控进程占用的内存过高相关信息 */
    g_pstWarningMsg[4].ulNo = mon_generate_warning_id(PROC_RES, 0x00, 0x01);
    g_pstWarningMsg[4].ulWarningLevel = MON_WARNING_SECONDARY;
    dos_snprintf(g_pstWarningMsg[4].szWarningDesc, sizeof(g_pstWarningMsg[4].szWarningDesc)
                    , "%s", "Memory of Process is too much");
    dos_snprintf(g_pstWarningMsg[4].szNormalDesc, sizeof(g_pstWarningMsg[4].szNormalDesc)
                    , "%s", "Memory of Process is OK");

    /* 第5个节点存放带宽占用过大相关的信息 */
    g_pstWarningMsg[5].ulNo = mon_generate_warning_id(NET_RES, 0x00, 0x01);
    g_pstWarningMsg[5].ulWarningLevel = MON_WARNING_IMPORTANT;
    dos_snprintf(g_pstWarningMsg[5].szWarningDesc, sizeof(g_pstWarningMsg[5].szWarningDesc)
                , "%s", "Network is too busy");
    dos_snprintf(g_pstWarningMsg[5].szNormalDesc, sizeof(g_pstWarningMsg[5].szNormalDesc)
                , "%s", "Network is not busy");

    /* 第6个节点存放进程丢失的相关信息 */
    g_pstWarningMsg[6].ulNo = mon_generate_warning_id(PROC_RES, 0x00, 0x02);
    g_pstWarningMsg[6].ulWarningLevel = MON_WARNING_EMERG;
    dos_snprintf(g_pstWarningMsg[6].szNormalDesc, sizeof(g_pstWarningMsg[6].szNormalDesc)
                , "%s", "All Processes are OK");

    return DOS_SUCC;
}

static U32 mon_deinit_warning_msg()
{
    if (DOS_ADDR_VALID(g_pstWarningMsg))
    {
        dos_dmem_free(g_pstWarningMsg);
        g_pstWarningMsg = NULL;
        return DOS_SUCC;
    }

    return DOS_FAIL;
}


U32 mon_get_msg_index(U32 ulNo)
{
    U32 ulIndex = 0;

    switch (ulNo >> 24)
    {
        case MEM_RES:
            ulIndex = 0;
            break;
        case CPU_RES:
            ulIndex = 1;
            break;
        case DISK_RES:
            ulIndex = 2;
            break;
        case NET_RES:
            {
                switch (ulNo & 0xff)
                {
                    case 0:
                        ulIndex = 3;
                        break;
                    case 1:
                        ulIndex = 5;
                        break;
                    default:
                        ulIndex = U32_BUTT;
                        break;
                }
                break;
            }
        case PROC_RES:
        {
            switch (ulNo & 0xff)
            {
                case 1:
                    ulIndex = 4;
                    break;
                case 2:
                    ulIndex = 6;
                    break;
                default:
                    ulIndex = U32_BUTT;
                    break;
            }
            break;
        }
        default:
            ulIndex = U32_BUTT;
            break;
    }

    return ulIndex;
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

   ulRet = mon_deinit_warning_msg();
   if (DOS_SUCC != ulRet)
   {
       logr_error("%s:Line %u:deinit warning message FAIL.", dos_get_filename(__FILE__), __LINE__);
   }

   return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

