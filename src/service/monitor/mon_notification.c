#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_notification.h"

/**
 * 函数: U32 mon_send_shortmsg(S8 * pszMsg, S8 * pszTelNo)
 * 参数:
 *     S8*  pszMsg    短信内容
 *     S8*  pszTelNo  接收短信的号码
 * 功能: 发送短信
 */
U32 mon_send_shortmsg(S8 * pszMsg, S8 * pszTelNo)
{
    return DOS_SUCC;
}

/**
 * 函数: U32 mon_dial_telephone(S8 * pszMsg, S8 * pszTelNo)
 * 参数:
 *     S8* pszMsg    电话内容
 *     S8* pszTelNo  电话号码
 * 返回:
 *     成功返回DOS_SUCC,反之返回DOS_FAIL
 */
U32 mon_dial_telephone(S8 * pszMsg, S8 * pszTelNo)
{
    return DOS_SUCC;
}

/**
 * 函数: U32 mon_send_audio(S8 * pszMsg, S8 * pszWechatNo)
 * 参数:
 *     S8 * pszMsg       语音内容
 *     S8 * pszWechatNo  微信号码
 * 返回:
 *   成功返回DOS_SUCC,反之返回DOS_FAIL
 */
U32 mon_send_audio(S8 * pszMsg, S8 * pszWechatNo)
{
    return DOS_SUCC;
}

/**
 * 函数:U32 mon_send_web(S8 * pszMsg, S8 * pszURL)
 * 参数:
 *     S8 * pszMsg  信息内容
 *     S8 * pszURL  Web的URL
 * 返回:
 *   成功返回DOS_SUCC,反之返回DOS_FAIL
 */
U32 mon_send_web(S8 * pszMsg, S8 * pszURL)
{
    return DOS_SUCC;
}

/**
 * 函数:U32 mon_send_email(S8* pszMsg, S8* pszTitle,S8* pszEmailAddress)
 * 参数:
 *     S8 * pszMsg          邮件内容
 *     S8 * pszTitle        邮件主题
 *     S8 * pszEmailAddress 邮件目的地址
 * 返回:
 *   成功返回DOS_SUCC,反之返回DOS_FAIL
 */
U32 mon_send_email(S8* pszMsg, S8* pszTitle,S8* pszEmailAddress)
{
    S8 szBuffCmd[1024] = {0};
    FILE * fp = NULL;

    dos_snprintf(szBuffCmd, sizeof(szBuffCmd), "echo %s | mail -s %s %s", pszMsg, pszTitle, pszEmailAddress);
    fp = popen(szBuffCmd, "r");
    if (DOS_ADDR_INVALID(fp))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pclose(fp);
    fp = NULL;
    return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

