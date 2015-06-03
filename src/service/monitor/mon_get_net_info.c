#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_NET_MONITOR

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <net/if.h>
#include <errno.h>

#include "mon_get_net_info.h"
#include "mon_lib.h"

typedef struct tagMonTransData
{
    U64 uLInSize;
    U64 uLOutSize;
}MON_TRANS_DATA_S;

extern S8 g_szMonNetworkInfo[MAX_NETCARD_CNT * MAX_BUFF_LENGTH];
extern MON_NET_CARD_PARAM_S * g_pastNet[MAX_NETCARD_CNT];
extern U32 g_ulNetCnt;

//用来记录两个时间戳
time_t *m_pt1 = NULL, *m_pt2 = NULL;

//用来记录之前特定时刻和当前时刻的数据传输
MON_TRANS_DATA_S *m_pstTransFormer = NULL;
MON_TRANS_DATA_S *m_pstTransCur = NULL;

static U32 mon_net_reset_data();

/**
 * 功能:为网卡数组分配内存
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_netcard_malloc()
{
    U32 ulRows = 0;
    MON_NET_CARD_PARAM_S * pastNet;

    pastNet = (MON_NET_CARD_PARAM_S *)dos_dmem_alloc(MAX_NETCARD_CNT * sizeof(MON_NET_CARD_PARAM_S));
    if(DOS_ADDR_INVALID(pastNet))
    {
      logr_cirt("%s:Line %u:mon_netcard_malloc|pastNet is %p!"
                , dos_get_filename(__FILE__), __LINE__, pastNet);
      goto fail;
    }

    memset(pastNet, 0, MAX_NETCARD_CNT * sizeof(MON_NET_CARD_PARAM_S));
    for(ulRows = 0; ulRows < MAX_NETCARD_CNT; ulRows++)
    {
      g_pastNet[ulRows] = &(pastNet[ulRows]);
    }

    m_pstTransFormer = (MON_TRANS_DATA_S *)dos_dmem_alloc(sizeof(MON_TRANS_DATA_S));
    if (DOS_ADDR_INVALID(m_pstTransFormer))
    {
       logr_error("%s:Line %u: Alloc Memory FAIL.", dos_get_filename(__FILE__), __LINE__);
       goto fail;
    }
    memset(m_pstTransFormer, 0, sizeof(MON_TRANS_DATA_S));

    m_pstTransCur = (MON_TRANS_DATA_S *)dos_dmem_alloc(sizeof(MON_TRANS_DATA_S));
    if (DOS_ADDR_INVALID(m_pstTransCur))
    {
       logr_error("%s:Line %u: Alloc Memory FAIL.", dos_get_filename(__FILE__), __LINE__);
       goto fail;
    }
    memset(m_pstTransCur, 0, sizeof(MON_TRANS_DATA_S));

    m_pt1 = dos_dmem_alloc(sizeof(time_t));
    if (DOS_ADDR_INVALID(m_pt1))
    {
       logr_error("%s:Line %u: Alloc memory FAIL.");
       goto fail;
    }
   
    m_pt2 = dos_dmem_alloc(sizeof(time_t));
    if (DOS_ADDR_INVALID(m_pt2))
    {
       logr_error("%s:Line %u: Alloc memory FAIL.");
       goto fail;
    }

    return DOS_SUCC;
fail:
    if (DOS_ADDR_VALID(pastNet))
    {
       dos_dmem_free(pastNet);
       pastNet = NULL;
    }
    if (DOS_ADDR_VALID(g_pastNet[0]))
    {
       dos_dmem_free( g_pastNet[0]); 
       g_pastNet[0] = NULL;
    }
    if (DOS_ADDR_VALID(m_pstTransFormer))
    {
       dos_dmem_free(m_pstTransFormer);
       m_pstTransFormer = NULL;
    }
    if (DOS_ADDR_VALID(m_pstTransCur))
    {
       dos_dmem_free(m_pstTransCur);
       m_pstTransCur = NULL;
    }
    if (DOS_ADDR_VALID(m_pt1))
    {
       dos_dmem_free(m_pt1);
       m_pt1 = NULL;
    }
    if (DOS_ADDR_VALID(m_pt2))
    {
       dos_dmem_free(m_pt2);
       m_pt2 = NULL;
    }

    return DOS_FAIL;
}

/**
 * 功能:释放为网卡数组分配的内存
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32  mon_netcard_free()
{
    U32 ulRows = 0;
    MON_NET_CARD_PARAM_S * pastNet = g_pastNet[0];
    
    if(DOS_ADDR_INVALID(pastNet))
    {
        logr_cirt("%s:Line %u:mon_netcard_free|pastNet is %p!"
                  , dos_get_filename(__FILE__), __LINE__ , pastNet);
        return DOS_FAIL;
    }
    dos_dmem_free(pastNet);

    for(ulRows = 0; ulRows < MAX_NETCARD_CNT; ulRows++)
    {
      g_pastNet[ulRows] = NULL;
    }

    if (DOS_ADDR_VALID(m_pstTransCur))
    {
      dos_dmem_free(m_pstTransCur);
      m_pstTransCur = NULL;
    }

    if (DOS_ADDR_VALID(m_pstTransFormer))
    {
      dos_dmem_free(m_pstTransFormer);
      m_pstTransFormer = NULL;
    }

    if (DOS_ADDR_VALID(m_pt1))
    {
       dos_dmem_free(m_pt1);
       m_pt1 = NULL;
    }
    if (DOS_ADDR_VALID(m_pt2))
    {
       dos_dmem_free(m_pt2);
       m_pt2 = NULL;
    }

    return DOS_SUCC;
} 

static U32 mon_net_reset_data()
{
    MON_NET_CARD_PARAM_S * pastNet = g_pastNet[0];
    if (DOS_ADDR_INVALID(pastNet))
    {
        logr_cirt("%s:Line %u:mon_netcard_free|pastNet is %p!"
                , dos_get_filename(__FILE__), __LINE__ , pastNet);
        return DOS_FAIL;
    }

    dos_memzero(pastNet, MAX_NETCARD_CNT * sizeof(MON_NET_CARD_PARAM_S));

    return DOS_SUCC;
}

/** 
 * 判断原理:
 *   proc文件系统中有这样一个文件记录了网络连接状态信息
 *   /sys/class/net/$netCardName/carrier
 *   如果文件为空，则表明当前网卡已失去连接
 *   如果连接OK，则它会读出数据"1"
 *
 *
 * 功能:判断网卡的网口是否开启
 * 参数集：
 *   参数1:const S8 * pszNetCard 网卡名称
 * 返回值：
 *   开启则返回DOS_TRUE，否则返回DOS_FALSE
 */
BOOL mon_is_netcard_connected(const S8 * pszNetCard)
{
    FILE *pstNetFp = NULL;
    S8  szBuff[512] = {0};
    S8  szCarrierPath[512] = {0};
    BOOL bRet = DOS_FALSE;

    dos_snprintf(szCarrierPath, sizeof(szCarrierPath), "/sys/class/net/%s/carrier", pszNetCard);
    if (NULL != (pstNetFp = fopen(szCarrierPath, "r")))
    {
        if(NULL != fgets(szBuff, sizeof(szBuff), pstNetFp))
        {
            if ('0' == szBuff[0])
            {
                bRet = DOS_FALSE;
            }
            else
            {
                bRet =  DOS_TRUE;
            }
        }
        else
        {
            logr_error("%s:Line %u: File \"%s\" has no content.", dos_get_filename(__FILE__), __LINE__, szCarrierPath);
            bRet = DOS_FALSE;
        }
    }
    else
    {
        logr_error("%s:Line %u:file \"%s\" open FAIL."
                    , dos_get_filename(__FILE__), __LINE__, szCarrierPath);
        bRet = DOS_FALSE;
    }

    fclose(pstNetFp);
    pstNetFp = NULL;
     
    return bRet;
}

/**
 *  文件/proc/net/dev列出了所有网卡的数据传输情况，对应的数据格式如下:
 *  Inter-|   Receive                                                |  Transmit
 *  face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
 *     lo:812170463 1868650    0    0    0     0          0         0 812170463 1868650    0    0    0     0       0          0
 *   eth0:2917065308 11065515    0    0    0     0          0         0 4520109097 13962214    0    0    0     0       0          0
 *
 *  算法: 假设t1时刻，对应的接收数据总字节数recv1,对应的发送数据总字节数send1
 *        t2时刻，对应的接收数据总字节数recv2,对应的发送数据总字节数send2
 *        则t1~t2时刻的数据传输速率是:  (recv2 - recv1 + send2 - send1)/(t2 - t1)
 *
 *
 * 输出结果参照mon_is_netcard_connected函数
 * 功能:获取网口的数据传输速率，单位是kbps
 * 参数集：
 *   参数1:const S8 * pszDevName 网卡名称
 * 返回值：
 *   成功返回网口的数据传输速率，失败返回-1
 */
U32 mon_get_data_trans_speed(const S8 * pszDevName)
{
    U32 ulInterval = 0;
    S8  szNetFile[] = "/proc/net/dev";
    S8  szNetInfo[1024] = {0};
    FILE *fp = NULL;
    S8 *pszInfo = NULL;
    S8 *ppszData[16] = {0};
    U32 ulRet = U32_BUTT;
    S64 LRet = -1;
    U64 uLIn = U64_BUTT, uLOut = U64_BUTT;

    *m_pt1 = *m_pt2;
    *m_pt2 = time(0);
    ulInterval = *m_pt2 - *m_pt1;
    if (0 == ulInterval)
    {
        ulInterval = 5;
        logr_info("%s:Line %u: First Run.");
    }

    m_pstTransFormer->uLInSize = m_pstTransCur->uLInSize;
    m_pstTransFormer->uLOutSize = m_pstTransCur->uLOutSize;

    fp = fopen(szNetFile, "r");
    if (DOS_ADDR_INVALID(fp))
    {
        logr_error("%s:Line %u:File \"%s\" open FAIL.", dos_get_filename(__FILE__), __LINE__, szNetFile);
        return DOS_FAIL;
    }

    while(NULL != fgets(szNetInfo, sizeof(szNetInfo), fp))
    {
        pszInfo = dos_strstr(szNetInfo, (S8 *)pszDevName);
        if (DOS_ADDR_VALID(pszInfo))
        {
            break;
        }
    }

    while(*(pszInfo - 1) != ':')
    {
        ++pszInfo;
    }

    ulRet = mon_analyse_by_reg_expr(pszInfo, " \t\n", ppszData, sizeof(ppszData) / sizeof(ppszData[0]));
    if (DOS_SUCC != ulRet)
    {
        logr_error("%s:Line %u: Analysis string FAIL.");
        fclose(fp);
        return DOS_FAIL;
    }

    if (dos_atoull(ppszData[0], &uLIn) < 0 
        || dos_atoull(ppszData[8], &uLOut) < 0)
    {
        logr_error("%s:Line %u: dos_atoull FAIL.");
        fclose(fp);
        return DOS_FAIL;
    }

    m_pstTransCur->uLInSize = uLIn;
    m_pstTransCur->uLOutSize = uLOut;

    LRet = (S64)m_pstTransCur->uLInSize - (S64)m_pstTransFormer->uLInSize 
         + (S64)m_pstTransCur->uLOutSize - (S64)m_pstTransFormer->uLOutSize;

    ulInterval *= 1024;
    
    fclose(fp);
    fp = NULL;

    /*单位是KB/s*/
    return (LRet + LRet % ulInterval) / ulInterval; 
}


/**
 * 功能:获取网卡数量，所有网卡的MAC地址、IPv4地址、广播IP地址、子网掩码、网卡的网口最大数据传输速率
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_get_netcard_data()
{
    U32 ulFd = 0, ulLength = 0, ulRet = 0;
    U32 ulInterfaceNum = 0;
    struct ifreq astReq[16];
    struct ifconf stIfc;
    struct ifreq stIfrcopy;
    S8  szMac[32] = {0};
    S8  szIPv4Addr[32] = {0};
    S8  szBroadAddr[32] = {0};
    S8  szSubnetMask[32] = {0};

    ulRet = mon_net_reset_data();
    if (DOS_SUCC != ulRet)
    {
        logr_error("%s:Line %u:reset net data FAIL.", dos_get_filename(__FILE__), __LINE__);
        return DOS_FAIL;
    }

    if ((ulFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        logr_error("%s:Line %u: socket failed!", dos_get_filename(__FILE__)
                 , __LINE__);
        goto failure;
    }

    stIfc.ifc_len = sizeof(astReq);
    stIfc.ifc_buf = (caddr_t)astReq;
    if (!ioctl(ulFd, SIOCGIFCONF, (S8 *)&stIfc))
    {
        /* 获取网卡设备的个数 */
        ulInterfaceNum = g_ulNetCnt= stIfc.ifc_len / sizeof(struct ifreq);
        while (ulInterfaceNum > 0)
        {
            ulInterfaceNum--;
            /*获得设备名称*/

            if(DOS_ADDR_INVALID(g_pastNet[ulLength]))
            {
                  logr_cirt("%s:Line %u:mon_get_netcard_data|get netcard data failure,m_pastNet[%d] is %p!"
                              , dos_get_filename(__FILE__), __LINE__ ,ulLength ,g_pastNet[ulLength]);
                  goto failure;
            }
         
           
            dos_strcpy(g_pastNet[ulLength]->szNetDevName, astReq[ulInterfaceNum].ifr_name);
            (g_pastNet[ulLength]->szNetDevName)[dos_strlen(astReq[ulInterfaceNum].ifr_name)] = '\0';
            stIfrcopy = astReq[ulInterfaceNum];

            if (ioctl(ulFd, SIOCGIFFLAGS, &stIfrcopy))
            {
                logr_error("%s:Line %u:mon_get_netcard_data|get netcard data failure,error no is %s"
                            , dos_get_filename(__FILE__), __LINE__, strerror(errno));
                goto failure;
            }

            if (!ioctl(ulFd, SIOCGIFHWADDR, (S8 *)(&astReq[ulInterfaceNum])))
            {
                memset(szMac, 0, sizeof(szMac));
                dos_snprintf(szMac, sizeof(szMac), "%02x:%02x:%02x:%02x:%02x:%02x",
                       (U8)astReq[ulInterfaceNum].ifr_hwaddr.sa_data[0],
                       (U8)astReq[ulInterfaceNum].ifr_hwaddr.sa_data[1],
                       (U8)astReq[ulInterfaceNum].ifr_hwaddr.sa_data[2],
                       (U8)astReq[ulInterfaceNum].ifr_hwaddr.sa_data[3],
                       (U8)astReq[ulInterfaceNum].ifr_hwaddr.sa_data[4],
                       (U8)astReq[ulInterfaceNum].ifr_hwaddr.sa_data[5]);
                /*获得设备Mac地址*/
                dos_strcpy(g_pastNet[ulLength]->szMacAddress, szMac);
                g_pastNet[ulLength]->szMacAddress[dos_strlen(szMac)] = '\0';
            }
            else
            {
                logr_error("%s:Line %u:mon_get_netcard_data|get netcard data failure,error no is %s"
                            , dos_get_filename(__FILE__), __LINE__, strerror(errno));
                goto failure;
            }
             
            if (!ioctl(ulFd, SIOCGIFADDR, (S8 *)&astReq[ulInterfaceNum]))
            {     
                dos_snprintf(szIPv4Addr, sizeof(szIPv4Addr), "%s",
                     (S8 *)inet_ntoa(((struct sockaddr_in *)&(astReq[ulInterfaceNum].ifr_addr))->sin_addr));

                /*获得设备ip地址*/
                dos_strcpy(g_pastNet[ulLength]->szIPAddress, szIPv4Addr);
                g_pastNet[ulLength]->szIPAddress[dos_strlen(szIPv4Addr)] = '\0';
            }
            else
            {
                logr_error("%s:Line %u:mon_get_netcard_data|get netcard data failure,error no is %s"
                              , dos_get_filename(__FILE__), __LINE__, strerror(errno));
                goto failure;
            }

            if (!ioctl(ulFd, SIOCGIFBRDADDR, &astReq[ulInterfaceNum]))
            {
                dos_snprintf(szBroadAddr, sizeof(szBroadAddr), "%s",
                     (S8 *)inet_ntoa(((struct sockaddr_in *)&(astReq[ulInterfaceNum].ifr_broadaddr))->sin_addr));
                /*获得设备广播ip地址*/
                dos_strcpy(g_pastNet[ulLength]->szBroadIPAddress, szBroadAddr);
                g_pastNet[ulLength]->szBroadIPAddress[dos_strlen(szBroadAddr)] = '\0';
            }
            else
            {
                logr_error("%s:Line %u:mon_get_netcard_data|get netcard data failure,error no is %s"
                            , dos_get_filename(__FILE__), __LINE__, strerror(errno));
                goto failure;
            }

            if (!ioctl(ulFd, SIOCGIFNETMASK, &astReq[ulInterfaceNum]))
            {
                dos_snprintf(szSubnetMask, sizeof(szSubnetMask), "%s",
                   (S8 *)inet_ntoa(((struct sockaddr_in *)&(astReq[ulInterfaceNum].ifr_netmask))->sin_addr));
                /*获得设备子网掩码*/
                dos_strcpy(g_pastNet[ulLength]->szNetMask, szSubnetMask);
                g_pastNet[ulLength]->szNetMask[dos_strlen(szSubnetMask)] = '\0';
            }
            else
            {
                logr_error("%s:Line %u:mon_get_netcard_data|get netcard data failure,error no is %s"
                            , dos_get_filename(__FILE__), __LINE__, strerror(errno));
                goto failure;
            }

            if (0 == dos_strcmp("lo", g_pastNet[ulLength]->szNetDevName))
            {
                g_pastNet[ulLength]->ulRWSpeed = 0;
                continue;
            }
         
            ulRet = mon_get_data_trans_speed(g_pastNet[ulLength]->szNetDevName);
            if (DOS_FAIL == ulRet)
            {
                logr_error("%s:Line %u: Get data transport speed FAIL."
                            , dos_get_filename(__FILE__), __LINE__);
                goto failure;
            }
         
            g_pastNet[ulLength]->ulRWSpeed = ulRet;
         
            ulLength++;
        }
    }
    else
    {
        logr_error("%s:Line %u:mon_get_netcard_data|get netcard data failure,error no is %s"
                , dos_get_filename(__FILE__), __LINE__, strerror(errno));
        goto failure;
    }
    goto success;

failure:
    close(ulFd);
    ulFd = U32_BUTT;
    return DOS_FAIL;
success:
    close(ulFd);
    ulFd = U32_BUTT;
    return DOS_SUCC;
}

/**
 * 功能:获取网卡数组信息的格式化信息字符串
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32  mon_netcard_formatted_info()
{
    U32 ulRows = 0;

    memset(g_szMonNetworkInfo, 0, MAX_NETCARD_CNT * MAX_BUFF_LENGTH);

    for (ulRows = 0; ulRows < g_ulNetCnt; ulRows++)
    {
        S8 szTempInfo[MAX_BUFF_LENGTH] = {0};

        if(DOS_ADDR_INVALID(g_pastNet[ulRows]))
        {
            logr_cirt("%s:Line %u:mon_netcard_formatted_info|get netcard formatted information failure,m_pastNet[%u] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, ulRows, g_pastNet[ulRows]);
            return DOS_FAIL;
        }
       
        dos_snprintf(szTempInfo, MAX_BUFF_LENGTH
                    , "Netcard:%s\nMacAddress:%s\nIPAddress:%s\nBroadIPAddress:%s\nSubNetMask:%s\nData Transmission speed:%u KB/s\n"
                    , g_pastNet[ulRows]->szNetDevName
                    , g_pastNet[ulRows]->szMacAddress
                    , g_pastNet[ulRows]->szIPAddress
                    , g_pastNet[ulRows]->szBroadIPAddress
                    , g_pastNet[ulRows]->szNetMask
                    , g_pastNet[ulRows]->ulRWSpeed);
        dos_strcat(g_szMonNetworkInfo, szTempInfo);
        dos_strcat(g_szMonNetworkInfo, "\n");
    }

    return DOS_SUCC;
}

#endif //end #if INCLUDE_NET_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif