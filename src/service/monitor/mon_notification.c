#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_notification.h"

/**
 * 功能:发送短信通知客户
 * 参数集：
 *     参数1:S8 * pszMsg   需要发送的短信内容
 *     参数2:S8 * pszTelNo 电话号码
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_send_shortmsg(S8 * pszMsg, S8 * pszTelNo)
{
   /* 目前相关内容没有封装好，待后续实现 */
   return DOS_SUCC;
}

/**
 * 功能:拨打电话方式通知用户
 * 参数集：
 *     参数1:S8 * pszMsg    需要拨打电话的文本内容
 *     参数2:S8 * pszTelNo  电话号码
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_dial_telephone(S8 * pszMsg, S8 * pszTelNo)
{
   /* 目前相关内容没有封装好，待后续实现 */
   return DOS_SUCC;
}

/**
 * 功能:发送语音方式通知用户
 * 参数集：
 *     参数1:S8 * pszMsg       需要发送语音的文本的内容
 *     参数2:S8 * pszWechatNo  微信账号
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_send_audio(S8 * pszMsg, S8 * pszWechatNo)
{
   /* 目前相关内容没有封装好，待后续实现 */
   return DOS_SUCC;
}

/**
 * 功能:弹出Web告警方式通知用户
 * 参数集：
 *     参数1:S8 * pszMsg  需要发送的Web告警内容
 *     参数2:S8 * pszURL  Web的URL
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_send_web(S8 * pszMsg, S8 * pszURL)
{
   /* 目前相关内容没有封装好，待后续实现 */
   return DOS_SUCC;
}

/**
 * 功能:电子邮件方式通知用户
 * 参数集：
 *     参数1:S8 * pszMsg          需要发送的电子邮件内容
 *     参数2:S8 * pszEmailAddress 电子邮件地址
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_send_email(S8 * pszMsg, S8 * pszEmailAddress)
{
   /* 目前相关内容没有封装好，待后续实现 */
   return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

