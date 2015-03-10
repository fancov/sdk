/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_task.h
 *
 *  创建时间: 2014年12月16日10:15:56
 *  作    者: Larry
 *  描    述: 业务控制模块，群呼任务向外部提供的接口函数
 *  修改历史:
 */

#ifndef __SC_HTTP_API_H__
#define __SC_HTTP_API_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

/* include public header files */
/* define marcos */
#define SC_API_PARAMS_MAX_NUM          24
#define SC_HTTP_REQ_MAX_LEN            64

/* HTTP响应数据错误码 */
enum SC_HTTP_ERRNO
{
	SC_HTTP_ERRNO_SUCC = 0,                      /* 无错误 */
	SC_HTTP_ERRNO_INVALID_USR = 0xF0000001,      /* 请求校验失败，不合法的用户 */
	SC_HTTP_ERRNO_INVALID_DATA,                  /* 请求校验失败，不合法的数据 */
	SC_HTTP_ERRNO_INVALID_TASK_STATUS,           /* 请求校验失败，不合法的数据 */
	SC_HTTP_ERRNO_INVALID_REQUEST,               /* 请求校验失败，不合法的请求 */
	SC_HTTP_ERRNO_INVALID_CMD,                   /* 请求校验失败，不合法的命令字 */
	SC_HTTP_ERRNO_INVALID_PARAM,                 /* 请求校验失败，不合法的参数 */
	SC_HTTP_ERRNO_CMD_EXEC_FAIL,                 /* 请求执行失败 */
	SC_HTTP_ERRNO_SERVER_ERROR,                  /* 请求执行失败 */

	SC_HTTP_ERRNO_BUTT
};


/* define enums */
enum tagAPICMDList
{
    SC_API_CMD_RELOAD                   = 0,      /* API命令字，reload配置文件 */
    SC_API_CMD_TASK_CTRL,                         /* API命令字，呼叫任务控制 */
    SC_API_CMD_CALL_CTRL,                         /* API命令字，呼叫干预 */
    SC_API_CMD_NUM_VERIFY,                        /* API命令字，号码审核 */
    // ------------------------------------------------------------
    SC_API_CMD_GATEWAY,                           /* API命令字，网关控制 */
    //------------------------------------------------------------

    SC_API_CMD_BUTT
};

enum tagAPICMDActionList
{
    SC_API_CMD_ACTION_FORCE             = 0,      /* API命令reload附加值，强制刷新指定的XML配置 */
    SC_API_CMD_ACTION_FORCE_ALL,                  /* API命令reload附加值，强制刷新所有配置 */
    SC_API_CMD_ACTION_CONDITIONAL,                /* API命令reload附加值，有条件的刷新，不影响业务 */

    SC_API_CMD_ACTION_START,                      /* API命令task ctrl附加值，启动任务 */
    SC_API_CMD_ACTION_STOP,                       /* API命令task ctrl附加值，停止任务 */
    SC_API_CMD_ACTION_CONTINUE,                   /* API命令task ctrl附加值，重新启动任务 */
    SC_API_CMD_ACTION_PAUSE,                      /* API命令task ctrl附加值，暂停任务 */

    SC_API_CMD_ACTION_MONITORING,                 /* API命令call ctrl附加值，监听呼叫 */
    SC_API_CMD_ACTION_HUNGUP,                     /* API命令call ctrl附加值，挂断呼叫 */

    SC_API_CMD_ACTION_SIGNIN,                     /* 坐席签入，登陆 */
    SC_API_CMD_ACTION_SIGNOFF,                    /* 坐席签出，登出 */
    SC_API_CMD_ACTION_ONLINE,                     /* 坐席上线，置闲 */
    SC_API_CMD_ACTION_OFFLINE,                    /* 坐席离线，置忙 */

    //---------------------------------------------------
    SC_API_CMD_ACTION_GATEWAY_ADD,                       /* 增加一个网关 */
    SC_API_CMD_ACTION_GATEWAY_DELETE,                    /* 删除一个网关 */
    SC_API_CMD_ACTION_GATEWAY_UPDATE,                    /* 网关更新     */

    SC_API_CMD_ACTION_SIP_ADD,                           /* 增加一个sip账户 */
    SC_API_CMD_ACTION_SIP_DELETE,                        /* 删除一个sip账户 */
    SC_API_CMD_ACTION_SIP_UPDATE,                        /* 更新一个sip账户 */
    //---------------------------------------------------

    SC_API_CMD_ACTION_BUTT
};

enum tagAPIParams
{
    SC_API_VERSION                      = 0,      /* API参数，API版本号 */
    SC_API_PARAM_CMD,                             /* API参数，命令 */
    SC_API_PARAM_ACTION,                          /* API参数，命令附加值 */
    SC_API_PARAM_CUSTOM_ID,                       /* API参数，用户ID */
    SC_API_PARAM_TASK_ID,                         /* API参数，任务ID */
    SC_API_PARAM_UUID,                            /* API参数，呼叫唯一标示符 */
    SC_API_PARAM_NUM,                             /* API参数，待审核的号码 */
    SC_API_PARAM_EXTRACODE,                       /* API参数，号码审核附加码 */

    SC_API_PARAM_BUTT
};

/* define structs */
typedef struct tagSCAPIParams
{
    S8    *pszStringName;                       /* 字符串值 */
    S8    *pszStringVal;                       /* 字符串值 */

    list_t  stList;
}SC_API_PARAMS_ST;

typedef struct tagHttpDataBuf
{
    S8          *pszBuff;                       /* 缓存数据指针 */
    U32         ulLength;                       /* 缓存长度 */
}SC_DATA_BUF_ST;


typedef struct tagHttpClientCB
{
    U32               ulValid;                  /* 是否可用 */
    U32               ulIndex;                  /* 当前编号 */
    U32               ulCurrentSrv;             /* 当前服务器编号 */

	S32               lSock;                    /* 当前连接的socket */

	U32               ulResponseCode;           /* HTTP响应码 */
    U32               ulErrCode;                /* HTTP响应码 */

    SC_DATA_BUF_ST    stDataBuff;

    list_t  stParamList;                        /* 请求参数列表 */
	list_t  stResponseDataList;   /* 数据域节点  refer to RESULT_DATA_NODE_ST */
}SC_HTTP_CLIENT_CB_S;


typedef U32 (*http_req_handle)(SC_HTTP_CLIENT_CB_S *pstClient);

typedef struct tagHttpRequestProcess
{
    S8              *pszRequest;
    http_req_handle callback;
}SC_HTTP_REQ_REG_TABLE_SC;

/* declare functions */
U32 sc_http_api_process(SC_HTTP_CLIENT_CB_S *pstClient);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __SC_TASK_PUB_H__ */

