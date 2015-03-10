#ifdef  __cplusplus
extern "C"{
#endif

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
#include <net/if.h>
#include <sys/ioctl.h>

#include <dos.h>
#include <pt/pt.h>
#include <pt/des.h>
#include <pt/md5.h>

PT_PROT_TYPE_EN g_aenDataProtType[PT_DATA_BUTT] = {PT_TCP, PT_TCP, PT_TCP};
static U32 g_ulPTCurrentLogLevel = LOG_LEVEL_NOTIC;  /* 日志打印级别 */

struct
{
    S32 a;
    U8 q1;
    S32 b;
    U8 q2;
    S32 c;
    U8 q3;
    S32 d;
}m_stIpFormat;

/**
 * 函数：BOOL pts_is_or_not_ip(S8 *szIp)
 * 功能：
 *      1.判断是否符合ip地址的格式
 * 参数
 * 返回值:
 */
BOOL pt_is_or_not_ip(S8 *szIp)
{
    dos_memzero(&m_stIpFormat, sizeof(m_stIpFormat));
    sscanf(szIp, "%d%c%d%c%d%c%d", &m_stIpFormat.a, &m_stIpFormat.q1, &m_stIpFormat.b, &m_stIpFormat.q2, &m_stIpFormat.c, &m_stIpFormat.q3, &m_stIpFormat.d);
    if(m_stIpFormat.a<256 && m_stIpFormat.a>=0 && m_stIpFormat.b<256 && m_stIpFormat.b>=0 && m_stIpFormat.c<256 && m_stIpFormat.c>=0 && m_stIpFormat.d<256 && m_stIpFormat.d>=0)
    {
        if(m_stIpFormat.q1=='.' && m_stIpFormat.q2=='.' && m_stIpFormat.q3=='.')
        {
            return DOS_TRUE;
        }
        else
        {
            return DOS_FALSE;
        }
    }
    else
    {
        return DOS_FALSE;
    }
}

/**
 * 函数：S32 ptc_DNS_resolution(S8 *szDomainName, U8 paucIPAddr[][IPV6_SIZE], U32 ulIPSize)
 * 功能：
 *      1.域名解析
 * 参数
 *      VOID *arg :通道通信的sockfd
 * 返回值：void *
 */
S32 pt_DNS_resolution(S8 *szDomainName, U8 paucIPAddr[IPV6_SIZE])
{
    S8 **pptr = NULL;
    struct hostent *hptr = NULL;
    S8 szstr[PT_IP_ADDR_SIZE] = {0};
    S32 i = 0;

    hptr = gethostbyname(szDomainName);
    if (NULL == hptr)
    {
        pt_logr_info("gethostbyname error for host:%s\n", szDomainName);
        return DOS_FAIL;
    }

    switch (hptr->h_addrtype)
    {
        case AF_INET:
            /* break */
        case AF_INET6:
            pptr = hptr->h_addr_list;
            for(i=0; *pptr!=NULL; pptr++, i++)
            {
                if (i > 0)
                {
                    break;
                }
                inet_ntop(hptr->h_addrtype, *pptr, szstr, sizeof(szstr));
                pt_logr_debug("address:%s\n", szstr);
                inet_pton(hptr->h_addrtype, szstr, (VOID *)(paucIPAddr));
            }
            inet_ntop(hptr->h_addrtype, hptr->h_addr, szstr, sizeof(szstr));
            pt_logr_debug("first address: %s", szstr);
            printf("first address: %s\n", szstr);
            break;
        default:
            break;
    }

    return i;
}

S8 *pt_get_gmt_time(U32 ulExpiresTime)
{
   time_t timestamp = 0;
   struct tm *pstTm = NULL;

   timestamp = time(NULL) + ulExpiresTime;
   pstTm = gmtime(&timestamp);
    //Mon, 20 Jul 2009 23:00:00
   //snprintf(szGTMTime, PT_DATA_BUFF_64, "%s, ", pstTm->);

   return asctime(pstTm);
}

S32 pt_md5_encrypt(S8 *szEncrypt, S8 *szDecrypt)
{
    if (NULL == szEncrypt || NULL == szDecrypt)
    {
        return DOS_FAIL;
    }

    S32 i = 0;
    U8 aucDecrypt[PT_MD5_ARRAY_SIZE];
    MD5_CTX md5;

    MD5Init(&md5);
    MD5Update(&md5, (U8 *)szEncrypt, dos_strlen(szEncrypt));
    MD5Final(&md5, aucDecrypt);

    for(i=0;i<PT_MD5_ARRAY_SIZE;i++)
    {
        sprintf(szDecrypt + 2*i, "%02x", aucDecrypt[i]);
    }
    szDecrypt[PT_MD5_LEN-1] = '\0';

    return DOS_SUCC;
}

#if 0
/**
 * 函数：VOID pt_des_encrypt_or_decrypt(U8 *ulDesKey, S16 sProcessMode, U8 *ucData, U32 ulDataLen, U8 *ucDestData)
 * 功能：
 *      1.DES加密
 *      2.DES解密
 * 参数
 * 返回值：
 */
S32 pt_des_encrypt_or_decrypt(U8 *ulDesKey, S16 sProcessMode, U8 *ucData, U32 ulDataLen, U8 *ucDestData)
{
    if (NULL == ulDesKey || NULL == ucData || NULL == ucDestData)
    {
        return DOS_FAIL;
    }

    U32 ulBlockCount = 0;
    U16 usPadding = 0;
    U8 *pDataBlock = (U8*)dos_dmem_alloc(8);
    if (NULL == pDataBlock)
    {
        return DOS_FAIL;
    }

    U8 *pProcessedBlock = (U8*)dos_dmem_alloc(8);
    if (NULL == pProcessedBlock)
    {
        dos_dmem_free(pDataBlock);
        return DOS_FAIL;
    }

    key_set* pstKeySets = (key_set*)dos_dmem_alloc(17*sizeof(key_set));
    if (NULL == pstKeySets)
    {
        dos_dmem_free(pDataBlock);
        dos_dmem_free(pProcessedBlock);
        return DOS_FAIL;
    }

    generate_sub_keys("abcdefgh", pstKeySets);
    U8 *pTest = (U8 *)pstKeySets;
    U32 i = 0;

    for (i=0; i<17*sizeof(key_set); i++)
    {
        printf("%02x ", pTest[i]);
    }
    printf("\n");

    dos_strncpy(pDataBlock, ucData, ulDataLen);
    if (sProcessMode == ENCRYPTION_MODE)
    {
        usPadding = 8 - ulDataLen%8;
        memset((pDataBlock + 8 - usPadding), (U8)usPadding, usPadding);

        process_message(pDataBlock, pProcessedBlock, pstKeySets, sProcessMode);
        dos_memcpy(ucDestData, pProcessedBlock, 8);
    }
    else
    {
        process_message(pDataBlock, pProcessedBlock, pstKeySets, sProcessMode);
        usPadding = pProcessedBlock[7];
        if (usPadding < 8)
        {
            dos_memcpy(ucDestData, pProcessedBlock, 8 - usPadding);
        }
    }

    dos_dmem_free(pDataBlock);
    dos_dmem_free(pProcessedBlock);
    dos_dmem_free(pstKeySets);

    return DOS_SUCC;
}

#endif
/**
 * 函数：BOOL pts_is_ptc_id(S8* pcUrl)
 * 功能：
 *      1.判断是否是ptc ID
 * 参数
 * 返回值：
 */
BOOL pts_is_ptc_id(S8* pcUrl)
{
    S32 i = 0;

    if(PTC_ID_LEN == dos_strlen(pcUrl))
    {
        for (i=0; i<PTC_ID_LEN; i++)
        {
            if ((pcUrl[i] < 'a' || pcUrl[i] > 'f') && (pcUrl[i] < '0' || pcUrl[i] > '9') && (pcUrl[i] < 'A' || pcUrl[i] > 'F'))
            {
                return DOS_FALSE;
            }
        }
        return DOS_TRUE;
    }
    return DOS_FALSE;
}

BOOL pts_is_int(S8* pcUrl)
{
    S32 i = 0;
    if (NULL == pcUrl)
    {
        return DOS_FALSE;
    }

    for (i=0; i<dos_strlen(pcUrl); i++)
    {
        if (pcUrl[i] < '0' || pcUrl[i] > '9')
        {
            return DOS_FALSE;
        }
    }
    return DOS_TRUE;
}

/**
 * 函数：VOID pt_log_set_level(U32 ulLevel)
 * 功能：
 *      1.设置日志级别
 * 参数
 *      U32 ulLevel :新的级别
 * 返回值：void
 */
VOID pt_log_set_level(U32 ulLevel)
{
    g_ulPTCurrentLogLevel = ulLevel;
}

/**
 * 函数：U32 pt_log_current_level()
 * 功能：
 *      1.获得当前的日志级别
 * 参数
 * 返回值：
 */
U32 pt_log_current_level()
{
    return g_ulPTCurrentLogLevel;
}

/**
 * 函数：VOID pt_logr_write(U32 ulLevel, S8 *pszFormat, ...)
 * 功能：
 *      1.日志打印函数
 * 参数
 * 返回值：
 */
VOID pt_logr_write(U32 ulLevel, S8 *pszFormat, ...)
{
    va_list argptr;
    char szBuf[PT_DATA_BUFF_1024];

    va_start(argptr, pszFormat);
    vsnprintf(szBuf, PT_DATA_BUFF_1024, pszFormat, argptr);
    va_end(argptr);
    szBuf[sizeof(szBuf) -1] = '\0';

    if (ulLevel <= g_ulPTCurrentLogLevel)
    {
        dos_log(ulLevel, LOG_TYPE_RUNINFO, szBuf);
    }
}

/**
 * 函数：PT_DATA_TCP_ST *pt_data_tcp_queue_create(U32 ulCacheSize)
 * 功能：
 *      1.创建存放TCP数据的数组
 * 参数
 *      U32 ulCacheSize : 缓存数量
 * 返回值：
 *      NULL :分配空间失败
 */
PT_DATA_TCP_ST *pt_data_tcp_queue_create(U32 ulCacheSize)
{
    PT_DATA_TCP_ST *pstHead = NULL;

    pstHead = (PT_DATA_TCP_ST *)dos_dmem_alloc(sizeof(PT_DATA_TCP_ST) * ulCacheSize);
    if (NULL == pstHead)
    {
        perror("malloc");
        return NULL;
    }
    else
    {
        dos_memzero(pstHead, sizeof(PT_DATA_TCP_ST) * ulCacheSize);
        return pstHead;
    }
}

/**
 * 函数：PT_STREAM_CB_ST *pt_stream_node_create(U32 ulStreamID)
 * 功能：
 *      1.创建stream节点
 * 参数
 *      U32 ulStreamID: 创建的stream节点的id
 * 返回值：NULL: 分配失败
 */
PT_STREAM_CB_ST *pt_stream_node_create(U32 ulStreamID)
{
    PT_STREAM_CB_ST *stStreamNode = NULL;
    list_t *pstHead = NULL;
    stStreamNode = (PT_STREAM_CB_ST *)dos_dmem_alloc(sizeof(PT_STREAM_CB_ST));
    if (NULL == stStreamNode)
    {
        perror("dos_dmem_malloc");
        return NULL;
    }

    stStreamNode->unDataQueHead.pstDataTcp = NULL;
    stStreamNode->unDataQueHead.pstDataUdp = NULL;
    stStreamNode->ulStreamID = ulStreamID;
    stStreamNode->lMaxSeq = -1;

    stStreamNode->lCurrSeq = -1;
    stStreamNode->lConfirmSeq = -1;
    stStreamNode->hTmrHandle = NULL;
    stStreamNode->ulCountResend = 0;
    stStreamNode->pstLostParam = NULL;

    pstHead = &(stStreamNode->stStreamListNode);
    dos_list_init(pstHead);

    return stStreamNode;
}

/**
 * 函数：S32 pt_send_data_tcp_queue_insert(PT_STREAM_CB_ST *pstStreamNode, PT_MSG_TAG *pstMsgDes, S8 *acSendBuf, S32 lDataLen, U32 lCacheSize)
 * 功能：
 *      1.添加消息到发送缓存
 * 参数
 *      PT_STREAM_CB_ST *pstStreamNode  : 缓存的所在的stream节点
 *      PT_MSG_TAG *pstMsgDes           : 消息描述
 *      S8 *acSendBuf                   : 消息内容
 *      S32 lDataLen                    : 消息内容大小
 *      U32 lCacheSize                  : 缓存的个数(用已求出数组的下标)
 * 返回值:
 */
S32 pt_send_data_tcp_queue_insert(PT_STREAM_CB_ST *pstStreamNode, S8 *acSendBuf, S32 lDataLen, U32 lCacheSize)
{
    if (NULL == pstStreamNode || NULL == acSendBuf)
    {
        return PT_SAVE_DATA_FAIL;
    }

    S32 lSeq = 0;
    U32 ulArraySub = 0;
    PT_DATA_TCP_ST *pstDataQueHead = pstStreamNode->unDataQueHead.pstDataTcp;
    if (NULL == pstDataQueHead)
    {
        return PT_SAVE_DATA_FAIL;
    }

    /* TODO 翻转判断,1023G,会翻转吗 */
    lSeq = ++pstStreamNode->lMaxSeq;
    ulArraySub = lSeq & (lCacheSize - 1);  /*求余数,包存放在数组中的位置*/
    dos_memcpy(pstDataQueHead[ulArraySub].szBuff, acSendBuf, lDataLen);
    pstDataQueHead[ulArraySub].lSeq = lSeq;
    pstDataQueHead[ulArraySub].ulLen = lDataLen;
    pstDataQueHead[ulArraySub].ExitNotifyFlag = DOS_FALSE;
    pt_logr_debug("save into send cache data seq : %d,  currSeq : %d, array :%d, stream : %d , data len : %d", lSeq , pstStreamNode->lCurrSeq, ulArraySub, pstStreamNode->ulStreamID, lDataLen);
    /* 每64包，发送一次 */
    if (lSeq - pstStreamNode->lConfirmSeq >= PT_CONFIRM_RECV_MSG_SIZE)
    {
        return PT_NEED_CUT_PTHREAD;
    }
    return PT_SAVE_DATA_SUCC;
}

/**
 * 函数：S32 pt_recv_data_tcp_queue_insert(PT_STREAM_CB_ST *pstStreamNode, PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen, U32 lCacheSize)
 * 功能：
 *      1.添加消息到接收缓存
 * 参数
 *      PT_STREAM_CB_ST *pstStreamNode  : 缓存的所在的stream节点
 *      PT_MSG_TAG *pstMsgDes           : 消息描述
 *      S8 *acRecvBuf                   : 消息内容
 *      S32 lDataLen                    : 消息内容大小
 *      U32 lCacheSize                  : 缓存的个数(用已求出数组的下标)
 * 返回值:
 */
S32 pt_recv_data_tcp_queue_insert(PT_STREAM_CB_ST *pstStreamNode, PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen, U32 lCacheSize)
{
    if (NULL == pstStreamNode || NULL == pstMsgDes || NULL == acRecvBuf)
    {
        return PT_SAVE_DATA_FAIL;
    }

    PT_DATA_TCP_ST *pstDataQueHead = pstStreamNode->unDataQueHead.pstDataTcp;
    if (NULL == pstDataQueHead)
    {
        return PT_SAVE_DATA_FAIL;
    }

    S32 lRet = 0;
    U32 ulNextSendArraySub = 0;
    U32 ulArraySub = pstMsgDes->lSeq & (lCacheSize - 1);  /*求余数,包存放在数组中的位置*/

    if (pstStreamNode->lCurrSeq >= pstMsgDes->lSeq)
    {
        /* 包已发送，不需要存储 */
        return PT_SAVE_DATA_SUCC;
    }

    if ((pstDataQueHead[ulArraySub].lSeq == pstMsgDes->lSeq) &&  pstMsgDes->lSeq != 0)
    {
        /*包已存在。0号包时，即使没有存，也成立*/
        return PT_SAVE_DATA_SUCC;
    }
    dos_memcpy(pstDataQueHead[ulArraySub].szBuff, acRecvBuf, lDataLen);
    pstDataQueHead[ulArraySub].lSeq = pstMsgDes->lSeq;
    pstDataQueHead[ulArraySub].ulLen = lDataLen;
    pstDataQueHead[ulArraySub].ExitNotifyFlag = pstMsgDes->ExitNotifyFlag;

    /* 更新stream node信息 */
    pstStreamNode->lMaxSeq = pstStreamNode->lMaxSeq > pstMsgDes->lSeq ? pstStreamNode->lMaxSeq : pstMsgDes->lSeq;

    lRet = PT_SAVE_DATA_SUCC;

    if (pstMsgDes->lSeq - pstStreamNode->lCurrSeq == lCacheSize)
    {
        /* 缓存已经存满 */
        ulNextSendArraySub =  (pstStreamNode->lCurrSeq + 1) & (lCacheSize - 1);
        if (pstDataQueHead[ulNextSendArraySub].lSeq == pstStreamNode->lCurrSeq + 1)
        {
            /* 需要执行接收线程 */
            lRet = PT_NEED_CUT_PTHREAD;
        }
        else
        {
            /* TODO 丢包，失败，关闭socket */
            return PT_SAVE_DATA_FAIL;
        }
    }

    return lRet;
}

/**
 * 函数：PT_CC_CB_ST *pt_ptc_node_create(U8 *pcIpccId, S8 *szPtcVersion, struct sockaddr_in stDestAddr, S32 lSocket)
 * 功能：
 *      1.创建ptc节点
 * 参数
 *      U8 *pcIpccId     : PTC设备ID
 *      S8 *szPtcVersion : 登陆的PTC的版本号
 *      struct sockaddr_in stDestAddr : PTC的地址
 *      S32 lSocket     : udp 套接字
 * 返回值：void *
 */
PT_CC_CB_ST *pt_ptc_node_create(U8 *pcIpccId, S8 *szPtcVersion, struct sockaddr_in stDestAddr, S32 lSocket)
{
    PT_CC_CB_ST *pstNewNode = NULL;

    if (NULL == pcIpccId)
    {
        pt_logr_debug("create ptc node : age is NULL");
        return NULL;
    }

    pstNewNode = (PT_CC_CB_ST *)dos_dmem_alloc(sizeof(PT_CC_CB_ST));
    if (NULL == pstNewNode)
    {
        perror("dos_dmem_malloc");
        return NULL;
    }

    dos_memcpy(pstNewNode->aucID, pcIpccId, PTC_ID_LEN);
    pstNewNode->lSocket = lSocket;
    pstNewNode->stDestAddr = stDestAddr;

    pstNewNode->astDataTypes[PT_DATA_CTRL].enDataType = PT_DATA_CTRL;
    pstNewNode->astDataTypes[PT_DATA_CTRL].bValid = DOS_TRUE;
    pstNewNode->astDataTypes[PT_DATA_CTRL].pstStreamQueHead = NULL;
    pstNewNode->astDataTypes[PT_DATA_CTRL].ulStreamNumInQue = 0;
    pstNewNode->astDataTypes[PT_DATA_WEB].enDataType = PT_DATA_WEB;
    pstNewNode->astDataTypes[PT_DATA_WEB].bValid = DOS_FALSE;
    pstNewNode->astDataTypes[PT_DATA_WEB].pstStreamQueHead = NULL;
    pstNewNode->astDataTypes[PT_DATA_WEB].ulStreamNumInQue = 0;
    pstNewNode->astDataTypes[PT_DATA_CMD].enDataType = PT_DATA_CMD;
    pstNewNode->astDataTypes[PT_DATA_CMD].bValid = DOS_FALSE;
    pstNewNode->astDataTypes[PT_DATA_CMD].pstStreamQueHead = NULL;
    pstNewNode->astDataTypes[PT_DATA_CMD].ulStreamNumInQue = 0;
    pstNewNode->ulUdpLostDataCount = 0;
    pstNewNode->ulUdpRecvDataCount = 0;
    pstNewNode->usHBOutTimeCount = 0;
    pstNewNode->stHBTmrHandle = NULL;

    if (dos_strcmp(szPtcVersion, "1.1") == 0)
    {
        /* 1.1版本，支持web和cmd */
        pstNewNode->astDataTypes[PT_DATA_WEB].bValid = DOS_TRUE;
        pstNewNode->astDataTypes[PT_DATA_CMD].bValid = DOS_TRUE;
    }

    return pstNewNode;
}

/**
 * 函数：list_t *pt_ptc_list_insert(list_t *pstPtcListHead, PT_CC_CB_ST *pstPtcNode)
 * 功能：
 *      1.将新的ptc节点插入到ptc链表中
 * 参数
 *      list_t *pstPtcListHead  : 链表头结点
 *      PT_CC_CB_ST *pstPtcNode : 需要添加的节点
 * 返回值：void *
 */
list_t *pt_ptc_list_insert(list_t *pstPtcListHead, PT_CC_CB_ST *pstPtcNode)
{
    if (NULL == pstPtcNode)
    {
        pt_logr_debug("ptc list insert: new node is NULL");
        return pstPtcListHead;
    }

    if (NULL == pstPtcListHead)
    {
        pstPtcListHead = &(pstPtcNode->stCCListNode);
        dos_list_init(pstPtcListHead);
    }
    else
    {
        dos_list_add_tail(pstPtcListHead, &(pstPtcNode->stCCListNode));
    }

    return pstPtcListHead;
}

/**
 * 函数：list_t *pt_stream_queue_insert(list_t *pstHead, list_t  *pstStreamNode)
 * 功能：
 *      1.链表中插入新的节点
 * 参数
 *      list_t  *pstHead        ：stream头节点
 *      list_t  *pstStreamNode  ：需要插入的节点
 * 返回值：返回头节点
 */
list_t *pt_stream_queue_insert(list_t *pstHead, list_t  *pstStreamNode)
{
    if (NULL == pstStreamNode)
    {
        return pstHead;
    }

    if (NULL == pstHead)
    {
        pstHead = pstStreamNode;
        dos_list_init(pstHead);
    }
    else
    {
        dos_list_add_tail(pstHead, pstStreamNode);
    }

    return pstHead;
}


/**
 * 函数：PT_STREAM_CB_ST *pt_stream_queue_search(list_t *pstStreamListHead, U32 ulStreamID)
 * 功能：
 *      1.在stream链表中，查找stream ID为ulStreamID的节点
 * 参数
 *      list_t *pstStreamListHead    ：stream头结点
 *      U32 ulStreamID               ：需要查找的stream ID
 * 返回值：找到到返回stream node地址，未找到返回NULL
 */
PT_STREAM_CB_ST *pt_stream_queue_search(list_t *pstStreamListHead, U32 ulStreamID)
{
    if (NULL == pstStreamListHead)
    {
        pt_logr_debug("search stream node in list : empty list!");
        return NULL;
    }

    list_t *pstNode = NULL;
    PT_STREAM_CB_ST *pstData = NULL;

    pstNode = pstStreamListHead;
    pstData = dos_list_entry(pstNode, PT_STREAM_CB_ST, stStreamListNode);

    while (pstData->ulStreamID != ulStreamID && pstNode->next != pstStreamListHead)
    {
        pstNode = pstNode->next;
        pstData = dos_list_entry(pstNode, PT_STREAM_CB_ST, stStreamListNode);
    }

    if (pstData->ulStreamID == ulStreamID)
    {
        return pstData;
    }
    else
    {
        pt_logr_debug("search stream node in list : not found!");
        return NULL;
    }
}

/**
 * 函数：list_t *pt_delete_stream_resource(list_t *pstStreamListHead, list_t *pstStreamListNode, PT_DATA_TYPE_EN enDataType)
 * 功能：
 *      1.释放一个stream
 * 参数
 *      list_t *pstStreamListHead  ：
 *      list_t *pstStreamListNode  ：
 *      PT_DATA_TYPE_EN enDataType ：套接字
 * 返回值：
 */
list_t *pt_delete_stream_node(list_t *pstStreamListHead, list_t *pstStreamListNode, PT_DATA_TYPE_EN enDataType)
{
    if (NULL == pstStreamListHead || NULL == pstStreamListNode || enDataType >= PT_DATA_BUTT)
    {
        return pstStreamListHead;
    }
    PT_STREAM_CB_ST *pstStreamNode = dos_list_entry(pstStreamListNode, PT_STREAM_CB_ST, stStreamListNode);
    if (pstStreamListHead == pstStreamListNode)
    {
        /* 释放第一个节点，头结点发生改变 */
        if (pstStreamListHead == pstStreamListHead->next)
        {
            pstStreamListHead = NULL;
        }
        else
        {
            pstStreamListHead = pstStreamListNode->next;
        }
    }
    dos_list_del(pstStreamListNode);

    if (g_aenDataProtType[enDataType] == PT_UDP)
    {
        /* 需要释放存储data的空间 */
    }
    else
    {
        /* TCP */
        if (pstStreamNode->hTmrHandle != NULL)
        {
            dos_tmr_stop(&pstStreamNode->hTmrHandle);
            pstStreamNode->hTmrHandle = NULL;
        }
        if (pstStreamNode->pstLostParam != NULL)
        {
            pstStreamNode->pstLostParam->pstStreamNode = NULL;
            dos_dmem_free(pstStreamNode->pstLostParam);
            pstStreamNode->pstLostParam = NULL;
        }
        if (pstStreamNode->unDataQueHead.pstDataTcp != NULL)
        {
            dos_dmem_free(pstStreamNode->unDataQueHead.pstDataTcp);
            pstStreamNode->unDataQueHead.pstDataTcp = NULL;
        }
        dos_dmem_free(pstStreamNode);
        pstStreamNode = NULL;
    }
    return pstStreamListHead;
}

/**
 * 函数：list_t *pt_delete_ptc_node(list_t *stPtcListHead, PT_CC_CB_ST *pstPtcNode)
 * 功能：
 *      1.释放ptc下的所有申请的空间，删除ptc节点
 * 参数
 * 返回值：链表头结点
 */
list_t *pt_delete_ptc_node(list_t *stPtcListHead, PT_CC_CB_ST *pstPtcNode)
{
    if (NULL == stPtcListHead || NULL == pstPtcNode)
    {
        return stPtcListHead;
    }

    //printf("pt ptc list delete pucID : %.16s\n", pstPtcNode->aucID);
    S32 i = 0;
    list_t *pstStreamListHead = NULL;

    /* 释放掉cc下所有的stream */
    for (i=0; i<PT_DATA_BUTT; i++)
    {
        pstStreamListHead = pstPtcNode->astDataTypes[i].pstStreamQueHead;
        if (pstStreamListHead != NULL)
        {
            while (pstStreamListHead->next != pstStreamListHead)
            {
                pstStreamListHead = pt_delete_stream_node(pstStreamListHead, pstStreamListHead, i);
            }
            pt_delete_stream_node(pstStreamListHead, pstStreamListHead, i);
        }
    }
    /* 释放掉ptc */
    if (stPtcListHead == &pstPtcNode->stCCListNode)
    {
        if (stPtcListHead->next == stPtcListHead)
        {
            stPtcListHead = NULL;
        }
        else
        {
            stPtcListHead = stPtcListHead->next;
        }
    }
    dos_list_del(&pstPtcNode->stCCListNode);
    dos_dmem_free(pstPtcNode);
    pstPtcNode = NULL;
    return stPtcListHead;
}

/**
 * 函数：PT_CC_CB_ST *pt_ptc_list_search(list_t* pstHead, U8 *pucID)
 * 功能：
 *      1.根据ptc ID 在ptc链表中查找ptc节点
 * 参数
 * 返回值：
 */
PT_CC_CB_ST *pt_ptc_list_search(list_t* pstHead, U8 *pucID)
{
    list_t  *pstNode = NULL;
    PT_CC_CB_ST *pstData = NULL;
    S32 lResult = 0;
    //printf("pt ptc list search pucID : %.16s\n", pucID);
    if (NULL == pstHead)
    {
        //printf("search ptc list : not found!\n");
        pt_logr_debug("%s", "search ptc list : empty list!");
        return NULL;
    }
    else if (NULL == pucID)
    {
        //printf("search ptc list : not found!\n");
        return NULL;
    }

    pstNode = pstHead;
    pstData = dos_list_entry(pstNode, PT_CC_CB_ST, stCCListNode);
    while (dos_memcmp(pstData->aucID, pucID, PTC_ID_LEN) && pstNode->next!=pstHead)
    {
        pstNode = pstNode->next;
        pstData = dos_list_entry(pstNode, PT_CC_CB_ST, stCCListNode);
    }

    lResult = dos_memcmp(pstData->aucID, pucID, PTC_ID_LEN);
    if (lResult == 0)
    {
        return pstData;
    }
    else
    {
        //printf("search ptc list : not found!\n");
        pt_logr_debug("search ptc list : not found!");
        return NULL;
    }
}

/**
 * 函数：list_t *pt_need_recv_node_list_search(list_t *pstHead, U32 ulStreamID)
 * 功能：
 *      1.在需要接受消息队列中，查找ulStreamID是否存在(ExitNotifyFlag为true状态的除外)
 * 参数
 * 返回值：
 */
list_t *pt_need_recv_node_list_search(list_t *pstHead, U32 ulStreamID)
{
    list_t  *pstNode = NULL;
    PT_NEND_RECV_NODE_ST *pstData = NULL;

    if (NULL == pstHead)
    {
        pt_logr_debug("search need recv node list : empty list!");
        return NULL;
    }

    pstNode = pstHead;
    pstData = dos_list_entry(pstNode, PT_NEND_RECV_NODE_ST, stListNode);
    while (pstNode->next != pstHead)
    {
        if (pstData->ulStreamID == ulStreamID)
        {
            if (!pstData->ExitNotifyFlag)
            {
                break;
            }

        }
        pstNode = pstNode->next;
        pstData = dos_list_entry(pstNode, PT_NEND_RECV_NODE_ST, stListNode);
    }

    if (pstData->ulStreamID == ulStreamID)
    {
        return &pstData->stListNode;
    }
    else
    {
        pt_logr_debug("search need recv node list : not found!");
        return NULL;
    }
}

/**
 * 函数：list_t *pt_need_recv_node_list_insert(list_t *pstHead, PT_MSG_TAG *pstMsgDes)
 * 功能：
 *      1.需要接受消息队列队列中插入新节点
 * 参数
 * 返回值：
 */
list_t *pt_need_recv_node_list_insert(list_t *pstHead, PT_MSG_TAG *pstMsgDes)
{
    if (NULL == pstMsgDes)
    {
        pt_logr_debug("pt_need_recv_node_list_insert : arg pstMsgDes is NULL");
        return pstHead;
    }

    PT_NEND_RECV_NODE_ST *pstNewNode = (PT_NEND_RECV_NODE_ST *)dos_dmem_alloc(sizeof(PT_NEND_RECV_NODE_ST));
    if (NULL == pstNewNode)
    {
        perror("malloc");
        return pstHead;
    }

    dos_memcpy(pstNewNode->aucID, pstMsgDes->aucID, PTC_ID_LEN);
    pstNewNode->enDataType = pstMsgDes->enDataType;
    pstNewNode->ulStreamID = pstMsgDes->ulStreamID;
    pstNewNode->ExitNotifyFlag = pstMsgDes->ExitNotifyFlag;

    if (NULL == pstHead)
    {
        pstHead = &(pstNewNode->stListNode);
        dos_list_init(pstHead);
    }
    else
    {
        dos_list_add_tail(pstHead, &(pstNewNode->stListNode));
    }

    return pstHead;
}

/**
 * 函数：list_t *pt_need_send_node_list_search(list_t *pstHead, U32 ulStreamID)
 * 功能：
 *      1.发送消息队列中查找ulStreamID的节点
 * 参数
 * 返回值：
 */
list_t *pt_need_send_node_list_search(list_t *pstHead, U32 ulStreamID)
{
    list_t  *pstNode = NULL;
    PT_NEND_SEND_NODE_ST *pstData = NULL;

    if (NULL == pstHead)
    {
        pt_logr_debug("search need send node list : empty list!");
        return NULL;
    }

    pstNode = pstHead;
    pstData = dos_list_entry(pstNode, PT_NEND_SEND_NODE_ST, stListNode);
    while (pstNode->next != pstHead)
    {
        if (pstData->ulStreamID == ulStreamID)
        {
            if (pstData->enCmdValue == PT_CMD_NORMAL && pstData->bIsResend != DOS_TRUE)
            {
                break;
            }
        }

        pstNode = pstNode->next;
        pstData = dos_list_entry(pstNode, PT_NEND_SEND_NODE_ST, stListNode);
    }

    if (pstData->ulStreamID == ulStreamID)
    {
        return &pstData->stListNode;
    }
    else
    {
        pt_logr_debug("search need recv node list : not found!");
        return NULL;
    }
}

/**
 * 函数：list_t *pt_need_send_node_list_insert(list_t *pstHead, U8 *aucID, PT_MSG_TAG *pstMsgDes, PT_CMD_EN enCmdValue, BOOL bIsResend)
 * 功能：
 *      1.发送消息队列中插入新节点
 * 参数
 * 返回值：队列头地址
 */
list_t *pt_need_send_node_list_insert(list_t *pstHead, U8 *aucID, PT_MSG_TAG *pstMsgDes, PT_CMD_EN enCmdValue, BOOL bIsResend)
{
    if (NULL == aucID || NULL == pstMsgDes)
    {
        pt_logr_debug("pt_need_recv_node_list_insert : arg is NULL");
        return pstHead;
    }

    PT_NEND_SEND_NODE_ST *pstNewNode = (PT_NEND_SEND_NODE_ST *)dos_dmem_alloc(sizeof(PT_NEND_SEND_NODE_ST));
    if (NULL == pstNewNode)
    {
        perror("malloc");
        return pstHead;
    }

    dos_memcpy(pstNewNode->aucID, aucID, PTC_ID_LEN);
    pstNewNode->enDataType = pstMsgDes->enDataType;
    pstNewNode->ulStreamID = pstMsgDes->ulStreamID;
    pstNewNode->lSeqResend = pstMsgDes->lSeq;
    pstNewNode->enCmdValue = enCmdValue;
    pstNewNode->bIsResend = bIsResend;
    pstNewNode->ExitNotifyFlag = pstMsgDes->ExitNotifyFlag;

    if (NULL == pstHead)
    {
        pstHead = &pstNewNode->stListNode;
        dos_list_init(pstHead);
    }
    else
    {
        dos_list_add_tail(pstHead, &pstNewNode->stListNode);
    }

    return pstHead;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */
