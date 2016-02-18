/**
 * @file : sc_hint.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 业务控制模块放音相关功能辅助函数
 *
 *
 * @date: 2016年1月9日
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_hint.h"


#if SC_HINT_LANG_CN
SC_HINT_DESC_ST  astHitList[] = {
    {SC_SND_CALL_OVER,                 "call_over"},
    {SC_SND_INCOMING_CALL_TIP,         "incoming_tip"},
    {SC_SND_LOW_BALANCE,               "low_balance"},
    {SC_SND_YOUR_BALANCE,              "nindyew"},
    {SC_SND_MUSIC_HOLD,                "music_hold"},
    {SC_SND_MUSIC_SIGNIN,              "music_signin"},
    {SC_SND_NETWORK_FAULT,             "network_fault"},
    {SC_SND_NO_PERMISSION,             "no_permission_for_service"},
    {SC_SND_SET_SUCC,                  "set_succ"},
    {SC_SND_SET_FAIL,                  "set_fail"},
    {SC_SND_OPT_SUCC,                  "opt_succ"},
    {SC_SND_OPT_FAIL,                  "opt_fail"},
    {SC_SND_SYS_MAINTAIN,              "sys_in_maintain"},
    {SC_SND_TMP_UNAVAILABLE,           "temporarily_unavailable"},
    {SC_SND_USER_BUSY,                 "user_busy"},
    {SC_SND_USER_LINE_FAULT,           "user_line_fault"},
    {SC_SND_USER_NOT_FOUND,            "user_not_found"},
    {SC_SND_CONNECTING,                "connecting"},
    {SC_SND_YOUR_CODE_IS,              "nindyzm"},
    {SC_SND_1_YAO,                     "1yao"},
    {SC_SND_0,                         "0"},
    {SC_SND_1,                         "1"},
    {SC_SND_2,                         "2"},
    {SC_SND_3,                         "3"},
    {SC_SND_4,                         "4"},
    {SC_SND_5,                         "5"},
    {SC_SND_6,                         "6"},
    {SC_SND_7,                         "7"},
    {SC_SND_8,                         "8"},
    {SC_SND_9,                         "9"},
    {SC_SND_FU,                        "fu"},
    {SC_SND_WAN,                       "wan"},
    {SC_SND_QIAN,                      "qian"},
    {SC_SND_BAI,                       "bai"},
    {SC_SND_SHI,                       "shi"},
    {SC_SND_YUAN,                      "yuan"},
    {SC_SND_JIAO,                      "jiao"},
    {SC_SND_FEN,                       "fen"},
    {SC_SND_BUTT,                      NULL}
};

SC_HINT_DESC_ST  astToneList[] = {
    {SC_TONE_RINGBACK,         "tone_stream://%(1000,4000,450);loops=-1"},
    {SC_TONE_DIAL,             ""},
    {SC_TONE_RING,             ""},

    {SC_TONE_BUTT,             ""}
};

#endif

S8 *sc_hint_get_name(U32 ulSndInd)
{
    if (ulSndInd >= SC_SND_BUTT)
    {
        return NULL;
    }

    return astHitList[ulSndInd].pszName;
}

S8 *sc_hine_get_tone(U32 ulSNDInd)
{
    if (ulSNDInd > SC_TONE_BUTT)
    {
        return NULL;
    }

    return astToneList[ulSNDInd].pszName;
}


#ifdef __cplusplus
}
#endif /* End of __cplusplus */

