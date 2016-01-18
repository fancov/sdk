#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_notification.h"
#include "mon_def.h"
#include "../../util/heartbeat/heartbeat.h"
#include "mon_mail.h"

const S8* m_szBase64Char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
extern MON_MSG_MAP_ST m_pstMsgMap;
extern S32  g_lMailSockfd;
extern U32 mon_get_sp_email(MON_NOTIFY_MSG_ST *pstMsg);
extern U32 mon_cli_conn_init();
//static S32 base64_encode(const U8* ulBinData, S8* szBase64, S32 ulBinLen);
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
 * 函数: U32  mon_send_email(U32 ulCustomerID, S8 *pszTitle, S8 *pszMessage)
 * 参数:
 *     S8 * pszTitle     邮件标题
 *     S8 * pszMessage   邮件内容
 * 返回:
 *   成功返回DOS_SUCC,反之返回DOS_FAIL
 */
U32  mon_send_email(U32 ulCustomerID, S8 *pszTitle, S8 *pszMessage, U32 ulWarnLevel, U32 ulWarnType)
{
    S32  lRet;
    U32  ulRet = U32_BUTT;
    MON_NOTIFY_MSG_ST   stMsg = {0};

    if (DOS_ADDR_INVALID(pszTitle)
        || DOS_ADDR_INVALID(pszMessage)
        || ulWarnLevel >= MON_WARNING_LEVEL_BUTT
        || ulWarnType >= MON_WARNING_TYPE_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 获取邮件联系人，保留 */
    stMsg.ulCustomerID = ulCustomerID;
    ulRet = mon_get_sp_email(&stMsg);
    if (ulRet != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    /* 发邮件 */
    //lRet = mon_stream_client(stMsg.stContact.szEmail, pszTitle, pszMessage);
    lRet = mon_mail_send_warning(ulWarnType, ulWarnLevel, stMsg.stContact.szEmail, pszTitle, pszMessage);
    if (DOS_SUCC != lRet)
    {
        mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "Send Email FAIL.");
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

/**
 *  函数: S32 base64_encode( const U8* szBinData, S8* szBase64, S32 lBinLen)
 *  功能: 对输入的参数进行base64编码
 *  参数:
 *        const U8* szBinData  需要进行编码的参数
 *        S8* szBase64         编码后的输出参数
 *        S32 lBinLen          编码参数的长度
 *  返回值:
 *      成功则返回DOS_SUCC，否则返回DOS_FAIL
 */
S32 base64_encode( const U8* szBinData, S8* szBase64, S32 lBinLen)
{
    S32 i, j;
    U8  ucCurrent;

    for ( i = 0, j = 0 ; i < lBinLen ; i += 3 )
    {
        ucCurrent = (szBinData[i] >> 2) ;
        ucCurrent &= (U8)0x3F;
        szBase64[j++] = m_szBase64Char[(S32)ucCurrent];

        ucCurrent = ((U8)(szBinData[i] << 4 ) ) & ( (U8)0x30);
        if ( i + 1 >= lBinLen )
        {
            szBase64[j++] = m_szBase64Char[(S32)ucCurrent];
            szBase64[j++] = '=';
            szBase64[j++] = '=';
            break;
        }
        ucCurrent |= ((U8)(szBinData[i+1] >> 4)) & ((U8)0x0F);
        szBase64[j++] = m_szBase64Char[(S32)ucCurrent];

        ucCurrent = ((U8)(szBinData[i+1] << 2)) & ((U8)0x3C) ;
        if ( i + 2 >= lBinLen )
        {
            szBase64[j++] = m_szBase64Char[(S32)ucCurrent];
            szBase64[j++] = '=';
            break;
        }
        ucCurrent |= ( (U8)(szBinData[i+2] >> 6) ) & ((U8)0x03);
        szBase64[j++] = m_szBase64Char[(S32)ucCurrent];

        ucCurrent = ((U8)szBinData[i+2] ) & ((U8)0x3F) ;
        szBase64[j++] = m_szBase64Char[(S32)ucCurrent];
    }
    szBase64[j] = '\0';
    return DOS_SUCC;
}


/**
 * 函数: U32  mon_stream_client(S8 *pszAddress, S8 *pszTitle, S8 *pszMessage)
 * 功能: 推送邮件
 * 参数:
 *      S8 *pszAddress  邮件地址
 *      S8 *pszTitle    邮件标题
 *      S8 *pszMessage  邮件内容
 * 返回:
 *      成功返回DOS_SUCC,否则返回DOS_FAIL
 */
U32  mon_stream_client(S8 *pszAddress, S8 *pszTitle, S8 *pszMessage)
{
    S32 lRet;
    S8  szMailServ[64] = {0}, szUsername[64] = {0}, szPasswd[64] = {0};
    S8  szBase64Usrname[64] = {0}, szBase64Passwd[64] = {0};
    S8  szBuff[512] = {0}, szRecvBuff[256] = {0};

    if (DOS_ADDR_INVALID(pszAddress)
        || DOS_ADDR_INVALID(pszTitle)
        || DOS_ADDR_INVALID(pszMessage))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    lRet = config_hb_get_mail_server(szMailServ, sizeof(szMailServ));
    if (lRet < 0)
    {
        mon_trace(MON_TRACE_NOTIFY,  LOG_LEVEL_ERROR, "Get default SMTP Server FAIL.");
        return DOS_FAIL;
    }
    /* 获取默认的邮箱账户认证名称 */
    lRet = config_hb_get_auth_username(szUsername, sizeof(szUsername));
    if (lRet < 0)
    {
        mon_trace(MON_TRACE_NOTIFY,  LOG_LEVEL_ERROR, "Get Default Auth Username FAIL.");
        return DOS_FAIL;
    }
    /* 获取默认的邮箱账户额认证密码 */
    lRet = config_hb_get_auth_passwd(szPasswd, sizeof(szPasswd));
    if (lRet < 0)
    {
        mon_trace(MON_TRACE_NOTIFY,  LOG_LEVEL_ERROR, "Get Default Auth Password FAIL.");
        return DOS_FAIL;
    }

    if (g_lMailSockfd < 0)
    {
        lRet = mon_cli_conn_init();
        if (DOS_SUCC != lRet)
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
    }

    dos_snprintf(szBuff, sizeof(szBuff), "%s", "HELO ");
    dos_strcat(szBuff, szMailServ);
    dos_strcat(szBuff, " \r\n");

    dos_memzero(szRecvBuff, sizeof(szRecvBuff));
    lRet = recv(g_lMailSockfd, szRecvBuff, sizeof(szRecvBuff) + 1, 0);
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "Data received over data:%s, lRet:%d", szRecvBuff, lRet);
    lRet = send(g_lMailSockfd, szBuff, dos_strlen(szBuff), 0);
    dos_memzero(szRecvBuff, sizeof(szRecvBuff));
    lRet = recv(g_lMailSockfd, szRecvBuff, sizeof(szRecvBuff), 0);
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "[HELO] recv:%s, lRet:%d", szRecvBuff, lRet);

    /* 发送准备登录信息 */
    lRet = send(g_lMailSockfd, "AUTH LOGIN\r\n", dos_strlen("AUTH LOGIN\r\n"), 0);
    dos_memzero(szRecvBuff, sizeof(szRecvBuff));
    lRet = recv(g_lMailSockfd, szRecvBuff, sizeof(szRecvBuff), 0);
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "[AUTH LOGIN] recv:%s, lRet:%d", szRecvBuff, lRet);

    /* 发送用户名以及密码 */
    base64_encode((const U8 *)szUsername, szBase64Usrname, dos_strlen(szUsername));
    base64_encode((const U8 *)szPasswd, szBase64Passwd, dos_strlen(szPasswd));
    dos_strcat(szBase64Usrname, "\r\n");
    dos_strcat(szBase64Passwd, "\r\n");

    lRet = send(g_lMailSockfd, szBase64Usrname, dos_strlen(szBase64Usrname), 0);
    dos_memzero(szRecvBuff, sizeof(szRecvBuff));
    lRet = recv(g_lMailSockfd, szRecvBuff, sizeof(szRecvBuff), 0);
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "Send Username Recv:%s, lRet:%d", szRecvBuff, lRet);
    lRet = send(g_lMailSockfd, szBase64Passwd, dos_strlen(szBase64Passwd), 0);
    dos_memzero(szRecvBuff, sizeof(szRecvBuff));
    lRet = recv(g_lMailSockfd, szRecvBuff, sizeof(szRecvBuff), 0);
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "Send Password Recv:%s, lRet:%d", szRecvBuff, lRet);

    /* 发送[发送邮箱]的信箱，该邮箱与用户名一致 */
    dos_snprintf(szBuff, sizeof(szBuff), "MAIL FROM: <%s>\r\n", szUsername);
    lRet = send(g_lMailSockfd, szBuff, dos_strlen(szBuff), 0);
    dos_memzero(szRecvBuff, sizeof(szRecvBuff));
    lRet = recv(g_lMailSockfd, szRecvBuff, sizeof(szRecvBuff), 0);
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "[MAIL FROM] Recv:%s, lRet:%d", szRecvBuff, lRet);

    /* 发送[接收邮箱]的信箱 */
    dos_snprintf(szBuff, sizeof(szBuff), "RCPT TO: <%s>\r\n", pszAddress);
    lRet = send(g_lMailSockfd, szBuff, dos_strlen(szBuff), 0);
    dos_memzero(szRecvBuff, sizeof(szRecvBuff));
    lRet = recv(g_lMailSockfd, szRecvBuff, sizeof(szRecvBuff), 0);
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "[RCPT TO] Recv:%s, lRet:%d", szRecvBuff, lRet);

    /* 告诉邮件服务器准备发送邮件内容 */
    lRet = send(g_lMailSockfd, "DATA\r\n", dos_strlen("DATA\r\n"), 0);
    /* 发送邮件标题 */
    dos_snprintf(szBuff, sizeof(szBuff), "From: \"%s\"<%s>\r\nTo: %s\r\nContent-type: text/html;charset=gb2312\r\nSubject: %s\r\n\r\n"
                    , szUsername, szUsername, pszAddress, pszTitle);
    lRet = send(g_lMailSockfd, szBuff, dos_strlen(szBuff), 0);
    /* 发送邮件内容 */
    dos_strcat(pszMessage, "\r\n");
    lRet = send(g_lMailSockfd, pszMessage, dos_strlen(pszMessage), 0);
    /* 发送邮件结束 */
    lRet = send(g_lMailSockfd, "\r\n.\r\n", dos_strlen("\r\n.\r\n"), 0);
    /* 接收邮件服务器返回信息 */
    dos_memzero(szRecvBuff, sizeof(szRecvBuff));
    lRet = recv(g_lMailSockfd, szRecvBuff, sizeof(szRecvBuff), 0);
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "[DATA] Recv:%s, lRet:%d", szRecvBuff, lRet);

    /* 告诉离开邮件服务器 */
    lRet = send(g_lMailSockfd, "QUIT\r\n", dos_strlen("QUIT\r\n"), 0);
    dos_memzero(szRecvBuff, sizeof(szRecvBuff));
    lRet = recv(g_lMailSockfd, szRecvBuff, sizeof(szRecvBuff), 0);
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "[QUIT] Recv:%s, lRet:%d", szRecvBuff, lRet);

    return DOS_SUCC;
}


#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

