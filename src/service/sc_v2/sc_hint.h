/**
 * @file : sc_hint.h
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 业务控制模块放音相关功能定义
 *
 *
 * @date: 2016年1月9日
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#ifndef __SC_HINT_H__
#define __SC_HINT_H__

#define SC_HINT_FILE_PATH       "/usr/local/freeswitch/sounds/okcc"


#define SC_HINT_LANG_CN         1

typedef enum tagSoundType{
    SC_SND_CALL_OVER         = 0, /**< 标记提示音 */
    SC_SND_INCOMING_CALL_TIP,     /**< 长签时，呼叫提示音 */
    SC_SND_LOW_BALANCE,           /**< 你的余额已不足 */
    SC_SND_YOUR_BALANCE,          /**< 你的余额为 */
    SC_SND_MUSIC_HOLD,            /**< 保持音 */
    SC_SND_MUSIC_SIGNIN,          /**< 长签音音 */
    SC_SND_NETWORK_FAULT,         /**< 网络故障 */
    SC_SND_NO_PERMISSION,         /**< 无业务权限 */
    SC_SND_SET_SUCC,              /**< 设置成功 */
    SC_SND_SET_FAIL,              /**< 设置失败 */
    SC_SND_OPT_SUCC,              /**< 操作正常 */
    SC_SND_OPT_FAIL,              /**< 操作失败 */
    SC_SND_SYS_MAINTAIN,          /**< 系统维护 */
    SC_SND_TMP_UNAVAILABLE,       /**< 你所拨打的用户暂时无法接通，请稍后再拨 */
    SC_SND_USER_BUSY,             /**< 你所拨打的用户忙，请稍后再拨 */
    SC_SND_USER_LINE_FAULT,       /**< 用户线忙 */
    SC_SND_USER_NOT_FOUND,        /**< 你所拨打的用户忙不存在，请查证后再拨 */
    SC_SND_CONNECTING,            /**< 正在为你接通 */
    SC_SND_1_YAO,
    SC_SND_0,
    SC_SND_1,
    SC_SND_2,
    SC_SND_3,
    SC_SND_4,
    SC_SND_5,
    SC_SND_6,
    SC_SND_7,
    SC_SND_8,
    SC_SND_9,

    SC_SND_BUTT
}SC_SOUND_TYPE_EN;

typedef enum tagToneType{
    SC_TONE_RINGBACK,
    SC_TONE_DIAL,
    SC_TONE_RING,

    SC_TONE_BUTT
}SC_TONE_TYPE_EN;

typedef struct tagSCHintDesc{
    U32 ulIndex;
    S8  *pszName;
}SC_HINT_DESC_ST;


S8 *sc_hint_get_name(U32 ulSndInd);
S8 *sc_hine_get_tone(U32 ulSNDInd);

#endif

#ifdef __cplusplus
}
#endif /* End of __cplusplus */

