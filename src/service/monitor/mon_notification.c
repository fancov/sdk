#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_notification.h"
#include "mon_def.h"
#include "../../util/heartbeat/heartbeat.h"

/* 目前认为所有邮件的发件人是运营商 */
#define MSGBODY "From:%s\r\n" \
                "To:%s\r\n" \
                "Content-type: text/html;charset=gb2312\r\n" \
                "Subject: %s\r\n" \
                "<p>%s</p>"

extern MON_MSG_MAP_ST m_pstMsgMap;
extern U32 mon_get_sp_email(S8 *pszEmail);

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
    S8 szSPEmail[32] = {0}, szEmailMsg[1024] = {0}, szCmdBuff[1024] = {0}, szFile[] = "mail.txt";
    U32 ulRet = 0;

    ulRet = mon_get_sp_email(szSPEmail);
    if (DOS_SUCC != ulRet)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 格式化邮件 */
    dos_snprintf(szEmailMsg, sizeof(szEmailMsg), MSGBODY, szSPEmail, pszEmailAddress, pszTitle, pszMsg);
    /* 格式化命令 */
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), " echo \"%s\" > %s", szEmailMsg, szFile);
    system(szCmdBuff);

    dos_memzero(szCmdBuff, sizeof(szCmdBuff));
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "sendmail -t %s < %s", pszEmailAddress, szFile);
    system(szCmdBuff);

    unlink(szFile);

    return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

