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
#include <dos/dos_types.h>

#ifndef _NOTIFICATION_H__
#define _NOTIFICATION_H__


#if INCLUDE_RES_MONITOR

U32 mon_send_sms(S8 * pszMsg, S8* pszTitle,S8 * pszTelNo);
U32 mon_send_audio(S8 * pszMsg, S8* pszTitle, S8 * pszTelNo);
U32 mon_send_email(S8* pszMsg, S8* pszTitle, S8* pszEmailAddress);

#endif //#if INCLUDE_RES_MONITOR
#endif // end _NOTIFICATION_H__
