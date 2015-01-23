/*
  *      (C)Copyright 2014,dipcc,CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *       notification.h
  *       Created on:2014-12-07
  *           Author:bubble
  *             Todo:define notification method
  *          History:
  */

#ifndef _NOTIFICATION_H__
#define _NOTIFICATION_H__

#if INCLUDE_RES_MONITOR  

#include <dos/dos_types.h>


S32 mon_send_shortmsg(S8 * pszMsg, S8 * pcszTelNo);
S32 mon_dial_telephone(S8 * pszMsg, S8 * pszTelNo);
S32 mon_send_audio(S8 * pszMsg, S8 * pszWechatNo);
S32 mon_send_web(S8 * pszMsg, S8 * pszURL);
S32 mon_send_email(S8 * pszMsg, S8 * pszEmailAddress);

#endif //#if INCLUDE_RES_MONITOR  
#endif // end _NOTIFICATION_H__