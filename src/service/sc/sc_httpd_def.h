/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_httpd_pub.h
 *
 *  创建时间: 2014年12月16日10:10:52
 *  作    者: Larry
 *  描    述: FS接收外部请求所用的HTTP Server向外部提供的接口
 *  修改历史:
 */

#ifndef __SC_HTTPD_PUB_H__
#define __SC_HTTPD_PUB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>

/* include private header files */

/* define marcos */

/* define enums */
/* HTTP响应数据类型枚举 */
enum SC_HTTP_DATA
{
    SC_HTTP_DATA_ERRNO = 0,
    SC_HTTP_DATA_MSG,

    SC_HTTP_DATA_BUTT
};

enum SC_HTTP_RSP_CODE
{
    SC_HTTP_1XX          = 0,
    SC_HTTP_OK,
    SC_HTTP_300,
    SC_HTTP_302,
    SC_HTTP_FORBIDDEN,
    SC_HTTP_NOTFOUND,
    SC_HTTP_500,

    SC_HTTP_BUTT
};


/* define structs */
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

/* declare functions */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SC_HTTPD_PUB_H__ */

