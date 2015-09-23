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
    SC_ERR_SUCC                             = 200,          /* 成功 */
    SC_ERR_BAD_REQUEST                      = 400,          /* 错误请求 */
    SC_ERR_UNAUTHORIZED                     = 401,          /* 未授权 */
    SC_ERR_PAYMENT_REQUIRED                 = 402,          /* 付费要求 */
    SC_ERR_FORBIDDEN                        = 403,          /* 禁止 */
    SC_ERR_NOT_FOUND                        = 404,          /* 未发现 */
    SC_ERR_METHOD_NO_ALLOWED                = 405,          /* 方法不允许 */
    SC_ERR_NOT_ACCEPTABLE                   = 406,          /* 不可接受 */
    SC_ERR_PROXY_AUTHENTICATION_REQUIRED    = 407,          /* 代理需要认证 */
    SC_ERR_REQUEST_TIMEOUT                  = 408,          /* 请求超时 */
    SC_ERR_GONE                             = 410,          /* 离开 */
    SC_ERR_REQUEST_ENTITY_TOO_LARGE         = 413,          /* 请求实体太大 */
    SC_ERR_REQUEST_URL_TOO_LONG             = 414,          /* 请求URL太长 */
    SC_ERR_UNSUPPORTED_MEDIA_TYPE           = 415,          /* 不支持的媒体类型 */
    SC_ERR_UNSUPPORTED_URL_SCHEME           = 416,          /* 不支持的URL计划 */
    SC_ERR_BAD_EXTENSION                    = 420,          /* 不良扩展 */
    SC_ERR_EXTENSION_REQUIRED               = 421,          /* 需要扩展 */
    SC_ERR_INTERVAL_TOO_BRIEF               = 423,          /* 间隔太短 */
    SC_ERR_TEMPORARILY_UNAVAILABLE          = 480,          /* 临时失效 */
    SC_ERR_CALL_TRANSACTION_DOES_NOT_EXIST  = 481,          /* 呼叫/事务不存在 */
    SC_ERR_LOOP_DETECTED                    = 482,          /* 发现环路 */
    SC_ERR_TOO_MANY_HOPS                    = 483,          /* 跳数太多 */
    SC_ERR_ADDRESS_INCOMPLETE               = 484,          /* 地址不完整 */
    SC_ERR_AMBIGUOUS                        = 485,          /* 不明朗 */
    SC_ERR_BUSY_HERE                        = 486,          /* 这里忙 */
    SC_ERR_REQUEST_TERMINATED               = 487,          /* 请求终止 */
    SC_ERR_NOT_ACCEPTABLE_HERE              = 488,          /* 这里请求不可接受 */
    SC_ERR_REQUEST_PENDING                  = 491,          /* 未决请求 */
    SC_ERR_UNDECIPHERABLE                   = 493,          /* 不可辨识 */
    SC_ERR_SERVER_INTERNAL_ERROR            = 500,          /* 服务器内部错误 */
    SC_ERR_NOT_IMPLEMENTED                  = 501,          /* 不可执行 */
    SC_ERR_BAD_GATEWAY                      = 502,          /* 坏网关 */
    SC_ERR_SERVICE_UNAVAILABLE              = 503,          /* 服务无效 */
    SC_ERR_SERVER_TIME_OUT                  = 504,          /* 服务器超时 */
    SC_ERR_VERSION_NOT_SUPPORTED            = 505,          /* 版本不支持 */
    SC_ERR_MESSAGE_TOO_LARGE                = 513,          /* 消息太大 */
    SC_ERR_BUSY_EVERYWHERE                  = 600,          /* 全忙 */
    SC_ERR_DECLINE                          = 603,          /* 丢弃 */
    SC_ERR_DOES_NOT_EXIST_ANYWHERE          = 604,          /* 不存在 */
    SC_ERR_NOT_ACCEPTABLE_606               = 606,          /* 不可接受 */

    SC_ERR_BUTT
};


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* end of __SC_PUB_H__ */


