/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  debug_cli.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现和统一管理平台的通讯相关功能
 *     History:
 */


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include dos header files */
#include <dos.h>

/* include private header files */
#include "debug_cli.h"

/* include sys header files */
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#if INCLUDE_DEBUG_CLI

/* 宏定义 */
#define MAX_READ_BUFF 512
#define MAX_SEND_BUFF 512*3

#define MAX_KEY_WORDS 16

#define CLI_RE_REG_TIME_LEN   10

/* 模块内部的全局变量 */
/* 对等待退出标志进行保护 */
static pthread_mutex_t    g_DebugMutex = PTHREAD_MUTEX_INITIALIZER;

/*  线程id */
static pthread_t          g_pthDebugTask;

/* 服务器地址 */
static struct sockaddr_un g_stSrvAddr;

/* 连接socket */
static S32                g_lCliSocket = -1;

/* 等待退出标志 */
static S32                g_lTaskWaitingExit = 0;

/* 连接是否正常标志 */
static BOOL               g_bIsConnectOK = DOS_FALSE;

/* 定期发送注册消息，作为心跳 */
static DOS_TMR_ST         g_hTmrReRegTmr = NULL;

/* 外部函数 */
extern struct tagCommand * debug_cli_cmd_find(S32 argc, S8 **argv);


/**
 * 函数：S32 debug_cli_reg_rsp_proc()
 * 功能：处理注册响应消息
 * 参数：
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 debug_cli_reg_rsp_proc()
{
    return 0;
}

/**
 * 函数：S32 debug_cli_unreg_rsp_proc()
 * 功能：处理取消注册现象
 * 参数：
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 debug_cli_unreg_rsp_proc()
{
    return 0;
}

/**
 * 函数：S32 debug_cli_cmd_analyse(U32 ulClientIndex, S8 *pszMsg, U32 ulLength)
 * 功能：处理取消注册现象
 * 参数：
 *      U32 ulClientIndex：telnet客户端编号
 *      S8 *pszMsg：消息体
 *      U32 ulLength：消息长度
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 debug_cli_cmd_analyse(U32 ulClientIndex, S8 *pszMsg, U32 ulLength)
{
    struct tagCommand *pstCurrentCmd;
    S8 *pszKeyWord[MAX_KEY_WORDS] = { 0 };
    S8 pszErrorMsg[64] = { 0 };
    S32 lKeyCnt = 0, lRet = 0, i=0;
    S8 *pWord = NULL;

    logo_info("Cli Client", "Cli Clinet", DOS_TRUE, "Recv Control Panel CMD: %s", pszMsg);

    /* 获取命令 */
    pWord = strtok(pszMsg, " ");
    while (pWord)
    {
        pszKeyWord[lKeyCnt] = dos_dmem_alloc(dos_strlen(pWord) + 1);
        if (!pszKeyWord[lKeyCnt])
        {
            cli_logr_warning("%s", "Alloc fail.");
            lRet = -1;
            goto finished;
        }

        dos_strcpy(pszKeyWord[lKeyCnt], pWord);
        lKeyCnt++;
        pWord = strtok(NULL, " ");
        if (NULL == pWord)
        {
            break;
        }
    }

    if (lKeyCnt <= 0)
    {
        DOS_ASSERT(0);
        snprintf(pszErrorMsg, sizeof(pszErrorMsg), "Invalid command.\r\n");
        goto error;
    }

    pstCurrentCmd = debug_cli_cmd_find(lKeyCnt, pszKeyWord);
    if (!pstCurrentCmd)
    {
        lRet = -1;
        snprintf(pszErrorMsg, sizeof(pszErrorMsg), "Cannot find the command.\r\n");
        goto error;
    }

    lRet = pstCurrentCmd->func(ulClientIndex, lKeyCnt, pszKeyWord);
    goto finished;

error:
    debug_cli_send_cmd_responce(pszErrorMsg, dos_strlen(pszErrorMsg) + 1, ulClientIndex);

finished:
    for (i=0; i<MAX_KEY_WORDS; i++)
    {
        if (pszKeyWord[i])
        {
            dos_dmem_free(pszKeyWord[i]);
        }
    }

    return lRet;
}

/**
 * 函数：S32 debug_cli_msg_proc(U8 *_pMsg, U32 ulLen)
 * 功能：处理取消注册现象
 * 参数：
 *      U8 *_pMsg：消息体
 *      U32 ulLen：消息长度
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 debug_cli_msg_proc(U8 *_pMsg, U32 ulLen)
{
    CLI_MSG_HEADER *pstMsgHeader = NULL;
    MSG_UNIT_ST    *pstMsgCmd = NULL;
    U32 ulLength;
    S32 lRet = 0;

    if (!_pMsg || ulLen <= 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    cli_logr_info("Recv msg, length:%d", ulLen);

    if (ulLen < sizeof(CLI_MSG_HEADER))
    {
        DOS_ASSERT(0);
        cli_logr_warning("Recv msg with invalid length(%d).", ulLen);
        return -1;
    }

    /* 获取消息头 */
    pstMsgHeader = (CLI_MSG_HEADER*)_pMsg;
    if (pstMsgHeader->usLength <= 0)
    {
        cli_logr_info("%s", "Recv msg without data.");
        return 0;
    }

    if (pstMsgHeader->usLength > ulLen)
    {
    	cli_logr_info("%s", "Recv invalid msg droped.");
    	return 0;
    }

    cli_logr_debug("Start process message, total length:%d, body length:%d.", ulLen, pstMsgHeader->usLength);

    /* 逐个读取消息体 */
    ulLength = 0;
    pstMsgCmd = (MSG_UNIT_ST *)pstMsgHeader->pszData;
    while (ulLength <= pstMsgHeader->usLength
            && pstMsgCmd)
    {
    	if (ulLength > ulLen)
    	{
    		DOS_ASSERT(0);
    		return 0;
    	}

        if (pstMsgHeader->usLength - ulLength < sizeof(MSG_UNIT_ST) + pstMsgCmd->usLength)
        {
            cli_logr_info("%s(%d,%d,%d)", "Recv msg, but data is not cpmplete."
                        , pstMsgHeader->usLength, ulLength, pstMsgCmd->usLength);
            break;
        }

        cli_logr_info("Recv cmd. Type:%d, Length:%d", pstMsgCmd->usType, pstMsgCmd->usLength);

        switch (pstMsgCmd->usType)
        {
            case MSG_TYPE_PROCESS_REG:
            case MSG_TYPE_PROCESS_NAME:
            case MSG_TYPE_PROCESS_VERSION:
            case MSG_TYPE_PROCESS_UNREG:
            case MSG_TYPE_CMD_RESPONCE:
            case MSG_TYPE_LOG:
                DOS_ASSERT(0);
                cli_logr_info("%s(%d)", "Templately cannot support this cmd.", pstMsgCmd->usType);
                break;
            case MSG_TYPE_PROCESS_REG_RSP:
                lRet = debug_cli_reg_rsp_proc();
                break;
            case MSG_TYPE_PROCESS_UNREG_RESPONCE:
                lRet = debug_cli_unreg_rsp_proc();
                break;
            case MSG_TYPE_CMD:
                lRet = debug_cli_cmd_analyse(pstMsgHeader->usClientIndex
                        , (S8*)pstMsgCmd->pszData
                        , pstMsgCmd->usLength);
                break;
            default:
                DOS_ASSERT(0);
                break;
        }

        cli_logr_info("Recv cmd. Type:%d, Length:%d, process finished.", pstMsgCmd->usType, pstMsgCmd->usLength);

        /* 获取下一个信令单元 */
        ulLength += sizeof(MSG_UNIT_ST) + pstMsgCmd->usLength;
        cli_logr_debug("%d,%d,%d", pstMsgCmd->usLength, ulLength, pstMsgHeader->usLength);
        if (pstMsgHeader->usLength == ulLength)
        {
            break;
        }
        pstMsgCmd = (MSG_UNIT_ST *)(_pMsg + sizeof(CLI_MSG_HEADER) + ulLength);
    }

    return lRet;
}



/**
 * 函数：S32 debug_cli_send_msg(S32 lSocket, struct sockaddr_un *pstAddr, U8 *pszBuffer, S32 lLength)
 * 功能：通过lSocket想地址为pstAddr的进程发送消息
 * 参数：
 *  S32 lSocket,
 *  struct sockaddr_un *pstAddr,
 *  U8 *pszBuffer,
 *  S32 lLength
 *  返回值：成功返回0，失败返回－1
 */
S32 debug_cli_send_msg(S32 lSocket, struct sockaddr_un *pstAddr, U8 *pszBuffer, S32 lLength)
{
    S32 lRet, lAddrLen;

    if (!pstAddr)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (!pszBuffer || lLength <= 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (!g_bIsConnectOK)
    {
        cli_logr_info("%s", "Debug cli init not complete. discard this msg.");
        return -1;
    }

    lAddrLen = offsetof(struct sockaddr_un, sun_path) + dos_strlen(pstAddr->sun_path);
    lRet = sendto(lSocket, pszBuffer, lLength, 0, (struct sockaddr *)pstAddr, lAddrLen);
    /* 出错之后就直接修改socket的值，避免sigpipe出现 */
    if (lRet < 0)
    {
        cli_logr_debug("%s(%d)", "Send log to the control platfrom fail.", errno);
        DOS_ASSERT(0);

        g_bIsConnectOK = DOS_FALSE;

        return -1;
    }

    return 0;
}

/**
 * 函数：S32 debug_cli_send_log(S8 * _pszMsg, S32 _iLength)
 * 功能：向统一管理平台发送日志信息
 * 参数：
 *  S8 * _pszMsg：消息体
 *  S32 _iLength：消息大小
 * 返回值：
 *      成功返回0，失败返回－1
 *
 * !!!!!!!! 特别说明 !!!!!!!!
 *  该函数为日志模块调用写cli日志接口函数，勿要在此函数中使用日志函数，否则会造成死锁，或者死循环
 */
S32 debug_cli_send_log(S8 * _pszMsg, S32 _lLength)
{
    CLI_MSG_HEADER *pstCliMsgHeader;
    MSG_UNIT_ST    *pstCliMsg;
    S32            lAddrLen = 0, lRet = 0;
    U8             szSendBuff[MAX_SEND_BUFF];

    if (g_lCliSocket < 0)
    {
        printf("%s", _pszMsg);
        cli_logr_debug("%s", "Invalid socket while send log to cli server.");
        return -1;
    }

    if (!g_bIsConnectOK)
    {
        cli_logr_debug("%s", "Cli task init not complete.");
        return -1;
    }

    //dos_printf("Debug send log to cli : %s", _pszMsg);

    pstCliMsgHeader = (CLI_MSG_HEADER *)szSendBuff;
    pstCliMsg = (MSG_UNIT_ST *)pstCliMsgHeader->pszData;

    dos_memzero(pstCliMsgHeader, sizeof(CLI_MSG_HEADER));
    dos_memzero(pstCliMsg, sizeof(MSG_UNIT_ST));

    pstCliMsg->usType = MSG_TYPE_LOG;
    pstCliMsg->usLength = _lLength;
    memcpy(pstCliMsg->pszData, _pszMsg, _lLength);

    pstCliMsgHeader->usClientIndex = 0xFF;
    pstCliMsgHeader->usLength = sizeof(MSG_UNIT_ST) + _lLength;
    pstCliMsgHeader->usProteVersion = 0;

    lAddrLen = offsetof(struct sockaddr_un, sun_path) + dos_strlen(g_stSrvAddr.sun_path);
    lRet = sendto(g_lCliSocket
                    , szSendBuff
                    , pstCliMsgHeader->usLength + sizeof(CLI_MSG_HEADER)
                    , 0
                    , (struct sockaddr *)&g_stSrvAddr
                    , lAddrLen);
    /* 出错之后就直接修改socket的值，避免sigpipe出现 */
    if (lRet < 0)
    {
        cli_logr_debug("%s(%d)", "Send log to the control platfrom fail.", errno);

        g_bIsConnectOK = DOS_FALSE;

        DOS_ASSERT(0);
        return -1;
    }

    return 0;
}

/**
 * 函数：S32 debug_cli_send_cmd_responce(const S8 * _pszMsg, const S32 _lLength, const S32 _lClientIndex)
 * 功能：响应统一管理平台的命令
 * 参数：
 *  S8 * _pszMsg：消息体
 *  S32 _iLength：消息大小
 *  S32 _lClientIndex：客户端index
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 debug_cli_send_cmd_responce(const S8 * _pszMsg, const S32 _lLength, const S32 _lClientIndex)
{
    CLI_MSG_HEADER *pstCliMsgHeader;
    MSG_UNIT_ST    *pstCliMsg;
    U8             szSendBuff[MAX_SEND_BUFF];

    if (g_lCliSocket < 0)
    {
        return -1;
    }

    //dos_printf("Debug send log to cli : %s\n", _pszMsg);

    pstCliMsgHeader = (CLI_MSG_HEADER *)szSendBuff;
    pstCliMsg = (MSG_UNIT_ST *)pstCliMsgHeader->pszData;

    dos_memzero(pstCliMsgHeader, sizeof(CLI_MSG_HEADER));
    dos_memzero(pstCliMsg, sizeof(MSG_UNIT_ST));

    pstCliMsg->usType = MSG_TYPE_CMD_RESPONCE;
    pstCliMsg->usLength = _lLength;
    memcpy(pstCliMsg->pszData, _pszMsg, _lLength);

    pstCliMsgHeader->usClientIndex = _lClientIndex;
    pstCliMsgHeader->usLength = sizeof(MSG_UNIT_ST) + _lLength;
    pstCliMsgHeader->usProteVersion = 0;

    return debug_cli_send_msg(g_lCliSocket, &g_stSrvAddr, szSendBuff, pstCliMsgHeader->usLength + sizeof(CLI_MSG_HEADER));
}

/**
 * 函数：S32 debug_cli_send_reg()
 * 功能：响应统一管理平台发送注册消息
 * 参数：
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 debug_cli_send_reg()
{
    CLI_MSG_HEADER *pstCliMsgHeader = NULL;
    MSG_UNIT_ST    *pstMsgName = NULL, *pstMsgVersion = NULL, *pstMsgReg = NULL;
    U8             szSendBuff[MAX_SEND_BUFF] = { 0 };
    U32 ulMsgLen = 0;

    if (g_lCliSocket < 0)
    {
        return -1;
    }

    cli_logr_info("%s", "Debug cli start to register to the cli server.");

    /* 初始化消息头 */
    pstCliMsgHeader = (CLI_MSG_HEADER *)szSendBuff;
    pstCliMsgHeader->usClientIndex = 0xFF;
    pstCliMsgHeader->usProteVersion = 0;
    ulMsgLen += sizeof(CLI_MSG_HEADER);

    if (sizeof(szSendBuff) - ulMsgLen < sizeof(MSG_UNIT_ST))
    {
        DOS_ASSERT(0);
        return -1;
    }

    pstMsgReg = (MSG_UNIT_ST *)(szSendBuff + ulMsgLen);
    pstMsgReg->usLength = 0;
    pstMsgReg->usType = MSG_TYPE_PROCESS_REG;
    ulMsgLen += sizeof(MSG_UNIT_ST);

    /* 判断buffer长度 */
    if (sizeof(szSendBuff) - ulMsgLen < sizeof(MSG_UNIT_ST) + strlen(dos_get_process_name()) + 1)
    {
        DOS_ASSERT(0);
        return -1;
    }
    pstMsgName = (MSG_UNIT_ST *)(szSendBuff + ulMsgLen);
    pstMsgName->usType = MSG_TYPE_PROCESS_NAME;
    pstMsgName->usLength = strlen(dos_get_process_name());
    memcpy(pstMsgName->pszData, dos_get_process_name(), pstMsgName->usLength);
    pstMsgName->usLength++;
    pstMsgName->pszData[pstMsgName->usLength] = '\0';
    ulMsgLen += sizeof(MSG_UNIT_ST) + pstMsgName->usLength;

    /* 判断buffer长度 */
    if (sizeof(szSendBuff) - ulMsgLen < sizeof(MSG_UNIT_ST) + strlen(dos_get_process_version()) + 1)
    {
        DOS_ASSERT(0);
        return -1;
    }
    pstMsgVersion = (MSG_UNIT_ST *)(szSendBuff + ulMsgLen);
    pstMsgVersion->usType = MSG_TYPE_PROCESS_VERSION;
    pstMsgVersion->usLength = strlen(dos_get_process_version());
    memcpy(pstMsgVersion->pszData, dos_get_process_version(), pstMsgVersion->usLength);
    pstMsgVersion->usLength++;
    pstMsgVersion->pszData[pstMsgVersion->usLength] = '\0';
    ulMsgLen += sizeof(MSG_UNIT_ST) + pstMsgVersion->usLength;

    pstCliMsgHeader->usLength =  ulMsgLen - sizeof(CLI_MSG_HEADER);

    return debug_cli_send_msg(g_lCliSocket, &g_stSrvAddr, szSendBuff, ulMsgLen);
}

/**
 * 函数：VOID debug_cli_re_reg_timeout(U64 uLParam)
 * 功能：注册超市处理函数，主要工作为调用注册函数 重新发起注册
 */
VOID debug_cli_re_reg_timeout(U64 uLParam)
{
    if (debug_cli_send_reg() < 0)
    {
        DOS_ASSERT(0);
        cli_logr_error("Re-registe to the cli server fail. It will re-connect later");
    }
}

/**
 * 函数：S32 debug_cli_reconn()
 * 功能：发现服务器连接失败之后重新连接到服务器
 */
S32 debug_cli_reconn()
{
    S8 szBuffSockPath[256] = { 0 };
    S32 lAddrLen;

    dos_memzero(&g_stSrvAddr, sizeof(g_stSrvAddr));
    g_stSrvAddr.sun_family = AF_UNIX;
    snprintf(szBuffSockPath, sizeof(szBuffSockPath)
                , "%s/var/run/socket/cli-srv.sock"
                , dos_get_sys_root_path());
    dos_strcpy(g_stSrvAddr.sun_path, szBuffSockPath);
    lAddrLen = offsetof(struct sockaddr_un, sun_path) + dos_strlen(szBuffSockPath);

    if (connect(g_lCliSocket, (struct sockaddr *)&g_stSrvAddr, lAddrLen) < 0)
    {
        cli_logr_info("Cannot connect to cli server.(%d)", errno);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 函数：static VOID * debug_cli_main_loop(VOID *ptr)
 * 功能：模块线程函数，主要用于读取苹果发过来的数据，并分发到不同的模块
 */
static VOID * debug_cli_main_loop(VOID *ptr)
{
    S32 lRet, lMaxFd;
    struct timeval stTimeout={1, 0};
    U8 szReadBuff[MAX_READ_BUFF] = { 0 };
    fd_set stFdset;

    pthread_mutex_lock(&g_DebugMutex);
    g_lTaskWaitingExit = 0;
    pthread_mutex_unlock(&g_DebugMutex);

    while(1)
    {
        pthread_mutex_lock(&g_DebugMutex);
        if (g_lTaskWaitingExit)
        {
            cli_logr_debug("%s", "Cli task finished flag has been set.");
            pthread_mutex_unlock(&g_DebugMutex);
            break;
        }
        pthread_mutex_unlock(&g_DebugMutex);

        if (!g_bIsConnectOK)
        {
            /* 连接失败了，先停掉定重注册时器 */
            if (g_hTmrReRegTmr)
            {
                dos_tmr_stop(&g_hTmrReRegTmr);
            }

            /* 轮询链接到服务器 */
            cli_logr_info("%s", "Waiting to connect to the cli server.");
            sleep(1);
            if (debug_cli_reconn() != DOS_SUCC)
            {
                continue;
            }
            cli_logr_info("%s", "Connect to the cli server OK.");

            g_bIsConnectOK = DOS_TRUE;

            /* 连接OK，向服务器发送注册报文 */
            debug_cli_send_reg();

            /* 启动定期注册的定时器 */
            dos_tmr_start(&g_hTmrReRegTmr
                    , CLI_RE_REG_TIME_LEN * 1000
                    , debug_cli_re_reg_timeout
                    , 0
                    , TIMER_NORMAL_LOOP);
            cli_logr_debug("Start re-registe timer");
        }

        if (g_lCliSocket < 0)
        {
            cli_logr_debug("%s", "Socket has been closed!");
            break;
        }

        FD_ZERO(&stFdset);
        FD_SET(g_lCliSocket, &stFdset);
        lMaxFd = g_lCliSocket + 1;
        szReadBuff[0] = '\0';
        stTimeout.tv_sec = 1;
        stTimeout.tv_usec = 0;

        lRet = select(lMaxFd, &stFdset, NULL, NULL, &stTimeout);
        if (-1 == lRet)
        {
            perror("Error happened when select return.");
            g_lCliSocket = -1;
            pthread_exit(NULL);
        }
        else if (0 == lRet)
        {
            continue;
        }

        if(FD_ISSET(g_lCliSocket, &stFdset))
        {
            lRet = recvfrom(g_lCliSocket, szReadBuff, sizeof(szReadBuff), 0, NULL, NULL);
            if (lRet < 0)
            {
                cli_logr_warning("Error happened when read form socket.(%d)", errno);
                continue;
            }
            szReadBuff[lRet] = '\0';

            debug_cli_msg_proc(szReadBuff, lRet);
        }
    }

    cli_logr_debug("%s", (S8 *)"Cli task goodbye!");

    if (g_lCliSocket > 0)
    {
        close(g_lCliSocket);
        g_lCliSocket= -1;
    }

    pthread_exit(NULL);
}


/**
 * 函数：S32 debug_cli_init(S32 _iArgc, S8 **_pszArgv)
 * 功能：初始化模块
 * 参数：
 *      参数表：
 *      unix sock文件路径
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 debug_cli_init(S32 _iArgc, S8 **_pszArgv)
{
    g_lTaskWaitingExit = 0;
    g_lCliSocket = -1;
    S8 szBuffSockPath[256] = { 0 };
    S8 szBuffCMD[256] = { 0 };
    S32 lAddrLen;
    static struct sockaddr_un stSrvAddr;

    //create unix socket
    g_lCliSocket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(g_lCliSocket < 0)
    {
        g_lCliSocket = -1;
        perror("Cannot create communication socket");
        return DOS_FAIL;
    }

    /* 检查目录，如果不存在就要创建 */
    snprintf(szBuffCMD, sizeof(szBuffCMD), "mkdir -p %s/var/run/socket", dos_get_sys_root_path());
    system(szBuffCMD);

    dos_memzero(&stSrvAddr, sizeof(stSrvAddr));
    stSrvAddr.sun_family = AF_UNIX;
    snprintf(szBuffSockPath, sizeof(szBuffSockPath)
                , "%s/var/run/socket/cli-srv-%s.sock"
                , dos_get_sys_root_path()
                , dos_get_process_name());
    dos_strcpy(stSrvAddr.sun_path, szBuffSockPath);
    lAddrLen = offsetof(struct sockaddr_un, sun_path) + dos_strlen(stSrvAddr.sun_path);
    unlink(szBuffSockPath);

    if(bind(g_lCliSocket, (struct sockaddr*)&stSrvAddr, lAddrLen) < 0)
    {
        cli_logr_debug("Cannot bind server addr.(%d)", errno);
        close(g_lCliSocket);
        g_lCliSocket = -1;
        unlink(szBuffSockPath);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * 函数：S32 debug_cli_start()
 * 功能：启动模块
 * 参数：
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 debug_cli_start()
{
    S32 lResult;

    lResult = pthread_create(&g_pthDebugTask, NULL, debug_cli_main_loop, NULL);
    if (lResult != 0)
    {
        return -1;
    }

/*
    i_result = pthread_join(g_pthIDDebugTask, NULL);
    if (i_result != 0)
    {
        return -1;
    }
    */
    return 0;
}

/**
 * 函数：S32 debug_cli_stop()
 * 功能：停止模块
 * 参数：
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 debug_cli_stop()
{
    pthread_mutex_lock(&g_DebugMutex);
    g_lTaskWaitingExit = 1;
    pthread_mutex_unlock(&g_DebugMutex);

    //dos_log(LOG_LEVEL_NOTIC, LOG_TYPE_RUNINFO, (S8 *)"Cli task will be stopped!\n");

    pthread_join(g_pthDebugTask, NULL);

    return 0;
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
