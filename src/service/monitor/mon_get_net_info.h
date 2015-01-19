/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        network_info.h
  *        Created on:2014-12-04
  *            Author:bubble
  *              Todo:Monitoring process resources
  *           History:
  */
 
#ifndef _MON_GET_NET_INFO_H__
#define _MON_GET_NET_INFO_H__

#if INCLUDE_RES_MONITOR  

#include <dos/dos_types.h>

#define MAX_PID_LENGTH   8
#define MAX_NETCARD_CNT  8
#define MAX_CMD_LENGTH   64
#define MAX_BUFF_LENGTH  1024

typedef struct tagNetcardParam //网卡信息
{
   S8  szNetDevName[32];    //网络设备名称
   S8  szMacAddress[32];    //设备MAC地址
   S8  szIPAddress[32];     //本地IPv4地址
   S8  szBroadIPAddress[32];//广播IP地址
   S8  szNetMask[32];       //子网掩码
   S32 lRWSpeed;            //数据传输最大速率
}MON_NET_CARD_PARAM_S;


S32  mon_netcard_malloc();
S32  mon_netcard_free();
BOOL mon_is_netcard_connected(const S8 * pszNetCard);
S32  mon_get_max_trans_speed(const S8 * pszDevName);
S32  mon_get_netcard_data();
S32  mon_netcard_formatted_info();


#endif //#if INCLUDE_RES_MONITOR  
#endif // end of _MON_GET_NET_INFO_H__