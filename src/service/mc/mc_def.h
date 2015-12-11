#ifndef __MC_DEF__
#define __MC_DEF__


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* 录音文件路径 */
#define MC_RECORD_FILE_PATH       "/ipcc/var/data/voicerecord"

/* 录音文件转换工具路径 */
#define MC_SCRIPT_PATH            "/usr/bin/codec_convert.sh"

/* 最大转化线程数 */
#define MC_MAX_WORKING_TASK_CNT   2

/* 最大等待时长，如果在如下时间内CPU占用持续在20%以下，就可以继续执行转化任务 */
#define MC_WAITING_TIME           60

/* 工作任务每次加载的数据量 */
#define MC_MAX_DATA_PRE_TIME      100

/* 文件名最大长度 */
#define MC_MAX_FILENAME_LEN       128

#define MC_ROOT_UID               0
#define MC_NOBODY_UID             99

/* 链表中每个节点的内存大小 */
#define MC_MAX_FILENAME_BUFF_LEN  (MC_MAX_FILENAME_LEN+(sizeof(DLL_NODE_S)))

typedef enum tagMCTaskWorkingStatus
{
    /* 转换任务状态，可以工作 */
    MC_WORKING_STATUS_WORKING,

    /* 转换任务状态，等待空闲 */
    MC_WORKING_STATUS_WAITING,
}MC_WORKING_STATUS_EN;

typedef enum tagMCTaskStatus
{
    /* 工作任务状态: 初始化转改 */
    MC_TSK_STATUS_INIT,

    /* 工作任务状态: 运行状态 */
    MC_TSK_STATUS_RUNNING,

    /* 工作任务状态: 等待数据 */
    MC_TSK_STATUS_WAITING,

    /* 工作任务状态: 退出状态 */
    MC_TSK_STATUS_EXITED,
}MC_TASK_STATUS_EN;


typedef struct tagMCServerCB
{
    U32              ulStatus;   /* Refer to MC_TASK_STATUS_EN */

    pthread_t        pthServer;  /* 线程句柄 */
    pthread_mutex_t  mutexQueue; /* 队列的锁 */
    pthread_cond_t   condQueue;  /* 队列条件变量 */
    DLL_S            stQueue;    /* 工作队列 */

    U32              ulTotalProc;
    U32              ulSuccessProc;
}MC_SERVER_CB;

VOID mc_log(BOOL blMaster, U32 ulLevel, S8 *szFormat, ...);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __MC_DEF__ */



