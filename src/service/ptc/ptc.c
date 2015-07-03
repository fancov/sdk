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
#include <net/if.h>
#include <sys/ioctl.h>
/* include the dos header files */
#include <dos.h>
#include <pt/ptc.h>
#include <pt/dos_sqlite3.h>
/* include provate header file */
#include "ptc_msg.h"
#include "ptc_web.h"
#include "ptc_telnet.h"
#include "../telnetd/telnetd.h"

U32 g_ulGetPtsHistoryCount = 0;

/**
 * 函数：S32 getlocalip(S8 *szLocalIp, S32 lSockfd)
 * 功能：
 *      1.获取本机IP
 * 参数
 *      VOID *arg :通道通信的sockfd
 * 返回值：void *
 */
S32 ptc_get_ifr_name_by_ip(S8 *szIp, S32 lSockfd, S8 *szName, U32 ulNameLen)
{
    S32 i = 0;
    S8 *ip = NULL;
    S8 buf[PT_DATA_BUFF_512];
    struct ifreq *ifreq;
    struct ifconf ifconf;

    /* 初始化ifconf */
    ifconf.ifc_len = PT_DATA_BUFF_512;
    ifconf.ifc_buf = buf;

    /* 获取所有接口信息 */
    if (ioctl(lSockfd, SIOCGIFCONF, &ifconf) < 0)
    {
        perror("ioctl");
        return DOS_FAIL;
    }

    /* 接下来一个一个的获取IP地址 */
    ifreq = (struct ifreq*)buf;
    i = ifconf.ifc_len/sizeof(struct ifreq);
    for (; i>0; i--)
    {
        ip = inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr);

        if(dos_strcmp(ip, szIp) == 0)
        {
            dos_strncpy(szName, ifreq->ifr_name, ulNameLen);
            return DOS_SUCC;
        }
        ifreq++;
    }

    return DOS_FAIL;
}

S32 ptc_get_mac(S8 *szName, S32 lSockfd, S8 *szMac, U32 ulMacLen)
{
    struct ifreq stIfreq;
    U8 *pTr = NULL;

    dos_strcpy(stIfreq.ifr_name, szName);

    if (ioctl(lSockfd, SIOCGIFHWADDR, &stIfreq) < 0)
    {
        perror("ioctl");
        return DOS_FAIL;
    }

    pTr = (U8 *)&stIfreq.ifr_ifru.ifru_hwaddr.sa_data[0];
    snprintf(szMac, ulMacLen, "%02X-%02X-%02X-%02X-%02X-%02X",*pTr,*(pTr+1),*(pTr+2),*(pTr+3),*(pTr+4),*(pTr+5));

    return DOS_SUCC;
}

S32 ptc_get_pts_domain_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
{
    dos_strncpy(para, column_value[0], PT_DATA_BUFF_128);
    return DOS_SUCC;
}

VOID ptc_get_pts_history()
{
    S32 lResult = 0;
    S32 i = 0;
    FILE *pFileHandle = NULL;
    S8 szPtsHistory[4][PT_DATA_BUFF_64];
    S32 lPtsHistoryConut = 0;
    S8 szPtsIP[PT_IP_ADDR_SIZE] = {0};
    S8 szPtsPort[PT_DATA_BUFF_16] = {0};

    pFileHandle = fopen(g_stServMsg.szHistoryPath, "ab+");
    if (NULL == pFileHandle)
    {
        return;
    }

    while (!feof(pFileHandle) && lPtsHistoryConut < 4)
    {
        fgets(szPtsHistory[lPtsHistoryConut], PT_DATA_BUFF_64, pFileHandle);
        lPtsHistoryConut++;
    }
    fclose(pFileHandle);
    lPtsHistoryConut--;

    for (i=1; i<=lPtsHistoryConut; i++)
    {
        lResult = sscanf(szPtsHistory[i-1], "%[^&]&%s", szPtsIP, szPtsPort);
        if (2 == lResult)
        {
            if (1 == i)
            {
                //inet_pton(AF_INET, szPtsIP, (VOID *)(g_stServMsg.achPtsLasterIP1));
                dos_strncpy(g_stServMsg.szPtsLasterIP1, szPtsIP, PT_IP_ADDR_SIZE);
                g_stServMsg.usPtsLasterPort1 = dos_htons(atoi(szPtsPort));
            }
            else if (2 == i)
            {
                //inet_pton(AF_INET, szPtsIP, (VOID *)(g_stServMsg.achPtsLasterIP2));
                dos_strncpy(g_stServMsg.szPtsLasterIP2, szPtsIP, PT_IP_ADDR_SIZE);
                g_stServMsg.usPtsLasterPort2 = dos_htons(atoi(szPtsPort));
            }
            else
            {
                //inet_pton(AF_INET, szPtsIP, (VOID *)(g_stServMsg.achPtsLasterIP3));
                dos_strncpy(g_stServMsg.szPtsLasterIP3, szPtsIP, PT_IP_ADDR_SIZE);
                g_stServMsg.usPtsLasterPort3 = dos_htons(atoi(szPtsPort));
            }
        }
    }

}

/**
 * 函数：VOID ptc_init_serv_msg(S32 lSockfd)
 * 功能：
 *      1.初始化 g_stServMsg
 * 参数
 * 返回值：
 */
S32 ptc_read_configure()
{
    S32 lResult = 0;
    U8 paucIPAddr[IPV6_SIZE] = {0};
    S8 szPtsIp[PT_IP_ADDR_SIZE] = {0};
    S8 szMajorDoMain[PTC_PTS_DOMAIN_SIZE] = {0}; /* 主域名 */
    S8 szMinorDoMain[PTC_PTS_DOMAIN_SIZE] = {0}; /* 副域名 */
    S8 szServiceRoot[PT_DATA_BUFF_256] = {0};
    BOOL bIsGetMajorDomain = DOS_FALSE;
    BOOL bIsGetMinorDomain = DOS_FALSE;

    if (config_get_service_root(szServiceRoot, sizeof(szServiceRoot)) == NULL)
    {
        return DOS_FAIL;
    }

get_domain:
    /* 获取主域名*/
    lResult = config_get_pts_major_domain(szMajorDoMain, PTC_PTS_DOMAIN_SIZE);
    if (lResult != DOS_SUCC)
    {
        logr_info("get domain fail");
        bIsGetMajorDomain = DOS_FALSE;
    }
    else
    {
        bIsGetMajorDomain = DOS_TRUE;
    }

    /* 获取备域名*/
    lResult = config_get_pts_minor_domain(szMinorDoMain, PTC_PTS_DOMAIN_SIZE);
    if (lResult != DOS_SUCC)
    {
        logr_info("get domain fail");
        bIsGetMinorDomain = DOS_FALSE;
    }
    else
    {
        bIsGetMinorDomain = DOS_TRUE;
    }

    logr_debug("major domain name is : %s, minor domain is : %s", szMajorDoMain, szMinorDoMain);
    /* 保存主/副域名 */
    if (bIsGetMajorDomain)
    {
        dos_strncpy(g_stServMsg.achPtsMajorDomain, szMajorDoMain, PT_DATA_BUFF_64-1);
    }

    if (bIsGetMinorDomain)
    {
        dos_strncpy(g_stServMsg.achPtsMinorDomain, szMinorDoMain, PT_DATA_BUFF_64-1);
    }

    /* 获得主域名 ip */
    if (bIsGetMajorDomain)
    {
        if (pt_is_or_not_ip(szMajorDoMain))
        {
            dos_strncpy(szPtsIp, szMajorDoMain, PT_IP_ADDR_SIZE);
            bIsGetMajorDomain = DOS_TRUE;
        }
        else
        {
            lResult = pt_DNS_analyze(szMajorDoMain, paucIPAddr);
            if (lResult <= 0)
            {
                logr_info("major domain DNS fail");
                bIsGetMajorDomain = DOS_FALSE;
            }
            else
            {
                inet_ntop(AF_INET, (void *)(paucIPAddr), szPtsIp, PT_IP_ADDR_SIZE);
                logr_debug("domain name is : %s, ip : %s", szMajorDoMain, szPtsIp);
                bIsGetMajorDomain = DOS_TRUE;
            }
        }

        inet_pton(AF_INET, szPtsIp, (VOID *)(g_stServMsg.achPtsMajorIP));
    }

    /* 获得副域名 ip */
    if (bIsGetMinorDomain)
    {
        if (pt_is_or_not_ip(szMinorDoMain))
        {
            dos_strncpy(szPtsIp, szMinorDoMain, PT_IP_ADDR_SIZE);
            bIsGetMinorDomain = DOS_TRUE;
        }
        else
        {
            lResult = pt_DNS_analyze(szMinorDoMain, paucIPAddr);
            if (lResult <= 0)
            {
                logr_info("mainor domain DNS fail");
                bIsGetMinorDomain = DOS_FALSE;
            }
            else
            {
                inet_ntop(AF_INET, (void *)(paucIPAddr), szPtsIp, PT_IP_ADDR_SIZE);
                logr_debug("domain name is : %s, ip : %s", szMinorDoMain, szPtsIp);
                bIsGetMinorDomain = DOS_TRUE;
            }
        }

        inet_pton(AF_INET, szPtsIp, (VOID *)(g_stServMsg.achPtsMinorIP));
    }

    if (DOS_FALSE == bIsGetMajorDomain && DOS_FALSE == bIsGetMinorDomain)
    {
        sleep(2);
        goto get_domain;
    }

    /* 获得主/副域名的端口 */
    if (bIsGetMajorDomain)
    {
        g_stServMsg.usPtsMajorPort = dos_htons(config_get_pts_major_port());
    }
    else
    {
        g_stServMsg.usPtsMajorPort = 0;
    }

    if (bIsGetMinorDomain)
    {
        g_stServMsg.usPtsMinorPort = dos_htons(config_get_pts_minor_port());
    }
    else
    {
        g_stServMsg.usPtsMinorPort = 0;
    }

    g_stServMsg.usLocalPort = 0;

    lResult = pt_DNS_analyze(PTC_GET_LOCAL_IP_SERVER_DOMAIN, paucIPAddr);
    if (lResult <= 0)
    {
        dos_strcpy(szPtsIp, PTC_GET_LOCAL_IP_SERVER_IP);
    }
    else
    {
        inet_ntop(AF_INET, (void *)(paucIPAddr), szPtsIp, PT_IP_ADDR_SIZE);
        logr_debug("server is : %s, ip : %s", PTC_GET_LOCAL_IP_SERVER_DOMAIN, szPtsIp);
    }

    inet_pton(AF_INET, szPtsIp, (VOID *)(g_stServMsg.achBaiduIP));
    g_stServMsg.usBaiduPort = dos_htons(PTC_GET_LOCAL_IP_SERVER_PORT);

    /* 记录pts历史记录的文件的路径 */
    dos_snprintf(g_stServMsg.szHistoryPath, PT_DATA_BUFF_128, "%s/%s", szServiceRoot, PTC_HISTORY_PATH);
    /* 获得存放ptc注册pts的记录表 */
    ptc_get_pts_history();

    return DOS_SUCC;
}

/**
 * 函数：VOID ptc_init_send_cache_queue(S32 lSocket, U8 *pcID)
 * 功能：
 *      1.初始化发送缓存ptc节点
 * 参数
 * 返回值：
 */
S32 ptc_init_send_cache_queue(U8 *pcID)
{
    S32 i = 0;
    struct sockaddr_in stDestAddr;
    PT_DATA_TCP_ST *pstTcpData = NULL;
    PT_STREAM_CB_ST *pstStreamIDHead = NULL;
    PT_STREAM_CB_ST *pstStreamIDNode = NULL;

    if (DOS_ADDR_INVALID(pcID))
    {
        return DOS_FAIL;
    }

    /* 预分配 PTC_STREAMID_MAX_COUNT 个 stream的空间 */
    pstStreamIDHead = (PT_STREAM_CB_ST *)dos_dmem_alloc(sizeof(PT_STREAM_CB_ST) * PTC_STREAMID_MAX_COUNT);
    if (DOS_ADDR_INVALID(pstStreamIDHead))
    {
        return DOS_FAIL;
    }

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        pstStreamIDNode = &pstStreamIDHead[i];
        pt_stream_node_init(pstStreamIDNode);
        pstTcpData = pt_data_tcp_queue_create(PT_DATA_SEND_CACHE_SIZE);
        if (DOS_ADDR_INVALID(pstTcpData))
        {
            dos_dmem_free(pstStreamIDHead);
            return DOS_FAIL;
        }
        pstStreamIDNode->unDataQueHead.pstDataTcp = pstTcpData;
    }

    g_pstPtcSend = (PT_CC_CB_ST *)dos_dmem_alloc(sizeof(PT_CC_CB_ST));
    if (DOS_ADDR_INVALID(g_pstPtcSend))
    {
        perror("send cache malloc");
        return DOS_FAIL;
    }

    dos_memcpy(g_pstPtcSend->aucID, pcID, PTC_ID_LEN);

    bzero(&stDestAddr, sizeof(stDestAddr));
    stDestAddr.sin_family = AF_INET;

    if (g_stServMsg.usPtsMajorPort != 0)
    {
        stDestAddr.sin_port = g_stServMsg.usPtsMajorPort;
        stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMajorIP);
    }
    else if (g_stServMsg.usPtsMinorPort != 0)
    {
        stDestAddr.sin_port = g_stServMsg.usPtsMinorPort;
        stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMinorIP);
    }
    else
    {
        stDestAddr.sin_port = 0;
        stDestAddr.sin_addr.s_addr = 0;
    }

    g_pstPtcSend->stDestAddr = stDestAddr;
    g_pstPtcSend->pstStreamHead = pstStreamIDHead;

    return DOS_SUCC;
}

/**
 * 函数：VOID ptc_init_recv_cache_queue(S32 lSocket, U8 *pcID)
 * 功能：
 *      1.初始化接收缓存ptc节点
 * 参数
 * 返回值：
 */
S32 ptc_init_recv_cache_queue(U8 *pcID)
{
   S32 i = 0;
    struct sockaddr_in stDestAddr;
    PT_DATA_TCP_ST *pstTcpData = NULL;
    PT_STREAM_CB_ST *pstStreamIDHead = NULL;
    PT_STREAM_CB_ST *pstStreamIDNode = NULL;

    if (DOS_ADDR_INVALID(pcID))
    {
        return DOS_FAIL;
    }

    /* 预分配 PTC_STREAMID_MAX_COUNT 个 stream的空间 */
    pstStreamIDHead = (PT_STREAM_CB_ST *)dos_dmem_alloc(sizeof(PT_STREAM_CB_ST) * PTC_STREAMID_MAX_COUNT);
    if (DOS_ADDR_INVALID(pstStreamIDHead))
    {
        return DOS_FAIL;
    }

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        pstStreamIDNode = &pstStreamIDHead[i];
        pt_stream_node_init(pstStreamIDNode);
        pstTcpData = pt_data_tcp_queue_create(PT_DATA_RECV_CACHE_SIZE);
        if (DOS_ADDR_INVALID(pstTcpData))
        {
            dos_dmem_free(pstStreamIDHead);
            return DOS_FAIL;
        }
        pstStreamIDNode->unDataQueHead.pstDataTcp = pstTcpData;
    }

    g_pstPtcRecv = (PT_CC_CB_ST *)dos_dmem_alloc(sizeof(PT_CC_CB_ST));
    if (DOS_ADDR_INVALID(g_pstPtcRecv))
    {
        perror("send cache malloc");
        return DOS_FAIL;
    }

    dos_memcpy(g_pstPtcRecv->aucID, pcID, PTC_ID_LEN);

    bzero(&stDestAddr, sizeof(stDestAddr));
    stDestAddr.sin_family = AF_INET;

    if (g_stServMsg.usPtsMajorPort != 0)
    {
        stDestAddr.sin_port = g_stServMsg.usPtsMajorPort;
        stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMajorIP);
    }
    else if (g_stServMsg.usPtsMinorPort != 0)
    {
        stDestAddr.sin_port = g_stServMsg.usPtsMinorPort;
        stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMinorIP);
    }
    else
    {
        stDestAddr.sin_port = 0;
        stDestAddr.sin_addr.s_addr = 0;
    }

    g_pstPtcRecv->stDestAddr = stDestAddr;
    g_pstPtcRecv->pstStreamHead = pstStreamIDHead;

    return DOS_SUCC;
}

/**
 * 函数：S32 ptc_create_socket_proxy(U8 *alServIp, U16 usServPort)
 * 功能：
 *      1.创建和proxy通信的套接字
 * 参数
 * 返回值：
 */
S32 ptc_create_socket_proxy(U8 *alServIp, U16 usServPort)
{
    if (NULL == alServIp)
    {
        return -1;
    }

    S32 lResult = 0;
    S32 lSockfd = 0;
    struct sockaddr_in stServerAddr;
    //socklen_t addrlen = 1;
    unsigned long ul = 1;
    struct timeval tm;
    fd_set set;
    S32 error = -1;
    S32 len = sizeof(int);
    BOOL lRet = DOS_FALSE;
    S32 ulIPAddr = 0;

    lSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lSockfd <= 0)
    {
        perror("create socket");
        return -1;
    }

    //setsockopt(lSockfd, SOL_SOCKET, SO_REUSEADDR, &addrlen, sizeof(addrlen));
    bzero(&stServerAddr, sizeof(stServerAddr));
    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_port = usServPort;
    stServerAddr.sin_addr.s_addr = *(U32 *)alServIp;

    ulIPAddr = *(U32 *)alServIp;

    ioctl(lSockfd, FIONBIO, &ul);  //设置为非阻塞模式

    lResult = connect(lSockfd, (struct sockaddr*)(&stServerAddr), sizeof(stServerAddr));
    if (lResult < 0)
    {
        tm.tv_sec  = TIME_OUT_TIME;
        tm.tv_usec = 0;
        FD_ZERO(&set);
        FD_SET(lSockfd, &set);
        if (select(lSockfd+1, NULL, &set, NULL, &tm) > 0)
        {
            getsockopt(lSockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
            if (error == 0)
            {
                lRet = DOS_TRUE;
            }
            else
            {
                lRet = DOS_FALSE;
            }
        }
        else
        {
            lRet = DOS_FALSE;
        }

    }
    else
    {
        lRet = DOS_TRUE;
    }

    ul = 0;
    ioctl(lSockfd, FIONBIO, &ul);  //设置为阻塞模式

    if (!lRet)
    {
        perror("socket connect");
        close(lSockfd);
        return -1;
    }

    return lSockfd;
}

/**
 * 函数：S32 ptc_get_ptc_id(U8 *szPtcID, S32 lSockfd)
 * 功能：
 *      1.获得ptc 的id
 * 参数
 * 返回值：
 */
S32 ptc_get_ptc_id(U8 *szPtcID)
{

    S32 lResult = 0;
    S32 lTimeStemp = 0;
    S32 i = 0;
    FILE *pFileFd = NULL;
    U8 *pucTr = NULL;
    S8 *pIp= NULL;
    S8 buf[PT_DATA_BUFF_512] = {0};
    struct ifreq *pstIfreq = NULL;
    struct ifconf stIfconf;
    struct ifreq stIfreq;

    pFileFd = fopen("./.id", "r");
    if (pFileFd != NULL)
    {
        lResult = fread(szPtcID, PTC_ID_LEN, 1, pFileFd);
        if (lResult < 1)
        {
            /* 重新生成 */
            fclose(pFileFd);
            pFileFd = NULL;
        }
        else
        {
            szPtcID[PTC_ID_LEN] = '\0';
            if (pts_is_ptc_sn((S8 *)szPtcID))
            {
                fclose(pFileFd);
                pFileFd = NULL;
                return DOS_SUCC;
            }
            else
            {
                /* 重新生成 */
                fclose(pFileFd);
                pFileFd = NULL;
            }
        }
    }

    pFileFd = fopen("./.id", "w");
    if (NULL == pFileFd)
    {
        logr_info("create file '.id' fail");
        return DOS_FAIL;
    }

    /* 获得mac地址 */
    stIfconf.ifc_len = PT_DATA_BUFF_512;
    stIfconf.ifc_buf = buf;

    /* 获取所有接口信息 */
    if (ioctl(g_ulUdpSocket, SIOCGIFCONF, &stIfconf) < 0)
    {
        perror("ioctl");
        return DOS_FAIL;
    }
    pstIfreq = (struct ifreq*)buf;
    i = stIfconf.ifc_len/sizeof(struct ifreq);
    for (; i>0; i--)
    {
        pIp = inet_ntoa(((struct sockaddr_in*)&(pstIfreq->ifr_addr))->sin_addr);

        if(dos_strcmp(pIp, "127.0.0.1") == 0)
        {
            pstIfreq++;
            continue;
        }
        break;
    }

    dos_strcpy(stIfreq.ifr_name, pstIfreq->ifr_name);
    if (ioctl(g_ulUdpSocket, SIOCGIFHWADDR, &stIfreq) < 0)
    {
        perror("ioctl");
        return DOS_FAIL;
    }

    pucTr = (U8 *)&stIfreq.ifr_ifru.ifru_hwaddr.sa_data[0];
    lTimeStemp = time((time_t *)NULL);
    snprintf((S8 *)szPtcID, PTC_ID_LEN+1, "%d0%08X%X%X%X%X%X%X", g_enPtcType, lTimeStemp, *pucTr & 0x0f, *(pucTr+1) & 0x0f, *(pucTr+2) & 0x0f, *(pucTr+3) & 0x0f, *(pucTr+4) & 0x0f, *(pucTr+5) & 0x0f);

    fwrite(szPtcID, PTC_ID_LEN, 1, pFileFd);
    fclose(pFileFd);

    return DOS_SUCC;
}

/**
 * 函数：S32 ptc_main()
 * 功能：
 *      1.ptc main函数
 * 参数
 * 返回值：
 */
S32 ptc_main()
{
    S32 lRet = 0;
    U8 aucID[PTC_ID_LEN+1] = {0};
    pthread_t tid1, tid2, tid3, tid4, tid5, tid6, tid7, tid8;

    /* 初始化socket和streamID对应关系数组 */
    ptc_init_all_ptc_client_cb();

    g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);

    lRet = ptc_get_ptc_id(aucID);
    if (DOS_SUCC == lRet)
    {
        logr_info("ptc id is : %s", aucID);
    }
    else
    {
        logr_info("get ptc id fail");
        return DOS_FAIL;
    }

    lRet = ptc_read_configure();
    if (lRet != DOS_SUCC)
    {
        return -1;
    }

    //ptc_get_udp_use_ip();
    lRet = ptc_init_send_cache_queue(aucID);
    if (lRet != DOS_SUCC)
    {
        return -1;
    }

    lRet = ptc_init_recv_cache_queue(aucID);
    if (lRet != DOS_SUCC)
    {
        return -1;
    }

    lRet = pthread_create(&tid1, NULL, ptc_send_msg2pts, NULL);
    if (lRet < 0)
    {
        logr_info("create pthread error : ptc_send_msg2pts!");
        return -1;
    }
    else
    {
        logr_debug("create pthread succ : ptc_send_msg2pts!");
    }

    lRet = pthread_create(&tid2, NULL, ptc_recv_msg_from_pts, NULL);
    if (lRet < 0)
    {
        logr_info("create pthread error : ptc_recv_msg_from_pts!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("create pthread succ : ptc_recv_msg_from_pts!");
    }

    lRet = pthread_create(&tid7, NULL, ptc_handle_recvfrom_web_msg, NULL);
    if (lRet < 0)
    {
        logr_info("create pthread error : ptc_handle_recvfrom_web_msg!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("create pthread succ : ptc_handle_recvfrom_web_msg!");
    }

    lRet = pthread_create(&tid3, NULL, ptc_recv_msg_from_web, NULL);
    if (lRet < 0)
    {
        logr_info("create pthread error : ptc_recv_msg_from_web!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("create pthread succ : ptc_recv_msg_from_web!");
    }

    lRet = pthread_create(&tid4, NULL, ptc_send_msg2proxy, NULL);
    if (lRet < 0)
    {
        logr_info("create pthread error : ptc_send_msg2proxy!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("create pthread succ : ptc_send_msg2proxy!");
    }

    lRet = pthread_create(&tid5, NULL, ptc_recv_msg_from_cmd, NULL);
    if (lRet < 0)
    {
        logr_info("create pthread error : ptc_recv_msg_from_cmd!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("create pthread succ : ptc_recv_msg_from_cmd!");
    }

    lRet = pthread_create(&tid6, NULL, ptc_deal_with_pts_command, NULL);
    if (lRet < 0)
    {
        logr_info("create pthread error : ptc_deal_with_pts_command!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("create pthread succ : ptc_deal_with_pts_command!");
    }

    lRet = pthread_create(&tid8, NULL, ptc_terminal_dispose, NULL);
    if (lRet < 0)
    {
        logr_info("create pthread error : ptc_terminal_dispose!");
        return DOS_FAIL;
    }
    else
    {
        logr_debug("create pthread succ : ptc_terminal_dispose!");
    }

    sleep(2);
    ptc_send_login2pts();

    return DOS_SUCC;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

