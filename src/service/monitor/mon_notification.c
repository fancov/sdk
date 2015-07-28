#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_notification.h"
#include "mon_def.h"

/* 定义中文版标题与格式化内容的映射 */
MON_MSG_MAP_ST m_pstMsgMapCN[] = {
            {"lack_fee",   "余额不足", "尊敬的用户%s，截止%04u-%02u-%02u %02u:%02u:02u,你的余额为%s,请充值. 错误码:%x"},
            {"lack_gw",    "中继不足", "尊敬的用户%s,,您的中继数量为%u,请增加中继,以免影响业务,错误码:%x"},
            {"lack_route", "路由不足", "尊敬的用户%s,您的路由数量为%u，请增加路由以免影响业务,错误码:%x"}
        };

/* 定义英文版标题与格式化内容的映射表 */
MON_MSG_MAP_ST m_pstMsgMapEN[] = {
        {"lack_fee",   "No enough balance",  "Dear %s, by the time %04u-%02u-%02u %02u:%02u:02u,your balance is %.2f,please recharge, thank you! errno:%x."},
        {"lack_gw",    "No enough gateway",  "Dear %s,you have %u gateway(s) in total,please buy new ones to avoid your services being affacted,thank you! errno:%x."},
        {"lack_route", "No enough route",    "Dear %s,you have %u route(s) in total,please buy new ones to avoid your services being affacted,thank you! errno:%x."}
     };

/* 告警手段与行为映射表 */
MON_NOTIFY_MEANS_CB_ST m_pstNotifyMeansCB[] = {
        {MON_NOTIFY_MEANS_EMAIL, mon_send_email},
        {MON_NOTIFY_MEANS_SMS,   mon_send_sms},
        {MON_NOTIFY_MEANS_AUDIO, mon_send_audio}
     };


/**
 * 函数: U32 mon_send_sms(S8 * pszMsg, S8 * pszTelNo)
 * 参数:
 *     S8*  pszMsg    短信内容
 *     S8*  pszTitle
 *     S8*  pszTelNo  接收短信的号码
 * 功能: 发送短信
 */
U32 mon_send_sms(S8 * pszMsg, S8* pszTitle,S8 * pszTelNo)
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
U32 mon_send_audio(S8 * pszMsg, S8* pszTitle, S8 * pszTelNo)
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

    if (DOS_ADDR_INVALID(pszMsg)
        || DOS_ADDR_INVALID(pszTitle)
        || DOS_ADDR_INVALID(pszEmailAddress))
    {
        DOS_ASSERT(0);
        mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Msg:%p; Title:%p; Email Address:%p.", pszMsg, pszTitle, pszEmailAddress);
        return DOS_FAIL;
    }

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

