/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名
 *
 *  创建时间:
 *  作    者:
 *  描    述:
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dos.h>
#include "mon_def.h"
#include "mon_mail.h"

static BOOL             g_blExitFlag = DOS_TRUE;
static pthread_t        g_pthreadMailRecv;
static pthread_mutex_t  g_stWarnQueueMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   g_stWarnQueueCond = PTHREAD_COND_INITIALIZER;
static sem_t g_stSem;

MON_MAIL_GLOBAL_ST g_astMonMailGlobal[MON_MAIL_SERV_COUNT];
MON_WARNING_CB_ST g_astMonWarnCB[MON_WARNING_LEVEL_BUTT];

S32 mon_mail_dns_analyze(S8 *szDomainName, S8 szIPAddr[MON_MAIL_IP_SIZE])
{
    S8 **pptr = NULL;
    struct hostent *hptr = NULL;
    S8 szstr[MON_MAIL_IP_SIZE] = {0};
    S32 i = 0;

    if (DOS_ADDR_INVALID(szDomainName))
    {
        return i;
    }

    hptr = gethostbyname(szDomainName);
    if (NULL == hptr)
    {
        mon_trace(MON_TRACE_MAIL, LOG_LEVEL_ERROR, "gethostbyname error for host:%s", szDomainName);
        return i;
    }

    switch (hptr->h_addrtype)
    {
        case AF_INET:
        case AF_INET6:
            pptr = hptr->h_addr_list;
            for(i=0; *pptr!=NULL; pptr++, i++)
            {
                if (i > 0)
                {
                    break;
                }
                inet_ntop(hptr->h_addrtype, *pptr, szstr, sizeof(szstr));
                dos_strncpy(szIPAddr, szstr, MON_MAIL_IP_SIZE);
            }

            break;
        default:
            break;
    }

    return i;
}


U32 mon_mail_connect(U32 ulIndex)
{
    S32  lRet = U32_BUTT;
    struct sockaddr_in stServerAddr;

    if (ulIndex >= MON_MAIL_SERV_COUNT
        || !g_astMonMailGlobal[ulIndex].bIsValid)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (mon_mail_init() != DOS_SUCC)
    {
        mon_trace(MON_TRACE_CONFIG, LOG_LEVEL_ERROR, "analyze mail server address fail.");
        return DOS_FAIL;
    }

    mon_trace(MON_TRACE_CONFIG, LOG_LEVEL_DEBUG, "analyze mail server address fail. IP: %s", g_astMonMailGlobal[ulIndex].szMailIP);

    /* 创建套接字 */
    g_astMonMailGlobal[ulIndex].lMonMailSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_astMonMailGlobal[ulIndex].lMonMailSockfd < 0)
    {
        mon_trace(MON_TRACE_MAIL, LOG_LEVEL_ERROR, "Create Socket error.");
        return DOS_FAIL;
    }
    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "Create Socket SUCC.(g_lSockfd:%d)", g_astMonMailGlobal[ulIndex].lMonMailSockfd);

    bzero(&stServerAddr, sizeof(stServerAddr));
    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_port = htons(g_astMonMailGlobal[ulIndex].ulMailPort);
    inet_pton(AF_INET, g_astMonMailGlobal[ulIndex].szMailIP, (VOID *)(&stServerAddr.sin_addr));

    /* 客户程序发起连接请求 */
    lRet = connect(g_astMonMailGlobal[ulIndex].lMonMailSockfd, (struct sockaddr *)(&stServerAddr), sizeof(struct sockaddr));
    if (lRet < 0)
    {
        g_astMonMailGlobal[ulIndex].lMonMailSockfd = -1;
        mon_trace(MON_TRACE_MAIL, LOG_LEVEL_ERROR, "Mail Server Connect error. errno:%u, cause:%s", errno, strerror(errno));
        return DOS_FAIL;
    }
    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "Mail Server Connect SUCC.");

    return DOS_SUCC;
}

static VOID *mon_mail_recv_mainloop(VOID *ptr)
{
    S32 lMaxFdp = 0;
    fd_set ReadFds;
    S32 i = 0;
    U32 ulCount = 0;
    S32 lSocketFd = 0;
    S32 lResult = 0;
    U32 ulIndex = 0;
    struct timeval stTimeval = {2, 0};
    struct timeval stTvCopy;
    S8  szRecvBuff[512] = {0, };
    S32 lRecvRes = 0;
    U32 ulErrno = 0;

    g_blExitFlag = DOS_FALSE;


    while (1)
    {
        if (g_blExitFlag)
        {
            mon_trace(MON_TRACE_MAIL, LOG_LEVEL_NOTIC, "requit exit.");
            break;
        }

        FD_ZERO(&ReadFds);
        lMaxFdp = 0;
        stTvCopy = stTimeval;
        for (i=0; i<MON_MAIL_SERV_COUNT; i++)
        {
            if (g_astMonMailGlobal[i].lMonMailSockfd > 0)
            {
                lSocketFd = g_astMonMailGlobal[i].lMonMailSockfd;
                FD_SET(lSocketFd, &ReadFds);
                lMaxFdp = lMaxFdp > lSocketFd ? lMaxFdp : lSocketFd;
                ulCount++;
                mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "add socket : %d", lSocketFd);
            }
        }

        if (ulCount == 0)
        {
            sleep(2);
            continue;
        }

        lResult = select(lMaxFdp + 1, &ReadFds, NULL, NULL, &stTvCopy);
        if (lResult < 0)
        {
            DOS_ASSERT(0);
            sleep(1);
            continue;
        }
        else if (0 == lResult)
        {
            sleep(1);
            continue;
        }

        for (i=0; i<=lMaxFdp; i++)
        {
            if (FD_ISSET(i, &ReadFds))
            {
                for (ulIndex=0; ulIndex<MON_MAIL_SERV_COUNT; ulIndex++)
                {
                    if (g_astMonMailGlobal[ulIndex].lMonMailSockfd == i)
                    {
                        break;
                    }
                }

                if (ulIndex == MON_MAIL_SERV_COUNT)
                {
                    continue;
                }

                lRecvRes = recv(i, szRecvBuff, sizeof(szRecvBuff), 0);
                if (lRecvRes <= 0)
                {
                    sem_post(&g_astMonMailGlobal[ulIndex].stSem);
                    close(i);
                    g_astMonMailGlobal[ulIndex].lMonMailSockfd = -1;
                    continue;
                }

                /* 获取错误码 */
                dos_sscanf(szRecvBuff, "%u ", &ulErrno);
                g_astMonMailGlobal[ulIndex].ulErrno = ulErrno;
                mon_trace(MON_TRACE_MAIL, LOG_LEVEL_NOTIC, "proc : %u, errno : %u"
                    , g_astMonMailGlobal[ulIndex].enOperate, ulErrno);

                mon_trace(MON_TRACE_MAIL, LOG_LEVEL_NOTIC, "%s", szRecvBuff);

                sem_post(&g_astMonMailGlobal[ulIndex].stSem);
            }
        }
    }

    return NULL;
}

U32 mon_mail_get_warn_msg(U32 ulIndex, MON_WARNING_MSG_ST *pstWarningMsg)
{
    S32 i = 0;
    MON_WARNING_ST *pstWarningList = NULL;

    if (ulIndex >= MON_WARNING_LEVEL_BUTT
        || DOS_ADDR_INVALID(pstWarningMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pstWarningList = g_astMonWarnCB[ulIndex].pstWarningList;
    for (i=0; i<MON_MAIL_QUE_COUNT; i++)
    {
        if (pstWarningList[i].bIsValid)
        {
            if (DOS_ADDR_INVALID(pstWarningList[i].pstWarningMsg))
            {
                pstWarningList[i].bIsValid = DOS_FALSE;
                continue;
            }

            dos_memcpy(pstWarningMsg, pstWarningList[i].pstWarningMsg, sizeof(MON_WARNING_MSG_ST));
            dos_dmem_free(pstWarningList[i].pstWarningMsg);
            pstWarningList[i].pstWarningMsg = NULL;
            pstWarningList[i].bIsValid = DOS_FALSE;
            g_astMonWarnCB[ulIndex].ulWarningCount--;

            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}

U32 mon_mail_sem_wait(sem_t *pstSem, U32 ulTimeSecond)
{
    struct timespec stSemTime;
    struct timeval now;
    S32 lRes;

    if (DOS_ADDR_INVALID(pstSem))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    gettimeofday(&now, NULL);
    stSemTime.tv_sec = now.tv_sec + ulTimeSecond;
    stSemTime.tv_nsec = now.tv_usec * 1000;
    lRes = sem_timedwait(pstSem, &stSemTime);

    if (lRes == -1)
    {
        if (errno == ETIMEDOUT)
        {
            mon_trace(MON_TRACE_MAIL, LOG_LEVEL_INFO, "Sem time wait timeout");
            return DOS_FAIL;
        }
        else
        {
            mon_trace(MON_TRACE_MAIL, LOG_LEVEL_INFO, "Sem time wait FAIL");
            return DOS_FAIL;
        }
    }

    return DOS_SUCC;
}

U32 mon_mail_send_mail(U32 ulIndex, MON_WARNING_MSG_ST *pstWarningMsg)
{
    S32 lRet = DOS_FAIL;
    S32 lSocket = 0;
    S8  szBuff[512] = {0};

    if (DOS_ADDR_INVALID(pstWarningMsg)
        || ulIndex >= MON_MAIL_SERV_COUNT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (!g_astMonMailGlobal[ulIndex].bIsValid)
    {
        return DOS_FAIL;
    }

    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_NOTIC, "Start send mail.");

    lSocket = g_astMonMailGlobal[ulIndex].lMonMailSockfd;
    if (lSocket < 0)
    {
        while (1)
        {
            if (mon_mail_connect(ulIndex) != DOS_SUCC)
            {
                sleep(2);
                continue;
            }
            lSocket = g_astMonMailGlobal[ulIndex].lMonMailSockfd;
            break;
        }
    }

    /* 将 信号量 stSem 置为0 */
    while (1)
    {
        if (sem_trywait(&g_astMonMailGlobal[ulIndex].stSem) == -1)
        {
            break;
        }
    }

    /* hello */
    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "[HELO]");
    dos_snprintf(szBuff, sizeof(szBuff), "%s", "HELO ");
    dos_strcat(szBuff, g_astMonMailGlobal[ulIndex].szMailServ);
    dos_strcat(szBuff, " \r\n");
    g_astMonMailGlobal[ulIndex].enOperate = MON_WARN_MAIL_HELO;
    lRet = send(lSocket, szBuff, dos_strlen(szBuff), 0);
    if (lRet <= 0)
    {
        goto send_fail;
    }
    if (mon_mail_sem_wait(&g_astMonMailGlobal[ulIndex].stSem, MON_MAIL_WAIT_TIMEOUT) != DOS_SUCC)
    {
        goto smtp_error;
    }

    if (g_astMonMailGlobal[ulIndex].ulErrno != MON_SMTP_ERRNO_220
        && g_astMonMailGlobal[ulIndex].ulErrno != MON_SMTP_ERRNO_250)
    {
        goto smtp_error;
    }

    /* 发送准备登录信息 */
    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "[AUTH LOGIN]");
    g_astMonMailGlobal[ulIndex].enOperate = MON_WARN_MAIL_AUTH;
    lRet = send(lSocket, "AUTH LOGIN\r\n", dos_strlen("AUTH LOGIN\r\n"), 0);
    if (lRet <= 0)
    {
        goto send_fail;
    }

    if (mon_mail_sem_wait(&g_astMonMailGlobal[ulIndex].stSem, MON_MAIL_WAIT_TIMEOUT) != DOS_SUCC)
    {
        goto smtp_error;
    }

    if (g_astMonMailGlobal[ulIndex].ulErrno != MON_SMTP_ERRNO_334)
    {
        goto smtp_error;
    }

    /* 发送用户名以及密码 */
    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "[User name]");
    g_astMonMailGlobal[ulIndex].enOperate = MON_WARN_MAIL_USERNAME;
    lRet = send(lSocket, g_astMonMailGlobal[ulIndex].szUsernameBase64, dos_strlen(g_astMonMailGlobal[ulIndex].szUsernameBase64), 0);
    if (lRet <= 0)
    {
        goto send_fail;
    }

    if (mon_mail_sem_wait(&g_astMonMailGlobal[ulIndex].stSem, MON_MAIL_WAIT_TIMEOUT) != DOS_SUCC)
    {
        goto smtp_error;
    }

    if (g_astMonMailGlobal[ulIndex].ulErrno != MON_SMTP_ERRNO_334)
    {
        goto smtp_error;
    }

    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "[password]");
    g_astMonMailGlobal[ulIndex].enOperate = MON_WARN_MAIL_PASSWORD;
    lRet = send(lSocket, g_astMonMailGlobal[ulIndex].szPasswdBase64, dos_strlen(g_astMonMailGlobal[ulIndex].szPasswdBase64), 0);
    if (lRet <= 0)
    {
        goto send_fail;
    }

    if (mon_mail_sem_wait(&g_astMonMailGlobal[ulIndex].stSem, MON_MAIL_WAIT_TIMEOUT) != DOS_SUCC)
    {
        goto smtp_error;
    }

    if (g_astMonMailGlobal[ulIndex].ulErrno != MON_SMTP_ERRNO_235)
    {
        goto smtp_error;
    }

    /* 发送[发送邮箱]的信箱，该邮箱与用户名一致 */
    g_astMonMailGlobal[ulIndex].enOperate = MON_WARN_MAIL_FROM;
    dos_snprintf(szBuff, sizeof(szBuff), "MAIL FROM: <%s>\r\n", g_astMonMailGlobal[ulIndex].szUsername);
    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "%s", szBuff);
    lRet = send(lSocket, szBuff, dos_strlen(szBuff), 0);
    if (lRet <= 0)
    {
        goto send_fail;
    }

    if (mon_mail_sem_wait(&g_astMonMailGlobal[ulIndex].stSem, MON_MAIL_WAIT_TIMEOUT) != DOS_SUCC)
    {
        goto smtp_error;
    }

    if (g_astMonMailGlobal[ulIndex].ulErrno != MON_SMTP_ERRNO_250)
    {
        goto smtp_error;
    }

    /* 发送[接收邮箱]的信箱 */
    g_astMonMailGlobal[ulIndex].enOperate = MON_WARN_MAIL_RCPT_TO;
    dos_snprintf(szBuff, sizeof(szBuff), "RCPT TO: <%s>\r\n", pstWarningMsg->szEmail);
    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "%s", szBuff);
    lRet = send(lSocket, szBuff, dos_strlen(szBuff), 0);
    if (lRet <= 0)
    {
        goto send_fail;
    }

    if (mon_mail_sem_wait(&g_astMonMailGlobal[ulIndex].stSem, MON_MAIL_WAIT_TIMEOUT) != DOS_SUCC)
    {
        goto smtp_error;
    }

    if (g_astMonMailGlobal[ulIndex].ulErrno != MON_SMTP_ERRNO_250)
    {
        goto smtp_error;
    }

    if (pstWarningMsg->ulWarningLevel == MON_WARNING_LEVEL_CRITICAL)
    {
        /* 密送 */
        dos_snprintf(szBuff, sizeof(szBuff), "RCPT TO: <%s>\r\n", MON_MAIL_BSS_MAIL);
        mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "%s", szBuff);
        lRet = send(lSocket, szBuff, dos_strlen(szBuff), 0);
        if (lRet <= 0)
        {
            goto send_fail;
        }

        if (mon_mail_sem_wait(&g_astMonMailGlobal[ulIndex].stSem, MON_MAIL_WAIT_TIMEOUT) != DOS_SUCC)
        {
            goto smtp_error;
        }

        if (g_astMonMailGlobal[ulIndex].ulErrno != MON_SMTP_ERRNO_250)
        {
            goto smtp_error;
        }
    }

    /* 告诉邮件服务器准备发送邮件内容 */
    g_astMonMailGlobal[ulIndex].enOperate = MON_WARN_MAIL_DATA;
    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "[Data]");
    lRet = send(lSocket, "DATA\r\n", dos_strlen("DATA\r\n"), 0);
    if (lRet <= 0)
    {
        goto send_fail;
    }

    /* 发送邮件标题 */
    if (pstWarningMsg->ulWarningLevel == MON_WARNING_LEVEL_CRITICAL)
    {
        dos_snprintf(szBuff, sizeof(szBuff), "From: \"%s\"<%s>\r\nTo: %s\r\nBSS: %s\r\nContent-type: text/html;charset=gb2312\r\nSubject: %s\r\n\r\n"
                    , g_astMonMailGlobal[ulIndex].szUsername
                    , g_astMonMailGlobal[ulIndex].szUsername
                    , pstWarningMsg->szEmail
                    , MON_MAIL_BSS_MAIL
                    , pstWarningMsg->szTitle);
    }
    else
    {
        dos_snprintf(szBuff, sizeof(szBuff), "From: \"%s\"<%s>\r\nTo: %s\r\nContent-type: text/html;charset=gb2312\r\nSubject: %s\r\n\r\n"
                    , g_astMonMailGlobal[ulIndex].szUsername
                    , g_astMonMailGlobal[ulIndex].szUsername
                    , pstWarningMsg->szEmail
                    , pstWarningMsg->szTitle);
    }
    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_NOTIC, "%s", szBuff);
    lRet = send(lSocket, szBuff, dos_strlen(szBuff), 0);
    if (lRet <= 0)
    {
        goto send_fail;
    }

    /* 发送邮件内容 */
    dos_strcat(pstWarningMsg->szMessage, "\r\n");
    lRet = send(lSocket, pstWarningMsg->szMessage, dos_strlen(pstWarningMsg->szMessage), 0);
    if (lRet <= 0)
    {
        goto send_fail;
    }

    /* 发送邮件结束 */
    lRet = send(lSocket, "\r\n.\r\n", dos_strlen("\r\n.\r\n"), 0);
    if (lRet <= 0)
    {
        goto send_fail;
    }

    if (mon_mail_sem_wait(&g_astMonMailGlobal[ulIndex].stSem, MON_MAIL_WAIT_TIMEOUT) != DOS_SUCC)
    {
        goto smtp_error;
    }

    //if (g_astMonMailGlobal[ulIndex].ulErrno != MON_SMTP_ERRNO_354)
    //{
    //    goto smtp_error;
    //}

    if (mon_mail_sem_wait(&g_astMonMailGlobal[ulIndex].stSem, MON_MAIL_WAIT_TIMEOUT) != DOS_SUCC)
    {
        goto smtp_error;
    }

    if (g_astMonMailGlobal[ulIndex].ulErrno != MON_SMTP_ERRNO_250)
    {
        goto smtp_error;
    }

    /* 告诉离开邮件服务器 */
    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "[QUIT]");
    g_astMonMailGlobal[ulIndex].enOperate = MON_WARN_MAIL_QUIT;
    lRet = send(lSocket, "QUIT\r\n", dos_strlen("QUIT\r\n"), 0);
    if (lRet <= 0)
    {
        goto send_fail;
    }

    if (mon_mail_sem_wait(&g_astMonMailGlobal[ulIndex].stSem, MON_MAIL_WAIT_TIMEOUT) != DOS_SUCC)
    {
        /* 超时/或者失败，邮件发送失败 */
        goto smtp_error;
    }

    if (g_astMonMailGlobal[ulIndex].ulErrno != MON_SMTP_ERRNO_221)
    {
        goto smtp_error;
    }

    return DOS_SUCC;

send_fail:
    /* send() 失败 */
    return DOS_FAIL;

smtp_error:
    /* 响应码错误。或者没有收到响应 */
    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_NOTIC, "FAIL. operate : %u, errno : %u"
        , g_astMonMailGlobal[ulIndex].enOperate, g_astMonMailGlobal[ulIndex].ulErrno);

    return DOS_FAIL;
}

U32 mon_mail_warn_proc(U32 ulIndex, MON_WARNING_MSG_ST *pstWarningMsg)
{
    U32 ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstWarningMsg)
        || ulIndex >= MON_MAIL_SERV_COUNT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstWarningMsg->ucWarningType)
    {
        case MON_WARNING_TYPE_MAIL:
            /* 邮件告警 */
            ulRet = mon_mail_send_mail(ulIndex, pstWarningMsg);
            break;
        case MON_WARNING_TYPE_SMS:
            /* 短信告警 TODO */
        case MON_WARNING_TYPE_VOICE:
            /* 语音告警 TODO */
            break;
        default:
            break;
    }

    return ulRet;
}

static VOID *mon_mail_send_mainloop(VOID *ptr)
{
    U32 ulIndex = 0;
    S32 i = 0;
    MON_WARNING_MSG_ST stWarningMsg;

    g_blExitFlag = DOS_FALSE;
    ulIndex = *(U32 *)ptr;

    sem_post(&g_stSem);

    if (ulIndex >= MON_MAIL_SERV_COUNT)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    while (1)
    {
        if (g_blExitFlag)
        {
            mon_trace(MON_TRACE_MAIL, LOG_LEVEL_NOTIC, "requit exit.");
            break;
        }

        pthread_mutex_lock(&g_stWarnQueueMutex);
        pthread_cond_wait(&g_stWarnQueueCond, &g_stWarnQueueMutex);
        pthread_mutex_unlock(&g_stWarnQueueMutex);

        /* 创建socket */
        while (g_astMonMailGlobal[ulIndex].lMonMailSockfd <= 0)
        {
            if (mon_mail_connect(ulIndex) != DOS_SUCC)
            {
                sleep(2);
                continue;
            }

            break;
        }

        for (i=0; i<MON_WARNING_LEVEL_BUTT; i++)
        {
            if (g_astMonWarnCB[i].ulWarningCount == 0)
            {
                continue;
            }

            while (1)
            {
                pthread_mutex_lock(&g_astMonWarnCB[i].stMutex);
                if (mon_mail_get_warn_msg(i, &stWarningMsg) != DOS_SUCC)
                {
                    pthread_mutex_unlock(&g_astMonWarnCB[i].stMutex);
                    break;
                }
                pthread_mutex_unlock(&g_astMonWarnCB[i].stMutex);
                mon_trace(MON_TRACE_MAIL, LOG_LEVEL_NOTIC, "!!!!!!!!!!!!!!!!!start proce warn. type : %u"
                    , stWarningMsg.ucWarningType);
                mon_mail_warn_proc(ulIndex, &stWarningMsg);
            }
        }
    }

    return NULL;
}

U32 mon_mail_read_configure()
{
    S32  lRet = U32_BUTT;

    /* 获取mail server */
    lRet = config_warn_get_mail_server(g_astMonMailGlobal[0].szMailServ, sizeof(g_astMonMailGlobal[0].szMailServ));
    if (lRet < 0)
    {
        mon_trace(MON_TRACE_MAIL, LOG_LEVEL_ERROR, "Get default SMTP Server FAIL.");
        return DOS_FAIL;
    }

    /* 获取默认的邮箱账户认证名称 */
    lRet = config_warn_get_auth_username(g_astMonMailGlobal[0].szUsername, sizeof(g_astMonMailGlobal[0].szUsername));
    if (lRet < 0)
    {
        mon_trace(MON_TRACE_MAIL, LOG_LEVEL_ERROR, "Get Default Auth Username FAIL.");
        return DOS_FAIL;
    }
    /* 获取默认的邮箱账户额认证密码 */
    lRet = config_warn_get_auth_passwd(g_astMonMailGlobal[0].szPasswd, sizeof(g_astMonMailGlobal[0].szPasswd));
    if (lRet < 0)
    {
        mon_trace(MON_TRACE_MAIL, LOG_LEVEL_ERROR, "Get Default Auth Password FAIL.");
        return DOS_FAIL;
    }

    base64_encode((const U8 *)g_astMonMailGlobal[0].szUsername, g_astMonMailGlobal[0].szUsernameBase64, dos_strlen(g_astMonMailGlobal[0].szUsername));
    dos_strcat(g_astMonMailGlobal[0].szUsernameBase64, "\r\n");
    base64_encode((const U8 *)g_astMonMailGlobal[0].szPasswd, g_astMonMailGlobal[0].szPasswdBase64, dos_strlen(g_astMonMailGlobal[0].szPasswd));
    dos_strcat(g_astMonMailGlobal[0].szPasswdBase64, "\r\n");

    /* 获得端口 */
    g_astMonMailGlobal[0].ulMailPort = config_warn_get_mail_port();
    if (g_astMonMailGlobal[0].ulMailPort > U16_BUTT)
    {
        mon_trace(MON_TRACE_MAIL, LOG_LEVEL_ERROR, "Get Default Port FAIL.");
        return DOS_FAIL;
    }

    g_astMonMailGlobal[0].bIsValid = DOS_TRUE;

    return DOS_SUCC;
}

void mon_mail_free_resource()
{
    S32 i = 0;

    for (i=0; i<MON_WARNING_LEVEL_BUTT; i++)
    {
        if (DOS_ADDR_VALID(g_astMonWarnCB[i].pstWarningList))
        {
            dos_dmem_free(g_astMonWarnCB[i].pstWarningList);
            g_astMonWarnCB[i].pstWarningList = NULL;

            pthread_mutex_destroy(&g_astMonWarnCB[i].stMutex);
        }
    }
}

//MON_WARNING_CB_ST g_astMonWarnCB[MON_WARNING_LEVEL_BUTT];
U32 mon_mail_alloc_resource()
{
    S32 i = 0;

    for (i=0; i<MON_WARNING_LEVEL_BUTT; i++)
    {
        g_astMonWarnCB[i].enWarningLevel = i;
        g_astMonWarnCB[i].ulWarningCount = 0;
        g_astMonWarnCB[i].pstWarningList = (MON_WARNING_ST *)dos_dmem_alloc(sizeof(MON_WARNING_ST) * MON_MAIL_QUE_COUNT);
        if (DOS_ADDR_INVALID(g_astMonWarnCB[i].pstWarningList))
        {
            DOS_ASSERT(0);
            break;
        }
        dos_memzero(g_astMonWarnCB[i].pstWarningList, sizeof(MON_WARNING_ST) * MON_MAIL_QUE_COUNT);
        pthread_mutex_init(&g_astMonWarnCB[i].stMutex, NULL);
        //pthread_cond_init(&g_astMonWarnCB[i].stCont, NULL);
    }

    if (i != MON_WARNING_LEVEL_BUTT)
    {
        mon_mail_free_resource();

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 mon_mail_send_warning(MON_WARNING_TYPE_EN enWarnType, MON_WARNING_LEVEL_EN enWarnLevel, S8 *szAddr, S8 *szTitle, S8 *szMessage)
{
    MON_WARNING_MSG_ST *pstWarnMsg = NULL;
    S32 i = 0;
    U32 ulRet = DOS_FAIL;

    if (enWarnType >= MON_WARNING_TYPE_BUTT
        || enWarnLevel >= MON_WARNING_LEVEL_BUTT
        || DOS_ADDR_INVALID(szAddr)
        || DOS_ADDR_INVALID(szTitle)
        || DOS_ADDR_INVALID(szMessage))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstWarnMsg = (MON_WARNING_MSG_ST *)dos_dmem_alloc(sizeof(MON_WARNING_MSG_ST));
    if (DOS_ADDR_INVALID(pstWarnMsg))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstWarnMsg->ucWarningType = enWarnType;
    pstWarnMsg->ulWarningLevel = enWarnLevel;
    if (enWarnType == MON_WARNING_TYPE_MAIL)
    {
        dos_strncpy(pstWarnMsg->szEmail, szAddr, sizeof(pstWarnMsg->szEmail));
        pstWarnMsg->szEmail[sizeof(pstWarnMsg->szEmail)-1] = '\0';
    }
    else
    {
        dos_strncpy(pstWarnMsg->szTelNo, szAddr, sizeof(pstWarnMsg->szTelNo));
        pstWarnMsg->szTelNo[sizeof(pstWarnMsg->szTelNo)-1] = '\0';
    }

    dos_strncpy(pstWarnMsg->szTitle, szTitle, sizeof(pstWarnMsg->szTitle));
    pstWarnMsg->szTitle[sizeof(pstWarnMsg->szTitle)-1] = '\0';
    dos_strncpy(pstWarnMsg->szMessage, szMessage, sizeof(pstWarnMsg->szMessage));
    pstWarnMsg->szMessage[sizeof(pstWarnMsg->szMessage)-1] = '\0';

    pthread_mutex_lock(&g_astMonWarnCB[enWarnLevel].stMutex);
    for (i=0; i<MON_MAIL_QUE_COUNT; i++)
    {
        if (g_astMonWarnCB[enWarnLevel].pstWarningList[i].bIsValid == DOS_FALSE)
        {
            g_astMonWarnCB[enWarnLevel].pstWarningList[i].bIsValid = DOS_TRUE;
            g_astMonWarnCB[enWarnLevel].pstWarningList[i].pstWarningMsg = pstWarnMsg;
            g_astMonWarnCB[enWarnLevel].ulWarningCount++;
            ulRet = DOS_SUCC;
            break;
        }
    }

    pthread_mutex_unlock(&g_astMonWarnCB[enWarnLevel].stMutex);

    if (ulRet != DOS_SUCC)
    {
        dos_dmem_free(pstWarnMsg);
        pstWarnMsg = NULL;
        mon_trace(MON_TRACE_MAIL, LOG_LEVEL_INFO, "Add into warning queue FAIL");

        return DOS_FAIL;
    }

    mon_trace(MON_TRACE_MAIL, LOG_LEVEL_DEBUG, "Add into warning queue SUCC");

    pthread_mutex_lock(&g_stWarnQueueMutex);
    pthread_cond_signal(&g_stWarnQueueCond);
    pthread_mutex_unlock(&g_stWarnQueueMutex);

    return DOS_SUCC;
}


U32 mon_mail_init()
{
    U32 ulRes   = DOS_FAIL;
    S32 i       = 0;
    U32 ulCount = 0;

    dos_memzero(g_astMonMailGlobal, sizeof(MON_MAIL_GLOBAL_ST) * MON_MAIL_SERV_COUNT);
    ulRes = mon_mail_read_configure();
    if (ulRes != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    for (i=0; i<MON_MAIL_SERV_COUNT; i++)
    {
        if (!g_astMonMailGlobal[i].bIsValid)
        {
            continue;
        }
        sem_init(&g_astMonMailGlobal[i].stSem, 0, 0);
        /* 域名解析 */
        ulRes = mon_mail_dns_analyze(g_astMonMailGlobal[i].szMailServ, g_astMonMailGlobal[i].szMailIP);
        if (ulRes <= 0)
        {
            /* 失败 */
            g_astMonMailGlobal[i].bIsValid = DOS_FALSE;
            continue;
        }
        ulCount++;
    }

    if (ulCount == 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (mon_mail_alloc_resource() != DOS_SUCC)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 mon_mail_start()
{
    S32 i = 0;

    sem_init(&g_stSem, 0, 0);

    if (pthread_create(&g_pthreadMailRecv, NULL, mon_mail_recv_mainloop, NULL) < 0)
    {
        mon_trace(MON_TRACE_MAIL, LOG_LEVEL_NOTIC, "Create recv pthread fail");
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    for (i=0; i<MON_MAIL_SERV_COUNT; i++)
    {
        if (!g_astMonMailGlobal[i].bIsValid)
        {
            continue;
        }

        if (pthread_create(&g_astMonMailGlobal[i].pthreadMailSend, NULL, mon_mail_send_mainloop, (void *)&i) < 0)
        {
            mon_trace(MON_TRACE_MAIL, LOG_LEVEL_NOTIC, "Create send pthread fail");
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
        sem_wait(&g_stSem);
    }

    sem_destroy(&g_stSem);

    return DOS_SUCC;
}


void mon_mail_test(S32 argc, S8 **argv)
{
    if (argc != 5)
    {
        return;
    }

    mon_mail_send_warning(MON_WARNING_TYPE_MAIL, MON_WARNING_LEVEL_CRITICAL, argv[2], argv[3], argv[4]);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


