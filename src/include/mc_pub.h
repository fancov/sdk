#ifndef __MC_PUB__
#define __MC_PUB__


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifdef INCLUDE_SERVICE_MC

/* 初始化媒体转换任务 */
U32 mc_init();

/* 启动媒体转换任务 */
U32 mc_start();

/* 停止媒体转换任务 */
U32 mc_stop();

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __MC_PUB__ */


