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

#include <netdb.h>
#include <dos.h>
#include "mon_def.h"
#include "mon_mail.h"

static BOOL             g_blExitFlag = DOS_TRUE;
static pthread_t        g_pthreadMailRecv;
static pthread_mutex_t  g_mutexMailQue = PTHREAD_MUTEX_INITIALIZER;

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
    inet_pton(AF_INET, g_astMonMailGlobal[ulIndex].szMailIP, (VOID *)(stServerAddr.sin_addr));

    /* 客户程序发起连接请求 */
    lRet = connect(g_astMonMailGlobal[ulIndex].lMonMailSockfd, (struct sockaddr *)(&stServerAddr), sizeof(struct sockaddr));
    if (lRet < 0)
    {
        g_astMonMailGlobal[ulIndex].lMonMailSockfd = -1;
        mon_trace(MON_TRACE_MAIL,  LOG_LEVEL_ERROR, "Mail Server Connect error. errno:%u, cause:%s", errno, strerror(errno));
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

                /* TODO */
            }
        }
    }

    return NULL;
}

static VOID *mon_mail_send_mainloop(VOID *ptr)
{
    U32 ulIndex = 0;
    S32 lSocketFd = 0;
    S8  szBuff[512] = {0};

    g_blExitFlag = DOS_FALSE;
    ulIndex = *(U32 *)ptr;

    if (ulIndex >= MON_MAIL_SERV_COUNT)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /* 创建 socket */
    while (1)
    {
        if (mon_mail_connect(ulIndex) != DOS_SUCC)
        {
            sleep(2);
            continue;
        }

        lSocketFd = g_astMonMailGlobal[ulIndex].lMonMailSockfd;
        break;
    }

    while (1)
    {
        if (g_blExitFlag)
        {
            mon_trace(MON_TRACE_MAIL, LOG_LEVEL_NOTIC, "requit exit.");
            break;
        }

        pthread_mutex_lock(&g_mutexMailQue);
        if ()



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

            pthread_mutexattr_destroy(&g_astMonWarnCB[i].mutexList);
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
        pthread_mutex_init(&g_astMonWarnCB[i].mutexList, NULL);
    }

    if (i != MON_WARNING_LEVEL_BUTT)
    {
        mon_mail_free_resource();

        return DOS_FAIL;
    }

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

        if (pthread_create(&g_astMonMailGlobal[i].pthreadMailSend, NULL, mon_mail_send_mainloop, (void*)&i) < 0)
        {
            mon_trace(MON_TRACE_MAIL, LOG_LEVEL_NOTIC, "Create send pthread fail");
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
    }

    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


