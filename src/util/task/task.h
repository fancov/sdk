/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  task.h
 *
 *  Created on: 2014-11-6
 *      Author: Larry
 *        Todo: 线程管理相关定义
 *     History:
 */

#ifndef __TASK_H___
#define __TASK_H___

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

typedef enum tagThreadStatus{
    THREAD_STATUS_UNINIT = 0,
    THREAD_STATUS_INITED,
    THREAD_STATUS_RUNING,
    THREAD_STATUS_WAITING_EXIT,
}THREAD_STATUS_EN;

typedef enum tagThreadPriority{
    THREAD_PRIORITY_HEIGHT  = 0,
    THREAD_PRIORITY_MID,
    THREAD_PRIORITY_LOW,
    THREAD_PRIORITY_DEFAULT
}THREAD_PRIORITY_EN;

typedef struct tagThreadMngtCB{
    /* 线程的初始化函数 ， 没有可置空*/
    U32 *(*init)();

    /* 线程的启动函数，在该函数中调用pthread_create函数，以创建线程️ */
    U32 *(*start)();

    /* 线程的停止函数， 没有可置空 */
    U32 *(*stop)();

    /* 设置线程的join模式， 没有可置空 */
    U32 *(*join)();

    /* 设置线程的别名 */
    S8 *pszThreadName;
}THREAD_MNGT_CB;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
