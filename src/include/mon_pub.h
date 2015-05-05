/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        mon_pub.h
  *        Created on: 2014-12-29
  *        Author: bubble
  *        Todo: Monitoring CPU resources
  *        History:
  */

#ifndef _MON_PUB_H__
#define _MON_PUB_H__

#if INCLUDE_RES_MONITOR  

#include <dos/dos_types.h>

U32 mon_init();
U32 mon_start();
U32 mon_stop();

#endif //#if INCLUDE_RES_MONITOR  
#endif //_MON_PUB_H__