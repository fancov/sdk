/**
 * @file : sc_pthread.h
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 线程库中的一些函数定义
 *
 * @date: 2016年1月15日
 * @arthur: Larry
 */

#ifndef __DOS_PTHREAD_H__
#define __DOS_PTHREAD_H__

#include <pthread.h>

#ifndef __const
#define __const const
#endif

#ifndef __restrict
#define __restrict
#endif

#define pthread_t pthread_t
#define pthread_attr_t pthread_attr_t
#define pthread_mutex_t pthread_mutex_t
#define pthread_mutexattr_t pthread_mutexattr_t
#define pthread_cond_t pthread_cond_t
#define pthread_condattr_t pthread_condattr_t

#define DOS_PTHREAD_NAME_LEN          64

#ifndef PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_MUTEX_INITIALIZER  { { 0, 0, 0, 0, 0, (void *) 0, 0, 0 } }
#endif
#ifndef PTHREAD_COND_INITIALIZER
#define PTHREAD_COND_INITIALIZER { { 0, 0, 0, 0, 0, (void *) 0, 0, 0 } }
#endif

extern int pthread_create(pthread_t *__restrict __newthread
                            , __const pthread_attr_t * __attr
                            , void *(*__start_routine) (void *)
                            , void *__restrict __arg);

extern int pthread_mutex_init(pthread_mutex_t *__mutex, __const pthread_mutexattr_t *__mutexattr);

extern int pthread_mutex_destroy (pthread_mutex_t *__mutex);
extern int pthread_mutex_trylock (pthread_mutex_t *__mutex);
extern int pthread_mutex_lock (pthread_mutex_t *__mutex);
extern int pthread_mutex_unlock (pthread_mutex_t *__mutex);

extern int pthread_mutexattr_init (pthread_mutexattr_t *__attr);
extern int pthread_mutexattr_destroy (pthread_mutexattr_t *__attr);


extern int pthread_cond_init (pthread_cond_t *__restrict __cond,
                              __const pthread_condattr_t *__restrict __cond_attr);
extern int pthread_cond_destroy (pthread_cond_t *__cond);
extern int pthread_cond_signal (pthread_cond_t *__cond);
extern int pthread_cond_broadcast (pthread_cond_t *__cond);

extern int pthread_cond_wait (pthread_cond_t *__restrict __cond,
                              pthread_mutex_t *__restrict __mutex);

extern int pthread_cond_timedwait (pthread_cond_t *__restrict __cond,
                                   pthread_mutex_t *__restrict __mutex,
                                   __const struct timespec *__restrict __abstime);

extern int pthread_condattr_init (pthread_condattr_t *__attr);

extern int pthread_condattr_destroy (pthread_condattr_t *__attr);


typedef struct tagSCPthreadMsg
{
    BOOL        bIsValid;
    pthread_t   ulPthID;
    time_t      ulLastTime;

    VOID        *pParam;
    VOID        *(*func)(VOID *);

    S8          szName[DOS_PTHREAD_NAME_LEN];

}SC_PTHREAD_MSG_ST;

SC_PTHREAD_MSG_ST *dos_pthread_cb_alloc();

U32 dos_pthread_init();
U32 dos_pthread_start();

#endif /* __DOS_PTHREAD_H__ */

