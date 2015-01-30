#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include <pthread.h>
#include "mon_pub.h"
#include "mon_monitor_and_handle.h"


pthread_t mon_thr, hnd_thr;

S32 mon_init()
{
   S32 lRet = 0;
   lRet = mon_res_generate();
   if(DOS_FAIL == lRet)
   {
      logr_error("%s:Line %d:mon_start|lRet is %d!", dos_get_filename(__FILE__), __LINE__, lRet);
      return DOS_FAIL;
   }
   return DOS_SUCC;
}

S32 mon_start()
{
   S32 lRet = 0;
   
   lRet = pthread_create(&mon_thr, NULL, mon_res_monitor, NULL);
   if (lRet != 0)
   {
      logr_error("%s:Line %d:mon_start|Create thread mon_thr error!", dos_get_filename(__FILE__), __LINE__);
      return DOS_FAIL;
   }

   lRet = pthread_create(&hnd_thr, NULL, mon_warning_handle, NULL);
   if (lRet != 0)
   {
      logr_error("%s:Line %d:mon_start|Create thread mon_thr error!", dos_get_filename(__FILE__), __LINE__);
      return DOS_FAIL;
   }
   return DOS_SUCC;
}

S32 mon_stop()
{
   S32 lRet = 0;
   lRet = mon_res_destroy();
   if(DOS_SUCC != lRet)
   {
      logr_error("%s:Line %d:mon_stop|destroy resource failure!", dos_get_filename(__FILE__), __LINE__);
      return DOS_FAIL;
   }
   return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif