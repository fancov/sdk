/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_log.h
 *
 *  Created on: 2015年9月4日10:19:31
 *      Author: Larry
 *        Todo: log模块私有头文件
 *     History:
 */

#ifndef __LOG_DEF_H__
#define __LOG_DEF_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* 操作对象缓存长度 */
#define MAX_OPERAND_LENGTH    32

/* 操作员缓存长度 */
#define MAX_OPERATOR_LENGTH   32

/* 日志最大长度，大于这个长度将被截断 */
#define LOG_MAX_LENGTH        1000

#define MAX_DB_INFO_LEN       100

enum tagLogMod{
    LOG_MOD_CONSOLE,
    LOG_MOD_CLI,
    LOG_MOD_DB,

    LOG_MOD_BUTT
}LOG_MOD_LIST;


/* 日志模块初始化函数类型 */
typedef U32 (*log_init)();

/* 日志模块启动函数类型 */
typedef U32 (*log_start)();

/* 日志模块停止函数类型 */
typedef U32 (*log_stop)();

/* 设置告警基本函数 */
typedef U32 (*log_set_level)(U32);

/* 日志模块写函数类型 */
typedef VOID (*log_write_rlog)(time_t, const S8 *, const S8 *, const S8 *, U32);

/* 日志模块写函数类型 */
typedef VOID (*log_write_olog)(const S8 *, const S8 *, const S8 *, const S8*, const S8 *);

typedef struct tagLogModule
{
    BOOL blInited;                      /* 是否被初始化了 */
    BOOL blIsRunning;                   /* 是否在正常运行 */
    BOOL blWaitingExit;                 /* 是否在等待退出 */

    U32 ulCurrentLevel;                 /* 当前log级别 */

    log_init        fnInit;            /* 模块初始化函数 */
    log_start       fnStart;           /* 模块启动函数 */
    log_stop        fnStop;            /* 模块停止函数 */
    log_write_rlog  fnWriteRlog;       /* 模块写运行日志 */
    log_write_olog  fnWriteOlog;       /* 模块写操作日志 */
    log_set_level   fnSetLevel;
}DOS_LOG_MODULE_ST;


/* 日志模块注册函数 */
typedef U32 (*log_reg)(DOS_LOG_MODULE_ST **);
typedef struct tagLogModuleNode
{
    U32                ulLogModule;    /* 模块编号 */
    log_reg            fnReg;          /* 模块注册函数 */
    DOS_LOG_MODULE_ST  *pstLogModule;  /* 模块控制块 */
}DOS_LOG_MODULE_NODE_ST;

typedef struct tagLogDataNode
{
    DLL_NODE_S  stNode;
    time_t stTime;
    S32    lLevel;
    S32    lType;
    S8     *pszMsg;

    S8     szOperand[MAX_OPERAND_LENGTH];
    S8     szOperator[MAX_OPERATOR_LENGTH];
    U32    ulResult;
}LOG_DATA_NODE_ST;


U32 log_console_reg(DOS_LOG_MODULE_ST **pstModuleCB);
U32 log_db_reg(DOS_LOG_MODULE_ST **pstModuleCB);
U32 log_cli_reg(DOS_LOG_MODULE_ST **pstModuleCB);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* end of __LOG_DEF_H__ */

