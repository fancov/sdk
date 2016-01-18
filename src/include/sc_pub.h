/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名
 *
 *  创建时间:
 *  作    者:
 *  描    述:
 *  修改历史:
 */

#ifndef __SC_PUB_H__
#define __SC_PUB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


/* 定义错误码 */
enum SC_ERRCODE_E
{
    CC_ERR_NORMAL_CLEAR                         = 0,            /* 正常释放 */
    CC_ERR_NO_REASON                            = 1,            /* 无原因 */

    /* SIP 状态码 */
    CC_ERR_SIP_SUCC                             = 200,          /* 成功 */
    CC_ERR_SIP_BAD_REQUEST                      = 400,          /* 错误请求 */
    CC_ERR_SIP_UNAUTHORIZED                     = 401,          /* 未授权 */
    CC_ERR_SIP_PAYMENT_REQUIRED                 = 402,          /* 付费要求 */
    CC_ERR_SIP_FORBIDDEN                        = 403,          /* 禁止 */
    CC_ERR_SIP_NOT_FOUND                        = 404,          /* 未发现 */
    CC_ERR_SIP_METHOD_NO_ALLOWED                = 405,          /* 方法不允许 */
    CC_ERR_SIP_NOT_ACCEPTABLE                   = 406,          /* 不可接受 */
    CC_ERR_SIP_PROXY_AUTHENTICATION_REQUIRED    = 407,          /* 代理需要认证 */
    CC_ERR_SIP_REQUEST_TIMEOUT                  = 408,          /* 请求超时 */
    CC_ERR_SIP_GONE                             = 410,          /* 离开 */
    CC_ERR_SIP_REQUEST_ENTITY_TOO_LARGE         = 413,          /* 请求实体太大 */
    CC_ERR_SIP_REQUEST_URL_TOO_LONG             = 414,          /* 请求URL太长 */
    CC_ERR_SIP_UNSUPPORTED_MEDIA_TYPE           = 415,          /* 不支持的媒体类型 */
    CC_ERR_SIP_UNSUPPORTED_URL_SCHEME           = 416,          /* 不支持的URL计划 */
    CC_ERR_SIP_BAD_EXTENSION                    = 420,          /* 不良扩展 */
    CC_ERR_SIP_EXTENSION_REQUIRED               = 421,          /* 需要扩展 */
    CC_ERR_SIP_INTERVAL_TOO_BRIEF               = 423,          /* 间隔太短 */
    CC_ERR_SIP_TEMPORARILY_UNAVAILABLE          = 480,          /* 临时失效 */
    CC_ERR_SIP_CALL_TRANSACTION_NOT_EXIST       = 481,          /* 呼叫/事务不存在 */
    CC_ERR_SIP_LOOP_DETECTED                    = 482,          /* 发现环路 */
    CC_ERR_SIP_TOO_MANY_HOPS                    = 483,          /* 跳数太多 */
    CC_ERR_SIP_ADDRESS_INCOMPLETE               = 484,          /* 地址不完整 */
    CC_ERR_SIP_AMBIGUOUS                        = 485,          /* 不明朗 */
    CC_ERR_SIP_BUSY_HERE                        = 486,          /* 这里忙 */
    CC_ERR_SIP_REQUEST_TERMINATED               = 487,          /* 请求终止 */
    CC_ERR_SIP_NOT_ACCEPTABLE_HERE              = 488,          /* 这里请求不可接受 */
    CC_ERR_SIP_REQUEST_PENDING                  = 491,          /* 未决请求 */
    CC_ERR_SIP_UNDECIPHERABLE                   = 493,          /* 不可辨识 */
    CC_ERR_SIP_INTERNAL_SERVER_ERROR            = 500,          /* 服务器内部错误 */
    CC_ERR_SIP_NOT_IMPLEMENTED                  = 501,          /* 不可执行 */
    CC_ERR_SIP_BAD_GATEWAY                      = 502,          /* 坏网关 */
    CC_ERR_SIP_SERVICE_UNAVAILABLE              = 503,          /* 服务无效 */
    CC_ERR_SIP_SERVER_TIME_OUT                  = 504,          /* 服务器超时 */
    CC_ERR_SIP_VERSION_NOT_SUPPORTED            = 505,          /* 版本不支持 */
    CC_ERR_SIP_MESSAGE_TOO_LARGE                = 513,          /* 消息太大 */
    CC_ERR_SIP_BUSY_EVERYWHERE                  = 600,          /* 全忙 */
    CC_ERR_SIP_DECLINE                          = 603,          /* 丢弃 */
    CC_ERR_SIP_NOT_EXIST_ANYWHERE               = 604,          /* 不存在 */
    CC_ERR_SIP_NOT_ACCEPTABLE_606               = 606,          /* 不可接受 */


    /* SC 模块的错误码 */
    CC_ERR_SC_SERV_NOT_EXIST                    = 1000,         /* 业务不存在       403 */
    CC_ERR_SC_NO_SERV_RIGHTS                    = 1001,         /* 无业务权限       403 */
    CC_ERR_SC_USER_OFFLINE                      = 1002,         /* 用户离线         480 */
    CC_ERR_SC_USER_BUSY                         = 1003,         /* 用户忙           486 */
    CC_ERR_SC_USER_HAS_BEEN_LEFT                = 1004,         /* 用户已离开       480 */
    CC_ERR_SC_USER_DOES_NOT_EXIST               = 1005,         /* 用户不存在       403 */
    CC_ERR_SC_CUSTOMERS_NOT_EXIST               = 1006,         /* 客户不存在       403 */
    CC_ERR_SC_CB_ALLOC_FAIL                     = 1007,         /* 控制块分配失败   500 */
    CC_ERR_SC_MEMORY_ALLOC_FAIL                 = 1008,         /* 内存分配失败     500 */
    CC_ERR_SC_IN_BLACKLIST                      = 1009,         /* 黑名单           404 */
    CC_ERR_SC_CALLER_NUMBER_ILLEGAL             = 1010,         /* 主叫号码非法     404 */
    CC_ERR_SC_CALLEE_NUMBER_ILLEGAL             = 1011,         /* 被叫号码非法     404 */
    CC_ERR_SC_NO_ROUTE                          = 1012,         /* 无可用路由       404 */
    CC_ERR_SC_NO_TRUNK                          = 1013,         /* 无可用中继       404 */
    CC_ERR_SC_PERIOD_EXCEED                     = 1014,         /* 超出时间限制     480 */
    CC_ERR_SC_RESOURCE_EXCEED                   = 1015,         /* 超出资源限制     480 */
    CC_ERR_SC_CONFIG_ERR                        = 1016,         /* 数据配置错误     503 */
    CC_ERR_SC_MESSAGE_PARAM_ERR                 = 1017,         /* 消息参数错误     503 */
    CC_ERR_SC_MESSAGE_SENT_ERR                  = 1018,         /* 消息发送错误     503 */
    CC_ERR_SC_MESSAGE_RECV_ERR                  = 1019,         /* 消息接收错误     503 */
    CC_ERR_SC_MESSAGE_TIMEOUT                   = 1020,         /* 消息超时         408 */
    CC_ERR_SC_AUTH_TIMEOUT                      = 1021,         /* 认证超时         408 */
    CC_ERR_SC_QUERY_TIMEOUT                     = 1022,         /* 查询超时         408 */
    CC_ERR_SC_CLEAR_FORCE                       = 1023,         /* 强制拆除         503 */
    CC_ERR_SC_SYSTEM_ABNORMAL                   = 1024,         /* 系统异常         503 */
    CC_ERR_SC_SYSTEM_BUSY                       = 1025,         /* 系统忙           503 */
    CC_ERR_SC_SYSTEM_MAINTAINING                = 1026,         /* 系统维护         503 */
    CC_ERR_SC_FORBIDDEN                         = 1027,         /* 系统维护         403 */

    /* BS认证错误码 */
    CC_ERR_BS_HEAD                              = 1199,         /* BS错误码开始 */
    CC_ERR_BS_NOT_EXIST                         = 1200,         /* 不存在 */
    CC_ERR_BS_EXPIRE                            = 1201,         /* 过期/失效 */
    CC_ERR_BS_FROZEN                            = 1202,         /* 被冻结 */
    CC_ERR_BS_LACK_FEE                          = 1203,         /* 余额不足 */
    CC_ERR_BS_PASSWORD                          = 1204,         /* 密码错误 */
    CC_ERR_BS_RESTRICT                          = 1205,         /* 业务受限 */
    CC_ERR_BS_OVER_LIMIT                        = 1206,         /* 超过限制 */
    CC_ERR_BS_TIMEOUT                           = 1207,         /* 超时 */
    CC_ERR_BS_LINK_DOWN                         = 1208,         /* 连接中断 */
    CC_ERR_BS_SYSTEM                            = 1209,         /* 系统错误 */
    CC_ERR_BS_MAINTAIN                          = 1210,         /* 系统维护中 */
    CC_ERR_BS_DATA_ABNORMAL                     = 1211,         /* 数据异常*/
    CC_ERR_BS_PARAM_ERR                         = 1212,         /* 参数错误 */
    CC_ERR_BS_NOT_MATCH                         = 1213,         /* 不匹配 */

    /* DB 错误码 */
    CC_ERR_DB_ADD_FAIL                          = 1400,         /* 增 */
    CC_ERR_DB_DELETE_FAIL                       = 1401,         /* 删 */
    CC_ERR_DB_UPDATE_FAIL                       = 1402,         /* 改 */
    CC_ERR_DB_SELECT_FAIL                       = 1403,         /* 查 */

    CC_ERR_BUTT
};

typedef enum tagCallResult
{
    CC_RST_CONNECT_FAIL          = 0,  /* 未接通 */
    CC_RST_NOT_FOUND             = 1,  /* 空号 */
    CC_RST_REJECTED              = 2,  /* 呼叫被拒绝 */
    CC_RST_BUSY                  = 3,  /* 客户忙  */
    CC_RST_NO_ANSWER             = 4,  /* 客户未接听  */
    CC_RST_HANGUP_WHILE_IVR      = 5,  /* 播放语音时挂断 */
    CC_RST_HANGUP_AFTER_KEY      = 6,  /* 客户输入后挂断 */
    CC_RST_HANGUP_NO_ANSER       = 7,  /* 坐席还没接听就挂断了 */
    CC_RST_AGNET_BUSY            = 8,  /* 坐席全忙 */
    CC_RST_AGENT_NO_ANSER        = 9,  /* 坐席未接听 */
    CC_RST_QUEUE_TIMEOUT         = 10,  /* 排队超时挂断 */
    CC_RST_AGENT_HANGUP          = 11, /* 呼叫成功坐席挂断 */
    CC_RST_CUSTOMER_HANGUP       = 12, /* 呼叫成功客户挂断 */

    CC_RST_BUTT
}CC_CALL_RESULT_EN;

U32 sc_esl_reloadxml();

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* end of __SC_PUB_H__ */


