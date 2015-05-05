#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include <pthread.h>
#include "mon_pub.h"
#include "mon_monitor_and_handle.h"


pthread_t g_pMonthr, g_pHndthr;

/**
 * 功能:生成并初始化所有资源
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_init()
{
   U32 ulRet = 0;
   ulRet = mon_res_alloc();
   if(DOS_FAIL == ulRet)
   {
      logr_error("%s:Line %u:mon_start|ulRet is %u!", dos_get_filename(__FILE__), __LINE__, ulRet);
      return DOS_FAIL;
   }
   return DOS_SUCC;
}

/**
 * 功能: 创建调度任务
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_start()
{
   S32 lRet = 0;
   
   lRet = pthread_create(&g_pMonthr, NULL, mon_res_monitor, NULL);
   if (lRet != 0)
   {
      logr_error("%s:Line %d:mon_start|Create thread mon_thr error!", dos_get_filename(__FILE__), __LINE__);
      return DOS_FAIL;
   }

   lRet = pthread_create(&g_pHndthr, NULL, mon_warning_handle, NULL);
   if (lRet != 0)
   {
      logr_error("%s:Line %d:mon_start|Create thread mon_thr error!", dos_get_filename(__FILE__), __LINE__);
      return DOS_FAIL;
   }
   return DOS_SUCC;
}

/**
 * 功能:任务停止，释放所有资源
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_stop()
{
   U32 ulRet = 0;
   ulRet = mon_res_destroy();
   if(DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_stop|destroy resource failure!", dos_get_filename(__FILE__), __LINE__);
      return DOS_FAIL;
   }
   return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif