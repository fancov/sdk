#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_notification.h"

S32 mon_send_shortmsg(S8 * pszMsg, S8 * pcszTelNo)
{
   return DOS_SUCC;
}

S32 mon_dial_telephone(S8 * pszMsg, S8 * pszTelNo)
{
   return DOS_SUCC;
}

S32 mon_send_audio(S8 * pszMsg, S8 * pszWechatNo)
{
   return DOS_SUCC;
}

S32 mon_send_web(S8 * pszMsg, S8 * pszURL)
{
   return DOS_SUCC;
}

S32 mon_send_email(S8 * pszMsg, S8 * pszEmailAddress)
{
   return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

