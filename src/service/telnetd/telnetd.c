/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  telnetd.c
 *
 *  Created on: 2014-11-3
 *      Author: Larry
 *        Todo: 提供telnet服务
 *     History:
 */


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if INCLUDE_DEBUG_CLI_SERVER || INCLUDE_PTS

/* include sys header files */
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <netinet/in.h>

/* include private header files */
#include "telnetd.h"
#include "../cli/cli_server.h"
#include "../pts/pts_telnet.h"
#include "../pts/pts_goahead.h"

VOID telnetd_opt_init();
VOID telnetd_set_options4client(FILE *output);
VOID telnetd_send_cmd2client(FILE *output, S32 lCmd, S32 lOpt);

/* 定义最大接收缓存，用在命令行接收输入时 */
#define MAX_RECV_BUFF_LENGTH    512

/* 最大telnet客户端连接数 */
#define MAX_CLIENT_NUMBER       DOS_CLIENT_MAX_NUM

/* 定义telnetserver监听端口 */
#define TELNETD_LISTEN_PORT     DOS_TELNETD_LINSTEN_PORT

/* 特殊按键ascii序列 */
#define TELNETD_SPECIAL_KEY_LEN          8

/* 定义目前支持的最大命令个数 */
#define MAX_HISTORY_CMD_CNT  10

/* 定义两个常用按键 */
#define ESC_ASCII_CODE                   27  /* ESC Key */
#define DEL_ASCII_CODE                   127 /* DEL Key */
#define TAB_ASCII_CODE                   9   /* TAB Key */

/* 定义当前命令位置和最新命令位置 */
S32 m_lLastestIndex = 0, m_lCurIndex = 0, m_lCmdCnt = 0;

enum KEY_MATCH_RET{
    KEY_FULLY_MATCH = 0,
    KEY_PART_MATCH,
    KEY_FAIL,
};

/**
 * 特殊按键枚举
 */
enum KEY_LIST{
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,

    KEY_UP,
    KEY_RIGHT,
    KEY_DOWN,
    KEY_LEFT,

    KEY_BUTT
};

/**
 * telnet读取时的错误码
 */
enum TELNET_RECV_RESULT{
    TELNET_RECV_RESULT_ERROR = -1,
    TELNET_RECV_RESULT_TAB,
    TELNET_RECV_RESULT_HOT_KEY,
    TELNET_RECV_RESULT_NORMAL,

    TELNET_RECV_RESULT_BUTT
};

/**
 * telnet客户端管理控制块
 */
typedef struct tagTelnetClinetInfo{
    U32    ulIndex;     /* 和其他模块通讯使用的编号 */

    U32    ulValid;      /* 是否启用 */
    S32    lSocket;     /* 客户端socket */
    FILE   *pFDInput;   /* 输入描述符 */
    FILE   *pFDOutput;  /* 输出描述符 */

    U32    ulMode;      /* 但前客户端处于什么模式 */
    pthread_t pthClientTask; /* 线程id */

#if INCLUDE_TELNET_PAGE
    DLL_S stCmdList;    /* ctrl_panel客户端命令列表 */
    DLL_NODE_S *pstCurNode; /* 当前的命令指针 */
#endif

#if INCLUDE_PTS
    S32    bIsExit;
    S32    bIsLogin;    /* 是否登陆 */
    S32    bIsConnect;  /* 是否连接ptc */
    S32    bIsGetLineEnd; /* 是否获得行末回车符 */
    S32    bIsRecvLFOrNUL; /* 回车符中是否包含/n或者/0 */
    U32    ulLoginFailCount;
    DOS_TMR_ST stTimerHandle;
#endif

}TELNET_CLIENT_INFO_ST;

/**
 * 特殊按键时需要缓存，缓存结构体
 */
typedef struct tagKeyLisyBuff{
    U32 ulLength;
    S8  szKeyBuff[TELNETD_SPECIAL_KEY_LEN];
}KEY_LIST_BUFF_ST;

/* 模块内部的全局变量 */
/* 对客户端控制块进行保护 */
static pthread_mutex_t g_TelnetdMutex = PTHREAD_MUTEX_INITIALIZER;

/* 服务器线程ID */
static pthread_t g_pthTelnetdTask;

/* 等待退出标志 */
static S32 g_lTelnetdWaitingExit = 0;

/* telnet服务器监听socket */
static S32 g_lTelnetdSrvSocket = -1;

/* 客户端控制块列表 */
static TELNET_CLIENT_INFO_ST *g_pstTelnetClientList[MAX_CLIENT_NUMBER];

/*
 * These are the options we want to use as
 * a telnet server. These are set in telnetd_set_options2client()
 */
static U8 g_aucTelnetOptions[TELNET_MAX_OPTIONS] = { 0 };
static U8 g_taucTelnetWillack[TELNET_MAX_OPTIONS] = { 0 };

/*
 * These are the values we have set or
 * agreed to during our handshake.
 * These are set in telnetd_send_cmd2client(...)
 */
static U8 g_aucTelnetDoSet[TELNET_MAX_OPTIONS]  = { 0 };
static U8 g_aucTelnetWillSet[TELNET_MAX_OPTIONS]= { 0 };

/* 终端类型，默认为ansi */
static S8 g_szTermTye[TELNET_NEGO_BUFF_LENGTH] = {'a','n','s','i', 0};

/*
 * 特殊按键定义,
 *
 * 1.请不要打乱该数组的顺序；
 * 2.请保持该数组顺序与KEY_LIST一致
 */
static const S8 g_szSpecialKey[][TELNETD_SPECIAL_KEY_LEN] = {
        {ESC_ASCII_CODE, 'O', 'P', '\0', },   /* F1 */
        {ESC_ASCII_CODE, 'O', 'Q', '\0', },   /* F2 */
        {ESC_ASCII_CODE, 'O', 'R', '\0', },   /* F3 */
        {ESC_ASCII_CODE, 'O', 'S', '\0', },   /* F4 */
        {ESC_ASCII_CODE, '[', '1', '5' , DEL_ASCII_CODE, '\0', },   /* F5 */
        {ESC_ASCII_CODE, '[', '1', '7' , DEL_ASCII_CODE, '\0', },   /* F6 */
        {ESC_ASCII_CODE, '[', '1', '8' , DEL_ASCII_CODE, '\0', },   /* F7 */
        {ESC_ASCII_CODE, '[', '1', '9' , DEL_ASCII_CODE, '\0', },   /* F8 */
        {ESC_ASCII_CODE, '[', '2', '0' , DEL_ASCII_CODE, '\0', },   /* F9 */
        {ESC_ASCII_CODE, '[', '2', '1' , DEL_ASCII_CODE, '\0', },   /* F10 */

        {ESC_ASCII_CODE, '[', 'A', '\0', },   /* UP */
        {ESC_ASCII_CODE, '[', 'C', '\0', },   /* DOWN */
        {ESC_ASCII_CODE, '[', 'B', '\0', },   /* RIGHT */
        {ESC_ASCII_CODE, '[', 'D', '\0', },   /* LEFT */

        {'\0'}
};

/**
 * 函数：S32 telnet_set_mode(U32 ulIndex, U32 ulMode)
 * 功能：
 *      1.设置index所标示的客户端当前模式
 *      2.提供给外部模块是有
 * 参数
 *      U32 ulIndex：客户端ID
 *      U32 ulMode：模式
 * 返回值：成功返回0，失败返回－1
 */
S32 telnet_set_mode(U32 ulIndex, U32 ulMode)
{
    TELNET_CLIENT_INFO_ST *pstTelnetClient;

    if (ulIndex >= MAX_CLIENT_NUMBER)
    {
        logr_debug("Request telnet server send data to client, but given an invalid client index(%d).", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    if (ulMode >= CLIENT_LEVEL_BUTT)
    {
        logr_debug("Request telnet server send data to client, but given an invalid mode (%d).", ulMode);
        DOS_ASSERT(0);
        return -1;
    }

    pstTelnetClient = g_pstTelnetClientList[ulIndex];
    if (!pstTelnetClient->ulValid)
    {
        logr_debug("Cannot find a valid client which have an index %d.", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    pstTelnetClient->ulMode = ulMode;
    return 0;
}

/**
 * 函数：S32 telnet_out_string(U32 ulIndex, const S8 *pszBuffer)
 * 功能：
 *      1.index所标示的客户端发送数据
 *      2.提供给外部模块是有
 * 参数
 *      U32 ulIndex：客户端ID
 *      S8 *pszBuffer：缓存
 * 返回值：成功返回0，失败返回－1
 */
S32 telnet_out_string(U32 ulIndex, const S8 *pszBuffer)
{
    TELNET_CLIENT_INFO_ST *pstTelnetClient = NULL;

    if (ulIndex >= MAX_CLIENT_NUMBER)
    {
        logr_debug("Request telnet server send data to client, but given an invalid client index(%d).", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    if (!pszBuffer)
    {
        logr_debug("%s", "Request telnet server send data to client, but given an empty buffer.");
        DOS_ASSERT(0);
        return -1;
    }

    pstTelnetClient = g_pstTelnetClientList[ulIndex];
    if (!pstTelnetClient->ulValid)
    {
        logr_debug("Cannot find a valid client which have an index %d.", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    if (pstTelnetClient->lSocket <= 0)
    {
        logr_debug("Client with the index %d have an invalid socket.", ulIndex);
        pstTelnetClient->pFDOutput = NULL;
        pstTelnetClient->pFDOutput = NULL;
        DOS_ASSERT(0);
        return -1;
    }

    fprintf(pstTelnetClient->pFDOutput, "%s", (S8 *)pszBuffer);
    fflush(pstTelnetClient->pFDOutput);

    return 0;
}

#if INCLUDE_PTS

VOID telnet_close_client(U32 ulIndex)
{
    TELNET_CLIENT_INFO_ST *pstTelnetClient;
    if (ulIndex >= MAX_CLIENT_NUMBER)
    {
        logr_debug("Request telnet server send data to client, but given an invalid client index(%d).", ulIndex);
        DOS_ASSERT(0);
        return;
    }
    pstTelnetClient = g_pstTelnetClientList[ulIndex];
    if (pstTelnetClient  != NULL)
    {
        pstTelnetClient->bIsConnect = DOS_FALSE;
        fwrite("\r\n", 2, 1, pstTelnetClient->pFDOutput);
        //fwrite(" >", 2, 1, pstTelnetClient->pFDOutput);
        fflush(pstTelnetClient->pFDOutput);
    }
}


VOID telentd_negotiate_cmd_server(U32 ulIndex, S8 *szBuff, U32 ulLen)
{
    if (NULL == szBuff)
    {
        return;
    }

    /* Various pieces for the telnet communication */
    S8 szNegoResult[TELNET_NEGO_BUFF_LENGTH] = { 0 };
    S32 lDone = 0, lSBMode = 0, lSBLen = 0;
    U8 opt, ch;
    S32 i = 0;
    S8 szSendBuf[100] = {0};

    while (i < ulLen && lDone < 1)
    {
        /* Get either IAC (start command) or a regular character (break, unless in SB mode) */
        ch = (U8)szBuff[i++];
        if (IAC == ch)
        {
            ch = (U8)szBuff[i++];
            switch (ch)
            {
                case SE:
                    /* End of extended option mode */
                    lSBMode = 0;
                    //sprintf(szSendBuf, "%c%c", IAC, SE);
                    //pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 2);
                    break;
                case NOP:
                    /* No Op */
                    //telnetd_send_cmd2client(output, NOP, 0);
                    sprintf(szSendBuf, "%c%c", NOP, 0);
                    pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 2);
                    break;
                case WILL:
                    opt = (U8)szBuff[i++];
                    if (ECHO == opt || SGA == opt)
                    {
                        sprintf(szSendBuf, "%c%c%c", IAC, DO, opt);
                        pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 3);
                    }
                    else
                    {
                        sprintf(szSendBuf, "%c%c%c", IAC, g_taucTelnetWillack[opt], opt);
                        pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 3);
                    }
                    break;
                case WONT:
                    /* Will / Won't Negotiation */
                    opt = (U8)szBuff[i++];

                    if (NAWS != opt || NAOFFD != opt || NAOLFD != opt)
                    {
                        if (!g_taucTelnetWillack[opt])
                        {
                            /* We default to WONT */
                            g_taucTelnetWillack[opt] = DONT;
                        }
                        sprintf(szSendBuf, "%c%c%c", IAC, g_taucTelnetWillack[opt], opt);
                        pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 3);
                    }

                    break;
                case DO:
                    opt = (U8)szBuff[i++];
                    if (TTYPE == opt)
                    {
                         sprintf(szSendBuf, "%c%c%c", IAC, WILL, TTYPE);
                         pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 3);
                         sprintf(szSendBuf, "%c%c%c", IAC, WILL, NAWS);
                         pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 3);
                         //sprintf(szSendBuf, "%c%c%c", IAC, DO, NAOLFD);
                         //pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 3);

                         //sprintf(szSendBuf, "%c%c%c", IAC, WONT, NAOCRD);
                         //pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 3);
                         /*
                         sprintf(szSendBuf, "%c%c%c", IAC, DO, NAOFFD);
                         pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 3);
                         */
                    }
                    else
                    {
                         if (!g_aucTelnetOptions[opt])
                         {
                             /* We default to DONT */
                             g_aucTelnetOptions[opt] = WONT;
                         }

                         sprintf(szSendBuf, "%c%c%c", IAC, g_aucTelnetOptions[opt], opt);
                         pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 3);
                    }
                    break;
                case DONT:
                    /* Do / Don't Negotiation */
                    opt = (U8)szBuff[i++];
                    if (NAWS == opt)
                    {
                       // sprintf(szSendBuf, "%c%c%c", IAC, WONT, NAWS);
                       // pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 3);
                    }
                    else
                    {
                        if (!g_aucTelnetOptions[opt])
                        {
                             /* We default to DONT */
                             g_aucTelnetOptions[opt] = WONT;
                        }
                        sprintf(szSendBuf, "%c%c%c", IAC, g_aucTelnetOptions[opt], opt);
                        pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 3);
                    }
                    break;
                case SB:
                    /* Begin Extended Option Mode */
                    opt = (U8)szBuff[i++];
                    if (TTYPE == opt)
                    {
                        /* 终端类型 */
                        opt = (U8)szBuff[i++];
                        if (SEND == opt)
                        {
                             sprintf(szSendBuf, "%c%c%c%c%s%c%c", IAC, SB, TTYPE, IS, g_szTermTye, IAC, SE);
                             pts_telnet_send_msg2ptc(ulIndex, szSendBuf, 6+dos_strlen(g_szTermTye));
                        }
                        else if (IS == opt)
                        {
                            lSBMode = 1;
                            lSBLen  = 0;
                            memset(szNegoResult, 0, sizeof(szNegoResult));
                        }
                    }
                    break;
                case IAC:
                    /* IAC IAC? That's probably not right. */
                    lDone = 2;
                    break;
                default:
                    break;
            }
        }
        else if (lSBMode)
        {
            /* Extended Option Mode -> Accept character */
            if (lSBLen < (sizeof(szNegoResult) - 1))
            {
                /* Append this character to the SB string,
                 * but only if it doesn't put us over
                 * our limit; honestly, we shouldn't hit
                 * the limit, as we're only collecting characters
                 * for a terminal type or window size, but better safe than
                 * sorry (and vulnerable).
                 */
                szNegoResult[lSBLen++] = ch;
            }
        }
    }

}

VOID pts_recv_cr_timeout(U64 ulParam)
{
    TELNET_CLIENT_INFO_ST *pstClientInfo = (TELNET_CLIENT_INFO_ST *)ulParam;
    pstClientInfo->bIsGetLineEnd = DOS_TRUE;
    pstClientInfo->stTimerHandle = NULL;
}

#endif

/*
 * Negotiate the telnet options.
 */
VOID telnetd_negotiate(FILE *output, FILE *input)
{
    /* The default terminal is ANSI */
    //S8 szTerm[TELNET_NEGO_BUFF_LENGTH] = {'a','n','s','i', 0};
    /* Various pieces for the telnet communication */
    S8 szNegoResult[TELNET_NEGO_BUFF_LENGTH] = { 0 };
    S32 lDone = 0, lSBMode = 0, lDoEcho = 0, lSBLen = 0, lMaxNegoTime = 0;
    U8 opt, i;

    if (!output || !input)
    {
        DOS_ASSERT(0);
        return;
    }

    telnetd_opt_init();

    /* Set the default options. */
    telnetd_set_options4client(output);

    /* Let's do this */
    while (!feof(stdin) && lDone < 1)
    {
        if (lMaxNegoTime > TELNET_MAX_NEGO_TIME)
        {
            DOS_ASSERT(0);
            return;
        }

        /* Get either IAC (start command) or a regular character (break, unless in SB mode) */
        i = getc(input);
        if (IAC == i)
        {
            /* If IAC, get the command */
            i = getc(input);
            switch (i)
            {
                case SE:
                    /* End of extended option mode */
                    lSBMode = 0;
                    if (TTYPE == szNegoResult[0])
                    {
                        //alarm(0);
                        //is_telnet_client = 1;
                        /* This was a response to the TTYPE command, meaning
                         * that this should be a terminal type */
                        dos_strncpy(g_szTermTye, &szNegoResult[2], sizeof(g_szTermTye) - 1);
                        g_szTermTye[sizeof(g_szTermTye) - 1] = 0;
                        ++lDone;
                    }
                    break;
                case NOP:
                    /* No Op */
                    telnetd_send_cmd2client(output, NOP, 0);
                    fflush(output);
                    break;
                case WILL:
                case WONT:
                    /* Will / Won't Negotiation */
                    opt = getc(input);
                    if (opt < 0 || opt >= sizeof(g_taucTelnetWillack))
                    {
                        DOS_ASSERT(0);
                        return;
                    }
                    if (!g_taucTelnetWillack[opt])
                    {
                        /* We default to WONT */
                        g_taucTelnetWillack[opt] = WONT;
                    }
                    telnetd_send_cmd2client(output, g_taucTelnetWillack[opt], opt);
                    fflush(output);
                    if ((WILL == i) && (TTYPE == opt))
                    {
                        /* WILL TTYPE? Great, let's do that now! */
                        fprintf(output, "%c%c%c%c%c%c", IAC, SB, TTYPE, SEND, IAC, SE);
                        fflush(output);
                    }
                    break;
                case DO:
                case DONT:
                    /* Do / Don't Negotiation */
                    opt = getc(input);
                    if (opt < 0 || opt >= sizeof(g_aucTelnetOptions))
                    {
                        DOS_ASSERT(0);
                        return;
                    }
                    if (!g_aucTelnetOptions[opt])
                    {
                        /* We default to DONT */
                        g_aucTelnetOptions[opt] = DONT;
                    }
                    telnetd_send_cmd2client(output, g_aucTelnetOptions[opt], opt);

                    if (opt == ECHO)
                    {
                        lDoEcho = (i == DO);
                        //DOS_ASSERT(lDoEcho);
                    }
                    fflush(output);
                    break;
                case SB:
                    /* Begin Extended Option Mode */
                    lSBMode = 1;
                    lSBLen  = 0;
                    memset(szNegoResult, 0, sizeof(szNegoResult));
                    break;
                case IAC:
                    /* IAC IAC? That's probably not right. */
                    lDone = 2;
                    break;
                default:
                    break;
            }
        }
        else if (lSBMode)
        {
            /* Extended Option Mode -> Accept character */
            if (lSBLen < (sizeof(szNegoResult) - 1))
            {
                /* Append this character to the SB string,
                 * but only if it doesn't put us over
                 * our limit; honestly, we shouldn't hit
                 * the limit, as we're only collecting characters
                 * for a terminal type or window size, but better safe than
                 * sorry (and vulnerable).
                 */
                szNegoResult[lSBLen++] = i;
            }
        }
    }
    /* What shall we now do with term, ttype, do_echo, and terminal_width? */
}


/**
 * 函数：S32 telnet_out_string(U32 ulIndex, S8 *pszBuffer)
 * 功能：
 *      1.index所标示的客户端发送数据
 *      2.提供给外部模块是有
 *      3.buffer需要携带结束符
 * 参数
 *      U32 ulIndex：客户端ID
 *      S8 *pszBuffer：缓存
 * 返回值：成功返回0，失败返回－1
 */
S32 telnet_send_data(U32 ulIndex, U32 ulType, S8 *pszBuffer, U32 ulLen)
{
    TELNET_CLIENT_INFO_ST *pstTelnetClient;
    U32 i;
#if INCLUDE_PTS
    //S8 szNegotiateEnd[4] = {0};
    S8 *pStrstr = NULL;
#endif
    if (!pszBuffer || ulLen<=0)
    {
        logr_debug("%s", "Request telnet server send data to client, but given an empty buffer.");
        DOS_ASSERT(0);
        return -1;
    }

    /* 强制给结束符 */
    //pszBuffer[ulLen] = '\0';

    if (MSG_TYPE_LOG == ulType)
    {
        logr_debug("%s", "Request telnet server send log to client.");
        goto broadcast;
    }

    if (ulIndex >= MAX_CLIENT_NUMBER)
    {
        logr_debug("Request telnet server send data to client, but given an invalid client index(%d).", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    pstTelnetClient = g_pstTelnetClientList[ulIndex];
    if (!pstTelnetClient->ulValid)
    {
        logr_debug("Cannot find a valid client which have an index %d.", ulIndex);
        DOS_ASSERT(0);
        return -1;
    }

    if (pstTelnetClient->lSocket <= 0)
    {
        logr_debug("Client with the index %d have an invalid socket.", ulIndex);
        pstTelnetClient->pFDOutput = NULL;
        pstTelnetClient->pFDOutput = NULL;
        DOS_ASSERT(0);
        return -1;
    }
#if INCLUDE_PTS
    pStrstr = memchr(pszBuffer, 0xff, ulLen);
    if (pStrstr != NULL)
    {
        telentd_negotiate_cmd_server(ulIndex, pStrstr, ulLen-(pStrstr - pszBuffer));
        pStrstr = strstr(pszBuffer, "\r\n");
        if (pStrstr != NULL)
        {
            fwrite((S8 *)pStrstr, ulLen-(pStrstr - pszBuffer), 1, pstTelnetClient->pFDOutput);
            fflush(pstTelnetClient->pFDOutput);
        }
    }
    else
#endif
    {
        fwrite((S8 *)pszBuffer, ulLen, 1, pstTelnetClient->pFDOutput);
        fflush(pstTelnetClient->pFDOutput);
    }

    return 0;

broadcast:

    for (i=0; i<MAX_CLIENT_NUMBER; i++)
    {
        if (g_pstTelnetClientList[i]->ulValid
            && CLIENT_LEVEL_LOG == g_pstTelnetClientList[i]->ulMode)
        {
            fprintf(g_pstTelnetClientList[i]->pFDOutput, "%s", (S8 *)pszBuffer);
            fflush(g_pstTelnetClientList[i]->pFDOutput);
        }
    }

    logr_debug("Send data to client, client index:%d length:%d", ulIndex, ulLen);

    return 0;
}


/**
 * 函数：VOID telnet_opt_init()
 * 功能：初始化telnet协商相关数据
 * 参数：
 * 返回值：
 */
VOID telnetd_opt_init()
{
    dos_memzero(g_aucTelnetOptions, sizeof(g_aucTelnetOptions));
    dos_memzero(g_taucTelnetWillack, sizeof(g_taucTelnetWillack));

    dos_memzero(g_aucTelnetDoSet, sizeof(g_aucTelnetDoSet));
    dos_memzero(g_aucTelnetWillSet, sizeof(g_aucTelnetWillSet));
}


/**
 * 函数：VOID telnetd_send_cmd2client(FILE *output, S32 lCmd, S32 lOpt)
 * 功能：
 *      用于和客户端协商时向客户端发送命令字
 * 参数
 *      FILE *output: 输出描述符
 *      S32 lCmd：命令字
 *      S32 lOpt：命令值
 * 返回值：void
 */
VOID telnetd_send_cmd2client(FILE *output, S32 lCmd, S32 lOpt)
{
    if (!output)
    {
        DOS_ASSERT(0);
        return;
    }

    /* Send a command to the telnet client */
    if (lCmd == DO || lCmd == DONT)
    {
        /* DO commands say what the client should do. */
        if (((lCmd == DO) && (g_aucTelnetDoSet[lOpt] != DO)) || ((lCmd == DONT) && (g_aucTelnetDoSet[lOpt] != DONT)))
        {
            /* And we only send them if there is a disagreement */
            g_aucTelnetDoSet[lOpt] = lCmd;
            fprintf(output, "%c%c%c", IAC, lCmd, lOpt);
        }
    }
    else if (lCmd == WILL || lCmd == WONT)
    {
        /* Similarly, WILL commands say what the server will do. */
        if (((lCmd == WILL) && (g_aucTelnetWillSet[lOpt] != WILL)) || ((lCmd == WONT) && (g_aucTelnetWillSet[lOpt] != WONT)))
        {
            /* And we only send them during disagreements */
            g_aucTelnetWillSet[lOpt] = lCmd;
            fprintf(output, "%c%c%c", IAC, lCmd, lOpt);
        }
    }
    else
    {
        /* Other commands are sent raw */
        fprintf(output, "%c%c", IAC, lCmd);
    }
    fflush(output);
}

/*
 * Set the default options for the telnet server.
 */
VOID telnetd_set_options4client(FILE *output)
{
    S32 option;

    if (!output)
    {
        DOS_ASSERT(0);
        return;
    }

    /* We will echo input */
    g_aucTelnetOptions[ECHO] = WILL;
    /* We will set graphics modes */
    g_aucTelnetOptions[SGA] = WILL;
    /* We will not set new environments */
    g_aucTelnetOptions[NEW_ENVIRON] = WONT;

    g_taucTelnetWillack[NAOLFD] = DO;

    /* The client should not echo its own input */
    g_taucTelnetWillack[ECHO] = DONT;
    /* The client can set a graphics mode */
    g_taucTelnetWillack[SGA] = DO;
    /* We do not care about window size updates */
    g_taucTelnetWillack[NAWS] = DONT;
    /* The client should tell us its terminal type (very important) */
    g_taucTelnetWillack[TTYPE] = DO;
    /* No linemode */
    g_taucTelnetWillack[LINEMODE] = DONT;
    /* And the client can set a new environment */
    g_taucTelnetWillack[NEW_ENVIRON] = DO;


    /* Let the client know what we're using */
    for (option = 0; option < sizeof(g_aucTelnetOptions); ++option)
    {
        if (g_aucTelnetOptions[option])
        {
            telnetd_send_cmd2client(output, g_aucTelnetOptions[option], option);
        }
    }
    for (option = 0; option < sizeof(g_taucTelnetWillack); ++option)
    {
        if (g_taucTelnetWillack[option])
        {
            telnetd_send_cmd2client(output, g_taucTelnetWillack[option], option);
        }
    }

#if 0
    fprintf(output, "%c%c%c", IAC, WILL, ECHO);
    fprintf(output, "%c%c%c", IAC, WILL, SGA);
    fprintf(output, "%c%c%c", IAC, WONT, NEW_ENVIRON);
    fflush(output);
    fprintf(output, "%c%c%c", IAC, DONT, ECHO);
    fprintf(output, "%c%c%c", IAC, DO, SGA);
    fprintf(output, "%c%c%c", IAC, DO, TTYPE);
    fprintf(output, "%c%c%c", IAC, DONT, NAWS);
    fprintf(output, "%c%c%c", IAC, DONT, LINEMODE);
    fprintf(output, "%c%c%c", IAC, DO, NEW_ENVIRON);

    fflush(output);
#endif

}


/**
 * 函数：VOID telnetd_client_output_prompt(FILE *output, U32 ulMode)
 * 功能：
 *      用于向客户端输出命令提示符
 * 参数
 *      FILE *output: 输出描述符
 *      U32 ulMode:当前客户端所处的模式
 * 返回值：void
 */
VOID telnetd_client_output_prompt(FILE *output, U32 ulMode)
{
#define check_fd1(_fd) \
do{ \
    if (feof(_fd)) \
    { \
        DOS_ASSERT(0); \
        return; \
    }\
}while(0)
    check_fd1(output);
    if (CLIENT_LEVEL_LOG == ulMode)
    {
        fprintf(output, "\r #");
    }
    else
    {
        fprintf(output, "\r >");
    }
    fflush(output);
}


/**
 * 函数：VOID telnet_send_new_line(FILE* output, S32 ulLines)
 * 功能：向output描述符输出空行
 * 参数：
 *      FILE* output：指向telnet客户端的描述符
 *      S32 ulLines：需要输出几个空行
 * 返回值：
 */
VOID telnetd_client_send_new_line(FILE* output, S32 ulLines)
{
    S32 i;

    for (i=0; i<ulLines; i++)
    {
        /* Send the telnet newline sequence */
        putc('\r', output);
        putc(0, output);
        putc('\n', output);
        //fflush(output);
    }
}
S32 telnet_client_tab_proc(S8 *pszCurrCmd, FILE *pfOutput)
{
    return DOS_SUCC;
}

#if INCLUDE_TELNET_PAGE
/**
 * 函数：telnet_client_special_key_proc
 * 功能：处理客户端输入的特殊按键，包括方向件，以及f1-f10
 * 参数：
 *      U32 ulKey： 特殊键枚举值
 *      FILE *pfOutput：telnet客户端输出流
 *      S8 *pszCurrCmd：
 *      S32 lMaxLen,
 *      S32 *plCurrent
 * 返回值：
 *      成功返回接收缓存的长度，失败返回－1
 */
S32 telnet_client_special_key_proc(U32 ulKey, TELNET_CLIENT_INFO_ST *pstClientInfo, S8 *szBuffer, S32 *plSize)
{
    switch(ulKey)
    {
        /* 处理方向键 */
        case KEY_UP:
            {
                /* 如果说当前节点的前驱节点不是头结点，前移指针 */
                if (DLL_First(&pstClientInfo->stCmdList) != pstClientInfo->pstCurNode)
                {
                    pstClientInfo->pstCurNode = DLL_Previous(&pstClientInfo->stCmdList, pstClientInfo->pstCurNode);
                    *plSize = dos_snprintf(szBuffer, MAX_RECV_BUFF_LENGTH, "%s", (S8*)pstClientInfo->pstCurNode->pHandle);
                }
                else
                {
                    pstClientInfo->pstCurNode = DLL_First(&pstClientInfo->stCmdList);
                    if (pstClientInfo->pstCurNode && pstClientInfo->pstCurNode->pHandle)
                    {
                        *plSize = dos_snprintf(szBuffer, MAX_RECV_BUFF_LENGTH, "%s", (S8*)pstClientInfo->pstCurNode->pHandle);
                    }
                }
                break;
            }
        case KEY_DOWN:
            {
                if (DLL_Last(&pstClientInfo->stCmdList) != pstClientInfo->pstCurNode)
                {
                    pstClientInfo->pstCurNode = DLL_Next(&pstClientInfo->stCmdList, pstClientInfo->pstCurNode);
                    *plSize = dos_snprintf(szBuffer, MAX_RECV_BUFF_LENGTH, "%s", (S8*)pstClientInfo->pstCurNode->pHandle);
                }
                else
                {
                    pstClientInfo->pstCurNode = DLL_Last(&pstClientInfo->stCmdList);
                    if (pstClientInfo->pstCurNode && pstClientInfo->pstCurNode->pHandle)
                    {
                        *plSize = dos_snprintf(szBuffer, MAX_RECV_BUFF_LENGTH, "%s", (S8*)pstClientInfo->pstCurNode->pHandle);
                    }
                }
                break;
            }
        case KEY_F1:
            break;
        case KEY_F2:
        case KEY_F3:
        case KEY_F4:
        case KEY_F5:
        case KEY_F6:
        case KEY_F7:
        case KEY_F8:
        case KEY_F9:
        case KEY_F10:
        case KEY_LEFT:
        case KEY_RIGHT:
        default:
            break;
    }

    return DOS_SUCC;
}
#endif

/**
 * 函数：S32 telnet_read_line(FILE *input, FILE *output, S8 *szBuffer, S32 lSize, S32 lMask)
 * 功能：读取客户端输入，直到回车
 * 参数：
 *      FILE *input：读取描述符
 *      FILE *output：输出描述符
 *      S8 *szBuffer：接收缓存
 *      S32 lSize：缓存大小
 *      S32 lMask：是否需要掩盖回显
 * 返回值：
 *      成功返回接收缓存的长度，失败返回－1
 */
S32 telnetd_client_read_line(TELNET_CLIENT_INFO_ST *pstClientInfo, S8 *szBuffer, S32 lSize, S32 lMask)
{
    S32 lLength;
    U8  c;
    struct stat buf;
    KEY_LIST_BUFF_ST stKeyList;
    S8  szLastCmd[512];
    S32 lCmdLen = 0, lSpecialKeyMatchRet;
    S32 lRes = 0;
#if 1
    S32 lLoop;
#endif

    if (!pstClientInfo->pFDInput || !pstClientInfo->pFDOutput)
    {
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }

    if (!szBuffer || lSize <= 0)
    {
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }


#define check_fd(_fd) \
do{ \
    if (feof(_fd)) \
    { \
        DOS_ASSERT(0); \
        return TELNET_RECV_RESULT_ERROR; \
    }\
}while(0)

    /* We make sure to restore the cursor. */

    if (fstat(fileno(pstClientInfo->pFDOutput), &buf) != -1)
    {
        lRes = fprintf(pstClientInfo->pFDOutput, "\033[?25h");
        if (lRes < 0)
        {
            DOS_ASSERT(0);
            return TELNET_RECV_RESULT_ERROR;
        }
        check_fd(pstClientInfo->pFDOutput);
        fflush(pstClientInfo->pFDOutput);
    }

    if (feof(pstClientInfo->pFDInput))
    {
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }

    stKeyList.ulLength = 0;
    stKeyList.szKeyBuff[0] = '\0';

    for (lLength = 0; lLength < lSize - 1; lLength++)
    {
        c = getc(pstClientInfo->pFDInput);

        check_fd(pstClientInfo->pFDInput);
#if 1
        if (TAB_ASCII_CODE == c)
        {
            szBuffer[lLength] = '\0';
            return TELNET_RECV_RESULT_TAB;
        }
        /* 本流程主要检查左右方向键 */
        else if (ESC_ASCII_CODE == c || 0 != stKeyList.ulLength)
        {
            stKeyList.szKeyBuff[stKeyList.ulLength] = c;
            stKeyList.ulLength++;
            stKeyList.szKeyBuff[stKeyList.ulLength] = '\0';

            /* 检查特殊按键 */
            lSpecialKeyMatchRet = KEY_FAIL;
            for (lLoop = 0; lLoop < KEY_BUTT; lLoop++)
            {
                /* 制定长度比较，如果没有匹配成功，说明已经不是特殊按键了 */
                if (dos_strncmp(g_szSpecialKey[lLoop], stKeyList.szKeyBuff, stKeyList.ulLength) == 0)
                {
                    /* 并将是否匹配特殊按键置为0 */
                    lSpecialKeyMatchRet = KEY_PART_MATCH;
                }

                /* 全比较，如果没有匹配成功，有可能时还没有收完，或者是别的特殊按键，就要继续查找 */
                if (dos_strcmp(g_szSpecialKey[lLoop], stKeyList.szKeyBuff) == 0)
                {
                    //printf("\nloop:%d\n", lLoop);
                    lSpecialKeyMatchRet = KEY_FULLY_MATCH;
                    break;
                }
            }

            if (KEY_FULLY_MATCH == lSpecialKeyMatchRet)
            {

                /* 全比较成功，匹配特殊按键成功 */
                lCmdLen = sizeof(szLastCmd);

#if INCLUDE_TELNET_PAGE
                telnet_client_special_key_proc(lLoop, pstClientInfo, szLastCmd, &lCmdLen);
#endif
                if (lCmdLen > 0)
                {
                    /* 删除 */
                    while (lLength > 0)
                    {
                        lLength--;
                        /* 光标后退一个，输出一个空格， 光标再次后退，实现删除字符的功能*/
                        fprintf(pstClientInfo->pFDOutput, "\b \b");
                        check_fd(pstClientInfo->pFDOutput);
                        fflush(pstClientInfo->pFDOutput);
                    }

                    /* 打印命令 */
                    dos_strncpy(szBuffer, szLastCmd, lSize);
                    szBuffer[lCmdLen] = '\0';
                    fprintf(pstClientInfo->pFDOutput, szBuffer);
                    fflush(pstClientInfo->pFDOutput);
                    stKeyList.ulLength = 0;
                    stKeyList.szKeyBuff[0] = '\0';

                    lLength = lCmdLen - 1;
                }
                else
                {
                    stKeyList.ulLength = 0;
                    stKeyList.szKeyBuff[0] = '\0';
                }

                continue;
            }
            else if (KEY_PART_MATCH == lSpecialKeyMatchRet)
            {
                lLength--;
                continue;
            }
            else
            {
                /* 成功就要处理特殊按键，失败要将所有缓存字符回显 */
                for (lLoop=0; lLoop < stKeyList.ulLength; lLoop++)
                {
                    putc(stKeyList.szKeyBuff[lLoop], pstClientInfo->pFDOutput);
                }

                stKeyList.ulLength = 0;
                stKeyList.szKeyBuff[0] = '\0';
            }

            continue;
        }
#endif

#if INCLUDE_PTS
        if (c == '\r')
        {
            if (!pstClientInfo->bIsGetLineEnd)
            {
                lRes = dos_tmr_start(&pstClientInfo->stTimerHandle, PTS_TELNET_CR_TIME, pts_recv_cr_timeout, (U64)pstClientInfo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                }
                telnetd_client_send_new_line(pstClientInfo->pFDOutput, 1);
                continue;
            }
            else
            {
                if (pstClientInfo->bIsRecvLFOrNUL)
                {
                    getc(pstClientInfo->pFDInput);
                }
                telnetd_client_send_new_line(pstClientInfo->pFDOutput, 1);
                break;
            }
        }
        else if (c == '\0' || c == '\n')
        {
            if (!pstClientInfo->bIsGetLineEnd)
            {
                pstClientInfo->bIsRecvLFOrNUL = DOS_TRUE;
            }
            break;
        }
#else
        if (c == '\r' || c == '\n')
        {
            if (c == '\r')
            {
            /* the next char is either \n or \0, which we can discard. */
                getc(pstClientInfo->pFDInput);
            }
            telnetd_client_send_new_line(pstClientInfo->pFDOutput, 1);

            break;
        }
#endif
        else if (c == '\b' || c == 0x7f)
        {
            if (!lLength)
            {
                lLength = -1;
                continue;
            }
            if (lMask)
            {
                lRes = fprintf(pstClientInfo->pFDOutput, "\033[%dD\033[K", lLength);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    return TELNET_RECV_RESULT_ERROR;
                }
                check_fd(pstClientInfo->pFDOutput);
                fflush(pstClientInfo->pFDOutput);
                lLength = -1;
                continue;
            }
            else
            {
                lRes = fprintf(pstClientInfo->pFDOutput, "\b \b");
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    return TELNET_RECV_RESULT_ERROR;
                }
                check_fd(pstClientInfo->pFDOutput);
                fflush(pstClientInfo->pFDOutput);
                lLength -= 2;
                continue;
            }
        }
#if INCLUDE_PTS
        else if (c == 0xff)
        /* 客户端退出 */
#else
        else if (c == 0xff || '\0' == c)
#endif
        {
            DOS_ASSERT(0);
            return TELNET_RECV_RESULT_ERROR;
        }
        else if (iscntrl(c))
        {
            --lLength;
            continue;
        }

        szBuffer[lLength] = c;
        putc(lMask ? '*' : c, pstClientInfo->pFDOutput);
        check_fd(pstClientInfo->pFDOutput);
        fflush(pstClientInfo->pFDOutput);
    }

    szBuffer[lLength] = 0;

    /* And we hide it again at the end. */
    lRes = fprintf(pstClientInfo->pFDOutput, "\033[?25l");
    if (lRes < 0)
    {
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }
    check_fd(pstClientInfo->pFDOutput);
    fflush(pstClientInfo->pFDOutput);

    return TELNET_RECV_RESULT_NORMAL;
}


#if INCLUDE_PTS
S32 telnetd_client_read_char(TELNET_CLIENT_INFO_ST *pstClientInfo, S8 *szBuffer)
{
    U8  c;
    S32 lSize = 1;
    KEY_LIST_BUFF_ST stKeyList;
    S32 lRes = 0;
#if 0
    S32 lLoop, lSpecialKeyMatchRet;
#endif

    if (!pstClientInfo->pFDInput || !pstClientInfo->pFDOutput)
    {
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }

    if (!szBuffer || lSize <= 0)
    {
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }

#define check_fd(_fd) \
do{ \
    if (feof(_fd)) \
    { \
        DOS_ASSERT(0); \
        return TELNET_RECV_RESULT_ERROR; \
    }\
}while(0)

    /* We make sure to restore the cursor. */
    lRes = fprintf(pstClientInfo->pFDOutput, "\033[?25h");
    if (lRes < 0)
    {
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }
    check_fd(pstClientInfo->pFDOutput);

    fflush(pstClientInfo->pFDOutput);

    if (feof(pstClientInfo->pFDInput))
    {
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }

    stKeyList.ulLength = 0;
    stKeyList.szKeyBuff[0] = '\0';
    c = getc(pstClientInfo->pFDInput);
    check_fd(pstClientInfo->pFDInput);
#if 0
    if (c == 0xff)
    {
        /* 客户端退出 */
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }
#endif
    *szBuffer = c;

    if ('\r' == c)
    {
        if (pstClientInfo->bIsRecvLFOrNUL)
        {
            getc(pstClientInfo->pFDInput);
        }
        *szBuffer = '\n';
    }
    else if (c == 0xff)
    {
        DOS_ASSERT(0);
        return TELNET_RECV_RESULT_ERROR;
    }
    //putc(c, output);
    //check_fd(output);
    //fflush(output);

    return TELNET_RECV_RESULT_NORMAL;

}

#endif

/**
 * 函数：TELNET_CLIENT_INFO_ST *telnetd_client_alloc(S32 lSock)
 * 功能：
 *      查找空闲控制块，初始化，然后返回
 * 参数:
 *      S32 lSock: 新客户端的socket
 * 返回值：成功返回控制块指针，失败返回0
 */
TELNET_CLIENT_INFO_ST *telnetd_client_alloc(S32 lSock)
{
    S32 i;

    for (i=0; i<MAX_CLIENT_NUMBER; i++)
    {
        if (!g_pstTelnetClientList[i]->ulValid)
        {
            break;
        }
    }

    if (i >= MAX_CLIENT_NUMBER)
    {
        logr_notice("%s", "Too many user.");
        DOS_ASSERT(0);
        return NULL;
    }

    g_pstTelnetClientList[i]->ulValid = DOS_TRUE;
    g_pstTelnetClientList[i]->lSocket = lSock;
    g_pstTelnetClientList[i]->pFDInput = NULL;
    g_pstTelnetClientList[i]->pFDOutput = NULL;
    g_pstTelnetClientList[i]->ulMode = CLIENT_LEVEL_CONFIG;
#if INCLUDE_PTS
    g_pstTelnetClientList[i]->bIsExit = DOS_FALSE;
    g_pstTelnetClientList[i]->bIsLogin = DOS_FALSE;
    g_pstTelnetClientList[i]->bIsConnect = DOS_FALSE;
    g_pstTelnetClientList[i]->bIsGetLineEnd = DOS_FALSE;
    g_pstTelnetClientList[i]->bIsRecvLFOrNUL = DOS_FALSE;
    g_pstTelnetClientList[i]->stTimerHandle = NULL;
    g_pstTelnetClientList[i]->ulLoginFailCount = 0;
#endif
    return g_pstTelnetClientList[i];
}

/**
 * 函数：U32 telnetd_client_count()
 * 功能：
 *      统计激化状态的客户端控制块
 * 参数:
 * 返回值：成功返回激活状态控制块数量，失败返回0
 */
U32 telnetd_client_count()
{
    U32 ulCount = 0, i;

    for (i=0; i<MAX_CLIENT_NUMBER; i++)
    {
        if (g_pstTelnetClientList[i]->ulValid)
        {
            ulCount++;
        }
    }

    return ulCount;
}


/**
 * 函数：VOID *telnetd_client_task(VOID *ptr)
 * 功能：
 *      telnet客户端接收线程
 * 参数:
 * 返回值：
 */
VOID *telnetd_client_task(VOID *ptr)
{
    TELNET_CLIENT_INFO_ST *pstClientInfo;
    // struct rlimit limit;
    S8 szRecvBuff[MAX_RECV_BUFF_LENGTH];
#if INCLUDE_TELNET_PAGE
    S8 *pszHistoryCmd = NULL;   /* 存储命令的缓存 */
    DLL_NODE_S *pstTempNode = NULL; /* 用来临时存储被删除的节点 */
#endif
    S32 lLen;
    S8 szCmd[16] = { 0 }, *pszPtr = NULL;
#if INCLUDE_PTS
    S8 szUseName[MAX_RECV_BUFF_LENGTH];
    S8 cRecvChar;
    S8 *pcPassWord = NULL;
    S32 lResult = 0;
    S8 *pPassWordMd5 = NULL;
#endif
    if (!ptr)
    {
        logr_warning("%s", "New telnet client thread with invalid param, exit.");
        DOS_ASSERT(0);
        pthread_exit(NULL);
    }

    pstClientInfo = (TELNET_CLIENT_INFO_ST *)ptr;

    /* 配置／协商telnet */
    // limit.rlim_cur = limit.rlim_max = 90;
    // setrlimit(RLIMIT_CPU, &limit);
    // limit.rlim_cur = limit.rlim_max = 0;
    // setrlimit(RLIMIT_NPROC, &limit);

    pstClientInfo->pFDInput = fdopen(pstClientInfo->lSocket, "r");
    if (!pstClientInfo->pFDInput) {
        logr_warning("%s", "Create input fd fail for the new telnet client.");
        DOS_ASSERT(0);
        pthread_exit(NULL);
    }
    pstClientInfo->pFDOutput = fdopen(pstClientInfo->lSocket, "w");
    if (!pstClientInfo->pFDOutput) {
        logr_warning("%s", "Create output fd fail for the new telnet client.");
        DOS_ASSERT(0);
        pthread_exit(NULL);
    }

    telnetd_negotiate(pstClientInfo->pFDOutput, pstClientInfo->pFDInput);

    dos_printf("%s", "Telnet negotiate finished.");

#if INCLUDE_DEBUG_CLI_SERVER
    fprintf(pstClientInfo->pFDOutput, "Welcome Control Panel!");
    telnetd_client_send_new_line(pstClientInfo->pFDOutput, 1);
    fflush(pstClientInfo->pFDOutput);
#else
    fprintf(pstClientInfo->pFDOutput, "Welcome to OK-RAS!\n\rPlease input Enter to continue.");
    telnetd_client_read_line(pstClientInfo, szRecvBuff, sizeof(szRecvBuff), 0);
#endif


#if INCLUDE_DEBUG_CLI_SERVER
    while (1)
    {
        /* 输出提示符 */
        telnetd_client_output_prompt(pstClientInfo->pFDOutput, pstClientInfo->ulMode);

        /* 读取命令行输入 */
        lLen = telnetd_client_read_line(pstClientInfo, szRecvBuff, sizeof(szRecvBuff), 0);
        if (lLen < 0)
        {
            logo_notice("Telnetd", "Telnet Exit", DOS_TRUE, "%s", "Telnet client exit.");
            break;
        }

        if (0 == lLen)
        {
            continue;
        }

        pszPtr = dos_strstr(szRecvBuff, "\n");
        if (pszPtr)
        {
            *pszPtr = '\0';
        }
        pszPtr = dos_strstr(szRecvBuff, "\r");
        if (pszPtr)
        {
            *pszPtr = '\0';
        }
#if INCLUDE_TELNET_PAGE
        if (0 == dos_is_printable_str(szRecvBuff))
        {
            /* 如果说链表长度是MAX_HISTROY_CMD_CNT，则不用添加新节点 */
            if (pstClientInfo->stCmdList.ulCount >= MAX_HISTORY_CMD_CNT)
            {
                /* 去掉一个节点，并复制数据 */
                pstTempNode = dll_fetch(&pstClientInfo->stCmdList);
                pszHistoryCmd = (S8*)pstTempNode->pHandle;

                dos_snprintf(pszHistoryCmd, MAX_RECV_BUFF_LENGTH, "%s", szRecvBuff);

                /* 重新加入该队列 */
                DLL_Add(&pstClientInfo->stCmdList, pstTempNode);

                /* 维护好最新的位置 */
                pstClientInfo->pstCurNode = &(pstClientInfo->stCmdList.Head);
            }
            else
            {
                /* 先分配好资源 */
                pszHistoryCmd = (S8 *)dos_dmem_alloc(MAX_RECV_BUFF_LENGTH);
                if (DOS_ADDR_INVALID(pszHistoryCmd))
                {
                    DOS_ASSERT(0);
                    return NULL;
                }

                pstTempNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
                if (DOS_ADDR_INVALID(pstTempNode))
                {
                    DOS_ASSERT(0);
                    return NULL;
                }

                /* 初始化节点 */
                DLL_Init_Node(pstTempNode);

                /* 复制数据并加入链表 */
                pstTempNode->pHandle = (VOID *)pszHistoryCmd;
                dos_snprintf(pszHistoryCmd, MAX_RECV_BUFF_LENGTH, "%s", szRecvBuff);

                DLL_Add(&pstClientInfo->stCmdList, pstTempNode);

                /* 维护好最新的位置 */
                pstClientInfo->pstCurNode = &(pstClientInfo->stCmdList.Head);
            }

        }
#endif  //end INCLUDE_TELNET_PAGE

        /* 分析并执行命令 */
        cli_server_cmd_analyse(pstClientInfo->ulIndex, pstClientInfo->ulMode, szRecvBuff, lLen);

    }
#else
    while (!pstClientInfo->bIsExit)
    {
        if (!pstClientInfo->bIsLogin)
        {
            pstClientInfo->ulLoginFailCount++;
            if (pstClientInfo->ulLoginFailCount > 3)
            {
                /* 密码输入错误三次 */
                break;
            }

            fprintf(pstClientInfo->pFDOutput, "\rUsername:");
            lLen = telnetd_client_read_line(pstClientInfo, szRecvBuff, sizeof(szRecvBuff), 0);
            if (lLen < 0)
            {
                logo_notice("Telnetd", "Telnet Exit", DOS_TRUE, "%s", "Telnet client exit.");
                break;
            }

            if (0 == lLen)
            {
                continue;
            }

            pcPassWord = (S8 *)dos_dmem_alloc(PTS_PASSWORD_SIZE);
            if (NULL == pcPassWord)
            {
                perror("dos_dmem_alloc");
                break;
            }
            dos_memzero(pcPassWord, PTS_PASSWORD_SIZE);
            lResult = pts_get_password(szRecvBuff, pcPassWord, PTS_PASSWORD_SIZE);
            if (lResult < 0)
            {
                pcPassWord[0] = '\0';
            }

            dos_strncpy(szUseName, szRecvBuff, MAX_RECV_BUFF_LENGTH);
            fprintf(pstClientInfo->pFDOutput, "\rPassword:");
            lLen = telnetd_client_read_line(pstClientInfo, szRecvBuff, sizeof(szRecvBuff), 1);
            if (lLen < 0)
            {
                logo_notice("Telnetd", "Telnet Exit", DOS_TRUE, "%s", "Telnet client exit\n");
                dos_dmem_free(pcPassWord);
                pcPassWord = NULL;
                break;
            }

            if (0 == lLen)
            {
                dos_dmem_free(pcPassWord);
                pcPassWord = NULL;
                continue;
            }

            pPassWordMd5 = pts_md5_encrypt(szUseName, szRecvBuff);
            if (dos_strcmp(pcPassWord, pPassWordMd5) == 0)
            {
                dos_dmem_free(pcPassWord);
                pcPassWord = NULL;
                pts_goAhead_free(pPassWordMd5);
                pPassWordMd5 = NULL;
                pstClientInfo->bIsLogin = DOS_TRUE;
                pstClientInfo->ulLoginFailCount = 0;
            }
            else
            {
                dos_dmem_free(pcPassWord);
                pcPassWord = NULL;
                pts_goAhead_free(pPassWordMd5);
                pPassWordMd5 = NULL;
                continue;
            }

        }
        else if (!pstClientInfo->bIsConnect)
        {
            /* 输出提示符 */
            telnetd_client_output_prompt(pstClientInfo->pFDOutput, pstClientInfo->ulMode);
            /* 读取命令行输入 */
            lLen = telnetd_client_read_line(pstClientInfo, szRecvBuff, sizeof(szRecvBuff), 0);
            if (lLen < 0)
            {
                logo_notice("Telnetd", "Telnet Exit", DOS_TRUE, "%s", "Telnet client exit.");
                break;
            }
            if (0 == lLen)
            {
                continue;
            }

            lResult = pts_server_cmd_analyse(pstClientInfo->ulIndex, pstClientInfo->ulMode, szRecvBuff, lLen);
            if (PT_TELNET_CONNECT == lResult)
            {
                /* connect ptc */
                pstClientInfo->bIsConnect = DOS_TRUE;
            }
            else if (PT_TELNET_EXIT == lResult)
            {
                pstClientInfo->bIsExit = DOS_TRUE;
            }
        }
        else
        {
            lLen = telnetd_client_read_char(pstClientInfo, &cRecvChar);
            if (lLen < 0)
            {
                logo_notice("Telnetd", "Telnet Exit", DOS_TRUE, "%s", "Telnet client exit.");
                /* 通知pts ptc释放资源 */
                pts_cmd_client_delete(pstClientInfo->ulIndex);
                break;
            }

            if (0 == lLen)
            {
                continue;
            }
            if (pstClientInfo->bIsConnect)
            {
                pts_telnet_send_msg2ptc(pstClientInfo->ulIndex, &cRecvChar, 1);
            }
        }
    }
#endif

    fclose(pstClientInfo->pFDOutput);
    fclose(pstClientInfo->pFDInput);
    close(pstClientInfo->lSocket);

    /* 重新初始化控制块 */
    pstClientInfo->pFDOutput = NULL;
    pstClientInfo->pFDInput = NULL;
    pstClientInfo->ulValid = DOS_FALSE;
    pstClientInfo->lSocket = -1;
#if INCLUDE_PTS
    pstClientInfo->bIsExit = DOS_FALSE;
    pstClientInfo->bIsLogin = DOS_FALSE;
    pstClientInfo->bIsConnect = DOS_FALSE;
    pstClientInfo->bIsGetLineEnd = DOS_FALSE;
    pstClientInfo->bIsRecvLFOrNUL = DOS_FALSE;
    pstClientInfo->stTimerHandle = NULL;
    pstClientInfo->ulLoginFailCount = 0;
#endif
    /* 如果所有客户端都关闭了，强制关闭控制台日志 */
    if (!telnetd_client_count())
    {
        snprintf(szCmd, sizeof(szCmd), "debug 0 ");
#if INCLUDE_DEBUG_CLI_SERVER
        cli_server_send_broadcast_cmd(INVALID_CLIENT_INDEX, szCmd, dos_strlen(szCmd) + 1);
#else
#endif
    }

    return NULL;
}

/**
 * 函数：VOID *telnetd_main_loop(VOID *ptr)
 * 功能：
 *      telnet服务器监听线程
 * 参数:
 * 返回值：
 */
VOID *telnetd_main_loop(VOID *ptr)
{
    struct sockaddr_storage stAddr;
    TELNET_CLIENT_INFO_ST   *pstClientInfo = NULL;
    struct timeval           stTimeout={1, 0};
    socklen_t                ulLen = 0;
    fd_set                   stFDSet;
    S32                      lConnectSock = -1;
    S32                      lRet = 0, lMaxFd = 0, i;

    pthread_mutex_lock(&g_TelnetdMutex);
    g_lTelnetdWaitingExit = 0;
    pthread_mutex_unlock(&g_TelnetdMutex);

    while(1)
    {
        pthread_mutex_lock(&g_TelnetdMutex);
        if (g_lTelnetdWaitingExit)
        {
            logr_info("%d", "Telnetd waiting exit flag has been set. exit");
            pthread_mutex_unlock(&g_TelnetdMutex);
            break;
        }
        pthread_mutex_unlock(&g_TelnetdMutex);

        FD_ZERO(&stFDSet);
        FD_SET(g_lTelnetdSrvSocket, &stFDSet);
        lMaxFd = g_lTelnetdSrvSocket + 1;
        stTimeout.tv_sec = 1;
        stTimeout.tv_usec = 0;

        lRet = select(lMaxFd, &stFDSet, NULL, NULL, &stTimeout);
        if (lRet < 0)
        {
            logr_warning("%s", "Error happened when select return.");
            lConnectSock = -1;
            DOS_ASSERT(0);
            break;
        }
        else if (0 == lRet)
        {
            continue;
        }

        ulLen = sizeof(stAddr);
        lConnectSock = accept(g_lTelnetdSrvSocket, (struct sockaddr *)&stAddr, &ulLen);
        if (lConnectSock < 0)
        {
            logr_warning("Telnet server accept connection fail.(%d)", errno);
            DOS_ASSERT(0);
            break;
        }

        logr_notice("%s", "New telnet connection.");

        pthread_mutex_lock(&g_TelnetdMutex);
        pstClientInfo = telnetd_client_alloc(lConnectSock);
        if (!pstClientInfo)
        {
            send(lConnectSock, "Too many user.\r\n", sizeof("Too many user.\r\n"), 0);
            close(lConnectSock);

            pthread_mutex_unlock(&g_TelnetdMutex);
            continue;
        }
        pthread_mutex_unlock(&g_TelnetdMutex);

        lRet = pthread_create(&pstClientInfo->pthClientTask, 0, telnetd_client_task, pstClientInfo);
        if (lRet < 0)
        {
            DOS_ASSERT(0);
            logr_warning("%s", "Create thread for telnet client fail.");
            close(lConnectSock);
            continue;
        }
    }

    /* 释放所有客户端 */
    for (i = 0; i < MAX_CLIENT_NUMBER; i++)
    {
        if (g_pstTelnetClientList[i]->ulValid)
        {
            pthread_cancel(g_pstTelnetClientList[i]->pthClientTask);

            if (g_pstTelnetClientList[i]->pFDInput)
            {
                fclose(g_pstTelnetClientList[i]->pFDInput);
                g_pstTelnetClientList[i]->pFDInput = NULL;
            }

            if (g_pstTelnetClientList[i]->pFDOutput)
            {
                fclose(g_pstTelnetClientList[i]->pFDOutput);
                g_pstTelnetClientList[i]->pFDOutput = NULL;
            }

            if (g_pstTelnetClientList[i]->lSocket)
            {
                close(g_pstTelnetClientList[i]->lSocket);
                g_pstTelnetClientList[i]->lSocket = -1;
            }
        }
    }

    if (g_lTelnetdSrvSocket > 0)
    {
        close(g_lTelnetdSrvSocket);
    }

    dos_printf("%s", "Telnetd exited.");

    return NULL;
}

/**
 * 函数：S32 telnetd_init()
 * 功能：
 *      初始化telnet服务器
 * 参数:
 * 返回值：
 */
S32 telnetd_init()
{
    S32 flag, i;
    struct sockaddr_in stListenAddr;
    TELNET_CLIENT_INFO_ST *pstClientCB;

    pstClientCB = (TELNET_CLIENT_INFO_ST *)dos_dmem_alloc(sizeof(TELNET_CLIENT_INFO_ST) * MAX_CLIENT_NUMBER);
    if (!pstClientCB)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero((VOID*)pstClientCB, sizeof(TELNET_CLIENT_INFO_ST) * MAX_CLIENT_NUMBER);
    for (i = 0; i < MAX_CLIENT_NUMBER; i++)
    {
        g_pstTelnetClientList[i] = pstClientCB + i;
        g_pstTelnetClientList[i]->ulValid = DOS_FALSE;
        g_pstTelnetClientList[i]->lSocket = -1;
        g_pstTelnetClientList[i]->ulIndex = i;
        g_pstTelnetClientList[i]->pFDInput = NULL;
        g_pstTelnetClientList[i]->pFDOutput = NULL;
        g_pstTelnetClientList[i]->ulMode = CLIENT_LEVEL_CONFIG;

#if INCLUDE_TELNET_PAGE
        DLL_Init(&g_pstTelnetClientList[i]->stCmdList);
#endif
    }

    /* We bind to port 23 before chrooting, as well. */
    g_lTelnetdSrvSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_lTelnetdSrvSocket < 0)
    {
        logr_warning("%s", "Create the telnetd server socket fail.");
        return DOS_FAIL;
    }
    flag = 1;
    setsockopt(g_lTelnetdSrvSocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    dos_memzero(&stListenAddr, sizeof(stListenAddr));
    stListenAddr.sin_family = AF_INET;
    stListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    #if INCLUDE_DEBUG_CLI_SERVER
    stListenAddr.sin_port = htons(TELNETD_LISTEN_PORT);
    #else
    stListenAddr.sin_port = dos_htons(config_get_pts_telnet_server_port());
    #endif
    if (bind(g_lTelnetdSrvSocket, (struct sockaddr *)&stListenAddr, sizeof(stListenAddr)) < 0)
    {
        perror("bind");
        logr_error("Telnet server bind IP/Address FAIL,errno:%u, cause:%s", errno, strerror(errno));
        close(g_lTelnetdSrvSocket);
        g_lTelnetdSrvSocket = -1;
        return DOS_FAIL;
    }

    if (listen(g_lTelnetdSrvSocket, 5) < 0)
    {
        logr_error("Linsten on port %d fail", TELNETD_LISTEN_PORT);
        close(g_lTelnetdSrvSocket);
        g_lTelnetdSrvSocket = -1;
        return DOS_FAIL;
    }

    return 0;
}

/**
 * 函数：S32 telnetd_start()
 * 功能：
 *      启动telnet服务器
 * 参数:
 * 返回值：
 */
S32 telnetd_start()
{
    S32 lRet = 0;

    lRet = pthread_create(&g_pthTelnetdTask, 0, telnetd_main_loop, NULL);
    if (lRet < 0)
    {
        logr_warning("%s", "Create thread for telnetd fail.");
        close(g_lTelnetdSrvSocket);
        g_lTelnetdSrvSocket = -1;
        return DOS_FAIL;
    }

    dos_printf("%s", "telnet server start!");

    //pthread_join(g_pthTelnetdTask, NULL);

    return DOS_SUCC;
}

/**
 * 函数：S32 telnetd_stop()
 * 功能：
 *      停止telnet服务器
 * 参数:
 * 返回值：
 */
S32 telnetd_stop()
{
    pthread_mutex_lock(&g_TelnetdMutex);
    g_lTelnetdWaitingExit = 1;
    pthread_mutex_unlock(&g_TelnetdMutex);

    pthread_join(g_pthTelnetdTask, NULL);

    return DOS_SUCC;
}

#else

S32 telnet_out_string(U32 ulIndex, S8 *pszBuffer)
{
    dos_printf("%s", pszBuffer);

    return 0;
}
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

