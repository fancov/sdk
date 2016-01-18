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

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */

/* include private header files */

/* define marcos */
/* 定义最大接收缓存 */
#define SC_HTTP_MAX_RECV_BUFF_LEN      1024
#define SC_HTTP_MAX_SEND_BUFF_LEN      512

/* define enums */
enum SC_HTTP_DATA{
    SC_HTTP_DATA_ERRNO = 0,
    SC_HTTP_DATA_MSG,

    SC_HTTP_DATA_BUTT
};

enum SC_HTTP_RSP_CODE{
    SC_HTTP_1XX          = 0,
    SC_HTTP_OK,
    SC_HTTP_300,
    SC_HTTP_302,
    SC_HTTP_FORBIDDEN,
    SC_HTTP_NOTFOUND,
    SC_HTTP_500,

    SC_HTTP_BUTT
};

/* 定义HTTP响应的数据类型名称列表 */
typedef struct tagResultNameList
{
    U32    ulHttpDataType;         /* 数据类型, refer to enum SC_HTTP_DATA*/
    S8     *pszName;               /* 数据类型名称 */
}SC_RESULT_NAME_NODE_ST;

/* 定义HTTP命令错误码描述 */
typedef struct tagHttpCmdErrNODesc
{
    U32    ulHttpErrNO;            /* 数据类型, refer to enum SC_HTTP_ERRNO*/
    S8     *pszVal;                /* 数据类型名称 */
}SC_HTTP_ERRNO_DESC_ST;

/* 定义HTTP响应码 */
typedef struct tagHttpRspCodeDesc
{
    U32    ulIndex;                /* 编号 refer to enum SC_HTTP_RSP_CODE */
    S8     *pszDesc;               /* 数据类型名称 */
}SC_HTTP_RSP_CODE_ST;


/* 定义HTTP响应的数据列表 */
typedef struct tagResultDataList
{
    list_t stList;                 /* 数据链表节点 */

    U32    ulHttpDataType;         /* 数据类型, refer to enum SC_HTTP_DATA*/
    U32    ulIntegerResult;        /* 整数类型数据 */

    /* 其他类型数据(只允许字符串)，如果该字段和ulIntegerResult都为非法，则忽略该数据项 */
    S8     *pszResult;
}SC_RESULT_DATA_NODE_ST;


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
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DIPCC_HTTPD_H__ */

