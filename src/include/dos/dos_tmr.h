/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_tmr.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: DOS定时器模块给提供的公共头文件
 *     History:
 */

#ifndef __DOS_TMR_H__
#define __DOS_TMR_H__

#if INCLUDE_SERVICE_TIMER

typedef enum tagTimerType{
    TIMER_NORMAL_NO_LOOP = 0,
    TIMER_NORMAL_LOOP,

    TIMER_NORMAL_BUTT,
}TIME_TYPE_E;

typedef enum tagTimerStatus{
    TIMER_STATUS_WAITING_ADD = 0,   /* 在等待添加队列里面 */
    TIMER_STATUS_WAITING_DEL,       /* 在工作队列中，等待被删除 */
    TIMER_STATUS_WAITING_RESET,     /* 在工作队列中，等待恢复 */
    TIMER_STATUS_WORKING,           /* 等待超时 */
    TIMER_STATUS_TIMEOUT,           /* 超时 */

    TIMER_STATUS_BUTT,
}TIMER_STATUS_E;

typedef struct tagTimerHandle{
    U64          ulData;           /* 定时器参数 TIMER_STATUS_E*/
    U32          ulTmrStatus;      /* 定时器状态 refer to  */
    TIME_TYPE_E  eType;            /* 定时器类型 refer to TIME_TYPE_E */
    S64          lInterval;        /* 定时器超时间隔 */
    VOID         (*func)(U64 );    /* 定时器回调函数 */

    struct tagTimerHandle **hTmr;  /* handle所在地址 */
}TIMER_HANDLE_ST;

#define DOS_TMR_ST TIMER_HANDLE_ST*

S32 tmr_task_init();
S32 tmr_task_start();
S32 tmr_task_stop();
S32 dos_tmr_start(DOS_TMR_ST *hTmrHandle, U32 ulInterval, VOID (*func)(U64 ), U64 uLParam, U32 ulType);
S32 dos_tmr_stop(DOS_TMR_ST *hTmrHandle);

#endif

#endif

