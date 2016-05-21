/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_http_api.h
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
    SC_HTTP_ERRNO_SERVER_NOT_READY,

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

    SC_API_CMD_ACTION_ADD,                        /* API命令task ctrl附加值，启动任务 */
    SC_API_CMD_ACTION_DELETE,                     /* API命令task ctrl附加值，停止任务 */
    SC_API_CMD_ACTION_UPDATE,
    SC_API_CMD_ACTION_START,                      /* API命令task ctrl附加值，启动任务 */
    SC_API_CMD_ACTION_STOP,                       /* API命令task ctrl附加值，停止任务 */
    SC_API_CMD_ACTION_CONTINUE,                   /* API命令task ctrl附加值，重新启动任务 */
    SC_API_CMD_ACTION_PAUSE,                      /* API命令task ctrl附加值，暂停任务 */
    SC_API_CMD_ACTION_STATUS,

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
    SC_API_CMD_ACTION_GATEWAY_SYNC,                      /* 网关数据强制同步 */

    SC_API_CMD_ACTION_SIP_ADD,                           /* 增加一个sip账户 */
    SC_API_CMD_ACTION_SIP_DELETE,                        /* 删除一个sip账户 */
    SC_API_CMD_ACTION_SIP_UPDATE,                        /* 更新一个sip账户 */

    SC_API_CMD_ACTION_ROUTE_ADD,                         /* 增加一个路由 */
    SC_API_CMD_ACTION_ROUTE_DELETE,                      /* 删除一个路由 */
    SC_API_CMD_ACTION_ROUTE_UPDATE,                      /* 路由更新 */
    SC_API_CMD_ACTION_ROUTE_SYNC,                        /* 路由强制数据同步 */

    SC_API_CMD_ACTION_GW_GROUP_ADD,                      /* 增加一个网关组 */
    SC_API_CMD_ACTION_GW_GROUP_DELETE,                   /* 删除一个网关组 */
    SC_API_CMD_ACTION_GW_GROUP_UPDATE,

    SC_API_CMD_ACTION_DID_ADD,                           /* 添加一个did */
    SC_API_CMD_ACTION_DID_DELETE,                        /* 删除一个did */
    SC_API_CMD_ACTION_DID_UPDATE,                        /* 更新一个did */

    SC_API_CMD_ACTION_BLACK_ADD,                         /* 添加黑名单 */
    SC_API_CMD_ACTION_BLACK_DELETE,                      /* 删除黑名单 */
    SC_API_CMD_ACTION_BLACK_UPDATE,                      /* 更新黑名单 */

    SC_API_CMD_ACTION_CALLER_ADD,                        /* 添加主叫 */
    SC_API_CMD_ACTION_CALLER_DELETE,                     /* 删除主叫 */
    SC_API_CMD_ACTION_CALLER_UPDATE,                     /* 主叫更新 */

    SC_API_CMD_ACTION_CALLER_GRP_ADD,                    /* 添加主叫号码组 */
    SC_API_CMD_ACTION_CALLER_GRP_DELETE,                 /* 删除主叫号码组 */
    SC_API_CMD_ACTION_CALLER_GRP_UPDATE,                 /* 更新主叫号码组 */

    SC_API_CMD_ACTION_CALLER_SET_ADD,                    /* 增加主叫号码设定 */
    SC_API_CMD_ACTION_CALLER_SET_DELETE,                 /* 删除主叫号码设定 */
    SC_API_CMD_ACTION_CALLER_SET_UPDATE,                 /* 更新主叫号码设定 */

    SC_API_CMD_ACTION_TRANSFORM_ADD,                     /* 增加号码变换 */
    SC_API_CMD_ACTION_TRANSFORM_DELETE,                  /* 删除号码变换 */
    SC_API_CMD_ACTION_TRANSFORM_UPDATE,                  /* 更新号码变换 */

    SC_API_CMD_ACTION_DEMO_TASK,                         /* 群呼任务演示 */
    SC_API_CMD_ACTION_DEMO_PREVIEW,                      /* 预览外呼演示 */

    SC_API_CMD_ACTION_SWITCHBOARD_ADD,
    SC_API_CMD_ACTION_SWITCHBOARD_UPDATE,
    SC_API_CMD_ACTION_SWITCHBOARD_DELETE,

    SC_API_CMD_ACTION_KEYMAP_ADD,
    SC_API_CMD_ACTION_KEYMAP_UPDATE,
    SC_API_CMD_ACTION_KEYMAP_DELETE,

    SC_API_CMD_ACTION_PERIOD_ADD,
    SC_API_CMD_ACTION_PERIOD_UPDATE,
    SC_API_CMD_ACTION_PERIOD_DELETE,

    SC_API_CMD_ACTION_BUTT
};

enum tagAPICallCtrlCmd
{
    SC_API_MAKE_CALL                 = 0, /* 发起呼叫 */
    SC_API_HANGUP_CALL,                   /* 挂断呼叫 */
    SC_API_RECORD,                        /* 呼叫录音 */
    SC_API_WHISPERS,                      /* 耳语 */
    SC_API_INTERCEPT,                     /* 监听 */
    SC_API_TRANSFOR_ATTAND,               /* attend transfer */
    SC_API_TRANSFOR_BLIND,                /* blind transfer */
    SC_API_CONFERENCE,                    /* 会议 */
    SC_API_HOLD,                          /* 呼叫保持 */
    SC_API_UNHOLD,                        /* 取消呼叫保持 */

    SC_API_CALLCTRL_BUTT
}SC_API_CALLCTRL_CMD_EN;

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


typedef U32 (*http_req_handle)(list_t *pstArgv);

typedef struct tagHttpRequestProcess
{
    S8              *pszRequest;
    http_req_handle callback;
}SC_HTTP_REQ_REG_TABLE_SC;

/* declare functions */
http_req_handle sc_http_api_find(S8 *pszName);
U32 sc_http_api_process(SC_HTTP_CLIENT_CB_S *pstClient);
U32 sc_http_api_reload_xml(list_t *pstArgv);
U32 sc_http_api_task_ctrl(list_t *pstArgv);
U32 sc_http_api_gateway_action(list_t *pstArgv);
U32 sc_http_api_sip_action(list_t *pstArgv);
U32 sc_http_api_num_verify(list_t *pstArgv);
U32 sc_http_api_call_ctrl(list_t *pstArgv);
U32 sc_http_api_agent_action(list_t *pstArgv);
U32 sc_http_api_agent_grp(list_t *pstArgv);
U32 sc_http_api_route_action(list_t *pstArgv);
U32 sc_http_api_gw_group_action(list_t *pstArgv);
U32 sc_http_api_did_action(list_t *pstArgv);
U32 sc_http_api_switchboard_action(list_t *pstArgv);
U32 sc_http_api_period_action(list_t *pstArgv);
U32 sc_http_api_keymap_action(list_t *pstArgv);
U32 sc_http_api_sys_stat_sync();

U32 sc_http_api_black_action(list_t *pstArgv);
U32 sc_http_api_caller_action(list_t *pstArgv);
U32 sc_http_api_callergrp_action(list_t *pstArgv);
U32 sc_http_api_callerset_action(list_t *pstArgv);
U32 sc_http_api_eix_action(list_t *pstArgv);
U32 sc_http_api_numlmt_action(list_t *pstArgv);
U32 sc_http_api_numtransform_action(list_t *pstArgv);
U32 sc_http_api_customer_action(list_t *pstArgv);
U32 sc_http_api_demo_action(list_t *pstArgv);
U32 sc_agent_group_http_update_proc(U32 ulAction, U32 ulGrpID);
U32 sc_http_api_agent_call_ctrl(list_t *pstArgv);
U32 sc_http_api_agent(list_t *pstArgv);
U32 sc_http_api_serv_ctrl_action(list_t *pstArgv);
U32 sc_http_api_stat_syn(list_t *pstArgv);
U32 sc_task_mngt_cmd_proc(U32 ulAction, U32 ulCustomerID, U32 ulTaskID);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __SC_TASK_PUB_H__ */

