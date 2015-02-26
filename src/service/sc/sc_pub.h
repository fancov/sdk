/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_pub.h
 *
 *  创建时间: 2014年12月25日17:35:03
 *  作    者: Larry
 *  描    述: 定义业务控制模块的公共数据
 *  修改历史:
 */

#ifndef __SC_PUB_H__
#define __SC_PUB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <pthread.h>
#include <semaphore.h>


/* include private header files */

/* define marcos */
/* 定义HTTP服务器最大个数 */
#define SC_MAX_HTTPD_NUM               1


/* 定义最多有16个客户端连接HTTP服务器 */
#define SC_MAX_HTTP_CLIENT_NUM         16

/* 定义HTTP API中最大参数个数 */
#define SC_API_PARAMS_MAX_NUM          24

/* 定有HTTP请求行中，文件名的最大长度 */
#define SC_HTTP_REQ_MAX_LEN            64

/* 定义ESL模块命令的最大长度 */
#define SC_ESL_CMD_BUFF_LEN            1024

/* 定义呼叫模块，最大放音次数 */
#define SC_DEFAULT_PLAYCNT             3

/* 定义呼叫模块，最大无人接听超时时间 */
#define SC_MAX_TIMEOUT4NOANSWER        10

/* 电话号码长度 */
#define SC_TEL_NUMBER_LENGTH           24

/* 工号长度最大值 */
#define SC_EMP_NUMBER_LENGTH           8

/* 每个任务最多时间段描述节点 */
#define SC_MAX_PERIOD_NUM              4

/* UUID 最大长度 */
#define SC_MAX_UUID_LENGTH             40

/* 语言文件名长度 */
#define SC_MAX_AUDIO_FILENAME_LEN      128

/* 比例呼叫的比例 */
#define SC_MAX_CALL_MULTIPLE           3

#define SC_MAX_SRV_TYPE_PRE_LEG        4

/* 最大呼叫数量 */
#define SC_MAX_CALL                    3000

/* 最大任务数 */
#define SC_MAX_TASK_NUM                1024

/* 最大呼叫数 */
#define SC_MAX_SCB_NUM                 SC_MAX_CALL*2

/* SCB hash表最大数 */
#define SC_MAX_SCB_HASH_NUM            4096

/* 每个任务的坐席最大数 */
#define SC_MAX_SITE_NUM                1024

/* 每个任务主叫号码最大数量 */
#define SC_MAX_CALLER_NUM              1024

/* 每个任务被叫号码最小数量(一次LOAD的最大数量，实际会根据实际情况计算) */
#define SC_MIN_CALLEE_NUM              65535

/* 定义一个呼叫最大可以持续时长。12小时(43200s) */
#define SC_MAX_CALL_DURATION           43200

/* 允许同一个号码被重复呼叫的最大时间间隔，4小时 */
#define SC_MAX_CALL_INTERCAL           60 * 4

#define SC_CALL_THRESHOLD_VAL0         40         /* 阀值0，百分比 ，需要调整呼叫发起间隔*/
#define SC_CALL_THRESHOLD_VAL1         80         /* 阀值1，百分比 ，需要调整呼叫发起间隔*/

#define SC_CALL_INTERVAL_VAL0          300        /* 当前呼叫量/最大呼叫量*100 < 阀值0，300毫秒一个 */
#define SC_CALL_INTERVAL_VAL1          500        /* 当前呼叫量/最大呼叫量*100 < 阀值1，500毫秒一个 */
#define SC_CALL_INTERVAL_VAL2          1000       /* 当前呼叫量/最大呼叫量*100 > 阀值1，1000毫秒一个 */

#define SC_MEM_THRESHOLD_VAL0          90         /* 系统状态阀值，内存占用率阀值0 */
#define SC_MEM_THRESHOLD_VAL1          95         /* 系统状态阀值，内存占用率阀值0 */
#define SC_CPU_THRESHOLD_VAL0          90         /* 系统状态阀值，CPU占用率阀值0 */
#define SC_CPU_THRESHOLD_VAL1          95         /* 系统状态阀值，CPU占用率阀值0 */

/* define enums */

/* define structs */

/* declare functions */
#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __SC_HTTPD_PUB_H__ */


