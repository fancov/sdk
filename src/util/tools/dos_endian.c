/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  endian.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 定义网络序与主机序相互转换的函数
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

/**
 * 函数：static inline BOOL test_id_big_endian()
 * 功能：判断主机是否为大端序
 * 参数：
 * 返回值：如果是大端序返回真，否则返回假
 */
static inline BOOL test_id_big_endian()
{
    S32 lTest = 1;

    if(((char*)&lTest)[sizeof(S32) - 1] ==1)
    {
        return DOS_TRUE;
    }
    else
    {
        return DOS_FALSE;
    }
}

/**
 * 函数：U32 dos_htonl(U32 ulSrc)
 * 功能：主机续转网络需
 * 参数：
 * 		U32 ulSrc：需要转换的数据
 * 返回值：返回网络序数据
 */
DLLEXPORT U32 dos_htonl(U32 ulSrc)
{
	U32 ulTmp;

	if (test_id_big_endian())
	{
		return ulSrc;
	}
	else
	{
		ulTmp = ((ulSrc & 0xFF000000) >> 24) |  ((ulSrc & 0x00FF0000) >> 8) |
                ((ulSrc & 0x0000FF00) << 8 ) |  ((ulSrc & 0x000000FF) << 24);

		return ulTmp;
	}
}

/**
 * 函数：U32 dos_htonl(U32 ulSrc)
 * 功能：网络序转主机续
 * 参数：
 * 		U32 ulSrc：需要转换的数据
 * 返回值：返回主机序数据
 */
DLLEXPORT U32 dos_ntohl(U32 ulSrc)
{
	return dos_htonl(ulSrc);
}


/**
 * 函数：U16 dos_htonl(U16 usSrc)
 * 功能：主机续转网络需
 * 参数：
 * 		U16 usSrc：需要转换的数据
 * 返回值：返回网络序数据
 */
DLLEXPORT U16 dos_htons(U16 usSrc)
{
	U16 usTmp;

	if (test_id_big_endian())
	{
		return usSrc;
	}
	else
	{
		usTmp = ((usSrc & 0xFF00)>>8) |  ((usSrc & 0x00FF)<<8);

		return usTmp;
	}
}

/**
 * 函数：U16 dos_htonl(U16 usSrc)
 * 功能：网络序转主机续
 * 参数：
 * 		U16 usSrc：需要转换的数据
 * 返回值：返回主机序数据
 */
DLLEXPORT U16 dos_ntohs(U16 usSrc)
{
	return dos_htons(usSrc);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

