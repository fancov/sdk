#ifdef  __cplusplus
extern "C" {
#endif

/* include sys header files */

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <limits.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/time.h>
#include <semaphore.h>
#include <signal.h>

/* include the dos header files */
#include <pt/dos_sqlite3.h>
#include <pt/md5.h>
#include <dos.h>
#include <pt/pts.h>

/* include provate header file */
#include "pts_msg.h"
#include "pts_web.h"
#include "pts_telnet.h"
#include "pts_goahead.h"
#include "../telnetd/telnetd.h"


/* 用户数据库 */
#define PTS_CREATE_USER_DB "CREATE TABLE pts_user (\
name  varchar(100) NOT NULL,\
password  varchar(33) NOT NULL,\
userName  varchar(50),\
fixedTel  varchar(20),\
mobile  varchar(12),\
mailbox  varchar(50),\
regiestTime TimeStamp NOT NULL DEFAULT (datetime('now','localtime')),\
PRIMARY KEY (\"name\" ASC))"



/* PTC数据库 */
#define PTS_CREATE_PTC_DB "create table ipcc_alias(\
id  INTEGER NOT NULL,\
sn  varchar(20) NOT NULL ON CONFLICT FAIL,\
name  varchar(64),\
remark  varchar(64),\
version  varchar(10) NOT NULL,\
register  integer,\
lastLoginTime  TimeStamp NOT NULL DEFAULT (datetime('now','localtime')),\
domain  INTEGER,\
intranetIP  varchar(16),\
internetIP  varchar(16),\
intranetPort  INTEGER,\
internetPort  INTEGER,\
ptcType varchar(16),\
achPtsMajorDomain varchar(64),\
achPtsMinorDomain varchar(64),\
usPtsMajorPort integer,\
usPtsMinorPort integer,\
szPtsHistoryIp1 varchar(16),\
szPtsHistoryIp2 varchar(16),\
szPtsHistoryIp3 varchar(16),\
usPtsHistoryPort1 integer,\
usPtsHistoryPort2 integer,\
usPtsHistoryPort3 integer,\
szMac varchar(64),\
heartbeatTime INTEGER,\
PRIMARY KEY (\"id\" ASC) ON CONFLICT REPLACE,\
UNIQUE (\"id\" COLLATE RTRIM ASC, \"sn\" ASC))"

/* 存放正在进行的ping操作的ptc的结果 */
#define PTS_CREATE_PING_RESULT_DB "create table ping_result(\
id  INTEGER NOT NULL,\
sn  varchar(20) NOT NULL,\
curr_position integer NOT NULL,\
timer0 integer,\
timer1 integer,\
timer2 integer,\
timer3 integer,\
timer4 integer,\
timer5 integer,\
timer6 integer,\
timer7 integer,\
PRIMARY KEY (\"id\" ASC) ON CONFLICT REPLACE,\
UNIQUE (\"id\" COLLATE RTRIM ASC))"

DOS_SQLITE_ST  *g_pstMySqlite = NULL;              /* 数据库sqlite3结构体 */

/**
 * 函数：U32 pts_create_stream_id()
 * 功能：
 *      1.获得stream ID
 * 参数
 * 返回值：
 */
U32 pts_create_stream_id()
{
    static U32 ulStreamId = PT_CTRL_BUTT;
    ulStreamId++;
    if (0 == ulStreamId)
    {
        ulStreamId = PT_CTRL_BUTT;
    }
    return ulStreamId;
}

/**
 * 函数：U32 pts_create_udp_socket(U16 usUdpPort)
 * 功能：
 *      1.创建udp 服务器端套接字
 * 参数
 * 返回值：
 */
U32 pts_create_udp_socket(U16 usUdpPort)
{
    S32 lSockfd = 0;
    S32 lError  = 0;
    struct sockaddr_in stMyAddr;

    lSockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (lSockfd < 0)
    {
        perror("udp create socket");
        exit(DOS_FAIL);
    }

    dos_memzero(&stMyAddr, sizeof(stMyAddr));
    stMyAddr.sin_family = AF_INET;
    stMyAddr.sin_port = dos_htons(usUdpPort);
    stMyAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    pt_logr_debug("udp channel server port: %d", usUdpPort);
    lError = bind(lSockfd, (struct sockaddr*)&stMyAddr, sizeof(stMyAddr));
    if (lError != 0)
    {
        perror("udp server bind");
        close(lSockfd);
        exit(DOS_FAIL);
    }

    return lSockfd;
}

/**
 * 函数：U32 pts_create_tcp_socket(U16 usTcpPort)
 * 功能：
 *      1.创建tcp服务器端套接字
 * 参数
 * 返回值：
 */
U32 pts_create_tcp_socket(U16 usTcpPort)
{
    S32 lSockfd = 0;
    S32 lError  = 0;
    socklen_t addrlen;
    struct sockaddr_in stMyAddr;

    pt_logr_debug("create proxy socket, port is %d\n", usTcpPort);
    dos_memzero(&stMyAddr, sizeof(stMyAddr));
    stMyAddr.sin_family = AF_INET;
    stMyAddr.sin_port = dos_htons(usTcpPort);
    stMyAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    lSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lSockfd < 0)
    {
        perror("create tcp socket");
        exit(DOS_FAIL);
    }

    /* 设置端口快速重用 */
    addrlen = 1;
    setsockopt(lSockfd, SOL_SOCKET, SO_REUSEADDR, &addrlen, sizeof(addrlen));

    lError = bind(lSockfd, (struct sockaddr*)&stMyAddr, sizeof(stMyAddr));
    if (lError != 0)
    {
        perror("web service bind");
        close(lSockfd);
        return DOS_FAIL;
    }

    lError = listen(lSockfd, PTS_LISTEN_COUNT);
    if (lError != 0)
    {
        perror("web service listen");
        close(lSockfd);
        return DOS_FAIL;
    }

    return lSockfd;
}

/**
 * 函数：S32 pts_get_password_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
 * 功能：
 *      1.从sqlite3数据库获得密码的回调函数
 * 参数
 * 返回值：
 */
S32 pts_get_password_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
{
    S8 *szPassWord = (S8 *)para;
    dos_strncpy(szPassWord, column_value[0], PT_DATA_BUFF_128);

    return DOS_SUCC;
}

/**
 * 函数：VOID pts_handle_ctrl_msg(PT_NEND_RECV_NODE_ST *pstNeedRevNode)
 * 功能：
 *      1.处理控制消息
 * 参数
 * 返回值：
 */
VOID pts_handle_ctrl_msg(PT_NEND_RECV_NODE_ST *pstNeedRevNode)
{
    U32              ulArraySub            = 0;
    list_t           *pstStreamList        = NULL;
    PT_CC_CB_ST      *pstCCNode            = NULL;
    PT_STREAM_CB_ST  *pstStreamNode        = NULL;
    PT_DATA_TCP_ST   *pstDataTcp           = NULL;
    PT_CTRL_DATA_ST  *pstCtrlData          = NULL;
    S8 szSql[PT_DATA_BUFF_128] = {0};
    U8 szID[PTC_ID_LEN+1] = {0};
    S32 lResult = 0;

    if (PT_CTRL_LOGOUT == pstNeedRevNode->ulStreamID)
    {
        /* 退出登陆/掉线 */
        dos_snprintf(szSql, PT_DATA_BUFF_128, "update ipcc_alias set register=0 where sn='%s';", pstNeedRevNode->aucID);
        lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
        {
            DOS_ASSERT(0);
        }
        return;
    }

    pstCCNode = pt_ptc_list_search(g_pstPtcListRecv, pstNeedRevNode->aucID);
    if(NULL == pstCCNode)
    {
        pt_logr_debug("pts send msg to proxy : not found ptc id is %s", pstNeedRevNode->aucID);
        return;
    }

    pstStreamList = pstCCNode->astDataTypes[pstNeedRevNode->enDataType].pstStreamQueHead;
    if (NULL == pstStreamList)
    {
        pt_logr_debug("pts send msg to proxy : not found stream list of type is %d", pstNeedRevNode->enDataType);
        return;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamList, pstNeedRevNode->ulStreamID);
    if (NULL == pstStreamNode)
    {
        pt_logr_debug("pts send msg to proxy : not found stream : %d", pstNeedRevNode->ulStreamID);
        return;
    }

    if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
    {
        pt_logr_debug("pts send msg to proxy : not found data queue");
        return;
    }

    while(1)
    {
        pstStreamNode->lCurrSeq++;
        ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1);
        pstDataTcp = pstStreamNode->unDataQueHead.pstDataTcp;

        if (pstDataTcp[ulArraySub].lSeq == pstStreamNode->lCurrSeq)
        {
            pstCtrlData = (PT_CTRL_DATA_ST *)(pstDataTcp[ulArraySub].szBuff);
            switch (pstCtrlData->enCtrlType)
            {
            case PT_CTRL_LOGIN_ACK:
                /* 登陆成功 */
                dos_snprintf(szSql, PT_DATA_BUFF_128, "select * from ipcc_alias where sn='%s'", pstNeedRevNode->aucID);
                lResult = dos_sqlite3_record_is_exist(g_pstMySqlite, szSql);
                if (lResult < 0)
                {
                    DOS_ASSERT(0);
                }
                else if (lResult)  /* 判断是否存在 */
                {
                    /* 存在，更新IPCC的注册状态 */
                    pt_logr_debug("pts_send_msg2client : db existed");
                    dos_snprintf(szSql, PT_DATA_BUFF_128, "update ipcc_alias set register=1, name='%s', version='%s' where sn='%s';", pstCtrlData->szPtcName, pstCtrlData->szVersion, pstNeedRevNode->aucID);

                    lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
                    if (lResult != DOS_SUCC)
                    {
                        DOS_ASSERT(0);
                    }
                }
                else
                {
                    /* 不存在，添加IPCC到DB */
                    pt_logr_debug("pts_send_msg2client : db insert");
                    dos_memcpy(szID, pstNeedRevNode->aucID, PTC_ID_LEN);
                    szID[PTC_ID_LEN] = '\0';
                    dos_snprintf(szSql, PT_DATA_BUFF_128, "INSERT INTO ipcc_alias (\"id\", \"sn\", \"name\", \"remark\", \"version\", \"register\", \"lastLoginTime\", \"domain\", \"intranetIP\", \"internetIP\", \"intranetPort\", \"internetPort\") VALUES (NULL, '%s', '%s', NULL, '%s', %d, 3123, NULL, NULL, NULL, NULL, NULL);", szID, pstCtrlData->szPtcName, pstCtrlData->szVersion, DOS_TRUE);
                    lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
                    if (lResult != DOS_SUCC)
                    {
                        DOS_ASSERT(0);
                    }
                }
                break;
            default:
                break;
            }
        }
        else
        {
            pstStreamNode->lCurrSeq--;
            break;
        }
    }

    return;
}

/**
 * 函数：VOID *pts_send_msg2proxy(VOID *arg)
 * 功能：
 *      1.线程  发送消息到proxy
 * 参数
 * 返回值：
 */
VOID *pts_send_msg2proxy(VOID *arg)
{
    list_t  *pstNendRecvList = NULL;
    PT_NEND_RECV_NODE_ST *pstNeedRevNode = NULL;
    struct timeval now;
    struct timespec timeout;
    S32 lLoopMaxCount = 0;
    //signal(SIGPIPE, SIG_IGN);

    while (1)
    {
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = now.tv_usec * 1000;
#if PT_MUTEX_DEBUG
        pts_recv_pthread_mutex_lock(__FILE__, __LINE__);
#else
        pthread_mutex_lock(&g_pts_mutex_recv);
#endif
        sem_post(&g_SemPtsRecv);
#if PT_MUTEX_DEBUG
        pts_recv_pthread_cond_timedwait(&timeout, __FILE__, __LINE__);
#else
        pthread_cond_timedwait(&g_pts_cond_recv, &g_pts_mutex_recv, &timeout);
#endif

        /* 循环发送g_pstPtsNendRecvNode中的stream */
        pstNendRecvList = g_pstPtsNendRecvNode;
        lLoopMaxCount = 0;
        DOS_LOOP_DETECT_INIT(lLoopMaxCount, DOS_DEFAULT_MAX_LOOP);

        while(1)
        {
            /* 防止死循环 */
            DOS_LOOP_DETECT(lLoopMaxCount)
            if (NULL == pstNendRecvList)
            {
                break;
            }
            pstNeedRevNode = dos_list_entry(pstNendRecvList, PT_NEND_RECV_NODE_ST, stListNode);
            if (pstNendRecvList == pstNendRecvList->next)
            {
                pstNendRecvList = NULL;
            }
            else
            {
                pstNendRecvList = pstNendRecvList->next;
                dos_list_del(&pstNeedRevNode->stListNode);
            }

            /* if (pstNeedRevNode->enDataType == PT_DATA_CTRL)
             {
                 pts_handle_ctrl_msg(pstNeedRevNode);
                 dos_dmem_free(pstNeedRevNode);
                 pstNeedRevNode = NULL;
                 continue;

             }
             else */
            if (pstNeedRevNode->enDataType == PT_DATA_WEB)
            {
                /* web */
                pts_send_msg2web(pstNeedRevNode);
                dos_dmem_free(pstNeedRevNode);
                pstNeedRevNode = NULL;
            }
            else if (pstNeedRevNode->enDataType == PT_DATA_CMD)
            {
                /* cmd */
                pts_send_msg2cmd(pstNeedRevNode);
                dos_dmem_free(pstNeedRevNode);
                pstNeedRevNode = NULL;
            }
            else
            {
                dos_dmem_free(pstNeedRevNode);
                pstNeedRevNode = NULL;
            }
        } /* end of while(1) */
        g_pstPtsNendRecvNode = pstNendRecvList;
#if PT_MUTEX_DEBUG
        pts_recv_pthread_mutex_unlock(__FILE__, __LINE__);
#else
        pthread_mutex_unlock(&g_pts_mutex_recv);
#endif

    } /* end of while(1) */

}

/**
 * 函数：VOID *pts_send_msg2proxy(VOID *arg)
 * 功能：
 *      1.初始化  g_stPtsMsg
 * 参数
 * 返回值：
 */
VOID pts_init_serv_msg()
{
    S8 szServiceRoot[PT_DATA_BUFF_256] = {0};

    g_stPtsMsg.usPtsPort = config_get_pts_port();
    g_stPtsMsg.usWebServPort = config_get_pts_proxy_port(); /* web server 端口 */
    g_stPtsMsg.usCmdServPort = config_get_pts_telnet_server_port(); /* cmd server 端口 */
    /* 数据库目录 */

    if (config_get_service_root(szServiceRoot, sizeof(szServiceRoot)) == NULL)
    {
        exit(-1);
    }

    g_pstMySqlite = (DOS_SQLITE_ST *)dos_dmem_alloc(sizeof(DOS_SQLITE_ST));
    if (NULL == g_pstMySqlite)
    {
        logr_error("malloc fail");
        exit(-1);
    }

    snprintf(g_pstMySqlite->pchDbPath, SQLITE_DB_PATH_LEN, "%s/%s", szServiceRoot, PTS_SQLITE_DB_PATH);

    logr_debug("db path : %s", g_pstMySqlite->pchDbPath);
    return;
}

/**
 * 函数：S32 pts_main()
 * 功能：
 *      1.pts main函数
 * 参数
 * 返回值：
 */
S32 pts_main()
{
    S32 lSocket, lRet;
    pthread_t tid1, tid2, tid3, tid4, tid5;
    S8 szSql[PT_DATA_BUFF_128] = {0};
    U32 ulSocketCache = PTS_SOCKET_CACHE;    /* socket 收发缓存 */
    S8 *pPassWordMd5 = NULL;

    pts_init_serv_msg();
    lSocket = pts_create_udp_socket(g_stPtsMsg.usPtsPort);

    lRet = pts_create_tcp_socket(g_stPtsMsg.usPtsPort);
    if (lRet <= 0)
    {
        exit(DOS_FAIL);
    }

    lRet = setsockopt(lSocket, SOL_SOCKET, SO_SNDBUF, (char*)&ulSocketCache, sizeof(ulSocketCache));
    if (lRet != 0)
    {
        logr_error("setsockopt error : %d", lRet);
        return DOS_FAIL;
    }

    lRet = setsockopt(lSocket, SOL_SOCKET, SO_RCVBUF, (char*)&ulSocketCache, sizeof(ulSocketCache));
    if (lRet != 0)
    {
        logr_error("setsockopt error : %d", lRet);
        return DOS_FAIL;
    }

    lRet = dos_sqlite3_create_db(g_pstMySqlite);
    if (lRet < 0)
    {
        return DOS_FAIL;
    }

    lRet = dos_sqlite3_creat_table(g_pstMySqlite, PTS_CREATE_PTC_DB);
    if (PT_SLITE3_FAIL == lRet)
    {
        return DOS_FAIL;
    }

    lRet = dos_sqlite3_creat_table(g_pstMySqlite, PTS_CREATE_PING_RESULT_DB);
    if (PT_SLITE3_FAIL == lRet)
    {
        return DOS_FAIL;
    }
    /* 清空 ping_result 中数据 */
    dos_snprintf(szSql, PT_DATA_BUFF_128, "delete from ping_result;");
    dos_sqlite3_exec(g_pstMySqlite, szSql);

    lRet = dos_sqlite3_creat_table(g_pstMySqlite, PTS_CREATE_USER_DB);
    if (PT_SLITE3_FAIL == lRet)
    {
        return DOS_FAIL;
    }
    else if (PT_SLITE3_SUCC == lRet)
    {
        /* 创建默认账户 */
        pPassWordMd5 = pts_md5_encrypt("admin", "admin");
        dos_snprintf(szSql, PT_DATA_BUFF_128, "INSERT INTO pts_user (\"name\", \"password\") VALUES ('admin', '%s');", pPassWordMd5);
        lRet = dos_sqlite3_exec(g_pstMySqlite, szSql);
        if (lRet != DOS_SUCC)
        {
            DOS_ASSERT(0);
        }

        pts_goAhead_free(pPassWordMd5);
        pPassWordMd5 = NULL;
    }

    dos_snprintf(szSql, PT_DATA_BUFF_128, "update ipcc_alias set register=0 where register=1;");
    lRet = dos_sqlite3_exec(g_pstMySqlite, szSql);
    if (lRet != DOS_SUCC)
    {
        DOS_ASSERT(0);
    }

    lRet = pthread_create(&tid1, NULL, pts_send_msg2ptc, (VOID *)&lSocket);
    if (lRet < 0)
    {
        logr_info("create pthread error : pts_send_msg2ptc!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("create pthread succ : pts_send_msg2ptc!");
    }

    lRet = pthread_create(&tid2, NULL, pts_recv_msg_from_ptc, (VOID *)&lSocket);
    if (lRet < 0)
    {
        logr_info("create pthread error : pts_recv_msg_from_ptc!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("create pthread succ : pts_recv_msg_from_ptc!");
    }

    lRet = pthread_create(&tid3, NULL, pts_recv_msg_from_web, NULL);
    if (lRet < 0)
    {
        logr_info("create pthread error : pts_recv_msg_from_web!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("create pthread succ : pts_recv_msg_from_web!");
    }

    lRet = pthread_create(&tid4, NULL, pts_send_msg2proxy, NULL);
    if (lRet < 0)
    {
        logr_info("create pthread error : pts_send_msg2proxy!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("create pthread succ : pts_send_msg2proxy!");
    }

    lRet = pthread_create(&tid5, NULL, pts_goahead_service, NULL);
    if (lRet < 0)
    {
        logr_info("create pthread error : pts_goahead_service!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("create pthread succ : pts_goahead_service!");
    }

    lRet = telnetd_start();
    if (lRet < 0)
    {
        logr_info("telnetd_start fail!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("telnetd_start succ!");
    }

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    pthread_join(tid4, NULL);
    pthread_join(tid5, NULL);
    telnetd_stop();

    lRet = dos_sqlite3_close(g_pstMySqlite);
    if (lRet)
    {
        dos_dmem_free(g_pstMySqlite);
        g_pstMySqlite = NULL;

        return DOS_FAIL;
    }
    dos_dmem_free(g_pstMySqlite);
    g_pstMySqlite = NULL;

    return DOS_SUCC;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */


