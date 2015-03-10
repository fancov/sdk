/*
*            (C) Copyright 2014, DIPCC . Co., Ltd.
*                    ALL RIGHTS RESERVED
*
*  文件名: httpd.c
*
*  创建时间: 2014年12月16日10:10:52
*  作    者: Larry
*  描    述: FS接收外部请求所用的HTTP Server中相关的定义
*  修改历史:
*/

#ifndef __SC_HTTPD_H__
#define __SC_HTTPD_H__

#if 0
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
#endif
/* include public header files */

/* include private header files */

/* define marcos */
/* 定义最大接收缓存 */
#define SC_HTTP_MAX_RECV_BUFF_LEN      1024
#define SC_HTTP_MAX_SEND_BUFF_LEN      512


/* define enums */

/* define structs */
typedef struct tagHTTPDServerCB
{
    U32 ulValid;            /* 是否可用状态 */
    U32 ulIndex;            /* 当前编号 */

	S32 lListenSocket;      /* 当前socket监听的socket */
	U32 aulIPAddr[4];       /* ip地址，如果是IPv4就只使用第一个U32，如果是IPv6就全部使用 */
	U16 usPort;             /* 当前httpd监听的端口 */
    U32 ulStatus;           /* 状态 */

    U32 ulReqCnt;           /* 请求统计 */
}SC_HTTPD_CB_ST;

/* declare functions */

#if 0
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif

#endif /* __DIPCC_HTTPD_H__ */

